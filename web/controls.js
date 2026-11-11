// A game page's controls (docs/WEB_PLAYER.md, "Controls"): the Controls panel, which lists the game's controls in our own
// short words from web/controldata.js, and what the page adds to the game's input, never what the game does with it.
//   remapped keys  a key the player chose for one of the game's keys: the page sends the game's key in its place, as the
//                  browser would have sent it, so the game sees only its own key (the player's map is kept per game in the
//                  localStorage item "byopop-keys"; a game's own key still works unless the player gave it to another)
//   gamepad        (setting "gamepad", on by default) the left stick or the d-pad moves the mouse over the game, A clicks,
//                  B right-clicks, and the other buttons press the game's keys (X, Y, LB, RB, Back, Start in the order of
//                  the table, unless the player chose others); nothing runs until a gamepad is connected
//   touch keys     (setting "touchKeys": auto shows them on a touch screen) a button under the game for each of its keys
//   input display  (setting "inputDisplay") the keys and mouse buttons held, in the game's corner
//   game speed     (setting "speed", 1 by default) the framework's own update multiplier (byopop_set_speed): the game runs
//                  its updates that much faster or slower in real time; never on the player-only test page (?player=1),
//                  whose runs keep the game's own speed
// The events go to the canvas as a player's own do (web/testkit.js does the same for tests), so Emscripten and SDL take them
// on the same path.
var ByopopControls = (function () {
  "use strict";
  var MAPKEY = "byopop-keys";
  var pageGame = null, canvas = null, stage = null, ours = typeof WeakSet === "function" ? new WeakSet() : null;

  function setting(name) { try { return ByopopSettings.get(name, pageGame || undefined); } catch (e) { return null; } }
  function T(key) { try { return ByopopStrings.t(key); } catch (e) { return key; } }
  function table() { try { return (window.ByopopControlData && ByopopControlData[pageGame]) || []; } catch (e) { return []; } }
  function keyEntries() { return table().filter(function (c) { return c.keys && c.keys.length; }); }

  // ---------- the player's map ----------
  function loadAll() { try { return JSON.parse(localStorage.getItem(MAPKEY) || "{}") || {}; } catch (e) { return {}; } }
  function loadMap() {
    var m = loadAll()[pageGame] || {};
    return { keys: m.keys || {}, pad: m.pad || {} };
  }
  function saveMap(m) {
    var all = loadAll();
    if (Object.keys(m.keys).length || Object.keys(m.pad).length) all[pageGame] = m; else delete all[pageGame];
    try { localStorage.setItem(MAPKEY, JSON.stringify(all)); } catch (e) {}
  }
  var map = { keys: {}, pad: {} };

  // ---------- keys as a browser sends them ----------
  var NAMED = { Backspace: [8, "Backspace"], Tab: [9, "Tab"], Enter: [13, "Enter"], NumpadEnter: [13, "Enter"], ShiftLeft: [16, "Shift"],
    ShiftRight: [16, "Shift"], ControlLeft: [17, "Control"], ControlRight: [17, "Control"], AltLeft: [18, "Alt"], AltRight: [18, "Alt"],
    Pause: [19, "Pause"], Escape: [27, "Escape"], Space: [32, " "], PageUp: [33, "PageUp"], PageDown: [34, "PageDown"], End: [35, "End"],
    Home: [36, "Home"], ArrowLeft: [37, "ArrowLeft"], ArrowUp: [38, "ArrowUp"], ArrowRight: [39, "ArrowRight"], ArrowDown: [40, "ArrowDown"],
    Insert: [45, "Insert"], Delete: [46, "Delete"], Minus: [189, "-"], Equal: [187, "="], BracketLeft: [219, "["], BracketRight: [221, "]"],
    Semicolon: [186, ";"], Quote: [222, "'"], Comma: [188, ","], Period: [190, "."], Slash: [191, "/"], Backslash: [220, "\\"], Backquote: [192, "`"] };
  for (var f = 1; f <= 12; f++) NAMED["F" + f] = [111 + f, "F" + f];
  function info(code) {
    if (NAMED[code]) return { code: code, keyCode: NAMED[code][0], key: NAMED[code][1] };
    var m = /^Key([A-Z])$/.exec(code);
    if (m) return { code: code, keyCode: m[1].charCodeAt(0), key: m[1].toLowerCase() };
    m = /^(?:Digit|Numpad)([0-9])$/.exec(code);
    if (m) return { code: code, keyCode: (code[0] === "N" ? 96 : 48) + +m[1], key: m[1] };
    return null;
  }
  // a key's name for the panel and the input display
  function keyName(code) {
    var NAMES = { Space: "Space", Escape: "Esc", Enter: "Enter", Backspace: "Backspace", Tab: "Tab", ArrowLeft: "←", ArrowUp: "↑",
      ArrowRight: "→", ArrowDown: "↓", ShiftLeft: "Shift", ShiftRight: "Shift", ControlLeft: "Ctrl", ControlRight: "Ctrl",
      AltLeft: "Alt", AltRight: "Alt", PageUp: "PgUp", PageDown: "PgDn", Delete: "Del", Insert: "Ins" };
    if (NAMES[code]) return NAMES[code];
    var i = info(code);
    if (i && i.key.length === 1) return i.key.toUpperCase();
    return i ? i.key : code;
  }
  function sendKey(type, code, like) {
    var i = info(code);
    if (!i || !canvas) return;
    var e = new KeyboardEvent(type, { bubbles: true, cancelable: true, key: i.key, code: i.code,
      shiftKey: !!(like && like.shiftKey), ctrlKey: !!(like && like.ctrlKey), altKey: !!(like && like.altKey), repeat: !!(like && like.repeat) });
    var printable = i.key.length === 1, charCode = type === "keypress" ? i.key.charCodeAt(0) : 0;
    Object.defineProperty(e, "keyCode", { get: function () { return type === "keypress" ? charCode : i.keyCode; } });
    Object.defineProperty(e, "charCode", { get: function () { return charCode; } });
    Object.defineProperty(e, "which", { get: function () { return type === "keypress" ? charCode : i.keyCode; } });
    if (ours) ours.add(e); else e.byopopOurs = true;
    canvas.dispatchEvent(e);
    if (type === "keydown" && printable) sendKey("keypress", code, like);
  }
  function isOurs(e) { return ours ? ours.has(e) : !!e.byopopOurs; }

  // a remapped key: the player's key stands for the game's, on the way to the canvas (before Emscripten's own listener)
  function onKey(e) {
    if (isOurs(e) || capturing) return;
    var to = map.keys[e.code];
    if (!to || to === e.code) return;
    e.stopImmediatePropagation();
    e.preventDefault();
    if (e.type !== "keypress") sendKey(e.type, to, e);
  }

  // ---------- the mouse, for the gamepad ----------
  var pointer = { x: 0.5, y: 0.5 }, mouseHeld = 0;
  function sendMouse(type, button) {
    if (!canvas) return;
    var r = canvas.getBoundingClientRect(), x = r.left + pointer.x * r.width, y = r.top + pointer.y * r.height;
    var MASK = [1, 4, 2];
    if (type === "mousedown") mouseHeld |= MASK[button || 0];
    if (type === "mouseup") mouseHeld &= ~MASK[button || 0];
    canvas.dispatchEvent(new MouseEvent(type, { bubbles: true, cancelable: true, view: window, clientX: x, clientY: y,
      screenX: x, screenY: y, button: button || 0, buttons: mouseHeld, detail: 1 }));
  }

  // ---------- the gamepad ----------
  // the standard mapping's buttons: 0 A, 1 B, 2 X, 3 Y, 4 LB, 5 RB, 8 Back, 9 Start, 12-15 the d-pad
  var PAD = [{ b: 2, name: "X" }, { b: 3, name: "Y" }, { b: 4, name: "LB" }, { b: 5, name: "RB" }, { b: 8, name: "Back" }, { b: 9, name: "Start" }];
  var padHeld = {}, padOn = false, lastPad = 0;
  function padKeyFor(button) {
    if (map.pad[button]) return map.pad[button];
    var entries = keyEntries(), i;
    for (i = 0; i < PAD.length; i++) if (PAD[i].b === +button) break;
    // a button the player gave to another entry is not also a default
    var e = entries[i];
    if (!e) return null;
    for (var k in map.pad) if (map.pad[k] === e.keys[0]) return null;
    return e.keys[0];
  }
  function padButton(n) {
    for (var i = 0; i < PAD.length; i++) if (PAD[i].b === n) return PAD[i];
    return null;
  }
  function pollPad(now) {
    if (!padOn || setting("gamepad") === "off" || !navigator.getGamepads) return;
    var pads = navigator.getGamepads(), dt = Math.min(0.1, Math.max(0, (now - (lastPad || now)) / 1000));
    lastPad = now;
    for (var p = 0; p < pads.length; p++) {
      var g = pads[p];
      if (!g || !g.connected) continue;
      var ax = g.axes[0] || 0, ay = g.axes[1] || 0, b = g.buttons;
      function down(n) { return !!(b[n] && b[n].pressed); }
      if (down(14)) ax = -1; else if (down(15)) ax = 1;
      if (down(12)) ay = -1; else if (down(13)) ay = 1;
      var dead = 0.18, mag = Math.sqrt(ax * ax + ay * ay);
      if (mag > dead) {
        // across the game in about a second and a half at full tilt, finer near the middle
        var s = 0.7 * dt * Math.pow((mag - dead) / (1 - dead), 1.6) / mag;
        pointer.x = Math.min(0.999, Math.max(0, pointer.x + ax * s));
        pointer.y = Math.min(0.999, Math.max(0, pointer.y + ay * s * (canvas.width / (canvas.height || 1))));
        sendMouse("mousemove");
      }
      for (var n = 0; n < b.length; n++) {
        var was = !!padHeld[n], is = down(n);
        if (was === is) continue;
        padHeld[n] = is;
        if (n === 0 || n === 1) { sendMouse("mousemove"); sendMouse(is ? "mousedown" : "mouseup", n === 0 ? 0 : 2); show(); continue; }
        if (!padButton(n)) continue;
        var code = padKeyFor(n);
        if (code) { sendKey(is ? "keydown" : "keyup", code); held[code] = is; show(); }
      }
      break;
    }
  }

  // ---------- the game speed ----------
  var testPage = /[?&]player=1(&|$)/.test(location.search), speedSet = 1;
  function speedWanted() {
    var v = testPage ? 1 : parseFloat(setting("speed"));
    return v > 0 ? v : 1;
  }
  function applySpeed() {
    var want = speedWanted(), M = window.Module;
    if (want === speedSet || !M || typeof M._byopop_set_speed !== "function") return;
    try {
      M._byopop_set_speed(want);
      // 0 until the game's application exists: tried again on the next frame
      if (M._byopop_speed() > 0) speedSet = want;
    } catch (e) {}
  }

  // ---------- the frame hook (the gamepad's poll and the speed, which the framework calls each frame) ----------
  var hooked = false;
  function hook() {
    if (hooked || !window.Module) return;
    hooked = true;
    var M = window.Module, prev = M.byopopTestFrame;
    M.byopopTestFrame = function (drew, ms) {
      if (prev) prev(drew, ms);
      if (speedSet !== speedWanted()) applySpeed();
      if (padOn) pollPad(performance.now());
    };
  }
  window.addEventListener("gamepadconnected", function () { padOn = true; hook(); });

  // ---------- the input display ----------
  var held = {}, display = null, shownText = "";
  function show() {
    if (!display) return;
    var parts = [];
    for (var c in held) if (held[c]) parts.push(keyName(c));
    if (mouseHeld & 1) parts.push(T("controls.display.left"));
    if (mouseHeld & 2) parts.push(T("controls.display.right"));
    if (mouseHeld & 4) parts.push(T("controls.display.middle"));
    var text = parts.join(" ");
    if (text === shownText) return;
    shownText = text;
    display.textContent = "";
    parts.forEach(function (p) { var s = document.createElement("span"); s.textContent = p; display.appendChild(s); });
    display.hidden = !parts.length;
  }
  function onSeen(e) {
    if (!display) return;
    if (e.type === "keydown" || e.type === "keyup") {
      if (map.keys[e.code] && !isOurs(e)) return;   // the remapped key's own event: its stand-in is shown
      held[e.code] = e.type === "keydown";
    } else if (e.type === "mousedown" || e.type === "mouseup") {
      if (!isOurs(e)) { var MASK = [1, 4, 2]; if (e.type === "mousedown") mouseHeld |= MASK[e.button] || 0; else mouseHeld &= ~(MASK[e.button] || 0); }
    } else if (e.type === "blur") { held = {}; mouseHeld = 0; }
    show();
  }
  function syncDisplay() {
    if (setting("inputDisplay") !== "on") { if (display) { display.remove(); display = null; shownText = ""; } return; }
    if (!display) {
      display = document.createElement("div");
      display.className = "bc-display";
      display.setAttribute("aria-hidden", "true");
      display.hidden = true;
      stage.appendChild(display);
    }
  }

  // ---------- touch keys ----------
  var touchBar = null;
  function touchWanted() {
    var v = setting("touchKeys");
    if (v === "on") return true;
    if (v === "off") return false;
    try { return window.matchMedia("(pointer: coarse)").matches; } catch (e) { return false; }
  }
  // Where the bar is (setting "touchPlace"): under the game, at its side, or automatic: at the side while the screen is wider
  // than tall (the user, 2026-11-10: "When in landscape mode on mobile, the mobile controls completely obscure the game. An
  // option to put them to the side of the game and have them show in full screen is needed"). The page lays it out by the class
  // on <html> (touch-side) and leaves it room in full screen (web/shell.html, fitFullscreen).
  function touchSide() {
    var v = setting("touchPlace");
    if (v === "side") return true;
    if (v === "under") return false;
    return window.innerWidth > window.innerHeight;
  }
  function placeTouch() {
    var side = !!touchBar && touchSide(), root = document.documentElement;
    if (root.classList.contains("touch-side") === side && root.classList.contains("touch-keys") === !!touchBar) return;
    root.classList.toggle("touch-keys", !!touchBar);
    root.classList.toggle("touch-side", side);
    try { window.dispatchEvent(new CustomEvent("byopop-touch-keys")); } catch (e) {}
  }
  // The screen keyboard, for a game's own text fields (a player's name): a touch screen shows its keyboard only for a text field
  // of the page, so the bar's keyboard button puts the focus in one, kept out of sight, and what is typed there goes to the game
  // as keys (the user, 2026-11-10: "The mobile keyboard did not pop up for me to type my username")
  var typeField = null;
  function sendTyped(key, keyCode) {
    if (!canvas) return;
    ["keydown", "keypress", "keyup"].forEach(function (type) {
      var printable = key.length === 1;
      if (type === "keypress" && !printable && key !== "Enter") return;
      var charCode = type === "keypress" ? (printable ? key.charCodeAt(0) : 13) : 0;
      var e = new KeyboardEvent(type, { bubbles: true, cancelable: true, key: key, code: "" });
      Object.defineProperty(e, "keyCode", { get: function () { return type === "keypress" ? charCode : keyCode; } });
      Object.defineProperty(e, "charCode", { get: function () { return charCode; } });
      Object.defineProperty(e, "which", { get: function () { return type === "keypress" ? charCode : keyCode; } });
      if (ours) ours.add(e); else e.byopopOurs = true;
      canvas.dispatchEvent(e);
    });
  }
  function keyCodeOf(ch) {
    var u = ch.toUpperCase().charCodeAt(0);
    if ((u >= 48 && u <= 57) || (u >= 65 && u <= 90)) return u;
    return ch === " " ? 32 : 0;
  }
  function typeButton() {
    var b = document.createElement("button");
    b.type = "button";
    b.className = "bc-keyboard";
    b.setAttribute("aria-label", T("controls.touch.keyboard"));
    b.title = T("controls.touch.keyboard");
    // a keyboard, drawn here: its case and three rows of keys
    b.innerHTML = '<svg viewBox="0 0 24 16" width="24" height="16" aria-hidden="true" focusable="false" shape-rendering="crispEdges">' +
      '<rect x="0.5" y="0.5" width="23" height="15" fill="none" stroke="currentColor"/>' +
      '<path fill="currentColor" d="M3 3h2v2H3zM7 3h2v2H7zM11 3h2v2h-2zM15 3h2v2h-2zM19 3h2v2h-2zM3 7h2v2H3zM7 7h2v2H7zM11 7h2v2h-2zM15 7h2v2h-2zM19 7h2v2h-2zM6 11h12v2H6z"/></svg>';
    typeField = document.createElement("input");
    typeField.type = "text";
    typeField.className = "bc-typefield";
    typeField.setAttribute("aria-label", T("controls.touch.keyboard"));
    typeField.autocomplete = "off"; typeField.autocapitalize = "off"; typeField.spellcheck = false;
    typeField.setAttribute("autocorrect", "off");
    typeField.tabIndex = -1;
    var field = typeField;
    // one space kept in it, so that Backspace always has something to delete and so sends an event
    function rest() { field.value = " "; try { field.setSelectionRange(1, 1); } catch (e) {} }
    field.addEventListener("beforeinput", function (ev) {
      var t = ev.inputType || "";
      if (t === "deleteContentBackward") { ev.preventDefault(); sendTyped("Backspace", 8); rest(); }
      else if (t === "insertLineBreak" || t === "insertParagraph") { ev.preventDefault(); sendTyped("Enter", 13); }
    });
    field.addEventListener("input", function () {
      var text = field.value.replace(/^ /, "");
      for (var i = 0; i < text.length; i++) sendTyped(text.charAt(i), keyCodeOf(text.charAt(i)));
      rest();
    });
    field.addEventListener("keydown", function (ev) { if (ev.key === "Enter") { ev.preventDefault(); sendTyped("Enter", 13); } });
    field.addEventListener("focus", function () { rest(); b.setAttribute("aria-pressed", "true"); });
    field.addEventListener("blur", function () { b.setAttribute("aria-pressed", "false"); });
    b.setAttribute("aria-pressed", "false");
    // (the press, not the click: the keyboard comes up only for a focus given inside the touch itself)
    b.addEventListener("pointerdown", function (ev) {
      ev.preventDefault();
      if (document.activeElement === field) { field.blur(); if (canvas) canvas.focus(); } else field.focus();
    });
    b.addEventListener("click", function (ev) { if (ev.detail === 0) field.focus(); });
    return b;
  }
  function syncTouch() {
    var entries = keyEntries();
    if (!touchWanted()) { if (touchBar) { touchBar.remove(); touchBar = null; typeField = null; } placeTouch(); return; }
    if (touchBar) { placeTouch(); return; }
    touchBar = document.createElement("div");
    touchBar.className = "bc-touch";
    touchBar.setAttribute("role", "group");
    touchBar.setAttribute("aria-label", T("controls.touch.label"));
    entries.forEach(function (c) {
      var b = document.createElement("button");
      b.type = "button";
      b.textContent = c.does;
      b.title = keyName(c.keys[0]);
      var code = c.keys[0];
      function press(ev) { ev.preventDefault(); if (held[code]) return; held[code] = true; sendKey("keydown", code); show(); }
      function release(ev) { ev.preventDefault(); if (!held[code]) return; held[code] = false; sendKey("keyup", code); show(); }
      b.addEventListener("pointerdown", press);
      b.addEventListener("pointerup", release);
      b.addEventListener("pointercancel", release);
      b.addEventListener("pointerleave", release);
      // the keyboard's click on the button (Enter, Space): a press and its release
      b.addEventListener("click", function (ev) { if (ev.detail === 0) { press(ev); release(ev); } });
      touchBar.appendChild(b);
    });
    touchBar.appendChild(typeButton());
    touchBar.appendChild(typeField);
    var wrap = document.getElementById("main") || stage.parentNode;
    wrap.appendChild(touchBar);
    placeTouch();
  }
  window.addEventListener("resize", function () { if (touchBar) placeTouch(); });

  // ---------- the Controls panel ----------
  var capturing = null;
  function drawPanel() {
    var body = document.getElementById("controls-body");
    if (!body) return;
    body.textContent = "";
    var rows = table();
    if (!rows.length) {
      var none = document.createElement("p");
      none.className = "note";
      none.textContent = T("controls.none");
      body.appendChild(none);
      return;
    }
    var dl = document.createElement("dl");
    dl.className = "bc-list";
    rows.forEach(function (c) {
      var dt = document.createElement("dt"), dd = document.createElement("dd");
      dt.textContent = c.does;
      if (!c.keys || !c.keys.length) { dd.textContent = c.how || ""; dl.appendChild(dt); dl.appendChild(dd); return; }
      var game = c.keys[0], mine = [];
      for (var from in map.keys) if (map.keys[from] === game) mine.push(from);
      var keys = document.createElement("span");
      keys.className = "bc-keys";
      c.keys.concat(mine).forEach(function (k, i) {
        var kbd = document.createElement("kbd");
        kbd.textContent = keyName(k);
        if (i >= c.keys.length) kbd.className = "bc-mine";
        keys.appendChild(kbd);
      });
      dd.appendChild(keys);
      var change = document.createElement("button");
      change.type = "button";
      change.textContent = capturing === game ? T("controls.press") : T("controls.change");
      change.setAttribute("aria-label", T("controls.change") + ": " + c.does);
      change.setAttribute("data-game-key", game);
      change.addEventListener("click", function () { capturing = capturing === game ? null : game; drawPanel(); if (capturing) focusAfter(game); });
      dd.appendChild(change);
      // the gamepad button for it
      var sel = document.createElement("select");
      sel.setAttribute("aria-label", T("controls.pad") + ": " + c.does);
      var cur = null;
      PAD.forEach(function (p) { if (padKeyFor(p.b) === game) cur = p.b; });
      var opt0 = document.createElement("option");
      opt0.value = "";
      opt0.textContent = T("controls.pad.none");
      sel.appendChild(opt0);
      PAD.forEach(function (p) {
        var o = document.createElement("option");
        o.value = String(p.b);
        o.textContent = T("controls.pad") + " " + p.name;
        if (cur === p.b) o.selected = true;
        sel.appendChild(o);
      });
      sel.addEventListener("change", function () {
        for (var k in map.pad) if (map.pad[k] === game) delete map.pad[k];
        // the defaults: a button chosen here stops standing for the entry it had by default
        if (sel.value) map.pad[sel.value] = game;
        PAD.forEach(function (p) { if (String(p.b) !== sel.value && !map.pad[p.b] && padKeyFor(p.b) === game) map.pad[p.b] = "-"; });
        saveMap(map);
        drawPanel();
      });
      dd.appendChild(sel);
      dl.appendChild(dt);
      dl.appendChild(dd);
    });
    body.appendChild(dl);
    var row = document.createElement("div");
    row.className = "row";
    var reset = document.createElement("button");
    reset.type = "button";
    reset.textContent = T("controls.reset");
    reset.disabled = !Object.keys(map.keys).length && !Object.keys(map.pad).length;
    reset.addEventListener("click", function () { map = { keys: {}, pad: {} }; saveMap(map); capturing = null; drawPanel(); });
    row.appendChild(reset);
    body.appendChild(row);
    var note = document.createElement("p");
    note.className = "note";
    note.textContent = T("controls.note");
    body.appendChild(note);
  }
  function focusAfter(game) {
    var body = document.getElementById("controls-body");
    var b = body && body.querySelector("button[data-game-key=\"" + game + "\"]");
    if (b) b.focus();
  }
  // while "Press a key" is up: the next key the player presses becomes theirs for that game key (Esc gives up)
  function onCapture(e) {
    if (!capturing || e.type !== "keydown") return;
    var panel = document.getElementById("panel-controls");
    if (!panel || panel.hidden) { capturing = null; return; }
    e.preventDefault();
    e.stopImmediatePropagation();
    if (e.code !== "Escape" && info(e.code)) {
      delete map.keys[e.code];
      if (e.code !== capturing) map.keys[e.code] = capturing;
      saveMap(map);
    }
    capturing = null;
    drawPanel();
  }

  var STYLE = ".bc-display { position: absolute; bottom: 6px; left: 6px; z-index: 2; pointer-events: none; display: flex; gap: 4px; flex-wrap: wrap; }" +
    ".bc-display span { padding: 2px 7px; border-radius: 4px; font: 600 13px/1.3 ui-monospace, Consolas, monospace; color: #fff;" +
    " background: rgba(0, 0, 0, 0.6); text-shadow: 0 1px 1px #000; }" +
    ".bc-touch { display: flex; flex-wrap: wrap; gap: 8px; justify-content: center; padding: 8px 16px; touch-action: none; }" +
    ".bc-touch .bc-typefield { position: absolute; width: 1px; height: 1px; padding: 0; border: 0; opacity: 0; pointer-events: none; }" +
    ".bc-touch button.bc-keyboard { min-width: 44px; padding: 6px 8px; }" +
    ".bc-touch button { min-width: 64px; min-height: 44px; padding: 6px 14px; font: inherit; touch-action: none; -webkit-user-select: none; user-select: none; }" +
    ".bc-list { margin: 0 0 12px; } .bc-list dt { font-weight: 600; margin-top: 10px; } .bc-list dd { margin: 3px 0 0; display: flex; flex-wrap: wrap; gap: 6px; align-items: center; }" +
    ".bc-keys kbd { display: inline-block; min-width: 1.6em; text-align: center; padding: 1px 6px; margin-right: 4px; border: 1px solid currentColor; border-radius: 4px; font: 600 12px/1.5 ui-monospace, Consolas, monospace; opacity: 0.85; }" +
    ".bc-keys kbd.bc-mine { border-style: dashed; }";

  function sync() {
    stage = stage || document.getElementById("stage");
    canvas = canvas || document.getElementById("canvas");
    if (!stage || !canvas) return;
    if (!document.getElementById("bc-style")) {
      var st = document.createElement("style");
      st.id = "bc-style";
      st.textContent = STYLE;
      document.head.appendChild(st);
    }
    syncDisplay();
    syncTouch();
    if (speedWanted() !== speedSet) hook();
  }
  var listening = false;
  function listen() {
    if (listening) return;
    listening = true;
    // window, capture: before the canvas's own listeners, so a remapped key's event never reaches the game
    window.addEventListener("keydown", onCapture, true);
    ["keydown", "keyup", "keypress"].forEach(function (t) { window.addEventListener(t, function (e) { if (e.target === canvas) onKey(e); }, true); });
    ["keydown", "keyup", "mousedown", "mouseup"].forEach(function (t) { window.addEventListener(t, function (e) { if (e.target === canvas) onSeen(e); }, true); });
    window.addEventListener("blur", onSeen);
  }
  function setGame(id) {
    pageGame = id || null;
    map = loadMap();
    listen();
    sync();
    drawPanel();
    try { if (navigator.getGamepads && Array.prototype.some.call(navigator.getGamepads(), function (g) { return g && g.connected; })) { padOn = true; hook(); } } catch (e) {}
  }
  window.addEventListener("byopop-setting", function (ev) {
    var n = ev.detail && ev.detail.name;
    if (n === "inputDisplay" || n === "touchKeys" || n === "touchPlace" || n === "speed") sync();
  });
  window.addEventListener("storage", function (ev) {
    if (ev.key === MAPKEY) { map = loadMap(); drawPanel(); }
    if (ev.key === "byopop-settings" || ev.key === null) sync();
  });
  return { setGame: setGame, sync: sync, drawPanel: drawPanel, map: function () { return map; } };
})();
