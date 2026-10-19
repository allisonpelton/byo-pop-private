#!/usr/bin/env node
// port(variant): the Node runner for Bejeweled's WebAssembly builds (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9,
// the stage 4 row of 9.2). Not part of the original game.
//
//   node wasmrun.js verify <bejeweled_verify.wasm> <replay.bjr> [-trace] [-forcevariant]
//       the standalone verifier (no JS glue: this file stubs its WASI imports) plays the replay: its "REPLAY ..."
//       lines (with -trace also the TRACE settles) and then the result JSON on stdout, exactly as the native
//       bejeweled_verify.exe prints them for a file named replay.bjr. Exit status 0 when the replay is ok, 1
//       when not.
//   node wasmrun.js difftest <bejeweled_verify.wasm> --seeds <a>..<b> [--ticks <n>] [--mode normal|timed|both] [-v]
//       the verifier's difftest in wasm: its report as the native exe prints it (-v: a line per game, with the
//       game's last LOGIC hash); exit status 1 when a game differs
//   node wasmrun.js rulesid <bejeweled_verify.wasm> <mod.json>
//       the RulesId JSON of a mod's rules
//   node wasmrun.js game <bejeweled.js> <game switches...>
//       the game's web build (build/web/bejeweled.js, the one the web shell runs) with the switches a native
//       run takes, which the web shell never passes: the test hooks -seed, -script, -fast, -trace,
//       -tracehash, -autoplay..., -codeccheck, -replay, -gameseed, -forcevariant, -gamedir, -userdir, -modsdir.
//       Run it in the directory a native run would use (the oracle's case directory): that directory is copied
//       into the game's in-memory file system and becomes its working directory, a switch naming an absolute
//       host path (the game files, a mods folder) gets a read-only copy, and when the game quits every file it
//       wrote or deleted in the working directory is written or deleted on the host. Its stdout and stderr are
//       the game's (LF line ends; SDL_Log lines on stderr as natively). As natively, finished replays go to
//       <user dir>/replays unless -replaydir says otherwise (the web shell's /persist/replays is no folder
//       here). The page is replaced by stubs: no canvas is drawn (SDL falls back to its software renderer
//       on a 2D context that keeps nothing), no audio device (the game plays silently, as without one), no URL
//       is opened (window.open does nothing), the browser's frames come back to back (-fast then runs as fast
//       as natively). Exit status: the game's 0, 3
//       when it crashed, 4 when it did not quit within --timeout seconds (default 600).
//
// Only Node's own modules are used. Node: E:/retools/emsdk/node/<version>/node.exe (emsdk's).

"use strict";

const fs = require("fs");
const path = require("path");

function usage() {
	process.stderr.write(
		"usage: node wasmrun.js verify <bejeweled_verify.wasm> <replay.bjr> [-trace] [-forcevariant]\n" +
		"       node wasmrun.js difftest <bejeweled_verify.wasm> --seeds <a>..<b> [--ticks <n>] [--mode normal|timed|both] [-v]\n" +
		"       node wasmrun.js rulesid <bejeweled_verify.wasm> <mod.json>\n" +
		"       node wasmrun.js game [--timeout <seconds>] <bejeweled.js> <game switches...>\n");
	process.exit(2);
}

// ---- The standalone verifier ----

// Instantiates bejeweled_verify.wasm with every import it declares stubbed: fd_write to this process's
// stdout and stderr, the rest of WASI as calls that fail politely (the verifier opens no file)
function instantiateVerifier(theModule) {
	let aMemory = null;
	const ERRNO_BADF = 8;
	const ERRNO_NOSYS = 52;
	const ERRNO_SPIPE = 70;
	const view = () => new DataView(aMemory.buffer);
	const stubs = {
		fd_write(theFd, theIovs, theCount, theWritten) {
			let aTotal = 0;
			for (let i = 0; i < theCount; i++) {
				const aPtr = view().getUint32(theIovs + i * 8, true);
				const aLen = view().getUint32(theIovs + i * 8 + 4, true);
				if (aLen > 0 && (theFd === 1 || theFd === 2))
					fs.writeSync(theFd, Buffer.from(new Uint8Array(aMemory.buffer, aPtr, aLen)));
				aTotal += aLen;
			}
			view().setUint32(theWritten, aTotal, true);
			return (theFd === 1 || theFd === 2) ? 0 : ERRNO_BADF;
		},
		fd_read: () => ERRNO_BADF,
		fd_close: () => 0,
		fd_seek: () => ERRNO_SPIPE,
		proc_exit(theCode) { throw new Error("bejeweled_verify.wasm called proc_exit(" + theCode + ")"); },
		emscripten_notify_memory_growth: () => {},
	};
	const anImports = {};
	for (const anImport of WebAssembly.Module.imports(theModule)) {
		if (anImport.kind !== "function")
			throw new Error("bejeweled_verify.wasm imports a " + anImport.kind + ": " + anImport.module + "." + anImport.name);
		anImports[anImport.module] = anImports[anImport.module] || {};
		anImports[anImport.module][anImport.name] = stubs[anImport.name] || (() => ERRNO_NOSYS);
	}
	const anInstance = new WebAssembly.Instance(theModule, anImports);
	aMemory = anInstance.exports.memory;
	if (anInstance.exports._initialize)
		anInstance.exports._initialize();
	const ex = anInstance.exports;

	// Bytes into the module's memory, a C string out of it
	const withBytes = (theBytes, theCall) => {
		const aPtr = ex.bv_alloc(theBytes.length);
		new Uint8Array(aMemory.buffer, aPtr, theBytes.length).set(theBytes);
		try {
			return theCall(aPtr, theBytes.length);
		} finally {
			ex.bv_free(aPtr);
		}
	};
	const cString = (thePtr) => {
		const aHeap = new Uint8Array(aMemory.buffer);
		let anEnd = thePtr;
		while (aHeap[anEnd] !== 0)
			anEnd++;
		return Buffer.from(aHeap.subarray(thePtr, anEnd)).toString("utf8");
	};
	return {ex, memory: aMemory, withBytes, cString};
}

// The verifier's calls, the host contract a long-lived host (the leaderboard's Worker) keeps too: an instance
// that trapped (a WebAssembly.RuntimeError: a bug, an input the checks missed) is never called again, because
// a trap leaves it as it was at that moment (its stack pointer not restored, what it had allocated still
// taken), and neither is one whose memory grew past MAX_KEPT_MEMORY (wasm memory never shrinks: a 16 MB replay
// of 4M events takes 200 MB; the leaderboard's 256 KB ones a few): the next call gets a fresh instance of the
// module. Each call here makes at most one.
const MAX_KEPT_MEMORY = 64 * 1024 * 1024;
function loadVerifier(theWasmPath) {
	const aModule = new WebAssembly.Module(fs.readFileSync(theWasmPath));
	let anInstance = null;
	const call = (theWork) => {
		if (anInstance === null)
			anInstance = instantiateVerifier(aModule);
		try {
			const aResult = theWork(anInstance);
			if (anInstance.memory.buffer.byteLength > MAX_KEPT_MEMORY)
				anInstance = null;
			return aResult;
		} catch (e) {
			anInstance = null;
			throw e;
		}
	};
	return {
		verify(theBytes, theTrace, theForceVariant) {
			return call(({ex, withBytes, cString}) => {
				ex.bv_set_options((theTrace ? 1 : 0) | (theForceVariant ? 2 : 0));
				return cString(withBytes(theBytes, (p, n) => ex.bv_verify(p, n)));
			});
		},
		rulesId(theText) {
			return call(({ex, withBytes, cString}) => cString(withBytes(Buffer.from(theText, "utf8"), (p, n) => ex.bv_rules_id(p, n))));
		},
		difftest(theFirst, theLast, theTicks, theModes) {
			return call(({ex, cString}) => cString(ex.bv_difftest(theFirst >>> 0, theLast >>> 0, theTicks | 0, theModes | 0)));
		},
	};
}

function verifyCommand(theArgs) {
	if (theArgs.length < 2)
		usage();
	const aSwitches = theArgs.slice(2);
	if (aSwitches.some((a) => a !== "-trace" && a !== "-forcevariant"))
		usage();
	const aVerifier = loadVerifier(theArgs[0]);
	const aJson = aVerifier.verify(fs.readFileSync(theArgs[1]), aSwitches.includes("-trace"), aSwitches.includes("-forcevariant"));
	process.stdout.write(aJson + "\n");
	return JSON.parse(aJson).result === "ok" ? 0 : 1;
}

function difftestCommand(theArgs) {
	if (theArgs.length < 1)
		usage();
	let aFirst = 0, aLast = 0, aTicks = 0, aModes = 3, aVerbose = 0;
	for (let i = 1; i < theArgs.length; i++) {
		if (theArgs[i] === "--seeds" && i + 1 < theArgs.length) {
			const m = /^(\d+)(?:\.\.(\d+))?$/.exec(theArgs[++i]);
			if (!m)
				usage();
			aFirst = Number(m[1]);
			aLast = (m[2] !== undefined) ? Number(m[2]) : aFirst;
		} else if (theArgs[i] === "--ticks" && i + 1 < theArgs.length)
			aTicks = Number(theArgs[++i]);
		else if (theArgs[i] === "--mode" && i + 1 < theArgs.length)
			aModes = {normal: 1, timed: 2, both: 3}[theArgs[++i]] || usage();
		else if (theArgs[i] === "-v")
			aVerbose = 4;
		else
			usage();
	}
	if (aFirst > 0xFFFFFFFF || aLast > 0xFFFFFFFF)
		usage();
	// The export checks the range, the ticks and the modes itself (Verifier::Difftest): "difftest: <why>"
	const aReport = loadVerifier(theArgs[0]).difftest(aFirst, aLast, aTicks, aModes | aVerbose);
	if (aReport.startsWith("difftest: ")) {
		process.stderr.write(aReport);
		usage();
	}
	process.stdout.write(aReport);
	return /: \d+ games, 0 differ;/.test(aReport) && !/, [1-9]\d* differ;/.test(aReport) ? 0 : 1;
}

function rulesIdCommand(theArgs) {
	if (theArgs.length !== 2)
		usage();
	process.stdout.write(loadVerifier(theArgs[0]).rulesId(fs.readFileSync(theArgs[1], "utf8")) + "\n");
	return 0;
}

// ---- The game's web build ----

// What the web build reads of the page: a canvas whose 2D context keeps nothing, a window, a document and a
// screen that take event listeners, no WebGL (SDL then renders in software) and no AudioContext
function installPageStubs() {
	const noop = () => {};
	const makeElement = (theName) => ({
		id: theName, style: {}, width: 640, height: 480, clientWidth: 640, clientHeight: 480, parentNode: null,
		addEventListener: noop, removeEventListener: noop, dispatchEvent: noop, focus: noop, blur: noop,
		setAttribute: noop, getAttribute: () => null, requestPointerLock: noop,
		getBoundingClientRect: () => ({left: 0, top: 0, right: 640, bottom: 480, width: 640, height: 480, x: 0, y: 0}),
		getContext: (theKind) => (theKind !== "2d") ? null : {
			createImageData: (w, h) => ({width: w, height: h, data: new Uint8ClampedArray(w * h * 4)}),
			getImageData: (x, y, w, h) => ({width: w, height: h, data: new Uint8ClampedArray(w * h * 4)}),
			putImageData: noop, drawImage: noop, fillRect: noop, clearRect: noop, save: noop, restore: noop,
			scale: noop, setTransform: noop,
		},
	});
	const aCanvas = makeElement("canvas");
	const aDocument = {
		addEventListener: noop, removeEventListener: noop, hidden: false, visibilityState: "visible", title: "",
		fullscreenElement: null, body: makeElement("body"), documentElement: makeElement("html"),
		createElement: (n) => makeElement(n),
		querySelector: (s) => (s === "#canvas" || s === "canvas") ? aCanvas : null,
		getElementById: (s) => (s === "canvas") ? aCanvas : null,
	};
	globalThis.document = aDocument;
	// window.open is SDL_OpenURL on the web: here no URL ever leaves the process
	globalThis.window = {addEventListener: noop, removeEventListener: noop, devicePixelRatio: 1, innerWidth: 640, innerHeight: 480,
		document: aDocument, open: () => null};
	globalThis.screen = {width: 1920, height: 1080, availWidth: 1920, availHeight: 1080};
	globalThis.WebGLRenderingContext = class {};
	globalThis.WebGL2RenderingContext = class {};
	// The browser's next frame at once: the game's -fast loop runs back to back
	globalThis.requestAnimationFrame = (theCallback) => setImmediate(() => theCallback(performance.now()));
	return aCanvas;
}

const MAX_COPY_FILES = 5000;
const MAX_COPY_BYTES = 256 * 1024 * 1024;

// Host folder -> in-memory folder; returns {relative path: bytes} of what it copied
function copyIn(FS, theHostDir, theMemDir, theBudget) {
	const aFiles = {};
	const walk = (theHost, theMem, theRel) => {
		FS.mkdirTree(theMem);
		for (const aName of fs.readdirSync(theHost)) {
			const aHost = path.join(theHost, aName);
			const aMem = theMem + "/" + aName;
			const aRel = theRel ? theRel + "/" + aName : aName;
			const aStat = fs.statSync(aHost);
			if (aStat.isDirectory()) {
				walk(aHost, aMem, aRel);
				continue;
			}
			const aBytes = fs.readFileSync(aHost);
			theBudget.files++;
			theBudget.bytes += aBytes.length;
			if (theBudget.files > MAX_COPY_FILES || theBudget.bytes > MAX_COPY_BYTES)
				throw new Error("wasmrun: " + theHostDir + " is too large to copy in (run it in a test run's own directory)");
			FS.writeFile(aMem, aBytes);
			FS.utime(aMem, aStat.mtimeMs, aStat.mtimeMs);
			aFiles[aRel] = aBytes;
		}
	};
	walk(theHostDir, theMemDir, "");
	return aFiles;
}

// In-memory folder -> host folder: the files that are new or changed, the deletion of those that went
function copyOut(FS, theMemDir, theHostDir, theBefore) {
	const aSeen = new Set();
	const walk = (theMem, theRel) => {
		for (const aName of FS.readdir(theMem)) {
			if (aName === "." || aName === "..")
				continue;
			const aMem = theMem + "/" + aName;
			const aRel = theRel ? theRel + "/" + aName : aName;
			const aHost = path.join(theHostDir, ...aRel.split("/"));
			if (FS.isDir(FS.stat(aMem).mode)) {
				fs.mkdirSync(aHost, {recursive: true});
				walk(aMem, aRel);
				continue;
			}
			aSeen.add(aRel);
			const aBytes = FS.readFile(aMem);
			const anOld = theBefore[aRel];
			if (!anOld || Buffer.compare(Buffer.from(aBytes), anOld) !== 0)
				fs.writeFileSync(aHost, aBytes);
		}
	};
	walk(theMemDir, "");
	for (const aRel of Object.keys(theBefore)) {
		if (!aSeen.has(aRel))
			fs.rmSync(path.join(theHostDir, ...aRel.split("/")), {force: true});
	}
}

function isHostAbsolute(theValue) {
	return /^[A-Za-z]:[\\/]/.test(theValue) || theValue.startsWith("/") || theValue.startsWith("\\\\");
}

function gameCommand(theArgs) {
	let aTimeout = 600;
	while (theArgs.length > 0 && theArgs[0].startsWith("--")) {
		if (theArgs[0] === "--timeout" && theArgs.length > 1) {
			aTimeout = Number(theArgs[1]);
			theArgs = theArgs.slice(2);
		} else
			usage();
	}
	if (theArgs.length < 1)
		usage();
	const aJsPath = path.resolve(theArgs[0]);
	const aGameArgs = theArgs.slice(1);
	const aWorkHost = process.cwd();

	const aCanvas = installPageStubs();
	const Module = require(aJsPath);
	Module.canvas = aCanvas;
	Module.silentAudio = true;

	let isDone = false;
	let FS = null;
	let aBefore = {};
	const WORK = "/work";
	// The files the game wrote, back on the host (also after a crash: what was written by then)
	const finish = (theCode, theWhy) => {
		if (isDone)
			return;
		isDone = true;
		try {
			if (typeof Module._fflush === "function")
				Module._fflush(0);
		} catch (e) {
			// a crashed module may not take the call
		}
		if (FS !== null)
			copyOut(FS, WORK, aWorkHost, aBefore);
		if (theWhy)
			process.stderr.write("wasmrun: " + theWhy + "\n");
		process.exit(theCode);
	};
	process.on("uncaughtException", (e) => finish(3, "the game crashed: " + (e && e.stack ? e.stack : e)));
	setTimeout(() => finish(4, "the game did not quit within " + aTimeout + " seconds"), aTimeout * 1000).unref();

	Module.onRuntimeInitialized = () => {
		FS = Module.FS;
		const aBudget = {files: 0, bytes: 0};
		aBefore = copyIn(FS, aWorkHost, WORK, aBudget);
		// A switch with an absolute host path: a read-only copy of it
		let aHostCopies = 0;
		const aCallArgs = aGameArgs.map((theArg) => {
			const m = /^(-[A-Za-z]+=)(.+)$/.exec(theArg);
			if (!m || !isHostAbsolute(m[2]) || !fs.existsSync(m[2]))
				return theArg;
			const aMem = "/host/" + (aHostCopies++);
			if (fs.statSync(m[2]).isDirectory())
				copyIn(FS, m[2], aMem, aBudget);
			else {
				FS.mkdirTree(aMem);
				FS.writeFile(aMem + "/" + path.basename(m[2]), fs.readFileSync(m[2]));
				return m[1] + aMem + "/" + path.basename(m[2]);
			}
			return m[1] + aMem;
		});
		// Finished replays where a native run puts them (<user dir>/replays), not in the web shell's /persist
		const aUserDir = aCallArgs.map((a) => /^-userdir=(.+)$/.exec(a)).find((m) => m);
		if (aUserDir && !aCallArgs.some((a) => a.startsWith("-replaydir=")))
			aCallArgs.push("-replaydir=" + aUserDir[1] + "/replays");
		FS.chdir(WORK);
		Module.onGameExit = () => finish(0, null);
		try {
			Module.callMain(aCallArgs);
		} catch (e) {
			if (e !== "unwind" && !(e && e.name === "ExitStatus"))
				throw e;
		}
	};
	return null; // the game runs on; finish() ends the process
}

function main(theArgv) {
	const aCommand = theArgv[0];
	const aRest = theArgv.slice(1);
	if (aCommand === "verify")
		return verifyCommand(aRest);
	if (aCommand === "difftest")
		return difftestCommand(aRest);
	if (aCommand === "rulesid")
		return rulesIdCommand(aRest);
	if (aCommand === "game")
		return gameCommand(aRest);
	usage();
}

const aStatus = main(process.argv.slice(2));
if (aStatus !== null && aStatus !== undefined)
	process.exitCode = aStatus;
