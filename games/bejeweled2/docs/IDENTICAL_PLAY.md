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
"5,500 on level 10", a hyper cube "2400 PER GEM on level 11"). Both are the numbers of the `config.xml` that the install had (Steam's own file check replaced it on 2026-11-15 with the retail constants; GAME_NOTES, "Steam's file check"). `properties/config.xml`, the
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
| A flame or bomb gem exploding in a chain paid 200 where the original paid 800 (level value 20), and 1800 for the third where the original paid 1200 | `Board::ExplodeAt` (`0x5a68b0`) multiplies the chain count by `ExplosionChainBonusAdd` (default 10) and adds `ExplosionChainBonus` (default 40); the port had the two keys the other way round. Play with the stale install file (1.0 and 1000) hid it. Found by tracing the first Finity match on the retail file, where the original scored 1300 and the port 700 | the keys are read as in the original; Finity's score now agrees at every step of the tour | (this commit) |

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
The 3D game (1024 by 768), which the sandbox cannot run, was compared afterwards on the visible desktop ("The 3D game", below).

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
| A gem off the board (a shard-less gem flung out by a blast, the game-over's falling board) turned the wrong way | the original calls `Rotate(-mRot)`, which turns as the port's `RotateRad(-x)`; the port had `RotateRad(mRot)` (the framework header's note "old Rotate(a) is RotateRad(-a)" misled) | `DrawBoard` turns by `RotateRad(-mRot)` at both sites; the Action tour's game-over frame went from 25% of the picture different to 0.6% | e1ecff84 |
| The text of a puzzle's hint and the pause text showed under a dialog | the original tests the application's dialog count (+0x32c), not the demo pause flag | `GetDialogCount() == 0` | 0c48bccf |
| The 3D whirlpool of a level change and of Puzzle's WARP turned the board, score box and controls the wrong way | `PushTurnedPart` used the other rotation convention | `RotateRad(-angle)` as the original's `Rotate(angle)` | 6056fe1a |
| The first frames of the 3D whirlpool were blurred and had a dark first row and column | at render scale 2 a transformed quad is sampled linearly, which blurs a copy on whole pixels, and the mesh's `(v + 0.5) * scale` shifts it a whole device pixel | `DDInterface::mOriginalSampling` (off by default, on for this game's 3D display): such a quad samples nearest, the mesh takes `v * scale + 0.5` | 1bbcbf88 (shared), 57a6a00a |
| A replay that sent the input faster than the port read it lost input | the command queue was cut at 1024 lines | a longer commands file is read as the queue empties | d5aee40e (shared) |

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

### The 3D game (1024 by 768)

**Where it ran.** The original's 3D game needs a Direct3D device that neither Windows Sandbox nor rundesk's desktop gives it
(docs/ORIGRUN.md, "Hardware acceleration"): with `--sandbox --sandbox-vgpu --sandbox-connect` the game moves to the user session,
whose Microsoft Remote Display Adapter fails Direct3D 7's CreateDevice, and the original runs its 2D game. The user allowed the
visible desktop as the second route ("Yes, both routes", 2026-11-11), on terms that `origcmp/visrun.py` keeps: a copy under
`build/`, silent (origrun's sound stand-in mutes the copy's audio sessions), the picture taken only by `PrintWindow` from the game's own
window, input posted to that window and never through the cursor or the keyboard, no link, Buy or More Games button clicked, and the
window, the game's registry key and its users folder put back afterwards.

**How.** The port's 3D tour (`record_port.sh tour`, 16 shots: the title, the new player's screen, the welcome box, the menu, the high
scores, the options dialog from the menu and over a game, the How To Play screen's four pages, the first board and the board after
its moves, the menu again, "Load Game?" and the loaded board) is turned into an origrun script, and `visrun.py` plays it with the
original on the virtual clock as the 2D runs do (the game held at an update counter, an input read in the next update, a shot taken
when nothing is left to draw). Two things differ from the sandbox runs. The title's loading runs on the wall clock, so the update at
which the Play link shows differs from run to run: a `gate` waits for the title's loaded flag and moves every later command on by the
updates that took, and the Play click lands at a fixed update, since the board's seed is the generator's state there. And the port is
given the updates at which each input really landed: `visrun.py` writes `landed.tsv`, `landed_to_events.py` turns the recording into
events at those updates, and the port replays them (`to_commands.py --shots`; `bejeweled2.exe -commands -realinput -hold -fast
-drawevery -seed=909 -trace` under rundesk, in Direct3D 11 through SDL). `run_tour_visible.sh` runs all of it, then shotdiff. Two
rules of the replay come from what the original does on the desktop: the click that closes the first game's help screen goes one
update later (`--shift`, the 3D loop's click timing, GAME_NOTES, "Oddities"), and the pointer leaves the window after each click
(`--leave-after 1`), since the real cursor is never over the original's window and no gem glows under it.

| Difference | Cause in the original | What the port does now | Commit |
| --- | --- | --- | --- |
| The 3D whirlpool, as it opens at a game's start and over a loaded game, showed a hole across the board's top once its progress passed about two thirds | the original's Direct3D textures use clamp addressing, and the portal draws the board's backdrop through a window whose source starts above the picture (`EffectOverlay`); SDL's Direct3D 11 renderer draws no pixel of a quad whose texture coordinates go negative | `DDInterface::DrawTexturedQuad` cuts an axis-aligned 3D quad where its coordinates cross 0 and 1 and draws the part outside as a strip of the edge texels | 38ecf46f (shared) |
| The 3D whirlpool of a level change and of Puzzle's WARP never flew the interface away: the board, the score box and the controls stayed where they were (the controls at the top left, the board shifted left by 260 pixels) while the backdrop's mesh turned into the black hole (the user's frame of Ceti Alpha V's WARP, 2026-11-14; none of the earlier shots reached a whirlpool with an interface over it, only the loaded game's) | the original draws each of the three parts between its D3DInterface's `PushTransform` and `PopTransform` (0x44b8f0, 0x44b990), which move, turn and shrink it about its middle as `mInterfaceFly` grows; the port's 3D game draws through the plain SDL `DDInterface`, which had no transform stack, and `BoardDraw3D` pushed only onto a `GPUDisplay`'s GPU2D (a null cast here), so the pushes did nothing | `DDInterface` keeps the stack (`PushTransform`, `PopTransform`; empty unless a game pushes) and Blt, StretchBlt, BltTransformed, DrawTrianglesTex, FillRect, FillPoly and DrawLine take its top matrix as GPU2D's do (a blit filtered, the clip left to the screen); `GPUDisplay` forwards both calls to GPU2D; `BoardDraw3D` calls the display | 2dd66ab5 (shared) |
| The 3D menu showed the More Games button's empty pill, which the original covers with the button | the 3D game's picture of the gadgets (`IMAGE_MAIN_GADGETS`) has the pill baked in | `MainMenu::Draw` draws the picture in three pieces and leaves the pill out, as the 2D game's menu has none (the button itself is out by the user's decision) | 16b906be |

The comparison tools came with these commits (`origcmp/visrun.py`, `landed_to_events.py`, `run_tour_visible.sh`).

**The eight modes' 3D tours (2026-11-15).** Every mode has a tour of its own in 3D (`record_3d_tours.sh`, the port's bot with the test
commands, seed 909, the Steam files' constants of the time; the original on the visible desktop, `run_tour_visible.sh`; 21 to 160
shots a mode, the animated stretches every 23 updates), each compared with shotdiff and every box looked at. The replay keeps the pointer
where the click put it for the widget manager (`--leave-keep`: the original's WARP glow reads that position, and the first runs lost it),
gives the help click its extra update by itself, and the generators' indices were equal at every shot in all eight modes.
After the fixes above: Classic 18 identical, 2 faint, 1 differing; Action 42, 31, 1; Hyper 34, 23, 2; Twilight 14, 6, 2; Endless 5, 92, 4;
Puzzle 6, 150, 4; Cognito 10, 53, 10; Finity 1, 24, 77. The differing shots of Classic, Action, Hyper and Twilight are the More Games
button's pill in the menu shots (out by the user's decision) and, in Hyper and Twilight, the same in the secret menu. What is left:

- **Cognito's warp shots 0 to 8** (sparkles and explosion stars at slightly other positions and phases, up to 4,700 pixels, the shards
  matched) and its warp 45.
- **Endless** (level 3-4, 4-14 and 5-11, 33 to 39 pixels over the threshold) and **Puzzle** (the warp's frames 43, 10 and 45: the box of 110 by
  79 pixels round the black hole's middle, 32 pixels over): faint filtering differences along the hole's edge, Direct3D 11 against Direct3D 7.
- **Finity, from level 3** (77 shots): every gem of the board washed out white by the point lights of the explosion waves, where the
  original's gems are not. The cause found so far: the original never sets a shock wave's `mGrowing` (its stack slot decides). The
  original's watch showed a wave kept for its 30 updates for each of the four explosions of the Action tour (so the byte was non-zero
  there) and none at all in Finity, whose cascades the port starts waves for 1,759 times; the port sets the byte true. Not settled: what
  the original's stack holds in Finity (its watch of the wave list did not move even for the score texts, so the offsets read there are
  not trusted). To go on: a watch of the first wave's `+0x14` in the original in Finity, and `AddPointLight`'s calls (`BoardDrawBoard.cpp`).
  Written under "Oddities" in GAME_NOTES with how to see it.

**Re-recorded on the retail constants (2026-11-15, later).** The first tours above ran on a stale `config.xml` (every gem a flame gem, chain bonuses of 1000); Steam's file check put the retail file back, and the port's staged copy is the same file. All eight tours were recorded again with the port's bot (4, 10, 20, 10, 41, 11, 11 and 16 shots in the order Puzzle, Classic, Action, Endless, Hyper, Twilight, Cognito, Finity: flame gems are rare now, so the games are shorter) and played by the original on the visible desktop. Tracing the first Finity match found the chain bonus keys read the wrong way round (the table above, 3ab848a5). Two faults of the comparison itself cost a detour and are fixed: `run_tour_visible.sh` derived the help click's one-update shift from `landed.tsv` before the original's run had written it, so a run of a fresh tag replayed the port without the shift (the board's own update counter then differed by one, which moved the clearing's every third update, the global generator's shooting-star draws and so Classic's and Finity's level change); and one original run of Action slowed down while the user was using the desktop, so its time bar ran late. With the shift, and Action's original run again, the score sequences of all eight modes are equal step for step, Classic's board counter and generator indices agree with the original's, and Action's time runs out at the same update (19861, state 6 then 7 at 20235). Shotdiff, matching, faint and differing shots: Puzzle 2, 1, 1; Classic 1, 8, 1; Action 10, 9, 1; Endless 1, 8, 1; Hyper 37, 2, 2; Twilight 4, 5, 2; Cognito 2, 7, 2; Finity 1, 12, 3. Every differing shot is a menu shot (the More Games button's pill, out by the user's decision) except Finity's first board, where a yellow gem on its way down is 1 to 2 pixels from the original's (274 pixels). Hyper's two boxes of the first replay (level 4-4, one gem lit more brightly; level 6-1, 173 pixels) did not come back when Hyper was recorded and played again (37 identical), so they were the first original run's, not the port's. The faint shots are sub-threshold differences of filtering and sparkle phase, looked at in every box. Not compared in the re-recorded runs: the washed-out gems of Finity (the old oddity below), which the retail constants do not reach in these tours, because their waves are rare.

The classic hyperspace jump (2026-11-15): a Classic game's level change in 3D is the whirlpool, then `DoHyperspace` 100 updates in, the tunnel, and the new level (`BoardMisc.cpp`). The tour's level shots start after it, so a Classic tour with an extra shot every 25 updates from the whirlpool's start to the new level's (updates 5500 to 6225, 30 shots) was played by the original and the port: 40 shots compared, 35 faint, 2 differing by 40 and 41 pixels over the threshold (w5775 and w5975, boxes of 96 by 114 and 195 by 190 pixels along the tunnel's edge), and the menu's pill. The tunnel, its mesh, rays and colours, and the way out onto the new level's board match. Puzzle's WARP carried the same mesh and matched earlier. A seam in the tunnel (SDL refusing triangles with texture coordinates outside 0 to 1) was looked for in `Hyperspace::Draw3dPortal` and is not visible.

**Not compared in 3D.** The 3D game's sounds and music (they run through the 2D game's code, compared and matching; the visible desktop runs
are silent); the web build's 3D pictures by shot-for-shot judgement of every box (it shares the code). The page replays each mode's native tour commands under the same seed (`tools/web/replay3d.js`, generated into a driver by `replay3d_gen.py` from a commands file and the tour's profile, holding the page after each count of updates; `tools/web/native_ref.sh` records a mode and plays the native reference) and its shots were set beside the native shots, with no error on the page in any mode. Matching, faint and differing shots: Puzzle 4 (1, 3, 0, with the click correction below), Classic 10 (0, 6, 4), Action 20 (1, 12, 7), Endless 10 (0, 3, 7), Hyper 41 (2, 22, 17), Twilight 11 (0, 6, 5), Cognito 11 (1, 5, 5), Finity 16 (0, 11, 5). The boards, gems and scores are the same in every pair I opened (Classic's 605, Action's 32,600 at its game over), and the differing boxes are animation phase of one to two updates: the How To Play dialog's slide-in (22.56 percent of the picture in every mode that shows it, the background 10 pixels apart), the first board's shooting star (3,121 pixels), falling gems after a level change, and the game-over explosion's flying gems. I opened a few of these, not every box of Hyper's and Endless's, and judged the rest by their sizes and places, which repeat from mode to mode. The likely cause (not proved) is a click reaching the game one update away from the native click: Puzzle's board slide-in matched once the wait after a click was one update shorter (`cd=1` on the page), but that correction made Action's first board a different one (the random draws follow the updates), so it is off by default; both are unfixed. The shots after a game over (Action's over0 to over3) and Hyper's later levels carry the same offset;
the original's full screen (a Direct3D exclusive mode change of the user's display, which the port does not make by the user's decision of 2026-11-07, and which no run puts on the user's screen). The port's own full screen, borderless over the desktop at render scale 3 (3840 by 2160 here), was replayed through the same 40-shot Classic tour (whirlpool and hyperspace included) and set beside its windowed run (scale 2): 18 identical, 22 faint, none differing; the picture is the same 1024 by 768 game drawn at another scale. The 2D shots' pass (above) covers every screen at 640 by 480.

### Differences that stay (phase two)

- **Finity's pictures** differ in small details from the first shot after level 2's clear and widely in the cascades of the later
  levels, where overlapping popups of millions cover the board; every state, the generator's index at every update (compared to update
  8716), the sounds and the music are the original's, so the difference is in what only Finity draws (the bomb, the rock, the wildcard
  destroyer) or in the popups' arithmetic. Not chased (GAME_NOTES, "Oddities"), with how to see it.
- **Finity's 3D boards** from level 3 (the washed-out gems, 77 shots) and **Cognito's warp sparkles** (shots 0 to 8): above, not settled.
- **The More Games button** is out by the user's decision, so the 640 by 480 menu's 99 by 26 pill and the 3D menu's pill, which the
  original draws with its label, are boxes of every menu-family shot, in 2D and 3D (the 3D whirlpool's frames over the menu too).
  The title's one click sound is above.
- **The "?" button on the new player's first menu in the 3D original** (the profile's uninitialised counters; GAME_NOTES, "Oddities"):
  the port zeroes them, so the secret modes' button is not there. It shows on the 3D original's first menu shot only.
- **A gem or two of the loaded board during its slide** (3D, updates 3810 to 3826, 8 of 13 shots, 100 to 500 pixels each): a gem
  at the board's bottom right has its facets in other places; at rest and after the slide the shots are identical. Not chased
  (GAME_NOTES, "Oddities").
- **The 3D tour's hover and first click timing** are shaped by the replay, not the port: the pointer leaves the window after each
  click and the help screen's click goes one update later, as the original's runs on the desktop do (GAME_NOTES, "Oddities").
- **The web page's animation phase** after a click (1 to 2 updates, in every mode's replay; GAME_NOTES, "Oddities"): boards, gems and scores are the original's, the dialog's slide-in, the shooting star and the game-over explosion are a couple of updates apart.
- **The software screen** (no GPU): identical to the GPU path in all eight 2D tours; its 3D look is the GPU's (a machine without a GPU has no 3D).

**Looks identical: done (2026-11-15).** Every screen, dialog and text of the 2D game and the 3D game in all eight modes was compared with the original (2D at 640 by 480, 3D on the visible desktop under dgVoodoo2, the port natively, on the web page and as the no-GPU software screen), and every flagged difference is fixed or listed above and under "Oddities" in GAME_NOTES with how to see it. What was not compared: the original's own full screen (a Direct3D exclusive mode change, never made by the port or put on the user's display) and the web page's pictures at the update (the phase offset above).

### Performance numbers

A Classic tour of 5008 updates ran in 1.5 seconds natively with `-fast` (about 3 300 updates a second, drawing after every update),
and the web build under Node ran its 5006 updates in 31.5 seconds (160 a second: the software stretch and triangle paths in wasm).
In the page (`?fast=1`) the eight modes' 60 moves each took 3 to 14 seconds. The performance pass itself comes later.


## Screen coverage pass (2026-11-21 and 22)

### Why the looks phase missed these screens

Phase two compared the screens a tour visited, and no tour had reached the No Hints and Mode Unlocked messages, the credits
roll or the galaxy's completion; coverage shows that code ran, never that a screen was compared. The audit of 2026-11-21
named the gaps, and `docs/SCREENS.md` now lists every screen with the shots that compare it (`tools/check.sh screens`).

### What was added

The tours gained the finale (`tour:puzzle` with `TOUR_FINALE=1` on a player at the galaxy's last planet: the warp, the credits
roll with a shot every 400 updates, GALAXY COMPLETED, MODE UNLOCKED, the Back to Main Menu link, the menu and a new Puzzle
board after it), the puzzle's hint dialogs (Still Solvable, Illegal Move), the galaxy map and a locked planet, the user
dialogs (Enter Name, Name Conflict), the mode help boxes and the one-time boxes. The original's side is origrun's sandbox lane
(`run_tour_original.sh`, `screens_run.sh`) and, on the host with the in-game frame reader, `run_tour_reader.sh` and
`screens_reader.sh` (the reader's surface is `[[app]+0x36c]+0x68`: +0x36c is the 2005 framework's application-to-DDInterface
pointer, +0x68 its draw surface, +0x3c a critical section; the waits are 4bbf1f and 4bbedb). `record.py` records a `loaded`
update, `to_origrun.py --gate` waits for it.

### Differences found

1. Dead controls after the Puzzle galaxy's credits link (the user's report): the stale `mBackLink` stayed in the widget
   manager after the board went; removed with the board's buttons, and a game saved in the end screen's fade loads as an idle
   board (46c05b33, 8707994d).
2. The No Hints and Mode Unlocked messages showed blank buttons: their footer now reads OK, as the original's (46c05b33).
3. The Deluxe text on the main screen is removed, at the user's request of 2026-11-18.
4. The credits roll drew white wireframe lines the original does not. The lines are drawn additively with the colour's alpha
   at zero until the face morphs; the framework's additive fill on the processor-drawn screen ignored the alpha, where the
   original's fill (0x455820, reached by DDImage::FillRect 0x450630) scales each channel by it. Fixed in
   `framework/DDImage.cpp` (e28cdbda, a `shared:` commit; every game's baseline is checked at the merge).
5. The credits' Back to Main Menu link drew a hover underline: the port set `mUnderlineOffset` to 0; the original writes 0 to
   the field at +0x140, the underline's row count (HyperlinkWidget::Draw 0x48f070), so it draws none (a81d4bb6).
After the fixes the credits shots (fin00 to fin23) match the original, as do the host reader's.

### Differences that stay

- A new message box under a resting cursor is lit in the original (Enter Name, Name Conflict, the verify boxes) and not in
  the port's recording: the cursor stays where the click that opened the box left it. Not found within the hour (the
  framework's `RehupMouse` needs the widget manager's `mMouseIn`, which a posted click sets); real input in the page should
  be checked before the merge (oddity).
- The title's letters and progress bar differ by phase: the original's title loads on the wall clock (319 updates warm, 1259
  cold in the sandbox), so a shot at one update count catches another frame. The tours wait for the load (a click at update
  900, 2000 for a cold sandbox).
- The warp's falling gems and sparkles, the shine of an Endless or Hyper board (level4-1) and the moon texture of the finale's
  menu are other draws; a host or desktop run of Twilight, Hyper, Endless or Cognito boards is not comparable with the port at
  all (the host original's boards and shine phases differ; the cause is not found, and the generator is main-thread and
  update-aligned). The sandbox's Twilight shots match the port (20 of 23).
- The High Resolution? / Full Screen? question depends on the desktop's width and is checked on the desktop only.
- Not in this pass: the original's full screen and 3D for the new tours (the desktop lane), the packs selector (one pack in
  the stock install), No Longer Solvable, No Hints, More Hints? and the secret puzzle's hint warning (no recorded input
  reaches them: rows in SCREENS.md say why).

### The desktop lane: the finale in 3D (1024 by 768), 2026-11-22

`games/bejeweled2/tools/visrun/run3d.py` (config `bejeweled2-3d.toml`) played the port's 3D recording of the finale (40 shots,
`build/visrun/rec3d/finale`, the player of `make_gapprofile.py --puzzle 75 --galaxy 0`) with the original on the visible desktop, the
stand-in's activation hook installed and no "run stopped" line, then `tools/shotdiff` (run `v3d_finale`): 40 pairs, 28 match, 9
faint, 3 differ. Every credits shot (fin00 to fin23, fin-dialog37 with the OK button, the link, the board after it) matches or is
faint (fin01, fin02: 0.03 percent). The three that differ are the title (the load's phase), the menu and link-menu (the More
Games pill, removed). The warp shots are faint (sparkle phases). The 3D credits roll therefore equals the original's, with the
fixes of the 2D pass. The title's logo region the run forbids is the 2D one scaled by 1.6, unconfirmed.

The other new tours in 3D (the same lane, one run each; `tools/shotdiff` runs `v3d_puzzle`, `v3d_dialogs`, `v3d_classic`):
- Puzzle (hints, Illegal Move, Still Solvable, the galaxy map and a locked planet, the warp): 22 pairs, 2 match, 18 faint, 2 differ (the title's
  load phase, the menu's More Games pill): every hint and galaxy shot is faint or identical.
- Dialogs (the user list, Rename, Delete, Create, Enter Name, Name Conflict, Quit, the reset question): 10 pairs, all differ only in the menu
  behind the dialog (the sky's stars and comet phase, the More Games pill) and the lit button of a box under a resting cursor; the dialogs' own
  pixels are the original's.
- Classic with the one-time boxes (the hint-boxes player): 11 pairs, the Help screen matches and every board differs, because the original
  dealt another board (other gems from the first shot). Not chased (the board draws of the host and desktop lanes are listed above as not
  comparable; this is the same, found here on the desktop lane in 3D with this player; the earlier 3D Classic tour with the other player
  matched).

**The original's full screen: not compared.** In Windows Sandbox (the user, via the coordinator, 2026-11-22: only where the mode change cannot
reach other runs) the 2D Classic tour was played by the original four times with the registry's `ScreenMode=1` (staged: the sandbox's
`registry.reg` holds it): with the sandbox's activation hook wrongly on (562bb6f4 to 170e7497) and without the player it stayed at the title
and then at the New User box; with main at cf1798fb (hook off in the sandbox), the player and the title's cold load waited out, the tour
played through, 14 of 16 pairs match the port's windowed shots (title phase, More Games), with and without `--sandbox-vgpu`. But the
game was windowed in both (its caption bar in every shot) and wrote `ScreenMode=0` back: with this setting the original does not stay in full
screen in the sandbox (a failed exclusive mode, then the windowed fallback), so the pairs are windowed ones and no full screen shot exists.
The desktop is not used for it (a display mode change reaches every other run and covers all four places); the port's own full screen
(borderless) was compared earlier with the windowed shots (phase two). To try again: an exclusive hold on the desktop (the coordinator's
plan) or finding what the sandbox's display refuses.

### Real input for the new screens, and the puzzle hint dialogs (2026-11-22)

Native, on the final build (`realplay/full.py`, real input only, every mode, 12 moves): 2D windowed, 3D windowed, 2D full screen and 3D full
screen, all four "full: ok". The finale tour is itself real input (record.py plays through the driver's posted input and the bot's
`move`), under rundesk. The page (the Browser pane, `?silent=1`, a rebuilt web build): `tools/web/pagecheck.js` 11 passed, 0 failed;
the finale's recorded input replayed in the page against the native shots (`web_finale`, 40 pairs): the credits text and the boards are
the same pictures, one update apart in the scroll and the board's slide-in (the page's known one-update input skew); every credits line
reads the same.

No Longer Solvable by real input (record.py `TOUR_NOHINTS=1` on the puzzle tour): after the hint, a legal swap that is not the solution's,
then the hint button: the dialog appears in the port and the original, and the pairs match (shotdiff `nohints2d`). No Hints needs a
board the solution no longer fits (the generated swap kept it valid), More Hints? follows the board's UNDO button when no move is left:
not reached; their rows say rare.

### Runs

`screens2d` (330 pairs; the last run, after the fixes, is named in the merge request).
