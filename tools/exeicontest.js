// Checks web/exeicon.js (a game's icon from its exe, docs/WEB_PLAYER.md, "The listing page") against the exes in
// assets/<game>/ (the user's own copies: git-ignored, read-only here), under Node:
//   node tools/exeicontest.js [assets folder]
// It prints one line for each exe (whether an icon was found, its size, its bit depth and which decoder read it) and the
// result of feeding the parser exes cut short and garbage. It writes no icon and keeps no bytes of an exe. Exit code 1
// when the parser throws, or finds nothing in an exe that has the icon resources, or when a damaged file gives anything
// but null.
"use strict";
const fs = require("fs");
const path = require("path");
const ByopopExeIcon = require("../web/exeicon.js");

const root = path.resolve(process.argv[2] || path.join(__dirname, "..", "assets"));
let failures = 0, found = 0, total = 0;

for (const game of fs.readdirSync(root).sort()) {
  const dir = path.join(root, game);
  if (!fs.statSync(dir).isDirectory()) continue;
  for (const name of fs.readdirSync(dir)) {
    if (!/\.exe$/i.test(name)) continue;
    total++;
    const bytes = new Uint8Array(fs.readFileSync(path.join(dir, name)));
    let icon = null, error = "";
    try { icon = ByopopExeIcon.parse(bytes); } catch (e) { error = String(e); failures++; }
    if (icon) found++;
    console.log((game + "/" + name).padEnd(44) + (error ? "THREW " + error :
      icon ? ("icon " + icon.width + "x" + icon.height + ", " + icon.bpp + "-bit, read as " + icon.path) : "no icon"));
  }
}
console.log("exes with an icon: " + found + " of " + total);

// ---------- synthetic exes: every decoder and the choice of the image, without a game's bytes ----------
function le16(n) { return [n & 255, (n >> 8) & 255]; }
function le32(n) { return [n & 255, (n >> 8) & 255, (n >> 16) & 255, (n >>> 24) & 255]; }
// A DIB for an icon: w x h, bpp, a palette of `colors` entries (entry c has b, g, r = 16c+1, 16c+2, 16c+3), the colour rows
// as pixel(x, y) gives them (a palette index, or [b, g, r, a] for 24 and 32 bits) and the mask (1 = transparent) from
// clear(x, y). Rows are stored bottom-up, as in a file.
function dib(w, h, bpp, colors, pixel, clear) {
  const out = [].concat(le32(40), le32(w), le32(h * 2), le16(1), le16(bpp), le32(0), le32(0), le32(0), le32(0), le32(colors), le32(0));
  for (let c = 0; c < colors; c++) out.push((c * 16 + 1) & 255, (c * 16 + 2) & 255, (c * 16 + 3) & 255, 0);
  const stride = ((w * bpp + 31) >> 5) * 4, mstride = ((w + 31) >> 5) * 4;
  for (let y = h - 1; y >= 0; y--) {
    const row = new Array(stride).fill(0);
    for (let x = 0; x < w; x++) {
      const v = pixel(x, y);
      if (bpp === 32) { for (let k = 0; k < 4; k++) row[x * 4 + k] = v[k]; }
      else if (bpp === 24) { for (let k = 0; k < 3; k++) row[x * 3 + k] = v[k]; }
      else if (bpp === 8) row[x] = v;
      else if (bpp === 4) row[x >> 1] |= (x & 1) ? v : (v << 4);
      else row[x >> 3] |= v << (7 - (x & 7));
    }
    for (const b of row) out.push(b);
  }
  for (let y = h - 1; y >= 0; y--) {
    const row = new Array(mstride).fill(0);
    for (let x = 0; x < w; x++) if (clear(x, y)) row[x >> 3] |= 1 << (7 - (x & 7));
    for (const b of row) out.push(b);
  }
  return Uint8Array.from(out);
}
// only the signature and the IHDR of a PNG, which is all that the parser reads (a browser decodes the rest)
function png(w, h) {
  return Uint8Array.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0, 0, 0, 13, 0x49, 0x48, 0x44, 0x52]
    .concat([w >>> 24, w >>> 16, w >>> 8, w].map((v) => v & 255), [h >>> 24, h >>> 16, h >>> 8, h].map((v) => v & 255),
      [8, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
}
// An exe with one section that holds a resource directory: type 3 (an RT_ICON for each image) and type 14 (one group)
function exe(images, pe32plus) {
  const dir = (named, ids) => [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0].concat(le16(named), le16(ids));
  const n = images.length;
  let at = 0;
  const slot = (len) => { const o = at; at += len; return o; };
  const root = slot(16 + 16), icons = slot(16 + 8 * n), group = slot(16 + 8);
  const langs = [], leaves = [];
  for (let i = 0; i <= n; i++) langs.push(slot(16 + 8));
  for (let i = 0; i <= n; i++) leaves.push(slot(16));
  let grp = [0, 0, 1, 0].concat(le16(n));
  images.forEach((im, i) => {
    grp = grp.concat([im.w === 256 ? 0 : im.w, im.h === 256 ? 0 : im.h, 0, 0], le16(1), le16(im.bits), le32(im.data.length), le16(i + 1));
  });
  const blobs = images.map((im) => im.data).concat([Uint8Array.from(grp)]);
  const blobAt = blobs.map((b) => slot((b.length + 3) & ~3));
  const sec = new Uint8Array(at);
  const put = (o, bytes) => sec.set(bytes, o);
  const RVA = 0x1000;
  put(root, [].concat(dir(0, 2), le32(3), le32(0x80000000 | icons), le32(14), le32(0x80000000 | group)));
  put(icons, dir(0, n));
  images.forEach((im, i) => put(icons + 16 + i * 8, [].concat(le32(i + 1), le32(0x80000000 | langs[i]))));
  put(group, [].concat(dir(0, 1), le32(1), le32(0x80000000 | langs[n])));
  for (let i = 0; i <= n; i++) {
    put(langs[i], [].concat(dir(0, 1), le32(1033), le32(leaves[i])));
    put(leaves[i], [].concat(le32(RVA + blobAt[i]), le32(blobs[i].length), le32(0), le32(0)));
    put(blobAt[i], blobs[i]);
  }
  const optSize = pe32plus ? 240 : 224;
  const file = new Uint8Array(0x400 + sec.length);
  file.set([0x4d, 0x5a], 0); file.set(le32(0x80), 0x3c);
  file.set([0x50, 0x45, 0, 0], 0x80);
  file.set([].concat(le16(pe32plus ? 0x8664 : 0x14c), le16(1)), 0x84);
  file.set(le16(optSize), 0x80 + 20);
  const opt = 0x80 + 24, dirs = opt + (pe32plus ? 112 : 96);
  file.set(le16(pe32plus ? 0x20b : 0x10b), opt);
  file.set(le32(16), opt + (pe32plus ? 108 : 92));
  file.set(le32(RVA), dirs + 16); file.set(le32(sec.length), dirs + 20);
  const table = opt + optSize;
  file.set(le32(sec.length), table + 8); file.set(le32(RVA), table + 12); file.set(le32(sec.length), table + 16); file.set(le32(0x400), table + 20);
  file.set(sec, 0x400);
  return file;
}
function check(label, ok, detail) { if (!ok) { console.log("synthetic " + label + ": FAIL " + (detail || "")); failures++; } }
{
  const before = failures;
  const px32 = (x, y) => [10 + x, 20 + y, 30, 255];  // b, g, r, a: a gradient, so a flipped image shows
  const d32 = (s) => ({ w: s, h: s, bits: 32, data: dib(s, s, 32, 0, px32, () => false) });
  const d24 = (s) => ({ w: s, h: s, bits: 24, data: dib(s, s, 24, 0, () => [1, 2, 3], () => false) });
  // the choice: a 32-bit image nearest to 48..64 px
  let r = ByopopExeIcon.parse(exe([d32(16), d32(48), { w: 256, h: 256, bits: 32, data: png(256, 256) }]));
  check("pick 48", r && r.width === 48 && r.path === "dib32 (alpha)", JSON.stringify(r && [r.width, r.path]));
  const last = (47 * 48 + 47) * 4;
  check("pixel order", r && r.rgba[0] === 30 && r.rgba[1] === 20 && r.rgba[2] === 10 && r.rgba[3] === 255 && r.rgba[last + 2] === 10 + 47 && r.rgba[last] === 30,
    "rows run bottom-up and a pixel is BGRA in the file, RGBA here");
  // a PNG is taken as it is
  r = ByopopExeIcon.parse(exe([{ w: 256, h: 256, bits: 32, data: png(256, 256) }, d32(16)], true));  // 256 is farther from 48..64 than 16 is
  check("png 256 or dib 16 (PE32+)", r && r.path === "dib32 (alpha)" && r.width === 16, JSON.stringify(r && [r.width, r.path]));
  r = ByopopExeIcon.parse(exe([{ w: 256, h: 256, bits: 32, data: png(256, 256) }], true));
  check("png 256, as it is (PE32+)", r && r.path === "png" && r.width === 256 && r.png && r.png.length > 0, JSON.stringify(r && [r.width, r.path]));
  r = ByopopExeIcon.parse(exe([{ w: 64, h: 64, bits: 32, data: png(64, 64) }]));
  check("png 64", r && r.path === "png" && r.width === 64 && r.bpp === 32, JSON.stringify(r && [r.width, r.path, r.bpp]));
  r = ByopopExeIcon.parse(exe([d32(16), d24(64)]));
  check("32-bit before a larger 24-bit", r && r.bpp === 32 && r.width === 16, JSON.stringify(r && [r.width, r.bpp]));
  r = ByopopExeIcon.parse(exe([{ w: 32, h: 32, bits: 4, data: dib(32, 32, 4, 16, () => 0, () => false) }, d24(64)]));
  check("no 32-bit: the largest", r && r.width === 64 && r.path.indexOf("dib24") === 0, JSON.stringify(r && [r.width, r.path]));
  // a 32-bit image whose alpha was never filled in takes it from the mask
  r = ByopopExeIcon.parse(exe([{ w: 16, h: 16, bits: 32, data: dib(16, 16, 32, 0, () => [5, 6, 7, 0], (x) => x < 8) }]));
  check("32-bit, mask", r && r.path === "dib32 (mask)" && r.rgba[3] === 0 && r.rgba[8 * 4 + 3] === 255, JSON.stringify(r && r.path));
  // 8, 4 and 1 bit through the palette; the mask's 1 is see-through
  r = ByopopExeIcon.parse(exe([{ w: 16, h: 16, bits: 8, data: dib(16, 16, 8, 256, (x, y) => (x + y * 16) & 255, (x, y) => y === 0) }]));
  const i8 = (16 + 5) & 255;
  check("8-bit", r && r.path === "dib8 (mask)" && r.rgba[(16 + 5) * 4] === ((i8 * 16 + 3) & 255) && r.rgba[3] === 0 && r.rgba[16 * 4 + 3] === 255,
    JSON.stringify(r && [r.path, r.rgba[(16 + 5) * 4]]));
  r = ByopopExeIcon.parse(exe([{ w: 16, h: 16, bits: 4, data: dib(16, 16, 4, 16, (x, y) => (x + y) & 15, () => false) }]));
  check("4-bit", r && r.path === "dib4 (mask)" && r.rgba[(2 * 16 + 3) * 4] === ((5 * 16 + 3) & 255), JSON.stringify(r && [r.path, r.rgba[(2 * 16 + 3) * 4]]));
  r = ByopopExeIcon.parse(exe([{ w: 16, h: 16, bits: 1, data: dib(16, 16, 1, 2, (x, y) => (x ^ y) & 1, () => false) }]));
  check("1-bit", r && r.path === "dib1 (mask)" && r.rgba[0] === 3 && r.rgba[4] === 19, JSON.stringify(r && [r.path, r.rgba[0], r.rgba[4]]));
  // an image that cannot be decoded (here a 16-bit DIB) is passed over for the next
  const odd = d32(48);
  odd.data[14] = 16;
  r = ByopopExeIcon.parse(exe([odd, d32(32)]));
  check("undecodable image skipped", r && r.width === 32, JSON.stringify(r && r.width));
  console.log("synthetic exes: " + (failures > before ? "FAIL" : "ok"));
}

// damaged and odd input: cut short at many places, garbage, empty, not an exe: null, never a throw
let damaged = 0;
function expectNull(label, bytes) {
  let r;
  try { r = ByopopExeIcon.parse(bytes); } catch (e) { console.log(label + ": THREW " + e); failures++; return; }
  damaged++;
  if (r !== null) { console.log(label + ": expected no icon"); failures++; }
}
expectNull("empty", new Uint8Array(0));
expectNull("text", new TextEncoder().encode("MZ this is not an exe"));
for (let i = 0; i < 20; i++) expectNull("random " + i, Uint8Array.from({ length: 4096 }, () => (Math.random() * 256) | 0));
// the first exe found, cut at many lengths (the resources are at its end, so most cuts lose them)
for (const game of fs.readdirSync(root).sort()) {
  const dir = path.join(root, game);
  const exe = fs.statSync(dir).isDirectory() && fs.readdirSync(dir).find((n) => /\.exe$/i.test(n));
  if (!exe) continue;
  const bytes = new Uint8Array(fs.readFileSync(path.join(dir, exe)));
  for (let k = 1; k < 40; k++) {
    const cut = bytes.subarray(0, Math.floor(bytes.length * k / 40));
    try { ByopopExeIcon.parse(cut); damaged++; } catch (e) { console.log("cut " + k + "/40 of " + game + ": THREW " + e); failures++; }
  }
  // a few bytes of the headers and the resource area changed
  for (let k = 0; k < 200; k++) {
    const copy = bytes.slice();
    for (let j = 0; j < 8; j++) copy[(Math.random() * Math.min(copy.length, 1024)) | 0] = (Math.random() * 256) | 0;
    try { ByopopExeIcon.parse(copy); damaged++; } catch (e) { console.log("changed header of " + game + ": THREW " + e); failures++; }
  }
  break;
}
console.log("damaged inputs: " + damaged + " parsed without a throw");
console.log(failures ? "FAIL (" + failures + ")" : "PASS");
process.exit(failures ? 1 : 0);
