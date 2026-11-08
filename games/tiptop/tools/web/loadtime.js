// The web page's load time, in real time (GAME_NOTES.md, "Performance"): the body of an async function of byopop.test (t), for the
// player-only page without ?fast=1 (docs/WEB_PLAYER.md, "Drivers"):
//
//   http://127.0.0.1:18793/tiptop.html?player=1&assets=1&silent=1&driver=games/tiptop/tools/web/loadtime.js
//
// It clicks the title's button every update until the screen leaves the title (a click before the load bar is full is ignored),
// and returns the time since the page's navigation start at which the driver began and at which the title took the click, the
// animation frames drawn between, and the rate those frames came at, and the game's own update count when the title took the
// click (state's `updates`, counted from the game's start). The update count does not depend on the machine's speed, the
// milliseconds do (a hidden pane draws at a fraction of 60 Hz and runs its timers slowly).
var result = { driverStartMs: Math.round(performance.now()) };
var frames = 0, firstFrameMs = null, lastFrameMs = null, running = true;
function frame(ms) {
  if (firstFrameMs === null) firstFrameMs = ms;
  lastFrameMs = ms;
  frames++;
  if (running) requestAnimationFrame(frame);
}
requestAnimationFrame(frame);
var clicks = 0;
try {
  for (;;) {
    await t.click(186, 288);
    clicks++;
    var st = await t.state();
    if (st.screen !== "title") {
      result.screen = st.screen;
      result.game_updates_when_taken = st.updates;
      break;
    }
    if (performance.now() - result.driverStartMs > 120000) throw new Error("the title never took the click");
  }
} catch (e) {
  result.error = String((e && e.message) || e);
}
running = false;
result.title_taken_ms = Math.round(performance.now());
result.load_ms = result.title_taken_ms - result.driverStartMs;
result.frames = frames;
result.clicks = clicks;
result.ms_per_frame = frames > 1 ? Math.round(((lastFrameMs - firstFrameMs) / (frames - 1)) * 10) / 10 : null;
result.updates = t.updates();
result.errors = t.errors.length;
return result;
