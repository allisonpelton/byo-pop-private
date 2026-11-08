// A session driver (docs/WEB_PLAYER.md, "Drivers"): plays the games of ?plan= by the port's own bot (the `move` test command,
// docs/TEST_COMMANDS.md), every move sent as real DOM events through byopop.test (click, key). The body of an async function
// of t. Parameters, from the page's own address:
//   plan=action:1,strategy:2,puzzle:1   the games to play, in order (a mode and its skill); default action:1. Before each
//                                       game `set mode` and `set skill` tell the selector's bot what to pick; a game of another
//                                       mode or skill than the one before goes back to the selector with `goto selector`, the
//                                       same one starts again by "play again"
//   cap=12000                           updates a game is played to win; then `set bot lose` has the bot play to lose, so that
//                                       the game ends by itself (0: never)
//   steps=N                             stop after N moves (0: play the plan to its end)
//   blurat=N,M                          after these moves lose the focus (byopop.test.blur), let a moment pass and regain it
//   maxseconds=1800                     stop after so long a time
//   scale=2                             draw the canvas at twice its size (CSS), so that the DOM events land at scaled points as they do
//                                       in full screen, which the Browser pane cannot enter (as Bejeweled's onegame.js does)
// Returns {stop, moves, kinds, games: [{mode, skill, level, score, updates}], blurs, scale, state, updates, errors, seconds}; stop is
// "over" (the plan was played), "steps", "no move", "timeout" or an error.
var q = new URLSearchParams(location.search);
var plan = (q.get("plan") || "action:1").split(",").map(function (x) {
  var p = x.split(":");
  return { mode: p[0], skill: Number(p[1] || 1) };
});
var cap = q.has("cap") ? Number(q.get("cap")) : 12000;
var maxSteps = Number(q.get("steps")) || 0;
var blurAt = (q.get("blurat") || "").split(",").filter(Boolean).map(Number);
var maxSeconds = Number(q.get("maxseconds")) || 1800;
var scale = Number(q.get("scale") || 1);
var t0 = Date.now();

async function ok(line) {
  var lines = await t.command(line);
  var last = lines[lines.length - 1];
  if (!/^OK /.test(last)) throw new Error(line + " -> " + last);
}
function pickPlan(i) { return ok("set mode " + plan[i].mode).then(function () { return ok("set skill " + plan[i].skill); }); }

var result = { stop: null, moves: 0, kinds: {}, games: [], blurs: 0 };
var n = 0, gameNo = 0, counted = false, losing = false, gameStart = null, nulls = 0;
if (scale !== 1) {
  var cv = document.getElementById("canvas");
  cv.style.width = Math.round(640 * scale) + "px";
  cv.style.height = Math.round(480 * scale) + "px";
  await t.wait(20);
  result.scale = { css: Math.round(cv.getBoundingClientRect().width) + "x" + Math.round(cv.getBoundingClientRect().height) };
}
await pickPlan(0);
try {
  for (;;) {
    var s = await t.state();
    if (s.screen === "board") {
      if (gameStart === null) gameStart = s.updates;
      if (cap && !losing && s.updates - gameStart > cap) { await ok("set bot lose"); losing = true; }
    }
    if (s.over) {
      if (!counted) {
        counted = true;
        result.games.push({ mode: s.mode, skill: s.skill, level: s.level, score: s.score, updates: s.updates });
        gameNo++;
        gameStart = null;
        if (losing) { await ok("set bot win"); losing = false; }
        if (gameNo >= plan.length) { result.stop = "over"; break; }
        await pickPlan(gameNo);
        if (plan[gameNo].mode !== s.mode || plan[gameNo].skill !== s.skill) { await ok("goto selector"); continue; }
      }
    } else counted = false;
    if (maxSteps && n >= maxSteps) { result.stop = "steps"; break; }
    if (Date.now() - t0 > maxSeconds * 1000) { result.stop = "timeout"; break; }
    if (blurAt.indexOf(n) >= 0) {
      blurAt.splice(blurAt.indexOf(n), 1);
      await t.blur();
      await Promise.race([t.wait(100), new Promise(function (r) { setTimeout(r, 3000); })]);
      await t.focus();
      result.blurs++;
    }
    var m = await t.botMove();
    if (m === null) {
      if (++nulls >= 3) { result.stop = "no move"; break; }
      await t.wait(30);
      continue;
    }
    nulls = 0;
    n++;
    var kind = m.kind.split(":")[0].split(" ")[0];
    result.kinds[kind] = (result.kinds[kind] || 0) + 1;
    await t.playMove(m);
  }
} catch (e) {
  result.stop = "error: " + String(e && e.message || e);
}
result.moves = n;
try { result.state = await t.state(); delete result.state.board; } catch (e) { /* the game may be gone */ }
result.updates = t.updates();
result.errors = t.errors.length;
result.seconds = Math.round((Date.now() - t0) / 100) / 10;
return result;
