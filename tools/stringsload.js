// The site's text for the Node tests of the pages' scripts, as keys, not words: web/i18n.js (ByopopStrings) over a table in
// which each key of web/strings/en.txt reads as the key itself followed by its placeholders, so that a test checks which
// string a script picked and what it filled in, never the wording, which the user rewrites (the user, 2026-11-03: tests
// check keys, not the localisation).
//   "time.under_minute"                       -> "time.under_minute"
//   "mods.remove_failed" ({name}, {error})    -> "mods.remove_failed {name} {error}"
//   a count string ("{n} play | {n} plays")   -> "index.played:one {n} | index.played:other {n}" (its own forms)
// A test that wants what a script shows writes it the same way, with keyText(key, vars) below.
//   require("./stringsload").intoGlobal()   for a script that a test require()s
//   require("./stringsload").into(ctx)      for a vm context a test runs scripts in (ctx must be vm.createContext'ed)
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const root = path.join(__dirname, "..");

// en.txt's keys and their placeholders (the build's rules: "key = text", indented lines go on, # notes)
function parse() {
  const table = {};
  let cur = null;
  for (const line of fs.readFileSync(path.join(root, "web", "strings", "en.txt"), "utf8").replace(/\r\n/g, "\n").split("\n")) {
    if (/^\s*(#.*)?$/.test(line)) { if (!/^\s*#/.test(line)) cur = null; continue; }
    if (/^\s/.test(line)) { if (cur) table[cur] += " " + line.trim(); continue; }
    const m = /^([a-z0-9_.-]+)\s*=\s*(.*)$/.exec(line);
    if (m) { cur = m[1]; table[cur] = m[2].trim(); }
  }
  return table;
}
function stub(key, text) {
  const names = (s) => { const r = []; s.replace(/\{([A-Za-z0-9_]+)\}/g, (m, n) => { if (r.indexOf(n) < 0) r.push(n); return m; }); return r; };
  const one = (label, s) => [label].concat(names(s).map((n) => "{" + n + "}")).join(" ");
  // a list of names that a script splits at ", " (chat.weekdays, chat.months): numbered, so that it splits the same
  const list = text.split(", ");
  if (list.length >= 5 && !/[{|]/.test(text) && list.every((w) => /^\S+$/.test(w))) return list.map((w, i) => key + "." + i).join(", ");
  const forms = text.split(" | ");
  if (forms.length < 2) return one(key, text);
  // English has two forms, one and other
  return forms.map((f, i) => one(key + ":" + (i === forms.length - 1 ? "other" : "one"), f)).join(" | ");
}
let cached = null;
function table() {
  if (cached) return cached;
  const t = parse();
  cached = {};
  for (const k of Object.keys(t)) cached[k] = stub(k, t[k]);
  return cached;
}
function source() {
  return "var BYOPOP_STRINGS = " + JSON.stringify(table()) + ";\n" + fs.readFileSync(path.join(root, "web", "i18n.js"), "utf8");
}

exports.into = function (ctx) {
  vm.runInContext(source(), ctx, { filename: "strings.js" });
  return ctx;
};

exports.intoGlobal = function () {
  vm.runInThisContext(source() + "\nglobalThis.ByopopStrings = ByopopStrings;", { filename: "strings.js" });
};

// What a script shows for t(key, vars) under this table, for a test's expectations
exports.keyText = function (key, vars) {
  const ctx = vm.createContext({});
  exports.into(ctx);
  return vm.runInContext("ByopopStrings", ctx).t(key, vars);
};
