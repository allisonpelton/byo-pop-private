// Bejeweled 2's table for the shared page check (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check"), opened as
//   bejeweled2.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/bejeweled2/tools/web/pagecheck.js&run=pagecheck
// Action, the timed mode, so that the clock would show a page that runs on while hidden. The first game of a new player opens
// the How To Play screen in front of the board, which `goto` leaves up (the bot's move closes it): the start waits for it
// and closes it with the Escape key, at the same updates in the page and in the native run, which hold lines the updates up.
// The hidden and shown windows are 60 updates, short of the one-time hint that the idle board opens a little later. The native
// answer is
//   python tools/web/pagecheck_native.py --settle 100 --updates 60 --first "seed 1" --first "goto action" --first "wait 900"
//     --first "key Escape" --first "wait 150" build/native/bejeweled2.exe -userdir=build/pagecheck/bejeweled2/user
return await t.run("tools/web/pagecheck.js", {
  start: ["seed 1", "goto action", "wait 900", "key Escape", "wait 150"],
  focus: {
    hidden: ["busy", "focus", "paused", "tick", "updates"],
    after: ["busy", "focus", "paused", "tick", "updates"]
  },
  focusUpdates: 60,
  moves: 3
});
