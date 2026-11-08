// A game page's view settings (docs/WEB_PLAYER.md, "View settings"; web/settings.js): what the page draws over or does to the
// game's picture, never what the game draws or how it plays.
//   scanlines    off, light or strong: a dark line along every second row of the game's own pixels, over the picture
//   colorFilter  none, greyscale, sepia, warm (less blue light), contrast or vivid: a CSS filter on the game's canvas
//   runTimer     on: a timer in the corner of the game that runs while the game does (from its first drawn frame, stopping
//                when no frame has come for half a second, as a paused or hidden game sends none), from 0 at each opening of
//                the page
// All three are off by default, and with all three off nothing of this runs but its link in the framework's frame hook
// (Module.byopopTestFrame), which returns at once.
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
  var pageGame = null, stage = null, canvas = null, scan = null, timer = null, resizeObs = null, hooked = false;
  var timing = false, runMs = 0, lastFrame = 0, shown = "";

  function setting(name) { try { return ByopopSettings.get(name, pageGame || undefined); } catch (e) { return null; } }
  function T(key) { try { return ByopopStrings.t(key); } catch (e) { return key; } }

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

  // the run timer
  function fmt(ms) {
    var s = Math.floor(ms / 1000), m = Math.floor(s / 60), h = Math.floor(m / 60), cs = Math.floor(ms / 10) % 100;
    function two(n) { return (n < 10 ? "0" : "") + n; }
    return (h ? h + ":" + two(m % 60) : m) + ":" + two(s % 60) + "." + two(cs);
  }
  function onFrame(drew) {
    if (!timer || !drew) return;
    var now = performance.now();
    // a gap of more than half a second is a pause (a hidden page, a game that stopped drawing), not run time
    if (timing && now - lastFrame < 500) runMs += now - lastFrame;
    timing = true;
    lastFrame = now;
    var text = fmt(runMs);
    if (text !== shown) { shown = text; timer.textContent = text; }
  }
  function hook() {
    if (hooked || !window.Module) return;
    hooked = true;
    var M = window.Module, prev = M.byopopTestFrame;
    M.byopopTestFrame = function (drew, ms) { if (prev) prev(drew, ms); onFrame(drew); };
  }
  function syncTimer() {
    if (setting("runTimer") !== "on") {
      if (timer) { timer.remove(); timer = null; }
      return;
    }
    if (!timer) {
      timer = document.createElement("div");
      timer.className = "bv-timer";
      timer.setAttribute("role", "timer");
      timer.setAttribute("aria-label", T("view.timer"));
      timer.textContent = fmt(runMs);
      stage.appendChild(timer);
    }
    hook();
  }

  var STYLE = ".bv-scan { position: absolute; inset: 0; pointer-events: none; z-index: 1;" +
    " background-image: linear-gradient(to bottom, transparent 50%, rgba(0, 0, 0, var(--bv-scan, 0.22)) 50%); background-repeat: repeat; }" +
    ".bv-timer { position: absolute; top: 6px; left: 6px; z-index: 2; pointer-events: none; padding: 2px 7px; border-radius: 4px;" +
    " font: 600 13px/1.3 ui-monospace, Consolas, monospace; font-variant-numeric: tabular-nums; color: #fff;" +
    " background: rgba(0, 0, 0, 0.55); text-shadow: 0 1px 1px #000; }";
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
    syncTimer();
  }
  function setGame(id) { pageGame = id || null; sync(); }
  window.addEventListener("byopop-setting", function (ev) {
    var n = ev.detail && ev.detail.name;
    if (n === "scanlines" || n === "colorFilter" || n === "runTimer") sync();
  });
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) sync(); });
  return { setGame: setGame, sync: sync, runMs: function () { return runMs; } };
})();
