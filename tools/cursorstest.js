// Checks the site's cursor (web/cursors.js; docs/WEB_PLAYER.md, "The site's cursor") under Node, in a stand-in page: which of a
// game's cursors are kept (the six kinds the site shows, none whose image means something else in that game, frames only when
// well-formed), the rules each context gets (the pointer, hand and text always, the circle-slash on disabled controls and the wait
// cursor while the site is busy only from a set that has them), busy() and the frames of an animated cursor (swapped on the
// element under the mouse while the mouse stays still, and given back when it leaves; no timer for a set without frames), and the
// Settings' choice, where games with the same set share one entry.
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
    css: () => (head.children[0] || { textContent: "" }).textContent,
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
  check("a set is kept", p.C.save("bejeweled", "Bejeweled Deluxe", full), true);
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
  p.C.save("bejeweled", "Bejeweled Deluxe", { pointer: img("p"), hand: img("h"), text: img("t") });
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
  p.C.save("bejeweled", "Bejeweled Deluxe", look);
  p.C.save("alchemy", "Alchemy Deluxe", look);
  p.C.save("atomica", "Atomica Deluxe", look);
  p.C.save("bigmoney", "Big Money Deluxe", Object.assign({}, look, { text: img("t2") }));
  p.C.save("bookworm", "BookWorm Deluxe", Object.assign({}, look, { pointer: img("p", 1, 3) }));
  function entries() {
    const el = node("div");
    p.C.render(el);
    const box = el.children[0].children[1];
    return box.children.filter((l) => l.tagName === "label").map((l) => ({ value: l.children[0].value, checked: l.children[0].checked, name: l.text().trim() }));
  }
  check("identical sets make one entry, named with every title in order; a set that differs in one kind or one hot spot keeps its own",
    entries().map((e) => e.name), ["Default", "Alchemy Deluxe, Atomica Deluxe and Bejeweled Deluxe", "Big Money Deluxe", "BookWorm Deluxe"]);
  check("nothing chosen: the browser's own is checked", entries().filter((e) => e.checked).map((e) => e.name), ["Default"]);
  p.C.choose("bejeweled");
  check("a choice of any member selects its group, and keeps that member", [entries().filter((e) => e.checked).map((e) => e.name), entries()[1].value],
    [["Alchemy Deluxe, Atomica Deluxe and Bejeweled Deluxe"], "bejeweled"]);
  p.C.choose("alchemy");
  check("the first member too", entries().filter((e) => e.checked).map((e) => e.value), ["alchemy"]);
  p.C.forget("atomica");
  check("forgetting one game leaves the rest of its group", entries()[1].name, "Alchemy Deluxe and Bejeweled Deluxe");
  p.C.save("alchemy", "Alchemy Deluxe", Object.assign({}, look, { hand: img("h2") }));
  check("a set that changes leaves its group, and the choice stays with it",
    entries().map((e) => [e.name, e.checked]), [["Default", false], ["Alchemy Deluxe", true], ["Bejeweled Deluxe", false], ["Big Money Deluxe", false], ["BookWorm Deluxe", false]]);
  check("the sets stay one a game", Object.keys(p.C.all()).sort(), ["alchemy", "bejeweled", "bigmoney", "bookworm"]);
}
// each entry pictures every cursor its set has, in order, and nothing for one it lacks
{
  const p = page();
  p.C.save("all", "All Six", { wait: img("w"), notallowed: img("n"), text: img("t"), drag: img("d"), hand: img("h"),
    pointer: Object.assign(img("p0"), { frames: [img("p1")], ms: 100 }) });
  p.C.save("one", "Just A Pointer", { pointer: img("q") });
  p.C.save("gaps", "Some Gaps", { pointer: img("g"), text: img("gt"), wait: img("gw") });
  const el = node("div");
  p.C.render(el);
  const labels = el.children[0].children[1].children.filter((l) => l.tagName === "label");
  check("the kinds each entry pictures", labels.map((l) => [l.text().trim(), l.kinds()]),
    [["Default", []], ["All Six", ["pointer", "hand", "drag", "text", "notallowed", "wait"]], ["Just A Pointer", ["pointer"]], ["Some Gaps", ["pointer", "text", "wait"]]]);
  const pics = labels[1].children[1].children;
  check("an animated cursor by its first frame, every picture decorative", [pics[0].src, pics.every((i) => i.alt === "")], ["data:image/png;base64,AAAAp0", true]);
}

console.log(`cursorstest: ${checks} checks, ${failed} failed`);
process.exit(failed ? 1 : 0);
