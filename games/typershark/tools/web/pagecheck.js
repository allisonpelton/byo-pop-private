// Typer Shark's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   typershark.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/typershark/tools/web/pagecheck.js&run=pagecheck
// An Easy adventure started by the test command (a player named Tester is made, the game's own buttons are pressed). The
// game is in play once the board is up, with the first hint over it or without. The native answer to the focus line is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "goto adventure 0" build/native/typershark.exe
//     -userdir=build/pagecheck/typershark/user
// The keys: the game takes every printing key (the words, the chests' letters on the map), Return (the zapper), the space
// bar (pause, the map, a hint), Escape and Backspace; the shared check already tries the keys a browser acts on.
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto adventure 0"],
  playing: function (s) { return s.screen === "board" || s.base === "board"; },
  focus: { hidden: ["paused", "timer"], after: ["base", "dialogHeader", "dialogLines", "emp", "screen"] },
  keys: ["Enter", " ", "Escape", "Backspace", "'", "/", ";", ",", "."],
  point: [320, 240],
  moves: 3
});
