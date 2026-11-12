// A game that is started again while the full-screen help of the one before is still up (the test commands `goto classic` and
// `level` start a game at any time; the original's player cannot): the help must not be left under the new Board, where it is not
// drawn, takes no click and still pauses the game. The Site session's driver saw the bot answer "help" for good after such a restart.
// Parameters of the page: restarts= how many times `goto classic 1 0` is sent with the help up (2); moves= the bot's moves after
// the last one (40); win=1 also clears the first level with `win` once the bot has made two shots, and expects the gate to follow.
// Opened as .../astropop.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/astropop/tools/web/gotohelp.js&run=gotohelp
// The result: the bot's answers by kind for each part, and the first one that was "help" after the restarts' own, which must not be.
var params = new URLSearchParams(location.search);
var restarts = Number(params.get("restarts") || 2);
var moves = Number(params.get("moves") || 40);
var doWin = params.get("win") === "1";
var result = { restarts: [], kinds: {}, helpAnswers: 0, level: 0, stop: "" };

async function goto() {
  var lines = [];
  for (var i = 0; i < 400; i++) {
    lines = await t.command("goto classic 1 0");
    if (/^OK/.test(lines[lines.length - 1])) break;
    await t.wait(30);
  }
  if (!/^OK/.test(lines[lines.length - 1])) throw new Error("goto: " + lines[lines.length - 1]);
  await t.wait(10);
}

try {
  if (params.get("hold") === "1") {
    await t.profile.wipe();
    await t.start();
  }
  await t.ready;
  for (var r = 0; r < restarts; r++) {
    await goto();
    var m = await t.botMove();
    result.restarts.push(m && m.kind);       // "help": the first game's help, or the one the restart left up
  }
  var won = false, shots = 0, levelBefore = 0;
  for (var i = 0; i < moves; i++) {
    var s = await t.state();
    result.level = s.level;
    m = await t.botMove();
    if (m === null) { await t.wait(30); continue; }
    result.kinds[m.kind] = (result.kinds[m.kind] || 0) + 1;
    if (m.kind === "help") result.helpAnswers++;
    await t.playMove(m);
    if (m.kind === "shoot") shots++;
    if (doWin && !won && shots >= 2) {
      won = true;
      levelBefore = s.level;
      await t.command("win");
      await t.wait(20);
    }
  }
  var last = await t.state();
  result.level = last.level;
  result.paused = last.paused;
  result.stop = "done";
} catch (e) {
  result.stop = "error: " + String(e && e.message || e);
}
return result;
