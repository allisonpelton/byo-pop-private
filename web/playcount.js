// The pages' side of the site's play counter (docs/PLAY_COUNTER.md; the server is web/counter/playcount.mjs). A game page opens
// one socket once its game is really playing (ByopopCounter.playing): opening it counts a play, and while it is open the page
// counts as playing now. The listing page reads every game's counts in one request (ByopopCounter.counts). Nothing about the
// player is sent: the socket's address holds the game's id and, when this browser counted a play of that game in the last
// few minutes, count=0.
//
// Off unless the build names the counter's address (BYOPOP_WEB_COUNTER in cmake/ByopopWeb.cmake, written below in place of the
// marker): every local build and test has none. Off too with ?silent=1 (the tests), under Node or another host without
// WebSocket, and in a browser driven by automation (navigator.webdriver). Nothing is sent while the page is hidden (a background
// tab, or the hidden Browser pane): the first contact waits until it is shown. Off on a development host (web/terms.js, even
// with ?terms=1) and on a page that skipped the terms card with the site's test key, so local work and tests never add to the
// counts or read them.
var ByopopCounter = (function () {
  "use strict";
  var BASE = "@BYOPOP_COUNTER@";   // the Worker's /api, "" for none
  var COOLDOWN_MS = 5 * 60 * 1000; // a play of a game counted in this browser in the last 5 minutes is not counted again
  var KEY = "byopop-counted";      // localStorage: {id: time of the last counted play}

  function on() {
    if (!BASE || BASE.charAt(0) === "@") return false;
    try {
      if (typeof window === "undefined" || typeof document === "undefined" || typeof WebSocket === "undefined") return false;
      if (new URLSearchParams(location.search).get("silent") === "1") return false;
      if (navigator.webdriver) return false;
      if (typeof ByopopTerms === "undefined" || ByopopTerms.devHost(location) || ByopopTerms.skipped()) return false;
    } catch (e) { return false; }
    return true;
  }
  function address(path, ws) {
    var u = new URL(BASE.replace(/\/+$/, "") + path, location.href);
    if (ws) u.protocol = u.protocol === "http:" ? "ws:" : "wss:";
    return u.href;
  }
  function whenShown(fn) {
    if (document.visibilityState === "visible") { fn(); return; }
    document.addEventListener("visibilitychange", function shown() {
      if (document.visibilityState !== "visible") return;
      document.removeEventListener("visibilitychange", shown);
      fn();
    });
  }
  function counted(id) {
    try {
      var o = JSON.parse(localStorage.getItem(KEY) || "{}"), now = Date.now();
      if (!o || typeof o !== "object") o = {};
      var fresh = !(typeof o[id] === "number" && now - o[id] < COOLDOWN_MS && now >= o[id]);
      Object.keys(o).forEach(function (k) { if (typeof o[k] !== "number" || now - o[k] >= COOLDOWN_MS) delete o[k]; });
      if (fresh) o[id] = now;
      localStorage.setItem(KEY, JSON.stringify(o));
      return fresh;
    } catch (e) { return true; }  // no storage: the server's own limit still holds
  }

  // ---------- a game page ----------
  // The game is really playing: open the socket (once a page). It is kept open with a "ping" every 50 s, which the server's
  // runtime answers without waking the counter, and opened again (not counted) if the connection drops while the game runs.
  // stop() closes it for good: the game ended or the page goes. A page kept in the browser's back/forward cache closes it while
  // it is away and opens it again (not counted) when it is shown again.
  var socket = null, pinger = 0, stopped = false, away = false, tries = 0, game = "";
  function playing(id) {
    if (game || !on() || !/^[a-z0-9_-]{1,40}$/.test(id)) return;
    game = id;
    whenShown(function () { connect(counted(id)); });
    window.addEventListener("pagehide", function (ev) {
      if (!ev.persisted) { stop(); return; }
      away = true;
      close();
    });
    window.addEventListener("pageshow", function (ev) {
      if (!ev.persisted || !away) return;
      away = false;
      tries = 0;
      if (!stopped) whenShown(function () { connect(false); });
    });
  }
  function connect(count) {
    if (stopped || away || socket) return;
    var ws;
    try { ws = new WebSocket(address("/play?game=" + encodeURIComponent(game) + (count ? "" : "&count=0"), true)); }
    catch (e) { return; }
    socket = ws;
    ws.onopen = function () {
      tries = 0;
      clearInterval(pinger);
      pinger = setInterval(function () { try { ws.send("ping"); } catch (e) {} }, 50000);
    };
    ws.onclose = function () {
      if (socket !== ws) return;  // one this page closed itself (close())
      clearInterval(pinger);
      socket = null;
      // a dropped connection is opened again, a few times, at growing intervals; a refused one (too many from here) too
      if (!stopped && !away && tries < 5) { tries++; setTimeout(function () { whenShown(function () { connect(false); }); }, 15000 * tries); }
    };
  }
  function close() {
    clearInterval(pinger);
    if (socket) { var s = socket; socket = null; try { s.close(1000); } catch (e) {} }
  }
  function stop() {
    stopped = true;
    close();
  }

  // ---------- the listing page ----------
  // fn({id: {plays, live}}) with every game's counts, read when the page is shown and again every 2 minutes while it is shown;
  // never called when the counter is off or does not answer
  function counts(fn) {
    var ready = (typeof ByopopTerms !== "undefined" && ByopopTerms.ready) || Promise.resolve();
    ready.then(function () { if (on()) reads(fn); });
  }
  function reads(fn) {
    var last = 0;
    function read() {
      if (document.visibilityState !== "visible" || Date.now() - last < 110000) return;
      last = Date.now();
      fetch(address("/counts", false), { credentials: "omit", cache: "no-cache" }).then(function (r) {
        return r.ok ? r.json() : null;
      }).then(function (o) {
        if (o && o.games && typeof o.games === "object") fn(clean(o.games));
      }).catch(function () {});
    }
    whenShown(read);
    document.addEventListener("visibilitychange", read);
    setInterval(read, 120000);
  }
  function clean(games) {
    var out = {};
    Object.keys(games).forEach(function (id) {
      var g = games[id];
      if (!/^[a-z0-9_-]{1,40}$/.test(id) || !g || typeof g !== "object") return;
      function n(v) { return (typeof v === "number" && isFinite(v) && v > 0) ? Math.floor(v) : 0; }
      out[id] = { plays: n(g.plays), live: n(g.live) };
    });
    return out;
  }

  return { on: on, playing: playing, stop: stop, counts: counts };
})();
