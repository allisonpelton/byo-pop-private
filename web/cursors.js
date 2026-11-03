// The site's cursor (docs/WEB_PLAYER.md, "The site's cursor"; the user, 2026-11-03: "pull custom cursors out of the user's
// uploaded games so that they can select one to use across the whole site"). A game's page, once the game has started, asks it
// for its set of cursors (framework/WebCursor.cpp, byopop_cursor_set_copy: its pointer, hand, dragging and text images) and keeps
// them here; the player picks one game's set in the Settings (the listing page's and every game page's), and every page of the
// site then shows it. A game whose own cursor is showing keeps it (its page's game-cursor class); where the game shows none, the
// site's cursor shows over the game too, as the user asked ("If the custom cursor is disabled in a game, could the site's custom
// cursor still display over it?").
//
// Two localStorage items for the whole site: "byopop-cursors", a JSON object from a game's id to its set, {title, pointer, hand,
// drag, text}, each {url, x, y} (a PNG data URL and its hot spot) or absent; and "byopop-site-cursor", the id of the chosen set,
// absent for the browser's own cursor. The images come from the player's own copy of the game and stay in this browser;
// forgetting the game's files forgets them too (ByopopGameFiles.forget, web/settings.js).
var ByopopCursors = (function () {
  "use strict";
  var KEY = "byopop-cursors", CHOICE = "byopop-site-cursor";
  // the most the sets may take together (characters of their JSON): localStorage's quota is shared with the games' save
  // backups (sexy-userdata-backup:*), so a set that would go past it is not kept
  var MAX_TOTAL = 256 * 1024;
  var KINDS = ["pointer", "hand", "drag", "text"];
  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function plain(o) { return !!o && typeof o === "object" && !Array.isArray(o); }
  function validId(id) { return typeof id === "string" && /^[a-z0-9_-]{1,40}$/.test(id); }
  function validImage(c) {
    return plain(c) && typeof c.url === "string" && c.url.indexOf("data:image/png;base64,") === 0 && c.url.length < 200000 &&
      c.x === (c.x | 0) && c.y === (c.y | 0) && c.x >= 0 && c.y >= 0 && c.x < 512 && c.y < 512;
  }

  // the stored sets, by game id: only well-formed ones, with a pointer
  function all() {
    var out = {};
    try {
      var s = store(), o = s ? JSON.parse(s.getItem(KEY) || "{}") : {};
      if (!plain(o)) return out;
      Object.keys(o).forEach(function (id) {
        var v = o[id];
        if (!validId(id) || !plain(v) || !validImage(v.pointer)) return;
        var set = { title: typeof v.title === "string" ? v.title.slice(0, 120) : id };
        KINDS.forEach(function (k) { if (validImage(v[k])) set[k] = { url: v[k].url, x: v[k].x, y: v[k].y }; });
        out[id] = set;
      });
    } catch (e) {}
    return out;
  }
  function write(o) {
    try {
      var s = store();
      if (!s) return false;
      if (Object.keys(o).length) s.setItem(KEY, JSON.stringify(o)); else s.removeItem(KEY);
      return true;
    } catch (e) { return false; }
  }
  // Keeps a game's set ({pointer, hand, drag, text}, as all() gives them); true when it was new or changed. Not kept (false)
  // without a pointer, or when the sets would then take more than MAX_TOTAL.
  function save(id, title, set) {
    if (!validId(id) || !plain(set) || !validImage(set.pointer)) return false;
    var o = all(), entry = { title: String(title || id).slice(0, 120) };
    KINDS.forEach(function (k) { if (validImage(set[k])) entry[k] = { url: set[k].url, x: set[k].x, y: set[k].y }; });
    if (JSON.stringify(o[id]) === JSON.stringify(entry)) return false;
    o[id] = entry;
    if (JSON.stringify(o).length > MAX_TOTAL) return false;
    if (!write(o)) return false;
    apply();
    redraw();
    return true;
  }
  function forget(id) {
    var o = all();
    if (!o.hasOwnProperty(id)) return;
    delete o[id];
    write(o);
    apply();
    redraw();
  }
  // the chosen set's id, or "" for the browser's own cursor
  function chosen() {
    try { var s = store(), v = s ? s.getItem(CHOICE) : null; return validId(v) ? v : ""; } catch (e) { return ""; }
  }
  function choose(id) {
    try {
      var s = store();
      if (s) { if (id && validId(id)) s.setItem(CHOICE, id); else s.removeItem(CHOICE); }
    } catch (e) {}
    apply();
  }

  // ---------- showing it ----------
  // A cursor value: the image with its hot spot, and the browser's own of that kind behind it
  function value(c, fallback) { return c ? 'url("' + c.url + '") ' + c.x + " " + c.y + ", " + fallback : fallback; }
  function css(set) {
    var on = ":root.site-cursor:not(.game-cursor)";
    var P = value(set.pointer, "auto"), H = value(set.hand, "pointer"), D = value(set.drag || set.hand, "grabbing"), T = value(set.text, "text");
    var hand = ["a[href]", "button:not(:disabled)", "select", "summary", "label[for]", "[role=button]", "[role=tab]", "input[type=checkbox]",
      "input[type=radio]", "input[type=range]", "input[type=file]", "input[type=button]", "input[type=submit]", ".badge[draggable=true]"];
    var text = ["input:not([type])", "input[type=text]", "input[type=search]", "input[type=email]", "input[type=number]", "input[type=url]",
      "textarea", "[contenteditable=true]"];
    function each(list) { return list.map(function (x) { return on + " " + x; }).join(", "); }
    return on + ", " + on + " * { cursor: " + P + " !important; }\n" +
      each(hand) + " { cursor: " + H + " !important; }\n" +
      each(text) + " { cursor: " + T + " !important; }\n" +
      on + " .badge[draggable=true]:active { cursor: " + D + " !important; }\n" +
      // over a game that shows no cursor of its own: SDL's cursor for the canvas (its inline style) by kind, and none stays none
      on + ' canvas[style*="cursor: pointer"] { cursor: ' + H + " !important; }\n" +
      on + ' canvas[style*="cursor: text"] { cursor: ' + T + " !important; }\n" +
      on + ' canvas[style*="cursor: move"], ' + on + ' canvas[style*="cursor: grab"] { cursor: ' + D + " !important; }\n" +
      on + ' canvas[style*="cursor: none"] { cursor: none !important; }\n';
  }
  var styleEl = null;
  function apply() {
    var set = chosen() ? all()[chosen()] : null, root = document.documentElement;
    if (!set) {
      root.classList.remove("site-cursor");
      if (styleEl) styleEl.textContent = "";
      return;
    }
    if (!styleEl) {
      styleEl = document.createElement("style");
      styleEl.id = "byopop-site-cursor";
      (document.head || root).appendChild(styleEl);
    }
    styleEl.textContent = css(set);
    root.classList.add("site-cursor");
  }

  // ---------- the choice in the Settings ----------
  // A group of radio buttons: the browser's own cursor, then each game's set with its pointer and hand. Rendered into el, kept
  // up to date with the other tabs.
  var STYLE =
    ".bpc-cur { margin: 12px 0 0; padding: 0; border: 0; min-width: 0; display: grid; gap: 6px; }\n" +
    // the legend as the Settings' group headings (web/settings.js, .bps-group), the choices in one box as its rows
    ".bpc-cur legend { padding: 0; margin-bottom: 8px; font-size: 12.5px; font-weight: 600; letter-spacing: 0.04em; text-transform: uppercase; color: var(--muted, #6b6680); }\n" +
    ".bpc-cur .bpc-cur-box { display: grid; gap: 4px; padding: 10px 14px; border: 1px solid var(--line, #d9d5e3); border-radius: 10px; background: var(--panel-2, var(--panel, transparent)); }\n" +
    ".bpc-cur label { display: flex; align-items: center; gap: 8px; min-height: 32px; }\n" +
    ".bpc-cur .bpc-cur-pics { display: inline-flex; align-items: center; gap: 4px; min-width: 72px; }\n" +
    ".bpc-cur .bpc-cur-pics img { max-width: 32px; max-height: 32px; image-rendering: pixelated; }\n" +
    ".bpc-cur .bpc-cur-hint { margin: 0; color: var(--muted, #6b6680); font-size: 13px; }\n";
  var rendered = [], idCount = 0;
  function redraw() { rendered.forEach(fill); }
  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text !== undefined) e.textContent = text;
    return e;
  }
  function fill(el) {
    var sets = all(), pick = chosen(), name = "bpc-cur-" + (++idCount);
    el.textContent = "";
    var fs = make("fieldset", "bpc-cur"), box = make("div", "bpc-cur-box");
    fs.appendChild(make("legend", null, ByopopStrings.t("cursors.legend")));
    fs.appendChild(box);
    function option(id, text, set) {
      var label = make("label"), input = make("input");
      input.type = "radio"; input.name = name; input.value = id; input.checked = id === pick || (!id && !sets[pick]);
      input.addEventListener("change", function () { if (input.checked) choose(id); });
      label.appendChild(input);
      if (set) {
        var pics = make("span", "bpc-cur-pics");
        ["pointer", "hand"].forEach(function (k) {
          if (!set[k]) return;
          var img = make("img");
          img.src = set[k].url; img.alt = "";
          pics.appendChild(img);
        });
        label.appendChild(pics);
      }
      label.appendChild(document.createTextNode(text));
      box.appendChild(label);
    }
    option("", ByopopStrings.t("cursors.browser"), null);
    Object.keys(sets).sort(function (a, b) { return sets[a].title.localeCompare(sets[b].title); })
      .forEach(function (id) { option(id, sets[id].title, sets[id]); });
    box.appendChild(make("p", "bpc-cur-hint", Object.keys(sets).length
      ? ByopopStrings.t("cursors.hint")
      : ByopopStrings.t("cursors.hint_none")));
    el.appendChild(fs);
  }
  function render(el) {
    if (!el) return;
    if (!document.getElementById("bpc-cur-style")) {
      var st = make("style");
      st.id = "bpc-cur-style";
      st.textContent = STYLE;
      document.head.appendChild(st);
    }
    rendered.push(el);
    fill(el);
  }

  apply();
  window.addEventListener("storage", function (ev) {
    if (ev.key !== null && ev.key !== KEY && ev.key !== CHOICE) return;
    apply();
    redraw();
  });
  return { KEY: KEY, CHOICE: CHOICE, all: all, save: save, forget: forget, chosen: chosen, choose: choose, apply: apply, render: render, refresh: redraw };
})();
