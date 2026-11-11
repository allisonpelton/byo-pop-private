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
  // the file's tables: [{name, rows: [{place, name, score}]}]
  function parse(spec, b) {
    if (spec.kind === "atomica") return parseAtomica(spec, b);
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
        [T("trophies.col.table"), T("trophies.col.place"), T("trophies.col.name"), T("trophies.col.score")].forEach(function (h) { var th = make("th", "", h); th.scope = "col"; head.appendChild(th); });
        table.appendChild(head);
        tables.forEach(function (t) {
          t.rows.forEach(function (r, i) {
            var tr = make("tr", i === 0 ? "tc-first" : "");
            tr.appendChild(make("td", "", i === 0 ? t.name : ""));
            tr.appendChild(make("td", "tc-num", r.place));
            tr.appendChild(make("td", "", r.name));
            tr.appendChild(make("td", "tc-num", r.score.toLocaleString()));
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
      var reads = games.filter(function (g) { return RECORDS[g.id]; }).map(function (g) {
        return readUser(g, RECORDS[g.id].file).then(function (b) {
          if (!b) return;
          var tables = parse(RECORDS[g.id], b);
          if (tables.length) found.push({ id: g.id, title: g.title, tables: tables });
        }, function () {});
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
  return { attach: attach, parse: parse, RECORDS: RECORDS };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopTrophies;
