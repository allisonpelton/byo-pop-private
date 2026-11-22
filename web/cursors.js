// The site's cursor (docs/WEB_PLAYER.md, "The site's cursor"; the user, 2026-11-03: "pull custom cursors out of the user's
// uploaded games so that they can select one to use across the whole site"). A game's page, once the game has started, asks it
// for its set of cursors (framework/WebCursor.cpp, byopop_cursor_set_copy: its image for each kind of cursor) and keeps the ones
// the site has a use for here; the player picks one game's set in the Settings (the listing page's and every game page's), and
// every page of the site then shows it, each cursor where the site means what the game means by it (the user, 2026-11-04: "make
// them contextual on the site too"): the pointer everywhere, the hand on links and controls, the text cursor in text fields, the
// dragging cursor on a cap being dragged, the circle-slash on a disabled control and the wait cursor while the site is busy
// (busy()). A kind the game has no image for falls back as before: the pointer, or the hand for dragging. A game whose own cursor is showing keeps it (its page's game-cursor class); where the game shows none, the
// site's cursor shows over the game too, as the user asked ("If the custom cursor is disabled in a game, could the site's custom
// cursor still display over it?").
//
// A set as the page has it (all(), save()) is {title, pointer, hand, drag, text, notallowed, wait}, each {url, x, y, k} (a PNG
// data URL, its hot spot and a hash of its pixels) or absent. An animated cursor (Water Bugs' and Cosmic Bugs' pointers,
// web/flicursor.js) also has frames, [{url, x, y, k}...] after the first, and ms, the time each shows: browsers animate no cursor
// image, so the page swaps the frames itself on the element under the mouse, and only while it shows that cursor.
//
// Where the sets are kept (three changes of 2026-11-22; the user, 2026-11-16: "Will want all three improvements applied"): each
// game's set in IndexedDB, the database "byopop-cursors" (its store "sets", by game id), its pictures as binary PNG Blobs. A
// picture with at most 256 colours is written again with a palette where that comes out smaller, every pixel that shows as it
// was; and an animated cursor's frames are one strip, side by side in cells of one size with the hot spot at the same place in
// each, cut into frames again as the set is read. localStorage keeps only small items: "byopop-cursors-kept", each kept set's
// title and signature (what a set given again is compared with, and what a page knows of the sets before it reads them);
// "byopop-cursor-shown", the chosen set as the site shows it, so that a page shows it from its first line and the browser's
// cursor never shows before the database is read; "byopop-site-cursor", the id of the chosen set, absent for the browser's
// own cursor; and "byopop-cursors-unkept", the games whose set could not be kept. The sets that were kept in localStorage
// itself, as base64 in "byopop-cursors", move into the database once, as a page loads. The images come from the player's own
// copy of the game and stay in this browser; forgetting the game's files forgets them too (ByopopGameFiles.forget,
// web/settings.js). The screen saver's frame (web/shell.html, ByopopInMemory) keeps its sets in its memory only.
var ByopopCursors = (function () {
  "use strict";
  var KEY = "byopop-cursors", CHOICE = "byopop-site-cursor", UNKEPT = "byopop-cursors-unkept", KEPT = "byopop-cursors-kept",
    SHOWN = "byopop-cursor-shown";
  var KINDS = ["pointer", "hand", "drag", "text", "notallowed", "wait"];
  // A game whose image for a kind means something else (WEB_PLAYER.md, "The site's cursor", the table): it is not kept for that
  // kind. Seven Seas keeps its shovel, the cursor of its bonus on the board, where the framework keeps the circle-slash.
  var OTHER_MEANING = { sevenseas: ["notallowed"] };
  // The games that give a set (tools/cursorstest.js checks the list against their sources): those whose port gives the framework
  // an image for one of the set's kinds (SetCursorImage, WebCursorSetKind), whose page takes the set once the game has started,
  // and those whose page reads an animated pointer from their files (web/flicursor.js), which the listing page also does as it
  // adds their files (web/gameimport.js, addGame). The Settings' choice names those whose files are here without their set.
  var GIVES = ["alchemy", "astropop", "atomica", "bejeweled", "bejeweled2", "bejeweledtwist", "bigmoney", "bookworm", "bookwormadv",
    "bookwormadv2", "cosmicbugs", "dynomite", "hammerheads", "heavyweapon", "mummymaze", "peggle", "pixelus", "rocketmania", "sevenseas",
    "typershark", "venice", "waterbugs"];
  // The kinds the Settings' choice pictures and compares sets by: every game's text cursor is the same I-beam (only its canvas
  // differs), so it tells no set from another. A set's own text cursor still shows in text fields.
  var COMPARED = ["pointer", "hand", "drag", "notallowed", "wait"];
  // The kinds that decide which sets look the same (setsAlike): the pointer and the hand, the two a player sees all the time.
  // Bejeweled's dragging hand is an open one without the shadow (its mask is another size, so the game, like the original,
  // leaves it out) and the user wants it with the others ("One-line fix it is, then", 2026-11-04).
  var LOOKS = ["pointer", "hand"];
  var MAX_FRAMES = 32;
  // the screen saver's frame, whose browser storage is in its memory (web/shell.html): its sets too
  var IN_MEMORY = typeof window !== "undefined" && window.ByopopInMemory === true;
  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function plain(o) { return !!o && typeof o === "object" && !Array.isArray(o); }
  function validId(id) { return typeof id === "string" && /^[a-z0-9_-]{1,40}$/.test(id); }
  function validHash(k) { return typeof k === "string" && /^[0-9a-f]{8}$/.test(k); }
  function validFrame(c) {
    return plain(c) && typeof c.url === "string" && c.url.indexOf("data:image/png;base64,") === 0 && c.url.length < 200000 &&
      c.x === (c.x | 0) && c.y === (c.y | 0) && c.x >= 0 && c.y >= 0 && c.x < 512 && c.y < 512 &&
      (c.k === undefined || validHash(c.k));
  }
  // a kind's image, its frames (if any) all well-formed and few, each shown for 20 ms to 2 s (Cosmic Bugs' pointer shows each
  // frame for 30 ms)
  function validImage(c) {
    if (!validFrame(c)) return false;
    if (c.frames === undefined) return true;
    return Array.isArray(c.frames) && c.frames.length >= 1 && c.frames.length < MAX_FRAMES && c.frames.every(validFrame) &&
      c.ms === (c.ms | 0) && c.ms >= 20 && c.ms <= 2000;
  }
  // (w and h: a frame's size, which the Settings' choice lays an animated cursor's frames out by)
  function copyFrame(c) {
    var out = { url: c.url, x: c.x, y: c.y };
    if (c.k) out.k = c.k;
    if (c.w === (c.w | 0) && c.h === (c.h | 0) && c.w > 0 && c.h > 0) { out.w = c.w; out.h = c.h; }
    return out;
  }
  function copyImage(c) {
    var out = copyFrame(c);
    if (c.frames) {
      out.frames = c.frames.map(copyFrame);
      out.ms = c.ms;
    }
    return out;
  }
  // a well-formed set with a pointer, or null
  function cleanSet(v, id) {
    if (!plain(v) || !validImage(v.pointer)) return null;
    var set = { title: typeof v.title === "string" ? v.title.slice(0, 120) : id };
    KINDS.forEach(function (k) { if (validImage(v[k])) set[k] = copyImage(v[k]); });
    return set;
  }
  function readJson(name) { try { var s = store(); return s ? JSON.parse(s.getItem(name) || "null") : null; } catch (e) { return null; } }
  // o, or null to remove the item; false when the browser would not store it
  function writeJson(name, o) {
    try {
      var s = store();
      if (!s) return false;
      if (o === null) s.removeItem(name); else s.setItem(name, JSON.stringify(o));
      return true;
    } catch (e) { return false; }
  }
  // FNV-1a over a string's characters, as 8 hex digits
  function hashText(s) {
    var h = 0x811c9dc5, i;
    for (i = 0; i < s.length; i++) h = Math.imul(h ^ s.charCodeAt(i), 16777619) >>> 0;
    return ("0000000" + h.toString(16)).slice(-8);
  }
  // What tells one set from another: its title and, for each kind, each frame's pixels (k, or its image without one) and hot
  // spot and the frames' time
  function signature(set) {
    return hashText(JSON.stringify([set.title].concat(KINDS.map(function (k) {
      var c = set[k];
      return c ? [c.ms || 0].concat([c].concat(c.frames || []).map(function (f) { return [f.k || f.url, f.x, f.y]; })) : 0;
    }))));
  }

  // ---------- what is kept ----------
  // the sets read from the database, or kept since, as the page shows them, by id, and the signature each was kept with; whole
  // once load() has read them all (wholeRead: tried, whether or not the database could be read)
  var view = {}, viewSig = {}, whole = false, wholeRead = false;
  // The sets as the page has them, by game id, as far as it has read them: every one once the Settings' choice has been drawn
  // (load()), else those read or kept since the page loaded
  function all() {
    var out = {};
    Object.keys(view).forEach(function (id) { out[id] = cleanSet(view[id], id); });
    return out;
  }
  // The kept sets' titles and signatures, by id (KEPT)
  function kept() {
    var o = readJson(KEPT), out = {};
    if (plain(o)) Object.keys(o).forEach(function (id) {
      var v = o[id];
      if (validId(id) && plain(v) && typeof v.title === "string" && typeof v.sig === "string") out[id] = { title: v.title.slice(0, 120), sig: v.sig };
    });
    return out;
  }
  function noteKept(id, entry) {
    var o = kept();
    if (entry) o[id] = entry; else if (o.hasOwnProperty(id)) delete o[id]; else return;
    writeJson(KEPT, Object.keys(o).length ? o : null);
  }
  // whether a set is kept for the game
  function has(id) { return validId(id) && (view.hasOwnProperty(id) || kept().hasOwnProperty(id) || !!(legacy() || {})[id]); }
  // The games whose set could not be kept (the browser will not store it, or its pictures could not be read): id -> title, in
  // localStorage under UNKEPT, so that the Settings' choice says so instead of the set going missing unseen. Kept the next time
  // the set is, or when the game's files are forgotten.
  function unkept() {
    var out = {}, o = readJson(UNKEPT);
    if (plain(o)) Object.keys(o).forEach(function (id) { if (validId(id) && typeof o[id] === "string") out[id] = o[id].slice(0, 120); });
    return out;
  }
  function noteUnkept(id, title) {
    var o = unkept();
    if (title === null) { if (!o.hasOwnProperty(id)) return; delete o[id]; } else { if (o[id] === title) return; o[id] = title; }
    writeJson(UNKEPT, Object.keys(o).length ? o : null);
    redraw();
  }
  // The database's work, one piece at a time, in the order asked: each job(next) calls next() once it has ended
  var jobs = [], working = false;
  function serial(job) { jobs.push(job); if (!working) nextJob(); }
  function nextJob() {
    var job = jobs.shift(), ended = false;
    if (!job) { working = false; return; }
    working = true;
    function next() { if (ended) return; ended = true; nextJob(); }
    try { job(next); } catch (e) { next(); }
  }
  // Keeps a game's set ({pointer, hand, drag, text, notallowed, wait}, as all() gives them; a kind whose image means something
  // else in that game is left out). True when the set is new or has changed, and is being kept; false without a pointer, or when
  // the same set is kept already. A set the browser will not store, or whose pictures cannot be read, is noted as unkept.
  function save(id, title, set) {
    if (!validId(id) || WITHDRAWN.indexOf(id) >= 0 || !plain(set) || !validImage(set.pointer)) return false;
    var entry = { title: String(title || id).slice(0, 120) }, other = OTHER_MEANING[id] || [];
    KINDS.forEach(function (k) { if (validImage(set[k]) && other.indexOf(k) < 0) entry[k] = copyImage(set[k]); });
    var sig = signature(entry), was = kept()[id];
    if (was && was.sig === sig) { noteUnkept(id, null); return false; }
    serial(function (next) {
      keep(id, entry, sig, function (ok) { noteUnkept(id, ok ? null : entry.title); next(); });
    });
    return true;
  }
  // The set written to the database with sig (null: its pictures' own signature), then read back as the page shows it; done(ok)
  function keep(id, entry, sig, done) {
    toStored(entry, function (rec) {
      if (!rec) { done(false); return; }
      if (sig) rec.sig = sig;
      vault.put(id, rec, function (ok) {
        if (!ok) { done(false); return; }
        toShown(rec, function (shown) {
          noteKept(id, { title: rec.title, sig: rec.sig });
          if (shown) { view[id] = shown; viewSig[id] = rec.sig; } else { delete view[id]; delete viewSig[id]; }
          apply();
          redraw();
          done(true);
        });
      });
    });
  }
  function forget(id) {
    noteUnkept(id, null);
    var old = readJson(KEY);
    if (plain(old) && old.hasOwnProperty(id)) { delete old[id]; writeJson(KEY, Object.keys(old).length ? old : null); }
    function drop() {
      noteKept(id, null);
      delete view[id]; delete viewSig[id];
      var c = shownCache();
      if (c && c.id === id) writeJson(SHOWN, null);
      apply();
      redraw();
    }
    drop();
    // and again once the database has it gone, after any keeping of it asked before
    serial(function (next) { vault.remove(id, function () { drop(); next(); }); });
  }
  // the chosen set's id, or "" for the browser's own cursor
  function chosen() {
    // a desktop theme's cursor (web/themes.js) comes first, where the game's page kept its set
    try { var th = typeof ByopopThemes !== "undefined" ? ByopopThemes.cursor() : ""; if (th) return th; } catch (e) {}
    try { var s = store(), v = s ? s.getItem(CHOICE) : null; return validId(v) ? v : ""; } catch (e) { return ""; }
  }
  function choose(id) {
    try {
      var s = store();
      if (s) { if (id && validId(id)) s.setItem(CHOICE, id); else s.removeItem(CHOICE); }
    } catch (e) {}
    apply();
  }

  // ---------- reading the database ----------
  // A set read from the database (vault.get) as the page shows it, for the chosen set when no page has shown it yet
  var fetching = {}, unread = {};
  function fetch(id) {
    if (!started || fetching[id] || unread[id] || !kept().hasOwnProperty(id)) return;
    fetching[id] = true;
    serial(function (next) {
      vault.get(id, function (rec, gone) {
        if (!rec) {
          delete fetching[id];
          if (gone) noteKept(id, null); else unread[id] = true;
          next();
          return;
        }
        toShown(rec, function (shown) {
          delete fetching[id];
          if (shown) { view[id] = shown; viewSig[id] = rec.sig; apply(); redraw(); } else unread[id] = true;
          next();
        });
      });
    });
  }
  // Every set read from the database (the Settings' choice shows them all), and the titles and signatures put right from it;
  // done() once read, or once it could not be
  var loading = false, loaded = [];
  function load(done) {
    if (wholeRead && whole) { if (done) done(); return; }
    if (done) loaded.push(done);
    if (loading) return;
    loading = true;
    serial(function (next) {
      vault.all(function (recs) {
        var ids = recs ? Object.keys(recs).filter(validId) : [], left = ids.length, index = {}, got = {};
        function finish() {
          loading = false;
          wholeRead = true;
          if (recs) {
            whole = true;
            Object.keys(view).forEach(function (id) { if (!got[id]) { delete view[id]; delete viewSig[id]; } });
            Object.keys(got).forEach(function (id) { view[id] = got[id]; viewSig[id] = index[id].sig; });
            writeJson(KEPT, Object.keys(index).length ? index : null);
          }
          var w = loaded;
          loaded = [];
          w.forEach(function (f) { try { f(); } catch (e) {} });
          apply();
          redraw();
          next();
        }
        if (!left) { finish(); return; }
        ids.forEach(function (id) {
          var rec = recs[id];
          toShown(rec, function (shown) {
            if (shown && typeof rec.sig === "string") { got[id] = shown; index[id] = { title: shown.title, sig: rec.sig }; }
            if (!--left) finish();
          });
        });
      });
    });
  }
  // another tab kept or forgot a set: what this page read of it is dropped, and read again where it shows them all
  function resync() {
    var k = kept();
    Object.keys(view).forEach(function (id) { if (!k[id] || k[id].sig !== viewSig[id]) { delete view[id]; delete viewSig[id]; } });
    if (whole) Object.keys(k).forEach(function (id) { delete unread[id]; if (!view[id]) fetch(id); });
  }

  // ---------- the sets kept before 2026-11-22 ----------
  // Before the database, the sets were in localStorage under KEY, by game id, as all() gives them (their pictures as data URLs).
  // They are read as the page loads, so that the chosen one shows from the first line while they move into the database
  // (migrate), and KEY is removed once every one is there; until then a page that loads tries again.
  function legacy() {
    var o = readJson(KEY);
    if (!plain(o)) return null;
    var out = {};
    Object.keys(o).forEach(function (id) {
      var set = validId(id) && WITHDRAWN.indexOf(id) < 0 ? cleanSet(o[id], id) : null, t = RETIMED[id];
      if (!set) return;
      if (t && set.pointer.frames && set.pointer.ms === t[0]) set.pointer.ms = t[1];
      out[id] = set;
    });
    return out;
  }
  // Water Bugs' and Cosmic Bugs' sets kept before 2026-11-16 took the frame time of their animation file as updates of 10 ms,
  // but the games step once a millisecond (WBStart.cpp and CBStart.cpp, GameFrame), so their pointers' 500 and 300 ms a frame
  // are 50 and 30 (the user, 2026-11-16: "They animate extremely slowly ... They animate smoothly in game"). Their pages and the
  // listing page now build them so (web/flicursor.js); a set kept before is put right as it moves into the database.
  var RETIMED = { waterbugs: [500, 50], cosmicbugs: [300, 30] };
  // Each set moves with its pictures cropped to the pixels that show (a set kept before 2026-11-08 has them whole, without `k`),
  // so that it compares like the ones taken since; a set the database has already, given since by its game, stays.
  function migrate() {
    var old = legacy();
    if (!old) { if (readJson(KEY) !== null) writeJson(KEY, null); return; }
    var ids = Object.keys(old), left = ids.length, lost = false;
    if (!left) { writeJson(KEY, null); return; }
    ids.forEach(function (id) {
      serial(function (next) {
        function end(ok) {
          if (!ok) lost = true;
          noteUnkept(id, ok ? null : old[id].title);
          if (!--left && !lost) writeJson(KEY, null);
          next();
        }
        if (kept().hasOwnProperty(id)) { end(true); return; }
        keep(id, old[id], null, end);
      });
    });
  }

  // Bonnie's Bookstore gave a set from 2026-11-16 until the user had it taken out (2026-11-17: "they look just like the Windows
  // default. Take them out"): a set kept for a game here is never kept again (nor moved, legacy), and a choice of it goes back to
  // the browser's own cursor
  var WITHDRAWN = ["bonnies"];
  function withdraw() {
    WITHDRAWN.forEach(function (id) { noteUnkept(id, null); });
    try { var s = store(), v = s ? s.getItem(CHOICE) : null; if (WITHDRAWN.indexOf(v) >= 0) s.removeItem(CHOICE); } catch (e) {}
  }

  // ---------- the pictures ----------
  // A cursor's image is kept cropped to the pixels that show (alpha above 0) and its hot spot, which is taken from the crop's
  // corner, with `k`, a hash of the cropped pixels: two games that draw one design on canvases of different sizes (Bejeweled's
  // hand on 54 by 54, Alchemy's on 37 by 37) then keep the same picture and hot spot. crop() takes pixels ({width, height, data}
  // as RGBA, as ImageData has them) and a hot spot, and gives the cropped pixels with the hot spot in them and the crop's
  // corner in the image (left, top); without a hot spot it crops to the pixels that show alone.
  function crop(px, hx, hy) {
    var w = px.width, h = px.height, d = px.data, x, y, spot = hx !== undefined;
    var x0 = spot ? hx : w, y0 = spot ? hy : h, x1 = spot ? hx : -1, y1 = spot ? hy : -1;
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
        if (!d[(y * w + x) * 4 + 3]) continue;
        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
      }
    }
    if (x1 < 0) { x0 = y0 = x1 = y1 = 0; }
    x0 = Math.max(0, x0); y0 = Math.max(0, y0); x1 = Math.min(w - 1, x1); y1 = Math.min(h - 1, y1);
    var cw = Math.max(1, x1 - x0 + 1), ch = Math.max(1, y1 - y0 + 1), out = new Uint8ClampedArray(cw * ch * 4), i;
    for (y = 0; y < ch && y0 + y < h; y++) {
      for (x = 0; x < cw && x0 + x < w; x++) {
        i = ((y0 + y) * w + x0 + x) * 4;
        // a pixel that does not show is all zero, as a canvas keeps it
        if (d[i + 3]) out.set([d[i], d[i + 1], d[i + 2], d[i + 3]], (y * cw + x) * 4);
      }
    }
    return { width: cw, height: ch, data: out, left: x0, top: y0,
      x: spot ? Math.min(cw - 1, Math.max(0, hx - x0)) : 0, y: spot ? Math.min(ch - 1, Math.max(0, hy - y0)) : 0 };
  }
  // FNV-1a over the size and pixels, as 8 hex digits
  function hash(px) {
    var h = 0x811c9dc5, d = px.data, i;
    function add(b) { h = Math.imul(h ^ b, 16777619) >>> 0; }
    add(px.width & 255); add(px.width >> 8); add(px.height & 255); add(px.height >> 8);
    for (i = 0; i < d.length; i++) add(d[i]);
    return ("0000000" + h.toString(16)).slice(-8);
  }
  // A kind's image to keep, {url, x, y, k}, from pixels and their hot spot. The pixels should be the ones a canvas gives back
  // (getImageData), so that they hash alike whether they come from the game or from a kept image decoded again.
  function picture(px, hx, hy) {
    var c = crop(px, hx, hy);
    return { url: codec.encode(c), x: c.x, y: c.y, k: hash(c) };
  }
  // The browser's PNG decoding and encoding and its zlib compression (tools/cursorstest.js puts its own in their place):
  // decode(url, done) gives done the pixels or null; encode(px) a PNG data URL; deflate(bytes, done) gives done the bytes
  // compressed as a zlib stream, or null
  var codec = {
    decode: function (url, done) {
      if (typeof Image === "undefined") { done(null); return; }
      var im = new Image();
      im.onload = function () {
        try {
          var cv = document.createElement("canvas"), g;
          cv.width = im.naturalWidth; cv.height = im.naturalHeight;
          g = cv.getContext("2d", { willReadFrequently: true });
          g.drawImage(im, 0, 0);
          done(g.getImageData(0, 0, cv.width, cv.height));
        } catch (e) { done(null); }
      };
      im.onerror = function () { done(null); };
      im.src = url;
    },
    encode: function (px) {
      var cv = document.createElement("canvas");
      cv.width = px.width; cv.height = px.height;
      cv.getContext("2d").putImageData(new ImageData(px.data, px.width, px.height), 0, 0);
      return cv.toDataURL("image/png");
    },
    deflate: function (bytes, done) {
      try {
        if (typeof CompressionStream === "undefined") { done(null); return; }
        new Response(new Blob([bytes]).stream().pipeThrough(new CompressionStream("deflate"))).arrayBuffer()
          .then(function (b) { done(new Uint8Array(b)); }, function () { done(null); });
      } catch (e) { done(null); }
    }
  };
  // bytes and data URLs
  function dataUrl(bytes) {
    var s = "", i;
    for (i = 0; i < bytes.length; i += 8192) s += String.fromCharCode.apply(null, Array.prototype.slice.call(bytes, i, i + 8192));
    return "data:image/png;base64," + btoa(s);
  }
  function bytesOf(url) {
    var s = atob(url.slice(url.indexOf(",") + 1)), out = new Uint8Array(s.length), i;
    for (i = 0; i < s.length; i++) out[i] = s.charCodeAt(i);
    return out;
  }
  // ---------- PNGs with a palette ----------
  // A picture as a PNG with a palette (colour type 3): its colours, at most 256, those that are see-through first (so that
  // tRNS, their alpha, is short), at the fewest bits a pixel that holds them, each row unfiltered (as PNG advises for a
  // palette). Lossless for every pixel that shows; one that does not is (0, 0, 0, 0), as a canvas keeps it. done(bytes), or
  // done(null) for a picture of more colours or where the browser cannot compress.
  var crcTable = null;
  function crc32(bytes, from, to) {
    var c, n, k;
    if (!crcTable) {
      crcTable = new Uint32Array(256);
      for (n = 0; n < 256; n++) { c = n; for (k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; crcTable[n] = c >>> 0; }
    }
    c = 0xffffffff;
    for (n = from; n < to; n++) c = crcTable[(c ^ bytes[n]) & 255] ^ (c >>> 8);
    return (c ^ 0xffffffff) >>> 0;
  }
  function put32(b, at, v) { b[at] = v >>> 24; b[at + 1] = (v >>> 16) & 255; b[at + 2] = (v >>> 8) & 255; b[at + 3] = v & 255; }
  function chunk(type, data) {
    var out = new Uint8Array(12 + data.length), i;
    put32(out, 0, data.length);
    for (i = 0; i < 4; i++) out[4 + i] = type.charCodeAt(i);
    out.set(data, 8);
    put32(out, 8 + data.length, crc32(out, 4, 8 + data.length));
    return out;
  }
  function joined(parts) {
    var n = 0, at = 0, out;
    parts.forEach(function (p) { n += p.length; });
    out = new Uint8Array(n);
    parts.forEach(function (p) { out.set(p, at); at += p.length; });
    return out;
  }
  var PNG_SIGNATURE = [137, 80, 78, 71, 13, 10, 26, 10];
  function palettePng(px, done) {
    var w = px.width, h = px.height, d = px.data, n = w * h, index = new Uint8Array(n), at = {}, colours = [], i, key, a;
    for (i = 0; i < n; i++) {
      a = d[i * 4 + 3];
      key = a ? ((d[i * 4] << 24) | (d[i * 4 + 1] << 16) | (d[i * 4 + 2] << 8) | a) >>> 0 : 0;
      if (!at.hasOwnProperty(key)) {
        if (colours.length === 256) { done(null); return; }
        at[key] = colours.length;
        colours.push(key);
      }
      index[i] = at[key];
    }
    var order = colours.map(function (c, j) { return j; }).sort(function (p, q) {
      return ((colours[p] & 255) === 255 ? 1 : 0) - ((colours[q] & 255) === 255 ? 1 : 0) || p - q;
    });
    var place = new Uint8Array(colours.length), plte = new Uint8Array(colours.length * 3), trns = [];
    order.forEach(function (o, j) {
      var c = colours[o];
      place[o] = j;
      plte[j * 3] = c >>> 24; plte[j * 3 + 1] = (c >>> 16) & 255; plte[j * 3 + 2] = (c >>> 8) & 255;
      if ((c & 255) !== 255) trns.push(c & 255);
    });
    var bits = colours.length <= 2 ? 1 : colours.length <= 4 ? 2 : colours.length <= 16 ? 4 : 8;
    var stride = Math.ceil(w * bits / 8) + 1, raw = new Uint8Array(stride * h), x, y, bit;
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
        bit = x * bits;
        raw[y * stride + 1 + (bit >> 3)] |= place[index[y * w + x]] << (8 - bits - (bit & 7));
      }
    }
    codec.deflate(raw, function (z) {
      if (!z) { done(null); return; }
      var ihdr = new Uint8Array(13);
      put32(ihdr, 0, w); put32(ihdr, 4, h); ihdr[8] = bits; ihdr[9] = 3;
      var parts = [new Uint8Array(PNG_SIGNATURE), chunk("IHDR", ihdr), chunk("PLTE", plte)];
      if (trns.length) parts.push(chunk("tRNS", new Uint8Array(trns)));
      parts.push(chunk("IDAT", z), chunk("IEND", new Uint8Array(0)));
      done(joined(parts));
    });
  }
  // A picture's PNG bytes to keep: with a palette where that comes out smaller than the browser's own, else the browser's
  function pngOf(px, done) {
    var own = null;
    try { own = bytesOf(codec.encode(px)); } catch (e) {}
    palettePng(px, function (b) { done(b && (!own || b.length < own.length) ? b : own); });
  }

  // ---------- sets as the database keeps them ----------
  // A set in the database: {title, sig, kinds}, each kind {png, x, y, k}, or for an animated one the strip {png, x, y, w, n,
  // ms, ks}: n frames side by side in cells w wide (and the strip's height high), each frame placed so that its hot spot is at
  // (x, y) in its cell, ks their hashes. Every picture is cropped first (a set kept before 2026-11-08 had them whole).
  // done(record), or done(null) when the pointer could not be made.
  function toStored(set, done) {
    var rec = { title: set.title, kinds: {} }, seen = { title: set.title }, kinds = KINDS.filter(function (k) { return !!set[k]; }), left = kinds.length;
    if (!left) { done(null); return; }
    kinds.forEach(function (k) {
      storedImage(set[k], function (s, cut) {
        if (s) { rec.kinds[k] = s; seen[k] = cut; }
        if (--left) return;
        if (!rec.kinds.pointer) { done(null); return; }
        rec.sig = signature(seen);
        done(rec);
      });
    });
  }
  // a kind's image as the database keeps it, done(kept, its frames cropped as {x, y, k, frames, ms} for its signature), or
  // done(null) when a frame cannot be read
  function storedImage(c, done) {
    var frames = [c].concat(c.frames || []), cuts = [], left = frames.length, bad = false;
    frames.forEach(function (f, i) {
      codec.decode(f.url, function (px) {
        try { if (px) { cuts[i] = crop(px, f.x, f.y); cuts[i].k = validHash(f.k) ? f.k : hash(cuts[i]); } else bad = true; } catch (e) { bad = true; }
        if (--left) return;
        if (bad) { done(null); return; }
        var cut = { x: cuts[0].x, y: cuts[0].y, k: cuts[0].k };
        if (cuts.length === 1) {
          pngOf(cuts[0], function (b) { done(b ? { png: b, x: cut.x, y: cut.y, k: cut.k } : null, cut); });
          return;
        }
        cut.frames = cuts.slice(1).map(function (q) { return { x: q.x, y: q.y, k: q.k }; });
        cut.ms = c.ms;
        var X = 0, Y = 0, R = 1, B = 1;
        cuts.forEach(function (q) { X = Math.max(X, q.x); Y = Math.max(Y, q.y); R = Math.max(R, q.width - q.x); B = Math.max(B, q.height - q.y); });
        var W = X + R, H = Y + B, strip = { width: W * cuts.length, height: H, data: new Uint8ClampedArray(W * cuts.length * H * 4) };
        cuts.forEach(function (q, j) {
          var ox = j * W + X - q.x, oy = Y - q.y, y;
          for (y = 0; y < q.height; y++) strip.data.set(q.data.subarray(y * q.width * 4, (y + 1) * q.width * 4), ((oy + y) * strip.width + ox) * 4);
        });
        pngOf(strip, function (b) {
          done(b ? { png: b, x: X, y: Y, w: W, n: cuts.length, ms: c.ms, ks: cuts.map(function (q) { return q.k; }) } : null, cut);
        });
      });
    });
  }
  // A record from the database as the page shows it (all()), an animated cursor's strip cut into its frames again and each frame
  // cropped and written as a PNG of its own; done(set), or done(null) when it has no pointer that can be read
  function toShown(rec, done) {
    if (!plain(rec) || !plain(rec.kinds)) { done(null); return; }
    var set = { title: typeof rec.title === "string" ? rec.title.slice(0, 120) : "" }, kinds = KINDS.filter(function (k) { return plain(rec.kinds[k]); }), left = kinds.length;
    if (!left) { done(null); return; }
    kinds.forEach(function (k) {
      shownImage(rec.kinds[k], function (c) {
        if (c && validImage(c)) set[k] = c;
        if (--left) return;
        done(set.pointer ? set : null);
      });
    });
  }
  function shownImage(s, done) {
    if (!s.png || typeof s.png.length !== "number") { done(null); return; }
    var url = dataUrl(s.png);
    if (!s.n) { done({ url: url, x: s.x, y: s.y, k: s.k }); return; }
    if (s.n !== (s.n | 0) || s.n < 2 || s.n > MAX_FRAMES || !(s.w > 0) || !Array.isArray(s.ks) || s.ks.length !== s.n) { done(null); return; }
    codec.decode(url, function (px) {
      if (!px || px.width !== s.w * s.n) { done(null); return; }
      var cuts = [], i, y, cell;
      for (i = 0; i < s.n; i++) {
        cell = { width: s.w, height: px.height, data: new Uint8ClampedArray(s.w * px.height * 4) };
        for (y = 0; y < px.height; y++) cell.data.set(px.data.subarray((y * px.width + i * s.w) * 4, (y * px.width + (i + 1) * s.w) * 4), y * s.w * 4);
        cuts.push(crop(cell, s.x, s.y));
      }
      var urls = [], left = cuts.length;
      cuts.forEach(function (q, j) {
        pngOf(q, function (b) {
          urls[j] = b ? dataUrl(b) : null;
          if (--left) return;
          if (urls.some(function (u) { return !u; })) { done(null); return; }
          var f = cuts.map(function (q, j) { return { url: urls[j], x: q.x, y: q.y, k: s.ks[j], w: q.width, h: q.height }; }), out = f[0];
          out.frames = f.slice(1);
          out.ms = s.ms;
          done(out);
        });
      });
    });
  }
  // The database (tools/cursorstest.js puts its own in its place): all(done) gives done the records by id, or null when it
  // cannot be read; get(id, done) the record, or null with true when there is none; put(id, record, done) and remove(id,
  // done) true once written. A record's pictures (png) are bytes here and Blobs in the database.
  var DB = "byopop-cursors", SETS = "sets", db = null, dbWaiting = null;
  function openDb(done) {
    if (db) { done(db); return; }
    if (dbWaiting) { dbWaiting.push(done); return; }
    dbWaiting = [done];
    function end(d) {
      if (!dbWaiting) return;
      db = d;
      var w = dbWaiting;
      dbWaiting = null;
      w.forEach(function (f) { f(d); });
    }
    try {
      if (typeof indexedDB === "undefined") { end(null); return; }
      var r = indexedDB.open(DB, 1);
      r.onupgradeneeded = function () { if (!r.result.objectStoreNames.contains(SETS)) r.result.createObjectStore(SETS); };
      r.onsuccess = function () {
        var d = r.result;
        d.onversionchange = function () { d.close(); db = null; };
        end(d);
      };
      r.onerror = function () { end(null); };
    } catch (e) { end(null); }
  }
  function withStore(mode, work, done) {
    openDb(function (d) {
      if (!d) { done(false); return; }
      try {
        var t = d.transaction(SETS, mode), got = work(t.objectStore(SETS));
        t.oncomplete = function () { done(true, got); };
        t.onerror = t.onabort = function () { done(false); };
      } catch (e) { done(false); }
    });
  }
  // a record with its pictures as Blobs, as the database keeps it, and back with them as bytes
  function toBlobs(rec) {
    var out = { title: rec.title, sig: rec.sig, kinds: {} };
    Object.keys(rec.kinds).forEach(function (k) {
      var s = rec.kinds[k], c = {};
      Object.keys(s).forEach(function (f) { c[f] = s[f]; });
      c.png = new Blob([s.png], { type: "image/png" });
      out.kinds[k] = c;
    });
    return out;
  }
  function fromBlobs(v, done) {
    if (!plain(v) || !plain(v.kinds)) { done(null); return; }
    var rec = { title: v.title, sig: v.sig, kinds: {} }, kinds = Object.keys(v.kinds), left = kinds.length;
    if (!left) { done(rec); return; }
    kinds.forEach(function (k) {
      var s = v.kinds[k], b = plain(s) ? s.png : null;
      Promise.resolve(b && typeof b.arrayBuffer === "function" ? b.arrayBuffer() : null).then(function (buf) {
        if (!buf) return;
        var c = {};
        Object.keys(s).forEach(function (f) { c[f] = s[f]; });
        c.png = new Uint8Array(buf);
        rec.kinds[k] = c;
      }).catch(function () {}).then(function () { if (!--left) done(rec); });
    });
  }
  var vault = {
    all: function (done) {
      var keys, vals;
      withStore("readonly", function (st) { keys = st.getAllKeys(); vals = st.getAll(); }, function (ok) {
        if (!ok) { done(null); return; }
        var out = {}, left = keys.result.length;
        if (!left) { done(out); return; }
        keys.result.forEach(function (id, i) { fromBlobs(vals.result[i], function (rec) { if (rec) out[id] = rec; if (!--left) done(out); }); });
      });
    },
    get: function (id, done) {
      var r;
      withStore("readonly", function (st) { r = st.get(id); }, function (ok) {
        if (!ok) { done(null); return; }
        if (r.result === undefined) { done(null, true); return; }
        fromBlobs(r.result, function (rec) { done(rec); });
      });
    },
    put: function (id, rec, done) {
      var v;
      try { v = toBlobs(rec); } catch (e) { done(false); return; }
      withStore("readwrite", function (st) { st.put(v, id); }, function (ok) { done(ok); });
    },
    remove: function (id, done) { withStore("readwrite", function (st) { st.delete(id); }, function (ok) { done(ok); }); }
  };
  // the screen saver's frame: in its memory
  if (IN_MEMORY) (function () {
    var recs = {};
    vault.all = function (done) { var o = {}; Object.keys(recs).forEach(function (id) { o[id] = recs[id]; }); done(o); };
    vault.get = function (id, done) { if (recs.hasOwnProperty(id)) done(recs[id]); else done(null, true); };
    vault.put = function (id, rec, done) { recs[id] = rec; done(true); };
    vault.remove = function (id, done) { delete recs[id]; done(true); };
  })();

  // ---------- showing it ----------
  // A cursor value: the image with its hot spot, and the browser's own of that kind behind it
  function value(c, fallback) { return c ? 'url("' + c.url + '") ' + c.x + " " + c.y + ", " + fallback : fallback; }
  var ON = ":root.site-cursor:not(.game-cursor)", BUSY = ":root.site-cursor.site-busy:not(.game-cursor)";
  var HAND = ["a[href]", "button:not(:disabled)", "select:not(:disabled)", "summary", "label[for]", "[role=button]:not([aria-disabled=true])",
    "[role=tab]", "input[type=checkbox]:not(:disabled)", "input[type=radio]:not(:disabled)", "input[type=range]:not(:disabled)",
    "input[type=file]:not(:disabled)", "input[type=button]:not(:disabled)", "input[type=submit]:not(:disabled)", ".badge[draggable=true]"];
  var TEXT = ["input:not([type])", "input[type=text]", "input[type=search]", "input[type=email]", "input[type=number]", "input[type=url]",
    "textarea", "[contenteditable=true]"];
  var DISABLED = ["button:disabled", "select:disabled", "input:disabled", "textarea:disabled", "[aria-disabled=true]"];
  function css(set) {
    var on = ON;
    var P = value(set.pointer, "auto"), H = value(set.hand, "pointer"), D = value(set.drag || set.hand, "grabbing"), T = value(set.text, "text");
    function each(list) { return list.map(function (x) { return on + " " + x; }).join(", "); }
    // Where the game has no image for a context, it keeps the cursor it had before: a disabled control the pointer, a busy page
    // whatever is under the mouse. The wait cursor wins over every other rule but the dragging one (the ids are no element's:
    // they only raise the rules' weight), so that a cap still shows the dragging cursor while the games it adds are read.
    return on + ", " + on + " * { cursor: " + P + " !important; }\n" +
      each(HAND) + " { cursor: " + H + " !important; }\n" +
      each(TEXT) + " { cursor: " + T + " !important; }\n" +
      (set.notallowed ? each(DISABLED) + " { cursor: " + value(set.notallowed, "not-allowed") + " !important; }\n" : "") +
      (set.wait ? BUSY + ", " + BUSY + " *:not(#bpc-wait) { cursor: " + value(set.wait, "wait") + " !important; }\n" : "") +
      on + " .badge[draggable=true]:active:not(#bpc-drag) { cursor: " + D + " !important; }\n" +
      // over a game that shows no cursor of its own: SDL's cursor for the canvas (its inline style) by kind, and none stays none
      on + ' canvas[style*="cursor: pointer"] { cursor: ' + H + " !important; }\n" +
      on + ' canvas[style*="cursor: text"] { cursor: ' + T + " !important; }\n" +
      on + ' canvas[style*="cursor: move"], ' + on + ' canvas[style*="cursor: grab"] { cursor: ' + D + " !important; }\n" +
      on + ' canvas[style*="cursor: none"] { cursor: none !important; }\n';
  }
  // The chosen set as the site shows it, in localStorage under SHOWN ({id, sig, set}), for the next page's first line
  function shownCache() {
    var o = readJson(SHOWN);
    return plain(o) && validId(o.id) && typeof o.sig === "string" && plain(o.set) ? o : null;
  }
  // the chosen set: as read, else as the last page left it (unless it has been kept again since), else as kept before the
  // database; null until it is read
  function shownSet(id) {
    if (!id) return null;
    if (view[id]) return view[id];
    var c = shownCache(), k = kept()[id], old;
    if (c && c.id === id && (!k || k.sig === c.sig)) { var set = cleanSet(c.set, id); if (set) return set; }
    old = legacy();
    return old && old[id] ? old[id] : null;
  }
  var styleEl = null;
  var current = null;  // the chosen set as apply() last read it
  var started = false;  // once the page has loaded (start): the database is read from then on
  function apply() {
    var id = chosen(), set = shownSet(id), root = document.documentElement, c;
    current = set;
    // the next page's copy of it: written as it is read; gone with no set chosen
    if (id && view[id]) {
      c = shownCache();
      if (!c || c.id !== id || c.sig !== viewSig[id]) writeJson(SHOWN, { id: id, sig: viewSig[id], set: view[id] });
    } else if (!id && shownCache()) writeJson(SHOWN, null);
    if (id && !view[id]) fetch(id);
    if (!set) {
      root.classList.remove("site-cursor");
      if (styleEl) styleEl.textContent = "";
      animate(hovered);
      return;
    }
    if (!styleEl) {
      styleEl = document.createElement("style");
      styleEl.id = "byopop-site-cursor";
      (document.head || root).appendChild(styleEl);
    }
    styleEl.textContent = css(set);
    root.classList.add("site-cursor");
    animate(hovered);
  }

  // ---------- the site is busy ----------
  // busy(what, on): a piece of work (a game page loading, games being added, a save backup, restore or import) has started or
  // ended; the page counts as busy while any has not ended, and the chosen set's wait cursor (if it has one) shows everywhere
  var busyWith = {};
  function busy(what, on) {
    if (on) busyWith[what] = true; else delete busyWith[what];
    document.documentElement.classList.toggle("site-busy", Object.keys(busyWith).length > 0);
    animate(hovered);
  }

  // ---------- animated cursors ----------
  // Browsers animate no cursor image, so the element under the mouse gets each frame in turn as its own cursor (an inline style,
  // which outweighs the site's rules), and gives it back when the mouse leaves it or that cursor no longer applies. Only one
  // element and one timer at a time, none for a set without frames, and none while the page is hidden. The browser shows a
  // changed cursor without the mouse moving.
  // The element's own inline cursor is kept aside while it shows the frames and put back after (a game's canvas has SDL's:
  // none while the game draws its own cursor). Each frame first looks again at what applies: a cursor the page or the game set
  // on the element since the last frame becomes its own, and a game that has started showing its own cursor (the page's
  // game-cursor class) or hidden the browser's over its canvas stops the frames at once, so that the site's cursor is never
  // drawn over the game's (the user, 2026-11-16: "The Cosmic Bugs cursor shows duplicated over the game cursor").
  // animCursor: the cursor whose frames show, so that another set chosen (or the same set kept again) starts its own frames
  var hovered = null, animKind = "", animEl = null, animCursor = null, animTimer = 0, animAt = 0, animOwn = null, animLast = "";
  // el's own inline cursor: the one kept aside while it shows the frames
  function ownCursor(el) {
    if (el === animEl && animOwn) return animOwn.value;
    try { return el.style.getPropertyValue("cursor") || ""; } catch (e) { return ""; }
  }
  // the kind of cursor the site's rules give el, as css() orders them
  function kindOf(el, set) {
    var root = document.documentElement;
    if (!el || el.nodeType !== 1 || !root.classList.contains("site-cursor") || root.classList.contains("game-cursor")) return "";
    function any(list) { return list.some(function (x) { try { return el.matches(x); } catch (e) { return false; } }); }
    if (any([".badge[draggable=true]:active"])) return set.drag ? "drag" : "hand";
    if (set.wait && root.classList.contains("site-busy")) return "wait";
    if (set.notallowed && any(DISABLED)) return "notallowed";
    if (any(TEXT)) return set.text ? "text" : "";
    if (any(HAND)) return set.hand ? "hand" : "";
    if (el.tagName === "CANVAS") {
      var s = ownCursor(el);
      if (/^\s*none\b/.test(s)) return "";
      if (/^\s*pointer\b/.test(s)) return set.hand ? "hand" : "";
      if (/^\s*text\b/.test(s)) return set.text ? "text" : "";
      if (/^\s*(move|grab)\b/.test(s)) return set.drag ? "drag" : set.hand ? "hand" : "";
    }
    return "pointer";
  }
  // While the frames show, the page's classes and the element's own cursor are watched, so that the frames stop as the game's
  // cursor comes on or SDL hides the browser's, before the next frame is drawn rather than at the next frame of the swap.
  // Whether the frames still apply: a cursor set on the element since the last frame is its own now; if what applies has
  // changed, the frames stop and start again for what applies now. False once they stopped.
  function stillApplies() {
    var now = animEl.style.getPropertyValue("cursor") || "";
    if (now !== animLast) animOwn = { value: now, priority: animEl.style.getPropertyPriority("cursor") || "" };
    if (kindOf(animEl, current) === animKind) return true;
    var at = animEl;
    stopAnimation();
    if (at === hovered) animate(at);
    return false;
  }
  var watcher = typeof MutationObserver !== "undefined" ? new MutationObserver(function () { if (animEl) stillApplies(); }) : null;
  function stopAnimation() {
    clearInterval(animTimer);
    animTimer = 0;
    if (watcher) watcher.disconnect();
    // its own cursor back, unless the page or the game has set another since the last frame
    if (animEl) {
      try {
        if (animEl.style.getPropertyValue("cursor") === animLast) {
          if (animOwn && animOwn.value) animEl.style.setProperty("cursor", animOwn.value, animOwn.priority);
          else animEl.style.removeProperty("cursor");
        }
      } catch (e) {}
    }
    animEl = null; animKind = ""; animCursor = null; animOwn = null; animLast = "";
  }
  function animate(el) {
    var set = current, kind = set ? kindOf(el, set) : "", c = kind ? set[kind] : null;
    if (!c || !c.frames || document.visibilityState === "hidden") { stopAnimation(); return; }
    if (el === animEl && kind === animKind && c === animCursor) return;
    stopAnimation();
    var frames = [c].concat(c.frames), fallback = { pointer: "auto", hand: "pointer", drag: "grabbing", text: "text", notallowed: "not-allowed", wait: "wait" }[kind];
    animEl = el; animKind = kind; animCursor = c; animAt = 0;
    animOwn = { value: el.style.getPropertyValue("cursor") || "", priority: el.style.getPropertyPriority("cursor") || "" };
    function show() {
      if (!stillApplies()) return;
      animEl.style.setProperty("cursor", value(frames[animAt], fallback), "important");
      animLast = animEl.style.getPropertyValue("cursor") || "";
      animAt = (animAt + 1) % frames.length;
    }
    animLast = animOwn.value;
    show();
    if (animEl !== el) return;
    animTimer = setInterval(show, c.ms);
    if (watcher) {
      try {
        watcher.observe(document.documentElement, { attributes: true, attributeFilter: ["class"] });
        watcher.observe(el, { attributes: true, attributeFilter: ["style"] });
      } catch (e) {}
    }
  }
  // (cheap: a set without frames never starts a timer; the listeners only look at the element the mouse entered)
  function anyFrames() { var set = current; return !!set && KINDS.some(function (k) { return !!(set[k] && set[k].frames); }); }
  function later() { setTimeout(function () { animate(hovered); }, 0); }
  document.addEventListener("mouseover", function (ev) { hovered = ev.target; if (animTimer || anyFrames()) animate(hovered); }, true);
  document.addEventListener("mouseout", function (ev) { if (!ev.relatedTarget) { hovered = null; stopAnimation(); } }, true);
  // what applies under a mouse that stays on one element can change (a game showing the browser's cursor again over its
  // canvas): looked at again as the mouse moves, at most every 100 ms, while nothing is animated
  var lastLook = 0;
  document.addEventListener("mousemove", function (ev) {
    if (animTimer || !anyFrames()) return;
    var t = Date.now();
    if (t - lastLook < 100) return;
    lastLook = t;
    hovered = ev.target;
    animate(hovered);
  }, true);
  // a press and a release can change the rule that applies (a cap being dragged)
  document.addEventListener("mousedown", function () { if (animTimer || anyFrames()) later(); }, true);
  document.addEventListener("mouseup", function () { if (animTimer || anyFrames()) later(); }, true);
  document.addEventListener("visibilitychange", function () { animate(hovered); });

  // ---------- the choice in the Settings ----------
  // A group of radio buttons: the browser's own cursor, then each set with its pointer and hand. Games whose sets are the same,
  // every kind with the same image, hot spot and frames, share one choice named with all their titles (the user, 2026-11-04:
  // "Alchemy and Bejeweled have the same cursor ... doesn't help to list both"); the sets stay one per game, and choosing the
  // group keeps one member's id, so the groups form again from what is kept: a set that changes leaves its group, and forgetting a
  // game leaves the rest. Rendered into el once every set has been read, kept up to date with the other tabs.
  var STYLE =
    ".bpc-cur { margin: 12px 0 0; padding: 0; border: 0; min-width: 0; display: grid; gap: 6px; }\n" +
    // the legend as the Settings' group headings (web/settings.js, .bps-group), the choices in one box as its rows
    ".bpc-cur legend { padding: 0; margin-bottom: 8px; font-size: 12.5px; font-weight: 600; letter-spacing: 0.04em; text-transform: uppercase; color: var(--muted, #6b6680); }\n" +
    ".bpc-cur .bpc-cur-box { display: grid; gap: 4px; padding: 10px 14px; border: 1px solid var(--line, #d9d5e3); border-radius: 10px; background: var(--panel-2, var(--panel, transparent)); }\n" +
    // a row: the button, the set's cursors and the games' names; on a narrow screen the cursors wrap and the names go under them
    ".bpc-cur label { display: flex; flex-wrap: wrap; align-items: center; gap: 4px 8px; min-height: 32px; }\n" +
    ".bpc-cur .bpc-cur-pics { display: inline-flex; flex-wrap: wrap; align-items: center; gap: 4px; min-width: 72px; max-width: 100%; }\n" +
    ".bpc-cur .bpc-cur-pics img { max-width: 32px; max-height: 32px; image-rendering: pixelated; }\n" +
    // an animated cursor: its frames in one box, each placed by its hot spot (moving)
    ".bpc-cur .bpc-cur-anim { position: relative; display: inline-block; flex: none; }\n" +
    ".bpc-cur .bpc-cur-name { flex: 1 1 8em; min-width: 0; overflow-wrap: anywhere; }\n" +
    "@media (max-width: 480px) { .bpc-cur .bpc-cur-pics img { max-width: 24px; max-height: 24px; } .bpc-cur .bpc-cur-anim { zoom: 0.75; } }\n" +
    ".bpc-cur .bpc-cur-pics .bpc-cur-anim img { position: absolute; max-width: none; max-height: none; }\n" +
    ".bpc-cur .bpc-cur-hint { margin: 0; color: var(--muted, #6b6680); font-size: 13px; }\n";
  var rendered = [], idCount = 0;
  function redraw() { rendered.forEach(fill); }
  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text !== undefined) e.textContent = text;
    return e;
  }
  // An animated cursor's picture: its frames laid on each other by their hot spots in a box of one size, drawn at most 32 pixels
  // across as the other pictures are, the first frame until the mouse is over its row, which plays them at the cursor's own
  // pace (the user, 2026-11-16: "Add mouse over for animated custom cursors to show the animation"), and the first again as the
  // mouse leaves. stops: where its stop goes, for the next drawing of the choice.
  function moving(c, kind, row, stops) {
    var frames = [c].concat(c.frames), X = 0, Y = 0, R = 1, B = 1;
    frames.forEach(function (f) { X = Math.max(X, f.x); Y = Math.max(Y, f.y); R = Math.max(R, (f.w || 1) - f.x); B = Math.max(B, (f.h || 1) - f.y); });
    var W = X + R, H = Y + B, s = Math.min(1, 32 / Math.max(W, H)), box = make("span", "bpc-cur-anim"), img = make("img"), at = 0, timer = 0;
    box.style.width = Math.round(W * s) + "px";
    box.style.height = Math.round(H * s) + "px";
    img.alt = "";
    img.setAttribute("data-kind", kind);
    function show(i) {
      var f = frames[i];
      img.src = f.url;
      img.style.left = Math.round((X - f.x) * s) + "px";
      img.style.top = Math.round((Y - f.y) * s) + "px";
      img.style.width = Math.max(1, Math.round((f.w || 1) * s)) + "px";
      img.style.height = Math.max(1, Math.round((f.h || 1) * s)) + "px";
    }
    function stop() { clearInterval(timer); timer = 0; at = 0; show(0); }
    show(0);
    box.appendChild(img);
    row.addEventListener("mouseenter", function () {
      if (timer) return;
      timer = setInterval(function () { at = (at + 1) % frames.length; show(at); }, c.ms);
    });
    row.addEventListener("mouseleave", stop);
    stops.push(stop);
    return box;
  }
  function fill(el) {
    // the animations of the last drawing stop
    (el.bpcStops || []).forEach(function (f) { f(); });
    var stops = el.bpcStops = [];
    if (!wholeRead) return;
    var sets = all(), pick = chosen(), name = "bpc-cur-" + (++idCount);
    lookAt(sets);
    el.textContent = "";
    var fs = make("fieldset", "bpc-cur"), box = make("div", "bpc-cur-box");
    fs.appendChild(make("legend", null, ByopopStrings.t("cursors.legend")));
    fs.appendChild(box);
    function option(id, text, set) {
      var label = make("label"), input = make("input");
      input.type = "radio"; input.name = name; input.value = id; input.checked = id === pick || (!id && !sets[pick]);
      input.addEventListener("change", function () { if (input.checked) choose(id); });
      label.appendChild(input);
      if (set) {
        // every cursor the set has but its text cursor, in KINDS' order (an animated one by its first frame until the mouse is
        // over the row), as pictures only: the names say which
        var pics = make("span", "bpc-cur-pics");
        COMPARED.forEach(function (k) {
          if (!set[k]) return;
          if (set[k].frames) { pics.appendChild(moving(set[k], k, label, stops)); return; }
          var img = make("img");
          img.src = set[k].url; img.alt = "";
          img.setAttribute("data-kind", k);
          pics.appendChild(img);
        });
        label.appendChild(pics);
      }
      label.appendChild(make("span", "bpc-cur-name", text));
      box.appendChild(label);
    }
    option("", ByopopStrings.t("cursors.browser"), null);
    groups(sets).forEach(function (g) {
      // the chosen member stands for its group; else the first by title
      var id = g.ids.indexOf(pick) >= 0 ? pick : g.ids[0];
      option(id, names(g.ids.map(function (i) { return sets[i].title; })), sets[id]);
    });
    // the games whose set could not be kept, then those whose files are here and whose set is not yet, else the plain hint
    var lost = unkept(), lostIds = Object.keys(lost).filter(function (id) { return !sets[id]; }), wait = waiting(sets, lost);
    if (lostIds.length) {
      var lostTitles = lostIds.map(function (id) { return lost[id]; }).sort(function (a, b) { return a.localeCompare(b); });
      box.appendChild(make("p", "bpc-cur-hint bpc-cur-unkept", ByopopStrings.t("cursors.unkept", { games: names(lostTitles), n: lostIds.length })));
    }
    box.appendChild(make("p", "bpc-cur-hint", wait.length ? ByopopStrings.t("cursors.hint_open", { games: names(wait), n: wait.length })
      : Object.keys(sets).length ? ByopopStrings.t("cursors.hint") : ByopopStrings.t("cursors.hint_none")));
    el.appendChild(fs);
  }
  // The titles of the games that give a set (GIVES) whose files are here (web/settings.js, ByopopGameFiles) and whose set is
  // neither kept nor unkept, sorted: a game that has to be opened once for it. Only where the games' titles are known (the listing
  // page's ByopopImport.games); a game's own page names none.
  function waiting(sets, lost) {
    var files, titles = {};
    try { files = ByopopGameFiles.all(); ByopopImport.games.forEach(function (g) { titles[g.spec.id || g.target] = g.title; }); }
    catch (e) { return []; }
    return GIVES.filter(function (id) { return !sets[id] && !lost[id] && titles[id] && ByopopGameFiles.present(files[id]); })
      .map(function (id) { return titles[id]; }).sort(function (a, b) { return a.localeCompare(b); });
  }
  // ---------- sets that look the same ----------
  // Two sets look the same, and share an entry, when every kind in LOOKS looks the same in both: pictures that, laid on each
  // other by their hot spots (or with one moved a pixel), drawn over white and over black, differ nowhere by more than
  // LOOK_ALIKE of 255 in any colour (the user, 2026-11-04: differences nobody can see do not split an entry). BookWorm's
  // pointer is Alchemy's with colours a step or three off. An entry pictures the set of the member that stands for it (the
  // chosen one, else the first by title), whose dragging cursor may not be every member's. The pictures are decoded once a URL; until then,
  // or when one cannot be decoded, a picture looks the same only as the same pixels (`k`) or image with the same hot spot.
  // 40 of 255: Bejeweled Twist's pointer and hand are Alchemy's design kept as JPEG 2000, a few pixels up to 33 off, with
  // faint edges (alpha up to 9) its GIF masks add or take away; Atomica's green differs by 127 to 220
  var LOOK_ALIKE = 40;
  // A few stray pixels may be further off, up to LOOK_STRAY of 255: at most 1 in 50 of the pixels that show in either picture,
  // and at least 2. Twist's hand, as the JPEG 2000 decoder gives it now, has 4 of its 300-odd pixels 47 off Alchemy's (a dark
  // brown at the cuff), and so stood apart (the user, 2026-11-08: "The Twist cursor set is still shown separately from the
  // other games in the list"); a picture of another colour differs everywhere, far more than that
  var LOOK_STRAY = 64;
  var seen = {}, seeing = {};  // URL: the pixels that show {width, height, data, left, top}, or null; URL: being decoded
  function lookAt(sets) {
    var urls = [], now = true, left;
    Object.keys(sets).forEach(function (id) {
      LOOKS.forEach(function (k) {
        var c = sets[id][k];
        if (c) [c].concat(c.frames || []).forEach(function (f) {
          if (!seen.hasOwnProperty(f.url) && !seeing[f.url] && urls.indexOf(f.url) < 0) urls.push(f.url);
        });
      });
    });
    left = urls.length;
    urls.forEach(function (u) {
      seeing[u] = true;
      codec.decode(u, function (px) {
        try { seen[u] = px ? crop(px) : null; } catch (e) { seen[u] = null; }
        delete seeing[u];
        // drawn again once all are in, unless they came at once (the stand-in codec), before the entries are made
        if (!--left && !now) redraw();
      });
    });
    now = false;
  }
  // A picture's pixel at (u, v) from its hot spot, as RGBA (all zero outside it)
  function pixelAt(p, hx, hy, u, v, out) {
    var x = u + hx - p.left, y = v + hy - p.top, i;
    if (x < 0 || y < 0 || x >= p.width || y >= p.height) { out[0] = out[1] = out[2] = out[3] = 0; return; }
    i = (y * p.width + x) * 4;
    out[0] = p.data[i]; out[1] = p.data[i + 1]; out[2] = p.data[i + 2]; out[3] = p.data[i + 3];
  }
  // The two pictures laid on each other by their hot spots, the second moved by (sx, sy): no pixel of either differs by more
  // than LOOK_ALIKE over white or over black, but for a stray few (LOOK_STRAY). Every pixel of both is looked at (before
  // 2026-11-22 the moved picture's bounds went the other way, so a row or column at its edge could go unseen).
  function overlayAlike(pa, a, pb, b, sx, sy) {
    var ax = a.x - pa.left, ay = a.y - pa.top, bx = b.x - pb.left - sx, by = b.y - pb.top - sy;
    var u0 = Math.min(-ax, -bx), v0 = Math.min(-ay, -by), u1 = Math.max(pa.width - ax, pb.width - bx), v1 = Math.max(pa.height - ay, pb.height - by);
    var ca = [0, 0, 0, 0], cb = [0, 0, 0, 0], u, v, c, bg, d, worst, shown = 0, stray = 0;
    for (v = v0; v < v1; v++) {
      for (u = u0; u < u1; u++) {
        pixelAt(pa, a.x, a.y, u, v, ca);
        pixelAt(pb, b.x, b.y, u - sx, v - sy, cb);
        if (ca[3] || cb[3]) shown++;
        if (ca[0] === cb[0] && ca[1] === cb[1] && ca[2] === cb[2] && ca[3] === cb[3]) continue;
        worst = 0;
        for (bg = 0; bg <= 255; bg += 255) {
          for (c = 0; c < 3; c++) {
            d = Math.abs((ca[c] * ca[3] - cb[c] * cb[3] + bg * (cb[3] - ca[3])) / 255);
            if (d > worst) worst = d;
          }
        }
        if (worst > LOOK_STRAY) return false;
        if (worst > LOOK_ALIKE) stray++;
      }
    }
    return stray <= Math.max(2, Math.floor(shown / 50));
  }
  function frameAlike(a, b) {
    if (a.k && a.k === b.k && a.x === b.x && a.y === b.y) return true;
    var pa = seen[a.url], pb = seen[b.url], sx, sy;
    if (!pa || !pb) return (a.k || a.url) === (b.k || b.url) && a.x === b.x && a.y === b.y;
    // a picture well apart in size is another picture; else the hot spots may be a pixel apart
    if (Math.abs(pa.width - pb.width) > 2 || Math.abs(pa.height - pb.height) > 2) return false;
    for (sy = -1; sy <= 1; sy++) for (sx = -1; sx <= 1; sx++) if (overlayAlike(pa, a, pb, b, sx, sy)) return true;
    return false;
  }
  function kindAlike(a, b) {
    if (!a || !b) return !a && !b;
    var fa = [a].concat(a.frames || []), fb = [b].concat(b.frames || []);
    return (a.ms || 0) === (b.ms || 0) && fa.length === fb.length && fa.every(function (f, i) { return frameAlike(f, fb[i]); });
  }
  function setsAlike(s, t) { return LOOKS.every(function (k) { return kindAlike(s[k], t[k]); }); }
  // The sets in groups that look the same, {ids} each with its games sorted by title, the groups by their first title; a set
  // joins the first group whose first set it looks like
  function groups(sets) {
    var out = [];
    function byTitle(a, b) { return sets[a].title.localeCompare(sets[b].title) || (a < b ? -1 : a > b ? 1 : 0); }
    Object.keys(sets).sort(byTitle).forEach(function (id) {
      for (var i = 0; i < out.length; i++) {
        if (setsAlike(sets[out[i].ids[0]], sets[id])) { out[i].ids.push(id); return; }
      }
      out.push({ ids: [id] });
    });
    return out;
  }
  // "Alchemy", "Alchemy and Bejeweled", "Alchemy, Atomica and Bejeweled"
  function names(titles) {
    if (titles.length < 2) return titles[0] || "";
    if (titles.length === 2) return ByopopStrings.t("site.list_two", {first: titles[0], second: titles[1]});
    return ByopopStrings.t("site.list_many", {rest: titles.slice(0, -1).join(", "), last: titles[titles.length - 1]});
  }
  function render(el) {
    if (!el) return;
    if (!document.getElementById("bpc-cur-style")) {
      var st = make("style");
      st.id = "bpc-cur-style";
      st.textContent = STYLE;
      document.head.appendChild(st);
    }
    rendered.push(el);
    if (wholeRead && whole) fill(el); else load();
  }

  // The page shows the chosen set from its first line (as the last page left it, or as kept before the database); once it has
  // loaded, the database is read for what it needs and the sets kept before it move in
  function start() {
    started = true;
    migrate();
    apply();
  }
  withdraw();
  apply();
  setTimeout(start, 0);
  window.addEventListener("storage", function (ev) {
    if (ev.key !== null && ev.key !== KEPT && ev.key !== SHOWN && ev.key !== CHOICE && ev.key !== UNKEPT && ev.key !== "byopop-settings" &&
      ev.key !== "byopop-game-files") return;
    if (ev.key === null || ev.key === KEPT) resync();
    apply();
    redraw();
  });
  return { KEY: KEY, CHOICE: CHOICE, UNKEPT: UNKEPT, KEPT: KEPT, SHOWN: SHOWN, GIVES: GIVES, all: all, has: has, unkept: unkept, save: save,
    forget: forget, chosen: chosen, choose: choose, apply: apply, render: render, refresh: redraw, busy: busy, crop: crop, picture: picture,
    codec: codec, vault: vault, load: load };
})();
