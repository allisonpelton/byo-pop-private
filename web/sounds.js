// The soundboard (docs/WEB_PLAYER.md, "The soundboard"): Desktop 95's Sounds window, on the listing's desktop. It plays the sound
// effects of the games the player added, from the games' own files in this browser, read as the music player reads them
// (ByopopMusic.files: each game's database opened read-only, without a version). The site ships no sound and sends nothing
// anywhere. Which files a game has, their groups, names and when the game plays each are in web/soundboards.js; a game with no
// entry there, or none of whose files are here, is not listed.
// The window is a soundboard of the late 1990s: a row of tools (a drop-down of the games, shown once two or more are listed;
// Stop, which silences everything playing; the speaker, which mutes, and the volume), then the games' sounds as raised push
// buttons in group boxes, the only part that scrolls, and a status bar with the count. A button plays its sound on a click, Enter
// or Space; pressed again while it plays, it starts again; different sounds overlap. Each file is decoded once, on its first play
// (the browser's decodeAudioData; stb_vorbis from the music player's wasm for an Ogg file the browser cannot decode; a Sun .au
// file here), and the decoded sounds are kept while the window is open and dropped when it closes. At ?silent=1 the sound goes
// through a final gain of 0, as the music player's does. The grid of buttons is one tab stop: the arrows walk it (Up and Down
// to the nearest button on the line above or below, across the group boxes), Home and End go to the first and last.
//   ByopopSounds.attach(desk, at)   adds the window to Desktop 95's desktop, closed (web/index.html)
var ByopopSounds = (function () {
  "use strict";
  var VOLUME_KEY = "byopop-sounds-volume", MUTE_KEY = "byopop-sounds-mute", GAME_KEY = "byopop-sounds-game";
  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function silentPage() { try { return new URLSearchParams(location.search).get("silent") === "1"; } catch (e) { return false; } }
  function make(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  function store(key, value) { try { localStorage.setItem(key, value); } catch (e) {} }
  function stored(key) { try { return localStorage.getItem(key); } catch (e) { return null; } }

  // ---- decoding ----
  // A Sun audio file (.au: BookWorm's blank.au, mu-law at 8012 Hz), which browsers do not decode: mu-law, 8-bit or 16-bit PCM
  function muLaw(u) {
    u = ~u & 0xff;
    var t = (((u & 0x0f) << 3) + 0x84) << ((u & 0x70) >> 4);
    return (u & 0x80) ? 0x84 - t : t - 0x84;
  }
  function decodeAu(ctx, bytes) {
    var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    if (bytes.length < 24 || dv.getUint32(0) !== 0x2e736e64) throw new Error("au");
    var off = dv.getUint32(4), size = dv.getUint32(8), enc = dv.getUint32(12), rate = dv.getUint32(16), ch = dv.getUint32(20);
    if (size === 0xffffffff || off + size > bytes.length) size = bytes.length - off;
    var width = enc === 3 ? 2 : (enc === 1 || enc === 2) ? 1 : 0, frames = width && ch ? Math.floor(size / (width * ch)) : 0;
    if (!frames || rate < 3000) throw new Error("au");
    var buf = ctx.createBuffer(ch, frames, rate);
    for (var c = 0; c < ch; c++) {
      var d = buf.getChannelData(c);
      for (var i = 0; i < frames; i++) {
        var p = off + (i * ch + c) * width;
        d[i] = enc === 1 ? muLaw(bytes[p]) / 32768 : enc === 2 ? dv.getInt8(p) / 128 : dv.getInt16(p) / 32768;
      }
    }
    return buf;
  }
  // A file's bytes as an AudioBuffer: the browser's decoder; for an Ogg file it cannot decode, stb_vorbis in the music player's
  // openmpt.wasm (interleaved stereo at the context's rate)
  function decode(ctx, rel, bytes) {
    if (/\.au$/i.test(rel)) return new Promise(function (resolve) { resolve(decodeAu(ctx, bytes)); });
    return ctx.decodeAudioData(bytes.slice().buffer).catch(function (e) {
      if (!/\.ogg$/i.test(rel)) throw e;
      return ByopopMusic.decoder().then(function (M) {
        var d = ByopopMusic.stbOgg(M, bytes, ctx.sampleRate), n = d.length >> 1, buf = ctx.createBuffer(2, n, ctx.sampleRate);
        var l = buf.getChannelData(0), r = buf.getChannelData(1);
        for (var i = 0; i < n; i++) { l[i] = d[i * 2]; r[i] = d[i * 2 + 1]; }
        return buf;
      });
    });
  }

  // ---- the games' files ----
  // The table's entries whose files this game's folder holds (matched without regard to case), each group with those it holds
  function boardOf(game, entry) {
    var F = ByopopMusic.files;
    return F.open(game.persist).then(function (db) {
      if (!db) return null;
      return F.list(db, game.persist).then(function (rels) {
        db.close();
        var have = {};
        rels.forEach(function (r) { have[r.toLowerCase()] = r; });
        var groups = [], count = 0;
        (entry.groups || []).forEach(function (g) {
          var sounds = [];
          g.sounds.forEach(function (s) {
            var rel = have[String(s[0]).toLowerCase()];
            if (rel) sounds.push({ rel: rel, name: s[1], when: s[2] || "" });
          });
          if (sounds.length) { groups.push({ name: g.name, sounds: sounds }); count += sounds.length; }
        });
        return count ? { game: game, groups: groups, count: count } : null;
      }, function (e) { db.close(); throw e; });
    });
  }
  function readBytes(game, rel) {
    var F = ByopopMusic.files;
    return F.open(game.persist).then(function (db) {
      if (!db) throw new Error("no database");
      return F.read(db, game.persist + "/game/" + rel).then(function (b) { db.close(); if (!b) throw new Error("no file"); return b; },
        function (e) { db.close(); throw e; });
    });
  }

  // ---- the window ----
  function build(games) {
    var el = make("section", "sb");
    el.id = "sounds";
    el.setAttribute("aria-label", T("sounds.title"));
    var muted = stored(MUTE_KEY) === "1", volume = 80;
    var v = parseInt(stored(VOLUME_KEY), 10);
    if (v >= 0 && v <= 100) volume = v;

    // the tools: the games' drop-down, Stop, then the speaker and the volume at the right
    var bar = make("div", "sb-bar");
    var pick = make("select", "sb-game");
    pick.id = "sounds-game";
    pick.setAttribute("aria-label", T("sounds.game")); pick.title = T("sounds.game");
    pick.hidden = true;
    function tool(name, label, cls) {
      var b = make("button", "sb-tool " + cls);
      b.type = "button";
      b.appendChild(ByopopMusic.glyph(name));
      b.setAttribute("aria-label", label); b.title = label;
      return b;
    }
    var stopBtn = tool("stop", T("sounds.stop"), "sb-stop");
    var muteBtn = tool(muted ? "muted" : "speaker", T("sounds.mute"), "sb-mute");
    muteBtn.setAttribute("aria-pressed", String(muted));
    var vol = make("input", "sb-vol");
    vol.type = "range"; vol.id = "sounds-volume";
    vol.min = "0"; vol.max = "100"; vol.step = "1"; vol.value = String(volume);
    vol.setAttribute("aria-label", T("sounds.volume")); vol.title = T("sounds.volume");
    [pick, stopBtn, make("span", "sb-gap"), muteBtn, vol].forEach(function (e) { bar.appendChild(e); });
    // the buttons, in their group boxes: the part that scrolls
    var grid = make("div", "sb-grid");
    // the status bar: a value (the count, or what the window is doing), its sentence in the tooltip
    var status = make("div", "sb-status");
    status.setAttribute("role", "status");
    el.appendChild(bar); el.appendChild(grid); el.appendChild(status);
    function say(word, sentence) { status.textContent = word; status.title = sentence || ""; }

    // ---- sound ----
    var ctx = null, gain = null, live = [], cache = {};
    function level() { return muted ? 0 : volume / 100; }
    function audio() {
      if (ctx) return ctx;
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return null;
      ctx = new AC();
      gain = ctx.createGain();
      gain.gain.value = level();
      // at ?silent=1 the speakers get exact zeros through a final gain of 0 (CLAUDE.md: tests in a pane that may be hidden)
      if (silentPage()) {
        var mute = ctx.createGain();
        mute.gain.value = 0;
        gain.connect(mute);
        mute.connect(ctx.destination);
      } else gain.connect(ctx.destination);
      return ctx;
    }
    function stopAll() {
      live.slice().forEach(function (s) { try { s.stop(); } catch (e) {} });
      live = [];
    }
    // the window closed: nothing plays, and the decoded sounds and the audio context go
    function release() {
      stopAll();
      cache = {};
      if (ctx) { try { ctx.close(); } catch (e) {} }
      ctx = null; gain = null;
    }
    function bufferOf(board, rel) {
      var id = board.game.id, c = cache[id] || (cache[id] = {});
      if (!c[rel]) {
        var at = ctx;
        c[rel] = readBytes(board.game, rel).then(function (bytes) { return decode(at, rel, bytes); });
        c[rel].catch(function () { if (cache[id] && cache[id][rel]) delete cache[id][rel]; });
      }
      return c[rel];
    }
    var stats = { started: 0, decoded: {}, failed: {} };
    function play(board, s, btn) {
      if (!audio()) return;
      if (ctx.state === "suspended") { try { ctx.resume(); } catch (e) {} }
      var at = ctx;
      bufferOf(board, s.rel).then(function (buf) {
        if (ctx !== at) return;   // the window closed meanwhile
        stats.decoded[s.rel] = buf.duration;
        // pressed again while it plays: it starts again
        if (btn.sbSource) { try { btn.sbSource.stop(); } catch (e) {} }
        var src = ctx.createBufferSource();
        src.buffer = buf;
        src.connect(gain);
        src.onended = function () {
          var i = live.indexOf(src);
          if (i >= 0) live.splice(i, 1);
          if (btn.sbSource === src) btn.sbSource = null;
        };
        live.push(src);
        btn.sbSource = src;
        src.start();
        stats.started++;
      }, function () {
        stats.failed[s.rel] = true;
        btn.title = s.name + "\n" + T("sounds.bad_file");
      });
    }
    stopBtn.addEventListener("click", stopAll);
    function applyLevel() { if (gain) gain.gain.value = level(); }
    muteBtn.addEventListener("click", function () {
      muted = !muted;
      store(MUTE_KEY, muted ? "1" : "0");
      muteBtn.setAttribute("aria-pressed", String(muted));
      muteBtn.replaceChild(ByopopMusic.glyph(muted ? "muted" : "speaker"), muteBtn.firstChild);
      applyLevel();
    });
    vol.addEventListener("input", function () {
      volume = parseInt(vol.value, 10) || 0;
      store(VOLUME_KEY, String(volume));
      applyLevel();
    });

    // ---- the grid ----
    var boards = [], shown = null, buttons = [], current = null;
    function render(board) {
      shown = board;
      grid.textContent = "";
      buttons = []; current = null;
      if (!board) return;
      board.groups.forEach(function (g) {
        // a group box: a fieldset named as its legend, so that screen readers give each button its group
        var box = make("fieldset", "sb-group"), legend = make("legend", "sb-legend", g.name), row = make("div", "sb-row");
        box.setAttribute("role", "group");
        box.setAttribute("aria-label", g.name);
        box.appendChild(legend);
        g.sounds.forEach(function (s) {
          var b = make("button", "sb-btn");
          b.type = "button";
          b.tabIndex = -1;
          b.appendChild(make("span", "sb-name", s.name));
          // named by its label; the tooltip adds when the game plays it (for screen readers, its description)
          b.setAttribute("aria-label", s.name);
          b.title = s.when ? s.name + "\n" + s.when : s.name;
          b.setAttribute("data-file", s.rel);
          b.addEventListener("click", function () { play(board, s, b); });
          b.addEventListener("focus", function () { setCurrent(b); });
          row.appendChild(b);
          buttons.push(b);
        });
        box.appendChild(row);
        grid.appendChild(box);
      });
      setCurrent(buttons[0] || null);
      say(T("sounds.count", { n: board.count }));
    }
    // the grid's one tab stop: the button last focused (the first at first)
    function setCurrent(b) {
      if (current && current !== b) current.tabIndex = -1;
      current = b;
      if (b) b.tabIndex = 0;
    }
    function go(b) { if (!b) return; setCurrent(b); try { b.focus(); } catch (e) {} if (b.scrollIntoView) b.scrollIntoView({ block: "nearest" }); }
    // Up and Down: the button whose middle is nearest across, on the nearest line above or below (the group boxes' lines too)
    function vertical(from, down) {
      var r = from.getBoundingClientRect(), cx = r.left + r.width / 2, best = null, bestLine = Infinity, bestDx = Infinity;
      buttons.forEach(function (b) {
        if (b === from) return;
        var q = b.getBoundingClientRect(), dy = down ? q.top - r.top : r.top - q.top;
        if (dy < r.height / 2) return;
        var dx = Math.abs(q.left + q.width / 2 - cx);
        if (dy < bestLine - 1 || (Math.abs(dy - bestLine) <= 1 && dx < bestDx)) { best = b; bestLine = dy; bestDx = dx; }
      });
      return best;
    }
    grid.addEventListener("keydown", function (ev) {
      var b = ev.target.closest && ev.target.closest(".sb-btn");
      if (!b || ev.altKey || ev.ctrlKey || ev.metaKey) return;
      var i = buttons.indexOf(b), to = null;
      if (ev.key === "ArrowRight") to = buttons[i + 1];
      else if (ev.key === "ArrowLeft") to = buttons[i - 1];
      else if (ev.key === "ArrowDown") to = vertical(b, true);
      else if (ev.key === "ArrowUp") to = vertical(b, false);
      else if (ev.key === "Home") to = buttons[0];
      else if (ev.key === "End") to = buttons[buttons.length - 1];
      else return;
      ev.preventDefault();
      go(to);
    });

    // ---- the games ----
    function choose(id) {
      var board = boards.filter(function (b) { return b.game.id === id; })[0] || boards[0] || null;
      if (board === shown) return;
      stopAll();
      if (board) { pick.value = board.game.id; store(GAME_KEY, board.game.id); }
      render(board);
    }
    pick.addEventListener("change", function () { choose(pick.value); });
    // the games, read each time the window opens (a game's files may have come or gone while it was closed)
    var scanned = false, generation = 0;
    function scan() {
      if (scanned) return;
      scanned = true;
      var gen = ++generation, found = [];
      var table = typeof ByopopSoundboards !== "undefined" ? ByopopSoundboards : {}, files = {};
      try { files = ByopopGameFiles.all(); } catch (e) {}
      var mine = games.filter(function (g) { return table[g.id] && ByopopGameFiles.present(files[g.id]); });
      if (!mine.length) { say(T("sounds.st_none"), T("sounds.none")); return; }
      say(T("sounds.st_reading"), T("sounds.reading"));
      mine.reduce(function (p, g) {
        return p.then(function () { return boardOf(g, table[g.id]).then(function (b) { if (b) found.push(b); }, function () {}); });
      }, Promise.resolve()).then(function () {
        if (gen !== generation) return;
        boards = found;
        if (!boards.length) { say(T("sounds.st_none"), T("sounds.none")); return; }
        pick.textContent = "";
        boards.forEach(function (b) { var o = make("option", null, b.game.title); o.value = b.game.id; pick.appendChild(o); });
        pick.hidden = boards.length < 2;
        choose(stored(GAME_KEY));
        // the window opened with the focus on its title bar's first button (web/skins.js, show): the grid's button instead
        var a = document.activeElement;
        if (current && (!a || a === document.body || (el.contains(a) && a.closest(".d95-tb")))) { try { current.focus({ preventScroll: true }); } catch (e) {} }
      }, function () { if (gen === generation) { scanned = false; say(T("sounds.st_error"), T("sounds.failed")); } });
    }
    // the window closed: its games are read again at its next opening
    function refresh() {
      generation++;
      scanned = false; boards = []; shown = null;
      grid.textContent = ""; buttons = []; current = null;
      say("");
    }
    return { el: el, scan: scan, release: release, refresh: refresh,
      // for tests: the buttons, what has played and what failed
      buttons: function () { return buttons.slice(); }, stats: function () { return stats; }, playing: function () { return live.length; } };
  }

  // The window on the listing's desktop (web/index.html): closed at first, its icon after Music's
  var board = null;
  function attach(desk, at) {
    if (board || !document.getElementById("games-title") || typeof ByopopMusic === "undefined") return null;
    var games = Array.prototype.map.call(document.querySelectorAll("a.tile[data-game]"), function (t) {
      return { id: t.getAttribute("data-game"), title: t.querySelector(".name").textContent, persist: t.getAttribute("data-persist") };
    });
    board = build(games);
    var foot = document.querySelector("body > footer");
    if (foot && foot.parentNode) foot.parentNode.insertBefore(board.el, foot); else document.body.appendChild(board.el);
    // its icon: a speaker sending out its sound, our own drawing in 32 x 32
    var pic = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    pic.setAttribute("viewBox", "0 0 32 32"); pic.setAttribute("aria-hidden", "true"); pic.setAttribute("shape-rendering", "crispEdges");
    pic.innerHTML = '<path d="M3 11h7l8-7v24l-8-7H3z" fill="#c0c0c0" stroke="#000"/><path d="M4 12h5v8H4z" fill="#808080"/>' +
      '<path d="M11 12l6-5v18l-6-5z" fill="#ffffff"/>' +
      '<path d="M21 12c2 2 2 6 0 8M24 9c4 4 4 10 0 14M27 6c5 6 5 14 0 20" fill="none" stroke="#000080" stroke-width="2" shape-rendering="auto"/>';
    desk.add(board.el, { id: "sounds", title: T("sounds.title"), closable: true, closed: true, pic: pic, at: at, scroll: ".sb-grid" });
    board.el.classList.add("sb-win");
    // read the games when it opens; stop, and drop the decoded sounds, when it closes (a minimized window keeps them); the
    // desktop going (another skin, a narrow screen) closes it too
    var open = false;
    function watch() {
      var now = !board.el.classList.contains("d95-closed") && document.documentElement.classList.contains("d95-desk");
      if (now === open) return;
      open = now;
      if (open) board.scan(); else { board.release(); board.refresh(); }
    }
    if (window.MutationObserver) {
      new MutationObserver(watch).observe(board.el, { attributes: true, attributeFilter: ["class"] });
      new MutationObserver(watch).observe(document.documentElement, { attributes: true, attributeFilter: ["class"] });
    }
    watch();
    return board;
  }
  return { attach: attach, panel: function () { return board; } };
})();
