# Rocket Mania Deluxe: the identical-play pass

Both phases of the pass CLAUDE.md names. Phase one: **plays identical**. The same input gives the same games in the original and the
port, judged by the game's state and not by pictures. Phase two (2026-11-11): **looks identical**, screens, dialogs, sound, focus,
real input and the web build, and a leak of the port found on the way.

The original is the game carved out of `RocketMania.exe` (`re/rocketmania/bin/RocketManiaGame.exe`), run only through
`tools/origrun` from a copy under a short path (`C:\byo-og\rm\game`), under rundesk (invisible, silent, below normal priority),
on origrun's virtual clock (`--clock virtual`: the game is held between updates, so an input lands on an exact update) and with
the scripted cursor. One game process at a time, the original or the port, never both together. The port runs with `-fast
-drawevery`.

## How both sides are driven

**One input for both.** The port's bot (`src/testhooks/UiBot.cpp`, `UiBotPlay.cpp`; `-uibot=<seed>`) plays whole games through
the mouse only: the loading screen's link, the selector's mode button, the difficulty and continue dialogs, the hint dialog,
the board's tiles (left and right clicks, the arcade drag, the top strip that drops the next tile), the quit button, the game-over
dialog, the name box of the high-score table and its menu link. What it does is written as script lines (`-uiscript`), which
`tools/identical/toorig.py` turns into an origrun script (`at <update - 1> ...`: origrun reads an input at counter N between
updates N and N+1, the port runs a script line in update K before its widgets). Replaying that script reproduces the bot's own
state log, so the script is the whole input. It never clicks a link button of the Options dialog or a "buy" or "more games" screen.

**One start.** The original's `srand(GetTickCount())` in `SexyAppBase::Init` is patched to a constant (origrun `--seed`, a
one-line patch list that `tools/identical/setup.sh` writes); the port's `-seed=N` is the same `srand(N)`.

**The values.** `tools/identical/values.txt` names 28 values by their address in the original (origrun `--watch`, four per run,
so the original runs once per four values) and by the board's member in the port (`-statelog`, which writes every change of
each value with the update it came at): the mode and difficulty, the board's update counter, state, score, level, the rockets
left and wanted, the moves left, coins, the drop timer, the fuse and its countdown, the burn path, the flames lit, the tiles
dropped, the pause count, the end-of-level timer, both generators' index and first word (the board's and the global one), the
effects count and the three launch counters. `values_grid.txt` adds the rotation and the kind of each of the 54 cells,
`values_right.txt` the number of each launcher on the right edge, `values_more.txt` the moves left, the strategy countdown and the
size of the list of moving pieces. `cmp.py` compares them change by change; `first.py` finds the first update at which a
value differs; `window.py` prints a window of updates, both sides side by side.

## What was compared

Every comparison below ends with every value the same at every update, except for the oddities listed at the end.

| Case | What it covers |
| --- | --- |
| c1, c2, c3 | One game each of classic (difficulty 2), arcade (2) and strategy (3), 3000 board updates, quit by the quit button |
| a1, a3, k1, k3, s1, s2 | Difficulties 1 and 3 of arcade and classic, 1 and 2 of strategy, 4000 updates |
| m3 | Three games in a row: arcade, classic, strategy, with the selector in between |
| g0, g1, g2 | Difficulty 3, two games each of arcade, classic and strategy played until they ended by themselves (about 60,000 updates a game): rockets launched, levels, buying rockets, game over, the high-score table with its name entry, a second game after it |
| h0, h1, h2 | The same at difficulty 1 |
| h3 | Three games in a row (arcade, classic, strategy) at difficulty 2 |
| cont1 to cont4 | A saved game written by one program and continued by both: classic written by the port, classic written by the original, arcade written by the port, strategy written by the original. Both programs read the same file and play the same input on from its Continue button |

The two programs' saved games have the same size in every case; the file contents are not compared byte for byte (they hold heap
pointers and uninitialised fields).

`-fast` is checked: a real-time run of the classic game without `-fast` and `-drawevery` gives the same script and the same
values, except for the global generator's index (the number of draws the menu's falling leaves make depends on how often the
screen is drawn; under origrun the original draws once per update, which `-drawevery` copies) and effects that start or end one
update apart for the same reason. Nothing that decides play depends on it.

A frozen baseline of the port is `build/baseline-rocketmania/` (never rebuilt; copy it to new checkouts). `tools/check.sh
rocketmania` plays three bot games (one per mode) in the current exe and the baseline and requires the same, non-empty state log
from both.

## What the pass changed

All the differences were found by bisection to the first update at which a value differed, then by reading the original's
code in our export or its disassembly. The commit is on branch `claude/frosty-poincare-77c4d9`.

| Difference | Cause in the original | What the port does now | Commit |
| --- | --- | --- | --- |
| The global generator and the board's generator drew other numbers | The original seeds its Mersenne twister the 2003 way (`mt[0] = seed`, `mt[i] = mt[i-1] * 69069`, 31-bit draws) from `srand`; the board's own generator is seeded from the global one's draws. The effects, the head and the tiles' sideways jitter draw from the global one; the CRT's `rand` only draws the picture | A game-local global generator seeded that way (`GlobalRand`); every call in the effects, head and pieces uses it; the board's generator is seeded from its draws | 4f774d4 |
| The global generator's draws came in another order | The original puts the board at the back of the widget list (`BringToBack`), so the board updates before the head and the effects | `StartBoard` does the same | 4f774d4 |
| Flames crossed a cell at the wrong speed | The two steps of an igniting flame are 0.166666 and 0.333332, and the rocket's is 0.015 (Ghidra showed the doubles stored as two dword immediates wrongly) | Those constants | 4f774d4 |
| A first board and the timed modes' rhythm were off | Two board members have starting values: -1 for the fuse's launch and its countdown, the arcade drop delay `Rows20()` (20 / rows) and 2 for the timed modes' drop interval | The constructor sets them before the state is set | 4f774d4 |
| Strategy mode's hinted first board had a wrong tile | The 18-entry rotation table's last entry is 2 | Fixed | 4f774d4 |
| A click on the arcade top strip dropped the next tile late | The click sets the drop timer to the whole delay (the next update drops the tile); after a launch it is the delay less 100 | As the original (`m2dc = m444`; the launch keeps `m444 - 100`) | 4f774d4 |
| A scripted click did not drag the pressed tile | The original's window procedure sends the board a move (a drag while a button is down) before every mouse message, so the release is preceded by a drag at the same point. A tile pressed earlier and still remembered moves to the next column when the click lands more than half a cell away. The script runner did press and release without the move in between | `-script` clicks move again before the release, as a real click does | de03ce5 (`shared:`) |
| A click on a score text did not reach the board | The floating score texts are not mouse targets (the constructor clears the flag) | The same | 0afca39 |
| A rocket on the right edge could get the wrong number | A new launcher takes its number from its row (the generator's fourth argument is the caller's row); the port passed the column | Row | 978675f |
| Strategy mode ended by itself when the night timer ran out | The night timer is for the classic mode only (the original tests the mode first) | Classic only | 7b7a74e |

Harness only (not in the port): the PNG loader of the carved exe overflows a buffer (about 26 KB) on the 1-bit `_Arial8.png`
and dies on a fresh heap, so origrun's patch list for the copy sizes it right (`png` line of `tools/origrun/patches/rocketmania.txt`,
written by `tools/identical/setup.sh`, git-ignored). `setup.sh` carves the exe from the installed game with the private
`extract_game_exe`, so it is in `.publishignore` (62e5f58).

## Oddities skipped

- **The fuse's countdown before the first launch** (`burnt`): the original never sets it before the first launch, so it holds
  what the heap held. The port's value is its own; the comparison leaves that first value out.
- **Pieces' fields the original leaves uninitialised** (`mFromRow` and the like) differ; nothing reads them.
- **The global generator's index follows the number of screen draws** (see above): in real play the effects' random positions
  differ from run to run, as in the original. It touches no rule.
- **origrun's watch keeps 4000 changes**: a value that changes every update (the global generator's index) is compared only for
  the first 4000 updates of a run; its first word and the board's generator cover the rest.

## Phase two: how it looks, sounds and takes input

**Sound.** Three whole games (arcade, classic, strategy, 4571 plays in the original) were played in the original and the port under
the same input with origrun's virtual clock and `--sound wav`, which logs each DirectSound play, and the port's `BYOPOP_SOUNDLOG`.
Sample by sample, the plays come in the same order at the same update with the same volume and pan; the original made 4571 and the
port 4569, the two that differ being a sample that the original plays at the very end of a run, after the port's last update. The
music's fade-ins, volume curves and stops match as well. One difference was found and fixed: the original's menu buttons beep when
pressed (`GameSelector::ButtonPress`, 0x405ec0), the port's did not (commit 83dc5e84).

**Focus.** A focus loss and regain in the middle of a game, on the host under rundesk: the pause count, the state and the sound match
the original's (the head slides into its hole, the music fades out and back, the fuse clock stops).

**Screens (2026-11-11).** Every tour of `games/rocketmania/tools/shots/` was played in the original inside Windows Sandbox (origrun
`--sandbox`, virtual clock, scripted cursor) and in the port, and compared by shotdiff: the full tour u2 (menu, the difficulty
dialogs, the hints, a game, level-up, high scores, game over), the tours t1 and t3 to t7 (generated by
`gen.py` from phase one's bot scripts: games of the three modes with all their dialogs), o1 and o2 (Options dialog, credits) and dd2
(dragging the dialogs). For a shot of a moving board the original's frame is not the one the label names, and the framework of 2003
gives origrun nothing to wait for, so the port is run with a shot at every update of each window, `lagscan.py` picks for each of the
original's shots the port frame that fits it best (aligned by the label's own frame) and shotdiff compares the pair
(`best.sh <tour>`; the run id is `<tour>best`). A box that fits at the label's own update but is passed over by the best-frame choice
(two such boxes were checked: u2 s41800 and t6 s41737) is no difference.

| tour | pairs | match | faint | differ |
|---|---|---|---|---|
| u2 | 99 | 46 | 46 | 7 |
| t1 | 30 | 6 | 24 | 0 |
| t3 | 90 | 46 | 43 | 1 |
| t4 | 64 | 13 | 46 | 5 |
| t5 | 26 | 7 | 18 | 1 |
| t6 | 33 | 11 | 16 | 6 |
| t7 | 35 | 7 | 27 | 1 |
| o1 (Options dialog) | 12 | 1 | 0 | 11 (the removed More Games row) |
| o2p (credits) | 4 | 0 | 0 | 4 (the Options dialog behind it, same cause) |
| dd2 (drag the dialogs) | 4 | 4 | 0 | 0 |
| all | 397 | 141 | 220 | 36 |

"Faint" is shotdiff's class for pixels that differ below what is visible (the blurred difference stays under 32 levels, or the box is
under 32 pixels); every faint box was looked at, and they are of two kinds. most of u2's faint pairs and a share of every other tour's
show the head in its hole, the one picture the original's software screen stretches by its own routine (0x44abeb) and the port's SDL
by bilinear filtering: the largest difference is 38 to 39 levels on about 800 pixels, and the stretch is a deviation listed below. The
rest is the particles (sparks, bursts, fireworks) whose positions come from the global generator (see the oddities), at most 33
levels. The 36 that differ: 15 are the Options dialog and the credits screen behind it, which have the same pieces, fonts, texts, check
boxes (windowed, no 3D) and slider (the music's 0.595) and differ only in the original's More Games row (a link out of the game, removed from the
port by the rule of 2026-11-07, so the port's dialog is one row shorter and stands that much lower); the others are the board's shake and its
column's jitter, particles, and the same frame choice that the two checked boxes showed, each judged one by one (u2 s46000 shake and
particles, t4 s43693 shake of (-2, 0) and particles, t5 and t7 s40450 a spark).

**Two things that fooled the comparison, and the fixes.** (1) With no value in the registry the port starts in full screen and in 3D
(its default), and the original in the sandbox has no GPU, so its game reads `Is3D` as 0 and draws into a software screen: the
screens are compared with `RM_REG="Is3D=0,ScreenMode=0"` (windowed, no 3D); a 3D comparison needs `--sandbox-vgpu`, which is allowed
only between 22:00 and 08:00, and was not made. (2) shotdiff searches a global shift of up to 8 pixels to excuse a window frame
that is a pixel off, and it can excuse a whole screen that is truly moved: a dialog that was dragged 8 pixels came out as "shifted"
and its only box as a line of text. A pair's `shift` in `summary.json` is read now, and a nonzero one is judged; `lagscan.py` takes
its alignment from the label's own frame (it took it from the window's first frame, which could be another screen).

### The changes of this phase

The first group is c17b6535 (found by the u2 tour and the comparison of the tours' first shots), the second 150ae162.

| difference | cause in the original | what the port does now |
|---|---|---|
| The sidebar's launch icon and the pictures after the removed "register" image were off by one | the port's table had lost an entry | the table's indices are the names `IMAGE_*`, taken from the original's resources |
| The next tile (0x41e530) stood still | it slides down 42 pixels as its fall timer runs, stops 32 down when the column's top cell is taken | the slide, done as the original's x87 code does it |
| The rocket slid in from a wrong place and the fuse strip stood wrong | the rocket moves 0.2 and 0.4 pixels for each of its 155 updates left; the strip is centred on the 42-pixel cell, then moved by 7 and 8 | as the original |
| A falling tile was drawn grey | the original sets the colour (120, 100, 120) but never turns image colouring on | plain picture |
| The burst effects' fields and speeds | the original keeps these doubles as two halves, which the export showed as 0 | the four sparklers' speeds (-5.0, 5.0), places and lives, set as the original's |
| The bar's two flames and the launch's points | the flames' places are 29.0, 166.0 and 44.0, 166.0 (doubles); the points appear where half of the lit fuses are reached, and the count of the lit fuses was a lost local | as the original |
| FinishBurn counted the board's rows | it counts the lit fuses | the count of lit fuses |
| The music and sound volumes | 0.595 and 0.85 until the registry says otherwise | as the original |
| The loading bar | the picture starts 20 pixels filled, the other 332 follow the progress | as the original |
| The name-entry box's colours | its four colour fields are red, green, blue and alpha, the background and outline clear | as the original |
| Menu buttons and dialogs tiled differently | the 2003 framework's DrawImageBox cuts the tiled middle as wide as the corners | `Graphics::mDrawImageBox2003` |
| The difficulty, continue, hint, high-score and level-up dialogs moved when dragged | their Move (0x415c30) and MouseDown (0x403880) do nothing; the whole-screen scenes' MouseDown (0x410470) and MouseDrag (0x415c30) do nothing | the same overrides; the drag test dd2 matches the original |
| Rotated pieces were smooth | with `Is3D` 0 the original draws rotations in software (DDImage::BltRotated, 0x441127) and keeps the last frame on its screen | the framework's software transforms and kept screen are on when the game is not 3D |
| A whole game left 100 KB behind (150ae162) | the original's end of a game (0x42e9f0) removes the board from the widget manager and deletes it; the port only removed it | the board is deleted; 120 and 140 cycles of games, dialogs and the game-over flow (`driver/leakcheck.py`) end flat |
| The board's game-over kind (150ae162) | the constructor (0x41ae80) sets it to 3 | the constructor sets it; nothing reads it before the game over sets it, but the save file holds it |

### Oddities and deviations that stay

- **The board's shake and the next tile's column jitter** (`Resize(CrtRand() % (m4b8 * 2 + 1) - m4b8, ...)`, `CrtRand() % 3 - 1`) draw
  from the C runtime's generator, whose index follows the number of screen draws; a screen draws a different number of times in the
  original and the port (a shot's own timing, a window's repaint), so the shake of a frame and the jitter of the column at
  x about 249 never agree. The shake moves the board by a random amount of up to the shock's size, the jitter by one pixel; in
  play they are random in both programs. Not chased (the rule of 2026-11-01 on oddities): a match would take the generator's
  index of the original's run and a draw count that depends on the machine.
- **Particles** (sparks, bursts, fireworks) take their places from the global generator and differ for the same reason.
- **Fields the original leaves in the heap** (the board's `m444` outside the arcade mode, the game-over kind before 150ae162 set it, the
  pieces' `mFromRow` and the like) and **raw heap pointers in the save file** (the piece's number, special and host): the original's
  saves differ from run to run in those bytes and in the clock, and the port's are the same kind. Every other byte of a save written by
  one program and read by the other is the same, and the high-score file of the same runs came out byte for byte identical.
- **The software screen's stretch (deviation).** In 2D the original stretches the head in its hole with an older routine of its own
  (0x44abeb); the port keeps SDL's bilinear stretch, which comes within 38 levels on 800 pixels. Not visible; the framework's 0x433ca0
  would be sharper than the original.
- **The original's loading takes more than 10000 updates in the sandbox** (a loading thread on the wall clock, updates on the virtual one), so a tour's
  first click belongs at update 40000 or later.

### What else was checked

- **Leaks.** Found by listing the live blocks of 30 game cycles by call site; the board's 100 KB a game was the one leak. After the
  fix, `driver/leakcheck.py` ran 140 cycles through every mode, the options, the high scores, the credits and a lost game: private
  bytes reach 154.57 MB at cycle 80 and stay at exactly that for the last 60 cycles. 120 cycles of the three modes alone drift up
  by 0.7 MB (about 6 KB a cycle, with no step by cycle: the allocator's growth).
- **High-score files.** The same runs were entered in the original and the port, and the two files are identical byte for byte.
- **Saves.** A save written by one program and read by the other, as phase one's cont runs, still plays on; the bytes that differ are
  the raw heap pointers, the clock and the leftover fields above.
- **The page check.** `tools/web/pagecheck.js` with this game's table (`games/rocketmania/tools/web/pagecheck.js`): 11 passes, 0
  failures, 1 not applicable.
- **Real input.** `driver/play.py` played three whole games (arcade, classic, strategy) through posted SDL events in each of four
  settings: windowed and full screen (Alt+Enter), each in 2D (`Is3D=0`) and in 3D; all four ended at the menu after the game over
  flow, through every hint dialog. On the web, the page check's own run and phase one's driver run (`?player=1&silent=1&driver=move`)
  play through DOM events.

**Focus in full screen.** `play.py --full` puts the window in full screen with Alt+Enter, and in the middle of each game sends a
focus loss and a regain: the game pauses (paused 1 after the loss) and goes on (0 after the regain), and all three games ended
normally.

<!-- private -->
**What origrun would need to hold this original at an update** (spec for the shared tool). This build of the framework has no
`mUpdateAppState` and no pending-draw flag: the byte at app+0x45a (0x4f68ba) is the "loaded" flag the loading screen waits for
(set once, 1 for good), and app+0x459 is its "load started" flag, so `--pending-draw` and `--draw-empty` cannot be given. The
update step is the function at 0x469593 (the counter at app+0x3f0 = 0x4f6850 is incremented at 0x46966f, after which the step
calls 0x4302eb and ends with a jump to 0x458676); its vtable slot is at 0x4c7c38 (slot of the next function, 0x4696a7, at
0x4c7c48). What origrun needs for a shot that shows update N is the address of the function that draws a frame to the screen (the
caller of the drawing after an update, not found: nothing calls 0x469593 directly, it is virtual): with its address, the shot
would be taken when that function returns after the counter has read N. A hook on the instruction after 0x46966f would give "the
update N has run" but not "has been drawn".
<!-- /private -->

**Web.** The web build under Node played the same tours as the native one (the log, and its shots compared with the native port's):
the only differences are sub-pixel edges of rotated tiles and of the head, which Emscripten's software drawing and the native build
draw a fraction apart. Not chased.

**Real input.** `games/rocketmania/tools/driver/play.py` plays whole games natively under rundesk with `-commands` and `-realinput`:
it asks `state` and `move` (the port's bot, `testhooks/Commands.cpp`: the play link, a dialog's button, the next mode button, the
clicks that turn a path across the board), sends each step as a real SDL event and, after 90 seconds of play, `lose`, so the game
over flow runs by real clicks too. Three games (arcade, classic, strategy) from launch through every hint dialog, windowed (231 s) and
with Alt+Enter to full screen (126 s) both finished at the menu. On the web, `?player=1&silent=1&driver=move` played 250 moves of
arcade through DOM events to level 2 (score 2727, no errors). Two faults of the test side were found: a bare `move` under
`-realinput` reached the framework as a mouse move (fixed on main by the coordinator in 4cf35432), and the bot's constructor
marked the input as scripted, which dropped every real click that came after a `move` (the command now keeps the flag as it was).

**Registration and links.** (Removal of 2026-11-08, listed in GAME_NOTES.md, "Registration code removed": the loading screen's thanks-for-registering line and its string, the register picture, the register-to-load dialog's id and check, `mIsRegistered` in the game's own code, and 33 registration texts of the game's text table; no visible change.) The port has no trial clock, no nag, no register, unlock or buy dialog, and the Options dialog has none of
the original's register, update, partner and web-page buttons; the loading bar's line under it is empty as it is in the registered
copy. There is no "More Games" button in the port.

**Loading.** The port loads in one go (the loading thread runs through, 1.7 s on the machine of the tests), and the play link shows
the update the game has loaded: the original looks at the thread's progress only every fourth update, the port at every one (a
deviation, `LoadingScreen::Update`; nothing else about the game's start changes).

**Fixes mod.** The phase found no behaviour of the original that is a bug changing how the game plays (scores, timing, rules) and
that the port keeps: the faults found were fixed in the port as they broke play or were leaks (phase one's table). So the game has no
fixes mod yet and nothing is listed under "Fix candidates".

## Not done, and what remains

- **3D screens against the original.** The original in Windows Sandbox has no GPU, so every screen of it was compared in 2D (`Is3D=0`
  in both). The 3D game's screens are checked by the page check and by real-input play in 3D, not pixel by pixel; a comparison needs
  `orig.sh ... --sandbox-vgpu`, allowed only between 22:00 and 08:00.
- **The hint flags.** The original's registry has some hint flags set, the port starts with all clear; the bot clicks every hint
  dialog it sees, in both programs, and the results were the same, but a run with different hint flags was not compared.
- **The web build's screens.** The web build under Node played the tours (above, "Web"), and the page check and a real-input run
  passed; its shots were not compared with the original again after this phase's fixes.
- **The original's links.** The More Games button is out of the port, as the rule of 2026-11-07 asks; the original's own link buttons are never
  clicked.
- **After this step** (CLAUDE.md's order): the fixes mod (no candidates so far), the coverage check, the performance pass, taking out code
  the game does not need, the readability pass, and the check against the PopCap Game Collection version.
