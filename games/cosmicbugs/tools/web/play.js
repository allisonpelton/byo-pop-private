// Cosmic Bugs: a whole game played through the web page by the port's own bot, every move as real DOM events (the player-only
// page's byopop.test: mouse and key events on the canvas). Run it as the page's driver (docs/WEB_PLAYER.md, "Drivers"):
//
//   http://127.0.0.1:<port>/cosmicbugs.html?player=1&assets=cosmicbugs&fast=1&driver=games/cosmicbugs/tools/web/play.js
//        &hold=1  &shooter=manual|auto  &scale=2  &max=<updates>
//
// It starts on the title: the options' Shooter Mode item is clicked to the wanted mode, then Start Game, the instructions
// page's Play, and the bot plays (state, move, the move's input) until the game is over. On the first board Esc pauses and
// Enter resumes, and the focus is taken from the page (byopop.test.blur) and given back, the game frozen while it is away. After
// the game over: a click through to the Hall of Fame, its Back button, the title. ?scale=2 draws the canvas at twice its size
// (CSS), so that the events land at scaled points as they do in full screen, which the Browser pane cannot enter. ?max= ends a
// game by the `lose` test command after that many updates (default 60000, 10 ms each); the game's own play never needs one.
// The result is {ok, shooter, scale, boards, score, log, errors}.
var params = new URLSearchParams(location.search);
var shooter = params.get("shooter") === "manual" ? "manual" : "auto";
var scale = Number(params.get("scale") || 1);
var maxUpdates = Number(params.get("max")) || 60000;
var log = [];

function say(s) { log.push(t.updates() + ": " + s); }
function fail(s) { throw new Error(s); }

var TITLE = { start: [175, 313], options: [175, 357] };
var OPTIONS = { shooter: [400, 418], back: [400, 520] };
var PLAY = [400, 563];
var HALL_BACK = [400, 543];

// a press the game sees: it polls the buttons once a step, so the button stays down for a few updates
async function click(p, button) {
  await t.down(p[0], p[1], button);
  await t.wait(3);
  await t.up(p[0], p[1], button);
  await t.wait(2);
}
async function key(name) {
  await t.keyDown(name);
  await t.wait(3);
  await t.keyUp(name);
  await t.wait(2);
}
// The game steps only while its window is the active one (the original's own rule, so a lost focus freezes it). The Browser
// pane can take the focus from the page between two calls of the tool that drives it, and the game then stands still until
// it comes back, which is the game working as it should but would stall a run for good. So the page's own blur events are
// logged (an expected one, from the focus check below, is not), and a game that stands still while the page goes on gets
// the focus given back; the result counts them.
var expectBlur = false, blurs = 0, regains = 0, lastGame = -1, lastGameAt = 0;
window.addEventListener("blur", function () { if (!expectBlur) { blurs++; say("the page lost the focus by itself"); } });
async function state() {
  var s = await t.state();
  if (s.updates !== lastGame) { lastGame = s.updates; lastGameAt = t.updates(); }
  else if (t.updates() - lastGameAt > 300) {
    regains++;
    say("the game stood still at its update " + s.updates + ": the focus given back");
    await t.focus();
    await t.wait(10);
    lastGameAt = t.updates();
  }
  return s;
}
async function until(what, pred, updates) {
  var limit = t.updates() + (updates || 1500);
  for (;;) {
    var s = await state();
    if (pred(s)) return s;
    if (t.updates() > limit) fail("timed out waiting for " + what + ": " + JSON.stringify(s));
    await t.wait(10);
  }
}

// every command and what became of it, for the result when one fails
var trace = [];
var sendCommand = t.command;
t.command = function (line, opts) {
  var at = Math.round(performance.now()), u = t.updates();
  var p = sendCommand.call(t, line, opts);
  p.then(function (lines) { trace.push(at + "ms u" + u + " " + line + " -> " + (lines[lines.length - 1] || "").slice(0, 40)); },
    function (e) { trace.push(at + "ms u" + u + " " + line + " FAILED after " + (Math.round(performance.now()) - at) + "ms: " + e.message.slice(0, 30)); });
  return p;
};

var result = { ok: false, shooter: shooter, scale: scale, boards: 0, score: 0 };
try {
  // with &hold=1 the game waits for the driver: a first run on a wiped profile (no saved player, so the instructions page comes)
  if (params.get("hold") === "1") {
    await t.profile.wipe();
    // (the game restores its saves from the browser's backup of them at its start, so the backup goes too)
    try {
      Object.keys(localStorage).forEach(function (k) { if (k.indexOf("sexy-userdata-backup:/cbpersist") === 0) localStorage.removeItem(k); });
    } catch (e) { /* no storage: the files alone are wiped */ }
    await t.start();
  }
  if (scale !== 1) {
    var cv = document.getElementById("canvas");
    cv.style.width = Math.round(800 * scale) + "px";
    cv.style.height = Math.round(600 * scale) + "px";
    await t.wait(20);
    say("the canvas drawn at " + Math.round(cv.getBoundingClientRect().width) + "x" + Math.round(cv.getBoundingClientRect().height));
  }
  await until("the title", function (s) { return s.screen === "title"; }, 3000);
  await t.wait(20);
  await t.shot("title");

  await click(TITLE.options);
  await until("the options", function (s) { return s.screen === "options"; });
  await t.wait(20);
  if ((await t.state()).shooter !== shooter) {
    await click(OPTIONS.shooter);
    await until("the shooter mode " + shooter, function (s) { return s.shooter === shooter; });
  }
  await t.shot("options-" + shooter);
  await click(OPTIONS.back);
  await until("the title", function (s) { return s.screen === "title"; });
  say("shooter mode " + shooter + " chosen in the options");

  await click(TITLE.start);
  await until("the instructions", function (s) { return s.screen === "instructions"; });
  await t.wait(20);
  await t.shot("instructions");
  await click(PLAY);
  await until("a board", function (s) { return s.screen === "board"; });
  say("Start Game, the instructions page, Play: a board");

  var start = t.updates(), level = null, boards = 0, checked = false, ended = false, lastProgress = "", lastChange = t.updates(), moves = 0, nulls = 0;
  for (;;) {
    var st = await state();
    if (st.screen === "board") {
      if (st.level !== level) {
        if (level !== null) boards++;
        level = st.level;
        say("board " + level + ": quota " + st.quota + "%, score " + st.score + ", lives " + st.lives);
        if (boards < 2) await t.shot("board" + level);
      }
      if (st.over) {
        say("game over on board " + level + ": score " + st.score + ", fill " + st.fill + "%");
        await t.shot("gameover");
        await t.wait(30);
        await click([400, 300]);
        await until("the hall of fame", function (s) { return s.screen === "highscores" || s.screen === "highscore_entry"; });
        ended = true;
        continue;
      }
      if (!ended && t.updates() - start > maxUpdates) {
        try { await t.command("lose"); say("ended by the lose test command after " + maxUpdates + " updates"); ended = true; }
        catch (e) { await t.wait(5); }
        continue;
      }
      if (!checked && level === 1 && st.shots >= 2) {
        await key("Escape");
        await until("paused", function (s) { return s.paused; });
        await t.wait(20);
        await t.shot("paused");
        await key("Enter");
        await until("resumed", function (s) { return !s.paused; });
        say("Esc paused the board, Enter resumed it");
        var u0 = (await t.state()).updates;
        expectBlur = true;
        await t.blur();
        await t.wait(10);
        var u1 = (await t.state()).updates;
        await t.wait(30);
        var u2 = (await t.state()).updates;
        await t.shot("focus-lost");
        await t.focus();
        expectBlur = false;
        await t.wait(30);
        lastGame = -1;
        var u3 = (await t.state()).updates;
        say("focus lost: updates " + u0 + ", then " + u1 + " and " + u2 + " while it was lost, " + u3 + " after the regain");
        if (u2 !== u1) fail("the game kept updating with the focus lost (" + u1 + ", " + u2 + ")");
        if (u3 <= u2) fail("the game did not run again after the focus came back (" + u2 + ", " + u3 + ")");
        checked = true;
        continue;
      }
      var progress = [st.score, st.fill, st.lives, st.level, st.shots, st.won].join(",");
      if (progress !== lastProgress) { lastProgress = progress; lastChange = t.updates(); }
      else if (t.updates() - lastChange > 6000) { await t.shot("stuck"); fail("no progress for 6000 updates: " + JSON.stringify(st)); }
      var mv = await t.botMove();
      if (mv === null) { await t.wait(3); nulls++; continue; }
      await t.playMove(mv);
      moves++;
    } else if (st.screen === "highscores" || st.screen === "highscore_entry") {
      await t.wait(100);
      await t.shot("halloffame");
      say("the hall of fame, score " + st.score);
      await click(HALL_BACK);
      await until("the title", function (s) { return s.screen === "title"; });
      result.score = st.score;
      break;
    } else if (st.screen === "mission_complete") {
      say("the mission-complete screen");
      await t.shot("missioncomplete");
      await click([400, 300]);
      await t.wait(30);
    } else {
      fail("an unexpected screen " + JSON.stringify(st));
    }
  }
  result.boards = boards + 1;
  result.moves = moves;
  result.updates = t.updates() - start;
  say("game: " + result.boards + " boards, last score " + result.score + ", " + moves + " moves, " + result.updates + " updates");
  await t.shot("title-end");
  result.ok = t.errors.length === 0;
} catch (e) {
  result.error = String(e && e.message || e);
}
result.blurs = blurs;
result.regains = regains;
result.log = log;
result.trace = trace.slice(-12);
result.errors = t.errors.length;
return result;
