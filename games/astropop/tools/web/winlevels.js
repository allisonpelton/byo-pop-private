// Clears levels of a classic game with the test command `win` and the port's own bot, by real DOM input only (docs/WEB_PLAYER.md,
// "Drivers"), as the Splits window's Any% check does (32 level clears in one game): `goto classic 1 <pilot>`, then the bot plays;
// once it has made `before` moves on a level, `win` clears the level, and the bot goes on through the level-up, the warp screen
// and its gate (and the boxes on the way) to the next level. Per level the result holds the bot's moves by kind and the updates it took.
// Parameters of the page: levels= how many levels to clear (3); pilot= 0 Vector (default), 1 Sprocket, 2 Turbot, 3 Vixx; before= the
// moves on a level before its `win` (3); moves= the most moves for the whole run (20000); charge=1 charges the weapon (`set charge`) on
// the first level, to see the bot start it.
// Opened as .../astropop.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/astropop/tools/web/winlevels.js&run=winlevels&levels=3
var params = new URLSearchParams(location.search);
var wanted = Number(params.get("levels") || 3);
var pilot = Number(params.get("pilot") || 0);
var before = Number(params.get("before") || 3);
var maxMoves = Number(params.get("moves") || 20000);
var charge = params.get("charge") === "1";
var result = { levels: [], kinds: {}, stop: "" };

try {
  if (params.get("hold") === "1") {
    await t.profile.wipe();
    await t.start();
  }
  await t.ready;
  var lines = [];
  for (var i = 0; i < 400; i++) {
    lines = await t.command("goto classic 1 " + pilot);
    if (/^OK/.test(lines[lines.length - 1])) break;
    await t.wait(30);
  }
  if (!/^OK/.test(lines[lines.length - 1])) throw new Error("goto: " + lines[lines.length - 1]);
  await t.wait(10);

  var level = 1, levelStart = t.updates(), onLevel = 0, won = false, kinds = {}, moves = 0, nulls = 0;
  while (moves < maxMoves) {
    var s = await t.state();
    if (s.over) { result.stop = "over"; break; }
    if (s.level !== level) {
      result.levels.push({ level: level, moves: onLevel, updates: t.updates() - levelStart, kinds: kinds });
      level = s.level; levelStart = t.updates(); onLevel = 0; won = false; kinds = {};
      if (level > wanted) { result.stop = "levels"; break; }
    }
    var m = await t.botMove();
    if (m === null) {
      if (++nulls >= 40) { result.stop = "no move"; break; }
      await t.wait(30);
      continue;
    }
    nulls = 0;
    kinds[m.kind] = (kinds[m.kind] || 0) + 1;
    result.kinds[m.kind] = (result.kinds[m.kind] || 0) + 1;
    moves++;
    onLevel++;
    await t.playMove(m);
    if (charge && level === 1 && onLevel === before) {
      await t.command("set charge");
      await t.wait(5);
    }
    if (!won && !charge && m.kind === "shoot" && onLevel >= before) {
      won = true;
      await t.command("win");
      await t.wait(20);
    }
  }
  if (!result.stop) result.stop = "limit";
  result.moves = moves;
  result.state = await t.state();
  result.updates = t.updates();
  result.errors = t.errors.length;
} catch (e) {
  result.stop = "error: " + String(e && e.message || e);
}
return result;
