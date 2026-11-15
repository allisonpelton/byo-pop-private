# Rocket Mania Deluxe: the lite mod (2026-11-12)

Rocket Mania has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was taken
(`docs/MOD_ROADMAP.md`, "The lite mod"); the performance numbers are in GAME_NOTES, "Performance".

## What was looked at

The game's logic is a 6 by 9 grid of tiles (54 cells) with a handful of scans and one flood over it; it has no solver, no hint search
and no search whose answer depends on the order it looks in. The cycle split of the port (`-cyclesplit`, GAME_NOTES "Performance": one
23 s run of Arcade play, 3D and 2D) puts the game's whole update function, which holds every one of the computations below, at 0.027 M
cycles an update in 3D and 0.028 in 2D, 1.7% and 1.9% of the process's 1.5 M. The picture is 90% of it, and the Direct3D 11 driver's
thread alone is half of that.

| Computation | What it does | A faster method | Result |
|---|---|---|---|
| `Board::Connections` (0x426370), each update | clears every tile's path and finds the fuses again from both edges (`Trace`, 0x426070, and `Flood`, 0x4251b0), then dies down the tiles' shake | a flag set when a tile turns or the grid changes, so an idle board skips it | exact: the same paths, in the same order |
| `Board::Flood` | marks the tiles of one path, 54 at most | none needed | |
| `Board::FindFreeTile` (0x423d10, 0x423e20) | scans from a random cell for a tile that takes a special, at most 54 cells; the two random draws come first | none: the draws stay | |
| `Board::AllTiles` (0x426eb0), `BothEnds` (0x426030), `LowestEmpty` (0x427b70), `CountBurning` (0x4246c0) | scans of the grid, of one column and of the list of objects | counts kept up to date | exact |
| the bot behind `move` | test code: not part of a player's run | | |

## Why none is lite

A lite change is one that may give another result, proved by a state the game can reach. Each method above gives the same result as
the original's in every state: a path is the connected tiles of the grid, and a flag that says "nothing changed" skips only a re-trace
that would find what it found before. So there is no state to show where they differ: they are exact changes, which belong in the port's
performance pass if they earn a place there. A variant that would differ (another order of tracing) gains nothing a measurement can
see, so no difference was looked for.

## Why none goes in the performance pass

All of them together are at most the 0.027 M cycles an update that the whole update costs (1.7% of the port's process, 0.6% of the
original's 4.2 M), and the port is at 0.25 to 0.78 of the original's processor time in every pair of the measurement. A change that
removed `Board::Connections` altogether could not be shown to cut the processor time of a stretch, which the roadmap requires of every
change: the five-pair medians resolve a few percent of the process at best, and the pairs spread by tens of percent on a busy machine.
The cost that is left is the picture and the display driver's thread, which a lite change may not touch (it keeps the look, the frame
rate and the pace).

The lite-mod step is therefore done with no mod.
