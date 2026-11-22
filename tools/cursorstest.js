// Checks the site's cursor (web/cursors.js; docs/WEB_PLAYER.md, "The site's cursor") under Node, in a stand-in page: which of a
// game's cursors are kept (the six kinds the site shows, none whose image means something else in that game, frames only when
// well-formed), the rules each context gets (the pointer, hand and text always, the circle-slash on disabled controls and the wait
// cursor while the site is busy only from a set that has them), busy() and the frames of an animated cursor (swapped on the
// element under the mouse while the mouse stays still, and given back when it leaves; no timer for a set without frames), and the
// Settings' choice, where games whose sets look the same share one entry: in every kind but the text cursor, cropped pictures
// that differ by nothing anyone can see (colours a few steps off, a hot spot a pixel away), sets kept before images were
// cropped included, and where an animated cursor's picture plays its frames while the mouse is over its row. Also where the
// sets are kept: in the database, their pictures as PNGs with a palette where they have few colours (every pixel as it was) and
// an animated cursor's frames as one strip; in localStorage only their titles and signatures and the chosen set, which the next
// page shows from its first line; the sets kept in localStorage before moving into the database. A set that cannot be kept is
// named in the Settings' choice, not dropped unseen; the games named there as needing to be opened once; and the list of games
// that give a set against their sources.
//   node tools/cursorstest.js
// It prints "cursorstest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const zlib = require("zlib");

const source = fs.readFileSync(path.join(__dirname, "..", "web", "cursors.js"), "utf8");

function classList() {
  const set = new Set();
  return { add: (c) => set.add(c), remove: (c) => set.delete(c), contains: (c) => set.has(c),
    toggle: (c, on) => { if (on === undefined) on = !set.has(c); if (on) set.add(c); else set.delete(c); return on; } };
}
// An element: its tag, the selectors it matches and its inline style (inline: each property with its priority), with own as its
// inline cursor (SDL's on a game's canvas)
function element(tag, matches, own) {
  const style = {}, priority = {};
  const el = { nodeType: 1, tagName: tag, matches: (sel) => (matches || []).indexOf(sel) >= 0, getAttribute: () => null,
    style: { setProperty: (k, v, p) => { style[k] = v; priority[k] = p || ""; }, removeProperty: (k) => { delete style[k]; delete priority[k]; },
      getPropertyValue: (k) => style[k] || "", getPropertyPriority: (k) => priority[k] || "" } };
  Object.defineProperty(el, "inline", { get() {
    const o = {};
    for (const k in style) o[k] = style[k] + (priority[k] ? " !" + priority[k] : "");
    return o;
  } });
  if (own) el.style.setProperty("cursor", own);
  return el;
}
// A node of the Settings' choice: its children, attributes, style and listeners
function node(tag) {
  const n = { tagName: tag, children: [], listeners: {}, attrs: {}, id: "", className: "", style: {},
    setAttribute(k, v) { this.attrs[k] = String(v); },
    appendChild(c) { this.children.push(c); return c; },
    addEventListener(t, f) { (this.listeners[t] = this.listeners[t] || []).push(f); },
    fire(t) { (this.listeners[t] || []).forEach((f) => f({})); },
    // the text a screen reader would read for it
    text() { return this.children.map((c) => c.tagName === "#text" ? c.text : c.text()).join("") + (this._text || ""); },
    // the kinds of the cursors it pictures, in order
    kinds() { return this.children.flatMap((c) => c.tagName === "img" ? [c.attrs["data-kind"]] : c.kinds ? c.kinds() : []); } };
  Object.defineProperty(n, "textContent", { get() { return n._text || ""; }, set(v) { n._text = v; n.children = []; } });
  return n;
}
const STRINGS = { "cursors.browser": "Default", "site.list_two": "{first} and {second}", "site.list_many": "{rest} and {last}",
  "cursors.hint": "hint", "cursors.hint_none": "none", "cursors.hint_open": "Open {games} ({n})", "cursors.unkept": "{games} not kept ({n})" };

// ---------- the pictures ----------
// The stand-in codec: the browser's own "PNG" is the pixels as JSON (enc); a real PNG, as web/cursors.js writes one with a
// palette, is read by png() below; deflate is zlib's
function enc(px) { return "data:image/png;base64," + Buffer.from(JSON.stringify([px.width, px.height, Array.from(px.data)])).toString("base64"); }
function dec(url) {
  if (typeof url !== "string" || url.indexOf("data:image/png;base64,") !== 0) return null;
  const b = Buffer.from(url.slice(22), "base64");
  if (b[0] === 137 && b[1] === 80) return png(b);
  try { const [width, height, data] = JSON.parse(b.toString()); return { width, height, data: new Uint8ClampedArray(data) }; } catch (e) { return null; }
}
// a PNG with a palette (colour type 3) at 1, 2, 4 or 8 bits, its rows unfiltered, every chunk's CRC checked; null otherwise
function png(b) {
  let pos = 8, w = 0, h = 0, bits = 0, type = -1, plte = null, trns = Buffer.alloc(0);
  const idat = [];
  while (pos + 12 <= b.length) {
    const len = b.readUInt32BE(pos), t = b.toString("latin1", pos + 4, pos + 8), d = b.subarray(pos + 8, pos + 8 + len);
    if (zlib.crc32(b.subarray(pos + 4, pos + 8 + len)) !== b.readUInt32BE(pos + 8 + len)) return null;
    if (t === "IHDR") { w = d.readUInt32BE(0); h = d.readUInt32BE(4); bits = d[8]; type = d[9]; }
    else if (t === "PLTE") plte = d;
    else if (t === "tRNS") trns = d;
    else if (t === "IDAT") idat.push(d);
    pos += 12 + len;
  }
  if (type !== 3 || !plte) return null;
  const raw = zlib.inflateSync(Buffer.concat(idat)), stride = Math.ceil(w * bits / 8) + 1, data = new Uint8ClampedArray(w * h * 4);
  for (let y = 0; y < h; y++) {
    if (raw[y * stride] !== 0) return null;
    for (let x = 0; x < w; x++) {
      const bit = x * bits, v = (raw[y * stride + 1 + (bit >> 3)] >> (8 - bits - (bit & 7))) & ((1 << bits) - 1);
      data.set([plte[v * 3], plte[v * 3 + 1], plte[v * 3 + 2], v < trns.length ? trns[v] : 255], (y * w + x) * 4);
    }
  }
  return { width: w, height: h, data };
}
// what a picture's URL shows: its size and pixels
function pixels(url) { const px = dec(url); return px ? [px.width, px.height, Array.from(px.data)].join() : null; }
function urlOf(bytes) { return "data:image/png;base64," + Buffer.from(bytes).toString("base64"); }
// A picture named n: (x + 2) by (y + 2) pixels in one colour of its name, its hot spot at (x, y). The name's hash is mixed
// (MurmurHash3's finish) so that names a letter apart get colours far apart, which do not look alike.
function img(n, x, y) {
  x = x || 0; y = y || 0;
  let h = 0x811c9dc5;
  for (const ch of n) h = Math.imul(h ^ ch.charCodeAt(0), 16777619) >>> 0;
  h ^= h >>> 16; h = Math.imul(h, 0x85ebca6b); h ^= h >>> 13; h = Math.imul(h, 0xc2b2ae35); h = (h ^ (h >>> 16)) >>> 0;
  const w = x + 2, t = y + 2, data = new Uint8ClampedArray(w * t * 4);
  for (let i = 0; i < w * t; i++) data.set([h & 255, (h >>> 8) & 255, (h >>> 16) & 255, 255], i * 4);
  return { url: enc({ width: w, height: t, data }), x, y };
}
const anim = (first, rest, ms) => Object.assign(img(first), { frames: rest.map((n) => img(n)), ms });

// ---------- the page ----------
// store: localStorage's items; extra: more of the page's globals (ByopopGameFiles, ByopopImport). opts.records: the database's
// records by id (shared between pages, as one browser's); opts.full: the database will not store; opts.hold: the page has not
// finished loading until ready() (what it shows from its first line)
function page(store, extra, opts) {
  opts = opts || {};
  const listeners = {}, timers = new Map(), held = [];
  let nextTimer = 1, holding = true;
  const root = { classList: classList() };
  const head = { children: [], appendChild(e) { this.children.push(e); } };
  const document = {
    documentElement: root, head, visibilityState: "visible",
    createElement: (t) => node(t),
    createTextNode: (text) => ({ tagName: "#text", text }),
    getElementById: () => null,
    addEventListener: (t, f) => { (listeners[t] = listeners[t] || []).push(f); }
  };
  const local = Object.assign({}, store || {});
  const ls = { getItem: (k) => (k in local ? local[k] : null), setItem: (k, v) => { local[k] = String(v); }, removeItem: (k) => { delete local[k]; } };
  const ctx = Object.assign({
    document, JSON, Object, Array, String, btoa, atob,
    ByopopStrings: { t: (k, f) => (STRINGS[k] || k).replace(/\{(\w+)\}/g, (m, n) => f[n]) },
    window: { localStorage: ls, addEventListener: () => {} },
    setInterval: (f, ms) => { timers.set(nextTimer, { f, ms }); return nextTimer++; },
    clearInterval: (id) => { timers.delete(id); },
    setTimeout: (f) => { if (holding) held.push(f); else f(); return 0; }
  }, extra || {});
  vm.createContext(ctx);
  vm.runInContext(source, ctx, { filename: "cursors.js" });
  const C = vm.runInContext("ByopopCursors", ctx), records = opts.records || {}, vaultOpts = { full: !!opts.full };
  C.codec.encode = enc;
  C.codec.decode = (url, done) => done(dec(url));
  C.codec.deflate = (bytes, done) => done(zlib.deflateSync(Buffer.from(bytes)));
  C.vault.all = (done) => done(Object.assign({}, records));
  C.vault.get = (id, done) => (records[id] ? done(records[id]) : done(null, true));
  C.vault.put = (id, rec, done) => { if (vaultOpts.full) { done(false); return; } records[id] = rec; done(true); };
  C.vault.remove = (id, done) => { delete records[id]; done(true); };
  function ready() { holding = false; held.splice(0).forEach((f) => f()); }
  if (!opts.hold) ready();
  return {
    C, root, local, ls, timers, records, vaultOpts, ready,
    css: () => (head.children.find((c) => c.id === "byopop-site-cursor") || { textContent: "" }).textContent,
    fire(t, ev) { (listeners[t] || []).forEach((f) => f(ev)); },
    tick() { [...timers.values()].forEach((t) => t.f()); }
  };
}
// a cursor value as the site's rules and the frames write it
const cur = (c, fallback) => 'url("' + c.url + '") ' + c.x + " " + c.y + ", " + fallback;
const ON = ":root.site-cursor:not(.game-cursor)", BUSY = ":root.site-cursor.site-busy:not(.game-cursor)";
// the rule of the site's cursor that starts so
const rule = (css, start) => css.split("\n").find((l) => l.indexOf(start) === 0) || "";

let checks = 0, failed = 0;
function check(what, got, want) {
  checks++;
  const g = JSON.stringify(got), w = JSON.stringify(want);
  if (g !== w) { failed++; console.log(`FAIL ${what}: got ${g}, want ${w}`); }
}

// which cursors are kept
{
  const p = page();
  const full = { pointer: img("p"), hand: img("h"), drag: img("d"), text: img("t"), notallowed: img("n", 3, 4), wait: img("w"), sizeall: img("s") };
  check("a set is kept", p.C.save("bejeweled", "Bejeweled", full), true);
  check("the six kinds the site shows, nothing else", Object.keys(p.C.all().bejeweled), ["title", "pointer", "hand", "drag", "text", "notallowed", "wait"]);
  const na = p.C.all().bejeweled.notallowed;
  check("each picture as it was given, with its hot spot", [pixels(na.url), na.x, na.y], [pixels(full.notallowed.url), 3, 4]);
  check("the same set given again is not kept again", p.C.save("bejeweled", "Bejeweled", full), false);
  p.C.save("sevenseas", "Seven Seas", full);
  check("Seven Seas' shovel is not the circle-slash", p.C.all().sevenseas.notallowed, undefined);
  check("its other kinds are kept", !!p.C.all().sevenseas.wait, true);
  check("no set without a pointer", p.C.save("tiptop", "TipTop", { hand: img("h") }), false);
  p.C.save("bananabugs", "Banana Bugs", { pointer: anim("p0", ["p1", "p2"], 100) });
  const a = p.C.all().bananabugs.pointer;
  check("an animated cursor keeps its frames, at their pace", [[a].concat(a.frames).map((f) => pixels(f.url)), a.ms],
    [["p0", "p1", "p2"].map((n) => pixels(img(n).url)), 100]);
  for (const [what, bad] of [["too many frames", { frames: Array.from({ length: 40 }, (_, i) => img("f" + i)), ms: 100 }],
    ["a frame that is not a PNG", { frames: [{ url: "javascript:x", x: 0, y: 0 }], ms: 100 }],
    ["no time", { frames: [img("f")] }], ["too fast", { frames: [img("f")], ms: 5 }], ["no frames", { frames: [], ms: 100 }]]) {
    const q = page();
    q.C.save("bananabugs", "Banana Bugs", { pointer: img("p"), hand: Object.assign(img("h"), bad) });
    check("a cursor with " + what + " is not kept", q.C.all().bananabugs.hand, undefined);
  }
}
// the rules for each context
{
  const p = page();
  p.C.save("bejeweled", "Bejeweled", { pointer: img("p"), hand: img("h"), text: img("t") });
  p.C.choose("bejeweled");
  const css = p.css(), b = p.C.all().bejeweled;
  check("chosen: the page's class", p.root.classList.contains("site-cursor"), true);
  check("the pointer, hand and text rules", [css.indexOf(cur(b.pointer, "auto")) >= 0, rule(css, ON + " a[href]").indexOf("{ cursor: " + cur(b.hand, "pointer")) >= 0,
    css.indexOf(cur(b.text, "text")) >= 0], [true, true, true]);
  check("no circle-slash or wait rule without their images", [css.indexOf(":disabled {") >= 0 || css.indexOf("[aria-disabled=true] {") >= 0, css.indexOf("site-busy") >= 0], [false, false]);
  check("a disabled button is not given the hand", /button:not\(:disabled\)/.test(css), true);
  p.C.save("alchemy", "Alchemy", { pointer: img("p"), notallowed: img("n", 2, 2), wait: img("w", 5, 6) });
  p.C.choose("alchemy");
  const css2 = p.css(), al = p.C.all().alchemy;
  check("the circle-slash on disabled controls", rule(css2, ON + " button:disabled").indexOf("[aria-disabled=true] { cursor: " + cur(al.notallowed, "not-allowed") + " !important; }") >= 0, true);
  check("the wait cursor while busy, under the dragging rule", [rule(css2, BUSY).indexOf("{ cursor: " + cur(al.wait, "wait")) >= 0 && al.wait.x === 5 && al.wait.y === 6,
    css2.indexOf("site-busy") < css2.indexOf(".badge[draggable=true]:active")], [true, true]);
  p.C.choose("");
  check("the browser's own: no class, no rules", [p.root.classList.contains("site-cursor"), p.css()], [false, ""]);
}
// busy()
{
  const p = page();
  p.C.busy("saves", true);
  p.C.busy("addgames", true);
  p.C.busy("saves", false);
  check("busy while any work has not ended", p.root.classList.contains("site-busy"), true);
  p.C.busy("addgames", false);
  p.C.busy("addgames", false);
  check("not once all have ended", p.root.classList.contains("site-busy"), false);
}
// animated cursors
{
  const p = page();
  p.C.save("plain", "Plain", { pointer: img("p"), hand: img("h") });
  p.C.choose("plain");
  const el = element("DIV");
  p.fire("mouseover", { target: el });
  check("a set without frames starts no timer", [p.timers.size, el.inline], [0, {}]);

  p.C.save("bananabugs", "Banana Bugs", { pointer: anim("p0", ["p1"], 120), hand: img("h"), wait: anim("w0", ["w1", "w2"], 80) });
  p.C.choose("bananabugs");
  const bb = p.C.all().bananabugs, P0 = cur(bb.pointer, "auto") + " !important", P1 = cur(bb.pointer.frames[0], "auto") + " !important";
  p.fire("mouseover", { target: el });
  check("the pointer's first frame on the element under the mouse", el.inline.cursor, P0);
  check("one timer, at the frames' pace", [...p.timers.values()].map((t) => t.ms), [120]);
  p.tick();
  check("the next frame with the mouse still", el.inline.cursor, P1);
  p.tick();
  check("and round again", el.inline.cursor, P0);
  const link = element("A", ["a[href]"]);
  p.fire("mouseover", { target: link });
  check("onto a link, whose hand is still: the element given back, no timer", [el.inline, link.inline, p.timers.size], [{}, {}, 0]);
  p.fire("mouseover", { target: el });
  p.C.busy("addgames", true);
  check("busy: the wait cursor's frames", [el.inline.cursor, [...p.timers.values()].map((t) => t.ms)], [cur(bb.wait, "wait") + " !important", [80]]);
  p.C.busy("addgames", false);
  check("busy no more: the pointer's again", el.inline.cursor, P0);
  p.root.classList.add("game-cursor");
  p.fire("mouseover", { target: el });
  check("a game's own cursor showing: nothing animated", [el.inline, p.timers.size], [{}, 0]);
  p.root.classList.remove("game-cursor");
  p.fire("mouseover", { target: el });
  p.fire("mouseout", { target: el, relatedTarget: null });
  check("the mouse leaves the page: given back, no timer", [el.inline, p.timers.size], [{}, 0]);
}
// animated cursors over a game's canvas: never over a game that shows its own cursor (the user, 2026-11-16: "The Cosmic Bugs
// cursor shows duplicated over the game cursor")
{
  const p = page();
  p.C.save("cosmicbugs", "Cosmic Bugs", { pointer: anim("p0", ["p1", "p2"], 30) });
  p.C.choose("cosmicbugs");
  const frame0 = cur(p.C.all().cosmicbugs.pointer, "auto") + " !important";
  const hidden = element("CANVAS", [], "none");
  p.fire("mouseover", { target: hidden });
  check("a canvas whose game hides the browser's cursor (drawing its own): nothing animated", [hidden.inline, p.timers.size], [{ cursor: "none" }, 0]);
  const canvas = element("CANVAS", [], "default");
  p.fire("mouseover", { target: canvas });
  check("a canvas showing the browser's cursor shows the frames, 30 ms each", [canvas.inline.cursor, [...p.timers.values()].map((t) => t.ms)], [frame0, [30]]);
  // the game hides the browser's cursor to draw its own, as SDL does (no priority)
  canvas.style.setProperty("cursor", "none");
  p.tick();
  check("the game hides it: the frames stop at the next one, and the game's none stays", [canvas.inline, p.timers.size], [{ cursor: "none" }, 0]);
  canvas.style.setProperty("cursor", "default");
  p.fire("mousemove", { target: canvas });
  check("the game shows it again: the frames again as the mouse moves", [canvas.inline.cursor, p.timers.size], [frame0, 1]);
  p.root.classList.add("game-cursor");
  p.tick();
  check("the game's own cursor shows on the page: the frames stop, and the canvas has its own cursor back", [canvas.inline, p.timers.size],
    [{ cursor: "default" }, 0]);
  p.root.classList.remove("game-cursor");
  p.fire("mousemove", { target: canvas });
  p.fire("mouseout", { target: canvas, relatedTarget: null });
  check("the mouse leaves the page: the canvas has its own cursor back", [canvas.inline, p.timers.size], [{ cursor: "default" }, 0]);
  p.C.save("waterbugs", "Water Bugs", { pointer: anim("w0", ["w1"], 50) });
  p.fire("mouseover", { target: canvas });
  p.C.choose("waterbugs");
  check("another animated set chosen under a still mouse: its own frames at its own pace",
    [canvas.inline.cursor, [...p.timers.values()].map((t) => t.ms)], [cur(p.C.all().waterbugs.pointer, "auto") + " !important", [50]]);
  p.C.choose("cosmicbugs");
  const plain = element("CANVAS");
  p.fire("mouseover", { target: plain });
  p.fire("mouseover", { target: element("DIV") });
  check("an element with no cursor of its own is left with none", plain.inline, {});
}
// with the page's changes watched (a stand-in MutationObserver whose records the test delivers): the frames stop as the game's
// cursor comes on, before the next frame of the swap
{
  const observers = [];
  class Watcher { constructor(cb) { this.cb = cb; this.on = []; observers.push(this); } observe(t) { this.on.push(t); } disconnect() { this.on = []; } }
  const deliver = () => observers.forEach((o) => { if (o.on.length) o.cb([]); });
  const p = page(null, { MutationObserver: Watcher });
  p.C.save("cosmicbugs", "Cosmic Bugs", { pointer: anim("p0", ["p1", "p2"], 30) });
  p.C.choose("cosmicbugs");
  const canvas = element("CANVAS", [], "default");
  p.fire("mouseover", { target: canvas });
  check("the page's classes and the element are watched while the frames show", observers[0].on.length, 2);
  canvas.style.setProperty("cursor", "none");
  deliver();
  check("SDL hides the browser's cursor: the frames stop with no frame of the swap between", [canvas.inline, p.timers.size, observers[0].on.length],
    [{ cursor: "none" }, 0, 0]);
  canvas.style.setProperty("cursor", "default");
  p.fire("mousemove", { target: canvas });
  p.root.classList.add("game-cursor");
  deliver();
  check("the game's cursor comes on: the same", [canvas.inline, p.timers.size], [{ cursor: "default" }, 0]);
}

// ---------- where the sets are kept ----------
// localStorage keeps no pictures but the chosen set's, which the next page shows from its first line; the database the rest
{
  const records = {};
  const p = page(null, null, { records });
  for (let i = 0; i < 18; i++) p.C.save("game" + i, "Game " + i, { pointer: img("g" + i, 3, 3), hand: img("h" + i) });
  p.C.save("waterbugs", "Water Bugs", { pointer: anim("w0", ["w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8", "w9"], 50) });
  p.C.choose("game3");
  check("every set is in the database", Object.keys(records).length, 19);
  check("localStorage keeps the sets' titles and signatures, and the chosen set", Object.keys(p.local).sort(),
    ["byopop-cursor-shown", "byopop-cursors-kept", "byopop-site-cursor"]);
  const index = JSON.parse(p.local["byopop-cursors-kept"]);
  check("each set there by its title and signature alone", [Object.keys(index).length, Object.values(index).every((v) => Object.keys(v).join() === "title,sig")], [19, true]);
  const shown = JSON.parse(p.local["byopop-cursor-shown"]), g3 = p.C.all().game3;
  check("the chosen set as the site shows it", [shown.id, shown.set.pointer.url === g3.pointer.url, shown.set.hand.url === g3.hand.url], ["game3", true, true]);
  const q = page(Object.assign({}, p.local), null, { records, hold: true });
  check("the next page shows the chosen set from its first line, before it reads the database",
    [q.root.classList.contains("site-cursor"), q.css().indexOf(cur(g3.pointer, "auto")) >= 0], [true, true]);
  q.ready();
  check("it reads no other set until the Settings' choice shows them all", Object.keys(q.C.all()), ["game3"]);
  check("which reads them all", entryNames(q).length, 20);
  p.C.choose("game4");
  p.C.forget("game4");
  check("the chosen game's files forgotten: its set and its copy go", [!!records.game4, "game4" in JSON.parse(p.local["byopop-cursors-kept"]),
    "byopop-cursor-shown" in p.local, p.css()], [false, false, false, ""]);
  p.C.choose("game5");
  p.C.choose("");
  check("no set chosen: no copy of one", "byopop-cursor-shown" in p.local, false);
}
// PNGs with a palette where a picture has few colours: every pixel as it was, at the fewest bits a pixel
{
  const records = {};
  const p = page(null, null, { records });
  const c = canvas(20, 20, 2, 3, 200);
  c.data.set([90, 90, 90, 9], (4 * 20 + 5) * 4);
  const pic = p.C.picture(c, 2, 3);
  p.C.save("few", "Few Colours", { pointer: pic });
  const b = Buffer.from(records.few.kinds.pointer.png);
  check("a picture of few colours is kept as a PNG with a palette", [b.subarray(1, 4).toString(), b[25]], ["PNG", 3]);
  check("every pixel as it was, the see-through ones too", pixels(urlOf(b)), pixels(pic.url));
  check("and shown so", pixels(p.C.all().few.pointer.url), pixels(pic.url));
  const bits = (n) => {
    const data = new Uint8ClampedArray(16 * 16 * 4);
    for (let i = 0; i < 256; i++) data.set([(i % n) * 13 & 255, (i % n) * 7 & 255, i % n, 255], i * 4);
    p.C.save("c" + n, "Colours", { pointer: { url: enc({ width: 16, height: 16, data }), x: 0, y: 0 } });
    return Buffer.from(records["c" + n].kinds.pointer.png)[24];
  };
  check("at 1, 2, 4 or 8 bits a pixel", [2, 3, 5, 17, 200].map(bits), [1, 2, 4, 8, 8]);
  const data = new Uint8ClampedArray(20 * 20 * 4);
  for (let i = 0; i < 400; i++) data.set([i & 255, i >> 8, 7, 255], i * 4);
  p.C.save("many", "Many Colours", { pointer: { url: enc({ width: 20, height: 20, data }), x: 0, y: 0 } });
  check("a picture of more than 256 colours is kept as the browser writes it", Buffer.from(records.many.kinds.pointer.png).toString()[0], "[");
}
// an animated cursor's frames as one strip: cells of one size, each frame placed by its hot spot; read back as each was
{
  const records = {};
  const p = page(null, null, { records });
  const f = [p.C.picture(canvas(30, 30, 4, 4, 200), 4, 4), p.C.picture(canvas(30, 30, 6, 2, 120), 7, 3), p.C.picture(canvas(40, 40, 1, 9, 60), 1, 9)];
  const want = f.map((q) => [pixels(q.url), q.x, q.y, q.k]);
  p.C.save("anim", "Animated", { pointer: Object.assign({}, f[0], { frames: [f[1], f[2]], ms: 60 }) });
  const s = records.anim.kinds.pointer, strip = dec(urlOf(s.png));
  check("an animated cursor's frames are kept as one strip of cells of one size", [s.n, strip.width === s.w * 3, s.ks, s.ms], [3, true, f.map((q) => q.k), 60]);
  const a = p.C.all().anim.pointer;
  check("and read back as each frame was, with its own hot spot", [a].concat(a.frames).map((q) => [pixels(q.url), q.x, q.y, q.k]), want);
}
// a set the browser will not store
{
  const p = page(null, null, { full: true });
  p.C.save("alchemy", "Alchemy", { pointer: img("a") });
  check("a set the browser will not store is not kept, and is noted as unkept, as the Settings' choice says",
    [Object.keys(p.C.all()), p.C.unkept(), hints(p)], [[], { alchemy: "Alchemy" }, ["Alchemy not kept (1)", "none"]]);
  p.vaultOpts.full = false;
  check("kept once the browser stores it, and no longer noted", [p.C.save("alchemy", "Alchemy", { pointer: img("a") }), Object.keys(p.C.all()), p.C.unkept(),
    "byopop-cursors-unkept" in p.local], [true, ["alchemy"], {}, false]);
  p.vaultOpts.full = true;
  p.C.save("bigmoney", "Big Money", { pointer: img("b") });
  p.C.forget("bigmoney");
  check("forgetting a game's files forgets that its set was not kept", p.C.unkept(), {});
}

// ---------- the sets kept in localStorage before the database ----------
// Water Bugs' and Cosmic Bugs' sets kept with 10 ms updates are put right as they move: the games step once a millisecond
{
  const old = (ms) => ({ title: "", pointer: anim("p0", ["p1"], ms) });
  const kept = { waterbugs: old(500), cosmicbugs: old(300), bananabugs: old(500) };
  const p = page({ "byopop-cursors": JSON.stringify(kept) });
  const a = p.C.all();
  check("moved into the database: Water Bugs' 500 ms is 50, Cosmic Bugs' 300 is 30, another game's set is left",
    [a.waterbugs.pointer.ms, a.cosmicbugs.pointer.ms, a.bananabugs.pointer.ms, Object.keys(p.records).sort()], [50, 30, 500, ["bananabugs", "cosmicbugs", "waterbugs"]]);
  check("and their old item is gone", "byopop-cursors" in p.local, false);
  const q = page();
  q.C.save("waterbugs", "Water Bugs", { pointer: anim("p0", ["p1"], 20) });
  q.C.save("cosmicbugs", "Cosmic Bugs", { pointer: anim("p0", ["p1"], 19) });
  check("frames of 20 ms are kept, of 19 not", [!!q.C.all().waterbugs, !!q.C.all().cosmicbugs], [true, false]);
}
// the chosen one shows from the old item while they move
{
  const old = { heavyweapon: { title: "Heavy Weapon", pointer: img("h") } };
  const p = page({ "byopop-cursors": JSON.stringify(old), "byopop-site-cursor": "heavyweapon" }, null, { hold: true });
  check("the chosen set shows from the old item before it moves", p.css().indexOf(cur(old.heavyweapon.pointer, "auto")) >= 0, true);
  p.ready();
  const h = p.C.all().heavyweapon;
  check("and from the database once moved, kept for the next page", [p.css().indexOf(cur(h.pointer, "auto")) >= 0, JSON.parse(p.local["byopop-cursor-shown"]).id,
    "byopop-cursors" in p.local], [true, "heavyweapon", false]);
  const q = page({ "byopop-cursors": JSON.stringify(old) }, null, { full: true });
  check("an old set the database will not take stays in the old item, for the next page to move", [q.C.unkept(), "byopop-cursors" in q.local],
    [{ heavyweapon: "Heavy Weapon" }, true]);
}
// Bonnie's set, taken out at the user's word (2026-11-17), does not move, and a choice of it goes; others move
{
  const set = (n) => ({ title: n, pointer: img(n) });
  const p = page({ "byopop-cursors": JSON.stringify({ bonnies: set("b"), heavyweapon: set("h") }), "byopop-site-cursor": "bonnies",
    "byopop-cursors-unkept": JSON.stringify({ bonnies: "Bonnie's Bookstore" }) });
  check("Bonnie's set dropped, Heavy Weapon's kept", Object.keys(p.C.all()), ["heavyweapon"]);
  check("and the choice of it gone: the browser's cursor", [p.C.chosen(), "byopop-site-cursor" in p.local], ["", false]);
  check("and not noted as unkept", "byopop-cursors-unkept" in p.local, false);
  check("and never kept again", p.C.save("bonnies", "Bonnie's Bookstore", { pointer: img("b2") }), false);
  const q = page({ "byopop-cursors": JSON.stringify({ heavyweapon: set("h") }), "byopop-site-cursor": "heavyweapon" });
  check("another choice stays", [q.C.chosen(), Object.keys(q.C.all())], ["heavyweapon", ["heavyweapon"]]);
}

// the Settings' choice: games with the same set share one entry
function entryNames(p) {
  const el = node("div");
  p.C.render(el);
  return el.children[0].children[1].children.filter((l) => l.tagName === "label").map((l) => l.text().trim());
}
{
  const p = page();
  const look = { pointer: img("p", 1, 2), hand: img("h"), drag: img("d"), text: img("t") };
  p.C.save("bejeweled", "Bejeweled", look);
  p.C.save("alchemy", "Alchemy", look);
  p.C.save("atomica", "Atomica", look);
  p.C.save("bigmoney", "Big Money", Object.assign({}, look, { hand: img("h3") }));
  p.C.save("bookworm", "BookWorm", Object.assign({}, look, { pointer: img("p", 1, 3) }));
  function entries() {
    const el = node("div");
    p.C.render(el);
    const box = el.children[0].children[1];
    return box.children.filter((l) => l.tagName === "label").map((l) => ({ value: l.children[0].value, checked: l.children[0].checked, name: l.text().trim() }));
  }
  check("identical sets make one entry, named with every title in order; a set that differs in one kind or one hot spot keeps its own",
    entries().map((e) => e.name), ["Default", "Alchemy, Atomica and Bejeweled", "Big Money", "BookWorm"]);
  check("nothing chosen: the browser's own is checked", entries().filter((e) => e.checked).map((e) => e.name), ["Default"]);
  p.C.choose("bejeweled");
  check("a choice of any member selects its group, and keeps that member", [entries().filter((e) => e.checked).map((e) => e.name), entries()[1].value],
    [["Alchemy, Atomica and Bejeweled"], "bejeweled"]);
  p.C.choose("alchemy");
  check("the first member too", entries().filter((e) => e.checked).map((e) => e.value), ["alchemy"]);
  p.C.forget("atomica");
  check("forgetting one game leaves the rest of its group", entries()[1].name, "Alchemy and Bejeweled");
  p.C.save("alchemy", "Alchemy", Object.assign({}, look, { hand: img("h2") }));
  check("a set that changes leaves its group, and the choice stays with it",
    entries().map((e) => [e.name, e.checked]), [["Default", false], ["Alchemy", true], ["Bejeweled", false], ["Big Money", false], ["BookWorm", false]]);
  check("the sets stay one a game", Object.keys(p.C.all()).sort(), ["alchemy", "bejeweled", "bigmoney", "bookworm"]);
}
// each entry pictures every cursor its set has, in order, and nothing for one it lacks
{
  const p = page();
  p.C.save("all", "All Six", { wait: img("w"), notallowed: img("n"), text: img("t"), drag: img("d"), hand: img("h"), pointer: anim("p0", ["p1"], 100) });
  p.C.save("one", "Just A Pointer", { pointer: img("q") });
  p.C.save("gaps", "Some Gaps", { pointer: img("g"), text: img("gt"), wait: img("gw") });
  p.C.save("text", "Only Text", { pointer: img("o"), text: img("ot") });
  const el = node("div");
  p.C.render(el);
  const labels = el.children[0].children[1].children.filter((l) => l.tagName === "label");
  check("the kinds each entry pictures", labels.map((l) => [l.text().trim(), l.kinds()]),
    [["Default", []], ["All Six", ["pointer", "hand", "drag", "notallowed", "wait"]], ["Just A Pointer", ["pointer"]], ["Only Text", ["pointer"]],
      ["Some Gaps", ["pointer", "wait"]]]);
  const pics = labels[1].children[1].children;
  check("an animated cursor by its first frame, every picture decorative", [pics[0].children[0].src, [pics[0].children[0]].concat(pics.slice(1)).every((i) => i.alt === "")],
    [p.C.all().all.pointer.url, true]);
}
// an animated cursor's picture plays its frames while the mouse is over its row (the user, 2026-11-16: "Add mouse over for
// animated custom cursors to show the animation"), laid on each other by their hot spots
{
  const p = page();
  p.C.save("anim", "Animated", { pointer: Object.assign(img("a0", 1, 1), { frames: [img("a1", 2, 1), img("a2")], ms: 70 }), hand: img("h") });
  const el = node("div");
  p.C.render(el);
  const row = el.children[0].children[1].children.filter((l) => l.tagName === "label")[1], box = row.children[1].children[0], pic = box.children[0];
  const a = p.C.all().anim.pointer, at = () => [pic.src === a.url ? 0 : pic.src === a.frames[0].url ? 1 : pic.src === a.frames[1].url ? 2 : -1, pic.style.left, pic.style.top];
  check("its first frame in a box that holds every frame by its hot spot, and no timer", [at(), box.style.width, box.style.height, p.timers.size],
    [[0, "1px", "0px"], "4px", "3px", 0]);
  row.fire("mouseenter");
  check("the mouse over its row: a timer at the cursor's pace", [...p.timers.values()].map((t) => t.ms), [70]);
  p.tick();
  const second = at();
  p.tick();
  check("each frame in turn, placed by its hot spot", [second, at()], [[1, "0px", "0px"], [2, "2px", "1px"]]);
  row.fire("mouseleave");
  check("the mouse leaves: the first frame again, no timer", [at(), p.timers.size], [[0, "1px", "0px"], 0]);
  row.fire("mouseenter");
  p.C.refresh();
  check("drawn again: the old row's timer stops", p.timers.size, 0);
}
// a canvas of w by h with a design drawn at (ox, oy): an arrow-ish shape in a colour, with a half-transparent shadow unless
// `bare` (as Bejeweled's dragging hand has none: its mask is another size, so the game leaves it out)
function canvas(w, h, ox, oy, colour, bare) {
  const data = new Uint8ClampedArray(w * h * 4);
  const put = (x, y, r, g, b, a) => data.set([r, g, b, a], ((oy + y) * w + ox + x) * 4);
  for (let y = 0; y < 6; y++) for (let x = 0; x <= y; x++) put(x, y, colour, 20, 30, 255);
  if (!bare) put(1, 6, 0, 0, 0, 128);
  return { width: w, height: h, data };
}
// Pictures compared after cropping
{
  const p = page();
  // Bejeweled's 54 by 54 and Alchemy's 37 by 37: the same design, its hot spot on its tip
  const big = p.C.picture(canvas(54, 54, 9, 11, 200), 9, 11), small = p.C.picture(canvas(37, 37, 2, 3, 200), 2, 3);
  check("the same design on two canvases keeps one picture and hot spot", [small.url === big.url, small.k === big.k, big.x, big.y], [true, true, 0, 0]);
  const cut = p.C.crop(canvas(54, 54, 9, 11, 200), 12, 13);
  check("a crop to the pixels that show, the hot spot taken from its corner", [cut.width, cut.height, cut.x, cut.y], [6, 7, 3, 2]);
  const out = p.C.crop(canvas(54, 54, 9, 11, 200), 20, 2);
  check("a hot spot outside them stays in the crop", [out.width, out.height, out.x, out.y], [12, 16, 11, 0]);
  const other = p.C.picture(canvas(37, 37, 2, 3, 100), 2, 3);
  check("another colour, another hash", other.k !== big.k, true);
}
{
  const p = page();
  const pic = (w, ox, colour, hx, bare) => p.C.picture(canvas(w, w, ox, ox, colour, bare), ox + (hx || 0), ox);
  const set = (colours, text) => ({ pointer: pic(54, 9, colours[0]), hand: pic(37, 2, colours[1]), drag: pic(40, 5, colours[2]), text });
  p.C.save("alchemy", "Alchemy", set([200, 20, 30], pic(37, 1, 77)));
  p.C.save("bigmoney", "Big Money", set([200, 20, 30], pic(54, 20, 99)));
  // BookWorm's pointer: Alchemy's, its colours up to three steps off
  p.C.save("bookworm", "BookWorm", set([197, 20, 30], pic(37, 1, 77)));
  // Bejeweled Twist's: Alchemy's design kept as JPEG 2000, a few pixels up to 33 off, and a faint edge (alpha 9) from its masks;
  // its hand as the decoder gives it now has a stray pixel or two 47 off
  const twist = (w, ox, colour, strays) => {
    const c = canvas(w, w, ox, ox, colour), at = (x, y) => ((ox + y) * w + ox + x) * 4;
    c.data[at(2, 4)] += 33; c.data.set([90, 90, 90, 9], at(3, 1)); c.data.set([0, 0, 0, 9], at(7, 6));
    (strays || []).forEach((s) => { c.data[at(s[0], s[1])] += s[2]; });
    return p.C.picture(c, ox, ox);
  };
  const strayHand = (strays) => twist(37, 2, 20, strays);
  p.C.save("bejeweledtwist", "Bejeweled Twist", Object.assign(set([200, 20, 30], pic(37, 1, 77)), { pointer: twist(54, 9, 200), hand: twist(58, 11, 20, [[0, 5, 47]]) }));
  // too many pixels off, or one too far off, is another picture
  p.C.save("many", "Many Pixels Off", Object.assign(set([200, 20, 30], pic(37, 1, 77)), { hand: strayHand([[0, 1, 47], [0, 2, 47], [1, 2, 47], [0, 3, 47]]) }));
  p.C.save("far", "One Pixel Far Off", Object.assign(set([200, 20, 30], pic(37, 1, 77)), { hand: strayHand([[0, 5, 90]]) }));
  p.C.save("near", "Hot Spot A Pixel Off", Object.assign(set([200, 20, 30], pic(37, 1, 77)), { hand: pic(37, 2, 20, 1) }));
  // Atomica's are green
  p.C.save("p", "Pointer Differs", set([90, 20, 30], pic(37, 1, 77)));
  p.C.save("h", "Hand Differs", set([200, 120, 30], pic(37, 1, 77)));
  // Bejeweled's dragging hand: an open one without the shadow
  p.C.save("bejeweled", "Bejeweled", Object.assign(set([200, 20, 30], pic(37, 1, 77)), { drag: pic(54, 9, 30, 0, true) }));
  p.C.save("bare", "Hand Without Shadow", Object.assign(set([200, 20, 30], pic(37, 1, 77)), { hand: pic(37, 2, 20, 0, true) }));
  p.C.save("hot", "Hot Spot Three Pixels Off", Object.assign(set([200, 20, 30], pic(37, 1, 77)), { hand: pic(37, 2, 20, 3) }));
  check("sets that differ only in their text or dragging cursor, by colours a few steps off, by compression's few pixels and " +
    "faint edges or by a hot spot a pixel away share " +
    "an entry; another colour, a missing shadow or a hot spot further away in the pointer or hand keeps a set apart", entryNames(p),
    ["Default", "Alchemy, Bejeweled, Bejeweled Twist, Big Money, BookWorm and Hot Spot A Pixel Off", "Hand Differs",
      "Hand Without Shadow", "Hot Spot Three Pixels Off", "Many Pixels Off", "One Pixel Far Off", "Pointer Differs"]);
  const dragPic = () => { const el = node("div"); p.C.render(el);
    return el.children[0].children[1].children[1].children[1].children.find((i) => i.attrs["data-kind"] === "drag").src; };
  check("the entry pictures the dragging cursor of the first by title", dragPic(), p.C.all().alchemy.drag.url);
  p.C.choose("bejeweled");
  check("or of the member chosen, whose own shows on the site", [dragPic(), p.css().indexOf(p.C.all().bejeweled.drag.url) >= 0],
    [p.C.all().bejeweled.drag.url, true]);
  p.C.choose("bigmoney");
  const bej = p.C.all().bigmoney.text.url, alc = p.C.all().alchemy.text.url;
  check("a member's own text cursor shows in text fields", [p.css().indexOf(bej) >= 0, p.css().indexOf(alc) >= 0], [true, false]);
}
{
  // Alchemy's set as the site kept it before images were cropped: whole canvases, no hash; Bejeweled's taken since
  const raw = (w, ox, colour) => ({ url: enc(canvas(w, w, ox, ox, colour)), x: ox, y: ox });
  const kept = { alchemy: { title: "Alchemy", pointer: raw(37, 2, 10), hand: raw(37, 2, 20), drag: raw(37, 2, 30), text: raw(37, 4, 40) } };
  const p = page({ "byopop-cursors": JSON.stringify(kept) });
  const pic = (w, ox, colour) => p.C.picture(canvas(w, w, ox, ox, colour), ox, ox);
  p.C.save("bejeweled", "Bejeweled", { pointer: pic(54, 9, 10), hand: pic(54, 9, 20), drag: pic(54, 9, 30), text: pic(54, 9, 40) });
  check("an old set moves cropped, and shares the entry of the same set taken since", entryNames(p), ["Default", "Alchemy and Bejeweled"]);
  const a = p.C.all();
  check("its pictures cropped, with their hot spots from the crop", [a.alchemy.hand.url === a.bejeweled.hand.url, a.alchemy.hand.k === a.bejeweled.hand.k,
    a.alchemy.hand.x, a.alchemy.hand.y, a.alchemy.text.k === a.bejeweled.text.k], [true, true, 0, 0, true]);
  p.C.choose("alchemy");
  check("the site's cursors are drawn from the cropped images", p.css().indexOf(a.alchemy.pointer.url) >= 0, true);
  check("the same set given by its game later is not kept again", p.C.save("alchemy", "Alchemy",
    { pointer: pic(37, 2, 10), hand: pic(37, 2, 20), drag: pic(37, 2, 30), text: pic(37, 4, 40) }), false);
}
// the hints under the choice
function hints(p) {
  const el = node("div");
  p.C.render(el);
  return el.children[0].children[1].children.filter((c) => c.tagName === "p").map((c) => c.text());
}
// the games named as needing to be opened once: those that give a set whose files are here, with no set kept or noted as unkept
{
  const records = { waterbugs: { loaded: 1 }, bejeweled: { loaded: 1 }, ningpo: { loaded: 1 }, cosmicbugs: { loaded: 1, forgotten: 2 }, alchemy: { loaded: 1 },
    rocketmania: { loaded: 1 } };
  const titles = { waterbugs: "Water Bugs", bejeweled: "Bejeweled", ningpo: "Ning Po", cosmicbugs: "Cosmic Bugs", alchemy: "Alchemy", rocketmania: "Rocket Mania",
    astropop: "AstroPop" };
  const files = { all: () => records, present: (r) => !!(r && r.loaded && !r.forgotten) };
  const games = Object.keys(titles).map((id) => ({ target: id, title: titles[id], spec: { id } }));
  const p = page({ "byopop-cursors-unkept": JSON.stringify({ alchemy: "Alchemy" }) }, { ByopopGameFiles: files, ByopopImport: { games } });
  p.C.save("bejeweled", "Bejeweled", { pointer: img("b") });
  check("named: files here and no set; not a game that gives none, forgotten files, no files, a set kept or one noted as unkept",
    hints(p), ["Alchemy not kept (1)", "Open Rocket Mania and Water Bugs (2)"]);
  p.C.save("waterbugs", "Water Bugs", { pointer: img("w") });
  p.C.save("alchemy", "Alchemy", { pointer: img("a") });
  check("each leaves the hint once its set is kept", hints(p), ["Open Rocket Mania (1)"]);
  p.C.save("rocketmania", "Rocket Mania", { pointer: img("r") });
  check("the plain hint once none is left", hints(p), ["hint"]);
  const own = page(null, { ByopopGameFiles: files });
  check("a game's own page, which knows no titles, names none", hints(own), ["none"]);
}
// GIVES against the sources: the games whose port gives the framework an image for one of the set's kinds (SetCursorImage with a
// kind other than CURSOR_CUSTOM or CURSOR_NONE, by name or number, or WebCursorSetKind), and those web/flicursor.js reads a pointer for
{
  const games = path.join(__dirname, "..", "games");
  const found = new Set(Object.keys(require(path.join(__dirname, "..", "web", "flicursor.js")).GAMES));
  const KIND = /\bSetCursorImage\s*\(\s*(?:Sexy::)?(?:CURSOR_(?!CUSTOM\b|NONE\b)[A-Z_]+|(?:[0-4]|10)\s*,)|\bWebCursorSetKind\s*\(/;
  const walk = (dir, id) => {
    for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
      const at = path.join(dir, e.name);
      if (e.isDirectory()) walk(at, id);
      else if (/\.(cpp|h)$/i.test(e.name) && KIND.test(fs.readFileSync(at, "utf8"))) found.add(id);
    }
  };
  for (const id of fs.readdirSync(games)) if (fs.statSync(path.join(games, id)).isDirectory()) walk(path.join(games, id), id);
  check("the games that give a set are those whose sources give one", [...page().C.GIVES], [...found].sort());
}

console.log(`cursorstest: ${checks} checks, ${failed} failed`);
process.exit(failed ? 1 : 0);
