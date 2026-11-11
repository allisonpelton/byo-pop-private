// The site's look (docs/WEB_PLAYER.md, "Desktop 95"): Desktop 95, an affectionate parody of the 1995 desktop (web/skins.css).
// It was one of three skins until 2026-11-09, when the user kept it alone ("drop the alt skins, including the plain one"); a
// stored choice of another skin is dropped when the settings are read (web/settings.js), and the light/dark theme went with them.
//
// This file is loaded in each page's <head>, so the page is drawn in its look from the start: it sets <html data-skin="d95">
// (nothing on the player-only page, ?player=1, which keeps the page's own plain styles for the sessions' tests).
var ByopopSkin = (function () {
  "use strict";
  var KEY = "byopop-settings";
  var root = document.documentElement;
  var pageGame = null;  // a game page's id (setGame): its desktop's windows are kept by it
  // Desktop 95 in one page (docs/WEB_PLAYER.md, "Desktop 95 in one page"): a page of ours inside one of the desktop's windows
  // (web/deskapps.js) is loaded with ?embed=1. It draws no desktop of its own (no task bar, no icons), hands its links to the
  // desktop, and tells it when it is clicked or focused. SITE is the site's folder, where the listing is.
  var Q = new URLSearchParams(location.search);
  var EMBED = Q.get("embed") === "1" && window.parent !== window;
  if (EMBED) root.classList.add("d95-embed");
  var SITE = (function () { try { return new URL(".", document.currentScript.src).href; } catch (e) { return null; } })();
  function parentApps() { try { return EMBED && window.parent.ByopopApps ? window.parent.ByopopApps : null; } catch (e) { return null; } }

  function readItem() {
    try {
      var o = JSON.parse(window.localStorage.getItem(KEY) || "{}");
      return o && typeof o === "object" && !Array.isArray(o) ? o : {};
    } catch (e) { return {}; }
  }
  // the look this page shows: Desktop 95, or none on the player-only page
  function current() { return root.classList.contains("player-only") ? "plain" : "d95"; }
  function apply() {
    var s = current();
    if (s === "plain") root.removeAttribute("data-skin"); else root.setAttribute("data-skin", s);
    try { window.dispatchEvent(new CustomEvent("byopop-skin", { detail: { skin: s } })); } catch (e) {}
  }
  // a game page names itself (web/shell.html), after it marked the player-only page
  function setGame(id) { pageGame = id || null; apply(); }
  // the light/dark theme's choice, kept until 2026-11-09 (web/theme.js): dropped quietly
  try { window.localStorage.removeItem("byopop-theme"); } catch (e) {}

  // The site is one page (the user, 2026-11-09: "What we're developing now is a SPA, and the url bar should never need to show
  // any path other than root"): any other page of ours opened in its own tab (an old address, a bookmark, a link someone
  // shared) goes to the site's root with ?open= naming it, where the desktop opens its window and puts the address back to the
  // root (web/deskapps.js). Not a page in one of the desktop's windows (?embed=1), and not a game's page opened with a test
  // parameter (?silent=1, ?player=1 or ?assets=): the sessions' drivers use that page as it is. The test parameters a page was
  // opened with stay in the address.
  function testQuery() {
    var q = new URLSearchParams();
    if (Q.get("silent") === "1") q.set("silent", "1");
    if (Q.get("assets") !== null) q.set("assets", Q.get("assets"));
    return q.toString();
  }
  function onePage() {
    if (EMBED || !SITE || window.top !== window) return false;
    var here = location.href.split("#")[0];
    if (here.indexOf(SITE) !== 0) return false;
    var rel = here.slice(SITE.length), path = rel.split("?")[0];
    if (path === "" || path === "index.html") return false;
    var game = root.classList.contains("game-unmade");
    if (game && (Q.get("silent") === "1" || Q.get("player") === "1" || Q.get("assets") !== null)) return false;
    // the page's own address without the test parameters, which go on the root's
    var u = new URL(location.href);
    ["silent", "assets", "player", "embed"].forEach(function (k) { u.searchParams.delete(k); });
    var t = testQuery();
    root.classList.add("d95-leaving");
    location.replace(SITE + "?open=" + encodeURIComponent(u.href.slice(SITE.length)) + (t ? "&" + t : ""));
    return true;
  }
  onePage();

  function T(k) { try { return ByopopStrings.t(k); } catch (e) { return k; } }
  function Tv(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }

  // ---------- the setup wizard ----------
  // A game page's three cards before play (the terms, the import, Ready: web/shell.html) become the pages of a setup wizard in
  // Desktop 95: a title bar ("byo-pop Setup: <game>"), the steps Licence, Your folder, Copying and Finish with the current one
  // marked, and an art column with the game's cap (the import card's own cap, which the opener still takes off). The cards keep
  // their own contents and buttons: the terms card is the licence page with every rule it has (docs/WEB_PLAYER.md, "The
  // terms"). The parts are drawn only in Desktop 95 (web/skins.css), not on the player-only page; they are hidden from
  // assistive technology, which reads the cards as they are.
  //   o: { game: the game's title, cards: [[element, step], ...] (step 0 to 3), bar: the import's progressbar, cap: {colour, text} }
  var STEPS = ["skin.wiz.licence", "skin.wiz.folder", "skin.wiz.copying", "skin.wiz.finish"];
  function wizard(o) {
    function el(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text) e.textContent = text; return e; }
    var heads = [];
    o.cards.forEach(function (c) {
      var card = c[0], step = c[1];
      if (!card || card.querySelector(".wiz-head")) return;
      card.classList.add("wiz-card");
      card.setAttribute("data-wiz-step", String(step));
      var head = el("div", "wiz-head");
      head.setAttribute("aria-hidden", "true");
      head.appendChild(el("span", "wiz-title", ByopopStrings.t("skin.wiz.title", { game: o.game })));
      var ol = el("ol", "wiz-steps");
      STEPS.forEach(function (k) { ol.appendChild(el("li", "", T(k))); });
      head.appendChild(ol);
      card.insertBefore(head, card.firstChild);
      var art = el("div", "wiz-art");
      art.setAttribute("aria-hidden", "true");
      if (!card.querySelector(".page-cap")) {
        var cap = el("span", "wiz-cap", o.cap.text);
        cap.style.setProperty("--tile", o.cap.colour);
        art.appendChild(cap);
      }
      card.insertBefore(art, head.nextSibling);
      heads.push(card);
    });
    // the import card is "Your folder" until the copy starts, then "Copying"
    if (o.bar && window.MutationObserver) {
      var follow = function () {
        var v = Number(o.bar.getAttribute("aria-valuenow")) || 0;
        heads.forEach(function (card) {
          if (card.getAttribute("data-wiz-step") === "1" || card.getAttribute("data-wiz-step") === "2")
            card.setAttribute("data-wiz-step", v > 0 ? "2" : "1");
        });
      };
      new MutationObserver(follow).observe(o.bar, { attributes: true, attributeFilter: ["aria-valuenow"] });
      follow();
    }
  }

  // ---------- Desktop 95's desktop ----------
  // On a game page in Desktop 95 the page is one desktop (html.d95-desk): the game, its controls (the page's bar), the chat and
  // the panels (Settings, Game files, Mods and the game's own) are windows that are dragged by their title bars, and a column of
  // desktop icons at the left opens each one again. Each page is its own desktop: the game pages stay separate pages, with their
  // own addresses. Where each window is, and which are closed, is kept for each game (the item "byopop-desk", by the page's id);
  // a panel's open or closed state stays the page's own. The game window always shows one canvas pixel for each of the game's
  // pixels, or an exact multiple (web/shell.html, sizeStage), and its picture starts on a whole device pixel. On a screen too
  // narrow for windows (under DESK_MIN pixels) the page is laid out as usual. More windows (the performance
  // panel, the controls panel) are added with desk.add.
  // The desktop itself never scrolls (the user, 2026-11-08: "The whole window shouldn't be scrollable ... any scrolling happening
  // only within each window"): every window is at most as large as the browser's window above the task bar, and scrolls its own
  // contents; when it opens, and when the browser's window changes size, it is moved to fit wholly on the screen where it can.
  // While it is dragged it may go part way off the screen, as on the 1995 desktop.
  var DESK_KEY = "byopop-desk", DESK_MIN = 760;
  function TW(k, title) { try { return ByopopStrings.t(k, { window: title }); } catch (e) { return title; } }
  var desk = (function () {
    var wins = [], top = 20, on = false, icons = null, taskbar = null;
    function store() {
      try { var o = JSON.parse(window.localStorage.getItem(DESK_KEY) || "{}"); return o && typeof o === "object" && !Array.isArray(o) ? o : {}; }
      catch (e) { return {}; }
    }
    // a game page's layout is kept under its id, the listing's under "index", a page in one of the one page's windows apart
    // ("embed:" and its id: there its game's window fills the frame)
    function pageKey() { return (EMBED ? "embed:" : "") + (pageGame || "index"); }
    function saved() { var o = store(), k = pageKey(); return (o[k] && typeof o[k] === "object") ? o[k] : {}; }
    function save() {
      var o = store(), mine = {}, was = saved();
      // a program's window (a game on the one page, kept under its page's name, o.keep) keeps its place while it is not open too,
      // so the game opens where it was left (the user, 2026-11-10: "The positions of the game windows weren't remembered")
      Object.keys(was).forEach(function (k) { if (k.indexOf("app:") === 0) mine[k] = was[k]; });
      wins.forEach(function (w) {
        // every window's open or closed state (a reload opens the listing's windows again as they were), its place once it has one
        // (a program's window not yet moved keeps no place: it opens where its turn puts it)
        if (!w.placed && !w.min) { if (w.keep === w.id) mine[w.keep] = { closed: !!w.closed }; return; }
        mine[w.keep] = { x: w.x, y: w.y, closed: !!w.closed, min: !!w.min, placed: !!w.placed };
        if (w.w && w.h) { mine[w.keep].w = w.w; mine[w.keep].h = w.h; }
      });
      o[pageKey()] = mine;
      try { window.localStorage.setItem(DESK_KEY, JSON.stringify(o)); } catch (e) {}
    }
    function dpr() { return window.devicePixelRatio || 1; }
    // a lifted window (a tab of the listing) is drawn on the desktop whatever its hidden attribute says (the tabs' own state)
    function hid(w) { return w.el.hidden && !w.home; }
    function shown(w) { return !w.closed && !hid(w) && w.el.getClientRects().length > 0; }
    // the room for windows: the browser's window less the task bar along the bottom
    function room() {
      var b = taskbar ? taskbar.el.offsetHeight : 0;
      return { w: document.documentElement.clientWidth || window.innerWidth, h: (document.documentElement.clientHeight || window.innerHeight) - b };
    }
    // keeps a title bar on the screen, and the game's picture on whole device pixels; fit (on opening, on a resize): the whole
    // window on the screen, where it is small enough
    function place(w, fit) {
      var r = room();
      w.el.style.maxWidth = Math.max(200, r.w - 8) + "px";
      w.el.style.maxHeight = Math.max(120, r.h - 8) + "px";
      if (fit && shown(w)) {
        w.x = Math.max(4, Math.min(w.x, r.w - w.el.offsetWidth - 4));
        w.y = Math.max(4, Math.min(w.y, r.h - w.el.offsetHeight - 4));
      }
      w.x = Math.max(-(w.el.offsetWidth - 80), Math.min(r.w - 80, Math.round(w.x)));
      w.y = Math.max(0, Math.min(r.h - 24, Math.round(w.y)));
      w.el.style.left = w.x + "px";
      w.el.style.top = w.y + "px";
      if (w.snap && shown(w)) {
        var r = w.snap.getBoundingClientRect(), d = dpr();
        var dx = (Math.round(r.left * d) - r.left * d) / d, dy = (Math.round(r.top * d) - r.top * d) / d;
        if (dx || dy) { w.el.style.left = (w.x + dx) + "px"; w.el.style.top = (w.y + dy) + "px"; }
      }
    }
    function raise(w) { w.el.style.zIndex = String(++top); tasks(); }
    function setClosed(w, c) {
      w.closed = !!c;
      if (w.closed) w.min = false;
      w.el.classList.toggle("d95-closed", w.closed);
      w.el.classList.toggle("d95-min", !!w.min);
      if (w.icon) w.icon.setAttribute("aria-pressed", w.closed ? "false" : "true");
      if (!w.closed) { raise(w); place(w, true); }
      save();
      tasks();
    }
    // a window's Close: its own (o.onClose: the game's window closes the program), else it goes off the desktop to its icon
    // the What's This? mode (the title bar's "?"): one at a time, ended by the next click anywhere, by Esc or by the "?" again
    var whatsOn = null;
    function whatsThis(button) {
      var was = whatsOn;
      endWhats();
      if (was === button) return;
      whatsOn = button;
      button.setAttribute("aria-pressed", "true");
      document.documentElement.classList.add("d95-whats");
      document.addEventListener("pointerdown", whatsDown, true);
      document.addEventListener("keydown", whatsKey, true);
    }
    function endWhats() {
      if (!whatsOn) return;
      whatsOn.setAttribute("aria-pressed", "false");
      whatsOn = null;
      document.documentElement.classList.remove("d95-whats");
      document.removeEventListener("pointerdown", whatsDown, true);
      document.removeEventListener("keydown", whatsKey, true);
    }
    // the click that asks: nothing under it works (not its press, nor the click after it); a click on the "?" itself is the
    // button's own, which ends the mode
    function swallow(ev) { ev.preventDefault(); ev.stopPropagation(); }
    function whatsDown(ev) {
      if (whatsOn && whatsOn.contains(ev.target)) return;
      swallow(ev);
      var target = ev.target, x = ev.clientX, y = ev.clientY;
      endWhats();
      ["mousedown", "mouseup", "click", "focus"].forEach(function (n) {
        var once = function (e) { swallow(e); document.removeEventListener(n, once, true); };
        document.addEventListener(n, once, true);
        setTimeout(function () { document.removeEventListener(n, once, true); }, 600);
      });
      try { target.dispatchEvent(new CustomEvent("byopop-whatsthis", { bubbles: true, detail: { x: x, y: y } })); } catch (e) {}
    }
    function whatsKey(ev) { if (ev.key === "Escape") { swallow(ev); endWhats(); } }
    // o.closed: told once the window went to its icon (Music stops its sound then)
    function closeWin(w) {
      closeAllMenus();
      if (w.onClose) { w.onClose(); return; }
      setClosed(w, true);
      if (w.afterClose) try { w.afterClose(); } catch (e) {}
    }
    // a window's control menu, from its title bar's icon
    function controlMenu(w, from, x, y) {
      var entries = [{ label: T("skin.desk.m.minimize"), fn: function () { setMin(w, true); } }];
      if (w.maximize) entries.push({ label: T(maxed(w) ? "skin.desk.m.restore" : "skin.desk.m.maximize"), fn: function () { w.maximize(); } });
      entries.push({ rule: true });
      // the game's window: its whole-multiple sizes that fit (o.sizes), the one it has ticked
      if (w.sizes) {
        var sz = w.sizes();
        if (sz && sz.list.length) {
          sz.list.forEach(function (s) { entries.push({ label: s.label, checked: s.n === sz.current, fn: function () { sz.set(s.n); } }); });
          entries.push({ rule: true });
        }
      }
      // no key beside it: Alt+F4 closes the browser's window, never the page's (the user, 2026-11-09: "Alt-F4 captured by browser")
      entries.push({ label: T("skin.desk.m.close"), bold: true, fn: function () { closeWin(w); } });
      popMenu(from, entries, x, y, w.el.getAttribute("data-d95-title"));
    }
    // A maximized window (html .d95-maxed, web/deskapps.js): its maximize button is Restore, with 1995's two overlapping
    // windows, and so is its control menu's entry
    function maxed(w) { return w.el.classList.contains("d95-maxed"); }
    function showMaxed(w) {
      var mx = w.el.querySelector(".d95-tb > .d95-mx"), m = maxed(w);
      if (!mx) return;
      mx.classList.toggle("d95-rs", m);
      mx.setAttribute("aria-label", TW(m ? "skin.desk.restore" : "skin.desk.maximize", w.el.getAttribute("data-d95-title") || ""));
    }
    // a window's title (the game's: its name and its mods), on its title bar and task bar button and in its buttons' names
    function setTitle(w, title) {
      if (w.el.getAttribute("data-d95-title") === title) return;
      w.el.setAttribute("data-d95-title", title);
      var h = w.el.querySelector(".d95-tb > span.d95-tb-text");
      if (h) h.textContent = title;
      [["d95-mn", "skin.desk.minimize"], ["d95-mx", maxed(w) ? "skin.desk.restore" : "skin.desk.maximize"], ["d95-x", "skin.desk.close"], ["d95-sys", "skin.desk.menu"]].forEach(function (b) {
        var e = w.el.querySelector(".d95-tb > ." + b[0]);
        if (e) e.setAttribute("aria-label", TW(b[1], title));
      });
      tasks();
    }
    // minimized: out of sight, its button on the task bar, which brings it back where it was
    function setMin(w, m) {
      if (m) closeAllMenus();
      w.min = !!m;
      w.el.classList.toggle("d95-min", w.min);
      if (!w.min) { raise(w); place(w, true); }
      save();
      tasks();
    }
    // ---- a window's size (the user, 2026-11-08: "Non-game windows should be resizable") ----
    // Every window but the game's (whose sizes are whole multiples, o.sizes) is resized by its edges and corners as in 1995: a
    // band along each edge and a square at each corner, inside its frame, that a press drags. The size goes from the smallest
    // that keeps its controls usable (RZ_MIN) to the room above the task bar; its contents scroll or reflow in it. The size is
    // kept with the window's place (byopop-desk), and goes off with the desktop.
    var RZ_EDGES = ["n", "s", "e", "w", "ne", "nw", "se", "sw"], RZ_MIN = { w: 200, h: 120 };
    function sizeTo(w, width, height) {
      w.w = Math.round(width); w.h = Math.round(height);
      w.el.style.width = w.w + "px"; w.el.style.height = w.h + "px";
    }
    function resizable(w) {
      RZ_EDGES.forEach(function (edge) {
        var h = document.createElement("span");
        h.className = "d95-rz d95-rz-" + edge; h.setAttribute("aria-hidden", "true");
        w.el.appendChild(h);
        var st = null;
        h.addEventListener("pointerdown", function (ev) {
          if (!on || ev.button !== 0) return;
          ev.preventDefault(); ev.stopPropagation();
          raise(w);
          st = { px: ev.clientX, py: ev.clientY, x: w.x, y: w.y, w: w.el.offsetWidth, h: w.el.offsetHeight, id: ev.pointerId };
          try { h.setPointerCapture(ev.pointerId); } catch (err) {}
          document.documentElement.classList.add("d95-dragging");
        });
        h.addEventListener("pointermove", function (ev) {
          if (!st || ev.pointerId !== st.id) return;
          var dx = ev.clientX - st.px, dy = ev.clientY - st.py, r = room(), nw = st.w, nh = st.h;
          if (edge.indexOf("e") >= 0) nw = st.w + dx;
          if (edge.indexOf("w") >= 0) nw = st.w - dx;
          if (edge.indexOf("s") >= 0) nh = st.h + dy;
          if (edge.indexOf("n") >= 0) nh = st.h - dy;
          // a window may ask for more than RZ_MIN (o.minHeight: Music keeps its player and three of its list's rows; o.minWidth
          // and o.minHeight: a game's window is never smaller than its game, web/deskapps.js)
          var leastW = RZ_MIN.w, least = RZ_MIN.h;
          if (w.minWidth) try { leastW = Math.max(leastW, w.minWidth() || 0); } catch (err) {}
          if (w.minHeight) try { least = Math.max(least, w.minHeight() || 0); } catch (err) {}
          nw = Math.max(Math.min(leastW, r.w - 8), Math.min(r.w - 8, nw));
          nh = Math.max(Math.min(least, r.h - 8), Math.min(r.h - 8, nh));
          // and the edge dragged stays on the desktop: the bottom above the task bar, the right inside the screen, the top and the
          // left at 0 at most
          if (edge.indexOf("s") >= 0) nh = Math.max(Math.min(least, r.h - st.y), Math.min(nh, r.h - st.y));
          if (edge.indexOf("e") >= 0) nw = Math.max(Math.min(leastW, r.w - st.x), Math.min(nw, r.w - st.x));
          if (edge.indexOf("n") >= 0) nh = Math.min(nh, st.y + st.h);
          if (edge.indexOf("w") >= 0) nw = Math.min(nw, st.x + st.w);
          // the left and top edges move the window as it grows the other way
          w.x = edge.indexOf("w") >= 0 ? st.x + st.w - nw : st.x;
          w.y = edge.indexOf("n") >= 0 ? st.y + st.h - nh : st.y;
          sizeTo(w, nw, nh);
          place(w);
        });
        var end = function (ev) {
          if (!st || ev.pointerId !== st.id) return;
          st = null;
          document.documentElement.classList.remove("d95-dragging");
          w.placed = true;
          save();
        };
        h.addEventListener("pointerup", end);
        h.addEventListener("pointercancel", end);
      });
    }
    function drag(w, handle) {
      var start = null, moved = false;
      handle.addEventListener("pointerdown", function (ev) {
        if (!on || ev.button !== 0) return;
        raise(w);
        // a press on a control in the title bar (a close button) is that control's own
        if (ev.target.closest("input, select, textarea, a") || (ev.target.closest("button") && ev.target.closest("button") !== handle)) return;
        start = { px: ev.clientX, py: ev.clientY, x: w.x, y: w.y, id: ev.pointerId };
        moved = false;
      });
      window.addEventListener("pointermove", function (ev) {
        if (!start || ev.pointerId !== start.id) return;
        var dx = ev.clientX - start.px, dy = ev.clientY - start.py;
        if (!moved && Math.abs(dx) + Math.abs(dy) < 4) return;
        if (!moved) { moved = true; try { handle.setPointerCapture(ev.pointerId); } catch (e) {} document.documentElement.classList.add("d95-dragging"); }
        w.x = start.x + dx; w.y = start.y + dy;
        place(w);
      });
      function end(ev) {
        if (!start || ev.pointerId !== start.id) return;
        start = null;
        document.documentElement.classList.remove("d95-dragging");
        if (moved) { w.placed = true; save(); }
      }
      window.addEventListener("pointerup", end);
      window.addEventListener("pointercancel", end);
      // a drag that began on a button (the chat's heading) is not also its click
      handle.addEventListener("click", function (ev) { if (moved) { moved = false; ev.stopPropagation(); ev.preventDefault(); } }, true);
      // the keyboard: Alt and the arrows move a window whose title bar has the focus, 8 pixels a press
      handle.addEventListener("keydown", function (ev) {
        if (!on || !ev.altKey) return;
        var k = { ArrowLeft: [-8, 0], ArrowRight: [8, 0], ArrowUp: [0, -8], ArrowDown: [0, 8] }[ev.key];
        if (!k) return;
        ev.preventDefault();
        w.x += k[0]; w.y += k[1]; w.placed = true;
        place(w); save();
      });
    }
    // el: the window; o: { id, title, handle (its title bar; else one is added), snap (the element on whole device pixels),
    // closable (a close button that hides it; the desktop icon shows it again), closed (closed until the player opens it), open
    // (what the icon does instead: a panel's own button), at: function () { return [x, y]: where it goes first }, body (its
    // contents in a box of their own under the title bar, which scrolls them: a window that is not already laid out that way),
    // lift (on the desktop the window is a child of the page's body, and goes back where it was when the desktop goes: a part of
    // another window, such as a tab of the listing), pic (an element copied into its desktop icon's picture), minimize (on the
    // desktop a click on this part of its own title bar, or the keyboard's press of the title bar, minimizes it, and every other
    // click on the title bar does nothing: the chat's collapser) }
    function add(el, o) {
      if (!el || wins.some(function (w) { return w.el === el; })) return null;
      var w = { focusIn: o.focusIn || null, el: el, id: o.id, keep: o.keep || o.id, snap: o.snap || null, x: 0, y: 0, placed: false, closed: !!(o.closed && o.closable), at: o.at, open: o.open,
                button: o.button, pic: o.pic || null, home: null, onClose: o.onClose || null, afterClose: o.afterClose || null, maximize: o.maximize || null, noIcon: !!o.noIcon, sizes: o.sizes || null,
                minHeight: o.minHeight || null, minWidth: o.minWidth || null };
      if (o.lift) { w.home = document.createComment("desktop window " + o.id); el.parentNode.insertBefore(w.home, el); }
      el.classList.add("d95-win");
      el.setAttribute("data-d95-title", o.title);
      el.setAttribute("data-d95-id", o.id);
      var handle = o.handle;
      if (!handle) {
        handle = document.createElement("div");
        handle.className = "d95-tb";
        // a window's small icon at the title's left (o.icon, SVG markup: a Settings section's)
        if (o.icon) { var ti = document.createElement("i"); ti.className = "d95-tb-icon"; ti.setAttribute("aria-hidden", "true"); ti.innerHTML = o.icon; handle.appendChild(ti); }
        var t = document.createElement("span");
        t.className = "d95-tb-text";
        t.textContent = o.title;
        handle.appendChild(t);
        // What's This? (o.whatsThis: a Settings section's window): Windows 95's "?" beside the window's other buttons. A press
        // turns the pointer into an arrow with a question mark; the next click on a control asks the page for its tip (the event
        // "byopop-whatsthis" on what was clicked, with where) instead of working the control, and ends the mode, as Esc does
        if (o.whatsThis) {
          var hlp = document.createElement("button");
          hlp.type = "button"; hlp.className = "d95-hlp"; hlp.setAttribute("aria-label", TW("skin.desk.whatsthis", o.title));
          hlp.setAttribute("aria-pressed", "false");
          hlp.addEventListener("click", function () { whatsThis(hlp); });
          handle.appendChild(hlp);
        }
        if (o.closable) {
          var mn = document.createElement("button");
          mn.type = "button"; mn.className = "d95-mn"; mn.setAttribute("aria-label", TW("skin.desk.minimize", o.title));
          mn.addEventListener("click", function () { setMin(w, true); });
          handle.appendChild(mn);
          // maximize (o.maximize: the game's window, whose maximize is the page's full screen)
          if (o.maximize) {
            var mx = document.createElement("button");
            mx.type = "button"; mx.className = "d95-mx"; mx.setAttribute("aria-label", TW("skin.desk.maximize", o.title));
            mx.addEventListener("click", function () { o.maximize(); });
            handle.appendChild(mx);
            if (window.MutationObserver) new MutationObserver(function () { showMaxed(w); }).observe(el, { attributes: true, attributeFilter: ["class"] });
          }
          var x = document.createElement("button");
          x.type = "button"; x.className = "d95-x"; x.setAttribute("aria-label", TW("skin.desk.close", o.title));
          x.addEventListener("click", function () { closeWin(w); });
          handle.appendChild(x);
        }
        el.insertBefore(handle, el.firstChild);
      }
      if (o.body) {
        var box = document.createElement("div");
        box.className = "d95-body";
        while (handle.nextSibling) box.appendChild(handle.nextSibling);
        el.appendChild(box);
        el.classList.add("d95-has-body");
        scrollbar(box);
      }
      // a part that scrolls on its own (a panel's body, the chat's messages) gets the drawn scroll bar too
      // (one made later, as the chat's, once it is there)
      if (o.scroll) {
        var findArea = function () { var a = typeof o.scroll === "string" ? el.querySelector(o.scroll) : o.scroll; if (a) scrollbar(a); return !!a; };
        if (!findArea() && window.MutationObserver) { var mo = new MutationObserver(function () { if (findArea()) mo.disconnect(); }); mo.observe(el, { childList: true, subtree: true }); }
      }
      handle.classList.add("d95-handle");
      // the control menu (o.menu): the title bar's icon opens it: Minimize, Maximize, Close. No key opens it: Alt+Space is the
      // browser window's own menu on Windows, and Alt+F4 closes the browser's window (the user, 2026-11-09)
      if (o.menu && handle.classList.contains("d95-tb")) {
        var sys = document.createElement("button");
        sys.type = "button"; sys.className = "d95-sys";
        sys.setAttribute("aria-label", TW("skin.desk.menu", o.title)); sys.setAttribute("aria-haspopup", "menu");
        if (o.menuIcon) sys.appendChild(o.menuIcon);
        handle.insertBefore(sys, handle.firstChild);
        sys.addEventListener("click", function () { var r = sys.getBoundingClientRect(); controlMenu(w, sys, r.left, r.bottom); });
        w.sys = sys;
      }
      if (o.minimize) {
        handle.addEventListener("click", function (ev) {
          if (!on || w.passClick) return;
          ev.stopImmediatePropagation(); ev.preventDefault();
          if (ev.detail === 0 || (ev.target.closest && ev.target.closest(o.minimize))) setMin(w, true);
        }, true);
        w.unfold = function () {
          // a window its own part had folded away (the chat collapsed) is open on the desktop: its own click opens it
          if (w.el.classList.contains("chat-collapsed")) { w.passClick = true; try { handle.click(); } catch (e) {} w.passClick = false; }
        };
      }
      drag(w, handle);
      if (!o.sizes) resizable(w);
      // the game's window: a resize corner at its bottom right, which snaps to its whole-multiple sizes (o.sizes) as it is dragged
      if (o.sizes) {
        var grip = document.createElement("span");
        grip.className = "d95-grip"; grip.setAttribute("aria-hidden", "true");
        el.appendChild(grip);
        var gs = null;
        grip.addEventListener("pointerdown", function (ev) {
          if (!on || ev.button !== 0) return;
          var sz = o.sizes();
          if (!sz || !sz.list.length) return;
          ev.preventDefault(); ev.stopPropagation();
          var cur = sz.list.filter(function (s) { return s.n === sz.current; })[0] || sz.list[sz.list.length - 1];
          gs = { px: ev.clientX, py: ev.clientY, w: cur.w, h: cur.h, id: ev.pointerId, n: sz.current };
          try { grip.setPointerCapture(ev.pointerId); } catch (e) {}
        });
        grip.addEventListener("pointermove", function (ev) {
          if (!gs || ev.pointerId !== gs.id) return;
          var sz = o.sizes();
          if (!sz || !sz.list.length) return;
          // the size whose width (or height, whichever the drag moved more) is nearest the corner's
          var dx = ev.clientX - gs.px, dy = ev.clientY - gs.py;
          var byW = Math.abs(dx) >= Math.abs(dy);
          var want = byW ? gs.w + dx : gs.h + dy, best = null, bd = Infinity;
          sz.list.forEach(function (s) { var d = Math.abs((byW ? s.w : s.h) - want); if (d < bd) { bd = d; best = s; } });
          if (best && best.n !== gs.n) { gs.n = best.n; sz.set(best.n); }
        });
        var end = function (ev) { if (gs && ev.pointerId === gs.id) gs = null; };
        grip.addEventListener("pointerup", end);
        grip.addEventListener("pointercancel", end);
      }
      el.addEventListener("pointerdown", function () { if (on) raise(w); }, true);
      var s = saved()[w.keep];
      if (s && isFinite(s.x) && isFinite(s.y)) { w.x = s.x; w.y = s.y; w.placed = s.placed !== false; w.closed = !!s.closed && !!o.closable; w.min = !!s.min && !w.closed; }
      else if (s && typeof s.closed === "boolean" && o.closable) w.closed = s.closed;
      if (!o.sizes && s && s.w > 0 && s.h > 0) { w.w = Math.round(s.w); w.h = Math.round(s.h); if (on) sizeTo(w, w.w, w.h); }
      // a window its page opens with the focus in it (a panel's button) is the active one, and one it closes hands that to the
      // window in front
      if (window.MutationObserver) new MutationObserver(function () {
        tasks();
        if (!on) return;
        if (!el.hidden && !w.closed && !w.min) { if (el.contains(document.activeElement)) activate(w); }
        else if (el.hidden && active === w) activate(frontmost());
      }).observe(el, { attributes: true, attributeFilter: ["hidden"] });
      wins.push(w);
      // (a window opened only from another, as a Settings section from the Settings folder, has no icon on the desktop)
      if (!o.noIcon) addIcon(w, o.title);
      if (on) { lift(w, true); layout(); }
      return w;
    }
    // ---- a window's scroll bar (the user, 2026-11-08: "It's not visible whether a window is scrollable") ----
    // The browser's own bar can't be relied on (an overlay bar that hides until hovered draws nothing at all), so a window's
    // scrolling client area gets one drawn here, 16 pixels wide in the 1995 manner: a raised arrow button at each end, the
    // dithered track and a raised thumb as long as the part in view. It is there whenever the contents overflow, and follows
    // every scroll (the wheel, the keys, touch, a script). Dragging the thumb scrolls, an arrow scrolls a line and repeats while
    // held, a press on the track scrolls a page and repeats until the thumb reaches it. It is a picture for a pointer
    // (aria-hidden): the area itself scrolls from the keyboard as before. The area is wrapped in a box (.d95-client) holding it
    // and its bar, which off the desktop is no box at all (display: contents).
    var LINE = 20;
    function scrollbar(area) {
      if (!area || area.parentNode && area.parentNode.classList && area.parentNode.classList.contains("d95-client")) return null;
      var box = document.createElement("div");
      box.className = "d95-client";
      area.parentNode.insertBefore(box, area);
      box.appendChild(area);
      area.classList.add("d95-sb-area");
      var bar = document.createElement("div");
      bar.className = "d95-vbar"; bar.setAttribute("aria-hidden", "true");
      var up = document.createElement("div"), track = document.createElement("div"), thumb = document.createElement("div"), down = document.createElement("div");
      up.className = "d95-sb-up"; down.className = "d95-sb-down"; track.className = "d95-sb-track"; thumb.className = "d95-sb-thumb";
      track.appendChild(thumb);
      bar.appendChild(up); bar.appendChild(track); bar.appendChild(down);
      box.appendChild(bar);
      var geo = { th: 0, room: 0, max: 0 };
      function update() {
        var sh = area.scrollHeight, ch = area.clientHeight, over = sh - ch > 1;
        box.classList.toggle("d95-scrolling", over);
        if (!over) return;
        var tr = track.clientHeight;
        var th = Math.max(8, Math.min(tr, Math.round(tr * ch / sh)));
        geo.th = th; geo.room = Math.max(0, tr - th); geo.max = sh - ch;
        thumb.style.height = th + "px";
        thumb.style.transform = "translateY(" + Math.round(geo.room * Math.min(1, area.scrollTop / geo.max)) + "px)";
      }
      area.addEventListener("scroll", update, { passive: true });
      if (window.ResizeObserver) {
        var ro = new ResizeObserver(update);
        ro.observe(area); ro.observe(track);
        var watch = function () { Array.prototype.forEach.call(area.children, function (c) { ro.observe(c); }); };
        watch();
        if (window.MutationObserver) new MutationObserver(function () { watch(); update(); }).observe(area, { childList: true, subtree: true, attributes: true, attributeFilter: ["hidden", "class"] });
      }
      window.addEventListener("resize", update);
      // a press that repeats while held: once, then after 400 ms every 50 ms, until the pointer lets go (or it says stop)
      function hold(ev, el, step) {
        if (ev.button !== 0) return;
        ev.preventDefault();
        try { el.setPointerCapture(ev.pointerId); } catch (e) {}
        el.classList.add("d95-down");
        var t = null, done = false;
        function stop() { if (done) return; done = true; clearTimeout(t); el.classList.remove("d95-down"); el.removeEventListener("pointerup", stop); el.removeEventListener("pointercancel", stop); el.removeEventListener("lostpointercapture", stop); }
        function again() { if (done) return; if (step() === false) { stop(); return; } t = setTimeout(again, 50); }
        el.addEventListener("pointerup", stop); el.addEventListener("pointercancel", stop); el.addEventListener("lostpointercapture", stop);
        if (step() === false) { stop(); return; }
        t = setTimeout(again, 400);
      }
      up.addEventListener("pointerdown", function (ev) { hold(ev, up, function () { area.scrollTop -= LINE; update(); }); });
      down.addEventListener("pointerdown", function (ev) { hold(ev, down, function () { area.scrollTop += LINE; update(); }); });
      track.addEventListener("pointerdown", function (ev) {
        if (ev.target !== track) return;
        var y = ev.clientY;
        hold(ev, track, function () {
          var r = thumb.getBoundingClientRect(), page = Math.max(LINE, area.clientHeight - LINE);
          if (y < r.top) area.scrollTop -= page; else if (y > r.bottom) area.scrollTop += page; else return false;
          update();
        });
      });
      thumb.addEventListener("pointerdown", function (ev) {
        if (ev.button !== 0) return;
        ev.preventDefault();
        try { thumb.setPointerCapture(ev.pointerId); } catch (e) {}
        var y0 = ev.clientY, s0 = area.scrollTop;
        thumb.classList.add("d95-down");
        function move(e) { if (geo.room > 0) { area.scrollTop = s0 + (e.clientY - y0) * geo.max / geo.room; update(); } }
        function end() { thumb.classList.remove("d95-down"); thumb.removeEventListener("pointermove", move); thumb.removeEventListener("pointerup", end); thumb.removeEventListener("pointercancel", end); }
        thumb.addEventListener("pointermove", move); thumb.addEventListener("pointerup", end); thumb.addEventListener("pointercancel", end);
      });
      box.d95Update = update;
      update();
      return box;
    }
    function addIcon(w, title) {
      if (!icons) return;
      var b = document.createElement("button");
      b.type = "button"; b.className = "d95-icon d95-icon-" + w.id;
      var pic = document.createElement("i"); pic.setAttribute("aria-hidden", "true");
      if (w.pic) { var c = w.pic.cloneNode(true); c.removeAttribute("id"); pic.appendChild(c); b.classList.add("d95-icon-pic"); }
      var name = document.createElement("span"); name.textContent = title;
      b.appendChild(pic); b.appendChild(name);
      b.setAttribute("aria-label", TW("skin.desk.open", title));
      b.addEventListener("click", function (ev) {
        if (w.open) { w.open(ev); raise(w); return; }
        show(w);
      });
      if (!w.open) b.setAttribute("aria-pressed", w.closed ? "false" : "true");
      // a panel's icon is there while its button on the bar is (Mods: once the game's files are here)
      if (w.button && window.MutationObserver) {
        var follow = function () { b.hidden = w.button.hidden; };
        new MutationObserver(follow).observe(w.button, { attributes: true, attributeFilter: ["hidden"] });
        follow();
      }
      w.icon = b;
      // the windows' icons in the order they were added, then About, then the favourites' (favIcons)
      var after = w.id === "about" ? icons.querySelector(".d95-fav") : icons.querySelector(".d95-icon-about, .d95-fav");
      icons.insertBefore(b, after);
    }
    // opens a window (its icon, a link to it) and puts the focus in it
    function show(w) {
      w.min = false;
      setClosed(w, false);
      if (on) activate(w);
      var f = w.el.matches("[tabindex]:not([tabindex='-1'])") ? w.el : w.el.querySelector("canvas, [tabindex]:not([tabindex='-1']), button");
      if (f && w.id !== "game") try { f.focus({ preventScroll: true }); } catch (e) {}
    }
    // ---- the task bar (the user, 2026-11-08: "Add a task bar to the bottom with a start menu, replacing "Start" with "Pop"") ----
    // Along the desktop's bottom edge: the Pop button and its menu at the left, a button for each open window (the front one
    // pressed in; a click brings a window to the front, or minimizes the one in front; a minimized one comes back from it), and
    // the tray at the right with the clock (and on a game page its sound switch). The page's footer (the copyright line and
    // "Full terms") goes into an About window. The menu works from the keyboard: the arrows, Home and End move through it,
    // Enter, Space or the right arrow opens an entry, Escape (or the left arrow, in the games' list) closes it, and the focus
    // goes back to the Pop button; Tab leaves it closed. The Windows key is left alone.
    function present(w) { return !w.closed && !hid(w) && (w.min || w.el.getClientRects().length > 0); }
    function frontmost() {
      var best = null, z = -1;
      wins.forEach(function (w) { if (present(w) && !w.min) { var k = parseInt(w.el.style.zIndex, 10) || 10; if (k > z) { z = k; best = w; } } });
      return best;
    }
    function tasks() { if (taskbar && on) taskbar.tasks(); }
    function makeTaskbar() {
      var bar = document.createElement("div"), buttons = {}, clock, timer = null, menu, sub, pop, list, soundHome = null, footHome = null;
      bar.className = "d95-taskbar";
      bar.setAttribute("role", "group");
      bar.setAttribute("aria-label", T("skin.taskbar"));
      // the Pop button and its menu
      var wrap = document.createElement("div");
      wrap.className = "d95-popwrap";
      pop = document.createElement("button");
      pop.type = "button"; pop.className = "d95-pop"; pop.id = "d95-pop";
      pop.setAttribute("aria-haspopup", "menu"); pop.setAttribute("aria-expanded", "false"); pop.setAttribute("aria-controls", "d95-pop-menu");
      var cap = document.createElement("i"); cap.setAttribute("aria-hidden", "true"); cap.className = "d95-pop-cap";
      var word = document.createElement("span"); word.textContent = T("skin.pop");
      pop.appendChild(cap); pop.appendChild(word);
      menu = document.createElement("div");
      menu.className = "d95-menu"; menu.id = "d95-pop-menu"; menu.hidden = true;
      menu.setAttribute("role", "menu"); menu.setAttribute("aria-labelledby", "d95-pop");
      var band = document.createElement("div"); band.className = "d95-band"; band.setAttribute("aria-hidden", "true");
      var bandText = document.createElement("span"); bandText.textContent = T("skin.pop.band"); band.appendChild(bandText);
      list = document.createElement("div"); list.className = "d95-items";
      menu.appendChild(band); menu.appendChild(list);
      sub = document.createElement("div");
      sub.className = "d95-menu d95-sub"; sub.id = "d95-pop-games"; sub.hidden = true;
      sub.setAttribute("role", "menu");
      wrap.appendChild(pop); wrap.appendChild(menu); wrap.appendChild(sub);
      var strip = document.createElement("div"); strip.className = "d95-tasks";
      var tray = document.createElement("div"); tray.className = "d95-tray";
      clock = document.createElement("span"); clock.className = "d95-clock";
      tray.appendChild(clock);
      bar.appendChild(wrap); bar.appendChild(strip); bar.appendChild(tray);
      document.body.appendChild(bar);

      function tick() {
        var d = new Date();
        try { clock.textContent = d.toLocaleTimeString([], { hour: "numeric", minute: "2-digit" }); } catch (e) { clock.textContent = d.getHours() + ":" + ("0" + d.getMinutes()).slice(-2); }
      }
      function item(label, act, o) {
        var b = document.createElement(o && o.href ? "a" : "button");
        if (o && o.href) b.href = o.href; else b.type = "button";
        b.className = "d95-item" + (o && o.cls ? " " + o.cls : "");
        b.setAttribute("role", "menuitem"); b.tabIndex = -1;
        var t = document.createElement("span"); t.textContent = label; b.appendChild(t);
        if (o && o.sub) { b.setAttribute("aria-haspopup", "menu"); b.setAttribute("aria-expanded", "false"); b.setAttribute("aria-controls", sub.id); b.subFill = o.fill || null; }
        if (act) b.addEventListener("click", function (ev) { act(ev, b); });
        return b;
      }
      function rule() { var r = document.createElement("div"); r.className = "d95-rule"; r.setAttribute("role", "separator"); return r; }
      function closeThen(fn) { return function () { close(false); try { fn(); } catch (e) {} }; }
      // the games whose files are here, each its page
      function playable() {
        var rec = {}, out = [];
        try { rec = ByopopGameFiles.all(); } catch (e) {}
        var here = function (id) { try { return ByopopGameFiles.present(rec[id]); } catch (e) { return false; } };
        if (window.ByopopGames && ByopopGames.length) {
          ByopopGames.forEach(function (g) { if (here(g.id)) out.push({ id: g.id, title: g.title, page: g.page }); });
        } else {
          Array.prototype.forEach.call(document.querySelectorAll("a.tile[data-game]"), function (a) {
            var n = a.querySelector(".name");
            if (here(a.getAttribute("data-game"))) out.push({ id: a.getAttribute("data-game"), title: n ? n.textContent : a.getAttribute("data-game"), page: a.getAttribute("href") });
          });
        }
        // the ports still in progress that web/localsoon.js opened on a development host (not in ByopopGames, the finished games)
        Array.prototype.forEach.call(document.querySelectorAll(".games > li.soon-local > a.tile[data-game]"), function (a) {
          var id = a.getAttribute("data-game"), n = a.querySelector(".name");
          if (here(id) && !out.some(function (g) { return g.id === id; })) out.push({ id: id, title: n ? n.textContent : id, page: a.getAttribute("href") });
        });
        // the favourites first, then the others, each by title
        var favs = [];
        try { favs = ByopopSettings.favourites().map(function (f) { return f.id; }); } catch (e) {}
        out.forEach(function (g) { g.fav = favs.indexOf(g.id) >= 0; });
        return out.sort(function (a, b) { return (b.fav - a.fav) || a.title.localeCompare(b.title); });
      }
      function fillSub() {
        sub.textContent = "";
        // the games' window the setting "Games open in" names (All games or the Library, web/library.js): on the listing, first
        var gw = document.getElementById("games-title") && find(gamesWindow());
        if (gw) {
          sub.appendChild(item(gw.el.getAttribute("data-d95-title") || gw.id, closeThen(function () { if (gw.min) setMin(gw, false); show(gw); })));
          sub.appendChild(rule());
        }
        var games = playable();
        if (!games.length) { var none = item(T("skin.pop.nogames"), null); none.setAttribute("aria-disabled", "true"); sub.appendChild(none); }
        var shown = games.filter(function (g) { return !win95Hides(g.id); });
        shown.forEach(function (g) { sub.appendChild(item(g.title, null, { href: g.page })); });
        if (games.length > shown.length) {
          var n = games.length - shown.length;
          var hid = item(Tv("skin.win95.hidden", { count: n }), null);
          hid.setAttribute("aria-disabled", "true");
          hid.title = T("skin.win95.hidden_title");
          if (!shown.length) sub.textContent = "";
          sub.appendChild(hid);
        }
      }
      function fill() {
        list.textContent = "";
        var listing = !!document.getElementById("games-title");
        var games = item(T("skin.pop.games"), function (ev, b) { openSub(b, true); }, { sub: true, cls: "d95-has-sub" });
        list.appendChild(games);
        list.appendChild(rule());
        function openWin(w) {
          if (w.min) setMin(w, false);
          if (w.open) { w.open(); raise(w); } else show(w);
        }
        function win(id, label) {
          var w = find(id);
          if (!w || (w.button && w.button.hidden)) return;
          list.appendChild(item(label || w.el.getAttribute("data-d95-title") || id, closeThen(function () { openWin(w); })));
        }
        // Settings, with its sections in a list beside it: the folder itself first, then each section's window
        function settings(id) {
          var w = find(id), secs = wins.filter(function (x) { return /^set-/.test(x.id); });
          if (!w) return;
          if (!secs.length) { win(id); return; }
          list.appendChild(item(w.el.getAttribute("data-d95-title") || id, function (ev, b) { openSub(b, true); }, { sub: true, cls: "d95-has-sub", fill: function () {
            sub.textContent = "";
            sub.appendChild(item(T("skin.pop.allsettings"), closeThen(function () { openWin(w); })));
            sub.appendChild(rule());
            secs.forEach(function (s) { sub.appendChild(item(s.el.getAttribute("data-d95-title"), closeThen(function () { openWin(s); }))); });
          } }));
        }
        if (listing) {
          // the Setup window (web/setup.js), for adding games
          win("setup", T("setup.pop"));
          win("library");
          settings("tab-settings"); win("tab-music"); win("sounds"); win("chat");
        } else {
          settings("panel-settings");
          [["music", "index.tabs.music"], ["sounds", "sounds.title"]].forEach(function (m) {
            list.appendChild(item(T(m[1]), null, { href: "./#" + m[0] }));
          });
          list.appendChild(rule());
          // the game's full screen, as its window's maximize button
          var gw = find("game");
          if (gw && gw.maximize) list.appendChild(item(T("skin.pop.full"), closeThen(gw.maximize)));
          if (gw && gw.sizes) {
            var sz = gw.sizes();
            if (sz && sz.list.length) list.appendChild(item(T("skin.pop.size"), function (ev, b) { openSub(b, true); }, { sub: true, cls: "d95-has-sub", fill: function () {
              sub.textContent = "";
              var now = gw.sizes();
              now.list.forEach(function (s) {
                var it = item(s.label, closeThen(function () { now.set(s.n); }));
                it.setAttribute("role", "menuitemradio"); it.setAttribute("aria-checked", s.n === now.current ? "true" : "false"); it.classList.add("d95-radio");
                sub.appendChild(it);
              });
            } }));
          }
          // this game's own windows
          wins.forEach(function (w) { if (w.id !== "panel-settings" && w.id !== "about" && !/^set-/.test(w.id) && !(w.id === "chat" && w.el.hidden)) win(w.id); });
        }
        list.appendChild(rule());
        win("about", T("skin.desk.about"));
        if (!listing) {
          list.appendChild(rule());
          list.appendChild(item(T("skin.pop.listing"), null, { href: "./", cls: "d95-leave" }));
        }
      }
      function items(m) { return Array.prototype.filter.call(m.querySelectorAll(".d95-item"), function (b) { return b.getClientRects().length > 0; }); }
      function focusIn(m, i) { var a = items(m); if (!a.length) return; a[(i + a.length) % a.length].focus(); }
      function openSub(from, focus) {
        var was = list.querySelector(".d95-has-sub[aria-expanded=true]");
        if (was && was !== from) was.setAttribute("aria-expanded", "false");
        if (from.subFill) from.subFill(); else fillSub();
        sub.hidden = false;
        from.setAttribute("aria-expanded", "true");
        var r = from.getBoundingClientRect(), wr = wrap.getBoundingClientRect();
        sub.style.left = (r.right - wr.left - 3) + "px";
        sub.style.bottom = "auto";
        sub.style.top = "0px";
        var h = sub.offsetHeight, topAt = r.top - wr.top - 3;
        // kept above the task bar: its bottom no lower than the item's
        sub.style.top = Math.min(topAt, r.bottom - wr.top - h + 3) + "px";
        if (focus) focusIn(sub, 0);
      }
      function closeSub(focusBack) {
        if (sub.hidden) return;
        sub.hidden = true;
        var g = list.querySelector(".d95-has-sub[aria-expanded=true]");
        if (g) { g.setAttribute("aria-expanded", "false"); if (focusBack) g.focus(); }
      }
      function open() {
        fill();
        menu.hidden = false;
        pop.setAttribute("aria-expanded", "true");
        focusIn(menu, 0);
      }
      function close(focusPop) {
        closeSub(false);
        if (menu.hidden) return;
        menu.hidden = true;
        pop.setAttribute("aria-expanded", "false");
        if (focusPop) pop.focus();
      }
      pop.addEventListener("click", function () { if (menu.hidden) open(); else close(true); });
      pop.addEventListener("keydown", function (ev) {
        if (ev.key === "ArrowUp" || ev.key === "ArrowDown") { ev.preventDefault(); if (menu.hidden) open(); }
      });
      function keys(m, isSub) {
        m.addEventListener("keydown", function (ev) {
          var a = items(m), i = a.indexOf(document.activeElement);
          if (ev.key === "ArrowDown") { ev.preventDefault(); focusIn(m, i + 1); }
          else if (ev.key === "ArrowUp") { ev.preventDefault(); focusIn(m, i < 0 ? -1 : i - 1); }
          else if (ev.key === "Home") { ev.preventDefault(); focusIn(m, 0); }
          else if (ev.key === "End") { ev.preventDefault(); focusIn(m, -1); }
          else if (ev.key === "ArrowRight" && !isSub && document.activeElement && document.activeElement.classList.contains("d95-has-sub")) { ev.preventDefault(); openSub(document.activeElement, true); }
          else if ((ev.key === "ArrowLeft" || ev.key === "Escape") && isSub) { ev.preventDefault(); ev.stopPropagation(); closeSub(true); }
          else if (ev.key === "Escape") { ev.preventDefault(); close(true); }
          else if (ev.key === "Tab") close(false);
          else if (ev.key === " " && document.activeElement && document.activeElement.tagName === "A") { ev.preventDefault(); document.activeElement.click(); }
          else if (ev.key === "ArrowDown" || ev.key === "ArrowUp") ev.stopPropagation();
        });
      }
      keys(menu, false); keys(sub, true);
      // a pointer over an entry is the focus; over the games, their list
      menu.addEventListener("pointerover", function (ev) {
        var b = ev.target.closest && ev.target.closest(".d95-item");
        if (!b) return;
        b.focus();
        if (b.classList.contains("d95-has-sub")) openSub(b, false); else closeSub(false);
      });
      sub.addEventListener("pointerover", function (ev) { var b = ev.target.closest && ev.target.closest(".d95-item"); if (b) b.focus(); });
      document.addEventListener("pointerdown", function (ev) { if (!menu.hidden && !wrap.contains(ev.target)) close(false); }, true);
      sub.addEventListener("click", function (ev) { if (ev.target.closest && ev.target.closest("a")) close(false); });
      menu.addEventListener("click", function (ev) { if (ev.target.closest && ev.target.closest("a")) close(false); });

      function taskList() {
        var fr = frontmost(), seen = {};
        wins.forEach(function (w) {
          if (!present(w)) return;
          seen[w.id] = true;
          var b = buttons[w.id];
          if (!b) {
            b = buttons[w.id] = document.createElement("button");
            b.type = "button"; b.className = "d95-task";
            var t = document.createElement("span"); b.appendChild(t);
            b.addEventListener("click", function () {
              if (w.min) setMin(w, false);
              else if (frontmost() === w) { setMin(w, true); return; }
              else { raise(w); place(w, true); }
              // the window in front is the active one, with the focus in it, as Windows 95's task bar left it
              focusPane(w.el);
            });
          }
          var title = w.el.getAttribute("data-d95-title") || w.id;
          if (b.firstChild.textContent !== title) b.firstChild.textContent = title;
          b.title = title;
          b.setAttribute("aria-pressed", w === fr ? "true" : "false");
          strip.appendChild(b);  // in the windows' order
        });
        Object.keys(buttons).forEach(function (id) { if (!seen[id] && buttons[id].parentNode) buttons[id].parentNode.removeChild(buttons[id]); });
      }
      function turn(want) {
        bar.hidden = !want;
        if (!want) close(false);
        if (want) { tick(); if (!timer) timer = setInterval(tick, 15000); } else if (timer) { clearInterval(timer); timer = null; }
        // a game page's sound switch sits in the tray, and back on the controls without the desktop
        var snd = document.getElementById("btn-sound");
        if (snd) {
          if (!soundHome) { soundHome = document.createComment("sound switch"); snd.parentNode.insertBefore(soundHome, snd); }
          if (want) tray.insertBefore(snd, clock); else if (soundHome.parentNode) soundHome.parentNode.insertBefore(snd, soundHome.nextSibling);
        }
        // the page's footer: in its About window on the desktop
        var foot = document.querySelector(".byopop-footer");
        if (foot) {
          if (!footHome) {
            footHome = document.createComment("footer");
            foot.parentNode.insertBefore(footHome, foot);
            if (!find("about")) {
              var box = document.createElement("section");
              box.className = "d95-about";
              document.body.appendChild(box);
              add(box, { id: "about", title: T("skin.desk.about"), closable: true, closed: true, body: true,
                at: function (left, vw) { return [Math.max(left, vw - 560), 80]; } });
            }
          }
          var about = find("about");
          if (want && about) (about.el.querySelector(".d95-body") || about.el).appendChild(foot);
          else if (!want && footHome.parentNode) footHome.parentNode.insertBefore(foot, footHome.nextSibling);
        }
        if (want) taskList();
      }
      return { el: bar, turn: turn, tasks: taskList, close: function () { close(false); } };
    }
    // ---- favourites' icons (the user, 2026-11-08: "on Windows 95, a desktop icon specifically for that game") ----
    // Each favourite game (ByopopSettings.favourites, web/settings.js) has an icon after the windows' in the column: its own icon
    // where the player's files hold one (the record's, web/exeicon.js), else its cap in its colour with its initials, and its
    // name under it. A double-click, or Enter or Space, opens its page (a coming-soon game has none: nothing happens); a single
    // click selects it. Its menu (a right click, the menu key or Shift+F10) opens the game or takes it out of the favourites,
    // and its icon goes. The arrows, Home and End move through the menu, Enter opens an entry, Escape closes it.
    var favMenu = null;
    function favIcons() {
      if (!icons) return;
      win95Sync();
      var list = [];
      try { list = window.ByopopSettings && ByopopSettings.favourites ? ByopopSettings.favourites() : []; } catch (e) {}
      var rec = {};
      try { rec = ByopopGameFiles.all(); } catch (e) {}
      Array.prototype.forEach.call(icons.querySelectorAll(".d95-fav"), function (b) { b.parentNode.removeChild(b); });
      list.forEach(function (g) {
        var b = document.createElement("button");
        b.type = "button"; b.className = "d95-icon d95-fav";
        b.setAttribute("data-win95", win95Of(g.id));
        b.setAttribute("aria-label", T2("skin.fav.icon", { game: g.title }));
        b.setAttribute("aria-haspopup", "menu");
        var pic = document.createElement("i"); pic.setAttribute("aria-hidden", "true");
        var r = rec[g.id], url = "";
        try { url = r && ByopopGameFiles.present(r) ? (r.small || r.icon || "") : ""; } catch (e) {}
        if (url) { var im = document.createElement("img"); im.alt = ""; im.draggable = false; im.src = url; pic.appendChild(im); pic.className = "d95-fav-exe"; }
        else { pic.className = "d95-fav-cap"; pic.style.setProperty("--tile", g.colour || "#808080"); var t = document.createElement("span"); t.textContent = g.tag; pic.appendChild(t); }
        var name = document.createElement("span"); name.textContent = g.title;
        b.appendChild(pic); b.appendChild(name);
        function go() { openPage(g.page); }
        b.addEventListener("dblclick", go);
        b.addEventListener("click", function (ev) { if (ev.detail === 0) go(); });
        b.addEventListener("contextmenu", function (ev) { ev.preventDefault(); openFavMenu(b, g, ev.clientX, ev.clientY); });
        b.addEventListener("keydown", function (ev) {
          if (ev.key === "ContextMenu" || (ev.key === "F10" && ev.shiftKey)) { ev.preventDefault(); var rr = b.getBoundingClientRect(); openFavMenu(b, g, rr.right - 8, rr.top + 8, true); }
        });
        icons.appendChild(b);
      });
    }
    // a game's page: its window on the one page (web/deskapps.js), else the page itself
    function openPage(page) {
      if (!page) return;
      try { if (window.ByopopApps && ByopopApps.open(new URL(page, SITE || location.href).href, "push")) return; } catch (e) {}
      location.href = page;
    }
    function T2(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
    function closeFavMenu(back) {
      if (!favMenu) return;
      var m = favMenu; favMenu = null;
      if (m.el.parentNode) m.el.parentNode.removeChild(m.el);
      if (m.opts && m.opts.done) try { m.opts.done(); } catch (e) {}
      if (back && m.from && m.from.parentNode) m.from.focus();
    }
    function openFavMenu(from, g, x, y) {
      popMenu(from, [{ label: T2("skin.fav.open"), bold: true, disabled: !g.page, fn: function () { openPage(g.page); } },
                     { label: T2("skin.fav.remove"), fn: function () {
                       try { ByopopSettings.setFavourite(g, false); } catch (e) {}
                       // its icon is gone: the focus on the column's first icon
                       var first = icons && icons.querySelector(".d95-icon:not([hidden])");
                       if (first) first.focus();
                     } }],
              x, y, T2("skin.fav.menu", { game: g.title }));
    }
    // a small menu at a point (a favourite's icon's, a window's control menu, a game window's menu bar): entries {label, fn,
    // bold, disabled, key, checked (a choice of several: its dot), ticked (an on or off: its tick)} or {rule: true}; the arrows,
    // Home and End move, Enter opens, Escape closes it with the focus back where it came from. opts: side (a menu bar's: the
    // left and right arrows open the menu beside it, side(-1 or 1)), done (called when it closes)
    function popMenu(from, entries, x, y, label, opts) {
      closeFavMenu(false);
      var m = document.createElement("div");
      m.className = "d95-menu d95-ctx"; m.setAttribute("role", "menu"); m.setAttribute("aria-label", label || "");
      entries.forEach(function (en) {
        if (en.rule) { var r = document.createElement("div"); r.className = "d95-rule"; r.setAttribute("role", "separator"); m.appendChild(r); return; }
        // an entry with an address (a store's page) is a link that opens in a new tab, never a script's window.open, which a
        // popup blocker may stop; its name says it opens in a new tab
        var b = document.createElement(en.href ? "a" : "button");
        if (en.href) { b.href = en.href; b.target = "_blank"; b.rel = "noopener noreferrer"; b.setAttribute("aria-label", en.label + " " + ByopopStrings.t("site.new_tab")); }
        else b.type = "button";
        b.className = "d95-item" + (en.bold ? " d95-default" : ""); b.tabIndex = -1;
        b.setAttribute("role", en.checked !== undefined ? "menuitemradio" : en.ticked !== undefined ? "menuitemcheckbox" : "menuitem");
        if (en.checked !== undefined) { b.setAttribute("aria-checked", en.checked ? "true" : "false"); b.classList.add("d95-radio"); }
        if (en.ticked !== undefined) { b.setAttribute("aria-checked", en.ticked ? "true" : "false"); b.classList.add("d95-tick"); }
        var s = document.createElement("span"); s.textContent = en.label; b.appendChild(s);
        if (en.key) { var k = document.createElement("span"); k.className = "d95-key"; k.textContent = en.key; b.appendChild(k); b.setAttribute("aria-keyshortcuts", en.key); }
        if (en.disabled) b.setAttribute("aria-disabled", "true");
        b.addEventListener("click", function () { if (en.disabled) return; closeFavMenu(true); if (en.fn) en.fn(); });
        m.appendChild(b);
      });
      document.body.appendChild(m);
      var vw = document.documentElement.clientWidth, vh = room().h;
      m.style.left = Math.max(0, Math.min(x, vw - m.offsetWidth - 2)) + "px";
      m.style.top = Math.max(0, Math.min(y, vh - m.offsetHeight - 2)) + "px";
      favMenu = { el: m, from: from, opts: opts || null };
      if (entries.some(function (en) { return en.ticked !== undefined; })) m.classList.add("d95-has-ticks");
      var items = Array.prototype.slice.call(m.querySelectorAll(".d95-item"));
      m.addEventListener("keydown", function (ev) {
        var i = items.indexOf(document.activeElement);
        if (ev.key === "ArrowDown") { ev.preventDefault(); items[(i + 1) % items.length].focus(); }
        else if (ev.key === "ArrowUp") { ev.preventDefault(); items[(i - 1 + items.length) % items.length].focus(); }
        else if (ev.key === "Home") { ev.preventDefault(); items[0].focus(); }
        else if (ev.key === "End") { ev.preventDefault(); items[items.length - 1].focus(); }
        else if (ev.key === "Escape") { ev.preventDefault(); closeFavMenu(true); }
        else if (ev.key === "Tab") closeFavMenu(false);
        else if ((ev.key === "ArrowLeft" || ev.key === "ArrowRight") && opts && opts.side) {
          ev.preventDefault();
          var side = opts.side;
          closeFavMenu(false);
          side(ev.key === "ArrowLeft" ? -1 : 1);
        }
      });
      m.addEventListener("pointerover", function (ev) { var b = ev.target.closest && ev.target.closest(".d95-item"); if (b) b.focus(); });
      items[0].focus();
    }
    document.addEventListener("pointerdown", function (ev) { if (favMenu && !favMenu.el.contains(ev.target)) closeFavMenu(false); }, true);
    // Every menu (the Pop menu and its lists, a window's menus, the context menus, What's This?) closes as Windows' did (the
    // user, 2026-11-09: "The dropdown doesn't disappear properly when clicking out of it"): on a press outside it, which still
    // does what it does; on Esc; when its window is minimized, closed or goes, or another window comes to the front. A press in
    // a game's frame never reaches this page: the page's window loses the focus to the frame instead (its blur).
    function closeAllMenus() {
      if (taskbar) try { taskbar.close(); } catch (e) {}
      closeFavMenu(false);
      endWhats();
    }
    window.addEventListener("blur", function () { if (on) closeAllMenus(); });
    document.addEventListener("keydown", function (ev) {
      if (ev.key === "Escape" && on && (favMenu || whatsOn || document.querySelector(".d95-pop:not([hidden]), .d95-menu:not([hidden]):not(.d95-ctx)"))) closeAllMenus();
    });
    // a lifted window: onto the desktop, or back where it was
    function lift(w, up) {
      if (!w.home) return;
      if (up) { if (w.el.parentNode !== document.body) document.body.appendChild(w.el); }
      else if (w.home.parentNode && w.home.nextSibling !== w.el) w.home.parentNode.insertBefore(w.el, w.home.nextSibling);
    }
    // where the windows go the first time: the controls at the top, the game under them, the chat beside it, the panels at the right
    function layout() {
      var vw = room().w;
      var left = icons ? icons.offsetWidth + 24 : 16;
      wins.forEach(function (w) {
        if (!w.placed) {
          var p = w.at ? w.at(left, vw) : [left, 16];
          w.x = p[0]; w.y = p[1];
        }
        w.el.classList.toggle("d95-closed", w.closed);
        w.el.classList.toggle("d95-min", !!w.min);
        place(w, true);
      });
      tasks();
    }
    function find(id) { for (var i = 0; i < wins.length; i++) if (wins[i].id === id) return wins[i]; return null; }
    // ---- the keyboard, as in 1995 (the user, 2026-11-08: "Very unclear how Tab is behaving on that skin") ----
    // The desktop is in panes: each open window (in the order they were added), the icons and the task bar. Tab and Shift+Tab
    // go round the controls of the pane that has the focus and never leave it, as in a dialog of the time; F6 and Shift+F6 go
    // to the next or the one before (the browser keeps Alt+Tab, Ctrl+Esc and the Windows key). A window that takes the focus
    // comes to the front and its title bar is the active one; the others' are grey. In the icons and on the task bar the arrows
    // move between the items. In the game's window the canvas, once it has the focus, gets every key, Tab too (some games use
    // it): F6 leaves it, and F6 to the game's window puts the focus in the game while it runs.
    var STOPS = "a[href], button, input, select, textarea, summary, [tabindex]";
    function stopsIn(pane) {
      return Array.prototype.filter.call(pane.querySelectorAll(STOPS), function (e) {
        if (e.disabled || e.tabIndex < 0 || e.type === "hidden" || e.closest("[hidden]:not(.d95-win), [inert], .d95-closed, .d95-min")) return false;
        if (e.closest(".d95-vbar")) return false;
        var r = e.getClientRects();
        return r.length > 0 && getComputedStyle(e).visibility !== "hidden";
      });
    }
    function taskEl() { return taskbar && taskbar.el; }
    function panes() {
      var out = wins.filter(function (w) { return present(w) && !w.min && w.el.getClientRects().length > 0; }).map(function (w) { return w.el; });
      if (icons && icons.getClientRects().length) out.push(icons);
      if (taskEl() && !taskEl().hidden) out.push(taskEl());
      return out;
    }
    function paneOf(e) {
      if (!e || !e.closest) return null;
      var w = e.closest(".d95-win");
      if (w) return w;
      if (icons && icons.contains(e)) return icons;
      if (taskEl() && taskEl().contains(e)) return taskEl();
      return null;
    }
    function winOf(el) { for (var i = 0; i < wins.length; i++) if (wins[i].el === el) return wins[i]; return null; }
    // the active window: its title bar in colour, the others grey (none while the icons or the task bar have the focus)
    var active = null;
    function activate(w) {
      if (active === w) return;
      // another window to the front: the open menus close, as Windows' did (the task bar's or the desktop's focus is no window,
      // and leaves the Pop menu open)
      if (w) closeAllMenus();
      if (active) active.el.classList.remove("d95-active");
      active = w;
      if (w) { w.el.classList.add("d95-active"); if (String(w.el.style.zIndex) !== String(top)) raise(w); }
      try { window.dispatchEvent(new CustomEvent("byopop-desk-active", { detail: { id: w ? w.id : null } })); } catch (e) {}
    }
    function focusPane(pane, back) {
      var w = winOf(pane);
      if (w && w.focusIn) { activate(w); w.focusIn(back); return; }
      if (w) {
        activate(w);
        // the game's window: the game itself while it runs
        var cv = w.id === "game" && pane.querySelector("canvas[tabindex='0']");
        if (cv && cv.getClientRects().length) { cv.focus(); return; }
        if (w.lastFocus && pane.contains(w.lastFocus) && stopsIn(pane).indexOf(w.lastFocus) >= 0) { w.lastFocus.focus(); return; }
      }
      var s = stopsIn(pane);
      if (pane === taskEl() || pane === icons) {
        // the item that last had the focus there, else the pressed one
        if (pane.lastFocus && s.indexOf(pane.lastFocus) >= 0) { pane.lastFocus.focus(); return; }
        var cur = pane.querySelector("[aria-pressed=true]"); if (cur && s.indexOf(cur) >= 0) { cur.focus(); return; }
      }
      if (s.length) s[back ? s.length - 1 : 0].focus();
      else if (pane.tabIndex >= 0) pane.focus();
    }
    function onKeys(ev) {
      if (!on || ev.defaultPrevented) return;
      var a = document.activeElement;
      // a modal dialog (the skin's card) or a menu keeps its own keys
      if (document.querySelector("dialog[open]") || (a && a.closest && a.closest(".d95-menu"))) return;
      var plain = !ev.ctrlKey && !ev.altKey && !ev.metaKey;
      if (ev.key === "F6" && plain) {
        var list = panes();
        if (!list.length) return;
        ev.preventDefault(); ev.stopImmediatePropagation();
        var i = list.indexOf(paneOf(a));
        var P = parentApps();
        if (P && i >= 0 && (ev.shiftKey ? i === 0 : i === list.length - 1)) {
          ev.preventDefault(); ev.stopImmediatePropagation();
          try { P.f6(window, ev.shiftKey); } catch (e) {}
          return;
        }
        var next = list[i < 0 ? (ev.shiftKey ? list.length - 1 : 0) : (i + (ev.shiftKey ? list.length - 1 : 1)) % list.length];
        focusPane(next, false);
        return;
      }
      if (a && a.tagName === "CANVAS") return;  // the game has every other key
      var pane = paneOf(a);
      if (ev.key === "Tab" && plain) {
        if (!pane) {
          // the focus on nothing of the desktop's (the page itself): the front window's first control
          var w0 = frontmost();
          pane = w0 ? w0.el : panes()[0];
          if (!pane) return;
        }
        var s = stopsIn(pane);
        if (!s.length) { ev.preventDefault(); return; }
        ev.preventDefault();
        var k = s.indexOf(a);
        var j = k < 0 ? (ev.shiftKey ? s.length - 1 : 0) : (k + (ev.shiftKey ? s.length - 1 : 1)) % s.length;
        s[j].focus();
        return;
      }
      // the arrows in the icons and on the task bar
      if (pane && (pane === icons || pane === taskEl()) && plain && /^Arrow(Up|Down|Left|Right)$/.test(ev.key) && !(a.tagName === "INPUT")) {
        var items = stopsIn(pane);
        var n = items.indexOf(a);
        if (n < 0) return;
        ev.preventDefault();
        var fwd = ev.key === "ArrowDown" || ev.key === "ArrowRight";
        var to = items[Math.max(0, Math.min(items.length - 1, n + (fwd ? 1 : -1)))];
        to.focus(); pane.lastFocus = to;
      }
    }
    document.addEventListener("keydown", onKeys, true);
    document.addEventListener("focusin", function (ev) {
      if (!on) return;
      var pane = paneOf(ev.target), w = pane && winOf(pane);
      if (w) { w.lastFocus = ev.target; activate(w); } else if (pane) { pane.lastFocus = ev.target; activate(null); }
    });
    document.addEventListener("pointerdown", function (ev) {
      if (!on) return;
      var pane = paneOf(ev.target), w = pane && winOf(pane);
      activate(w || null);
    }, true);
    function setOn(want) {
      if (want === on) { if (on) { layout(); if (!active) activate(frontmost()); } return; }
      on = want;
      root.classList.toggle("d95-desk", on);
      if (on && !taskbar && !EMBED) taskbar = makeTaskbar();
      if (taskbar) taskbar.turn(on);
      if (on) wins.forEach(function (w) { if (w.unfold) w.unfold(); });
      if (on && !icons && !EMBED) {
        icons = document.createElement("nav");
        icons.className = "d95-icons";
        icons.setAttribute("aria-label", T("skin.desk.icons"));
        document.body.insertBefore(icons, document.body.firstChild);
        wins.forEach(function (w) { if (!w.noIcon) addIcon(w, w.el.getAttribute("data-d95-title") || w.id); });
        favIcons();
        window.addEventListener("byopop-favourites", favIcons);
        window.addEventListener("storage", function (ev) { if (!ev.key || (window.ByopopSettings && ev.key === ByopopSettings.KEY)) favIcons(); });
      }
      // out of the desktop the page lays itself out: the windows' places go, and are put back when it comes again
      wins.forEach(function (w) { lift(w, on); });
      if (!on) wins.forEach(function (w) { ["left", "top", "z-index", "max-width", "max-height", "width", "height"].forEach(function (p) { w.el.style.removeProperty(p); }); });
      else wins.forEach(function (w) { if (w.w && w.h) sizeTo(w, w.w, w.h); });
      try { window.dispatchEvent(new CustomEvent("byopop-desk", { detail: { on: on } })); } catch (e) {}
      if (on) layout();
      // the keys, said once: a line in About
      var ab = find("about"), keysLine = ab && ab.el.querySelector(".d95-keys");
      if (on && ab && !keysLine) {
        keysLine = document.createElement("p"); keysLine.className = "d95-keys"; keysLine.textContent = T("skin.desk.keys");
        (ab.el.querySelector(".d95-body") || ab.el).appendChild(keysLine);
      } else if (!on && keysLine) keysLine.parentNode.removeChild(keysLine);
      activate(on ? frontmost() : null);
    }
    window.addEventListener("resize", function () { if (on) layout(); });
    // a window of another page taken off the desktop (web/deskapps.js)
    function remove(id) {
      var w = find(id);
      if (!w) return;
      closeAllMenus();
      if (active === w) activate(null);
      wins.splice(wins.indexOf(w), 1);
      if (w.el.parentNode) w.el.parentNode.removeChild(w.el);
      tasks();
      if (on) activate(frontmost());
    }
    return { add: add, setOn: setOn, layout: layout, find: find, isOn: function () { return on; }, remove: remove,
             active: function () { return active ? active.id : null; },
             activate: function (id) { var w = find(id); if (w && on && active !== w) activate(w); },
             // the focus into this page from the desktop it is in (F6): its first pane, or its last going back
             enter: function (back) { var p = panes(); if (p.length) focusPane(back ? p[p.length - 1] : p[0], back); },
             // F6 from a window's page, past its own panes: the desktop's pane after (or before) that window
             f6From: function (id, back) {
               var w = find(id), list = panes();
               if (!w || !list.length) return;
               var i = list.indexOf(w.el);
               focusPane(list[(i + (back ? list.length - 1 : 1) + list.length) % list.length], back);
             },
             // a window as the Pop menu opens it
             openWin: function (id) { var w = find(id); if (!w || !on) return; if (w.min) setMin(w, false); if (w.open) { w.open(); raise(w); } else show(w); },
             // a window closed as its Close button closes it (its onClose, or closed and its afterClose)
             close: function (id) { var w = find(id); if (w && !w.closed) closeWin(w); },
             list: function () { return wins.map(function (w) { return { id: w.id, title: w.el.getAttribute("data-d95-title") || w.id, closed: !!w.closed }; }); },
             menu: function (from, entries, x, y, label, opts) { popMenu(from, entries, x, y, label, opts); },
             closeMenus: closeAllMenus,
             raise: function (id) { var w = find(id); if (w && on) raise(w); },
             show: function (id) { var w = find(id); if (w && on) show(w); return !!(w && on); },
             minimize: function (id) { var w = find(id); if (w && on) setMin(w, true); },
             setTitle: function (id, title) { var w = find(id); if (w) setTitle(w, title); },
             place: function (id) { var w = find(id); if (w && on) place(w, true); },
             // a window (its id or its element) taller by up to dh pixels, down to the desktop's bottom edge (Music's splitter,
             // web/music.js): what it grew
             grow: function (id, dh) {
               var w = typeof id === "string" ? find(id) : wins.filter(function (x) { return x.el === id; })[0];
               if (!w || !on || w.closed || w.min || !(dh > 0)) return 0;
               var h = w.el.offsetHeight, most = room().h - w.y - h;
               var by = Math.max(0, Math.min(Math.ceil(dh), most));
               if (!by) return 0;
               sizeTo(w, w.el.offsetWidth, h + by);
               w.placed = true;
               place(w);
               save();
               return by;
             } };
  })();
  // a game page (one with a workspace) or the listing (its games' heading) in Desktop 95, wide enough for windows
  function deskWanted() {
    var listing = !!document.getElementById("games-title");
    return root.getAttribute("data-skin") === "d95" && !!(document.getElementById("workspace") || listing) &&
      (EMBED || listing || (document.documentElement.clientWidth || window.innerWidth) >= DESK_MIN);
  }
  function deskSync() {
    root.classList.toggle("d95-narrow", !EMBED && (document.documentElement.clientWidth || window.innerWidth) < DESK_MIN);
    if (document.body) desk.setOn(deskWanted());
  }

  // ---------- Windows 95 games only ----------
  // Desktop 95's setting "Windows 95 games only" (the user, 2026-11-08: an "immersion" toggle that "will only list the games that
  // actually supported Windows 95"): with it on, and only in Desktop 95, the lists show only the games whose original supported
  // Windows 95 (cmake/ByopopWeb.cmake's BYOPOP_WEB_WIN95, as each tile's data-win95 and the chat's table's win95): All games
  // (html.d95-win95only, web/skins.css), the favourites' icons, and the Pop menu's Games, each list saying how many it hides. An
  // unknown game is hidden too. A game page already open stays as it is.
  function win95Of(id) {
    var games = window.ByopopGames || [];
    for (var i = 0; i < games.length; i++) if (games[i].id === id) return games[i].win95 || "unknown";
    var a = document.querySelector('a.tile[data-game="' + String(id).replace(/[^A-Za-z0-9_-]/g, "") + '"]'), li = a && a.closest("li");
    return (li && li.getAttribute("data-win95")) || "unknown";
  }
  // which window the games' entries open (the setting "Games open in", Desktop 95 only): All games or the Library
  function gamesWindow() { return readItem().gamesOpen === "library" ? "library" : "games"; }
  function win95On() { return root.getAttribute("data-skin") === "d95" && readItem().win95Only === "on"; }
  function win95Hides(id) { return win95On() && win95Of(id) !== "yes"; }
  function win95Sync() {
    var on = win95On();
    root.classList.toggle("d95-win95only", on);
    var count = document.getElementById("win95-count");
    if (!count) return;
    var n = on ? Array.prototype.filter.call(document.querySelectorAll(".games > li[data-win95]"), function (li) {
      // a game the listing already hides (no longer sold, its files not added) is not counted
      return li.getAttribute("data-win95") !== "yes" && !li.hidden && !li.matches(".games.hide-unsold > li[data-unsold]:not([data-files=yes])");
    }).length : 0;
    count.hidden = !n;
    count.textContent = n ? Tv("skin.win95.hidden", { count: n }) : "";
    count.title = T("skin.win95.hidden_title");
  }
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && (ev.detail.name === "win95Only")) win95Sync(); });
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) win95Sync(); });
  // the listing's own filter (no longer sold games shown or not) changes what the count leaves out
  function win95Watch() {
    win95Sync();
    var games = document.querySelector(".games");
    if (!window.MutationObserver) return;
    if (games) new MutationObserver(win95Sync).observe(games, { attributes: true, attributeFilter: ["class"] });
  }
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", win95Watch); else win95Watch();

  window.addEventListener("byopop-skin", deskSync);
  window.addEventListener("resize", deskSync);

  apply();
  // On every page (the user, 2026-11-08: "A lot of elements on Windows 95 need to be made unselectable so that I'm not dragging
  // around the canvas contents as if it were an image"): the game's canvas and its stage start no selection and no drag, and no
  // image of the page's own (caps, icons, logos) is picked up; a card over the stage (the terms, the import) keeps its text.
  // The game's own drags are its pointer events, which this leaves alone.
  function noDrag(ev) {
    var t = ev.target;
    if (!t || !t.closest) return;
    if (t.tagName === "IMG" || (t.closest(".stage") && !t.closest(".overlay"))) ev.preventDefault();
  }
  document.addEventListener("dragstart", noDrag, true);
  if (EMBED) {
    document.addEventListener("click", function (ev) {
      var a = ev.target && ev.target.closest && ev.target.closest("a[href]"), P = parentApps();
      if (!a || !P || ev.defaultPrevented || ev.button !== 0 || ev.ctrlKey || ev.metaKey || ev.shiftKey || ev.altKey) return;
      if ((a.getAttribute("href") || "").charAt(0) === "#" || a.hasAttribute("download")) return;
      if (P.open(a.href, "push")) ev.preventDefault();
    });
    ["pointerdown", "focusin"].forEach(function (n) {
      document.addEventListener(n, function () { var P = parentApps(); if (P) try { P.activated(window); } catch (e) {} }, true);
    });
  }
  document.addEventListener("selectstart", function (ev) { var t = ev.target; if (t && t.nodeType !== 1) t = t.parentNode; if (t && t.closest && t.closest(".stage") && !t.closest(".overlay")) ev.preventDefault(); }, true);
  return { gamesWindow: gamesWindow, EMBED: EMBED, SITE: SITE, testQuery: testQuery, current: current, setGame: setGame, apply: apply, wizard: wizard, desk: desk, deskSync: deskSync };
})();
