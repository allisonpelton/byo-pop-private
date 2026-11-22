// The Fonts window's reader (web/fontview.js) under Node: node tools/fontviewtest.js [assets]
// First on fonts made here: a descriptor's command lines, words and lists, quotes and numbers as DescParser reads them;
// a font of two layers (a shadow behind its main layer, on a tag) with widths, spacing, a kerning pair, a character map,
// offsets, colours and orders, laid out and drawn; the framework's errors; the older width list; finding a game's fonts
// and pictures with their masks. Then, given the staged games (assets/), every font of each game is found and read as the
// window reads it, a main.pak's files with the folder's, the pictures by their size only (no pixels are decoded).
"use strict";
const fs = require("fs"), path = require("path");
require("./stringsload").intoGlobal();
global.Module = {};
require(path.join(__dirname, "../web/modpanel.js"));
const V = require("../web/fontview.js");
let checks = 0, failed = 0;
function same(name, got, want) {
  checks++;
  if (JSON.stringify(got) !== JSON.stringify(want)) { failed++; console.log("FAIL " + name + ": " + JSON.stringify(got) + " != " + JSON.stringify(want)); }
}
const bytes = (s) => new Uint8Array(Buffer.from(s, "latin1"));
// a picture w x h, every pixel the colour given
function picture(w, h, rgba) {
  const p = new Uint8ClampedArray(w * h * 4);
  for (let i = 0; i < p.length; i += 4) p.set(rgba, i);
  return { width: w, height: h, rgba: p };
}
const at = (pic, x, y) => Array.from(pic.rgba.slice((y * pic.width + x) * 4, (y * pic.width + x) * 4 + 4));

async function synthetic() {
  // the text: a UTF-8 file's characters as Latin-1, its euro sign 0x80
  same("utf8 text", V.descriptorText(new Uint8Array([0xef, 0xbb, 0xbf, 0x41, 0xc3, 0xa9, 0xe2, 0x82, 0xac, 0xe2, 0x80, 0x9c, 0x42])), "Aé\u0080B");
  same("plain text", V.descriptorText(bytes("Aé")), "Aé");
  // command lines: ";" ends one (not in quotes), "#" at a line's start is a note, a line's leading spaces go
  same("lines", V.commandLines("Define A (1,\n   2);\n# a note; here\n  CreateLayer Main;Define B 'x;y';\r\n").map((l) => [l.text, l.line]),
    [["Define A (1,\n2)", 1], ["CreateLayer Main", 4], ["Define B 'x;y'", 4]]);
  same("words and lists", V.parseToList("LayerSetImageMap Main ('a', 'b') ((0,0,1,1),(1,0,1,1))"),
    ["LayerSetImageMap", "Main", ["'a'", "'b'"], [["0", "0", "1", "1"], ["1", "0", "1", "1"]]]);
  same("quoted space", V.parseToList("SetCharMap (' ' ',') x"), ["SetCharMap", ["' '", "','"], "x"]);
  same("unquote doubled", V.unquote("'a''b'"), "a'b");
  same("unquote escaped", V.unquote("'\\''"), "'");
  same("unquote bare", V.unquote("abc"), "abc");
  same("ints", ["0x1F", "-12", "1.5", "", "-", "0X10"].map(V.toInt), [31, -12, null, null, 0, 16]);
  same("doubles", ["0.5", "-1.25", "1e3", "2"].map(V.toDouble), [0.5, -1.25, null, 2]);
  same("paths", [V.pathFrom("../images/afont1", "data"), V.pathFrom("x_layer0", "fonts/1200"), V.pathFrom("a/b", ""), V.pathFrom("..\\pic", "a\\b")],
    ["images/afont1", "fonts/1200/x_layer0", "a/b", "a/pic"]);

  // a font of two layers on a 12 x 4 picture, white and opaque
  const desc = [
    "Define CharList ('A', 'B', 'C');",
    "Define WidthList (3, 4, 5);",
    "CreateHorzSpanRectList RectList (0, 0, 0, 4) WidthList;",
    "SetDefaultPointSize 10;",
    "CreateLayer Main;",
    "LayerSetImage Main 'pic';",
    "LayerSetCharWidths Main CharList WidthList;",
    "LayerSetImageMap Main CharList RectList;",
    "LayerSetAscent Main 3;",
    "LayerSetSpacing Main 1;",
    "LayerSetKerningPairs Main ('AB') (-2);",
    "LayerSetPointSize Main 10;",
    "CreateLayerFrom Shadow Main;",
    "LayerSetOffset Shadow (1, 1);",
    "LayerSetColorMult Shadow (0, 0, 0, 0.5);",
    "LayerSetBaseOrder Shadow -1;",
    "LayerRequireTags Shadow ('shadow');",
    "SetCharMap ('a', 'b') ('A', 'B');"].join("\n");
  const asked = [];
  const font = await V.parseDescriptor(desc, "data/test.txt", (p) => { asked.push(p); return picture(12, 4, [255, 255, 255, 255]); });
  same("picture asked", asked, ["data/pic"]);
  same("layers", font.layers.map((l) => [l.name, l.required, l.offset, l.colorMult, l.baseOrder]),
    [["MAIN", [], [0, 0], [255, 255, 255, 255], 0], ["SHADOW", ["SHADOW"], [1, 1], [0, 0, 0, 127], -1]]);
  same("rects", [65, 66, 67].map((c) => font.layers[0].chars[c].rect), [[0, 0, 3, 4], [3, 0, 4, 4], [7, 0, 5, 4]]);
  same("sizes and tags", [V.sizes(font), V.tagsOf(font)], [[10], ["SHADOW"]]);
  const plain = V.activate(font, 10, []), shadowed = V.activate(font, 10, ["SHADOW"]);
  same("active layers", [plain.layers.length, shadowed.layers.length, plain.ascent, plain.height], [1, 2, 3, 4]);
  const line = V.layoutLine(plain, "ABC", 0, 3, [255, 255, 255, 255]);
  same("advance, spacing, kerning", [line.pieces.map((p) => p.x), line.width], [[0, 2, 7], 12]);
  same("character map", V.layoutLine(plain, "ab", 0, 3, [255, 255, 255, 255]).width, 6);
  const two = V.layoutLine(shadowed, "A", 0, 3, [255, 255, 255, 255]);
  same("orders and tints", two.pieces.map((p) => [p.x, p.y, p.colour, p.order]), [[1, 1, [0, 0, 0, 127], 127], [0, 0, [255, 255, 255, 255], 128]]);
  same("charset", V.charset(plain), "ABC");
  const one = V.render(plain, ["A"], [255, 255, 255, 255]);
  same("drawn", [one.width, one.height, at(one, 0, 0), at(one, 2, 3)], [3, 4, [255, 255, 255, 255], [255, 255, 255, 255]]);
  const shade = V.render(shadowed, ["A"], [255, 0, 0, 255]);
  same("drawn with its shadow", [shade.width, shade.height, at(shade, 0, 0), at(shade, 3, 4)], [4, 5, [255, 0, 0, 255], [0, 0, 0, 127]]);
  same("two lines", V.render(plain, ["A", "B"], [255, 255, 255, 255]).height, 8);

  // an additive red layer over a half-clear blue one: the red light adds to the blue's half, which stays half
  const add = await V.parseDescriptor("CreateLayer A; LayerSetImage A 'p'; LayerSetImageMap A ('x') ((0,0,1,1)); LayerSetCharWidths A ('x') (1);" +
    "CreateLayerFrom B A; LayerSetDrawMode B 1; LayerSetColorMult B (1, 0, 0, 1); LayerSetColorMult A (0, 0, 1, 0.5);", "t.txt",
    () => picture(1, 1, [255, 255, 255, 255]));
  same("additive", at(V.render(V.activate(add, 0, []), ["x"], [255, 255, 255, 255]), 0, 0), [255, 0, 127, 255]);

  // the framework's errors, with the line
  async function error(text, img) {
    try { await V.parseDescriptor(text, "t.txt", () => img === undefined ? picture(4, 4, [0, 0, 0, 0]) : img); return "loaded"; }
    catch (e) { return [e.message, e.line]; }
  }
  same("unknown command", await error("CreateLayer M;\nFoo M;"), ["Unknown Command", 2]);
  same("no picture", await error("CreateLayer M; LayerSetImage M 'x';", null), ["Failed to Load Image", 1]);
  same("out of bounds", await error("CreateLayer M; LayerSetImage M 'x'; LayerSetImageMap M ('a') ((0,0,5,1));"), ["Image rectangle out of bounds", 1]);
  same("no image set", await error("CreateLayer M; LayerSetImageMap M ('a') ((0,0,1,1));"), ["Layer image not set", 1]);
  same("wrong count", await error("CreateLayer M N;"), ["Invalid Number of Parameters", 1]);
  same("undefined layer", await error("LayerSetAscent Q 3;"), ["Undefined Layer", 1]);
  same("bad number", await error("CreateLayer M; LayerSetAscent M x3;"), ["Invalid Paramater Type", 1]);
  same("size mismatch", await error("CreateLayer M; LayerSetCharWidths M ('a', 'b') (1);"), ["List Size Mismatch", 1]);
  same("undefined value", await error("CreateLayer M; LayerSetCharWidths M Nothing (1);"), ['Unable to Dereference "Nothing"', 1]);
  same("taken name", await error("CreateLayer M; CreateLayer M;"), "loaded");

  // the older form: the space, the ascent, the characters; the other case where a letter has none
  const old = V.parseLegacy("4 7\nA 3 B 2\nc 5", "data/f.txt", picture(10, 7, [255, 255, 255, 255]));
  const L = old.layers[0];
  same("legacy cels", [L.chars[32].width, L.ascent, L.chars[65].rect, L.chars[66].rect, L.chars[99].rect], [4, 7, [0, 0, 3, 7], [3, 0, 2, 7], [5, 0, 5, 7]]);
  same("legacy cases", [old.charMap[97], old.charMap[67], old.charMap[98], old.charMap[65]], [65, 99, 66, 65]);
  const close = V.parseLegacy("4 A3 B2 C1 D", "f.txt", picture(10, 5, [0, 0, 0, 0]));
  same("legacy close", [close.layers[0].ascent, [65, 66, 67, 68].map((c) => close.layers[0].chars[c].width)], [5, [3, 2, 1, 0]]);
  same("sniff", ["CreateLayer Main;", "15 29\nA 18 B 18 C 15", "hello world", "1 2 3"].map(V.sniff), ["descriptor", "legacy", null, null]);

  // finding the fonts and pictures of a game's files
  const files = {
    "data/font1.txt": "CreateLayer M; LayerSetImage M '../images/font1';",
    "data/score.txt": "8 12\nA 5 B 5 C 5 D 5",
    "data/words.txt": "apple\nbanana",
    "data/lone.txt": "8 12\nA 5 B 5 C 5 D 5",
    "images/font1.gif": "c", "images/_font1.png": "m", "images/score_.gif": "m",
    "fonts/1200/a_layer0.jp2": "c"
  };
  const index = {};
  Object.keys(files).forEach((p) => { index[p.toLowerCase()] = { path: p, read: () => Promise.resolve(bytes(files[p])) }; });
  same("found", (await V.findFonts(index)).map((f) => [f.name, f.kind, f.picture || ""]), [["font1", "descriptor", ""], ["score", "legacy", "images/score"]]);
  same("picture files", [V.pictureFiles(index, "images/font1"), V.pictureFiles(index, "images/score"), V.pictureFiles(index, "fonts/1200/a_layer0.jp2")].map((f) => [f.colour && f.colour.path, f.mask && f.mask.path]),
    [["images/font1.gif", "images/_font1.png"], [null, "images/score_.gif"], ["fonts/1200/a_layer0.jp2", null]]);
  const decode = (b, p) => Promise.resolve(/\.gif$/.test(p) && b[0] === 99 ? picture(1, 1, [255, 0, 0, 255]) : picture(1, 1, [0, 0, 128, 255]));
  same("masked", at(await V.loadPicture(index, "images/font1", decode), 0, 0), [255, 0, 0, 128]);
  same("mask only", at(await V.loadPicture(index, "images/score", decode), 0, 0), [255, 255, 255, 128]);
}

// ---- the staged games ----
// a picture's size from its header, its pixels clear
function size(b, p) {
  const u16 = (i) => b[i] | (b[i + 1] << 8), be32 = (i) => ((b[i] << 24) | (b[i + 1] << 16) | (b[i + 2] << 8) | b[i + 3]) >>> 0;
  let w = 0, h = 0;
  if (b[0] === 0x89 && b[1] === 0x50) { w = be32(16); h = be32(20); }
  else if (b[0] === 0x47 && b[1] === 0x49) { w = u16(6); h = u16(8); }
  else if (b[0] === 0x42 && b[1] === 0x4d) { w = u16(18) | (u16(20) << 16); h = Math.abs((u16(22) | (u16(24) << 16)) << 0); }
  else if (b[0] === 0xff && b[1] === 0xd8) {
    for (let i = 2; i + 9 < b.length;) {
      if (b[i] !== 0xff) { i++; continue; }
      const m = b[i + 1], len = (b[i + 2] << 8) | b[i + 3];
      if (m >= 0xc0 && m <= 0xcf && m !== 0xc4 && m !== 0xc8 && m !== 0xcc) { h = (b[i + 5] << 8) | b[i + 6]; w = (b[i + 7] << 8) | b[i + 8]; break; }
      i += 2 + len;
    }
  } else {
    for (let i = 0; i + 12 < b.length; i++) {
      if (b[i] === 0x69 && b[i + 1] === 0x68 && b[i + 2] === 0x64 && b[i + 3] === 0x72) { h = be32(i + 4); w = be32(i + 8); break; }
    }
  }
  return Promise.resolve(w && h ? { width: w, height: h, rgba: new Uint8ClampedArray(w * h * 4) } : null);
}
function walk(dir, base, out) {
  fs.readdirSync(dir, { withFileTypes: true }).forEach((e) => {
    const p = path.join(dir, e.name), rel = path.relative(base, p).split(path.sep).join("/");
    if (e.isDirectory()) walk(p, base, out); else out.push(rel);
  });
  return out;
}
async function staged(root) {
  let games = 0, desc = 0, legacy = 0;
  const bad = [];
  for (const g of fs.readdirSync(root).sort()) {
    const dir = path.join(root, g);
    if (!fs.statSync(dir).isDirectory()) continue;
    V.setSource(() => Promise.resolve({ list: walk(dir, dir, []), read: (rel) => Promise.resolve(new Uint8Array(fs.readFileSync(path.join(dir, rel)))), close: () => {} }));
    const open = await V.openGame({ id: g, persist: g });
    const fonts = await V.findFonts(open.index);
    if (!fonts.length) continue;
    games++;
    for (const f of fonts) {
      try {
        const font = await V.loadFont(open.index, f, size);
        const act = V.activate(font, font.defaultPointSize, V.tagsOf(font));
        V.render(act, ["The quick brown fox jumps over the lazy dog. 1234567890"], [255, 255, 255, 255]);
        if (f.kind === "descriptor") desc++; else legacy++;
      } catch (e) { bad.push(g + "/" + f.path + ": " + e.message + (e.line ? " on line " + e.line : "")); }
    }
  }
  checks++;
  if (bad.length) { failed++; bad.forEach((b) => console.log("FAIL font " + b)); }
  return "; " + (desc + legacy) + " fonts read in " + games + " games (" + desc + " descriptors, " + legacy + " width lists)";
}

(async function () {
  await synthetic();
  let tail = "";
  const root = process.argv[2];
  if (root && fs.existsSync(root)) tail = await staged(root);
  console.log("fontviewtest: " + checks + " checks, " + failed + " failed" + tail);
  process.exit(failed ? 1 : 0);
})().catch((e) => { console.log("fontviewtest: FAILED " + (e && e.stack || e)); process.exit(1); });
