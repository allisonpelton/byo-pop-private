// The screen saver (docs/WEB_PLAYER.md, "The screen saver"; the user, 2026-11-11): Settings > Screen Saver picks None or a game
// whose port has a bot, and the Wait in minutes. When the listing's desktop has had no key, mouse or touch for that long, the
// desktop goes dark and the game plays itself, silently, in a frame over it; any key or mouse movement ends it and leaves everything
// as it was. Preview shows it at once.
//
// The game plays by its bot's moves through the hook every bot gives (docs/WEB_PLAYER.md, "The screen saver"): today the port's
// test command `move` on the player-only page (?player=1&driver=move, web/testkit.js), which exists only on a development host,
// so on the live site the list holds None alone until a game's bot mod (made in its modding phase) answers the same hook there.
// The frame's page runs at ?saver=1 (web/shell.html) with a profile of its own (the user, 2026-11-18: "Screensavers need to be
// reworked to not use your save data"): of the player's it reads the game's files alone, never their saves, settings, players or
// the site's records, and it writes nothing of theirs. The saver's games keep their own saves in a database of their own, the
// game's with "-saver" after its name (PROFILE), which no listing, the Saves window, an export or import or the Trophy Cabinet
// opens. Forgetting the game's files deletes it (wipe), and so does a run that goes wrong, once, before the saver starts that game
// afresh. When a game ends the frame starts another.
var ByopopSaver = (function () {
  "use strict";
  // the games whose port answers `move` (docs/TEST_COMMANDS.md, "The bot behind move") and which can be lost (the user, 2026-11-19:
  // "Screen saver should only be for games with a losing condition"): a game its bot never loses would hold one game on the screen
  // for good. So Pixelus (puzzles, nothing to lose), Mummy Maze (being caught only brings "try again", and its bot is a perfect
  // solver) and both Mahjong Escapes (no board is ever lost) are not here.
  var BOTS = ["alchemy", "astropop", "atomica", "bejeweled", "bejeweled2", "bejeweledtwist", "bigmoney", "bookworm", "bookwormadv",
    "bookwormadv2", "cosmicbugs", "dynomite", "hammerheads", "heavyweapon", "ningpo", "noahsark", "rocketmania", "sevenseas", "tiptop",
    "typershark", "venice", "waterbugs", "wordharmony"];
  // How the saver starts each game (the user, 2026-11-17: "Screensavers should choose between different modes if applicable to the
  // game", "Let the bot unlock them", and a watchable pace). Each game's modes are those its bot can play, that can be lost, and that
  // a new profile has open: a locked one opens only by the bot's own play, never by a test command. One is drawn at each start.
  //   begin  the names `goto` starts them by (web/testkit.js, ?begin=), for a game whose bot has no move on its title or menus
  //   prep   the test commands that make the bot pick one, for a game whose bot clicks through its menus itself (`set mode`)
  //   setup  test commands sent at every start before those, for a first run's screen no bot answers (Venice's name box,
  //          whose work its `player` command does)
  //   choose lists of test commands, one drawn from each, for choices any player has from the start (Ning Po's title arrows:
  //          its layout, tile set and table; the user: "only plays one setup")
  //   views  2D or 3D drawn too (the `3d on`/`3d off` command, as the Options dialog's box)
  //   pace   updates waited after each move (a hundred are about a second), about a person's speed; none for a game whose bot plays
  //          in real time (aiming, dodging), where a pause would only lose
  // A game left out has no modes to draw and plays as its bot starts it (?begin=mode). The modes come from each port's `goto`,
  // `set mode` and state (docs/TEST_COMMANDS.md and each game's notes).
  function counted(command, n) { var out = []; for (var i = 0; i < n; i++) out.push(command + i); return out; }
  var PLAY = {
    alchemy: { begin: ["strategic", "timetrial"], pace: 100 },
    astropop: { begin: ["classic"], views: true },
    atomica: { pace: 100 },
    bejeweled: { begin: ["normal", "timed"], pace: 120 },
    bejeweled2: { begin: ["classic", "action"], views: true, pace: 120 },
    bejeweledtwist: { begin: ["classic", "blitz"], views: true, pace: 120 },
    bigmoney: { pace: 80 },
    bookworm: { pace: 150 },
    bookwormadv: { begin: ["adventure"], pace: 150 },
    bookwormadv2: { begin: ["adventure"], pace: 150 },
    cosmicbugs: { begin: ["game"] },
    dynomite: { begin: ["stomped", "endless", "fossil", "timetrial"] },
    hammerheads: { begin: ["classic"] },
    heavyweapon: { begin: ["campaign"] },
    ningpo: { choose: [counted("set layout ", 18), counted("set tileset ", 9), counted("set background ", 10)], pace: 120 },
    noahsark: { prep: ["set mode action", "set mode strategy"], pace: 60 },
    tiptop: { prep: ["set mode regular", "set mode strategy", "set mode speed"] },
    typershark: { begin: ["adventure"], pace: 12 },
    venice: { begin: ["journey"], setup: ["player Saver"], pace: 80 },
    wordharmony: { begin: ["tsunami"], pace: 150 }
  };
  function pick(list) { return list[Math.floor(Math.random() * list.length)]; }
  var WAITS = ["1", "2", "3", "5", "10", "15", "20", "30", "60"];
  var ids = ["none", "random"].concat(BOTS);
  var PROFILE = "-saver";
  // deletes the saver's profile of the game whose database is persist (web/shell.html, PERSIST); a frame still open on it holds the
  // deletion back until it closes, and a frame opened after it gets the profile afresh
  function wipe(persist) {
    return new Promise(function (resolve) {
      var r;
      try { r = indexedDB.deleteDatabase(persist + PROFILE); } catch (e) { resolve(); return; }
      r.onsuccess = r.onerror = r.onblocked = function () { resolve(); };
    });
  }
  function dev() { try { return ByopopTerms.devHost(location); } catch (e) { return false; } }
  // the games offered: on a development host, those with a bot whose files are here; elsewhere none yet
  function offered() {
    if (!dev()) return [];
    return BOTS.filter(function (id) { try { return ByopopGameFiles.present(ByopopGameFiles.all()[id]); } catch (e) { return false; } });
  }
  // "random" (the user, 2026-11-19: "Add an option for the screensaver to pick a random game each time and switch games upon losing
  // (thus actually *saving* the screen from burn-in)"): a game drawn from those offered, and another drawn once it is lost or over.
  // It is offered once there are two games to draw from.
  function valid(v) { var o = offered(); return v === "random" ? o.length > 1 : o.indexOf(v) >= 0; }
  function chosen() { try { var v = ByopopSettings.get("saver"); return valid(v) ? v : "none"; } catch (e) { return "none"; } }
  function waitMs() { var m = 5; try { m = parseInt(ByopopSettings.get("saverWait"), 10) || 5; } catch (e) {} return m * 60000; }
  function tileOf(id) { return document.querySelector('a.tile[data-game="' + String(id).replace(/[^a-z0-9_-]/g, "") + '"]'); }
  function pageOf(id) { var t = tileOf(id); return t ? t.getAttribute("href") : id + ".html"; }
  function persistOf(id) { var t = tileOf(id); return t ? t.getAttribute("data-persist") : null; }
  // the frame's page for a game: the driver's mode, commands and pace drawn from PLAY
  function frameSrc(id) {
    var play = PLAY[id] || {}, prep = (play.setup || []).slice(), q = "?player=1&silent=1&saver=1&driver=move&steps=100000";
    q += "&begin=" + encodeURIComponent(play.begin ? pick(play.begin) : "mode");
    (play.choose || []).forEach(function (list) { prep.push(pick(list)); });
    if (play.prep) prep.push(pick(play.prep));
    if (play.views) prep.push(pick(["3d on", "3d off"]));
    if (prep.length) q += "&prep=" + encodeURIComponent(prep.join(";"));
    if (play.pace) q += "&pace=" + play.pace;
    return new URL(pageOf(id), location.href).pathname + q;
  }

  // playing: the game in the frame; shuffled: drawn for "random"
  var shade = null, frame = null, timer = null, startedAt = 0, playing = null, shuffled = false;
  // the games whose profile a run that went wrong has wiped while this page is open: each is wiped once. A second such run is left
  // on the screen it stopped at; with "random" the game is left out of the draw instead, until this page is opened again.
  var wiped = {}, broken = {};
  // a game for "random": any offered but the one just played (and those broken), or null when there is none
  function draw(not) {
    var o = offered().filter(function (id) { return id !== not && !broken[id]; });
    return o.length ? o[Math.floor(Math.random() * o.length)] : null;
  }
  function running() { return !!shade; }
  function start(choice) {
    choice = choice || chosen();
    if (shade || choice === "none" || !valid(choice)) return false;
    shuffled = choice === "random";
    playing = shuffled ? draw(null) : choice;
    if (!playing) return false;
    shade = document.createElement("div");
    shade.className = "saver-shade";
    frame = document.createElement("iframe");
    frame.className = "saver-frame";
    frame.setAttribute("aria-hidden", "true");
    frame.tabIndex = -1;
    frame.src = frameSrc(playing);
    shade.tabIndex = -1;
    shade.appendChild(frame);
    document.body.appendChild(shade);
    shade.focus();
    // the game's page takes the focus for its canvas, and with it the keys: a key in the frame ends the saver too (same origin)
    frame.addEventListener("load", function () {
      try { frame.contentWindow.addEventListener("keydown", activity, true); } catch (e) {}
    });
    startedAt = Date.now();
    // a game that ended (the driver's result) starts again; a run that made no move would only end the same way, so it is left
    // on the screen it stopped at rather than reloaded every two seconds
    watch();
    return true;
  }
  var watchTimer = null;
  function watch() {
    clearTimeout(watchTimer);
    watchTimer = setTimeout(function () {
      if (!frame) return;
      try {
        var t = frame.contentWindow.byopop && frame.contentWindow.byopop.test;
        if (t && t.result !== undefined) {
          if (!(t.result && t.result.moves > 0)) {
            // a run that went wrong (an error, or no move at all): the game's profile is wiped and the game started afresh, once
            var persist = persistOf(playing);
            if (!persist || wiped[playing]) {
              if (!shuffled) return;
              broken[playing] = true;
              if (!(playing = draw(playing))) return;
              frame.src = frameSrc(playing);
              watch();
              return;
            }
            wiped[playing] = true;
            var src = frame.src, f = frame;
            f.src = "about:blank";
            wipe(persist).then(function () { if (frame === f) { f.src = src; watch(); } });
            return;
          }
          // a game lost or over: the same again, or with "random" another
          if (shuffled) playing = draw(playing) || playing;
          frame.src = frameSrc(playing);
        }
      } catch (e) {}
      watch();
    }, 2000);
  }
  function stop() {
    clearTimeout(watchTimer);
    if (!shade) return;
    if (shade.parentNode) shade.parentNode.removeChild(shade);
    shade = frame = playing = null;
    shuffled = false;
  }
  // input ends a running saver (a movement within the first moment is the one that may have opened Preview) and restarts the wait
  function activity(ev) {
    // the driver's moves in the saver's frame are events it makes (web/testkit.js), never the player's: taken for input, a bot's
    // keys ended the saver as soon as a level began (the user, 2026-11-19: "Mummy Maze screensaver seems to just exit once it gets
    // into a level; ditto with Noah's Ark")
    if (ev && ev.isTrusted === false) return;
    if (shade) {
      if (Date.now() - startedAt < 600 && ev.type === "pointermove") return;
      stop();
      if (ev.type === "keydown" || ev.type === "pointerdown") { ev.preventDefault(); ev.stopPropagation(); }
    }
    arm();
  }
  // A game playing in the desktop's window in front holds it off too, with no input at all: one being watched, or played by
  // typing alone (the user, 2026-11-17: "Screensaver activated while I was playing Typer Shark")
  function gameInFront() {
    try {
      var f = document.querySelector(".d95-win.d95-active:not(.d95-min):not(.d95-closed) .d95-app-frame"), w = f && f.contentWindow;
      return !!(w && w.ByopopEmbed && w.ByopopEmbed.running && w.ByopopEmbed.running());
    } catch (e) { return false; }
  }
  function arm() {
    clearTimeout(timer);
    if (chosen() === "none") return;
    timer = setTimeout(function () { if (!document.hidden && !gameInFront()) start(); else arm(); }, waitMs());
  }
  // Input in the desktop's windows is the player's as well: a game's or a page's frame (web/deskapps.js) keeps its keys, presses
  // and movements to itself, so each frame's page is heard as it loads (same origin), the saver's own frame aside
  var INPUT = ["pointermove", "pointerdown", "keydown", "wheel", "touchstart"];
  function hearFrame(f) {
    if (!f || f === frame || f.tagName !== "IFRAME") return;
    try {
      var w = f.contentWindow;
      if (!w || w.byopopSaverHeard) return;
      w.byopopSaverHeard = true;
      INPUT.forEach(function (n) { w.addEventListener(n, activity, true); });
    } catch (e) {}
  }
  if (typeof window !== "undefined" && document.getElementById("games-title")) {
    INPUT.forEach(function (n) { window.addEventListener(n, activity, true); });
    document.addEventListener("load", function (ev) { hearFrame(ev.target); }, true);
    Array.prototype.forEach.call(document.querySelectorAll("iframe"), hearFrame);
    window.addEventListener("byopop-setting", function (ev) { var n = ev.detail && ev.detail.name; if (n === "saver" || n === "saverWait") arm(); });
    arm();
  }
  return { ids: ids, waits: WAITS, PROFILE: PROFILE, PLAY: PLAY, frameSrc: frameSrc, wipe: wipe, offered: offered, chosen: chosen, start: start, stop: stop, running: running,
    preview: function (id) { stop(); return start(id); } };
})();
