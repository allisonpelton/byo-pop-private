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

  apply();
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) apply(); });
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "skin") apply(); });
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", askOnce); else askOnce();
  return { SKINS: SKINS, current: current, choose: choose, setGame: setGame, apply: apply };
})();
