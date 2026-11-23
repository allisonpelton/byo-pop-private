// The site's look (docs/WEB_PLAYER.md, "Desktop 95"): Desktop 95, an affectionate parody of the 1995 desktop (web/skins.css).
// It was one of three skins until 2026-11-09, when the user kept it alone ("drop the alt skins, including the plain one"); a
// stored choice of another skin is dropped when the settings are read (web/settings.js), and the light/dark theme went with them.
//
// This file is loaded in each page's <head>, so the page is drawn in its look from the start: it sets <html data-skin="d95">
// (nothing on the player-only page, ?player=1, which keeps the page's own plain styles for the sessions' tests).
// A window that changes size step by step sends a resize event for every step: Firefox's full-screen transition, F11 in it,
// a browser window dragged larger. Each one had SDL size the canvas's pixels again and the page and the desktop lay themselves
// out again, so the page lagged and the sound crackled (the user, 2026-11-17: "The site lags a lot and causes audio crackling
// because it tries to resize repeatedly when Firefox enters full screen"). So the browser's own resize events are settled here,
// on every page, before any other listener sees them (a listener capturing at the window runs before those that do not, SDL's
// among them, and no other script captures the resize): the first of a run goes on at once, the rest are held until none has come for
// SETTLE_MS, and then one resize goes on for the size reached. A page's own resizes (its scripts', untrusted) go on at once.
(function () {
  var SETTLE_MS = 120, timer = 0, held = false;
  window.addEventListener("resize", function (ev) {
    if (!ev.isTrusted) return;
    if (timer) { ev.stopImmediatePropagation(); held = true; clearTimeout(timer); }
    timer = setTimeout(function () {
      timer = 0;
      if (held) { held = false; window.dispatchEvent(new Event("resize")); }
    }, SETTLE_MS);
  }, true);
})();
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
  // ---------- the desktop's zoom ----------
  // Desktop 95 is drawn in whole pixels (Byopop Sans, the patterns, the frames), so at a fractional scale (125%, 150%) its
  // pixels cannot all land on the screen's. Its pages zoom so that one CSS pixel is devicePixelRatio rounded of the screen's
  // (the user, 2026-11-11: "Zoom desktop to a whole-number scale"): at 125% the desktop is drawn as at 100%, at 150% as at
  // 200%. A page inside one (a program's window, web/deskapps.js) finds the rounded ratio as its own and zooms by 1. Under the
  // zoom a pointer's position, getBoundingClientRect and the window's size (clientWidth, innerWidth) are the screen's CSS
  // pixels and a style's are the page's: unzoom() turns the one into the other, ratio() is the screen's pixels to the page's,
  // and the style sheets' viewport units are divided by --d95-z. ?deskzoom=<z> sets the zoom for a test at any scale.
  var FORCED_ZOOM = parseFloat(Q.get("deskzoom")) || 0;
  function zoomFor() {
    if (EMBED) return 1;
    if (FORCED_ZOOM > 0) return FORCED_ZOOM;
    var d = window.devicePixelRatio || 1, z = Math.max(1, Math.round(d)) / d;
    return Math.abs(z - 1) < 0.001 ? 1 : z;
  }
  var zoomNow = 1;
  function applyZoom() {
    var z = root.classList.contains("d95-desk") ? zoomFor() : 1;
    if (z === zoomNow) return;
    zoomNow = z;
    if (z === 1) { root.style.removeProperty("zoom"); root.style.removeProperty("--d95-z"); }
    else { root.style.zoom = String(z); root.style.setProperty("--d95-z", String(z)); }
    try { window.dispatchEvent(new CustomEvent("byopop-zoom", { detail: { zoom: z } })); } catch (e) {}
  }
  function zoom() { return zoomNow; }
  function unzoom(v) { return v / zoomNow; }
  function ratio() { return (window.devicePixelRatio || 1) * zoomNow; }
  // the browser's window in the page's CSS pixels
  function viewW() { return (root.clientWidth || window.innerWidth) / zoomNow; }
  function viewH() { return (root.clientHeight || window.innerHeight) / zoomNow; }
  function parentApps() { try { return EMBED && window.parent.ByopopApps ? window.parent.ByopopApps : null; } catch (e) { return null; } }

  function readItem() {
    try {
      var o = JSON.parse(window.localStorage.getItem(KEY) || "{}");
      return o && typeof o === "object" && !Array.isArray(o) ? o : {};
    } catch (e) { return {}; }
  }
  // the look this page shows: Desktop 95, or none on the player-only page
  function current() { return root.classList.contains("player-only") ? "plain" : "d95"; }
  // The skin, the version of Windows the desktop is drawn as (docs/SKINS.md): <html data-look="95">, beside data-skin="d95",
  // which stays the desktop's own name. 95 is the base, drawn by web/skins.css alone, and the only skin so far; each later
  // one is drawn over it by its own style sheet, web/skin-<id>.css, its rules under :root[data-look="<id>"], which only a page
  // drawn in it loads (lookSheet). The player picks one with the setting "look" (Appearance's Skin list, web/settings.js), a
  // choice that redraws the page at once and every other page of the site open in this browser. A test picks one with
  // ?skin=<id>, as ?silent=1
  // does for sound; the address keeps it (testQuery), and the pages in the desktop's windows are opened with it (web/deskapps.js,
  // frameUrl).
  var LOOKS = ["95", "98", "2000"];
  function look() {
    var s = Q.get("skin");
    if (LOOKS.indexOf(s) >= 0) return s;
    s = readItem().look;
    return LOOKS.indexOf(s) >= 0 ? s : "95";
  }
  // the skin's own style sheet, once, from the page's template of them (<template id="skin-sheets">, which the build stamps as
  // it does the page's other files), just after web/skins.css; a page without the template (a copyright page, Jungle Juice's)
  // is drawn in 95, and says so (data-look="95"), so that web/schemes.js gives it 95's colours. True when the page can draw it.
  function lookSheet(id) {
    if (id === "95" || document.querySelector('link[data-look-sheet="' + id + '"]')) return true;
    var t = document.getElementById("skin-sheets"), at = t && t.content.querySelector('link[href^="skin-' + id + '.css"]');
    if (!at) return false;
    var el = at.cloneNode(); el.setAttribute("data-look-sheet", id);
    el.addEventListener("load", bandRamp);
    var base = document.querySelector('link[href^="skins.css"]');
    if (base && base.parentNode) base.parentNode.insertBefore(el, base.nextSibling); else document.head.appendChild(el);
    return true;
  }
  // The Pop menu's banner as a drawing, for a skin whose banner was a 16-colour bitmap: its sheet gives the bitmap as
  // --sk-band-ramp, "<rows> / <colours> / <place> <row>, ...": the bitmap's height in pixels, the colours it steps through,
  // and where each row stands among them, 0 the first colour, 1 the second and so on, a fraction a mix of the two either
  // side, straight between the rows named (docs/SKINS.md, "The banners"). Each mix is drawn as an ordered dither (4 by 4), as
  // a bitmap in 16 colours drew one. The drawing is our own, of the bitmap's colours and steps; it sits at the band's foot, as
  // the bitmap did, with the skin's --sk-band, the bitmap's top colour, above it (web/skins.css, .d95-band).
  var BAYER = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5], bandDrawn = null;
  function bandRamp() {
    var v = "", w = 21;
    try { var cs = getComputedStyle(root); v = cs.getPropertyValue("--sk-band-ramp").trim(); w = parseInt(cs.getPropertyValue("--sk-band-w"), 10) || 21; } catch (e) {}
    if (v === bandDrawn) return;
    bandDrawn = v;
    if (!v) { root.style.removeProperty("--sk-band-img"); return; }
    var parts = v.split("/"), rows = parseInt(parts[0], 10), cols = parts[1].trim().split(/\s+/).map(function (h) {
      return [1, 3, 5].map(function (i) { return parseInt(h.substr(i, 2), 16); });
    }), stops = parts[2].split(",").map(function (p) { var a = p.trim().split(/\s+/); return [parseFloat(a[0]), parseFloat(a[1])]; });
    var cv = document.createElement("canvas"), g = cv.getContext && cv.getContext("2d");
    if (!g || !(rows > 0) || cols.length < 2) return;
    cv.width = w; cv.height = rows;
    var img = g.createImageData(w, rows), px = img.data;
    for (var y = 0; y < rows; y++) {
      var at = stops[0][0];
      for (var i = 0; i < stops.length; i++) {
        if (y < stops[i][1]) { if (i > 0) at = stops[i - 1][0] + (stops[i][0] - stops[i - 1][0]) * (y - stops[i - 1][1]) / (stops[i][1] - stops[i - 1][1]); break; }
        at = stops[i][0];
      }
      var lo = Math.max(0, Math.min(Math.floor(at), cols.length - 2)), mix = at - lo;
      for (var x = 0; x < w; x++) {
        var c = mix * 16 > BAYER[(y % 4) * 4 + (x % 4)] + 0.5 ? cols[lo + 1] : cols[lo], o = (y * w + x) * 4;
        px[o] = c[0]; px[o + 1] = c[1]; px[o + 2] = c[2]; px[o + 3] = 255;
      }
    }
    g.putImageData(img, 0, 0);
    root.style.setProperty("--sk-band-img", 'url("' + cv.toDataURL("image/png") + '")');
  }
  // The behaviours a version had beyond its drawing (docs/SKINS.md, "The behaviours that hide things"), each with the setting
  // that turned it off: <html data-behaviours>, which the settings' rows that only some skins have read (web/settings.js,
  // "needs"). "cues": keyboard cues hidden until the keyboard is used, from 2000 on.
  var BEHAVIOURS = { "2000": ["cues"] };
  // Hidden keyboard cues: Windows 2000 hid the focus rectangles (and access keys' underlines, which no menu here has) until
  // the user showed keyboard use (Raymond Chen, "Why does Windows hide keyboard accelerators and focus rectangles by
  // default?"), with the setting "Hide keyboard navigation indicators until I use the ALT key" (the setting "hideCues",
  // on by default). Here Alt, Tab, an arrow key or F10 shows them for the rest of the page's life.
  var cuesUsed = false;
  function cuesSync() {
    var on = (root.getAttribute("data-behaviours") || "").split(" ").indexOf("cues") >= 0 && readItem().hideCues !== "off";
    root.classList.toggle("d95-cues-hidden", on && !cuesUsed);
  }
  document.addEventListener("keydown", function (ev) {
    if (cuesUsed || !/^(Alt|Tab|ArrowUp|ArrowDown|ArrowLeft|ArrowRight|F10)$/.test(ev.key)) return;
    cuesUsed = true; cuesSync();
  }, true);
  function apply() {
    var s = current();
    if (s === "plain") { root.removeAttribute("data-skin"); root.removeAttribute("data-look"); root.removeAttribute("data-behaviours"); }
    else {
      var l = look(); if (!lookSheet(l)) l = "95";
      root.setAttribute("data-skin", s); root.setAttribute("data-look", l); bandRamp();
      if (BEHAVIOURS[l]) root.setAttribute("data-behaviours", BEHAVIOURS[l].join(" ")); else root.removeAttribute("data-behaviours");
    }
    cuesSync();
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
    if (Q.get("perf") === "1") q.set("perf", "1");
    if (LOOKS.indexOf(Q.get("skin")) >= 0) q.set("skin", Q.get("skin"));
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
    // A window whose id changed comes back where it was, open or closed, its icon where it was placed: { old id: new id }
    // (the user, 2026-11-22: Venice Buildings became the Gallery). Read with the old id moved to the new, the next save
    // writes only the new. A window that went into another one that was already there (All games into the Library, the
    // coordinator's list of 2026-11-22: "Add all the features from All games to Library and then get rid of the all games
    // window") is merged into it: open if either was (minimized as the old one was), at its own place and size where the
    // player gave it one, else at the old one's; an icon keeps its own place, else takes the old one's.
    var RENAMED = { venicebuildings: "gallery", games: "library" };
    function merged(into, from) {
      if (!into || typeof into !== "object" || !from || typeof from !== "object") return into;
      var out = {};
      Object.keys(into).forEach(function (k) { out[k] = into[k]; });
      if (into.closed && from.closed === false) { out.closed = false; out.min = !!from.min; }
      if (!into.placed && from.placed) ["x", "y", "w", "h", "placed"].forEach(function (k) { if (k in from) out[k] = from[k]; });
      return out;
    }
    function carryOver(o, prefix) {
      Object.keys(o).forEach(function (page) {
        var p = o[page];
        if (!p || typeof p !== "object") return;
        Object.keys(RENAMED).forEach(function (old) {
          var was = prefix + old, now = prefix + RENAMED[old];
          if (!Object.prototype.hasOwnProperty.call(p, was)) return;
          if (!Object.prototype.hasOwnProperty.call(p, now)) p[now] = p[was];
          else p[now] = merged(p[now], p[was]);
          delete p[was];
        });
      });
      return o;
    }
    function store() {
      try { var o = JSON.parse(window.localStorage.getItem(DESK_KEY) || "{}"); return o && typeof o === "object" && !Array.isArray(o) ? carryOver(o, "") : {}; }
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
      return { w: viewW(), h: viewH() - b };
    }
    // Draws a window at its own place (w.x, w.y), pulled in only as far as the room needs: its title bar kept on the screen, the
    // game's picture on whole device pixels, and with fit (on opening, on a resize) the whole window on the screen where it is
    // small enough. Where it is drawn (w.dx, w.dy) is never written back into its own place, so a window pulled in by a narrower
    // browser window goes back to where the person left it when the room grows again (the user, 2026-11-11, of snapping the
    // browser to one side: "it shoves my windows to the left"), as Windows does when a display goes and comes back. Its own
    // place changes only by the person (a drag, Alt and the arrows, an edge's resize: commit) or at its first layout.
    function place(w, fit) {
      var r = room(), x = w.x, y = w.y;
      w.el.style.maxWidth = Math.max(200, r.w - 8) + "px";
      w.el.style.maxHeight = Math.max(120, r.h - 8) + "px";
      if (fit && shown(w)) {
        x = Math.max(4, Math.min(x, r.w - w.el.offsetWidth - 4));
        y = Math.max(4, Math.min(y, r.h - w.el.offsetHeight - 4));
      }
      x = Math.max(-(w.el.offsetWidth - 80), Math.min(r.w - 80, Math.round(x)));
      y = Math.max(0, Math.min(r.h - 24, Math.round(y)));
      w.dx = x; w.dy = y;
      w.el.style.left = x + "px";
      w.el.style.top = y + "px";
      if (w.snap && shown(w)) {
        var sr = w.snap.getBoundingClientRect(), d = dpr();
        var sx = unzoom((Math.round(sr.left * d) - sr.left * d) / d), sy = unzoom((Math.round(sr.top * d) - sr.top * d) / d);
        if (sx || sy) { w.el.style.left = (x + sx) + "px"; w.el.style.top = (y + sy) + "px"; }
      }
    }
    // where a window is drawn, as its own place: a move or resize by the person ends with what they see
    function drawn(w) { return { x: w.dx !== undefined ? w.dx : w.x, y: w.dy !== undefined ? w.dy : w.y }; }
    function commit(w) { var p = drawn(w); w.x = p.x; w.y = p.y; }
    function raise(w) { w.el.style.zIndex = String(++top); tasks(); }
    // a window event, for a desktop theme's sounds (web/themes.js): open, close, minimize, restore, maximize, menu, ding
    function deskEvent(what) { try { window.dispatchEvent(new CustomEvent("byopop-desk-event", { detail: { what: what } })); } catch (e) {} }
    function setClosed(w, c) {
      if (!!c !== !!w.closed) deskEvent(c ? "close" : "open");
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
      // as 1995's (the user, 2026-11-14, by the coordinator's list): Restore, Move, Size, Minimize, Maximize and Close, those
      // that do nothing in the window's state greyed; Move and Size work from the arrow keys (keyMove), and the game's window,
      // whose sizes are whole multiples, has no Size but its list of sizes
      var mx = maxed(w);
      var entries = [
        { label: T("skin.desk.m.restore"), disabled: !mx && !w.min, fn: function () { if (w.min) setMin(w, false); else if (w.maximize) w.maximize(); } },
        { label: T("skin.desk.m.move"), disabled: mx, fn: function () { keyMove(w, false); } },
        { label: T("skin.desk.m.resize"), disabled: mx || !!w.sizes || w.fixed, fn: function () { keyMove(w, true); } },
        { label: T("skin.desk.m.minimize"), fn: function () { setMin(w, true); } },
        { label: T("skin.desk.m.maximize"), disabled: mx || !w.maximize, fn: function () { w.maximize(); } },
        { rule: true }];
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
    // What a screen reader makes of a window while the desktop is on: a region named by its title bar's heading. A panel the
    // page shows as a tab elsewhere (Music, Settings and Settings' sections: role tabpanel, named by a tab the desktop hides)
    // loses that role; a landmark of its own (main, footer, aside) keeps its role and takes the title as its name. A heading of
    // the window's own that says the title again goes quiet: one only for screen readers (.pl-sr) is hidden, and a visible one
    // (the chat's collapser) stops being a heading. Off the desktop every attribute is put back as it was.
    var LANDMARKS = { MAIN: 1, FOOTER: 1, ASIDE: 1, NAV: 1, HEADER: 1, FORM: 1 };
    function titleHeading(w) {
      var hd = w.el.querySelector(":scope > .d95-tb > .d95-tb-text");
      if (hd) return hd;
      var hn = w.el.querySelector(":scope > .d95-handle");
      if (hn && /^H[1-6]$/.test(hn.tagName)) return hn;
      return hn ? hn.querySelector("h1, h2, h3, h4, h5, h6") : null;
    }
    function named(w, want) {
      var el = w.el, hd = titleHeading(w);
      if (want && hd) {
        if (!w.aria) {
          w.aria = { role: el.getAttribute("role"), by: el.getAttribute("aria-labelledby"), quiet: [] };
          var text = (el.getAttribute("data-d95-title") || "").trim().toLowerCase();
          Array.prototype.forEach.call(el.querySelectorAll("h1, h2, h3, h4, h5, h6"), function (h) {
            if (h === hd || hd.contains(h) || h.contains(hd) || h.textContent.trim().toLowerCase().indexOf(text) !== 0 || !text) return;
            w.aria.quiet.push({ h: h, hidden: h.hidden, role: h.getAttribute("role") });
            if (h.classList.contains("pl-sr")) h.hidden = true; else h.setAttribute("role", "presentation");
          });
          // (a section named by its title is a region of itself, with no role to say so; another element takes the role)
          if (w.aria.role === "tabpanel" || (!w.aria.role && !LANDMARKS[el.tagName])) { if (el.tagName === "SECTION") el.removeAttribute("role"); else el.setAttribute("role", "region"); }
        }
        if (!hd.id) hd.id = "d95-title-" + String(w.id).replace(/[^A-Za-z0-9_-]/g, "_");
        el.setAttribute("aria-labelledby", hd.id);
      } else if (!want && w.aria) {
        var a = w.aria; w.aria = null;
        if (a.role === null) el.removeAttribute("role"); else el.setAttribute("role", a.role);
        if (a.by === null) el.removeAttribute("aria-labelledby"); else el.setAttribute("aria-labelledby", a.by);
        a.quiet.forEach(function (q) {
          q.h.hidden = q.hidden;
          if (q.role === null) q.h.removeAttribute("role"); else q.h.setAttribute("role", q.role);
        });
      }
    }
    // a window's title (the game's: its name and its mods), on its title bar and task bar button and in its buttons' names
    function setTitle(w, title) {
      if (w.el.getAttribute("data-d95-title") === title) return;
      w.el.setAttribute("data-d95-title", title);
      var h = w.el.querySelector(".d95-tb > .d95-tb-text");
      if (h) h.textContent = title;
      if (on) named(w, true);
      [["d95-mn", "skin.desk.minimize"], ["d95-mx", maxed(w) ? "skin.desk.restore" : "skin.desk.maximize"], ["d95-x", "skin.desk.close"], ["d95-sys", "skin.desk.menu"]].forEach(function (b) {
        var e = w.el.querySelector(".d95-tb > ." + b[0]);
        if (e) e.setAttribute("aria-label", TW(b[1], title));
      });
      tasks();
    }
    // minimized: out of sight, its button on the task bar, which brings it back where it was
    function setMin(w, m) {
      if (m) closeAllMenus();
      if (!!m !== !!w.min) { deskEvent(m ? "minimize" : "restore"); fly(w, !!m); }
      w.min = !!m;
      w.el.classList.toggle("d95-min", w.min);
      if (!w.min) { raise(w); place(w, true); }
      save();
      tasks();
    }
    // The caption's flight (the user, 2026-11-14, by the coordinator's list): as 1995's, a title bar flies from the window's to
    // its task bar button when it is minimized, and back when it is restored; the window shows once it lands. None with reduced
    // motion asked for, nor without the desktop.
    function fly(w, toBar) {
      if (!on || !taskbar || !taskbar.button) return;
      try { if (window.matchMedia && matchMedia("(prefers-reduced-motion: reduce)").matches) return; } catch (e) {}
      var b = taskbar.button(w.id), cap = w.el.querySelector(":scope > .d95-tb") || w.el.querySelector(":scope > .d95-handle");
      if (!b || !cap || !b.getClientRects().length) return;
      var wasMin = w.el.classList.contains("d95-min");
      if (wasMin) w.el.classList.remove("d95-min");
      var cr = cap.getBoundingClientRect(), br = b.getBoundingClientRect();
      if (wasMin) w.el.classList.add("d95-min");
      if (!cr.width || !el95Animate()) return;
      function box(r) { return { left: unzoom(r.left) + "px", top: unzoom(r.top) + "px", width: unzoom(r.width) + "px", height: unzoom(r.height) + "px" }; }
      var f = document.createElement("div");
      f.className = "d95-fly"; f.setAttribute("aria-hidden", "true");
      var title = document.createElement("span"); title.textContent = w.el.getAttribute("data-d95-title") || ""; f.appendChild(title);
      document.body.appendChild(f);
      var a = box(toBar ? cr : br), z = box(toBar ? br : cr);
      if (!toBar) w.el.style.visibility = "hidden";
      var anim = f.animate([a, z], { duration: 220, easing: "linear" });
      // landed: also on a timer, as a page out of sight runs no animation and the window must never stay unseen
      var landed = function () { if (f.parentNode) f.parentNode.removeChild(f); w.el.style.visibility = ""; };
      anim.onfinish = anim.oncancel = landed;
      setTimeout(landed, 300);
    }
    function el95Animate() { return typeof document.body.animate === "function"; }
    // ---- a window's size (the user, 2026-11-08: "Non-game windows should be resizable") ----
    // Every window but the game's (whose sizes are whole multiples, o.sizes) is resized by its edges and corners as in 1995: a
    // band along each edge and a square at each corner, inside its frame, that a press drags. The size goes from the smallest
    // that keeps its controls usable (RZ_MIN) to the room above the task bar; its contents scroll or reflow in it. The size is
    // kept with the window's place (byopop-desk), and goes off with the desktop.
    var RZ_EDGES = ["n", "s", "e", "w", "ne", "nw", "se", "sw"], RZ_MIN = { w: 200, h: 120 };
    // ---- a window's bounds (the user, 2026-11-18: "Evaluate EVERY window to figure out minimum/maximum bounds for proper
    // display"; each window's are in docs/WEB_PLAYER.md's table). A dialog (o.fixed: Settings and every window opened from it,
    // Setup, About) is drawn at its contents' own size and never resized: no edges, no Size in its control menu, no size kept.
    // Any other window goes from its least (RZ_MIN, or more: o.minWidth, o.minHeight) to its most: o.maxWidth where a wider
    // window would only add empty space, and in height its contents' own at that width (tallest), so that no size leaves empty
    // space under them; the room above the task bar caps both, and a least larger than the room gives way to it.
    function bound(w, k) { var f = w[k]; if (!f) return 0; try { return (typeof f === "function" ? f() : f) || 0; } catch (e) { return 0; } }
    // the window's height at this width with nothing scrolled (its contents' own height)
    function tallest(w, width) {
      var st = w.el.style, keep = [st.width, st.height, st.maxHeight];
      st.width = width + "px"; st.height = "auto"; st.maxHeight = "none";
      var h = w.el.offsetHeight;
      st.width = keep[0]; st.height = keep[1]; st.maxHeight = keep[2];
      return h;
    }
    // the bounds for a size this wide (b.w: that width within them)
    function bounds(w, width) {
      var r = room(), b = { minW: Math.max(RZ_MIN.w, bound(w, "minWidth")), minH: Math.max(RZ_MIN.h, bound(w, "minHeight")) };
      b.maxW = Math.max(b.minW, bound(w, "maxWidth") || Infinity);
      b.minW = Math.min(b.minW, r.w - 8); b.maxW = Math.min(b.maxW, r.w - 8);
      b.w = Math.max(b.minW, Math.min(b.maxW, Math.round(width)));
      b.maxH = Math.max(b.minH, w.natural ? tallest(w, b.w) : Infinity);
      b.minH = Math.min(b.minH, r.h - 8); b.maxH = Math.min(b.maxH, r.h - 8);
      return b;
    }
    // ---- the outline (the user, 2026-11-14, by the coordinator's list): a window is moved and sized as in 1995, by a dotted
    // frame that follows the pointer or the arrow keys, and goes there when it is let go. Its place is clamped as place()
    // clamps a window's.
    var ghost = null;
    function clampAt(w, x, y, width) {
      var r = room();
      return { x: Math.max(-(width - 80), Math.min(r.w - 80, Math.round(x))), y: Math.max(0, Math.min(r.h - 24, Math.round(y))) };
    }
    function showGhost(x, y, width, height) {
      if (!ghost) {
        ghost = document.createElement("div");
        ghost.className = "d95-ghost"; ghost.setAttribute("aria-hidden", "true");
        document.body.appendChild(ghost);
      }
      ghost.style.left = x + "px"; ghost.style.top = y + "px";
      ghost.style.width = Math.round(width) + "px"; ghost.style.height = Math.round(height) + "px";
    }
    function hideGhost() { if (ghost && ghost.parentNode) ghost.parentNode.removeChild(ghost); ghost = null; }
    // where a move or a resize ends: the window takes the outline's place and size
    function settle(w, p) {
      if (p.w !== undefined) sizeTo(w, p.w, p.h);
      w.x = p.x; w.y = p.y;
      place(w); commit(w);
      w.placed = true;
      save();
    }
    // a size for edge's drag by dx, dy from st (the window's place and size when it began)
    function sized(w, edge, st, dx, dy) {
      var r = room(), nw = st.w, nh = st.h;
      if (edge.indexOf("e") >= 0) nw = st.w + dx;
      if (edge.indexOf("w") >= 0) nw = st.w - dx;
      if (edge.indexOf("s") >= 0) nh = st.h + dy;
      if (edge.indexOf("n") >= 0) nh = st.h - dy;
      // within the window's bounds (a window may ask for more than RZ_MIN, o.minHeight: Music keeps its player and three of its
      // list's rows; o.minWidth and o.minHeight: a game's window is never smaller than its game, web/deskapps.js)
      var b = bounds(w, nw), leastW = b.minW, least = b.minH;
      nw = b.w;
      nh = Math.max(least, Math.min(b.maxH, nh));
      // and the edge dragged stays on the desktop: the bottom above the task bar, the right inside the screen, the top and the
      // left at 0 at most
      if (edge.indexOf("s") >= 0) nh = Math.max(Math.min(least, r.h - st.y), Math.min(nh, r.h - st.y));
      if (edge.indexOf("e") >= 0) nw = Math.max(Math.min(leastW, r.w - st.x), Math.min(nw, r.w - st.x));
      if (edge.indexOf("n") >= 0) nh = Math.min(nh, st.y + st.h);
      if (edge.indexOf("w") >= 0) nw = Math.min(nw, st.x + st.w);
      // the left and top edges move the window as it grows the other way
      return { x: edge.indexOf("w") >= 0 ? st.x + st.w - nw : st.x, y: edge.indexOf("n") >= 0 ? st.y + st.h - nh : st.y, w: nw, h: nh };
    }
    // Move and Size from the control menu: the arrow keys move the outline (Size: the first one picks the edge it moves), Enter
    // or a click puts the window there, Escape leaves it where it was
    var keying = null;
    function keyMove(w, sizing) {
      if (!on || keying) return;
      var at = drawn(w);
      var st = { x: at.x, y: at.y, w: w.el.offsetWidth, h: w.el.offsetHeight };
      var k = { w: w, sizing: sizing, edge: "", dx: 0, dy: 0, p: { x: st.x, y: st.y } };
      if (sizing) { k.p.w = st.w; k.p.h = st.h; }
      keying = k;
      showGhost(st.x, st.y, st.w, st.h);
      function done(keep) {
        document.removeEventListener("keydown", key, true);
        document.removeEventListener("pointerdown", click, true);
        keying = null;
        hideGhost();
        if (keep) settle(w, k.p);
      }
      function key(ev) {
        var step = { ArrowLeft: [-8, 0], ArrowRight: [8, 0], ArrowUp: [0, -8], ArrowDown: [0, 8] }[ev.key];
        if (ev.key === "Enter" || ev.key === "Escape") { ev.preventDefault(); ev.stopImmediatePropagation(); done(ev.key === "Enter"); return; }
        if (!step) return;
        ev.preventDefault(); ev.stopImmediatePropagation();
        if (sizing && !k.edge) k.edge = { ArrowLeft: "w", ArrowRight: "e", ArrowUp: "n", ArrowDown: "s" }[ev.key];
        k.dx += step[0]; k.dy += step[1];
        if (sizing) k.p = sized(w, k.edge, st, k.dx, k.dy);
        else k.p = clampAt(w, st.x + k.dx, st.y + k.dy, st.w);
        showGhost(k.p.x, k.p.y, sizing ? k.p.w : st.w, sizing ? k.p.h : st.h);
      }
      function click(ev) { ev.preventDefault(); ev.stopImmediatePropagation(); done(true); }
      document.addEventListener("keydown", key, true);
      document.addEventListener("pointerdown", click, true);
    }
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
          var at = drawn(w);
          st = { px: ev.clientX, py: ev.clientY, x: at.x, y: at.y, w: w.el.offsetWidth, h: w.el.offsetHeight, id: ev.pointerId };
          try { h.setPointerCapture(ev.pointerId); } catch (err) {}
          document.documentElement.classList.add("d95-dragging");
        });
        h.addEventListener("pointermove", function (ev) {
          if (!st || ev.pointerId !== st.id) return;
          st.p = sized(w, edge, st, unzoom(ev.clientX - st.px), unzoom(ev.clientY - st.py));
          showGhost(st.p.x, st.p.y, st.p.w, st.p.h);
        });
        var end = function (ev) {
          if (!st || ev.pointerId !== st.id) return;
          var p = st.p;
          st = null;
          document.documentElement.classList.remove("d95-dragging");
          hideGhost();
          if (p) settle(w, p);
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
        var at = drawn(w);
        start = { px: ev.clientX, py: ev.clientY, x: at.x, y: at.y, id: ev.pointerId };
        moved = false;
      });
      window.addEventListener("pointermove", function (ev) {
        if (!start || ev.pointerId !== start.id) return;
        var dx = unzoom(ev.clientX - start.px), dy = unzoom(ev.clientY - start.py);
        if (!moved && Math.abs(dx) + Math.abs(dy) < 4) return;
        if (!moved) { moved = true; try { handle.setPointerCapture(ev.pointerId); } catch (e) {} document.documentElement.classList.add("d95-dragging"); }
        start.p = clampAt(w, start.x + dx, start.y + dy, w.el.offsetWidth);
        showGhost(start.p.x, start.p.y, w.el.offsetWidth, w.el.offsetHeight);
      });
      function end(ev) {
        if (!start || ev.pointerId !== start.id) return;
        var p = start.p;
        start = null;
        document.documentElement.classList.remove("d95-dragging");
        hideGhost();
        if (moved && p) settle(w, p);
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
        commit(w);
        w.x += k[0]; w.y += k[1]; w.placed = true;
        place(w); commit(w); save();
      });
    }
    // el: the window; o: { id, title, handle (its title bar; else one is added), snap (the element on whole device pixels),
    // closable (a close button that hides it; the desktop icon shows it again), closed (closed until the player opens it), open
    // (what the icon does instead: a panel's own button), at: function () { return [x, y]: where it goes first }, body (its
    // contents in a box of their own under the title bar, which scrolls them: a window that is not already laid out that way),
    // lift (on the desktop the window is a child of the page's body, and goes back where it was when the desktop goes: a part of
    // another window, such as a tab of the listing), minimize (on the
    // desktop a click on this part of its own title bar, or the keyboard's press of the title bar, minimizes it, and every other
    // click on the title bar does nothing: the chat's collapser) }
    function add(el, o) {
      if (!el || wins.some(function (w) { return w.el === el; })) return null;
      var w = { focusIn: o.focusIn || null, el: el, id: o.id, keep: o.keep || o.id, snap: o.snap || null, x: 0, y: 0, placed: false, closed: !!(o.closed && o.closable), at: o.at, open: o.open,
                button: o.button, home: null, onClose: o.onClose || null, afterClose: o.afterClose || null, maximize: o.maximize || null, noIcon: !!o.noIcon, sizes: o.sizes || null,
                minHeight: o.minHeight || null, minWidth: o.minWidth || null, maxWidth: o.maxWidth || null, natural: !o.sizes && o.natural !== false,
                fixed: !!o.fixed && !o.sizes, small: o.menuIcon || o.icon || null, lazy: o.lazy || null };
      if (o.lift) { w.home = document.createComment("desktop window " + o.id); el.parentNode.insertBefore(w.home, el); }
      el.classList.add("d95-win");
      el.setAttribute("data-d95-title", o.title);
      el.setAttribute("data-d95-id", o.id);
      var handle = o.handle;
      if (!handle) {
        // the title bar: the window's header where the window is a section (or an aside, a nav, a main), which keeps a header its
        // own; in a footer (System Information) or a dialog a header would be a second banner for the page, so there it is a div
        handle = document.createElement(/^(SECTION|ARTICLE|ASIDE|NAV|MAIN)$/.test(el.tagName) ? "header" : "div");
        handle.className = "d95-tb";
        // the title is the window's heading, under the page's one h1, so that a screen reader's heading keys go from window to
        // window, and its name (named(), below)
        var t = document.createElement("h2");
        t.className = "d95-tb-text";
        t.id = "d95-title-" + String(o.id).replace(/[^A-Za-z0-9_-]/g, "_");
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
            // a double-click on the title bar maximizes and restores, as in Windows 95 (the user, 2026-11-11, by the coordinator);
            // one on a button of the title bar is that button's
            handle.addEventListener("dblclick", function (ev) {
              if (ev.target.closest("input, select, textarea, a") || (ev.target.closest("button") && ev.target.closest("button") !== handle)) return;
              ev.preventDefault();
              o.maximize();
            });
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
      // a part that scrolls on its own (a panel's body, the chat's messages and its people) gets the drawn scroll bar too
      // (one made later, as the chat's, once it is there); o.scroll is an element or selectors, a comma between two parts
      if (o.scroll) {
        var findArea = function () {
          if (typeof o.scroll !== "string") { scrollbar(o.scroll); return true; }
          var parts = o.scroll.split(","), found = 0;
          parts.forEach(function (sel) { var a = el.querySelector(sel.trim()); if (a) { scrollbar(a); found++; } });
          return found === parts.length;
        };
        if (!findArea() && window.MutationObserver) { var mo = new MutationObserver(function () { if (findArea()) mo.disconnect(); }); mo.observe(el, { childList: true, subtree: true }); }
      }
      handle.classList.add("d95-handle");
      // the control menu: every window's title bar has its icon, which opens it (o.menuIcon, else a page's), and a right click
      // on the title bar opens it there, as in 1995 (the user, 2026-11-14, by the coordinator's list). No key opens it: Alt+Space
      // is the browser window's own menu on Windows, and Alt+F4 closes the browser's window (the user, 2026-11-09)
      if (handle.classList.contains("d95-tb")) {
        var sys = document.createElement("button");
        sys.type = "button"; sys.className = "d95-sys";
        sys.setAttribute("aria-label", TW("skin.desk.menu", o.title)); sys.setAttribute("aria-haspopup", "menu");
        // (the window's small icon: its own, o.menuIcon or o.icon, SVG markup: a Settings section's; else its desktop picture, smallPic)
        var mi = o.menuIcon || smallPic(w);
        sys.appendChild(mi);
        handle.insertBefore(sys, handle.firstChild);
        sys.addEventListener("click", function () { var r = sys.getBoundingClientRect(); controlMenu(w, sys, r.left, r.bottom); });
        handle.addEventListener("contextmenu", function (ev) {
          if (!on || ev.target.closest("button:not(.d95-sys), input, select, textarea, a")) return;
          ev.preventDefault();
          controlMenu(w, sys, ev.clientX, ev.clientY);
        });
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
      if (w.fixed) el.classList.add("d95-fixed");
      else if (!o.sizes) resizable(w);
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
          var dx = unzoom(ev.clientX - gs.px), dy = unzoom(ev.clientY - gs.py);
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
      if (!o.sizes && !w.fixed && s && s.w > 0 && s.h > 0) { w.w = Math.round(s.w); w.h = Math.round(s.h); if (on) sizeTo(w, w.w, w.h); }
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
      if (on) { named(w, true); lift(w, true); layout(); }
      // a window of a script still to load that the last visit left open: filled now, and the desktop's loading waits for it
      if (w.lazy && !w.closed) fill(w).catch(function () {});
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
        function move(e) { if (geo.room > 0) { area.scrollTop = s0 + unzoom(e.clientY - y0) * geo.max / geo.room; update(); } }
        function end() { thumb.classList.remove("d95-down"); thumb.removeEventListener("pointermove", move); thumb.removeEventListener("pointerup", end); thumb.removeEventListener("pointercancel", end); }
        thumb.addEventListener("pointermove", move); thumb.addEventListener("pointerup", end); thumb.addEventListener("pointercancel", end);
      });
      box.d95Update = update;
      update();
      return box;
    }
    // ---- a window's pictures (the user, 2026-11-21: "The "Pop" menu is messed up, and could use icons ... the taskbar is missing
    // icons") ----
    // A window's picture is its desktop icon's: at its own size in the Pop menu's first level, and as the window's small icon,
    // which its title bar, its task bar button and the Pop menu's lists show, as a 1995 program had one picture at two sizes. A
    // window of one of the site's apps (every window but a game's) has the app's own icon (the user, 2026-11-22: "Give the
    // other apps on the site better icons than just the window display"), drawn by tools/appicons.py at 32 by 32 and 16 by 16
    // (web/appicons.js) and named by the window's id less "tab-" or "panel-" (the listing's Settings and a game's are one app):
    // the large one on the desktop and in the menu's first level, the small one everywhere else; a Settings section's window
    // ("set-" and the section) has the section's (web/settingsicons.js). A window with a small icon of its own (a game's cap, a
    // Settings section's picture) shows that one. Any other window is drawn as a little window (d95-icon-<id> in
    // web/skins.css), and an entry that is no window draws its own by name (drawnPic: an app's icon where one has the name).
    function appIcon(id) {
      id = String(id);
      var sec = /^set-(.+)$/.exec(id), all = sec ? window.ByopopSettingsIcons : window.ByopopAppIcons;
      return (all && all[sec ? sec[1] : id.replace(/^(tab|panel)-/, "")]) || null;
    }
    function appPic(icon, small, cls) {
      var s = document.createElement("span");
      s.className = "d95-pic d95-appicon " + cls + (small ? " d95-pic-sm" : "");
      s.setAttribute("aria-hidden", "true");
      // (the large one also carries the small, which a menu of small entries shows: d95-menu-small)
      s.innerHTML = (small ? "" : '<span class="d95-appicon-32">' + icon.big + "</span>") + '<span class="d95-appicon-16">' + icon.small + "</span>";
      return s;
    }
    function picOf(w, small) {
      var icon = appIcon(w.id);
      if (icon) return appPic(icon, small, "d95-icon-" + w.id);
      var s = document.createElement("span");
      s.className = "d95-pic d95-icon-" + w.id + (small ? " d95-pic-sm" : "");
      s.setAttribute("aria-hidden", "true");
      s.appendChild(document.createElement("i"));
      return s;
    }
    function drawnPic(name, small) {
      var icon = appIcon(name);
      if (icon) return appPic(icon, small, "d95-pic-" + name);
      var s = document.createElement("span");
      s.className = "d95-pic d95-pic-" + name + (small ? " d95-pic-sm" : "");
      s.setAttribute("aria-hidden", "true");
      s.appendChild(document.createElement("i"));
      return s;
    }
    function smallPic(w) {
      if (typeof w.small === "string") {
        var t = document.createElement("i"); t.className = "d95-tb-icon"; t.setAttribute("aria-hidden", "true"); t.innerHTML = w.small;
        return t;
      }
      if (w.small) { var c = w.small.cloneNode(true); c.removeAttribute("id"); return c; }
      return picOf(w, true);
    }
    // a game's small icon in a list: its own, read out of the player's exe (web/exeicon.js), else its cap in its colour
    function gamePic(id) {
      var rec = null, g = null, url = "";
      try { rec = ByopopGameFiles.all()[id]; url = rec && ByopopGameFiles.present(rec) ? (rec.small || rec.icon || "") : ""; } catch (e) {}
      (window.ByopopGames || []).forEach(function (x) { if (x.id === id) g = x; });
      var s = document.createElement("i"); s.setAttribute("aria-hidden", "true");
      if (url) { s.className = "d95-pic-exe"; var im = document.createElement("img"); im.alt = ""; im.draggable = false; im.src = url; s.appendChild(im); }
      else { s.className = "d95-sys-cap"; if (g && g.colour) s.style.background = g.colour; }
      return s;
    }
    function addIcon(w, title) {
      if (!icons) return;
      var b = document.createElement("button");
      b.type = "button"; b.className = "d95-icon d95-icon-" + w.id;
      b.setAttribute("data-key", "w:" + w.id);
      var pic = document.createElement("i"), icon = appIcon(w.id); pic.setAttribute("aria-hidden", "true");
      if (icon) { pic.innerHTML = icon.big; b.classList.add("d95-icon-app"); }
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
      layoutIcons();
    }
    // opens a window (its icon, a link to it) and puts the focus in it
    // ---- a window whose script loads when it first opens (o.lazy: a function that fills the window's element and returns a
    // promise; the listing's Sounds, Trophies, Fonts, Splits and chat, web/index.html) ----
    // Until it is filled the pointer is Windows 95's "working in background" one, the arrow with a small hourglass
    // (html.d95-starting, web/skins.css), as while a program started; the window opens once it is filled. One that cannot
    // load stays closed and says so.
    var starting = 0;
    function fill(w) {
      if (!w.lazy) return Promise.resolve(w);
      if (!w.filling) {
        var hold = "window " + w.id;
        bootHold(hold);
        if (!starting++) root.classList.add("d95-starting");
        var end = function () { if (!--starting) root.classList.remove("d95-starting"); bootDone(hold); };
        w.filling = Promise.resolve().then(w.lazy).then(function () {
          w.lazy = null; w.filling = null; end();
          if (on && !w.closed) place(w, true);
          return w;
        }, function (e) {
          w.filling = null; end();
          try { console.warn("byo-pop: the window " + w.id + " could not start: " + (e && e.message || e)); } catch (f) {}
          if (!w.closed) setClosed(w, true);
          throw e;
        });
      }
      return w.filling;
    }
    function cannotStart(w) {
      var title = w.el.getAttribute("data-d95-title") || w.id;
      if (on) messageBox(title, TW("skin.desk.start_failed", title), false);
    }
    function show(w) {
      if (w.lazy) { fill(w).then(function () { if (on) show(w); }, function () { cannotStart(w); }); return; }
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
    // A menu is a <menu> with the menu role, each entry in a list item of its own that is no item to a screen reader
    // (role none) and lays no box of its own (web/skins.css: the entries are drawn as the menu's own children); a rule
    // between entries is an <hr>
    function menuEntry(m, el) { var li = document.createElement("li"); li.setAttribute("role", "none"); li.appendChild(el); m.appendChild(li); return el; }
    function menuRule() { var r = document.createElement("hr"); r.className = "d95-rule"; return r; }
    // the task bar, a navigation landmark: its buttons go to the windows, and its Pop menu to the site's windows and pages
    function makeTaskbar() {
      var bar = document.createElement("nav"), buttons = {}, clock, timer = null, menu, sub, pop, list, soundHome = null, footHome = null;
      bar.className = "d95-taskbar";
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
      // the menu's box, its banner beside its list; the list is the menu
      menu = document.createElement("div");
      menu.className = "d95-menu"; menu.hidden = true;
      var band = document.createElement("div"); band.className = "d95-band"; band.setAttribute("aria-hidden", "true");
      // the name in two colours, as Windows 95's banner had "Windows" in grey and "95" in white (skins.css): up to its last hyphen, then the rest
      var bandText = document.createElement("span"), name = T("skin.pop.band"), cut = name.lastIndexOf("-") + 1;
      var first = document.createElement("span"); first.className = "d95-band-1"; first.textContent = name.slice(0, cut);
      bandText.appendChild(first); bandText.appendChild(document.createTextNode(name.slice(cut))); band.appendChild(bandText);
      list = document.createElement("menu"); list.className = "d95-items"; list.id = "d95-pop-menu";
      list.setAttribute("role", "menu"); list.setAttribute("aria-labelledby", "d95-pop");
      menu.appendChild(band); menu.appendChild(list);
      sub = document.createElement("menu");
      sub.className = "d95-menu d95-sub"; sub.id = "d95-pop-games"; sub.hidden = true;
      sub.setAttribute("role", "menu");
      wrap.appendChild(pop); wrap.appendChild(menu); wrap.appendChild(sub);
      var strip = document.createElement("div"); strip.className = "d95-tasks";
      var tray = document.createElement("div"); tray.className = "d95-tray";
      clock = document.createElement("time"); clock.className = "d95-clock";
      // the speaker: a click opens the volume's slider above it, with Mute (ByopopVolume)
      var spk = document.createElement("button");
      spk.type = "button"; spk.className = "d95-spk"; spk.setAttribute("aria-label", T("skin.vol.button")); spk.title = T("skin.vol.button");
      spk.setAttribute("aria-haspopup", "dialog"); spk.setAttribute("aria-expanded", "false");
      var spkPic = document.createElement("i"); spkPic.setAttribute("aria-hidden", "true"); spk.appendChild(spkPic);
      tray.appendChild(spk);
      tray.appendChild(clock);
      var vol = null;
      function showMuted() { spk.classList.toggle("d95-spk-off", ByopopVolume.muted()); }
      ByopopVolume.on(function () { showMuted(); if (vol) { vol.range.value = ByopopVolume.get(); vol.mute.checked = ByopopVolume.muted(); } });
      showMuted();
      function closeVol(focusBack) {
        if (!vol) return;
        document.removeEventListener("pointerdown", vol.out, true);
        if (vol.el.parentNode) vol.el.parentNode.removeChild(vol.el);
        vol = null;
        spk.setAttribute("aria-expanded", "false");
        if (focusBack) spk.focus();
      }
      spk.addEventListener("click", function () {
        if (vol) { closeVol(false); return; }
        closeAllMenus();
        // a dialog, not modal, open from the start (Escape and a press outside it close it)
        var el = document.createElement("dialog"); el.open = true;
        el.className = "d95-vol"; el.setAttribute("aria-label", T("skin.vol.title"));
        var head = document.createElement("div"); head.className = "d95-vol-head"; head.textContent = T("skin.vol.title");
        var range = document.createElement("input");
        range.type = "range"; range.min = "0"; range.max = "100"; range.step = "1"; range.value = ByopopVolume.get();
        range.className = "d95-vol-range"; range.setAttribute("aria-label", T("skin.vol.title")); range.setAttribute("orient", "vertical");
        var lab = document.createElement("label"); lab.className = "d95-vol-mute";
        var mute = document.createElement("input"); mute.type = "checkbox"; mute.checked = ByopopVolume.muted();
        lab.appendChild(mute); lab.appendChild(document.createTextNode(" " + T("skin.vol.mute")));
        el.appendChild(head); el.appendChild(range); el.appendChild(lab);
        range.addEventListener("input", function () { ByopopVolume.set(+range.value); });
        mute.addEventListener("change", function () { ByopopVolume.set(null, mute.checked); });
        el.addEventListener("keydown", function (ev) { if (ev.key === "Escape") { ev.preventDefault(); closeVol(true); } });
        document.body.appendChild(el);
        var r = spk.getBoundingClientRect();
        el.style.left = Math.max(0, Math.min(unzoom(r.right) - el.offsetWidth, viewW() - el.offsetWidth - 2)) + "px";
        el.style.top = (unzoom(r.top) - el.offsetHeight - 4) + "px";
        vol = { el: el, range: range, mute: mute, out: function (ev) { if (!el.contains(ev.target) && ev.target !== spk && !spk.contains(ev.target)) closeVol(false); } };
        document.addEventListener("pointerdown", vol.out, true);
        spk.setAttribute("aria-expanded", "true");
        range.focus();
      });
      bar.appendChild(wrap); bar.appendChild(strip); bar.appendChild(tray);
      document.body.appendChild(bar);
      bar.addEventListener("contextmenu", function (ev) {
        if (ev.target.closest(".d95-popwrap, .d95-menu")) return;
        ev.preventDefault();
        barMenu(ev.clientX, ev.clientY);
      });

      function tick() {
        var d = new Date();
        try { clock.textContent = d.toLocaleTimeString([], { hour: "numeric", minute: "2-digit" }); } catch (e) { clock.textContent = d.getHours() + ":" + ("0" + d.getMinutes()).slice(-2); }
      }
      function item(label, act, o) {
        var b = document.createElement(o && o.href ? "a" : "button");
        if (o && o.href) b.href = o.href; else b.type = "button";
        b.className = "d95-item" + (o && o.cls ? " " + o.cls : "");
        b.setAttribute("role", "menuitem"); b.tabIndex = -1;
        // its picture (o.pic), in a column of its own so that the names line up (none for a choice of several, o.noPic)
        if (!(o && o.noPic)) { var ps = document.createElement("span"); ps.className = "d95-item-pic"; if (o && o.pic) ps.appendChild(o.pic); b.appendChild(ps); }
        var t = document.createElement("span"); t.textContent = label; b.appendChild(t);
        if (o && o.sub) { b.setAttribute("aria-haspopup", "menu"); b.setAttribute("aria-expanded", "false"); b.setAttribute("aria-controls", sub.id); b.subFill = o.fill || null; }
        if (act) b.addEventListener("click", function (ev) { act(ev, b); });
        return b;
      }
      function rule() { return menuRule(); }
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
        // the games' window, the Library (web/library.js): on the listing, first
        var gw = document.getElementById("games-title") && find("library");
        if (gw) {
          menuEntry(sub, item(gw.el.getAttribute("data-d95-title") || gw.id, closeThen(function () { if (gw.min) setMin(gw, false); show(gw); }), { pic: smallPic(gw) }));
          menuEntry(sub, rule());
        }
        var games = playable();
        if (!games.length) { var none = item(T("skin.pop.nogames"), null); none.setAttribute("aria-disabled", "true"); menuEntry(sub, none); }
        var shown = games.filter(function (g) { return !win95Hides(g.id); });
        shown.forEach(function (g) { menuEntry(sub, item(g.title, null, { href: g.page, pic: gamePic(g.id) })); });
        if (games.length > shown.length) {
          var n = games.length - shown.length;
          var hid = item(Tv("skin.win95.hidden", { count: n }), null);
          hid.setAttribute("aria-disabled", "true");
          hid.title = T("skin.win95.hidden_title");
          if (!shown.length) sub.textContent = "";
          menuEntry(sub, hid);
        }
      }
      function fill() {
        list.textContent = "";
        var listing = !!document.getElementById("games-title");
        var games = item(T("skin.pop.games"), function (ev, b) { openSub(b, true); }, { sub: true, cls: "d95-has-sub", pic: drawnPic("folder") });
        menuEntry(list, games);
        menuEntry(list, rule());
        function openWin(w) {
          if (w.min) setMin(w, false);
          if (w.open) { w.open(); raise(w); } else show(w);
        }
        function win(id, label) {
          var w = find(id);
          if (!w || (w.button && w.button.hidden)) return;
          menuEntry(list, item(label || w.el.getAttribute("data-d95-title") || id, closeThen(function () { openWin(w); }), { pic: picOf(w, false) }));
        }
        // Settings, with its sections in a list beside it: the folder itself first, then each section's window
        function settings(id) {
          var w = find(id), secs = wins.filter(function (x) { return /^set-/.test(x.id); });
          if (!w) return;
          if (!secs.length) { win(id); return; }
          menuEntry(list, item(w.el.getAttribute("data-d95-title") || id, function (ev, b) { openSub(b, true); }, { sub: true, cls: "d95-has-sub", pic: picOf(w, false), fill: function () {
            sub.textContent = "";
            menuEntry(sub, item(T("skin.pop.allsettings"), closeThen(function () { openWin(w); }), { pic: smallPic(w) }));
            menuEntry(sub, rule());
            secs.forEach(function (s) { menuEntry(sub, item(s.el.getAttribute("data-d95-title"), closeThen(function () { openWin(s); }), { pic: smallPic(s) })); });
          } }));
        }
        if (listing) {
          // the Setup window (web/setup.js), for adding games
          win("setup", T("setup.pop"));
          win("library");
          settings("tab-settings"); win("tab-music"); win("sounds"); win("splits"); win("chat");
        } else {
          settings("panel-settings");
          [["music", "index.tabs.music"], ["sounds", "sounds.title"]].forEach(function (m) {
            menuEntry(list, item(T(m[1]), null, { href: "./#" + m[0], pic: drawnPic(m[0]) }));
          });
          menuEntry(list, rule());
          // the game's full screen, as its window's maximize button
          var gw = find("game");
          if (gw && gw.maximize) menuEntry(list, item(T("skin.pop.full"), closeThen(gw.maximize), { pic: drawnPic("full") }));
          if (gw && gw.sizes) {
            var sz = gw.sizes();
            if (sz && sz.list.length) menuEntry(list, item(T("skin.pop.size"), function (ev, b) { openSub(b, true); }, { sub: true, cls: "d95-has-sub", pic: drawnPic("size"), fill: function () {
              sub.textContent = "";
              var now = gw.sizes();
              now.list.forEach(function (s) {
                var it = item(s.label, closeThen(function () { now.set(s.n); }), { noPic: true });
                it.setAttribute("role", "menuitemradio"); it.setAttribute("aria-checked", s.n === now.current ? "true" : "false"); it.classList.add("d95-radio");
                menuEntry(sub, it);
              });
            } }));
          }
          // this game's own windows
          wins.forEach(function (w) { if (w.id !== "panel-settings" && w.id !== "about" && !/^set-/.test(w.id) && !(w.id === "chat" && w.el.hidden)) win(w.id); });
        }
        menuEntry(list, rule());
        // About, or System Information on the listing page (web/sysinfo.js): the window's own title
        win("about");
        // a game's page: the Library, on the listing (its #library opens it)
        if (!listing) {
          menuEntry(list, rule());
          menuEntry(list, item(T("library.title"), null, { href: "./#library", cls: "d95-leave", pic: drawnPic("library") }));
        }
        // Shut Down, last (the user, 2026-11-14, by the coordinator's list): every window closes, then the last screen
        menuEntry(list, rule());
        menuEntry(list, item(T("skin.pop.shutdown"), closeThen(shutDown), { cls: "d95-shutdown", pic: drawnPic("shutdown") }));
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
        sub.style.left = (unzoom(r.right - wr.left) - 3) + "px";
        sub.style.bottom = "auto";
        sub.style.top = "0px";
        var h = sub.offsetHeight, topAt = unzoom(r.top - wr.top) - 3, bar = (wrap.closest(".d95-taskbar") || wrap).getBoundingClientRect();
        // level with its entry, as 1995's lists opened, and raised only as far as keeps it above the task bar and on the screen
        sub.style.top = Math.max(unzoom(-wr.top), Math.min(topAt, unzoom(bar.top - wr.top) - h)) + "px";
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
        menu.classList.remove("d95-menu-small");
        menu.hidden = false;
        // a screen too short for the large pictures takes the small ones, as 1995's Start menu offered
        if (menu.getBoundingClientRect().top < 0) menu.classList.add("d95-menu-small");
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
      // (on the list, the menu itself: its box holds only the banner besides)
      keys(list, false); keys(sub, true);
      // a pointer over an entry is the focus; over the games, their list
      list.addEventListener("pointerover", function (ev) {
        var b = ev.target.closest && ev.target.closest(".d95-item");
        if (!b) return;
        b.focus();
        if (b.classList.contains("d95-has-sub")) openSub(b, false); else closeSub(false);
      });
      sub.addEventListener("pointerover", function (ev) { var b = ev.target.closest && ev.target.closest(".d95-item"); if (b) b.focus(); });
      document.addEventListener("pointerdown", function (ev) { if (!menu.hidden && !wrap.contains(ev.target)) close(false); }, true);
      sub.addEventListener("click", function (ev) { if (ev.target.closest && ev.target.closest("a")) close(false); });
      list.addEventListener("click", function (ev) { if (ev.target.closest && ev.target.closest("a")) close(false); });

      function taskList() {
        var fr = frontmost(), seen = {};
        wins.forEach(function (w) {
          if (!present(w)) return;
          seen[w.id] = true;
          var b = buttons[w.id];
          if (!b) {
            b = buttons[w.id] = document.createElement("button");
            b.type = "button"; b.className = "d95-task";
            b.addEventListener("contextmenu", function (ev) { ev.preventDefault(); ev.stopPropagation(); controlMenu(w, b, ev.clientX, ev.clientY); });
            // the window's small icon, then its title (the front window's pressed in, its title bold, as 1995's)
            b.appendChild(smallPic(w));
            var t = document.createElement("span"); t.className = "d95-task-text"; b.appendChild(t); b.text = t;
            b.addEventListener("click", function () {
              if (w.min) setMin(w, false);
              else if (frontmost() === w) { setMin(w, true); return; }
              else { raise(w); place(w, true); }
              // the window in front is the active one, with the focus in it, as Windows 95's task bar left it
              focusPane(w.el);
            });
          }
          var title = w.el.getAttribute("data-d95-title") || w.id;
          if (b.text.textContent !== title) b.text.textContent = title;
          b.title = title;
          b.setAttribute("aria-pressed", w === fr ? "true" : "false");
          strip.appendChild(b);  // in the windows' order
        });
        Object.keys(buttons).forEach(function (id) { if (!seen[id] && buttons[id].parentNode) buttons[id].parentNode.removeChild(buttons[id]); });
        fitTasks();
      }
      // the buttons share the bar in whole pixels, as 1995's did: each 160 wide while they fit, else the room shared out, never
      // under 40 (a width with a fraction would put each icon after the first between the screen's pixels)
      function fitTasks() {
        var list = strip.children, n = list.length;
        if (!n || !on || !strip.clientWidth) return;
        var cs = getComputedStyle(strip), gap = parseFloat(cs.columnGap) || 0;
        var room = unzoom(strip.clientWidth) - (parseFloat(cs.paddingLeft) || 0) - (parseFloat(cs.paddingRight) || 0) - gap * (n - 1);
        var each = Math.max(40, Math.min(160, Math.floor(room / n)));
        for (var i = 0; i < n; i++) list[i].style.flex = "0 0 " + each + "px";
      }
      window.addEventListener("resize", fitTasks);
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
              add(box, { id: "about", title: T("skin.desk.about"), closable: true, closed: true, body: true, fixed: true,
                at: function (left, vw) { return [Math.max(left, vw - 560), 80]; } });
            }
          }
          var about = find("about");
          if (want && about) (about.el.querySelector(".d95-body") || about.el).appendChild(foot);
          else if (!want && footHome.parentNode) footHome.parentNode.insertBefore(foot, footHome.nextSibling);
        }
        if (want) taskList();
      }
      // the menu open on Shut Down, for Alt+4 with no window open (Enter then shuts down, Escape leaves it)
      function openShutDown() { if (menu.hidden) open(); var b = list.querySelector(".d95-shutdown"); if (b) b.focus(); }
      return { el: bar, turn: turn, tasks: taskList, close: function () { close(false); }, openShutDown: openShutDown,
               button: function (id) { return buttons[id] || null; } };
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
        b.setAttribute("data-key", "f:" + g.id);
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
      layoutIcons();
    }
    // ---- the desktop's icons where the player puts them, and the desktop's own menus (the user, 2026-11-14, by the
    // coordinator's list) ----
    // The icons stand in columns down the left until one is dragged; from then on each stays where it was put (kept for the
    // page, byopop-desk-icons), a new one taking the first free place. Arrange Icons puts them back in their columns, Line up
    // Icons moves each to the nearest free place of those columns. A drag on the empty desktop draws a dotted rectangle that
    // selects the icons it touches, and a drag of a selected icon moves every selected one.
    var ICON_KEY = "byopop-desk-icons", CELL = { w: 88, h: 64 };
    function iconStore() { try { var o = JSON.parse(localStorage.getItem(ICON_KEY) || "{}"); return o && typeof o === "object" ? carryOver(o, "w:") : {}; } catch (e) { return {}; } }
    function iconPlaces() { var o = iconStore()[pageKey()]; return o && typeof o === "object" ? o : null; }
    function keepIconPlaces(p) {
      var o = iconStore();
      if (p) o[pageKey()] = p; else delete o[pageKey()];
      try { localStorage.setItem(ICON_KEY, JSON.stringify(o)); } catch (e) {}
    }
    function iconList() { return icons ? Array.prototype.filter.call(icons.querySelectorAll(":scope > .d95-icon"), function (b) { return !b.hidden; }) : []; }
    function cellRows() { return Math.max(1, Math.floor((icons.clientHeight || 400) / CELL.h)); }
    function cellAt(n) { var rows = cellRows(); return [Math.floor(n / rows) * CELL.w, (n % rows) * CELL.h]; }
    function layoutIcons() {
      if (!icons) return;
      var p = on ? iconPlaces() : null;
      icons.classList.toggle("d95-icons-free", !!p);
      var list = iconList();
      if (!p) { list.forEach(function (b) { b.style.left = b.style.top = ""; }); return; }
      var taken = {}, n = 0;
      list.forEach(function (b) { var at = p[b.getAttribute("data-key")]; if (at) taken[at[0] + "," + at[1]] = true; });
      list.forEach(function (b) {
        var at = p[b.getAttribute("data-key")];
        if (!at) { do { at = cellAt(n++); } while (taken[at[0] + "," + at[1]]); taken[at[0] + "," + at[1]] = true; }
        b.style.left = at[0] + "px"; b.style.top = at[1] + "px";
      });
    }
    // every icon's place as it is drawn now (the columns' when none was moved yet)
    function iconsNow() {
      var p = {};
      iconList().forEach(function (b) { p[b.getAttribute("data-key")] = [b.offsetLeft, b.offsetTop]; });
      return p;
    }
    function arrangeIcons() { keepIconPlaces(null); layoutIcons(); }
    // Games installed by a drop on the desktop (the user, 2026-11-16: "If install is successful, create a desktop shortcut in
    // that location on the desktop"): the place of an icon whose picture is under a point of the window (iconPlaceAt, at the
    // drop), then each game not yet a favourite made one (addShortcuts, when Setup has installed them, web/setup.js), the first
    // at that place, the others in the next free places. Every icon keeps the place it has, as at the first drag of one.
    function iconPlaceAt(clientX, clientY) {
      if (!on || !icons) return null;
      var r = icons.getBoundingClientRect();
      return [Math.max(0, Math.round(unzoom(clientX - r.left) - CELL.w / 2 + 2)), Math.max(0, Math.round(unzoom(clientY - r.top) - 20))];
    }
    function addShortcuts(ids, at) {
      if (!on || !icons || !window.ByopopSettings) return;
      var have = [];
      try { have = ByopopSettings.favourites().map(function (f) { return f.id; }); } catch (e) {}
      var add = ids.filter(function (id, i) { return have.indexOf(id) < 0 && ids.indexOf(id) === i; }).map(function (id) {
        var a = document.querySelector('a.tile[data-game="' + id + '"]'), li = a && a.closest("li");
        if (!li) return null;
        var name = li.querySelector(".name"), badge = li.querySelector(".badge");
        // as the card's star makes it (web/index.html)
        return { id: id, title: li.getAttribute("data-title") || (name ? name.textContent : id), page: a.getAttribute("href") || "",
                 colour: (/--tile:\s*(#[0-9a-f]{3,8})/i.exec(li.getAttribute("style") || "") || [])[1] || "",
                 tag: badge ? (badge.getAttribute("data-initials") || badge.textContent || "").trim().slice(0, 4) : "" };
      }).filter(Boolean);
      if (!add.length) return;
      if (at) {
        var p = iconPlaces() || iconsNow();
        p["f:" + add[0].id] = at;
        keepIconPlaces(p);
      }
      add.forEach(function (g) { try { ByopopSettings.setFavourite(g, true); } catch (e) {} });
    }
    function lineUpIcons() {
      var p = iconPlaces();
      if (!p) return;
      var rows = cellRows(), taken = {}, out = {};
      iconList().forEach(function (b) {
        var k = b.getAttribute("data-key"), at = p[k] || [b.offsetLeft, b.offsetTop];
        var c = Math.max(0, Math.round(at[0] / CELL.w)), r = Math.max(0, Math.min(rows - 1, Math.round(at[1] / CELL.h)));
        var n = c * rows + r;
        while (taken[n]) n++;
        taken[n] = true;
        out[k] = cellAt(n);
      });
      keepIconPlaces(out);
      layoutIcons();
    }
    // The recycle bin's icon (the user, 2026-11-14, by the coordinator's list), on the listing: it opens the bin's panel in the
    // Library (web/addgames.js, web/library.js), a game dragged on it (a Library row) asks there to forget its files, a desktop
    // shortcut dropped on it is removed (binUnder), and it is full while the bin holds the names of the games forgotten
    // (byopop-bin), until Empty Recycle Bin, its menu's, clears them with the theme's sound
    var BIN_KEY = "byopop-bin";
    function binItems() { try { var a = JSON.parse(localStorage.getItem(BIN_KEY) || "[]"); return Array.isArray(a) ? a : []; } catch (e) { return []; } }
    function openBin(id) {
      var gw = find("library");
      if (gw) { if (gw.min) setMin(gw, false); show(gw); }
      if (window.ByopopAddGames && ByopopAddGames.bin) ByopopAddGames.bin(id || null);
    }
    function emptyBin() {
      try { localStorage.removeItem(BIN_KEY); } catch (e) {}
      deskEvent("empty");
      try { window.dispatchEvent(new CustomEvent("byopop-bin")); } catch (e) {}
    }
    function binIcon() {
      if (!icons || EMBED || !document.getElementById("bin-panel") || icons.querySelector(".d95-icon-bin")) return;
      var b = document.createElement("button");
      b.type = "button"; b.className = "d95-icon d95-icon-bin"; b.setAttribute("data-key", "w:bin");
      var pic = document.createElement("i"); pic.setAttribute("aria-hidden", "true");
      var name = document.createElement("span"); name.textContent = T("skin.bin.name");
      b.appendChild(pic); b.appendChild(name);
      function full() { return binItems().length > 0; }
      function draw() { b.classList.toggle("d95-bin-full", full()); b.setAttribute("aria-label", T(full() ? "skin.bin.label_full" : "skin.bin.label_empty")); }
      draw();
      window.addEventListener("byopop-bin", draw);
      window.addEventListener("storage", function (ev) { if (!ev.key || ev.key === BIN_KEY) draw(); });
      b.addEventListener("click", function () { openBin(null); });
      function menu(x, y) {
        popMenu(b, [{ label: T("skin.bin.open"), bold: true, fn: function () { openBin(null); } }, { rule: true },
          { label: T("skin.bin.empty"), disabled: !full(), fn: emptyBin }], x, y, T("skin.bin.name"));
      }
      b.addEventListener("contextmenu", function (ev) { ev.preventDefault(); menu(ev.clientX, ev.clientY); });
      b.addEventListener("keydown", function (ev) {
        if (ev.key === "ContextMenu" || (ev.key === "F10" && ev.shiftKey)) { ev.preventDefault(); var r = b.getBoundingClientRect(); menu(r.right - 8, r.top + 8); }
      });
      var CAP = "application/x-byopop-game";
      function isCap(ev) { return !!ev.dataTransfer && Array.prototype.indexOf.call(ev.dataTransfer.types || [], CAP) >= 0; }
      b.addEventListener("dragenter", function (ev) { if (isCap(ev)) { ev.preventDefault(); b.classList.add("d95-icon-sel"); } });
      b.addEventListener("dragover", function (ev) { if (isCap(ev)) { ev.preventDefault(); ev.dataTransfer.dropEffect = "move"; } });
      b.addEventListener("dragleave", function () { b.classList.remove("d95-icon-sel"); });
      b.addEventListener("drop", function (ev) {
        b.classList.remove("d95-icon-sel");
        if (!isCap(ev)) return;
        ev.preventDefault();
        openBin(ev.dataTransfer.getData(CAP));
      });
      icons.insertBefore(b, icons.querySelector(".d95-fav"));
      layoutIcons();
      startSound();
    }
    // the desktop starting: its sound at the first press or key of the browser's session on the listing (a page may not play a
    // sound before the player does something), and again when the last screen of Shut Down goes (binIcon, once the page is there)
    function startSound() {
      var started = false;
      try { started = sessionStorage.getItem("byopop-started") === "1"; } catch (e) {}
      if (!started) {
        var startUp = function () {
          document.removeEventListener("pointerdown", startUp, true);
          document.removeEventListener("keydown", startUp, true);
          try { sessionStorage.setItem("byopop-started", "1"); } catch (e) {}
          if (on && !offScreen) deskEvent("start");
        };
        document.addEventListener("pointerdown", startUp, true);
        document.addEventListener("keydown", startUp, true);
      }
    }
    function selected() { return iconList().filter(function (b) { return b.classList.contains("d95-icon-sel"); }); }
    function select(list) { iconList().forEach(function (b) { b.classList.toggle("d95-icon-sel", list.indexOf(b) >= 0); }); }
    // the empty desktop: nothing of a window, the task bar, a menu or an icon under the pointer
    function emptyDesk(t) {
      if (!on || !t || !t.closest) return false;
      if (t.closest(".d95-icon, .d95-taskbar, .d95-menu, .d95-ctx, .d95-switch, .d95-off, .d95-vol, button, a, input, select, textarea, label, iframe, dialog, [role=dialog]")) return false;
      return !wins.some(function (w) { return !w.closed && w.el.contains(t); });
    }
    var iconDrag = null, band = null;
    // the Recycle Bin under the pointer while desktop shortcuts are dragged (the user, 2026-11-16: "Allow dragging desktop
    // shortcuts into the recycle bin to delete the shortcut (without forgetting the files)"): it lights as for a cap, and a drop
    // there takes the shortcuts out of the favourites (pointerEnd), never reaching the game's files or the listing
    function binUnder(ev, d) {
      var bin = icons && icons.querySelector(".d95-icon-bin");
      if (!bin || bin.hidden || d.list.indexOf(bin) >= 0 || !d.list.some(function (x) { return x.classList.contains("d95-fav"); })) return null;
      var r = bin.getBoundingClientRect();
      return ev.clientX >= r.left && ev.clientX < r.right && ev.clientY >= r.top && ev.clientY < r.bottom ? bin : null;
    }
    document.addEventListener("pointerdown", function (ev) {
      if (!on || EMBED || !icons || ev.button !== 0) return;
      var b = ev.target.closest && ev.target.closest(".d95-icons > .d95-icon");
      if (b) {
        if (ev.ctrlKey) b.classList.toggle("d95-icon-sel");
        else if (!b.classList.contains("d95-icon-sel")) select([b]);
        iconDrag = { id: ev.pointerId, px: ev.clientX, py: ev.clientY, moved: false, list: selected(), from: null };
        return;
      }
      if (!emptyDesk(ev.target)) return;
      if (!ev.ctrlKey) select([]);
      band = { id: ev.pointerId, px: ev.clientX, py: ev.clientY, el: null, had: selected() };
    }, true);
    window.addEventListener("pointermove", function (ev) {
      if (iconDrag && ev.pointerId === iconDrag.id) {
        var dx = unzoom(ev.clientX - iconDrag.px), dy = unzoom(ev.clientY - iconDrag.py);
        if (!iconDrag.moved && Math.abs(dx) + Math.abs(dy) < 5) return;
        if (!iconDrag.moved) {
          iconDrag.moved = true;
          // the first move of any icon: every icon keeps the place it has now
          if (!iconPlaces()) { keepIconPlaces(iconsNow()); layoutIcons(); }
          iconDrag.from = iconDrag.list.map(function (x) { return [x, x.offsetLeft, x.offsetTop]; });
          document.documentElement.classList.add("d95-dragging");
        }
        iconDrag.from.forEach(function (f) { f[0].style.left = Math.max(0, f[1] + dx) + "px"; f[0].style.top = Math.max(0, f[2] + dy) + "px"; });
        var lit = binUnder(ev, iconDrag);
        if (iconDrag.lit && iconDrag.lit !== lit) iconDrag.lit.classList.remove("d95-icon-sel");
        if (lit) lit.classList.add("d95-icon-sel");
        iconDrag.lit = lit;
        return;
      }
      if (band && ev.pointerId === band.id) {
        var x0 = Math.min(band.px, ev.clientX), y0 = Math.min(band.py, ev.clientY), x1 = Math.max(band.px, ev.clientX), y1 = Math.max(band.py, ev.clientY);
        if (!band.el) {
          if (x1 - x0 + y1 - y0 < 5) return;
          band.el = document.createElement("div"); band.el.className = "d95-lasso"; band.el.setAttribute("aria-hidden", "true");
          document.body.appendChild(band.el);
          document.documentElement.classList.add("d95-dragging");
        }
        band.el.style.left = unzoom(x0) + "px"; band.el.style.top = unzoom(y0) + "px";
        band.el.style.width = unzoom(x1 - x0) + "px"; band.el.style.height = unzoom(y1 - y0) + "px";
        var hit = iconList().filter(function (b) {
          var r = b.getBoundingClientRect();
          return r.right > x0 && r.left < x1 && r.bottom > y0 && r.top < y1;
        });
        select(ev.ctrlKey ? band.had.concat(hit) : hit);
      }
    });
    function pointerEnd(ev) {
      if (iconDrag && ev.pointerId === iconDrag.id) {
        var d = iconDrag;
        iconDrag = null;
        document.documentElement.classList.remove("d95-dragging");
        if (!d.moved) return;
        if (d.lit) d.lit.classList.remove("d95-icon-sel");
        var p = iconPlaces() || {}, bin = binUnder(ev, d);
        if (bin) {
          // into the Recycle Bin: each shortcut goes as its menu's Remove from favourites takes it, with no question and no sound,
          // as Windows 95's drag to the bin; any other icon dragged with them goes back where it was
          var gone = [];
          d.from.forEach(function (f) {
            var k = f[0].getAttribute("data-key");
            if (f[0].classList.contains("d95-fav")) { gone.push(k.substring(2)); delete p[k]; }
            else { f[0].style.left = f[1] + "px"; f[0].style.top = f[2] + "px"; }
          });
          keepIconPlaces(p);
          var favs = [];
          try { favs = ByopopSettings.favourites(); } catch (e) {}
          favs.forEach(function (g) { if (gone.indexOf(g.id) >= 0) { try { ByopopSettings.setFavourite(g, false); } catch (e) {} } });
          bin.focus();
        } else {
          d.from.forEach(function (f) { p[f[0].getAttribute("data-key")] = [f[0].offsetLeft, f[0].offsetTop]; });
          keepIconPlaces(p);
        }
        // the drag is not also the icon's click (nor, dropped on the bin, the bin's)
        var eat = function (e) { e.stopImmediatePropagation(); e.preventDefault(); document.removeEventListener("click", eat, true); };
        document.addEventListener("click", eat, true);
        setTimeout(function () { document.removeEventListener("click", eat, true); }, 0);
      }
      if (band && ev.pointerId === band.id) {
        if (band.el && band.el.parentNode) band.el.parentNode.removeChild(band.el);
        band = null;
        document.documentElement.classList.remove("d95-dragging");
      }
    }
    window.addEventListener("pointerup", pointerEnd);
    window.addEventListener("pointercancel", pointerEnd);
    window.addEventListener("resize", function () { if (on) layoutIcons(); });
    // the empty desktop's menu: Arrange Icons, Line up Icons, and Properties, which opens Settings' Appearance
    document.addEventListener("contextmenu", function (ev) {
      if (EMBED || !emptyDesk(ev.target)) return;
      ev.preventDefault();
      var look = find("set-look") || find("tab-settings") || find("panel-settings");
      popMenu(document.body, [
        { label: T("skin.deskmenu.arrange"), fn: arrangeIcons },
        { label: T("skin.deskmenu.lineup"), disabled: !iconPlaces(), fn: lineUpIcons },
        { rule: true },
        { label: T("skin.deskmenu.props"), disabled: !look, fn: function () { if (look.min) setMin(look, false); show(look); } }
      ], ev.clientX, ev.clientY, T("skin.deskmenu.label"));
    });
    // ---- the task bar's menu: the open windows cascaded or tiled, or all minimized (and back) ----
    function openWins() {
      return wins.filter(function (w) { return present(w) && !w.min && !maxed(w); })
        .sort(function (a, b) { return (parseInt(a.el.style.zIndex, 10) || 0) - (parseInt(b.el.style.zIndex, 10) || 0); });
    }
    function putAt(w, x, y, width, height) {
      if (!w.sizes && !w.fixed && width) {
        var b = bounds(w, width);
        sizeTo(w, b.w, Math.max(b.minH, Math.min(b.maxH, height)));
      }
      w.x = x; w.y = y; w.placed = true;
      place(w, true); commit(w);
    }
    function cascade() {
      var list = openWins(), r = room();
      if (list.length) keepLayout("cascade");
      list.forEach(function (w, i) {
        var k = i % 10, x = 8 + k * 24, y = 8 + k * 24;
        putAt(w, x, y, Math.min(w.el.offsetWidth, r.w - x - 8), Math.min(w.el.offsetHeight, r.h - y - 8));
        raise(w);
      });
      save();
    }
    function tile(across) {
      var list = openWins(), r = room(), n = list.length;
      if (!n) return;
      keepLayout("tile");
      list.forEach(function (w, i) {
        if (across) putAt(w, Math.floor(i * r.w / n), 0, Math.floor(r.w / n), r.h);
        else putAt(w, 0, Math.floor(i * r.h / n), r.w, Math.floor(r.h / n));
      });
      save();
    }
    function minimizeAll() {
      keepLayout("minall");
      wins.filter(function (w) { return present(w) && !w.min; }).forEach(function (w) { setMin(w, true); });
    }
    // Undo (the user, 2026-11-18: "I accidentally horizontally tiled my windows, losing my layout. It is frustrating to put it
    // back."): Cascade, either Tile and Minimize All first keep every window's place, size, state (minimized or not; a
    // maximized window is neither cascaded nor tiled, so it stays so) and order (keepLayout), and the task bar's menu then offers
    // the matching Undo Cascade, Undo Tile or Undo Minimize All, which puts them all back as they were (undoLayout). One undo is
    // kept, the last arrangement's, until it is used. Undo Minimize All is offered while a window it minimized still is, and
    // the others while a window they kept is still open; a window closed since is left closed.
    var undo = null;
    function keepLayout(kind) {
      undo = { kind: kind, active: active, list: wins.filter(present).map(function (w) {
        return { w: w, x: w.x, y: w.y, width: w.w || 0, height: w.h || 0, placed: w.placed, min: !!w.min, z: parseInt(w.el.style.zIndex, 10) || 0 };
      }) };
    }
    function undoable() {
      if (!undo) return "";
      var some = undo.list.some(function (k) { return present(k.w) && (undo.kind !== "minall" || (k.w.min && !k.min)); });
      return some ? undo.kind : "";
    }
    function undoLayout() {
      var u = undo;
      undo = null;
      if (!u) return;
      var back = u.list.filter(function (k) { return present(k.w); });
      back.forEach(function (k) {
        var w = k.w;
        // its own size, or none (its style sheet's) if it had none; a dialog and the game's window have none to put back
        if (!w.sizes && !w.fixed) {
          if (k.width && k.height) sizeTo(w, k.width, k.height);
          else { w.w = 0; w.h = 0; w.el.style.removeProperty("width"); w.el.style.removeProperty("height"); }
        }
        w.x = k.x; w.y = k.y; w.placed = k.placed;
        if (k.min !== !!w.min) setMin(w, k.min);
        place(w, true);
      });
      // and in their order, the one in front last
      back.slice().sort(function (a, b) { return a.z - b.z; }).forEach(function (k) { if (!k.min) raise(k.w); });
      if (u.active && present(u.active) && !u.active.min) activate(u.active);
      save();
    }
    function barMenu(x, y) {
      var can = undoable();
      var some = openWins().length > 0;
      var entries = [
        { label: T("skin.bar.cascade"), disabled: !some, fn: cascade },
        { label: T("skin.bar.tileh"), disabled: !some, fn: function () { tile(false); } },
        { label: T("skin.bar.tilev"), disabled: !some, fn: function () { tile(true); } },
        { rule: true }];
      if (can === "cascade" || can === "tile") entries.push({ label: T(can === "tile" ? "skin.bar.undotile" : "skin.bar.undocascade"), fn: undoLayout });
      entries.push(can === "minall" ? { label: T("skin.bar.undominall"), fn: undoLayout } : { label: T("skin.bar.minall"), fn: minimizeAll });
      popMenu(taskbar ? taskbar.el : document.body, entries, x, y, T("skin.taskbar"));
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
    // ---- a Windows 95 message box (a game past the memory budget, an About box, a question) ----
    // title, text (a line of text, or nodes: paragraphs and a list), kind (true for an About box, "info" for the information
    // sign, "question" for the question mark, else the warning sign) and buttons ({label, fn}, the last one focused and taken
    // by Escape; OK alone when none). The one page's programs (web/deskapps.js) and Uninstall (below) share it.
    function messageBox(title, text, kind, buttons) {
      var d = document.createElement("dialog");
      d.className = "d95-msgbox";
      d.setAttribute("aria-labelledby", "d95-msgbox-title"); d.setAttribute("aria-describedby", "d95-msgbox-text");
      var tb = document.createElement("div"); tb.className = "d95-tb d95-msgbox-tb";
      var tt = document.createElement("span"); tt.className = "d95-tb-text"; tt.id = "d95-msgbox-title"; tt.textContent = title;
      tb.appendChild(tt);
      var body = document.createElement("div"); body.className = "d95-msgbox-body";
      var icon = document.createElement("i"); icon.className = "d95-msgbox-icon"; icon.setAttribute("aria-hidden", "true");
      var p;
      if (typeof text === "string") { p = document.createElement("p"); p.textContent = text; }
      else { p = document.createElement("div"); p.className = "d95-msgbox-text"; p.appendChild(text); d.classList.add("d95-msgbox-long"); }
      p.id = "d95-msgbox-text";
      // an About box: the game's lines as they are, without a sign
      if (kind === true) { p.style.whiteSpace = "pre-line"; d.classList.add("d95-msgbox-about"); }
      else {
        if (kind === "info") d.classList.add("d95-msgbox-info");
        else if (kind === "question") d.classList.add("d95-msgbox-question");
        body.appendChild(icon);
      }
      body.appendChild(p);
      var row = document.createElement("div"); row.className = "d95-msgbox-row";
      var last = null;
      // (a game's page that finishes loading behind the box focuses its own card, which takes the focus into its frame: it
      // comes back to the box)
      function keep() { setTimeout(function () { if (d.open && document.activeElement && document.activeElement.tagName === "IFRAME") last.el.focus(); }, 0); }
      window.addEventListener("blur", keep);
      function done(fn) {
        window.removeEventListener("blur", keep);
        try { d.close(); } catch (e) {}
        if (d.parentNode) d.parentNode.removeChild(d);
        if (fn) fn();
      }
      (buttons || [{ label: T("skin.apps.ok") }]).forEach(function (b) {
        var btn = document.createElement("button"); btn.type = "button"; btn.textContent = b.label;
        btn.addEventListener("click", function () { done(b.fn); });
        row.appendChild(btn);
        last = { el: btn, fn: b.fn };
      });
      // the default, with Windows 95's black frame while no other button has the focus (skins.css)
      last.el.classList.add("d95-def");
      d.appendChild(tb); d.appendChild(body); d.appendChild(row);
      document.body.appendChild(d);
      d.addEventListener("keydown", function (ev) { if (ev.key === "Escape") { ev.preventDefault(); ev.stopPropagation(); done(last.fn); } });
      d.addEventListener("cancel", function (ev) { ev.preventDefault(); done(last.fn); });
      try { d.showModal(); } catch (e) { d.setAttribute("open", ""); }
      deskEvent("ding");
      last.el.focus();
    }
    // ---- Uninstall (the coordinator's list, 2026-11-22: "an uninstall option with confirmation in the Library and shortcut
    // context menus, in addition to the recycle bin") ----
    // Whether a game's files are here: the listing's files check (its card's data-files), else the record a game's page keeps
    function installed(id) {
      var a = document.querySelector('a.tile[data-game="' + String(id).replace(/[^A-Za-z0-9_-]/g, "") + '"]'), li = a && a.parentNode;
      if (li && li.hasAttribute("data-files")) return li.getAttribute("data-files") === "yes";
      try { var r = ByopopGameFiles.all()[id]; return !!(r && ByopopGameFiles.present(r)); } catch (e) { return false; }
    }
    // The standard question in the message box, No the default; on Yes what the recycle bin does with a game (web/addgames.js,
    // uninstall: its files forgotten, its saves, mods and replays kept, its name in the bin). Only the listing reads the games'
    // import rules, so a game's own page asks, then hands the game to the listing (sessionStorage "byopop-uninstall",
    // web/index.html), which uninstalls it as it opens. from: what has the focus back afterwards (a shortcut drawn again since
    // is found by its key), or a function that gives it back (the Library's).
    function uninstall(id, title, from) {
      var key = from && from.getAttribute ? from.getAttribute("data-key") : null;
      function back() {
        if (typeof from === "function") { try { from(); } catch (e) {} return; }
        var to = from && document.body.contains(from) ? from : key && icons ? icons.querySelector('.d95-icon[data-key="' + key.replace(/[^A-Za-z0-9_:-]/g, "") + '"]') : null;
        if (to) { try { to.focus(); } catch (e) {} }
      }
      messageBox(T("skin.uninstall.title"), Tv("skin.uninstall.question", { game: title }), "question", [
        { label: T("skin.uninstall.yes"), fn: function () {
          if (!window.ByopopAddGames || !ByopopAddGames.uninstall) {
            try { sessionStorage.setItem("byopop-uninstall", id); } catch (e) { return; }
            var q = testQuery();
            location.href = "./" + (q ? "?" + q : "") + "#library";
            return;
          }
          ByopopAddGames.uninstall(id).then(back, function (e) {
            messageBox(title, (e && e.message) || T("add.bin.failed"), false, [{ label: T("skin.apps.ok"), fn: back }]);
          });
        } },
        { label: T("skin.uninstall.no"), fn: back }]);
    }
    function openFavMenu(from, g, x, y) {
      popMenu(from, [{ label: T2("skin.fav.open"), bold: true, disabled: !g.page, fn: function () { openPage(g.page); } },
                     { rule: true },
                     { label: T2("skin.uninstall.entry"), disabled: !installed(g.id), fn: function () { uninstall(g.id, g.title, from); } },
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
      deskEvent("menu");
      var m = document.createElement("menu");
      m.className = "d95-menu d95-ctx"; m.setAttribute("role", "menu"); m.setAttribute("aria-label", label || "");
      entries.forEach(function (en) {
        if (en.rule) { menuEntry(m, menuRule()); return; }
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
      // x and y are the screen's (a pointer's, a getBoundingClientRect's)
      var vw = viewW(), vh = room().h;
      x = unzoom(x); y = unzoom(y);
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
    // Where the windows are on the desktop: in the listing's main, which the desktop shows as nothing but the windows in it
    // (web/skins.css), so that they are the page's main content (Lighthouse, 2026-11-23: the live page had no main landmark,
    // its main hidden). A window that is a landmark of its own stays beside the main (System Information's footer, the
    // chat's aside); a game's page has no listing, and its windows stay in its body.
    function mainOf(w) { return LANDMARKS[w.el.tagName] ? null : document.querySelector("body > main.listing"); }
    // a lifted window: onto the desktop (into the main, else the body), or back where it was; any other window into the main
    // and back to where its page put it (w.nest marks the place)
    function lift(w, up) {
      if (!w.home) { nest(w, up); return; }
      var to = mainOf(w) || document.body;
      if (up) { if (w.el.parentNode !== to) to.appendChild(w.el); }
      else if (w.home.parentNode && w.home.nextSibling !== w.el) w.home.parentNode.insertBefore(w.el, w.home.nextSibling);
    }
    function nest(w, up) {
      var m = mainOf(w);
      if (!m || !w.el.parentNode) return;
      if (up) {
        if (w.el.parentNode === m) return;
        if (!w.nest) { w.nest = document.createComment("desktop window " + w.id + " (in the main on the desktop)"); w.el.parentNode.insertBefore(w.nest, w.el); }
        m.appendChild(w.el);
      } else if (w.nest && w.nest.parentNode && w.nest.nextSibling !== w.el) w.nest.parentNode.insertBefore(w.el, w.nest.nextSibling);
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
    var active = null, used = [];
    function activate(w) {
      if (active === w) return;
      // another window to the front: the open menus close, as Windows' did (the task bar's or the desktop's focus is no window,
      // and leaves the Pop menu open)
      if (w) closeAllMenus();
      if (active) active.el.classList.remove("d95-active");
      active = w;
      if (w) { var at = used.indexOf(w); if (at >= 0) used.splice(at, 1); used.unshift(w); }
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
    // ---- Alt+` (the user, 2026-11-13: "Add Alt+Backtick as a site shortcut that works like Alt-Tab in Windows"), since the
    // browser keeps Alt+Tab. Held down, each ` steps through the open windows, the one used last first, in a box in the middle
    // of the desktop (Shift+` steps back, Escape lets go of it); letting go of Alt brings the chosen one to the front, restored
    // if it was minimized, and gives it the focus. A quick Alt+` so goes back to the window used before. In a window's page on
    // the listing's desktop the keys go to that desktop (ByopopApps.altTab), as F6 does, so the switch is between its windows.
    var sw = null;
    function switchList() {
      var l = used.filter(function (w) { return present(w); });
      wins.forEach(function (w) { if (present(w) && l.indexOf(w) < 0) l.push(w); });
      return l;
    }
    function drawSwitch() {
      if (!sw.box) {
        sw.box = document.createElement("div");
        sw.box.className = "d95-switch";
        sw.icons = document.createElement("div"); sw.icons.className = "d95-switch-icons";
        sw.name = document.createElement("div"); sw.name.className = "d95-switch-name"; sw.name.setAttribute("aria-live", "polite");
        sw.box.appendChild(sw.icons); sw.box.appendChild(sw.name);
        document.body.appendChild(sw.box);
      }
      sw.icons.textContent = "";
      sw.list.forEach(function (w, i) {
        // the window's icon on the desktop, drawn by the same rules (d95-icon-<id>); else its app's (a window with no desktop
        // icon: Setup, a Settings section), else a plain window
        var c = document.createElement("span"), di = icons && icons.querySelector(".d95-icon-" + w.id + " > i");
        var icon = di ? null : appIcon(w.id), pic = di ? di.cloneNode(true) : document.createElement("i");
        if (icon) pic.innerHTML = icon.big;
        c.className = "d95-icon d95-icon-" + w.id + ((di ? di.parentNode.classList.contains("d95-icon-app") : icon) ? " d95-icon-app" : "") +
          " d95-switch-icon" + (i === sw.at ? " d95-switch-on" : "");
        c.appendChild(pic);
        sw.icons.appendChild(c);
      });
      var t = sw.list[sw.at];
      sw.name.textContent = t.el.getAttribute("data-d95-title") || t.id;
    }
    function altTab(how) {
      if (!on) return;
      if (how === "cancel" || how === "commit") {
        if (!sw) return;
        var cur = sw;
        sw = null;
        if (cur.box && cur.box.parentNode) cur.box.parentNode.removeChild(cur.box);
        if (how === "commit") { var w = cur.list[cur.at]; if (w.min) setMin(w, false); else raise(w); focusPane(w.el, false); }
        return;
      }
      var back = how === "back";
      if (!sw) {
        var l = switchList();
        if (!l.length) return;
        closeAllMenus();
        sw = { list: l, at: back ? l.length - 1 : (l[0] === active && l.length > 1 ? 1 : 0) };
      } else sw.at = (sw.at + (back ? sw.list.length - 1 : 1)) % sw.list.length;
      drawSwitch();
    }
    var forwarded = false;   // a page in a window: a switch is on in the desktop around it
    function switching() { return !!sw || forwarded; }
    function sendSwitch(how) {
      var P = parentApps();
      if (P && P.altTab) { forwarded = how === "next" || how === "back"; try { P.altTab(window, how); } catch (e) {} return; }
      altTab(how);
    }
    document.addEventListener("keydown", function (ev) {
      if (ev.code === "Backquote" && ev.altKey && !ev.ctrlKey && !ev.metaKey) {
        if (!on && !parentApps()) return;
        ev.preventDefault(); ev.stopImmediatePropagation();
        sendSwitch(ev.shiftKey ? "back" : "next");
      } else if (ev.key === "Escape" && switching()) {
        ev.preventDefault(); ev.stopImmediatePropagation();
        sendSwitch("cancel");
      }
    }, true);
    document.addEventListener("keyup", function (ev) {
      if (ev.key !== "Alt" || !switching()) return;
      ev.preventDefault(); ev.stopImmediatePropagation();
      sendSwitch("commit");
    }, true);
    window.addEventListener("blur", function () { if (sw) altTab("cancel"); forwarded = false; });
    // ---- tooltips drawn as 1995's (the user, 2026-11-14, by the coordinator's list): a pale yellow box with a black border under
    // the pointer, half a second after it rests on something with a title, in place of the browser's own. The browser draws its
    // own for any title under the pointer, so while the pointer is on the element its title is held off it, and the titles of its
    // ancestors too, until the pointer leaves it; a press, a key or the box's six seconds only take the box away. An element named
    // only by its title keeps that name. A title the page gives a held element meanwhile (Music's Play becoming Pause) is held
    // too and shown, and one given to an element under the pointer starts its box. Over a game's window (a frame) there is none:
    // the pointer's movements there never reach this page (the user, 2026-11-16: "Native tooltips are getting mixed in with the
    // Desktop 95 ones" and "Sometimes seeing an errant tooltip over the game window where my mouse is").
    var tip = null, hovered = null, px = 0, py = 0;
    function tipHide() {
      if (!tip) return;
      clearTimeout(tip.timer);
      if (tip.box && tip.box.parentNode) tip.box.parentNode.removeChild(tip.box);
      tip.box = null;
    }
    function tipEnd() {
      if (!tip) return;
      tipHide();
      var was = tip;
      tip = null;
      was.held.forEach(function (h) {
        if (h.el.getAttribute("title") === null) h.el.setAttribute("title", h.text);
        if (h.named) h.el.removeAttribute("aria-label");
      });
    }
    // the element and its titled ancestors, their titles taken off: [{el, text, named}], the element's first
    function holdTitles(el) {
      var held = [];
      for (var e = el; e && e.nodeType === 1; e = e.parentElement) {
        var text = e.getAttribute("title");
        if (text === null) continue;
        var named = !!text && !e.hasAttribute("aria-label") && !e.hasAttribute("aria-labelledby") && !e.textContent.trim();
        if (named) e.setAttribute("aria-label", text);
        e.removeAttribute("title");
        held.push({ el: e, text: text, named: named });
      }
      return held;
    }
    function tipBox() {
      if (!tip || !tip.text) return;
      var b = document.createElement("div");
      b.className = "d95-tip"; b.setAttribute("aria-hidden", "true"); b.textContent = tip.text;
      document.body.appendChild(b);
      var x = unzoom(tip.x), y = unzoom(tip.y) + 20;
      b.style.left = Math.max(0, Math.min(x, viewW() - b.offsetWidth - 2)) + "px";
      b.style.top = (y + b.offsetHeight > room().h ? unzoom(tip.y) - b.offsetHeight - 4 : y) + "px";
      tip.box = b;
      tip.timer = setTimeout(tipHide, 6000);
    }
    function tipFor(target, x, y) {
      var el = target && target.closest && target.closest("[title]");
      // still on the element whose title is held: a child finds no title of it (or the title of a child inside it, which takes over)
      if (tip && tip.el.contains(target) && (!el || el === tip.el || !tip.el.contains(el))) return;
      tipEnd();
      if (!el || target.tagName === "IFRAME" || !el.getAttribute("title") || el.closest(".d95-menu, .d95-ctx")) return;
      var held = holdTitles(el);
      tip = { el: el, held: held, text: held[0].text, x: x, y: y, box: null };
      tip.timer = setTimeout(tipBox, 500);
    }
    document.addEventListener("pointerover", function (ev) {
      hovered = ev.target; px = ev.clientX; py = ev.clientY;
      if (!on || ev.pointerType === "touch") { tipEnd(); return; }
      tipFor(ev.target, ev.clientX, ev.clientY);
    });
    document.addEventListener("pointermove", function (ev) { px = ev.clientX; py = ev.clientY; if (tip && !tip.box) { tip.x = px; tip.y = py; } }, true);
    document.addEventListener("pointerout", function (ev) {
      if (!ev.relatedTarget) hovered = null;
      if (tip && (!ev.relatedTarget || !tip.el.contains(ev.relatedTarget))) tipEnd();
    });
    if (typeof MutationObserver !== "undefined") new MutationObserver(function (recs) {
      recs.forEach(function (r) {
        var t = r.target.getAttribute("title");
        if (t === null) return;
        var h = tip && tip.held.filter(function (x) { return x.el === r.target; })[0];
        if (h) {
          h.text = t;
          r.target.removeAttribute("title");
          if (h.named) r.target.setAttribute("aria-label", t);
          if (h === tip.held[0]) { tip.text = t; if (tip.box) { if (t) tip.box.textContent = t; else tipHide(); } }
        } else if (tip && r.target.contains(tip.el)) {
          // an ancestor of the element given a title: held as well
          r.target.removeAttribute("title");
          tip.held.push({ el: r.target, text: t, named: false });
        } else if (on && hovered && r.target.contains(hovered) && hovered.isConnected) tipFor(hovered, px, py);
      });
    }).observe(root, { subtree: true, attributes: true, attributeFilter: ["title"] });
    document.addEventListener("pointerdown", tipHide, true);
    document.addEventListener("keydown", tipHide, true);
    window.addEventListener("blur", tipEnd);
    // ---- Alt+4 closes the window in front (the user, 2026-11-14, by the coordinator's list), standing in for the Alt+F4 the
    // browser keeps; with no window open it opens the Pop menu on Shut Down. In a window's page on the listing's desktop it goes to
    // that desktop (ByopopApps.closeFront), as Alt+` does, never to the game. AltGr+4 (Ctrl and Alt together on Windows, where it
    // types a currency sign on some layouts) and a key typed into a text field (Option+4 types a cent sign on a Mac) are left
    // alone.
    function closeFront() {
      if (!on) return;
      closeAllMenus();
      var w = active && present(active) && !active.min ? active : frontmost();
      if (w) closeWin(w);
      else if (taskbar) taskbar.openShutDown();
    }
    function typing(el) {
      if (!el || !el.tagName) return false;
      if (el.isContentEditable || el.tagName === "TEXTAREA") return true;
      return el.tagName === "INPUT" && !/^(button|checkbox|radio|range|color|file|image|reset|submit)$/i.test(el.type || "");
    }
    document.addEventListener("keydown", function (ev) {
      if (ev.code !== "Digit4" || !ev.altKey || ev.ctrlKey || ev.metaKey || ev.shiftKey || ev.repeat) return;
      if ((ev.getModifierState && ev.getModifierState("AltGraph")) || typing(ev.target)) return;
      var P = parentApps();
      if (!on && !P) return;
      ev.preventDefault(); ev.stopImmediatePropagation();
      if (P && P.closeFront) { try { P.closeFront(window); } catch (e) {} return; }
      closeFront();
    }, true);
    // ---- Shut Down: every window closes as its Close closes it (a game's with its saves), and the last screen covers the page,
    // wordless and black, until a key or a click brings the desktop back with its windows closed. On a game's own page the game
    // closes and the listing shows the last screen (web/shell.html, closeProgram); in a window's page the desktop around it
    // shuts down.
    function shutDown() {
      var P = parentApps();
      if (P && P.shutDown) { try { P.shutDown(window); } catch (e) {} return; }
      closeAllMenus();
      deskEvent("exit");
      var game = find("game");
      wins.slice().forEach(function (w) { if (w !== game && present(w)) closeWin(w); });
      if (game && present(game) && game.onClose) { window.byopopShutDown = true; lastScreen(true); closeWin(game); return; }
      lastScreen(false);
    }
    var offScreen = null;
    function lastScreen(stay) {
      if (offScreen) return;
      // a dialog over the whole screen, open from the start, modal to a screen reader as it covers everything
      var s = document.createElement("dialog"); s.open = true;
      s.className = "d95-off"; s.tabIndex = -1;
      s.setAttribute("aria-modal", "true"); s.setAttribute("aria-label", T("skin.pop.shutdown.screen"));
      document.body.appendChild(s);
      offScreen = s;
      try { s.focus(); } catch (e) {}
      if (stay) return;   // the game's page goes on to the listing, which shows its own
      var armed = false;
      setTimeout(function () { armed = true; }, 800);
      function back(ev) {
        if (!armed || ev.key === "Alt" || ev.key === "Shift" || ev.key === "Control" || ev.key === "Meta") return;
        ev.preventDefault(); ev.stopImmediatePropagation();
        document.removeEventListener("keydown", back, true);
        document.removeEventListener("pointerdown", back, true);
        s.parentNode.removeChild(s);
        offScreen = null;
        deskEvent("start");
        activate(frontmost());
        if (taskbar && taskbar.el) { var p = taskbar.el.querySelector(".d95-pop"); if (p) p.focus(); }
      }
      document.addEventListener("keydown", back, true);
      document.addEventListener("pointerdown", back, true);
    }
    document.addEventListener("focusin", function (ev) {
      if (!on) return;
      var pane = paneOf(ev.target), w = pane && winOf(pane);
      if (w) { w.lastFocus = ev.target; activate(w); } else if (pane) { pane.lastFocus = ev.target; activate(null); }
    });
    document.addEventListener("pointerdown", function (ev) {
      if (!on) return;
      // a press in an open menu (a window's menu bar, its control menu, a context menu: popMenu) leaves the window it belongs to
      // active: an entry that is greyed does nothing, and the window kept losing its focus to it (the user, 2026-11-21:
      // "Clicking a greyed out resolution causes the window to lose focus")
      if (ev.target && ev.target.closest && ev.target.closest(".d95-ctx")) return;
      var pane = paneOf(ev.target), w = pane && winOf(pane);
      activate(w || null);
    }, true);
    function setOn(want) {
      if (want === on) { applyZoom(); if (on) { layout(); if (!active) activate(frontmost()); } return; }
      on = want;
      root.classList.toggle("d95-desk", on);
      applyZoom();
      wins.forEach(function (w) { named(w, on); });
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
        binIcon();
        window.addEventListener("byopop-favourites", favIcons);
        // a game uninstalled (its files forgotten, byopop-bin) or its files loaded in another tab: its shortcut's picture
        window.addEventListener("byopop-bin", favIcons);
        window.addEventListener("storage", function (ev) {
          if (!ev.key || (window.ByopopSettings && ev.key === ByopopSettings.KEY) || (window.ByopopGameFiles && ev.key === ByopopGameFiles.KEY)) favIcons();
        });
      }
      // out of the desktop the page lays itself out: the windows' places go, and are put back when it comes again
      wins.forEach(function (w) { lift(w, on); });
      if (!on) wins.forEach(function (w) { ["left", "top", "z-index", "max-width", "max-height", "width", "height"].forEach(function (p) { w.el.style.removeProperty(p); }); });
      else wins.forEach(function (w) { if (w.w && w.h) sizeTo(w, w.w, w.h); });
      try { window.dispatchEvent(new CustomEvent("byopop-desk", { detail: { on: on } })); } catch (e) {}
      // the keys, said once: a line in About, there before the windows are fitted to the room (a few lines added after, About
      // grew past the task bar's top on a small screen)
      var ab = find("about"), keysLine = ab && ab.el.querySelector(".d95-keys");
      if (on && ab && !keysLine) {
        keysLine = document.createElement("p"); keysLine.className = "d95-keys"; keysLine.textContent = T("skin.desk.keys");
        (ab.el.querySelector(".sy-general") || ab.el.querySelector(".d95-body") || ab.el).appendChild(keysLine);
      } else if (!on && keysLine) keysLine.parentNode.removeChild(keysLine);
      if (on) layout();
      layoutIcons();
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
             altTab: altTab, closeFront: closeFront, shutDown: shutDown, lastScreen: lastScreen,
             f6From: function (id, back) {
               var w = find(id), list = panes();
               if (!w || !list.length) return;
               var i = list.indexOf(w.el);
               focusPane(list[(i + (back ? list.length - 1 : 1) + list.length) % list.length], back);
             },
             // a window as the Pop menu opens it; once it is open (a window whose script loads first, o.lazy, opens later), true
             openWin: function (id) {
               var w = find(id);
               if (!w || !on) return Promise.resolve(false);
               // (at once where there is nothing to load: web/deskapps.js marks what it opens while it puts windows back)
               function now() { if (!on) return false; if (w.min) setMin(w, false); if (w.open) { w.open(); raise(w); } else show(w); return true; }
               if (!w.lazy) return Promise.resolve(now());
               return fill(w).then(now, function () { cannotStart(w); return false; });
             },
             // a window's script loaded and the window filled (o.lazy), without opening it: a test's driver before it reads the
             // window's script (tools/web/splitscheck.js)
             ready: function (id) { var w = find(id); return w ? fill(w).then(function () { return true; }, function () { return false; }) : Promise.resolve(false); },
             // a window closed as its Close button closes it (its onClose, or closed and its afterClose)
             close: function (id) { var w = find(id); if (w && !w.closed) closeWin(w); },
             iconPlaceAt: iconPlaceAt, addShortcuts: addShortcuts,
             list: function () { return wins.map(function (w) { return { id: w.id, title: w.el.getAttribute("data-d95-title") || w.id, closed: !!w.closed }; }); },
             event: deskEvent,
             menu: function (from, entries, x, y, label, opts) { popMenu(from, entries, x, y, label, opts); },
             messageBox: messageBox, uninstall: uninstall, installed: installed,
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
               var h = w.el.offsetHeight, most = room().h - drawn(w).y - h;
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
      (EMBED || listing || (document.documentElement.clientWidth || window.innerWidth) / zoomFor() >= DESK_MIN);
  }
  function deskSync() {
    root.classList.toggle("d95-narrow", !EMBED && (document.documentElement.clientWidth || window.innerWidth) / zoomFor() < DESK_MIN);
    if (document.body) desk.setOn(deskWanted());
    applyZoom();
  }

  // ---------- Windows 95 games only ----------
  // Desktop 95's setting "Windows 95 games only" (the user, 2026-11-08: an "immersion" toggle that "will only list the games that
  // actually supported Windows 95"): with it on, and only in Desktop 95, the lists show only the games whose original supported
  // Windows 95 (cmake/ByopopWeb.cmake's BYOPOP_WEB_WIN95, as each tile's data-win95 and the chat's table's win95): the Library
  // (web/library.js, whose status bar says how many it hides), the favourites' icons (html.d95-win95only, web/skins.css), and
  // the Pop menu's Games, which says how many it hides. An unknown game is hidden too. A game page already open stays as it is.
  function win95Of(id) {
    var games = window.ByopopGames || [];
    for (var i = 0; i < games.length; i++) if (games[i].id === id) return games[i].win95 || "unknown";
    var a = document.querySelector('a.tile[data-game="' + String(id).replace(/[^A-Za-z0-9_-]/g, "") + '"]'), li = a && a.closest("li");
    return (li && li.getAttribute("data-win95")) || "unknown";
  }
  function win95On() { return root.getAttribute("data-skin") === "d95" && readItem().win95Only === "on"; }
  function win95Hides(id) { return win95On() && win95Of(id) !== "yes"; }
  function win95Sync() {
    var on = win95On();
    root.classList.toggle("d95-win95only", on);
  }
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && (ev.detail.name === "win95Only")) win95Sync(); });
  // the skin chosen here or on another page of the site (Appearance's Skin list): this page redraws in it
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "look") apply(); });
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "hideCues") cuesSync(); });
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) apply(); });
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) win95Sync(); });
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", win95Sync); else win95Sync();

  window.addEventListener("byopop-skin", deskSync);
  window.addEventListener("resize", deskSync);

  // ---------- the desktop's loading (the user, 2026-11-20: "The site briefly shows the all games window while loading. Replace
  // that load with something 95-themed") ----------
  // A page that becomes a desktop (the listing, a game page of its own) used to be drawn in stages as it loaded: the games'
  // list as a plain page, then the desktop, then each window in its saved place. Now, until the desktop is whole, the page shows
  // only the desktop's colour and the hourglass, with a small box in the middle whose strip fills as the page's scripts and
  // style sheets arrive, as 1995's Setup showed its progress (html.d95-booting, web/skins.css: the root's own pseudo-elements,
  // there before the body is). They cover the page rather than hide it, so that the page's scripts lay it out and focus it as
  // they always did. The desktop shows once the page has loaded with its fonts and the one page's own windows are back with
  // their pages (web/deskapps.js holds "apps" until then, bootHold and bootDone), and BOOT_MAX_MS after the page's text was
  // read at the latest, so that a file that never comes cannot keep the desktop away. A page that is no desktop (a page in a
  // window, the player-only page, the screen saver's, a screen too narrow) never shows it.
  var BOOT_MAX_MS = 15000, BOOT_BLOCKS = 20, BOOT_PITCH = 11, boot = null;
  function bootStart() {
    if (EMBED || window.top !== window || current() !== "d95" || Q.get("player") === "1" || Q.get("saver") === "1") return;
    // (the listing is a desktop at any width; a game page only from DESK_MIN, deskWanted)
    if (root.classList.contains("game-unmade") && (root.clientWidth || window.innerWidth) / zoomFor() < DESK_MIN) return;
    boot = { wait: { load: true }, got: 0, shown: 0, timer: 0 };
    root.classList.add("d95-booting");
    root.setAttribute("data-d95-boot", T("skin.boot"));
    document.addEventListener("load", bootFile, true);
    document.addEventListener("error", bootFile, true);
    document.addEventListener("DOMContentLoaded", function () {
      if (!boot) return;
      if (!deskWanted()) { bootEnd(); return; }
      bootFill();
      boot.timer = setTimeout(bootEnd, BOOT_MAX_MS);
    });
    window.addEventListener("load", function () {
      var fonts = null;
      try { fonts = document.fonts && document.fonts.ready; } catch (e) {}
      // after the load's other listeners and the timers they set (the one page's windows, web/deskapps.js, start)
      Promise.resolve(fonts).then(function () { setTimeout(function () { bootDone("load"); }, 0); }, function () { bootDone("load"); });
    });
  }
  // a script or a style sheet of the page arrived (or failed): the strip fills by the share of those the page has named so far
  function bootFile(ev) {
    var t = ev.target;
    if (!boot || !t || (t.tagName !== "SCRIPT" && t.tagName !== "LINK")) return;
    boot.got++;
    bootFill();
  }
  function bootFill() {
    if (!boot) return;
    var named = document.querySelectorAll("script[src], link[rel=stylesheet][href]").length;
    // while the page is still being read more are to come
    var share = boot.got / Math.max(1, named + (document.readyState === "loading" ? 4 : 0));
    boot.shown = Math.max(boot.shown, Math.min(BOOT_BLOCKS - 1, Math.floor(share * BOOT_BLOCKS)));
    root.style.setProperty("--d95-boot-w", (boot.shown * BOOT_PITCH) + "px");
  }
  // a part the desktop waits for (name), and that part done
  function bootHold(name) { if (boot) boot.wait[name] = true; }
  function bootDone(name) {
    if (!boot) return;
    delete boot.wait[name];
    if (!Object.keys(boot.wait).length) bootEnd();
  }
  function bootEnd() {
    if (!boot) return;
    clearTimeout(boot.timer);
    document.removeEventListener("load", bootFile, true);
    document.removeEventListener("error", bootFile, true);
    boot = null;
    root.classList.remove("d95-booting");
    root.removeAttribute("data-d95-boot");
    root.style.removeProperty("--d95-boot-w");
  }

  apply();
  bootStart();
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
  return { look: look, LOOKS: LOOKS, zoom: zoom, unzoom: unzoom, ratio: ratio, viewW: viewW, viewH: viewH, EMBED: EMBED, SITE: SITE, testQuery: testQuery, current: current, setGame: setGame, apply: apply, wizard: wizard, desk: desk, deskSync: deskSync,
           bootHold: bootHold, bootDone: bootDone, booting: function () { return !!boot; } };
})();
// The volume of everything the site plays (the tray's speaker; the user, 2026-11-14, by the coordinator's list): the games, Music,
// Sounds and a theme's sounds, each at its own level times this one. Kept in localStorage (byopop-master-volume, 0 to 100, and
// byopop-master-mute "1"), so every page and window follows it; on(fn) is told of each change, here or in another page.
var ByopopVolume = (function () {
  "use strict";
  var VOL = "byopop-master-volume", MUTE = "byopop-master-mute", fns = [];
  function get() { try { var v = parseInt(localStorage.getItem(VOL), 10); return v >= 0 && v <= 100 ? v : 100; } catch (e) { return 100; } }
  function muted() { try { return localStorage.getItem(MUTE) === "1"; } catch (e) { return false; } }
  function level() { return muted() ? 0 : get() / 100; }
  function fire() { fns.forEach(function (fn) { try { fn(level()); } catch (e) {} }); }
  function set(v, mute) {
    try {
      if (v !== null && v !== undefined) localStorage.setItem(VOL, String(Math.max(0, Math.min(100, Math.round(v)))));
      if (mute !== undefined) { if (mute) localStorage.setItem(MUTE, "1"); else localStorage.removeItem(MUTE); }
    } catch (e) {}
    fire();
  }
  window.addEventListener("storage", function (ev) { if (!ev.key || ev.key === VOL || ev.key === MUTE) fire(); });
  return { get: get, muted: muted, level: level, set: set, on: function (fn) { fns.push(fn); } };
})();
// The copy of a game's files, drawn as 1995's file-copy dialog on the desktop (the user, 2026-11-14, by the coordinator's list):
// our own drawing of a page flying from one folder to another, the file being copied, its size and a progress bar. Only while the
// desktop is on (in a game page inside the listing, the listing's desktop); else open() gives calls that do nothing, and the page's
// own messages say how the copy goes. open() -> { file(name, bytes), progress(done, total), close() }.
var ByopopCopy = (function () {
  "use strict";
  var NOOP = { file: function () {}, progress: function () {}, close: function () {} };
  function T(k, v) { try { return ByopopStrings.t(k, v); } catch (e) { return k; } }
  function open() {
    try { if (!document.documentElement.classList.contains("d95-desk") && window.parent !== window && window.parent.ByopopCopy) return window.parent.ByopopCopy.open(); } catch (e) {}
    if (!document.documentElement.classList.contains("d95-desk") || !document.body) return NOOP;
    var el = document.createElement("dialog"); el.open = true;
    el.className = "d95-win d95-active d95-copy";
    el.setAttribute("aria-labelledby", "d95-copy-title");
    var tb = document.createElement("div"); tb.className = "d95-tb d95-handle";
    var h = document.createElement("h2"); h.className = "d95-tb-text"; h.id = "d95-copy-title"; h.textContent = T("skin.copy.title");
    tb.appendChild(h); el.appendChild(tb);
    var pic = document.createElement("div"); pic.className = "d95-copy-pic"; pic.setAttribute("aria-hidden", "true");
    pic.innerHTML = "<i class='d95-copy-from'></i><i class='d95-copy-page'></i><i class='d95-copy-to'></i>";
    el.appendChild(pic);
    var name = document.createElement("div"); name.className = "d95-copy-name";
    var size = document.createElement("div"); size.className = "d95-copy-size";
    var bar = document.createElement("div"); bar.className = "d95-copy-bar"; bar.setAttribute("role", "progressbar");
    bar.setAttribute("aria-valuemin", "0"); bar.setAttribute("aria-valuemax", "100"); bar.setAttribute("aria-valuenow", "0");
    bar.setAttribute("aria-label", T("skin.copy.title"));
    var fill = document.createElement("i"); bar.appendChild(fill);
    el.appendChild(name); el.appendChild(size); el.appendChild(bar);
    document.body.appendChild(el);
    var closed = false;
    return {
      file: function (n, bytes) {
        if (closed) return;
        name.textContent = String(n).split("/").pop();
        name.title = String(n);
        size.textContent = bytes >= 0 ? T("skin.copy.size", { kb: Math.max(1, Math.ceil(bytes / 1024)).toLocaleString() }) : "";
      },
      progress: function (done, total) {
        if (closed) return;
        var pct = total ? Math.min(100, Math.round(done * 100 / total)) : 0;
        fill.style.width = pct + "%";
        bar.setAttribute("aria-valuenow", String(pct));
      },
      close: function () { if (closed) return; closed = true; if (el.parentNode) el.parentNode.removeChild(el); }
    };
  }
  return { open: open };
})();
