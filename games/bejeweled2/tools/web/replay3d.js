// The web page replays a native -commands file (origcmp/to_commands.py's: wait, move, click, leave, key, screenshot) in the 3D game, with
// the native run's own timing: the game is held after each `wait N` and every command is sent in the update it was given. The shots are
// build/webshots/<run>/<name>.png, for tools/shotdiff beside the native replay's (record_3d_tours.sh and run_tour_visible.sh's port half).
// replay3d_gen.py writes games/bejeweled2/tools/web/replay3d_gen.js, which is this file with the commands, the player's profile files
// and the seed in front of it; that file is opened as
//   bejeweled2.html?player=1&assets=1&hold=1&fast=1&drawevery=1&silent=1&driver=games/bejeweled2/tools/web/replay3d_gen.js&run=<id>
// (tools/webtest/serve.py), then `await byopop.test.driver`.
// Before it: var CMDS = [...lines of the commands file...], FILES = {"users/<name>/profile.dat": "<base64>"}, SEED = 909.
function b64(s) { var b = atob(s), u = new Uint8Array(b.length); for (var i = 0; i < b.length; i++) u[i] = b.charCodeAt(i); return u; }
var files = {};
Object.keys(FILES).forEach(function (k) { files[k] = b64(FILES[k]); });
await t.loaded;
await t.profile.wipe();
await t.profile.seed(files);
await Promise.all([t.start(), t.command("seed " + SEED)]);
var canvas = document.querySelector("canvas");
function leave() {
  ["mouseout", "mouseleave"].forEach(function (type) {
    canvas.dispatchEvent(new MouseEvent(type, { bubbles: true, cancelable: true, view: window, relatedTarget: document.body }));
  });
}
var shots = 0;
await t.step(1);
for (var i = 0; i < CMDS.length; i++) {
  var w = CMDS[i].split(/\s+/), c = w[0];
  if (c === "wait") await t.step(Number(w[1]));
  else if (c === "move") await t.move(Number(w[2]), Number(w[3]));
  else if (c === "click") { await t.down(Number(w[2]), Number(w[3])); await t.up(Number(w[2]), Number(w[3])); }
  else if (c === "leave") leave();
  else if (c === "key") await t.key(w[1]);
  else if (c === "screenshot") { await t.shot(w[1].replace(/^shots\//, "").replace(/\.png$/, "")); shots++; }
  else if (c === "quit") break;
  else if (c === "") continue;
  else throw new Error("replay3d: unknown command " + CMDS[i]);
}
return { shots: shots, updates: t.updates(), errors: (t.errors || []).length };
