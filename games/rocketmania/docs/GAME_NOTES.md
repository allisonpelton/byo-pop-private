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
