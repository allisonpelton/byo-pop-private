// The Splits window's auto splitter on a live game (docs/WEB_PLAYER.md, "Splits"): the front page opens the game in a desktop
// window, with the player-only page's test kit in it, and a driver plays runs of the game's speedrun.com categories through
// it while the Splits window, as a player has it, starts, splits and resets them. Runs in the front page, like deskcheck.js:
//   http://127.0.0.1:18780/index.html?silent=1&driver=tools/web/autosplitcheck.js
// The page rewrites its address before the driver runs, so the game is named in localStorage, "byopop-autosplitcheck" (a page
// id with a plan below; BookWorm when unset), set before loading the page. The game's moves come from its own bot (the
// `move` test command) and the standard `goto`, `win` and `lose` commands (docs/TEST_COMMANDS.md), all through the test
// kit's real DOM input; the window's frame is the desktop's own, with player=1&assets=1&fast=1 added, so that the game runs
// at the machine's speed. The result, window.byopopDeskCheck.result: {pass, fail, results: [{check, result, detail}]}.
// Every game's plan checks:
//   attach     the block is read from the window (layout 2, its id) and goes active once asked
//   start      a new game starts a run of the category
//   split      each level cleared splits it, and the category's end ends it
//   replaced   a new game during a run resets it, and the new game starts the next
//   lost       a game over during the run resets it
// (Mummy Maze: its catch is no game over, so "caught" checks the run goes on) and the game's own: Mummy Maze's 100% Run never
// starts on a Classic game ("mode") and splits at the first treasure, not at a chamber ("treasure"); BookWorm's Bookburner ends at a game over at rank 0 ("death") and resets at rank 1 ("rank1"); Seven
// Seas' Easy run never starts on a Hard game ("skill"); Rocket Mania's Easy run never starts on a Hard game or a Strategy one
// ("skill", "mode"). Heavy Weapon's (its own, instead of the common one, with no "replaced": a player starts no campaign
// during a run): the final level's results end the run in any segment ("split"); out of tanks in the campaign, TRY AGAIN
// plays the level again and the run goes on ("goeson"); QUIT to the high scores after it resets ("scores"); Survival and a
// continued campaign start nothing ("survival", "resumed"). Hammer Heads' (its own): Classic%, cut to Level 1 and Level 25,
// ends at the Go! after King Globus's trophy room ("split"), and Tough Cookie% never starts on a Classic game and starts on its
// own ("mode", "tough").
// Every reset's question (keep the new bests?) is answered No; the stored splits of the categories it ran are removed at the
// end, and the last run the window had open is put back.
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
// the Splits window's scripts load as it first opens (web/skins.js, fill)
await ByopopSkin.desk.ready("splits");
var GAME = "bookworm";
try { GAME = localStorage.getItem("byopop-autosplitcheck") || GAME; } catch (e) {}
var G = ByopopAutosplit.games[GAME];
var timer = ByopopSplits.timer();
if (!G || !timer || !ByopopSkin.desk.isOn()) return { pass: 0, fail: 1, results: [{ check: "page", result: "FAIL", detail: "no game " + GAME + ", no timer, or no desktop" }] };

var lastKey = null;
try { lastKey = localStorage.getItem("byopop-splits-last"); } catch (e) {}
var lastRun = timer.run();

// ---- the game's window ----
ByopopApps.open(GAME + ".html");
await wait(300);
var frame = document.querySelector("iframe.d95-app-frame");
frame.src = frame.src + "&player=1&assets=1&fast=1";
var F = null, t = null;
for (var i = 0; i < 600 && !t; i++) {
  await wait(100);
  F = frame.contentWindow;
  try { if (F.byopop && F.byopop.test && F.byopop.test.progress().state === "running") t = F.byopop.test; } catch (e) {}
}
if (!t) return { pass: 0, fail: 1, results: [{ check: "window", result: "FAIL", detail: "the game's test kit never came up" }] };

// the Splits window's polls, counted (it calls ByopopAutosplit.sample each time); the driver's own readings go round the count
var sample = ByopopAutosplit.sample, polls = 0;
ByopopAutosplit.sample = function () { polls++; return sample.apply(this, arguments); };
function snap() { var s = sample(F, GAME); return s && s.snap; }
// every change of the timer's phase and the block's counters, with the step the driver was at
var trace = [], step = "", lastT = "";
var tracing = setInterval(function () {
  var s = snap(), a = timer.attempt();
  var k = [step, a.phase, a.index, s && s.active, s && s.boardSerial, s && s.goCount, s && s.goSerial, s && s.flags, s && s.gameOvers].join(" ");
  if (k !== lastT) { trace.push(Math.round(performance.now()) + " " + k); lastT = k; }
}, 3);
function att() { return timer.attempt(); }
// a reset's question, answered No: whether one was open
function answerNo() {
  var dl = Array.prototype.filter.call(document.querySelectorAll("dialog.spl-ask"), function (x) { return x.open; }).pop();
  if (!dl) return false;
  var label = ByopopStrings.t("splits.no");
  var b = Array.prototype.filter.call(dl.querySelectorAll("button"), function (x) { return x.textContent === label; })[0];
  if (b) b.click();
  return !!b;
}
// lets the auto splitter poll the state the game is in now, twice, the game held meanwhile: a hidden pane's timers may come
// far apart, so the wait is for the polls themselves
async function settle() {
  var n = t.pause(), p0 = polls;
  while (polls < p0 + 2) await wait(10);
  t.resume();
  return n;
}
async function cmd(line) { var l = await t.command(line); return l[l.length - 1]; }
// (Hammer Heads' bot only smashes: its boxes are pressed by its own command before each move, stepFirst)
var stepFirst = null;
async function botStep() {
  if (stepFirst) await stepFirst();
  var m = await t.botMove();
  if (m === null) { await t.wait(5); return null; }
  await t.playMove(m);
  // (a move with no input, such as AstroPop's "wait" through a warp flight, lets the game run its updates)
  if (!m.input || !m.input.length) await t.wait(m.updates || 10);
  return m;
}
// the command, after as many of the bot's moves as it takes for it to be answered OK
async function until(line, max) {
  for (var k = 0; k < (max || 400); k++) {
    var a = await cmd(line);
    if (/^OK /.test(a)) return true;
    await botStep();
  }
  throw new Error(line + ": never OK");
}
// (AstroPop's bot stays on its title, where the `goto` is answered: its plan sends it from there)
var titleGoto = false, unpauseFirst = false;
// a new game: the command (BookWorm's selector bot picks the mode), then the bot's moves until a new board is in play
async function newGame(line) {
  step += "+new";
  var before = (snap() || {}).boardSerial;
  // (Seven Seas' map, up after every tenth level, is closed by the bot first: a `goto` under it leaves it stuck)
  for (var j = 0; j < 50 && (await t.state()).screen === "map"; j++) await botStep();
  // (a list of commands, or of real input steps such as {click: [x, y]}: Mummy Maze leaves a game only by its Quit to Main
  // button, and its bot starts the game `set botmode` names from the main menu)
  if (titleGoto || (await t.state()).screen !== "title")
    for (var c of [].concat(line)) { if (typeof c === "string") await until(c, 50); else await t.playMove({ input: [c] }); }
  for (var k = 0; k < 400; k++) {
    var s = snap();
    if (s && s.boardSerial && s.boardSerial !== before && s.inGame) {
      // (AstroPop's new game opens with its full-screen help, which pauses it: a `win` under it counts a clear but never
      // goes on to the next level, so the bot closes it first)
      for (var u = 0; unpauseFirst && u < 100 && (snap().flags & 0x80); u++) await botStep();
      await settle();
      return s.boardSerial;
    }
    await botStep();
  }
  throw new Error("no new game");
}
function name(tp) { return tp.sub ? tp.category + " - " + tp.sub : tp.category; }
// (segments: other names than the template's, as a player may cut them: Hammer Heads' Level 1 and Level 25)
async function load(category, segments) {
  step = category;
  var tp = G.templates.filter(function (x) { return name(x) === category; })[0];
  timer.load({ game: G.name, category: category, offset: 0, attempts: 0, segments: (segments || tp.segments).map(function (n) { return { name: n, pb: { rt: null, gt: null }, best: { rt: null, gt: null } }; }) });
  // (the auto splitter's first poll of the loaded run comes before the game starts: a start in that poll is taken as already
  // seen)
  await settle();
}
// clears the level in play, once the bot has reached one: the timer's index after the auto splitter saw it
// (BookWorm's clear is the `win` itself; Seven Seas' comes when the level's panel is closed, so the bot plays on until the
// block counts it)
// (Typer Shark's split is the return to the map after the level is left, its progress rising: with splitByProgress the bot
// plays on until it has)
var splitByProgress = false;
async function clearLevel() {
  var before = snap().boardLevelClears, p0 = snap().progress;
  // (AstroPop answers ERROR to a `win` until the level is in play and a brick has been taken, so the bot plays through its
  // help, the bonus round and the warp flight first: a longer wait with unpauseFirst)
  await until("win", unpauseFirst ? 3000 : 400);
  for (var k = 0; k < 400 && snap().boardLevelClears === before; k++) await botStep();
  for (var j = 0; splitByProgress && j < 400 && snap().progress === p0; j++) await botStep();
  await settle();
  return att();
}
// (Rocket Mania counts the loss at its game-over screen, 300 updates after the game ends)
async function lose() { var g = snap().gameOvers; await until("lose"); for (var k = 0; k < 200 && snap().gameOvers === g; k++) await t.wait(5); await settle(); }
// the timer back to idle, its question answered No
async function resetByHand() { if (att().phase !== "idle") { timer.reset(); await wait(50); answerNo(); await wait(50); } }
// waits up to ms for a reset's question (answered No) or an idle timer
async function sawReset(ms) {
  for (var k = 0; k < ms / 20; k++) { if (answerNo()) { await wait(50); return "asked"; } if (att().phase === "idle") return "idle"; await wait(20); }
  return null;
}

// the checks every plan makes on a category ending at its last level: a run started at a new game (newLine), split at each
// level and ended at the last, then reset by a new game and by a game over
// opts.finish: the end comes after the last level clear (Mummy Maze's next pyramid), so the bot plays on until the run ends;
// opts.noGameOver: the game's `lose` is no game over (Mummy Maze's catch), so the check is that it leaves the run running
async function common(category, levels, newLine, opts) {
  opts = opts || {};
  snap();
  await t.wait(5);
  var s0 = snap();
  note("attach", !!s0 && s0.active === 1, s0 ? "active " + s0.active : "no block read");

  await load(category);
  var serial = await newGame(newLine);
  var a = att();
  note("start", a.phase === "running" && a.index === 0, category + ": phase " + a.phase + ", index " + a.index + ", board " + serial);
  var idx = [];
  for (var lv = 1; lv <= levels; lv++) idx.push((await clearLevel()).index);
  if (opts.finish) { for (var k = 0; k < 400 && att().phase === "running"; k++) await botStep(); await settle(); }
  a = att();
  var wantIdx = idx.slice(0, opts.finish ? levels : levels - 1).every(function (x, k) { return x === k + 1; });
  note("split", wantIdx && a.phase === "ended", "indexes after each clear " + idx.join(",") + "; then " + a.phase + ", progress " + snap().progress);
  await resetByHand();

  // a new game resets, and starts the next run
  await newGame(newLine);
  await clearLevel();
  var a1 = att(), s1 = snap().boardSerial, resetCause = null;
  var p = (async function () { resetCause = await sawReset(30000); })();
  var serial2 = await newGame(newLine);
  await p;
  await settle();
  a = att();
  note("replaced", a1.phase === "running" && a1.index === 1 && resetCause !== null && a.phase === "running" && a.index === 0,
       "before: " + a1.phase + " at " + a1.index + " (board " + s1 + "); reset " + resetCause + "; after: " + a.phase + " at " + a.index + " (board " + serial2 + ")");

  await clearLevel();
  if (opts.noGameOver) {
    await until("lose");
    for (var j = 0; j < 30; j++) await botStep();
    await settle();
    a = att();
    note("caught", a.phase === "running" && a.index === 1, "after the catch and a try again: " + a.phase + " at " + a.index + "; game overs " + snap().gameOvers);
    return;
  }
  // a game over resets
  resetCause = null;
  p = (async function () { resetCause = await sawReset(30000); })();
  await lose();
  await p;
  var s = snap();
  note("lost", resetCause !== null && att().phase === "idle", "reset " + resetCause + ", then " + att().phase + "; deaths " + [s.death0, s.death1, s.death2, s.death3].join(","));
}

// Cosmic Bugs' instructions page, then its Play button
var CB_PLAY = ["goto instructions", { wait: 10 }, { move: [400, 563] }, { wait: 3 }, { click: [400, 563] }];
// Mummy Maze's Quit to Main button, in the game's sidebar
var MM_QUIT = [{ click: [64, 462] }];

var PLANS = {
  venice: async function () {
    // (Venice's block has no inGame: a run's start is its GO, the click the bot makes away from the world map in a new game)
    // (a click held a few updates, as Venice's run_tests.py gives it)
    async function press(x, y) { await t.move(x, y); await t.wait(3); await t.down(x, y); await t.wait(3); await t.up(x, y); await t.wait(3); }
    async function screen() { var st = JSON.parse(String(await cmd("state")).slice(6)); return st; }
    async function goNow(mode) {
      var g = snap().goCount;
      // (a fresh page has no player yet: the name screen is up until one is made; a board starts only from the mode menu's
      // click, so the main menu's Play Now and the mode's button are pressed)
      await until("player", 50);
      await until("goto menu", 50);
      // (Flood is locked until unlocked; the unlock also passes every building, so the mode is set back to a new game's)
      await until("unlock all", 50);
      await until("set mode " + (mode === "journey" ? 1 : 0), 50);
      await until("set level 1", 50);
      await until("goto menu", 50);
      await t.wait(100);
      async function pressUntil(x, y, screen) {
        for (var n = 0; n < 8; n++) {
          await press(x, y);
          await t.wait(100);
          if (JSON.parse(String(await cmd("state")).slice(6)).screen !== screen) return;
        }
      }
      await pressUntil(400, 232, "main_menu");
      await pressUntil.apply(null, (mode === "journey" ? [400, 220] : [401, 270]).concat(["mode_select_menu"]));
      for (var k = 0; k < 600 && snap().goCount === g; k++) await botStep();
      await settle();
      return att();
    }
    snap();
    await t.wait(5);
    var s0 = snap();
    note("attach", !!s0 && s0.active === 1, s0 ? "active " + s0.active : "no block read");
    // Journey: GO starts it, building 1 splits, the map's CONGRATULATIONS! after building 71 ends it in any building
    await load("Journey");
    var a0 = await goNow("journey");
    var a1 = await clearLevel();
    // (the map comes up with CONGRATULATIONS! once the profile is past building 71; within a game the map comes back only at a
    // continue, the next building loading straight after a summary, so the profile is set there and the map brought up)
    await until("set level 72", 400);
    await until("goto level_select", 400);
    for (var k = 0; k < 30 && att().phase === "running"; k++) await t.wait(20);
    await settle();
    note("journey", a0.phase === "running" && a0.index === 0 && a1.index === 1 && att().phase === "ended",
         "started " + a0.phase + " at " + a0.index + "; after building 1: " + a1.index + "; after building 71: " + att().phase + ", wins " + snap().wins);
    // the tries run out: a reset
    await resetByHand();
    await load("Journey");
    await goNow("journey");
    var resetCause = null;
    var p = (async function () { resetCause = await sawReset(30000); })();
    // (a lost try ends a Journey game only with no try left: the tries are set once the level is in play)
    for (var q = 0; q < 400 && !(await screen()).board; q++) await botStep();
    await until("set lives 0", 400);
    await lose();
    await p;
    var s = snap();
    note("lost", resetCause !== null && att().phase === "idle", "reset " + resetCause + ", then " + att().phase + "; deaths " + [s.death0, s.death1, s.death2, s.death3].join(","));
    // Flood: a sunk building goes on
    await resetByHand();
    await load("Flood");
    a0 = await goNow("flood");
    resetCause = null;
    p = (async function () { resetCause = await sawReset(3000); })();
    await lose();
    await p;
    s = snap();
    note("flood", a0.phase === "running" && resetCause === null && att().phase === "running",
         "started " + a0.phase + "; after the sink: reset " + resetCause + ", " + att().phase + "; deaths " + [s.death0, s.death1, s.death2, s.death3].join(","));
  },
  heavyweapon: async function () {
    // (the bot takes the help, the play tips, the results' ADVANCE, the armory and the map; it leaves the game-over box alone,
    // whose TRY AGAIN and QUIT are pressed here: the box is 360 by 220 in the campaign, 240 in Survival, centred on 640 by 480,
    // TRY AGAIN at its foot's left and QUIT at its right, games/heavyweapon/src/GameOverDialog.cpp)
    titleGoto = true;
    async function press(x, y) { await t.move(x, y); await t.wait(3); await t.down(x, y); await t.wait(3); await t.up(x, y); await t.wait(3); }
    async function state() { return JSON.parse(String(await cmd("state")).slice(6)); }
    async function pressUntil(x, y, screen) {
      for (var n = 0; n < 8; n++) {
        await press(x, y);
        await t.wait(50);
        if ((await state()).screen !== screen) return true;
      }
      return false;
    }
    // a level cleared: `win` jumps the level's time to the boss's warning, then kills the boss once it can be hit, the bot
    // playing between; the block counts the clear as the results come up
    async function hwClear() {
      var before = snap().boardLevelClears;
      for (var k = 0; k < 1500 && snap().boardLevelClears === before; k++) { await cmd("win"); await botStep(); }
      await settle();
      return att();
    }
    // the next level in play, no dialog up (a game over needs the tank on the field)
    async function inPlay() {
      for (var k = 0; k < 600; k++) {
        var st = await state();
        if (st.screen === "game" && st.dialog === -1 && !st.paused && snap().state === 1) return;
        await botStep();
      }
      throw new Error("no level in play");
    }
    snap();
    await t.wait(5);
    var s0 = snap();
    note("attach", !!s0 && s0.active === 1, s0 ? "active " + s0.active : "no block read");
    // Mission Mode (PC): the click on Campaign starts it (its GO), level 1's results split, the final level's results end it
    await load("Mission Mode (PC)");
    await newGame("goto campaign");
    var a0 = att();
    note("start", a0.phase === "running" && a0.index === 0, "phase " + a0.phase + ", index " + a0.index + ", board " + snap().boardSerial);
    await cmd("set lives 5");
    var a1 = await hwClear();
    // (the next level made the last: ADVANCE adds one to the level, so it is set one short while the results are up)
    var shown = (await state()).screen;
    await cmd(shown === "results" ? "set level 18" : "set level 19");
    await inPlay();
    var lv = (await state()).level;
    var a2 = await hwClear();
    note("split", a1.phase === "running" && a1.index === 1 && a2.phase === "ended" && snap().wins > 0,
         "after level 1: " + a1.phase + " at " + a1.index + "; after level " + lv + " (from the " + shown + "): " + a2.phase + ", wins " + snap().wins);
    await resetByHand();
    // (a new campaign starts only from the screens a `goto` takes away, the menus, the high scores, the credits and the help
    // (LeaveScreens): one under the armory, the map or a cutscene leaves that screen up, and the bot works it for good. So the
    // bot takes the results and the final cutscene to the main menu or the high scores first. There is no "replaced": a
    // player starts no campaign during a run, as the level is left first, a quit)
    for (var e = 0; e < 600 && /^(results|none)$/.test((await state()).screen); e++) await botStep();
    // out of tanks: the game-over box's TRY AGAIN plays the level again on the same board, and the run goes on to its results
    await newGame("goto campaign");
    await cmd("set lives 5");
    await hwClear();
    await inPlay();
    var resetCause = null;
    var p = (async function () { resetCause = await sawReset(3000); })();
    await lose();
    await p;
    var c0 = att(), boxed = (await state()).over, s = snap();
    var tried = await pressUntil(235, 310, "gameover");
    await inPlay();
    await settle();
    var c1 = att(), back = snap().inGame;
    var c2 = await hwClear();
    note("goeson", c0.phase === "running" && c0.index === 1 && resetCause === null && boxed && tried && c1.phase === "running" && back === 1 && c2.index === 2,
         "after the game over: " + c0.phase + " at " + c0.index + ", reset " + resetCause + ", box " + boxed + ", deaths " + [s.death0, s.death1, s.death2, s.death3].join(",") +
         "; after TRY AGAIN: " + (tried ? "" : "(the box stayed) ") + c1.phase + ", in game " + back + "; after the level's results: " + c2.index);
    // out of tanks again, then QUIT to the high scores: the board ends, a quit
    await inPlay();
    await lose();
    resetCause = null;
    p = (async function () { resetCause = await sawReset(30000); })();
    var quit = await pressUntil(385, 310, "gameover");
    await p;
    note("scores", quit && resetCause !== null && att().phase === "idle", "QUIT " + (quit ? "pressed" : "missed") + " (" + (await state()).screen + "); reset " + resetCause + ", then " + att().phase);
    // Survival never starts the campaign's run; neither does a campaign continued at its level (resumed)
    await newGame("goto survival");
    note("survival", att().phase === "idle", "a Survival game (mode " + snap().mode + "): " + att().phase);
    await lose();
    await pressUntil(385, 320, "gameover");
    await newGame("level 3");
    note("resumed", att().phase === "idle" && (snap().flags & 0x02) !== 0, "a continued campaign at level 3 (flags " + snap().flags.toString(16) + "): " + att().phase);
  },
  dynomite: async function () {
    // a fossil found: its three fragments, each a round the `win` ends (the fossil falls), until the block counts it
    async function fossilFound() {
      var before = snap().boardLevelClears;
      for (var n = 0; n < 6 && snap().boardLevelClears === before; n++) {
        var p0 = snap().progress;
        await until("win");
        for (var k = 0; k < 400 && snap().progress === p0; k++) await botStep();
      }
      await settle();
      return att().index;
    }
    // (the `goto`s start each mode from the main menu with the difficulty last chosen, the game's Normal: the board's Easy)
    titleGoto = true;
    snap();
    await t.wait(5);
    var s0 = snap();
    note("attach", !!s0 && s0.active === 1, s0 ? "active " + s0.active : "no block read");
    // Endless Puzzle 5K: the difficulty's click starts it, the score on screen splits at 1K and ends at 5K
    await load("Endless Puzzle - 5K - Easy");
    await newGame("goto endless");
    var a0 = att(), sk = snap().skill;
    var idx = [];
    for (var sc of [1000, 5000]) {
      await until("set score " + sc);
      for (var k = 0; k < 400 && att().index === idx.length && att().phase === "running"; k++) await botStep();
      await settle();
      idx.push(att().index);
    }
    note("endless", a0.phase === "running" && a0.index === 0 && idx.join(",") === "1,2" && att().phase === "ended",
         "started " + a0.phase + " (skill " + sk + "); index after 1,000 and 5,000 on screen " + idx.join(",") + "; then " + att().phase);
    // the eggs off the bottom reset it (`lose` once the first shot is in)
    await resetByHand();
    await load("Endless Puzzle - 5K - Easy");
    await newGame("goto endless");
    var resetCause = null;
    var p = (async function () { resetCause = await sawReset(30000); })();
    await lose();
    await p;
    var s = snap();
    note("lost", resetCause !== null && att().phase === "idle", "reset " + resetCause + ", then " + att().phase + "; deaths " + [s.death0, s.death1, s.death2, s.death3].join(","));
    // Time Trial: the first egg starts it (GO after the shot, not the click), the board cleared ends it
    await resetByHand();
    await load("Time Trial - Easy");
    await newGame("goto timetrial");
    a0 = att();
    var go = snap().goTick;
    await until("win");
    for (var j = 0; j < 400 && att().phase === "running"; j++) await botStep();
    await settle();
    note("timetrial", a0.phase === "running" && go >= 1 && att().phase === "ended", "started " + a0.phase + " at tick " + go + "; after the board cleared " + att().phase);
    // Fossil Challenge: Let's Play! starts it, each fossil found splits; the win at the fifteenth ends it, after a jump to it
    // (`level`) too
    await resetByHand();
    await load("Fossil Challenge - Strategic");
    await newGame("goto fossil");
    a0 = att();
    var fi = [];
    for (var f = 0; f < 2; f++) fi.push(await fossilFound());
    await until("level 15");
    fi.push(await fossilFound());
    note("fossil", a0.phase === "running" && fi.join(",").indexOf("1,2,") === 0 && att().phase === "ended",
         "started " + a0.phase + "; index after fossils 1, 2 and 15 " + fi.join(",") + "; then " + att().phase + "; wins " + snap().wins);
    // Tutorial: Next starts it, each mode's part splits, the main menu with every part done ends it (the bot plays it)
    await resetByHand();
    await load("Tutorial");
    var g = snap().goCount;
    await until("goto tutorial");
    for (var u = 0; u < 400 && att().phase !== "running"; u++) await botStep();
    a0 = att();
    var ti = [];
    for (var v = 0; v < 4000 && att().phase === "running"; v++) {
      await botStep();
      if (ti[ti.length - 1] !== att().index) ti.push(att().index);
    }
    await settle();
    note("tutorial", a0.phase === "running" && att().phase === "ended",
         "started " + a0.phase + " (GO " + (snap().goCount - g) + "); indexes " + ti.join(",") + "; then " + att().phase + "; parts " + snap().progress);
  },
  cosmicbugs: async function () {
    // a run starts at Play on the instructions page, which `goto instructions` opens (its Play, at 400,563, is clicked here:
    // the bot answers null on that page); the first board
    // splits, the next two don't (the next split is board 49's), and running out of lives resets. The 577 boards to the
    // win are the replay test's (tools/autosplittest.js)
    titleGoto = true;
    snap();
    await t.wait(5);
    var s0 = snap();
    note("attach", !!s0 && s0.active === 1, s0 ? "active " + s0.active : "no block read");
    await load("Any%");
    await newGame(CB_PLAY);
    var a0 = att();
    note("start", a0.phase === "running" && a0.index === 0, "phase " + a0.phase + ", index " + a0.index + " (board " + snap().boardSerial + ")");
    var idx = [];
    for (var lv = 1; lv <= 3; lv++) idx.push((await clearLevel()).index);
    note("split", idx.join(",") === "1,1,1", "index after boards 1 to 3: " + idx.join(","));
    var resetCause = null;
    var p = (async function () { resetCause = await sawReset(30000); })();
    await lose();
    await p;
    note("lost", resetCause !== null && att().phase === "idle", "reset " + resetCause + ", then " + att().phase + "; deaths " + [snap().death0, snap().death1].join(","));
    // a board started by the `level` command (as a continued game or a chosen board) starts nothing
    var g = snap().goCount;
    await until("level 5");
    for (var k = 0; k < 100 && !snap().inGame; k++) await botStep();
    await settle();
    note("resumed", att().phase === "idle" && snap().goCount === g && (snap().flags & 0x02), "after `level 5`: " + att().phase + ", GOs " + g + " to " + snap().goCount + ", flags " + snap().flags.toString(16));
  },
  typershark: async function () {
    splitByProgress = true;
    titleGoto = true;
    await common("Adventure Mode - Easy", 11, "goto adventure 0");
    // a game of another difficulty, or the Abyss, never starts the Easy run
    await resetByHand();
    await newGame("goto adventure 4");
    note("skill", att().phase === "idle", "an X-Treme! game (skill " + snap().skill + "): " + att().phase);
    // Abyss Mode: from the dive's GO to its game over
    await load("Abyss Mode");
    await newGame("goto abyss");
    var a0 = att();
    await lose();
    note("abyss", a0.phase === "running" && att().phase === "ended", "started " + a0.phase + " (mode " + snap().mode + "); after the game over " + att().phase);
  },
  astropop: async function () {
    await cmd("unlock all");
    titleGoto = true;
    unpauseFirst = true;
    await common("Any% - Vector", 32, "goto classic 1 0");
    // No SUPA Weapon: a charged weapon, which the bot starts, resets the run
    await resetByHand();
    await load("Any% (No SUPA Weapon) - Vector");
    await newGame("goto classic 1 0");
    var a0 = att(), resetCause = null;
    var p = (async function () { resetCause = await sawReset(60000); })();
    // (the bot closes the help, starts the charged weapon with its key and answers the box that explains it)
    await cmd("set charge");
    for (var k = 0; k < 200 && !snap().own0; k++) await botStep();
    await settle();
    await p;
    note("supa", a0.phase === "running" && snap().own0 > 0 && resetCause !== null, "started " + a0.phase + "; SUPA uses " + snap().own0 + "; reset " + resetCause);
    // 100%: four pilots, a game each; each game's level 32 (set to it, then won) splits, the next pilot's new game goes on
    // with the run, and a pilot already played doesn't
    await resetByHand();
    await load("100%");
    var idx = [], phases = [], first = null;
    for (var pilot of [0, 3, 0, 1, 2]) {
      await newGame("goto classic 1 " + pilot);
      if (first === null) first = att().phase;
      await cmd("set level 32");
      idx.push((await clearLevel()).index);
      phases.push(att().phase);
    }
    note("allpilots", first === "running" && idx.join(",") === "1,2,2,3,4" && phases[4] === "ended",
         "started " + first + "; index after each pilot's level 32 (Vector, Vixx, Vector again, Sprocket, Turbot) " + idx.join(",") + "; then " + phases.join(","));
  },
  alchemy: async function () {
    // (the bot takes Alchemy's title to the selector but starts no game there, so the plan passes the title first and
    // the `goto` starts each game)
    for (var j = 0; j < 400 && (await t.state()).screen === "title"; j++) await botStep();
    await common("7 Boards - Strategic", 7, "goto strategic 2");
    // a Time Trial game never starts a Strategic run
    await resetByHand();
    await newGame("goto timetrial 1");
    note("mode", att().phase === "idle", "a Time Trial game (mode " + snap().mode + "): " + att().phase);
    // Tutorial%: the bot plays the tutorial, which ends the run at its last step
    await load("Tutorial%");
    await newGame("goto tutorial");
    var a0 = att();
    for (var k = 0; k < 2000 && att().phase === "running"; k++) await botStep();
    await settle();
    note("tutorial", a0.phase === "running" && att().phase === "ended", "started " + a0.phase + " (mode " + snap().mode + "); then " + att().phase + ", wins " + snap().wins);
  },
  bookworm: async function () {
    await cmd("set mode classic");
    await common("Bookworm Supreme - Any%", 31, "goto classic");
    // Bookburner - Death%: a game over at rank 0 ends it, one at rank 1 resets it
    await load("Bookburner - Death%");
    await newGame("goto classic");
    var b0 = att();
    await lose();
    var a = att();
    note("death", b0.phase === "running" && a.phase === "ended", "started " + b0.phase + "; after the game over at rank " + snap().progress + ": " + a.phase);
    await resetByHand();
    await newGame("goto classic");
    b0 = att();
    await clearLevel();
    var resetCause = null;
    var p = (async function () { resetCause = await sawReset(30000); })();
    await lose();
    await p;
    note("rank1", b0.phase === "running" && resetCause !== null && att().phase === "idle", "started " + b0.phase + "; reset " + resetCause + " at rank " + snap().progress + ", then " + att().phase);
  },
  mummymaze: async function () {
    await cmd("set botmode classic");
    await common("1st Pyramid", 15, MM_QUIT, { finish: true, noGameOver: true });
    // 100% Run: a classic game never starts it; an adventure does, and its first treasure splits
    await resetByHand();
    await load("100% Run");
    await newGame(MM_QUIT);
    note("mode", att().phase === "idle", "a Classic game (mode " + snap().mode + "): " + att().phase);
    await cmd("set botmode adventure");
    await newGame(MM_QUIT);
    var a0 = att(), idx = [];
    for (var lv = 1; lv <= 15; lv++) idx.push((await clearLevel()).index);
    for (var k = 0; k < 200 && snap().progress < 1; k++) await botStep();
    await settle();
    note("treasure", a0.phase === "running" && idx.every(function (x) { return x === 0; }) && att().index === 1,
         "started " + a0.phase + "; index after each chamber " + idx.join(",") + "; after the first treasure " + att().index + " (progress " + snap().progress + ")");
  },
  rocketmania: async function () {
    await common("Level 10 - Easy", 10, "goto classic 1");
    // a game of another difficulty, or a Strategy game, never starts the run
    await resetByHand();
    await newGame("goto classic 3");
    note("skill", att().phase === "idle", "a Hard game (skill " + snap().skill + "): " + att().phase);
    await newGame("goto strategy 1");
    note("mode", att().phase === "idle", "a Strategy game (mode " + snap().mode + "): " + att().phase);
  },
  hammerheads: async function () {
    // (Hammer Heads' bot, `move`, only smashes gnomes: the level card's Go!, the "Ready?" box, the bonus round's box and the game
    // over box are pressed by its `botclick` before each move (sent straight to the game: it answers only when it clicks, so
    // the test kit's command would wait out its timeout). The trophy pages and the
    // trophy room's Back are pressed here: the page's Click to Continue and the Back both cover (400, 570),
    // games/hammerheads/src/TrophyRoomWidget.cpp. The room over a game is paused with the level in play and no dialog; the
    // shop is too, and botclick works it)
    stepFirst = async function () { F.Module.ccall("byopop_test_command", "number", ["string"], ["botclick"]); await t.wait(1); };
    async function state() { return JSON.parse(String(await cmd("state")).slice(6)); }
    async function press(x, y) { await t.move(x, y); await t.wait(3); await t.down(x, y); await t.wait(3); await t.up(x, y); await t.wait(3); }
    async function roomStep() {
      var st = await state();
      if (st.paused && st.inplay && !st.dialogs.length) { await t.wait(60); await press(400, 570); }
      else await botStep();
      return st;
    }
    // the level in play, past its card or "Ready?" box and any trophy room (Stats::Set looks for trophies at once, and the
    // game every 100 updates)
    async function inPlay() {
      for (var k = 0; k < 600; k++) { var st = await roomStep(); if (st.inplay && !st.paused) return; }
      throw new Error("no level in play");
    }
    snap();
    await t.wait(5);
    var s0 = snap();
    note("attach", !!s0 && s0.active === 1, s0 ? "active " + s0.active : "no block read");
    // (no damage, so that the bot's misses and the gnomes it lets go cost nothing; Tough Cookie opened)
    await cmd("cheat nomissdamage");
    await cmd("cheat noawaydamage");
    await cmd("unlock all");
    // Classic%, cut to Level 1 and Level 25: the level card's Go! starts it, level 1's end splits, then King Globus's level
    // with his death set: the trophy room that awards his trophy, and the "Ready?" box's Go! after it ends the run
    await load("Classic%", ["Level 1", "Level 25"]);
    await newGame("goto classic");
    var a0 = att();
    note("start", a0.phase === "running" && a0.index === 0, "phase " + a0.phase + ", index " + a0.index + ", board " + snap().boardSerial);
    var a1 = await clearLevel();
    await cmd("set level 25");
    await inPlay();
    await cmd("set kings 1");
    for (var k = 0; k < 600 && att().phase === "running"; k++) await roomStep();
    await settle();
    var s1 = snap(), a2 = att();
    note("split", a1.phase === "running" && a1.index === 1 && a2.phase === "ended" && s1.progress === 1 && s1.wins === 0,
         "after level 1: " + a1.phase + " at " + a1.index + "; after the king's trophy room: " + a2.phase + ", progress " + s1.progress + ", wins " + s1.wins);
    await resetByHand();
    // a new game resets, and starts the next run
    await load("Classic%");
    await newGame("goto classic");
    await clearLevel();
    var b1 = att(), resetCause = null;
    var p = (async function () { resetCause = await sawReset(30000); })();
    var serial2 = await newGame("goto classic");
    await p;
    await settle();
    var b2 = att();
    note("replaced", b1.phase === "running" && b1.index === 1 && resetCause !== null && b2.phase === "running" && b2.index === 0,
         "before: " + b1.phase + " at " + b1.index + "; reset " + resetCause + "; after: " + b2.phase + " at " + b2.index + " (board " + serial2 + ")");
    // out of health resets (its check runs while the level is in play)
    await clearLevel();
    await inPlay();
    resetCause = null;
    p = (async function () { resetCause = await sawReset(30000); })();
    await lose();
    await p;
    var s2 = snap();
    note("lost", resetCause !== null && att().phase === "idle", "reset " + resetCause + ", then " + att().phase + "; deaths " + [s2.death0, s2.death1, s2.death2, s2.death3].join(","));
    // Tough Cookie% never starts on a Classic game, and starts on its own
    await load("Tough Cookie%");
    await newGame("goto classic");
    note("mode", att().phase === "idle", "a Classic game (mode " + snap().mode + "): " + att().phase);
    await newGame("goto tough");
    var c1 = att();
    note("tough", c1.phase === "running" && c1.index === 0, "a Tough Cookie game (mode " + snap().mode + "): " + c1.phase + " at " + c1.index);
  },
  sevenseas: async function () {
    await common("Level 30 (Easy)", 30, "goto easy");
    // a game of another difficulty never starts the Easy run
    await resetByHand();
    await newGame("goto hard");
    var s = snap();
    note("skill", att().phase === "idle", "a Hard game (skill " + s.skill + "): " + att().phase);
  }
};

try {
  if (!PLANS[GAME]) throw new Error("no plan for " + GAME);
  await PLANS[GAME]();
} catch (e) {
  note("error", false, String(e && e.stack || e));
}

// ---- put things back ----
await resetByHand();
await new Promise(function (res) {
  var o = indexedDB.open("byopop-splits", 1);
  o.onsuccess = function () {
    var tx = o.result.transaction("runs", "readwrite"), st = tx.objectStore("runs");
    G.templates.forEach(function (tp) { st.delete(G.name + "\u0000" + name(tp)); });
    tx.oncomplete = tx.onerror = function () { o.result.close(); res(); };
  };
  o.onerror = function () { res(); };
});
try { if (lastKey) localStorage.setItem("byopop-splits-last", lastKey); else localStorage.removeItem("byopop-splits-last"); } catch (e) {}
timer.load(lastRun);
ByopopAutosplit.sample = sample;
clearInterval(tracing);
var fail = results.filter(function (x) { return x.result === "FAIL"; }).length;
return { pass: results.length - fail, fail: fail, results: results, updates: t.updates(), errors: t.errors.filter(function (e) { return !/^ERROR /.test(e.text); }).slice(0, 5), trace: fail ? trace.slice(-300) : [] };
