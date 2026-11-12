// The web build's memory and frame time for Rocket Mania Deluxe, measured in the Browser pane at the game's own speed (no fast=1: the
// game runs on the page's timers, as a player's would). A driver of the shared page (docs/WEB_PLAYER.md, "Drivers"), the body of an
// async function of `t` (byopop.test). Serve build/web with `python tools/webtest/serve.py --port <port>` and open
//
//   http://127.0.0.1:<port>/rocketmania.html?player=1&assets=1&silent=1&driver=games/rocketmania/tools/profile/webperf.js
//
// For each phase (the title or menu, an arcade game played by the port's own bot, a classic game, a strategy game) it reads, over a few
// seconds of real time, what the framework's frame hook passes (Module.byopopTestFrame: whether the frame drew, and the milliseconds
// EmscriptenMainLoop spent on its updates and draw) and, at the end of the phase, the framework's own memory figures (byopop_perf_json:
// the wasm heap, the bytes malloc has handed out, the images and sounds it holds). The result is {phases: [...], errors}; each phase
// has frames, drawn frames, average and longest frame milliseconds and the share of the phase's real time the game ran (the sum of the
// frame milliseconds over the phase). A page left hidden for minutes has its timers throttled: use a fresh tab for each run. A hidden
// pane gets no animation frames: install the shim of docs/WEB_PLAYER.md's tests first. games/typershark/tools/profile/webperf.js is
// the model; the bot plays through the page's real input (t.botMove, t.playMove).
var params = new URLSearchParams(location.search);
var phaseSeconds = Number(params.get("secs") || 8);
var difficulty = params.get("difficulty") || "2";
function sleep(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
async function until(what, test, timeoutMs) {
  var end = Date.now() + (timeoutMs || 60000), s = null;
  while (Date.now() < end) {
    s = await t.state();
    if (test(s)) return s;
    await t.wait(2);
  }
  throw new Error(what + ": not reached; last state " + JSON.stringify(s));
}
var acc = { frames: 0, drawn: 0, ms: 0, max: 0 };
var prevHook = Module.byopopTestFrame;
Module.byopopTestFrame = function (drew, ms) {
  if (prevHook) prevHook(drew, ms);
  acc.frames++;
  if (drew) acc.drawn++;
  acc.ms += ms;
  if (ms > acc.max) acc.max = ms;
};
function perfJson() {
  try { return JSON.parse(Module.ccall("byopop_perf_json", "string", [], [])); } catch (e) { return { error: String(e) }; }
}
var phases = [];
async function measure(name, work) {
  acc = { frames: 0, drawn: 0, ms: 0, max: 0 };
  var t0 = performance.now(), u0 = await t.updates();
  await work();
  var real = performance.now() - t0, u1 = await t.updates();
  var p = perfJson();
  var js = (performance.memory && performance.memory.usedJSHeapSize) || null;
  phases.push({
    phase: name, realSeconds: Math.round(real) / 1000, updates: u1 - u0, frames: acc.frames, drawn: acc.drawn,
    avgFrameMs: acc.frames ? Math.round(acc.ms / acc.frames * 100) / 100 : null, maxFrameMs: Math.round(acc.max * 100) / 100,
    gameShareOfRealTime: Math.round(acc.ms / real * 1000) / 10 + " %", jsHeapMB: js && Math.round(js / 1e4) / 100, perf: p
  });
}
// Starts a game: the loading screen's play link is the bot's first move, so a refused start is retried after one (pagecheck.js does the same).
async function startGame(mode) {
  for (var tries = 0; ; tries++) {
    var lines = await t.command("goto " + mode + " " + difficulty);
    var last = lines[lines.length - 1] || "";
    if (!/^ERROR /.test(last)) return;
    if (tries >= 20) throw new Error("goto " + mode + ": " + last);
    for (var e = t.errors.length - 1; e >= 0; e--) if (t.errors[e].text.indexOf(last) >= 0) t.errors.splice(e, 1);
    var m = await t.botMove();
    if (m) await t.playMove(m);
    await t.wait(30);
  }
}
async function play(seconds) {
  var end = Date.now() + seconds * 1000;
  while (Date.now() < end) {
    var st = await t.state();
    if (st.screen !== "board" || st.over) break;
    var mv = await t.botMove();
    if (!mv) { await t.wait(2); continue; }
    await t.playMove(mv);
  }
}
var err = null;
try {
  await t.ready;
  await measure("loading screen", function () { return sleep(phaseSeconds * 1000); });
  var modes = ["arcade", "classic", "strategy"];
  for (var i = 0; i < modes.length; i++) {
    await startGame(modes[i]);
    await until("the " + modes[i] + " board", function (s) { return s.screen === "board"; }, 60000);
    await measure(modes[i] + ", played by the bot", function () { return play(phaseSeconds); });
  }
} catch (e) {
  err = e.message;
}
return { error: err, phases: phases, errors: t.errors.length, userAgent: navigator.userAgent.replace(/.*(Chrome\/[0-9.]+).*/, "$1") };
