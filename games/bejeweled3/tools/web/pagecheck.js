// Bejeweled 3's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"),
// opened as
//   bejeweled3.html?player=1&assets=1&hold=1&fast=1&silent=1
//     &driver=games/bejeweled3/tools/web/pagecheck.js&run=pagecheck
// The fresh test profile opens on the new player's name box: `name Tester` answers it, `hints off` turns the tutorial boxes and the first game's help dialog
// off (the profile's one switch, else the board waits behind the help dialog and no key changes), and the menus' intro runs 1200 updates before "goto classic"
// starts the game. The native answer to the focus line is
//   python tools/web/pagecheck_native.py --title-move --first "seed 1" --first "name Tester" --first "hints off" --first "wait 1200"
//     --first "goto classic" --settle 400 build/native/bejeweled3.exe -gamedir=assets/bejeweled3 -userdir=build/pagecheck/bejeweled3/user
// (400 updates: the first fill has landed by then). Classic's board has no clock and cannot be paused (Board::CanPause, 0x73c220: the mode has a time limit), so the
// exe's focus handling leaves it running: the update count and the hint timer go on while the focus is away and after it comes back. A mode with a clock ("goto lightning")
// pauses: its game ticks and clock stand still once the pause's fade (state's pausefade) is 0.75 (NOTES.md, "Focus").
// The keys: the game takes the mouse, the name box's letters, Backspace and Return.
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "name Tester", "hints off", "wait 1200", "goto classic"],
  titleMove: true,
  playing: function (s) { return s.screen === "classic"; },
  settle: 400,
  focus: { hidden: ["hinttimer", "update"], after: ["hinttimer", "update"] },
  keys: ["Backspace"],
  moves: 5
});
