// port: presses of the arrow keys while the explorer moves, in the player-only page, as the DOM's own events (the check of 2026-11-11:
// `GAME_NOTES.md`, "Fix candidates", a quick tap of an arrow key). The page is real time (no fast=1, and a hidden pane needs
// requestAnimationFrame shimmed), the game is held until the driver starts it:
//   mummymaze.html?player=1&assets=1&hold=1&silent=1&seed=21   then   byopop.test.run("games/mummymaze/tools/web/keytiming.js", args)
// Each trial starts chamber 1 of a classic game (`level 1`), presses an open arrow key, presses the opposite one again d updates
// after the first key was let go, waits for the turn to end and reads how many moves the game took: 2 when the second press was
// taken, 1 when it was dropped. d runs through the explorer's walk, the mummy's turns and the update the board is ready again, and
// the last trials press at the update the board is ready and one and two updates after it.
// args: offsets (the d values), holdsMs (a list: how long the second key stays down; null is the kit's key, one frame, and 0 a
// keydown and a keyup in the same task; every value runs all the d), trace (each trial keeps its state before the second press and
// at the first polls after it).
// The result is {trials: [...], valid: n, dropped: n}; a trial in which the explorer was caught before the second turn began or in
// which a mummy stood on the cell of the second step (the game refuses that step) is not valid.
const offsets = (args && args.offsets) || [0, 10, 30, 58, 60, 65, 70, 100, 130, 134, 135, 136, 137, 140];
const holdsMs = (args && args.holdsMs) || [null];
const trace = !!(args && args.trace);   // keep each trial's [update, board state, moves, explorer, mummy] before the second press and at the first 14 polls after it
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const lastOf = (lines) => lines[lines.length - 1] || "";

async function accepted(line, tries) {
  for (let i = 0; i < tries; i++) {
    const r = lastOf(await t.command(line));
    if (/^OK/.test(r)) return;
    await t.wait(5);
  }
  throw new Error(line + " was not accepted");
}

async function ready() {
  for (let i = 0; i < 2000; i++) {
    const s = await t.state();
    if (s.screen === "board" && s.board && s.board.ready) return s;
    await t.wait(2);
  }
  throw new Error("the board is not ready");
}

// the second key: held for a frame (the kit's key), for ms milliseconds, or not at all (keydown and keyup in one task)
async function tap(key, ms) {
  if (ms === null) {
    await t.keyDown(key);
    await t.keyUp(key);
  } else if (ms === 0) {
    t.keyDown(key);
    t.keyUp(key);
    await t.frame();
  } else {
    t.keyDown(key);
    await sleep(ms);
    t.keyUp(key);
    await t.frame();
  }
}

// the keys of a trial: an open direction of the explorer's cell that leads away from the mummy, and the way back (the walls of
// the state are a hex digit a cell, row by row: 1 north, 2 south, 4 east, 8 west)
function keysOf(board) {
  const n = board.size, [x, y] = board.explorer, m = board.mummy0;
  const open = (cx, cy, bit) => (parseInt(board.walls[cy * n + cx], 16) & bit) === 0;
  const dirs = [["ArrowLeft", 8, -1, 0, "ArrowRight"], ["ArrowRight", 4, 1, 0, "ArrowLeft"], ["ArrowUp", 1, 0, -1, "ArrowDown"],
                ["ArrowDown", 2, 0, 1, "ArrowUp"]];
  let best = null;
  for (const [key, bit, dx, dy, back] of dirs) {
    const nx = x + dx, ny = y + dy;
    if (nx < 0 || ny < 0 || nx >= n || ny >= n || !open(x, y, bit)) continue;
    const dist = m[2] ? Math.abs(nx - m[0]) + Math.abs(ny - m[1]) : 0;
    if (!best || dist > best.dist) best = { key, back, dist };
  }
  return best;
}

// a fresh chamber 1: `level 1` on a classic board or the menu, the lost panel's try-again link first when the explorer was caught
async function freshBoard() {
  for (let i = 0; i < 400; i++) {
    let s = await t.state();
    if (s.screen === "lost") {
      await t.playMove(await t.botMove());
    } else if (/^OK/.test(lastOf(await t.command("level 1")))) {
      return await ready();
    }
    await t.wait(5);
  }
  throw new Error("no fresh board");
}

const trials = [];
await t.start();
const first = await t.state();
if (first.screen === "title" || first.screen === "loading") {
  await accepted("goto menu", 2000);
  await t.wait(20);
}
for (const holdMs of holdsMs) {
  const plan = offsets.map((d) => ["after-release", d]).concat([["at-ready", 0], ["at-ready", 1], ["at-ready", 2]]);
  for (const [tag, d] of plan) {
    let s = await freshBoard();
    const u0 = s.update, moves0 = s.moves || 0, start = s.board.explorer.join(","), ks = keysOf(s.board);
    await tap(ks.key, null);
    if (tag === "after-release") {
      if (d) await t.wait(d);
    } else {
      for (let i = 0; i < 4000; i++) {
        s = await t.state();
        if (s.board && s.board.ready && (s.moves || 0) >= moves0 + 1) break;
        await t.wait(1);
      }
      if (d) await t.wait(d);
    }
    const before = await t.state();
    const row = (x) => [x.update - u0, x.board.state, x.moves || 0, x.board.explorer.join(","), x.board.mummy0.join(",")];
    const uPress = before.update - u0, seen = [row(before)];
    // the game refuses a step onto a living mummy's cell (CanMove): a second press toward him is not a dropped press
    const step = { ArrowLeft: [-1, 0], ArrowRight: [1, 0], ArrowUp: [0, -1], ArrowDown: [0, 1] }[ks.back], [bx, by] = before.board.explorer, m = before.board.mummy0;
    const blocked = !!m[2] && m[0] === bx + step[0] && m[1] === by + step[1];
    await tap(ks.back, holdMs);
    for (let i = 0; i < 4000; i++) {
      s = await t.state();
      if (trace && seen.length < 15 && s.board) seen.push(row(s));
      if ((s.board && s.board.ready && s.update > u0 + 140) || s.update > u0 + 600) break;
      await t.wait(2);
    }
    trials.push({ holdMs, tag, d, uPress, moves: (s.moves || 0) - moves0, start, keys: ks.key + "," + ks.back, explorer: s.board && s.board.explorer, screen: s.screen, blocked, seen: trace ? seen : undefined });
  }
}
// a trial in which the explorer was caught before the second turn began, or a mummy stood on the cell of the second step (the game refuses
// such a step), says nothing (the lost board ends the queue)
const valid = trials.filter((r) => !(r.screen === "lost" && r.moves < 2) && !r.blocked);
return { trials, valid: valid.length, dropped: valid.filter((r) => r.moves < 2).length };
