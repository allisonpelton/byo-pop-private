// Pictures a game keeps in a form the browser cannot show as it is, decoded to RGBA for the desktop themes' wallpapers
// (web/themes.js): every one read from the player's own files in this browser.
//
// bbk(index, data, name): an image of a Retro64 bank (Cosmic Bugs, Water Bugs; engines/retro64/R64Bank.cpp, Bank_ParseXml):
// the .bbk is XML, a BANK whose IMAGELIST gives each IMAGE's name, dataOffset and dataSize in the .dat beside it, its bpp,
// width, height and pitch (bytes a row, rows top to bottom), its paletteNumber and, at 16 or 32 bits a pixel, its red, green
// and blue masks; its PALETTELIST holds the palettes, PALETTE number="n" colorsUsed="c" with c triplets of red, green and blue
// in its text. Returns {width, height, rgba} or null.
//
// jp2(bytes, factory): a JPEG 2000 picture (Bejeweled Twist's), by the framework's own decoder built on its own for the web
// (jp2.js and jp2.wasm, web/jp2/jp2_web.cpp), loaded the first time one is wanted; factory, under Node, is the module's
// ByopopJP2. Resolves to {width, height, rgba}, or null where the file cannot be decoded.
var ByopopGamePics = (function () {
  "use strict";
  function text(bytes) {
    if (typeof bytes === "string") return bytes;
    var s = "";
    for (var i = 0; i < bytes.length; i += 0x8000) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
    return s;
  }
  function attr(tag, name) {
    var m = new RegExp("\\s" + name + "=\"([^\"]*)\"").exec(tag);
    return m ? m[1] : null;
  }
  function num(tag, name) { var v = attr(tag, name); return v === null ? NaN : Number(v); }
  function palette(xml, n) {
    var re = /<PALETTE\s([^>]*)>([^<]*)<\/PALETTE>/g, m;
    while ((m = re.exec(xml))) {
      if (num(" " + m[1], "number") !== n) continue;
      var used = num(" " + m[1], "colorsUsed"), v = m[2].trim().split(/\s+/).map(Number), pal = new Uint8Array(1024);
      for (var i = 0; i < used && i < 256 && i * 3 + 2 < v.length; i++) {
        pal[i * 4] = v[i * 3]; pal[i * 4 + 1] = v[i * 3 + 1]; pal[i * 4 + 2] = v[i * 3 + 2]; pal[i * 4 + 3] = 255;
      }
      return pal;
    }
    return null;
  }
  // a mask's shift and width: the value of its field scaled to 0..255
  function field(mask) {
    var shift = 0, bits = 0;
    if (!mask) return null;
    while (!((mask >>> shift) & 1)) shift++;
    while ((mask >>> (shift + bits)) & 1) bits++;
    return { shift: shift, max: Math.pow(2, bits) - 1 };
  }
  function bbk(index, data, name) {
    var xml = text(index), re = /<IMAGE\s[^>]*>/g, m, tag = null;
    while ((m = re.exec(xml))) if (attr(m[0], "name") === name) { tag = m[0]; break; }
    if (!tag) return null;
    var off = num(tag, "dataOffset"), size = num(tag, "dataSize"), bpp = num(tag, "bpp"), w = num(tag, "width"), h = num(tag, "height");
    var pitch = num(tag, "pitch"), bytesPer = bpp / 8;
    if (!(w > 0 && h > 0) || [8, 16, 32].indexOf(bpp) < 0 || !(pitch >= w * bytesPer) || off + Math.min(size, pitch * h) > data.length || pitch * (h - 1) + w * bytesPer > size) return null;
    var out = new Uint8Array(w * h * 4), x, y, s, o;
    if (bpp === 8) {
      var pal = palette(xml, num(tag, "paletteNumber"));
      if (!pal) return null;
      for (y = 0; y < h; y++) for (x = 0, s = off + y * pitch, o = y * w * 4; x < w; x++, o += 4) {
        var c = data[s + x] * 4;
        out[o] = pal[c]; out[o + 1] = pal[c + 1]; out[o + 2] = pal[c + 2]; out[o + 3] = 255;
      }
    } else {
      var f = [field(num(tag, "redMask") >>> 0), field(num(tag, "greenMask") >>> 0), field(num(tag, "blueMask") >>> 0)];
      if (!f[0] || !f[1] || !f[2]) return null;
      for (y = 0; y < h; y++) for (x = 0, s = off + y * pitch, o = y * w * 4; x < w; x++, o += 4, s += bytesPer) {
        var p = bpp === 16 ? data[s] | (data[s + 1] << 8) : (data[s] | (data[s + 1] << 8) | (data[s + 2] << 16) | (data[s + 3] << 24)) >>> 0;
        for (var k = 0; k < 3; k++) out[o + k] = Math.round(((p >>> f[k].shift) & f[k].max) * 255 / f[k].max);
        out[o + 3] = 255;
      }
    }
    return { width: w, height: h, rgba: out };
  }
  var jp2Module = null;
  function jp2(bytes, factory) {
    if (!jp2Module) {
      jp2Module = (factory ? Promise.resolve(factory) : new Promise(function (resolve, reject) {
        if (typeof ByopopJP2 === "function") { resolve(ByopopJP2); return; }
        var s = document.createElement("script");
        s.src = "jp2.js";
        s.onload = function () { resolve(ByopopJP2); };
        s.onerror = function () { reject(new Error("jp2.js")); };
        document.head.appendChild(s);
      })).then(function (f) { return f(); });
      jp2Module.catch(function () { jp2Module = null; });
    }
    return jp2Module.then(function (M) {
      var p = M._malloc(bytes.length);
      if (!p) return null;
      M.HEAPU8.set(bytes, p);
      var ok = M._jp2_decode(p, bytes.length);
      M._free(p);
      if (!ok) return null;
      var w = M._jp2_width(), h = M._jp2_height(), at = M._jp2_pixels() >>> 2, out = new Uint8Array(w * h * 4);
      for (var i = 0, o = 0; i < w * h; i++, o += 4) {
        var v = M.HEAPU32[at + i];
        out[o] = (v >>> 16) & 255; out[o + 1] = (v >>> 8) & 255; out[o + 2] = v & 255; out[o + 3] = v >>> 24;
      }
      M._jp2_release();
      return { width: w, height: h, rgba: out };
    });
  }
  return { bbk: bbk, jp2: jp2 };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopGamePics;
