// The coming-soon tiles of ports this build made (web/localsoon.js, docs/WEB_PLAYER.md "Coming soon games on a development host"):
// opened only on a development host, never elsewhere or with ?localsoon=0, and only where cmake marked a page.
//   node tools/localsoontest.js
// It prints a line for each check that fails, then "localsoontest: N checks, M failed", and exits 1 when any failed.
"use strict";
const path = require("path");
const LS = require(path.join(__dirname, "..", "web", "localsoon.js"));
const { devHost } = (function () {
  // web/terms.js's own devHost, read from the file so the test follows it
  const src = require("fs").readFileSync(path.join(__dirname, "..", "web", "terms.js"), "utf8");
  const m = /function devHost\(loc\) \{[\s\S]*?\n  \}/.exec(src);
  return { devHost: new Function("return " + m[0])() };
})();

let checks = 0, failed = 0;
function check(ok, what) { checks++; if (!ok) { failed++; console.log("FAIL " + what); } }
const at = (href) => { const u = new URL(href); return { protocol: u.protocol, hostname: u.hostname, search: u.search }; };

// ---- where
check(LS.enabled(at("http://127.0.0.1:18795/?silent=1"), devHost), "127.0.0.1 opens them");
check(LS.enabled(at("http://localhost/"), devHost), "localhost opens them");
check(LS.enabled(at("http://[::1]:8000/"), devHost), "[::1] opens them");
check(LS.enabled(at("http://site.test/"), devHost), "*.test opens them");
check(LS.enabled(at("file:///C:/byo-pop/build/web/index.html"), devHost), "a file:// page opens them");
check(!LS.enabled(at("https://byo-pop.example.org/"), devHost), "a deployed host keeps them shut");
check(!LS.enabled(at("https://localhost.example.org/"), devHost), "a host that only starts with localhost keeps them shut");
check(!LS.enabled(at("http://127.0.0.1:18795/?silent=1&localsoon=0"), devHost), "?localsoon=0 keeps them shut on a development host");
check(!LS.enabled(at("http://127.0.0.1/"), () => false), "devHost false keeps them shut");

// ---- a tiny DOM: the listing's cards
function el(tag, attrs, kids) {
  const e = { tagName: tag, attrs: Object.assign({}, attrs || {}), kids: [], parentNode: null, classes: new Set(), title: "" };
  e.getAttribute = (k) => (k in e.attrs ? e.attrs[k] : null);
  e.setAttribute = (k, v) => { e.attrs[k] = String(v); if (k === "class") e.classes = new Set(String(v).split(/\s+/).filter(Boolean)); };
  e.hasAttribute = (k) => k in e.attrs;
  Object.defineProperty(e, "className", { set: (v) => e.setAttribute("class", v), get: () => [...e.classes].join(" ") });
  e.classList = { add: (c) => e.classes.add(c), remove: (c) => e.classes.delete(c), contains: (c) => e.classes.has(c) };
  if (e.attrs.class) e.setAttribute("class", e.attrs.class);
  Object.defineProperty(e, "firstChild", { get: () => e.kids[0] || null });
  e.appendChild = (k) => { if (k.parentNode) k.parentNode.kids.splice(k.parentNode.kids.indexOf(k), 1); k.parentNode = e; e.kids.push(k); return k; };
  e.replaceChild = (n, o) => { const i = e.kids.indexOf(o); e.kids[i] = n; n.parentNode = e; o.parentNode = null; return o; };
  e.querySelector = (sel) => { const c = sel.replace(/^\./, ""); for (const k of e.kids) if (k.classes.has(c)) return k; return null; };
  (kids || []).forEach((k) => e.appendChild(k));
  return e;
}
function card(marks) {
  const div = el("div", Object.assign({ class: "tile", "aria-label": "AstroPop, 2004, coming soon" }, marks), [el("span", { class: "badge" }), el("span", { class: "name" })]);
  return el("li", { class: "game soon" }, [div, el("div", { class: "ownrow" })]);
}
function listing() {
  const cards = [
    card({ "data-local-page": "astropop.html", "data-local-game": "astropop", "data-local-persist": "/astropop", "data-local-lock": "byopop-astropop" }),
    card({ "data-local-page": "bejeweled2.html", "data-local-game": "bejeweled2", "data-local-persist": "/bj2", "data-local-lock": "byopop-bj2" }),
    card({})  // Venice: no page in this build
  ];
  const doc = {
    createElement: (t) => el(t),
    querySelectorAll: () => cards.map((li) => li.kids[0]).filter((d) => d.tagName === "div" && d.hasAttribute("data-local-page") && d.parentNode.classes.has("soon"))
  };
  return { doc, cards };
}

{
  const { doc, cards } = listing();
  check(LS.apply(doc, at("http://127.0.0.1/"), devHost, "This computer only") === 2, "two marked tiles opened");
  const a = cards[0].kids[0];
  check(a.tagName === "a" && a.getAttribute("href") === "astropop.html" && a.getAttribute("data-game") === "astropop" &&
    a.getAttribute("data-persist") === "/astropop" && a.getAttribute("data-lock") === "byopop-astropop", "the tile is a finished game's link");
  check(a.kids.length === 3 && a.kids[2].classes.has("own"), "the badge and name moved in, and the files' state has its place");
  check(!cards[0].classes.has("soon") && cards[0].classes.has("soon-local"), "the card is marked as opened here");
  check(cards[0].kids[1].kids.length === 1 && cards[0].kids[1].kids[0].classes.has("soon-local-sign"), "the sign is beside Coming soon");
  check(cards[2].kids[0].tagName === "div" && cards[2].classes.has("soon"), "a game with no page stays shut");
}
{
  const { doc, cards } = listing();
  check(LS.apply(doc, at("https://byo-pop.example.org/"), devHost) === 0 && cards.every((c) => c.kids[0].tagName === "div" && c.classes.has("soon")),
    "on a deployed host all three stay shut");
}
{
  const { doc, cards } = listing();
  check(LS.apply(doc, at("http://127.0.0.1/"), () => false) === 0 && cards.every((c) => c.kids[0].tagName === "div"), "devHost forced false: all shut");
}
check(LS.linkOf(el("div", { "data-local-page": "x.js", "data-local-game": "x" })) === null, "only a page of this site's own shape is linked");

console.log("localsoontest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
