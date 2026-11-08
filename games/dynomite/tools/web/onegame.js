// Plays the tutorial or one game of a mode in the player-only page with real DOM input only (docs/WEB_PLAYER.md,
// "Drivers"): the title, the main menu's pages by their own buttons (the clicks of tools/realplay's step files), then
// the port's bot through playByMove (the test command "move", docs/TEST_COMMANDS.md), then at a game's end the name
// typed, Okay and the table. ?mode= is tutorial, endless, stomped, fossil, panic or timetrial (endless by default).
// Opened as .../dynomite.html?player=1&assets=1&fast=1&silent=1&driver=games/dynomite/tools/web/onegame.js&run=web-<mode>
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "endless";
var cap = Number(params.get("steps")) || 3000;	// the bot's moves at most, before the shots straight up
var started = Date.now();
// the menu's clicks after the title for each, as the native step files make them
var routes = {
  tutorial: [[486, 355], [488, 432]],
  endless: [[486, 213], [465, 160], [488, 432]],
  stomped: [[486, 213], [465, 230], [488, 432], [465, 162], [488, 432]],
  fossil: [[486, 213], [465, 295], [488, 432], [445, 200], [488, 432]],
  panic: [[486, 213], [465, 295], [488, 432], [445, 280], [488, 432]],
  timetrial: [[486, 213], [465, 360], [488, 432], [445, 265], [488, 432]]
};
if (!routes[mode]) return { error: "no such mode: " + mode };

async function clickAt(x, y) {
  await t.move(x - 5, y - 5);
  await t.wait(2);
  await t.click(x, y);
  await t.wait(30);
}

// the title waits for its loading to finish; a click on it then goes on to the main menu
var last;
for (;;) {
  last = await t.state();
  if (last.screen === "title") break;
  if (Date.now() - started > 60000) return { error: "no title", state: last };
  await t.wait(20);
}
await t.wait(200);
await t.shot(mode + "-title");
while ((last = await t.state()).screen === "title") {
  if (Date.now() - started > 120000) return { error: "the title stayed", state: last };
  await clickAt(320, 450);
  await t.wait(100);
}
await t.shot(mode + "-menu");
var route = routes[mode];
for (var i = 0; i < route.length; i++) await clickAt(route[i][0], route[i][1]);
await t.wait(300);
last = await t.state();
if (last.screen !== "game") return { error: "the game did not start", state: last };
await t.shot(mode + "-start");

// playByMove stops on two empty moves running; the bot has none for a while between the tutorial's messages (one fading
// in, the board's pause) and in a game while a blast, a power-up or a fossil's fall runs, so it is asked again while
// the game goes on, until twenty asks in a row bring no move
var play = await t.playByMove({ steps: cap });
var total = play.moves, idle = 0;
while (play.stop === "no move" && idle < 20 && Date.now() - started < 1800000) {
  last = await t.state();
  if (last.screen !== "game" || last.over) break;
  await t.wait(60);
  play = await t.playByMove({ steps: cap - total });
  total += play.moves;
  idle = (play.moves > 0) ? 0 : idle + 1;
}
play.moves = total;
// a game the bot is still winning at the limit is ended by real shots straight up, every forty updates, until the eggs
// pass the line
var ended = 0;
if (mode !== "tutorial" && play.stop === "limit") {
  for (last = await t.state(); last.screen === "game" && !last.over && ended < 1500; last = await t.state()) {
    if (!last.busy) {
      await t.move(400, 225);
      await t.click(400, 232);
      ended++;
    }
    await t.wait(40);
  }
  if (last.over) play.stop = "over";
}
if (play.stop === "no move" && last.over) play.stop = "over";
await t.wait(800);
await t.shot(mode + "-end");
var named = false;
if (mode !== "tutorial" && play.stop === "over") {
  // the name dialog (the low tables of the test profile make every score a high score), then Okay and the table
  last = await t.state();
  if (last.dialog === 6) {
    await t.type("bot");
    await t.wait(10);
    await t.shot(mode + "-name");
    await clickAt(352, 369);
    await t.wait(300);
    await t.shot(mode + "-table");
    named = true;
  }
}
last = await t.state();
return { mode: mode, moves: play.moves, ended: ended, stop: play.stop, named: named, last: last,
  seconds: Math.round((Date.now() - started) / 1000), updates: t.updates(), errors: t.errors.length };
