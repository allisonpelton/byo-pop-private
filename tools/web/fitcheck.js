// The game window's fit check (docs/WEB_PLAYER.md, "The game window's size"): a game window on Desktop 95 opened with 2x chosen
// in a browser window too small for it, then after that browser window shrinks and grows, never draws its game larger than the
// room the desktop has (the user, 2026-11-20: "Ning Po window is cut off", a game "tried to open larger than the window had room
// for"). It runs in the front page, served by tools/webtest/serve.py on a development host:
//   http://127.0.0.1:18780/index.html?silent=1&driver=tools/web/fitcheck.js
// The browser window is a frame of the front page holding the desktop (index.html?silent=1), whose size the check sets, so no
// real browser window is resized. In it, one game's window for each screen size (640 x 480, 800 x 600, 1024 x 768; the size is
// given through the page's own hook, Module.onGameScreen, so no game's files are needed) is opened with 2x chosen, and the frame
// goes small, smaller, large and small again. After each step:
//   inside   the game's picture starts at its area's top left and every part of it can be scrolled to (never cut off)
//   scale    the multiple drawn is 2x where the desktop has room for that window, else 1x, where the window scrolls if 1x does not fit
//   window   a game drawn at 2x has its whole window on the desktop, above the task bar
//   status   the status bar gives the multiple drawn
//   chosen   the View menu keeps 2x ticked, greyed where it does not fit
// The choice of scale and the desktop's layout are put back at the end. Its result is window.byopopDeskCheck.result, and the
// page's title ends "fitcheck: PASS" or "fitcheck: FAIL": {pass, fail, results: [{check, result: "pass"|"FAIL", detail}]}.
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
async function until(fn, ms) { for (var t = 0; t < ms; t += 100) { var v = null; try { v = fn(); } catch (e) {} if (v) return v; await wait(100); } return null; }

var GAMES = [{ id: "bejeweled", w: 640, h: 480 }, { id: "heavyweapon", w: 800, h: 600 }, { id: "bejeweledtwist", w: 1024, h: 768 }];
// the browser window's sizes, in CSS pixels: small, smaller, large (room for 2x of 1024 x 768) and small again
var STEPS = [["small", 1100, 820], ["smaller", 900, 660], ["large", 2300, 1750], ["small again", 1100, 820]];
var KEEP = ["byopop-d95-scale", "byopop-desk"];
var kept = {};
KEEP.forEach(function (k) { kept[k] = localStorage.getItem(k); });

// a new frame for each game, loaded before it is read (a frame given a new address keeps its old page until the new one comes)
var room = null;
function newRoom(w, h) {
  if (room) room.remove();
  room = document.createElement("iframe");
  room.style.cssText = "position:fixed;left:0;top:0;border:0;z-index:2147483647;background:#008080";
  size(w, h);
  var loaded = new Promise(function (r) { room.addEventListener("load", r, { once: true }); });
  room.src = "index.html?silent=1";
  document.body.appendChild(room);
  return loaded;
}
function size(w, h) { room.style.width = w + "px"; room.style.height = h + "px"; }
function box(el) { var r = el.getBoundingClientRect(); return { l: r.left, t: r.top, r: r.right, b: r.bottom, w: r.width, h: r.height }; }

try {
  for (var g = 0; g < GAMES.length; g++) {
    var game = GAMES[g];
    var scales = {}; scales[game.id] = 2;
    localStorage.setItem("byopop-d95-scale", JSON.stringify(scales));
    await newRoom(STEPS[0][1], STEPS[0][2]);
    var desk = await until(function () { var w = room.contentWindow; return w.ByopopApps && w.ByopopSkin && w.document.readyState === "complete" && w; }, 20000);
    if (!desk) { note("open", false, game.id + ": the desktop did not load in its frame"); continue; }
    if (!desk.ByopopSkin.desk.isOn()) { note("open", false, game.id + ": the desktop is off in a " + STEPS[0][1] + " pixel frame"); continue; }
    desk.ByopopApps.open(new URL(game.id + ".html", desk.location.href).href, "none");
    var page = await until(function () {
      var f = desk.document.querySelector(".d95-app-frame"), w = f && f.contentWindow;
      return w && w.ByopopEmbed && w.Module && w.Module.onGameScreen && w.document.getElementById("stage") && w;
    }, 30000);
    if (!page) { note("open", false, game.id + ": its window's page did not load"); continue; }
    page.Module.onGameScreen(game.w, game.h);
    for (var s = 0; s < STEPS.length; s++) {
      var step = STEPS[s], at = game.id + " (" + game.w + " x " + game.h + "), " + step[0] + " " + step[1] + " x " + step[2] + ": ";
      size(step[1], step[2]);
      await wait(1500);
      var frame = desk.document.querySelector(".d95-app-frame"), win = frame.closest(".d95-win");
      var stage = page.document.getElementById("stage"), area = stage.parentNode;
      area.scrollTop = 0; area.scrollLeft = 0;
      var sb = box(stage), ab = box(area), dpr = page.devicePixelRatio || 1;
      var n = Math.round(sb.w * dpr / game.w);
      note("inside", sb.l >= ab.l - 0.5 && sb.t >= ab.t - 0.5 && sb.w <= area.scrollWidth + 0.5 && sb.h <= area.scrollHeight + 0.5,
        at + "picture " + Math.round(sb.w) + " x " + Math.round(sb.h) + " at " + Math.round(sb.l - ab.l) + ", " + Math.round(sb.t - ab.t) +
        " in an area scrolling to " + area.scrollWidth + " x " + area.scrollHeight);
      // the window's frame and bars around the game: what 2x would add to the window as it is
      var wb = box(win), bar = desk.document.querySelector(".d95-taskbar"), barH = bar ? bar.offsetHeight : 28;
      var chromeW = wb.w - area.clientWidth, chromeH = wb.h - area.clientHeight;
      var fits2 = game.w * 2 / dpr + chromeW <= desk.innerWidth - 16 && game.h * 2 / dpr + chromeH <= desk.innerHeight - barH - 8;
      note("scale", n === (fits2 ? 2 : 1), at + "drawn at " + n + "x, where 2x " + (fits2 ? "fits" : "does not fit"));
      if (n === 2) note("window", wb.l >= -0.5 && wb.t >= -0.5 && wb.r <= desk.innerWidth + 0.5 && wb.b <= desk.innerHeight - barH + 0.5,
        at + "window " + Math.round(wb.w) + " x " + Math.round(wb.h) + " at " + Math.round(wb.l) + ", " + Math.round(wb.t));
      var sc = page.document.querySelector(".d95-scale"), said = sc ? sc.textContent : "";
      note("status", said.indexOf(n + "x") >= 0, at + "the status bar says \"" + said + "\"");
      var two = page.ByopopEmbed.menus().view.filter(function (v) { return v.label && /^2x\b/.test(v.label); })[0];
      note("chosen", !!two && !!two.checked && !!two.disabled === !fits2, at + "View's 2x " + (two ? (two.checked ? "ticked" : "not ticked") + ", " + (two.disabled ? "greyed" : "offered") : "missing"));
    }
  }
} finally {
  if (room) room.remove();
  KEEP.forEach(function (k) { if (kept[k] === null) localStorage.removeItem(k); else localStorage.setItem(k, kept[k]); });
}
var fail = results.filter(function (x) { return x.result === "FAIL"; }).length;
return { pass: results.length - fail, fail: fail, results: results };
