// moveloop.js: Bookworm Adventures played on the web by the port's own bot, the page's side of tools/movedriver.py. The
// test page runs it (?player=1&assets=1&driver=games/bookwormadv/tools/web/moveloop.js) as the body of an async function
// of t (byopop.test): it types ?name= into a fresh profile's new player dialog, sends the commands of ?first= (comma-separated, "goto menu,goto adventure"), then loops: state,
// move, the move as real DOM input (t.playMove), until the screen of ?until= is reached after leaving it, ?seconds= (600)
// are up, two minutes pass with no move, or the same move comes 200 times on an unchanged state. Unlike the built-in
// ?driver=move it waits through MOVE null (curtains, the enemy's turn, cutscenes). Returns {stop, moves, state, errors}.
const params = new URLSearchParams(location.search);
const until = params.get('until') || '-';
const seconds = Number(params.get('seconds')) || 600;
const first = (params.get('first') || '').split(',').map(s => s.trim()).filter(Boolean);
const t0 = Date.now();
const moves = {};
const firstFailed = [];
let stop = 'time up', left = false, nullsSince = null, last = null, same = 0, st = null;
// ?name=: a new profile's name, typed into the new player's dialog that a fresh profile opens on the title
if (params.get('name')) {
  for (let i = 0; i < 600 && (await t.state()).dialogs === 0; i++) await t.wait(10);
  await t.type(params.get('name'));
  await t.key('Enter');
}
for (const c of first) {
  // (one that fails is skipped: a profile in a chapter starts in it, with no menu to go to)
  const lines = await t.command(c, {timeout: 60000}).catch(e => [String(e)]);
  if (!/^OK /.test(lines[lines.length - 1])) firstFailed.push(lines[lines.length - 1]);
}
while (Date.now() - t0 < seconds * 1000) {
  st = await t.state();
  if (st.screen !== until) left = true;
  else if (left) { stop = 'reached ' + until; break; }
  const m = await t.botMove();
  if (m === null) {
    if (nullsSince === null) nullsSince = Date.now();
    else if (Date.now() - nullsSince > 120000) { stop = 'stuck: no move for 2 minutes'; break; }
    await t.wait(10);
    continue;
  }
  nullsSince = null;
  const key = JSON.stringify([st, m]);
  same = key === last ? same + 1 : 0;
  last = key;
  if (same >= 200) { stop = 'looping: ' + JSON.stringify(m); break; }
  moves[m.what] = (moves[m.what] || 0) + 1;
  await t.playMove(m);
  await t.wait(10);
}
// a picture of the end, build/webshots/<?run=>/end.png: a wait outside the page can watch for it
await t.shot('end').catch(() => null);
return {stop, firstFailed, moves, state: st, seconds: Math.round((Date.now() - t0) / 1000), errors: t.errors.length};
