// The web counterpart of tools/realplay/full.py (CLAUDE.md, "Done means identical": code plays, never the session): from the
// loading screen through the title's play link, the new player's name, the welcome message, the menu's mode buttons, the
// tutorial of the puzzles and the one-time hints, through the bot's moves of each mode, to the game's end and back to the menu.
// Every step is real DOM input on the canvas (byopop.test move, click, key, type: docs/WEB_PLAYER.md, "Drivers"); the test
// commands only read the state ("state"), ask the bot for its move ("move"), open every mode ("unlock all") and end a game
// ("lose"; the puzzle modes end by "goto menu"). The game is held between updates like the native -hold run (pause and step),
// so a run does the same on every machine; ?realtime=1 lets it run on its own clock instead.
//
// Opened as http://127.0.0.1:18780/bejeweled2.html?player=1&assets=1&fast=1&hold=1&silent=1&driver=games/bejeweled2/tools/web/full.js&run=full
// (the Browser pane, hidden or not), then `await byopop.test.driver`. Parameters: modes (classic,action,puzzle,endless,twilight,
// hyper,cognito,finity), moves (120 in each), scale (the canvas drawn at that many times its size, so that the DOM events
// land at scaled points as they do in full screen, which the pane cannot enter), realtime.
var params = new URLSearchParams(location.search);
var modes = (params.get("modes") || "classic,action,puzzle,endless,twilight,hyper,cognito,finity").split(",");
var maxMoves = Number(params.get("moves") || 120);
var realtime = params.get("realtime") === "1";
var scale = Number(params.get("scale") || 1);
var NAME = "Ali";
var SECRET = { twilight: "classic", hyper: "action", cognito: "puzzle", finity: "endless" };
var PUZZLE_MODES = { puzzle: 1, cognito: 1 };
var TEXT_DIALOGS = [10, 27, 41];  // a high score's name, a new player's name, "Enter Name"
var report = {}, last = null, lastButton = null;

async function adv(n) {
  if (realtime) await t.wait(n);
  else await t.step(n);
}
async function state() {
  last = await t.state();
  return last;
}
function describe(s) {
  var o = {};
  ["screen", "dialogs", "menu", "mode", "phase", "level", "score", "tutorial", "busy"].forEach(function (k) { o[k] = s ? s[k] : null; });
  return JSON.stringify(o);
}
async function until(what, ok, maxIdle) {
  var s;
  for (var i = 0; i < (maxIdle || 3000); i++) {
    s = await state();
    if (ok(s)) return s;
    await adv(2);
  }
  throw new Error("timeout waiting for " + what + ": " + describe(s));
}
async function botMove() {
  var lines = await t.command("move");
  var line = lines[lines.length - 1];
  if (line.indexOf("MOVE ") !== 0) throw new Error("move: " + line);
  var body = line.substring(5);
  return body === "null" ? null : JSON.parse(body);
}
async function ok(line) {
  var lines = await t.command(line);
  var a = lines[lines.length - 1];
  if (a.indexOf("OK") !== 0) throw new Error('"' + line + '": ' + a);
}
// a hand's input in game pixels: the cursor arrives at a place before the press, as a mouse's does
async function hover(p) {
  await t.move(p[0] - 8, p[1] - 8);
  await adv(2);
  await t.move(p[0], p[1]);
  await adv(2);
}
async function click(p) {
  await hover(p);
  await t.click(p[0], p[1], "left");
  await adv(2);
}
async function playInput(steps) {
  for (var i = 0; i < steps.length; i++) {
    var st = steps[i];
    if (st.move) await hover(st.move);
    else if (st.click) await t.click(st.click[0], st.click[1], st.button === "right" ? "right" : "left");
    else if (st.press) await t.down(st.press[0], st.press[1], "left");
    else if (st.release) await t.up(st.release[0], st.release[1], "left");
    else if (st.key) await t.key(st.key);
    else if (st.wait) await adv(st.wait);
  }
}

// the dialog on top: a name by typing it, any other by the bot's click on its button; a message in front of the menu (the
// welcome) is the bot's null, and gets a click on its yes button (its no button when it has no other) once the button stood
// still for two questions
async function handleDialog(s) {
  var top = Math.max.apply(null, s.dialogs);
  if (TEXT_DIALOGS.indexOf(top) >= 0) {
    await t.type(NAME);
    await t.key("Enter");
  } else {
    var m = await botMove();
    if (m !== null && (m.kind === "dialog" || m.kind === "wait")) {
      await playInput(m.input);
    } else if (m === null && s.buttons && (s.buttons.yes || s.buttons.no)) {
      var b = s.buttons.yes || s.buttons.no, key = top + ":" + b[0] + "," + b[1];
      if (lastButton === key) {
        lastButton = null;
        await click(b);
      } else {
        lastButton = key;
        await adv(2);
        return;
      }
    } else {
      await adv(2);
      return;
    }
  }
  await adv(2);
}
function menuReady(s) {
  return s.screen === "menu" && s.menu === 1 && (!s.dialogs || !s.dialogs.length) && s.user && s.widgets && s.widgets.classic &&
    (s.widgets.classic[0] !== 0 || s.widgets.classic[1] !== 0);
}
async function toMenu() {
  for (var i = 0; i < 4000; i++) {
    var s = await state();
    if (menuReady(s)) return s;
    var w = s.widgets || {};
    if (s.dialogs && s.dialogs.length) await handleDialog(s);
    else if (s.screen === "title" && w.play) { await click(w.play); await adv(10); }
    else if (s.screen === "highscores" && w.back) { await click(w.back); await adv(10); }
    else await adv(2);
  }
  throw new Error("timeout waiting for the menu: " + describe(last));
}
async function startMode(mode) {
  var base = SECRET[mode] || mode, idle = 0;
  for (var i = 0; i < 4000; i++) {
    var s = await state();
    if (s.mode === mode && (s.screen === "game" || s.screen === "dialog")) return s;
    if (s.dialogs && s.dialogs.length && s.mode === null) {
      await handleDialog(s);  // a message that came after the menu (the welcome)
      idle = 0;
    } else if (menuReady(s) && idle === 0) {
      if ((mode in SECRET) !== !!s.secret) await click(s.widgets.secret);
      else await click(s.widgets[base]);
      idle = 1;
    } else {
      await adv(2);
      idle = idle < 40 ? idle + 1 : 0;  // a click that did nothing is made again after a while
    }
  }
  throw new Error("timeout waiting for the game of " + mode + ": " + describe(last));
}
async function play(mode) {
  var made = 0, nulls = 0, levels = {}, t0 = Date.now();
  while (made < maxMoves) {
    var s = await state();
    if (s.screen === "menu" && s.mode === mode) { await adv(2); continue; }  // the board is still sliding in
    if (s.screen !== "game" && s.screen !== "dialog") { report[mode + "-note"] = "left the game early: " + describe(s); break; }
    if (s.dialogs && s.dialogs.length) { await handleDialog(s); continue; }
    if (s.over) { report[mode + "-note"] = "the game is over: " + describe(s); break; }
    levels[s.level] = 1;
    var m = await botMove();
    if (m === null) {
      if (++nulls > 400) throw new Error("no move for a long time: " + describe(s));
      await adv(1);
      continue;
    }
    nulls = 0;
    await playInput(m.input);
    if (m.kind === "wait") continue;
    made++;
  }
  var e = await state();
  return { moves: made, levels: Object.keys(levels).map(Number), score: e.score, level: e.level, seconds: Math.round((Date.now() - t0) / 1000) };
}
async function endGame(mode) {
  var s = await state();
  if (s.screen === "game" && !s.over) await ok(mode in PUZZLE_MODES ? "goto menu" : "lose");
  await toMenu();
}

try {
  if (scale !== 1) {
    var cv = document.getElementById("canvas");
    cv.style.width = Math.round(640 * scale) + "px";
    cv.style.height = Math.round(480 * scale) + "px";
    await t.wait(20);
  }
  await t.profile.wipe();
  await t.start();
  if (!realtime) await t.pause();
  await until("the title", function (s) { return s.screen === "title" || s.screen === "dialog" || s.screen === "menu"; }, 20000);
  var s0 = await toMenu();
  await ok("unlock all");
  for (var i = 0; i < modes.length; i++) {
    var mode = modes[i];
    await startMode(mode);
    report[mode] = await play(mode);
    await t.shot("end-" + mode);
    await endGame(mode);
  }
  return { ok: true, modes: report, updates: t.updates(), errors: t.errors.length };
} catch (e) {
  try { await t.shot("failure"); } catch (e2) { }
  return { ok: false, error: String(e && e.message || e), modes: report, last: last, errors: t.errors.length };
}
