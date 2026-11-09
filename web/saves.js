// The listing page's Saves tab: a backup of the player's saves in one .zip, and a restore from one (docs/WEB_PLAYER.md, "Save
// backup"). Nothing is uploaded: the page reads the games' browser storage itself and writes the zip itself.
//
// What the backup holds: for each game, the files of its save folder (the game's persist folder + "/userdata": its saves, high
// scores, settings and registry; and "/replays", for the games that keep replays) as the game would find them at its next start,
// and the game's own backup of them in localStorage ("sexy-userdata-backup:<persist>/userdata", platform/WebPersist.cpp), then the
// site's settings (byopop-settings), players (byopop-profiles) and play times (byopop-play). Never a game's files (its /game folder, the exe), the icons
// or the mods. The zip is not compressed ("store"), with a manifest, byopop-saves.json, that gives each file's date.
//
//   byopop-saves.json            {format: "byo-pop saves", version: 1, made, games: {<id>: {title, files, bytes, replays, ...}}, files: {path: ms},
//                                play: the item byopop-play (restored with this browser's, mergedPlay)}
//   byopop-settings.json         the item byopop-settings as it was
//   byopop-profiles.json         the item byopop-profiles as it was
//   <id>/userdata/<path>         a file of the save folder
//   <id>/replays/<path>          a replay
//   <id>/browser-backup.json     the game's localStorage backup item, as it was
//
// A restore reads such a zip and shows, for each game, what it holds and what it would replace. It writes only after the player's
// yes, refuses while any game page is open (each page holds a Web Lock, PAGE.lock, while it runs), and refuses a zip that has a
// path outside a game's save folder or a game that is not listed.
//
// The pure parts (the zip, the paths, the merge of a game's files with its localStorage backup) are tested under Node
// (tools/savestest.js); the browser parts read and write the games' IndexedDB databases, which Emscripten's IDBFS made: one
// object store, FILE_DATA, whose keys are paths and whose values are {timestamp, mode, contents}.
var ByopopSaves = (function () {
  "use strict";
  var FORMAT = "byo-pop saves", VERSION = 1;
  var MAX_ENTRIES = 20000, MAX_PATH = 240;
  var DB_STORE = "FILE_DATA", DB_VERSION = 21;
  var FILE_MODE = 33206, DIR_MODE = 16895;   // 0100666, 040777

  // ---------- the zip (store only) ----------
  var crcTable = null;
  function crc32(bytes) {
    if (!crcTable) {
      crcTable = new Uint32Array(256);
      for (var n = 0; n < 256; n++) {
        var c = n;
        for (var k = 0; k < 8; k++) c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
        crcTable[n] = c >>> 0;
      }
    }
    var crc = 0xffffffff;
    for (var i = 0; i < bytes.length; i++) crc = crcTable[(crc ^ bytes[i]) & 255] ^ (crc >>> 8);
    return (crc ^ 0xffffffff) >>> 0;
  }
  function dosTime(d) { return ((d.getHours() << 11) | (d.getMinutes() << 5) | (d.getSeconds() >> 1)) & 0xffff; }
  function dosDate(d) { return ((((d.getFullYear() - 1980) << 9) | ((d.getMonth() + 1) << 5) | d.getDate()) & 0xffff); }
  // entries: [{name, bytes}] (names are UTF-8). Returns a Uint8Array: local headers and data, the central directory, its end.
  function writeZip(entries, when) {
    var d = when || new Date(), time = dosTime(d), date = dosDate(d), enc = new TextEncoder();
    var parts = [], central = [], offset = 0;
    entries.forEach(function (e) {
      var name = enc.encode(e.name), crc = crc32(e.bytes), size = e.bytes.length;
      var local = new Uint8Array(30 + name.length), lv = new DataView(local.buffer);
      lv.setUint32(0, 0x04034b50, true); lv.setUint16(4, 20, true); lv.setUint16(6, 0x0800, true); lv.setUint16(8, 0, true);
      lv.setUint16(10, time, true); lv.setUint16(12, date, true); lv.setUint32(14, crc, true); lv.setUint32(18, size, true);
      lv.setUint32(22, size, true); lv.setUint16(26, name.length, true); lv.setUint16(28, 0, true);
      local.set(name, 30);
      var cd = new Uint8Array(46 + name.length), cv = new DataView(cd.buffer);
      cv.setUint32(0, 0x02014b50, true); cv.setUint16(4, 20, true); cv.setUint16(6, 20, true); cv.setUint16(8, 0x0800, true);
      cv.setUint16(10, 0, true); cv.setUint16(12, time, true); cv.setUint16(14, date, true); cv.setUint32(16, crc, true);
      cv.setUint32(20, size, true); cv.setUint32(24, size, true); cv.setUint16(28, name.length, true);
      cv.setUint32(42, offset, true);
      cd.set(name, 46);
      parts.push(local, e.bytes);
      central.push(cd);
      offset += local.length + size;
    });
    var cdSize = 0;
    central.forEach(function (c) { cdSize += c.length; });
    var end = new Uint8Array(22), ev = new DataView(end.buffer);
    ev.setUint32(0, 0x06054b50, true); ev.setUint16(8, entries.length, true); ev.setUint16(10, entries.length, true);
    ev.setUint32(12, cdSize, true); ev.setUint32(16, offset, true);
    var all = parts.concat(central, [end]), total = 0;
    all.forEach(function (p) { total += p.length; });
    var out = new Uint8Array(total), at = 0;
    all.forEach(function (p) { out.set(p, at); at += p.length; });
    return out;
  }
  // Reads a zip into [{name, read()}] (read: a promise of the bytes); stored and deflated entries; throws a message for a file that
  // is no zip, a damaged one, an encrypted entry or one this cannot read (zip64)
  function readZip(bytes) {
    var view = new DataView(bytes.buffer, bytes.byteOffset, bytes.length), eocd = -1;
    for (var i = bytes.length - 22; i >= Math.max(0, bytes.length - 65557); i--) {
      if (view.getUint32(i, true) === 0x06054b50) { eocd = i; break; }
    }
    if (eocd < 0) throw new Error(ByopopStrings.t("saves.zip.not_zip"));
    var count = view.getUint16(eocd + 10, true), off = view.getUint32(eocd + 16, true);
    if (count === 0xffff || off === 0xffffffff) throw new Error(ByopopStrings.t("saves.zip.zip64"));
    if (count > MAX_ENTRIES) throw new Error(ByopopStrings.t("saves.zip.too_many"));
    var dec = new TextDecoder("utf-8", { fatal: false }), entries = [];
    for (var n = 0; n < count; n++) {
      if (off + 46 > bytes.length || view.getUint32(off, true) !== 0x02014b50) throw new Error(ByopopStrings.t("saves.zip.directory"));
      var flags = view.getUint16(off + 8, true), method = view.getUint16(off + 10, true), crc = view.getUint32(off + 16, true);
      var csize = view.getUint32(off + 20, true), size = view.getUint32(off + 24, true);
      var nameLen = view.getUint16(off + 28, true), extraLen = view.getUint16(off + 30, true), commentLen = view.getUint16(off + 32, true);
      var local = view.getUint32(off + 42, true);
      if (off + 46 + nameLen > bytes.length) throw new Error(ByopopStrings.t("saves.zip.directory"));
      var name = dec.decode(bytes.subarray(off + 46, off + 46 + nameLen));
      if (flags & 1) throw new Error(ByopopStrings.t("saves.zip.encrypted"));
      entries.push({ name: name, method: method, crc: crc, csize: csize, size: size, local: local });
      off += 46 + nameLen + extraLen + commentLen;
    }
    return entries.filter(function (e) { return !/\/$/.test(e.name); }).map(function (e) {
      return {
        name: e.name,
        size: e.size,
        read: function () {
          if (e.local + 30 > bytes.length || view.getUint32(e.local, true) !== 0x04034b50) return Promise.reject(new Error(ByopopStrings.t("saves.zip.damaged")));
          var start = e.local + 30 + view.getUint16(e.local + 26, true) + view.getUint16(e.local + 28, true);
          if (start + e.csize > bytes.length) return Promise.reject(new Error(ByopopStrings.t("saves.zip.short")));
          var data = bytes.subarray(start, start + e.csize);
          function check(out) {
            if (out.length !== e.size || crc32(out) !== e.crc) throw new Error(ByopopStrings.t("saves.zip.checksum", {name: e.name}));
            return out;
          }
          if (e.method === 0) return Promise.resolve(check(data.slice()));
          if (e.method !== 8 || typeof DecompressionStream === "undefined") return Promise.reject(new Error(ByopopStrings.t("saves.zip.compressed", {name: e.name})));
          return new Response(new Blob([data]).stream().pipeThrough(new DecompressionStream("deflate-raw"))).arrayBuffer()
            .then(function (b) { return check(new Uint8Array(b)); });
        }
      };
    });
  }

  // ---------- paths ----------
  // A path inside a save folder: relative, no empty, "." or ".." parts, no backslash or control character, not too long
  function safeRel(rel) {
    if (typeof rel !== "string" || !rel || rel.length > MAX_PATH) return false;
    if (/[\\\u0000-\u001f]/.test(rel) || rel.charAt(0) === "/" || rel.charAt(rel.length - 1) === "/") return false;
    return rel.split("/").every(function (p) { return p !== "" && p !== "." && p !== ".."; });
  }
  // The play times ride in the manifest, not in a file of their own: a build from before them refuses a zip with a top-level name it
  // does not know, and reads past a manifest field it does not know. For the same reason a top-level byopop-<name>.json that this
  // build does not know (one a later build adds) is skipped, not refused.
  // Sorts a zip's entries: {ok: true, manifest, settings, profiles, files: [{id, kind, rel, entry}], backups: {id: entry}} or
  // {ok: false, error}. gameIds: the listed games. Anything else is refused: a path outside a game's save folder, an unknown game.
  function classify(entries, gameIds) {
    var out = { ok: true, manifest: null, settings: null, profiles: null, files: [], backups: {}, skipped: [] }, seen = {};
    for (var i = 0; i < entries.length; i++) {
      var name = entries[i].name;
      if (seen[name]) return { ok: false, error: ByopopStrings.t("saves.zip.twice", {name: name}) };
      seen[name] = true;
      if (name === "byopop-saves.json") { out.manifest = entries[i]; continue; }
      if (name === "byopop-settings.json") { out.settings = entries[i]; continue; }
      if (name === "byopop-profiles.json") { out.profiles = entries[i]; continue; }
      if (/^byopop-[a-z0-9_-]{1,40}\.json$/.test(name)) { out.skipped.push(name); continue; }
      var m = /^([a-z0-9_-]{1,40})\/(userdata|replays)\/(.+)$/.exec(name), b = /^([a-z0-9_-]{1,40})\/browser-backup\.json$/.exec(name);
      if (b && gameIds.indexOf(b[1]) >= 0) { out.backups[b[1]] = entries[i]; continue; }
      if (!m || gameIds.indexOf(m[1]) < 0 || !safeRel(m[3])) {
        return { ok: false, error: ByopopStrings.t("saves.zip.stray", {name: name.slice(0, 80)}) };
      }
      out.files.push({ id: m[1], kind: m[2], rel: m[3], entry: entries[i] });
    }
    return out;
  }

  // ---------- a game's files as the game would have them ----------
  // files: {rel: {bytes, m}} (the IDBFS database's save folder; m: the file's time in ms); item: the game's localStorage backup item
  // (the text), or null. The same as webpersist_restore does at the game's next start (platform/WebPersist.cpp): a backup file
  // that is newer than the file, or that the folder lacks, is written; a file that the backup does not list and that is older than
  // the backup was deleted before it. Returns {files, used: how many files came from the item, dropped: how many it removed}.
  function merged(files, item) {
    var out = {}, used = 0, dropped = 0, backup = null;
    Object.keys(files).forEach(function (k) { out[k] = files[k]; });
    try { backup = item ? JSON.parse(item) : null; } catch (e) { backup = null; }
    if (!backup || backup.v !== 1 || typeof backup.files !== "object" || !backup.files || typeof backup.time !== "number") return { files: out, used: 0, dropped: 0 };
    var listed = backup.files;
    Object.keys(listed).forEach(function (rel) {
      var f = listed[rel];
      if (!f || typeof f.d !== "string" || typeof f.m !== "number" || !safeRel(rel)) return;
      var have = out[rel] ? out[rel].m : -1;
      if (f.m <= have) return;
      try {
        var bin = atob(f.d), b = new Uint8Array(bin.length);
        for (var i = 0; i < bin.length; i++) b[i] = bin.charCodeAt(i);
        out[rel] = { bytes: b, m: f.m };
        used++;
      } catch (e) {}
    });
    Object.keys(out).forEach(function (rel) {
      if (!Object.prototype.hasOwnProperty.call(listed, rel) && out[rel].m < backup.time) { delete out[rel]; dropped++; }
    });
    return { files: out, used: used, dropped: dropped };
  }

  function dateStamp(d) {
    d = d || new Date();
    function two(n) { return (n < 10 ? "0" : "") + n; }
    return d.getFullYear() + "-" + two(d.getMonth() + 1) + "-" + two(d.getDate());
  }
  // The play times of a backup (byopop-play, web/settings.js) with this browser's: for each game the larger count of plays and of
  // seconds and the later day, so that restoring an older backup takes nothing away. Texts in, text out; a text that is not an
  // object counts as none.
  function mergedPlay(mine, theirs) {
    function parse(t) { try { var o = JSON.parse(t || "{}"); return o && typeof o === "object" && !Array.isArray(o) ? o : {}; } catch (e) { return {}; } }
    function num(n) { return typeof n === "number" && isFinite(n) && n > 0 ? Math.floor(n) : 0; }
    function day(d) { return typeof d === "string" && /^\d{4}-\d{2}-\d{2}$/.test(d) ? d : ""; }
    var a = parse(mine), b = parse(theirs), out = {};
    Object.keys(a).concat(Object.keys(b)).forEach(function (id) {
      if (!/^[a-z0-9_-]{1,40}$/.test(id) || out[id]) return;
      var x = a[id] || {}, y = b[id] || {}, dx = day(x.last), dy = day(y.last);
      out[id] = { plays: Math.max(num(x.plays), num(y.plays)), seconds: Math.max(num(x.seconds), num(y.seconds)), last: dx > dy ? dx : dy };
    });
    return JSON.stringify(out);
  }
  // The item byopop-play as the manifest's field play (an object, or null to leave the field out), and back: the text a restore
  // merges, or null when the manifest has none
  function playToManifest(item) {
    try { var o = JSON.parse(item); return o && typeof o === "object" && !Array.isArray(o) ? o : null; } catch (e) { return null; }
  }
  function playFromManifest(man) {
    return man && man.play && typeof man.play === "object" && !Array.isArray(man.play) ? JSON.stringify(man.play) : null;
  }
  function backupName(d) { return ByopopStrings.t("saves.backup_name", {date: dateStamp(d)}); }
  // n things, in the count string key ("saves.n.file": "{n} file | {n} files")
  function plural(n, key) { return ByopopStrings.t(key, {n: n}); }
  function size(n) {
    return n < 1024 ? ByopopStrings.t("size.bytes", {value: n}) : n < 1048576 ? ByopopStrings.t("size.kb", {value: (n / 1024).toFixed(n < 10240 ? 1 : 0)})
      : ByopopStrings.t("size.mb", {value: (n / 1048576).toFixed(1)});
  }

  var core = { crc32: crc32, writeZip: writeZip, readZip: readZip, safeRel: safeRel, classify: classify, merged: merged, mergedPlay: mergedPlay, playToManifest: playToManifest, playFromManifest: playFromManifest, backupName: backupName,
    size: size, FORMAT: FORMAT, VERSION: VERSION };
  if (typeof document === "undefined") return core;

  // ---------- IndexedDB (IDBFS's databases) ----------
  function ls() { try { return window.localStorage || null; } catch (e) { return null; } }
  function lsGet(key) { try { var s = ls(); return s ? s.getItem(key) : null; } catch (e) { return null; } }
  function lsSet(key, value) { try { var s = ls(); if (!s) return false; if (value === null) s.removeItem(key); else s.setItem(key, value); return true; } catch (e) { return false; } }
  function backupKey(persist) { return "sexy-userdata-backup:" + persist + "/userdata"; }
  function request(r) { return new Promise(function (resolve, reject) { r.onsuccess = function () { resolve(r.result); }; r.onerror = function () { reject(r.error); }; }); }
  function databases() {
    if (!(window.indexedDB && typeof indexedDB.databases === "function")) return Promise.resolve(null);
    return indexedDB.databases().then(function (l) { return l.map(function (d) { return d.name; }); }, function () { return null; });
  }
  // Opens a game's database without making one: null when there is none (or this browser cannot tell)
  function openExisting(persist, names) {
    if (!names || names.indexOf(persist) < 0) return Promise.resolve(null);
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
  // The files of a save folder ("userdata" or "replays") of an open database: {rel: {bytes, m}}
  function readFolder(db, persist, folder) {
    var prefix = persist + "/" + folder + "/", out = {};
    return new Promise(function (resolve, reject) {
      var tx = db.transaction(DB_STORE, "readonly"), c = tx.objectStore(DB_STORE).openCursor(IDBKeyRange.bound(prefix, prefix + "￿"));
      c.onerror = function () { reject(c.error); };
      c.onsuccess = function () {
        var at = c.result;
        if (!at) { resolve(out); return; }
        var v = at.value;
        if (v && v.contents && ((v.mode & 0xf000) === 0x8000 || v.mode === undefined)) {
          var rel = String(at.key).substring(prefix.length);
          if (safeRel(rel)) out[rel] = { bytes: new Uint8Array(v.contents), m: v.timestamp && v.timestamp.getTime ? v.timestamp.getTime() : Date.now() };
        }
        at.continue();
      };
    });
  }

  // What each game has in this browser: {files, bytes, replays, replayBytes, backup} (null: no database)
  function overview(game, names) {
    return openExisting(game.persist, names).then(function (db) {
      var item = lsGet(backupKey(game.persist));
      if (!db) return item ? { files: 0, bytes: 0, replays: 0, replayBytes: 0, backup: true } : null;
      return Promise.all([readFolder(db, game.persist, "userdata"), game.replays ? readFolder(db, game.persist, "replays") : Promise.resolve({})]).then(function (r) {
        db.close();
        var m = merged(r[0], item), o = { files: 0, bytes: 0, replays: 0, replayBytes: 0, backup: !!item };
        Object.keys(m.files).forEach(function (k) { o.files++; o.bytes += m.files[k].bytes.length; });
        Object.keys(r[1]).forEach(function (k) { o.replays++; o.replayBytes += r[1][k].bytes.length; });
        return o;
      }, function () { try { db.close(); } catch (e) {} return null; });
    });
  }

  // Writes files into a game's save folders in one transaction: {userdata: {rel: {bytes, m}}, replays: {...}}. The database is made if
  // the game has never run (the way IDBFS makes it), and a folder's missing parents get the directory entries IDBFS needs.
  function writeFiles(game, names, folders) {
    return new Promise(function (resolve, reject) {
      var known = names && names.indexOf(game.persist) >= 0, r;
      try { r = known ? indexedDB.open(game.persist) : indexedDB.open(game.persist, DB_VERSION); } catch (e) { reject(e); return; }
      r.onerror = function () { reject(r.error || new Error(ByopopStrings.t("saves.err.open"))); };
      r.onblocked = function () { reject(new Error(ByopopStrings.t("saves.err.busy"))); };
      r.onupgradeneeded = function (ev) {
        var db = ev.target.result, store = db.objectStoreNames.contains(DB_STORE) ? ev.target.transaction.objectStore(DB_STORE) : db.createObjectStore(DB_STORE);
        if (!store.indexNames.contains("timestamp")) store.createIndex("timestamp", "timestamp", { unique: false });
      };
      r.onsuccess = function () {
        var db = r.result;
        if (!db.objectStoreNames.contains(DB_STORE)) { db.close(); reject(new Error(ByopopStrings.t("saves.err.form"))); return; }
        var tx = db.transaction(DB_STORE, "readwrite"), store = tx.objectStore(DB_STORE), dirs = {}, written = 0, now = new Date();
        function ensureDir(path) {   // path: "/persist/userdata/players"
          if (dirs[path] !== undefined) return;
          dirs[path] = true;
          var g = store.get(path);
          g.onsuccess = function () {
            if (g.result === undefined) store.put({ timestamp: now, mode: DIR_MODE }, path);
          };
          var up = path.lastIndexOf("/");
          if (up > 0) ensureDir(path.substring(0, up));
        }
        ["userdata", "replays"].forEach(function (folder) {
          var files = folders[folder] || {};
          var root = game.persist + "/" + folder;
          Object.keys(files).forEach(function (rel) {
            var key = root + "/" + rel, up = key.lastIndexOf("/");
            ensureDir(key.substring(0, up));
            store.put({ timestamp: new Date(files[rel].m || Date.now()), mode: FILE_MODE, contents: files[rel].bytes }, key);
            written++;
          });
          // (the import removes files that the game's own browser backup had dropped: {remove: {userdata: [rel, ...]}})
          ((folders.remove && folders.remove[folder]) || []).forEach(function (rel) { store.delete(root + "/" + rel); });
        });
        tx.oncomplete = function () { db.close(); resolve(written); };
        tx.onerror = tx.onabort = function () { try { db.close(); } catch (e) {} reject(tx.error || new Error(ByopopStrings.t("saves.err.keep"))); };
      };
    });
  }

  // ---------- the Web Locks ----------
  // Which of the games' locks are held (a game page is open): the ids; null when this browser has no Web Locks (it cannot tell)
  function openGames(games) {
    if (!(navigator.locks && navigator.locks.query)) return Promise.resolve(null);
    return navigator.locks.query().then(function (q) {
      var held = {};
      (q.held || []).concat(q.pending || []).forEach(function (l) { held[l.name] = true; });
      return games.filter(function (g) { return g.lock && held[g.lock]; }).map(function (g) { return g.id; });
    }, function () { return null; });
  }
  // Holds every game's lock while fn runs (so that no game page opens in between): resolves with fn's result; rejects when a lock is not free
  function withLocks(games, fn) {
    var releases = [], pending = games.filter(function (g) { return g.lock; });
    return Promise.all(pending.map(function (g) {
      return new Promise(function (got, failed) {
        navigator.locks.request(g.lock, { ifAvailable: true }, function (lock) {
          if (!lock) { failed(new Error(ByopopStrings.t("saves.err.open_tab", {title: g.title}))); return null; }
          return new Promise(function (release) { releases.push(release); got(); });
        }).catch(failed);
      });
    })).then(function () { return fn(); }).then(function (v) { releases.forEach(function (r) { r(); }); return v; },
      function (e) { releases.forEach(function (r) { r(); }); throw e; });
  }

  // ---------- the section ----------
  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  var SETTINGS_KEY = "byopop-settings", PROFILES_KEY = "byopop-profiles", PLAY_KEY = "byopop-play";

  // root: the element to build in; games: [{id, title, persist, lock, replays}]
  function init(root, games) {
    var ids = games.map(function (g) { return g.id; }), byId = {};
    games.forEach(function (g) { byId[g.id] = g; });
    // A restore goes idle -> reading the zip (busy) -> asking (plan, the zip read, waiting for the player's yes, with the question
    // shown) -> restoring (busy) -> idle; No, Esc or a new pick goes back to idle (endPlan). busy also holds while a backup is
    // made, and the import (web/saveimport.js) sets it through setBusy, so only one of them runs at a time.
    var plan = null;
    var busy = false;
    function endPlan() { plan = null; confirmBox.hidden = true; }
    var errorControls = [];   // the controls that an error is read with (the import's, web/saveimport.js, join them)
    root.textContent = "";
    // where the saves are kept, as a value: the window's status bar in Desktop 95 (web/skins.css), a line at the foot elsewhere
    var where = make("p", "pl-about sv-where", ByopopStrings.t("saves.about"));
    var status = make("p", "pl-status");
    status.setAttribute("role", "status");
    status.setAttribute("aria-live", "polite");
    // an error is its own line, an alert (announced at once)
    var alertEl = make("p", "pl-status pl-error pl-alert");
    alertEl.id = "sv-alert";
    alertEl.setAttribute("role", "alert");
    root.appendChild(status);
    root.appendChild(alertEl);
    // an error is read with the controls it can be about, until the next message
    function say(text, isError) {
      status.textContent = isError ? "" : text;
      alertEl.textContent = isError ? text : "";
      errorControls.forEach(function (c) {
        // a control's own description (the backup button's, while the browser may clear the saves) stays with the error
        var own = c.getAttribute("data-own-desc") || "";
        var ids = (isError ? [alertEl.id, own] : [own]).filter(Boolean).join(" ");
        if (ids) c.setAttribute("aria-describedby", ids); else c.removeAttribute("aria-describedby");
      });
    }

    var listTitle = make("h3", "", ByopopStrings.t("saves.list.title"));
    var list = make("ul", "sv-list");
    var row = make("div", "pl-actions sv-actions");
    var backupButton = make("button", "", ByopopStrings.t("saves.backup.button"));
    backupButton.type = "button";
    var pickId = "sv-pick";
    var pickLabel = make("label", "", ByopopStrings.t("saves.restore.label"));
    pickLabel.htmlFor = pickId;
    var pick = make("input");
    pick.type = "file"; pick.id = pickId; pick.accept = ".zip,application/zip";
    row.appendChild(backupButton); row.appendChild(pickLabel); row.appendChild(pick);
    var report = make("div", "sv-report");
    var confirmBox = make("div", "pl-confirm sv-confirm");
    confirmBox.hidden = true;
    root.appendChild(listTitle); root.appendChild(list); root.appendChild(row); root.appendChild(report); root.appendChild(confirmBox);
    root.appendChild(where);
    // whether the browser has agreed to keep the site's storage (a game page asks after an import, web/shell.html,
    // askToKeepStorage): until it has, a drive that runs low on space can make it clear the whole site, and then the backup
    // button says so, as its tooltip and its description
    var kept = make("span", "visually-hidden", ByopopStrings.t("saves.kept"));
    kept.id = "sv-kept";
    try {
      if (navigator.storage && navigator.storage.persisted) navigator.storage.persisted().then(function (yes) {
        if (yes) return;
        backupButton.title = kept.textContent;
        row.appendChild(kept);
        backupButton.setAttribute("data-own-desc", kept.id);
        backupButton.setAttribute("aria-describedby", kept.id);
      }, function () {});
    } catch (e) {}

    errorControls.push(backupButton, pick);
    // the import (web/saveimport.js) is told to stand still too while anything runs
    var busyHooks = [];
    function setBusy(b) {
      busy = b; backupButton.disabled = b; pick.disabled = b; busyHooks.forEach(function (h) { h(b); });
      try { if (window.ByopopCursors) ByopopCursors.busy("saves", b); } catch (e) {}  // the site's wait cursor (web/cursors.js)
    }

    function renderList() {
      return databases().then(function (names) {
        return Promise.all(games.map(function (g) { return overview(g, names).catch(function () { return null; }); })).then(function (all) {
          list.textContent = "";
          var any = false;
          games.forEach(function (g, i) {
            var o = all[i];
            if (!o || (!o.files && !o.replays && !o.backup)) return;
            any = true;
            var li = make("li");
            li.appendChild(make("strong", "", g.title));
            var parts = [];
            parts.push(o.files ? ByopopStrings.t("saves.list.files", {n: o.files, size: size(o.bytes)}) : ByopopStrings.t("saves.list.no_files"));
            if (g.replays) parts.push(o.replays ? ByopopStrings.t("saves.list.replays", {n: o.replays, size: size(o.replayBytes)}) : ByopopStrings.t("saves.list.no_replays"));
            li.appendChild(document.createTextNode(ByopopStrings.t("saves.list.rest", {parts: parts.join(", "), backup: o.backup ? ByopopStrings.t("saves.list.backup") : ""})));
            list.appendChild(li);
          });
          if (!any) list.appendChild(make("li", "pl-empty", ByopopStrings.t("saves.list.empty")));
        });
      });
    }

    // ----- the backup -----
    function backup() {
      if (busy) return;
      setBusy(true);
      report.textContent = "";
      say(ByopopStrings.t("saves.backup.reading"));
      var made = new Date();
      Promise.all([databases(), openGames(games)]).then(function (r) {
        var names = r[0], open = r[1] || [];
        var entries = [], manifest = { format: FORMAT, version: VERSION, made: made.toISOString(), games: {}, files: {} }, enc = new TextEncoder(), lines = [];
        return games.reduce(function (p, g) {
          return p.then(function () {
            return openExisting(g.persist, names).then(function (db) {
              var item = lsGet(backupKey(g.persist));
              var load = db ? Promise.all([readFolder(db, g.persist, "userdata"), g.replays ? readFolder(db, g.persist, "replays") : Promise.resolve({})]).then(function (x) { db.close(); return x; }) : Promise.resolve([{}, {}]);
              return load.then(function (x) {
                var m = merged(x[0], item), n = 0, bytes = 0, rn = 0, rbytes = 0;
                Object.keys(m.files).sort().forEach(function (rel) {
                  entries.push({ name: g.id + "/userdata/" + rel, bytes: m.files[rel].bytes });
                  manifest.files[g.id + "/userdata/" + rel] = m.files[rel].m; n++; bytes += m.files[rel].bytes.length;
                });
                Object.keys(x[1]).sort().forEach(function (rel) {
                  entries.push({ name: g.id + "/replays/" + rel, bytes: x[1][rel].bytes });
                  manifest.files[g.id + "/replays/" + rel] = x[1][rel].m; rn++; rbytes += x[1][rel].bytes.length;
                });
                if (item) entries.push({ name: g.id + "/browser-backup.json", bytes: enc.encode(item) });
                if (n || rn || item) {
                  manifest.games[g.id] = { title: g.title, files: n, bytes: bytes, replays: rn, replayBytes: rbytes, browserBackup: !!item };
                  lines.push({ g: g, n: n, bytes: bytes, rn: rn, rbytes: rbytes, item: !!item, open: open.indexOf(g.id) >= 0, used: m.used });
                }
              });
            });
          });
        }, Promise.resolve()).then(function () {
          var s = lsGet(SETTINGS_KEY), pr = lsGet(PROFILES_KEY), play = playToManifest(lsGet(PLAY_KEY));
          if (s !== null) entries.unshift({ name: "byopop-settings.json", bytes: enc.encode(s) });
          if (pr !== null) entries.unshift({ name: "byopop-profiles.json", bytes: enc.encode(pr) });
          manifest.settings = s !== null; manifest.players = pr !== null;
          if (play) manifest.play = play;
          entries.unshift({ name: "byopop-saves.json", bytes: enc.encode(JSON.stringify(manifest, null, 1)) });
          var zip = writeZip(entries, made), blob = new Blob([zip], { type: "application/zip" }), a = document.createElement("a");
          a.href = URL.createObjectURL(blob); a.download = backupName(made);
          document.body.appendChild(a); a.click(); a.remove();
          setTimeout(function () { URL.revokeObjectURL(a.href); }, 60000);
          report.textContent = "";
          report.appendChild(make("h3", "", ByopopStrings.t("saves.backup.took")));
          var ul = make("ul", "sv-list");
          lines.forEach(function (l) {
            var li = make("li");
            li.appendChild(make("strong", "", l.g.title));
            var t = ByopopStrings.t("saves.report.line", {
              files: l.n ? ByopopStrings.t("saves.report.files", {n: l.n, size: size(l.bytes)}) : ByopopStrings.t("saves.report.no_files"),
              replays: l.g.replays ? ByopopStrings.t("saves.report.replays", {n: l.rn}) : "",
              backup: l.item ? (l.used ? ByopopStrings.t("saves.report.with_backup", {n: l.used}) : ByopopStrings.t("saves.report.backup_kept")) : "" });
            if (l.open) t += " " + ByopopStrings.t("saves.report.open");
            li.appendChild(document.createTextNode(t));
            ul.appendChild(li);
          });
          ul.appendChild(make("li", "", ByopopStrings.t("saves.report.site", {
            settings: ByopopStrings.t(s !== null ? "saves.report.included" : "saves.report.none_set"),
            players: ByopopStrings.t(pr !== null ? "saves.report.included" : "saves.report.none"),
            play: ByopopStrings.t(play ? "saves.report.included" : "saves.report.none") })));
          report.appendChild(ul);
          say(ByopopStrings.t("saves.backup.done", {name: backupName(made), size: size(zip.length), count: plural(entries.length, "saves.n.file")}));
        });
      }).catch(function (e) { say(ByopopStrings.t("saves.backup.failed", {error: e && e.message ? e.message : e}), true); }).then(function () { setBusy(false); });
    }
    backupButton.addEventListener("click", backup);

    // ----- the restore -----
    pick.addEventListener("change", function () {
      var file = pick.files && pick.files[0];
      pick.value = "";
      if (file) readBackup(file);
    });
    function readBackup(file) {
      if (busy) return;
      setBusy(true);
      endPlan(); report.textContent = "";
      say(ByopopStrings.t("saves.restore.reading", {name: file.name}));
      file.arrayBuffer().then(function (buf) {
        var entries = readZip(new Uint8Array(buf)), sorted = classify(entries, ids);
        if (!sorted.ok) throw new Error(sorted.error);
        if (!sorted.manifest) throw new Error(ByopopStrings.t("saves.zip.not_backup"));
        return sorted.manifest.read().then(function (b) {
          var man;
          try { man = JSON.parse(new TextDecoder().decode(b)); } catch (e) { man = null; }
          if (!man || man.format !== FORMAT || man.version !== VERSION) throw new Error(ByopopStrings.t("saves.zip.manifest"));
          return man;
        }).then(function (man) {
          // the files, read now (checked against their checksums), by game
          var perGame = {};
          return sorted.files.reduce(function (p, f) {
            return p.then(function () {
              return f.entry.read().then(function (bytes) {
                var pg = perGame[f.id] || (perGame[f.id] = { userdata: {}, replays: {}, bytes: 0, count: 0, replayCount: 0 });
                var key = f.id + "/" + f.kind + "/" + f.rel, m = man.files && typeof man.files[key] === "number" ? man.files[key] : Date.now();
                pg[f.kind][f.rel] = { bytes: bytes, m: m };
                pg.bytes += bytes.length;
                if (f.kind === "userdata") pg.count++; else pg.replayCount++;
              });
            });
          }, Promise.resolve()).then(function () {
            var texts = {};
            return Promise.all([["settings", sorted.settings, "saves.zip.bad_settings"], ["profiles", sorted.profiles, "saves.zip.bad_profiles"]].map(function (pair) {
              if (!pair[1]) return null;
              return pair[1].read().then(function (b) {
                var t = new TextDecoder().decode(b), o = null;
                try { o = JSON.parse(t); } catch (e) {}
                if (!o || typeof o !== "object" || Array.isArray(o)) throw new Error(ByopopStrings.t(pair[2]));
                texts[pair[0]] = t;
              });
            })).then(function () {
              var pt = playFromManifest(man);
              if (pt !== null) texts["play times"] = pt;
              return { perGame: perGame, texts: texts, man: man, file: file };
            });
          });
        });
      }).then(function (p) {
        return describe(p);
      }).catch(function (e) { say(e && e.message ? e.message : String(e), true); }).then(function () { setBusy(false); });
    }
    // What the zip holds and what it would replace, then the confirmation
    function describe(p) {
      return Promise.all([databases(), openGames(games)]).then(function (r) {
        var names = r[0], open = r[1];
        return Promise.all(Object.keys(p.perGame).map(function (id) {
          var g = byId[id];
          return openExisting(g.persist, names).then(function (db) {
            if (!db) return { id: id, replace: 0, add: Object.keys(p.perGame[id].userdata).length, rReplace: 0, rAdd: Object.keys(p.perGame[id].replays).length };
            return Promise.all([readFolder(db, g.persist, "userdata"), readFolder(db, g.persist, "replays")]).then(function (cur) {
              db.close();
              var res = { id: id, replace: 0, add: 0, rReplace: 0, rAdd: 0 };
              Object.keys(p.perGame[id].userdata).forEach(function (k) { if (cur[0][k]) res.replace++; else res.add++; });
              Object.keys(p.perGame[id].replays).forEach(function (k) { if (cur[1][k]) res.rReplace++; else res.rAdd++; });
              return res;
            }, function () { db.close(); return { id: id, replace: 0, add: 0, rReplace: 0, rAdd: 0 }; });
          });
        })).then(function (rows) {
          plan = { p: p, rows: rows, open: open };
          showConfirm();
          say(p.man.made ? ByopopStrings.t("saves.restore.found_made", {name: p.file.name, date: new Date(p.man.made).toLocaleString()}) : ByopopStrings.t("saves.restore.found", {name: p.file.name}));
        });
      });
    }
    function showConfirm() {
      confirmBox.textContent = "";
      confirmBox.hidden = false;
      // a dialog once its question is up: its name and text are made here (the box stays in the page, hidden, afterwards)
      confirmBox.setAttribute("role", "alertdialog");
      confirmBox.setAttribute("aria-labelledby", "sv-confirm-title");
      confirmBox.setAttribute("aria-describedby", "sv-confirm-desc");
      var h = make("h4", "", ByopopStrings.t("saves.confirm.title"));
      h.id = "sv-confirm-title";
      confirmBox.appendChild(h);
      var desc = make("p", "", ByopopStrings.t("saves.confirm.desc"));
      desc.id = "sv-confirm-desc";
      confirmBox.appendChild(desc);
      var ul = make("ul", "sv-list");
      plan.rows.forEach(function (r) {
        var g = byId[r.id], pg = plan.p.perGame[r.id], li = make("li");
        li.appendChild(make("strong", "", g.title));
        var t = ByopopStrings.t(pg.replayCount ? "saves.confirm.line_replays_detail" : g.replays ? "saves.confirm.line_replays" : "saves.confirm.line",
          {files: plural(pg.count, "saves.n.save_file"), replays: plural(pg.replayCount, "saves.n.replay"), size: size(pg.bytes),
            replace: r.replace, add: r.add, rreplace: r.rReplace, radd: r.rAdd});
        if (plan.open && plan.open.indexOf(r.id) >= 0) t += " " + ByopopStrings.t("saves.confirm.open");
        li.appendChild(document.createTextNode(t));
        ul.appendChild(li);
      });
      if (!plan.rows.length) ul.appendChild(make("li", "", ByopopStrings.t("saves.confirm.none")));
      if (plan.p.texts.settings !== undefined) ul.appendChild(make("li", "", ByopopStrings.t("saves.confirm.settings")));
      if (plan.p.texts.profiles !== undefined) ul.appendChild(make("li", "", ByopopStrings.t("saves.confirm.players")));
      if (plan.p.texts["play times"] !== undefined) ul.appendChild(make("li", "", ByopopStrings.t("saves.confirm.play")));
      confirmBox.appendChild(ul);
      var openNow = plan.open === null ? ByopopStrings.t("saves.confirm.no_locks")
        : plan.open.length ? ByopopStrings.t("saves.confirm.close_first", {games: plan.open.map(function (id) { return byId[id].title; }).join(", ")}) : "";
      if (openNow) confirmBox.appendChild(make("p", "pl-error", openNow));
      var act = make("div", "pl-actions");
      var yes = make("button", "pl-danger", ByopopStrings.t("saves.confirm.yes"));
      yes.type = "button";
      yes.disabled = !!openNow;
      var no = make("button", "", ByopopStrings.t("saves.confirm.no"));
      no.type = "button";
      yes.addEventListener("click", function () { doRestore(); });
      no.addEventListener("click", function () { endPlan(); say(ByopopStrings.t("saves.restore.nothing")); backupButton.focus(); });
      act.appendChild(yes); act.appendChild(no);
      confirmBox.appendChild(act);
      yes.focus();
    }
    confirmBox.addEventListener("keydown", function (ev) {
      if (ev.key === "Escape" && !busy) { endPlan(); say(ByopopStrings.t("saves.restore.nothing")); backupButton.focus(); }
    });
    function doRestore() {
      if (busy || !plan) return;
      var p = plan.p, targets = Object.keys(p.perGame).map(function (id) { return byId[id]; });
      setBusy(true);
      say(ByopopStrings.t("saves.restore.working"));
      // the games' pages are held shut while the files are written, and the check is made again just before
      openGames(games).then(function (open) {
        if (open === null) throw new Error(ByopopStrings.t("saves.restore.no_locks"));
        if (open.length) throw new Error(ByopopStrings.t("saves.restore.open", {games: open.map(function (id) { return byId[id].title; }).join(", ")}));
        return databases().then(function (names) {
          return withLocks(games, function () {
            var wrote = [];
            return targets.reduce(function (q, g) {
              return q.then(function () {
                var pg = p.perGame[g.id];
                return writeFiles(g, names, { userdata: pg.userdata, replays: pg.replays }).then(function (n) {
                  lsSet(backupKey(g.persist), null);
                  wrote.push(ByopopStrings.t("saves.restored.game", {title: g.title, count: plural(n, "saves.n.file")}));
                });
              });
            }, Promise.resolve()).then(function () {
              if (p.texts.settings !== undefined) lsSet(SETTINGS_KEY, p.texts.settings);
              if (p.texts.profiles !== undefined) lsSet(PROFILES_KEY, p.texts.profiles);
              if (p.texts["play times"] !== undefined) lsSet(PLAY_KEY, mergedPlay(lsGet(PLAY_KEY), p.texts["play times"]));
              return wrote;
            });
          });
        });
      }).then(function (wrote) {
        endPlan();
        report.textContent = "";
        report.appendChild(make("h3", "", ByopopStrings.t("saves.restored.title")));
        var ul = make("ul", "sv-list");
        wrote.forEach(function (t) { ul.appendChild(make("li", "", t)); });
        if (p.texts.settings !== undefined) ul.appendChild(make("li", "", ByopopStrings.t("saves.restored.settings")));
        if (p.texts.profiles !== undefined) ul.appendChild(make("li", "", ByopopStrings.t("saves.restored.players")));
        if (p.texts["play times"] !== undefined) ul.appendChild(make("li", "", ByopopStrings.t("saves.restored.play")));
        report.appendChild(ul);
        say(ByopopStrings.t("saves.restore.done"));
        try { window.dispatchEvent(new StorageEvent("storage", { key: null })); } catch (e) {}
        return renderList();
      }).catch(function (e) { say(ByopopStrings.t("saves.restore.failed", {error: e && e.message ? e.message : e}), true); renderList(); }).then(function () {
        setBusy(false);
        // the question that had the focus is gone
        if (document.activeElement === document.body || !document.activeElement) backupButton.focus();
      });
    }

    renderList();
    // "Import from an installed game": its own file, which needs the helpers above
    if (typeof ByopopSaveImport !== "undefined" && ByopopSaveImport.init) {
      try {
        ByopopSaveImport.init(root, games, {
          say: say, isBusy: function () { return busy; }, setBusy: setBusy, refresh: renderList, size: size,
          databases: databases, openExisting: openExisting, readFolder: readFolder, writeFiles: writeFiles, merged: merged,
          lsGet: lsGet, lsSet: lsSet, backupKey: backupKey, withLocks: withLocks,
          openGames: function () { return openGames(games); },
          addControls: function (list) { list.forEach(function (c) { errorControls.push(c); }); },
          onBusy: function (h) { busyHooks.push(h); }
        });
      } catch (e) { root.appendChild(make("p", "pl-error", ByopopStrings.t("saves.import_unavailable"))); }
    }
    root.appendChild(where);  // last, under the import too: a status bar is the window's foot
    document.addEventListener("visibilitychange", function () { if (!document.hidden && !busy) renderList(); });
    window.addEventListener("pageshow", function () { if (!busy) renderList(); });
    window.addEventListener("storage", function (ev) { if (!busy && (ev.key === null || /^sexy-userdata-backup:/.test(ev.key || ""))) renderList(); });
    return { refresh: renderList };
  }

  core.init = init;
  core.overview = overview;
  return core;
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopSaves;
