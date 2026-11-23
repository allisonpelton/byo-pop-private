// Bejeweled 3's web real-input run of one mode (docs/WEB_PLAYER.md, "Drivers"): the web twin of tools/realinput.sh. From a
// wiped test profile the game starts on its new player's name box, and the port's bot (TestCommands.cpp's move, played as
// DOM input by playByMove) types the name, clicks PLAY and the mode's ball (or the quest's button on the map) and plays the
// board to its end. Opened as
//   bejeweled3.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/bejeweled3/tools/web/playmode.js&mode=<mode>
//     &run=playmode-<mode>
// with <mode> classic, zen, lightning, quest, butterflies, poker, icestorm or diamondmine. The result: playByMove's
// {moves, stop, state, updates, errors} with the mode; the end's shot is build/webshots/<run>/end.png.
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "classic";
// as tools/realinput.sh: Stratamax for Quest (20 moves, a loss); Classic given a board with no move after 40 moves and Ice
// Storm left idle after 60 (their bots never lose); Zen has no end, so its step limit ends it
var QUEST_INDEX = 3;
var NOMOVE = "RGBYPORW GBYPORWG BYPORWGB YPORWGBY PORWGBYP ORWGBYPO RWGBYPOR WGBYPORW";
var ATMOVE = { classic: "40 board " + NOMOVE, icestorm: "60 idle" };
var STEPS = { zen: 400 };

await t.profile.wipe();
await t.start();
for (var i = 0; i < 600; i++) {
  try { if (await t.state()) break; } catch (e) { /* not up yet */ }
  await t.wait(10);
}
await t.command("seed 1");
await t.command("unlock all");
await t.command(mode === "quest" ? "play quest " + QUEST_INDEX : "play " + mode);
if (ATMOVE[mode]) await t.command("atmove " + ATMOVE[mode]);
var result = await t.playByMove({ steps: STEPS[mode] || 3000 });
await t.wait(60);
await t.shot("end");
result.mode = mode;
return result;
