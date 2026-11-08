// Plays Pixelus in the web build with real DOM input only (the counterpart of games/pixelus/tools/realplay/drive.py): the body
// of an async function of byopop.test (t), for the player-only page (docs/WEB_PLAYER.md, "Drivers"):
//
//   http://127.0.0.1:18766/pixelus.html?player=1&assets=1&silent=1&fast=1&driver=games/pixelus/tools/web/play.js&puzzles=12
//
// Launch, loading link, menu, the new-user dialog, the story and help screens, the island, the temples and the boards are all
// played by clicks and keys. It reads `state` (the screen), `targets` (what can be clicked) and `move` (the port's bot) and sends
// the moves' input steps as DOM events. ?expert=1 ticks the new player's expert box, ?puzzles=N stops once the profile has N solved puzzles (12 by default), ?fsat=1 presses
// Alt+Enter after the first board has opened (and back at the end), ?focusat=1 takes the focus away during the third move of the
// first board and gives it back. It returns the counts, the last state and the shots it took.
var params = new URLSearchParams(location.search);
var PUZZLES = Number(params.get("puzzles")) || 12;
var FULL = params.has("fsat");
var FOCUS = params.has("focusat");
var EXPERT = params.has("expert");
var result = { boards: 0, moves: 0, notes: [] };
var done = {};
var lastTemple = 0;
var started = Date.now();
var TIMEOUT = (Number(params.get("timeout")) || 1500) * 1000;

function last(lines) { return lines[lines.length - 1]; }
async function json(cmd, key) {
  var lines = await t.command(cmd, { timeout: 60000 });
  var l = last(lines);
  return JSON.parse(l.substring(l.indexOf(key) + key.length));
}
function targets() { return json("targets", "TARGETS "); }
async function waitUpdates(n) { await t.wait(n || 12); }

async function pressButton(label, index) {
  var b = (await targets()).buttons;
  if (!label) {
    var named = b.filter(function (x) { return x.label; });
    if (named.length) b = named;
  } else {
    var m = b.filter(function (x) { return x.label.toLowerCase() === label.toLowerCase(); });
    if (m.length) b = m;
  }
  if (!b.length) return false;
  await t.click(b[index || 0].x, b[index || 0].y);
  return true;
}

async function playMove(mv) {
  var steps = mv.input || [];
  for (var i = 0; i < steps.length; i++) {
    var st = steps[i];
    if (st.move) { await t.move(st.move[0], st.move[1]); await waitUpdates(2); }
    else if (st.click) await t.click(st.click[0], st.click[1]);
  }
}

async function playBoard() {
  var moved = 0, stall = 0, focused = false;
  for (;;) {
    var s = await t.state();
    if (s.screen !== "board") return moved;
    var b = s.board || {};
    if (b.done) { await waitUpdates(20); return moved; }
    if (b.busy) { await waitUpdates(3); continue; }
    var mv = await t.botMove();
    if (mv === null) {
      if (++stall > 400) throw new Error("the board stood still");
      await waitUpdates(5);
      continue;
    }
    stall = 0;
    var before = b.moves;
    await playMove(mv);
    moved++;
    for (var k = 0; k < 100; k++) {
      var n = (await t.state()).board || {};
      if (n.moves !== before || n.busy || n.done) break;
      await waitUpdates(2);
    }
    if (FOCUS && !focused && moved === 3) {
      focused = true;
      await t.blur();
      await waitUpdates(30);
      await t.shot("focus_lost");
      await t.focus();
      await waitUpdates(10);
      result.notes.push("focus lost and regained");
    }
  }
}

async function altEnter() {
  await t.keyDown("Alt");
  await t.key("Enter", { alt: true });
  await t.keyUp("Alt");
  await waitUpdates(20);
}

try {
  // ?hold=1: the page waits for the driver, which wipes the profile (a run starts as a first run) and starts the game
  if (params.get("hold") === "1") { await t.profile.wipe(); await t.start(); }
  var s = await t.state();
  for (var g = 0; g < 4000 && s.screen === "loading"; g++) {
    var tb = (await targets()).buttons;
    if (tb.length) { await t.shot("loading"); await t.click(tb[0].x, tb[0].y); await waitUpdates(5); }
    else await t.wait(3);
    s = await t.state();
  }
  var lastScreen = null, same = 0, fullDone = false;
  for (;;) {
    if (Date.now() - started > TIMEOUT) throw new Error("timeout");
    s = await t.state();
    var scr = s.screen;
    if (scr === lastScreen) same++;
    else { same = 0; lastScreen = scr; await t.shot("s" + result.notes.length + "_" + scr); }
    if (same > 200) throw new Error("stuck on " + scr);
    if (scr !== "board" && (s.solved || 0) >= PUZZLES) break;
    if (scr === "menu") {
      (await pressButton("Play")) || (await pressButton(null, 0));
      await waitUpdates(10);
    } else if (scr === "dialog") {
      var tg2 = await targets();
      var tb2 = tg2.buttons;
      if (EXPERT) {
        var boxes = (tg2.checks || []).filter(function (c) { return c.id === 1 && !c.checked; });
        if (boxes.length) { await t.click(boxes[0].x, boxes[0].y); await waitUpdates(3); }
      }
      await t.type("Driver");
      await waitUpdates(3);
      var ok = tb2.filter(function (x) { return ["ok", "continue", "yes"].indexOf(x.label.toLowerCase()) >= 0; });
      var pick = (ok.length ? ok : tb2)[0];
      await t.click(pick.x, pick.y);
      await waitUpdates(10);
    } else if (scr === "story" || scr === "help" || scr === "completed" || scr === "completion") {
      await waitUpdates(40);
      if (!(await pressButton(null, 0))) await waitUpdates(10);
      await waitUpdates(10);
    } else if (scr === "island") {
      var hot = (await targets()).buttons.filter(function (x) { return x.label === "" && x.id >= 2; });
      hot.sort(function (a, b) { return a.id - b.id; });
      var fresh = hot.filter(function (x) { return !done[x.id]; });
      if (fresh.length) hot = fresh;
      var entered = false;
      for (var h = 0; h < hot.length; h++) {
        lastTemple = hot[h].id;
        await t.click(hot[h].x, hot[h].y);
        await waitUpdates(10);
        var sc = (await t.state()).screen;
        if (sc === "temple") { entered = true; break; }
        if (sc === "dialog") { await pressButton(null, 0); await waitUpdates(5); }
      }
      if (!entered && same > 5) throw new Error("no temple opens");
    } else if (scr === "temple") {
      var pz = (await targets()).puzzles;
      var open = pz.filter(function (q) { return !q.solved && !q.mystery; });
      if (!open.length) open = pz.filter(function (q) { return !q.solved; });
      if (!open.length) {
        done[lastTemple] = true;
        await pressButton(null, 0);
        await waitUpdates(20);
        continue;
      }
      await t.click(open[0].x, open[0].y);
      await waitUpdates(20);
    } else if (scr === "board") {
      if (FULL && !fullDone) {
        fullDone = true;
        await altEnter();
        result.notes.push("fullscreenElement " + (document.fullscreenElement ? "set" : "none"));
        await t.shot("fullscreen");
      }
      result.moves += await playBoard();
      result.boards++;
    } else {
      await waitUpdates(20);
    }
  }
  if (FULL) await altEnter();
  await t.shot("end");
  result.solved = (await t.state()).solved;
} catch (e) {
  result.error = String((e && e.message) || e);
}
result.updates = t.updates();
result.errors = t.errors.length;
return result;
