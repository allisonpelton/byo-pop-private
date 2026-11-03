// The site's light and dark themes (docs/WEB_PLAYER.md, "Light and dark"). Every page follows the browser's
// prefers-color-scheme until the player presses its theme button (a moon in the light theme, a sun in the dark one), which
// sets <html data-theme="light"> or "dark"; every page's dark rules are written for that ("@media (prefers-color-scheme:
// dark) { :root:not([data-theme="light"]) ... }" and ":root[data-theme="dark"] ..."). The choice is one localStorage item
// for the whole site, "byopop-theme", "light" or "dark", and it is kept only while it differs from the browser's: pressing
// the button back to the browser's theme removes it, so the pages follow the browser again.
//
// This file is loaded in each page's <head>, before its body, so the page is drawn in the chosen theme from the start. It
// fills every button marked data-theme-toggle (its icons, name and state) once the page is read, and follows the choice
// made on another of the site's tabs (the storage event) and the browser's own setting.
var ByopopTheme = (function () {
  "use strict";
  var KEY = "byopop-theme";
  var root = document.documentElement;
  var media = null;
  try { media = window.matchMedia("(prefers-color-scheme: dark)"); } catch (e) {}
  // the choice when storage is blocked: this page only
  var pageChoice = null;

  function stored() {
    try {
      var v = window.localStorage.getItem(KEY);
      return v === "light" || v === "dark" ? v : null;
    } catch (e) { return pageChoice; }
  }
  function browser() { return media && media.matches ? "dark" : "light"; }
  // the theme shown: the player's choice, else the browser's
  function current() { return stored() || browser(); }

  // Our own line drawings: a crescent moon, and a sun of eight rays
  var ICONS =
    '<svg class="theme-moon" viewBox="0 0 24 24" aria-hidden="true" focusable="false"><path d="M20 14.5A8 8 0 1 1 9.5 4a6.5 6.5 0 0 0 10.5 10.5z"/></svg>' +
    '<svg class="theme-sun" viewBox="0 0 24 24" aria-hidden="true" focusable="false"><circle cx="12" cy="12" r="4"/>' +
    '<path d="M12 2.5v2.5M12 19v2.5M2.5 12H5M19 12h2.5M5.3 5.3l1.8 1.8M16.9 16.9l1.8 1.8M5.3 18.7l1.8-1.8M16.9 7.1l1.8-1.8"/></svg>';
  var STYLE =
    ".theme-toggle { display: inline-flex; align-items: center; justify-content: center; }\n" +
    ".theme-toggle svg { width: 20px; height: 20px; fill: none; stroke: currentColor; stroke-width: 1.8; stroke-linecap: round; stroke-linejoin: round; }\n" +
    ".theme-toggle[aria-pressed=true] .theme-moon, .theme-toggle[aria-pressed=false] .theme-sun { display: none; }\n";

  function buttons() { return document.querySelectorAll("[data-theme-toggle]"); }
  // The button is a switch named "Dark theme", pressed in the dark theme; its picture is the theme it changes to
  function show(b) {
    var dark = current() === "dark";
    b.setAttribute("aria-pressed", dark ? "true" : "false");
    b.title = dark ? "Use the light theme" : "Use the dark theme";
  }
  function apply() {
    var v = stored();
    if (v) root.setAttribute("data-theme", v); else root.removeAttribute("data-theme");
    Array.prototype.forEach.call(buttons(), show);
  }
  function toggle() {
    var next = current() === "dark" ? "light" : "dark";
    pageChoice = next === browser() ? null : next;
    try {
      if (pageChoice) window.localStorage.setItem(KEY, pageChoice); else window.localStorage.removeItem(KEY);
    } catch (e) {}
    apply();
  }
  function init() {
    var style = document.createElement("style");
    style.textContent = STYLE;
    document.head.appendChild(style);
    Array.prototype.forEach.call(buttons(), function (b) {
      b.classList.add("theme-toggle");
      b.setAttribute("aria-label", "Dark theme");
      b.innerHTML = ICONS;
      b.addEventListener("click", toggle);
    });
    apply();
  }

  apply();
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init); else init();
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) apply(); });
  if (media) {
    // the browser's setting changed: a choice that now matches it is no longer needed
    var follow = function () {
      if (stored() === browser()) { pageChoice = null; try { window.localStorage.removeItem(KEY); } catch (e) {} }
      apply();
    };
    if (media.addEventListener) media.addEventListener("change", follow); else if (media.addListener) media.addListener(follow);
  }
  return { KEY: KEY, current: current, toggle: toggle };
})();
