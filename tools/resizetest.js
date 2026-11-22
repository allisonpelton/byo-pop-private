#!/usr/bin/env node
// The settling of a window's resizes (web/skins.js, the first lines; docs/WEB_PLAYER.md, "Resizes"): a run of the browser's
// resize events, as Firefox sends one for every step of its full-screen transition, reaches the page's listeners (SDL's among
// them) as its first event and one more for the size reached, never once a step (the user, 2026-11-17: "The site lags a lot
// and causes audio crackling because it tries to resize repeatedly when Firefox enters full screen"). The code is read from
// web/skins.js and run on a stand-in window with a clock of its own. Prints "resizetest: N of M checks ok".
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const src = fs.readFileSync(path.join(__dirname, "..", "web", "skins.js"), "utf8");
const at = src.indexOf("(function () {\n  var SETTLE_MS");
const end = at < 0 ? -1 : src.indexOf("\n})();", at);
let ok = 0, all = 0;
function check(what, good, detail) { all++; if (good) ok++; else console.log("FAIL " + what + (detail ? ": " + detail : "")); }
if (at < 0 || end < 0) { console.log("FAIL web/skins.js has no settling of resizes"); console.log("resizetest: 0 of 1 checks ok"); process.exit(1); }
const code = src.slice(at, end + "\n})();".length);

// a window with capturing and other listeners, run as the DOM runs them at the target: capturing ones first, and
// stopImmediatePropagation ending the rest; and a clock that moves only when told
function stand() {
  const lists = { capture: [], bubble: [] };
  let now = 0, seq = 0;
  const timers = new Map();
  class Ev {
    constructor(type, trusted) { this.type = type; this.isTrusted = !!trusted; this.stopped = false; }
    stopImmediatePropagation() { this.stopped = true; }
  }
  const win = {
    addEventListener(type, fn, capture) { if (type === "resize") lists[capture ? "capture" : "bubble"].push(fn); },
    dispatchEvent(ev) {
      for (const fn of lists.capture.concat(lists.bubble)) { if (ev.stopped) break; fn(ev); }
      return true;
    },
  };
  const ctx = {
    window: win,
    Event: function (type) { return new Ev(type, false); },
    setTimeout(fn, ms) { const id = ++seq; timers.set(id, { at: now + ms, fn }); return id; },
    clearTimeout(id) { timers.delete(id); },
  };
  vm.createContext(ctx);
  vm.runInContext(code, ctx);
  const heard = [];
  // a listener added later and not capturing, as SDL's is
  win.addEventListener("resize", (ev) => heard.push({ at: now, trusted: ev.isTrusted }), false);
  function advance(ms) {
    const until = now + ms;
    for (;;) {
      let next = null;
      for (const [id, t] of timers) if (t.at <= until && (!next || t.at < next.t.at)) next = { id, t };
      if (!next) break;
      timers.delete(next.id);
      now = next.t.at;
      next.t.fn();
    }
    now = until;
  }
  return { heard, advance, browser() { win.dispatchEvent(new Ev("resize", true)); }, page() { win.dispatchEvent(new Ev("resize", false)); } };
}

// one resize alone goes on at once, and nothing follows it
{
  const w = stand();
  w.browser();
  check("one resize: at once", w.heard.length === 1 && w.heard[0].trusted && w.heard[0].at === 0, JSON.stringify(w.heard));
  w.advance(1000);
  check("one resize: nothing after it", w.heard.length === 1, JSON.stringify(w.heard));
}
// a run of 25 steps a frame apart (Firefox's transition): the first at once, then one for the size reached, SETTLE_MS on
{
  const w = stand();
  for (let i = 0; i < 25; i++) { w.browser(); w.advance(16); }
  check("a run: only its first while it lasts", w.heard.length === 1, JSON.stringify(w.heard));
  w.advance(1000);
  check("a run: one more once it settles", w.heard.length === 2 && !w.heard[1].trusted, JSON.stringify(w.heard));
  check("a run: settled 120 ms after its last step", w.heard.length === 2 && w.heard[1].at === 24 * 16 + 120, JSON.stringify(w.heard));
}
// the page's own resizes (onGameScreen's, chooseScale's, a full-screen change's) go on at once, even in a run
{
  const w = stand();
  w.browser(); w.advance(16); w.browser(); w.page();
  check("the page's own: at once in a run", w.heard.length === 2 && !w.heard[1].trusted, JSON.stringify(w.heard));
  w.advance(1000);
  check("the page's own: the run still ends with one", w.heard.length === 3, JSON.stringify(w.heard));
}
// two runs apart: each its first and its last
{
  const w = stand();
  for (let i = 0; i < 5; i++) { w.browser(); w.advance(16); }
  w.advance(500);
  for (let i = 0; i < 5; i++) { w.browser(); w.advance(16); }
  w.advance(500);
  check("two runs: two each", w.heard.length === 4 && w.heard[0].trusted && !w.heard[1].trusted && w.heard[2].trusted && !w.heard[3].trusted, JSON.stringify(w.heard));
}
// steps further apart than SETTLE_MS are separate resizes, each at once
{
  const w = stand();
  for (let i = 0; i < 4; i++) { w.browser(); w.advance(200); }
  check("slow steps: each at once", w.heard.length === 4 && w.heard.every((h) => h.trusted), JSON.stringify(w.heard));
}

console.log("resizetest: " + ok + " of " + all + " checks ok");
process.exit(ok === all ? 0 : 1);
