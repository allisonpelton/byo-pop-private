# Ning Po's lite mod (2026-11-15)

**Nothing is taken: Ning Po has no lite mod.** The step is done with no mod (`docs/MOD_ROADMAP.md`, "The lite mod").

## What was looked at

The game's logic is small: a board of 144 tiles, and every search is a scan of them (`games/ningpo/src/Board.cpp`).

- **`Board::tileAtCell` (0x4062f0)** finds the tile at a cell by walking all 144 tiles. `isFree` (0x406350 and on) and
  `isVisible` call it up to 15 times a tile, and `Board::draw` asks `isFree` of each tile a frame, `countMoves` and
  `findMove` of each move or hint. A table from cell to tile would give the same answer for every board a layout can
  produce (the layouts never put two active tiles in one cell), so it cannot differ in any state the game reaches: by the
  lite mod's own rule that is not a lite change. It would go into the port instead, and it is not taken there either: the
  cost is some hundred thousand integer compares a frame (an estimate from the loops, not a measurement), against the
  300,000 pixels the screen conversion handles each frame, and the 2026-11-11 performance pass found the port's
  processor time well under the original's (`GAME_NOTES.md`, "Performance"). It would need a table kept in step with
  every move, undo, shuffle and new game, for a gain nobody would see.
- **`Board::shuffle` (0x406aa0)** swaps random pairs of tiles and tries again until a move exists. A different shuffle
  (a Fisher-Yates pass, say) would draw other random numbers in another order and so deal other boards, which makes it
  a lite-type change; but a shuffle is a button the player presses a few times a game, runs in microseconds, and a
  faster one cuts no measurable processor time.
- **`findMove` and `countMoves`** are the hint and the move count: one pass over the free tiles in index order. No
  search stops early, no tie is broken by chance and nothing runs against a clock, so no faster method gives another
  answer.
- The deal is the original's `newGame`: layouts and tile types are dealt from the layout files, with no solver (a
  board may have no move, and the game's own End Game dialog says so).

## Why nothing is taken

A lite change has to cut measurable processor time and be able to change what the game does. The one change that
could cut time (the cell table) cannot change what the game does; the one that could change what it does (the
shuffle) cuts no time. Candidates remain only if a later measurement shows a cost worth cutting; `GAME_NOTES.md`
"Lite candidates" lists none.
