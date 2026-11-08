// Plays Bejeweled Deluxe in the player-only page with real DOM input (docs/WEB_PLAYER.md, "Drivers"): from the loading
// screen through the title, the first game's hint (the tutorial), the Options and Help dialogs, and a whole game of a mode,
// then the name entry. The port's own bot chooses each move (the test command "move", docs/TEST_COMMANDS.md) and this sends
// it as the page's mouse events; the state comes from "state". The mode is the page's ?mode= (normal or timed; normal by
// default), ?seconds= the time limit (900), ?focus=1 also loses and regains the focus a few moves into the game, and
// ?moves=N stops after N moves (the game goes on), and ?scale=2 draws the canvas at twice its size (CSS), so that the DOM
// events land at scaled points as they do in full screen, which the Browser pane cannot enter (its requestFullscreen() stays
// pending).
// Opened as .../bejeweled.html?player=1&assets=1&fast=1&silent=1&driver=games/bejeweled/tools/web/onegame.js&run=web
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "normal";
var limit = Number(params.get("seconds") || 900) * 1000;
var focusCheck = params.get("focus") === "1";
var maxMoves = Number(params.get("moves") || 0);
var scale = Number(params.get("scale") || 1);
var started = Date.now();
var made = 0, last = null, notes = {};

// the game's 640x480 coordinates (the buttons of the side bar and the dialogs, as in tools/realplay/drive.py)
var PLAY_LINK = [320, 398], NORMAL_MODE = [57, 200], TIMED_MODE = [135, 200], OPTIONS = [96, 261];
var OPTIONS_HELP = [330, 232], OPTIONS_DONE = [408, 299], HELP_DONE = [320, 399], PLAY_AGAIN = [415, 401], NO = [484, 200];

async function state() {
  try { return await t.state(); } catch (e) { return null; }
}
async function until(what, ok, ms) {
  var from = Date.now(), s;
  for (;;) {
    s = await state();
    if (s && ok(s)) return s;
    if (Date.now() - from > (ms || 60000)) throw new Error(what + ": " + JSON.stringify(s));
    await t.wait(10);
  }
}
async function clickAt(p) {
  await t.move(p[0] - 6, p[1] - 6);
  await t.wait(2);
  await t.move(p[0], p[1]);
  await t.wait(2);
  await t.click(p[0], p[1]);
  await t.wait(2);
}
async function play(input) {
  for (var i = 0; i < input.length; i++) {
    var step = input[i];
    if (step.move) {
      // the board takes the square under the cursor in its update: hover first, as a player's mouse arrives
      await t.move(step.move[0] - 8, step.move[1] - 8);
      await t.wait(2);
      await t.move(step.move[0], step.move[1]);
      await t.wait(2);
    } else if (step.click) {
      await t.click(step.click[0], step.click[1], step.button || "left");
    } else if (step.wait) {
      await t.wait(step.wait);
    } else if (step.key) {
      await t.key(step.key);
    }
  }
}

try {
  if (scale !== 1) {
    var cv = document.getElementById("canvas");
    cv.style.width = Math.round(640 * scale) + "px";
    cv.style.height = Math.round(480 * scale) + "px";
    await t.wait(20);
    notes.scale = { css: Math.round(cv.getBoundingClientRect().width) + "x" + Math.round(cv.getBoundingClientRect().height) };
  }
  // the title, then the play link
  await until("no title screen", function (s) { return s.screen === "title"; }, 180000);
  await t.shot("title");
  await clickAt(PLAY_LINK);
  var s = await until("no game after the play link", function (s) { return s.screen === "game" || s.screen === "dialog"; });
  if (s.screen === "dialog") {
    // a game saved when the page was left: "Continue old game?", answered No, by a click
    await t.shot("continue-question");
    await clickAt(NO);
    notes.continueQuestion = "No";
    s = await until("no game after the question", function (s) { return s.screen === "game"; });
  }
  // the hint dialog and the tutorial arrows come at the board's update 150 of a game of a profile that has played fewer than
  // five (?tutorial=1 says this profile is one: the run fails without them)
  try {
    s = await until("no tutorial", function (s) { return s.tutorial === true; }, 20000);
    notes.tutorial = true;
    await t.shot("hint");
  } catch (e) {
    if (params.get("tutorial") === "1") throw e;
    notes.tutorial = false;
  }
  // the Options dialog, Help in it, out again by Done twice
  await clickAt(OPTIONS);
  await until("no Options dialog", function (s) { return s.screen === "options"; }, 10000);
  await t.shot("options");
  await clickAt(OPTIONS_HELP);
  await until("no Help dialog", function (s) { return s.screen === "help"; }, 10000);
  await t.shot("help");
  await clickAt(HELP_DONE);
  s = await until("Help did not close", function (s) { return s.screen === "options" || s.screen === "game"; }, 10000);
  if (s.screen === "options") {
    await clickAt(OPTIONS_DONE);
    await until("Options did not close", function (s) { return s.screen === "game"; }, 10000);
  }
  notes.dialogs = "options, help";
  if (mode === "timed") {
    await clickAt(TIMED_MODE);
    s = await until("no timed game", function (s) { return s.screen === "game" && s.mode === "timed"; }, 20000);
  }
  await t.shot(mode + "-start");
  var levels = {}, stops = 0, nulls = 0;
  while (Date.now() - started < limit && !(maxMoves && made >= maxMoves)) {
    last = await state();
    if (!last) { await t.wait(5); continue; }
    if (last.screen !== "game") break;
    levels[last.level] = 1;
    if (last.over) { await t.wait(10); continue; }
    var lines = await t.command("move");
    var m = JSON.parse(lines[lines.length - 1].substring(5));
    if (m === null) {
      if (++nulls > 2000) throw new Error("no move for a long time: " + JSON.stringify(last));
      await t.wait(5);
      continue;
    }
    nulls = 0;
    await play(m.input);
    if (m.kind === "wait") continue;
    made++;
    if (made === 6 && focusCheck) {
      // the focus lost and regained: the board pauses and its updates stand still, then it runs again
      await t.blur();
      await t.wait(20);
      var a = await state();
      await t.wait(60);
      var b = await state();
      await t.shot("focus-lost");
      await t.focus();
      await t.wait(60);
      var c = await state();
      notes.focus = { lost: { paused: a.paused, tick: a.tick, drain: a.drain }, later: { paused: b.paused, tick: b.tick, drain: b.drain }, gained: { paused: c.paused, tick: c.tick, drain: c.drain } };
      await t.shot("focus-gained");
    }
    if (made % 100 === 0) await t.shot(mode + "-" + made);
  }
  await t.shot(mode + "-end");
  last = await state();
  if (maxMoves && made >= maxMoves) {
    var vp = innerWidth + "x" + innerHeight, box = document.getElementById("canvas").getBoundingClientRect();
    return { mode: mode, moves: made, viewport: vp, canvas: Math.round(box.width) + "x" + Math.round(box.height), notes: notes, last: last,
      errors: t.errors.length };
  }
  // the name entry after a good game: typed with real keys, then Enter
  s = await until("no name entry or game-over screen", function (s) { return s.screen === "recordscore" || s.screen === "gameover"; }, 60000);
  // the name entry comes a moment after the game-over screen when the score earns a place in the table
  for (var k = 0; k < 60 && s.screen === "gameover"; k++) {
    await t.wait(10);
    s = await state();
  }
  if (s.screen === "recordscore") {
    await t.shot("recordscore");
    await t.type("Driver");
    await t.key("Enter");
    notes.name = "entered";
    s = await until("no game-over screen after the name", function (s) { return s.screen === "gameover"; }, 30000);
  }
  await t.shot(mode + "-over");
  notes.score = s.score;
  return { mode: mode, moves: made, levels: Object.keys(levels).map(Number), seconds: Math.round((Date.now() - started) / 1000),
    notes: notes, last: last, errors: t.errors.length };
} catch (e) {
  return { error: String(e && e.message || e), mode: mode, moves: made, notes: notes, last: last, errors: t.errors.length };
}
