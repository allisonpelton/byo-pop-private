# Seven Seas Deluxe: the lite mod (2026-11-14)

Seven Seas has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was
taken (`docs/MOD_ROADMAP.md`, "The lite mod"); the like-for-like processor time is in `GAME_NOTES.md`, section 11b (the port's
6.2 G cycles a minute at the original's window size against the original's 9.6 G).

## What was looked at

The game's logic is an 11 x 11 board, the player's ship, up to a dozen enemy ships, ports and shots, moved one turn at a time. Its
one search is the distance map `Board::Flood` (0x405660), a recursive fill that gives each cell its step count from the player's
ship (eight directions, walls marked 100); `Board::PlanEnemyMoves` (0x404bb0) rebuilds it for each chasing ship at the start of an
enemy turn and `CanFireAt` tests a line of cells. Everything else is a few comparisons a step by fixed rules. The
rest of a player's time goes to the picture and its present (section 11b).

Temporary timers, not committed, wrapped `Board::Update` and `Board::PlanEnemyMoves` in a build of the port. The game was the check's
bot game of two beginner games from level 1 (`tools/check.sh sevenseas`, seed 1, 5,566 updates under `-fast`).

| Computation | Cost in the games | A faster method | Result |
|---|---|---|---|
| `Board::PlanEnemyMoves` with `Flood`, once for each enemy turn | 47 calls, under 0.1 ms in all | a breadth-first fill | the same map: the fill ends with every cell at its shortest step count whatever its order, so it gives the same answer. Not lite; a performance-pass change, and not earned |
| `Board::Update`, every update, all game logic included | 5,566 calls, 3.6 ms in all, 0.65 us a call | none | |

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. The search runs
47 times in two games of 5,566 updates, for under 0.1 ms: at a player's 100 updates a second a minute of play costs a few
microseconds of it against 6,200 M cycles a minute, and the five-pair medians resolve a few percent of that. The only change a
search offers gives the same result, so it is not lite either. The lite-mod step is therefore done with no mod.
