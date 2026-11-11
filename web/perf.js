// The performance panel (docs/WEB_PLAYER.md, "The performance panel"): a game page's readings of how the game runs in this
// browser, for a player who wants to see them. It is the site setting "perf" (web/settings.js), off by default, and it only
// reads: it changes nothing in the game, so play is the same with it on.
//
// With it off nothing of it runs: no animation frame, no observer, no timer, no wrapped WebGL call. The one thing left is its
// link in the framework's frame hook (Module.byopopTestFrame), which returns at once.
//
// What it reads, each second:
//   frames       frames per second and frame time from requestAnimationFrame; the time the game's updates and draw took in
//                each of them (the framework's frame hook passes it)
//   updates      updates per second, and catch-up frames, those that ran more than one update (byopop_update_count)
//   main thread  the game's share of the page's main thread, and the long tasks the browser reports (over 50 ms): shares of
//                one thread, never the machine's CPU use
//   memory       the wasm heap (its size and what malloc has handed out), the framework's images and sounds with their bytes
//                (byopop_perf_json), the JavaScript heap where the browser gives it (performance.memory), and the site's
//                storage (navigator.storage.estimate)
//   graphics     the WebGL context the game draws with (GPUInterfaceGL's), its calls counted by wrapping its methods while the
//                panel is on: draw calls per frame, texture uploads and the bytes uploaded each second, the renderer's name,
//                and the GPU's time per frame where the browser really offers a timer query (EXT_disjoint_timer_query)
//   sound        the audio context's baseLatency and outputLatency, and the mixed sound queued ahead (Module.byopopAudio,
//                platform/Audio.cpp)
var ByopopPerf = (function () {
  "use strict";
  var KEY = "byopop-settings";
  var on = false, panel = null, body = null, pageGame = null;
  var raf = 0, tick = 0, storageTimer = 0, longObs = null;
  var st = null;          // this second's counts
  var last = null;        // what the panel shows
  var gl = null, restore = [], timer = null, queries = [], activeQuery = null, gpuMs = [];
  var storage = null;

  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  // ?perf=1 turns the panel on for this visit, whatever the setting says: the way in on a phone, where the setting is some menus
  // away (the user, 2026-11-11: "I'll need a list of instructions that should pop it up for me"). The one page passes it on to
  // its game windows (web/deskapps.js, frameUrl).
  var ASKED = (function () { try { return new URLSearchParams(location.search).get("perf") === "1"; } catch (e) { return false; } })();
  function small() { try { return window.matchMedia("(max-width: 600px), (max-height: 500px)").matches; } catch (e) { return false; } }
  function setting() {
    if (ASKED) return "on";
    try {
      var o = JSON.parse(window.localStorage.getItem(KEY) || "{}"), g = pageGame && o.games && o.games[pageGame];
      if (g && (g.perf === "on" || g.perf === "off")) return g.perf;
      return o.perf === "on" ? "on" : "off";
    } catch (e) { return "off"; }
  }
  function fresh() { return { frames: 0, frameMs: 0, frameMax: 0, gameMs: 0, gameMax: 0, hooks: 0, updates: 0, catchups: 0, longN: 0, longMs: 0, draws: 0, uploads: 0, upBytes: 0, t0: performance.now(), lastRaf: 0, lastUpd: -1 }; }

  // ---------- the framework's frame hook ----------
  var framed = false;  // the game has drawn: its canvas has its context
  function onFrame(drew, ms) {
    if (!on || !st) return;
    st.hooks++;
    if (drew) framed = true;
    if (typeof ms === "number") { st.gameMs += ms; if (ms > st.gameMax) st.gameMax = ms; }
    var M = window.Module, n = -1;
    try { if (M && typeof M._byopop_update_count === "function") n = M._byopop_update_count(); } catch (e) {}
    if (n >= 0) {
      if (st.lastUpd >= 0 && n > st.lastUpd) { st.updates += n - st.lastUpd; if (n - st.lastUpd > 1) st.catchups++; }
      st.lastUpd = n;
    }
    if (timer) gpuQuery();
  }
  var hooked = false;
  function hook() {
    var M = window.Module;
    if (hooked || !M) return;
    hooked = true;
    var prev = M.byopopTestFrame;
    M.byopopTestFrame = function (drew, ms) { if (prev) prev(drew, ms); onFrame(drew, ms); };
  }

  // ---------- WebGL ----------
  function wrapGl() {
    var canvas = document.getElementById("canvas");
    if (!canvas || gl) return;
    // the context the game already made, once it has drawn: getContext gives the same one back for its own type, and null for
    // another (a game drawing on a 2D canvas has none). Asked before the game made its own, it would make one and take the
    // canvas from the game.
    var c = null;
    try { c = canvas.getContext("webgl2") || canvas.getContext("webgl"); } catch (e) {}
    if (!c) return;
    gl = c;
    function wrap(name, count) {
      var orig = gl[name];
      if (typeof orig !== "function") return;
      gl[name] = function () { if (on && st) count(arguments); return orig.apply(this, arguments); };
      restore.push(function () { gl[name] = orig; });
    }
    function draw() { st.draws++; }
    ["drawArrays", "drawElements", "drawArraysInstanced", "drawElementsInstanced", "drawRangeElements"].forEach(function (n) { wrap(n, draw); });
    // a pixel's bytes from its format and type: the channels a format has, the bytes a type gives each; a packed type
    // (5_6_5, 4_4_4_4, 5_5_5_1: 2 bytes; 2_10_10_10, 10F_11F_11F, 5_9_9_9, 24_8: 4) is the whole pixel; 4 when unknown
    var CHANNELS = { 0x1908: 4, 0x1907: 3, 0x1906: 1, 0x1909: 1, 0x190A: 2, 0x1903: 1, 0x8227: 2, 0x8D94: 1, 0x8228: 2,
      0x8D98: 3, 0x8D99: 4, 0x1902: 1, 0x84F9: 1 };
    var TYPE_BYTES = { 0x1400: 1, 0x1401: 1, 0x1402: 2, 0x1403: 2, 0x1404: 4, 0x1405: 4, 0x1406: 4, 0x140B: 2, 0x8D61: 2 };
    var PACKED = { 0x8363: 2, 0x8033: 2, 0x8034: 2, 0x8368: 4, 0x8C3B: 4, 0x8C3E: 4, 0x84FA: 4 };
    function pixelBytes(format, type) {
      if (PACKED[type]) return PACKED[type];
      return CHANNELS[format] && TYPE_BYTES[type] ? CHANNELS[format] * TYPE_BYTES[type] : 4;
    }
    // texImage2D(target, level, internalformat, width, height, border, format, type, pixels[, offset]) and
    // texSubImage2D(target, level, x, y, width, height, format, type, pixels[, offset]): width, height, format and type at
    // wi, wi + 1, 6 and 7; from an image, a canvas or ImageData, (.., format, type, source) as the last three
    function texBytes(a, wi) {
      if (a.length >= 9 && typeof a[wi] === "number") return a[wi] * a[wi + 1] * pixelBytes(a[6], a[7]);
      var src = a[a.length - 1];
      return src && src.width ? src.width * src.height * pixelBytes(a[a.length - 3], a[a.length - 2]) : 0;
    }
    wrap("texImage2D", function (a) { st.uploads++; st.upBytes += texBytes(a, 3); });
    wrap("texSubImage2D", function (a) { st.uploads++; st.upBytes += texBytes(a, 4); });
    // bufferData(target, size or data, usage[, srcOffset, length]) and bufferSubData(target, dstOffset, data[, srcOffset,
    // length]): WebGL2's forms take a view and a part of it. Emscripten passes the whole wasm heap with an offset and a
    // length, so the length (in the view's elements) is what goes up; without one, the view from srcOffset to its end.
    function bufBytes(d, srcOffset, length) {
      if (typeof d === "number") return d;
      if (!d || !d.byteLength) return 0;
      var each = d.BYTES_PER_ELEMENT || 1;
      if (length) return length * each;
      return Math.max(0, d.byteLength - (srcOffset || 0) * each);
    }
    wrap("bufferData", function (a) { st.upBytes += bufBytes(a[1], a[3], a[4]); });
    wrap("bufferSubData", function (a) { st.upBytes += bufBytes(a[2], a[3], a[4]); });
    // the GPU's own time, only where the browser offers the query
    var isGl2 = typeof WebGL2RenderingContext !== "undefined" && gl instanceof WebGL2RenderingContext;
    var ext = null;
    try { ext = gl.getExtension(isGl2 ? "EXT_disjoint_timer_query_webgl2" : "EXT_disjoint_timer_query"); } catch (e) {}
    timer = ext ? { ext: ext, gl2: isGl2 } : null;
  }
  function unwrapGl() {
    restore.forEach(function (f) { try { f(); } catch (e) {} });
    restore = [];
    try { if (activeQuery) endQuery(); } catch (e) {}
    queries.forEach(function (q) { try { deleteQuery(q); } catch (e) {} });
    queries = []; activeQuery = null; gl = null; timer = null;
  }
  function endQuery() {
    if (timer.gl2) gl.endQuery(timer.ext.TIME_ELAPSED_EXT); else timer.ext.endQueryEXT(timer.ext.TIME_ELAPSED_EXT);
    queries.push(activeQuery);
    activeQuery = null;
  }
  function deleteQuery(q) { if (timer.gl2) gl.deleteQuery(q); else timer.ext.deleteQueryEXT(q); }
  // ends the query that covered the frame just drawn, starts one for the next, and reads the ones that are ready
  function gpuQuery() {
    try {
      var x = timer.ext;
      if (activeQuery) endQuery();
      while (queries.length) {
        var q = queries[0];
        var ready = timer.gl2 ? gl.getQueryParameter(q, gl.QUERY_RESULT_AVAILABLE) : x.getQueryObjectEXT(q, x.QUERY_RESULT_AVAILABLE_EXT);
        if (!ready) break;
        var disjoint = gl.getParameter(x.GPU_DISJOINT_EXT);
        var ns = timer.gl2 ? gl.getQueryParameter(q, gl.QUERY_RESULT) : x.getQueryObjectEXT(q, x.QUERY_RESULT_EXT);
        if (!disjoint) gpuMs.push(ns / 1e6);
        deleteQuery(q);
        queries.shift();
      }
      if (queries.length < 8) {
        activeQuery = timer.gl2 ? gl.createQuery() : x.createQueryEXT();
        if (timer.gl2) gl.beginQuery(x.TIME_ELAPSED_EXT, activeQuery); else x.beginQueryEXT(x.TIME_ELAPSED_EXT, activeQuery);
      }
    } catch (e) { timer = null; }
  }
  function renderer() {
    if (!gl) return "";
    try {
      var info = gl.getExtension("WEBGL_debug_renderer_info");
      return String(gl.getParameter(info ? info.UNMASKED_RENDERER_WEBGL : gl.RENDERER) || "");
    } catch (e) { return ""; }
  }

  // ---------- the second's readings ----------
  function onRaf(t) {
    if (!on) return;
    if (st.lastRaf) { var d = t - st.lastRaf; st.frames++; st.frameMs += d; if (d > st.frameMax) st.frameMax = d; }
    st.lastRaf = t;
    raf = requestAnimationFrame(onRaf);
  }
  function framework() {
    var M = window.Module;
    try {
      if (M && typeof M._byopop_perf_json === "function") {
        return JSON.parse(M.ccall("byopop_perf_json", "string", [], []));
      }
    } catch (e) {}
    return null;
  }
  function second() {
    if (!on) return;
    if (!gl && framed) wrapGl();
    var now = performance.now(), secs = Math.max(0.001, (now - st.t0) / 1000), s = st;
    var fw = framework(), audio = null;
    try {
      var M = window.Module, ctx = M && M.SDL2 && M.SDL2.audioContext, a = M && M.byopopAudio;
      if (ctx) {
        audio = { base: ctx.baseLatency, output: ctx.outputLatency, state: ctx.state, rate: ctx.sampleRate };
        if (a && a.kind === "worklet" && a.mixedEnd) audio.queued = (a.mixedEnd - ctx.currentTime * ctx.sampleRate) / ctx.sampleRate * 1000;
        if (a) { audio.kind = a.kind; audio.gaps = a.gaps; audio.target = a.target; }
      }
    } catch (e) {}
    var gpu = gpuMs.length ? { avg: gpuMs.reduce(function (x, y) { return x + y; }, 0) / gpuMs.length, max: Math.max.apply(null, gpuMs) } : null;
    gpuMs = [];
    var mem = null;
    try { if (performance.memory) mem = { used: performance.memory.usedJSHeapSize, total: performance.memory.totalJSHeapSize }; } catch (e) {}
    last = {
      fps: s.frames / secs, frameAvg: s.frames ? s.frameMs / s.frames : 0, frameMax: s.frameMax,
      gameAvg: s.hooks ? s.gameMs / s.hooks : 0, gameMax: s.gameMax, share: s.gameMs / (secs * 1000),
      ups: s.updates / secs, catchups: s.catchups, longN: s.longN, longShare: s.longMs / (secs * 1000),
      fw: fw, js: mem, storage: storage,
      renderer: renderer(), hasGl: !!gl, framed: framed, draws: s.hooks ? s.draws / s.hooks : 0, uploads: s.uploads / secs, upBytes: s.upBytes / secs,
      timer: !!timer, gpu: gpu, audio: audio
    };
    var lastUpd = s.lastUpd;
    st = fresh();
    st.lastUpd = lastUpd;
    draw();
  }
  function readStorage() {
    try {
      if (navigator.storage && navigator.storage.estimate)
        navigator.storage.estimate().then(function (e) { storage = e; }, function () {});
    } catch (e) {}
  }

  // ---------- the panel ----------
  function bytes(n) {
    if (typeof n !== "number" || !isFinite(n)) return T("perf.na");
    if (n >= 1073741824) return (n / 1073741824).toFixed(2) + " GB";
    if (n >= 1048576) return (n / 1048576).toFixed(1) + " MB";
    if (n >= 1024) return Math.round(n / 1024) + " KB";
    return Math.round(n) + " B";
  }
  function ms(n) { return (typeof n === "number" && isFinite(n)) ? n.toFixed(1) + " ms" : T("perf.na"); }
  function pct(n) { return Math.round(n * 100) + "%"; }
  function draw() {
    if (!body || !last) return;
    var L = last, fw = L.fw, a = L.audio, rows = [];
    function group(k) { rows.push([T(k), null]); }
    function row(k, v) { rows.push([T(k), v]); }
    group("perf.frames");
    row("perf.fps", L.fps.toFixed(0));
    row("perf.frame_ms", ms(L.frameAvg) + " / " + ms(L.frameMax));
    row("perf.game_ms", ms(L.gameAvg) + " / " + ms(L.gameMax));
    row("perf.updates", L.ups.toFixed(0));
    row("perf.catchups", String(L.catchups));
    group("perf.thread");
    row("perf.thread_game", pct(L.share));
    row("perf.longtasks", longObs ? L.longN + " · " + pct(L.longShare) : T("perf.na"));
    group("perf.memory");
    row("perf.heap", fw ? bytes(fw.heapUsed) + " / " + bytes(fw.heap) : T("perf.na"));
    row("perf.images", fw ? fw.images + " · " + bytes(fw.imageBytes) : T("perf.na"));
    row("perf.sounds", fw ? fw.sounds + " · " + bytes(fw.soundBytes) : T("perf.na"));
    row("perf.js_heap", L.js ? bytes(L.js.used) + " / " + bytes(L.js.total) : T("perf.na"));
    row("perf.storage", L.storage ? bytes(L.storage.usage) + " / " + bytes(L.storage.quota) : T("perf.na"));
    group("perf.gpu");
    row("perf.renderer", L.renderer || T(L.framed ? "perf.no_gl" : "perf.na"));
    row("perf.draws", L.hasGl ? L.draws.toFixed(0) : T("perf.na"));
    row("perf.uploads", L.hasGl ? L.uploads.toFixed(0) + " · " + bytes(L.upBytes) : T("perf.na"));
    row("perf.gpu_ms", L.timer ? (L.gpu ? ms(L.gpu.avg) + " / " + ms(L.gpu.max) : "…") : T("perf.no_timer"));
    group("perf.audio");
    row("perf.latency", a ? ms(a.base * 1000) + " / " + ms(a.output * 1000) : T("perf.na"));
    row("perf.queued", a && typeof a.queued === "number" ? ms(a.queued) : T("perf.na"));
    row("perf.gaps", a && typeof a.gaps === "number" ? String(a.gaps) : T("perf.na"));
    // on a small screen the Sound group comes first, where it is read without scrolling (it is the one asked for there)
    if (small()) rows = rows.slice(-4).concat(rows.slice(0, -4));
    var dl = document.createElement("dl");
    rows.forEach(function (r) {
      if (r[1] === null) { var h = document.createElement("dt"); h.className = "perf-group"; h.textContent = r[0]; dl.appendChild(h); return; }
      var dt = document.createElement("dt"), dd = document.createElement("dd");
      dt.textContent = r[0]; dd.textContent = r[1];
      dl.appendChild(dt); dl.appendChild(dd);
    });
    var was = body.offsetHeight;
    body.textContent = "";
    body.appendChild(dl);
    // beside the game, a panel that grew (its readings come after "waiting") asks the window to fit it again (web/shell.html,
    // sideHeight)
    if (panel.classList.contains("perf-side") && body.offsetHeight > was) try { window.dispatchEvent(new CustomEvent("byopop-side")); } catch (e) {}
  }
  var STYLE =
    ".perf-panel { position: fixed; right: 12px; top: 60px; z-index: 9; width: 290px; max-height: calc(calc(100vh / var(--d95-z, 1)) - 80px); overflow: auto; box-sizing: border-box;\n" +
    "  background: var(--panel); color: var(--fg); border: 1px solid var(--line); border-radius: 8px; box-shadow: 0 6px 24px rgba(0,0,0,.25); font-size: 12px; }\n" +
    ".perf-panel h2 { margin: 0; padding: 6px 10px; font-size: 13px; border-bottom: 1px solid var(--line); }\n" +
    ".perf-panel p { margin: 6px 10px 0; color: var(--muted); }\n" +
    ".perf-panel dl { display: grid; grid-template-columns: minmax(0, 1fr) minmax(0, 46%); gap: 1px 10px; margin: 6px 10px 10px; }\n" +
    ".perf-panel dt { color: var(--muted); }\n" +
    ".perf-panel dd { margin: 0; text-align: right; font-variant-numeric: tabular-nums; font-family: ui-monospace, Consolas, monospace; overflow-wrap: anywhere; }\n" +
    ".perf-panel dt.perf-group { grid-column: 1 / -1; color: var(--fg); font-weight: 700; margin-top: 6px; }\n" +
    "html.player-only .perf-panel { display: none !important; }\n" +
    ".perf-panel.perf-side { position: static; width: 250px; max-height: none; border-radius: 0; box-shadow: none; border-width: 0 0 0 1px;\n" +
    "  grid-column: 2; grid-row: 1 / 3; align-self: stretch; overflow: auto; }\n" +
    "html.perf-side .stage-wrap { grid-template-columns: minmax(0, 1fr) auto; }\n" +
    // a game's own page with Desktop 95's desktop: the game's window (#main) keeps its title bar over both and its status bar
    // under both, as a 1995 window's
    "html.d95-desk:not(.d95-embed).perf-side #main { display: grid; grid-template-columns: auto auto; grid-template-rows: auto auto 1fr auto; }\n" +
    "html.d95-desk:not(.d95-embed).perf-side #main > .d95-tb, html.d95-desk:not(.d95-embed).perf-side #main > .d95-statusbar { grid-column: 1 / -1; }\n" +
    "html.d95-desk:not(.d95-embed).perf-side #main > .stage { grid-column: 1; grid-row: 2; }\n" +
    "html.d95-desk:not(.d95-embed).perf-side #main > .d95-statusbar { grid-row: 4; }\n" +
    "html.d95-desk:not(.d95-embed).perf-side #main > .perf-panel { grid-column: 2; grid-row: 2 / 4; }\n" +
    // a narrow page (a phone held upright): under the game, as wide as the page, scrolling in itself
    "@media (max-width: 600px) {\n" +
    "  .perf-panel.perf-side { grid-column: 1; grid-row: auto; width: auto; max-height: calc(40vh / var(--d95-z, 1)); border-width: 1px 0 0; }\n" +
    "  html.perf-side .stage-wrap { grid-template-columns: minmax(0, 1fr); }\n" +
    "}\n" +
    ".perf-panel.perf-side p { display: none; }\n" +
    "html.perf-side.touch-side .stage-wrap { grid-template-columns: minmax(0, 1fr) auto auto; }\n" +
    "html.perf-side.touch-side .stage-wrap > .bc-touch { grid-column: 3; }\n";
  function build() {
    if (panel) return;
    var style = document.createElement("style");
    style.textContent = STYLE;
    document.head.appendChild(style);
    panel = document.createElement("aside");
    panel.className = "perf-panel";
    panel.id = "perf-panel";
    panel.setAttribute("aria-labelledby", "perf-title");
    panel.hidden = true;
    var h = document.createElement("h2"); h.id = "perf-title"; h.textContent = T("perf.title");
    var note = document.createElement("p"); note.textContent = T("perf.note");
    body = document.createElement("div");
    body.textContent = T("perf.waiting");
    panel.appendChild(h); panel.appendChild(note); panel.appendChild(body);
    // In a game's window on the one page the panel is part of the window, at the right of the game, which it never covers; the
    // window grows by its width (the user, 2026-11-10: "The performance panel should expand the game window and be static to the
    // right"; web/shell.html, sideWidth). On a game's own page with Desktop 95's desktop the game's window is #main, which
    // takes its size from what it holds, and the panel goes into it in the same way (a play-test, 2026-11-11, found it a window
    // of its own at the desktop's right, there from the setup screen on, apart from the game's)
    var main = document.getElementById("main"), cls = document.documentElement.classList;
    if ((cls.contains("d95-embed") || cls.contains("d95-desk")) && main) {
      panel.classList.add("perf-side");
      main.appendChild(panel);
      return;
    }
    document.body.appendChild(panel);
    // a window of Desktop 95's desktop (web/skins.js), whose icon is there while the panel is
    try {
      if (window.ByopopSkin && ByopopSkin.desk)
        ByopopSkin.desk.add(panel, { id: "perf", title: T("perf.title"), handle: h, button: panel,
          open: function () {}, at: function (left, vw) { return [Math.max(left, vw - 310), 12]; } });
    } catch (e) {}
  }
  function start() {
    if (on) return;
    on = true;
    hook();
    build();
    panel.hidden = false;
    side();
    st = fresh();
    raf = requestAnimationFrame(onRaf);
    tick = setInterval(second, 1000);
    readStorage();
    storageTimer = setInterval(readStorage, 10000);
    try {
      if (window.PerformanceObserver && (PerformanceObserver.supportedEntryTypes || []).indexOf("longtask") >= 0) {
        longObs = new PerformanceObserver(function (list) {
          if (!st) return;
          list.getEntries().forEach(function (e) { st.longN++; st.longMs += e.duration; });
        });
        longObs.observe({ type: "longtask" });
      }
    } catch (e) { longObs = null; }
  }
  function stop() {
    if (!on) return;
    on = false;
    cancelAnimationFrame(raf); raf = 0;
    clearInterval(tick); tick = 0;
    clearInterval(storageTimer); storageTimer = 0;
    if (longObs) { try { longObs.disconnect(); } catch (e) {} longObs = null; }
    unwrapGl();
    st = null; last = null; storage = null; gpuMs = [];
    if (panel) { panel.hidden = true; body.textContent = T("perf.waiting"); }
    side();
  }
  function side() {
    var is = !!panel && !panel.hidden && panel.classList.contains("perf-side");
    document.documentElement.classList.toggle("perf-side", is);
    try { window.dispatchEvent(new CustomEvent("byopop-side")); } catch (e) {}
  }
  function sync() {
    if (!document.body || !document.getElementById("canvas")) return;
    if (document.documentElement.classList.contains("player-only")) return;
    if (setting() === "on") start(); else stop();
  }
  function setGame(id) { pageGame = id || null; sync(); }
  // a choice made on the page (the window's View menu, Settings) wins over ?perf=1 from then on, so the panel can be put away
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "perf") { ASKED = false; sync(); } });
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) sync(); });
  return { setGame: setGame, sync: sync, isOn: function () { return on; }, reading: function () { return last; } };
})();
