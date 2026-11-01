// Checks the chat's rules and its local test source (web/chat.js, docs/CHAT_DESIGN.md) under Node:
//   node tools/chattest.js
// The username rules (Discord's: 2 to 32 characters, lowercase letters, digits, "_" and ".", never two periods in a row), the text of
// a message (one line, 500 characters, nothing that hides or reorders text), the rate limit, the line, time and day formats, the
// history (the stored messages: their cap, their checks on the way back in, two pages sharing one room, blocked storage; the made-up
// seed; paging back with "before"), the local source (send, history, the rate limit, the name, the messages "received", a reload
// that keeps what was sent), the game tags ("[BW]" before a username: the table of games and what it lets in, the one link and where
// it comes from, a game the site does not know, the made-up messages from a spread of games, and the generated table against the
// badge table and the listing page's tiles), and that a message is put on the page as text and nothing else (a fake DOM that
// refuses markup, and a look at the source for the ways of writing markup or reaching the network).
// It prints "chattest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const C = require(path.join(__dirname, "..", "web", "chat.js"));

let checks = 0, failed = 0;
function check(name, ok, detail) {
  checks++;
  if (!ok) { failed++; console.log("FAIL " + name + (detail !== undefined ? ": " + JSON.stringify(detail) : "")); }
}
function same(name, a, b) { check(name, JSON.stringify(a) === JSON.stringify(b), { got: a, want: b }); }
const ch = (n) => String.fromCharCode(n);

// a localStorage in memory; blocked: every call throws
function fakeStorage(blocked) {
  const data = {};
  return {
    data,
    getItem(k) { if (blocked) throw new Error("blocked"); return Object.prototype.hasOwnProperty.call(data, k) ? data[k] : null; },
    setItem(k, v) { if (blocked) throw new Error("blocked"); data[k] = String(v); },
    removeItem(k) { delete data[k]; },
  };
}
const local = (y, mo, d, h, mi, s) => new Date(y, mo, d, h || 0, mi || 0, s || 0).getTime();
// a few of the site's games, as the table (build/web/chat-games.js) lists them, for the tests
const TABLE = [
  { id: "bookworm", page: "bookworm.html", tag: "BW", title: "BookWorm Deluxe", colour: "#955c23" },
  { id: "sevenseas", page: "sevenseas.html", tag: "7S", title: "Seven Seas Deluxe", colour: "#1e8080" },
  { id: "noahsark", page: "noahsark.html", tag: "NA", title: "Noah's Ark Deluxe", colour: "#234995" },
];
// n made-up games
const bigTable = (n) => Array.from({ length: n }, (_, i) => ({ id: "game" + String(i).padStart(2, "0"), page: "game" + String(i).padStart(2, "0") + ".html", tag: "G" + i, title: "Game Number " + i, colour: "#234995" }));
// the contrast of two #rrggbb colours (WCAG)
function contrast(a, b) {
  const lum = (c) => { const v = [1, 3, 5].map((i) => parseInt(c.slice(i, i + 2), 16) / 255).map((x) => x <= 0.03928 ? x / 12.92 : Math.pow((x + 0.055) / 1.055, 2.4)); return 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2]; };
  const l1 = lum(a), l2 = lum(b);
  return (Math.max(l1, l2) + 0.05) / (Math.min(l1, l2) + 0.05);
}

(async () => {
  // ---------- usernames ----------
  for (const ok of ["ab", "a_b", "a.b", ".ab.", "__", "42", "a1.b2_c3", "x".repeat(32), "a.b.c.d", "_.", "._"]) {
    check("valid username " + JSON.stringify(ok), C.usernameError(ok) === "", C.usernameError(ok));
  }
  for (const bad of ["", "a", "x".repeat(33), "a..b", "..", "a b", "a-b", "a@b", "a#1", "Ab", "AB", "éa", "abé", "a/b", "a,b", "a:b", " ab", "ab "]) {
    check("invalid username " + JSON.stringify(bad), C.usernameError(bad) !== "");
  }
  check("not a string", C.usernameError(null) !== "" && C.usernameError(undefined) !== "" && C.usernameError(12) !== "" && C.usernameError({}) !== "");
  check("the three reasons differ", new Set([C.usernameError("a"), C.usernameError("a b"), C.usernameError("a..b")]).size === 3);
  check("the length bounds are Discord's", C.LIMITS.nameMin === 2 && C.LIMITS.nameMax === 32);
  same("clean username", [C.cleanUsername("  Alice.B "), C.cleanUsername("ABC_def"), C.cleanUsername(null), C.cleanUsername(undefined)], ["alice.b", "abc_def", "", ""]);
  check("a cleaned name is checked", C.usernameError(C.cleanUsername("  Alice.B ")) === "" && C.usernameError(C.cleanUsername("a..B")) !== "");
  check("every made-up username is valid", C.SEED_USERS.length >= 8 && C.SEED_USERS.every((u) => C.usernameError(u) === ""));

  // ---------- the text of a message ----------
  same("plain text is kept", C.cleanText("hello there"), "hello there");
  same("a newline and a tab become spaces", C.cleanText("a\nb\tc\r\nd"), "a b c d");
  same("runs of spaces and the ends", C.cleanText("   a   b    c  "), "a b c");
  same("controls", C.cleanText("a" + ch(0) + "b" + ch(7) + "c" + ch(0x7f) + "d" + ch(0x85) + "e"), "a b c d e");
  same("zero width and direction marks", C.cleanText("a" + ch(0x200b) + "b" + ch(0x202e) + "c" + ch(0x2066) + "d" + ch(0xfeff) + "e" + ch(0x2028) + "f"), "a b c d e f");
  same("markup and a link stay as typed", C.cleanText('<b>x</b> <img src=x onerror=alert(1)> https://example.com/a?b=1&c=2 javascript:alert(1)'),
    '<b>x</b> <img src=x onerror=alert(1)> https://example.com/a?b=1&c=2 javascript:alert(1)');
  same("an emoji and an accent stay", C.cleanText("café 😀"), "café 😀");
  same("nothing but blanks", C.cleanText(" \n\t " + ch(0x200b)), "");
  same("null and undefined", [C.cleanText(null), C.cleanText(undefined)], ["", ""]);
  check("a number is text", C.cleanText(12) === "12");
  same("text problems", [C.textProblem(""), C.textProblem("a"), C.textProblem("x".repeat(500)), C.textProblem("x".repeat(501)), C.textProblem(null)],
    ["empty", null, null, "long", "empty"]);
  check("the cap is 500", C.LIMITS.textMax === 500);

  // ---------- the rate limit ----------
  {
    const r = C.createRateLimiter(1000);
    check("the first message may go", r.take(5000) === 0);
    check("0 ms later: wait the whole second", r.take(5000) === 1000);
    check("1 ms later: 999", r.take(5001) === 999);
    check("999 ms later: 1", r.take(5999) === 1);
    check("a refused try does not move the clock", r.wait(5999) === 1);
    check("exactly a second later it may go", r.take(6000) === 0);
    check("and then it waits again", r.take(6500) === 500);
    check("a fraction rounds up", r.wait(6999.2) === 1);
    check("a clock set back never waits longer than the interval", r.wait(1000) === 1000);
    r.reset();
    check("reset", r.take(6001) === 0);
    check("the limit is one a second", C.LIMITS.rateMs === 1000);
    const slow = C.createRateLimiter(5000);
    slow.take(0);
    check("another interval", slow.take(4999) === 1 && slow.take(5000) === 0);
  }

  // ---------- times and days ----------
  const t0 = local(2026, 9, 30, 9, 5, 7);
  same("time", [C.formatTime(t0), C.formatTime(local(2026, 9, 30, 0, 0)), C.formatTime(local(2026, 9, 30, 23, 59)), C.formatTime(local(2026, 0, 2, 12, 30))],
    ["09:05", "00:00", "23:59", "12:30"]);
  same("offsets", [C.formatOffset(0), C.formatOffset(-120), C.formatOffset(360), C.formatOffset(-330), C.formatOffset(180), C.formatOffset(-765)],
    ["UTC", "UTC+2", "UTC-6", "UTC+5:30", "UTC-3", "UTC+12:45"]);
  {
    const full = C.formatFull(t0);
    check("the full date and time", /^2026-10-30 09:05:07 UTC([+-]\d{1,2}(:\d\d)?)?$/.test(full), full);
    check("its offset is the machine's", full.endsWith(C.formatOffset(new Date(t0).getTimezoneOffset())), full);
  }
  same("day key", [C.dayKey(t0), C.dayKey(local(2026, 0, 1, 0, 0)), C.dayKey(local(2026, 11, 31, 23, 59))], ["2026-10-30", "2026-01-01", "2026-12-31"]);
  const noon = local(2026, 9, 30, 12);
  same("day labels", [C.dayLabel(noon, noon), C.dayLabel(local(2026, 9, 29, 23, 59), noon), C.dayLabel(local(2026, 9, 28, 8), noon), C.dayLabel(local(2025, 11, 31, 8), noon)],
    ["Today, Friday 30 October 2026", "Yesterday, Thursday 29 October 2026", "Wednesday 28 October 2026", "Wednesday 31 December 2025"]);
  same("yesterday across a month", C.dayLabel(local(2026, 9, 31, 20), local(2026, 10, 1, 0, 5)), "Yesterday, Saturday 31 October 2026");
  same("the line", C.formatLine({ id: "x", t: t0, user: "alice", text: "hi <b>" }), "09:05 <alice> hi <b>");
  {
    const games = C.createGames(TABLE, "bookworm");
    const base = { id: "x", t: t0, user: "alice", text: "hi" };
    same("the line with a game's tag", C.formatLine(Object.assign({ game: "bookworm" }, base), games), "09:05 [BW] <alice> hi");
    same("an initialism that starts with a digit", C.formatLine(Object.assign({ game: "sevenseas" }, base), games), "09:05 [7S] <alice> hi");
    same("no tag for a game the site does not know", C.formatLine(Object.assign({ game: "oldgame", tag: "OG", gameTitle: "Old Game" }, base), games), "09:05 <alice> hi");
    same("no tag when the message has no game", C.formatLine(base, games), "09:05 <alice> hi");
    same("no tag when there is no table", C.formatLine(Object.assign({ game: "bookworm" }, base)), "09:05 <alice> hi");
    same("the message's own tag is not drawn", C.formatLine(Object.assign({ game: "bookworm", tag: "ADMIN" }, base), games), "09:05 [BW] <alice> hi");
  }
  {
    const msgs = [
      { id: "1", t: local(2026, 9, 28, 23, 58), user: "a_a", text: "one" }, { id: "2", t: local(2026, 9, 29, 0, 1), user: "b_b", text: "two" },
      { id: "3", t: local(2026, 9, 29, 0, 2), user: "a_a", text: "three" }, { id: "4", t: local(2026, 9, 30, 8, 0), user: "b_b", text: "four" },
    ];
    const rows = C.withDays(msgs);
    same("rows: a separator before each day's first message", rows.map((r) => r.day ? "day " + r.day : r.msg.id),
      ["day 2026-10-28", "1", "day 2026-10-29", "2", "3", "day 2026-10-30", "4"]);
    same("no messages, no rows", C.withDays([]), []);
    const hues = C.SEED_USERS.map(C.hueOf);
    check("hues are 0 to 359 and always the same", hues.every((h) => h >= 0 && h < 360 && Number.isInteger(h)) && JSON.stringify(hues) === JSON.stringify(C.SEED_USERS.map(C.hueOf)));
    check("a longest name has a hue too", [C.hueOf("z".repeat(32)), C.hueOf("a.b_c".repeat(6) + "xy")].every((h) => Number.isInteger(h) && h >= 0 && h < 360));
    check("the made-up users have different hues", new Set(hues).size >= C.SEED_USERS.length - 2, hues);
  }

  // ---------- the table of games ----------
  {
    const g = C.createGames(TABLE, "bookworm");
    same("the table keeps the three games, in order", g.all().map((x) => x.id), ["bookworm", "sevenseas", "noahsark"]);
    same("an entry", g.get("sevenseas"), TABLE[1]);
    same("the page's own game", g.here, "bookworm");
    check("a game that is not there", g.get("nope") === null && g.get("") === null && g.get(undefined) === null && g.get(5) === null && g.get(null) === null);
    check("a name that every object has is not a game", ["constructor", "__proto__", "toString", "hasOwnProperty", "valueOf"].every((k) => g.get(k) === null));
    check("all() is a copy", (() => { const a = g.all(); a.pop(); return g.all().length === 3; })());
    check("no table: empty, never an error", [undefined, null, "x", 5, {}, [], [null, 5, "x"]].every((x) => C.createGames(x).all().length === 0));
    check("no page's game: none", C.createGames(TABLE).here === "" && C.createGames(TABLE, 5).here === "");
    const one = (over) => C.createGames([Object.assign({}, TABLE[0], over)]).all().length;
    check("a good entry passes the same check", one({}) === 1);
    for (const [name, over] of [
      ["an upper-case id", { id: "BW" }], ["an id with a space", { id: "b w" }], ["an id that starts with a dash", { id: "-bw" }], ["an id with a slash", { id: "a/b" }],
      ["a long id", { id: "a".repeat(41) }], ["no id", { id: undefined }], ["a number for an id", { id: 5 }], ["an empty id", { id: "" }],
      ["a javascript: page", { page: "javascript:alert(1)" }], ["a page in another folder", { page: "../x.html" }], ["a page with a folder", { page: "a/b.html" }],
      ["a page with a query", { page: "x.html?a=1" }], ["a page with a fragment", { page: "x.html#a" }], ["an address", { page: "https://example.com/x.html" }],
      ["a page that is not html", { page: "x.js" }], ["a page with a space", { page: "x y.html" }], ["no page", { page: undefined }],
      ["an empty tag", { tag: "" }], ["a long tag", { tag: "ABCDE" }], ["a tag with markup", { tag: "<b>" }], ["a tag with a space", { tag: "B W" }], ["a tag with brackets", { tag: "[B]" }], ["no tag", { tag: undefined }],
      ["no title", { title: undefined }], ["an empty title", { title: "" }], ["a blank title", { title: " \n " }], ["a long title", { title: "x".repeat(101) }], ["a number for a title", { title: 5 }],
    ]) check("an entry with " + name + " is left out", one(over) === 0);
    check("the same id twice is one game", C.createGames([TABLE[0], Object.assign({}, TABLE[0], { title: "Other" })]).all().length === 1 && C.createGames([TABLE[0], TABLE[0]]).get("bookworm").title === "BookWorm Deluxe");
    check("a bad entry among good ones does not stop the rest", C.createGames([TABLE[0], null, { id: "x" }, TABLE[1]]).all().length === 2);
    for (const bad of ["red", "#fff", "#12345g", "url(x)", "#1234567", "rgb(1,2,3)", 5, undefined, null, ""]) {
      check("a bad colour falls back to the default: " + String(bad), C.createGames([Object.assign({}, TABLE[0], { colour: bad })]).get("bookworm").colour === C.GAME_COLOUR_DEFAULT);
    }
    check("the default colour keeps white text above 4.5", contrast(C.GAME_COLOUR_DEFAULT, "#ffffff") >= 4.5, contrast(C.GAME_COLOUR_DEFAULT, "#ffffff"));
    check("an upper-case colour is fine", C.createGames([Object.assign({}, TABLE[0], { colour: "#ABCDEF" })]).get("bookworm").colour === "#ABCDEF");
    check("a title is one clean line", C.createGames([Object.assign({}, TABLE[0], { title: " Two\nLines  here " })]).get("bookworm").title === "Two Lines here");
    // the game fields of a message
    same("fields for a known game", C.gameFields(g, "sevenseas"), { game: "sevenseas", tag: "7S", gameTitle: "Seven Seas Deluxe" });
    same("fields for a game the table lacks: its id only", C.gameFields(g, "mystery_game"), { game: "mystery_game" });
    same("fields with no table: the id only", C.gameFields(null, "bookworm"), { game: "bookworm" });
    for (const bad of ["", "Bad Id", "a/b", undefined, null, 5, "x".repeat(41), "javascript:alert(1)"]) same("no fields for the id " + JSON.stringify(bad), C.gameFields(g, bad), {});
    same("cleanGame keeps good fields", C.cleanGame({ game: "bookworm", tag: "BW", gameTitle: "BookWorm Deluxe", extra: 1 }), { game: "bookworm", tag: "BW", gameTitle: "BookWorm Deluxe" });
    same("cleanGame: a bad tag or title is dropped, the id kept", C.cleanGame({ game: "bookworm", tag: "<b>x", gameTitle: "x".repeat(101) }), { game: "bookworm" });
    same("cleanGame: a title is cleaned", C.cleanGame({ game: "bookworm", gameTitle: "a\nb" }), { game: "bookworm", gameTitle: "a b" });
    same("cleanGame: without a good id, nothing", [C.cleanGame({ tag: "BW" }), C.cleanGame({ game: "Bad Id", tag: "BW" }), C.cleanGame({ game: 5 }), C.cleanGame(null), C.cleanGame("x")], [{}, {}, {}, {}, {}]);
  }

  // ---------- the stored history ----------
  {
    const st = fakeStorage();
    const KEY = C.KEYS.messages;
    const mk = (i, over) => Object.assign({ id: String(1000 + i).padStart(13, "0") + "-a", t: 1000 + i, user: "abc", text: "m" + i }, over || {});
    const s = C.createStore(st, KEY, 300);
    check("starts empty", s.all().length === 0 && s.isSaved());
    s.add(mk(2)); s.add(mk(1)); s.add(mk(3));
    same("kept in time order", s.all().map((m) => m.text), ["m1", "m2", "m3"]);
    check("written as one item with a version", JSON.parse(st.data[KEY]).v === 1 && JSON.parse(st.data[KEY]).messages.length === 3);
    same("a new store reads it back", C.createStore(st, KEY, 300).all().map((m) => m.text), ["m1", "m2", "m3"]);
    s.add(mk(2));
    check("the same id twice is one message", s.all().length === 3);
    // the cap
    const big = C.createStore(fakeStorage(), KEY, 300);
    for (let i = 0; i < 350; i++) big.add(mk(i));
    check("at most 300 are kept, the newest", big.all().length === 300 && big.all()[0].text === "m50" && big.all()[299].text === "m349");
    // the checks on the way back in: the stored item is the viewer's to edit
    const evil = fakeStorage();
    evil.data[KEY] = JSON.stringify({ v: 1, messages: [
      mk(1), mk(1), { id: "x" }, null, 5, "text", mk(2, { user: "Has Space" }), mk(3, { user: "a" }), mk(4, { text: "" }), mk(5, { text: "x".repeat(501) }),
      mk(6, { t: "late" }), mk(7, { id: 7 }), mk(8, { id: "" }), mk(9, { id: "i".repeat(65) }), mk(10, { t: Infinity }), mk(11, { text: "a\nb\tc" }), mk(12, { text: "  " }),
      mk(13, { text: "<img src=x onerror=1>" }),
    ] });
    same("bad entries are dropped, good ones cleaned", C.createStore(evil, KEY, 300).all().map((m) => m.text), ["m1", "a b c", "<img src=x onerror=1>"]);
    // the game of a message: kept with it, checked on the way back in
    {
      const gs = fakeStorage();
      const gstore = C.createStore(gs, KEY, 300);
      gstore.add(mk(1, { game: "bookworm", tag: "BW", gameTitle: "BookWorm Deluxe" }));
      gstore.add(mk(2));
      gstore.add(mk(3, { game: "retired_game" }));
      same("a message keeps its game through the store", C.createStore(gs, KEY, 300).all().map((m) => [m.game, m.tag, m.gameTitle]), [["bookworm", "BW", "BookWorm Deluxe"], [undefined, undefined, undefined], ["retired_game", undefined, undefined]]);
      const gevil = fakeStorage();
      gevil.data[KEY] = JSON.stringify({ v: 1, messages: [
        mk(1, { game: "Bad Id", tag: "BW" }), mk(2, { game: "bookworm", tag: "<script>" }), mk(3, { game: "bookworm", gameTitle: "x".repeat(101) }), mk(4, { game: 5, tag: 6, gameTitle: 7 }),
        mk(5, { game: "bookworm", tag: "BW", gameTitle: "a\nb" }), mk(6, { game: { x: 1 } }), mk(7, { tag: "BW", gameTitle: "No Game" }),
      ] });
      const got = C.createStore(gevil, KEY, 300).all();
      same("bad game fields are dropped, the message is kept", got.map((m) => [m.text, m.game, m.tag, m.gameTitle]),
        [["m1", undefined, undefined, undefined], ["m2", "bookworm", undefined, undefined], ["m3", "bookworm", undefined, undefined], ["m4", undefined, undefined, undefined],
          ["m5", "bookworm", "BW", "a b"], ["m6", undefined, undefined, undefined], ["m7", undefined, undefined, undefined]]);
    }
    for (const junk of ["{nope", "null", "[]", "5", JSON.stringify({ v: 2, messages: [mk(1)] }), JSON.stringify({ v: 1, messages: "x" }), JSON.stringify({ v: 1 })]) {
      const j = fakeStorage();
      j.data[KEY] = junk;
      check("a stored item that is not ours is ignored: " + junk.slice(0, 20), C.createStore(j, KEY, 300).all().length === 0);
    }
    // blocked storage: the messages last until the page closes
    const none = C.createStore(null, KEY, 300), blocked = C.createStore(fakeStorage(true), KEY, 300);
    none.add(mk(1)); blocked.add(mk(1));
    check("no storage: kept in memory, and says it is not saved", none.all().length === 1 && !none.isSaved());
    check("blocked storage: the same, no exception", blocked.all().length === 1 && !blocked.isSaved());
    // two pages of the site (two games) share one room
    const shared = fakeStorage(), a = C.createStore(shared, KEY, 300), b = C.createStore(shared, KEY, 300);
    a.add(mk(1)); b.add(mk(2)); a.add(mk(3));
    same("a write keeps what the other page wrote", a.all().map((m) => m.text), ["m1", "m2", "m3"]);
    same("and the other page sees the new ones", b.reload().map((m) => m.text), ["m3"]);
    same("(and has them all)", b.all().map((m) => m.text), ["m1", "m2", "m3"]);
    same("a second look finds nothing new", b.reload(), []);
  }

  // ---------- pages of history ----------
  {
    const list = [];
    for (let i = 0; i < 120; i++) list.push({ id: "id" + String(i).padStart(3, "0"), t: 5000 + i, user: "abc", text: "m" + i });
    const ids = (a) => a.map((m) => m.text);
    same("the newest 50", [ids(C.pageOf(list, 50, null))[0], ids(C.pageOf(list, 50, null)).length, ids(C.pageOf(list, 50, null))[49]], ["m70", 50, "m119"]);
    same("the 50 before m70", [ids(C.pageOf(list, 50, "id070"))[0], ids(C.pageOf(list, 50, "id070")).length, ids(C.pageOf(list, 50, "id070"))[49]], ["m20", 50, "m69"]);
    same("the start: fewer than asked", [ids(C.pageOf(list, 50, "id020")).length, ids(C.pageOf(list, 50, "id020"))[0]], [20, "m0"]);
    same("before the very first: none", C.pageOf(list, 50, "id000"), []);
    same("an id that is not there: none", C.pageOf(list, 50, "nope"), []);
    same("a short list", ids(C.pageOf(list.slice(0, 3), 50, null)), ["m0", "m1", "m2"]);
    same("an empty list", C.pageOf([], 50, null), []);
    check("the pages together are the list", [].concat(C.pageOf(list, 50, "id020"), C.pageOf(list, 50, "id070"), C.pageOf(list, 50, null)).length === 120);
    check("the page is 50", C.LIMITS.page === 50);
    check("same time, ids decide", C.byTime({ t: 1, id: "a" }, { t: 1, id: "b" }) < 0 && C.byTime({ t: 2, id: "a" }, { t: 1, id: "b" }) > 0 && C.byTime({ t: 1, id: "a" }, { t: 1, id: "a" }) === 0);
  }

  // ---------- the made-up seed ----------
  {
    const now = local(2026, 9, 30, 15, 0);
    const a = C.seedMessages(now), b = C.seedMessages(now);
    check("the same every time", JSON.stringify(a) === JSON.stringify(b));
    check("a few days' worth, over 100 messages", a.length > 100 && a.length < 200, a.length);
    const days = new Set(a.map((m) => C.dayKey(m.t)));
    check("five calendar days, today and the four before", days.size === 5 && days.has("2026-10-30") && days.has("2026-10-26"), Array.from(days));
    check("none is in the future", a.every((m) => m.t <= now - 60000));
    check("in time order, with unique ids", a.every((m, i) => i === 0 || C.byTime(a[i - 1], m) < 0) && new Set(a.map((m) => m.id)).size === a.length);
    check("each is a valid message", a.every((m) => C.validMessage(m)));
    check("only the made-up users and lines", a.every((m) => C.SEED_USERS.includes(m.user) && C.SEED_LINES.includes(m.text)));
    check("every made-up line is one clean line under the cap", C.SEED_LINES.length >= 40 && C.SEED_LINES.every((l) => C.cleanText(l) === l && l.length <= 500));
    check("more than a screenful of different speakers", new Set(a.map((m) => m.user)).size >= 8);
    const early = C.seedMessages(local(2026, 9, 30, 6, 0));
    check("early in the day, today has none yet", !early.some((m) => C.dayKey(m.t) === "2026-10-30") && early.length < a.length);
    const next = C.seedMessages(local(2026, 9, 31, 15, 0));
    check("the next day it has moved on a day", next.length === a.length && C.dayKey(next[next.length - 1].t) === "2026-10-31" && C.dayKey(next[0].t) === "2026-10-27");
    check("with no table of games, no message has a game", a.every((m) => m.game === undefined && m.tag === undefined && m.gameTitle === undefined));
    // with the table of the site's games: a spread of games
    const table = C.createGames(bigTable(18));
    const ga = C.seedMessages(now, table), gb = C.seedMessages(now, table);
    check("with games: the same every time", JSON.stringify(ga) === JSON.stringify(gb));
    check("with games: about the same count, valid, in time order", Math.abs(ga.length - a.length) <= 2 &&ga.every((m) => C.validMessage(m)) && ga.every((m, i) => i === 0 || C.byTime(ga[i - 1], m) < 0));
    check("with games: every message is from a game of the table, with its tag and title", ga.every((m) => { const g = table.get(m.game); return g && m.tag === g.tag && m.gameTitle === g.title; }));
    const used = new Set(ga.map((m) => m.game));
    check("with games: a spread, 12 of 18 games or more", used.size >= 12, used.size);
    check("with games: no game is more than a quarter of the room", Array.from(used).every((id) => ga.filter((m) => m.game === id).length <= ga.length / 4));
    const perUser = {};
    ga.forEach((m) => { (perUser[m.user] = perUser[m.user] || []).push(m.game); });
    const talkers = Object.values(perUser).filter((games) => games.length >= 8);
    check("with games: a user has a usual game (over 40% of their lines) but plays others too", talkers.length >= 6 && talkers.every((games) => {
      const counts = {};
      games.forEach((id) => { counts[id] = (counts[id] || 0) + 1; });
      return Object.keys(counts).length >= 2 && Math.max.apply(null, Object.values(counts)) / games.length > 0.4;
    }), talkers.map((games) => games.length));
    const gearly = C.seedMessages(local(2026, 9, 30, 6, 0), table);
    check("with games: earlier in the day is the same messages, without the later ones", gearly.length < ga.length && gearly.every((m) => JSON.stringify(ga.find((x) => x.id === m.id)) === JSON.stringify(m)));
    check("with three games: still every message from one of them", C.seedMessages(now, C.createGames(TABLE)).every((m) => TABLE.some((g) => g.id === m.game)));
    check("with one game: all from it", C.seedMessages(now, C.createGames(TABLE.slice(0, 1))).every((m) => m.game === "bookworm"));
  }

  // ---------- the local source ----------
  {
    let clock = local(2026, 9, 30, 15, 0), seq = 0;
    const timers = { set: [], cleared: [], set_(f, ms) { this.set.push({ f, ms }); return this.set.length; }, clear_(id) { this.cleared.push(id); } };
    const mkSource = (storage, over) => C.localSource(Object.assign({
      now: () => clock, random: () => ((++seq * 0.137) % 1), storage,
      timers: { set: (f, ms) => timers.set_(f, ms), clear: (id) => timers.clear_(id) },
    }, over || {}));
    const st = fakeStorage();
    const src = mkSource(st);
    const got = [], errors = [];
    const off = src.onMessage((m) => got.push(m));
    src.onError((e) => errors.push(e));
    const info = await src.connect();
    check("connect: the sender, with no sign-in yet", info.user.name === "test_player" && info.user.signedIn === false && info.canRename === true && info.testData === true, info);
    check("connect: says it is test data", /made up/.test(info.note) && src.testData === true);
    check("connect: the first message comes in 20 to 60 seconds", timers.set.length === 1 && timers.set[0].ms >= 20000 && timers.set[0].ms < 60000, timers.set.map((t) => t.ms));
    let page = await src.history(50, null);
    check("history: the newest 50, oldest first", page.length === 50 && page.every((m, i) => i === 0 || C.byTime(page[i - 1], m) < 0));
    const all = [];
    let before = null;
    for (let n = 0; n < 10; n++) {
      const p = await src.history(50, before);
      if (!p.length) break;
      all.unshift(...p);
      before = p[0].id;
      if (p.length < 50) break;
    }
    check("history: paging back reaches the start with no gap or repeat", all.length === C.seedMessages(clock).length && new Set(all.map((m) => m.id)).size === all.length, all.length);
    // sending
    let m1 = await src.send("  hello \n there ");
    check("send: the message, cleaned, with the viewer's name and the time", m1.text === "hello there" && m1.user === "test_player" && m1.t === clock && C.validMessage(m1), m1);
    check("send: the source tells its listeners (an echo)", got.length === 1 && got[0].id === m1.id);
    page = await src.history(50, null);
    check("history: the sent message is the newest", page[page.length - 1].id === m1.id);
    async function rejection(p) { try { await p; return null; } catch (e) { return e; } }
    let e = await rejection(src.send("again"));
    check("send: a second in the same instant is refused as too fast", e && e.code === "rate" && e.waitMs === 1000, e && e.code);
    clock += 400;
    e = await rejection(src.send("again"));
    check("send: 400 ms later it is still too fast, and the wait is counted from the last message", e && e.code === "rate" && e.waitMs === 600, e && e.waitMs);
    clock += 700;
    e = await rejection(src.send("   \n\t "));
    check("send: nothing to send", e && e.code === "empty");
    e = await rejection(src.send("x".repeat(501)));
    check("send: over the cap", e && e.code === "long");
    check("send: a refused message is not kept or announced", got.length === 1);
    clock += 1000;
    const m2 = await src.send("x".repeat(500));
    check("send: exactly 500 is fine, a second after the last one", m2.text.length === 500);
    // the name
    e = await rejection(src.setUsername("a..b"));
    check("setUsername: refused for the rules", e && e.code === "name" && /two periods/i.test(e.message), e && e.message);
    e = await rejection(src.setUsername("x"));
    check("setUsername: too short", e && e.code === "name");
    check("setUsername: a name is cleaned, then kept", (await src.setUsername("  Sam.Q_9 ")) === "sam.q_9" && st.data[C.KEYS.name] === "sam.q_9");
    clock += 1000;
    check("send: with the new name", (await src.send("hi")).user === "sam.q_9");
    // simulated messages
    clock += 60000;
    const sim = src.simulate();
    check("simulate: a message from another made-up user, announced and kept", sim.user !== "sam.q_9" && C.SEED_USERS.includes(sim.user) && got[got.length - 1].id === sim.id && C.validMessage(sim));
    const fire = timers.set[0].f;
    const before2 = got.length;
    fire();
    check("the timer posts a message and sets the next timer", got.length === before2 + 1 && timers.set.length === 2 && timers.set[1].ms >= 20000, timers.set.length);
    // a reload keeps what was sent
    const again = mkSource(st, { auto: false });
    const info2 = await again.connect();
    check("reload: the name is kept", info2.user.name === "sam.q_9");
    const newest = await again.history(50, null);
    const texts = newest.map((m) => m.text);
    check("reload: the sent messages are still the newest", texts.includes("hello there") && texts.includes("hi") && texts.includes("x".repeat(500)) && newest.some((m) => m.id === m1.id));
    check("reload: auto false sets no timer", timers.set.length === 2);
    const heard = got.length;
    off();
    src.simulate();
    check("a stopped listener hears no more", got.length === heard);
    src.close();
    check("close clears the timer", timers.cleared.length >= 1);
    e = await rejection(src.send("after close"));
    check("send after close: offline", e && e.code === "offline");
    // the cap over a long time
    const busy = mkSource(fakeStorage(), { auto: false });
    await busy.connect();
    for (let i = 0; i < 340; i++) { clock += 1000; await busy.send("n" + i); }
    const sentNow = (await busy.history(500, null)).filter((m) => m.user === "test_player");
    check("the kept messages are capped at 300", sentNow.length === 300 && sentNow[0].text === "n40" && sentNow[299].text === "n339", sentNow.length);
    // storage that will not keep
    const dead = mkSource(fakeStorage(true), { auto: false });
    const deadErrors = [];
    dead.onError((x) => deadErrors.push(x));
    const di = await dead.connect();
    clock += 1000;
    await dead.send("one");
    clock += 1000;
    await dead.send("two");
    check("blocked storage: the name is the default, sending works, and it says so once", di.user.name === "test_player" && deadErrors.length === 1 && deadErrors[0].code === "storage" && !deadErrors[0].fatal, deadErrors);
    check("blocked storage: the messages are there until the page closes", (await dead.history(500, null)).filter((m) => m.text === "one" || m.text === "two").length === 2);
    // a stored name that is not valid is not used
    const badName = fakeStorage();
    badName.data[C.KEYS.name] = "Bad Name";
    check("a stored name that breaks the rules is not used", (await mkSource(badName, { auto: false }).connect()).user.name === "test_player");
    // another page wrote to the room: its message comes in as a new one
    const sa = fakeStorage(), storageHandlers = [];
    global.window = { addEventListener(t, f) { if (t === "storage") storageHandlers.push(f); }, removeEventListener(t, f) { const i = storageHandlers.indexOf(f); if (i >= 0) storageHandlers.splice(i, 1); } };
    const pa = mkSource(sa, { auto: false }), pb = mkSource(sa, { auto: false });
    const heardByB = [];
    pb.onMessage((m) => heardByB.push(m.text));
    await pa.connect(); await pb.connect();
    check("connect listens for the other pages' writes", storageHandlers.length === 2);
    clock += 1000;
    await pa.send("from page a");
    check("a page that has not heard of it yet", heardByB.length === 0);
    storageHandlers.forEach((f) => f({ key: "something-else" }));
    check("a write to another key is not ours", heardByB.length === 0);
    storageHandlers.forEach((f) => f({ key: C.KEYS.messages }));
    same("the storage event brings the other page's message in as a new one, once", heardByB, ["from page a"]);
    storageHandlers.forEach((f) => f({ key: C.KEYS.messages }));
    check("and not again", heardByB.length === 1);
    pb.close();
    check("close stops listening", storageHandlers.length === 1);
    const pc = mkSource(sa, { auto: false });
    await pc.connect();
    check("a page that opens later has it in its history", (await pc.history(50, null)).some((m) => m.text === "from page a"));
    pa.close(); pc.close();
    delete global.window;
  }

  // ---------- the local source and the game it is playing ----------
  {
    let clock = local(2026, 9, 30, 15, 0), seq = 0;
    const timers = { set: () => 1, clear: () => {} };
    const mkSource = (storage, over) => C.localSource(Object.assign({ now: () => clock, random: () => ((++seq * 0.137) % 1), storage, timers, auto: false }, over || {}));
    const shared = fakeStorage();
    const a = mkSource(shared, { game: "sevenseas", games: TABLE });
    await a.connect();
    const m = await a.send("hello from one game");
    check("send: carries the page's game, its initials and its title", m.game === "sevenseas" && m.tag === "7S" && m.gameTitle === "Seven Seas Deluxe" && C.validMessage(m), m);
    const stored = JSON.parse(shared.data[C.KEYS.messages]).messages.find((x) => x.id === m.id);
    check("the stored message has them too", stored && stored.game === "sevenseas" && stored.tag === "7S" && stored.gameTitle === "Seven Seas Deluxe", stored);
    clock += 1500;
    const b = mkSource(shared, { game: "bookworm", games: C.createGames(TABLE, "bookworm") });
    await b.connect();
    const hist = await b.history(500, null);
    const seen = hist.find((x) => x.id === m.id);
    check("another game's page has the message, still from the first game", seen && seen.game === "sevenseas" && seen.tag === "7S", seen);
    const mb = await b.send("and a reply from another game");
    check("and sends its own game", mb.game === "bookworm" && mb.tag === "BW" && mb.gameTitle === "BookWorm Deluxe");
    check("the seeded history is from games too", hist.filter((x) => /-s\d{4}$/.test(x.id)).length > 100 &&hist.filter((x) => x.game && TABLE.some((g) => g.id === x.game)).length > 100);
    clock += 1500;
    const c = mkSource(fakeStorage(), { game: "mystery_game", games: TABLE });
    await c.connect();
    const mc = await c.send("from a game the site does not know");
    check("a page whose game the table lacks: the id only, no tag or title", mc.game === "mystery_game" && mc.tag === undefined && mc.gameTitle === undefined && C.validMessage(mc), mc);
    const d = mkSource(fakeStorage(), { game: "Bad Id!", games: TABLE });
    await d.connect();
    const md = await d.send("from a page with a bad game id");
    check("a bad game id is not put on the message", md.game === undefined && md.tag === undefined && C.validMessage(md), md);
    const e = mkSource(fakeStorage());
    await e.connect();
    const me = await e.send("no game at all");
    check("no game, no table: no game fields", me.game === undefined && me.tag === undefined && me.gameTitle === undefined, me);
    check("no table: the seed has no games", (await e.history(500, null)).every((x) => x.game === undefined));
    const f = mkSource(fakeStorage(), { game: "bookworm", games: TABLE, auto: false });
    await f.connect();
    const sims = []; for (let i = 0; i < 24; i++) sims.push(f.simulate());
    check("simulate: messages from a spread of the table's games, each with its tag", new Set(sims.map((x) => x.game)).size === 3 && sims.every((x) => { const g = TABLE.find((y) => y.id === x.game); return g && x.tag === g.tag && x.gameTitle === g.title; }), sims.map((x) => x.game));
    const g0 = mkSource(fakeStorage());
    await g0.connect();
    check("simulate with no table: no game", g0.simulate().game === undefined);
    a.close(); b.close(); c.close(); d.close(); e.close(); f.close(); g0.close();
  }

  // ---------- the message put on the page ----------
  // A DOM that refuses to take markup: innerHTML and its kin throw, and every element made is recorded.
  {
    const made = [];
    class Node {
      constructor(tag) { this.tag = tag; this.children = []; this.attrs = {}; this.className = ""; this.style = { props: {}, setProperty(k, v) { this.props[k] = v; } }; }
      appendChild(c) { this.children.push(c); return c; }
      setAttribute(k, v) { this.attrs[k] = String(v); }
      get textContent() { return this.tag === "#text" ? this.value : this.children.map((c) => c.textContent).join(""); }
      set textContent(v) { this.children = v === "" ? [] : [Object.assign(new Node("#text"), { value: String(v) })]; }
      set innerHTML(v) { throw new Error("innerHTML is never used for a message"); }
      set outerHTML(v) { throw new Error("outerHTML is never used for a message"); }
      insertAdjacentHTML() { throw new Error("insertAdjacentHTML is never used for a message"); }
    }
    const doc = {
      createElement(tag) { made.push(tag); return new Node(tag); },
      createTextNode(v) { return Object.assign(new Node("#text"), { value: String(v) }); },
    };
    const hostile = [
      "<script>alert(1)</script>", "<img src=x onerror=alert(1)>", "<b>bold</b> &amp; &lt;i&gt;", "javascript:alert(1)", "https://example.com/a?b=1&c=2#d", "www.example.com",
      "[link](https://example.com)", "**bold** _it_ `code`", '"quote" \'single\' `tick`', "<a href=\"https://example.com\">x</a>", "😀 café", "<!-- c -->", "</li><li>injected",
      "{{7*7}} ${7*7} %s", "<style>*{display:none}</style>",
    ];
    for (const text of hostile) {
      const m = { id: "i", t: t0, user: "abc", text };
      const li = C.renderRow(doc, m, "someone_else");
      check("the row's text is exactly the IRC line: " + text.slice(0, 24), li.textContent === C.formatLine(m), li.textContent);
    }
    check("only li, time and span were made, so no link, image, script or style", made.length > 0 && made.every((t) => t === "li" || t === "time" || t === "span"), Array.from(new Set(made)));
    const li = C.renderRow(doc, { id: "i", t: t0, user: "abc", text: "x" }, "abc");
    check("the viewer's own line is marked", /bpc-me/.test(li.className) && !/bpc-me/.test(C.renderRow(doc, { id: "i", t: t0, user: "abc", text: "x" }, "zzz").className));
    const time = li.children[0];
    check("the time is a time element with its full date on hover", time.tag === "time" && time.textContent === "09:05" && time.attrs.title === C.formatFull(t0) && time.attrs.datetime === new Date(t0).toISOString(), time.attrs);
    check("the angle brackets are for the eye only", li.children[2].children[0].attrs["aria-hidden"] === "true" && li.children[2].children[2].attrs["aria-hidden"] === "true");
    check("the nick has its hue", li.children[2].style.props["--bpc-h"] === String(C.hueOf("abc")));
    const day = C.renderDay(doc, t0, noon);
    check("a day separator is its words", day.textContent === "Today, Friday 30 October 2026" && /bpc-day/.test(day.className), day.textContent);
    // ---------- the game tag: the one link ----------
    const games = C.createGames(TABLE, "bookworm");
    const linkOf = (li) => li.children.find((c) => c.tag === "a");
    const madeBefore = made.filter((t) => t === "a").length;
    {
      const m = { id: "i", t: t0, user: "alice", text: "hi", game: "sevenseas", tag: "7S", gameTitle: "Seven Seas Deluxe" };
      const li = C.renderRow(doc, m, "bob", games), a = linkOf(li);
      check("a row from a known game has the tag in its line", li.textContent === C.formatLine(m, games) && li.textContent === "09:05 [7S] <alice> hi", li.textContent);
      check("the tag is a link to the game's page", a && a.tag === "a" && a.attrs.href === "sevenseas.html", a && a.attrs);
      check("its hover text is the game's full title", a.attrs.title === "Seven Seas Deluxe");
      check("its name holds the full title and the initials it shows", a.attrs["aria-label"] === "Seven Seas Deluxe (7S)");
      const visible = a.children.filter((c) => c.attrs["aria-hidden"] !== "true").map((c) => c.textContent).join("");
      const norm = (x) => x.toLowerCase().replace(/[^a-z0-9 ]/g, "");
      check("the name contains the words the link shows (label in name)", visible === "7S" && norm(a.attrs["aria-label"]).indexOf(norm(visible)) >= 0, visible);
      check("the brackets are for the eye only", a.children[0].attrs["aria-hidden"] === "true" && a.children[0].textContent === "[" && a.children[2].attrs["aria-hidden"] === "true" && a.children[2].textContent === "]");
      check("it opens in the same tab: no target, no rel", a.attrs.target === undefined && a.attrs.rel === undefined && a.attrs.download === undefined);
      check("its colour is the badge's, and its class is the tag's", a.style.props["--bpc-c"] === "#1e8080" && /bpc-game/.test(a.className));
      check("it is not the current page's game here", a.attrs["aria-current"] === undefined);
      check("the order is time, tag, name, text", li.children[0].tag === "time" && li.children[2] === a && li.children[4].children[1].textContent === "alice" && li.children[6].textContent === "hi");
      const own = linkOf(C.renderRow(doc, { id: "i", t: t0, user: "alice", text: "hi", game: "bookworm" }, "bob", games));
      check("the page's own game is marked as the current page", own.attrs["aria-current"] === "page" && own.attrs.href === "bookworm.html" && own.textContent === "[BW]");
      check("a title with an apostrophe goes in as it is", linkOf(C.renderRow(doc, { id: "i", t: t0, user: "alice", text: "hi", game: "noahsark" }, "bob", games)).attrs.title === "Noah's Ark Deluxe");
    }
    {
      // what the message says about its tag is never drawn: the table is the site's
      const forged = { id: "i", t: t0, user: "alice", text: "hi", game: "bookworm", tag: "ADMIN", gameTitle: "<b>Official</b>" };
      const li = C.renderRow(doc, forged, "bob", games), a = linkOf(li);
      check("a forged tag and title are not drawn: the table's are", a.textContent === "[BW]" && a.attrs.title === "BookWorm Deluxe" && li.textContent.indexOf("ADMIN") < 0 && li.textContent.indexOf("Official") < 0, li.textContent);
      // a game the site does not know has no tag and no link
      for (const unknown of [{ game: "oldgame", tag: "OG", gameTitle: "Old Game" }, { game: "javascript:alert(1)", tag: "JS" }, { game: "constructor" }, { game: "__proto__" }, { game: 5 }, { game: null }, { game: { id: "bookworm" } }, { game: ["bookworm"] }, { tag: "BW", gameTitle: "BookWorm Deluxe" }, {}]) {
        const m = Object.assign({ id: "i", t: t0, user: "alice", text: "hi" }, unknown);
        const row = C.renderRow(doc, m, "bob", games);
        check("no tag and no link for " + JSON.stringify(unknown), !linkOf(row) && row.textContent === "09:05 <alice> hi", row.textContent);
      }
      const noTable = C.renderRow(doc, { id: "i", t: t0, user: "alice", text: "hi", game: "bookworm" }, "bob");
      check("no table: no tag and no link", !linkOf(noTable) && noTable.textContent === "09:05 <alice> hi");
      check("an entry the table refused has no link", !linkOf(C.renderRow(doc, { id: "i", t: t0, user: "alice", text: "hi", game: "bookworm" }, "bob", C.createGames([Object.assign({}, TABLE[0], { page: "javascript:alert(1)" })]))));
      // the text of the message is still only text, with a tag in front of it
      for (const text of hostile) {
        const m = { id: "i", t: t0, user: "abc", text, game: "bookworm" };
        const row = C.renderRow(doc, m, "bob", games);
        check("with a tag, the row's text is exactly the IRC line: " + text.slice(0, 20), row.textContent === C.formatLine(m, games) && row.children.filter((c) => c.tag === "a").length === 1, row.textContent);
      }
    }
    check("a link was made for each row with a known game and for no other", made.filter((t) => t === "a").length - madeBefore === 4 + hostile.length, made.filter((t) => t === "a").length - madeBefore);
    check("only li, time, span and the tag's a were made", made.every((t) => t === "li" || t === "time" || t === "span" || t === "a"), Array.from(new Set(made)));
    // the source of the file: no way to write markup, no way to reach the network
    const src = fs.readFileSync(path.join(__dirname, "..", "web", "chat.js"), "utf8")
      .replace(/\/\*[\s\S]*?\*\//g, "").split("\n").map((l) => l.replace(/^\s*\/\/.*$/, "").replace(/\s\/\/\s.*$/, "")).join("\n");
    for (const bad of ["innerHTML", "outerHTML", "insertAdjacentHTML", "document.write", "createContextualFragment", "DOMParser", "srcdoc", "eval(", "new Function", "createElement(\"img\")", "make(\"a\"", "make(\"img\"", "location", "window.open", "\"target\"", "\"rel\""]) {
      check("chat.js never uses " + bad, src.indexOf(bad) < 0);
    }
    const times = (needle) => src.split(needle).length - 1;
    check("the one link is made in one place, with the table's page as its address", times("createElement(\"a\")") === 1 && times("setAttribute(\"href\"") === 1 && src.indexOf("setAttribute(\"href\", g.page)") >= 0);
    check("nothing else is made into an address or a script source", times("href") === 1 && times(".src") === 0 && times("setAttribute(\"src\"") === 0);
    check("the tag's text is white, on the badge's colour", /\.bpc-game \{[^}]*background: var\(--bpc-c[^}]*color: #fff;/.test(src));
    for (const net of ["fetch(", "XMLHttpRequest", "WebSocket", "EventSource", "sendBeacon", "new Image", "importScripts", "navigator.", "location.", "indexedDB", "caches."]) {
      check("chat.js makes no network request: no " + net, src.indexOf(net) < 0);
    }
  }

  // ---------- the site's table of games: the badge table, and what the build wrote ----------
  {
    const root = path.join(__dirname, "..");
    const cmake = fs.readFileSync(path.join(root, "cmake", "ByopopWeb.cmake"), "utf8");
    const block = (/set\(BYOPOP_WEB_BADGES([\s\S]*?)\)\r?\n/.exec(cmake) || [])[1] || "";
    const rows = Array.from(block.matchAll(/"([^"|]+)\|([^"|]+)\|(#[0-9a-fA-F]{6})"/g)).map((x) => ({ target: x[1], tag: x[2], colour: x[3] }));
    const finished = ((/set\(BYOPOP_WEB_FINISHED([\s\S]*?)\)\r?\n/.exec(cmake) || [])[1] || "").split(/\s+/).filter(Boolean);
    check("the badge table is read: at least the 18 games it had", rows.length >= 18, rows.length);
    check("every finished game has a badge row", finished.length >= 18 && finished.every((t) => rows.some((r) => r.target === t)), finished.filter((t) => !rows.some((r) => r.target === t)));
    check("the initials are unique and fit a tag", new Set(rows.map((r) => r.tag)).size === rows.length && rows.every((r) => /^[A-Za-z0-9]{1,4}$/.test(r.tag)), rows.map((r) => r.tag));
    check("the targets are unique", new Set(rows.map((r) => r.target)).size === rows.length);
    const low = rows.filter((r) => contrast(r.colour, "#ffffff") < 4.5);
    check("every badge colour keeps white text above 4.5 to 1, so a tag does in both themes", low.length === 0, low);
    // what the build wrote beside the pages (cmake/ByopopWeb.cmake, byopop_web_index): the same games as the listing page's tiles
    const dir = process.env.BYOPOP_WEB_DIR || path.join(root, "build", "web");
    if (fs.existsSync(path.join(dir, "index.html"))) {
      const file = path.join(dir, "chat-games.js");
      check("the web build has chat-games.js (configure the web build again if not)", fs.existsSync(file));
      if (fs.existsSync(file)) {
        let list = null;
        try { list = vm.runInNewContext(fs.readFileSync(file, "utf8") + "\nByopopGames;", {}); } catch (err) { list = null; }
        check("chat-games.js is a list", Array.isArray(list) && list.length >= 1, list);
        if (Array.isArray(list)) {
          const table = C.createGames(list);
          check("the page accepts every entry", table.all().length === list.length, list.length - table.all().length);
          const index = fs.readFileSync(path.join(dir, "index.html"), "utf8");
          const unescape = (x) => x.replace(/&quot;/g, "\"").replace(/&lt;/g, "<").replace(/&gt;/g, ">").replace(/&amp;/g, "&");
          const tiles = Array.from(index.matchAll(/<li class="game" style="--tile: (#[0-9a-fA-F]{6})"[^>]*data-title="([^"]*)"><a class="tile" href="([^"]+)" data-game="([^"]*)"[^>]*><span class="badge" aria-hidden="true">([^<]*)<\/span>/g))
            .map((x) => ({ colour: x[1], title: unescape(x[2]), page: x[3], id: x[4], tag: unescape(x[5]) }));
          check("one entry for each tile of the listing page, in its order", tiles.length === list.length && tiles.length >= 1, [tiles.length, list.length]);
          same("the entries are the tiles': id, page, initials, colour and title", list.map((g) => ({ id: g.id, page: g.page, tag: g.tag, colour: g.colour, title: g.title })), tiles.map((x) => ({ id: x.id, page: x.page, tag: x.tag, colour: x.colour, title: x.title })));
          check("each page is its game's target, whose badge row gave the initials and colour", list.every((g) => { const r = rows.find((x) => x.target + ".html" === g.page); return r && r.tag === g.tag && r.colour.toLowerCase() === g.colour.toLowerCase(); }));
        }
      }
    } else {
      console.log("chattest: no web build at " + dir + ", so the generated table was not compared");
    }
  }

  console.log("chattest: " + checks + " checks, " + failed + " failed");
  process.exit(failed ? 1 : 0);
})();
