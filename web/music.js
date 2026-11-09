// The music player (docs/WEB_PLAYER.md, "The music player"; the user, 2026-11-07): the listing page's Music tab plays the music of
// the games the player added, from the game's own files in this browser (each game's database, as the import wrote them). Nothing is
// shipped with the site and nothing is sent anywhere. The tunes of a game are the subsongs of its module files (web/soundtracks.js),
// decoded by libopenmpt built on its own for the web (openmpt.js, web/music/openmpt_web.c, loaded the first time the tab opens) and
// played through an AudioWorklet. A media player's controls are on the page (the user, 2026-11-08): previous, play or pause, stop and
// next, a seek bar between the time played and the tune's length, repeat and the volume; the Media Session API gives them to the
// media keys and the system's media controls. Two views can be shown: a spectrum (a Web Audio AnalyserNode) and, for a module's
// tune, a tracker's view of its patterns, the rows the listener hears (the decoder says where it is only while the view is on the
// screen). At ?silent=1 nothing reaches the speakers.
//   ByopopMusic.init(root, games)   builds the player in root; games: [{id, title, persist}], the listing's tiles
// The player is one state at a time (state below): idle (nothing chosen), loading (a tune's file being read), playing, paused.
var ByopopMusic = (function () {
  "use strict";
  var DB_STORE = "FILE_DATA";
  var MIN_TUNE = 2;          // seconds: a shorter subsong is a silence or a sting between tunes, not a tune
  var CHUNK = 4096;          // frames a message to the worklet carries
  var AHEAD = 12;            // chunks kept queued in the worklet: about a second
  var VOLUME_KEY = "byopop-music-volume", LOOP_KEY = "byopop-music-loop";
  function T(key, vars) { return ByopopStrings.t(key, vars); }
  function silentPage() { try { return new URLSearchParams(location.search).get("silent") === "1"; } catch (e) { return false; } }
  function make(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined) e.textContent = text;
    return e;
  }

  // ---------- the games' files ----------
  // A path pattern of web/soundtracks.js as a regular expression for a path in the game's folder
  function patternRe(p) {
    return new RegExp("^" + p.replace(/[.+?^${}()|[\]\\]/g, "\\$&").replace(/\*/g, "[^/]*") + "$", "i");
  }
  // Opens a game's database without making one (as web/saves.js does): null when there is none
  function openExisting(persist) {
    return new Promise(function (resolve) {
      var r;
      try { r = indexedDB.open(persist); } catch (e) { resolve(null); return; }
      r.onerror = r.onblocked = function () { resolve(null); };
      r.onupgradeneeded = function (ev) { try { ev.target.transaction.abort(); } catch (e) {} resolve(null); };
      r.onsuccess = function () {
        var db = r.result;
        if (!db.objectStoreNames.contains(DB_STORE)) { db.close(); resolve(null); return; }
        resolve(db);
      };
    });
  }
  // Every file of a game's folder, as paths in the folder
  function gameFiles(db, persist) {
    var prefix = persist + "/game/";
    return new Promise(function (resolve, reject) {
      var r = db.transaction(DB_STORE, "readonly").objectStore(DB_STORE).getAllKeys(IDBKeyRange.bound(prefix, prefix + "￿"));
      r.onerror = function () { reject(r.error); };
      r.onsuccess = function () { resolve(r.result.map(function (k) { return String(k).substring(prefix.length); })); };
    });
  }
  // The files that match the patterns, in the order of the patterns, then by name, each once
  function matching(rels, patterns) {
    var found = [];
    (patterns || []).forEach(function (p) {
      var re = patternRe(p);
      rels.filter(function (rel) { return re.test(rel); }).sort().forEach(function (rel) { if (found.indexOf(rel) < 0) found.push(rel); });
    });
    return found;
  }
  function readFile(db, key) {
    return new Promise(function (resolve, reject) {
      var r = db.transaction(DB_STORE, "readonly").objectStore(DB_STORE).get(key);
      r.onerror = function () { reject(r.error); };
      r.onsuccess = function () {
        var c = r.result && r.result.contents;
        resolve(c ? (c instanceof Uint8Array ? c : new Uint8Array(c)) : null);
      };
    });
  }
  // The pieces a playlist joins, in order (Rocket Mania and Pixelus; games/rocketmania/src/AppMusic.cpp): three header lines,
  // then the first word of each line names a piece; blank lines do not count
  function playlistPieces(text) {
    return text.split(/\r?\n/).filter(function (l) { return /\S/.test(l); }).slice(3).map(function (l) { return l.trim().split(/\s+/)[0]; });
  }

  // ---------- the decoder ----------
  var decoderPromise = null;
  // the decoder let go (the Music window closed): the next tune loads it again
  function dropDecoder() { decoderPromise = null; }
  function decoder() {
    if (decoderPromise) return decoderPromise;
    decoderPromise = new Promise(function (resolve, reject) {
      if (typeof ByopopOpenMPT === "function") { resolve(); return; }
      var s = document.createElement("script");
      s.src = "openmpt.js";
      s.onload = function () { resolve(); };
      s.onerror = function () { reject(new Error("openmpt.js")); };
      document.head.appendChild(s);
    }).then(function () { return ByopopOpenMPT(); });
    decoderPromise.catch(function () { decoderPromise = null; });
    return decoderPromise;
  }
  // A module from its bytes, or 0
  function loadModule(M, bytes) {
    var p = M._malloc(bytes.length);
    M.HEAPU8.set(bytes, p);
    var m = M._bm_load(p, bytes.length);
    M._free(p);
    return m;
  }
  function isOgg(rel) { return /\.ogg$/i.test(rel); }
  // An Ogg Vorbis file's length in seconds, from its last page's position and the rate in its first header, without decoding it
  function oggSeconds(bytes) {
    var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength), rate = 0, i;
    for (i = 0; i + 16 < bytes.length && i < 4096; i++) {
      if (bytes[i] === 1 && bytes[i + 1] === 0x76 && bytes[i + 2] === 0x6f && bytes[i + 3] === 0x72) { rate = dv.getUint32(i + 12, true); break; }
    }
    if (!rate) return 0;
    for (i = bytes.length - 14; i >= 0; i--) {
      if (bytes[i] === 0x4f && bytes[i + 1] === 0x67 && bytes[i + 2] === 0x67 && bytes[i + 3] === 0x53) {
        return (dv.getInt32(i + 10, true) * 4294967296 + dv.getUint32(i + 6, true)) / rate;
      }
    }
    return 0;
  }
  // An Ogg Vorbis file as interleaved stereo at the context's rate: the browser's decoder, else stb_vorbis in openmpt.wasm
  function decodeOgg(M, ctx, bytes) {
    return ctx.decodeAudioData(bytes.slice().buffer).then(function (buf) {
      var n = buf.length, l = buf.getChannelData(0), r = buf.numberOfChannels > 1 ? buf.getChannelData(1) : l, out = new Float32Array(n * 2);
      for (var i = 0; i < n; i++) { out[i * 2] = l[i]; out[i * 2 + 1] = r[i]; }
      return out;
    }, function () { return stbOgg(M, bytes, ctx.sampleRate); });
  }
  // stb_vorbis's 16-bit samples at the file's rate, made stereo float at rate (linear interpolation)
  function stbOgg(M, bytes, rate) {
    var p = M._malloc(bytes.length), io = M._malloc(12);
    M.HEAPU8.set(bytes, p);
    var n = M._bm_ogg(p, bytes.length, io, io + 4, io + 8), ch = M.HEAP32[io >> 2], from = M.HEAP32[(io + 4) >> 2], data = M.HEAP32[(io + 8) >> 2];
    M._free(p);
    M._free(io);
    if (n <= 0 || !data || !from || !ch) { if (data) M._free(data); throw new Error("ogg"); }
    var s = M.HEAP16.subarray(data >> 1, (data >> 1) + n * ch), frames = Math.floor(n * rate / from), out = new Float32Array(frames * 2);
    for (var i = 0; i < frames; i++) {
      var x = i * from / rate, a = Math.floor(x), f = x - a, b = Math.min(a + 1, n - 1);
      for (var c = 0; c < 2; c++) {
        var k = Math.min(c, ch - 1);
        out[i * 2 + c] = (s[a * ch + k] * (1 - f) + s[b * ch + k] * f) / 32768;
      }
    }
    M._free(data);
    return out;
  }

  // ---------- the tunes ----------
  // A file's name without its folder and extension
  function baseName(rel) { return rel.split("/").pop().replace(/\.[^.]+$/, ""); }
  function clock(seconds) {
    var s = Math.round(seconds), m = Math.floor(s / 60);
    return T("music.duration", { min: m, sec: (s % 60 < 10 ? "0" : "") + (s % 60) });
  }
  // The path in rels that is rel up to case, or null
  function findRel(rels, rel) {
    var low = rel.toLowerCase();
    for (var i = 0; i < rels.length; i++) if (rels[i].toLowerCase() === low) return rels[i];
    return null;
  }
  // A game's tunes: [{game, kind, keys, sub, range, name, seconds}] (kind: module, ogg or joined). A module's tunes are its
  // subsongs, or the ranges of orders its parts list; an Ogg file is one tune; a joined tune is the pieces its playlist names, one
  // after the other. Tunes under MIN_TUNE seconds and those in skip are left out; names names a tune, else its file does.
  function tunesOf(M, game, entry) {
    return openExisting(game.persist).then(function (db) {
      if (!db) return [];
      var prefix = game.persist + "/game/", tunes = [];
      function read(rel) { return readFile(db, prefix + rel); }
      function add(rel, list) {
        var kept = list.filter(function (k) { return k.seconds >= MIN_TUNE && (entry.skip || []).indexOf(k.id) < 0; });
        kept.forEach(function (k, j) {
          var named = entry.names && entry.names[k.id];
          k.game = game;
          k.name = named || (kept.length > 1 ? T("music.tune_part", { file: baseName(rel), n: j + 1 }) : baseName(rel));
          tunes.push(k);
        });
      }
      return gameFiles(db, game.persist).then(function (rels) {
        var steps = matching(rels, entry.files).map(function (rel) {
          return function () {
            return read(rel).then(function (bytes) {
              if (!bytes) return;
              var id = baseName(rel);
              if (isOgg(rel)) { add(rel, [{ kind: "ogg", keys: [prefix + rel], id: id, seconds: oggSeconds(bytes) }]); return; }
              var m = loadModule(M, bytes), list = [];
              if (!m) return;
              var ranges = entry.parts && entry.parts[id];
              if (ranges) {
                ranges.forEach(function (r, i) {
                  var t0 = M._bm_set_order(m, r[0]), t1 = r[1] + 1 < M._bm_orders(m) ? M._bm_set_order(m, r[1] + 1) : M._bm_duration(m);
                  list.push({ kind: "module", keys: [prefix + rel], range: r, start: t0, id: id + "#" + i, seconds: t1 - t0, level: entry.level });
                });
              } else {
                for (var i = 0, n = M._bm_subsongs(m); i < n; i++) {
                  M._bm_select(m, i);
                  list.push({ kind: "module", keys: [prefix + rel], sub: i, id: id + "#" + i, seconds: M._bm_duration(m), level: entry.level });
                }
              }
              M._bm_free(m);
              add(rel, list);
            });
          };
        });
        (entry.joined || []).forEach(function (j) {
          steps.push(function () {
            var listRel = findRel(rels, j.playlist);
            if (!listRel) return;
            return read(listRel).then(function (bytes) {
              if (!bytes) return;
              var pieces = playlistPieces(new TextDecoder("latin1").decode(bytes)).map(function (w) { return findRel(rels, j.piece.replace("{}", w)); });
              if (!pieces.length || pieces.some(function (p) { return !p; })) return;
              var seconds = 0, seen = {};
              return pieces.reduce(function (p, rel) {
                return p.then(function () {
                  if (seen[rel] !== undefined) { seconds += seen[rel]; return; }
                  return read(rel).then(function (b) { seen[rel] = b ? oggSeconds(b) : 0; seconds += seen[rel]; });
                });
              }, Promise.resolve()).then(function () {
                var id = baseName(listRel).replace(/ playlist$/i, "");
                tunes.push({ game: game, kind: "joined", keys: pieces.map(function (r) { return prefix + r; }), seconds: seconds,
                  name: (entry.names && entry.names[id]) || id });
              });
            });
          });
        });
        return steps.reduce(function (p, step) { return p.then(step); }, Promise.resolve());
      }).then(function () { db.close(); return tunes; }, function (e) { db.close(); throw e; });
    });
  }

  // ---------- playing ----------
  // The worklet: plays the stereo chunks it is sent, in order, and says when it has used one up (1) and when it ran dry (0)
  var WORKLET = "class ByopopMusicOut extends AudioWorkletProcessor {\n" +
    "  constructor() { super(); this.q = []; this.at = 0; this.dry = true;\n" +
    "    this.port.onmessage = (e) => { if (e.data === 'flush') { this.q = []; this.at = 0; this.dry = true; this.port.postMessage('flushed'); } else { this.q.push(e.data); this.dry = false; } }; }\n" +
    "  process(inputs, outputs) {\n" +
    "    const out = outputs[0], l = out[0], r = out[1] || out[0]; let i = 0;\n" +
    "    while (i < l.length && this.q.length) {\n" +
    "      const b = this.q[0], n = Math.min(l.length - i, (b.length >> 1) - this.at);\n" +
    "      for (let k = 0; k < n; k++) { l[i + k] = b[(this.at + k) * 2]; r[i + k] = b[(this.at + k) * 2 + 1]; }\n" +
    "      i += n; this.at += n;\n" +
    "      if (this.at * 2 >= b.length) { this.q.shift(); this.at = 0; this.port.postMessage(1); }\n" +
    "    }\n" +
    "    for (; i < l.length; i++) { l[i] = 0; r[i] = 0; }\n" +
    "    if (!this.q.length && !this.dry) { this.dry = true; this.port.postMessage(0); }\n" +
    "    return true;\n" +
    "  }\n" +
    "}\n" +
    "registerProcessor('byopop-music-out', ByopopMusicOut);\n";

  // The controls' pictures, our own drawing in a 16 x 16 box (no icon font, no image): the transport, repeat (two arrows in a
  // loop) and the speaker, with its waves or muted
  var GLYPHS = {
    previous: "M3 3h2v10H3zM13 3v10L6 8z",
    play: "M4 2v12l10-6z",
    pause: "M3 3h4v10H3zM9 3h4v10H9z",
    stop: "M3 3h10v10H3z",
    next: "M11 3h2v10h-2zM3 3v10l7-5z",
    repeat: "M3 7V5h8V3l3 3-3 3V7H5v1H3zM13 9v2H5v2l-3-3 3-3v2h6V9h2z",
    speaker: "M2 6h3l4-3v10l-4-3H2zM11 5.5c1.2 1.2 1.2 3.8 0 5l-.8-.7c.8-.8.8-2.8 0-3.6zM12.8 3.8c2.2 2.2 2.2 6.2 0 8.4l-.8-.7c1.8-1.8 1.8-5.2 0-7z",
    muted: "M2 6h3l4-3v10l-4-3H2zM10.5 5.8l.7-.7 1.8 1.8 1.8-1.8.7.7-1.8 1.8 1.8 1.8-.7.7-1.8-1.8-1.8 1.8-.7-.7 1.8-1.8z"
  };
  function glyph(name) {
    var ns = "http://www.w3.org/2000/svg", svg = document.createElementNS(ns, "svg"), p = document.createElementNS(ns, "path");
    svg.setAttribute("viewBox", "0 0 16 16");
    svg.setAttribute("width", "16"); svg.setAttribute("height", "16");
    svg.setAttribute("aria-hidden", "true"); svg.setAttribute("focusable", "false");
    p.setAttribute("d", GLYPHS[name]);
    p.setAttribute("fill", "currentColor");
    svg.appendChild(p);
    return svg;
  }
  var VIS_KEY = "byopop-music-vis", MUTE_KEY = "byopop-music-mute";
  var VIS = ["spectrum", "tracker", "off"];  // what the display's lower half shows; a click on it goes to the next
  var SLICE = 512;           // frames between two looks at the decoder's row while the tracker view is shown
  // The display's height, in the tracker's rows (the user, 2026-11-09: "increase how many rows show in the tracker"): five at
  // least, the one heard in the middle; the spectrum takes the same height. The splitter under the player sets it, and it is
  // kept for this browser (localStorage, ROWS_KEY).
  var ROWS_MIN = 5, ROWS_MAX = 64, ROWS_KEY = "byopop-music-rows", ROW_PX = 12;
  var BARS = 24;             // the spectrum's bars

  function init(root, games) {
    var M = null, ctx = null, node = null, gain = null, analyser = null;
    var tunes = [], current = -1, source = null, queued = 0, flushes = 0, ended = false;
    // what the worklet holds, in order: each chunk's frames, the decoder's position at its start, and (tracker view shown) where
    // the decoder was in its patterns every SLICE frames; shifted as the worklet uses a chunk up, at the context's time then
    var heads = [], shiftedAt = 0;
    var state = "idle";  // idle | loading | playing | paused
    var scanned = false, loop = false, volume = 80, muted = false, vis = "spectrum", trackerOn = false, spectrumOn = false;
    try { loop = localStorage.getItem(LOOP_KEY) === "1"; muted = localStorage.getItem(MUTE_KEY) === "1"; } catch (e) {}
    try { var v = parseInt(localStorage.getItem(VOLUME_KEY), 10); if (v >= 0 && v <= 100) volume = v; } catch (e) {}
    try { var kept = localStorage.getItem(VIS_KEY); if (VIS.indexOf(kept) >= 0) vis = kept; } catch (e) {}

    root.textContent = "";
    root.classList.add("mu");
    var player = make("div", "mu-player");
    root.appendChild(player);

    // The display, as players of the time had one: the state, the time heard and the tune's length, the tune's name with its game's
    // below, and the view (spectrum, tracker or nothing), which a click turns to the next. Values only, no sentences: what the
    // player is doing is in the names' tooltip, and for screen readers in the names, a live region.
    var lcd = make("div", "mu-lcd");
    var top = make("div", "mu-lcd-top");
    var stateGlyph = make("span", "mu-lcd-state");
    stateGlyph.appendChild(glyph("stop"));
    var timeEl = make("span", "mu-lcd-time", "0:00");
    var names = make("span", "mu-lcd-names");
    names.setAttribute("role", "status");
    var tuneEl = make("span", "mu-lcd-tune", ""), gameEl = make("span", "mu-lcd-game", "");
    names.appendChild(tuneEl); names.appendChild(gameEl);
    var totalEl = make("span", "mu-lcd-total", "0:00");
    [stateGlyph, timeEl, names, totalEl].forEach(function (e) { top.appendChild(e); });
    lcd.appendChild(top);
    var visEl = make("div", "mu-vis");
    visEl.tabIndex = 0;
    visEl.setAttribute("role", "button");
    var spectrum = make("canvas", "mu-spectrum");
    spectrum.setAttribute("aria-hidden", "true");
    var tracker = make("div", "mu-tracker");
    tracker.setAttribute("aria-hidden", "true");
    var trackerAt = make("div", "mu-tracker-at"), trackerRows = make("pre", "mu-tracker-rows");
    // a scroll bar under the rows when the channels do not all fit, drawn here (arrows, a track and its thumb; the browser's own
    // bar draws no arrows): it moves the view by whole channels and follows nothing on its own. A click on it is the
    // bar's, not the display's (which would go to the next view); the keyboard's way is Left and Right on the display.
    var trackerScroll = make("div", "mu-hbar"), hbLeft = make("span", "mu-hb-left"), hbTrack = make("span", "mu-hb-track"),
        hbThumb = make("span", "mu-hb-thumb"), hbRight = make("span", "mu-hb-right");
    trackerScroll.setAttribute("aria-hidden", "true");
    trackerScroll.hidden = true;
    hbTrack.appendChild(hbThumb);
    [hbLeft, hbTrack, hbRight].forEach(function (e) { trackerScroll.appendChild(e); });
    tracker.appendChild(trackerAt); tracker.appendChild(trackerRows); tracker.appendChild(trackerScroll);
    visEl.appendChild(spectrum); visEl.appendChild(tracker);
    lcd.appendChild(visEl);
    player.appendChild(lcd);

    var seek = make("input", "mu-seekbar");
    seek.type = "range";
    seek.id = "music-seek";
    seek.min = "0"; seek.max = "1"; seek.step = "0.1"; seek.value = "0";
    seek.setAttribute("aria-label", T("music.seek"));
    seek.title = T("music.seek");
    player.appendChild(seek);

    // One row of controls: the transport, repeat, and the speaker (a click mutes) with the volume
    var row = make("div", "mu-controls");
    function control(name, label, cls) {
      var b = make("button", "mu-btn mu-" + (cls || name));
      b.type = "button";
      b.appendChild(glyph(name));
      b.setAttribute("aria-label", label);
      b.title = label;
      row.appendChild(b);
      return b;
    }
    var prevBtn = control("previous", T("music.previous"));
    var playBtn = control("play", T("music.play"));
    var stopBtn = control("stop", T("music.stop"));
    var nextBtn = control("next", T("music.next"));
    row.appendChild(make("span", "mu-gap"));
    var loopBtn = control("repeat", T("music.loop"), "loop");
    loopBtn.setAttribute("aria-pressed", String(loop));
    row.appendChild(make("span", "mu-gap"));
    var muteBtn = control(muted ? "muted" : "speaker", T("music.mute"), "mute");
    muteBtn.setAttribute("aria-pressed", String(muted));
    var vol = make("input", "mu-vol");
    vol.type = "range";
    vol.id = "music-volume";
    vol.min = "0"; vol.max = "100"; vol.step = "1"; vol.value = String(volume);
    vol.setAttribute("aria-label", T("music.volume"));
    vol.title = T("music.volume");
    row.appendChild(vol);
    player.appendChild(row);

    // The splitter between the player and the playlist: dragging it gives the display whole rows more or fewer, as do the arrow
    // keys on it (Page Up and Down five at a time, Home and End the least and the most). No words: its cursor, and its name as
    // a tooltip and for screen readers, with the rows as its value.
    var rows = ROWS_MIN;
    try { rows = Math.max(ROWS_MIN, Math.min(ROWS_MAX, parseInt(localStorage.getItem(ROWS_KEY), 10) || ROWS_MIN)); } catch (e) {}
    var split = make("div", "mu-split");
    split.tabIndex = 0;
    split.setAttribute("role", "separator");
    split.setAttribute("aria-orientation", "horizontal");
    split.setAttribute("aria-valuemin", String(ROWS_MIN));
    split.setAttribute("aria-valuemax", String(ROWS_MAX));
    split.setAttribute("aria-label", T("music.split"));
    split.title = T("music.split");
    root.appendChild(split);
    // Only the playlist scrolls (the user, 2026-11-09: "only the list of songs should scroll within the music menu, so that the
    // tracker/spectral analyzer and controls are always visible"): in its window the list takes the height the player leaves,
    // and keeps at least LIST_ROWS of its rows, so the display grows only as far as that leaves them
    var LIST_ROWS = 3;
    function listBox() { var p = list.parentNode; return p && p.classList && p.classList.contains("d95-client") ? p : list; }
    function inWindow() {
      return document.documentElement.classList.contains("d95-desk") && !!(root.closest && root.closest(".d95-win")) && root.getClientRects().length > 0;
    }
    function listLeast() {
      var t = list.querySelector(".mu-tune"), h = t && t.offsetHeight ? t.offsetHeight : 16;
      return LIST_ROWS * h + 6;
    }
    // the least height of the window that keeps the player, the splitter and LIST_ROWS rows (web/skins.js resizable)
    function windowLeast() {
      var win = root.closest && root.closest(".d95-win");
      if (!win || !inWindow()) return 0;
      return win.offsetHeight - listBox().clientHeight + listLeast();
    }
    function setRows(n, keep) {
      n = Math.max(ROWS_MIN, Math.min(ROWS_MAX, Math.round(n)));
      if (inWindow()) {
        // as many as leave the list its rows in the window as it is
        root.style.setProperty("--mu-rows", String(n));
        while (n > ROWS_MIN && listBox().clientHeight < listLeast()) { n--; root.style.setProperty("--mu-rows", String(n)); }
      }
      var changed = n !== rows;
      rows = n;
      root.style.setProperty("--mu-rows", String(rows));
      split.setAttribute("aria-valuenow", String(rows));
      split.setAttribute("aria-valuetext", T("music.split_rows", { n: rows }));
      if (changed) { shownRow = ""; drawTracker(); drawSpectrum(); }
      if (keep) { asked = rows; try { localStorage.setItem(ROWS_KEY, String(rows)); } catch (e) {} }
    }
    var splitDrag = null;
    split.addEventListener("pointerdown", function (ev) {
      if (ev.button !== 0) return;
      ev.preventDefault();
      splitDrag = { y: ev.clientY, rows: rows, id: ev.pointerId };
      try { split.setPointerCapture(ev.pointerId); } catch (e) {}
      split.focus();
    });
    split.addEventListener("pointermove", function (ev) {
      if (!splitDrag || ev.pointerId !== splitDrag.id) return;
      setRows(splitDrag.rows + Math.round((ev.clientY - splitDrag.y) / ROW_PX), false);
    });
    function endSplit(ev) {
      if (!splitDrag || ev.pointerId !== splitDrag.id) return;
      splitDrag = null;
      setRows(rows, true);
    }
    split.addEventListener("pointerup", endSplit);
    split.addEventListener("pointercancel", endSplit);
    split.addEventListener("keydown", function (ev) {
      var to = { ArrowDown: rows + 1, ArrowUp: rows - 1, PageDown: rows + 5, PageUp: rows - 5, Home: ROWS_MIN, End: ROWS_MAX }[ev.key];
      if (to === undefined || ev.altKey || ev.ctrlKey || ev.metaKey) return;
      ev.preventDefault();
      setRows(to, true);
    });

    var list = make("div", "mu-list");
    root.appendChild(list);
    [prevBtn, playBtn, stopBtn, nextBtn].forEach(function (b) { b.disabled = true; });
    seek.disabled = true;

    // On the screen: the views draw, and the tracker's rows are looked up, only while the player is (web/skins.js closes its
    // window; the tab's panel hides): so the tracker costs nothing while its window is closed
    function onScreen() { return document.visibilityState !== "hidden" && root.getClientRects().length > 0; }

    // A value in the display when there is no tune to show (reading, none, an error): a word or two, the sentence in its tooltip
    function say(word, sentence) {
      tuneEl.textContent = word || "";
      gameEl.textContent = "";
      names.title = sentence || "";
    }
    function isModule() { var t = tunes[current]; return !!(t && t.kind === "module" && source); }
    // The view the display shows: the chosen one, but the spectrum where a tune has no patterns (Ogg)
    function applyVis() {
      var wasSpectrum = spectrumOn;
      trackerOn = vis === "tracker" && isModule();
      spectrumOn = vis === "spectrum" || (vis === "tracker" && !trackerOn);
      tracker.hidden = !trackerOn;
      spectrum.hidden = !spectrumOn;
      var label = T({ spectrum: "music.vis_spectrum", tracker: "music.vis_tracker", off: "music.vis_off" }[vis]);
      visEl.setAttribute("aria-label", label);
      visEl.title = label;
      if (spectrumOn !== wasSpectrum) route();
      shownRow = "";
    }
    // The one place the state changes: the buttons, the list, the display and the media session follow it
    function setState(next) {
      state = next;
      var going = state === "playing" || state === "loading", label = T(going ? "music.pause" : "music.play");
      playBtn.replaceChild(glyph(going ? "pause" : "play"), playBtn.firstChild);
      playBtn.setAttribute("aria-label", label);
      playBtn.title = label;
      playBtn.disabled = !tunes.length;
      prevBtn.disabled = nextBtn.disabled = !tunes.length;
      stopBtn.disabled = state === "idle";
      seek.disabled = !source;
      stateGlyph.replaceChild(glyph(state === "playing" ? "play" : state === "paused" ? "pause" : "stop"), stateGlyph.firstChild);
      stateGlyph.classList.toggle("mu-blink", state === "loading" || state === "paused");
      Array.prototype.forEach.call(list.querySelectorAll("button[data-tune]"), function (b) {
        if (+b.getAttribute("data-tune") === current && state !== "idle") b.setAttribute("aria-current", "true"); else b.removeAttribute("aria-current");
      });
      var t = tunes[current];
      if (t && state !== "idle") {
        tuneEl.textContent = t.name;
        gameEl.textContent = t.game.title;
        names.title = t.name + " — " + t.game.title;
      } else say("", "");
      applyVis();
      media();
      showTime();
      tick();
    }

    // ---------- the media keys (Media Session) ----------
    function media() {
      var ms = navigator.mediaSession;
      if (!ms) return;
      try {
        var t = tunes[current];
        if (!t || state === "idle") { ms.metadata = null; ms.playbackState = "none"; return; }
        var art = [];
        try { var rec = ByopopGameFiles.all()[t.game.id]; if (rec && rec.icon) art.push({ src: rec.icon, sizes: "32x32", type: "image/png" }); } catch (e) {}
        ms.metadata = new MediaMetadata({ title: t.name, artist: t.game.title, album: T("music.album"), artwork: art });
        ms.playbackState = state === "paused" ? "paused" : "playing";
      } catch (e) {}
    }
    var MEDIA_ACTIONS = ["play", "pause", "nexttrack", "previoustrack", "stop", "seekto"];
    function unwireMedia() {
      var ms = navigator.mediaSession;
      if (!ms) return;
      MEDIA_ACTIONS.forEach(function (a) { try { ms.setActionHandler(a, null); } catch (e) {} });
      try { ms.metadata = null; ms.playbackState = "none"; } catch (e) {}
    }
    function wireMedia() {
      var ms = navigator.mediaSession;
      if (!ms) return;
      var on = function (action, fn) { try { ms.setActionHandler(action, fn); } catch (e) {} };
      on("play", function () { if (state === "paused") resume(); else if (state === "idle" && tunes.length) play(current >= 0 ? current : 0); });
      on("pause", function () { if (state === "playing") pause(); });
      on("nexttrack", function () { next(); });
      on("previoustrack", function () { previous(); });
      on("stop", function () { stop(); });
      on("seekto", function (d) { if (d && isFinite(d.seekTime)) seekTo(d.seekTime); });
    }

    // ---------- the sound ----------
    function audio() {
      if (ctx) return Promise.resolve();
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC || !AC.prototype || !("audioWorklet" in AC.prototype)) return Promise.reject(new Error("unsupported"));
      ctx = new AC();
      gain = ctx.createGain();
      gain.gain.value = muted ? 0 : volume / 100;
      // at ?silent=1 the sound passes through a gain of 0 (CLAUDE.md: tests in a pane that may be hidden): the speakers get exact
      // zeros, and the graph still reaches the destination, without which the browser never runs the worklet
      if (silentPage()) {
        var mute = ctx.createGain();
        mute.gain.value = 0;
        gain.connect(mute);
        mute.connect(ctx.destination);
      } else gain.connect(ctx.destination);
      var url = URL.createObjectURL(new Blob([WORKLET], { type: "text/javascript" }));
      return ctx.audioWorklet.addModule(url).then(function () {
        URL.revokeObjectURL(url);
        node = new AudioWorkletNode(ctx, "byopop-music-out", { numberOfInputs: 0, outputChannelCount: [2] });
        route();
        // the worklet's messages come in order, so those before its answer to a flush are about the tune before
        node.port.onmessage = function (e) {
          if (e.data === "flushed") flushes--;
          else if (flushes > 0) return;
          else if (e.data === 1) { queued--; heads.shift(); shiftedAt = ctx.currentTime; fill(); }
          else if (e.data === 0 && ended && state === "playing") afterEnd();
        };
      });
    }
    // The worklet to the volume, through the spectrum's analyser while the spectrum is shown (before the volume, so its bars are
    // the tune's own)
    function route() {
      if (!node) return;
      try { node.disconnect(); } catch (e) {}
      if (analyser) { try { analyser.disconnect(); } catch (e) {} }
      if (spectrumOn) {
        if (!analyser) { analyser = ctx.createAnalyser(); analyser.fftSize = 2048; analyser.smoothingTimeConstant = 0.5; analyser.minDecibels = -85; analyser.maxDecibels = -15; }
        node.connect(analyser);
        analyser.connect(gain);
      } else node.connect(gain);
    }
    // The sources a tune plays from: next(frames, marks) gives the next stereo frames (fewer at the end of a repeat, null once it
    // ended), and with marks where the decoder was in its patterns every SLICE frames: [frame, pattern, row, rows, channels, order]
    // A module: a subsong (libopenmpt repeats it), or a range of orders, from its first order until the order after its last
    // gain: the game's own level for its modules (web/soundtracks.js's "level", platform/MusicLevels.h through the decoder)
    var viewBuf = 0;
    function moduleSource(m, range, gain, start) {
      var again = false;
      if (range) { M._bm_loop(m, 1); M._bm_set_order(m, range[0]); }
      return {
        repeat: function (on) { again = on; if (!range) M._bm_loop(m, on ? 1 : 0); },
        next: function (frames, marks) {
          if (range && M._bm_order(m) > range[1]) {
            if (!again) return null;
            M._bm_set_order(m, range[0]);
          }
          var out = M._malloc(frames * 8), got = 0;
          if (!marks) got = M._bm_render(m, ctx.sampleRate, frames, out);
          else {
            if (!viewBuf) viewBuf = M._malloc(20);
            while (got < frames) {
              M._bm_view(m, viewBuf);
              var w = viewBuf >> 2;
              marks.push([got, M.HEAP32[w], M.HEAP32[w + 1], M.HEAP32[w + 2], M.HEAP32[w + 3], M.HEAP32[w + 4]]);
              var n = Math.min(SLICE, frames - got), g = M._bm_render(m, ctx.sampleRate, n, out + got * 8);
              got += g;
              if (g < n) break;
            }
          }
          var c = got > 0 ? M.HEAPF32.slice(out >> 2, (out >> 2) + got * 2) : null;
          M._free(out);
          if (c && gain !== 1) for (var i = 0; i < c.length; i++) c[i] *= gain;
          return c;
        },
        // seconds into the tune
        position: function () { return M._bm_position(m) - start; },
        seek: function (s) { M._bm_seek(m, ctx.sampleRate, start + s); },
        rowText: function (pattern, row) { return M.UTF8ToString(M._bm_row_text(m, pattern, row)); },
        // the channels that play a note anywhere in the module, by number
        used: function () {
          var buf = M._malloc(256), n = Math.min(256, M._bm_used(m, buf, 256)), list = [];
          for (var c = 0; c < n; c++) if (M.HEAPU8[buf + c]) list.push(c);
          M._free(buf);
          return list;
        },
        free: function () { M._bm_free(m); }
      };
    }
    // Decoded sound: interleaved stereo at the context's rate
    function pcmSource(data) {
      var at = 0, again = false, total = data.length >> 1;
      return {
        repeat: function (on) { again = on; },
        next: function (frames) {
          if (at >= total) {
            if (!again || !total) return null;
            at = 0;
          }
          var n = Math.min(frames, total - at), c = data.slice(at * 2, (at + n) * 2);
          at += n;
          return c;
        },
        position: function () { return at / ctx.sampleRate; },
        seek: function (s) { at = Math.max(0, Math.min(total, Math.round(s * ctx.sampleRate))); },
        free: function () {}
      };
    }
    // A game's music level as a gain: {bassAmplify: n} the BASS amplify the game sets, {fmod3XM: true} FMOD 3's XM pre-amp
    function levelGain(M, level) {
      if (!level) return 1;
      if (level.bassAmplify !== undefined) return M._bm_level_bass_amplify(level.bassAmplify);
      if (level.fmod3XM) return M._bm_level_fmod3_xm();
      return 1;
    }
    // The source of a tune, from its files (a joined tune's pieces decoded once each, then put one after the other)
    function sourceOf(t, files) {
      if (t.kind === "module") {
        var m = files[0] ? loadModule(M, files[0]) : 0;
        if (!m) throw new Error("module");
        if (!t.range) M._bm_select(m, t.sub);
        return Promise.resolve(moduleSource(m, t.range, levelGain(M, t.level), t.start || 0));
      }
      var decoded = {};
      return t.keys.reduce(function (p, key, i) {
        return p.then(function () {
          if (decoded[key] || !files[i]) return;
          return decodeOgg(M, ctx, files[i]).then(function (d) { decoded[key] = d; });
        });
      }, Promise.resolve()).then(function () {
        var parts = t.keys.map(function (key) { return decoded[key]; });
        if (parts.some(function (d) { return !d; })) throw new Error("ogg");
        var all = new Float32Array(parts.reduce(function (n, d) { return n + d.length; }, 0)), at = 0;
        parts.forEach(function (d) { all.set(d, at); at += d.length; });
        return pcmSource(all);
      });
    }
    // the tracker view wants the decoder's rows: shown, on the screen, and the tune a module's
    function marking() { return trackerOn && isModule() && onScreen(); }
    // Keeps AHEAD chunks queued in the worklet while playing
    function fill() {
      if (state !== "playing" || !source || ended) return;
      if (!heads.length) shiftedAt = ctx.currentTime;
      while (queued < AHEAD) {
        var at = source.position(), marks = marking() ? [] : null;
        var chunk = source.next(CHUNK, marks);
        if (!chunk) { ended = true; return; }
        heads.push({ frames: chunk.length >> 1, at: at, marks: marks });
        node.port.postMessage(chunk, [chunk.buffer]);
        queued++;
      }
    }
    // Where the listener is: the chunk the worklet plays and how far into it (from the context's clock since it began it)
    function heard() {
      var h = heads[0];
      if (!h || !ctx) return null;
      var into = Math.max(0, Math.min(h.frames - 1, Math.round((ctx.currentTime - shiftedAt) * ctx.sampleRate)));
      return { head: h, into: into, seconds: h.at + into / ctx.sampleRate };
    }
    // Empties the worklet (a seek, another tune)
    function flush() {
      if (node && queued) { flushes++; node.port.postMessage("flush"); }
      queued = 0; ended = false; heads = [];
    }
    // A tune that ended (repeat off): the next one, after the last the first again
    function afterEnd() { play((current + 1) % tunes.length); }
    function unload() {
      if (source) { source.free(); source = null; }
      flush();
      rowCache = {}; usedChannels = null; firstChannel = 0;
    }
    function play(i) {
      var t = tunes[i];
      if (!t) return;
      unload();
      current = i;
      setState("loading");
      var ticket = {};
      play.ticket = ticket;
      if (!wired) { wired = true; wireMedia(); }
      (M ? Promise.resolve() : decoder().then(function (m) { M = m; })).then(audio).then(function () {
        return openExisting(t.game.persist).then(function (db) {
          if (!db) throw new Error("gone");
          var unique = t.keys.filter(function (k, j) { return t.keys.indexOf(k) === j; }), got = {};
          return unique.reduce(function (p, key) {
            return p.then(function () { return readFile(db, key).then(function (b) { got[key] = b; }); });
          }, Promise.resolve()).then(function () { db.close(); return t.keys.map(function (k) { return got[k]; }); },
            function (e) { db.close(); throw e; });
        });
      }).then(function (files) {
        if (play.ticket !== ticket) return null;  // another tune was chosen meanwhile
        return sourceOf(t, files);
      }).then(function (s) {
        if (!s) return;
        if (play.ticket !== ticket) { s.free(); return; }
        source = s;
        source.repeat(loop);
        return ctx.resume().then(function () { setState("playing"); fill(); });
      }).catch(function (e) {
        if (play.ticket !== ticket) return;
        unload();
        setState("idle");
        var why = e && e.message === "unsupported" ? "music.unsupported" : "music.failed";
        say(T("music.lcd_error"), T(why));
      });
    }
    function pause() { if (ctx) ctx.suspend(); setState("paused"); }
    function resume() { if (ctx) ctx.resume(); setState("playing"); fill(); }
    // Stop: the tune stays chosen, back at its start (Play plays it from there)
    function stop() { play.ticket = {}; unload(); if (ctx) ctx.suspend(); setState("idle"); }
    // The window closed (the user, 2026-11-09: "Music should quit playing if the window is closed"): the music stops and its
    // sound goes, the context closed with its worklet and graph, the decoder let go and the media keys handed back; the tune
    // stays chosen, so the window opened again shows it stopped. Minimizing keeps it playing, as a minimized player did.
    var wired = false;
    function shutdown() {
      play.ticket = {};
      unload();
      if (viewBuf && M) try { M._free(viewBuf); } catch (e) {}
      viewBuf = 0;
      if (node) { try { node.port.onmessage = null; node.disconnect(); } catch (e) {} node = null; }
      if (analyser) { try { analyser.disconnect(); } catch (e) {} analyser = null; }
      if (gain) { try { gain.disconnect(); } catch (e) {} gain = null; }
      if (ctx) { try { ctx.close(); } catch (e) {} ctx = null; }
      queued = 0; flushes = 0; ended = false; heads = [];
      M = null;
      dropDecoder();
      if (wired) { wired = false; unwireMedia(); }
      setState("idle");
    }
    function next() { if (tunes.length) play((current + 1) % tunes.length); }
    // Previous, as players do: the tune's own start once it has played a few seconds, else the tune before
    function previous() {
      if (!tunes.length) return;
      if (source && current >= 0 && elapsed() > 3) { seekTo(0); return; }
      play(current < 0 ? tunes.length - 1 : (current - 1 + tunes.length) % tunes.length);
    }
    function seekTo(s) {
      if (!source) return;
      var t = tunes[current];
      source.seek(Math.max(0, Math.min(t ? t.seconds : s, s)));
      flush();
      if (state === "playing") fill();
      showTime();
      tick();
    }

    // ---------- the time and the views ----------
    function elapsed() {
      var h = heard();
      return h ? h.seconds : source ? source.position() : 0;
    }
    var seeking = false, shownClock = "";
    function showTime() {
      var t = tunes[current], total = t && state !== "idle" ? t.seconds : 0, at = state === "idle" ? 0 : Math.max(0, Math.min(total, elapsed()));
      if (!seeking) { seek.max = String(Math.max(1, total)); seek.value = String(at); }
      var text = clock(at) + "/" + clock(total);
      if (text !== shownClock) {
        shownClock = text;
        if (!seeking) timeEl.textContent = clock(at);
        totalEl.textContent = clock(total);
        seek.setAttribute("aria-valuetext", T("music.position", { elapsed: clock(at), total: clock(total) }));
      }
    }
    // The tracker view: the rows around the one heard, the cells from the decoder (each row read once a tune)
    var rowCache = {}, shownRow = "";
    // the channels shown: only those with a note somewhere (read once a tune), as many whole columns as the view's width holds,
    // from firstChannel (the scroll bar's); each cell whole, or not shown
    var usedChannels = null, firstChannel = 0, shownChannels = 0, totalChannels = 0, measure = null;
    // a character's width in the rows' own font, measured where the rows are drawn (a canvas's font can resolve to another face)
    function charWidth() {
      if (!measure) { measure = make("span", "", "0000000000"); measure.style.cssText = "position:absolute;visibility:hidden;white-space:pre"; }
      trackerRows.appendChild(measure);
      var w = measure.getBoundingClientRect().width / 10;
      trackerRows.removeChild(measure);
      return w || 6;
    }
    // a cell in the narrow view: its note and instrument, the first six characters of the decoder's cell ("C-5 01"; libopenmpt
    // writes the volume straight after the instrument, so the cell's spaces do not divide it)
    function narrow(cell) { return cell.slice(0, 6); }
    function pad(text, n) { while (text.length < n) text += " "; return text.slice(0, n); }
    function rowOf(pattern, row) {
      var k = pattern + ":" + row;
      if (!(k in rowCache)) rowCache[k] = source.rowText(pattern, row);
      return rowCache[k];
    }
    function two(n) { return (n < 10 ? "0" : "") + n; }
    function rowNumber(n, rows) { var t = String(n); while (t.length < String(rows - 1).length) t = "0" + t; return t; }
    function drawTracker() {
      if (tracker.hidden || !source || !source.rowText) return;
      var h = heard(), mark = null;
      if (h && h.head.marks) h.head.marks.forEach(function (m) { if (m[0] <= h.into) mark = m; });
      if (!mark) return;
      var cells = rowOf(mark[1], mark[2]).split("|");
      if (!usedChannels) usedChannels = source.used ? source.used() : [];
      var used = usedChannels.filter(function (c) { return c < cells.length; });
      if (!used.length) used = cells.map(function (x, c) { return c; });
      // the full cell (note, instrument, volume, effect) when every channel fits with it, else note and instrument
      var full = 0, small = 0;
      cells.forEach(function (c) { full = Math.max(full, c.length); small = Math.max(small, narrow(c).length); });
      var cw = charWidth(), room = Math.floor((trackerRows.clientWidth - 8) / cw), lead = String(mark[3] - 1).length + 1;
      function fits(width) { return Math.max(0, Math.floor((room - lead + 1) / (width + 1))); }
      var wide = fits(full) >= used.length, width = wide ? full : small, shown = Math.max(1, Math.min(used.length, fits(width)));
      firstChannel = Math.max(0, Math.min(firstChannel, used.length - shown));
      shownChannels = shown; totalChannels = used.length;
      var key = [mark[1], mark[2], firstChannel, shown, width, room].join(":");
      if (key === shownRow) return;
      shownRow = key;
      var at = T("music.tracker_at", { order: two(mark[5]), pattern: two(mark[1]), row: two(mark[2]), rows: two(mark[3]) });
      if (shown < used.length) at += "  " + T("music.tracker_ch", { first: firstChannel + 1, last: firstChannel + shown, count: used.length });
      trackerAt.textContent = at;
      var showing = used.slice(firstChannel, firstChannel + shown);
      trackerRows.textContent = "";
      var above = Math.floor((rows - 1) / 2);
      for (var r = mark[2] - above; r < mark[2] - above + rows; r++) {
        var line = make("span", r === mark[2] ? "mu-row mu-row-now" : "mu-row");
        if (r >= 0 && r < mark[3]) {
          var row = rowOf(mark[1], r).split("|");
          line.textContent = rowNumber(r, mark[3]) + " " + showing.map(function (c) { return pad(wide ? (row[c] || "") : narrow(row[c] || ""), width); }).join("│");
        } else line.textContent = " ";
        trackerRows.appendChild(line);
      }
      trackerScroll.hidden = shown >= used.length;
      placeThumb();
    }
    // The spectrum: BARS bars over 40 Hz to 16 kHz spaced as the ear hears them, each with a peak that falls back
    var bins = null, peaks = [], levels = [];
    function drawSpectrum() {
      if (spectrum.hidden || !analyser) return;
      var w = spectrum.clientWidth, hgt = spectrum.clientHeight, dpr = window.devicePixelRatio || 1;
      if (!w || !hgt) return;
      if (spectrum.width !== Math.round(w * dpr) || spectrum.height !== Math.round(hgt * dpr)) { spectrum.width = Math.round(w * dpr); spectrum.height = Math.round(hgt * dpr); }
      if (!bins || bins.length !== analyser.frequencyBinCount) bins = new Uint8Array(analyser.frequencyBinCount);
      if (state === "playing") analyser.getByteFrequencyData(bins); else bins.fill(0);
      var g = spectrum.getContext("2d"), cs = getComputedStyle(spectrum);
      var barColour = cs.getPropertyValue("--mu-bar").trim() || cs.color, peakColour = cs.getPropertyValue("--mu-peak").trim() || barColour;
      g.setTransform(dpr, 0, 0, dpr, 0, 0);
      g.clearRect(0, 0, w, hgt);
      var hz = ctx.sampleRate / analyser.fftSize, gap = 2, bw = (w - gap * (BARS + 1)) / BARS;
      for (var i = 0; i < BARS; i++) {
        var lo = 40 * Math.pow(400, i / BARS), hi = 40 * Math.pow(400, (i + 1) / BARS), v = 0;
        for (var b = Math.floor(lo / hz); b <= Math.min(bins.length - 1, Math.ceil(hi / hz)); b++) v = Math.max(v, bins[b]);
        var level = v / 255;
        levels[i] = Math.max(level, (levels[i] || 0) - 0.04);
        peaks[i] = Math.max(levels[i], (peaks[i] || 0) - 0.01);
        var x = gap + i * (bw + gap), bh = Math.round(levels[i] * (hgt - 4));
        g.fillStyle = barColour;
        g.fillRect(x, hgt - bh, bw, bh);
        g.fillStyle = peakColour;
        g.fillRect(x, hgt - 2 - Math.round(peaks[i] * (hgt - 4)), bw, 2);
      }
    }
    // One look a frame while a tune plays (the browser runs none for a page in the background), the views drawn only while the
    // player is on the screen; none at all while nothing plays and the spectrum's bars have fallen
    var ticking = false;
    function tick() {
      if (ticking) return;
      ticking = true;
      requestAnimationFrame(function frame() {
        ticking = false;
        showTime();
        if (onScreen()) { drawTracker(); drawSpectrum(); }
        var settling = levels.some(function (l, i) { return l > 0.001 || peaks[i] > 0.001; });
        if (state === "playing" || (!spectrum.hidden && settling)) { ticking = true; requestAnimationFrame(frame); }
      });
    }

    playBtn.addEventListener("click", function () {
      if (state === "playing" || state === "loading") pause();
      else if (state === "paused") resume();
      else if (tunes.length) play(current >= 0 ? current : 0);
    });
    prevBtn.addEventListener("click", previous);
    stopBtn.addEventListener("click", stop);
    nextBtn.addEventListener("click", next);
    seek.addEventListener("input", function () {
      seeking = true;
      timeEl.textContent = clock(+seek.value);
    });
    seek.addEventListener("change", function () { seeking = false; seekTo(+seek.value); });
    function keep(key, on) { try { if (on) localStorage.setItem(key, "1"); else localStorage.removeItem(key); } catch (e) {} }
    loopBtn.addEventListener("click", function () {
      loop = !loop;
      loopBtn.setAttribute("aria-pressed", String(loop));
      keep(LOOP_KEY, loop);
      if (source) source.repeat(loop);
    });
    function setGain() { if (gain) gain.gain.value = muted ? 0 : volume / 100; }
    muteBtn.addEventListener("click", function () {
      muted = !muted;
      muteBtn.setAttribute("aria-pressed", String(muted));
      muteBtn.replaceChild(glyph(muted ? "muted" : "speaker"), muteBtn.firstChild);
      keep(MUTE_KEY, muted);
      setGain();
    });
    vol.addEventListener("input", function () {
      volume = +vol.value;
      if (muted && volume > 0) muteBtn.click();
      setGain();
      try { localStorage.setItem(VOLUME_KEY, String(volume)); } catch (e) {}
    });
    // a click on the view, Enter or Space: the next view (the chunks already queued were made without the tracker's rows, so the
    // tracker starts with the next ones)
    function nextVis() {
      vis = VIS[(VIS.indexOf(vis) + 1) % VIS.length];
      try { localStorage.setItem(VIS_KEY, vis); } catch (e) {}
      applyVis();
      tick();
    }
    setRows(rows, false);
    // the room changes with the browser's window and the Music window: the rows asked for, as far as they fit again
    var asked = rows;
    window.addEventListener("resize", function () { if (!splitDrag) setRows(asked, false); });
    if (window.ResizeObserver) try { new ResizeObserver(function () { if (!splitDrag) setRows(asked, false); }).observe(root); } catch (e) {}
    visEl.addEventListener("click", function (ev) { if (!trackerScroll.contains(ev.target)) nextVis(); });
    // the bar: the thumb's size is the share of the channels shown, its place the first one's
    function placeThumb() {
      if (trackerScroll.hidden) return;
      var room = hbTrack.clientWidth, size = Math.max(8, Math.round(room * shownChannels / totalChannels));
      hbThumb.style.width = size + "px";
      hbThumb.style.left = Math.round((room - size) * firstChannel / Math.max(1, totalChannels - shownChannels)) + "px";
    }
    function scrollTo(first) {
      first = Math.max(0, Math.min(first, totalChannels - shownChannels));
      if (first !== firstChannel) { firstChannel = first; shownRow = ""; drawTracker(); }
    }
    // an arrow moves a channel, and again and again while it is held; the track a page of channels toward the click; the thumb
    // drags
    var repeat = 0;
    function stopRepeat() { clearTimeout(repeat); repeat = 0; [hbLeft, hbRight, hbTrack].forEach(function (e) { e.classList.remove("mu-down"); }); }
    [[hbLeft, -1], [hbRight, 1]].forEach(function (a) {
      a[0].addEventListener("pointerdown", function (ev) {
        ev.preventDefault(); ev.stopPropagation();
        a[0].classList.add("mu-down");
        (function step(wait) { scrollTo(firstChannel + a[1]); repeat = setTimeout(function () { step(80); }, wait); })(400);
      });
    });
    hbTrack.addEventListener("pointerdown", function (ev) {
      if (ev.target === hbThumb) return;
      ev.preventDefault(); ev.stopPropagation();
      var toward = ev.clientX < hbThumb.getBoundingClientRect().left ? -1 : 1;
      scrollTo(firstChannel + toward * Math.max(1, shownChannels - 1));
    });
    hbThumb.addEventListener("pointerdown", function (ev) {
      ev.preventDefault(); ev.stopPropagation();
      var startX = ev.clientX, startFirst = firstChannel, room = hbTrack.clientWidth - hbThumb.offsetWidth;
      try { hbThumb.setPointerCapture(ev.pointerId); } catch (e) {}
      function move(e) { if (room > 0) scrollTo(startFirst + Math.round((e.clientX - startX) / room * (totalChannels - shownChannels))); }
      function up() { hbThumb.removeEventListener("pointermove", move); hbThumb.removeEventListener("pointerup", up); hbThumb.removeEventListener("pointercancel", up); }
      hbThumb.addEventListener("pointermove", move); hbThumb.addEventListener("pointerup", up); hbThumb.addEventListener("pointercancel", up);
    });
    ["pointerup", "pointercancel", "pointerleave"].forEach(function (n) { trackerScroll.addEventListener(n, stopRepeat); });
    // Left and Right on the display move the tracker by a channel, as the bar's arrows do
    visEl.addEventListener("keydown", function (ev) {
      if (tracker.hidden || trackerScroll.hidden || (ev.key !== "ArrowLeft" && ev.key !== "ArrowRight")) return;
      ev.preventDefault();
      scrollTo(firstChannel + (ev.key === "ArrowLeft" ? -1 : 1));
    });
    if (window.ResizeObserver) new ResizeObserver(function () { if (!tracker.hidden) { shownRow = ""; drawTracker(); } }).observe(visEl);
    visEl.addEventListener("keydown", function (ev) { if (ev.key === "Enter" || ev.key === " ") { ev.preventDefault(); nextVis(); } });

    applyVis();

    // ---------- the list ----------
    function render() {
      list.textContent = "";
      var byGame = [];
      tunes.forEach(function (t, i) {
        var g = byGame.filter(function (x) { return x.game === t.game; })[0];
        if (!g) { g = { game: t.game, items: [] }; byGame.push(g); }
        g.items.push(i);
      });
      byGame.forEach(function (g) {
        var h = make("h3", "mu-game", g.game.title), ol = make("ol", "mu-tunes");
        h.id = "music-game-" + g.game.id;
        ol.setAttribute("aria-labelledby", h.id);
        g.items.forEach(function (i) {
          var li = make("li"), b = make("button", "mu-tune");
          b.type = "button";
          b.setAttribute("data-tune", String(i));
          b.appendChild(make("span", "mu-num", String(g.items.indexOf(i) + 1)));
          var name = make("span", "mu-name", tunes[i].name);
          name.title = tunes[i].name;
          b.appendChild(name);
          b.appendChild(make("span", "mu-time", clock(tunes[i].seconds)));
          b.addEventListener("click", function () { play(i); });
          li.appendChild(b);
          ol.appendChild(li);
        });
        list.appendChild(h);
        list.appendChild(ol);
      });
    }
    // The games the player has, with music in the table, read once, the first time the tab is shown
    function scan() {
      if (scanned) return;
      scanned = true;
      var table = typeof ByopopSoundtracks !== "undefined" ? ByopopSoundtracks : {}, files = {};
      try { files = ByopopGameFiles.all(); } catch (e) {}
      var mine = games.filter(function (g) { return table[g.id] && ByopopGameFiles.present(files[g.id]); });
      if (!mine.length) { say(T("music.lcd_none"), T("music.none")); return; }
      say(T("music.lcd_reading"), T("music.reading"));
      decoder().then(function (m) {
        M = m;
        return mine.reduce(function (p, g) {
          return p.then(function () { return tunesOf(M, g, table[g.id]).then(function (t) { tunes = tunes.concat(t); }, function () {}); });
        }, Promise.resolve());
      }).then(function () {
        render();
        if (!tunes.length) { say(T("music.lcd_none"), T("music.none")); return; }
        wired = true;
        wireMedia();
        setState("idle");
      }, function () { scanned = false; say(T("music.lcd_error"), T("music.failed")); });
    }
    // the player shown: read the games' music then (the user, 2026-11-09: "Music seems to have vanished in my browser"). The
    // tab's panel is hidden until its tab is chosen (a narrow screen); Desktop 95 lifts the panel into a window whatever its
    // hidden attribute says (web/skins.js), so the window opening (its class) or the player's own box counts too
    var panel = root.closest("[role=tabpanel]") || root;
    function shown() {
      return !panel.hidden || (panel.classList.contains("d95-win") && !panel.classList.contains("d95-closed")) || root.getClientRects().length > 0;
    }
    if (shown()) scan();
    else {
      var watch = new MutationObserver(function () { if (shown()) { watch.disconnect(); if (seen) seen.disconnect(); scan(); } });
      watch.observe(panel, { attributes: true, attributeFilter: ["hidden", "class"] });
      var seen = window.ResizeObserver ? new ResizeObserver(function () { if (shown()) { seen.disconnect(); watch.disconnect(); scan(); } }) : null;
      if (seen) seen.observe(root);
    }
    // for tests: the state, the tunes, and how far the decoder has rendered the tune (it moves only as the worklet plays)
    return { state: function () { return state; }, tunes: function () { return tunes.slice(); },
      position: function () { return source ? source.position() : 0; }, elapsed: function () { return elapsed(); },
      seek: function (s) { seekTo(s); }, minHeight: windowLeast, shutdown: shutdown,
      // for tests: whether any of the sound is still held (the context and its state, the worklet, the decoder)
      held: function () { return { ctx: ctx ? ctx.state : null, node: !!node, decoder: !!M, media: wired }; }, view: function () { return { tracker: !tracker.hidden, row: shownRow, spectrum: !!analyser && !spectrum.hidden, rows: rows }; } };
  }

  // (the Sounds window, web/sounds.js, reads the games' files with the same read-only reader, draws its Stop and speaker with the
  // same pictures, and falls back on the same stb_vorbis decoder)
  return { init: init, patternRe: patternRe, glyph: glyph, decoder: decoder, stbOgg: stbOgg,
    files: { open: openExisting, list: gameFiles, read: readFile } };
})();
