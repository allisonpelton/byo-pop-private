# Mummy Maze Deluxe: the lite mod (2026-11-14)

Mummy Maze has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was
taken (`docs/MOD_ROADMAP.md`, "The lite mod"); the like-for-like processor time is in `GAME_NOTES.md`, "Performance" (the port's
7.7 G cycles a minute against the original's 8.7 at the game's own window size).

## What was looked at

The game's logic is a board of at most 10 x 10 cells, an explorer, up to two mummies and a scorpion that each take a step or two a
turn by fixed rules, a gate and a key, and one search: the solver. The rest of a player's time goes to the picture and its present
(`GAME_NOTES.md`, "Performance": the game's own code is about 1% of a profile's samples, the rest the framework's present path and the
graphics driver's thread). The candidate the survey named is the solver, so it was timed where it runs.

Two temporary timers, not committed, wrapped `Board::CheckSolvable` and `Board::DoSolution` in a build of the port, and a third pair
wrapped `Board::Update` and `MMApp::UpdateFrames`. The game was the check's bot game of the whole classic game (`tools/check.sh
mummymaze`, "full": seed 21, 225 chambers, 1,006,551 updates under `-fast`, a draw every tenth update), twice, once for each pair.
The machine was shared with other sessions' game processes, so the two runs' wall times differ (29.0 and 25.9 s) and the figures are
rough.

| Computation | What it does | Cost in the whole game | A faster method | Result |
|---|---|---|---|---|
| `Board::CheckSolvable` (0x411640) with `DoSolutionHelper` (0x411a90), after every explorer move on mazes under 7 x 7 (the Ankh) | a depth-first search of the states (explorer, mummies, scorpion, gate) that the board's own rules reach, with a visited set and the path copied at every branch; stops at the first exit | 2,253 calls, 0.217 s in all, 96 us a call, the slowest 18.6 ms, one call over 10 ms | breadth-first, or the same search without the path copy | the same yes or no: the next state depends only on the state and the move, so any exhaustive search says the same. Not lite; a performance-pass change, and not earned |
| `Board::DoSolution` (0x4050b0) with `DoSolutionHelper` (0x405580), the Abandon Hope button | the same search, keeping the shortest way it meets and taking over the rest of the best way where a shorter one reaches one of its states | 225 calls (the bot's planning runs the search the button runs, 225 times in the game), 0.297 s, 1.3 ms a call, the slowest 17.2 ms, three over 10 ms | breadth-first, which finds the shortest way | the way it finds first, in the order west, north, wait, south, east, need not be the shortest, so another search could play another route. Not proved (see below) |
| the explorer's, mummies' and scorpion's steps (`Board::SetMummiesInMotion`, `UpdateMummies`, `UpdateScorp`, `UpdateExplorer`), the gate, the arrows | a few comparisons a step by fixed rules | `Board::Update` 0.70 s over the 1,006,102 updates it ran, 0.70 us each, the solver included | none: no search | |
| the widgets' updates, the music's tick, the title, the map | small fixed work | what is left of `MMApp::UpdateFrames`' 1.73 s | none | |

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. The solver is the
only search the game has, and in a whole classic game it took about 0.5 s of the 29: 1.7% of the run under `-fast`, which draws
every tenth update. At a player's pace the game has 100 updates a second, so those 1,006,551 updates are 168 minutes of play, and the
solver's share is about 3 ms of processor time a minute: 9 M cycles of the 7,700 M that a minute costs. The five-pair medians of the
measurement resolve a few percent of a minute at best (the port's own runs spread from 6.3 to 8.0 G cycles in the same window), so
no change to the solver could show in them. The slowest single call, 18.6 ms, is under two updates and falls on a move, not in a
stretch of play. Everything else in the game's logic is a few comparisons an update.

The two solver changes are also not the kind the lite mod is for. A search that finds the same yes or no (the Ankh's) gives the same
result, so it is not lite and belongs to the port's performance pass, where it would not earn its place at 0.1% of a minute. A
breadth-first search for Abandon Hope may play another route, so it would be lite, but it was not proved: no chamber was found where
the original's first-found way is longer than the shortest, since nothing hangs on it; and a lite change keeps the game's rules and
look, while Abandon Hope's route is what a player sees played.

The lite-mod step is therefore done with no mod.
