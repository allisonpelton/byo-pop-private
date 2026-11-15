// Plays Heavy Weapon in the player-only page with real DOM input only (docs/WEB_PLAYER.md, "Drivers"), as tools/playbymove plays
// the native build from launch: the intro clicked away, the title's "click here to play", the new player box (a wrong letter
// and a Backspace first), the main menu's campaign (or, with mode=survival, every mode unlocked by the test command and the
// main menu's survival), and then the port's own bot chooses every move (the test command "move": the help's PLAY, the map's
// link, the play tips, the results, the armory, and the tank's moves and fire) and this sends it as mouse events, until the
// game-over box. A shot is taken only to check what is drawn. Code plays; the session reads the result.
// Parameters of the page: mode= campaign (default) or survival; is3d= 0 (2D, the software screen) or 1 (3D); moves= the bot's
// moves at most (40000); focus=1 loses and regains the focus once a few hundred moves into the game (the game pauses, stays
// paused once the focus is back, and goes on at a click, as the original does); scale= draws the canvas at that many times its
// size (CSS), so that the DOM events land at scaled points as they do in full screen, which the Browser pane cannot enter;
// shots=1 saves a shot at each screen.
// Opened as .../heavyweapon.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/heavyweapon/tools/web/onegame.js&run=web-heavyweapon
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "campaign";
var is3d = params.get("is3d") === "1";
var maxMoves = Number(params.get("moves") || 40000);
var doFocus = params.get("focus") === "1";
var scale = Number(params.get("scale") || 1);
var shots = params.get("shots") === "1";
var started = Date.now();
var result = { notes: [], screens: [] };

var INTRO = [320, 240];
var TITLE_PLAY = [320, 419];
var MENU_BUTTON = { campaign: [100, 250], survival: [230, 380] };
var CONTINUE = [320, 240];          // "click anywhere to continue" after a pause

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
  // a held page (hold=1): a fresh test profile with the display setting, so that the run is a first run (the name box)
  if (params.get("hold") === "1") {
    await t.profile.wipe();
    await t.profile.seed({ "registry.ini": "[SOFTWARE\\PopCap\\HeavyWeapon]\nIs3D=dword:" + (is3d ? 1 : 0) + "\n" });
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
  // the intro clicked away, as a player who does not watch it
  await t.click(INTRO[0], INTRO[1]);
  await t.wait(2);
  await t.click(INTRO[0], INTRO[1]);
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
  await t.wait(80);
  s = await state();
  expect(s, "screen", "menu", "after the name");
  await shot("menu");
  if (mode === "survival") {
    var unlock = await t.command("unlock all");
    note("unlock all: " + unlock[unlock.length - 1]);
    await t.wait(10);
  }
  await handClick(MENU_BUTTON[mode]);
  await t.wait(40);
  s = await state();
  await shot("after_menu");

  // the bot's game: a move null (a screen still coming in, the map's link not yet up) waits 30 updates; 60 of them running
  // stop the run
  var moves = 0, nulls = 0, focusDone = false, stop = "limit", lastPoint = [320, 240];
  for (;;) {
    s = await state();
    if (s.over) { stop = "over"; break; }
    if (moves >= maxMoves) break;
    if (doFocus && !focusDone && s.screen === "game" && moves >= 300) {
      focusDone = true;
      var before = s;
      // a hand lets go of the fire button before it switches away (held, the next click would only release it: SDL takes
      // no press of a button that is already down)
      await t.up(lastPoint[0], lastPoint[1]);
      await t.wait(2);
      await t.blur();
      await t.wait(100);
      var hidden = await state();
      await t.focus();
      await t.wait(100);
      var back = await state();
      await shot("focus_back");
      await handClick(CONTINUE);
      await t.wait(20);
      var after = await state();
      result.focus = { before: before.paused, hidden: hidden.paused, back: back.paused, afterClick: after.paused };
      note("focus: paused " + before.paused + " -> " + hidden.paused + " -> " + back.paused + " -> " + after.paused + " after a click");
      continue;
    }
    var m = await t.botMove();
    if (m === null) {
      if (++nulls >= 60) { stop = "no move"; break; }
      await t.wait(30);
      continue;
    }
    nulls = 0;
    moves++;
    if (typeof m.x === "number") lastPoint = [m.x, m.y];
    await t.playMove(m);
  }
  await shot("end");
  result.moves = moves;
  result.stop = stop;
  result.state = s;
} catch (e) {
  result.error = String(e && e.message || e);
}
result.errors = t.errors.slice(0, 5);
result.seconds = Math.round((Date.now() - started) / 1000);
return result;
