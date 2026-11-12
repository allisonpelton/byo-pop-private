// The Music window's channel mutes (the user, 2026-11-11: "Channels appear to stay muted when I switch tracks in the player but
// they still sound"; "Switching off the tracker view should unmute all the tracks, but returning to it should re-mute what was
// previously muted"; web/music.js, syncMutes). Runs in the front page, like deskcheck.js, with games added that have module
// tunes (two at least, two channels or more):
//   http://127.0.0.1:18780/index.html?silent=1&driver=tools/web/musicmutecheck.js
// What plays is read from the player itself (bm_mute_status), what shows from the tracker's header. The cases:
//   muted       a channel muted from the header: struck through there, muted in the player, the others neither
//   view off    the tracker view turned off: every channel plays, no header shows
//   view on     the view back: the same channel muted again, drawn and in the player
//   switch      another tune: its header and its player agree (none muted)
//   back        the first tune again: its mute is back, drawn and in the player
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
async function until(fn, ms) { for (var t = 0; t < ms; t += 50) { var v = fn(); if (v) return v; await wait(50); } return fn(); }
// the browser starts sound only after the page was clicked: it waits up to a minute for a click (a driver clicks the page once)
if (navigator.userActivation && !navigator.userActivation.hasBeenActive) {
  for (var w0 = 0; w0 < 600 && !navigator.userActivation.hasBeenActive; w0++) await new Promise(function (r) { setTimeout(r, 100); });
}
// a hidden pane runs no animation frames, which draw the tracker's header: they run on a timer here while it is hidden
var rafWas = window.requestAnimationFrame;
if (document.hidden) window.requestAnimationFrame = function (cb) { return setTimeout(function () { cb(performance.now()); }, 16); };
var d = ByopopSkin.desk;
ByopopSkin.deskSync();
var wasClosed = d.find("tab-music") && d.find("tab-music").el.classList.contains("d95-closed");
if (wasClosed) d.openWin("tab-music");
var P = window.ByopopMusicPanel;
var tunes = await until(function () { var t = P.tunes().filter(function (x) { return x.kind === "module"; }); return t.length >= 2 ? t : null; }, 20000);
if (!tunes) return { pass: 0, fail: 1, results: [{ check: "tunes", result: "FAIL", detail: "fewer than two module tunes: add games first" }] };
var all = P.tunes();
async function playTune(t) {
  P.play(all.indexOf(t));
  // the tune playing, and its header drawn for it (the view's row key starts with the tune)
  return until(function () {
    var ch = P.playing() && P.view().row.indexOf("own:" + t.id + ":") === 0 && P.channels();
    return ch && ch.tune === t.id && ch.list.length && ch.list.every(function (x) { return x.head !== null; }) ? ch : null;
  }, 15000);
}
function show(ch) { return ch ? ch.list.map(function (x) { return x.c + ":" + (x.head === null ? "-" : x.head ? "M" : "o") + "/" + x.heard; }).join(" ") : "no channels"; }
P.setVis("tracker");
// the first tune with two channels or more, and a second
var A = null, B = null, chA = null;
for (var i = 0; i < tunes.length && !B; i++) {
  var ch = await playTune(tunes[i]);
  if (!ch || ch.list.length < 2) continue;
  if (!A) { A = tunes[i]; chA = ch; } else B = tunes[i];
}
if (!A || !B) return { pass: 0, fail: 1, results: [{ check: "tunes", result: "FAIL", detail: "no two module tunes with two channels" }] };

await playTune(A);
var c = chA.list[0].c;
P.clickChannel(c);
await wait(100);
var ch = P.channels();
note("muted", ch && ch.list.every(function (x) { return x.c === c ? x.head === true && x.heard === 1 : x.head === false && x.heard === 0; }), A.id + ": " + show(ch));

P.setVis("spectrum");
await wait(100);
ch = P.channels();
note("view off", ch && ch.list.every(function (x) { return x.head === null && x.heard === 0; }), show(ch));

P.setVis("tracker");
await wait(100);
ch = await until(function () { var x = P.channels(); return x && x.list.every(function (y) { return y.head !== null; }) ? x : null; }, 5000);
note("view on", ch && ch.list.every(function (x) { return x.c === c ? x.head === true && x.heard === 1 : x.head === false && x.heard === 0; }), show(ch));

ch = await playTune(B);
note("switch", ch && ch.list.every(function (x) { return x.head === false && x.heard === 0; }), B.id + ": " + show(ch));

ch = await playTune(A);
await wait(100);
ch = P.channels();
note("back", ch && ch.list.every(function (x) { return x.c === c ? x.head === true && x.heard === 1 : x.head === false && x.heard === 0; }), A.id + ": " + show(ch));

P.clickChannel(c);

// The keyboard's media keys (the user, 2026-11-11), as the page hears them, with the window open and closed: what the player and
// its sound context do, not its buttons
function key(k) { window.dispatchEvent(new KeyboardEvent("keydown", { key: k, bubbles: true })); }
function st() { return P.state() + "/" + P.held().ctx + "/" + P.current(); }
key("MediaPlayPause"); await wait(150);
note("key pause", P.state() === "paused" && P.held().ctx === "suspended", st());
key("MediaPlayPause"); await wait(300);
note("key play", P.state() === "playing" && P.held().ctx === "running", st());
var before = P.current();
key("MediaTrackNext");
await until(function () { return P.current() !== before && P.playing(); }, 15000);
note("key next", P.current() === all[(all.findIndex(function (x) { return x.id === before; }) + 1) % all.length].id && P.playing(), before + " -> " + st());
key("MediaTrackPrevious");
await until(function () { return P.current() === before && P.playing(); }, 15000);
note("key previous", P.current() === before && P.playing(), st());
key("MediaStop"); await wait(150);
note("key stop", P.state() === "idle", st());
d.close("tab-music"); await wait(100);
key("MediaPlayPause"); await wait(300);
note("closed", P.state() === "idle", "Music closed, Play/Pause: " + st());
d.openWin("tab-music");

// put it back: the player stopped
P.shutdown();
if (wasClosed) d.close("tab-music");
window.requestAnimationFrame = rafWas;
var fail = results.filter(function (x) { return x.result === "FAIL"; }).length;
return { pass: results.length - fail, fail: fail, results: results };
