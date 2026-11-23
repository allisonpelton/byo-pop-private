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

  // Whether the files (paths relative to the game folder, in any case) show the game's paks already unpacked, as a staged copy or a
  // zip of the unpacked game has them: every file of spec.unpacked where the page names them (a game whose folders' names are
  // common ones, Venice's "gfx" or "levels", names a few files only its pak holds; the user, 2026-11-16: "Fix it now", of
  // another game's config.xml beside a gfx folder taken as Venice), else any file in one of its folders.
  // relOf(x): the path of an item of list relative to the game folder, or null for one outside it
  function unpackedIn(spec, list, relOf) {
    if (!spec.unpacked) return list.some(function (x) {
      var r = relOf(x), i = r === null ? -1 : r.indexOf("/");
      return i > 0 && spec.gameDirs.indexOf(r.substring(0, i).toLowerCase()) >= 0;
    });
    return spec.unpacked.every(function (f) {
      return list.some(function (x) { var r = relOf(x, f.length); return r !== null && r.length === f.length && r.toLowerCase() === f; });
    });
  }
  function itself(r) { return r; }

  // The folder inside the files that holds every marker (the game root, "" or ending in "/"), or null; with placeholders,
  // failing that, the shallowest folder that holds one of the game's folders. A marker that is a pak (spec.paks) is there when
  // the pak is, or when the folder's files show it unpacked (unpackedIn; hasMarkers does the same for picked files).
  // among (sort's): the entries whose names may be a marker's (markerNames, below), in their order; the markers are looked for
  // only among them, and the root is the same as when every entry is looked at
  function findRoot(spec, entries, placeholders, among) {
    var paks = spec.paks || [], marks = among || entries;
    function isPak(m) { return paks.indexOf(m.toLowerCase()) >= 0; }
    var anyDepth = spec.markers.map(function (m) { return new RegExp("^(.*/)?" + markerPattern(m) + "$", "i"); });
    var atRoot = spec.markers.map(function (m) { return new RegExp("^" + markerPattern(m) + "$", "i"); });
    var allPaks = spec.markers.every(isPak), tried = {};
    function holds(root) {
      var inRoot = marks.filter(function (e) { return under(e, root); });
      return spec.markers.every(function (m, k) {
        if (inRoot.some(function (e) { return atRoot[k].test(e.path.substring(root.length)); })) return true;
        // length: a file of spec.unpacked is looked for, so only a path of its length is cut; those files are among the marks
        // (markerNames holds their names)
        return isPak(m) && unpackedIn(spec, spec.unpacked ? marks : entries, function (e, length) {
          if (length !== undefined && e.path.length !== root.length + length) return null;
          return under(e, root) ? e.path.substring(root.length) : null;
        });
      });
    }
    for (var i = 0; i < marks.length; i++) {
      var roots = [];
      anyDepth.forEach(function (re, k) {
        // a pak marker names a root only when every marker is one (else the other markers name it)
        if (isPak(spec.markers[k]) && !allPaks) return;
        var m = re.exec(marks[i].path);
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
  var README = /^read ?me\.(html?|txt)$/i;
  // Files a port never opens, left out of the stored copy by game id (the user, 2026-11-13: "Let's do the first two", of the
  // ways to store less; docs/WEB_PLAYER.md, "What happens when a game's page opens"). Only these: the static check
  // tools/web/unreferenced.py finds about 21 MB of 381 named by nothing, but most of the rest is reached by names the code
  // builds in ways a search cannot be sure of, so only what is checked against the port by hand goes. The 2004 framework's
  // originals decode each Ogg sound once and write it beside it as cached_<name>.wav (docs/FRAMEWORK_2004.md, 1.3); AstroPop's
  // and Dynomite's ports decode the Ogg itself and never ask for the copy (their GAME_NOTES and staging), so the 67 and 38 of
  // them, 11.7 and 3.7 MB of the Steam copies, stay out. Rocket Mania's port reads its music's cached_ files, so it is not here.
  var NEVER_OPENED = {
    astropop: [/^sounds\/cached_[^\/]*\.wav$/i],
    dynomite: [/^sounds\/cached_[^\/]*\.wav$/i]
  };
  function pick(spec, entries, root, safeRel) {
    var skip = (spec.skip || []).concat(NEVER_OPENED[spec.id] || []), files = spec.gameFiles || [], wanted = [];
    entries.forEach(function (e) {
      if (!under(e, root)) return;
      var rel = safeRel(e.path.substring(root.length));
      if (!rel || /\.sig$/i.test(rel) || skip.some(function (re) { return re.test(rel); })) return;
      var top = rel.split("/")[0];
      if (spec.gameDirs.indexOf(top.toLowerCase()) >= 0) wanted.push({ rel: top.toLowerCase() + rel.substring(top.length), read: e.read });
      else if (files.indexOf(rel.toLowerCase()) >= 0) wanted.push({ rel: rel, read: e.read });
      // the game's own readme beside its exe, which the desktop's game window shows in a text window (Help, web/deskapps.js)
      else if (README.test(rel)) wanted.push({ rel: rel.toLowerCase(), read: e.read });
    });
    return wanted;
  }

  // Whether the picked files hold every marker: a pak that is a marker was unpacked, so then its files will do (unpackedIn)
  function hasMarkers(spec, rels) {
    var paks = spec.paks || [];
    var unpacked = paks.length ? unpackedIn(spec, rels, itself) : false;
    return spec.markers.every(function (m) {
      if (paks.indexOf(m.toLowerCase()) >= 0) return unpacked;
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
      return !hasMarkers({ markers: [m], paks: spec.paks, gameDirs: spec.gameDirs, unpacked: spec.unpacked }, rels);
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
  // A port still in progress whose page this build made (import-games.js names it so): added only where web/localsoon.js opens its
  // tile, a development host without ?localsoon=0, so the import takes exactly the games the listing opens. Elsewhere it stays
  // one of the games this site does not play (other-games.js). The user, 2026-11-23: Peggle's, Banana Bugs' and Feeding Frenzy's
  // folders were "not supported" while their tiles said they needed files.
  function addSoon(target, title, makeSpec) {
    var open = false;
    try {
      open = typeof ByopopLocalSoon !== "undefined" && typeof ByopopTerms !== "undefined" && typeof location !== "undefined" &&
        ByopopLocalSoon.enabled(location, ByopopTerms.devHost);
    } catch (e) {}
    if (open) add(target, title, makeSpec);
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
  // The files under a folder, with the folder's own part of each path left on: the first depth + 1 parts of each path, or its
  // first depth parts for a file with no more than that (the slashes counted, rather than the path split, as this runs over
  // every file of a big drop)
  function groupKey(p, depth) {
    var n = 0, s = -1, cut = -1;
    while (n <= depth) {
      s = p.indexOf("/", s + 1);
      if (s < 0) break;
      n++;
      if (n === depth) cut = s;
    }
    if (n === depth + 1) return p.substring(0, s);
    return depth === 0 ? "" : n === depth ? p.substring(0, cut) : p;
  }
  // A grouping of group's entries (and of its marks, the entries that may be markers, sort's) by groupKey, done a part at a time
  // by groupSome; groups(g) once every entry is in
  function grouping(group, depth) {
    return { entries: group.entries, marks: group.marks || [], depth: depth, at: 0, out: Object.create(null), order: [] };
  }
  function groupSome(g, count) {
    for (var end = Math.min(g.entries.length, g.at + count); g.at < end; g.at++) {
      var e = g.entries[g.at], key = groupKey(e.path, g.depth);
      if (!(key in g.out)) { g.out[key] = []; g.order.push(key); }
      g.out[key].push(e);
    }
    return g.at < g.entries.length;
  }
  function groups(g) {
    var marks = Object.create(null);
    g.marks.forEach(function (e) { var key = groupKey(e.path, g.depth); (marks[key] = marks[key] || []).push(e); });
    return g.order.map(function (k) { return { path: k, entries: g.out[k], marks: marks[k] || [] }; });
  }
  // The last names of every game's markers, the site's and the others', in lower case (a marker's "x.*" kept as its stem x), and
  // of the files that show a game's paks unpacked (spec.unpacked): a file named none of them is no game's marker, so the sort
  // looks for markers only among the few files that are
  var markerIndex = null;
  function markerNames() {
    var list = otherList(), key = games.length + "/" + list.length;
    if (markerIndex && markerIndex.key === key) return markerIndex;
    var exact = Object.create(null), stems = Object.create(null);
    function addMarker(m) {
      m.split("|").forEach(function (alt) {
        var last = alt.substring(alt.lastIndexOf("/") + 1).toLowerCase();
        if (/\.\*$/.test(last)) stems[last.slice(0, -2)] = true; else exact[last] = true;
      });
    }
    games.forEach(function (g) { g.spec.markers.concat(g.spec.unpacked || []).forEach(addMarker); });
    list.forEach(function (o) { o.markers.forEach(addMarker); });
    markerIndex = { key: key, exact: exact, stems: stems };
    return markerIndex;
  }
  function mayBeMarker(names, path) {
    var name = path.substring(path.lastIndexOf("/") + 1).toLowerCase(), dot = name.lastIndexOf(".");
    return names.exact[name] === true || (dot >= 0 && names.stems[name.substring(0, dot)] === true);
  }
  // ---------- games this site does not play ----------
  // ByopopOtherGames (other-games.js, which byopop_web_index() writes from cmake/ByopopOthers.cmake): [{id, title, markers, project,
  // url}], the games the site recognises but does not play, each with another project that plays it (project, url) or coming soon
  function otherList() {
    try { return typeof ByopopOtherGames !== "undefined" && Array.isArray(ByopopOtherGames) ? ByopopOtherGames : []; } catch (e) { return []; }
  }
  // Whether another game's own files are where a game of the site's was found (m: {game, root}), so that the folder is that other
  // game: Peggle Extreme's folder holds Peggle's markers (main.pak, music/PeggleBeats.mo3) as well as its own PeggleExtreme.exe.
  // A row of the others with the game's own id is the game itself, and never counts.
  function otherHere(m, entries, among) {
    return otherList().some(function (o) {
      return o.id !== m.game.target && o.id !== m.game.spec.id &&
        findRoot({ markers: o.markers, gameDirs: [] }, entries, false, among) === m.root;
    });
  }
  // The site's game a folder is named as, by its name alone (nameScore 3), or null: a folder named so that holds none of the
  // game's markers is that game with files missing, which the import names, not a folder of no game
  function namedGame(folder) {
    var best = null, tie = false;
    games.forEach(function (g) {
      if (nameScore(folder, g) < 3) return;
      if (best) tie = true; else best = g;
    });
    return tie ? null : best;
  }
  // The games of those whose markers the files hold, as [{game, root}]; among as findRoot's
  function otherMatches(entries, among) {
    var out = [];
    otherList().forEach(function (o) {
      var root = findRoot({ markers: o.markers, gameDirs: [] }, entries, false, among);
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
  // The sort is made of steps, each short (a few thousand files looked at, or one folder sorted), so that sortSteps lets the page
  // draw between them: a whole Steam library (the user's, 2026-11-16: 146,409 files in 24,158 folders) held the page for ten
  // seconds at once. The markers are looked for only among the files named as one is (markerNames).
  function sorter(entries) {
    var found = [], rejected = [], others = [], lacking = [];
    var names = markerNames(), marked = [], at = 0, todo = null;
    // the work still to do, the next last: folders to visit, and folders being grouped into their subfolders (then(subs) once
    // they are, with the subfolders only); each step does one, so the results come in the order a visit of each folder in turn,
    // grouped at once, gives them
    function later(subs, depth) { for (var i = subs.length - 1; i >= 0; i--) todo.push({ group: subs[i], depth: depth }); }
    function grouped(group, depth, then) {
      todo.push({ grouping: grouping(group, depth), then: function (all) { then(all.filter(function (g) { return g.path !== group.path; })); } });
    }
    function visit(group, depth) {
      var name = group.path.split("/").pop() || ByopopStrings.t("import.dropped_files");
      var matches = [];
      games.forEach(function (g) {
        var root = findRoot(g.spec, group.entries, false, group.marks);
        if (root !== null) matches.push({ game: g, root: root });
      });
      matches = matches.filter(function (m) { return !otherHere(m, group.entries, group.marks); });
      var roots = {};
      matches.forEach(function (m) { roots[m.root] = true; });
      if (matches.length > 0 && Object.keys(roots).length === 1) {
        var root = matches[0].root;
        // the game is in a subfolder of a folder that holds others: the subfolder is the row, and the others are named as not added
        if (depth < 3 && root.length > (group.path ? group.path.length + 1 : 0)) {
          grouped(group, depth + 1, function (subs) { if (subs.length > 1) later(subs, depth + 1); else foundAt(group, name, root, matches); });
          return;
        }
        foundAt(group, name, root, matches);
        return;
      }
      // (no game anywhere in it: findRoot looks at every depth)
      if (matches.length > 0 && depth < 3) {
        grouped(group, depth + 1, function (deeper) { if (deeper.length > 0) later(deeper, depth + 1); else noneOfOurs(group, depth, name, matches); });
        return;
      }
      noneOfOurs(group, depth, name, matches);
    }
    function foundAt(group, name, root, matches) {
      var best = null, top = 0, tie = false;
      matches.forEach(function (m) {
        var s = Math.max(nameScore(name, m.game), nameScore(root.replace(/\/$/, "").split("/").pop(), m.game));
        if (s > top) { top = s; best = m.game; tie = false; } else if (s === top && s > 0) tie = true;
      });
      var game = matches.length === 1 ? matches[0].game : (top > 0 && !tie ? best : null);
      found.push({ folder: name, entries: group.entries, root: root, game: game, choices: matches.map(function (m) { return m.game; }) });
    }
    // none of the site's games: one it does not play is named; a folder of several of them is looked into as above
    function noneOfOurs(group, depth, name, matches) {
      var other = matches.length === 0 ? otherMatches(group.entries, group.marks) : [];
      if (other.length > 0) {
        var otherRoots = {};
        other.forEach(function (m) { otherRoots[m.root] = true; });
        if (depth < 3 && Object.keys(otherRoots).length > 1) {
          grouped(group, depth + 1, function (subs) { if (subs.length > 0) later(subs, depth + 1); else others.push({ folder: name, game: other[0].game }); });
          return;
        }
        others.push({ folder: name, game: other[0].game });
        return;
      }
      // a folder named as one of the site's games, without its files: the game and the markers it lacks (the user, 2026-11-23,
      // asked that the import name what is missing rather than call the game unsupported)
      var named = matches.length === 0 ? namedGame(name) : null;
      if (named) { lacking.push({ folder: name, game: named, missing: missingMarkers(named.spec, group.entries) }); return; }
      rejected.push(name);
    }
    return {
      // one step: a few thousand files looked at for markers' names or put in their folders, or one folder visited; true while
      // there is more
      step: function () {
        if (at < entries.length) {
          for (var end = Math.min(entries.length, at + 8192); at < end; at++)
            if (mayBeMarker(names, entries[at].path)) marked.push(entries[at]);
          return true;
        }
        if (!todo) {
          todo = [{ grouping: grouping({ entries: entries, marks: marked }, 0), then: function (all) { later(all, 0); } }];
          return true;
        }
        var t = todo.pop();
        if (!t) return false;
        if (!t.grouping) visit(t.group, t.depth);
        else if (groupSome(t.grouping, 8192)) todo.push(t);
        else t.then(groups(t.grouping));
        return todo.length > 0;
      },
      result: function () { return { found: found, rejected: rejected, others: others, lacking: lacking }; }
    };
  }
  function sort(entries) {
    var s = sorter(entries);
    while (s.step()) { /* to the end */ }
    return s.result();
  }
  // The sort in steps, about 16 ms of them at a time, the page drawing between: resolves with what sort gives
  function sortSteps(entries) {
    var s = sorter(entries);
    return new Promise(function (resolve, reject) {
      function run() {
        try {
          var until = Date.now() + 16;
          while (s.step()) if (Date.now() >= until) { yieldThen(run); return; }
          resolve(s.result());
        } catch (e) { reject(e); }
      }
      yieldThen(run);
    });
  }
  // fn after the page has had its turn: a message to itself, which a hidden tab does not slow down as it does a timer
  function yieldThen(fn) {
    if (typeof MessageChannel !== "function") { setTimeout(fn, 0); return; }
    var c = new MessageChannel();
    c.port1.onmessage = function () { c.port1.close(); fn(); };
    c.port2.postMessage(0);
  }

  // ---------- installers ----------
  // A game's installer dropped in place of the folder it installs to (the user, 2026-11-16: "detect when an installer was
  // dragged in and provide explicit direction to the user against that"). The site takes a game's installed files; it never
  // runs or unpacks an installer, only reads the first 4 MB and the last 64 KB of the file for its marks. A program is one
  // when its name says so (setup, install or installer anywhere in it: setup.exe, Alchemy_Setup.exe, ZumaDeluxeInstaller.exe;
  // never an uninstaller, DirectX's setup or a redistributable) or it carries one of these marks:
  //   msi            a Windows Installer package: the compound file header D0 CF 11 E0 A1 B1 1A E1 at its start
  //   inno           Inno Setup: "Inno" at offset 0x30 (5.1.5 on), its loader's "rDlPtS" table, or "Inno Setup" in ASCII or
  //                  UTF-16 (its version text, "This installation was built with Inno Setup.")
  //   nsis           NSIS: its first header's "NullsoftInst", or "Nullsoft Install System"
  //   installshield  InstallShield: "InstallShield" in ASCII or UTF-16 (its version text)
  //   wise           Wise: a PE section named .WISE, or "Wise Installation" in ASCII or UTF-16
  //   sfx            a self-extracting archive: a 7-Zip ("7z" BC AF 27 1C) or RAR ("Rar!" 1A 07) archive or a cabinet ("MSCF" and
  //                  four zero bytes, IExpress's) inside it, "WinZip Self-Extractor", or a zip's end record ("PK" 05 06) in its
  //                  last 64 KB
  // (all but msi in a program, which starts "MZ")
  var NOT_INSTALLER = /^(popuninstall|uninst|unins\d|dxsetup|.*redist)/i;
  var HEAD_BYTES = 4 * 1048576, TAIL_BYTES = 65558;
  function wide(text) { return text.replace(/[\s\S]/g, "$&\0"); }
  function holdsText(b, text) { return find(b, text) >= 0 || find(b, wide(text)) >= 0; }
  function sectionNamed(b, name) {
    if (b[0] !== 0x4d || b[1] !== 0x5a) return false;
    var pe = u32(b, 0x3c);
    if (u32(b, pe) !== 0x4550) return false;
    var n = u16(b, pe + 6), sec = pe + 24 + u16(b, pe + 20);
    for (var i = 0; i < n && sec + 8 <= b.length; i++, sec += 40) {
      var k = 0;
      while (k < 8 && k < name.length && b[sec + k] === name.charCodeAt(k)) k++;
      if (k === name.length && (k === 8 || b[sec + k] === 0)) return true;
    }
    return false;
  }
  function installerMark(head, tail) {
    if (u32(head, 0) === 0xe011cfd0 && u32(head, 4) === 0xe11ab1a1) return "msi";
    if (head[0] !== 0x4d || head[1] !== 0x5a) return null;
    if (u32(head, 0x30) === 0x6f6e6e49 || find(head, "rDlPtS") >= 0 || holdsText(head, "Inno Setup")) return "inno";
    if (find(head, "NullsoftInst") >= 0 || holdsText(head, "Nullsoft Install System")) return "nsis";
    if (holdsText(head, "InstallShield")) return "installshield";
    if (sectionNamed(head, ".WISE") || holdsText(head, "Wise Installation")) return "wise";
    if (find(head, "7z\xbc\xaf\x27\x1c") >= 0 || find(head, "Rar!\x1a\x07") >= 0 || find(head, "MSCF\0\0\0\0") >= 0 ||
      holdsText(head, "WinZip Self-Extractor") || find(tail, "PK\x05\x06") >= 0) return "sfx";
    return null;
  }
  function baseName(p) { return p.substring(p.lastIndexOf("/") + 1); }
  // A file's first and last bytes: through part(start, end) where the entry has it (a drop's, a pick's), else from all of it
  function ends(e) {
    if (e.part) return Promise.all([e.part(0, HEAD_BYTES), e.part(-TAIL_BYTES)]);
    return Promise.resolve(e.read()).then(function (d) {
      var b = d instanceof Uint8Array ? d : new Uint8Array(d);
      return [b.subarray(0, HEAD_BYTES), b.subarray(Math.max(0, b.length - TAIL_BYTES))];
    });
  }
  // The game an installer's name (or its folder's) names: of the site's games (game), else of the others (other), the one named
  // best, or null
  // whole: only a name that is the game's (a folder's name; a file's may also hold the game's, or be held in it, at four
  // letters or more: "PopCapAlchemyDeluxe")
  function gameNamed(file, whole) {
    var name = file.replace(/\.(exe|msi)$/i, "").replace(/[-_. ]*(setup|installer|install)[-_. ]*/gi, " ");
    var least = whole || plain(name).length < 4 ? 3 : 2;
    function best(list) {
      var got = null, top = 0, tie = false;
      list.forEach(function (c) {
        var s = nameScore(name, c.as);
        if (s > top) { top = s; got = c; tie = false; } else if (s === top && s > 0) tie = true;
      });
      return top >= least && !tie ? got : null;
    }
    return best(games.map(function (g) { return { game: g, as: g }; })) ||
      best(otherList().map(function (o) { return { other: o, as: { title: o.title, target: o.id, spec: {} } }; }));
  }
  // Where a game's copy usually is, from its page's import text (its first path): {path, steam}, or null
  function usualFolder(spec) {
    var m = /<span class="mono">([^<]*)<\/span>/.exec(spec.importHtml || "");
    if (!m) return null;
    var p = m[1].replace(/&#39;|&apos;/g, "'").replace(/&quot;/g, '"').replace(/&lt;/g, "<").replace(/&gt;/g, ">").replace(/&amp;/g, "&")
      .replace(/\//g, "\\");
    if (/^steamapps\\/i.test(p)) return { path: p, steam: true };
    return { path: "C:\\Program Files (x86)\\" + p.replace(/^Program Files\\/i, ""), steam: false };
  }
  // The installer in a drop the sort found no game in: a program alone or with a few other files (12 files at most, 3 programs
  // at most), or the only program in the drop (a .zip of an installer). Resolves with {entry, file, mark ("name" or a mark
  // above), group (the sort's name for the folder it is in), game (a site's game its name names) or other (one the site does
  // not play), folder (game's usualFolder)}, or null.
  function installerIn(entries, sorted) {
    if (sorted.found.length || (sorted.others || []).length) return Promise.resolve(null);
    var programs = entries.filter(function (e) { return /\.(exe|msi)$/i.test(e.path) && !NOT_INSTALLER.test(baseName(e.path)); });
    if (!programs.length || !(programs.length === 1 || (entries.length <= 12 && programs.length <= 3))) return Promise.resolve(null);
    function result(e, mark) {
      // the game its name names, else the one its folder's (or its .zip's) name names
      var file = baseName(e.path), at = e.path.indexOf("/"), up = e.path.lastIndexOf("/");
      var named = gameNamed(file, false) || (up > 0 ? gameNamed(baseName(e.path.substring(0, up)), true) : null);
      return { entry: e, file: file, mark: mark, group: at < 0 ? ByopopStrings.t("import.dropped_files") : e.path.substring(0, at),
        game: named && named.game || null, other: named && named.other || null, folder: named && named.game ? usualFolder(named.game.spec) : null };
    }
    return programs.reduce(function (p, e) {
      return p.then(function (got) {
        if (got) return got;
        if (/setup|install/i.test(baseName(e.path))) return result(e, "name");
        return ends(e).then(function (two) {
          var mark = installerMark(two[0], two[1]);
          return mark ? result(e, mark) : null;
        }, function () { return null; });
      });
    }, Promise.resolve(null));
  }
  // What a drop's installer row says: what it is and what to do instead
  function installerText(inst) {
    if (!inst.game) return ByopopStrings.t("import.installer", { file: inst.file });
    var vars = { file: inst.file, title: inst.game.title, folder: inst.folder ? inst.folder.path : "" };
    if (!inst.folder) return ByopopStrings.t("import.installer_game_any", vars);
    return ByopopStrings.t(inst.folder.steam ? "import.installer_steam" : "import.installer_game", vars);
  }

  // A drop's sort for its reading row (Setup's, the panel's): sortSteps, then installerIn. An installer found takes the place of
  // the folder it is in, as installer (installerIn's, with its text), or, where its name names a game this site does not play,
  // is named as that game is
  function sortDrop(entries) {
    return sortSteps(entries).then(function (sorted) {
      return installerIn(entries, sorted).then(null, function () { return null; }).then(function (inst) {
        if (!inst) return sorted;
        var rejected = sorted.rejected.slice(), at = rejected.indexOf(inst.group);
        if (at >= 0) rejected.splice(at, 1);
        // (a folder named as a game that holds its installer: the installer's row says it, not a row of missing files)
        var lacking = (sorted.lacking || []).filter(function (l) { return l.folder !== inst.group; });
        if (inst.other) return { found: sorted.found, others: (sorted.others || []).concat([{ folder: inst.file, game: inst.other }]), rejected: rejected, lacking: lacking };
        inst.text = installerText(inst);
        return { found: sorted.found, others: sorted.others || [], rejected: rejected, lacking: lacking, installer: inst };
      });
    });
  }

  // ---------- reading a drop ----------
  // The files of a drop as {path, read()}: a dropped folder's paths start with its name, and a dropped .zip's with its name
  // without ".zip". Must be called in the drop event itself (the entries are only readable during it). zip: web/modpanel.js's.
  // watch (optional, watch() below): told the folders listed and the files found as the walk goes, and asked before each folder
  // whether to stop, when the walk rejects with an Error whose cancelled is true. Every folder is listed, however deep: a game is
  // known by its files at any depth, so no folder can be passed over before its files are seen.
  function fromDrop(dataTransfer, zip, watch) {
    var tops = [];
    Array.prototype.forEach.call(dataTransfer.items || [], function (item) {
      if (item.kind !== "file") return;
      var entry = item.webkitGetAsEntry ? item.webkitGetAsEntry() : null;
      if (entry) tops.push(entry);
      else { var f = item.getAsFile(); if (f) tops.push({ isFile: true, name: f.name, fullPath: "/" + f.name, plainFile: f }); }
    });
    var out = [], folders = 0;
    function told() { if (watch && watch.progress) watch.progress(folders, out.length); }
    function stop() {
      if (!(watch && watch.stopped && watch.stopped())) return null;
      var e = new Error(ByopopStrings.t("import.cancelled"));
      e.cancelled = true;
      return e;
    }
    function fileOf(entry) {
      if (entry.plainFile) return Promise.resolve(entry.plainFile);
      return new Promise(function (resolve, reject) { entry.file(resolve, reject); });
    }
    function walk(entry) {
      var stopped = stop();
      if (stopped) return Promise.reject(stopped);
      if (entry.isFile) {
        var path = entry.fullPath.replace(/^\//, "");
        if (/\.zip$/i.test(entry.name) && path.indexOf("/") < 0) {
          return fileOf(entry).then(function (f) { return zip.fromFile(f); }).then(function (files) {
            var base = entry.name.replace(/\.zip$/i, "");
            files.forEach(function (z) { out.push({ path: base + "/" + z.path, read: z.read }); });
            told();
          });
        }
        out.push({ path: path, read: function () { return fileOf(entry).then(function (f) { return f.arrayBuffer(); }).then(function (b) { return new Uint8Array(b); }); },
          // a part of it (installerIn reads a program's ends), as File.slice takes start and end
          part: function (start, end) { return fileOf(entry).then(function (f) { return f.slice(start, end).arrayBuffer(); }).then(function (b) { return new Uint8Array(b); }); } });
        return Promise.resolve();
      }
      if (!entry.isDirectory) return Promise.resolve();
      var reader = entry.createReader();
      return new Promise(function (resolve, reject) {
        var all = [];
        (function more() {
          var stopped = stop();
          if (stopped) { reject(stopped); return; }
          reader.readEntries(function (batch) {
            if (!batch.length) { resolve(all); return; }
            for (var i = 0; i < batch.length; i++) all.push(batch[i]);
            more();
          }, reject);
        })();
      }).then(function (children) {
        folders++;
        told();
        return children.reduce(function (p, c) { return p.then(function () { return walk(c); }); }, Promise.resolve());
      });
    }
    return tops.reduce(function (p, t) { return p.then(function () { return walk(t); }); }, Promise.resolve())
      .then(function () { told(); return out; });
  }
  // A walk's watch for fromDrop: listen(fn) has fn(folders, files) called with the walk's count at most four times a second, and
  // cancel() stops the walk at its next folder
  function watch() {
    var folders = 0, files = 0, stopped = false, fn = null, timer = 0;
    function show() { timer = 0; if (fn) fn(folders, files); }
    return {
      progress: function (f, n) { folders = f; files = n; if (fn && !timer) timer = setTimeout(show, 250); },
      stopped: function () { return stopped; },
      cancel: function () { stopped = true; },
      listen: function (f) { fn = f; if (!f && timer) { clearTimeout(timer); timer = 0; } }
    };
  }
  // What a reading row says while a drop is read: the folders and files found so far, the numbers as the page's language writes them
  function readingText(folders, files) {
    function num(n) { try { return n.toLocaleString(ByopopStrings.lang); } catch (e) { return String(n); } }
    return ByopopStrings.t("import.reading_count", {
      folders: ByopopStrings.t("import.count.folders", { n: folders, count: num(folders) }),
      files: ByopopStrings.t("import.count.files", { n: files, count: num(files) }) });
  }
  // The folders with no game: listed one by one up to MANY_REJECTED - 1 of them; from MANY_REJECTED on, as one line with their
  // names behind a Details button (a whole Steam library has about ninety)
  var MANY_REJECTED = 4;
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
  // progress(done, total) after each batch; cleared() once the old copy is gone; each(rel, bytes, done, total), if given, as each
  // file is read (the desktop's copy dialog, ByopopCopy). The files go in their order, so the caller
  // puts last the ones the game page checks for (ordered): a copy that stops partway (the tab closed) is then not taken for the
  // game. A write that fails partway takes out what it wrote; one that fails before the old copy is gone leaves that copy.
  // Each batch writes the folder records of its own files, so a folder is not there before its files are.
  function writeGame(persist, files, progress, cleared, each) {
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
            return f.read().then(function (data) {
              chunk.push({ key: game + "/" + f.rel, data: data }); bytes += data.length;
              if (each) each(f.rel, data.length, at, files.length);
              return readMore();
            });
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
  // The files as they are stored: a game's exe without its code (web/exetrim.js), once the import has checked the original
  function stored(id, files) {
    var X = typeof ByopopExeTrim !== "undefined" ? ByopopExeTrim : null;
    if (!X || !X.TRIM[id]) return files;
    return files.map(function (f) {
      if (String(f.rel).toLowerCase() !== X.TRIM[id].exe) return f;
      return { rel: f.rel, read: function () {
        return Promise.resolve(f.read()).then(function (d) { return X.stored(id, f.rel, d instanceof Uint8Array ? d : new Uint8Array(d)); });
      } };
    });
  }
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

  // A game whose pointer is an animation of its files (web/flicursor.js) gives its set for the site's cursor as its files are
  // added, so the set is in Settings' choice before the game's first opening (the user, 2026-11-16); its page builds the same
  // set from the same files, and a set that does not build here is left to it. ByopopFlicCursor.build reads synchronously, so
  // the files it asks for are read and it is run again, a round for each level (the animations' XML, then the FLICs).
  // Never rejects.
  function flicSet(g, files) {
    var F = typeof ByopopFlicCursor !== "undefined" ? ByopopFlicCursor : null, C = typeof ByopopCursors !== "undefined" ? ByopopCursors : null,
      id = g.spec.id;
    if (!F || !C || !F.GAMES[id]) return Promise.resolve();
    var byRel = {}, got = {};
    files.forEach(function (f) { byRel[String(f.rel).toLowerCase()] = f; });
    function round(n) {
      var want = {};
      var set = F.build(id, function (rel) {
        var k = rel.toLowerCase();
        if (got.hasOwnProperty(k)) return got[k];
        if (byRel.hasOwnProperty(k)) want[k] = true;
        else got[k] = null;
        return null;
      }, C);
      var keys = Object.keys(want);
      if (set || !keys.length || n >= 4) return Promise.resolve(set);
      return Promise.all(keys.map(function (k) {
        return Promise.resolve(byRel[k].read()).then(function (d) { got[k] = d instanceof Uint8Array ? d : new Uint8Array(d); },
          function () { got[k] = null; });
      })).then(function () { return round(n + 1); });
    }
    return Promise.resolve().then(function () { return round(0); })
      .then(function (set) { if (set) C.save(id, g.title, set); }).catch(function () {});
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
        var copy = typeof ByopopCopy !== "undefined" ? ByopopCopy.open() : null;
        return writeGame(spec.persist, ordered(spec, stored(spec.id, files)), function (n, total) { if (say) say(ByopopStrings.t("import.say.progress", {percent: Math.round(n * 100 / total)})); },
          function () { try { ByopopGameFiles.replacing(spec.id); } catch (e) {} },  // the old copy is gone
          function (rel, bytes, n, total) { if (copy) { copy.file(rel, bytes); copy.progress(n, total); } })
          .then(function (v) { if (copy) copy.close(); return v; }, function (e) { if (copy) copy.close(); throw e; })
          .then(function () { return icon(spec, all, root, files); })
          .then(function (got) {
            try { ByopopGameFiles.mark(spec.id); if (got.url) ByopopGameFiles.setIcon(spec.id, got.url, got.built, got.small); } catch (e) {}
            try { if (navigator.storage && navigator.storage.persist) navigator.storage.persist().catch(function () {}); } catch (e) {}
            return flicSet(g, files).then(function () { return files.length; });
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
      }).then(function () {
        try { ByopopGameFiles.forget(spec.id); } catch (e) {}
        try { ByopopSaver.wipe(spec.persist); } catch (e) {}  // the screen saver's profile of the game goes with its files
      });
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

  return { stored: stored, exeUnreadable: exeUnreadable, probeHolds: probeHolds, markerPattern: markerPattern, findRoot: findRoot, withPaks: withPaks, pick: pick, hasMarkers: hasMarkers, exeIcon: exeIcon,
    games: games, add: add, addSoon: addSoon, sort: sort, sortSteps: sortSteps, otherGame: otherGame, otherNote: otherNote, nameScore: nameScore, fromDrop: fromDrop, watch: watch, readingText: readingText, installerIn: installerIn, installerMark: installerMark, installerText: installerText, usualFolder: usualFolder, sortDrop: sortDrop, MANY_REJECTED: MANY_REJECTED, hasFiles: hasFiles, writeGame: writeGame, addGame: addGame,
    forgetGame: forgetGame, ordered: ordered, missingIn: missingIn, missingMarkers: missingMarkers, markerList: markerList };
})();
