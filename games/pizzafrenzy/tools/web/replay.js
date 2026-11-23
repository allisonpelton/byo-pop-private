// The web page replays a case's -realinput file (tools/identical: "<update> move game x y", "click game x y",
// "type C", "key <name>", "focus lose|regain", "quit") and takes the case's shot list's pictures, for tools/shotdiff
// beside the native port's (tools/identical/shots.sh, PORT_ONLY=1). The game is held between updates: each input and
// each shot of update N is sent while the game waits before its update N, as -realinput sends them, so the frames the
// page's input takes run no update. replay_gen.py writes games/pizzafrenzy/tools/web/replay_case.js (not committed),
// this file with the case in front of it, opened as
//   pizzafrenzy.html?player=1&assets=1&hold=1&fast=1&drawevery=1&silent=1
//     &driver=games/pizzafrenzy/tools/web/replay_case.js&run=<id>
// (tools/webtest/serve.py), then `await byopop.test.driver`.
// Before it: var CASE = {seed: N, lines: [...the input's lines...], shots: [[update, name], ...],
// files: {"<path under the player's folder>": "<base64>"}}.
function b64(s) {
  var b = atob(s), u = new Uint8Array(b.length);
  for (var i = 0; i < b.length; i++) u[i] = b.charCodeAt(i);
  return u;
}
var files = {};
Object.keys(CASE.files).forEach(function (k) { files[k] = b64(CASE.files[k]); });
await t.loaded;
await t.profile.wipe();
if (Object.keys(files).length) await t.profile.seed(files);
await Promise.all([t.start(), t.command("seed " + CASE.seed)]);
// the events by update, the shots of an update before its input (shots.sh's order). The page's picture held before
// update N shows what -realinput's picture before update N+1 does (the early probe, frames 1-90 of look1: every
// picture that differed was the native one of the next update, the still ones the same), so each shot is taken
// one update sooner than its list says; the input goes on its own update.
var SHOT_LEAD = 1;
var events = [];
CASE.shots.forEach(function (s) { events.push({ at: Math.max(0, s[0] - SHOT_LEAD), order: 0, shot: s[1] }); });
CASE.lines.forEach(function (l) {
  var w = l.trim().split(/\s+/);
  if (w.length >= 2) events.push({ at: Number(w[0]), order: 1, words: w.slice(1) });
});
events.sort(function (a, b) { return a.at - b.at || a.order - b.order; });
var KEYS = { Space: " ", Backspace: "Backspace", Escape: "Escape", Enter: "Enter" };
var shots = 0;
await t.pause();
for (var i = 0; i < events.length; i++) {
  var e = events[i];
  if (t.updates() < e.at) await t.step(e.at - t.updates());
  if (e.shot) { await t.shot(e.shot); shots++; continue; }
  var c = e.words[0];
  if (c === "quit") break;
  else if (c === "move") await t.move(Number(e.words[2]), Number(e.words[3]));
  else if (c === "click") {
    await t.down(Number(e.words[2]), Number(e.words[3]));
    await t.up(Number(e.words[2]), Number(e.words[3]));
  }
  else if (c === "type") await t.type(e.words[1]);
  else if (c === "key") await t.key(KEYS[e.words[1]] || e.words[1]);
  else if (c === "focus") await (e.words[1] === "lose" ? t.blur() : t.focus());
  else throw new Error("replay: unknown input " + e.words.join(" "));
}
return { shots: shots, updates: t.updates(), errors: (t.errors || []).length };
