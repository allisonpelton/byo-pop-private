#!/usr/bin/env node
// port(variant): the Node runner for Bookworm Adventures Volume 2's WebAssembly build. Not part of the original game.
// A trimmed copy of games/bejeweled/tools/verify/wasmrun.js (its `game` command only), with copy limits of its own: the
// game's folder (assets/bookwormadv2) is about 9,300 files and 87 MB, more than Bejeweled's limits allow.
//
//   node wasmrun.js [--timeout <seconds>] <bookwormadv2.js> <game switches...>
//       the game's web build (build/web/bookwormadv2.js, the one the web shell runs) with the switches a native
//       run takes, which the web shell never passes: the test hooks -seed, -script, -fast, -gamedir, -userdir.
//       Run it in an empty folder (or the folder a test run would use): that directory is copied into the game's
//       in-memory file system and becomes its working directory, a switch naming an absolute host path (the game
//       files) gets a read-only copy, and when the game quits every file it wrote or deleted in the working
//       directory is written or deleted on the host (so a -script run's screenshots appear there; give -userdir
//       a path inside it to get the profile back too: a relative one is made absolute in the game's file system,
//       which is the only kind the game reads a profile from). Its stdout and stderr are the game's (SDL_Log
//       lines on stderr as natively). The page is replaced by stubs: no canvas is drawn (SDL falls back to its software
//       renderer), no audio device (the game plays silently), no URL is opened, the browser's frames come back
//       to back (-fast then runs as fast as natively). Exit status: the game's 0, 3 when it crashed, 4 when it did
//       not quit within --timeout seconds (default 600).
//
// Needs no rundesk (no window, no sound, no URL), but counts as a game process for the three-at-a-time rule.
// The wasm heap only grows (ALLOW_MEMORY_GROWTH): a long soak (`soak <update>` in TestHooks.cpp) reaches about
// 800 MB; start node with --max-old-space-size only if the host's own allocations ever need it.
// Only Node's own modules are used. Node: E:/retools/emsdk/node/<version>/node.exe (emsdk's).

"use strict";

const fs = require("fs");
const path = require("path");

function usage() {
	process.stderr.write("usage: node wasmrun.js [--timeout <seconds>] <bookwormadv2.js> <game switches...>\n");
	process.exit(2);
}

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

const MAX_COPY_FILES = 20000;
const MAX_COPY_BYTES = 512 * 1024 * 1024;


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
			// The game reads a profile only from an absolute path (the page's is /bwa2persist/userdata): a relative
			// -userdir names the folder in the working directory, where its files come back when the game quits
			if (m && m[1] === "-userdir=" && !isHostAbsolute(m[2]))
				return m[1] + WORK + "/" + m[2];
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

const aStatus = gameCommand(process.argv.slice(2));
if (aStatus !== null && aStatus !== undefined)
	process.exitCode = aStatus;
