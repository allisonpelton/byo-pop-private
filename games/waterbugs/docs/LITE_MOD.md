# Water Bugs: the lite mod (2026-11-15)

Water Bugs has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was taken
(`docs/MOD_ROADMAP.md`, "The lite mod"). The processor-time measurements are GAME_NOTES, "Processor time, like for like" and "The surface
presenter".

## What was looked at

The logic is small beside the picture. In the earlier breakdown of the main thread (198 M cycles a second, before the surface presenter) the
game's steps and their drawing into the 16-bit screen took 37, and the drawing is most of that; the rest was the host's change scan, widening,
upload and present, which are not the game's logic and which a lite change may not touch (it keeps the look, the frame rate and the pace). The
game's logic runs one 1 ms step ten times per 10 ms update, and the computations in it:

| Computation | Where | Cost | Faster method | Result |
|---|---|---|---|---|
| The flood after a closed beam, `Cell_Flood`, a recursive four-neighbour fill over the board's open cells | `WBCell.cpp`, called from `WBShooter.cpp` when a beam closes | at most 50 x 37 = 1,850 cells, once per closed beam (a few a minute) | an explicit stack or a scanline fill | the same cells marked in any order, so the same result in every state: not lite |
| Fills and captures (`Fill_Update`, `Cell_UpdateFill`, `Cell_UpdateCapture`) | `WBCell.cpp`, `WBLevel.cpp` | a few cells a step, only while a fill runs | none needed | |
| `Level_Blast`'s square (up to the radius's square of cells) and its tidy-up of lines | `WBCell.cpp` | once per blast | none needed | |
| Bug, beam, powerup and particle updates | `WBEnemy.cpp`, `WBBeam.cpp`, `WBPowerup.cpp`, `WBEmitter.cpp` | a dozen objects at most, a few hundred cycles each | none needed | |

Nothing searches or solves: there is no path-finding, no hint search and no solver, and every list the logic walks is short.

## Why none is lite, and why none is taken

The one computation over the whole board is the flood, and it marks the same set of cells under any faster order (the set of open cells connected
to the starting cell does not depend on how it is walked), so a faster flood is an exact change for the performance pass, not a lite one, and at
one flood of under 2,000 cells per closed beam it costs a few tens of thousands of cycles each time against the 100 M or more the process spends
every second: no round of alternating pairs could show a gain, and the roadmap requires a measured one for every change. Everything else the
logic does is a few objects or a few cells a step. No change here would play differently, and none would save processor time that a measurement
could see.

The processor time that is above the original's at the default window was the renderer path, which the surface presenter has removed at the
original's own window size; that is the host's drawing, outside the lite mod for the same reason.

The lite-mod step is therefore done with no mod.
