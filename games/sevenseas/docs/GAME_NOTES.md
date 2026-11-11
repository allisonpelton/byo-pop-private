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
- **A new game's first-level warning is closed at once (a soft lock of the port, fixed 2026-11-08).** The Board's constructor makes the
  first level, whose whirlpool warning on Hard (and Average above level 10) is a dialog; the board is then put in front of it. The
  original's StartGame (0x41eef0) ends with `ShowWarning(4)` and `KillDialog(14)`, which closes that dialog again, so the warning first
  shows at level 2; the port left both calls out, so on a new Hard game with the warning not yet switched off the dialog sat hidden
  behind the board and took every click for good. Found by the web whole-game driver (`move` named the dialog's button and the click
  did nothing). `StartGame` now calls `KillDialog(DIALOG_WARNING)` at its end; the `late` autoplay run of `tools/check.sh sevenseas`
  shifted by it (the bot no longer answers that dialog: update 1958, score 200, moves 14, where it was 1852, 140, 13).
- **Registration and the way out of the game are left out (the user's rules, 2026-11-07).** The options dialog has neither the Register button
  nor the "more games" button (a link to the publisher's site); Help and High Scores share the row next to the footer button, each
  half of the dialog's width. The title screen has no register link, no count of plays left and no delay before the Play link for an
  unregistered copy (the gate on 15 games played, `mDelay`), and the app no longer counts games played (`mGamesPlayed`, which only that
  gate read). The title's registered-copy line (the thanks for registering, drawn by 0x425c00 above the Play link) is gone too (the user,
  2026-11-08, "Seven Seas still says Thanks for registering"): its draw call is removed and so is every text of the shareware and
  registration code from the text table (`GameTextTable.inc`, 46 entries: the register dialog and its prompts, the register-to-record
  texts of the high-score dialog, the Internet-score texts, the site and "more games" texts, the title's registered and unregistered
  lines, the plays-left and nag texts), by the generator's `EXCLUDE` list (`tools/make_gametext.py`). The Play link and the loading bar
  stay where they were, the line's place is empty. Against the original's title (same crop, 3 and 26 pixels in) the screen differs only
  in the line's box and the sparkles. The publisher's logo and its address picture (`IMAGE_POPCAPLOGO`, `IMAGE_POPCAPURL`) are art on
  the title, not buttons or links, and stay; no other image or data file of the game holds registration text.
- **Loading is fast in the port.** The framework runs the loading thread in one go (the sounds, the arrows, the animations): the
  title's Play link takes a click 50 ms of game time after the start (-fast), and the port added no wait of its own. The title still looks
  at the thread every fourth update, the original's own cadence, a 40 ms step at most.
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

## 8. Running the original through origrun (2026-11-07)

The update counter is `[0x4e21bc]+0x378` (`mUpdateCount`, incremented by 0x430ba0 once the app is loaded), not `+0x368`, which is
the app's frame time and stays at 10: with it every `at` waits for an update that never comes and the run looks stalled at update 10
(the shared session found this; the main thread's sleep in the frame loop at 0x431c70 is normal). `tools/origrun/sevenseas.cfg` holds
`counter=[0x4e21bc]+0x378`. The title takes no click until the original has finished loading, which varies from run to run (a few
seconds to over a minute of real time): a script that clicks the title early is ignored without a word, so put the first click late
(about update 6500 on the real clock) and look at the first shot before trusting the rest. Clicks and moves take the game's own
coordinates (640 x 480), not the picture's.

Compared with it (sandbox, real clock, 2026-11-07): the twelve tutorial hints at their steps, the hint panel's text against the
port's native shots at the same steps, mean difference about 2 grey levels (the port's shots are resampled from 1280 x 960), and the
text lines at the same rows, including step 9 (second line only) and step 14. The Easy demo's captions run as in the port. The board's first box after the demo's "Click Here to Start!" is compared too (2026-11-07, origrun sandbox with the real clock, shot f0
against the port's shot at the same moment): the four text lines sit at the same pixels (the text's bounding box is 193 x 62 in both,
at the same offset as the window's border and title bar, 3 and 26), and the box's region differs only where the two runs' boards and the
arrow's tour differ (the port's board is not seeded like the original's here). Click the demo's button once: a second click on the board
moves the ship and takes the box away.

## 9. Test commands (docs/TEST_COMMANDS.md; `src/testhooks/Commands.cpp`)

Script commands of the port (`-script`, later the live channel), answered on stdout with one line each (`OK <command>`,
`ERROR <command>: <reason>`, `STATE {...}`, `MOVE {...}`). A command that is never given changes nothing; `tools/check.sh sevenseas`
(the autoplay games and their logs) gives the same results as before them. Tried with `tools/commands_test.txt` and
`tools/commands_test2.txt` (`tools/run_script.sh`).

| Command | Does |
|---|---|
| `state` | `STATE {"screen","mode","level","score","lives","kills","board":{"player":[x,y],"heading","state","levelOver","ships":[[type,x,y]...],"islands":[[x,y,kind]...],"wrecks":[[x,y]...],"whirlpools":[[x,y,open]...]}}`; `screen` is title, menu, demo, board, bonus, map, options, help, highscores; `mode` is easy, average, hard or tutorial; the game keys are absent off the board |
| `goto <screen>` | menu, options, help, highscores (after a game), or easy, average, hard, tutorial through the menu's own buttons (the game starts at the next update); title and credits answer ERROR (the game has none to go to), and so does any `goto` while the title is up |
| `level <n>` | `NewLevel` for level n of the current game, as `-autoplaylevel` does (not in the tutorial or the demo) |
| `set score\|lives <n>` | the board's score or lives |
| `win` | the level is complete now (the flag the board sets when the last pirate is gone; the game's level-complete screen follows) |
| `lose` | the last life is lost now through `LoseLife`; the game over and the high-score screen follow as in play |
| `seed <n>` | the game's random numbers (the C library generator the original used) are seeded with n |
| `move` | `MOVE {"kind":"fire"\|"wait"\|"sail","cell":[x,y],"input":[{"move":[gx,gy]},{"click":[gx,gy]}]}`, the autoplay bot's next move (the bot's choice is `ChooseBotMove` in `testhooks/Bot.h`, shared with `-autoplay`) and the input (a hover and a click, in game coordinates) that makes it, or `MOVE null` when no ship waits for a click or on the bonus screen; it draws no random number of the game's and changes nothing |
| `unlock all` | ERROR: the game has nothing locked |

`move` also answers the screens between moves, so that a driver needs nothing else: a dialog's button, the title's Play link, the
menu's Play, the demo's and the map's buttons, the high-score screen's, and a dig of the bonus level, as `{"kind":"ui","what":"dialog 14",
"input":[...]}`; they come before the ship's own move. `state` starts with the screen and `update` (the app's update count, so that a driver's transcript can be laid on the original's counter) and adds `over` (the last life is gone), `busy` (nothing to click yet), `dialogs`
(id, whether it sits in the widget manager, box, button) and, on the menu, `menu` (the Play and Tutorial buttons and the three wheels, in
game coordinates; the difficulty wheel is a real click, then Play). `tools/web/hard.js` plays a whole Hard game that way.

## 10. Fix candidates (2026-11-08)

None for the fixes mod. What was checked: the whole play of the port against the original in the identical-play pass and in the
real-input passes (native and web, Easy, Average, Hard, the tutorial, a saved game), looking for waits that only cost the player
time and for bugs of the original that do not break play. The bugs found broke play, so they are fixed in the port itself (the
user's rule): the pirate's box that took the clicks of the squares under it (section 5), and the tutorial's missing last hint and
its mis-placed first box. Nothing else came up: the title, the demo and the level-end screens answer a click at once, and the
ships' sailing and the broadsides are the game's own animation, which holds the player only as the game's rules do. This is a
result of the play-throughs, not of a line-by-line audit of the export for waits, so a later find is added here.

## 11. Performance (2026-11-08)

The original and the port, back to back on this machine (a Core i7-12700F, Windows 11), for the user's morning report. The machine
was not quiet: other sessions' games ran meanwhile (3 to 7 game processes counted by `tools/gamecount.py` at the start and end of
each run; `build/perf/procs_*.txt`, git-ignored), so the numbers compare the two here and now; the performance pass re-measures on a
quiet machine. Both ran windowed, in real time, with no sound (the port under rundesk with SDL's silent driver; the original under
origrun on the host with its silent stand-in and the seed pinned), a fresh user folder each, no mods. The input is the same on both
sides (`tools/profile/perf_orig.txt` and `perf_port.txt`: the title at 2 s, the menu's Play at 10 s, the Easy demo's button at 24 s,
then a broadside at the ship every 1.5 s to 88 s, quit at 90 s). One run is one process from its creation to its exit, sampled
every 50 ms by `tools/profile/perfmin.py` (a copy of Bejeweled's sampler); the window for the processor figures is 20 to 80 s.

| | original (3 runs) | port (3 runs) |
|---|---|---|
| Peak working set | 55.4, 65.0, 65.1 MB | 66.1, 66.2, 66.3 MB |
| Peak private bytes | 41.5, 50.9, 51.2 MB | 65.9, 65.9, 66.0 MB |
| Processor time in the window | 2.2, 8.7, 7.5 % of one core | 4.8, 5.0, 5.5 % of one core |
| Processor time, whole 90 s run | 3.2, 8.3, 7.6 s | 5.1, 4.9, 5.7 s |
| Launch to the first screen that takes input | 1 to 2 s (origrun's log has a one-second clock: the counter's first update comes in the second after the launch, 3 runs) | 1.07 s median (1.04 to 1.15, 5 runs: a click on the title's Play link every 50 ms, the first one the menu answers) |

The original's first run sits apart from its other two (55 MB and 2 % against 65 MB and 8 %); its script ran the same lines at the
same updates, and the cause was not examined (a different state of the game at that moment, as the Easy demo's timing varies, is
the likely one), so the three are given as measured. The port holds about 15 MB more private memory than the original's
three-run range and about the same working set; its processor use sits between the original's quiet and busy runs. Seven Seas
loads in about a second in both: the port added no wait (the framework runs the loading thread in one go) and the original
has none that this clock can see. The tools: `runperf.sh` (both sides), `runport.sh` (the port alone: its Popen needs absolute paths
under rundesk), `loadtime.py` (the port's load time through the live channel).

**What could not be measured.** The web build's load time: the Browser pane is hidden, its timers are throttled and
`requestAnimationFrame` does not run, and a load in a player's browser is dominated by the network and the wasm compile. The
original's load to better than a second (no state command; origrun's clock is in whole seconds). The GPU's own load, and full screen.

## 12. Coverage check (2026-11-11)

The whole check of `docs/COVERAGE.md`. origrun's `--coverage` recorded which of the original's functions ran in 16 runs on the
host, on the virtual clock (about 26 times real time). `tools/coverage/run.sh <name> <script> <seed> [timeout]` runs one (it
waits for room among the 3 game processes, and with `SAVE=<savegame.dat>` stages a saved game into the copy first),
`tools/coverage/mkscript.py` writes the scripts (the title, the menu and then clicks over the board's cells in a seeded random
order, the panel's buttons and every dialog, or a run from a saved game: the Continue dialog, the Treasure Island nag, the digs)
and `tools/coverage/mksaves.py` makes the four saved games by the port's own test commands (the port writes the original's file
format, and the original read them). The screens' click positions come from the port's own shots and from two sandbox shots of
the original (its Options dialog is taller than the port's: the Register button and the "Visit PopCap.com" button are in it). A
click that comes before the original has loaded is ignored without a word, so the title's Play link is clicked six times, by a
spot of it that is empty in the menu (360, 385).

The runs: `perf` (the demo), `e1`, `e2`, `e3` (a new Easy, Average and Hard game and 150 to 700 clicks), `t` (the tutorial), `u2`
(the panel's buttons, the options with its sliders and check boxes, Help, High Scores, New Game and Quit, answered both ways),
`fs` (the Fullscreen and Custom Cursors boxes), `p1` (the port's own bot's first Easy game replayed by update number: it falls out
of step with the original on the first board, so it added nothing) and, from saved games, `vbonus` (Treasure Island), `vlose`
and `vno` (one life, Continue answered Yes and No: the loss, the high scores and the name box), `vl9` (Average level 9: the
crimson pirates fire), `vl9empty` (the level is won at once: the next level is the bonus level, then the map), `vlvl4` and
`vlvl4b` (a Hard level 4 save). Nine ended by themselves, five at origrun's timeout (the last click of a script that ends in a
dialog does not leave the game) and two with an access violation (below).

`games/sevenseas/coverage.txt` sets the game's own code as one range, 0x401000 to 0x427870 (399 functions; the framework starts
at the Widget at 0x427880). **The runs reached 362 of the 399. 0 are reached with no counterpart in the port**; at first 170 were.

- **Placed: 13.** The addresses of functions the port translates inline or in another shape are now in the comment above the
  function that holds them: 0x40a410 (the board's StartTalk call), 0x40b0e5 (the second half of Undo, which Ghidra left outside
  the function), 0x419ab0 (the end of a talk), 0x426140 (the tail of the title's sparkle loop, likewise), 0x411190 and 0x4111c0
  (the high-score screen's AddedToManager and RemovedFromManager: the original adds two buttons), 0x412c80 (the high-score screen's button handler: its other branch, the score upload, is not ported), 0x41f280 (HasSavedGame; the
  comment said 0x41ac10), 0x418fc0 (the options dialog's SetColor, which the constructor's colours replace), and three the port
  had under the wrong address: the options dialog's slider handler is 0x4196f0 (the comment said 0x419730 / 0x419770), its
  buttons are 0x4197a0 (it said 0x4196f0), and the check box handler 0x419730 does nothing in the registered game.
- **Left out on purpose: 194 `none` lines.** What the compiler made (40 static initializer stubs, 20 deleting destructors, 11
  vtable thunks, 10 empty or constant functions), the original's std::string library (22), the support of the anims.txt parser
  0x41c960 (map and list instances, 46: the port parses into std containers), the destructors of dialogs and widgets (15),
  registration (the Register dialog and its page link, the number check, the windowed-mode nag of the unregistered game: 19) and
  the online score submission (11: the "Submit to Internet High Score List" button, its dialog, the upload and the "High Score"
  answer; README.md lists it as not ported).
- **A slip found: the click of the options and nag dialogs.** The original's OptionsDialog (0x419770) and NagDialog (0x40d280)
  override ButtonPress to play sample 0 after the base class. The base forwards only the footer button to the app, which clicks,
  so Help and High Scores clicked in the original and not in the port, and the footer button clicked twice in the original. Both
  overrides are in the port now (`OptionsDialog::ButtonPress`, `NagDialog::ButtonPress`). No state changes; sound is not in the
  baseline.
- **Defined in the port and never reached: 6, each read again against the export.** `Board::SpawnSparkle` (0x4030f0: the two
  draws in the original's order, y then x) and `Board::EnemyFire` (0x404890: the headings, the scan of three cells each side, the
  shot's fields, the trail flash and the sound; both match line by line, and EnemyFire ran in the identical-play pass's lost
  game), `Board::MouseUp` (0x40ae70, a call of the base class), `Board::ButtonDepress` (0x40ae90, "Click here to start playing!"
  at the end of the tutorial, which needs the whole tutorial: it ended the tutorial in the identical-play pass), the options
  dialog's `SliderVal` (0x4196f0: origrun cannot hold the thumb to drag it, and a click on the track does not call it; two lines
  that match the export) and the warnings' variant of `NagDialog::Draw` (0x4266f0: the port's one function is the counterpart of
  both copies, and the bonus nag's copy 0x40cbe0 ran).
- **Candidates for taking out: none.** Nothing in the game's code is unreachable by play except what is `none` above.
- **Found on the way.** (1) Two of the runs (`e3`, `vlose`) reached 0x41f340, the original's score upload: random clicks hit its
  "Submit to Internet High Score List" button on the high-score screen (y 356 to 386). The function wrote its temporary files and
  may have opened a connection to the PopCap site; no browser or program was started. The scripts now keep their clicks off that
  row. (2) The original dies with an access violation (0xC0000005) when it loads the Hard level 4 save the port wrote (`vlvl4`
  and `vlvl4b`, the same place both times): the save has whirlpools and a warning up, and the port's writer has been compared with
  the original's file only for the levels of the identical-play pass. An oddity of a file the original did not write; not chased.
  (3) The Fullscreen check box does nothing in the original's registered game but tick (its handler 0x419730 acts only for the
  unregistered game, after the nag): `fs` reached no mode switch, and the port's box does the same.

Cost: most of the session's time went to finding the original's click positions (two sandbox runs) and sorting the 170
functions of the first report; the runs themselves took about 25 minutes of machine time.

## 13. Performance pass (2026-11-11)

The sampled profile of the port over the demo's 90 s (`games/bejeweled/tools/profile/cpuprof.py`, the port under rundesk in
real time with `tools/profile/perf_port.txt`; 16.8 G cycles in 92 s): **88.0 % of the cycles are in ntdll.dll** (for a 32-bit
process, system calls and waits: the frame's present and the timer), 5.1 % win32u.dll (window and message calls), 1.9 % the
GPU driver, 0.8 % imagehlp.dll (the shared cost already listed in `docs/COORDINATION.md`), and **2.6 % the game's own code, all
of it loading**: Ogg decoding (`_vorbis_lsp_to_curve` 0.4 %, the Huffman set-up and the inverse MDCT 0.5 % together), GIF
decoding (0.4 %), JPEG decoding (0.2 %). No function of the game's logic or drawing (the board, the ships, the sea, the
sprites) is in the profile at all; the largest game function of play is below 0.1 %. The processor time of the port was 4.8 to
5.5 % of one core in the morning's measurement (section 11), as for Bejeweled, whose frames are the cost and not its code.

So there is nothing in Seven Seas' own code to speed up that a player or a frozen baseline would see, and none was changed:
the decoding at load is about 0.2 s of one core in a load of about a second, and a faster or lazier decoder would change when
work happens, not what it does, for a saving a player cannot notice. What remains is shared and already listed by the
coordinator (the per-frame present and Direct3D's 20 to 25 MB of private memory, the 20 MB of image pixels the framework keeps
after their upload, the GIF quantizer, imagehlp.dll); this game adds no new one. The memory gap to the original (about 15 MB
of private bytes, section 11) is those shared costs. The frozen baseline (`tools/check.sh sevenseas`) is unchanged, since the
code is.

A slip: this profile run was started while 3 other game processes were running, so the machine held 4 for 92 s; the
count is checked before a run's start, and the run's exit code was not read before starting. Nothing else ran, and the run's
files (`build/perf`) are small.

## 14. Removal of code the port does not need (2026-11-11)

What the coverage check (section 12) found: every function of the game that play can reach is in the port, and the original's
code the port leaves out is listed there. The other way round, a scan of the port for what nothing calls or uses (every member
function defined in `src/` whose name appears nowhere else but its declaration, and every id of `Res.h` named once) found:

- **Taken out:** the dialog ids of the Register dialog, its invalid-code box, the high-score note, the online score submission
  and the windowed-mode nag (`DIALOG_REGISTER`, `REG_INVALID`, `HIGH_SCORE_NOTE`, `SUBMIT_SCORE`, `NO_WINDOWED`) and a
  `KillDialog(DIALOG_REGISTER)` of a dialog the port does not have (commit 74d0c0fb); `SevenSeasApp::UpdateCursor`, an empty
  function nothing called, and `StopTalk`, which nothing called.
- **Kept on purpose:** the resource table (`Res.cpp`, `Res.h`) holds every image and sound of the original's list in its order
  and with its ids, since the names are looked up in that order; 10 images and 8 sounds of it nothing in the game's code names
  (the online score's picture, a few fonts and sprites, and the pirate's mouths and talking sounds, which the code reaches by
  their numbers, 0xe to 0x15: the readability pass names them). Dropping the picture of the removed online score button would renumber every id after it for 40 KB
  of memory; not done.
- **Left alone:** the Treasure Island and warning nags are the registered game's own dialogs (they stay), and the registry
  values `BonusNag`, `Warn*Nag` are theirs. The registration and outbound-button removal was done earlier (section 8's list);
  a search of `src/` for register, trial, unlock and URL finds only the framework's registry hooks and the test command
  `unlock`, which answers that the game has nothing locked.

`tools/check.sh build sevenseas` passes with the baseline identical.

## 15. Readability pass (2026-11-11)

`python tools/namecheck.py games/sevenseas` reported 70 findings and now reports none; `tools/objsnap.py` shows that all 18
objects (471 functions) compile to the same instructions as before the pass, and `tools/check.sh sevenseas` is unchanged
(baseline identical). `tools/splitfields.py` has no per-function export folder to read here (only `functions.jsonl`), and
reported no member declared twice.

- **Names (rule 2).** `mFlag4c4` is `mPirateWelcome` (the pirate's welcome at the first menu, until a game starts) and
  `mUnused_95008` is `mCellFlashFloor`, with a comment on what reads it (the glow's reach and the hint arrows) and that only the
  level's reset writes it. The one-letter names that ran over 10 lines are `aState`, `aBoard`, `aCand`, `theBoard`, `aWriter`,
  `aReader` and `thePos`.
- **Addresses (rule 6).** 50 functions got an address or a `port:` marker. Addresses: the dialogs' and widgets' destructors and
  the second hook of each pair that one comment named twice (`0x4190c0`, `0x4172e0`, `0x4261b0`, `0x40cb60`, `0x426670` and the
  like), `ReadFromRegistry` 0x41c630, `ShowHelp` 0x421980, the demo's destructor 0x413700 and the high-score screen's button
  handler 0x412c80 (which the coverage list had as the score upload, a branch it also holds). `port:` markers: the helpers the
  original writes inline (the demo's `InitShip`, `Sink`, `DrawShip` and `DrawBall`, the board's heading and step helpers),
  the framework hooks the original leaves to its base class, and the game-text reader. The test hooks' files carry the marker in
  their first comment.
- **Numbers (rule 7).** The pirate's lines and the bonus screen's losing sound are named (`SOUND_TALK...`, `SOUND_BONUSLOSE`)
  where the code gave `PlaySample` their numbers (0xe to 0x16).
- **Left as they are.** The music orders (`SetMusicOrder(0x10...)`), which no list of the game names, and the original's
  geometry in hexadecimal, which each line's comment explains. The three copies of the heading helper (`StepHeading`,
  `StepHeadingOf`, `HeadingFor`) are separate on purpose: each stands for the original's inline code at its own place.

## 16. The PopCap Game Collection version (2026-11-11)

The Collection disc (the user's ISO) has Seven Seas Deluxe (the group `SevenSeasDeluxeComponent`, 176 files). `unshield` read it from
the read-only mounted image, dismounted afterwards, into the main checkout's `re/_installers/SevenSeasDeluxeCollection` (git-ignored).
Unlike Typer Shark's, the Collection's copy has no launcher: `WinSS.exe` is the game itself, under the same name, beside `fmod.dll`,
`license.txt`, `readme.txt`, `scoreul.tpl` and the folders `data`, `images`, `music` and `sounds`.

**The same game.** All 172 files the port stages (every picture, sound, song and data file) are byte for byte the Collection's. The
four files only the Collection has are the licence, the readme, the score template and fmod.dll, which the port does not use. Of
`WinSS.exe` (both 720,896 bytes, the same sections) 7 bytes differ, all in `.text`: one byte at 0x42ce4e (`mov [ebp+0x32c], bl` in the
port's copy, `cl` in the Collection's) and six at 0x42ded0, where the port's copy has eight NOPs and the Collection's has
`xor eax,eax; inc eax; ret 8`, a function that returns 1: both are in the framework's code, at addresses above the game's own code
(0x401000 to 0x427870), and the port's copy is the one origrun's patch list had touched. `.rdata` and `.data`, where the texts are, are the
same, so every text of `GameText.cpp` matches by address and hash.

**The port on the Collection's files.** With the Collection's `WinSS.exe` in the staged folder, `tools/check.sh sevenseas` gives
the same result as on the other copy (baseline identical; beginner 2 over at update 5868, score 380; bonus 1 over at 2746, 680; late 1
over at 1958, 200), and the port needs no change: it reads `WinSS.exe` by that name from the game folder, as the Collection's has it.
The staged copy was put back afterwards. The Collection's own exe was not run: its game code is the same, and a run could show nothing
the identical-play pass did not.
