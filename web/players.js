// The Players section of the listing page (docs/WEB_PLAYER.md, "Player profiles"): the site's players, which games have each
// and under what name, the players inside the games, and renaming and deleting them across games. The data is web/profiles.js's
// (ByopopProfiles: the site's list, what each game reported of its own players, the operations waiting for a game) and the
// setting "profile" (ByopopSettings). Nothing is sent anywhere; it uses real form controls, and no alert, confirm or prompt.
//
// A game that is not running cannot be changed from here, so a rename or a delete is queued and the game applies it the
// next time it starts (with its own functions); until then the section shows it as pending, and afterwards what became of
// it. A game that has never reported its players (not started since this existed) is listed as such and cannot be targeted.
var ByopopPlayers = (function () {
  "use strict";
  var MODE_WORDS = { game: "as the game does", fill: "fills in the name", create: "uses the player" };

  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  function quote(name) { return "“" + name + "”"; }
  function when(iso) {
    try { var d = new Date(iso); return isNaN(d.getTime()) ? "" : d.toLocaleString(); } catch (e) { return ""; }
  }
  var uid = 0;
  function nextId(prefix) { return prefix + "-" + (++uid); }

  // The section's code. root: the element to build in; games: [{id, title}] in the listing's order.
  function init(root, games) {
    var title = {};
    games.forEach(function (g) { title[g.id] = g.title; });
    var ticked = {};       // "<game>\n<name>" -> true: the game players ticked for the bulk operations
    var openCards = {};    // site player id -> its games list is open
    var renaming = null;   // id of the site player being renamed
    var confirming = null; // {items: [{game, name}]}: a delete waiting for the player's yes
    var message = { text: "", error: false, undo: null };

    root.textContent = "";
    var about = make("p", "pl-about", "The players you use in the games, kept in this browser. A game uses the one in use for the " +
      "names it asks for, and can make and select the player itself; the setting “Player names” under Settings decides, " +
      "for all games or for each one. Renaming and deleting inside the games happens the next time each game starts.");
    var status = make("p", "pl-status");
    status.setAttribute("role", "status");
    status.setAttribute("aria-live", "polite");
    // an error is its own line, an alert (announced at once), and the field it is about is described by it
    var alertEl = make("p", "pl-status pl-error pl-alert");
    alertEl.id = nextId("pl-alert");
    alertEl.setAttribute("role", "alert");
    root.appendChild(about);
    root.appendChild(status);
    root.appendChild(alertEl);

    // ----- your players -----
    var h3a = make("h3", "", "Your players");
    var siteList = make("div", "pl-site");
    var addForm = make("form", "pl-form");
    var addId = nextId("pl-add");
    var addLabel = make("label", "", "New player");
    addLabel.htmlFor = addId;
    var addInput = make("input");
    addInput.type = "text"; addInput.id = addId; addInput.maxLength = ByopopProfiles.NAME_MAX; addInput.autocomplete = "off";
    addInput.setAttribute("autocapitalize", "off"); addInput.spellcheck = false;
    var addButton = make("button", "", "Add player");
    addButton.type = "submit";
    addForm.appendChild(addLabel); addForm.appendChild(addInput); addForm.appendChild(addButton);
    var modeNote = make("p", "pl-hint");
    root.appendChild(h3a); root.appendChild(siteList); root.appendChild(addForm); root.appendChild(modeNote);

    // ----- the players inside the games -----
    var h3b = make("h3", "", "Players in the games");
    var gamesAbout = make("p", "pl-about", "What each game last said of its players, at its last start and whenever its players " +
      "changed. Tick players, in any games and with any names, to rename them all to one name or to delete them. A game that keeps " +
      "no players only asks for a name at each high score and remembers the last one; that name can be renamed too.");
    var gameList = make("div", "pl-games");
    var bulk = make("form", "pl-form pl-bulk");
    var bulkId = nextId("pl-bulk");
    var bulkLabel = make("label", "", "Rename the ticked players to");
    bulkLabel.htmlFor = bulkId;
    var bulkInput = make("input");
    bulkInput.type = "text"; bulkInput.id = bulkId; bulkInput.maxLength = ByopopProfiles.NAME_MAX; bulkInput.autocomplete = "off";
    bulkInput.spellcheck = false;
    var bulkRename = make("button", "", "Rename ticked");
    bulkRename.type = "submit";
    var bulkDelete = make("button", "pl-danger", "Delete ticked…");
    bulkDelete.type = "button";
    var tickAll = make("button", "", "Tick all");
    tickAll.type = "button";
    var tickNone = make("button", "", "Tick none");
    tickNone.type = "button";
    var bulkCount = make("span", "pl-count");
    bulk.appendChild(bulkLabel); bulk.appendChild(bulkInput); bulk.appendChild(bulkRename); bulk.appendChild(bulkDelete);
    bulk.appendChild(tickAll); bulk.appendChild(tickNone); bulk.appendChild(bulkCount);
    var confirmBox = make("div", "pl-confirm");
    confirmBox.hidden = true;
    var resultsTitle = make("h3", "", "What the games did");
    var resultsList = make("ul", "pl-results");
    var resultsClear = make("button", "", "Clear this list");
    resultsClear.type = "button";
    root.appendChild(h3b); root.appendChild(gamesAbout); root.appendChild(gameList); root.appendChild(bulk);
    root.appendChild(confirmBox); root.appendChild(resultsTitle); root.appendChild(resultsList); root.appendChild(resultsClear);

    var tied = null;  // the field that the error on show is about
    function untie() {
      if (!tied) return;
      tied.removeAttribute("aria-invalid");
      tied.removeAttribute("aria-describedby");
      tied = null;
    }
    // field: the input an error is about (it is marked invalid and described by the alert until it is changed)
    function say(text, isError, undo, field) {
      untie();
      message = { text: text, error: !!isError, undo: undo || null };
      status.textContent = "";
      alertEl.textContent = "";
      if (isError) {
        alertEl.textContent = text;
        if (field) {
          tied = field;
          field.setAttribute("aria-invalid", "true");
          field.setAttribute("aria-describedby", alertEl.id);
          field.addEventListener("input", untie, { once: true });
        }
        return;
      }
      status.appendChild(document.createTextNode(text));
      if (undo) {
        status.appendChild(document.createTextNode(" "));
        var b = make("button", "pl-link", "Undo");
        b.type = "button";
        b.addEventListener("click", function () { undo(); });
        status.appendChild(b);
      }
    }
    function key(game, name) { return game + "\n" + name; }

    // ---------- your players ----------
    function modeOf(id) { try { return ByopopSettings.get("profile", id); } catch (e) { return "game"; } }

    // One line of a site player's table: how a game has it
    function gameRow(p, g, rec) {
      var tr = make("tr");
      tr.appendChild(make("th", "", g.title));
      tr.firstChild.scope = "row";
      var used = make("td"), setting = make("td", "", capital(MODE_WORDS[modeOf(g.id)] || MODE_WORDS.game));
      if (!rec) {
        used.appendChild(make("span", "pl-muted", "Not started since players were added"));
      } else {
        var m = ByopopProfiles.match(rec, p.name);
        var fit = (rec.fits && typeof rec.fits[p.name] === "string") ? rec.fits[p.name] : null;
        var flags = [];
        if (rec.kind === "scores") {
          if (fit === "") used.appendChild(make("span", "pl-muted", "No high-score name"));
          else used.appendChild(document.createTextNode("High scores start with " + quote(fit === null ? p.name : fit)));
          if (fit !== null && fit !== "" && fit !== p.name) flags.push("changed by the game’s rules");
        } else if (m) {
          used.appendChild(make("strong", "", m.name));
          if (!m.exact) flags.push(fit !== null && fit !== p.name && fit.toLowerCase() === m.name.toLowerCase()
            ? "changed by the game’s rules (you asked for " + quote(p.name) + ")" : "written " + quote(m.name) + " in this game");
          if (rec.current && rec.current.toLowerCase() === m.name.toLowerCase()) flags.push("the game’s current player");
        } else {
          used.appendChild(make("span", "pl-muted", "No player of this name"));
          if (fit === "") flags.push("the game’s rules leave nothing of this name");
          else if (fit !== null && fit !== p.name) flags.push("would be " + quote(fit) + ": the game’s rules change the name");
        }
        if (flags.length) used.appendChild(make("span", "pl-flag", " — " + flags.join("; ")));
      }
      tr.appendChild(used); tr.appendChild(setting);
      return tr;
    }
    function capital(t) { return t.charAt(0).toUpperCase() + t.slice(1); }
    // a list that is drawn again loses the focus: give it back to a player's Rename button
    function focusRename(id) {
      var b = siteList.querySelector('button[data-rename="' + id + '"]');
      if (b) b.focus();
    }

    function renderSite() {
      var all = ByopopProfiles.list(), cur = ByopopProfiles.current(), recs = ByopopProfiles.games();
      siteList.textContent = "";
      if (!all.length) {
        siteList.appendChild(make("p", "pl-empty", "No players yet. Add one below, and the games can use its name."));
      }
      var group = make("fieldset", "pl-use");
      group.appendChild(make("legend", "pl-sr", "Player in use"));
      var cards = make("ul", "pl-cards");
      group.appendChild(cards);
      all.forEach(function (p) {
        var li = make("li", "pl-card");
        var head = make("div", "pl-head");
        var rid = nextId("pl-use");
        var label = make("label", "pl-current");
        label.htmlFor = rid;
        var radio = make("input");
        radio.type = "radio"; radio.name = "pl-current"; radio.id = rid; radio.value = p.id; radio.checked = !!cur && cur.id === p.id;
        radio.addEventListener("change", function () {
          if (radio.checked && ByopopProfiles.setCurrent(p.id).ok) { say(p.name + " is the player in use."); renderAll(); }
        });
        label.appendChild(radio);
        label.appendChild(make("span", "pl-name", p.name));
        head.appendChild(label);
        if (cur && cur.id === p.id) head.appendChild(make("span", "pl-badge", "In use"));
        var actions = make("div", "pl-actions");
        var rn = make("button", "", "Rename");
        rn.type = "button";
        rn.setAttribute("aria-label", "Rename " + p.name);
        rn.setAttribute("data-rename", p.id);
        rn.addEventListener("click", function () { renaming = p.id; renderSite(); var f = siteList.querySelector(".pl-rename input"); if (f) f.focus(); });
        var inGames = make("button", "", "Delete in the games…");
        inGames.type = "button";
        inGames.setAttribute("aria-label", "Delete " + p.name + " in the games");
        inGames.addEventListener("click", function () {
          var items = [];
          games.forEach(function (g) {
            var rec = recs[g.id];
            if (!rec || rec.kind === "scores") return;
            var m = ByopopProfiles.match(rec, p.name);
            if (m) items.push({ game: g.id, name: m.name });
          });
          if (!items.length) { say(p.name + " is not a player in any game that has reported its players.", true); return; }
          askDelete(items);
        });
        var rm = make("button", "", "Remove");
        rm.type = "button";
        rm.setAttribute("aria-label", "Remove " + p.name + " from your players");
        rm.title = "Takes the name off your list. The games’ players are not touched.";
        rm.addEventListener("click", function () {
          var wasCurrent = !!cur && cur.id === p.id;
          if (!ByopopProfiles.remove(p.id).ok) { say("This browser would not keep the change.", true); return; }
          say(p.name + " was removed from your players. The games’ players are untouched.", false, function () {
            var back = ByopopProfiles.add(p.name);
            if (back.ok && wasCurrent) ByopopProfiles.setCurrent(back.profile.id);
            say(p.name + " is back.");
            renderAll();
            addInput.focus();
          });
          renderAll();
          (siteList.querySelector("input[type=radio]") || addInput).focus();
        });
        actions.appendChild(rn); actions.appendChild(inGames); actions.appendChild(rm);
        head.appendChild(actions);
        li.appendChild(head);
        if (renaming === p.id) {
          var form = make("form", "pl-form pl-rename");
          var iid = nextId("pl-rn");
          var l2 = make("label", "", "New name for " + p.name);
          l2.htmlFor = iid;
          var input = make("input");
          input.type = "text"; input.id = iid; input.value = p.name; input.maxLength = ByopopProfiles.NAME_MAX; input.autocomplete = "off";
          var save = make("button", "", "Save"); save.type = "submit";
          var cancel = make("button", "", "Cancel"); cancel.type = "button";
          cancel.addEventListener("click", function () { renaming = null; renderSite(); focusRename(p.id); });
          form.addEventListener("submit", function (ev) {
            ev.preventDefault();
            var r = ByopopProfiles.rename(p.id, input.value);
            if (!r.ok) { say(r.error, true, null, input); input.focus(); return; }
            renaming = null;
            say("Renamed to " + r.profile.name + " here. The games keep the names they have until you rename them under Players in the games.");
            renderAll();
            focusRename(p.id);
          });
          form.appendChild(l2); form.appendChild(input); form.appendChild(save); form.appendChild(cancel);
          li.appendChild(form);
        }
        // which games have this player, and under what name
        var details = make("details", "pl-details");
        var have = 0;
        games.forEach(function (g) { var rec = recs[g.id]; if (rec && rec.kind !== "scores" && ByopopProfiles.match(rec, p.name)) have++; });
        details.appendChild(make("summary", "", "In the games (" + have + " " + (have === 1 ? "has" : "have") + " this player)"));
        details.open = !!openCards[p.id];
        details.addEventListener("toggle", function () { openCards[p.id] = details.open; });
        var table = make("table", "pl-table");
        var cap = make("caption", "pl-sr", p.name + " in each game");
        var thead = make("thead"), hr = make("tr");
        ["Game", "Name it uses", "Player names setting"].forEach(function (t) { var th = make("th", "", t); th.scope = "col"; hr.appendChild(th); });
        thead.appendChild(hr);
        var tbody = make("tbody");
        games.forEach(function (g) { tbody.appendChild(gameRow(p, g, recs[g.id])); });
        table.appendChild(cap); table.appendChild(thead); table.appendChild(tbody);
        details.appendChild(table);
        li.appendChild(details);
        cards.appendChild(li);
      });
      if (all.length) siteList.appendChild(group);
      // what the setting is now
      var modes = {}, n = 0;
      games.forEach(function (g) { var m = modeOf(g.id); modes[m] = (modes[m] || 0) + 1; n++; });
      var parts = Object.keys(modes);
      modeNote.textContent = "";
      if (parts.length === 1 && parts[0] === "game") {
        modeNote.appendChild(document.createTextNode("No game uses these players yet: the setting “Player names” is “as the game does” for every game. "));
      } else {
        var VERBS = { game: ["asks as the original does", "ask as the original does"], fill: ["fills in the name", "fill in the name"],
          create: ["uses the player", "use the player"] };
        modeNote.appendChild(document.createTextNode("Player names: " + parts.map(function (m) {
          var v = VERBS[m] || [m, m];
          return (modes[m] === n ? "all " + n + " games " : modes[m] + " game" + (modes[m] > 1 ? "s " : " ")) + v[modes[m] > 1 ? 1 : 0];
        }).join(", ") + ". "));
      }
      var a = make("a", "", "Change it under Settings");
      a.href = "#settings";
      modeNote.appendChild(a);
      modeNote.appendChild(document.createTextNode("."));
    }

    addForm.addEventListener("submit", function (ev) {
      ev.preventDefault();
      var r = ByopopProfiles.add(addInput.value);
      if (!r.ok) { say(r.error, true, null, addInput); addInput.focus(); return; }
      addInput.value = "";
      say(r.profile.name + " was added." + (ByopopProfiles.current() && ByopopProfiles.current().id === r.profile.id ? " It is the player in use." : ""));
      renderAll();
      addInput.focus();
    });

    // ---------- the players inside the games ----------
    function pendingFor(game, name) {
      var found = null;
      ByopopProfiles.ops(game).forEach(function (op) {
        if (op.op === "rename" && (op.from || "").toLowerCase() === name.toLowerCase()) found = op;
        else if (op.op === "delete" && (op.name || "").toLowerCase() === name.toLowerCase()) found = op;
      });
      return found;
    }
    function renderGames() {
      var recs = ByopopProfiles.games(), unseen = [], any = false;
      gameList.textContent = "";
      games.forEach(function (g) {
        var rec = recs[g.id], queued = ByopopProfiles.ops(g.id);
        if (!rec) { unseen.push(g.title); return; }
        if (!rec.names.length && !queued.length) { any = true; gameList.appendChild(emptyGame(g, rec)); return; }
        any = true;
        var fs = make("fieldset", "pl-game");
        var lg = make("legend", "", g.title);
        fs.appendChild(lg);
        var sub = make("p", "pl-hint", (rec.kind === "scores" ? "High-score names. " : "") + (rec.rules ? "Names: " + rec.rules + ". " : "") +
          (rec.at ? "Reported " + when(rec.at) + "." : ""));
        fs.appendChild(sub);
        if (queued.length) {
          fs.appendChild(make("p", "pl-pending", queued.length + " change" + (queued.length > 1 ? "s" : "") +
            " waiting: " + g.title + " applies " + (queued.length > 1 ? "them" : "it") + " the next time it starts."));
        }
        var ul = make("ul", "pl-names");
        rec.names.forEach(function (n) {
          var li = make("li");
          var cid = nextId("pl-t");
          var cb = make("input");
          cb.type = "checkbox"; cb.id = cid; cb.checked = !!ticked[key(g.id, n)];
          cb.addEventListener("change", function () {
            if (cb.checked) ticked[key(g.id, n)] = true; else delete ticked[key(g.id, n)];
            countTicked();
          });
          var lb = make("label", "", n);
          lb.htmlFor = cid;
          li.appendChild(cb); li.appendChild(lb);
          if (rec.current && rec.current === n && rec.kind !== "scores") li.appendChild(make("span", "pl-badge", "current"));
          var op = pendingFor(g.id, n);
          if (op) {
            var t = op.op === "rename" ? "Pending: rename to " + quote(op.to) : "Pending: delete";
            li.appendChild(make("span", "pl-pending-tag", t));
            var cx = make("button", "pl-link", "Cancel");
            cx.type = "button";
            cx.setAttribute("aria-label", "Cancel the pending change of " + n + " in " + g.title);
            cx.addEventListener("click", function () { ByopopProfiles.cancel(g.id, op.id); say("Cancelled."); renderAll(); });
            li.appendChild(cx);
          }
          ul.appendChild(li);
        });
        fs.appendChild(ul);
        gameList.appendChild(fs);
      });
      if (!any && !unseen.length) gameList.appendChild(make("p", "pl-empty", "No games are listed."));
      if (unseen.length) {
        var p = make("p", "pl-hint", "Not reported yet (the game has not started since players were added): " + unseen.join(", ") + ".");
        gameList.appendChild(p);
      }
      countTicked();
    }
    function emptyGame(g, rec) {
      var p = make("p", "pl-hint");
      p.appendChild(make("strong", "", g.title));
      p.appendChild(document.createTextNode(rec.kind === "scores" ? ": no high-score name remembered yet." : ": no players yet."));
      return p;
    }
    function tickedItems() {
      var recs = ByopopProfiles.games(), out = [];
      games.forEach(function (g) {
        var rec = recs[g.id];
        if (!rec) return;
        rec.names.forEach(function (n) { if (ticked[key(g.id, n)]) out.push({ game: g.id, name: n, kind: rec.kind }); });
      });
      return out;
    }
    function countTicked() {
      var n = tickedItems().length;
      bulkCount.textContent = n ? n + " ticked" : "None ticked";
    }
    tickAll.addEventListener("click", function () {
      var recs = ByopopProfiles.games();
      games.forEach(function (g) { var rec = recs[g.id]; if (rec) rec.names.forEach(function (n) { ticked[key(g.id, n)] = true; }); });
      renderGames();
    });
    tickNone.addEventListener("click", function () { ticked = {}; renderGames(); });

    bulk.addEventListener("submit", function (ev) {
      ev.preventDefault();
      var items = tickedItems(), to = ByopopProfiles.clean(bulkInput.value);
      if (!items.length) { say("Tick the players to rename first.", true); return; }
      if (!to) { say("Type the new name first.", true, null, bulkInput); bulkInput.focus(); return; }
      var queued = 0, games_ = {};
      items.forEach(function (it) {
        if (ByopopProfiles.queue(it.game, { op: "rename", from: it.name, to: to })) { queued++; games_[it.game] = true; }
      });
      ticked = {};
      bulkInput.value = "";
      var n = Object.keys(games_).length;
      say(queued ? "Queued " + queued + " rename" + (queued > 1 ? "s" : "") + " to " + quote(to) + " in " + n + " game" + (n > 1 ? "s" : "") +
        ". Each game applies them the next time it starts and reports what it did below; a game that already has a player called " +
        quote(to) + " skips it, as players are never merged." : "This browser would not keep the changes.", !queued);
      renderAll();
    });

    // ---------- deleting inside the games ----------
    bulkDelete.addEventListener("click", function () {
      var items = tickedItems().filter(function (it) { return it.kind !== "scores"; });
      var skipped = tickedItems().length - items.length;
      if (!items.length) { say(skipped ? "A name remembered for high scores is not a player: there is nothing to delete. Rename it instead." : "Tick the players to delete first.", true); return; }
      askDelete(items);
    });
    // The in-page confirmation: what will be lost, game by game, and the player's yes
    var confirmReturn = null;  // what had the focus when the question came up
    function returnFocus() {
      var r = confirmReturn;
      confirmReturn = null;
      if (r && r.isConnected) r.focus(); else bulkDelete.focus();
    }
    function askDelete(items) {
      confirmReturn = document.activeElement;
      confirming = { items: items };
      renderConfirm();
      var t = confirmBox.querySelector("button.pl-danger");
      if (t) t.focus();
      confirmBox.scrollIntoView({ block: "nearest" });
    }
    function renderConfirm() {
      confirmBox.textContent = "";
      // a dialog only while the question is up: its name and text are made with it
      ["role", "aria-labelledby", "aria-describedby"].forEach(function (a) { confirmBox.removeAttribute(a); });
      if (!confirming) { confirmBox.hidden = true; return; }
      var recs = ByopopProfiles.games();
      confirmBox.hidden = false;
      confirmBox.setAttribute("role", "alertdialog");
      confirmBox.setAttribute("aria-labelledby", "pl-confirm-title");
      confirmBox.setAttribute("aria-describedby", "pl-confirm-desc");
      var h = make("h4", "", "Delete these players in the games?");
      h.id = "pl-confirm-title";
      confirmBox.appendChild(h);
      var desc = make("p", "", "This deletes each player and what the game keeps for it, the way the game’s own delete does. " +
        "It cannot be undone. Each game does it the next time it starts.");
      desc.id = "pl-confirm-desc";
      confirmBox.appendChild(desc);
      var ul = make("ul");
      confirming.items.forEach(function (it) {
        var rec = recs[it.game] || {};
        var li = make("li");
        li.appendChild(make("strong", "", (title[it.game] || it.game) + ": " + it.name));
        li.appendChild(document.createTextNode(" — " + (rec.deletes || "the player and the progress that goes with it")));
        ul.appendChild(li);
      });
      confirmBox.appendChild(ul);
      var row = make("div", "pl-actions");
      var yes = make("button", "pl-danger", "Delete " + confirming.items.length + " player" + (confirming.items.length > 1 ? "s" : ""));
      yes.type = "button";
      yes.addEventListener("click", function () {
        var queued = 0;
        confirming.items.forEach(function (it) { if (ByopopProfiles.queue(it.game, { op: "delete", name: it.name })) queued++; });
        confirming = null;
        ticked = {};
        say(queued ? "Queued " + queued + " delete" + (queued > 1 ? "s" : "") + ". Each game does it the next time it starts." : "This browser would not keep the changes.", !queued);
        renderAll();
        returnFocus();
      });
      var no = make("button", "", "Keep them");
      no.type = "button";
      no.addEventListener("click", function () { confirming = null; renderConfirm(); say("Nothing was deleted."); returnFocus(); });
      row.appendChild(yes); row.appendChild(no);
      confirmBox.appendChild(row);
    }
    confirmBox.addEventListener("keydown", function (ev) {
      if (ev.key === "Escape") { confirming = null; renderConfirm(); say("Nothing was deleted."); returnFocus(); }
    });

    // ---------- what the games did ----------
    function renderResults() {
      var recs = ByopopProfiles.games(), items = [];
      games.forEach(function (g) {
        var rec = recs[g.id];
        if (rec && Array.isArray(rec.results)) rec.results.forEach(function (r) { items.push({ game: g, r: r }); });
      });
      items.sort(function (a, b) { return String(b.r.at).localeCompare(String(a.r.at)); });
      resultsList.textContent = "";
      resultsTitle.hidden = resultsClear.hidden = !items.length;
      items.slice(0, 30).forEach(function (it) {
        var r = it.r, li = make("li", r.status === "done" ? "pl-done" : "pl-skipped");
        var what = r.op === "rename" ? "rename " + quote(r.from || "") + " to " + quote(r.to || "") : "delete " + quote(r.name || "");
        li.appendChild(make("strong", "", it.game.title + ": "));
        li.appendChild(document.createTextNode(what + " — " + (r.status === "done" ? "done" : "skipped") + (r.note ? ". " + r.note : ".")));
        var t = when(r.at);
        if (t) li.appendChild(make("span", "pl-muted", " (" + t + ")"));
        resultsList.appendChild(li);
      });
    }
    resultsClear.addEventListener("click", function () {
      ByopopProfiles.clearResults();
      say("The list was cleared.");
      renderAll();
    });

    function renderAll() {
      renderSite(); renderGames(); renderConfirm(); renderResults();
    }
    renderAll();
    // another tab changed something (a game reported, the settings changed)
    window.addEventListener("storage", function (ev) {
      if (ev.key === null || ev.key === ByopopProfiles.KEY || ev.key === ByopopProfiles.GAME_KEY || ev.key === ByopopProfiles.OPS_KEY ||
        ev.key === ByopopSettings.KEY) renderAll();
    });
    document.addEventListener("visibilitychange", function () { if (!document.hidden) renderAll(); });
    window.addEventListener("pageshow", renderAll);
    return { refresh: renderAll };
  }

  return { init: init };
})();
