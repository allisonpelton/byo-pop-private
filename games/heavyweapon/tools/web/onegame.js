// Plays Heavy Weapon Deluxe in the player-only page with real DOM input only (docs/WEB_PLAYER.md, "Drivers"): the intro clicked
// away, the title's "click here to play", the new player's name typed in its box, the main menu's mode button, and then the port's
// own bot chooses every move (the test command "move", src/testhooks/Bot.cpp: the map's link, the help's OK, the play tips, the
// results, the armory's upgrades and GO, the tank's aim and fire) and this sends it as mouse events, until the game is over or the
// moves run out; a game that ends is left by the game-over box's QUIT. A shot is taken only to check what is drawn. Code plays; the
// session reads the result.
// Parameters of the page: mode= campaign (default) or survival; moves= the bot's moves (1500); shots=1 saves a shot at each screen.
// Opened as .../heavyweapon.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/heavyweapon/tools/web/onegame.js&run=web-heavyweapon
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "campaign";
var maxMoves = Number(params.get("moves") || 1500);
var shots = params.get("shots") === "1";
var started = Date.now();
var result = { notes: [], screens: [] };

var INTRO_CLICK = [320, 240];
var TITLE_PLAY = [320, 419];						// TitleScreen's play button (PLAY_BUTTON_Y 405, 29 high)
var MENU_BUTTON = { campaign: [100, 250], survival: [230, 380] };	// MainMenu's buttons 0 and 1
var GAME_OVER_QUIT = { campaign: [395, 310], survival: [395, 320] };	// GameOverDialog's QUIT
var USER_NAME_DIALOG = 0x1b;

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

async function waitFor(test, what, limit) {
  var s;
  for (var i = 0; i < (limit || 100); i++) {
    s = await state();
    if (test(s)) return s;
    await t.wait(10);
  }
  throw new Error(what + ": the screen is " + (s && s.screen) + ", dialog " + (s && s.dialog));
}

try {
  // a fresh test profile first, so that the game starts with no player, then the game starts
  if (params.get("hold") === "1") {
    await t.profile.wipe();
    await t.start();
  }
  await t.ready;
  var s = await waitFor(function (x) { return x.screen === "title"; }, "start", 400);
  await t.wait(30);
  await shot("intro");
  // the intro takes a click for the focus and another to end; the game loads, and the title shows its link
  await handClick(INTRO_CLICK);
  await handClick(INTRO_CLICK);
  await t.wait(120);
  await shot("title");
  await handClick(TITLE_PLAY);
  await t.wait(30);
  s = await state();
  if (s.dialog === USER_NAME_DIALOG) {
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
    await t.wait(40);
    note("a new player's name typed");
  }
  s = await waitFor(function (x) { return x.screen === "menu" && x.dialog === -1; }, "the main menu", 50);
  // survival opens once the campaign is won (the menu shows a box until then): the standard test command reaches that
  if (mode === "survival") {
    var lines = await t.command("unlock all");
    if (!/^OK /.test(lines[lines.length - 1])) throw new Error("unlock all -> " + lines[lines.length - 1]);
    note("survival unlocked with the test command");
  }
  await shot("menu");
  await handClick(MENU_BUTTON[mode]);
  await t.wait(30);
  s = await state();
  await shot(mode === "campaign" ? "map" : "survival");

  var moves = 0, nulls = 0, stop = null, seen = {};
  for (;;) {
    s = await state();
    if (!seen[s.screen]) { seen[s.screen] = true; await shot("screen-" + s.screen); }
    if (s.over === true) { stop = "over"; break; }
    if (moves >= maxMoves) { stop = "limit"; break; }
    var m = await t.botMove();
    if (m === null) {
      if (++nulls >= 60) { stop = "no move"; break; }
      await t.wait(30);
      continue;
    }
    nulls = 0;
    await t.playMove(m);
    moves++;
  }
  await shot("end-of-play");
  if (stop === "over") {
    await handClick(GAME_OVER_QUIT[mode]);
    await t.wait(60);
    s = await state();
    note("QUIT on the game-over box: " + s.screen);
    await shot("after-quit");
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
