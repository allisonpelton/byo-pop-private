// The shared page check (docs/WEB_PLAYER.md, "The page check"): how a game's page behaves as a browser page, which no
// comparison with the original reaches. It runs in the player-only page as a driver (web/testkit.js), from a game's own table
// (games/<game>/tools/web/pagecheck.js: return await t.run("tools/web/pagecheck.js", {...})) or bare as
// ?driver=tools/web/pagecheck.js, and needs the test profile and the driver before the start:
//   <game>.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/<game>/tools/web/pagecheck.js
// The checks, each a line of the result:
//   focus     the page hidden and shown again mid-play (blur, visibilitychange): which state keys change while hidden and
//             after, against the native build's own answer to "focus lose" and "focus regain" (tools/web/pagecheck_native.py)
//   keys      whether the page cancels the keys a player presses in the game, so that the browser does nothing with them
//             (Tab's focus move, the page's scrolling, Firefox's quick find on ' and /); which browser keys (F5, F11, F12,
//             F7, Alt, Ctrl+R/F/L) it cancels, as information; and that it cancels nothing typed into a text field outside
//             the game
//   resize    the canvas drawn at other sizes (as a resized window or full screen draws it): the pointer lands at the game
//             pixel the mouse points at (the framework's "pointer" command)
//   pages     the same game opened a second time (an iframe, as a second tab): it waits for the first (the shell's Web Lock)
//   reload    the page reloaded mid-play, as a player's F5 (after the table's moves, so that a game is in progress):
//             every save file that was there is there again, with the same bytes (unless the table's reloadChanges names
//             it, or the page's backup restores the version a quit would have written), before the game starts and after; and every save the page had backed up for a game in progress (the
//             framework's shutdown saves, platform/WebPersist.cpp) is there once the game has started
// The result (byopop.test.result, and what byopop.test.driver resolves with): {reloading: true} from the first half, which
// reloads the page, then from the second {pass, fail, info, results: [{check, result: "pass"|"FAIL"|"info", detail}]}.
//
// A game's table (args), every field optional:
//   start         commands that start a game, sent in turn once the first screen is up (["seed 1", "goto timed"])
//   titleMove     when a start command answers ERROR, play the bot's move (a title's link), wait 30 updates, send them again
//   ready(s)      the first screen is up (default: a state whose screen is not "loading")
//   playing(s)    the started game is in play (default: screen "game")
//   settle        updates to wait once in play (100)
//   focus         the native build's answer, {hidden: [keys], after: [keys]}, printed by tools/web/pagecheck_native.py
//                 with the same start, settle and updates; without it the line is information
//   focusUpdates  updates hidden, and again after (200)
//   keys          more keys the game uses, by their DOM names, that the page must keep from the browser
//   point         the game pixel the resize check points at ([w / 2, h / 2])
//   moves         moves by the port's bot (botMove, playMove) before the reload, so that a game is in progress (0)
//   reloadChanges RegExp sources of save files that may change across a reload (a file the game rewrites at its start)
//   afterReload   async function (t, before) after the reload's start, resolving with {ok, detail} (before: {files, state})
var cfg = args || {};
var params = new URLSearchParams(location.search);
var STORE = "byopop-pagecheck:" + location.pathname;
var saved = null;
try { saved = JSON.parse(sessionStorage.getItem(STORE) || "null"); } catch (e) { saved = null; }
var results = saved ? saved.results : [];

function note(check, ok, detail) {
  results.push({ check: check, result: ok === null ? "info" : ok ? "pass" : "FAIL", detail: detail === undefined ? "" : detail });
}
if (params.get("hold") !== "1" || !params.get("assets"))
  throw new Error("pagecheck: open the page with ?player=1&assets=1&hold=1 (the test profile, and the driver before the start)");

async function state() {
  try { return await t.state(); } catch (e) { return null; }
}
async function until(what, ok, ms) {
  var from = Date.now(), s = null;
  for (;;) {
    s = await state();
    if (s && ok(s)) return s;
    if (Date.now() - from > ms) throw new Error("pagecheck: " + what + " never came: " + JSON.stringify(s));
    await t.wait(10);
  }
}
var ready = cfg.ready || function (s) { return !!s.screen && s.screen !== "loading"; };
async function startGame() {
  for (var tries = 0; ; tries++) {
    var failed = null;
    for (var i = 0; i < (cfg.start || []).length && !failed; i++) {
      var lines = await t.command(cfg.start[i]);
      var last = lines[lines.length - 1] || "";
      if (/^ERROR /.test(last)) failed = last;
    }
    if (!failed) return;
    if (!cfg.titleMove || tries >= 20) throw new Error("pagecheck: " + failed);
    // the refusal is expected while a title is up: it is not one of the page's errors
    for (var e = t.errors.length - 1; e >= 0; e--) if (t.errors[e].text.indexOf(failed) >= 0) t.errors.splice(e, 1);
    var m = await t.botMove();
    if (m) await t.playMove(m);
    await t.wait(30);
  }
}
var playing = cfg.playing || function (s) { return s.screen === "game"; };

// a save file's bytes, as a short FNV-1a hash
function hash(bytes) {
  var h = 0x811c9dc5;
  for (var i = 0; i < bytes.length; i++) { h ^= bytes[i]; h = Math.imul(h, 0x01000193) >>> 0; }
  return ("0000000" + h.toString(16)).slice(-8) + "/" + bytes.length;
}
function snapshot() {
  var files = {};
  t.profile.list().forEach(function (f) { files[f] = hash(t.profile.read(f)); });
  return files;
}
// the save files the page's backup holds with their contents (platform/WebPersist.cpp): what a reload restores
function backupFiles() {
  try {
    var backup = JSON.parse(localStorage.getItem("sexy-userdata-backup:" + window.ByopopShell.USER) || "null");
    if (backup && backup.files) return Object.keys(backup.files).filter(function (f) { return !!backup.files[f].d; }).sort();
  } catch (e) {}
  return [];
}
// a file may change when the table's reloadChanges names it, or when the page's backup restores it (a quit's version of it)
function compareFiles(before, now, restored) {
  var allowed = (cfg.reloadChanges || []).map(function (r) { return new RegExp(r); });
  var free = function (f) { return (restored || []).indexOf(f) >= 0 || allowed.some(function (r) { return r.test(f); }); };
  var out = { missing: [], changed: [], added: [] };
  Object.keys(before).forEach(function (f) {
    if (!(f in now)) out.missing.push(f);
    else if (now[f] !== before[f] && !free(f)) out.changed.push(f);
  });
  Object.keys(now).forEach(function (f) { if (!(f in before)) out.added.push(f); });
  return out;
}
// the top-level state keys whose values differ between two states
function changedKeys(a, b) {
  var keys = {};
  Object.keys(a || {}).concat(Object.keys(b || {})).forEach(function (k) { keys[k] = true; });
  return Object.keys(keys).filter(function (k) { return JSON.stringify((a || {})[k]) !== JSON.stringify((b || {})[k]); }).sort();
}
function sameSet(a, b) { return JSON.stringify(a.slice().sort()) === JSON.stringify((b || []).slice().sort()); }

async function focusCheck() {
  var n = cfg.focusUpdates || 200;
  t.pause();
  var s0 = await state();
  await t.blur({ hidden: true });
  await t.step(n);
  var s1 = await state();
  await t.focus();
  await t.step(n);
  var s2 = await state();
  t.resume();
  if (!s0 || !s1 || !s2) return note("focus", false, "no state answer: " + JSON.stringify([!!s0, !!s1, !!s2]));
  var got = { hidden: changedKeys(s0, s1), after: changedKeys(s1, s2) };
  if (!cfg.focus) return note("focus: hidden and back (no native record to compare)", null, got);
  var ok = sameSet(got.hidden, cfg.focus.hidden) && sameSet(got.after, cfg.focus.after);
  note("focus: hidden and back, as the native build", ok, ok ? got : { web: got, native: cfg.focus });
}

async function keysCheck() {
  var canvas = document.getElementById("canvas");
  canvas.focus();
  var kept = ["Tab", "Backspace", " ", "ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown", "PageUp", "PageDown", "Home", "End"]
    .concat(cfg.keys || []);
  var cancelled = function (r) { return r.down || r.press === true; };
  var leaked = [];
  for (var i = 0; i < kept.length; i++) {
    var r = await t.keyCheck(kept[i]);
    if (!cancelled(r)) leaked.push(r);
  }
  note("keys: the game's keys stay out of the browser", leaked.length === 0,
    leaked.length ? leaked : kept.map(function (k) { return k === " " ? "Space" : k; }).join(" "));
  var find = [];
  for (var j = 0, quick = ["'", "/"]; j < quick.length; j++) {
    var q = await t.keyCheck(quick[j]);
    if (!cancelled(q)) find.push(q);
  }
  note("keys: ' and / open no quick find (Firefox)", find.length === 0, find.length ? find : "both cancelled");
  var browser = [["F5"], ["F11"], ["F12"], ["F7"], ["Alt"], ["r", { ctrl: true }], ["f", { ctrl: true }], ["l", { ctrl: true }]];
  var taken = [], left = [];
  for (var k = 0; k < browser.length; k++) {
    var b = await t.keyCheck(browser[k][0], browser[k][1]);
    var name = (browser[k][1] ? "Ctrl+" + browser[k][0].toUpperCase() : browser[k][0]);
    (cancelled(b) ? taken : left).push(name);
  }
  note("keys: browser keys the game takes while it has the focus", null, { cancelled: taken, left: left });
  var field = document.createElement("input");
  field.type = "text";
  field.style.cssText = "position:fixed;left:0;bottom:0;width:120px";
  document.body.appendChild(field);
  field.focus();
  var typed = [];
  for (var m = 0, keys = ["a", "'", "/", " ", "Backspace", "ArrowLeft", "Tab"]; m < keys.length; m++) {
    var f = await t.keyCheck(keys[m], null, field);
    if (cancelled(f)) typed.push(f);
  }
  field.remove();
  canvas.focus();
  note("keys: typing in a text field outside the game is left alone", typed.length === 0, typed.length ? typed : "none cancelled");
}

async function resizeCheck() {
  var canvas = document.getElementById("canvas"), size = t.screen();
  var keep = { w: canvas.style.width, h: canvas.style.height };
  var p = cfg.point || [size.w >> 1, size.h >> 1];
  var bad = [];
  var scales = [1.5, 0.75, 2, 1];
  for (var i = 0; i < scales.length; i++) {
    canvas.style.width = size.w * scales[i] + "px";
    canvas.style.height = size.h * scales[i] + "px";
    await t.frame();
    await t.move(p[0] + 4, p[1] + 4);
    await t.wait(2);
    await t.move(p[0], p[1]);
    await t.wait(2);
    var at = null, lines = [];
    try {
      lines = await t.command("pointer");
      var m = /^POINTER (.*)$/.exec(lines[lines.length - 1]);
      at = m ? JSON.parse(m[1]) : null;
    } catch (e) { lines = [String(e && e.message || e)]; }
    var r = canvas.getBoundingClientRect();
    if (!at || Math.abs(at.x - p[0]) > 1 || Math.abs(at.y - p[1]) > 1)
      bad.push({ scale: scales[i], drawn: [Math.round(r.width), Math.round(r.height)], at: at || lines[lines.length - 1] });
  }
  canvas.style.width = keep.w;
  canvas.style.height = keep.h;
  await t.frame();
  note("resize: the pointer lands where it points", bad.length === 0,
    bad.length ? bad : "at " + p.join(",") + " in canvases drawn at x" + scales.join(", x"));
}

// ms of real time: messages, which a hidden pane does not slow as it does timers, and a held game may draw no frame
function pause(ms) {
  var until = Date.now() + ms, ch = new MessageChannel();
  return new Promise(function (resolve) {
    ch.port1.onmessage = function () { if (Date.now() >= until) { ch.port1.close(); resolve(); } else ch.port2.postMessage(0); };
    ch.port2.postMessage(0);
  });
}

async function pagesCheck() {
  var url = location.pathname + "?player=1&silent=1&assets=" + encodeURIComponent(params.get("assets"));
  var frame = document.createElement("iframe");
  frame.style.cssText = "position:fixed;left:0;top:0;width:" + t.screen().w + "px;height:" + t.screen().h + "px;opacity:0;pointer-events:none";
  frame.src = url;
  // the first game held meanwhile (a second page loads slowly in a hidden pane, and a timed game would run out)
  t.pause();
  document.body.appendChild(frame);
  var from = Date.now(), seen = null;
  while (Date.now() - from < 120000) {
    var w = frame.contentWindow, d = null;
    try { d = frame.contentDocument; } catch (e) { d = null; }
    var busy = d && d.getElementById("busy-overlay");
    var kit = w && w.byopop && w.byopop.test;
    seen = { waiting: !!(busy && !busy.hidden), updates: kit ? kit.updates() : null, status: w && w.ByopopShell ? w.ByopopShell.status : null };
    if (seen.waiting) break;
    await pause(100);
  }
  await pause(1000);
  var kit2 = frame.contentWindow && frame.contentWindow.byopop && frame.contentWindow.byopop.test;
  var ran = kit2 ? kit2.updates() : -1;
  frame.remove();
  t.resume();
  var s = await state();
  note("pages: a second copy waits for the first", !!(seen && seen.waiting) && !(ran > 0) && !!s,
    { second: seen, secondUpdates: ran, firstAnswers: !!s });
}

if (!saved) {
  // the first half: a fresh test profile, the game started and in play, the checks, then the reload
  await t.profile.wipe();
  await t.start();
  await until("the first screen", ready, 180000);
  await startGame();
  await until("play", playing, 120000);
  await t.wait(cfg.settle || 100);
  await focusCheck();
  await keysCheck();
  await resizeCheck();
  await pagesCheck();
  var made = 0;
  for (var tries = 0; made < (cfg.moves || 0) && tries < 300; tries++) {
    var mv = await t.botMove();
    if (mv) { await t.playMove(mv); made++; continue; }
    var now = await state();
    if (now && now.over) break;
    await t.wait(10);
  }
  if (cfg.moves) await t.wait(60);
  note("no errors before the reload", t.errors.length === 0, t.errors.slice(0, 5));
  var before = { files: snapshot(), state: await state(), moves: made };
  sessionStorage.setItem(STORE, JSON.stringify({ results: results, before: before }));
  setTimeout(function () { location.reload(); }, 0);
  return { reloading: true, checks: results.length };
}

// the second half, after the reload: the saves as the page found them, before the start and after it
sessionStorage.removeItem(STORE);
var was = saved.before;
// the backup as the page left it when it went away (its last one is written then), before the start restores from it
var restored = backupFiles();
var found = compareFiles(was.files, snapshot(), restored);
note("reload: the saves come back", found.missing.length === 0 && found.changed.length === 0,
  { files: Object.keys(was.files).length, missing: found.missing, changed: found.changed, added: found.added });
await t.start();
await until("the first screen after the reload", ready, 180000);
await t.wait(cfg.settle || 100);
var kept = compareFiles(was.files, snapshot(), restored);
note("reload: the saves are still there once the game has started", kept.missing.length === 0,
  { missing: kept.missing, changed: kept.changed, added: kept.added });
var started = snapshot();
var lost = restored.filter(function (f) { return !(f in started); });
if (restored.length)
  note("reload: the game in progress is kept", lost.length === 0, { moves: was.moves, backedUp: restored, lost: lost });
else note("reload: the game in progress is kept (no save backed up to restore)", null, { moves: was.moves });
if (cfg.afterReload) {
  var r = await cfg.afterReload(t, was);
  note("reload: " + (r.name || "the game's own check"), !!r.ok, r.detail);
}
note("no errors after the reload", t.errors.length === 0, t.errors.slice(0, 5));
var count = function (what) { return results.filter(function (x) { return x.result === what; }).length; };
return { pass: count("pass"), fail: count("FAIL"), info: count("info"), results: results };
