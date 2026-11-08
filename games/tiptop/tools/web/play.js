// A whole game of TipTop as real DOM input, played by the port's own bot (docs/TEST_COMMANDS.md, "The bot behind move"): the
// body of an async function of byopop.test (t), for the player-only page (docs/WEB_PLAYER.md, "Drivers"):
//
//   http://127.0.0.1:18781/tiptop.html?player=1&assets=1&silent=1&fast=1&driver=games/tiptop/tools/web/play.js&mode=regular
//
// ?mode=regular|strategy|speed|timed picks the game (timed has no button: the driver reaches the menu with the bot's moves and
// then sends the standard test command goto timed). The loop asks `state` and `move` (t.state, t.botMove), plays the MOVE's
// input steps as DOM events (move, click, key, wait: the steps of docs/TEST_COMMANDS.md) and waits its updates, until the game is
// over, or ?steps=N moves (20000 by default). Options: ?fsat=N presses Alt+Enter after N moves (and again at ?windat=M), ?focusat=N
// takes the window's focus away after N moves (blur, 60 updates, focus), ?pauseat=N clicks the pause button after N moves. ?botseed=N is the bot's own dice. It returns what
// happened, with shots of the full screen state, the focus loss, the end and the high scores, if the page keeps them.
var params = new URLSearchParams(location.search);
var mode = params.get("mode") || "regular";
var limit = Number(params.get("steps")) || 20000;
var fsAt = params.has("fsat") ? Number(params.get("fsat")) : -1;
var windowedAt = params.has("windat") ? Number(params.get("windat")) : -1;
var focusAt = params.has("focusat") ? Number(params.get("focusat")) : -1;
var pauseAt = params.has("pauseat") ? Number(params.get("pauseat")) : -1;
var result = { mode: mode, moves: 0, waits: 0, focusRestored: 0, notes: [] };

async function command(line) {
  var lines = await t.command(line);
  return lines[lines.length - 1];
}

async function playMove(mv) {
  var steps = mv.input || [];
  for (var i = 0; i < steps.length; i++) {
    var st = steps[i];
    if (st.move) await t.move(st.move[0], st.move[1]);
    else if (st.click) await t.click(st.click[0], st.click[1], st.button === "right" ? "right" : "left");
    else if (st.key) await t.key(st.key);
    else if (st.wait) await t.wait(st.wait);
    else throw new Error("a MOVE step the driver does not play: " + JSON.stringify(st));
  }
  await t.wait(Math.max(1, mv.updates || 1));
}

async function alt_enter() {
  await t.keyDown("Alt");
  await t.key("Enter", { alt: true });
  await t.keyUp("Alt");
  await t.wait(30);
}

try {
  await command("set botseed " + (Number(params.get("botseed")) || 7));
  if (mode === "timed") {
    // the title and the menu by the bot's own moves; the timed mode's button is off the screen
    for (var guard = 0; guard < 50; guard++) {
      var s0 = await t.state();
      if (s0.screen === "menu") break;
      await playMove(await t.botMove());
    }
    var a = await command("goto timed");
    if (!/^OK/.test(a)) throw new Error(a);
  } else {
    var a2 = await command("set mode " + mode);
    if (!/^OK/.test(a2)) throw new Error(a2);
  }
  var nulls = 0, fullscreen = false, st = null;
  while (result.moves < limit) {
    st = await t.state();
    if (st.over) break;
    if (st.paused && st.inactive && !(focusAt >= 0 && result.moves === focusAt)) {
      // the page lost the focus by itself (a real blur: the pane is hidden, or another window was clicked): a player would click
      // back into the window, so the driver gives the focus back, as byopop.test.focus() does
      await t.focus();
      await t.wait(10);
      result.focusRestored++;
      continue;
    }
    if (fsAt >= 0 && result.moves === fsAt && !fullscreen) {
      await alt_enter();
      fullscreen = true;
      result.notes.push("Alt+Enter at move " + result.moves + ": fullscreenElement " + (document.fullscreenElement ? "set" : "none"));
    }
    if (windowedAt >= 0 && result.moves === windowedAt && fullscreen) {
      await alt_enter();
      fullscreen = false;
      result.notes.push("Alt+Enter back at move " + result.moves + ": fullscreenElement " + (document.fullscreenElement ? "set" : "none"));
    }
    if (focusAt >= 0 && result.moves === focusAt) {
      await t.blur();
      await t.wait(60);
      var sb = await t.state();
      result.notes.push("focus lost at move " + result.moves + ": paused " + sb.paused);
      await t.shot("focus_lost");
      await t.focus();
      await t.wait(60);
      await t.shot("focus_gained");
    }
    if (pauseAt >= 0 && result.moves === pauseAt && st.screen === "board" && !st.paused) {
      // the pause button, as a player clicks it; the bot's next move unpauses by a click on the pause image
      await t.click(65, 350);
      await t.wait(20);
      var sp = await t.state();
      result.notes.push("pause button at move " + result.moves + ": paused " + sp.paused);
      await t.shot("paused_by_button");
      result.moves++;
    }
    var mv = await t.botMove();
    if (mv === null) {
      if (++nulls >= 2) break;
      await t.wait(5);
      continue;
    }
    nulls = 0;
    await playMove(mv);
    if (mv.kind === "wait") {
      if (++result.waits > 20000) throw new Error("20000 waits: the game never took a move");
    } else result.moves++;
  }
  var fin = await t.state();
  result.over = !!fin.over;
  result.screen = fin.screen;
  result.level = fin.level;
  result.score = fin.score;
  if (fin.over) {
    await t.shot("gameover");
    await command("goto highscores");
    await t.wait(30);
    await t.shot("highscores");
  }
} catch (e) {
  result.error = String((e && e.message) || e);
}
result.updates = t.updates();
result.errors = t.errors.length;
return result;
