// Pizza Frenzy's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"),
// opened as
//   pizzafrenzy.html?player=1&assets=1&hold=1&fast=1&silent=1
//     &driver=games/pizzafrenzy/tools/web/pagecheck.js&run=pagecheck
// The fresh test profile opens on the new player's name box: the start commands are refused until the bot's moves have
// named a player, then the speed mode's first level starts on First Hill's story, which the bot's own move (Escape, as
// the story's skip hint says) passes. The native answer to the focus line is
//   python tools/web/pagecheck_native.py --title-move --first "seed 1" --first "level 1 1" --first "wait 200"
//     --first "key Escape" --settle 200 build/native/pizzafrenzy.exe -gamedir=assets/pizzafrenzy
//     -userdir=build/pagecheck/pizzafrenzy/user
// (the focus lost shows the pause menu and stops the clock; regained, the game stays paused). The keys: the game
// takes the mouse, the name boxes' letters, Backspace and Return, Escape (the story's skip, the pause menu) and F (the
// story's fun facts); the shared check already tries the keys a browser acts on.
var moving = false;
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "level 1 1"],
  titleMove: true,
  playing: function (s) {
    if (s.screen === "story" && !moving) {
      moving = true;
      t.botMove().then(function (m) { return m ? t.playMove(m) : null; }).then(function () { moving = false; },
        function () { moving = false; });
    }
    return s.screen === "game";
  },
  settle: 200,
  focus: { hidden: ["board", "frame", "screen", "timer"], after: ["frame"] },
  keys: ["Backspace", "f"],
  point: [400, 300],
  moves: 5
});
