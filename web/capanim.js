// The bottle cap and the opener (docs/WEB_PLAYER.md, "The cap and the opener"; the user's test notes: "Make the game pages have a
// bottle cap on top that is removed when you upload your files via an animation using the bottle opener. Implement this on the
// front page too if possible. When uploading multiple games' files at once, have the bottle opener fall from the top of the
// screen onto the grid of games, flipping the caps for games that are now playable, revealing their logo on the other side.")
//
// pop(cap, options): the opener (web/brand/opener.svg, our own drawing) slides in from the right, hooks the cap's edge and levers
// it up. With options.away the cap flies off and is gone (a game page's cap over its import card); with options.back (an
// element: the game's icon) it hops and flips over, showing that on its other side (a card of the listing page).
// drop(caps, backs): the opener falls from the top of the window onto the grid, and the caps flip over one after another, the
// nearest first, each showing its back.
//
// options.after (drop's third argument): a function, which may return a promise, run once the animation has ended and before
// its copies are taken away: the caller draws the cap's new state there, so the old one never shows again in between.
// Both resolve when they are done, and never wait long: a hidden page (no animation frames), a browser without the Web
// Animations API, or a player who asked for less motion (prefers-reduced-motion) gets the end at once, or a short fade. The cap
// itself is not changed: the animation runs on copies over it, laid on the page with position: fixed, and the caller then shows
// the cap's new state (the listing's check(), the game page's boot()).
var ByopopCapAnim = (function () {
  "use strict";
  var OPENER = "opener.svg", OPENER_W = 76, OPENER_H = 104;
  var SLOT_X = 0.5, SLOT_Y = 0.21;  // where the opener's slot is in its picture: the point that hooks the cap's edge
  var STYLE =
    ".bpa-layer { position: fixed; left: 0; top: 0; width: 0; height: 0; z-index: 1000; pointer-events: none; }\n" +
    ".bpa-opener { position: fixed; display: block; transform-origin: " + (SLOT_X * 100) + "% " + (SLOT_Y * 100) + "%; filter: drop-shadow(0 4px 6px rgba(0, 0, 0, 0.35)); }\n" +
    ".bpa-flip { position: fixed; perspective: 600px; }\n" +
    ".bpa-turn { position: absolute; inset: 0; transform-style: preserve-3d; }\n" +
    ".bpa-face { position: absolute; inset: 0; margin: 0; -webkit-backface-visibility: hidden; backface-visibility: hidden; display: flex; align-items: center; justify-content: center; }\n" +
    ".bpa-face.bpa-back { transform: rotateY(180deg); }\n" +
    ".bpa-face img { display: block; width: 100%; height: 100%; object-fit: contain; image-rendering: pixelated; }\n";
  var styled = false;
  function style() {
    if (styled) return;
    styled = true;
    var s = document.createElement("style");
    s.textContent = STYLE;
    document.head.appendChild(s);
  }
  function reduced() {
    try { return window.matchMedia("(prefers-reduced-motion: reduce)").matches; } catch (e) { return false; }
  }
  function canAnimate() {
    return !document.hidden && typeof Element !== "undefined" && typeof Element.prototype.animate === "function";
  }
  // an animation's end, or the time it should take and a little more (a page hidden partway has no frames to finish it)
  function finish(anim, ms) {
    return new Promise(function (resolve) {
      var done = false;
      function end() { if (!done) { done = true; resolve(); } }
      try { anim.finished.then(end, end); } catch (e) { end(); }
      setTimeout(end, ms + 400);
    });
  }
  function wait(ms) { return new Promise(function (resolve) { setTimeout(resolve, ms); }); }
  function layer() {
    var l = document.createElement("div");
    l.className = "bpa-layer";
    l.setAttribute("aria-hidden", "true");
    document.body.appendChild(l);
    return l;
  }
  function opener(l, width) {
    var img = document.createElement("img");
    img.className = "bpa-opener";
    img.src = OPENER;
    img.alt = "";
    img.width = width;
    img.height = Math.round(width * OPENER_H / OPENER_W);
    l.appendChild(img);
    return img;
  }
  // A copy of the cap at its place, with its colour (a CSS variable the cap may inherit from its card)
  function copyOf(cap) {
    var c = cap.cloneNode(true), cs = getComputedStyle(cap);
    c.removeAttribute("id");
    c.removeAttribute("draggable");
    ["--tile", "color", "font-size", "font-weight", "font-family", "letter-spacing"].forEach(function (p) {
      var v = cs.getPropertyValue(p);
      if (v) c.style.setProperty(p, v);
    });
    c.style.visibility = "visible";  // (the cap itself is hidden while its copy moves)
    c.style.margin = "0";
    c.style.width = "100%";
    c.style.height = "100%";
    return c;
  }
  // The cap and its back, turning together, laid over the cap
  function flipper(l, cap, back) {
    var r = cap.getBoundingClientRect();
    var box = document.createElement("div");
    box.className = "bpa-flip";
    box.style.left = r.left + "px"; box.style.top = r.top + "px"; box.style.width = r.width + "px"; box.style.height = r.height + "px";
    var turn = document.createElement("div");
    turn.className = "bpa-turn";
    var front = document.createElement("div");
    front.className = "bpa-face";
    front.appendChild(copyOf(cap));
    var rear = document.createElement("div");
    rear.className = "bpa-face bpa-back";
    rear.appendChild(back || copyOf(cap));
    turn.appendChild(front);
    turn.appendChild(rear);
    box.appendChild(turn);
    l.appendChild(box);
    return { box: box, turn: turn, rect: r };
  }
  function flip(f, delay) {
    var a = f.turn.animate([
      { transform: "translateY(0) rotateY(0deg)" },
      { transform: "translateY(-30%) rotateY(90deg)", offset: 0.45 },
      { transform: "translateY(-12%) rotateY(160deg)", offset: 0.75 },
      { transform: "translateY(0) rotateY(180deg)" }
    ], { duration: 650, delay: delay, easing: "ease-out", fill: "forwards" });
    return finish(a, 650 + delay);
  }
  // the cap hidden while its copy moves, and shown again once the caller has drawn its new state
  function hide(cap) { var v = cap.style.visibility; cap.style.visibility = "hidden"; return function () { cap.style.visibility = v; }; }
  function settle(after, cleanup) {
    var p;
    try { p = Promise.resolve(after ? after() : null); } catch (e) { p = Promise.resolve(); }
    return p.catch(function () {}).then(function () { if (cleanup) cleanup(); });
  }

  // ---------- one cap ----------
  function pop(cap, options) {
    options = options || {};
    if (!cap || !cap.isConnected) return settle(options.after);
    var r = cap.getBoundingClientRect();
    if (!canAnimate() || r.width === 0 || r.bottom < 0 || r.top > window.innerHeight) return settle(options.after);
    style();
    var l = layer(), show = hide(cap);
    function done() { return function () { return settle(options.after, function () { l.remove(); show(); }); }; }
    if (reduced()) {
      // no movement: the cap fades (away), or its back fades in over it
      var f0 = flipper(l, cap, options.back);
      var fade = options.away ? f0.box.animate([{ opacity: 1 }, { opacity: 0 }], { duration: 250, fill: "forwards" })
        : f0.turn.animate([{ transform: "rotateY(180deg)", opacity: 0 }, { transform: "rotateY(180deg)", opacity: 1 }], { duration: 250, fill: "forwards" });
      return finish(fade, 250).then(done());
    }
    var f = flipper(l, cap, options.back);
    var w = Math.max(40, Math.min(110, r.width * 0.9)), o = opener(l, w), oh = w * OPENER_H / OPENER_W;
    // the slot on the cap's right edge, the opener lying to the right of it, its head toward the cap
    var px = r.right - r.width * 0.06, py = r.top + r.height * 0.5;
    o.style.left = (px - w * SLOT_X) + "px";
    o.style.top = (py - oh * SLOT_Y) + "px";
    var slide = o.animate([
      { transform: "translateX(" + Math.round(w * 0.9) + "px) rotate(-90deg)", opacity: 0 },
      { transform: "translateX(0) rotate(-90deg)", opacity: 1 }
    ], { duration: 320, easing: "ease-out", fill: "forwards" });
    return finish(slide, 320).then(function () {
      // the lever: the handle goes down, the head lifts the cap's edge
      var lever = o.animate([{ transform: "rotate(-90deg)" }, { transform: "rotate(-68deg)" }], { duration: 200, easing: "ease-in", fill: "forwards" });
      f.box.style.transformOrigin = "0% 100%";
      var lift = f.box.animate([{ transform: "rotate(0deg)" }, { transform: "rotate(-14deg) translateY(-6%)" }], { duration: 200, easing: "ease-in", fill: "forwards" });
      return Promise.all([finish(lever, 200), finish(lift, 200)]);
    }).then(function () {
      var off = o.animate([{ transform: "rotate(-68deg)", opacity: 1 }, { transform: "translateX(" + Math.round(w * 0.9) + "px) rotate(-90deg)", opacity: 0 }],
        { duration: 300, delay: 150, easing: "ease-in", fill: "forwards" });
      var cap2;
      if (options.away) {
        // the cap flies up and spins away
        cap2 = finish(f.box.animate([
          { transform: "rotate(-14deg) translateY(-6%)", opacity: 1 },
          { transform: "translate(-40%, -260%) rotate(-250deg)", opacity: 0 }
        ], { duration: 650, easing: "cubic-bezier(.2, .7, .4, 1)", fill: "forwards" }), 650);
      } else {
        // the cap settles back, then hops and flips over
        cap2 = finish(f.box.animate([{ transform: "rotate(-14deg) translateY(-6%)" }, { transform: "rotate(0deg)" }], { duration: 160, fill: "forwards" }), 160)
          .then(function () { return flip(f, 0); });
      }
      return Promise.all([finish(off, 450), cap2]);
    }).then(done());
  }

  // ---------- many caps ----------
  function drop(caps, backs, after) {
    backs = backs || [];
    caps = caps || [];
    var seen = caps.map(function (c) {
      if (!c || !c.isConnected) return false;
      var r = c.getBoundingClientRect();
      return r.width > 0 && r.bottom > 0 && r.top < window.innerHeight;
    });
    if (!canAnimate() || !seen.some(Boolean)) return settle(after);
    style();
    var l = layer(), shows = [];
    var flips = [];
    caps.forEach(function (c, i) { if (seen[i]) { flips.push({ f: flipper(l, c, backs[i]), i: i }); shows.push(hide(c)); } });
    function done() { return settle(after, function () { l.remove(); shows.forEach(function (s) { s(); }); }); }
    if (reduced()) {
      return Promise.all(flips.map(function (x) {
        return finish(x.f.turn.animate([{ transform: "rotateY(180deg)", opacity: 0 }, { transform: "rotateY(180deg)", opacity: 1 }], { duration: 250, fill: "forwards" }), 250);
      })).then(done);
    }
    // where it lands: the middle of the caps that can be seen
    var xs = flips.map(function (x) { return x.f.rect.left + x.f.rect.width / 2; }), ys = flips.map(function (x) { return x.f.rect.top + x.f.rect.height / 2; });
    var cx = (Math.min.apply(null, xs) + Math.max.apply(null, xs)) / 2, cy = (Math.min.apply(null, ys) + Math.max.apply(null, ys)) / 2;
    var w = 96, o = opener(l, w), oh = w * OPENER_H / OPENER_W;
    o.style.left = (cx - w / 2) + "px";
    o.style.top = (cy - oh / 2) + "px";
    var fall = cy + oh;  // from just above the window
    var LAND = 0.62, TIME = 800;
    var a = o.animate([
      { transform: "translateY(" + (-fall) + "px) rotate(-25deg)", easing: "cubic-bezier(.5, 0, 1, 1)" },
      { transform: "translateY(0) rotate(8deg)", offset: LAND, easing: "ease-out" },
      { transform: "translateY(-18px) rotate(-4deg)", offset: 0.8, easing: "ease-in" },
      { transform: "translateY(0) rotate(0deg)" }
    ], { duration: TIME, fill: "forwards" });
    // the caps flip from where it landed, outwards
    var landed = TIME * LAND;
    var turned = flips.map(function (x) {
      var dx = x.f.rect.left + x.f.rect.width / 2 - cx, dy = x.f.rect.top + x.f.rect.height / 2 - cy;
      return flip(x.f, landed + Math.min(900, Math.sqrt(dx * dx + dy * dy) * 0.8));
    });
    return finish(a, TIME).then(function () {
      return finish(o.animate([{ opacity: 1, transform: "translateY(0)" }, { opacity: 0, transform: "translateY(12px)" }],
        { duration: 300, delay: 250, fill: "forwards" }), 550);
    }).then(function () { return Promise.all(turned); }).then(done);
  }

  return { pop: pop, drop: drop };
})();
