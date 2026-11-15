// The web counterpart of origcmp/record.py's `tour:<mode>` for the 3D game (1024 by 768, WebGL through SDL's GLES2 renderer): the page's own
// game plays a mode by the port's bot with real DOM input and takes the same screens as shots, so that tools/shotdiff can set them beside the
// native 3D build's (record_3d_tours.sh) and the original's (run_tour_visible.sh). Opened as
//   bejeweled2.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/bejeweled2/tools/web/tour3d.js&run=tour3d_<mode>&mode=<mode>
// served by tools/webtest/serve.py (launch configuration bejeweled2-webtest, port 18808), then `await byopop.test.driver`; the shots are
// build/webshots/<run>/<name>.png.
// Page parameters:
//   mode      classic, action, puzzle, endless, twilight, hyper, cognito or finity (default puzzle)
//   moves     the bot's moves in the board modes (default 40)
//   worlds    the puzzle modes: the planets warped through (default 1)
//   warp      the most shots of each WARP, one every `step` updates, until the game says "post warp" (default 120)
//   step      updates between the WARP's shots (default 6)
//   win       1: each puzzle is ended by `win` (the game's own ending, which leaves the gems on the board) instead of played by the bot
//   warpfirst the update of the first shot after the click (default 0: the first shot is `step` after it, as the rest); with it, shot k is at the click + warpfirst + k * step
//   fine      shots every 6 updates along the first N after the mode's click
//   realtime  1: the game on the browser's own clock (open the page without fast=1), as a player has it
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "puzzle";
var maxMoves = Number(params.get("moves") || 40);
var warpShots = Number(params.get("warp") || 120);
var warpStep = Number(params.get("step") || 6);
var warpFirst = Number(params.get("warpfirst") || 0);
var winCmd = params.get("win") === "1";  // the puzzles ended by `win` (the gems stay on the board) instead of played
var worlds = Number(params.get("worlds") || 1);
var realtime = params.get("realtime") === "1";  // the game on the browser's clock (open the page without fast=1), as a player has it
var fine = Number(params.get("fine") || 0);
var NAME = "Ali";
var SECRET = { twilight: "classic", hyper: "action", cognito: "puzzle", finity: "endless" };
var PUZZLE_MODES = { puzzle: 1, cognito: 1 };
var TEXT_DIALOGS = [10, 27, 41];  // a high score's name, a new player's name, "Enter Name"
var report = { shots: [] }, last = null, lastButton = null;

async function adv(n) { if (realtime) await t.wait(n); else await t.step(n); }
async function state() { last = await t.state(); return last; }
function describe(s) {
  var o = {};
  ["screen", "dialogs", "menu", "mode", "phase", "level", "score", "tutorial", "busy", "is3d"].forEach(function (k) { o[k] = s ? s[k] : null; });
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
async function shot(name) {
  var s = await state();
  report.shots.push([name, s.updates, s.phase]);
  await t.shot(name);
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
// the mode's button (the secret ones through the "?" button), then the game's own messages and the board at rest
async function startMode() {
  var base = SECRET[mode] || mode, idle = 0, clicked = false;
  for (var i = 0; i < 4000; i++) {
    var s = await state();
    if (s.mode === mode && (s.screen === "game" || s.screen === "dialog")) { clicked = true; break; }
    if (s.dialogs && s.dialogs.length && s.mode === null) {
      await handleDialog(s);
      idle = 0;
    } else if (menuReady(s) && idle === 0) {
      if ((mode in SECRET) !== !!s.secret) await click(s.widgets.secret);
      else await click(s.widgets[base]);
      idle = 1;
    } else {
      await adv(2);
      idle = idle < 40 ? idle + 1 : 0;
    }
  }
  if (!clicked) throw new Error("timeout waiting for the game of " + mode + ": " + describe(last));
  for (var k = 0; k < fine; k++) { await adv(6); await shot("f" + (k < 10 ? "0" : "") + k); }
}
// everything in front of the board (the tutorial, the one-time hints) answered, until the board is idle
async function settleBoard() {
  for (var i = 0; i < 4000; i++) {
    var s = await state();
    if (s.dialogs && s.dialogs.length) { await handleDialog(s); continue; }
    if (s.screen === "game" && s.phase === "idle") return s;
    await adv(2);
  }
  throw new Error("the board did not settle: " + describe(last));
}
async function playMoves() {
  var made = 0, nulls = 0, level = (await state()).level;
  while (made < maxMoves) {
    var s = await state();
    if (s.screen !== "game" && s.screen !== "dialog") break;
    if (s.dialogs && s.dialogs.length) { await handleDialog(s); continue; }
    if (s.over) break;
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
    s = await state();
    if (s.level !== level) {  // five shots along each level change
      level = s.level;
      for (var k = 0; k < 5; k++) { await adv(40); await shot("level" + level + "-" + k); }
    }
  }
  return made;
}

try {
  await t.profile.wipe();
  await t.start();
  if (!realtime) await t.pause();
  await until("the title", function (s) { return s.screen === "title" || s.screen === "dialog" || s.screen === "menu"; }, 20000);
  await toMenu();
  await ok("unlock all");
  await shot("menu");
  await startMode();
  await settleBoard();
  await adv(60);
  await shot("board");
  report.is3d = (await state()).is3d;
  if (mode in PUZZLE_MODES) {
    // each planet's five puzzles are played by the bot's moves (with win=1: ended by the game's own ending, `win`, which leaves the gems on
    // the board); between them the bot chooses the next pod, and with all five solved it clicks the dome, which is the WARP to the next planet
    for (var world = 0; world < worlds; world++) {
      var warped = false;
      for (var n = 0; n < 4000 && !warped; n++) {
        var s2 = await state();
        if (s2.screen !== "game" && s2.screen !== "dialog") throw new Error("left the game before the WARP: " + describe(s2));
        if (s2.dialogs && s2.dialogs.length) { await handleDialog(s2); continue; }
        if (winCmd && s2.phase === "idle" && s2.screen === "game") { await ok("win"); await adv(2); }
        var m = await botMove();
        if (m === null) { await adv(2); continue; }
        if (m.kind === "dome") {
          warped = true;
          await adv(200);  // the dome waits, as record.py's puzzle_world has it
          await shot("w" + world + "-solved");
          report["dome" + world] = m.input[0].move;
          await playInput(m.input.slice(0, 2));
          var u0 = (await state()).updates;  // the click's update
          await playInput(m.input.slice(2, 3));  // the click; the shots follow it
          var post = 0;
          for (var w = 0; w < warpShots; w++) {
            if (warpFirst > 0) {  // the shots at the offsets of a native recording (record.py's puzzle_world: 34 updates after the click, then every 14)
              var need = u0 + warpFirst + w * warpStep - (await state()).updates;
              if (need > 0) await adv(need);
            } else {
              await adv(warpStep);
            }
            await shot("w" + world + "-warp" + (w < 10 ? "0" : "") + w);
            var ph = (await state()).phase;
            if (ph === "post warp") post++;
            else if (post > 0) break;  // the warp is over
          }
        } else {
          await playInput(m.input);
        }
      }
      if (!warped) throw new Error("the planet's puzzles did not end in a dome: " + describe(last));
      await adv(300);  // the next planet's first screen: the dome's five pods, not yet a board
      await shot("w" + world + "-after-warp");
    }
  } else {
    report.moves = await playMoves();
    await settleBoard();
    await shot("board-played");
  }
  report.state = describe(await state());
  return { ok: true, report: report, updates: t.updates(), errors: t.errors.length };
} catch (e) {
  try { await t.shot("failure"); } catch (e2) { }
  return { ok: false, error: String(e && e.message || e), report: report, last: last, errors: t.errors.length };
}
