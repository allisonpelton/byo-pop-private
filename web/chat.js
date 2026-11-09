// The site's chat: one plain-text room for every game, beside the game's stage (docs/CHAT_DESIGN.md). web/shell.html loads this
// file and calls ByopopChat.mount() with its <aside id="chat">; the file has three parts:
//
//   1. the rules, which a server would enforce too: a username as Discord's current rules have it (2 to 32 characters, lowercase
//      letters, digits, "_" and ".", never two periods in a row), the text of a message (one line, 500 characters), the rate
//      limit (one message a second), and how a line, its time and its day are written. They are plain functions, tested under Node
//      (tools/chattest.js).
//   2. the source: where the messages come from. The panel talks only to this interface, so that a server can replace the local
//      one without the panel changing:
//        source.connect()           -> Promise of { user: {name, signedIn}, room, note, canRename, testData }: opens the room.
//                                      user.signedIn is false until a sign-in exists (then the panel offers none).
//        source.history(limit, before) -> Promise of an array of messages, oldest first: the newest `limit` messages (before:
//                                      null) or the `limit` messages just before the message whose id is `before`. Fewer than
//                                      `limit` back means that is the start of the chat.
//        source.send(text)          -> Promise of the stored message, or a rejection: an Error whose .code is "empty", "long",
//                                      "rate" (and .waitMs), "name" or "offline". The text is plain; the source checks it again.
//        source.onMessage(fn)       -> a function that stops it: fn(message) for each new message of anyone, the sender's own
//                                      included (a server echoes it, so the panel has one way in for every message).
//        source.onError(fn)         -> a function that stops it: fn({code, message, fatal}) when the room cannot be reached.
//        source.setUsername(name)   -> Promise of the name kept; only a source with canRename (the local one: a server takes the
//                                      name from the account).
//        source.close()
//      A message is { id, t, user, text } and, when the sender's page is a game's, { game, tag, gameTitle }: id a string that sorts
//      in time order, t milliseconds since 1970, user a username, text one line of plain text, game the id of the game the sender was
//      playing (a page's id: PAGE.id in web/shell.html), tag that game's initials and gameTitle its name as they were when it was sent.
//      The panel draws the tag from the site's own table of games (build/web/chat-games.js, below) by the game id, so what a message
//      says about its tag or title is never drawn, and a game the table does not know has no tag. The only source today is
//      localSource(), made up and kept in this browser; it makes no network request.
//   3. the panel: the log (a role="log" live region), the field to write in, the username control and the disabled "Sign in with
//      Discord". Everything a message says goes in with textContent, never as markup, and a URL stays text.
//
// The table of games is made by cmake/ByopopWeb.cmake (byopop_web_index) from BYOPOP_WEB_BADGES and the titles the listing page
// shows, and written to chat-games.js beside the pages: var ByopopGames = [{ id, page, tag, title, colour }, ...]. shell.html passes
// it to mount() with the page's own game id; this file keeps no list of games.
//
// Plain ES5, like the other shared page files. Every localStorage read and write is guarded. The three keys, all this file's:
//   byopop-chat-collapsed       "1" while the viewer has the panel collapsed (remembered per browser)
//   byopop-chat-test-name       the viewer's test username (local source only)
//   byopop-chat-test-messages   the local source's messages sent or received, as JSON, at most LIMITS.keep of them
var ByopopChat = (function () {
  "use strict";

  var LIMITS = { nameMin: 2, nameMax: 32, textMax: 500, rateMs: 1000, keep: 300, page: 50 };
  var KEY_COLLAPSED = "byopop-chat-collapsed", KEY_NAME = "byopop-chat-test-name", KEY_LIVE = "byopop-chat-test-messages";
  var DEFAULT_NAME = "test_player";
  // what a game's entry in the table, and a message's game fields, may hold (the table is the site's own and these only guard it)
  var GAME_ID = /^[a-z0-9][a-z0-9_-]{0,39}$/, GAME_PAGE = /^[a-z0-9][a-z0-9_-]{0,39}\.html$/, GAME_TAG = /^[A-Za-z0-9]{1,4}$/;
  var GAME_COLOUR = /^#[0-9a-fA-F]{6}$/, GAME_COLOUR_DEFAULT = "#6b3fd1", GAME_TITLE_MAX = 100;
  var CHAT_MIN = 240, CHAT_MAX = 340;  // the chat's width beside the stage, in pixels

  // ---------- the rules ----------
  function storage() { try { return window.localStorage || null; } catch (e) { return null; } }

  // What the viewer typed as a username, made ready to check: Discord ignores case and lowercases what is typed
  function cleanUsername(raw) { return String(raw === undefined || raw === null ? "" : raw).trim().toLowerCase(); }
  // "" when the name is a valid username, else the reason. Discord's current rules (support.discord.com, "New Usernames &
  // Display Names"): 2 to 32 characters; lowercase Latin letters, digits, underscore and period; no two periods in a row
  // (a period may start or end a name). Discord bars a few more names of its own; the real names will come from Discord.
  function usernameError(name) {
    if (typeof name !== "string" || name.length < LIMITS.nameMin || name.length > LIMITS.nameMax) {
      return ByopopStrings.t("chat.name.length", {min: LIMITS.nameMin, max: LIMITS.nameMax});
    }
    if (/[^a-z0-9_.]/.test(name)) return ByopopStrings.t("chat.name.chars");
    if (name.indexOf("..") >= 0) return ByopopStrings.t("chat.name.periods");
    return "";
  }
  // A message's text as it is kept: one line. Control characters (a newline, a tab), the characters that hide or reorder text
  // (zero width, bidirectional overrides) and the Unicode line breaks become spaces, runs of spaces become one, and the ends are
  // trimmed. Nothing else changes: a URL, a "<b>" and an emoji stay as typed.
  function cleanText(raw) {
    return String(raw === undefined || raw === null ? "" : raw)
      .replace(/[\u0000-\u001f\u007f-\u009f\u200b\u200c\u200e\u200f\u2028\u2029\u202a-\u202e\u2060-\u206f\ufeff]/g, " ")
      .replace(/ {2,}/g, " ")
      .replace(/^ | $/g, "");
  }
  // null when the (cleaned) text can be sent, else "empty" or "long"
  function textProblem(text) {
    if (typeof text !== "string" || text === "") return "empty";
    return text.length > LIMITS.textMax ? "long" : null;
  }
  // The rate limit: take(now) is 0 when a message may go (and counts it), else the milliseconds to wait. The panel keeps one and
  // the source keeps one, as a server would keep its own.
  function createRateLimiter(minMs) {
    var last = -Infinity;
    function wait(now) {
      var w = last + minMs - now;
      return w > 0 ? Math.min(Math.ceil(w), minMs) : 0;  // a clock set back never waits longer than the interval
    }
    return {
      wait: wait,
      take: function (now) { var w = wait(now); if (w === 0) last = now; return w; },
      reset: function () { last = -Infinity; }
    };
  }

  function pad(n, width) { var s = String(n); while (s.length < (width || 2)) s = "0" + s; return s; }
  // The viewer's local time, "HH:MM"
  function formatTime(t) { var d = new Date(t); return pad(d.getHours()) + ":" + pad(d.getMinutes()); }
  // "UTC", "UTC+2", "UTC+5:30", "UTC-3" from Date.getTimezoneOffset() (minutes west of UTC)
  function formatOffset(minutesWest) {
    var east = -minutesWest;
    if (!east) return "UTC";
    var a = Math.abs(east), h = Math.floor(a / 60), m = a % 60;
    return "UTC" + (east > 0 ? "+" : "-") + h + (m ? ":" + pad(m) : "");
  }
  // The local calendar day, "YYYY-MM-DD"
  function dayKey(t) { var d = new Date(t); return pad(d.getFullYear(), 4) + "-" + pad(d.getMonth() + 1) + "-" + pad(d.getDate()); }
  // The full date and time of a message, for the hover: "2026-10-30 09:05:07 UTC+2"
  function formatFull(t) {
    var d = new Date(t);
    return dayKey(t) + " " + pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds()) + " " + formatOffset(d.getTimezoneOffset());
  }
  // The day separator's words: "Friday 30 October 2026", with "Today, " or "Yesterday, " before it
  function dayLabel(t, now) {
    var d = new Date(t), key = dayKey(t), n = new Date(now);
    var text = ByopopStrings.t("chat.day.date", {weekday: ByopopStrings.t("chat.weekdays").split(", ")[d.getDay()], day: d.getDate(),
      month: ByopopStrings.t("chat.months").split(", ")[d.getMonth()], year: d.getFullYear()});
    if (key === dayKey(now)) return ByopopStrings.t("chat.day.today", {date: text});
    if (key === dayKey(new Date(n.getFullYear(), n.getMonth(), n.getDate() - 1).getTime())) return ByopopStrings.t("chat.day.yesterday", {date: text});
    return text;
  }
  // A message as an IRC line: "09:05 [BW] <alice> hello", with the tag of the game it was sent from when the site's table (a
  // createGames() result) knows the game, else "09:05 <alice> hello"
  function formatLine(m, games) {
    var g = gameOf(games, m);
    return formatTime(m.t) + " " + (g ? "[" + g.tag + "] " : "") + "<" + m.user + "> " + m.text;
  }

  // ---------- the games ----------
  // The games the site knows, from its table: [{ id, page, tag, title, colour }] (page a file name beside this page's, tag the
  // initials of the listing page's badge, colour its #rrggbb, which keeps white text above 4.5 to 1). `here` is the id of the
  // page's own game. An entry that is not well formed is left out, so that what the table says is safe to put on the page. get(id)
  // is the entry or null; a name like "constructor" is not an entry. iconOf(id), when given, is the game's own icon for a game the
  // player has (its files loaded in this browser: web/settings.js, ByopopGameFiles), a PNG data URL, or ""; icon(id) gives it
  // only when it is one.
  function createGames(list, here, iconOf) {
    var byId = Object.create(null), all = [];
    (Array.isArray(list) ? list : []).forEach(function (g) {
      if (!g || typeof g !== "object" || typeof g.id !== "string" || !GAME_ID.test(g.id) || byId[g.id]) return;
      if (typeof g.page !== "string" || !GAME_PAGE.test(g.page) || typeof g.tag !== "string" || !GAME_TAG.test(g.tag)) return;
      var title = typeof g.title === "string" ? cleanText(g.title) : "";
      if (!title || title.length > GAME_TITLE_MAX) return;
      var entry = { id: g.id, page: g.page, tag: g.tag, title: title, colour: typeof g.colour === "string" && GAME_COLOUR.test(g.colour) ? g.colour : GAME_COLOUR_DEFAULT };
      byId[g.id] = entry;
      all.push(entry);
    });
    return {
      // the game this page plays (a function: the desktop's game in front), whose tag is drawn as the reader's own
      get here() { var h = typeof here === "function" ? here() : here; return typeof h === "string" ? h : ""; },
      get: function (id) { return typeof id === "string" && byId[id] ? byId[id] : null; },
      all: function () { return all.slice(); },
      icon: function (id) {
        if (typeof iconOf !== "function" || typeof id !== "string" || !byId[id]) return "";
        var url = "";
        try { url = iconOf(id); } catch (e) {}
        return typeof url === "string" && url.length <= 16384 && /^data:image\/png;base64,[A-Za-z0-9+\/=]+$/.test(url) ? url : "";
      }
    };
  }
  // a table made already, or a list to make one from
  function asGames(x, here, iconOf) { return x && typeof x.get === "function" && typeof x.all === "function" ? x : createGames(x, here, iconOf); }
  // the table's entry for the game a message says it came from, or null: no table, no game on the message, or a game the table lacks
  function gameOf(games, m) { return games && m && typeof m.game === "string" ? games.get(m.game) : null; }
  // The game fields a source puts on a message sent from the game `id`: the id if it is one, and the table's initials and title
  // for it when the table knows it
  function gameFields(games, id) {
    if (typeof id !== "string" || !GAME_ID.test(id)) return {};
    var g = games ? games.get(id) : null;
    return g ? { game: id, tag: g.tag, gameTitle: g.title } : { game: id };
  }
  // The game fields of a stored or received message as they are kept: the id if it is a well formed one, the initials and the title
  // if they are. A message with bad ones is kept without them.
  function cleanGame(m) {
    var out = {};
    if (!m || typeof m.game !== "string" || !GAME_ID.test(m.game)) return out;
    out.game = m.game;
    if (typeof m.tag === "string" && GAME_TAG.test(m.tag)) out.tag = m.tag;
    if (typeof m.gameTitle === "string") {
      var title = cleanText(m.gameTitle);
      if (title && title.length <= GAME_TITLE_MAX) out.gameTitle = title;
    }
    return out;
  }
  function addFields(to, from) { for (var k in from) if (Object.prototype.hasOwnProperty.call(from, k)) to[k] = from[k]; return to; }
  // The rows of a list of messages (oldest first): a { day: "YYYY-MM-DD", t } row before the first message of each day, and a
  // { msg } row for each message
  function withDays(list) {
    var rows = [], last = null;
    for (var i = 0; i < list.length; i++) {
      var k = dayKey(list[i].t);
      if (k !== last) { rows.push({ day: k, t: list[i].t }); last = k; }
      rows.push({ msg: list[i] });
    }
    return rows;
  }
  // A hue (0 to 359) that a username always has, for its colour
  function hueOf(name) {
    var h = 0;
    for (var i = 0; i < name.length; i++) h = (h * 31 + name.charCodeAt(i)) >>> 0;
    return h % 360;
  }
  function byTime(a, b) { return (a.t - b.t) || (a.id < b.id ? -1 : a.id > b.id ? 1 : 0); }
  // A page of a list sorted by byTime: the newest `limit` messages (before: null), or the `limit` just before the message whose
  // id is `before`; none when that id is not in the list. Oldest first.
  function pageOf(list, limit, before) {
    var end = list.length;
    if (before !== null && before !== undefined) {
      end = -1;
      for (var i = 0; i < list.length; i++) if (list[i].id === before) { end = i; break; }
      if (end < 0) return [];
    }
    return list.slice(Math.max(0, end - limit), end);
  }
  // Whether a stored message is well formed (the stored copy is the viewer's to edit, so it is checked on the way back in)
  function validMessage(m) {
    return !!m && typeof m === "object" && typeof m.id === "string" && m.id.length > 0 && m.id.length <= 64 &&
      typeof m.t === "number" && isFinite(m.t) && typeof m.user === "string" && usernameError(m.user) === "" &&
      typeof m.text === "string" && m.text.length > 0 && m.text.length <= LIMITS.textMax;
  }

  // ---------- the local source ----------
  // TEST DATA. Everything below down to the end of the source is made up for testing, until a server exists: the usernames are
  // invented, the lines are general talk about playing, and none is a quote of any game's text or a real person's. The seed
  // (some days of it) is made again at each load from a fixed random sequence, and is never stored; the messages the viewer sends
  // and the ones the source "receives" now and then are stored (byopop-chat-test-messages).
  var SEED_USERS = ["gemfan", "word.wizard", "mahjong_mel", "bug_hunter", "lowscore.larry", "nightowl_22", "tiles.and.tea",
    "ark_keeper", "pixel_pat", "zenmode", "rocket.rae", "corner_cutter"];
  var SEED_LINES = [
    "morning all, one quick round before work",
    "anyone else leave the music on low while they play?",
    "just loaded my files for the first time, worked on the first try",
    "I keep losing by one move, every single time",
    "the timed mode is stressful in a good way",
    "does the mod menu remember what you turned on?",
    "tried the zen option today, so relaxing",
    "my best score is nothing special but it is mine",
    "can someone explain the combo thing? I never get it",
    "new personal best, I am buying myself a coffee",
    "the old soundtrack hits different at midnight",
    "which of these do you play to wind down?",
    "I only play the word ones when nobody is watching",
    "played the same board for an hour, regret nothing",
    "fullscreen works fine for me on the laptop",
    "the page remembered my player name, nice touch",
    "took a break for a week and my hands forgot everything",
    "slow and steady beats fast and sloppy, learned that the hard way",
    "anyone tried it on a phone? curious about touch controls",
    "the tutorial level is still the best tutorial",
    "I always pick the same difficulty and never change it",
    "good game everyone",
    "just reached the last stage, wish me luck",
    "ok that was close",
    "the backgrounds are so pretty, I just stare sometimes",
    "lost my streak again, back to square one",
    "shout out to whoever made the save backup, it saved me twice",
    "first time playing in the browser and it feels like I remember",
    "who is up for a score challenge this weekend?",
    "I think the timer is faster after a few levels, or I am slower",
    "ran out of time with three tiles left, painful",
    "is there a way to turn the effects down a bit?",
    "settings has volume and cursor options, that helped",
    "my kid beat my score last night, we are no longer speaking",
    "casual game, serious feelings",
    "one more round, then bed",
    "that last move was a lucky accident, I am keeping it",
    "the pirate one has the best music",
    "what is your go-to opening move?",
    "I like how quiet this chat is",
    "back from lunch, picking up where I left off",
    "someone should write down all the tricks they have found",
    "the bug one is harder than it looks",
    "I just realised I can pause it, game changer",
    "anyone else save and quit right before a big move?",
    "finally unlocked the next stage",
    "good luck on the hard one today",
    "that is a wrap for me, see you tomorrow"
  ];
  var SEED_DAYS = 4;  // the seed runs over this many days before today, and today up to now

  // a small seeded random sequence, so that the same seed is the same every load
  function mulberry32(a) {
    return function () {
      a = (a + 0x6D2B79F5) | 0;
      var t = Math.imul(a ^ (a >>> 15), 1 | a);
      t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }
  // The made-up history as of `now`, oldest first: 34 messages for each of the SEED_DAYS days before today and the part of
  // today's that is already past, at times of the day that never change. With a table of games (createGames) each message is from
  // a game: most of a user's are from the game that user mostly plays, the rest from any, so that the room has a spread of games.
  function seedMessages(now, games) {
    var rnd = mulberry32(20261030), out = [], d0 = new Date(now), index = 0, perDay = 34;
    var list = games ? games.all() : [];
    for (var back = SEED_DAYS; back >= 0; back--) {
      for (var i = 0; i < perDay; i++) {
        var minute = 7 * 60 + 20 + Math.floor(i * 16 * 60 / perDay + rnd() * 18);
        var t = new Date(d0.getFullYear(), d0.getMonth(), d0.getDate() - back, 0, minute).getTime();
        var user = SEED_USERS[Math.floor(rnd() * SEED_USERS.length)];
        var text = SEED_LINES[Math.floor(rnd() * SEED_LINES.length)];
        index++;
        var m = { id: pad(t, 13) + "-s" + pad(index, 4), t: t, user: user, text: text };
        if (list.length) {
          var r = rnd(), g = list[r < 0.7 ? (SEED_USERS.indexOf(user) * 5 + 3) % list.length : Math.floor(rnd() * list.length)];
          addFields(m, gameFields(games, g.id));
        }
        if (t <= now - 60000) out.push(m);
      }
    }
    return out.sort(byTime);
  }

  // The messages that are kept: those sent here and those received since, in one localStorage item, newest `cap`. `store` may be
  // null (storage blocked): the messages then last until the page closes. A write merges what another tab wrote first, so that
  // two pages (two games) share one room.
  function createStore(store, key, cap) {
    var items = [], saved = !!store;
    function clean(raw) {
      var out = [], seen = {};
      (Array.isArray(raw) ? raw : []).forEach(function (m) {
        if (!validMessage(m)) return;
        var text = cleanText(m.text);
        if (!text || seen[m.id]) return;
        seen[m.id] = true;
        out.push(addFields({ id: m.id, t: m.t, user: m.user, text: text }, cleanGame(m)));
      });
      return out;
    }
    function read() {
      try {
        var o = store ? JSON.parse(store.getItem(key) || "null") : null;
        return (o && o.v === 1) ? clean(o.messages) : [];
      } catch (e) { return []; }
    }
    // merges what is stored into the list; the messages that were new here
    function reload() {
      var have = {}, fresh = [];
      items.forEach(function (m) { have[m.id] = true; });
      read().forEach(function (m) { if (!have[m.id]) { items.push(m); fresh.push(m); } });
      items.sort(byTime);
      if (items.length > cap) items = items.slice(items.length - cap);
      return fresh;
    }
    function write() {
      if (!store) { saved = false; return; }
      try { store.setItem(key, JSON.stringify({ v: 1, messages: items })); saved = true; } catch (e) { saved = false; }
    }
    reload();
    return {
      all: function () { return items.slice(); },
      add: function (m) {
        reload();
        if (!items.some(function (x) { return x.id === m.id; })) items.push(m);
        items.sort(byTime);
        if (items.length > cap) items = items.slice(items.length - cap);
        write();
      },
      reload: reload,
      isSaved: function () { return saved; }
    };
  }

  function sendError(code, message, waitMs) {
    var e = new Error(message);
    e.code = code;
    if (waitMs) e.waitMs = waitMs;
    return e;
  }
  // The local test source (the interface is at the top of this file). opts: game (the id of the page's game, which every message
  // sent here carries) and games (the site's table, a list or a createGames() result); and, all for tests: now() (milliseconds),
  // random(), storage (null: none), timers {set, clear}, auto (false: no messages "received" by themselves).
  function localSource(opts) {
    opts = opts || {};
    var games = asGames(opts.games, opts.game);
    // the game the messages sent here carry: a page's own, or (a function) whichever game the desktop has in front
    function hereNow() { var g = typeof opts.game === "function" ? opts.game() : opts.game; return typeof g === "string" ? g : ""; }
    var now = opts.now || function () { return Date.now(); };
    var rnd = opts.random || Math.random;
    var store = opts.storage !== undefined ? opts.storage : storage();
    var timers = opts.timers || { set: function (f, ms) { return setTimeout(f, ms); }, clear: function (id) { clearTimeout(id); } };
    var live = createStore(store, KEY_LIVE, LIMITS.keep);
    var limiter = createRateLimiter(LIMITS.rateMs);
    var handlers = { message: [], error: [] };
    var name = DEFAULT_NAME, timer = null, closed = false, connected = false, warnedStorage = false, count = 0;
    var seedCache = { day: "", list: [] };
    try {
      var kept = store ? store.getItem(KEY_NAME) : null;
      if (kept && usernameError(kept) === "") name = kept;
    } catch (e) {}

    function seed() {
      var t = now(), day = dayKey(t);
      if (seedCache.day !== day) seedCache = { day: day, list: seedMessages(t, games) };
      return seedCache.list.filter(function (m) { return m.t <= t; });
    }
    function everything() { return seed().concat(live.all()).sort(byTime); }
    function emit(kind, arg) { handlers[kind].slice().forEach(function (f) { try { f(arg); } catch (e) {} }); }
    // an id sorts in time order; the random part and the count keep two messages of the same millisecond apart, here and in another tab
    function make(t, user, text, game) {
      var m = { id: pad(t, 13) + "-" + pad(Math.floor(rnd() * 1679616).toString(36), 4) + (++count).toString(36), t: t, user: user, text: text };
      return addFields(m, gameFields(games, game));
    }
    function pick(list) { return list[Math.floor(rnd() * list.length)]; }
    // A message "from another viewer", for the panel to show something arriving
    function simulate() {
      var user = pick(SEED_USERS), tries = 0;
      while (user === name && tries++ < 5) user = pick(SEED_USERS);
      var listed = games.all(), m = make(now(), user, pick(SEED_LINES), listed.length ? pick(listed).id : "");
      live.add(m);
      emit("message", m);
      return m;
    }
    function schedule() {
      if (closed || opts.auto === false) return;
      timer = timers.set(function () { simulate(); schedule(); }, 20000 + Math.floor(rnd() * 40000));
    }
    // another page of the site wrote to the room: its messages are new here
    function onStorage(ev) {
      if (ev && ev.key !== KEY_LIVE && ev.key !== null) return;
      live.reload().forEach(function (m) { emit("message", m); });
    }

    return {
      testData: true,
      connect: function () {
        return new Promise(function (resolve) {
          connected = true;
          schedule();
          try { if (typeof window !== "undefined" && window.addEventListener) window.addEventListener("storage", onStorage); } catch (e) {}
          resolve({
            user: { name: name, signedIn: false }, room: "all games", canRename: true, testData: true,
            note: ByopopStrings.t("chat.note")
          });
        });
      },
      history: function (limit, before) {
        return new Promise(function (resolve) { resolve(pageOf(everything(), limit, before)); });
      },
      send: function (text) {
        return new Promise(function (resolve, reject) {
          if (closed || !connected) { reject(sendError("offline", ByopopStrings.t("chat.send.offline"))); return; }
          var clean = cleanText(text), problem = textProblem(clean);
          if (problem === "empty") { reject(sendError("empty", ByopopStrings.t("chat.send.empty"))); return; }
          if (problem === "long") { reject(sendError("long", ByopopStrings.t("chat.send.long", {max: LIMITS.textMax}))); return; }
          if (usernameError(name)) { reject(sendError("name", ByopopStrings.t("chat.send.name"))); return; }
          var wait = limiter.take(now());
          if (wait) { reject(sendError("rate", ByopopStrings.t("chat.send.rate"), wait)); return; }
          var m = make(now(), name, clean, hereNow());
          live.add(m);
          if (!live.isSaved() && !warnedStorage) {
            warnedStorage = true;
            emit("error", { code: "storage", message: ByopopStrings.t("chat.send.storage"), fatal: false });
          }
          emit("message", m);
          resolve(m);
        });
      },
      onMessage: function (fn) { handlers.message.push(fn); return function () { handlers.message = handlers.message.filter(function (f) { return f !== fn; }); }; },
      onError: function (fn) { handlers.error.push(fn); return function () { handlers.error = handlers.error.filter(function (f) { return f !== fn; }); }; },
      setUsername: function (next) {
        return new Promise(function (resolve, reject) {
          var n = cleanUsername(next), why = usernameError(n);
          if (why) { reject(sendError("name", why)); return; }
          name = n;
          try { if (store) store.setItem(KEY_NAME, n); } catch (e) {}
          resolve(n);
        });
      },
      // for tests and for looking at the panel: a message now, as if another viewer had sent it
      simulate: simulate,
      close: function () {
        closed = true;
        if (timer !== null) timers.clear(timer);
        try { if (typeof window !== "undefined" && window.removeEventListener) window.removeEventListener("storage", onStorage); } catch (e) {}
      }
    };
  }
  // END OF THE TEST DATA

  // ---------- the panel ----------
  // The panel's own styles (prefix bpc-), added once. The colours are the page's tokens (--panel, --fg, --muted, --line, --accent).
  var STYLE =
    ".bpc { display: flex; flex-direction: column; flex: 1; min-height: 0; }\n" +
    ".bpc [hidden] { display: none !important; }\n" +
    ".bpc-sr { position: absolute; width: 1px; height: 1px; margin: -1px; padding: 0; overflow: hidden; clip-path: inset(50%); white-space: nowrap; border: 0; }\n" +
    ".bpc-log { flex: 1; min-height: 0; overflow-y: auto; overscroll-behavior: contain; padding: 8px 10px; background: var(--bg);\n" +
    "  font-family: var(--font-mono, ui-monospace, monospace); font-size: 12.5px; line-height: 1.45; }\n" +
    ".bpc-list { list-style: none; margin: 0; padding: 0; }\n" +
    ".bpc-row { padding: 1px 0 1px 6ch; text-indent: -6ch; overflow-wrap: anywhere; }\n" +
    ".bpc-time { color: var(--muted); cursor: help; }\n" +
    ".bpc-game { display: inline-block; text-indent: 0; padding: 0 5px; border-radius: 4px; background: var(--bpc-c, #6b3fd1); color: #fff;\n" +
    "  font-size: 11.5px; font-weight: 700; line-height: 1.4; text-decoration: none; vertical-align: 1px; }\n" +
    ".bpc-game:hover { text-decoration: underline; }\n" +
    "@media (forced-colors: active) { .bpc-game { border: 1px solid LinkText; } }\n" +
    ".bpc-game.bpc-game-icon { padding: 0; background: none; box-shadow: none !important; vertical-align: -3px; border-radius: 3px; }\n" +
    ".bpc-game-icon img { display: block; width: 16px; height: 16px; }\n" +
    ".bpc-game-icon:hover, .bpc-game-icon:focus-visible { outline: 2px solid var(--accent); outline-offset: 1px; }\n" +
    ".bpc-nick { font-weight: 700; color: hsl(var(--bpc-h, 260) 55% 25%); }\n" +
    ".bpc-me .bpc-nick { text-decoration: underline; text-decoration-thickness: 1px; text-underline-offset: 2px; }\n" +
    ".bpc-day { display: flex; align-items: center; gap: 8px; text-indent: 0; padding: 10px 0 4px 0; color: var(--muted); font-size: 12px; }\n" +
    ".bpc-day::before, .bpc-day::after { content: \"\"; flex: 1; border-top: 1px solid var(--line); }\n" +
    ".bpc-start, .bpc-older { display: block; margin: 0 auto 6px; font-size: 12px; color: var(--muted); text-align: center; }\n" +
    ".bpc-older { padding: 3px 10px; }\n" +
    ".bpc-jump { margin: 0; align-self: center; font-size: 12px; padding: 3px 12px; border-radius: 99px; background: var(--accent); color: var(--accent-fg); border-color: var(--accent); font-weight: 600; }\n" +
    ".bpc-foot { padding: 8px 10px 10px; display: grid; gap: 6px; border-top: 1px solid var(--line); }\n" +
    ".bpc-foot p { margin: 0; }\n" +
    ".bpc-note, .bpc-hint, .bpc-status, .bpc-count { font-size: 12px; color: var(--muted); }\n" +
    ".bpc-status:empty, .bpc-count:empty, .bpc-note:empty { display: none; }\n" +
    ".bpc-form { display: flex; gap: 6px; }\n" +
    ".bpc-form input, .bpc-nameform input { flex: 1; min-width: 0; font: inherit; color: var(--fg); background: var(--bg);\n" +
    "  border: 1px solid var(--line); border-radius: 8px; padding: 6px 8px; }\n" +
    ".bpc-form input:focus-visible, .bpc-nameform input:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }\n" +
    ".bpc-form input[aria-invalid=true], .bpc-nameform input[aria-invalid=true] { border-color: var(--gem-ruby, #b3261e); }\n" +
    ".bpc-who { display: flex; flex-wrap: wrap; align-items: center; gap: 4px 8px; font-size: 13px; }\n" +
    ".bpc-who strong { font-family: var(--font-mono, monospace); font-weight: 700; overflow-wrap: anywhere; }\n" +
    ".bpc-who button, .bpc-nameform button, .bpc-form button { font-size: 13px; padding: 4px 10px; }\n" +
    ".bpc-nameform { display: grid; grid-template-columns: minmax(0, 1fr) auto auto; gap: 6px; align-items: center; }\n" +
    ".bpc-nameform label { grid-column: 1 / -1; font-size: 13px; font-weight: 600; }\n" +
    ".bpc-nameform .bpc-hint, .bpc-nameform .bpc-nameerr { grid-column: 1 / -1; }\n" +
    ".bpc-nameerr { margin: 0; font-size: 12.5px; color: #b3261e; }\n" +
    ".bpc-nameerr:empty { display: none; }\n" +
    // the test chat's tag (values, not sentences: the sentence is its tooltip)
    ".bpc-tag { display: inline-block; padding: 0 4px; border: 1px solid currentColor; font-size: 11px; font-weight: 700; letter-spacing: 0.06em; text-decoration: none; cursor: help; }\n";

  function make(tag, className, text) {
    var e = document.createElement(tag);
    if (className) e.className = className;
    if (text) e.textContent = text;
    return e;
  }
  // One message as a list item, "09:05 [BW] <alice> hello": its text is exactly formatLine(m, games), with the pieces in their own
  // elements. Text goes in with textContent and text nodes only. The one link is the game's tag, and it is made from the site's
  // table of games (a createGames() result) alone: its address, words and colour are the table's, never the message's.
  function renderRow(doc, m, me, games) {
    var li = doc.createElement("li");
    li.className = "bpc-row" + (m.user === me ? " bpc-me" : "");
    var time = doc.createElement("time");
    time.className = "bpc-time";
    time.textContent = formatTime(m.t);
    time.setAttribute("datetime", new Date(m.t).toISOString());
    time.setAttribute("title", formatFull(m.t));
    var nick = doc.createElement("span");
    nick.className = "bpc-nick";
    nick.style.setProperty("--bpc-h", String(hueOf(m.user)));
    var open = doc.createElement("span"), close = doc.createElement("span");
    open.setAttribute("aria-hidden", "true"); open.textContent = "<";
    close.setAttribute("aria-hidden", "true"); close.textContent = ">";
    nick.appendChild(open);
    nick.appendChild(doc.createTextNode(m.user));
    nick.appendChild(close);
    var text = doc.createElement("span");
    text.className = "bpc-text";
    text.textContent = m.text;
    li.appendChild(time);
    li.appendChild(doc.createTextNode(" "));
    var g = gameOf(games, m);
    if (g) {
      li.appendChild(renderGameTag(doc, g, g.id === games.here, typeof games.icon === "function" ? games.icon(g.id) : ""));
      li.appendChild(doc.createTextNode(" "));
    }
    li.appendChild(nick);
    li.appendChild(doc.createTextNode(" "));
    li.appendChild(text);
    return li;
  }
  // The tag of a game, "[BW]": a link to the game's page (same tab) with the game's name on hover, in the badge's colour. Its name
  // for a screen reader is the full title and the initials (a name must hold the words the link shows). The brackets are for the eye.
  // For a game the player has, the tag is the game's own icon instead (the user, 2026-11-07): the same link and hover, named by
  // the title alone, since it shows no words.
  function renderGameTag(doc, g, own, icon) {
    var a = doc.createElement("a"), open = doc.createElement("span"), close = doc.createElement("span");
    a.setAttribute("href", g.page);
    a.setAttribute("title", g.title);
    if (own) a.setAttribute("aria-current", "page");
    if (icon) {
      var img = doc.createElement("img");
      a.className = "bpc-game bpc-game-icon";
      a.setAttribute("aria-label", g.title);
      img.setAttribute("src", icon);
      img.setAttribute("alt", "");
      img.setAttribute("width", "16");
      img.setAttribute("height", "16");
      a.appendChild(img);
      return a;
    }
    a.className = "bpc-game";
    a.setAttribute("aria-label", g.title + " (" + g.tag + ")");
    a.style.setProperty("--bpc-c", g.colour);
    open.setAttribute("aria-hidden", "true"); open.textContent = "[";
    close.setAttribute("aria-hidden", "true"); close.textContent = "]";
    a.appendChild(open);
    a.appendChild(doc.createTextNode(g.tag));
    a.appendChild(close);
    return a;
  }
  function renderDay(doc, t, now) {
    var li = doc.createElement("li"), span = doc.createElement("span");
    li.className = "bpc-day";
    span.textContent = dayLabel(t, now);
    li.appendChild(span);
    return li;
  }
  function errorWords(err) {
    var code = err && err.code;
    if (code === "rate") return ByopopStrings.t("chat.words.rate");
    if (code === "long") return ByopopStrings.t("chat.words.long", {max: LIMITS.textMax});
    if (code === "empty") return ByopopStrings.t("chat.words.empty");
    if (code === "name") return ByopopStrings.t("chat.words.name");
    if (code === "offline") return ByopopStrings.t("chat.words.offline");
    return (err && err.message) || ByopopStrings.t("chat.words.failed");
  }

  var idCount = 0;
  // Draws the chat into root, the page's <aside id="chat"> (web/shell.html), which has its heading with the toggle button
  // ([data-chat-toggle], with an [data-chat-unread] count inside it) and an empty [data-chat-body]. It builds on first showing.
  //   source      the source (default: the local test source)
  //   stage       the page's stage; the panel puts the chat beside it when the stage keeps its size with the chat there, and below it
  //               otherwise (layout: the class "chat-below" on the root element, which the page's styles answer)
  //   onLeave     onLeave(backwards) after F6 or Shift+F6 typed in the chat: the page moves the focus to the next region
  //   visible     whether it is shown (false: it is not even built, and no source is opened, until setVisible(true))
  //   game, games the id of the page's own game and the site's table of games (ByopopGames: a list, see above); the local source
  //               puts the game on what it sends, and the panel draws each message's game tag from the table
  // Returns { setVisible, layout, setCollapsed, source }.
  function mount(root, opts) {
    opts = opts || {};
    var games = asGames(opts.games, opts.game, opts.iconOf);
    var source = opts.source || localSource({ game: opts.game, games: games });
    var toggle = root.querySelector("[data-chat-toggle]"), unreadEl = root.querySelector("[data-chat-unread]"), body = root.querySelector("[data-chat-body]");
    var st = { built: false, visible: false, collapsed: false, me: "", hasMore: true, loading: false, ids: {}, list: [], unread: 0, sending: false, canRename: false };
    var limiter = createRateLimiter(LIMITS.rateMs);
    var log, listEl, older, startNote, jump, input, countEl, statusEl, meName, nameForm, nameInput, nameErr, changeBtn, note;
    var uid = ++idCount;
    try { st.collapsed = storage().getItem(KEY_COLLAPSED) === "1"; } catch (e) {}

    function now() { return Date.now(); }
    function say(text) { if (statusEl) statusEl.textContent = text || ""; }
    function nearBottom() { return log.scrollHeight - log.scrollTop - log.clientHeight < 40; }
    function toBottom() { log.scrollTop = log.scrollHeight; jump.hidden = true; }
    // Changes to the log that the screen reader should not read out (the first screenful, older messages put above)
    function quietly(fn) {
      log.setAttribute("aria-live", "off");
      fn();
      setTimeout(function () { log.setAttribute("aria-live", "polite"); }, 150);
    }
    function renderAll() {
      listEl.textContent = "";
      var t = now();
      withDays(st.list).forEach(function (row) {
        listEl.appendChild(row.msg ? renderRow(document, row.msg, st.me, games) : renderDay(document, row.t, t));
      });
      older.hidden = !st.hasMore;
      startNote.hidden = st.hasMore || !st.list.length;
      older.disabled = false;
      older.textContent = ByopopStrings.t("chat.older");
    }
    // a new message: after the last one, in place when it is the newest
    function addMessage(m, mine) {
      if (st.ids[m.id]) return;
      st.ids[m.id] = true;
      var follow = nearBottom() || mine, last = st.list[st.list.length - 1];
      if (last && byTime(m, last) < 0) {  // out of order (another page of the site): put in its place
        st.list.push(m); st.list.sort(byTime);
        renderAll();
      } else {
        st.list.push(m);
        if (!last || dayKey(last.t) !== dayKey(m.t)) listEl.appendChild(renderDay(document, m.t, now()));
        listEl.appendChild(renderRow(document, m, st.me, games));
        startNote.hidden = st.hasMore;
      }
      if (st.collapsed) { st.unread++; showUnread(); }
      if (follow) toBottom(); else if (!st.collapsed) jump.hidden = false;
    }
    function showUnread() {
      if (!unreadEl) return;
      unreadEl.textContent = st.unread ? ByopopStrings.t("chat.unread", {count: st.unread > 99 ? "99+" : String(st.unread)}) : "";
      unreadEl.hidden = !st.unread;
    }
    function loadOlder() {
      if (st.loading || !st.hasMore || !st.list.length) return;
      st.loading = true;
      older.disabled = true;
      older.textContent = ByopopStrings.t("chat.loading");
      source.history(LIMITS.page, st.list[0].id).then(function (msgs) {
        st.loading = false;
        if (msgs.length < LIMITS.page) st.hasMore = false;
        msgs = msgs.filter(function (m) { return !st.ids[m.id]; });
        msgs.forEach(function (m) { st.ids[m.id] = true; });
        var height = log.scrollHeight, top = log.scrollTop;
        quietly(function () { st.list = msgs.concat(st.list); renderAll(); });
        log.scrollTop = top + (log.scrollHeight - height);  // what the viewer was looking at stays where it was
      }, function (err) {
        st.loading = false;
        older.disabled = false;
        older.textContent = ByopopStrings.t("chat.older");
        say((err && err.message) || ByopopStrings.t("chat.older_failed"));
      });
    }
    function fill() {  // a tall log with few messages: load until it can scroll, or the start
      if (st.hasMore && !st.collapsed && log.clientHeight > 0 && log.scrollHeight <= log.clientHeight && !st.loading) {
        var n = st.list.length;
        loadOlder();
        setTimeout(function () { if (st.list.length !== n) fill(); }, 50);
      }
    }
    function showMe() { meName.textContent = st.me; }

    function submit(ev) {
      ev.preventDefault();
      if (st.sending) return;
      var clean = cleanText(input.value), problem = textProblem(clean);
      if (problem === "empty") { say(errorWords({ code: "empty" })); return; }
      if (problem === "long") { say(errorWords({ code: "long" })); return; }
      if (limiter.take(now())) { say(errorWords({ code: "rate" })); return; }
      st.sending = true;
      source.send(clean).then(function (m) {
        st.sending = false;
        input.value = "";
        updateCount();
        say("");
        addMessage(m, true);
      }, function (err) {
        st.sending = false;
        say(errorWords(err));
      });
    }
    function updateCount() {
      var left = LIMITS.textMax - input.value.length;
      countEl.textContent = left <= 60 ? ByopopStrings.t("chat.left", {count: left}) : "";
    }
    function nameFormOpen(open) {
      nameForm.hidden = !open;
      if (open) {
        nameInput.value = st.me;
        nameInput.removeAttribute("aria-invalid");
        nameErr.textContent = "";
        nameInput.focus();
        nameInput.select();
      } else {
        changeBtn.focus();
      }
    }
    function saveName(ev) {
      ev.preventDefault();
      var n = cleanUsername(nameInput.value), why = usernameError(n);
      if (why) {
        nameInput.setAttribute("aria-invalid", "true");
        nameErr.textContent = why;
        nameInput.focus();
        return;
      }
      source.setUsername(n).then(function (kept) {
        st.me = kept;
        showMe();
        var follow = nearBottom();
        renderAll();
        if (follow) toBottom();
        nameInput.removeAttribute("aria-invalid");
        nameErr.textContent = "";
        nameFormOpen(false);
        say(ByopopStrings.t("chat.you_are_now", {name: kept}));
      }, function (err) {
        nameInput.setAttribute("aria-invalid", "true");
        nameErr.textContent = (err && err.message) || ByopopStrings.t("chat.name_failed");
        nameInput.focus();
      });
    }

    function build() {
      if (!document.getElementById("byopop-chat-style")) {
        var style = make("style");
        style.id = "byopop-chat-style";
        style.textContent = STYLE;
        document.head.appendChild(style);
      }
      body.textContent = "";
      var wrap = make("div", "bpc");
      var id = "bpc-" + uid;
      log = make("div", "bpc-log");
      log.setAttribute("role", "log");
      log.setAttribute("aria-live", "polite");
      log.setAttribute("aria-relevant", "additions");
      log.setAttribute("aria-label", ByopopStrings.t("chat.log"));
      log.tabIndex = 0;
      older = make("button", "bpc-older", ByopopStrings.t("chat.older"));
      older.type = "button";
      older.hidden = true;
      startNote = make("p", "bpc-start", ByopopStrings.t("chat.start"));
      startNote.hidden = true;
      listEl = make("ol", "bpc-list");
      log.appendChild(older); log.appendChild(startNote); log.appendChild(listEl);
      jump = make("button", "bpc-jump", ByopopStrings.t("chat.jump"));
      jump.type = "button";
      jump.hidden = true;

      var foot = make("div", "bpc-foot");
      // a test chat says so with a tag, its sentence the tag's tooltip (the user's rule: values, not sentences)
      note = make("p", "bpc-note");
      var form = make("form", "bpc-form");
      form.noValidate = true;
      var label = make("label", "bpc-sr", ByopopStrings.t("chat.message"));
      label.htmlFor = id + "-text";
      input = make("input");
      input.id = id + "-text";
      input.type = "text";
      input.maxLength = LIMITS.textMax;
      input.autocomplete = "off";
      input.spellcheck = false;
      input.setAttribute("autocapitalize", "off");
      input.setAttribute("enterkeyhint", "send");
      input.setAttribute("placeholder", ByopopStrings.t("chat.placeholder"));
      input.setAttribute("aria-describedby", id + "-hint");
      var send = make("button", "primary", ByopopStrings.t("chat.send"));
      send.type = "submit";
      form.appendChild(label); form.appendChild(input); form.appendChild(send);
      // (read out with the field, not shown: the placeholder and the count say what matters)
      var hint = make("p", "bpc-sr", ByopopStrings.t("chat.hint", {max: LIMITS.textMax}));
      hint.id = id + "-hint";
      countEl = make("p", "bpc-count");
      statusEl = make("p", "bpc-status");
      statusEl.setAttribute("role", "status");

      var who = make("div", "bpc-who");
      who.appendChild(document.createTextNode(ByopopStrings.t("chat.you") + " "));
      meName = make("strong");
      who.appendChild(meName);
      changeBtn = make("button", "", ByopopStrings.t("chat.change_name"));
      changeBtn.type = "button";
      who.appendChild(changeBtn);
      nameForm = make("form", "bpc-nameform");
      nameForm.noValidate = true;
      nameForm.hidden = true;
      var nameLabel = make("label", "", ByopopStrings.t("chat.username"));
      nameLabel.htmlFor = id + "-name";
      nameInput = make("input");
      nameInput.id = id + "-name";
      nameInput.type = "text";
      nameInput.autocomplete = "off";
      nameInput.spellcheck = false;
      nameInput.maxLength = 64;
      nameInput.setAttribute("autocapitalize", "off");
      nameErr = make("p", "bpc-nameerr");
      nameErr.id = id + "-nameerr";
      nameErr.setAttribute("role", "alert");
      var nameHint = make("p", "bpc-hint", ByopopStrings.t("chat.username_hint"));
      nameHint.id = id + "-namehint";
      nameInput.setAttribute("aria-describedby", nameHint.id + " " + nameErr.id);
      var saveBtn = make("button", "primary", ByopopStrings.t("chat.save"));
      saveBtn.type = "submit";
      var cancelBtn = make("button", "", ByopopStrings.t("chat.cancel"));
      cancelBtn.type = "button";
      nameForm.appendChild(nameLabel); nameForm.appendChild(nameInput); nameForm.appendChild(saveBtn); nameForm.appendChild(cancelBtn);
      nameForm.appendChild(nameHint); nameForm.appendChild(nameErr);
      // signing in is not there yet: the button is disabled, and its tooltip says why
      var signin = make("button", "bpc-signin", ByopopStrings.t("chat.signin"));
      signin.type = "button";
      signin.disabled = true;
      signin.title = ByopopStrings.t("chat.signin_note");
      var signRow = make("div", "bpc-who");
      signRow.appendChild(signin);

      foot.appendChild(form); foot.appendChild(hint); foot.appendChild(countEl); foot.appendChild(statusEl);
      statusEl.setAttribute("aria-atomic", "true");
      foot.appendChild(who); foot.appendChild(nameForm); foot.appendChild(signRow); foot.appendChild(note);
      wrap.appendChild(log); wrap.appendChild(jump); wrap.appendChild(foot);
      body.appendChild(wrap);

      form.addEventListener("submit", submit);
      input.addEventListener("input", updateCount);
      older.addEventListener("click", loadOlder);
      jump.addEventListener("click", function () { toBottom(); input.focus(); });
      log.addEventListener("scroll", function () {
        if (nearBottom()) jump.hidden = true;
        if (log.scrollTop < 30) loadOlder();
      });
      changeBtn.addEventListener("click", function () { nameFormOpen(nameForm.hidden); });
      cancelBtn.addEventListener("click", function () { nameFormOpen(false); });
      nameForm.addEventListener("submit", saveName);
      nameInput.addEventListener("input", function () { nameInput.removeAttribute("aria-invalid"); nameErr.textContent = ""; });
      nameInput.addEventListener("keydown", function (ev) { if (ev.key === "Escape") { ev.preventDefault(); ev.stopPropagation(); nameFormOpen(false); } });

      say(ByopopStrings.t("chat.connecting"));
      // (a message of this viewer's own that comes in while a send is under way is that send's)
      source.onMessage(function (m) { if (validMessage(m)) addMessage(m, st.sending && m.user === st.me); });
      source.onError(function (e) { say((e && e.message) || ByopopStrings.t("chat.unreachable")); if (e && e.fatal) { input.disabled = true; send.disabled = true; } });
      source.connect().then(function (info) {
        st.me = info.user && info.user.name || "";
        st.canRename = !!info.canRename;
        showMe();
        note.textContent = "";
        if (info.note) {
          var tag = make("abbr", "bpc-tag", ByopopStrings.t("chat.test"));
          tag.title = info.note;
          note.appendChild(tag);
        }
        changeBtn.hidden = !st.canRename;
        signRow.hidden = !!(info.user && info.user.signedIn);
        return source.history(LIMITS.page, null);
      }).then(function (msgs) {
        st.hasMore = msgs.length >= LIMITS.page;
        msgs.forEach(function (m) { st.ids[m.id] = true; });
        st.list = msgs;
        quietly(function () { renderAll(); toBottom(); });
        say("");
        fill();
      }, function (err) {
        say((err && err.message) || ByopopStrings.t("chat.unreachable_start"));
        input.disabled = true; send.disabled = true;
      });
      st.built = true;
    }

    function setCollapsed(c, remember) {
      st.collapsed = !!c;
      root.classList.toggle("chat-collapsed", st.collapsed);
      body.hidden = st.collapsed;
      toggle.setAttribute("aria-expanded", st.collapsed ? "false" : "true");
      toggle.setAttribute("title", ByopopStrings.t(st.collapsed ? "chat.show" : "chat.hide"));
      if (!st.collapsed) { st.unread = 0; showUnread(); if (st.built) { toBottom(); fill(); } }
      if (remember) { try { storage().setItem(KEY_COLLAPSED, st.collapsed ? "1" : "0"); } catch (e) {} }
      layout();
    }

    // The chat goes beside the stage only when the stage keeps its size there. The stage's width is
    // min(100% of its box, (100vh - 90px) * the game's aspect) (web/shell.html): beside the chat its box is narrower by the chat's width,
    // so the chat is beside only when the viewport has room, past the stage at that width and the box's padding, for a chat of at
    // least CHAT_MIN wide; it takes what is left up to CHAT_MAX. Else it is below the stage (a phone upright). The stage's size is
    // never what gives way.
    function layout() {
      if (!opts.stage) return;
      var below = false;
      if (!root.hidden) {
        var vw = document.documentElement.clientWidth || window.innerWidth, vh = window.innerHeight;
        var style = window.getComputedStyle(opts.stage), aw = parseFloat(style.getPropertyValue("--aspect-w")) || 4 / 3;
        // a pixel-perfect stage has its own width (web/shell.html, sizeStage)
        var stageWidth = parseFloat(style.getPropertyValue("--pp-w")) || Math.min(vw - 24, (vh - 90) * aw);
        document.documentElement.style.setProperty("--stage-px", stageWidth + "px");
        var room = Math.floor(vw - stageWidth - 24 - 12);  // the box's padding, and the chat's margin at its right
        below = room < CHAT_MIN;
        root.style.setProperty("--chat-w", Math.min(CHAT_MAX, Math.max(CHAT_MIN, room)) + "px");
      }
      document.documentElement.classList.toggle("chat-below", below);
    }
    function setVisible(on) {
      st.visible = !!on;
      root.hidden = !on;
      if (on && !st.built) { build(); setCollapsed(st.collapsed, false); }
      layout();
    }

    toggle.addEventListener("click", function () { setCollapsed(!st.collapsed, true); });
    // F6 and Shift+F6 move on from the chat as they do from the game (web/shell.html leaveGame): the page's function does it
    root.addEventListener("keydown", function (ev) {
      if (ev.key === "F6" && !ev.ctrlKey && !ev.altKey && !ev.metaKey && opts.onLeave) {
        ev.preventDefault();
        opts.onLeave(ev.shiftKey);
      }
    });
    window.addEventListener("resize", layout);
    window.addEventListener("orientationchange", layout);
    toggle.setAttribute("aria-expanded", st.collapsed ? "false" : "true");
    setVisible(opts.visible !== false);
    return { setVisible: setVisible, layout: layout, setCollapsed: function (c) { setCollapsed(c, true); }, source: source };
  }

  return {
    LIMITS: LIMITS, KEYS: { collapsed: KEY_COLLAPSED, name: KEY_NAME, messages: KEY_LIVE },
    cleanUsername: cleanUsername, usernameError: usernameError, cleanText: cleanText, textProblem: textProblem,
    createRateLimiter: createRateLimiter, formatTime: formatTime, formatOffset: formatOffset, formatFull: formatFull, dayKey: dayKey,
    dayLabel: dayLabel, formatLine: formatLine, withDays: withDays, hueOf: hueOf, pageOf: pageOf, byTime: byTime, validMessage: validMessage,
    seedMessages: seedMessages, SEED_USERS: SEED_USERS, SEED_LINES: SEED_LINES, createStore: createStore, localSource: localSource,
    createGames: createGames, gameFields: gameFields, cleanGame: cleanGame, GAME_COLOUR_DEFAULT: GAME_COLOUR_DEFAULT,
    renderRow: renderRow, renderDay: renderDay, mount: mount
  };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopChat;
