// Checks web/modpanel.js's PopCap pak reader (Module.byopop.pak.read), which the page's import uses to unpack a
// game's main.pak (web/shell.html, PAGE.paks). Under Node:
//   node tools/paktest/paktest.js [<main.pak> <staged folder>]
// It builds a small pak (two files, one in a folder with a backslash in its name, and an empty one) and reads it
// back, checks that a damaged pak is refused, and with a real pak compares every file with the staged copy that
// tools/popcap_pak.py unpacked (names in lower case, as staging writes them).
"use strict";
var fs = require("fs");
var path = require("path");

global.Module = {};
require(path.join(__dirname, "../../web/modpanel.js"));
var pak = global.Module.byopop.pak;
var failures = 0;
function check(ok, what) {
  if (!ok) { failures++; console.log("paktest: FAILED " + what); }
}

// A pak with the given files ({name, data}), XORed with key
function makePak(files, key) {
  var parts = [];
  function u32(v) { var b = Buffer.alloc(4); b.writeUInt32LE(v >>> 0); parts.push(b); }
  u32(0xbac04ac0); u32(0);
  files.forEach(function (f) {
    parts.push(Buffer.from([0, f.name.length])); parts.push(Buffer.from(f.name, "latin1"));
    u32(f.data.length); parts.push(Buffer.alloc(8));
  });
  parts.push(Buffer.from([0x80]));
  files.forEach(function (f) { parts.push(Buffer.from(f.data)); });
  var all = Buffer.concat(parts);
  for (var i = 0; i < all.length; i++) all[i] ^= key;
  return new Uint8Array(all);
}

function synthetic() {
  var files = [
    { name: "properties\\resources.xml", data: Buffer.from("<ResourceManifest/>") },
    { name: "empty.txt", data: Buffer.alloc(0) },
    { name: "images/a.png", data: Buffer.from([0x89, 0x50, 0x4e, 0x47, 0xf7, 0x00, 0xff]) }
  ];
  var bytes = makePak(files, 0xf7);
  var entries = pak.read(bytes);
  check(entries.length === 3, "synthetic: 3 entries, got " + entries.length);
  check(entries[0].path === "properties/resources.xml", "synthetic: backslash becomes a slash");
  return Promise.all(entries.map(function (e) { return e.read(); })).then(function (datas) {
    datas.forEach(function (d, i) {
      check(Buffer.from(d).equals(files[i].data), "synthetic: " + files[i].name + " reads back");
    });
    var refused = 0;
    [bytes.subarray(0, 5), bytes.subarray(0, 20), bytes.subarray(0, bytes.length - 3), makePak(files, 0x11)].forEach(function (b) {
      try { pak.read(b); } catch (e) { refused++; }
    });
    check(refused === 4, "synthetic: 4 damaged or foreign paks refused, " + refused + " were");
    console.log("paktest: synthetic pak of 3 files, " + refused + " damaged ones refused");
  });
}

function real(pakPath, staged) {
  var entries = pak.read(new Uint8Array(fs.readFileSync(pakPath)));
  var same = 0, differ = 0, missing = 0;
  return entries.reduce(function (p, e) {
    return p.then(function () {
      return e.read().then(function (data) {
        var copy = path.join(staged, e.path.toLowerCase());
        if (!fs.existsSync(copy)) { missing++; return; }
        if (Buffer.from(data).equals(fs.readFileSync(copy))) same++;
        else { differ++; console.log("paktest: differs from the staged copy: " + e.path); }
      });
    });
  }, Promise.resolve()).then(function () {
    check(differ === 0 && missing === 0 && same > 0, "real pak: every file the same as its staged copy");
    console.log("paktest: " + path.basename(pakPath) + ": " + entries.length + " files, " + same + " the same as staged, " +
      differ + " different, " + missing + " not staged");
  });
}

var args = process.argv.slice(2);
synthetic().then(function () { if (args.length >= 2) return real(args[0], args[1]); }).then(function () {
  console.log("paktest: " + (failures ? failures + " failed" : "0 failed"));
  process.exit(failures ? 1 : 0);
}).catch(function (e) { console.log("paktest: FAILED " + e.message); process.exit(1); });
