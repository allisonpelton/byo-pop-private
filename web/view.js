// A game page's view settings (docs/WEB_PLAYER.md, "View settings"; web/settings.js): what the page draws over or does to the
// game's picture, never what the game draws or how it plays.
//   scanlines    off, light or strong: a dark line along every second row of the game's own pixels, over the picture
//   colorFilter  none, greyscale, sepia, warm (less blue light), contrast or vivid: a CSS filter on the game's canvas
// Both are off by default, and neither runs anything in a frame.
var ByopopView = (function () {
  "use strict";
  var KEY = "byopop-settings";
  var FILTERS = {
    none: "",
    grey: "grayscale(1)",
    sepia: "sepia(0.85)",
    warm: "sepia(0.35) saturate(1.1) hue-rotate(-12deg) brightness(0.97)",
    contrast: "contrast(1.35) saturate(1.15)",
    vivid: "saturate(1.6)"
  };
  var SCAN = { light: 0.22, strong: 0.45 };
  var pageGame = null, stage = null, canvas = null, scan = null, resizeObs = null;

  function setting(name) { try { return ByopopSettings.get(name, pageGame || undefined); } catch (e) { return null; } }

  // the scanlines: one dark line a game row high every two game rows, as many as the canvas has rows
  function sizeScan() {
    if (!scan || !canvas) return;
    var rows = canvas.height || 480, h = stage.clientHeight || 1, px = h / rows;
    scan.style.backgroundSize = "100% " + (2 * px) + "px";
  }
  function syncScan() {
    var v = setting("scanlines");
    if (!SCAN[v]) {
      if (scan) { scan.remove(); scan = null; }
      if (resizeObs) { resizeObs.disconnect(); resizeObs = null; }
      return;
    }
    if (!scan) {
      scan = document.createElement("div");
      scan.className = "bv-scan";
      scan.setAttribute("aria-hidden", "true");
      stage.insertBefore(scan, canvas.nextSibling);
      if (window.ResizeObserver) { resizeObs = new ResizeObserver(sizeScan); resizeObs.observe(stage); resizeObs.observe(canvas); }
    }
    scan.style.setProperty("--bv-scan", String(SCAN[v]));
    sizeScan();
  }
  function syncFilter() {
    var v = setting("colorFilter");
    canvas.style.filter = FILTERS[v] || "";
  }

  var STYLE = ".bv-scan { position: absolute; inset: 0; pointer-events: none; z-index: 1;" +
    " background-image: linear-gradient(to bottom, transparent 50%, rgba(0, 0, 0, var(--bv-scan, 0.22)) 50%); background-repeat: repeat; }";
  function sync() {
    stage = stage || document.getElementById("stage");
    canvas = canvas || document.getElementById("canvas");
    if (!stage || !canvas) return;
    if (!document.getElementById("bv-style")) {
      var st = document.createElement("style");
      st.id = "bv-style";
      st.textContent = STYLE;
      document.head.appendChild(st);
    }
    syncScan();
    syncFilter();
  }
  function setGame(id) { pageGame = id || null; sync(); }
  window.addEventListener("byopop-setting", function (ev) {
    var n = ev.detail && ev.detail.name;
    if (n === "scanlines" || n === "colorFilter") sync();
  });
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) sync(); });
  return { setGame: setGame, sync: sync };
})();
