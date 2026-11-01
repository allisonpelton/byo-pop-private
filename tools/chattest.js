// Checks the chat's rules and its local test source (web/chat.js, docs/CHAT_DESIGN.md) under Node:
//   node tools/chattest.js
// The username rules (Discord's: 2 to 32 characters, lowercase letters, digits, "_" and ".", never two periods in a row), the text of
// a message (one line, 500 characters, nothing that hides or reorders text), the rate limit, the line, time and day formats, the
// history (the stored messages: their cap, their checks on the way back in, two pages sharing one room, blocked storage; the made-up
// seed; paging back with "before"), the local source (send, history, the rate limit, the name, the messages "received", a reload
// that keeps what was sent), and that a message is put on the page as text and nothing else (a fake DOM that refuses markup, and a
// look at the source for the ways of writing markup or reaching the network).
// It prints "chattest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
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
    // the source of the file: no way to write markup, no way to reach the network
    const src = fs.readFileSync(path.join(__dirname, "..", "web", "chat.js"), "utf8")
      .replace(/\/\*[\s\S]*?\*\//g, "").split("\n").map((l) => l.replace(/^\s*\/\/.*$/, "").replace(/\s\/\/\s.*$/, "")).join("\n");
    for (const bad of ["innerHTML", "outerHTML", "insertAdjacentHTML", "document.write", "createContextualFragment", "DOMParser", "srcdoc", "eval(", "new Function", "setAttribute(\"href\"", "createElement(\"a\")", "createElement(\"img\")", "make(\"a\"", "make(\"img\""]) {
      check("chat.js never uses " + bad, src.indexOf(bad) < 0);
    }
    for (const net of ["fetch(", "XMLHttpRequest", "WebSocket", "EventSource", "sendBeacon", "new Image", "importScripts", "navigator.", "location.", "indexedDB", "caches."]) {
      check("chat.js makes no network request: no " + net, src.indexOf(net) < 0);
    }
  }

  console.log("chattest: " + checks + " checks, " + failed + " failed");
  process.exit(failed ? 1 : 0);
})();
