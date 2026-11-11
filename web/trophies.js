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
  // the file's tables: [{name, rows: [{place, name, score, text?, time?}]}] (text: a time shown in place of the number; time:
  // a best time, in a column of its own)
  function parse(spec, b) {
    if (spec.kind === "atomica") return parseAtomica(spec, b);
    if (spec.kind === "dynomite") return parseDynomite(spec, b);
    if (spec.kind === "twist") return parseTwist(spec, b);
    if (spec.kind === "astropop") return parseAstropop(spec, b);
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

  // a Mahjong Escape's tables: its players' files (userdata/<id>.usr) and its layouts' names from the player's Resource.dll
  function readMahjong(game, spec) {
    var F = ByopopMusic.files;
    return F.open(game.persist).then(function (db) {
      if (!db) return [];
      var user = game.persist + "/userdata/";
      var keys = new Promise(function (resolve) {
        try {
          var st = db.objectStoreNames[0], r = db.transaction(st, "readonly").objectStore(st).getAllKeys(IDBKeyRange.bound(user, user + "￿"));
          r.onsuccess = function () { resolve(r.result.map(String).filter(function (k) { return /^\d+\.usr$/i.test(k.substring(user.length)); })); };
          r.onerror = function () { resolve([]); };
        } catch (e) { resolve([]); }
      });
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
        [T("trophies.col.table"), T("trophies.col.place"), T("trophies.col.name"), T("trophies.col.score")].concat(timed ? [T("trophies.col.time")] : []).forEach(function (h) { var th = make("th", "", h); th.scope = "col"; head.appendChild(th); });
        table.appendChild(head);
        tables.forEach(function (t) {
          t.rows.forEach(function (r, i) {
            var tr = make("tr", i === 0 ? "tc-first" : "");
            tr.appendChild(make("td", "", i === 0 ? t.name : ""));
            tr.appendChild(make("td", "tc-num", r.place));
            tr.appendChild(make("td", "", r.name));
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
  return { attach: attach, parse: parse, parseMahjongPlayer: parseMahjongPlayer, mahjongTables: mahjongTables, layoutName: layoutName, RECORDS: RECORDS };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopTrophies;
