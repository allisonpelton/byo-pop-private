// The site's skins (docs/WEB_PLAYER.md, "Skins"): A Demo Disc, B Desktop 95 and C Soda Fountain, each in a light and a dark version
// (web/skins.css), and Plain, the site's look without a skin. The player picks one on their first visit to the site (a card with
// the three), may change it at any time in Settings, and the choice is kept with the site settings: the field "skin" of the item
// "byopop-settings" (web/settings.js), which a game's own settings may override like any other.
//
// This file is loaded in each page's <head> after web/theme.js, so the page is drawn in its skin from the start: it sets
// <html data-skin="demo" | "d95" | "soda"> (nothing for Plain). It follows a change made in Settings on this page (the
// "byopop-setting" event, web/settings.js) and on another of the site's tabs (the storage event).
//
// The card on the first visit: never on a development host (web/terms.js, devHost: local tests never meet it, as with the terms
// card; ?skins=1 shows it there), never on the player-only page, and closing it without a choice keeps Plain, chosen, so that it
// is asked once.
var ByopopSkin = (function () {
  "use strict";
  var KEY = "byopop-settings";
  var SKINS = ["demo", "d95", "soda"];
  var ALL = SKINS.concat(["plain"]);
  var root = document.documentElement;
  var pageGame = null;  // a game page's id (setGame): its own value wins over the site-wide one

  function readItem() {
    try {
      var o = JSON.parse(window.localStorage.getItem(KEY) || "{}");
      return o && typeof o === "object" && !Array.isArray(o) ? o : {};
    } catch (e) { return {}; }
  }
  // the skin this page shows: the game's own, else the site-wide one; null before the player chose any
  function chosen() {
    var o = readItem(), g = pageGame && o.games && o.games[pageGame];
    if (g && ALL.indexOf(g.skin) >= 0) return g.skin;
    return ALL.indexOf(o.skin) >= 0 ? o.skin : null;
  }
  function current() { return chosen() || "plain"; }
  function apply() {
    // the player-only page has no skin (web/shell.html)
    var s = root.classList.contains("player-only") ? "plain" : current();
    if (s === "plain") root.removeAttribute("data-skin"); else root.setAttribute("data-skin", s);
    try { window.dispatchEvent(new CustomEvent("byopop-skin", { detail: { skin: s } })); } catch (e) {}
  }
  // stores the site-wide choice (what Settings does too, through ByopopSettings.set, when it is loaded)
  function choose(s) {
    if (ALL.indexOf(s) < 0) return false;
    var ok = false;
    try {
      if (window.ByopopSettings && ByopopSettings.set) ok = ByopopSettings.set("skin", s);
      else {
        var o = readItem();
        o.skin = s;
        window.localStorage.setItem(KEY, JSON.stringify(o));
        ok = true;
      }
    } catch (e) {}
    apply();
    return ok;
  }
  function setGame(id) { pageGame = id || null; apply(); }

  // ---------- the card on the first visit ----------
  function T(k) { try { return ByopopStrings.t(k); } catch (e) { return k; } }
  var STYLE =
    ".skin-pick { max-width: min(760px, calc(100vw - 32px)); padding: 0; border: 1px solid var(--line); border-radius: 12px; background: var(--panel); color: var(--fg); box-shadow: 0 20px 60px rgba(0,0,0,.4); }\n" +
    ".skin-pick::backdrop { background: rgba(10, 8, 16, .55); }\n" +
    ".skin-pick form { display: grid; gap: 14px; padding: 20px; }\n" +
    ".skin-pick h2 { margin: 0; font-size: 20px; }\n" +
    ".skin-pick p { margin: 0; color: var(--muted); }\n" +
    ".skin-pick .skin-cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 12px; }\n" +
    ".skin-pick .skin-card { display: grid; gap: 8px; text-align: left; padding: 10px; border: 2px solid var(--line); border-radius: 10px; background: var(--bg); color: var(--fg); cursor: pointer; font: inherit; }\n" +
    ".skin-pick .skin-card:hover, .skin-pick .skin-card:focus-visible { border-color: var(--accent); }\n" +
    ".skin-pick .skin-card b { font-size: 15px; }\n" +
    ".skin-pick .skin-card span { color: var(--muted); font-size: 13px; }\n" +
    ".skin-pick .skin-plain { justify-self: start; background: none; border: 0; padding: 0; color: var(--accent); text-decoration: underline; cursor: pointer; font: inherit; }\n" +
    // the previews: each skin's top bar and a cap, drawn here, the same in light and dark
    ".skin-pv { height: 72px; border-radius: 6px; overflow: hidden; position: relative; }\n" +
    ".skin-pv i { position: absolute; display: block; }\n" +
    ".skin-pv-demo { background: linear-gradient(#eaf4ff, #fff); }\n" +
    ".skin-pv-demo .t { left: 0; right: 0; top: 0; height: 22px; background: linear-gradient(#1e6fd0, #3d8ee8 55%, #2a7ddd); border-bottom: 3px solid #ffb547; }\n" +
    ".skin-pv-demo .g { left: 12px; bottom: 10px; width: 64px; height: 18px; border-radius: 10px; background: linear-gradient(#ffc061, #ff8a00 50%, #e66d00); }\n" +
    ".skin-pv-d95 { background: #008080; }\n" +
    ".skin-pv-d95 .t { left: 14px; right: 30px; top: 10px; height: 46px; background: #c0c0c0; box-shadow: inset -1px -1px #000, inset 1px 1px #fff, inset -2px -2px #808080; }\n" +
    ".skin-pv-d95 .g { left: 17px; right: 33px; top: 13px; height: 9px; background: linear-gradient(90deg, #000080, #1084d0); }\n" +
    ".skin-pv-soda { background: #fdf3e1; }\n" +
    ".skin-pv-soda .t { left: 0; right: 0; top: 0; height: 12px; background: repeating-linear-gradient(90deg, #c8102e 0 16px, #fff 16px 32px); }\n" +
    ".skin-pv-soda .g { left: 12px; right: 12px; top: 22px; bottom: 8px; border-radius: 6px; background: radial-gradient(circle at 30% 20%, #3c6b5c, #1f3d34); box-shadow: inset 0 0 0 4px #6b4228; }\n";
  var CARDS = [["demo", "skin.demo", "skin.demo.about"], ["d95", "skin.d95", "skin.d95.about"], ["soda", "skin.soda", "skin.soda.about"]];
  function askOnce() {
    if (chosen() !== null || typeof HTMLDialogElement === "undefined") return;
    if (root.classList.contains("player-only")) return;
    var show = new URLSearchParams(location.search).get("skins") === "1";
    if (!show && typeof ByopopTerms !== "undefined" && ByopopTerms.devHost(location)) return;
    var style = document.createElement("style");
    style.textContent = STYLE;
    document.head.appendChild(style);
    var d = document.createElement("dialog");
    d.className = "skin-pick";
    d.setAttribute("aria-labelledby", "skin-pick-title");
    var form = document.createElement("form");
    form.method = "dialog";
    var h = document.createElement("h2"); h.id = "skin-pick-title"; h.textContent = T("skin.pick.title");
    var p = document.createElement("p"); p.textContent = T("skin.pick.text");
    var cards = document.createElement("div"); cards.className = "skin-cards";
    CARDS.forEach(function (c) {
      var b = document.createElement("button");
      b.type = "submit"; b.value = c[0]; b.className = "skin-card";
      var pv = document.createElement("div"); pv.className = "skin-pv skin-pv-" + c[0]; pv.setAttribute("aria-hidden", "true");
      pv.innerHTML = '<i class="t"></i><i class="g"></i>';
      var name = document.createElement("b"); name.textContent = T(c[1]);
      var about = document.createElement("span"); about.textContent = T(c[2]);
      b.appendChild(pv); b.appendChild(name); b.appendChild(about);
      cards.appendChild(b);
    });
    var plain = document.createElement("button");
    plain.type = "submit"; plain.value = "plain"; plain.className = "skin-plain"; plain.textContent = T("skin.pick.plain");
    var later = document.createElement("p"); later.textContent = T("skin.pick.later");
    form.appendChild(h); form.appendChild(p); form.appendChild(cards); form.appendChild(plain); form.appendChild(later);
    d.appendChild(form);
    document.body.appendChild(d);
    d.addEventListener("close", function () {
      choose(ALL.indexOf(d.returnValue) >= 0 ? d.returnValue : "plain");
      d.remove();
    });
    try { d.showModal(); } catch (e) { d.remove(); }
  }

  // ---------- the setup wizard ----------
  // A game page's three cards before play (the terms, the import, Ready: web/shell.html) become the pages of a setup wizard in
  // a skin: a title bar ("byo-pop Setup: <game>", in C "Bottling <game>"), the steps Licence, Your folder, Copying and Finish
  // with the current one marked, and an art column with the game's cap (the import card's own cap, which the opener still takes
  // off) or C's bottle, which fills as the files are copied. The cards keep their own contents and buttons: the terms card is the
  // licence page with every rule it has (docs/WEB_PLAYER.md, "The terms"). The parts are always in the page, and drawn only in a
  // skin (web/skins.css), so a change of skin needs nothing here; they are hidden from assistive technology, which reads the
  // cards as they are.
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
      head.appendChild(el("span", "wiz-title wiz-title-setup", ByopopStrings.t("skin.wiz.title", { game: o.game })));
      head.appendChild(el("span", "wiz-title wiz-title-bottle", ByopopStrings.t("skin.wiz.bottling", { game: o.game })));
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
      var bottle = el("span", "wiz-bottle");
      bottle.appendChild(el("i"));
      art.appendChild(bottle);
      card.insertBefore(art, head.nextSibling);
      heads.push(card);
    });
    // the import card is "Your folder" until the copy starts, then "Copying"; the bottle fills with it
    if (o.bar && window.MutationObserver) {
      var follow = function () {
        var v = Number(o.bar.getAttribute("aria-valuenow")) || 0;
        heads.forEach(function (card) {
          card.style.setProperty("--wiz-fill", v + "%");
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
  // narrow for windows (under DESK_MIN pixels), or in another skin, the page is laid out as usual. More windows (the performance
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
    // a game page's layout is kept under its id, the listing's under "index"
    function pageKey() { return pageGame || "index"; }
    function saved() { var o = store(), k = pageKey(); return (o[k] && typeof o[k] === "object") ? o[k] : {}; }
    function save() {
      var o = store(), mine = {};
      wins.forEach(function (w) { if (w.placed || w.min) mine[w.id] = { x: w.x, y: w.y, closed: !!w.closed, min: !!w.min, placed: !!w.placed }; });
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
    // minimized: out of sight, its button on the task bar, which brings it back where it was
    function setMin(w, m) {
      w.min = !!m;
      w.el.classList.toggle("d95-min", w.min);
      if (!w.min) { raise(w); place(w, true); }
      save();
      tasks();
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
      var w = { el: el, id: o.id, snap: o.snap || null, x: 0, y: 0, placed: false, closed: !!(o.closed && o.closable), at: o.at, open: o.open,
                button: o.button, pic: o.pic || null, home: null };
      if (o.lift) { w.home = document.createComment("desktop window " + o.id); el.parentNode.insertBefore(w.home, el); }
      el.classList.add("d95-win");
      el.setAttribute("data-d95-title", o.title);
      var handle = o.handle;
      if (!handle) {
        handle = document.createElement("div");
        handle.className = "d95-tb";
        var t = document.createElement("span");
        t.textContent = o.title;
        handle.appendChild(t);
        if (o.closable) {
          var mn = document.createElement("button");
          mn.type = "button"; mn.className = "d95-mn"; mn.setAttribute("aria-label", TW("skin.desk.minimize", o.title));
          mn.addEventListener("click", function () { setMin(w, true); });
          handle.appendChild(mn);
          var x = document.createElement("button");
          x.type = "button"; x.className = "d95-x"; x.setAttribute("aria-label", TW("skin.desk.close", o.title));
          x.addEventListener("click", function () { setClosed(w, true); });
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
      }
      handle.classList.add("d95-handle");
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
      el.addEventListener("pointerdown", function () { if (on) raise(w); }, true);
      var s = saved()[o.id];
      if (s && isFinite(s.x) && isFinite(s.y)) { w.x = s.x; w.y = s.y; w.placed = s.placed !== false; w.closed = !!s.closed && !!o.closable; w.min = !!s.min && !w.closed; }
      if (window.MutationObserver) new MutationObserver(function () { tasks(); }).observe(el, { attributes: true, attributeFilter: ["hidden"] });
      wins.push(w);
      addIcon(w, o.title);
      if (on) { lift(w, true); layout(); }
      return w;
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
        if (o && o.sub) { b.setAttribute("aria-haspopup", "menu"); b.setAttribute("aria-expanded", "false"); b.setAttribute("aria-controls", sub.id); }
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
        // the favourites first, then the others, each by title
        var favs = [];
        try { favs = ByopopSettings.favourites().map(function (f) { return f.id; }); } catch (e) {}
        out.forEach(function (g) { g.fav = favs.indexOf(g.id) >= 0; });
        return out.sort(function (a, b) { return (b.fav - a.fav) || a.title.localeCompare(b.title); });
      }
      function fillSub() {
        sub.textContent = "";
        var games = playable();
        if (!games.length) { var none = item(T("skin.pop.nogames"), null); none.setAttribute("aria-disabled", "true"); sub.appendChild(none); }
        games.forEach(function (g) { sub.appendChild(item(g.title, null, { href: g.page })); });
      }
      function fill() {
        list.textContent = "";
        var listing = !!document.getElementById("games-title");
        var games = item(T("skin.pop.games"), function (ev, b) { openSub(b, true); }, { sub: true, cls: "d95-has-sub" });
        list.appendChild(games);
        list.appendChild(rule());
        function win(id, label) {
          var w = find(id);
          if (!w || (w.button && w.button.hidden)) return;
          list.appendChild(item(label || w.el.getAttribute("data-d95-title") || id, closeThen(function () {
            if (w.min) setMin(w, false);
            if (w.open) { w.open(); raise(w); } else show(w);
          })));
        }
        if (listing) {
          win("tab-settings"); win("tab-players"); win("tab-saves"); win("tab-music");
        } else {
          win("panel-settings");
          [["players", "index.tabs.players"], ["saves", "index.tabs.saves"], ["music", "index.tabs.music"]].forEach(function (m) {
            list.appendChild(item(T(m[1]), null, { href: "./#" + m[0] }));
          });
          list.appendChild(rule());
          // this game's own windows
          wins.forEach(function (w) { if (w.id !== "panel-settings" && w.id !== "about" && !(w.id === "chat" && w.el.hidden)) win(w.id); });
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
        fillSub();
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
        var g = list.querySelector(".d95-has-sub");
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
              else if (frontmost() === w) setMin(w, true);
              else { raise(w); place(w, true); }
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
      return { el: bar, turn: turn, tasks: taskList };
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
      var list = [];
      try { list = window.ByopopSettings && ByopopSettings.favourites ? ByopopSettings.favourites() : []; } catch (e) {}
      var rec = {};
      try { rec = ByopopGameFiles.all(); } catch (e) {}
      Array.prototype.forEach.call(icons.querySelectorAll(".d95-fav"), function (b) { b.parentNode.removeChild(b); });
      list.forEach(function (g) {
        var b = document.createElement("button");
        b.type = "button"; b.className = "d95-icon d95-fav";
        b.setAttribute("aria-label", T2("skin.fav.icon", { game: g.title }));
        b.setAttribute("aria-haspopup", "menu");
        var pic = document.createElement("i"); pic.setAttribute("aria-hidden", "true");
        var r = rec[g.id], url = "";
        try { url = r && ByopopGameFiles.present(r) ? (r.small || r.icon || "") : ""; } catch (e) {}
        if (url) { var im = document.createElement("img"); im.alt = ""; im.src = url; pic.appendChild(im); pic.className = "d95-fav-exe"; }
        else { pic.className = "d95-fav-cap"; pic.style.setProperty("--tile", g.colour || "#808080"); var t = document.createElement("span"); t.textContent = g.tag; pic.appendChild(t); }
        var name = document.createElement("span"); name.textContent = g.title;
        b.appendChild(pic); b.appendChild(name);
        function go() { if (g.page) location.href = g.page; }
        b.addEventListener("dblclick", go);
        b.addEventListener("click", function (ev) { if (ev.detail === 0) go(); });
        b.addEventListener("contextmenu", function (ev) { ev.preventDefault(); openFavMenu(b, g, ev.clientX, ev.clientY); });
        b.addEventListener("keydown", function (ev) {
          if (ev.key === "ContextMenu" || (ev.key === "F10" && ev.shiftKey)) { ev.preventDefault(); var rr = b.getBoundingClientRect(); openFavMenu(b, g, rr.right - 8, rr.top + 8, true); }
        });
        icons.appendChild(b);
      });
    }
    function T2(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
    function closeFavMenu(back) {
      if (!favMenu) return;
      var m = favMenu; favMenu = null;
      if (m.el.parentNode) m.el.parentNode.removeChild(m.el);
      if (back && m.from && m.from.parentNode) m.from.focus();
    }
    function openFavMenu(from, g, x, y, keys) {
      closeFavMenu(false);
      var m = document.createElement("div");
      m.className = "d95-menu d95-ctx"; m.setAttribute("role", "menu"); m.setAttribute("aria-label", T2("skin.fav.menu", { game: g.title }));
      function entry(label, fn, bold) {
        var b = document.createElement("button");
        b.type = "button"; b.className = "d95-item" + (bold ? " d95-default" : ""); b.setAttribute("role", "menuitem"); b.tabIndex = -1;
        var s = document.createElement("span"); s.textContent = label; b.appendChild(s);
        b.addEventListener("click", function () { closeFavMenu(true); fn(); });
        m.appendChild(b);
        return b;
      }
      var first = entry(T2("skin.fav.open"), function () { if (g.page) location.href = g.page; }, true);
      if (!g.page) first.setAttribute("aria-disabled", "true");
      entry(T2("skin.fav.remove"), function () {
        try { ByopopSettings.setFavourite(g, false); } catch (e) {}
        // its icon is gone: the focus on the column's first icon
        var first = icons && icons.querySelector(".d95-icon:not([hidden])");
        if (first) first.focus();
      });
      document.body.appendChild(m);
      var vw = document.documentElement.clientWidth, vh = room().h;
      m.style.left = Math.max(0, Math.min(x, vw - m.offsetWidth - 2)) + "px";
      m.style.top = Math.max(0, Math.min(y, vh - m.offsetHeight - 2)) + "px";
      favMenu = { el: m, from: from };
      var items = Array.prototype.slice.call(m.querySelectorAll(".d95-item"));
      m.addEventListener("keydown", function (ev) {
        var i = items.indexOf(document.activeElement);
        if (ev.key === "ArrowDown") { ev.preventDefault(); items[(i + 1) % items.length].focus(); }
        else if (ev.key === "ArrowUp") { ev.preventDefault(); items[(i - 1 + items.length) % items.length].focus(); }
        else if (ev.key === "Home") { ev.preventDefault(); items[0].focus(); }
        else if (ev.key === "End") { ev.preventDefault(); items[items.length - 1].focus(); }
        else if (ev.key === "Escape") { ev.preventDefault(); closeFavMenu(true); }
        else if (ev.key === "Tab") closeFavMenu(false);
      });
      m.addEventListener("pointerover", function (ev) { var b = ev.target.closest && ev.target.closest(".d95-item"); if (b) b.focus(); });
      items[0].focus();
    }
    document.addEventListener("pointerdown", function (ev) { if (favMenu && !favMenu.el.contains(ev.target)) closeFavMenu(false); }, true);
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
    function setOn(want) {
      if (want === on) { if (on) layout(); return; }
      on = want;
      root.classList.toggle("d95-desk", on);
      if (on && !taskbar) taskbar = makeTaskbar();
      if (taskbar) taskbar.turn(on);
      if (on) wins.forEach(function (w) { if (w.unfold) w.unfold(); });
      if (on && !icons) {
        icons = document.createElement("nav");
        icons.className = "d95-icons";
        icons.setAttribute("aria-label", T("skin.desk.icons"));
        document.body.insertBefore(icons, document.body.firstChild);
        wins.forEach(function (w) { addIcon(w, w.el.getAttribute("data-d95-title") || w.id); });
        favIcons();
        window.addEventListener("byopop-favourites", favIcons);
        window.addEventListener("storage", function (ev) { if (!ev.key || (window.ByopopSettings && ev.key === ByopopSettings.KEY)) favIcons(); });
      }
      // out of the desktop the page lays itself out: the windows' places go, and are put back when it comes again
      wins.forEach(function (w) { lift(w, on); });
      if (!on) wins.forEach(function (w) { ["left", "top", "z-index", "max-width", "max-height"].forEach(function (p) { w.el.style.removeProperty(p); }); });
      try { window.dispatchEvent(new CustomEvent("byopop-desk", { detail: { on: on } })); } catch (e) {}
      if (on) layout();
    }
    window.addEventListener("resize", function () { if (on) layout(); });
    return { add: add, setOn: setOn, layout: layout, find: find, isOn: function () { return on; },
             raise: function (id) { var w = find(id); if (w && on) raise(w); },
             show: function (id) { var w = find(id); if (w && on) show(w); return !!(w && on); },
             minimize: function (id) { var w = find(id); if (w && on) setMin(w, true); },
             place: function (id) { var w = find(id); if (w && on) place(w, true); } };
  })();
  // a game page (one with a workspace) or the listing (its games' heading) in Desktop 95, wide enough for windows
  function deskWanted() {
    return root.getAttribute("data-skin") === "d95" && !!(document.getElementById("workspace") || document.getElementById("games-title")) &&
      (document.documentElement.clientWidth || window.innerWidth) >= DESK_MIN;
  }
  function deskSync() { if (document.body) desk.setOn(deskWanted()); }

  // ---------- ambient light ----------
  // On a game page in the dark Demo Disc and Soda Fountain (B has no dark version), the game's picture glows out around the stage:
  // a sample of it 16 by 12 pixels, scaled a quarter beyond the stage on every side, blurred far and cross-faded between two
  // canvases (html.sk-ambient, web/skins.css). The sample is taken from the game's canvas right after a frame the game drew (the
  // framework's frame hook, Module.byopopTestFrame), while its WebGL picture is still there, every half second; with reduced motion
  // every 8 seconds, without the fade. The light versions have none, and the site setting "Ambient light" turns it off. Nothing
  // runs while it is off.
  var ambient = (function () {
    var glows = null, front = 0, last = -1e9, active = false, hooked = false;
    var still = window.matchMedia && matchMedia("(prefers-reduced-motion: reduce)").matches;
    function isDark() {
      var t = root.getAttribute("data-theme");
      if (t === "dark") return true;
      if (t === "light") return false;
      return !!(window.matchMedia && matchMedia("(prefers-color-scheme: dark)").matches);
    }
    function setting() {
      var o = readItem(), g = pageGame && o.games && o.games[pageGame];
      if (g && (g.ambient === "on" || g.ambient === "off")) return g.ambient;
      return o.ambient === "off" ? "off" : "on";
    }
    function wanted() {
      var s = root.getAttribute("data-skin");
      return (s === "demo" || s === "soda") && isDark() && setting() === "on" &&
        !!document.getElementById("stage") && !!document.getElementById("canvas");
    }
    function position() {
      var stage = document.getElementById("stage");
      if (!glows || !stage) return;
      var wrap = stage.parentNode.getBoundingClientRect(), r = stage.getBoundingClientRect();
      var mx = r.width * 0.14, my = r.height * 0.14;
      glows.forEach(function (c) {
        c.style.left = (r.left - wrap.left - mx) + "px"; c.style.top = (r.top - wrap.top - my) + "px";
        c.style.width = (r.width + 2 * mx) + "px"; c.style.height = (r.height + 2 * my) + "px";
      });
    }
    function sample() {
      var back = glows[1 - front], canvas = document.getElementById("canvas");
      try { back.getContext("2d").drawImage(canvas, 0, 0, 16, 12); } catch (e) { return; }
      back.classList.add("on");
      glows[front].classList.remove("on");
      front = 1 - front;
    }
    function frame(drew) {
      if (!active || !drew) return;
      var now = performance.now();
      if (now - last < (still ? 8000 : 500)) return;
      last = now;
      sample();
    }
    function hook() {
      var M = window.Module;
      if (hooked || !M || root.classList.contains("player-only")) return;
      hooked = true;
      var prev = M.byopopTestFrame;
      M.byopopTestFrame = function (drew, ms) { if (prev) prev(drew, ms); frame(drew); };
    }
    function sync() {
      if (!document.body) return;
      active = wanted();
      root.classList.toggle("sk-ambient", active);
      if (!active) return;
      hook();
      if (!glows) {
        var stage = document.getElementById("stage");
        glows = [0, 1].map(function () {
          var c = document.createElement("canvas");
          c.width = 16; c.height = 12; c.className = "sk-glow"; c.setAttribute("aria-hidden", "true");
          stage.parentNode.insertBefore(c, stage);
          return c;
        });
      }
      position();
    }
    window.addEventListener("resize", function () { if (active) position(); });
    try { matchMedia("(prefers-color-scheme: dark)").addEventListener("change", sync); } catch (e) {}
    if (window.MutationObserver) new MutationObserver(sync).observe(root, { attributes: true, attributeFilter: ["data-theme", "data-skin", "class"] });
    window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "ambient") sync(); });
    window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) sync(); });
    if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", sync); else sync();
    return { sync: sync, position: position };
  })();
  window.addEventListener("byopop-skin", deskSync);
  window.addEventListener("resize", deskSync);

  apply();
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) apply(); });
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "skin") apply(); });
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", askOnce); else askOnce();
  return { SKINS: SKINS, current: current, choose: choose, setGame: setGame, apply: apply, wizard: wizard, desk: desk, deskSync: deskSync, ambient: ambient };
})();
