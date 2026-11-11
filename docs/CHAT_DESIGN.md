# The chat

The site has one chat room for every game, beside the game on its page. It is plain text and IRC style, with no pictures, no
embeds, no markup, no reactions and no links in what people write (the one link is the tag of the game a message was sent from,
made by the site and not by the sender), because anything richer needs storage for other people's files and the site does not want
to hold much of its players' data (the user, 2026-10-30). Today the room is a test: its messages are made up
and live in the visitor's browser, and the page makes no network request for it. This file says how the panel and its source
work, and what a server built only on Cloudflare could look like when the website is launched.

## The panel and its source

`web/chat.js` draws the panel into the `<aside id="chat">` of `web/shell.html`; its header documents the layout, the
message fields, the keyboard and screen-reader behaviour, and the test data of the local source, and `tools/chattest.js`
checks them. What the code does not say is the reasoning below.

The site-wide setting **Show chat** (`showChat` in `web/settings.js`) is on until a player turns it off, and with it off nothing
is drawn or opened, which is also the way for a player to have no chat connection at all. The chat never changes the size of the
canvas: it goes beside the stage only when the window has room left after the stage at the width it has without the chat, and
below it otherwise, where the page's scrollbar is hidden because a classic scrollbar would make the stage narrower than it is
without the chat. A message goes in with `textContent` and text nodes only, so markup, `javascript:` and URLs all stay text. Text
is one line of at most 500 characters, a send is limited to one a second (in the panel and in the source, as a server would do
its own), and the characters that hide or reorder text become spaces.

**The game tag.** The `[BW]` before a username is the initials of the game the speaker is playing, so that a room for every game
is not a Tower of Babel (the user's wish). It is the one link in the chat: it goes to the game's page, and its name for a screen
reader is the title and the initials, "Bookworm Deluxe (BW)", because a link's name has to hold the words it shows. The panel
keeps no list of games. `cmake/ByopopWeb.cmake` writes `chat-games.js` beside the pages (`byopop_web_index()`), from the same
`BYOPOP_WEB_BADGES` and the same titles as the listing page's tiles: `var ByopopGames = [{id, page, tag, title, colour}, ...]`,
with `id` the page's own game id (`PAGE.id`). Every game page loads it before `chat.js` and hands it to `mount()` with its own
game id, once the game has started (the user, 2026-11-03: "Don't load the chat until the game has launched": a page waiting for
its files or for Play shows none). The panel draws every tag from that table by the message's game id and from nothing else: the
page name has to be a file name beside the page's own, and the colour has to be `#rrggbb`. What a message says about its own tag
or title is never drawn, so a stored or received message cannot make up a tag or a link; a message from a game the table does
not know shows no tag and no link. A message records the game it was sent from as three fields: `game` (the page's id), and `tag`
and `gameTitle` (the initials and the name as they were when it was sent, for whoever reads the records; a source puts them on
from its table). Fields that are not well formed are dropped from a stored message and the message is kept.

A username follows Discord's current rules: 2 to 32 characters, lowercase Latin letters, digits, underscore and period, and
never two periods in a row (a period may begin or end a name), as Discord's help article "New Usernames & Display Names"
has them. A chat name is the person's Discord username exactly as Discord gives it: the site never lowercases or otherwise
changes it, and a profile's name never feeds it (the user, 2026-11-11: "going forward let's just have chat names match people's
Discord usernames exactly so we don't have to worry about that"). The test name typed in the panel is only trimmed, and one with
capitals is refused by the rules above rather than lowered. The real names will come from Discord, which also reserves a few more. There is no sign-in yet: the panel shows a "Sign in with Discord" button that is
`aria-disabled` and says "Not available yet". Meanwhile the sender is a test username (`test_player` until changed) that the
viewer can change in the panel, kept in `byopop-chat-test-name`.

Everything the panel knows about the room goes through a source (`connect`, `history`, `send`, `onMessage`, `onError`,
`setUsername`, `close`, documented in the header of `web/chat.js`), so a server can replace the local one without the panel
changing.

Ids sort in time order, which is all `history` needs from them. The panel shows the newest 50 messages on load and loads 50
more when the viewer scrolls up or presses "Show older messages". The local source, `localSource()`, is test data from top to
bottom and marked so in the code; what the viewer sends and what it "receives" is stored in `byopop-chat-test-messages` (newest
300, checked again as they come back in because the stored item is the viewer's to edit), shared between the site's pages
through the `storage` event, so a message sent in one game's tab shows in another's: one room for every game.

## A backend that is only Cloudflare

Everything here is from Cloudflare's own documentation at developers.cloudflare.com, read on 2026-10-31, and from Discord's. No
account, tool or connector of Cloudflare's was used, and nothing was created.

The room is one Durable Object, addressed by a fixed name (`idFromName("all-games")`), in front of which a Worker on the site's own
domain (so cookies are first-party and nothing needs CORS) checks who is asking. A Durable Object is a Worker with private,
strongly consistent storage; one instance exists for a name worldwide and it is single-threaded, which the documentation gives a
chat room as its example. The Worker forwards the WebSocket upgrade to it, and the object accepts the socket with
`ctx.acceptWebSocket(server)`, the Hibernation API, rather than `ws.accept()`. With it the object can be evicted from memory while
the clients stay connected, nothing is billed for duration while it is idle, and it is woken by the next message, with the
constructor run again. So the constructor reloads what it needs, and what belongs to a connection (a user's id and name) is kept
with `serializeAttachment`, which survives hibernation and holds up to 16,384 bytes. Messages arrive in `webSocketMessage` and are
sent to every socket from `ctx.getWebSockets()`. Alarms, timers and scheduled callbacks keep an object from hibernating while they
run, so the room uses no timer and no outgoing socket; the pruning alarm fires once a day, wakes the object for a moment and lets
it sleep again.

Recent history lives in the object's own SQLite database (`ctx.storage.sql.exec`, one database per object, up to 1 GB on
the Free plan and 10 GB on the Paid plan, with point-in-time recovery for 30 days): a `messages` table with an increasing id, the
user's Discord id, the username at the time, the text and the time. On connect a client asks for the newest 50 and then for older
ones with `before`, which is the `history` call above. D1 is the choice if longer history, or history that other parts of the site
read from outside the room, is wanted (free plan: databases of up to 500 MB, 5 million rows read and 100,000 written a day, 50
queries per Worker invocation, 7 days of Time Travel; paid: 10 GB, 30 days). For one room of short plain lines the object's own
database is enough, and a second store would only add a second write for every message.

Sign-in is Discord's OAuth2 authorization code grant with the `identify` scope alone, which lets `/users/@me` answer without the
email (the `email` scope is what adds it). The Worker sends the browser to `https://discord.com/oauth2/authorize` with the client
id, `response_type=code`, the registered `redirect_uri`, `scope=identify` and a random `state` that it also puts in a short-lived
cookie; Discord recommends `state`, and its page documents no PKCE parameters, so the client secret and `state` are what protect
the exchange. The callback checks `state`, posts the code to `https://discord.com/api/oauth2/token` (form-encoded, with the client
secret kept as a Worker secret), asks `/users/@me` for the `id` (a snowflake: it carries the account's creation time, which lets
the server refuse accounts younger than a day or two) and the `username`, and then revokes the token at
`/oauth2/token/revoke`, since the site never calls Discord again. The session is a signed cookie, `__Host-byopop-session`,
`HttpOnly; Secure; SameSite=Lax; Path=/`, holding the id, the name and an expiry (30 days), signed with HMAC-SHA-256 from Web
Crypto and a secret: nothing is stored for it, and a ban is checked by id at every connection and message. The WebSocket upgrade
carries the cookie; the Worker verifies it and the `Origin` header and passes the verified id and name to the object. Without a
valid cookie a visitor can have a read-only socket, or none, and a plain request for the newest 50 messages instead keeps a lurker
from holding a connection.

The rate limits that count are in the object. It is single-threaded, so its counters are exact: one message a second and about
twenty a minute for each user id, at most three sockets for each user, a text of at most 500 characters and a frame of at most a
couple of kilobytes checked before it is parsed, the username checked against the rules above, the game id checked against the
site's table of games (the Worker bundles a copy of `chat-games.js`; it takes the initials and the title from it, never from the
client, and stores an id that is not in the table as none), and a socket closed with code 1008 on abuse. Cloudflare's
rate-limiting binding is the wrong tool for this: its limits are per Cloudflare location, "approximate" and "eventually
consistent", with a period of only 10 or 60 seconds, and the documentation itself recommends a Durable Object for a strict limit.
It still suits the Worker's own unauthenticated routes, such as the sign-in callback.

Moderation is by Discord id. A moderator is a Discord id in a Worker variable, or a row in a table of the object. A moderator's
`delete` message removes the row and sends every socket `deleted <id>`, and the panel drops the line; `mute <id> <until>` lets the
user read and not write; `ban <id>` refuses the upgrade and closes the user's sockets; and each action goes in a `mod_log` table.
Bans and mutes hold the id and an expiry, nothing else. A player can ask to delete their own messages, which is a delete of
every row with their id.

About each user the server holds: their messages (id, Discord user id, the username at the time, text, time and the id of the
game they were sent from, which is what the tag shows), kept for the newest 1,000 messages or 14 days, whichever is fewer, pruned
by the alarm once a day; a row for a mute or a ban while it lasts; and a session cookie that lives in the browser, not on the
server, for 30 days. It holds no email (the scope does not give it), no avatar, no token (revoked at once), no IP address (the code
never records one; Cloudflare handles the request as it does for any site, and Discord sees the visitor only when they sign in),
no user table, and nothing of the player's games beyond the id of the game a message was typed in: no files, saves, scores or
progress. When a user's last message ages out, nothing about them remains.

The cost at small scale, by Cloudflare's pages as they stood on 2026-10-31. The Workers Free plan allows 100,000 requests a day,
and for Durable Objects 100,000 requests and 13,000 GB-s of duration a day, with SQLite storage of 5 GB, 5 million rows read and
100,000 rows written a day; SQLite-backed objects work on the Free plan, and requests to static assets are free and unlimited. A
WebSocket connection counts as a request and an incoming message as one twentieth of a request; messages sent out to clients and
pings are free. Duration is billed at 128 MB whatever is used, so an object that never hibernates costs 10,800 GB-s a day, which
still fits the free 13,000. A day of 500 players who each connect twice and write ten lines is about 1,250 Durable Object
requests, 2,000 Worker requests and 5,000 rows written, a twentieth of the free allowances or less. Deletes count as rows
written, so a message costs two when pruning removes it, and the free 100,000 rows written a day is reached at about 50,000
messages a day. Past that the Workers Paid plan is $5 a month (10 million Worker requests, 1 million object requests, 400,000
GB-s, 25 billion rows read and 50 million written, 5 GB-months of storage, with overage rates on the pricing pages). At this scale
the chat is free, and the plan to be on is the $5 one the day the Free daily limits start to bite. An object's soft limit of
1,000 requests a second is far above one room's traffic; if it ever mattered the room would split by game.

## What else Cloudflare alone could carry

The same Worker, with D1 where a table is better than a room's own database, can carry leaderboards, accounts and a page where
players vote on mod ideas, and still hold little. The user's line of 2026-10-27 governs all of it: an account system must never
store players' game files on the server, and the site should not store much user data at all. Saves, mods and game files stay
in the browser, and the zip a player exports is theirs.

An account is only the Discord id from the sign-in above, as the key of whatever a feature keeps, and nothing else: no password,
no email, no profile. A leaderboard is a D1 table of the best score for each game, id and name, kept to the top hundred or so of
each board. Without a replay the server cannot check a score, and a replay is exactly the kind of file the site will not hold,
so the board would say "reported by the player" and a moderator could remove entries; if cheating becomes a problem, that is the
point to decide again. A vote is a row (idea, Discord id) with the pair as primary key, so a player has one vote for an idea; the
ideas are plain text rows of a title and a few hundred characters, posted behind the same rate limits and moderated by the same
moderators; the tally is a `COUNT`. All of it fits in the free allowances above until the site is busy, and the chat's Worker and
its secrets would be shared, so a new feature is a new route and a new table, not a new service.

## Sources

Cloudflare, read on 2026-10-31: Durable Objects (what they are; pricing; limits; WebSocket best practices and the Hibernation API;
the SQLite storage API; alarms), Workers pricing, D1 pricing and limits, and the Workers Rate Limiting binding, all under
developers.cloudflare.com. Discord: the OAuth2 topic and the User resource of the developer documentation (docs.discord.com),
the blog post "Evolving Usernames on Discord", and the help article "New Usernames & Display Names" on support.discord.com.
