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
  var DESK_KEY = "byopop-desk", DESK_MIN = 760;
  function TW(k, title) { try { return ByopopStrings.t(k, { window: title }); } catch (e) { return title; } }
  var desk = (function () {
    var wins = [], top = 20, on = false, icons = null;
    function store() {
      try { var o = JSON.parse(window.localStorage.getItem(DESK_KEY) || "{}"); return o && typeof o === "object" && !Array.isArray(o) ? o : {}; }
      catch (e) { return {}; }
    }
    function saved() { var o = store(); return (pageGame && o[pageGame] && typeof o[pageGame] === "object") ? o[pageGame] : {}; }
    function save() {
      if (!pageGame) return;
      var o = store(), mine = {};
      wins.forEach(function (w) { if (w.placed) mine[w.id] = { x: w.x, y: w.y, closed: !!w.closed }; });
      o[pageGame] = mine;
      try { window.localStorage.setItem(DESK_KEY, JSON.stringify(o)); } catch (e) {}
    }
    function dpr() { return window.devicePixelRatio || 1; }
    function shown(w) { return !w.closed && !w.el.hidden && w.el.getClientRects().length > 0; }
    // keeps a title bar on the screen, and the game's picture on whole device pixels
    function place(w) {
      var vw = document.documentElement.clientWidth || window.innerWidth;
      w.x = Math.max(-(w.el.offsetWidth - 80), Math.min(vw - 80, Math.round(w.x)));
      w.y = Math.max(0, Math.round(w.y));
      w.el.style.left = w.x + "px";
      w.el.style.top = w.y + "px";
      if (w.snap && shown(w)) {
        var r = w.snap.getBoundingClientRect(), d = dpr();
        var dx = (Math.round(r.left * d) - r.left * d) / d, dy = (Math.round(r.top * d) - r.top * d) / d;
        if (dx || dy) { w.el.style.left = (w.x + dx) + "px"; w.el.style.top = (w.y + dy) + "px"; }
      }
    }
    function raise(w) { w.el.style.zIndex = String(++top); }
    function setClosed(w, c) {
      w.closed = !!c;
      w.el.classList.toggle("d95-closed", w.closed);
      if (w.icon) w.icon.setAttribute("aria-pressed", w.closed ? "false" : "true");
      if (!w.closed) { raise(w); place(w); }
      save();
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
    // closable (a close button that hides it; the desktop icon shows it again), open (what the icon does instead: a panel's own
    // button), at: function () { return [x, y]: where it goes first } }
    function add(el, o) {
      if (!el || wins.some(function (w) { return w.el === el; })) return null;
      var w = { el: el, id: o.id, snap: o.snap || null, x: 0, y: 0, placed: false, closed: false, at: o.at, open: o.open, button: o.button };
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
          var x = document.createElement("button");
          x.type = "button"; x.className = "d95-x"; x.setAttribute("aria-label", TW("skin.desk.close", o.title));
          x.addEventListener("click", function () { setClosed(w, true); });
          handle.appendChild(x);
        }
        el.insertBefore(handle, el.firstChild);
      }
      handle.classList.add("d95-handle");
      drag(w, handle);
      el.addEventListener("pointerdown", function () { if (on) raise(w); }, true);
      var s = saved()[o.id];
      if (s && isFinite(s.x) && isFinite(s.y)) { w.x = s.x; w.y = s.y; w.placed = true; w.closed = !!s.closed && !!o.closable; }
      wins.push(w);
      addIcon(w, o.title);
      if (on) layout();
      return w;
    }
    function addIcon(w, title) {
      if (!icons) return;
      var b = document.createElement("button");
      b.type = "button"; b.className = "d95-icon d95-icon-" + w.id;
      var pic = document.createElement("i"); pic.setAttribute("aria-hidden", "true");
      var name = document.createElement("span"); name.textContent = title;
      b.appendChild(pic); b.appendChild(name);
      b.setAttribute("aria-label", TW("skin.desk.open", title));
      b.addEventListener("click", function (ev) {
        if (w.open) { w.open(ev); raise(w); return; }
        setClosed(w, false);
        var f = w.el.querySelector("canvas, [tabindex]:not([tabindex='-1']), button");
        if (f && w.id !== "game") try { f.focus(); } catch (e) {}
      });
      if (!w.open) b.setAttribute("aria-pressed", w.closed ? "false" : "true");
      // a panel's icon is there while its button on the bar is (Mods: once the game's files are here)
      if (w.button && window.MutationObserver) {
        var follow = function () { b.hidden = w.button.hidden; };
        new MutationObserver(follow).observe(w.button, { attributes: true, attributeFilter: ["hidden"] });
        follow();
      }
      w.icon = b;
      icons.appendChild(b);
    }
    // where the windows go the first time: the controls at the top, the game under them, the chat beside it, the panels at the right
    function layout() {
      var vw = document.documentElement.clientWidth || window.innerWidth;
      var left = icons ? icons.offsetWidth + 24 : 16;
      wins.forEach(function (w) {
        if (!w.placed) {
          var p = w.at ? w.at(left, vw) : [left, 16];
          w.x = p[0]; w.y = p[1];
        }
        w.el.classList.toggle("d95-closed", w.closed);
        place(w);
      });
    }
    function find(id) { for (var i = 0; i < wins.length; i++) if (wins[i].id === id) return wins[i]; return null; }
    function setOn(want) {
      if (want === on) { if (on) layout(); return; }
      on = want;
      root.classList.toggle("d95-desk", on);
      if (on && !icons) {
        icons = document.createElement("nav");
        icons.className = "d95-icons";
        icons.setAttribute("aria-label", T("skin.desk.icons"));
        document.body.insertBefore(icons, document.body.firstChild);
        wins.forEach(function (w) { addIcon(w, w.el.getAttribute("data-d95-title") || w.id); });
      }
      // out of the desktop the page lays itself out: the windows' places go, and are put back when it comes again
      if (!on) wins.forEach(function (w) { w.el.style.removeProperty("left"); w.el.style.removeProperty("top"); w.el.style.removeProperty("z-index"); });
      try { window.dispatchEvent(new CustomEvent("byopop-desk", { detail: { on: on } })); } catch (e) {}
      if (on) layout();
    }
    window.addEventListener("resize", function () { if (on) layout(); });
    return { add: add, setOn: setOn, layout: layout, find: find, isOn: function () { return on; },
             raise: function (id) { var w = find(id); if (w && on) raise(w); },
             place: function (id) { var w = find(id); if (w && on) place(w); } };
  })();
  // a game page (one with a workspace) in Desktop 95, wide enough for windows
  function deskWanted() {
    return root.getAttribute("data-skin") === "d95" && !!document.getElementById("workspace") &&
      (document.documentElement.clientWidth || window.innerWidth) >= DESK_MIN;
  }
  function deskSync() { if (document.body) desk.setOn(deskWanted()); }
  window.addEventListener("byopop-skin", deskSync);
  window.addEventListener("resize", deskSync);

  apply();
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) apply(); });
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "skin") apply(); });
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", askOnce); else askOnce();
  return { SKINS: SKINS, current: current, choose: choose, setGame: setGame, apply: apply, wizard: wizard, desk: desk, deskSync: deskSync };
})();
