// Plays Atomica in the player-only page with real DOM input (docs/WEB_PLAYER.md, "Drivers"): the launch (the title screen's link),
// the tutorial boxes of the first game, and a whole game of each mode asked for. The port's own bot chooses every input (the test
// command "move", docs/TEST_COMMANDS.md: the link, the menu's start button, the hint boxes' check marks and Continue, the atoms
// and their cells, the level boxes, the name of the high score entry and its button, "main menu" after a game) and this sends it as
// the page's mouse and key events. Code plays; the session reads the result.
// Parameters of the page: games= a list of strategy, strategy-expert, action, action-expert (default strategy); seconds= the most to
// play in one game (900); blur= the number of moves at which the window loses the focus for 300 updates and gets it back (0: never);
// shots= every that many moves a shot is saved (0: only at the end of each game).
// idle= after that many moves in a game only wait on the board, so the game ends by itself (action games the bot keeps going for good).
// Opened as .../atomica.html?player=1&assets=1&fast=1&silent=1&driver=games/atomica/tools/web/onegame.js&run=web-atomica
// (a fresh profile for the tutorial: ?hold=1 is not needed, the page's test profile starts empty).
var params = new URLSearchParams(location.search);
var games = (params.get("games") || "strategy").split(",");
var limit = Number(params.get("seconds") || 900) * 1000;
var blurAt = Number(params.get("blur") || 0);
var shotEvery = Number(params.get("shots") || 0);
var idleAfter = Number(params.get("idle") || 0);
var started = Date.now();
var result = { games: [], notes: [] };

function note(text) {
  result.notes.push({ at: Math.round((Date.now() - started) / 1000), update: t.updates(), text: text });
}

// one MOVE's input, in order, as DOM events
async function send(m) {
  var steps = m.input || [];
  for (var i = 0; i < steps.length; i++) {
    var st = steps[i];
    if (st.move) await t.move(st.move[0], st.move[1]);
    else if (st.wait) await t.wait(st.wait);
    else if (st.click) await t.click(st.click[0], st.click[1], st.button || "left");
    else if (st.key) await t.key(st.key);
  }
  await t.wait(Math.max(1, m.updates || 1));
}

async function nextMove() {
  var lines = await t.command("move");
  var line = lines[lines.length - 1];
  if (!/^MOVE /.test(line)) throw new Error("move: " + line);
  var body = line.substring(5);
  return body === "null" ? null : JSON.parse(body);
}

async function ok(line) {
  var lines = await t.command(line);
  var last = lines[lines.length - 1];
  if (!/^OK /.test(last)) throw new Error(line + " -> " + last);
}

async function playOne(game) {
  var parts = game.split("-");
  await ok("set mode " + parts[0]);
  await ok("set expert " + (parts[1] === "expert" ? 1 : 0));
  var r = { game: game, moves: 0, kinds: {}, screens: {}, levels: 0, stop: null };
  var t0 = Date.now(), nulls = 0, lastSig = null, lastChange = 0, blurred = false;
  for (;;) {
    var s = await t.state();
    r.screens[s.screen] = (r.screens[s.screen] || 0) + 1;
    r.levels = Math.max(r.levels, s.level || 0);
    if (s.over) { r.stop = "over"; break; }
    if (Date.now() - t0 > limit) { r.stop = "timeout"; break; }
    var sig = JSON.stringify(s, function (k, v) { return k === "update" ? undefined : v; });
    if (sig !== lastSig) { lastSig = sig; lastChange = s.update; }
    else if (s.update - lastChange > 20000) { r.stop = "stuck: " + sig.substring(0, 300); break; }
    if (blurAt && !blurred && r.moves >= blurAt) {
      blurred = true;
      note(game + ": focus lost at move " + r.moves + ", screen " + s.screen);
      await t.blur();
      await t.wait(300);
      await t.focus();
      await t.wait(30);
      note(game + ": focus back, screen " + (await t.state()).screen);
    }
    if (idleAfter && r.moves >= idleAfter && s.screen === "board") {
      await t.wait(100);   // no more moves: the game runs on to its own end (the field fills); the bot takes over after it
      continue;
    }
    var m = await nextMove();
    if (m === null) {
      if (++nulls >= 3) { r.stop = "no move: " + JSON.stringify(s).substring(0, 300); break; }
      await t.wait(30);
      continue;
    }
    nulls = 0;
    r.kinds[m.kind] = (r.kinds[m.kind] || 0) + 1;
    await send(m);
    if (m.kind !== "wait") r.moves++;
    if (shotEvery && m.kind !== "wait" && r.moves % shotEvery === 0) await t.shot(game + "-" + r.moves);
  }
  r.state = await t.state();
  r.seconds = Math.round((Date.now() - t0) / 1000);
  await t.shot(game + "-end");
  return r;
}

try {
  await t.ready;
  for (var i = 0; i < games.length; i++) {
    var r = await playOne(games[i]);
    result.games.push(r);
    note(games[i] + ": " + r.stop + ", " + r.moves + " moves, level " + r.levels + ", score " + (r.state.score || 0));
    if (r.stop !== "over" || i === games.length - 1) break;
    // "main menu" is the bot's move on the page after a game; the main page then shows again
    var m = await nextMove();
    if (m === null) throw new Error("no move on the page after a game");
    await send(m);
    await t.wait(200);
  }
} catch (e) {
  result.error = String(e && e.stack || e);
}
result.errors = t.errors.length;
result.seconds = Math.round((Date.now() - started) / 1000);
return result;
