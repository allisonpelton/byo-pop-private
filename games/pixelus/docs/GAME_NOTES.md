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

(To be extended as the Grid is ported; see `games/pixelus/src/Grid.*` once it exists.)

## 3. Files in the game folder

`images/`, `ncframework/images/` (loading graphic and bar), `data/*.txt` (ImageFont descriptions) and `data/story/*`
(story pages), `levels/`, `sounds/`, `music/` (songs, `pixelus in-game playlist.txt` with 40 numbered tracks),
`properties/partner.xml`, `pixelus.exe` (the game's texts are read from it).
