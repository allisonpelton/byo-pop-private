# The chat

The site has one chat room for every game, beside the game on its page. It is plain text and IRC style, with no pictures, no
embeds, no markup, no reactions and no clickable links, because anything richer needs storage for other people's files and the
site does not want to hold much of its players' data (the user, 2026-10-30). Today the room is a test: its messages are made up
and live in the visitor's browser, and the page makes no network request for it. This file says how the panel and its source
work, and what a server built only on Cloudflare could look like when the website is launched.

## The panel and its source

`web/chat.js` draws the panel into the `<aside id="chat">` of `web/shell.html`, the template of every game page, and the shell
loads it next to `settings.js`. The panel is built only when it is shown. The site-wide setting **Show chat** (`showChat` in
`web/settings.js`) is on until a player turns it off, and with it off nothing is drawn or opened, which is also the way for a
player to have no chat connection at all. Its heading is the button that collapses it, and the collapse is remembered per
browser in localStorage (`byopop-chat-collapsed`); every storage access is in a try/catch.

The chat never changes the size of the canvas. The stage is as wide as the smaller of its box and `(100vh - 90px)` times the
game's shape, so putting a column beside it would shrink it whenever the window is not wide enough for both. `layout()` in
`chat.js` therefore does the arithmetic first: the chat goes beside the stage, 240 to 340 pixels wide, only when the window has
that much room left after the stage at the width it has without the chat, and goes below the stage otherwise, as on a phone
upright or when a widescreen game fills the width. Below the stage the page scrolls to it, and the page's scrollbar is hidden
there (the wheel, touch, the keyboard and Tab still scroll), because a classic scrollbar takes 15 pixels of the page's width and
would make a stage that is as wide as the page narrower than it is without the chat. `Module.onGameScreen` calls `layout()` when a
game changes the stage's shape. Checked in the Browser pane at 1920 by 1080, 1366 by 768, 1280 by 800, 1024 by 768, 812 by 375,
768 by 1024 and 360 by 640, and at the last with a widescreen game's shape: the stage has the same size with the chat as with
the chat hidden.

A message is `{id, t, user, text}` and a line reads `HH:MM <username> text`, the time in the viewer's local time with the full
date and time (`2026-10-30 09:05:07 UTC-6`) as the hover text of the time, and a day separator between days. A message goes in
with `textContent` and text nodes only: markup, `javascript:` and URLs all stay text, and `tools/chattest.js` checks the rows
against a DOM that throws on `innerHTML`, and the file for any way of writing markup or reaching the network. Text is one line
of at most 500 characters: a newline or tab, the characters that hide or reorder text (zero width, bidirectional overrides) and
the Unicode line breaks become spaces, and Enter sends. A send is limited to one a second, once in the panel and once in the
source, as a server would do its own.

A username follows Discord's current rules: 2 to 32 characters, lowercase Latin letters, digits, underscore and period, and
never two periods in a row (a period may begin or end a name). Discord's help article "New Usernames & Display Names" says so,
and the blog post that announced them and the developer documentation of the User object agree on the characters and the
length; the help page itself refused an automated fetch, so its text was read through a search summary. Discord lowercases what
is typed, so the panel does too before it checks, and the real names will come from Discord, which also reserves a few more.
There is no sign-in yet: the panel shows a "Sign in with Discord" button that is `aria-disabled` and says "Not available yet".
Meanwhile the sender is a test username (`test_player` until changed) that the viewer can change in the panel, checked by those
rules and kept in `byopop-chat-test-name`.

Everything the panel knows about the room goes through a source, so a server can replace the local one without the panel
changing:

    connect()                 Promise of {user: {name, signedIn}, room, note, canRename, testData}
    history(limit, before)    Promise of the newest `limit` messages (before: null), or the `limit` before the message
                              with id `before`; oldest first; fewer than `limit` means that is the start
    send(text)                Promise of the stored message, or an Error with .code: empty, long, rate (and .waitMs), name, offline
    onMessage(fn)             a message from anyone, the sender's own included, as a server echoes it; returns a way to stop
    onError(fn)               {code, message, fatal}; returns a way to stop
    setUsername(name)         only a source with canRename: the local one (a server takes the name from the account)
    close()

Ids sort in time order, which is all `history` needs from them. The panel shows the newest 50 messages on load, loads 50 more
when the viewer scrolls up or presses "Show older messages" (the log keeps what was on screen where it was, and does not read
the older ones out), says "This is the start of the chat" at the end, and follows new messages only while the viewer is at the
bottom, else offering "New messages below". A collapsed panel counts what came in.

The local source, `localSource()`, is test data from top to bottom and marked so in the code. It makes 34 messages for each of
the four days before today and the part of today's that is past, from twelve invented usernames and 48 lines of talk about playing
in general terms (none quotes a game); the same ones every load, made from a fixed random sequence and never stored. The
messages the viewer sends, and the ones the source "receives" now and then (one every 20 to 60 seconds, from the invented users),
are stored in `byopop-chat-test-messages`, newest 300, checked again as they come back in because the stored item is the
viewer's to edit. Two pages of the site share that item through the `storage` event, so a message sent in one game's tab shows in
another's: one room for every game. With storage blocked the messages last until the page closes, and the panel says so.

For the keyboard and for screen readers the message list is a `role="log"` live region (polite, additions only, silent while
the first screenful or older messages go in), the field has a label, the panel is a named `<aside>`, and it comes after the stage
in the page's order, so Tab and F6 from the canvas reach it. F6 and Shift+F6 typed in the chat move the focus on, as they do from
the game, to the control after the chat or the one before it. The game takes keys only while its canvas has the focus (the
shell's `SDL_EMSCRIPTEN_KEYBOARD_ELEMENT`, and the window guard behind it), and the chat is not inside the canvas, so what is
typed in it does not reach the game; in the Browser pane a key listener on the canvas and one on the window, added after the
shell's, saw nothing typed in the chat and saw a key typed on the canvas. The colours are the shell's tokens, and the colour of a
username is its hue (a hash of the name) at a lightness that passes 4.5 to 1 against the page in both themes.

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
sent to every socket from `ctx.getWebSockets()`. The documentation says that alarms, timers and scheduled callbacks keep an object from hibernating while they run, so the room
uses no timer and no outgoing socket; the pruning alarm fires once a day, wakes the object for a moment and lets it sleep again.

Recent history lives in the object's own SQLite database (`ctx.storage.sql.exec`, one database per object, up to 1 GB on
the Free plan and 10 GB on the Paid plan, with point-in-time recovery for 30 days): a `messages` table with an increasing id, the user's Discord id, the username at
the time, the text and the time. On connect a client asks for the newest 50 and then for older ones with `before`, which is the
`history` call above. D1 is the choice if longer history, or history that other parts of the site read from outside the room,
is wanted: its free plan has databases of up to 500 MB (10 GB on the paid plan), 5 million rows read and 100,000 written a day,
and 50 queries per Worker invocation, 7 days of Time Travel (30 on the paid plan). For one room of short plain lines the object's
own database is enough, and a second store would only add a second write for every message.

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
couple of kilobytes checked before it is parsed, the username checked against the rules above, and a socket closed with code 1008
on abuse. Cloudflare's rate-limiting binding is the wrong tool for this: its limits are per Cloudflare location, "approximate" and
"eventually consistent", with a period of only 10 or 60 seconds, and the documentation itself recommends a Durable Object for a
strict limit. It still suits the Worker's own unauthenticated routes, such as the sign-in callback.

Moderation is by Discord id. A moderator is a Discord id in a Worker variable, or a row in a table of the object. A moderator's
`delete` message removes the row and sends every socket `deleted <id>`, and the panel drops the line; `mute <id> <until>` lets the
user read and not write; `ban <id>` refuses the upgrade and closes the user's sockets; and each action goes in a `mod_log` table.
Bans and mutes hold the id and an expiry, nothing else. A player can ask to delete their own messages, which is a delete of
every row with their id.

About each user the server holds: their messages (id, Discord user id, the username at the time, text and time), kept for the
newest 1,000 messages or 14 days, whichever is fewer, pruned by the alarm once a day; a row for a mute or a ban while it lasts; and
a session cookie that lives in the browser, not on the server, for 30 days. It holds no email (the scope does not give it), no
avatar, no token (revoked at once), no IP address (the code never records one; Cloudflare handles the request as it does for any
site, and Discord sees the visitor only when they sign in), no user table, and nothing of the player's games. When a user's last
message ages out, nothing about them remains.

The cost at small scale, by Cloudflare's pages as they stood on 2026-10-31 (the Durable Objects pricing page says it was last updated on 18
October 2026). The Workers Free plan allows 100,000 requests a day, and for Durable Objects 100,000 requests and
13,000 GB-s of duration a day, with SQLite storage of 5 GB, 5 million rows read and 100,000 rows written a day. SQLite-backed objects,
which is what is wanted here, work on the Free plan. Requests to static assets are free and unlimited, so the pages themselves cost
nothing to serve. A WebSocket connection counts as a request and an incoming message as one twentieth of a request (100 messages
are 5 requests); messages sent out to clients and pings are free. Duration is billed at 128 MB whatever is used, so an object that
never hibernates costs 86,400 s times 0.125 GB, 10,800 GB-s a day, which still fits the free 13,000; a month of it, 324,000 GB-s,
fits the 400,000 that the paid plan includes. A day of 500 players who each connect twice and write ten lines is 1,000 connections
and 5,000 messages: about 1,250 Durable Object requests, 2,000 Worker requests and 5,000 rows written, a twentieth of the free
allowances or less. Deletes count as rows written, so a message costs two when pruning removes it, and the free 100,000 rows
written a day is reached at about 50,000 messages a day, before index entries; the free requests would be reached at about a
hundred thousand connections. Past that the Workers Paid plan is $5 a month, with 10 million Worker requests (then $0.30 a million),
1 million object requests, which is 20 million incoming messages (then $0.15 a million), 400,000 GB-s (then $12.50 a million), and for
SQLite 25 billion rows read and 50 million written a month (then $0.001 and $1.00 a million) and 5 GB-months of storage (then
$0.20 a GB-month). At this scale the chat is free, and the plan to be on is the $5 one the day the Free daily limits start to
bite. The documentation's own WebSocket example with the Hibernation API, 100 objects with 100 sockets each, comes to $20.65 a
month (clients that send a message a minute), a hundred rooms and ten thousand open sockets, a size this site will not have. The
limits page gives an object a soft limit of 1,000 requests a second, which is far above one room's traffic; if it ever mattered the room would split by game.

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
moderators; the tally is a `COUNT`, and the Free plan's 5 million rows read a day is far more than a page of ideas needs. All of
it fits in the free allowances above until the site is busy, and the chat's Worker and its secrets would be shared, so a new feature
is a new route and a new table, not a new service.

## Sources

Cloudflare, read on 2026-10-31: Durable Objects (what they are; pricing; limits; WebSocket best practices and the Hibernation API;
the SQLite storage API; alarms), Workers pricing, D1 pricing and limits, and the Workers Rate Limiting binding, all under
developers.cloudflare.com. Discord: the OAuth2 topic and the User resource of the developer documentation (docs.discord.com),
the blog post "Evolving Usernames on Discord", and the help article "New Usernames & Display Names" on support.discord.com.
