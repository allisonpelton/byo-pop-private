// Water Bugs' table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   waterbugs.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/waterbugs/tools/web/pagecheck.js&run=pagecheck
// The first board of the campaign, started by the test command ("level 1": as the menu's Start reaches it, at the story screen). The
// game is in play once the story or the board is up; the bot's moves before the reload press the story's button and play the board. The native answer to the focus line is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "level 1" build/native/waterbugs.exe
//     -userdir=build/pagecheck/waterbugs/user
// The keys the game reads are the function keys the engine takes (F5 sound, F6 music, F7 full screen, F8 centred), Escape (quit) and
// Return; the shared check lists the keys a browser acts on, and tells which of the function keys it cancels, as information.
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "level 1"],
  playing: function (s) { return s.screen === "story" || s.screen === "board"; },   // "level 1" stops at the story screen, whose button the bot's first move presses
  focus: { hidden: ["frame", "update"], after: ["frame", "update"] },
  point: [400, 300],
  moves: 3
});
