// The site's settings: what the player sets once and every game page applies (docs/WEB_PLAYER.md, "Site settings").
// The listing page's Settings section (web/index.html) and the Settings panel of every game page (web/shell.html) both load
// this file: it holds the key, the names and the defaults, and it builds the controls (ByopopSettings.render), so the two
// places are one copy of the code and edit the same stored settings. They are site-wide overrides of the games' own options.
//
// They are one localStorage item, "byopop-settings", a JSON object with one field per setting. A setting that is not
// in it has its default, and its default leaves everything as the game has it ("Game default"), so a player who never
// opens the settings sees the games exactly as they are. Storage may be blocked (a private window, the site's data refused):
// every read and write is guarded, a read then gives the defaults and a write says it did not happen.
//
// A choice setting is "game" (Game default, the default), "on" or "off". A level setting is a whole number from 0 to 100. A
// switch is "off" by default: a page feature the originals do not have, or the starting volume below. The one exception is
// showChat, a switch that is "on" by default: the site's chat (web/chat.js) is part of the page, not of any game, and the switch
// only hides it.
//
// The starting volume is three settings: the switch startVolume and the two levels startMusic and startSound (50 until the
// player moves them). With the switch on, every game starts at those two volumes (docs/WEB_PLAYER.md). Until 2026-10-27 the
// volume was "mute" (a choice) and "musicVolume" and "sfxVolume" (a level or "game"); a stored item that still has them is
// migrated when it is read (migrate): mute "on" becomes the switch on with both levels 0, volumes become the switch on with
// those levels (50 for the one that was left to the game), and the old fields are removed, in the item and in each game's own
// values.
//
// Every setting can also be set for one game: the field "games" of the same item holds an object from a game's id to
// the settings of that game ({"games": {"bejeweled": {"fullscreen": "off"}}}). A game's own value wins over the site-wide one, and
// a game without one has the site-wide value: get(name, gameId) is the value the game uses. The listing page's Settings
// section is site-wide; the game pages' Settings panel edits either (its "All games (site-wide)" / "This game only" choice).
var ByopopSettings = (function () {
  "use strict";
  var KEY = "byopop-settings";
  var CHOICE = ["game", "on", "off"], SWITCH = ["off", "on"];
  // name -> its values (a level: null) and its default
  var DEFS = {
    fullscreen: { values: CHOICE, def: "game" },    // on: Play asks for full screen; off: the game stays windowed
    cursors: { values: CHOICE, def: "game" },       // on or off: the game's custom cursors, whatever it saved
    accel3d: { values: CHOICE, def: "game" },       // on: 3D whenever the browser has WebGL2; off: never (ACCEL3D_GAMES)
    startVolume: { values: SWITCH, def: "off" },    // on: every game starts at the two volumes below
    startMusic: { values: null, def: 50 },          // a level: the music volume a game starts at
    startSound: { values: null, def: 50 },          // a level: the sound volume a game starts at
    wakeLock: { values: SWITCH, def: "off" },       // on: the screen stays awake while a game runs
    confirmLeave: { values: SWITCH, def: "off" },   // on: ask before leaving the page while a game runs
    pauseHidden: { values: SWITCH, def: "off" },    // on: a hidden tab pauses the game
    quitHome: { values: SWITCH, def: "off" },       // on: a game that quits goes back to the listing page, not its "Thanks for playing"
    autoStart: { values: SWITCH, def: "on" },       // on: a game starts when its page opens, if the browser allows sound
    showChat: { values: SWITCH, def: "on" },        // off: the chat beside the game is not drawn (web/chat.js)
    chatPlace: { values: ["edge", "game"], def: "edge" },  // game: the chat beside the game, the two centred; edge: at the page's right edge
    pixelPerfect: { values: SWITCH, def: "off" },   // on: the game takes its own size, one game pixel a whole number of device pixels
    profile: { values: ["game", "fill", "create"], def: "game" }  // fill: names start with the site's player; create: and the player is made and chosen
  };

  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function plain(o) { return !!o && typeof o === "object" && !Array.isArray(o); }
  function has(o, k) { return Object.prototype.hasOwnProperty.call(o, k); }
  // a whole number 0..100 (a number or a string of one, rounded), else null
  function level(value) {
    var n = (typeof value === "string" && value.trim() !== "") ? Number(value) : value;
    return (typeof n === "number" && isFinite(n) && n >= 0 && n <= 100) ? Math.round(n) : null;
  }

  // The fields of the old volume settings (mute, musicVolume, sfxVolume) become the starting volume (see the top). base is the
  // site-wide object's old values, which a game's own object falls back on for what it lacks. Returns true when it changed o.
  var OLD_KEYS = ["mute", "musicVolume", "sfxVolume"];
  function hasOld(o) { return plain(o) && OLD_KEYS.some(function (k) { return has(o, k); }); }
  function migrate(o) {
    var games = plain(o.games) ? o.games : {};
    var ids = Object.keys(games).filter(function (id) { return hasOld(games[id]); });
    if (!hasOld(o) && !ids.length) return false;
    var base = { mute: o.mute, music: o.musicVolume, sfx: o.sfxVolume };
    function convert(t, isGame) {
      var mute = has(t, "mute") ? t.mute : base.mute;
      var music = level(has(t, "musicVolume") ? t.musicVolume : base.music);
      var sfx = level(has(t, "sfxVolume") ? t.sfxVolume : base.sfx);
      delete t.mute; delete t.musicVolume; delete t.sfxVolume;
      if (mute === "on") { t.startVolume = "on"; t.startMusic = 0; t.startSound = 0; }
      else if (music !== null || sfx !== null) {
        t.startVolume = "on";
        t.startMusic = music === null ? DEFS.startMusic.def : music;
        t.startSound = sfx === null ? DEFS.startSound.def : sfx;
      } else if (isGame && o.startVolume === "on") t.startVolume = "off";  // this game was left to itself while the others were set
    }
    convert(o, false);
    ids.forEach(function (id) { convert(games[id], true); });
    return true;
  }

  // the stored object (also any fields this file does not know, which are kept when it writes), or {}; an old one is
  // migrated and written back
  function readAll() {
    try {
      var s = store();
      var o = s ? JSON.parse(s.getItem(KEY) || "{}") : {};
      if (!plain(o)) return {};
      if (migrate(o)) write(o);
      return o;
    } catch (e) { return {}; }
  }
  // value if it is one of the setting's, else null (a level: a whole number 0..100)
  function valid(name, value) {
    var d = DEFS.hasOwnProperty(name) ? DEFS[name] : null;
    if (!d) return null;
    if (d.values) return d.values.indexOf(value) >= 0 ? value : null;
    return level(value);
  }
  function validId(gameId) { return typeof gameId === "string" && /^[a-z0-9_-]{1,40}$/.test(gameId); }
  // This game's own value of a setting, or null when it has none (the site-wide value then applies)
  function override(name, gameId) {
    if (!validId(gameId)) return null;
    var g = readAll().games;
    return (plain(g) && plain(g[gameId])) ? valid(name, g[gameId][name]) : null;
  }
  // The setting's site-wide value: what is stored if it is one of the setting's values, else its default
  function globalValue(name) {
    var v = valid(name, readAll()[name]);
    return v === null ? (DEFS.hasOwnProperty(name) ? DEFS[name].def : null) : v;
  }
  // The value a game uses: its own if it has one, else the site-wide value. Without a game, the site-wide value.
  function get(name, gameId) {
    var v = gameId === undefined ? null : override(name, gameId);
    return v === null ? globalValue(name) : v;
  }
  function write(o) {
    try {
      var s = store();
      if (!s) return false;
      if (plain(o.games)) {
        Object.keys(o.games).forEach(function (id) {
          if (!plain(o.games[id]) || !Object.keys(o.games[id]).length) delete o.games[id];
        });
      }
      if (!plain(o.games) || !Object.keys(o.games).length) delete o.games;
      if (Object.keys(o).length) s.setItem(KEY, JSON.stringify(o)); else s.removeItem(KEY);
      return true;
    } catch (e) { return false; }
  }
  // Stores a value; the site-wide default is stored as no field. Returns false when it was not written (no such setting, a
  // bad value, or storage blocked). With a game's id the value is that game's own and is stored even when it is the
  // default, since it then overrides the site-wide one; a value of null removes the game's own value.
  function set(name, value, gameId) {
    if (!DEFS.hasOwnProperty(name)) return false;
    var o = readAll();
    if (gameId !== undefined) {
      if (!validId(gameId)) return false;
      if (!plain(o.games)) o.games = {};
      if (!plain(o.games[gameId])) o.games[gameId] = {};
      if (value === null) delete o.games[gameId][name];
      else {
        var gv = valid(name, value);
        if (gv === null) return false;
        o.games[gameId][name] = gv;
      }
      return write(o);
    }
    var v = valid(name, value);
    if (v === null) return false;
    if (v === DEFS[name].def) delete o[name]; else o[name] = v;
    return write(o);
  }
  // every setting's value, by name (the game's, with an id)
  function all(gameId) {
    var o = {};
    Object.keys(DEFS).forEach(function (n) { o[n] = get(n, gameId); });
    return o;
  }
  // the names of the settings this game has a value of its own for
  function overrides(gameId) {
    return Object.keys(DEFS).filter(function (n) { return override(n, gameId) !== null; });
  }

  // ---------- the controls ----------
  // What the player sees, in the order it is shown. start: the setting is applied when a game starts, and a game that
  // is running keeps what it started with (the panel in a game's page says so). kind "volume": the starting volume's three
  // settings in one row (a switch, with a "?" that explains it, and two sliders).
  var T = ByopopStrings.t;
  var GAME_DEFAULT = T("settings.game_default");
  // The games that read "3D acceleration" (Byopop::SiteAccel3D, framework/WebSettings.h): a game page of another game does
  // not show it. Perhaps later, once their passes settle their 3D modes: Bookworm Adventures 1 and 2, Rocket Mania.
  var ACCEL3D_GAMES = ["bejeweledtwist"];
  // What this browser gives the games for 3D, shown under that setting: "hardware", "software" or "none". Tested as the
  // games test it (Twist's Is3DAccelerationSupported asks a new canvas for "webgl2"), then once more with
  // failIfMajorPerformanceCaveat, which a browser refuses when its WebGL2 is drawn without the graphics card (Chrome's
  // SwiftShader with hardware acceleration off). Each context is let go at once (WEBGL_lose_context). Asked once a page.
  var accel3dSupport = null;
  function accel3d() {
    if (accel3dSupport) return accel3dSupport;
    function context(attrs) {
      try {
        var gl = document.createElement("canvas").getContext("webgl2", attrs);
        if (!gl) return false;
        var lose = gl.getExtension("WEBGL_lose_context");
        if (lose) lose.loseContext();
        return true;
      } catch (e) { return false; }
    }
    accel3dSupport = !context(undefined) ? "none" : context({ failIfMajorPerformanceCaveat: true }) ? "hardware" : "software";
    return accel3dSupport;
  }
  var FIELDS = [
    { group: T("settings.group.players") },
    { name: "profile", label: T("settings.profile.label"), options: [["game", GAME_DEFAULT], ["fill", T("settings.profile.fill")], ["create", T("settings.profile.create")]], start: true,
      hint: T("settings.profile.hint") },
    { group: T("settings.group.sound") },
    { name: "startVolume", kind: "volume", label: T("settings.startvolume.label"),
      hint: T("settings.startvolume.hint"),
      help: T("settings.startvolume.help") },
    { group: T("settings.group.display") },
    { name: "fullscreen", label: T("settings.fullscreen.label"), options: [["game", GAME_DEFAULT], ["on", T("settings.fullscreen.on")], ["off", T("settings.fullscreen.off")]], start: true,
      hint: T("settings.fullscreen.hint") },
    { name: "cursors", label: T("settings.cursors.label"), options: [["game", GAME_DEFAULT], ["on", T("settings.cursors.on")], ["off", T("settings.cursors.off")]],
      hint: T("settings.cursors.hint") },
    { name: "accel3d", label: T("settings.accel3d.label"), options: [["game", GAME_DEFAULT], ["on", T("settings.accel3d.on")], ["off", T("settings.accel3d.off")]], start: true,
      games: ACCEL3D_GAMES, hint: T("settings.accel3d.hint"), help: T("settings.accel3d.help") },
    { name: "pixelPerfect", label: T("settings.pixelperfect.label"),
      hint: T("settings.pixelperfect.hint") },
    { group: T("settings.group.page") },
    { name: "autoStart", label: T("settings.autostart.label"),
      hint: T("settings.autostart.hint") },
    { name: "wakeLock", label: T("settings.wakelock.label"),
      hint: T("settings.wakelock.hint") },
    { name: "confirmLeave", label: T("settings.confirmleave.label"),
      hint: T("settings.confirmleave.hint") },
    { name: "pauseHidden", label: T("settings.pausehidden.label"),
      hint: T("settings.pausehidden.hint") },
    { name: "quitHome", label: T("settings.quithome.label"),
      hint: T("settings.quithome.hint") },
    { name: "showChat", label: T("settings.showchat.label"),
      hint: T("settings.showchat.hint") },
    { name: "chatPlace", label: T("settings.chatplace.label"), options: [["edge", T("settings.chatplace.edge")], ["game", T("settings.chatplace.game")]],
      hint: T("settings.chatplace.hint") }
  ];
  var STYLE =
    ".bps { display: grid; gap: 8px; }\n" +
    ".bps p { margin: 0; }\n" +
    ".bps-about, .bps-hint, .bps-msg { color: var(--muted); font-size: 13px; }\n" +
    ".bps-group { margin: 12px 0 0; font-size: 12.5px; font-weight: 600; letter-spacing: 0.04em; text-transform: uppercase; color: var(--muted); }\n" +
    ".bps-scope { margin: 0; padding: 8px 14px; border: 1px solid var(--line); border-radius: 10px; display: flex; flex-wrap: wrap; gap: 4px 18px; }\n" +
    ".bps-scope legend { padding: 0 6px; font-size: 13px; font-weight: 600; }\n" +
    ".bps-scope label { display: flex; align-items: center; gap: 6px; cursor: pointer; }\n" +
    ".bps-scope input { accent-color: var(--accent); margin: 0; }\n" +
    ".bps-row { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 2px 16px; align-items: center;\n" +
    "  padding: 10px 14px; background: var(--panel-2, var(--panel)); border: 1px solid var(--line); border-radius: 10px; }\n" +
    ".bps-row label { font-weight: 600; }\n" +
    ".bps-row .bps-hint { grid-column: 1; }\n" +
    ".bps-row .bps-start { display: block; color: var(--accent); }\n" +
    ".bps-row .bps-own { display: block; color: var(--accent); }\n" +
    ".bps-hint a { color: var(--accent); font-weight: 600; }\n" +
    ".bps-ctl { grid-column: 2; grid-row: 1 / span 2; display: flex; flex-wrap: wrap; gap: 6px; align-items: center; justify-content: flex-end; min-width: 0; }\n" +
    ".bps-ctl select, .bps-level input[type=number] { font: inherit; color: var(--fg);\n" +
    "  background: var(--bg); border: 1px solid var(--line); border-radius: 8px; padding: 5px 8px; min-width: 0; max-width: 100%; }\n" +
    ".bps-ctl select { max-width: min(14em, 100%); }\n" +
    ".bps-ctl input[type=checkbox] { width: 18px; height: 18px; accent-color: var(--accent); margin: 0; }\n" +
    ".bps-ctl select:focus-visible, .bps-ctl input:focus-visible, .bps-scope input:focus-visible, .bps-level input:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }\n" +
    ".bps-row [disabled] { opacity: 0.5; }\n" +
    // the starting volume's row: the label with its "?", the explanation, and the two sliders
    ".bps-vol .bps-ctl { grid-row: 1; }\n" +
    ".bps-labline { display: flex; align-items: center; gap: 8px; min-width: 0; }\n" +
    ".bps-help { flex: none; width: 26px; height: 26px; padding: 0; border-radius: 50%; border: 1px solid var(--line);\n" +
    "  background: var(--bg); color: var(--fg); font: inherit; font-size: 13px; font-weight: 700; line-height: 1; cursor: pointer; }\n" +
    ".bps-help:hover { border-color: var(--accent); }\n" +
    ".bps-help:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }\n" +
    ".bps-help[aria-expanded=true] { background: var(--accent); border-color: var(--accent); color: var(--panel); }\n" +
    ".bps-row .bps-pop { grid-column: 1 / -1; margin: 4px 0 2px; padding: 8px 12px; background: var(--bg); color: var(--fg);\n" +
    "  border: 1px solid var(--accent); border-radius: 8px; font-size: 13px; }\n" +
    ".bps-row .bps-pop[hidden] { display: none; }\n" +
    ".bps-levels { grid-column: 1 / -1; display: grid; gap: 8px; margin: 6px 0 0; padding: 0; border: 0; min-width: 0; }\n" +
    ".bps-levels[hidden] { display: none; }\n" +
    ".bps-level { display: grid; grid-template-columns: 3.6em minmax(0, 1fr) 5.2em; gap: 10px; align-items: center; }\n" +
    ".bps-level input[type=range] { width: 100%; min-width: 0; margin: 0; accent-color: var(--accent); }\n" +
    ".bps-level input[type=number] { width: 100%; box-sizing: border-box; }\n" +
    ".bps-msg { min-height: 1.5em; }\n" +
    ".bps-msg.bps-error { color: #b3261e; }\n" +
    ".bps-msg.bps-error:empty { display: none; }\n" +
    ".bps-sr { position: absolute; width: 1px; height: 1px; margin: -1px; overflow: hidden; clip-path: inset(50%); white-space: nowrap; }\n" +
    "@media (prefers-color-scheme: dark) { :root:not([data-theme=\"light\"]) .bps-msg.bps-error { color: #ff8a80; } }\n" +
    ":root[data-theme=\"dark\"] .bps-msg.bps-error { color: #ff8a80; }\n" +
    "@media (max-width: 520px) { .bps-row { grid-template-columns: minmax(0, 1fr); } .bps-ctl, .bps-vol .bps-ctl { grid-column: 1; grid-row: auto; justify-content: flex-start; } }\n";
  var idCount = 0;

  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text) e.textContent = text;
    return e;
  }
  // the words for a value of a field ("Windowed", "On")
  function words(f, value) {
    if (f.options) {
      for (var i = 0; i < f.options.length; i++) if (f.options[i][0] === value) return f.options[i][1];
      return String(value);
    }
    return T(value === "on" ? "settings.on" : "settings.off");
  }
  // the starting volume in words: for a game (its own, else the site's), or the site-wide one
  function volumeWords(gameId) {
    var on = gameId ? get("startVolume", gameId) : globalValue("startVolume");
    if (on !== "on") return GAME_DEFAULT;
    return T("settings.volume_words", { music: gameId ? get("startMusic", gameId) : globalValue("startMusic"),
      sound: gameId ? get("startSound", gameId) : globalValue("startSound") });
  }

  // The "?" button and the explanation it shows (WCAG 1.4.13): a real button, aria-expanded and aria-controls, and the
  // explanation is also the description of the control it belongs to. It shows while the pointer is over the button or the
  // text, while the button has the focus, and after a click (a second click closes it, and so does Esc or a click elsewhere,
  // which a touch screen needs). A pointer that moves from the button to the text has a moment to get there.
  // What a click or Esc chose is one state, mode: "auto" (shown while the pointer is over it or the button has the focus),
  // "pinned" (a click: shown until a second click, Esc or a click elsewhere) or "dismissed" (closed by that second click or Esc:
  // hidden until the pointer comes back or the focus does). over and focused are where the pointer and the focus are.
  function wireHelp(button, pop, around) {
    var mode = "auto", over = false, focused = false, timer = 0;
    function sync() {
      var shown = mode === "pinned" || (mode === "auto" && (over || focused));
      pop.hidden = !shown;
      button.setAttribute("aria-expanded", shown ? "true" : "false");
    }
    function touch(ev) { return !!ev && (ev.pointerType === "touch"); }
    function enter(ev) { if (touch(ev)) return; clearTimeout(timer); over = true; if (mode === "dismissed") mode = "auto"; sync(); }
    function leave(ev) { if (touch(ev)) return; clearTimeout(timer); timer = setTimeout(function () { over = false; sync(); }, 250); }
    var enterName = window.PointerEvent ? "pointerenter" : "mouseenter", leaveName = window.PointerEvent ? "pointerleave" : "mouseleave";
    [button, pop].forEach(function (e) { e.addEventListener(enterName, enter); e.addEventListener(leaveName, leave); });
    button.addEventListener("focus", function () { focused = true; if (mode === "dismissed") mode = "auto"; sync(); });
    button.addEventListener("blur", function () { focused = false; sync(); });
    button.addEventListener("click", function () {
      mode = mode === "pinned" ? "dismissed" : "pinned";
      sync();
    });
    // Esc closes it from the button or from inside the text, and says it took the key (a panel around it stays open)
    around.addEventListener("keydown", function (ev) {
      if (ev.key !== "Escape" || pop.hidden) return;
      mode = "dismissed"; sync();
      ev.preventDefault();
      if (pop.contains(document.activeElement)) button.focus();
    });
    document.addEventListener("click", function (ev) {
      if (mode !== "pinned" || button.contains(ev.target) || pop.contains(ev.target)) return;
      mode = "auto"; sync();
    });
    sync();
  }

  // Builds the settings into container (and replaces what it held). The controls save as they change.
  //   live        the container is in a game's page: a setting that only applies when a game starts says so
  //   gameId      the container is in this game's page (or edits this game): it gets the choice between "All games (site-wide)"
  //               and "This game only", and in "This game only" every control starts with "Same as all games" and the value it
  //               inherits
  //   gameName    the game's name, for the note under that choice
  //   onChange    onChange(name, value) after a setting was saved: value is the one the game uses now (the game's page
  //               applies it to the running game)
  //   playerNote  a function that returns the line shown under the "Player names" control (the current site player)
  // Returns {refresh}, which shows the stored values again (it also runs when another tab changes them).
  function render(container, opts) {
    opts = opts || {};
    var gameId = validId(opts.gameId) ? opts.gameId : null;
    var scope = "all";  // "game": the controls edit this game's own values
    if (!document.getElementById("byopop-settings-style")) {
      var style = make("style");
      style.id = "byopop-settings-style";
      style.textContent = STYLE;
      document.head.appendChild(style);
    }
    container.textContent = "";
    container.classList.add("bps");
    container.appendChild(make("p", "bps-about", T(opts.live ? "settings.about_live" : "settings.about", { default: GAME_DEFAULT })));
    var controls = {};
    var msg = make("p", "bps-msg");
    msg.setAttribute("role", "status");
    var err = make("p", "bps-msg bps-error"), errId = "bps-err-" + (++idCount);
    err.id = errId;
    err.setAttribute("role", "alert");
    var tied = null;  // the control that the error is about: it is described by the error until the next change
    function untie() {
      if (!tied) return;
      var d = (tied.getAttribute("aria-describedby") || "").split(" ").filter(function (x) { return x && x !== errId; });
      if (d.length) tied.setAttribute("aria-describedby", d.join(" ")); else tied.removeAttribute("aria-describedby");
      tied = null;
    }
    function say(text, isError, control) {
      untie();
      if (isError) {
        msg.textContent = "";
        err.textContent = text;
        if (control) { tied = control; control.setAttribute("aria-describedby", (control.getAttribute("aria-describedby") || "") + " " + errId); }
      } else {
        err.textContent = "";
        msg.textContent = text;
      }
    }

    var scopeNote = null;
    if (gameId) {
      var fs = make("fieldset", "bps-scope");
      fs.appendChild(make("legend", "", T("settings.scope.legend")));
      var scopeName = "bps-scope-" + (++idCount);
      [["all", T("settings.scope.all")], ["game", T("settings.scope.game")]].forEach(function (s) {
        var label = make("label"), radio = make("input");
        radio.type = "radio"; radio.name = scopeName; radio.value = s[0]; radio.checked = s[0] === "all";
        radio.addEventListener("change", function () { if (radio.checked) { scope = s[0]; refresh(); } });
        label.appendChild(radio);
        label.appendChild(document.createTextNode(s[1]));
        fs.appendChild(label);
      });
      container.appendChild(fs);
      scopeNote = make("p", "bps-hint");
      container.appendChild(scopeNote);
    }

    // The effective value of a setting under the scope being edited, and what each control shows
    function show(name) {
      var c = controls[name], f = c.field;
      if (c.kind === "volume") { showVolume(c); return; }
      var own = gameId ? override(name, gameId) : null, inherited = globalValue(name);
      if (scope === "game") {
        c.select.hidden = false;
        if (c.checkbox) c.checkbox.hidden = true;
        // the first option says what "same as all games" means now
        c.select.options[0].textContent = T("settings.inherit", { value: words(f, inherited) });
        c.select.options[0].hidden = c.select.options[0].disabled = false;
        c.select.value = own === null ? "inherit" : own;
      } else if (f.options) {
        c.select.hidden = false;
        c.select.options[0].hidden = c.select.options[0].disabled = true;  // there is nothing to inherit from
        c.select.value = inherited;
        if (c.select.value !== inherited) c.select.value = "game";
      } else {
        c.select.hidden = true;
        c.checkbox.hidden = false;
        c.checkbox.checked = inherited === "on";
      }
      // in the all-games view, say that this game has a value of its own
      if (c.own) c.own.textContent = (scope === "all" && own !== null) ? T("settings.own", { value: words(f, own) }) : "";
    }
    // the starting volume: a switch in the all-games view, a choice in this game's own; the sliders show with it on
    function showVolume(c) {
      var own = gameId ? override("startVolume", gameId) : null, inherited = globalValue("startVolume");
      var gameScope = scope === "game", on, music, sound;
      if (gameScope) {
        c.select.hidden = false; c.checkbox.hidden = true;
        c.select.options[0].textContent = T("settings.inherit", { value: volumeWords(null) });
        c.select.value = own === null ? "inherit" : own;
        c.label.htmlFor = c.select.id;
        on = own === "on";
        music = get("startMusic", gameId); sound = get("startSound", gameId);
      } else {
        c.select.hidden = true; c.checkbox.hidden = false;
        c.checkbox.checked = inherited === "on";
        c.label.htmlFor = c.checkbox.id;
        on = inherited === "on";
        music = globalValue("startMusic"); sound = globalValue("startSound");
      }
      c.group.hidden = !on;
      c.levels.startMusic.range.value = c.levels.startMusic.number.value = music;
      c.levels.startSound.range.value = c.levels.startSound.number.value = sound;
      if (c.own) c.own.textContent = (scope === "all" && own !== null) ? T("settings.own", { value: volumeWords(gameId) }) : "";
    }
    // the control the player used for a setting: the one that is showing
    function usedControl(c) {
      return [c.checkbox, c.select].filter(function (e) { return e && !e.hidden; })[0] || c.select;
    }
    function ownerOf(name) { return (name === "startMusic" || name === "startSound") ? "startVolume" : name; }
    // after a write: say so, tell the page, and show what is kept (a level is rounded, a bad one is not stored)
    function saved(name, ok, control) {
      if (ok) {
        say(T("settings.saved"), false);
        if (opts.onChange) { try { opts.onChange(name, get(name, gameId || undefined)); } catch (e) {} }
      } else {
        say(T("settings.save_error"), true, control);
      }
      show(ownerOf(name));
    }
    function applyChange(name, stored, control) {
      saved(name, (scope === "game") ? set(name, stored, gameId) : set(name, stored), control);
    }

    function buildVolume(f) {
      var row = make("div", "bps-row bps-vol"), id = "bps-" + (++idCount) + "-" + f.name;
      var c = { field: f, kind: "volume", select: null, checkbox: null, own: null, levels: {}, group: null, label: null };
      var hint = make("p", "bps-hint", f.hint);
      hint.id = id + "-hint";
      var popId = id + "-help", pop = make("p", "bps-pop", f.help);
      pop.id = popId;
      pop.hidden = true;
      var describedBy = hint.id + " " + popId;
      c.label = make("label", "", f.label);
      var help = make("button", "bps-help", "?");
      help.type = "button";
      help.setAttribute("aria-label", T("settings.help_label", { label: f.label }));
      help.setAttribute("aria-expanded", "false");
      help.setAttribute("aria-controls", popId);
      var line = make("div", "bps-labline");
      line.appendChild(c.label);
      line.appendChild(help);
      var ctl = make("div", "bps-ctl");
      c.checkbox = make("input");
      c.checkbox.type = "checkbox";
      c.checkbox.id = id + "-box";
      c.checkbox.setAttribute("aria-label", f.label);
      c.select = make("select");
      c.select.id = id + "-select";
      c.select.setAttribute("aria-label", f.label);
      [["inherit", ""], ["off", GAME_DEFAULT], ["on", f.label]].forEach(function (o) {
        var op = make("option", "", o[1]);
        op.value = o[0];
        c.select.appendChild(op);
      });
      [c.checkbox, c.select].forEach(function (e) { e.setAttribute("aria-describedby", describedBy); e.setAttribute("data-setting", f.name); });
      ctl.appendChild(c.checkbox);
      ctl.appendChild(c.select);
      // the two sliders, each with the number it stands for (the number can also be typed)
      c.group = make("fieldset", "bps-levels");
      c.group.appendChild(make("legend", "bps-sr", T("settings.vol.legend")));
      [["startMusic", T("settings.vol.music")], ["startSound", T("settings.vol.sound")]].forEach(function (L) {
        var lid = id + "-" + L[0], item = make("div", "bps-level");
        var lab = make("label", "", L[1]);
        lab.htmlFor = lid;
        var range = make("input");
        range.type = "range"; range.min = "0"; range.max = "100"; range.step = "1"; range.id = lid;
        var num = make("input");
        num.type = "number"; num.min = "0"; num.max = "100"; num.step = "1";
        num.setAttribute("aria-label", T("settings.vol.level", { name: L[1] }));
        [range, num].forEach(function (e) { e.setAttribute("aria-describedby", hint.id); e.setAttribute("data-setting", L[0]); });
        // dragging only shows the number; the level is kept (and given to a running game) when the slider is let go, or a key moves it
        range.addEventListener("input", function () { num.value = range.value; });
        range.addEventListener("change", function () { applyChange(L[0], range.value, range); });
        num.addEventListener("change", function () { applyChange(L[0], num.value, num); });
        item.appendChild(lab); item.appendChild(range); item.appendChild(num);
        c.group.appendChild(item);
        c.levels[L[0]] = { range: range, number: num };
      });
      if (gameId) { c.own = make("span", "bps-own"); hint.appendChild(c.own); }
      c.checkbox.addEventListener("change", function () {
        applyChange("startVolume", c.checkbox.checked ? "on" : "off", c.checkbox);
      });
      // in this game's own view the switch is a choice: following the site, leaving the game as it is, or its own volumes
      // (which start at what it follows now)
      c.select.addEventListener("change", function () {
        var v = c.select.value;
        if (v === "on") {
          set("startMusic", globalValue("startMusic"), gameId);
          set("startSound", globalValue("startSound"), gameId);
        } else {
          set("startMusic", null, gameId);
          set("startSound", null, gameId);
        }
        saved("startVolume", set("startVolume", v === "inherit" ? null : v, gameId), c.select);
      });
      row.appendChild(line); row.appendChild(ctl); row.appendChild(pop); row.appendChild(hint); row.appendChild(c.group);
      wireHelp(help, pop, row);
      controls[f.name] = c;
      container.appendChild(row);
    }

    FIELDS.forEach(function (f) {
      if (f.group) { container.appendChild(make("h3", "bps-group", f.group)); return; }
      if (f.kind === "volume") { buildVolume(f); return; }
      // a setting only some games read is left out of the other games' pages
      if (f.games && gameId && f.games.indexOf(gameId) < 0) return;
      var row = make("div", "bps-row"), id = "bps-" + (++idCount) + "-" + f.name;
      var label = make("label", "", f.label);
      label.htmlFor = id;
      var hint = make("p", "bps-hint", f.hint);
      hint.id = id + "-hint";
      // the "?" button and its explanation, as the starting volume has them
      var help = null, pop = null;
      if (f.help) {
        pop = make("p", "bps-pop", f.help);
        pop.id = id + "-help";
        pop.hidden = true;
        help = make("button", "bps-help", "?");
        help.type = "button";
        help.setAttribute("aria-label", T("settings.help_label", { label: f.label }));
        help.setAttribute("aria-expanded", "false");
        help.setAttribute("aria-controls", pop.id);
      }
      var c = { field: f, select: null, checkbox: null, own: null };
      var ctl = make("div", "bps-ctl");
      // a select: a choice, and in "This game only" every setting (its first option is "Same as all games")
      c.select = make("select");
      c.select.id = id;
      var inherit = make("option"); inherit.value = "inherit";
      c.select.appendChild(inherit);
      if (f.options) {
        f.options.forEach(function (o) { var op = make("option", "", o[1]); op.value = o[0]; c.select.appendChild(op); });
      } else {
        [["on", T("settings.on")], ["off", T("settings.off")]].forEach(function (o) { var op = make("option", "", o[1]); op.value = o[0]; c.select.appendChild(op); });
      }
      c.select.setAttribute("data-setting", f.name);
      ctl.appendChild(c.select);
      if (!f.options) {
        c.checkbox = make("input");
        c.checkbox.type = "checkbox";
        c.checkbox.setAttribute("aria-label", f.label);
        c.checkbox.setAttribute("data-setting", f.name);
        ctl.appendChild(c.checkbox);
        // the control that carries the label's id: the one that is visible (the select is hidden in the all-games view,
        // where the checkbox takes its place), so the label always names a control the player can reach
        c.select.removeAttribute("id"); c.checkbox.id = id; c.select.setAttribute("aria-label", f.label);
      }
      if (f.start && opts.live) hint.appendChild(make("span", "bps-start", T("settings.applies_next")));
      if (gameId) { c.own = make("span", "bps-own"); hint.appendChild(c.own); }
      controls[f.name] = c;
      [c.select, c.checkbox].forEach(function (e) { if (e) e.setAttribute("aria-describedby", hint.id + (pop ? " " + pop.id : "")); });
      c.select.addEventListener("change", function () {
        var v = c.select.value;
        applyChange(f.name, (scope === "game" && v === "inherit") ? null : v, c.select);
      });
      if (c.checkbox) c.checkbox.addEventListener("change", function () {
        applyChange(f.name, c.checkbox.checked ? "on" : "off", c.checkbox);
      });
      if (help) {
        var line = make("div", "bps-labline");
        line.appendChild(label);
        line.appendChild(help);
        row.appendChild(line); row.appendChild(ctl); row.appendChild(pop); row.appendChild(hint);
        wireHelp(help, pop, row);
      } else {
        row.appendChild(label); row.appendChild(ctl); row.appendChild(hint);
      }
      if (f.name === "accel3d") {
        var support = make("p", "bps-hint", T("settings.accel3d.support." + accel3d()));
        support.id = id + "-support";
        support.setAttribute("data-accel3d-support", accel3d());
        row.appendChild(support);
        c.select.setAttribute("aria-describedby", c.select.getAttribute("aria-describedby") + " " + support.id);
      }
      container.appendChild(row);
      if (f.name === "profile" && opts.playerNote) {
        var note = make("p", "bps-hint");
        note.setAttribute("data-player-note", "1");
        note.appendChild(make("span", "bps-note-text"));
        note.appendChild(document.createTextNode(" "));
        var link = make("a", "", T("settings.manage_players"));
        link.href = "./#players";
        link.target = "_blank";
        link.rel = "noopener";
        link.appendChild(make("span", "bps-sr", " " + T("site.new_tab")));
        note.appendChild(link);
        row.appendChild(note);
        c.note = note;
      }
    });
    container.appendChild(msg);
    container.appendChild(err);
    // how many settings this game has a value of its own for (the starting volume is one)
    function ownCount() {
      return overrides(gameId).filter(function (n) { return n !== "startMusic" && n !== "startSound"; }).length;
    }
    function refresh() {
      Object.keys(controls).forEach(show);
      if (scopeNote) {
        var n = ownCount();
        scopeNote.textContent = scope === "game"
          ? T("settings.scope.game_note", { name: opts.gameName || T("settings.scope.this_game") })
          : (n ? T("settings.scope.own_note", { n: n }) : "");
      }
      if (opts.playerNote && controls.profile && controls.profile.note) {
        try { controls.profile.note.firstChild.textContent = opts.playerNote() || ""; } catch (e) {}
      }
    }
    refresh();
    // another tab changed them (the listing page, while a game runs here)
    window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null || ev.key === "byopop-profiles") refresh(); });
    return { refresh: refresh };
  }

  return { KEY: KEY, names: Object.keys(DEFS), get: get, set: set, all: all, override: override, overrides: overrides, render: render };
})();

// Which games have their files loaded in this browser, as a sign that the player owns them (docs/WEB_PLAYER.md, "The
// listing page"): one localStorage item for the whole site, "byopop-game-files", a JSON object from a game's id to
// {loaded, forgotten, missing}, dates as YYYY-MM-DD. loaded: when the game's files were last found. forgotten: when the
// player used "Forget game files" on the game's page. missing: when a game page found the files gone without that (the
// browser's data was cleared, or its storage freed). The files are there when there is a loaded date and neither of the
// others; finding them again (loading them) clears both. icon: the game's own icon, read out of its exe by
// web/exeicon.js, as a PNG data URL of at most 16 KB; it is derived from the files, so it goes with them (when they are
// forgotten or gone) and is kept only while they are there. A game page writes it, the listing page reads it, without
// starting any game, and an entry is never removed: that a game was played here is part of what the listing says. Every
// read and write is guarded: with storage blocked, nothing is recorded.
var ByopopGameFiles = (function () {
  "use strict";
  var KEY = "byopop-game-files";
  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function today() { return new Date().toISOString().slice(0, 10); }
  // id -> {loaded, forgotten, missing}; {} when there is none or storage is blocked
  function all() {
    var out = {};
    try {
      var s = store();
      var o = s ? JSON.parse(s.getItem(KEY) || "{}") : {};
      if (o && typeof o === "object" && !Array.isArray(o)) {
        Object.keys(o).forEach(function (id) {
          if (o[id] && typeof o[id] === "object" && !Array.isArray(o[id])) out[id] = o[id];
        });
      }
    } catch (e) {}
    return out;
  }
  function write(o) {
    try {
      var s = store();
      if (s) s.setItem(KEY, JSON.stringify(o));
    } catch (e) {}
  }
  // The files are there (found at a start, or just imported)
  function mark(id) {
    var o = all(), r = o[id];
    if (r && r.loaded === today() && !r.forgotten && !r.missing) return;
    var icon = present(r) ? r.icon : "";  // the same files as before: their icon stays
    o[id] = { loaded: today() };
    if (icon) o[id].icon = icon;
    write(o);
  }
  // The player forgot the files ("Forget game files"): kept with the date, and when they were last loaded
  function forget(id) {
    var o = all(), r = o[id] || {};
    r.forgotten = today();
    delete r.missing;
    delete r.icon;
    o[id] = r;
    write(o);
    try { if (window.ByopopCursors) ByopopCursors.forget(id); } catch (e) {}  // its cursors came from its files too
  }
  // The files are gone, and not by "Forget game files": noted for a game that had a record
  function lost(id) {
    var o = all(), r = o[id];
    if (!r || r.forgotten || r.missing) return;
    r.missing = today();
    delete r.icon;
    write(o);
  }
  // An import has started replacing the files (the old copy is gone, the new one not all there yet): noted as missing
  // whatever the record held, a first add and a re-add after "Forget game files" too, until mark() records the new copy. A
  // copy cut off partway (the tab closed) is then not taken for the game by the listing's files check.
  function replacing(id) {
    var o = all(), r = o[id] || {};
    r.missing = today();
    delete r.icon;
    o[id] = r;
    write(o);
  }
  // The icon read out of the game's exe (a PNG data URL, at most 16 KB), kept with the entry while the files are there
  function setIcon(id, url) {
    if (typeof url !== "string" || url.length > 16384 || url.indexOf("data:image/png;base64,") !== 0) return false;
    var o = all(), r = o[id];
    if (!present(r)) return false;
    r.icon = url;
    write(o);
    return true;
  }
  // Whether a record says the files are there
  function present(r) { return !!(r && r.loaded && !r.forgotten && !r.missing); }
  // Whether a record says the files were ever loaded in this browser, whether they are there now or were forgotten or lost
  // since. The listing page hides a game's buy links for such a game (docs/WEB_PLAYER.md, "The listing page").
  function everLoaded(r) { return !!(r && r.loaded); }
  return { KEY: KEY, all: all, mark: mark, forget: forget, lost: lost, replacing: replacing, present: present, everLoaded: everLoaded, setIcon: setIcon };
})();

// How much each game was played in this browser (docs/WEB_PLAYER.md, "The listing page"): one localStorage item for the whole
// site, "byopop-play", a JSON object from a game's id to {plays, seconds, last}. plays: how many times the game was started (a
// restart of the game's own mod menu goes on with the same play). seconds: the time the game ran with its page shown and in the
// focus, not paused, hidden or behind another window. last: the last day it was played, the player's own date as YYYY-MM-DD. A game
// page writes it, the listing page shows it on the game's card, and the save backup keeps it (web/saves.js). Every read and write
// is guarded: with storage blocked, nothing is counted.
var ByopopPlay = (function () {
  "use strict";
  var KEY = "byopop-play";
  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function today() {
    var d = new Date();
    return d.getFullYear() + "-" + ("0" + (d.getMonth() + 1)).slice(-2) + "-" + ("0" + d.getDate()).slice(-2);
  }
  function count(n) { return (typeof n === "number" && isFinite(n) && n > 0) ? Math.floor(n) : 0; }
  // id -> {plays, seconds, last}; {} when there is none or storage is blocked
  function all() {
    var out = {};
    try {
      var s = store();
      var o = s ? JSON.parse(s.getItem(KEY) || "{}") : {};
      if (o && typeof o === "object" && !Array.isArray(o)) {
        Object.keys(o).forEach(function (id) {
          var r = o[id];
          if (!r || typeof r !== "object" || Array.isArray(r)) return;
          out[id] = { plays: count(r.plays), seconds: count(r.seconds),
            last: (typeof r.last === "string" && /^\d{4}-\d{2}-\d{2}$/.test(r.last)) ? r.last : "" };
        });
      }
    } catch (e) {}
    return out;
  }
  function change(id, fn) {
    if (!/^[a-z0-9_-]{1,40}$/.test(id)) return false;
    try {
      var s = store();
      if (!s) return false;
      var o = all();
      o[id] = o[id] || { plays: 0, seconds: 0, last: "" };
      fn(o[id]);
      o[id].last = today();
      s.setItem(KEY, JSON.stringify(o));
      return true;
    } catch (e) { return false; }
  }
  // The game was started
  function started(id) { return change(id, function (r) { r.plays++; }); }
  // ms more of play
  function played(id, ms) {
    var add = Math.round(ms / 1000);
    return add > 0 ? change(id, function (r) { r.seconds += add; }) : false;
  }
  // "3 h 20 min", "12 min", "under a minute"
  function duration(seconds) {
    var m = Math.floor(count(seconds) / 60), h = Math.floor(m / 60);
    if (h > 0) return m % 60 ? ByopopStrings.t("time.hours_minutes", { h: h, m: m % 60 }) : ByopopStrings.t("time.hours", { h: h });
    return m > 0 ? ByopopStrings.t("time.minutes", { m: m }) : ByopopStrings.t("time.under_minute");
  }
  return { KEY: KEY, all: all, started: started, played: played, duration: duration, today: today };
})();
