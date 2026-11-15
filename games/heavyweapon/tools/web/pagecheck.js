// Heavy Weapon's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   heavyweapon.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/heavyweapon/tools/web/pagecheck.js&run=pagecheck
// Level 1 of a campaign, started by the test command once the title has loaded (the game makes a player named Test). The
// native answer to the focus line is
//   python tools/web/pagecheck_native.py --title-move --first "seed 1" --first "level 1" build/native/heavyweapon.exe
//     -userdir=build/pagecheck/heavyweapon/user
// (the focus lost pauses the game, as the original's LostFocus does; regained, it stays paused until a click). The keys: the
// game takes Space (pause), Backspace (a player's name) and the printing keys (the board listens for a typed code); the
// shared check already tries the keys a browser acts on. Return and Escape stay out of the list, as Typer Shark's table has
// them.
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "level 1"],
  titleMove: true,
  playing: function (s) { return s.screen === "game"; },
  focus: { hidden: ["appupdates", "paused", "updates"], after: ["appupdates", "updates"] },
  keys: [" ", "Backspace", "'", "/"],
  point: [320, 240],
  moves: 3
});
