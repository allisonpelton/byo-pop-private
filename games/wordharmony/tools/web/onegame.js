// Plays one game of a mode in the player-only page with real DOM input only (docs/WEB_PLAYER.md, "Drivers"): the menu and
// level buttons by their places, and in the game each move the port's own bot chooses (the test command "move",
// docs/TEST_COMMANDS.md), sent as the page's mouse events: a ball pressed, carried and let go, a right click to submit.
// ?mode= tsunami, relax or think (tsunami by default), ?seconds= the time limit (600), ?levels= how many levels before
// it leaves the game by its own Save/Quit, Quit (0: until the game is over). The name entry gets ?name= (WHP).
// Opened as .../wordharmony.html?player=1&silent=1&assets=1&driver=games/wordharmony/tools/web/onegame.js
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "tsunami";
var limit = Number(params.get("seconds") || 600) * 1000;
var levels = Number(params.get("levels") || 0);
var name = (params.get("name") || "WHP").toUpperCase();
var started = Date.now();
var made = 0, last = null, log = [];
var MENU = { tsunami: [447, 237], relax: [330, 310], think: [530, 395] };
var LEVEL1 = { tsunami: [300, 200], relax: [175, 125], think: [300, 200] };

async function until(test, ms, what) {
  var t0 = Date.now();
  for (;;) {
    last = await t.state();
    if (test(last)) return last;
    if (Date.now() - t0 > ms) throw new Error("timed out waiting for " + what + " (screen " + last.screen + ")");
    await t.wait(15);
  }
}
async function click(x, y, button) {
  await t.move(x, y);
  await t.wait(2);
  await t.click(x, y, button || "left");
}
// a MOVE's input, step by step, as real events
async function send(m) {
  for (var st of m.input || []) {
    if (st.move) await t.move(st.move.x, st.move.y);
    else if (st.down) await t.down(st.down.x, st.down.y);
    else if (st.up) await t.up(st.up.x, st.up.y);
    else if (st.click) await t.click(st.click.x, st.click.y, st.click.button || "left");
    else if (st.wait) await t.wait(st.wait);
  }
  await t.wait(m.updates > 0 ? m.updates : 1);
}

await until(function (s) { return s.screen === "menu"; }, 120000, "the main menu");
await t.wait(30);
await click(MENU[mode][0], MENU[mode][1]);
// its levels screen (a first game goes straight to the help or the game), then the first level
last = await until(function (s) { return s.screen !== "menu"; }, 30000, "the mode's first screen");
if (last.screen === "levels") {
  await t.wait(40);
  await click(LEVEL1[mode][0], LEVEL1[mode][1]);
}
await t.shot(mode + "-start");
var levelsDone = 0, level = null;
while (Date.now() - started < limit) {
  last = await t.state();
  if (last.over || last.screen === "gameover" || last.screen === "nameentry" || last.screen === "congrats") break;
  if (level !== null && last.level !== level && last.screen === "game") {
    levelsDone++;
    log.push("level " + last.level + " at score " + last.score);
    if (levels && levelsDone >= levels) break;
  }
  if (last.screen === "game") level = last.level;
  var lines = await t.command("move");
  var m = JSON.parse(lines[lines.length - 1].substring(5));
  if (m === null) {
    await t.wait(30);
    continue;
  }
  if (m.what !== "wait") made++;
  await send(m);
}
var end = { moves: made, levels: levelsDone, screen: last.screen, level: last.level, score: last.score, over: last.over, log: log };
await t.shot(mode + "-end");
if (last.screen === "game" && !last.over) {
  // leave by the game's own Save/Quit and then its dialog's Don't Save (quitgamescreen: the popup at 300,200, the
  // button at 205,125 in it)
  await click(155, 30);
  await t.wait(60);
  await t.shot(mode + "-savequit");
  await click(505, 325);
  last = await until(function (s) { return s.screen === "menu"; }, 30000, "the menu");
  end.left = true;
  end.final = last.screen;
  return end;
}
// the game over: its CONTINUE, the name typed, DONE, the score table's DONE
await t.wait(60);
await click(430, 500);
last = await until(function (s) { return s.screen === "nameentry" || s.screen === "highscores" || s.screen === "menu"; }, 30000, "the name entry");
if (last.screen === "nameentry") {
  await t.wait(30);
  await click(400, 366);
  for (var c of name) { await t.key(c.toLowerCase()); await t.wait(5); }
  await t.shot(mode + "-name");
  await click(400, 470);
  last = await until(function (s) { return s.screen === "highscores"; }, 30000, "the score table");
}
if (last.screen === "highscores") {
  await t.wait(40);
  await t.shot(mode + "-scores");
  await click(400, 530);
  last = await until(function (s) { return s.screen === "menu"; }, 30000, "the menu");
}
end.final = last.screen;
return end;
