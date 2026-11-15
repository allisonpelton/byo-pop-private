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

  // A marker ("images/gemsheet6.*") as a pattern for a path relative to the game folder, in any case. A marker may name
  // alternatives split by "|" ("WinTS.exe|TyperShark.exe": a game whose copies name a file differently), any one of which
  // will do; the pattern then groups them, so every pattern made from a marker takes either
  function markerPattern(marker) {
    var alts = marker.split("|").map(function (m) {
      return m.replace(/[.*+?^${}()|[\]\\]/g, "\\$&").replace(/\\\.\\\*$/, "\\.[a-z0-9]+");
    });
    return alts.length > 1 ? "(?:" + alts.join("|") + ")" : alts[0];
  }
  function under(e, root) { return e.path.substring(0, root.length) === root; }

  // The folder inside the files that holds every marker (the game root, "" or ending in "/"), or null; with placeholders,
  // failing that, the shallowest folder that holds one of the game's folders. A marker that is a pak (spec.paks) is there when
  // the pak is, or when the folder holds any of the game's folders: the pak already unpacked, as a staged copy or a zip of the
  // unpacked game has it (hasMarkers does the same for picked files).
  function findRoot(spec, entries, placeholders) {
    var paks = spec.paks || [];
    function isPak(m) { return paks.indexOf(m.toLowerCase()) >= 0; }
    var anyDepth = spec.markers.map(function (m) { return new RegExp("^(.*/)?" + markerPattern(m) + "$", "i"); });
    var atRoot = spec.markers.map(function (m) { return new RegExp("^" + markerPattern(m) + "$", "i"); });
    var allPaks = spec.markers.every(isPak), tried = {};
    function holds(root) {
      var inRoot = entries.filter(function (e) { return under(e, root); });
      return spec.markers.every(function (m, k) {
        if (inRoot.some(function (e) { return atRoot[k].test(e.path.substring(root.length)); })) return true;
        return isPak(m) && inRoot.some(function (e) {
          var rel = e.path.substring(root.length), j = rel.indexOf("/");
          return j > 0 && spec.gameDirs.indexOf(rel.substring(0, j).toLowerCase()) >= 0;
        });
      });
    }
    for (var i = 0; i < entries.length; i++) {
      var roots = [];
      anyDepth.forEach(function (re, k) {
        // a pak marker names a root only when every marker is one (else the other markers name it)
        if (isPak(spec.markers[k]) && !allPaks) return;
        var m = re.exec(entries[i].path);
        if (m) roots.push(m[1] || "");
      });
      for (var r = 0; r < roots.length; r++) {
        if (tried[roots[r]]) continue;
        tried[roots[r]] = true;
        if (holds(roots[r])) return roots[r];
      }
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

  // The markers a folder of files lacks, as the error names them (the user, 2026-11-03: the files it names are the missing
  // ones, not every marker): missingIn() for the files of a known root (paths relative to it), missingMarkers() for picked or
  // dropped files whose root was not found, from the folder among them that holds the most markers (all of them when none
  // holds any). Picked files have their paks unpacked, so a pak marker stands for the game's folders there (hasMarkers);
  // dropped files are as they came, so every marker, a pak too, is a file to find. Each is named with its casing in the game's markerText where that has it ("images/gemsheet6.*" as
  // "images/gemsheet6"), and the names are joined as a list in the page's language.
  function lacking(spec, rels) {
    return spec.markers.filter(function (m) {
      return !hasMarkers({ markers: [m], paks: spec.paks, gameDirs: spec.gameDirs }, rels);
    });
  }
  function markerName(spec, m) {
    if (m.indexOf("|") >= 0) {
      // alternatives: each named, joined as "a or b" in the page's language
      var each = m.split("|").map(function (one) { return markerName(spec, one); });
      try { return new Intl.ListFormat(ByopopStrings.lang, { style: "long", type: "disjunction" }).format(each); }
      catch (e) { return each.join(" / "); }
    }
    var text = spec.markerText || "", low = text.toLowerCase();
    var names = [m, m.replace(/\.\*$/, "")];
    for (var i = 0; i < names.length; i++) {
      var at = low.indexOf(names[i].toLowerCase());
      if (at >= 0) return text.substring(at, at + names[i].length);
    }
    return names[1];
  }
  function markerList(spec, markers) {
    var names = (markers || spec.markers).map(function (m) { return markerName(spec, m); });
    try { return new Intl.ListFormat(ByopopStrings.lang, { style: "long", type: "conjunction" }).format(names); }
    catch (e) { return names.join(", "); }
  }
  function missingIn(spec, rels) {
    var miss = lacking(spec, rels);
    return markerList(spec, miss.length ? miss : spec.markers);
  }
  function missingMarkers(spec, entries) {
    // the folders to try: those holding a file that is one of the markers
    var anyDepth = spec.markers.map(function (m) { return new RegExp("^(.*/)?" + markerPattern(m) + "$", "i"); });
    var atRoot = spec.markers.map(function (m) { return new RegExp("^" + markerPattern(m) + "$", "i"); });
    var roots = {};
    entries.forEach(function (e) {
      anyDepth.forEach(function (re) { var m = re.exec(e.path); if (m) roots[m[1] || ""] = true; });
    });
    var best = null;
    Object.keys(roots).forEach(function (r) {
      var miss = spec.markers.filter(function (m, i) {
        return !entries.some(function (e) { return under(e, r) && atRoot[i].test(e.path.substring(r.length)); });
      });
      if (best === null || miss.length < best.length) best = miss;
    });
    // none found, or (oddly) none lacking: name them all
    return markerList(spec, best && best.length ? best : spec.markers);
  }

  // The icon in the install's exe, for a game that does not store its exe (importIcon): the first .exe at the root that is not
  // an uninstaller or setup. Resolves with {url: a PNG data URL or "", built: the exe's link year or 0 (ByopopExeIcon.built),
  // small: the icon nearest 32 px, or ""}.
  var SKIP_EXE = /^(popuninstall|uninstall[^\/]*|unins\d*|setup[^\/]*|dxsetup|[^\/]*redist[^\/]*|[^\/]*updater[^\/]*)\.exe$/i;
  function exeIcon(entries, root) {
    if (!window.ByopopExeIcon) return Promise.resolve({ url: "", built: 0 });
    var exes = entries.filter(function (e) {
      var rel = under(e, root) ? e.path.substring(root.length) : "";
      return /^[^\/]+\.exe$/i.test(rel) && !SKIP_EXE.test(rel);
    }).slice(0, 4);
    return exes.reduce(function (p, e) {
      return p.then(function (got) {
        if (got.url) return got;
        return e.read().then(function (bytes) { return ByopopExeIcon.fromBytes(bytes); })
          .then(function (icon) { return { url: (icon && icon.dataUrl) || "", built: (icon && icon.built) || 0, small: (icon && icon.small) || "" }; },
                function () { return got; });
      });
    }, Promise.resolve({ url: "", built: 0 })).catch(function () { return { url: "", built: 0 }; });
  }

  // ---------- which game a folder is ----------
  // The games the listing page can add: {target, title, spec}, filled by import-games.js, which cmake/ByopopWeb.cmake writes from
  // each listed game's page.js
  var games = [];
  function add(target, title, makeSpec) {
    try { games.push({ target: target, title: title, spec: makeSpec() }); }
    catch (e) { try { console.warn("byo-pop: " + target + "'s page.js failed, so its folders cannot be added here:", e); } catch (x) {} }
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
  // ---------- games this site does not play ----------
  // ByopopOtherGames (other-games.js, which byopop_web_index() writes from cmake/ByopopOthers.cmake): [{id, title, markers, project,
  // url}], the games the site recognises but does not play, each with another project that plays it (project, url) or coming soon
  function otherList() {
    try { return typeof ByopopOtherGames !== "undefined" && Array.isArray(ByopopOtherGames) ? ByopopOtherGames : []; } catch (e) { return []; }
  }
  // The games of those whose markers the files hold, as [{game, root}]
  function otherMatches(entries) {
    var out = [];
    otherList().forEach(function (o) {
      var root = findRoot({ markers: o.markers, gameDirs: [] }, entries, false);
      if (root !== null) out.push({ game: o, root: root });
    });
    return out;
  }
  // The one game of those the files hold, or null
  function otherGame(entries) {
    var m = otherMatches(entries);
    return m.length ? m[0].game : null;
  }
  // What the import says of one: a <span> with the game's name and either a link to the project that plays it (a new tab, as the
  // store links) or "coming soon"
  function otherNote(doc, o) {
    var span = doc.createElement("span");
    if (!o.project || !/^https:\/\/github\.com\//.test(o.url || "")) {
      span.textContent = ByopopStrings.t("import.other.soon", {title: o.title});
      return span;
    }
    span.appendChild(doc.createTextNode(ByopopStrings.t("import.other.elsewhere", {title: o.title}) + " "));
    var a = doc.createElement("a");
    a.setAttribute("href", o.url);
    a.setAttribute("target", "_blank");
    a.setAttribute("rel", "noopener noreferrer");
    a.textContent = o.project;
    a.setAttribute("aria-label", o.project + " " + ByopopStrings.t("site.new_tab"));
    span.appendChild(a);
    return span;
  }

  // Sorts dropped or picked files into the games they are: {found: [{folder, entries, root, game, choices}], rejected: [folder],
  // others: [{folder, game}]}, others the folders that hold a game the site does not play (ByopopOtherGames, above).
  // A folder's game is the one whose markers it holds (the files say it more surely than the name). Where several games' markers
  // are there at the same place (a game and its sequel may share their files' names), the folder's name picks among them, and
  // where it cannot, game is null and choices are the games it could be, for the player to pick. A folder that holds no game but
  // holds games' folders (the "PopCap Games" folder) is looked into, three levels deep at most; a folder with no game in it is
  // rejected.
  function sort(entries) {
    var found = [], rejected = [], others = [];
    function visit(group, depth) {
      var name = group.path.split("/").pop() || ByopopStrings.t("import.dropped_files");
      var matches = [];
      games.forEach(function (g) {
        var root = findRoot(g.spec, group.entries, false);
        if (root !== null) matches.push({ game: g, root: root });
      });
      var roots = {};
      matches.forEach(function (m) { roots[m.root] = true; });
      if (matches.length > 0 && Object.keys(roots).length === 1) {
        var root = matches[0].root, best = null, top = 0, tie = false;
        // the game is in a subfolder of a folder that holds others: the subfolder is the row, and the others are named as not added
        if (depth < 3 && root.length > (group.path ? group.path.length + 1 : 0)) {
          var subs = groupBy(group.entries, depth + 1).filter(function (g) { return g.path !== group.path; });
          if (subs.length > 1) { subs.forEach(function (g) { visit(g, depth + 1); }); return; }
        }
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
      // none of the site's games: one it does not play is named; a folder of several of them is looked into as above
      var other = matches.length === 0 ? otherMatches(group.entries) : [];
      if (other.length > 0) {
        var otherRoots = {};
        other.forEach(function (m) { otherRoots[m.root] = true; });
        var subs = depth < 3 && Object.keys(otherRoots).length > 1 ? groupBy(group.entries, depth + 1).filter(function (g) { return g.path !== group.path; }) : [];
        if (subs.length > 0) { subs.forEach(function (g) { visit(g, depth + 1); }); return; }
        others.push({ folder: name, game: other[0].game });
        return;
      }
      rejected.push(name);
    }
    groupBy(entries, 0).forEach(function (g) { visit(g, 0); });
    return { found: found, rejected: rejected, others: others };
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
      r.onerror = function () { reject(r.error || new Error(ByopopStrings.t("import.err.open"))); };
      r.onblocked = function () { reject(new Error(ByopopStrings.t("import.err.busy"))); };
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
      tx.onerror = tx.onabort = function () { reject(tx.error || new Error(ByopopStrings.t("import.err.keep"))); };
    });
  }
  // Replaces the game folder (persist + "/game") with files [{rel, read}], reading and writing about 32 MB at a time.
  // progress(done, total) after each file; cleared() once the old copy is gone. The files go in their order, so the caller
  // puts last the ones the game page checks for (ordered): a copy that stops partway (the tab closed) is then not taken for the
  // game. A write that fails partway takes out what it wrote; one that fails before the old copy is gone leaves that copy.
  // Each batch writes the folder records of its own files, so a folder is not there before its files are.
  function writeGame(persist, files, progress, cleared) {
    var game = persist + "/game", now = new Date(), range = IDBKeyRange.bound(game + "/", game + "/\uffff"), dirs = {};
    function putDirs(store, rel) {
      var parts = rel.split("/");
      for (var i = 1; i < parts.length; i++) {
        var d = game + "/" + parts.slice(0, i).join("/");
        if (!dirs[d]) { dirs[d] = true; store.put({ timestamp: now, mode: DIR_MODE }, d); }
      }
    }
    return openDb(persist).then(function (db) {
      var tx = db.transaction(DB_STORE, "readwrite"), store = tx.objectStore(DB_STORE);
      store.delete(range);
      store.put({ timestamp: now, mode: DIR_MODE }, game);
      dirs[game] = true;
      return done(tx).then(function () {
        if (cleared) cleared();
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
            chunk.forEach(function (c) {
              putDirs(s, c.key.substring(game.length + 1));
              s.put({ timestamp: now, mode: FILE_MODE, contents: c.data }, c.key);
            });
            return done(t);
          }).then(function () { if (progress) progress(at, files.length); return batch(); });
        }
        return batch().catch(function (e) {
          // (a quota, a file of the drop that would not read): the part written goes, as the game page's Forget would take it
          var undo;
          try {
            var t = db.transaction(DB_STORE, "readwrite"), s = t.objectStore(DB_STORE);
            s.delete(game);
            s.delete(range);
            undo = done(t);
          } catch (x) { undo = Promise.resolve(); }
          return undo.catch(function () {}).then(function () { throw e; });
        });
      }).then(function () { db.close(); }, function (e) { try { db.close(); } catch (x) {} throw e; });
    });
  }
  // The files in the order writeGame writes them: last the game's markers (what the game page's hasGameFiles looks for), and
  // before them, for a game whose marker is a pak (it checks that a game folder holds something), the files of its game folders
  function ordered(spec, files) {
    var paks = spec.paks || [], pakMarker = false, res = [];
    spec.markers.forEach(function (m) {
      if (paks.indexOf(m.toLowerCase()) >= 0) pakMarker = true;
      else res.push(new RegExp("^" + markerPattern(m) + "$", "i"));
    });
    function rank(f) {
      if (res.some(function (re) { return re.test(f.rel); })) return 2;
      var i = f.rel.indexOf("/");
      return pakMarker && i > 0 && spec.gameDirs.indexOf(f.rel.substring(0, i)) >= 0 ? 1 : 0;
    }
    return files.map(function (f, i) { return { f: f, r: rank(f), i: i }; })
      .sort(function (a, b) { return a.r - b.r || a.i - b.i; }).map(function (x) { return x.f; });
  }
  // Runs fn while holding the game's Web Lock; rejects when the lock is held: by a page of the game (it holds the lock all its
  // life), or by this page's own work on the game (a restore, an add, the bin). A browser without Web Locks (a page not served
  // over https or from localhost) is refused: an open page of the game could not be seen, and its next save would delete the
  // files written here.
  var mine = {};
  function withLock(lock, title, fn) {
    if (!lock) return fn();
    if (!(navigator.locks && navigator.locks.request))
      return Promise.reject(new Error(ByopopStrings.t("import.err.no_locks", {title: title})));
    return new Promise(function (resolve, reject) {
      navigator.locks.request(lock, { ifAvailable: true }, function (held) {
        if (!held) {
          reject(new Error(ByopopStrings.t(mine[lock] ? "import.err.working" : "import.err.in_use", {title: title})));
          return null;
        }
        mine[lock] = true;
        return Promise.resolve().then(fn).then(function (v) { delete mine[lock]; resolve(v); }, function (e) { delete mine[lock]; reject(e); });
      }).catch(reject);
    });
  }
  // ---- whether the game's exe gives up what its port reads from it ----
  // The ports that read texts or tables out of their game's exe (GameText.cpp and the like) need it as a plain image, or as PopCap's
  // own launcher with the plain image appended. A copy whose exe is packed and encrypted (a store's own wrapper) would import
  // and then miss its texts, so the import checks it first (the user, 2026-11-11, of the Shockwave copies). The table
  // (web/exeprobes.js, ByopopExeProbes) names, for each such game, the exe's names and one or more probes, each the address of
  // one text the port reads, its length and the FNV-1a hash of its bytes (never the text): an exe passes when any probe holds.
  // tools/exeprobe.py is the same check.
  var POPCAP_MARKERS = ["!popcapdrmprotect!", "!popcapcdprotect!"];
  function u16(b, i) { return i + 2 <= b.length ? b[i] | (b[i + 1] << 8) : 0; }
  function u32(b, i) { return i + 4 <= b.length ? (b[i] | (b[i + 1] << 8) | (b[i + 2] << 16) | (b[i + 3] << 24)) >>> 0 : 0; }
  function find(b, text, from) {
    var n = text.length, c = text.charCodeAt(0);
    for (var i = from || 0; i + n <= b.length; i++) {
      if (b[i] !== c) continue;
      var k = 1;
      while (k < n && b[i + k] === text.charCodeAt(k)) k++;
      if (k === n) return i;
    }
    return -1;
  }
  // where the game's image starts: after PopCap's launcher marker, at its MZ, else at the file's start
  function imageBase(b) {
    for (var m = 0; m < POPCAP_MARKERS.length; m++) {
      var at = find(b, POPCAP_MARKERS[m]);
      if (at >= 0) { var mz = find(b, "MZ", at + POPCAP_MARKERS[m].length); return mz >= 0 ? mz : b.length; }
    }
    return 0;
  }
  // the file offset of a virtual address of the image at base, or -1
  function vaOffset(b, base, va) {
    if (b[base] !== 0x4d || b[base + 1] !== 0x5a) return -1;
    var pe = base + u32(b, base + 0x3c);
    if (u32(b, pe) !== 0x4550) return -1;
    var n = u16(b, pe + 6), sec = pe + 24 + u16(b, pe + 20), image = u32(b, pe + 24 + 28);
    for (var i = 0; i < n; i++, sec += 40) {
      var start = image + u32(b, sec + 12), size = u32(b, sec + 16);
      if (va >= start && va < start + size) return base + u32(b, sec + 20) + (va - start);
    }
    return -1;
  }
  function fnv1a(b, at, n) {
    var h = 2166136261;
    for (var i = 0; i < n; i++) { h ^= b[at + i]; h = Math.imul(h, 16777619) >>> 0; }
    return h >>> 0;
  }
  function probeHolds(b, p) {
    var at = vaOffset(b, imageBase(b), p.va);
    return at >= 0 && at + p.len < b.length && b[at + p.len] === 0 && fnv1a(b, at, p.len) === (p.fnv >>> 0);
  }
  // Resolves with null when the game reads no exe, when none of its exe's names is among the files (the markers say what is
  // missing) or when a probe holds; else with the exe's name as the files have it, for the message
  function exeUnreadable(id, files) {
    var t = (typeof ByopopExeProbes !== "undefined" && ByopopExeProbes[id]) || null;
    if (!t || !t.probes || !t.probes.length) return Promise.resolve(null);
    var names = t.files.map(function (n) { return n.toLowerCase(); }), exe = null;
    for (var i = 0; i < names.length && !exe; i++)
      exe = files.filter(function (f) { return f.rel.toLowerCase() === names[i]; })[0] || null;
    if (!exe) return Promise.resolve(null);
    return Promise.resolve(exe.read()).then(function (data) {
      var b = data instanceof Uint8Array ? data : new Uint8Array(data);
      return t.probes.some(function (p) { return probeHolds(b, p); }) ? null : exe.rel;
    }, function () { return null; });
  }

  // Adds one game's files from the listing page: g a games entry, entries the folder's files, root its game root (sort, or
  // findRoot). Resolves with the number of files; rejects with an Error to show. pak and safeRel: web/modpanel.js's. say(text)
  // reports the steps. The record of the game's files (ByopopGameFiles) is kept as the game page keeps it after an import.
  function addGame(g, entries, root, pak, safeRel, say) {
    var spec = g.spec;
    return withLock(spec.lock, g.title, function () {
      return withPaks(spec, entries, root, pak, function () { if (say) say(ByopopStrings.t("import.say.unpacking")); }).then(function (all) {
        var files = pick(spec, all, root, safeRel);
        if (!hasMarkers(spec, files.map(function (f) { return f.rel; })))
          throw new Error(ByopopStrings.t("import.err.wrong_folder", {title: g.title, markers: missingIn(spec, files.map(function (f) { return f.rel; }))}));
        return exeUnreadable(spec.id, files).then(function (bad) {
        if (bad) throw new Error(ByopopStrings.t("import.err.exe", {exe: bad}));
        if (say) say(ByopopStrings.t("import.say.copying", {count: files.length}));
        return writeGame(spec.persist, ordered(spec, files), function (n, total) { if (say) say(ByopopStrings.t("import.say.progress", {percent: Math.round(n * 100 / total)})); },
          function () { try { ByopopGameFiles.replacing(spec.id); } catch (e) {} })  // the old copy is gone
          .then(function () { return icon(spec, all, root, files); })
          .then(function (got) {
            try { ByopopGameFiles.mark(spec.id); if (got.url) ByopopGameFiles.setIcon(spec.id, got.url, got.built, got.small); } catch (e) {}
            try { if (navigator.storage && navigator.storage.persist) navigator.storage.persist().catch(function () {}); } catch (e) {}
            return files.length;
          });
        });
      });
    });
  }
  // Forgets a game's files from the listing page, as the game page's "Forget game files" does: the game folder's records go (saves,
  // mods and replays stay), and the record of the files says when they were forgotten. Held under the game's Web Lock, like addGame.
  function forgetGame(g) {
    var spec = g.spec, game = spec.persist + "/game";
    return withLock(spec.lock, g.title, function () {
      return openDb(spec.persist).then(function (db) {
        var tx = db.transaction(DB_STORE, "readwrite"), store = tx.objectStore(DB_STORE);
        store.delete(game);
        store.delete(IDBKeyRange.bound(game + "/", game + "/\uffff"));
        return done(tx).then(function () { db.close(); }, function (e) { try { db.close(); } catch (x) {} throw e; });
      }).then(function () { try { ByopopGameFiles.forget(spec.id); } catch (e) {} });
    });
  }
  // The game's icon for the record: from the install's exe (importIcon), or from the exe it stores (the largest of its .exe files)
  function icon(spec, all, root, files) {
    if (spec.importIcon) return exeIcon(all, root);
    var none = { url: "", built: 0 };
    if (!window.ByopopExeIcon) return Promise.resolve(none);
    var exes = files.filter(function (f) { return /\.exe$/i.test(f.rel) && f.rel.indexOf("/") < 0; });
    return Promise.all(exes.map(function (f) { return f.read(); })).then(function (datas) {
      var big = null;
      datas.forEach(function (d) { if (!big || d.length > big.length) big = d; });
      return big ? ByopopExeIcon.fromBytes(big).then(function (i) {
        return { url: (i && i.dataUrl) || "", built: ByopopExeIcon.built(big), small: (i && i.small) || "" };
      }) : none;
    }).catch(function () { return none; });
  }

  return { exeUnreadable: exeUnreadable, probeHolds: probeHolds, markerPattern: markerPattern, findRoot: findRoot, withPaks: withPaks, pick: pick, hasMarkers: hasMarkers, exeIcon: exeIcon,
    games: games, add: add, sort: sort, otherGame: otherGame, otherNote: otherNote, nameScore: nameScore, fromDrop: fromDrop, hasFiles: hasFiles, writeGame: writeGame, addGame: addGame,
    forgetGame: forgetGame, ordered: ordered, missingIn: missingIn, missingMarkers: missingMarkers, markerList: markerList };
})();
