// Desktop 95's colour schemes (the user, 2026-11-10: "The site might benefit from more color"; 2026-11-11, of the proposal's
// schemes and patterns: "Yes, 1 then 2"; docs/WEB_PLAYER.md, "Colour schemes"). A scheme is a few colours of the desktop, as a
// 1995 appearance scheme was: the desktop, the face of windows and buttons with its shadow and its soft light, the title bar
// (one colour, or two as a gradient: each scheme decides) and the selection. The names and values are byo-pop's own, after
// soda flavours. The setting "scheme" picks one (web/settings.js, Appearance); "standard" is the look the site always had and
// changes nothing. Text sizes never change with a scheme.
//
// web/skins.css draws the windows from variables, which apply() sets on <html>. The small pictures it holds as SVG in data
// addresses (scroll arrows, title bar buttons, check marks) cannot read a variable, so for a scheme other than the standard one
// their rules are written again with the scheme's face, shadow and soft light in place of the standard greys (uriRules).
var ByopopSchemes = (function () {
  "use strict";
  // desk: the desktop; face: windows and buttons; lo: the shadow of a raised edge (and an inactive title bar); mid: the soft light
  // inside it; title: the active title bar, one colour or [left, right]; sel: selected items, links and the menu's highlight
  var LIST = [
    { id: "standard", desk: "#008080", face: "#c0c0c0", lo: "#808080", mid: "#dfdfdf", title: ["#000080", "#1084d0"], sel: "#000080" },
    { id: "cola", desk: "#3b1f14", face: "#c9b8a8", lo: "#85705f", mid: "#e2d6ca", title: "#5a1a12", sel: "#5a1a12" },
    { id: "creamsoda", desk: "#86622a", face: "#e4d7ba", lo: "#958768", mid: "#f1e9d6", title: "#6e4f16", sel: "#6e4f16" },
    { id: "grape", desk: "#3a2456", face: "#c8c0d4", lo: "#7f7691", mid: "#e0dae8", title: ["#3f2170", "#7a4fb4"], sel: "#3f2170" },
    { id: "lemonlime", desk: "#2f6b2a", face: "#c6d0b4", lo: "#7b8666", mid: "#dee5d1", title: "#2a5519", sel: "#2a5519" },
    { id: "cherry", desk: "#7a1626", face: "#d2c2c2", lo: "#8a7676", mid: "#e6dada", title: ["#6e0d1c", "#b23a4c"], sel: "#6e0d1c" },
    { id: "blueraspberry", desk: "#0f4c8a", face: "#bcc8d6", lo: "#717f95", mid: "#d8e0ea", title: ["#0a3470", "#1f7fc0"], sel: "#0a3470" },
    { id: "gingerale", desk: "#857437", face: "#d6d0b0", lo: "#89835e", mid: "#e8e4cc", title: "#554b14", sel: "#554b14" },
    { id: "seltzer", desk: "#55656d", face: "#d4d8da", lo: "#7a8286", mid: "#e8ebec", title: "#263640", sel: "#263640" }
  ];
  var STANDARD = LIST[0];
  function byId(id) { for (var i = 0; i < LIST.length; i++) if (LIST[i].id === id) return LIST[i]; return STANDARD; }
  function ends(s) { return typeof s.title === "string" ? [s.title, s.title] : s.title; }

  // ---- contrast (WCAG): for the test, and nothing else ----
  function lum(hex) {
    var v = [1, 3, 5].map(function (i) { var c = parseInt(hex.substr(i, 2), 16) / 255; return c <= 0.03928 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4); });
    return 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2];
  }
  function contrast(a, b) { var x = lum(a), y = lum(b); return (Math.max(x, y) + 0.05) / (Math.min(x, y) + 0.05); }

  // ---- the page ----
  var KEY = "byopop-settings";
  function chosen() {
    try { var o = JSON.parse(window.localStorage.getItem(KEY) || "{}"); return byId(o && o.scheme).id; } catch (e) { return STANDARD.id; }
  }
  var VARS = ["--bg", "--panel", "--panel-2", "--line", "--accent", "--sk-lo", "--sk-mid", "--sk-title", "--sk-band"];
  // the data-address pictures of web/skins.css, with the scheme's colours for the standard greys
  function uriRules(s) {
    var out = [], swap = [[/(%23|#)c0c0c0/gi, s.face], [/(%23|#)808080/gi, s.lo], [/(%23|#)dfdfdf/gi, s.mid]];
    function walk(rules, wrap) {
      for (var i = 0; i < rules.length; i++) {
        var r = rules[i];
        if (r.cssRules && r.conditionText !== undefined) { walk(r.cssRules, [r.cssText.slice(0, r.cssText.indexOf("{") + 1), "}"]); continue; }
        if (!r.style || !r.selectorText) continue;
        var decl = "";
        for (var k = 0; k < r.style.length; k++) {
          var prop = r.style[k], val = r.style.getPropertyValue(prop);
          if (val.indexOf("data:image/svg") < 0) continue;
          var next = val;
          swap.forEach(function (p) { next = next.replace(p[0], "%23" + p[1].slice(1)); });
          if (next !== val) decl += prop + ":" + next + (r.style.getPropertyPriority(prop) ? " !important" : "") + ";";
        }
        if (decl) out.push((wrap ? wrap[0] : "") + r.selectorText + "{" + decl + "}" + (wrap ? wrap[1] : ""));
      }
    }
    for (var n = 0; n < document.styleSheets.length; n++) {
      var sheet = document.styleSheets[n];
      if (!sheet.href || !/skins\.css(\?|$)/.test(sheet.href)) continue;
      try { walk(sheet.cssRules, null); } catch (e) {}
    }
    return out.join("\n");
  }
  var applied = null;
  function apply(id) {
    var s = byId(id || chosen()), root = document.documentElement, st = document.getElementById("d95-scheme");
    if (applied === s.id) return;
    applied = s.id;
    if (s === STANDARD) {
      VARS.forEach(function (v) { root.style.removeProperty(v); });
      if (st) st.remove();
      root.removeAttribute("data-scheme");
      return;
    }
    var t = ends(s);
    root.style.setProperty("--bg", s.desk);
    root.style.setProperty("--panel", s.face);
    root.style.setProperty("--panel-2", s.face);
    root.style.setProperty("--line", s.lo);
    root.style.setProperty("--accent", s.sel);
    root.style.setProperty("--sk-lo", s.lo);
    root.style.setProperty("--sk-mid", s.mid);
    root.style.setProperty("--sk-title", t[0] === t[1] ? t[0] : "linear-gradient(90deg, " + t[0] + ", " + t[1] + ")");
    root.style.setProperty("--sk-band", t[0] === t[1] ? t[0] : "linear-gradient(0deg, " + t[0] + ", " + t[1] + ")");
    root.setAttribute("data-scheme", s.id);
    if (!st) { st = document.createElement("style"); st.id = "d95-scheme"; document.head.appendChild(st); }
    st.textContent = uriRules(s);
  }
  if (typeof document !== "undefined") {
    apply();
    window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "scheme") apply(); });
    window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) apply(); });
  }
  return { LIST: LIST, ids: LIST.map(function (s) { return s.id; }), byId: byId, ends: ends, contrast: contrast, apply: apply, chosen: chosen };
})();
if (typeof module !== "undefined") module.exports = ByopopSchemes;
