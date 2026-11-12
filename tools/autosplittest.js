#!/usr/bin/env node
// The speedrun timer's auto splitters (web/autosplit.js) against their references. Bejeweled: every scenario of
// games/bejeweled/tools/livesplit/lsdecide.py (27, each on the game's log and on lsprobe's real polling log: 50 replays),
// from tools/autosplit/bejeweled-scenarios.json.gz (tools/autosplit/bejeweled_fixture.py writes it from run_tests.sh's runs, with
// the events lsdecide.py decides), through the page's decider with a timer that behaves as LiveSplit's does in lsdecide.py:
// every event, its poll and its board must be the same. Also the block reader on a block laid out as LiveSplitBlock.h says.
//   node tools/autosplittest.js
var path = require("path"), fs = require("fs");
var A = require(path.join(__dirname, "..", "web", "autosplit.js"));
var fails = 0, checks = 0;
function check(ok, what) { checks++; if (!ok) { fails++; console.log("FAIL " + what); } }

function load(name) { return JSON.parse(require("zlib").gunzipSync(fs.readFileSync(path.join(__dirname, "autosplit", name))).toString("utf8")); }
var fx = load("bejeweled-scenarios.json.gz");
function expand(packed) {
  var polls = [];
  packed.forEach(function (p) {
    if (p === null) { polls.push([0, null]); return; }
    if (!Array.isArray(p)) { polls.push([p.t, p.f]); return; }
    for (var i = 0; i < p[0]; i++) polls.push([0, p[1]]);
  });
  return polls.map(function (x) {
    if (!x[1]) return [x[0], null];
    var snap = {};
    fx.fields.forEach(function (k, i) { snap[k] = x[1][i]; });
    return [x[0], snap];
  });
}
var logs = {};
Object.keys(fx.logs).forEach(function (k) { logs[k] = expand(fx.logs[k]); });

function replay(fx, logs, makeDecider, who) {
 fx.cases.forEach(function (c) {
  var polls = logs[c.log], events = [], i = 0;
  var t = { ph: "NotRunning", idx: -1, runBoard: 0, segs: c.segments.length };
  var d;
  var timer = {
    phase: function () { return t.ph; }, index: function () { return t.idx; },
    start: function (label) {
      if (t.ph !== "NotRunning") return;
      t.ph = "Running"; t.idx = 0;
      d.onStart(); t.runBoard = d.board();
      events.push((label || "start") + " " + i + " board=" + d.board());
    },
    split: function () {
      t.idx++;
      events.push("split " + (t.idx - 1) + " " + i + " board=" + d.board());
      if (t.idx >= t.segs) t.ph = "Ended";
    },
    reset: function (cause) {
      if (t.ph === "NotRunning") return;
      t.ph = "NotRunning"; t.idx = -1;
      events.push(cause === "exit" ? "reset exit " + polls.length + " board=" + t.runBoard : "reset " + cause + " " + i + " board=" + d.board());
      d.onReset();
    }
  };
  d = makeDecider(timer, { category: c.category, segments: c.segments, settings: c.settings });
  // the polls before the attach are not seen; the message box's 10 s count from the first poll seen
  for (i = c.attach; i < polls.length; i++) {
    if (d.poll(polls[i][0], polls[i][1])) events.push("messagebox " + i);
    if (c.manual !== null && i === c.manual) timer.start("manualstart");
  }
  d.exit();
  check(JSON.stringify(events) === JSON.stringify(c.events), who + " " + c.name + " (" + c.log + "): page [" + events.join("; ") + "], lsdecide [" + c.events.join("; ") + "]");
 });
}
replay(fx, logs, A.bejeweled, "bejeweled");

// Big Money, on the shared decider (its row of the rules table): games/bigmoney/tools/livesplit/lsdecide.py's scenarios
var bm = load("bigmoney-scenarios.json.gz"), bmLogs = {}, fxSave = fx;
fx = bm;
Object.keys(bm.logs).forEach(function (k) { bmLogs[k] = expand(bm.logs[k]); });
fx = fxSave;
replay(bm, bmLogs, A.games.bigmoney.decider, "bigmoney");

// BookWorm, on the same decider (its row): games/bookworm/tools/livesplit/lsdecide.py's scenarios
var bw = load("bookworm-scenarios.json.gz"), bwLogs = {};
fx = bw;
Object.keys(bw.logs).forEach(function (k) { bwLogs[k] = expand(bw.logs[k]); });
fx = fxSave;
replay(bw, bwLogs, A.games.bookworm.decider, "bookworm");


// the block reader: a block as the game lays it out, then one the game is writing, one of another game
var heap = new Uint8Array(4096), at = 256, dv = new DataView(heap.buffer);
[0x42, 0x59, 0x4F, 0x50, 0x4F, 0x50, 0x2D, 0x4C, 0x49, 0x56, 0x45, 0x53, 0x50, 0x4C, 0x49, 0x54, 0xF0, 0xF1, 0xF2, 0xF3].forEach(function (b, i) { heap[at + i] = b; });
dv.setUint32(at + 0x14, 0x58444A42, true); dv.setUint16(at + 0x18, 1, true); dv.setUint16(at + 0x1A, 0xB0, true);
dv.setUint32(at + 0x20, 6, true); dv.setUint32(at + 0x24, 1, true); dv.setUint32(at + 0x30, 3, true); dv.setUint32(at + 0x74, 2, true);
dv.setInt32(at + 0x78, -1, true); dv.setInt32(at + 0x7C, 12345, true);
var b = A.readBlock(heap, at);
check(b && b.active === 1 && b.boardSerial === 3 && b.boardLevelClears === 2 && b.goTick === -1 && b.displayScoreMax === 12345, "readBlock: the fields at their offsets");
dv.setUint32(at + 0x20, 7, true);
check(A.readBlock(heap, at) === null, "readBlock: a block being written (seq odd) is not read");
dv.setUint32(at + 0x20, 6, true); heap[at + 19] = 0;
check(A.readBlock(heap, at) === null, "readBlock: another magic is not read");
check(A.readBlock(heap, 4000) === null && A.readBlock(heap, 0) === null, "readBlock: out of the heap, or no address");
check(JSON.stringify(A.parseSplits(["Level 1", "10000", "25,000", "50K", "-{Act} Level 2", "999"])) ===
      JSON.stringify([{ score: false, target: 1 }, { score: true, target: 10000 }, { score: true, target: 25000 }, { score: true, target: 50000 },
                      { score: false, target: 2 }, { score: false, target: 3 }]), "parseSplits: lsdecide.py's reading of segment names");
check(A.games.bejeweled.templates.length === 12, "Bejeweled's 12 split templates");
check(A.games.bookworm.templates.map(function (t) { return t.category + " - " + t.sub + " " + t.end; }).join("|") ===
      "Bookworm Supreme - Any% progress 31|Bookburner - Death% lost 0", "BookWorm's two speedrun.com categories");

// the layout-2 reader: Big Money's block as platform/AutosplitBlock.h lays it out; a layout-1 block is not read
var rd = A.games.bigmoney.read;
heap.fill(0);
[0x42, 0x59, 0x4F, 0x50, 0x4F, 0x50, 0x2D, 0x4C, 0x49, 0x56, 0x45, 0x53, 0x50, 0x4C, 0x49, 0x54, 0xF0, 0xF1, 0xF2, 0xF3].forEach(function (b, i) { heap[at + i] = b; });
dv.setUint32(at + 0x14, 0x58444D42, true); dv.setUint16(at + 0x18, 2, true); dv.setUint16(at + 0x1A, 0xD0, true);
dv.setUint32(at + 0x20, 8, true); dv.setUint32(at + 0x24, 1, true); dv.setUint32(at + 0x50, 4, true); dv.setUint32(at + 0xB0, 1, true);
dv.setInt32(at + 0xB8, 2, true); dv.setInt32(at + 0xBC, 3, true); dv.setUint32(at + 0xC0, 1, true); dv.setInt32(at + 0xC4, -1, true);
b = rd(heap, at);
check(b && b.active === 1 && b.death1 === 4 && b.wins === 1 && b.mode === 2 && b.skill === 3 && b.inGame === 1 && b.progress === -1,
      "blockReader: layout 2's fields at their offsets");
dv.setUint16(at + 0x18, 1, true); dv.setUint16(at + 0x1A, 0xB0, true);
check(rd(heap, at) === null, "blockReader: a layout-1 block is not read");
dv.setUint16(at + 0x18, 2, true); dv.setUint16(at + 0x1A, 0xD0, true); dv.setUint32(at + 0x14, 0x58444A42, true);
check(rd(heap, at) === null, "blockReader: another game's block is not read");

// the template's end where the segments can't say it: won, lost, progress
function endRun(end, polls, goFlags) {
  var row = { name: "T", id: "TEST", modes: {}, skills: {}, deaths: [{ setting: "resetA" }, { setting: null }, { setting: null }, { setting: null }],
              templates: [{ category: "Cat", segments: ["Level 1", "End"], end: end }] };
  var ev = [], t = { ph: "NotRunning", idx: -1 }, d;
  var timer = { phase: function () { return t.ph; }, index: function () { return t.idx; },
    start: function () { t.ph = "Running"; t.idx = 0; d.onStart(); ev.push("start"); },
    split: function () { t.idx++; ev.push("split"); if (t.idx >= 2) t.ph = "Ended"; },
    reset: function (c) { t.ph = "NotRunning"; t.idx = -1; ev.push("reset " + c); d.onReset(); } };
  d = A.makeDecider(row)(timer, { category: "Cat", segments: ["Level 1", "End"] });
  var base = { active: 1, flags: 0, boardSerial: 1, goCount: 0, goSerial: 0, gameOvers: 0, gameOverSerial: 0, death0: 0, boardLevelClears: 0,
               displayScoreMax: 0, wins: 0, winSerial: 0, progress: -1, mode: 0, skill: 0 };
  [{}, { goCount: 1, goSerial: 1, flags: goFlags || 0 }].concat(polls).reduce(function (s, p) {
    var n = {}; Object.keys(s).forEach(function (k) { n[k] = s[k]; }); Object.keys(p).forEach(function (k) { n[k] = p[k]; });
    d.poll(0, n); return n;
  }, base);
  return ev.join("; ");
}
check(endRun("won", [{ boardLevelClears: 1 }, { boardLevelClears: 2 }, { wins: 1, winSerial: 1 }]) === "start; split; split", "end won: the last split at the win");
check(endRun("lost", [{ boardLevelClears: 1 }, { gameOvers: 1, gameOverSerial: 1, death0: 1 }]) === "start; split; split", "end lost: the game over ends the run");
check(endRun("levels 2", [{ boardLevelClears: 1 }, { gameOvers: 1, gameOverSerial: 1, death0: 1 }]) === "start; split; reset resetA", "a game over resets otherwise");
check(endRun("lost 0", [{ boardLevelClears: 1, progress: 0 }, { gameOvers: 1, gameOverSerial: 1, death0: 1, progress: 0 }]) === "start; split; split",
      "end lost 0: a game over at progress 0 ends the run");
check(endRun("lost", [{ gameOvers: 1, gameOverSerial: 1, death0: 1 }]) === "start; reset resetA", "end lost: a game over before the last segment resets");
check(endRun("lost 0", [{ boardLevelClears: 1, progress: 1 }, { gameOvers: 1, gameOverSerial: 1, death0: 1, progress: 1 }]) === "start; split; reset resetA",
      "end lost 0: a later game over resets");
check(endRun("progress 5", [{ boardLevelClears: 1 }, { progress: 4 }, { progress: 5 }]) === "start; split; split", "end progress: the figure reaches N");
check(endRun("won", [{ boardLevelClears: 1 }], 0x02) === "", "no start on a resumed board");

// A game's -livesplitlog event lines ("board N GO ... (difficulty D)", "board N level clear K ...", "board N game over (kind)
// ...", "board N win ..."), replayed as the block they describe, one poll an event, through the game's decider with the
// template named category: the timer's calls. A line "board N GO" after a game over on board N is a new game (the log's
// serials restart in each run of the game). progress follows the level cleared, as the games that keep one do.
function eventReplay(game, category, lines, deathOf) {
  var g = A.games[game], tp = g.templates.filter(function (x) { return (x.sub ? x.category + " - " + x.sub : x.category) === category; })[0];
  var ev = [], t = { ph: "NotRunning", idx: -1 }, d;
  var timer = { phase: function () { return t.ph; }, index: function () { return t.idx; },
    start: function () { t.ph = "Running"; t.idx = 0; d.onStart(); ev.push("start"); },
    split: function () { t.idx++; ev.push("split"); if (t.idx >= tp.segments.length) t.ph = "Ended"; },
    reset: function (c) { t.ph = "NotRunning"; t.idx = -1; ev.push("reset " + c); d.onReset(); } };
  d = g.decider(timer, { category: category, segments: tp.segments });
  var s = { active: 1, flags: 0, boardSerial: 0, goCount: 0, goSerial: 0, gameOvers: 0, gameOverSerial: 0, death0: 0, death1: 0, death2: 0,
            death3: 0, boardLevelClears: 0, displayScoreMax: 0, wins: 0, winSerial: 0, progress: -1, mode: -1, skill: 0, inGame: 0 };
  var serial = 0;
  function poll() { var n = {}; Object.keys(s).forEach(function (k) { n[k] = s[k]; }); d.poll(0, n); }
  poll();
  lines.forEach(function (line) {
    var m;
    if ((m = /^board \d+ GO .*\((?:difficulty|skill) (\d+)\)/.exec(line))) {
      serial++;
      s.boardSerial = serial; s.goCount++; s.goSerial = serial; s.boardLevelClears = 0; s.progress = 0; s.mode = tp.mode || 0;
      s.skill = +m[1]; s.inGame = 1; s.flags = 0;
    } else if ((m = /^board \d+ level clear (\d+)/.exec(line))) {
      s.boardLevelClears++; s.progress = +m[1];
    } else if ((m = /^board \d+ game over \(([^)]*)\)/.exec(line))) {
      s.gameOvers++; s.gameOverSerial = serial; s["death" + deathOf(m[1])]++; s.inGame = 0; s.flags |= 0x20;
    } else if (/^board \d+ win/.test(line)) {
      s.wins++; s.winSerial = serial; s.inGame = 0;
    } else return;
    poll();
  });
  return ev.join("; ");
}
var ssLines = fs.readFileSync(path.join(__dirname, "autosplit", "sevenseas-events.txt"), "utf8").split(/\r?\n/);
function ssDeath(k) { return k === "out of lives" ? 0 : 3; }
function games(lines, skill) {
  // the runs of one difficulty in the log: each from its GO up to the next GO
  var out = [], cur = null;
  lines.forEach(function (l) { var m = /GO .*\(difficulty (\d+)\)/.exec(l); if (m) { cur = +m[1] === skill ? [] : null; if (cur) out.push(cur); } if (cur) cur.push(l); });
  return [].concat.apply([], out);
}
check(eventReplay("sevenseas", "Level 30 (Easy)", games(ssLines, 1), ssDeath) === "start; split; split; split; split; reset resetLives; start; split; split; split; reset resetLives",
      "Seven Seas Easy: a split at each level, a reset at each loss of every life, a new game starts the next run");
check(eventReplay("sevenseas", "Level 30 (Average)", games(ssLines, 2), ssDeath) === "start; split; split; split; split; split; reset resetLives",
      "Seven Seas Average (from a level jump)");
check(eventReplay("sevenseas", "Level 30 (Easy)", games(ssLines, 3), ssDeath) === "", "Seven Seas: a game of another difficulty never starts the run");
var ss30 = ["board 1 GO at tick 1 (difficulty 3)"];
for (var lv = 1; lv <= 31; lv++) ss30.push("board 1 level clear " + lv + " at tick " + lv * 100 + " (map " + (lv % 10 ? 0 : 1) + ")");
check(eventReplay("sevenseas", "Level 30 (Hard)", ss30, ssDeath) === "start; " + Array(31).join("split; ").replace(/; $/, ""),
      "Seven Seas Hard: 30 splits, the last at progress 30 (the map after the treasure level), none after");
check(A.games.sevenseas.templates.map(function (x) { return x.category + " " + x.skill + " " + x.end; }).join("|") ===
      "Level 30 (Easy) 1 progress 30|Level 30 (Average) 2 progress 30|Level 30 (Hard) 3 progress 30", "Seven Seas' three speedrun.com categories");

// A game's -livesplitlog in the full form ("u=<update> <event> | seq=... board=... go=a/b lc=a/b over=a/b deaths=a,b,c,d
// ... wins=a/b mode=m skill=k ingame=i progress=p", Mummy Maze's): each line the block as it was, polled once, through the
// game's decider with the template named category. Returns the timer's calls.
function blockReplay(game, category, lines) {
  var g = A.games[game], tp = g.templates.filter(function (x) { return (x.sub ? x.category + " - " + x.sub : x.category) === category; })[0];
  var ev = [], t = { ph: "NotRunning", idx: -1 }, d;
  var timer = { phase: function () { return t.ph; }, index: function () { return t.idx; },
    start: function () { t.ph = "Running"; t.idx = 0; d.onStart(); ev.push("start"); },
    split: function () { t.idx++; ev.push("split"); if (t.idx >= tp.segments.length) t.ph = "Ended"; },
    reset: function (c) { t.ph = "NotRunning"; t.idx = -1; ev.push("reset " + c); d.onReset(); } };
  d = g.decider(timer, { category: category, segments: tp.segments });
  var PAIRS = { go: ["goCount", "goSerial"], lc: ["levelClears", "levelClearSerial"], over: ["gameOvers", "gameOverSerial"], wins: ["wins", "winSerial"] };
  var ONE = { board: "boardSerial", blc: "boardLevelClears", dsmax: "displayScoreMax", mode: "mode", skill: "skill", ingame: "inGame",
              progress: "progress", active: "active", tick: "tick", state: "state" };
  lines.forEach(function (line) {
    // (Rocket Mania's lines are the fields alone, without the event and its bar)
    var bar = line.indexOf("|");
    if (bar < 0 && !/^u=\d+ seq=/.test(line)) return;
    var s = {};
    line.slice(bar + 1).trim().split(/\s+/).forEach(function (kv) {
      var eq = kv.indexOf("="), k = kv.slice(0, eq), v = kv.slice(eq + 1);
      if (PAIRS[k]) { var ab = v.split("/"); s[PAIRS[k][0]] = +ab[0]; s[PAIRS[k][1]] = +ab[1]; }
      else if (ONE[k]) s[ONE[k]] = +v;
      else if (k === "flags") s.flags = parseInt(v, 16);
      else if (k === "deaths") v.split(",").forEach(function (n, i) { s["death" + i] = +n; });
      // (the game's own bytes: their first word, little-endian)
      else if (k === "own") s.own0 = parseInt(v.slice(6, 8) + v.slice(4, 6) + v.slice(2, 4) + v.slice(0, 2), 16);
    });
    d.poll(0, s);
  });
  return ev.join("; ");
}
function mmLog(name) { return fs.readFileSync(path.join(__dirname, "autosplit", "mummymaze-" + name + ".log"), "utf8").split(/\r?\n/); }
check(blockReplay("mummymaze", "1st Pyramid", mmLog("c1")) === "start; split; split; split; reset resetAbandon",
      "Mummy Maze 1st Pyramid (from chamber 14): a split at each chamber, Abandon Hope resets");
check(blockReplay("mummymaze", "1st Pyramid", mmLog("c2")) === "start; split; split", "Mummy Maze: the last pyramid's win is no reset");
check(blockReplay("mummymaze", "100% Run", mmLog("c2")) === "", "Mummy Maze 100% Run never starts on a Classic game");
check(blockReplay("mummymaze", "100% Run", mmLog("a1")) === "start; split; split", "Mummy Maze 100% Run: a split at each treasure, none at a chamber");
check(blockReplay("mummymaze", "100% Run", mmLog("a2")) === "" && blockReplay("mummymaze", "1st Pyramid", mmLog("r1")) === "",
      "Mummy Maze: a resumed game never starts a run");
// the ends, on made-up blocks: 1st Pyramid ends at progress 1 after its 15 chambers, 100% at the win after 14 treasures
function mmRun(category, mode, steps) {
  var lines = [], s = { board: 1, go: "1/1", blc: 0, progress: 0, wins: "0/0", over: "0/0", mode: mode, ingame: 1, active: 1, flags: "04" };
  function line() { return "u=0 x | " + Object.keys(s).map(function (k) { return k + "=" + s[k]; }).join(" "); }
  lines.push(line().replace("go=1/1", "go=0/0"));
  lines.push(line());
  steps.forEach(function (p) { Object.keys(p).forEach(function (k) { s[k] = p[k]; }); lines.push(line()); });
  return blockReplay("mummymaze", category, lines);
}
var mmSteps = [];
for (var ch = 1; ch <= 15; ch++) mmSteps.push({ blc: ch });
mmSteps.push({ progress: 1 });
var mmWant = "start; " + Array(17).join("split; ").replace(/; $/, "");
check(mmRun("1st Pyramid", 0, mmSteps) === mmWant, "Mummy Maze 1st Pyramid: 15 chamber splits, then the end at the next pyramid");
var mmAdv = [];
for (var tr = 1; tr <= 14; tr++) mmAdv.push({ blc: tr * 15, progress: tr });
mmAdv.push({ blc: 225, progress: 15, wins: "1/1" });
check(mmRun("100% Run", 1, mmAdv) === "start; " + Array(16).join("split; ").replace(/; $/, ""), "Mummy Maze 100% Run: 14 treasure splits, the last at the win");

// Rocket Mania: its block logs (tools/autosplit/rocketmania/, the port's bot and scripted games; a line at each change)
function rmLog(name) { return fs.readFileSync(path.join(__dirname, "autosplit", "rocketmania", name + ".block.log"), "utf8").split(/\r?\n/); }
function splits(n) { return Array(n + 1).join("split; ").replace(/; $/, ""); }
var rmSkill = function (name) { var m = /skill=(\d+)/.exec(rmLog(name).filter(function (l) { return /ingame=1/.test(l); })[0] || ""); return m ? +m[1] : -1; };
check(rmSkill("script_classic_2") === 1 && rmSkill("bot_classic_1") === 0 && rmSkill("script_arcade_3") === 2, "Rocket Mania's logs: their difficulties as the test expects");
check(blockReplay("rocketmania", "Level 30 - Normal", rmLog("script_classic_2")) === "start; " + splits(30), "Rocket Mania Level 30 (Classic, Normal): 30 splits, the loss after it ignored");
check(blockReplay("rocketmania", "Level 10 - Normal", rmLog("script_classic_2")) === "start; " + splits(10), "Rocket Mania Level 10: ends at the 10th Level Up");
check(blockReplay("rocketmania", "Level 20 - Hard", rmLog("script_arcade_3")) === "start; " + splits(20), "Rocket Mania Level 20 in Arcade (Hard)");
check(blockReplay("rocketmania", "Level 10 - Easy", rmLog("script_classic_2")) === "", "Rocket Mania: a game of another difficulty never starts the run");
check(blockReplay("rocketmania", "Level 10 - Easy", rmLog("bot_classic_1")) === "start; split; split; split; reset resetOutOfTime",
      "Rocket Mania Classic: a split at each Level Up, the night running out resets");
check(blockReplay("rocketmania", "Level 10 - Easy", rmLog("bot_arcade_1")) === "start; split; split; reset resetTopLine", "Rocket Mania Arcade: the top line filling resets");
check(blockReplay("rocketmania", "Level 10 - Normal", rmLog("script_strategy_2")) === "", "Rocket Mania: a Strategy game never starts a Level run");
check(blockReplay("rocketmania", "Level 10 - Easy", rmLog("quit")) === "start; reset resetQuit", "Rocket Mania: leaving the game resets as a quit");
check(blockReplay("rocketmania", "Level 10 - Normal", rmLog("resume2")) === "", "Rocket Mania: a continued save never starts a run");

// Alchemy: its block logs (tools/autosplit/alchemy/, the port's scripted games; a line at each change)
function alLog(name) { return fs.readFileSync(path.join(__dirname, "autosplit", "alchemy", name + ".block.log"), "utf8").split(/\r?\n/); }
check(blockReplay("alchemy", "1 Board - Strategic", alLog("strategic1")) === "start; split", "Alchemy 1 Board: ends at the result screen, the loss after it ignored");
check(blockReplay("alchemy", "7 Boards - Strategic", alLog("strategic7")) === "start; " + splits(7), "Alchemy 7 Boards: a split at each board cleared");
check(blockReplay("alchemy", "7 Boards - Strategic", alLog("strategic1")) === "start; split; reset resetForgeDiscard", "Alchemy Strategic: the forge overflowing resets");
check(blockReplay("alchemy", "1 Board - Strategic", alLog("timetrial_full")) === "", "Alchemy: a Time Trial game never starts a Strategic run");
check(blockReplay("alchemy", "7 Boards - Time Trial", alLog("timetrial_full")) === "start; split; reset resetForgeFull", "Alchemy Time Trial: the forge staying full resets");
check(blockReplay("alchemy", "7 Boards - Time Trial", alLog("timetrial_discard")) === "start; split; reset resetForgeDiscardTT", "Alchemy Time Trial: a discard overflowing resets");
check(blockReplay("alchemy", "1 Board - Time Trial", alLog("timetrial_discard")) === "start; split", "Alchemy 1 Board in Time Trial");
check(blockReplay("alchemy", "Tutorial%", alLog("tutorial")) === "start; split", "Alchemy Tutorial%: starts at its GO, ends at its win");
check(blockReplay("alchemy", "1 Board - Strategic", alLog("tutorial")) === "", "Alchemy: the tutorial never starts a board run");
check(blockReplay("alchemy", "Tutorial%", alLog("strategic1")) === "", "Alchemy: a game never starts Tutorial%");
check(blockReplay("alchemy", "1 Board - Strategic", alLog("left")) === "start; reset resetLeft; start; reset resetForgeDiscard", "Alchemy: leaving a game resets, the next game starts again");
check(blockReplay("alchemy", "1 Board - Strategic", alLog("resume")) === "", "Alchemy: a continued save never starts a run");

// AstroPop: its block logs (tools/autosplit/astropop/, the port's scripted games; a line at each change), and runs of 100%
// made up of its fields (four pilots, a game each)
function apLog(name) { return fs.readFileSync(path.join(__dirname, "autosplit", "astropop", name + ".block.log"), "utf8").split(/\r?\n/); }
check(blockReplay("astropop", "Any% - Sprocket", apLog("classic")) === "start; split; split; reset resets", "AstroPop Any%: a split at each level, the test's game over resets");
check(blockReplay("astropop", "Any% (No SUPA Weapon) - Sprocket", apLog("classic")) === "start; split; reset resetSupa",
      "AstroPop No SUPA Weapon: the first SUPA Weapon use resets");
check(blockReplay("astropop", "Any% (No SUPA Weapon) - Vector", apLog("supa")) === "start; reset resetSupa", "AstroPop No SUPA Weapon: a use before any level resets");
check(blockReplay("astropop", "Any% - Vector", apLog("supa")) === "start; reset resets", "AstroPop Any% allows the SUPA Weapon");
check(blockReplay("astropop", "Any% - Vector", apLog("classic")) === "", "AstroPop: another pilot's game never starts the run");
check(blockReplay("astropop", "Any% - Turbot", apLog("survival")) === "" && blockReplay("astropop", "100%", apLog("survival")) === "",
      "AstroPop: a Survival game never starts a run");
// a made-up game in the log's fields: from its GO (serial n, the nth GO) through progress up to `to`, then `after`: "left"
// (END GAME, kind 2), "lost" (kind 0) or nothing
function apGame(n, pilot, to, after, deathsBefore) {
  var d = deathsBefore || [0, 0, 0, 0], over = d[0] + d[1] + d[2] + d[3], out = [];
  function line(p, flags, dd, ov, ing) {
    out.push("u=" + (n * 1000 + out.length) + " seq=1 active=1 flags=" + flags + " board=" + n + " go=" + n + "/" + n + " over=" + ov + "/" + (ov ? n : 0) +
             " deaths=" + dd.join(",") + " mode=1 skill=" + pilot + " ingame=" + ing + " progress=" + p + " own=0000000001000000");
  }
  for (var p = 0; p <= to; p++) line(p, "04", d, over, 1);
  if (after === "left") { var l = d.slice(); l[2]++; line(to, "44", l, over + 1, 0); d = l; }
  if (after === "lost") { var k = d.slice(); k[0]++; line(to, "44", k, over + 1, 0); d = k; }
  return { lines: out, deaths: d };
}
function apRun(games) {
  // (the first line is the decider's baseline: the title, no game)
  var lines = ["u=0 seq=1 active=1 flags=00 board=0 go=0/0 over=0/0 deaths=0,0,0,0 mode=-1 skill=0 ingame=0 progress=-1"], d = [0, 0, 0, 0];
  games.forEach(function (g, i) {
    var r = apGame(i + 1, g[0], g[1], g[2], d);
    // (the game is polled again at its end: a poll that splits is never the last)
    lines = lines.concat(r.lines, [r.lines[r.lines.length - 1]]); d = r.deaths;
  });
  return blockReplay("astropop", "100%", lines);
}
check(apRun([[0, 32, "left"], [1, 32, "left"], [2, 32]]) === "start; split; split; split",
      "AstroPop 100%: after the third pilot's level 32 the run waits for the fourth (found in the pane: the end split again)");
check(apRun([[0, 32, "left"], [3, 32, "left"], [1, 32, "left"], [2, 32]]) === "start; split; split; split; split",
      "AstroPop 100%: a split at each pilot's level 32, END GAME between pilots resets nothing");
check(apRun([[0, 32, "left"], [0, 32, "left"], [1, 32]]) === "start; split; split", "AstroPop 100%: a pilot already played never counts again");
check(apRun([[0, 32, "left"], [1, 5, "lost"]]) === "start; split; reset resetRowPush", "AstroPop 100%: a loss in a later pilot's game resets");
check(apRun([[0, 10, "left"], [1, 32]]) === "start; reset resetLeft; start; split", "AstroPop 100%: leaving a pilot's game before level 32 resets");

// Typer Shark: its block logs (tools/autosplit/typershark/, the port's scripted games; a line at each change)
function tsLog(name) { return fs.readFileSync(path.join(__dirname, "autosplit", "typershark", name + ".block.log"), "utf8").split(/\r?\n/); }
check(blockReplay("typershark", "Adventure Mode - Easy", tsLog("adventure_easy")) === "start; split; split; reset resetLives",
      "Typer Shark Adventure Mode: a split at each return to the map, the last life lost resets");
check(blockReplay("typershark", "Adventure Mode - X-Treme!", tsLog("adventure_xtreme")) === "start; split; split; reset resetLives", "Typer Shark Adventure Mode on X-Treme!");
check(blockReplay("typershark", "Adventure Mode - Easy", tsLog("adventure_normal")) === "", "Typer Shark: a game of another difficulty never starts the run");
check(blockReplay("typershark", "Adventure Mode - Easy", tsLog("adventure_won")) === "start; split; split", "Typer Shark: the win after a level jump ends nothing early");
check(blockReplay("typershark", "Death%", tsLog("adventure_easy")) === "start; split" && blockReplay("typershark", "Death%", tsLog("abyss")) === "",
      "Typer Shark Death%: an adventure's first game over ends it; the Abyss never starts it");
check(blockReplay("typershark", "Abyss Mode", tsLog("abyss")) === "start; split", "Typer Shark Abyss Mode: from the dive's GO to its game over");
check(blockReplay("typershark", "Abyss Mode", tsLog("abyss_deep_dive_resumed")) === "start; reset resetReplaced",
      "Typer Shark Abyss Mode: a dive replaced by a deeper one resets, and the deeper one starts nothing");
check(blockReplay("typershark", "Death%", tsLog("adventure_left_from_options")) === "start; reset resetQuit", "Typer Shark: end this game resets as a quit");
check(blockReplay("typershark", "Death%", tsLog("adventure_continued_save")) === "", "Typer Shark: a continued save never starts a run");

// Cosmic Bugs: its block logs (tools/autosplit/cosmicbugs/, the port's scripted games; a line at each change), and a whole
// mission made up of its fields (577 board clears, then the win)
function cbLog(name) { return fs.readFileSync(path.join(__dirname, "autosplit", "cosmicbugs", name + ".block.log"), "utf8").split(/\r?\n/); }
check(blockReplay("cosmicbugs", "Any%", cbLog("game")) === "start; split; reset resetLives", "Cosmic Bugs Any%: the first board splits, out of lives resets");
check(blockReplay("cosmicbugs", "Any%", cbLog("left")) === "start; reset resetLeft; start; reset resetLives", "Cosmic Bugs: a game left with Esc resets, the next one starts");
check(blockReplay("cosmicbugs", "Any%", cbLog("resume")) === "start; split; reset resetLeft" && blockReplay("cosmicbugs", "Any%", cbLog("select")) === "start; split; reset resetLives",
      "Cosmic Bugs: a continued game or a chosen board starts nothing");
var cbMission = ["u=0 seq=1 active=1 flags=00 board=0 go=0/0 over=0/0 deaths=0,0,0,0 mode=-1 skill=0 ingame=0 progress=-1 blc=0 wins=0/0"];
for (var cb = 0; cb <= 577; cb++) cbMission.push("u=" + (cb + 1) + " seq=1 active=1 flags=04 board=1 go=1/1 over=0/0 deaths=0,0,0,0 mode=0 skill=0 ingame=1 progress=-1 blc=" + cb + " wins=0/0");
cbMission.push("u=600 seq=1 active=1 flags=104 board=1 go=1/1 over=0/0 deaths=0,0,0,0 mode=0 skill=0 ingame=0 progress=-1 blc=577 wins=1/1");
check(blockReplay("cosmicbugs", "Any%", cbMission) === "start; " + splits(13), "Cosmic Bugs Any%: splits at boards 1, 49 ... 529, the end at the win after board 577");
check(blockReplay("cosmicbugs", "Any%", cbMission.slice(0, 579)) === "start; " + splits(12), "Cosmic Bugs Any%: the 577th board's clear is not the end");

// Dynomite: its block logs (tools/autosplit/dynomite/, the port's -livesplitlog games on a test seed; a line at each change),
// and a whole Fossil Challenge made up of its fields (15 fossils, the win with the last)
function dyLog(name) { return fs.readFileSync(path.join(__dirname, "autosplit", "dynomite", name + ".block.log"), "utf8").split(/\r?\n/); }
check(blockReplay("dynomite", "Endless Puzzle - 1K - Easy", dyLog("endless")) === "start; split", "Dynomite Endless Puzzle 1K: from the click to 1,000 on screen");
check(blockReplay("dynomite", "Endless Puzzle - 5K - Easy", dyLog("endless")) === "start; split; reset resetEndless",
      "Dynomite Endless Puzzle 5K: 1K splits, the eggs off the bottom reset");
check(blockReplay("dynomite", "Endless Puzzle - 1K - Normal", dyLog("endless")) === "" && blockReplay("dynomite", "Endless Puzzle - 1K - Easy", dyLog("stomped")) === "",
      "Dynomite: a game of another difficulty or mode never starts the run");
check(blockReplay("dynomite", "Endless Puzzle - 5K - Easy", dyLog("left")) === "start; reset resetLeft; start; reset resetEndless", "Dynomite: a game left resets, the next one starts");
check(blockReplay("dynomite", "Time Trial - Normal", dyLog("timetrial")) === "start; split" && blockReplay("dynomite", "Time Trial - Easy", dyLog("timetrial")) === "",
      "Dynomite Time Trial: from the first egg to the board cleared");
check(blockReplay("dynomite", "Fossil Challenge - Panic", dyLog("panic")) === "start; reset resetFossil", "Dynomite Fossil Challenge Panic: the eggs off the bottom reset");
check(blockReplay("dynomite", "Tutorial", dyLog("tutorial")) === "start; " + splits(5), "Dynomite Tutorial: each mode's part splits, the main menu ends it");
// (the win and the tutorial's closing come in one update: a poll that sees both at once still ends the run)
var dyTut = dyLog("tutorial").filter(function (l) { return !/ flags=108 board=1 .* wins=1\/1 /.test(l); });
check(dyTut.length === dyLog("tutorial").length - 1 && blockReplay("dynomite", "Tutorial", dyTut) === "start; " + splits(5),
      "Dynomite Tutorial: the win seen with the board already gone ends the run");
// (and in a fast run the last part's clear, the win and the closing come in one poll: the win splits the segments left)
var dyLast = dyLog("tutorial"), dyCut = dyLast.findIndex(function (l) { return / lc=4\//.test(l); });
var dyFast = dyLast.slice(0, dyCut).concat(dyLast.slice(dyCut).filter(function (l) { return / board=0 /.test(l); }));
check(blockReplay("dynomite", "Tutorial", dyFast) === "start; " + splits(5), "Dynomite Tutorial: the last part, the win and the closing in one poll end the run");
check(blockReplay("dynomite", "Fossil Challenge - Strategic", dyLog("fossil")) === "start; " + splits(15),
      "Dynomite Fossil Challenge: a win after a jump to the last fossil ends the run");
// (its saved games are Stomped Puzzle, which has no category: read as Endless Puzzle's, the continued one counts no GO)
var dySaved = dyLog("saved2").map(function (l) { return l.replace("mode=1 ", "mode=2 "); });
check(blockReplay("dynomite", "Endless Puzzle - 1K - Easy", dySaved) === "" &&
      blockReplay("dynomite", "Endless Puzzle - 1K - Easy", dyLog("saved1").map(function (l) { return l.replace("mode=1 ", "mode=2 "); })) === "start; reset resetLeft",
      "Dynomite: a continued save never starts a run");
var dyFossil = ["u=0 seq=1 active=1 flags=00 board=0 go=0/0 over=0/0 deaths=0,0,0,0 mode=-1 skill=0 ingame=0 progress=-1 blc=0 wins=0/0"];
for (var dy = 0; dy <= 15; dy++) dyFossil.push("u=" + (dy + 1) + " seq=1 active=1 flags=04 board=1 go=1/1 over=0/0 deaths=0,0,0,0 mode=3 skill=2 ingame=1 progress=" + 3 * dy + " blc=" + dy + " wins=" + (dy === 15 ? "1/1" : "0/0"));
check(blockReplay("dynomite", "Fossil Challenge - Panic", dyFossil) === "start; " + splits(15), "Dynomite Fossil Challenge: 15 fossils, 15 splits");
check(blockReplay("dynomite", "Fossil Challenge - Panic", dyFossil.slice(0, 16)) === "start; " + splits(14), "Dynomite Fossil Challenge: the 14th fossil is not the end");

console.log("autosplittest: " + (fx.cases.length + bm.cases.length + bw.cases.length) + " scenario replays, " + checks + " checks, " + fails + " failed");
process.exit(fails ? 1 : 0);
