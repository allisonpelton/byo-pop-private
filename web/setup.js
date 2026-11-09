// The Setup window (docs/WEB_PLAYER.md, "The Setup window"; the user, 2026-11-09: "I did still like the fake installer skin idea
// though, so add an installer window where files can be dropped in bulk"): Desktop 95's "byo-pop Setup", a wizard for adding many
// games at once. It is a new face on the listing's own import (web/addgames.js): the same sorting of folders into games
// (web/gameimport.js), the same terms and agreement for each game (web/terms.js), the same queue that copies one game at a time.
//
// Its pages, with Back, Next and Cancel: Your games (a drop target for game folders, .zip files or a folder of many games, and
// Browse), a Licence page for each game found whose terms are not yet agreed to, Copying (an overall bar and a row for each game)
// and Finish (each game's result). Each game has terms of its own, so the Licence pages come once the games are known. The Pop
// menu's Add Games and the All games window's Add games button open it; folders or .zip files dropped on the desktop, outside a
// game's window, open it with them, at the first page after Your games; a drop on a game's card stays that game's own import.
var ByopopSetup = (function () {
  "use strict";

  // ---------- the logic (tools/setuptest.js tests it under Node) ----------
  // The pages for the games found, after Your games: a Licence page for each game to be installed whose terms are not agreed
  // to, in the order found, then Copying and Finish. games: [{ id, agreed, here, supported }]
  function plan(games) {
    var out = [];
    games.forEach(function (g) { if (g.supported && !g.here && !g.agreed) out.push({ page: "licence", id: g.id }); });
    out.push({ page: "copying" });
    out.push({ page: "finish" });
    return out;
  }
  // What a game came to: installed, already (its files were here), unsupported (no game this site plays), skipped (its terms
  // not agreed, or Setup cancelled before it began), error (code 1: the files could not be read; 2: they could not be copied)
  var RESULT = { installed: "setup.result.installed", already: "setup.result.already", unsupported: "setup.result.unsupported",
    skipped: "setup.result.skipped", error: "setup.result.error" };
  // Finish's rows and counts from the results ([{ title, kind, code, reason }]), in the order given; t: the strings' lookup
  function summary(results, t) {
    var counts = { installed: 0, already: 0, unsupported: 0, skipped: 0, error: 0 };
    var rows = results.map(function (r) {
      counts[r.kind] = (counts[r.kind] || 0) + 1;
      return { title: r.title, kind: r.kind, value: t(RESULT[r.kind], r.kind === "error" ? { code: r.code } : undefined), tip: r.reason || "" };
    });
    return { rows: rows, counts: counts, total: results.length, failed: counts.error };
  }
  var logic = { plan: plan, summary: summary, RESULT: RESULT };
  if (typeof document === "undefined") return { logic: logic };

  // ---------- the window ----------
  function T(k, v) { return ByopopStrings.t(k, v); }
  function make(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  var A = function () { return window.ByopopAddGames; };
  var ui = null, deskApi = null;
  // the state of one run: the folders found (each { key, folder, game, choices, entries, root, here, agreed, supported, el }), the
  // pages after Your games, the page shown, the results by key, the jobs queued
  var found = [], steps = [], at = -1, results = {}, pending = 0, run = 0, agreedNow = {};

  function badgeOf(game) {
    var a = game && document.querySelector('a.tile[data-game="' + game.spec.id + '"] .badge');
    var b = a ? a.cloneNode(true) : make("span", "badge");
    b.removeAttribute("draggable");
    b.removeAttribute("id");
    b.classList.add("su-icon");
    b.setAttribute("aria-hidden", "true");
    if (!a) b.classList.add("su-icon-none");
    return b;
  }
  function filesHere(game) {
    try { var rec = ByopopGameFiles.all(); return ByopopGameFiles.present(rec[game.spec.id]); } catch (e) { return false; }
  }
  function agreed(game) {
    var terms = (window.ByopopTermsTexts || {})[game.spec.id];
    if (!terms) return false;
    return agreedNow[game.spec.id] === terms.version || ByopopTerms.agreed(game.spec.id, terms.version);
  }

  function build() {
    var el = make("section", "su-win");
    el.id = "setup";
    el.setAttribute("aria-label", T("setup.title"));
    var wiz = make("div", "su-wiz");
    var art = make("div", "su-art");
    art.setAttribute("aria-hidden", "true");
    var mark = make("img", "su-art-mark");
    mark.src = "favicon.svg"; mark.alt = ""; mark.width = 48; mark.height = 48;
    art.appendChild(mark);
    art.appendChild(make("i", "su-art-box"));
    var pages = make("div", "su-pages");
    wiz.appendChild(art); wiz.appendChild(pages);
    el.appendChild(wiz);
    function page(id) {
      var pg = make("section", "su-page");
      pg.setAttribute("data-page", id);
      var h = make("h3", "su-title");
      h.id = "su-title-" + id;
      h.tabIndex = -1;
      pg.setAttribute("aria-labelledby", h.id);
      pg.appendChild(h);
      pg.hidden = true;
      pages.appendChild(pg);
      return { el: pg, title: h };
    }
    var u = { el: el };
    // Your games: the drop target, Browse, and what was found
    u.games = page("games");
    u.games.title.textContent = T("setup.page.games");
    u.drop = make("div", "su-drop");
    u.drop.appendChild(make("span", "su-drop-text", T("setup.drop")));
    u.browse = make("button", "su-browse", T("setup.browse"));
    u.browse.type = "button";
    u.drop.appendChild(u.browse);
    u.pick = make("input");
    u.pick.type = "file"; u.pick.multiple = true; u.pick.hidden = true; u.pick.tabIndex = -1;
    u.pick.setAttribute("webkitdirectory", "");
    u.pick.setAttribute("aria-hidden", "true");
    try { if (window.matchMedia("(pointer: coarse) and (not (any-pointer: fine))").matches) { u.pick.removeAttribute("webkitdirectory"); u.pick.accept = ".zip,application/zip"; } } catch (e) {}
    u.drop.appendChild(u.pick);
    u.games.el.appendChild(u.drop);
    u.found = make("ul", "su-rows su-found");
    u.found.setAttribute("aria-label", T("setup.found"));
    u.games.el.appendChild(u.found);
    // Licence: the game's terms, as its own terms card shows them, the box to agree, and Don't add this game
    u.licence = page("licence");
    u.terms = make("div", "terms su-terms");
    u.terms.tabIndex = 0;
    u.terms.setAttribute("role", "region");
    u.terms.setAttribute("aria-label", T("game.terms.region"));
    var agreeRow = make("div", "su-agree-row");
    var lab = make("label", "agree su-agree");
    u.agree = make("input");
    u.agree.type = "checkbox"; u.agree.autocomplete = "off";
    u.agreeText = make("span");
    lab.appendChild(u.agree); lab.appendChild(document.createTextNode(" ")); lab.appendChild(u.agreeText);
    u.skip = make("button", "su-skip", T("index.terms.skip"));
    u.skip.type = "button";
    agreeRow.appendChild(lab); agreeRow.appendChild(u.skip);
    u.licence.el.appendChild(agreeRow);
    u.licence.el.appendChild(u.terms);
    // Copying: the bar for all of them and a row for each game
    u.copying = page("copying");
    u.copying.title.textContent = T("setup.page.copying");
    u.bar = make("div", "progress su-bar");
    u.bar.setAttribute("role", "progressbar");
    u.bar.setAttribute("aria-valuemin", "0"); u.bar.setAttribute("aria-valuemax", "100"); u.bar.setAttribute("aria-valuenow", "0");
    u.bar.setAttribute("aria-label", T("setup.page.copying"));
    u.fill = make("div");
    u.fill.style.width = "0%";
    u.bar.appendChild(u.fill);
    u.copying.el.appendChild(u.bar);
    u.rows = make("ul", "su-rows");
    u.copying.el.appendChild(u.rows);
    // Finish: each game's result
    u.finish = page("finish");
    u.finish.title.textContent = T("setup.page.finish");
    u.done = make("ul", "su-rows");
    u.finish.el.appendChild(u.done);
    // the one live region: how each game ended, and the page shown
    u.live = make("p", "pl-sr");
    u.live.setAttribute("role", "status");
    el.appendChild(u.live);
    // Back, Next and Cancel, under an etched line
    var nav = make("div", "su-nav");
    u.back = make("button", "su-back", T("setup.back"));
    u.next = make("button", "su-next", T("setup.next"));
    u.cancel = make("button", "su-cancel", T("setup.cancel"));
    [u.back, u.next, u.cancel].forEach(function (b) { b.type = "button"; nav.appendChild(b); });
    el.appendChild(nav);

    u.browse.addEventListener("click", function () { u.pick.click(); });
    u.pick.addEventListener("change", function () {
      var read = A().fromInput(u.pick.files);
      u.pick.value = "";
      if (read) take(read, false);
    });
    // a drop on the window itself is taken here (not by the desktop beneath it); on Your games it adds to what was found
    el.addEventListener("dragover", function (ev) { if (ByopopImport.hasFiles(ev.dataTransfer)) { ev.preventDefault(); ev.dataTransfer.dropEffect = page_() === "games" ? "copy" : "none"; } });
    el.addEventListener("drop", function (ev) {
      if (!ByopopImport.hasFiles(ev.dataTransfer)) return;
      ev.preventDefault(); ev.stopPropagation();
      if (page_() === "games") take(ByopopImport.fromDrop(ev.dataTransfer, A().W().zip), false);
    });
    var depth = 0;
    u.drop.addEventListener("dragenter", function (ev) { if (ByopopImport.hasFiles(ev.dataTransfer)) { depth++; u.drop.classList.add("drop-on"); } });
    u.drop.addEventListener("dragleave", function () { if (--depth <= 0) { depth = 0; u.drop.classList.remove("drop-on"); } });
    u.drop.addEventListener("drop", function () { depth = 0; u.drop.classList.remove("drop-on"); });
    u.agree.addEventListener("change", buttons);
    u.skip.addEventListener("click", function () { var s = steps[at]; if (s && s.page === "licence") { setResult(s.id, "skipped", 0, T("add.not_agreed")); go(at + 1); } });
    u.back.addEventListener("click", function () { if (at > 0) go(at - 1); else if (at === 0) showGames(); });
    u.next.addEventListener("click", onNext);
    u.cancel.addEventListener("click", function () { close(); });
    return u;
  }

  // ---------- Your games ----------
  // the files dropped or chosen: read, sorted into games, and listed; a drop on the desktop (jump) goes straight on once sorted
  function take(promise, jump) {
    var mine = run, reading = make("li", "su-row su-reading", T("setup.reading"));
    ui.found.appendChild(reading);
    buttons();
    promise.then(function (entries) {
      if (mine !== run) return;
      reading.remove();
      var sorted = ByopopImport.sort(entries);
      sorted.found.forEach(addFound);
      (sorted.others || []).forEach(function (o) {
        var note = ByopopImport.otherNote(document, o.game);
        addFound({ folder: o.folder, unsupported: true, reason: T("add.other", { folder: o.folder }) + " " + (note.textContent || "") });
      });
      sorted.rejected.forEach(function (folder) { addFound({ folder: folder, unsupported: true, reason: T("add.rejected", { folders: folder }) }); });
      if (!sorted.found.length && !sorted.rejected.length && !(sorted.others || []).length)
        addFound({ folder: T("setup.dropped"), error: 1, reason: T("add.nothing") });
      buttons();
      if (jump && found.some(function (f) { return f.game || f.choices; })) onNext();
    }, function (e) {
      if (mine !== run) return;
      reading.remove();
      addFound({ folder: T("setup.dropped"), error: 1, reason: (e && e.message) || T("add.read_failed") });
      buttons();
    });
  }
  var keySeq = 0;
  function addFound(f) {
    f.key = "f" + (++keySeq);
    var li = make("li", "su-row");
    var name = make("span", "su-name");
    var value = make("span", "su-value");
    if (f.game) {
      f.here = filesHere(f.game);
      li.appendChild(badgeOf(f.game));
      name.textContent = f.game.title;
      name.title = f.folder;
      value.textContent = T(f.here ? "setup.result.already" : "setup.ready");
    } else if (f.choices && f.choices.length) {
      // a folder that could be one of several games: the player picks which, before Next
      li.appendChild(badgeOf(null));
      name.textContent = f.folder;
      var sel = make("select", "su-pick");
      sel.setAttribute("aria-label", T("add.which") + " " + f.folder);
      f.choices.forEach(function (g) { var o = make("option", null, g.title); o.value = g.target; sel.appendChild(o); });
      f.select = sel;
      value.appendChild(sel);
    } else {
      li.appendChild(badgeOf(null));
      name.textContent = f.folder;
      value.textContent = f.error ? T("setup.result.error", { code: f.error }) : T("setup.result.unsupported");
      value.title = f.reason || "";
    }
    li.appendChild(name); li.appendChild(value);
    f.el = li;
    found.push(f);
    ui.found.appendChild(li);
  }
  function showGames() {
    at = -1;
    show("games");
  }
  // what Next does on each page
  function onNext() {
    var p = page_();
    if (p === "games") {
      // the folders whose game was picked become that game; then the pages for the games found
      found.forEach(function (f) {
        if (!f.game && f.select) { var g = A().byTarget(f.select.value); if (g) { f.game = g; f.here = filesHere(g); } }
      });
      var games = found.filter(function (f) { return f.game; });
      steps = plan(games.map(function (f) { return { id: f.key, agreed: agreed(f.game), here: f.here, supported: true }; }));
      found.forEach(function (f) {
        if (!f.game) setResult(f.key, f.error ? "error" : "unsupported", f.error || 0, f.reason || "");
        else if (f.here) setResult(f.key, "already", 0, "");
      });
      go(0);
    } else if (p === "licence") {
      var s = steps[at], f = byKey(s.id), terms = f && (window.ByopopTermsTexts || {})[f.game.spec.id];
      if (!ui.agree.checked || !terms) return;
      agreedNow[f.game.spec.id] = terms.version;
      ByopopTerms.agree(f.game.spec.id, terms.version);
      delete results[f.key];
      go(at + 1);
    } else if (p === "finish") close();
  }
  function byKey(key) { for (var i = 0; i < found.length; i++) if (found[i].key === key) return found[i]; return null; }
  function setResult(key, kind, code, reason) { results[key] = { kind: kind, code: code, reason: reason }; }

  // ---------- the pages ----------
  function page_() { var s = at < 0 ? null : steps[at]; return s ? s.page : "games"; }
  function go(i) {
    at = Math.max(0, Math.min(steps.length - 1, i));
    var s = steps[at];
    if (s.page === "licence") showLicence(byKey(s.id));
    else if (s.page === "copying") startCopying();
    else show(s.page);
  }
  function show(p) {
    ["games", "licence", "copying", "finish"].forEach(function (k) { ui[k].el.hidden = k !== p; });
    buttons();
    var h = ui[p].title;
    if (ui.el.contains(document.activeElement) || document.activeElement === document.body) { try { h.focus({ preventScroll: true }); } catch (e) {} }
    ui.live.textContent = h.textContent;
  }
  function showLicence(f) {
    var terms = (window.ByopopTermsTexts || {})[f.game.spec.id];
    ui.licence.title.textContent = T("setup.page.licence", { title: f.game.title });
    ui.terms.innerHTML = terms ? terms.html : "";  // the build's own text (web/terms.html, filled in), never anything the player gave
    ui.agreeText.textContent = T("game.terms.agree", { title: f.game.title });
    ui.agree.checked = false;
    show("licence");
    ui.terms.scrollTop = 0;
  }
  // the buttons as each page needs them: Back where there is a page before; Next once there is something to go on with, Finish
  // at the end; Cancel until the end
  function buttons() {
    if (!ui) return;
    var p = page_(), busyReading = !!ui.found.querySelector(".su-reading");
    ui.back.disabled = p === "games" || p === "copying" || p === "finish";
    ui.next.textContent = T(p === "finish" ? "setup.finish" : "setup.next");
    if (p === "games") ui.next.disabled = busyReading || !found.some(function (f) { return f.game || f.select; });
    else if (p === "licence") ui.next.disabled = !ui.agree.checked;
    else if (p === "copying") ui.next.disabled = true;
    else ui.next.disabled = false;
    ui.cancel.disabled = p === "finish";
  }

  // ---------- Copying ----------
  function startCopying() {
    show("copying");
    ui.rows.textContent = "";
    var games = found.filter(function (f) { return f.game && !results[f.key]; });
    var total = games.length, finished = 0;
    function progress() {
      var v = total ? Math.round(finished * 100 / total) : 100;
      ui.fill.style.width = v + "%";
      ui.bar.setAttribute("aria-valuenow", String(v));
    }
    progress();
    pending = total;
    var mine = run;
    games.forEach(function (f) {
      var li = make("li", "su-row");
      li.appendChild(badgeOf(f.game));
      li.appendChild(make("span", "su-name", f.game.title));
      var value = make("span", "su-value", T("add.waiting"));
      li.appendChild(value);
      ui.rows.appendChild(li);
      function end(kind, code, reason) {
        if (mine !== run) return;
        setResult(f.key, kind, code, reason);
        value.textContent = kind === "error" ? T("setup.result.error", { code: code }) : T(RESULT[kind]);
        value.title = reason || "";
        value.className = "su-value su-" + kind;
        finished++;
        progress();
        if (--pending <= 0) setTimeout(function () { if (mine === run) { go(steps.length - 1); finish(); } }, 400);
      }
      A().enqueue({
        game: f.game, entries: f.entries, root: f.root, setup: mine,
        say: function (t) { if (mine === run) value.textContent = t; },
        done: function (ok, t) { if (ok) end("installed", 0, t); else if (t === T("add.not_agreed")) end("skipped", 0, t); else end("error", 2, t); },
        cancelled: function () { end("skipped", 0, T("setup.cancelled")); }
      });
    });
    if (!total) setTimeout(function () { if (mine === run) { go(steps.length - 1); finish(); } }, 0);
  }

  // ---------- Finish ----------
  function finish() {
    var list = found.map(function (f) {
      var r = results[f.key] || { kind: "skipped", code: 0, reason: T("setup.cancelled") };
      return { title: f.game ? f.game.title : f.folder, kind: r.kind, code: r.code, reason: r.reason, game: f.game };
    });
    var sum = summary(list, T);
    ui.done.textContent = "";
    sum.rows.forEach(function (r, i) {
      var li = make("li", "su-row");
      li.appendChild(badgeOf(list[i].game));
      li.appendChild(make("span", "su-name", r.title));
      var v = make("span", "su-value su-" + r.kind, r.value);
      if (r.tip) v.title = r.tip;
      li.appendChild(v);
      ui.done.appendChild(li);
    });
    ui.live.textContent = T("setup.summary", { installed: sum.counts.installed, total: sum.total });
  }

  // ---------- open and close ----------
  function reset() {
    run++;
    // the games of an earlier run not yet begun are left out (one begun goes on to its end in the queue)
    try { A().cancel(function (job) { return job.setup !== undefined && job.setup !== run; }); } catch (e) {}
    found = []; steps = []; at = -1; results = {}; pending = 0;
    ui.found.textContent = ""; ui.rows.textContent = ""; ui.done.textContent = "";
    ui.fill.style.width = "0%"; ui.bar.setAttribute("aria-valuenow", "0");
    showGames();
  }
  function open() {
    if (!ui) return;
    // a run that ended (Finish shown) starts again; one under way is shown as it is
    if (page_() === "finish" || (at < 0 && !found.length)) reset();
    deskApi.show("setup");
    // the focus on Browse while nothing is found yet (the keyboard's way in), once the window has taken it
    if (page_() === "games" && !found.length) setTimeout(function () { try { ui.browse.focus(); } catch (e) {} }, 0);
  }
  // with files dropped on the desktop: a new run, on to the next page once they are sorted (a run under way keeps going, and
  // the drop waits for the player on Your games of a new one once it ends)
  function openWith(promise) {
    if (!ui) return;
    var p = page_();
    if (p === "copying" || p === "licence") { deskApi.show("setup"); return; }
    if (p === "finish") reset();
    deskApi.show("setup");
    take(promise, true);
  }
  function close() {
    var p = page_();
    if (p === "copying") { try { A().cancel(function (job) { return job.setup === run; }); } catch (e) {} }
    reset();
    try { deskApi.close("setup"); } catch (e) {}
  }
  // The window on the listing's desktop, closed at first, with no icon of its own: the Pop menu's Add Games and the Add games
  // button open it
  function attach(desk, at_) {
    if (ui || !document.getElementById("games-title") || !window.ByopopAddGames || !window.ByopopImport) return;
    ui = build();
    var foot = document.querySelector("body > footer");
    if (foot && foot.parentNode) foot.parentNode.insertBefore(ui.el, foot); else document.body.appendChild(ui.el);
    deskApi = desk;
    desk.add(ui.el, { id: "setup", title: T("setup.title"), closable: true, closed: true, noIcon: true, at: at_,
      afterClose: function () { var p = page_(); if (p === "copying") { try { A().cancel(function (job) { return job.setup === run; }); } catch (e) {} } reset(); } });
    reset();
  }
  function ready() { return !!ui && document.documentElement.classList.contains("d95-desk"); }
  return { logic: logic, attach: attach, ready: ready, open: open, openWith: openWith,
    // for tests: the page shown, what was found, and the results
    state: function () { return { page: page_(), found: found.map(function (f) { return { folder: f.folder, game: f.game ? f.game.spec.id : null, here: !!f.here }; }), results: results, steps: steps.slice(), at: at }; } };
})();
if (typeof module !== "undefined") module.exports = ByopopSetup;
