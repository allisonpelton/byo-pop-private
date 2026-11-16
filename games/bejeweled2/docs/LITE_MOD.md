# Bejeweled 2 Deluxe: the lite mod (2026-11-15)

Bejeweled 2 has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what the
measurements bound, where a faster computation could differ, and why nothing was taken (`docs/MOD_ROADMAP.md`, "The lite
mod"). It is a shorter file than Bejeweled's (`games/bejeweled/docs/LITE_MOD.md`), because this game's functions were not
timed one by one; the reason is below.

## What was looked at

The game's logic is a board of 8 by 8 gems with three computations that scan it: the search for a move (`Board::FindMove`,
0x59543c, used when the board settles and for the hint), the test for a row of three (`Board::HasSet`, 0x595346, in the fill of
a new board) and the marking of sets (`Board::FindSets`, 0x5a7ec1, after a swap and when gems land). The rest of an update is a
few comparisons a gem (falling, the swap, the clearing, the timer bar and the mode's rules) and, in the 3D game, the effects
(sparkles, lightning, shock waves, the gems' flames), whose cost is mostly drawing. The survey in `docs/MOD_ROADMAP.md`
("Candidates found in the code") found nothing beyond small board scans here, and the performance pass lists no "Lite
candidates": it turned down no speed-up for changing what the game does.

## What the measurements bound

The performance pass (`GAME_NOTES.md`, "Performance") measured the game's own code on the whole at 0.14 ms an update, below 1% of
the process on every path and 2.5% on the software path of the first table, and a run of 6,400 updates of a Classic game
without waiting took 0.62 s of processor time on the port's software screen, 0.13 s on the GPU (2026-11-15, five pinned pairs;
the original 6.41 s). At a player's pace the game updates 100 times a second, so the whole of the game's code, every scan above
included, is at most 14 ms of each second, 1.4% of a core, against the 5 to 6% the idle board takes in all. Each scan is a part
of that. The functions were not timed one by one for this file; their sum is the bound, and it is already under what two runs
of the same exe differ by in the pairs (4.4 to 5.15% of a core for the same exe).

## Where a faster method could differ

Nothing was changed, so nothing was proved to differ. For the record, the three scans have the faster methods the roadmap names
(test only the rows and columns through the gems that changed; stop a search at the first move). The first gives the same answer
and would go into the port's performance pass if it were worth it; the second is what the game already does (the search stops
at the first move it finds, in the order of the original's loops), so there is no cheaper result to take.

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. The whole
logic is at most 1.4% of a core and the pairs spread by more than that, so no change to it could show. The port's time goes to
presenting the frame and to the framework's drawing, which a lite change may not touch since it keeps the look and the frame
rate (the existing `lean` mod, which caps the frame rate, is for that reason not lite). The game's lite-mod step is done with no mod.
