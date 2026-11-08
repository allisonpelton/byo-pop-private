// The web play-through of Mahjong Escape: Ancient Japan and Ancient China (the same script for both: the two games
// answer the same test commands, docs/TEST_COMMANDS.md). A driver of the shared page (docs/WEB_PLAYER.md, "Drivers"): the
// body of an async function of `t` (byopop.test). Every click and key goes through the page as the DOM's own events
// (t.click, t.key, t.type) on the canvas, which Emscripten and SDL take on a player's path; the port's own bot chooses
// every pair (`move`, played by t.playMove), and this script reads the game's state to know where it is.
//
//   http://127.0.0.1:18780/mahjongjapan.html?player=1&assets=1&fast=1&silent=1&driver=games/mahjongjapan/tools/web/realplay.js
//
// ?steps=N (read from the page's address) is the number of adventure levels played (default all), ?classic=N the number
// of classic games (default 2: the first by the bot to its end, the second ended by `win` after ten real pairs).
// byopop.test.result is {ok, log, levels, errors}: the log lists every screen reached, with its update count.
var params = new URLSearchParams(location.search);
var wantLevels = params.has("levels") ? parseInt(params.get("levels"), 10) : 1000;
var wantClassic = params.has("classic") ? parseInt(params.get("classic"), 10) : 2;
var out = [];
function say(text) { out.push(t.updates() + " " + text); }
function btns(s) { var m = {}; s.buttons.forEach(function (b) { m[b.cmd] = [b.x, b.y]; }); return m; }
async function state() { return await t.state(); }
async function until(what, test, timeoutMs) {
  var end = Date.now() + (timeoutMs || 120000), s = null;
  while (Date.now() < end) {
    s = await state();
    if (test(s)) return s;
    await t.wait(2);
  }
  throw new Error(what + ": not reached; last state " + JSON.stringify(s));
}
async function press(s, cmd) {
  var b = btns(s)[cmd];
  if (!b) throw new Error("no button 0x" + cmd.toString(16) + " on " + s.screen + "/" + s.dialog + ": " + JSON.stringify(s.buttons));
  await t.move(b[0], b[1]);
  await t.click(b[0], b[1]);
}
async function toScreen(name) {
  return await until("screen " + name, function (s) { return s.screen === name && s.dialog === null && !s.busy; });
}
async function dialogOk(name) {
  for (var i = 0; i < 200; i++) {
    var s = await state();
    if (s.dialog !== name) return;
    if (!s.busy) {
      var mv = await t.botMove();
      if (mv) { await t.playMove(mv); await t.wait(10); continue; }
    }
    await t.wait(3);
  }
  throw new Error("the " + name + " did not close");
}
async function mapReady() {
  return await until("the map ready", function (s) { return s.screen === "map" && s.dialog === null && btns(s)[0xe6]; });
}
async function boardReady(screen) {
  var s = await until("the " + screen + " board", function (s) { return s.screen === screen && (s.dialog === null || s.dialog === "tip") && !s.busy; }, 60000);
  if (s.dialog === "tip") {
    await dialogOk("tip");
    s = await until("the " + screen + " board", function (s) { return s.screen === screen && s.dialog === null && !s.busy; }, 60000);
  }
  return s;
}

// the board to its end by the bot's moves; winAfter: ended by `win` after that many real pairs
async function playBoard(tag, winAfter) {
  var pairs = 0, stuck = 0, seen = {}, focusDone = false;
  for (var i = 0; i < 100000; i++) {
    var s = await state();
    var sig = s.screen + "/" + s.dialog;
    if (!seen[sig]) { seen[sig] = 1; say(tag + " new screen " + sig + " level " + s.level + " score " + s.score + " tiles " + s.tiles); await t.shot(tag + "-" + sig.replace(/[^a-z0-9]+/g, "_")); }
    if (s.screen !== "escape" && s.screen !== "classic") return { pairs: pairs, state: s };
    if (s.dialog !== null) {
      if (["tip", "no moves", "result", "pause"].indexOf(s.dialog) >= 0) { await dialogOk(s.dialog); continue; }
      return { pairs: pairs, state: s };
    }
    if (s.busy) { await t.wait(2); continue; }
    if (winAfter !== undefined && pairs === winAfter) {
      await t.command("win");
      say(tag + ": won by the test command after " + pairs + " pairs");
      winAfter = undefined;
      await t.wait(20);
      continue;
    }
    if (!focusDone && pairs === 5) {
      // the focus lost and regained in the middle of a game
      focusDone = true;
      await t.blur(); await t.wait(10); await t.shot(tag + "-focus-lost"); await t.focus(); await t.wait(10);
      var f = await state();
      say(tag + ": focus lost and regained, screen " + f.screen + " dialog " + f.dialog);
    }
    var mv = await t.botMove();
    if (!mv) { if (++stuck > 200) throw new Error(tag + ": no move for a long time: " + JSON.stringify(s)); await t.wait(3); continue; }
    stuck = 0;
    await t.playMove(mv);
    if (mv.kind === "pair" || mv.kind === "special") pairs++;
    else if (mv.kind === "shuffle") say(tag + ": no pair free, shuffled");
    if (pairs % 25 === 0 && (mv.kind === "pair" || mv.kind === "special")) say(tag + ": " + pairs + " pairs, " + s.tiles + " tiles left, score " + s.score + ", timer " + s.timer);
    await t.wait(1);
  }
  throw new Error(tag + " did not end");
}

// the classic board's buttons, the in-game menu and its dialogs, before the first pair
async function openGameMenu() {
  var s = await until("the board or its menu", function (s) { return (s.dialog === null || s.dialog === "game menu") && !s.busy; });
  if (s.dialog === null) await press(s, 0x109);
}
async function menuItem(cmd, name) {
  await openGameMenu();
  var s = await until("the game menu", function (s) { return s.dialog === "game menu" && !s.busy; });
  await press(s, cmd);
  s = await until("the " + name, function (s) { return s.dialog === name && !s.busy; });
  await t.shot("classic-" + name.replace(/ /g, "_"));
  await dialogOk(name);
  say("the " + name + " closed");
}
async function boardTour() {
  await boardReady("classic");
  await t.wait(60);
  var s = await boardReady("classic"); // the game's tip opens a moment after the board is first ready
  await press(s, 0x16f); await t.wait(10); await t.shot("classic-freetiles-off");
  await press(await state(), 0x16f); await t.wait(10);
  await press(await state(), 0x108); await t.wait(30); await t.shot("classic-hint");
  s = await until("settled", function (s) { return !s.busy; });
  var before = s.timer;
  await press(s, 0x10c); await t.wait(60);
  s = await boardReady("classic");
  say("shuffle before the first pair: the timer shows " + s.timer + " (was " + before + ")");
  if (s.timer < 20 || s.timer > 1000) throw new Error("the shuffle should add 20 s to the clock, it shows " + s.timer);
  await press(s, 0x10a);
  await until("the pause", function (s) { return s.dialog === "pause" && !s.busy; });
  await t.shot("classic-pause");
  await t.move(400, 300); await t.click(400, 300);
  await until("the pause closed", function (s) { return s.dialog === null; });
  await press(await state(), 0x109);
  await menuItem(15, "options");
  await menuItem(16, "tile sets");
  await menuItem(17, "backgrounds");
  await menuItem(14, "help");
  await openGameMenu();
  s = await until("the game menu", function (s) { return s.dialog === "game menu" && !s.busy; });
  await press(s, 18);
  await boardReady("classic");
}
var SLOTS = [[165, 140], [400, 140], [635, 140], [165, 305], [400, 305], [635, 305]];
async function classicGame(n, slot, winAfter) {
  await t.command("goto menu");
  var s = await toScreen("menu");
  await press(s, 0x84);
  s = await toScreen("classic list");
  await t.move(SLOTS[slot][0], SLOTS[slot][1]); await t.click(SLOTS[slot][0], SLOTS[slot][1]);
  await t.wait(10);
  await press(await state(), 0xc4);
  s = await boardReady("classic");
  say("classic game " + n + " begins: layout " + s.layout + ", " + s.tiles + " tiles, " + s.moves + " pairs free");
  if (n === 1) await boardTour();
  var r = await playBoard("classic" + n, winAfter);
  say("classic game " + n + ": " + r.pairs + " pairs by the bot, then " + r.state.screen + "/" + r.state.dialog + ", score " + r.state.score);
  for (var i = 0; i < 40; i++) {
    s = await state();
    if (s.screen !== "classic" && s.screen !== "escape") break;
    if (!s.busy && s.dialog !== null) { var mv = await t.botMove(); if (mv) await t.playMove(mv); }
    await t.wait(3);
  }
  say("classic game " + n + " over: now " + s.screen + "/" + s.dialog);
}

var ok = false, levelsDone = 0;
try {
  // the launch: the menu, a focus loss and regain, the help pages
  var s = await toScreen("menu");
  await t.shot("menu");
  await t.blur(); await t.wait(10); await t.shot("menu-focus-lost"); await t.focus(); await t.wait(10);
  say("menu: focus lost and regained, screen " + (await state()).screen);
  await press(s, 0x87);
  await until("the help", function (s) { return s.dialog === "help" && !s.busy; });
  await t.shot("help0");
  await dialogOk("help");
  // a new player's name
  s = await toScreen("menu");
  await press(s, 0x8e);
  await until("the name box", function (s) { return s.dialog === "newplayer" && !s.busy; });
  await t.shot("newplayer");
  await t.type("Bot");
  await t.wait(5);
  await t.key("Enter");
  say("player Bot made");
  // the adventure from its map
  s = await mapReady();
  await t.shot("map-start");
  var total = s.levels;
  for (var n = 1; n <= Math.min(wantLevels, total); n++) {
    s = await mapReady();
    if (s.progress !== n - 1) throw new Error("the map shows progress " + s.progress + ", expected " + (n - 1));
    await press(s, 0xe6);
    s = await until("level " + n, function (s) { return s.screen === "escape" && (s.dialog === null || s.dialog === "tip") && !s.busy; }, 60000);
    say("level " + n + " begins: layout " + s.layout + ", " + s.tiles + " tiles");
    var r = await playBoard("level" + n);
    say("level " + n + ": " + r.pairs + " pairs by the bot, then " + r.state.screen + "/" + r.state.dialog + ", score " + r.state.score);
    var end = Date.now() + 120000;
    for (;;) {
      s = await state();
      if (s.screen === "map" && s.dialog === null && btns(s)[0xe6]) break;
      if (s.screen === "ending") break;
      if ((!s.busy && (s.screen === "level done" || s.screen === "result")) || s.dialog !== null) {
        var mv = await t.botMove();
        if (mv) await t.playMove(mv);
      }
      await t.wait(3);
      if (Date.now() > end) throw new Error("level " + n + ": the map did not come back: " + JSON.stringify(s));
    }
    levelsDone = n;
    say("level " + n + " won, now " + s.screen + ", progress " + s.progress);
  }
  await t.shot("adventure-end");
  if (wantClassic >= 1) await classicGame(1, 0);
  if (wantClassic >= 2) await classicGame(2, 4, 10);
  ok = true;
} catch (e) {
  say("FAILED: " + e.message);
  try { await t.shot("failed"); } catch (e2) {}
}
return { ok: ok, levels: levelsDone, log: out, errors: t.errors.length };
