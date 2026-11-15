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
    // an MP3 in a WAV (Word Harmony's instruments): its frames, as an MP3, since Firefox reads no WAV but PCM
    var mp3 = /\.wav$/i.test(rel) ? ByopopMusic.wavMp3(bytes) : null;
    if (mp3) return ctx.decodeAudioData(mp3.slice().buffer);
    return ctx.decodeAudioData(bytes.slice().buffer).catch(function (e) {
      var w = /\.wav$/i.test(rel) ? ByopopMusic.msAdpcm(bytes) : null;   // an MS ADPCM WAV, which the browsers do not read
      if (w) {
        var wb = ctx.createBuffer(w.channels.length, Math.max(1, w.channels[0].length), w.rate);
        w.channels.forEach(function (d, c) { wb.getChannelData(c).set(d); });
        return wb;
      }
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
  var EXTS = [".ogg", ".wav", ".au", ".mp3"];
  // The table's entries whose files this game's folder holds (matched without regard to case), each group with those it holds
  function boardOf(game, entry) {
    var F = ByopopMusic.files;
    return F.open(game.persist).then(function (db) {
      if (!db) return null;
      return F.list(db, game.persist).then(function (rels) {
        db.close();
        var have = {};
        rels.forEach(function (r) { have[r.toLowerCase()] = r; });
        var groups = [], count = 0, presets = [];
        // a sound inside one of the game's files ("Resource.dll|pe:WAVE:<id>", "ningpo.pyz|zip:<name>",
        // "TipTop.dat|tiptop:<name>"; ByopopMusic.files.read reads it out): there when that file is
        function inside(path) {
          var bar = path.indexOf("|");
          if (bar < 0) return null;
          var file = have[path.slice(0, bar).toLowerCase()];
          return file ? file + path.slice(bar) : null;
        }
        (entry.groups || []).forEach(function (g) {
          var sounds = [];
          g.sounds.forEach(function (s) {
            var rel = String(s[0]).indexOf("|") >= 0 ? inside(String(s[0])) : have[String(s[0]).toLowerCase()];
            if (rel) sounds.push({ rel: rel, name: s[1], when: s[2] || "" });
          });
          if (sounds.length) { groups.push({ name: g.name, sounds: sounds }); count += sounds.length; }
        });
        // a preset's steps name their files with or without the extension (as a port's sound log names them): the first of
        // the game's own extensions it has; a preset with a file missing is left out
        function find(path) {
          if (String(path).indexOf("|") >= 0) return inside(String(path));
          var p = String(path).toLowerCase();
          if (have[p]) return have[p];
          for (var i = 0; i < EXTS.length; i++) if (have[p + EXTS[i]]) return have[p + EXTS[i]];
          return null;
        }
        (entry.presets || []).forEach(function (pr) {
          var steps = [], whole = true;
          pr.steps.forEach(function (s) {
            var rel = find(s[0]), o = s[2] || {};
            if (!rel) { whole = false; return; }
            steps.push({ rel: rel, at: s[1], v: o.v || 0, p: o.p || 0, r: o.r || 1, loop: !!o.loop, end: o.end, ramp: o.ramp, pans: o.pans });
          });
          if (whole && steps.length) presets.push({ name: pr.name, when: pr.when || "", steps: steps });
        });
        return count ? { game: game, groups: groups, count: count, presets: presets } : null;
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
    // the search across every game's sounds (their labels, the words of a voice clip among them, and when they play), and the
    // filters to voice clips and to sounds the game never plays (the user, 2026-11-12: "Add a feature to search through all
    // games' sound effects, and add a filter to only show voice clips"; "add a filter to the sfx player to only show unused
    // sounds")
    var search = make("input", "sb-search");
    search.type = "search"; search.id = "sounds-search"; search.autocomplete = "off"; search.spellcheck = false;
    search.setAttribute("aria-label", T("sounds.search")); search.title = T("sounds.search"); search.placeholder = T("sounds.search");
    function check(id, label) {
      var lab = make("label", "sb-check"), box = make("input");
      box.type = "checkbox"; box.id = id;
      lab.appendChild(box); lab.appendChild(document.createTextNode(label));
      box.addEventListener("change", function () { render(shown); });
      return { lab: lab, box: box };
    }
    var voiceOnly = check("sounds-voice", T("sounds.voice")), unusedOnly = check("sounds-unused", T("sounds.unused"));
    search.addEventListener("input", function () { render(shown); });
    [pick, search, voiceOnly.lab, unusedOnly.lab, stopBtn, make("span", "sb-gap"), muteBtn, vol].forEach(function (e) { bar.appendChild(e); });
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
    // A preset: every step's sound at its time from the press, on the audio clock, as the game's sound log had it: its volume
    // and pan in hundredths of a decibel (DirectSound's units, the pan the other side's cut), its pitch as a rate, a loop
    // stopped at its end, and the changes of volume and pan while it plays. Pressed again while it plays: it starts again.
    function cB(v) { return Math.pow(10, v / 2000); }
    function side(p) { return p > 0 ? 1 - cB(-p) : p < 0 ? cB(p) - 1 : 0; }
    function playPreset(board, pr, btn) {
      if (!audio()) return;
      if (ctx.state === "suspended") { try { ctx.resume(); } catch (e) {} }
      var at = ctx;
      var bad = null;   // the first step whose file could not be played, named in the button's tooltip
      Promise.all(pr.steps.map(function (s) {
        return bufferOf(board, s.rel).catch(function (e) { if (!bad) bad = s.rel; stats.failed[s.rel] = true; throw e; });
      })).then(function (bufs) {
        if (ctx !== at) return;
        (btn.sbSources || []).forEach(function (s) { try { s.stop(); } catch (e) {} });
        var t0 = ctx.currentTime + 0.05, mine = [];
        pr.steps.forEach(function (s, i) {
          var src = ctx.createBufferSource(), g = ctx.createGain(), node = g, t = t0 + s.at / 1000;
          src.buffer = bufs[i];
          src.loop = s.loop;
          src.playbackRate.value = s.r;
          g.gain.setValueAtTime(cB(s.v), t);
          (s.ramp || []).forEach(function (pt) { g.gain.linearRampToValueAtTime(cB(pt[1]), t0 + pt[0] / 1000); });
          src.connect(g);
          if ((s.p || s.pans) && ctx.createStereoPanner) {
            var pan = ctx.createStereoPanner();
            pan.pan.setValueAtTime(side(s.p), t);
            (s.pans || []).forEach(function (pt) { pan.pan.setValueAtTime(side(pt[1]), t0 + pt[0] / 1000); });
            g.connect(pan);
            node = pan;
          }
          node.connect(gain);
          src.onended = function () {
            var k = live.indexOf(src);
            if (k >= 0) live.splice(k, 1);
          };
          src.start(t);
          if (s.end != null) src.stop(t0 + s.end / 1000);
          live.push(src);
          mine.push(src);
        });
        btn.sbSources = mine;
        stats.started++;
        stats.presets = (stats.presets || 0) + 1;
      }, function () {
        btn.title = pr.name + "\n" + (bad ? T("sounds.bad_step", { file: bad.split("|").pop().split("/").pop() }) : T("sounds.bad_file"));
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
    // a voice clip's label is its words in quotes; a sound the game never plays says so at the end of its label
    function isVoice(s) { return /^["“]/.test(s.name); }
    function isUnused(s) { return /\(unused\)$/i.test(s.name); }
    function render(board) {
      shown = board;
      grid.textContent = "";
      buttons = []; current = null;
      if (!board) return;
      var words = search.value.toLowerCase().split(/\s+/).filter(function (w) { return w; });
      var voice = voiceOnly.box.checked, unused = unusedOnly.box.checked;
      if (words.length || voice || unused) {
        // a search looks through every game, the filters alone through the game chosen; the matches in their groups, each
        // group named with its game when a search found it
        var n = 0;
        var keep = function (s) {
          var text = (s.name + " " + s.when + " " + s.rel).toLowerCase();
          return (!voice || isVoice(s)) && (!unused || isUnused(s)) && words.every(function (w) { return text.indexOf(w) >= 0; });
        };
        (words.length ? boards : [board]).forEach(function (bd) {
          bd.groups.forEach(function (g) {
            var items = g.sounds.filter(keep);
            if (!items.length) return;
            n += items.length;
            grid.appendChild(groupBox(words.length ? bd.game.title + ": " + g.name : g.name, items, function (s, b) {
              b.setAttribute("data-file", s.rel);
              b.addEventListener("click", function () { play(bd, s, b); });
            }));
          });
        });
        setCurrent(buttons[0] || null);
        say(T("sounds.count", { n: n }));
        snapSoon();
        return;
      }
      // the presets first, in a group box of their own: sequences the game plays, as it plays them
      if (board.presets.length) grid.appendChild(groupBox(T("sounds.presets"), board.presets, function (pr, b) {
        b.setAttribute("data-preset", pr.name);
        b.addEventListener("click", function () { playPreset(board, pr, b); });
      }));
      board.groups.forEach(function (g) {
        grid.appendChild(groupBox(g.name, g.sounds, function (s, b) {
          b.setAttribute("data-file", s.rel);
          b.addEventListener("click", function () { play(board, s, b); });
        }));
      });
      setCurrent(buttons[0] || null);
      say(T("sounds.count", { n: board.count }));
      snapSoon();
    }
    // Each label centred on a whole pixel: a label's own width is a fraction of a pixel, so centring it in the button put
    // some at x.5, which Firefox and Chromium draw blurred beside crisp ones (the user, 2026-11-12: "Tile locked text looks
    // in focus, Tiles plagued text does not"). Each label gets a whole width of the same parity as the button's inside,
    // centred by its margins; one wider than the inside fills it and ends in an ellipsis. Done once a label is laid out
    // (not in a hidden window: a resize of the grid tries again).
    function snapNames() {
      buttons.forEach(function (b) {
        var name = b.firstChild;
        if (!name || name.style.width) return;
        var range = document.createRange();
        range.selectNodeContents(name);
        var text = range.getBoundingClientRect().width, cs = getComputedStyle(b);
        var inside = b.clientWidth - Math.round(parseFloat(cs.paddingLeft) + parseFloat(cs.paddingRight));
        if (!text || inside <= 0) return;
        var w = Math.ceil(text);
        if ((inside - w) % 2) w++;
        name.style.width = Math.min(w, inside) + "px";
      });
    }
    function snapSoon() { requestAnimationFrame(snapNames); }
    if (typeof ResizeObserver === "function") new ResizeObserver(snapSoon).observe(grid);
    // a group box: a fieldset named as its legend, so that screen readers give each button its group; each button named by
    // its label, the tooltip adding when the game plays it (for screen readers, its description)
    function groupBox(name, items, wire) {
      var box = make("fieldset", "sb-group"), legend = make("legend", "sb-legend", name), row = make("div", "sb-row");
      box.setAttribute("role", "group");
      box.setAttribute("aria-label", name);
      box.appendChild(legend);
      items.forEach(function (s) {
        var b = make("button", "sb-btn");
        b.type = "button";
        b.tabIndex = -1;
        b.appendChild(make("span", "sb-name", s.name));
        b.setAttribute("aria-label", s.name);
        // its label, when the game plays it, and where its file lies in the game's folder
        b.title = [s.name, s.when, s.rel ? ByopopMusic.filePlace(s.rel) : ""].filter(function (x) { return x; }).join("\n");
        wire(s, b);
        b.addEventListener("focus", function () { setCurrent(b); });
        row.appendChild(b);
        buttons.push(b);
      });
      box.appendChild(row);
      return box;
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
    }).sort(function (a, b) { return a.title.localeCompare(b.title); });   // by title, as the listing and the Start menu list them
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
    // desktop going (a screen under 760 pixels wide) closes it too
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
  return { attach: attach, panel: function () { return board; }, decode: decode };
})();
