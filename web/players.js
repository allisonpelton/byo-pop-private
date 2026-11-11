// The Players section of the listing page (docs/WEB_PLAYER.md, "Player profiles"): the site's players, which games have each
// and under what name, the players inside the games, and renaming and deleting them across games. The data is web/profiles.js's
// (ByopopProfiles: the site's list, what each game reported of its own players, the operations waiting for a game) and the
// setting "profile" (ByopopSettings). Nothing is sent anywhere; it uses real form controls, and no alert, confirm or prompt.
//
// A game that is not running cannot be changed from here, so a rename or a delete is queued and the game applies it the
// next time it starts (with its own functions); until then the section shows it as pending, and afterwards what became of
// it. A game that has never reported its players (not started since this existed) is not listed and cannot be targeted.
var ByopopPlayers = (function () {
  "use strict";
  var MODE_WORDS = { game: "players.setting.game", fill: "players.setting.fill", create: "players.setting.create" };

  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
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
    var status = make("p", "pl-status");
    status.setAttribute("role", "status");
    status.setAttribute("aria-live", "polite");
    // an error is its own line, an alert (announced at once), and the field it is about is described by it
    var alertEl = make("p", "pl-status pl-error pl-alert");
    alertEl.id = nextId("pl-alert");
    alertEl.setAttribute("role", "alert");
    root.appendChild(status);
    root.appendChild(alertEl);

    // ----- your players -----
    var h3a = make("h3", "", ByopopStrings.t("players.title"));
    var siteList = make("div", "pl-site");
    var addForm = make("form", "pl-form");
    var addId = nextId("pl-add");
    var addLabel = make("label", "", ByopopStrings.t("players.new"));
    addLabel.htmlFor = addId;
    var addInput = make("input");
    addInput.type = "text"; addInput.id = addId; addInput.maxLength = ByopopProfiles.NAME_MAX; addInput.autocomplete = "off";
    addInput.setAttribute("autocapitalize", "off"); addInput.spellcheck = false;
    var addButton = make("button", "", ByopopStrings.t("players.add"));
    addButton.type = "submit";
    addForm.appendChild(addLabel); addForm.appendChild(addInput); addForm.appendChild(addButton);
    var modeNote = make("p", "pl-hint");
    root.appendChild(h3a); root.appendChild(siteList); root.appendChild(addForm); root.appendChild(modeNote);

    // ----- the players inside the games -----
    var h3b = make("h3", "", ByopopStrings.t("players.in_games"));
    var gameList = make("div", "pl-games");
    var bulk = make("form", "pl-form pl-bulk");
    var bulkId = nextId("pl-bulk");
    var bulkLabel = make("label", "", ByopopStrings.t("players.bulk.label"));
    bulkLabel.htmlFor = bulkId;
    var bulkInput = make("input");
    bulkInput.type = "text"; bulkInput.id = bulkId; bulkInput.maxLength = ByopopProfiles.NAME_MAX; bulkInput.autocomplete = "off";
    bulkInput.spellcheck = false;
    var bulkRename = make("button", "", ByopopStrings.t("players.bulk.rename"));
    bulkRename.type = "submit";
    var bulkDelete = make("button", "pl-danger", ByopopStrings.t("players.bulk.delete"));
    bulkDelete.type = "button";
    var tickAll = make("button", "", ByopopStrings.t("players.bulk.tick_all"));
    tickAll.type = "button";
    var tickNone = make("button", "", ByopopStrings.t("players.bulk.tick_none"));
    tickNone.type = "button";
    var bulkCount = make("span", "pl-count");
    bulk.appendChild(bulkLabel); bulk.appendChild(bulkInput); bulk.appendChild(bulkRename); bulk.appendChild(bulkDelete);
    bulk.appendChild(tickAll); bulk.appendChild(tickNone); bulk.appendChild(bulkCount);
    var confirmBox = make("div", "pl-confirm");
    confirmBox.hidden = true;
    var resultsTitle = make("h3", "", ByopopStrings.t("players.results.title"));
    var resultsList = make("ul", "pl-results");
    var resultsClear = make("button", "", ByopopStrings.t("players.results.clear"));
    resultsClear.type = "button";
    root.appendChild(h3b); root.appendChild(gameList); root.appendChild(bulk);
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
        var b = make("button", "pl-link", ByopopStrings.t("players.undo"));
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
      var used = make("td"), setting = make("td", "", ByopopStrings.t(MODE_WORDS[modeOf(g.id)] || MODE_WORDS.game));
      if (!rec) {
        used.appendChild(make("span", "pl-muted", ByopopStrings.t("players.row.not_started")));
      } else {
        var m = ByopopProfiles.match(rec, p.name);
        var fit = (rec.fits && typeof rec.fits[p.name] === "string") ? rec.fits[p.name] : null;
        var flags = [];
        if (rec.kind === "scores") {
          if (fit === "") used.appendChild(make("span", "pl-muted", ByopopStrings.t("players.row.no_score_name")));
          else used.appendChild(document.createTextNode(ByopopStrings.t("players.row.scores", {name: fit === null ? p.name : fit})));
          if (fit !== null && fit !== "" && fit !== p.name) flags.push(ByopopStrings.t("players.flag.changed"));
        } else if (m) {
          used.appendChild(make("strong", "", m.name));
          if (!m.exact) flags.push(fit !== null && fit !== p.name && fit.toLowerCase() === m.name.toLowerCase()
            ? ByopopStrings.t("players.flag.changed_asked", {name: p.name}) : ByopopStrings.t("players.flag.written", {name: m.name}));
          if (rec.current && rec.current.toLowerCase() === m.name.toLowerCase()) flags.push(ByopopStrings.t("players.flag.current"));
        } else {
          used.appendChild(make("span", "pl-muted", ByopopStrings.t("players.row.no_player")));
          if (fit === "") flags.push(ByopopStrings.t("players.flag.nothing"));
          else if (fit !== null && fit !== p.name) flags.push(ByopopStrings.t("players.flag.would_be", {name: fit}));
        }
        if (flags.length) used.appendChild(make("span", "pl-flag", " " + ByopopStrings.t("players.flags", {flags: flags.join("; ")})));
      }
      tr.appendChild(used); tr.appendChild(setting);
      return tr;
    }
    // a list that is drawn again loses the focus: give it back to a player's Rename button
    function focusRename(id) {
      var b = siteList.querySelector('button[data-rename="' + id + '"]');
      if (b) b.focus();
    }

    function renderSite() {
      var all = ByopopProfiles.list(), cur = ByopopProfiles.current(), recs = ByopopProfiles.games();
      siteList.textContent = "";
      if (!all.length) {
        siteList.appendChild(make("p", "pl-empty", ByopopStrings.t("players.none")));
      }
      var group = make("fieldset", "pl-use");
      group.appendChild(make("legend", "pl-sr", ByopopStrings.t("players.in_use_legend")));
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
          if (radio.checked && ByopopProfiles.setCurrent(p.id).ok) { say(ByopopStrings.t("players.is_current", {name: p.name})); renderAll(); }
        });
        label.appendChild(radio);
        label.appendChild(make("span", "pl-name", p.name));
        head.appendChild(label);
        if (cur && cur.id === p.id) head.appendChild(make("span", "pl-badge", ByopopStrings.t("players.badge")));
        var actions = make("div", "pl-actions");
        var rn = make("button", "", ByopopStrings.t("players.rename"));
        rn.type = "button";
        rn.setAttribute("aria-label", ByopopStrings.t("players.rename_label", {name: p.name}));
        rn.setAttribute("data-rename", p.id);
        rn.addEventListener("click", function () { renaming = p.id; renderSite(); var f = siteList.querySelector(".pl-rename input"); if (f) f.focus(); });
        var inGames = make("button", "", ByopopStrings.t("players.delete_in_games"));
        inGames.type = "button";
        inGames.setAttribute("aria-label", ByopopStrings.t("players.delete_in_games_label", {name: p.name}));
        inGames.addEventListener("click", function () {
          var items = [];
          games.forEach(function (g) {
            var rec = recs[g.id];
            if (!rec || rec.kind === "scores") return;
            var m = ByopopProfiles.match(rec, p.name);
            if (m) items.push({ game: g.id, name: m.name });
          });
          if (!items.length) { say(ByopopStrings.t("players.not_in_games", {name: p.name}), true); return; }
          askDelete(items);
        });
        var rm = make("button", "", ByopopStrings.t("players.remove"));
        rm.type = "button";
        rm.setAttribute("aria-label", ByopopStrings.t("players.remove_label", {name: p.name}));
        rm.title = ByopopStrings.t("players.remove_title");
        rm.addEventListener("click", function () {
          var wasCurrent = !!cur && cur.id === p.id;
          if (!ByopopProfiles.remove(p.id).ok) { say(ByopopStrings.t("players.keep_change"), true); return; }
          say(ByopopStrings.t("players.removed", {name: p.name}), false, function () {
            var back = ByopopProfiles.add(p.name);
            if (back.ok && wasCurrent) ByopopProfiles.setCurrent(back.profile.id);
            say(ByopopStrings.t("players.back", {name: p.name}));
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
          var l2 = make("label", "", ByopopStrings.t("players.new_name", {name: p.name}));
          l2.htmlFor = iid;
          var input = make("input");
          input.type = "text"; input.id = iid; input.value = p.name; input.maxLength = ByopopProfiles.NAME_MAX; input.autocomplete = "off";
          var save = make("button", "", ByopopStrings.t("players.save")); save.type = "submit";
          var cancel = make("button", "", ByopopStrings.t("players.cancel")); cancel.type = "button";
          cancel.addEventListener("click", function () { renaming = null; renderSite(); focusRename(p.id); });
          form.addEventListener("submit", function (ev) {
            ev.preventDefault();
            var r = ByopopProfiles.rename(p.id, input.value);
            if (!r.ok) { say(r.error, true, null, input); input.focus(); return; }
            renaming = null;
            say(ByopopStrings.t("players.renamed", {name: r.profile.name}));
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
        details.appendChild(make("summary", "", ByopopStrings.t("players.card.in_games", {n: have})));
        details.open = !!openCards[p.id];
        details.addEventListener("toggle", function () { openCards[p.id] = details.open; });
        var table = make("table", "pl-table");
        var cap = make("caption", "pl-sr", ByopopStrings.t("players.card.caption", {name: p.name}));
        var thead = make("thead"), hr = make("tr");
        [ByopopStrings.t("players.card.game"), ByopopStrings.t("players.card.name"), ByopopStrings.t("players.card.setting")].forEach(function (t) { var th = make("th", "", t); th.scope = "col"; hr.appendChild(th); });
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
      // as values (the user's rule: values, not sentences), and only while the games differ: one value for every game is the
      // setting's own, just above
      var NAMES = { game: "settings.game_default", fill: "settings.profile.fill", create: "settings.profile.create" };
      modeNote.hidden = parts.length < 2;
      modeNote.textContent = parts.length < 2 ? "" : ByopopStrings.t("players.mode.note", {parts: parts.map(function (m) {
        // a mode this page does not know is named as it is
        return ByopopStrings.t("players.mode.part", {mode: NAMES[m] ? ByopopStrings.t(NAMES[m]) : m, n: modes[m]});
      }).join(", ")});
      // (the setting itself is just above, in the same Players section of Settings: no link to it)
    }

    addForm.addEventListener("submit", function (ev) {
      ev.preventDefault();
      var r = ByopopProfiles.add(addInput.value);
      if (!r.ok) { say(r.error, true, null, addInput); addInput.focus(); return; }
      addInput.value = "";
      say(ByopopStrings.t(ByopopProfiles.current() && ByopopProfiles.current().id === r.profile.id ? "players.added_current" : "players.added", {name: r.profile.name}));
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
      var recs = ByopopProfiles.games(), any = false;
      gameList.textContent = "";
      games.forEach(function (g) {
        var rec = recs[g.id], queued = ByopopProfiles.ops(g.id);
        if (!rec) return;
        if (!rec.names.length && !queued.length) return;
        any = true;
        var fs = make("fieldset", "pl-game");
        var lg = make("legend", "", g.title);
        fs.appendChild(lg);
        var hint = (rec.kind === "scores" ? ByopopStrings.t("players.game.scores") + " " : "") + (rec.rules ? ByopopStrings.t("players.game.rules", {rules: rec.rules}) : "");
        // a game's naming rules: the legend's tooltip, and for screen readers the fieldset's description (values, not sentences)
        if (hint) {
          lg.title = hint;
          var rules = make("p", "visually-hidden", hint);
          rules.id = nextId("pl-r");
          fs.setAttribute("aria-describedby", rules.id);
          fs.appendChild(rules);
        }
        if (queued.length) {
          fs.appendChild(make("p", "pl-pending", ByopopStrings.t("players.game.pending", {n: queued.length, title: g.title})));
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
          if (rec.current && rec.current === n && rec.kind !== "scores") li.appendChild(make("span", "pl-badge", ByopopStrings.t("players.game.current")));
          var op = pendingFor(g.id, n);
          if (op) {
            var t = op.op === "rename" ? ByopopStrings.t("players.game.pending_rename", {to: op.to}) : ByopopStrings.t("players.game.pending_delete");
            li.appendChild(make("span", "pl-pending-tag", t));
            var cx = make("button", "pl-link", ByopopStrings.t("players.cancel"));
            cx.type = "button";
            cx.setAttribute("aria-label", ByopopStrings.t("players.game.cancel_label", {name: n, title: g.title}));
            cx.addEventListener("click", function () { ByopopProfiles.cancel(g.id, op.id); say(ByopopStrings.t("players.game.cancelled")); renderAll(); });
            li.appendChild(cx);
          }
          ul.appendChild(li);
        });
        fs.appendChild(ul);
        gameList.appendChild(fs);
      });
      if (!any) gameList.appendChild(make("p", "pl-empty", ByopopStrings.t("players.game.none")));
      bulk.hidden = !any;
      countTicked();
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
      bulkCount.textContent = n ? ByopopStrings.t("players.bulk.ticked", {n: n}) : ByopopStrings.t("players.bulk.none_ticked");
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
      if (!items.length) { say(ByopopStrings.t("players.bulk.tick_first"), true); return; }
      if (!to) { say(ByopopStrings.t("players.bulk.type_first"), true, null, bulkInput); bulkInput.focus(); return; }
      var queued = 0, games_ = {};
      items.forEach(function (it) {
        if (ByopopProfiles.queue(it.game, { op: "rename", from: it.name, to: to })) { queued++; games_[it.game] = true; }
      });
      ticked = {};
      bulkInput.value = "";
      var n = Object.keys(games_).length;
      say(queued ? ByopopStrings.t("players.bulk.queued", {renames: ByopopStrings.t("players.n.rename", {n: queued}), to: to, games: ByopopStrings.t("players.n.game", {n: n})})
        : ByopopStrings.t("players.keep_changes"), !queued);
      renderAll();
    });

    // ---------- deleting inside the games ----------
    bulkDelete.addEventListener("click", function () {
      var items = tickedItems().filter(function (it) { return it.kind !== "scores"; });
      var skipped = tickedItems().length - items.length;
      if (!items.length) { say(ByopopStrings.t(skipped ? "players.delete.scores_only" : "players.delete.tick_first"), true); return; }
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
      var h = make("h4", "", ByopopStrings.t("players.delete.title"));
      h.id = "pl-confirm-title";
      confirmBox.appendChild(h);
      var desc = make("p", "", ByopopStrings.t("players.delete.desc"));
      desc.id = "pl-confirm-desc";
      confirmBox.appendChild(desc);
      var ul = make("ul");
      confirming.items.forEach(function (it) {
        var rec = recs[it.game] || {};
        var li = make("li");
        li.appendChild(make("strong", "", (title[it.game] || it.game) + ": " + it.name));
        li.appendChild(document.createTextNode(" " + ByopopStrings.t("players.delete.line", {what: rec.deletes || ByopopStrings.t("players.delete.default")})));
        ul.appendChild(li);
      });
      confirmBox.appendChild(ul);
      var row = make("div", "pl-actions");
      var yes = make("button", "pl-danger", ByopopStrings.t("players.delete.yes", {n: confirming.items.length}));
      yes.type = "button";
      yes.addEventListener("click", function () {
        var queued = 0;
        confirming.items.forEach(function (it) { if (ByopopProfiles.queue(it.game, { op: "delete", name: it.name })) queued++; });
        confirming = null;
        ticked = {};
        say(queued ? ByopopStrings.t("players.delete.queued", {n: queued}) : ByopopStrings.t("players.keep_changes"), !queued);
        renderAll();
        returnFocus();
      });
      var no = make("button", "", ByopopStrings.t("players.delete.no"));
      no.type = "button";
      no.addEventListener("click", function () { confirming = null; renderConfirm(); say(ByopopStrings.t("players.delete.nothing")); returnFocus(); });
      row.appendChild(yes); row.appendChild(no);
      confirmBox.appendChild(row);
    }
    confirmBox.addEventListener("keydown", function (ev) {
      if (ev.key === "Escape") { confirming = null; renderConfirm(); say(ByopopStrings.t("players.delete.nothing")); returnFocus(); }
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
        var what = r.op === "rename" ? ByopopStrings.t("players.result.rename", {from: r.from || "", to: r.to || ""}) : ByopopStrings.t("players.result.delete", {name: r.name || ""});
        li.appendChild(make("strong", "", ByopopStrings.t("players.result.game", {title: it.game.title}) + " "));
        li.appendChild(document.createTextNode(ByopopStrings.t(r.note ? "players.result.line_note" : "players.result.line",
          {what: what, status: ByopopStrings.t(r.status === "done" ? "players.result.done" : "players.result.skipped"), note: r.note})));
        var t = when(r.at);
        if (t) li.appendChild(make("span", "pl-muted", " " + ByopopStrings.t("players.result.when", {when: t})));
        resultsList.appendChild(li);
      });
    }
    resultsClear.addEventListener("click", function () {
      ByopopProfiles.clearResults();
      say(ByopopStrings.t("players.results.cleared"));
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
