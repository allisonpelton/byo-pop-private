// The site's settings: what the player sets once and every game page applies (docs/WEB_PLAYER.md, "Site settings").
// The listing page's Settings section (web/index.html) and the Settings panel of every game page (web/shell.html) both load
// this file: it holds the key, the names and the defaults, and it builds the controls (ByopopSettings.render), so the two
// places are one copy of the code and edit the same stored settings.
//
// They are one localStorage item, "byopop-settings", a JSON object with one field per setting. A setting that is not
// in it has its default, and its default leaves everything as the game has it, so a player who never opens the
// settings sees the games exactly as they are. Storage may be blocked (a private window, the site's data refused):
// every read and write is guarded, a read then gives the defaults and a write says it did not happen.
//
// A choice setting is "game" (leave it to the game, the default), "on" or "off". A level setting is "game" or a whole
// number from 0 to 100. A switch is a page feature the originals do not have and is "off" by default.
var ByopopSettings = (function () {
  "use strict";
  var KEY = "byopop-settings";
  var CHOICE = ["game", "on", "off"], SWITCH = ["off", "on"];
  // name -> its values (a level: null) and its default
  var DEFS = {
    mute: { values: CHOICE, def: "game" },          // on: the game starts muted
    fullscreen: { values: CHOICE, def: "game" },    // on: Play asks for full screen
    cursors: { values: CHOICE, def: "game" },       // on or off: the game's custom cursors, whatever it saved
    musicVolume: { values: null, def: "game" },     // a level: the game's music volume
    sfxVolume: { values: null, def: "game" },       // a level: the game's sound volume
    wakeLock: { values: SWITCH, def: "off" },       // on: the screen stays awake while a game runs
    confirmLeave: { values: SWITCH, def: "off" },   // on: ask before leaving the page while a game runs
    pauseHidden: { values: SWITCH, def: "off" }     // on: a hidden tab pauses the game
  };

  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  // the stored object (also any fields this file does not know, which are kept when it writes), or {}
  function readAll() {
    try {
      var s = store();
      var o = s ? JSON.parse(s.getItem(KEY) || "{}") : {};
      return (o && typeof o === "object" && !Array.isArray(o)) ? o : {};
    } catch (e) { return {}; }
  }
  // value if it is one of the setting's, else null (a level: a whole number 0..100, or "game")
  function valid(name, value) {
    var d = DEFS[name];
    if (!d) return null;
    if (d.values) return d.values.indexOf(value) >= 0 ? value : null;
    if (value === "game") return value;
    var n = (typeof value === "string" && value.trim() !== "") ? Number(value) : value;
    return (typeof n === "number" && isFinite(n) && n >= 0 && n <= 100) ? Math.round(n) : null;
  }
  // The setting's value: what is stored if it is one of the setting's values, else its default
  function get(name) {
    var v = valid(name, readAll()[name]);
    return v === null ? (DEFS[name] ? DEFS[name].def : null) : v;
  }
  // Stores a value; the default is stored as no field. Returns false when it was not written (no such setting, a bad
  // value, or storage blocked).
  function set(name, value) {
    var v = valid(name, value);
    if (v === null) return false;
    try {
      var s = store();
      if (!s) return false;
      var o = readAll();
      if (v === DEFS[name].def) delete o[name]; else o[name] = v;
      if (Object.keys(o).length) s.setItem(KEY, JSON.stringify(o)); else s.removeItem(KEY);
      return true;
    } catch (e) { return false; }
  }
  // every setting's value, by name
  function all() {
    var o = {};
    Object.keys(DEFS).forEach(function (n) { o[n] = get(n); });
    return o;
  }

  // ---------- the controls ----------
  // What the player sees, in the order it is shown. start: the setting is applied when a game starts, and a game that
  // is running keeps what it started with (the panel in a game's page says so). volume: the game's own volume option,
  // which the games with an engine of their own do not take yet (render's option volumes).
  var GAME_OR = [["game", "As the game is"]];
  var FIELDS = [
    { group: "Sound" },
    { name: "mute", label: "Start muted", options: GAME_OR.concat([["on", "Muted"]]), start: true,
      hint: "A game\u2019s page has a Sound button that turns the sound back on." },
    { name: "musicVolume", label: "Music volume", level: true, volume: true,
      hint: "0 to 100; empty leaves the game\u2019s own. Sets the game\u2019s own music volume option." },
    { name: "sfxVolume", label: "Sound volume", level: true, volume: true,
      hint: "0 to 100; empty leaves the game\u2019s own. Sets the game\u2019s own sound volume option." },
    { group: "Display" },
    { name: "fullscreen", label: "Full screen at start", options: GAME_OR.concat([["on", "Full screen"]]), start: true,
      hint: "Asked for when you press Play, as browsers allow it only after a click." },
    { name: "cursors", label: "Custom cursors", options: GAME_OR.concat([["on", "On"], ["off", "Off"]]),
      hint: "Sets the game\u2019s own custom cursors option, for the games that have one." },
    { group: "Page" },
    { name: "wakeLock", label: "Keep the screen awake while a game runs", hint: "Off by default. A browser may refuse." },
    { name: "confirmLeave", label: "Ask before leaving a page while a game runs", hint: "Off by default." },
    { name: "pauseHidden", label: "Pause when the tab is hidden",
      hint: "Off by default, as the original games keep running in a window you cannot see. When on, a hidden tab stops the game and its sound until you come back." }
  ];
  var STYLE =
    ".bps { display: grid; gap: 8px; }\n" +
    ".bps p { margin: 0; }\n" +
    ".bps-about, .bps-hint, .bps-msg { color: var(--muted); font-size: 13px; }\n" +
    ".bps-group { margin: 12px 0 0; font-size: 12.5px; font-weight: 600; letter-spacing: 0.04em; text-transform: uppercase; color: var(--muted); }\n" +
    ".bps-row { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 2px 16px; align-items: center;\n" +
    "  padding: 10px 14px; background: var(--panel-2, var(--panel)); border: 1px solid var(--line); border-radius: 10px; }\n" +
    ".bps-row label { font-weight: 600; }\n" +
    ".bps-row .bps-hint { grid-column: 1; }\n" +
    ".bps-row .bps-start { display: block; color: var(--accent); }\n" +
    ".bps-row select, .bps-row input[type=number] { grid-column: 2; grid-row: 1 / span 2; font: inherit; color: var(--fg);\n" +
    "  background: var(--bg); border: 1px solid var(--line); border-radius: 8px; padding: 5px 8px; min-width: 0; }\n" +
    ".bps-row input[type=number] { width: 7em; }\n" +
    ".bps-row input[type=checkbox] { grid-column: 2; grid-row: 1 / span 2; width: 18px; height: 18px; accent-color: var(--accent); margin: 0; }\n" +
    ".bps-row select:focus-visible, .bps-row input:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }\n" +
    ".bps-row [disabled] { opacity: 0.5; }\n" +
    ".bps-msg { min-height: 1.5em; }\n" +
    ".bps-msg.bps-error { color: #b3261e; }\n" +
    "@media (prefers-color-scheme: dark) { .bps-msg.bps-error { color: #ff8a80; } }\n";
  var idCount = 0;

  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text) e.textContent = text;
    return e;
  }

  // Builds the settings into container (and replaces what it held). The controls save as they change.
  //   live        the container is in a game's page: a setting that only applies when a game starts says so
  //   volumes     false: this game takes no volume setting (it has an engine of its own): those controls are off
  //   onChange    onChange(name, value) after a setting was saved (the game's page applies it to the running game)
  // Returns {refresh}, which shows the stored values again (it also runs when another tab changes them).
  function render(container, opts) {
    opts = opts || {};
    if (!document.getElementById("byopop-settings-style")) {
      var style = make("style");
      style.id = "byopop-settings-style";
      style.textContent = STYLE;
      document.head.appendChild(style);
    }
    container.textContent = "";
    container.classList.add("bps");
    container.appendChild(make("p", "bps-about", "Kept in this browser and used by every game. \u201cAs the game is\u201d " +
      "leaves a setting to the game, which then plays as the original does." +
      (opts.live ? "" : " A game applies the settings when it starts.")));
    var controls = {};
    var msg = make("p", "bps-msg");
    msg.setAttribute("role", "status");
    function say(text, isError) { msg.textContent = text; msg.className = "bps-msg" + (isError ? " bps-error" : ""); }

    function show(name) {
      var c = controls[name], value = get(name);
      if (!c) return;
      if (c.type === "checkbox") c.checked = value === "on";
      else if (c.type === "number") c.value = value === "game" ? "" : value;
      else { c.value = value; if (c.value !== value) c.value = "game"; }  // "off" for a setting with only two choices
    }
    FIELDS.forEach(function (f) {
      if (f.group) { container.appendChild(make("h3", "bps-group", f.group)); return; }
      var row = make("div", "bps-row"), id = "bps-" + (++idCount) + "-" + f.name;
      var label = make("label", "", f.label);
      label.htmlFor = id;
      var hint = make("p", "bps-hint", f.hint);
      var control;
      if (f.options) {
        control = make("select");
        f.options.forEach(function (o) { var op = make("option", "", o[1]); op.value = o[0]; control.appendChild(op); });
      } else if (f.level) {
        control = make("input");
        control.type = "number"; control.min = "0"; control.max = "100"; control.step = "1";
        control.placeholder = "game\u2019s own";
      } else {
        control = make("input");
        control.type = "checkbox";
      }
      control.id = id;
      control.setAttribute("data-setting", f.name);
      if (f.volume && opts.volumes === false) {
        control.disabled = true;
        hint.textContent = "This game does not take this setting yet.";
      } else if (f.volume && !opts.live) {
        hint.textContent += " TipTop, Word Harmony, Cosmic Bugs, Water Bugs and Ning Po do not take it yet.";
      } else if (f.start && opts.live) {
        hint.appendChild(make("span", "bps-start", "Applies the next time the game starts."));
      }
      controls[f.name] = control;
      control.addEventListener("change", function () {
        var v = control.type === "checkbox" ? (control.checked ? "on" : "off") : (control.value === "" ? "game" : control.value);
        if (set(f.name, v)) {
          say("Saved.", false);
          if (opts.onChange) { try { opts.onChange(f.name, get(f.name)); } catch (e) {} }
        } else {
          say("This browser would not keep the setting (a level is 0 to 100, or its storage is blocked).", true);
        }
        show(f.name);  // what is kept: a level is rounded, a bad one is not stored
      });
      row.appendChild(label); row.appendChild(control); row.appendChild(hint);
      container.appendChild(row);
    });
    container.appendChild(msg);
    function refresh() { Object.keys(controls).forEach(show); }
    refresh();
    // another tab changed them (the listing page, while a game runs here)
    window.addEventListener("storage", function (ev) { if (ev.key === KEY || ev.key === null) refresh(); });
    return { refresh: refresh };
  }

  return { KEY: KEY, names: Object.keys(DEFS), get: get, set: set, all: all, render: render };
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
  }
  // The files are gone, and not by "Forget game files": noted for a game that had a record
  function lost(id) {
    var o = all(), r = o[id];
    if (!r || r.forgotten || r.missing) return;
    r.missing = today();
    delete r.icon;
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
  return { KEY: KEY, all: all, mark: mark, forget: forget, lost: lost, present: present, setIcon: setIcon };
})();
