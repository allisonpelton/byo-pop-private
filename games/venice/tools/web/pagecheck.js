// Venice Deluxe's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   venice.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/venice/tools/web/pagecheck.js&run=pagecheck
// A player named Test is made by the test command `player` (the name box's work), every building is unlocked and Journey's building 5
// is started; the game is in play once its level scene is up. The native answer to the focus line is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "player Test" --first "unlock all" --first "level 5"
//     build/native/venice.exe -userdir=build/pagecheck/venice/user
// The keys: the game takes the mouse only, apart from the name box's letters, Return and Backspace, Escape and Space (the pause menu),
// and the arrows and Return of its menus; the shared check already tries the keys a browser acts on.
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "player Test", "unlock all", "level 5"],
  playing: function (s) { return !!s.board && /level$/.test(s.screen || ""); },
  focus: { hidden: ["timer"], after: ["timer"] },
  keys: [" ", "Backspace"],
  point: [400, 300],
  moves: 0
});
