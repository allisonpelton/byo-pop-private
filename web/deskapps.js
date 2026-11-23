// Desktop 95 in one page (docs/WEB_PLAYER.md, "Desktop 95 in one page"; the user, 2026-11-08: "Make the Windows 95 skin work
// entirely in a single page", and "Would it be possible for that skin to support multiple games open at once in the same
// window?"). On the listing in Desktop 95 the player never leaves the desktop: every page of ours a link, an icon or a menu
// leads to opens as a window on it.
//   - a game: its page in a frame (?embed=1: the game, its setup cards and its own windows, no desktop of its own), with a
//     menu bar for those windows, each with its own heap, files, sound and keys. Only the active window's game has the
//     focus, so the others get the focus loss they get when a page loses it. Every game shares this page's memory, so one
//     that would take the open games past a memory budget asks first (the user, 2026-11-16: "Make it a warning"; below).
//   - another page of ours (a game's copyright and licences, the terms): its page in a frame, scrolling with the skin's bars.
//   - a licence text (licenses/*.txt): the text itself in the window.
// Links to other sites still open a tab. The address is always the site's root (the user, 2026-11-09: "What we're developing
// now is a SPA, and the url bar should never need to show any path other than root"), with only the test parameters the page
// was opened with (?silent=1, ?assets=): the windows open live in history.state, a new entry (pushState) for a window the
// player opens and the same entry (replaceState) as the active one changes or a window closes, so Back and Forward close or
// reopen windows without leaving the page and a reload opens them again. An old address of one of the pages, opened on its
// own, comes here as ?open=<page> (web/skins.js, onePage), which opens its window and goes back to the root.
var ByopopApps = (function () {
  "use strict";
  var SKIN = window.ByopopSkin, desk = SKIN && SKIN.desk, SITE = SKIN && SKIN.SITE;
  var Q = new URLSearchParams(location.search), SILENT = Q.get("silent") === "1";
  var NARROW = 760;
  var apps = [], seq = 0, restoring = false;
  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function on() { return !!(desk && desk.isOn()); }
  // the desktop's loading (web/skins.js) waits for the windows start() puts back, with their pages
  try { SKIN.bootHold("apps"); } catch (e) {}
  function bootDone() { try { SKIN.bootDone("apps"); } catch (e) {} }
  // the desktop's zoom (web/skins.js): the window's sizes are the screen's CSS pixels, a style's the page's
  function zf() { try { return ByopopSkin.zoom(); } catch (e) { return 1; } }
  function narrow() { return (document.documentElement.clientWidth || window.innerWidth) / zf() < NARROW; }

  // ---- what a link is ----
  // its address from the site's folder (null: another site's, or not a page); the key a window is known by is that address
  // without this site's own parameters (embed, silent) or a fragment
  function relOf(href) {
    if (!SITE) return null;
    var u;
    try { u = new URL(href, SITE); } catch (e) { return null; }
    if (u.href.indexOf(SITE) !== 0) return null;
    u.searchParams.delete("embed"); u.searchParams.delete("silent"); u.hash = "";
    return u.href.slice(SITE.length);
  }
  function pathOf(key) { return key.split("?")[0]; }
  function games() {
    var out = {};
    Array.prototype.forEach.call(document.querySelectorAll("a.tile[data-game]"), function (a) {
      var n = a.querySelector(".name");
      var key = relOf(a.href);
      if (key !== null) out[pathOf(key)] = { id: a.getAttribute("data-game"), title: n ? n.textContent : a.getAttribute("data-game") };
    });
    return out;
  }
  function kindOf(key) {
    var p = pathOf(key);
    if (p === "" || p === "index.html") return "index";
    if (/\.txt$/i.test(p)) return "txt";
    if (games()[p]) return "game";
    if (/\.html$/i.test(p) || /\/$/.test(p)) return "page";
    return null;
  }
  function frameUrl(key) {
    var u = new URL(key, SITE);
    u.searchParams.set("embed", "1");
    if (SILENT) u.searchParams.set("silent", "1");
    // the performance panel asked for in the address (?perf=1, web/perf.js): in every game window of this visit
    if (Q.get("perf") === "1") u.searchParams.set("perf", "1");
    return u.href;
  }
  // the address in the bar, whatever window is open: the site's root, with the test parameters kept (a reload of a test run
  // stays silent)
  function rootAddress() {
    var t = SKIN && SKIN.testQuery ? SKIN.testQuery() : (SILENT ? "silent=1" : "");
    return SITE + (t ? "?" + t : "");
  }
  function byKey(key) { for (var i = 0; i < apps.length; i++) if (apps[i].key === key) return apps[i]; return null; }
  function byId(id) { for (var i = 0; i < apps.length; i++) if (apps[i].id === id) return apps[i]; return null; }
  function byWin(win) { for (var i = 0; i < apps.length; i++) if (apps[i].frame && apps[i].frame.contentWindow === win) return apps[i]; return null; }
  function gameCount() { return apps.filter(function (a) { return a.kind === "game"; }).length; }

  // ---- how many games at once (docs/WEB_PLAYER.md, "Desktop 95 in one page"; the user, 2026-11-16: "Make it a warning") ----
  // Every game window is a frame of this one page, so they all share one browser process. When it runs out of memory the
  // browser shows its error in place of the whole page (an iPad reloads it), and every open game closes at once. So each
  // game has a weight and the browser a budget, and a game that would take the open games' total past the budget asks
  // first. A weight is the game's own memory at a first board in 100 MB, rounded, at least 1: docs/MEMORY.md, "The web
  // builds", "At a first board", its renderer private bytes less about 60 MB for the page itself. Bejeweled Twist (about
  // 400 MB) is 4 and Bookworm Adventures 2 (about 190 MB) is 2; every other game measured comes to 1, and a game not
  // measured counts 1.
  var WEIGHTS = { bejeweledtwist: 4, bookwormadv2: 2 };
  function weightOf(id) { return WEIGHTS[id] || 1; }
  function gamesWeight() {
    return apps.reduce(function (n, a) { return n + (a.kind === "game" ? weightOf(a.game) : 0); }, 0);
  }
  // 6, or less where the browser says how much memory the device has (navigator.deviceMemory, only Chromium's, which says 8
  // for "8 GB or more", or in newer ones up to 32): 3 at 4 GB, 1 at 2 GB or less. Safari and Firefox say nothing, so 6.
  function budget() {
    var gb = navigator.deviceMemory;
    if (typeof gb !== "number" || !(gb > 0) || gb >= 8) return 6;
    return gb >= 4 ? 3 : 1;
  }

  // ---- the address ----
  function state() {
    var act = desk.active(), a = act && byId(act);
    return { d95: true, open: apps.filter(function (x) { return !x.note; }).map(function (x) { return x.key; }), active: a && !a.note ? a.key : null };
  }
  function write(push) {
    if (restoring) return;
    var st = state();
    try { history[push ? "pushState" : "replaceState"](st, "", rootAddress()); } catch (e) {}
  }

  // ---- a Windows 95 message box (a game past the memory budget, an About box): the desktop's (web/skins.js) ----
  function messageBox(title, text, plain, buttons) { desk.messageBox(title, text, plain, buttons); }

  // ---- a window ----
  // note: { title, text }, a text window of text the page gave (a game's readme), which the address does not keep
  function make(key, kind, note) {
    var id = "app-" + (++seq), g = kind === "game" ? games()[pathOf(key)] : null;
    var app = { id: id, key: key, kind: kind, frame: null, el: null, game: g ? g.id : "", note: !!note };
    var el = document.createElement("section");
    el.className = "d95-app d95-app-" + kind;
    var title = note ? note.title : g ? g.title : pathOf(key).split("/").filter(Boolean).pop() || key;
    app.title = title;
    // a game's menu bar, as a Windows 95 program's (the user, 2026-11-09: "don't hide everything under "Game""): Game, View,
    // Options and Help (barMenus), each with its access key, the first letter underlined (Alt and the letter, menuKey)
    if (kind === "game") {
      var bar = document.createElement("div"); bar.className = "d95-menubar"; bar.setAttribute("role", "menubar");
      app.bar = [];
      BAR.forEach(function (m, i) {
        var b = document.createElement("button"); b.type = "button"; b.className = "d95-menubar-item"; b.setAttribute("role", "menuitem");
        b.setAttribute("aria-haspopup", "menu"); b.tabIndex = i ? -1 : 0;
        var name = T(m.key), u = document.createElement("u");
        u.textContent = name.charAt(0);
        b.appendChild(u); b.appendChild(document.createTextNode(name.slice(1)));
        b.setAttribute("aria-keyshortcuts", "Alt+" + name.charAt(0).toUpperCase());
        app.bar.push({ el: b, letter: name.charAt(0).toLowerCase() });
        b.addEventListener("click", function () { openBar(app, i); });
        bar.appendChild(b);
      });
      el.appendChild(bar);
    }
    var client = document.createElement("div"); client.className = "d95-app-client";
    if (kind === "txt") {
      var pre = document.createElement("pre"); pre.className = "d95-app-text"; pre.tabIndex = 0;
      pre.setAttribute("aria-label", title);
      client.appendChild(pre);
      if (note) pre.textContent = note.text;
      else fetch(new URL(key, SITE).href).then(function (r) { return r.ok ? r.text() : Promise.reject(new Error(r.status)); })
        .then(function (t) { pre.textContent = t; }, function () { pre.textContent = T("skin.apps.unreadable"); });
      app.text = pre;
    } else {
      var f = document.createElement("iframe");
      f.className = "d95-app-frame"; f.title = title; f.src = frameUrl(key);
      f.setAttribute("allow", "fullscreen; autoplay; clipboard-write");
      client.appendChild(f);
      app.frame = f;
      f.addEventListener("load", function () { app.loaded = true; }, { once: true });
      // a page's own title, once it has loaded
      if (kind === "page") f.addEventListener("load", function () {
        try { var t = f.contentDocument && f.contentDocument.title; if (t) { desk.setTitle(id, t); f.title = t; } } catch (e) {}
      });
    }
    el.appendChild(client);
    document.body.appendChild(el);
    app.el = el;
    var n = apps.length;
    // the title bar's icon, which opens the control menu: a game's cap, a document's page
    var icon = document.createElement("i");
    icon.className = kind === "game" ? "d95-sys-cap" : "d95-sys-doc"; icon.setAttribute("aria-hidden", "true");
    desk.add(el, { id: id, keep: "app:" + pathOf(key), title: title, closable: true, menu: true, menuIcon: icon, noIcon: true,
      onClose: function () { closeApp(app); },
      maximize: kind === "game" ? function () { maximize(app); } : null,
      // a game's window is never dragged smaller than its game (the user, 2026-11-09: "Don't allow a window to be dragged smaller
      // than the game inside it"): the game at the multiple it is drawn at (1x with Fit window), its status bar and the frame
      minWidth: kind === "game" ? function () { return least(app).w; } : null,
      minHeight: kind === "game" ? function () { return least(app).h; } : null,
      at: function (left) { return [left + 40 + (n % 6) * 26, 20 + (n % 6) * 26]; },
      scroll: kind === "txt" ? app.text : null,
      // a game's or a page's frame has no height of its own to measure: those windows go to the room; a text's to its length
      natural: kind === "txt",
      focusIn: function (back) { focusApp(app, back); } });
    apps.push(app);
    if (kind === "game") watchBar();
    return app;
  }
  function inner(app) { try { return app.frame && app.frame.contentWindow; } catch (e) { return null; } }
  // the smallest a game's window may be: what its page needs (web/shell.html ByopopEmbed.least) and the window's own frame
  function least(app) {
    var w = inner(app), need = null;
    try { need = w && w.ByopopEmbed && w.ByopopEmbed.least(); } catch (e) {}
    if (!need || !app.frame) return { w: 0, h: 0 };
    return { w: Math.ceil(need.w + app.el.offsetWidth - app.frame.clientWidth), h: Math.ceil(need.h + app.el.offsetHeight - app.frame.clientHeight) };
  }
  // The menus: Game (what acts on the game: its files, its controls, Close), View (its scales, full screen, the view options
  // and the chat), Options (mods, the game's settings) and Help (the terms, copyright and licences; About). The page in the
  // window gives its own parts (ByopopEmbed.menus, web/shell.html); the desktop adds what is its own.
  var BAR = [{ key: "skin.apps.game_menu", part: "game" }, { key: "skin.apps.view_menu", part: "view" },
             { key: "skin.apps.options_menu", part: "options" }, { key: "skin.apps.help_menu", part: "help" }];
  function barMenus(app) {
    var w = inner(app), own = {};
    try { own = w.ByopopEmbed.menus() || {}; } catch (e) {}
    function part(name) { return (own[name] || []).slice(); }
    var game = part("game");
    if (game.length) game.push({ rule: true });
    // Close shows no key: Alt+F4 never reaches a page, as Windows closes the browser's window with it (the user, 2026-11-09)
    game.push({ label: T("skin.apps.close"), fn: function () { closeApp(app); } });
    var view = part("view");
    if (view.length) view.push({ rule: true });
    view.push({ label: T("skin.pop.full"), fn: function () { fullScreen(app); } });
    // the desktop's one chat window (web/index.html), which follows the game in front
    if (desk.find("chat")) view.push({ label: desk.find("chat").el.getAttribute("data-d95-title"), fn: function () { desk.openWin("chat"); } });
    var options = part("options");
    // the game's terms, copyright and licenses (its page's footer links to them, and a window shows no footer), and About
    var help = [], terms = null;
    try { terms = w.document.querySelector("a[href$='-copyright.html']"); } catch (e) {}
    if (terms) help.push({ label: T("skin.apps.terms"), fn: function () { open(terms.href, "push"); } });
    // the game's own readme, in a text window (the user, 2026-11-14, by the coordinator's list), named by its file
    var readme = null;
    try { readme = w.ByopopEmbed.readme ? w.ByopopEmbed.readme() : null; } catch (e) {}
    if (readme) help.push({ label: readme.name, fn: function () { openNote("readme:" + app.key, readme.name, readme.text); } });
    var about = null;
    try { about = w.ByopopEmbed.about(); } catch (e) {}
    if (about) {
      if (help.length) help.push({ rule: true });
      help.push({ label: T("skin.apps.about", { game: about.title }), fn: function () { messageBox(T("skin.apps.about", { game: about.title }), about.text, true); } });
    }
    return { game: game, view: view, options: options, help: help };
  }
  function openBar(app, i) {
    var m = BAR[i], b = app.bar[i].el, r = b.getBoundingClientRect();
    var list = barMenus(app)[m.part];
    if (!list.length) list = [{ label: T("skin.apps.empty"), disabled: true, fn: function () {} }];
    app.bar.forEach(function (x, k) { x.el.classList.toggle("d95-open", k === i); });
    desk.menu(b, list, r.left, r.bottom, T(m.key), {
      side: function (dir) { openBar(app, (i + dir + BAR.length) % BAR.length); },
      // closed with nothing chosen (Escape): the keys go back to the game, as a program's went back to its document
      done: function () {
        app.bar.forEach(function (x) { x.el.classList.remove("d95-open"); });
        setTimeout(function () {
          if (app.bar.some(function (x) { return x.el === document.activeElement; }) && !document.querySelector(".d95-menu.d95-ctx")) focusApp(app);
        }, 0);
      }
    });
  }
  // Alt and a menu's letter, from the desktop or from the page in the window (which gets the keys while its game has them)
  function menuKey(win, ev) {
    var a = win ? byWin(win) : byId(desk.active());
    if (!a || !a.bar || !ev.altKey || ev.ctrlKey || ev.metaKey || !ev.key || ev.key.length !== 1) return false;
    var k = ev.key.toLowerCase();
    for (var i = 0; i < a.bar.length; i++) if (a.bar[i].letter === k) { openBar(a, i); return true; }
    return false;
  }
  document.addEventListener("keydown", function (ev) {
    if (on() && !ev.defaultPrevented && menuKey(null, ev)) { ev.preventDefault(); ev.stopPropagation(); }
  }, true);
  // the game alone in full screen: the window's frame, which the page in it draws as its stage alone (web/shell.html frameFull)
  function fullScreen(app) { try { app.frame.requestFullscreen(); } catch (e) {} }
  // the window's maximize: the whole desktop above the task bar, and back (the page in it scales its game to fit, web/shell.html)
  function maximize(app) {
    var m = !app.el.classList.contains("d95-maxed"), w = desk.find(app.id);
    // its place, which the desktop would otherwise move to fit a window as wide as itself, comes back with it
    if (m && w) app.restoreAt = [w.x, w.y, w.placed];
    app.el.classList.toggle("d95-maxed", m);
    try { desk.event(m ? "maximize" : "restore"); } catch (e) {}
    if (!m && w && app.restoreAt) { w.x = app.restoreAt[0]; w.y = app.restoreAt[1]; w.placed = app.restoreAt[2]; }
    desk.place(app.id);
  }
  function focusApp(app, back) {
    if (app.text) { app.text.focus(); return; }
    var w = inner(app);
    if (!w) return;
    try { w.focus(); } catch (e) {}
    try {
      if (w.ByopopSkin && w.ByopopSkin.desk.isOn()) { w.ByopopSkin.desk.enter(back); return; }
      var first = w.document.querySelector("a[href], button, input, select, textarea, [tabindex]:not([tabindex='-1'])");
      if (first) first.focus();
    } catch (e) {}
  }
  // Close: a game's own way out (its saves first, web/shell.html closeProgram), which then hands its window back (closed)
  // Closing never depends on the page in the window (the user, 2026-11-11: a window whose game failed to load could not be
  // closed): a frame that never loaded, or an error page the desktop may not read, has no ByopopEmbed and goes at once; a page
  // that takes its close but never hands the window back (a game that stopped answering, a save that never ends) is dropped
  // after CLOSE_WAIT, and a second Close drops it at once.
  var CLOSE_WAIT = 4000;
  function closeApp(app) {
    var embed = null;
    if (app.kind === "game" && !app.closing) {
      try { var w = inner(app); embed = w && w.ByopopEmbed && typeof w.ByopopEmbed.close === "function" ? w.ByopopEmbed : null; } catch (e) {}
    }
    if (embed) {
      app.closing = true;
      try {
        embed.close();
        app.closeTimer = setTimeout(function () { drop(app); }, CLOSE_WAIT);
        return;
      } catch (e) {}
    }
    drop(app);
  }
  function drop(app) {
    var i = apps.indexOf(app);
    if (i < 0) return;
    if (app.closeTimer) clearTimeout(app.closeTimer);
    apps.splice(i, 1);
    if (lastGame === app) lastGame = null;
    desk.remove(app.id);
    write(false);
  }

  // a text window of text a page gave (a game's readme): one for each key, brought to the front when it is open already
  function openNote(key, title, text) {
    if (!on()) return false;
    desk.closeMenus();
    var app = byKey(key) || make(key, "txt", { title: title, text: text });
    var was = restoring;
    restoring = true;
    try { desk.openWin(app.id); } finally { restoring = was; }
    return true;
  }

  // ---- opening ----
  // how: "push" (the player opened it: a new address), "replace" (this address), "none" (Back or Forward is putting it back);
  // anyway: the player answered Open anyway to the memory warning
  function open(href, how, anyway) {
    if (!on()) return false;
    var key = relOf(href);
    if (key === null) return false;
    var kind = kindOf(key);
    if (!kind) return false;
    desk.closeMenus();
    if (kind === "index") { desk.openWin("library"); return true; }
    var app = byKey(key);
    if (!app) {
      // past the budget, the player is asked each time, unless no other game is open (nothing else to lose) or windows are
      // being put back (a reload, Back or Forward: they were open before)
      var g = kind === "game" ? games()[pathOf(key)] : null;
      if (g && !anyway && !restoring && gameCount() > 0 && gamesWeight() + weightOf(g.id) > budget()) {
        messageBox(T("skin.apps.limit_title"), T("skin.apps.limit"), false, [
          { label: T("skin.apps.limit_open"), fn: function () { open(href, how, true); } },
          { label: T("skin.apps.limit_cancel") }
        ]);
        return true;
      }
      app = make(key, kind);
    }
    // the window comes to the front without touching the address (the front window's replace would write the new window's state
    // over the entry the player is leaving, so Back would find it again); the address is written once, below
    var was = restoring;
    restoring = true;
    try { desk.openWin(app.id); } finally { restoring = was; }
    if (how === "push") write(true); else if (how === "replace") write(false);
    return true;
  }

  // ---- what the pages in the windows tell the desktop ----
  function activated(win) { var a = byWin(win); if (a && desk.active() !== a.id) desk.activate(a.id); }
  function f6(win, back) { var a = byWin(win); if (a) desk.f6From(a.id, back); }
  // Alt+` from a window's page (web/skins.js): the switch between the desktop's windows
  function altTab(win, how) { if (desk.altTab) desk.altTab(how); }
  // Alt+4 and Shut Down from a window's page (web/skins.js): the desktop's front window closes, or the desktop shuts down
  function closeFront(win) { if (desk.closeFront) desk.closeFront(); }
  function shutDown(win) { if (desk.shutDown) desk.shutDown(); }
  function closed(win) { var a = byWin(win); if (a) drop(a); }
  function title(win, text) { var a = byWin(win); if (a && text) { desk.setTitle(a.id, text); if (a.frame) a.frame.title = text; } }
  // the largest size a window's page can have on the desktop, above the task bar (the scales its View menu offers)
  function room(win) {
    var a = byWin(win);
    if (!a) return null;
    var inner = a.frame || a.el.querySelector(".d95-app-client");
    var cw = a.el.offsetWidth - inner.clientWidth, ch = a.el.offsetHeight - inner.clientHeight;
    var bar = document.querySelector(".d95-taskbar");
    return { w: (document.documentElement.clientWidth || window.innerWidth) / zf() - 16 - cw, h: window.innerHeight / zf() - (bar ? bar.offsetHeight : 28) - 8 - ch };
  }
  // The room changed: the browser window's size (full screen too), the desktop's zoom or the task bar's height. Each game's page
  // sizes its game again (web/shell.html, byopop-room), since its window, keeping its size, gives it no resize of its own: a
  // multiple chosen for a game that the room had no space for comes back once it has (the user, 2026-11-20, of a game that "tried
  // to open larger than the window had room for"). Once, at the next frame (or 200 ms on, where a hidden page draws none), after
  // the desktop has laid its windows out again.
  var roomQueued = false, barWatch = null;
  function roomChanged() {
    if (roomQueued) return;
    roomQueued = true;
    var timer = 0, frame = 0;
    function run() {
      if (!roomQueued) return;
      roomQueued = false;
      clearTimeout(timer);
      try { cancelAnimationFrame(frame); } catch (e) {}
      apps.forEach(function (a) { var w = a.kind === "game" && inner(a); if (w) try { w.dispatchEvent(new Event("byopop-room")); } catch (e) {} });
    }
    timer = setTimeout(run, 200);
    try { frame = requestAnimationFrame(run); } catch (e) {}
  }
  window.addEventListener("resize", roomChanged);
  window.addEventListener("byopop-zoom", roomChanged);
  function watchBar() {
    var bar = document.querySelector(".d95-taskbar");
    if (barWatch || !bar || !window.ResizeObserver) return;
    barWatch = new ResizeObserver(roomChanged);
    barWatch.observe(bar);
  }
  // the size a game wants (its screen at one pixel for each of the device's, and its window's frame and status bar): the window
  // takes it the first time, as far as the desktop has room
  // (force: size the window again even to a size it had before; chosen: the player chose the scale, which restores a
  // maximized window first)
  function fit(win, w, h, force, chosen) {
    var a = byWin(win);
    if (!a || narrow()) return;
    fitWindow(a, w, h, force, chosen);
    // (the desktop's loading waits for it, start)
    if (a.onFit) { var told = a.onFit; a.onFit = null; told(); }
  }
  function fitWindow(a, w, h, force, chosen) {
    // a size the player chose restores a maximized window first (its place comes back), as Windows did
    if (chosen && a.el.classList.contains("d95-maxed")) maximize(a);
    if (a.el.classList.contains("d95-maxed") || (!force && a.fitted && a.fitted[0] === w && a.fitted[1] === h)) return;
    a.fitted = [w, h];
    var inner = a.frame || a.el.querySelector(".d95-app-client");
    var cw = a.el.offsetWidth - inner.clientWidth, ch = a.el.offsetHeight - inner.clientHeight;
    var bar = document.querySelector(".d95-taskbar");
    var roomW = (document.documentElement.clientWidth || window.innerWidth) / zf() - 16, roomH = window.innerHeight / zf() - (bar ? bar.offsetHeight : 28) - 8;
    a.el.style.width = Math.min(roomW, Math.round(w + cw)) + "px";
    a.el.style.height = Math.min(roomH, Math.round(h + ch)) + "px";
    desk.place(a.id);
  }

  // ---- the address, Back and Forward ----
  window.addEventListener("byopop-desk-active", function (ev) {
    // no window active (the Pop menu or the desktop has the press): the address stays the last window's
    if (on() && !restoring && ev.detail && ev.detail.id) write(false);
    // a window made active by its title bar, its task bar button or the Pop menu takes the focus, as a window did in Windows:
    // the page in its frame (a game: its game), once the press that made it active has done with the focus
    var app = byId(ev.detail && ev.detail.id);
    if (app && app.kind === "game") lastGame = app;
    if (app) setTimeout(function () {
      if (desk.active() !== app.id) return;
      var at = document.activeElement;
      if (app.text ? app.text.contains(at) : at === app.frame) return;
      if (app.kind === "game" || app.text) focusApp(app);
      else { var w = inner(app); try { if (w) w.focus(); } catch (e) {} }
    }, 0);
  });
  window.addEventListener("popstate", function (ev) {
    var st = ev.state;
    if (!on() || !st || !st.d95) return;
    restoring = true;
    try {
      apps.slice().forEach(function (a) { if (st.open.indexOf(a.key) < 0) closeApp(a); });
      st.open.forEach(function (k) { if (!byKey(k)) open(SITE + k, "none"); });
      var act = st.active && byKey(st.active);
      if (act) desk.openWin(act.id);
    } finally { restoring = false; }
  });
  // our own links on the desktop (the icons' and menus' are links too): a window instead of a page
  document.addEventListener("click", function (ev) {
    var a = ev.target && ev.target.closest && ev.target.closest("a[href]");
    if (!a || ev.defaultPrevented || ev.button !== 0 || ev.ctrlKey || ev.metaKey || ev.shiftKey || ev.altKey) return;
    if ((a.getAttribute("href") || "").charAt(0) === "#" || a.hasAttribute("download")) return;
    if (open(a.href, "push")) ev.preventDefault();
  });
  // the first state: the windows a reload finds in history.state, open again, and the page an old address named (?open=, from
  // web/skins.js onePage); then the root's address
  function start() {
    var want = Q.get("open"), st = history.state;
    if (!on()) { bootDone(); return; }
    if (st && st.d95 && st.open && st.open.length) {
      restoring = true;
      try {
        st.open.forEach(function (k) { if (!byKey(k)) open(SITE + k, "none"); });
        var act = st.active && byKey(st.active);
        if (act) desk.openWin(act.id);
      } finally { restoring = false; }
    }
    if (want !== null) {
      try { history.replaceState(state(), "", rootAddress()); } catch (e) {}
      open(SITE + want, "replace");
    }
    write(false);
    // the desktop shows once each window put back has its page, and a game's window its game's size (fit), which its page
    // gives once it knows it: until then, or for 3 s after its page has loaded
    var left = 0;
    apps.forEach(function (a) {
      if (!a.frame) return;
      left++;
      var done = false;
      function finish() { if (done) return; done = true; if (--left === 0) bootDone(); }
      function loaded() { if (a.kind !== "game" || a.fitted) finish(); else { a.onFit = finish; setTimeout(finish, 3000); } }
      if (a.loaded) loaded(); else a.frame.addEventListener("load", loaded, { once: true });
    });
    if (!left) bootDone();
  }
  if (document.readyState === "complete") setTimeout(start, 0); else window.addEventListener("load", function () { setTimeout(start, 0); });

  // The game the chat speaks from (web/index.html): the game window last in front, else the newest game still open, else none
  var lastGame = null;
  function frontGame() {
    if (lastGame && apps.indexOf(lastGame) >= 0) return lastGame.game;
    for (var i = apps.length - 1; i >= 0; i--) if (apps[i].kind === "game") return apps[i].game;
    return "";
  }
  // the game window in front, for the Music window's views (web/music.js, "Following a game"): the game window last made active,
  // while it is open and not minimized; its page, its game's title and its game's id, or null
  function frontFrame() {
    var a = lastGame && apps.indexOf(lastGame) >= 0 ? lastGame : null;
    if (!a || !a.el || a.el.classList.contains("d95-min") || a.el.classList.contains("d95-closed")) return null;
    var w = inner(a);
    return w ? { win: w, title: a.title || "", game: a.game || "" } : null;
  }
  // ---- one window heard at a time (the user, 2026-11-10: "keep in mind the need to prevent multiple windows from playing audio at
  // once") ----
  // A game does with the focus what its original does: most mute themselves when another window is active, some keep sounding.
  // Of those that would sound, only one is let through: the game in front; with no game in front, none while Music plays a tune of
  // its own, else the game that was last in front. The others' output node is taken off their speakers (not their page's mute,
  // which is the player's own switch) and put back when their turn comes; the game itself runs on and is told nothing.
  // the game windows open, for System Information (web/sysinfo.js): each one's window id, its game's title, its page
  function running() {
    return apps.filter(function (a) { return a.kind === "game" && !a.note; }).map(function (a) { return { id: a.id, title: a.title || "", game: a.game || "", win: inner(a) }; });
  }
  function musicPlaying() { try { return !!(window.ByopopMusicPanel && ByopopMusicPanel.playing && ByopopMusicPanel.playing()); } catch (e) { return false; } }
  function heardApp() {
    var act = desk.active(), a = act && byId(act);
    if (a && a.kind === "game") return a;
    if (musicPlaying()) return null;
    return lastGame && apps.indexOf(lastGame) >= 0 ? lastGame : null;
  }
  function hush() {
    var one = heardApp();
    apps.forEach(function (a) {
      if (a.kind !== "game") return;
      var want = a !== one;
      try {
        var w = inner(a), out = w && w.Module && w.Module.byopopAudio, node = out && (out.master || out.node);
        if (!node) return;
        if (want && a.hushedNode !== node) { node.disconnect(node.context.destination); a.hushedNode = node; }
        else if (!want && a.hushedNode) { if (a.hushedNode === node) node.connect(node.context.destination); a.hushedNode = null; }
      } catch (e) {}
    });
  }
  window.addEventListener("byopop-desk-active", function () { setTimeout(hush, 0); });
  // a game's page put its sound through its own gain (the site's volume, web/shell.html): heard or hushed again by that
  function rehush() { hush(); }
  window.addEventListener("byopop-music-state", hush);
  // (a game's output is made when it starts, some time after its window opens)
  setInterval(function () { if (gameCount()) hush(); }, 1000);
  function hushed(win) { var a = byWin(win); return !!(a && a.hushedNode); }

  // a question from a window's page (a game's Game menu, web/shell.html ask): the desktop's message box, titled by the window
  function ask(win, text, buttons) { var a = byWin(win); messageBox(a ? a.title : "", text, false, buttons); }
  // a message that tells rather than warns (the listing's "Why aren't my files here?"): the information sign
  function tell(title, content, buttons) { messageBox(title, content, "info", buttons); }
  // and one that warns or asks (the Saves window's question before a restore, web/saves.js): the warning sign
  function warn(title, content, buttons) { messageBox(title, content, false, buttons); }

  return { hushed: hushed, heard: function () { var a = heardApp(); return a ? a.key : null; }, open: open, ask: ask, tell: tell, warn: warn, activated: activated, f6: f6, altTab: altTab, closeFront: closeFront, shutDown: shutDown, rehush: rehush, closed: closed, title: title, fit: fit, room: room, frontGame: frontGame, frontFrame: frontFrame,
    running: running, menuKey: menuKey, fullScreen: function (win) { var a = byWin(win); if (a) fullScreen(a); },
    count: function () { return apps.length; },
    // the address back to the root (a link to #settings and the like, web/index.html)
    settle: function () { if (on()) write(false); } };
})();
