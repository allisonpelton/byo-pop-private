// The player-only page's test tools (docs/WEB_PLAYER.md, "The player-only page"): window.byopop.test, for sessions that test a
// game in the Browser pane. web/shell.html loads this file only at ?player=1 on a development host (web/terms.js, devHost), right
// after its own script and before the game's, so that Module's hooks are in place when the runtime starts; on any other page, or
// any other host, it is never loaded, and it checks both again itself. Nothing here changes play: the input goes through the
// same DOM events and the same SDL path as a player's, the hold on the updates (byopop_hold_updates_at) only stops the clock
// between updates, and every reading is a copy.
(function () {
  "use strict";
  var shell = window.ByopopShell, M = window.Module;
  if (!shell || !M || typeof ByopopTerms === "undefined" || !ByopopTerms.devHost(location)) return;
  var params = new URLSearchParams(location.search);
  var canvas = M.canvas;

  // ---------- the log and the errors ----------
  // Every line the game prints (print and printErr), at most LOG_MAX of them; the errors: an abort, an uncaught exception or
  // rejection, a line that reports an error (Lua's among them) and every message box, which is answered at once (an alert is
  // dismissed, a confirm answered with byopop.test.confirmAnswer, true by default) and never shown.
  // Each line has its number (seq, from 0, kept when the oldest lines go), so that a driver can wait for a line after a given one.
  var LOG_MAX = 20000, log = [], errors = [], logSeq = 0, logWaits = [];
  function addLog(kind, text) {
    var line = { seq: logSeq++, kind: kind, text: String(text), update: updates() };
    log.push(line);
    if (log.length > LOG_MAX) log.shift();
    for (var i = 0; i < logWaits.length; i++) {
      if (logWaits[i].re.test(line.text)) { clearTimeout(logWaits[i].timer); logWaits[i].resolve(line); logWaits.splice(i--, 1); }
    }
  }
  function addError(kind, text) { errors.push({ kind: kind, text: String(text), update: updates(), at: performance.now() }); }
  var ERROR_LINE = /\b(error|fatal|assert(ion)? failed|exception|lua:|stack traceback)\b/i;
  var print = M.print, printErr = M.printErr, onAbort = M.onAbort;
  M.print = function (t) { addLog("out", t); if (ERROR_LINE.test(t)) addError("log", t); return print.apply(this, arguments); };
  M.printErr = function (t) { addLog("err", t); if (ERROR_LINE.test(t)) addError("log", t); return printErr.apply(this, arguments); };
  M.onAbort = function (what) { addError("abort", what); return onAbort.apply(this, arguments); };
  window.addEventListener("error", function (e) { addError("exception", (e.error && e.error.stack) || e.message); });
  window.addEventListener("unhandledrejection", function (e) { addError("rejection", (e.reason && e.reason.stack) || e.reason); });
  var test = {};
  test.confirmAnswer = true;
  window.alert = function (t) { addError("messagebox", t); addLog("box", t); };
  window.confirm = function (t) { addError("messagebox", t); addLog("box", t); return !!test.confirmAnswer; };

  // ---------- time ----------
  // The frames of a hidden pane or page (below)
  // (reallyHidden: the real document.hidden, not the one byopop.test.blur() pretends)
  var hiddenGet = Object.getOwnPropertyDescriptor(Document.prototype, "hidden").get;
  var stateGet = Object.getOwnPropertyDescriptor(Document.prototype, "visibilityState").get;
  function reallyHidden() { return hiddenGet.call(document); }
  var rAF = window.requestAnimationFrame.bind(window), cAF = window.cancelAnimationFrame.bind(window);
  var timers = {}, nextId = 1e9;
  // A hidden Browser pane sends no frames while document.hidden stays false, so each frame is whichever comes first, the
  // browser's own or a timer's after FALLBACK_MS (16 ms when the page is really hidden); where frames come, the browser's nearly always wins.
  var FALLBACK_MS = 20;
  window.requestAnimationFrame = function (cb) {
    var id = nextId++, done = false;
    function run(time) {
      if (done) return;
      done = true;
      var t = timers[id];
      delete timers[id];
      if (t) { cAF(t.raf); clearTimeout(t.timer); }
      cb(time);
    }
    timers[id] = { raf: rAF(run), timer: setTimeout(function () { run(performance.now()); }, reallyHidden() ? 16 : FALLBACK_MS) };
    return id;
  };
  window.cancelAnimationFrame = function (id) {
    var t = timers[id];
    if (t) { cAF(t.raf); clearTimeout(t.timer); delete timers[id]; } else cAF(id);
  };
  function call(name, args) {
    var f = M["_" + name];
    return typeof f === "function" ? f.apply(null, args || []) : undefined;
  }
  function updates() { var n = call("byopop_update_count"); return typeof n === "number" ? n : -1; }
  // promises waiting for a frame: {when(), resolve}
  var waiting = [];
  function afterFrame(when) { return new Promise(function (resolve) { waiting.push({ when: when, resolve: resolve }); }); }
  test.updates = updates;
  // holds the game between two updates: it is still drawn, and its input still arrives, but no update runs
  var holdAt = -1;
  function hold(n) { holdAt = n; call("byopop_hold_updates_at", [n]); }
  test.pause = function () { var n = updates(); hold(n); return n; };
  test.resume = function () { hold(-1); };
  // n more updates, then held again: resolves with the update count once they ran
  test.step = function (n) {
    var target = Math.max(updates(), 0) + Math.max(1, n | 0);
    hold(target);
    return afterFrame(function () { return updates() >= target; }).then(updates);
  };
  // resolves after n more updates, holding nothing
  test.wait = function (n) {
    var target = Math.max(updates(), 0) + Math.max(1, n | 0);
    return afterFrame(function () { return updates() >= target; }).then(updates);
  };
  test.waitUpdates = test.wait;
  test.frame = function () { return afterFrame(function () { return true; }); };

  // ---------- ready and the frame hooks ----------
  var frames = 0, resolveReady, shotsDue = [];
  test.ready = new Promise(function (resolve) { resolveReady = resolve; });
  test.progress = function () { return { status: shell.status, state: shell.state, frames: frames, updates: updates() }; };
  // after each browser frame's updates and draw (framework/SexyAppBase.cpp, EmscriptenMainLoop)
  M.byopopTestFrame = function (drew) {
    if (drew) frames++;
    if (drew && frames === 1) resolveReady({ updates: updates(), screen: shell.screen });
    // the shots, read after the frame (the canvas keeps its buffer: below), so that a held game, which draws nothing, has them too
    while (shotsDue.length) { var s = shotsDue.shift(); try { s.resolve(grab()); } catch (e) { s.reject(e); } }
    for (var i = 0; i < waiting.length; i++) {
      if (waiting[i].when()) { waiting[i].resolve(); waiting.splice(i--, 1); }
    }
  };
  test.frames = function () { return frames; };

  // ---------- shots ----------
  // The game's WebGL context keeps its drawing buffer on this page (preserveDrawingBuffer), so that the canvas can be read back
  // between the frames it draws: the pixels drawn are the same.
  var getContext = HTMLCanvasElement.prototype.getContext;
  HTMLCanvasElement.prototype.getContext = function (type, attrs) {
    if (this === canvas && /^(webgl2?|experimental-webgl)$/.test(type)) attrs = Object.assign({}, attrs, { preserveDrawingBuffer: true });
    return getContext.call(this, type, attrs);
  };
  // The canvas as it was drawn, at the game's own resolution (the page keeps one canvas pixel for each game pixel); when the
  // canvas is another size (a game drawn larger), it is scaled to the game's size without smoothing.
  function grab() {
    var w = shell.screen.w, h = shell.screen.h, src = canvas;
    if (canvas.width !== w || canvas.height !== h) {
      src = document.createElement("canvas");
      src.width = w; src.height = h;
      var g = src.getContext("2d");
      g.imageSmoothingEnabled = false;
      g.drawImage(canvas, 0, 0, w, h);
    }
    return src.toDataURL("image/png");
  }
  // shot() resolves with a PNG data URL; shot(name) also posts it to tools/webtest/serve.py, which saves it as
  // build/webshots/<run>/<name>.png (run: ?run=, or "default"), and resolves with the server's answer
  test.shot = function (name) {
    var p = new Promise(function (resolve, reject) { shotsDue.push({ resolve: resolve, reject: reject }); });
    if (!name) return p;
    return p.then(function (url) {
      return fetch("/__shot", { method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ run: params.get("run") || "default", name: name, png: url }) }).then(function (r) { return r.json(); });
    });
  };

  // ---------- input ----------
  // At game coordinates: x and y are the game's pixels, turned into the canvas's place on the page. The events are the DOM's own,
  // sent to the canvas as a player's would be, so that Emscripten and SDL take them on the same path.
  function at(x, y) {
    var r = canvas.getBoundingClientRect();
    return { clientX: r.left + (x + 0.5) * r.width / shell.screen.w, clientY: r.top + (y + 0.5) * r.height / shell.screen.h };
  }
  var BUTTONS = { left: 0, middle: 1, right: 2 }, MASK = [1, 4, 2], held = 0, mouseAt = { x: 0, y: 0 };
  function mouse(type, x, y, button) {
    var p = at(x, y), b = BUTTONS[button || "left"] || 0;
    if (type === "mousedown") held |= MASK[b];
    if (type === "mouseup") held &= ~MASK[b];
    mouseAt = { x: x, y: y };
    var e = new MouseEvent(type, { bubbles: true, cancelable: true, view: window, clientX: p.clientX, clientY: p.clientY,
      screenX: p.clientX, screenY: p.clientY, button: b, buttons: held, detail: 1 });
    canvas.dispatchEvent(e);
  }
  test.move = function (x, y) { mouse("mousemove", x, y); return test.frame(); };
  test.down = function (x, y, button) { mouse("mousemove", x, y); mouse("mousedown", x, y, button); return test.frame(); };
  test.up = function (x, y, button) {
    if (x === undefined) { x = mouseAt.x; y = mouseAt.y; }
    mouse("mousemove", x, y); mouse("mouseup", x, y, button); return test.frame();
  };
  // a click: down and up on separate frames, as a real click reaches the game
  test.click = function (x, y, button) { return test.down(x, y, button).then(function () { return test.up(x, y, button); }); };
  // a drag in steps (default 10), one frame each
  test.drag = function (x0, y0, x1, y1, steps, button) {
    steps = Math.max(1, steps | 0 || 10);
    var p = test.down(x0, y0, button);
    for (var i = 1; i <= steps; i++) (function (i) {
      p = p.then(function () { return test.move(Math.round(x0 + (x1 - x0) * i / steps), Math.round(y0 + (y1 - y0) * i / steps)); });
    })(i);
    return p.then(function () { return test.up(x1, y1, button); });
  };
  test.wheel = function (x, y, dy) {
    var p = at(x, y);
    canvas.dispatchEvent(new WheelEvent("wheel", { bubbles: true, cancelable: true, clientX: p.clientX, clientY: p.clientY, deltaY: dy, deltaMode: 0 }));
    return test.frame();
  };
  // keys by the DOM's names ("Enter", "Escape", "ArrowLeft", " ", "a", "F1"), with keyCode and code as a browser sets them
  var NAMED = { Backspace: [8, "Backspace"], Tab: [9, "Tab"], Enter: [13, "Enter"], Shift: [16, "ShiftLeft"], Control: [17, "ControlLeft"],
    Alt: [18, "AltLeft"], Pause: [19, "Pause"], Escape: [27, "Escape"], " ": [32, "Space"], PageUp: [33, "PageUp"], PageDown: [34, "PageDown"],
    End: [35, "End"], Home: [36, "Home"], ArrowLeft: [37, "ArrowLeft"], ArrowUp: [38, "ArrowUp"], ArrowRight: [39, "ArrowRight"],
    ArrowDown: [40, "ArrowDown"], Insert: [45, "Insert"], Delete: [46, "Delete"] };
  for (var f = 1; f <= 12; f++) NAMED["F" + f] = [111 + f, "F" + f];
  NAMED.Space = NAMED[" "];
  function keyInfo(key) {
    if (NAMED[key]) return { key: key === "Space" ? " " : key, keyCode: NAMED[key][0], code: NAMED[key][1] };
    if (key.length === 1) {
      var up = key.toUpperCase(), c = up.charCodeAt(0);
      if (up >= "A" && up <= "Z") return { key: key, keyCode: c, code: "Key" + up };
      if (key >= "0" && key <= "9") return { key: key, keyCode: c, code: "Digit" + key };
      return { key: key, keyCode: 0, code: "" };
    }
    throw new Error("byopop.test: unknown key " + key);
  }
  function keyEvent(type, info, mods) {
    var e = new KeyboardEvent(type, { bubbles: true, cancelable: true, key: info.key, code: info.code,
      shiftKey: !!(mods && mods.shift), ctrlKey: !!(mods && mods.ctrl), altKey: !!(mods && mods.alt) });
    var charCode = type === "keypress" ? info.key.charCodeAt(0) : 0;
    var which = type === "keypress" ? charCode : info.keyCode;
    Object.defineProperty(e, "keyCode", { get: function () { return type === "keypress" ? charCode : info.keyCode; } });
    Object.defineProperty(e, "charCode", { get: function () { return charCode; } });
    Object.defineProperty(e, "which", { get: function () { return which; } });
    canvas.dispatchEvent(e);
  }
  test.keyDown = function (key, mods) { keyEvent("keydown", keyInfo(key), mods); return test.frame(); };
  test.keyUp = function (key, mods) { keyEvent("keyup", keyInfo(key), mods); return test.frame(); };
  // a press: down (with its keypress where it types a character) and up on the next frame
  test.key = function (key, mods) {
    var info = keyInfo(key);
    keyEvent("keydown", info, mods);
    if (info.key.length === 1) keyEvent("keypress", info, mods);
    return test.frame().then(function () { keyEvent("keyup", info, mods); return test.frame(); });
  };
  test.type = function (text) {
    return Array.prototype.reduce.call(String(text), function (p, ch) {
      return p.then(function () { return test.key(ch === "\n" ? "Enter" : ch, ch !== ch.toLowerCase() ? { shift: true } : null); });
    }, Promise.resolve());
  };

  // ---------- focus ----------
  // What a real loss of the focus sends: blur on the window and the canvas, and for a hidden tab visibilitychange with
  // document.hidden true, until focus() gives it back
  var pretendHidden = false;
  Object.defineProperty(document, "hidden", { configurable: true, get: function () { return pretendHidden || hiddenGet.call(document); } });
  Object.defineProperty(document, "visibilityState", { configurable: true, get: function () { return pretendHidden ? "hidden" : stateGet.call(document); } });
  var hasFocus = document.hasFocus.bind(document), focusLost = false;
  document.hasFocus = function () { return !focusLost && hasFocus(); };
  test.blur = function (opts) {
    focusLost = true;
    canvas.dispatchEvent(new FocusEvent("blur"));
    window.dispatchEvent(new FocusEvent("blur"));
    if (opts && opts.hidden) { pretendHidden = true; document.dispatchEvent(new Event("visibilitychange")); }
    return test.frame();
  };
  test.focus = function () {
    focusLost = false;
    if (pretendHidden) { pretendHidden = false; document.dispatchEvent(new Event("visibilitychange")); }
    window.dispatchEvent(new FocusEvent("focus"));
    canvas.dispatchEvent(new FocusEvent("focus"));
    return test.frame();
  };

  // ---------- readings ----------
  // the lines from number since on (all that are kept, without it)
  test.log = function (since) { since = since || 0; return log.filter(function (l) { return l.seq >= since; }); };
  // the number the next line will have
  test.logNext = function () { return logSeq; };
  // resolves with the first line from number since on (default: the next line printed) that matches pattern (a RegExp or a
  // string it contains); rejects after timeout ms (default 60000; 0 waits for ever)
  test.waitLog = function (pattern, opts) {
    var re = pattern instanceof RegExp ? pattern : new RegExp(String(pattern).replace(/[.*+?^${}()|[\]\\]/g, "\\$&"));
    var since = opts && opts.since !== undefined ? opts.since : logSeq, timeout = opts && opts.timeout !== undefined ? opts.timeout : 60000;
    var old = log.filter(function (l) { return l.seq >= since && re.test(l.text); })[0];
    if (old) return Promise.resolve(old);
    return new Promise(function (resolve, reject) {
      var w = { re: re, resolve: resolve, timer: 0 };
      if (timeout > 0) w.timer = setTimeout(function () {
        var i = logWaits.indexOf(w);
        if (i >= 0) logWaits.splice(i, 1);
        reject(new Error("byopop.test.waitLog: no line matched " + re + " in " + timeout + " ms"));
      }, timeout);
      logWaits.push(w);
    });
  };
  test.errors = errors;
  // the ports' watch values, where a port gives them (byopop_watch_json, a JSON object); null where it does not
  test.watch = function () {
    if (typeof M._byopop_watch_json !== "function") return null;
    try { return JSON.parse(M.UTF8ToString(M._byopop_watch_json())); } catch (e) { return null; }
  };
  // the mixer: waits for the shared session's platform/Audio.cpp (the byopop_mixer_json call), null until then
  test.mixer = function () {
    if (typeof M._byopop_mixer_json !== "function") return null;
    try { return JSON.parse(M.UTF8ToString(M._byopop_mixer_json())); } catch (e) { return null; }
  };
  test.screen = function () { return shell.screen; };
  // The standard test commands (docs/TEST_COMMANDS.md: state, goto <screen>, level <n>, unlock all, set <name> <value>, win,
  // lose, seed <n>, move), through the framework's live channel (byopop_test_command, which queues the line for the next
  // update and the -script dispatch). Resolves with the lines printed from the call up to the answer, STATE {json},
  // MOVE {json}|null, OK <command> or ERROR <command>: <reason> (the last of them); rejects after timeout ms (default 10000), or at once where the build has no
  // channel. A held game is stepped one update, so that the command runs.
  var ANSWER = /^(INFO: )?(STATE |MOVE |OK |ERROR )/;
  test.command = function (line, opts) {
    if (typeof M._byopop_test_command !== "function") return Promise.reject(new Error("byopop.test.command: the build has no byopop_test_command"));
    var since = logSeq;
    var answer = test.waitLog(ANSWER, { since: since, timeout: opts && opts.timeout !== undefined ? opts.timeout : 10000 });
    var ok = M.ccall("byopop_test_command", "number", ["string"], [String(line)]);
    if (!ok) return Promise.reject(new Error("byopop.test.command: refused (no game yet, or the queue is full)"));
    if (holdAt >= 0) test.step(1);
    return answer.then(function (last) {
      return test.log(since).filter(function (l) { return l.seq <= last.seq; }).map(function (l) { return l.text.replace(/^INFO: /, ""); });
    });
  };
  // the game's state, parsed: command("state")'s STATE line
  test.state = function () {
    return test.command("state").then(function (lines) {
      var last = lines[lines.length - 1];
      if (!/^STATE /.test(last)) throw new Error("byopop.test.state: " + last);
      return JSON.parse(last.substring(6));
    });
  };

  // the move the port's own bot would make next, parsed: command("move")'s MOVE line, null when it has none
  test.botMove = function () {
    return test.command("move").then(function (lines) {
      var last = lines[lines.length - 1];
      if (!/^MOVE /.test(last)) throw new Error("byopop.test.botMove: " + last);
      return JSON.parse(last.substring(5));
    });
  };
  // Plays one MOVE through real DOM input, in game coordinates: its "input" list in order where it has one, else its clicks,
  // then its drags, then its keys; then waits its "updates" (1 by default)
  test.playMove = function (m) {
    var steps = [];
    if (Array.isArray(m.input)) m.input.forEach(function (i) { steps.push(i); });
    else {
      (m.clicks || []).forEach(function (c) { steps.push({ click: c }); });
      (m.drags || []).forEach(function (d) { steps.push({ drag: d }); });
      (m.keys || []).forEach(function (k) { steps.push({ key: k }); });
    }
    var p = Promise.resolve();
    steps.forEach(function (st) {
      p = p.then(function () {
        // the step forms of docs/TEST_COMMANDS.md ([x, y] points), and {x, y} points as the first drivers wrote them
        function pt(v) { return Array.isArray(v) ? { x: v[0], y: v[1] } : v; }
        var btn = st.button || (st.click && st.click.button) || "left", a = pt(st.click || st.move || st.press || st.release);
        if (st.click) return test.click(a.x, a.y, btn);
        if (st.move) return test.move(a.x, a.y);
        if (st.press) return test.down(a.x, a.y, btn);
        if (st.release) return test.up(a.x, a.y, btn);
        if (st.drag) return test.drag(st.drag.x0, st.drag.y0, st.drag.x1, st.drag.y1, st.drag.steps || 4);
        if (st.key) return test.key(st.key);
        if (st.wait !== undefined) return test.wait(st.wait);
      });
    });
    return p.then(function () { return test.wait(m.updates > 0 ? m.updates : 1); });
  };
  // The built-in driver ?driver=move (docs/TEST_COMMANDS.md, "The bot behind move"): plays the game by the port's own bot. Each
  // step asks for the state and the move and sends the move as real input. It stops when the state says "over", on MOVE null twice
  // running, an ERROR, or after ?steps= moves (2000 by default). Returns { moves, stop, state, updates, errors }.
  test.playByMove = function (opts) {
    var limit = (opts && opts.steps) || Number(params.get("steps")) || 2000;
    var moves = 0, nulls = 0, st = null;
    function step() {
      return test.state().then(function (s) {
        st = s;
        if (s && s.over) return "over";
        if (moves >= limit) return "limit";
        return test.botMove().then(function (m) {
          if (m === null) {
            if (++nulls >= 2) return "no move";
            return test.wait(30).then(step);
          }
          nulls = 0;
          moves++;
          return test.playMove(m).then(step);
        });
      });
    }
    return step().catch(function (e) { return "error: " + String(e && e.message || e); }).then(function (stop) {
      return { moves: moves, stop: stop, state: st, updates: test.updates(), errors: test.errors.length };
    });
  };

  // ---------- the test profile and the staged files ----------
  // ?assets=<name> (or ?assets=1 for the page's own id): the game's files from tools/webtest/serve.py, which serves the
  // staged assets/<name>/ at /__assets/<name>/, through the page's own import. Only the files the import picks are fetched.
  var assetsName = params.get("assets");
  if (assetsName === "1" || assetsName === "") assetsName = shell.PAGE.id;
  if (shell.testProfile) shell.hooks.needFiles = function () {
    var base = "/__assets/" + encodeURIComponent(assetsName) + "/";
    return fetch(base + "__list", { cache: "no-store" }).then(function (r) {
      if (!r.ok) throw new Error("no staged files at " + base + " (" + r.status + ")");
      return r.json();
    }).then(function (list) {
      var entries = list.map(function (rel) {
        return { path: "assets/" + rel, read: function () {
          return fetch(base + rel.split("/").map(encodeURIComponent).join("/")).then(function (r) {
            if (!r.ok) throw new Error(rel + ": " + r.status);
            return r.arrayBuffer();
          }).then(function (b) { return new Uint8Array(b); });
        } };
      });
      return shell.importGame(entries);
    }).catch(function (e) { addError("assets", e && e.message || e); return false; });
  };
  function underUser(rel) {
    var parts = String(rel).split("/").filter(function (p) { return p && p !== "." && p !== ".."; });
    return shell.USER + "/" + parts.join("/");
  }
  function rmTree(fs, dir) {
    var names; try { names = fs.readdir(dir); } catch (e) { return; }
    names.forEach(function (n) {
      if (n === "." || n === "..") return;
      var p = dir + "/" + n;
      if (fs.isDir(fs.stat(p).mode)) { rmTree(fs, p); fs.rmdir(p); } else fs.unlink(p);
    });
  }
  test.profile = {
    // the test profile's saves, all gone (the staged game files stay); only before the game starts, then reload
    wipe: function () {
      if (!shell.testProfile) return Promise.reject(new Error("byopop.test.profile: only with ?assets= (the test profile)"));
      rmTree(M.FS, shell.USER);
      return shell.sync();
    },
    // files written into the test profile's saves: {"<path under the user dir>": string, ArrayBuffer or Uint8Array}
    seed: function (files) {
      if (!shell.testProfile) return Promise.reject(new Error("byopop.test.profile: only with ?assets= (the test profile)"));
      Object.keys(files).forEach(function (rel) {
        var path = underUser(rel), v = files[rel];
        M.FS.mkdirTree(path.substring(0, path.lastIndexOf("/")));
        M.FS.writeFile(path, typeof v === "string" ? v : new Uint8Array(v.buffer || v));
      });
      return shell.sync();
    },
    // the files of the saves, by path under the user dir
    list: function () {
      var out = [];
      (function walk(dir, rel) {
        var names; try { names = M.FS.readdir(dir); } catch (e) { return; }
        names.forEach(function (n) {
          if (n === "." || n === "..") return;
          var st = M.FS.stat(dir + "/" + n);
          if (M.FS.isDir(st.mode)) walk(dir + "/" + n, rel + n + "/"); else out.push(rel + n);
        });
      })(shell.USER, "");
      return out;
    },
    read: function (rel) { return M.FS.readFile(underUser(rel)); }
  };

  // the start: ?hold=1 waits for byopop.test.start(), so that a test may wipe or seed the profile first; otherwise the game
  // starts as soon as it is ready. ?fast=1 runs it on its virtual clock (-fast).
  if (params.get("fast") === "1") shell.hooks.args = function () { return ["-fast"]; };
  var startNow = params.get("hold") !== "1", canStart = false;
  shell.hooks.ready = function () { canStart = true; if (startNow) shell.start(); };
  test.start = function () {
    startNow = true;
    if (canStart && shell.state === "waiting") shell.start();
    return test.ready;
  };

  // ?driver=move: the built-in driver that plays by the port's bot (playByMove). ?driver=<path>: a session's driver script, games/<game>/tools/<...>.js in the repo (served by tools/webtest/serve.py at
  // /__repo/), run once the game is ready, as the body of an async function of t (byopop.test): it may await, and what it returns
  // is byopop.test.result. byopop.test.driver resolves with the same result (or rejects with the driver's error, which also goes
  // into errors); result is {error: message} after a failure.
  var driverPath = params.get("driver");
  test.result = undefined;
  if (driverPath === "move") {
    test.driver = test.ready.then(function () { return test.playByMove(); });
    test.driver.then(function (v) { test.result = v; }, function (e) { addError("driver", (e && e.stack) || e); test.result = { error: String(e && e.message || e) }; });
  } else if (driverPath) {
    test.driver = !/^games\/[a-z0-9]+\/tools\/[A-Za-z0-9._\/-]+\.js$/.test(driverPath) || driverPath.indexOf("..") >= 0
      ? Promise.reject(new Error("byopop.test: a driver is games/<game>/tools/<path>.js"))
      : fetch("/__repo/" + driverPath, { cache: "no-store" }).then(function (r) {
        if (!r.ok) throw new Error("byopop.test: driver " + driverPath + ": " + r.status);
        return r.text();
      }).then(function (code) {
        var AsyncFunction = Object.getPrototypeOf(async function () {}).constructor;
        var run = new AsyncFunction("t", code + "\n//# sourceURL=" + location.origin + "/__repo/" + driverPath);
        return test.ready.then(function () { return run(test); });
      });
    test.driver.then(function (v) { test.result = v; },
      function (e) { addError("driver", (e && e.stack) || e); test.result = { error: String(e && e.message || e) }; });
  } else test.driver = null;

  window.byopop = window.byopop || {};
  window.byopop.test = test;
})();
