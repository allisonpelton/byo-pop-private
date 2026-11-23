// Desktop themes (docs/WEB_PLAYER.md, "Desktop themes"; the user, 2026-11-11): Settings > Appearance's Theme gives the desktop one
// game's look, as a 1995 Desktop Themes pack did: the wallpaper one of the game's own backgrounds, the site's cursor the game's own
// (web/cursors.js, the set its page kept once the game had run) and the window events' sounds from its soundboard
// (web/soundboards.js). Every picture and sound is the player's own, read at run time from the game's files in this browser; the
// site ships none and sends nothing anywhere. A theme is offered for each game in THEMES whose files are here.
//
// The files can be read only where the game's folder is known, on the listing page (each tile's data-persist), so that page
// prepares the theme when it is chosen or the page loads: the background drawn once into a data URL (at its own size up to
// WALL_MAX pixels wide; a PNG for a picture without loss, a JPEG for a photograph) and the event sounds' bytes (each at most
// SOUND_MAX bytes), kept in two localStorage items. Every Desktop 95 page then shows and plays them: byopop-theme-wallpaper
// {id, url, w, h} and byopop-theme-sounds {id, sounds: {event: {rel, data}}} (data: the file's bytes in base64). Choosing None,
// or forgetting the game's files, removes both.
//
// The same background is also a wallpaper of its own, without the rest of the theme (the setting wallpaper "game:<id>", web/
// schemes.js; the user, 2026-11-18: "Still would like usable backgrounds from the games either as pixel perfect or fit to screen
// (no stretching)"): prepared the same way into byopop-game-wallpaper {id, url, w, h} when it is chosen (prepareWall).
//
// The events (web/skins.js and web/deskapps.js send "byopop-desk-event"): open and close (a window to and from its icon), minimize,
// restore, maximize, menu (a menu opens), ding (a message box), start (the desktop starting: the session's first press on the
// listing, and the end of Shut Down's last screen), exit (Shut Down), empty (Empty Recycle Bin) and error (a game stopped by an
// error, a folder refused). The sounds play at the Sounds window's volume and mute
// (byopop-sounds-volume, byopop-sounds-mute; web/sounds.js), and never at ?silent=1.
//
// THEMES: by game id, {wallpaper: the background's path in the game's folder, sounds: {event: path}}, every path one the game's
// own code loads (its source names it), the sounds among its soundboard's.
//
// The player's own sounds (the user, 2026-11-14, by the coordinator's list): Settings' Sound section lets the player pick, or drop,
// sound files from their own computer for any of the events. A file is matched to its event by its name, as a sound scheme's files
// were named ("Windows Minimize.wav", "recycle.wav", "tada.wav": OWN_NAMES), and each event can be given another file or none. They
// are kept in this browser only, in IndexedDB (byopop-own-sounds; a sound may be larger than localStorage holds), and nothing is
// uploaded; byopop-own-sounds-at changes with each change, so every open page reads them again. An event with the player's own
// sound plays it, whatever the theme; the others play the theme's.
var ByopopThemes = (function () {
  "use strict";
  var THEMES = (typeof ByopopThemeData !== "undefined") ? ByopopThemeData : {};
  var WALL_KEY = "byopop-theme-wallpaper", SOUND_KEY = "byopop-theme-sounds", WALL_MAX = 2048, SOUND_MAX = 98304;
  var GAME_WALL_KEY = "byopop-game-wallpaper", WALL_CHARS = 1600000;
  var EVENTS = ["open", "close", "minimize", "restore", "maximize", "menu", "ding", "start", "exit", "empty", "error"];
  var SILENT = /[?&]silent=1(&|$)/.test(location.search);
  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function readJson(key) { try { var s = store(), o = s ? JSON.parse(s.getItem(key) || "null") : null; return o && typeof o === "object" ? o : null; } catch (e) { return null; } }
  function writeJson(key, o) { try { var s = store(); if (!s) return false; if (o) s.setItem(key, JSON.stringify(o)); else s.removeItem(key); return true; } catch (e) { return false; } }

  var ids = ["none"].concat(Object.keys(THEMES));
  // the chosen theme's id, or "none"
  function chosen() { try { var v = ByopopSettings.get("theme"); return THEMES[v] ? v : "none"; } catch (e) { return "none"; } }
  // the games whose theme can be offered: their files are in this browser
  function offered() {
    return Object.keys(THEMES).filter(function (id) {
      try { return ByopopGameFiles.present(ByopopGameFiles.all()[id]); } catch (e) { return false; }
    });
  }
  function title(id) {
    var g = (window.ByopopGames || []).filter(function (x) { return x.id === id; })[0];
    return g ? g.title : id;
  }

  // ---- what every page uses ----
  // the prepared wallpaper of the chosen theme: its data URL, or ""
  function wallpaper() {
    var id = chosen(), w = readJson(WALL_KEY);
    return id !== "none" && w && w.id === id && typeof w.url === "string" && w.url.indexOf("data:image/") === 0 ? w.url : "";
  }
  // the game whose background is the wallpaper (the setting wallpaper "game:<id>"), or ""
  function wallGame() {
    try { var v = String(ByopopSettings.get("wallpaper") || ""), id = /^game:/.test(v) ? v.slice(5) : ""; return THEMES[id] && THEMES[id].wallpaper ? id : ""; }
    catch (e) { return ""; }
  }
  // its prepared picture: {url, w, h}, or null
  function gameWallpaper() {
    var id = wallGame(), w = readJson(GAME_WALL_KEY);
    return id && w && w.id === id && typeof w.url === "string" && w.url.indexOf("data:image/") === 0 ? { url: w.url, w: w.w || 0, h: w.h || 0 } : null;
  }
  // the chosen theme's cursor set: the game's id when its page kept one (web/cursors.js), else ""
  function cursor() {
    var id = chosen();
    if (id === "none") return "";
    try { return ByopopCursors.has(id) ? id : ""; } catch (e) { return ""; }
  }
  var ctx = null, buffers = {}, buffersOf = "";
  function level() {
    if (SILENT) return 0;
    try {
      if (localStorage.getItem("byopop-sounds-mute") === "1") return 0;
      var v = parseInt(localStorage.getItem("byopop-sounds-volume"), 10);
      return v >= 0 && v <= 100 ? v / 100 : 0.8;
    } catch (e) { return 0.8; }
  }
  // ---- the player's own sounds ----
  var OWN_DB = "byopop-own-sounds", OWN_STORE = "s", OWN_AT = "byopop-own-sounds-at", OWN_MAX = 4 * 1024 * 1024;
  // a file's name, without its folder and type, in lower case and without a leading "windows ", to its event
  var OWN_NAMES = {
    "open program": "open", "open": "open",
    "close program": "close", "close": "close",
    "minimize": "minimize", "minimise": "minimize",
    "restore up": "restore", "restore down": "restore", "restore": "restore",
    "maximize": "maximize", "maximise": "maximize",
    "menu popup": "menu", "menu command": "menu", "menu": "menu",
    "ding": "ding", "default": "ding", "default sound": "ding", "exclamation": "ding", "asterisk": "ding", "notify": "ding",
    "startup": "start", "start": "start", "logon": "start", "logon sound": "start", "tada": "start", "the microsoft sound": "start",
    "shutdown": "exit", "shut down": "exit", "exit": "exit", "logoff": "exit", "logoff sound": "exit",
    "recycle": "empty", "recycle bin": "empty", "empty recycle bin": "empty", "empty": "empty",
    "error": "error", "critical stop": "error", "chord": "error", "hardware fail": "error", "critical": "error"
  };
  function ownEvent(name) {
    var n = String(name || "").split(/[\/\\]/).pop().replace(/\.[a-z0-9]{1,5}$/i, "").toLowerCase().replace(/[_-]+/g, " ").replace(/\s+/g, " ").trim();
    n = n.replace(/^windows /, "");
    return Object.prototype.hasOwnProperty.call(OWN_NAMES, n) ? OWN_NAMES[n] : null;
  }
  var own = {}, ownBuffers = {}, ownLoaded = null;
  function ownDb() {
    return new Promise(function (resolve, reject) {
      if (typeof indexedDB === "undefined") { reject(new Error("no IndexedDB")); return; }
      var r = indexedDB.open(OWN_DB, 1);
      r.onupgradeneeded = function () { if (!r.result.objectStoreNames.contains(OWN_STORE)) r.result.createObjectStore(OWN_STORE); };
      r.onsuccess = function () { resolve(r.result); };
      r.onerror = function () { reject(r.error); };
    });
  }
  // reads every event's own sound: {event: {name, bytes}}
  function loadOwn() {
    ownLoaded = ownDb().then(function (db) {
      return new Promise(function (resolve) {
        var got = {}, tx = db.transaction(OWN_STORE, "readonly"), st = tx.objectStore(OWN_STORE), cur = st.openCursor();
        cur.onsuccess = function () {
          var c = cur.result;
          if (c) { if (EVENTS.indexOf(c.key) >= 0 && c.value && c.value.bytes) got[c.key] = c.value; c.continue(); }
        };
        tx.oncomplete = function () { db.close(); resolve(got); };
        tx.onerror = tx.onabort = function () { db.close(); resolve({}); };
      });
    }).catch(function () { return {}; }).then(function (got) { own = got; ownBuffers = {}; return got; });
    return ownLoaded;
  }
  function ownChanged() {
    try { localStorage.setItem(OWN_AT, String(Date.now())); } catch (e) {}
    try { window.dispatchEvent(new CustomEvent("byopop-own-sounds")); } catch (e) {}
  }
  // keeps a file (a File or Blob, with its name) as an event's sound; resolves with true, or false when it is too large or cannot
  // be kept
  function setOwn(what, file) {
    if (EVENTS.indexOf(what) < 0 || !file || file.size > OWN_MAX) return Promise.resolve(false);
    return file.arrayBuffer().then(function (buf) {
      return ownDb().then(function (db) {
        return new Promise(function (resolve) {
          var tx = db.transaction(OWN_STORE, "readwrite");
          tx.objectStore(OWN_STORE).put({ name: String(file.name || what), bytes: buf }, what);
          tx.oncomplete = function () { db.close(); resolve(true); };
          tx.onerror = tx.onabort = function () { db.close(); resolve(false); };
        });
      });
    }).catch(function () { return false; }).then(function (ok) { return loadOwn().then(function () { if (ok) ownChanged(); return ok; }); });
  }
  function clearOwn(what) {
    return ownDb().then(function (db) {
      return new Promise(function (resolve) {
        var tx = db.transaction(OWN_STORE, "readwrite");
        tx.objectStore(OWN_STORE).delete(what);
        tx.oncomplete = tx.onerror = tx.onabort = function () { db.close(); resolve(); };
      });
    }).catch(function () {}).then(function () { return loadOwn(); }).then(function () { ownChanged(); });
  }
  // files the player picked or dropped: each to the event its name matches. Resolves with {matched: [[event, name]], unmatched: [name]}
  function addOwnFiles(files) {
    var list = Array.prototype.slice.call(files || []), matched = [], unmatched = [];
    return list.reduce(function (p, f) {
      return p.then(function () {
        var ev = ownEvent(f.name);
        if (!ev) { unmatched.push(f.name); return; }
        return setOwn(ev, f).then(function (ok) { if (ok) matched.push([ev, f.name]); else unmatched.push(f.name); });
      });
    }, Promise.resolve()).then(function () { return { matched: matched, unmatched: unmatched }; });
  }
  function audio() {
    var AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return null;
    try { if (!ctx) ctx = new AC(); } catch (e) { return null; }
    if (ctx.state === "suspended") { try { ctx.resume(); } catch (e) {} }
    return ctx;
  }
  function sound(buf) {
    var src = ctx.createBufferSource(), g = ctx.createGain();
    g.gain.value = level() * (window.ByopopVolume ? ByopopVolume.level() : 1);
    src.buffer = buf; src.connect(g); g.connect(ctx.destination); src.start();
  }
  // a sound the browser cannot decode, by the soundboard's own decoders (web/sounds.js), whose script the listing loads only
  // when it is first needed (ByopopLazy, web/index.html)
  function soundsDecode(ctx, name, bytes, e) {
    var ready = window.ByopopSounds ? Promise.resolve() : window.ByopopLazy ? ByopopLazy.need("sounds") : Promise.reject(e);
    return ready.then(function () {
      if (window.ByopopSounds && ByopopSounds.decode) return ByopopSounds.decode(ctx, name, bytes);
      throw e;
    });
  }
  // plays an event's own sound: true when it has one
  function playOwn(what) {
    var o = own[what];
    if (!o) return false;
    if (!audio()) return true;
    if (!ownBuffers[what]) {
      ownBuffers[what] = ctx.decodeAudioData(o.bytes.slice(0)).catch(function (e) { return soundsDecode(ctx, o.name, new Uint8Array(o.bytes), e); });
      ownBuffers[what].catch(function () { delete ownBuffers[what]; });
    }
    ownBuffers[what].then(sound, function () {});
    return true;
  }
  // Settings' Sound section on the listing (web/index.html): each event, the file kept for it, Browse, a test press and Remove,
  // and a button and a drop for many files at once, matched by their names. T: the strings' lookup.
  function ownChooser(T) {
    var box = document.createElement("div"); box.className = "own-sounds";
    var head = document.createElement("div"); head.className = "bps-row own-sounds-head";
    var label = document.createElement("span"); label.textContent = T("settings.ownsounds.label");
    var many = document.createElement("input"); many.type = "file"; many.accept = "audio/*,.wav,.ogg,.mp3"; many.multiple = true; many.hidden = true;
    var add = document.createElement("button"); add.type = "button"; add.textContent = T("settings.ownsounds.add");
    var ctl = document.createElement("div"); ctl.className = "bps-ctl"; ctl.appendChild(add); ctl.appendChild(many);
    head.appendChild(label); head.appendChild(ctl);
    var hint = document.createElement("p"); hint.className = "own-sounds-hint"; hint.textContent = T("settings.ownsounds.hint");
    var said = document.createElement("p"); said.className = "own-sounds-said"; said.setAttribute("role", "status");
    var table = document.createElement("table"); table.className = "own-sounds-table";
    var body = document.createElement("tbody"); table.appendChild(body);
    box.appendChild(head); box.appendChild(hint); box.appendChild(table); box.appendChild(said);
    var rows = {};
    EVENTS.forEach(function (ev) {
      var tr = document.createElement("tr"), name = T("settings.ownsounds.ev." + ev);
      var th = document.createElement("th"); th.scope = "row"; th.textContent = name;
      var file = document.createElement("td"); file.className = "own-sounds-file";
      var acts = document.createElement("td");
      var input = document.createElement("input"); input.type = "file"; input.accept = "audio/*,.wav,.ogg,.mp3"; input.hidden = true;
      var browse = document.createElement("button"); browse.type = "button"; browse.textContent = T("settings.wallpaper.browse");
      browse.setAttribute("aria-label", T("settings.ownsounds.browse_for", { event: name }));
      var test = document.createElement("button"); test.type = "button"; test.className = "own-sounds-play"; test.textContent = "\u25b6";
      test.setAttribute("aria-label", T("settings.ownsounds.play_for", { event: name }));
      var remove = document.createElement("button"); remove.type = "button"; remove.textContent = T("settings.wallpaper.remove");
      remove.setAttribute("aria-label", T("settings.ownsounds.remove_for", { event: name }));
      browse.addEventListener("click", function () { input.click(); });
      input.addEventListener("change", function () {
        var f = input.files && input.files[0];
        input.value = "";
        if (f) setOwn(ev, f).then(function (ok) { said.textContent = ok ? "" : T("settings.ownsounds.too_big", { name: f.name }); draw(); });
      });
      test.addEventListener("click", function () { if (!playOwn(ev)) play(ev); });
      remove.addEventListener("click", function () { clearOwn(ev).then(draw); });
      acts.appendChild(browse); acts.appendChild(test); acts.appendChild(remove); acts.appendChild(input);
      tr.appendChild(th); tr.appendChild(file); tr.appendChild(acts);
      body.appendChild(tr);
      rows[ev] = { file: file, remove: remove, test: test };
    });
    function draw() {
      EVENTS.forEach(function (ev) {
        var o = own[ev], r = rows[ev];
        r.file.textContent = o ? o.name : T("settings.theme.none");
        r.remove.disabled = !o;
      });
    }
    function took(files) {
      addOwnFiles(files).then(function (r) {
        var parts = [];
        if (r.matched.length) parts.push(T("settings.ownsounds.matched", { count: r.matched.length }));
        if (r.unmatched.length) parts.push(T("settings.ownsounds.unmatched", { names: r.unmatched.join(", ") }));
        said.textContent = parts.join(" ");
        draw();
      });
    }
    add.addEventListener("click", function () { many.click(); });
    many.addEventListener("change", function () { var f = Array.prototype.slice.call(many.files || []); many.value = ""; if (f.length) took(f); });
    // files dropped on the section: as Add files
    box.addEventListener("dragover", function (e) {
      if (e.dataTransfer && Array.prototype.indexOf.call(e.dataTransfer.types || [], "Files") >= 0) { e.preventDefault(); e.stopPropagation(); box.classList.add("own-sounds-drop"); }
    });
    box.addEventListener("dragleave", function () { box.classList.remove("own-sounds-drop"); });
    box.addEventListener("drop", function (e) {
      box.classList.remove("own-sounds-drop");
      if (!e.dataTransfer || !e.dataTransfer.files || !e.dataTransfer.files.length) return;
      e.preventDefault(); e.stopPropagation();
      took(e.dataTransfer.files);
    });
    window.addEventListener("byopop-own-sounds", draw);
    draw();
    (ownLoaded || loadOwn()).then(draw);
    return box;
  }

  function bytesOf(b64) { var s = atob(b64), out = new Uint8Array(s.length); for (var i = 0; i < s.length; i++) out[i] = s.charCodeAt(i); return out; }
  function play(what) {
    if (level() === 0) return;
    if (playOwn(what)) return;
    var id = chosen();
    if (id === "none") return;
    var kept = readJson(SOUND_KEY);
    if (!kept || kept.id !== id || !kept.sounds || !kept.sounds[what]) return;
    var AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    try { if (!ctx) ctx = new AC(); } catch (e) { return; }
    if (ctx.state === "suspended") { try { ctx.resume(); } catch (e) {} }
    if (buffersOf !== id) { buffers = {}; buffersOf = id; }
    var s = kept.sounds[what];
    if (!buffers[what]) {
      var bytes = bytesOf(s.data);
      buffers[what] = (/\.au$/i.test(s.rel) ? Promise.reject(new Error("au")) : ctx.decodeAudioData(bytes.buffer)).catch(function (e) {
        // an Ogg file the browser cannot decode, and a Sun .au file: the soundboard's own decoders (web/sounds.js)
        return soundsDecode(ctx, s.rel, bytes, e);
      });
      buffers[what].catch(function () { delete buffers[what]; });
    }
    buffers[what].then(function (buf) {
      var src = ctx.createBufferSource(), g = ctx.createGain();
      g.gain.value = level() * (window.ByopopVolume ? ByopopVolume.level() : 1);
      src.buffer = buf; src.connect(g); g.connect(ctx.destination); src.start();
    }, function () {});
  }
  if (typeof window !== "undefined") window.addEventListener("byopop-desk-event", function (ev) {
    var what = ev.detail && ev.detail.what;
    if (EVENTS.indexOf(what) >= 0) play(what);
  });

  // ---- the listing page: preparing the chosen theme from the game's files ----
  function gameOf(id) {
    var t = document.querySelector('a.tile[data-game="' + String(id).replace(/[^a-z0-9_-]/g, "") + '"]');
    return t ? { id: id, persist: t.getAttribute("data-persist") } : null;
  }
  function readFile(game, rel) {
    var F = ByopopMusic.files;
    return F.open(game.persist).then(function (db) {
      if (!db) throw new Error("no database");
      // the folder's own spelling of the name (the table's paths are matched without regard to case)
      return F.list(db, game.persist).then(function (rels) {
        var bar = rel.indexOf("|"), file = bar < 0 ? rel : rel.slice(0, bar), lower = file.toLowerCase();
        var found = rels.filter(function (r) { return r.toLowerCase() === lower; })[0];
        if (!found) { db.close(); throw new Error("no file"); }
        return F.read(db, game.persist + "/game/" + found + (bar < 0 ? "" : rel.slice(bar)));
      }).then(function (b) { db.close(); if (!b) throw new Error("no file"); return b; }, function (e) { db.close(); throw e; });
    });
  }
  function toBase64(bytes) {
    var s = "";
    for (var i = 0; i < bytes.length; i += 0x8000) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
    return btoa(s);
  }
  // the wallpaper's picture: a file the browser decodes, or one web/gamepics.js decodes: "<bank>.dat|bbk:<name>", an image of a
  // Retro64 bank with its .bbk index beside it, or a .jp2
  function readWallpaper(game, rel) {
    var bbk = rel.indexOf("|bbk:"), G = typeof ByopopGamePics !== "undefined" ? ByopopGamePics : null;
    var picture;
    if (bbk >= 0) {
      var dat = rel.slice(0, bbk);
      picture = Promise.all([readFile(game, dat.replace(/\.dat$/i, ".bbk")), readFile(game, dat)]).then(function (r) {
        return G ? G.bbk(r[0], r[1], rel.slice(bbk + 5)) : null;
      });
    } else if (/\.jp2$/i.test(rel)) {
      picture = readFile(game, rel).then(function (b) { return G ? G.jp2(b) : null; });
    } else {
      return readFile(game, rel).then(function (b) { return drawWallpaper(b, rel, /\.(png|gif)$/i.test(rel)); });
    }
    return picture.then(function (pic) {
      if (!pic) throw new Error("no picture");
      var c = document.createElement("canvas");
      c.width = pic.width; c.height = pic.height;
      c.getContext("2d").putImageData(new ImageData(new Uint8ClampedArray(pic.rgba.buffer, pic.rgba.byteOffset, pic.rgba.length), pic.width, pic.height), 0, 0);
      // a Retro64 bank's image is drawn art, kept without loss; a JPEG 2000 picture is a photograph's
      return drawWallpaper(c, rel, bbk >= 0);
    });
  }
  // The picture as a data URL with its size: at its own size up to WALL_MAX wide, so that it can be shown pixel-perfect; a PNG
  // when the picture had no loss (drawn art), else a JPEG; and smaller (a JPEG, then a narrower one) when that is too long for
  // the browser's storage, which the saves' records share
  function drawWallpaper(bytes, rel, lossless) {
    var type = /\.png$/i.test(rel) ? "image/png" : /\.gif$/i.test(rel) ? "image/gif" : "image/jpeg";
    return (bytes instanceof HTMLCanvasElement ? Promise.resolve(bytes) : createImageBitmap(new Blob([bytes], { type: type }))).then(function (bmp) {
      var tries = [[WALL_MAX, lossless ? "png" : 0.92], [WALL_MAX, 0.85], [1280, 0.85]], out = null;
      for (var i = 0; i < tries.length && !out; i++) {
        var k = Math.min(1, tries[i][0] / bmp.width), c = document.createElement("canvas");
        c.width = Math.round(bmp.width * k); c.height = Math.round(bmp.height * k);
        c.getContext("2d").drawImage(bmp, 0, 0, c.width, c.height);
        var url = tries[i][1] === "png" ? c.toDataURL("image/png") : c.toDataURL("image/jpeg", tries[i][1]);
        if (url.length <= WALL_CHARS || i === tries.length - 1) out = { url: url, w: c.width, h: c.height };
      }
      return out;
    });
  }
  var preparing = "";
  // Prepares the chosen theme when what is kept is another's (or none). Resolves when done; a part the files lack is left out.
  function prepare() {
    var id = chosen();
    // none, or a game whose files are gone: nothing kept
    if (id === "none" || offered().indexOf(id) < 0) {
      if (readJson(WALL_KEY) || readJson(SOUND_KEY)) { writeJson(WALL_KEY, null); writeJson(SOUND_KEY, null); changed(); }
      return Promise.resolve(false);
    }
    var w = readJson(WALL_KEY), s = readJson(SOUND_KEY);
    if (w && w.id === id && s && s.id === id) return Promise.resolve(true);
    if (preparing === id || typeof ByopopMusic === "undefined" || !ByopopMusic.files) return Promise.resolve(false);
    var game = gameOf(id), t = THEMES[id];
    if (!game) return Promise.resolve(false);
    preparing = id;
    var wall = t.wallpaper ? readWallpaper(game, t.wallpaper).catch(function () { return null; }) : Promise.resolve(null);
    var sounds = {};
    var reads = Object.keys(t.sounds || {}).filter(function (e) { return EVENTS.indexOf(e) >= 0; }).map(function (e) {
      return readFile(game, t.sounds[e]).then(function (b) { if (b.length <= SOUND_MAX) sounds[e] = { rel: t.sounds[e], data: toBase64(b) }; }, function () {});
    });
    return Promise.all([wall].concat(reads)).then(function (r) {
      preparing = "";
      if (chosen() !== id) return false;
      var pic = r[0];
      if (!writeJson(WALL_KEY, { id: id, url: pic ? pic.url : "", w: pic ? pic.w : 0, h: pic ? pic.h : 0 })) writeJson(WALL_KEY, { id: id, url: "" });
      if (!writeJson(SOUND_KEY, { id: id, sounds: sounds })) writeJson(SOUND_KEY, { id: id, sounds: {} });
      changed();
      return true;
    });
  }
  // Prepares the game's background chosen as the wallpaper, as prepare does the theme's; removes it when another (or none) is
  // chosen or the game's files went
  var preparingWall = "";
  function prepareWall() {
    var id = wallGame(), kept = readJson(GAME_WALL_KEY);
    if (!id || offered().indexOf(id) < 0) {
      if (kept) { writeJson(GAME_WALL_KEY, null); changed(); }
      return Promise.resolve(false);
    }
    if (kept && kept.id === id) return Promise.resolve(true);
    if (preparingWall === id || typeof ByopopMusic === "undefined" || !ByopopMusic.files) return Promise.resolve(false);
    var game = gameOf(id);
    if (!game) return Promise.resolve(false);
    preparingWall = id;
    return readWallpaper(game, THEMES[id].wallpaper).catch(function () { return null; }).then(function (pic) {
      preparingWall = "";
      if (wallGame() !== id) return false;
      var ok = !!pic && writeJson(GAME_WALL_KEY, { id: id, url: pic.url, w: pic.w, h: pic.h });
      if (!ok) writeJson(GAME_WALL_KEY, null);
      changed();
      return ok;
    });
  }
  // what the theme shows changed: the desktop's backdrop and the cursor
  function changed() {
    try { ByopopSchemes.backdrop(); } catch (e) {}
    try { ByopopCursors.apply(); } catch (e) {}
  }
  if (typeof window !== "undefined") {
    window.addEventListener("byopop-setting", function (ev) {
      if (ev.detail && ev.detail.name === "theme") { changed(); prepare(); }
      if (ev.detail && ev.detail.name === "wallpaper") prepareWall();
    });
    if (document.readyState === "complete") setTimeout(function () { prepare(); prepareWall(); }, 0);
    else window.addEventListener("load", function () { prepare(); prepareWall(); });
  }
  if (typeof window !== "undefined") window.addEventListener("storage", function (ev) {
    if (ev.key === null || ev.key === WALL_KEY || ev.key === SOUND_KEY || ev.key === GAME_WALL_KEY || ev.key === "byopop-settings") changed();
    if (ev.key === null || ev.key === OWN_AT) loadOwn().then(function () { try { window.dispatchEvent(new CustomEvent("byopop-own-sounds")); } catch (e) {} });
  });
  if (typeof window !== "undefined" && typeof indexedDB !== "undefined") loadOwn();

  return { ids: ids, chosen: chosen, offered: offered, title: title, wallpaper: wallpaper, gameWallpaper: gameWallpaper, prepareWall: prepareWall,
    GAME_WALL_KEY: GAME_WALL_KEY, cursor: cursor, prepare: prepare, changed: changed,
    play: play, EVENTS: EVENTS, WALL_KEY: WALL_KEY, SOUND_KEY: SOUND_KEY, ownEvent: ownEvent, setOwn: setOwn, clearOwn: clearOwn,
    addOwnFiles: addOwnFiles, ownChooser: ownChooser, own: function () { return own; } };
})();
