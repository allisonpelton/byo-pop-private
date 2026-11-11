// The splits timer (docs/WEB_PLAYER.md, "The splits timer"): Desktop 95's Splits window, a stand-in for LiveSplit so that a
// speedrun can be timed on the site. It holds one set of splits at a time (a game, a category and its segments), with the
// personal best's split times and the best segment times ("golds"), in Real Time and Game Time, kept in this browser's IndexedDB.
// The window: the game and category and the attempt count at the top; a row for each segment with its delta and its split time
// (the comparison's while it is ahead of the run); the big timer; the previous segment's delta and the sum of best segments.
// Menus: File (New, Open, Save, Import, Export: LiveSplit's .lss), Edit (Edit Splits), Control (Start/Split, Undo Split, Skip
// Split, Pause, Reset), Options (Hotkeys, Comparison, Timing).
// Real Time is performance.now; Game Time is the game window's update count (byopop_update_count) since the start, times its
// update step (10 ms; 5 ms in the Mahjong games), for a game that reports one. The hotkeys (Numpad 1 split, 3 reset, 8 undo,
// 2 skip at first) are heard on the page and in every game window, which are of the same origin, and are never taken from the
// game: the key still reaches it. No web build has the LiveSplit block that desktop builds have (features.livesplit), so there
// is no autosplitting here.
//   ByopopSplits.attach(desk, at)   adds the window to Desktop 95's desktop, closed (web/index.html)
var ByopopSplits = (function () {
  "use strict";
  var DB = "byopop-splits", STORE = "runs", LAST_KEY = "byopop-splits-last", OPT_KEY = "byopop-splits-options";
  var KEYS = ["split", "reset", "undo", "skip", "pause"];
  // each action's name in the Hotkeys dialog (whole keys, which the strings test finds)
  var KEY_LABELS = { split: "splits.key_split", reset: "splits.key_reset", undo: "splits.key_undo", skip: "splits.key_skip", pause: "splits.key_pause" };
  var DEFAULT_KEYS ={ split: "Numpad1", reset: "Numpad3", undo: "Numpad8", skip: "Numpad2", pause: "" };
  // the games whose update step is not the framework's 10 ms (MahjongApp.cpp sets mFrameTime 5)
  var STEP = { mahjongchina: 5, mahjongjapan: 5 };
  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function make(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  function now() { return performance.now(); }

  // ---- times ----
  // a time on the display: h:mm:ss.cc, m:ss.cc or s.cc (dec places after the point; 0 for none); null is a dash
  function fmt(ms, dec, sign) {
    if (ms === null || ms === undefined || isNaN(ms)) return "—";
    var neg = ms < 0, a = Math.abs(ms);
    var unit = Math.pow(10, 3 - dec), t = Math.floor(a / unit);
    var frac = dec ? t % Math.pow(10, dec) : 0, s = Math.floor(t / Math.pow(10, dec));
    var h = Math.floor(s / 3600), m = Math.floor(s / 60) % 60, sec = s % 60;
    var out = h ? h + ":" + pad(m) + ":" + pad(sec) : m ? m + ":" + pad(sec) : String(sec);
    if (dec) out += "." + String(frac).padStart(dec, "0");
    return (neg ? "−" : sign ? "+" : "") + out;
  }
  function pad(n) { return n < 10 ? "0" + n : String(n); }
  // LiveSplit's TimeSpan: [-]hh:mm:ss[.fffffff]
  function toSpan(ms) {
    var neg = ms < 0, a = Math.abs(ms), ticks = Math.round(a * 10000);
    var s = Math.floor(ticks / 1e7), f = ticks % 1e7;
    var out = pad(Math.floor(s / 3600)) + ":" + pad(Math.floor(s / 60) % 60) + ":" + pad(s % 60);
    if (f) out += "." + String(f).padStart(7, "0");
    return (neg ? "-" : "") + out;
  }
  function fromSpan(text) {
    if (!text) return null;
    var m = /^\s*(-)?(?:(\d+)\.)?(\d+):(\d+):(\d+)(?:\.(\d+))?\s*$/.exec(text);
    if (!m) return null;
    var ms = ((+(m[2] || 0) * 24 + +m[3]) * 3600 + +m[4] * 60 + +m[5]) * 1000 + (m[6] ? +("0." + m[6]) * 1000 : 0);
    return m[1] ? -ms : ms;
  }
  // a time typed into Edit Splits: h:mm:ss.ff, m:ss.ff or s.ff; empty is none
  function parseTyped(text) {
    text = (text || "").trim();
    if (!text) return null;
    var neg = /^[-−]/.test(text);
    if (neg) text = text.slice(1);
    var parts = text.split(":"), ms = 0;
    if (parts.length > 3 || parts.some(function (p) { return !/^\d+(\.\d+)?$/.test(p); })) return NaN;
    parts.forEach(function (p) { ms = ms * 60 + parseFloat(p); });
    ms = Math.round(ms * 1000);
    return neg ? -ms : ms;
  }

  // ---- the splits ----
  function blankRun() { return { game: "", category: "", offset: 0, attempts: 0, segments: [segment(T("splits.segment1"))] }; }
  function segment(name) { return { name: name, pb: { rt: null, gt: null }, best: { rt: null, gt: null } }; }
  function runKey(run) { return (run.game || "") + "\u0000" + (run.category || ""); }
  function clone(o) { return JSON.parse(JSON.stringify(o)); }

  // IndexedDB: one record a set of splits, by its game and category
  var dbp = null;
  function db() {
    if (dbp) return dbp;
    dbp = new Promise(function (res, rej) {
      var r;
      try { r = indexedDB.open(DB, 1); } catch (e) { rej(e); return; }
      r.onupgradeneeded = function () { r.result.createObjectStore(STORE, { keyPath: "key" }); };
      r.onsuccess = function () { res(r.result); };
      r.onerror = function () { rej(r.error); };
    });
    return dbp;
  }
  function tx(mode, fn) {
    return db().then(function (d) {
      return new Promise(function (res, rej) {
        var t = d.transaction(STORE, mode), out = fn(t.objectStore(STORE));
        t.oncomplete = function () { res(out && "result" in out ? out.result : undefined); };
        t.onerror = function () { rej(t.error); };
      });
    });
  }
  function saveRun(run) {
    var rec = clone(run);
    rec.key = runKey(run);
    try { localStorage.setItem(LAST_KEY, rec.key); } catch (e) {}
    return tx("readwrite", function (s) { return s.put(rec); }).catch(function () {});
  }
  function listRuns() { return tx("readonly", function (s) { return s.getAll(); }).catch(function () { return []; }); }
  function loadRun(key) { return tx("readonly", function (s) { return s.get(key); }).catch(function () { return null; }); }
  function dropRun(key) { return tx("readwrite", function (s) { return s.delete(key); }).catch(function () {}); }

  // ---- .lss ----
  function readLss(text) {
    var doc = new DOMParser().parseFromString(text, "application/xml");
    var root = doc.documentElement;
    if (!root || root.nodeName !== "Run" || doc.getElementsByTagName("parsererror").length) throw new Error("lss");
    function child(el, name) { for (var c = el && el.firstElementChild; c; c = c.nextElementSibling) if (c.nodeName === name) return c; return null; }
    function txt(el, name) { var c = child(el, name); return c ? c.textContent.trim() : ""; }
    function times(el) { return { rt: el ? fromSpan(txt(el, "RealTime")) : null, gt: el ? fromSpan(txt(el, "GameTime")) : null }; }
    var run = { game: txt(root, "GameName"), category: txt(root, "CategoryName"), offset: fromSpan(txt(root, "Offset")) || 0,
                attempts: parseInt(txt(root, "AttemptCount"), 10) || 0, segments: [] };
    var segs = child(root, "Segments");
    for (var s = segs && segs.firstElementChild; s; s = s.nextElementSibling) {
      if (s.nodeName !== "Segment") continue;
      var seg = segment(txt(s, "Name")), st = child(s, "SplitTimes");
      for (var t = st && st.firstElementChild; t; t = t.nextElementSibling)
        if (t.nodeName === "SplitTime" && t.getAttribute("name") === "Personal Best") seg.pb = times(t);
      seg.best = times(child(s, "BestSegmentTime"));
      run.segments.push(seg);
    }
    if (!run.segments.length) throw new Error("lss");
    return run;
  }
  function writeLss(run) {
    var doc = document.implementation.createDocument(null, "Run", null), root = doc.documentElement;
    root.setAttribute("version", "1.7.0");
    function el(parent, name, text) { var e = doc.createElement(name); if (text !== undefined) e.textContent = text; parent.appendChild(e); return e; }
    function times(parent, t) {
      if (t.rt !== null) el(parent, "RealTime", toSpan(t.rt));
      if (t.gt !== null) el(parent, "GameTime", toSpan(t.gt));
    }
    el(root, "GameIcon"); el(root, "GameName", run.game); el(root, "CategoryName", run.category);
    el(root, "Offset", toSpan(run.offset || 0)); el(root, "AttemptCount", String(run.attempts || 0)); el(root, "AttemptHistory");
    var segs = el(root, "Segments");
    run.segments.forEach(function (sg) {
      var s = el(segs, "Segment");
      el(s, "Name", sg.name); el(s, "Icon");
      var st = el(el(s, "SplitTimes"), "SplitTime");
      st.setAttribute("name", "Personal Best");
      times(st, sg.pb);
      times(el(s, "BestSegmentTime"), sg.best);
      el(s, "SegmentHistory");
    });
    el(root, "AutoSplitterSettings");
    // two spaces an element deeper, as LiveSplit writes its files
    var lines = new XMLSerializer().serializeToString(doc).replace(/></g, ">\n<").split("\n"), depth = 0;
    lines = lines.map(function (ln) {
      if (/^<\//.test(ln)) depth--;
      var out = "  ".repeat(Math.max(0, depth)) + ln;
      if (/^<[^\/!?][^>]*[^\/]>$|^<[^\/!?]>$/.test(ln) && !/<\/[^>]+>$/.test(ln)) depth++;
      return out;
    });
    return '<?xml version="1.0" encoding="UTF-8"?>\n' + lines.join("\n") + "\n";
  }

  // ---- dialogs: Windows 95's, in the message box's look ----
  function dialog(title, cls) {
    var d = make("dialog", "d95-msgbox spl-dlg" + (cls ? " " + cls : ""));
    var tb = make("div", "d95-tb d95-msgbox-tb"), tt = make("span", "d95-tb-text", title);
    tb.appendChild(tt); d.appendChild(tb);
    var body = make("div", "spl-dlg-body"), row = make("div", "d95-msgbox-row spl-dlg-row");
    d.appendChild(body); d.appendChild(row);
    document.body.appendChild(d);
    var api = { el: d, body: body, row: row, onCancel: null };
    api.close = function () { try { d.close(); } catch (e) {} if (d.parentNode) d.parentNode.removeChild(d); };
    api.button = function (label, fn, dflt) {
      var b = make("button", null, label); b.type = "button";
      b.addEventListener("click", function () { fn(); });
      row.appendChild(b);
      if (dflt) api.dflt = b;
      return b;
    };
    d.addEventListener("cancel", function (ev) { ev.preventDefault(); if (api.onCancel) api.onCancel(); else api.close(); });
    api.show = function () { d.showModal(); (api.focus || api.dflt || row.querySelector("button")).focus(); };
    return api;
  }
  // a question with Yes and No (Reset's, about the new golds)
  function ask(title, text, yes, no) {
    var d = dialog(title, "spl-ask"), b = make("div", "d95-msgbox-body"), icon = make("i", "d95-msgbox-icon");
    icon.setAttribute("aria-hidden", "true");
    b.appendChild(icon); b.appendChild(make("p", null, text));
    d.body.appendChild(b);
    d.button(T("splits.yes"), function () { d.close(); yes(); }, true);
    d.button(T("splits.no"), function () { d.close(); no(); });
    d.onCancel = function () { d.close(); no(); };
    d.show();
  }
  function note(title, text) {
    var d = dialog(title), b = make("div", "d95-msgbox-body"), icon = make("i", "d95-msgbox-icon");
    icon.setAttribute("aria-hidden", "true");
    b.appendChild(icon); b.appendChild(make("p", null, text));
    d.body.appendChild(b);
    d.button(T("skin.apps.ok"), d.close, true);
    d.show();
  }
  function field(parent, label, value, cls) {
    var l = make("label", "spl-field" + (cls ? " " + cls : "")), s = make("span", null, label), i = make("input");
    i.type = "text"; i.value = value; i.spellcheck = false;
    l.appendChild(s); l.appendChild(i); parent.appendChild(l);
    return i;
  }

  // ---- the timer ----
  function build() {
    var opts = { keys: clone(DEFAULT_KEYS), compare: "pb", method: "rt", hotkeys: true };
    try { var so = JSON.parse(localStorage.getItem(OPT_KEY) || "null"); if (so) { Object.assign(opts, so); opts.keys = Object.assign(clone(DEFAULT_KEYS), so.keys || {}); } } catch (e) {}
    function keepOpts() { try { localStorage.setItem(OPT_KEY, JSON.stringify(opts)); } catch (e) {} }

    var run = blankRun();
    // the attempt: phase idle, running, paused or ended; index, the segment being run; times, each split's {rt, gt} or null (skipped)
    var att = { phase: "idle", index: 0, times: [], t0: 0, pausedAt: 0, pausedFor: 0, gameWin: null, gameStep: 10, c0: 0, gtLast: null };

    var el = make("section", "spl-win");
    el.id = "splits";
    var bar = make("div", "d95-menubar"); bar.setAttribute("role", "menubar");
    el.appendChild(bar);
    var head = make("div", "spl-head"), gameEl = make("div", "spl-game"), catEl = make("div", "spl-cat"), triesEl = make("div", "spl-tries");
    head.appendChild(gameEl); head.appendChild(catEl); head.appendChild(triesEl);
    var list = make("div", "spl-list"); list.setAttribute("role", "table");
    var timerEl = make("div", "spl-timer");
    var foot = make("div", "spl-foot");
    var prevRow = make("div", "spl-stat"), prevVal = make("span", "spl-val");
    prevRow.appendChild(make("span", null, T("splits.prev"))); prevRow.appendChild(prevVal);
    var sobRow = make("div", "spl-stat"), sobVal = make("span", "spl-val");
    sobRow.appendChild(make("span", null, T("splits.sob"))); sobRow.appendChild(sobVal);
    foot.appendChild(prevRow); foot.appendChild(sobRow);
    var client = make("div", "spl-client");
    client.appendChild(head); client.appendChild(list); client.appendChild(timerEl); client.appendChild(foot);
    el.appendChild(client);

    // ---- the run's numbers ----
    function m() { return opts.method; }
    function cumBest(i, k) {
      var sum = 0;
      for (var j = 0; j <= i; j++) { var b = run.segments[j].best[k]; if (b === null) return null; sum += b; }
      return sum;
    }
    function compareAt(i, k) { k = k || m(); return opts.compare === "best" ? cumBest(i, k) : run.segments[i].pb[k]; }
    function sumOfBest(k) { return run.segments.length ? cumBest(run.segments.length - 1, k || m()) : null; }
    // the time of the attempt now, in each method (Game Time null when the game reports no updates)
    function current() {
      var rt = null;
      if (att.phase === "running") rt = now() - att.t0 - att.pausedFor;
      else if (att.phase === "paused") rt = att.pausedAt - att.t0 - att.pausedFor;
      else if (att.phase === "ended") { var last = att.times[att.times.length - 1]; rt = last ? last.rt : null; }
      var gt = null;
      if (att.phase === "running" || att.phase === "paused") gt = gameTime();
      else if (att.phase === "ended") { var l2 = att.times[att.times.length - 1]; gt = l2 ? l2.gt : null; }
      return { rt: rt, gt: gt };
    }
    function gameTime() {
      var w = att.gameWin;
      if (!w) return null;
      try {
        var n = w.Module && typeof w.Module._byopop_update_count === "function" ? w.Module._byopop_update_count() : -1;
        if (n >= 0 && n >= att.c0) att.gtLast = (n - att.c0) * att.gameStep;
      } catch (e) {}
      return att.gtLast;
    }
    // the segment time of split i in method k: its time less the previous split's; null when either is missing
    function segTime(times, i, k) {
      var t = times[i];
      if (!t || t[k] === null) return null;
      if (i === 0) return t[k];
      var p = times[i - 1];
      return p && p[k] !== null ? t[k] - p[k] : null;
    }
    function isGold(i, k) {
      var s = segTime(att.times, i, k), b = run.segments[i].best[k];
      return s !== null && (b === null || s < b);
    }

    // ---- drawing ----
    function draw() {
      gameEl.textContent = run.game || " ";
      catEl.textContent = run.category || " ";
      triesEl.textContent = String(run.attempts);
      var cur = current(), k = m();
      var rows = list.children;
      while (rows.length > run.segments.length) list.removeChild(list.lastChild);
      run.segments.forEach(function (sg, i) {
        var r = rows[i];
        if (!r) {
          r = make("div", "spl-row"); r.setAttribute("role", "row");
          r.appendChild(make("span", "spl-name")); r.appendChild(make("span", "spl-delta")); r.appendChild(make("span", "spl-time"));
          r.children[0].setAttribute("role", "cell"); r.children[1].setAttribute("role", "cell"); r.children[2].setAttribute("role", "cell");
          list.appendChild(r);
        }
        var done = i < att.times.length, t = done ? att.times[i] : null, cmp = compareAt(i, k);
        var active = (att.phase === "running" || att.phase === "paused") && i === att.index;
        r.classList.toggle("spl-cur", active);
        r.children[0].textContent = sg.name;
        var delta = "", cls = "";
        if (done && t && t[k] !== null && cmp !== null) {
          var dv = t[k] - cmp;
          delta = fmt(dv, 1, true);
          cls = isGold(i, k) ? "spl-gold" : dv < 0 ? "spl-ahead" : "spl-behind";
        } else if (active && cur[k] !== null && cmp !== null && cur[k] > cmp) {
          delta = fmt(cur[k] - cmp, 1, true); cls = "spl-behind";
        }
        r.children[1].textContent = delta;
        r.children[1].className = "spl-delta " + cls;
        r.children[2].textContent = done ? (t && t[k] !== null ? fmt(t[k], 2) : "—") : fmt(cmp, 2);
      });
      var shown = cur[k];
      if (att.phase === "idle") shown = run.offset;
      timerEl.textContent = fmt(shown === null ? null : shown, 2);
      var tc = "";
      if (att.phase === "ended" || att.phase === "running" || att.phase === "paused") {
        var ci = att.phase === "ended" ? att.times.length - 1 : att.index, cv = compareAt(ci, k);
        // a finished run: blue when it is a new personal best (a first finished run is one), else ahead or behind the comparison
        if (att.phase === "ended") {
          var lt = att.times[ci], pbEnd = run.segments[ci].pb[k];
          if (lt && lt[k] !== null) tc = pbEnd === null || lt[k] < pbEnd ? "spl-pb" : cv !== null && lt[k] < cv ? "spl-ahead" : "spl-behind";
        }
        else if (cur[k] !== null) tc = cv !== null && cur[k] > cv ? "spl-behind" : "spl-ahead";
      }
      timerEl.className = "spl-timer " + tc + (att.phase === "paused" ? " spl-paused" : "");
      // the previous segment: its segment time against the comparison's
      var pi = att.phase === "ended" ? att.times.length - 1 : att.index - 1, pv = "", pc = "";
      if (pi >= 0) {
        var s = segTime(att.times, pi, k), c1 = compareAt(pi, k), c0 = pi ? compareAt(pi - 1, k) : 0;
        if (s !== null && c1 !== null && c0 !== null) { var d = s - (c1 - c0); pv = fmt(d, 2, true); pc = isGold(pi, k) ? "spl-gold" : d < 0 ? "spl-ahead" : "spl-behind"; }
      }
      prevVal.textContent = pv || "—"; prevVal.className = "spl-val " + pc;
      sobVal.textContent = fmt(sumOfBest(), 2);
    }
    var ticking = false;
    function tick() {
      if (att.phase !== "running") { ticking = false; return; }
      draw();
      requestAnimationFrame(tick);
    }
    function startTicking() { if (!ticking) { ticking = true; requestAnimationFrame(tick); } }

    // ---- the controls ----
    function frontGame() {
      try {
        var f = ByopopApps.frontFrame(), g = ByopopApps.frontGame();
        return f ? { win: f.win, step: STEP[g] || 10 } : null;
      } catch (e) { return null; }
    }
    function start() {
      var g = frontGame();
      att = { phase: "running", index: 0, times: [], t0: now() - run.offset, pausedAt: 0, pausedFor: 0, gameWin: g ? g.win : null, gameStep: g ? g.step : 10, c0: 0, gtLast: null };
      if (att.gameWin) {
        try { var n = att.gameWin.Module._byopop_update_count(); if (n >= 0) { att.c0 = n; att.gtLast = 0; } else att.gameWin = null; } catch (e) { att.gameWin = null; }
      }
      run.attempts++;
      saveRun(run);
      startTicking();
    }
    function split() {
      if (att.phase === "idle") { start(); draw(); return; }
      if (att.phase !== "running") return;
      var c = current();
      att.times[att.index] = { rt: c.rt, gt: c.gt };
      att.index++;
      if (att.index >= run.segments.length) finish();
      draw();
    }
    function finish() { att.phase = "ended"; }
    // a finished run faster than the personal best, in the method being timed, becomes it (a first run always does); kept at
    // Reset, as LiveSplit keeps it, so an undone last split leaves the personal best as it was
    function keepPB() {
      if (att.phase !== "ended") return false;
      var k = m(), last = run.segments.length - 1, t = att.times[last], pb = run.segments[last].pb[k];
      if (!t || t[k] === null || (pb !== null && t[k] >= pb)) return false;
      run.segments.forEach(function (sg, i) { var x = att.times[i]; sg.pb = { rt: x ? x.rt : null, gt: x ? x.gt : null }; });
      return true;
    }
    function undo() {
      // undoing the last split takes the run back up: the timer runs on from where it is now
      if (att.phase === "ended") att.phase = "running";
      if (att.phase !== "running" && att.phase !== "paused") return;
      if (att.index === 0) return;
      att.index--;
      att.times.length = att.index;
      startTicking();
      draw();
    }
    function skip() {
      if (att.phase !== "running" || att.index >= run.segments.length - 1) return;
      att.times[att.index] = null;
      att.index++;
      draw();
    }
    function pause() {
      if (att.phase === "running") { att.phase = "paused"; att.pausedAt = now(); }
      else if (att.phase === "paused") { att.pausedFor += now() - att.pausedAt; att.phase = "running"; startTicking(); }
      draw();
    }
    // Reset: new golds in either method are kept if the player says so
    var asking = false;
    function reset() {
      if (att.phase === "idle" || asking) return;
      var golds = [];
      ["rt", "gt"].forEach(function (k) { for (var i = 0; i < att.times.length; i++) if (isGold(i, k)) golds.push([i, k, segTime(att.times, i, k)]); });
      function clear() { att = { phase: "idle", index: 0, times: [], t0: 0, pausedAt: 0, pausedFor: 0, gameWin: null, gameStep: 10, c0: 0, gtLast: null }; draw(); }
      if (keepPB()) saveRun(run);
      if (!golds.length) { clear(); return; }
      if (att.phase === "running") pause();
      asking = true;
      ask(T("splits.title"), T("splits.keepgolds"), function () {
        asking = false;
        golds.forEach(function (g) { run.segments[g[0]].best[g[1]] = g[2]; });
        saveRun(run);
        clear();
      }, function () { asking = false; clear(); });
    }

    // ---- File ----
    function setRun(r) {
      run = r;
      att = { phase: "idle", index: 0, times: [], t0: 0, pausedAt: 0, pausedFor: 0, gameWin: null, gameStep: 10, c0: 0, gtLast: null };
      draw();
    }
    function idleOnly() { return att.phase === "idle"; }
    function fileNew() { editSplits(blankRun(), true); }
    function fileOpen() {
      listRuns().then(function (runs) {
        var d = dialog(T("splits.open")), sel = make("select", "spl-pick");
        sel.size = 8;
        runs.sort(function (a, b) { return a.key < b.key ? -1 : 1; }).forEach(function (r) {
          var o = make("option", null, (r.game || "—") + " — " + (r.category || "—")); o.value = r.key; sel.appendChild(o);
        });
        if (sel.options.length) sel.selectedIndex = 0;
        d.body.appendChild(sel);
        function pick() {
          var key = sel.value;
          if (!key) return;
          loadRun(key).then(function (r) { if (r) { delete r.key; setRun(r); try { localStorage.setItem(LAST_KEY, key); } catch (e) {} } d.close(); });
        }
        sel.addEventListener("dblclick", pick);
        d.button(T("splits.openbtn"), pick, true);
        d.button(T("splits.delete"), function () {
          var key = sel.value;
          if (!key) return;
          dropRun(key).then(function () { sel.remove(sel.selectedIndex); });
        });
        d.button(T("splits.cancel"), d.close);
        d.focus = sel;
        d.show();
      });
    }
    function fileImport() {
      var inp = make("input"); inp.type = "file"; inp.accept = ".lss"; inp.hidden = true;
      document.body.appendChild(inp);
      inp.addEventListener("change", function () {
        var f = inp.files && inp.files[0];
        inp.remove();
        if (!f) return;
        f.text().then(function (text) {
          var r;
          try { r = readLss(text); } catch (e) { note(T("splits.title"), T("splits.badfile")); return; }
          setRun(r); saveRun(r);
        });
      });
      inp.click();
    }
    function fileExport() {
      var blob = new Blob([writeLss(run)], { type: "application/xml" });
      var a = make("a"); a.href = URL.createObjectURL(blob);
      a.download = ((run.game || "splits") + (run.category ? " - " + run.category : "")).replace(/[\\/:*?"<>|]+/g, "_") + ".lss";
      document.body.appendChild(a); a.click(); a.remove();
      setTimeout(function () { URL.revokeObjectURL(a.href); }, 1000);
    }

    // ---- Edit Splits: the game, category, offset, attempts, and each segment's name, personal best split and best segment ----
    function editSplits(base, fresh) {
      var r = clone(base), k = m();
      var d = dialog(T("splits.edit"), "spl-edit");
      var top = make("div", "spl-edit-top");
      var gi = field(top, T("splits.game"), r.game), ci = field(top, T("splits.category"), r.category);
      var oi = field(top, T("splits.offset"), fmt(r.offset || 0, 2).replace("−", "-"), "spl-short");
      oi.defaultValue = oi.value;
      var ai = field(top, T("splits.attempts"), String(r.attempts), "spl-short");
      d.body.appendChild(top);
      var grid = make("div", "spl-grid"), headRow = make("div", "spl-grow spl-ghead");
      [T("splits.segname"), T("splits.splittime"), T("splits.bestseg")].forEach(function (h) { headRow.appendChild(make("span", null, h)); });
      grid.appendChild(headRow);
      var rowsBox = make("div", "spl-grows");
      grid.appendChild(rowsBox);
      d.body.appendChild(grid);
      var tools = make("div", "spl-gtools");
      var sel = 0;
      function rowOf(i, sg) {
        var row = make("div", "spl-grow"), n = make("input"), p = make("input"), b = make("input");
        [n, p, b].forEach(function (x) { x.type = "text"; x.spellcheck = false; row.appendChild(x); x.addEventListener("focus", function () { pickRow(i); }); });
        n.value = sg.name;
        p.defaultValue = p.value = sg.pb[k] === null ? "" : fmt(sg.pb[k], 2).replace("−", "-");
        b.defaultValue = b.value = sg.best[k] === null ? "" : fmt(sg.best[k], 2).replace("−", "-");
        n.setAttribute("aria-label", T("splits.segname")); p.setAttribute("aria-label", T("splits.splittime")); b.setAttribute("aria-label", T("splits.bestseg"));
        return row;
      }
      function pickRow(i) { sel = i; Array.prototype.forEach.call(rowsBox.children, function (x, j) { x.classList.toggle("spl-gsel", j === i); }); }
      // the rows' typed values back into r, before a row is moved or the dialog closes; false when a time does not read
      function readRows() {
        var ok = true;
        Array.prototype.forEach.call(rowsBox.children, function (row, i) {
          var sg = r.segments[i], inp = row.querySelectorAll("input");
          sg.name = inp[0].value;
          // a time is read again only when its text changed, so one left alone keeps the precision the display rounds away
          var pv = inp[1].value === inp[1].defaultValue ? sg.pb[k] : parseTyped(inp[1].value);
          var bv = inp[2].value === inp[2].defaultValue ? sg.best[k] : parseTyped(inp[2].value);
          if (isNaN(pv) || isNaN(bv)) { ok = false; return; }
          sg.pb[k] = pv; sg.best[k] = bv;
        });
        return ok;
      }
      function fill() {
        rowsBox.textContent = "";
        r.segments.forEach(function (sg, i) { rowsBox.appendChild(rowOf(i, sg)); });
        pickRow(Math.min(sel, r.segments.length - 1));
      }
      function tool(label, fn) {
        var b = make("button", null, label); b.type = "button";
        b.addEventListener("click", function () { if (!readRows()) { note(T("splits.edit"), T("splits.badtime")); return; } fn(); fill(); });
        tools.appendChild(b);
      }
      tool(T("splits.insabove"), function () { r.segments.splice(sel, 0, segment("")); });
      tool(T("splits.insbelow"), function () { r.segments.splice(sel + 1, 0, segment("")); sel++; });
      tool(T("splits.remove"), function () { if (r.segments.length > 1) r.segments.splice(sel, 1); });
      tool(T("splits.up"), function () { if (sel > 0) { var s = r.segments.splice(sel, 1)[0]; r.segments.splice(--sel, 0, s); } });
      tool(T("splits.down"), function () { if (sel < r.segments.length - 1) { var s = r.segments.splice(sel, 1)[0]; r.segments.splice(++sel, 0, s); } });
      d.body.appendChild(tools);
      fill();
      d.button(T("skin.apps.ok"), function () {
        var off = oi.value === oi.defaultValue ? r.offset : parseTyped(oi.value);
        if (!readRows() || isNaN(off)) { note(T("splits.edit"), T("splits.badtime")); return; }
        r.game = gi.value.trim(); r.category = ci.value.trim(); r.offset = off === null ? 0 : off;
        r.attempts = Math.max(0, parseInt(ai.value, 10) || 0);
        d.close();
        setRun(r); saveRun(r);
      }, true);
      d.button(T("splits.cancel"), d.close);
      d.focus = gi;
      d.show();
    }

    // ---- Hotkeys: each action and its key; a button takes the next key pressed ----
    function hotkeys() {
      var d = dialog(T("splits.hotkeys"), "spl-keys"), keys = clone(opts.keys), capture = null;
      var on = make("label", "spl-check"), cb = make("input");
      cb.type = "checkbox"; cb.checked = opts.hotkeys;
      on.appendChild(cb); on.appendChild(make("span", null, T("splits.hotkeys_on")));
      d.body.appendChild(on);
      var tbl = make("div", "spl-keygrid");
      KEYS.forEach(function (a) {
        var l = make("span", null, T(KEY_LABELS[a])), b = make("button", "spl-keybtn", keyName(keys[a])); b.type = "button";
        b.addEventListener("click", function () {
          if (capture) capture.b.textContent = keyName(keys[capture.a]);
          capture = { a: a, b: b }; b.textContent = "…";
        });
        tbl.appendChild(l); tbl.appendChild(b);
      });
      d.body.appendChild(tbl);
      d.el.addEventListener("keydown", function (ev) {
        if (!capture) return;
        ev.preventDefault(); ev.stopPropagation();
        keys[capture.a] = ev.code === "Escape" ? keys[capture.a] : ev.code === "Backspace" || ev.code === "Delete" ? "" : ev.code;
        capture.b.textContent = keyName(keys[capture.a]);
        capture = null;
      }, true);
      d.button(T("skin.apps.ok"), function () { opts.keys = keys; opts.hotkeys = cb.checked; keepOpts(); d.close(); }, true);
      d.button(T("splits.cancel"), d.close);
      d.onCancel = function () { if (capture) return; d.close(); };
      d.show();
    }
    function keyName(code) {
      if (!code) return T("splits.key_none");
      var m1 = /^Numpad(\d)$/.exec(code);
      if (m1) return T("splits.numpad", { n: m1[1] });
      return code.replace(/^Key|^Digit/, "");
    }

    // ---- the menus ----
    var MENUS = [
      { key: "splits.m_file", list: function () {
        var idle = idleOnly();
        return [{ label: T("splits.new"), fn: fileNew, disabled: !idle }, { label: T("splits.open") + "…", fn: fileOpen, disabled: !idle },
                { label: T("splits.save"), fn: function () { saveRun(run); } }, { rule: true },
                { label: T("splits.import") + "…", fn: fileImport, disabled: !idle }, { label: T("splits.export") + "…", fn: fileExport }];
      } },
      { key: "splits.m_edit", list: function () { return [{ label: T("splits.edit") + "…", fn: function () { editSplits(run, false); }, disabled: !idleOnly() }]; } },
      { key: "splits.m_control", list: function () {
        var ph = att.phase, kn = function (a) { return opts.keys[a] ? keyName(opts.keys[a]) : undefined; };
        return [{ label: T(ph === "idle" ? "splits.start" : "splits.split"), key: kn("split"), fn: split, disabled: ph === "ended" || ph === "paused" },
                { label: T("splits.undo"), key: kn("undo"), fn: undo, disabled: ph === "idle" || (ph !== "ended" && att.index === 0) },
                { label: T("splits.skip"), key: kn("skip"), fn: skip, disabled: ph !== "running" || att.index >= run.segments.length - 1 },
                { label: T(ph === "paused" ? "splits.resume" : "splits.pause"), key: kn("pause"), fn: pause, disabled: ph !== "running" && ph !== "paused" },
                { rule: true }, { label: T("splits.reset"), key: kn("reset"), fn: reset, disabled: ph === "idle" }];
      } },
      { key: "splits.m_options", list: function () {
        return [{ label: T("splits.hotkeys") + "…", fn: hotkeys }, { rule: true },
                { label: T("splits.cmp_pb"), checked: opts.compare === "pb", fn: function () { opts.compare = "pb"; keepOpts(); draw(); } },
                { label: T("splits.cmp_best"), checked: opts.compare === "best", fn: function () { opts.compare = "best"; keepOpts(); draw(); } }, { rule: true },
                { label: T("splits.rt"), checked: opts.method === "rt", fn: function () { opts.method = "rt"; keepOpts(); draw(); } },
                { label: T("splits.gt"), checked: opts.method === "gt", fn: function () { opts.method = "gt"; keepOpts(); draw(); } }];
      } }
    ];
    var barItems = [];
    var desk = null;
    function openMenu(i) {
      var b = barItems[i], r = b.getBoundingClientRect();
      barItems.forEach(function (x, j) { x.classList.toggle("d95-open", j === i); });
      desk.menu(b, MENUS[i].list(), r.left, r.bottom, T(MENUS[i].key), {
        side: function (dir) { openMenu((i + dir + MENUS.length) % MENUS.length); },
        done: function () { barItems.forEach(function (x) { x.classList.remove("d95-open"); }); }
      });
    }
    MENUS.forEach(function (mn, i) {
      var b = make("button", "d95-menubar-item"); b.type = "button"; b.setAttribute("role", "menuitem"); b.setAttribute("aria-haspopup", "menu");
      b.tabIndex = i ? -1 : 0;
      var name = T(mn.key), u = make("u", null, name.charAt(0));
      b.appendChild(u); b.appendChild(document.createTextNode(name.slice(1)));
      b.addEventListener("click", function () { openMenu(i); });
      bar.appendChild(b); barItems.push(b);
    });
    // Alt and a menu's letter while the window has the focus
    el.addEventListener("keydown", function (ev) {
      if (!ev.altKey || ev.ctrlKey || ev.metaKey || !ev.key || ev.key.length !== 1) return;
      for (var i = 0; i < MENUS.length; i++) if (T(MENUS[i].key).charAt(0).toLowerCase() === ev.key.toLowerCase()) { ev.preventDefault(); ev.stopPropagation(); openMenu(i); return; }
    });

    // ---- the hotkeys, on this page and in every game window ----
    function onKey(ev) {
      if (!opts.hotkeys || ev.repeat || !ev.code || ev.ctrlKey || ev.altKey || ev.metaKey) return;
      if (document.querySelector("dialog.spl-dlg[open]")) return;
      if (el.classList.contains("d95-closed") || !document.documentElement.classList.contains("d95-desk")) return;
      var t = ev.target;
      if (t && t.ownerDocument === document && t.closest && t.closest("input, textarea, select, [contenteditable]")) return;
      var a = null;
      KEYS.forEach(function (x) { if (opts.keys[x] && opts.keys[x] === ev.code) a = x; });
      if (!a) return;
      ({ split: split, reset: reset, undo: undo, skip: skip, pause: pause })[a]();
    }
    window.addEventListener("keydown", onKey, true);
    function hookFrames() {
      Array.prototype.forEach.call(document.querySelectorAll("iframe.d95-app-frame"), function (f) {
        var w;
        try { w = f.contentWindow; if (!w || w.__byopopSplitsKeys === onKey) return; w.addEventListener("keydown", onKey, true); w.__byopopSplitsKeys = onKey; } catch (e) {}
      });
    }
    if (window.MutationObserver) new MutationObserver(hookFrames).observe(document.body, { childList: true });
    document.addEventListener("load", function (ev) { if (ev.target && ev.target.classList && ev.target.classList.contains("d95-app-frame")) hookFrames(); }, true);
    setInterval(hookFrames, 2000);

    var last = null;
    try { last = localStorage.getItem(LAST_KEY); } catch (e) {}
    if (last) loadRun(last).then(function (r) { if (r && att.phase === "idle") { delete r.key; setRun(r); } });
    draw();
    return { el: el, setDesk: function (d) { desk = d; }, draw: draw,
             // for the page's tests
             run: function () { return clone(run); }, attempt: function () { return clone({ phase: att.phase, index: att.index, times: att.times }); },
             split: split, undo: undo, skip: skip, pause: pause, reset: reset, readLss: readLss, writeLss: writeLss, load: setRun };
  }

  var timer = null;
  function attach(desk, at) {
    if (timer || !document.getElementById("games-title")) return null;
    timer = build();
    timer.setDesk(desk);
    var foot = document.querySelector("body > footer");
    if (foot && foot.parentNode) foot.parentNode.insertBefore(timer.el, foot); else document.body.appendChild(timer.el);
    // its icon: a stopwatch, our own drawing in 32 x 32
    var pic = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    pic.setAttribute("viewBox", "0 0 32 32"); pic.setAttribute("aria-hidden", "true"); pic.setAttribute("shape-rendering", "crispEdges");
    pic.innerHTML = '<path d="M13 2h6v3h-6z" fill="#808080" stroke="#000"/><path d="M15 5h2v3h-2z" fill="#000"/>' +
      '<circle cx="16" cy="18" r="12" fill="#c0c0c0" stroke="#000" shape-rendering="auto"/><circle cx="16" cy="18" r="9" fill="#ffffff" stroke="#808080" shape-rendering="auto"/>' +
      '<path d="M16 18V11" stroke="#000" stroke-width="2"/><path d="M16 18l5 3" stroke="#800000" stroke-width="2" shape-rendering="auto"/>' +
      '<path d="M25 6l3 3" stroke="#000" stroke-width="2"/>';
    desk.add(timer.el, { id: "splits", title: T("splits.title"), closable: true, closed: true, pic: pic, at: at, scroll: ".spl-list" });
    return timer;
  }
  return { attach: attach, timer: function () { return timer; }, fmt: fmt, toSpan: toSpan, fromSpan: fromSpan };
})();
