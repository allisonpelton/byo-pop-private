# Mahjong Escape: Ancient Japan: notes on the original

This file records what the translation found in `MahjongEscapeAJ.exe` (linked 2006-11-08), for whoever reads or changes
the port. Addresses are the original's, and the port's declarations carry them as `// 0x...` comments, so a grep finds
both sides. `ENGINE.md` maps JollyBear's engine under the game. `DEVIATIONS.md` lists where the port's code differs.

## The program

The game is SpinTop Games' and runs on JollyBear's engine: an app object (0x424a30, vtable 0x51df94) whose manager
keeps a stack of layers and sends them a frame every 25 ms. Each layer is a tree of objects. The game's layers are
the splash, the main menu (0x40f540, 0x40f5a0), the player list, the classic list of layouts, the map of the escape,
the play layer with its board, the ending, and the dialogs over them: the options, the tile sets, the backgrounds,
the help pages, the tips and the yes/no questions.

The game's resources are in two modules. `Resource.dll` holds the pictures, the sounds, the music, the 222 layouts
(`LAYOUTS` resources: 60 for the escape, 162 for the classic game) and the longer texts (`TXT`). The exe holds the
string tables of the menus and dialogs and two pictures named in its folder (`SplashLogo.bmp`, `MenuLogo.bmp`). The
port reads both modules with its own PE parser (`src/PeResources.cpp`) and never runs either. The game's texts are
read from them when the game runs; the few the port needs from the exe's code are named by address in
`src/GameTextTable.inc` (made by `tools/make_gametext.py`) and are never copied into the repository.

## Saves and settings

Each player has a file in the user folder: `list.usr` lists the players, and `<n>.usr` holds a player's settings,
best times and scores (one of each per classic layout and per escape stage), the escape's progress and the chosen
tile sets (0x415eb0; a CRC that also covers the exe's folder guards it). A game in progress is saved as a file of
its own per level. The app's settings (the volumes, full screen, wide screen, the hardware acceleration and the window's
place) are in the registry under `HKCU\Software\JollyBear\MahjongEscapeAncientJapan`, which the port emulates in
`registry.ini`.

## The rules

A tile is free when nothing lies on it and its left or right side is open (`Layout::IsFree`, 0x414c20). Two free
tiles of the same kind make a pair; the four seasons match each other and so do the four flowers. A pair scores its
kind's points, plus 500 when it is the same kind as the pair before. A hint costs 100 points. The undo takes the
pair's points back. A shuffle adds 20 seconds to the clock. "Show free tiles" shades the tiles that cannot be taken.
The autohint (an option) gives a hint after a while without a click.

**Emperor's Adventure** is 60 stages along a path through twelve periods of Japanese history, from the Jomon period to
the Taisho period. A stage is won by matching the two Magic Gold Tiles (kind 0x2b) once both are free (0x40d8b0). The
tiles still on the board then fly off: 200 points for each pair, 500 for each special tile. A level's end shows the
time, the score, the bests and the period's saying, and the map walks the marker on to the next stage. The first
stage's layout gives every tile its kind. The later stages give only some tiles their kinds and deal the rest, seeded
with the stage's own number from the stage table (TXT 0x17f), so a stage is the same every time it is played. Later
stages bring special tiles: a key (0x2c) that opens a lock (0x2d, which can be picked only with the key selected and
need not be free), and two tiles (0x2e, 0x2f) that pair with any ordinary tile that can be seen, free or not. A right
click on a special tile shows its tip. Each new thing gets a tip box the first time it appears in a level, until that
level has a best score.

**Classic** is the 162 layouts of the list, nine to a page, each dealt anew from the clock. A game is won when the
board is empty. The options' difficulty sets the share of the deal that goes in fours rather than pairs: 0 percent
for relaxed (a new player's), 25 for normal and 85 for expert.

Both modes have the same bar of buttons (undo, hint, shuffle, the menu and pause) and the same panel (the time, the
score, the bests, the moves available and the tiles left). The in-game menu goes back to the main menu, chooses a new
layout, opens the options, the tile sets, the backgrounds and the instructions, or resumes the game.

## The trial

The exe is the full game of the PopCap Game Collection. The translation found no trial code: no registration, no nag
and no buy screen.
