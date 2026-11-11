// Desktop themes (docs/WEB_PLAYER.md, "Desktop themes"; the user, 2026-11-11): Settings > Appearance's Theme gives the desktop one
// game's look, as a 1995 Desktop Themes pack did: the wallpaper one of the game's own backgrounds, the site's cursor the game's own
// (web/cursors.js, the set its page kept once the game had run) and the window events' sounds from its soundboard
// (web/soundboards.js). Every picture and sound is the player's own, read at run time from the game's files in this browser; the
// site ships none and sends nothing anywhere. A theme is offered for each game in THEMES whose files are here.
//
// The files can be read only where the game's folder is known, on the listing page (each tile's data-persist), so that page
// prepares the theme when it is chosen or the page loads: the background drawn once into a JPEG data URL (at most WALL_MAX pixels
// wide) and the event sounds' bytes (each at most SOUND_MAX bytes), kept in two localStorage items. Every Desktop 95 page then
// shows and plays them: byopop-theme-wallpaper {id, url} and byopop-theme-sounds {id, sounds: {event: {rel, data}}} (data: the
// file's bytes in base64). Choosing None, or forgetting the game's files, removes both.
//
// The events (web/skins.js and web/deskapps.js send "byopop-desk-event"): open and close (a window to and from its icon), minimize,
// restore, maximize, menu (a menu opens) and ding (a message box). The sounds play at the Sounds window's volume and mute
// (byopop-sounds-volume, byopop-sounds-mute; web/sounds.js), and never at ?silent=1.
//
// THEMES: by game id, {wallpaper: the background's path in the game's folder, sounds: {event: path}}, every path one the game's
// own code loads (its source names it), the sounds among its soundboard's.
var ByopopThemes = (function () {
  "use strict";
  var THEMES = (typeof ByopopThemeData !== "undefined") ? ByopopThemeData : {};
  var WALL_KEY = "byopop-theme-wallpaper", SOUND_KEY = "byopop-theme-sounds", WALL_MAX = 1280, SOUND_MAX = 98304;
  var EVENTS = ["open", "close", "minimize", "restore", "maximize", "menu", "ding"];
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
  // the chosen theme's cursor set: the game's id when its page kept one (web/cursors.js), else ""
  function cursor() {
    var id = chosen();
    if (id === "none") return "";
    try { var sets = ByopopCursors.all(); return sets[id] ? id : ""; } catch (e) { return ""; }
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
  function bytesOf(b64) { var s = atob(b64), out = new Uint8Array(s.length); for (var i = 0; i < s.length; i++) out[i] = s.charCodeAt(i); return out; }
  function play(what) {
    var id = chosen();
    if (id === "none" || level() === 0) return;
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
        if (window.ByopopSounds && ByopopSounds.decode) return ByopopSounds.decode(ctx, s.rel, bytes);
        throw e;
      });
      buffers[what].catch(function () { delete buffers[what]; });
    }
    buffers[what].then(function (buf) {
      var src = ctx.createBufferSource(), g = ctx.createGain();
      g.gain.value = level();
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
  function drawWallpaper(bytes, rel) {
    var type = /\.png$/i.test(rel) ? "image/png" : /\.gif$/i.test(rel) ? "image/gif" : "image/jpeg";
    return createImageBitmap(new Blob([bytes], { type: type })).then(function (bmp) {
      var k = Math.min(1, WALL_MAX / bmp.width), c = document.createElement("canvas");
      c.width = Math.round(bmp.width * k); c.height = Math.round(bmp.height * k);
      c.getContext("2d").drawImage(bmp, 0, 0, c.width, c.height);
      return c.toDataURL("image/jpeg", 0.9);
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
    var wall = t.wallpaper ? readFile(game, t.wallpaper).then(function (b) { return drawWallpaper(b, t.wallpaper); }).catch(function () { return ""; }) : Promise.resolve("");
    var sounds = {};
    var reads = Object.keys(t.sounds || {}).filter(function (e) { return EVENTS.indexOf(e) >= 0; }).map(function (e) {
      return readFile(game, t.sounds[e]).then(function (b) { if (b.length <= SOUND_MAX) sounds[e] = { rel: t.sounds[e], data: toBase64(b) }; }, function () {});
    });
    return Promise.all([wall].concat(reads)).then(function (r) {
      preparing = "";
      if (chosen() !== id) return false;
      writeJson(WALL_KEY, { id: id, url: r[0] || "" });
      if (!writeJson(SOUND_KEY, { id: id, sounds: sounds })) writeJson(SOUND_KEY, { id: id, sounds: {} });
      changed();
      return true;
    });
  }
  // what the theme shows changed: the desktop's backdrop and the cursor
  function changed() {
    try { ByopopSchemes.backdrop(); } catch (e) {}
    try { ByopopCursors.apply(); } catch (e) {}
  }
  if (typeof window !== "undefined") {
    window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "theme") { changed(); prepare(); } });
    if (document.readyState === "complete") setTimeout(prepare, 0); else window.addEventListener("load", function () { prepare(); });
  }
  if (typeof window !== "undefined") window.addEventListener("storage", function (ev) {
    if (ev.key === null || ev.key === WALL_KEY || ev.key === SOUND_KEY || ev.key === "byopop-settings") changed();
  });

  return { ids: ids, chosen: chosen, offered: offered, title: title, wallpaper: wallpaper, cursor: cursor, prepare: prepare, changed: changed,
    play: play, EVENTS: EVENTS, WALL_KEY: WALL_KEY, SOUND_KEY: SOUND_KEY };
})();
