// The listing page's "Import from an installed game" (docs/WEB_PLAYER.md, "Importing saves from an installed game"): the player brings the
// saves, high scores and settings of an original game that is installed on this computer into this browser. Nothing is uploaded, and
// nothing is read but what the table (platform/saveimport.json, here as saveimport-table.js) lists for the chosen game.
//
// What the page takes in: the folder that holds the game's save files (an install folder, a Steam folder, the folder in C:\ProgramData,
// or the VirtualStore copy of an install) and the .reg files that `reg export` writes for the keys the game may use. It reads the
// .reg files (both "Windows Registry Editor Version 5.00", in UTF-16, and REGEDIT4), merges the VirtualStore key over the machine key
// value by value (what a game that was not run as administrator reads), and turns the values and files the table lists into what the
// port keeps: lines of its registry file (userdata/registry.ini: "SOFTWARE\<key>\[sub\]name=sz:text | dword:123 | hex:0a1b") and
// files of its user folder. It shows what that would add, replace or leave, and what it skipped and why; only the player's yes writes,
// through the same locked, transactional path as a restore (web/saves.js), and never while the game is open.
//
// The pure parts (the .reg reader, the merge, the conversion, the port's registry file, the file matching) are tested under Node
// (tools/saveimporttest.js); platform/SaveImport.cpp does the same for the native games, from the same table.
var ByopopSaveImport = (function () {
  "use strict";
  var table = null;
  var MAX_FILE = 4 * 1024 * 1024, MAX_TOTAL = 32 * 1024 * 1024;

  function setTable(t) { table = t; }
  function getTable() { return table || (typeof ByopopSaveTable !== "undefined" ? ByopopSaveTable : null); }
  function gameEntry(id) { var t = getTable(); return t && t.games && Object.prototype.hasOwnProperty.call(t.games, id) ? t.games[id] : null; }

  // ---------- text ----------
  var CP1252 = { 0x20ac: 0x80, 0x201a: 0x82, 0x0192: 0x83, 0x201e: 0x84, 0x2026: 0x85, 0x2020: 0x86, 0x2021: 0x87, 0x02c6: 0x88, 0x2030: 0x89,
    0x0160: 0x8a, 0x2039: 0x8b, 0x0152: 0x8c, 0x017d: 0x8e, 0x2018: 0x91, 0x2019: 0x92, 0x201c: 0x93, 0x201d: 0x94, 0x2022: 0x95, 0x2013: 0x96,
    0x2014: 0x97, 0x02dc: 0x98, 0x2122: 0x99, 0x0161: 0x9a, 0x203a: 0x9b, 0x0153: 0x9c, 0x017e: 0x9e, 0x0178: 0x9f };
  var CP1252_BACK = null;
  // text -> a string of one char per byte in Windows-1252 (the ANSI text the original games keep), or null when a char has no byte
  function toAnsi(text) {
    var out = "";
    for (var i = 0; i < text.length; i++) {
      var c = text.charCodeAt(i);
      if (c < 0x80 || (c >= 0xa0 && c <= 0xff)) out += text.charAt(i);
      else if (CP1252[c] !== undefined) out += String.fromCharCode(CP1252[c]);
      else if (c >= 0x80 && c < 0xa0) out += text.charAt(i);
      else return null;
    }
    return out;
  }
  // a string of bytes (one char per byte) -> text, Windows-1252
  function fromAnsi(bytes) {
    if (!CP1252_BACK) { CP1252_BACK = {}; Object.keys(CP1252).forEach(function (k) { CP1252_BACK[CP1252[k]] = Number(k); }); }
    var out = "";
    for (var i = 0; i < bytes.length; i++) {
      var b = bytes.charCodeAt(i);
      out += String.fromCharCode(CP1252_BACK[b] !== undefined ? CP1252_BACK[b] : b);
    }
    return out;
  }
  function bytesToBinary(u8) {
    var out = [];
    for (var i = 0; i < u8.length; i += 8192) out.push(String.fromCharCode.apply(null, u8.subarray(i, Math.min(u8.length, i + 8192))));
    return out.join("");
  }
  function binaryToBytes(s) {
    var u8 = new Uint8Array(s.length);
    for (var i = 0; i < s.length; i++) u8[i] = s.charCodeAt(i) & 255;
    return u8;
  }
  function utf16le(bytes, start, end) {
    var out = [];
    for (var i = start; i + 1 < end; i += 2) out.push(String.fromCharCode(bytes[i] | (bytes[i + 1] << 8)));
    return out.join("");
  }
  // lower case of the letters A to Z only, as the port's registry compares (sexy_stricmp in the "C" locale), so that a name with an accent sorts as it does there
  function lc(s) { return s.replace(/[A-Z]+/g, function (m) { return m.toLowerCase(); }); }
  function esc(s) { return s.replace(/[.*+?^${}()|[\]\\\/]/g, "\\$&"); }

  // ---------- the table: patterns ----------
  // A pattern of the table: '*' is any characters inside one part, '#' is digits; the parts are separated by sep. Without regard to case.
  function patternRe(pattern, sep) {
    var src = "", i;
    for (i = 0; i < pattern.length; i++) {
      var c = pattern.charAt(i);
      if (c === "*") src += "([^" + esc(sep) + "]*)";
      else if (c === "#") src += "([0-9]+)";
      else src += esc(c);
    }
    return new RegExp("^" + src + "$", "i");
  }
  // "Name=type" -> {name, type, re}
  function valueRule(spec) {
    var at = spec.lastIndexOf("=");
    return { name: spec.slice(0, at), type: spec.slice(at + 1), re: patternRe(spec.slice(0, at), "\\") };
  }
  // the registry rules of a game: its values and those of the sets it names; {key, hives, rules, skips}
  function registryOf(game) {
    var r = game.registry, t = getTable();
    if (!r) return null;
    var specs = [];
    (r.sets || []).forEach(function (s) { (t.valueSets[s] || []).forEach(function (v) { specs.push(v); }); });
    (r.values || []).forEach(function (v) { specs.push(v); });
    var skips = [];
    [t.skipValues || {}, r.skip || {}].forEach(function (m) {
      Object.keys(m).forEach(function (k) { skips.push({ re: patternRe(k, "\\"), why: m[k] }); });
    });
    return { key: r.key, hives: r.hives.slice(), rules: specs.map(valueRule), skips: skips };
  }
  function fileRules(game) {
    return {
      take: (game.files || []).map(function (f) { return { re: patternRe(f.from, "/"), from: f.from, to: f.to, what: f.what }; }),
      skip: (game.skipFiles || []).map(function (f) { return { re: patternRe(f.from, "/"), from: f.from, why: f.why }; })
    };
  }
  // the target of a file rule: the '*' and '#' of the target take the captures of the source, in order
  function fillTarget(to, caps) {
    var n = 0;
    return to.replace(/[*#]/g, function () { var v = caps[n++]; return v === undefined ? "" : v; });
  }

  // ---------- .reg files ----------
  // Reads a .reg file's bytes: {ok, error, version, events}. Events in file order: {t: "key", key}, {t: "delkey", key},
  // {t: "value", key, name, kind, ...}, {t: "delvalue", key, name}. kind: sz (text), expand (text), dword (num), hex (bytes, a string of one char
  // per byte), multi (bytes), qword (bytes), other (bytes, with type).
  function parseReg(u8) {
    var text, unicode = false;
    if (u8.length >= 2 && u8[0] === 0xff && u8[1] === 0xfe) { text = utf16le(u8, 2, u8.length); unicode = true; }
    else if (u8.length >= 2 && u8[0] === 0xfe && u8[1] === 0xff) return { ok: false, error: ByopopStrings.t("saveimport.reg.byte_order") };
    else if (u8.length >= 3 && u8[0] === 0xef && u8[1] === 0xbb && u8[2] === 0xbf) text = new TextDecoder("utf-8").decode(u8.subarray(3));
    else text = fromAnsi(bytesToBinary(u8));
    var lines = text.split(/\r\n|\n|\r/);
    var first = (lines[0] || "").trim();
    var version;
    if (/^Windows Registry Editor Version 5\.00$/i.test(first)) version = 5;
    else if (/^REGEDIT4$/i.test(first)) version = 4;
    else return { ok: false, error: ByopopStrings.t("saveimport.reg.not_reg") };
    var wide = version === 5 || unicode;
    var events = [], key = null, bad = 0, i;
    // logical lines: a line that ends in a backslash goes on in the next one (hex values)
    var logical = [];
    for (i = 1; i < lines.length; i++) {
      var ln = lines[i];
      while (/\\\s*$/.test(ln) && /=\s*hex(\([0-9a-fA-F]+\))?:/i.test(ln) && i + 1 < lines.length) ln = ln.replace(/\\\s*$/, "") + lines[++i].replace(/^\s+/, "");
      logical.push(ln);
    }
    function unq(s) { return s.replace(/\\(.)/g, "$1"); }
    logical.forEach(function (raw) {
      var ln = raw.trim();
      if (!ln || ln.charAt(0) === ";" || ln.charAt(0) === "#") return;
      var m;
      if (ln.charAt(0) === "[") {
        m = /^\[(-?)(.*)\]$/.exec(ln);
        if (!m) { bad++; return; }
        key = m[2];
        events.push({ t: m[1] ? "delkey" : "key", key: key });
        return;
      }
      if (key === null) { bad++; return; }
      m = /^(@|"((?:[^"\\]|\\.)*)")\s*=\s*(.*)$/.exec(ln);
      if (!m) { bad++; return; }
      var name = m[1] === "@" ? "" : unq(m[2]), data = m[3], d;
      if (data === "-") { events.push({ t: "delvalue", key: key, name: name }); return; }
      if ((d = /^"((?:[^"\\]|\\.)*)"$/.exec(data))) { events.push({ t: "value", key: key, name: name, kind: "sz", text: unq(d[1]) }); return; }
      if ((d = /^dword:([0-9a-fA-F]{1,8})$/i.exec(data))) { events.push({ t: "value", key: key, name: name, kind: "dword", num: parseInt(d[1], 16) | 0 }); return; }
      if ((d = /^hex(?:\(([0-9a-fA-F]+)\))?:(.*)$/i.exec(data))) {
        var type = d[1] === undefined ? 3 : parseInt(d[1], 16), parts = d[2].split(","), raw8 = [], ok = true;
        for (var k = 0; k < parts.length; k++) {
          var p = parts[k].trim();
          if (!p) continue;
          if (!/^[0-9a-fA-F]{1,2}$/.test(p)) { ok = false; break; }
          raw8.push(parseInt(p, 16));
        }
        if (!ok) { bad++; return; }
        var bytes = "", n;
        for (n = 0; n < raw8.length; n += 8192) bytes += String.fromCharCode.apply(null, raw8.slice(n, n + 8192));
        if (type === 3) events.push({ t: "value", key: key, name: name, kind: "hex", bytes: bytes });
        else if (type === 7) events.push({ t: "value", key: key, name: name, kind: "multi", bytes: bytes });
        else if (type === 11) events.push({ t: "value", key: key, name: name, kind: "qword", bytes: bytes });
        else if (type === 1 || type === 2) {
          // text: UTF-16 in a version 5 file, ANSI in a REGEDIT4 one; ends at the first NUL
          var txt;
          if (wide) { var u = new Uint8Array(raw8); txt = utf16le(u, 0, u.length); }
          else txt = fromAnsi(bytes);
          var z = txt.indexOf("\u0000");
          if (z >= 0) txt = txt.slice(0, z);
          events.push({ t: "value", key: key, name: name, kind: type === 1 ? "sz" : "expand", text: txt });
        }
        else events.push({ t: "value", key: key, name: name, kind: "other", type: type, bytes: bytes });
        return;
      }
      bad++;
    });
    return { ok: true, version: version, events: events, unreadable: bad };
  }

  // Which of a game's places a key of a .reg file is: {hive, rel} (rel: the path below the game's key, "" for the key itself) or null
  function hiveOf(keyPath, regKey) {
    var k = esc(regKey), m;
    m = new RegExp("^HKEY_CURRENT_USER\\\\Software\\\\Classes\\\\VirtualStore\\\\MACHINE\\\\SOFTWARE\\\\(?:WOW6432Node\\\\)?" + k + "(?:\\\\(.*))?$", "i").exec(keyPath);
    if (m) return { hive: "vs", rel: m[1] || "" };
    m = new RegExp("^HKEY_LOCAL_MACHINE\\\\SOFTWARE\\\\(?:WOW6432Node\\\\)?" + k + "(?:\\\\(.*))?$", "i").exec(keyPath);
    if (m) return { hive: "hklm", rel: m[1] || "" };
    m = new RegExp("^HKEY_CURRENT_USER\\\\Software\\\\" + k + "(?:\\\\(.*))?$", "i").exec(keyPath);
    if (m) return { hive: "hkcu", rel: m[1] || "" };
    return null;
  }

  // Applies the events of parsed .reg files to one map for each place: {vs, hklm, hkcu}: Map of lower-case path -> {path, kind, text|num|bytes}.
  // Also {foreign: keys of the files that are not this game's}, {unreadable: lines that could not be read}.
  function collect(game, parsedFiles) {
    var reg = registryOf(game), maps = { vs: {}, hklm: {}, hkcu: {} }, foreign = [], unreadable = 0, seenForeign = {};
    if (!reg) return { maps: maps, foreign: foreign, unreadable: 0, unused: [] };
    var unused = {};
    parsedFiles.forEach(function (pf) {
      unreadable += pf.unreadable || 0;
      (pf.events || []).forEach(function (ev) {
        var h = hiveOf(ev.key, reg.key);
        if (!h) {
          var short = ev.key.slice(0, 140);
          if (ev.t !== "delkey" && !seenForeign[lc(short)]) { seenForeign[lc(short)] = true; foreign.push(short); }
          return;
        }
        if (reg.hives.indexOf(h.hive) < 0) { unused[h.hive] = true; return; }
        var map = maps[h.hive];
        if (ev.t === "delkey") {
          var pre = lc(h.rel) + "\\";
          Object.keys(map).forEach(function (lk) { if (!h.rel || lk.indexOf(pre) === 0) delete map[lk]; });
        } else if (ev.t === "delvalue") {
          delete map[lc(h.rel ? h.rel + "\\" + ev.name : ev.name)];
        } else if (ev.t === "value") {
          var p = h.rel ? h.rel + "\\" + ev.name : ev.name, v = { path: p, kind: ev.kind };
          if (ev.text !== undefined) v.text = ev.text;
          if (ev.num !== undefined) v.num = ev.num;
          if (ev.bytes !== undefined) v.bytes = ev.bytes;
          if (ev.type !== undefined) v.type = ev.type;
          map[lc(p)] = v;
        }
      });
    });
    return { maps: maps, foreign: foreign, unreadable: unreadable, unused: Object.keys(unused) };
  }

  // ---------- the port's registry file ----------
  function escapeSz(s) { return s.replace(/\\/g, "\\\\").replace(/\n/g, "\\n").replace(/\r/g, "\\r"); }
  function hex(bytes) {
    var out = [];
    for (var i = 0; i < bytes.length; i++) { var b = bytes.charCodeAt(i) & 255; out.push((b < 16 ? "0" : "") + b.toString(16)); }
    return out.join("");
  }
  // A value of the original in the port's form, for the table's type: {value: "dword:1", shown: "1"} or {skip: reason}
  function convert(v, type) {
    if (type === "sz") {
      if (v.kind !== "sz" && v.kind !== "expand") return { skip: ByopopStrings.t("saveimport.skip.not_text", {kind: kindName(v)}) };
      var a = toAnsi(v.text);
      if (a === null) return { skip: ByopopStrings.t("saveimport.skip.charset") };
      var z = a.indexOf("\u0000");
      if (z >= 0) a = a.slice(0, z);
      return { value: "sz:" + escapeSz(a), shown: "“" + (a.length > 40 ? a.slice(0, 40) + "…" : a) + "”" };
    }
    if (type === "dword") {
      if (v.kind !== "dword") return { skip: ByopopStrings.t("saveimport.skip.not_number", {kind: kindName(v)}) };
      return { value: "dword:" + String(v.num | 0), shown: String(v.num | 0) };
    }
    if (type === "hex") {
      if (v.kind !== "hex" && v.kind !== "multi") return { skip: ByopopStrings.t("saveimport.skip.not_binary", {kind: kindName(v)}) };
      return { value: "hex:" + hex(v.bytes), shown: ByopopStrings.t("saveimport.shown_binary", {n: v.bytes.length}) };
    }
    return { skip: ByopopStrings.t("saveimport.skip.unknown_type") };
  }
  function kindName(v) {
    var key = { sz: "text", expand: "text", dword: "dword", hex: "binary", multi: "binary", qword: "qword", other: "other" }[v.kind] || "unknown";
    return ByopopStrings.t("saveimport.kind." + key);
  }

  // The port's registry file (userdata/registry.ini): a string of one char per byte -> Map of lower-case full path -> {full, value}
  function parseIni(text) {
    var map = {}, section = null;
    (text || "").split("\n").forEach(function (line) {
      if (line.charAt(line.length - 1) === "\r") line = line.slice(0, -1);
      if (!line || line.charAt(0) === ";") return;
      if (line.charAt(0) === "[") {
        var close = line.lastIndexOf("]");
        section = line.slice(1, close < 0 ? undefined : close);
        return;
      }
      var eq = line.indexOf("=");
      if (eq >= 0 && section) { var full = section + "\\" + line.slice(0, eq); map[lc(full)] = { full: full, value: line.slice(eq + 1) }; }
    });
    return map;
  }
  // The same order as the port's std::map (a byte-wise compare of the lower-cased text)
  function byLower(a, b) { var x = lc(a), y = lc(b); return x < y ? -1 : x > y ? 1 : 0; }
  // The port's text for a registry (SexyAppBase::GetRegistryText): the entries grouped by key, in its order
  function writeIni(map) {
    var entries = Object.keys(map).map(function (k) { return map[k]; }), keys = {};
    entries.sort(function (a, b) { return byLower(a.full, b.full); });
    entries.forEach(function (e) {
      var at = e.full.lastIndexOf("\\");
      if (at < 0) return;
      var sec = e.full.slice(0, at);
      (keys[lc(sec)] || (keys[lc(sec)] = { name: sec, lines: "" })).lines += e.full.slice(at + 1) + "=" + e.value + "\n";
    });
    var out = "; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n";
    Object.keys(keys).sort().forEach(function (lk) { out += "\n[" + keys[lk].name + "]\n" + keys[lk].lines; });
    return out;
  }

  // What the registry values of the original would do to the port's registry: the merge of the places in the game's order of preference, value
  // by value; then each value is matched against the table. ini: the port's current registry file (a string of bytes), or "".
  // {items: [{path, full, hive, value, shown, status: "new"|"changes"|"same", old}], skipped: [{path, hive, why}], overridden: n, mergedInto}
  function planRegistry(game, collected, ini) {
    var reg = registryOf(game);
    if (!reg) return { items: [], skipped: [], overridden: 0, ini: ini || "", map: parseIni(ini) };
    var merged = {}, overridden = 0;
    reg.hives.slice().reverse().forEach(function (h) {
      var m = collected.maps[h] || {};
      Object.keys(m).forEach(function (lk) {
        if (merged[lk]) overridden++;
        merged[lk] = { v: m[lk], hive: h };
      });
    });
    var cur = parseIni(ini), items = [], skipped = [];
    Object.keys(merged).sort().forEach(function (lk) {
      var e = merged[lk], v = e.v, rule = null, i;
      for (i = 0; i < reg.rules.length && !rule; i++) if (reg.rules[i].re.test(v.path)) rule = reg.rules[i];
      if (!rule) {
        var why = ByopopStrings.t("saveimport.skip.not_kept");
        for (i = 0; i < reg.skips.length; i++) if (reg.skips[i].re.test(v.path)) { why = reg.skips[i].why; break; }
        skipped.push({ path: v.path, hive: e.hive, why: why });
        return;
      }
      var c = convert(v, rule.type), bad = badName(v.path);
      if (bad) c = { skip: bad };
      if (c.skip) { skipped.push({ path: v.path, hive: e.hive, why: c.skip }); return; }
      var full = "SOFTWARE\\" + reg.key + "\\" + v.path, old = cur[lc(full)];
      items.push({ path: v.path, full: old ? old.full : full, hive: e.hive, value: c.value, shown: c.shown,
        status: !old ? "new" : old.value === c.value ? "same" : "changes", old: old ? old.value : null });
    });
    return { items: items, skipped: skipped, overridden: overridden, map: cur };
  }
  // Why the port's registry file could not hold a value of this name (it is a line "name=value" in a section), or null
  function badName(path) {
    var leaf = path.slice(path.lastIndexOf("\\") + 1);
    if (/[\u0000-\u001f]/.test(path)) return ByopopStrings.t("saveimport.skip.name_control");
    if (leaf.indexOf("=") >= 0 || leaf.charAt(0) === "[" || leaf.charAt(0) === ";") return ByopopStrings.t("saveimport.skip.name_char");
    return null;
  }
  // The registry file after the import
  function applyRegistry(plan) {
    var map = {};
    Object.keys(plan.map).forEach(function (k) { map[k] = plan.map[k]; });
    plan.items.forEach(function (it) { map[lc(it.full)] = { full: it.full, value: it.value }; });
    return writeIni(map);
  }

  // ---------- files ----------
  // A path inside the user folder: relative, no empty, "." or ".." part, no backslash or control character, not too long (as web/saves.js)
  function safeRel(rel) {
    if (typeof rel !== "string" || !rel || rel.length > 240) return false;
    if (/[\\\u0000-\u001f]/.test(rel) || rel.charAt(0) === "/" || rel.charAt(rel.length - 1) === "/") return false;
    return rel.split("/").every(function (p) { return p !== "" && p !== "." && p !== ".."; });
  }
  // folders: [[{rel, size, m, read()}]] in order of preference (a later folder wins). Matches the table's file patterns.
  // {take: [{to, from, what, folder, size, read, m}], skipped: [{rel, why}], ignored: n (files of the folders that the table does not list)}
  function planFiles(game, folders) {
    var rules = fileRules(game), take = {}, skipped = [], ignored = 0, skipSeen = {};
    folders.forEach(function (files, fi) {
      files.forEach(function (f) {
        var matched = false, i, m;
        for (i = 0; i < rules.take.length && !matched; i++) {
          m = rules.take[i].re.exec(f.rel);
          if (!m) continue;
          matched = true;
          var to = fillTarget(rules.take[i].to, m.slice(1));
          if (!safeRel(to)) { skipped.push({ rel: f.rel, why: ByopopStrings.t("saveimport.skip.path") }); continue; }
          take[lc(to)] = { to: to, from: f.rel, what: rules.take[i].what, folder: fi, size: f.size, read: f.read, m: f.m };
        }
        for (i = 0; i < rules.skip.length && !matched; i++) {
          if (rules.skip[i].re.test(f.rel)) {
            matched = true;
            if (!skipSeen[lc(f.rel)]) { skipSeen[lc(f.rel)] = true; skipped.push({ rel: f.rel, why: rules.skip[i].why }); }
          }
        }
        if (!matched) ignored++;
      });
    });
    return { take: Object.keys(take).sort().map(function (k) { return take[k]; }), skipped: skipped, ignored: ignored };
  }

  // ---------- what to tell the player to run ----------
  var HIVE_NAMES = { vs: ByopopStrings.t("saveimport.hive.vs"), hklm: ByopopStrings.t("saveimport.hive.hklm"), hkcu: ByopopStrings.t("saveimport.hive.hkcu") };
  var HIVE_SUFFIX = { vs: "vs", hklm: "hklm", hkcu: "hkcu" };
  // The exact `reg export` command for a place of a game's key: reads only, and needs no administrator rights
  function exportCommand(id, regKey, hive) {
    var key = hive === "vs" ? "HKCU\\Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\WOW6432Node\\" + regKey
      : hive === "hklm" ? "HKLM\\SOFTWARE\\WOW6432Node\\" + regKey : "HKCU\\Software\\" + regKey;
    return 'reg export "' + key + '" ' + id + "-" + HIVE_SUFFIX[hive] + ".reg /y";
  }
  function commands(id) {
    var g = gameEntry(id);
    if (!g || !g.registry) return [];
    return g.registry.hives.map(function (h) { return { hive: h, text: exportCommand(id, g.registry.key, h), file: id + "-" + HIVE_SUFFIX[h] + ".reg" }; });
  }

  var core = { setTable: setTable, getTable: getTable, gameEntry: gameEntry, toAnsi: toAnsi, parseReg: parseReg, hiveOf: hiveOf, collect: collect, convert: convert,
    safeRel: safeRel, badName: badName, parseIni: parseIni, writeIni: writeIni, planRegistry: planRegistry, applyRegistry: applyRegistry, planFiles: planFiles, commands: commands,
    exportCommand: exportCommand, registryOf: registryOf, patternRe: patternRe, bytesToBinary: bytesToBinary, binaryToBytes: binaryToBytes, HIVE_NAMES: HIVE_NAMES };
  if (typeof document === "undefined") return core;

  // ---------- the section ----------
  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text !== undefined && text !== null) e.textContent = text;
    return e;
  }
  // n things, in the count string key ("saveimport.n.file": "{n} file | {n} files")
  function plural(n, key) { return ByopopStrings.t(key, {n: n}); }
  var uid = 0;
  function nextId(p) { return p + "-" + (++uid); }

  // root: the saves section's body; games: [{id, title, persist, lock, replays}]; ctx: the save backup's helpers (web/saves.js)
  function init(root, games, ctx) {
    var t = getTable();
    if (!t) return;
    var list = games.filter(function (g) { return gameEntry(g.id); }), byId = {};
    list.forEach(function (g) { byId[g.id] = g; });
    if (!list.length) return;

    // A wizard, as Windows 95's (the user, 2026-11-08: "values, not sentences"): the picture at the left, one step a page (a
    // heading, the fields it needs, at most one short line), and Back, Next and Cancel. The pages: the game; its save folders
    // (when it keeps files); the registry commands and then the .reg files (when it keeps values there); what would be imported,
    // where Next is Import. A fact a field needs is its tooltip and its description.
    root.appendChild(make("h3", "", ByopopStrings.t("saveimport.title")));
    var wiz = make("div", "si-wiz");
    var art = make("div", "si-art");
    art.setAttribute("aria-hidden", "true");
    art.appendChild(make("i", "si-art-disk"));
    var pagesEl = make("div", "si-pages");
    wiz.appendChild(art); wiz.appendChild(pagesEl);
    root.appendChild(wiz);

    function page(title) {
      var pg = make("section", "si-page");
      var h = make("h4", "si-page-title", title);
      h.id = nextId("si-page");
      pg.setAttribute("aria-labelledby", h.id);
      pg.appendChild(h);
      pg.hidden = true;
      return pg;
    }
    var gamePage = page(ByopopStrings.t("saveimport.page.game"));
    gamePage.appendChild(make("p", "si-line", ByopopStrings.t("saveimport.about")));
    var form = make("div", "pl-form");
    var selId = "si-game";
    var selLabel = make("label", "", ByopopStrings.t("saveimport.game_label"));
    selLabel.htmlFor = selId;
    var sel = make("select");
    sel.id = selId;
    var first = make("option", "", ByopopStrings.t("saveimport.choose_game"));
    first.value = "";
    sel.appendChild(first);
    list.forEach(function (g) { var o = make("option", "", g.title); o.value = g.id; sel.appendChild(o); });
    form.appendChild(selLabel); form.appendChild(sel);
    gamePage.appendChild(form);
    gamePage.hidden = false;
    pagesEl.appendChild(gamePage);

    // the pages a game's entry needs after the first, made when the game is chosen
    var help = make("div", "si-help");
    var pickers = make("div", "si-pickers");
    var reviewPage = page(ByopopStrings.t("saveimport.page.review"));
    var box = make("div", "pl-confirm sv-confirm si-confirm");
    box.hidden = true;
    var takes = make("div", "si-takes");
    reviewPage.appendChild(box); reviewPage.appendChild(takes);
    pagesEl.appendChild(help); pagesEl.appendChild(pickers); pagesEl.appendChild(reviewPage);
    var pages = [gamePage], at = 0;

    var nav = make("div", "si-nav");
    function navButton(key) { var b = make("button", "", ByopopStrings.t(key)); b.type = "button"; nav.appendChild(b); return b; }
    var backBtn = navButton("saveimport.back"), nextBtn = navButton("saveimport.next"), cancelBtn = navButton("saveimport.cancel");
    root.appendChild(nav);
    var report = make("div", "si-report");
    root.appendChild(report);

    function onReview() { return pages[at] === reviewPage; }
    function syncNav() {
      backBtn.disabled = at === 0 || ctx.isBusy();
      nextBtn.textContent = ByopopStrings.t(onReview() ? "saveimport.plan.yes" : "saveimport.next");
      nextBtn.disabled = ctx.isBusy() || (at === 0 && !chosen) || (onReview() && !(plan && plan.todo && !plan.blocked));
      cancelBtn.disabled = ctx.isBusy();
    }
    function go(n) {
      pages[at].hidden = true;
      at = Math.max(0, Math.min(n, pages.length - 1));
      pages[at].hidden = false;
      if (onReview()) refresh();
      syncNav();
      var h = pages[at].querySelector(".si-page-title");
      if (h) { h.tabIndex = -1; h.focus(); }
    }
    backBtn.addEventListener("click", function () { go(at - 1); });
    nextBtn.addEventListener("click", function () { if (onReview()) doImport(); else go(at + 1); });
    cancelBtn.addEventListener("click", function () { cancel(); });

    var chosen = null;          // the game's id
    var picks = { folder1: [], folder2: [], regs: [] };    // what the player chose: File lists
    var plan = null;            // the plan waiting for the player's yes
    var copyStatus = make("p", "pl-sr");
    copyStatus.setAttribute("role", "status");
    copyStatus.setAttribute("aria-live", "polite");
    root.appendChild(copyStatus);

    // the box is a dialog only while its question is shown (its name and text are made with it)
    function resetPlan() {
      plan = null; box.hidden = true; box.textContent = "";
      ["role", "aria-labelledby", "aria-describedby"].forEach(function (a) { box.removeAttribute(a); });
      syncNav();
    }

    sel.addEventListener("change", function () {
      chosen = sel.value || null;
      picks = { folder1: [], folder2: [], regs: [] };
      resetPlan();
      report.textContent = "";
      renderHelp();
      syncNav();
    });

    function copyText(text, what) {
      function done(ok) { copyStatus.textContent = ByopopStrings.t(ok ? "saveimport.copied" : "saveimport.copy_failed", {what: what}); }
      // inside the click, the synchronous way first (it needs no permission); the clipboard API if that does not work
      var prev = document.activeElement, ta = make("textarea"), ok = false;
      ta.value = text; ta.setAttribute("readonly", ""); ta.setAttribute("aria-hidden", "true"); ta.style.position = "fixed"; ta.style.left = "-9999px";
      document.body.appendChild(ta);
      ta.select();
      try { ok = document.execCommand("copy"); } catch (e) {}
      ta.remove();
      if (prev && prev.focus) prev.focus();
      if (ok) { done(true); return; }
      try {
        if (navigator.clipboard && navigator.clipboard.writeText) { navigator.clipboard.writeText(text).then(function () { done(true); }, function () { done(false); }); return; }
      } catch (e) {}
      done(false);
    }

    // a fact as a field's tooltip and its description, for screen readers too
    function tipFor(el, text) {
      el.title = text;
      var d = make("span", "visually-hidden", text);
      d.id = nextId("si-tip");
      el.setAttribute("aria-describedby", ((el.getAttribute("aria-describedby") || "") + " " + d.id).trim());
      el.parentNode.appendChild(d);
    }
    function renderHelp() {
      help.textContent = ""; pickers.textContent = ""; takes.textContent = "";
      pages.slice(1).forEach(function (pg) { if (pg !== reviewPage && pg.parentNode) pg.parentNode.removeChild(pg); });
      pages = [gamePage];
      at = 0; gamePage.hidden = false; reviewPage.hidden = true;
      if (!chosen) { syncNav(); return; }
      var e = gameEntry(chosen);
      function picker(pg, id, label, accept, multiple, dir, key, tip) {
        var row = make("div", "si-row");
        var lab = make("label", "", label);
        lab.htmlFor = id;
        var inp = make("input");
        inp.type = "file"; inp.id = id;
        if (accept) inp.accept = accept;
        if (multiple) inp.multiple = true;
        if (dir) { inp.setAttribute("webkitdirectory", ""); inp.setAttribute("directory", ""); }
        row.appendChild(lab); row.appendChild(inp);
        pg.appendChild(row);
        if (tip) tipFor(inp, tip);
        inp.addEventListener("change", function () { picks[key] = Array.prototype.slice.call(inp.files || []); refresh(); });
        return inp;
      }
      // the save folders: the files the game keeps, where to look (each place a path, what it is in its tooltip), the two fields
      if (e.files && e.files.length) {
        var fp = page(ByopopStrings.t("saveimport.page.files"));
        var ul = make("ul", "sv-list si-files");
        e.files.forEach(function (f) {
          var li = make("li");
          li.appendChild(make("code", "", f.from.replace(/\//g, "\\")));
          li.appendChild(document.createTextNode(ByopopStrings.t("saveimport.file_what", {what: f.what})));
          ul.appendChild(li);
        });
        fp.appendChild(ul);
        var places = make("dl", "si-places");
        function place(key, path) {
          var dt = make("dt", "", ByopopStrings.t("saveimport.loc." + key)), dd = make("dd");
          dd.appendChild(make("code", "", path));
          places.appendChild(dt); places.appendChild(dd);
          tipFor(dd, ByopopStrings.t("saveimport.loc." + key + "_tip"));
        }
        fp.appendChild(places);
        place("install", "C:\\Program Files (x86)\\PopCap Games\\" + e.folder);
        if (e.steam) place("steam", "steamapps\\common\\" + e.steam);
        place("vs", "%LOCALAPPDATA%\\VirtualStore\\Program Files (x86)\\PopCap Games\\" + e.folder);
        if (e.appdata) place("appdata", "C:\\ProgramData\\PopCap Games\\" + e.appdata);
        picker(fp, "si-folder1", ByopopStrings.t("saveimport.folder1.label"), null, false, true, "folder1", ByopopStrings.t("saveimport.folder1.hint"));
        picker(fp, "si-folder2", ByopopStrings.t("saveimport.folder2.label"), null, false, true, "folder2", ByopopStrings.t("saveimport.folder2.hint"));
        help.appendChild(fp); pages.push(fp);
      }
      // the registry: the commands, each with its Copy, then a page for the files they wrote
      if (e.registry) {
        var rp = page(ByopopStrings.t("saveimport.page.registry"));
        rp.appendChild(make("p", "si-line", ByopopStrings.t("saveimport.registry.line")));
        var cmds = commands(chosen), box2 = make("div", "si-cmds");
        cmds.forEach(function (c) {
          var row = make("div", "si-cmd");
          var pre = make("pre", "si-pre"), code = make("code", "", c.text);
          pre.appendChild(code);
          var btn = make("button", "", ByopopStrings.t("saveimport.copy"));
          btn.type = "button";
          btn.setAttribute("aria-label", ByopopStrings.t("saveimport.copy_label", {hive: HIVE_NAMES[c.hive], file: c.file}));
          btn.addEventListener("click", function () { copyText(c.text, ByopopStrings.t("saveimport.copied_command", {hive: HIVE_NAMES[c.hive]})); });
          row.appendChild(pre); row.appendChild(btn);
          box2.appendChild(row);
        });
        rp.appendChild(box2);
        var all = make("button", "", ByopopStrings.t("saveimport.copy_all"));
        all.type = "button";
        all.addEventListener("click", function () { copyText(cmds.map(function (c) { return c.text; }).join("\r\n"), ByopopStrings.t("saveimport.copied_all", {count: cmds.length})); });
        var allRow = make("div", "si-cmd-all");
        allRow.appendChild(all);
        rp.appendChild(allRow);
        var order = e.registry.hives.map(function (hv) { return HIVE_NAMES[hv]; });
        tipFor(box2, [ByopopStrings.t(order.length > 1 ? "saveimport.order_many" : "saveimport.order_one",
          {order: order.join(", " + ByopopStrings.t("saveimport.then") + " ")}), ByopopStrings.t("saveimport.registry.not_found")].join(" "));
        help.appendChild(rp); pages.push(rp);
        var gp = page(ByopopStrings.t("saveimport.page.regs"));
        picker(gp, "si-regs", ByopopStrings.t("saveimport.regs.label"), ".reg,text/plain", true, false, "regs", ByopopStrings.t("saveimport.regs.hint"));
        pickers.appendChild(gp); pages.push(gp);
      }
      pages.push(reviewPage);
      // what the page takes from the original, folded away on the last page
      var tk = make("details", "pl-details");
      tk.appendChild(make("summary", "", ByopopStrings.t("saveimport.takes")));
      var tul = make("ul", "sv-list");
      if (e.registry) {
        var rg = registryOf(e), names = rg.rules.map(function (r) { return r.name; });
        tul.appendChild(make("li", "", ByopopStrings.t("saveimport.takes_registry", {key: e.registry.key, names: names.join(", ")})));
      }
      (e.files || []).forEach(function (f) { tul.appendChild(make("li", "", ByopopStrings.t("saveimport.takes_file", {from: f.from.replace(/\//g, "\\"), what: f.what}))); });
      (e.skipFiles || []).forEach(function (f) { tul.appendChild(make("li", "", ByopopStrings.t("saveimport.takes_skip", {from: f.from.replace(/\//g, "\\"), why: f.why}))); });
      tk.appendChild(tul);
      takes.appendChild(tk);
      syncNav();
    }

    // ----- reading the choices -----
    function folderFiles(files) {
      return files.map(function (f) {
        var rel = (f.webkitRelativePath || f.name).replace(/\\/g, "/").split("/");
        if (f.webkitRelativePath) rel.shift();
        return { rel: rel.join("/"), size: f.size, m: f.lastModified, file: f, read: function () { return f.arrayBuffer().then(function (b) { return new Uint8Array(b); }); } };
      });
    }
    var token = 0;
    function refresh() {
      resetPlan();
      report.textContent = "";
      if (!chosen) return;
      if (!picks.folder1.length && !picks.folder2.length && !picks.regs.length) {
        if (onReview()) { box.hidden = false; box.appendChild(make("p", "si-line", ByopopStrings.t("saveimport.choose_first"))); }
        return;
      }
      var my = ++token;
      ctx.say(ByopopStrings.t("saveimport.reading"));
      build().then(function (p) {
        if (my !== token) return;
        plan = p;
        showPlan();
      }, function (err) { if (my === token) ctx.say(err && err.message ? err.message : String(err), true); });
    }

    // The current registry file and user files of the game in this browser, as the game would find them at its next start
    function currentState(g) {
      return ctx.databases().then(function (names) {
        return ctx.openExisting(g.persist, names).then(function (db) {
          var item = ctx.lsGet(ctx.backupKey(g.persist));
          var load = db ? ctx.readFolder(db, g.persist, "userdata").then(function (x) { db.close(); return x; }, function (er) { db.close(); throw er; }) : Promise.resolve({});
          return load.then(function (files) {
            var m = ctx.merged(files, item);
            return { names: names, files: files, merged: m, item: item };
          });
        });
      });
    }

    function build() {
      var g = byId[chosen], e = gameEntry(chosen);
      return Promise.all([currentState(g), Promise.all(picks.regs.map(function (f) {
        return f.arrayBuffer().then(function (b) { var r = parseReg(new Uint8Array(b)); r.name = f.name; return r; });
      }))]).then(function (r) {
        var cur = r[0], parsed = r[1];
        var out = { game: g, cur: cur, parsed: parsed, notes: [], reg: null, files: null, bad: [] };
        parsed.forEach(function (p) { if (!p.ok) out.bad.push(ByopopStrings.t("saveimport.bad", {name: p.name, error: p.error})); });
        var good = parsed.filter(function (p) { return p.ok; });
        if (e.registry) {
          var col = collect(e, good), ini = cur.merged.files["registry.ini"] ? bytesToBinary(cur.merged.files["registry.ini"].bytes) : "";
          out.reg = planRegistry(e, col, ini);
          out.col = col;
          if (good.length && !out.reg.items.length && !out.reg.skipped.length) out.notes.push(ByopopStrings.t("saveimport.note.no_key", {title: g.title, key: e.registry.key}));
          if (col.foreign.length) out.notes.push(ByopopStrings.t("saveimport.note.foreign", {n: col.foreign.length}));
          if (col.unused.length) out.notes.push(ByopopStrings.t("saveimport.note.unused", {hives: col.unused.map(function (h) { return HIVE_NAMES[h]; }).join(" " + ByopopStrings.t("saveimport.and") + " ")}));
          if (col.unreadable) out.notes.push(ByopopStrings.t("saveimport.note.unreadable", {n: col.unreadable}));
        }
        if (e.files && e.files.length) {
          var folders = [folderFiles(picks.folder1), folderFiles(picks.folder2)];
          out.files = planFiles(e, folders);
          var total = 0, tooBig = [];
          out.files.take = out.files.take.filter(function (f) {
            if (f.size > MAX_FILE || total + f.size > MAX_TOTAL) { tooBig.push(f.to); return false; }
            total += f.size;
            return true;
          });
          out.tooBig = tooBig;
          if ((picks.folder1.length || picks.folder2.length) && !out.files.take.length && !out.files.skipped.length)
            out.notes.push(ByopopStrings.t(picks.folder1.length && picks.folder2.length ? "saveimport.note.no_files_two" : "saveimport.note.no_files", {title: g.title}));
          return Promise.all(out.files.take.map(function (f) {
            return f.read().then(function (b) {
              f.bytes = b;
              var rel = null;
              Object.keys(cur.merged.files).forEach(function (k) { if (lc(k) === lc(f.to)) rel = k; });
              f.target = rel || f.to;
              var old = rel ? cur.merged.files[rel].bytes : null;
              f.status = !old ? "new" : sameBytes(old, b) ? "same" : "changes";
            });
          })).then(function () { return out; });
        }
        return out;
      });
    }
    function sameBytes(a, b) {
      if (a.length !== b.length) return false;
      for (var i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
      return true;
    }

    var STATUS = { "new": "saveimport.status.new", changes: "saveimport.status.changes", same: "saveimport.status.same" };

    function showPlan() {
      var g = plan.game, e = gameEntry(g.id), openNow = null;
      box.textContent = "";
      box.hidden = false;
      var items = (plan.reg ? plan.reg.items : []), files = (plan.files ? plan.files.take : []);
      var todo = items.filter(function (i) { return i.status !== "same"; }).length + files.filter(function (f) { return f.status !== "same"; }).length;
      box.setAttribute("role", "alertdialog");
      box.setAttribute("aria-labelledby", "si-confirm-title");
      box.setAttribute("aria-describedby", "si-confirm-desc");
      var h = make("h4", "", ByopopStrings.t(todo ? "saveimport.plan.title" : "saveimport.plan.title_none", {title: g.title}));
      h.id = "si-confirm-title";
      box.appendChild(h);
      var desc = make("p", "", ByopopStrings.t(todo ? "saveimport.plan.desc" : "saveimport.plan.desc_none", {title: g.title}));
      desc.id = "si-confirm-desc";
      box.appendChild(desc);
      plan.bad.concat(plan.notes).forEach(function (n) { box.appendChild(make("p", "pl-error", n)); });
      if (plan.tooBig && plan.tooBig.length) box.appendChild(make("p", "pl-error", ByopopStrings.t("saveimport.too_big", {files: plan.tooBig.join(", ")})));

      function section(title, rows, empty) {
        box.appendChild(make("h5", "si-sub", title));
        var ul = make("ul", "sv-list");
        rows.forEach(function (r) { ul.appendChild(r); });
        if (!rows.length) ul.appendChild(make("li", "pl-empty", empty));
        box.appendChild(ul);
      }
      if (e.registry) {
        var rows = items.map(function (it) {
          var li = make("li");
          li.appendChild(make("strong", "", ByopopStrings.t("saveimport.status.label", {status: ByopopStrings.t(STATUS[it.status])}) + " "));
          li.appendChild(document.createTextNode(ByopopStrings.t(it.status === "changes" ? "saveimport.item_changes" : "saveimport.item",
            {path: it.path, value: it.shown, hive: HIVE_NAMES[it.hive], old: it.status === "changes" ? shortValue(it.old) : ""})));
          return li;
        });
        section(ByopopStrings.t("saveimport.section.values", {count: plural(items.length, "saveimport.n.value")}), rows, ByopopStrings.t("saveimport.section.no_values"));
        if (plan.reg.overridden) box.appendChild(make("p", "pl-hint", ByopopStrings.t("saveimport.overridden", {n: plan.reg.overridden})));
        if (plan.reg.skipped.length) {
          var det = make("details", "pl-details");
          det.appendChild(make("summary", "", ByopopStrings.t("saveimport.left_values", {count: plan.reg.skipped.length})));
          var sul = make("ul", "sv-list");
          plan.reg.skipped.forEach(function (s) { sul.appendChild(make("li", "", ByopopStrings.t("saveimport.skipped_value", {path: s.path, why: s.why}))); });
          det.appendChild(sul);
          box.appendChild(det);
        }
      }
      if (e.files && e.files.length) {
        var frows = files.map(function (f) {
          var li = make("li");
          li.appendChild(make("strong", "", ByopopStrings.t("saveimport.status.label", {status: ByopopStrings.t(STATUS[f.status])}) + " "));
          li.appendChild(document.createTextNode(ByopopStrings.t(f.folder === 1 ? "saveimport.file_second" : "saveimport.file",
            {target: f.target, what: f.what, size: ctx.size(f.size), from: f.from.replace(/\//g, "\\")})));
          return li;
        });
        section(ByopopStrings.t("saveimport.section.files", {count: plural(files.length, "saveimport.n.file")}), frows, ByopopStrings.t("saveimport.section.no_files"));
        if (plan.files.skipped.length) {
          var det2 = make("details", "pl-details");
          det2.appendChild(make("summary", "", ByopopStrings.t("saveimport.left_files", {count: plan.files.skipped.length})));
          var sul2 = make("ul", "sv-list");
          plan.files.skipped.forEach(function (s) { sul2.appendChild(make("li", "", ByopopStrings.t("saveimport.skipped_file", {path: s.rel, why: s.why}))); });
          det2.appendChild(sul2);
          box.appendChild(det2);
        }
        if (plan.files.ignored) box.appendChild(make("p", "pl-hint", ByopopStrings.t("saveimport.ignored", {n: plan.files.ignored})));
      }
      plan.todo = todo;
      var warn = make("p", "pl-error");
      box.appendChild(warn);
      // a game that is open could write over what is imported
      ctx.openGames().then(function (open) {
        if (open === null) { warn.textContent = ByopopStrings.t("saveimport.warn.no_locks"); if (plan) plan.blocked = true; }
        else if (open.indexOf(g.id) >= 0) { warn.textContent = ByopopStrings.t("saveimport.warn.open", {title: g.title}); if (plan) plan.blocked = true; }
        syncNav();
      });
      syncNav();
      ctx.say(ByopopStrings.t("saveimport.plan.said", {title: g.title}));
    }
    function shortValue(v) { return v === null ? "" : v.length > 40 ? v.slice(0, 40) + "…" : v; }
    // Cancel: back to the first page with no game chosen
    function cancel() {
      resetPlan();
      sel.value = ""; chosen = null; picks = { folder1: [], folder2: [], regs: [] };
      renderHelp();
      ctx.say(ByopopStrings.t("saveimport.nothing"));
      sel.focus();
    }
    box.addEventListener("keydown", function (ev) { if (ev.key === "Escape" && !ctx.isBusy()) cancel(); });

    // ----- the write -----
    function doImport() {
      if (ctx.isBusy() || !plan) return;
      var p = plan, g = p.game, e = gameEntry(g.id);
      ctx.setBusy(true);
      ctx.say(ByopopStrings.t("saveimport.importing"));
      ctx.openGames().then(function (open) {
        if (open === null) throw new Error(ByopopStrings.t("saveimport.err.no_locks"));
        if (open.indexOf(g.id) >= 0) throw new Error(ByopopStrings.t("saveimport.err.open", {title: g.title}));
        // read again under the game's lock: the files as the game would find them now
        return ctx.withLocks([g], function () {
          return currentState(g).then(function (cur) {
            var userdata = {}, now = Date.now(), remove = [];
            // what the game's own browser backup had that the folder lacks or has older goes in; what it dropped goes
            Object.keys(cur.merged.files).forEach(function (k) { if (cur.merged.files[k] !== cur.files[k]) userdata[k] = cur.merged.files[k]; });
            Object.keys(cur.files).forEach(function (k) { if (!cur.merged.files[k]) remove.push(k); });
            var wroteReg = 0, wroteFiles = 0;
            if (p.reg && p.reg.items.length) {
              var ini = cur.merged.files["registry.ini"] ? bytesToBinary(cur.merged.files["registry.ini"].bytes) : "";
              var re = planRegistry(e, p.col, ini);
              userdata["registry.ini"] = { bytes: binaryToBytes(applyRegistry(re)), m: now };
              wroteReg = re.items.length;
            }
            (p.files ? p.files.take : []).forEach(function (f) {
              var k = f.target;
              Object.keys(cur.merged.files).forEach(function (x) { if (lc(x) === lc(f.to)) k = x; });
              userdata[k] = { bytes: f.bytes, m: f.m || now };
              wroteFiles++;
            });
            return ctx.writeFiles(g, cur.names, { userdata: userdata, replays: {}, remove: { userdata: remove } }).then(function () {
              ctx.lsSet(ctx.backupKey(g.persist), null);
              return { reg: wroteReg, files: wroteFiles };
            });
          });
        });
      }).then(function (n) {
        resetPlan();
        report.textContent = "";
        report.appendChild(make("h3", "", ByopopStrings.t("saveimport.done.title")));
        var ul = make("ul", "sv-list");
        var counts = {title: g.title, values: plural(n.reg, "saveimport.n.registry_value"), files: plural(n.files, "saveimport.n.file")};
        ul.appendChild(make("li", "", ByopopStrings.t("saveimport.done.line", counts)));
        report.appendChild(ul);
        ctx.say(ByopopStrings.t("saveimport.done.said", counts));
        picks = { folder1: [], folder2: [], regs: [] };
        renderHelp();
        syncNav();
        return ctx.refresh();
      }).catch(function (er) { resetPlan(); ctx.say(ByopopStrings.t("saveimport.failed", {error: er && er.message ? er.message : er}), true); }).then(function () {
        ctx.setBusy(false);
        if (document.activeElement === document.body || !document.activeElement) sel.focus();
      });
    }

    ctx.addControls([sel]);
    ctx.onBusy(function (b) {
      sel.disabled = b;
      Array.prototype.forEach.call(root.querySelectorAll(".si-page input"), function (el) { el.disabled = b; });
      syncNav();
    });
    return { refresh: refresh };
  }

  core.init = init;
  return core;
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopSaveImport;
