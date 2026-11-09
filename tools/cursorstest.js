// Checks the site's cursor (web/cursors.js; docs/WEB_PLAYER.md, "The site's cursor") under Node, in a stand-in page: which of a
// game's cursors are kept (the six kinds the site shows, none whose image means something else in that game, frames only when
// well-formed), the rules each context gets (the pointer, hand and text always, the circle-slash on disabled controls and the wait
// cursor while the site is busy only from a set that has them), busy() and the frames of an animated cursor (swapped on the
// element under the mouse while the mouse stays still, and given back when it leaves; no timer for a set without frames), and the
// Settings' choice, where games whose sets look the same share one entry: in every kind but the text cursor, cropped pictures
// that differ by nothing anyone can see (colours a few steps off, a hot spot a pixel away), sets kept before images were
// cropped included.
//   node tools/cursorstest.js
// It prints "cursorstest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const source = fs.readFileSync(path.join(__dirname, "..", "web", "cursors.js"), "utf8");

function classList() {
  const set = new Set();
  return { add: (c) => set.add(c), remove: (c) => set.delete(c), contains: (c) => set.has(c),
    toggle: (c, on) => { if (on === undefined) on = !set.has(c); if (on) set.add(c); else set.delete(c); return on; } };
}
// An element: its tag, the selectors it matches and its inline style
function element(tag, matches) {
  const style = {};
  return { nodeType: 1, tagName: tag, matches: (sel) => (matches || []).indexOf(sel) >= 0, getAttribute: () => null,
    style: { setProperty: (k, v, p) => { style[k] = v + (p ? " !" + p : ""); }, removeProperty: (k) => { delete style[k]; } }, inline: style };
}
// A node of the Settings' choice: its children, attributes and listeners
function node(tag) {
  const n = { tagName: tag, children: [], listeners: {}, attrs: {}, id: "", className: "",
    setAttribute(k, v) { this.attrs[k] = String(v); },
    appendChild(c) { this.children.push(c); return c; },
    addEventListener(t, f) { (this.listeners[t] = this.listeners[t] || []).push(f); },
    // the text a screen reader would read for it
    text() { return this.children.map((c) => c.tagName === "#text" ? c.text : c.text()).join("") + (this._text || ""); },
    // the kinds of the cursors it pictures, in order
    kinds() { return this.children.flatMap((c) => c.tagName === "img" ? [c.attrs["data-kind"]] : c.kinds ? c.kinds() : []); } };
  Object.defineProperty(n, "textContent", { get() { return n._text || ""; }, set(v) { n._text = v; n.children = []; } });
  return n;
}
const STRINGS = { "cursors.browser": "Default", "site.list_two": "{first} and {second}", "site.list_many": "{rest} and {last}" };
function page(store) {
  const listeners = {}, timers = new Map();
  let nextTimer = 1;
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
  const ctx = {
    document, JSON, Object, Array, String,
    ByopopStrings: { t: (k, f) => (STRINGS[k] || k).replace(/\{(\w+)\}/g, (m, n) => f[n]) },
    window: { localStorage: { getItem: (k) => (k in local ? local[k] : null), setItem: (k, v) => { local[k] = String(v); }, removeItem: (k) => { delete local[k]; } },
      addEventListener: () => {} },
    setInterval: (f, ms) => { timers.set(nextTimer, { f, ms }); return nextTimer++; },
    clearInterval: (id) => { timers.delete(id); },
    setTimeout: (f) => { f(); return 0; }
  };
  vm.createContext(ctx);
  vm.runInContext(source, ctx, { filename: "cursors.js" });
  return {
    C: vm.runInContext("ByopopCursors", ctx), root, local, timers,
    css: () => (head.children.find((c) => c.id === "byopop-site-cursor") || { textContent: "" }).textContent,
    fire(t, ev) { (listeners[t] || []).forEach((f) => f(ev)); },
    tick() { [...timers.values()].forEach((t) => t.f()); }
  };
}
const img = (n, x, y) => ({ url: "data:image/png;base64,AAAA" + n, x: x || 0, y: y || 0 });

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
  p.C.save("sevenseas", "Seven Seas", full);
  check("Seven Seas' shovel is not the circle-slash", p.C.all().sevenseas.notallowed, undefined);
  check("its other kinds are kept", !!p.C.all().sevenseas.wait, true);
  check("no set without a pointer", p.C.save("tiptop", "TipTop", { hand: img("h") }), false);
  const anim = { pointer: Object.assign(img("p0"), { frames: [img("p1"), img("p2")], ms: 100 }) };
  p.C.save("bananabugs", "Banana Bugs", anim);
  check("an animated cursor keeps its frames", p.C.all().bananabugs.pointer, Object.assign(img("p0"), { frames: [img("p1"), img("p2")], ms: 100 }));
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
  const css = p.css();
  check("chosen: the page's class", p.root.classList.contains("site-cursor"), true);
  check("the pointer, hand and text rules", [css.indexOf("AAAAp") >= 0, /a\[href\][^{]*\{ cursor: url\("data:image\/png;base64,AAAAh"\)/.test(css), css.indexOf("AAAAt") >= 0], [true, true, true]);
  check("no circle-slash or wait rule without their images", [css.indexOf(":disabled {") >= 0 || css.indexOf("[aria-disabled=true] {") >= 0, css.indexOf("site-busy") >= 0], [false, false]);
  check("a disabled button is not given the hand", /button:not\(:disabled\)/.test(css), true);
  p.C.save("alchemy", "Alchemy", { pointer: img("p"), notallowed: img("n", 2, 2), wait: img("w", 5, 6) });
  p.C.choose("alchemy");
  const css2 = p.css();
  check("the circle-slash on disabled controls", /button:disabled[^{]*\[aria-disabled=true\] \{ cursor: url\("data:image\/png;base64,AAAAn"\) 2 2, not-allowed !important; \}/.test(css2), true);
  check("the wait cursor while busy, under the dragging rule", [/site-busy[^{]*\{ cursor: url\("data:image\/png;base64,AAAAw"\) 5 6, wait/.test(css2),
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

  p.C.save("bananabugs", "Banana Bugs", { pointer: Object.assign(img("p0"), { frames: [img("p1")], ms: 120 }), hand: img("h"),
    wait: Object.assign(img("w0"), { frames: [img("w1"), img("w2")], ms: 80 }) });
  p.C.choose("bananabugs");
  p.fire("mouseover", { target: el });
  check("the pointer's first frame on the element under the mouse", el.inline.cursor, 'url("data:image/png;base64,AAAAp0") 0 0, auto !important');
  check("one timer, at the frames' pace", [...p.timers.values()].map((t) => t.ms), [120]);
  p.tick();
  check("the next frame with the mouse still", el.inline.cursor, 'url("data:image/png;base64,AAAAp1") 0 0, auto !important');
  p.tick();
  check("and round again", el.inline.cursor, 'url("data:image/png;base64,AAAAp0") 0 0, auto !important');
  const link = element("A", ["a[href]"]);
  p.fire("mouseover", { target: link });
  check("onto a link, whose hand is still: the element given back, no timer", [el.inline, link.inline, p.timers.size], [{}, {}, 0]);
  p.fire("mouseover", { target: el });
  p.C.busy("addgames", true);
  check("busy: the wait cursor's frames", [el.inline.cursor, [...p.timers.values()].map((t) => t.ms)], ['url("data:image/png;base64,AAAAw0") 0 0, wait !important', [80]]);
  p.C.busy("addgames", false);
  check("busy no more: the pointer's again", el.inline.cursor, 'url("data:image/png;base64,AAAAp0") 0 0, auto !important');
  p.root.classList.add("game-cursor");
  p.fire("mouseover", { target: el });
  check("a game's own cursor showing: nothing animated", [el.inline, p.timers.size], [{}, 0]);
  p.root.classList.remove("game-cursor");
  p.fire("mouseover", { target: el });
  p.fire("mouseout", { target: el, relatedTarget: null });
  check("the mouse leaves the page: given back, no timer", [el.inline, p.timers.size], [{}, 0]);
}

// the Settings' choice: games with the same set share one entry
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
  p.C.save("all", "All Six", { wait: img("w"), notallowed: img("n"), text: img("t"), drag: img("d"), hand: img("h"),
    pointer: Object.assign(img("p0"), { frames: [img("p1")], ms: 100 }) });
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
  check("an animated cursor by its first frame, every picture decorative", [pics[0].src, pics.every((i) => i.alt === "")], ["data:image/png;base64,AAAAp0", true]);
}
// Pictures compared after cropping: a stand-in codec whose "PNG" is the pixels as JSON
function codec(p) {
  p.C.codec.encode = (px) => "data:image/png;base64," + Buffer.from(JSON.stringify([px.width, px.height, Array.from(px.data)])).toString("base64");
  p.C.codec.decode = (url, done) => {
    const [width, height, data] = JSON.parse(Buffer.from(url.slice(22), "base64").toString());
    done({ width, height, data: new Uint8ClampedArray(data) });
  };
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
{
  const p = page();
  codec(p);
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
function entryNames(p) {
  const el = node("div");
  p.C.render(el);
  return el.children[0].children[1].children.filter((l) => l.tagName === "label").map((l) => l.text().trim());
}
{
  const p = page();
  codec(p);
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
  const old = page();
  codec(old);
  const raw = (w, ox, colour) => ({ url: old.C.codec.encode(canvas(w, w, ox, ox, colour)), x: ox, y: ox });
  const kept = { alchemy: { title: "Alchemy", pointer: raw(37, 2, 10), hand: raw(37, 2, 20), drag: raw(37, 2, 30), text: raw(37, 4, 40) } };
  const p = page({ "byopop-cursors": JSON.stringify(kept) });
  codec(p);
  const pic = (w, ox, colour) => p.C.picture(canvas(w, w, ox, ox, colour), ox, ox);
  p.C.save("bejeweled", "Bejeweled", { pointer: pic(54, 9, 10), hand: pic(54, 9, 20), drag: pic(54, 9, 30), text: pic(54, 9, 40) });
  check("an old set looks like the same set taken since, before it is cropped", entryNames(p), ["Default", "Alchemy and Bejeweled"]);
  p.C.choose("alchemy");
  p.C.upgrade();
  check("cropped, it shares the entry of the same set taken since", entryNames(p), ["Default", "Alchemy and Bejeweled"]);
  const a = p.C.all();
  check("and is kept cropped, with its hot spot from the crop", [a.alchemy.hand.url === a.bejeweled.hand.url, a.alchemy.hand.k === a.bejeweled.hand.k,
    a.alchemy.hand.x, a.alchemy.hand.y, JSON.parse(p.local["byopop-cursors"]).alchemy.text.k === a.bejeweled.text.k], [true, true, 0, 0, true]);
  check("the site's cursors are drawn again from the cropped images", p.css().indexOf(a.alchemy.pointer.url) >= 0, true);
  const before = p.local["byopop-cursors"];
  p.C.upgrade();
  check("a second pass changes nothing", p.local["byopop-cursors"] === before, true);
}

console.log(`cursorstest: ${checks} checks, ${failed} failed`);
process.exit(failed ? 1 : 0);
