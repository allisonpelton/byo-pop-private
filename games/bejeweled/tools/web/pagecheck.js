// Bejeweled's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   bejeweled.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/bejeweled/tools/web/pagecheck.js&run=pagecheck
// The timed mode, so that the clock would show a page that runs on while hidden. The native answer is
//   python tools/web/pagecheck_native.py --first "seed 1" --first "goto timed" build/native/bejeweled.exe
//     -userdir=build/pagecheck/bejeweled/user
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto timed"],
  focus: { hidden: ["paused", "tick"], after: ["busy", "paused", "phase", "tick", "tutorial"] },
  point: [400, 240],
  moves: 3
});
