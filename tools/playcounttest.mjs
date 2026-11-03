// Checks the site's play counter (web/counter/playcount.mjs; docs/PLAY_COUNTER.md) under Node, without Cloudflare: the
// Worker and the Durable Object run against small stand-ins for the object's state (its SQLite database through node:sqlite,
// its sockets, tags and attachments) and for the runtime's WebSocketPair and 101 responses.
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
const Worker = mod.default, PlayCounter = mod.PlayCounter;

function makeSite(opts) {
  const state = makeState();
  const env = { GAMES: "bejeweled, bejeweledtwist,tiptop", ORIGIN: (opts && opts.origin) || "" };
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
    async counts() { return (await (await Worker.fetch(new Request("https://site.example/api/counts"), env, {})).json()).games; }
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
  check("one row written per play", s.state.written, 1);
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
  check("a wake forgets the cooldown (memory only)", (await s.counts()).tiptop, { plays: 3, live: 4 });
  check("the counts survive a wake", s.state.written, 3);
}
{
  const s = makeSite();
  const statuses = [];
  for (let i = 0; i < 10; i++) statuses.push((await s.play("bejeweled", { count: false })).status);
  check("at most 8 sockets from one source", statuses, [101, 101, 101, 101, 101, 101, 101, 101, 429, 429]);
  check("another source is not held back", (await s.play("bejeweled", { ip: "192.0.2.9" })).status, 101);
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
  s.object.webSocketMessage(server, "ping");
  check("a small frame is ignored", server.closed, null);
  s.object.webSocketMessage(server, "x".repeat(65));
  check("a large frame closes the socket", server.closed && server.closed.code, 1009);
  const r2 = await s.play("bejeweled", { ip: "192.0.2.10" });
  s.object.webSocketMessage(r2.webSocket.peer, new Uint8Array(1000).buffer);
  check("a large binary frame too", r2.webSocket.peer.closed && r2.webSocket.peer.closed.code, 1009);
}
// nothing personal is kept: the database holds only game ids and counts, the sockets only a salted hash and the game
{
  const s = makeSite();
  const r = await s.play("bejeweled", { ip: "203.0.113.77" });
  const rows = s.state.storage.sql.exec("SELECT * FROM plays");
  check("the database's rows", rows.map((x) => Object.keys(x)), [["game", "n"]]);
  const tags = r.webSocket.peer.tags;
  check("the socket's tags: a hash and the game", [/^s:[0-9a-f]{16}$/.test(tags[0]), tags[1]], [true, "g:bejeweled"]);
  check("the address is in no tag or attachment", JSON.stringify([tags, r.webSocket.peer.attachment]).indexOf("203.0.113.77"), -1);
  const t2 = makeSite();
  const other = await t2.play("bejeweled", { ip: "203.0.113.77" });
  check("each object's salt is its own", other.webSocket.peer.tags[0] === tags[0], false);
}
// the counts response
{
  const s = makeSite();
  const res = await Worker.fetch(new Request("https://site.example/api/counts"), s.env, {});
  check("counts: cacheable for 30 s", res.headers.get("cache-control"), "public, max-age=30");
  check("counts with no plays", await res.json(), { games: {} });
}

console.log(`playcounttest: ${checks} checks, ${failed} failed`);
process.exit(failed ? 1 : 0);
