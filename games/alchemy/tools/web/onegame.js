// Plays the tutorial or a whole game of a mode in the player-only page with real DOM input (docs/WEB_PLAYER.md, "Drivers"):
// the game's own bot chooses every input (the test command "move", docs/TEST_COMMANDS.md: the title's link, the selector is
// pressed here, the pieces, the forge, the wizard's questions, the board-cleared panel, the name and the score table's button)
// and this sends it as the page's mouse and key events.
// Parameters of the page: mode= strategic (default), timetrial or tutorial; skill= 1 (default), 2 or 3; seconds= the most to
// play (900); blur= the number of pieces placed at which the window loses the focus for a while and gets it back (0: never);
// shots= every that many pieces a shot is saved (60); waitfull=1 shows the page's bar (the player-only page hides it), waits until
// the page is in full screen (a real click on its Fullscreen button, made from outside: the browser accepts nothing else), plays
// there, and at the end waits until full screen is left (F11 or Esc, or the button again).
// Opened as .../alchemy.html?player=1&assets=1&fast=1&silent=1&driver=games/alchemy/tools/web/onegame.js&run=web-strategic
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "strategic";
var skill = Number(params.get("skill") || 1);
var limit = Number(params.get("seconds") || 900) * 1000;
var blurAt = Number(params.get("blur") || 0);
var shotEvery = Number(params.get("shots") || 60);
var started = Date.now();
var stats = { places: 0, forges: 0, dialogs: 0, names: 0, cleared: 0, tutorial: 0, nulls: 0 };
var notes = [];
var last = null;
var seen = {};
var blurred = false;
var waitFull = params.get("waitfull") === "1";
var full = [];

function note(text) {
  notes.push({ at: Math.round((Date.now() - started) / 1000), update: t.updates(), text: text });
}

// the pointer arrives, waits an update as a player's hand does, and the button goes down and up
async function press(pt, what) {
  await t.move(pt[0] - 6, pt[1] - 6);
  await t.wait(1);
  await t.move(pt[0], pt[1]);
  await t.wait(2);
  await t.click(pt[0], pt[1]);
  await t.wait(5);
  note("pressed " + what + " at " + pt[0] + "," + pt[1]);
}

// one MOVE's input, in order, as DOM events
async function send(m) {
  for (var i = 0; i < m.input.length; i++) {
    var st = m.input[i];
    if (st.move) await t.move(st.move[0], st.move[1]);
    else if (st.wait) await t.wait(st.wait);
    else if (st.click) await t.click(st.click[0], st.click[1], st.button || "left");
    else if (st.key) await t.key(st.key);
  }
  await t.wait(1);
}

async function nextMove() {
  var lines = await t.command("move");
  var line = lines[lines.length - 1];
  if (!/^MOVE /.test(line)) throw new Error("move: " + line);
  var body = line.substring(5);
  return body === "null" ? null : JSON.parse(body);
}

async function untilState(pred, ms, what) {
  var t0 = Date.now();
  for (;;) {
    last = await t.state();
    if (pred(last)) return last;
    if (Date.now() - t0 > ms) throw new Error("timeout waiting for " + what + "; last " + JSON.stringify(last).substring(0, 400));
    await t.wait(5);
  }
}

function sleep(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }

// what the page shows of its full screen, with the game's own idea of the window
async function fullNote(what) {
  var fe = document.fullscreenElement;
  var cv = document.getElementById("canvas");
  var r = cv ? cv.getBoundingClientRect() : null;
  var s2 = await t.state();
  var o = { what: what, fullscreenElement: fe ? (fe.id || fe.tagName) : null, screen: [screen.width, screen.height],
    inner: [innerWidth, innerHeight], canvas: r ? [Math.round(r.left), Math.round(r.top), Math.round(r.width), Math.round(r.height)] : null,
    gameWindowed: s2.windowed, active: s2.active, muted: s2.muted };
  full.push(o);
  note("full screen, " + what + ": " + JSON.stringify(o));
}

try {
  if (waitFull) {
    document.documentElement.classList.remove("player-only");
    var tw = Date.now();
    note("waiting for a real click on the Fullscreen button");
    while (!document.fullscreenElement) {
      if (Date.now() - tw > 300000) throw new Error("no full screen within 300 s");
      await sleep(250);
    }
    await sleep(1500);
    await fullNote("entered");
  }
  // the title: its link is the bot's first move
  await untilState(function (s) { return s.screen === "title" && s.ready; }, 120000, "the title link");
  await t.shot("title");
  await send(await nextMove());
  await untilState(function (s) { return s.screen === "menu"; }, 30000, "the selector");
  await t.wait(30);
  await t.shot("menu");

  // the selector: its own buttons, pressed with the mouse
  last = await t.state();
  if (mode === "tutorial") {
    await press(last.selector.tutorial, "tutorial");
  } else {
    await press(last.selector["skill" + skill], "skill " + skill);
    last = await t.state();
    await press(last.selector[mode], mode);
    last = await t.state();
    await t.shot("selector-" + mode + "-" + skill);
    await press(last.selector.start, "start");
  }
  await untilState(function (s) { return s.screen === "game" || s.dialog === "load"; }, 30000, "the board");
  if (last.dialog === "load") {
    // a game left unfinished by an earlier run is in the test profile's saves: the question to load it is answered No
    await press(last.buttons.no, "no to the saved game");
    await untilState(function (s) { return s.screen === "menu" && !s.dialog; }, 30000, "the selector again");
    last = await t.state();
    await press(last.selector.start, "start again");
    await untilState(function (s) { return s.screen === "game"; }, 30000, "the board");
  }
  await t.shot(mode + "-start");
  if (waitFull) await fullNote("the board");

  // the game: until the selector is back
  var lastShot = 0;
  var lastTutorial = null;
  while (Date.now() - started < limit) {
    last = await t.state();
    if (last.screen === "menu") break;
    if (last.screen !== "game" && last.screen !== "highscores" && last.screen !== "menu") {
      note("screen " + last.screen);
    }
    var b = last.board || {};
    var key = [b.state, last.screen, last.dialog || "x"].join("-");
    if (!seen[key] && (b.state === "cleared" || b.state === "ending" || b.state === "gameover" || last.screen === "highscores" || last.dialog)) {
      seen[key] = 1;
      await t.shot(mode + "-" + key);
    }
    if (b.tutorial && b.tutorial.step !== lastTutorial) {
      lastTutorial = b.tutorial.step;
      await t.shot("tutorial-step" + ("0" + b.tutorial.step).slice(-2));
    }
    if (blurAt > 0 && !blurred && stats.places >= blurAt) {
      // the window loses the focus: the game must hold still, then play on when it comes back
      blurred = true;
      var before = JSON.stringify([last.score, b.moves]);
      await t.blur();
      await t.wait(60);
      var during = await t.state();
      var held = JSON.stringify([during.score, (during.board || {}).moves]) === before;
      // the time trial's forge fills by itself, so its level is only reported
      note("blur: active " + during.active + " muted " + during.muted + " paused " + (during.board || {}).paused + " score and moves held " + held +
        " forge " + last.forge + " -> " + during.forge);
      await t.shot("blur");
      await t.focus();
      await t.wait(30);
      var after = await t.state();
      note("focus: active " + after.active + " muted " + after.muted + " paused " + (after.board || {}).paused);
      await t.shot("focus");
      continue;
    }
    var m = await nextMove();
    if (m === null) {
      stats.nulls++;
      await t.wait(5);
      continue;
    }
    await send(m);
    if (m.kind === "place") stats.places++;
    else if (m.kind === "forge") stats.forges++;
    else if (m.kind === "name") stats.names++;
    else if (m.kind === "cleared") stats.cleared++;
    else if (m.kind === "tutorial") stats.tutorial++;
    else stats.dialogs++;
    if (m.kind === "place" && stats.places % shotEvery === 0 && stats.places !== lastShot) {
      lastShot = stats.places;
      await t.shot(mode + "-play" + stats.places);
    }
  }
  last = await t.state();
  await t.shot(mode + "-end");
  if (waitFull) {
    await fullNote("the game is over");
    var tl = Date.now();
    note("waiting for full screen to be left");
    while (document.fullscreenElement) {
      if (Date.now() - tl > 300000) throw new Error("full screen not left within 300 s");
      await sleep(250);
    }
    await sleep(1500);
    await fullNote("left");
    await t.shot(mode + "-left");
  }
} catch (e) {
  note("error: " + (e && e.message || e));
}
return { mode: mode, skill: skill, stats: stats, seconds: Math.round((Date.now() - started) / 1000), updates: t.updates(), last: last,
  notes: notes, fullscreen: full, errors: t.errors.slice(0, 5) };
