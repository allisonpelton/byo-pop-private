// AstroPop's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   astropop.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/astropop/tools/web/pagecheck.js&run=pagecheck
// A classic game from level 1, started by the test command (the game makes a player named Test). The native answer to the focus
// line is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "goto classic 1" build/native/astropop.exe
//     -userdir=build/pagecheck/astropop/user
// The keys: the game takes Space (pause), Backspace (a player's name) and the printing keys; the shared check already tries the
// keys a browser acts on. Return and Escape stay out of the list, as Typer Shark's table has them (the shell does not cancel
// their keydown).
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto classic 1"],
  playing: function (s) { return s.screen === "game"; },
  focus: { hidden: ["appupdates", "paused", "updates"], after: ["appupdates", "paused", "updates"] },
  keys: [" ", "Backspace", "'", "/"],
  point: [320, 240],
  moves: 3
});
