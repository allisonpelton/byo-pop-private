// The Library (docs/WEB_PLAYER.md, "The Library"): Desktop 95's other window of the games, on the listing's desktop. It
// evokes a game client of 2004 in its own look (web/skins.css, .lib-*: olive and khaki, flat panels with 1-pixel bevels,
// small type, square buttons, its own title bar), drawn by us and named for no real program. It is a grid of the games, one
// row each: a 16-pixel icon and the name, the status, the play time, the last day played and the year; a click on a column's
// header sorts by it, again the other way (kept in localStorage "byopop-library-sort"). A double-click or Enter plays the
// selected game, its menu (a right click, the menu key or Shift+F10) has Play, Add files, the favourite, Properties and where
// to buy it, and a Play button and a Filter box sit above the grid. Everything it shows comes from the listing's own cards (their
// files check, favourite and buy links) and ByopopPlay (web/settings.js), so the two windows always agree.
var ByopopLibrary = (function () {
  "use strict";
  var KEY = "byopop-library-sort";
  var COLS = ["name", "status", "time", "last", "year"];
  var STATUS_ORDER = { ready: 0, files: 1, unsold: 2, soon: 3 };
  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function make(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  function load() {
    try { var v = JSON.parse(localStorage.getItem(KEY) || "null"); if (v && COLS.indexOf(v.col) >= 0) return { col: v.col, dir: v.dir === -1 ? -1 : 1 }; } catch (e) {}
    return { col: "name", dir: 1 };
  }
  function keep(s) { try { localStorage.setItem(KEY, JSON.stringify(s)); } catch (e) {} }

  // ---- the games, as the listing's cards have them ----
  function readGames() {
    var play = {}, rec = {};
    try { play = ByopopPlay.all(); } catch (e) {}
    try { rec = ByopopGameFiles.all(); } catch (e) {}
    var hide95 = document.documentElement.classList.contains("d95-win95only");
    return Array.prototype.map.call(document.querySelectorAll("ul.games > li"), function (li) {
      // a game still to come has a card with no link (div.tile): its title is its id here
      var tile = li.querySelector(".tile");
      if (!tile || li.hidden) return null;
      if (hide95 && li.getAttribute("data-win95") !== "yes") return null;
      var id = tile.getAttribute("data-game") || "soon:" + (li.getAttribute("data-title") || ""), files = li.getAttribute("data-files");
      var status = li.classList.contains("soon") ? "soon" : files === "yes" ? "ready" : li.hasAttribute("data-unsold") ? "unsold" : "files";
      var p = play[id] || {}, r = rec[id], icon = "";
      try { icon = r && ByopopGameFiles.present(r) ? (r.small || r.icon || "") : ""; } catch (e) {}
      var badge = tile.querySelector(".badge");
      var released = li.getAttribute("data-released") || "";
      return { id: id, li: li, tile: tile, href: tile.href || "", title: li.getAttribute("data-title") || id, status: status,
        seconds: p.seconds > 0 ? p.seconds : 0, plays: p.plays > 0 ? p.plays : 0, last: p.last || "", released: released,
        year: /^\d{4}/.test(released) ? +released.slice(0, 4) : 0, icon: icon,
        tag: badge ? (badge.getAttribute("data-initials") || badge.textContent) : "", colour: li.style.getPropertyValue("--tile") || "",
        fav: li.querySelector("button.fav"), buy: Array.prototype.filter.call(li.querySelectorAll("a[href]"), function (a) { return a !== tile && /^https?:/.test(a.href) && a.host !== location.host; }),
        win95: li.getAttribute("data-win95") || "unknown" };
    }).filter(Boolean);
  }
  function lastText(day) {
    var m = /^(\d{4})-(\d{2})-(\d{2})$/.exec(day || "");
    if (!m) return "";
    try { return new Date(+m[1], +m[2] - 1, +m[3]).toLocaleDateString(undefined, { year: "numeric", month: "numeric", day: "numeric" }); } catch (e) { return day; }
  }
  function timeText(s) { if (!(s > 0)) return ""; try { return ByopopPlay.duration(s); } catch (e) { return Math.round(s / 60) + " min"; } }
  function statusText(s) { return T({ ready: "library.status.ready", files: "library.status.files", soon: "library.status.soon", unsold: "library.status.unsold" }[s]); }
  function compare(col) {
    return function (a, b) {
      switch (col) {
        case "status": return STATUS_ORDER[a.status] - STATUS_ORDER[b.status];
        case "time": return a.seconds - b.seconds;
        case "last": return (a.last < b.last ? -1 : a.last > b.last ? 1 : 0);
        case "year": return (a.released < b.released ? -1 : a.released > b.released ? 1 : 0);
      }
      return 0;
    };
  }
  function byName(a, b) { return a.title.toLowerCase().replace(/ deluxe/g, "").localeCompare(b.title.toLowerCase().replace(/ deluxe/g, "")); }

  // ---- the window ----
  function build() {
    var el = make("section", "lib");
    el.id = "library";
    el.setAttribute("aria-label", T("library.title"));
    var bar = make("div", "lib-bar");
    var play = make("button", "lib-btn lib-play", T("library.play"));
    play.type = "button";
    var flabel = make("label", "lib-flabel", T("library.filter"));
    var filter = make("input", "lib-filter");
    filter.type = "search"; filter.id = "lib-filter"; filter.autocomplete = "off"; filter.spellcheck = false;
    flabel.htmlFor = filter.id;
    bar.appendChild(play); bar.appendChild(flabel); bar.appendChild(filter);
    var area = make("div", "lib-area");
    var grid = make("table", "lib-grid");
    grid.setAttribute("role", "grid");
    grid.setAttribute("aria-label", T("library.title"));
    grid.setAttribute("aria-readonly", "true");
    var head = make("thead"), hr = make("tr");
    hr.setAttribute("role", "row");
    var heads = {};
    COLS.forEach(function (c) {
      var th = make("th", "lib-h lib-c-" + c);
      th.setAttribute("role", "columnheader"); th.scope = "col";
      var b = make("button", "lib-hb", T("library.col." + c));
      b.type = "button";
      b.addEventListener("click", function () { sortBy(c, true); });
      th.appendChild(b);
      hr.appendChild(th);
      heads[c] = th;
    });
    head.appendChild(hr);
    var body = make("tbody");
    grid.appendChild(head); grid.appendChild(body);
    area.appendChild(grid);
    var foot = make("div", "lib-status");
    var count = make("span", "lib-count"), said = make("span", "visually-hidden");
    said.setAttribute("role", "status"); said.setAttribute("aria-live", "polite");
    foot.appendChild(count); foot.appendChild(said);
    el.appendChild(bar); el.appendChild(area); el.appendChild(foot);

    var sort = load(), rows = [], games = [], sel = null, typed = "", typedAt = 0;
    function order() {
      var cmp = compare(sort.col);
      return games.slice().sort(function (a, b) { return (sort.col === "name" ? 0 : cmp(a, b) * sort.dir) || byName(a, b) * (sort.col === "name" ? sort.dir : 1); });
    }
    function visible() { return rows.filter(function (r) { return !r.tr.hidden; }); }
    // jump: bring the selected row into view (a sort); a refresh from the cards or the storage keeps the list where the player
    // scrolled it (the user, 2026-11-10: "Scroll keeps snapping back in the library window to the selected game")
    function fill(jump) {
      var keepId = sel ? sel.g.id : null, keepTop = area.scrollTop, hadFocus = !!(sel && document.activeElement === sel.cell);
      games = readGames();
      body.textContent = "";
      rows = order().map(function (g) {
        var tr = make("tr", "lib-row lib-s-" + g.status);
        tr.setAttribute("role", "row"); tr.setAttribute("aria-selected", "false");
        var name = make("td", "lib-c-name");
        name.setAttribute("role", "rowheader"); name.tabIndex = -1;
        var ic = make("i", "lib-icon");
        ic.setAttribute("aria-hidden", "true");
        if (g.icon) { var im = make("img"); im.alt = ""; im.draggable = false; im.src = g.icon; ic.appendChild(im); }
        else { ic.classList.add("lib-icon-cap"); ic.style.setProperty("--tile", g.colour || "#808080"); ic.textContent = (g.tag || "").slice(0, 2); }
        var wrap = make("span", "lib-name");
        wrap.appendChild(ic); wrap.appendChild(make("span", "", g.title));
        name.appendChild(wrap);
        tr.appendChild(name);
        [["status", statusText(g.status)], ["time", timeText(g.seconds)], ["last", lastText(g.last)], ["year", g.year ? String(g.year) : ""]].forEach(function (c) {
          var td = make("td", "lib-c-" + c[0], c[1]);
          td.setAttribute("role", "gridcell");
          tr.appendChild(td);
        });
        var r = { tr: tr, cell: name, g: g };
        // (the default would move the focus off the row's first cell again when another cell was pressed, and the keys with it)
        tr.addEventListener("mousedown", function (ev) { if (ev.button !== 1) { ev.preventDefault(); select(r, true); } });
        tr.addEventListener("dblclick", function () { act(r, "play"); });
        tr.addEventListener("contextmenu", function (ev) { ev.preventDefault(); select(r, true); menu(r, ev.clientX, ev.clientY); });
        body.appendChild(tr);
        return r;
      });
      Object.keys(heads).forEach(function (c) { heads[c].setAttribute("aria-sort", c === sort.col ? (sort.dir > 0 ? "ascending" : "descending") : "none"); });
      applyFilter();
      var again = rows.filter(function (r) { return r.g.id === keepId && !r.tr.hidden; })[0] || visible()[0] || null;
      select(again, hadFocus, jump !== true);
      if (jump !== true) area.scrollTop = keepTop;
    }
    function applyFilter() {
      var q = filter.value.trim().toLowerCase();
      rows.forEach(function (r) { r.tr.hidden = !!q && r.g.title.toLowerCase().indexOf(q) < 0; });
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
      sort = { col: c, dir: sort.col === c ? -sort.dir : 1 };
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
    function menu(r, x, y) {
      var g = r.g, fav = g.fav && g.fav.getAttribute("aria-pressed") === "true";
      var entries = [
        { label: T("library.menu.play"), bold: true, disabled: g.status !== "ready", fn: function () { act(r, "play"); } },
        { label: T("library.menu.files"), disabled: g.status === "ready" || g.status === "soon", fn: function () { act(r, "files"); } },
        { rule: true },
        { label: T(fav ? "library.menu.fav_remove" : "library.menu.fav_add"), disabled: !g.fav, fn: function () { act(r, "fav"); } },
        { label: T("library.menu.props"), fn: function () { act(r, "props"); } }
      ];
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
       ["library.props.plays", String(g.plays)], ["library.col.time", timeText(g.seconds) || "0"], ["library.col.last", lastText(g.last)],
       ["library.props.win95", T(g.win95 === "yes" ? "library.props.yes" : "library.props.no")]].forEach(function (p) {
        dl.appendChild(make("dt", "", T(p[0]))); dl.appendChild(make("dd", "", p[1] || "—"));
      });
      var ok = make("button", "", T("library.ok"));
      ok.type = "button";
      ok.addEventListener("click", function () { d.close(); });
      d.addEventListener("keydown", function (ev) { if (ev.key === "Escape" || ev.key === "Enter") { ev.preventDefault(); ev.stopPropagation(); d.close(); } });
      d.addEventListener("close", function () { if (d.parentNode) d.parentNode.removeChild(d); if (sel) select(sel, true); });
      var row = make("div", "lib-props-row"); row.appendChild(ok);
      d.appendChild(tb); d.appendChild(dl); d.appendChild(row);
      document.body.appendChild(d);
      try { d.showModal(); } catch (e) { d.setAttribute("open", ""); }
      ok.focus();
    }

    play.addEventListener("click", function () { act(sel, "play"); });
    filter.addEventListener("input", applyFilter);
    filter.addEventListener("keydown", function (ev) {
      if (ev.key === "ArrowDown" || ev.key === "Enter") { ev.preventDefault(); var v = visible(); if (v.length) select(sel && !sel.tr.hidden ? sel : v[0], true); }
    });
    // the grid's keys: one tab stop (the selected row's first cell), the arrows, Home and End, Page Up and Down, typing a name's
    // first letters, Enter to play, and the menu
    // anywhere in the window but its filter and buttons, so the keys work as soon as the window is in front
    el.addEventListener("keydown", function (ev) {
      if (ev.defaultPrevented || (ev.target.closest && ev.target.closest("input, button, dialog"))) return;
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

    // the listing's cards change (the files check ends, a favourite, a game hidden): the rows follow while the window is open
    var pending = false;
    function later() { if (pending) return; pending = true; setTimeout(function () { pending = false; if (!el.closest(".d95-closed")) fill(); }, 50); }
    var list = document.querySelector("ul.games");
    if (list && window.MutationObserver) new MutationObserver(later).observe(list, { attributes: true, subtree: true, attributeFilter: ["data-files", "aria-pressed", "hidden"] });
    window.addEventListener("storage", later);
    window.addEventListener("byopop-setting", later);
    window.addEventListener("pageshow", later);
    fill();
    return { el: el, refresh: function () { fill(); }, focus: function () { if (sel && !el.contains(document.activeElement)) select(sel, true, true); } };
  }

  // The window on the listing's desktop (web/index.html adds the others): closed at first, its icon in the column after All games'
  var lib = null;
  function attach(desk, at) {
    if (lib || !document.getElementById("games-title")) return null;
    lib = build();
    var main = document.querySelector("main");
    if (main && main.parentNode) main.parentNode.insertBefore(lib.el, main.nextSibling); else document.body.appendChild(lib.el);
    var pic = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    pic.setAttribute("viewBox", "0 0 32 32"); pic.setAttribute("aria-hidden", "true");
    pic.innerHTML = '<rect x="2" y="4" width="28" height="24" fill="#4c5844" stroke="#20261c"/><rect x="2" y="4" width="28" height="5" fill="#958831"/>' +
      '<rect x="5" y="12" width="4" height="3" fill="#c4b550"/><rect x="11" y="12" width="16" height="3" fill="#d8ded3"/>' +
      '<rect x="5" y="17" width="4" height="3" fill="#c4b550"/><rect x="11" y="17" width="12" height="3" fill="#d8ded3"/>' +
      '<rect x="5" y="22" width="4" height="3" fill="#c4b550"/><rect x="11" y="22" width="14" height="3" fill="#d8ded3"/>';
    desk.add(lib.el, { id: "library", title: T("library.title"), closable: true, closed: true, pic: pic, at: at });
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
