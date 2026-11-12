# Rocket Mania Deluxe: notes on the original and the port

The game is PopCap and Nuclide's Rocket Mania Deluxe (2003, Steam version 1.01). Its exe (`RocketMania.exe`) is PopCap's
2006 wrapper around the game proper; the port reads the game's texts from it (`GameText.h`) and never runs it. The game
image is linked 2003-09-08, base 0x400000, app class `WinTubeTricksApp` (the port's `RocketApp`). Its framework is the
2003 SexyApp (`FRAMEWORK_2003.md`): flat widgets, 62 app slots. Addresses below are the original's.

## Where things are (the original's classes)

| Class | Constructor | Role |
|---|---|---|
| WinTubeTricksApp | 0x42aec0 | the app: Init 0x42b9b0, loading thread 0x42bdb0, button handler 0x42ed10 |
| LoadingScreen | 0x4104b0 | the loading picture and bar, then the play link (ported: `Screens.cpp`) |
| GameSelector | 0x404e30 | the menu with six buttons (ported); a mode button opens Difficulty or Continue |
| DifficultyDialog | 0x404380 | id 0xc: EASY, MEDIUM, HARD, CANCEL (ported) |
| ContinueDialog | 0x402630 | id 0xf: NEW GAME or CONTINUE SAVED GAME (ported, the loading half waits for the Board) |
| Board | 0x41ae80 | the game: Update 0x426f40, Draw 0x41f540, input 0x4285b0 to 0x4283e0, helpers 0x41b270 to 0x429460 |
| Rocket (the effects) | 0x4172f0, manager 0x41a830 | the particle effects: fireworks, sparkles, flames and the menu's petals (ported, Effects.cpp) |
| TalkingHead | 0x4161a0 | the dragon announcer and the hover tips 0x413aa0 (ported, TalkingHead.cpp); its speech bubble, SpeechBubble 0x415c70, is never shown and was taken out (see "Coverage check") |
| HighScoreDisplayDialog | 0x4064e0 | id 0xe |
| OptionsDialog | 0x411ba0 | id 1 |
| RecordScoreDialog | 0x414060 | the name entry after a high score |
| LevelUpDialog | 0x40c2c0, MoneyDialog 0x4118d0, BuyRocketsWidget 0x401010, CreditsDialog 0x403990, HintDialog 0x408f60 (id 0xb), Points 0x429ed0 | |
| RegisterDialog 0x414790, RegisterToLoadDialog 0x415020 | | shareware nags; the Steam build is registered, the port skips them |

## Menu (GameSelector)

Six `DialogButton`s (picture DARKBUTTON, font Domo 14) at x 4, 134 wide, 35 high: classic y 167, arcade 210, strategy 253
(label colour 0x97efc2), high scores 304, options 347, quit 390 (0x97c2ef); hover colour white, mouse insets -10. Button
ids: arcade 0, classic 1, strategy 2, quit 3, high scores 4, options 5. The hovered button sets the hint (a pointer table
at 0x4e44d0) drawn in Verdana 12 bold at y 460, centred, fading in over 26 updates with colour (0, a, a, a). The
modes are 0 arcade ("falling tiles"), 1 classic ("launch the rockets before dawn"), 2 strategy ("use your matches wisely").
Difficulties are 1 easy, 2 medium, 3 hard.

The selector is added to the manager and sent to the back (`BringToBack`), so its buttons are above it.

## Saves

`Players//savea.dat` (classic), `saveb.dat` (arcade), `savec.dat` (strategy): the first four bytes are 0xFADA0002; then
one unused value, the difficulty, the score and the level index (the continue dialog shows level + 1; strategy has no
levels). The port keeps them under the user folder as `Players/save?.dat`.

## Differences and decisions of the port

- Texts are read from the user's exe by address (`GT(0x...)`); none is in the repository.
- The hint's alpha reaches 260 in the original (the test is `< 255`, the step 10); the port draws it at 255.
- Not ported yet (marked in the code): see the list at the end of this file.
- The dialogs' buttons arrive at the app as 2000 + dialog id (yes, ok) and 3000 + dialog id (no, cancel) like in the
  other 2003 games; `RocketApp::ButtonDepress` mirrors the original's handler (0x42ed10).

## The Board (read so far; not ported)

141 functions, 60 KB, 0x41ae80 to 0x429ed0. Fields are the original's offsets.

- The grid is 6 columns (board+0x4e4) by 9 rows (board+0x4e8) of 42 by 42 cells (corrected 2026-10-26: the first notes had
  them the other way round; 0x422620 and 0x422a90 take x from piece+0x54, the column, and y from piece+0x50, the row):
  x = 250 + 42 column (0x4220e0), y = 78 + 42 row (0x422100). The cells' objects (0xb8 bytes each, constructor 0x41ad90,
  which gets a random spin of -5 to 5 in its field +0x18) are at board+0x114 + (row * 6 + column) * 4. Around the grid
  are the lines of edge objects: board+0x1ec (6, above the first row, the tiles that wait to fall in arcade mode), board+0x208
  (9, the left, column -1: where the fuses start) and board+0x22c (9, the right, column 6: the rocket launchers);
  board+0x204 is one more object (the spare tile).
- The Board's constructor (0x41ae80) makes a random generator (0x9c4 bytes, the framework's MTRand-like class), the
  pieces' generator `0x42a510` (0x74 bytes: tile distribution tables at 0x4e6800 and 0x4e6828, filled by 0x42a460 from the
  difficulty and level), a 0x14-byte fireworks manager (0x41a830) and an 0x18-byte object (0x413aa0), then runs the two
  setup functions 0x422120 (state reset per mode) and 0x422620 (the first board: fills the top and bottom rows and the
  left column, for strategy and classic the rows made by 0x428780). With the hints on (app+0x549, the Options hints
  checkbox, is clear) the opening board is the same every time: the arcade mode's first top line gets the kinds 0, 0, 2, 0, 2, 0
  turned 1, 1, 1, 1, 2, 1; the classic mode's rows 5 and 6 and the strategy mode's rows 4 to 6 are written over the generated
  tiles, in the loop of 0x428780 before each check of 0x428ae0, from fixed tables of the kind and the turn of each tile.
- Draw (0x41f540, 9306 bytes) clips to x 176..640, shakes the whole screen with a random offset while board+0x4b4 > 0
  (the "earthquake"), draws the night sky (image 67, with the day sky 68 fading in over it as the dawn comes) at (63, 0), the sun (71) rising,
  the city (72) at (116, 317) and the ground (66) at (116, 382), the fireworks, the column picture (65) at (0, 0), the
  talking head, in strategy mode the matches (112) at (0, 166), the poles (69, 70) at (195, 24) and (492, 24), then the
  pieces by columns, the edge rows and the lists at board+0x278 (0x41d420 each). State is at board+0x5c (0 playing, 1, 4,
  5, 6, 7 are other screens such as the level-up and the end, 2 over).

### Board translation plan and progress (2026-10-26)

Board.h holds the field layout read from the saved-game writer 0x41c830 (the list of saved fields, in file order) and the
Piece object (0x41ad90 constructor, 0x41c5a0 writer, 0x41b430 reader). Fields keep their offset in the name until the code
using them has been read. Translate in address order, one group at a time, each a compiled step:

1. Piece, list helpers, Save/Load (one field table visits both): 0x41b2a0 destructor, 0x41b430 to 0x41b7e0, 0x41b880 load
   (3350 bytes), 0x41c830 save (2890), 0x41c5a0, 0x41d380 to 0x41d3a0.
2. Setup: 0x422120 (state reset per mode), 0x422390, 0x422460 (clear), 0x422620 (the first board), 0x422a90 to 0x4231a0,
   the generator class 0x42a510 (distribution tables 0x4e6800/0x4e6828, 0x42a460 fills 0x4f6cfc..0x4f6d0c from level and
   difficulty, 0x42a590 draws a kind 0 to 4, 0x42a770 per-difficulty values, 0x42a980 speed from 0x42a940).
3. Update 0x426f40 (2288 bytes) and its helpers 0x423270 (2143), 0x424730 (2687), 0x425490 (1157), 0x426860 (1587).
4. Input: 0x4285b0 (vf24) to 0x4283e0 (vf26/vf29), 0x427d00, 0x428780.
5. Draw 0x41f540 (9306 bytes) with the helpers 0x41d420, 0x41d620, 0x41d670 (3362), 0x41e3e0 to 0x41f360, 0x4219a0,
   0x421b80, 0x421de0, 0x429230. Draw's `__ftol` calls hide the x and y arguments: read the disassembly
   (`bash build/rm_dis.sh <start> <stop>`, llvm-objdump) where an argument matters.
6. Points 0x429ed0 (floating score text), the dialogs, high scores, the fireworks (Rocket 0x4172f0, manager 0x41a830) and
   the talking head (0x4161a0), which Board calls at many places (stubs first).

The pieces' pictures are the app's mPieces[kind 0..4][rotation 0..3][variant 0..6] (app+0x5f8; the original reads
`app+0x610 + (rotation + kind * 4) * 0x1c`, which is variant 6, the grey one, used for the edge pieces).

### Status of the Board translation (2026-10-26, commit "Board rules")

Translated and compiling (BoardLogic.cpp, BoardLogic2.cpp, Board.h; nothing runs them yet, StartBoard is still a stub):
the generator class 0x42a460-0x42adf0, Points 0x429ed0-0x42a2d0, ResetState 0x422120, AddRight 0x422390, ClearBoard 0x422460,
SetupBoard 0x422620, DropPiece/SettleAll 0x422a90-0x422bc0, NewTop 0x422c50, LevelEffect/Burst/BurstSome/ComboSound/BonusFor
0x422d30-0x4231a0, SmokePuff/DestroySpecial 0x423200-0x423270, MakeSpecial/MaybeSpecial/FindFreeTile/AttachBomb/AttachSpecial10
0x423b00-0x423e20, DropTop/DropTopTimed 0x424010-0x424180, AddText/AddScoreText/CountBurning/Ping 0x424370-0x424700,
UpdatePieces 0x424730, Flood 0x4251b0, StartBurn/Launch/AddScore/MakeFlame/MakeMarker/MakeRocket/FinishBurn/Ignite/
ResolveLaunch 0x425410-0x425e30, BothEnds/Trace/TraceUp/Connections 0x426030-0x426370, SmoothFraction/NightTimer/Timers
0x426580-0x426860, AllTiles 0x426eb0, Bounce/HitBombs 0x427830-0x427910, PieceAtPoint/LowestEmpty/MovePiece/Shock/RemovePiece
0x427ab0-0x427d00, AddMover/EraseMover/AddFlash 0x427f90-0x428100. BoardShell.cpp has the constructor and stubs for what is
not read yet (marked with their addresses).

Not yet read or translated: Update 0x426f40 (the order Connections, Timers, UpdatePieces and the effects is expected),
the input handlers 0x4282a0-0x428780 and 0x428ae0-0x429460 (incl. 0x428d10, 0x428de0, 0x428ef0, 0x429050/60/80), Draw 0x41f540
with its helpers, Save/Load 0x41b430-0x41c830, the fireworks (Rocket 0x4172f0), the talking head,
the game-to-menu return.

Hidden arguments the decompiler drops (found with tools/callargs.py): 0x4259c0 AddScore(int), 0x423120 ComboSound(double x,
double y). The values pushed at every AddScore call were checked against the disassembly (2026-10-26: a launch scores 2000, or
4000 while m4b0 (250 after each such score) is above 0; the bonus messages of 5 to 9 launches score 1000, 2500, 10000, 25000 and 100000; a torch 300;
a bonus tile BonusFor; a match 500; the coins of the buy-rockets screen their own points; each rocket of a launch 100 times
its number). The floating texts do not rise: the board's update calls Move(x, y - 1) with the text's own y, which nothing
lowers, so a text jumps once and stays (0x427451). m84 is 0 (DL at 0x42a77f). Guesses still to verify: DropTopTimed's use of m4c.

### Where the Board stands after the input work (2026-10-26, evening)

The Board runs end to end without a picture: StartBoard builds it, `tools/check.sh rocketmania` plays a game of each mode with
the autoplay hook (src/testhooks/Autoplay.cpp: random clicks and drags, a straight pipe forced into the lowest full row now
and then, a check of every grid cell) and sees launches score (scores 130, 1604, 1630 at update 3000) and the strategy
mode end. Translated since the status above: Update 0x426f40, KeyChar/MouseMove/MouseLeave/MouseDown/MouseUp/MouseDrag,
pause 0x421f30, the first board of the timed modes 0x428780/0x428ae0, the dealing of a new board (state 7) 0x428ef0 with
0x428e80 (never entered: taken out again, see "Coverage check"), game over 0x421de0, 0x4282a0, 0x428090, 0x428d10, 0x428de0, 0x41f280, 0x429050-0x429080, the app's StartBoard
0x42ea30.

### Draw (2026-10-26, late)

BoardDraw.cpp translates Draw 0x41f540 and its helpers as far as the board's own picture goes: the sky, sun, city, ground,
column, poles, the tiles in their seven variants (0x41e770; the right-connected case with m41a set is variant 3, the
decompile's `(rotation + 0x37 + kind * 4) * 0x1c` is the address of that variant, not another one), the tiles above the board
(0x41e3e0, 0x41e530), the fuses and rocket launchers (0x41ec20, with the rocket picture chosen by 0x41d620), the pieces that
move (0x41d420), the score digit by digit, the level and mode texts, the chest and the coins under it (0x41f240, 0x421b80),
the count of rockets or matches left (0x41ef20, 0x41f0b0), the highlight of the tile under the pointer, the hint arrow
(shown until the first tile is turned; the option at app+0x549 turns it off), the banners (0x4219a0 draws a text four times
with phases 0, 5, 10 and 15 so that the copies trail each other) and the texts "Get Ready", game over, level up, "just in
time", combo, unfreezing and low on matches. Every text is named by its address (GT), none is in the source. The offsets of
the dark copies behind "just in time" are a reading of the disassembly with one y value inferred from the other texts'
pattern; compare with the original's picture when the test site can show both.

The dimming and the pause window (m350 > 0, the dialog picture 11 over a 345 by 170 box) are in. The original's Draw returns
without drawing once the dimming is complete (m358 == 1.0) and relies on its back buffer keeping the last frame; this port's
screen showed black, so the frame is drawn once more at that point (a port difference, noted in BoardDraw.cpp). StartBoard now
removes and deletes the menu (0x42e190) as the original does, so nothing of it shows under the board.

The specials pass 0x41d670 is in too (kinds 0 and 1 the bomb that pulses and grows, 2 and 3 the bonuses that bounce in, 4 the
flame, 5 a rocket, 6 and 7 the coins that fall into the chest with their sparkle, 8 a fading picture, 10 and 11 the torch
whose frame shows its hits left, 12 and 13 the clock; the images are the table's entries 30, 85-89, 48, 45-47 and 84, 21,
91 and 92). A special that rides on a tile waiting above the board is drawn down with that tile (SpecialDy). The effects
the pieces point at (mFlash) are moved and removed as the original does, and are drawn with the others (see the effects below).

Not translated yet, in Draw: the number of a launch (kind 9 of the specials pass, and 0x429230/0x4290b0), the tilted tile 0x41e630 and the overlay 0x41e690, the ghost trail of a moved tile, the
frozen-time bar 0x41f360 of classic mode, and the head and its bubble
(0x416d50, 0x414000).

The three buttons on the left (app 0x42d1a0, called by StartBoard 0x42ea30 through 0x42d090): ButtonWidgets with ids 3, 1 and 2
(Pause at y 245, Options at 210, Quit at 440; x 10, 107 wide, 35 high, mouse insets (1, 1, 3, 3), hand cursor), whose
over and down pictures are the image table entries 49/50, 40/41 and 38/39; the normal picture is part of the column
picture. Each is added to the widget manager and to the app's list at app+0x87c. The ids are acted on in the app's handler 0x42ed10
(its tail, after the dialog switch): 1 opens the options dialog (0x42de20) when a game is in state 0, 2 is Quit (0x42e530:
in a game in progress a yes/no dialog with id 0 whose YES goes back to the menu, in a finished game the menu at once) and 3
is Pause (0x42d820: a one-button modal dialog with id 8 and the footer "PLAY!"). The Pause button does not pause the board
itself: a modal dialog does, through the app's ModalOpen/ModalClose (0x42f270/0x42f290, slots 57 and 58), which call the
board's pause counter 0x421f30 with true and false; the window's LostFocus and GotFocus (slots 60 and 59) call the same two.
Every dialog made with DoDialog comes from the app's own NewDialog (0x42d4e0: the dialog and button pictures, the
Verdana 12 bold and Showcard 22 fonts, the three dialog colours, insets of 36, the default place (206, 42, 348 wide)), and
its KillDialog (0x42d620) gives the board the keyboard focus again when the last dialog is gone. The port draws the board
dimmed and with the pause window under every modal dialog, as the original does. The options dialog is not translated yet,
so the Options button does nothing.

Music (the app's four songs: 0 menu, 1 game, 2 game over, 3 level up): StartBoard fades all four out at 0.005 a step, the
Board's constructor fades the game song in at 0.666, a pause fades it out at 0.008 without stopping it and the resume fades it
in again, and a game over fades the game song out at 0.02 and the game-over song in. The pause also disables the three
left-hand buttons (0x42d270) and the resume enables them.

Saved games (BoardSave.cpp, 2026-10-26): the writer 0x41c830 and the loader 0x41b880 share one order, so one visitor
(Board::Fields) lists the plain fields for both. The file is the 2003 framework's Buffer: the magic 0xFADA0002, the app's mode
and difficulty, the score, the level and the state, the random generator's state as a string (MTRand::Serialize, 2496 bytes),
the 54 grid slots, the 6 top, the spare, the 9 left and the 9 right (a flag byte, then the piece when it is set), the board's
plain fields, the generator's 25 counters, and the pieces that move about (a count, then the pieces). A piece is written
with its own address as its number and its special and host as the addresses of those pieces; the loader reads a piece
once per number (0x41b6f0, one list m504) and 0x41b690 turns the numbers into pieces. The port keeps the file layout, so
a save of the original loads (the random state apart, which the original wrote with its own 2003 generator). The file is
Players/savea.dat (classic), saveb.dat (arcade) and savec.dat (strategy) in the user folder. A game in progress is saved
when the player goes back to the menu (0x42e010) and when the app shuts down (0x42f730, the port's ShutdownHook) unless
its state is 2 (finished); a new game deletes the mode's save (0x42ea30, 0x41d380) and CONTINUE SAVED GAME (0x403380) makes
the board, loads the save into it and deletes the file. What a game saved in the middle of a level's end (mFc, m100, m104)
shows again after loading (AfterLoad, 0x41b7e0: the buy-rockets screen 0x428620, the question 0x4286b0 and the next level
0x428bc0). One quirk of the originals is kept: the loop that should save the three per-path arrays (+0x384, +0x3fc, +0x41a:
which edges a lit path touches) never moves its three addresses, so it writes, and reads, the same four bytes at +0x3fc and
the bytes at +0x41a and +0x438 thirty times; a loaded game has the arrays as a new board has them until the next pass of the
connection finder (0x426370) sets them afresh. Checked by saving a game, going to the menu and continuing it: the board,
score, level and difficulty come back pixel for pixel.

Options dialog (read 2026-10-26, not translated yet): OptionsDialog (ctor 0x411ba0, opened by 0x42de20 with a flag for
"from the menu") derives from MoneyDialog (0x4118d0: the dialog on a black fill of the window minus 8 pixels, Dialog::Draw on
top, a phase at +0xf0 that Update raises by 0.0005 and wraps at 1; the loop in its Draw that works out 40 points round the edge
is dead code, nothing is drawn with them). It is made with the app's dialog picture and the three dialog colours, 0x184 wide,
placed at x 0x7e and centred on y 240, and holds two sliders (music id 0 at (0x98, 0x35), sound effects id 1 at (0x98, 0x55),
each the window width minus 0xc0 wide and 0x24 high, their ends the slider pictures at images 0x30/0x34 of the app's table),
four checkboxes of 0x18 by 0x18 (fullscreen id 0 at (0x30, 0x7b), custom cursors id 1 at (0xb0, 0x7b), disabled hints id 2 at
(0x30, 0x94), hardware acceleration id 3 hidden behind the others), and DialogButtons from the dialog button picture: Register
id 1 (only when the game is not registered, so never in the port), Reset Hints id 0xe, Credits id 6, one id 7 button whose label
and meaning (0x42e770) are for the next reading, and one link button per "NumOptionsLinks" entry of the app's string table (ids
100 and up; their URLs are never opened in a test run). Its handlers: the slider (0x413040) sets the music or sound volume
and, for sound effects, plays sample 5 at most once in 50 updates; the checkbox listener (0x4130b0) keeps fullscreen ticked
when the app forces it, stores the hints checkbox in the app (the port's mNoHints) and for hardware acceleration shows two
message dialogs (id 0x10: "Not Supported" and "Warning"); the button handler (0x4134b0) sends Credits to 0x42d760, 2 to the
high scores 0x42dcc0 (flag 3 or 2), Reset Hints (id 0xe) clears the app's seen-hint flags (app+0x549 and the 8 bytes after it)
and re-ticks the hints box, and Done (dialog id 1) brings back the menu's music or head. Where the fullscreen and acceleration
choices are applied was not found in the dialog's own functions (its destructor 0x412600 only deletes the widgets).

The high scores (HighScoreDisplayDialog 0x4064e0, id 0xe) fill the window: the Hall of Fame picture, three titles, three lists of
ten and, under a list that has one, the best score of the day. Classic is on the left, arcade on the right, strategy in the middle and
lower. Every list starts with ten of 30 names of the makers and their friends, taken in turn from two lists in the exe (13 and 17
names, tables at 0x4e4658 and 0x4e4690), and the scores 10000 down in steps of 250. The lists live in Players/hiscores.dat: for
each mode the day's number (the year times 366 plus the day of the year), the best score's name and score and a check, then ten
entries of name, score and check, where the check is the score xor 0x42beef xor each character times 0x11; an entry whose check does
not match keeps its name and its score is spoiled (it becomes the score modulo 256), and the best of the day is dropped when it
was made on another day. The file is written as the dialog goes away (the port also writes it when a score is added, because a
browser tab that is closed never destroys anything). The link under the lists is "MAIN MENU" after a game or from the menu (an
unopened menu's head is put back), "BACK TO GAME" in a game and "OPTIONS MENU" from the options; it does nothing while the record
dialog (id 3) is up. The dialog looks at the score in its 10th update after a game (CheckScore 0x4073b0): a score that beats the best
of the day or makes the lists opens the record dialog (RecordScoreDialog 0x414060, a MoneyDialog with OK and CANCEL and a name field)
with a line that says which of the two it made and its place (1st, 2nd, 3rd, 4th ..., 11th to 13th end in th); the unregistered
game's question about registering is not ported. OK or the Return key hands the name to the app (ButtonDepress case 3), which adds
it to the lists (AddScore 0x4084b0, the new entry is drawn in a colour that moves) and remembers it as the default name for the
next time (LastScoreName in the registry). The test script command "gameover <mode> <score>" starts a game of the mode, gives it
the score and opens the high scores the way the end of a game does, which is how this was run until the game-over screens exist.
Who opens the high scores after a game is the buy-rockets screen's button 5 (0x4102c0) and the end-of-level screens (0x41b7e0); both
come with those screens.

The hint dialog (HintDialog 0x408f60, id 0xb) has two forms. The three mode hints (0 arcade, 1 strategy, 2 classic) are opened by
GameSelector::StartGame (0x405ed0) before a mode's first game and fill the window: a strip of four (classic: three) small animated
pictures on a green board, each fading in 100 updates after the one before it, under the mode's title and one line; "Click Here
To Play" starts the game that waited (StartBoard with the "continuing" flag the hint was given). The five bonus hints (3 gem, 4
bomb, 5 clock, 6 coin, 7 matches) are a smaller box with the item's picture pulsing; 7 is opened by the board when the first matches
special of a strategy game is attached (0x428d10), the others by the end-of-level question 0x4286b0, which comes with the level-up
screen. Closing a bonus hint other than 7 opens the buy-rockets screen (0x428620); Board::m104 is 2 while one is up, so that a
game saved then comes back at that screen. The checkbox "Don't show this again" sets the app's hint flag (HintsDisabledFlag0-7 in the
registry; the Options dialog's Reset Hints clears them). The test script command "hint <n>" opens hint n (a board is started first
for 3 to 7).

The level-up screen (LevelUpDialog 0x40c2c0, id 0xd) is the whole window over the board. Kind 4 ("Well Done!") comes when a level
is finished (Board::Update in state 5, which then goes to 6) and kind 5 ("Congratulations!") when a game is over (Board::Timers
when the countdown of state 1 ends, state 1 goes to 2). It shows the rank of the score (RankName 0x428ca0: 27 ranks by the table
at 0x4c7848, the medallion is frame rank of a strip with frames 200 pixels high, the name changes colour with the hue going up and
down) over a pot, a jar with a page of figures in it, and one link at the bottom (to the next question 0x4286b0 after a level,
to the high scores after a game). Every 150 updates the page turns (Step 0x40f5a0): the score, the points to the next rank
(a level's screen only; at the top rank it says so), bombs (level 5 on), the longest fuse, coins (not strategy), the largest launch,
gems (level 3 on), clocks (level 7 on), matches (strategy) and the level (not strategy); a level's screen shows the level's
figures (Board::mC4-mDc) and a game's the sums (mE0-mF8, which the dialog adds the level's into when it opens). The song fades
to the level-up song, and fireworks burst behind the pot now and then (BurstSome); the head's lines are not translated yet. When the window is not active the page goes dark with a pause notice and the link is taken away
until it is. Each of the rank-name strings is read from the table at 0x4e62e8 (make_gametext.py lists the pointer table). The
test script command "levelup <kind> <mode> <level> <score> [<page>]" opens it.

The buy-rockets screen (BuyRocketsWidget 0x401010) is a widget of the board's size that the board lays over itself after a level
(ShowBuyRockets 0x428620, which the board keeps in m4e0) and, in its game-over form, when a game ends (0x421de0). It runs only
while the board has the focus. Between two levels every five coins that were collected buy an upgrade of a launcher's meter
(PieceGenerator::Upgrades 0x42ae10: the launchers in the order of the table at 0x4e6990, the lowest meters first, never more than
one level above the rest): five coins fly one after the other, 50 updates apart, from the box of the coins to the launcher, each on a
path that is straight across and slows down going up or down, and the fifth raises the meter (the rocket then glows). When every
meter is full, and at the end of a game, the coins turn into points instead: up to 15 coins fly to the score, 14 updates apart,
and the first one adds 500 for each coin. With nothing to buy the screen shows the help for the coins (four lines) in the first
five levels when the hints are on, waits 250 updates, and goes on; in any other level it goes on at once. Going on is
Board::NextLevel (0x428bc0): the level goes up, the rank is kept in m88, the board is set up again and plays. The board's
m100 says what a saved game was in the middle of (1 the buy-rockets screen, 2 the game's end), and AfterLoad (0x41b7e0) puts
the screens up again. Board::RemovedFromManager (0x424260) deletes the widgets that the board owns. The test script command
"buyrockets <over> <level> <coins> <meter>" opens the screen.

The effects (Effects.h, Effects.cpp; the original class "Rocket", 0x128 bytes, made by the manager's Add 0x41a9c0) are particle
emitters of 24 kinds (0 to 0x17; the game makes 0 to 0x12). An effect holds a position, how far its particles spread from it,
the direction they fly (a unit vector made when it is built), their speed, drag, acceleration, life and how many it makes
every so many updates; EffectKinds.inc is what the original constructor (0x4172f0) stores for each kind, generated from its
disassembly. The board sets the rest after Add: where it is (mX, mY), how many updates it keeps making particles (mLife; zero
stops it, below zero never), the delay before it starts (mc0) and, for the sounds, m40. Each update (0x419340) the effect moves,
every particle moves by its speed and then keeps the drag's fraction of it plus the acceleration, a particle at the end of its
age goes back to the manager's pool (1000 particles, the same for all of its effects; an effect holds at most 200), and new
ones are made while the effect has life. The manager removes a finished effect only when the flag given to Add is set. A
particle's colour is a blend of a pair out of six colours that the effect holds (it takes the pair of 3 to 5 at random, the
kinds 5 and 0xf one of 0 to 2 for each particle). The effect is drawn (0x4194d0) from strips of pictures chosen by the
particle's age, with the add mode, in the layer given by m90: the explosion (0, 3, 0x14 to 0x16), the strip of nine (1, 0x11),
the leaf (2, the petals of the menu, whose first 60 updates fade in), the stars (4 to 6, 0xb to 0xe, 0x17; kind 6 turns every
other one), the rotating fireworks and the novas (7 to 10) and the sparkles (0xf, 0x10, 0x12, 0x13). The effects behind the board
(layer set) are drawn before the column, those in front after the grid; the level-up screen draws the layer that is set,
clipped to the window, and the buy-rockets screen the other one; the menu keeps one effect of kind 2 with a life of -1. An
effect of a kind that tells the board when it goes (the byte at +8; kinds 1, 7, 9, 0xa, 0xf, 0x10, 0x11, 0x12, 8) calls
Board::F428d10 when it is deleted (not when the manager is) and plays a firework sound when its delay is over (+9; the board puts the
sound in m40 and the delay in mc0). While the board is paused (m350) its effects stand still, as
the original's update shows. The test script command "effect <kind> <x> <y> <life> [<layer>]" adds one.

The talking head (TalkingHead.h, TalkingHead.cpp; the original class 0x4161a0, 0x110 bytes) is the dragon who announces the
big moments. The app makes it at start-up (CreateHead, 0x42d360) as a button widget in its manager and keeps it for the whole
run; it is never a mouse target. A head, a body and a shadow are drawn one over the other: the body at (sway, 20), the face
at (sway, 40 - sway) turned by a pendulum when that turn is at least 0.005 radians, where the sway (5 to 25) is a sine of the
speed. Two springs hold it: Place sets the target, the real position follows it with a damping, and the widget is moved to the
position every update (when it is nearly still it bobs a little on sine terms). A mood (SetMood, table 0x4c5270, 23 moods of
pairs of mouth and half-duration) is a list of mouths shown one after the other, with a random chance of 1 in 250 each update
of a new face; the first games speak through Say, which stops the old voice line and plays a new one from the sound manager.
The head also comes out of a hole at the board's left corner (the board draws the hole pictures, OUTOFHOLE01 + position / 100,
and the head is drawn at 80 percent size while it is in the hole): StartSlide climbs 12 a step (or goes down at -12) over 600
steps after a delay, and Surface brings it to the front. For the end of a level it flies in on a rocket: FlyIn gives it a
trail effect of the board's effects (kind 0, 300 updates) and puts it at the top, where it swings down on a sine path, and
Follow puts the trail's flames under it and clears the board's rows to that height; FlyOut sends it to the side. Dialogs and
the menu call HeadBesideMenu or HeadBesideDialog (StepAside, Place and BringToFront) to put it next to what is open. The
original's speech bubble (0x415c70) never gets a message in the shipped game (the function that would set one, 0x415d30, is empty), so
it stays invisible and the port has none (taken out, see "Coverage check"). The hover tips (HoverTip, 0x413aa0) watch the
pointer on the board: seven regions, and a pointer resting for 81 updates in one shows its tip (not in region 1 unless the
mode is classic, not in region 2 in strategy), which fades in over 16 updates (alpha `shown * 4`) and goes when the pointer
leaves. The tip's picture is the bubble (turned or flipped by region), its text eight lines centred in Arial 10 in the colour
(29, 96, 78), the lines being addresses in two tables (0x4e57b0 for arcade and classic, 0x4e5890 for strategy) that
`tools/make_headdata.py` turns into HeadData.inc, together with the numbers for the moods and places. The test command
"endlevel <n>" flies the head in for the end of a level.

The classic mode's night is shown as ten lanterns on a wire (DrawNightBar, 0x41f360; the check in Board::Draw 0x42f6f0 is only
"mode is 1"): with time frozen all show the frozen picture, otherwise the lit ones are those above the fraction of the night
that has gone, the lantern at that fraction fades from the dim to the unlit picture over a tenth of the night, and in the last
tenth the lit ones flicker. The first lantern to go out sets a flag the board's update turns into a puff (0x41f280). A tile
that was hit swings (the angle in the piece's +0x80) and is drawn turned in its plain picture (0x41e630); a tile in the top two
rows gets an additive warning picture over it (0x41e690), and a tile that was moved leaves one faded grey ghost for each cell
it passed. The points of the launch in progress show beside its row (0x4290b0, 0x429230) in a hue that moves with the length of
the chain, as five points for each flame times the difficulty's factor.

Still to do: the deliverables: the coordinator's `BYOPOP_BUY_rocketmania` line
(cmake/ByopopStores.cmake has an uncommitted local placeholder that must not be committed), and an AddressSanitizer run of the
autoplay (tools/build.sh has no asan option yet). The trial clock (0x42f770) and the shareware dialogs (0x414790, 0x415020, 0x4153b0) are left out on purpose: the Steam copy
is registered.

## Fix candidates

None. Checked for the fixes mod (`docs/MOD_ROADMAP.md`, "The fixes mod"):

- **Faults that change play.** None was found that the port keeps: the faults phase one met (a wrong rotation table entry, a launcher
  numbered by the column, the night timer running in the strategy mode, a click on a score text) were fixed in the port because the
  original's own code does not have them (`IDENTICAL_PLAY.md`, the table of changes).
- **Waits that only cost the player time.** The loading is the one (done in the port, below). The hint dialogs, the level-up and
  game-over screens wait for a click and the head's welcome and the fuse's animations do not hold a click, so there is nothing a
  player waits for without being able to act.
- **Run-to-run variance with the same input.** Play follows the board's generator, which is seeded from the game's seed and draws
  its numbers in the same order whatever the machine's speed. The only generator that follows how often the screen is drawn is
  the C library's (`CrtRand`), which the drawing alone reads (the board's shake, the lanterns' jitter, the sparks' frames). A
  fix that seeded it from the update count was tried and left out: 99 shots of a whole tour (a game of each mode) were the same with
  and without it, played with `-fast -drawevery` and in real time, so there is no difference for it to mend that the tools can show.
- **Phase two and the coverage check (2026-11-11).** Neither added a candidate: every difference the screen comparison found was the port's
  (the table of changes in `IDENTICAL_PLAY.md`) and is fixed there, the oddities it listed (the shake and the jitter, the particles) are
  random in both programs, and the coverage check found no code of the original with a fault the port should keep. There is no fixes
  mod for this game: no toggle to serve.

## Loading

The port loads in one go and shows the play link the update the game has loaded. The original's loading screen looks at the loading
thread's progress every fourth update (`LoadingScreen::Update`, 0x4111b0); for a registered copy the bar then follows the thread's
progress directly, so the original has no minimum time of its own. The port's loading is not faster than the original's (see
"Performance").

## Performance

**Processor time (the performance pass, 2026-11-12): the port is under half the original's, in every configuration, stretch and
pair.** Measured as `docs/COORDINATION.md`'s target asks, by cycle counts: `tools/profile/pairs.sh <out dir>` runs rounds of the
original and the port's three configurations, one game process at a time, each pinned to a performance core of its own
(`tools/pincore.py`), over `tools/profile/scenario.txt` (the port's bot plays one game of each mode, Arcade, Classic and Strategy,
difficulty Medium: 8,833 updates, 88 s at 100 updates a second). `tools/profile/cycles.py` reads the threads' cycle counts
(QueryThreadCycleTime, summed over the process) at marker updates only: the port is the shipped exe reading a `-script` (no
`-commands` channel, no state log, no bot) and prints a `pointer` line to SDL's log at each marker; the original's update counter
(0x4f6850) is read from outside. The original runs under origrun on rundesk's private desktop (its software screen: no GPU there),
windowed 640x480 on its real clock, which it paces by its own sleep as on a 60 Hz screen (no vertical-blank spin); `Players/save?.dat`
and `hiscores.dat` are deleted before each run, since a save left by the last run turns the first game's start into a Continue dialog
that swallowed the scripted clicks, and its registry values are reset. Sound: origrun's stand-in mixes nothing for the original, and
the port's mixer thread (SDL's audio thread, 0.2 to 0.4% of a core, at most 1.2%) is inside the port's numbers.

The port runs in three configurations, all on SDL's Direct3D 11 renderer (the display line each run logs): `port3d`, Hardware
Acceleration on, windowed at the game's own size (`ScreenMode=0`, `-windowscale=1`, "client 640x480 px, render scale 1"), the one
that is like for like with the original's window; `port2d`, the same with Hardware Acceleration off (`Is3D=0`); `portdef`, 3D on in the
window the port opens by default (twice the game's size, "client 1280x960 px, render scale 2"), what a player gets. Medians of cycles
an update in millions (share of one core at 100 updates a second), five originals and six runs of each configuration, the runs
interleaved round by round (the first round order turns by one place each round):

| Stretch (updates) | Original | port3d | port2d | portdef |
|---|---|---|---|---|
| 900 to 3200, Arcade play | 4.18 (19.9%) | 1.69 (8.2%) | 1.67 (9.4%) | 1.44 (8.6%) |
| 3200 to 3500, game over and the next start | 4.31 (20.5%) | 2.08 (10.1%) | 2.50 (13.4%) | 1.91 (10.6%) |
| 3500 to 5900, Classic play | 4.57 (21.7%) | 1.75 (8.8%) | 1.88 (11.2%) | 1.68 (9.7%) |
| 5900 to 6200, game over and the next start | 4.02 (19.1%) | 1.89 (9.5%) | 2.45 (13.2%) | 1.77 (10.2%) |
| 6200 to 8600, Strategy play | 4.10 (19.4%) | 1.82 (9.1%) | 1.85 (10.6%) | 1.85 (10.0%) |

The port over the original, a pair taken from one round (the median of the five pairs by stretch, in the table's order, and the worst
pair): `port3d` 0.41, 0.48, 0.38, 0.47, 0.41 (worst 0.63); `port2d` 0.39, 0.57, 0.46, 0.72, 0.48 (worst 0.78); `portdef` 0.33, 0.46, 0.41,
0.48, 0.43 (worst 0.72). The runs were made on a busy machine (4 to 5 other game processes at the start of a run, median; the machine's
load 0.36 to 1.0), so the spread is wide; the runs were interleaved so that a drift of the machine falls on each alike. A sixth round was needed because the third round's original lost its second game's
start (modes 0, 0, 0, 0, 2, 2 at the six markers: the scripted clicks land on the original's real clock, which varies from run to run);
`cycles.py summary` leaves out an original whose three games did not all start, so each ratio above is over the five valid pairs.

Where the port's cycles go (`-cyclesplit`, one 23 s run of the Arcade stretch in each of 3D and 2D, `tools/cyclesplit.py`): the game's
update is 0.027 M cycles an update in 3D (1.7% of the process) and 0.028 in 2D; the picture, the game's draw calls and the present on
the loop's thread plus the Direct3D 11 driver's own thread (3.4% of a core), is 1.42 M (92%) in 3D and 1.34 M in 2D; the audio thread
0.05 M and the loop's events, sleeps and readings 0.05 M. With a GPU renderer the draw calls are batched until the present, so the
draw and the present are not split further. The game's update is not where the time goes, and half of the picture is the driver's own
thread; with the target met at 0.25 to 0.78 of the original's in every pair, the pass made no change to the port.

**Web build** (the Browser pane, Chrome 156, the page's own timers, no `fast=1`, `tools/profile/webperf.js` as `rocketmania.html?player=1&assets=1&silent=1&driver=...`,
one run of 8 s a phase): the first frame takes 1.4 s (the load, in the loading screen's phase); then a frame costs 1.76 ms
in Arcade (9.2% of real time at 100 updates a second, 430 frames drawn of 821 updates, longest frame 49.5 ms), 1.65 ms in Classic
(8.7%, longest 20.7 ms) and 1.34 ms in Strategy (7.3%, longest 21.9 ms), with no page error. The wasm heap is 136 MB with 94 MB in
use (23 MB in 281 pictures, 17 MB in 65 sounds) and the in-use figure is the same, within 0.5 MB, after the first game and after
the third: no growth over three games. The hidden pane gets no animation frames, so the page's `requestAnimationFrame` was
replaced by a 16 ms timer; the frame hook's milliseconds are the game's own cost either way.

**Load time and memory** (measured 2026-11-08 with `games/rocketmania/tools/profile/perf.py`: the original under origrun and the
port under rundesk, without `-fast`, back to back, the same scripted 55 seconds, on a machine with seven other games running; its
processor-time row used GetProcessTimes and is replaced by the cycle counts above):

| | Original | Port |
|---|---|---|
| Launch to the play link (s) | 2.4, 1.1, 0.9 (the first run was cold) | 1.8, 2.0, 1.8 (3.5 and 2.8 on cold runs) |
| Peak working set (MB) | 85 | 138 |
| Peak private bytes (MB) | 62 | 132 |

The port loads more slowly than the original (its resource thread takes 1.2 s of that, in the log's "Resource Loading Time") and keeps
about 70 MB more in private memory (the decoded pictures as 32-bit surfaces, the mixer's buffers, SDL): the framework's fixed
cost, which the target allows.

## Registration code removed

The port plays as the registered original does (the Steam copy: `properties/partner.xml` says `NoReg`), and nothing that only an
unregistered or trial copy reaches is in it:

- the trial clock (0x42f770), the play counters and the shareware dialogs (0x414790, 0x415020, 0x4153b0), the "register to load" and
  "register to record your score" questions (the original's texts offer to register before a name is entered; the port goes to the
  name), the loading screen's second link ("register now") and its slower bar for an unregistered copy;
- the line under the loading bar that thanks the player for registering (`LoadingScreen::Draw`, was `GetBoolean("NoReg")` and the
  text at 0x4e5258): it was empty with the Steam copy's `NoReg`, now the branch and the string are gone, with no visible change;
- `mIsRegistered` is not set by the game any more (the framework's `SexyApp` sets it true, and no code of the game reads it);
  `DIALOG_REGISTER_TO_LOAD` and the check of it in `GameSelector::Draw` (the selector's hint text hides while a dialog is up;
  the register dialog never exists), and the register picture (`IMAGE_REGISTER`, "register", the later image numbers move down
  by one: the rocket pictures begin at image 53);
- the Options dialog's register button, update check, partner links and PopCap page button (and the "More Games" row);
- 33 entries of the game text table that only those used: the questions to register, the trial counters and their debug lines,
  "UNREGISTERED VERSION", the trial's messages, the register prompts of the loading screen and the registration dialogs, the
  already-registered thanks, the license-code error, `http://www.popcap.com` and the update check's address.

Looked at and kept: "buy rockets" is the game's own rocket shop (coins), not a purchase; no "registered to <name>" stamp or status
line exists in the game's texts (the strings were searched for register, trial, thank, license, order, www, http). Visible change:
none in the registered game; the Options dialog is shorter by the "More Games" row.

## What the screen comparison showed (phase two, 2026-11-11)

Found while lining the port's screens up with the original's (`IDENTICAL_PLAY.md`, "Phase two"); each is a trap for later work.

- **Image numbers after the removed "register" picture.** The port dropped the picture at the original's index 19, so every port
  index after it is one lower. A literal number copied from the original's code is a bug; use the `IMAGE_*` names. Four were
  found that way (the sidebar's launch icon, the gems of the board's draw, the multiplier and clock pictures, the buy-rockets
  rocket).
- **2D or 3D.** The original reads the registry value `Is3D` (the string at 0x4ecae0). The game's app constructor sets
  `mTest3D`, so the first start runs the framework's 3D tester and turns 3D on when the card passes. Inside Windows Sandbox (no
  GPU) the original is therefore a 2D (software) run, and its Options dialog's "Hardware Acceleration" box is unchecked. The port
  starts in 3D (GPU) by default. The screen comparisons use `RM_REG="Is3D=0,ScreenMode=0"` (`port.sh`, `best.sh`, `lag.sh` and
  `play.py --reg` take it): 2D, and windowed, since without a `ScreenMode` value the port's Options dialog shows its Fullscreen
  box checked. The 3D game was compared on the host's desktop on the night of 2026-11-12 (`tools/shots/run3d.sh`, `visrun3d.py`, `lag3d.sh <tour>`,
  `IDENTICAL_PLAY.md`, "3D screens"): the original there reads `Is3D` 1 from its registry key at update 2708, and the port runs with
  `RM_REG="Is3D=1,ScreenMode=0"`. The Options dialog's Hardware Acceleration box answers from the framework's `mCan3D` (`Is3DAccelerationSupported`),
  so it can be ticked again after it was unticked, as in the original; before 1821bccf it could not.
- **2D draws rotations in software.** With 3D off the original rotates tiles with `DDImage::BltRotated` (0x441127) and keeps the
  screen between frames; the port's `RocketApp::UpdateFrames` turns `mSoftwareTransforms` and `mKeepScreen` on in that case. The
  original's software stretch (0x44abeb, the talking head in its hole) is not ported: the framework's `StretchCopy` (0x433ca0)
  is sharper than it, and SDL's bilinear stretch lands within a few levels of the original (about 800 pixels of the head differ,
  none by more than 40 levels), so `mSoftwareStretch` stays off.
- **Dialogs that do not drag.** The difficulty, continue and hint dialogs do not move when dragged, and the high-score and
  level-up dialogs ignore a press and a drag too (the original overrides `Move` (slot 14) and `MouseDown` (slot 26) and, for the
  last two, `MouseDrag` (slot 29) with empty functions: 0x415c30, 0x403880, 0x410470). Options, credits and the record-score
  dialog drag as the framework's `Dialog` does, clamped to 8 pixels.
- **Board fields the original leaves as the heap had them.** The constructor (0x41ae80) sets `+0x444` (the arcade mode's drop
  delay) only for the arcade mode, so in the other modes it holds whatever the heap held (a 2 in the runs that were looked at),
  and nothing reads it there. The port's board starts as zeros. The constructor does set `+0x460`, the game-over kind, to 3,
  which the port now does too (nothing reads it before it is set, but the save file holds it).
- **The save files.** After the same play the two high-score files (`Players/hiscores.dat`) are the same 604 bytes. The strategy
  mode's `savec.dat` has the same size and differs in raw heap addresses (each piece's own number, and its special and host: the
  original writes the pointers, `Board::ReadPiece` turns them into numbers again), in the seconds-played fields (`+0x360`
  to `+0x368`: real clock) and in `+0x444`, above.
- **A leak of the port, fixed.** `RocketApp::ShowGameSelector` removed the board from the widget manager and set `mBoard` to
  NULL; the original (0x42e9f0) also deletes it. The board, its effect pool (80 KB), generator, tip and pieces stayed: about
  100 KB a game. Found by listing the live blocks after 10 and 40 game cycles by their call sites (a throw-away
  `operator new` that recorded the caller's return address, resolved against the linker's map file), then
  `driver/leakcheck.py` over games, the options, high-score and credits screens and a lost game (140 cycles): private bytes stay
  flat from the 80th cycle on (154.57 MB; the three modes alone drift by about 6 KB a cycle). The tracker is not in the tree; to repeat it, replace the global `operator new` and `delete` in a
  scratch build linked with `-static` (the shared libc++ has its own).
- **The shift search of `shotdiff` hides a whole-screen shift of up to 8 pixels.** A dialog that moved 8 pixels (the drag above)
  came out as "shifted" and its only box as a line of text. Read a pair's `shift` in `summary.json` (and `lagscan.py`'s line)
  before judging a box. `lagscan.py` also takes the alignment from the label's own frame now; it took it from the first frame of
  the window, which could be a different screen.
- **The original's loading takes more than 10000 updates in the sandbox** (a loading thread on the wall clock, updates on the
  virtual one), so a tour's first click belongs at update 40000 or later; the tours `t3` to `t7` were moved there. A first
  shot of the original that shows the loading screen means the click came too early.

## Coverage check (2026-11-11)

The whole check of `docs/COVERAGE.md`. origrun's `--coverage` recorded which of the original's functions ran, in 26 runs on the host
(`tools/coverage/run.sh`; the hits are in `build/coverage/rocketmania/hits/`, git-ignored). All ran on the virtual clock with no sound
device, with the host's registry set to `HintsDisabled=0,ScreenMode=0,Is3D=0` (2D, windowed, the hint dialogs on):

- the phase-two screen tours (`tools/shots/`) as host scripts: u2 (the whole flow), t1 and t3 to t7 (games of the three modes with all
  their dialogs), f1, h3 (three games in a row), o1 (the Options dialog), o2o (the credits), dd2 (the dialogs dragged) and hf1 (all eight
  hint flags set), each about 1300 to 1650 functions;
- eight bot games (bot1 to bot8), made by the port's bot as the identical-play cases are (`-uibot`, with `-uihold` so that the level-up
  and game-over screen is left to turn through all its pages): games of the three modes, played until they end by themselves or reach the
  bot's cap (bot6 was stopped after ten minutes: the original's counter ran behind the script's, so its inputs came out of step; the
  strategy games bot7 and bot8 never ended, see the last list);
- `tools/coverage/cov1.txt` to `cov4.txt` and `music1.txt`, written for what these missed: the high scores, the Options dialog's two
  sliders and boxes, Reset Hints and the credits (cov1); pause and resume, the space bar, the head's tips by hovering, the quit dialog
  answered no and then yes and the continue dialog in each mode (cov2); the menu's Quit button (cov3); clicks in the strip left of the
  board where no button sits (which hides the speech bubble) and the window losing and regaining the focus in a game (cov4); and a copy
  with its two `cached_*.wav` music files removed, so that the original joins the Ogg pieces itself (music1).

Every run but bot6 ended with the game exiting by itself. No run had the real clock and the silent sound stand-in (which the check asks
for once, for the sound code): the game's functions that start or stop a sound were reached on the virtual clock all the same, and none
of the unreached ones is one of them.

`games/rocketmania/coverage.txt` sets the game's own code as 0x401000 to 0x42f7e0: the game's classes and its statics, which the port
translates (the framework of 2003 from 0x42f880 on, the C runtime and the libraries are the platform's). It holds 480 functions, and the
runs reached 437 of them.

**Reached in the original with no counterpart in the port: 87 at first, 0 at the end.** The 87 split into 32 placed, 55 left out on purpose, and the one logic fix inside the placed ones.

- **Placed: 32 (comment edits only, play unchanged).** The destructors that the port writes without the original's address (the
  dialogs', the menu's, the head's, the speech bubble's, the board's, the app's, the loading screen's), the `RemovedFromManager` of
  two dialogs, the small helpers the port holds inline (the two counts at the left of the board, 0x41ef20 and 0x41f0b0; the vector
  normalising 0x416f50; the board buttons' 0x42d090; the end-of-game widget removals 0x42e9f0, 0x42e190, 0x42d660 and 0x4220a0; the
  loading screen's play link 0x410ae0; a mode button's 0x4061f0; the continue dialog's file read 0x403570; the pool destructor
  0x41ad00; the three of the music's joining 0x429880, 0x4299e0 and 0x429b30, which only music1 reached) and the two real constructors
  whose address was wrong or missing (the board's 0x41ae80, the app's destructor 0x42b3f0).
- **Left out on purpose: 74 `none` lines for 93 functions (55 of them reached), each with its reason.** The compiler's deleting
  destructors and the destructors that only call the base class's (20), overrides, thunks and the framework's empty defaults that only
  call the base class (11), the C++ library's `std::string`, `std::list` and `std::vector` as the compiler instantiated them (24),
  registration (26: the dialogs, their texts, the "already registered" and "invalid code" boxes, the high-score box that asks to
  register, and the registered flag's setter), and 12 others: one-line helpers (0x4172e0, 0x42d4d0 a log call that is a bare return,
  0x42f6f0 "the mode is 1", 0x00401000), the C runtime's start-up and `WinMain` (0x4293c0 to 0x4293f0), a compiler's exception
  handler, the update check (0x42e770) and the speech bubble's drawing (0x415ec0, 0x416040, 0x429260). 0x42d190, "is the game
  registered", is a registration check too and is always true in the port.
- **One piece of missing logic.** The menu's `OrderInManagerChanged` (0x405e40): when the menu is moved in the order of the widgets
  (`BringToBack` when it is shown), the original puts its six buttons directly in front of it, one after the other. The port had no
  such override, and had given 0x405e40 to its `RemovedFromManager` (the original's is 0x405de0, vf02; vf03 is the order hook). The
  buttons then stayed in front of the talking head and the speech bubble, which were added earlier; the original has them behind. The
  u2 tour's screens did not change, so no shot shows the difference, but the order of the widgets (what draws and what takes a click
  first) is the original's now. `tools/check.sh build rocketmania` passes with the baseline identical, and the u2 tour gives 47 match,
  45 faint and 7 differ (46, 46 and 7 before: the pairs with the column's jitter and the board's shake that phase two judged).

**A deviation the music's run showed.** With its cache removed the original joins the Ogg pieces of the menu and in-game songs and
writes `cached_*.wav`; both files came out byte for byte the ones the game ships with. The port joins them in memory
(`AppMusic.cpp`) and serves the result as that file; compared with a temporary dump (not kept), the two files have the same size and
header, and every sample is the same or one level off (the port's is one higher in half of the samples: the original's `ov_read`
truncates where the platform's decoder rounds, and the difference is below 0.02% of the songs' level). Not audible, and the platform's
decoder is shared by every game, so it is listed here and not chased.

**Defined in the port, never reached in the runs: 12 at first.**

- **Reached by the extra runs:** the first-click hide of the speech bubble (`HideBubble`, `FadeOut`: cov4; the bubble never has a
  message in the shipped game, so it did nothing; it is taken out, below), and the level-up pages the bots held open.
- **Read again against the export:** `Points::SetBack` (0x42a230) is reached only by five or more rockets launched together; it
  matches the export (the image, then a colour of three values, then the place is set again).
- **Taken out as unneeded code (the unneeded-code pass, 2026-11-12):** nothing in the original reaches them, so play is unchanged
  (the frozen baseline's state logs are the same):
  - the board's state 7 (a new board dealt row by row: `F428ef0` 0x428ef0, `EdgeOpen` 0x428e80, the sweep effect's pointer, the state's
    place in the update and the draw): the original has no store of 7 to the state, and the saves keep no 7 (the original never saves
    one; a hand-edited save with 7 would no longer deal a board). Its two saved fields (+0x348, +0x34c) stay for the save file's format.
    State 6, which the first notes listed with it, is entered (state 5 sets it when the level-up dialog is shown) and stays;
  - `Board::ButtonDepress` (0x428600) and the board's `ButtonListener` base: no button has the board as its listener;
  - the speech bubble's whole class (0x415c70, `Update` 0x415d90, `FadeOut` 0x415d40, the destructor 0x415d20), the head's
    `ShowBubble` (0x416a30) and `HideBubble` (0x416a80), the board's calls to them (the hint countdowns' case 0 to 4 and the click at the
    lower left) and the app's pointer: the one function that sets a message (0x415d30) is an empty stub in the shipped game, so the
    bubble's message stays at "none", it is never visible, never takes a click and is never drawn. What the calls did to the order of
    the widgets was the bubble's own place only (`BringToFront`, which reorders one widget and marks nothing dirty), so the other
    widgets' order, updates and drawing are the same. The board's `m439` (a right click, read only to decide whether to show the
    bubble) stays for the save file. The tips' pictures, `mBubbleFlipH` and `mBubbleFlipV`, are the hover tips' and stay.
  Proved by `tools/check.sh rocketmania` (the three bot games' state logs equal the frozen baseline's) and by the tours u2, t1, t3,
  t5 and hf1 (`tools/shots/port.sh`, the hint dialogs on, 2D): 307 shots, every one pixel-identical to the frozen baseline's (the
  PNG files differ in their bytes, the pictures do not).
  `coverage.txt` lists each as `none` with its reason, and the coverage report's "Reached with no definition in the port" and
  "Never reached and not defined" lists are empty again; what is left under "Defined in the port, never reached" are the two read
  above (0x40ee40 and 0x42a230).
- **Level-up page 8, the matches (`LevelUpDialog::DrawMatchesPage`, 0x40ee40, strategy mode only): no run reached it, read against the
  export.** The page shows only in the strategy mode, and the strategy bots never reached a level-up screen (no page's drawing ran): bot7 (easy,
  cap 80000) and bot8 (hard, cap 300000) played to the bot's cap. The port's state log of the same bot with a cap of 60000 shows the game in
  state 0 and the level 0 all the way, at 3.5 million points, so no level ended and the matches never ran out: a strategy game's own end is the matches at 0 with nothing burning for 250 updates
  (`Board::Update`, `F421de0` 0x421de0 kind 2), and the bot's paths earn matches back (the match tiles) as fast as they spend them. A bot that plays
  badly on purpose would be new test code for one 1079-byte page, so the page was read again against the export instead: the same
  font and the title colour (25, 48, 79), "Matches" centred at x 97 at y 220 and "Collected" at y 245 (centred the same way), the light
  colour (191, 241, 241), then "1 match" or "%d matches" (0x4e505c, 0x4e5050) at y 275, the value the level's (board +0xdc) when the
  dialog is a level's and the game's (+0xf8) when it is a game's, the text lengths 7, 7 and 10 of the three strings as the game's text
  table has them. It is the same pattern as the coins page (0x40f280), which the runs did reach.

**Cost.** The runs took about an hour of machine time in all, one process at a time on the stage (the bots' long games 5 to 10 minutes,
the others under two); the whole check took the session about two hours, most of it classifying the first list and waiting for runs.
