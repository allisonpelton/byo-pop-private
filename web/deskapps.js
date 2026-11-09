// Desktop 95 in one page (docs/WEB_PLAYER.md, "Desktop 95 in one page"; the user, 2026-11-08: "Make the Windows 95 skin work
// entirely in a single page", and "Would it be possible for that skin to support multiple games open at once in the same
// window?"). On the listing in Desktop 95 the player never leaves the desktop: every page of ours a link, an icon or a menu
// leads to opens as a window on it.
//   - a game: its page in a frame (?embed=1: the game, its setup cards and its own windows, no desktop of its own), with a
//     menu bar for those windows; at most three at once (one on a narrow screen), each with its own heap, files, sound and
//     keys. Only the active window's game has the focus, so the others get the focus loss they get when a page loses it.
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
  var MAX_GAMES = 3, NARROW = 760;
  var apps = [], seq = 0, restoring = false;
  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function on() { return !!(desk && desk.isOn()); }
  function narrow() { return (document.documentElement.clientWidth || window.innerWidth) < NARROW; }

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

  // ---- the address ----
  function state() {
    var act = desk.active(), a = act && byId(act);
    return { d95: true, open: apps.map(function (x) { return x.key; }), active: a ? a.key : null };
  }
  function write(push) {
    if (restoring) return;
    var st = state();
    try { history[push ? "pushState" : "replaceState"](st, "", rootAddress()); } catch (e) {}
  }

  // ---- a Windows 95 message box (the fourth game) ----
  function messageBox(title, text, plain) {
    var d = document.createElement("dialog");
    d.className = "d95-msgbox";
    d.setAttribute("aria-labelledby", "d95-msgbox-title"); d.setAttribute("aria-describedby", "d95-msgbox-text");
    var tb = document.createElement("div"); tb.className = "d95-tb d95-msgbox-tb";
    var tt = document.createElement("span"); tt.className = "d95-tb-text"; tt.id = "d95-msgbox-title"; tt.textContent = title;
    tb.appendChild(tt);
    var body = document.createElement("div"); body.className = "d95-msgbox-body";
    var icon = document.createElement("i"); icon.className = "d95-msgbox-icon"; icon.setAttribute("aria-hidden", "true");
    var p = document.createElement("p"); p.id = "d95-msgbox-text"; p.textContent = text;
    // an About box: the game's lines as they are, without the warning sign
    if (plain) { p.style.whiteSpace = "pre-line"; d.classList.add("d95-msgbox-about"); } else body.appendChild(icon);
    body.appendChild(p);
    var ok = document.createElement("button"); ok.type = "button"; ok.textContent = T("skin.apps.ok");
    var row = document.createElement("div"); row.className = "d95-msgbox-row"; row.appendChild(ok);
    d.appendChild(tb); d.appendChild(body); d.appendChild(row);
    document.body.appendChild(d);
    function done() { try { d.close(); } catch (e) {} if (d.parentNode) d.parentNode.removeChild(d); }
    ok.addEventListener("click", done);
    d.addEventListener("keydown", function (ev) { if (ev.key === "Escape") { ev.preventDefault(); ev.stopPropagation(); done(); } });
    d.addEventListener("cancel", function (ev) { ev.preventDefault(); done(); });
    d.showModal();
    ok.focus();
  }

  // ---- a window ----
  function make(key, kind) {
    var id = "app-" + (++seq), g = kind === "game" ? games()[pathOf(key)] : null;
    var app = { id: id, key: key, kind: kind, frame: null, el: null, game: g ? g.id : "" };
    var el = document.createElement("section");
    el.className = "d95-app d95-app-" + kind;
    var title = g ? g.title : pathOf(key).split("/").filter(Boolean).pop() || key;
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
      fetch(new URL(key, SITE).href).then(function (r) { return r.ok ? r.text() : Promise.reject(new Error(r.status)); })
        .then(function (t) { pre.textContent = t; }, function () { pre.textContent = T("skin.apps.unreadable"); });
      app.text = pre;
    } else {
      var f = document.createElement("iframe");
      f.className = "d95-app-frame"; f.title = title; f.src = frameUrl(key);
      f.setAttribute("allow", "fullscreen; autoplay; clipboard-write");
      client.appendChild(f);
      app.frame = f;
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
    desk.add(el, { id: id, title: title, closable: true, menu: true, menuIcon: icon, noIcon: true,
      onClose: function () { closeApp(app); },
      maximize: kind === "game" ? function () { maximize(app); } : null,
      // a game's window is never dragged smaller than its game (the user, 2026-11-09: "Don't allow a window to be dragged smaller
      // than the game inside it"): the game at the multiple it is drawn at (1x with Fit window), its status bar and the frame
      minWidth: kind === "game" ? function () { return least(app).w; } : null,
      minHeight: kind === "game" ? function () { return least(app).h; } : null,
      at: function (left) { return [left + 40 + (n % 6) * 26, 20 + (n % 6) * 26]; },
      scroll: kind === "txt" ? app.text : null,
      focusIn: function (back) { focusApp(app, back); } });
    apps.push(app);
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
  function closeApp(app) {
    var w = inner(app);
    if (app.kind === "game" && w && w.ByopopEmbed && !app.closing) {
      app.closing = true;
      try { w.ByopopEmbed.close(); return; } catch (e) {}
    }
    drop(app);
  }
  function drop(app) {
    var i = apps.indexOf(app);
    if (i < 0) return;
    apps.splice(i, 1);
    if (lastGame === app) lastGame = null;
    desk.remove(app.id);
    write(false);
  }

  // ---- opening ----
  // how: "push" (the player opened it: a new address), "replace" (this address), "none" (Back or Forward is putting it back)
  function open(href, how) {
    if (!on()) return false;
    var key = relOf(href);
    if (key === null) return false;
    var kind = kindOf(key);
    if (!kind) return false;
    desk.closeMenus();
    if (kind === "index") { desk.openWin("games"); return true; }
    var app = byKey(key);
    if (!app) {
      var limit = narrow() ? 1 : MAX_GAMES;
      if (kind === "game" && gameCount() >= limit) {
        messageBox(T("skin.apps.limit_title"), T(limit === 1 ? "skin.apps.limit_one" : "skin.apps.limit", { n: limit }));
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
  function closed(win) { var a = byWin(win); if (a) drop(a); }
  function title(win, text) { var a = byWin(win); if (a && text) { desk.setTitle(a.id, text); if (a.frame) a.frame.title = text; } }
  // the largest size a window's page can have on the desktop, above the task bar (the scales its View menu offers)
  function room(win) {
    var a = byWin(win);
    if (!a) return null;
    var inner = a.frame || a.el.querySelector(".d95-app-client");
    var cw = a.el.offsetWidth - inner.clientWidth, ch = a.el.offsetHeight - inner.clientHeight;
    var bar = document.querySelector(".d95-taskbar");
    return { w: (document.documentElement.clientWidth || window.innerWidth) - 16 - cw, h: window.innerHeight - (bar ? bar.offsetHeight : 28) - 8 - ch };
  }
  // the size a game wants (its screen at one pixel for each of the device's, and its window's frame and status bar): the window
  // takes it the first time, as far as the desktop has room
  // (force: size the window again even to a size it had before; chosen: the player chose the scale, which restores a
  // maximized window first)
  function fit(win, w, h, force, chosen) {
    var a = byWin(win);
    if (!a || narrow()) return;
    // a size the player chose restores a maximized window first (its place comes back), as Windows did
    if (chosen && a.el.classList.contains("d95-maxed")) maximize(a);
    if (a.el.classList.contains("d95-maxed") || (!force && a.fitted && a.fitted[0] === w && a.fitted[1] === h)) return;
    a.fitted = [w, h];
    var inner = a.frame || a.el.querySelector(".d95-app-client");
    var cw = a.el.offsetWidth - inner.clientWidth, ch = a.el.offsetHeight - inner.clientHeight;
    var bar = document.querySelector(".d95-taskbar");
    var roomW = (document.documentElement.clientWidth || window.innerWidth) - 16, roomH = window.innerHeight - (bar ? bar.offsetHeight : 28) - 8;
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
    if (!on()) return;
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
  // while it is open and not minimized; its page and its game's title, or null
  function frontFrame() {
    var a = lastGame && apps.indexOf(lastGame) >= 0 ? lastGame : null;
    if (!a || !a.el || a.el.classList.contains("d95-min") || a.el.classList.contains("d95-closed")) return null;
    var w = inner(a);
    return w ? { win: w, title: a.title || "" } : null;
  }
  return { open: open, activated: activated, f6: f6, closed: closed, title: title, fit: fit, room: room, frontGame: frontGame, frontFrame: frontFrame,
    menuKey: menuKey, fullScreen: function (win) { var a = byWin(win); if (a) fullScreen(a); },
    count: function () { return apps.length; },
    // the address back to the root (a link to #settings and the like, web/index.html)
    settle: function () { if (on()) write(false); } };
})();
