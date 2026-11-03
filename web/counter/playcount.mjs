// The site's play counter (docs/PLAY_COUNTER.md): a Cloudflare Worker and one Durable Object for the whole site. It counts each
// game's plays for good and the pages playing it now, and holds nothing about the players (no IP address, cookie or
// identifier). Not deployed: the pages contact it only when a build names its address (BYOPOP_WEB_COUNTER), and nothing goes
// on a public host until the terms card is on again (docs/WEB_PLAYER.md, "The terms").
//
// The Worker, on the site's own domain:
//   GET /api/counts          {"games": {"<id>": {"plays": n, "live": n}, ...}}, every game with a play or a page playing it;
//                            cached at the edge for 30 s, so a busy listing page costs the object one request in 30 s
//   GET /api/play?game=<id>  a WebSocket upgrade, from a game page whose game is playing: opening it counts a play (unless
//                            the page adds &count=0) and while it is open the page counts as playing now
// Its bindings (wrangler.toml, written by the build): COUNTER, the Durable Object namespace of PlayCounter; GAMES, the site's
// game ids, comma-separated (from the build's table of games, chat-games.js); ORIGIN, the site's origin, which the play
// socket's Origin header must be (empty: not checked, for a local test).
//
// The object keeps the lifetime counts in its SQLite database, one row a game, so a play writes one row. The live count is
// its open sockets, by the tag of the game each was opened for. It sets no timer or alarm, and the page's keep-alive
// "ping" is answered by the runtime without waking it (setWebSocketAutoResponse), so between plays it hibernates.

const MAX_FRAME = 64;                 // bytes: the page sends only "ping"; anything larger closes the socket
const SOCKETS_PER_SOURCE = 8;         // open play sockets for one connection source (an address, a household behind it)
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

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    if (url.pathname === "/api/counts") {
      if (request.method !== "GET") return new Response(null, { status: 405 });
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
      const count = url.searchParams.get("count") === "0" ? "0" : "1";
      // the address goes to the object only to be hashed there, with a salt that lives in its memory; neither keeps it
      return counter(env).fetch(new Request("https://counter/play?game=" + game + "&count=" + count, {
        headers: { "Upgrade": "websocket", "X-Source": request.headers.get("CF-Connecting-IP") || "" }
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
    // Memory only, and so new each time the object wakes: the salt of the sources' hashes, and each source's last counted
    // play of a game. Forgetting them on a wake only loosens the limits for a while.
    this.salt = crypto.getRandomValues(new Uint8Array(16));
    this.recent = new Map();
    ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair("ping", "pong"));
  }

  async fetch(request) {
    const url = new URL(request.url);
    if (url.pathname === "/counts") return json({ games: this.counts() });
    if (url.pathname === "/play") {
      const game = url.searchParams.get("game") || "";
      if (!games(this.env).has(game)) return new Response(null, { status: 404 });
      const source = await this.source(request.headers.get("X-Source") || "");
      if (this.ctx.getWebSockets(source).length >= SOCKETS_PER_SOURCE) return new Response(null, { status: 429 });
      const pair = new WebSocketPair();
      const client = pair[0], server = pair[1];
      this.ctx.acceptWebSocket(server, [source, "g:" + game]);
      server.serializeAttachment({ game: game });
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
    for (const ws of this.ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (!a || !a.game) continue;
      out[a.game] = out[a.game] || { plays: 0, live: 0 };
      out[a.game].live++;
    }
    return out;
  }

  // A connection source as a short salted hash, the tag its sockets carry while they are open
  async source(address) {
    const bytes = new TextEncoder().encode(address);
    const data = new Uint8Array(this.salt.length + bytes.length);
    data.set(this.salt);
    data.set(bytes, this.salt.length);
    const hash = new Uint8Array(await crypto.subtle.digest("SHA-256", data));
    return "s:" + Array.from(hash.slice(0, 8), (b) => b.toString(16).padStart(2, "0")).join("");
  }

  // Whether this play of a game from a source counts: not if the source had one counted in the last COOLDOWN_MS
  fresh(source, game, now) {
    for (const [key, at] of this.recent) if (now - at >= COOLDOWN_MS) this.recent.delete(key);
    const key = source + " " + game;
    if (this.recent.has(key)) return false;
    this.recent.set(key, now);
    return true;
  }

  webSocketMessage(ws, message) {
    const size = typeof message === "string" ? message.length : message.byteLength;
    if (size > MAX_FRAME) ws.close(1009, "too big");
    // anything else is ignored: the page's "ping" is answered by the runtime without waking the object
  }

  webSocketClose(ws, code) {
    // the runtime answers the close itself (compatibility date 2026-04-07 or later); closing again is harmless
    try { ws.close(code >= 1000 && code < 5000 && code !== 1005 && code !== 1006 ? code : 1000); } catch (e) {}
  }

  webSocketError() {}
}
