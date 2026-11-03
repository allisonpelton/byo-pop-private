# The play counter

The site's first feature with a server: how many pages are playing each game now, how many times each game has been played on
the site, and a "Most popular" sort on the listing page. The user asked for it on 2026-11-03 ("Would be great to add a lifetime
play count sort to the site", then "Yes, build them"). It runs on Cloudflare's free plan, as `docs/CHAT_DESIGN.md`, "A backend
that is only Cloudflare", lays out for the chat, whose pricing and limits (read on 2026-10-31) this page relies on.

It is built and tested but not deployed. No Cloudflare account, tool or connector was used and nothing was created there, and
nothing goes on a public host until the user decides to put the site up.

## How it works

The server is one file, `web/counter/playcount.mjs`: a Worker and a Durable Object class, `PlayCounter`, of which the whole site
has one instance (`idFromName("site")`). The Worker sits on the site's own domain under `/api/`, so the pages need no CORS and
send no cookie of another site.

A game page counts as playing once its game has run for 30 seconds of real play in that page: the game running, the page shown
and focused, measured by the same clock that keeps the play time on the listing page (`web/shell.html`, `playClock`). A page left
on its loading screen, its Ready card or a background tab never gets there. The page then opens one WebSocket,
`/api/play?game=<id>` (`web/playcount.js`, `ByopopCounter.playing`). Opening it counts a play; while it is open the page counts as
playing now. The page closes it when the game ends or stops and when the page goes, and if the connection drops while the game
runs it opens it again, a few times at growing intervals, without counting a play. To keep it open through proxies it sends
"ping" every 50 seconds, which the runtime answers with "pong" itself (`setWebSocketAutoResponse`), without waking the object.

The Worker checks the upgrade: the request must be a WebSocket upgrade, its `Origin` must be the site's (the `ORIGIN` variable),
and the game id must be one of the site's games (the `GAMES` variable, which the build writes from the same table as the chat's
`chat-games.js`). It forwards the upgrade to the object. The object accepts the socket with the Hibernation API
(`ctx.acceptWebSocket`), tagged with the game, and adds one to the game's row in its SQLite database:
`plays (game TEXT PRIMARY KEY, n INTEGER) WITHOUT ROWID`, one row a game, so a counted play writes one row. The number playing a
game now is the number of open sockets with its tag. The object sets no timer and no alarm, so between plays it hibernates
while the sockets stay open.

The listing page asks `/api/counts` for every game's counts in one request (`ByopopCounter.counts`) when it is shown, and again
every 2 minutes while it stays shown. The answer is `{"games": {"<id>": {"plays": n, "live": n}}}` for each game that has a play
or a page playing it. The Worker caches it at the edge for 30 seconds (the Cache API on the site's domain), so a busy listing
page wakes the object at most about twice a minute in each Cloudflare location. Under each game's name the listing shows
"2 playing now · 1234 plays", or the plays alone when nobody is playing it, and "Most popular" joins the sorts: by lifetime plays,
then by the number playing now, then by name. Until the counts arrive, and always without a counter, neither shows, and a saved
"Most popular" choice sorts by name.

## Off by default

The counter's address is a build option, `BYOPOP_WEB_COUNTER` in `cmake/ByopopWeb.cmake`: a path on the site's own domain
(`/api`) or an https address, empty by default. The build writes it into `playcount.js`. With it empty, which is every local build
and every test, the pages contact nothing and show no counts and no "Most popular". Even with an address, a page contacts nothing
with `?silent=1` (the tests), under Node or anywhere without `WebSocket`, in a browser driven by automation
(`navigator.webdriver`), or while it is hidden: the first contact waits until the page is shown, so a page in the hidden Browser
pane never makes one.

## What it keeps, and what it does not

The project's stance, the user's, is that the site holds as little about its players as it can. The counter stores a game id and
two numbers for it, and nothing else. It stores no IP address, cookie, account, identifier, user agent or time of a play, and the
requests carry nothing about the player: the socket's address holds the game id, and `count=0` when this browser counted a play
of that game in the last five minutes.

To limit how many sockets one connection source holds, the Worker passes the visitor's address (`CF-Connecting-IP`) to the object,
which hashes it with SHA-256 and a random salt of 16 bytes that lives only in its memory and is made new each time the object
wakes, and tags the socket with the first 8 bytes of the hash. The address itself is never stored or tagged, the hash cannot be
linked to the address without the salt, and a tag goes away with its socket. The object also remembers, in memory only, which hashed
source last had a play of a game counted in the past five minutes. Cloudflare sees each request as it does for any site.

In the browser, `localStorage` holds `byopop-counted`: for each game whose play this browser counted in the last five minutes,
the time it did. Older entries are dropped whenever it is written.

## What the counts can be trusted for

Without accounts no count of plays can be made cheat-proof, so the counter limits casual inflation where it is cheap. One browser
counts one play of a game in five minutes (its `byopop-counted`). One connection source counts one play of a game in five minutes
(the object's memory). One source can hold at most 8 open sockets, so it can add at most 8 to a game's "playing now" (a
household or an office behind one address shares that). A frame larger than 64 bytes closes the socket with 1009. A game id
outside the site's table is refused, and so is a socket from another site's page.

So the counts are good for what the listing uses them for: which games people play on the site, roughly how much, and how many
are playing now. They are not exact. A play is counted after 30 seconds of play, not at the click. A player who reloads a game
within five minutes is counted once. Several players behind one address who start the same game within five minutes count once
between them. Someone who sets out to inflate a count can do it by writing their own client and using many addresses. The memory
limits are also forgotten when the object hibernates and wakes, which only loosens them for a while. If inflation becomes a
problem, Cloudflare's rate-limiting binding on the Worker's `/api/play` route, or Turnstile before the socket, are the next steps.

## The cost

By Cloudflare's pages as `docs/CHAT_DESIGN.md` quotes them (2026-10-31), the Free plan allows 100,000 Worker requests a day,
100,000 Durable Object requests, 13,000 GB-s of object duration and 100,000 SQLite rows written. Static assets are free, so the
pages themselves cost nothing. A WebSocket connection is a request. An incoming message is a twentieth of an object request.
The state API's page says the auto-response avoids waking the object and its duration charge, but not whether the message
still counts, so the figures below count every ping as a twentieth.

A counted play costs one Worker request, one object request and one row written. An hour of play adds 72 pings, 3.6 object requests at
most. A listing visit costs one Worker request when it is shown and one more every 2 minutes while it stays shown; the
object sees at most two in a minute in each location, whatever the visits, because of the 30-second cache. Duration is small:
the object wakes for a moment at each play and each counts read, and sleeps between.

For example, a day of 5,000 plays of half an hour each and 10,000 listing visits of 4 minutes each comes to about 35,000 Worker
requests (5,000 plays and 30,000 counts reads), about 14,000 object requests (5,000 plays, 9,000 for the pings, and a few
thousand counts reads that miss the cache) and 5,000 rows written. The Worker requests are the first limit to bite, at about
100,000 a day: roughly 30,000 listing visits of a few minutes a day, or fewer if visitors leave the listing open for long. Object requests come next for long plays: about
20,000 hour-long plays a day. Rows written would take 100,000 counted plays a day. Past these, the Workers Paid plan at $5 a
month includes 10 million Worker requests and 1 million object requests a month, with 50 million SQLite rows written a month,
which is far more than this site will see. The plan to be on is the $5 one from the day the free daily limits start
to refuse requests. Raising the listing's interval, or reading the counts only when the page is first shown, would push the
first limit back if it is near.

## Deploying it

Not now: nothing is deployed until the user decides to put the site up. When it is time, every web build already
writes the counter's folder beside the pages', `build/counter/` (for the build folder `build/web`): `playcount.mjs` and a
`wrangler.toml` with the site's game ids in `GAMES` (from `web/counter/wrangler.toml.in`). To deploy it, with the user's own
Cloudflare account and Wrangler:

1. In `build/counter/wrangler.toml`, set `ORIGIN` to the site's origin (`https://example.org`) and uncomment `routes` with the
   site's domain, so the Worker answers `/api/*` on it.
2. `npx wrangler deploy` in `build/counter/`. The first deploy creates the Durable Object class from the `v1` migration
   (SQLite-backed, which the Free plan needs).
3. Configure the web build with `-DBYOPOP_WEB_COUNTER=/api`, build it, and publish `build/web` as the site's static assets on the
   same domain.
4. Open the listing page and a game page with the browser's network panel: one `counts` request, and after 30 seconds of play
   one `play` socket. A page with `?silent=1` makes neither.

The compatibility date in `wrangler.toml` is 2026-11-01, past the 2026-04-07 date from which the runtime answers a WebSocket's
close frame by itself.

## Testing

`tools/check.sh unit-web` runs two tests of it under Node. `tools/playcounttest.mjs` runs the Worker and the object against small
stand-ins for Cloudflare's runtime: the object's SQLite database through Node's own `node:sqlite`, its sockets, tags, attachments
and auto-response, `WebSocketPair` and the 101 response. It checks the counting and the live count, the five-minute limit, `count=0`,
the cap of sockets for one source, what the Worker refuses, the frame size, the counts' answer and cache header, and that
nothing but game ids and counts is stored and no address is kept. `tools/playcountpagetest.js` runs `web/playcount.js` in a
stand-in browser: that it contacts nothing without an address, with `?silent=1`, under automation or without `WebSocket`, and
nothing while hidden; what a game page's socket asks for and when; and what the listing takes from the counts. Wrangler's local
runtime (workerd, via Miniflare) would run the real thing, but it is a download, which needs the user's OK first; it has not
been used.
