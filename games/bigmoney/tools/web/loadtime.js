// A session driver (docs/WEB_PLAYER.md, "Drivers"): the time from the start of the page's navigation to the first screen that takes
// input, for GAME_NOTES.md "Performance". The body of an async function of t. The driver starts once the game is ready (its files are
// in); it then asks the bot's `move` every update until the answer is the title's play link (`title: play`, which the port gives once
// its loading bar is done). Returns {driverStartedMs, titleMs, updates}: milliseconds since the navigation started
// (performance.now()) when the driver began and when the link was first seen, and the updates run by then.
var started = Math.round(performance.now());
var found = null;
for (var i = 0; i < 6000 && found === null; i++) {
  var m = await t.botMove();
  if (m && /^title: play/.test(m.kind)) found = Math.round(performance.now());
  else await t.wait(1);
}
return { driverStartedMs: started, titleMs: found, updates: t.updates() };
