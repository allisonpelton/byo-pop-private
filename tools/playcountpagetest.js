// Checks the pages' side of the play counter (web/playcount.js; docs/PLAY_COUNTER.md) under Node, in a stand-in browser: when
// it contacts the counter and when it must not (no address in the build, ?silent=1, automation, no WebSocket, a hidden page, a
// development host even with ?terms=1, the site's test key, with the real web/terms.js beside it), what a game page's socket asks
// for (count=0 within the cooldown, again after the back/forward cache), and what the listing page takes from the counts.
//   node tools/playcountpagetest.js
// It prints "playcountpagetest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const crypto = require("crypto");

const source = fs.readFileSync(path.join(__dirname, "..", "web", "playcount.js"), "utf8");
const KEY = "a test key";
const HASH = crypto.createHash("sha256").update(KEY).digest("hex");
const terms = fs.readFileSync(path.join(__dirname, "..", "web", "terms.js"), "utf8").replace('"@BYOPOP_TESTKEY_HASH@"', JSON.stringify(HASH));

// A page: its address, whether it is shown, and what it contacts
function page(opts) {
  opts = opts || {};
  const contacts = [], listeners = {}, timers = [], store = Object.assign({}, opts.store || {}), session = Object.assign({}, opts.session || {});
  const u = new URL((opts.href || "https://site.example/bejeweled.html") + (opts.search || ""));
  const loc = { href: u.href, protocol: u.protocol, hostname: u.hostname, pathname: u.pathname, search: u.search, hash: u.hash };
  const document = {
    visibilityState: opts.hidden ? "hidden" : "visible",
    addEventListener: (t, f) => { (listeners[t] = listeners[t] || []).push(f); },
    removeEventListener: (t, f) => { listeners[t] = (listeners[t] || []).filter((g) => g !== f); }
  };
  class FakeSocket {
    constructor(url) { this.url = url; this.sent = []; contacts.push({ kind: "socket", url }); FakeSocket.last = this; }
    send(m) { this.sent.push(m); }
    close(code) { this.closed = code; }
  }
  const window = {
    addEventListener: (t, f) => { (listeners["w:" + t] = listeners["w:" + t] || []).push(f); }
  };
  const ctx = {
    window, document,
    location: loc,
    history: { state: null, replaceState: (st, t, url) => { const n = new URL(url, loc.href); loc.href = n.href; loc.search = n.search; loc.hash = n.hash; } },
    sessionStorage: { getItem: (k) => (k in session ? session[k] : null), setItem: (k, v) => { session[k] = String(v); } },
    TextEncoder, Promise, crypto: globalThis.crypto,
    navigator: { webdriver: !!opts.webdriver },
    URL, URLSearchParams, JSON, Date: opts.Date || Date, Object, Math,
    localStorage: { getItem: (k) => (k in store ? store[k] : null), setItem: (k, v) => { store[k] = String(v); } },
    setInterval: (f, ms) => { timers.push({ f, ms }); return timers.length; },
    clearInterval: () => {},
    setTimeout: (f, ms) => { timers.push({ f, ms, once: true }); return timers.length; },
    fetch: (url) => {
      contacts.push({ kind: "fetch", url });
      return Promise.resolve({ ok: true, json: () => Promise.resolve(opts.answer || { games: {} }) });
    },
    console
  };
  if (!opts.noWebSocket) ctx.WebSocket = FakeSocket;
  vm.createContext(ctx);
  vm.runInContext(terms, ctx, { filename: "terms.js" });
  const base = opts.base === undefined ? "/api" : opts.base;
  vm.runInContext(base === null ? source : source.replace('"@BYOPOP_COUNTER@"', JSON.stringify(base)), ctx, { filename: "playcount.js" });
  return {
    C: vm.runInContext("ByopopCounter", ctx), contacts, store, timers, FakeSocket,
    show() { document.visibilityState = "visible"; (listeners.visibilitychange || []).slice().forEach((f) => f()); },
    leave(persisted) { (listeners["w:pagehide"] || []).forEach((f) => f({ persisted: !!persisted })); },
    back() { (listeners["w:pageshow"] || []).forEach((f) => f({ persisted: true })); }
  };
}

let checks = 0, failed = 0;
function check(what, got, want) {
  checks++;
  const g = JSON.stringify(got), w = JSON.stringify(want);
  if (g !== w) { failed++; console.log(`FAIL ${what}: got ${g}, want ${w}`); }
}
const tick = () => new Promise((r) => setImmediate(r));

(async () => {
  // off: nothing is contacted
  for (const [what, opts] of [["no address in the build (the marker left)", { base: null }], ["an empty address", { base: "" }],
    ["?silent=1", { search: "?silent=1" }], ["automation", { webdriver: true }], ["no WebSocket (Node)", { noWebSocket: true }]]) {
    const p = page(opts);
    p.C.playing("bejeweled");
    p.C.counts(() => {});
    await tick();
    check("off, " + what, [p.C.on(), p.contacts.length], [false, 0]);
  }
  // a development host and the site's test key: no play and no counts read
  for (const [what, opts] of [["127.0.0.1", { href: "http://127.0.0.1:18795/bejeweled.html" }],
    ["?terms=1 on a development host", { href: "http://127.0.0.1:18795/bejeweled.html", search: "?terms=1" }],
    ["a *.test host", { href: "https://byo-pop.test/" }], ["a file:// page", { href: "file:///C:/build/web/index.html" }],
    ["the test key's mark in the tab", { session: { "byopop-testkey": HASH } }],
    ["the test key in the fragment", { search: "#testkey=" + encodeURIComponent(KEY) }]]) {
    const p = page(opts);
    p.C.counts(() => {});
    await tick(); await tick();
    p.C.playing("bejeweled");
    check("off, " + what, [p.C.on(), p.contacts.length], [false, 0]);
  }
  {
    const p = page({ href: "https://site.example/", search: "#testkey=wrong" });
    p.C.counts(() => {});
    await tick(); await tick(); await tick();
    check("a wrong key: the counts are read once it is checked", p.contacts.length, 1);
  }
  // a game page
  {
    const p = page();
    p.C.playing("bejeweled");
    check("a play opens one socket, counted", p.contacts.map((c) => c.url), ["wss://site.example/api/play?game=bejeweled"]);
    p.C.playing("bejeweled");
    check("once a page", p.contacts.length, 1);
    p.FakeSocket.last.onopen();
    const ping = p.timers.filter((t) => t.ms === 50000)[0];
    ping.f();
    check("a ping every 50 s", p.FakeSocket.last.sent, ["ping"]);
    p.leave();
    check("closed when the page goes", p.FakeSocket.last.closed, 1000);
  }
  {
    const p = page();
    p.C.playing("bejeweled");
    const first = p.FakeSocket.last;
    p.leave(true);
    check("closed while the page is in the back/forward cache", first.closed, 1000);
    first.onclose();
    check("and not opened again while it is away", [p.contacts.length, p.timers.filter((t) => t.once).length], [1, 0]);
    p.back();
    check("opened again, not counted, when it comes back", p.contacts.map((c) => c.url)[1], "wss://site.example/api/play?game=bejeweled&count=0");
    p.C.stop();
    p.leave(true);
    p.back();
    check("but not after the game stopped", p.contacts.length, 2);
  }
  {
    const p = page({ store: { "byopop-counted": JSON.stringify({ bejeweled: Date.now() - 60000 }) } });
    p.C.playing("bejeweled");
    check("counted here a minute ago: count=0", p.contacts[0].url, "wss://site.example/api/play?game=bejeweled&count=0");
    const q = page({ store: { "byopop-counted": JSON.stringify({ bejeweled: Date.now() - 6 * 60000 }) } });
    q.C.playing("bejeweled");
    check("six minutes ago: counted", q.contacts[0].url, "wss://site.example/api/play?game=bejeweled");
    check("the browser keeps only the time of its last counted plays", Object.keys(JSON.parse(q.store["byopop-counted"])), ["bejeweled"]);
  }
  {
    const p = page({ hidden: true });
    p.C.playing("bejeweled");
    check("a hidden page waits", p.contacts.length, 0);
    p.show();
    check("and plays once shown", p.contacts.length, 1);
  }
  {
    const p = page({ base: "https://counter.example/api/" });
    p.C.playing("tiptop");
    check("a full address", p.contacts[0].url, "wss://counter.example/api/play?game=tiptop");
    const q = page();
    q.C.playing("../evil");
    check("a game id that is not an id", q.contacts.length, 0);
  }
  {
    const p = page();
    p.C.playing("bejeweled");
    p.FakeSocket.last.onclose();
    const again = p.timers.filter((t) => t.once)[0];
    again.f();
    check("a dropped socket is opened again, not counted", p.contacts.map((c) => c.url)[1], "wss://site.example/api/play?game=bejeweled&count=0");
    p.C.stop();
    p.FakeSocket.last.onclose();
    check("not after stop", p.timers.filter((t) => t.once).length, 1);
  }
  // the listing page
  {
    const p = page({ href: "https://site.example/", answer: { games: { bejeweled: { plays: 12, live: 2 }, tiptop: { plays: -4, live: "x" }, "../x": { plays: 9 } } } });
    let got = null;
    p.C.counts((g) => { got = g; });
    await tick(); await tick();
    check("the counts, one request", p.contacts.map((c) => c.url), ["https://site.example/api/counts"]);
    check("the counts as the page takes them", got, { bejeweled: { plays: 12, live: 2 }, tiptop: { plays: 0, live: 0 } });
    check("read again every 2 minutes", p.timers.filter((t) => t.ms === 120000).length, 1);
  }
  {
    const p = page({ hidden: true, href: "https://site.example/" });
    p.C.counts(() => {});
    await tick();
    check("a hidden listing does not ask", p.contacts.length, 0);
    p.show();
    await tick();
    check("until it is shown", p.contacts.length, 1);
  }

  console.log(`playcountpagetest: ${checks} checks, ${failed} failed`);
  process.exit(failed ? 1 : 0);
})();
