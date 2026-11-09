// Checks that every link to the site's own legal pages opens in a new tab (the user, 2026-11-08: "libopenmpt in the music pane
// doesn't open in a new tab"): a link to a licence text (licenses/...), a game's terms, copyright and licences page
// (<game>-copyright.html, the footer's @BYOPOP_COPYRIGHT_PAGE@) or the terms (terms.html) carries target="_blank" and a rel with
// noopener, wherever it is written: the pages and scripts in web/, the site's text (web/strings/*.txt, whose *_html strings go
// into pages as markup) and, when there is a build, its pages (build/web, or BYOPOP_WEB_DIR; minified, with bare attributes).
//   node tools/linkstest.js
// It prints one line for each link that fails, then "linkstest: N links, M failed", and exits 1 when any failed.
"use strict";
const fs = require("fs");
const os = require("os");
const path = require("path");

const root = path.join(__dirname, "..");
const LEGAL = /(^|\/)(licenses\/.*|[\w.-]*copyright\.html([#?].*)?|terms\.html([#?].*)?|@BYOPOP_COPYRIGHT_PAGE@)$/i;
let links = 0, failed = 0;

function files(dir, re) {
  try { return fs.readdirSync(dir).filter((f) => re.test(f)).map((f) => path.join(dir, f)); } catch (e) { return []; }
}
// an attribute's value, quoted either way or bare, or null
function attr(attrs, name) {
  const m = new RegExp("(?:^|\\s)" + name + "\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)'|([^\\s>]+))", "i").exec(attrs);
  if (!m) return null;
  return m[1] !== undefined ? m[1] : m[2] !== undefined ? m[2] : m[3];
}
function check(file, quiet) {
  const text = fs.readFileSync(file, "utf8");
  const re = /<a\s([^>]*)>/gi;
  let m, n = 0, bad = 0;
  while ((m = re.exec(text))) {
    const attrs = m[1];
    const href = attr(attrs, "href");
    if (href === null || !LEGAL.test(href)) continue;
    n++;
    const blank = attr(attrs, "target") === "_blank";
    const rel = attr(attrs, "rel");
    const noopener = rel !== null && /\bnoopener\b/i.test(rel);
    if (!blank || !noopener) {
      bad++;
      if (quiet) continue;
      const line = text.slice(0, m.index).split("\n").length;
      console.log(path.relative(root, file).replace(/\\/g, "/") + ":" + line + ": " + href +
        (blank ? "" : " has no target=\"_blank\"") + (noopener ? "" : " has no rel=\"noopener\""));
    }
  }
  return [n, bad];
}

// the rule itself: a link without them fails, quoted or bare ones with them pass, other links are not counted
(function selftest() {
  const tmp = path.join(os.tmpdir(), "linkstest-" + process.pid + ".html");
  fs.writeFileSync(tmp, '<a href="licenses/x.txt">x</a>\n<a href="g-copyright.html" target="_blank" rel="noopener">y</a>\n' +
    '<a href="./">z</a>\n<a href=h-copyright.html rel=noopener target=_blank>w</a>\n<a href="terms.html" target="_blank">v</a>');
  const r = check(tmp, true);
  fs.unlinkSync(tmp);
  if (r[0] !== 4 || r[1] !== 2) { failed++; console.log("linkstest: the self-test failed (" + r.join(", ") + ")"); }
})();

const web = path.join(root, "web");
const built = process.env.BYOPOP_WEB_DIR ? path.resolve(root, process.env.BYOPOP_WEB_DIR) : path.join(root, "build", "web");
files(web, /\.(html|js)$/i).concat(files(path.join(web, "strings"), /\.txt$/i), files(built, /\.html$/i)).forEach((f) => {
  const r = check(f, false);
  links += r[0]; failed += r[1];
});

console.log("linkstest: " + links + " links, " + failed + " failed");
process.exit(failed ? 1 : 0);
