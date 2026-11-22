// The guides (web/guides.js, web/guidetexts.js) under Node: node tools/guidestest.js
// First the markup on a text made here (headings, tips and their leads, paragraphs that join, notes not kept, words
// counted). Then every guide against the rules of docs/GUIDE_STYLE.md that a program can check: a game of the listing
// (BYOPOP_WEB_FINISHED), a checked date, three to seven sections of two to six tips, 300 to 900 words, a source note in
// every section, every tip led by two to six words and a colon, and none of the words the style guide rules out.
"use strict";
const fs = require("fs"), path = require("path");
const G = require("../web/guides.js"), TEXTS = require("../web/guidetexts.js");
let checks = 0, failed = 0;
function same(name, got, want) {
  checks++;
  if (JSON.stringify(got) !== JSON.stringify(want)) { failed++; console.log("FAIL " + name + ": " + JSON.stringify(got) + " != " + JSON.stringify(want)); }
}

const made = G.parse(["## One", "// a note", "- A lead: the tip,", "  carried on.", "", "A paragraph", "that joins.", "- No lead here",
  "## Two", "- Lead two: a: b", "//"].join("\n"));
same("sections", made.sections.map((s) => [s.head, s.notes, s.items.length]), [["One", 1, 3], ["Two", 1, 1]]);
same("tip and lead", made.sections[0].items[0], { tip: true, lead: "A lead", text: "the tip, carried on." });
same("paragraph joins", made.sections[0].items[1], { tip: false, lead: "", text: "A paragraph that joins." });
same("tip without a lead", made.sections[0].items[2], { tip: true, lead: "", text: "No lead here" });
same("first colon only", made.sections[1].items[0].text, "a: b");
same("counts", [made.tips, made.words], [3, 19]);

// the listing's games
const cmake = fs.readFileSync(path.join(__dirname, "../cmake/ByopopWeb.cmake"), "utf8");
const listed = ((cmake.match(/set\(BYOPOP_WEB_FINISHED([^)]*)\)/) || [])[1] || "").split(/\s+/).filter(Boolean);
const BANNED = /\b(simply|just|probably|maybe|good luck|in this guide|AI|port|decompil\w*|the site|emulat\w*)\b|!|[\u{1F300}-\u{1FAFF}☀-➿]/iu;
let guides = 0, tipsAll = 0;
for (const id of Object.keys(TEXTS)) {
  guides++;
  const g = TEXTS[id], parsed = G.parse(g.text), bad = [];
  if (listed.indexOf(id) < 0) bad.push("not a game of the listing");
  if (!/^\d{4}-\d{2}-\d{2}$/.test(g.checked || "")) bad.push("no checked date");
  const secs = parsed.sections.filter((s) => s.head);
  if (secs.length !== parsed.sections.length) bad.push("text before the first heading");
  if (secs.length < 3 || secs.length > 7) bad.push(secs.length + " sections");
  secs.forEach((s) => {
    const tips = s.items.filter((i) => i.tip);
    if (tips.length < 2 || tips.length > 6) bad.push('"' + s.head + '": ' + tips.length + " tips");
    if (!s.notes) bad.push('"' + s.head + '": no source note');
    tips.forEach((t) => {
      const n = t.lead.split(/\s+/).filter(Boolean).length;
      if (n < 2 || n > 6) bad.push('"' + (t.lead || t.text.slice(0, 30)) + '": a lead of ' + n + " words");
    });
    s.items.concat([{ lead: s.head, text: "" }]).forEach((i) => {
      const m = (i.lead + " " + i.text).match(BANNED);
      if (m) bad.push('"' + m[0] + '" in "' + (i.lead || i.text.slice(0, 30)) + '"');
    });
  });
  if (parsed.words < 300 || parsed.words > 900) bad.push(parsed.words + " words");
  tipsAll += parsed.tips;
  checks++;
  if (bad.length) { failed++; bad.forEach((b) => console.log("FAIL " + id + ": " + b)); }
}
console.log("guidestest: " + checks + " checks, " + failed + " failed; " + guides + " guide" + (guides === 1 ? "" : "s") + ", " + tipsAll + " tips");
process.exit(failed ? 1 : 0);
