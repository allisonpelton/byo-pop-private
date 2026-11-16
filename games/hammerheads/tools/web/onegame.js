// Plays Hammer Heads in the player-only page with real DOM input only (docs/WEB_PLAYER.md, "Drivers"), as games/hammerheads/tools/
// realplay.py plays the native build: the loading screen's button, the name box (a wrong letter and a Backspace first), the main menu's
// mode button, and then every button the game shows (the how-to-play page, each introduction's and Ready box's Go, the shop's Back to
// Game, the bonus round's opening, the summaries), while the port's own bot (the test command "move") chooses every smash and this
// sends it as mouse events. The state says which box is up; where it does not, the buttons the first run showed are tried in turn.
// Parameters of the page: mode= classic (default), marathon or tough (the test command unlocks it, `goto tough`); levels= stop when
// the level reaches it; moves= the bot's moves at most (3000); heal=1 sets the hearts back to 5 at 2.5 (the bot is weak); focus=1
// loses and regains the focus once a few moves into the game; scale= draws the canvas at that many times its size (CSS).
// Opened as .../hammerheads.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/hammerheads/tools/web/onegame.js&run=web-hh
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "classic";
var maxLevel = Number(params.get("levels") || 0);
var maxMoves = Number(params.get("moves") || 3000);
var heal = params.get("heal") === "1";
var doFocus = params.get("focus") === "1";
var started = Date.now();
var result = { screens: [], moves: 0, stop: null };

var LOADING_PLAY = [397, 532], NAME_OK = [310, 392];
var MENU_BUTTON = { classic: [140, 320], marathon: [668, 325] };
var TRY = [[690, 405], [300, 550], [400, 438], [400, 344], [400, 388], [400, 575], [400, 540], [400, 510], [400, 470], [400, 300]];

async function handClick(p, after) {
  await t.move(p[0] - 5, p[1] - 5);
  await t.wait(2);
  await t.move(p[0], p[1]);
  await t.wait(2);
  await t.click(p[0], p[1]);
  await t.wait(after || 30);
}
async function state() {
  var s = await t.state();
  var key = s.screen + ":" + s.dialogs.join(",");
  if (result.screens[result.screens.length - 1] !== key) result.screens.push(key);
  return s;
}
async function type(text) {
  for (var c of text) { await t.type(c); await t.wait(5); }
}

try {
  if (params.get("hold") === "1") await t.profile.wipe();
  await t.start();
  await t.wait(700);
  var s = await state();
  await handClick(LOADING_PLAY, 300);
  s = await state();
  if (s.dialogs.indexOf(5) >= 0) {
    await type("X"); await t.key("Backspace"); await t.wait(5);
    await type("Test");
    await handClick(NAME_OK, 300);
    s = await state();
  }
  if (s.screen !== "menu") throw new Error("the main menu is not up after the name: " + JSON.stringify(s));
  if (mode === "marathon") await t.command("unlock marathon");
  if (mode === "tough") { await t.command("unlock all"); await t.command("goto tough"); }
  else await handClick(MENU_BUTTON[mode], 300);
  var stuck = 0, waits = 0, last = null, focused = false;
  for (;;) {
    s = await state();
    if (maxLevel && s.level >= maxLevel && s.screen === "game") { result.stop = "over: level " + s.level; break; }
    if (s.screen === "gamewon" || s.dialogs.indexOf(15) >= 0) { result.stop = "over: " + (s.screen === "gamewon" ? "won" : "lost"); break; }
    if (result.moves >= maxMoves) { result.stop = "limit"; break; }
    if (doFocus && !focused && result.moves >= 5) { await t.blur({ hidden: true }); await t.wait(60); await t.focus(); await t.wait(30); focused = true; }
    if (heal && s.screen === "game" && s.health > 0 && s.health <= 2.5 && s.inplay) await t.command("set health 5");
    var m = null;
    if ((s.screen === "game" || s.screen === "challenge") && !s.dialogs.length && s.paused === 0) m = await t.botMove();
    if (m) { await t.playMove(m); result.moves++; stuck = 0; continue; }
    var key = JSON.stringify(s);
    if (key === last) stuck++; else { stuck = 0; last = key; waits = 0; }
    if (stuck === 0 || (!s.dialogs.length && waits < 12)) { waits++; await t.wait(60); continue; }
    if (stuck > 3 * TRY.length) throw new Error("no button changes the screen " + s.screen + " " + JSON.stringify(s.dialogs));
    await handClick(TRY[(stuck - 1) % TRY.length], 40);
  }
} catch (e) {
  result.stop = "error: " + e.message;
}
result.seconds = Math.round((Date.now() - started) / 1000);
result.errors = t.errors.length;
return result;
