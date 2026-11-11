// A game page's view settings (docs/WEB_PLAYER.md, "View settings"; web/settings.js): what the page draws over or does to the
// game's picture, never what the game draws or how it plays.
//   scanlines    off, light or strong: a dark line along every second row of the game's own pixels, over the picture
//   colorFilter  none, greyscale, sepia, warm (less blue light), contrast or vivid: a CSS filter on the game's canvas; or
//                256 colours, plain or dithered: a WebGL canvas over the game's (below)
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
      stage.insertBefore(scan, (pal ? pal.cv : canvas).nextSibling);   // over the 256-colour picture, if there is one
      if (window.ResizeObserver) { resizeObs = new ResizeObserver(sizeScan); resizeObs.observe(stage); resizeObs.observe(canvas); }
    }
    scan.style.setProperty("--bv-scan", String(SCAN[v]));
    sizeScan();
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
    canvas.style.filter = FILTERS[v] || "";
    syncPal(v);
  }

  var STYLE = ".bv-scan { position: absolute; inset: 0; pointer-events: none; z-index: 1;" +
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
  return { setGame: setGame, sync: sync };
})();
