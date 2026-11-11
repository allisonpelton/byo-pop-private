// Plays AstroPop in the player-only page with real DOM input only (docs/WEB_PLAYER.md, "Drivers"), as tools/realplay.py plays the native
// build: the title's link, the new player box (a wrong letter and a Backspace first), the main menu, the mode's button, the pilot
// selector, the full-screen help, and then the port's own bot chooses every move (the test command "move", src/BoardBot.cpp: hints, the
// demo, the stories, the upgrade box, the weapon boxes, the emergency panel) and this sends it as mouse and key events. A shot is taken
// only to check what is drawn. Code plays; the session reads the result.
// Parameters of the page: mode= classic (default) or survival; pilot= 0 Vector, 1 Sprocket; moves= the bot's moves (150); lose= after that
// many moves the test command `lose` ends the game and the end screen's Continue and Menu are clicked; focus=1 loses and regains the
// focus once a few moves into the game (the game must pause and run on); scale= draws the canvas at that many times its size (CSS), so
// that the DOM events land at scaled points as they do in full screen, which the Browser pane cannot enter; shots=1 saves a shot at each
// screen.
// Opened as .../astropop.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/astropop/tools/web/onegame.js&run=web-astropop
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "classic";
var pilot = Number(params.get("pilot") || 0);
var maxMoves = Number(params.get("moves") || 150);
var loseAt = params.get("lose") === null ? null : Number(params.get("lose"));
var doFocus = params.get("focus") === "1";
var scale = Number(params.get("scale") || 1);
var shots = params.get("shots") === "1";
var started = Date.now();
var result = { notes: [], screens: [] };

var TITLE_PLAY = [320, 447];
var MENU_BUTTON = { classic: [100, 190], survival: [100, 330] };
var PILOT_GO = [[80, 418], [399, 418], [559, 418], [239, 418]];
var HELP_CONTINUE = [320, 440];
var END_CONTINUE = [278, 449];

function note(text) {
  result.notes.push({ at: Math.round((Date.now() - started) / 1000), update: t.updates(), text: text });
}

async function shot(name) {
  if (shots) await t.shot(name);
}

// a hand's click: the pointer arrives (a hover, an update), rests, then presses and releases
async function handClick(p) {
  await t.move(p[0] - 5, p[1] - 5);
  await t.wait(2);
  await t.move(p[0], p[1]);
  await t.wait(2);
  await t.click(p[0], p[1]);
  await t.wait(4);
}

async function state() {
  var s = await t.state();
  if (result.screens.indexOf(s.screen) < 0) result.screens.push(s.screen);
  return s;
}

function expect(s, key, value, what) {
  if (s[key] !== value) throw new Error(what + ": " + key + " is " + JSON.stringify(s[key]) + ", not " + JSON.stringify(value));
}

try {
  // a held page (hold=1, as the page check has it): a fresh test profile first, so that no saved game or player is there (a saved
  // game would ask "Continue?" at the mode's button), then the game starts
  if (params.get("hold") === "1") {
    await t.profile.wipe();
    await t.start();
  }
  await t.ready;
  if (scale !== 1) {
    var cv = document.getElementById("canvas");
    cv.style.width = Math.round(640 * scale) + "px";
    cv.style.height = Math.round(480 * scale) + "px";
    await t.wait(20);
    result.scale = Math.round(cv.getBoundingClientRect().width) + "x" + Math.round(cv.getBoundingClientRect().height);
  }
  // the title (the page has loaded the files; the game's own loader runs first)
  var s;
  for (var i = 0; i < 400; i++) {
    s = await state();
    if (s.screen === "title") break;
    await t.wait(30);
  }
  expect(s, "screen", "title", "start");
  await t.wait(60);
  await shot("title");
  await handClick(TITLE_PLAY);
  await t.wait(40);
  await shot("newuser");
  // the player's name with real keys: a wrong letter and a Backspace first
  await t.type("X");
  await t.wait(4);
  await t.key("Backspace");
  await t.wait(4);
  var name = "Test";
  for (var c = 0; c < name.length; c++) {
    await t.type(name[c]);
    await t.wait(3);
  }
  await t.key("Enter");
  await t.wait(60);
  s = await state();
  expect(s, "screen", "menu", "after the name");
  expect(s, "user", "Test", "the name typed");
  await shot("menu");
  await handClick(MENU_BUTTON[mode]);
  await t.wait(40);
  await shot("pilots");
  await handClick(PILOT_GO[pilot]);
  await t.wait(40);
  s = await state();
  if (s.checkpoint) {
    await handClick([s.checkpoint[1], s.checkpoint[2]]);
    await t.wait(40);
  }
  await handClick(HELP_CONTINUE);
  await t.wait(20);
  s = await state();
  expect(s, "screen", "game", "after the help");

  var moves = 0, nulls = 0, focused = false, stop = null;
  for (;;) {
    s = await state();
    if (s.over === true || s.over === 1) { stop = "over"; break; }
    if (moves >= maxMoves || (loseAt !== null && moves >= loseAt)) { stop = "limit"; break; }
    if (doFocus && !focused && moves >= 5) {
      focused = true;
      await t.blur();
      await t.wait(60);
      var s1 = await state();
      note("focus lost: paused=" + s1.paused);
      await shot("focus-lost");
      await t.focus();
      await t.wait(60);
      var s2 = await state();
      note("focus gained: paused=" + s2.paused);
      await shot("focus-gained");
      if (s1.paused !== 1) throw new Error("the game did not pause when the focus was lost");
      if (s2.paused === 1) throw new Error("the game did not run on when the focus came back");
      continue;
    }
    var m = await t.botMove();
    if (m === null) {
      if (++nulls >= 40) { stop = "no move"; break; }
      await t.wait(30);
      continue;
    }
    nulls = 0;
    var before = s.updates;
    webKeys(m);
    await t.playMove(m);
    var steps = Array.isArray(m.input) ? m.input.length : (m.clicks || []).length + (m.drags || []).length + (m.keys || []).length;
    if (steps) moves++;
  }
  await shot("end-of-play");
  if (loseAt !== null && stop === "limit") {
    await ok("lose");
    await t.wait(400);
    s = await state();
    if (!(s.over === true || s.over === 1)) throw new Error("`lose` did not end the game (over is " + JSON.stringify(s.over) + ")");
    stop = "lose";
  }
  if (stop === "over" || stop === "lose") {
    await shot("end-screen");
    await handClick(END_CONTINUE);
    await t.wait(60);
    s = await state();
    expect(s, "screen", "game", "Continue on the end screen");
    note("Continue: a new game (screen game)");
    await shot("new-game");
    // a player who has passed a checkpoint is asked where the new game starts: the first level's gate (the box comes up some
    // updates after the click, so the state is polled for it for up to 200 updates; a click on the help's button before it was up
    // would land on its checkpoint gate)
    for (var w = 0; w < 10; w++) {
      s = await state();
      if (s.checkpoint) break;
      await t.wait(20);
    }
    if (s.checkpoint) {
      await handClick([s.checkpoint[0], s.checkpoint[2]]);
      await t.wait(40);
      note("a checkpoint box: the first gate");
    }
    await handClick(HELP_CONTINUE);
    await t.wait(40);
    // a game that went deep continues from its checkpoint ("Continue from level 5?"), and a level's first brick of a new kind opens
    // its hint box, which holds the game (paused) until its OK is clicked: the bot's moves click it, and play on, until it runs
    for (var h = 0; h < 6; h++) {
      s = await state();
      if (s.paused !== 1) break;
      var hm = await t.botMove();
      if (hm === null) { await t.wait(20); continue; }
      webKeys(hm);
      await t.playMove(hm);
      note("a box at the start of the new game: the bot's move " + (h + 1));
    }
    // out by the options box: Escape, Main Menu, Yes
    await t.key("Escape");
    await t.wait(40);
    await shot("options-box");
    await handClick([230, 388]);
    await t.wait(20);
    await shot("main-menu-box");
    await handClick([255, 279]);
    // the box takes the game away over some updates: the number it took is noted (the native driver waits 80)
    var waited = 0;
    do {
      await t.wait(20);
      waited += 20;
      s = await state();
    } while (s.screen !== "menu" && waited < 600);
    note("Main Menu, Yes: " + s.screen + " after " + waited + " updates (plus the click)");
    expect(s, "screen", "menu", "after Main Menu, Yes");
    note("Main Menu, Yes: the menu");
    await shot("menu-again");
  }
  result.mode = mode;
  result.moves = moves;
  result.stop = stop;
  result.state = s;
} catch (e) {
  result.error = String(e && e.stack || e);
}
result.errors = t.errors.length;
result.updates = t.updates();
result.seconds = Math.round((Date.now() - started) / 1000);
return result;

// the bot names a key as the native build reads it ("Return"); the page's kit names it as the DOM does ("Enter")
function webKeys(m) {
  var names = { Return: "Enter" };
  var fix = function (k) { return names[k] || k; };
  if (Array.isArray(m.input)) m.input.forEach(function (i) { if (i.key) i.key = fix(i.key); });
  else if (m.keys) m.keys = m.keys.map(fix);
}

async function ok(line) {
  var lines = await t.command(line);
  var last = lines[lines.length - 1];
  if (!/^OK /.test(last)) throw new Error(line + " -> " + last);
}
