# Noah's Ark Deluxe: the lite mod (2026-11-14)

Noah's Ark has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what each search costs,
which of the faster methods would give another result and the proof of it, and why nothing is taken (`docs/MOD_ROADMAP.md`, "The
lite mod"). The two candidates the survey named came out differently. The check for a move left gives the same answer by a faster
method, so it is not a lite change at all. The route search can give another route, so it would be one. Neither is worth making: every
search in the game together costs about 8 M cycles of the 14,500 M that a minute of play costs.

## What was looked at

The game's logic is a board of 9 by 9 cells and four kinds of search on it: whether a pair that can be joined is left, the route a
joined pair walks, the pair a hint flashes, and the level generator's test that a new board can be cleared. Puzzle mode and the
Stampede round have scans and one small flood instead. Everything else in an update moves what is on the screen: the animals that
fall, walk and fly to the ark, the water, the fish, the bubbles, the sparkles and the score texts.

A scratch build of the port, never committed, read the thread's cycle counter (`QueryThreadCycleTime`, which stops while the thread
is switched out, so other sessions' games stay out of the figures) around each of these and counted their calls. It played fifteen
games of the port's bot with `-fast` under rundesk: action and strategy on easy with seeds 1, 2 and 3 and on hard with seeds 1 and
2 (`-seed=N -autoplay=N`, 20,000 updates each, so 100,005 updates or 16.7 minutes of play for each mode), puzzle on easy with seed 1
and on hard with seed 2 to the end of the game (17,633 updates), and three games in which the bot stops (`-autoplaystop`, at
update 3000 in action and strategy, 1500 in puzzle) and the board waits, so that the hint comes up every 500 updates. The first of
them is the check's action game and ended on the check's line (level 4 done at update 17369, score 6074), so the timers did not
change the game. One reading costs a few hundred to a few thousand cycles, so the smallest figures below are mostly the reading.

For the size of the whole, the same action game ran twice in real time with the framework's cycle split (`-cyclesplit`, updates
1400 to 7400, a minute of play): the process cost 2.35 and 2.48 M cycles an update, 14,100 and 14,900 M a minute, of which the
picture (drawing, the present and the graphics driver's threads) was 93%, the mixer 3.5% and the game's whole update 1.6 and 1.9%.
The two runs differ by 5.6%, which is what a comparison of two builds would have to beat. Three or four game processes of other
sessions were running throughout.

| Computation | What it does, and when | Cost measured | A faster method | Result |
|---|---|---|---|---|
| `Board::HasMove` (0x406aa0) with `HasPartner` (0x406cd0) and `Reachable` (0x4026d0) | every 48 updates in action and strategy, and every 6 while the last animals are being abandoned: for each animal and each other animal of its kind, a fresh breadth-first search from the other | with a pair on the board 17.8 thousand cycles a call in action (1,541 calls, the slowest 107 thousand) and 12.7 thousand in strategy (1,980 calls, the slowest 87 thousand); with no pair left and animals standing 14.2 thousand (183 calls, the slowest 49.8 thousand); 1.7 M cycles a minute | number the connected regions of open cells once, then note which regions each kind's animals stand beside: 4.5 to 4.9 thousand cycles a call, 0.5 to 0.6 M a minute | the same yes or no, shown below: not lite, and not earned as a performance change |
| `Board::FindPath` (0x403370) with `SearchPath` (0x4036f0), from `BuildTrail` (0x402ec0) | when a pair is clicked, and when the pointer comes onto an animal the picked one can reach: a depth-first search along straight runs that keeps the cheapest route, 7,500 to 8,000 steps a call and 63,233 at most | 82 thousand cycles a call in action (740 calls, the slowest 704 thousand, a third of a millisecond) and 71.5 thousand in strategy (974 calls, the slowest 688 thousand); 3.6 and 4.2 M cycles a minute of the bot's play | breadth-first by cost over cell and heading, 324 states | another route of the same cost, proved below: lite, and not taken |
| `Board::ShowHint` (0x406d70) | after 500 updates without a move: draws two random cells until they hold a pair that can be joined, four random numbers a try; 138 tries a hint in the action game, 250 in strategy, 1,166 at most | 23 to 24 thousand cycles a hint (28 hints, the slowest 60 thousand); 0.2 M cycles a minute of a waiting board | list the pairs that can be joined and draw one | other random numbers from that hint on: lite, and not taken |
| the level generator: `CheckSolvable` (0x401e40) and `EnsureMoves` (0x401f10) with `TryRemoveClonePair` (0x4020e0), `CloneReachable` (0x402a70) and `SwapRandom` (0x4025c0) | once a level: places the blocks and crates again until the empty cells can be cleared in pairs, then clears a copy of the board pair by pair, swapping two random animals whenever no pair can be joined | 0.24 M cycles a level in action (0.11 ms, neither part over 0.3 ms in any level), 0.8 M in strategy (0.38 ms, neither part over 1.5 ms); 0.35 to 1.6 M cycles a minute | the same numbering of regions for `CloneReachable` | the same answers and so the same draws: not lite; once a level, not earned |
| puzzle mode: `HasMove`'s swap test, `MarkRuns` (0x402260), the puzzle hint | each a scan of the 81 cells: the four swaps of each cell, the runs of three | the swap test 3.7 thousand cycles a call, 9.5 thousand on a board with no swap; `MarkRuns` 129 thousand a call (753 calls), a figure that holds the sounds it starts and the score texts it makes (a sound's first play decodes it: the slowest call took 6.7 ms); its scan is two comparisons for each of the 81 cells | none: no search | |
| the Stampede: `HighlightGroup` and `FloodGroup` (0x4070d0, 0x407160) | when the pointer moves onto another cell: the group of equal neighbours under it | under a thousand cycles a call (8,990 calls) | none needed | |
| the rest of `Board::Update` (0x403d60) | the 81 cells each update, the water, the fish, the bubbles, the sparkles, the texts | the whole update 53 to 67 M cycles a minute with `-fast`, 4 to 5 microseconds an update, the searches included | none: no search | |

## The check for a move gives the same answer

`Reachable` floods from one animal's cell through the open cells (empty ones, and those whose animal is leaving, fading or walking)
and says yes when the other animal stands beside a cell it reached. So one animal reaches another exactly when the two stand side by
side or both stand beside the same connected region of open cells. Which regions there are does not depend on the pair that is
asked about, so one flood that numbers the regions, and one pass that notes for each kind the regions its animals stand beside, gives
`HasMove`'s answer without a search for each pair. One thing could make them differ: `Reachable` starts at the cell the piece itself
records (`mX`, `mY`), not at the cell the loop found it in. The scratch build checked that too.

It ran both methods at every call of the action and strategy games: 4,366 calls, the same answer in every one, and in every one each
standing animal's recorded cell was its cell. In 183 of them no pair was left and animals still stood on the board, up to 26 of them
(the two hard strategy games), which is the case where a shortcut would be likeliest to slip. A method that cannot differ gives the
same result, so it is not lite: it would belong to the port's performance pass. There it would save about 1.1 M cycles a minute of
14,500 M, a hundredth of a percent, so the port keeps the original's search.

The survey's figure for this check, about half a million steps when no move is left, is the product of 81 animals, 80 partners and
81 cells, and no board comes near it: an animal with no open cell beside it ends its search at once, and a board that holds many
animals has few open cells to flood. The slowest call measured on a board with no pair left took 24 microseconds.

## The route can differ

`SearchPath` tries the four directions in the order right, down, left, up, goes depth first, and keeps a route only when it is
cheaper than the best so far (a step costs 1 and each straight run 1 more). Among routes of the same cost it therefore keeps the
one its order meets first, and a breadth-first search meets them in another order. To see whether a board holds two such routes, the
scratch build ran the search a second time after each route with the four directions reversed. Of 1,774 routes, 43 came out as
another route of the same cost: 42 of the 740 in the action games, 1 of the 974 in strategy, none of the 60 in the waiting games.
In all 43 the two routes have the same number of runs, and so of steps.

The first of them is in the check's action game (`-seed=1 -autoplay=1 -autoplaymode=action`, easy), on level 1, the pair the bot
clicks at update 3406 (its clicks at 353,341 and 403,291): the two animals of kind `c` at column 3, row 6 and at column 4, row 5,
counted from 0 at the top left. The board then, a letter for an animal's kind and `X` for a block:

```
..c......
.........
......d..
..d......
.......Xb
....c....
.a.c.....
b..a....b
...cb....
```

The original's route goes right and then up, turning at column 4, row 6. The reversed order's goes up and then right, turning at
column 3, row 5. Both cost 4. What a player would see differ is the trail and the cell where the two animals meet, since each walks
half the route: the sparkles are drawn there and the pair's flight to the ark starts there. The flight covers 5.3 pixels an update,
so from another cell it can land some updates earlier or later; that was read from the code and not followed in a run, since the
change is not taken.

What it would gain is at most the whole of the search: 3.6 and 4.2 M cycles a minute of the bot's play, three hundredths of a
percent of the process. The bot joins a pair every 1.0 to 1.4 seconds. A player joins fewer, and asks for more routes with the
pointer; at ten times the bot's routes the search would still be 0.3% of the process, below the 5.6% by which two runs of the same
build differ.

## The hint and the level generator

A hint that listed the pairs and drew one of them would use one random number where the original uses four a try, so every random
number after the first hint would be another: it can differ on any board that shows a hint, and it is lite by the same rule as a
changed draw order. It would save about 20 thousand cycles every 500 updates of a board that waits.

The generator's searches run once a level and feed its random draws (`SwapRandom` draws only when no pair can be joined), so a
faster `CloneReachable` would have to give the same answers, and by the argument above it does. That makes it a performance change,
for a third of a millisecond a level.

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. The four
searches together cost under 8 M cycles a minute, about 0.05% of the process's 14,500 M, and the game's whole update is under 2% of
it; the rest is the picture, which a lite change may not touch. Two real-time runs of the same build differ by 5.6%, so no change to
any of the searches could be told from none. The two that give the same answer (the check for a move, the generator) are not lite
and would not earn a place in the performance pass; the two that can differ (the route, the hint's draws) would change what a
player sees or every later random number, for a gain that cannot be measured.

Nothing in the port changed for this step, so its frozen baseline stands as it was. The lite-mod step is therefore done with no
mod.
