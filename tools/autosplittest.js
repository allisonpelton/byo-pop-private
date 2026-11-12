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

console.log("autosplittest: " + (fx.cases.length + bm.cases.length + bw.cases.length) + " scenario replays, " + checks + " checks, " + fails + " failed");
process.exit(fails ? 1 : 0);
