# Atomica Deluxe: the lite mod (2026-11-15)

Atomica has no lite mod: its one search is not worth a toggle. This file says what was looked at, what the search costs, which
faster method gives another route and which gives the same one, the proof of each, and why nothing is taken
(`docs/MOD_ROADMAP.md`, "The lite mod"). The candidate the survey named was the route search that runs on every pointer move while
a placed atom is held. It can be made much cheaper and still give another route, which would make the change lite, and the saving
cannot be told from noise in any state of play that was measured. Nothing in the port changed for this step except one line in the
profiling script (below), so the frozen baseline stands as it was, and nothing reads `Platform::LiteOn`.

Every figure below says the conditions it was taken under. The ones that depend on the renderer are marked, because the framework
session is re-deciding the default (see "Figures to redo"); the per-call cycle counts of the game thread do not depend on it.

## What was looked at

The game's logic is a field of 9 by 8 cells, and one search on it: the route an atom flies from its cell to the one under the
pointer (`PathFinder::Find` 0x424e80, with `Search` 0x4249d0 and the recursive `Recurse` 0x4246a0). `Grid::MouseMove` (0x40e710, in the
port `Grid.cpp`) calls it on every pointer move over the field while a placed atom is selected; the click that sends the atom uses
the route found by the last move. `PathFinder::Contains` (0x4248b0) scans the found route and is called once an update for each
atom while the ghost bonus is on. Everything else in an update moves what is on the screen: the atoms and their trails, the
molecules, the bonuses, the particles, the professor.

A scratch build of the port, never committed, read the thread's cycle counter (`QueryThreadCycleTime`, which stops while the thread is
switched out, so other sessions' games stay out of the figures) around each `Find` and counted the steps of `Recurse`. It ran the
search a second time on the same board by two other methods and compared what came back: the route's cells and their shapes, the
cost, the node count the atom's speed is taken from, and whether a route was found. One was a breadth-first search by cost; the other
is the exact method of the section after the next. The bot (`-autoplay`) does not move the pointer: for each of its candidate moves
it asks `Find` for every empty cell, so those games give many searches on real boards, though not the pointer's pattern; the
pointer's pattern came from scripted runs below.

The games, all with `-fast` under rundesk and counted by thread cycles (the game thread's own, the renderer's driver threads not in
them), on the SDL default of the day, Direct3D 11:

- eleven games of the port's bot: action and strategy with seeds 1, 2 and 3 (`-seed=N -autoplay=N -autoplaymode=...`, 20,000
  updates, 200 seconds of play; the strategy game of seed 3 ended at update 3,539), the same seed 1 with the expert bot
  (`-autoplayexpert=1`), and three games played on (action seed 1 to 48,960 updates, strategy seed 1 to 60,001, strategy seed 2
  with the expert bot to 60,001);
- a second batch of six of those games with the exact method and 2,000,000 random boards allowed (1,006,056 came out; see below).

For the share of the whole, the scripted runs under "How much it would save" ran the port in real time, pinned to one performance
core, with the original under the same script for comparison.

## What the search costs

| Computation | What it does, and when | Cost measured | Another method | Result |
|---|---|---|---|---|
| `Find` with `Search` and `Recurse` (0x424e80, 0x4249d0, 0x4246a0) | a depth-first search along the four straight lines from each cell, directions +x, +y, -x, -y, keeping the cheapest route (a cell costs 1, a segment 1 more) and cutting a branch only when it is not cheaper than what reached that cell before; run for each pointer move with an atom held, and not in any other way by the game | a search takes 1.1 thousand cycles at the median and 11.8 thousand on average (the bot's action game, seed 1, 40,257 searches, 99th percentile 183 thousand, slowest 426 thousand); 101.8 thousand on average in the strategy game of seed 1 (1,065 searches, median 65 thousand, slowest 461 thousand); 45.6 thousand on average for the searches that found a route, 136 thousand in the strategy game | a Dijkstra pass over the cells that fixes each cell's cheapest cost, then the same search restricted to arrivals at the cheapest cost and cut by a lower bound | the same route in every case compared: not lite (below) |
| the same with the original's pruning, but breadth-first by cost | | 2.0 thousand on average (the same 40,257), 4.2 thousand in the strategy game | | another route of the same cost in 0.5 to 1.4% of the bot's chosen moves in action games, 2 of 15 in strategy: lite, and not taken |
| `Contains` (0x4248b0) | each update for each atom while the ghost bonus is on: a walk along the route's cells, at most 50 | not timed; a few dozen steps for each atom | none needed | |
| the rest of `Grid::Update` and `Atom::Update` | movement, trails, collision, the molecules, the meter | none of it a search | none | |

The cost depends on how empty the board is. In the action game of seed 1 (the original search, the game thread's cycles, `-fast`) a
search on a board with 60 to 71 of the 72 cells blocked took 566 cycles on average (9,869 searches), with 48 to 59 blocked 1.6
thousand (17,182), with 24 to 35 blocked 15 thousand (3,053), with 12 to 23 blocked 89 thousand (4,260); in the strategy game of
seed 1 the searches on the pointer's line with fewer than 12 blocked took 197 thousand on average (16 searches). The slowest search
of all the runs took 501 thousand cycles. A pair of timer reads is 633 cycles (423 in the strategy runs), so the cheapest figures
are mostly the reading.

## The route can differ

`Recurse` keeps a route only when it is cheaper than the best so far, and the order in which it tries the directions decides which
of several routes of the same cost it meets first. A breadth-first search, or any search that meets them in another order, can
therefore return another route of the same cost. In every search of the eleven bot games the cost and the answer "found" were the same in both methods; the route itself was not:

| Game (scratch build, `-fast`, Direct3D 11 default) | searches | routes that differ | the bot's chosen moves | of them differing |
|---|---|---|---|---|
| action, seed 1, 20,000 updates | 40,257 | 184 (0.46%) | 567 | 3 |
| action, seed 2 | 39,831 | 228 (0.57%) | 561 | 8 |
| action, seed 3 | 41,535 | 189 (0.46%) | 585 | 5 |
| action, seed 1 to the end (48,960 updates) | 97,270 | 244 (0.25%) | 1,370 | 7 |
| strategy, seed 1 (the same up to 60,001 updates) | 1,065 | 82 (7.7%) | 15 | 2 |
| strategy, seed 3 (ended at update 3,539) | 2,627 | 54 (2.1%) | 37 | 0 |
| strategy, seed 2, expert bot | 71 | 14 (20%) | 1 | 1 |

On 1,006,056 random boards (a random density of blockers and of the two kinds of unfinished atom, a random start and target, both
modes, the ghost bonus on one board in twenty) the breadth-first search returned another route on 54,735 of the 459,241 that had a
route: 11.9% of them. So a board with two routes of the same cost is common, and a mod that changed the search order would change
what some players see.

What differs is the arrows the pointer draws, the cells the atom passes, the trail it leaves, and so the sparkle places. It also
differs in the node count the atom's speed is taken from (`Atom::SetPath` sets the speed to the number of route nodes, and the
atom moves that many times three pixels an update: 16 of the 40,257 searches in the action game of seed 1, 1 of the bot's 15 chosen
moves in the strategy game of seed 1), so a route with another number of turns changes the atom's flight by an update or two.
That follows from `Atom.cpp` and the scratch's comparison; no run was followed through the flight, since the change is not taken.
In the action mode the atom also waits at a node for the next one to be free of a spawning atom, so another route can wait at
another cell. In the ghost mode `Contains` colours the atoms near the route.

Two examples a person can see at once. In the strategy game of seed 1 (`-seed=1 -autoplay=1 -autoplaymode=strategy`), at update 739
on level 1, on a field of three blocks (row 3, columns 4 and 5, and row 4, column 5), an atom at column 1, row 5 and the pointer at
column 0, row 7: the original's route goes down to row 7 and then left (turning at column 1, row 7), the breadth-first one left to column 0 and then
down (turning at column 0, row 5). Both cost 5 and have three nodes. In the action game of seed 1 (`-autoplaymode=action`), at update
773 (the fourth sweep), an atom at column 5, row 7 sent to column 0, row 2 winds up through columns 6 and 5 and along row 2 in
the original, and goes right to column 7 and straight up and along row 2 in the other, at the same cost of 17, with four corners
against two.

## The exact method gives the same route

A search that never returns another route needs the original's order only where routes of the same cost tie, and the method below
keeps it there. First a Dijkstra search over the cells gives each cell its cheapest cost from the start, in the original's
cost (a segment and the cells it crosses). The cheapest cost to the target, D, bounds the original's search: it starts from the
best cost D + 1 instead of 100,000, and admits a branch into a cell only when the cost it arrives with is the cell's cheapest cost
and the lower bound (the Manhattan distance to the target, plus one segment, or two when it is not in line) keeps the total
within D. The rest of `Recurse` is the original's, so the directions are tried in the original's order, and the first route found
at the cost D is the original's.

Why it is the same route: a prefix of a cheapest route is a cheapest route to its last cell, so an arrival that is not at its
cell's cheapest cost can never lead to a cheapest route, and the original's rule that the first arrival to claim a cell at a cost
lower than before blocks later ones at that cost or higher never blocks a cheapest arrival, because the arrivals the restricted
search admits are all at cheapest costs and the first one the original meets in its order is also the first one the restricted
search meets. The early return when the target is reached is the same in both. This argument was checked, not trusted:
181,121 searches of the bot's boards (six games) and the 1,006,056 random boards gave the same cost, the same route cells and
shapes, the same "found" and the same node count in every one: 0 differences in 1,187,177 searches.

What it saves is on sparse boards, where the original's search is expensive: in the pointer's stretch below 279 thousand cycles a
search become 18.7 thousand. On a dense board it saves nothing, since the original already costs 0.6 to 1.6 thousand there (60 to
71 blocked cells, 566 cycles; 48 to 59, 1.6 thousand) and the breadth-first alternative, which does comparable work, cost 1.4 to 1.7
thousand. A method that cannot give another result is not lite; it would belong to the port's performance pass.

## How much it would save

The search's total share in the bot's games is small: the action game of seed 1 spends 476 M cycles on its 40,257 searches in 200
seconds of play, 2.4 M a second, against the 350 to 550 M a second that the process costs in the scripted stretch below (the closest
whole-process figure; the bot's games were not timed as a whole), so about half a percent; the strategy game of seed 1 spends 108 M on its 1,065
searches. The game itself asks for a search only on a pointer move over the field with an atom held, one for each move; the stretch below
has one in every update.

The case that maximises the share is an atom held while the pointer moves every update over a sparse board. A scripted run held
one placed atom and moved the pointer at every update over 2,000 updates (20 seconds of play at 100 updates a second, the
script `sweep-late` of the scratch's scenarios, `-fix=drawRandom` so that the boards of a real-time run are the same, the original
search and the exact one run on every pointer move in the same process). That gives 2,002 searches, a mean of 9.0 blocked cells,
and the original costing 559.5 M cycles (279 thousand a search) against 37.5 M (18.7 thousand) for the exact method, with no
different route in any of them. Against the process's own cost of the stretch, 7 to 11 G cycles in the runs below, the original's
search is 5 to 8%, and the exact method would remove about nine tenths of that (every pointer move of the 2,000 asks for a search).

To see whether that can be measured as the process's processor time, the build with the original search and the build with the
exact one ran in turn, six pairs, each pinned to one performance core (`tools/pincore.py`), in real time (`PERF_CLOCK=real`), with
`SDL_RENDER_DRIVER=opengl`, under rundesk, with the other sessions' games and builds running on the machine (the machine's busy
fraction in the runs' own log was 0.43 to 0.99), over the 2,000 updates of the stretch:

| pair | original | exact |
|---|---|---|
| 1 | 7.59 G | 6.87 G |
| 2 | 6.98 | 7.11 |
| 3 | 6.94 | 8.61 |
| 4 | 10.96 | 10.00 |
| 5 | 10.48 | 9.33 |
| 6 | 11.38 | 10.21 |

The medians are 9.04 and 8.97 G, a ratio of 0.993, and the pairs' own ratios run from 0.89 to 1.24: four of the six are lower by
about a tenth, one is level, one is higher by a quarter, and the runs of one build alone vary by as much as the difference expected between the
two builds (6.9 to 11.4 G for the original). The effect cannot be told from the machine's load. With the default
renderer, Direct3D 11, the same stretch (four runs, real time, pinned, the original search, the atom held) cost 17 to 33 G, of which
the driver's thread (`nvwgf2um.dll`) took 10.3 to 26.6 G against the game thread's 6.4 to 7.4 G, so half a gigacycle in the game thread's
share could not be seen in the process total.

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. The original's
search is 5 to 8% of the process in the one state built to maximise it (a held atom, the pointer in motion every update, a sparse
board), well under 1% in the bot's games, and nothing in the rest of play; and with the exact method, which does not change what
a player sees, the saving could not be told from the run-to-run spread of the process. The change that would be lite, a breadth-first
or other search order, saves less than the exact one (2 thousand cycles a search on average against a few thousand) and does change
what a player sees in about one in eight of the boards that have a route, so it is not taken. The exact method is not lite, and it
is not taken into the port either: the port keeps the original's search in the original's shape, as the readability rule has it,
and a change of about 70 lines with a proof by argument and a million comparisons (not an exhaustive one) buys a saving of
nothing measurable. If the performance pass ever needs the 5 to 8% of a sweeping pointer, the exact method is the one to use, and
the scratch's comparison is the test it would have to pass: the same route in every case.

The lite-mod step is therefore done with no mod.

## Figures to redo when the renderer default is settled

The framework session is re-deciding the renderer default (trunk commit ccec8c20 asks SDL for Direct3D 11 first; the
`SDL_RENDER_DRIVER` environment variable overrides it; a run's log line says `renderer direct3d11` even with
`SDL_RENDER_DRIVER=direct3d`). What depends on it, and what does not:

| Figure | Depends on the renderer? | Taken with |
|---|---|---|
| a search's cycles, the counts of routes that differ, the 1,187,177 comparisons | no: the game thread's own cycles and the game's own logic | `-fast` under rundesk, the Direct3D 11 default |
| the original's 559.5 M and the exact 37.5 M cycles for the 2,002 searches | no: the game thread's | real time, pinned, `SDL_RENDER_DRIVER=opengl` |
| the six pairs, 6.9 to 11.4 G, median ratio 0.993 | yes, through the process total | real time, pinned, `SDL_RENDER_DRIVER=opengl`, `-fix=drawRandom`, held atom, `LITE_PLAIN` build |
| the four runs with the atom held (17 to 33 G in all, game thread 6.4 to 7.4 G, driver 10.3 to 26.6 G) | yes | real time, pinned, the default (Direct3D 11), the original search, no `-fix=drawRandom` |
| the scenario with the pointer still (`idle`, three runs per renderer) by renderer | the point of the table: default 15 to 32.5 G (driver 9.2 to 26.4 G of it), `SDL_RENDER_DRIVER=direct3d` 7.4 to 9.1 G, `SDL_RENDER_DRIVER=opengl` 7.0 to 8.6 G; the game thread 5.2 to 6.5 G with Direct3D, 6.9 to 8.5 G with OpenGL, where the driver's work runs on it | real time, pinned, a build of the branch without the scratch edits (the trunk's of that day) |

Redoing them means the scratch build, which is not committed (a copy of the file, its hooks as a patch, the scenarios and the A/B
script is kept outside the repository, in the coordinator's notes folder, `atomica-lite-scratch`): the `LiteFind` wrapper around `Find`, the two alternative searches
and the comparison were about 630 lines in a file included from `PathFinder.cpp`, with three hooks (`Grid::MouseMove`, `Autoplay.cpp`'s
`Move`, `PathFinder::Search`). The conclusion would only change if a renderer made the game thread's share of the process grow by
an order of magnitude, since the search costs the same cycles whatever the renderer, and then only for the sweeping pointer.

## What changed in the repo

`games/atomica/tools/profile/perf.py` reads `PERF_ARGS` (more game arguments for the port, such as `-fix=drawRandom`). Nothing else
changed in the port, so its frozen baseline is as it was.
