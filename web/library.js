// The Library (docs/WEB_PLAYER.md, "The Library"): Desktop 95's window of the games, on the listing's desktop, open at first.
// It took in All games (the coordinator's list of 2026-11-22: "Add all the features from All games to Library and then get rid
// of the all games window"). It evokes a game client of 2004 in its own look (web/skins.css, .lib-*: olive and khaki, flat panels
// with 1-pixel bevels, small type, square buttons, its own title bar), drawn by us and named for no real program. It is a grid
// of the games, one row each: a 16-pixel icon and the name, the status, the plays and play time here, the last day played,
// the year and, once the site's play counter answers, every player's plays; a click on a column's header sorts by it, again the
// other way (kept in localStorage "byopop-library-sort"). A double-click or Enter plays the selected game, its menu (a right
// click, the menu key or Shift+F10) has Play, Add files, Uninstall, the favourite, Properties, why a game's files are gone and
// where to buy it. Above the grid sit Play, Add games, "Show games no longer sold" and a Filter box; under it what became of the
// folders added and the recycle bin's panel (web/addgames.js) when they are open; in its status bar the count, how many games
// "Windows 95 games only" hides, and Jungle Juice. A row of a game whose files are here can be dragged onto the desktop's
// Recycle Bin, and a game's folder dropped on its row is added to that game. Everything it shows comes from the listing's own
// cards (their files check, favourite and buy links: web/index.html's games, built by cmake/ByopopWeb.cmake and kept out of
// sight on the desktop) and ByopopPlay (web/settings.js).
var ByopopLibrary = (function () {
  "use strict";
  var KEY = "byopop-library-sort";
  // the old All games' sort (web/index.html until 2026-11-23), read once when the Library has none of its own
  var OLD_SORT_KEY = "byopop-listing-sort", OLD_SORT = { name: ["name", 1], release: ["year", 1], time: ["time", -1], plays: ["plays", -1], popular: ["popular", -1] };
  // "Show games no longer sold" (the user, 2026-11-03: "add an option to hide games that can no longer be purchased, and
  // default to hiding those games from the front page"): "show", or nothing for the default; All games' own key
  var UNSOLD_KEY = "byopop-listing-unsold";
  var COLS = ["name", "status", "plays", "time", "last", "year"], ALL_COLS = COLS.concat(["phase", "popular"]);
  // on a development host, each game's phase after its status (web/phase.js)
  try { if (ByopopPhase.on()) COLS.splice(2, 0, "phase"); } catch (e) {}
  var STATUS_ORDER = { ready: 0, unknown: 1, files: 2, unsold: 3, soon: 4 };
  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function make(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  function load() {
    try { var v = JSON.parse(localStorage.getItem(KEY) || "null"); if (v && ALL_COLS.indexOf(v.col) >= 0) return { col: v.col, dir: v.dir === -1 ? -1 : 1 }; } catch (e) {}
    try { var o = OLD_SORT[localStorage.getItem(OLD_SORT_KEY)]; if (o) return { col: o[0], dir: o[1] }; } catch (e) {}
    return { col: "name", dir: 1 };
  }
  function keep(s) { try { localStorage.setItem(KEY, JSON.stringify(s)); localStorage.removeItem(OLD_SORT_KEY); } catch (e) {} }
  function unsoldShown() { try { return localStorage.getItem(UNSOLD_KEY) === "show"; } catch (e) { return false; } }

  // ---- the games, as the listing's cards have them ----
  // { games: the rows to show, hidden95: how many "Windows 95 games only" leaves out, unsold: whether any game is no longer sold }
  function readGames() {
    var play = {}, rec = {};
    try { play = ByopopPlay.all(); } catch (e) {}
    try { rec = ByopopGameFiles.all(); } catch (e) {}
    var hide95 = document.documentElement.classList.contains("d95-win95only"), showUnsold = unsoldShown(), hidden95 = 0, anyUnsold = false;
    var games = Array.prototype.map.call(document.querySelectorAll("ul.games > li"), function (li) {
      // a game still to come has a card with no link (div.tile): its title is its id here
      var tile = li.querySelector(".tile");
      if (!tile || li.hidden) return null;
      var files = li.getAttribute("data-files"), unsold = li.hasAttribute("data-unsold");
      if (unsold) anyUnsold = true;
      // a game no store sells is shown when asked for, or when its files are here (a player who has them can play it)
      if (unsold && files !== "yes" && !showUnsold) return null;
      if (hide95 && li.getAttribute("data-win95") !== "yes") { hidden95++; return null; }
      var id = tile.getAttribute("data-game") || "soon:" + (li.getAttribute("data-title") || "");
      var status = li.classList.contains("soon") ? "soon" : files === "yes" ? "ready" : unsold ? "unsold" : files === "unknown" ? "unknown" : "files";
      var p = play[id] || {}, r = rec[id], icon = "";
      try { icon = r && ByopopGameFiles.present(r) ? (r.small || r.icon || "") : ""; } catch (e) {}
      var badge = tile.querySelector(".badge");
      var released = li.getAttribute("data-released") || "";
      // where to buy it, as its card shows it: not to a player whose files for it are or were here, or who played it here
      // (web/index.html, show: the links would suggest the site is used for copies it was not given by a purchase)
      var buyRow = li.querySelector(".buyrow");
      var buy = buyRow && !buyRow.hidden ? Array.prototype.filter.call(buyRow.querySelectorAll("a[href]"), function (a) { return /^https?:/.test(a.href) && a.host !== location.host; }) : [];
      return { id: id, li: li, tile: tile, href: tile.href || "", title: li.getAttribute("data-title") || id, status: status,
        seconds: p.seconds > 0 ? p.seconds : 0, plays: p.plays > 0 ? p.plays : 0, first: p.first || "", last: p.last || "", by: p.by || {}, released: released,
        year: /^\d{4}/.test(released) ? +released.slice(0, 4) : 0, icon: icon,
        tag: badge ? (badge.getAttribute("data-initials") || badge.textContent) : "", colour: li.style.getPropertyValue("--tile") || "",
        fav: li.querySelector("button.fav"), buy: buy, why: !!li.querySelector("button.why"),
        win95: li.getAttribute("data-win95") || "unknown", phase: phaseOf(tile.getAttribute("data-game")) };
    }).filter(Boolean);
    return { games: games, hidden95: hidden95, unsold: anyUnsold };
  }
  function phaseOf(id) { try { return (id && ByopopPhase.of(id)) || null; } catch (e) { return null; } }
  function lastText(day) {
    var m = /^(\d{4})-(\d{2})-(\d{2})$/.exec(day || "");
    if (!m) return "";
    try { return new Date(+m[1], +m[2] - 1, +m[3]).toLocaleDateString(undefined, { year: "numeric", month: "numeric", day: "numeric" }); } catch (e) { return day; }
  }
  function timeText(s) { if (!(s > 0)) return ""; try { return ByopopPlay.duration(s); } catch (e) { return Math.round(s / 60) + " min"; } }
  function countText(n) { try { return n.toLocaleString(); } catch (e) { return String(n); } }
  function statusText(s) {
    return T({ ready: "library.status.ready", files: "library.status.files", soon: "library.status.soon", unsold: "library.status.unsold",
      unknown: "library.status.unknown" }[s]);
  }
  // Every player's counts from the site's play counter (web/playcount.js; docs/PLAY_COUNTER.md), {id: {plays, live}}, once it
  // answers: without a counter in the build, or before it answers, there is no Popular column
  var site = null;
  function popular(g) { return (site && site[g.id]) || { plays: 0, live: 0 }; }
  function popularTip(c) {
    var plays = T("index.counts.plays", { n: c.plays });
    return c.live > 0 ? T("index.counts.both", { live: T("index.counts.live", { n: c.live }), plays: plays }) : plays;
  }
  function compare(col) {
    return function (a, b) {
      switch (col) {
        case "status": return STATUS_ORDER[a.status] - STATUS_ORDER[b.status];
        case "phase": return (a.phase ? a.phase.rank : 99) - (b.phase ? b.phase.rank : 99);
        case "plays": return (a.plays - b.plays) || (a.seconds - b.seconds);
        case "time": return (a.seconds - b.seconds) || (a.plays - b.plays);
        case "last": return (a.last < b.last ? -1 : a.last > b.last ? 1 : 0);
        case "year": return (a.released < b.released ? -1 : a.released > b.released ? 1 : 0);
        case "popular": var x = popular(a), y = popular(b); return (x.plays - y.plays) || (x.live - y.live);
      }
      return 0;
    };
  }
  function byName(a, b) { return a.title.toLowerCase().replace(/ deluxe/g, "").localeCompare(b.title.toLowerCase().replace(/ deluxe/g, "")); }

  // Filter, as All games' Find was: each word typed is looked for in each game's title, letters and digits only, with a few
  // typing slips allowed ("mumy maz", "bejewled"): the fewest edits that turn the word into some stretch of the title, at most one
  // for a word of four to six letters, two up to nine, three beyond, none for a shorter one (whose slip would find too many). A
  // game matches when every word does, in any order.
  function plain(s) { return String(s || "").toLowerCase().normalize("NFD").replace(/[̀-ͯ]/g, "").replace(/[^a-z0-9]/g, ""); }
  // the fewest edits that turn q into a stretch of t (an approximate substring search: a stretch may start anywhere for free)
  function edits(q, t) {
    var prev = [], cur, i, j;
    for (j = 0; j <= t.length; j++) prev.push(0);
    for (i = 1; i <= q.length; i++) {
      cur = [i];
      for (j = 1; j <= t.length; j++)
        cur.push(Math.min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (q.charAt(i - 1) === t.charAt(j - 1) ? 0 : 1)));
      prev = cur;
    }
    return Math.min.apply(null, prev);
  }
  function slips(n) { return n < 4 ? 0 : n < 7 ? 1 : n < 10 ? 2 : 3; }
  function matches(words, title) {
    var t = plain(title);
    return words.every(function (w) { return edits(w, t) <= slips(w.length); });
  }

  // ---- the window ----
  function build() {
    var el = make("section", "lib");
    el.id = "library";
    el.setAttribute("aria-label", T("library.title"));
    var bar = make("div", "lib-bar");
    var play = make("button", "lib-btn lib-play", T("library.play"));
    play.type = "button";
    // Add games (web/addgames.js: the Setup window); its id is the one the adding panel's Close gives the focus back to
    var add = make("button", "lib-btn lib-add", T("library.add"));
    add.type = "button"; add.id = "add-games";
    var unsold = make("label", "lib-unsold"), unsoldBox = make("input");
    unsoldBox.type = "checkbox";
    unsold.appendChild(unsoldBox); unsold.appendChild(document.createTextNode(" " + T("library.unsold")));
    unsold.hidden = true;
    var flabel = make("label", "lib-flabel", T("library.filter"));
    var filter = make("input", "lib-filter");
    filter.type = "search"; filter.id = "lib-filter"; filter.autocomplete = "off"; filter.spellcheck = false;
    flabel.htmlFor = filter.id;
    // the label and its box wrap to a row of their own together, at the right
    var find = make("span", "lib-find");
    find.appendChild(flabel); find.appendChild(filter);
    bar.appendChild(play); bar.appendChild(add); bar.appendChild(unsold); bar.appendChild(find);
    var area = make("div", "lib-area");
    // where the rows are scrolled to, as the area's own scrolls last said: fill() reads this, since reading the area there, in
    // the middle of the listing's own changes, made the browser lay the whole page out at once (a forced reflow as the page
    // loads); nothing is scrolled until the player scrolls
    var scrolled = 0;
    area.addEventListener("scroll", function () { scrolled = area.scrollTop; });
    var grid = make("table", "lib-grid");
    grid.setAttribute("role", "grid");
    grid.setAttribute("aria-label", T("library.title"));
    grid.setAttribute("aria-readonly", "true");
    // (a table with the grid role: its rows, headers and cells are the grid's own by their elements, the name a row's header)
    var head = make("thead"), hr = make("tr");
    head.appendChild(hr);
    var body = make("tbody");
    grid.appendChild(head); grid.appendChild(body);
    area.appendChild(grid);
    var foot = make("footer", "lib-status");
    var count = make("span", "lib-count"), hid95 = make("span", "lib-hidden"), said = make("span", "visually-hidden");
    hid95.hidden = true;
    hid95.title = T("skin.win95.hidden_title");
    said.setAttribute("role", "status"); said.setAttribute("aria-live", "polite");
    // Jungle Juice, the page for other projects' decompilations (web/jungle/index.html; the user, 2026-11-04: "the front page
    // will link to it after the game tiles")
    var jungle = make("span", "lib-jungle"), jl = make("a", "", T("index.jungle.name"));
    jl.href = "jungle/";
    jungle.appendChild(jl); jungle.appendChild(make("span", "lib-jungle-about", T("index.jungle.about")));
    foot.appendChild(count); foot.appendChild(hid95); foot.appendChild(said); foot.appendChild(jungle);
    el.appendChild(bar); el.appendChild(area); el.appendChild(foot);

    var sort = load(), rows = [], games = [], sel = null, typed = "", typedAt = 0, heads = {}, cols = COLS.slice();
    // the column headers, again when the Popular column comes
    function header() {
      hr.textContent = "";
      heads = {};
      cols.forEach(function (c) {
        var th = make("th", "lib-h lib-c-" + c);
        th.scope = "col";
        var b = make("button", "lib-hb", T("library.col." + c));
        b.type = "button";
        b.addEventListener("click", function () { sortBy(c, true); });
        th.appendChild(b);
        hr.appendChild(th);
        heads[c] = th;
      });
    }
    header();
    function order() {
      var cmp = compare(cols.indexOf(sort.col) >= 0 ? sort.col : "name");
      return games.slice().sort(function (a, b) { return (sort.col === "name" ? 0 : cmp(a, b) * sort.dir) || byName(a, b) * (sort.col === "name" ? sort.dir : 1); });
    }
    function visible() { return rows.filter(function (r) { return !r.tr.hidden; }); }
    // jump: bring the selected row into view (a sort); a refresh from the cards or the storage keeps the list where the player
    // scrolled it (the user, 2026-11-10: "Scroll keeps snapping back in the library window to the selected game")
    function fill(jump) {
      var keepId = sel ? sel.g.id : null, keepTop = scrolled, hadFocus = !!(sel && document.activeElement === sel.cell);
      var read = readGames();
      games = read.games;
      unsold.hidden = !read.unsold;
      unsoldBox.checked = unsoldShown();
      hid95.hidden = !read.hidden95;
      hid95.textContent = read.hidden95 ? T("skin.win95.hidden", { count: read.hidden95 }) : "";
      body.textContent = "";
      rows = order().map(function (g) {
        var tr = make("tr", "lib-row lib-s-" + g.status);
        tr.setAttribute("aria-selected", "false");
        var name = make("th", "lib-c-name");
        name.scope = "row"; name.tabIndex = -1;
        var ic = make("i", "lib-icon");
        ic.setAttribute("aria-hidden", "true");
        if (g.icon) { var im = make("img"); im.alt = ""; im.draggable = false; im.src = g.icon; ic.appendChild(im); }
        else { ic.classList.add("lib-icon-cap"); ic.style.setProperty("--tile", g.colour || "#808080"); ic.textContent = (g.tag || "").slice(0, 2); }
        var wrap = make("span", "lib-name");
        wrap.appendChild(ic); wrap.appendChild(make("span", "", g.title));
        name.appendChild(wrap);
        tr.appendChild(name);
        var c = popular(g);
        [["status", statusText(g.status)], ["phase", g.phase ? g.phase.text : ""], ["plays", g.plays ? countText(g.plays) : ""],
         ["time", timeText(g.seconds)], ["last", lastText(g.last)], ["year", g.year ? String(g.year) : ""],
         ["popular", c.plays || c.live ? countText(c.plays) : ""]].forEach(function (v) {
          if (cols.indexOf(v[0]) < 0) return;
          var td = make("td", "lib-c-" + v[0], v[1]);
          if (v[0] === "phase" && g.phase) td.title = g.phase.tip;
          if (v[0] === "popular" && (c.plays || c.live)) td.title = popularTip(c);
          tr.appendChild(td);
        });
        var r = { tr: tr, cell: name, g: g };
        // a game whose files are here can be dragged onto the desktop's Recycle Bin (web/skins.js), which asks to forget them
        var drag = g.status === "ready" && !!(window.ByopopAddGames && ByopopAddGames.has && ByopopAddGames.has(g.id));
        if (drag) {
          tr.draggable = true;
          tr.addEventListener("dragstart", function (ev) {
            ev.dataTransfer.setData(ByopopAddGames.CAP_TYPE, g.id);
            ev.dataTransfer.effectAllowed = "move";
          });
        }
        // (the default would move the focus off the row's first cell again when another cell was pressed, and the keys with it;
        // a row that can be dragged keeps the default, which starts the drag, and takes the focus back on the click)
        tr.addEventListener("mousedown", function (ev) { if (ev.button !== 1) { if (!drag) ev.preventDefault(); select(r, true); } });
        if (drag) tr.addEventListener("click", function () { if (sel === r && document.activeElement !== r.cell) select(r, true, true); });
        tr.addEventListener("dblclick", function () { act(r, "play"); });
        tr.addEventListener("contextmenu", function (ev) { ev.preventDefault(); select(r, true); menu(r, ev.clientX, ev.clientY); });
        body.appendChild(tr);
        return r;
      });
      Object.keys(heads).forEach(function (c) { heads[c].setAttribute("aria-sort", c === sort.col ? (sort.dir > 0 ? "ascending" : "descending") : "none"); });
      applyFilter();
      var again = rows.filter(function (r) { return r.g.id === keepId && !r.tr.hidden; })[0] || visible()[0] || null;
      select(again, hadFocus, jump !== true);
      // (rows scrolled back where they were; at the top, as at first, they are already there)
      if (jump !== true && keepTop) area.scrollTop = keepTop;
    }
    function applyFilter() {
      var words = filter.value.split(/\s+/).map(plain).filter(Boolean);
      rows.forEach(function (r) { r.tr.hidden = !!words.length && !matches(words, r.g.title); });
      count.textContent = T("library.count", { n: visible().length });
      if (sel && sel.tr.hidden) select(visible()[0] || null, false);
    }
    function select(r, focus, stay) {
      if (sel && sel !== r) { sel.tr.setAttribute("aria-selected", "false"); sel.cell.tabIndex = -1; }
      sel = r;
      if (r) {
        r.tr.setAttribute("aria-selected", "true"); r.cell.tabIndex = 0;
        if (focus) { try { r.cell.focus({ preventScroll: true }); } catch (e) {} }
        if (!stay) reveal(r.tr);
      }
      play.disabled = !r || r.g.status === "soon";
    }
    // the row in view below the column headers, which stay at the top as the rows scroll
    function reveal(tr) {
      var ar = area.getBoundingClientRect(), r = tr.getBoundingClientRect(), top = ar.top + area.clientTop + head.offsetHeight;
      var bottom = ar.top + area.clientTop + area.clientHeight;
      if (r.top < top) area.scrollTop -= top - r.top;
      else if (r.bottom > bottom) area.scrollTop += r.bottom - bottom;
    }
    function sortBy(c, say) {
      // the counts and the times sort most first at the first click, the rest from the start of the alphabet or the calendar
      var first = c === "plays" || c === "time" || c === "popular" ? -1 : 1;
      sort = { col: c, dir: sort.col === c ? -sort.dir : first };
      keep(sort);
      fill(true);
      if (say) said.textContent = T("library.sorted", { col: T("library.col." + c), dir: T(sort.dir > 0 ? "library.asc" : "library.desc") });
    }
    function act(r, what) {
      if (!r) return;
      var g = r.g;
      if (what === "play" || what === "files") {
        if (g.status === "soon") return;
        try { if (window.ByopopApps && ByopopApps.open(g.href, "push")) return; } catch (e) {}
        location.href = g.href;
      } else if (what === "fav") { if (g.fav) g.fav.click(); setTimeout(function () { fill(); }, 0); }
      else if (what === "props") props(g);
    }
    // the focus back on the selected row after a box of the menu's (the rows may have been drawn again meanwhile)
    function back() { if (sel && document.body.contains(sel.cell)) select(sel, true, true); }
    function menu(r, x, y) {
      var g = r.g, fav = g.fav && g.fav.getAttribute("aria-pressed") === "true";
      var entries = [
        { label: T("library.menu.play"), bold: true, disabled: g.status !== "ready", fn: function () { act(r, "play"); } },
        { label: T("library.menu.files"), disabled: g.status === "ready" || g.status === "soon", fn: function () { act(r, "files"); } },
        { rule: true },
        // Uninstall: the question, then what the recycle bin does (web/skins.js, uninstall); greyed for a game whose files are
        // not here
        { label: T("skin.uninstall.entry"), disabled: g.status !== "ready" || !(window.ByopopAddGames && ByopopAddGames.has && ByopopAddGames.has(g.id)),
          fn: function () { ByopopSkin.desk.uninstall(g.id, g.title, back); } },
        { rule: true },
        { label: T(fav ? "library.menu.fav_remove" : "library.menu.fav_add"), disabled: !g.fav, fn: function () { act(r, "fav"); } },
        { label: T("library.menu.props"), fn: function () { act(r, "props"); } }
      ];
      // a game played here whose files are gone: why, in a message box (web/index.html, ByopopTiles.explain)
      if (g.why) entries.push({ label: T("index.why.title"), fn: function () { try { ByopopTiles.explain(g.id, back); } catch (e) {} } });
      if (g.buy.length) {
        entries.push({ rule: true });
        g.buy.forEach(function (a) { entries.push({ label: a.textContent.trim(), href: a.href }); });
      }
      ByopopSkin.desk.menu(r.cell, entries, x, y, g.title);
    }
    // Properties: a Windows 95 property sheet of the game's values
    function props(g) {
      var d = make("dialog", "d95-msgbox lib-props");
      d.setAttribute("aria-labelledby", "lib-props-title");
      var tb = make("div", "d95-tb d95-msgbox-tb");
      var tt = make("span", "d95-tb-text", T("library.props.title", { game: g.title }));
      tt.id = "lib-props-title";
      tb.appendChild(tt);
      var dl = make("dl", "lib-props-list");
      [["library.col.name", g.title], ["library.col.status", statusText(g.status)], ["library.props.released", lastText(g.released) || g.released],
       ["library.props.plays", String(g.plays)], ["library.col.time", timeText(g.seconds) || "0"], ["library.props.first", lastText(g.first)],
       ["library.col.last", lastText(g.last)],
       ["library.props.win95", T(g.win95 === "yes" ? "library.props.yes" : "library.props.no")]].forEach(function (p) {
        dl.appendChild(make("dt", "", T(p[0]))); dl.appendChild(make("dd", "", p[1] || "—"));
      });
      // the play of each site player (web/profiles.js; ByopopPlay's by), the players in their own order, then play with none chosen
      // and play of a player since removed, each on a row of its own
      var by = g.by || {}, names = {}, order = [];
      try { ByopopProfiles.list().forEach(function (p) { names[p.id] = p.name; order.push(p.id); }); } catch (e) {}
      Object.keys(by).forEach(function (pid) { if (order.indexOf(pid) < 0) order.push(pid); });
      var rows = order.filter(function (pid) { return by[pid] && (by[pid].plays || by[pid].seconds); });
      var table = null;
      if (rows.length) {
        table = make("table", "lib-props-time");
        var head = make("tr", "");
        ["library.props.player", "library.props.plays", "library.col.time", "library.props.first", "library.col.last"].forEach(function (k) {
          var th = make("th", "", T(k)); th.scope = "col"; head.appendChild(th);
        });
        table.appendChild(head);
        rows.forEach(function (pid) {
          var r = by[pid], tr = make("tr", "");
          var who = make("th", "", pid === "" ? T("library.props.no_player") : names[pid] || T("library.props.removed"));
          who.scope = "row"; tr.appendChild(who);
          [String(r.plays), timeText(r.seconds) || "0", lastText(r.first), lastText(r.last)].forEach(function (v) { tr.appendChild(make("td", "", v || "—")); });
          table.appendChild(tr);
        });
      }
      var ok = make("button", "d95-def", T("library.ok"));   // the default, framed (skins.css)
      ok.type = "button";
      ok.addEventListener("click", function () { d.close(); });
      d.addEventListener("keydown", function (ev) { if (ev.key === "Escape" || ev.key === "Enter") { ev.preventDefault(); ev.stopPropagation(); d.close(); } });
      d.addEventListener("close", function () { if (d.parentNode) d.parentNode.removeChild(d); if (sel) select(sel, true); });
      var row = make("div", "lib-props-row"); row.appendChild(ok);
      d.appendChild(tb); d.appendChild(dl); if (table) d.appendChild(table); d.appendChild(row);
      document.body.appendChild(d);
      try { d.showModal(); } catch (e) { d.setAttribute("open", ""); }
      ok.focus();
    }

    play.addEventListener("click", function () { act(sel, "play"); });
    add.addEventListener("click", function () { try { ByopopAddGames.add(); } catch (e) {} });
    unsoldBox.addEventListener("change", function () {
      try { if (unsoldBox.checked) localStorage.setItem(UNSOLD_KEY, "show"); else localStorage.removeItem(UNSOLD_KEY); } catch (e) {}
      fill();
    });
    filter.addEventListener("input", applyFilter);
    filter.addEventListener("keydown", function (ev) {
      if (ev.key === "ArrowDown" || ev.key === "Enter") { ev.preventDefault(); var v = visible(); if (v.length) select(sel && !sel.tr.hidden ? sel : v[0], true); }
    });
    // the grid's keys: one tab stop (the selected row's first cell), the arrows, Home and End, Page Up and Down, typing a name's
    // first letters, Enter to play, and the menu
    // anywhere in the window but its filter, buttons and panels, so the keys work as soon as the window is in front
    el.addEventListener("keydown", function (ev) {
      if (ev.defaultPrevented || (ev.target.closest && ev.target.closest("input, button, select, a, dialog, .adding"))) return;
      var v = visible(), i = v.indexOf(sel), page = Math.max(1, Math.floor(area.clientHeight / ((sel && sel.tr.offsetHeight) || 18)) - 1), to = -1;
      if (ev.key === "ArrowDown") to = i + 1;
      else if (ev.key === "ArrowUp") to = i - 1;
      else if (ev.key === "Home") to = 0;
      else if (ev.key === "End") to = v.length - 1;
      else if (ev.key === "PageDown") to = i + page;
      else if (ev.key === "PageUp") to = i - page;
      else if (ev.key === "Enter") { ev.preventDefault(); act(sel, "play"); return; }
      else if (ev.key === "ContextMenu" || (ev.key === "F10" && ev.shiftKey)) {
        ev.preventDefault();
        if (sel) { var rr = sel.cell.getBoundingClientRect(); menu(sel, rr.left + 24, rr.bottom); }
        return;
      } else if (ev.key.length === 1 && !ev.ctrlKey && !ev.metaKey && !ev.altKey && ev.key !== " ") {
        var now = Date.now();
        typed = (now - typedAt < 800 ? typed : "") + ev.key.toLowerCase();
        typedAt = now;
        var start = typed.length === 1 ? i + 1 : i, n = v.length;
        for (var k = 0; k < n; k++) {
          var r = v[(start + k) % n];
          if (r.g.title.toLowerCase().indexOf(typed) === 0) { select(r, true); break; }
        }
        ev.preventDefault();
        return;
      } else return;
      ev.preventDefault();
      if (!v.length) return;
      select(v[Math.max(0, Math.min(v.length - 1, to))], true);
    });

    // A game's folder dragged over its row: the row lights, and the drop adds it to that game (web/addgames.js, dropOn), as
    // a drop on its card did. Elsewhere in the window the desktop's own drop takes it (the Setup window).
    var over = null;
    function rowOf(ev) {
      var tr = ev.target && ev.target.closest ? ev.target.closest("tr.lib-row") : null, r = null;
      rows.forEach(function (x) { if (x.tr === tr) r = x; });
      return r && r.g.status !== "soon" && window.ByopopAddGames && ByopopAddGames.has && ByopopAddGames.has(r.g.id) ? r : null;
    }
    function light(r) { if (over && over !== r) over.tr.classList.remove("lib-drop"); over = r; if (r) r.tr.classList.add("lib-drop"); }
    function hasFiles(ev) { try { return !!ByopopImport.hasFiles(ev.dataTransfer); } catch (e) { return false; } }
    area.addEventListener("dragover", function (ev) {
      if (!hasFiles(ev)) return;
      var r = rowOf(ev);
      light(r);
      if (!r) return;
      ev.preventDefault();
      ev.stopPropagation();
      ev.dataTransfer.dropEffect = "copy";
    });
    area.addEventListener("dragleave", function (ev) { if (!ev.relatedTarget || !area.contains(ev.relatedTarget)) light(null); });
    area.addEventListener("drop", function (ev) {
      var r = rowOf(ev);
      light(null);
      if (!r || !hasFiles(ev)) return;
      ev.preventDefault();
      ev.stopPropagation();
      try { ByopopAddGames.dropOn(r.g.id, ev.dataTransfer); } catch (e) {}
    });

    // the listing's cards change (the files check ends, a favourite, a game hidden): the rows follow while the window is open
    var pending = false;
    function later() { if (pending) return; pending = true; setTimeout(function () { pending = false; if (!el.closest(".d95-closed")) fill(); }, 50); }
    var list = document.querySelector("ul.games");
    if (list && window.MutationObserver) new MutationObserver(later).observe(list, { attributes: true, subtree: true, attributeFilter: ["data-files", "aria-pressed", "hidden"] });
    window.addEventListener("storage", later);
    window.addEventListener("byopop-phase", later);
    window.addEventListener("byopop-setting", later);
    window.addEventListener("pageshow", later);
    // the play counter's counts (every two minutes while the page is in view): the Popular column, when there is a counter
    try {
      ByopopCounter.counts(function (games) {
        site = games;
        if (cols.indexOf("popular") < 0) { cols.push("popular"); header(); el.classList.add("lib-has-popular"); }
        later();
      });
    } catch (e) {}
    fill();
    return { el: el, refresh: function () { fill(); }, focus: function () { if (sel && !el.contains(document.activeElement)) select(sel, true, true); } };
  }

  // The window on the listing's desktop (web/index.html adds the others), open at first, its icon first in the column. What
  // became of the folders added and the recycle bin's panel (web/addgames.js) come into it from the page, under its bar.
  var lib = null;
  function attach(desk, at) {
    if (lib || !document.getElementById("games-title")) return null;
    lib = build();
    var main = document.querySelector("main");
    if (main && main.parentNode) main.parentNode.insertBefore(lib.el, main.nextSibling); else document.body.appendChild(lib.el);
    var area = lib.el.querySelector(".lib-area");
    ["adding", "bin-panel"].forEach(function (id) { var p = document.getElementById(id); if (p) lib.el.insertBefore(p, area); });
    // never narrower than its bar's controls, nor shorter than five rows (the user, 2026-11-18: "Some windows should have a
    // minimum size too, like the library")
    desk.add(lib.el, { id: "library", title: T("library.title"), closable: true, at: at, minWidth: 560, minHeight: 200 });
    lib.el.classList.add("lib-win");
    // fresh values each time it comes to the front (play time and files change while it is behind a game)
    var front = false;
    window.addEventListener("byopop-desk-active", function (ev) {
      var now = !!(ev.detail && ev.detail.id === "library");
      if (now && !front) { lib.refresh(); lib.focus(); }
      front = now;
    });
    return lib;
  }
  return { attach: attach, refresh: function () { if (lib) lib.refresh(); } };
})();
