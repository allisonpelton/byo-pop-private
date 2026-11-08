// Plays one game of a mode in the player-only page with real DOM input (docs/WEB_PLAYER.md, "Drivers"): the game's own
// bot chooses each twist (the test command "move", docs/TEST_COMMANDS.md) and this sends it as the page's mouse events.
// The mode comes from the page's ?mode= (classic, zen or blitz; classic by default) and the time limit from ?seconds=
// (300). Zen never ends by itself, so it stops at the limit and leaves through the tube's menu ("Save and Quit").
// Opened as .../bejeweledtwist.html?player=1&assets=1&fast=1&driver=games/bejeweledtwist/tools/web/onegame.js&run=web
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "classic";
var limit = Number(params.get("seconds") || 300) * 1000;
var started = Date.now();
var made = 0, stops = 0, tips = 0, last = null;
var named = false;
// a command whose answer did not come in 10 s is asked again (twice at most)
async function retry(fn) {
  for (var i = 0; ; i++) {
    try { return await fn(); } catch (e) { if (i >= 2) throw e; lost++; }
  }
}
var lost = 0, refocused = 0;

// with ?hold=1 the test profile is wiped first, so the run starts as a new player (the name, then the tutorial)
// ?raf=timer: frames from a timer, for a hidden Browser pane, which gives a page no animation frames
if (params.get("raf") === "timer")
  window.requestAnimationFrame = function (cb) { return setTimeout(function () { cb(performance.now()); }, 16); };
if (params.get("hold")) {
  await t.profile.wipe();
  await t.start();
}

// the main menu, then the mode by the menu's own buttons (goto presses Play and then the mode's button)
for (;;) {
  last = await retry(function () { return t.state(); });
  if (last.screen === "menu") break;
  if (last.screen === "newplayer" && !named) {
    // a new player's name, typed, then Return (the first launch with no profile)
    await t.wait(60);
    await t.type("twist");
    await t.key("Enter");
    named = true;
  }
  if (Date.now() - started > 60000) return { error: "no main menu", state: last };
  await t.wait(50);
}
if (mode !== "classic") await t.command("unlock all");  // a new player has Classic only
await t.command("goto " + mode);
for (;;) {
  last = await retry(function () { return t.state(); });
  if (last.screen === "game" && last.mode === mode) break;
  if (Date.now() - started > 120000) return { error: "the game did not start", state: last };
  await t.wait(50);
}
await t.shot(mode + "-start");

while (Date.now() - started < limit) {
  last = await retry(function () { return t.state(); });
  if (last.screen !== "game" || last.over) break;
  if (!last.active) {
    // the pane lost the focus (the user working in the app around a hidden pane): the game pauses, as it should;
    // the page gets it back, counted
    refocused++;
    await t.focus();
    continue;
  }
  if (last.busy && !last.wheel && !last.rank && !(last.tip && last.tip.dialog)) {
    await t.wait(20);
    continue;
  }
  var lines = await retry(function () { return t.command("move"); });
  var m = JSON.parse(lines[lines.length - 1].substring(5));
  if (m === null) {
    await t.wait(20);
    continue;
  }
  for (var i = 0; i < m.input.length; i++) {
    var step = m.input[i];
    if (step.move) {
      // the board takes the square under the cursor in its update: hover first, as a player's mouse arrives
      await t.move(step.move[0] - 8, step.move[1] - 8);
      await t.wait(2);
      await t.move(step.move[0], step.move[1]);
      await t.wait(2);
    } else if (step.click) {
      await t.click(step.click[0], step.click[1]);
    }
  }
  if (m.kind === "stop") stops++;
  else if (m.kind !== "twist") tips++;  // the tutorial's, the tips' and the rank bar's clicks  // the tutorial's and the tips' clicks
  else made++;
  if (m.kind !== "stop" && made % 50 === 0) await t.shot(mode + "-" + made);
  await t.wait(10);
}
await t.shot(mode + "-end");

if (mode === "zen" && last.screen === "game" && !last.over) {
  // out with clicks: the tube's menu button, the options' Main Menu and Yes in "Save and Quit" (3D at 1228x768 art; the
  // page's ?quit= gives other points as x,y;x,y;x,y)
  var quit = (params.get("quit") || "220,672;490,628;487,460").split(";");
  for (var q = 0; q < quit.length; q++) {
    var p = quit[q].split(",").map(Number);
    await t.move(p[0] - 5, p[1] - 5);
    await t.wait(5);
    await t.click(p[0], p[1]);
    await t.wait(150);
    await t.shot("zen-quit" + q);
  }
  last = await retry(function () { return t.state(); });
}
return { mode: mode, twists: made, stops: stops, tips: tips, seconds: Math.round((Date.now() - started) / 1000), last: last,
  errors: t.errors.length, retried: lost, refocused: refocused };
