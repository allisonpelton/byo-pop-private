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
  // the desktop's zoom (web/skins.js): a pointer's movement is the screen's CSS pixels, the window's sizes the page's
  function zoomOf() { try { return ByopopSkin.zoom(); } catch (e) { return 1; } }
  var DB_STORE = "FILE_DATA";
  var MIN_TUNE = 2;          // seconds: a shorter subsong is a silence or a sting between tunes, not a tune
  var CHUNK = 4096;          // frames a message to the worklet carries
  var AHEAD = 12;            // chunks kept queued in the worklet: about a second
  var VOLUME_KEY = "byopop-music-volume", LOOP_KEY = "byopop-music-loop", FX_KEY = "byopop-music-fx";
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
  // A stored file's bytes, or null. A key "<file>|pe:<type>:<id>" is that resource of that Windows module, "<file>|zip:<name>"
  // that entry of that zip archive, and "<file>|tiptop:<name>" that entry of Tip Top's archive: tunes and sounds a game keeps
  // inside its own files (the Sounds window reads them so too, web/sounds.js)
  function readFile(db, key) {
    var bar = key.indexOf("|");
    if (bar >= 0) {
      var pick = key.slice(bar + 1);
      return readFile(db, key.slice(0, bar)).then(function (bytes) {
        if (!bytes) return null;
        if (pick.indexOf("pe:") === 0) {
          var r = peResources(bytes).filter(function (x) { return "pe:" + x.type + ":" + x.id === pick; })[0];
          return r ? r.bytes : null;
        }
        if (pick.indexOf("zip:") === 0) return zipEntry(bytes, pick.slice(4));
        if (pick.indexOf("tiptop:") === 0) return tipTopEntry(bytes, pick.slice(7));
        return null;
      });
    }
    return new Promise(function (resolve, reject) {
      var r = db.transaction(DB_STORE, "readonly").objectStore(DB_STORE).get(key);
      r.onerror = function () { reject(r.error); };
      r.onsuccess = function () {
        var c = r.result && r.result.contents;
        resolve(c ? (c instanceof Uint8Array ? c : new Uint8Array(c)) : null);
      };
    });
  }
  // A Windows module's resources, in the order its resource directory lists them: [{ type, id, bytes }] (type and id the
  // numbers, or the names, of its first two levels); [] where it has none or is not one
  function peResources(bytes) {
    var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength), out = [];
    try {
      if (dv.getUint16(0, true) !== 0x5a4d) return out;
      var pe = dv.getUint32(0x3c, true);
      if (dv.getUint32(pe, true) !== 0x4550) return out;
      var sections = dv.getUint16(pe + 6, true), opt = pe + 24, optSize = dv.getUint16(pe + 20, true);
      var dirs = opt + (dv.getUint16(opt, true) === 0x20b ? 112 : 96), rsrc = dv.getUint32(dirs + 16, true);
      if (!rsrc) return out;
      var table = [];
      for (var s = 0; s < sections; s++) {
        var h = opt + optSize + s * 40;
        table.push({ va: dv.getUint32(h + 12, true), size: Math.max(dv.getUint32(h + 8, true), dv.getUint32(h + 16, true)), raw: dv.getUint32(h + 20, true) });
      }
      var at = function (rva) {
        for (var i = 0; i < table.length; i++) if (rva >= table[i].va && rva < table[i].va + table[i].size) return rva - table[i].va + table[i].raw;
        return -1;
      };
      var base = at(rsrc);
      if (base < 0) return out;
      var nameOf = function (v) {
        if (!(v & 0x80000000)) return v;
        var o = base + (v & 0x7fffffff), n = dv.getUint16(o, true), s2 = "";
        for (var i = 0; i < n; i++) s2 += String.fromCharCode(dv.getUint16(o + 2 + i * 2, true));
        return s2;
      };
      var walk = function (dir, path) {
        var n = dv.getUint16(base + dir + 12, true) + dv.getUint16(base + dir + 14, true);
        for (var i = 0; i < n; i++) {
          var e = base + dir + 16 + i * 8, name = nameOf(dv.getUint32(e, true)), to = dv.getUint32(e + 4, true);
          if (to & 0x80000000) { if (path.length < 2) walk(to & 0x7fffffff, path.concat([name])); continue; }
          var off = at(dv.getUint32(base + to, true)), size = dv.getUint32(base + to + 4, true);
          if (off >= 0 && off + size <= bytes.length) out.push({ type: path[0], id: path[1], bytes: bytes.subarray(off, off + size) });
        }
      };
      walk(0, []);
    } catch (e) {}
    return out;
  }
  // An entry of Tip Top's archive (TipTop.dat; games/tiptop/tools/tiptopdat.py) by name, ignoring case, "\" or "/" between
  // folders, or null. Every byte of the file is stored with its bits permuted, the same way: bit i of a stored byte holds bit
  // SRC[i] of the real one. Decoded, it is a u32 table size, the table (a NUL-terminated name, u32 offset from the file's
  // start and u32 size, until an empty name) and the files
  var TIPTOP_SRC = [2, 6, 0, 7, 5, 4, 1, 3], tipTopDecode = null;
  function tipTopEntry(bytes, name) {
    if (!tipTopDecode) {
      tipTopDecode = new Uint8Array(256);
      for (var c = 0; c < 256; c++) {
        var enc = 0;
        for (var b = 0; b < 8; b++) enc |= ((c >> TIPTOP_SRC[b]) & 1) << b;
        tipTopDecode[enc] = c;
      }
    }
    var d = tipTopDecode, want = name.replace(/\//g, "\\").toLowerCase();
    function u32(o) { return (d[bytes[o]] | (d[bytes[o + 1]] << 8) | (d[bytes[o + 2]] << 16) | (d[bytes[o + 3]] << 24)) >>> 0; }
    if (bytes.length < 4) return null;
    var tableEnd = 4 + u32(0), p = 4;
    while (p < tableEnd && p < bytes.length) {
      var e = p, n = "";
      while (e < bytes.length && d[bytes[e]] !== 0) n += String.fromCharCode(d[bytes[e++]]);
      if (!n) break;
      var off = u32(e + 1), size = u32(e + 5);
      if (n.toLowerCase() === want) {
        if (off + size > bytes.length) return null;
        var out = new Uint8Array(size);
        for (var k = 0; k < size; k++) out[k] = d[bytes[off + k]];
        return out;
      }
      p = e + 9;
    }
    return null;
  }
  // A zip archive's entry by name (stored or deflated), or null
  function zipEntry(bytes, name) {
    var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength), end = -1;
    for (var i = bytes.length - 22; i >= Math.max(0, bytes.length - 65557); i--) if (dv.getUint32(i, true) === 0x06054b50) { end = i; break; }
    if (end < 0) return Promise.resolve(null);
    var n = dv.getUint16(end + 10, true), c = dv.getUint32(end + 16, true), low = name.toLowerCase();
    for (var k = 0; k < n && c + 46 <= bytes.length; k++) {
      var method = dv.getUint16(c + 10, true), size = dv.getUint32(c + 20, true), nl = dv.getUint16(c + 28, true);
      var extra = dv.getUint16(c + 30, true), note = dv.getUint16(c + 32, true), local = dv.getUint32(c + 42, true);
      var entry = new TextDecoder("latin1").decode(bytes.subarray(c + 46, c + 46 + nl));
      if (entry.toLowerCase() === low) {
        var from = local + 30 + dv.getUint16(local + 26, true) + dv.getUint16(local + 28, true), data = bytes.subarray(from, from + size);
        if (method === 0) return Promise.resolve(data);
        if (method !== 8 || typeof DecompressionStream === "undefined") return Promise.resolve(null);
        return new Response(new Blob([data]).stream().pipeThrough(new DecompressionStream("deflate-raw"))).arrayBuffer()
          .then(function (b) { return new Uint8Array(b); }, function () { return null; });
      }
      c += 46 + nl + extra + note;
    }
    return Promise.resolve(null);
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
  // a recorded tune (Ogg Vorbis, or a WAV file: Word Harmony's), decoded whole by the browser rather than rendered
  function isStream(rel) { return /\.(ogg|wav)$/i.test(rel); }
  // A WAV file's length in seconds, from its data chunk's size and its format's bytes a second
  function wavSeconds(bytes) {
    var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength), rate = 0;
    for (var at = 12; at + 8 <= bytes.length;) {
      var id = String.fromCharCode(bytes[at], bytes[at + 1], bytes[at + 2], bytes[at + 3]), size = dv.getUint32(at + 4, true);
      if (id === "fmt " && size >= 16) rate = dv.getUint32(at + 16, true);
      if (id === "data") return rate ? Math.min(size, bytes.length - at - 8) / rate : 0;
      at += 8 + size + (size & 1);
    }
    return 0;
  }
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
  // An Ogg Vorbis or WAV file as interleaved stereo at the context's rate: the browser's decoder, else an MS ADPCM WAV's (msAdpcm),
  // else stb_vorbis in openmpt.wasm
  function decodeOgg(M, ctx, bytes) {
    var mp3 = wavMp3(bytes);   // an MP3 in a WAV: its frames, which every browser reads as an MP3
    return ctx.decodeAudioData((mp3 || bytes).slice().buffer).then(function (buf) {
      var n = buf.length, l = buf.getChannelData(0), r = buf.numberOfChannels > 1 ? buf.getChannelData(1) : l, out = new Float32Array(n * 2);
      for (var i = 0; i < n; i++) { out[i * 2] = l[i]; out[i * 2 + 1] = r[i]; }
      return out;
    }, function () {
      var w = msAdpcm(bytes);
      if (!w) return stbOgg(M, bytes, ctx.sampleRate);
      var n = w.channels[0].length, rate = ctx.sampleRate, frames = Math.floor(n * rate / w.rate), out = new Float32Array(frames * 2);
      for (var i = 0; i < frames; i++) {
        var x = i * w.rate / rate, a = Math.floor(x), f = x - a, b = Math.min(a + 1, n - 1);
        for (var c = 0; c < 2; c++) { var d = w.channels[Math.min(c, w.channels.length - 1)]; out[i * 2 + c] = d[a] * (1 - f) + d[b] * f; }
      }
      return out;
    });
  }
  // An MS ADPCM WAV (format 2, which the browsers' decoders do not read: Word Harmony's music and some of its sounds) as
  // {rate, channels: [Float32Array]}; null for any other file. Each block starts with each channel's predictor, step and two
  // samples, then four-bit codes (a stereo byte's high half the left's), each predicted from the two before, the division
  // rounding toward zero as Windows' own decoder's does
  // An MP3 inside a WAV (format 0x55, as Word Harmony's instruments and menu music are): the data chunk's MPEG frames, which
  // every browser's decoder reads as an MP3, where Firefox's reads no WAV but PCM; null for any other file
  function wavMp3(bytes) {
    var w = wavChunks(bytes);
    return w && w.tag === 0x55 ? bytes.subarray(w.data, w.data + w.size) : null;
  }
  // A RIFF WAVE's format tag and where its data chunk lies ({tag, data, size}), or null
  function wavChunks(bytes) {
    try {
      var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      if (bytes.length < 12 || dv.getUint32(0) !== 0x52494646 || dv.getUint32(8) !== 0x57415645) return null;
      var at = 12, tag = -1, data = -1, size = 0;
      while (at + 8 <= bytes.length) {
        var id = dv.getUint32(at), len = dv.getUint32(at + 4, true);
        if (id === 0x666d7420) tag = dv.getUint16(at + 8, true);
        else if (id === 0x64617461) { data = at + 8; size = Math.min(len, bytes.length - data); }
        at += 8 + len + (len & 1);
      }
      return tag >= 0 && data >= 0 ? { tag: tag, data: data, size: size } : null;
    } catch (e) { return null; }
  }
  var ADAPT = [230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230];
  function msAdpcm(bytes) {
    try {
      var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      if (bytes.length < 12 || dv.getUint32(0) !== 0x52494646 || dv.getUint32(8) !== 0x57415645) return null;
      var at = 12, fmt = -1, data = -1, size = 0, total = -1;
      while (at + 8 <= bytes.length) {
        var id = dv.getUint32(at), len = dv.getUint32(at + 4, true);
        if (id === 0x666d7420) fmt = at + 8;
        else if (id === 0x64617461) { data = at + 8; size = Math.min(len, bytes.length - data); }
        else if (id === 0x66616374 && len >= 4) total = dv.getUint32(at + 8, true);
        at += 8 + len + (len & 1);
      }
      if (fmt < 0 || data < 0 || dv.getUint16(fmt, true) !== 2) return null;
      var ch = dv.getUint16(fmt + 2, true), rate = dv.getUint32(fmt + 4, true), align = dv.getUint16(fmt + 12, true);
      var count = dv.getUint16(fmt + 20, true), coef = [], c, k;
      if (ch < 1 || ch > 2 || align <= 7 * ch || !count || rate < 1000) return null;
      for (k = 0; k < count; k++) coef.push([dv.getInt16(fmt + 22 + k * 4, true), dv.getInt16(fmt + 24 + k * 4, true)]);
      var perBlock = 2 + (align - 7 * ch) * 2 / ch, frames = Math.ceil(size / align) * perBlock, out = [], n = 0;
      for (c = 0; c < ch; c++) out.push(new Float32Array(frames));
      for (var b = data; b + 7 * ch <= data + size; b += align) {
        var end = Math.min(b + align, data + size), pred = [], step = [], s1 = [], s2 = [];
        for (c = 0; c < ch; c++) {
          if (bytes[b + c] >= count) return null;
          pred.push(coef[bytes[b + c]]);
          step.push(dv.getInt16(b + ch + c * 2, true));
          s1.push(dv.getInt16(b + 3 * ch + c * 2, true));
          s2.push(dv.getInt16(b + 5 * ch + c * 2, true));
          out[c][n] = s2[c] / 32768;
          out[c][n + 1] = s1[c] / 32768;
        }
        n += 2;
        for (var q = b + 7 * ch; q < end; q++) {
          for (var h = 0; h < 2; h++) {
            var u = h ? bytes[q] & 15 : bytes[q] >> 4;
            c = ch === 2 ? h : 0;
            var x = ((s1[c] * pred[c][0] + s2[c] * pred[c][1]) / 256 | 0) + (u >= 8 ? u - 16 : u) * step[c];
            x = x < -32768 ? -32768 : x > 32767 ? 32767 : x;
            s2[c] = s1[c]; s1[c] = x;
            step[c] = (ADAPT[u] * step[c]) >> 8;
            if (step[c] < 16) step[c] = 16;
            out[c][n] = x / 32768;
            if (ch === 1 || h) n++;
          }
        }
      }
      if (total >= 0 && total < n) n = total;
      return { rate: rate, channels: out.map(function (d) { return d.subarray(0, n); }) };
    } catch (e) { return null; }
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
  // Where a file lies in the game's folder, as the copy has it, for a tooltip (the user, 2026-11-12: "The descriptions should
  // include the path to the audio file within the game folder"): its path, or the file it is inside and where in it
  // ("Resource.dll, WAVE resource 373", "TipTop.dat: sounds/Pop.ogg")
  function filePlace(rel) {
    var bar = rel.indexOf("|");
    if (bar < 0) return rel;
    var host = rel.slice(0, bar), pick = rel.slice(bar + 1), m = /^pe:([^:]+):(.+)$/.exec(pick);
    if (m) return T("music.in_resource", { file: host, type: m[1], id: m[2] });
    return T("music.in_archive", { file: host, entry: pick.replace(/^[a-z]+:/, "") });
  }
  // A file's name without its folder and extension
  function baseName(rel) {
    var bar = rel.indexOf("|");
    if (bar >= 0) {
      // a file inside a file: a zip entry's own name, or a resource's module and id ("resource 335")
      var pick = rel.slice(bar + 1);
      return pick.indexOf("zip:") === 0 ? baseName(pick.slice(4)) : baseName(rel.slice(0, bar)) + " " + pick.split(":").pop();
    }
    return rel.split("/").pop().replace(/\.[^.]+$/, "");
  }
  // the display's second line for a tune: its game, where the game plays it (or that it never does) and its format
  function subLine(t) {
    return [t.game.title, t.unused ? T("music.unused") : t.where, t.format].filter(function (s) { return s; }).join(" · ");
  }
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
  // A game's tunes: [{game, kind, keys, sub, range, name, seconds}] (kind: module, stream or joined). A module's tunes are its
  // subsongs, or the ranges of orders its parts list; an Ogg file is one tune; a joined tune is the pieces its playlist names, one
  // after the other. Tunes under MIN_TUNE seconds and those in skip are left out; names names a tune, else its file does.
  function tunesOf(M, game, entry) {
    return openExisting(game.persist).then(function (db) {
      if (!db) return [];
      var prefix = game.persist + "/game/", tunes = [];
      function read(rel) { return readFile(db, prefix + rel); }
      // a tune's format (its file's extension: MO3, IT, S3M, XM, OXM or OGG), where in the game it plays and whether the game
      // never plays it, from the table
      var formats = {};   // a resource's format, by its key, from its first bytes
      // a table field's value for a tune id, its case ignored (file names are stored as the game's copy has them)
      function byId(obj, id) {
        var low = id.toLowerCase(), hit;
        Object.keys(obj || {}).forEach(function (k) { if (k.toLowerCase() === low) hit = obj[k]; });
        return hit;
      }
      function placeOf(k, rel) {
        k.format = formats[rel] || (rel.match(/\.([^.\/]+)$/) || ["", ""])[1].toUpperCase();
        var low = k.id.toLowerCase();
        k.where = byId(entry.where, k.id) || "";
        // when exactly the game plays it, for the place's tooltip (the user, 2026-11-12: "Make hovering over the brief track
        // description explain when it plays")
        k.when = byId(entry.when, k.id) || "";
        // who wrote it, from the game's own credits: the tune's own where they split the music, else the game's
        k.composer = byId(entry.composers, k.id) || entry.composer || "";
        k.unused = (entry.unused || []).some(function (u) { return u.toLowerCase() === low; });
      }
      function add(rel, list) {
        var kept = list.filter(function (k) { return k.seconds >= MIN_TUNE && !(entry.skip || []).some(function (x) { return x.toLowerCase() === k.id.toLowerCase(); }); });
        kept.forEach(function (k, j) {
          var named = entry.names && entry.names[k.id];
          k.game = game;
          k.name = named || (kept.length > 1 ? T("music.tune_part", { file: baseName(rel), n: j + 1 }) : baseName(rel));
          placeOf(k, rel);
          tunes.push(k);
        });
      }
      return gameFiles(db, game.persist).then(function (rels) {
        var steps = matching(rels, entry.files).map(fileStep);
        // tunes inside a game's files: a Windows module's resources that start with the magic given (their format), and a zip
        // archive's entries
        (entry.inside || []).forEach(function (n) {
          var rel = findRel(rels, n.file);
          if (!rel) return;
          if (n.zip) { n.zip.forEach(function (name) { steps.push(fileStep(rel + "|zip:" + name)); }); return; }
          steps.push(function () {
            return read(rel).then(function (bytes) {
              if (!bytes) return;
              return peResources(bytes).filter(function (r) {
                return new TextDecoder("latin1").decode(r.bytes.subarray(0, n.magic.length)) === n.magic;
              }).reduce(function (p, r) {
                var key = rel + "|pe:" + r.type + ":" + r.id;
                formats[key] = n.magic.toUpperCase();
                return p.then(fileStep(key));
              }, Promise.resolve());
            });
          });
        });
        // A subsong cut at the orders the game starts it at partway (the table's "cuts"): its pieces, each a tune of its own
        // (segs: [{sub, from, until}], playing the subsong from order from, or its start, until it reaches order until, or its
        // end). A cut the subsong reaches as it plays ends the piece before it; one it never reaches (an order the game starts
        // that only a jump of the game's own leads to) is a piece played from there to where the music ends. Lengths are timed
        // by rendering the subsong unheard (libopenmpt at 8000 Hz, libxmp-lite at its own 48000, as a rate change restarts it).
        function cutTune(m, base, cuts, fast) {
          var reached = timeOrders(m, base.sub, null, cuts, fast);
          var inFlow = cuts.filter(function (c) { return c in reached.at; }).sort(function (a, b) { return reached.at[a] - reached.at[b]; });
          var pieces = [], starts = [null].concat(inFlow);
          starts.forEach(function (from, k) {
            var t0 = from === null ? 0 : reached.at[from], until = k + 1 < starts.length ? starts[k + 1] : null;
            var t1 = until === null ? reached.end : reached.at[until];
            pieces.push(piece(base, from, until, t1 - t0));
          });
          cuts.filter(function (c) { return !(c in reached.at); }).forEach(function (c) {
            pieces.push(piece(base, c, null, timeOrders(m, base.sub, c, [], fast).end));
          });
          return pieces;
        }
        function piece(base, from, until, seconds) {
          var k = {};
          Object.keys(base).forEach(function (f) { k[f] = base[f]; });
          if (from !== null) { k.id = base.id + "@" + from; k.entry = from; }
          k.segs = [{ sub: base.sub, from: from, until: until }];
          k.seconds = seconds;
          delete k.sub;
          return k;
        }
        // Renders subsong sub (from order from, or its start) once through, unheard: when each of orders is first reached, in
        // seconds, and where it ends ({at: {order: s}, end: s})
        function timeOrders(m, sub, from, orders, fast) {
          var rate = fast ? 8000 : 48000, buf = M._malloc(1024 * 8), at = {}, frames = 0, guard = 0;
          M._bm_select(m, sub);
          M._bm_loop(m, 0);
          if (from !== null) M._bm_set_order(m, from);
          try {
            for (;;) {
              var o = M._bm_order(m);
              if (orders.indexOf(o) >= 0 && !(o in at) && frames > 0) at[o] = frames / rate;
              var g = M._bm_render(m, rate, 1024, buf);
              frames += g;
              if (g < 1024 || ++guard > rate * 3600 / 1024) break;
            }
          } finally { M._free(buf); }
          return { at: at, end: frames / rate };
        }
        // Subsongs the game only plays one after another (the table's "chains": {"<file>#<n>": ["<file>#<m>", ...]}): one tune
        // of them all, in that order, the others no longer tunes of their own
        function chainTunes(list) {
          var same = function (a, b) { return a.toLowerCase() === b.toLowerCase(); };
          Object.keys(entry.chains || {}).forEach(function (head) {
            var first = list.filter(function (k) { return same(k.id, head); })[0];
            if (!first) return;
            var rest = entry.chains[head].map(function (id) { return list.filter(function (k) { return same(k.id, id); })[0]; });
            if (rest.some(function (k) { return !k; })) return;
            first.segs = [first].concat(rest).map(function (k) { return { sub: k.sub, from: null, until: null }; });
            first.seconds = [first].concat(rest).reduce(function (n, k) { return n + k.seconds; }, 0);
            delete first.sub;
            rest.forEach(function (k) { list.splice(list.indexOf(k), 1); });
          });
        }
        function fileStep(rel) {
          return function () {
            return read(rel).then(function (bytes) {
              if (!bytes) return;
              var id = baseName(rel);
              if (isStream(rel)) { add(rel, [{ kind: "stream", keys: [prefix + rel], id: id, seconds: isOgg(rel) ? oggSeconds(bytes) : wavSeconds(bytes) }]); return; }
              var m = loadModule(M, bytes), list = [];
              if (!m) return;
              // the pattern each order plays (a marker below 0), which a game's song is known by (Music's "Following a game")
              var seq = [];
              for (var o = 0, no = M._bm_orders(m); o < no; o++) seq.push(M._bm_order_pattern(m, o));
              var ranges = entry.parts && entry.parts[id];
              if (ranges) {
                ranges.forEach(function (r, i) {
                  var t0 = M._bm_set_order(m, r[0]), t1 = r[1] + 1 < M._bm_orders(m) ? M._bm_set_order(m, r[1] + 1) : M._bm_duration(m);
                  list.push({ kind: "module", keys: [prefix + rel], range: r, start: t0, entry: r[0], seq: seq, id: id + "#" + i, seconds: t1 - t0, level: entry.level });
                });
              } else {
                var one = M._malloc(16), fast = /\.(mo3|oxm)$/i.test(rel) || formats[rel] === "MO3", subEntries = [];
                for (var i = 0, n = M._bm_subsongs(m); i < n; i++) {
                  M._bm_select(m, i);
                  var secs = M._bm_duration(m);
                  M._bm_render(m, 48000, 1, one);   // libxmp-lite moves to the subsong's first order as it plays
                  var base = { kind: "module", keys: [prefix + rel], sub: i, entry: M._bm_order(m), seq: seq, id: id + "#" + i, seconds: secs, level: entry.level };
                  subEntries[i] = base.entry;
                  var cuts = byId(entry.cuts, base.id), starts = byId(entry.starts, base.id);
                  if (cuts) cutTune(m, base, cuts, fast).forEach(function (k) { list.push(k); });
                  else list.push(base);
                  // (the table's "starts": orders the game also starts the subsong at, its first tune playing on through
                  // them: each a piece from there to where the music ends, the subsong left whole)
                  (starts || []).forEach(function (c) { list.push(piece(base, c, null, timeOrders(m, base.sub, c, [], fast).end)); });
                }
                M._free(one);
                // every subsong's first order, for the orders each tune plays (tuneOrders)
                list.forEach(function (k) { k.subEntries = subEntries; });
                chainTunes(list);
              }
              M._bm_free(m);
              add(rel, list);
            });
          };
        }
        (entry.joined || []).forEach(function (j) {
          steps.push(function () {
            var listRel = j.playlist ? findRel(rels, j.playlist) : null;
            if (j.playlist && !listRel) return;
            return (listRel ? read(listRel) : Promise.resolve(null)).then(function (bytes) {
              if (listRel && !bytes) return;
              var pieces = listRel ? playlistPieces(new TextDecoder("latin1").decode(bytes)).map(function (w) { return findRel(rels, j.piece.replace("{}", w)); })
                : j.pieces.map(function (r) { return findRel(rels, r); });
              if (!pieces.length || pieces.some(function (p) { return !p; })) return;
              var seconds = 0, seen = {};
              return pieces.reduce(function (p, rel) {
                return p.then(function () {
                  if (seen[rel] !== undefined) { seconds += seen[rel]; return; }
                  return read(rel).then(function (b) { seen[rel] = !b ? 0 : isOgg(rel) ? oggSeconds(b) : wavSeconds(b); seconds += seen[rel]; });
                });
              }, Promise.resolve()).then(function () {
                var id = j.id || baseName(listRel).replace(/ playlist$/i, ""), k = { game: game, kind: "joined", id: id,
                  keys: pieces.map(function (r) { return prefix + r; }), seconds: seconds, name: (entry.names && entry.names[id]) || id,
                  loopFrom: j.loopFrom || 0 };
                placeOf(k, pieces[0]);
                tunes.push(k);
              });
            });
          });
        });
        return steps.reduce(function (p, step) { return p.then(step); }, Promise.resolve());
      }).then(function () { db.close(); return numbered(tunes); }, function (e) { db.close(); throw e; });
    });
  }

  // Tunes of one place numbered as the game reaches them ("Level Theme 1" to "Level Theme 4") put in that order, in the places
  // their files' order gave them in the list, the other tunes left where they are (the user, 2026-11-12, of Water Bugs' themes)
  function numbered(list) {
    var groups = {};
    list.forEach(function (k, i) {
      var m = /^(.*\S)\s+(\d+)$/.exec(k.where || "");
      if (m) (groups[m[1]] = groups[m[1]] || []).push(i);
    });
    Object.keys(groups).forEach(function (g) {
      var at = groups[g], n = function (k) { return parseInt(/\d+$/.exec(k.where)[0], 10); };
      var sorted = at.map(function (i) { return list[i]; }).sort(function (a, b) { return n(a) - n(b); });
      at.forEach(function (i, j) { list[i] = sorted[j]; });
    });
    return list;
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
  var FOLD_KEY = "byopop-music-folded";   // the games folded in the list, their ids joined by spaces
  var VIS = ["spectrum", "tracker", "piano", "score", "off"];  // what the display's lower half shows; a click on it goes to the next
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
    var scanned = false, loop = false, volume = 80, muted = false, vis = "spectrum", trackerOn = false, pianoOn = false, scoreOn = false, spectrumOn = false;
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
    // over the rows, each column's channel number (XM, MOD and S3M have no channel names, and libxmp-lite keeps none from an IT)
    var trackerHead = make("pre", "mu-tracker-head");
    // a scroll bar under the rows when the channels do not all fit, drawn here (arrows, a track and its thumb; the browser's own
    // bar draws no arrows): it moves the view by whole channels and follows nothing on its own. A click on it is the
    // bar's, not the display's (which would go to the next view); the keyboard's way is Left and Right on the display.
    var trackerScroll = make("div", "mu-hbar"), hbLeft = make("span", "mu-hb-left"), hbTrack = make("span", "mu-hb-track"),
        hbThumb = make("span", "mu-hb-thumb"), hbRight = make("span", "mu-hb-right");
    trackerScroll.setAttribute("aria-hidden", "true");
    trackerScroll.hidden = true;
    hbTrack.appendChild(hbThumb);
    [hbLeft, hbTrack, hbRight].forEach(function (e) { trackerScroll.appendChild(e); });
    tracker.appendChild(trackerAt); tracker.appendChild(trackerHead); tracker.appendChild(trackerRows); tracker.appendChild(trackerScroll);
    var piano = make("canvas", "mu-piano");
    piano.setAttribute("aria-hidden", "true");
    piano.hidden = true;
    // the score (drawScore below): the patterns as staves sliding past the playhead (scoreHead), which scroll up and down, the
    // clefs fixed at the left, and the metre as a value (scoreMeter)
    var score = make("div", "mu-score"), scoreSheet = make("div", "mu-score-sheet"), scoreHead = make("div", "mu-score-head"), scoreMeter = make("span", "mu-meter");
    score.setAttribute("aria-hidden", "true");
    score.hidden = true;
    scoreSheet.appendChild(scoreHead);
    scoreSheet.appendChild(scoreMeter);
    score.appendChild(scoreSheet);
    visEl.appendChild(spectrum); visEl.appendChild(tracker); visEl.appendChild(piano); visEl.appendChild(score);
    // the game the views follow (below, "Following a game"), over the view's top right corner
    var followEl = make("span", "mu-follow", "");
    followEl.hidden = true;
    followEl.setAttribute("aria-hidden", "true");
    visEl.appendChild(followEl);
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
    // the tracker's effect column on or off (fxText; the user, 2026-11-11: "add a brief English description of tracker commands
    // ... as an optional column"), off by default, shown with the tracker view only
    var fxOn = false;
    try { fxOn = localStorage.getItem(FX_KEY) === "1"; } catch (e) {}
    var fxBtn = make("button", "mu-btn mu-fx", T("music.fx_button"));
    fxBtn.type = "button";
    fxBtn.title = T("music.fx_title");
    fxBtn.setAttribute("aria-pressed", String(fxOn));
    fxBtn.hidden = true;
    row.appendChild(fxBtn);
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
    // grow: the player asked for the rows (the splitter or its keys): a window too short for them and LIST_ROWS of the list grows
    // downward by the difference, as far as the desktop's bottom edge (the user, 2026-11-09: "Doesn't seem like I'm able to
    // expand the tracker view to add more rows"); fewer rows give the height back to the list and never shrink the window
    function setRows(n, keep, grow) {
      n = Math.max(ROWS_MIN, Math.min(ROWS_MAX, Math.round(n)));
      if (inWindow()) {
        root.style.setProperty("--mu-rows", String(n));
        var win = root.closest(".d95-win");
        // (the list has a floor of its own, so the display can overflow it: the window grows a step at a time, at least a row,
        // until the list has its rows or the window meets the desktop's bottom edge)
        for (var guard = 0; grow && win && guard < 200 && listBox().clientHeight < listLeast(); guard++) {
          var by = 0;
          try { by = ByopopSkin.desk.grow(win, Math.max(ROW_PX, listLeast() - listBox().clientHeight)); } catch (e) {}
          if (!by) break;
        }
        // as many as leave the list its rows in the window as it is
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
      setRows(splitDrag.rows + Math.round((ev.clientY - splitDrag.y) / zoomOf() / ROW_PX), false, true);
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
      setRows(to, true, true);
    });

    var list = make("div", "mu-list");
    // Up and Down move from tune to tune in the list, as in a 1995 list box (the user, 2026-11-12: "Using arrow keys in music
    // player won't navigate through the songs; instead, it scrolls"), Home and End to the first and the last; Enter or Space
    // plays the one that has the focus, as a click does. With the focus elsewhere in the list they start at the tune playing.
    list.addEventListener("keydown", function (ev) {
      if (ev.altKey || ev.ctrlKey || ev.metaKey || ["ArrowDown", "ArrowUp", "Home", "End", "ArrowLeft", "ArrowRight"].indexOf(ev.key) < 0) return;
      if (ev.key === "ArrowLeft" || ev.key === "ArrowRight") {
        if (!document.activeElement || !document.activeElement.classList.contains("mu-fold")) return;
      }
      if ((ev.key === "ArrowLeft" || ev.key === "ArrowRight") && document.activeElement && document.activeElement.classList.contains("mu-fold")) {
        ev.preventDefault();
        fold(document.activeElement, ev.key === "ArrowLeft");
        return;
      }
      var all = Array.prototype.filter.call(list.querySelectorAll(".mu-fold, .mu-tune"), function (b) { return b.getClientRects().length > 0; });
      if (!all.length) return;
      var at = all.indexOf(document.activeElement);
      if (at < 0) at = Math.max(0, all.indexOf(list.querySelector('.mu-tune[data-tune="' + current + '"]')));
      var to = ev.key === "Home" ? 0 : ev.key === "End" ? all.length - 1 : at + (ev.key === "ArrowDown" ? 1 : -1);
      ev.preventDefault();
      all[Math.max(0, Math.min(all.length - 1, to))].focus();
    });
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
    // (a module still loading counts: unloaded and not yet read again, it once showed the spectrum for a moment between two
    // tunes; the user, 2026-11-12: "the spectral analyzer briefly pops up before returning to the tracker")
    function isModule() { var t = tunes[current]; return !!(t && t.kind === "module" && (source || state === "loading")); }
    // The view the display shows: the chosen one, but the spectrum where a tune has no patterns (Ogg)
    function applyVis() {
      var wasSpectrum = spectrumOn;
      var patterned = follow ? follow.view.kind === "module" : isModule();
      trackerOn = vis === "tracker" && patterned;
      pianoOn = vis === "piano" && patterned;
      scoreOn = vis === "score" && patterned;
      spectrumOn = vis === "spectrum" || ((vis === "tracker" || vis === "piano" || vis === "score") && !patterned);
      tracker.hidden = !trackerOn;
      fxBtn.hidden = !trackerOn;
      piano.hidden = !pianoOn;
      score.hidden = !scoreOn;
      if (scoreOn) loadScore();
      spectrum.hidden = !spectrumOn;
      var label = T({ spectrum: "music.vis_spectrum", tracker: "music.vis_tracker", piano: "music.vis_piano", score: "music.vis_score", off: "music.vis_off" }[vis]);
      visEl.setAttribute("aria-label", label);
      visEl.title = label;
      if (spectrumOn !== wasSpectrum) route();
      syncMutes();
      shownRow = "";
    }
    // The one place the state changes: the buttons, the list, the display and the media session follow it
    function setState(next) {
      var was = state;
      state = next;
      // the desktop lets one window be heard at a time (web/deskapps.js): it asks again when this player starts or stops
      if ((was === "playing") !== (next === "playing")) try { window.dispatchEvent(new CustomEvent("byopop-music-state", { detail: { playing: next === "playing" } })); } catch (e) {}
      var going = state === "playing" || state === "loading", label = T(going ? "music.pause" : "music.play");
      playBtn.replaceChild(glyph(going ? "pause" : "play"), playBtn.firstChild);
      playBtn.setAttribute("aria-label", label);
      playBtn.title = label;
      playBtn.disabled = !tunes.length;
      prevBtn.disabled = nextBtn.disabled = !tunes.length;
      stopBtn.disabled = state === "idle";
      seek.disabled = !source && !followedClock();
      stateGlyph.replaceChild(glyph(state === "playing" ? "play" : state === "paused" ? "pause" : "stop"), stateGlyph.firstChild);
      stateGlyph.classList.toggle("mu-blink", state === "loading" || state === "paused");
      Array.prototype.forEach.call(list.querySelectorAll("button[data-tune]"), function (b) {
        if (+b.getAttribute("data-tune") === current && state !== "idle") {
          b.setAttribute("aria-current", "true");
          var ol = b.closest("ol"), head = ol && ol.previousSibling && ol.previousSibling.querySelector && ol.previousSibling.querySelector(".mu-fold");
          if (head && ol.hidden) fold(head, false);   // the tune playing is never hidden in a folded game
        } else b.removeAttribute("aria-current");
      });
      var t = tunes[current];
      if (t && state !== "idle") {
        tuneEl.textContent = t.name;
        gameEl.textContent = subLine(t);
        names.title = t.name + " — " + t.game.title;
      } else say("", "");
      shownTrack = null;
      showTrack();
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
        sink(false);
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

    // ---------- the keyboard's media keys ----------
    // (the user, 2026-11-11: "My keyboard has media buttons ... could functionality be added to have them control the music player
    // if it's open?") A media key pressed while this page, or a game window's page on it, has the focus drives the player while
    // its window is open, wherever the focus is: Play/Pause, Next and Previous in the list, Stop. Closed, the keys do nothing. The
    // key is not taken from the page (the input display still shows it); no port reads these keys (none maps SDL's AUDIOPLAY,
    // AUDIONEXT, AUDIOPREV or AUDIOSTOP). With the browser's window in the background only the Media Session's handlers above
    // could hear them, and browsers give a page that session only while a media element plays.
    function playerOpen() {
      var win = root.closest(".d95-win");
      if (win) return !win.classList.contains("d95-closed");
      return root.getClientRects().length > 0;
    }
    var MEDIA_KEYS = {
      MediaPlayPause: function () { if (state === "playing") pause(); else if (state === "paused") resume(); else if (state === "idle" && tunes.length) play(current >= 0 ? current : 0); },
      MediaTrackNext: function () { next(); },
      MediaTrackPrevious: function () { previous(); },
      MediaStop: function () { if (state !== "idle") stop(); }
    };
    function mediaKey(ev) {
      var fn = ev && !ev.repeat && MEDIA_KEYS[ev.key];
      if (!fn || !playerOpen()) return;
      fn();
    }
    window.addEventListener("keydown", mediaKey, true);
    // and in the game windows' pages (web/deskapps.js), as the splits timer's hotkeys are heard there (web/splits.js)
    function hookMediaKeys() {
      Array.prototype.forEach.call(document.querySelectorAll("iframe.d95-app-frame"), function (f) {
        try { var w = f.contentWindow; if (!w || w.__byopopMediaKeys === mediaKey) return; w.addEventListener("keydown", mediaKey, true); w.__byopopMediaKeys = mediaKey; } catch (e) {}
      });
    }
    if (window.MutationObserver) new MutationObserver(hookMediaKeys).observe(document.body, { childList: true });
    document.addEventListener("load", function (ev) { if (ev.target && ev.target.classList && ev.target.classList.contains("d95-app-frame")) hookMediaKeys(); }, true);
    setInterval(function () { hookMediaKeys(); sink(false); }, 2000);

    // ---------- the media keys with the browser in the background ----------
    // (the user, 2026-11-12: "Yes, prototype") A browser hands a page its media session, and so the keyboard's media keys while
    // another window has the focus, only while a media element of the page plays; Web Audio alone does not count. While Music's
    // window is open its sound goes through a MediaStreamDestination into a hidden <audio> element that plays while a tune plays
    // or is paused, and the session's handlers (wireMedia) hear the keys. The element's path adds output latency, so with the
    // window closed (where the keys do nothing anyway) the sound goes straight to the speakers again.
    var tail = null, streamOut = null, streamEl = null, viaStream = false;
    function sink(force) {
      if (!ctx || !tail) return;
      var want = playerOpen() && state !== "idle" && typeof ctx.createMediaStreamDestination === "function";
      if (want && !streamOut) {
        try {
          streamOut = ctx.createMediaStreamDestination();
          streamEl = document.createElement("audio");
          streamEl.srcObject = streamOut.stream;
        } catch (e) { streamOut = null; streamEl = null; want = false; }
      }
      if (force || want !== viaStream) {
        try { tail.disconnect(); } catch (e) {}
        tail.connect(want ? streamOut : ctx.destination);
        viaStream = want;
      }
      if (streamEl) {
        if (want && streamEl.paused) { var p = streamEl.play(); if (p && p.catch) p.catch(function () {}); }
        else if (!want && !streamEl.paused) streamEl.pause();
      }
    }
    function dropSink() {
      if (streamEl) { try { streamEl.pause(); streamEl.srcObject = null; } catch (e) {} }
      streamEl = null; streamOut = null; viaStream = false; tail = null;
    }
    (function watchWindow() {
      var win = root.closest(".d95-win");
      if (win && window.MutationObserver) new MutationObserver(function () { sink(false); }).observe(win, { attributes: true, attributeFilter: ["class"] });
    })();

    // ---------- the sound ----------
    // A play chosen while the worklet is still being added waits for the same setup (audioReady): it once found the context
    // made and the worklet's node not yet there, and failed ("not played: Cannot read properties of null (reading 'port')").
    var audioReady = null;
    function audio() {
      if (ctx && audioReady) return audioReady;
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC || !AC.prototype || !("audioWorklet" in AC.prototype)) return Promise.reject(new Error("unsupported"));
      ctx = new AC();
      var made = ctx;
      audioReady = makeGraph(made);
      audioReady.catch(function () { if (ctx === made) { ctx = null; audioReady = null; } });
      return audioReady;
    }
    function makeGraph(ctx) {
      gain = ctx.createGain();
      gain.gain.value = muted ? 0 : volume / 100;
      // at ?silent=1 the sound passes through a gain of 0 (CLAUDE.md: tests in a pane that may be hidden): the speakers get exact
      // zeros, and the graph still reaches the destination, without which the browser never runs the worklet
      tail = gain;
      if (silentPage()) {
        tail = ctx.createGain();
        tail.gain.value = 0;
        gain.connect(tail);
      }
      sink(true);
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
        if (!analyser) { analyser = ctx.createAnalyser(); analyser.fftSize = 2048; analyser.smoothingTimeConstant = SMOOTHING; analyser.minDecibels = -85; analyser.maxDecibels = -15; }
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
      var again = false, stopAt = 0;
      if (range) { M._bm_loop(m, 1); M._bm_set_order(m, range[0]); }
      return {
        repeat: function (on) { again = on; if (!range) M._bm_loop(m, on ? 1 : 0); },
        // where the tune ends instead of its own end, in seconds into it (a long silent ending cut: trimEnd), 0 for its own
        stop: function (s) { stopAt = s; },
        next: function (frames, marks) {
          if ((range && M._bm_order(m) > range[1]) || (stopAt && M._bm_position(m) - start >= stopAt)) {
            if (!again) return null;
            if (range) M._bm_set_order(m, range[0]); else M._bm_seek(m, ctx.sampleRate, start);
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
        // its instruments' names (its samples' where it has no instruments), a line each: "1A Bass"
        names: function () { return typeof M._bm_names === "function" ? M.UTF8ToString(M._bm_names(m)) : ""; },   // (a decoder cached from before it: none)
        // a channel muted or brought back; false where the decoder cannot (one cached from before bm_mute). It is heard once the
        // second the decoder has rendered ahead has played
        mute: function (c, on) { return typeof M._bm_mute === "function" && M._bm_mute(m, c, on ? 1 : 0) === 1; },
        // whether the player itself has a channel muted: 1, 0, or -1 not known (for tests: tools/web/musicmutecheck.js)
        muteStatus: function (c) { return typeof M._bm_mute_status === "function" ? M._bm_mute_status(m, c) : -1; },
        // a pattern's rows per beat and per bar as the file stores them (beat << 16 | bar), 0 where it stores none
        meter: function (pattern) { return typeof M._bm_meter === "function" ? M._bm_meter(m, pattern) : 0; },
        // the speed (ticks a row) and tempo playing now, or null
        tempo: function () { var t = typeof M._bm_tempo === "function" ? M._bm_tempo(m) : 0; return t ? { speed: t >> 16, bpm: t & 0xffff } : null; },
        // the channels that play a note anywhere in the module, by number
        used: function () {
          var buf = M._malloc(256), n = Math.min(256, M._bm_used(m, buf, 256)), list = [];
          for (var c = 0; c < n; c++) if (M.HEAPU8[buf + c]) list.push(c);
          M._free(buf);
          return list;
        },
        // the lowest and highest note anywhere in it (1 is C-0), or null for none
        range: function () {
          var buf = M._malloc(8);
          M._bm_range(m, buf);
          var lo = M.HEAP32[buf >> 2], hi = M.HEAP32[(buf >> 2) + 1];
          M._free(buf);
          return lo ? [lo, hi] : null;
        },
        // the pattern an order plays and its rows; "skip" for a marker the player passes over; null past the end
        orderPattern: function (order) {
          var p = M._bm_order_pattern(m, order);
          return p >= 0 ? [p, M._bm_pattern_rows(m, p)] : p === -2 ? "skip" : null;
        },
        free: function () { M._bm_free(m); }
      };
    }
    // A module tune of segments (web/soundtracks.js's "cuts" and "chains"): each plays its subsong from order from (or its
    // start) until it reaches order until (or its end), then the next; with repeat on the first again. The views and the rest
    // are moduleSource's. Seeking renders unheard from the start (libopenmpt at 8000 Hz, which costs little; libxmp-lite at
    // the context's rate, as a rate change restarts it).
    function segSource(m, segs, gain, fast) {
      var src = moduleSource(m, null, gain, 0), i = 0, done = 0, again = false, stopAt = 0, pass = 0;
      function begin(k) {
        i = k;
        if (k === 0) pass = 0;
        var sg = segs[k];
        M._bm_select(m, sg.sub);
        M._bm_loop(m, 0);
        if (sg.from !== null) M._bm_set_order(m, sg.from);
      }
      // frames at rate into ptr (or a scratch buffer), the segments followed; returns those rendered
      function render(rate, frames, ptr, marks) {
        var got = 0, fresh = 0;
        while (got < frames) {
          var sg = segs[i];
          if (sg.until !== null && M._bm_order(m) === sg.until) {
            if (i + 1 < segs.length) { begin(i + 1); continue; }
            if (!again || fresh++) break;
            begin(0); continue;
          }
          if (marks) {
            M._bm_view(m, viewBuf);
            var w = viewBuf >> 2;
            marks.push([got, M.HEAP32[w], M.HEAP32[w + 1], M.HEAP32[w + 2], M.HEAP32[w + 3], M.HEAP32[w + 4]]);
          }
          var n = Math.min(SLICE, frames - got), g = M._bm_render(m, rate, n, ptr + got * 8);
          got += g;
          if (g < n) {
            if (g === 0 && fresh++ > 1) break;   // nothing more anywhere
            if (i + 1 < segs.length) begin(i + 1);
            else if (again) begin(0);
            else break;
          } else fresh = 0;
        }
        return got;
      }
      begin(0);
      src.repeat = function (on) { again = on; };
      // (a long silent ending cut, as moduleSource's: the pass ends stopAt seconds in)
      src.stop = function (s) { stopAt = s; };
      src.next = function (frames, marks) {
        if (stopAt && pass >= stopAt * ctx.sampleRate) {
          if (!again) return null;
          begin(0);
        }
        if (stopAt) frames = Math.max(1, Math.min(frames, Math.ceil(stopAt * ctx.sampleRate) - pass));
        if (!viewBuf) viewBuf = M._malloc(20);
        var out = M._malloc(frames * 8), got = render(ctx.sampleRate, frames, out, marks);
        var c = got > 0 ? M.HEAPF32.slice(out >> 2, (out >> 2) + got * 2) : null;
        M._free(out);
        if (c && gain !== 1) for (var j = 0; j < c.length; j++) c[j] *= gain;
        if (c) { done += got; pass += got; }
        return c;
      };
      src.position = function () { return done / ctx.sampleRate; };
      src.seek = function (sec) {
        var rate = fast ? 8000 : ctx.sampleRate, buf = M._malloc(4096 * 8), left = Math.round(sec * rate), keep = again;
        again = false;
        begin(0);
        while (left > 0) {
          var g = render(rate, Math.min(4096, left), buf, null);
          if (g <= 0) break;
          left -= g;
        }
        M._free(buf);
        again = keep;
        done = pass = Math.round(sec * ctx.sampleRate);
      };
      return src;
    }
    // Decoded sound: interleaved stereo at the context's rate; with repeat on it starts again at frame loopAt (an intro played
    // once, then its loop: web/soundtracks.js's "loopFrom")
    function pcmSource(data, loopAt) {
      var at = 0, again = false, total = data.length >> 1;
      return {
        repeat: function (on) { again = on; },
        next: function (frames) {
          if (at >= total) {
            if (!again || !total) return null;
            at = Math.min(loopAt || 0, total - 1);
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
    // A long silent ending skipped (the user's notes, 2026-11-10: "Some of the short tracks have a bunch of dead air at the end,
    // where the tracker slows to a crawl. The ending silence should be skipped somehow if long."): when a module's tune starts,
    // a second copy of it renders its last TRIM_SCAN seconds at TRIM_RATE, unheard, to find the last moment any sample reaches
    // TRIM_LEVEL (-60 dB); where more than TRIM_LONG seconds of silence follow it, the tune ends TRIM_KEEP seconds after it, as
    // it plays on and in the length shown (the list's, the clock's and the seek bar's), and with repeat on it starts again
    // there. A tune's length is known this way once it has been played; trims holds them by tune for the page's life. Returns
    // the new end in seconds into the tune, or 0 to keep its own.
    var TRIM_SCAN = 60, TRIM_RATE = 11025, TRIM_LEVEL = 0.001, TRIM_LONG = 3, TRIM_KEEP = 1, trims = {};
    function trimEnd(t, bytes) {
      var key = t.keys[0] + "#" + (t.range ? t.range.join("-") : t.segs ? t.id : t.sub);
      if (!(key in trims)) trims[key] = scanEnd(t, bytes);
      var end = trims[key];
      if (end && t.seconds !== end) {
        t.seconds = end;
        var shown = list && list.querySelector('[data-tune="' + tunes.indexOf(t) + '"] .mu-time');
        if (shown) shown.textContent = clock(end);
      }
      return end;
    }
    function scanEnd(t, bytes) {
      var m = bytes ? loadModule(M, bytes) : 0;
      if (!m) return 0;
      var out = M._malloc(4096 * 8), start = t.start || 0, last = -1, length = t.seconds;
      try {
        var until = t.segs ? t.segs[0].until : null;
        if (!t.range) M._bm_select(m, t.segs ? t.segs[0].sub : t.sub);
        var from = Math.max(0, length - TRIM_SCAN);
        M._bm_seek(m, TRIM_RATE, start + from);
        for (var guard = 0; guard < 400; guard++) {
          if ((t.range && M._bm_order(m) > t.range[1]) || (until !== null && M._bm_order(m) === until)) break;
          var at = M._bm_position(m) - start;
          if (at >= length + 1) break;
          var got = M._bm_render(m, TRIM_RATE, 4096, out);
          if (got <= 0) break;
          var a = M.HEAPF32.subarray(out >> 2, (out >> 2) + got * 2);
          for (var i = a.length - 1; i >= 0; i--) if (a[i] > TRIM_LEVEL || a[i] < -TRIM_LEVEL) { last = at + (i >> 1) / TRIM_RATE; break; }
          if (got < 4096) break;
        }
      } finally { M._free(out); M._bm_free(m); }
      return last >= 0 && length - last > TRIM_LONG ? Math.round((last + TRIM_KEEP) * 100) / 100 : 0;
    }
    // The source of a tune, from its files (a joined tune's pieces decoded once each, then put one after the other)
    function sourceOf(t, files) {
      if (t.kind === "module") {
        var m = files[0] ? loadModule(M, files[0]) : 0;
        if (!m) throw new Error("module");
        if (t.segs) {
          var seg = segSource(m, t.segs, levelGain(M, t.level), /\.(mo3|oxm)$/i.test(t.keys[0]) || /\|pe:/.test(t.keys[0]));
          // (a piece from its subsong's start, cut where the game starts another tune: its silent ending trimmed too)
          var cutEnd = t.segs.length === 1 && t.segs[0].from === null ? trimEnd(t, files[0]) : 0;
          if (cutEnd) seg.stop(cutEnd);
          return Promise.resolve(seg);
        }
        if (!t.range) M._bm_select(m, t.sub);
        var src = moduleSource(m, t.range, levelGain(M, t.level), t.start || 0), end = trimEnd(t, files[0]);
        if (end) src.stop(end);
        return Promise.resolve(src);
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
        var all = new Float32Array(parts.reduce(function (n, d) { return n + d.length; }, 0)), at = 0, loopAt = 0;
        parts.forEach(function (d, i) { if (i === t.loopFrom) loopAt = at >> 1; all.set(d, at); at += d.length; });
        return pcmSource(all, loopAt);
      });
    }
    // the tracker view wants the decoder's rows: shown, on the screen, and the tune a module's
    function marking() { return (trackerOn || pianoOn || scoreOn) && isModule() && onScreen(); }
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
      rowCache = {}; usedChannels = null; ownNames = null; firstChannel = 0; rollReset();
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
      // the context is made and resumed now, inside the click that chose the tune: Firefox lets a page start its sound only from
      // the press itself, and a resume after the decoder and the files were read left it suspended and the tune silent (the
      // user, 2026-11-12: "they're dead silent ... it began working when I launched a game")
      var ready = audio();
      ready.catch(function () {});
      if (ctx && ctx.state !== "running") { try { ctx.resume(); } catch (e) {} }
      (M ? Promise.resolve() : decoder().then(function (m) { M = m; })).then(function () { return ready; }).then(function () {
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
        syncMutes();
        return ctx.resume().then(function () { setState("playing"); fill(); });
      }).catch(function (e) {
        if (play.ticket !== ticket) return;
        try { console.warn("music: " + t.id + " not played:", e); } catch (x) {}
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
      unfollow();
      unload();
      if (viewBuf && M) try { M._free(viewBuf); } catch (e) {}
      viewBuf = 0;
      if (node) { try { node.port.onmessage = null; node.disconnect(); } catch (e) {} node = null; }
      if (analyser) { try { analyser.disconnect(); } catch (e) {} analyser = null; }
      if (gain) { try { gain.disconnect(); } catch (e) {} gain = null; }
      dropSink();
      if (ctx) { try { ctx.close(); } catch (e) {} ctx = null; }
      queued = 0; flushes = 0; ended = false; heads = [];
      M = null;
      dropDecoder();
      if (wired) { wired = false; unwireMedia(); }
      setState("idle");
    }
    function next() { if (follow && follow.track >= 0) { stepGame(1); return; } if (tunes.length) play((current + 1) % tunes.length); }
    // Previous, as players do: the tune's own start once it has played a few seconds, else the tune before
    function previous() {
      if (follow && follow.track >= 0) { stepGame(-1); return; }
      if (!tunes.length) return;
      if (source && current >= 0 && elapsed() > 3) { seekTo(0); return; }
      play(current < 0 ? tunes.length - 1 : (current - 1 + tunes.length) % tunes.length);
    }
    function seekTo(s) {
      if (followedClock()) { seekGame(s); return; }
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
      var game = followedClock();
      if (game) { total = game.total; at = game.at; seek.disabled = false; }
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
    var rowCache = {}, shownRow = "", shownHead = "";
    // the channels shown: only those with a note somewhere (read once a tune), as many whole columns as the view's width holds,
    // from firstChannel (the scroll bar's); each cell whole, or not shown
    // the instruments' names by their number as the cells write it ({"1A": "Bass"}), from the decoder's or the game's list
    var ownNames = null, chanIns = {}, HEAD_NAME = 6;
    // The channels muted from the tracker's header belong to the tracker view and to the tune (the user, 2026-11-11: "Channels
    // appear to stay muted when I switch tracks in the player but they still sound"; "Switching off the tracker view should
    // unmute all the tracks, but returning to it should re-mute what was previously muted"). chanMutes keeps each tune's, by
    // number ({tune key: {channel: true}}), for this visit; a tune not muted yet has none, and the same tune played again gets
    // its own back. syncMutes makes what plays agree with them: the tune's mutes while the tracker view shows it, none while
    // it does not, set again on a new player (a tune started or restarted, a game's song followed), which plays every channel.
    var chanMutes = {}, applied = { key: null, target: null, set: {} };
    function tuneKey(feed) { return follow ? feed.id + ":" + follow.track : "own:" + (tunes[current] ? tunes[current].id : current); }
    function shownMutes(feed) { return (feed && trackerOn && chanMutes[tuneKey(feed)]) || {}; }
    function syncMutes() {
      var feed = muteFeed(), target = follow ? follow.song + ":" + follow.serial : source;
      if (!feed || !target) { applied = { key: null, target: null, set: {} }; return; }
      var key = tuneKey(feed), want = shownMutes(feed);
      // a new player: every channel is set as it should be, since what the player had is not known. Where the player says what
      // it has muted (the own tune's), that is compared every time: libxmp-lite drops its mutes when its player starts, which
      // can be after a mute set while the tune loads
      var fresh = applied.target !== target || applied.key !== key;
      var set = fresh ? {} : applied.set;
      var chans = fresh || feed.status ? feed.used() : Object.keys(want).concat(Object.keys(set)).map(Number);
      chans.forEach(function (c) {
        var on = !!want[c], st = feed.status ? feed.status(c) : -1;
        var has = st >= 0 ? st === 1 : fresh ? false : !!set[c];
        if (on !== has || (fresh && st < 0 && on)) {
          if (feed.mute && feed.mute(c, on)) { if (on) set[c] = true; else delete set[c]; }
        } else if (on) set[c] = true; else delete set[c];
      });
      applied = { key: key, target: target, set: set };
    }
    function clickChannel(c, alone) {
      var feed = trackerFeed();
      if (!feed || !trackerOn) return;
      var key = tuneKey(feed), m = chanMutes[key] || (chanMutes[key] = {});
      var used = feed.used();
      if (alone) {
        // that channel alone; again on the channel already alone, every channel back
        var lone = !m[c] && used.every(function (u) { return u === c || m[u]; });
        used.forEach(function (u) { if (!lone && u !== c) m[u] = true; else delete m[u]; });
      } else if (m[c]) delete m[c]; else m[c] = true;
      syncMutes();
      shownRow = "";
      drawTracker();
    }
    function parseNames(text) {
      var out = {};
      text.split("\n").forEach(function (line) { if (line.length > 3) out[line.slice(0, 2)] = line.slice(3); });
      return out;
    }
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
    // The channels a tune itself plays notes in (the user, 2026-11-11: "Some songs seem to have totally empty channels the whole
    // way through. Possible to hide from tracker on a per-song basis?"): those with a note in a row of a pattern its own orders
    // play, read once a tune; the tracker leaves the others out, the columns kept keeping their numbers. A channel with commands
    // and no note counts as empty: its effects still show in the effect column (fxText). A tune's orders: a range's; a
    // subsong's from its first order to the next subsong's first; a cut or joined tune's segments, each from its order (or its
    // subsong's first) to the order it stops at (or its subsong's end). null where they are not known (a list read before
    // subEntries): the tracker then keeps the module's own (bm_used).
    function tuneOrders(t) {
      if (!t || t.kind !== "module" || !t.seq) return null;
      var n = t.seq.length, out = [];
      function span(first, until) {   // first to the order before until (or the end of first's subsong)
        if (first === null || first === undefined || first < 0) return false;
        var last = n - 1;
        if (until !== null && until !== undefined) last = until - 1;
        else if (t.subEntries) t.subEntries.forEach(function (e) { if (e > first && e - 1 < last) last = e - 1; });
        for (var o = first; o <= last && o < n; o++) if (out.indexOf(o) < 0) out.push(o);
        return true;
      }
      if (t.range) { span(t.range[0], t.range[1] + 1); return out; }
      if (!t.subEntries) return null;
      if (t.segs) {
        for (var i = 0; i < t.segs.length; i++) {
          var sg = t.segs[i], from = sg.from !== null && sg.from !== undefined ? sg.from : t.subEntries[sg.sub];
          if (!span(from, sg.until)) return null;
        }
        return out;
      }
      return span(t.entry, null) ? out : null;
    }
    // NOTE: a cell's note, as both decoders write it ("C-5", "C#5"); "===" (off), "^^^" (cut) and "~~~" (fade) are no note
    var NOTE = /^[A-G][-#]\d/;
    function tuneUsed(t, rowAt, rowsOf) {
      var orders = tuneOrders(t);
      if (!orders) return null;
      var found = {}, list = [];
      orders.forEach(function (o) {
        var p = t.seq[o], rows = p >= 0 ? rowsOf(o, p) : 0;
        for (var r = 0; r < rows; r++) rowAt(p, r).split("|").forEach(function (cell, c) { if (!found[c] && NOTE.test(cell)) found[c] = true; });
      });
      Object.keys(found).forEach(function (c) { list.push(+c); });
      return list.sort(function (a, b) { return a - b; });
    }
    // The effect column (the user, 2026-11-11: "add a brief English description of tracker commands when they appear in the song
    // as an optional column"): what a cell's effect, its volume column's command where it is not a plain volume, and a note off,
    // cut or fade do, in a few words. libxmp-lite's cells (the player's S3M, IT, XM and MOD; a game's libxmp-lite songs) write
    // its own effect numbers as four hex digits ("A306": its S3M speed, 6), the same whatever the file's format; libopenmpt's
    // (MO3, and a game's libopenmpt songs) write the file's own letter and a hex value ("A06"), whose meaning depends on the
    // format's family: digits and A to F are MOD and XM's, the rest of the alphabet S3M and IT's. fxStyle reads which from
    // the tune's rows: "xm" where any effect is a digit, else "it"; "" for libxmp-lite's cells.
    var FX_WIDE = 28, fxStyleKey = "", fxStyleWas = "";
    function fxStyle(feed) {
      var key = feed.id + ":" + (follow ? follow.track : current);
      if (key === fxStyleKey) return fxStyleWas;
      var rows = follow ? follow.rows : rowCache, digit = false, letter = false;
      Object.keys(rows || {}).forEach(function (k) {
        (rows[k] || "").split("|").forEach(function (cell) {
          if (cell.length !== 13) return;
          var e = cell.charAt(10);
          if (/[0-9]/.test(e)) digit = true; else if (/[A-Z]/.test(e)) letter = true;
        });
      });
      fxStyleKey = key;
      fxStyleWas = digit ? "xm" : "it";
      return fxStyleWas;
    }
    function fx(id, n) { return T("music.fx." + id, n === undefined ? {} : typeof n === "object" ? n : { n: n }); }
    // a volume slide xy: up by x, down by y; in S3M and IT (fine true) xF is a fine slide up by x and Fy one down by y
    function volSlide(p, fine) {
      var x = p >> 4, y = p & 15;
      if (fine && y === 15 && x) return fx("finevolup", x);
      if (fine && x === 15 && y) return fx("finevoldown", y);
      return x ? fx("volup", x) : y ? fx("voldown", y) : fx("volslide");
    }
    // a pitch slide in S3M and IT: Fx fine, Ex extra fine, else by the value
    function pitchSlide(p, up, fine) {
      var x = p >> 4, y = p & 15;
      if (fine && x === 15) return fx(up ? "fineup" : "finedown", y);
      if (fine && x === 14) return fx(up ? "xfineup" : "xfinedown", y);
      return fx(up ? "up" : "down", p);
    }
    function xy(id, p) { return fx(id, { x: p >> 4, y: p & 15 }); }
    // MOD and XM's E commands, libxmp-lite's extended (S3M and IT's S commands come through it as these too)
    function extended(p) {
      var x = p >> 4, y = p & 15;
      switch (x) {
        case 1: return fx("fineup", y);
        case 2: return fx("finedown", y);
        case 3: return fx("gliss");
        case 4: return fx("vibwave", y);
        case 5: return fx("finetune", y);
        case 6: return y ? fx("loop", y) : fx("loopstart");
        case 7: return fx("tremwave", y);
        case 8: return fx("pan", y * 17);
        case 9: return fx("retrig", y);
        case 10: return fx("finevolup", y);
        case 11: return fx("finevoldown", y);
        case 12: return fx("cut", y);
        case 13: return fx("delay", y);
        case 14: return fx("rowdelay", y);
        default: return fx("other", { code: "E" + x.toString(16).toUpperCase() + y.toString(16).toUpperCase() });
      }
    }
    function xmpEffect(t, p) {
      var x = p >> 4, y = p & 15;
      switch (t) {
        case 0x00: return p ? fx("arp", { x: x, y: y }) : "";
        case 0x01: return pitchSlide(p, true, true);
        case 0x02: return pitchSlide(p, false, true);
        case 0x03: return p ? fx("porta", p) : fx("portaon");
        case 0x04: case 0x92: return p ? xy("vibrato", p) : fx("vibratoon");
        case 0x05: return fx("plus", { a: fx("portaon"), b: volSlide(p, true) });
        case 0x06: return fx("plus", { a: fx("vibratoon"), b: volSlide(p, true) });
        case 0x07: return p ? xy("tremolo", p) : fx("tremoloon");
        case 0x08: return fx("pan", p);
        case 0x09: return fx("offset", p * 256);
        case 0x0a: case 0xa4: return volSlide(p, true);
        case 0x0b: return fx("jump", p);
        case 0x0c: return fx("volume", p);
        case 0x0d: return fx("break", x * 10 + y);
        case 0x8e: return fx("break", p);
        case 0x0e: return extended(p);
        case 0x0f: return p < 0x20 ? fx("speed", p) : fx("tempo", p);
        case 0xa3: return fx("speed", p);
        case 0xab: return fx("tempo", p);
        case 0x87: return p < 0x10 ? fx("tempodown", p) : p < 0x20 ? fx("tempoup", y) : fx("tempo", p);
        case 0x10: return fx("gvolume", p);
        case 0x11: return x ? fx("gvolup", x) : fx("gvoldown", y);
        case 0x14: return fx("keyoff", p);
        case 0x15: return fx("envpos", p);
        case 0x19: case 0x89: case 0xb5: return x ? fx("panright", x) : fx("panleft", y);
        case 0x1b: case 0xbb: return fx("retrig", y);
        case 0x1d: return xy("tremor", p);
        case 0x21: return x === 1 ? fx("xfineup", y) : fx("xfinedown", y);
        case 0x80: return fx("chvolume", p);
        case 0x81: case 0x82: return x ? fx("chvolup", x) : fx("chvoldown", y);
        case 0x83: return fx("instctl", p);
        case 0x88: return fx("rowdelay", p);
        case 0x8a: return xy("panbrello", p);
        case 0x8b: return fx("panwave", p);
        case 0x8c: return fx("hioffset", p);
        case 0x8d: return fx("surround");
        case 0x8f: return fx("reverse");
        case 0xac: return xy("finevibrato", p);
        case 0xb4: return fx("arp", { x: x, y: y });
        case 0xbd: case 0xbe: case 0xbf: return fx("macro", p);
        case 0xc0: return fx("volup", p);
        case 0xc1: return fx("voldown", p);
        case 0xc2: return fx("finevolup", p);
        case 0xc3: return fx("finevoldown", p);
        default: return fx("other", { code: (t < 16 ? "0" : "") + t.toString(16).toUpperCase() + (p < 16 ? "0" : "") + p.toString(16).toUpperCase() });
      }
    }
    // libopenmpt's letters: S3M and IT's
    function itEffect(e, p) {
      var x = p >> 4, y = p & 15;
      switch (e) {
        case "A": return fx("speed", p);
        case "B": return fx("jump", p);
        case "C": return fx("break", p);
        case "D": return volSlide(p, true);
        case "E": return pitchSlide(p, false, true);
        case "F": return pitchSlide(p, true, true);
        case "G": return p ? fx("porta", p) : fx("portaon");
        case "H": return p ? xy("vibrato", p) : fx("vibratoon");
        case "I": return xy("tremor", p);
        case "J": return fx("arp", { x: x, y: y });
        case "K": return fx("plus", { a: fx("vibratoon"), b: volSlide(p, true) });
        case "L": return fx("plus", { a: fx("portaon"), b: volSlide(p, true) });
        case "M": return fx("chvolume", p);
        case "N": return x ? fx("chvolup", x) : fx("chvoldown", y);
        case "O": return fx("offset", p * 256);
        case "P": return x ? fx("panright", x) : fx("panleft", y);
        case "Q": return fx("retrig", y);
        case "R": return p ? xy("tremolo", p) : fx("tremoloon");
        case "S":
          switch (x) {
            case 1: return fx("gliss");
            case 2: return fx("finetune", y);
            case 3: return fx("vibwave", y);
            case 4: return fx("tremwave", y);
            case 5: return fx("panwave", y);
            case 6: return fx("tickdelay", y);
            case 7: return fx("instctl", y);
            case 8: return fx("pan", y * 17);
            case 9: return y === 1 ? fx("surround") : fx("other", { code: "S9" + y.toString(16).toUpperCase() });
            case 10: return fx("hioffset", y);
            case 11: return y ? fx("loop", y) : fx("loopstart");
            case 12: return fx("cut", y);
            case 13: return fx("delay", y);
            case 14: return fx("rowdelay", y);
            case 15: return fx("macro", y);
          }
          return fx("other", { code: "S" + x.toString(16).toUpperCase() + y.toString(16).toUpperCase() });
        case "T": return p < 0x10 ? fx("tempodown", p) : p < 0x20 ? fx("tempoup", y) : fx("tempo", p);
        case "U": return xy("finevibrato", p);
        case "V": return fx("gvolume", p);
        case "W": return x ? fx("gvolup", x) : fx("gvoldown", y);
        case "X": return fx("pan", p);
        case "Y": return xy("panbrello", p);
        case "Z": case "\\": return fx("macro", p);
      }
      return fx("other", { code: e + (p < 16 ? "0" : "") + p.toString(16).toUpperCase() });
    }
    // libopenmpt's letters: MOD and XM's
    function xmEffect(e, p) {
      var x = p >> 4, y = p & 15;
      switch (e) {
        case "0": return p ? fx("arp", { x: x, y: y }) : "";
        case "1": return fx("up", p);
        case "2": return fx("down", p);
        case "3": return p ? fx("porta", p) : fx("portaon");
        case "4": return p ? xy("vibrato", p) : fx("vibratoon");
        case "5": return fx("plus", { a: fx("portaon"), b: volSlide(p, false) });
        case "6": return fx("plus", { a: fx("vibratoon"), b: volSlide(p, false) });
        case "7": return p ? xy("tremolo", p) : fx("tremoloon");
        case "8": return fx("pan", p);
        case "9": return fx("offset", p * 256);
        case "A": return volSlide(p, false);
        case "B": return fx("jump", p);
        case "C": return fx("volume", p);
        case "D": return fx("break", p);
        case "E": return extended(p);
        case "F": return p < 0x20 ? fx("speed", p) : fx("tempo", p);
        case "G": return fx("gvolume", p);
        case "H": return x ? fx("gvolup", x) : fx("gvoldown", y);
        case "K": return fx("keyoff", p);
        case "L": return fx("envpos", p);
        case "P": return x ? fx("panright", x) : fx("panleft", y);
        case "R": return fx("retrig", y);
        case "T": return xy("tremor", p);
        case "X": return x === 1 ? fx("xfineup", y) : fx("xfinedown", y);
      }
      return fx("other", { code: e + (p < 16 ? "0" : "") + p.toString(16).toUpperCase() });
    }
    // libopenmpt's volume column, where it is not a plain volume ("v64")
    function volColumn(v) {
      var c = v.charAt(0), n = parseInt(v.slice(1), 10) || 0;
      switch (c) {
        case "p": return fx("pan", Math.min(255, n * 4));
        case "c": return fx("volup", n);
        case "d": return fx("voldown", n);
        case "a": return fx("finevolup", n);
        case "b": return fx("finevoldown", n);
        case "u": return fx("vibspeed", n);
        case "h": return fx("vibdepth", n);
        case "l": return fx("panleft", n);
        case "r": return fx("panright", n);
        case "g": return n ? fx("porta", n) : fx("portaon");
        case "f": return fx("up", n);
        case "e": return fx("down", n);
        case "o": return fx("offset", n * 256);
      }
      return "";
    }
    function fxText(cell, style) {
      var out = [], note = cell.slice(0, 3);
      if (note === "===") out.push(fx("noteoff"));
      else if (note === "^^^") out.push(fx("notecut"));
      else if (note === "~~~") out.push(fx("notefade"));
      if (cell.length === 15) {   // libxmp-lite: "C-5 01 v64 A306"
        var code = cell.slice(11, 15);
        if (/^[0-9A-F]{4}$/.test(code)) { var d = xmpEffect(parseInt(code.slice(0, 2), 16), parseInt(code.slice(2), 16)); if (d) out.push(d); }
      } else if (cell.length === 13) {   // libopenmpt: "C-5 01v64 A06"
        var vol = cell.slice(6, 9);
        if (/^[a-z]/.test(vol) && vol.charAt(0) !== "v") { var vd = volColumn(vol); if (vd) out.push(vd); }
        var e = cell.charAt(10), p = parseInt(cell.slice(11, 13), 16);
        if (e !== "." && e !== " " && !isNaN(p)) { var ed = style === "xm" ? xmEffect(e, p) : itEffect(e, p); if (ed) out.push(ed); }
      }
      return out;
    }
    // What the tracker shows: the followed game's module where it plays one, else the player's own tune. mark is [frame, pattern,
    // row, rows, channels, order]; rowAt(pattern, row) a row's cells; used() the channels that play a note
    // the player's own tune's channels that play a note (read once a tune)
    function ownUsed() {
      if (!usedChannels) {
        usedChannels = source.orderPattern ? tuneUsed(tunes[current], rowOf, function (o) { var op = source.orderPattern(o); return op && op !== "skip" ? op[1] : 0; }) : null;
        if (!usedChannels) usedChannels = source.used ? source.used() : [];
      }
      return usedChannels;
    }
    // what the mutes are set on: the tracker's feed, or the player's own tune from the moment it is loaded, before it has a row to
    // show (the sound must not wait for the drawing)
    function muteFeed() {
      if (follow) return trackerFeed();
      if (!source || !source.mute || !source.rowText) return null;
      return { id: "own", used: ownUsed, mute: function (c, on) { return !!source.mute(c, on); },
        status: function (c) { return source.muteStatus ? source.muteStatus(c) : -1; } };
    }
    function trackerFeed() {
      if (follow) {
        var v = follow.view;
        if (v.kind !== "module" || !(v.rows > 0)) return null;
        return { mark: [0, v.pattern, v.row, v.rows, v.channels, v.order], rowAt: followedRow, used: followedUsed, names: followedNames, mute: followedMute,
          meter: followedMeter, tempo: function () { return v.speed > 0 && v.bpm > 0 ? { speed: v.speed, bpm: v.bpm } : null; }, id: "g" + follow.serial };
      }
      if (!source || !source.rowText) return null;
      var h = heard(), mark = null;
      if (h && h.head.marks) h.head.marks.forEach(function (m) { if (m[0] <= h.into) mark = m; });
      if (!mark) return null;
      return { mark: mark, rowAt: rowOf, used: ownUsed,
        names: function () { if (!ownNames) ownNames = parseNames(source.names ? source.names() : ""); return ownNames; },
        mute: function (c, on) { return !!(source.mute && source.mute(c, on)); },
        meter: function (p) { return source.meter ? source.meter(p) : 0; }, tempo: function () { return source.tempo ? source.tempo() : null; }, id: "own" };
    }
    function drawTracker() {
      if (tracker.hidden) { syncMutes(); return; }
      var feed = trackerFeed();
      if (!feed) return;
      syncMutes();
      var mark = feed.mark, rowOf = feed.rowAt;
      var cells = rowOf(mark[1], mark[2]).split("|");
      var used = feed.used().filter(function (c) { return c < cells.length; });
      if (!used.length) used = cells.map(function (x, c) { return c; });
      // the full cell (note, instrument, volume, effect) when every channel fits with it, else note and instrument
      var full = 0, small = 0;
      cells.forEach(function (c) { full = Math.max(full, c.length); small = Math.max(small, narrow(c).length); });
      var cw = charWidth(), room = Math.floor((trackerRows.clientWidth - 8) / cw), lead = String(mark[3] - 1).length + 1;
      // the effect column at the right (fxText), FX_WIDE characters while the narrow cells leave a channel room beside it
      var fxWidth = fxOn ? Math.max(0, Math.min(FX_WIDE, room - lead - 8)) : 0;
      if (fxWidth) room -= fxWidth + 1;
      function fits(width) { return Math.max(0, Math.floor((room - lead + 1) / (width + 1))); }
      var wide = fits(full) >= used.length, width = wide ? full : small, shown = Math.max(1, Math.min(used.length, fits(width)));
      firstChannel = Math.max(0, Math.min(firstChannel, used.length - shown));
      shownChannels = shown; totalChannels = used.length;
      var key = [tuneKey(feed), mark[1], mark[2], firstChannel, shown, width, room, used.join(","), fxOn ? 1 : 0, Object.keys(shownMutes(feed)).join(",")].join(":");
      if (key === shownRow) return;
      shownRow = key;
      var at = T("music.tracker_at", { order: two(mark[5]), pattern: two(mark[1]), row: two(mark[2]), rows: two(mark[3]) });
      if (shown < used.length) at += "  " + T("music.tracker_ch", { first: firstChannel + 1, last: firstChannel + shown, count: used.length });
      trackerAt.textContent = at;
      var showing = used.slice(firstChannel, firstChannel + shown);
      // each column's channel number and the name of the instrument it played last, up to the heard row (in an earlier pattern
      // where this one has not set it yet), cut to the column's width; the number alone where the column leaves fewer than
      // HEAD_NAME characters for the name (the narrow cells' three would be noise)
      var names = feed.names ? feed.names() : {};
      if (chanIns.id !== feed.id) chanIns = { id: feed.id };
      for (var r0 = 0; r0 <= mark[2]; r0++) {
        var cells0 = rowOf(mark[1], r0).split("|");
        showing.forEach(function (c) { var t = (cells0[c] || "").slice(4, 6); if (/^[0-9A-F]{2}$/.test(t) && t !== "00") chanIns[c] = t; });
      }
      // each a span that a click mutes or brings back (Ctrl-click: that channel alone), a muted one dimmed and struck through
      var muted = shownMutes(feed);
      // the header is built again only when what it shows changes, so that a press on a channel finds the same element when
      // it is released (rebuilt every row, a click often lost its target, and Firefox then gives the click to the display)
      var heads = showing.map(function (c) {
        var nm = width - 3 >= HEAD_NAME && chanIns[c] && names[chanIns[c]];
        return { c: c, text: pad(two(c + 1) + (nm ? " " + nm : ""), width), full: nm ? two(c + 1) + " " + names[chanIns[c]] : "" };
      });
      var headKey = String(mark[3]) + "|" + fxWidth + "|" + heads.map(function (h) { return h.c + ":" + h.text + ":" + h.full + ":" + (muted[h.c] ? 1 : 0); }).join("|");
      if (headKey !== shownHead) {
        shownHead = headKey;
        trackerHead.textContent = pad("", String(mark[3] - 1).length) + " ";
        heads.forEach(function (h, i) {
          if (i) trackerHead.appendChild(document.createTextNode("│"));
          var span = make("span", muted[h.c] ? "mu-ch mu-ch-off" : "mu-ch", h.text);
          span.setAttribute("data-ch", String(h.c));
          // the instrument's whole name, which the column may cut (the user, 2026-11-12: "Should be able to hover over them to
          // view the full name"); it is the instrument the channel played last, so it changes as the channel plays another
          if (h.full) span.title = h.full;
          trackerHead.appendChild(span);
        });
        if (fxWidth) trackerHead.appendChild(document.createTextNode("│" + pad(T("music.fx_head"), fxWidth)));
      }
      var style = fxWidth ? fxStyle(feed) : "";
      trackerRows.textContent = "";
      // the view stays inside the pattern (the user, 2026-11-09: "it should never scroll empty lines into the top or bottom"): the
      // heard row in the middle only while the view can scroll to it, so at the pattern's start the view sits at row 0 and the
      // highlight moves down to the middle, and near its end the last row stays at the bottom while the highlight moves down to
      // it; a display with room for the whole pattern does not scroll at all ("it shouldn't scroll at all, but just move the
      // cursor down"), the rows below the pattern left empty. A new pattern starts at its own row 0, so it shows from its top.
      var above = Math.floor((rows - 1) / 2);
      var top = Math.max(0, Math.min(mark[2] - above, mark[3] - rows));
      for (var r = top; r < top + rows; r++) {
        var line = make("span", r === mark[2] ? "mu-row mu-row-now" : "mu-row");
        if (r >= 0 && r < mark[3]) {
          var row = rowOf(mark[1], r).split("|");
          line.textContent = rowNumber(r, mark[3]) + " " + showing.map(function (c) { return pad(wide ? (row[c] || "") : narrow(row[c] || ""), width); }).join("│");
          if (fxWidth) {
            // the channels shown first, then the others (scrolled off, or hidden for having no note: a speed or a jump there
            // still shows), each with its number
            var said = [];
            showing.concat(row.map(function (x, c) { return c; }).filter(function (c) { return showing.indexOf(c) < 0; })).forEach(function (c) {
              var d = fxText(row[c] || "", style);
              if (d.length) said.push(two(c + 1) + " " + d.join(", "));
            });
            line.textContent += "│" + pad(said.join("  "), fxWidth);
            if (said.length) line.title = said.join("\n");
          }
        } else line.textContent = " ";
        trackerRows.appendChild(line);
      }
      trackerScroll.hidden = shown >= used.length;
      placeThumb();
    }
    // The piano roll (the user, 2026-11-09: "add the piano roll too"): the notes as bars on a pitch grid, time running left to
    // right in rows with the heard row at a fixed playhead, pitch up the display, one colour for each instrument. A note lasts
    // until the next note, key off, cut or fade in its channel, or to its pattern's end. It reads the cells the tracker reads,
    // each pattern once (its notes kept), and shows the pattern heard with the orders on either side of it (for a game's module,
    // whose order list the page does not have, the pattern heard before it and none after). The pitch range is the
    // tune's own lowest to highest note: the player's own decoder gives it at once; for a game's module it is the notes of the
    // patterns heard so far, so it can widen while the tune plays. About a pattern fills the width; the pitches are named at
    // the left in the tracker's spelling, each pattern's beats and bars are lines, and each bar carries its instrument's number
    // (or, where the bars are too thin, a key at the bottom right gives each colour's number).
    var ROLL_ROWS = 64, KEY_NAME = 12, KEY_LEAST = 6, BLACK_KEYS = [1, 3, 6, 8, 10];
    var PITCH_NAMES = ["C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"];
    var ROLL = ["#00ff00", "#ffff00", "#00ffff", "#ff00ff", "#ff0000", "#ffffff", "#c0c0c0", "#808000", "#008080", "#800080",
      "#008000", "#800000"];   // the instruments' colours, the bright and the dark of the 16-colour palette (no blue: too dark on black)
    var NAMES = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };
    // { id, patterns: {pattern: {pattern, rows, notes: [[start, end, pitch, colour, instrument]]}}, lo, hi, exact,
    //   before: [pattern, rows], after: {pattern: [the pattern that came next, its rows]} }
    var roll = null;
    function rollReset() {
      roll = null; tune = null; sheet = null; sheets = {};
      Array.prototype.slice.call(scoreSheet.querySelectorAll(".mu-score-svg, .mu-score-clefs")).forEach(function (e) { e.parentNode.removeChild(e); });
    }
    // a cell's note: its pitch (1 is C-0), "off" for a key off, cut or fade, or null; and its instrument's colour, or -1
    function noteOf(cell) {
      var n = cell.charAt(0), acc = cell.charAt(1), oct = cell.charAt(2);
      if (n === "=" || n === "^" || n === "~") return { pitch: "off" };
      if (!(n in NAMES) || !(oct >= "0" && oct <= "9")) return null;
      var ins = cell.slice(4, 6), num = parseInt(ins, 16);   // both renderers write it in hex
      return { pitch: NAMES[n] + (acc === "#" ? 1 : 0) + 12 * (+oct) + 1, colour: num > 0 ? (num - 1) % ROLL.length : -1, ins: num > 0 ? num : 0 };
    }
    function rollPattern(feed, pattern, rows) {
      var k = String(pattern);
      if (roll.patterns[k]) return roll.patterns[k];
      var notes = [], open = {}, last = roll.instrument, lastText = roll.insText;
      for (var r = 0; r < rows; r++) {
        var cells = feed.rowAt(pattern, r).split("|");
        for (var c = 0; c < cells.length; c++) {
          var n = noteOf(cells[c] || "");
          if (!n) continue;
          if (open[c]) { open[c][1] = r; open[c] = null; }
          if (n.pitch === "off") continue;
          if (n.colour >= 0) { last[c] = n.colour; lastText[c] = cells[c].slice(4, 6); }
          // [start row, end row, pitch, colour, the instrument as the cell writes it ("" for none)]
          var note = [r, rows, n.pitch, last[c] >= 0 ? last[c] : c % ROLL.length, lastText[c] || ""];
          notes.push(note);
          open[c] = note;
          if (!roll.exact) { roll.lo = roll.lo ? Math.min(roll.lo, n.pitch) : n.pitch; roll.hi = Math.max(roll.hi || 0, n.pitch); }
        }
      }
      return (roll.patterns[k] = { pattern: pattern, rows: rows, notes: notes });
    }
    function drawPiano() {
      if (piano.hidden) return;
      var feed = trackerFeed();
      if (!feed) return;
      var mark = feed.mark;
      if (!roll || roll.id !== feed.id) {
        roll = { id: feed.id, patterns: {}, instrument: {}, insText: {}, lo: 0, hi: 0, exact: false, before: null, after: {}, at: null, shown: "" };
        var range = !follow && source && source.range ? source.range() : null;
        if (range) { roll.lo = range[0]; roll.hi = range[1]; roll.exact = true; }
      }
      var w = piano.clientWidth, hgt = piano.clientHeight, dpr = (window.ByopopSkin && ByopopSkin.ratio ? ByopopSkin.ratio() : window.devicePixelRatio || 1);
      var style = getComputedStyle(piano), g = piano.getContext("2d");
      g.font = "10px " + style.fontFamily;
      // the pitch names in a margin at the left; in the rest a row as wide as lets about a pattern (ROLL_ROWS rows) fill it (the
      // user, 2026-11-11: "way too zoomed out"), 4 pixels at least; the playhead a third of the way in
      var left = Math.ceil(g.measureText("C#9").width) + 6, col = Math.max(4, Math.floor((w - left) / ROLL_ROWS));
      var head = left + Math.floor((w - left) / 3 / col) * col;
      // the patterns on either side of the one heard: the orders before and after it where the decoder knows the order list (the
      // player's own tunes), else, for a game's module, the last one heard before it, and after it the patterns that followed it
      // the last time the tune went on from it (a game's tune comes round again; until then the display past its end is empty)
      tuneScan(feed);
      tunePattern(feed, mark[1], mark[3]);
      var now = rollPattern(feed, mark[1], mark[3]), parts = [[now, 0]];
      if (!follow && source && source.orderPattern) {
        // the orders before and after, as many as the display holds
        for (var o = mark[5] - 1, shift = 0; head + (shift - mark[2]) * col > left; o--) {
          var b = source.orderPattern(o);
          if (b === "skip") continue;
          if (!b || !b[1]) break;
          var bp = rollPattern(feed, b[0], b[1]);
          shift -= bp.rows;
          parts.push([bp, shift]);
        }
        for (var o2 = mark[5] + 1, shift2 = now.rows; head + (shift2 - mark[2]) * col < w; o2++) {
          var a = source.orderPattern(o2);
          if (a === "skip") continue;
          if (!a || !a[1]) break;
          var ap = rollPattern(feed, a[0], a[1]);
          parts.push([ap, shift2]);
          shift2 += ap.rows;
        }
      } else {
        if (roll.at && roll.at[0] !== mark[1]) { roll.before = roll.at; roll.after[roll.at[0]] = [mark[1], mark[3]]; }
        if (roll.before) { var bp2 = rollPattern(feed, roll.before[0], roll.before[1]); parts.push([bp2, -bp2.rows]); }
        for (var nx = roll.after[mark[1]], shift3 = now.rows, more = 0; nx && head + (shift3 - mark[2]) * col < w && more < 16; nx = roll.after[nx[0]], more++) {
          var np = rollPattern(feed, nx[0], nx[1]);
          parts.push([np, shift3]);
          shift3 += np.rows;
        }
      }
      roll.at = [mark[1], mark[3]];
      if (!w || !hgt || !roll.hi) return;
      var key = [mark[2], roll.lo, roll.hi, w, hgt, tuneOf(feed).read, parts.map(function (p) { return p[0].pattern + "@" + p[1]; }).join(",")].join(":");
      if (key === roll.shown) return;
      roll.shown = key;
      if (piano.width !== Math.round(w * dpr) || piano.height !== Math.round(hgt * dpr)) { piano.width = Math.round(w * dpr); piano.height = Math.round(hgt * dpr); }
      g = piano.getContext("2d");
      g.setTransform(dpr, 0, 0, dpr, 0, 0);
      g.clearRect(0, 0, w, hgt);
      g.font = "10px " + style.fontFamily;
      g.textBaseline = "middle";
      // a semitone as high as the display allows, a pixel at least
      var span = roll.hi - roll.lo + 1, step = Math.max(1, Math.floor((hgt - 2) / span)), base = Math.floor((hgt + span * step) / 2);
      function y(pitch) { return base - (pitch - roll.lo + 1) * step; }
      // the pitch names, in the tracker's spelling: each C, and every note where a row is tall enough for its name
      g.fillStyle = style.color;
      g.globalAlpha = 0.7;
      g.textAlign = "right";
      for (var p = roll.lo; p <= roll.hi; p++) {
        var pc = (p - 1) % 12;
        if (pc && step < 11) continue;
        g.fillText(PITCH_NAMES[pc] + Math.floor((p - 1) / 12), left - 3, Math.max(6, Math.min(hgt - 6, y(p) + step / 2)));
      }
      g.globalAlpha = 1;
      g.textAlign = "left";
      g.save();
      g.beginPath();
      g.rect(left, 0, w - left, hgt);
      g.clip();
      // the black keys' rows a shade lighter, where a row is tall enough to tell; the octaves' C rows, faintly
      if (step >= 4) {
        g.fillStyle = "#101010";
        for (p = roll.lo; p <= roll.hi; p++) if (BLACK_KEYS.indexOf((p - 1) % 12) >= 0) g.fillRect(left, y(p), w - left, step);
      }
      g.fillStyle = "#003000";
      for (p = roll.lo; p <= roll.hi; p++) if ((p - 1) % 12 === 0) g.fillRect(left, y(p) + step - 1, w - left, 1);
      // each pattern's beats and bars from its metre (meterOf, as in the score), its first row brighter; the bars run on from one
      // pattern into the next
      parts.forEach(function (part) {
        var m = meterOf(feed, part[0].pattern);
        for (var r = 0; r < part[0].rows; r += m.beat) {
          var x = head + (r + part[1] - mark[2]) * col;
          if (x < left || x >= w) continue;
          g.fillStyle = r === 0 ? "#707070" : r % m.bar === 0 ? "#383838" : "#1c1c1c";
          g.fillRect(x, 0, 1, hgt);
        }
      });
      // the notes, each with its instrument's number and name at its start, cut to the bar's length, where the bar has room for
      // the number; the instruments drawn, for the key where the bars have none
      var drawn = {}, names = feed.names ? feed.names() : {};
      function fit(text, room) {
        if (g.measureText(text).width <= room) return text;
        while (text.length > 2 && g.measureText(text).width > room) text = text.slice(0, -1);
        return text.replace(/ +$/, "");
      }
      parts.forEach(function (part) {
        part[0].notes.forEach(function (n) {
          var x0 = head + (n[0] + part[1] - mark[2]) * col, x1 = head + (n[1] + part[1] - mark[2]) * col - 1;
          if (x1 < left || x0 > w) return;
          g.fillStyle = ROLL[n[3]];
          g.fillRect(x0, y(n[2]), Math.max(1, x1 - x0), step);
          if (n[4]) drawn[n[4]] = n[3];
          if (n[4] && step >= 10 && x1 - x0 >= 15) {
            g.fillStyle = n[3] >= 7 ? "#ffffff" : "#000000";
            g.fillText(fit(names[n[4]] ? n[4] + " " + names[n[4]] : n[4], Math.min(x1, w) - x0 - 4), x0 + 2, y(n[2]) + step / 2);
          }
        });
      });
      // the key: a swatch, the number and the name of each instrument drawn, along the bottom right, when the bars are too thin
      // to carry them. It takes one line or two; the names are cut to fit (KEY_NAME characters at most, KEY_LEAST at least),
      // and where even that does not fit, the numbers alone
      var shown = Object.keys(drawn).sort();
      if (step < 10 && shown.length) {
        var room = w - left - 4, lines = null;
        function lay(cut) {
          var out = [[]], used = 0;
          shown.forEach(function (t) {
            var nm = cut > 0 && names[t] ? names[t].slice(0, cut).replace(/ +$/, "") : "", label = nm ? t + " " + nm : t;
            var lw = Math.ceil(g.measureText(label).width) + 16;
            if (used + lw > room && out[out.length - 1].length) { out.push([]); used = 0; }
            out[out.length - 1].push([t, label, lw]);
            used += lw;
          });
          return out;
        }
        for (var cut = KEY_NAME; cut >= KEY_LEAST && !lines; cut -= 2) { var tried = lay(cut); if (tried.length <= 2) lines = tried; }
        if (!lines) lines = lay(0);
        lines.forEach(function (line, k) {
          var ky = hgt - 13 * (lines.length - k), kx = w - line.reduce(function (a, it) { return a + it[2]; }, 0) - 2;
          g.fillStyle = "#000000";
          g.fillRect(kx - 2, ky, w - kx + 2, 13);
          line.forEach(function (it) {
            g.fillStyle = ROLL[drawn[it[0]]];
            g.fillRect(kx, ky + 3, 8, 8);
            g.fillStyle = style.color;
            g.fillText(it[1], kx + 10, ky + 7);
            kx += it[2];
          });
        });
      }
      // the heard pattern's metre as a value at the top left ("4/4 16TH FILE")
      var label = sigOf(meterOf(feed, mark[1])).label, lw = Math.ceil(g.measureText(label).width) + 4;
      g.fillStyle = "#000000";
      g.fillRect(left, 0, lw, 13);
      g.fillStyle = style.color;
      g.globalAlpha = 0.7;
      g.fillText(label, left + 2, 7);
      g.globalAlpha = 1;
      g.restore();
      // the playhead, over the heard row
      g.fillStyle = "rgba(255, 255, 255, 0.25)";
      g.fillRect(head, 0, col, hgt);
    }
    // The tune as a whole, for the bar lines and the score: each pattern's notes by instrument and its note onsets by channel
    // (read once), the staves the tune needs, and its metre. tune: { id, pats: {pattern: {rows, parts, onsets}}, read, staves,
    // staffAsk, staffSet, inferred, inferredAt, scanned }
    var tune = null;
    function tuneOf(feed) {
      if (!tune || tune.id !== feed.id) tune = { id: feed.id, pats: {}, read: 0, staffAsk: "", staffSet: null, inferred: null, inferredAt: -1, scanned: false,
        at: null, before: null, after: {} };
      return tune;
    }
    // a pattern of it: its notes by instrument ({ rows, parts: [{ ins, ch, notes: [[start, end, pitch]] }] }) and which rows
    // start a note in each channel (onsets[c][r] true)
    function tunePattern(feed, pattern, rows) {
      var t = tuneOf(feed), k = String(pattern);
      if (t.pats[k]) return t.pats[k];
      var parts = {}, open = {}, last = {}, onsets = [];
      for (var r = 0; r < rows; r++) {
        var cells = feed.rowAt(pattern, r).split("|");
        for (var c = 0; c < cells.length; c++) {
          var n = noteOf(cells[c] || "");
          if (!n) continue;
          if (open[c]) { open[c][1] = r; open[c] = null; }
          if (n.pitch === "off") continue;
          if (!onsets[c]) onsets[c] = [];
          onsets[c][r] = true;
          if (n.ins) last[c] = n.ins;
          var ins = last[c] || 0, key = ins ? "i" + ins : "c" + c;
          if (!parts[key]) parts[key] = { key: key, ins: ins, ch: c, notes: [] };
          var note = [r, rows, n.pitch];
          parts[key].notes.push(note);
          open[c] = note;
        }
      }
      t.read++;
      t.pats[k] = { rows: rows, parts: parts, onsets: onsets };
      return t.pats[k];
    }
    // the staves for some patterns (those on the display): a staff for each instrument they play, in instrument order, its clef
    // from all its notes in the tune read so far. { list, key }, kept until the patterns or what has been read change
    function staffSet(t, patterns) {
      var ask = patterns.join(",") + ":" + t.read;
      if (t.staffAsk === ask) return t.staffSet;
      var want = {}, all = {};
      patterns.forEach(function (pk) { var pt = t.pats[String(pk)]; if (pt) Object.keys(pt.parts).forEach(function (key) { want[key] = 1; }); });
      Object.keys(t.pats).forEach(function (pk) {
        var ps = t.pats[pk].parts;
        Object.keys(ps).forEach(function (key) {
          if (!want[key]) return;
          var a = all[key] || (all[key] = { key: key, ins: ps[key].ins, ch: ps[key].ch, pitches: [] });
          ps[key].notes.forEach(function (nn) { a.pitches.push(nn[2]); });
        });
      });
      var list = Object.keys(all).map(function (key) {
        var a = all[key], distinct = a.pitches.filter(function (v, i, arr) { return arr.indexOf(v) === i; });
        var drum = distinct.length === 1 && a.pitches.length >= 4;
        var avg = a.pitches.reduce(function (s2, v) { return s2 + v; }, 0) / a.pitches.length;
        return { key: key, ins: a.ins, ch: a.ch, drum: drum, clef: drum ? "percussion" : avg >= 56 ? "treble" : "bass",
                 colour: ROLL[a.ins > 0 ? (a.ins - 1) % ROLL.length : a.ch % ROLL.length] };
      });
      list.sort(function (a, b) { return (a.ins || 1000 + a.ch) - (b.ins || 1000 + b.ch); });
      t.staffAsk = ask;
      t.staffSet = { list: list, key: list.map(function (st) { return st.key + st.clef; }).join(",") };
      return t.staffSet;
    }
    // the player's own tunes: every pattern of the order list read at once, so the staves and the metre are the whole tune's
    function tuneScan(feed) {
      var t = tuneOf(feed);
      if (t.scanned || follow || !source || !source.orderPattern) return;
      t.scanned = true;
      for (var o = 0, seen = 0; seen < 512; o++, seen++) {
        var op = source.orderPattern(o);
        if (op === "skip") continue;
        if (!op) break;
        if (op[1]) tunePattern(feed, op[0], op[1]);
      }
    }
    // The metre (the user, 2026-11-11: "not every pattern has 4 bars. Some have 8 or 12"): a pattern's rows per beat and per
    // bar as the file stores them (an IT's row highlights, an MO3's; bm_meter, byopop_music_meter); else found from the notes:
    // the shortest bar length in rows (8, 12, 16, 24 or 32) at which the channels' note onsets come round again (channels that
    // repeat every 2, 3 or 4 rows, a hi-hat's, left out as telling nothing), doubled while a bar would last under 1.2 seconds
    // at the tune's speed and tempo, and the beat that bar divides into (2 to 4 beats) nearest 110 beats a minute; else 4 and
    // 16. { beat, bar, src: "FILE", "NOTES" or "DEFAULT" }
    var METER_DEFAULT = { beat: 4, bar: 16, src: "DEFAULT" };
    function meterOf(feed, pattern) {
      var stored = feed.meter ? feed.meter(pattern) : 0;
      if (stored > 0) return { beat: stored >> 16, bar: stored & 0xffff, src: "FILE" };
      var t = tuneOf(feed);
      if (t.inferredAt !== t.read) { t.inferred = inferMeter(t, feed.tempo ? feed.tempo() : null); t.inferredAt = t.read; }
      return t.inferred || METER_DEFAULT;
    }
    function inferMeter(t, tempo) {
      var LAGS = [2, 3, 4, 8, 12, 16, 24, 32], hit = {}, tot = {};
      LAGS.forEach(function (L) { hit[L] = 0; tot[L] = 0; });
      Object.keys(t.pats).forEach(function (pk) {
        var pt = t.pats[pk];
        pt.onsets.forEach(function (on) {
          if (!on) return;
          var count = 0;
          for (var r = 0; r < pt.rows; r++) if (on[r]) count++;
          if (count < 4) return;
          var own = {};
          LAGS.forEach(function (L) {
            var h = 0, n = 0;
            for (var r2 = 0; r2 + L < pt.rows; r2++) if (on[r2]) { n++; if (on[r2 + L]) h++; }
            own[L] = [h, n];
          });
          // a channel that repeats every 2, 3 or 4 rows tells nothing of the bar
          if ([2, 3, 4].some(function (L) { return own[L][1] >= 4 && own[L][0] >= 0.9 * own[L][1]; })) return;
          LAGS.forEach(function (L) { hit[L] += own[L][0]; tot[L] += own[L][1]; });
        });
      });
      var BARS = [8, 12, 16, 24, 32], score = {}, best = 0;
      BARS.forEach(function (L) { score[L] = tot[L] >= 16 ? hit[L] / tot[L] : 0; best = Math.max(best, score[L]); });
      if (best < 0.5) return null;
      var bar = 0;
      BARS.forEach(function (L) { if (!bar && score[L] >= 0.9 * best) bar = L; });
      var rowSec = tempo && tempo.speed > 0 && tempo.bpm > 0 ? tempo.speed * 2.5 / tempo.bpm : 0;
      while (rowSec && bar * rowSec < 1.2 && bar * 2 <= 32 && score[bar * 2] >= 0.9 * best) bar *= 2;
      var beat = 0, gap = 1e9;
      [4, 3, 2, 6, 8].forEach(function (d) {
        if (bar % d || bar / d < 2 || bar / d > 4) return;
        var miss = rowSec ? Math.abs(60 / (d * rowSec) - 110) : (d === 4 ? 0 : 1);
        if (miss < gap) { gap = miss; beat = d; }
      });
      return beat ? { beat: beat, bar: bar, src: "NOTES" } : null;
    }
    // A metre as notation: the rows in a whole note (a row a sixteenth at 4 or 6 rows a beat, an eighth at 2 or 3, a
    // thirty-second at 8 or 12; 3, 6 and 12 compound, a beat a dotted quarter), the time signature, and the label the display
    // shows ("4/4 16TH FILE")
    function sigOf(m) {
      var compound = m.beat % 3 === 0, whole = compound ? m.beat * 8 / 3 : m.beat * 4;
      if ([4, 8, 16, 32].indexOf(whole) < 0) { whole = 16; compound = false; }
      var beats = m.bar / m.beat, num, den;
      if (beats === Math.floor(beats)) { num = compound ? beats * 3 : beats; den = compound ? 8 : 4; }
      else { num = m.bar; den = whole; }
      var unit = { 4: "4TH", 8: "8TH", 16: "16TH", 32: "32ND" }[whole];
      return { whole: whole, num: num, den: den, label: num + "/" + den + " " + unit + " " + m.src };
    }
    // the note values a length in rows is written with, longest first, for that many rows in a whole note
    function noteValues(whole) {
      return [[1, "w"], [0.75, "hd"], [0.5, "h"], [0.375, "qd"], [0.25, "q"], [0.1875, "8d"], [0.125, "8"], [0.09375, "16d"], [0.0625, "16"], [0.03125, "32"]]
        .map(function (v) { return [v[0] * whole, v[1]]; }).filter(function (v) { return v[0] >= 1 && v[0] === Math.floor(v[0]); });
    }
    // the patterns on either side of the one heard, as the piano roll finds them: the order list's for the player's own
    // tunes, else the one heard before and those that followed it the last time the tune went on from it. [[pattern, rows,
    // offset from the heard pattern's first row in patterns: -1, 1, 2 ...]]
    function neighbours(feed, mark, before, after) {
      var t = tuneOf(feed), out = [];
      if (!follow && source && source.orderPattern) {
        for (var o = mark[5] - 1, k = 0; k < before && o >= 0; o--) { var b = source.orderPattern(o); if (b === "skip") continue; if (!b || !b[1]) break; out.push([b[0], b[1], -(++k)]); }
        for (var o2 = mark[5] + 1, k2 = 0; k2 < after; o2++) { var a = source.orderPattern(o2); if (a === "skip") continue; if (!a || !a[1]) break; out.push([a[0], a[1], ++k2]); }
        return out;
      }
      if (t.at && t.at[0] !== mark[1]) { t.before = t.at; t.after[t.at[0]] = [mark[1], mark[3]]; }
      t.at = [mark[1], mark[3]];
      if (t.before && before > 0) out.push([t.before[0], t.before[1], -1]);
      for (var nx = t.after[mark[1]], k3 = 0; nx && k3 < after; nx = t.after[nx[0]]) out.push([nx[0], nx[1], ++k3]);
      return out;
    }
    // The score (the user, 2026-11-09, sheet music; 2026-11-11: "sheet music should scroll instead of fitting to window
    // width"): the tune in staff notation, drawn by VexFlow 5.0.0 (web/third_party/vexflow-bravura.js, served as
    // vexflow/vexflow-bravura.js; MIT, with the Bravura and Academico fonts, SIL OFL), which the page fetches only when this
    // view is first chosen. A staff for each instrument that the patterns on the display play (the heard one and those either
    // side of it; a whole tune can have a hundred), in its piano-roll colour, its clef from all its notes in the tune (treble when they lie at G below middle C or higher on
    // average, else bass; an instrument that plays one pitch only, four times or more, a drum on a percussion staff with x
    // noteheads). Each pattern is drawn once, at a fixed scale (the display's height over the staves, 0.45 to 0.8), each row
    // SCORE_ROW wide before scaling, its bars from its metre, and kept (SCORE_KEEP at most); the patterns run on side by side and
    // slide past a playhead a third of the way in, the clefs and time signature standing still at the left. Within an
    // instrument a note lasts until its channel's next note, key off, cut or fade, or until that instrument's next note in any
    // channel; notes that start together are a chord; a length that no single note value gives is tied notes, as is a note
    // held over a bar line. Sharps are written as the bar needs them (VexFlow's own rule for C major). Effects are not drawn. A
    // tracker's C-5 is written as middle C. The staves scroll up and down where they do not fit.
    var vf = null, vfLoading = false, sheet = null, sheets = {}, sheetSerial = 0, SCORE_KEEP = 32, SCORE_ROW = 26, SCORE_LINE = 84, SCORE_TOP = 24;
    var STEP_NAMES = ["c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b"];
    function loadScore() {
      if (vf || vfLoading) return;
      vfLoading = true;
      var el = document.createElement("script");
      el.src = "vexflow/vexflow-bravura.js";
      el.onload = function () {
        var V = window.VexFlow;
        // its fonts are in the file; drawn only once the music font is ready, or its glyphs would be missing
        var ready = document.fonts && document.fonts.load ? document.fonts.load("30px Bravura").catch(function () {}) : Promise.resolve();
        ready.then(function () { vf = V || null; vfLoading = false; sheet = null; tick(); });
      };
      el.onerror = function () { vfLoading = false; };
      document.head.appendChild(el);
    }
    function vfKey(pitch, drum) {
      if (drum) return "c/5/x2";
      var p = pitch - 1, oct = Math.floor(p / 12) - 1;
      return STEP_NAMES[p % 12] + "/" + oct;
    }
    // one instrument's line in a pattern: its notes cut at the instrument's next note, then split at every start, end and bar
    // line into note values; returns [{ pitches, start, rows, dur, tieFrom }] in order
    function scoreEvents(part, rows, bar, values) {
      var notes = part ? part.notes.map(function (n) { return n.slice(); }) : [];
      var starts = notes.map(function (n) { return n[0]; }).filter(function (v, i, a) { return a.indexOf(v) === i; }).sort(function (a, b) { return a - b; });
      notes.forEach(function (n) {
        for (var i = 0; i < starts.length; i++) if (starts[i] > n[0]) { n[1] = Math.min(n[1], starts[i]); break; }
      });
      var cuts = { 0: 1 };
      cuts[rows] = 1;
      for (var b = bar; b < rows; b += bar) cuts[b] = 1;
      notes.forEach(function (n) { cuts[n[0]] = 1; cuts[n[1]] = 1; });
      var at = Object.keys(cuts).map(Number).filter(function (v) { return v >= 0 && v <= rows; }).sort(function (a, b) { return a - b; });
      var out = [];
      for (var i = 0; i + 1 < at.length; i++) {
        var a = at[i], z = at[i + 1];
        var sounding = notes.filter(function (n) { return n[0] <= a && a < n[1]; });
        var pitches = sounding.map(function (n) { return n[2]; }).filter(function (v, k, arr) { return arr.indexOf(v) === k; }).sort(function (x, y) { return x - y; });
        var held = sounding.length > 0 && sounding.every(function (n) { return n[0] < a; });
        // the length in note values, longest first, each a tie on from the one before, none across a bar line
        for (var r = a, first = true; r < z;) {
          var len = z - r, d = null;
          for (var k = 0; k < values.length; k++) if (values[k][0] <= len && (r % bar) + values[k][0] <= bar) { d = values[k]; break; }
          if (!d) d = values[values.length - 1];
          out.push({ pitches: pitches, start: r, rows: d[0], dur: d[1], tieFrom: pitches.length > 0 && (!first || held) });
          r += d[0]; first = false;
        }
      }
      return out;
    }
    // one pattern drawn: { host (absolutely placed), width, height, rowX: each row's x and the end's, in pixels }
    function renderSheet(t, staves, pattern, meter, scale) {
      var V = vf, pat = t.pats[String(pattern)], rows = pat.rows, sig = sigOf(meter), values = noteValues(sig.whole), bar = meter.bar;
      var host = make("div", "mu-score-svg");
      var bars = Math.ceil(rows / bar), barW = [], total = 0;
      for (var b = 0; b < bars; b++) { barW.push((Math.min(rows, (b + 1) * bar) - b * bar) * SCORE_ROW + 14); total += barW[b]; }
      var height = SCORE_TOP + staves.length * SCORE_LINE + 16;
      var renderer = new V.Renderer(host, V.Renderer.Backends.SVG);
      renderer.resize(Math.ceil(total * scale), Math.ceil(height * scale));
      var ctx = renderer.getContext();
      ctx.scale(scale, scale);
      var lines = staves.map(function (st, si) {
        var restKey = st.clef === "bass" ? "d/3" : "b/4", events = scoreEvents(pat.parts[st.key], rows, bar, values), row = [];
        for (var b2 = 0, x = 0; b2 < bars; x += barW[b2], b2++) row.push({ stave: new V.Stave(x, SCORE_TOP + si * SCORE_LINE, barW[b2]), notes: [], events: [] });
        var made = [];
        events.forEach(function (ev) {
          var cell = row[Math.floor(ev.start / bar)];
          var keys = ev.pitches.length ? ev.pitches.map(function (p) { return vfKey(p, st.drum); }) : [restKey];
          var n = new V.StaveNote({ keys: keys, duration: ev.dur + (ev.pitches.length ? "" : "r"), clef: st.clef, autoStem: true });
          if (/d$/.test(ev.dur)) V.Dot.buildAndAttach([n], { all: true });
          n.setStyle({ fillStyle: st.colour, strokeStyle: st.colour });
          n.setLedgerLineStyle({ strokeStyle: st.colour });
          cell.notes.push(n); cell.events.push(ev);
          made.push([n, ev]);
        });
        var ties = [];
        for (var i = 1; i < made.length; i++) {
          var cur = made[i][1], prev = made[i - 1][1];
          if (!cur.tieFrom || !prev.pitches.length) continue;
          var fi = [], li = [];
          cur.pitches.forEach(function (p, k) { var j = prev.pitches.indexOf(p); if (j >= 0) { fi.push(j); li.push(k); } });
          if (fi.length) ties.push(new V.StaveTie({ firstNote: made[i - 1][0], lastNote: made[i][0], firstIndexes: fi, lastIndexes: li }));
        }
        return { row: row, colour: st.colour, ties: ties, drum: st.drum };
      });
      // each bar's notes lined up across the staves
      for (var b3 = 0; b3 < bars; b3++) {
        var voices = [], fmt = new V.Formatter();
        lines.forEach(function (ln) {
          var cell = ln.row[b3], voice = new V.Voice({ numBeats: sig.num, beatValue: sig.den });
          voice.setMode(V.Voice.Mode.SOFT);
          voice.addTickables(cell.notes);
          if (!ln.drum) V.Accidental.applyAccidentals([voice], "C");
          cell.voice = voice;
          voices.push(voice);
          fmt.joinVoices([voice]);
        });
        var startX = lines.length ? lines[0].row[b3].stave.getNoteStartX() : 0;
        fmt.format(voices, Math.max(10, lines.length ? lines[0].row[b3].stave.getX() + barW[b3] - startX - 10 : 10));
      }
      var points = [[0, 0], [rows, total]];
      lines.forEach(function (ln) {
        ln.row.forEach(function (cell, bi) {
          cell.stave.setContext(ctx).draw();
          var beams = V.Beam.generateBeams(cell.notes, { beamRests: false, maintainStemDirections: true });   // the rests break a beam
          cell.voice.draw(ctx, cell.stave);
          beams.forEach(function (bm) { bm.setStyle({ fillStyle: ln.colour, strokeStyle: ln.colour }); bm.setContext(ctx).draw(); });
          cell.notes.forEach(function (n, k) { points.push([cell.events[k].start, n.getAbsoluteX()]); });
          if (bi) points.push([bi * bar, cell.stave.getX()]);
        });
        ln.ties.forEach(function (tie) { tie.setContext(ctx).draw(); });
      });
      // each row's x: the notes' and bar lines' known places, the rows between them in proportion
      points.sort(function (a, b) { return a[0] - b[0] || a[1] - b[1]; });
      var known = [];
      points.forEach(function (p) { if (!known.length || known[known.length - 1][0] !== p[0]) known.push(p); });
      var rowX = [];
      for (var r = 0, j = 0; r <= rows; r++) {
        while (j + 1 < known.length && known[j + 1][0] <= r) j++;
        var p0 = known[j], p1 = known[Math.min(j + 1, known.length - 1)];
        var x = p1[0] > p0[0] ? p0[1] + (p1[1] - p0[1]) * (r - p0[0]) / (p1[0] - p0[0]) : p0[1];
        rowX.push(Math.round(x * scale));
      }
      rowX[rows] = Math.round(total * scale);
      var svg = host.querySelector("svg");
      // the staff lines, bar lines and rests take the drawing's own colour (black, unseen on the display): grey
      if (svg) { svg.setAttribute("fill", "#808080"); svg.setAttribute("stroke", "#808080"); }
      return { host: host, width: Math.round(total * scale), height: Math.ceil(height * scale), rowX: rowX };
    }
    // the clefs and time signature at the left, standing still while the patterns slide past
    function renderClefs(staves, meter, scale) {
      var V = vf, host = make("div", "mu-score-clefs"), sig = sigOf(meter), width = 80;
      var height = SCORE_TOP + staves.length * SCORE_LINE + 16;
      var renderer = new V.Renderer(host, V.Renderer.Backends.SVG);
      renderer.resize(Math.ceil(width * scale), Math.ceil(height * scale));
      var ctx = renderer.getContext();
      ctx.scale(scale, scale);
      staves.forEach(function (st, si) {
        var stave = new V.Stave(0, SCORE_TOP + si * SCORE_LINE, width);
        stave.addClef(st.clef).addTimeSignature(sig.num + "/" + sig.den);
        stave.setContext(ctx).draw();
      });
      var svg = host.querySelector("svg");
      if (svg) { svg.setAttribute("fill", "#808080"); svg.setAttribute("stroke", "#808080"); }
      return { host: host, width: Math.ceil(width * scale) };
    }
    function drawScore() {
      if (score.hidden) return;
      if (!vf) { loadScore(); return; }
      var feed = trackerFeed();
      if (!feed) return;
      var mark = feed.mark, w = score.clientWidth, hgt = score.clientHeight;
      if (!w || !hgt) return;
      var t = tuneOf(feed);
      tuneScan(feed);
      tunePattern(feed, mark[1], mark[3]);
      var near = neighbours(feed, mark, 2, 4);
      near.forEach(function (n) { tunePattern(feed, n[0], n[1]); });
      // the staves of the patterns the display can show: the heard one and those either side of it (a pattern is wider than
      // two thirds of the display at the least scale); the sheets are drawn again for a new set, each kept by its set
      var set = staffSet(t, [mark[1]].concat(near.filter(function (n) { return Math.abs(n[2]) === 1; }).map(function (n) { return n[0]; })));
      var staves = set.list;
      if (!staves.length) return;
      var scale = Math.max(0.45, Math.min(0.8, (hgt - 2) / (SCORE_TOP + staves.length * SCORE_LINE + 16)));
      scale = Math.round(scale * 20) / 20;
      var meter = meterOf(feed, mark[1]), sig = sigOf(meter);
      var base = [t.id, set.key, scale].join(":");
      // a sheet for each pattern, drawn when first needed: the heard one at once, then at most one more a frame
      var fresh = 0;
      function sheetFor(pattern, rows) {
        var m = meterOf(feed, pattern), key = base + ":" + pattern + ":" + rows + ":" + m.beat + "/" + m.bar;
        if (sheets[key]) return sheets[key];
        if (fresh++ > 0) return null;
        var drawn;
        try { drawn = renderSheet(t, staves, pattern, m, scale); } catch (e) { drawn = null; }
        var kept = Object.keys(sheets);
        if (kept.length >= SCORE_KEEP) delete sheets[kept[0]];
        if (drawn) { drawn.n = ++sheetSerial; drawn.host.style.position = "absolute"; drawn.host.style.top = "0"; sheets[key] = drawn; }
        return drawn;
      }
      var cur = sheetFor(mark[1], mark[3]);
      if (!cur) return;
      var clefKey = base + ":" + sig.num + "/" + sig.den;
      if (!sheet || sheet.clefKey !== clefKey) {
        if (sheet && sheet.clefs && sheet.clefs.host.parentNode) sheet.clefs.host.parentNode.removeChild(sheet.clefs.host);
        var clefs = null;
        try { clefs = renderClefs(staves, meter, scale); } catch (e) { clefs = null; }
        sheet = { clefKey: clefKey, clefs: clefs, placed: "" };
        if (clefs) scoreSheet.appendChild(clefs.host);
      }
      var cw = sheet.clefs ? sheet.clefs.width : 0, head = cw + Math.floor((w - cw) / 3);
      var curLeft = head - cur.rowX[mark[2]];
      // the sheets in place: the heard one with its playhead row under the head, the ones before and after it beside it
      var placed = [[cur, curLeft]], left = curLeft, right = curLeft + cur.width;
      near.forEach(function (n) {
        if (n[2] < 0 ? left <= cw : right >= w) return;
        var s = sheetFor(n[0], n[1]);
        if (!s) { if (n[2] < 0) left = -1e9; else right = 1e9; return; }
        if (n[2] < 0) { left -= s.width; placed.push([s, left]); }
        else { placed.push([s, right]); right += s.width; }
      });
      var stamp = placed.map(function (p) { return p[0].n + "@" + p[1]; }).join(",") + ":" + sig.label;
      if (stamp === sheet.placed) return;
      sheet.placed = stamp;
      var height = 0, onPage = placed.map(function (p) { return p[0]; });
      Object.keys(sheets).forEach(function (k) {
        var s = sheets[k], i = onPage.indexOf(s);
        if (i < 0) { if (s.host.parentNode) s.host.parentNode.removeChild(s.host); return; }
        if (s.host.parentNode !== scoreSheet) scoreSheet.insertBefore(s.host, scoreSheet.firstChild);
        s.host.style.left = placed[i][1] + "px";
        height = Math.max(height, s.height);
      });
      scoreSheet.style.height = height + "px";
      if (sheet.clefs) sheet.clefs.host.style.height = height + "px";
      scoreHead.hidden = false;
      scoreHead.style.left = (head - 1) + "px";
      scoreHead.style.height = height + "px";
      scoreMeter.textContent = sig.label;
    }
    // The spectrum: BARS bars over 40 Hz to 16 kHz spaced as the ear hears them, each with a peak that falls back. Steadied (the
    // user, 2026-11-10: "a little too jittery. Something to do with dropoff speed, maybe?"): the analyser averages over more of
    // its past frames (SMOOTHING), a bar takes the mean of its bins with their highest (the lowest bars cover a bin or two, so
    // their highest alone jumps), rises over a couple of frames, and falls by time (FALL a second, the whole height in about a
    // second whatever the frame rate) instead of by frame; a peak holds PEAK_HOLD seconds, then falls at PEAK_FALL
    var SMOOTHING = 0.8, RISE = 0.6, FALL = 1.1, PEAK_HOLD = 0.5, PEAK_FALL = 0.5;
    var bins = null, peaks = [], levels = [], held = [], lastSpectrum = 0;
    function drawSpectrum() {
      // the followed game's own output where it is tapped, else the player's own analyser
      var tap = follow && follow.tap, an = tap ? tap.an : analyser, live = tap ? true : state === "playing", rate = tap ? tap.rate : ctx && ctx.sampleRate;
      if (spectrum.hidden || !an) return;
      var w = spectrum.clientWidth, hgt = spectrum.clientHeight, dpr = (window.ByopopSkin && ByopopSkin.ratio ? ByopopSkin.ratio() : window.devicePixelRatio || 1);
      if (!w || !hgt) return;
      if (spectrum.width !== Math.round(w * dpr) || spectrum.height !== Math.round(hgt * dpr)) { spectrum.width = Math.round(w * dpr); spectrum.height = Math.round(hgt * dpr); }
      if (!bins || bins.length !== an.frequencyBinCount) bins = new Uint8Array(an.frequencyBinCount);
      if (live) an.getByteFrequencyData(bins); else bins.fill(0);
      var g = spectrum.getContext("2d"), cs = getComputedStyle(spectrum);
      var barColour = cs.getPropertyValue("--mu-bar").trim() || cs.color, peakColour = cs.getPropertyValue("--mu-peak").trim() || barColour;
      g.setTransform(dpr, 0, 0, dpr, 0, 0);
      g.clearRect(0, 0, w, hgt);
      var now = performance.now(), dt = lastSpectrum ? Math.min(0.1, (now - lastSpectrum) / 1000) : 0;
      lastSpectrum = now;
      var hz = rate / an.fftSize, gap = 2, bw = (w - gap * (BARS + 1)) / BARS;
      for (var i = 0; i < BARS; i++) {
        var lo = 40 * Math.pow(400, i / BARS), hi = 40 * Math.pow(400, (i + 1) / BARS), v = 0, sum = 0, n = 0;
        for (var b = Math.floor(lo / hz); b <= Math.min(bins.length - 1, Math.ceil(hi / hz)); b++) { v = Math.max(v, bins[b]); sum += bins[b]; n++; }
        var level = n ? (v + sum / n) / 2 / 255 : 0, was = levels[i] || 0;
        levels[i] = level > was ? was + (level - was) * RISE : Math.max(level, was - FALL * dt);
        if (levels[i] >= (peaks[i] || 0)) { peaks[i] = levels[i]; held[i] = PEAK_HOLD; }
        else if ((held[i] -= dt) <= 0) peaks[i] = Math.max(levels[i], peaks[i] - PEAK_FALL * dt);
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
        // a game is followed only while the player plays nothing of its own: its own tune is the one heard then (the desktop
        // hushes the games, web/deskapps.js), so the views and the seek bar are its (the user, 2026-11-12: "If a game is open while
        // I use the music player, jumping around in the song does not actually skip the audio")
        var seen = onScreen(), own = state === "playing" || state === "loading", front = seen && !own ? frontGame() : null;
        if (seen) { followGame(front); drawTracker(); drawPiano(); drawScore(); drawSpectrum(); } else unfollow();
        var settling = levels.some(function (l, i) { return l > 0.001 || peaks[i] > 0.001; });
        if (state === "playing" || front || (!spectrum.hidden && settling)) { ticking = true; requestAnimationFrame(frame); }
        // game windows open but none in front (all minimized, say): a look twice a second, so one restored is followed again
        else if (seen && gameWindows()) setTimeout(tick, 500);
      });
    }
    function gameWindows() { try { return window.ByopopApps && ByopopApps.count ? ByopopApps.count() : 0; } catch (e) { return 0; } }

    // ---------- following a game (the user, 2026-11-09: "make the tracker/spectral analyzer sync with what's playing in the active
    // game"; docs/WEB_PLAYER.md, "Following a game") ----------
    // While a game window is in front (web/deskapps.js frontFrame) and its game plays music, the views show that music: the tracker
    // its module where the game plays one (the game's mixer through byopop_music_*, platform/Audio.cpp), the spectrum the game's
    // own output, from an analyser on its sound's last node. Otherwise they show the player's own tune, as before. The player's
    // own playback goes on as it was; following changes nothing the game plays or what is heard (a silent game stays silent: its
    // spectrum is then flat), except where the player moves it (the seek bar, Previous and Next: matchTrack below). While the
    // player plays or loads a tune of its own, nothing is followed: that tune is the one heard.
    var follow = null, followSerial = 0;
    function frontGame() { try { return window.ByopopApps && ByopopApps.frontFrame ? ByopopApps.frontFrame() : null; } catch (e) { return null; } }
    // the game's module, once its runtime is up and if its build has the calls
    function gameModule(win) { try { var m = win && win.Module; return m && typeof m._byopop_music_song === "function" && typeof m.ccall === "function" ? m : null; } catch (e) { return null; } }
    function followGame(front) {
      var m = front ? gameModule(front.win) : null, song = 0, view = null;
      if (m) try {
        song = m._byopop_music_song();
        if (song) view = JSON.parse(m.ccall("byopop_music_view", "string", ["number"], [song]) || "null");
      } catch (e) { view = null; }
      if (!view) { unfollow(); return; }
      if (!follow || follow.win !== front.win || follow.song !== song) {
        unfollow();
        follow = { win: front.win, M: m, song: song, view: view, title: front.title, rows: {}, used: null, tap: null, serial: ++followSerial,
                   hushed: function () { try { return !!(window.ByopopApps && ByopopApps.hushed && ByopopApps.hushed(front.win)); } catch (e) { return false; } } };
        firstChannel = 0;
        applyVis();
      }
      var wasKind = follow.view.kind;
      follow.view = view;
      if (view.kind !== wasKind) applyVis();
      follow.track = matchTrack();
      if (follow.track >= 0) timesOf(follow.track);
      showTrack();
      tapGame();
      showFollowed();
    }
    // the analyser on the game's output: one more connection from its last node, which goes on to the speakers as before
    function tapGame() {
      try {
        var out = follow.M.byopopAudio, gctx = follow.M.SDL2 && follow.M.SDL2.audioContext;
        if (follow.tap && out && follow.tap.node === out.node) return;
        untap();
        if (!out || !out.node || !gctx || gctx.state === "closed") return;
        var an = gctx.createAnalyser();
        an.fftSize = 2048; an.smoothingTimeConstant = SMOOTHING; an.minDecibels = -85; an.maxDecibels = -15;
        out.node.connect(an);
        follow.tap = { an: an, node: out.node, rate: gctx.sampleRate };
      } catch (e) {}
    }
    function untap() {
      if (!follow || !follow.tap) return;
      try { follow.tap.node.disconnect(follow.tap.an); } catch (e) {}
      follow.tap = null;
    }
    function unfollow() {
      if (!follow) return;
      untap();
      follow = null;
      firstChannel = 0; shownRow = "";
      rollReset();
      setState(state);   // the display, the list and the seek bar the player's own again
      showFollowed();
    }
    function followedRow(pattern, row) {
      var k = pattern + ":" + row;
      if (!(k in follow.rows)) {
        try { follow.rows[k] = follow.M.ccall("byopop_music_row", "string", ["number", "number", "number"], [follow.song, pattern, row]) || ""; }
        catch (e) { follow.rows[k] = ""; }
      }
      return follow.rows[k];
    }
    function followedNames() {
      if (follow.names == null) {
        // a game built before byopop_music_names has no such call: no names
        try { follow.names = parseNames(follow.M.ccall("byopop_music_names", "string", ["number"], [follow.song]) || ""); }
        catch (e) { follow.names = {}; }
      }
      return follow.names;
    }
    // a channel of the game's song muted or brought back (byopop_music_mute); false where its build has no such call
    function followedMute(c, on) {
      try { return follow.M.ccall("byopop_music_mute", "number", ["number", "number", "number"], [follow.song, c, on ? 1 : 0]) === 1; }
      catch (e) { return false; }
    }
    // a pattern's stored rows per beat and per bar (byopop_music_meter, beat << 16 | bar), 0 where it stores none or the game's
    // build has no such call
    function followedMeter(pattern) {
      if (!follow.meters) follow.meters = {};
      if (!(pattern in follow.meters)) {
        try { follow.meters[pattern] = follow.M.ccall("byopop_music_meter", "number", ["number", "number"], [follow.song, pattern]) || 0; }
        catch (e) { follow.meters[pattern] = 0; }
      }
      return follow.meters[pattern];
    }
    function followedUsed() {
      // the matched tune's own channels once its orders' rows are known (timesOf), else the song's
      var track = follow.track, ot = track >= 0 ? timesOf(track) : null;
      if (ot && ot.rows && follow.usedFor !== track) {
        follow.usedFor = track;
        follow.tuneUsed = tuneUsed(tunes[track], followedRow, function (o) { return ot.rows[o] || 0; });
      }
      if (follow.usedFor === track && follow.tuneUsed && follow.tuneUsed.length) return follow.tuneUsed;
      if (!follow.used) {
        try { follow.used = (follow.M.ccall("byopop_music_used", "string", ["number"], [follow.song]) || "").split(",").filter(Boolean).map(Number); }
        catch (e) { follow.used = []; }
      }
      return follow.used;
    }
    // The followed game's own track (the user's notes, 2026-11-10: "The tracker sync should show what track is actually playing and
    // its progress", "If possible, make the controls change what plays in game"). The list's tune the game plays: one of that
    // game's module tunes whose file has the game's song's orders, each playing the same pattern, and the one whose orders hold
    // the order heard (a range's, else the subsong that starts there or nearest before it). While there is one, the display
    // shows it as the player's own would be (its name, the game's icon in place of the state, its time and length) and the list
    // marks it; its time is its order's start, from a copy of the module read once a tune, and the rows since at the speed and
    // tempo playing. The seek bar moves the game's song (byopop_music_seek), and Previous and Next take it to the tune before or
    // after in the same file, from its first order (Previous: the tune's own start after 3 seconds); the game may change it
    // again as it plays on, as its own code decides. Play, Pause and Stop stay the player's own.
    function matchTrack() {
      var v = follow.view, best = -1, bestEntry = -1;
      if (v.kind !== "module" || !(v.orders > 0)) return -1;
      tunes.forEach(function (t, i) {
        if (t.kind !== "module" || t.segs || t.game.title !== follow.title || !t.seq || t.seq.length !== v.orders || t.seq[v.order] !== v.pattern) return;
        if (t.range) { if (v.order >= t.range[0] && v.order <= t.range[1]) { best = i; bestEntry = 1e9; } return; }
        if (t.entry <= v.order && t.entry > bestEntry) { best = i; bestEntry = t.entry; }
      });
      return best;
    }
    // each order's start in seconds into tune i and its pattern's rows ({ times: {order: s}, rows: {order: n} }), from a copy of
    // its module read once a tune; null until read
    var orderTimes = {};
    function timesOf(i) {
      var t = tunes[i], key = t.id + "@" + t.keys[0];
      if (orderTimes[key] !== undefined) return orderTimes[key];
      orderTimes[key] = null;
      (M ? Promise.resolve() : decoder().then(function (d) { M = d; })).then(function () {
        return openExisting(t.game.persist);
      }).then(function (db) {
        if (!db) return;
        return readFile(db, t.keys[0]).then(function (bytes) {
          db.close();
          var m = bytes ? loadModule(M, bytes) : 0;
          if (!m) return;
          var one = M._malloc(16), times = {}, rows = {}, last = t.range ? t.range[1] : t.seq.length - 1;
          try {
            if (!t.range) M._bm_select(m, t.sub);
            // a subsong's orders: from its first to the next subsong's first (or the end)
            if (!t.range) tunes.forEach(function (u) { if (u.keys[0] === t.keys[0] && !u.range && u.entry > t.entry) last = Math.min(last, u.entry - 1); });
            var base = null, before = {}, counted = 0, moved = false;
            for (var o = t.entry; o <= last; o++) {
              if (t.seq[o] < 0) continue;
              M._bm_set_order(m, o);
              M._bm_render(m, 48000, 1, one);
              var at = M._bm_position(m);
              if (base === null) base = at;
              times[o] = Math.max(0, at - base);
              if (times[o] > 0.05) moved = true;
              rows[o] = M._bm_pattern_rows(m, t.seq[o]);
              before[o] = counted;
              counted += rows[o];
            }
          } finally { M._free(one); M._bm_free(m); }
          // libopenmpt answers each order's start; libxmp-lite's clock does not follow a jump (finding the starts by seeking
          // took 4 seconds for a 4-minute tune), so there the rows since the tune's first order count, at the speed and tempo
          // playing: exact while the tune keeps one tempo
          orderTimes[key] = { times: moved ? times : null, before: before, rows: rows };
        });
      }).catch(function () {});
      return null;
    }
    function rowSeconds() { var v = follow.view; return v.speed > 0 && v.bpm > 0 ? v.speed * 2.5 / v.bpm : 0.125; }
    function orderStart(ot, o) { return ot.times ? ot.times[o] : ot.before[o] * rowSeconds(); }
    // the followed track's time and length, or null where there is none (no tune matched, or its orders not read yet)
    function followedClock() {
      if (!follow || !(follow.track >= 0)) return null;
      var t = tunes[follow.track], ot = t && timesOf(follow.track), v = follow.view;
      if (!ot || !(v.order in ot.rows)) return null;
      return { at: Math.max(0, Math.min(t.seconds, orderStart(ot, v.order) + v.row * rowSeconds())), total: t.seconds };
    }
    // the game's song moved to s seconds into its track: the last order that starts by then, and the rows into it
    function seekGame(s) {
      var ot = timesOf(follow.track), at = -1;
      if (!ot) return;
      Object.keys(ot.rows).forEach(function (o) { if (orderStart(ot, o) <= s && (at < 0 || orderStart(ot, o) >= orderStart(ot, at))) at = +o; });
      if (at < 0) return;
      var row = Math.max(0, Math.min((ot.rows[at] || 64) - 1, Math.floor((s - orderStart(ot, at)) / rowSeconds())));
      gameSeek(at, row);
    }
    function gameSeek(order, row) {
      try { follow.M.ccall("byopop_music_seek", "number", ["number", "number", "number"], [follow.song, order, row]); } catch (e) {}
      rollReset(); shownRow = "";
      tick();
    }
    // Previous and Next for the game: the tune before or after in the same file, from its first order
    function stepGame(dir) {
      var t = tunes[follow.track], now = followedClock();
      if (dir < 0 && now && now.at > 3) { gameSeek(t.entry, 0); return; }
      var same = tunes.filter(function (u) { return u.keys[0] === t.keys[0] && u.game === t.game; });
      var to = same[same.indexOf(t) + dir];
      if (to) gameSeek(to.entry, 0);
    }
    // the display and the list while a track is followed; the player's own again when none is
    var shownTrack = null;
    function showTrack() {
      var i = follow ? follow.track : -1, key = i + ":" + (follow ? follow.title : "");
      if (shownTrack === key) return;
      shownTrack = key;
      Array.prototype.forEach.call(list.querySelectorAll("button[data-tune]"), function (b) { b.classList.toggle("mu-followed", +b.getAttribute("data-tune") === i); });
      var t = tunes[i];
      if (t) {
        tuneEl.textContent = t.name;
        gameEl.textContent = subLine(t);
        names.title = t.name + " \u2014 " + t.game.title;
        var icon = null;
        // the game's icon from its files, as the Library shows it, else its two letters on its colour
        try { var rec = ByopopGameFiles.all()[t.game.id], src = rec && ByopopGameFiles.present(rec) ? (rec.small || rec.icon) : ""; if (src) { icon = make("img", "mu-lcd-icon"); icon.src = src; icon.alt = ""; } } catch (e) {}
        if (!icon) {
          var info = t.game;
          try { var all = window.ByopopGames && (ByopopGames.list ? ByopopGames.list() : ByopopGames); Array.prototype.forEach.call(all || [], function (g) { if (g.id === t.game.id) info = g; }); } catch (e) {}
          icon = make("span", "mu-lcd-cap", (info.tag || info.title || "").slice(0, 2).toUpperCase());
          icon.style.setProperty("--tile", info.colour || "#808080");
        }
        stateGlyph.replaceChild(icon, stateGlyph.firstChild);
        stateGlyph.classList.remove("mu-blink");
      } else if (follow) {
        // a game's music the list does not have (a stream, a module not found): its module's title, if any
        tuneEl.textContent = follow.view.title || "";
        gameEl.textContent = follow.title;
        names.title = follow.title;
      }
      seek.disabled = !source && !followedClock();
    }
    // the game followed, as a value in the display: its title, and its module's own where it has one (the tooltip says both)
    var shownFollow = null;
    function showFollowed() {
      var text = follow ? follow.title : "", tip = follow ? (follow.view.title ? follow.title + ": " + follow.view.title : follow.title) : "";
      // a game whose music does not reach the speakers (it muted itself without the focus, as its original does, or its sound is
      // off, or the desktop lets another window be heard): the tag says so with the muted speaker; the position shown is the game's
      var muted = !!follow && (follow.view.heard === 0 || follow.hushed());
      if (muted) tip = T("music.follow.muted", { game: tip });
      if (shownFollow === tip) return;
      shownFollow = tip;
      followEl.classList.toggle("mu-muted", muted);
      followEl.hidden = !follow;
      followEl.textContent = text;
      followEl.title = tip;
    }
    window.addEventListener("byopop-desk-active", function () { if (onScreen()) tick(); });
    // back from another tab: the frame that ran as the page was hidden ended the loop (nothing seen, nothing followed), so it starts
    // again here (the user, 2026-11-10: "when you switch back, the tracker window has to be activated again to resume")
    document.addEventListener("visibilitychange", function () { if (onScreen()) tick(); });
    window.addEventListener("pageshow", function () { if (onScreen()) tick(); });

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
    fxBtn.addEventListener("click", function () {
      fxOn = !fxOn;
      fxBtn.setAttribute("aria-pressed", String(fxOn));
      keep(FX_KEY, fxOn);
      shownRow = "";
      drawTracker();
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
    // (not the click that ends a press on the tracker's header, which Firefox may give to the display when the header changed)
    var headDownAt = 0;
    visEl.addEventListener("click", function (ev) { if (Date.now() - headDownAt < 1500) return; if (!trackerScroll.contains(ev.target)) nextVis(); });
    // a click on a channel's number in the tracker's header is the channel's, not the display's: a click mutes it or brings it
    // back, Ctrl-click or a double-click plays it alone (again on the channel alone, every channel back; the user, 2026-11-11:
    // "Add double clicking a tracker channel to isolate its sound"). A plain click waits out the double-click's time before
    // it mutes, so the two clicks of a double-click neither leave the channel muted nor cut its sound for a moment.
    // The whole header line is the target (the user, 2026-11-11: "It's too hard to click on the channel to toggle the mute"):
    // a click between two columns, on the line between them, is the nearer column's.
    function headSpan(ev) {
      var span = ev.target.closest ? ev.target.closest(".mu-ch") : null;
      if (span) return span;
      var best = null, far = Infinity;
      Array.prototype.forEach.call(trackerHead.querySelectorAll(".mu-ch"), function (s) {
        var r = s.getBoundingClientRect(), d = ev.clientX < r.left ? r.left - ev.clientX : Math.max(0, ev.clientX - r.right);
        if (d < far) { far = d; best = s; }
      });
      return best;
    }
    var pendingClick = null, DOUBLE_MS = 300;
    // on the press, not the click: a click needs its press and release on the same element, and the names in the header change
    // as the channels play other instruments (the user, 2026-11-12: "Still hard to actually click the channel to mute")
    trackerHead.addEventListener("click", function (ev) { ev.stopPropagation(); });
    trackerHead.addEventListener("mousedown", function (ev) {
      if (ev.button !== 0) return;
      headDownAt = Date.now();
      ev.stopPropagation();
      var span = headSpan(ev);
      if (!span) return;
      var c = +span.getAttribute("data-ch");
      if (pendingClick) { clearTimeout(pendingClick.timer); var was = pendingClick; pendingClick = null; if (ev.detail >= 2 && was.c === c) { clickChannel(c, true); return; } clickChannel(was.c, false); }
      if (ev.ctrlKey || ev.metaKey) { clickChannel(c, true); return; }
      if (ev.detail === 0) { clickChannel(c, false); return; }   // a key's click: at once
      if (ev.detail === 2) { clickChannel(c, true); return; }   // a slow double-click, its first click already taken: still alone
      if (ev.detail > 2) return;   // the third click of a triple: the double-click has been taken
      pendingClick = { c: c, timer: setTimeout(function () { pendingClick = null; clickChannel(c, false); }, DOUBLE_MS) };
    });
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
      function move(e) { if (room > 0) scrollTo(startFirst + Math.round((e.clientX - startX) / zoomOf() / room * (totalChannels - shownChannels))); }
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
      var folded = [];
      try { folded = (localStorage.getItem(FOLD_KEY) || "").split(" "); } catch (e) {}
      byGame.forEach(function (g) {
        // the game's heading: a button that folds its tunes away, with how many there are and how long they last together
        // (the user, 2026-11-12: "make each game collapsible in the player, have each game show its track count and total length")
        var h = make("h3", "mu-game"), ol = make("ol", "mu-tunes"), btn = make("button", "mu-fold");
        var secs = g.items.reduce(function (n, i) { return n + (tunes[i].seconds || 0); }, 0);
        btn.type = "button";
        btn.setAttribute("data-game", g.game.id);
        btn.appendChild(make("span", "mu-fold-mark", ""));
        btn.appendChild(make("span", "mu-game-name", g.game.title));
        btn.appendChild(make("span", "mu-game-sum", T("music.game_sum", { tracks: T("music.game_tracks", { n: g.items.length }), time: T("music.game_minutes", { n: Math.max(1, Math.round(secs / 60)) }) })));
        btn.addEventListener("click", function () { fold(btn, !ol.hidden); });
        h.appendChild(btn);
        h.id = "music-game-" + g.game.id;
        ol.id = "music-tunes-" + g.game.id;
        btn.setAttribute("aria-controls", ol.id);
        ol.setAttribute("aria-labelledby", h.id);
        setFold(btn, ol, folded.indexOf(g.game.id) >= 0);
        g.items.forEach(function (i) {
          var li = make("li"), b = make("button", "mu-tune");
          b.type = "button";
          b.setAttribute("data-tune", String(i));
          b.appendChild(make("span", "mu-num", String(g.items.indexOf(i) + 1)));
          var t = tunes[i], name = make("span", "mu-name", t.name);
          // its name, and where its files lie in the game's folder
          name.title = t.name + "\n" + t.keys.map(function (k) { return filePlace(k.slice(t.game.persist.length + 6)); }).join("\n");
          b.appendChild(name);
          // where the game plays it (or that it never does), and its format
          var where = make("span", "mu-where", t.unused ? T("music.unused") : t.where);
          where.title = t.when ? where.textContent + "\n" + t.when : where.textContent;
          if (t.unused) where.classList.add("mu-unused");
          b.appendChild(where);
          var by = make("span", "mu-composer", t.composer || T("music.composer_unknown"));
          by.title = by.textContent;
          b.appendChild(by);
          b.appendChild(make("span", "mu-format", t.format || ""));
          b.appendChild(make("span", "mu-time", clock(t.seconds)));
          b.addEventListener("click", function () { play(i); });
          li.appendChild(b);
          ol.appendChild(li);
        });
        list.appendChild(h);
        list.appendChild(ol);
      });
    }
    function setFold(btn, ol, shut) {
      ol.hidden = shut;
      btn.setAttribute("aria-expanded", String(!shut));
      btn.firstChild.textContent = shut ? "+" : "-";
    }
    // folds a game's tunes away (shut) or opens them, and keeps the games folded for this browser
    function fold(btn, shut) {
      var ol = document.getElementById(btn.getAttribute("aria-controls"));
      if (!ol) return;
      if (shut && ol.querySelector("[aria-current]")) shut = false;   // the tune playing stays in view
      setFold(btn, ol, shut);
      var ids = Array.prototype.filter.call(list.querySelectorAll(".mu-fold"), function (b) { return b.getAttribute("aria-expanded") === "false"; })
        .map(function (b) { return b.getAttribute("data-game"); });
      try { localStorage.setItem(FOLD_KEY, ids.join(" ")); } catch (e) {}
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
      held: function () { return { ctx: ctx ? ctx.state : null, node: !!node, decoder: !!M, media: wired, sink: viaStream ? "stream" : "direct", element: streamEl ? (streamEl.paused ? "paused" : "playing") : null, latency: ctx ? { base: ctx.baseLatency, output: ctx.outputLatency } : null }; }, view: function () { return { tracker: !tracker.hidden, row: shownRow, spectrum: !!analyser && !spectrum.hidden, rows: rows }; },
      // for tests: the game the views follow, where its module is, and whether its output is tapped
      playing: function () { return state === "playing"; },
      // for tests: the player's own tune's channels, each as its header shows it (muted: struck through; null where the header
      // does not show it) and as its player plays it (1 muted, 0 playing, -1 not known), and the tune playing
      channels: function () {
        var feed = !follow && trackerFeed();
        if (!feed || !source || !source.muteStatus) return null;
        return { tune: tunes[current] && tunes[current].id, list: feed.used().map(function (c) {
          var head = tracker.hidden ? null : trackerHead.querySelector('[data-ch="' + c + '"]');
          return { c: c, head: head ? head.classList.contains("mu-ch-off") : null, heard: source.muteStatus(c) };
        }) };
      },
      // for tests: a click on a channel's head (alone: Ctrl-click), and the view the display shows
      clickChannel: function (c, alone) { clickChannel(c, !!alone); },
      setVis: function (v) { if (VIS.indexOf(v) >= 0) { vis = v; applyVis(); drawTracker(); } },
      play: function (i) { play(i); },
      current: function () { return tunes[current] ? tunes[current].id : null; },
      following: function () { return follow ? { muted: followEl.classList.contains("mu-muted"),  title: follow.title, view: follow.view, tapped: !!follow.tap, at: trackerAt.textContent } : null; },
      tick: tick };
  }

  // (the Sounds window, web/sounds.js, reads the games' files with the same read-only reader, draws its Stop and speaker with the
  // same pictures, and falls back on the same stb_vorbis decoder)
  return { init: init, patternRe: patternRe, glyph: glyph, decoder: decoder, stbOgg: stbOgg, msAdpcm: msAdpcm, wavMp3: wavMp3, filePlace: filePlace, numbered: numbered,
    files: { open: openExisting, list: gameFiles, read: readFile } };
})();
