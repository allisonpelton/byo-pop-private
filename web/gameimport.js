// The import of a game's files, shared by the game pages and the listing page (docs/WEB_PLAYER.md, "Adding games from the
// listing page"). A game page imports through its own card (web/shell.html) and writes through its Emscripten file system; the
// listing page takes a folder dropped on a game's tile or on the logo, or many games' folders at once, works out which game each
// folder is, and writes the files straight into the game's IDBFS database, as the records IDBFS itself writes, while it holds the
// game's Web Lock (PAGE.lock), so that no game page has the database open meanwhile. The rules for a game are its page.js's PAGE
// (web/shell.html lists the fields): markers, gameDirs, gameFiles, paks, skip, importIcon. Files are {path, read()}, as
// web/modpanel.js's zip helpers make them; its pak reader and safeRel are passed in, as the listing page loads modpanel.js
// without a game. Plain ES5, like the shells.
var ByopopImport = (function () {
  "use strict";

  // A marker ("images/gemsheet6.*") as a pattern for a path relative to the game folder, in any case
  function markerPattern(marker) {
    return marker.replace(/[.*+?^${}()|[\]\\]/g, "\\$&").replace(/\\\.\\\*$/, "\\.[a-z0-9]+");
  }
  function under(e, root) { return e.path.substring(0, root.length) === root; }

  // The folder inside the files that holds every marker (the game root, "" or ending in "/"), or null; with placeholders,
  // failing that, the shallowest folder that holds one of the game's folders.
  function findRoot(spec, entries, placeholders) {
    var first = new RegExp("^(.*/)?" + markerPattern(spec.markers[0]) + "$", "i");
    var others = spec.markers.slice(1).map(function (m) { return new RegExp("^" + markerPattern(m) + "$", "i"); });
    for (var i = 0; i < entries.length; i++) {
      var m = first.exec(entries[i].path);
      if (!m) continue;
      var root = m[1] || "";
      var inRoot = entries.filter(function (e) { return under(e, root); });
      if (others.every(function (re) { return inRoot.some(function (e) { return re.test(e.path.substring(root.length)); }); }))
        return root;
    }
    if (!placeholders) return null;
    var best = null;
    entries.forEach(function (e) {
      var parts = e.path.split("/");
      for (var j = 0; j < parts.length - 1; j++) {
        if (spec.gameDirs.indexOf(parts[j].toLowerCase()) >= 0) {
          var r = parts.slice(0, j).join("/") + (j > 0 ? "/" : "");
          if ((best === null) || (r.length < best.length)) best = r;
          break;
        }
      }
    });
    return best;
  }

  // The install's files with its paks (spec.paks) unpacked: each pak's files take its place, after the loose files, so that a
  // file in both is written from the pak. onUnpack(names) is told which paks are being read.
  function withPaks(spec, entries, root, pak, onUnpack) {
    var paks = spec.paks || [];
    var found = entries.filter(function (e) { return under(e, root) && paks.indexOf(e.path.substring(root.length).toLowerCase()) >= 0; });
    if (found.length > 0 && onUnpack) onUnpack(found.map(function (e) { return e.path.substring(root.length); }));
    return found.reduce(function (p, e) {
      return p.then(function (all) {
        return e.read().then(function (bytes) {
          return all.concat(pak.read(bytes).map(function (f) { return { path: root + f.path, read: f.read }; }));
        });
      });
    }, Promise.resolve(entries.filter(function (e) { return found.indexOf(e) < 0; })));
  }

  // The files that are copied, as {rel, read}: the game's folders (stored in lower case, as spec.gameDirs names them: the pages
  // look for them that way, and a Steam install's "Images" would never be found; the game itself finds files in any case) and its
  // files at the top, but the .sig files and spec.skip's
  function pick(spec, entries, root, safeRel) {
    var skip = spec.skip || [], files = spec.gameFiles || [], wanted = [];
    entries.forEach(function (e) {
      if (!under(e, root)) return;
      var rel = safeRel(e.path.substring(root.length));
      if (!rel || /\.sig$/i.test(rel) || skip.some(function (re) { return re.test(rel); })) return;
      var top = rel.split("/")[0];
      if (spec.gameDirs.indexOf(top.toLowerCase()) >= 0) wanted.push({ rel: top.toLowerCase() + rel.substring(top.length), read: e.read });
      else if (files.indexOf(rel.toLowerCase()) >= 0) wanted.push({ rel: rel, read: e.read });
    });
    return wanted;
  }

  // Whether the picked files hold every marker: a pak that is a marker was unpacked, so then any file in the game's folders will do
  function hasMarkers(spec, rels) {
    var paks = spec.paks || [];
    var anyGameDir = rels.some(function (r) { var i = r.indexOf("/"); return i > 0 && spec.gameDirs.indexOf(r.substring(0, i)) >= 0; });
    return spec.markers.every(function (m) {
      if (paks.indexOf(m.toLowerCase()) >= 0) return anyGameDir;
      var re = new RegExp("^" + markerPattern(m) + "$", "i");
      return rels.some(function (r) { return re.test(r); });
    });
  }

  // The icon in the install's exe, for a game that does not store its exe (importIcon): the first .exe at the root that is not
  // an uninstaller or setup. Resolves with a PNG data URL, or "".
  var SKIP_EXE = /^(popuninstall|uninstall[^\/]*|unins\d*|setup[^\/]*|dxsetup|[^\/]*redist[^\/]*|[^\/]*updater[^\/]*)\.exe$/i;
  function exeIcon(entries, root) {
    if (!window.ByopopExeIcon) return Promise.resolve("");
    var exes = entries.filter(function (e) {
      var rel = under(e, root) ? e.path.substring(root.length) : "";
      return /^[^\/]+\.exe$/i.test(rel) && !SKIP_EXE.test(rel);
    }).slice(0, 4);
    return exes.reduce(function (p, e) {
      return p.then(function (url) {
        if (url) return url;
        return e.read().then(function (bytes) { return ByopopExeIcon.fromBytes(bytes); })
          .then(function (icon) { return (icon && icon.dataUrl) || ""; }, function () { return ""; });
      });
    }, Promise.resolve("")).catch(function () { return ""; });
  }

  // ---------- which game a folder is ----------
  // The games the listing page can add: {target, title, spec}, filled by import-games.js, which cmake/ByopopWeb.cmake writes from
  // each listed game's page.js
  var games = [];
  function add(target, title, makeSpec) {
    try { games.push({ target: target, title: title, spec: makeSpec() }); } catch (e) {}
  }
  // A name reduced to its letters and digits, without the words every game's folder may have
  function plain(s) {
    return String(s).toLowerCase().replace(/&/g, "and").replace(/\b(deluxe|popcap|games?|the|pc|windows|steam|install(ed)?)\b/g, "")
      .replace(/[^a-z0-9]/g, "");
  }
  // How well a folder's name says it is the game: 3 the same name, 2 one holds the other, 0 neither
  function nameScore(folder, game) {
    var f = plain(folder);
    if (!f) return 0;
    var names = [game.title, game.spec.shortName || "", game.target].map(plain).filter(Boolean);
    if (names.indexOf(f) >= 0) return 3;
    return names.some(function (n) { return n.length >= 4 && (f.indexOf(n) >= 0 || n.indexOf(f) >= 0); }) ? 2 : 0;
  }
  // The files under a folder, with the folder's own part of each path left on
  function groupBy(entries, depth) {
    var out = {}, order = [];
    entries.forEach(function (e) {
      var parts = e.path.split("/");
      var key = parts.length > depth + 1 ? parts.slice(0, depth + 1).join("/") : parts.slice(0, depth).join("/");
      if (!(key in out)) { out[key] = []; order.push(key); }
      out[key].push(e);
    });
    return order.map(function (k) { return { path: k, entries: out[k] }; });
  }
  // Sorts dropped or picked files into the games they are: {found: [{folder, entries, root, game, choices}], rejected: [folder]}.
  // A folder's game is the one whose markers it holds (the files say it more surely than the name). Where several games' markers
  // are there at the same place (a game and its sequel may share their files' names), the folder's name picks among them, and
  // where it cannot, game is null and choices are the games it could be, for the player to pick. A folder that holds no game but
  // holds games' folders (the "PopCap Games" folder) is looked into, three levels deep at most; a folder with no game in it is
  // rejected.
  function sort(entries) {
    var found = [], rejected = [];
    function visit(group, depth) {
      var name = group.path.split("/").pop() || "(the dropped files)";
      var matches = [];
      games.forEach(function (g) {
        var root = findRoot(g.spec, group.entries, false);
        if (root !== null) matches.push({ game: g, root: root });
      });
      var roots = {};
      matches.forEach(function (m) { roots[m.root] = true; });
      if (matches.length > 0 && Object.keys(roots).length === 1) {
        var root = matches[0].root, best = null, top = 0, tie = false;
        matches.forEach(function (m) {
          var s = Math.max(nameScore(name, m.game), nameScore(root.replace(/\/$/, "").split("/").pop(), m.game));
          if (s > top) { top = s; best = m.game; tie = false; } else if (s === top && s > 0) tie = true;
        });
        var game = matches.length === 1 ? matches[0].game : (top > 0 && !tie ? best : null);
        found.push({ folder: name, entries: group.entries, root: root, game: game, choices: matches.map(function (m) { return m.game; }) });
        return;
      }
      // (no game anywhere in it: findRoot looks at every depth)
      var deeper = matches.length > 0 ? groupBy(group.entries, depth + 1).filter(function (g) { return g.path !== group.path; }) : [];
      if (depth < 3 && deeper.length > 0) {
        deeper.forEach(function (g) { visit(g, depth + 1); });
        return;
      }
      rejected.push(name);
    }
    groupBy(entries, 0).forEach(function (g) { visit(g, 0); });
    return { found: found, rejected: rejected };
  }

  // ---------- reading a drop ----------
  // The files of a drop as {path, read()}: a dropped folder's paths start with its name, and a dropped .zip's with its name
  // without ".zip". Must be called in the drop event itself (the entries are only readable during it). zip: web/modpanel.js's.
  function fromDrop(dataTransfer, zip) {
    var tops = [];
    Array.prototype.forEach.call(dataTransfer.items || [], function (item) {
      if (item.kind !== "file") return;
      var entry = item.webkitGetAsEntry ? item.webkitGetAsEntry() : null;
      if (entry) tops.push(entry);
      else { var f = item.getAsFile(); if (f) tops.push({ isFile: true, name: f.name, fullPath: "/" + f.name, plainFile: f }); }
    });
    var out = [];
    function fileOf(entry) {
      if (entry.plainFile) return Promise.resolve(entry.plainFile);
      return new Promise(function (resolve, reject) { entry.file(resolve, reject); });
    }
    function walk(entry) {
      if (entry.isFile) {
        var path = entry.fullPath.replace(/^\//, "");
        if (/\.zip$/i.test(entry.name) && path.indexOf("/") < 0) {
          return fileOf(entry).then(function (f) { return zip.fromFile(f); }).then(function (files) {
            var base = entry.name.replace(/\.zip$/i, "");
            files.forEach(function (z) { out.push({ path: base + "/" + z.path, read: z.read }); });
          });
        }
        out.push({ path: path, read: function () { return fileOf(entry).then(function (f) { return f.arrayBuffer(); }).then(function (b) { return new Uint8Array(b); }); } });
        return Promise.resolve();
      }
      if (!entry.isDirectory) return Promise.resolve();
      var reader = entry.createReader();
      return new Promise(function (resolve, reject) {
        var all = [];
        (function more() {
          reader.readEntries(function (batch) {
            if (!batch.length) { resolve(all); return; }
            all = all.concat(Array.prototype.slice.call(batch));
            more();
          }, reject);
        })();
      }).then(function (children) {
        return children.reduce(function (p, c) { return p.then(function () { return walk(c); }); }, Promise.resolve());
      });
    }
    return tops.reduce(function (p, t) { return p.then(function () { return walk(t); }); }, Promise.resolve()).then(function () { return out; });
  }
  // Whether a drag carries files (a folder is a file to the browser), so that a drop target can say it takes them
  function hasFiles(dataTransfer) {
    return !!dataTransfer && Array.prototype.indexOf.call(dataTransfer.types || [], "Files") >= 0;
  }

  // ---------- writing a game's files from the listing page ----------
  var DB_STORE = "FILE_DATA", DB_VERSION = 21, FILE_MODE = 33206, DIR_MODE = 16895;  // as IDBFS writes them (web/saves.js)
  var BATCH_BYTES = 32 * 1048576;
  function openDb(persist) {
    return new Promise(function (resolve, reject) {
      var r;
      try { r = indexedDB.open(persist, DB_VERSION); } catch (e) { reject(e); return; }
      r.onerror = function () { reject(r.error || new Error("The browser would not open the game's storage.")); };
      r.onblocked = function () { reject(new Error("The game's storage is in use. Close the game's page and try again.")); };
      r.onupgradeneeded = function (ev) {
        var db = ev.target.result, store = db.objectStoreNames.contains(DB_STORE) ? ev.target.transaction.objectStore(DB_STORE) : db.createObjectStore(DB_STORE);
        if (!store.indexNames.contains("timestamp")) store.createIndex("timestamp", "timestamp", { unique: false });
      };
      r.onsuccess = function () { resolve(r.result); };
    });
  }
  function done(tx) {
    return new Promise(function (resolve, reject) {
      tx.oncomplete = function () { resolve(); };
      tx.onerror = tx.onabort = function () { reject(tx.error || new Error("The browser would not keep the files.")); };
    });
  }
  // Replaces the game folder (persist + "/game") with files [{rel, read}], reading and writing about 32 MB at a time.
  // progress(done, total) after each file.
  function writeGame(persist, files, progress) {
    var game = persist + "/game", now = new Date();
    return openDb(persist).then(function (db) {
      var tx = db.transaction(DB_STORE, "readwrite"), store = tx.objectStore(DB_STORE);
      store.delete(IDBKeyRange.bound(game + "/", game + "/￿"));
      var dirs = {};
      dirs[game] = true;
      files.forEach(function (f) {
        var parts = f.rel.split("/");
        for (var i = 1; i < parts.length; i++) dirs[game + "/" + parts.slice(0, i).join("/")] = true;
      });
      Object.keys(dirs).forEach(function (d) { store.put({ timestamp: now, mode: DIR_MODE }, d); });
      return done(tx).then(function () {
        var at = 0;
        function batch() {
          if (at >= files.length) return Promise.resolve();
          var chunk = [], bytes = 0;
          function readMore() {
            if (at >= files.length || (chunk.length > 0 && bytes >= BATCH_BYTES)) return Promise.resolve();
            var f = files[at++];
            return f.read().then(function (data) { chunk.push({ key: game + "/" + f.rel, data: data }); bytes += data.length; return readMore(); });
          }
          return readMore().then(function () {
            var t = db.transaction(DB_STORE, "readwrite"), s = t.objectStore(DB_STORE);
            chunk.forEach(function (c) { s.put({ timestamp: now, mode: FILE_MODE, contents: c.data }, c.key); });
            return done(t);
          }).then(function () { if (progress) progress(at, files.length); return batch(); });
        }
        return batch();
      }).then(function () { db.close(); }, function (e) { try { db.close(); } catch (x) {} throw e; });
    });
  }
  // Runs fn while holding the game's Web Lock; rejects when a page of the game is open (it holds the lock all its life)
  function withLock(lock, title, fn) {
    if (!lock || !(navigator.locks && navigator.locks.request)) return fn();
    return new Promise(function (resolve, reject) {
      navigator.locks.request(lock, { ifAvailable: true }, function (held) {
        if (!held) { reject(new Error(title + " is open in another tab or window. Close it and try again.")); return null; }
        return Promise.resolve().then(fn).then(resolve, reject);
      }).catch(reject);
    });
  }
  // Adds one game's files from the listing page: g a games entry, entries the folder's files, root its game root (sort, or
  // findRoot). Resolves with the number of files; rejects with an Error to show. pak and safeRel: web/modpanel.js's. say(text)
  // reports the steps. The record of the game's files (ByopopGameFiles) is kept as the game page keeps it after an import.
  function addGame(g, entries, root, pak, safeRel, say) {
    var spec = g.spec;
    return withLock(spec.lock, g.title, function () {
      return withPaks(spec, entries, root, pak, function () { if (say) say("Unpacking…"); }).then(function (all) {
        var files = pick(spec, all, root, safeRel);
        if (!hasMarkers(spec, files.map(function (f) { return f.rel; })))
          throw new Error("That folder doesn't hold " + g.title + "'s " + spec.markerText + ".");
        if (say) say("Copying " + files.length + " files…");
        try { ByopopGameFiles.lost(spec.id); } catch (e) {}  // the old copy is about to go
        return writeGame(spec.persist, files, function (n, total) { if (say) say("Copying… " + Math.round(n * 100 / total) + "%"); })
          .then(function () { return icon(spec, all, root, files); })
          .then(function (url) {
            try { ByopopGameFiles.mark(spec.id); if (url) ByopopGameFiles.setIcon(spec.id, url); } catch (e) {}
            try { if (navigator.storage && navigator.storage.persist) navigator.storage.persist().catch(function () {}); } catch (e) {}
            return files.length;
          });
      });
    });
  }
  // The game's icon for the record: from the install's exe (importIcon), or from the exe it stores (the largest of its .exe files)
  function icon(spec, all, root, files) {
    if (spec.importIcon) return exeIcon(all, root);
    if (!window.ByopopExeIcon) return Promise.resolve("");
    var exes = files.filter(function (f) { return /\.exe$/i.test(f.rel) && f.rel.indexOf("/") < 0; });
    return Promise.all(exes.map(function (f) { return f.read(); })).then(function (datas) {
      var big = null;
      datas.forEach(function (d) { if (!big || d.length > big.length) big = d; });
      return big ? ByopopExeIcon.fromBytes(big).then(function (i) { return (i && i.dataUrl) || ""; }) : "";
    }).catch(function () { return ""; });
  }

  return { markerPattern: markerPattern, findRoot: findRoot, withPaks: withPaks, pick: pick, hasMarkers: hasMarkers, exeIcon: exeIcon,
    games: games, add: add, sort: sort, nameScore: nameScore, fromDrop: fromDrop, hasFiles: hasFiles, writeGame: writeGame, addGame: addGame };
})();
