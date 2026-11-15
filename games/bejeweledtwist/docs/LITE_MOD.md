# Bejeweled Twist: the lite mod (2026-11-15)

Bejeweled Twist has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what
the logic costs, and why nothing is taken (`docs/MOD_ROADMAP.md`, "The lite mod"). The game's whole update, every search
in it included, is between 3.2 and 3.7 % of the processor time of a minute of play, in 2D and in 3D; the rest is the
picture and the sound. No change to the logic could cut a minute's cost by more than that, and runs of the same build
already differ by more.

## What was looked at

The board is 8 by 8 gems, and a move turns the four gems around one of the 49 inner corners. The game's logic is
scans of that board, the falling gems and the effects:

| Function | What it does, and when | A faster method, and whether it could give another result |
|---|---|---|
| `Board::FindMove` (0x6c14fc) | after each turn settles (`SettleTurn`, the move kept at +0x201c), for a hint, for each `-autoplay` move, and once or twice in each fill's pass (`FillPieces`, to deal again when the board has no move): it puts the cursor on each corner in turn, rows from the top and columns from the left, and evaluates the turn there (`MoveCursor`, `EvaluateCursor`), so 49 turns at most; the first turn that matches wins, unless a later one puts a bomb in its line, which wins at once | stopping at the first match when the board holds no bomb gives the same answer, so it would be a performance change, not lite. Stopping at the first match even with a bomb on the board would be lite: on a board with a bomb, where a turn earlier in the scan matches without it, the hint and the autoplay's move would go elsewhere. Neither is earned (below): a call is 49 small evaluations, a few times a turn |
| `Board::MatchAt` and the fill's passes in `FillPieces` | when gems fall in: a new gem that would match at once makes the pass run again, and a board left with no move has its new gems recoloured from the board's random generator, up to the pass limit | none: the colours are the game's random draws, and any other order changes the deal |
| `Board::Update` and the gems' motion | every update: each falling, turning or bursting gem, the bombs' and the doom gems' counters, the tube | none: no search, one step for each moving piece |
| The effects (`EffectsManager::Update`) and the visualizers (`VizManager::Update`) | every update: particles, shards, the 3D backgrounds | none that is lite: they draw random numbers in their own order, which a faster method would change, and their cost is part of the update below |

Nothing in the original tries moves ahead beyond FindMove's one turn, and there is no path finding and no solver.
The roadmap's read of every port (2026-11-12) found the same: "FindMove simulates each twist, bounded".

## What it costs

The port's seeded Classic autoplay (`-autoplay=classic -seed=1234`, a fresh user folder at the 768 art, windowed) ran
a minute of play, updates 3000 to 9000, in real time with the framework's cycle split (`-cyclesplit`,
`tools/cyclesplit.py --from a --to b`, docs/TEST_COMMANDS.md "The cycle split"). Each run was under rundesk, pinned to
a core of its own (`tools/pincore.py`), with other sessions' games running. Two runs in 2D and two in 3D:

| | 2D, run 1 | 2D, run 2 | 3D, run 1 | 3D, run 2 |
|---|---|---|---|---|
| The game's update (logic, script, widgets, effects, visualizer, music update) | 0.313 M cycles an update, 3.6 % | 0.277 M, 3.2 % | 0.279 M, 3.7 % | 0.284 M, 3.6 % |
| The picture (drawing, present, the driver's threads) | 8.187 M, 94.0 % | 8.103 M, 94.4 % | 7.175 M, 94.0 % | 7.422 M, 94.1 % |
| The sound thread | 0.157 M, 1.8 % | 0.153 M, 1.8 % | 0.128 M, 1.7 % | 0.130 M, 1.7 % |
| The loop (events, sleeps, the readings) | 0.050 M, 0.6 % | 0.048 M, 0.6 % | 0.047 M, 0.6 % | 0.047 M, 0.6 % |
| The whole process | 8.707 M cycles an update | 8.582 M | 7.629 M | 7.884 M |

So the update costs about 1.7 to 1.9 G cycles a minute of the 46 to 52 G the process spends, and two runs of one build
differ by up to 3 % in the total, more than a lite change to FindMove could save. The update holds the widgets, the
effects and the visualizers as well as the board's logic. In an earlier profile of 30,000 updates of the same game
(NOTES.md, "The performance pass") no function of the board's own logic was above 0.2 % of the process. The processor
time goes to the picture, which is drawn in software in 2D as it is in the original.

## Memory and processor time on the merged trunk (2026-11-15)

The framework's memory and drawing changes since the last figures (the batching, the 2D flags and the wait for the
compositor) made every game's earlier measurements stale, so the like-for-like measurement of NOTES.md ("Processor
time like for like") was run again on the trunk as merged into this branch (747d7f85). `pairs.sh` ran the original
and the port in turn over the same seeded Classic autoplay, pinned to one core. Ten pairs ran in 2D and five in 3D,
with five to twelve other sessions' game processes beside every run. `cyclemeter.py` now also reads each process's
private bytes. The table gives the cycles of the minute from 30 to 90 s of each process's life, and the private
bytes at 30 s (on the first board on both sides) and at their peak:

| | Original | Port |
|---|---|---|
| 2D, G cycles in the minute (median) | 41.5 (32.2 to 51.7) | 38.8 (33.0 to 47.5) |
| 2D, private MB at 30 s / peak | 179 to 180 / 183 to 189 | 206 to 214 / 354 to 390 |
| 3D, G cycles in the minute (median) | 47.2 (37.1 to 48.8) | 43.2 (37.7 to 44.1) |
| 3D, private MB at 30 s / peak | 300 to 303 / 332 to 340 | 231 to 234 / 250 to 258 |

The port stays under its original in both modes: 6.5 % less in 2D, where it is lower in six of the eight clean pairs,
and 8 % less in 3D. Two 2D runs were left out of the medians and ranges. In one, the original took 15.7 G cycles in
the minute, with 159 MB at 30 s. In the other, the port took 24.0 G cycles, with 92 MB at 30 s, and lived 177 s
against 105. Each process reached its game late, so its minute was not inside the game. Three 3D pairs of a first
batch were spoiled when the original under visrun ran at the same time (the game runs one copy at a time) and were
run again. In 3D the original's figures include dgVoodoo's translation to Direct3D 11, which a player with a
Direct3D 9 driver would not run.

The port's 2D memory is above the original's by about 30 MB on the first board, and by twice the original at the
level-up jumps' peaks. The causes are the framework's and are described in NOTES.md ("The performance pass"). Nothing
here would change them: a lite change computes the game's logic another way, and it holds no memory worth naming.
