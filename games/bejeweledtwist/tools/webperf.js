// webperf.js: frame times of Bejeweled Twist's web build in headless Edge (or Chrome), driven over the DevTools protocol
// with Node's own WebSocket (Node 22 or later); no Puppeteer or Playwright. NOTES.md, "Performance (web)".
//
//   node webperf.js --browser <msedge.exe> --profile <dir> --zip <twist.zip> --out <results.jsonl>
//                   [--sw] [--size 1280,800] [--dsf 1] [--modes 2d,3d] [--throttles 1,2,4,6] [--reps 3]
//                   [--from 1500] [--to 7500] [--step 500] [--cap 240] [--pilot <dir>]
//
// One browser launch (hardware rendering, or SwiftShader with --sw) serves build/web from this checkout on 127.0.0.1,
// imports the game's files from the zip into the profile once (the page's own import, through its zip input), and then
// for each mode, slowdown and repetition reloads the page with the site's "3D acceleration" set (never for 2D, always for
// 3D), starts the game with -autoplay=1 -seed=1234 in real time, and records every requestAnimationFrame time and the
// times at which the game's own update count passes each marker (a -script screenshot every --step updates, seen as its
// file appears). The CPU slowdown (Emulation.setCPUThrottlingRate) is set once the first marker (--from) is reached, so
// the loading is not slowed, and the stretch ends at --to or after --cap seconds. Each run appends one JSON line.
"use strict";
const fs = require("fs");
const path = require("path");
const http = require("http");
const { spawn } = require("child_process");

function arg(name, def) {
	const i = process.argv.indexOf("--" + name);
	if (i < 0) return def;
	const v = process.argv[i + 1];
	return v === undefined || v.startsWith("--") ? true : v;
}
const ROOT = path.resolve(__dirname, "../../..");
const WEB = path.join(ROOT, "build/web");
const BROWSER = arg("browser");
const PROFILE = path.resolve(arg("profile"));
const ZIP = path.resolve(arg("zip"));
const OUT = path.resolve(arg("out"));
const SW = !!arg("sw", false);
const SIZE = arg("size", "1280,800");
const DSF = arg("dsf", "1");
const MODES = arg("modes", "2d,3d").split(",");
const THROTTLES = arg("throttles", "1,2,4,6").split(",").map(Number);
const REPS = Number(arg("reps", "3"));
const FROM = Number(arg("from", "1500"));
const TO = Number(arg("to", "7500"));
const STEP = Number(arg("step", "500"));
const CAP = Number(arg("cap", "240"));
const PILOT = arg("pilot", null);
const PORT = 18791;
const sleep = ms => new Promise(r => setTimeout(r, ms));
const log = (...a) => console.log(new Date().toISOString().slice(11, 19), ...a);

// ---- a static server for build/web ----
const TYPES = { ".html": "text/html", ".js": "text/javascript", ".wasm": "application/wasm", ".css": "text/css",
	".json": "application/json", ".png": "image/png", ".svg": "image/svg+xml", ".ico": "image/x-icon", ".woff2": "font/woff2",
	".txt": "text/plain", ".data": "application/octet-stream" };
const server = http.createServer((req, res) => {
	const p = path.join(WEB, decodeURIComponent(req.url.split("?")[0]));
	if (!p.startsWith(WEB) || !fs.existsSync(p) || fs.statSync(p).isDirectory()) { res.writeHead(404); res.end(); return; }
	res.writeHead(200, { "Content-Type": TYPES[path.extname(p)] || "application/octet-stream", "Cache-Control": "no-store" });
	fs.createReadStream(p).pipe(res);
});

// ---- the DevTools protocol ----
let ws, browser = null, lastSid = null, nextId = 1;
const pending = new Map();
const events = [];
function send(method, params = {}, sessionId) {
	const id = nextId++;
	ws.send(JSON.stringify({ id, method, params, sessionId }));
	return new Promise((resolve, reject) => pending.set(id, { resolve, reject, method }));
}
async function evaluate(sid, expr, awaitPromise = true) {
	const r = await send("Runtime.evaluate", { expression: expr, awaitPromise, returnByValue: true }, sid);
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

// The page script: rAF times, the markers' times, and the game's start with autoplay, a seed and the markers' script
const INJECT = `(() => {
	window.__perf = { raf: [], marks: {}, started: 0 };
	const raf = window.requestAnimationFrame.bind(window);
	window.requestAnimationFrame = cb => raf(t => { __perf.raf.push(t); cb(t); });
})();`;
function startGame(markers) {
	return `(() => {
		const lines = ${JSON.stringify(markers)}.map(n => n + " screenshot /perf_" + n + ".png");
		Module.FS.writeFile("/perf_script.txt", lines.join("\\n") + "\\n");
		const call = Module.callMain.bind(Module);
		Module.callMain = args => { __perf.started = performance.now(); return call(args.concat(["-autoplay=1", "-seed=1234", "-script=/perf_script.txt"])); };
		const want = ${JSON.stringify(markers)};
		const poll = setInterval(() => {
			for (const n of want)
				if (!(n in __perf.marks) && Module.FS.analyzePath("/perf_" + n + ".png").exists) __perf.marks[n] = performance.now();
		}, 10);
		document.getElementById("btn-start").click();
		return true;
	})()`;
}

function stats(intervals) {
	const s = intervals.slice().sort((a, b) => a - b);
	const q = p => s[Math.min(s.length - 1, Math.floor(p * s.length))];
	return { frames: s.length, median: q(0.5), p95: q(0.95), over16: intervals.filter(x => x > 17).length / s.length,
		over33: intervals.filter(x => x > 34).length / s.length };
}

async function main() {
	await new Promise(r => server.listen(PORT, "127.0.0.1", r));
	fs.mkdirSync(PROFILE, { recursive: true });
	try { fs.unlinkSync(path.join(PROFILE, "DevToolsActivePort")); } catch (e) {}
	const [w, h] = SIZE.split(",");
	const flags = ["--headless", "--remote-debugging-port=0", "--user-data-dir=" + PROFILE, "--window-size=" + w + "," + h,
		"--force-device-scale-factor=" + DSF, "--mute-audio", "--no-first-run", "--no-default-browser-check",
		"--disable-background-timer-throttling", "--disable-renderer-backgrounding", "--disable-backgrounding-occluded-windows",
		"--disable-extensions", "--disable-sync"];
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
		} else if (d.method) events.push(d);
	};
	const { targetId } = await send("Target.createTarget", { url: "about:blank" });
	const { sessionId: sid } = await send("Target.attachToTarget", { targetId, flatten: true });
	lastSid = sid;
	await send("Page.enable", {}, sid);
	await send("Runtime.enable", {}, sid);
	await send("Page.addScriptToEvaluateOnNewDocument", { source: INJECT }, sid);
	const URL = "http://127.0.0.1:" + PORT + "/bejeweledtwist.html?silent=1";
	const load = async () => {
		events.length = 0;
		await send("Page.navigate", { url: URL }, sid);
		await until(() => events.some(e => e.method === "Page.loadEventFired" && e.sessionId === sid), 60000, "load");
	};
	await load();
	const renderer = await evaluate(sid, `(() => { const gl = document.createElement("canvas").getContext("webgl2");
		if (!gl) return "no WebGL2"; const i = gl.getExtension("WEBGL_debug_renderer_info");
		const r = i ? gl.getParameter(i.UNMASKED_RENDERER_WEBGL) : gl.getParameter(gl.RENDERER);
		const l = gl.getExtension("WEBGL_lose_context"); if (l) l.loseContext(); return r; })()`);
	log("renderer:", renderer);

	// the game's files, once a profile
	// the page shows its import card while it has no game files, its start card once it has them
	const card = () => evaluate(sid, `(() => { const i = document.getElementById("import-overlay"), s = document.getElementById("start-overlay");
		return i && !i.hidden ? "import" : s && !s.hidden ? "start" : ""; })()`);
	if ((await until(card, 60000, "a card")) === "import") {
		log("importing", ZIP);
		const doc = await send("DOM.getDocument", {}, sid);
		const { nodeId } = await send("DOM.querySelector", { nodeId: doc.root.nodeId, selector: "#pick-game-zip" }, sid);
		await send("DOM.setFileInputFiles", { files: [ZIP], nodeId }, sid);
		await until(async () => (await card()) === "start", 600000, "import");
		log("imported");
	}

	const markers = [];
	for (let n = FROM; n <= TO; n += STEP) markers.push(n);
	for (const mode of MODES) for (const rate of THROTTLES) for (let rep = 1; rep <= REPS; rep++) {
		await evaluate(sid, `ByopopSettings.set("accel3d", "${mode === "3d" ? "on" : "off"}")`);
		await load();
		await until(() => evaluate(sid, `(() => { const s = document.getElementById("status"); const o = document.getElementById("start-overlay");
			return s && /Ready/.test(s.textContent) && o && !o.hidden; })()`), 60000, "ready");
		await sleep(4000);  // the mod panel is made after the page's files are read
		await evaluate(sid, startGame(markers));
		const t0 = Date.now();
		const first = await until(() => evaluate(sid, `__perf.marks[${FROM}] || 0`), 300000, "first marker");
		if (rate !== 1) await send("Emulation.setCPUThrottlingRate", { rate }, sid);
		await until(async () => (await evaluate(sid, `__perf.marks[${TO}] || 0`)) ||
			Date.now() - t0 > 1000 * 60 * 10 || (await evaluate(sid, `performance.now()`)) - first > CAP * 1000, 1000 * 60 * 15, "stretch");
		if (rate !== 1) await send("Emulation.setCPUThrottlingRate", { rate: 1 }, sid);
		const d = await evaluate(sid, `(() => { const c = document.getElementById("canvas");
			const gl = typeof GL !== "undefined" && GL.currentContext ? GL.currentContext.version : 0;
			return { raf: __perf.raf, marks: __perf.marks, canvas: [c.width, c.height], gl, dpr: devicePixelRatio,
				log: (document.getElementById("output") || {}).value || "" }; })()`);
		const done = Object.keys(d.marks).map(Number).sort((a, b) => a - b);
		const last = done[done.length - 1];
		const tA = d.marks[FROM], tB = d.marks[last];
		const times = d.raf.filter(t => t >= tA && t <= tB);
		// the frame of each marker's screenshot (a draw and a PNG of the screen) is left out
		const marks = done.map(n => d.marks[n]);
		const intervals = times.slice(1).map((t, i) => t - times[i]).filter((x, i) => !marks.some(m => times[i + 1] >= m - 50 && times[i + 1] <= m + 150));
		const r = { sw: SW, size: SIZE, dsf: DSF, renderer, mode, rate, rep, canvas: d.canvas, gl: d.gl, dpr: d.dpr,
			from: FROM, to: last, seconds: (tB - tA) / 1000, ups: (last - FROM) * 1000 / (tB - tA), fps: intervals.length * 1000 / (tB - tA),
			...stats(intervals) };
		log(JSON.stringify(r));
		fs.appendFileSync(OUT, JSON.stringify(r) + "\n");
		if (PILOT) {
			fs.mkdirSync(PILOT, { recursive: true });
			for (const n of done) {
				const b64 = await evaluate(sid, `(() => { const a = Module.FS.readFile("/perf_${n}.png"); let s = "";
					for (let i = 0; i < a.length; i += 0x8000) s += String.fromCharCode.apply(null, a.subarray(i, i + 0x8000)); return btoa(s); })()`);
				fs.writeFileSync(path.join(PILOT, mode + "_" + n + ".png"), Buffer.from(b64, "base64"));
			}
		}
	}
	await send("Page.navigate", { url: "about:blank" }, sid);
	await send("Browser.close").catch(() => {});
	await sleep(1000);
	try { browser.kill(); } catch (e) {}
	server.close();
}
const STATE = `JSON.stringify({ status: (document.getElementById("status") || {}).textContent,
	cards: [...document.querySelectorAll("[id$=-overlay],[id$=-card]")].filter(e => !e.hidden && e.offsetParent).map(e => e.id + ": " +
		e.textContent.trim().replace(/\\s+/g, " ").slice(0, 160)), files: localStorage.getItem("byopop-game-files") ? "yes" : "no" })`;
main().then(() => process.exit(0), async e => { console.error(e); try { console.error(await evaluate(lastSid, STATE)); } catch (x) {} try { if (browser) browser.kill(); } catch (x) {} process.exit(1); });
