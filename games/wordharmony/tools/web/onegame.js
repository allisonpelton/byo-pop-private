// Plays one game of a mode in the player-only page with real DOM input only (docs/WEB_PLAYER.md, "Drivers"): the menu and
// level buttons by their places, and in the game each move the port's own bot chooses (the test command "move",
// docs/TEST_COMMANDS.md), sent as the page's mouse events: a ball pressed, carried and let go, a right click to submit.
// ?mode= tsunami, relax or think (tsunami by default), ?seconds= the time limit (600), ?levels= how many levels before
// it leaves the game by its own Save/Quit, Quit (0: until the game is over), ?level= a level to start at by the test
// commands (the buttons otherwise). The name entry gets ?name= (WHP).
// Opened as .../wordharmony.html?player=1&silent=1&assets=1&driver=games/wordharmony/tools/web/onegame.js
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "tsunami";
var limit = Number(params.get("seconds") || 600) * 1000;
var levels = Number(params.get("levels") || 0);
var name = (params.get("name") || "WHP").toUpperCase();
var started = Date.now();
var made = 0, last = null, log = [], paused = 0;
var MENU = { tsunami: [447, 237], relax: [330, 310], think: [530, 395] };
var LEVEL1 = { tsunami: [300, 200], relax: [175, 125], think: [300, 200] };

// the commands with a long wait for their answer: a hidden pane runs the page's timers seldom, and a command waits for
// the next update
function command(line) { return t.command(line, { timeout: 180000 }); }
async function state() {
  var lines = await command("state");
  var answer = lines[lines.length - 1];
  if (!/^STATE /.test(answer)) throw new Error("state: " + answer);
  return JSON.parse(answer.substring(6));
}
async function until(test, ms, what) {
  var t0 = Date.now();
  for (;;) {
    last = await state();
    if (test(last)) return last;
    if (Date.now() - t0 > ms) throw new Error("timed out waiting for " + what + " (screen " + last.screen + ")");
    await adv(15);
  }
}
// In the game the driver holds it between its steps (test.pause) and moves it on by test.step: a hidden pane runs the
// page's timers seldom and then many updates at once, so that the game would run on while a drag's events wait for their
// frames. The web build's fixes mod (steadyClock) times each frame by the updates, so a held game's time stands still.
var held = false;
function adv(n) { return held ? t.step(n) : t.wait(n); }
async function click(x, y, button) {
  await t.move(x, y);
  await adv(2);
  await t.click(x, y, button || "left");
}
// a MOVE's input, step by step, as real events
async function send(m) {
  for (var st of m.input || []) {
    if (st.move) await t.move(st.move.x, st.move.y);
    else if (st.press) await t.down(st.press.x, st.press.y);
    else if (st.release) await t.up(st.release.x, st.release.y);
    else if (st.click) await t.click(st.click.x, st.click.y, st.click.button || "left");
    else if (st.wait) await adv(st.wait);
    if (held && !st.wait) await t.step(1);
  }
  await adv(m.updates > 0 ? m.updates : 1);
}

// ?botdebug=1: the bot's plans on the log (the test command botdebug)
if (params.get("botdebug")) await command("botdebug");
await until(function (s) { return s.screen === "menu"; }, 600000, "the main menu");
// ?level=<n>: that level by the test commands goto and level (docs/TEST_COMMANDS.md) instead of the buttons; the
// rest is real input
var startLevel = Number(params.get("level") || 0);
if (startLevel > 0) {
  await t.wait(150);
  await command("goto " + mode);
  await t.wait(100);
  await command("level " + startLevel + " " + mode);
  last = await until(function (s) { return s.screen !== "menu"; }, 600000, "the level");
}
// the menu's button, again while the menu still shows (a click during the menu's fade-in is not taken)
for (var tries = 0; last.screen === "menu" && tries < 5; tries++) {
  await t.wait(150);
  await click(MENU[mode][0], MENU[mode][1]);
  await t.wait(60);
  last = await state();
}
// its levels screen (a first game goes straight to the help or the game), then the first level
last = await until(function (s) { return s.screen !== "menu"; }, 600000, "the mode's first screen");
// (the page's updates are 10 ms, a third of a frame: the screens' fades need a few hundred of them)
for (var tries = 0; last.screen === "levels" && tries < 5; tries++) {
  await t.wait(150);
  await click(LEVEL1[mode][0], LEVEL1[mode][1]);
  last = await until(function (s) { return true; }, 600000, "the levels screen");
  await t.wait(60);
  last = await state();
}
await t.shot(mode + "-start");
t.pause();
held = true;
var levelsDone = 0, level = null;
while (Date.now() - started < limit) {
  last = await state();
  if (last.over || last.screen === "gameover" || last.screen === "nameentry" || last.screen === "congrats") break;
  if (level !== null && last.level !== level && last.screen === "game") {
    levelsDone++;
    log.push("level " + last.level + " at score " + last.score);
    if (levels && levelsDone >= levels) break;
  }
  if (last.screen === "game") level = last.level;
  if (last.state === 4) {
    // paused: the pane lost the focus, as a player's window can (the original pauses then too); the focus given back
    // and the game's own P key
    paused++;
    await t.focus();
    await t.key("p");
    await adv(10);
    continue;
  }
  var lines = await command("move");
  var m = JSON.parse(lines[lines.length - 1].substring(5));
  if (m === null) {
    await adv(30);
    continue;
  }
  if (m.what !== "wait") made++;
  await send(m);
}
t.resume();
held = false;
var end = { moves: made, paused: paused, levels: levelsDone, screen: last.screen, level: last.level, score: last.score, over: last.over, log: log };
await t.shot(mode + "-end");
if (last.screen === "game" && !last.over) {
  // leave by the game's own Save/Quit and then its dialog's Don't Save (quitgamescreen: the popup at 300,200, the
  // button at 205,125 in it)
  await click(155, 30);
  await t.wait(60);
  await t.shot(mode + "-savequit");
  await click(505, 325);
  last = await until(function (s) { return s.screen === "menu"; }, 600000, "the menu");
  end.left = true;
  end.final = last.screen;
  return end;
}
// the game over: its CONTINUE, the name typed, DONE, the score table's DONE
await t.wait(60);
await click(430, 500);
last = await until(function (s) { return s.screen === "nameentry" || s.screen === "highscores" || s.screen === "menu"; }, 600000, "the name entry");
if (last.screen === "nameentry") {
  await t.wait(30);
  await click(400, 366);
  for (var c of name) { await t.key(c.toLowerCase()); await t.wait(5); }
  await t.shot(mode + "-name");
  await click(400, 470);
  last = await until(function (s) { return s.screen === "highscores"; }, 600000, "the score table");
}
if (last.screen === "highscores") {
  await t.wait(40);
  await t.shot(mode + "-scores");
  await click(400, 530);
  last = await until(function (s) { return s.screen === "menu"; }, 600000, "the menu");
}
end.final = last.screen;
return end;
