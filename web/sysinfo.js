// System Information (docs/WEB_PLAYER.md, "System Information"): Desktop 95's window about the site itself, in the shape of
// Windows 95's System Properties (the user, 2026-11-18: ""About" section has accumulated cruft. It was originally a footer. I
// suggest a new window, System Information, that indicates how the site is performing overall (sort of like the performance
// panel) and also has that text, maybe in one of the window's tabs. Figure out a 95-related app to model the design on").
// It replaces the About window of the listing page. Three tabs:
//   General      the site and the computer as the browser reports them, then the page's old footer: what the site is, the
//                trademarks, what it is made with, and who made it. Off the desktop this tab alone is the page's footer.
//   Performance  the site's state, as the 1995 tab's "Performance status": the storage it uses, the games whose files are
//                here, and each running game's frame rate, frame time and sound gaps; and the performance panel's switch
//   Monitor      the live line charts of Windows 95's System Monitor, for one running game: a minute of its readings
// The readings are each game window's own (web/perf.js, watch), taken once a second while this window is open and not
// minimized on the desktop, and not at all otherwise.
var ByopopSysInfo = (function () {
  "use strict";
  var HISTORY = 60;  // seconds of readings a chart shows
  // the charts: the reading, its scale's least top (the scale grows with the readings), and how a value is written
  var CHARTS = [
    { id: "fps", key: "index.sysinfo.chart.fps", top: 60, text: function (v) { return v.toFixed(0); } },
    { id: "frame", key: "index.sysinfo.chart.frame", top: 40, text: function (v) { return v.toFixed(1); } },
    { id: "share", key: "index.sysinfo.chart.share", top: 100, text: function (v) { return v.toFixed(0); } },
    { id: "gaps", key: "index.sysinfo.chart.gaps", top: 4, text: function (v) { return v.toFixed(0); } }
  ];
  var TABS = ["general", "perf", "monitor"];

  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function make(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  function bytes(n) {
    if (typeof n !== "number" || !isFinite(n)) return T("index.sysinfo.unknown");
    if (n >= 1073741824) return (n / 1073741824).toFixed(2) + " GB";
    if (n >= 1048576) return (n / 1048576).toFixed(1) + " MB";
    if (n >= 1024) return Math.round(n / 1024) + " KB";
    return Math.round(n) + " B";
  }

  var root = null, tabEls = {}, panelEls = {}, shown = "general";
  var statusList = null, gameRows = null, panelBox = null, gameSelect = null, chartEls = [];
  var timer = 0, storage = null, storageAt = 0;
  // each running game's readings, by its window's id: { id, title, perf, reading, lastGaps, fps: [], frame: [], share: [], gaps: [] }
  var games = {}, order = [], chosen = "";

  // ---------- what the page knows ----------
  // the game windows open on the desktop (web/deskapps.js)
  function running() { try { return window.ByopopApps && ByopopApps.running ? ByopopApps.running() : []; } catch (e) { return []; } }
  function cards() { return Array.prototype.slice.call(document.querySelectorAll("ul.games > li")).filter(function (li) { return !li.classList.contains("soon"); }); }
  function installed() { return cards().filter(function (li) { return li.getAttribute("data-files") === "yes"; }).length; }
  function readStorage() {
    storageAt = Date.now();
    try {
      if (navigator.storage && navigator.storage.estimate)
        navigator.storage.estimate().then(function (e) { storage = e; drawPerf(); }, function () {});
    } catch (e) {}
  }

  // ---------- the readings, once a second ----------
  function push(list, v) { list.push(v); if (list.length > HISTORY) list.shift(); }
  function tick() {
    if (document.hidden) return;
    var seen = {};
    order = [];
    running().forEach(function (g) {
      seen[g.id] = true;
      order.push(g.id);
      var rec = games[g.id] || (games[g.id] = { id: g.id, title: "", perf: null, reading: null, lastGaps: null, fps: [], frame: [], share: [], gaps: [] });
      rec.title = g.title;
      var P = null;
      try { P = g.win && g.win.ByopopPerf; } catch (e) {}
      // a page that loaded again has a new ByopopPerf, to be asked again
      if (P && P.watch && rec.perf !== P) { try { P.watch(true); rec.perf = P; } catch (e) {} }
      var r = null;
      try { r = P && P.reading ? P.reading() : null; } catch (e) {}
      rec.reading = r;
      if (!r) return;
      var gaps = r.audio && typeof r.audio.gaps === "number" ? r.audio.gaps : null;
      push(rec.fps, r.fps);
      push(rec.frame, r.frameAvg);
      push(rec.share, r.share * 100);
      push(rec.gaps, gaps === null || rec.lastGaps === null ? 0 : Math.max(0, gaps - rec.lastGaps));
      rec.lastGaps = gaps;
    });
    Object.keys(games).forEach(function (id) { if (!seen[id]) delete games[id]; });
    if (Date.now() - storageAt > 10000) readStorage();
    drawPerf();
    drawMonitor();
  }
  function start() {
    if (timer) return;
    computerFacts();
    readStorage();
    tick();
    timer = setInterval(tick, 1000);
  }
  function stop() {
    if (!timer) return;
    clearInterval(timer); timer = 0;
    Object.keys(games).forEach(function (id) { var P = games[id].perf; try { if (P && P.watch) P.watch(false); } catch (e) {} });
    games = {}; order = [];
  }
  // open and not minimized on the desktop, with the page in view: the readings are taken
  function wanted() {
    var d = document.documentElement;
    return !!root && d.classList.contains("d95-desk") && root.classList.contains("d95-win") &&
      !root.classList.contains("d95-closed") && !root.classList.contains("d95-min") && !root.hidden;
  }
  function follow() {
    if (wanted()) start(); else stop();
    // off the desktop the footer is the General tab alone, which must not stay hidden from a tab chosen on it
    if (!document.documentElement.classList.contains("d95-desk") && shown !== "general" && tabEls.general) select("general", false);
  }

  // ---------- General ----------
  // the computer as the browser reports it, read again each time the window comes up (a screen may change, and a page opened
  // hidden reports none)
  var computerEl = null;
  function computerFacts() {
    if (!computerEl) return;
    var facts = [];
    if (navigator.hardwareConcurrency) facts.push(T("index.sysinfo.threads", { n: navigator.hardwareConcurrency }));
    try {
      var dpr = window.devicePixelRatio || 1, sw = Math.round(screen.width * dpr), sh = Math.round(screen.height * dpr);
      if (sw > 0 && sh > 0) facts.push(T("index.sysinfo.screen", { w: sw, h: sh }));
    } catch (e) {}
    if (!facts.length) facts.push(T("index.sysinfo.unknown"));
    computerEl.textContent = "";
    facts.forEach(function (v) { computerEl.appendChild(make("span", "", v)); });
  }
  function line(dl, label, values) {
    var dt = make("dt", "", label), dd = make("dd");
    values.forEach(function (v) { dd.appendChild(make("span", "", v)); });
    dl.appendChild(dt); dl.appendChild(dd);
  }
  function buildSystem() {
    var box = make("div", "sy-system");
    // the site's mark where Windows 95 had its own, at the left
    var logo = make("img", "sy-logo");
    logo.src = "favicon.svg"; logo.alt = ""; logo.draggable = false; logo.width = 64; logo.height = 64;
    logo.onerror = function () { logo.hidden = true; };
    var dl = make("dl", "sy-facts");
    line(dl, T("index.sysinfo.system"), ["byo-pop", T("index.sysinfo.games", { n: cards().length })]);
    line(dl, T("index.sysinfo.computer"), []);
    computerEl = dl.lastChild;
    computerFacts();
    box.appendChild(logo); box.appendChild(dl);
    return box;
  }

  // ---------- Performance ----------
  function group(title) {
    var f = make("fieldset", "sy-group");
    f.appendChild(make("legend", "", title));
    return f;
  }
  function buildPerf(panel) {
    var status = group(T("index.sysinfo.status"));
    statusList = make("dl", "sy-facts sy-status");
    status.appendChild(statusList);
    var well = make("div", "sy-well");
    var table = make("table", "sy-table"), head = make("thead"), tr = make("tr");
    ["index.sysinfo.col.game", "index.sysinfo.col.fps", "index.sysinfo.col.frame", "index.sysinfo.col.gaps"].forEach(function (k, i) {
      var th = make("th", i ? "sy-num" : "", T(k)); th.scope = "col"; tr.appendChild(th);
    });
    head.appendChild(tr); table.appendChild(head);
    gameRows = make("tbody");
    table.appendChild(gameRows);
    well.appendChild(table);
    status.appendChild(well);
    panel.appendChild(status);
    // the 1995 tab's "Advanced settings": here the performance panel beside each game (web/perf.js), the site setting
    var adv = group(T("index.sysinfo.advanced"));
    var lab = make("label", "sy-check");
    panelBox = make("input");
    panelBox.type = "checkbox";
    lab.appendChild(panelBox); lab.appendChild(document.createTextNode(T("index.sysinfo.perf_panel")));
    lab.title = T("settings.perf.hint");
    panelBox.addEventListener("change", function () {
      try { ByopopSettings.set("perf", panelBox.checked ? "on" : "off"); } catch (e) {}
    });
    adv.appendChild(lab);
    panel.appendChild(adv);
  }
  function syncPanelBox() { try { if (panelBox) panelBox.checked = ByopopSettings.get("perf") === "on"; } catch (e) {} }
  function drawPerf() {
    if (!statusList) return;
    statusList.textContent = "";
    var all = cards().length, list = order.map(function (id) { return games[id]; }).filter(Boolean);
    line(statusList, T("index.sysinfo.storage"), [storage ? T("index.sysinfo.storage_of", { used: bytes(storage.usage), quota: bytes(storage.quota) }) : T("index.sysinfo.unknown")]);
    line(statusList, T("index.sysinfo.installed"), [T("index.sysinfo.of", { n: installed(), total: all })]);
    line(statusList, T("index.sysinfo.running"), [String(running().length)]);
    gameRows.textContent = "";
    if (!list.length) {
      var tr = make("tr"), td = make("td", "sy-empty", T("index.sysinfo.none"));
      td.colSpan = 4; tr.appendChild(td); gameRows.appendChild(tr);
      return;
    }
    list.forEach(function (g) {
      var r = g.reading, tr = make("tr");
      tr.appendChild(make("td", "", g.title));
      tr.appendChild(make("td", "sy-num", r ? r.fps.toFixed(0) : "…"));
      tr.appendChild(make("td", "sy-num", r ? r.frameAvg.toFixed(1) + " ms" : "…"));
      tr.appendChild(make("td", "sy-num", r && r.audio && typeof r.audio.gaps === "number" ? String(r.audio.gaps) : r ? "–" : "…"));
      gameRows.appendChild(tr);
    });
  }

  // ---------- Monitor ----------
  function buildMonitor(panel) {
    var row = make("div", "sy-pick");
    var id = "sy-game";
    var lab = make("label", "", T("index.sysinfo.game"));
    lab.htmlFor = id;
    gameSelect = make("select");
    gameSelect.id = id;
    gameSelect.addEventListener("change", function () { chosen = gameSelect.value; drawMonitor(); });
    row.appendChild(lab); row.appendChild(gameSelect);
    panel.appendChild(row);
    var grid = make("div", "sy-charts");
    CHARTS.forEach(function (c) {
      var box = make("figure", "sy-chart");
      var cap = make("figcaption", "sy-chart-cap");
      var name = make("span", "sy-chart-name", T(c.key)), now = make("span", "sy-chart-now");
      cap.appendChild(name); cap.appendChild(now);
      var canvas = make("canvas", "sy-chart-plot");
      canvas.setAttribute("role", "img");
      canvas.setAttribute("aria-label", T(c.key));
      // the canvas in a sunken well (a canvas covers its own inset shadow)
      var plot = make("div", "sy-plot");
      plot.appendChild(canvas);
      box.appendChild(cap); box.appendChild(plot);
      grid.appendChild(box);
      chartEls.push({ def: c, canvas: canvas, now: now });
    });
    panel.appendChild(grid);
  }
  function niceTop(v, least) {
    var top = Math.max(v, least);
    var step = Math.pow(10, Math.floor(Math.log(top) / Math.LN10));
    return Math.ceil(top / step) * step;
  }
  function drawChart(ch, values) {
    var c = ch.canvas, w = c.clientWidth, h = c.clientHeight;
    if (!w || !h) return;
    var dpr = window.devicePixelRatio || 1;
    if (c.width !== Math.round(w * dpr) || c.height !== Math.round(h * dpr)) { c.width = Math.round(w * dpr); c.height = Math.round(h * dpr); }
    var g = c.getContext("2d");
    if (!g) return;
    g.setTransform(dpr, 0, 0, dpr, 0, 0);
    g.fillStyle = "#ffffff"; g.fillRect(0, 0, w, h);
    var max = 0;
    values.forEach(function (v) { if (v > max) max = v; });
    var top = niceTop(max, ch.def.top);
    // the scale's top at the left, and a dotted line at its half, as System Monitor marked its charts
    g.fillStyle = "#808080"; g.font = "10px " + (getComputedStyle(c).fontFamily || "sans-serif"); g.textBaseline = "top";
    g.fillText(ch.def.text(top), 2, 1);
    g.strokeStyle = "#c0c0c0"; g.lineWidth = 1; g.setLineDash([1, 2]);
    g.beginPath(); g.moveTo(0, Math.round(h / 2) + 0.5); g.lineTo(w, Math.round(h / 2) + 0.5); g.stroke();
    g.setLineDash([]);
    if (values.length) {
      var dx = w / (HISTORY - 1), x0 = w - (values.length - 1) * dx;
      g.strokeStyle = "#000080"; g.lineWidth = 1.5;
      g.beginPath();
      values.forEach(function (v, i) {
        var x = x0 + i * dx, y = h - 1 - (v / top) * (h - 2);
        if (i) g.lineTo(x, y); else g.moveTo(x, y);
      });
      g.stroke();
    }
    ch.now.textContent = values.length ? ch.def.text(values[values.length - 1]) : "";
    ch.canvas.setAttribute("aria-label", T(ch.def.key) + (values.length ? ": " + ch.def.text(values[values.length - 1]) : ""));
  }
  function drawMonitor() {
    if (!gameSelect) return;
    var list = order.map(function (id) { return games[id]; }).filter(Boolean);
    if (!games[chosen]) chosen = list.length ? list[list.length - 1].id : "";
    // the list of games, rebuilt only when the running games change
    var key = list.map(function (g) { return g.id + ":" + g.title; }).join("|");
    if (gameSelect.getAttribute("data-key") !== key) {
      gameSelect.textContent = "";
      if (!list.length) gameSelect.appendChild(make("option", "", T("index.sysinfo.none")));
      list.forEach(function (g) { var o = make("option", "", g.title); o.value = g.id; gameSelect.appendChild(o); });
      gameSelect.setAttribute("data-key", key);
    }
    gameSelect.disabled = !list.length;
    if (chosen) gameSelect.value = chosen;
    if (shown !== "monitor") return;
    var rec = games[chosen];
    chartEls.forEach(function (ch) { drawChart(ch, rec ? rec[ch.def.id] : []); });
  }

  // ---------- the tabs ----------
  function select(id, focus) {
    shown = id;
    TABS.forEach(function (t) {
      var on = t === id;
      tabEls[t].setAttribute("aria-selected", on ? "true" : "false");
      tabEls[t].tabIndex = on ? 0 : -1;
      panelEls[t].classList.toggle("sy-off", !on);
      if (on) panelEls[t].removeAttribute("aria-hidden"); else panelEls[t].setAttribute("aria-hidden", "true");
      panelEls[t].inert = !on;
    });
    if (focus) tabEls[id].focus();
    if (id === "perf") { syncPanelBox(); drawPerf(); }
    if (id === "monitor") drawMonitor();
  }

  // Builds the window in the page's footer (web/index.html), whose General tab holds the footer's own text: the tabs above it and
  // the other two tabs beside it, shown on the desktop only (web/index.html's styles)
  function mount(footer) {
    if (root || !footer) return;
    root = footer;
    var general = footer.querySelector(".sy-general");
    if (!general) return;
    var tabs = make("div", "sy-tabs");
    tabs.setAttribute("role", "tablist");
    tabs.setAttribute("aria-label", T("index.sysinfo.title"));
    var sheet = make("div", "sy-sheet");
    TABS.forEach(function (t) {
      var tab = make("button", "sy-tab", T("index.sysinfo.tab." + t));
      tab.type = "button"; tab.id = "sy-tab-" + t;
      tab.setAttribute("role", "tab"); tab.setAttribute("aria-controls", "sy-panel-" + t);
      tab.addEventListener("click", function () { select(t, false); });
      tabs.appendChild(tab);
      tabEls[t] = tab;
      var panel = t === "general" ? general : make("div", "sy-panel sy-" + t);
      panel.id = "sy-panel-" + t;
      panel.setAttribute("role", "tabpanel"); panel.setAttribute("aria-labelledby", tab.id);
      panelEls[t] = panel;
    });
    tabs.addEventListener("keydown", function (ev) {
      var i = TABS.indexOf(shown), n = TABS.length, j = -1;
      if (ev.key === "ArrowRight" || ev.key === "ArrowDown") j = (i + 1) % n;
      else if (ev.key === "ArrowLeft" || ev.key === "ArrowUp") j = (i - 1 + n) % n;
      else if (ev.key === "Home") j = 0;
      else if (ev.key === "End") j = n - 1;
      if (j < 0) return;
      ev.preventDefault();
      select(TABS[j], true);
    });
    general.insertBefore(buildSystem(), general.firstChild);
    buildPerf(panelEls.perf);
    buildMonitor(panelEls.monitor);
    footer.insertBefore(tabs, general);
    footer.insertBefore(sheet, general);
    TABS.forEach(function (t) { sheet.appendChild(panelEls[t]); });
    select("general", false);
    // the readings while the window is up: its class says (web/skins.js: d95-closed, d95-min), and the desktop on or off
    if (window.MutationObserver) new MutationObserver(follow).observe(footer, { attributes: true, attributeFilter: ["class", "hidden"] });
    window.addEventListener("byopop-desk", follow);
    window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "perf") syncPanelBox(); });
    window.addEventListener("storage", function (ev) { if (ev.key === null || (window.ByopopSettings && ev.key === ByopopSettings.KEY)) syncPanelBox(); });
    document.addEventListener("visibilitychange", function () { if (!document.hidden && timer) tick(); });
    follow();
  }
  return { mount: mount };
})();
