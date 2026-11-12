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
// ("skill", "mode").
// Every reset's question (keep the new bests?) is answered No; the stored splits of the categories it ran are removed at the
// end, and the last run the window had open is put back.
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
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
async function botStep() {
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
async function load(category) {
  step = category;
  var tp = G.templates.filter(function (x) { return name(x) === category; })[0];
  timer.load({ game: G.name, category: category, offset: 0, attempts: 0, segments: tp.segments.map(function (n) { return { name: n, pb: { rt: null, gt: null }, best: { rt: null, gt: null } }; }) });
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
  // (AstroPop takes a `win` in any state, and one under its help or during the warp flight counts a clear that never goes
  // on to the next level: with unpauseFirst the bot plays until the board is in play, state 0, before it)
  for (var u = 0; unpauseFirst && u < 2000 && (snap().state !== 0 || (snap().flags & 0x80)); u++) await botStep();
  await until("win");
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

// Mummy Maze's Quit to Main button, in the game's sidebar
var MM_QUIT = [{ click: [64, 462] }];

var PLANS = {
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
