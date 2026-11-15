# Pixelus Deluxe: the lite mod (2026-11-15)

Pixelus has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing is
taken (`docs/MOD_ROADMAP.md`, "The lite mod"). The survey of the ports named no candidate for Pixelus, and a second read of
the whole of `Grid.cpp` and the board's hint code found none either.

## What was looked at

The game's logic is a grid of at most 32 by 32 cells (`Grid::MAX_SIZE`; the puzzles of the shipped levels are far smaller) in which
the player throws tiles from the edges, and each throw is a walk along one row or column. There is no search anywhere in it:

| Computation | What it does, and when | Work | A faster method | Result |
|---|---|---|---|---|
| `Grid::ComputeLanding` (0x423ee0) | after every throw, pull or cursor move: walks from the edge cell along the line until it meets a tile | at most 32 steps | none needed | |
| `Grid::Recount` (0x422320) | after every throw and pull: counts the wanted, empty and stray cells | one pass of the cells, 1,024 at most | counting as tiles move gives the same numbers, so it would be a change of the port, not lite | not taken: under 10 thousand cycles a call, called a few times a second at most |
| `Grid::FindStray` / `FindStrayFrom` (0x4221f0, 0x422280) | the lessons: the first tile that is not wanted and has a free line to an edge (`HasOpenSide`, four scans of a row or column) | at most 1,024 cells times 128 reads, and only while a lesson waits for the player to take a tile back | the same answer by a prefix count per row and column | not taken: a lesson's few puzzles (the grids are 5 to 10 cells wide), a call every few updates, and it gives the same answer, so it is not lite either |
| `Board::FindHint` (the next move of the best known solution; `ShowHint` is 0x433050) | the Tile Master's hint: compares the moves played with the best known solution's, six characters a move, and gives the next one | a string comparison of the moves so far | none: it reads a stored solution, nothing is searched | |
| `Grid::UpdateFinish` (0x424b30), the glow and wave of a finished puzzle | one pass of the cells each update while the finish plays | one pass of the cells | none | |

Every one of these is a scan or a walk over a small array, and the faster forms of the two that cost anything (`Recount`,
`HasOpenSide`) give the same answer in every state, which is a speed-up for the performance pass, not a change that may differ.
No state of the game can reach a different result through them, so the proof that a lite change needs (a state where the two
differ) cannot be given, and nothing is taken.

## Why it does not matter

The performance pass (`GAME_NOTES.md`, section 10) measured the port at about half the original's processor time over a minute
of play, with the picture (the grid's tiles and glow, the tablets, the island's gulls and light) as nearly all of it. The
solved-puzzle search that other games have does not exist here: the levels come with their solutions, so no solver runs and the
hint is a lookup. The whole of the logic above costs far less than the noise between two runs of the same build (5 to 6 percent).

The port makes no `LiteOn` call and the frozen baseline is unchanged by this file.
