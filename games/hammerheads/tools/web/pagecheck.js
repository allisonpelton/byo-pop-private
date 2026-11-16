// Hammer Heads' table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   hammerheads.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/hammerheads/tools/web/pagecheck.js&run=pagecheck
// The game waits on its loading screen for a click on "Click to Play" and, for a new profile, for a name in the name box, so the
// table's ready() plays those two once (a click, the letters of "Test" and OK) and answers when the main menu is up. Classic's
// first level is then started by the test command. The native answer to the focus line is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "goto classic" build/native/hammerheads.exe
//     -userdir=build/pagecheck/hammerheads/user
// (the pause count changes while the focus is away and again when it is back). The game takes the printing keys (the cheat words
// and a player's name) and Backspace; the shared check already tries the keys a browser acts on.
var booted = null;
function boot() {
  var b = { done: false };
  (async function () {
    var s = await t.state();
    if (s.screen === "loading") {
      await t.wait(700);                       // the loading screen's button waits for a click from about update 650
      await t.click(397, 532);
      await t.wait(300);
      s = await t.state();
      if (s.dialogs && s.dialogs.length) {     // a new profile's name box
        for (var c of "Test") { await t.command("char " + c); await t.wait(10); }
        await t.wait(50);
        await t.click(310, 392);
        await t.wait(300);
      }
    }
    b.done = true;
  })();
  return b;
}
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto classic"],
  ready: function (s) { if (!booted) booted = boot(); return booted.done; },
  playing: function (s) { return s.screen === "game"; },
  focus: { hidden: ["paused"], after: ["paused"] },
  keys: ["Backspace", "'", "/"],
  point: [400, 300],
  moves: 3
});
