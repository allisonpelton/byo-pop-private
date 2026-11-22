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
  // the games whose port answers `move` (docs/TEST_COMMANDS.md, "The bot behind move") and whose state names the mode that
  // ?begin=mode starts (Pixelus's since dff4920a: before it named none, its frame stayed on the title; the user, 2026-11-13)
  var BOTS = ["alchemy", "astropop", "atomica", "bejeweled", "bejeweled2", "bejeweledtwist", "bigmoney", "bookworm", "bookwormadv",
    "bookwormadv2", "cosmicbugs", "dynomite", "mummymaze", "ningpo", "noahsark", "pixelus", "rocketmania", "sevenseas", "tiptop",
    "typershark", "waterbugs", "wordharmony"];
  var WAITS = ["1", "2", "3", "5", "10", "15", "20", "30", "60"];
  var ids = ["none"].concat(BOTS);
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
  function chosen() { try { var v = ByopopSettings.get("saver"); return offered().indexOf(v) >= 0 ? v : "none"; } catch (e) { return "none"; } }
  function waitMs() { var m = 5; try { m = parseInt(ByopopSettings.get("saverWait"), 10) || 5; } catch (e) {} return m * 60000; }
  function tileOf(id) { return document.querySelector('a.tile[data-game="' + String(id).replace(/[^a-z0-9_-]/g, "") + '"]'); }
  function pageOf(id) { var t = tileOf(id); return t ? t.getAttribute("href") : id + ".html"; }
  function persistOf(id) { var t = tileOf(id); return t ? t.getAttribute("data-persist") : null; }

  var shade = null, frame = null, timer = null, startedAt = 0, playing = null;
  // the games whose profile a run that went wrong has wiped while this page is open: each is wiped once, and a second such run is
  // left on the screen it stopped at
  var wiped = {};
  function running() { return !!shade; }
  function start(id) {
    id = id || chosen();
    if (shade || id === "none" || offered().indexOf(id) < 0) return false;
    playing = id;
    shade = document.createElement("div");
    shade.className = "saver-shade";
    frame = document.createElement("iframe");
    frame.className = "saver-frame";
    frame.setAttribute("aria-hidden", "true");
    frame.tabIndex = -1;
    frame.src = new URL(pageOf(id), location.href).pathname + "?player=1&silent=1&saver=1&driver=move&begin=mode&steps=100000";
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
            if (!persist || wiped[playing]) return;
            wiped[playing] = true;
            var src = frame.src, f = frame;
            f.src = "about:blank";
            wipe(persist).then(function () { if (frame === f) { f.src = src; watch(); } });
            return;
          }
          frame.src = frame.src;
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
  }
  // input ends a running saver (a movement within the first moment is the one that may have opened Preview) and restarts the wait
  function activity(ev) {
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
  return { ids: ids, waits: WAITS, PROFILE: PROFILE, wipe: wipe, offered: offered, chosen: chosen, start: start, stop: stop, running: running,
    preview: function (id) { stop(); return start(id); } };
})();
