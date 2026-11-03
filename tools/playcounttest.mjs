// Checks the site's play counter (web/counter/playcount.mjs; docs/PLAY_COUNTER.md) under Node, without Cloudflare: the
// Worker and the Durable Object run against small stand-ins for the object's state (its SQLite database through node:sqlite,
// its sockets, tags and attachments), for the runtime's WebSocketPair and 101 responses and for the rate limiters.
//   node tools/playcounttest.mjs
// It prints "playcounttest: N checks, M failed" and exits 1 when any failed.
import { DatabaseSync } from "node:sqlite";
import { fileURLToPath, pathToFileURL } from "node:url";
import path from "node:path";

// ---------- the runtime's stand-ins ----------
class FakeSocket {
  constructor() { this.peer = null; this.closed = null; this.attachment = null; this.tags = []; }
  close(code, reason) { if (!this.closed) this.closed = { code, reason }; if (this.peer && !this.peer.closed) this.peer.closed = { code, reason }; }
  serializeAttachment(v) { this.attachment = JSON.parse(JSON.stringify(v)); }
  deserializeAttachment() { return this.attachment; }
}
globalThis.WebSocketPair = function () { const a = new FakeSocket(), b = new FakeSocket(); a.peer = b; b.peer = a; this[0] = a; this[1] = b; };
globalThis.WebSocketRequestResponsePair = function (request, response) { this.request = request; this.response = response; };
const NativeResponse = globalThis.Response;
globalThis.Response = class extends NativeResponse {
  constructor(body, init) {
    if (init && init.status === 101) { super(null, { status: 200 }); this.upgraded = 101; this.webSocket = init.webSocket; }
    else super(body, init);
  }
  get status() { return this.upgraded || super.status; }
};

function makeState() {
  const db = new DatabaseSync(":memory:");
  let written = 0;
  const sockets = [];
  const state = {
    autoResponse: null,
    storage: { sql: { exec(sql, ...binds) {
      const st = db.prepare(sql);
      if (/^\s*SELECT/i.test(sql)) return st.all(...binds);
      const r = st.run(...binds);
      if (/^\s*(INSERT|UPDATE|DELETE)/i.test(sql)) written += r.changes;  // (SQLite reports the last change again after DDL)
      return [];
    } } },
    acceptWebSocket(ws, tags) { ws.tags = tags || []; sockets.push(ws); },
    getWebSockets(tag) { return sockets.filter((ws) => !ws.closed && (tag === undefined || ws.tags.indexOf(tag) >= 0)); },
    setWebSocketAutoResponse(pair) { state.autoResponse = pair; },
    get written() { return written; }
  };
  return state;
}

const here = path.dirname(fileURLToPath(import.meta.url));
const mod = await import(pathToFileURL(path.join(here, "..", "web", "counter", "playcount.mjs")).href);
const Worker = mod.default, PlayCounter = mod.PlayCounter, sourceTag = mod.sourceTag;

// a rate limiter that lets `limit` calls through for each key, then refuses
function limiter(limit) {
  const seen = new Map();
  return { limit: async ({ key }) => { const n = (seen.get(key) || 0) + 1; seen.set(key, n); return { success: n <= limit }; } };
}

function makeSite(opts) {
  const state = makeState();
  opts = opts || {};
  const env = { GAMES: "bejeweled, bejeweledtwist,tiptop", ORIGIN: opts.origin || "", SOURCE_KEY: "key" in opts ? opts.key : "test secret" };
  if (opts.playLimit) env.PLAY_LIMIT = limiter(opts.playLimit);
  if (opts.countsLimit) env.COUNTS_LIMIT = limiter(opts.countsLimit);
  let object = new PlayCounter(state, env);
  let objectRequests = 0;
  env.COUNTER = {
    idFromName: (name) => name,
    get: () => ({ fetch: (req) => { objectRequests++; return object.fetch(typeof req === "string" ? new Request(req) : req); } })
  };
  return {
    state, env,
    get objectRequests() { return objectRequests; },
    // the object evicted and woken again: a new instance over the same state
    wake() { object = new PlayCounter(state, env); },
    get object() { return object; },
    play(game, o) {
      o = o || {};
      const q = "game=" + encodeURIComponent(game) + (o.count === false ? "&count=0" : "");
      const headers = { "Upgrade": "websocket", "CF-Connecting-IP": o.ip || "203.0.113.1" };
      if (o.origin) headers.Origin = o.origin;
      return Worker.fetch(new Request("https://site.example/api/play?" + q, { headers }), env, {});
    },
    countsResponse(ip) { return Worker.fetch(new Request("https://site.example/api/counts", { headers: { "CF-Connecting-IP": ip || "203.0.113.1" } }), env, {}); },
    async counts() { return (await (await this.countsResponse()).json()).games; }
  };
}

let checks = 0, failed = 0;
function check(what, got, want) {
  checks++;
  const g = JSON.stringify(got), w = JSON.stringify(want);
  if (g !== w) { failed++; console.log(`FAIL ${what}: got ${g}, want ${w}`); }
}

// a play counts and is live while its socket is open
{
  const s = makeSite();
  const r = await s.play("bejeweled");
  check("a play is an upgrade", r.status, 101);
  check("counts after a play", await s.counts(), { bejeweled: { plays: 1, live: 1 } });
  r.webSocket.close(1000);
  s.object.webSocketClose(r.webSocket.peer, 1000);
  check("closed: no longer live", await s.counts(), { bejeweled: { plays: 1, live: 0 } });
  check("two rows written per counted play: its count and its cooldown", s.state.written, 2);
  check("the keep-alive is answered without the object", [s.state.autoResponse.request, s.state.autoResponse.response], ["ping", "pong"]);
}
// the limits: the cooldown, count=0, sockets per source
{
  const s = makeSite();
  await s.play("tiptop");
  await s.play("tiptop");
  check("a second play from the same source within minutes is not counted", (await s.counts()).tiptop, { plays: 1, live: 2 });
  await s.play("tiptop", { ip: "198.51.100.7" });
  check("another source's play counts", (await s.counts()).tiptop, { plays: 2, live: 3 });
  await s.play("bejeweled", { count: false, ip: "198.51.100.8" });
  check("count=0: live but not counted", (await s.counts()).bejeweled, { plays: 0, live: 1 });
  s.wake();
  await s.play("tiptop");
  check("the cooldown survives a wake", (await s.counts()).tiptop, { plays: 2, live: 4 });
  await s.play("bejeweled");
  check("but not for another game", (await s.counts()).bejeweled, { plays: 1, live: 2 });
  const realNow = Date.now;
  Date.now = () => realNow() + 5 * 60 * 1000 + 1;
  try { await s.play("tiptop"); } finally { Date.now = realNow; }
  check("counted again once the cooldown is over", (await s.counts()).tiptop.plays, 3);
  check("the cooldown's rows of expired plays go", s.state.storage.sql.exec("SELECT k FROM recent").length, 1);
}
{
  const s = makeSite();
  const statuses = [];
  for (let i = 0; i < 10; i++) {
    if (i === 4) s.wake();
    statuses.push((await s.play("bejeweled", { count: false })).status);
  }
  check("at most 8 sockets from one source, across a wake", statuses, [101, 101, 101, 101, 101, 101, 101, 101, 429, 429]);
  check("another source is not held back", (await s.play("bejeweled", { ip: "192.0.2.9" })).status, 101);
}
{
  const s = makeSite();
  for (let i = 0; i < 10000; i++) {
    const tag = "s:" + i.toString(16).padStart(16, "0");
    await s.object.fetch(new Request("https://counter/play?game=tiptop&count=0", { headers: { "X-Source": tag } }));
  }
  check("at most 10000 sockets for the site", (await s.play("tiptop", { ip: "192.0.2.200" })).status, 503);
  check("a tag that is not a hash", (await s.object.fetch(new Request("https://counter/play?game=tiptop", { headers: { "X-Source": "203.0.113.1" } }))).status, 404);
}
// the rate limiters and the secret
{
  const s = makeSite({ playLimit: 3, countsLimit: 2 });
  const statuses = [];
  for (let i = 0; i < 4; i++) statuses.push((await s.play("tiptop", { count: false })).status);
  check("the play limiter, keyed by the source", statuses, [101, 101, 101, 429]);
  check("another source has its own", (await s.play("tiptop", { ip: "192.0.2.50" })).status, 101);
  const reads = [];
  for (let i = 0; i < 3; i++) reads.push((await s.countsResponse()).status);
  check("the counts limiter", reads, [200, 200, 429]);
  check("no secret: no play taken", (await makeSite({ key: "" }).play("tiptop")).status, 503);
}
// what the Worker refuses
{
  const s = makeSite({ origin: "https://site.example" });
  check("an unknown game", (await s.play("notagame", { origin: "https://site.example" })).status, 404);
  check("a game id that is not an id", (await s.play("../x", { origin: "https://site.example" })).status, 404);
  check("another site's page", (await s.play("bejeweled", { origin: "https://evil.example" })).status, 403);
  check("no Origin", (await s.play("bejeweled")).status, 403);
  check("the site's own page", (await s.play("bejeweled", { origin: "https://site.example" })).status, 101);
  const plain = await Worker.fetch(new Request("https://site.example/api/play?game=bejeweled"), s.env, {});
  check("not an upgrade", plain.status, 426);
  check("a post to the counts", (await Worker.fetch(new Request("https://site.example/api/counts", { method: "POST" }), s.env, {})).status, 405);
  check("another path", (await Worker.fetch(new Request("https://site.example/api/other"), s.env, {})).status, 404);
  check("nothing refused was counted", Object.keys(await s.counts()), ["bejeweled"]);
}
// frames
{
  const s = makeSite();
  const r = await s.play("bejeweled");
  const server = r.webSocket.peer;
  s.object.webSocketMessage(server, "x");
  check("any frame that reaches the object closes the socket", server.closed && server.closed.code, 1008);
  const r2 = await s.play("bejeweled", { ip: "192.0.2.10" });
  s.object.webSocketMessage(r2.webSocket.peer, new Uint8Array(1000).buffer);
  check("a binary frame too", r2.webSocket.peer.closed && r2.webSocket.peer.closed.code, 1008);
}
// nothing personal is kept: the database holds only game ids and counts, the sockets only a salted hash and the game
{
  const s = makeSite();
  const r = await s.play("bejeweled", { ip: "203.0.113.77" });
  const rows = s.state.storage.sql.exec("SELECT * FROM plays");
  check("the database's rows", rows.map((x) => Object.keys(x)), [["game", "n"]]);
  const recent = s.state.storage.sql.exec("SELECT * FROM recent");
  check("the cooldown's rows: a hash and the game, and the time", [recent.length, /^s:[0-9a-f]{16} bejeweled$/.test(recent[0].k)], [1, true]);
  check("the address is in no row", JSON.stringify([rows, recent]).indexOf("203.0.113.77"), -1);
  const tags = r.webSocket.peer.tags;
  check("the socket's tags: a hash and the game", [/^s:[0-9a-f]{16}$/.test(tags[0]), tags[1]], [true, "g:bejeweled"]);
  check("the address is in no tag or attachment", JSON.stringify([tags, r.webSocket.peer.attachment]).indexOf("203.0.113.77"), -1);
  const now = Date.now();
  check("the tag is a hash keyed by the secret and the date", tags[0], await sourceTag("test secret", "203.0.113.77", now));
  check("another secret, another tag", (await sourceTag("other secret", "203.0.113.77", now)) === tags[0], false);
  check("another day, another tag", (await sourceTag("test secret", "203.0.113.77", now + 86400000)) === tags[0], false);
}
// the counts response
{
  const s = makeSite();
  const res = await s.countsResponse();
  check("counts: cacheable for 30 s", res.headers.get("cache-control"), "public, max-age=30");
  check("counts with no plays", await res.json(), { games: {} });
}

console.log(`playcounttest: ${checks} checks, ${failed} failed`);
process.exit(failed ? 1 : 0);
