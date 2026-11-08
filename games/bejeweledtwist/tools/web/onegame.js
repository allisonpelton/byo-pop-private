// Plays one game of a mode in the player-only page with real DOM input (docs/WEB_PLAYER.md, "Drivers"): the game's own
// bot chooses each twist (the test command "move", docs/TEST_COMMANDS.md) and this sends it as the page's mouse events.
// The mode comes from the page's ?mode= (classic, zen or blitz; classic by default) and the time limit from ?seconds=
// (300). Zen never ends by itself, so it stops at the limit and leaves through the tube's menu ("Save and Quit").
// Opened as .../bejeweledtwist.html?player=1&assets=1&fast=1&driver=games/bejeweledtwist/tools/web/onegame.js&run=web
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "classic";
var limit = Number(params.get("seconds") || 300) * 1000;
var started = Date.now();
var made = 0, stops = 0, last = null;

// the main menu, then the mode by the menu's own buttons (goto presses Play and then the mode's button)
for (;;) {
  last = await t.state();
  if (last.screen === "menu") break;
  if (Date.now() - started > 60000) return { error: "no main menu", state: last };
  await t.wait(50);
}
await t.command("goto " + mode);
for (;;) {
  last = await t.state();
  if (last.screen === "game" && !last.busy) break;
  if (Date.now() - started > 120000) return { error: "the game did not start", state: last };
  await t.wait(50);
}
await t.shot(mode + "-start");

while (Date.now() - started < limit) {
  last = await t.state();
  if (last.screen !== "game" || last.over) break;
  if (last.busy) {
    await t.wait(20);
    continue;
  }
  var lines = await t.command("move");
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
  last = await t.state();
}
return { mode: mode, twists: made, stops: stops, seconds: Math.round((Date.now() - started) / 1000), last: last,
  errors: t.errors.length };
