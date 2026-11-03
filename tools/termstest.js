// Checks when a page asks for a game's terms (web/terms.js; docs/WEB_PLAYER.md, "The terms") under Node: the development hosts
// that skip the card, ?terms=1 that brings it back, the site's test key (the right key, a wrong one, none in the build), what a
// skip keeps (nothing but the tab's own mark) and the agreement kept per game and version; the listing page's agreement for each
// game it adds (web/addgames.js, agreeTo, run on its own against a stand-in dialog); and, when there is a web build made with a
// test key, that the build holds the key's hash and never the key.
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
    history: { state: null, replaceState: (s, t, url) => { const n = new URL(url, loc.href); loc.href = n.href; loc.search = n.search; loc.hash = n.hash; } },
    localStorage: storage(local), sessionStorage: storage(session),
    URL, URLSearchParams, TextEncoder, Promise, crypto: globalThis.crypto
  };
  Object.assign(ctx, opts.extra || {});
  vm.createContext(ctx);
  const baked = opts.keyHash === null ? source : source.replace('"@BYOPOP_TESTKEY_HASH@"', JSON.stringify(opts.keyHash === undefined ? HASH : opts.keyHash));
  vm.runInContext(baked, ctx, { filename: "terms.js" });
  return { T: vm.runInContext("ByopopTerms", ctx), local, session, loc, ctx };
}

// The listing's agreeTo(game), taken from web/addgames.js as it is, in a page with a stand-in of its dialog
const addgames = fs.readFileSync(path.join(__dirname, "..", "web", "addgames.js"), "utf8");
const agreeSource = (/\n  function agreeTo\(game\) \{[\s\S]*?\n  \}\n/.exec(addgames) || [""])[0];
function listing(href, opts) {
  opts = opts || {};
  const els = {};
  function el(id) {
    const e = { id, listeners: {}, textContent: "", innerHTML: "", checked: false, disabled: false, open: false, scrollTop: 5,
      addEventListener(t, f) { (this.listeners[t] = this.listeners[t] || []).push(f); },
      removeEventListener(t, f) { this.listeners[t] = (this.listeners[t] || []).filter((g) => g !== f); },
      fire(t, ev) { (this.listeners[t] || []).slice().forEach((f) => f(ev || {})); },
      showModal() { this.open = true; }, close() { this.open = false; }, focus() { focused.push(id); } };
    return (els[id] = e);
  }
  const focused = [];
  ["terms-dialog", "terms-agree", "btn-terms", "btn-terms-skip", "terms-title", "terms-text", "terms-agree-text", "add-button"].forEach(el);
  const extra = {
    document: { getElementById: (id) => els[id] || null, activeElement: els["add-button"], body: { contains: () => true } },
    window: {}, agreedHere: {},
    ByopopStrings: { t: (k, f) => k + (f ? " " + JSON.stringify(f) : "") }
  };
  extra.window.ByopopTermsTexts = opts.texts || { bejeweled: { version: "v7", html: "<p>The terms</p>" } };
  const p = page(href, { extra, local: opts.local });
  vm.runInContext("function $(id) { return document.getElementById(id); }\n" + agreeSource + "\nthis.agreeTo = agreeTo;", p.ctx);
  const game = (id) => ({ spec: { id }, title: "Game " + id });
  const listeners = () => Object.keys(els).reduce((n, k) => n + Object.values(els[k].listeners).reduce((m, a) => m + a.length, 0), 0);
  return Object.assign(p, { els, focused, agreeTo: (id) => p.ctx.agreeTo(game(id)), listeners });
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
    const p = page("https://site.example/bejeweled.html?silent=1#testkey=" + encodeURIComponent(KEY));
    check("the key leaves the address bar before it is checked", [p.loc.search, p.loc.hash], ["?silent=1", ""]);
    check("the right key: pending while it is checked", p.T.skipped(), "pending");
    check("the right key skips the card", await p.T.ready, "testkey");
    check("for every game", p.T.agreed("tiptop", "v1"), true);
    check("marked for the tab only, no agreement kept", [p.session, p.local], [{ "byopop-testkey": HASH }, {}]);
    const later = page("https://site.example/tiptop.html", { session: p.session });
    check("another page in the same tab", [later.T.skipped(), await later.T.ready], ["testkey", "testkey"]);
  }
  {
    const p = page("https://site.example/bejeweled.html#testkey=wrong");
    check("a wrong key shows the card", [await p.T.ready, p.T.agreed("bejeweled", "v1"), p.session], [null, false, {}]);
    check("and leaves the address bar too", p.loc.hash, "");
    const q = page("https://site.example/bejeweled.html#testkey=" + encodeURIComponent(KEY), { keyHash: null });
    check("no key in the build: the card shows", [q.T.skipped(), await q.T.ready, q.loc.hash], [null, null, ""]);
    const r = page("https://site.example/bejeweled.html#testkey=" + encodeURIComponent(KEY), { keyHash: "" });
    check("an empty key in the build: the card shows", await r.T.ready, null);
    const s = page("https://site.example/bejeweled.html?terms=1#testkey=" + encodeURIComponent(KEY));
    check("?terms=1 wins over the key, which still leaves", [await s.T.ready, s.loc.hash], [null, ""]);
    const d = page("http://127.0.0.1:18795/index.html#testkey=" + encodeURIComponent(KEY));
    check("a key on a development host leaves the address bar too", [await d.T.ready, d.loc.hash], ["dev", ""]);
    const m = page("https://site.example/index.html#mods&testkey=" + encodeURIComponent(KEY));
    check("the fragment's other parts stay", [await m.T.ready, m.loc.hash], ["testkey", "#mods"]);
    const old = page("https://site.example/bejeweled.html?testkey=" + encodeURIComponent(KEY));
    check("the key in the query is not read", await old.T.ready, null);
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

  // the listing's agreement for each game it adds
  check("agreeTo is in web/addgames.js", agreeSource.length > 0, true);
  {
    const L = listing("https://site.example/index.html");
    const d = L.els["terms-dialog"], box = L.els["terms-agree"], go = L.els["btn-terms"];
    let result = "waiting";
    L.agreeTo("bejeweled").then((r) => { result = r; });
    check("a game not agreed to: the dialog opens with its terms", [d.open, L.els["terms-text"].innerHTML, L.els["terms-text"].scrollTop],
      [true, "<p>The terms</p>", 0]);
    check("titled with the game", L.els["terms-title"].textContent, 'game.terms.title {"title":"Game bejeweled"}');
    check("Continue waits for the box", go.disabled, true);
    go.fire("click");
    await Promise.resolve();
    check("a click without the box does nothing", [result, d.open], ["waiting", true]);
    box.checked = true; box.fire("change");
    check("the box enables Continue", go.disabled, false);
    go.fire("click");
    await Promise.resolve();
    check("agreed: kept for the game at its version, the dialog closed", [result, L.local, d.open], [true, { "byopop-terms-bejeweled": "v7" }, false]);
    check("its listeners gone and the focus back", [L.listeners(), L.focused], [0, ["add-button"]]);
    let again = "waiting";
    L.agreeTo("bejeweled").then((r) => { again = r; });
    await Promise.resolve();
    check("not asked again", [again, d.open], [true, false]);
  }
  {
    const L = listing("https://site.example/index.html");
    let a = "waiting", b = "waiting";
    L.agreeTo("bejeweled").then((r) => { a = r; });
    L.els["btn-terms-skip"].fire("click");
    await Promise.resolve();
    let prevented = false;
    L.agreeTo("bejeweled").then((r) => { b = r; });
    L.els["terms-dialog"].fire("cancel", { preventDefault: () => { prevented = true; } });
    await Promise.resolve();
    check("Don't add, and Escape: not agreed, nothing kept", [a, b, prevented, L.local, L.listeners()], [false, false, true, {}, 0]);
  }
  {
    const L = listing("https://site.example/index.html", { local: { "byopop-terms-bejeweled": "v7" } });
    check("agreed before, on a game page or here: not asked", [await L.agreeTo("bejeweled"), L.els["terms-dialog"].open], [true, false]);
    const M = listing("https://site.example/index.html", { local: { "byopop-terms-bejeweled": "v6" } });
    M.agreeTo("bejeweled");
    check("agreed to older terms: asked again", M.els["terms-dialog"].open, true);
    check("a game the build gave no terms: never added", await L.agreeTo("tiptop"), false);
    const D = listing("http://127.0.0.1:18795/index.html");
    check("a development host: not asked, nothing kept", [await D.agreeTo("bejeweled"), D.els["terms-dialog"].open, D.local], [true, false, {}]);
    const T = listing("https://site.example/index.html?terms=1");
    T.agreeTo("bejeweled");
    check("?terms=1: asked", T.els["terms-dialog"].open, true);
  }

  // the web build: the hash baked in, the key nowhere (only when the build and the key's file are here; nothing printed of either)
  {
    const root = path.join(__dirname, "..");
    const keyFile = path.join(root, "secrets", "web-testkey.txt"), built = path.join(root, "build", "web", "terms.js");
    if (fs.existsSync(keyFile) && fs.existsSync(built)) {
      const key = fs.readFileSync(keyFile, "utf8").split(/\r?\n/)[0].trim();
      const hash = crypto.createHash("sha256").update(key).digest("hex");
      check("the build's terms.js holds the key's hash", fs.readFileSync(built, "utf8").indexOf('"' + hash + '"') >= 0, true);
      const dir = path.dirname(built);
      const leaks = fs.readdirSync(dir).filter((f) => /\.(js|html|json|txt|css|mjs|toml)$/.test(f))
        .filter((f) => key && fs.readFileSync(path.join(dir, f), "utf8").indexOf(key) >= 0);
      check("no file of the build holds the key", leaks.length, 0);
    }
  }

  console.log(`termstest: ${checks} checks, ${failed} failed`);
  process.exit(failed ? 1 : 0);
})();
