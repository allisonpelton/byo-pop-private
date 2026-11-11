# Bejeweled 2 Deluxe: identical play, phase one (plays identical)

Phase one asks that, given the same input, the port and the original play the same: the same boards and random draws in the same
order, the same moves, scores, levels, timers and endings. This report covers that phase. It was started 2026-11-08 and done
2026-11-11, with one difference left as an oddity (a game resumed in the 3D game, below); phase two (looks identical: screens,
sound, focus, the web build) has not begun.

## What was compared, and how

The port plays a mode by its own bot with real input events (`games/bejeweled2/tools/origcmp/record.py`: the driver sends the bot's
moves as mouse clicks, and the mode's start, the dialogs and the name entry as clicks and keys) with the generator pinned to a seed,
held and uncapped, and writes down each input with the update at which it ran. `to_origrun.py` turns that list into an origrun script;
`run_original.sh` runs the original on origrun's virtual clock with the same seed (a seed patch in the copy), a scripted cursor and
four `--watch` values read from the board in the original's memory: the score, the level, the board's state and the position of the
board's own random generator. The port's `-trace` prints the same four values, as origrun prints them, and `compare.py` lines the two
lists of changes up by value and by update. The global generator's position (`0x67d910`) was watched too whenever a difference needed
looking at. A run is `IDENTICAL` when every change of the four values is the same and falls on the same update on both sides (the
original's labels in the 3D game fall one update after the port's, which `compare.py` reports as an offset of its own).

Two settings of the original exist. In the 3D game (1024 by 768, what the web build plays) the original runs under dgVoodoo2, since it
needs a 3D device; in the 2D game (640 by 480, what the original does without 3D) the port runs true 2D with `HighResolution 0` and
the software renderer. The secret modes start from a player made by `make_profile.py` with every mode unlocked, on both sides. The
bot is the port's own (`move` of the test commands), so each run's moves are those a bot makes on the port's boards; the original is
given the same clicks at the same updates and has to make the same moves out of them.

The web build was checked under Node (`origcmp/web_check.sh`, which runs `games/bejeweled/tools/verify/wasmrun.js game`): the recording
of a 640 by 480 run is turned into a `-commands` file (`to_commands.py`: the inputs of the recording with a `wait` for each gap, so
that no driver is needed), and the same file is played by the native exe and by `build/web/bejeweled2.js`, held and uncapped with the
recording's seed. The three `-trace` lists (the recording's, the native replay's and the wasm's) are then compared whole. Node has no 3D
device, so the web build plays the 2D game there (the page's own 3D game, `is3d`, is the one the real-input matrix of phase two will
play in the browser).

## What was found to be identical

| Mode | 3D game | 2D game, 640 by 480 |
| --- | --- | --- |
| Classic | 40 moves, 2 levels, identical | 40 moves, 2 levels (score 1020), identical |
| Action | 40 moves, 3 levels, identical; then idle through the end of the time (20 moves, 9000 idle updates) identical | 40 moves, 3 levels (score 19950), identical |
| Puzzle | 40 moves of the first puzzle, identical | 40 moves of the first puzzle, identical |
| Endless | 40 moves, 25 levels (score 56350), identical | 40 moves, 27 levels, identical (with `nosparkle` in phase one, without it since the port shrinks its pictures: phase two, below) |
| Twilight | 40 moves, 2 levels, identical; idle for 9000 updates identical | 38 moves, 2 levels (score 972), through game over, identical |
| Hyper | 40 moves, 7 levels (score 254700), identical; 20 moves and then idle through game over and 700 updates more, identical | 40 moves, 9 levels (score 395750), identical |
| Cognito | 40 moves, 2 levels, identical | 40 moves, 2 levels (score 1805), identical |
| Finity | 19 moves, 16 levels, through game over, identical (the score runs past what 32 bits hold, in the original and the port alike) | 14 moves, 13 levels, through game over, identical |
| Hidden mode | 40 moves, 2 levels (score 940), identical | two games through their ending, identical: 6 moves (score 130) and, from another seed, 17 moves over 2 levels (score 640) |
| A puzzle pack | not run | 150 moves: fifteen puzzles of the first pack solved over three planets (thirty tutorial clicks, twelve pods, three jumps to the next planet), every change of the board's state on the same update, identical |
| Classic to its ending | 158 moves, 6 levels, through "no more moves" and game over (score 15631), identical | 174 moves, 5 levels, through game over (score 7741), identical |
| A game left and resumed | identical through the load and 22 more moves, then different (see "Differences that stay") | 25 moves, the menu button, Main Menu, Classic, "Load Game?" yes, 25 more moves over three levels (score 1649), identical; the global generator's position too, as far as origrun logs it (4,000 changes) |
| A saved game at level 10 | not run | a hyper cube swapped and 40 moves from a save made at level 10 with eight flame gems and three hyper cubes on its board, levels 10 to 13 (score 87797), 83 changes of the score, identical |

The saved game at level 10 was made for the two scores that the user doubted on the site (2026-11-10: a flame gem's blast
"5,500 on level 10", a hyper cube "2400 PER GEM on level 11"). Both are the game's own numbers. `properties/config.xml`, the
same file in the Steam folder, the staged assets and the original's copy, sets `ExplosionChainBonus` and
`ExplosionChainBonusAdd` to 1000 and `ElectNormalBonus` to 400, and Classic multiplies them by 1 + 0.5 for each level past the
first: 5.5 on level 10, 6 on level 11. So a flame or bomb gem caught in another's blast pays 5,500 on level 10 (and 1000 more
times the multiplier for each further one of the chain), and every gem that a hyper cube strikes pays 2,400 on level 11 (2,200
on level 10, 2,600 on level 12). `make_save.py` makes the save with the port (the test command `level 10`, the game left by
its menu button) and sets the flame flags and the hyper cubes in the file; the original loaded that file, was given the same
clicks, and paid the same 83 amounts at the same updates. The web build under Node played the same recording with a trace
identical to the native exe's (18,285 lines).

The web build under Node played all nine modes of the 640 by 480 recordings (the ones compared with the original above, a seed of
909, 40 moves each, Twilight through game over, Finity through its ending) with a trace identical to the native exe's, line for
line (the board's score, level, state and generator position, the global generator's position and the three other watches of
`-trace`: 4,976 lines for Puzzle up to 56,692 for Endless), and the native replay's trace is the recording's.

## What the pass changed in the port

| Difference | Cause in the original | What the port does now | Commit |
| --- | --- | --- | --- |
| After the carried-over flame gem of a later level, the next clear scored 450 where the original scored 200 | `Board::ExplodeAt` (`0x5a68b0`) and the shock wave of `UpdateEffects` (`0x5a7712`) test their accumulators with `>= 1.0` (the decompiler shows it as `!NAN(x) && 1.0 < x != (x == 1.0)`); the port had translated it as `> 1.0`. The tables of shard and explosion increments hold exact 1.0, 0.5 and 0.25, so a burst draws one more number in the original at the update where the sum is exactly 1.0. The global generator was then one draw behind, the next level change put the flame gem on another square, and a later clear exploded where the original's did not | the three accumulators and the shock wave progress test `>= 1.0` | e03dc153 |
| "Reloading graphics" came up every time a game in progress was left for the main menu | `WinDMApp::DoMainMenu` (`0x5dcd44`) reloads only when the board's byte at +0x165ae is set, which the cheat words that recolour every picture set (COLORSWAP, BLACKANDWHITE, AMBERSCREEN, GREENSCREEN); the port tested `CanPlay()` | the port tests that byte; the game is deleted by the menu's slide as in the original | affae6de |
| An exploding flame or bomb gem made no sound; the combo sound did not rise with the clears that follow a move; the star on a flame gem and on the selected gem never turned or flickered; Cognito's bar and its count of puzzles solved without hints stood still; the planet's intro puzzle did not mark its gems; the board's lights did not fade with the whirlpool | Each of these is one field of the original that the port had declared twice, once by name in one file and once by its offset in another, so one side wrote a member that the other never read: +0x15e78 (set by `ExplodeAt`, read by the update that plays `SOUND_BOMB_EXPLODE`), +0x15e50 (counted by `DoneClearing`, read where a set picks its combo sound), +0x15e8c, +0x15fb0, +0x15fc0, +0x15fc8 and +0x15fd0 (`UpdateStarRots`), +0x17a4, +0x17a8 and +0x17ac (the secret galaxy's counters), +0x18f0, +0x18fc, +0x191c and +0x191d (the intro puzzle's flag) | one member for each field (`Board.h`). Play is the same: the nine modes' transcripts did not change, only the end screens of Classic, Puzzle, Cognito and Finity, which show the stars and the marked gems now | 97e99400 |

The same audit looked at every other float comparison of that shape in the board, the effects overlay, Hyperspace, the main menu, the
star field and the puzzle overlay; the others already agreed with the original.

Nothing else of the game changed in this phase; the other commits of the pass are the comparison tools and test commands
(`origcmp/`, `state`'s new keys, the `nosparkle` command, `-trace`).

## Differences that stay, and why

These are listed under "Oddities" in `GAME_NOTES.md` with how to see each one.

The 2D runs of Endless and Finity were compared with `nosparkle`, which makes `Board::AddAlphaSparkle` return at once in both games (a
patch in the original's copy, a test command in the port); the other 2D runs, shorter, ran without it and were identical too. The
reason is that the function's pixel test reads the 640 by 480 scaled copies of the gem images that the original makes when it
starts at that size, and the port keeps one set of 1024 by 768 images and scales on the screen, so the pixels it finds, and so the
numbers it draws, differ. The 3D game needs no such step and was compared without it. `Hyperspace::Update2d` draws one number per star
line from `Rand() % mWidth`, and the width is 640 in the original at 640 by 480 and 1024 in the port's 2D (scaled down), so a 2D game in
which a jump takes place would draw differently from there on; no 2D run so far has shown it, and the 3D game, where both are 1024 wide,
needs no such care. In the 3D original a click on a dialog reaches the board one update later than in the
port, which the comparison allows for by giving those inputs one update early (the Escape key that closes a hint dialog excepted). The
original, started again and again, now and then never makes its board, or only makes it late, or ignores its first clicks; the
comparison runs it again when the board is late (a run that ended without a board or after the first moves is not a difference of the
game). Bejeweled 2's `Done3dTesting` is never called by the port (a deviation already in the notes).

A resumed game in the 3D game is identical through the load and the 22 moves after it and then differs: the two flame gems that
the next level change carries over land on other squares (the original's two picks are one draw of the global generator later than
the port's, the words of the generator and its position being equal up to that update), and a later clear explodes where the
port's does not. The 640 by 480 game, where both sides tick on the same update, is identical through the same flow and two level
changes, generator included, so the difference is one of timing between the 3D original's loop and the port's around the load (the
"Load Game?" click was tried at four updates, the Main Menu click with and without its shift); it is under "Oddities". At 640 by
480 the port lays its dialogs out as the 1024 layout scaled, so the yes button of "Load Game?" is at (257,222) where the original
has it at (257,203), on the row that is the original's Cancel; the comparison gives the original its own place
(`to_origrun.py --remap`), and the layout is phase two's.

The hidden mode at 640 by 480 had never started in the original in the first runs. The cause was the comparison's: the circle
round the four mode buttons takes long enough for the new player's welcome message to open, the driver clicked its button where
the port's 640 by 480 layout has it, the original's button is elsewhere, and the original's message stayed up over its menu.
`record.py` closes that message with the Escape key now, and the mode starts on both sides at the same update.

## Phase two (looks identical), begun 2026-11-11

Phase two asks that every screen, dialog and text look as the original's does, that the music and sounds follow the same states,
that focus loss and regain do the same, and that the web build and the real input paths work. Nothing here changes how the
game plays but one wrap-around (Finity's timer bar, below): the bot's games in every mode (`tools/check.sh bejeweled2`) have the
frozen baseline's transcripts but for four lines of the start, where the menu slides in, and the baseline is refrozen with them.

### What was compared, and how

**Screens at 640 by 480** (the original's size without 3D, which is what it runs at in Windows Sandbox). The port plays a tour with
real input (`origcmp/record.py --mode tour`, `tourload`, `tour:<mode>` for each of the eight modes, `tour:dialogs`) and notes the
update of every shot; `origcmp/run_tour_original.sh` gives the original the same inputs at the same updates inside Windows Sandbox,
where origrun takes a shot at each update (the original's window is cut out of the picture); `tools/shotdiff` pairs them. A shot
is taken once the screen stands still, and the animated ones (the How To Play page's slide and its four pages, the whirlpool, the
level changes, the puzzle world's WARP) along their way, every 40 to 200 updates. The port runs with `-drawevery` (a draw after every
update, as the original does on its virtual clock) in these runs. Covered: title, new player, welcome, menu, secret menu, high
scores, options (from the menu and over a game), "Load Game?" and the loaded board, the messages in front of a first game, How To
Play, the board of each mode with its tutorial and big text, every level change, the end screens and the scores after them, the
puzzle world's WARP, and the dialogs that open from the menu: Quit, the user list with Rename, Delete and the create dialog, and the
high scores' reset question.

**Music and sounds by state.** origrun's DirectSound stand-in logs every sound the original plays (`logs/sound.log`) and a BASS
proxy beside the copy (`games/bookwormadv/tools/bassproxy`, the same 48 functions) logs its music (`basslog.txt`); the port's own
`BYOPOP_SOUNDLOG` logs its plays; `games/bejeweled/tools/origcmp/soundcmp.py` pairs them by order and state (each sound named by
load order, with its volume, pan and pitch; the music by song, order and each change of volume). The original plays on origrun's
host run (a second copy of its folder, `ORIG_RUN=bej2s`, so the sandbox copy keeps the real BASS).

**Focus and the page.** The shared page check (`tools/web/pagecheck.js`, with the game's table in
`games/bejeweled2/tools/web/pagecheck.js`) ran in the Browser pane against the web build.

**Real input.** `realplay/full.py` plays every mode with real mouse and key events only, from launch through the new player, the
welcome and the first-game messages, in 2D and 3D, windowed and full screen (below).

### What was found, and what the port does now

| Difference | Cause in the original | What the port does now | Commit |
| --- | --- | --- | --- |
| At 640 by 480 every dialog, button and what is laid out from a picture's size stood wrong, and `AddAlphaSparkle` read other pixels | the original loads each picture without small art of its own at five eighths of its size (`0x5afda3`: 8 by 8 pixels become 5 by 5 by area weights in 256ths) | the same arithmetic at each load (`CustomResourceManager::ShrinkImage`) | 35d0991d |
| Text at 640 by 480 was set in the wrong size | a font without an `sm_path` is the same descriptor at five eighths of its size (`0x5b4f8c`, an `ImageFont` scaled by 0.625) | the same | 72ec0b60 |
| The options dialog's checkbox labels and the welcome box's header were laid out differently | the options' label lines come in the original's order, and the welcome box's header widths differ for the two games (Deluxe and its trial) | the same orders and widths | 72ec0b60 |
| The main menu slid from and to the wrong place | it moves from and to 1000, not 700 (`0x5c8aa9`, `0x5c80e1`) | the same | 72ec0b60 |
| Stretched and rotated pictures (the big text, the power gem's glow, the spinning tiles) were sampled differently in the 2D game | the original's 2D game draws them through its own software paths on its software screen | the 2D game turns on the framework's software stretch and rotation (`mSoftwareStretch`, `mSoftwareTransforms`) | 72ec0b60 |
| The 2D whirlpool did not open over the menu's last picture, and each shot added its glow twice | the original's software screen is one surface that keeps the last frame; a shot reads it as it is | `DDInterface::mKeepScreen` (off by default; the 2D game turns it on): a frame does not clear the screen and a screenshot takes the screen as it is | 999c8f04 (shared), 63c4fbd5 |
| The menu's music was cut and started again at a fade-in when a game began, from the options dialog's Main Menu and from the high scores' Back | slot 0x30 of the music interface is `FadeOut(song, stop, speed)`: the original fades the song out and the pending offset (`MUSICOFFSET_MAIN`, `_ACTION` or `_MAIN_MENU`) plays when the volume reaches 0. The port called these three sites as `FadeIn(song, offset 1, speed)`, which restarts the song at order 1 and fades it in | `FadeOut(song, true, speed)` at the three sites; Classic's music events are then the original's, 105 each, with the same orders, the same volume steps and the same updates | 756ffefc |
| The test command `goto menu` did not leave a game, so the real-input run of Cognito never reached the menu | since `DoMainMenu` keeps the board (as the original does unless a cheat changed its pictures), the command has to hide the board and slide the menu in as the options dialog's Main Menu button does (`0x5cc563`) | the command does that | 756ffefc |
| The puzzle tour's board buttons and the app's own buttons made no click sound | the original's Board and WinDMApp each override the button listener's ButtonPress (0x596bc2, 0x5dc1cf) with a play of `SOUND_CLICK` | `Board::ButtonPress` and `WinDMApp::ButtonPress` do the same | f17efe75 |
| Finity's timer bar ran up where the original's fell, and the warning beep played in three extra stretches of 640 updates | the original's integer arithmetic wraps: `((score - target) * 0x2c4) / 2` goes negative once the score passes about 3 million, which Finity reaches at level 4. The compiler folded the port's expression into one multiply by 354 that does not wrap until twice that score, so the bar's target stayed at the top where the original's went to 0 | the game builds with `-fwrapv`, so every int expression wraps as the original's x86 code does; Finity's sounds and music are then the original's (562 plays against the original's 563, the one that is the title's click, and 204 music events each) | f17efe75 |

The comparison tools came with these commits (`origcmp/record.py`, `replay.py`, `run_tour_original.sh`, `to_commands.py --shots`,
`web_shots.sh`, `state`'s new buttons).

### What is identical now

**Pictures at 640 by 480.** Each of the eight modes was played by the port's tour (12 moves, a profile with the secret modes
unlocked, seed 909, `-drawevery`) and by the original in Windows Sandbox with the same inputs at the same updates, and the shots
compared by `tools/shotdiff`: Classic 5 shots, Action 15, Puzzle 4, Hyper 16, Twilight 6, Cognito 6 and Endless 55 (through its level
changes, five shots each) are identical but for one box on the menu's shots, the 99 by 26 pill at (384,436) where the
original has the empty "More Games" button and the port has nothing (the outbound button is out by the user's decision). The
earlier tours were identical in the same way: the first screens of a new player (title to the first board, commit 72ec0b60), the dialogs tour (quit, user list,
rename, delete, create, reset: 8), the puzzle world's whirlpool and WARP frame by frame (13) and the How To Play pages at every
200 updates of their 675 (19 shots, 18 identical and the pill). Finity differs in its pictures (below).

**The web build** plays the same recordings under Node (`origcmp/web_shots.sh`, the page's wasm with the page's files in place) and
its shots equal the native port's pixel for pixel: the first screens 31 of 31, dialogs 8, Classic 14, Twilight 6, Action 10, Hyper 16,
puzzle 13, the help pages 19; replaying Endless (40 shots), Finity (30) and Cognito (6) natively and on the web gives the same
pictures too (the shots in those three recordings, made between 03:00 and 03:05 with the build of that time, before commit 63c4fbd5, differ from
a native replay's of the same inputs; the fresh tours above, made with the final build, cover those modes). The page check (`tools/web/pagecheck.js`
with this game's table) passes in the Browser pane: 11 checks, none failed, one note. The page's own real-input play,
`games/bejeweled2/tools/web/full.js`, plays all eight modes (60 moves each, Finity to its end) through the canvas's DOM events,
silently, with no error, with the final build at the canvas's size (60 moves a mode) and at twice it (30 moves a mode, the pointer
at other sizes, as in full screen).

**Sounds and music** by state, in the eight tours: the music is the original's in every one (204 events each: the same songs,
orders, volume steps and updates; the menu song fades out at a game's start and the board's song starts at its offset), and every
sound play is the original's, with its volume, pan and pitch, in the same order (75, 138, 24, 122, 92, 8, 176 and 563 plays), but
for one `menuclick` in each tour that only the original makes: the click of the title's Play link, which the original's
TitleScreen::ButtonPress (0x5d9b0e) plays only while its trial counter at +0x308 has run down to 0, which it does in an unregistered
copy (origrun's) and never in a registered one, where the port is (GAME_NOTES, deviations).

**Real input.** `realplay/full.py` plays each of the eight modes with real mouse and key events only (window messages posted to
the game's window under rundesk, as visrun does; no test command but the state read, the bot's move, `unlock all` and `lose`), from
the title through the new player's name, the welcome message, the menu and the one-time hints to a game's end and back to the menu,
in 2D and 3D, windowed and full screen: all four runs end "full: ok" with the final build (12 moves a mode, Finity to its end).

**Focus** is the page check's: the window loses and regains the focus, and the page's answers are checked against the native
build's (`tools/web/pagecheck_native.py`), as the page check's table lists them.

### Differences that stay (phase two)

- **Finity's pictures** differ in small details from the first shot after level 2's clear and widely in the cascades of the later
  levels, where overlapping popups of millions cover the board; every state, the generator's index at every update (compared to update
  8716), the sounds and the music are the original's, so the difference is in what only Finity draws (the bomb, the rock, the wildcard
  destroyer) or in the popups' arithmetic. Not chased (GAME_NOTES, "Oddities"), with how to see it.
- **The 3D game's screens** were not compared with the original's in phase two: the original at 1024 by 768 needs the user's leave
  for its captures, which the coordinator has been asked for, and the 3D tour is recorded (`build/logs/bej2/t3_tour`). The 3D game's
  logic is identical (phase one), its sounds go through the code the 2D game's do, and its real-input play passes.
- **The empty More Games pill** (above), and the title's one click sound (above).

### Performance numbers

A Classic tour of 5008 updates ran in 1.5 seconds natively with `-fast` (about 3 300 updates a second, drawing after every update),
and the web build under Node ran its 5006 updates in 31.5 seconds (160 a second: the software stretch and triangle paths in wasm).
In the page (`?fast=1`) the eight modes' 60 moves each took 3 to 14 seconds. The performance pass itself comes later.
