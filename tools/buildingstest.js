// The Gallery's Venice buildings, their layout (web/buildings.js) under Node: node tools/buildingstest.js [assets]
// First on scenes made here: the tags and the scene tree with each stage's end values, the config's frames and lists, the
// row's places as LevelSelect::Load makes them, and a small scene laid out (a layer at its home place, a picture rounded
// where it is not bilinear, a scaled one centred, a tiled row with its clip carried to the row's ends, the screen's sky
// kept and mirrored, depth order). Then, given the staged Venice (assets/venice), the whole row is laid out as the window
// lays it, the pictures by their size only (no pixels are decoded).
"use strict";
const fs = require("fs"), path = require("path");
const B = require("../web/buildings.js");
let checks = 0, failed = 0;
function same(name, got, want) {
  checks++;
  if (JSON.stringify(got) !== JSON.stringify(want)) { failed++; console.log("FAIL " + name + ": " + JSON.stringify(got) + " != " + JSON.stringify(want)); }
}

function synthetic() {
  same("tags", B.readTags('<object name="a" z_order="1"\n x="2"><stage ms="3"></object>').map((t) => [t.name, t.close, t.attrs]),
    [["object", false, { name: "a", z_order: "1", x: "2" }], ["stage", false, { ms: "3" }], ["object", true, {}]]);
  const scene = B.parseScene('<object type="level"><stage ms="1"><attribute type="x" start="0" end="400"><attribute type="x" start="5" end="7">' +
    '<object name="kid" image="a.png"><stage><attribute type="y" start="-3" end="-4"><stage><attribute type="y" start="9" end="9"></object></object>');
  same("scene tree", [scene.length, scene[0].children.length, scene[0].children[0].attrs.name], [1, 1, "kid"]);
  same("stage end values, the last of a kind", scene[0].stages[0].values, { x: 7 });
  same("stages kept", scene[0].children[0].stages.map((s) => s.values.y), [-4, 9]);
  const config = B.parseConfig('<image name="gfx\\Bird.png" frame_width="24" frame_height="20" num_frames="8" fps="15"></image><image name="gfx\\x.png">' +
    '<level_file name="levels\\menu.xml"><level_file_relaxed name="levels\\a.xml"><level_file_relaxed name="levels\\win_sequence.xml">');
  same("config frames", config.images, { "gfx/bird.png": { w: 24, h: 20, count: 8 } });
  same("config lists", [config.lists.level_file, config.lists.level_file_relaxed], [["levels\\menu.xml"], ["levels\\a.xml", "levels\\win_sequence.xml"]]);
  same("thumbnail", B.thumbnailOf('\t<object type="level" thumbnail="gfx\\t1.png" name="x">'), "gfx\\t1.png");
  same("no thumbnail", B.thumbnailOf('<object type="level">'), "");
  // the row: a picture at the levels 1, 10, 19 ... and the win sequence's 90 on; each level a ninth of its picture less 4
  const levels = [];
  for (let i = 0; i < 20; i++) levels.push({ name: "levels\\l" + i + ".xml", thumbnail: "T" + Math.floor((i + 8) / 9) + ".png", width: 400 });
  levels.push({ name: "levels\\win_sequence.xml", thumbnail: "W.png", width: 270 });
  const row = B.strip(levels);
  same("row pictures", row.map((p) => [p.path, p.win]), [["t1.png", false], ["t2.png", false], ["t3.png", false], ["w.png", true]]);
  same("row places", row.map((p) => Math.round(p.x * 100) / 100), [140.44, 504.44, 868.44, 998.89]);

  // a small scene: the level at (400, 300) fading in; its sky (lowest) and a bird (left out); a layer sliding home with a
  // picture not bilinear, a scaled bilinear one, a tiled row clipped, and the level select object with two pictures
  const text = '<object type="level"><stage><attribute type="x" start="400" end="400"><attribute type="y" start="300" end="300"><attribute type="alpha" start="0" end="1">' +
    '<object name="tile" image="sky.jpg" z_order="-1" bilinear="0"><stage><attribute type="y" start="-20" end="-20"></object>' +
    '<object image="bird.png"><stage><attribute type="x" start="5" end="5"></object>' +
    '<object type="layer" name="near" z_order="1" bilinear="0"><stage><attribute type="x" start="-400" end="-200"><attribute type="y" start="70" end="70">' +
    '<object image="far.png" z_order="1.5"><stage><attribute type="x" start="10.4" end="10.4"></object>' +
    '<object image="glow.png" bilinear="1" z_order="1.2"><stage><attribute type="x_scale" start="2" end="2"><attribute type="x" start="1" end="1"></object>' +
    '<object image="water.png" z_order="6" tiles_left="1" tiles_right="2" tiles_up="1" clip_left="-9999" clip_right="9999" clip_top="560" clip_bottom="600">' +
    '<stage><attribute type="y" start="200" end="200"></object>' +
    '<object type="level_select"><stage><attribute type="x" start="-350" end="-350"><attribute type="y" start="100" end="100"></object>' +
    '<object type="text" image="no.png"></object>' +
    '</object></object>';
  const sizes = { "sky.jpg": [800, 600], "bird.png": [20, 20], "far.png": [100, 50], "glow.png": [10, 10], "water.png": [40, 20], "t1.png": [400, 400], "t2.png": [400, 400], "no.png": [1, 1] };
  const size = (p) => sizes[p] ? { w: sizes[p][0], h: sizes[p][1] } : null;
  const lay = B.layout(B.parseScene(text), [{ path: "t1.png", x: 140.44, win: false }, { path: "t2.png", x: 504.44, win: false }], size, {});
  same("sky kept", [lay.sky.path, lay.sky.dx, lay.sky.dy], ["sky.jpg", 0, -19]);
  same("draw order", lay.ops.map((o) => o.path), ["t2.png", "t1.png", "glow.png", "far.png", "water.png"]);
  const op = (p) => lay.ops.filter((o) => o.path === p)[0];
  // the layer's own x is its home place, 0, whatever its stage says; not bilinear: rounded, its left the centre less half
  same("plain picture", [op("far.png").dx, op("far.png").dy, op("far.png").smooth], [360, 346, false]);
  // scaled: centred, one pixel down, drawn smooth where bilinear
  same("scaled picture", [op("glow.png").dx, op("glow.png").dy, op("glow.png").dw, op("glow.png").smooth], [391, 366, 20, true]);
  same("tiled row", [op("water.png").row, op("water.png").clip], [{ tl: 1, tr: 2, tu: 1, td: 0, stepX: 40, stepY: 20 }, [-9999, 560, 9999, 600]]);
  // the row's pictures: bilinear (not rounded, a place truncated toward zero), the first drawn last, over the second, and both
  // before the layer's other pictures, the level select object being at depth 0
  same("row picture", [op("t1.png").dx, op("t1.png").dy, op("t2.png").dx], [-9, 161, 354]);
  same("ends", [lay.left, lay.right, lay.top, lay.bottom], [-9, 754, 0, 600]);
  same("bare ends", lay.bare, { left: -9, right: 754, top: 161, bottom: 561 });
  // the whole row with its scene: the sky mirrored every other copy, the tiled row carried to both ends
  const all = B.picture(lay, "all", true);
  same("whole size", [all.left, all.width, all.height], [-9, 763, 600]);
  same("sky copies", all.ops.filter((o) => o.path === "sky.jpg").map((o) => [o.dx, o.flipX]), [[-800, true], [0, false]]);
  const water = all.ops.filter((o) => o.path === "water.png").map((o) => [o.dx, o.dy]);
  same("water carried", [water.length, water[0], water[water.length - 1]], [40, [-20, 541], [740, 561]]);
  // one building with its scene: its own columns, no other building
  const one = B.picture(lay, "1", true);
  same("one building", [one.left, one.width, one.height, one.ops.filter((o) => o.picture >= 0).map((o) => o.path)], [354, 400, 600, ["t2.png"]]);
  const bare = B.picture(lay, "0", false);
  same("one bare", [bare.left, bare.top, bare.width, bare.height, bare.ops.map((o) => o.path)], [-9, 161, 400, 400, ["t1.png"]]);
  same("all bare", B.picture(lay, "all", false).ops.map((o) => o.path), ["t2.png", "t1.png"]);
}

// ---- the staged Venice ----
// a picture's size from its header
function sizeOf(b) {
  const be32 = (i) => ((b[i] << 24) | (b[i + 1] << 16) | (b[i + 2] << 8) | b[i + 3]) >>> 0;
  if (b[0] === 0x89 && b[1] === 0x50) return { width: be32(16), height: be32(20) };
  if (b[0] === 0xff && b[1] === 0xd8) {
    for (let i = 2; i + 9 < b.length;) {
      if (b[i] !== 0xff) { i++; continue; }
      const m = b[i + 1], len = (b[i + 2] << 8) | b[i + 3];
      if (m >= 0xc0 && m <= 0xcf && m !== 0xc4 && m !== 0xc8 && m !== 0xcc) return { width: (b[i + 7] << 8) | b[i + 8], height: (b[i + 5] << 8) | b[i + 6] };
      i += 2 + len;
    }
  }
  return null;
}
function walk(dir, base, out) {
  fs.readdirSync(dir, { withFileTypes: true }).forEach((e) => {
    const p = path.join(dir, e.name), rel = path.relative(base, p).split(path.sep).join("/");
    if (e.isDirectory()) walk(p, base, out); else out.push(rel);
  });
  return out;
}
async function staged(dir) {
  B.setSource(() => Promise.resolve({ list: walk(dir, dir, []), read: (rel) => Promise.resolve(new Uint8Array(fs.readFileSync(path.join(dir, rel)))), close: () => {} }));
  const d = await B.load({ id: "venice", persist: "venice" }, (bytes) => Promise.resolve(sizeOf(bytes)));
  const lay = d.lay;
  // eight buildings in the order Journey builds them, then the trophy
  same("venice pictures", lay.pictures.map((p) => p.path.replace("gfx/levelselect/", "") + (p.win ? " win" : "")),
    ["thumbnail1.png", "thumbnail3.png", "thumbnail4.png", "thumbnail5.png", "thumbnail6.png", "thumbnail2.png", "thumbnail7.png", "thumbnail8.png", "thumbnail9.png win"]);
  same("venice first building", lay.pictures[0].rect, { x: -9, y: 161, w: 400, h: 400 });
  same("venice spacing", lay.pictures.slice(0, 8).map((p, i) => p.rect.x - 364 * i), [-9, -10, -10, -10, -10, -10, -10, -10]);
  same("venice trophy", lay.pictures[8].rect, { x: 2976, y: 144, w: 270, h: 434 });
  const all = B.picture(lay, "all", true), bare = B.picture(lay, "all", false);
  same("venice row", [all.left, all.width, all.height], [-10, 3280, 600]);
  same("venice bare row", [bare.left, bare.top, bare.width, bare.height], [-9, 144, 3255, 434]);
  // every picture the scene draws is there, and the row reaches both ends at the quay and the water
  const missing = lay.ops.filter((o) => !d.pictures[o.path]).map((o) => o.path);
  same("venice pictures read", missing, []);
  const reach = (p) => { const t = all.ops.filter((o) => o.path === p); return [Math.min.apply(null, t.map((o) => o.dx)) <= all.left, Math.max.apply(null, t.map((o) => o.dx + o.dw)) >= all.left + all.width]; };
  same("venice quay and water reach the ends", ["gfx/levelselect/ls_floor.png", "gfx/water_underlay2.png", "gfx/waterline02.png"].map(reach), [[true, true], [true, true], [true, true]]);
  return "; Venice: " + lay.pictures.length + " pictures in a row of " + all.width + " x " + all.height + " (" + all.ops.length + " draws)";
}

(async function () {
  synthetic();
  let tail = "";
  const root = process.argv[2], dir = root && path.join(root, "venice");
  if (dir && fs.existsSync(path.join(dir, "config.xml"))) tail = await staged(dir);
  console.log("buildingstest: " + checks + " checks, " + failed + " failed" + tail);
  process.exit(failed ? 1 : 0);
})().catch((e) => { console.log("buildingstest: " + e.stack); process.exit(1); });
