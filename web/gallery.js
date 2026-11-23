// The Gallery (the user, 2026-11-22: "Rename Venice Buildings to Gallery; we'll collect other interesting graphics there for
// each game once they're all done. In particular, I'd like to show off animations and other things you can't see just by
// looking at the image files directly"): a Desktop 95 program on the listing page's desktop that shows a game's pictures from
// the player's own files in this browser. Its Game list holds the games of the listing that have an entry here, and under
// it the chosen game's entry draws its own controls and picture. One entry so far, Venice's buildings (web/buildings.js).
// Nothing is shipped: an entry reads the player's files while the window is open and lets them go when it closes.
//
// An entry (ENTRIES): the game's id and its module (a global), whose panel(game) makes { el, show(), hide() } for game = { id,
// persist }: show reads and draws, hide lets the files go. The window shows the chosen entry's panel while it is open on the
// desktop.

var ByopopGallery = (function () {
  "use strict";
  function T(k, a) { return typeof ByopopStrings !== "undefined" ? ByopopStrings.t(k, a) : k; }
  function make(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; }

  var ENTRIES = [{ id: "venice", module: "ByopopBuildings" }];

  var win = null;
  // The window on the desktop (web/index.html), closed at first, only where the listing has a game with an entry; at(left, vw)
  // gives its place
  function attach(desk, at) {
    if (win || !document.getElementById("games-title")) return null;
    var games = [];
    ENTRIES.forEach(function (e) {
      var tile = document.querySelector('a.tile[data-game="' + e.id + '"]'), mod = window[e.module];
      if (tile && mod && mod.panel) games.push({ module: mod, id: e.id, title: tile.querySelector(".name").textContent, persist: tile.getAttribute("data-persist") });
    });
    if (!games.length) return null;
    games.sort(function (a, b) { return a.title.localeCompare(b.title); });

    var el = make("section", "gal-win");
    el.id = "gallery";
    var bar = make("div", "fv-bar"), pick = make("select", "gal-game"), label = make("label", "", T("gallery.game"));
    games.forEach(function (g) { var o = make("option", "", g.title); o.value = g.id; pick.appendChild(o); });
    label.appendChild(pick);
    bar.appendChild(label);
    el.appendChild(bar);

    var panels = {}, current = null, open = false;
    function panelOf(id) {
      if (!panels[id]) {
        var g = games.filter(function (x) { return x.id === id; })[0];
        panels[id] = g.module.panel({ id: g.id, persist: g.persist });
        el.appendChild(panels[id].el);
      }
      return panels[id];
    }
    function choose() {
      if (current) { current.hide(); current.el.hidden = true; }
      current = panelOf(pick.value);
      current.el.hidden = false;
      if (open) current.show();
    }
    pick.addEventListener("change", choose);
    choose();

    var foot = document.querySelector("body > footer");
    if (foot && foot.parentNode) foot.parentNode.insertBefore(el, foot); else document.body.appendChild(el);
    desk.add(el, { id: "gallery", title: T("gallery.title"), closable: true, closed: true, at: at, scroll: ".vb-well",
      minWidth: 420, minHeight: 290 });
    function watch() {
      // (minimized, it keeps what it shows)
      var now = !el.classList.contains("d95-closed") && document.documentElement.classList.contains("d95-desk");
      if (now === open) return;
      open = now;
      if (open) current.show(); else current.hide();
    }
    if (window.MutationObserver) {
      new MutationObserver(watch).observe(el, { attributes: true, attributeFilter: ["class"] });
      new MutationObserver(watch).observe(document.documentElement, { attributes: true, attributeFilter: ["class"] });
    }
    watch();
    win = { el: el, game: function () { return pick.value; }, panel: function () { return current; } };
    return win;
  }

  return { attach: attach, window: function () { return win; } };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopGallery;
