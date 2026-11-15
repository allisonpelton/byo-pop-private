# Bonnie's Bookstore Deluxe: the lite mod (2026-11-15)

Bonnie's has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what it costs and
why nothing is taken (`docs/MOD_ROADMAP.md`, "The lite mod"). The like-for-like processor time is in `GAME_NOTES.md`,
"Performance", "Processor time like for like".

## What was looked at

The game's logic is a board of hexagonal cells that the player chains into words, a dictionary of 121,664 words held as
sorted tables with a first-letter index, and the hint. The only searches in it are two.

- **`Game::findHint` (0x4093f0).** When the hint is asked for, it makes up to 2,500 random walks of 3 to 10 tiles across the
  board, looks each prefix up in the dictionary and keeps the best-scoring word, stopping early after 120 ms of the game's
  clock. A faster method (a search over the board with the dictionary's prefixes, the best word by score) would show another
  word from the same seed, so it would be lite in kind: the walks draw random numbers, and a different search draws others.
- **`Dictionary::isWord` (0x404880).** A word without wildcards is a lookup in the sorted table. A word with `?` or `!` scans the
  table's bucket for its first letter (or all of it when the first letter is a wildcard), comparing lengths and letters. A
  prefix index would give the same answer, so it is a performance change, not a lite one.

Everything else in an update and a frame is the picture's: the tiles' motion, the popups, the backgrounds and the drawing of
the screen into the 800x600 buffer.

## What they cost

A scratch build (not committed) read the processor's cycle counter around each call of `findHint` while the port's tour
ran under `-fast` (`tools/tour_test.txt`, seed 1, run twice). The hint was asked for once in each run and cost 2.2 and
2.6 million cycles: the 2,500 walks, with the dictionary lookups inside them and the wildcard scans among those, take about a
millisecond, so the 120 ms limit is never reached. A minute of play costs the process 6 to 10 billion cycles
(`GAME_NOTES.md`), so a hint is 0.03 % of a minute, and a player asks for one every so often, not every minute.

## Why none is taken

A lite change goes in only when it measurably cuts the processor time of a stretch of play where it runs. The hint's cost is
under a thousandth of the stretch, below what the paired rounds resolve (a few percent), and a change would show other words
for a hint. The wildcard scan is the same answer by a faster method, so it belongs in the port, and it is no larger than
the hint it sits in. The game's processor time is the drawing, the framework's present and the mixer, which a lite change
may not touch since it keeps the look. The lite-mod step is done with no mod.
