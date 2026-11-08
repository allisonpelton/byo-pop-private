// moveloop.js: Bookworm Adventures played on the web by the port's own bot, the page's side of tools/movedriver.py. The
// test page runs it (?player=1&assets=1&driver=games/bookwormadv/tools/web/moveloop.js) as the body of an async function
// of t (byopop.test): it types ?name= into a fresh profile's new player dialog, sends the commands of ?first= (comma-separated, "goto menu,goto adventure"), then loops: state,
// move, the move as real DOM input (t.playMove; ?lockstep=1 holds the game and steps it; ?scale=2 draws the canvas at twice its size first), until the screen of ?until= is reached after leaving it, ?seconds= (600)
// are up, two minutes pass with no move, or the same move comes 200 times on an unchanged state. Unlike the built-in
// ?driver=move it waits through MOVE null (curtains, the enemy's turn, cutscenes). Returns {stop, moves, state, errors}.
const params = new URLSearchParams(location.search);
const until = params.get('until') || '-';
const seconds = Number(params.get('seconds')) || 600;
const first = (params.get('first') || '').split(',').map(s => s.trim()).filter(Boolean);
const t0 = Date.now();
const lockstep = params.get('lockstep') === '1';
const moves = {};
const firstFailed = [];
// a command that got no answer in time is asked again, thirty times at most in a row (on a hidden page an answer on the
// log was sometimes missed: the kit's waitLog timed out though the line had come)
let timeouts = 0, refocus = 0;
let held = false;  // (?lockstep=1's hold begins once the mini-game is up, and lifts for a curtain or a conversation: held, a
// curtain stopped the page's frames)
async function ask(f) {
  for (let i = 0; ; i++) {
    try {
      const p = f();
      if (!held) return await p;
      // a held game: a command waiting for its answer is helped along an update at a time (one stepped with it can go
      // unanswered). No timers while held: a hidden tab throttles them to one a minute
      let done = false;
      p.then(() => { done = true; }, () => { done = true; });
      await t.frame();
      for (let k = 0; k < 100 && !done; k++) await t.step(1);
      return await p;
    } catch (e) {
      timeouts++;
      if (i >= 29) throw e;
      if (held) await t.step(10); else await t.wait(10);
    }
  }
}
let stop = 'time up', left = false, nullsSince = null, last = null, same = 0, st = null;
// ?scale=2: the canvas drawn at twice its size (CSS), so that the DOM events land at scaled points as in full screen, which
// the Browser pane cannot enter
const scale = Number(params.get('scale') || 1);
let scaled = null;
if (scale !== 1) {
  const cv = document.getElementById('canvas');
  cv.style.width = Math.round(800 * scale) + 'px';
  cv.style.height = Math.round(600 * scale) + 'px';
  await t.wait(20);
  scaled = Math.round(cv.getBoundingClientRect().width) + 'x' + Math.round(cv.getBoundingClientRect().height);
}
// ?name=: a new profile's name, typed into the new player's dialog that a fresh profile opens on the title
if (params.get('name')) {
  for (let i = 0; i < 600 && (await t.state()).dialogs === 0; i++) await t.wait(10);
  await t.type(params.get('name'));
  await t.key('Enter');
}
for (const [i, c] of first.entries()) {
  // between two of them the conversations and dialogs that came are clicked away by the bot's moves, as a player does
  // before going on (a goto over Lex's unlock news on the menu left his panel active behind the next screen, and its curtain stuck)
  for (let k = 0; i > 0 && k < 300; k++) {
    const s = await ask(() => t.state());
    if (s.curtain) { await t.wait(10); continue; }
    if (!s.talk && !s.dialogs) break;
    const m = await ask(() => t.botMove());
    if (m) await t.playMove(m);
    await t.wait(10);
  }
  // (one that fails is skipped: a profile in a chapter starts in it, with no menu to go to)
  const lines = await t.command(c, {timeout: 60000}).catch(e => [String(e)]);
  if (!/^OK /.test(lines[lines.length - 1])) firstFailed.push(lines[lines.length - 1]);
}
// ?lockstep=1: the game held between updates (t.pause), stepped only by the moves' own waits and 30 updates after each move,
// 10 while there is none: on a fast page a mini-game's clock would run out between two questions
async function idle(n) { if (held) await t.step(n); else await t.wait(n); }
async function play(m) {
  if (!held) return t.playMove(m);
  for (const s of m.input) {
    if (s.click) await t.click(s.click[0], s.click[1], s.button);
    else if (s.move) await t.move(s.move[0], s.move[1]);
    else if (s.key) await t.key(s.key);
    else if (s.wait) await t.step(s.wait);
  }
}
while (Date.now() - t0 < seconds * 1000) {
  st = await ask(() => t.state());
  if (lockstep && !held && st.screen === 'minigame' && !st.curtain && !st.talk) {
    await t.pause();
    held = true;
  } else if (held && (st.screen !== 'minigame' || st.curtain || st.talk)) {
    await t.resume();
    held = false;
  }
  if (st.screen !== until) left = true;
  else if (left) { stop = 'reached ' + until; break; }
  const m = await ask(() => t.botMove());
  if (m === null && st.paused && !st.dialogs && !st.talk && st.screen === 'battle') {
    // the focus-loss pause ("Game Paused"): the page lost the focus (a hidden pane can), and a player brings it back
    refocus++;
    await t.focus();
    await idle(10);
    continue;
  }
  if (m === null) {
    if (nullsSince === null) nullsSince = Date.now();
    else if (Date.now() - nullsSince > 120000) { stop = 'stuck: no move for 2 minutes'; break; }
    await idle(10);
    continue;
  }
  nullsSince = null;
  const key = JSON.stringify([st, m]);
  same = key === last ? same + 1 : 0;
  last = key;
  if (same >= 200) { stop = 'looping: ' + JSON.stringify(m); break; }
  moves[m.what] = (moves[m.what] || 0) + 1;
  await play(m);
  await idle(lockstep ? 30 : 10);
}
if (held) await t.resume();
// a picture of the end, build/webshots/<?run=>/end.png: a wait outside the page can watch for it
await t.shot('end').catch(() => null);
return {stop, timeouts, refocus, scaled, firstFailed, moves, state: st, seconds: Math.round((Date.now() - t0) / 1000), errors: t.errors.length};
