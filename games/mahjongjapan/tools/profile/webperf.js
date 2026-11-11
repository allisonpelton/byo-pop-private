// The web build's memory and frame time for Mahjong Escape (Ancient Japan and Ancient China, the same script), measured
// in the Browser pane at the game's own speed (no fast=1: the game runs on the page's timers, as a player's would).
// A driver of the shared page (docs/WEB_PLAYER.md, "Drivers"), the body of an async function of `t` (byopop.test):
//
//   http://127.0.0.1:<port>/mahjongjapan.html?player=1&assets=1&silent=1&driver=games/mahjongjapan/tools/profile/webperf.js
//
// For each phase (the menu, the adventure's map, the first board, ten pairs of the first board) it reads, over a few
// seconds of real time, what the framework's frame hook passes (Module.byopopTestFrame: whether the frame drew, and the
// milliseconds EmscriptenMainLoop spent on its updates and draw) and, at the end of the phase, the framework's own
// memory figures (byopop_perf_json: the wasm heap, the bytes malloc has handed out, the images and sounds it holds).
// The result is {phases: [...], errors}; each phase has frames, drawn frames, average and longest frame milliseconds
// and the share of the phase's real time the game ran (the sum of the frame milliseconds over the phase).
var params = new URLSearchParams(location.search);
var phaseSeconds = Number(params.get("secs") || 8);
function sleep(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
function btns(s) { var m = {}; s.buttons.forEach(function (b) { m[b.cmd] = [b.x, b.y]; }); return m; }
async function until(what, test, timeoutMs) {
  var end = Date.now() + (timeoutMs || 60000), s = null;
  while (Date.now() < end) {
    s = await t.state();
    if (test(s)) return s;
    await t.wait(2);
  }
  throw new Error(what + ": not reached; last state " + JSON.stringify(s));
}
async function clickAt(x, y) { await t.move(x, y); await t.wait(5); await t.click(x, y); }
async function press(s, cmd) {
  var b = btns(s)[cmd];
  if (!b) throw new Error("no button 0x" + cmd.toString(16) + " on " + s.screen + "/" + s.dialog);
  await clickAt(b[0], b[1]);
}
// the board's tip opens a moment after the board is first ready: the port's bot answers a dialog with the click that closes it;
// done when no dialog has been there for two seconds of real time
async function clearDialogs() {
  var quiet = Date.now();
  for (var i = 0; i < 20000 && Date.now() - quiet < 2000; i++) {
    var st = await t.state();
    if (st.dialog === null) { await t.wait(2); continue; }
    quiet = Date.now();
    if (!st.busy) {
      var mv = await t.botMove();
      if (mv) await t.playMove(mv);
    }
    await t.wait(3);
  }
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
  var t0 = performance.now();
  await work();
  var real = performance.now() - t0;
  var p = perfJson();
  var js = (performance.memory && performance.memory.usedJSHeapSize) || null;
  phases.push({
    phase: name, realSeconds: Math.round(real) / 1000, frames: acc.frames, drawn: acc.drawn,
    avgFrameMs: acc.frames ? Math.round(acc.ms / acc.frames * 100) / 100 : null, maxFrameMs: Math.round(acc.max * 100) / 100,
    gameShareOfRealTime: Math.round(acc.ms / real * 1000) / 10 + " %", jsHeapMB: js && Math.round(js / 1e4) / 100, perf: p
  });
}
var err = null;
try {
  var s = await until("the menu", function (s) { return s.screen === "menu" && s.dialog === null && !s.busy; }, 120000);
  await measure("menu", function () { return sleep(phaseSeconds * 1000); });
  await press(s, 0x8e);
  s = await until("the name box or the map", function (s) { return (s.dialog === "newplayer" || s.screen === "map") && !s.busy; });
  if (s.dialog === "newplayer") {
    await t.type("Bot");
    await t.wait(5);
    await t.key("Enter");
  }
  s = await until("the map", function (s) { return s.screen === "map" && s.dialog === null && btns(s)[0xe6]; });
  await measure("map", function () { return sleep(phaseSeconds * 1000); });
  await press(s, 0xe6);
  s = await until("level 1", function (s) { return s.screen === "escape" && (s.dialog === null || s.dialog === "tip") && !s.busy; });
  await clearDialogs();
  await measure("board, idle", function () { return sleep(phaseSeconds * 1000); });
  await measure("board, ten pairs", async function () {
    var pairs = 0;
    for (var i = 0; i < 4000 && pairs < 10; i++) {
      var st = await t.state();
      if (st.screen !== "escape" || st.dialog !== null || st.busy) { await t.wait(2); continue; }
      var mv = await t.botMove();
      if (!mv) { await t.wait(3); continue; }
      await t.playMove(mv);
      if (mv.kind === "pair" || mv.kind === "special") pairs++;
      await t.wait(1);
    }
  });
} catch (e) {
  err = e.message;
}
return { error: err, phases: phases, errors: t.errors.length, userAgent: navigator.userAgent.replace(/.*(Chrome\/[0-9.]+).*/, "$1") };
