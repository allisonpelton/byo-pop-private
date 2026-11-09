// Desktop 95 in one page (docs/WEB_PLAYER.md, "Desktop 95 in one page"; the user, 2026-11-08: "Make the Windows 95 skin work
// entirely in a single page", and "Would it be possible for that skin to support multiple games open at once in the same
// window?"). On the listing in Desktop 95 the player never leaves the desktop: every page of ours a link, an icon or a menu
// leads to opens as a window on it.
//   - a game: its page in a frame (?embed=1: the game, its setup cards and its own windows, no desktop of its own), with a
//     menu bar for those windows; at most three at once (one on a narrow screen), each with its own heap, files, sound and
//     keys. Only the active window's game has the focus, so the others get the focus loss they get when a page loses it.
//   - another page of ours (a game's copyright and licences, the terms): its page in a frame, scrolling with the skin's bars.
//   - a licence text (licenses/*.txt): the text itself in the window.
// Links to other sites still open a tab. The address follows the active window (pushState for a window the player opens,
// replaceState as the active one changes), Back and Forward close or reopen windows without leaving the page, and any of
// those addresses opened on its own comes back here with its window open (web/skins.js, onePage). Other skins keep their pages.
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
  // the address a window has in the bar: its own, with ?silent=1 kept (a reload of a test run stays silent)
  function addressOf(key) {
    if (!key) return SITE + (SILENT ? "?silent=1" : "");
    // a page in a folder (a licence text): the listing's address naming it, as the page's own address would move the
    // document's base, and every relative link and later load of the listing with it
    if (pathOf(key).indexOf("/") >= 0) return SITE + "?open=" + encodeURIComponent(key) + (SILENT ? "&silent=1" : "");
    var u = new URL(key, SITE);
    if (SILENT) u.searchParams.set("silent", "1");
    return u.href;
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
    try { history[push ? "pushState" : "replaceState"](st, "", addressOf(st.active)); } catch (e) {}
  }

  // ---- a Windows 95 message box (the fourth game) ----
  function messageBox(title, text) {
    var d = document.createElement("dialog");
    d.className = "d95-msgbox";
    d.setAttribute("aria-labelledby", "d95-msgbox-title"); d.setAttribute("aria-describedby", "d95-msgbox-text");
    var tb = document.createElement("div"); tb.className = "d95-tb d95-msgbox-tb";
    var tt = document.createElement("span"); tt.className = "d95-tb-text"; tt.id = "d95-msgbox-title"; tt.textContent = title;
    tb.appendChild(tt);
    var body = document.createElement("div"); body.className = "d95-msgbox-body";
    var icon = document.createElement("i"); icon.className = "d95-msgbox-icon"; icon.setAttribute("aria-hidden", "true");
    var p = document.createElement("p"); p.id = "d95-msgbox-text"; p.textContent = text;
    body.appendChild(icon); body.appendChild(p);
    var ok = document.createElement("button"); ok.type = "button"; ok.textContent = T("skin.apps.ok");
    var row = document.createElement("div"); row.className = "d95-msgbox-row"; row.appendChild(ok);
    d.appendChild(tb); d.appendChild(body); d.appendChild(row);
    document.body.appendChild(d);
    function done() { try { d.close(); } catch (e) {} if (d.parentNode) d.parentNode.removeChild(d); }
    ok.addEventListener("click", done);
    d.addEventListener("cancel", function (ev) { ev.preventDefault(); done(); });
    d.showModal();
    ok.focus();
  }

  // ---- a window ----
  function make(key, kind) {
    var id = "app-" + (++seq), g = kind === "game" ? games()[pathOf(key)] : null;
    var app = { id: id, key: key, kind: kind, frame: null, el: null };
    var el = document.createElement("section");
    el.className = "d95-app d95-app-" + kind;
    var title = g ? g.title : pathOf(key).split("/").filter(Boolean).pop() || key;
    // a game's menu bar: its own windows (Mods, Game files, Settings, Controls, Chat), full screen, Close
    if (kind === "game") {
      var bar = document.createElement("div"); bar.className = "d95-menubar"; bar.setAttribute("role", "menubar");
      var gm = document.createElement("button"); gm.type = "button"; gm.className = "d95-menubar-item"; gm.setAttribute("role", "menuitem");
      gm.setAttribute("aria-haspopup", "menu"); gm.textContent = T("skin.apps.game_menu");
      gm.addEventListener("click", function () { var r = gm.getBoundingClientRect(); desk.menu(gm, gameMenu(app), r.left, r.bottom, title); });
      bar.appendChild(gm);
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
      maximize: kind === "game" ? function () { try { app.frame.requestFullscreen(); } catch (e) {} } : null,
      at: function (left) { return [left + 40 + (n % 6) * 26, 20 + (n % 6) * 26]; },
      scroll: kind === "txt" ? app.text : null,
      focusIn: function (back) { focusApp(app, back); } });
    apps.push(app);
    return app;
  }
  function inner(app) { try { return app.frame && app.frame.contentWindow; } catch (e) { return null; } }
  function gameMenu(app) {
    var w = inner(app), list = [];
    try {
      w.ByopopSkin.desk.list().forEach(function (x) {
        if (x.id === "game" || /^set-/.test(x.id) || x.id === "about") return;
        list.push({ label: x.title, fn: function () { w.ByopopSkin.desk.openWin(x.id); } });
      });
    } catch (e) {}
    if (list.length) list.push({ rule: true });
    // the game's terms, copyright and licenses (its page's footer links to them, and a window shows no footer)
    var terms = null;
    try { terms = w.document.querySelector("a[href$='-copyright.html']"); } catch (e) {}
    if (terms) list.push({ label: T("skin.apps.terms"), fn: function () { open(terms.href, "push"); } });
    list.push({ label: T("skin.pop.full"), fn: function () { try { app.frame.requestFullscreen(); } catch (e) {} } });
    list.push({ rule: true });
    list.push({ label: T("skin.apps.close"), key: "Alt+F4", fn: function () { closeApp(app); } });
    return list;
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
  // the size a game wants (its screen at one pixel for each of the device's, and its window's frame and status bar): the window
  // takes it the first time, as far as the desktop has room
  function fit(win, w, h) {
    var a = byWin(win);
    if (!a || narrow() || (a.fitted && a.fitted[0] === w && a.fitted[1] === h)) return;
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
  // leaving Desktop 95 on another page's address: that page, as the other skins have it
  window.addEventListener("byopop-skin", function (ev) {
    if (!ev.detail || ev.detail.skin === "d95") return;
    var key = relOf(location.href), kind = key && kindOf(key);
    if (kind && kind !== "index") { location.reload(); return; }
    // a licence text (?open=, addressOf) is a tab of its own in the other skins: the listing, where its link is
    if (Q.get("open") !== null || /[?&]open=/.test(location.search)) { try { history.replaceState(null, "", addressOf(null)); } catch (e) {} }
    // staying on the listing: the windows' pages close as their Close closes them (a game's saves first)
    apps.slice().forEach(closeApp);
  });
  // the first state, and the page this address named (?open=, from web/skins.js onePage)
  function start() {
    var want = Q.get("open");
    // the desktop is not here after all (another skin chosen meanwhile): the page itself
    if (!on()) { if (want !== null && relOf(new URL(want, SITE).href) !== null) location.replace(new URL(want, SITE).href); return; }
    if (want !== null) {
      try { history.replaceState(state(), "", addressOf(null)); } catch (e) {}
      if (!open(SITE + want, "replace")) location.replace(new URL(want, SITE).href);
    } else write(false);
  }
  if (document.readyState === "complete") setTimeout(start, 0); else window.addEventListener("load", function () { setTimeout(start, 0); });

  return { open: open, activated: activated, f6: f6, closed: closed, title: title, fit: fit, count: function () { return apps.length; } };
})();
