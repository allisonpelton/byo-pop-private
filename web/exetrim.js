// The game's exe as the site stores it (the user, 2026-11-13: "Let's do the first two", of the ways to make the stored files
// smaller; docs/WEB_PLAYER.md, "Stored files"): the ports read texts, numbers and resources out of the player's exe through its
// section table (each port's GameText.cpp and the like), and never run its code. So the import keeps the PE headers and every
// section but the code, and leaves out the code's bytes and whatever follows the last section. The section table is kept whole:
// each address still maps to the same bytes, and an address in the code maps to none, as one outside every section did. An exe
// that PopCap's launcher wraps (the real game's image after the launcher's) can be stored as that inner image instead.
//
// TRIM: by game id, { exe: the exe's name in the game's folder, in lower case, unwrap: true to keep the inner image }. Only the
// games listed are trimmed; each was checked against every reader of its exe (tools/exetrimtest.js checks every address of the
// port's text tables against the trimmed file). The exe the import checks (web/exeprobes.js) is the original, before the trim.
var ByopopExeTrim = (function () {
  "use strict";
  var TRIM = {};
  var CODE = [".text", "CODE", ".code"];

  function u16(b, o) { return b[o] | (b[o + 1] << 8); }
  function u32(b, o) { return (b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | (b[o + 3] << 24)) >>> 0; }
  function put32(b, o, v) { b[o] = v & 255; b[o + 1] = (v >>> 8) & 255; b[o + 2] = (v >>> 16) & 255; b[o + 3] = (v >>> 24) & 255; }

  // The PE image's layout at offset at: null when it is not one
  function parse(b, at) {
    at = at || 0;
    if (at + 0x40 > b.length || b[at] !== 0x4d || b[at + 1] !== 0x5a) return null;
    var pe = at + u32(b, at + 0x3c);
    if (pe + 0xf8 > b.length || b[pe] !== 0x50 || b[pe + 1] !== 0x45 || b[pe + 2] !== 0 || b[pe + 3] !== 0) return null;
    var count = u16(b, pe + 6), opt = u16(b, pe + 20), table = pe + 24 + opt;
    if (table + 40 * count > b.length || opt < 64) return null;
    var magic = u16(b, pe + 24), dirs = pe + 24 + (magic === 0x20b ? 112 : 96);
    var img = { at: at, pe: pe, table: table, headers: u32(b, pe + 24 + 60), align: u32(b, pe + 24 + 36) || 512, dirs: dirs,
      dirCount: u32(b, dirs - 4), sections: [], end: 0 };
    for (var i = 0; i < count; i++) {
      var s = table + 40 * i, name = "";
      for (var k = 0; k < 8 && b[s + k]; k++) name += String.fromCharCode(b[s + k]);
      var raw = u32(b, s + 16), ptr = u32(b, s + 20);
      img.sections.push({ name: name, header: s, raw: raw, ptr: ptr, va: u32(b, s + 12), chars: u32(b, s + 36) });
      if (raw && at + ptr + raw > img.end) img.end = at + ptr + raw;
    }
    return img;
  }
  // The image a launcher wraps: the first PE image after the outer one's last section, or null
  function inner(b) {
    var outer = parse(b, 0);
    if (!outer) return null;
    for (var at = outer.end; at + 0x40 < b.length; at++) {
      if (b[at] === 0x4d && b[at + 1] === 0x5a) { var img = parse(b, at); if (img && img.sections.length) return img; }
    }
    return null;
  }
  // Whether a section is the code, which the ports never read: by its name
  function isCode(s) { return CODE.indexOf(s.name) >= 0; }

  // The trimmed exe: b (a Uint8Array) as a new Uint8Array, or b itself when it is not a PE image (nothing is lost then)
  function trim(b, o) {
    o = o || {};
    var img = o.unwrap ? inner(b) : parse(b, 0);
    if (!img) img = parse(b, 0);
    if (!img) return b;
    var base = img.at;
    var headers = Math.min(img.headers || 0x400, (img.sections.filter(function (s) { return s.raw; }).map(function (s) { return s.ptr; }).sort(function (x, y) { return x - y; })[0]) || 0x400);
    var keep = img.sections.filter(function (s) { return s.raw && !isCode(s); });
    var size = headers;
    keep.slice().sort(function (x, y) { return x.ptr - y.ptr; }).forEach(function (s) {
      size = Math.ceil(size / img.align) * img.align;
      s.newPtr = size;
      size += s.raw;
    });
    var out = new Uint8Array(size);
    out.set(b.subarray(base, base + headers), 0);
    keep.forEach(function (s) { out.set(b.subarray(base + s.ptr, base + s.ptr + s.raw), s.newPtr); });
    img.sections.forEach(function (s) {
      var h = s.header - base;
      if (keep.indexOf(s) >= 0) put32(out, h + 20, s.newPtr);
      else { put32(out, h + 16, 0); put32(out, h + 20, 0); }
    });
    // the certificate table is a file offset into the bytes left out
    if (img.dirCount > 4) { put32(out, img.dirs - base + 32, 0); put32(out, img.dirs - base + 36, 0); }
    return out;
  }
  // The bytes at a virtual address (image base plus RVA) through the section table: null outside every section's raw data
  function at(b, va, len) {
    var img = parse(b, 0);
    if (!img) return null;
    var baseVa = u32(b, img.pe + 24 + 28);
    for (var i = 0; i < img.sections.length; i++) {
      var s = img.sections[i], lo = baseVa + s.va;
      if (va >= lo && va < lo + s.raw) { var off = s.ptr + (va - lo); return off + len <= b.length ? b.subarray(off, off + len) : null; }
    }
    return null;
  }
  // The stored form of a game's file: rel its path in the game's folder; the trimmed exe for a listed game's exe, else the bytes
  function stored(gameId, rel, b) {
    var t = TRIM[gameId];
    if (!t || String(rel).toLowerCase() !== t.exe) return b;
    try { return trim(b, t); } catch (e) { return b; }
  }
  return { TRIM: TRIM, parse: parse, inner: inner, trim: trim, at: at, stored: stored };
})();
if (typeof module !== "undefined") module.exports = ByopopExeTrim;
