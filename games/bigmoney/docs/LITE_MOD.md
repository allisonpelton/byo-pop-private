# Big Money Deluxe: the lite mod (2026-11-15)

Big Money has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what the logic
costs, and why nothing is taken (`docs/MOD_ROADMAP.md`, "The lite mod"). The game's whole update, every search in it
included, is about 4 % of the processor time of a minute of play; the rest is the picture and the sound. No change to
the logic could cut a minute's cost by more than that, and two runs of the same build already differ by more.

## What was looked at

The board is 14 columns by 15 rows of coins. The game's logic is a handful of scans and floods over it, and the coins'
motion:

| Function | What it does, and when | A faster method, and whether it could give another result |
|---|---|---|
| `Board::FloodFill` (0x401720) and `Board::FindGroups` (0x401840) | the group of equal coins under a cell, by recursion over its neighbours; `FindGroups` floods every group twice (to count, then to mark each coin with its group's size) after each settle, so at most 210 cells, each visited twice | one labelling pass with a union of neighbours gives the same sizes and the same yes or no for a move left, so it would not be lite but a performance change, and it is not earned (below) |
| `Board::UpdateHover` (0x401e90) with `PieceAt` (0x401960) | when the pointer moves: the coin under it, by scanning the grid for the coin whose 29-pixel square holds the point | the cell from the point's position gives another answer while a coin falls between rows, since the scan finds a falling coin by where it is drawn: lite in principle, but the scan is 210 comparisons a move |
| `Board::Compact` (0x402b70) | after a group or a bomb goes: the coins fall, the columns close up, the groups are found again | none: its order of moves is what the player sees |
| `Board::UpdatePhysics` (0x408a60) and `UpdateLoosePieces` (0x409520) | every update: each falling or sliding coin, the loose coins and the bags | none: no search, one step for each moving piece |
| `Board::GenerateBoard` (0x40a110) | once a level: the board, and in Puzzle the backward build of its groups, each with attempts up to its target size (at most 240), and more only until it has two coins | none needed: once a level; the Winnable Puzzles mod already replaces the Puzzle build where a player wants it |

There is no search that tries moves ahead, no path finding and no solver in the original; the port's Puzzle Solver is a
mod of its own and is not part of the original's logic.

## What it costs

The scripted minute of the performance figures (GAME_NOTES, Part 6: the last 6000 updates of a 9000-update Action game
the bot played with seed 21) ran twice in real time on the software screen at 1x, with the framework's cycle split
(`-cyclesplit`, `PERF_ARGS` in `tools/profile/perf.py`; `tools/cyclesplit.py` from the minute's first update to its
last), each run pinned to a core of its own under rundesk, with two or three other sessions' games running:

| | Run 1 | Run 2 |
|---|---|---|
| The game's update (logic, script, widgets, music update) | 0.050 M cycles an update, 4.1 % | 0.047 M cycles an update, 4.3 % |
| The picture (drawing and present) | 1.036 M, 84.8 % | 0.919 M, 84.7 % |
| The sound thread | 0.066 M, 5.4 % | 0.061 M, 5.6 % |
| The loop (events, sleeps, the readings) | 0.070 M, 5.7 % | 0.059 M, 5.4 % |
| The whole process | 1.222 M cycles an update | 1.085 M cycles an update |

So the update costs about 290 M cycles a minute of the 6,500 to 7,300 M the process spends, and the two runs of one
build differ by 12 % in the total. Strategy and Puzzle run the same physics every update and their floods only after a
click, so their update is no larger. Removing the logic altogether would save less than the runs' own spread, and a
lite change can only make part of it cheaper: nothing would show as a gain in alternating pairs. The processor time
goes to the picture, which the performance pass already brought to about half the original's.
