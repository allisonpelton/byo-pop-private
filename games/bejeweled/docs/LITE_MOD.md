# Bejeweled Deluxe: the lite mod (2026-11-14)

Bejeweled has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what each
computation costs, where a faster one could differ, and why nothing was taken (`docs/MOD_ROADMAP.md`, "The lite mod").
The like-for-like processor time is in `GAME_NOTES.md`, section 19, "The performance pass": the port's 4.1% of one core
in a minute of play against the original's 4.5%, with no function of the game's own code above 0.3% of a run.

## What was looked at

The game's logic is a board of 8 x 8 gems, and it has three computations that scan the board: the search for a move, the
test for a row of three, and the fill of a new board. Everything else an update does is a few comparisons a gem
(falling, the swap, the clearing, the timer bar). The performance pass listed no "Lite candidates", and the survey in
`docs/MOD_ROADMAP.md` found nothing beyond small board scans, so the three scans were timed where they run.

Temporary timers, never committed, wrapped `Board::Update`, `Board::FindMove`, `Board::HasSet` and `Board::NewBoard` in a
build of the port. The games were the port's own bot's (`-autoplay=7 -seed=7 -fast`, one Normal game and one Timed game
with `-autoplaytimed`, each to its end). Other sessions' games ran on the machine meanwhile and the timers' own cost is
in the figures, so they are rough, and high if anything.

| Computation | What it does | Normal game (11,862 board updates) | Timed game (84,425 board updates) | A faster method | Result |
|---|---|---|---|---|---|
| `Board::Update` (the whole board, the three scans in it included) | one update of the board | 12.7 ms in all, 1.07 us an update | 81.0 ms in all, 0.96 us an update | | |
| `Board::FindMove` (0x4c9dda), as the game calls it: once each time the board settles (is a move left?), and for a hint | tries each gem's four swaps in turn and measures the row and column through the gem swapped in; stops at the first move | 68 calls, 0.06 ms, 0.9 us a call, the slowest 4.0 us | 441 calls, 0.49 ms, 1.1 us a call, the slowest 3.9 us | match the shapes of a move around each pair of gems without swapping, or search in another order | the same yes or no; another order could name another move for the hint |
| `Board::HasSet` (0x4c9cf5), in `NewBoard`'s loop | scans every row and column for three of a colour | 1,387 calls, 0.38 ms, 0.27 us a call | 2,441 calls, 0.53 ms, 0.22 us a call | test only the rows through the gem just coloured | the same answer |
| `Board::NewBoard` (0x4cb182), `HasSet` included | fills the board from the bottom row up, rolling each gem's colour again until the board has no row of three | 21 calls, 0.63 ms, 30 us a call, the slowest 55 us | 37 calls, 0.85 ms, 23 us a call, the slowest 40 us | as `HasSet` | the same board, from the same draws |
| `Board::FindSets` (0x4cd287), when the gems land and after a swap | marks the rows of three | inside `Board::Update`'s figure | inside `Board::Update`'s figure | none needed | |

`FindMove`'s calls from outside the board's update (696 and 4,025) are not in the table: nearly all are the bot's, which is
test code, and the rest look for the tutorial's move on a new game's first board.

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. At a
player's pace the board has 100 updates a second, so the Timed game's 84,425 updates are 14 minutes of play, in which the
whole board's logic took 81 ms: about a hundredth of one percent of a core, and a four-hundredth of the 4.1% the port
uses. The three scans together are under 2 ms of those 14 minutes. The slowest single call of any of them, 55 us, is a 180th of
one update's 10 ms. The pairs of the performance pass spread from 2.6 to 5.7% of a core for the same exe, so no change to
any of this could show in them. The port's time goes to presenting the frame and to the framework's drawing
(`GAME_NOTES.md`, "Where the port's time goes"), which a lite change may not touch, since it keeps the look and the frame
rate; the existing `lean` mod, which caps the frame rate, is for that reason not lite.

Two of the three faster methods are also not the kind the lite mod is for. `HasSet` and `NewBoard` tested only around
the new gem give the same answer in every state: `NewBoard` colours one gem at a time on a board that had no row of
three before it, so any row of three passes through that gem, and the same colours are drawn in the same order. A change
that cannot differ is not lite; it would belong to the performance pass, where it does not earn its place at under 2 ms in 14
minutes.

A move search in another order is the one change that can differ, and that it can needs no run: the hint shows the first
move `FindMove` meets, row by row from the top left, and a board with two moves is ordinary: the Timed board that
`tools/fixes_test.sh` saves at level 2 (seed 3) has the gem in column 2, row 2 moved down and the gem in column 4, row 6
moved down (columns and rows from 0). A search that meets the second first shows another hint. The yes or no after a
settle, which decides whether a game ends or Timed mode's board falls out, cannot differ. So the only effect would be a
hint on another gem, for a gain of under a microsecond a hint.

The lite-mod step is therefore done with no mod.
