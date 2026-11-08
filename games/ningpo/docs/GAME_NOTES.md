# Ning Po Mahjong Deluxe: notes on the original

What the translation found in `Ningpo.exe` (linked 2002-09-25), for whoever reads or changes the port. Addresses are
the original's; the port's declarations carry them as `// 0x...` comments, so a grep finds both sides.

## How the program is built

The program is C++ on Pyrogon's framework. `WinMain` (0x40c890) makes the one game object (the global at 0x475550,
0xb9c4 bytes, a subclass of PGApp) and calls `PGApp::run`, which starts the subsystems (the display, the input, the
sound, the cvars, the registry, the archive) and then runs `PGApp::mainLoop` until the game says to quit. The loop
takes the window's events from a queue, gives each first to the app (activation, painting, the controls) and then to
the game, and when the queue is empty redraws the dirty rectangles (the game's render, then the controls) and flips
them to the window. Every dialog is a modal loop of its own (`PGDialog::doModal`) that draws the game under it and
every open dialog over it. Time is the time of the event being handled; the game takes the time spent paused off
its clock.

The display is a 640x480 GDI back buffer of 15-bit pixels (5-5-5). Images become surfaces of the same pixels,
dithered down from 24 bits, with an 8-bit alpha plane when they have transparency, and every drawing call blends a
surface into the back buffer. The sound is a software mixer of 8 voices and one music stream (Ogg Vorbis decoded as
it plays) into a DirectSound buffer of 22050 Hz; a voice's pan is kept but never applied.

## The game

The game is one screen: the title, where arrows choose the layout (18), the tile set and the table (the background),
the board, and the two end screens (won, and no more moves). The controls run down the left side and a panel of
numbers (the time, the moves available, the tiles left) along the bottom. Everything else is a dialog: the options,
the best times, the name entry, the yes/no questions, the no-moves dialog and the rules.

A layout (a `.nml` file of the archive) gives 144 cells, each x and y on a grid of half tiles and z the level. A new
game puts the 144 tiles of the set into the cells at random (0x4066d0); nothing checks that the board can be
cleared. The set is the 34 suits and honours four times each, and the four flowers and four seasons, one of each,
which match any of their kind. The tiles are drawn in an order sorted once per game (0x4067f0, a bubble sort): the
lower levels first, then from the right to the left and from the top down.

A tile is free when nothing is on top of it and its left or right side is open (0x4064a0). The left button selects a
free tile (0x4045c0); a second free tile of the same kind takes the pair off, while one of another kind drops the
selection with the sound of a blocked tile, as a click on a blocked tile has. A click on the selected tile, or the
right button, drops the selection. The moves available are, for each kind with a free tile, half of its free tiles
rounded up (0x405ef0).

The clock counts the seconds played (the time paused not counted) plus penalties: 30 seconds for a hint asked for,
60 for an undo and 120 for a shuffle. A hint (the button, or `f` or `h`) shows a move, the next one at each
hint; with a tile selected it shows that tile's matches that can be seen, or the "no match" sign for 3 seconds when
there are none. The autohint (an option) gives one after 45 seconds without a match, without the penalty.
Show moves (the button, or Space) lights the free tiles over half a second, keeps them lit for 3.5 seconds (or while
Space is held) and lets them go back over half a second; how far it lights them depends on the tile set. `s`
shuffles, and `u` undoes. A shuffle swaps the tiles' faces at random until there is a move (0x406c00); the undo list
holds every pair of the game.

When no move is left, a dialog offers to shuffle (only when more than one tile is free), to undo, or to end the
game. A game ends won (no tile left) or ended in that dialog; its time goes into the layout's best times if it
ranks among the 8 (0x4081d0): fewer tiles left first, then the lower time with the penalties, then the lower time
played. The name entry suggests the account's name (none when it is longer than 23 characters) and takes up to 12
characters.

Typing Backspace, q, Backspace toggles a cheat (0x403e20): with it on, `m` plays the first move the board has and
`n` shows the "no match" sign, and the game cannot be paused.

## The registry

The game keeps everything under `HKEY_CURRENT_USER\Software\Pyrogon\NingPo` (the port's emulated registry,
`registry.ini` in the user folder). Its own values are binary: `Layout`, `Tileset`, `Background`, `Autohint`,
`Fullscreen`, `Sound Volume` and `Music Volume` (floats) and the best times, one value of 0x30 bytes per layout and
rank named `HS:<layout>x<rank>`, with the name at +4 (0x20 bytes), the time with the penalties at +0x24, the time
played at +0x28 and the tiles left at +0x2c. The other values in the file are the framework's.

## The trial

The trial's code is in the same exe, behind `checkRegistration` (0x40edb0): the "full version only" layouts and tile
sets refused at a new board (0x404c90), the count of games played against a maximum (the registry's `InstallFlags`),
the loading screen's progress bar stopped at a quarter, a buy button among the controls, a register button in the
options and a page asking to buy at exit. The loading screen also carries a thanks for registering
(`demo_menu_thanks_np.tga`), loaded and placed but never shown. The port plays as registered and leaves all of it out,
the thanks included (the user, 2026-11-08: a thanks for registering is part of the registration code). No text of the
game's tables is shown only to a registered copy.
