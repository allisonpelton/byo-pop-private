// The sound panel (docs/WEB_PLAYER.md, "The sound panel"; the user, 2026-11-13: "Add a toggleable menu that appears beside
// the game like the perf panel that shows what sounds are being played in the game and at what volume and pitch"). It is
// the site setting "soundPanel" (web/settings.js), off by default, shown and hidden from the game window's View menu, and it
// only reads: the game plays the same with it on.
//
// With it off nothing of it runs: no timer, and the mixer keeps no log (byopop_sounds_watch(0)). With it on, ten times a
// second it asks the game's mixer (platform/Audio.cpp, Mixer::WebVoicesView) for the voices playing and the ones started
// since the last look, so a sound that ends between two looks still shows. Each row is one voice: the sound's file, its
// volume and pan as the game set them, and its pitch as a rate and in semitones; a row stays for two seconds after its
// voice ends, greyed. A game built before the call existed says so.
var ByopopSoundPanel = (function () {
  "use strict";
  var KEY = "byopop-settings";
  var on = false, panel = null, body = null, head = null, pageGame = null, timer = 0, since = 0, rows = {}, order = 0;
  var HOLD = 2000, MAX_ROWS = 24;

  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  function setting() {
    try {
      var o = JSON.parse(window.localStorage.getItem(KEY) || "{}"), g = pageGame && o.games && o.games[pageGame];
      if (g && (g.soundPanel === "on" || g.soundPanel === "off")) return g.soundPanel;
      return o.soundPanel === "on" ? "on" : "off";
    } catch (e) { return "off"; }
  }
  function mod() {
    var M = window.Module;
    return M && typeof M._byopop_sounds_view === "function" && typeof M.ccall === "function" ? M : null;
  }

  // ---------- reading ----------
  function look() {
    var M = mod();
    if (!M) { body.textContent = T(window.Module && window.Module.calledRun ? "sounds.panel.old" : "sounds.panel.waiting"); return; }
    var v = null;
    try {
      M.ccall("byopop_sounds_watch", null, ["number"], [1]);
      v = JSON.parse(M.ccall("byopop_sounds_view", "string", ["number"], [since]) || "null");
    } catch (e) { v = null; }
    if (!v) return;
    var now = performance.now();
    since = v.n;
    // the voices started since the last look: each a row, whether it still plays or not
    (v.started || []).forEach(function (s) {
      rows[s.id + ":" + s.n] = { id: s.id, name: s.name, l: s.l, r: s.r, pitch: s.pitch, loop: s.loop, len: s.len, at: 0, live: false, seen: now, order: ++order };
    });
    // the voices playing now: their row (the newest one with their id), its gains and pitch as they are now
    var live = {};
    (v.voices || []).forEach(function (p) {
      var best = null;
      Object.keys(rows).forEach(function (k) { var r = rows[k]; if (r.id === p.id && (!best || r.order > best.order)) best = r; });
      if (!best) { best = rows[p.id + ":old"] = { id: p.id, name: p.name, len: p.len, order: ++order }; }
      best.l = p.l; best.r = p.r; best.pitch = p.pitch; best.loop = p.loop; best.at = p.at; best.live = true; best.seen = now;
      live[best.order] = true;
    });
    Object.keys(rows).forEach(function (k) {
      var r = rows[k];
      if (!live[r.order]) r.live = false;
      if (!r.live && now - r.seen > HOLD) delete rows[k];
    });
    draw(v.sfx, now);
  }

  // ---------- drawing ----------
  function file(name) {
    var n = String(name || "").replace(/\\/g, "/");
    n = n.slice(n.lastIndexOf("/") + 1).replace(/\.(ogg|wav|au|mp3)$/i, "");
    return n || T("sounds.panel.unnamed");
  }
  function pan(l, r) {
    var s = l + r;
    if (!(s > 0)) return "–";
    var p = Math.round((r - l) / s * 100);
    return p === 0 ? T("sounds.panel.centre") : (p < 0 ? "L" : "R") + Math.abs(p);
  }
  function pitch(p) {
    if (!(p > 0)) return "–";
    var semis = 12 * Math.log(p) / Math.LN2, s = Math.abs(semis) < 0.05 ? "0" : (semis > 0 ? "+" : "−") + Math.abs(semis).toFixed(1);
    return "×" + p.toFixed(2) + " (" + s + ")";
  }
  function draw(sfx, now) {
    head.textContent = T("sounds.panel.effects", { volume: Math.round((sfx || 0) * 100) + "%" });
    var list = Object.keys(rows).map(function (k) { return rows[k]; }).sort(function (a, b) { return b.order - a.order; }).slice(0, MAX_ROWS);
    if (!list.length) { body.textContent = T("sounds.panel.none"); return; }
    var t = document.createElement("table"), tr = document.createElement("tr");
    ["sounds.panel.sound", "sounds.panel.volume", "sounds.panel.pan", "sounds.panel.pitch"].forEach(function (k) {
      var th = document.createElement("th"); th.textContent = T(k); tr.appendChild(th);
    });
    t.appendChild(tr);
    list.forEach(function (r) {
      var row = document.createElement("tr");
      if (!r.live) row.className = "snd-ended";
      var cells = [(r.live ? "▶ " : "") + file(r.name) + (r.loop ? " ↻" : ""), Math.round(Math.max(r.l, r.r) * 100) + "%", pan(r.l, r.r), pitch(r.pitch)];
      cells.forEach(function (c, i) { var td = document.createElement("td"); td.textContent = c; if (i === 0) td.title = r.name || ""; row.appendChild(td); });
      t.appendChild(row);
    });
    var was = body.offsetHeight;
    body.textContent = "";
    body.appendChild(t);
    if (panel.classList.contains("snd-side") && body.offsetHeight > was) try { window.dispatchEvent(new CustomEvent("byopop-side")); } catch (e) {}
  }
  var STYLE =
    ".snd-panel { position: fixed; right: 12px; top: 60px; z-index: 9; width: 300px; max-height: calc(calc(100vh / var(--d95-z, 1)) - 80px); overflow: auto; box-sizing: border-box;\n" +
    "  background: var(--panel); color: var(--fg); border: 1px solid var(--line); border-radius: 8px; box-shadow: 0 6px 24px rgba(0,0,0,.25); font-size: 12px; }\n" +
    ".snd-panel h2 { margin: 0; padding: 6px 10px; font-size: 13px; border-bottom: 1px solid var(--line); }\n" +
    ".snd-panel p { margin: 6px 10px 0; color: var(--muted); }\n" +
    ".snd-panel > div { margin: 6px 10px 10px; }\n" +
    ".snd-panel table { border-collapse: collapse; width: 100%; font-variant-numeric: tabular-nums; }\n" +
    ".snd-panel th { text-align: left; font-weight: 700; padding: 0 4px 2px 0; }\n" +
    ".snd-panel td { padding: 0 4px 0 0; white-space: nowrap; font-family: ui-monospace, Consolas, monospace; }\n" +
    ".snd-panel td:first-child { max-width: 110px; overflow: hidden; text-overflow: ellipsis; }\n" +
    ".snd-panel tr.snd-ended td { color: var(--muted); }\n" +
    "html.player-only .snd-panel { display: none !important; }\n" +
    // beside the game, at the right of the performance panel when that is shown too (web/perf.js takes the column before)
    ".snd-panel.snd-side { position: static; width: 270px; max-height: none; border-radius: 0; box-shadow: none; border-width: 0 0 0 1px;\n" +
    "  grid-column: 3; grid-row: 1 / 3; align-self: stretch; overflow: auto; }\n" +
    "html.snd-side .stage-wrap { grid-template-columns: minmax(0, 1fr) auto auto; }\n" +
    "html.d95-desk:not(.d95-embed).snd-side #main { display: grid; grid-template-columns: auto auto auto; grid-template-rows: auto auto 1fr auto; }\n" +
    "html.d95-desk:not(.d95-embed).snd-side #main > .d95-tb, html.d95-desk:not(.d95-embed).snd-side #main > .d95-statusbar { grid-column: 1 / -1; }\n" +
    "html.d95-desk:not(.d95-embed).snd-side #main > .stage { grid-column: 1; grid-row: 2; }\n" +
    "html.d95-desk:not(.d95-embed).snd-side #main > .d95-statusbar { grid-row: 4; }\n" +
    "html.d95-desk:not(.d95-embed).snd-side #main > .snd-panel { grid-column: 3; grid-row: 2 / 4; }\n" +
    "@media (max-width: 600px) {\n" +
    "  .snd-panel.snd-side { grid-column: 1; grid-row: auto; width: auto; max-height: calc(40vh / var(--d95-z, 1)); border-width: 1px 0 0; }\n" +
    "  html.snd-side .stage-wrap { grid-template-columns: minmax(0, 1fr); }\n" +
    "}\n" +
    "html.snd-side.touch-side .stage-wrap { grid-template-columns: minmax(0, 1fr) auto auto auto; }\n" +
    "html.snd-side.touch-side .stage-wrap > .bc-touch { grid-column: 4; }\n";
  function build() {
    if (panel) return;
    var style = document.createElement("style");
    style.textContent = STYLE;
    document.head.appendChild(style);
    panel = document.createElement("aside");
    panel.className = "snd-panel";
    panel.id = "snd-panel";
    panel.setAttribute("aria-labelledby", "snd-title");
    panel.hidden = true;
    var h = document.createElement("h2"); h.id = "snd-title"; h.textContent = T("sounds.panel.title");
    head = document.createElement("p");
    body = document.createElement("div");
    body.setAttribute("aria-live", "off");
    body.textContent = T("sounds.panel.waiting");
    panel.appendChild(h); panel.appendChild(head); panel.appendChild(body);
    // as the performance panel: part of the game's window, at its right, which grows by its width
    var main = document.getElementById("main"), cls = document.documentElement.classList;
    if ((cls.contains("d95-embed") || cls.contains("d95-desk")) && main) {
      panel.classList.add("snd-side");
      main.appendChild(panel);
      return;
    }
    document.body.appendChild(panel);
  }
  function start() {
    if (on) return;
    on = true;
    build();
    panel.hidden = false;
    side();
    since = 0; rows = {};
    timer = setInterval(look, 100);
  }
  function stop() {
    if (!on) return;
    on = false;
    clearInterval(timer); timer = 0;
    var M = mod();
    if (M) try { M.ccall("byopop_sounds_watch", null, ["number"], [0]); } catch (e) {}
    rows = {};
    if (panel) { panel.hidden = true; body.textContent = T("sounds.panel.waiting"); head.textContent = ""; }
    side();
  }
  function side() {
    var is = !!panel && !panel.hidden && panel.classList.contains("snd-side");
    document.documentElement.classList.toggle("snd-side", is);
    try { window.dispatchEvent(new CustomEvent("byopop-side")); } catch (e) {}
  }
  function sync() {
    if (!document.body || !document.getElementById("canvas")) return;
    if (document.documentElement.classList.contains("player-only")) return;
    if (setting() === "on") start(); else stop();
  }
  function setGame(id) { pageGame = id || null; sync(); }
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "soundPanel") sync(); });
  window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) sync(); });
  return { setGame: setGame, sync: sync, isOn: function () { return on; } };
})();
