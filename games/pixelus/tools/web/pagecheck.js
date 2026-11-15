// Pixelus' table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   pixelus.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/pixelus/tools/web/pagecheck.js&run=pagecheck
// The game starts on the loading screen's play link (titleMove), then `goto normal` makes a player and opens the first
// puzzle after the tutorial; it is in play once the board is up. The native answer to the focus line is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "goto normal" --title-move
//     build/native/pixelus.exe -userdir=build/pagecheck/pixelus/user
// The keys: the game takes the mouse only, apart from the name box's letters, Return and Backspace, and Escape (the pause screen);
// the shared check already tries the keys a browser acts on.
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto normal"],
  titleMove: true,
  ready: function (s) { return !!s.ready; },
  playing: function (s) { return s.screen === "board" && !!s.board; },
  focus: { hidden: ["timer"], after: ["timer"] },
  keys: ["Backspace"],
  moves: 2
});
