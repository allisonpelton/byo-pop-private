# TipTop Deluxe: notes on the original

What the translation found in `TipTop.exe` (2003-02-01), for whoever reads or changes the port. Addresses are the
original's; the port's declarations carry them as `// 0x...` comments, so a grep finds both sides.

## How the program is built

The program is C-style code in C++: global objects, structs with methods and a few vtables, no class hierarchy to
speak of. The start-up (0x412b90 and around) reads the preferences, starts LOWOS and the GUI and builds the app;
then `App::Frame` (0x41b9fe) runs once a frame until the game sets its quit flag.

LOWOS (0x42d000-0x438000) is the engine: the screen (DirectDraw, a 640x480 mode of 32 bits for TipTop), the blitter,
input queues of 16 events for the keyboard, the typed characters and the mouse, the sound buffers and the clock. The
resource manager (0x438390-0x439d70) loads named entries of `TipTop.dat` on first use, converts them by type (PCX and
JPEG to images of the display's format, Ogg Vorbis to WAV images, text) and counts references; the game loads and frees
them by groups.

The GUI (`src/Gui.cpp`) is a list of handlers, each a procedure with a few slots of its own, a rectangle, a layer and
a pass.
Messages go to every handler: create, destroy, mouse down, move and up, key, update, draw, typed characters, and
the set and get value of sliders and text fields. Layers can be pushed as modal; then input goes only to the top
modal layer, and drawing runs in groups from the layers outside every modal one up to the top, six passes within each
group. The screens are built from the handlers of a few procedures: images, shaped buttons (their hit test is the
image's color key), plain buttons, labels, sliders, check boxes, text fields and the many animations of
`src/App.cpp`.

The commands (0x41f922) are what a button sends: its second create argument holds the command in the low bits, and
flag 0x80000000 makes Enter press it, 0x40000000 Escape (both: 0xc0000000).

## The application

One `App` object (0x488710, 0x198 bytes) holds the screen in play and its layer, the board, the fonts, the bar
images and the handlers that need to be found again. The screens are numbered: 1 splash, 2 main menu, 3 options,
4 instructions, 5 credits, 6 game, 7 level summary, 8 new game, 9 high scores, 10 game over. `App::SetScreen`
(0x41c8f5) frees the old screen's layer and builds the new one.

The game modes are set by commands 0x20-0x23, each with its own list of levels: 0x20 regular (mode 0), 0x21 speed
(mode 2), 0x22 timed (mode 1) and 0x23 strategy (mode 3). The configuration places the timed mode's button at
(1000, 1000), off the screen, so TipTop Deluxe shows three modes; the timed mode's rules (a meter that empties with
time, the level won when it reaches the goal, the game lost when it runs out) are all in the board. Its high score
rows on the main menu and the high scores screen are at y=1000 too. Drawn on a larger screen (the port's `-canvas`),
they are the names, scores and levels alone, since the panels and the mode's names are parts of the background
pictures, which have none for the timed mode; and its button's picture (`ButtonNewGameTimed.pcx`) is a 16x16
placeholder, two states of 16x8 like the other buttons' stacked pictures, where theirs are about 130 pixels wide.

The game's random numbers are MSVC's `rand` (0x44ccaf), seeded once with the tick count (0x411fff); the port seeds it
with `-seed` when given, which makes the bot's games repeatable.

## The board

`src/Board.h` describes the four structures with their original layouts: the piece (0x5c bytes), the field (0x96c
bytes, the grid and the rules), the state (0x12b4 bytes, the level in play and the board's own copy of the
configuration, into which each level's values are written) and the board (0x1658 bytes, the previews, the layout,
the level changes, the bonus rounds and the save). The grid is indexed by row * columns + column. Row 0 is where the
pieces settle, drawn at the top of the playfield; the player's block enters at the bottom and rises into the column
it is sent to. Kinds 0-9 are the fruit; 10-16 are the special blocks (rocket, exploder, hammer, magnet, rainbow,
color bomb, column nuke) and 0x11 is none.

Outside the timed mode a level is won when no block is left and lost when a column is full; the configuration's
winning precedence decides which of the two is checked first when both happen. The modes differ in their lists of
levels (each level's record sets the level's values in the board's configuration) and in the ramps that make the
special blocks and the random floaters more frequent once the list has run out. The bonus round has its own timer,
layout and queue of blocks, which the save file keeps.

## The save file and the preferences

`savegame.dat` (0x41efbb, read by 0x41f499 and by the new game screen's summary) is a sequence of tagged fields: the
date, the mode, the level, the best chain, the score, the bonus round's time, score, queue and layout, and then the
board. Continuing a game deletes the file. The preferences (volumes, full screen, tips) are a file of their own
written at exit. Both went to the working directory; the port puts them in the user dir.

## Data records worth knowing

An effect (0x24 bytes in the configuration's effect lists) is a particle kind, a delay, a count, a speed, the levels
it is shown at (max at +0x10, min at +0x11, bytes), a pulse period (+0x14) and a fade-in, hold and fade-out
(+0x18, +0x1c, +0x20). A particle kind gives a size and, at +0x1c, whether its image has an alpha byte per pixel.

The blitter (0x437e3c) picks one of 32 routines: (format class << 3) | (mode << 1) | translucent, with mode 0 plain, 1
an alpha byte before every pixel, 2 a color key (0xfefe01 in the display's format) and 3 additive. Two tables made at
start drive the blending: 0x47ebc0[(difference << 8) | alpha] = difference * (255 - alpha) / 255 and
0x47ebc4[b * 256 + c] = b * c / 255.
