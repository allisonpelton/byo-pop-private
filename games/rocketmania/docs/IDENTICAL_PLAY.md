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
| A click on the arcade top strip dropped the next tile late | The click sets the drop timer to the whole delay (the next update drops the tile); after a launch it is the delay less 100 | As the original (`mFallTimer = mFallPeriod`; the launch keeps `mFallPeriod - 100`) | 4f774d4 |
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
screens are compared with `RM_REG="Is3D=0,ScreenMode=0"` (windowed, no 3D); the 3D game was compared on the host's desktop on the night
of 2026-11-12 (the next section). (2) shotdiff searches a global shift of up to 8 pixels to excuse a window frame
that is a pixel off, and it can excuse a whole screen that is truly moved: a dialog that was dragged 8 pixels came out as "shifted"
and its only box as a line of text. A pair's `shift` in `summary.json` is read now, and a nonzero one is judged; `lagscan.py` takes
its alignment from the label's own frame (it took it from the window's first frame, which could be another screen).

**3D screens (2026-11-12, overnight).** The original in Windows Sandbox has no GPU, so phase two compared it in 2D. The 3D game was compared on the
host's visible desktop (CLAUDE.md, "Overnight, desktop and sound allowed"): the copy of the original under `C:/byo-og/rm3d` ran with the stand-in's
silent sound and scripted cursor, `Is3D` 1 in its registry key (restored after each run; the game reads the flag at update 2708, "3D flag 1"), on the
wall clock, and `tools/shots/visrun3d.py` read the update counter with the process held, posted the tour's mouse messages and took each shot with
`PrintWindow` while the game was held. A shot is refused when another window covers the game's, so a run waits for a desktop that no other game's window
has been on for three minutes, and a refused shot counts as a failed run, which `build/ph2/night2.sh` repeated. The port ran the same steps with
`Is3D=1,ScreenMode=0` under rundesk (`mkport3d.py`, `lag3d.py`; the frozen exe of the build, `PORT_EXE`), an input of the original read at update C of
its counter being the port's script line C+1. On the real clock a held game goes on for a few updates before the shot is drawn (the passes that run two
updates at once, `docs/ORIGRUN.md`), so the port takes a shot at every update from C-1 to C+12 of each of the original's shots, and the frame that differs
from it in the fewest pixels is its pair (the offset is mostly +1 and +2, up to +11); `lag3d.py` also tries each frame moved sideways by up to three
columns (the board's shake moves the whole picture by that, see the oddities) and prints the shift it used. shotdiff compares the pairs (`lag3d.sh <tour>`,
run id `<tour>3dbest`). Two checks that shotdiff's boxes do not make: `tourstruct.py` counts the pixels that still differ after both pictures are blurred
by a 7x7 box (the sub-pixel noise of rotated and additive sprites averages out, a moved or missing piece does not), and `cells3d.py` names the board
cells that differ after the blur.

| tour | pairs | match | faint | differ | shots with more than 40 pixels that differ after the blur |
|---|---|---|---|---|---|
| u2 | 99 | 18 | 41 | 40 | 18 |
| t1 | 30 | 2 | 14 | 14 | 7 |
| t3 | 90 | 26 | 37 | 27 | 5 |
| t4 | 64 | 5 | 16 | 43 | 29 |
| t5 | 26 | 2 | 14 | 10 | 3 |
| t6 | 33 | 7 | 10 | 16 | 8 |
| t7 | 35 | 2 | 14 | 19 | 6 |
| hf1 (all hint flags set) | 62 | 3 | 22 | 37 | 21 |
| o2o (credits) | 4 | 0 | 0 | 4 | 4 (the Options dialog behind it) |
| o3o (Hardware Acceleration box, twice) | 11 | 5 | 2 | 4 | 4 (the Options dialog) |
| dd2 (drag the dialogs) | 4 | 2 | 2 | 0 | 0 |

In 3D shotdiff calls far more boxes "differ" than in 2D, because every rotated, stretched or additive sprite has edges that come out a little differently
(kind 3 below); the blurred count is the one that tells a piece that is wrong from an edge. Every shot with more than 40 pixels after the blur was looked
at, by its cells (`cells3d.py`), its regions and by eye, and each belongs to one of these kinds:

1. **Static pictures are identical.** Tiles, panels, text, dialogs, lanterns and rockets match pixel for pixel at the best frame in every tour; t5 has no
   board cell that differs after the blur. The Options dialog (o3o s300104, s300504) matches the original's in both halves at a vertical offset of +18 and
   -18 pixels (the removed More Games row), the credits dialog (o2o) and the screen behind it exactly except for the strip of the port's shorter Options
   dialog, and the Options dialog's 3D switch matches: after Done with the box unticked the original's screen is the software one and the port's is, at
   0 pixels (s300204 to s300400).
2. **The oddities of phase two, seen in 3D too.** The next tile's column jitter and the board's first column (the regions at x 192 to 304 of u2 s41350 to
   s41737, t6 s41350 to s41800, t1 s302982 and others), the particles (sparks of u2 s41523, t3 s71842, t7 s40450 and s40465; the explosions and rockets
   of u2 s45985 and s46000), the board's shake (u2 s45985 and s46000 are the best frames at a shift of two columns), the menu dragon (t3 s82000: its
   animation draws from the global generator, so its pose follows the draw count), and the tiles that swing after a click (u2 s41800 and t3 s41500: the swing's
   phase shows the click's timing to within an update; in s41800 the three tiles in the swing are each best at another offset, +2 and +9).
3. **Rotated, stretched and additive sprites differ at their edges** (the head's tilt, a tile that swings after a click, sparks, explosions and flames):
   a few hundred to a few thousand pixels a shot, correlated 0.95 to 0.99 with the original, a sub-pixel shift of about +0.5 to +0.75 pixel in x at best.
   Listed under the oddities.
4. **A click burst on the real clock** (t4 s43750 onward: five lower-right tiles are turned differently, the original shows a red path through them where
   the port shows white). Not the port: the state log of the port shows the clicks of the burst taking effect only on the updates the board allows, and
   delaying only those clicks (script updates 6396 to 6413) by one update gives the original's board (the cells that differ go from six to one; delays of
   -4 to +4 were tried), so the original read them one update later than C+1: the driver's log has the game at about 90 updates a second there (6 updates
   in 67 ms), a catch-up pass that runs two updates before the next messages are read. A uniform shift of all inputs changes nothing (it only moves the
   whole run in time), so this is a limit of the run, not of the port (the oddities).
5. **The Hardware Acceleration box, the one difference of the port (found by o3o, fixed).** The tour unticks the Options dialog's Hardware Acceleration
   box, shows the game switching to the software screen, ticks it again and shows the switch back. The original lets the box be ticked again; the port
   answered "Not Supported: Hardware Acceleration cannot be enabled on this computer" at the second tick. The cause is in the shared
   framework, whose `Is3DAccelerationSupported` and `Is3DAccelerationRecommended` returned whether 3D is on now (`Is3DAccelerated`), so they turned
   false once the game had switched it off; the original's tester answers from the card (its result does not change with the game's choice). They now
   answer what the renderer can do (`DDInterface::mCan3D`), commit 1821bccf (`shared:`). The replay of o3o with the fix matches the original's dialogs at
   the same offsets (s300504 and s300554 identical in both halves, s300604 at 0 pixels, s300610 and s300700 faint with no blurred difference).
   The other games that call these two functions (AstroPop, Bejeweled 2 and Twist, the Bookworms, Typer Shark) take the change with the framework; their
   checks are the coordinator's to run (`tools/check.sh --changed`).

The first Options tour for the port (o1, written for the port's layout) clicked the original's More Games row and the Hardware Acceleration box when it was
played in the original, whose dialog has the row: the stand-in's launch guard failed the launch (the net log has "LAUNCH BLOCKED ... steam://publisher/popcap")
and nothing reached the machine. The tours played in the original now are written for its layout (o2o, o3o) and the port plays their twins with its own
coordinates (o2p, o3p; `lag3d.py` and `mkport3d.py` read them), so a tour never clicks a link in the original.

### The changes of this phase

The first group is c17b6535 (found by the u2 tour and the comparison of the tours' first shots), the second 150ae162, the last row 1821bccf (found by the 3D tour o3o).

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
| The Hardware Acceleration box could not be ticked again after it was unticked (1821bccf, shared) | the original's 3D tester answers from the card, so the box can be ticked again whatever the game has switched | `Is3DAccelerationSupported` and `Is3DAccelerationRecommended` answer what the renderer can do (`mCan3D`), not whether 3D is on now; o3o's dialogs and 3D switch match |

### Oddities and deviations that stay

- **The board's shake and the next tile's column jitter** (`Resize(CrtRand() % (mShakeAmount * 2 + 1) - mShakeAmount, ...)`, `CrtRand() % 3 - 1`) draw
  from the C runtime's generator, whose index follows the number of screen draws; a screen draws a different number of times in the
  original and the port (a shot's own timing, a window's repaint), so the shake of a frame and the jitter of the column at
  x about 249 never agree. The shake moves the board by a random amount of up to the shock's size, the jitter by one pixel; in
  play they are random in both programs. Not chased (the rule of 2026-11-01 on oddities): a match would take the generator's
  index of the original's run and a draw count that depends on the machine.
- **Particles** (sparks, bursts, fireworks) take their places from the global generator and differ for the same reason.
- **Fields the original leaves in the heap** (the board's `mFallPeriod` outside the arcade mode, the game-over kind before 150ae162 set it, the
  pieces' `mFromRow` and the like) and **raw heap pointers in the save file** (the piece's number, special and host): the original's
  saves differ from run to run in those bytes and in the clock, and the port's are the same kind. Every other byte of a save written by
  one program and read by the other is the same, and the high-score file of the same runs came out byte for byte identical.
- **The software screen's stretch (deviation).** In 2D the original stretches the head in its hole with an older routine of its own
  (0x44abeb); the port keeps SDL's bilinear stretch, which comes within 38 levels on 800 pixels. Not visible; the framework's 0x433ca0
  would be sharper than the original.
- **The original's loading takes more than 10000 updates in the sandbox** (a loading thread on the wall clock, updates on the virtual one), so a tour's
  first click belongs at update 40000 or later.
- **In 3D the edges of rotated, stretched and additive pictures differ from the original's** (2026-11-12). The head when it tilts, a tile that swings
  after a click (`DrawTilted`), the sparks, explosions and flames (additive strips) come out of the port a little differently from the original's Direct3D:
  a few hundred to a few thousand pixels a shot, each below the visible (the pictures correlate 0.95 to 0.99; the best sub-pixel shift of a tilted tile or
  the head is about +0.5 to +0.75 pixel in x), and the same pictures drawn without rotation, stretch or blending are identical pixel for pixel. The head
  differs more between the original's 2D and 3D (6457 pixels) than between the port's two (1674), so the port's GPU path is nearer the original's software one than
  the original's own Direct3D is. Not found after more than an hour: a half pixel added to `DrawTrianglesTex`'s vertices (the framework's comment says D3D7
  takes them without the half-pixel correction that Blt uses), and nearest sampling instead of bilinear, each made the difference no smaller. The Direct3D 7
  textured quads of the original go through the driver's own filter and rounding, which the SDL renderer does not copy. Not chased (the rule of 2026-11-01).
- **The 3D run on the wall clock reads some inputs one update later than the update after the one it noted** (the harness, not the port). The driver holds
  the game, reads its counter C and posts the input, which the game reads at update C+1; when the game is in a pass that runs two updates
  (the log has 6 updates in 67 ms around the burst of t4), the input is read after both. Of a burst of six clicks on a falling board (t4, updates 6396 to
  6413) the original's board is reproduced only when those clicks, and only those, come one update later than the port's mapping, and in the arcade mode
  the tile a click turns depends on the update (`MouseUp` turns the tile that is under the pointer then). A uniform delay of every input of the run changes
  nothing (it moves the whole run in time), so a shot after such a burst does not match, and the first board difference of that run (t4 s43750) is
  followed by every later shot. The port's state log is not at fault: it takes the same inputs on the same updates as the script says. A driver that held the game only
  between two passes might not meet it (not tried); the run is not repeated for that.
- **The board's shake picks the wrong frame of the lag scan** (2026-11-12). The shake (above) moves the whole picture of a frame by up to the shock's
  size, so a frame at the right update can differ from the original's over the whole screen and the pixel count then prefers another update (hf1 s44606: +7
  with 52714 pixels, +2 with 11577 pixels when the same frames are taken in a run with fewer shots, the state log of the two runs the same line by line).
  `lag3d.py` prints the offset and the pixels of the best frame so that this shows, and it now also tries each frame moved by up to three columns (a shift costs
  200 pixels in the choice, so a frame that fits without one keeps its place); with that, u2 s45985 and s46000 pick the frame the shake fits (+2, shifted by two) and hf1 s44606 picks +2 shifted by one column (13664 pixels, 1652 after the blur: the explosion and the board's shake).

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

**Web.** The web build (`TARGET=rocketmania tools/build.sh web`, run under Node by `games/bejeweled/tools/verify/wasmrun.js`) played the
tours u2, t1 and t3 to t7 as the native port did, with a shot at every update of each window (`tools/shots/bestweb.sh <tour>`, the web
twin of `best.sh`; `RM_REG` sets the emulated registry the same way), and each tour's best frames went through shotdiff against the original's
shots, captured again for this check. 377 pairs: 136 match, 221 faint, 20 differ, against the native port's 136, 220 and 21 on the same
tours; every tour's counts are the native one's (t6 has one faint box more and one differing box fewer). The boxes that differ are the
ones the native port has (the board's shake, the column's jitter, particles; the two that the best-frame choice passed over match at the
shot's own update in the web build as well). A web frame against the native port's own differs only by the sub-pixel edges of the head and the
rotated tiles, as in phase one.

**Hint flags.** The original keeps one flag for each hint (`HintsDisabledFlag0` to `7`, the "don't show this again" box of each hint
dialog) and the Options box that disables hints and tooltips (`HintsDisabled`). The tours so far ran with all of them clear, so each hint dialog
was shown. Two runs set them: the tour hf1 (a bot's three games, one of each mode, with all eight flags set: no hint dialog comes up, the
games start at once) went through the sandbox's registry preset (`tools/shots/hf1.reg`, `orig.sh hf1 900 --reg ...`) and through the port's
`RM_REG`: 62 pairs, 13 match, 44 faint, 5 differ, and the five are a firework burst, three explosion flames and the board's shake with its
one-pixel shift (the pause shot of a strategy game). The Options dialog with `HintsDisabled` set (tour o1, `hf2.reg`) shows the box ticked in
both, with the same texts, boxes and sliders, and differs only by the More Games row, as in every o1 run.

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
that the port keeps: the faults found were fixed in the port as they broke play or were leaks (phase one's table). The 3D comparison added no candidate
either (its one fault, the Hardware Acceleration box, was a fault of the port and is fixed). So the game has no fixes mod to make and nothing is listed
under "Fix candidates".

## Not done, and what remains

- **3D screens in full screen.** The 3D comparison of 2026-11-12 was windowed (`ScreenMode=0`) on the host's desktop; the original's full-screen 3D
  was not compared pixel by pixel, only played by real input in the port (the four settings above). The sub-pixel edges of rotated and additive sprites in
  3D (the oddities) are the one visible-at-zoom difference that remains.
- **The original's links.** The More Games button is out of the port, as the rule of 2026-11-07 asks; the original's own link buttons are never
  clicked.
- **After this step** (the coordinator's order of 2026-11-12): the fixes mod (no candidates, so not needed), the performance pass, taking out code
  the game does not need, the readability pass, and the check against the PopCap Game Collection version.
