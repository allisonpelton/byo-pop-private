# Noah's Ark Deluxe: the identical-play pass

"A game is done when it plays identically to the original" (CLAUDE.md, 2026-10-27). This file records the pass in
phases. Phase one is play: with the same input, a game plays out the same in the original and in the port (boards,
random draws, moves, scores, levels, timers, endings). Phase two (screens, sounds, focus, the web build) is not
started.

## Phase one (2026-11-03)

### How it was compared

The original (`WinNoah.exe`) runs only through `tools/origrun` (`docs/ORIGRUN.md`), from a copy, on the virtual clock
(`--clock virtual`, exactly 10 ms per update, measured: the game's tick at a level's start is 10 x update + a constant
in all three levels of a run). Its C library generator is pinned by `--seed` (a patch note kept out of the
repository, `tools/origrun/patches/noahsark.txt`), and the port runs with `-seed=N -fast`; the two generators are the
same MSVC `rand`, so the same seed gives the same draws.

What is compared is the game's state, read from the original's memory by `--watch`: the board's state
(`board+0x310`: get ready, playing, won, lost, level done, ...), the score (`app+0x474`), the level (`app+0x43c`) and
the mode (`app+0x444`). The port writes the same four values in `-statelog=<file>` whenever one changes. The port's
bot plays a game by itself (`-autoplay`, with `-autoplaymenu` so that a script does the menus); its clicks are written to its log,
`games/noahsark/tools/origrun_replay.py script` turns them into an origrun script (the bot's update U is the
original's counter U+1, because the port's log is written after the update), and the original gets exactly those
clicks at the same counter values. `origrun_replay.py compare` then checks, update by update, that the two logs hold
the same rows, in both directions. `games/noahsark/tools/compare_mode.sh <name> <menu script> <mode> <cap>` does the whole
series in one command (the port's run, the conversion, the original's run inside Windows Sandbox, the comparison).
`-statelogcells=` adds a few cells' types and the water's level to the log.

Runs of the original on the host stopped receiving input at some point in the session (cause not found: other
sessions ran their own origruns at the time), so the original runs inside Windows Sandbox (`SANDBOX=1`, one minute
per run).

Series compared: action 9000 updates (levels 1 and 2), strategy to its first level and a half (since the walker fix
below: to its first two levels and up to the start of the third), puzzle 9000 updates up to its game over, the name entry
and the high-score screen, and the first level of an action game on medium and on hard difficulty (the same clicks,
the difficulty button of the dialog being the only change). Not compared: stampede levels and a series of several
games (both lie behind the get-ready difference below, which stops the replay at level 2 or 3).

The random draws are compared too, since a draw taken by another consumer is the first thing that shows when two games
part. The port counts its draws (`CrtRandCount()`, `CrtRandSnapshot()`) and `-statelogcells=r` or `c` writes the
generator's state after every 512th draw, or the number of draws made at each update; the original's `rand` and
`srand` are replaced in its copy by ones that keep the state, a count and the same snapshots (a patch note of the
copy, kept out of the repository with the other patches). `games/noahsark/tools/rngcmp.py` maps a snapshot to its
draw number (running the generator from the seed) and says at which update the two games reached it, and
`games/noahsark/tools/rngc.py` lists the updates in which the two made a different number of draws. The original keeps at
most about 4000 rows of a watch, so a run is read in windows (the snapshot from a chosen draw on).

### Result

| Mode | Result |
|---|---|
| Puzzle (260 changes of state, score, level) | identical, including game over, name entry and the high-score screen |
| Strategy, levels 1 and 2 | identical (every state, score, level); the board of level 3 differs, see Oddities |
| Action, levels 1 and 2 | identical, except the start of level 2 (see Oddities) |
| Action, level 1 on medium and on hard | identical |

### Changes (each is the original's own logic, from the Ghidra export)

| Change | Cause in the original | Commit |
|---|---|---|
| A falling animal's cell gets a **new piece** copied from the lower one; the old lower piece is deleted | 0x40407c: it copies the position, type, alpha, offsets, flags, fall speed and path into a new piece (blink, scale, flash, pending and match fields start at their defaults) and keeps the partner link only outside puzzle and stampede. The port swapped the two pieces, so a swapped piece kept its blink and the per-update random draws (one per piece with blink 0) came out in another number | ace140c |
| Flights to the ark and the fading of abandoned animals use the original's arithmetic | 0x405408 and 0x405440: int-truncated distances, a double square root, the step `(dx / dist) * 5.3` with a floor of 1, a clamp written with an unordered compare (the x87 treats a NaN as "not greater", so a zero distance snaps the animal to its target); on a zero or NaN position the piece becomes empty | ace140c |
| The ark rises to water - 0x37 in the get-ready | the original takes the water level before the +3 step and subtracts 0x34 | ace140c |
| The next-level screen adds 1 to its drawn tip in strategy | `mTip = rand() % kinds; if (mode == strategy) mTip++` (the port had a dead `mDifficulty == 0 && false`) | ace140c |
| A walking pair keeps its own cell when the animal above it falls past it | 0x40407c copies the lower piece, position included, into the falling animal's slot, and moves only the old upper piece (its `mY` + 1, its pixel and offset one cell down). A walker that has left its cell holds the cell it walks to in `mY`, and the copy keeps it. The port also set the old piece's `mY` to the slot it sits in, so a walker was moved back into its first cell: its walk, its partner link and its blink draws (one draw per piece with blink 0) went wrong, and every later draw of the game came out in another place. This is the "animals move around bizarrely when matched" fault | dead376 |

The changes alter the bot's games, so the pins of `tools/check.sh noahsark` and the tide-0 line of the mod tests moved
(`shared:` commit 0513cf6; the old pins were the port's own earlier output, which proved nothing about the original),
and the strategy pin moved again with the walker fix (level 5 done at update 19538, score 6380; action and puzzle did
not change). The frozen baseline now exists: `build/baseline-noahsark/` holds a copy of the port as of this pass (not
committed, like the other baselines) and the noahsark suite plays the action, strategy and puzzle games with it too and
needs the same lines and screens ("baseline identical"). The four mods' tests pass.

The "Animals move around bizarrely when matched" fault: confirmed against the original by state. Watching a pair's cells
in the original (the type, and the piece's path index and offsets in the walking pair's cell) showed a walker staying in
the cell it walks to while the animal above falls past; the port moved it back into the slot it was in. The walker fix
above makes the pieces' cells, and with them every later draw of the game, the same: the strategy run's board of level 2
was another one before it, and is the original's after it. The drawing of a walking animal (pixel positions) is still for
the screen comparison of phase two.

### Oddities (not fixed; how to see each)

- **The get-ready of the second level on differs from the original's.** This is what stops the replay at level 2 (action)
  or 3 (strategy); everything below was measured on strategy level 2 (`compare_mode.sh st3 build/noahtest/og/menu_s.txt
  strategy 20000`, the scripts live under build/ and are not committed). The original's get-ready lasts 323 updates from
  the new level to play, the port's 321 (level 1 takes 322 in both). Found by the draws:
  - The two games make the same number of draws in every update of level 1 and of the first 30 updates after level 2's
    release of waiting animals starts (the 41st update after the level's start: `400 < tick - start`). Then the original makes
    one more draw in a few updates (a retry of the release's column pick, which draws again when the column has no waiting
    animal), and from about 15 updates on the difference grows (3 draws by update 4575, 12 by 4838). The board of level 3,
    drawn from the generator at its start, is then another one, so the replayed clicks no longer fit.
  - The release of the waiting animals is where it starts: in the original the first animal of the level is released 4 updates
    later than the port's (the pending flag of the first column's lowest animal comes at the 45th update after the start, the
    port's at the 41st), the animals of the column picked are released every second update, and the order in which the
    columns are first picked is another one. With the generator's values the picks of the original come from the draw one
    before the port's pick (the 81st draw of the update, after 80 blink draws; the port's is the 82nd, after 81), which
    fits seven picks watched in the original at one offset, but nothing in the export puts the pick before the last
    animal's blink draw (the release is called after the loop that makes the blink draws, in both). The equal
    draws per update (81 blink + 162 for the get-ready loop, +1 for the pick) hide this. No cause found in the time given
    (several hours; the generator's snapshots, the get-ready loop, `FUN_00406750` and `FUN_00406710` were read against
    the port's code and equal).
  - Not the clock (10 ms per update in both), not the board's update counter (the original's `mUpdateCnt` is the app's
    counter minus 230 throughout), not the grid (the states of the level agree), not the water in strategy (it agrees).
  - Seen through the water in action: the original's click that ends the get-ready sets the state without snapping the water
    to its target, so the water is 1 higher for the rest of the level and the won state lasts one update longer (an extra
    +2 of bonus score). See it with `compare_mode.sh act1 build/noahtest/og/menu.txt action 9000` (4 rows differ: update 4804
    and the end of level 2).
  - A click on the "next level" button acts one update later in the original than a click on the board (the button acts on
    the release; origrun delivers it one update after the press). Putting the replay's click at the update it was made in
    brought the draws of the release into step (the replay with that mapping makes the draws of the first 30 updates after the
    release equal) but broke the action replay of level 2 (the clicks that end the get-ready found no pair), so the script
    keeps its +1 for every click.
  Because the bot clicks by update, the levels after it (action 3 and up, stampede at level 4, a series of games) cannot be
  compared by replay until this is explained; the same fix may be what the user's reported faults in later levels need.
- The original's registered state and its high-score file are untouched by these runs: each run starts from a copy.

### Deviations left

- The port's win-state water goes down at the same pace, but the original's level-end check one update later (above).
- The comparison watches 4 values; pixel positions, the fish and bubbles, the sparkles and the texts are not compared
  (phase two compares screens).
- Stampede, the later levels of medium and hard, and a series of games remain to compare once the get-ready difference is understood.
