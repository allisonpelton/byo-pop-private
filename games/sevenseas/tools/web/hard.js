// A whole Hard game by real DOM input, every move taken from the port's own bot (`move`, docs/TEST_COMMANDS.md): the title's
// Play link, the menu's Hard wheel and Play button (their positions come from `state`'s "menu"), then every broadside, sail,
// dialog button and bonus dig as the click `move` names, until the state says the game is over. Run in the test page:
//   sevenseas.html?player=1&assets=1&fast=1&driver=games/sevenseas/tools/web/hard.js&run=hard   (tools/build.sh web-test)
// The body of an async function of t (byopop.test). A `move` answer is sometimes missed by the kit's wait at fast speed, so
// each ask has a short timeout and is repeated.
var ask = async function () {
  for (var i = 0; i < 6; i++) {
    try {
      var l = await t.command('move', { timeout: 1500 });
      var x = l[l.length - 1];
      if (x === 'MOVE null') return null;
      if (x.indexOf('MOVE ') === 0) return JSON.parse(x.slice(5));
    } catch (e) { }
  }
  return undefined;
};
var st = async function () { for (var i = 0; i < 6; i++) { try { return await t.state(); } catch (e) { await t.wait(5); } } throw new Error('state'); };
var s = await st();
var t0 = Date.now();
while (s.screen === 'title' && Date.now() - t0 < 30000) { var m = await ask(); if (m) await t.playMove(m); await t.wait(10); s = await st(); }
while (!(s.menu && s.menu.play) && Date.now() - t0 < 40000) { await t.wait(10); s = await st(); }
var w = s.menu;
await t.playMove({ input: [{ move: w.wheels[2] }, { click: w.wheels[2] }] });
await t.wait(10);
await t.playMove({ input: [{ move: w.play }, { click: w.play }] });
await t.wait(300);
var G = { moves: 0, kinds: {}, lost: 0, maxLevel: 1 };
for (;;) {
  var mv = await ask();
  if (mv === undefined) { G.lost++; continue; }
  if (mv === null) {
    await t.wait(5);
    s = await st();
    if (s.over) break;
    if (s.level > G.maxLevel) G.maxLevel = s.level;
    continue;
  }
  await t.playMove(mv);
  G.moves++;
  var k = mv.kind + (mv.what ? ':' + mv.what : '');
  G.kinds[k] = (G.kinds[k] || 0) + 1;
  if (G.moves > 20000) break;
}
s = await st();
await t.shot('end');
return { game: G, level: s.level, score: s.score, over: s.over, screen: s.screen, errors: t.errors.length, updates: t.updates() };
