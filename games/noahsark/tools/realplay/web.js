// Plays Noah's Ark in the browser with real DOM input only (CLAUDE.md, "Done means identical"): code plays, the session judges.
// The body of an async function of t (byopop.test), run once the game is ready: tools/webtest/serve.py serves it when the page is
//   http://127.0.0.1:<port>/noahsark.html?player=1&assets=1&silent=1&driver=games/noahsark/tools/realplay/web.js&mode=action
// Parameters: mode=action|strategy|puzzle (action), diff=easy|medium|hard (easy), steps=N moves at most (6000), finish=1 (one
// more move after the list, the click on its link back to the menu), blurAt=N (after N moves the focus is lost for 200 updates and
// regained; hidden=1 sends the visibility change too), stuck=N updates without a change of the screen and the board (20000),
// loseLevel=N (the bot wins every level of an easy game, so a whole game of the action and strategy modes would not end: once the
// game is on level N and playing, the test command `lose` brings the water to the ark, and the game-over screen, the name entry and
// the list are played as any other screen).
// stepped=1 holds the game between the driver's calls (t.pause, t.step): with ?fast=1 on the page the game would otherwise run
// on while the driver asks, and its water would drown it; each move's waits are then steps of the game's updates (the input still
// reaches the canvas as DOM events).
// The moves are the game's own bot's (`move`: the title's link, the menu's button, the difficulty, the tutorial, the level-up
// panel, the name entry, and on the board the pair, the swap or the group to click) and are sent as DOM events on the canvas. The
// result (byopop.test.result) is { moves, stop, kinds, screens, state, errors, updates }.
const q = new URLSearchParams(location.search);
const steps = Number(q.get("steps")) || 6000;
const stuckAfter = Number(q.get("stuck")) || 20000;
const blurAt = Number(q.get("blurAt")) || 0;
const loseLevel = Number(q.get("loseLevel")) || 0;
const stepped = q.get("stepped") === "1";
const idle = (n) => (stepped ? t.step(n) : t.wait(n));
// one move: its input in order, then its updates; in a held game the waits are steps
async function play(m) {
  if (!stepped) return t.playMove(m);
  for (const s of m.input || []) {
    if (s.move) await t.move(s.move[0], s.move[1]);
    else if (s.click) await t.click(s.click[0], s.click[1]);
    else if (s.key) await (s.key.length === 1 ? t.type(s.key) : t.key(s.key));
    else if (s.wait) await t.step(s.wait);
    else throw new Error("unknown input step " + JSON.stringify(s));
  }
  await t.step(m.updates > 0 ? m.updates : 1);
}
if (stepped) await t.pause();
for (const line of ["set mode " + (q.get("mode") || "action"), "set difficulty " + (q.get("diff") || "easy")]) {
  const a = (await t.command(line)).slice(-1)[0];       // the answer is the last of the lines printed
  if (!/^OK/.test(a)) return { stop: "error: " + line + " -> " + a };
}
const screens = [], kinds = {};
let moves = 0, nulls = 0, stop = null, st = null, sig = null, sigAt = 0, blurred = 0, lost = null, focusInfo = null;
const t0 = Date.now();
for (;;) {
  st = await t.state();
  if (!screens.length || screens[screens.length - 1][0] !== st.screen) screens.push([st.screen, st.update, st.mode || null, st.level || null]);
  if (st.over) { stop = "over"; break; }
  if (loseLevel && !lost && st.screen === "game" && st.phase === "playing" && (st.mode === "action" || st.mode === "strategy") && (st.level || 0) >= loseLevel) {
    lost = (await t.command("lose")).slice(-1)[0] + " at update " + st.update + ", level " + st.level;
    continue;
  }
  if (moves >= steps) { stop = "steps"; break; }
  const s = JSON.stringify(st, (k, v) => (k === "update" || k === "water" ? undefined : v));
  if (s !== sig) { sig = s; sigAt = st.update; }
  else if (st.update - sigAt > stuckAfter) { stop = "stuck: " + s.slice(0, 300); break; }
  if (blurAt && !blurred && moves >= blurAt) {
    blurred = 1;
    const u0 = t.updates();
    await t.blur(q.get("hidden") === "1" ? { hidden: true } : undefined);
    await idle(200);
    const during = t.updates() - u0;
    await t.focus();
    screens.push(["blur: updates while the focus was lost", during, null, null]);
    // the game must have its focus back (state.focus) within a few frames, without a second call
    let f = await t.state(), tries = 0;
    while (f.screen === "game" && f.focus === false && tries < 10) { await idle(20); f = await t.state(); tries++; }
    focusInfo = { screen: f.screen, phase: f.phase, focus: f.focus, tries };
    if (f.screen === "game" && f.focus === false) { await t.focus(); focusInfo.neededSecondFocus = true; }
  }
  const m = await t.botMove();
  if (m === null) {
    if (++nulls >= 3) { stop = "no move: " + JSON.stringify(st).slice(0, 300); break; }
    await idle(30);
    continue;
  }
  nulls = 0;
  kinds[m.kind] = (kinds[m.kind] || 0) + 1;
  moves++;
  await play(m);
}
let after = null;
if (q.get("finish") === "1" && stop === "over") {
  const m = await t.botMove();
  if (m !== null) { await play(m); await idle(60); after = (await t.state()).screen; }
}
return { moves, stop, lose: lost, focus: focusInfo, kinds, screens, state: st, after, errors: t.errors.length, errorList: t.errors.slice(0, 5), updates: t.updates(), seconds: Math.round((Date.now() - t0) / 100) / 10 };
