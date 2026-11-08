// The music player (docs/WEB_PLAYER.md, "The music player"; the user, 2026-11-07): the listing page's Music tab plays the music of
// the games the player added, from the game's own files in this browser (each game's database, as the import wrote them). Nothing is
// shipped with the site and nothing is sent anywhere. The tunes of a game are the subsongs of its module files (web/soundtracks.js),
// decoded by libopenmpt built on its own for the web (openmpt.js, web/music/openmpt_web.c, loaded the first time the tab opens) and
// played through an AudioWorklet. Play, pause, the next tune, repeat and the volume are on the page, and the Media Session API
// gives them to the media keys and the system's media controls. At ?silent=1 nothing reaches the speakers.
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
  // The files of a game's folder that match its patterns: [{rel, key}], in the order of the patterns, then by name
  function musicFiles(db, persist, patterns) {
    var prefix = persist + "/game/", res = patterns.map(patternRe);
    return new Promise(function (resolve, reject) {
      var tx = db.transaction(DB_STORE, "readonly"), r = tx.objectStore(DB_STORE).getAllKeys(IDBKeyRange.bound(prefix, prefix + "￿"));
      r.onerror = function () { reject(r.error); };
      r.onsuccess = function () {
        var found = [];
        res.forEach(function (re) {
          r.result.map(String).filter(function (k) { return re.test(k.substring(prefix.length)); }).sort().forEach(function (k) {
            if (!found.some(function (f) { return f.key === k; })) found.push({ rel: k.substring(prefix.length), key: k });
          });
        });
        resolve(found);
      };
    });
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

  // ---------- the decoder ----------
  var decoderPromise = null;
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

  // ---------- the tunes ----------
  // A file's name without its folder and extension
  function baseName(rel) { return rel.split("/").pop().replace(/\.[^.]+$/, ""); }
  function clock(seconds) {
    var s = Math.round(seconds), m = Math.floor(s / 60);
    return T("music.duration", { min: m, sec: (s % 60 < 10 ? "0" : "") + (s % 60) });
  }
  // A game's tunes: [{game, rel, key, sub, name, seconds}], each file's subsongs of MIN_TUNE seconds or more, less its skip list
  function tunesOf(M, game, entry) {
    return openExisting(game.persist).then(function (db) {
      if (!db) return [];
      return musicFiles(db, game.persist, entry.files).then(function (files) {
        var tunes = [];
        return files.reduce(function (p, f) {
          return p.then(function () {
            return readFile(db, f.key).then(function (bytes) {
              if (!bytes) return;
              var m = loadModule(M, bytes);
              if (!m) return;
              var n = M._bm_subsongs(m), kept = [];
              for (var i = 0; i < n; i++) {
                M._bm_select(m, i);
                var d = M._bm_duration(m), id = baseName(f.rel) + "#" + i;
                if (d >= MIN_TUNE && (entry.skip || []).indexOf(id) < 0) kept.push({ sub: i, seconds: d, id: id });
              }
              M._bm_free(m);
              kept.forEach(function (k, j) {
                var named = entry.names && entry.names[k.id];
                tunes.push({ game: game, rel: f.rel, key: f.key, sub: k.sub, seconds: k.seconds,
                  name: named || (kept.length > 1 ? T("music.tune_part", { file: baseName(f.rel), n: j + 1 }) : baseName(f.rel)) });
              });
            });
          });
        }, Promise.resolve()).then(function () { db.close(); return tunes; }, function (e) { db.close(); throw e; });
      });
    });
  }

  // ---------- playing ----------
  // The worklet: plays the stereo chunks it is sent, in order, and says when it has used one up (1) and when it ran dry (0)
  var WORKLET = "class ByopopMusicOut extends AudioWorkletProcessor {\n" +
    "  constructor() { super(); this.q = []; this.at = 0; this.dry = true;\n" +
    "    this.port.onmessage = (e) => { if (e.data === 'flush') { this.q = []; this.at = 0; } else { this.q.push(e.data); this.dry = false; } }; }\n" +
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

  function init(root, games) {
    var M = null, ctx = null, node = null, gain = null;
    var tunes = [], current = -1, module = 0, queued = 0, ended = false;
    var state = "idle";  // idle | loading | playing | paused
    var scanned = false, loop = false, volume = 80;
    try { loop = localStorage.getItem(LOOP_KEY) === "1"; } catch (e) {}
    try { var v = parseInt(localStorage.getItem(VOLUME_KEY), 10); if (v >= 0 && v <= 100) volume = v; } catch (e) {}

    root.textContent = "";
    root.classList.add("mu");
    var about = make("p", "mu-about");
    about.innerHTML = ByopopStrings.html("music.about_html");
    root.appendChild(about);
    var status = make("p", "mu-status");
    status.setAttribute("role", "status");
    root.appendChild(status);
    var bar = make("div", "mu-bar");
    var playBtn = make("button", "mu-play", T("music.play"));
    playBtn.type = "button";
    var nextBtn = make("button", "mu-next", T("music.next"));
    nextBtn.type = "button";
    var loopLabel = make("label", "mu-loop"), loopBox = make("input");
    loopBox.type = "checkbox";
    loopBox.id = "music-loop";
    loopBox.checked = loop;
    loopLabel.appendChild(loopBox);
    loopLabel.appendChild(document.createTextNode(" " + T("music.loop")));
    var volLabel = make("label", "mu-volume"), vol = make("input");
    vol.type = "range";
    vol.id = "music-volume";
    vol.min = "0"; vol.max = "100"; vol.step = "1"; vol.value = String(volume);
    volLabel.appendChild(document.createTextNode(T("music.volume") + " "));
    volLabel.appendChild(vol);
    [playBtn, nextBtn, loopLabel, volLabel].forEach(function (e) { bar.appendChild(e); });
    root.appendChild(bar);
    var list = make("div", "mu-list");
    root.appendChild(list);
    playBtn.disabled = nextBtn.disabled = true;

    function say(text) { status.textContent = text; }
    function nowText() {
      var t = tunes[current];
      if (!t) return T("music.stopped");
      var words = T(state === "paused" ? "music.paused" : state === "loading" ? "music.loading_tune" : "music.now", { tune: t.name, game: t.game.title });
      return silentPage() ? words + " " + T("music.silent") : words;
    }
    // The one place the state changes: the buttons, the list and the media session follow it
    function setState(next) {
      state = next;
      playBtn.textContent = T(state === "playing" || state === "loading" ? "music.pause" : "music.play");
      playBtn.disabled = !tunes.length;
      nextBtn.disabled = tunes.length < 2;
      Array.prototype.forEach.call(list.querySelectorAll("button[data-tune]"), function (b) {
        if (+b.getAttribute("data-tune") === current && state !== "idle") b.setAttribute("aria-current", "true"); else b.removeAttribute("aria-current");
      });
      say(nowText());
      media();
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
    function wireMedia() {
      var ms = navigator.mediaSession;
      if (!ms) return;
      var on = function (action, fn) { try { ms.setActionHandler(action, fn); } catch (e) {} };
      on("play", function () { if (state === "paused") resume(); else if (state === "idle" && tunes.length) play(0); });
      on("pause", function () { if (state === "playing") pause(); });
      on("nexttrack", function () { if (tunes.length) play((current + 1) % tunes.length); });
      on("previoustrack", function () { if (tunes.length) play((current - 1 + tunes.length) % tunes.length); });
      on("stop", function () { stop(); });
    }

    // ---------- the sound ----------
    function audio() {
      if (ctx) return Promise.resolve();
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC || !AC.prototype || !("audioWorklet" in AC.prototype)) return Promise.reject(new Error("unsupported"));
      ctx = new AC();
      gain = ctx.createGain();
      gain.gain.value = volume / 100;
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
        node.connect(gain);
        node.port.onmessage = function (e) {
          if (e.data === 1) { queued--; fill(); }
          else if (e.data === 0 && ended && state === "playing") afterEnd();
        };
      });
    }
    // Keeps AHEAD chunks queued in the worklet while playing
    function fill() {
      if (state !== "playing" || !module || ended) return;
      while (queued < AHEAD) {
        var out = M._malloc(CHUNK * 8), got = M._bm_render(module, ctx.sampleRate, CHUNK, out);
        if (got > 0) {
          var chunk = new Float32Array(got * 2);
          chunk.set(M.HEAPF32.subarray(out >> 2, (out >> 2) + got * 2));
          node.port.postMessage(chunk, [chunk.buffer]);
          queued++;
        }
        M._free(out);
        if (got < CHUNK) { ended = true; return; }
      }
    }
    // A tune that ended (repeat off): the next one, after the last the first again
    function afterEnd() { play((current + 1) % tunes.length); }
    function unload() {
      if (module) { M._bm_free(module); module = 0; }
      if (node) node.port.postMessage("flush");
      queued = 0; ended = false;
    }
    function play(i) {
      var t = tunes[i];
      if (!t) return;
      unload();
      current = i;
      setState("loading");
      var ticket = {};
      play.ticket = ticket;
      audio().then(function () {
        return openExisting(t.game.persist).then(function (db) {
          if (!db) throw new Error("gone");
          return readFile(db, t.key).then(function (bytes) { db.close(); return bytes; });
        });
      }).then(function (bytes) {
        if (play.ticket !== ticket) return;  // another tune was chosen meanwhile
        module = bytes ? loadModule(M, bytes) : 0;
        if (!module) throw new Error("module");
        M._bm_select(module, t.sub);
        M._bm_loop(module, loop ? 1 : 0);
        return ctx.resume().then(function () { setState("playing"); fill(); });
      }).catch(function (e) {
        if (play.ticket !== ticket) return;
        unload();
        setState("idle");
        say(T(e && e.message === "unsupported" ? "music.unsupported" : "music.failed"));
      });
    }
    function pause() { if (ctx) ctx.suspend(); setState("paused"); }
    function resume() { if (ctx) ctx.resume(); setState("playing"); fill(); }
    function stop() { unload(); if (ctx) ctx.suspend(); current = -1; setState("idle"); }

    playBtn.addEventListener("click", function () {
      if (state === "playing" || state === "loading") pause();
      else if (state === "paused") resume();
      else if (tunes.length) play(current >= 0 ? current : 0);
    });
    nextBtn.addEventListener("click", function () { if (tunes.length) play((current + 1) % tunes.length); });
    loopBox.addEventListener("change", function () {
      loop = loopBox.checked;
      try { if (loop) localStorage.setItem(LOOP_KEY, "1"); else localStorage.removeItem(LOOP_KEY); } catch (e) {}
      if (module) M._bm_loop(module, loop ? 1 : 0);
    });
    vol.addEventListener("input", function () {
      volume = +vol.value;
      if (gain) gain.gain.value = volume / 100;
      try { localStorage.setItem(VOLUME_KEY, String(volume)); } catch (e) {}
    });

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
          b.appendChild(make("span", "mu-name", tunes[i].name));
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
      if (!mine.length) { say(T("music.none")); return; }
      say(T("music.reading"));
      decoder().then(function (m) {
        M = m;
        return mine.reduce(function (p, g) {
          return p.then(function () { return tunesOf(M, g, table[g.id]).then(function (t) { tunes = tunes.concat(t); }, function () {}); });
        }, Promise.resolve());
      }).then(function () {
        render();
        if (!tunes.length) { say(T("music.none")); return; }
        wireMedia();
        setState("idle");
      }, function () { scanned = false; say(T("music.failed")); });
    }
    // the tab's panel shown: read the games' music then (the panel is hidden until its tab is chosen)
    var panel = root.closest("[role=tabpanel]") || root;
    if (!panel.hidden) scan();
    else new MutationObserver(function (records, obs) { if (!panel.hidden) { obs.disconnect(); scan(); } }).observe(panel, { attributes: true, attributeFilter: ["hidden"] });
    // for tests: the state, the tunes, and how far the decoder has rendered the tune (it moves only as the worklet plays)
    return { state: function () { return state; }, tunes: function () { return tunes.slice(); },
      position: function () { return module ? M._bm_position(module) : 0; } };
  }

  return { init: init, patternRe: patternRe };
})();
