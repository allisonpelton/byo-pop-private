// webdet.js: whether two web builds of the games play the same: each game is played the same way in each build, held between
// updates and stepped (lockstep), and every state and shot is compared. For a change that should alter nothing a game does
// (compiler options, docs/MEMORY.md, "The web builds"). Headless Edge (or Chrome) over the DevTools protocol with Node's own
// WebSocket (Node 22 or later).
//
//   node tools/webtest/webdet.js --browser <msedge.exe> --profile <dir> --out <results.jsonl>
//        --root <label>=<dir> --root <label>=<dir> [--games a,b,...] [--reps 1] [--moves 30] [--shot-every 5] [--port 18797]
//
// A run opens <game>.html?player=1&assets=1&hold=1&fast=1&silent=1&drawevery=1&seed=1 (the page's start seed, for the games
// that take no "seed" command), holds the game once its first frame is drawn, and from then on steps it: the start commands
// (tools/webtest/webstarts.js, after "seed 1"; a refused one is sent again after a bot move and 30 updates), then --moves moves
// of the port's bot (the "move" command), each played as DOM input with its waits stepped, followed by 30 updates and a
// "state". Every --shot-every moves the canvas is shot (a PNG, its SHA-256 kept). Each game's first run (the import of its
// files into the profile) is unmeasured. Before each run the test profile's saves are wiped and the page loaded again. A run
// appends one JSON line: {game, root, rep, heldAt, states, shots, moves, error}; at the end each game's runs are compared
// with its first and the first difference is printed.
// The game starts with -seed=1 (the framework's start seed) besides the test kit's own arguments.
// Run it under build/native/rundesk.exe, and count the browser as a game process.
"use strict";
const fs = require("fs");
const path = require("path");
const crypto = require("crypto");
const { spawn } = require("child_process");
const START = require("./webstarts.js");

function args(name) {
	const out = [];
	for (let i = 2; i < process.argv.length; i++) if (process.argv[i] === "--" + name) out.push(process.argv[i + 1]);
	return out;
}
function arg(name, def) {
	const a = args(name);
	return a.length ? a[a.length - 1] : def;
}
const ROOT = path.resolve(__dirname, "../..");
const BROWSER = arg("browser");
const PROFILE = path.resolve(arg("profile", "."));
const OUT = path.resolve(arg("out", "webdet.jsonl"));
const ROOTS = args("root").map(r => { const i = r.indexOf("="); return { label: r.slice(0, i), dir: path.resolve(r.slice(i + 1)) }; });
const REPS = Number(arg("reps", "1"));
const MOVES = Number(arg("moves", "30"));
const SHOT_EVERY = Number(arg("shot-every", "5"));
const PORT = Number(arg("port", "18797"));
const GAMES = arg("games", Object.keys(START).join(",")).split(",");
const sleep = ms => new Promise(r => setTimeout(r, ms));
const log = (...a) => console.log(new Date().toISOString().slice(11, 19), ...a);

// ---- the server: serve.py on one port, restarted for each root ----
let server = null, serverRoot = null;
async function serve(dir) {
	if (serverRoot === dir) return;
	await stopServer();
	server = spawn("python", [path.join(ROOT, "tools/webtest/serve.py"), "--port", String(PORT), "--root", dir], { stdio: ["ignore", "pipe", "inherit"] });
	await new Promise((resolve, reject) => {
		let text = "";
		server.stdout.on("data", d => { text += d; if (text.includes("serving")) resolve(); });
		server.on("exit", c => reject(new Error("serve.py exited with " + c)));
	});
	serverRoot = dir;
}
async function stopServer() {
	if (!server) return;
	const s = server;
	server = null; serverRoot = null;
	s.removeAllListeners("exit");
	await new Promise(r => { s.on("exit", r); s.kill(); });
}

// ---- the DevTools protocol ----
let ws, browser = null, nextId = 1;
const pending = new Map();
function send(method, params = {}, sessionId) {
	const id = nextId++;
	ws.send(JSON.stringify({ id, method, params, sessionId }));
	return new Promise((resolve, reject) => pending.set(id, { resolve, reject, method }));
}
async function evaluate(sid, expr) {
	const r = await send("Runtime.evaluate", { expression: expr, awaitPromise: true, returnByValue: true }, sid);
	if (r.exceptionDetails) throw new Error("evaluate: " + JSON.stringify(r.exceptionDetails).slice(0, 400));
	return r.result.value;
}

// in the page: the game held from its first frame, started, played; resolves with {heldAt, states, shots, moves} or {error}
function playScript(lines) {
	return `(async () => {
		const t0 = performance.now();
		const t = await new Promise(r => { const p = () => (window.byopop && byopop.test) ? r(byopop.test) : performance.now() - t0 > 120000 ? r(null) : setTimeout(p, 50); p(); });
		if (!t) return { error: "no byopop.test on the page after 120 s" };
		await t.loaded;
		// -seed=1, the framework's start seed (SexyAppBase.h): a game that seeds a generator of its own at its start from the
		// clock (AstroPop's board generator) takes no "seed" command for it
		const hooks = window.ByopopShell.hooks, args = hooks.args;
		hooks.args = function () { return (args ? args() : []).concat(["-seed=1"]); };
		await t.start();
		const heldAt = t.pause();
		// a command answered while held: stepped an update at a time until its answer comes (games/bookwormadv/tools/web/moveloop.js)
		const ask = async (f) => {
			let done = false;
			const p = f();
			p.then(() => { done = true; }, () => { done = true; });
			for (let k = 0; k < 6000 && !done; k++) await t.step(1);
			return p;
		};
		// a move played as DOM input, its waits stepped
		// (read as the kit's playMove reads it: a point is [x, y] or {x, y, button}, and a move without "input" lists clicks, then
		// drags, then keys)
		const pt = (v) => Array.isArray(v) ? { x: v[0], y: v[1] } : v;
		const play = async (m) => {
			let steps = Array.isArray(m.input) ? m.input : null;
			if (!steps) {
				steps = [];
				(m.clicks || []).forEach(c => steps.push({ click: c, button: c.button }));
				(m.drags || []).forEach(d => steps.push({ drag: d }));
				(m.keys || []).forEach(k => steps.push({ key: k }));
			}
			for (const s of steps) {
				let p;
				if (s.move) { p = pt(s.move); await t.move(p.x, p.y); }
				else if (s.click) { p = pt(s.click); await t.click(p.x, p.y, s.button || p.button || "left"); }
				else if (s.press) { p = pt(s.press); await t.down(p.x, p.y, s.button || "left"); }
				else if (s.release) { p = pt(s.release); await t.up(p.x, p.y, s.button || "left"); }
				else if (s.drag) await t.drag(s.drag.x0, s.drag.y0, s.drag.x1, s.drag.y1, s.drag.steps || 4);
				else if (s.key) await t.key(s.key);
				else if (s.wait) await t.step(s.wait);
			}
			await t.step(Math.max(1, m.updates | 0));
		};
		const sha = async (url) => Array.from(new Uint8Array(await crypto.subtle.digest("SHA-256", new TextEncoder().encode(url))))
			.map(b => b.toString(16).padStart(2, "0")).join("").slice(0, 16);
		const R = { heldAt, states: [], shots: [], moves: 0, refused: [] };
		let named = false;
		// the dialogs open: a count, or a list of their ids (Hammer Heads)
		const dialogsOf = (s) => Array.isArray(s.dialogs) ? s.dialogs.length : (s.dialogs | 0);
		for (const line of ${JSON.stringify(["seed 1"].concat(lines))}) {
			if (line.startsWith("@name ")) {
				let dialog = false;
				for (let i = 0; i < 60 && !dialog; i++) {
					try { const s = await ask(() => t.state()); dialog = dialogsOf(s) > 0 || s.screen === "newplayer"; } catch (e) {}
					if (!dialog) await t.step(100);
				}
				if (dialog) {
					const [name, okX, okY] = line.slice(6).split(" ");
					await t.type(name);
					if (okY !== undefined) await t.click(Number(okX), Number(okY)); else await t.key("Enter");
					await t.step(30);
					named = true;
				}
				continue;
			}
			if (line.startsWith("@click ")) {
				const [x, y] = line.slice(7).split(" ").map(Number);
				let before = null;
				try { before = JSON.stringify(await ask(() => t.state())); } catch (e) {}
				for (let i = 0; i < 60; i++) {
					await t.click(x, y);
					await t.step(30);
					let now = null;
					try { now = JSON.stringify(await ask(() => t.state())); } catch (e) {}
					if (now !== before) break;
					await t.step(100);
				}
				continue;
			}
			// after a name, the conversations and dialogs that come are clicked away by the bot's moves before each command,
			// the curtain let finish first (Bookworm Adventures' web driver)
			for (let k = 0; named && k < 300; k++) {
				let s = null;
				try { s = await ask(() => t.state()); } catch (e) { break; }
				if (s.curtain) { await t.step(10); continue; }
				if (!s.talk && !dialogsOf(s)) break;
				try { const m = await ask(() => t.botMove()); if (m) await play(m); } catch (e) {}
				await t.step(10);
			}
			let ok = false, last = null;
			// a refusal is tried again after a bot move and 100 updates, for up to 120 tries (Venice's levels take a while to load)
			for (let i = 0; i < 120 && !ok; i++) {
				try { const r = await ask(() => t.command(line, { timeout: 600000 })); last = r[r.length - 1]; ok = /^OK /.test(last) || !/^ERROR /.test(last); }
				catch (e) { last = String(e && e.message || e); }
				if (!ok && line === "seed 1") break;
				if (!ok) {
					try { const m = await ask(() => t.botMove()); if (m) await play(m); } catch (e) {}
					await t.step(100);
				}
			}
			if (!ok) R.refused.push(line + ": " + last);
			if (!ok && line !== "seed 1") {
				let st = "";
				try { st = JSON.stringify(await ask(() => t.state())).slice(0, 400); } catch (e) {}
				return Object.assign(R, { error: "start: " + line + ": " + last + " (state " + st + ")" });
			}
			await t.step(30);
		}
		for (let k = 0; k < ${MOVES} * 3 && R.moves < ${MOVES}; k++) {
			let m = null;
			try { m = await ask(() => t.botMove()); } catch (e) { R.states.push("move: " + String(e && e.message || e)); }
			if (m) { await play(m); R.moves++; }
			await t.step(30);
			try { R.states.push(JSON.stringify(await ask(() => t.state()))); } catch (e) { R.states.push("state: " + String(e && e.message || e)); }
			if (m && R.moves % ${SHOT_EVERY} === 0) R.shots.push(await sha(await t.shot()));
		}
		R.updates = t.updates();
		return R;
	})()`;
}

// the test profile's saves wiped before the game starts (byopop.test.profile.wipe: a game left in progress by the run before,
// which the framework saves as the page closes, would greet this one with its dialog), and the page loaded again
const WIPE = `(async () => {
	const t0 = performance.now();
	while (!(window.byopop && byopop.test) && performance.now() - t0 < 120000) await new Promise(r => setTimeout(r, 50));
	await byopop.test.loaded;
	await byopop.test.profile.wipe();
	return true;
})()`;
async function wipe(sid) {
	await evaluate(sid, WIPE);
	const loaded = new Promise(r => { onLoad = r; });
	await send("Page.reload", { ignoreCache: false }, sid);
	await Promise.race([loaded, sleep(30000)]);
	onLoad = null;
}
let onLoad = null;

async function run(game, root, rep) {
	await serve(root.dir);
	for (const t of (await send("Target.getTargets")).targetInfos.filter(t => t.type === "page"))
		try { await send("Target.closeTarget", { targetId: t.targetId }); } catch (e) {}
	const { targetId } = await send("Target.createTarget", { url: "about:blank" });
	const { sessionId: sid } = await send("Target.attachToTarget", { targetId, flatten: true });
	await send("Page.enable", {}, sid);
	await send("Runtime.enable", {}, sid);
	await send("Page.navigate", { url: "http://127.0.0.1:" + PORT + "/" + game + ".html?player=1&assets=1&hold=1&fast=1&silent=1&drawevery=1&seed=1" }, sid);
	const row = { game, root: root.label, rep };
	try {
		await wipe(sid);
		const R = await Promise.race([evaluate(sid, playScript(START[game] || [])), sleep(600000).then(() => ({ error: "not done after 600 s" }))]);
		Object.assign(row, R);
	} catch (e) {
		row.error = String(e && e.message || e);
	}
	for (let i = 0; i < 5; i++) {
		try { await send("Target.closeTarget", { targetId }); } catch (e) {}
		await sleep(1000);
		if (!(await send("Target.getTargets")).targetInfos.some(t => t.targetId === targetId)) break;
	}
	return row;
}

// the first place two runs differ, or null
function difference(a, b) {
	if (a.error || b.error) return "error: " + (a.error || "-") + " / " + (b.error || "-");
	if (a.heldAt !== b.heldAt) return "held at update " + a.heldAt + " / " + b.heldAt;
	for (let i = 0; i < Math.max(a.states.length, b.states.length); i++)
		if (a.states[i] !== b.states[i]) return "state " + i + ": " + String(a.states[i]).slice(0, 200) + " / " + String(b.states[i]).slice(0, 200);
	for (let i = 0; i < Math.max(a.shots.length, b.shots.length); i++)
		if (a.shots[i] !== b.shots[i]) return "shot " + i + " (after move " + (i + 1) * SHOT_EVERY + ")";
	if (a.updates !== b.updates) return "updates " + a.updates + " / " + b.updates;
	return null;
}

async function main() {
	if (!BROWSER || ROOTS.length < 1 || !arg("profile") || !arg("out")) {
		console.error("usage: node webdet.js --browser <exe> --profile <dir> --out <file> --root <label>=<dir> --root <label>=<dir> [...]");
		process.exit(2);
	}
	const unknown = GAMES.filter(g => !START[g]);
	if (unknown.length) { console.error("webdet: no start for " + unknown.join(", ")); process.exit(2); }
	fs.mkdirSync(PROFILE, { recursive: true });
	fs.mkdirSync(path.dirname(OUT), { recursive: true });
	try { fs.unlinkSync(path.join(PROFILE, "DevToolsActivePort")); } catch (e) {}
	for (const f of ["Default/Sessions", "Default/Current Session", "Default/Current Tabs", "Default/Last Session", "Default/Last Tabs"])
		try { fs.rmSync(path.join(PROFILE, f), { recursive: true, force: true }); } catch (e) {}
	browser = spawn(BROWSER, ["--headless", "--remote-debugging-port=0", "--user-data-dir=" + PROFILE, "--window-size=1280,800",
		"--force-device-scale-factor=1", "--mute-audio", "--no-first-run", "--no-default-browser-check", "--disable-extensions",
		"--disable-sync", "--enable-gpu", "--use-angle=d3d11", "about:blank"], { stdio: "ignore" });
	let portLine = null;
	for (let i = 0; i < 120 && !portLine; i++) {
		try { portLine = fs.readFileSync(path.join(PROFILE, "DevToolsActivePort"), "utf8").split("\n"); } catch (e) { await sleep(250); }
	}
	if (!portLine) throw new Error("no DevToolsActivePort");
	ws = new WebSocket("ws://127.0.0.1:" + portLine[0].trim() + portLine[1].trim());
	await new Promise((r, j) => { ws.onopen = r; ws.onerror = j; });
	ws.onmessage = m => {
		const d = JSON.parse(m.data);
		if (d.method === "Page.loadEventFired" && onLoad) onLoad();
		if (d.id && pending.has(d.id)) {
			const p = pending.get(d.id); pending.delete(d.id);
			d.error ? p.reject(new Error(p.method + ": " + d.error.message)) : p.resolve(d.result);
		}
	};
	let same = 0, differ = 0;
	for (const game of GAMES) {
		const warm = await run(game, ROOTS[0], -1);
		log(game, "warm-up", warm.error ? "ERROR " + warm.error.slice(0, 300) : "ok, " + warm.moves + " moves");
		const rows = [];
		for (let rep = 0; rep < REPS; rep++) {
			for (const root of (rep % 2 ? ROOTS.slice().reverse() : ROOTS)) {
				const row = await run(game, root, rep);
				fs.appendFileSync(OUT, JSON.stringify(row) + "\n");
				rows.push(row);
				log(game, root.label, rep, row.error ? "ERROR " + row.error.slice(0, 300) : row.moves + " moves, " + row.states.length + " states, " +
					row.shots.length + " shots, " + row.updates + " updates");
			}
		}
		const diffs = rows.slice(1).map(r => [r, difference(rows[0], r)]).filter(([, d]) => d);
		if (diffs.length) { differ++; for (const [r, d] of diffs) log(game, "DIFFERS", rows[0].root + " " + rows[0].rep + " / " + r.root + " " + r.rep + ":", d); }
		else { same++; log(game, "SAME in all " + rows.length + " runs"); }
	}
	log("webdet:", same, "games the same,", differ, "differ");
	if (differ) process.exitCode = 1;
}

main().catch(e => { console.error(e); process.exitCode = 1; }).finally(async () => {
	if (browser && ws) {
		const ended = new Promise(r => browser.once("exit", r));
		try { await Promise.race([send("Browser.close"), sleep(3000)]); } catch (e) {}
		await Promise.race([ended, sleep(10000)]);
	}
	try { ws && ws.close(); } catch (e) {}
	if (browser && browser.exitCode === null) browser.kill();
	await stopServer();
	setTimeout(() => process.exit(), 500);
});
