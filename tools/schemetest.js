// Desktop 95's colour schemes (web/schemes.js, docs/WEB_PLAYER.md "Colour schemes"): each scheme whole and well-formed, named in
// the site's text, and its text readable on its colours.
//   node tools/schemetest.js
// It prints a line for each check that fails, then "schemetest: N checks, M failed", and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const S = require(path.join(__dirname, "..", "web", "schemes.js"));
const strings = fs.readFileSync(path.join(__dirname, "..", "web", "strings", "en.txt"), "utf8");

let checks = 0, failed = 0;
function check(ok, what) { checks++; if (!ok) { failed++; console.log("FAIL " + what); } }
const HEX = /^#[0-9a-f]{6}$/;
const at = (a, b) => S.contrast(a, b);

check(S.LIST[0].id === "standard", "the standard scheme is the first");
check(new Set(S.ids).size === S.ids.length, "the ids are unique");
check(S.LIST.length >= 8, "about eight schemes");
check(S.byId("no such scheme").id === "standard" && S.byId(undefined).id === "standard", "an unknown scheme is the standard one");
const names = new Set();
for (const s of S.LIST) {
  const t = S.ends(s), n = s.id;
  check([s.desk, s.face, s.lo, s.mid, s.sel, t[0], t[1]].every((c) => HEX.test(c)), n + ": every colour is #rrggbb");
  const m = new RegExp("^settings\.scheme\." + n + " = (.+)$", "m").exec(strings);
  check(!!m, n + ": named in the site's text");
  if (m) { check(!names.has(m[1]), n + ": its name is its own"); names.add(m[1]); }
  // the page's text and its muted text on the face; links and the selection's own colour on the face
  check(at("#000000", s.face) >= 7, n + ": black text on the face, 7:1");
  check(at("#3a3a3a", s.face) >= 4.5, n + ": muted text on the face, 4.5:1");
  check(at(s.sel, s.face) >= 4.5, n + ": links on the face, 4.5:1");
  // white text on the selection, on the title bar (where its text starts, and 3:1 still at its far end) and on the desktop
  check(at("#ffffff", s.sel) >= 4.5, n + ": white on the selection, 4.5:1");
  check(at("#ffffff", t[0]) >= 4.5, n + ": white on the title bar's start, 4.5:1");
  check(at("#ffffff", t[1]) >= 3, n + ": white on the title bar's end, 3:1");
  check(at("#ffffff", s.desk) >= 4.5, n + ": white icon names on the desktop, 4.5:1");
  // the bevel reads: the shadow darker than the face, the soft light between the face and white
  check(at(s.lo, s.face) >= 1.9, n + ": the shadow stands off the face");
  check(at(s.mid, s.face) > 1.05 && at("#ffffff", s.mid) > 1.02, n + ": the soft light is between the face and white");
}
console.log("schemetest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
