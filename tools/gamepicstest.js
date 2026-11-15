// The desktop themes' picture decoders (web/gamepics.js) under Node, on pictures made here: a Retro64 bank's index and data
// with an 8-bit image on its palette and a 32-bit one by its masks, rows padded past their width; and, given the built
// JPEG 2000 decoder (build/web/jp2.js), a 4x3 RGB picture encoded once, losslessly, by OpenJPEG's own encoder from
// third_party/openjpeg (pixel i: red i*20, green 255-i*10, blue i*37 mod 256). node tools/gamepicstest.js [jp2.js]
"use strict";
const G = require("../web/gamepics.js");
let checks = 0, failed = 0;
function same(name, got, want) {
  checks++;
  if (JSON.stringify(got) !== JSON.stringify(want)) { failed++; console.log("FAIL " + name + ": " + JSON.stringify(got) + " != " + JSON.stringify(want)); }
}
const rgba = (p) => p ? Array.from(p.rgba) : null;

// The data: "flat", 3x2 at 8 bits, pitch 4 (one byte of padding a row), from offset 2; "deep", 2x1 at 32 bits from offset 10
const dat = new Uint8Array([9, 9, 0, 1, 2, 7, 2, 1, 0, 7, 0x00, 0x80, 0xff, 0, 0x10, 0x20, 0x30, 0]);
const index = `<BANK datafile="t.dat">
  <IMAGELIST>
    <IMAGE name="flat" dataOffset="2" dataSize="8" bpp="8" width="3" height="2" pitch="4" originalWidth="3" originalHeight="2" xOffset="0" yOffset="0" paletteNumber="1" redMask="0" greenMask="0" blueMask="0" />
    <IMAGE name="deep" dataOffset="10" dataSize="8" bpp="32" width="2" height="1" pitch="8" originalWidth="2" originalHeight="1" xOffset="0" yOffset="0" paletteNumber="-1" redMask="16711680" greenMask="65280" blueMask="255" />
    <IMAGE name="past" dataOffset="12" dataSize="8" bpp="32" width="2" height="1" pitch="8" originalWidth="2" originalHeight="1" xOffset="0" yOffset="0" paletteNumber="-1" redMask="16711680" greenMask="65280" blueMask="255" />
  </IMAGELIST>
  <PALETTELIST>
    <PALETTE number="0" colorsUsed="3">
       1 1 1 1 1 1 1 1 1
    </PALETTE>
    <PALETTE number="1" colorsUsed="3">
       255 0 0 0 255 0
       0 0 255
    </PALETTE>
  </PALETTELIST>
</BANK>`;
const flat = G.bbk(Buffer.from(index, "latin1"), dat, "flat");
same("an 8-bit image's size", flat && [flat.width, flat.height], [3, 2]);
same("an 8-bit image on its own palette, row by row past the padding", rgba(flat),
  [255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 255, 0, 255, 255, 0, 0, 255]);
same("a 32-bit image by its masks", rgba(G.bbk(index, dat, "deep")), [0xff, 0x80, 0x00, 255, 0x30, 0x20, 0x10, 255]);
same("an image past the data's end is none", G.bbk(index, dat, "past"), null);
same("a name the index lacks is none", G.bbk(index, dat, "missing"), null);
same("a palette the index lacks is none", G.bbk(index.replace('paletteNumber="1"', 'paletteNumber="5"'), dat, "flat"), null);

const JP2 = Buffer.from("AAAADGpQICANCocKAAAAFGZ0eXBqcDIgAAAAAGpwMiAAAAAtanAyaAAAABZpaGRyAAAAAwAAAAQAAwcHAAAAAAAPY29scgEAAAAAABAAAADBanAyY/9P/1EALwAAAAAABAAAAAMAAAAAAAAAAAAAAAQAAAADAAAAAAAAAAAAAwcBAQcBAQcBAf9SAAwAAAABAAEEBAAB/1wAB0BASEhQ/2QAJQABQ3JlYXRlZCBieSBPcGVuSlBFRyB2ZXJzaW9uIDIuNS40/5AACgAAAAAARgAB/5PfgDAHo1Tt6X/PtBgDmJjqfn/PtBQH0D1478HzgwAM/2DA+QGADglnx9oHPwAp+YEADgmuA1cMX//Z", "base64");
async function jp2Checks(jsPath) {
  const factory = require(require("path").resolve(jsPath));
  const pic = await G.jp2(new Uint8Array(JP2), factory);
  same("a .jp2's size", pic && [pic.width, pic.height], [4, 3]);
  const want = [];
  for (let i = 0; i < 12; i++) want.push(i * 20, 255 - i * 10, (i * 37) & 255, 255);
  same("a .jp2's pixels, losslessly", rgba(pic), want);
  same("a damaged .jp2 is none", await G.jp2(new Uint8Array(JP2.subarray(0, 60)), factory), null);
  same("a picture after a damaged one", rgba(await G.jp2(new Uint8Array(JP2), factory)), want);
}
(process.argv[2] ? jp2Checks(process.argv[2]) : Promise.resolve(console.log("(no jp2.js: the JPEG 2000 checks skipped)"))).then(() => {
  console.log("gamepicstest: " + checks + " checks, " + failed + " failed");
  process.exitCode = failed ? 1 : 0;
}, (e) => { console.log("FAIL " + e); console.log("gamepicstest: " + checks + " checks, " + (failed + 1) + " failed"); process.exitCode = 1; });
