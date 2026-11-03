// The site's play counter (docs/PLAY_COUNTER.md): a Cloudflare Worker and one Durable Object for the whole site. It counts each
// game's plays for good and the pages playing it now, and keeps no address, cookie or identifier of a player. Not deployed:
// the pages contact it only when a build names its address (BYOPOP_WEB_COUNTER), and nothing goes on a public host until the
// user decides to put the site up.
//
// The Worker, on the site's own domain:
//   GET /api/counts          {"games": {"<id>": {"plays": n, "live": n}, ...}}, every game with a play or a page playing it;
//                            cached at the edge for 30 s, so a busy listing page costs the object one request in 30 s
//   GET /api/play?game=<id>  a WebSocket upgrade, from a game page whose game is playing: opening it counts a play (unless
//                            the page adds &count=0) and while it is open the page counts as playing now
// Its bindings (wrangler.toml, written by the build): COUNTER, the Durable Object namespace of PlayCounter; GAMES, the site's
// game ids, comma-separated (from the build's table of games, chat-games.js); ORIGIN, the site's origin, which the play
// socket's Origin header must be (empty: not checked, for a local test); SOURCE_KEY, a secret (wrangler secret put) that keys
// the hash of a connection's address, without which no play is taken; PLAY_LIMIT and COUNTS_LIMIT, Cloudflare's rate limiters
// for each route (absent: not limited, for a local test).
//
// A connection source (an address, a household behind it) is known only by a short HMAC-SHA-256 of its address, keyed by
// SOURCE_KEY and the UTC date, made here in the Worker: the object never sees the address, and the same address has the same
// tag all day, so the limits hold when the object hibernates and wakes again. The object keeps the lifetime counts in its
// SQLite database, one row a game, and each source's counted plays of the last COOLDOWN_MS (its tag, the game and the time),
// so a counted play writes two rows and clears the expired ones. The live count is its open sockets, by the tag of the game
// each was opened for. It sets no timer or alarm, and the page's keep-alive "ping" is answered by the runtime without waking
// it (setWebSocketAutoResponse), so between plays it hibernates; any other frame that reaches it closes its socket.

const SOCKETS_PER_SOURCE = 8;         // open play sockets for one connection source
const SOCKETS = 10000;                // open play sockets for the whole site
const COOLDOWN_MS = 5 * 60 * 1000;    // a source's plays of one game in this time count once
const ID = /^[a-z0-9_-]{1,40}$/;

function games(env) {
  return new Set(String(env.GAMES || "").split(/[\s,]+/).filter((id) => ID.test(id)));
}
function json(value, extra) {
  return new Response(JSON.stringify(value), { headers: Object.assign({ "content-type": "application/json" }, extra || {}) });
}
function counter(env) {
  return env.COUNTER.get(env.COUNTER.idFromName("site"));
}
// whether the route's rate limiter lets this key through (no limiter bound: yes)
async function allowed(limiter, key) {
  if (!limiter || typeof limiter.limit !== "function") return true;
  try { return (await limiter.limit({ key: key })).success !== false; } catch (e) { return true; }
}
// A source's tag: "s:" and 16 hex digits of HMAC-SHA-256(SOURCE_KEY, the UTC date, a space and the address)
export async function sourceTag(secret, address, now) {
  const enc = new TextEncoder();
  const key = await crypto.subtle.importKey("raw", enc.encode(secret), { name: "HMAC", hash: "SHA-256" }, false, ["sign"]);
  const day = new Date(now).toISOString().slice(0, 10);
  const mac = new Uint8Array(await crypto.subtle.sign("HMAC", key, enc.encode(day + " " + address)));
  return "s:" + Array.from(mac.slice(0, 8), (b) => b.toString(16).padStart(2, "0")).join("");
}

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    const address = request.headers.get("CF-Connecting-IP") || "";
    if (url.pathname === "/api/counts") {
      if (request.method !== "GET") return new Response(null, { status: 405 });
      // keyed by the address itself: the limiter is Cloudflare's own counter, kept by nobody here
      if (!(await allowed(env.COUNTS_LIMIT, address))) return new Response(null, { status: 429 });
      const cache = (typeof caches !== "undefined" && caches.default) || null;
      const key = new Request(url.origin + "/api/counts");
      if (cache) {
        const hit = await cache.match(key);
        if (hit) return hit;
      }
      const answer = await counter(env).fetch("https://counter/counts");
      const res = new Response(await answer.text(), { headers: { "content-type": "application/json", "cache-control": "public, max-age=30" } });
      if (cache && ctx && ctx.waitUntil) ctx.waitUntil(cache.put(key, res.clone()));
      return res;
    }
    if (url.pathname === "/api/play") {
      if ((request.headers.get("Upgrade") || "").toLowerCase() !== "websocket") return new Response(null, { status: 426 });
      if (env.ORIGIN && request.headers.get("Origin") !== env.ORIGIN) return new Response(null, { status: 403 });
      const game = url.searchParams.get("game") || "";
      if (!games(env).has(game)) return new Response(null, { status: 404 });
      if (!env.SOURCE_KEY) return new Response(null, { status: 503 });
      const source = await sourceTag(env.SOURCE_KEY, address, Date.now());
      if (!(await allowed(env.PLAY_LIMIT, source))) return new Response(null, { status: 429 });
      const count = url.searchParams.get("count") === "0" ? "0" : "1";
      return counter(env).fetch(new Request("https://counter/play?game=" + game + "&count=" + count, {
        headers: { "Upgrade": "websocket", "X-Source": source }
      }));
    }
    return new Response(null, { status: 404 });
  }
};

export class PlayCounter {
  constructor(ctx, env) {
    this.ctx = ctx;
    this.env = env;
    ctx.storage.sql.exec("CREATE TABLE IF NOT EXISTS plays (game TEXT PRIMARY KEY, n INTEGER NOT NULL) WITHOUT ROWID");
    ctx.storage.sql.exec("CREATE TABLE IF NOT EXISTS recent (k TEXT PRIMARY KEY, at INTEGER NOT NULL) WITHOUT ROWID");
    ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair("ping", "pong"));
  }

  async fetch(request) {
    const url = new URL(request.url);
    if (url.pathname === "/counts") return json({ games: this.counts() });
    if (url.pathname === "/play") {
      const game = url.searchParams.get("game") || "";
      const source = request.headers.get("X-Source") || "";
      if (!games(this.env).has(game) || !/^s:[0-9a-f]{16}$/.test(source)) return new Response(null, { status: 404 });
      if (this.ctx.getWebSockets(source).length >= SOCKETS_PER_SOURCE) return new Response(null, { status: 429 });
      if (this.ctx.getWebSockets().length >= SOCKETS) return new Response(null, { status: 503 });
      const pair = new WebSocketPair();
      const client = pair[0], server = pair[1];
      this.ctx.acceptWebSocket(server, [source, "g:" + game]);
      if (url.searchParams.get("count") !== "0" && this.fresh(source, game, Date.now())) {
        this.ctx.storage.sql.exec("INSERT INTO plays (game, n) VALUES (?, 1) ON CONFLICT (game) DO UPDATE SET n = n + 1", game);
      }
      return new Response(null, { status: 101, webSocket: client });
    }
    return new Response(null, { status: 404 });
  }

  // {id: {plays, live}} for every game with a play or an open socket
  counts() {
    const out = {};
    for (const row of this.ctx.storage.sql.exec("SELECT game, n FROM plays")) out[row.game] = { plays: row.n, live: 0 };
    for (const id of games(this.env)) {
      const live = this.ctx.getWebSockets("g:" + id).length;
      if (!live) continue;
      out[id] = out[id] || { plays: 0, live: 0 };
      out[id].live = live;
    }
    return out;
  }

  // Whether this play of a game from a source counts: not if the source had one counted in the last COOLDOWN_MS. Kept in the
  // database, so a wake forgets nothing; rows older than that go on the next play.
  fresh(source, game, now) {
    const sql = this.ctx.storage.sql;
    sql.exec("DELETE FROM recent WHERE at <= ?", now - COOLDOWN_MS);
    const key = source + " " + game;
    if (Array.from(sql.exec("SELECT k FROM recent WHERE k = ?", key)).length) return false;
    sql.exec("INSERT INTO recent (k, at) VALUES (?, ?)", key, now);
    return true;
  }

  // The page sends only "ping", which the runtime answers without waking the object; a frame that does reach it is not the
  // page's, and each one would spend a request of the object's daily allowance, so it closes the socket.
  webSocketMessage(ws) {
    try { ws.close(1008, "unexpected frame"); } catch (e) {}
  }

  webSocketClose(ws, code) {
    // the runtime answers the close itself (compatibility date 2026-04-07 or later); closing again is harmless
    try { ws.close(code >= 1000 && code < 5000 && code !== 1005 && code !== 1006 ? code : 1000); } catch (e) {}
  }

  webSocketError() {}
}
