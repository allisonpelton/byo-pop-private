// The Splits window's auto splitter on a live game (docs/WEB_PLAYER.md, "Splits"): the front page opens the game in a desktop
// window, with the player-only page's test kit in it, and a driver plays runs of the game's speedrun.com categories through
// it while the Splits window, as a player has it, starts, splits and resets them. Runs in the front page, like deskcheck.js:
//   http://127.0.0.1:18780/index.html?silent=1&driver=tools/web/autosplitcheck.js
// The game's moves come from its own bot (the `move` test command) and the standard `win` and `lose` commands
// (docs/TEST_COMMANDS.md), all through the test kit's real DOM input; the window's frame is the desktop's own, with
// player=1&assets=1&fast=1 added, so that the game runs at the machine's speed. The result, window.byopopDeskCheck.result:
// {pass, fail, results: [{check, result, detail}]}. The checks (BookWorm's, the only game with a row here yet):
//   attach     the block is read from the window (layout 2, its id) and goes active once asked
//   start      a new game starts the Any% run
//   split      each level cleared splits it, the 31st (rank 31) ends it
//   replaced   a new game during a run resets it, and the new game starts the next
//   fire       a game over during an Any% run resets it
//   death      Bookburner: a game over at rank 0 ends the run
//   rank1      Bookburner: a game over after a level cleared resets it instead
// Every reset's question (keep the new bests?) is answered No; the stored splits of the categories it ran are removed at the
// end, and the last run the window had open is put back.
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
// (the game's page id; the page rewrites its address before the driver runs, so it is not a parameter)
var GAME = "bookworm";
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
// a new game of the mode the selector's bot picks: the bot's moves until a new board is in play
async function newGame() {
  step += "+new";
  var before = (snap() || {}).boardSerial;
  if (t.progress && (await t.state()).screen !== "title") await until("goto classic", 50);
  for (var k = 0; k < 400; k++) {
    var s = snap();
    if (s && s.boardSerial && s.boardSerial !== before && s.inGame) { await settle(); return s.boardSerial; }
    await botStep();
  }
  throw new Error("no new game");
}
function load(category) {
  step = category;
  var tp = G.templates.filter(function (x) { return x.category + " - " + x.sub === category; })[0];
  timer.load({ game: G.name, category: category, offset: 0, attempts: 0, segments: tp.segments.map(function (n) { return { name: n, pb: { rt: null, gt: null }, best: { rt: null, gt: null } }; }) });
}
// clears the level in play, once the bot has reached one: the timer's index after the auto splitter saw it
async function clearLevel() { await until("win"); await settle(); return att(); }
async function lose() { await until("lose"); for (var k = 0; k < 20 && snap().gameOvers === 0; k++) await t.wait(5); await settle(); }
// the timer back to idle, its question answered No
async function resetByHand() { if (att().phase !== "idle") { timer.reset(); await wait(50); answerNo(); await wait(50); } }
// waits up to ms for a reset's question (answered No) or an idle timer
async function sawReset(ms) {
  for (var k = 0; k < ms / 20; k++) { if (answerNo()) { await wait(50); return "asked"; } if (att().phase === "idle") return "idle"; await wait(20); }
  return null;
}

try {
  await cmd("set mode classic");
  // ---- attach ----
  snap();
  await t.wait(5);
  var s0 = snap();
  note("attach", !!s0 && s0.active === 1, s0 ? "active " + s0.active : "no block read");

  // ---- Bookworm Supreme - Any% ----
  load("Bookworm Supreme - Any%");
  var serial = await newGame();
  var a = att();
  note("start", a.phase === "running" && a.index === 0, "phase " + a.phase + ", index " + a.index + ", board " + serial);
  var idx = [];
  for (var lv = 1; lv <= 31; lv++) idx.push((await clearLevel()).index);
  a = att();
  var wantIdx = idx.slice(0, 30).every(function (x, k) { return x === k + 1; });
  note("split", wantIdx && a.phase === "ended", "indexes after each clear " + idx.join(",") + "; then " + a.phase + ", progress " + snap().progress);
  await resetByHand();

  // ---- a new game resets ----
  serial = await newGame();
  await clearLevel();
  var a1 = att();
  var s1 = snap().boardSerial;
  var resetCause = null, started = false;
  // the new game: the reset comes when its board replaces the run's
  var p = (async function () { resetCause = await sawReset(30000); })();
  var serial2 = await newGame();
  await p;
  await settle();
  a = att();
  note("replaced", a1.phase === "running" && a1.index === 1 && resetCause !== null && a.phase === "running" && a.index === 0,
       "before: " + a1.phase + " at " + a1.index + " (board " + s1 + "); reset " + resetCause + "; after: " + a.phase + " at " + a.index + " (board " + serial2 + ")");

  // ---- a game over resets Any% ----
  await clearLevel();
  p = (async function () { resetCause = await sawReset(30000); })();
  await lose();
  await p;
  note("fire", resetCause !== null && att().phase === "idle", "reset " + resetCause + ", then " + att().phase + "; death0 " + snap().death0);

  // ---- Bookburner - Death% ----
  load("Bookburner - Death%");
  await newGame();
  var b0 = att();
  await lose();
  a = att();
  note("death", b0.phase === "running" && a.phase === "ended", "started " + b0.phase + "; after the game over at rank " + snap().progress + ": " + a.phase);
  await resetByHand();
  await newGame();
  b0 = att();
  await clearLevel();
  resetCause = null;
  p = (async function () { resetCause = await sawReset(30000); })();
  await lose();
  await p;
  note("rank1", b0.phase === "running" && resetCause !== null && att().phase === "idle", "started " + b0.phase + "; reset " + resetCause + " at rank " + snap().progress + ", then " + att().phase);
} catch (e) {
  note("error", false, String(e && e.stack || e));
}

// ---- put things back ----
await resetByHand();
await new Promise(function (res) {
  var o = indexedDB.open("byopop-splits", 1);
  o.onsuccess = function () {
    var tx = o.result.transaction("runs", "readwrite"), st = tx.objectStore("runs");
    G.templates.forEach(function (tp) { st.delete(G.name + "\u0000" + tp.category + " - " + tp.sub); });
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
