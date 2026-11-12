// The speedrun timer's auto splitters (the user, 2026-11-11: "Add Bejeweled autosplitter to the site. Also, let's build in
// autosplitters to every port of a game with a leaderboard on speedrun.com"). A port publishes a small block of its state
// after every update (Bejeweled: games/bejeweled/src/LiveSplitBlock.h, filled by LiveSplitObserver.cpp, the same block the
// native auto splitter games/bejeweled/tools/livesplit/BejeweledDeluxe-byopop.asl reads); the Splits window (web/splits.js)
// reads it from the front game window every frame and starts, splits and resets its timer as the game's rules here decide.
//
// Bejeweled's rules are games/bejeweled/tools/livesplit/lsdecide.py's (the .asl's, re-implemented), line for line: the start
// at GO of a board in the category's mode, a split at each wormhole (a segment named "Level N" or any name that is not a
// score) or when the score on screen reaches a segment's number (1000 or more: "10000", "25,000", "25K"), and a reset when the
// run's board ends (no moves, time up, New Game, replaced by another board, or the game quit), each reset kind a setting.
// tools/autosplittest.js replays lsdecide.py's 27 scenarios through it (tools/autosplit/bejeweled-scenarios.json.gz).
var ByopopAutosplit = (function () {
  "use strict";

  // ---- Bejeweled: the block, layout 1 (LiveSplitBlock.h) ----
  var BJ = {
    magic: [0x42, 0x59, 0x4F, 0x50, 0x4F, 0x50, 0x2D, 0x4C, 0x49, 0x56, 0x45, 0x53, 0x50, 0x4C, 0x49, 0x54, 0xF0, 0xF1, 0xF2, 0xF3],
    gameId: 0x58444A42, size: 0xB0,
    // u32 and i32 fields by byte offset
    u32: { self: 0x1C, seq: 0x20, active: 0x24, updates: 0x28, flags: 0x2C, boardSerial: 0x30, goCount: 0x34, goSerial: 0x38,
           levelClears: 0x3C, levelClearSerial: 0x40, gameOvers: 0x44, gameOverSerial: 0x48, deathsNoMoves: 0x4C,
           deathsTimeUp: 0x50, deathsNewGame: 0x54, deathsOther: 0x58 },
    i32: { tick: 0x5C, state: 0x60, level: 0x64, displayLevel: 0x68, displayScore: 0x6C, score: 0x70, boardLevelClears: 0x74,
           goTick: 0x78, displayScoreMax: 0x7C }
  };
  var FLAG_TIMED = 0x01, FLAG_VANILLA = 0x04, FLAG_REPLAY = 0x20, FLAG_GAME_OVER = 0x40;
  var FIELDS = ["self", "seq", "active", "updates", "flags", "boardSerial", "goCount", "goSerial", "levelClears", "levelClearSerial",
                "gameOvers", "gameOverSerial", "deathsNoMoves", "deathsTimeUp", "deathsNewGame", "deathsOther", "tick", "state", "level",
                "displayLevel", "displayScore", "score", "boardLevelClears", "goTick", "displayScoreMax"];

  // One consistent copy of the block from a game's memory (heap: its HEAPU8), or null: the block is not there, or the game was
  // writing it (seq odd, or changed during the copy). The game writes it between its updates, on the page's own thread, so a
  // copy is never torn in practice; the check is the native reader's, kept for the same block.
  function readBlock(heap, at) {
    if (!heap || !at || at + BJ.size > heap.length) return null;
    var dv = new DataView(heap.buffer, heap.byteOffset + at, BJ.size);
    for (var i = 0; i < BJ.magic.length; i++) if (dv.getUint8(i) !== BJ.magic[i]) return null;
    if (dv.getUint32(0x14, true) !== BJ.gameId) return null;
    var s1 = dv.getUint32(0x20, true);
    if (s1 & 1) return null;
    var f = {};
    for (var k in BJ.u32) f[k] = dv.getUint32(BJ.u32[k], true);
    for (var j in BJ.i32) f[j] = dv.getInt32(BJ.i32[j], true);
    if (dv.getUint32(0x20, true) !== s1) return null;
    return f;
  }

  // A segment list's targets (lsdecide.py parse_splits): [{score: bool, target}], a number of at least 1000 a score, anything
  // else the next level (its target the level clears it needs); a "-" or "{Section}" prefix is dropped
  function parseSplits(names) {
    var out = [], levels = 0;
    names.forEach(function (name) {
      name = String(name).trim();
      if (name.charAt(0) === "-") name = name.slice(1).trim();
      if (name.charAt(0) === "{" && name.indexOf("}") > 0) name = name.slice(name.indexOf("}") + 1).trim();
      var digits = name.replace(/,/g, ""), scale = 1;
      if (/[Kk]$/.test(digits)) { scale = 1000; digits = digits.slice(0, -1); }
      if (digits.length > 0 && digits.length <= 12 && /^[0-9]+$/.test(digits) && parseInt(digits, 10) * scale >= 1000)
        out.push({ score: true, target: parseInt(digits, 10) * scale });
      else { levels++; out.push({ score: false, target: levels }); }
    });
    return out;
  }

  // The settings (the .asl's), each with its parent: a child counts only while its parent is on
  var SETTINGS = { modeGuard: [true, null], vanillaOnly: [false, null], resets: [true, null], resetNoMoves: [true, "resets"],
                   resetTimeUp: [true, "resets"], resetNewGame: [true, "resets"], resetReplaced: [true, "resets"],
                   resetQuit: [true, "resets"] };

  // Bejeweled's decider. timer: what it drives and reads, as LiveSplit's script does (the split index and phase are the
  // timer's, so a split undone by hand splits again on the next poll):
  //   phase(): "NotRunning", "Running", "Paused" or "Ended";  index(): the current split's index
  //   start(), split(), reset(cause): the timer's own; the timer calls the decider's onStart() and onReset() for every
  //   start and reset, whoever made it
  // opts: {category, segments: [names], settings: {name: bool}}. poll(t, snap) takes one poll: t in ms, snap a block (null for
  // a torn read). Returns "ask for the mod" once, as the script's message box: the block stayed inactive for 10 s.
  function bejeweled(timer, opts) {
    var values = {};
    Object.keys(SETTINGS).forEach(function (k) { values[k] = SETTINGS[k][0]; });
    Object.keys((opts && opts.settings) || {}).forEach(function (k) { values[k] = !!opts.settings[k]; });
    function setting(name) {
      while (name) { if (!values[name]) return false; name = SETTINGS[name][1]; }
      return true;
    }
    var splits = [], runSerial = 0, pending = null, baseline = true, seenActive = false, warned = false, attachT = null;
    var current = {};
    FIELDS.forEach(function (k) { current[k] = 0; });
    function reached() {
      var i = timer.index();
      if (runSerial === 0 || current.boardSerial !== runSerial || i < 0 || i >= splits.length) return false;
      var s = splits[i];
      return (s.score ? current.displayScoreMax : current.boardLevelClears) >= s.target;
    }
    var d = {
      // the timer started (by the decider or by hand): the run is the current board's
      onStart: function () { splits = parseSplits((opts && opts.segments) || []); runSerial = current.boardSerial; pending = null; },
      onReset: function () { runSerial = 0; pending = null; },
      board: function () { return current.boardSerial; },
      poll: function (t, snap) {
        if (attachT === null) attachT = t;
        var old = {}, k;
        for (k in current) old[k] = current[k];
        var updated = false, ask = false;
        if (snap) {
          if (!snap.active) {
            if (!seenActive && !warned && t - attachT > 10000) { warned = true; ask = true; }
            baseline = true;
          } else {
            seenActive = true;
            FIELDS.forEach(function (f) { current[f] = snap[f] || 0; });
            if (baseline) baseline = false; else updated = true;
          }
        }
        if (updated) {
          var ph = timer.phase();
          if (ph === "Running" || ph === "Paused") {
            var cause = null;
            if (setting("resets") && runSerial !== 0) {
              // a new game over or board change, else one held back in an earlier poll
              cause = pending; pending = null;
              if (current.gameOvers !== old.gameOvers && current.gameOverSerial === runSerial) {
                cause = current.deathsNoMoves !== old.deathsNoMoves ? "resetNoMoves" : current.deathsTimeUp !== old.deathsTimeUp ? "resetTimeUp" :
                        current.deathsNewGame !== old.deathsNewGame ? "resetNewGame" : "resets";
              } else if (old.boardSerial === runSerial && current.boardSerial !== runSerial && !(old.flags & FLAG_GAME_OVER)) {
                cause = current.boardSerial === 0 ? "resetQuit" : "resetReplaced";
              }
              if (cause !== null && !setting(cause)) cause = null;
              // the segment was reached by the game over (the same poll saw both): split first
              if (cause !== null && reached()) { pending = cause; cause = null; }
            }
            if (cause !== null) timer.reset(cause);
            else if (reached()) timer.split();
          }
          if (timer.phase() === "NotRunning") {
            var cat = String((opts && opts.category) || "").toLowerCase(), timed = !!(current.flags & FLAG_TIMED);
            if (current.goCount !== old.goCount && current.goSerial === current.boardSerial && !(current.flags & FLAG_REPLAY) &&
                !(setting("vanillaOnly") && !(current.flags & FLAG_VANILLA)) &&
                !(setting("modeGuard") && ((cat.indexOf("timetrial") >= 0 && !timed) || (cat.indexOf("normal") >= 0 && timed))))
              timer.start();
          }
        }
        return ask;
      },
      // the game went away (its window closed, its page reloaded): the script's exit action
      exit: function () {
        var ph = timer.phase();
        if ((ph === "Running" || ph === "Paused") && runSerial !== 0 && setting("resets") && setting("resetQuit")) timer.reset("exit");
      }
    };
    return d;
  }

  // Bejeweled's split templates: games/bejeweled/tools/make_example_mods.py LIVESPLIT_TEMPLATES, named after speedrun.com's
  // bejeweled1 categories and their subcategories
  function levels(n) { var a = []; for (var i = 1; i <= n; i++) a.push("Level " + i); return a; }
  var SCORES = [10000, 25000, 50000, 100000, 150000, 200000];
  var BJ_TEMPLATES = [5, 10].map(function (n) { return { category: "Normal", sub: n + " Levels", segments: levels(n) }; })
    .concat([5, 10, 15, 20].map(function (n) { return { category: "Timetrial", sub: n + " Levels", segments: levels(n) }; }))
    .concat(SCORES.map(function (t) {
      return { category: "Timetrial - Score Attack", sub: (t / 1000) + "K Points", segments: SCORES.filter(function (x) { return x <= t; }).map(String) };
    }));

  // The games with an auto splitter: the page id, the game's name on speedrun.com, how to find its block, its decider and its
  // templates
  var GAMES = {
    bejeweled: { name: "Bejeweled Deluxe", block: "_byopop_livesplit_block", read: readBlock, decider: bejeweled, templates: BJ_TEMPLATES }
  };

  // The front game's block, read from its window: {game, snap} or null (no game with an auto splitter in front, or it has not
  // started). Asking for the block's address turns the game's writing of it on (the port writes it only once asked).
  function sample(win, gameId) {
    var g = GAMES[gameId];
    if (!g || !win) return null;
    try {
      var M = win.Module;
      if (!M || typeof M[g.block] !== "function" || !M.HEAPU8) return null;
      var at = M[g.block]();
      return { game: gameId, at: at, snap: g.read(M.HEAPU8, at) };
    } catch (e) { return null; }
  }

  return { games: GAMES, sample: sample, readBlock: readBlock, parseSplits: parseSplits, bejeweled: bejeweled, settings: SETTINGS };
})();
if (typeof module !== "undefined") module.exports = ByopopAutosplit;
