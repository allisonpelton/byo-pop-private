// The desktop's accessibility check (docs/WEB_PLAYER.md, "The page check"): the front page's Desktop 95 as a screen reader
// meets it, which tools/a11ycheck.js cannot see in the built files, since the desktop's windows are built by its scripts. It
// runs in the front page, served by tools/webtest/serve.py on a development host, at a width the desktop takes (760 pixels
// or more):
//   http://127.0.0.1:18780/index.html?silent=1&driver=tools/web/deskcheck.js
// The page fetches it from /__repo/ and runs it as the body of an async function of t (null here) and args; its result is
// window.byopopDeskCheck.result (and the page's title ends "deskcheck: PASS" or "deskcheck: FAIL"):
// {pass, fail, results: [{check, result: "pass"|"FAIL", detail}]}. The checks:
//   desktop   the desktop is on (the page is wide enough)
//   window    each window of the desktop, opened in turn: a region (or a landmark of its own: main, footer, aside) named by
//             aria-labelledby after an h2 whose text is the window's title, with no tabpanel role left from the narrow page's
//             tabs, and its title the only heading saying it
//   a11y      tools/a11ycheck.js's checks on the live page, with that window open (A11yCheck.checkHtml on the page's
//             markup as the scripts left it, live: what is hidden at that moment is left out): every error, by window
//   headings  with every window open, the h2 titles of the open windows are all among the page's headings, under its one h1
// Every window it opened is closed again at the end.
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }

if (!window.A11yCheck) {
  var r = await fetch("/__repo/tools/a11ycheck.js", { cache: "no-store" });
  if (!r.ok) throw new Error("deskcheck: tools/a11ycheck.js: " + r.status);
  // (without its #! line, which only Node reads)
  (new Function((await r.text()).replace(/^#!.*/, "")))();
}
var d = ByopopSkin.desk;
ByopopSkin.deskSync();
note("desktop", d.isOn(), d.isOn() ? "" : "the desktop is off: the page is " + document.documentElement.clientWidth + " pixels wide");

var LANDMARK = { MAIN: "main", FOOTER: "contentinfo", ASIDE: "complementary", NAV: "navigation", HEADER: "banner", FORM: "form" };
function windowCheck(id, title, el) {
  var bad = [];
  var role = el.getAttribute("role") || LANDMARK[el.tagName] || "";
  if (role === "tabpanel") bad.push("still role tabpanel");
  else if (role !== "region" && role !== "dialog" && !LANDMARK[el.tagName]) bad.push("role " + (role || "none") + ", not a region");
  var by = (el.getAttribute("aria-labelledby") || "").split(/\s+/).filter(Boolean);
  var named = by.map(function (x) { return document.getElementById(x); }).filter(Boolean);
  if (!named.length) bad.push("no aria-labelledby that names it");
  else {
    var h = named[0];
    if (h.tagName !== "H2") bad.push("named by a " + h.tagName.toLowerCase() + ", not an h2");
    if (h.textContent.trim() !== title.trim()) bad.push("named \"" + h.textContent.trim() + "\", titled \"" + title + "\"");
    if (!el.contains(h)) bad.push("its name is outside it");
  }
  var again = Array.prototype.filter.call(el.querySelectorAll("h1, h2, h3, h4, h5, h6"), function (x) {
    return named.indexOf(x) < 0 && !x.hidden && x.getAttribute("role") !== "presentation" && x.getAttribute("role") !== "none" &&
      x.closest("[hidden]") === null && x.textContent.trim().toLowerCase() === title.trim().toLowerCase();
  });
  if (again.length) bad.push(again.length + " other heading(s) saying the title again");
  note("window", !bad.length, id + ": " + (bad.length ? bad.join("; ") : "a " + role + " named \"" + title + "\""));
}

if (d.isOn()) {
  var wins = d.list().filter(function (w) { return !/^app-/.test(w.id); });
  var opened = [];
  for (var i = 0; i < wins.length; i++) {
    var w = wins[i], win = d.find(w.id);
    if (!win) continue;
    // (the chat's window is checked closed: opening it starts the chat, which talks to its server)
    if ((w.closed || win.el.hidden) && w.id !== "chat") { d.openWin(w.id); opened.push(w.id); await wait(50); }
    windowCheck(w.id, w.title, win.el);
    // what is showing (live), on the front page (its music's canvases do not make it a game's)
    var rep = A11yCheck.checkHtml(document.documentElement.outerHTML, { name: "desk:" + w.id, live: true, kind: "index" });
    note("a11y", !rep.errors.length, w.id + ": " + rep.checked + " checks" + (rep.errors.length ? ", " + rep.errors.join(" | ") : ", 0 errors"));
  }
  var h1 = document.querySelectorAll("h1").length;
  var titles = wins.map(function (w) { var x = d.find(w.id); return x && x.el.getAttribute("aria-labelledby"); }).filter(Boolean);
  var missing = titles.filter(function (id) { var h = document.getElementById(id); return !h || h.tagName !== "H2"; });
  note("headings", h1 === 1 && !missing.length, h1 + " h1, " + titles.length + " window titles" + (missing.length ? ", not h2: " + missing.join(", ") : ", every one an h2"));
  for (var k = opened.length - 1; k >= 0; k--) d.close(opened[k]);
}
var fail = results.filter(function (x) { return x.result === "FAIL"; }).length;
return { pass: results.length - fail, fail: fail, results: results };
