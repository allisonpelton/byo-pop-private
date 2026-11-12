// The trophy cabinet (docs/WEB_PLAYER.md, "The trophy cabinet"; the user, 2026-11-11): one Desktop 95 window on the listing's
// desktop that gathers each game's own records from its saves in this browser (its high-score tables as the game keeps them) and
// shows them as a table for each game, under the names the games give their modes. A drop-down picks a site player (web/profiles.js):
// then only the entries under that player's name are shown, compared as the games compare names, without regard to case. Nothing
// is written and nothing leaves the browser: the files are read as the Sounds window reads the games' (ByopopMusic.files).
//
// RECORDS: by game id, where its records are and how they are kept, from the port's own reader (each entry names it):
//   file     the file in the game's user dir
//   kind     "atomica": Atomica's fixed tables (parseAtomica); "popcap": PopCap's high-score record of 2001-2004 (games/bejeweled/docs/GAME_NOTES.md, "hiscores.dat"): a u16 name
//            length, the name, an i32 score and an i32 check (score ^ 0x42BEEF ^ the name's bytes, each signed times 0x11, xored);
//            a record whose check fails shows score % 256, as the games do
//   tables   the tables in the file's order, by the names the game shows them under
//   places   records in each table
//   day      each table starts with an i32 day and the day's best record (shown as the table's "Today" place when it is today's)
//   magic    a u32 the file starts with
//   twice    each name's length is written twice (Rocket Mania)
var ByopopTrophies = (function () {
  "use strict";
  var RECORDS = {
    // games/bejeweled/src/HighScoreWidget.cpp, ReadFromFile
    bejeweled: { file: "hiscores.dat", kind: "popcap", tables: ["Normal", "Time Trial"], places: 5, day: true },
    // games/bigmoney/src/HighScoreWidget.cpp, LoadHighScores (by mGameType)
    bigmoney: { file: "hiscores.dat", kind: "popcap", tables: ["Action", "Strategy", "Puzzle"], places: 5, day: true },
    // games/alchemy/src/HighScores.cpp, ReadFromFile
    alchemy: { file: "hiscores.dat", kind: "popcap", tables: ["Strategic", "Time Trial"], places: 5, day: true },
    // games/sevenseas/src/Screens.cpp, LoadScores
    sevenseas: { file: "hiscores.dat", kind: "popcap", tables: ["High Scores"], places: 5, day: true },
    // games/noahsark/src/Screens.cpp, HighScoreWidget::Load
    noahsark: { file: "hiscores.dat", kind: "popcap", tables: ["High Scores"], places: 5 },
    // games/mummymaze/src/Screens2.cpp, ReadFromFile
    mummymaze: { file: "hiscores.dat", kind: "popcap", tables: ["Classic"], places: 5 },
    // games/rocketmania/src/Dialogs.cpp, LoadScores
    rocketmania: { file: "Players/hiscores.dat", kind: "popcap", tables: ["Classic", "Arcade", "Strategy"], places: 10, day: true, twice: true },
    // games/atomica/src/AppGame.cpp, ReadHighScores (HighScores.cpp): two fixed tables, expert games sharing their mode's
    atomica: { file: "HS.CFG", kind: "atomica", tables: ["Strategy", "Action"] },
    // games/dynomite/src/HighScores.cpp: a file a table, five rows of 28 bytes (an i32 id, a 20-byte name, an i32 score);
    // Time Trial's score is minus the hundredths of a second the board took, shown as m:ss
    dynomite: { files: [{ file: "STOMPED.HS", kind: "dynomite", tables: ["Stomped"] }, { file: "ENDLESS.HS", kind: "dynomite", tables: ["Endless"] },
      { file: "FOSSIL.HS", kind: "dynomite", tables: ["Fossil"] }, { file: "TIMETRIAL.HS", kind: "dynomite", tables: ["Time Trial"], time: true }] },
    // games/bejeweledtwist/src/WinTwistApp.cpp, SaveScores: a long 9, then 20 entries (a short count and a UTF-8 name, a long
    // level, a long score), ten Classic and ten Blitz
    bejeweledtwist: { file: "users/scores.dat", kind: "twist", tables: ["Classic", "Blitz"], places: 10 },
    // games/astropop/src/HallOfFame.cpp: the hall of fame, two strings (product, version), then entries to the end (a string
    // name, i32 score, i32 second, i32 pilot, i32 kind): kind 1 Classic by score, kind 2 Survival by its seconds (m:ss)
    astropop: { file: "users/datahs.usr", kind: "astropop", tables: ["Classic", "Survival"] },
    // engines/mahjongescape/src/User.cpp: a player a file, <id>.usr, each with its best time and score of every classic layout
    // and escape level (kind "mahjong", parseMahjongPlayer); a classic layout's name is the NAME of its LAYOUTS resource in the
    // player's own Resource.dll (LayoutSelect.cpp ListLayoutId), so no layout's name is written here
    mahjongjapan: { kind: "mahjong", classic: 175, escape: 60, layouts: 162, layoutId: function (i) { return i < 125 ? 437 + i : 600 + (i - 125); } },
    mahjongchina: { kind: "mahjong", classic: 150, escape: 52, layouts: 126, layoutId: function (i) { return 437 + i; } },
    // games/cosmicbugs/src/CBMain.cpp and games/waterbugs/src/WBMain.cpp, LoadProfiles: cb.dat, exactly 49052 bytes, starts with
    // the eight high scores (a 32-byte name and an i32 score each)
    cosmicbugs: { file: "cb.dat", kind: "bugs", tables: ["High Scores"] },
    waterbugs: { file: "cb.dat", kind: "bugs", tables: ["High Scores"] },
    // games/typershark/src/UserProfileMgr.cpp: users/user.idx, framework strings (a u16 length and the bytes): the version (below
    // 0.48 the game takes its defaults), a number, then ten entries of the score table and ten of the abyss's, each the score, the
    // mode, the name and the level reached, the numbers as text
    typershark: { file: "users/user.idx", kind: "typershark", tables: ["High Scores", "Abyss"] },
    // The registry's (userdata/registry.ini, framework/SexyAppBase.cpp, RegistryPath): games/bookworm/src/BookWormApp.cpp,
    // LoadHighScores: the key Scores, four REG_MULTI_SZ values (name and score; name, word and score; name and word, its score
    // its length)
    bookworm: { file: "registry.ini", kind: "bookworm", key: "SOFTWARE\\PopCap\\BookWorm\\Scores",
      tables: [["Classic", "Classic"], ["Action", "Action"], ["Best", "Best Word"], ["Longest", "Longest Word"]] },
    // games/ningpo/src/NingPo.cpp, loadBestTimes: HS:<layout>x<rank>, 18 layouts of eight, 48 bytes each (an i32 0x300, a 32-byte
    // name, the i32 time with its penalties, the time played, the tiles left); an unplayed place is 5999 s with 144 tiles left, so
    // only played places are shown
    ningpo: { file: "registry.ini", kind: "ningpo", key: "SOFTWARE\\Pyrogon\\NingPo", layouts: 18, places: 8 },
    // games/wordharmony/game/HighScores.cpp, load (Game.cpp names the keys): scoreName0..9 and scoreValue0..9, a score of 0 skipped
    wordharmony: { file: "registry.ini", kind: "wordharmony", key: "SOFTWARE\\GameHouse\\WordHarmony\\scores",
      tables: [["normal", "Puzzle"], ["solitaire", "Solitaire"], ["action", "Action"]] },
    // games/bonnies/game/HighScores.cpp, load: Data/HighScores.dat, ten entries (an i32 used flag, the name and the longest word as
    // an i32 length and its bytes, an i32 score), five of Classic (Language 1194) then five of Action (1195), best first; both
    // rank points (the Bonnie's session, 2026-11-11)
    bonnies: { file: "Data/HighScores.dat", kind: "bonnies", tables: ["Classic", "Action"], places: 5 },
    // games/bookwormadv/src/Profile.cpp and games/bookwormadv2/src/Profile.cpp: a player a file, users/<name>.bwa, records of a key
    // and a typed value (kind "bwa", parseBwa); the Hall of Fame (HallOfFame.cpp) ranks every profile's TopWordsEverSpelled (a
    // word and its points, each word once a profile) and its TM_Top10Times (arena runs, by the stage reached, a time in
    // hundredths). The mini-games' tables are named and shown by the game's scripts, so they are left out
    bookwormadv: { kind: "bwa", tables: ["Top Words", "Arena"] },
    bookwormadv2: { kind: "bwa", tables: ["Top Words", "Arena"] },
    // games/tiptop/src/GameMain.cpp, LoadPrefs: Preferences.dat starts with four tables of five 28-byte entries (a 20-byte name,
    // an i32 score, an i32 level), by the game's mode: Regular, Timed (no button reaches it in this edition, so it is left out),
    // Speed, Strategy (the Tip Top session, 2026-11-12). A file shorter than 560 bytes is the empty tables; an empty place
    // scores 0 and is not shown
    tiptop: { file: "Preferences.dat", kind: "tiptop", tables: ["Regular", null, "Speed", "Strategy"] },
    // games/bejeweled2/src/HighScoreWidget.cpp, ReadFromFile
    bejeweled2: { file: "users/hiscores.dat", kind: "popcap", magic: 0xb9e7e3f8,
      tables: ["Classic", "Action", "Puzzle", "Endless", "Twilight", "Hyper", "Cognito", "Finity", "Hidden"], places: 10 }
  };
  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function make(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; }

  // ---- reading ----
  // a record of PopCap's family at at: {name, score, end}, or null past the end
  function popcapRecord(b, at, twice) {
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength);
    if (twice) { if (at + 2 > b.length) return null; at += 2; }
    if (at + 2 > b.length) return null;
    var n = dv.getUint16(at, true); at += 2;
    if (at + n + 8 > b.length) return null;
    var name = "", x = 0;
    for (var i = 0; i < n; i++) { var c = b[at + i]; name += String.fromCharCode(c); x ^= ((c << 24) >> 24) * 0x11; }
    at += n;
    var score = dv.getInt32(at, true), check = dv.getInt32(at + 4, true);
    if (check !== ((score ^ 0x42beef ^ x) | 0)) score = score % 256;
    return { name: decodeName(name), score: score, end: at + 8 };
  }
  // the names are the Windows code page's bytes
  function decodeName(s) {
    try { var a = new Uint8Array(s.length); for (var i = 0; i < s.length; i++) a[i] = s.charCodeAt(i); return new TextDecoder("windows-1252").decode(a); } catch (e) { return s; }
  }
  function today() { var d = new Date(), s = new Date(d.getFullYear(), 0, 0); return (d.getFullYear() - 1900) * 366 + Math.round((new Date(d.getFullYear(), d.getMonth(), d.getDate()) - s) / 86400000) - 1; }
  // Atomica's HS.CFG: a table is 208 bytes, five i32 scores, five 30-byte NUL-ended names, the day's best (i32 score, -1 for
  // none, its 30-byte name) and its day, ((month + year * 12) * 31 + day of the month) in UTC (HighScores.cpp, DateCode)
  function atomicaDay() { var d = new Date(); return ((d.getUTCMonth() + 1) + d.getUTCFullYear() * 12) * 31 + d.getUTCDate(); }
  function cString(b, at, n) { var s = ""; for (var i = 0; i < n && b[at + i]; i++) s += String.fromCharCode(b[at + i]); return decodeName(s); }
  function parseAtomica(spec, b) {
    var out = [], dv = new DataView(b.buffer, b.byteOffset, b.byteLength);
    for (var t = 0; t < spec.tables.length && (t + 1) * 208 <= b.length; t++) {
      var base = t * 208, rows = [];
      var best = dv.getInt32(base + 170, true), bestName = cString(b, base + 174, 30);
      if (best !== -1 && dv.getInt32(base + 204, true) === atomicaDay()) rows.push({ place: T("trophies.today"), name: bestName, score: best });
      for (var i = 0; i < 5; i++) rows.push({ place: String(i + 1), name: cString(b, base + 20 + i * 30, 30), score: dv.getInt32(base + i * 4, true) });
      out.push({ name: spec.tables[t], rows: rows });
    }
    return out;
  }
  function clock(sec) { sec = Math.floor(Math.abs(sec)); return Math.floor(sec / 60) + ":" + ("0" + sec % 60).slice(-2); }
  function utf8(b, at, n) { try { return new TextDecoder("utf-8").decode(b.subarray(at, at + n)); } catch (e) { return cString(b, at, n); } }
  // a framework Buffer string: a u16 length and its bytes; {s, end} or null past the end
  function lenString(b, dv, at, decode) {
    if (at + 2 > b.length) return null;
    var n = dv.getUint16(at, true);
    if (at + 2 + n > b.length) return null;
    return { s: decode(b, at + 2, n), end: at + 2 + n };
  }
  function parseDynomite(spec, b) {
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), rows = [];
    for (var i = 0; i < 5 && (i + 1) * 28 <= b.length; i++) {
      var v = dv.getInt32(i * 28 + 24, true), r = { place: String(i + 1), name: cString(b, i * 28 + 4, 20), score: v };
      if (spec.time) r.text = clock(v / 100);
      rows.push(r);
    }
    return [{ name: spec.tables[0], rows: rows }];
  }
  function parseTwist(spec, b) {
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), out = [], at = 4;
    if (b.length < 4 || dv.getInt32(0, true) !== 9) return [];
    for (var t = 0; t < spec.tables.length; t++) {
      var rows = [];
      for (var i = 0; i < spec.places; i++) {
        var n = lenString(b, dv, at, utf8);
        if (!n || n.end + 8 > b.length) break;
        rows.push({ place: String(i + 1), name: n.s, score: dv.getInt32(n.end + 4, true) });
        at = n.end + 8;
      }
      out.push({ name: spec.tables[t], rows: rows });
    }
    return out;
  }
  function parseAstropop(spec, b) {
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), at = 0, kinds = [[], []];
    for (var k = 0; k < 2; k++) { var h = lenString(b, dv, at, cString); if (!h) return []; at = h.end; }
    for (;;) {
      var n = lenString(b, dv, at, cString);
      if (!n || n.end + 16 > b.length) break;
      var score = dv.getInt32(n.end, true), sec = dv.getInt32(n.end + 4, true), kind = dv.getInt32(n.end + 12, true);
      at = n.end + 16;
      if (kind === 1) kinds[0].push({ name: n.s, score: score });
      else if (kind === 2) kinds[1].push({ name: n.s, score: sec, text: clock(sec) });
    }
    return kinds.map(function (rows, t) {
      return { name: spec.tables[t], rows: rows.map(function (r, i) { r.place = String(i + 1); return r; }) };
    });
  }
  // A Mahjong Escape player's file (big-endian): "ME", version 1, a u16 id, the NUL-ended name, 9 bytes of settings, the classic
  // layouts' records, a u16 and a byte, the escape levels' records. A record is a u16 best time in seconds (36000: none) and an
  // i32 best score; a layout or level is played when its time is not 36000. {name, classic: [...], escape: [...]} or null
  var MJ_NONE = 36000;
  function parseMahjongPlayer(spec, b) {
    if (b.length < 6 || b[0] !== 0x4d || b[1] !== 0x45 || b[2] !== 1) return null;
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), end = 5;
    while (end < b.length && b[end]) end++;
    var name = cString(b, 5, end - 5), at = end + 10;
    function records(n) {
      var out = [];
      for (var i = 0; i < n; i++, at += 6) {
        if (at + 6 > b.length) return null;
        out.push({ time: dv.getUint16(at, false), score: dv.getInt32(at + 2, false) });
      }
      return out;
    }
    var classic = records(spec.classic);
    if (!classic) return null;
    at += 3;
    var escape = records(spec.escape);
    return escape ? { name: name, classic: classic, escape: escape } : null;
  }
  function hms(sec) { return Math.floor(sec / 3600) + ":" + ("0" + Math.floor(sec / 60) % 60).slice(-2) + ":" + ("0" + sec % 60).slice(-2); }
  // a LAYOUTS resource's name: a u16 magic, a version byte, the NUL-ended name
  function layoutName(bytes) { return bytes && bytes.length > 3 ? cString(bytes, 3, bytes.length - 3) : ""; }
  // The players' records as tables: each classic layout someone has won, "<n>. <its name>", then each escape level won, the
  // players ranked by score, with their best times
  function mahjongTables(spec, players, names) {
    var out = [];
    function table(label, list, i) {
      var rows = players.filter(function (p) { return p[list][i] && p[list][i].time !== MJ_NONE; }).map(function (p) {
        return { name: p.name, score: p[list][i].score, time: hms(p[list][i].time) };
      });
      if (!rows.length) return;
      rows.sort(function (a, c) { return c.score - a.score; });
      rows.forEach(function (r, k) { r.place = String(k + 1); });
      out.push({ name: label, rows: rows });
    }
    for (var i = 0; i < spec.layouts; i++) table((i + 1) + ". " + (names[i] || ""), "classic", i);
    for (var e = 0; e < spec.escape; e++) table(T("trophies.escape", { n: e + 1 }), "escape", e);
    return out;
  }
  function parseBugs(spec, b) {
    if (b.length !== 0xbf9c) return [];
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), rows = [];
    for (var i = 0; i < 8; i++) rows.push({ place: String(i + 1), name: cString(b, i * 36, 32), score: dv.getInt32(i * 36 + 32, true) });
    return [{ name: spec.tables[0], rows: rows }];
  }
  function parseBonnies(spec, b) {
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), at = 0, out = [];
    function str() {
      if (at + 4 > b.length) return null;
      var n = dv.getInt32(at, true);
      if (n < 0 || at + 4 + n > b.length) return null;
      var r = cString(b, at + 4, n); at += 4 + n; return r;
    }
    for (var t = 0; t < spec.tables.length; t++) {
      var rows = [];
      for (var i = 0; i < spec.places; i++) {
        if (at + 4 > b.length) break;
        var used = dv.getInt32(at, true); at += 4;
        var name = str(), word = name === null ? null : str();
        if (word === null || at + 4 > b.length) break;
        var score = dv.getInt32(at, true); at += 4;
        if (used) rows.push({ place: String(i + 1), name: name, word: word, score: score });
      }
      out.push({ name: spec.tables[t], rows: rows });
    }
    return out;
  }
  // A BookWorm Adventures profile: records to the end, each a string key (u16 length and bytes) and a value: a u32 type (0 an
  // i32; 1 a float as a string of its text; 2 a byte; 3 a string; 4 a table; 5 nothing, an empty table's one entry), a table
  // being entries of a byte "another follows", a byte "the key is a string", the key (a string or an i32) and a value.
  // -> {key: value}, a table as an array of {key, value}; null when the file does not read to its end
  function parseBwa(b) {
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), at = 0;
    function need(n) { if (at + n > b.length) throw 0; }
    function str() { need(2); var n = dv.getUint16(at, true); need(2 + n); var r = cString(b, at + 2, n); at += 2 + n; return r; }
    function value(depth) {
      need(4); var type = dv.getUint32(at, true); at += 4;
      if (type === 0) { need(4); at += 4; return dv.getInt32(at - 4, true); }
      if (type === 1) return parseFloat(str());
      if (type === 2) { need(1); return !!b[at++]; }
      if (type === 3) return str();
      if (type === 5) return null;
      if (type !== 4 || depth > 16) throw 0;
      var list = [];
      for (;;) {
        need(2); var more = b[at], named = b[at + 1]; at += 2;
        var key;
        if (named) key = str(); else { need(4); key = dv.getInt32(at, true); at += 4; }
        list.push({ key: key, value: value(depth + 1) });
        if (!more) return list;
      }
    }
    var out = {};
    try { while (at < b.length) { var k = str(); out[k] = value(0); } } catch (e) { return null; }
    return out;
  }
  // a table's entries that are tables, each as {field: value}
  function bwaRows(t) {
    return Array.isArray(t) ? t.filter(function (e) { return Array.isArray(e.value); }).map(function (e) {
      var o = {}; e.value.forEach(function (f) { o[f.key] = f.value; }); return o;
    }) : [];
  }
  function hmsCents(t) { return hms(Math.floor(t / 100)); }
  // The Hall of Fame's tables from the players' profiles [{name, data}]: their top words, best first, each word once a player;
  // their arena runs, by the stage reached, then name
  function bwaTables(spec, players) {
    var words = [], runs = [];
    players.forEach(function (p) {
      var seen = {};
      bwaRows(p.data.TopWordsEverSpelled).forEach(function (r) {
        if (typeof r.mWord !== "string" || seen[r.mWord]) return;
        seen[r.mWord] = true;
        words.push({ name: p.name, word: r.mWord, score: r.mValue | 0 });
      });
      bwaRows(p.data.TM_Top10Times).forEach(function (r) { runs.push({ name: p.name, score: r.mStage | 0, time: hmsCents(r.mTime | 0) }); });
    });
    words.sort(function (a, c) { return c.score - a.score; });
    runs.sort(function (a, c) { return c.score - a.score || (a.name < c.name ? -1 : a.name > c.name ? 1 : 0); });
    return [[spec.tables[0], words], [spec.tables[1], runs]].filter(function (t) { return t[1].length; }).map(function (t) {
      t[1].forEach(function (r, k) { r.place = String(k + 1); });
      return { name: t[0], rows: t[1] };
    });
  }
  function parseTiptop(spec, b) {
    if (b.length < 0x230) return [];
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), out = [];
    spec.tables.forEach(function (name, t) {
      if (!name) return;
      var rows = [];
      for (var i = 0; i < 5; i++) {
        var at = (t * 5 + i) * 0x1c, score = dv.getInt32(at + 0x14, true);
        if (score) rows.push({ place: String(i + 1), name: cString(b, at, 0x14), score: score });
      }
      out.push({ name: name, rows: rows });
    });
    return out;
  }
  function parseTypershark(spec, b) {
    var dv = new DataView(b.buffer, b.byteOffset, b.byteLength), at = 0;
    function str() { var r = lenString(b, dv, at, cString); if (!r) return null; at = r.end; return r.s; }
    var version = str();
    if (version === null || !(parseFloat(version) >= 0.48) || str() === null) return [];
    var out = [];
    for (var t = 0; t < 2; t++) {
      var rows = [];
      for (var i = 0; i < 10; i++) {
        var score = str(), mode = str(), name = str(), level = str();
        if (level === null) break;
        rows.push({ place: String(i + 1), name: name, score: parseInt(score, 10) || 0 });
      }
      out.push({ name: spec.tables[t], rows: rows });
    }
    return out;
  }
  // The port's registry file: [key] sections of name=type:value lines (sz: text with \\\\, \\n and \\r escaped; dword: a signed
  // number; hex: the bytes) -> {lower-case key\\name: value}, a value as a string, a number or a Uint8Array
  function parseIni(b) {
    var text = "", map = {}, section = null;
    for (var i = 0; i < b.length; i++) text += String.fromCharCode(b[i]);
    text.split("\n").forEach(function (line) {
      if (line.charAt(line.length - 1) === "\r") line = line.slice(0, -1);
      if (!line || line.charAt(0) === ";") return;
      if (line.charAt(0) === "[") { var c = line.lastIndexOf("]"); section = line.slice(1, c < 0 ? undefined : c); return; }
      var eq = line.indexOf("=");
      if (eq < 0 || section === null) return;
      var v = line.slice(eq + 1), colon = v.indexOf(":"), type = v.slice(0, colon), data = v.slice(colon + 1), val;
      if (type === "dword") val = parseInt(data, 10) | 0;
      else if (type === "hex") { val = new Uint8Array(data.length >> 1); for (var k = 0; k < val.length; k++) val[k] = parseInt(data.substr(k * 2, 2), 16); }
      else if (type === "sz") val = decodeName(data.replace(/\\(.)/g, function (m, ch) { return ch === "n" ? "\n" : ch === "r" ? "\r" : ch; }));
      else return;
      map[(section + "\\" + line.slice(0, eq)).toLowerCase()] = val;
    });
    return map;
  }
  // a REG_MULTI_SZ's strings: each NUL-ended, the last one followed by one more NUL
  function multiString(bytes) {
    var out = [], s = "";
    for (var i = 0; i < bytes.length; i++) {
      if (bytes[i]) { s += String.fromCharCode(bytes[i]); continue; }
      if (!s) break;
      out.push(decodeName(s)); s = "";
    }
    return out;
  }
  function parseBookworm(spec, reg) {
    return spec.tables.map(function (t) {
      var v = reg[(spec.key + "\\" + t[0]).toLowerCase()], rows = [];
      var list = v instanceof Uint8Array ? multiString(v) : [], best = t[0] === "Best", longest = t[0] === "Longest";
      for (var i = 0; i < list.length; ) {
        var r = { name: list[i++], score: 0 };
        if (best || longest) r.word = i < list.length ? list[i++] : "";
        if (longest) r.score = r.word.length;
        else if (i < list.length) r.score = parseInt(list[i++], 10) || 0;
        rows.push(r);
      }
      // the game keeps each table sorted, best first, the rows of a score in the order they came
      rows.forEach(function (r, k) { r.at = k; });
      rows.sort(function (a, c) { return c.score - a.score || a.at - c.at; });
      rows.forEach(function (r, k) { r.place = String(k + 1); delete r.at; });
      return { name: t[1], rows: rows };
    }).filter(function (t) { return t.rows.length; });
  }
  function parseNingpo(spec, reg) {
    var out = [];
    for (var l = 0; l < spec.layouts; l++) {
      var rows = [];
      for (var r = 0; r < spec.places; r++) {
        var v = reg[(spec.key + "\\HS:" + l + "x" + r).toLowerCase()];
        if (!(v instanceof Uint8Array) || v.length !== 0x30) continue;
        var dv = new DataView(v.buffer, v.byteOffset, v.byteLength), time = dv.getInt32(0x24, true), tiles = dv.getInt32(0x2c, true);
        if (dv.getInt32(0, true) !== 0x300 || (time === 5999 && tiles === 144)) continue;
        rows.push({ place: String(r + 1), name: cString(v, 4, 32), score: time, text: clock(time) });
      }
      if (rows.length) out.push({ name: T("trophies.layout", { n: l + 1 }), rows: rows });
    }
    return out;
  }
  function parseWordharmony(spec, reg) {
    return spec.tables.map(function (t) {
      var rows = [], base = (spec.key + "\\" + t[0] + "\\").toLowerCase();
      for (var i = 0; i < 10; i++) {
        var score = reg[base + "scorevalue" + i];
        if (typeof score !== "number" || !score) continue;
        var name = reg[base + "scorename" + i];
        rows.push({ place: String(rows.length + 1), name: typeof name === "string" ? name.slice(0, 16) : "", score: score >>> 0 });
      }
      return { name: t[1], rows: rows };
    }).filter(function (t) { return t.rows.length; });
  }
  // the file's tables: [{name, rows: [{place, name, score, text?, time?, word?}]}] (text: a time shown in place of the number; time:
  // a best time, in a column of its own)
  function parse(spec, b) {
    if (spec.kind === "atomica") return parseAtomica(spec, b);
    if (spec.kind === "dynomite") return parseDynomite(spec, b);
    if (spec.kind === "twist") return parseTwist(spec, b);
    if (spec.kind === "astropop") return parseAstropop(spec, b);
    if (spec.kind === "bugs") return parseBugs(spec, b);
    if (spec.kind === "tiptop") return parseTiptop(spec, b);
    if (spec.kind === "bonnies") return parseBonnies(spec, b);
    if (spec.kind === "typershark") return parseTypershark(spec, b);
    if (spec.kind === "bookworm") return parseBookworm(spec, parseIni(b));
    if (spec.kind === "ningpo") return parseNingpo(spec, parseIni(b));
    if (spec.kind === "wordharmony") return parseWordharmony(spec, parseIni(b));
    var at = 0, out = [], dv = new DataView(b.buffer, b.byteOffset, b.byteLength);
    if (spec.magic !== undefined) { if (b.length < 4 || dv.getUint32(0, true) !== spec.magic) return []; at = 4; }
    for (var t = 0; t < spec.tables.length; t++) {
      var rows = [];
      if (spec.day) {
        if (at + 4 > b.length) break;
        var day = dv.getInt32(at, true); at += 4;
        var r0 = popcapRecord(b, at, spec.twice);
        if (!r0) break;
        at = r0.end;
        if (day === today() && r0.name) rows.push({ place: T("trophies.today"), name: r0.name, score: r0.score });
      }
      for (var i = 0; i < spec.places; i++) {
        var r = popcapRecord(b, at, spec.twice);
        if (!r) break;
        at = r.end;
        rows.push({ place: String(i + 1), name: r.name, score: r.score });
      }
      out.push({ name: spec.tables[t], rows: rows });
    }
    return out;
  }
  function readUser(game, rel) {
    var F = ByopopMusic.files;
    return F.open(game.persist).then(function (db) {
      if (!db) return null;
      return F.read(db, game.persist + "/userdata/" + rel).then(function (b) { db.close(); return b || null; }, function () { db.close(); return null; });
    });
  }

  // the keys of the files under dir whose names after it match re
  function userKeys(db, dir, re) {
    return new Promise(function (resolve) {
      try {
        var st = db.objectStoreNames[0], r = db.transaction(st, "readonly").objectStore(st).getAllKeys(IDBKeyRange.bound(dir, dir + "￿"));
        r.onsuccess = function () { resolve(r.result.map(String).filter(function (k) { return re.test(k.substring(dir.length)); })); };
        r.onerror = function () { resolve([]); };
      } catch (e) { resolve([]); }
    });
  }
  // BookWorm Adventures' tables: its players' profiles (userdata/users/<name>.bwa), each named by its file
  function readBwa(game, spec) {
    var F = ByopopMusic.files;
    return F.open(game.persist).then(function (db) {
      if (!db) return [];
      var dir = game.persist + "/userdata/users/";
      return userKeys(db, dir, /^[^\/]+\.bwa$/i).then(function (list) {
        return Promise.all(list.map(function (k) {
          return F.read(db, k).then(function (b) {
            var data = b ? parseBwa(b) : null;
            return data ? { name: decodeName(k.substring(dir.length, k.length - 4)), data: data } : null;
          }, function () { return null; });
        }));
      }).then(function (players) { db.close(); return bwaTables(spec, players.filter(Boolean)); }, function () { db.close(); return []; });
    });
  }
  // a Mahjong Escape's tables: its players' files (userdata/<id>.usr) and its layouts' names from the player's Resource.dll
  function readMahjong(game, spec) {
    var F = ByopopMusic.files;
    return F.open(game.persist).then(function (db) {
      if (!db) return [];
      var user = game.persist + "/userdata/";
      var keys = userKeys(db, user, /^\d+\.usr$/i);
      return keys.then(function (list) {
        return Promise.all(list.map(function (k) { return F.read(db, k).then(function (b) { return b ? parseMahjongPlayer(spec, b) : null; }, function () { return null; }); }));
      }).then(function (players) {
        players = players.filter(Boolean);
        if (!players.length) return [];
        return F.list(db, game.persist).then(function (rels) {
          var dll = rels.filter(function (r) { return /(^|\/)resource\.dll$/i.test(r); })[0];
          if (!dll) return [];
          return Promise.all(Array.from({ length: spec.layouts }, function (_, i) {
            return F.read(db, game.persist + "/game/" + dll + "|pe:LAYOUTS:" + spec.layoutId(i)).then(layoutName, function () { return ""; });
          }));
        }, function () { return []; }).then(function (names) { return mahjongTables(spec, players, names); });
      }).then(function (t) { db.close(); return t; }, function () { db.close(); return []; });
    });
  }

  // ---- the window ----
  var win = null;
  function build(games) {
    var el = make("section", "tc-win");
    el.id = "trophies";
    var bar = make("div", "tc-bar");
    var lab = make("label", "", T("trophies.player"));
    var pick = make("select");
    lab.appendChild(pick); bar.appendChild(lab);
    var body = make("div", "tc-body");
    var status = make("div", "tc-status d95-statusbar");
    el.appendChild(bar); el.appendChild(body); el.appendChild(status);
    var found = [];
    function players() {
      var was = pick.value;
      pick.textContent = "";
      var all = make("option", "", T("trophies.everyone")); all.value = ""; pick.appendChild(all);
      try { ByopopProfiles.list().forEach(function (p) { var o = make("option", "", p.name); o.value = p.name; pick.appendChild(o); }); } catch (e) {}
      pick.value = was && Array.prototype.some.call(pick.options, function (o) { return o.value === was; }) ? was : (function () {
        try { var c = ByopopProfiles.current(); return c ? c.name : ""; } catch (e) { return ""; }
      })();
    }
    function draw() {
      body.textContent = "";
      var who = pick.value.toLowerCase(), shown = 0;
      found.forEach(function (g) {
        var tables = g.tables.map(function (t) {
          return { name: t.name, rows: t.rows.filter(function (r) { return !who || r.name.toLowerCase() === who; }) };
        }).filter(function (t) { return t.rows.length; });
        if (!tables.length) return;
        shown++;
        var box = make("fieldset", "tc-game");
        box.appendChild(make("legend", "", g.title));
        var table = make("table", "tc-table");
        var head = make("tr", "");
        // a Time column where the game keeps best times beside its scores (the Mahjong Escapes)
        var timed = tables.some(function (t) { return t.rows.some(function (r) { return r.time !== undefined; }); });
        // a Word column where the game keeps a word beside the name (BookWorm's best and longest words, Bonnie's longest, BookWorm Adventures' top words)
        var worded = tables.some(function (t) { return t.rows.some(function (r) { return r.word !== undefined; }); });
        [T("trophies.col.table"), T("trophies.col.place"), T("trophies.col.name")].concat(worded ? [T("trophies.col.word")] : [], [T("trophies.col.score")], timed ? [T("trophies.col.time")] : []).forEach(function (h) { var th = make("th", "", h); th.scope = "col"; head.appendChild(th); });
        table.appendChild(head);
        tables.forEach(function (t) {
          t.rows.forEach(function (r, i) {
            var tr = make("tr", i === 0 ? "tc-first" : "");
            tr.appendChild(make("td", "", i === 0 ? t.name : ""));
            tr.appendChild(make("td", "tc-num", r.place));
            tr.appendChild(make("td", "", r.name));
            if (worded) tr.appendChild(make("td", "", r.word || ""));
            tr.appendChild(make("td", "tc-num", r.text || r.score.toLocaleString()));
            if (timed) tr.appendChild(make("td", "tc-num", r.time || ""));
            table.appendChild(tr);
          });
        });
        box.appendChild(table);
        body.appendChild(box);
      });
      status.textContent = T("trophies.count", { n: shown });
    }
    function scan() {
      players();
      found = [];
      // a game whose tables are in several files (spec.files) reads each, its tables in the files' order
      var reads = games.filter(function (g) { return RECORDS[g.id]; }).map(function (g) {
        if (RECORDS[g.id].kind === "bwa") return readBwa(g, RECORDS[g.id]).then(function (tables) {
          if (tables.length) found.push({ id: g.id, title: g.title, tables: tables });
        }, function () {});
        if (RECORDS[g.id].kind === "mahjong") return readMahjong(g, RECORDS[g.id]).then(function (tables) {
          if (tables.length) found.push({ id: g.id, title: g.title, tables: tables });
        }, function () {});
        var specs = RECORDS[g.id].files || [RECORDS[g.id]];
        return Promise.all(specs.map(function (spec) {
          return readUser(g, spec.file).then(function (b) { return b ? parse(spec, b) : []; }, function () { return []; });
        })).then(function (lists) {
          var tables = [].concat.apply([], lists);
          if (tables.length) found.push({ id: g.id, title: g.title, tables: tables });
        });
      });
      return Promise.all(reads).then(function () {
        var order = games.map(function (g) { return g.id; });
        found.sort(function (a, b) { return order.indexOf(a.id) - order.indexOf(b.id); });
        draw();
      });
    }
    pick.addEventListener("change", draw);
    return { el: el, scan: scan };
  }
  function attach(desk, at) {
    if (win || !document.getElementById("games-title") || typeof ByopopMusic === "undefined") return null;
    var games = Array.prototype.map.call(document.querySelectorAll("a.tile[data-game]"), function (t) {
      return { id: t.getAttribute("data-game"), title: t.querySelector(".name").textContent, persist: t.getAttribute("data-persist") };
    });
    win = build(games);
    var foot = document.querySelector("body > footer");
    if (foot && foot.parentNode) foot.parentNode.insertBefore(win.el, foot); else document.body.appendChild(win.el);
    // its icon: a gold cup on a stand, our own drawing in 32 x 32
    var pic = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    pic.setAttribute("viewBox", "0 0 32 32"); pic.setAttribute("aria-hidden", "true"); pic.setAttribute("shape-rendering", "crispEdges");
    pic.innerHTML = '<path d="M9 4h14v9c0 4-3 7-7 7s-7-3-7-7z" fill="#ffff00" stroke="#000"/><path d="M9 7H5v3c0 3 2 5 4 5M23 7h4v3c0 3-2 5-4 5" fill="none" stroke="#000"/>' +
      '<path d="M14 20h4v5h-4z" fill="#808000" stroke="#000"/><path d="M9 25h14v4H9z" fill="#808000" stroke="#000"/><path d="M12 6h2v8h-2z" fill="#ffffff"/>';
    desk.add(win.el, { id: "trophies", title: T("trophies.title"), closable: true, closed: true, pic: pic, at: at, scroll: ".tc-body" });
    var open = false;
    function watch() {
      var now = !win.el.classList.contains("d95-closed") && document.documentElement.classList.contains("d95-desk");
      if (now === open) return;
      open = now;
      if (open) win.scan();
    }
    if (window.MutationObserver) {
      new MutationObserver(watch).observe(win.el, { attributes: true, attributeFilter: ["class"] });
      new MutationObserver(watch).observe(document.documentElement, { attributes: true, attributeFilter: ["class"] });
    }
    watch();
    return win;
  }
  return { attach: attach, parse: parse, parseIni: parseIni, parseBwa: parseBwa, bwaTables: bwaTables, parseMahjongPlayer: parseMahjongPlayer, mahjongTables: mahjongTables, layoutName: layoutName, RECORDS: RECORDS };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopTrophies;
