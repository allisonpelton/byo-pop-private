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

## 5. Deviations from the original (each without a visible effect)

- **Help buttons' pressed look (0x40d330).** The original gives each help button `mDownImage` = `CopyImage(IMAGE_DBUTTONBLACK)`; the port
  sets none and draws the black plate with the framework's flat path (`COLOR_BKG` black). `DBUTTONBLACK` is all black, so a pressed
  button shows black text, offset by 1, over all black either way. To confirm with shotdiff once captures of the original work again.
- **The pirate's box after "No" in the Continue dialog (a bug of the original that the port fixes).** Case 3000 + 0xf of 0x421a10
  starts a new game (0x41eef0) without putting the pirate's talk widget away, as 0x41ed70's no-saved-game branch and case 0xf (Continue)
  do. On Average and Hard his 200 x 200 box at (142, 10), invisible once he has faded out at the first update, still takes the clicks and
  the hover of board columns 0 to 3, rows 0 to 4 for the whole game: no arrow and no move on those squares (the user's "column 4, rows
  1-3", counting from 1). The port puts him away (mode 0, talking flag off, widget hidden) before that StartGame. The user, 2026-11-07:
  "That sort of bug shouldn't be part of a fix mod, IMO", so a bug that breaks play is fixed in the port itself. To see the original: a
  saved game (the New Game dialog's Yes), Average, Start New Game, "No", then hover a square next to the ship under the box. Natively
  checked with real window messages on 2026-11-07 (tools/realinput.py `saved`): arrow and move on those squares.
- **No Register button.** The options dialog's Register button is hidden (registration is taken out by the user's rule).
- **Options dialog's four extra buttons' label colour** is gold, and a hovered label stays gold (captured in the Windows Sandbox,
  2026-11-01, `build/ss-cmp/o`: the original's Help label under the mouse matches the port). The Yes and No labels do not
  change under the mouse either (`qh`), nor do the help screen's two labels (`hh`), although the decompilation reads colour 1
  while the mouse is over a button (+0x1f); a pressed button is not captured yet (origrun cannot hold a button down).

## 6. Comparison with the original, state of 2026-11-01 (sandbox captures)

Static screens match pixel for pixel (title, menu, quit and options dialogs, help screen, the first board and the arrows under the
mouse). The map scenario (six sailing moves, a sinking and the new ship, 84 shots) shows the same board, ships, kills and score
texts in every shot. The 2003 exe has no `mHasPendingDraw` or `mUpdateAppState`, but its main loop (0x431c70) counts the updates
since the last draw at app+0x364, which origrun takes for both (`--pending-draw` and `--update-state` `[0x4e21bc]+0x364`), so a
shot lands exactly on the update it says. `--input-eip 430ba0,430d40` (the update, vf05) holds each click until the game is
between updates, and the log's "input at counter N" is the update that read it: the port's script puts the click's move, down and
up on that update and nothing else is guessed (origrun posts all three back to back in the sandbox). With that, the pirate's start
delay and every board state of the map scenario (six moves, kills, the sinking) match the original update for update; 25 of the 84
shots match exactly and 39 more differ only faintly (the corner whirls by at most 8 of 255). The new ship's cell after the sinking is not a fault of the port: it was (2,2) in one original run and (1,9) in a second
run of the same script and seed (shots at every update from 5726), and (1,9) is the port's cell. Logging every `CrtRand` draw
of the port with its caller showed the same sites as the decompilation (two per update for the waves, one per enemy move for the
whirls, two per `PlaceShip` attempt), and `PlaceShip` tries (5,2), rejected by the ship next to it, then (1,9). So the original's
draw count between its start and the placement varies a little from run to run (something timed, not found), and an exact match
of the placement cannot be had; the corner whirls and waves are insensitive to the shift (the waves clamp their speed).

**Is3D.** The 2003 exe has no Direct3D mode at all (its export has no D3D or DirectDraw 7 calls beyond the plain surface), so
there is no Is3D=0 original to compare with and no Hardware Acceleration checkbox to port. The port's framework picks its
renderer by what SDL gives: the same script (the first moves of the map scenario, seed 7) drawn by the accelerated renderer and
by `SDL_RENDER_DRIVER=software` differs in 16813 pixels of a shot by at most 3 of 255, never more, so either mode is as
close to the original's software drawing as the other.

**Continue (fixed 2026-11-01).** After "Continue" the port left the pirate's talk widget visible, a 200 x 200 box at (142, 10) over the
board that took the clicks (and hover) of every cell under it, so moves failed on some squares and showed no arrows there until a
new game: the original's case 0xf of 0x421a10 sets its mode to 0, clears the talking flag and hides it, as a new game does. The
likely cause of the reported "controls fail at random times". Found by loading the original's own savegame.dat (its format is the
port's; the file the original writes at exit) and clicking a cell under the box.

**New game from the Continue dialog (fixed 2026-11-07).** The same box took the clicks again after the Continue dialog's "No":
case 3000 + 0xf of 0x421a10 starts a new game without putting the pirate away (only 0x41ed70's no-saved-game branch and case 0xf do),
so on Average or Hard (Easy goes through the demo and its button first) the board's first update fades him out and his 200 x 200
box, still visible, took the clicks and the hover of columns 1 to 4, rows 1 to 5 for the whole game: no arrow and no move on
those squares (the user's "column 4, rows 1-3", 2026-11-07). The original has the same fault, so this is a fix of its bug that the port
makes now at the user's request: the pirate is put away as in ContinueGame. Seen in the web build with real mouse events (a saved
game, Average, "No"), and gone with the fix. The tutorial's first-game box and its hint panel were checked in the same run.

**Tutorial text (fixed 2026-11-07).** Two faults of the tutorial's texts. The last hint (step 14, on the line in the middle of the
panel: "That's it! ...") never showed, because the generator of the text table listed 14 texts for each hint table and the tutorial has
15 steps. The first box ("This is your ship...") drew its four lines at 0, 0x12, 0x23 and 0x34 below the row; the exe's code (0x406400,
read from the bytes, the decompiler dropped the first two offsets) puts them at 0x12, 0x23, 0x34 and 0x45, 17 pixels lower, inside the box.

**Continue: gun smoke on the loaded ships (not emulated).** The original's saved-game loader fills a ship's position, heading and
type but not its nine gun-smoke timers (`mTrail`), which keep what the heap had there. In the sandbox the two ships showed
cannon blasts in the diagonal cells (lower left, upper right) 60 updates after the board came up, and one of them again at
update ~250: a timer that counts down from a large leftover value shows its blast once it falls below 14. The port starts them
at -1 (no smoke). The leftovers differ with the heap, so there is nothing to copy; the effect is a second of smoke on a
Continue, now and then. Everything else of the Continue, the level-complete screen and the next level matches (shots lc7000 and
lc7300 are pixel-identical).

The bonus screen's first message line ("Try to find the", "The Treasure is", "You Win!" and the like) is drawn at the top of the board (y 31), only the second line at the bottom (height - 0x1e); the port had both at the bottom. The Treasure Island nag's button label is the dialog's gold. Where the treasure lies (and so the direction text after a dig) differs between the original and the port in the comparison run: the original's draw count before the bonus level varies from run to run (see the new ship's cell above), so it is not compared.

**Movement arrows (fixed 2026-11-01).** `DrawCellArrow` (0x409840) draws, in each of its three column cases, the picture of the neighbouring
cell under the mouse; the port drew only the middle column's (the cells above and below the ship), so hovering the cells to the left or
right, or the diagonals, showed no arrow (the user's "movement arrows don't show up properly"; native and web alike). The first board
comparison had only hovered the vertical ones. Nine hovers around the ship (all eight neighbours and its own cell) now match the original.

## 7. Oddities (what the original does that the port does not copy)

Each is described in section 6 and in IDENTICAL_PLAY.md: the new ship's cell after a sinking (the original's own draw count varies
between runs), the gun smoke on the loaded ships of a continued game (uninitialised timers), and the treasure's cell on the bonus
level (random, same variation).

## 8. origrun on Seven Seas stalls at update 10 (seen 2026-11-07, not solved)

Every run of the original through origrun (host under rundesk, and the Windows Sandbox), with and without the seed and
registration patches, `--sound stub` or `nodriver`, with and without the virtual clock, with a script that starts with
`focus regain` or none, stops with the counter `[0x4e21bc]+0x368` at 10 and the title screen's loading bar empty. The main
thread sleeps (CPU use near zero, one thread in `Sleep`, the rest in waits), so the game is not busy and not showing a
dialog (msgbox.log is empty). An origrun built from the commit of 2026-11-01 (6a225cbe), which ran this game in the
identical-play pass, stalls the same way on the same copy, so the 2026-11-03 stand-in changes are not the cause. What
changed since the pass is the machine (Windows 10.0.26300): a later OS build may have changed what FMOD 3 or the old
DirectSound path expects. Not looked for further (about an hour spent); a way in would be a debugger's stack of the
sleeping loader thread, or a run of another FMOD 3 original (Bonnies) to see whether it stalls too.
