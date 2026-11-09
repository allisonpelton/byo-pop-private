// The Ning Po tour in the player-only page with real DOM input only (docs/WEB_PLAYER.md, "Drivers"), the web twin of
// tools/realplay/drive.py: the start screen, the title's arrows one way and back, the options and their best times,
// the rules both pages, a focus loss and regain, then ?games= games (2 by default) on the port's own bot (the test
// command "move", docs/TEST_COMMANDS.md), the first with every board button (hint, show moves, pause and resume, a
// focus loss, undo, shuffle, New Game answered no), each to its end, the name, the best times and back to the title.
// Opened as .../ningpo.html?player=1&silent=1&assets=1&fast=1&hold=1&driver=games/ningpo/tools/web/tour.js&run=tour
// (served by tools/webtest/serve.py), with &scale=2 in place of full screen
var params = new URLSearchParams(location.search);
var games = Number(params.get("games") || 2);
var BOARD = { newgame: [47, 115], options: [50, 185], rules: [55, 220], hint: [50, 265], showmoves: [50, 300],
  pause: [50, 335], undo: [60, 370], shuffle: [50, 405] };
var ARROWS = { background: [[212, 97], [248, 97]], tileset: [[508, 96], [543, 96]], layout: [[270, 161], [484, 161]] };
var BESTTIMES = [282, 289];  // in the options (OptionsDialog::setup)
var notes = [];
// A hidden Browser pane slows timers and frames down to a few a second; a message to itself is not slowed, so the
// game's sleep (NingPoApp.cpp, ningpo_sleep) yields through one
(function () {
  var ch = new MessageChannel(), due = [];
  ch.port1.onmessage = function () { var f = due.shift(); if (f) f(); };
  Module.ningpoSetTimeout = function (f) { due.push(f); ch.port2.postMessage(0); };
})();

function say(text) { notes.push(text); }
async function until(what, test, updates) {
  var limit = t.updates() + (updates || 3000), s;
  do {
    s = await t.state();
    if (test(s)) return s;
    await t.wait(2);
  } while (t.updates() < limit);
  throw new Error(what + ": not reached; last state " + JSON.stringify(s));
}
async function bot(kinds) {
  for (var i = 0; i < 200; i++) {
    var m = await t.botMove();
    if (m && kinds.indexOf(m.kind) >= 0) {
      await t.playMove(m);
      return m;
    }
    await t.wait(5);
  }
  throw new Error("no move " + kinds.join("/"));
}
async function button(name, what, test) {
  await t.click(BOARD[name][0], BOARD[name][1]);
  var s = await until(what, test);
  say(name + ": " + what);
  return s;
}
function dialog(name) { return function (s) { return s.dialog === name; }; }
async function closeDialog(name) {
  await until("the " + name + " open", dialog(name));
  await bot(["default", "ok"]);
  await until("the " + name + " closed", function (s) { return s.dialog !== name; });
}
async function focus(where) {
  var before = await t.state();
  await t.blur();
  await t.wait(10);
  var lost = await t.state();
  await t.shot(where + "-focus-lost");
  await t.focus();
  await t.wait(10);
  // a press within 150 ms of the focus coming back is the click that activates the page and reaches no game
  // (framework/SexyAppBase.cpp, mActivationClickPending), as natively SDL drops a window's activating click
  await new Promise(function (r) { setTimeout(r, 200); });
  await t.wait(2);
  var back = await t.state();
  say("focus " + where + ": paused " + before.paused + ", lost " + lost.paused + ", regained " + back.paused);
  if (back.paused && !before.paused) {
    await bot(["resume"]);
    await until("resumed", function (s) { return !s.paused; });
  }
}

async function playGame(n, buttons, win) {
  await bot(["play"]);
  var s = await until("a deal", function (s) { return s.screen === "playing" && s.dialog === null && s.tiles > 0; });
  say("game " + n + ": layout " + s.layout + ", " + s.tiles + " tiles, " + s.moves + " free pairs");
  await t.shot("game" + n + "-deal");
  var pairs = 0, tried = {};
  for (var guard = 0; guard < 2000; guard++) {
    s = await t.state();
    if (s.dialog !== null || s.screen !== "playing") {
      if (s.dialog === "nomoves" || s.dialog === "name" || s.screen === "won" || s.screen === "gameover") break;
      await t.wait(5);
      continue;
    }
    if (win && pairs === 10) {
      // the bot's first-found pair never clears a board: the last game is won by the test command after ten real
      // pairs, and its end screens are answered by real input (as drive.py does)
      await t.command("win");
      s = await until("the game won", function (x) { return x.screen === "won" || x.dialog !== null; });
      say("game " + n + ": won by the test command after " + pairs + " pairs");
      break;
    }
    if (buttons && pairs === 3 && !tried.hint) {
      tried.hint = true;
      var h = s.hints;
      await button("hint", "a hint", function (x) { return x.hints === h + 1; });
      await t.shot("game" + n + "-hint");
      await button("showmoves", "show moves", function () { return true; });
      await t.wait(20);
      await t.shot("game" + n + "-showmoves");
      await button("pause", "paused", function (x) { return x.paused; });
      await t.shot("game" + n + "-paused");
      await button("pause", "resumed", function (x) { return !x.paused; });
      await focus("game" + n);
      continue;
    }
    if (buttons && pairs === 6 && !tried.undo) {
      tried.undo = true;
      var u = s.undos, tl = s.tiles, sh = s.shuffles;
      await button("undo", "an undo", function (x) { return x.undos === u + 1 && x.tiles === tl + 2; });
      await button("shuffle", "a shuffle", function (x) { return x.shuffles === sh + 1; });
      await t.shot("game" + n + "-shuffled");
      await button("newgame", "the new-game question", dialog("question"));
      await t.shot("game" + n + "-newgame-question");
      tl = (await t.state()).tiles;
      await closeDialog("question");
      var s2 = await t.state();
      if (s2.screen !== "playing" || s2.tiles !== tl) throw new Error("New Game answered no changed the game");
      say("New Game: no");
      continue;
    }
    var m = await t.botMove();
    if (!m || m.kind !== "pair") {
      await t.wait(5);
      continue;
    }
    var left = s.tiles;
    await t.playMove(m);
    s = await until("a pair matched", function (x) {
      return x.tiles < left || x.dialog !== null || x.screen !== "playing";
    });
    pairs++;
  }
  say("game " + n + ": " + pairs + " pairs by the bot, " + s.screen + ", dialog " + s.dialog + ", " + s.tiles +
    " tiles left, timer " + s.timer + ", penalty " + s.penalty);
  await t.shot("game" + n + "-end");
  if (s.dialog === "nomoves") await bot(["endgame"]);
  await bot(["continue"]);
  await until("the name entry", dialog("name"));
  await t.shot("game" + n + "-name");
  await bot(["name"]);
  await until("the best times", dialog("besttimes"));
  await t.shot("game" + n + "-besttimes");
  await bot(["ok"]);
  return until("the title", function (x) { return x.screen === "title" && x.dialog === null; });
}

// ?hold=1: the driver starts the game, from a fresh profile (the best times of an earlier run are in its backup too)
await t.profile.wipe();
Object.keys(localStorage).filter(function (k) { return k.indexOf("/nppersist-test/") >= 0; })
  .forEach(function (k) { localStorage.removeItem(k); });
var loadFrom = performance.now();
await t.start();
await until("the start screen", dialog("start"), 20000);
say("the start screen " + Math.round(performance.now() - loadFrom) + " ms after t.start()");
// ?scale=2 draws the canvas at twice its size (CSS), so that the page's events land at scaled points as they do in full
// screen, which the Browser pane cannot enter
var scale = Number(params.get("scale") || 1);
if (scale !== 1) {
  var cv = document.getElementById("canvas");
  cv.style.width = Math.round(640 * scale) + "px";
  cv.style.height = Math.round(480 * scale) + "px";
  await t.wait(20);
  var box = cv.getBoundingClientRect();
  say("scale " + scale + ": the canvas drawn at " + Math.round(box.width) + "x" + Math.round(box.height));
}
await t.shot("start");
await bot(["start"]);
var s = await until("the title", function (x) { return x.screen === "title" && x.dialog === null; });
await t.shot("title");
for (var what in ARROWS) {
  var v = s[what];
  await t.click(ARROWS[what][1][0], ARROWS[what][1][1]);
  s = await until(what + " on", function (x) { return x[what] !== v; });
  await t.click(ARROWS[what][0][0], ARROWS[what][0][1]);
  s = await until(what + " back", function (x) { return x[what] === v; });
  say("title arrows: " + what + " " + v + ", on and back");
}
await button("options", "the options", dialog("options"));
await t.shot("options");
await t.click(BESTTIMES[0], BESTTIMES[1]);
await until("the best times", dialog("besttimes"));
await t.shot("options-besttimes");
await closeDialog("besttimes");
await closeDialog("options");
s = await button("rules", "the rules", dialog("rules"));
await t.shot("rules0");
var arrow = s.buttons[s.buttons.length - 1];
await t.click(arrow[0], arrow[1]);
await t.wait(5);
await t.shot("rules1");
await t.click(arrow[0], arrow[1]);
await t.wait(5);
await t.shot("rules0-back");
await closeDialog("rules");
await focus("title");
for (var n = 1; n <= games; n++) await playGame(n, n === 1, n === games && games > 1);
return { notes: notes, updates: t.updates(), errors: t.errors.length };
