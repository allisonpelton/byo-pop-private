// Clears levels of a classic game with the test command `win` and the port's own bot, by real DOM input only (docs/WEB_PLAYER.md,
// "Drivers"), as the Splits window's Any% check does (32 level clears in one game): `goto classic 1 <pilot>`, then the bot plays;
// once it has made `before` moves on a level, `win` clears the level, and the bot goes on through the level-up, the warp screen
// and its gate (and the boxes on the way) to the next level. Per level the result holds the bot's moves by kind and the updates it took.
// Parameters of the page: levels= how many levels to clear (3); pilot= 0 Vector (default), 1 Sprocket, 2 Turbot, 3 Vixx; before= the
// moves on a level before its `win` (3); moves= the most moves for the whole run (20000); charge=1 charges the weapon (`set charge`) on
// the first level, to see the bot start it; stop=clear ends the game at the `win` of level `levels` (as the Splits window's Any% check
// does, with the level-up and the warp still to come) and again=N then sends `goto classic 1 <pilot>` from that screen and plays N more
// bot moves of the second game (the result's `again`: the bot's answers in order, the state and what `win` answers after them);
// againat=<kind> restarts at the first bot answer of that kind instead (a dialog or a box up: upgrade, power, ...);
// unlock=1 sends `unlock all` first (the Site session's page does). Without hold=1 the page starts the game itself.
// Opened as .../astropop.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/astropop/tools/web/winlevels.js&run=winlevels&levels=3
var params = new URLSearchParams(location.search);
var wanted = Number(params.get("levels") || 3);
var pilot = Number(params.get("pilot") || 0);
var before = Number(params.get("before") || 3);
var maxMoves = Number(params.get("moves") || 20000);
var charge = params.get("charge") === "1";
var stopClear = params.get("stop") === "clear";
var unlock = params.get("unlock") === "1";
var againAt = params.get("againat") || "";   // restart (again=N) at the first bot answer of this kind (upgrade, power, gate...), not played
var againMoves = Number(params.get("again") || 0);
var result = { levels: [], kinds: {}, stop: "" };

try {
  if (params.get("hold") === "1") {
    await t.profile.wipe();
    await t.start();
  }
  await t.ready;
  var lines = [];
  if (unlock) {
    // `unlock all` needs a player: the first goto makes it, so one goto comes first (the game it starts is replaced by the next)
    for (var u = 0; u < 400; u++) {
      lines = await t.command("goto menu");
      if (/^OK/.test(lines[lines.length - 1])) break;
      await t.wait(30);
    }
    lines = await t.command("unlock all");
    if (!/^OK/.test(lines[lines.length - 1])) throw new Error("unlock: " + lines[lines.length - 1]);
  }
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
    if (againAt && m.kind === againAt) { result.stop = "at " + againAt; result.level = s.level; break; }
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
      // `win` is answered with an ERROR unless the level is in play: asked again after the next shot
      var w = await t.command("win");
      if (/^OK/.test(w[w.length - 1])) {
        won = true;
        if (stopClear && level === wanted) { result.stop = "clear"; break; }
        await t.wait(20);
      }
    }
  }
  if (!result.stop) result.stop = "limit";
  if (againMoves > 0) {
    // a second game, started from the first one's screen
    var again = { seq: [], goto: "" };
    var g2 = await t.command("goto classic 1 " + pilot);
    again.goto = g2[g2.length - 1];
    await t.wait(10);
    for (var j = 0; j < againMoves; j++) {
      var m2 = await t.botMove();
      if (m2 === null) { again.seq.push("null"); await t.wait(30); continue; }
      again.seq.push(m2.kind);
      await t.playMove(m2);
    }
    var s2 = await t.state();
    delete s2.field;
    again.state = s2;
    var w2 = await t.command("win");
    again.win = w2[w2.length - 1];
    var run = [], prev = "";
    again.seq.forEach(function (k) { if (k === prev) run[run.length - 1][1]++; else run.push([k, 1]); prev = k; });
    again.runs = run;
    delete again.seq;
    result.again = again;
  }
  result.moves = moves;
  result.state = await t.state();
  result.updates = t.updates();
  result.errors = t.errors.length;
} catch (e) {
  result.stop = "error: " + String(e && e.message || e);
}
return result;
