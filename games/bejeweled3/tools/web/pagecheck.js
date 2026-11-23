// Bejeweled 3's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"),
// opened as
//   bejeweled3.html?player=1&assets=1&hold=1&fast=1&silent=1
//     &driver=games/bejeweled3/tools/web/pagecheck.js&run=pagecheck
// The fresh test profile opens on the new player's name box: "goto classic" is refused until the bot's move (the
// name's keys and Enter, TestCommands.cpp's move) has named a player, then Classic starts. The native answer to the focus
// line is
//   python tools/web/pagecheck_native.py --title-move --first "seed 1" --first "goto classic" --settle 400
//     build/native/bejeweled3.exe -gamedir=assets/bejeweled3 -userdir=build/pagecheck/bejeweled3/user
// (400 updates: the first fill has landed by then; the port keeps the board running while the focus is away: the exe's focus handling is not translated yet, NOTES.md).
// The keys: the game takes the mouse, the name box's letters, Backspace and Return.
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto classic"],
  titleMove: true,
  playing: function (s) { return s.screen === "classic"; },
  settle: 400,
  focus: { hidden: ["hinttimer", "update"], after: ["hinttimer", "update"] },
  keys: ["Backspace"],
  moves: 5
});
