// The screen saver (docs/WEB_PLAYER.md, "The screen saver"; the user, 2026-11-11): Settings > Screen Saver picks None or a game
// whose port has a bot, and the Wait in minutes. When the listing's desktop has had no key, mouse or touch for that long, the
// desktop goes dark and the game plays itself, silently, in a frame over it; any key or mouse movement ends it and leaves everything
// as it was. Preview shows it at once.
//
// The game plays by its bot's moves through the hook every bot gives (docs/WEB_PLAYER.md, "The screen saver"): today the port's
// test command `move` on the player-only page (?player=1&driver=move, web/testkit.js), which exists only on a development host,
// so on the live site the list holds None alone until a game's bot mod (made in its modding phase) answers the same hook there.
// The frame's page runs at ?saver=1 (web/shell.html): it reads the player's own files of the game and writes nothing back, not
// its saves, not the site's records, so a saver's games leave no trace. When a game ends the frame starts another.
var ByopopSaver = (function () {
  "use strict";
  // the games whose port answers `move` (docs/TEST_COMMANDS.md, "The bot behind move") and whose state names the mode that
  // ?begin=mode starts. Pixelus is left out until its state gives one: its title and menu name none, so its frame stayed on the
  // title (the user, 2026-11-13: "The "screensaver" just seems to blink the home screen every couple of seconds, at least set
  // to Pixelus", when a run that made no move was reloaded every two seconds)
  var BOTS = ["alchemy", "astropop", "atomica", "bejeweled", "bejeweled2", "bejeweledtwist", "bigmoney", "bookworm", "bookwormadv",
    "bookwormadv2", "cosmicbugs", "dynomite", "mummymaze", "ningpo", "noahsark", "rocketmania", "sevenseas", "tiptop",
    "typershark", "waterbugs", "wordharmony"];
  var WAITS = ["1", "2", "3", "5", "10", "15", "20", "30", "60"];
  var ids = ["none"].concat(BOTS);
  function dev() { try { return ByopopTerms.devHost(location); } catch (e) { return false; } }
  // the games offered: on a development host, those with a bot whose files are here; elsewhere none yet
  function offered() {
    if (!dev()) return [];
    return BOTS.filter(function (id) { try { return ByopopGameFiles.present(ByopopGameFiles.all()[id]); } catch (e) { return false; } });
  }
  function chosen() { try { var v = ByopopSettings.get("saver"); return offered().indexOf(v) >= 0 ? v : "none"; } catch (e) { return "none"; } }
  function waitMs() { var m = 5; try { m = parseInt(ByopopSettings.get("saverWait"), 10) || 5; } catch (e) {} return m * 60000; }
  function pageOf(id) {
    var t = document.querySelector('a.tile[data-game="' + String(id).replace(/[^a-z0-9_-]/g, "") + '"]');
    return t ? t.getAttribute("href") : id + ".html";
  }

  var shade = null, frame = null, timer = null, startedAt = 0;
  function running() { return !!shade; }
  function start(id) {
    id = id || chosen();
    if (shade || id === "none" || offered().indexOf(id) < 0) return false;
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
          if (!(t.result && t.result.moves > 0)) return;
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
    shade = frame = null;
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
  function arm() {
    clearTimeout(timer);
    if (chosen() === "none") return;
    timer = setTimeout(function () { if (!document.hidden) start(); else arm(); }, waitMs());
  }
  if (typeof window !== "undefined" && document.getElementById("games-title")) {
    ["pointermove", "pointerdown", "keydown", "wheel", "touchstart"].forEach(function (n) { window.addEventListener(n, activity, true); });
    window.addEventListener("byopop-setting", function (ev) { var n = ev.detail && ev.detail.name; if (n === "saver" || n === "saverWait") arm(); });
    arm();
  }
  return { ids: ids, waits: WAITS, offered: offered, chosen: chosen, start: start, stop: stop, running: running,
    preview: function (id) { stop(); return start(id); } };
})();
