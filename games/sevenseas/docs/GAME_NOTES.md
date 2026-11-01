# Seven Seas Deluxe: behaviour notes (work in progress)

Addresses are virtual addresses in `re/sevenseas/bin/WinSS.exe` (linked 2003-01-29 with VC++ 6.0, image base
0x400000, no wrapper). The exe has RTTI for the 50 framework and library classes only: the game's own classes
have none (compiled without /GR), so their vtables were found from the constructors (a store of an address in
.rdata into `[this]`). Ghidra did not create about 125 of the game's functions (they are reached only through
vtables); `re/sevenseas/gapfuncs.jsonl` (candidates at 16-byte function starts after a `ret` or padding in the
gaps of the function list) was applied with `ApplySymbols.java`. Game code is 0x401000-0x427880 (about 410
functions); the framework starts at 0x427880 (Widget) and the library after 0x450000. `tools/funcmatch.py`
against Noah's Ark matched 1,269 functions (framework and string helpers, not the game's classes).

## 1. Classes (vtable, constructor, destructor, Draw)

| Class | vtable | ctor | notes |
|---|---|---|---|
| Board | 0x493360 | 0x401000 | 0x95a0 bytes of state; Draw 0x406400 (slot 16), Update 0x4060e0 (17), MouseMove 0x40ad20, MouseDown 0x40a4a0, MouseUp 0x40ae70; ButtonListener at +0x58 (0x493350) |
| BonusLevelDialog-like | 0x49348c | 0x40c7d0 | Dialog with a check box, "do not show this screen again" (BonusNag) |
| help/pause widget | 0x493580 | 0x40d330 | draw 0x40da40, "return to game" button |
| HighScoreDialog | 0x49363c | 0x40f7b0 | Dialog, draw 0x40faf0 |
| HighScoreWidget | 0x49370c | 0x40ff70 | draw 0x411260; file hiscores.dat |
| 0x4132b0 widget | 0x493844 | 0x4132b0 | "Click here to start", draw 0x413850, update 0x414830 |
| Dialog (register-ish) | 0x493910 | 0x416160 | "CANCEL" |
| IntroWidget | 0x4939f0 | 0x416900 | "WELCOME TO", draw 0x416df0 |
| 0x4175d0 widget | 0x493aac | 0x4175d0 | draw 0x4177d0 |
| NextLevel dialog | 0x493b68 | 0x417a30 | draw 0x417ca0 ("Next rank at %d") |
| Options dialog | 0x493c40 | 0x418760 | draw 0x4192e0 |
| 0x4199f0 widget | 0x493d04 | 0x4199f0 | draw 0x419ac0 |
| RecordScore dialog | 0x493e08 | 0x419f40 | draw 0x41a2f0 |
| Register dialog | 0x493ee8 | 0x41a650 | draw 0x41ae30 |
| 0x41b560 widget | 0x493fa8 | 0x41b560 | draw 0x41b5a0 |
| SevenSeasApp | 0x494080 | 0x41bc00 | 55 slots like Noah's; Init 0x41e580 (slot 30), loading 0x41db50 (11) |
| Dialog base | 0x49416c | 0x425540 | generic dialog |
| TitleScreen | 0x49423c | 0x4256f0 | draw 0x425c00, update 0x425fb0 |
| 0x4262c0 dialog | 0x494304 | 0x4262c0 | "Warning" (nag with a check box) |

In this framework the Widget vtable has 43 slots (Noah's 2002 one has 44): slot n here is slot n+1 there from
about slot 3 on.

## 2. Application (0x41bc00 etc.)

Offsets in the app object: images array +0x400 (88 images, table 0x4a5ab8), fonts +0x404..0x420, widgets: title
+0x424, intro +0x428, high-score widget +0x42c, +0x430 trial start screen, Board +0x434, +0x438 next-level, +0x43c help.
+0x494 byte (tutorial done), +0x498 int difficulty 1..3, +0x4f1..0x4f5 nag flags (registry BonusNag, WarnBlueNag,
WarnGhostNag, WarnSerpNag, WarnWhirlNag), +0x4d0 LastScoreName, +0x4e0 LastInternetScoreName, +0x3cc games played.
Registry key "PopCap\SevenSeas". The 88 images are loaded by 0x41d880 and also entered in a name map (+0x4b4);
data\anims.txt (0x41c960) is parsed into animation definitions (+0x4a4 map name -> frame list).

## 3. Board (0x401000-0x40b490; the port's `Board.h` has the original's offsets on every field)

An 11 x 11 grid of 42-pixel cells (`GRID`, `CellToPixel` 0x40a3f0); the player's ship starts at (5, 5) heading 4. Ports
(the whirlpools) sit in the four corners. The board is a turn-based game on top of a small state machine
(`BoardState`, field +0x138): 0 waiting for the player, 1 the ship turns, 2 it sails to the next cell, 3 and 4 a
whirlpool (spin away, appear elsewhere), 5 and 6 after a hit (the pause, the blinking new ship), 7 the player's
cannonballs fly, 8 the enemies', 9 the enemy ships turn, 10 they sail, 12 game over. The mouse handler (0x40a4a0) turns a
click on a neighbouring cell into a move and a click on the ship into a broadside; the cursor keys and the number pad do
the same (`UpdateMove`, 0x4035d0, reads `mKeyDown`).

Ship types (`Ship::mType`): 0 pirate, 1 red pirate, 2 sea monster, 3 blue pirate, 4 ghost ship. `PlanEnemyMoves` (0x404bb0)
chooses every ship's next cell and heading each turn (a pirate steps toward the player on both axes; the other kinds
use the flood-filled distance map `Flood`, 0x405660, or other rules, and the blue ones fire, `EnemyFire`, 0x404890); a
ship that sails into an island, a wreck or another ship sinks (20 points, `UpdateEnemies`, 0x405700) and leaves a wreck
(`mVisited`) that blocks the cell; one that sails into the player costs a life (`LoseLife`, 0x403200). The player's
cannonballs (`UpdateShots`, 0x403e80) sink the other kinds for 30, 40, 50 and 60 points (pirate, red, blue, ghost); the
sea monster takes no damage and its collision costs a life. A level ends when the planner finds only sea monsters left
(`mLevelComplete`); `SevenSeasApp::LevelComplete` (0x420770) adds 10 points per ship sunk in the level (half on the
beginner's levels below 10) and shows the next-level dialog. Every thousand points give an extra life (`AddPoints`,
0x403240).

`NewLevel` (0x401580) takes `rank = level + (difficulty - 2) * 10`: normal levels have `rank / 10 + 4` pirates, the last
digit of the rank adds ships (2: one more; 3: two more; 8: four more) or red pirates (4 and 6: `rank / 10 + 1`), blue
pirates (7) or ghost ships (9); from rank 31 on there are `rank / 10 - 2` more red pirates, and a rank ending in 5 or 6
(normal levels only) adds one sea monster. The beginner's first ten levels (negative rank) are the simple layouts with
three to five ships. Every tenth level (level % 10 == 0) is a bonus screen: a 9 x 9 dig for a
treasure whose first click places it (`SetupBonus`, 0x402540); the map is shown at the start of levels 11, 21, ... and a
warning dialog (blue pirates, ghost ships, sea monsters, whirlpools) the first time one of those kinds appears.

The undo list (`SaveUndo` 0x40aec0, `Undo` 0x40b0b0, `ClearUndo` 0x40b490) holds the score, the ships, the lives, the player
and the ports for each move; a whirlpool, a lost life and the changing of the corner whirlpools clear it with a reason
that the undo button's hint shows (`mHintReason`).

## 4. Screens and dialogs

The app creates and removes the screens itself: the title (0x4256f0; the loading bar, then the play link), the menu
`IntroWidget` (0x416900; difficulty and tutorial), the board with its side panel (0x41b560: score, level, lives, the
difficulty plate), the map (0x4175d0), the pirate's head (0x4199f0, the talk widget that speaks while a line plays) and the
high-score screen (0x40ff70). The sidebar's four buttons are made at run time from the backdrop and the label images
(`SevenSeasApp::TitleScreenDone`): undo (id 10), options (12), new game (14), quit (16). Dialogs: options (0x418760, the
sliders and check boxes), next level (0x417a30, "Next rank at ..."), record score (0x419f40), the bonus-screen note
(0x40c7d0) and the warning dialog (0x4262c0), both with a "do not show again" check box that is stored in the registry.
The beginner's demo (`DemoWidget`, 0x4132b0, `Demo.cpp`) is shown by `StartGame` (0x41eef0) when the difficulty is the first and
no tutorial is on: a board that plays itself for nine parts (the fade in, four captioned lessons, the fade out) with a fake
cursor, and a "Click Here to Start!" button that removes it (`RemoveDemo`, 0x41f300) and starts the game. Its random draws
(`CrtRand`) come in the original's order. Its button is drawn by the 2003 `DrawImageBox` (`Button2003.h`, 0x42beb0): the
pieces are cut at width/3 and height/3, so a picture that is not a multiple of three (the 98 x 50 wooden button) shows
different rows at the bottom and right than in the later framework. The demo's captions are two tables of pointers
(0x4a39f0, 0x4a3a10) that no code refers to by address, so `make_gametext.py` lists them by hand.

The help screen (`HelpWidget`, 0x40d330, `Help.cpp`; Options -> Help) is a board-sized picture: the sea, the four corner
whirlpools, one ship of each kind on its heading with the eight arrows around the player's ship and two red crosses three
cells to either side, a block of tutorial-font lines, and two black buttons (play the tutorial: `EndTutorial` with the
tutorial on; return to game: `CloseHelp`). While it is up, `TalkWidget::Draw` draws nothing (its test is the help widget at
+0x43c, not the map's at +0x438). The black plate of the buttons is the colors of a plain `ButtonWidget` all black.


The game-over screen (`HighScoreWidget`, constructor 0x40ff70, 467 x 480 at x 0xad) does nothing but load `hiscores.dat`
(0x410550: the best score of the day and the five best, each with a name, a score and a check value) when it is made.
Its update (0x412af0) shows it from the top down, 6 pixels an update (`+0x60`, the clip line of the draw, 0x411260); the
button comes out of the frame as that line passes it; when the line reaches the bottom, 0x410ae0 works out the place of the
score (0x411160: the first of the five whose score is lower) and, if the score is a place or the best of the day, opens the
name dialog with "You got ... place of all time!" (an unregistered player is asked to register instead). The dialog's
answer calls 0x412cc0, which puts the score into the list and into "Today's Top Score" when it beats it (an empty name
adds nothing); the original writes the file when the widget is deleted. The draw prints each of the four totals lines
25 times (offsets -2, -1, 1, 2 and 0 on both axes; black, then gold and yellow in the middle) for an outline, the rank,
the list on its panel with a colour per row that moves with `+0x7c` (a counter of ten updates; hue
`|(row + 2 + counter) * 10 % 170 - 85|`, lightness 0x60 for the name and 0x96 for the score), and the best of the day.
The rank names come from a table of 18 thresholds (two per name) embedded in `Screens.cpp`.

The name dialog's edit widget has its own colour table (0x4a5908: background and outline clear, text 240/240/255) and a
limit of 104 pixels (`+0x8c`), not a number of characters. The dialog draws the box under it (0x41a480): the picture
`editbox` (96 x 32) as a left cap, a right cap and the middle slice repeated under a clip, in a rectangle 4 pixels larger than
the widget on each side. The flash after an extra life (`Board::Draw`, 0x406400) draws the large ship (image 37), while
the side panel's lives are the small icon (image 31).
