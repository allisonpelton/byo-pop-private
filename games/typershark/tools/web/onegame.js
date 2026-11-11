// Plays Typer Shark Deluxe in the player-only page with real DOM input only (docs/WEB_PLAYER.md, "Drivers"; the rule behind it,
// docs/REAL_INPUT.md: scripted, autoplay and Node runs never pass the browser's input path). From the loading screen it goes
// through the new-user dialog (the name typed with real keys, one wrong letter and a Backspace first), the WPM question, the
// menu, the mode's button, and then the whole mode: the adventure (every level the bot is asked to play: hints dismissed
// by key presses and clicks in turn, the map by the Space key or a chest's letter, boss and treasure levels, the end of the adventure),
// the abyss (until the game over), or the typing tutor (lessons, the stats screen, the way out). The port's own bot chooses
// every move (test command `move`, docs/TEST_COMMANDS.md; src/testhooks/Bot.cpp) and this sends it as the page's key and
// mouse events; the state comes from `state` (GAME_NOTES.md, section 9). A screenshot is only taken to check what is drawn.
//
// The game is held between updates as the native driver's -hold holds it (tools/playbymove, tools/realplay.py): an update runs
// only when the driver steps it, a letter every update ("wait 1" between steps) and the bot never meets a stale board. So a
// game plays out the same on every run of a seed, and the typing is as fast in game time as the native driver's.
//
// Opened (the server: python tools/webtest/serve.py --port 18803, launch.json "typershark-webtest") as
//   http://127.0.0.1:18803/typershark.html?player=1&assets=1&fast=1&silent=1&hold=1&driver=games/typershark/tools/web/onegame.js&run=web
// and read by calling, in the pane (each call under 40 s: the game runs on in between)
//   await Promise.race([byopop.test.driver, new Promise(r => setTimeout(r, 30000))]); byopop.test.drive
// (`byopop.test.drive` is the driver's live summary, `byopop.test.result` the final result once it is there: {mode, diff, moves,
// levels, wins, stop, screens, updates, seconds, notes, last, errors, prof}; stop is levels, won, endless, gameover, lessons, moves or
// time). Close any other tab of the same origin first (its Web Lock makes the page say "Already open") and pass the tab's id to every
// call when the pane has more than one tab: a call goes to the fronted tab, whose page may be another session's. A page takes about a
// minute to load (the staged files are imported) and the pane, hidden, plays about 10 bot moves a second: a whole adventure takes
// a long time, so run it in the background and read `byopop.test.drive` now and then. hold=1 makes the driver wipe the test profile (the game starts as a first run: the new-user
// dialog) and start the game itself; without hold=1 the game starts on its own with whatever profile the page has, and the
// driver plays on from the screen it finds (a menu, a "who are you" list is an error).
//
// Parameters (page's query string):
//   mode=adventure|abyss|tutor   the mode (adventure)
//   diff=0..4                    the adventure's difficulty: Easy, Normal, Hard, Expert, X-Treme (0)
//   levels=N                     adventure: stop on the map after the Nth level won (0: play to the end: the CONGRATULATIONS box)
//   win=home|keep                the end of the adventure: RETURN HOME (a new game starts; the run stops there) or KEEP GOING (the
//                                endless levels, N of them: endless=N) (home). The map before the box scrolls by itself, and the driver
//                                waits for it: the space bar there would start the endless levels without the box.
//   endless=N                    with win=keep: stop after N endless levels (3)
//   retry=N                      a game over in the adventure or abyss is answered PLAY AGAIN up to N times, then MAIN MENU (0)
//   depth=0..4                   abyss: the line of the CONTINUE? list (start from the beginning, 400 feet, ...), when the profile
//                                has a deepest depth (0)
//   lessons=N                    tutor: the lessons to play, the next button between them (1)
//   wpm=yes|no                   the WPM question: YES plays the typing test (lesson 0) first (no)
//   name=<text>                  the new player's name (Driver)
//   moves=N                      stop after N bot moves, the game going on (0: no limit)
//   seconds=N                    stop after N real seconds (3600)
//   focus=1                      lose and regain the focus once, a few moves into the first game
//   scale=2                      the canvas drawn at twice its size (CSS), so that the DOM events land at scaled points as they do
//                                in full screen, which the Browser pane cannot enter
//   keep=1                       with hold=1: keep the test profile (no wipe)
//   level=N                      start somewhere else by the test command `level N` (docs/TEST_COMMANDS.md), once, when the first board
//                                or lesson is up: an adventure site 0 to 35 (35: the last boss and the end), an abyss depth in feet, a
//                                tutor lesson 0 to 18; the rest of the run is the bot's play from there
//   lose=N                       after N bot moves of the first game, the test command `lose` (the last life goes): the game-over box
// Steps and the game's pixels each one lands on are in the tables below (the buttons the bot's `move` does not give: it clicks only
// a one-button box). The run fails, with the state it saw, on a page error, on a screen or dialog it does not know, on a dialog or screen
// that does not change after its button, and on a game whose score, level, typing and screen stand still for 20000 updates.
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "adventure";
var diff = Number(params.get("diff") || 0);
var maxLevels = Number(params.get("levels") || 0);
var winChoice = params.get("win") || "home";
var retries = Number(params.get("retry") || 0);
var endlessWanted = Number(params.get("endless") || 3);
var abyssLine = Number(params.get("depth") || 0);
var lessonsWanted = Number(params.get("lessons") || 1);
var wpmYes = params.get("wpm") === "yes";
var userName = params.get("name") || "Driver";
var maxMoves = Number(params.get("moves") || 0);
var limitMs = Number(params.get("seconds") || 3600) * 1000;
var focusCheck = params.get("focus") === "1";
var startLevel = params.has("level") ? Number(params.get("level")) : -1;
var loseAt = Number(params.get("lose") || 0);
var scale = Number(params.get("scale") || 1);
var started = Date.now(), updates0 = 0;
var made = 0, wins = 0, last = null, notes = {}, screens = {}, levelsSeen = [], stop = null;
var STALL_UPDATES = 20000, BATCH = 12;

// the game's 640x480 pixels (from the sources' layouts and shots of each screen)
var MENU_BUTTON = { adventure: [458, 82], abyss: [458, 186], tutor: [458, 290] };
var DIFF_BUTTON_Y = [164, 209, 254, 299, 344];			// Easy ... X-Treme, at x 320; Cancel is [320, 407]
var DIFF_X = 320;
var NEW_USER_NAME_OK = "Enter";
var MESSAGE_YES_X = 248, MESSAGE_NO_X = 392;			// the two-button boxes: the buttons' y comes with the box's height, by its header
var MESSAGE_Y = { "ASSESS WPM?": 341, "VIEW STATS?": 272, "QUIT?": 293 };
var GAMEOVER = { adventure: { again: [238, 333], menu: [401, 333] }, abyss: { again: [238, 374], menu: [394, 374] } };
var WIN_HOME = [237, 434], WIN_KEEP = [416, 434];		// RETURN HOME, KEEP GOING
var ABYSS_LIST = function (i) { return [285, 115 + 19 * i]; }, ABYSS_OK = [235, 221];
var TUTOR_NEXT = [612, 395], TUTOR_BACK_TO_MENU = [551, 459];
var STATS_BUTTONS = [[145, 359], [145, 396], [145, 323]], STATS_CLOSE = [530, 433];	// Accuracy, Adjusted WPM, Words Per Minute; Close
var HINT_WAYS = ["Enter", " ", "click", "Escape"];

function fail(what) { throw new Error(what + " | state: " + JSON.stringify(last)); }
var state = timed("state", async function () {
  var s = await t.state();
  last = s;
  return s;
});
// one frame at game time: n updates stepped (the game is held between steps)
var step = timed("step", async function (n) { await t.step(n); });
// the page's errors: the kit takes a printed line with a word like "fatal" for an error, and a typed word is one (FATAL, in the streak
// game), printed in the STATE and MOVE answers of the live channel: those answers are not errors
var ANSWER_LINE = /^(INFO: )?(STATE|MOVE) (\{|null)/;
var ignoredErrors = 0;
function realErrors() {
  var n = 0, first = null;
  for (var i = 0; i < t.errors.length; i++) {
    var e = t.errors[i];
    if (e.kind === "log" && ANSWER_LINE.test(e.text)) continue;
    n++;
    if (!first) first = e;
  }
  ignoredErrors = t.errors.length - n;
  return { n: n, first: first };
}
function checkErrors() {
  var r = realErrors();
  if (r.n) throw new Error("page error: " + JSON.stringify(r.first) + " | state: " + JSON.stringify(last));
}
// real milliseconds spent in each kind of call, to see where a slow run goes
var prof = {};
function timed(name, f) {
  return async function () {
    var t0 = performance.now();
    try { return await f.apply(null, arguments); } finally { prof[name] = Math.round((prof[name] || 0) + performance.now() - t0); prof[name + "#"] = (prof[name + "#"] || 0) + 1; }
  };
}
async function clickAt(p) {
  // a hand's click: the pointer arrives (hover, an update), moves on, then presses and releases
  await t.move(p[0] - 6, p[1] - 6);
  await step(2);
  await t.move(p[0], p[1]);
  await step(2);
  await t.click(p[0], p[1]);
  await step(2);
}
async function pressKey(k) {
  await t.key(k);
  await step(1);
}
// one move of the bot as real input, the steps one update apart as tools/playbymove puts them
var play = timed("play", async function (m) {
  var prev = null;
  for (var i = 0; i < m.input.length; i++) {
    var st = m.input[i];
    if (prev && !prev.wait && !st.wait) await step(1);
    if (st.key) await t.key(st.key);
    else if (st.click) {
      await t.move(st.click[0] - 4, st.click[1] - 4);
      await step(1);
      await t.click(st.click[0], st.click[1], st.button || "left");
    } else if (st.move) await t.move(st.move[0], st.move[1]);
    else if (st.wait) await step(st.wait);
    else fail("a step of a move that this driver does not know: " + JSON.stringify(st));
    prev = st;
  }
  await step(m.updates > 0 ? m.updates : 1);
});
var botMove = timed("move", function () { return t.botMove(); });
// the held game draws only as it updates (a draw every 10 updates with fast=1), so a few updates are stepped for what the state
// says to be on the screen
async function shot(name) { await step(10); await t.shot(mode + "-" + name); }
async function firstShot(name) {
  if (!notes.shots) notes.shots = {};
  if (notes.shots[name]) return;
  notes.shots[name] = 1;
  await shot(name);
}
// the progress watchdog's signature: what changes while a game is played
function signature(s) {
  var b = s.board || {}, tu = s.tutor || {}, mp = s.map || {};
  return [s.screen, s.base, s.dialogHeader, s.level, s.wave, s.score, s.lives, s.depth, s.won, b.typed, b.clear, s.diver,
    tu.state, tu.task, tu.correct, mp.chosen, mp.arrived, s.endless].join("|");
}

var lastSig = "", lastChange = 0, tries = { boxBetween: 0 };
// a button that must change the screen: clicked again after some loops, an error after a few
function attempt(key, limit) {
  tries[key] = (tries[key] || 0) + 1;
  if (tries[key] > (limit || 6)) fail("no change after clicking " + key + " " + (limit || 6) + " times");
}
function done(key) { tries[key] = 0; }

var levelSent = false, loseSent = false, hintCount = 0, focusDone = false, lessonsDone = 0, gameOvers = 0, wasWon = false, viewedStats = false, retried = 0;

// the focus lost and regained, a few moves into a game: the game's own pause, and it runs again after
async function focusProbe() {
  focusDone = true;
  await t.blur();
  await step(20);
  var a = await state();
  await step(60);
  var b = await state();
  await shot("focus-lost");
  await t.focus();
  await step(60);
  var c = await state();
  await shot("focus-gained");
  notes.focus = { lost: { paused: a.paused, timer: a.timer, typed: a.board.typed }, later: { paused: b.paused, timer: b.timer },
    gained: { paused: c.paused, timer: c.timer } };
  if (c.paused) {
    // paused still after the focus came back: a click on the board is what a player does next
    await clickAt([320, 240]);
    await step(30);
    var d = await state();
    notes.focus.afterClick = { paused: d.paused, timer: d.timer };
    if (d.paused) fail("the game is still paused after the focus came back and a click");
  }
}


try {
  if (params.get("hold") === "1") {
    // a first run: no players, the new-user dialog (the page wipes only its test profile)
    if (params.get("keep") !== "1") await t.profile.wipe();
    await t.start();
  } else await t.ready;
  updates0 = t.updates();
  t.pause();
  t.drive = { mode: mode, screen: "start" };
  await step(2);

  if (scale !== 1) {
    var cv = document.getElementById("canvas");
    cv.style.width = Math.round(640 * scale) + "px";
    cv.style.height = Math.round(480 * scale) + "px";
    await step(2);
    var box = cv.getBoundingClientRect();
    notes.scale = { css: Math.round(box.width) + "x" + Math.round(box.height) };
  }

  for (;;) {
    checkErrors();
    var s = await state();
    var scr = s.screen;
    screens[scr] = (screens[scr] || 0) + 1;
    var sig = signature(s);
    var now = t.updates() - updates0;
    if (sig !== lastSig) { lastSig = sig; lastChange = now; }
    else if (now - lastChange > STALL_UPDATES) fail("no progress for " + STALL_UPDATES + " updates");
    t.drive = { mode: mode, screen: scr, level: s.level, score: s.score, lives: s.lives, depth: s.depth, moves: made, wins: wins,
      updates: now, seconds: Math.round((Date.now() - started) / 1000), prof: prof };
    if (Date.now() - started > limitMs) { stop = "time"; break; }
    if (maxMoves && made >= maxMoves) { stop = "moves"; break; }

    if (scr === "title" || scr === "none") {
      await firstShot("title");
      await step(20);
      continue;
    }

    if (scr === "newuser") {
      await firstShot("newuser");
      // a wrong letter and a Backspace first, so that the dialog's text keys are the real thing
      await t.type(userName + "Z");
      await step(1);
      await pressKey("Backspace");
      await shot("newuser-typed");
      await pressKey(NEW_USER_NAME_OK);
      await step(10);
      s = await state();
      if (s.screen === "newuser") fail("the new-user dialog did not take the name");
      if (s.user !== userName) fail("the player is " + JSON.stringify(s.user) + ", not " + userName);
      notes.user = s.user;
      continue;
    }

    if (scr === "message") {
      var head = s.dialogHeader;
      var y = MESSAGE_Y[head];
      notes.messages = notes.messages || [];
      notes.messages.push(head);
      await firstShot("message-" + head.replace(/[^A-Za-z]/g, ""));
      if (head === "ASSESS WPM?") {
        await clickAt([wpmYes ? MESSAGE_YES_X : MESSAGE_NO_X, y]);
        await step(10);
        notes.wpm = wpmYes ? "yes" : "no";
        continue;
      }
      if (head === "VIEW STATS?") {
        lessonsDone++;
        notes.lessons = (notes.lessons || []).concat([s.level]);
        var view = !viewedStats;
        await clickAt([view ? MESSAGE_YES_X : MESSAGE_NO_X, y]);
        await step(20);
        if (view) viewedStats = true;
        continue;
      }
      if (head === "QUIT?" && mode === "tutor") {
        await clickAt([MESSAGE_YES_X, y]);
        await step(20);
        stop = "lessons";
        continue;
      }
      // a box with one button: the bot's click (the typing test's "Test Completed!", a notice)
      var mv = await botMove();
      if (mv && mv.what === "dialog") { await play(mv); await step(10); continue; }
      fail("a message box this driver does not know: " + head);
    }

    if (scr === "menu") {
      if (stop) break;
      attempt("menu " + mode, 12);
      await firstShot("menu");
      await clickAt(MENU_BUTTON[mode]);
      await step(20);
      continue;
    }

    if (scr === "difficulty") {
      done("menu " + mode);
      attempt("difficulty", 6);
      await firstShot("difficulty");
      await clickAt([DIFF_X, DIFF_BUTTON_Y[diff]]);
      await step(20);
      continue;
    }

    if (scr === "abyssmode") {
      if (mode !== "abyss") fail("the CONTINUE? list outside the abyss");
      done("menu " + mode);
      attempt("abyss list", 6);
      await firstShot("abyssmode");
      if (abyssLine > 4) fail("depth= is the list's visible lines 0 to 4");
      if (abyssLine > 0) { await clickAt(ABYSS_LIST(abyssLine)); await step(5); }
      await clickAt(ABYSS_OK);
      await step(20);
      continue;
    }

    if (scr === "hint") {
      var way = HINT_WAYS[hintCount % HINT_WAYS.length];
      hintCount++;
      notes.hints = notes.hints || [];
      if (notes.hints.length < 12) notes.hints.push((s.base || "") + ":" + way + ":" + (s.dialogLines || "").substring(0, 40).replace(/\s+/g, " "));
      await firstShot("hint");
      if (way === "click") {
        // the box's own button, where the bot finds it
        var hm = await botMove();
        if (!hm || hm.what !== "dialog") fail("the bot has no click for the hint box");
        await play(hm);
      } else await pressKey(way);
      await step(10);
      s = await state();
      if (s.screen === "hint" && s.dialogLines === last.dialogLines && hintCount > 1) {
        // the same box again after a key: a way the box does not take (the next try is another way)
        notes.hintStuck = (notes.hintStuck || 0) + 1;
        if (notes.hintStuck > 8) fail("the hint box does not close");
      }
      continue;
    }

    if (scr === "options" || scr === "credits" || scr === "selectuser" || scr === "deleteuser" ||
        scr === "nowindowed" || scr === "highscores") fail("a screen this driver does not play: " + scr);

    if ((scr === "board" || scr === "tutor") && startLevel >= 0 && !levelSent) {
      levelSent = true;
      notes.level = (await t.command("level " + startLevel)).join(" ");
      await step(100);
      continue;
    }

    if (scr === "board") {
      done("menu " + mode); done("difficulty"); done("abyss list");
      if (levelsSeen.length === 0 || levelsSeen[levelsSeen.length - 1] !== s.level) {
        levelsSeen.push(s.level);
        wasWon = false;
        await shot("level" + s.level + "-start");
      }
      if (s.won && !wasWon) { wasWon = true; wins++; await shot("level" + s.level + "-won"); }
      if (s.level === -1 && mode === "adventure") {
        // the endless levels, after KEEP GOING
        if (winChoice !== "keep") fail("the endless levels began without the CONGRATULATIONS box");
        if (s.endless > endlessWanted) { stop = "endless"; break; }
      }
      // up to BATCH moves between two reads of the state (a read costs as much as a word; the bot's own answer says when a box
      // opened, the level is won or there is nothing to type)
      for (var k = 0; k < BATCH; k++) {
        var m = await botMove();
        if (m === null) { await step(10); break; }
        if (m.what === "dialog") {
          // a box that opened since the state was read (the state names it at the next read)
          if (++tries.boxBetween > 20) fail("a dialog the state does not name: " + JSON.stringify(m));
          await step(2);
          break;
        }
        tries.boxBetween = 0;
        await play(m);
        if (m.what === "continue") {
          // the level is won and the space bar was pressed
          if (!wasWon) { wasWon = true; wins++; }
          break;
        }
        made++;
        if (loseAt && made >= loseAt && !loseSent) {
          loseSent = true;
          notes.lose = (await t.command("lose")).join(" ");
          await step(60);
          break;
        }
        if (made === 6 && focusCheck && !focusDone) await focusProbe();
        if (made % 200 === 0) await shot("move" + made);
        if (maxMoves && made >= maxMoves) break;
      }
      continue;
    }

    if (scr === "map") {
      done("menu " + mode); done("difficulty");
      if (s.map && s.map.chests && !s.map.arrived) notes.forks = (notes.forks || 0) + (s.map.chests.length > 0 ? 1 : 0);
      if (maxLevels && wins >= maxLevels && s.map && s.map.arrived) { await shot("map-stop"); stop = "levels"; break; }
      if (s.map && s.map.arrived) await firstShot("map-arrived");
      wasWon = false;
      if (s.map && s.map.final && mode === "adventure" && s.level !== -1) {
        // the last site: the map scrolls on its own to the end and the CONGRATULATIONS box opens; the space bar before that starts the
        // endless levels instead, so a player who wants the end waits for it
        notes.finalWaits = (notes.finalWaits || 0) + 1;
        if (notes.finalWaits > 400) fail("no CONGRATULATIONS box on the final map");
        await step(30);
        continue;
      }
      var mm = await botMove();
      if (mm === null) { await step(20); continue; }
      await play(mm);
      continue;
    }

    if (scr === "gameover") {
      await firstShot("gameover");
      gameOvers++;
      var layout = GAMEOVER[s.mode === "abyss" ? "abyss" : "adventure"];
      if (retried < retries) {
        retried++;
        await clickAt(layout.again);
        await step(30);
        wasWon = false;
        continue;
      }
      stop = "gameover";
      attempt("game over main menu", 4);
      await clickAt(layout.menu);
      await step(30);
      continue;
    }

    if (scr === "wingame") {
      await firstShot("wingame");
      if (s.base === "board" || s.base === "hint") {
        // RETURN HOME started a new game; the box stays in the app's dialog list under the new board (src/WinWSAppButtons.cpp, case 0x85d)
        notes.wingameLingers = true;
        stop = "won";
        break;
      }
      if (maxLevels && wins >= maxLevels && winChoice === "keep") { stop = "levels"; break; }
      if (winChoice === "keep") {
        notes.won = "keep going";
        await clickAt(WIN_KEEP);
        await step(30);
        s = await state();
        if (s.screen === "wingame") fail("KEEP GOING did not close the CONGRATULATIONS box");
        continue;
      }
      notes.won = "return home";
      await clickAt(WIN_HOME);
      await step(60);
      s = await state();
      notes.afterReturnHome = { screen: s.screen, base: s.base, level: s.level };
      stop = "won";
      break;
    }

    if (scr === "tutor") {
      done("menu " + mode);
      if (!s.over) {
        var tm = await botMove();
        if (tm === null) { await step(10); continue; }
        await firstShot("tutor-typing");
        await play(tm);
        made++;
        continue;
      }
      // the lesson is finished and its questions are answered
      await firstShot("tutor-finished");
      if (lessonsDone >= lessonsWanted) {
        attempt("tutor back to menu", 4);
        await clickAt(TUTOR_BACK_TO_MENU);
        await step(20);
        continue;
      }
      attempt("tutor next", 4);
      await clickAt(TUTOR_NEXT);
      await step(30);
      s = await state();
      if (s.screen === "tutor" && !s.over) done("tutor next");
      continue;
    }

    if (scr === "stats") {
      // the statistics after a lesson: the graph buttons, then Close
      await firstShot("stats");
      for (var i = 0; i < STATS_BUTTONS.length; i++) { await clickAt(STATS_BUTTONS[i]); await step(5); }
      await shot("stats-wpm");
      await clickAt(STATS_CLOSE);
      await step(20);
      s = await state();
      if (s.screen === "stats") fail("Close did not leave the statistics");
      continue;
    }

    fail("an unknown screen: " + scr);
  }

  await shot("end");
  last = await state();
  checkErrors();
  t.resume();
  var out = { mode: mode, diff: diff, moves: made, levels: levelsSeen, wins: wins, stop: stop, screens: screens,
    updates: t.updates() - updates0, seconds: Math.round((Date.now() - started) / 1000), notes: notes, last: last, errors: realErrors().n, ignored: ignoredErrors, prof: prof };
  t.drive = out;
  return out;
} catch (e) {
  try { t.resume(); } catch (x) { }
  var bad = { error: String(e && e.message || e), mode: mode, moves: made, levels: levelsSeen, wins: wins, screens: screens, notes: notes, last: last,
    errors: realErrors().n, ignored: ignoredErrors, prof: prof };
  t.drive = bad;
  return bad;
}
