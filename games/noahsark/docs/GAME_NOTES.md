# Noah's Ark Deluxe: behaviour notes (work in progress)

Addresses are virtual addresses in `re/noahsark/bin/WinNoah.exe` (version 1.1, linked 2002-10-17 with VC++ 6.0,
image base 0x400000, full RTTI, 64 classes, 2,709 functions of which 260 (0x401000-0x421dc0, 131 KB) are game
code). Offsets such as `app+0x3e0` are byte offsets in the original object; the port's classes name the fields.

## 1. Binary layout

- Game code 0x401000-0x421dc0: `Board` 0x401000-0x410bf0, DifficultyDialog (0x410c00 ctor), HighScoreDialog,
  HighScoreWidget, IntroWidget, NextLevelWidget, NoahApp, OptionsDialog (ctor 0x41c230), RecordScoreDialog,
  RegisterDialog, TitleScreen, TutorialDialog. Framework from 0x421dc0 (Widget), libraries after 0x450000.
- The framework is the SexyApp of the 2002 generation: one app class `SexyApp` (54 vtable slots, derived from
  `ButtonListener` with 4 slots), `Widget` without WidgetContainer (44 slots, sizeof 0x4c), Graphics with
  non-virtual methods, std::string of MSVC 6 (copy on write, refcount in the byte before the data).
  docs/FRAMEWORK_2004.md describes the 2004 successor; FRAMEWORK_2002.md records the differences found here.
- Data: `images/` (GIF/JPG/PNG, alpha in `_name.gif`), `data/*.txt` (ImageFont descriptors), `sounds/*.ogg` +
  `levelup.au`, `music/*.ogg`, `fmod.dll` (the original plays music through FMOD; the port does not).

## 2. Resource tables (Res.h)

83 images in the original's table 0x494f40 (the port's has 82: the registration picture, id 47, is left out, section 9), 34 sounds
(0x49508c), 9 font descriptors (the port loads 8: the register font is left out), 4 music tracks (loading, mainloop, mainintro,
gameover). Fonts (app+0x3e4..0x404): fontbig, fontmedium, fontsmall, colorlessfont, garamondfontmed, garamondfontsmall,
garamondfonttiny, scorefont, registerfont.

## 3. Application (NoahApp, vtable 0x4888ec, 54 slots)

The app (`NoahApp.h/.cpp`) owns the images (82; the original 83) and fonts (8; the original 9), the board and the screens (title, intro menu, level-up
panel, high-score screen), and the game's counters: level, difficulty (0 easy, 4 medium, 8 hard), game mode, bonus round,
score and shown score, animals saved, combo count and best combo, the hint and tutorial settings. The sidebar's Options
button is at (5,390) and Quit at (5,431). Dialog buttons arrive at `ButtonDepress` with the dialog's id plus 2000 (see
FRAMEWORK_2002.md): difficulty (ids 0x3c, 0x3d, 0x3e for easy, medium, hard), options, the tutorial, the high-score
list and the score-entry dialog. Ranks: 24 thresholds up to 999,999,999 and 23 rank names, shown on the level-up panel
("Your Rank", "Next Rank at"). The music is the menu loop, then the game's intro and its loop, then the game-over piece;
`NoahApp::UpdateFrames` starts the loop when the intro ends.

## 4. Board (WinNoah.exe 0x401000-0x410bf0; object 0x1108 bytes, vtable 0x488368, ButtonListener at +0x4c)

Work notes while the translation is written (see Board.h for the final field names).

- A 9x9 grid of `Piece*` at +0x5c, index `x*9 + y` (x column, y row, y 0 = top). `FUN_0040e280(x)` is the cell's left
  pixel, `FUN_0040e2a0(y)` its top; a cell is 50x50 (0x32).
- Piece (0x68 bytes): +0 x, +4 y, +8/+c the cell it moves to, +0x10/+0x14 float pixel position, +0x18 type
  (0..12 animals, 0xd, 0xe empty, 0xf crate, 0x10 block), +0x1c alpha, +0x20/+0x24 pixel offset to the cell (a new piece
  starts at y offset -0x1db and falls), +0x28 fall state, +0x2c selected, +0x2d removed (animating out), +0x2e,
  +0x2f flash, +0x30 moving along a path (+0x3c its partner, +0x34 index into its path vector +0x48), +0x38,
  +0x59 match mark (puzzle), +0x5c blink frame.
- Second grid of 81 clones at +0x1c4 (FUN_004060d0 copies a piece): the solvability simulation of the level
  generator (FUN_00401e40) works on it. A row of 9 queued pieces at +0x1a0.
- Pairing rule: two pieces of the same type can be removed when a path of at most N straight segments joins them
  through empty cells (FUN_004036f0, depth-first with a cost bound; FUN_00403370 keeps the cheapest path in +0xe08).
  FUN_004026d0 / FUN_00402a70 are plain breadth-first reachability on the real / cloned grid (4-neighbour).
- State (+0x310): 0 get ready, 1 playing, 2 won (Noah climbs aboard), 3 lost, 4 level done (the ark leaves), 5 no more
  moves, 6 pair abandoned, 7/8 puzzle swap and swap back, 9 settle (puzzle refill).
- App game mode (+0x444): 1 action, 3 strategy, 4 puzzle, 5 stampede bonus.
- A pair is removed by a path of empty cells (`Reachable`, breadth-first; `SearchPath`, depth-first with a cost bound,
  gives the trail that is drawn). The pieces walk along the trail toward each other and are taken in within 5 pixels;
  removed pieces then fly to the ark (speed 5.3). Combos lower the water. Strategy mode has gravity and pushes a new row
  in (`PushRow`); puzzle mode is match-3 (`MarkRuns`, `RefillPuzzle`); the stampede bonus removes groups of three or
  more while the columns close in on the middle.
- Drawing Noah (`Board::Draw`): two flags choose his picture, the board's `mNoahWorried` (+0x35c) and `mArkLeaving`
  (+0xe15). Normal play uses his idle sprites (first image 62 plus a frame); once worried, his climbing frames
  (images 48 to 57); after a win or while the ark leaves, only the last climbing frame, and only until the ark's ramp stage
  moves on.

## 5. The standard test commands (`src/testhooks/TestCommands.cpp`, `Bot.cpp`)

The commands of `docs/TEST_COMMANDS.md`, reached only from a test (`-script`, `-commands`, the web test page's `command`). A command
that is never given changes nothing and draws no random number. Each answers `STATE {...}`, `MOVE {...}`, `OK <command>` or
`ERROR <command>: <reason>`.

| Command | Does |
|---|---|
| `state` | one JSON line: `update`, `screen` (loading, title, menu, newgame, difficulty, tutorial, modeintro, game, nextlevel, entername, highscores, gameover, quit, options, scores, dialog), `mode` (action, strategy, puzzle, stampede), `difficulty`, `level`, `score`, `focus` (the window has the focus: without it the board stops and draws PAUSED), `windowed` (false in full screen), `over`; with a board `phase` (getready, playing, won, lost, leveldone, nomoves, abandon, swap, swapback, settle), `busy` (something moves or is being removed), `water` and `waterTarget` (the water's height and where it is going), `ark`, `animals` (left on the board), `paused` (a dialog has paused the board) and `board` (9 strings, one per row: a digit or letter for an animal, `.` empty, `X` block, `Y` crate) |
| `goto menu\|title\|options\|highscores` | the screen, by the game's own link or dialog function |
| `goto <mode> [easy\|medium\|hard]` | a game of that mode from the menu: the menu's own button, the answer to the saved-game question, the difficulty dialog's button |
| `level <n>` | level n of the game in progress, as the level-up panel's link starts a level |
| `unlock all` | nothing in this game is locked (answers `OK`) |
| `set score <n>`, `set level <n>` | the score and the level counter (for the next level) |
| `set mode <mode>`, `set difficulty <d>` | the game the bot starts from the menu; `move` has no arguments here, since under `-realinput` `move <x> <y>` is the framework's pointer move |
| `seed <n>` | the C library generator (every draw of the game) and the bot's own |
| `win`, `lose` | ends the round of an action or strategy game that is being played: every animal off the board (`win`), or the water at the ark (`lose`) |
| `move` | the next input toward playing a game of the mode set, from the bot of `Bot.cpp` (the one `-autoplay` clicks): `MOVE {"kind", "input": [...], "updates": n}`, `input` in the order of `docs/TEST_COMMANDS.md` (`{"move":[x,y]}`, `{"click":[x,y]}`, `{"key":"B"}`), game coordinates; `"kind":"wait"` (with an empty `input`) when the game needs time, `MOVE null` when there is nothing to do |

`move` answers each screen: the title's play link, the menu button of the mode set (`MOVE null` if none is set), the saved-game
question (Yes: a new game), the difficulty button, the tutorial and the how-to-play dialogs' Yes button, the level-up panel's link, the high-score
name entry (the keys B, O, T and the Yes button) and the list's menu link. On the board it waits while a round is getting ready or
anything moves, and then clicks the bot's move: a pair of animals that can reach each other (action, strategy), a swap that makes a
run of three (puzzle), or a group of three or more in the stampede round. The bot is the autoplay's, with its linear generator and
its seed (`seed <n>`), so a `-seed` run plays the same game by `move` as by `-autoplay`. `over` is true on the high-score list after a
game.

## 6. Fix candidates

What the original does that the fixes mod (`mods/fixes`, below) mends, and what it leaves alone.

- **Made: the clock of the level's start and the level-up panel.** The original lets the animals in 400 ms of the machine's clock
  after a level starts (`Board::Update`, the release of the waiting animals) and counts the seconds on the level-up panel
  (`NextLevelWidget`) by `GetTickCount`, so a slow moment or a pause moves the animals' entrance and costs seconds, and two
  runs of the same seed part by a frame or two. `steadyClock` counts the
  game's own updates instead (40 updates, 100 to the second), which is the same time on a machine that keeps up.
- **Made: the water after an early click.** The first click of a level ends its get-ready. The water rises by steps of 3 to its level
  and ends a pixel above it (417 for 416 on level 2); the get-ready's own end, when no click comes, snaps it to the level, but a click
  leaves it. `snapWater` snaps it at the click too (strategy has no rising water and is left alone).
- **Made: the game-over screen.** The high-score screen fades in over 255 updates (one step of 1 an update) and checks the rank, and
  so asks for the name, when the fade reaches 175: a wait of 1.75 s on a game that is over. `quickScores` runs the fade three times
  faster (a step of 3).

## 7. The fixes mod (`mods/fixes`)

`mod.json` with the feature `fixes` and one bool setting for each fix (`steadyClock`, `snapWater`, `quickScores`), all on by default.
The game reads them where it needs them (`ModFixOn` in `ModFeatures.cpp`); without the mod every fix is off and the game is the
original. The site ships the mod beside the page and starts the game with it on (`cmake/ByopopWeb.cmake`); natively it is a folder
in `mods-noahsark`, or in a `-modsdir`. It does not split the high scores (a feature with no rules or tunables). Checked by
`tools/run_mod_tests.sh`: the bot's games with the mod on give the lines of the unmodded game; the water is one pixel nearer its level
after a click with `snapWater` and not without it; the game-over screen asks for the name at update 1525 with `quickScores` and at
1700 without it.

## 8. The drivers that play by `move`

`tools/realplay/drive.py` plays a game natively: it starts the exe under rundesk with `-commands` and `-realinput`, asks `move` for
each step, and sends it as real SDL events (`move game X Y`, `click game X Y`, `type B`); it reads `state` to know where the game is
and when it is over (`--mode action|strategy|puzzle`, `--diff`, `--fast`, `--steps`, `--at N:<lines>` for focus and full screen in
the middle of a game, `--finish`). On the web the test page's `byopop.test.playByMove` plays by the same `move` as DOM events
(`?player=1&silent=1&driver=move`). The game runs in real time for the native runs: with `-fast` it runs ahead of the driver, and the
water, which rises with the game's updates, ended a first action game after 18 moves.

## 9. Registration and the buttons that lead out of the game (taken out 2026-11-08)

The port was the registered game already (the shareware branches were never translated); what remained of the registration, and the
buttons that open a page outside the game, is gone, and each removal leaves the baseline's logs and screenshots alone except where it
is said.

- The Options dialog's Register button (id 1, the registration message) and Web button (id 3, which opened popcap.com in the
  browser). The Help and High Scores buttons now share the first row and Credits takes the second; the dialog is a row shorter (it
  was 438 high, it is 396). This is the one visible change; the suite has no Options photograph.
- `ShowRegisteredDialog`, the registered flag and the play counter (`mRegistered`, `mTimesPlayed`), the register dialog's id and the
  other dialog ids nothing uses, the register picture (`IMAGE_REGISTER`, so every later image id moves down by one: Noah's sprites are
  named by the enum now, and a saved game keeps the original's numbering) and the register font with its file entry, and four texts
  of the table (the register, the order and the web page messages).
- The drawn "popcap.com" under the title (`IMAGE_POPCAPURL`) is a picture, not a button, and stays.
