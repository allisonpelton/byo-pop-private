// The ports still in progress, on a development host only (the user, 2026-11-09, asked "Make built ports' Coming soon tiles open
// their game on the local site?": "Yes, locally"). cmake/ByopopWeb.cmake marks a coming-soon tile whose game's page this build
// made (data-local-page, data-local-game, data-local-persist, data-local-lock on its div.tile); on 127.0.0.1, localhost, [::1],
// *.localhost, *.test or a file:// page (web/terms.js, devHost) this turns such a tile into the link a finished game's tile is, so
// the listing, the Library, favourites and the Start menu open the game as they open any other. The card keeps its Coming soon
// look and gains a sign that it opens only here. On any other host nothing changes. ?localsoon=0 keeps them shut on a development
// host too (for tests). docs/WEB_PLAYER.md, "Coming soon games on a development host".
var ByopopLocalSoon = (function () {
  "use strict";

  function enabled(loc, devHost) {
    if (!loc || !devHost || !devHost(loc)) return false;
    try { return new URLSearchParams(loc.search || "").get("localsoon") !== "0"; } catch (e) { return true; }
  }

  // the link's attributes from the tile's marks; null when the tile has no page in this build
  function linkOf(div) {
    var page = div.getAttribute("data-local-page"), game = div.getAttribute("data-local-game");
    if (!page || !game || !/^[a-z0-9_-]+\.html$/.test(page)) return null;
    return { href: page, game: game, persist: div.getAttribute("data-local-persist") || "", lock: div.getAttribute("data-local-lock") || "" };
  }

  // turns the marked tiles into links; returns how many
  function apply(doc, loc, devHost, signTitle) {
    if (!enabled(loc, devHost)) return 0;
    var n = 0;
    Array.prototype.forEach.call(doc.querySelectorAll(".games > li.soon > div.tile[data-local-page]"), function (div) {
      var to = linkOf(div);
      if (!to) return;
      var li = div.parentNode, a = doc.createElement("a");
      a.className = "tile";
      a.setAttribute("href", to.href);
      a.setAttribute("data-game", to.game);
      a.setAttribute("data-persist", to.persist);
      a.setAttribute("data-lock", to.lock);
      if (div.hasAttribute("aria-label")) a.setAttribute("aria-label", div.getAttribute("aria-label"));
      while (div.firstChild) a.appendChild(div.firstChild);
      // the files check writes the files' state here, as on a finished game's card
      var own = doc.createElement("span");
      own.className = "own";
      own.setAttribute("aria-hidden", "true");
      a.appendChild(own);
      li.replaceChild(a, div);
      li.classList.remove("soon");
      li.classList.add("soon-local");
      // the sign: this computer's icon beside "Coming soon"
      var row = li.querySelector(".ownrow");
      if (row) {
        var sign = doc.createElement("span");
        sign.className = "soon-local-sign";
        sign.setAttribute("role", "img");
        if (signTitle) { sign.title = signTitle; sign.setAttribute("aria-label", signTitle); }
        row.appendChild(sign);
      }
      n++;
    });
    return n;
  }

  return { enabled: enabled, linkOf: linkOf, apply: apply };
})();
if (typeof module !== "undefined") module.exports = ByopopLocalSoon;
