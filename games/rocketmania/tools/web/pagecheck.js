// Rocket Mania's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   rocketmania.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/rocketmania/tools/web/pagecheck.js&run=pagecheck
// The arcade game, difficulty 2 (the loading screen's play link is clicked by the bot's first move, "titleMove"). The native answer is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "goto arcade 2" --title-move build/native/rocketmania.exe
//     -userdir=build/pagecheck/rocketmania/user
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto arcade 2"],
  titleMove: true,
  ready: function (s) { return !!s.screen; },   // the loading screen stays up until its play link is clicked
  playing: function (s) { return s.screen === "board"; },
  focus: { hidden: ["paused"], after: ["board", "paused"] },
  moves: 3
});
