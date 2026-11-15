// Checks web/exetrim.js against the staged exes (assets/<game>/, git-ignored): every address in a port's tables
// (games/<game>/src/*.inc, the first argument of each entry, with its length when the second is one) reads the same bytes from
// the trimmed exe as from the original, through the section table, with the launcher's outer image or the wrapped inner one as
// the game lists (TRIM). Also prints, for every staged exe, the size before and after, both ways, so a game can be added.
// node tools/exetrimtest.js [assets folder]
"use strict";
const fs = require("fs"), path = require("path");
const X = require("../web/exetrim.js");
const repo = path.join(__dirname, "..");
const assets = process.argv[2] || path.join(repo, "assets");
let failed = 0, checks = 0;
function fail(msg) { failed++; console.log("FAIL " + msg); }

function addresses(game) {
  const dir = path.join(repo, "games", game, "src"), out = [];
  if (!fs.existsSync(dir)) return out;
  for (const f of fs.readdirSync(dir).filter((n) => /\.inc$/i.test(n))) {
    const re = /^[A-Z_]+\(\s*(0x[0-9a-fA-F]{6,8})\s*(?:,\s*(\d+|0x[0-9a-fA-F]+))?/gm;
    let m;
    const text = fs.readFileSync(path.join(dir, f), "utf8");
    while ((m = re.exec(text))) {
      const va = parseInt(m[1], 16), n = m[2] ? Number(m[2]) : 16;
      if (va >= 0x400000 && va < 0x2000000) out.push({ va, len: Math.max(1, Math.min(n + 1, 65536)), from: f });
    }
  }
  return out;
}
function image(b, unwrap) {
  if (!unwrap) return b;
  const img = X.inner(b);
  if (!img) return null;
  return b.subarray(img.at);
}
const games = fs.existsSync(assets) ? fs.readdirSync(assets) : [];
let before = 0, after = 0;
for (const game of games) {
  const dir = path.join(assets, game);
  if (!fs.statSync(dir).isDirectory()) continue;
  for (const name of fs.readdirSync(dir).filter((n) => /\.exe$/i.test(n))) {
    const b = new Uint8Array(fs.readFileSync(path.join(dir, name)));
    if (!X.parse(b, 0)) continue;
    const listed = X.TRIM[game] && X.TRIM[game].exe === name.toLowerCase() ? X.TRIM[game] : null;
    const wrapped = !!X.inner(b);
    const ways = listed ? [!!listed.unwrap] : wrapped ? [false, true] : [false];
    const addrs = addresses(game);
    for (const unwrap of ways) {
      const src = image(b, unwrap), t = X.trim(b, { unwrap });
      let same = 0, lost = 0, none = 0;
      for (const a of addrs) {
        const was = X.at(src, a.va, a.len), now = X.at(t, a.va, a.len);
        if (!was) { none++; continue; }
        if (now && Buffer.compare(Buffer.from(was), Buffer.from(now)) === 0) same++;
        else { lost++; if (listed && lost <= 3) fail(game + " " + name + ": " + a.from + " 0x" + a.va.toString(16) + " reads differently"); }
      }
      if (listed) { checks += addrs.length; before += b.length; after += t.length; }
      console.log((listed ? "TRIM " : "     ") + game + " " + name + (wrapped ? (unwrap ? " (inner)" : " (outer)") : "") + ": " +
        Math.round(b.length / 1024) + " KB -> " + Math.round(t.length / 1024) + " KB; addresses " + addrs.length + ": same " + same +
        ", lost " + lost + ", outside the image " + none);
    }
  }
}
// a file that is not a PE image is stored as it is
const junk = new Uint8Array([1, 2, 3]);
checks++;
if (X.trim(junk) !== junk) fail("a file that is not an exe is changed");
console.log("exetrimtest: " + checks + " checks, " + failed + " failed; listed exes " + Math.round(before / 1024) + " KB -> " + Math.round(after / 1024) + " KB");
process.exit(failed ? 1 : 0);
