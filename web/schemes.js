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

  // ---- the desktop's pattern and wallpaper (the proposal's second part; the user, 2026-11-11: "Yes, 1 then 2") ----
  // A pattern is 8 by 8 pixels in two colours, as a 1995 desktop pattern was: the desktop's own colour and a darker one, drawn
  // here as eight rows of bits (the settings "pattern"). A wallpaper (the setting "wallpaper") is one of our own tiles, drawn
  // here in translucent white and black so that it suits every scheme, or a picture of the player's own, which never leaves
  // this browser: it is drawn into a canvas, scaled down to 1600 pixels at most and kept as a JPEG in localStorage
  // ("byopop-wallpaper"). The setting "wallpaperFit" centres, tiles or stretches it. Our own drawings, every one.
  var PATTERNS = {
    dots: [0x88, 0, 0, 0, 0x22, 0, 0, 0],
    halftone: [0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55],
    checks: [0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f],
    diagonal: [0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01],
    zigzag: [0x81, 0x42, 0x24, 0x18, 0, 0, 0, 0],
    lattice: [0xff, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80],
    blocks: [0xff, 0x80, 0x80, 0x80, 0xff, 0x08, 0x08, 0x08]
  };
  var TILES = {
    // bubbles rising, as in a glass of soda
    bubbles: "<svg xmlns='http://www.w3.org/2000/svg' width='96' height='96'><g fill='none' stroke='%23fff' stroke-opacity='.28'>" +
      "<circle cx='20' cy='22' r='9'/><circle cx='66' cy='14' r='5'/><circle cx='50' cy='50' r='13'/><circle cx='84' cy='66' r='7'/>" +
      "<circle cx='18' cy='74' r='6'/><circle cx='40' cy='86' r='3'/></g><g fill='%23fff' fill-opacity='.22'>" +
      "<circle cx='17' cy='19' r='2'/><circle cx='45' cy='45' r='3'/><circle cx='82' cy='64' r='1.5'/></g></svg>",
    // a picnic cloth
    gingham: "<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' shape-rendering='crispEdges'>" +
      "<rect width='16' height='32' fill='%23fff' fill-opacity='.16'/><rect width='32' height='16' fill='%23fff' fill-opacity='.16'/>" +
      "<rect x='16' y='16' width='16' height='16' fill='%23000' fill-opacity='.12'/></svg>",
    // bottle caps, seen from above
    caps: "<svg xmlns='http://www.w3.org/2000/svg' width='64' height='64'><g fill='none' stroke='%23000' stroke-opacity='.22'>" +
      "<circle cx='16' cy='16' r='11' stroke-width='3' stroke-dasharray='2.4 2.2'/><circle cx='48' cy='48' r='11' stroke-width='3' stroke-dasharray='2.4 2.2'/></g>" +
      "<g fill='%23fff' fill-opacity='.2'><circle cx='16' cy='16' r='8'/><circle cx='48' cy='48' r='8'/></g></svg>"
  };
  var FITS = ["centre", "tile", "stretch"], WALL_KEY = "byopop-wallpaper";
  var patternIds = ["none"].concat(Object.keys(PATTERNS)), wallpaperIds = ["none"].concat(Object.keys(TILES)).concat(["own"]);
  function stored(name, allowed, def) {
    try { var o = JSON.parse(window.localStorage.getItem(KEY) || "{}"), v = o && o[name]; return allowed.indexOf(v) >= 0 ? v : def; } catch (e) { return def; }
  }
  function patternUri(rows) {
    var rects = "";
    rows.forEach(function (bits, y) { for (var x = 0; x < 8; x++) if (bits & (0x80 >> x)) rects += "<rect x='" + x + "' y='" + y + "' width='1' height='1'/>"; });
    return "url(\"data:image/svg+xml," + "<svg xmlns='http://www.w3.org/2000/svg' width='8' height='8' shape-rendering='crispEdges' fill='%23000' fill-opacity='.38'>" + rects + "</svg>\")";
  }
  function picture() { try { return window.localStorage.getItem(WALL_KEY) || ""; } catch (e) { return ""; } }
  // the desktop is the listing's (and a game page's of its own); a page in a window of the one page has none to draw
  function backdrop() {
    var body = document.body;
    if (!body || window.top !== window) return;
    var layers = [], pat = stored("pattern", patternIds, "none"), wall = stored("wallpaper", wallpaperIds, "none"), fit = stored("wallpaperFit", FITS, "centre");
    if (PATTERNS[pat]) layers.push({ image: patternUri(PATTERNS[pat]), repeat: "repeat", position: "0 0", size: "8px 8px" });
    if (TILES[wall]) layers.push({ image: "url(\"data:image/svg+xml," + TILES[wall] + "\")", repeat: "repeat", position: "0 0", size: "auto" });
    else if (wall === "own" && picture()) {
      layers.push({ image: "url(\"" + picture() + "\")", repeat: fit === "tile" ? "repeat" : "no-repeat", position: fit === "tile" ? "0 0" : "center",
                    size: fit === "stretch" ? "100% 100%" : "auto" });
    }
    var st = body.style;
    st.backgroundImage = layers.map(function (l) { return l.image; }).join(", ");
    st.backgroundRepeat = layers.map(function (l) { return l.repeat; }).join(", ");
    st.backgroundPosition = layers.map(function (l) { return l.position; }).join(", ");
    st.backgroundSize = layers.map(function (l) { return l.size; }).join(", ");
    st.imageRendering = PATTERNS[pat] && layers.length === 1 ? "pixelated" : "";
  }
  // A picture of the player's own as the wallpaper: resolves to true when it is kept
  function setPicture(file) {
    return new Promise(function (resolve) {
      if (!file || !/^image\//.test(file.type || "")) { resolve(false); return; }
      var url = URL.createObjectURL(file), img = new Image();
      img.onload = function () {
        var ok = false;
        try {
          var k = Math.min(1, 1600 / Math.max(img.naturalWidth, img.naturalHeight));
          for (var tries = 0; tries < 5 && !ok; tries++, k *= 0.75) {
            var c = document.createElement("canvas");
            c.width = Math.max(1, Math.round(img.naturalWidth * k)); c.height = Math.max(1, Math.round(img.naturalHeight * k));
            c.getContext("2d").drawImage(img, 0, 0, c.width, c.height);
            var data = c.toDataURL("image/jpeg", 0.82);
            // (the browser gives a site a few megabytes there, which the saves' records share)
            if (data.length > 1500000) continue;
            try { window.localStorage.setItem(WALL_KEY, data); ok = true; } catch (e) {}
          }
        } catch (e) {}
        URL.revokeObjectURL(url);
        if (ok) backdrop();
        resolve(ok);
      };
      img.onerror = function () { URL.revokeObjectURL(url); resolve(false); };
      img.src = url;
    });
  }
  function clearPicture() { try { window.localStorage.removeItem(WALL_KEY); } catch (e) {} backdrop(); }
  // The picture's own controls for Appearance (web/index.html puts them under the settings' rows): a Browse button, the
  // picture kept as a small view, and Remove. onChange(true) once a picture is kept, so the page can pick "My picture".
  function chooser(T, onChange) {
    var row = document.createElement("div"); row.className = "bps-row sch-picture";
    var label = document.createElement("span"); label.className = "sch-picture-label"; label.textContent = T("settings.wallpaper.picture");
    var view = document.createElement("span"); view.className = "sch-picture-view"; view.setAttribute("role", "img");
    var input = document.createElement("input"); input.type = "file"; input.accept = "image/*"; input.hidden = true;
    var browse = document.createElement("button"); browse.type = "button"; browse.textContent = T("settings.wallpaper.browse");
    var remove = document.createElement("button"); remove.type = "button"; remove.textContent = T("settings.wallpaper.remove");
    function show() {
      var p = picture();
      view.style.backgroundImage = p ? "url(\"" + p + "\")" : "";
      view.setAttribute("aria-label", T(p ? "settings.wallpaper.kept" : "settings.wallpaper.none_kept"));
      remove.disabled = !p;
    }
    browse.addEventListener("click", function () { input.click(); });
    input.addEventListener("change", function () {
      var f = input.files && input.files[0];
      input.value = "";
      if (f) setPicture(f).then(function (ok) { show(); if (onChange) onChange(ok); });
    });
    remove.addEventListener("click", function () { clearPicture(); show(); if (onChange) onChange(false); });
    var ctl = document.createElement("div"); ctl.className = "bps-ctl";
    ctl.appendChild(view); ctl.appendChild(browse); ctl.appendChild(remove); ctl.appendChild(input);
    row.appendChild(label); row.appendChild(ctl);
    show();
    return row;
  }

  if (typeof document !== "undefined") {
    apply();
    if (document.body) backdrop(); else document.addEventListener("DOMContentLoaded", backdrop);
    window.addEventListener("byopop-setting", function (ev) {
      var n = ev.detail && ev.detail.name;
      if (n === "scheme") apply();
      if (n === "pattern" || n === "wallpaper" || n === "wallpaperFit") backdrop();
    });
    window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === WALL_KEY || ev.key === null) { apply(); backdrop(); } });
  }
  return { patterns: PATTERNS, patternIds: patternIds, wallpaperIds: wallpaperIds, fits: FITS, backdrop: backdrop, setPicture: setPicture,
    clearPicture: clearPicture, chooser: chooser, picture: picture, LIST: LIST, ids: LIST.map(function (s) { return s.id; }), byId: byId, ends: ends, contrast: contrast, apply: apply, chosen: chosen };
})();
if (typeof module !== "undefined") module.exports = ByopopSchemes;
