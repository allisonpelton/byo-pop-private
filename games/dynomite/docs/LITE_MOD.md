# Dynomite Deluxe: the lite mod (2026-11-12)

Dynomite has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing
was taken (`docs/MOD_ROADMAP.md`, "The lite mod"); the performance numbers are in `GAME_NOTES.md`, section 6.

## What was looked at

The game's logic is a grid of eggs 15 cells wide (1,500 cells, 100 rows of which a board uses about 14), a shot that
flies across it, and a few floods and scans run when a shot lands. It has no solver and no hint search; the bot behind
the `move` test command is test code, not part of a player's run. The cycle split of the port (GAME_NOTES, "The software
screen": the scripted minute of Time Trial at 640 by 480) puts the whole update at 0.044 M cycles an update, 3.0% of
the process's 1.49 M; the sampler of the performance pass put the board's own update at 0.2% of the port and the game's
code altogether under 2%. The picture and its present are the rest.

| Computation | What it does | A faster method | Result |
|---|---|---|---|
| `EggGrid::DropLoose` (0x416b80) with `CeilingFinder` (0x430cb0, 0x430e60), after every match, blast and row push | copies the grid, and for each egg with both cells above it empty copies it again and searches depth first for a path to the top row, giving up after 399 steps and then counting the egg as held; an egg with no path falls | one flood from the top row, each egg not reached falls | differs only where a search gives up on an egg that has no path: see below |
| `EggGrid::Match` (0x417bd0), when a shot lands | copies the grid and floods the shot's colour from where it landed | none needed | |
| `Egg::Update`'s probes (`EggGrid::Probe`, 0x4169c0), each update of a flying shot | eight cell lookups around the shot | none needed | |
| `EggGrid::CountRows` (0x415980), `HasColor` (0x415f00) | scans of the rows in use | counts kept up to date | exact |
| `EggGrid::PickColor` (0x4163e0), the next egg's colour | up to 50 draws of the app's picker for a colour in the lowest six rows | none: the draws stay | |

## DropLoose

This is the candidate `docs/MOD_ROADMAP.md` names for Dynomite. Its search counts a step at every call, the calls on
empty and already-visited cells included, and an egg makes up to six of them, so it gives up somewhere past 66 eggs.
Giving up counts the egg as held. For an egg that is held anyway, the answer is the same as a flood's. It differs only
for a loose group of more than about 66 eggs. Such a group would stay hanging in the original, where a flood would drop
it.

**What it costs.** It was instrumented in a scratch build, never committed. In the scripted minute of Time Trial,
`DropLoose` ran 18 times, at 42 k cycles a call (the median) and 64 k at most. That is 0.76 M cycles in the minute, of
which the searches were 0.36 M. The port's minute is 8.46 G cycles, so all of `DropLoose` is 0.009% of it.

The bot also played eight whole games through `tools/shots/botrec.py`, 400 moves each: Endless, Panic, Stomped and the
Fossil Challenge, with seeds 1 and 2. That is 72 minutes of play, 429,459 updates. Across them `DropLoose` ran 1,774
times and made 3,658 searches, and it dropped 2,255 eggs. It cost 40.9 M cycles in all, 0.57 M a minute of play
(0.007%), at 13 to 22 k cycles a call (the median of each game) and 215 k at most.

**Whether a game reaches the limit.** In none of those games did a search reach the limit. The longest took 154 steps
of the 399, in Panic, and the longest in the Time Trial minute took 130. The instrumented build would also have flooded the grid
from the top row at each search that gave up, to see whether the egg was really held; with none giving up, it never had
to. So the bot's games give no state where the flood would differ. Whether a player could cut a loose group of more than about
66 eggs off the top was not looked into further: it would matter only if the change were worth taking, and it is not.

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. All
of `DropLoose` is under a hundredth of a percent of the minute. The five-pair medians of the measurement resolve a few
percent of the process at best, and on a machine busy with other sessions' games the pairs spread by tens of percent.
Even within the update a shot lands in, `DropLoose`'s 42 k cycles are 3% of the update's 1.49 M. The other
computations in the table are smaller still, inside the 0.044 M an update that the whole update costs. The cost that is
left is the picture and its present, which a lite change may not touch, since it keeps the look, the frame rate and the
pace.

The flood would not earn a place in the port's performance pass either, for the same reason.

The lite-mod step is therefore done with no mod.
