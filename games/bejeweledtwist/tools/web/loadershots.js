// The loader and the main menu's arrival in the player-only page, one shot at each of a few update counts (the PopCap
// logo's span, for the site's "Always skip start-up logos"; NOTES.md, "The loader screen"). Nothing is clicked.
// Opened as .../bejeweledtwist.html?player=1&assets=bejeweledtwist-web&silent=1&driver=games/bejeweledtwist/tools/web/loadershots.js&run=<run>
var AT = [10, 40, 80, 120, 150, 200];
var tickChannel = new MessageChannel(), tickResolve = null;
tickChannel.port1.onmessage = function () { var r = tickResolve; tickResolve = null; if (r) r(); };
function tick() { return new Promise(function (r) { tickResolve = r; tickChannel.port2.postMessage(0); }); }
var taken = [];
for (var i = 0; i < AT.length; i++) {
  while (t.updates() < AT[i]) await tick();
  await t.shot("u" + String(AT[i]).padStart(3, "0"));
  taken.push(t.updates());
}
return { taken: taken, errors: t.errors.length };
