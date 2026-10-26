# Pixelus Deluxe: mods worth making

None is made: the order of work (CLAUDE.md) gives no further game mods until every PopCap game works in the browser. These
are the ideas, each with the place in the code it would touch. The game is turn based, so a rule change never touches the frame
loop.

**A puzzle maker.** The puzzles are plain text files (`levels/<name>.txt`: a title, a difficulty, rows of characters) with an
optional 16 by 16 picture (`images/levels/<name>`) that gives the solved puzzle its colours, and `levels/levels.txt` lists
the temples' puzzles in order. A maker needs a board where the player places tiles, walls, bouncy blocks and sticky spots,
saves the file, and records a solution with it (the hint and the autoplay bot throw the tiles of a puzzle's best known
solution, which `levels/maxedscores.txt` carries), so a puzzle can be proved solvable before it is offered. Grid.cpp loads boards up to
32 by 32 cells, the shipped puzzles are all 16 by 16, so a size choice is cheap.

**A random puzzle.** A generated puzzle (a random picture, tiles thrown in from the sides by a random walk, so a solution
exists by construction) written into a level file and offered from the main menu.

**A move and time board.** The tablet already keeps the player's shortest solution and best time per puzzle
(`PuzzleCompletedScreen`, `ProgressManager`); a statistics screen could list them for all 210 puzzles, with the medals (bronze
within 10 moves of the best known, silver within 5) and the temples' totals, and show the player's own best solution as a
replay on the board.

**Par moves.** A setting that ends a puzzle with a failure when the move count passes the best known solution's plus a
margin, for players who want the medals only.

**A colour-blind set.** The tiles are five variants of four colours (`IMAGE_TILEBLUE01` and the others): a mod that draws the
variant's number or a shape in the corner of each, or a high-contrast set for the aim marks and the cells that want a tile.

**Keyboard play.** The aim moves by the mouse; arrow keys could move the aim along a side and the space bar throw, which
touches `Board::KeyDown` and the aim code (`Board::Aim`).

**Expert mode as a setting.** A player who has switched the hints and the undo off (`Board::mExpert`) plays the puzzles the
hard way; a mod could record that on the tablet and give the medals only to expert solutions, or let every other player undo
all the way back through a puzzle's history (`Board::RecordUndo`, `Board::Undo`).

**Daily puzzle.** The same puzzle for everyone on a day: pick `levels.txt`'s entry by the date and show it on the main menu.

**Speed run clock.** The board counts the time of a puzzle in `Grid::mTime`; a clock for the whole game that stops while a
story or the tablet is on, with splits at each temple, for runners.
