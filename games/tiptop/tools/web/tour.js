// A tour of TipTop's screens as real DOM input (the player-only page, docs/WEB_PLAYER.md, "Drivers"): the body of an async
// function of byopop.test (t). It replays one of the phase-two tours (games/tiptop/tools/shots/tour<N>.txt, the port's -script
// format; tours.js holds them, made by mktours.py) as DOM events on the canvas, each line when the game's update count reaches its number, and returns what happened:
//
//   http://127.0.0.1:18781/tiptop.html?player=1&assets=1&silent=1&fast=1&tour=1&driver=games/tiptop/tools/web/tour.js&run=tour1
//
// Lines: click <x> <y>, move <x> <y>, key <NAME>, char <c> (typed), focus lose|regain, screenshot <name> (t.shot), quit (the
// end). The cursor line the port's scripts put before a click (move at K-1) is kept: the move is a DOM mouse move. A click on
// the options screen's publisher link is never in a tour.
var params = new URLSearchParams(location.search);
var tour = params.get("tour") || "1";
var res = await fetch("/__repo/games/tiptop/tools/web/tours.js");
if (!res.ok) throw new Error("tours.js: " + res.status);
var text = new Function((await res.text()) + "; return TOURS;")()[tour];
if (!text) throw new Error("no tour " + tour);
var lines = text.split(/\r?\n/).map(function (l) { return l.trim(); }).filter(function (l) { return l && l[0] !== "#"; });
var done = [];
for (var i = 0; i < lines.length; i++) {
  var m = /^(\d+)\s+(\S+)\s*(.*)$/.exec(lines[i]);
  if (!m) continue;
  var at = parseInt(m[1], 10), cmd = m[2], args = m[3].split(/\s+/).filter(Boolean);
  var d = at - t.updates();
  if (d > 0) await t.wait(d);
  if (cmd === "click") await t.click(+args[0], +args[1]);
  else if (cmd === "move") await t.move(+args[0], +args[1]);
  else if (cmd === "key") await t.key(args[0]);
  else if (cmd === "char") await t.type(args[0]);
  else if (cmd === "focus") await (args[0] === "lose" ? t.blur() : t.focus());
  else if (cmd === "screenshot") await t.shot(args[0]);
  else if (cmd === "quit") break;
  done.push(at + " " + cmd);
}
return { tour: tour, lines: done.length, updates: t.updates(), errors: t.errors.length };
