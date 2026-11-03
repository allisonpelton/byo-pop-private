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

Series compared: action 9000 updates (levels 1 and 2), strategy to its first level and a half, and puzzle
9000 updates up to its game over, the name entry and the high-score screen. Not compared: medium and hard
difficulty, stampede levels, a series of several games.

### Result

| Mode | Result |
|---|---|
| Puzzle (260 changes of state, score, level) | identical, including game over, name entry and the high-score screen |
| Strategy, level 1 | identical (46 changes) |
| Action, levels 1 and 2 | identical except the start of level 2 (see Oddities): the water, the end of the get-ready and the end of the level are 2 and 1 updates off |

### Changes (each is the original's own logic, from the Ghidra export)

| Change | Cause in the original | Commit |
|---|---|---|
| A falling animal's cell gets a **new piece** copied from the lower one; the old lower piece is deleted | 0x40407c: it copies the position, type, alpha, offsets, flags, fall speed and path into a new piece (blink, scale, flash, pending and match fields start at their defaults) and keeps the partner link only outside puzzle and stampede. The port swapped the two pieces, so a swapped piece kept its blink and the per-update random draws (one per piece with blink 0) came out in another number | ace140c |
| Flights to the ark and the fading of abandoned animals use the original's arithmetic | 0x405408 and 0x405440: int-truncated distances, a double square root, the step `(dx / dist) * 5.3` with a floor of 1, a clamp written with an unordered compare (the x87 treats a NaN as "not greater", so a zero distance snaps the animal to its target); on a zero or NaN position the piece becomes empty | ace140c |
| The ark rises to water - 0x37 in the get-ready | the original takes the water level before the +3 step and subtracts 0x34 | ace140c |
| The next-level screen adds 1 to its drawn tip in strategy | `mTip = rand() % kinds; if (mode == strategy) mTip++` (the port had a dead `mDifficulty == 0 && false`) | ace140c |

The changes alter the bot's games, so the pins of `tools/check.sh noahsark` and the tide-0 line of the mod tests moved
(`shared:` commit 0513cf6; the old pins were the port's own earlier output, which proved nothing about the
original). There is no frozen baseline of this port yet: that is a later item of the pass. The four mods' tests pass.

The "Animals move around bizarrely when matched" fault, fixed in 4ce5580: the walking animals' pixel positions are not
among the watched values, so it was not confirmed against the original here. What the comparison does show is that the
board, the moves, the scores and the states of every compared level agree, so the walk's logic (which pairs join, when
they land) is the original's; the drawing position of a walking animal needs the screen comparison of phase two.

### Oddities (not fixed; how to see each)

- **Strategy level 2 has another board.** Level 1 matches, then the original's level-2 board differs from the port's
  (a sample of the first column: 3,1,3,1 in the original, 2,2,1,2 in the port), so the replayed clicks no longer fit.
  The generator is one draw (or more) off by the end of level 1; the formulas of `NewLevel` (0x401580) are the same
  in both. The candidates are the per-update draws of the level-done and next-level phase with animals left on the
  board (blink, Noah, fish), which act and strategy handle alike but strategy leaves pieces on the board. No cause
  found in the hour given. See it with `compare_mode.sh st2 build/noahtest/og/menu_s.txt strategy 20000` (the
  scripts live under build/ and are not committed; the clicks are the title, strategy, difficulty, the tutorial's
  confirm).
- **Action level 2 and later: the get-ready ends 2 updates later in the original** (323 updates from the new level to
  play; the port takes 321, level 1 takes 322 in both). Its end is the last animal's landing plus the 139 updates the
  fall speed takes to decay, so the landing times differ a little. Not the clock (10 ms per update in both; the
  400 ms release comes 41 updates after the level's start in both), not the board's update counter (the original's
  `mUpdateCnt` is the app's counter minus 230 throughout) and not the grid (the states of the level agree). Seen
  through the water: the original's click that ends the get-ready sets the state without snapping the water to
  its target, so the water is 1 higher for the rest of the level and the won state lasts one update longer (an
  extra +2 of bonus score). See it with `compare_mode.sh act1 build/noahtest/og/menu.txt action 9000` (4 rows differ:
  update 4804 and the end of level 2). Because the bot clicks by update, everything after it in the series
  (levels 3 and up, stampede at level 4) cannot be compared by replay until this is explained.
- The original's registered state and its high-score file are untouched by these runs: each run starts from a copy.

### Deviations left

- The port's win-state water goes down at the same pace, but the original's level-end check one update later (above).
- The comparison watches 4 values; pixel positions, the fish and bubbles, the sparkles and the texts are not compared
  (phase two compares screens).
- Stampede, medium and hard, and a series of games remain to compare once the get-ready difference is understood.
