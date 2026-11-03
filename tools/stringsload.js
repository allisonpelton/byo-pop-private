// Loads the site's text for the Node tests of the pages' scripts: build/web/strings.js (BYOPOP_STRINGS and ByopopStrings,
// written by the web build; BYOPOP_WEB_DIR names another folder), as the first script of a page is loaded.
//   require("./stringsload").intoGlobal()   for a script that a test require()s
//   require("./stringsload").into(ctx)      for a vm context a test runs scripts in (ctx must be vm.createContext'ed)
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

function source() {
  const dir = process.env.BYOPOP_WEB_DIR || path.join(__dirname, "..", "build", "web");
  const file = path.resolve(dir, "strings.js");
  if (!fs.existsSync(file)) throw new Error("no " + file + ": run tools/check.sh build-web first");
  return { file, text: fs.readFileSync(file, "utf8") };
}

exports.into = function (ctx) {
  const s = source();
  vm.runInContext(s.text, ctx, { filename: "strings.js" });
  return ctx;
};

exports.intoGlobal = function () {
  const s = source();
  vm.runInThisContext(s.text + "\nglobalThis.ByopopStrings = ByopopStrings;", { filename: "strings.js" });
};
