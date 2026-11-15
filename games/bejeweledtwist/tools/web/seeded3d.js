// The seeded 3D autoplay game in the player-only page, shot at given update counts, for comparison with the native port's
// shots at the same updates (IDENTICAL_PLAY.md, "3D on the web"). The page holds the start (?hold=1) so the driver can give
// the game the native run's arguments: -fast -3d -seed=1234 -autoplay=<mode>. ?mode=1 (Classic) or blitz; ?at=a,b,c the
// updates to shoot (each shot is named u<update>); ?state=1 gives the game's state there instead of a shot.
// Opened as .../bejeweledtwist.html?player=1&assets=1&silent=1&fast=1&hold=1&mode=1&at=3000,5330&run=w3d-classic
//   &driver=games/bejeweledtwist/tools/web/seeded3d.js
var q = new URLSearchParams(location.search);
var mode = q.get("mode") || "1";
var at = (q.get("at") || "").split(",").filter(Boolean).map(Number).sort(function (a, b) { return a - b; });
// the window's aspect is the screen's (WinTwistApp's Init): a 16:10 screen, as the native runs' desktop, so the game is
// 1228 by 768 there and here
Object.defineProperty(window.screen, "width", { get: function () { return 1920; } });
Object.defineProperty(window.screen, "height", { get: function () { return 1200; } });
window.ByopopShell.hooks.args = function () { return ["-fast", "-drawevery", "-3d", "-seed=1234", "-autoplay=" + mode]; };
// a first run, as the native runs' fresh user folder: a profile that has seen the first game's tips plays the same moves
// some 75 updates sooner. Its settings are the native runs' (ArtRes 768, windowed).
await t.profile.wipe();
await t.profile.seed({ "registry.ini": "[SOFTWARE\\Steam\\BejeweledTwist]\nScreenMode=dword:0\nArtRes=dword:768\n" });
await t.start();
var taken = [];
// -drawevery: -fast alone draws every tenth update, and a held page draws nothing new, so a shot could be of up to nine
// updates before the held one (where the browser frame's update budget ended); the native shots are the same either way.
// A native "<n> screenshot" runs at the start of update n, before the game's own update: the picture of n - 1 updates,
// which is where the page is held for the shot named n (a state is asked for at n, as the native "<n> state").
for (var i = 0; i < at.length; i++) {
  var stop = q.get("state") ? at[i] : at[i] - 1;
  while (t.updates() < stop) await t.step(stop - t.updates());
  if (q.get("state")) taken.push(await t.state());
  else { await t.shot("u" + String(at[i]).padStart(5, "0")); taken.push(t.updates()); }
}
return { taken: taken, errors: t.errors.length };
