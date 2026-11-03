// Adding games' files from the listing page (docs/WEB_PLAYER.md, "Adding games from the listing page"; the user, 2026-11-03): a game
// folder (or a .zip of one) dropped on a game's card adds that game's files; folders dropped on the logo (the bottle opener) or
// chosen with "Add games", one game's folder or a folder of many (the "PopCap Games" folder), are sorted into the games they are
// (web/gameimport.js), and the panel under the toolbar says what each one is, asks where that is unclear and names the folders
// that hold no game this site plays. The games are added one at a time, each while its Web Lock is held, so a game whose page is
// open is refused with a note. Nothing leaves the browser.
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
  var queue = [], running = false;
  function enqueue(job) { job.say("Waiting…"); queue.push(job); if (!running) next(); }
  function next() {
    var job = queue.shift();
    if (!job) { running = false; return; }
    running = true;
    var card = cardOf(job.game), own = card && card.querySelector(".own");
    function say(text) { job.say(text); if (own) own.textContent = text; }
    say("Reading…");
    ByopopImport.addGame(job.game, job.entries, job.root, W().pak, W().fs.safeRel, say).then(function (n) {
      job.done(true, "Added (" + n + " files)");
    }, function (e) {
      job.done(false, (e && e.message) || "The files could not be added.");
    }).then(function () {
      try { if (window.ByopopTiles) ByopopTiles.check(); } catch (x) {}
      next();
    });
  }

  // ---------- the panel ----------
  var panel, list, rejectedEl, closeBtn;
  function openPanel() {
    panel.hidden = false;
  }
  function clearPanel() {
    list.textContent = "";
    rejectedEl.textContent = "";
    rejectedEl.hidden = true;
  }
  // A row for one folder: its name, the game, and its state (a live line); unclear: a choice of the games it could be
  function row(found) {
    var li = make("li", "ag-row");
    li.appendChild(make("span", "ag-folder", found.folder));
    var state = make("span", "ag-state");
    state.setAttribute("role", "status");
    function job(game) {
      return {
        game: game, entries: found.entries, root: found.root,
        say: function (t) { state.textContent = t; },
        done: function (ok, t) { state.textContent = t; state.className = "ag-state " + (ok ? "ag-ok" : "ag-error"); }
      };
    }
    if (found.game) {
      li.appendChild(make("span", "ag-game", "→ " + found.game.title));
      li.appendChild(state);
      enqueue(job(found.game));
    } else {
      var id = "ag-pick-" + Math.random().toString(36).slice(2);
      var label = make("label", "ag-ask", "Which game?");
      label.htmlFor = id;
      var select = make("select");
      select.id = id;
      found.choices.forEach(function (g) { var o = make("option", null, g.title); o.value = g.target; select.appendChild(o); });
      var add = make("button", null, "Add");
      add.type = "button";
      add.addEventListener("click", function () {
        var g = byTarget[select.value];
        if (!g) return;
        select.disabled = true; add.remove(); label.remove();
        select.replaceWith(make("span", "ag-game", "→ " + g.title));
        enqueue(job(g));
      });
      li.appendChild(label); li.appendChild(select); li.appendChild(add); li.appendChild(state);
    }
    list.appendChild(li);
  }
  function showSorted(sorted) {
    openPanel();
    sorted.found.forEach(row);
    if (sorted.rejected.length) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = "Not added (no game this site plays): " + sorted.rejected.join(", ");
    }
    if (!sorted.found.length && !sorted.rejected.length) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = "Nothing was dropped that could be read. Drop the game's folder, or a .zip of it.";
    }
  }
  function fromFiles(promise) {
    clearPanel();
    openPanel();
    var reading = make("li", "ag-row", "Reading…");
    list.appendChild(reading);
    promise.then(function (entries) {
      reading.remove();
      showSorted(ByopopImport.sort(entries));
    }, function (e) {
      reading.remove();
      rejectedEl.hidden = false;
      rejectedEl.textContent = (e && e.message) || "The files could not be read.";
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
        rejectedEl.textContent = "That folder doesn't hold " + game.title + "'s " + game.spec.markerText + ".";
        return;
      }
      var top = (entries[0] && entries[0].path.split("/")[0]) || game.title;
      row({ folder: top, entries: entries, root: root, game: game, choices: [game] });
    }, function (e) {
      rejectedEl.hidden = false;
      rejectedEl.textContent = (e && e.message) || "The files could not be read.";
    });
  }

  function init() {
    panel = $("adding");
    if (!panel || !W()) return;
    list = $("adding-list");
    rejectedEl = $("adding-rejected");
    closeBtn = $("adding-close");
    closeBtn.addEventListener("click", function () { panel.hidden = true; if (!running) clearPanel(); $("add-games").focus(); });
    $("add-games").hidden = false;
    $("add-games").addEventListener("click", function () { $("add-games-pick").click(); });
    $("add-games-pick").addEventListener("change", function (ev) {
      var files = W().zip.fromInput(ev.target.files);
      ev.target.value = "";
      if (files.length) fromFiles(Promise.resolve(files));
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
  }
  return { init: init, fromFiles: fromFiles, onCardDrop: onCardDrop };
})();
