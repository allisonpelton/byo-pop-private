// The Guides window (the user, 2026-11-18, the one exception to the site's rule of no AI prose: "I've decided I'm actually ok
// with AI-generated guides for the games that actually benefit from it on the site, as long as they are clearly marked as
// AI-generated"): a Desktop 95 window on the listing page's desktop in the manner of a help topic, showing the guide of the game
// chosen. The texts are in web/guidetexts.js and follow docs/GUIDE_STYLE.md; the window draws the AI-generated banner at the
// head of every guide, with the date it was checked, and says AI-generated in its status line, so no guide shows without it.
// A guide is drawn as text only.

var ByopopGuides = (function () {
  "use strict";
  function T(k, a) { return typeof ByopopStrings !== "undefined" ? ByopopStrings.t(k, a) : k; }

  // ---------- the markup ----------
  // A guide's text as {sections: [{head, items: [{tip, lead, text}], notes}], tips, words}: "## " a heading, "- " a tip whose
  // lead is the words before the first colon, "// " a note (counted, not kept), any other line a paragraph; lines next to each
  // other join, and a blank line ends a paragraph or tip
  function parse(text) {
    var sections = [], cur = null, item = null, tips = 0, words = 0;
    function section(head) { cur = { head: head, items: [], notes: 0 }; sections.push(cur); item = null; }
    String(text || "").split(/\r?\n/).forEach(function (raw) {
      var line = raw.trim();
      if (!line) { item = null; return; }
      if (line.indexOf("// ") === 0 || line === "//") { if (!cur) section(""); cur.notes++; item = null; return; }
      if (line.indexOf("## ") === 0) { section(line.slice(3).trim()); return; }
      if (!cur) section("");
      if (line.indexOf("- ") === 0) {
        var body = line.slice(2).trim(), at = body.indexOf(":");
        item = { tip: true, lead: at > 0 ? body.slice(0, at).trim() : "", text: at > 0 ? body.slice(at + 1).trim() : body };
        cur.items.push(item);
        tips++;
        return;
      }
      if (item) item.text += " " + line;
      else { item = { tip: false, lead: "", text: line }; cur.items.push(item); }
    });
    sections.forEach(function (s) {
      words += s.head.split(/\s+/).filter(Boolean).length;
      s.items.forEach(function (i) { words += (i.lead + " " + i.text).split(/\s+/).filter(Boolean).length; });
    });
    return { sections: sections, tips: tips, words: words };
  }

  // ---------- the window ----------
  function make(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; }
  function labelled(text, control) { var l = make("label", "", text); l.appendChild(control); return l; }

  var win = null;
  function build(games) {
    var el = make("section", "gd-win");
    el.id = "guides";
    var bar = make("div", "fv-bar"), pick = make("select", "gd-pick");
    games.forEach(function (g) { var o = make("option", "", g.title); o.value = g.id; pick.appendChild(o); });
    bar.appendChild(labelled(T("guides.game"), pick));
    var page = make("div", "gd-page"), status = make("footer", "gd-status");
    var statusAi = make("span", "", T("guides.ai")), statusCount = make("span");
    status.appendChild(statusAi); status.appendChild(statusCount);
    el.appendChild(bar); el.appendChild(page); el.appendChild(status);

    function draw() {
      var g = games.filter(function (x) { return x.id === pick.value; })[0];
      page.textContent = "";
      statusCount.textContent = "";
      if (!g) return;
      var guide = parse(g.text);
      // the banner: drawn by the window for every guide
      var banner = make("div", "gd-banner");
      banner.appendChild(make("b", "", T("guides.banner")));
      banner.appendChild(make("span", "", T("guides.checked", { date: g.checked })));
      page.appendChild(banner);
      page.appendChild(make("h2", "gd-title", g.title));
      guide.sections.forEach(function (s) {
        if (s.head) page.appendChild(make("h3", "gd-head", s.head));
        s.items.forEach(function (i) {
          var p = make("p", i.tip ? "gd-tip" : "gd-para");
          if (i.lead) { p.appendChild(make("b", "", i.lead + ":")); p.appendChild(document.createTextNode(" ")); }
          p.appendChild(document.createTextNode(i.text));
          page.appendChild(p);
        });
      });
      page.scrollTop = 0;
      statusCount.textContent = T("guides.count", { n: guide.tips });
    }
    pick.addEventListener("change", draw);
    return { el: el, show: draw, state: function () { return { game: pick.value }; } };
  }

  // The window on the desktop (web/index.html), closed at first, with a guide for each game on the listing that has one, by
  // the listing's titles; at(left, vw) gives its place
  function attach(desk, at) {
    var texts = typeof ByopopGuideTexts !== "undefined" ? ByopopGuideTexts : {};
    if (win || !document.getElementById("games-title")) return null;
    var games = Object.keys(texts).map(function (id) {
      var tile = document.querySelector('a.tile[data-game="' + id + '"]'), name = tile && tile.querySelector(".name");
      return name ? { id: id, title: name.textContent, checked: texts[id].checked, text: texts[id].text } : null;
    }).filter(Boolean).sort(function (a, b) { return a.title.localeCompare(b.title); });
    if (!games.length) return null;
    win = build(games);
    var foot = document.querySelector("body > footer");
    if (foot && foot.parentNode) foot.parentNode.insertBefore(win.el, foot); else document.body.appendChild(win.el);
    desk.add(win.el, { id: "guides", title: T("guides.title"), closable: true, closed: true, at: at, scroll: ".gd-page",
      minWidth: 320, minHeight: 240 });
    win.show();
    return win;
  }

  return { attach: attach, parse: parse, window: function () { return win; } };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopGuides;
