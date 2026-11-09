// The Setup window's logic (web/setup.js, docs/WEB_PLAYER.md "The Setup window"): the pages it walks for the games found, and
// Finish's rows and counts from a list of results. The window itself runs on the listing's desktop only; this is its pure part.
//   node tools/setuptest.js
// It prints a line for each check that fails, then "setuptest: N checks, M failed", and exits 1 when any failed.
"use strict";
const path = require("path");
const { logic } = require(path.join(__dirname, "..", "web", "setup.js"));

let checks = 0, failed = 0;
function check(ok, what) {
  checks++;
  if (!ok) { failed++; console.log("FAIL " + what); }
}
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
const pages = (games) => logic.plan(games).map((s) => s.page + (s.id ? ":" + s.id : ""));

// ---- the pages
check(same(pages([]), ["copying", "finish"]), "nothing found: Copying, then Finish");
check(same(pages([{ id: "a", agreed: true, supported: true }]), ["copying", "finish"]), "a game agreed to: no Licence page");
check(same(pages([{ id: "a", agreed: false, supported: true }]), ["licence:a", "copying", "finish"]), "a game not agreed to: its Licence page first");
check(same(pages([
  { id: "a", agreed: false, supported: true },
  { id: "b", agreed: true, supported: true },
  { id: "c", agreed: false, supported: true }
]), ["licence:a", "licence:c", "copying", "finish"]), "a Licence page for each game not agreed to, in the order found");
check(same(pages([{ id: "a", agreed: false, here: true, supported: true }]), ["copying", "finish"]), "a game already here: no Licence page");
check(same(pages([{ id: "a", agreed: false, supported: false }]), ["copying", "finish"]), "a folder with no game: no Licence page");
check(logic.plan([{ id: "a", agreed: false, supported: true }]).slice(-1)[0].page === "finish", "Finish is the last page");

// ---- Finish's summary
const t = (k, v) => k + (v ? JSON.stringify(v) : "");
const sum = logic.summary([
  { title: "Alchemy", kind: "installed" },
  { title: "Big Money", kind: "already" },
  { title: "Old Stuff", kind: "unsupported", reason: "no game this site plays" },
  { title: "Pixelus", kind: "skipped", reason: "terms not agreed" },
  { title: "Dropped files", kind: "error", code: 1, reason: "could not be read" },
  { title: "Noah's Ark", kind: "error", code: 2, reason: "page open" }
], t);
check(sum.total === 6, "summary: total");
check(same(sum.counts, { installed: 1, already: 1, unsupported: 1, skipped: 1, error: 2 }), "summary: counts by kind");
check(sum.failed === 2, "summary: the errors counted as failed");
check(same(sum.rows.map((r) => r.title), ["Alchemy", "Big Money", "Old Stuff", "Pixelus", "Dropped files", "Noah's Ark"]), "summary: rows in the order given");
check(sum.rows[0].value === "setup.result.installed" && sum.rows[1].value === "setup.result.already" &&
  sum.rows[2].value === "setup.result.unsupported" && sum.rows[3].value === "setup.result.skipped", "summary: each kind's value");
check(sum.rows[4].value === 'setup.result.error{"code":1}' && sum.rows[5].value === 'setup.result.error{"code":2}', "summary: an error's value carries its code");
check(sum.rows[2].tip === "no game this site plays" && sum.rows[5].tip === "page open" && sum.rows[0].tip === "", "summary: the reason is the tooltip");
const none = logic.summary([], t);
check(none.total === 0 && none.failed === 0 && none.rows.length === 0, "summary: nothing");

console.log("setuptest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
