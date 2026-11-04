// The site's cursor (docs/WEB_PLAYER.md, "The site's cursor"; the user, 2026-11-03: "pull custom cursors out of the user's
// uploaded games so that they can select one to use across the whole site"). A game's page, once the game has started, asks it
// for its set of cursors (framework/WebCursor.cpp, byopop_cursor_set_copy: its image for each kind of cursor) and keeps the ones
// the site has a use for here; the player picks one game's set in the Settings (the listing page's and every game page's), and
// every page of the site then shows it, each cursor where the site means what the game means by it (the user, 2026-11-04: "make
// them contextual on the site too"): the pointer everywhere, the hand on links and controls, the text cursor in text fields, the
// dragging cursor on a cap being dragged, the circle-slash on a disabled control and the wait cursor while the site is busy
// (busy()). A kind the game has no image for falls back as before: the pointer, or the hand for dragging. A game whose own cursor is showing keeps it (its page's game-cursor class); where the game shows none, the
// site's cursor shows over the game too, as the user asked ("If the custom cursor is disabled in a game, could the site's custom
// cursor still display over it?").
//
// Two localStorage items for the whole site: "byopop-cursors", a JSON object from a game's id to its set, {title, pointer, hand,
// drag, text, notallowed, wait}, each {url, x, y} (a PNG data URL and its hot spot) or absent; and "byopop-site-cursor", the id of
// the chosen set, absent for the browser's own cursor. An animated cursor (none of the ported games has one; Banana Bugs will)
// also has frames, [{url, x, y}...] after the first, and ms, the time each shows: browsers animate no cursor image, so the page
// swaps the frames itself on the element under the mouse, and only while it shows that cursor. The images come from the player's own copy of the game and stay in this browser;
// forgetting the game's files forgets them too (ByopopGameFiles.forget, web/settings.js).
var ByopopCursors = (function () {
  "use strict";
  var KEY = "byopop-cursors", CHOICE = "byopop-site-cursor";
  // the most the sets may take together (characters of their JSON): localStorage's quota is shared with the games' save
  // backups (sexy-userdata-backup:*), so a set that would go past it is not kept
  var MAX_TOTAL = 256 * 1024;
  var KINDS = ["pointer", "hand", "drag", "text", "notallowed", "wait"];
  // A game whose image for a kind means something else (WEB_PLAYER.md, "The site's cursor", the table): it is not kept for that
  // kind. Seven Seas keeps its shovel, the cursor of its bonus on the board, where the framework keeps the circle-slash.
  var OTHER_MEANING = { sevenseas: ["notallowed"] };
  var MAX_FRAMES = 32;
  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function plain(o) { return !!o && typeof o === "object" && !Array.isArray(o); }
  function validId(id) { return typeof id === "string" && /^[a-z0-9_-]{1,40}$/.test(id); }
  function validFrame(c) {
    return plain(c) && typeof c.url === "string" && c.url.indexOf("data:image/png;base64,") === 0 && c.url.length < 200000 &&
      c.x === (c.x | 0) && c.y === (c.y | 0) && c.x >= 0 && c.y >= 0 && c.x < 512 && c.y < 512;
  }
  // a kind's image, its frames (if any) all well-formed and few, each shown for 40 ms to 2 s
  function validImage(c) {
    if (!validFrame(c)) return false;
    if (c.frames === undefined) return true;
    return Array.isArray(c.frames) && c.frames.length >= 1 && c.frames.length < MAX_FRAMES && c.frames.every(validFrame) &&
      c.ms === (c.ms | 0) && c.ms >= 40 && c.ms <= 2000;
  }
  function copyImage(c) {
    var out = { url: c.url, x: c.x, y: c.y };
    if (c.frames) {
      out.frames = c.frames.map(function (f) { return { url: f.url, x: f.x, y: f.y }; });
      out.ms = c.ms;
    }
    return out;
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
        KINDS.forEach(function (k) { if (validImage(v[k])) set[k] = copyImage(v[k]); });
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
  // Keeps a game's set ({pointer, hand, drag, text, notallowed, wait}, as all() gives them; a kind whose image means something
  // else in that game is left out); true when it was new or changed. Not kept (false) without a pointer, or when the sets would
  // then take more than MAX_TOTAL.
  function save(id, title, set) {
    if (!validId(id) || !plain(set) || !validImage(set.pointer)) return false;
    var o = all(), entry = { title: String(title || id).slice(0, 120) }, other = OTHER_MEANING[id] || [];
    KINDS.forEach(function (k) { if (validImage(set[k]) && other.indexOf(k) < 0) entry[k] = copyImage(set[k]); });
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
  var ON = ":root.site-cursor:not(.game-cursor)", BUSY = ":root.site-cursor.site-busy:not(.game-cursor)";
  var HAND = ["a[href]", "button:not(:disabled)", "select:not(:disabled)", "summary", "label[for]", "[role=button]:not([aria-disabled=true])",
    "[role=tab]", "input[type=checkbox]:not(:disabled)", "input[type=radio]:not(:disabled)", "input[type=range]:not(:disabled)",
    "input[type=file]:not(:disabled)", "input[type=button]:not(:disabled)", "input[type=submit]:not(:disabled)", ".badge[draggable=true]"];
  var TEXT = ["input:not([type])", "input[type=text]", "input[type=search]", "input[type=email]", "input[type=number]", "input[type=url]",
    "textarea", "[contenteditable=true]"];
  var DISABLED = ["button:disabled", "select:disabled", "input:disabled", "textarea:disabled", "[aria-disabled=true]"];
  function css(set) {
    var on = ON;
    var P = value(set.pointer, "auto"), H = value(set.hand, "pointer"), D = value(set.drag || set.hand, "grabbing"), T = value(set.text, "text");
    function each(list) { return list.map(function (x) { return on + " " + x; }).join(", "); }
    // Where the game has no image for a context, it keeps the cursor it had before: a disabled control the pointer, a busy page
    // whatever is under the mouse. The wait cursor wins over every other rule but the dragging one (the ids are no element's:
    // they only raise the rules' weight), so that a cap still shows the dragging cursor while the games it adds are read.
    return on + ", " + on + " * { cursor: " + P + " !important; }\n" +
      each(HAND) + " { cursor: " + H + " !important; }\n" +
      each(TEXT) + " { cursor: " + T + " !important; }\n" +
      (set.notallowed ? each(DISABLED) + " { cursor: " + value(set.notallowed, "not-allowed") + " !important; }\n" : "") +
      (set.wait ? BUSY + ", " + BUSY + " *:not(#bpc-wait) { cursor: " + value(set.wait, "wait") + " !important; }\n" : "") +
      on + " .badge[draggable=true]:active:not(#bpc-drag) { cursor: " + D + " !important; }\n" +
      // over a game that shows no cursor of its own: SDL's cursor for the canvas (its inline style) by kind, and none stays none
      on + ' canvas[style*="cursor: pointer"] { cursor: ' + H + " !important; }\n" +
      on + ' canvas[style*="cursor: text"] { cursor: ' + T + " !important; }\n" +
      on + ' canvas[style*="cursor: move"], ' + on + ' canvas[style*="cursor: grab"] { cursor: ' + D + " !important; }\n" +
      on + ' canvas[style*="cursor: none"] { cursor: none !important; }\n';
  }
  var styleEl = null;
  var current = null;  // the chosen set as apply() last read it
  function apply() {
    var set = chosen() ? all()[chosen()] || null : null, root = document.documentElement;
    current = set;
    if (!set) {
      root.classList.remove("site-cursor");
      if (styleEl) styleEl.textContent = "";
      animate(hovered);
      return;
    }
    if (!styleEl) {
      styleEl = document.createElement("style");
      styleEl.id = "byopop-site-cursor";
      (document.head || root).appendChild(styleEl);
    }
    styleEl.textContent = css(set);
    root.classList.add("site-cursor");
    animate(hovered);
  }

  // ---------- the site is busy ----------
  // busy(what, on): a piece of work (a game page loading, games being added, a save backup, restore or import) has started or
  // ended; the page counts as busy while any has not ended, and the chosen set's wait cursor (if it has one) shows everywhere
  var busyWith = {};
  function busy(what, on) {
    if (on) busyWith[what] = true; else delete busyWith[what];
    document.documentElement.classList.toggle("site-busy", Object.keys(busyWith).length > 0);
    animate(hovered);
  }

  // ---------- animated cursors ----------
  // Browsers animate no cursor image, so the element under the mouse gets each frame in turn as its own cursor (an inline style,
  // which outweighs the site's rules), and gives it back when the mouse leaves it or that cursor no longer applies. Only one
  // element and one timer at a time, none for a set without frames, and none while the page is hidden. The browser shows a
  // changed cursor without the mouse moving.
  var hovered = null, animKind = "", animEl = null, animTimer = 0, animAt = 0;
  // the kind of cursor the site's rules give el, as css() orders them
  function kindOf(el, set) {
    var root = document.documentElement;
    if (!el || el.nodeType !== 1 || !root.classList.contains("site-cursor") || root.classList.contains("game-cursor")) return "";
    function any(list) { return list.some(function (x) { try { return el.matches(x); } catch (e) { return false; } }); }
    if (any([".badge[draggable=true]:active"])) return set.drag ? "drag" : "hand";
    if (set.wait && root.classList.contains("site-busy")) return "wait";
    if (set.notallowed && any(DISABLED)) return "notallowed";
    if (any(TEXT)) return set.text ? "text" : "";
    if (any(HAND)) return set.hand ? "hand" : "";
    if (el.tagName === "CANVAS") {
      var s = el.getAttribute("style") || "";
      if (/cursor:\s*none/.test(s)) return "";
      if (/cursor:\s*pointer/.test(s)) return set.hand ? "hand" : "";
      if (/cursor:\s*text/.test(s)) return set.text ? "text" : "";
      if (/cursor:\s*(move|grab)/.test(s)) return set.drag ? "drag" : set.hand ? "hand" : "";
    }
    return "pointer";
  }
  function stopAnimation() {
    clearInterval(animTimer);
    animTimer = 0;
    if (animEl) { try { animEl.style.removeProperty("cursor"); } catch (e) {} }
    animEl = null; animKind = "";
  }
  function animate(el) {
    var set = current, kind = set ? kindOf(el, set) : "", c = kind ? set[kind] : null;
    if (!c || !c.frames || document.visibilityState === "hidden") { stopAnimation(); return; }
    if (el === animEl && kind === animKind) return;
    stopAnimation();
    var frames = [c].concat(c.frames), fallback = { pointer: "auto", hand: "pointer", drag: "grabbing", text: "text", notallowed: "not-allowed", wait: "wait" }[kind];
    animEl = el; animKind = kind; animAt = 0;
    function show() {
      animEl.style.setProperty("cursor", value(frames[animAt], fallback), "important");
      animAt = (animAt + 1) % frames.length;
    }
    show();
    animTimer = setInterval(show, c.ms);
  }
  // (cheap: a set without frames never starts a timer; the listeners only look at the element the mouse entered)
  function anyFrames() { var set = current; return !!set && KINDS.some(function (k) { return !!(set[k] && set[k].frames); }); }
  function later() { setTimeout(function () { animate(hovered); }, 0); }
  document.addEventListener("mouseover", function (ev) { hovered = ev.target; if (animTimer || anyFrames()) animate(hovered); }, true);
  document.addEventListener("mouseout", function (ev) { if (!ev.relatedTarget) { hovered = null; stopAnimation(); } }, true);
  // a press and a release can change the rule that applies (a cap being dragged)
  document.addEventListener("mousedown", function () { if (animTimer || anyFrames()) later(); }, true);
  document.addEventListener("mouseup", function () { if (animTimer || anyFrames()) later(); }, true);
  document.addEventListener("visibilitychange", function () { animate(hovered); });

  // ---------- the choice in the Settings ----------
  // A group of radio buttons: the browser's own cursor, then each set with its pointer and hand. Games whose sets are the same,
  // every kind with the same image, hot spot and frames, share one choice named with all their titles (the user, 2026-11-04:
  // "Alchemy and Bejeweled have the same cursor ... doesn't help to list both"); the sets stay one per game, and choosing the
  // group keeps one member's id, so the groups form again from what is kept: a set that changes leaves its group, and forgetting a
  // game leaves the rest. Rendered into el, kept up to date with the other tabs.
  var STYLE =
    ".bpc-cur { margin: 12px 0 0; padding: 0; border: 0; min-width: 0; display: grid; gap: 6px; }\n" +
    // the legend as the Settings' group headings (web/settings.js, .bps-group), the choices in one box as its rows
    ".bpc-cur legend { padding: 0; margin-bottom: 8px; font-size: 12.5px; font-weight: 600; letter-spacing: 0.04em; text-transform: uppercase; color: var(--muted, #6b6680); }\n" +
    ".bpc-cur .bpc-cur-box { display: grid; gap: 4px; padding: 10px 14px; border: 1px solid var(--line, #d9d5e3); border-radius: 10px; background: var(--panel-2, var(--panel, transparent)); }\n" +
    // a row: the button, the set's cursors and the games' names; on a narrow screen the cursors wrap and the names go under them
    ".bpc-cur label { display: flex; flex-wrap: wrap; align-items: center; gap: 4px 8px; min-height: 32px; }\n" +
    ".bpc-cur .bpc-cur-pics { display: inline-flex; flex-wrap: wrap; align-items: center; gap: 4px; min-width: 72px; max-width: 100%; }\n" +
    ".bpc-cur .bpc-cur-pics img { max-width: 32px; max-height: 32px; image-rendering: pixelated; }\n" +
    ".bpc-cur .bpc-cur-name { flex: 1 1 8em; min-width: 0; overflow-wrap: anywhere; }\n" +
    "@media (max-width: 480px) { .bpc-cur .bpc-cur-pics img { max-width: 24px; max-height: 24px; } }\n" +
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
        // every cursor the set has, in KINDS' order (an animated one by its first frame), as pictures only: the names say which
        var pics = make("span", "bpc-cur-pics");
        KINDS.forEach(function (k) {
          if (!set[k]) return;
          var img = make("img");
          img.src = set[k].url; img.alt = "";
          img.setAttribute("data-kind", k);
          pics.appendChild(img);
        });
        label.appendChild(pics);
      }
      label.appendChild(make("span", "bpc-cur-name", text));
      box.appendChild(label);
    }
    option("", ByopopStrings.t("cursors.browser"), null);
    groups(sets).forEach(function (g) {
      // the chosen member stands for its group; else the first by title
      var id = g.ids.indexOf(pick) >= 0 ? pick : g.ids[0];
      option(id, names(g.ids.map(function (i) { return sets[i].title; })), sets[id]);
    });
    box.appendChild(make("p", "bpc-cur-hint", Object.keys(sets).length
      ? ByopopStrings.t("cursors.hint")
      : ByopopStrings.t("cursors.hint_none")));
    el.appendChild(fs);
  }
  // The sets in groups of the same images, {ids} each with its games sorted by title, the groups by their first title
  function groups(sets) {
    var byLook = {}, out = [];
    function byTitle(a, b) { return sets[a].title.localeCompare(sets[b].title) || (a < b ? -1 : a > b ? 1 : 0); }
    Object.keys(sets).sort(byTitle).forEach(function (id) {
      var look = JSON.stringify(KINDS.map(function (k) { return sets[id][k] || null; }));
      if (!byLook[look]) out.push(byLook[look] = { ids: [] });
      byLook[look].ids.push(id);
    });
    return out;
  }
  // "Alchemy", "Alchemy and Bejeweled", "Alchemy, Atomica and Bejeweled"
  function names(titles) {
    if (titles.length < 2) return titles[0] || "";
    if (titles.length === 2) return ByopopStrings.t("site.list_two", {first: titles[0], second: titles[1]});
    return ByopopStrings.t("site.list_many", {rest: titles.slice(0, -1).join(", "), last: titles[titles.length - 1]});
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
  return { KEY: KEY, CHOICE: CHOICE, all: all, save: save, forget: forget, chosen: chosen, choose: choose, apply: apply, render: render,
    refresh: redraw, busy: busy };
})();
