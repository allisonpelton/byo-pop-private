// The game windows' Close never depends on the page inside (the user, 2026-11-11: "If a game's iframe fails to load, the
// window becomes unclosable with the button or menu"; web/deskapps.js, closeApp). Runs in the front page, like deskcheck.js:
//   http://127.0.0.1:18780/index.html?silent=1&driver=tools/web/closecheck.js
// Each case opens a game's window, puts another page in its frame and closes it as the close box does (the desktop's
// close, which the window menu's Close and the task bar's Close also call):
//   missing   a page that is not there (404): the window goes at once
//   failed    a frame that could not load at all (an address nothing answers, the browser's error page): at once
//   silent    a page that takes the close but never hands the window back: still open at once, gone after the wait
//   twice     the same page closed twice: the second Close drops it at once
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
var d = ByopopSkin.desk;
ByopopSkin.deskSync();
if (!d.isOn()) return { pass: 0, fail: 1, results: [{ check: "desktop", result: "FAIL", detail: "the desktop is off" }] };

function appIds() { return d.list().filter(function (w) { return /^app-/.test(w.id); }).map(function (w) { return w.id; }); }
async function openGame() {
  var before = appIds();
  ByopopApps.open(new URL("bejeweled.html", location.href).href, "none");
  await wait(100);
  var id = appIds().filter(function (x) { return before.indexOf(x) < 0; })[0];
  return { id: id, frame: id && d.find(id).el.querySelector("iframe") };
}
function loaded(frame) { return new Promise(function (r) { frame.addEventListener("load", function () { r(); }, { once: true }); setTimeout(r, 5000); }); }
function isOpen(id) { return appIds().indexOf(id) >= 0; }

// missing
var g = await openGame();
if (!g.id) note("open", false, "no game window opened");
else {
  var l = loaded(g.frame); g.frame.src = "/__no_such_page__.html"; await l;
  d.close(g.id);
  note("missing", !isOpen(g.id), isOpen(g.id) ? "still open after Close" : "closed at once");

  // failed: nothing listens on port 9, so the frame shows the browser's own error page (another origin)
  g = await openGame();
  l = loaded(g.frame); g.frame.src = "http://127.0.0.1:9/"; await l;
  d.close(g.id);
  note("failed", !isOpen(g.id), isOpen(g.id) ? "still open after Close" : "closed at once");

  // silent: a page that answers nothing
  g = await openGame();
  l = loaded(g.frame); g.frame.src = "about:blank"; await l;
  g.frame.contentWindow.ByopopEmbed = { close: function () {}, menus: function () { return {}; }, least: function () { return null; } };
  d.close(g.id);
  var first = isOpen(g.id);
  await wait(4500);
  note("silent", first && !isOpen(g.id), (first ? "waited for the page" : "closed without waiting") + ", " + (isOpen(g.id) ? "still open after 4.5 s" : "gone after the wait"));

  // twice
  g = await openGame();
  l = loaded(g.frame); g.frame.src = "about:blank"; await l;
  g.frame.contentWindow.ByopopEmbed = { close: function () {}, menus: function () { return {}; }, least: function () { return null; } };
  d.close(g.id);
  d.close(g.id);
  note("twice", !isOpen(g.id), isOpen(g.id) ? "still open after a second Close" : "the second Close dropped it");
  if (isOpen(g.id)) d.close(g.id);
}
var fail = results.filter(function (x) { return x.result === "FAIL"; }).length;
return { pass: results.length - fail, fail: fail, results: results };
