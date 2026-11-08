// A whole game of one mode on the web, by real DOM input (docs/WEB_PLAYER.md, "Drivers"): from the launch (the logos, the
// main menu) it sets the Options screen's SHOOTER MODE and DIFFICULTY by clicks, then the port's own bot plays from the main
// menu's Start until the game is over (the built-in ?driver=move loop, byopop.test.playByMove).
//
//   .../waterbugs.html?player=1&silent=1&assets=1&fast=1&driver=games/waterbugs/tools/web/modes.js&shooter=0&diff=-1
//
// shooter: 1 auto (the default), 0 manual; diff: 0 (the default), 1 or -1 (the button cycles 0, 1, -1; the test profile keeps the
// options between loads, so the clicks are counted from the difficulty it has); focus=1 loses the
// focus (byopop.test.blur) for 300 updates and regains it on board 4, which the `level` test command goes to first;
// fullscreen=1 sends Alt+Enter twice at the main menu.
// Returns { shooter, diff, moves, stop, score, lives, board, over, settings, errors }.
var q = new URLSearchParams(location.search);
var shooter = q.has("shooter") ? Number(q.get("shooter")) : 1;
var diff = q.has("diff") ? Number(q.get("diff")) : 0;

async function untilScreen(name) {
  for (var i = 0; i < 400; i++) {
    var s = await t.state();
    if (s.screen === name) return s;
    await t.wait(30);
  }
  throw new Error("no " + name + " screen");
}
async function press(x, y) {
  await t.click(x, y);
  await t.wait(10);
}

await t.ready;
await untilScreen("menu");
await press(400, 397);                     // Options
var s = await untilScreen("options");
if (shooter !== s.shootermode) await press(400, 317);
// the button cycles 0, 1, -1: the clicks from where the page's saved options have it to the difficulty wanted
var order = [0, 1, -1];
var clicks = (order.indexOf(diff) - order.indexOf(s.difficulty) + 3) % 3;
for (var i = 0; i < clicks; i++) await press(400, 377);
s = await t.state();
var settings = { shooter: s.shootermode, difficulty: s.difficulty };
await press(400, 537);                     // back to the main menu
await untilScreen("menu");

if (q.get("fullscreen") === "1") {
  // Alt+Enter, twice (the page may refuse the full screen, as a hidden Browser pane does; the game goes on in the window)
  for (var k = 0; k < 2; k++) {
    await t.keyDown("Alt");
    await t.key("Enter");
    await t.keyUp("Alt");
    await t.wait(60);
  }
  settings.fullscreenKeys = true;
}

if (q.get("focus") === "1") {
  // the focus lost and regained in the middle of a board: the game stops meanwhile and goes on
  await t.command("level 4");
  await t.wait(200);
  var before = await t.state();
  await t.blur();
  await t.wait(300);
  var during = await t.state();
  await t.focus();
  await t.wait(100);
  var after = await t.state();
  settings.focus = { frameBefore: before.frame, frameWhileLost: during.frame, frameAfter: after.frame };
}

var r = await t.playByMove({ steps: Number(q.get("steps")) || 3000 });
var f = r.state || {};
return { shooter: shooter, diff: diff, moves: r.moves, stop: r.stop, score: f.score, lives: f.lives, board: f.reached + 1,
         over: f.over, settings: settings, errors: r.errors };
