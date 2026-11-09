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
  var panel, list, rejectedEl, othersEl, closeBtn, live;
  function announce(text) { if (live) live.textContent = text; }
  function openPanel() {
    panel.hidden = false;
  }
  function clearPanel() {
    list.textContent = "";
    rejectedEl.textContent = "";
    rejectedEl.hidden = true;
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
    othersEl.hidden = !othersEl.firstChild;
    if (sorted.rejected.length) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = ByopopStrings.t("add.rejected", {folders: sorted.rejected.join(", ")});
    }
    if (!sorted.found.length && !sorted.rejected.length && !(sorted.others || []).length) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = ByopopStrings.t("add.nothing");
    }
  }
  function fromFiles(promise) {
    clearPanel();
    openPanel();
    var reading = make("li", "ag-row", ByopopStrings.t("add.reading_files"));
    list.appendChild(reading);
    promise.then(function (entries) {
      reading.remove();
      showSorted(ByopopImport.sort(entries));
    }, function (e) {
      reading.remove();
      rejectedEl.hidden = false;
      rejectedEl.textContent = (e && e.message) || ByopopStrings.t("add.read_failed");
    });
  }

  // ---------- drop targets ----------
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
      onDrop(ByopopImport.fromDrop(ev.dataTransfer, W().zip));
    });
  }
  // A folder dropped on a game's card is that game's, as on its own page: its files are looked for there and nowhere else
  function onCardDrop(game, promise) {
    clearPanel();
    openPanel();
    promise.then(function (entries) {
      var root = ByopopImport.findRoot(game.spec, entries, false);
      if (root === null) {
        rejectedEl.hidden = false;
        rejectedEl.textContent = ByopopStrings.t("add.wrong_folder", {title: game.title, markers: ByopopImport.missingMarkers(game.spec, entries)});
        var other = ByopopImport.otherGame(entries);
        if (other) { rejectedEl.appendChild(document.createTextNode(" ")); rejectedEl.appendChild(ByopopImport.otherNote(document, other)); }
        return;
      }
      var top = (entries[0] && entries[0].path.split("/")[0]) || game.title;
      row({ folder: top, entries: entries, root: root, game: game, choices: [game] });
    }, function (e) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = (e && e.message) || ByopopStrings.t("add.read_failed_game");
    });
  }

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
    closeBtn.addEventListener("click", function () { panel.hidden = true; if (!running) clearPanel(); $("add-games").focus(); });
    $("add-games").hidden = false;
    $("add-games").addEventListener("click", function () { $("add-games-pick").click(); });
    // Where a folder cannot be picked (phones: a touch screen and no mouse), "Add games" picks files, .zip files of game folders
    var pick = $("add-games-pick");
    try { if (window.matchMedia("(pointer: coarse) and (not (any-pointer: fine))").matches) { pick.removeAttribute("webkitdirectory"); pick.accept = ".zip,application/zip"; } }
    catch (e) {}
    pick.addEventListener("change", function (ev) {
      var chosen = Array.prototype.slice.call(ev.target.files || []);
      var zips = chosen.filter(function (f) { return /\.zip$/i.test(f.name) && !f.webkitRelativePath; });
      var files = W().zip.fromInput(chosen.filter(function (f) { return zips.indexOf(f) < 0; }));
      ev.target.value = "";
      if (!files.length && !zips.length) return;
      // each .zip as a folder named after it, as when it is dropped
      fromFiles(zips.reduce(function (p, z) {
        return p.then(function (out) {
          return W().zip.fromFile(z).then(function (inner) {
            var base = z.name.replace(/\.zip$/i, "");
            return out.concat(inner.map(function (e) { return { path: base + "/" + e.path, read: e.read }; }));
          });
        });
      }, Promise.resolve(files)));
    });
    Array.prototype.forEach.call(document.querySelectorAll("a.tile[data-game]"), function (a) {
      var game = byId[a.getAttribute("data-game")];
      if (game) dropTarget(a.parentNode, function (p) { onCardDrop(game, p); });
    });
    var logo = $("logo-drop");
    if (logo) dropTarget(logo, fromFiles);
    // a drop anywhere else does nothing, rather than the browser opening the file in place of the page
    document.addEventListener("dragover", function (ev) { if (ByopopImport.hasFiles(ev.dataTransfer)) ev.preventDefault(); });
    document.addEventListener("drop", function (ev) { if (ByopopImport.hasFiles(ev.dataTransfer)) ev.preventDefault(); });
    initBin();
  }

  // ---------- the recycle bin ----------
  // A game's cap (its badge, draggable while its files are here) dragged into the bin forgets the game's files, as "Forget game
  // files" on its page does: saves and mods stay. It deletes what the player loaded, so nothing goes without a yes in the bin's own
  // panel (a confirm() dialog is not shown in some places the site runs). The bin's button opens the same panel with a list of the
  // games whose files are here, for the keyboard and for browsers without drag and drop.
  var CAP_TYPE = "application/x-byopop-game";
  var asking = null;  // the game whose files the panel asks about
  function initBin() {
    var bin = $("bin"), binPanel = $("bin-panel");
    if (!bin || !binPanel) return;
    bin.hidden = false;
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
      bin.setAttribute("aria-expanded", "true");
      status("");
      var select = $("bin-game"), list = gamesWithFiles();
      select.textContent = "";
      list.forEach(function (g) { var o = make("option", null, g.title); o.value = g.spec.id; select.appendChild(o); });
      $("bin-forget").disabled = list.length === 0;
      if (!list.length) status(ByopopStrings.t("add.bin.none"));
      if (game) ask(game);
      else { $("bin-confirm").hidden = true; $("bin-pick").hidden = false; asking = null; (list.length ? select : $("bin-close")).focus(); }
    }
    function hide() {
      binPanel.hidden = true;
      bin.setAttribute("aria-expanded", "false");
      asking = null;
      bin.focus();
    }
    bin.addEventListener("click", function () { if (binPanel.hidden) show(null); else hide(); });
    $("bin-close").addEventListener("click", hide);
    binPanel.addEventListener("keydown", function (ev) { if (ev.key === "Escape" && !$("bin-yes").disabled) { ev.preventDefault(); hide(); } });
    $("bin-forget").addEventListener("click", function () { var g = byId[$("bin-game").value]; if (g) ask(g); });
    $("bin-no").addEventListener("click", function () { show(null); });
    $("bin-yes").addEventListener("click", function () {
      var g = asking;
      if (!g) return;
      $("bin-yes").disabled = $("bin-no").disabled = true;
      status(ByopopStrings.t("add.bin.forgetting"));
      ByopopImport.forgetGame(g).then(function () {
        return ByopopStrings.t("add.bin.forgotten", {title: g.title});
      }, function (e) { return (e && e.message) || ByopopStrings.t("add.bin.failed"); }).then(function (text) {
        // the list again, once the cards have been checked (their data-files)
        var checked;
        try { checked = window.ByopopTiles ? ByopopTiles.check() : null; } catch (x) { checked = null; }
        return Promise.resolve(checked).catch(function () {}).then(function () {
          $("bin-yes").disabled = $("bin-no").disabled = false;
          $("bin-confirm").hidden = true;
          asking = null;
          show(null);
          status(text);
        });
      });
    });
    // the caps: a drag carries the game's id
    document.addEventListener("dragstart", function (ev) {
      var badge = ev.target && ev.target.closest ? ev.target.closest(".badge[draggable=true]") : null;
      var a = badge && badge.closest("a.tile[data-game]");
      if (!a) return;
      ev.stopPropagation();
      ev.dataTransfer.setData(CAP_TYPE, a.getAttribute("data-game"));
      ev.dataTransfer.effectAllowed = "move";
    }, true);
    function isCap(ev) { return !!ev.dataTransfer && Array.prototype.indexOf.call(ev.dataTransfer.types || [], CAP_TYPE) >= 0; }
    bin.addEventListener("dragenter", function (ev) { if (isCap(ev)) { ev.preventDefault(); bin.classList.add("drop-on"); } });
    bin.addEventListener("dragover", function (ev) { if (isCap(ev)) { ev.preventDefault(); ev.dataTransfer.dropEffect = "move"; } });
    bin.addEventListener("dragleave", function () { bin.classList.remove("drop-on"); });
    bin.addEventListener("drop", function (ev) {
      bin.classList.remove("drop-on");
      if (!isCap(ev)) return;
      ev.preventDefault();
      var g = byId[ev.dataTransfer.getData(CAP_TYPE)];
      if (g) show(g);
    });
  }
  return { init: init, fromFiles: fromFiles, onCardDrop: onCardDrop };
})();
