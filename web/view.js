// A game page's view settings (docs/WEB_PLAYER.md, "View settings"; web/settings.js): what the page draws over or does to the
// game's picture, never what the game draws or how it plays.
//   scanlines    off, light or strong: a dark line along every second row of the game's own pixels, over the picture
//   colorFilter  none, greyscale, sepia, warm (less blue light), contrast, vivid, or one for each of protanopia, deuteranopia
//                and tritanopia: a CSS filter on the game's canvas (the last three an SVG colour matrix, below); or 256
//                colours, plain or dithered: a WebGL canvas over the game's (below)
// Both are off by default, and only the 256 colours run anything in a frame.
var ByopopView = (function () {
  "use strict";
  var KEY = "byopop-settings";
  var FILTERS = {
    none: "",
    grey: "grayscale(1)",
    sepia: "sepia(0.85)",
    warm: "sepia(0.35) saturate(1.1) hue-rotate(-12deg) brightness(0.97)",
    contrast: "contrast(1.35) saturate(1.15)",
    vivid: "saturate(1.6)",
    protan: "url(#bv-protan)",
    deutan: "url(#bv-deutan)",
    tritan: "url(#bv-tritan)"
  };
  var SCAN = { light: 0.22, strong: 0.45 };
  var pageGame = null, stage = null, canvas = null, scan = null, scanBox = "", resizeObs = null, styleObs = null;

  function setting(name) { try { return ByopopSettings.get(name, pageGame || undefined); } catch (e) { return null; } }

  // the game's own rows, from the shape the page gave the stage (web/shell.html, onGameScreen: "--aspect: w / h"); 480 until
  // a game gives one
  function gameRows() {
    var m = /\/\s*([0-9.]+)/.exec(stage.style.getPropertyValue("--aspect"));
    return (m && parseFloat(m[1])) || 480;
  }
  // the scanlines: one dark line a game row high every two game rows, over the canvas's own box (centred in a larger stage in
  // full screen). The rows are the game's and the height the canvas's drawn one, never the canvas's pixels: SDL sizes those
  // to the stage on its own time (300 x 150 before the game makes its window, the old size for a while after a resize, the
  // device's pixels on a high-density screen), and lines sized from them were drawn too large (the user, 2026-11-21:
  // "Somehow, the size of the scanlines briefly became much larger than they should be").
  function sizeScan() {
    if (!scan || !canvas) return;
    var rows = gameRows(), h = canvas.offsetHeight;
    var box = canvas.offsetLeft + "," + canvas.offsetTop + "," + canvas.offsetWidth + "," + h + "," + rows;
    if (box === scanBox) return;
    scanBox = box;
    var st = scan.style;
    st.left = canvas.offsetLeft + "px"; st.top = canvas.offsetTop + "px"; st.width = canvas.offsetWidth + "px"; st.height = h + "px";
    st.backgroundSize = "100% " + (2 * h / rows) + "px";
  }
  function syncScan() {
    var v = setting("scanlines");
    if (!SCAN[v]) {
      if (scan) { scan.remove(); scan = null; }
      if (resizeObs) { resizeObs.disconnect(); resizeObs = null; }
      if (styleObs) { styleObs.disconnect(); styleObs = null; }
      return;
    }
    if (!scan) {
      scan = document.createElement("div");
      scan.className = "bv-scan";
      scan.setAttribute("aria-hidden", "true");
      scanBox = "";
      stage.insertBefore(scan, (pal ? pal.cv : canvas).nextSibling);   // over the 256-colour picture, if there is one
      // every change of the canvas's box, its place in the stage (the stage's size) or the game's shape (the stage's style)
      if (window.ResizeObserver) { resizeObs = new ResizeObserver(sizeScan); resizeObs.observe(stage); resizeObs.observe(canvas); }
      if (window.MutationObserver) { styleObs = new MutationObserver(sizeScan); styleObs.observe(stage, { attributes: true, attributeFilter: ["style"] }); }
    }
    scan.style.setProperty("--bv-scan", String(SCAN[v]));
    sizeScan();
  }
  // ---------- for colour-blind players ----------
  // Standard mod 15's palettes, made a site filter (the user, 2026-11-07, of moving them to the site: "Yes, move them"): the
  // picture recoloured so that colours a colour-blind player confuses come apart, by daltonization. What the player would see
  // is simulated (Machado, Oliveira and Fernandes 2009, each matrix at severity 1, in linear RGB); the difference from the
  // real colour, which that player loses, is moved into channels that player still tells apart (shift); and the colour
  // becomes colour + shift x (colour - simulated), one matrix for each kind. Greys stay as they are (each row of a simulation
  // sums to 1). Red and green lost (protanopia, deuteranopia) go into green and blue (Fidaner, Lin and Ozguven 2005); blue and
  // yellow lost (tritanopia) go into red and green against each other, the opposition a tritanope keeps. Checked against
  // random colour pairs that the simulation puts within 10 of each other in CIELAB but are 25 or more apart: half of them
  // end at least 4.0 to 4.4 times further apart for protanopia, 3.2 to 3.4 for deuteranopia and 2.0 to 2.2 for tritanopia
  // (where the usual shift into red and green alike gave 1.5 to 1.6). The filter is the browser's (an SVG feColorMatrix
  // in linear RGB, as the matrices are), with nothing run in a frame.
  var SIMULATED = {
    protan: [[0.152286, 1.052583, -0.204868], [0.114503, 0.786281, 0.099216], [-0.003882, -0.048116, 1.051998]],
    deutan: [[0.367322, 0.860646, -0.227968], [0.280085, 0.672501, 0.047413], [-0.011820, 0.042940, 0.968881]],
    tritan: [[1.255528, -0.076749, -0.178779], [-0.078411, 0.930809, 0.147602], [0.004733, 0.691367, 0.303900]]
  };
  var RED_GREEN_SHIFT = [[0, 0, 0], [0.7, 1, 0], [0.7, 0, 1]], BLUE_YELLOW_SHIFT = [[1, 0, 0.7], [0, 1, -0.7], [0, 0, 0]];
  var SHIFT = { protan: RED_GREEN_SHIFT, deutan: RED_GREEN_SHIFT, tritan: BLUE_YELLOW_SHIFT };
  // colour + shift x (colour - simulated), as one matrix: identity + shift x (identity - simulated)
  function daltonized(kind) {
    var sim = SIMULATED[kind], shift = SHIFT[kind], out = [];
    for (var row = 0; row < 3; row++) {
      out.push([]);
      for (var col = 0; col < 3; col++) {
        var lost = 0;
        for (var k = 0; k < 3; k++) lost += shift[row][k] * ((k === col ? 1 : 0) - sim[k][col]);
        out[row].push((row === col ? 1 : 0) + lost);
      }
    }
    return out;
  }
  // the three filters, put once into the page in a hidden SVG that the canvas's filter names
  function colourBlindFilters() {
    if (document.getElementById("bv-filters")) return;
    var NS = "http://www.w3.org/2000/svg", svg = document.createElementNS(NS, "svg");
    svg.id = "bv-filters";
    svg.setAttribute("width", "0"); svg.setAttribute("height", "0"); svg.setAttribute("aria-hidden", "true"); svg.setAttribute("focusable", "false");
    svg.style.position = "absolute";
    Object.keys(SIMULATED).forEach(function (kind) {
      var f = document.createElementNS(NS, "filter"), m = document.createElementNS(NS, "feColorMatrix"), d = daltonized(kind), values = [];
      f.id = "bv-" + kind;
      f.setAttribute("color-interpolation-filters", "linearRGB");
      d.forEach(function (r) { values.push(r[0].toFixed(6), r[1].toFixed(6), r[2].toFixed(6), "0", "0"); });
      values.push("0", "0", "0", "1", "0");
      m.setAttribute("type", "matrix");
      m.setAttribute("values", values.join(" "));
      f.appendChild(m); svg.appendChild(f);
    });
    document.body.appendChild(svg);
  }

  // ---------- 256 colours ----------
  // The game's picture as a 256-colour screen showed it (the user, 2026-11-09: "Is it possible to make a filter for the game
  // windows to make them appear in 256 colors only?"): each pixel the nearest colour of the palette Windows kept for programs
  // that set none, its 20 static colours and the 6x6x6 cube (levels 0, 51, ..., 255), either plainly ("pal256") or through
  // an 8x8 ordered dither ("pal256d"). A WebGL canvas the size of the game's own pixels lies exactly over the game's canvas,
  // takes no input, and is filled from the game's canvas each time the game draws a frame (the framework's frame hook,
  // Module.byopopTestFrame), while that frame's picture is still there. The nearest colour is the cube's own arithmetic and
  // a comparison with the twelve static colours the cube lacks: no search of 256 entries. Nothing is made or run while
  // another filter is chosen.
  var PAL = { pal256: 0, pal256d: 1 };
  // the twenty static colours that are not on the cube (black, white and the six full primaries are)
  var EXTRAS = [[128, 0, 0], [0, 128, 0], [128, 128, 0], [0, 0, 128], [128, 0, 128], [0, 128, 128], [192, 192, 192], [192, 220, 192],
    [166, 202, 240], [255, 251, 240], [160, 160, 164], [128, 128, 128]];
  var VERT = "attribute vec2 p; varying vec2 uv; void main() { uv = p * 0.5 + 0.5; gl_Position = vec4(p, 0.0, 1.0); }";
  function fragment() {
    var ex = EXTRAS.map(function (c) { return "vec3(" + c.map(function (v) { return (v / 255).toFixed(6); }).join(",") + ")"; });
    var s = "precision highp float; varying vec2 uv; uniform sampler2D tex; uniform float dither; uniform float cell;\n" +
      // Bayer's 8x8 matrix, from its 2x2 one by the usual recursion
      "float bayer2(vec2 a) { a = floor(a); return fract(a.x / 2.0 + a.y * a.y * 0.75); }\n" +
      "float bayer8(vec2 a) { return bayer2(0.25 * a) * 0.0625 + bayer2(0.5 * a) * 0.25 + bayer2(a); }\n" +
      "void main() {\n" +
      "  vec3 c = texture2D(tex, uv).rgb;\n" +
      // the dither moves a colour by up to half a step of the cube either way, so that the cube's colours mix to it on average;
      // its cells are the game's own pixels (the canvas has the stage's device pixels, the game's picture scaled into them)
      "  if (dither > 0.5) c = clamp(c + (bayer8(gl_FragCoord.xy / cell) - 0.5 + 1.0 / 128.0) * 0.2, 0.0, 1.0);\n" +
      "  vec3 best = floor(c * 5.0 + 0.5) / 5.0; vec3 d = best - c; float bd = dot(d, d);\n";
    ex.forEach(function (e) { s += "  { vec3 e = " + e + "; vec3 f = e - c; float fd = dot(f, f); if (fd < bd) { bd = fd; best = e; } }\n"; });
    return s + "  gl_FragColor = vec4(best, 1.0);\n}";
  }
  var pal = null;   // { cv, gl, tex, dither } while a 256-colour filter is chosen
  function makePal() {
    var cv = document.createElement("canvas");
    cv.className = "bv-pal";
    cv.setAttribute("aria-hidden", "true");
    var gl = null;
    try { gl = cv.getContext("webgl", { alpha: false, antialias: false, depth: false, stencil: false, premultipliedAlpha: false }); } catch (e) {}
    if (!gl) return null;
    function shader(type, src) {
      var sh = gl.createShader(type); gl.shaderSource(sh, src); gl.compileShader(sh);
      if (!gl.getShaderParameter(sh, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(sh));
      return sh;
    }
    var prog = gl.createProgram();
    try {
      gl.attachShader(prog, shader(gl.VERTEX_SHADER, VERT));
      gl.attachShader(prog, shader(gl.FRAGMENT_SHADER, fragment()));
    } catch (e) { console.warn("the 256-colour filter could not be made", e); return null; }
    gl.linkProgram(prog);
    gl.useProgram(prog);
    var buf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);
    var loc = gl.getAttribLocation(prog, "p");
    gl.enableVertexAttribArray(loc);
    gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);
    var tex = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
    return { cv: cv, gl: gl, dither: gl.getUniformLocation(prog, "dither"), cell: gl.getUniformLocation(prog, "cell"), box: "" };
  }
  // the overlay's place: the game canvas's own box in the stage (it is centred, not stretched, in full screen)
  function placePal() {
    if (!pal || !canvas) return;
    var box = canvas.offsetLeft + "," + canvas.offsetTop + "," + canvas.offsetWidth + "," + canvas.offsetHeight + "," +
      getComputedStyle(canvas).imageRendering;
    if (box === pal.box) return;
    pal.box = box;
    var b = box.split(","), st = pal.cv.style;
    st.left = b[0] + "px"; st.top = b[1] + "px"; st.width = b[2] + "px"; st.height = b[3] + "px"; st.imageRendering = b[4];
  }
  function drawPal() {
    if (!pal || !canvas || !canvas.width) return;
    var gl = pal.gl;
    if (pal.cv.width !== canvas.width || pal.cv.height !== canvas.height) {
      pal.cv.width = canvas.width; pal.cv.height = canvas.height;
      gl.viewport(0, 0, canvas.width, canvas.height);
      // the game's own width, from the shape the page gave the stage (web/shell.html, onGameScreen); 640 until a game gives one
      var gameW = parseFloat(stage.style.getPropertyValue("--aspect")) || 640;
      gl.uniform1f(pal.cell, Math.max(1, canvas.width / gameW));
    }
    try { gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, canvas); } catch (e) { return; }
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    placePal();
    pal.cv.style.visibility = "";
  }
  var palHooked = false;
  function hookPal() {
    var M = window.Module;
    if (palHooked || !M) return;
    palHooked = true;
    var prev = M.byopopTestFrame;
    M.byopopTestFrame = function (drew, ms) { if (prev) prev(drew, ms); if (pal && drew) drawPal(); };
  }
  function syncPal(v) {
    if (!(v in PAL)) {
      if (pal) { pal.cv.remove(); try { pal.gl.getExtension("WEBGL_lose_context").loseContext(); } catch (e) {} pal = null; }
      return;
    }
    if (!pal) {
      pal = makePal();
      if (!pal) return;
      // unseen until the game next draws: the game's canvas keeps no picture between its frames to copy now
      pal.cv.style.visibility = "hidden";
      stage.insertBefore(pal.cv, canvas.nextSibling);
      hookPal();
      if (window.ResizeObserver && !resizePal) { resizePal = new ResizeObserver(function () { placePal(); }); resizePal.observe(stage); resizePal.observe(canvas); }
    }
    pal.gl.uniform1f(pal.dither, PAL[v]);
    placePal();
  }
  var resizePal = null;

  function syncFilter() {
    var v = setting("colorFilter");
    if (v in SIMULATED) colourBlindFilters();
    canvas.style.filter = FILTERS[v] || "";
    syncPal(v);
  }

  var STYLE = ".bv-scan { position: absolute; left: 0; top: 0; width: 100%; height: 100%; pointer-events: none; z-index: 1;" +
    " background-image: linear-gradient(to bottom, transparent 50%, rgba(0, 0, 0, var(--bv-scan, 0.22)) 50%); background-repeat: repeat; }" +
    ".bv-pal { position: absolute; pointer-events: none; z-index: 1; display: block; }";
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
  // a change no observer above sees: the canvas moved in a stage of the same size, or the page's zoom
  window.addEventListener("resize", sizeScan);
  document.addEventListener("fullscreenchange", sizeScan);
  return { setGame: setGame, sync: sync };
})();
