// Adding games' files from the listing page (docs/WEB_PLAYER.md, "Adding games from the listing page"; the user, 2026-11-03): a game
// folder (or a .zip of one) dropped on a game's card adds that game's files; folders dropped on the logo (the bottle opener) or
// chosen with "Add games", one game's folder or a folder of many (the "PopCap Games" folder), are sorted into the games they are
// (web/gameimport.js), and the panel under the toolbar says what each one is, asks where that is unclear and names the folders
// that hold no game this site plays. The games are added one at a time, each while its Web Lock is held, so a game whose page is
// open is refused with a note. Each game's terms come first, one game at a time (below). Nothing leaves the browser. The recycle
// bin beside "Add games" forgets a game's files (below).
var ByopopAddGames = (function () {
  "use strict";
  function $(id) { return document.getElementById(id); }
  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  function W() { return (window.Module && Module.byopop) || null; }
  // the games by their build target and by their page's id, which a card's data-game holds
  var byTarget = {}, byId = {};
  ByopopImport.games.forEach(function (g) { byTarget[g.target] = g; byId[g.spec.id] = g; });
  function cardOf(game) {
    var a = document.querySelector('a.tile[data-game="' + game.spec.id + '"]');
    return a ? a.parentNode : null;
  }

  // ---------- the queue: one game at a time ----------
  // The games added while the queue ran are shown together once it is empty (opened): the cards are checked again (their
  // state, the icons).
  var queue = [], running = false, opened = [];
  // the site's wait cursor while a game's files are read (web/cursors.js)
  function busy(on) { try { if (window.ByopopCursors) ByopopCursors.busy("addgames", on); } catch (e) {} }
  function enqueue(job) { job.say(ByopopStrings.t("add.waiting")); queue.push(job); if (!running) next(); }
  function next() {
    var job = queue.shift();
    if (!job) { running = false; busy(false); openCaps(); return; }
    running = true;
    busy(false);  // (not while the terms wait for the player)
    var card = cardOf(job.game), own = card && card.querySelector(".own");
    function say(text) { job.say(text); if (own) own.textContent = text; }
    agreeTo(job.game).then(function (ok) {
      if (ok) { busy(true); add(); return; }
      job.done(false, ByopopStrings.t("add.not_agreed"));
      if (own) own.textContent = "";
      notAgreed(job.game);
      try { if (window.ByopopTiles) ByopopTiles.check(); } catch (x) {}
      next();
    });
    function add() {
      say(ByopopStrings.t("add.reading"));
      ByopopImport.addGame(job.game, job.entries, job.root, W().pak, W().fs.safeRel, say).then(function (n) {
        job.done(true, ByopopStrings.t("add.added_count", {count: n}));
        if (own) own.textContent = ByopopStrings.t("add.added");
        if (opened.indexOf(job.game) < 0) opened.push(job.game);
      }, function (e) {
        // the theme's error sound (web/themes.js)
        try { window.dispatchEvent(new CustomEvent("byopop-desk-event", { detail: { what: "error" } })); } catch (x) {}
        job.done(false, (e && e.message) || ByopopStrings.t("add.failed"));
        try { if (window.ByopopTiles) ByopopTiles.check(); } catch (x) {}
      }).then(next);
    }
  }

  // ---------- the terms ----------
  // Each game's terms before its files are added (docs/WEB_PLAYER.md, "The terms"; the user, 2026-11-03: "Make it go through an
  // agreement for each game uploaded but allow it to be done on the front page"): the dialog shows the same text, checkbox and
  // Continue as the game's own terms card (terms-games.js, written by the build from the same filled-in terms) and keeps the same
  // agreement, byopop-terms-<id> at the terms' version (web/terms.js), so the game's page does not ask again. Each game is asked
  // on its own, one after another as the queue reaches it; a game already agreed to at its current version is not asked, and
  // neither is any game on a page that skips the card (a development host, the site's test key). "Don't add this game" or Escape
  // leaves the game out, and the panel names the games left out. Where localStorage fails, an agreement lasts until the page closes.
  var agreedHere = {}, leftOut = [], leftOutEl = null;
  function agreeTo(game) {
    var id = game.spec.id, terms = (window.ByopopTermsTexts || {})[id];
    if (!terms) return Promise.resolve(false);  // a game the build gave no terms: never added without them
    if (agreedHere[id] === terms.version || ByopopTerms.agreed(id, terms.version)) return Promise.resolve(true);
    var dialog = $("terms-dialog");
    if (!dialog || typeof dialog.showModal !== "function") return Promise.resolve(false);
    return new Promise(function (resolve) {
      var box = $("terms-agree"), go = $("btn-terms"), skip = $("btn-terms-skip"), had = document.activeElement;
      $("terms-title").textContent = ByopopStrings.t("game.terms.title", {title: game.title});
      $("terms-text").innerHTML = terms.html;  // the build's own text (web/terms.html, filled in), never anything the player gave
      $("terms-agree-text").textContent = ByopopStrings.t("game.terms.agree", {title: game.title});
      box.checked = false;
      go.disabled = true;
      function onBox() { go.disabled = !box.checked; }
      function onGo() {
        if (!box.checked) return;
        agreedHere[id] = terms.version;
        ByopopTerms.agree(id, terms.version);
        done(true);
      }
      function onSkip() { done(false); }
      function onCancel(ev) { ev.preventDefault(); done(false); }  // Escape
      function done(ok) {
        box.removeEventListener("change", onBox);
        go.removeEventListener("click", onGo);
        skip.removeEventListener("click", onSkip);
        dialog.removeEventListener("cancel", onCancel);
        dialog.close();
        if (had && document.body.contains(had)) { try { had.focus(); } catch (e) {} }
        resolve(ok);
      }
      box.addEventListener("change", onBox);
      go.addEventListener("click", onGo);
      skip.addEventListener("click", onSkip);
      dialog.addEventListener("cancel", onCancel);
      dialog.showModal();
      $("terms-text").scrollTop = 0;
    });
  }
  // the games left out for their terms, named under the panel's list, as the folders with no game are
  function notAgreed(game) {
    if (leftOut.indexOf(game.title) < 0) leftOut.push(game.title);
    if (!leftOutEl) {
      leftOutEl = make("p", "adding-rejected");
      leftOutEl.setAttribute("aria-live", "polite");
      rejectedEl.parentNode.insertBefore(leftOutEl, rejectedEl.nextSibling);
    }
    leftOutEl.hidden = false;
    leftOutEl.textContent = ByopopStrings.t("add.not_agreed_list", {titles: leftOut.join(", ")});
  }
  function openCaps() {
    var games = opened;
    opened = [];
    if (!games.length) return;
    try { if (window.ByopopTiles) ByopopTiles.check(); } catch (x) {}
  }

  // ---------- the panel ----------
  // live: the panel's one live region, which reads out how each folder ended (the rows' own state lines are not live, so a big
  // folder does not read "Waiting…" for every game)
  var panel, list, rejectedEl, othersEl, closeBtn, live, rejectedNames, panelWalk = null;
  function announce(text) { if (live) live.textContent = text; }
  function openPanel() {
    panel.hidden = false;
  }
  function clearPanel() {
    // a reading under way for the panel stops: what it read would go into a cleared panel
    if (panelWalk) { panelWalk.cancel(); panelWalk = null; }
    list.textContent = "";
    rejectedEl.textContent = "";
    rejectedEl.hidden = true;
    if (rejectedNames) { rejectedNames.remove(); rejectedNames = null; }
    othersEl.textContent = "";
    othersEl.hidden = true;
    leftOut = [];
    if (leftOutEl) { leftOutEl.textContent = ""; leftOutEl.hidden = true; }
  }
  // A row for one folder: its name, the game, and its state (a live line); unclear: a choice of the games it could be
  function row(found) {
    var li = make("li", "ag-row"), uid = Math.random().toString(36).slice(2);
    var folder = make("span", "ag-folder", found.folder);
    folder.id = "ag-folder-" + uid;
    li.appendChild(folder);
    var state = make("span", "ag-state");
    state.tabIndex = -1;  // the focus goes here once the game is chosen
    function job(game) {
      return {
        game: game, entries: found.entries, root: found.root,
        say: function (t) { state.textContent = t; },
        done: function (ok, t) {
          state.textContent = t;
          state.className = "ag-state " + (ok ? "ag-ok" : "ag-error");
          announce(ByopopStrings.t("add.announce", {folder: found.folder, title: game.title, text: t}));
        }
      };
    }
    if (found.game) {
      li.appendChild(make("span", "ag-game", ByopopStrings.t("add.game", {title: found.game.title})));
      li.appendChild(state);
      enqueue(job(found.game));
    } else {
      var id = "ag-pick-" + uid;
      var label = make("label", "ag-ask", ByopopStrings.t("add.which"));
      label.htmlFor = id;
      var select = make("select");
      select.id = id;
      // each row's "Which game?" and "Add" are told apart by the folder they are for
      select.setAttribute("aria-describedby", folder.id);
      found.choices.forEach(function (g) { var o = make("option", null, g.title); o.value = g.target; select.appendChild(o); });
      var add = make("button", null, ByopopStrings.t("add.add"));
      add.type = "button";
      add.setAttribute("aria-describedby", folder.id);
      add.addEventListener("click", function () {
        var g = byTarget[select.value];
        if (!g) return;
        select.disabled = true; add.remove(); label.remove();
        select.replaceWith(make("span", "ag-game", ByopopStrings.t("add.game", {title: g.title})));
        enqueue(job(g));
        // the focus to the next folder still to choose, else to this row's state
        var nextPick = list.querySelector("select:not([disabled])");
        (nextPick || state).focus();
      });
      li.appendChild(label); li.appendChild(select); li.appendChild(add); li.appendChild(state);
    }
    list.appendChild(li);
  }
  function showSorted(sorted) {
    openPanel();
    sorted.found.forEach(row);
    // a game the site does not play, by name: another project's link or "coming soon" (cmake/ByopopOthers.cmake)
    (sorted.others || []).forEach(function (o) {
      var p = make("p", "adding-other");
      p.appendChild(document.createTextNode(ByopopStrings.t("add.other", {folder: o.folder}) + " "));
      p.appendChild(ByopopImport.otherNote(document, o.game));
      othersEl.appendChild(p);
    });
    // a folder named as one of the site's games, without its files: the game and what is missing (web/gameimport.js, lacking)
    (sorted.lacking || []).forEach(function (l) {
      othersEl.appendChild(make("p", "adding-other", ByopopStrings.t("add.other", {folder: l.folder}) + " " + l.game.title + ". " +
        ByopopStrings.t("setup.result.missing", {files: l.missing})));
    });
    othersEl.hidden = !othersEl.firstChild;
    // a game's installer dropped in place of its folder: what to do instead, before the folders with no game
    var lead = sorted.installer ? sorted.installer.text + " " : "";
    if (sorted.rejected.length >= ByopopImport.MANY_REJECTED) {
      // many: one line, their names behind Details
      rejectedEl.hidden = false;
      rejectedEl.textContent = lead + ByopopStrings.t("import.rejected_many", {n: sorted.rejected.length});
      rejectedNames = make("details", "adding-rejected-names");
      rejectedNames.appendChild(make("summary", null, ByopopStrings.t("import.rejected_details")));
      rejectedNames.appendChild(make("p", null, sorted.rejected.join(", ")));
      rejectedEl.parentNode.insertBefore(rejectedNames, rejectedEl.nextSibling);
    } else if (sorted.rejected.length || lead) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = lead + (sorted.rejected.length ? ByopopStrings.t("add.rejected", {folders: sorted.rejected.join(", ")}) : "");
    }
    if (!sorted.found.length && !sorted.rejected.length && !(sorted.others || []).length && !(sorted.lacking || []).length && !lead) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = ByopopStrings.t("add.nothing");
    }
  }
  // watch: a drop's (ByopopImport.watch): the reading row counts the folders and files found so far, and its Cancel stops it
  function fromFiles(promise, watch) {
    clearPanel();
    openPanel();
    var reading = make("li", "ag-row"), text = make("span", null, ByopopStrings.t("add.reading_files"));
    reading.appendChild(text);
    if (watch) {
      panelWalk = watch;
      var stop = make("button", null, ByopopStrings.t("setup.cancel"));
      stop.type = "button";
      text.id = "ag-reading-" + Math.random().toString(36).slice(2);
      stop.setAttribute("aria-describedby", text.id);
      stop.addEventListener("click", function () { watch.cancel(); stop.disabled = true; });
      reading.appendChild(stop);
      watch.listen(function (folders, files) { text.textContent = ByopopImport.readingText(folders, files); });
    }
    list.appendChild(reading);
    function over() { if (watch) watch.listen(null); if (panelWalk === watch) panelWalk = null; }
    var mine = watch;
    promise.then(function (entries) { return ByopopImport.sortDrop(entries); }).then(function (sorted) {
      over();
      reading.remove();
      if (mine && mine.stopped()) return;  // (stopped, or the panel cleared, while the files were sorted)
      showSorted(sorted);
    }, function (e) {
      over();
      reading.remove();
      if (e && e.cancelled) return;
      rejectedEl.hidden = false;
      rejectedEl.textContent = (e && e.message) || ByopopStrings.t("add.read_failed");
    });
  }

  // ---------- drop targets ----------
  // the drop target over the desktop while files are dragged onto it (init): the desktop dimmed, and a box under the pointer with
  // an installer's floppy disk, our own drawing in 32 x 32 in the 1995 palette, an arrow going into it, and what a drop does; the
  // pointer goes through it to what is under it
  function dropZone() {
    var el = make("div", "d95-dropzone"), box = make("div", "d95-dropzone-box");
    el.setAttribute("aria-hidden", "true");
    el.hidden = true;
    var pic = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    pic.setAttribute("viewBox", "0 0 32 32"); pic.setAttribute("shape-rendering", "crispEdges");
    pic.innerHTML = '<path d="M6 13h18l2 2v16H6z" fill="#000080" stroke="#000"/><path d="M11 13h10v6H11z" fill="#c0c0c0" stroke="#000"/>' +
      '<path d="M17 14h2v4h-2z" fill="#000080"/><path d="M9 22h14v8H9z" fill="#fff" stroke="#000"/><path d="M11 24h10v1H11zM11 27h10v1H11z" fill="#808080"/>' +
      '<path d="M14 0h4v6h4l-6 6-6-6h4z" fill="#00ff00" stroke="#000"/>';
    box.appendChild(pic);
    box.appendChild(make("span", "", ByopopStrings.t("skin.drop.install")));
    el.appendChild(box);
    document.body.appendChild(el);
    return el;
  }
  function dropTarget(el, onDrop) {
    var depth = 0;
    el.addEventListener("dragenter", function (ev) {
      if (!ByopopImport.hasFiles(ev.dataTransfer)) return;
      ev.preventDefault();
      depth++;
      el.classList.add("drop-on");
    });
    el.addEventListener("dragover", function (ev) {
      if (!ByopopImport.hasFiles(ev.dataTransfer)) return;
      ev.preventDefault();
      ev.dataTransfer.dropEffect = "copy";
    });
    el.addEventListener("dragleave", function () { if (--depth <= 0) { depth = 0; el.classList.remove("drop-on"); } });
    el.addEventListener("drop", function (ev) {
      depth = 0;
      el.classList.remove("drop-on");
      if (!ByopopImport.hasFiles(ev.dataTransfer)) return;
      ev.preventDefault();
      ev.stopPropagation();
      var watch = ByopopImport.watch();
      onDrop(ByopopImport.fromDrop(ev.dataTransfer, W().zip, watch), watch);
    });
  }
  // A folder dropped on a game's card is that game's, as on its own page: its files are looked for there and nowhere else
  function onCardDrop(game, promise) {
    clearPanel();
    openPanel();
    promise.then(function (entries) {
      var root = ByopopImport.findRoot(game.spec, entries, false);
      if (root === null) {
        // the game's installer, dropped on its card: what to do instead (the card's game, where its name names none)
        return ByopopImport.installerIn(entries, { found: [], others: [], rejected: [] }).then(null, function () { return null; }).then(function (inst) {
          rejectedEl.hidden = false;
          if (inst && !inst.other) {
            if (!inst.game) { inst.game = game; inst.folder = ByopopImport.usualFolder(game.spec); }
            rejectedEl.textContent = ByopopImport.installerText(inst);
            return;
          }
          rejectedEl.textContent = ByopopStrings.t("add.wrong_folder", {title: game.title, markers: ByopopImport.missingMarkers(game.spec, entries)});
          var other = ByopopImport.otherGame(entries);
          if (other) { rejectedEl.appendChild(document.createTextNode(" ")); rejectedEl.appendChild(ByopopImport.otherNote(document, other)); }
        });
      }
      var top = (entries[0] && entries[0].path.split("/")[0]) || game.title;
      row({ folder: top, entries: entries, root: root, game: game, choices: [game] });
    }, function (e) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = (e && e.message) || ByopopStrings.t("add.read_failed_game");
    });
  }

  // The files chosen in a file input (folders, or .zip files where a folder cannot be picked) as the entries a drop gives: each .zip
  // as a folder named after it, as when it is dropped. Null when nothing was chosen.
  function fromInput(fileList) {
    var chosen = Array.prototype.slice.call(fileList || []);
    var zips = chosen.filter(function (f) { return /\.zip$/i.test(f.name) && !f.webkitRelativePath; });
    var files = W().zip.fromInput(chosen.filter(function (f) { return zips.indexOf(f) < 0; }));
    if (!files.length && !zips.length) return null;
    return zips.reduce(function (p, z) {
      return p.then(function (out) {
        return W().zip.fromFile(z).then(function (inner) {
          var base = z.name.replace(/\.zip$/i, "");
          return out.concat(inner.map(function (e) { return { path: base + "/" + e.path, read: e.read }; }));
        });
      });
    }, Promise.resolve(files));
  }
  // The Setup window (web/setup.js) takes the folders added from the button, the logo and the desktop while the desktop is on;
  // a drop on a game's card stays that game's (onCardDrop), and on a screen with no desktop the panel above does all of it
  function setup() { try { return window.ByopopSetup && ByopopSetup.ready() ? ByopopSetup : null; } catch (e) { return null; } }
  function fromDropOrSetup(promise, watch) { var s = setup(); if (s) s.openWith(promise, null, watch); else fromFiles(promise, watch); }

  function init() {
    panel = $("adding");
    if (!panel || !W()) return;
    list = $("adding-list");
    rejectedEl = $("adding-rejected");
    // the games the site does not play, each in a line under the folders with no game
    othersEl = make("div", "adding-others");
    othersEl.hidden = true;
    rejectedEl.parentNode.insertBefore(othersEl, rejectedEl.nextSibling);
    live = make("p", "pl-sr");
    live.setAttribute("role", "status");
    panel.appendChild(live);
    closeBtn = $("adding-close");
    // (the Library's Add games button, web/library.js, has the id add-games)
    closeBtn.addEventListener("click", function () { panel.hidden = true; if (!running) clearPanel(); var b = $("add-games"); if (b) b.focus(); });
    // Where a folder cannot be picked (phones: a touch screen and no mouse), "Add games" picks files, .zip files of game folders
    var pick = $("add-games-pick");
    try { if (window.matchMedia("(pointer: coarse) and (not (any-pointer: fine))").matches) { pick.removeAttribute("webkitdirectory"); pick.accept = ".zip,application/zip"; } }
    catch (e) {}
    pick.addEventListener("change", function (ev) {
      var read = fromInput(ev.target.files);
      ev.target.value = "";
      if (read) fromFiles(read);
    });
    Array.prototype.forEach.call(document.querySelectorAll("a.tile[data-game]"), function (a) {
      var game = byId[a.getAttribute("data-game")];
      if (game) dropTarget(a.parentNode, function (p) { onCardDrop(game, p); });
    });
    var logo = $("logo-drop");
    if (logo) dropTarget(logo, fromDropOrSetup);
    // a drop anywhere else on the desktop opens the Setup window with it (a game's window is a page of its own, which takes its
    // own drops); with no desktop it does nothing, rather than the browser opening the file in place of the page.
    // While files are dragged over the desktop's own area (no window, the task bar or the logo), it dims behind a drop target
    // that says they will be installed, and the games a drop there installs get their shortcuts where it fell (web/setup.js;
    // the user, 2026-11-16: "Can the site detect when a folder is being dragged into the desktop and show an icon indicating
    // it will install it? Design I have in mind is like reverse image search on Google"). A drag tells only that files are
    // coming, not whether they are a folder or a .zip, so its words fit any, and the drop sorts them as before.
    var zone = null, zoneTimer = 0, zoneDepth = 0;
    function onDesk(t) {
      return !!setup() && !!t && !!t.closest && !t.closest(".d95-win, .d95-taskbar, .d95-menu, .d95-ctx, #logo-drop, dialog, [role=dialog]");
    }
    function showZone(on, ev) {
      if (on && !zone) zone = dropZone();
      if (zone) zone.hidden = !on;
      // its box under the pointer, inside the window
      if (on && ev) {
        var box = zone.firstChild, S = window.ByopopSkin, x = S ? S.unzoom(ev.clientX) : ev.clientX, y = S ? S.unzoom(ev.clientY) : ev.clientY;
        box.style.left = Math.max(8, Math.min(x - box.offsetWidth / 2, zone.clientWidth - box.offsetWidth - 8)) + "px";
        box.style.top = Math.max(8, Math.min(y + 18, zone.clientHeight - box.offsetHeight - 8)) + "px";
      }
      clearTimeout(zoneTimer);
      // a drag the browser ends without a word (Escape, or the pointer gone out of the window): dragover comes every 50 ms or so
      // while it lasts
      if (on) zoneTimer = setTimeout(function () { zoneDepth = 0; showZone(false); }, 350);
    }
    document.addEventListener("dragenter", function (ev) { if (ByopopImport.hasFiles(ev.dataTransfer)) zoneDepth++; });
    document.addEventListener("dragleave", function (ev) {
      if (ByopopImport.hasFiles(ev.dataTransfer) && --zoneDepth <= 0) { zoneDepth = 0; showZone(false); }
    });
    document.addEventListener("keydown", function (ev) { if (ev.key === "Escape" && zone && !zone.hidden) showZone(false); });
    document.addEventListener("dragover", function (ev) {
      if (!ByopopImport.hasFiles(ev.dataTransfer)) return;
      ev.preventDefault();
      if (setup()) ev.dataTransfer.dropEffect = "copy";
      showZone(onDesk(ev.target), ev);
    });
    document.addEventListener("drop", function (ev) {
      zoneDepth = 0;
      showZone(false);
      if (!ByopopImport.hasFiles(ev.dataTransfer)) return;
      ev.preventDefault();
      var s = setup(), place = null;
      if (s && onDesk(ev.target)) { try { place = ByopopSkin.desk.iconPlaceAt(ev.clientX, ev.clientY); } catch (e) {} }
      if (!s) return;
      var watch = ByopopImport.watch();
      s.openWith(ByopopImport.fromDrop(ev.dataTransfer, W().zip, watch), place, watch);
    });
    initBin();
  }

  // ---------- the recycle bin and Uninstall ----------
  // A game dragged into the bin (a Library row of a game whose files are here, onto the desktop's Recycle Bin icon, web/skins.js)
  // forgets the game's files, as "Forget game files" on its page does: saves and mods stay. It deletes what the player loaded, so
  // nothing goes without a yes in the bin's own panel, in the Library (a confirm() dialog is not shown in some places the site
  // runs). The bin's icon opens the same panel with a list of the games whose files are here, for the keyboard and for browsers
  // without drag and drop. Uninstall, in the Library's and the shortcuts' menus (web/skins.js, uninstall), asks its own question
  // and then does the same as the bin's Yes, through uninstall below.
  var CAP_TYPE = "application/x-byopop-game";
  var asking = null;  // the game whose files the panel asks about
  var binOpen = null;
  // The game's files forgotten (ByopopImport.forgetGame: the game folder's records go, its saves, mods and replays stay, the
  // record of its files says when), its name put in the bin (byopop-bin, which fills the desktop's Recycle Bin icon until it is
  // emptied, web/skins.js), and the cards checked again (their data-files, which the Library and the shortcuts read). Resolves
  // once the cards are checked; rejects with the reason the files could not be forgotten (the game open somewhere, say).
  function uninstall(id) {
    var g = byId[id];
    function checked() {
      var c;
      try { c = window.ByopopTiles ? ByopopTiles.check() : null; } catch (x) { c = null; }
      return Promise.resolve(c).catch(function () {});
    }
    if (!g) return Promise.reject(new Error(ByopopStrings.t("add.bin.failed")));
    return ByopopImport.forgetGame(g).then(function () {
      try {
        var held = JSON.parse(localStorage.getItem("byopop-bin") || "[]");
        if (!Array.isArray(held)) held = [];
        held.push({ id: g.spec.id, title: g.title, at: Date.now() });
        localStorage.setItem("byopop-bin", JSON.stringify(held.slice(-50)));
      } catch (x) {}
      try { window.dispatchEvent(new CustomEvent("byopop-bin")); } catch (x) {}
      return checked();
    }, function (e) { return checked().then(function () { throw e; }); });
  }
  function initBin() {
    var binPanel = $("bin-panel"), opener = null;
    if (!binPanel) return;
    function gamesWithFiles() {
      return Array.prototype.filter.call(document.querySelectorAll("a.tile[data-game]"), function (a) {
        return a.parentNode.getAttribute("data-files") === "yes" && byId[a.getAttribute("data-game")];
      }).map(function (a) { return byId[a.getAttribute("data-game")]; });
    }
    function status(text) { $("bin-status").textContent = text; }
    function ask(game) {
      asking = game;
      $("bin-question").textContent = ByopopStrings.t("add.bin.question", {title: game.title});
      $("bin-confirm").hidden = false;
      $("bin-pick").hidden = true;
      $("bin-no").focus();
    }
    function show(game) {
      binPanel.hidden = false;
      status("");
      var select = $("bin-game"), list = gamesWithFiles();
      select.textContent = "";
      list.forEach(function (g) { var o = make("option", null, g.title); o.value = g.spec.id; select.appendChild(o); });
      $("bin-forget").disabled = list.length === 0;
      if (!list.length) status(ByopopStrings.t("add.bin.none"));
      if (game) ask(game);
      else { $("bin-confirm").hidden = true; $("bin-pick").hidden = false; asking = null; (list.length ? select : $("bin-close")).focus(); }
    }
    // the focus goes back where the panel was opened from (the desktop's icon, a Library row), when it is still there
    function hide() {
      binPanel.hidden = true;
      asking = null;
      var to = opener;
      opener = null;
      if (to && document.body.contains(to)) { try { to.focus(); } catch (e) {} }
    }
    // the desktop's Recycle Bin icon (web/skins.js): the panel, asking about a game dropped on it
    binOpen = function (id) {
      if (binPanel.hidden) opener = document.activeElement && document.activeElement !== document.body ? document.activeElement : null;
      show(id && byId[id] ? byId[id] : null);
    };
    $("bin-close").addEventListener("click", hide);
    binPanel.addEventListener("keydown", function (ev) { if (ev.key === "Escape" && !$("bin-yes").disabled) { ev.preventDefault(); ev.stopPropagation(); hide(); } });
    $("bin-forget").addEventListener("click", function () { var g = byId[$("bin-game").value]; if (g) ask(g); });
    $("bin-no").addEventListener("click", function () { show(null); });
    $("bin-yes").addEventListener("click", function () {
      var g = asking;
      if (!g) return;
      $("bin-yes").disabled = $("bin-no").disabled = true;
      status(ByopopStrings.t("add.bin.forgetting"));
      uninstall(g.spec.id).then(function () { return ByopopStrings.t("add.bin.forgotten", {title: g.title}); },
        function (e) { return (e && e.message) || ByopopStrings.t("add.bin.failed"); }).then(function (text) {
        // the list again, the cards checked (their data-files)
        $("bin-yes").disabled = $("bin-no").disabled = false;
        $("bin-confirm").hidden = true;
        asking = null;
        show(null);
        status(text);
      });
    });
  }
  // A folder dropped on a game's row in the Library (web/library.js) is that game's, as on its card (onCardDrop): false for a
  // game the listing does not add
  function dropOn(id, dataTransfer) {
    var g = byId[id];
    if (!g || !W()) return false;
    var watch = ByopopImport.watch();
    onCardDrop(g, ByopopImport.fromDrop(dataTransfer, W().zip, watch), watch);
    return true;
  }
  // Add games (the Library's button): the Setup window, or where there is none, the folder picker
  function addGames() { var s = setup(); if (s) s.open(); else { var pick = $("add-games-pick"); if (pick) pick.click(); } }
  // for the Setup window (web/setup.js): the same queue (each game's terms, then its files, one game at a time), the same reading
  // of a file input's choice, and the cancelling of its jobs not yet begun
  function cancel(pred) {
    var kept = [];
    queue.forEach(function (job) { if (pred(job)) { if (job.cancelled) job.cancelled(); } else kept.push(job); });
    queue = kept;
  }
  return { init: init, fromFiles: fromFiles, onCardDrop: onCardDrop, enqueue: enqueue, fromInput: fromInput, cancel: cancel,
    bin: function (id) { if (binOpen) binOpen(id); }, uninstall: uninstall, dropOn: dropOn, add: addGames, CAP_TYPE: CAP_TYPE,
    has: function (id) { return !!byId[id]; },
    W: W, byTarget: function (t) { return byTarget[t] || null; } };
})();
