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
//
// Every other port writes the shared block, layout 2 (platform/AutosplitBlock.h), which one decider reads, driven by the game's
// row of the rules table below (docs/WEB_PLAYER.md, "One design for every port"). Big Money's and BookWorm's rows replay their
// native scripts' scenarios (games/<game>/tools/livesplit/lsdecide.py; tools/autosplit/<game>-scenarios.json.gz).
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

  // ---- Every other port: the shared block, layout 2 (platform/AutosplitBlock.h; docs/WEB_PLAYER.md, "One design for every
  // port"), and one decider driven by the game's row of GAMES ----
  var V2 = {
    u32: { self: 0x1C, seq: 0x20, active: 0x24, updates: 0x28, flags: 0x2C, boardSerial: 0x30, goCount: 0x34, goSerial: 0x38,
           levelClears: 0x3C, levelClearSerial: 0x40, gameOvers: 0x44, gameOverSerial: 0x48, death0: 0x4C, death1: 0x50,
           death2: 0x54, death3: 0x58, own0: 0x80, wins: 0xB0, winSerial: 0xB4, inGame: 0xC0 },
    i32: { tick: 0x5C, state: 0x60, level: 0x64, displayLevel: 0x68, displayScore: 0x6C, score: 0x70, boardLevelClears: 0x74,
           goTick: 0x78, displayScoreMax: 0x7C, mode: 0xB8, skill: 0xBC, progress: 0xC4 }
  };
  var V2_FIELDS = Object.keys(V2.u32).concat(Object.keys(V2.i32));
  var FLAG_RESUMED = 0x02;

  // A reader of one game's layout-2 block (id: its four letters): one consistent copy, or null, as readBlock. A block of an
  // older layout (a port not yet converted) is not read.
  function blockReader(id) {
    var want = 0;
    for (var c = 3; c >= 0; c--) want = want * 256 + id.charCodeAt(c);
    return function (heap, at) {
      if (!heap || !at || at + 0xD0 > heap.length) return null;
      var dv = new DataView(heap.buffer, heap.byteOffset + at, 0xD0);
      for (var i = 0; i < BJ.magic.length; i++) if (dv.getUint8(i) !== BJ.magic[i]) return null;
      if (dv.getUint32(0x14, true) !== want || dv.getUint16(0x18, true) < 2 || dv.getUint16(0x1A, true) < 0xD0) return null;
      var s1 = dv.getUint32(0x20, true);
      if (s1 & 1) return null;
      var f = {};
      for (var k in V2.u32) f[k] = dv.getUint32(V2.u32[k], true);
      for (var j in V2.i32) f[j] = dv.getInt32(V2.i32[j], true);
      if (dv.getUint32(0x20, true) !== s1) return null;
      return f;
    };
  }

  // The number a category's name gives from one of a game's tables ({name: number}): that of the one name it contains, else
  // -1 (none, or more than one)
  function named(category, table) {
    var found = -1, c = String(category || "").toLowerCase();
    Object.keys(table || {}).forEach(function (n) { if (c.indexOf(n.toLowerCase()) >= 0) found = found === -1 ? table[n] : -2; });
    return found < 0 ? -1 : found;
  }

  function templateName(t) { return t.sub ? t.category + " - " + t.sub : t.category; }

  // A template's end ("levels N", "score N", "won", "lost", "lost N", "progress N") as {kind, n, hasN}
  function parseEnd(text) {
    var w = String(text || "").trim().split(/\s+/);
    return w[0] ? { kind: w[0], n: parseInt(w[1], 10) || 0, hasN: w.length > 1 } : null;
  }

  // The shared decider: Bejeweled's, with the game's rules in place of its own. rules: the game's row of GAMES. The run's
  // template is the one named as the run's category; its mode and skill, else the mode and skill names the category contains,
  // guard the start. Its end decides the last segment where the segments can't say it (won, lost, progress); every other
  // split is the segment's level or score, as in Bejeweled. A game over on the run's board resets the run, each death kind
  // its own setting, unless the end is "lost". timer and opts as bejeweled().
  function makeDecider(rules) {
    var settings = { modeGuard: [true, null], vanillaOnly: [false, null], resets: [true, null] };
    rules.deaths.forEach(function (d) { if (d.setting) settings[d.setting] = [true, "resets"]; });
    settings.resetReplaced = [true, "resets"];
    settings.resetQuit = [true, "resets"];
    (rules.templates || []).forEach(function (t) { if (t.limit) settings[t.limit.setting] = [true, "resets"]; });
    var decider = function (timer, opts) {
      var values = {};
      Object.keys(settings).forEach(function (k) { values[k] = settings[k][0]; });
      Object.keys((opts && opts.settings) || {}).forEach(function (k) { values[k] = !!opts.settings[k]; });
      function setting(name) {
        while (name) { if (!values[name]) return false; name = settings[name] ? settings[name][1] : null; }
        return true;
      }
      var category = String((opts && opts.category) || ""), template = null;
      (rules.templates || []).forEach(function (t) { if (templateName(t).toLowerCase() === category.toLowerCase()) template = t; });
      var catMode = template && template.mode !== undefined ? template.mode : named(category, rules.modes);
      var catSkill = template && template.skill !== undefined ? template.skill : named(category, rules.skills);
      // whether a block's mode or skill is outside the category's: a number (-1 for any) or a list of the allowed ones
      function outside(want, v) { return Array.isArray(want) ? want.indexOf(v) < 0 : want >= 0 && want !== v; }
      var end = template ? parseEnd(template.end) : null;
      var splits = [], runSerial = 0, pending = null, baseline = true, seenActive = false, warned = false, attachT = null;
      // (a run over several games, template.games: the skills its games have had, and whether the run's game has reached
      // its end and the run waits for the next game's GO)
      var skillsUsed = [], between = false;
      var current = {}, old = {};
      V2_FIELDS.forEach(function (k) { current[k] = 0; });
      // a game over on the run's board that ends it, in its last segment ("lost"; "lost N": only while the game's progress is
      // at most N); one in an earlier segment resets the run
      function lostEnd() {
        return !!end && end.kind === "lost" && timer.index() === splits.length - 1 && current.gameOvers !== old.gameOvers &&
               current.gameOverSerial === runSerial && (!end.hasN || current.progress <= end.n);
      }
      // a new game that may start a run: a GO on the board in play, not a replay or resumed game, of the category's mode and skill
      function isGo() {
        return current.goCount !== old.goCount && current.goSerial === current.boardSerial && !(current.flags & (FLAG_REPLAY | FLAG_RESUMED)) &&
               !(setting("vanillaOnly") && !(current.flags & FLAG_VANILLA)) &&
               !(setting("modeGuard") && (outside(catMode, current.mode) || outside(catSkill, current.skill)));
      }
      function reached() {
        var i = timer.index();
        if (runSerial === 0 || current.boardSerial !== runSerial || i < 0 || i >= splits.length) return false;
        // (each segment of a run over several games is one game, which ends at the end's progress; between games, none)
        if (template && template.games) return !between && end.kind === "progress" && current.progress >= end.n;
        if (end && i === splits.length - 1) {
          if (end.kind === "won") return current.wins !== old.wins && current.winSerial === runSerial;
          if (end.kind === "lost") return lostEnd();
          if (end.kind === "progress") return current.progress >= end.n;
        }
        // (a template with by "progress" splits on the game's own figure instead: Mummy Maze's treasures; one with targets
        // at the level clears it lists, a segment for many levels: Cosmic Bugs' steps of 48 boards)
        if (template && template.targets) return current.boardLevelClears >= template.targets[i];
        var s = splits[i];
        if (template && template.by === "progress") return current.progress >= s.target;
        return (s.score ? current.displayScoreMax : current.boardLevelClears) >= s.target;
      }
      return {
        // the timer started (by the decider or by hand): the run is the current board's
        onStart: function () {
          splits = parseSplits((opts && opts.segments) || []); runSerial = current.boardSerial; pending = null;
          skillsUsed = [current.skill]; between = false;
        },
        onReset: function () { runSerial = 0; pending = null; between = false; },
        board: function () { return current.boardSerial; },
        poll: function (t, snap) {
          if (attachT === null) attachT = t;
          old = {};
          for (var k in current) old[k] = current[k];
          var updated = false, ask = false;
          if (snap) {
            if (!snap.active) {
              if (!seenActive && !warned && t - attachT > 10000) { warned = true; ask = true; }
              baseline = true;
            } else {
              seenActive = true;
              V2_FIELDS.forEach(function (f) { current[f] = snap[f] || 0; });
              if (baseline) baseline = false; else updated = true;
            }
          }
          if (!updated) return ask;
          var ph = timer.phase();
          if (ph === "Running" || ph === "Paused") {
            var cause = null;
            if (setting("resets") && runSerial !== 0 && !between) {
              // a new game over or board change, else one held back in an earlier poll
              cause = pending; pending = null;
              if (current.gameOvers !== old.gameOvers && current.gameOverSerial === runSerial && !lostEnd()) {
                cause = "resets";
                for (var d = 0; d < rules.deaths.length; d++)
                  if (current["death" + d] !== old["death" + d]) { cause = rules.deaths[d].setting || "resets"; break; }
              } else if (old.boardSerial === runSerial && current.boardSerial !== runSerial && !(old.flags & FLAG_GAME_OVER)) {
                cause = current.boardSerial === 0 ? "resetQuit" : "resetReplaced";
              } else if (template && template.limit && current.boardSerial === runSerial &&
                         current[template.limit.field] > template.limit.max && !(old[template.limit.field] > template.limit.max)) {
                // a figure of the game's own over the category's limit (AstroPop's SUPA Weapon used in No SUPA Weapon)
                cause = template.limit.setting;
              }
              if (cause !== null && !setting(cause)) cause = null;
              // the segment was reached by the game over (the same poll saw both): split first
              if (cause !== null && reached()) { pending = cause; cause = null; }
            }
            if (cause !== null) timer.reset(cause);
            else if (reached()) { timer.split(); if (template && template.games && timer.phase() === "Running") between = true; }
            // the next game of a run over several games: a new game of the category's mode with a skill the run has not had
            if (between && timer.phase() === "Running" && isGo() && skillsUsed.indexOf(current.skill) < 0) {
              runSerial = current.boardSerial; skillsUsed.push(current.skill); between = false;
            }
          }
          if (timer.phase() === "NotRunning" && isGo()) timer.start();
          return ask;
        },
        // the game went away (its window closed, its page reloaded)
        exit: function () {
          var ph = timer.phase();
          if ((ph === "Running" || ph === "Paused") && runSerial !== 0 && setting("resets") && setting("resetQuit")) timer.reset("exit");
        }
      };
    };
    decider.settings = settings;
    return decider;
  }

  // The rules table, one row a game (docs/WEB_PLAYER.md, "The rules table"): its name on speedrun.com, its block's id, the
  // names behind its mode and skill numbers, its four death kinds (each a reset setting, or none for the reset setting alone) and its templates.
  //
  // Big Money has no speedrun.com board: its rules are its native script's, its templates that script's suggestions
  // (games/bigmoney/tools/make_example_mods.py LIVESPLIT_TEMPLATES), with the mode and skill in the category's name.
  var BM_RULES = {
    name: "Big Money Deluxe", id: "BMDX",
    modes: { Action: 0, Strategy: 1, Puzzle: 2 }, skills: { Easy: 1, Normal: 2, Hard: 3 },
    deaths: [{ setting: "resetOverflow", what: "the coins overflow (Action, Strategy)" }, { setting: "resetNoMoves", what: "no moves left (Puzzle)" },
             { setting: "resetNewGame", what: "New Game" }, { setting: null, what: "any other game over (the reset setting only)" }],
    templates: ["Action", "Strategy", "Puzzle"].reduce(function (a, mode) {
      return a.concat(["Easy", "Hard"].map(function (skill) { return { category: mode + " " + skill, sub: "5 Levels", segments: levels(5), end: "levels 5" }; }));
    }, []).concat([{ category: "Action Easy", sub: "25K Points", segments: ["5K", "10K", "25K"], end: "score 25000" }])
  };

  // BookWorm (speedrun.com: Bookworm Deluxe): its native script's rules, and the board's two categories. Its progress is the
  // rank the summary shows, the levels cleared up to 31: Bookworm Supreme (Any%) ends at the 31st, Bookburner (Death%) at a
  // game over before the first level is cleared (rank 0); a later game over resets that run.
  var BW_RULES = {
    name: "Bookworm Deluxe", id: "BWDX",
    modes: { Classic: 0, Action: 1 }, skills: {},
    deaths: [{ setting: "resetFire", what: "a fire tile burns through (Classic)" }, { setting: "resetCountdown", what: "the countdown ends (Action)" },
             { setting: "resetLeft", what: "LEAVE GAME" }, { setting: null, what: "any other game over (the reset setting only)" }],
    templates: [{ category: "Bookworm Supreme", sub: "Any%", segments: levels(31), end: "progress 31" },
                { category: "Bookburner", sub: "Death%", segments: ["Bookburner"], end: "lost 0" }]
  };

  // Seven Seas (speedrun.com: Seven Seas Deluxe): Level 30 on each difficulty, the first 30 levels. The game's level clear is
  // the update its level counter goes up, after the level's panel; after levels 10, 20 and 30 the map is made in that same
  // update, which is the board's end. Its progress is the levels completed. Undo can't take a level clear back. Its one game
  // over is running out of lives; New Game saves the game and takes the board away, which resets as a quit.
  var SS_RULES = {
    name: "Seven Seas Deluxe", id: "SSDX",
    modes: {}, skills: { Easy: 1, Average: 2, Hard: 3 },
    deaths: [{ setting: "resetLives", what: "out of lives" }, { setting: null, what: "(none)" }, { setting: null, what: "(none)" },
             { setting: null, what: "(none)" }],
    templates: ["Easy", "Average", "Hard"].map(function (skill, i) {
      return { category: "Level 30 (" + skill + ")", segments: levels(30), end: "progress 30", mode: 0, skill: i + 1 };
    })
  };

  // Mummy Maze (speedrun.com: Mummy Maze Deluxe): a board is a whole game, menu to menu, its GO the first update the explorer
  // takes moves, and a level clear the level-complete screen of each chamber (15 a pyramid). Its progress is the treasures
  // collected; in Classic it rises when the next pyramid has begun, which is 1st Pyramid's end (the spawn in the next
  // pyramid). 100% Run (Adventure) splits at each treasure and ends at the win, the last pyramid's chest. Being caught is no
  // game over (the chamber is tried again); the one game over is Abandon Hope, in Classic.
  var MM_RULES = {
    name: "Mummy Maze Deluxe", id: "MMDX",
    modes: { Classic: 0, Adventure: 1 }, skills: {},
    deaths: [{ setting: "resetAbandon", what: "Abandon Hope (Classic)" }, { setting: null, what: "(none)" }, { setting: null, what: "(none)" },
             { setting: null, what: "(none)" }],
    templates: [{ category: "1st Pyramid", segments: levels(15).map(function (x) { return x.replace("Level", "Chamber"); }).concat(["Next Pyramid"]),
                  end: "progress 1", mode: 0 },
                { category: "100% Run", segments: levels(15).map(function (x) { return x.replace("Level", "Pyramid"); }), end: "won", mode: 1,
                  by: "progress" }]
  };

  // Rocket Mania Deluxe has no speedrun.com board of its own: the board named Rocket Mania! is for a Java edition. Its row is
  // the site's own categories, Level 10, 20 and 30 on each difficulty, from "Get Ready!" (its GO, the first update of a new
  // game) to the "Level Up!" of the last level (its level clear), in Arcade or Classic (Strategy never ends a level). A loss
  // counts when the game-over screen comes (a launch still burning at the night's end can still clear the level in Classic).
  // No win: the levels go on.
  var RM_RULES = {
    name: "Rocket Mania Deluxe", id: "RMDX",
    modes: { Arcade: 0, Classic: 1, Strategy: 2 }, skills: { Easy: 0, Normal: 1, Hard: 2 },
    deaths: [{ setting: "resetOutOfTime", what: "the night ran out (Classic)" }, { setting: "resetTopLine", what: "the top line filled (Arcade)" },
             { setting: "resetNoMatches", what: "no matches left (Strategy)" }, { setting: null, what: "(none)" }],
    templates: [10, 20, 30].reduce(function (a, n) {
      return a.concat(["Easy", "Normal", "Hard"].map(function (skill, i) {
        return { category: "Level " + n, sub: skill, segments: levels(n), end: "levels " + n, mode: [0, 1], skill: i };
      }));
    }, [])
  };

  // Alchemy (speedrun.com: Alchemy Deluxe): 1 Board and 7 Boards in Strategic or Time Trial, from the click on Start New
  // Game (its GO, the first update of the new board) to the board's result screen (its level clear, the update the
  // statistics panel goes up); the panel's button goes on with the same game. Tutorial% ends at its win, the tutorial's
  // last step appearing (the wizard's "Click here to start playing!"), the code's one moment for done; the button's own
  // click only leaves. The skill is the player's choice, so any. Leaving a game mid-board is its own game over.
  var AL_RULES = {
    name: "Alchemy Deluxe", id: "ALDX",
    modes: { Strategic: 0, "Time Trial": 1, Tutorial: 2 }, skills: {},
    deaths: [{ setting: "resetForgeDiscard", what: "a discard overflowed the forge (Strategic)" },
             { setting: "resetForgeFull", what: "the forge stayed full (Time Trial)" },
             { setting: "resetLeft", what: "another game or the selector replaced the board" },
             { setting: "resetForgeDiscardTT", what: "a discard overflowed the forge (Time Trial)" }],
    templates: [1, 7].reduce(function (a, n) {
      return a.concat(["Strategic", "Time Trial"].map(function (mode, i) {
        return { category: n + (n === 1 ? " Board" : " Boards"), sub: mode, segments: levels(n), end: "levels " + n, mode: i };
      }));
    }, []).concat([{ category: "Tutorial%", segments: ["Tutorial"], end: "won", mode: 2 }])
  };

  // AstroPop (speedrun.com: Astropop Deluxe): Any% and Any% (No SUPA Weapon) for each pilot, a new Classic game from level 1
  // to level 32's "Level Up" (its GO, the first update of the new game: the start click on the title comes before the pilot
  // is chosen, so the block cannot see it; a game from a later level or a saved one is resumed and starts nothing). Its
  // progress is the level just cleared: a fast warp gate skips levels, so the splits follow progress, not the clears. No SUPA
  // Weapon resets on the first use, counted in the game's own first word. 100% is the four pilots, one new game each, a
  // segment a game: the run waits after each pilot's level 32 for the next pilot's new game. Survival has no timer.
  var AP_PILOTS = ["Vector", "Sprocket", "Turbot", "Vixx"];
  var AP_RULES = {
    name: "Astropop Deluxe", id: "APDX",
    modes: { Classic: 1, Survival: 2 }, skills: { Vector: 0, Sprocket: 1, Turbot: 2, Vixx: 3 },
    deaths: [{ setting: "resetRowPush", what: "a new row pushed bricks to the ship" }, { setting: "resetFieldFull", what: "the stack filled the field" },
             { setting: "resetLeft", what: "END GAME" }, { setting: null, what: "any other game over (the reset setting only)" }],
    templates: ["Any%", "Any% (No SUPA Weapon)"].reduce(function (a, category) {
      return a.concat(AP_PILOTS.map(function (pilot, i) {
        var t = { category: category, sub: pilot, segments: levels(32), end: "progress 32", mode: 1, skill: i, by: "progress" };
        if (category !== "Any%") t.limit = { field: "own0", max: 0, setting: "resetSupa" };
        return t;
      }));
    }, []).concat([{ category: "100%", segments: AP_PILOTS.map(function (x, i) { return "Pilot " + (i + 1); }), end: "progress 32", mode: 1,
                     games: 4 }])
  };

  // Typer Shark (speedrun.com: Typer Shark Deluxe): a block board is a whole game, its GO the first update after the
  // difficulty (or the Abyss) is chosen. Adventure Mode, for each difficulty, ends on returning to the map after level 11
  // is beaten: its progress is the levels beaten and left, which rises as the map comes back, so each level splits there
  // (a level's own clear comes earlier, before the player leaves it). Abyss Mode ends at its game over, whose time the
  // game-over screen shows; Death% at the first game over of an adventure. Leaving through the options' "end this game"
  // is no game over: the game goes away, a quit.
  var TS_SKILLS = ["Easy", "Normal", "Hard", "Expert", "X-Treme!"];
  var TS_RULES = {
    name: "Typer Shark Deluxe", id: "TSDX",
    modes: { Adventure: 0, Abyss: 1 }, skills: {},
    deaths: [{ setting: "resetLives", what: "out of lives (Adventure)" }, { setting: null, what: "out of lives in the Abyss (Abyss Mode's end)" },
             { setting: null, what: "(none)" }, { setting: null, what: "(none)" }],
    templates: TS_SKILLS.map(function (skill, i) {
      return { category: "Adventure Mode", sub: skill, segments: levels(11), end: "progress 11", mode: 0, skill: i, by: "progress" };
    }).concat([{ category: "Abyss Mode", segments: ["Abyss"], end: "lost", mode: 1 },
               { category: "Death%", segments: ["Death"], end: "lost", mode: 0 }])
  };

  // Cosmic Bugs (speedrun.com: Cosmic Bugs): Any%, from the click on Play on the instructions page (its GO, the update
  // that click makes the first board in; a new player's Start Game shows that page, a continued or chosen board does not
  // and starts nothing) to the "Congratulations! Mission complete" screen, its win. A mission is 577 boards, the first
  // made by hand and then 12 steps of 48, so the splits are the first board and each step's last (`targets`, the level
  // clears each needs); the last is the win, which follows the 577th board's clicked-through texts. One mode; the
  // difficulty has no screen to change it, so any.
  var CB_RULES = {
    name: "Cosmic Bugs", id: "CBDX",
    modes: {}, skills: {},
    deaths: [{ setting: "resetLives", what: "out of lives" }, { setting: "resetLeft", what: "the game left for another screen (Esc)" },
             { setting: null, what: "(none)" }, { setting: null, what: "(none)" }],
    templates: [{ category: "Any%", segments: ["Board 1"].concat([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12].map(function (k) { return "Step " + k; })),
                  targets: [1, 49, 97, 145, 193, 241, 289, 337, 385, 433, 481, 529], end: "won", mode: 0 }]
  };

  // A row of GAMES from a row of the rules table
  function ruled(rules) {
    return { name: rules.name, block: "_byopop_livesplit_block", read: blockReader(rules.id), decider: makeDecider(rules),
             templates: rules.templates, rules: rules };
  }

  // The games with an auto splitter: the page id, the game's name on speedrun.com, how to find its block, its decider and its
  // templates
  var GAMES = {
    bejeweled: { name: "Bejeweled Deluxe", block: "_byopop_livesplit_block", read: readBlock, decider: bejeweled, templates: BJ_TEMPLATES },
    bigmoney: ruled(BM_RULES),
    bookworm: ruled(BW_RULES),
    sevenseas: ruled(SS_RULES),
    mummymaze: ruled(MM_RULES),
    rocketmania: ruled(RM_RULES),
    alchemy: ruled(AL_RULES),
    astropop: ruled(AP_RULES),
    typershark: ruled(TS_RULES),
    cosmicbugs: ruled(CB_RULES)
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

  return { games: GAMES, sample: sample, readBlock: readBlock, blockReader: blockReader, parseSplits: parseSplits, bejeweled: bejeweled,
           makeDecider: makeDecider, settings: SETTINGS };
})();
if (typeof module !== "undefined") module.exports = ByopopAutosplit;
