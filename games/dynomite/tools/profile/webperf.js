// The web build's memory and frame time for Dynomite Deluxe, measured in the Browser pane at the game's own speed (no fast=1: the
// game runs on the page's timers, as a player's would). A driver of the shared page (docs/WEB_PLAYER.md, "Drivers"), the body of an
// async function of `t` (byopop.test); Typer Shark's tools/profile/webperf.js is the model. Serve build/web with
// `python tools/webtest/serve.py --port <port>` and open
//
//   http://127.0.0.1:<port>/dynomite.html?player=1&assets=1&silent=1&driver=games/dynomite/tools/profile/webperf.js
//
// The title is clicked through with real DOM events as a player would; then for each phase (the main menu standing, an Endless game
// played by the port's own bot through the test command "move", sent as real clicks) it reads, over a few seconds of real time, what
// the framework's frame hook passes (Module.byopopTestFrame: whether the frame drew, and the milliseconds EmscriptenMainLoop spent on
// its updates and draw) and, at the end of the phase, the framework's own memory figures (byopop_perf_json). The result is
// {phases: [...], errors}; each phase has frames, drawn frames, average and longest frame milliseconds and the share of the phase's
// real time the game ran. ?secs= sets a phase's seconds (8; the game gets two and a half times that). A page left hidden for minutes
// has its timers throttled: use a fresh tab for each run.
var params = new URLSearchParams(location.search);
var phaseSeconds = Number(params.get("secs") || 8);
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
  var extra = await work();
  var real = performance.now() - t0, u1 = await t.updates();
  var p = perfJson();
  var js = (performance.memory && performance.memory.usedJSHeapSize) || null;
  phases.push({
    phase: name, realSeconds: Math.round(real) / 1000, updates: u1 - u0, frames: acc.frames, drawn: acc.drawn,
    avgFrameMs: acc.frames ? Math.round(acc.ms / acc.frames * 100) / 100 : null, maxFrameMs: Math.round(acc.max * 100) / 100,
    gameShareOfRealTime: Math.round(acc.ms / real * 1000) / 10 + " %", jsHeapMB: js && Math.round(js / 1e4) / 100, perf: p,
    extra: extra || null
  });
}
async function clickAt(x, y) {
  await t.move(x - 5, y - 5);
  await t.wait(2);
  await t.click(x, y);
  await t.wait(30);
}
var err = null;
try {
  await t.ready;
  await until("the title", function (s) { return s.screen === "title"; }, 60000);
  await t.wait(200);
  for (var i = 0; i < 20 && (await t.state()).screen === "title"; i++) {
    await clickAt(320, 450);
    await t.wait(100);
  }
  await until("the main menu", function (s) { return s.screen === "menu"; }, 60000);
  await measure("main menu", function () { return sleep(phaseSeconds * 1000); });
  // New Game, Endless, Let's Play (the native step files' clicks)
  await clickAt(486, 213);
  await clickAt(465, 160);
  await clickAt(488, 432);
  await until("the game", function (s) { return s.screen === "game"; }, 60000);
  await measure("Endless, played by the bot", async function () {
    var end = Date.now() + phaseSeconds * 2500, moves = 0;
    while (Date.now() < end) {
      var st = await t.state();
      if (st.screen !== "game" || st.over) break;
      var play = await t.playByMove({ steps: 1 });
      if (play.moves) moves += play.moves; else await t.wait(10);
    }
    return { moves: moves };
  });
} catch (e) {
  err = e.message;
}
return { error: err, phases: phases, errors: t.errors.length, userAgent: navigator.userAgent.replace(/.*(Chrome\/[0-9.]+).*/, "$1") };
