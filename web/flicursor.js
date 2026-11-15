// The site's cursor from the games that draw their own pointer as an animation (the user, by the coordinator, 2026-11-14:
// Water Bugs' and Cosmic Bugs' pointers for the site's cursor and the desktop themes, every frame with its time). These games
// hide the system's pointer and draw the "pointer" animation of their own files at the mouse, with a shadow, wherever no board is
// in play (games/waterbugs/src/WBLevel.cpp, Level_AddPointer and Pointer_Update). The animation is a FLIC with a mask FLIC beside
// it (mouse_.flc and mask_mouse_.flc), named in the game's animation file with its frame time in the game's updates, 10 ms each.
// The game's page reads them from its own files and keeps them as its set (web/shell.html, takeCursorSet; web/cursors.js).
//
// The FLIC reading is engines/retro64/R64Flic.cpp's, line for line: palette chunks taken as they are (no scaling), BYTE_RUN,
// DELTA_FLI and DELTA_FLC, the palette fixed at the first frame (WBAnim.cpp, Animation_Load), and a mask frame's value the red of
// its index in the mask FLIC's own palette. The picture is drawn as the game draws it: the shadow, the image in (10, 10, 10)
// at 130/255 of its alpha, 5 pixels right and down, under the image itself, whose alpha is its mask; the mouse is at the image's
// top left. Ning Po also draws its own pointer as a sprite, but its sprite is empty, so it has no set.
var ByopopFlicCursor = (function () {
  "use strict";
  // by game id: the animation files to look in (the first naming the pointer wins) and the folder of the FLICs
  var GAMES = {
    waterbugs: { xml: ["wanims/water_bugs.xml"], dir: "wanims/" },
    cosmicbugs: { xml: ["animations/main.xml", "animations/bugs.xml"], dir: "animations/" }
  };
  var UPDATE_MS = 10, SHADOW = 5;

  function u16(b, o) { return b[o] | (b[o + 1] << 8); }
  function u32(b, o) { return (b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | (b[o + 3] << 24)) >>> 0; }
  function s8(v) { return v > 127 ? v - 256 : v; }
  function s16(v) { return v > 32767 ? v - 65536 : v; }

  // R64Flic.cpp's Flic_Open: the file's frames from the header's first frame on, its size and frame count
  function open(file) {
    if (!file || file.length < 0x80) return null;
    var size = u32(file, 0), magic = u16(file, 4), pos = 0x80;
    if (magic !== 0xaf11) { if (magic !== 0xaf12) return null; pos = u32(file, 0x50); }
    if (size < 0x14) return null;
    var mem = new Uint8Array(size);
    if (pos < file.length) mem.set(file.subarray(pos, pos + Math.min(file.length - pos, size - 0x14)), 0x14);
    return { mem: mem, frames: u16(file, 6), width: u16(file, 8), height: u16(file, 0xa), frame: 0, offset: 0x14, palette: new Uint8Array(0x300) };
  }
  function colour(f, at) {
    var mem = f.mem, packets = mem[at], c = at + 2, index = 0;
    for (var p = 0; p < packets; p++) {
      index = (index + mem[c]) & 255;
      var count = mem[c + 1] || 0x100;
      c += 2;
      for (var j = 0; j < count * 3; j++, c++) { var a = index * 3 + j; if (a < 0x300) f.palette[a] = mem[c]; }
    }
  }
  function byteRun(f, at, dst, pitch) {
    var mem = f.mem, c = at, row = 0;
    for (var y = 0; y < f.height; y++, row += pitch) {
      var packets = mem[c++], d = row;
      for (var k = 0; k < packets; k++) {
        var size = s8(mem[c++]);
        if (size < 0) { for (var i = 0; i < -size; i++) dst[d++] = mem[c++]; }
        else { var v = mem[c++]; for (var n = 0; n < size; n++) dst[d++] = v; }
      }
    }
  }
  function deltaFli(f, at, dst, pitch, end) {
    var mem = f.mem, c = at, row = 0;
    for (;;) {
      var rows = s16(u16(mem, c + 2));
      row += s16(u16(mem, c)) * pitch;
      c += 4;
      for (var r = 0; r < rows; r++, row += pitch) {
        var packets = mem[c++], d = row;
        for (var k = 0; k < packets; k++) {
          var size = s8(mem[c + 1]);
          d += mem[c]; c += 2;
          if (size > 0) for (var n = 0; n < size; n++) dst[d++] = mem[c++];
          if (size < 0) { var v = mem[c++]; for (var i = 0; i < -size; i++) dst[d++] = v; }
          if (end <= c) return;
        }
      }
      if (end <= c) return;
    }
  }
  function deltaFlc(f, at, dst, pitch) {
    var mem = f.mem, c = at + 2, lines = s16(u16(mem, at)), row = 0;
    for (var l = 0; l < lines; l++) {
      var word = u16(mem, c), look = c;
      while (c = look + 2, (word & 0xc000) === 0xc000) { row -= s16(word) * pitch; look = c; word = u16(mem, c); }
      var count = word;
      if (s16(word) < 0) { count = u16(mem, c); c = look + 4; }
      var d = row;
      for (var p = 0; p < s16(count); p++) {
        d += mem[c];
        var size = s8(mem[c + 1]);
        c += 2;
        if (size > 0) for (var i = 0; i < size; i++) { dst[d] = mem[c]; dst[d + 1] = mem[c + 1]; d += 2; c += 2; }
        if (size < 0) { var a = mem[c], b = mem[c + 1]; c += 2; for (var j = 0; j < -size; j++) { dst[d] = a; dst[d + 1] = b; d += 2; } }
      }
      if (s16(word) < 0) dst[d] = word & 255;
      row += pitch;
    }
  }
  // Flic_NextFrame: the next frame into dst (width x height indices)
  function next(f, dst) {
    var mem = f.mem, end = mem.length;
    if (f.offset + 16 > end) return;
    var frame = f.offset, chunk = frame + 16;
    f.frame++;
    if (u16(mem, frame + 4) === 0xf1fa && u16(mem, frame + 6) !== 0) {
      for (var i = 0; i < u16(mem, frame + 6); i++) {
        if (chunk + 6 > end || chunk + u32(mem, chunk) > end || u32(mem, chunk) < 6) break;
        var type = u16(mem, chunk + 4);
        if (type === 4 || type === 0xb) colour(f, chunk + 6);
        else if (type === 7) deltaFlc(f, chunk + 6, dst, f.width);
        else if (type === 0xc) deltaFli(f, chunk + 6, dst, f.width, chunk + u32(mem, chunk) - 1);
        else if (type === 0xf) byteRun(f, chunk + 6, dst, f.width);
        chunk += u32(mem, chunk);
      }
    }
    if (f.frames < f.frame) { f.frame = 1; f.offset = 0x14; frame = 0x14; }
    f.offset += u32(mem, frame);
  }

  // The pointer's frames as RGBA: [{width, height, rgba}], with the frame time in ms; null when the files lack it
  function frames(art, mask) {
    var f = open(art);
    if (!f || !f.frames || !f.width || !f.height) return null;
    var w = f.width, h = f.height, work = new Uint8Array(w * h), out = [], palette = null, i;
    for (i = 0; i < f.frames; i++) {
      next(f, work);
      if (i === 0) palette = f.palette.slice();
      out.push({ width: w, height: h, index: work.slice(), alpha: null });
    }
    var m = mask ? open(mask) : null;
    if (m && m.width === w && m.height === h) {
      var mw = new Uint8Array(w * h);
      for (i = 0; i < m.frames && i < out.length; i++) {
        next(m, mw);
        var a = new Uint8Array(w * h);
        for (var j = 0; j < a.length; j++) a[j] = m.palette[mw[j] * 3];
        out[i].alpha = a;
      }
    }
    return out.map(function (fr) {
      var W = w + SHADOW, H = h + SHADOW, rgba = new Uint8ClampedArray(W * H * 4);
      for (var y = 0; y < h; y++) for (var x = 0; x < w; x++) {
        var a = fr.alpha ? fr.alpha[y * w + x] : 255;
        if (!a) continue;
        // the shadow first, under the image
        var s = ((y + SHADOW) * W + x + SHADOW) * 4, sa = Math.round(a * 130 / 255);
        rgba[s] = rgba[s + 1] = rgba[s + 2] = 10; rgba[s + 3] = sa;
      }
      for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        var al = fr.alpha ? fr.alpha[y * w + x] : 255;
        if (!al) continue;
        var o = (y * W + x) * 4, k = fr.index[y * w + x] * 3, under = rgba[o + 3] / 255, top = al / 255, all = top + under * (1 - top);
        for (var c = 0; c < 3; c++) rgba[o + c] = Math.round((palette[k + c] * top + rgba[o + c] * under * (1 - top)) / all);
        rgba[o + 3] = Math.round(all * 255);
      }
      return { width: W, height: H, rgba: rgba };
    });
  }

  // The pointer's animation in a game's animation file (its text): {file, count, frameTime} or null
  function animation(xml) {
    var m = /<animation\b[^>]*\bname\s*=\s*"pointer"[^>]*>/i.exec(xml || "");
    if (!m) return null;
    function attr(n) { var a = new RegExp("\\b" + n + "\\s*=\\s*\"([^\"]*)\"", "i").exec(m[0]); return a ? a[1] : ""; }
    var file = attr("start_frame").replace(/\.eco$/i, ".flc");
    if (!file || /[\/\\]|\.\./.test(file)) return null;
    return { file: file, count: parseInt(attr("num_frames"), 10) || 0, frameTime: parseInt(attr("frame_time"), 10) || 0 };
  }

  // A game's pointer as the site's set ({pointer}), from read(rel): the game's file as a Uint8Array, or null. C: ByopopCursors.
  function build(id, read, C) {
    var g = GAMES[id];
    if (!g || !C) return null;
    var anim = null;
    for (var i = 0; i < g.xml.length && !anim; i++) {
      var x = read(g.xml[i]);
      if (x) anim = animation(new TextDecoder("latin1").decode(x));
    }
    if (!anim) return null;
    var list = frames(read(g.dir + anim.file), read(g.dir + "mask_" + anim.file));
    if (!list || !list.length) return null;
    var pics = list.map(function (fr) {
      var cut = C.crop(new ImageData(fr.rgba, fr.width, fr.height), 0, 0);
      return C.picture(cut, cut.x, cut.y);
    });
    var pointer = pics[0];
    if (pics.length > 1) {
      pointer.frames = pics.slice(1);
      pointer.ms = Math.max(40, Math.min(2000, anim.frameTime * UPDATE_MS));
    }
    return { pointer: pointer };
  }
  return { GAMES: GAMES, build: build, frames: frames, animation: animation, open: open };
})();
if (typeof module !== "undefined") module.exports = ByopopFlicCursor;
