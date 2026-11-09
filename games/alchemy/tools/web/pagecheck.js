// Alchemy's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   alchemy.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/alchemy/tools/web/pagecheck.js&run=pagecheck
// The strategic game (no clock, so that a game is still in progress at the reload); "goto" waits for the title's link, which the bot's first move clicks. The native answer is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "goto strategic" --title-move build/native/alchemy.exe
//     -userdir=build/pagecheck/alchemy/user
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto strategic"],
  titleMove: true,
  focus: { hidden: ["active", "muted", "updates"], after: ["active", "muted", "updates"] },
  point: [400, 300],
  moves: 3
});
