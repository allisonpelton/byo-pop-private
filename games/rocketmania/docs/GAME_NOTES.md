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
| Rocket (fireworks) | 0x4172f0, manager 0x41a830 | the decorative rockets over the menu and board (not ported) |
| TalkingHead, SpeechBubble | 0x4161a0, 0x415c70 | the guide who speaks in the menu and in the first games (not ported) |
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
- Not ported yet (marked in the code): the rockets over the menu, the talking head, the first-time mode hints, the Board.
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
  left column, for strategy and classic the rows made by 0x428780).
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
with its helpers, Save/Load 0x41b430-0x41c830, the fireworks (Rocket 0x4172f0), the talking head, the dialogs 0x4064e0
0x411ba0 0x414060 0x40c2c0 0x4118d0 0x401010 0x403990 0x408f60, high scores, the game-to-menu return.

Hidden arguments the decompiler drops (found with tools/callargs.py): 0x4259c0 AddScore(int), 0x423120 ComboSound(double x,
double y). Guesses still to verify against the original at the first run: PlaySample ids against Res.h's SOUND_ enum, the
float-up of Points, m84, DropTopTimed's use of m4c.

### Where the Board stands after the input work (2026-10-26, evening)

The Board runs end to end without a picture: StartBoard builds it, `tools/check.sh rocketmania` plays a game of each mode with
the autoplay hook (src/testhooks/Autoplay.cpp: random clicks and drags, a straight pipe forced into the lowest full row now
and then, a check of every grid cell) and sees launches score (scores 130, 1604, 1630 at update 3000) and the strategy
mode end. Translated since the status above: Update 0x426f40, KeyChar/MouseMove/MouseLeave/MouseDown/MouseUp/MouseDrag,
pause 0x421f30, the first board of the timed modes 0x428780/0x428ae0, the dealing of a new board (state 7) 0x428ef0 with
0x428e80, game over 0x421de0, 0x4282a0, 0x428090, 0x428d10, 0x428de0, 0x41f280, 0x429050-0x429080, the app's StartBoard
0x42ea30 (without the music fades, the three left-hand buttons 0x42d1a0 and the deletion of the mode's saved game).

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

Not translated yet, in Draw: the specials pass 0x41d670 (3362 bytes: a switch over the special's kind 0 to 13 that draws
the chest, bombs, coins and the rockets in flight with stretched drawings and attaches sparks to them; the conversion
operands are in the disassembly, `callargs.py 41d670 41e3a0 -n 3` lists them), the effects 0x41abb0 (called twice, behind
and in front), the tilted tile 0x41e630 and the overlay 0x41e690, the ghost trail of a moved tile, the dimmed screen and the
pause window (m350 > 0), the frozen-time bar 0x41f360 of classic mode, the launch counter 0x429230/0x4290b0, the head and its
bubble (0x416d50, 0x414000). Until the specials pass is in, a rocket in flight and the bombs are not visible, although the
rules run.

The three buttons on the left (app 0x42d1a0, called by StartBoard 0x42ea30 through 0x42d090): ButtonWidgets with ids 3, 1 and 2
(Pause at y 245, Options at 210, Quit at 440; x 10, 107 wide, 35 high, mouse insets (1, 1, 3, 3), hand cursor), whose
over and down pictures are the image table entries 49/50, 40/41 and 38/39; the normal picture is part of the column
picture. Each is added to the widget manager and to the app's list at app+0x87c. The Board's listener (the sub-object at
Board+0x58, slot 1, 0x428600) only plays sample 0. Where the three ids are acted on is not found yet: the app's handler
0x42ed10 does nothing for ids below 2000 in the decompile, and the Board's MouseDown only resumes a paused game, so the
Pause toggle (0x421f30, whose callers are 0x41c830, 0x427a90, 0x4283e0 and 0x42f270) must be reached from another place:
check the disassembly of 0x42ed10 and of the widget manager's button path before writing them.

Still to do after Draw (above): Save/Load 0x41b430-0x41c830, the dialogs (high scores 0x4064e0, options 0x411ba0, record 0x414060,
level-up 0x40c2c0, money 0x4118d0, buy rockets 0x401010, credits 0x403990, hint 0x408f60), the fireworks (Rocket 0x4172f0), the
talking head (0x4161a0, 0x413aa0), the left-hand buttons 0x42d1a0 and the Board's ButtonDepress, the music fades, then the
deliverables: the LICENSES entry, the coordinator's `BYOPOP_BUY_rocketmania` line (cmake/ByopopStores.cmake has an
uncommitted local placeholder that must not be committed), and an AddressSanitizer run of the autoplay (tools/build.sh has no
asan option yet).
