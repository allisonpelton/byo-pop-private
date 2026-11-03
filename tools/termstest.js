// Checks when a page asks for a game's terms (web/terms.js; docs/WEB_PLAYER.md, "The terms") under Node: the development hosts
// that skip the card, ?terms=1 that brings it back, the site's test key (the right key, a wrong one, none in the build), what a
// skip keeps (nothing but the tab's own mark) and the agreement kept per game and version.
//   node tools/termstest.js
// It prints "termstest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const crypto = require("crypto");

const source = fs.readFileSync(path.join(__dirname, "..", "web", "terms.js"), "utf8");
const KEY = "correct horse battery staple";
const HASH = crypto.createHash("sha256").update(KEY).digest("hex");

// A page at href; keyHash is what the build baked in (null: the marker left, as in a build without the key's file)
function page(href, opts) {
  opts = opts || {};
  const u = new URL(href);
  const local = Object.assign({}, opts.local || {}), session = Object.assign({}, opts.session || {});
  const storage = (o) => ({ getItem: (k) => (k in o ? o[k] : null), setItem: (k, v) => { o[k] = String(v); } });
  const loc = { href: u.href, protocol: u.protocol, hostname: u.hostname, search: u.search, pathname: u.pathname, hash: u.hash };
  const ctx = {
    location: loc,
    history: { state: null, replaceState: (s, t, url) => { const n = new URL(url, loc.href); loc.href = n.href; loc.search = n.search; } },
    localStorage: storage(local), sessionStorage: storage(session),
    URL, URLSearchParams, TextEncoder, Promise, crypto: globalThis.crypto
  };
  vm.createContext(ctx);
  const baked = opts.keyHash === null ? source : source.replace('"@BYOPOP_TESTKEY_HASH@"', JSON.stringify(opts.keyHash === undefined ? HASH : opts.keyHash));
  vm.runInContext(baked, ctx, { filename: "terms.js" });
  return { T: vm.runInContext("ByopopTerms", ctx), local, session, loc };
}

let checks = 0, failed = 0;
function check(what, got, want) {
  checks++;
  const g = JSON.stringify(got), w = JSON.stringify(want);
  if (g !== w) { failed++; console.log(`FAIL ${what}: got ${g}, want ${w}`); }
}

(async () => {
  // the host check
  for (const [href, want] of [
    ["http://127.0.0.1:18795/bejeweled.html", "dev"], ["http://localhost:8080/", "dev"], ["http://[::1]:8000/x.html", "dev"],
    ["http://games.localhost/", "dev"], ["https://byo-pop.test/", "dev"], ["file:///C:/build/web/index.html", "dev"],
    ["https://byo-pop.example/", null], ["https://localhost.example.com/", null], ["https://test.example/", null],
    ["http://127.0.0.2/", null], ["https://example.org/index.html?terms=0", null]]) {
    const p = page(href);
    check("host " + href, [p.T.skipped(), await p.T.ready], [want, want]);
  }
  {
    const p = page("http://127.0.0.1:18795/bejeweled.html?silent=1&terms=1");
    check("?terms=1 shows the card on a development host", [p.T.skipped(), p.T.agreed("bejeweled", "abc")], [null, false]);
    const q = page("http://127.0.0.1/x.html");
    check("a development host agrees to every game", q.T.agreed("bejeweled", "abc"), true);
    check("and stores nothing", [q.local, q.session], [{}, {}]);
  }
  // the test key
  {
    const p = page("https://site.example/bejeweled.html?testkey=" + encodeURIComponent(KEY) + "&silent=1");
    check("the right key: pending while it is checked", p.T.skipped(), "pending");
    check("the right key skips the card", await p.T.ready, "testkey");
    check("for every game", p.T.agreed("tiptop", "v1"), true);
    check("marked for the tab only, no agreement kept", [p.session, p.local], [{ "byopop-testkey": HASH }, {}]);
    check("the key leaves the address bar", p.loc.search, "?silent=1");
    const later = page("https://site.example/tiptop.html", { session: p.session });
    check("another page in the same tab", [later.T.skipped(), await later.T.ready], ["testkey", "testkey"]);
  }
  {
    const p = page("https://site.example/bejeweled.html?testkey=wrong");
    check("a wrong key shows the card", [await p.T.ready, p.T.agreed("bejeweled", "v1"), p.session], [null, false, {}]);
    check("and leaves the address bar too", p.loc.search, "");
    const q = page("https://site.example/bejeweled.html?testkey=" + encodeURIComponent(KEY), { keyHash: null });
    check("no key in the build: the card shows", [q.T.skipped(), await q.T.ready], [null, null]);
    const r = page("https://site.example/bejeweled.html?testkey=" + encodeURIComponent(KEY), { keyHash: "" });
    check("an empty key in the build: the card shows", await r.T.ready, null);
    const s = page("https://site.example/bejeweled.html?testkey=" + encodeURIComponent(KEY) + "&terms=1");
    check("?terms=1 wins over the key", await s.T.ready, null);
    const t = page("https://site.example/bejeweled.html", { session: { "byopop-testkey": "0".repeat(64) } });
    check("a tab's mark for another key does not count", await t.T.ready, null);
  }
  // the agreement
  {
    const p = page("https://site.example/bejeweled.html");
    check("not agreed yet", p.T.agreed("bejeweled", "v1"), false);
    p.T.agree("bejeweled", "v1");
    check("agreed, kept per game and version", [p.T.agreed("bejeweled", "v1"), p.T.agreed("bejeweled", "v2"), p.T.agreed("tiptop", "v1")],
      [true, false, false]);
    check("what is kept", p.local, { "byopop-terms-bejeweled": "v1" });
  }

  console.log(`termstest: ${checks} checks, ${failed} failed`);
  process.exit(failed ? 1 : 0);
})();
