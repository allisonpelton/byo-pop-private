# Pixelus Deluxe: notes on the original (Pixelus.exe, 2004)

Working notes of the port, kept so a new session need not read the decompilation again. Addresses are virtual
addresses in the carved game image (base 0x400000). The game's texts are never copied here: they are named by their
address (`GT(0x...)`) and read from the player's own exe.

## 1. Layers

- **SexyApp** (framework, 71 slots) -> **NcApp** (Nuclide's layer 0x4e2c10-0x4ea000, 87 slots: the resource tables,
  pause count, quit question, credits/options dialogs, the loading screen) -> **PixelusApp** (0x4326c0-0x436000,
  ctor 0x435880, vftable 0x5123a0). WinMain 0x432360. Slots 71-86 are NcApp's own virtuals the game fills in:
  71 ShowRegisterDialog (0x435580, shop only), 72 ShowCredits (0x432a00 -> 0x402290), 73 ShowOptions (0x432a10 ->
  0x4194d0), 74 MoreGamesLink (0x434710), 77/78 (0x432a20 -> 0x432a40: quit the main menu; the first run shows the
  story 0x432960, later runs the island map 0x432600), 79 PlayClicked (0x433b90: the loading screen's play link),
  80 (0x433910: leave the board for the temple 0x432420), 81 (0x432e00: reset the user's flags), 82 (0x4331a0: colour
  by index 0 light, 1 mid, else dark), 83 (0x4330d0: the "more games"-style dialog id 13), 84 HandlePaused (0x4333a0),
  85 SetHintsDisabled (0x4336c0).
- Screens each have one pointer in PixelusApp (+0x678 board, +0x67c help, +0x680 temple, +0x684 loading, +0x688 main
  menu, +0x68c island map, +0x690 puzzle completed, +0x694 more games, +0x698 hint overlay, +0x69c hint screen, +0x6a0
  story). One screen at a time is on the widget manager (the help screen sits over the board).
- All dialogs are `GameDialog` (0x41c260, ctor args include an optional decoration) made by `NewDialog` (0x4331e0):
  336 wide at (206,42), insets (48,60,48,32), header font Garamond20, lines font Garamond14, white text.

## 2. The puzzle ("PuzzPix")

The game is a sliding-tile puzzle on a grid of at most 32x32 cells (16x16 in the shipped puzzles). The player clicks
a bright target cell and a tile slides in from the edge of the grid along that row or column; it can only be placed
when there is a tile (or other stopper) behind the empty spot. Temporary tiles are used as stoppers and are removed
again. The workbench at the bottom shows how many tiles are left; with none left a message appears (the
"no more tiles on the bench" text). Obstacles: glue patches ("sticky": tiles stay for ever, no UNDO in expert mode),
bouncy blocks, sticky edges, red crosses. A **Tile Master** (0x42bd40) animates the throwing hero. A puzzle has a
title, a difficulty 0-7, a uuid, a data block of rows, and an optional picture images/levels/<name> that colours the
solved picture. Buttons on the board: RESET, HINT (shows the solution), UNDO, MENU, back to temple/cave.

### 2.1 Level files (levels/<name>.txt, loaded by 0x426910 `Grid::LoadFromFile`)

```
#uuid#<uuid>   #name#<file name without .txt>   #title#<title>   #difficulty#<n>   #data#
<rows of characters, one per cell>
```
Characters (cell byte +0x10 is "a tile is wanted here", +0x11 "a tile is there", +0x14 the object, +0x19 start flag):
`0` empty; `?` wanted (the tile count +1); `S` wanted and the start cell; `+` wanted, with a tile object; `-` unwanted
with a tile object (count -1); `X` a fixed object (+0x11 = 1); `B` a fixed object too and flags the cell two cells
on (+0x18). `levels/levels.txt` is `#level#<god>` followed by `#puzzle#<file>.txt` lines (the temples and their
puzzles, in order); `levels/improvedmax.txt` and `levels/maxedscores.txt` hold the best scores.

### 2.2 Grid object layout (OldGrid/Grid, vftable 0x50f0f4, ctor 0x425d40)

Cells start at +0x40, 32 per row, 0x2c bytes each (rows x cols at +0x30/+0x34). Cell: +0 int, +4 int, +8 bool,
+9 bool, +0xc int, +0x10 wanted, +0x11 filled, +0x14 object*, +0x18 bool, +0x19 start, +0x1c Color. After the cells
(+0xb040): the object list (ptr, capacity, count), +0xb050 tiles still needed, +0xb054/58 last move,
+0xb05c bool done, +0xb088-94 the cursor, +0xb0ac move counter, +0xb0b0 title, +0xb0cc name, +0xb0e8 difficulty,
+0xb0ec uuid, +0xb114 move history, +0xb144 hint (solution) as a list of 6-character moves "%02d%02d.." (row, col, ...).

The port is `src/Grid.*` (the same layout, with the cells as a struct and the history as a vector); `tests/gridtest.cpp` replays the
best known solutions of the game through it.

## 3. Files in the game folder

`images/`, `ncframework/images/` (loading graphic and bar), `data/*.txt` (ImageFont descriptions) and `data/story/*`
(story pages), `levels/`, `sounds/`, `music/` (songs, `pixelus in-game playlist.txt` with 40 numbered tracks),
`properties/partner.xml`, `pixelus.exe` (the game's texts are read from it).

## 4. The screen flow

Loading screen (resources load on a thread; when it is done the play link calls `PlayClicked`, 0x433b90) -> main menu (0x40b1c0:
play, options, more games, quit, "who are you" link; the first run has no player and asks for a name) -> `PlayGame`
(0x432a40): a player's first play shows the story (mode 0, the introduction), later plays go straight to the island map
(0x432600) -> `EnterTemple` (0x4170d0, a click on a temple) -> the temple screen (0x432420, over a snapshot of the island): the
god's puzzles with their pictures and medals, a slider for the pages, a button for each puzzle -> `StartPuzzle` (0x433820;
asks "carry on or start again" first when a puzzle has saved moves) -> the board -> the puzzle-completed tablet ->
`ContinueAfterPuzzle`: the story (mode 1) when a temple has opened, the ending (mode 3) when the last puzzle of the last
temple is done, the island when a temple was finished, otherwise the temple screen again. The first temple (the tutorial) has
nine puzzles and a skip question (2027 YES -> `SkipTutorialOK`, 0x4101d0: the player's flag 0x79 is set and the island comes).
One screen is on the widget manager at a time; the board, the tablet, the story and the island each remove the one before
and let `SafeDeleteWidget` free it later.

Dialog ids: 2 options, 3 credits, 4 quit question, 0x12 who are you, 0x13 new player, 0x14 delete question, 0x15 rename,
0x17 name taken, 0x19 "not yet" and the mystery-puzzle notice, 0x1b skip tutorial, 0x20
saved puzzle, 24 reset. The buttons of a dialog report to `PixelusApp::ButtonDepress` as 2000 + id (yes, OK or the footer) and
3000 + id (no or cancel); the base class kills the dialog of an id it does not know.

## 5. Results (0x41b3c0, the constructor of the puzzle-completed tablet)

The constructor does the bookkeeping before the tablet draws anything. The player's previous record for the puzzle (moves and
time) is looked up first. `ProgressManager::UpdateBest` (the table of best known solutions, `levels/maxedscores.txt` plus
`improvedmax.txt` for the ones a player improves) says whether the new solution equals or beats the best known: that is the
"world record" flag. The player's own flag "shortest" is a new personal best in moves; "bronze" and "silver" are within 10 and
5 moves of the best known; a puzzle the player had already solved as well as the best known counts its time instead (the
"maxed" flag). The tutorial's last puzzle sets the tutorial-done flag (the player's flag 0x79). The result is entered with
`SetRecord(uuid, matched, moves, time)`; a puzzle that finishes a temple can open the next one (`mOpenedTemple`, which the
story then announces), and the last puzzle of the game sets `mGameDone`. The time is `Grid::mTime`: milliseconds, 10 added
every update while the puzzle is not done or a tile was still in flight at the last update, so the last tile's flight counts.
The tutorial-finished text and the player's flag 0x79 belong to the solve that completes the tutorial, not to every later one.

The finish of a puzzle (0x422590, 0x424b30, 0x421e10, 0x409bd0). The throw that fills the last wanted cell calls `Complete`:
the puzzle is done at once, sample 24 (the completed sting) plays and all music fades out. Then, once no tile is in flight,
a delay of 84 updates runs (+0xb108 from 1.0 down to -1.5 by 0.03), then the picture is swept in over the board, three cells
an update in reading order (+0xb100 from rows * cols down to 0), and a shimmer (+0x10, -25 to 60 by 0.6 an update) goes across
the tiles from the top left. When the sweep is over (+0xb110) the Board counts 60 updates (the Tile Master throws once at the
start) and shows the tablet: at 100 updates a second that is about 2.3 seconds from the moment the last tile comes to rest on
a 16 by 16 board (84 updates of delay, 86 of sweep, 60 of countdown; a bigger board sweeps longer). The sweep draws the
picture's colour on every cell it has passed, wanted or not, in place of the cell's back and sticky spot, and the tiles on
the swept cells (blocks too) are not drawn. The
shimmer lights a tile by 80/255 and a cell's back, and the playfield behind it, by 30/255 of its strength. The back of a
wanted cell and its sticky spot are only drawn while no tile rests on the cell. A tile that flies out of the grid (a
throw into an empty line, move F, or its undo) is never freed in the original: 0x42f740 marks it dead in state 2 and leaves
it there, its trail held at 200 and the grid's "a tile is moving" flag (+0xb0a8, 0x42ee70 is only +0x44 != 0) set for good,
so a puzzle finished after such a throw would never show its picture or the tablet. Nobody has seen this in a run; the
port skips the tiles that are gone and does not stall. A tile that lands pushes up to four tiles in line behind it
(0x423d30: the nearest by the most and the soonest, one pixel an update out, a wait, back); a tile that bounces off a bouncy
block does the same from the cell it stopped in. The undo plays the pull sound, throws a bounced tile again (kind B) or
flies a tile that left the grid back in (kind F); a bounce is B in the move history and a tile that flies out is F.

The tablet itself: the picture of the solved puzzle (20 pixels a cell, the picture's colours on the wanted cells) flies from
the board's grid origin to the tablet over 80 updates (smoothstep, 320 to 117 pixels) and the medal grows after it. The two signs at the sides tell what is new. The continue button's label is one of the game's own
texts and renders with the game's decorative font.

## 6. Story screens (0x432960)

`data/story/<names>.txt` are scripts of speeches (speaker, lines, vertical place, start tick) per pair of temples: the mode 0
introduction, mode 1 "a temple has opened" (the temple just left and the one that opened), mode 2 the temple the player is in
again, mode 3 the ending. A script that is not there is no conversation (`StoryScreen::HasSpeeches` is false and the screen is
skipped). The portraits are the gods' heads (`PixelusApp::mHeads`, 11 gods with 4 expressions each, built from the head
sheets); the background music is the story loop and the continue button ends the story (`EndStory`: to the main menu when it
came from there, otherwise to the island).

## 7. Options, credits, help and the pause

The options dialog (0x419330) is a `GameDialog` 420 wide, 396 high, at (110, 240 - h/2): two sliders (music, sound effects), the
boxes Full screen, Custom Cursors and Disable Hints and Tooltips (the hint switch is a registry boolean, `NcApp::SetHintsDisabled`),
the buttons Reset Hints (clears the player's tutorial-hint flags and greys itself), Credits and How to Play, and, only when it
was opened from a puzzle, Main Menu: that one saves the puzzle where it is, removes the board and shows the menu (0x4191e0 with
0x433e70). The footer button is the dialog's: 2002 kills it and, with a board open, ends the pause. Opening it from a puzzle
(`ShowGameMenu`) is `Pause(true)`; `NcApp::Pause` nests, and `HandlePaused` (0x4333a0) pauses all music, except that with the
options open over a puzzle the in-game music plays so the sliders can be tried against it. The board starts the in-game music
150 updates after the puzzle appeared (`mMusicDelay`).

The credits (0x402290 over the Nc layer's credits dialog 0x4e5580, id 3, 396 high) are centred lines at fixed offsets from the
top: who developed the game, and the thanks (all texts are the game's own, by address in `CreditsDialog.cpp`).

The how-to-play screen (0x40e1a0, shown by 0x4328c0, closed by 0x432c20) is a picture with two windows over a full-screen
frame (`helpfront`, `helpback`): the left plays two demonstrations of throwing a tile onto a board and pulling one off, the right
completes a picture (the puzzle `levels/helpscreen.txt`, a ring of walls with eight holes), with fades between the runs; a
check box "Don't show this screen again" keeps the opposite of its state in the player's flag 0x2d (`mFlag2D`), and a player
whose flag is set sees the screen before every puzzle outside the tutorial. The port draws the demonstration its own way (the
tiles of the picture slide in one after the other).

## 8. Tests and tools

`tools/decread.py <hexaddr>...` prints a function from the Ghidra export with image and font ids named. `tools/make_gametext.py`
writes the table of text addresses, `tools/make_res.py` the resource tables, `tools/make_symbols.py` the symbol file for Ghidra
from the address comments in `src/`. The tests are in README.md (Testing): the grid replay (211 solutions), the menu tour, the
bot's first 25 puzzles twice and its whole game of 210 puzzles; a game's whole bot run is the same natively and in the web build
under Node.
