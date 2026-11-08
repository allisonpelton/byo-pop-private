// Plays one game of a mode in the player-only page with real DOM input (docs/WEB_PLAYER.md, "Drivers"): the game's own
// bot chooses each twist (the test command "move", docs/TEST_COMMANDS.md) and this sends it as the page's mouse events.
// The mode comes from the page's ?mode= (classic, zen or blitz; classic by default) and the time limit from ?seconds=
// (300). Zen never ends by itself, so it stops at the limit and leaves through the tube's menu ("Save and Quit").
// Opened as .../bejeweledtwist.html?player=1&assets=1&fast=1&driver=games/bejeweledtwist/tools/web/onegame.js&run=web
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "classic";
var limit = Number(params.get("seconds") || 300) * 1000;
// ?twists=N: after N twists the test command "lose" ends the game through its own ending (Classic seldom ends by itself
// while the bot defuses its bombs), and the driver clicks on through the end screens; ?jump=1 sends "level 10" after
// half of them, which brings Classic's zen jump question
var twistLimit = Number(params.get("twists") || 0);
var jumped = false, ended = false;
var started = Date.now();
var made = 0, stops = 0, tips = 0, last = null;
var named = false;
// A command and its answer (STATE, MOVE, OK or ERROR), read from the page's log between its tasks: a command with no
// answer in 600 updates is sent again (twice at most), counted in "retried". The kit's own wait for the answer runs on a
// timer, which a hidden pane fires about once a minute, and in a hidden pane it sometimes missed an answer already in
// the log, which held the driver for minutes
var ANSWER = /^(INFO: )?([A-Z]{3,} (\{|\[|null\b)|OK |ERROR )/;
async function ask(line) {
  for (var attempt = 0; attempt < 3; attempt++) {
    var since = t.logNext();
    t.command(line, { timeout: 0 }).catch(function () {});
    var start = t.updates();
    while (t.updates() - start < 600) {
      var lines = t.log(since);
      for (var k = 0; k < lines.length; k++) {
        if (ANSWER.test(lines[k].text))
          return lines.slice(0, k + 1).map(function (l) { return l.text.replace(/^INFO: /, ""); });
      }
      await tick();
    }
    lost++;
  }
  throw new Error("no answer to " + line);
}
async function state() {
  var lines = await ask("state");
  var last = lines[lines.length - 1];
  if (!/^STATE /.test(last)) throw new Error("state: " + last);
  return JSON.parse(last.substring(6));
}
var lost = 0, refocused = 0;
// n more updates, counted between the page's tasks (a message loop): byopop.test.wait resolves on drawn frames, and a
// game that draws nothing new (a paused or waiting screen) left a driver waiting there for minutes
var tickChannel = new MessageChannel(), tickResolve = null;
tickChannel.port1.onmessage = function () { var r = tickResolve; tickResolve = null; if (r) r(); };
function tick() { return new Promise(function (r) { tickResolve = r; tickChannel.port2.postMessage(0); }); }
async function waitUpdates(n) {
  var target = t.updates() + n;
  while (t.updates() < target) await tick();
}

// with ?hold=1 the test profile is wiped first, so the run starts as a new player (the name, then the tutorial)
if (params.get("hold")) {
  await t.profile.wipe();
  await t.start();
}

// the main menu, then the mode by the menu's own buttons (goto presses Play and then the mode's button)
for (;;) {
  last = await state();
  if (last.screen === "menu") break;
  if (last.screen === "newplayer" && !named) {
    // a new player's name, typed, then Return (the first launch with no profile)
    await waitUpdates(60);
    await t.type("twist");
    await t.key("Enter");
    named = true;
  }
  if (Date.now() - started > 60000) return { error: "no main menu", state: last };
  await waitUpdates(50);
}

// ?scale=2 draws the canvas at twice the game's size (CSS), so that the page's mouse events land at scaled points as they
// do in full screen, which the Browser pane cannot enter (its requestFullscreen() is refused)
var scale = Number(params.get("scale") || 1), scaled = null;
if (scale !== 1) {
  var cv = document.getElementById("canvas"), size = t.screen();
  cv.style.width = Math.round(size.w * scale) + "px";
  cv.style.height = Math.round(size.h * scale) + "px";
  await waitUpdates(20);
  var box = cv.getBoundingClientRect();
  scaled = Math.round(box.width) + "x" + Math.round(box.height) + " for " + size.w + "x" + size.h;
}
if (mode !== "classic") await ask("unlock all");  // a new player has Classic only
await ask("goto " + mode);
for (;;) {
  last = await state();
  if (last.screen === "game" && last.mode === mode) break;
  if (Date.now() - started > 120000) return { error: "the game did not start", state: last };
  await waitUpdates(50);
}
await t.shot(mode + "-start");

while (Date.now() - started < limit) {
  last = await state();
  // a game ends through its own screens (the rank bar, the stats, the high scores), clicked through to the main menu
  if (last.screen === "menu") break;
  if (!last.active) {
    // the pane lost the focus (the user working in the app around a hidden pane): the game pauses, as it should;
    // the page gets it back, counted
    refocused++;
    await t.focus();
    continue;
  }
  if (last.screen === "game" && last.busy && !last.wheel && !last.rank && !(last.tip && last.tip.dialog)) {
    await waitUpdates(20);
    continue;
  }
  var lines = await ask("move");
  var m = JSON.parse(lines[lines.length - 1].substring(5));
  if (m === null) {
    await waitUpdates(20);
    continue;
  }
  for (var i = 0; i < m.input.length; i++) {
    var step = m.input[i];
    if (step.move) {
      // the board takes the square under the cursor in its update: hover first, as a player's mouse arrives
      await t.move(step.move[0] - 8, step.move[1] - 8);
      await waitUpdates(2);
      await t.move(step.move[0], step.move[1]);
      await waitUpdates(2);
    } else if (step.click) {
      await t.click(step.click[0], step.click[1]);
    }
  }
  if (twistLimit > 0 && m.kind === "twist") {
    if (params.get("jump") && !jumped && made + 1 >= twistLimit / 2) {
      jumped = true;
      await ask("level 10");
    }
    if (!ended && made + 1 >= twistLimit) {
      ended = true;
      await ask("lose");
    }
  }
  if (m.kind === "stop") stops++;
  else if (m.kind !== "twist") tips++;  // the tutorial's, the tips', the rank bar's and the end screens' clicks
  else made++;
  if (m.kind !== "stop" && made % 50 === 0) await t.shot(mode + "-" + made);
  await waitUpdates(10);
}
await t.shot(mode + "-end");

if (mode === "zen" && last.screen === "game") {
  // out with clicks: the tube's menu button, the options' Main Menu and Yes in "Save and Quit" (3D at 1228x768 art; the
  // page's ?quit= gives other points as x,y;x,y;x,y)
  var quit = (params.get("quit") || "220,672;490,628;487,460").split(";");
  for (var q = 0; q < quit.length; q++) {
    var p = quit[q].split(",").map(Number);
    await t.move(p[0] - 5, p[1] - 5);
    await waitUpdates(5);
    await t.click(p[0], p[1]);
    await waitUpdates(150);
    await t.shot("zen-quit" + q);
  }
  last = await state();
}
return { mode: mode, twists: made, stops: stops, tips: tips, seconds: Math.round((Date.now() - started) / 1000), last: last,
  errors: t.errors.length, retried: lost, refocused: refocused, jumped: jumped, ended: ended, scaled: scaled };
