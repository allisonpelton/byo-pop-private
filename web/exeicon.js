// A game's own icon, taken from the game's exe (docs/WEB_PLAYER.md, "The listing page"). The import already stores the
// exe of most games (PAGE.gameFiles in games/<game>/web/page.js); this reads the icon out of its resources, so that the
// listing page can show it in place of the initials badge and the game's page can use it as the tab icon. It stays in
// this browser and is never sent anywhere. No library: it reads the PE file itself.
//
//   ByopopExeIcon.parse(bytes)        Uint8Array of an exe -> {width, height, bpp, path, rgba | png} or null
//   ByopopExeIcon.fromBytes(bytes)    the same, as a small PNG: Promise of {dataUrl, width, height, bpp, path, built} or null
//   ByopopExeIcon.built(bytes)        the year the exe was linked (its PE header's time stamp), or 0 when it has none that
//                                     is likely (before 1995 or after this year: a fixed or zero stamp, as some linkers write)
//
// The PE: "MZ", the offset of the PE header at 0x3C, the section table (to turn the resource directory's RVAs into file
// offsets) and the resource directory, whose first level is the resource type: RT_GROUP_ICON (14) lists one group's
// images (each with a size and a bit depth and the id of its RT_ICON (3) resource), and each image is either a PNG,
// used as it is, or a DIB, a BITMAPINFOHEADER whose height counts the colour bitmap (XOR) and a 1-bit transparency mask
// (AND) under it, at 1, 4, 8, 24 or 32 bits per pixel, with a palette up to 8. Of a group's images it takes a 32-bit one
// at the size nearest 48 to 64 px, or without a 32-bit one the largest; if that one cannot be decoded, the next, and then
// the next group. An exe that is wrapped, damaged or has no icon gives null: nothing here throws. parse() is plain
// JavaScript and runs under Node (tools/exeicontest.js); only fromBytes() needs a browser (a canvas).
//
// The kept PNG is at most 64 px a side (a larger icon is made smaller, as the tile shows it at 72 px and enlarges
// a smaller one with crisp pixels) and its data URL at most 16 KB (a larger one is made smaller until it fits).
var ByopopExeIcon = (function () {
  "use strict";
  var MAX_SIDE = 64, MAX_URL = 16384;
  var RT_ICON = 3, RT_GROUP_ICON = 14;

  function u16(d, o) { if (o < 0 || o + 2 > d.length) throw new RangeError("read"); return d[o] | (d[o + 1] << 8); }
  function u32(d, o) { if (o < 0 || o + 4 > d.length) throw new RangeError("read"); return (d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (d[o + 3] << 24)) >>> 0; }
  function i32(d, o) { return u32(d, o) | 0; }
  function be32(d, o) { return ((d[o] << 24) | (d[o + 1] << 16) | (d[o + 2] << 8) | d[o + 3]) >>> 0; }

  // ---------- the PE file and its resource directory ----------
  // Returns {type(id) -> the type's entries [{id, sub, off}], data(entry) -> the resource's bytes}; null when it is not an exe
  // with resources. Throws RangeError for anything out of range.
  function openResources(d) {
    if (d.length < 0x40 || d[0] !== 0x4d || d[1] !== 0x5a) return null;
    var pe = u32(d, 0x3c);
    if (u32(d, pe) !== 0x00004550) return null;  // "PE\0\0"
    var nSections = u16(d, pe + 6), optSize = u16(d, pe + 20), opt = pe + 24;
    var magic = u16(d, opt);
    var dirCount = (magic === 0x20b) ? u32(d, opt + 108) : (magic === 0x10b) ? u32(d, opt + 92) : 0;
    var dirs = (magic === 0x20b) ? opt + 112 : opt + 96;
    if (dirCount < 3 || nSections > 96) return null;
    var resRva = u32(d, dirs + 16);  // data directory 2: the resources
    if (!resRva) return null;
    var sections = [], table = opt + optSize, i;
    for (i = 0; i < nSections; i++) {
      var s = table + i * 40;
      sections.push({ va: u32(d, s + 12), span: u32(d, s + 8) || u32(d, s + 16), raw: u32(d, s + 20) });
    }
    function offset(rva) {
      for (var j = 0; j < sections.length; j++) {
        var sec = sections[j];
        if (rva >= sec.va && rva < sec.va + sec.span) return rva - sec.va + sec.raw;
      }
      throw new RangeError("rva");
    }
    var base = offset(resRva);
    // a resource directory at a file offset -> its entries
    function entries(off) {
      var n = u16(d, off + 12) + u16(d, off + 14), out = [];
      if (n > 4096) throw new RangeError("entries");
      for (var k = 0; k < n; k++) {
        var e = off + 16 + k * 8, name = u32(d, e), data = u32(d, e + 4);
        out.push({ id: (name & 0x80000000) ? -1 : name, sub: !!(data & 0x80000000), off: base + (data & 0x7fffffff) });
      }
      return out;
    }
    // the bytes of a leaf: through its language level to the data entry
    function data(entry) {
      var e = entry;
      for (var depth = 0; e.sub && depth < 4; depth++) {
        var list = entries(e.off);
        if (!list.length) return null;
        e = list[0];
      }
      if (e.sub) return null;
      var rva = u32(d, e.off), size = u32(d, e.off + 4), at = offset(rva);
      if (size === 0 || at + size > d.length) throw new RangeError("data");
      return d.subarray(at, at + size);
    }
    function type(id) {
      var root = entries(base), list = [];
      for (var k = 0; k < root.length; k++) {
        if (root[k].id === id && root[k].sub) list = list.concat(entries(root[k].off));
      }
      return list;
    }
    return { type: type, data: data };
  }

  // ---------- one image ----------
  var PNG_SIGNATURE = [0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a];
  function isPng(b) {
    if (b.length < 33) return false;
    for (var i = 0; i < 8; i++) if (b[i] !== PNG_SIGNATURE[i]) return false;
    return true;
  }
  function decodePng(b) {
    var w = be32(b, 16), h = be32(b, 20), depth = b[24], ct = b[25];
    if (!(w > 0 && h > 0 && w <= 1024 && h <= 1024)) return null;
    var channels = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 }[ct];
    if (!channels) return null;
    return { width: w, height: h, bpp: depth * channels, path: "png", png: b };
  }

  // A DIB in an icon: BITMAPINFOHEADER, palette, the colour bitmap bottom-up, then the 1-bit mask (1 = transparent)
  function decodeDib(b) {
    var hdr = u32(b, 0);
    if (hdr < 40 || hdr > 124 || hdr > b.length) return null;
    var w = i32(b, 4), h2 = i32(b, 8), bpp = u16(b, 14), comp = u32(b, 16), used = u32(b, 32);
    var h = h2 >> 1;  // the header's height counts both bitmaps
    if (!(w > 0 && w <= 512 && h > 0 && h <= 512)) return null;
    if (!(comp === 0 || (comp === 3 && bpp === 32))) return null;  // plain RGB (BI_BITFIELDS only for the usual 32-bit BGRA)
    if ([1, 4, 8, 24, 32].indexOf(bpp) < 0) return null;
    var colors = (bpp <= 8) ? (used || (1 << bpp)) : 0;
    if (colors > (1 << bpp)) colors = 1 << bpp;
    var pal = hdr, xor = pal + colors * 4;
    var xorStride = ((w * bpp + 31) >> 5) * 4, andStride = ((w + 31) >> 5) * 4;
    var and = xor + xorStride * h;
    if (xor + xorStride * h > b.length) return null;
    var hasMask = and + andStride * h <= b.length;
    var rgba = new Uint8ClampedArray(w * h * 4), anyAlpha = false, x, y, o, p;
    for (y = 0; y < h; y++) {
      var row = xor + (h - 1 - y) * xorStride, mrow = and + (h - 1 - y) * andStride;
      for (x = 0; x < w; x++) {
        o = (y * w + x) * 4;
        if (bpp === 32) {
          p = row + x * 4;
          rgba[o] = b[p + 2]; rgba[o + 1] = b[p + 1]; rgba[o + 2] = b[p]; rgba[o + 3] = b[p + 3];
          if (b[p + 3]) anyAlpha = true;
        } else if (bpp === 24) {
          p = row + x * 3;
          rgba[o] = b[p + 2]; rgba[o + 1] = b[p + 1]; rgba[o + 2] = b[p]; rgba[o + 3] = 255;
        } else {
          var index = (bpp === 8) ? b[row + x]
            : (bpp === 4) ? ((x & 1) ? (b[row + (x >> 1)] & 15) : (b[row + (x >> 1)] >> 4))
            : ((b[row + (x >> 3)] >> (7 - (x & 7))) & 1);
          if (index >= colors) index = 0;
          p = pal + index * 4;
          rgba[o] = b[p + 2]; rgba[o + 1] = b[p + 1]; rgba[o + 2] = b[p]; rgba[o + 3] = 255;
        }
        if (bpp !== 32 && hasMask && ((b[mrow + (x >> 3)] >> (7 - (x & 7))) & 1)) rgba[o + 3] = 0;
      }
    }
    var path = "dib" + bpp;
    if (bpp === 32 && !anyAlpha) {
      // a 32-bit image whose alpha was never filled in: the mask says what is see-through (opaque without one)
      for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
          var clear = hasMask && ((b[and + (h - 1 - y) * andStride + (x >> 3)] >> (7 - (x & 7))) & 1);
          rgba[(y * w + x) * 4 + 3] = clear ? 0 : 255;
        }
      }
      path = "dib32 (mask)";
    } else if (bpp === 32) {
      path = "dib32 (alpha)";
    } else if (hasMask) {
      path += " (mask)";
    }
    return { width: w, height: h, bpp: bpp, path: path, rgba: rgba };
  }

  // Which of a group's images to try first: a 32-bit one at the size nearest 48 to 64 px (the larger when two are as near);
  // without a 32-bit one, the largest. The rest follow, so that an image that cannot be decoded is passed over.
  function order(images) {
    function away(size) { return size < 48 ? 48 - size : size > 64 ? size - 64 : 0; }
    var has32 = images.some(function (im) { return im.bits === 32; });
    return images.slice().sort(function (a, b) {
      if (has32) {
        if ((a.bits === 32) !== (b.bits === 32)) return a.bits === 32 ? -1 : 1;
        if (a.bits === 32 && away(a.size) !== away(b.size)) return away(a.size) - away(b.size);
      }
      return (b.size - a.size) || (b.bits - a.bits);
    });
  }

  // The first icon of the exe that can be decoded: {width, height, bpp, path, rgba | png}, or null
  function parse(bytes) {
    try {
      var d = (bytes instanceof Uint8Array) ? bytes : new Uint8Array(bytes);
      var res = openResources(d);
      if (!res) return null;
      var groups = res.type(RT_GROUP_ICON), icons = res.type(RT_ICON);
      for (var g = 0; g < groups.length; g++) {
        var dir = res.data(groups[g]);
        if (!dir || dir.length < 6 || u16(dir, 2) !== 1) continue;  // a GRPICONDIR: reserved, type 1 (icons), count
        var count = Math.min(u16(dir, 4), 64), images = [];
        for (var i = 0; i < count && 6 + i * 14 + 14 <= dir.length; i++) {
          var at = 6 + i * 14;
          images.push({ size: Math.max(dir[at] || 256, dir[at + 1] || 256), bits: u16(dir, at + 6), id: u16(dir, at + 12) });
        }
        var ranked = order(images);
        for (var r = 0; r < ranked.length; r++) {
          var entry = null;
          for (var k = 0; k < icons.length; k++) if (icons[k].id === ranked[r].id) { entry = icons[k]; break; }
          if (!entry) continue;
          var raw = res.data(entry), icon = null;
          try { icon = raw ? (isPng(raw) ? decodePng(raw) : decodeDib(raw)) : null; } catch (e) { icon = null; }
          if (icon) return icon;
        }
      }
    } catch (e) { /* damaged or odd: no icon */ }
    return null;
  }

  // ---------- as a small PNG (browser) ----------
  function base64(bytes) {
    var text = "";
    for (var i = 0; i < bytes.length; i += 8192) text += String.fromCharCode.apply(null, bytes.subarray(i, i + 8192));
    return btoa(text);
  }
  // source (a canvas or an image) drawn at most `side` px a side as a PNG data URL, smaller until it is short enough
  function shrink(source, sw, sh, side) {
    for (; side >= 16; side = Math.floor(side * 0.75)) {
      var scale = Math.min(1, side / Math.max(sw, sh));
      var w = Math.max(1, Math.round(sw * scale)), h = Math.max(1, Math.round(sh * scale));
      var c = document.createElement("canvas");
      c.width = w; c.height = h;
      var g = c.getContext("2d");
      g.imageSmoothingEnabled = true;
      g.imageSmoothingQuality = "high";
      g.drawImage(source, 0, 0, w, h);
      var url = c.toDataURL("image/png");
      if (url.length <= MAX_URL) return { dataUrl: url, width: w, height: h };
    }
    return null;
  }
  // the link year: "MZ", the PE header's offset at 0x3C, "PE  ", then the COFF header's TimeDateStamp 8 bytes in
  function built(bytes) {
    try {
      var d = (bytes instanceof Uint8Array) ? bytes : new Uint8Array(bytes);
      if (d[0] !== 0x4d || d[1] !== 0x5a) return 0;
      var pe = u32(d, 0x3c);
      if (u32(d, pe) !== 0x4550) return 0;
      var year = new Date(u32(d, pe + 8) * 1000).getUTCFullYear();
      return (year >= 1995 && year <= new Date().getUTCFullYear()) ? year : 0;
    } catch (e) { return 0; }
  }
  function fromBytes(bytes) {
    return new Promise(function (resolve) {
      try {
        var icon = parse(bytes);
        if (!icon) { resolve(null); return; }
        function done(kept) {
          resolve(kept ? { dataUrl: kept.dataUrl, width: kept.width, height: kept.height, bpp: icon.bpp, path: icon.path, built: built(bytes) } : null);
        }
        if (icon.png) {
          var url = "data:image/png;base64," + base64(icon.png);
          if (Math.max(icon.width, icon.height) <= MAX_SIDE && url.length <= MAX_URL) { done({ dataUrl: url, width: icon.width, height: icon.height }); return; }
          var img = new Image();  // too big: drawn smaller
          img.onload = function () { try { done(shrink(img, img.naturalWidth, img.naturalHeight, MAX_SIDE)); } catch (e) { resolve(null); } };
          img.onerror = function () { resolve(null); };
          img.src = url;
          return;
        }
        var canvas = document.createElement("canvas");
        canvas.width = icon.width; canvas.height = icon.height;
        canvas.getContext("2d").putImageData(new ImageData(icon.rgba, icon.width, icon.height), 0, 0);
        done(shrink(canvas, icon.width, icon.height, MAX_SIDE));
      } catch (e) { resolve(null); }
    });
  }

  return { parse: parse, fromBytes: fromBytes, built: built, MAX_URL: MAX_URL, MAX_SIDE: MAX_SIDE };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopExeIcon;
