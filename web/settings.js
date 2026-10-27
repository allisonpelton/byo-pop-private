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
//
// Every setting can also be set for one game: the field "games" of the same item holds an object from a game's id to
// the settings of that game ({"games": {"bejeweled": {"mute": "on"}}}). A game's own value wins over the site-wide one, and
// a game without one has the site-wide value: get(name, gameId) is the value the game uses. The listing page's Settings
// section is site-wide; the game pages' Settings panel edits either (its "All games" / "This game" choice).
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
    pauseHidden: { values: SWITCH, def: "off" },    // on: a hidden tab pauses the game
    profile: { values: ["game", "fill", "create"], def: "game" }  // fill: names start with the site's player; create: and the player is made and chosen
  };

  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function plain(o) { return !!o && typeof o === "object" && !Array.isArray(o); }
  // the stored object (also any fields this file does not know, which are kept when it writes), or {}
  function readAll() {
    try {
      var s = store();
      var o = s ? JSON.parse(s.getItem(KEY) || "{}") : {};
      return plain(o) ? o : {};
    } catch (e) { return {}; }
  }
  // value if it is one of the setting's, else null (a level: a whole number 0..100, or "game")
  function valid(name, value) {
    var d = DEFS.hasOwnProperty(name) ? DEFS[name] : null;
    if (!d) return null;
    if (d.values) return d.values.indexOf(value) >= 0 ? value : null;
    if (value === "game") return value;
    var n = (typeof value === "string" && value.trim() !== "") ? Number(value) : value;
    return (typeof n === "number" && isFinite(n) && n >= 0 && n <= 100) ? Math.round(n) : null;
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
  // is running keeps what it started with (the panel in a game's page says so). volume: the game's own volume option,
  // which the games with an engine of their own do not take unless their port says so (render's option volumes).
  var GAME_OR = [["game", "As the game is"]];
  var FIELDS = [
    { group: "Players" },
    { name: "profile", label: "Player names", options: [["game", "As the game does"], ["fill", "Fill in the name"], ["create", "Use the player"]], start: true,
      hint: "The player is chosen on the games page. “Fill in” starts a game’s name boxes (a new player, a high score) " +
        "with the player’s name and creates nothing. “Use the player” also makes the player’s profile in a game " +
        "that has profiles, if it has none of that name, and selects it when the game starts, so the game does not ask." },
    { group: "Sound" },
    { name: "mute", label: "Start muted", options: GAME_OR.concat([["on", "Muted"]]), start: true,
      hint: "A game’s page has a Sound button that turns the sound back on." },
    { name: "musicVolume", label: "Music volume", level: true, volume: true,
      hint: "0 to 100; empty leaves the game’s own. Sets the game’s own music volume option." },
    { name: "sfxVolume", label: "Sound volume", level: true, volume: true,
      hint: "0 to 100; empty leaves the game’s own. Sets the game’s own sound volume option." },
    { group: "Display" },
    { name: "fullscreen", label: "Full screen at start", options: GAME_OR.concat([["on", "Full screen"]]), start: true,
      hint: "Asked for when you press Play, as browsers allow it only after a click." },
    { name: "cursors", label: "Custom cursors", options: GAME_OR.concat([["on", "On"], ["off", "Off"]]),
      hint: "Sets the game’s own custom cursors option, for the games that have one." },
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
    ".bps-ctl select, .bps-ctl input[type=number] { font: inherit; color: var(--fg);\n" +
    "  background: var(--bg); border: 1px solid var(--line); border-radius: 8px; padding: 5px 8px; min-width: 0; max-width: 100%; }\n" +
    ".bps-ctl select { max-width: min(14em, 100%); }\n" +
    ".bps-ctl input[type=number] { width: 6.5em; }\n" +
    ".bps-ctl input[type=checkbox] { width: 18px; height: 18px; accent-color: var(--accent); margin: 0; }\n" +
    ".bps-ctl select:focus-visible, .bps-ctl input:focus-visible, .bps-scope input:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }\n" +
    ".bps-row [disabled] { opacity: 0.5; }\n" +
    ".bps-msg { min-height: 1.5em; }\n" +
    ".bps-msg.bps-error { color: #b3261e; }\n" +
    "@media (prefers-color-scheme: dark) { .bps-msg.bps-error { color: #ff8a80; } }\n" +
    "@media (max-width: 520px) { .bps-row { grid-template-columns: minmax(0, 1fr); } .bps-ctl { grid-column: 1; grid-row: auto; justify-content: flex-start; } }\n";
  var idCount = 0;

  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text) e.textContent = text;
    return e;
  }
  // the words for a value of a field ("muted", "60", "the game's own")
  function words(f, value) {
    if (f.options) {
      for (var i = 0; i < f.options.length; i++) if (f.options[i][0] === value) return lowerFirst(f.options[i][1]);
      return String(value);
    }
    if (f.level) return value === "game" ? "the game’s own" : String(value);
    return value === "on" ? "on" : "off";
  }
  function lowerFirst(t) { return /^[A-Z][a-z]/.test(t) ? t.charAt(0).toLowerCase() + t.slice(1) : t; }

  // Builds the settings into container (and replaces what it held). The controls save as they change.
  //   live        the container is in a game's page: a setting that only applies when a game starts says so
  //   volumes     false: this game takes no volume setting (it has an engine of its own): those controls are off
  //   gameId      the container is in this game's page (or edits this game): it gets the choice between "All games" and
  //               "This game", and in "This game" every control starts with "Same as all games" and the value it inherits
  //   gameName    the game's name for that choice
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
    container.appendChild(make("p", "bps-about", "Kept in this browser and used by every game. “As the game is” " +
      "leaves a setting to the game, which then plays as the original does." +
      (opts.live ? "" : " A game applies the settings when it starts. Each game’s own page can also set them for that game alone.")));
    var controls = {};
    var msg = make("p", "bps-msg");
    msg.setAttribute("role", "status");
    function say(text, isError) { msg.textContent = text; msg.className = "bps-msg" + (isError ? " bps-error" : ""); }

    var scopeBox = null, scopeNote = null;
    if (gameId) {
      var fs = make("fieldset", "bps-scope");
      fs.appendChild(make("legend", "", "Change the settings for"));
      var scopeName = "bps-scope-" + (++idCount);
      [["all", "All games"], ["game", "This game" + (opts.gameName ? " (" + opts.gameName + ")" : "")]].forEach(function (s) {
        var label = make("label"), radio = make("input");
        radio.type = "radio"; radio.name = scopeName; radio.value = s[0]; radio.checked = s[0] === "all";
        radio.addEventListener("change", function () { if (radio.checked) { scope = s[0]; refresh(); } });
        label.appendChild(radio);
        label.appendChild(document.createTextNode(s[1]));
        fs.appendChild(label);
      });
      scopeBox = fs;
      container.appendChild(fs);
      scopeNote = make("p", "bps-hint");
      container.appendChild(scopeNote);
    }

    // The effective value of a setting under the scope being edited, and what each control shows
    function show(name) {
      var c = controls[name], f = c.field;
      var own = gameId ? override(name, gameId) : null, inherited = globalValue(name);
      if (scope === "game") {
        c.select.hidden = false;
        if (c.checkbox) c.checkbox.hidden = true;
        // the first option says what "same as all games" means now
        c.select.options[0].textContent = "Same as all games (" + words(f, inherited) + ")";
        c.select.options[0].hidden = c.select.options[0].disabled = false;
        if (f.level) {
          c.select.value = own === null ? "inherit" : (own === "game" ? "game" : "level");
          c.number.hidden = typeof own !== "number";
          c.number.value = typeof own === "number" ? own : "";
        } else {
          c.select.value = own === null ? "inherit" : own;
        }
      } else {
        if (f.options) {
          c.select.hidden = false;
          c.select.options[0].hidden = c.select.options[0].disabled = true;  // there is nothing to inherit from
          c.select.value = inherited;
          if (c.select.value !== inherited) c.select.value = "game";  // "off" for a setting with only two choices
        } else {
          if (c.select) c.select.hidden = true;
          if (f.level) { c.number.hidden = false; c.number.value = inherited === "game" ? "" : inherited; }
          else { c.checkbox.hidden = false; c.checkbox.checked = inherited === "on"; }
        }
      }
      // in the all-games view, say that this game has a value of its own
      if (c.own) c.own.textContent = (scope === "all" && own !== null) ? "This game has its own: " + words(f, own) + "." : "";
    }
    function applyChange(name, stored) {
      var ok = (scope === "game") ? set(name, stored, gameId) : set(name, stored);
      if (ok) {
        say("Saved.", false);
        if (opts.onChange) { try { opts.onChange(name, get(name, gameId || undefined)); } catch (e) {} }
      } else {
        say("This browser would not keep the setting (a level is 0 to 100, or its storage is blocked).", true);
      }
      show(name);  // what is kept: a level is rounded, a bad one is not stored
    }
    FIELDS.forEach(function (f) {
      if (f.group) { container.appendChild(make("h3", "bps-group", f.group)); return; }
      var row = make("div", "bps-row"), id = "bps-" + (++idCount) + "-" + f.name;
      var label = make("label", "", f.label);
      label.htmlFor = id;
      var hint = make("p", "bps-hint", f.hint);
      var c = { field: f, select: null, number: null, checkbox: null, own: null };
      var ctl = make("div", "bps-ctl");
      // a select: a choice, and in "This game" every setting (its first option is "Same as all games")
      c.select = make("select");
      c.select.id = id;
      var inherit = make("option"); inherit.value = "inherit";
      c.select.appendChild(inherit);
      if (f.options) {
        f.options.forEach(function (o) { var op = make("option", "", o[1]); op.value = o[0]; c.select.appendChild(op); });
      } else if (f.level) {
        [["game", "As the game is"], ["level", "A level:"]].forEach(function (o) { var op = make("option", "", o[1]); op.value = o[0]; c.select.appendChild(op); });
      } else {
        [["on", "On"], ["off", "Off"]].forEach(function (o) { var op = make("option", "", o[1]); op.value = o[0]; c.select.appendChild(op); });
      }
      c.select.setAttribute("data-setting", f.name);
      ctl.appendChild(c.select);
      if (f.level) {
        c.number = make("input");
        c.number.type = "number"; c.number.min = "0"; c.number.max = "100"; c.number.step = "1";
        c.number.placeholder = "game’s own";
        c.number.setAttribute("aria-label", f.label + " (0 to 100)");
        c.number.setAttribute("data-setting", f.name);
        ctl.appendChild(c.number);
      } else if (!f.options) {
        c.checkbox = make("input");
        c.checkbox.type = "checkbox";
        c.checkbox.setAttribute("aria-label", f.label);
        c.checkbox.setAttribute("data-setting", f.name);
        ctl.appendChild(c.checkbox);
      }
      // the control that carries the label's id: the one that is visible (the select is hidden when a checkbox or a number
      // takes its place), so the label always names a control the player can reach
      if (!f.options) { c.select.removeAttribute("id"); (c.number || c.checkbox).id = id; c.select.setAttribute("aria-label", f.label); }
      if (f.volume && opts.volumes === false) {
        ctl.querySelectorAll("select,input").forEach(function (e) { e.disabled = true; });
        hint.textContent = "This game does not take this setting yet.";
      } else if (f.start && opts.live) {
        hint.appendChild(make("span", "bps-start", "Applies the next time the game starts."));
      }
      if (gameId) { c.own = make("span", "bps-own"); hint.appendChild(c.own); }
      controls[f.name] = c;
      c.select.addEventListener("change", function () {
        var v = c.select.value;
        if (scope === "game") {
          if (v === "inherit") applyChange(f.name, null);
          else if (f.level && v === "level") {
            var g = globalValue(f.name);
            applyChange(f.name, typeof g === "number" ? g : 100);
          } else applyChange(f.name, v);
        } else applyChange(f.name, v);
      });
      if (c.number) c.number.addEventListener("change", function () {
        applyChange(f.name, c.number.value === "" ? "game" : c.number.value);
      });
      if (c.checkbox) c.checkbox.addEventListener("change", function () {
        applyChange(f.name, c.checkbox.checked ? "on" : "off");
      });
      row.appendChild(label); row.appendChild(ctl); row.appendChild(hint);
      container.appendChild(row);
      if (f.name === "profile" && opts.playerNote) {
        var note = make("p", "bps-hint");
        note.setAttribute("data-player-note", "1");
        note.appendChild(make("span", "bps-note-text"));
        note.appendChild(document.createTextNode(" "));
        var link = make("a", "", "Manage players");
        link.href = "./#players";
        link.target = "_blank";
        link.rel = "noopener";
        note.appendChild(link);
        row.appendChild(note);
        c.note = note;
      }
    });
    container.appendChild(msg);
    function refresh() {
      Object.keys(controls).forEach(show);
      if (scopeNote) {
        var n = overrides(gameId).length;
        scopeNote.textContent = scope === "game"
          ? "These values are for this game alone. “Same as all games” follows the site-wide value."
          : (n ? "This game has its own value for " + n + " setting" + (n > 1 ? "s" : "") + "; they are marked below." : "");
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
  // Whether a record says the files were ever loaded in this browser, whether they are there now or were forgotten or lost
  // since. The listing page hides a game's buy links for such a game (docs/WEB_PLAYER.md, "The listing page").
  function everLoaded(r) { return !!(r && r.loaded); }
  return { KEY: KEY, all: all, mark: mark, forget: forget, lost: lost, present: present, everLoaded: everLoaded, setIcon: setIcon };
})();
