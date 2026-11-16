// webmem.js: what a game's web build holds in memory at a first board, and the processor time it takes to get there and
// to sit there, in headless Edge (or Chrome) over the DevTools protocol with Node's own WebSocket (Node 22 or later; no
// Puppeteer). docs/MEMORY.md, "The web builds".
//
//   node tools/webtest/webmem.js --browser <msedge.exe> --profile <dir> --out <results.jsonl>
//        --root <label>=<dir>[;NAME=VALUE,...] [--root ...] [--games a,b,...] [--reps 2] [--settle 500] [--hold 20]
//        [--port 18795] [--env NAME=VALUE,...] [--sw] [--frames native|worker] [--frame-ms 16] [--hide] [--realfocus] [--detail <dir>]
//        [--browser-flag <flag> ...] [--cadence] [--growstack] [--audio] [--menu]
//
// tools/webtest/serve.py serves each root in turn on one port (one origin, so one test profile: a game's files are
// imported from the staged assets once, by the page's own import), with --isolate, since the tab's own figure
// (performance.measureUserAgentSpecificMemory) needs a cross-origin isolated page. Each game first runs once unmeasured
// (the import, if its files are not in the profile yet), then --reps rounds of a run of each root, their order turned
// about each round. A run opens a new tab at <game>.html?player=1&assets=1&hold=1&silent=1 (real time), starts the game,
// sends its start commands (seed 1 and a goto to a first board; refused commands are tried again while the game loads),
// lets --settle updates pass and holds the board --hold seconds, then reads:
//   heap          the WebAssembly memory (Module.HEAPU8.length): it never shrinks, so this is the peak
//   jsUsed        the JS heap in use (performance.memory, exact with --enable-precise-memory-info)
//   tab           performance.measureUserAgentSpecificMemory's bytes: the tab's JS, DOM and WebAssembly memory
//   memreport     the game's own memreport line (docs/TEST_COMMANDS.md): pictures, sounds, music, the heap in use
//   private       the renderer processes' private bytes and peak working set (Windows' counters, by process id)
//   cpu           the renderer and GPU processes' processor seconds (SystemInfo.getProcessInfo): from the tab's
//                 start to the board (load), and over the hold (hold), with the frames drawn and updates run in it
// --frames native: once at the board, the page's frames are the browser's own animation frames only, as on the site's own
// page (the player-only page races each against a 20 ms timer, for a hidden Browser pane; ?frames=browser keeps that race).
// --frames worker: once at the board, a frame each time a Worker's interval (--frame-ms) ticks, which a hidden tab does not slow (its
// own timers wake once a second). --hide: the hold is spent with another tab in front, so the game's tab is hidden (row.hidden:
// the browser's visibilityState then). The test kit tells the game its page is visible and swallows visibilitychange unless
// --realfocus (?realfocus=1), which lets the game see the page hidden as on the site.
// After each run the test profile's saves are wiped, so that every run starts on a clean profile. Each run appends one JSON line. Run it under build/native/rundesk.exe, and count the browser as a game process.
"use strict";
const fs = require("fs");
const path = require("path");
const { spawn, execFileSync } = require("child_process");

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
const PROFILE = path.resolve(arg("profile"));
const OUT = path.resolve(arg("out"));
// --root <label>=<dir>[;NAME=VALUE,...]: a build, and environment variables of its own for the game (?env=), added to --env's
const ROOTS = args("root").map(r => {
	const i = r.indexOf("="), j = r.indexOf(";");
	return { label: r.slice(0, i), dir: path.resolve(r.slice(i + 1, j < 0 ? undefined : j)), env: j < 0 ? "" : r.slice(j + 1) };
});
const REPS = Number(arg("reps", "2"));
const SETTLE = Number(arg("settle", "500"));
const HOLD = Number(arg("hold", "20"));
const PORT = Number(arg("port", "18795"));
const ENV = arg("env", "");
const SW = process.argv.includes("--sw");
const FRAMES = arg("frames", "");
// --frame-ms <n>: the Worker's interval for --frames worker (16 by default; 8 stands for a 120 Hz display's steady frames)
const FRAME_MS = Number(arg("frame-ms", "16"));
const HIDE = process.argv.includes("--hide");
const REALFOCUS = process.argv.includes("--realfocus");
// --cadence: row.cadence, how many updates each drawn frame of the hold followed ({updates: {n: frames}}) and how many browser
// frames it took ({frames: {n: frames}}), and the first 600 drawn frames' updates in order (seq, one digit each): a game never
// presents more often than it updates, and how even its motion is
const CADENCE = process.argv.includes("--cadence");
// --menu: times.menu, when the started game first shows its main menu, its title or the new player's dialog (the state's
// screen menu, main_menu (Venice), title or newplayer; looked at every 20 ms from the first frame, for up to 90 s, before the
// start's commands),
// and times.screens, each screen it showed on the way with its time ([screen, ms since the page's start])
const MENU = process.argv.includes("--menu");
// --growstack: row.grows, each growth of the WebAssembly memory with its time and the JavaScript stack that asked for it, whose
// WebAssembly frames are named in a build linked with --profiling-funcs (what set the heap's peak)
const GROWSTACK = process.argv.includes("--growstack");
// --audio: row.audio, what the game sent to the speakers (the chunks platform/Audio.cpp's page output posts to its worklet,
// counted before they leave): up to the board (atBoard), from there until the tab is hidden and the half second after
// (toHide, with --hide), and over the rest of the hold (hold: with --hide, the tab hidden throughout), each as {chunks, loud,
// peak}, loud being the chunks with a sample above one step of 16 bits (the peak is each part's own); and the AudioContext's
// state at the hold's end. The
// page is opened with ?silent=0, so that the mix is not zeroed, and the browser with --mute-audio (nothing reaches the
// speakers) and --autoplay-policy=no-user-gesture-required (the context runs without a click)
const AUDIO = process.argv.includes("--audio");
const AUDIO_HOOK = `(() => {
	const a = window.__aud = { chunks: 0, loud: 0, peak: 0 };
	const post = MessagePort.prototype.postMessage;
	const measure = (d) => {
		if (!(d instanceof Float32Array)) return;
		let m = 0;
		for (let i = 0; i < d.length; i++) { const v = Math.abs(d[i]); if (v > m) m = v; }
		a.chunks++;
		if (m > 1 / 32768) a.loud++;
		if (m > a.peak) a.peak = m;
	};
	MessagePort.prototype.postMessage = function (d) {
		try { measure(d); if (d && d.lead) measure(d.lead); } catch (e) {}
		return post.apply(this, arguments);
	};
})()`;
const AUDIO_READ = "(() => { const a = window.__aud || {}; const c = window.Module && Module.SDL2 && Module.SDL2.audioContext; " +
	"const r = { chunks: a.chunks | 0, loud: a.loud | 0, peak: a.peak || 0, ctx: c ? c.state : null }; a.peak = 0; return r; })()";
const GROW_HOOK = `(() => {
	Error.stackTraceLimit = 60;
	window.__grows = [];
	const grow = WebAssembly.Memory.prototype.grow;
	WebAssembly.Memory.prototype.grow = function (pages) {
		const before = this.buffer.byteLength;
		const r = grow.call(this, pages);
		window.__grows.push({ ms: performance.now(), from: before, to: this.buffer.byteLength, stack: new Error().stack });
		return r;
	};
})()`;
// --detail <dir>: each run's memreport detail (every picture with its copies, the heap) kept as <game>-<root>-<rep>.txt there
const DETAIL = arg("detail", "");
const sleep = ms => new Promise(r => setTimeout(r, ms));
const log = (...a) => console.log(new Date().toISOString().slice(11, 19), ...a);

const START = require("./webstarts.js");
const GAMES = arg("games", Object.keys(START).join(",")).split(",");

// ---- the server: serve.py on one port, restarted for each root ----
let server = null, serverRoot = null;
async function serve(dir) {
	if (serverRoot === dir) return;
	await stopServer();
	server = spawn("python", [path.join(ROOT, "tools/webtest/serve.py"), "--port", String(PORT), "--root", dir, "--isolate"],
		{ stdio: ["ignore", "pipe", "inherit"] });
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
async function until(fn, ms, what) {
	const t0 = Date.now();
	for (;;) {
		const v = await fn();
		if (v) return v;
		if (Date.now() - t0 > ms) throw new Error("timed out: " + what);
		await sleep(250);
	}
}

// the browser's processes: {renderers: [{id, cpu}], gpu: cpu seconds}
async function processes() {
	const { processInfo } = await send("SystemInfo.getProcessInfo");
	return { renderers: processInfo.filter(p => p.type === "renderer").map(p => ({ id: p.id, cpu: p.cpuTime })),
		gpu: processInfo.filter(p => p.type === "GPU").reduce((s, p) => s + p.cpuTime, 0) };
}
// Windows' counters of the given processes, in bytes
function counters(ids) {
	if (!ids.length) return [];
	// a renderer that has ended since (a spare, a closed frame's) is left out
	const ps = "Get-Process -Id " + ids.join(",") + " -ErrorAction SilentlyContinue | ForEach-Object { '{0} {1} {2} {3}' -f $_.Id, $_.PrivateMemorySize64, " +
		"$_.WorkingSet64, $_.PeakWorkingSet64 }";
	let out;
	// PowerShell exits with 1 after a left-out process, with the others printed
	try { out = execFileSync("powershell", ["-NoProfile", "-Command", ps], { encoding: "utf8", stdio: ["ignore", "pipe", "ignore"] }); }
	catch (e) { out = String(e.stdout || ""); }
	return out.trim().split(/\r?\n/).filter(Boolean).map(l => {
		const [id, priv, ws, peak] = l.trim().split(/\s+/).map(Number);
		return { id, private: priv, workingSet: ws, peakWorkingSet: peak };
	});
}

// in the page: the game started and taken to its board; resolves with the times (ms since the page's start)
function startScript(lines) {
	return `(async () => {
		const t0 = performance.now();
		const t = await new Promise(r => { const p = () => (window.byopop && byopop.test) ? r(byopop.test) : performance.now() - t0 > 120000 ? r(null) : setTimeout(p, 50); p(); });
		if (!t) return { error: "no byopop.test on the page after 120 s" };
		const T = {};
		await t.loaded; T.loaded = performance.now();
		await t.start(); T.firstFrame = performance.now();
		if (${MENU}) {
			T.screens = [];
			let last;
			for (const end = performance.now() + 90000; performance.now() < end; ) {
				let screen = null;
				try { screen = (await t.state()).screen; } catch (e) {}
				if (screen !== last) { T.screens.push([screen, Math.round(performance.now())]); last = screen; }
				if (screen === "menu" || screen === "main_menu" || screen === "title" || screen === "newplayer") { T.menu = performance.now(); break; }
				await new Promise(r => setTimeout(r, 20));
			}
		}
		// the dialogs open: a count, or a list of their ids (Hammer Heads)
		const dialogsOf = (s) => Array.isArray(s.dialogs) ? s.dialogs.length : (s.dialogs | 0);
		for (const line of ${JSON.stringify(["seed 1"].concat(lines))}) {
			// "@name X": X typed into the new player's dialog that a fresh profile opens on the title, when one comes within
			// 60 s (Bookworm Adventures' web driver, games/bookwormadv/tools/web/moveloop.js)
			if (line.startsWith("@name ")) {
				let dialog = false;
				for (let i = 0; i < 600 && !dialog; i++) {
					try { const s = await t.state(); dialog = dialogsOf(s) > 0 || s.screen === "newplayer"; } catch (e) {}
					if (!dialog) await t.wait(10);
				}
				if (dialog) {
					const [name, okX, okY] = line.slice(6).split(" ");
					await t.type(name);
					if (okY !== undefined) await t.click(Number(okX), Number(okY)); else await t.key("Enter");
					T.named = true;
				}
				continue;
			}
			// "@click X Y": clicked until the game's state changes, for up to 60 s
			if (line.startsWith("@click ")) {
				const [x, y] = line.slice(7).split(" ").map(Number);
				let before = null;
				try { before = JSON.stringify(await t.state()); } catch (e) {}
				for (let i = 0; i < 60; i++) {
					await t.click(x, y);
					await t.wait(300);
					let now = null;
					try { now = JSON.stringify(await t.state()); } catch (e) {}
					if (now !== before) break;
					await t.wait(700);
				}
				continue;
			}
			// after a name, the conversations and dialogs that come are clicked away by the bot's moves before each command,
			// the curtain let finish first (Bookworm Adventures' web driver)
			for (let k = 0; T.named && k < 300; k++) {
				let s = null;
				try { s = await t.state(); } catch (e) { break; }
				if (s.curtain) { await t.wait(10); continue; }
				if (!s.talk && !dialogsOf(s)) break;
				try { const m = await t.botMove(); if (m) await t.playMove(m); } catch (e) {}
				await t.wait(10);
			}
			let ok = false, last = null;
			for (let i = 0; i < 240 && !ok; i++) {
				try { const r = await t.command(line, { timeout: 20000 }); ok = !r.some(l => /^ERROR /.test(l)); last = r.slice(-2); }
				catch (e) { last = String(e && e.message || e); }
				// a game that takes no seed this way refuses it for good (the Mahjong Escapes, Bejeweled Twist): go on without
				if (!ok && line === "seed 1") { T.noSeed = JSON.stringify(last); break; }
				// a screen the command cannot leave (Big Money's title, which waits for a click on its play link; Bookworm
				// Adventures' tutorial, whose goto waits 20 s in the game before it gives up): the port's bot plays a move
				// after each refusal, as the games' own drivers do
				if (!ok) {
					try { const m = await t.botMove(); if (m) { await t.playMove(m); T.botMoves = (T.botMoves || 0) + 1; } } catch (e) {}
				}
				if (!ok) await new Promise(r => setTimeout(r, 250));
			}
			if (!ok && !T.noSeed) {
				let st = "";
				try { st = JSON.stringify(await t.state()).slice(0, 400); } catch (e) {}
				return { error: "start: " + line, last, state: st };
			}
		}
		T.board = performance.now();
		if (${JSON.stringify(FRAMES)} === "native") {
			// the browser's own animation frames, as the site's page has them: a same-origin iframe's requestAnimationFrame (the
			// test kit's race with a timer is on window's own, which is window's only one in Chromium), stopping as the tab hides
			const f = document.createElement("iframe");
			f.style.cssText = "position:fixed;left:0;top:0;width:2px;height:2px;border:0;opacity:0.01";
			document.body.appendChild(f);
			await new Promise(r => setTimeout(r, 200));
			const w = f.contentWindow;
			window.requestAnimationFrame = w.requestAnimationFrame.bind(w);
			window.cancelAnimationFrame = w.cancelAnimationFrame.bind(w);
			T.frameSource = "native";
		} else if (${JSON.stringify(FRAMES)} === "worker") {
			// a Worker's interval, whose messages a hidden tab still delivers at its rate
			const w = new Worker(URL.createObjectURL(new Blob(["setInterval(function () { postMessage(0); }, ${FRAME_MS});"], { type: "text/javascript" })));
			let queue = [], next = 1;
			w.onmessage = () => { const q = queue; queue = []; const now = performance.now(); for (const e of q) e.fn(now); };
			window.requestAnimationFrame = fn => { queue.push({ id: next, fn }); return next++; };
			window.cancelAnimationFrame = id => { queue = queue.filter(e => e.id !== id); };
			T.frameSource = "worker";
		}
		await t.wait(${SETTLE});
		if (${JSON.stringify(CADENCE)}) {
			// --cadence: for each drawn frame, the updates run since the drawn frame before it, and the browser frames between
			const prev = Module.byopopTestFrame;
			let last = t.updates(), since = 0;
			window.__cadence = { updates: {}, frames: {}, seq: "" };
			Module.byopopTestFrame = function (drew) {
				const r = prev.apply(this, arguments);
				since++;
				if (drew) {
					const u = t.updates(), c = window.__cadence;
					c.updates[u - last] = (c.updates[u - last] || 0) + 1;
					c.frames[since] = (c.frames[since] || 0) + 1;
					if (c.seq.length < 600) c.seq += String(Math.min(9, u - last));
					last = u; since = 0;
				}
				return r;
			};
		}
		T.settled = performance.now();
		const p = t.progress(); T.frames = p.frames; T.updates = p.updates;
		return T;
	})()`;
}
const READ_SCRIPT = `(async () => {
	const t = byopop.test, r = {};
	const p = t.progress(); r.frames = p.frames; r.updates = p.updates;
	r.heap = Module.HEAPU8.length;
	r.jsUsed = performance.memory.usedJSHeapSize; r.jsTotal = performance.memory.totalJSHeapSize;
	r.isolated = self.crossOriginIsolated;
	// the files' bytes in the in-memory file system (MEMFS, IDBFS's copy among them), by the first two folders of the path
	r.files = {};
	const walk = (p, depth) => {
		let names;
		try { names = Module.FS.readdir(p); } catch (e) { return; }
		for (const n of names) {
			if (n === "." || n === "..") continue;
			const q = (p === "/" ? "" : p) + "/" + n;
			if (q === "/dev" || q === "/proc") continue;
			let st;
			try { st = Module.FS.stat(q); } catch (e) { continue; }
			if (Module.FS.isDir(st.mode)) walk(q, depth + 1);
			else {
				const key = q.split("/").slice(0, 3).join("/");
				r.files[key] = (r.files[key] || 0) + st.size;
			}
		}
	};
	walk("/", 0);
	try {
		// with a file, whose last lines give the heap in use (mallinfo; the line's "private" is the whole WebAssembly memory)
		const detail = "/tmp/webmem-memreport.txt";
		const lines = await t.command("memreport " + detail, { timeout: 20000 });
		const m = lines.find(l => l.startsWith("MEMREPORT "));
		r.memreport = m ? JSON.parse(m.slice(10)) : null;
		try {
			const text = new TextDecoder().decode(Module.FS.readFile(detail));
			const h = /heap: ([0-9.]+) MB in use, ([0-9.]+) MB free/.exec(text);
			if (h) r.heapInUse = Number(h[1]) * 1048576, r.heapFree = Number(h[2]) * 1048576;
			if (${JSON.stringify(!!DETAIL)}) r.detail = text;
			Module.FS.unlink(detail);
		} catch (e) {}
	} catch (e) { r.memreport = String(e && e.message || e); }
	if (self.crossOriginIsolated && performance.measureUserAgentSpecificMemory) {
		const m = await performance.measureUserAgentSpecificMemory();
		r.tab = m.bytes;
		r.tabParts = {};
		for (const b of m.breakdown) for (const ty of (b.types.length ? b.types : ["other"])) r.tabParts[ty] = (r.tabParts[ty] || 0) + b.bytes / b.types.length;
	}
	r.errors = (t.errors || []).slice(0, 5);
	return r;
})()`;

async function run(game, root, rep) {
	await serve(root.dir);
	// any tab still open (a restored one, one that would not close) is closed first, so that no page holds a game's lock
	const stray = (await send("Target.getTargets")).targetInfos.filter(t => t.type === "page");
	for (const t of stray) { try { await send("Target.closeTarget", { targetId: t.targetId }); } catch (e) {} }
	if (stray.length) await sleep(1500);
	const { targetId } = await send("Target.createTarget", { url: "about:blank" });
	const { sessionId: sid } = await send("Target.attachToTarget", { targetId, flatten: true });
	await send("Page.enable", {}, sid);
	await send("Runtime.enable", {}, sid);
	if (GROWSTACK) await send("Page.addScriptToEvaluateOnNewDocument", { source: GROW_HOOK }, sid);
	if (AUDIO) await send("Page.addScriptToEvaluateOnNewDocument", { source: AUDIO_HOOK }, sid);
	const before = await processes();
	const known = new Set(before.renderers.map(p => p.id));
	const envs = [ENV, root.env].filter(Boolean).join(",");
	const env = envs ? "&env=" + encodeURIComponent(envs) : "";
	const t0 = Date.now();
	await send("Page.navigate", { url: "http://127.0.0.1:" + PORT + "/" + game + ".html?player=1&assets=1&hold=1&silent=" + (AUDIO ? "0" : "1") + (FRAMES === "browser" ? "&frames=browser" : "") + (REALFOCUS ? "&realfocus=1" : "") + env }, sid);
	const row = { game, root: root.label, rep, at: new Date().toISOString() };
	try {
		// at most four minutes to the board; then where the page stopped (its progress, errors and last lines) goes in the error
		const T = await Promise.race([evaluate(sid, startScript(START[game] || [])), sleep(240000).then(() => null)]);
		if (!T) {
			const at = await Promise.race([evaluate(sid, `(() => { const t = window.byopop && byopop.test; if (!t) return "no byopop.test";
				return JSON.stringify({ progress: t.progress(), errors: (t.errors || []).slice(0, 3), log: t.log().slice(-4).map(l => l.text) }); })()`),
				sleep(10000).then(() => "the page did not answer")]);
			throw new Error("not at the board after 240 s: " + String(at).slice(0, 600));
		}
		if (T.error) throw new Error(T.error + " " + JSON.stringify(T.last));
		row.times = T;
		row.wallToBoard = (Date.now() - t0) / 1000;
		const atBoard = await processes();
		const mine = atBoard.renderers.filter(p => !known.has(p.id));
		const audioAtBoard = AUDIO ? await evaluate(sid, AUDIO_READ) : null;
		let cover = null;
		if (HIDE) {
			cover = (await send("Target.createTarget", { url: "about:blank" })).targetId;
			await send("Target.activateTarget", { targetId: cover });
			await sleep(500);
			// the browser's own state (the test kit's document.visibilityState says "visible" without --realfocus)
			row.hidden = await evaluate(sid, "Object.getOwnPropertyDescriptor(Document.prototype, 'visibilityState').get.call(document)");
		}
		const audioAtHide = (AUDIO && HIDE) ? await evaluate(sid, AUDIO_READ) : audioAtBoard;
		await sleep(HOLD * 1000);
		// the hold's frames, updates and processor time, read before the tab is in front again
		const held = await evaluate(sid, "(() => { const p = byopop.test.progress(); return { frames: p.frames, updates: p.updates, cadence: window.__cadence }; })()");
		const atEnd = await processes();
		const audioAtEnd = AUDIO ? await evaluate(sid, AUDIO_READ) : null;
		if (cover) {
			await send("Target.activateTarget", { targetId });
			await send("Target.closeTarget", { targetId: cover });
		}
		const r = await evaluate(sid, READ_SCRIPT);
		// V8's own count: the JS heap, and the ArrayBuffers' backing stores outside it (the WebAssembly memory, the files)
		row.v8 = await send("Runtime.getHeapUsage", {}, sid);
		const cpuOf = (snap) => snap.renderers.filter(p => mine.some(m => m.id === p.id)).reduce((s, p) => s + p.cpu, 0);
		row.cpu = { load: cpuOf(atBoard), hold: cpuOf(atEnd) - cpuOf(atBoard), gpuLoad: atBoard.gpu - before.gpu, gpuHold: atEnd.gpu - atBoard.gpu,
			renderers: mine.length };
		row.hold = { seconds: HOLD, frames: held.frames - T.frames, updates: held.updates - T.updates };
		if (held.cadence) row.cadence = held.cadence;
		if (AUDIO) {
			const part = (from, to) => ({ chunks: to.chunks - from.chunks, loud: to.loud - from.loud, peak: to.peak });
			row.audio = { atBoard: audioAtBoard, hold: part(audioAtHide, audioAtEnd), ctxAtEnd: audioAtEnd.ctx };
			if (HIDE) row.audio.toHide = part(audioAtBoard, audioAtHide);
		}
		if (GROWSTACK) row.grows = await evaluate(sid, "window.__grows || null");
		row.private = counters(mine.map(p => p.id)).reduce((s, c) => ({ private: s.private + c.private, peakWorkingSet: s.peakWorkingSet + c.peakWorkingSet,
			workingSet: s.workingSet + c.workingSet }), { private: 0, peakWorkingSet: 0, workingSet: 0 });
		Object.assign(row, { heap: r.heap, jsUsed: r.jsUsed, jsTotal: r.jsTotal, tab: r.tab, tabParts: r.tabParts, isolated: r.isolated,
			files: r.files, memreport: r.memreport, heapInUse: r.heapInUse, heapFree: r.heapFree, errors: r.errors });
		if (DETAIL && r.detail) {
			fs.mkdirSync(DETAIL, { recursive: true });
			fs.writeFileSync(path.join(DETAIL, game + "-" + root.label + "-" + rep + ".txt"), r.detail);
		}
	} catch (e) {
		row.error = String(e && e.message || e);
	}
	// closed until it is gone: a tab whose start timed out once stayed open (holding its game's lock) after a close the
	// browser answered with success
	for (let i = 0; i < 5; i++) {
		try { await send("Target.closeTarget", { targetId }); } catch (e) {}
		await sleep(1500);
		if (!(await send("Target.getTargets")).targetInfos.some(t => t.targetId === targetId)) break;
		if (i === 4) row.notClosed = true;
	}
	await cleanProfile(game);
	return row;
}

// The test profile's saves wiped for the next run (byopop.test.profile.wipe), in a tab of the game's page that never starts
// it: the framework saves a game in progress as its page closes, and the next run's start would meet its dialog. Done after
// a run and not before, so that no measured tab loads twice.
async function cleanProfile(game) {
	const { targetId } = await send("Target.createTarget", { url: "about:blank" });
	try {
		const { sessionId: sid } = await send("Target.attachToTarget", { targetId, flatten: true });
		await send("Page.navigate", { url: "http://127.0.0.1:" + PORT + "/" + game + ".html?player=1&assets=1&hold=1&silent=1" }, sid);
		await Promise.race([evaluate(sid, `(async () => {
			const t0 = performance.now();
			while (!(window.byopop && byopop.test) && performance.now() - t0 < 120000) await new Promise(r => setTimeout(r, 50));
			await byopop.test.loaded;
			await byopop.test.profile.wipe();
			return true;
		})()`), sleep(150000)]);
	} catch (e) {
		log(game, "wiping the profile failed: " + String(e && e.message || e).slice(0, 200));
	}
	for (let i = 0; i < 5; i++) {
		try { await send("Target.closeTarget", { targetId }); } catch (e) {}
		await sleep(1000);
		if (!(await send("Target.getTargets")).targetInfos.some(t => t.targetId === targetId)) break;
	}
}

async function main() {
	if (!BROWSER || !ROOTS.length || !arg("profile") || !arg("out")) {
		console.error("usage: node webmem.js --browser <exe> --profile <dir> --out <file> --root <label>=<dir> [...]");
		process.exit(2);
	}
	const unknown = GAMES.filter(g => !START[g]);
	if (unknown.length) {
		console.error("webmem: no start for " + unknown.join(", ") + " (known: " + Object.keys(START).join(", ") + ")");
		process.exit(2);
	}
	fs.mkdirSync(PROFILE, { recursive: true });
	fs.mkdirSync(path.dirname(OUT), { recursive: true });
	try { fs.unlinkSync(path.join(PROFILE, "DevToolsActivePort")); } catch (e) {}
	// no tabs restored from an earlier run's session: one restored after the first tabs were closed held its game's lock,
	// and the next page of that game waited ("Already open") until its start timed out
	for (const f of ["Default/Sessions", "Default/Current Session", "Default/Current Tabs", "Default/Last Session", "Default/Last Tabs"])
		try { fs.rmSync(path.join(PROFILE, f), { recursive: true, force: true }); } catch (e) {}
	const flags = ["--headless", "--remote-debugging-port=0", "--user-data-dir=" + PROFILE, "--window-size=1280,800",
		"--force-device-scale-factor=1", "--mute-audio", "--no-first-run", "--no-default-browser-check", "--disable-extensions",
		"--disable-sync", "--enable-precise-memory-info", "--disable-features=SpareRendererForSitePerProcess"];
	// --browser-flag <flag> (repeatable): added to the browser's command line (--disable-frame-rate-limit and
	// --disable-gpu-vsync: animation frames as fast as the machine allows, as on a fast display)
	flags.push(...args("browser-flag"));
	if (AUDIO) flags.push("--autoplay-policy=no-user-gesture-required");
	if (SW) flags.push("--use-angle=swiftshader", "--enable-unsafe-swiftshader");
	else flags.push("--enable-gpu", "--use-angle=d3d11");
	browser = spawn(BROWSER, flags.concat(["about:blank"]), { stdio: "ignore" });
	const portLine = await until(() => {
		try { return fs.readFileSync(path.join(PROFILE, "DevToolsActivePort"), "utf8").split("\n"); } catch (e) { return null; }
	}, 30000, "DevToolsActivePort");
	ws = new WebSocket("ws://127.0.0.1:" + portLine[0].trim() + portLine[1].trim());
	await new Promise((r, j) => { ws.onopen = r; ws.onerror = j; });
	ws.onmessage = m => {
		const d = JSON.parse(m.data);
		if (d.id && pending.has(d.id)) {
			const p = pending.get(d.id); pending.delete(d.id);
			d.error ? p.reject(new Error(p.method + ": " + d.error.message)) : p.resolve(d.result);
		}
	};
	// the first tab (about:blank) closed, so that each run's renderer is its own
	for (const t of (await send("Target.getTargets")).targetInfos)
		if (t.type === "page") await send("Target.closeTarget", { targetId: t.targetId });
	for (const game of GAMES) {
		const warm = await run(game, ROOTS[0], -1);
		log(game, "warm-up", warm.error ? "ERROR " + warm.error : "ok, board after " + warm.wallToBoard + " s");
		fs.appendFileSync(OUT, JSON.stringify(warm) + "\n");
		if (warm.error) continue;
		for (let rep = 0; rep < REPS; rep++) {
			const order = rep % 2 ? ROOTS.slice().reverse() : ROOTS;
			for (const root of order) {
				const row = await run(game, root, rep);
				fs.appendFileSync(OUT, JSON.stringify(row) + "\n");
				log(game, root.label, rep, row.error ? "ERROR " + row.error :
					"heap " + (row.heap / 1048576).toFixed(1) + " MB, js " + (row.jsUsed / 1048576).toFixed(1) + ", tab " +
					(row.tab ? (row.tab / 1048576).toFixed(1) : "-") + ", private " + (row.private.private / 1048576).toFixed(1) + ", cpu load " +
					row.cpu.load.toFixed(2) + " s hold " + row.cpu.hold.toFixed(2) + " s (gpu " + row.cpu.gpuHold.toFixed(2) + "), " +
					row.hold.frames + " frames");
			}
		}
	}
}

main().catch(e => { console.error(e); process.exitCode = 1; }).finally(async () => {
	// the browser closed as a user closes it (its session saved with no tabs), and killed only if it has not ended in 10 s
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
