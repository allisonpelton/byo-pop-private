// The Fonts window (the user, 2026-11-17: "Add a font viewer to the site with a text generator for the games' custom
// fonts"): a Desktop 95 program in the manner of Windows 95's font viewer, on the listing page's desktop. Pick a game and
// one of its fonts, read from the player's own files in this browser, and see its sample sheet: its name, file, size and
// layers, every character it draws, and the text in the Text box at each of its sizes. Save As writes that text as a PNG,
// drawn as the game draws it: each layer in its order with its picture, colours, offsets, spacing and kerning, in the
// colour chosen and with the layers its tags turn on. Nothing is shipped: every picture is decoded here from the player's
// files and kept only while the window shows it.
//
// The fonts are the SexyAppFramework's ImageFonts, read as framework/ImageFont.cpp reads them:
//   - a descriptor: lines of commands split as DescParser::LoadDescriptor splits them (";" ends a command, "#" at a line's
//     start a note, quotes and backslashes as there), each parsed by ParseToList and run by FontData::HandleCommand (Define,
//     CreateHorzSpanRectList, SetDefaultPointSize, SetCharMap, CreateLayer, CreateLayerFrom and the Layer* commands);
//   - the older form, FontData::LoadLegacy: the space's width, the ascent, then each character and its width, the
//     characters side by side in a strip picture of the same name.
// A layer is active at a point size within its range and with its tags (ImageFont::GenerateActiveFontLayers), and a
// string is drawn as ImageFont::DrawStringEx draws it: each character's layers placed from the ascent and the offsets,
// the widest layer's advance taken, and the pieces drawn by their order, colourized by the text's colour times the layer's
// multiplier plus its addition, in the layer's draw mode (1 additive).
//
// A game's fonts are found by their text: any .txt with CreateLayer, LayerSetImage or LayerSetImageMap is a descriptor, and
// a .txt of the older shape with a picture of the same name is a legacy font; the files of a main.pak count as the
// folder's. A picture is found as platform/ImageLoader.cpp finds one: its name with any of the image extensions, its alpha
// from "_<name>" or "<name>_" (the mask's low byte), white where there is only a mask. The other engines' fonts (Retro64's,
// Jolly Bear's, Bonnie's and the rest) are not read by this window.
var ByopopFontView = (function () {
  "use strict";
  function T(k, a) { return typeof ByopopStrings !== "undefined" ? ByopopStrings.t(k, a) : k; }

  // ---------- the descriptor's text ----------
  // Bytes as the narrow characters the framework reads; a file with a UTF-8 byte order mark as UTF-8, each character the
  // Latin-1 byte it is, the euro sign 0x80, any other above U+00FF skipped (DescParser.cpp, ReadDescriptorChar)
  function descriptorText(bytes) {
    var utf8 = bytes.length >= 3 && bytes[0] === 0xef && bytes[1] === 0xbb && bytes[2] === 0xbf;
    var codes = [], i = utf8 ? 3 : 0;
    while (i < bytes.length) {
      var c = bytes[i++];
      if (!utf8 || c < 0x80) { codes.push(c); continue; }
      var trail = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : c >= 0xc0 ? 1 : 0, cp = c & (0x3f >> trail), j;
      for (j = 0; j < trail && i < bytes.length; j++) cp = (cp << 6) | (bytes[i++] & 0x3f);
      if (j < trail) break;
      if (trail > 0 && cp <= 0xff) codes.push(cp);
      else if (cp === 0x20ac) codes.push(0x80);
    }
    var s = "";
    for (var k = 0; k < codes.length; k += 0x2000) s += String.fromCharCode.apply(null, codes.slice(k, k + 0x2000));
    return s;
  }
  // ASCII upper case, as StringToUpper
  function upper(s) { return s.replace(/[a-z]+/g, function (m) { return m.toUpperCase(); }); }
  function fail(msg) { var e = new Error(msg); e.descriptor = true; throw e; }

  // The commands of a descriptor, each with the line it starts on (DescParser::LoadDescriptor, with ";" as the separator)
  function commandLines(text) {
    var out = [], pos = 0, n = text.length, lineCount = 0, current = "", currentLine = 0;
    while (pos < n) {
      var skipLine = false, atLineStart = true, inSingle = false, inDouble = false, escaped = false;
      for (;;) {
        if (pos >= n) break;
        var ch = text.charAt(pos++);
        if (ch === "\r") continue;
        if (ch === "\n") lineCount++;
        if (!atLineStart || (ch !== " " && ch !== "\t" && ch !== "\n")) {
          if (atLineStart) {
            if (ch === "#") skipLine = true;
            atLineStart = false;
          }
          if (ch === "\n") atLineStart = true;
          if (ch === "\n" && skipLine) skipLine = false;
          else if (!skipLine) {
            if (ch === "\\" && (inSingle || inDouble) && !escaped) escaped = true;
            else {
              if (ch === "'" && !inDouble && !escaped) inSingle = !inSingle;
              if (ch === '"' && !inSingle && !escaped) inDouble = !inDouble;
              if (ch === ";" && !inSingle && !inDouble) break;
              if (escaped) { current += "\\"; escaped = false; }
              if (!current.length) currentLine = lineCount + 1;
              current += ch;
            }
          }
        }
      }
      if (current.length) { out.push({ text: current, line: currentLine }); current = ""; }
    }
    return out;
  }
  // A command's words and lists (DescParser::ParseToList): a list is an array, a word a string, quotes kept
  function parseToList(s, at, expectEnd) {
    var list = [], inSingle = false, inDouble = false, escaped = false, cur = -1;
    while (at.pos < s.length) {
      var add = false, backslash = false, ch = s.charAt(at.pos++);
      var sep = ch === " " || ch === "\t" || ch === "\n" || ch === ",";
      if (escaped) { add = true; backslash = true; escaped = false; }
      else {
        if (ch === "'" && !inDouble) inSingle = !inSingle;
        else if (ch === '"' && !inSingle) inDouble = !inDouble;
        if (ch === "\\") escaped = true;
        else if (!inSingle && !inDouble) {
          if (ch === ")") { if (expectEnd) return list; fail("Unexpected List End"); }
          else if (ch === "(") {
            if (cur >= 0) fail("Unexpected List Start");
            list.push(parseToList(s, at, true));
          } else if (sep) cur = -1;
          else add = true;
        } else add = true;
      }
      if (add) {
        if (cur < 0) { cur = list.length; list.push(""); }
        list[cur] += (backslash ? "\\" : "") + ch;
      }
    }
    if (inSingle) fail("Unterminated Single Quotes");
    if (inDouble) fail("Unterminated Double Quotes");
    if (expectEnd) fail("Unterminated List");
    return list;
  }
  function isImmediate(s) { var c = s.charAt(0); return (c >= "0" && c <= "9") || c === "-" || c === "+" || c === "'" || c === '"'; }
  // DescParser::Unquote: the quotes at both ends dropped, a doubled quote one, a backslash making the next character literal
  function unquote(s) {
    var q = s.charAt(0);
    if (q !== "'" && q !== '"') return s;
    var out = "", lastWasQuote = true, escaped = false;
    for (var i = 1; i + 1 < s.length; i++) {
      var ch = s.charAt(i);
      if (escaped) { out += ch; escaped = false; }
      else if (ch === q) { if (lastWasQuote) out += q; lastWasQuote = true; }
      else { if (ch === "\\") escaped = true; else out += ch; lastWasQuote = false; }
    }
    return out;
  }
  // Sexy::StringToInt (decimal, or hexadecimal after "0x"), Sexy::StringToDouble; null where the text is not one
  function toInt(s) {
    if (!s.length) return null;
    var radix = 10, neg = false, i = 0, v = 0;
    if (s.charAt(0) === "-") { neg = true; i++; }
    for (; i < s.length; i++) {
      var c = s.charAt(i);
      if (radix === 10 && c >= "0" && c <= "9") v = (v * 10 + (c.charCodeAt(0) - 48)) | 0;
      else if (radix === 16 && /[0-9A-Fa-f]/.test(c)) v = (v * 16 + parseInt(c, 16)) | 0;
      else if ((c === "x" || c === "X") && i === 1 && v === 0) radix = 16;
      else return null;
    }
    return neg ? (-v | 0) : v;
  }
  function toDouble(s) {
    if (!s.length) return null;
    var neg = false, i = 0, v = 0;
    if (s.charAt(0) === "-") { neg = true; i++; }
    for (; i < s.length; i++) {
      var c = s.charAt(i);
      if (c >= "0" && c <= "9") v = v * 10 + (c.charCodeAt(0) - 48);
      else if (c === ".") { i++; break; }
      else return null;
    }
    for (var mult = 0.1; i < s.length; i++, mult /= 10) {
      var d = s.charAt(i);
      if (d >= "0" && d <= "9") v += (d.charCodeAt(0) - 48) * mult;
      else return null;
    }
    return neg ? -v : v;
  }
  function dup(e) { return Array.isArray(e) ? e.map(dup) : e; }

  function newChar() { return { rect: [0, 0, 0, 0], width: 0, offset: [0, 0], kern: null, order: 0 }; }
  function newLayer(name) {
    var chars = [];
    for (var i = 0; i < 256; i++) chars.push(newChar());
    return { name: name, required: [], excluded: [], image: null, drawMode: -1, offset: [0, 0], spacing: 0, minPointSize: -1,
      maxPointSize: -1, pointSize: 0, ascent: 0, ascentPadding: 0, height: 0, defaultHeight: 0, colorMult: [255, 255, 255, 255],
      colorAdd: [0, 0, 0, 0], lineSpacingOffset: 0, baseOrder: 0, alphaCorrection: false, chars: chars };
  }
  function copyLayer(l, name) {
    var c = newLayer(name);
    Object.keys(l).forEach(function (k) {
      if (k === "name" || k === "chars") return;
      c[k] = Array.isArray(l[k]) ? l[k].slice() : l[k];
    });
    c.chars = l.chars.map(function (ch) {
      return { rect: ch.rect.slice(), width: ch.width, offset: ch.offset.slice(), kern: ch.kern ? Object.assign({}, ch.kern) : null, order: ch.order };
    });
    return c;
  }
  function newFont(kind, path) {
    var map = [];
    for (var i = 0; i < 256; i++) map.push(i);
    return { kind: kind, path: path, layers: [], layerMap: {}, charMap: map, defaultPointSize: 0, defines: {} };
  }

  // Sexy::GetFileDir and Sexy::GetPathFrom: a picture's name taken from the descriptor's folder ("../images/x" from data/)
  function fileDir(p) { var i = Math.max(p.lastIndexOf("\\"), p.lastIndexOf("/")); return i < 0 ? "" : p.slice(0, i); }
  function pathFrom(rel, dir) {
    if (rel.length >= 2 && rel.charAt(1) === ":") return rel;
    var path = dir, drive = "";
    if (path.length >= 2 && path.charAt(1) === ":") { drive = path.slice(0, 2); path = path.slice(2); }
    if (path.length && !/[\\\/]$/.test(path)) path += "/";
    var rest = rel;
    for (;;) {
      if (!path.length) break;
      var b = rest.indexOf("\\"), f = rest.indexOf("/"), slash = b < 0 || (f >= 0 && f < b) ? f : b;
      if (slash < 0) break;
      var part = rest.slice(0, slash);
      rest = rest.slice(slash + 1);
      if (part === "..") {
        var start = path.length - 1;
        while (start > 0 && path.charAt(start - 1) !== "\\" && path.charAt(start - 1) !== "/") start--;
        if (path.slice(start, path.length - 1) === "..") path += "../";
        else path = path.slice(0, start);
      } else if (part === "") { path = "/"; break; }
      else if (part !== ".") { path += part + "/"; break; }
    }
    return (drive + path + rest).replace(/\\/g, "/");
  }

  // A descriptor's font (FontData::Load). loadImage(path) resolves to {width, height, rgba} or null. Rejects, with the
  // framework's message and the command's line, where the game would have failed to load it.
  function parseDescriptor(text, path, loadImage) {
    var font = newFont("descriptor", path), defines = font.defines, lines = commandLines(text), images = {};
    function deref(name) { var k = upper(name); return Object.prototype.hasOwnProperty.call(defines, k) ? defines[k] : null; }
    function getValues(list) {
      var out = [];
      list.forEach(function (e) {
        if (Array.isArray(e)) { out.push(getValues(e)); return; }
        if (!e.length) return;
        var c = e.charAt(0);
        if (c === "'" || c === '"') out.push(unquote(e));
        else if (isImmediate(e)) out.push(e);
        else { var d = deref(e); if (d === null) fail('Unable to Dereference "' + e + '"'); out.push(dup(d)); }
      });
      return out;
    }
    function toStr(e) {
      if (Array.isArray(e)) return null;
      var d = deref(e);
      if (d !== null) return Array.isArray(d) ? null : unquote(d);
      return unquote(e);
    }
    function toStrings(e) {
      var values;
      if (Array.isArray(e)) values = getValues(e);
      else {
        values = deref(e);
        if (values === null) fail('Unable to Dereference "' + e + '"');
        if (!Array.isArray(values)) return null;
      }
      for (var i = 0; i < values.length; i++) if (Array.isArray(values[i])) return null;
      return values.slice();
    }
    function toList(e) {
      if (Array.isArray(e)) return getValues(e);
      var d = deref(e);
      return d === null || !Array.isArray(d) ? null : dup(d);
    }
    function toInts(e) {
      var s = toStrings(e);
      if (!s) return null;
      var v = s.map(toInt);
      return v.indexOf(null) >= 0 ? null : v;
    }
    function toDoubles(e) {
      var s = toStrings(e);
      if (!s) return null;
      var v = s.map(toDouble);
      return v.indexOf(null) >= 0 ? null : v;
    }
    function layerOf(e) {
      if (Array.isArray(e)) return null;
      var l = font.layerMap[upper(e)];
      if (!l) fail("Undefined Layer");
      return l;
    }
    // FontData::GetColorFromDataElement: a list of four factors of 255, or a number 0xAARRGGBB (no alpha is opaque)
    function colourOf(e) {
      if (Array.isArray(e)) {
        var f = toDoubles(e);
        if (!f) return null;
        return [0, 1, 2, 3].map(function (i) { return Math.trunc((f[i] || 0) * 255); });
      }
      var n = toInt(e);
      if (n === null) return null;
      var a = (n >>> 24) & 255;
      return [(n >>> 16) & 255, (n >>> 8) & 255, n & 255, a || 255];
    }
    function single(e) { return !Array.isArray(e); }
    // one command; false where the parameters are of the wrong kind, a promise where it loads a picture
    function command(p) {
      var cmd = p[0].toLowerCase(), n = p.length, L, v, i;
      function count(k) { if (n !== k) fail("Invalid Number of Parameters"); }
      function layerInt(field) {
        count(3);
        L = layerOf(p[1]);
        if (!L || !single(p[2]) || (v = toInt(p[2])) === null) return false;
        L[field] = v;
        return true;
      }
      switch (cmd) {
        case "define":
          count(3);
          if (!single(p[1])) return false;
          var name = upper(p[1]);
          if (isImmediate(name)) return false;
          if (Array.isArray(p[2])) defines[name] = getValues(p[2]);
          else { var d = deref(p[2]); defines[name] = d !== null ? dup(d) : p[2]; }
          return true;
        case "createhorzspanrectlist":
          count(4);
          var rect = toInts(p[2]), widths = toInts(p[3]);
          if (!single(p[1]) || !rect || rect.length !== 4 || !widths) return false;
          var x = 0, rects = [];
          widths.forEach(function (w) { rects.push([String(rect[0] + x), String(rect[1]), String(w), String(rect[3])]); x += w; });
          defines[upper(p[1])] = rects;
          return true;
        case "setdefaultpointsize":
          count(2);
          if (!single(p[1]) || (v = toInt(p[1])) === null) return false;
          font.defaultPointSize = v;
          return true;
        case "setcharmap":
          count(3);
          var from = toStrings(p[1]), to = toStrings(p[2]);
          if (!from || !to) return false;
          if (from.length !== to.length) fail("List Size Mismatch");
          var ok = true;
          for (i = 0; i < from.length; i++) {
            if (from[i].length === 1 && to[i].length === 1) font.charMap[from[i].charCodeAt(0) & 255] = to[i].charCodeAt(0) & 255;
            else ok = false;
          }
          return ok;
        case "createlayer":
        case "createlayerfrom":
          count(cmd === "createlayer" ? 2 : 3);
          if (!single(p[1])) return false;
          var src = cmd === "createlayerfrom" ? layerOf(p[2]) : null;
          if (cmd === "createlayerfrom" && !src) return false;
          var lname = upper(p[1]), layer = src ? copyLayer(src, lname) : newLayer(lname);
          // a name already taken: the game says so and goes on, the new layer drawn but not named
          font.layers.push(layer);
          if (!font.layerMap[lname]) font.layerMap[lname] = layer;
          return true;
        case "layerrequiretags":
        case "layerexcludetags":
          count(3);
          L = layerOf(p[1]);
          var tags = toStrings(p[2]);
          if (!L || !tags) return false;
          tags.forEach(function (t) { (cmd === "layerrequiretags" ? L.required : L.excluded).push(upper(t)); });
          return true;
        case "layerpointrange":
          count(4);
          L = layerOf(p[1]);
          if (!L || !single(p[2]) || !single(p[3])) return false;
          var lo = toInt(p[2]), hi = toInt(p[3]);
          if (lo === null || hi === null) return false;
          L.minPointSize = lo; L.maxPointSize = hi;
          return true;
        case "layersetpointsize": return layerInt("pointSize");
        case "layersetheight": return layerInt("height");
        case "layersetascent": return layerInt("ascent");
        case "layersetascentpadding": return layerInt("ascentPadding");
        case "layersetlinespacingoffset": return layerInt("lineSpacingOffset");
        case "layersetspacing": return layerInt("spacing");
        case "layersetbaseorder": return layerInt("baseOrder");
        case "layersetimage":
          count(3);
          L = layerOf(p[1]);
          var file = toStr(p[2]);
          if (!L || file === null) return false;
          var at = pathFrom(file, fileDir(path)), key = at.toLowerCase();
          if (!images[key]) images[key] = Promise.resolve(loadImage(at));
          return images[key].then(function (img) {
            if (!img) fail("Failed to Load Image");
            L.image = img;
            return true;
          });
        case "layersetdrawmode":
          count(3);
          L = layerOf(p[1]);
          if (!L || !single(p[2]) || (v = toInt(p[2])) === null || v < 0 || v > 1) return false;
          L.drawMode = v;
          return true;
        case "layersetcolormult":
        case "layersetcoloradd":
          count(3);
          L = layerOf(p[1]);
          var col = L ? colourOf(p[2]) : null;
          if (!col) return false;
          L[cmd === "layersetcolormult" ? "colorMult" : "colorAdd"] = col;
          return true;
        case "layersetoffset":
          count(3);
          L = layerOf(p[1]);
          var off = toInts(p[2]);
          if (!L || !off || off.length !== 2) return false;
          L.offset = off;
          return true;
        case "layersetcharwidths":
        case "layersetcharorders":
          count(4);
          L = layerOf(p[1]);
          var chars = toStrings(p[2]), nums = toInts(p[3]);
          if (!L || !chars || !nums) return false;
          if (chars.length !== nums.length) fail("List Size Mismatch");
          var good = true;
          chars.forEach(function (c, k) {
            if (c.length !== 1) { good = false; return; }
            L.chars[c.charCodeAt(0) & 255][cmd === "layersetcharwidths" ? "width" : "order"] = nums[k];
          });
          return good;
        case "layersetimagemap":
          count(4);
          L = layerOf(p[1]);
          var mapChars = toStrings(p[2]), list = toList(p[3]);
          if (!L || !mapChars || !list) return false;
          if (mapChars.length !== list.length) fail("List Size Mismatch");
          if (!L.image) fail("Layer image not set");
          var fine = true;
          for (i = 0; i < mapChars.length; i++) {
            var r = Array.isArray(list[i]) ? toInts(list[i]) : null;
            if (mapChars[i].length !== 1 || !r || r.length !== 4) { fine = false; continue; }
            if (r[0] < 0 || r[1] < 0 || r[0] + r[2] > L.image.width || r[1] + r[3] > L.image.height) fail("Image rectangle out of bounds");
            L.chars[mapChars[i].charCodeAt(0) & 255].rect = r;
          }
          L.defaultHeight = 0;
          L.chars.forEach(function (c) { if (c.rect[3] + c.offset[1] > L.defaultHeight) L.defaultHeight = c.rect[3] + c.offset[1]; });
          return fine;
        case "layersetcharoffsets":
          count(4);
          L = layerOf(p[1]);
          var offChars = toStrings(p[2]), offs = toList(p[3]);
          if (!L || !offChars || !offs) return false;
          if (offChars.length !== offs.length) fail("List Size Mismatch");
          var okOff = true;
          offChars.forEach(function (c, k) {
            var o = Array.isArray(offs[k]) ? toInts(offs[k]) : null;
            if (c.length !== 1 || !o || o.length !== 2) { okOff = false; return; }
            L.chars[c.charCodeAt(0) & 255].offset = o;
          });
          return okOff;
        case "layersetkerningpairs":
          count(4);
          L = layerOf(p[1]);
          var pairs = toStrings(p[2]), kerns = toInts(p[3]);
          if (!L || !pairs || !kerns) return false;
          if (pairs.length !== kerns.length) fail("List Size Mismatch");
          var okPairs = true;
          pairs.forEach(function (pr, k) {
            if (pr.length !== 2) { okPairs = false; return; }
            var ch = L.chars[pr.charCodeAt(0) & 255];
            if (!ch.kern) ch.kern = {};
            ch.kern[pr.charCodeAt(1) & 255] = kerns[k];
          });
          return okPairs;
        case "layersetexinfo":
          count(4);
          L = layerOf(p[1]);
          var keys = toStrings(p[2]), values = toStrings(p[3]);
          if (!L || !keys || !values) return false;
          if (keys.length !== values.length) fail("List Size Mismatch");
          return true;
        case "layersetalphacorrection":
          count(3);
          L = layerOf(p[1]);
          var s = toStr(p[2]), ac = s === null ? null : toInt(s);
          if (!L || ac === null) return false;
          L.alphaCorrection = ac !== 0;
          return true;
      }
      fail("Unknown Command");
    }
    var chain = Promise.resolve();
    lines.forEach(function (ln) {
      chain = chain.then(function () {
        try {
          var params = parseToList(ln.text, { pos: 0 }, false);
          if (!params.length) return;
          if (Array.isArray(params[0])) fail("Missing Command");
          var r = command(params);
          return Promise.resolve(r).then(function (ok) { if (ok === false) fail("Invalid Paramater Type"); });
        } catch (e) { e.line = ln.line; throw e; }
      }).catch(function (e) { if (e.line === undefined) e.line = ln.line; throw e; });
    });
    return chain.then(function () { return font; });
  }

  // The older form (FontData::LoadLegacy): the space's width and the ascent, then each character and its width, every
  // character a cel of the strip in turn; a letter of one case with no cel takes the other case's. null where the text
  // is not of that shape.
  function parseLegacy(text, path, image) {
    // sscanf's steps: "%d" skips white space and takes a number, leaving the place where it is not one; "%1s" skips
    // white space and takes one character
    var pos = 0, WS = /[ \t\r\n\v\f]/;
    function skip() { while (pos < text.length && WS.test(text.charAt(pos))) pos++; }
    function int() {
      var at = pos;
      skip();
      var m = /^[+-]?\d+/.exec(text.slice(pos, pos + 24));
      if (!m) { pos = at; return null; }
      pos += m[0].length;
      return parseInt(m[0], 10);
    }
    var space = int();
    if (space === null) return null;
    var font = newFont("legacy", path), L = newLayer("");
    font.layers.push(L); font.layerMap[""] = L;
    L.image = image;
    L.defaultHeight = image ? image.height : 0;
    L.ascent = image ? image.height : 0;
    L.chars[32].width = space;
    var asc = int();
    if (asc !== null) L.ascent = asc;
    var x = 0, count = 0;
    for (;;) {
      skip();
      if (pos >= text.length) break;
      var ch = text.charCodeAt(pos++) & 255, width = int();
      if (width === null) width = 0;
      L.chars[ch].rect = [x, 0, width, image ? image.height : 0];
      L.chars[ch].width = width;
      x += width;
      count++;
    }
    if (!count) return null;
    for (var c = 65; c <= 90; c++) {
      if (L.chars[c].width === 0 && L.chars[c + 32].width !== 0) font.charMap[c] = c + 32;
      if (L.chars[c + 32].width === 0 && L.chars[c].width !== 0) font.charMap[c + 32] = c;
    }
    return font;
  }

  // ---------- laying out and drawing ----------
  // The font's point sizes: its default and every size a layer is drawn at or starts from; 0 where it has none
  function sizes(font) {
    var s = [font.defaultPointSize];
    font.layers.forEach(function (l) {
      if (l.pointSize > 0) s.push(l.pointSize);
      if (l.minPointSize > 0) s.push(l.minPointSize);
    });
    return s.filter(function (v, i) { return s.indexOf(v) === i; }).sort(function (a, b) { return a - b; });
  }
  // Every tag a layer asks for or refuses, in their first order
  function tagsOf(font) {
    var t = [];
    font.layers.forEach(function (l) { l.required.concat(l.excluded).forEach(function (x) { if (t.indexOf(x) < 0) t.push(x); }); });
    return t;
  }
  // ImageFont::GenerateActiveFontLayers at a point size with tags (the scale 1)
  function activate(font, pointSize, tags) {
    var act = { font: font, pointSize: pointSize, layers: [], ascent: 0, ascentPadding: 0, height: 0, lineSpacingOffset: 0 }, first = true;
    font.layers.forEach(function (L) {
      if (!(pointSize >= L.minPointSize && (pointSize <= L.maxPointSize || L.maxPointSize === -1))) return;
      if (L.required.some(function (t) { return tags.indexOf(t) < 0; })) return;
      if (tags.some(function (t) { return L.excluded.indexOf(t) >= 0; })) return;
      var a = { base: L, image: L.image, rects: null }, layerPoint = 1, point = 1;
      if (L.pointSize === 0 || pointSize === L.pointSize) a.rects = L.chars.map(function (c) { return c.rect; });
      else {
        layerPoint = L.pointSize; point = pointSize;
        scaleLayer(a, point / layerPoint);
      }
      var asc = Math.trunc(L.ascent * point / layerPoint);
      if (asc > act.ascent) act.ascent = asc;
      var h = Math.trunc((L.height !== 0 ? L.height : L.defaultHeight) * point / layerPoint);
      if (h > act.height) act.height = h;
      var pad = Math.trunc(L.ascentPadding * point / layerPoint);
      if (first || pad < act.ascentPadding) act.ascentPadding = pad;
      var lso = Math.trunc(L.lineSpacingOffset * point / layerPoint);
      if (first || lso > act.lineSpacingOffset) act.lineSpacingOffset = lso;
      first = false;
      act.layers.push(a);
    });
    return act;
  }
  // a layer drawn at another point size: every cel scaled into one row of a new picture, as the framework makes it
  // (nearest pixels here, where the framework's stretch filters them)
  function scaleLayer(a, k) {
    var L = a.base, x = 0, maxH = 0;
    a.rects = L.chars.map(function (c) {
      var r = [x, 0, Math.trunc(c.rect[2] * k), Math.trunc(c.rect[3] * k)];
      if (r[3] > maxH) maxH = r[3];
      x += r[2];
      return r;
    });
    var W = x, H = maxH, out = new Uint8ClampedArray(W * H * 4), src = L.image;
    if (src && src.rgba) L.chars.forEach(function (c, i) {
      var d = a.rects[i];
      for (var yy = 0; yy < d[3]; yy++) for (var xx = 0; xx < d[2]; xx++) {
        var sx = c.rect[0] + Math.floor(xx * c.rect[2] / d[2]), sy = c.rect[1] + Math.floor(yy * c.rect[3] / d[3]);
        var si = (sy * src.width + sx) * 4, di = (yy * W + d[0] + xx) * 4;
        out[di] = src.rgba[si]; out[di + 1] = src.rgba[si + 1]; out[di + 2] = src.rgba[si + 2]; out[di + 3] = src.rgba[si + 3];
      }
    });
    a.image = { width: W, height: H, rgba: out };
  }
  function lineSpacing(act) { return act.height + act.lineSpacingOffset; }
  // how wide a string is drawn: its advance, and every piece that reaches out past it (a glow, a shadow)
  function extent(act, text) {
    var l = layoutLine(act, text, 0, 0, [255, 255, 255, 255]), lo = 0, hi = l.width;
    l.pieces.forEach(function (p) { if (p.src[2] && p.src[3]) { lo = Math.min(lo, p.x); hi = Math.max(hi, p.x + p.src[2]); } });
    return hi - lo;
  }
  // ImageFont::DrawStringEx's placing: the pieces of one string at (x, y), y the baseline less nothing (the game's y), in
  // the order they are drawn, and the string's width
  function layoutLine(act, text, x, y, colour) {
    var font = act.font, map = font.charMap, pieces = [], cur = x, POOL = 4096;
    for (var n = 0; n < text.length; n++) {
      var ch = map[text.charCodeAt(n) & 255], next = n < text.length - 1 ? map[text.charCodeAt(n + 1) & 255] : 0, max = cur;
      for (var li = 0; li < act.layers.length; li++) {
        var a = act.layers[li], L = a.base, c = L.chars[ch], scale = 1, ix, iy, w, sp;
        if (L.pointSize !== 0) scale *= Math.trunc(act.pointSize / L.pointSize);
        var kern = next && c.kern && c.kern[next] ? c.kern[next] : 0;
        if (scale === 1) {
          ix = cur + L.offset[0] + c.offset[0];
          iy = y - (L.ascent - L.offset[1] - c.offset[1]);
          w = c.width;
          sp = next ? L.spacing + kern : 0;
        } else {
          ix = cur + Math.trunc((L.offset[0] + c.offset[0]) * scale);
          iy = y - Math.trunc((L.ascent - L.offset[1] - c.offset[1]) * scale);
          w = Math.trunc(c.width * scale);
          sp = next ? Math.trunc((L.spacing + kern) * scale) : 0;
        }
        var tint = [0, 1, 2, 3].map(function (k) { return Math.min(Math.trunc(colour[k] * L.colorMult[k] / 255) + L.colorAdd[k], 255); });
        var order = Math.min(Math.max(L.baseOrder + c.order + 128, 0), 255);
        if (pieces.length >= POOL) break;
        pieces.push({ image: a.image, src: a.rects[ch], x: ix, y: iy, colour: tint, mode: L.drawMode, order: order, seq: pieces.length });
        if (cur + w + sp > max) max = cur + w + sp;
      }
      cur = max;
    }
    pieces.sort(function (p, q) { return p.order - q.order || p.seq - q.seq; });
    return { pieces: pieces, width: cur - x };
  }
  // Lines of text drawn into a new picture {width, height, rgba} (not premultiplied; clear where nothing is drawn), each
  // line a line spacing under the one before, the picture as large as the layout and every piece drawn
  function render(act, lines, colour) {
    var spacing = lineSpacing(act), laid = lines.map(function (t, i) { return layoutLine(act, t, 0, act.ascent + i * spacing, colour); });
    var x0 = 0, y0 = 0, x1 = 0, y1 = Math.max(1, lines.length * spacing);
    laid.forEach(function (l) {
      x1 = Math.max(x1, l.width);
      l.pieces.forEach(function (p) {
        if (!p.src[2] || !p.src[3]) return;
        x0 = Math.min(x0, p.x); y0 = Math.min(y0, p.y);
        x1 = Math.max(x1, p.x + p.src[2]); y1 = Math.max(y1, p.y + p.src[3]);
      });
    });
    var W = Math.max(1, x1 - x0), H = Math.max(1, y1 - y0), buf = new Float32Array(W * H * 4);
    laid.forEach(function (l) { l.pieces.forEach(function (p) { blit(buf, W, H, p, -x0, -y0); }); });
    var out = new Uint8ClampedArray(W * H * 4);
    for (var i = 0; i < W * H * 4; i += 4) {
      var a = buf[i + 3];
      if (a <= 0) continue;
      out[i] = buf[i] / a * 255; out[i + 1] = buf[i + 1] / a * 255; out[i + 2] = buf[i + 2] / a * 255; out[i + 3] = a * 255;
    }
    return { width: W, height: H, rgba: out, baseline: act.ascent - y0, left: -x0 };
  }
  // one piece, colourized (the picture's colour times the tint's, its alpha times the tint's), over what is there (mode 0
  // or none) or added to it (mode 1), in premultiplied light
  function blit(buf, W, H, p, ox, oy) {
    var img = p.image;
    if (!img || !img.rgba) return;
    var s = p.src, cr = p.colour[0] / 255, cg = p.colour[1] / 255, cb = p.colour[2] / 255, ca = p.colour[3] / 255;
    for (var yy = 0; yy < s[3]; yy++) {
      var sy = s[1] + yy, dy = p.y + oy + yy;
      if (sy < 0 || sy >= img.height || dy < 0 || dy >= H) continue;
      for (var xx = 0; xx < s[2]; xx++) {
        var sx = s[0] + xx, dx = p.x + ox + xx;
        if (sx < 0 || sx >= img.width || dx < 0 || dx >= W) continue;
        var si = (sy * img.width + sx) * 4, a = img.rgba[si + 3] / 255 * ca;
        if (a <= 0) continue;
        var r = img.rgba[si] / 255 * cr * a, g = img.rgba[si + 1] / 255 * cg * a, b = img.rgba[si + 2] / 255 * cb * a, di = (dy * W + dx) * 4;
        if (p.mode === 1) {
          buf[di] += r; buf[di + 1] += g; buf[di + 2] += b; buf[di + 3] = Math.min(1, buf[di + 3] + a);
        } else {
          var k = 1 - a;
          buf[di] = r + buf[di] * k; buf[di + 1] = g + buf[di + 1] * k; buf[di + 2] = b + buf[di + 2] * k; buf[di + 3] = a + buf[di + 3] * k;
        }
      }
    }
  }
  // the characters the font draws itself (not through its character map), from "!" up
  function charset(act) {
    var out = "";
    for (var c = 33; c < 256; c++) {
      if (act.font.charMap[c] !== c) continue;
      if (act.layers.some(function (a) { var r = a.rects[c]; return r && r[2] > 0 && r[3] > 0; })) out += String.fromCharCode(c);
    }
    return out;
  }

  // ---------- finding a game's fonts in its files ----------
  var DESCRIPTOR = /\b(CreateLayer|LayerSetImage|LayerSetImageMap)\b/i;
  var LEGACY = /^\s*[+-]?\d+\s+([+-]?\d+\s+)?(\S\s*[+-]?\d+\s+){3,}/;
  var IMAGE_EXTS = [".tga", ".jpg", ".png", ".gif", ".jpeg", ".bmp", ".jp2", ".j2k"];
  function stemOf(p) { var s = p.replace(/^.*\//, ""); var d = s.lastIndexOf("."); return d > 0 ? s.slice(0, d) : s; }
  // A picture's files by name (its extension, if any, one of the picture extensions): {colour, mask} paths or nulls
  function pictureFiles(index, path) {
    var dir = fileDir(path), file = path.slice(dir.length ? dir.length + 1 : 0), dot = file.lastIndexOf(".");
    if (dot > 0 && IMAGE_EXTS.indexOf(file.slice(dot).toLowerCase()) >= 0) file = file.slice(0, dot);
    var pre = dir ? dir + "/" : "";
    function find(stem) {
      for (var i = 0; i < IMAGE_EXTS.length; i++) {
        var hit = index[(pre + stem + IMAGE_EXTS[i]).toLowerCase()];
        if (hit) return hit;
      }
      return null;
    }
    return { colour: find(file), mask: find("_" + file) || find(file + "_") };
  }
  // A legacy font's strip: a picture of the descriptor's name, in images/ first, else in any folder
  function legacyPicture(index, path) {
    var stem = stemOf(path).toLowerCase(), dirs = {};
    Object.keys(index).forEach(function (k) {
      var s = stemOf(k), d = fileDir(k), ext = k.slice(k.lastIndexOf("."));
      if (IMAGE_EXTS.indexOf(ext) < 0) return;
      if (s === stem || s === "_" + stem || s === stem + "_") dirs[d] = true;
    });
    var list = Object.keys(dirs).sort(function (a, b) { return (/(^|\/)images$/.test(b) ? 1 : 0) - (/(^|\/)images$/.test(a) ? 1 : 0) || a.localeCompare(b); });
    return list.length ? (list[0] ? list[0] + "/" : "") + stem : null;
  }
  // the pictures a descriptor's LayerSetImage commands name in so many words (a name kept in a Define is not looked up)
  function picturesNamed(text, path) {
    var out = [];
    commandLines(text).forEach(function (ln) {
      try {
        var p = parseToList(ln.text, { pos: 0 }, false);
        if (p.length === 3 && typeof p[0] === "string" && p[0].toLowerCase() === "layersetimage" && typeof p[2] === "string" && /^['"]/.test(p[2]))
          out.push(pathFrom(unquote(p[2]), fileDir(path)));
      } catch (e) {}
    });
    return out;
  }
  // which kind of font a text file is: "descriptor" (its commands anywhere: a long list of rectangles can come first),
  // "legacy" (by its start) or null
  function sniff(text) {
    if (DESCRIPTOR.test(text)) return "descriptor";
    if (LEGACY.test(text.slice(0, 4000))) return "legacy";
    return null;
  }
  // Every font of a game's files: index maps a lower-case path to {path, read()}. Resolves to [{path, name, kind, picture}]
  // in name order, a legacy font only where its strip is found.
  function findFonts(index) {
    var texts = Object.keys(index).filter(function (k) { return /\.txt$/.test(k); }).sort();
    return Promise.all(texts.map(function (k) {
      return index[k].read().then(function (bytes) {
        if (!bytes) return null;
        // (no font is half a megabyte long; a word list may be)
        var kind = sniff(descriptorText(bytes.subarray ? bytes.subarray(0, 524288) : bytes));
        if (kind === "legacy") {
          var pic = legacyPicture(index, k);
          return pic ? { path: index[k].path, kind: kind, picture: pic } : null;
        }
        // a descriptor whose picture the files lack (a font the game never loads) is left out
        if (kind && picturesNamed(descriptorText(bytes), index[k].path).some(function (p) {
          var f = pictureFiles(index, p);
          return !f.colour && !f.mask;
        })) return null;
        return kind ? { path: index[k].path, kind: kind } : null;
      }, function () { return null; });
    })).then(function (found) {
      found = found.filter(Boolean);
      // each named by its path without the extension, less the folder every one of them shares
      var dirs = found.map(function (f) { return fileDir(f.path); });
      var common = dirs.length ? dirs.reduce(function (a, b) {
        var x = a.split("/"), y = b.split("/"), n = 0;
        while (n < x.length && n < y.length && x[n].toLowerCase() === y[n].toLowerCase()) n++;
        return x.slice(0, n).join("/");
      }) : "";
      found.forEach(function (f) {
        var p = f.path.replace(/\.txt$/i, "");
        f.name = common && p.toLowerCase().indexOf(common.toLowerCase() + "/") === 0 ? p.slice(common.length + 1) : p;
      });
      return found.sort(function (a, b) { return a.name.toLowerCase().localeCompare(b.name.toLowerCase()); });
    });
  }
  // A picture as the game loads it: {width, height, rgba} with the mask's alpha, or null. decode(bytes, path) resolves to
  // {width, height, rgba} or null.
  function loadPicture(index, path, decode) {
    var f = pictureFiles(index, path);
    if (!f.colour && !f.mask) return Promise.resolve(null);
    function one(e) { return e ? e.read().then(function (b) { return b ? decode(b, e.path) : null; }, function () { return null; }) : Promise.resolve(null); }
    return Promise.all([one(f.colour), one(f.mask)]).then(function (r) {
      var img = r[0], mask = r[1];
      if (!img && !mask) return null;
      if (!img) {
        var white = new Uint8ClampedArray(mask.width * mask.height * 4);
        for (var i = 0; i < white.length; i += 4) { white[i] = white[i + 1] = white[i + 2] = 255; white[i + 3] = mask.rgba[i + 2]; }
        return { width: mask.width, height: mask.height, rgba: white };
      }
      if (mask && mask.width === img.width && mask.height === img.height)
        for (var j = 0; j < img.rgba.length; j += 4) img.rgba[j + 3] = mask.rgba[j + 2];
      return img;
    });
  }
  // A font of the list, read: resolves to the font, or rejects with the framework's message
  function loadFont(index, entry, decode) {
    var e = index[entry.path.toLowerCase()];
    if (!e) return Promise.reject(new Error("no file"));
    return e.read().then(function (bytes) {
      var text = descriptorText(bytes);
      if (entry.kind === "descriptor")
        return parseDescriptor(text, entry.path, function (p) { return loadPicture(index, p, decode); });
      return loadPicture(index, entry.picture, decode).then(function (img) {
        var f = img ? parseLegacy(text, entry.path, img) : null;
        if (!f) throw new Error("Failed to Load Image");
        return f;
      });
    });
  }

  // ---------- the browser's side: the player's files and pictures ----------
  // A game's files: {index, close()} from its IndexedDB copy (web/music.js's helpers), with a main.pak's files after the
  // folder's own; tests give their own with setSource
  var source = function (game) {
    var F = typeof ByopopMusic !== "undefined" && ByopopMusic.files;
    if (!F) return Promise.resolve(null);
    return F.open(game.persist).then(function (db) {
      if (!db) return null;
      return F.list(db, game.persist).then(function (rels) {
        return { list: rels, read: function (rel) { return F.read(db, game.persist + "/game/" + rel); }, close: function () { db.close(); } };
      }, function (e) { db.close(); throw e; });
    });
  };
  function openGame(game) {
    return source(game).then(function (src) {
      if (!src) return null;
      var index = {};
      src.list.forEach(function (rel) { index[rel.toLowerCase()] = { path: rel, read: function () { return src.read(rel); } }; });
      var pak = Object.keys(index).filter(function (k) { return k === "main.pak"; })[0];
      var W = typeof Module !== "undefined" && Module.byopop;
      if (!pak || !W || !W.pak) return { index: index, close: src.close };
      return index[pak].read().then(function (bytes) {
        try {
          W.pak.read(bytes).forEach(function (f) {
            var k = f.path.toLowerCase();
            if (!index[k]) index[k] = { path: f.path, read: f.read };
          });
        } catch (e) {}
        return { index: index, close: src.close };
      }, function () { return { index: index, close: src.close }; });
    });
  }
  function decodePicture(bytes, path) {
    if (/\.(jp2|j2k)$/i.test(path)) return typeof ByopopGamePics !== "undefined" ? ByopopGamePics.jp2(bytes) : Promise.resolve(null);
    if (/\.tga$/i.test(path)) return Promise.resolve(null);
    var type = /\.png$/i.test(path) ? "image/png" : /\.gif$/i.test(path) ? "image/gif" : /\.bmp$/i.test(path) ? "image/bmp" : "image/jpeg";
    return createImageBitmap(new Blob([bytes], { type: type }), { premultiplyAlpha: "none", colorSpaceConversion: "none" }).then(function (bmp) {
      var c = document.createElement("canvas");
      c.width = bmp.width; c.height = bmp.height;
      var g = c.getContext("2d", { willReadFrequently: true });
      g.drawImage(bmp, 0, 0);
      var d = g.getImageData(0, 0, c.width, c.height);
      if (bmp.close) bmp.close();
      return { width: d.width, height: d.height, rgba: d.data };
    }, function () { return null; });
  }

  // ---------- the window ----------
  var COLOURS = [["white", [255, 255, 255]], ["black", [0, 0, 0]], ["silver", [192, 192, 192]], ["grey", [128, 128, 128]],
    ["red", [255, 0, 0]], ["maroon", [128, 0, 0]], ["yellow", [255, 255, 0]], ["olive", [128, 128, 0]], ["lime", [0, 255, 0]],
    ["green", [0, 128, 0]], ["aqua", [0, 255, 255]], ["teal", [0, 128, 128]], ["blue", [0, 0, 255]], ["navy", [0, 0, 128]],
    ["fuchsia", [255, 0, 255]], ["purple", [128, 0, 128]]];
  var BACKS = [["black", "#000000"], ["white", "#ffffff"], ["grey", "#808080"], ["none", ""]];
  function make(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; }
  function labelled(text, control) { var l = make("label", "", text); l.appendChild(control); return l; }
  function options(sel, list) {
    sel.textContent = "";
    list.forEach(function (o) { var e = make("option", "", o[1]); e.value = o[0]; sel.appendChild(e); });
  }
  function ratio() { var K = window.ByopopSkin; return K && K.ratio ? K.ratio() : window.devicePixelRatio || 1; }
  // a picture on a canvas, each of its pixels a square of whole screen pixels near the zoom's size
  function canvasOf(pic, zoom, back) {
    var c = make("canvas", "fv-pic");
    c.width = pic.width; c.height = pic.height;
    var g = c.getContext("2d");
    if (back) { g.fillStyle = back; g.fillRect(0, 0, c.width, c.height); }
    var t = make("canvas");
    t.width = pic.width; t.height = pic.height;
    t.getContext("2d").putImageData(new ImageData(new Uint8ClampedArray(pic.rgba.buffer, pic.rgba.byteOffset, pic.rgba.length), pic.width, pic.height), 0, 0);
    g.drawImage(t, 0, 0);
    var r = ratio(), k = Math.max(1, Math.round(zoom * r));
    c.style.width = pic.width * k / r + "px";
    c.style.height = pic.height * k / r + "px";
    return c;
  }

  var win = null;
  function build(allGames) {
    var el = make("section", "fv-win");
    el.id = "fontview";
    var bar1 = make("div", "fv-bar"), bar2 = make("div", "fv-bar"), bar3 = make("div", "fv-bar fv-options");
    var gamePick = make("select", "fv-game"), fontPick = make("select", "fv-font"), save = make("button", "fv-save", T("fontview.save"));
    save.type = "button";
    bar1.appendChild(labelled(T("fontview.game"), gamePick));
    bar1.appendChild(labelled(T("fontview.font"), fontPick));
    bar1.appendChild(save);
    var text = make("input", "fv-text");
    text.type = "text"; text.maxLength = 200; text.value = T("fontview.sample"); text.spellcheck = false;
    var textLabel = labelled(T("fontview.text"), text);
    textLabel.className = "fv-grow";
    bar2.appendChild(textLabel);
    var sizePick = make("select"), colourPick = make("select"), backPick = make("select"), zoomPick = make("select"), tagBox = make("span", "fv-tags");
    options(colourPick, COLOURS.map(function (c) { return [c[0], T("fontview.colour." + c[0])]; }));
    options(backPick, BACKS.map(function (b) { return [b[0], T("fontview.back." + b[0])]; }));
    options(zoomPick, [1, 2, 3, 4].map(function (z) { return [String(z), z * 100 + "%"]; }));
    var sizeLabel = labelled(T("fontview.size"), sizePick);
    bar3.appendChild(sizeLabel);
    bar3.appendChild(labelled(T("fontview.colour"), colourPick));
    bar3.appendChild(labelled(T("fontview.back"), backPick));
    bar3.appendChild(labelled(T("fontview.zoom"), zoomPick));
    bar3.appendChild(tagBox);
    var facts = make("dl", "fv-facts"), sheet = make("div", "fv-sheet"), status = make("div", "fv-status");
    el.appendChild(bar1); el.appendChild(bar2); el.appendChild(bar3); el.appendChild(facts); el.appendChild(sheet); el.appendChild(status);

    var game = null, fonts = [], font = null, files = null, busy = 0, tags = [];
    function say(s) { status.textContent = s; }
    function games() {
      var rec = {};
      try { rec = ByopopGameFiles.all(); } catch (e) {}
      return allGames.filter(function (g) {
        try { return ByopopGameFiles.present(rec[g.id]); } catch (e) { return false; }
      });
    }
    function fillGames() {
      var was = gamePick.value, list = games();
      options(gamePick, list.map(function (g) { return [g.id, g.title]; }));
      if (list.some(function (g) { return g.id === was; })) gamePick.value = was;
      return list;
    }
    function chooseGame() {
      var id = gamePick.value, g = allGames.filter(function (x) { return x.id === id; })[0] || null, ticket = ++busy;
      if (files) { files.close(); files = null; }
      game = g; fonts = []; font = null;
      options(fontPick, []);
      sheet.textContent = ""; facts.textContent = "";
      if (!g) { say(T("fontview.count", { n: 0 })); return Promise.resolve(); }
      say(T("fontview.reading"));
      return openGame(g).then(function (f) {
        if (ticket !== busy) { if (f) f.close(); return; }
        files = f;
        return f ? findFonts(f.index) : [];
      }).then(function (list) {
        if (ticket !== busy || !list) return;
        fonts = list;
        options(fontPick, fonts.map(function (f, i) { return [String(i), f.name]; }));
        say(T("fontview.count", { n: fonts.length }));
        if (fonts.length) return chooseFont();
      }, function () { if (ticket === busy) say(T("fontview.count", { n: 0 })); });
    }
    function chooseFont() {
      var entry = fonts[+fontPick.value], ticket = ++busy;
      font = null;
      sheet.textContent = ""; facts.textContent = "";
      if (!entry || !files) return Promise.resolve();
      say(T("fontview.reading"));
      return loadFont(files.index, entry, decodePicture).then(function (f) {
        if (ticket !== busy) return;
        font = f;
        options(sizePick, sizes(f).map(function (s) { return [String(s), String(s)]; }));
        sizePick.value = String(f.defaultPointSize);
        // a font of one size has no choice to make
        sizeLabel.hidden = sizePick.options.length < 2;
        tags = tagsOf(f);
        tagBox.textContent = "";
        if (tags.length) {
          tagBox.appendChild(make("span", "", T("fontview.layers")));
          tags.forEach(function (t) {
            var box = make("input");
            // off at first: a game turns a tag on only for the draws it is for (a glow, an outline in its own colour)
            box.type = "checkbox"; box.checked = false; box.value = t;
            box.addEventListener("change", draw);
            // the box before its name, as Windows sets a check box
            var lab = make("label");
            lab.appendChild(box); lab.appendChild(document.createTextNode(t));
            tagBox.appendChild(lab);
          });
        }
        draw();
        say(T("fontview.count", { n: fonts.length }));
      }, function (e) {
        if (ticket !== busy) return;
        say(T("fontview.failed", { file: entry.path, error: e && e.message || "", line: e && e.line || 0 }));
      });
    }
    function tagsOn() {
      return Array.prototype.filter.call(tagBox.querySelectorAll("input"), function (b) { return b.checked; }).map(function (b) { return b.value; });
    }
    function colour() { var c = COLOURS.filter(function (x) { return x[0] === colourPick.value; })[0][1]; return [c[0], c[1], c[2], 255]; }
    function back() { return BACKS.filter(function (b) { return b[0] === backPick.value; })[0][1]; }
    function draw() {
      sheet.textContent = "";
      facts.textContent = "";
      if (!font) return;
      var zoom = +zoomPick.value, on = tagsOn(), list = sizes(font), act = activate(font, +sizePick.value, on), bg = back();
      sheet.classList.toggle("fv-clear", !bg);
      sheet.style.backgroundColor = bg || "";
      // the font's facts, as the font viewer's head
      function fact(k, v) { facts.appendChild(make("dt", "", T(k))); facts.appendChild(make("dd", "", v)); }
      fact("fontview.info.name", stemOf(font.path));
      fact("fontview.info.file", font.path);
      if (font.defaultPointSize) fact("fontview.info.size", list.join(", "));
      fact("fontview.info.height", T("fontview.pixels", { n: lineSpacing(act) }));
      fact("fontview.info.layers", font.layers.map(function (l) { return l.name || "-"; }).join(", "));
      var chars = charset(act);
      fact("fontview.info.chars", String(chars.length));
      // every character, in rows that fit the sheet
      var room = Math.max(80, (sheet.clientWidth || 520) - 20) / Math.max(1, Math.round(zoom * ratio())) * ratio(), row = "", rows = [];
      for (var i = 0; i < chars.length; i++) {
        var tryRow = row + chars.charAt(i);
        if (row && extent(act, tryRow) > room) { rows.push(row); row = chars.charAt(i); }
        else row = tryRow;
      }
      if (row) rows.push(row);
      var col = colour();
      rows.forEach(function (r) { var line = make("div", "fv-line"); line.appendChild(canvasOf(render(act, [r], col), zoom, bg)); sheet.appendChild(line); });
      sheet.appendChild(make("hr", "fv-rule"));
      // the text at each size
      var words = text.value || " ";
      (list.length > 1 ? list : [+sizePick.value]).forEach(function (s) {
        var line = make("div", "fv-line");
        if (list.length > 1) line.appendChild(make("span", "fv-size", String(s)));
        line.appendChild(canvasOf(render(s === +sizePick.value ? act : activate(font, s, on), [words], col), zoom, bg));
        sheet.appendChild(line);
      });
    }
    // the text at the chosen size, at the font's own pixels, saved as a PNG (clear where Background is None)
    function saveAs() {
      if (!font) return;
      var pic = render(activate(font, +sizePick.value, tagsOn()), [text.value || " "], colour());
      var c = canvasOf(pic, 1, back());
      c.toBlob(function (blob) {
        if (!blob) return;
        var url = URL.createObjectURL(blob), a = document.createElement("a");
        a.href = url;
        a.download = (stemOf(font.path) + "-" + (text.value || "text").slice(0, 40)).replace(/[\\\/:*?"<>|]+/g, "_").trim() + ".png";
        document.body.appendChild(a);
        a.click();
        a.remove();
        setTimeout(function () { URL.revokeObjectURL(url); }, 60000);
      }, "image/png");
    }
    gamePick.addEventListener("change", chooseGame);
    fontPick.addEventListener("change", chooseFont);
    [sizePick, colourPick, backPick, zoomPick].forEach(function (s) { s.addEventListener("change", draw); });
    var typing = 0;
    text.addEventListener("input", function () { clearTimeout(typing); typing = setTimeout(draw, 120); });
    // the rows of characters fit the sheet's width: drawn again when the window opens to its size or is resized
    var drawnWidth = 0;
    if (window.ResizeObserver) new ResizeObserver(function () {
      var w = sheet.clientWidth;
      if (!w || w === drawnWidth) return;
      drawnWidth = w;
      clearTimeout(typing); typing = setTimeout(draw, 120);
    }).observe(sheet);
    save.addEventListener("click", saveAs);
    function show() {
      var list = fillGames();
      if (!game || !list.some(function (g) { return g.id === game.id; })) return chooseGame();
      return Promise.resolve();
    }
    // its pictures let go while it is closed
    function hide() { busy++; if (files) { files.close(); files = null; } game = null; font = null; fonts = []; sheet.textContent = ""; facts.textContent = ""; }
    return { el: el, show: show, hide: hide, redraw: draw, state: function () { return { game: game && game.id, fonts: fonts, font: font }; } };
  }

  // The window on the desktop (web/index.html), closed at first; at(left, vw) gives its place
  function attach(desk, at) {
    if (win || !document.getElementById("games-title")) return null;
    var games = Array.prototype.map.call(document.querySelectorAll("a.tile[data-game]"), function (t) {
      return { id: t.getAttribute("data-game"), title: t.querySelector(".name").textContent, persist: t.getAttribute("data-persist") };
    }).sort(function (a, b) { return a.title.localeCompare(b.title); });
    win = build(games);
    var foot = document.querySelector("body > footer");
    if (foot && foot.parentNode) foot.parentNode.insertBefore(win.el, foot); else document.body.appendChild(win.el);
    desk.add(win.el, { id: "fontview", title: T("fontview.title"), closable: true, closed: true, at: at, scroll: ".fv-sheet",
      minWidth: 480, minHeight: 300 });
    var open = false;
    function watch() {
      // (minimized, it keeps what it shows)
      var now = !win.el.classList.contains("d95-closed") && document.documentElement.classList.contains("d95-desk");
      if (now === open) return;
      open = now;
      if (open) win.show(); else win.hide();
    }
    if (window.MutationObserver) {
      new MutationObserver(watch).observe(win.el, { attributes: true, attributeFilter: ["class"] });
      new MutationObserver(watch).observe(document.documentElement, { attributes: true, attributeFilter: ["class"] });
    }
    watch();
    return win;
  }

  return { attach: attach, setSource: function (fn) { source = fn; }, window: function () { return win; },
    descriptorText: descriptorText, commandLines: commandLines, parseToList: function (s) { return parseToList(s, { pos: 0 }, false); },
    unquote: unquote, toInt: toInt, toDouble: toDouble, pathFrom: pathFrom, parseDescriptor: parseDescriptor, parseLegacy: parseLegacy,
    sizes: sizes, tagsOf: tagsOf, activate: activate, layoutLine: layoutLine, render: render, charset: charset, sniff: sniff,
    findFonts: findFonts, openGame: openGame, pictureFiles: pictureFiles, legacyPicture: legacyPicture, loadPicture: loadPicture, loadFont: loadFont };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopFontView;
