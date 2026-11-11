# AstroPop Deluxe: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. With the same input, a series of games plays out the same in the
original and the port, by the game's state and not by pictures: 38 games, with all four pilots' weapons and levels up to 33, came
out identical. Phase two, **looks identical** (screen by screen, the sound, the focus, the web build and the whole game played with
real input), is the last section.

## How both sides are driven

**One input for both.** The port's own bot (the `move` test command, driven by `tools/playbymove` through real mouse and key
events in the `-hold` mode) plays a game from the title: it clicks the title's link, types the name `Test`, picks the mode,
clicks the pilot's start button and the help screen's button, then plays moves (a click takes bricks from a column, a right
click fires them; it also clicks the tutorial demo's link, the gate on the screen between two levels and the button of a story).
`games/astropop/tools/origcmp/record.py` runs the port that way and writes every input with the update it was posted at
(`events.json`). `to_origrun.py` turns that file into an origrun script (`at <update> click|rclick|move x y`), and
`run_original.sh` plays it on the original in Windows Sandbox on origrun's virtual clock (one update is 10 ms, an input lands on an
exact update), with the scripted cursor. `compare.py` compares the two logs. `run_mode.sh <mode> <moves> <seed>` does all of it.

**One start.** The original seeds the C library's generator from the clock at start; a one-line patch of the staged copy
(origrun's `--seed`; the patch list is git-ignored) makes it a constant, and the port's `-seed=N` is the same call. A second patch
makes the start of the loading thread call its procedure directly, so the original has loaded by the same update in every run.
On this machine (Windows 11, build 26300) the original cannot load its fonts: the three palette PNGs of the game are
stored as RGBA with the same pixels in the staged copy (`stage_original.sh`; see GAME_NOTES, "Oddities"). The port is run with the registry
value `Is3D=0`, as the original runs in the Sandbox (no Direct3D there).

**Four watches**, read by origrun in the original (`--watch`) and printed by the port's `-trace`: the board's score
(board +0x1974), the board's level (+0x1940), the position of the game generator (`[0x5a4e28]+0x9c0`, the Mersenne twister of the
game that the board, the bricks and the particles draw from) and the position of the C library's `Rand()` (0x5a6858; nothing
draws from it after the start, so it never changes). A game is identical when every change of each watch comes at the same
update with the same value. The generator's position is the strong one: every draw anywhere moves it, so two games that agree on
it have drawn the same numbers in the same updates.

**The pilots and their weapons.** A game of a few hundred moves never fills the weapon's charge (18000, and it only grows in
some states), so the weapon cases start with it full in both games: the port by the test command `set charge 18000`, sent with
the first move of the game, the original by four patches of the staged copy that store 18000.0 where the board's constructor and
`ResetState` store 0 (the patch list is git-ignored, like the seed patch). The bot then meets the box that explains the weapon
and clicks its button, starts the weapon with Return whenever it is charged, and plays it: the laser of Vector takes the bottom
brick of the tallest column, the psychic of Vixx takes the bricks lit under the pointer, and the weapons of Sprocket and Turbot
run by themselves while the bot waits. The pilot is the start button the script clicks on the pilot screen (`PILOT=n`).
Turbot and Vixx have to be earned, so their cases start both games with one player who has them: the port makes the profile
(`record.py --make-profile`: the file `users/data1.usr` and the player's registry keys), and the original gets the same file in
its staged copy and the keys as a registry preset of the sandbox (`PROFILE=file`, `ini_to_reg.py`, origrun's `--reg`). A player
is the file and the registry's `Users\<name>` keys together, and the original deletes every player when the registry's
`SGFVersion` is missing, so the preset carries that value too (the original keeps its settings under HKLM, the port in its own file).

**Later levels.** A game that the bot plays from the first level ends within the first four or five levels, so the later ones
start from the checkpoints. A player whose best level is 4n offers, on the pilot screen, a box with two gates: the first level and
the checkpoint, level 4n + 1. The profile of those cases has the best level of every pilot set to n times four (`BEST=n`), the
bot's driver clicks the checkpoint's gate (the port's `state` reports where the gates are), and the box that the warp screen shows
after every fourth level (the weapon's upgrade) is answered by a click on its button. Eight games start at levels 5, 9, 13, 17, 21,
25, 29 and 33 with different pilots and are played through the next level or more.

A comparison counts only when the original has really been at every step of the script: when origrun reports "counter N not
reached within 30000 ms" (the machine was busy and the original ran slowly) the step was done at an earlier counter, and
`run_original.sh` makes the run again (the last step, the mark that ends the script, is allowed to be out of reach: `compare.py`
then compares up to the update the original had reached and says so in its verdict). The original's watch log is raised to 400000
changes with `--watch-max`. That option did not reach a run in Windows Sandbox until the shared commit ea95b965: every sandbox run kept
origrun's default of 4000 changes per watch, the log of a long game stopped there and looked like a generator that stopped
(see "Oddities skipped"). `compare.py` now refuses a log with the line "4000 changes logged; no more", and it judges the update of
every change as well as its value.

## What was compared

Each row is one comparison with the final build (the trunk of 2026-11-11 merged): identical in all four watches over the whole game,
every change at the same update with the same value. 38 games in all. "Moves" is what the bot played before it stopped (the limit
given, or no scoring move left); the weapon's moves count among them.

**Plain games, pilot Vector, from the first level** (12):

| game | seed | moves | last update | end of the game |
|---|---|---|---|---|
| classic | 2 | 60 (the limit) | 8433 | level 1, 12440 points |
| classic | 1 | 100 (the limit) | 10962 | through the first warp, level 2, 104330 points |
| classic | 7 | 200 (the limit) | 15308 | through two warps and the story of level 3, 224810 points |
| classic | 3 | 300 (the limit) | 21155 | level 4, 436240 points |
| classic | 11 | 600 (the limit) | 32579 | level 6, with the bonus rounds on the way, 812620 points |
| classic | 13 | 600 (the limit) | 25323 | level 2, 120950 points (the game that was listed as an oddity) |
| survival | 1 | 60 (the limit) | 8784 | 113000 points |
| survival | 3 | 60 (the limit) | 8647 | 77600 points |
| survival | 4 | 150 (the limit) | 12088 | 121390 points |
| survival | 5 | 300 (the limit) | 18442 | 1288480 points |
| survival | 12 | 600 (the limit) | 24906 | 2571340 points |
| survival | 14 | 251 | 16012 | no move left, 137430 points |

**With the weapon charged, the four pilots** (18). The charge is full from the start in both games, the bot starts the weapon at
once and plays it. Vector's laser and Sprocket's marked bricks are the pilots every player has; Turbot and Vixx are open in the
profile both games start with.

| pilot | game | seed | moves | last update | end of the game |
|---|---|---|---|---|---|
| Vector | classic | 3 | 300 (the limit) | 8636 | 29090 points |
| Vector | classic | 7 | 200 (the limit) | 6966 | 600 points |
| Vector | classic | 11 | 260 | 8421 | no move left, 12130 points |
| Vector | survival | 4 | 150 (the limit) | 6797 | 480 points |
| Vector | survival | 5 | 300 (the limit) | 8838 | 27090 points |
| Sprocket | classic | 3 | 200 (the limit) | 13959 | level 2, 49160 points |
| Sprocket | classic | 7 | 200 (the limit) | 14124 | level 2, 47880 points |
| Sprocket | survival | 4 | 150 (the limit) | 12041 | 156060 points |
| Sprocket | survival | 5 | 296 | 18418 | no move left, 164520 points |
| Turbot | classic | 3 | 100 (the limit) | 10872 | level 2, 55420 points |
| Turbot | classic | 7 | 200 (the limit) | 15505 | level 3, 199070 points |
| Turbot | survival | 4 | 150 (the limit) | 11026 | 23350 points |
| Turbot | survival | 5 | 283 | 16902 | no move left, 124230 points |
| Vixx | classic | 3 | 100 (the limit) | 6888 | 380 points |
| Vixx | classic | 7 | 200 (the limit) | 10539 | level 2, 36960 points |
| Vixx | classic | 11 | 600 (the limit) | 31163 | level 6, 705630 points |
| Vixx | survival | 4 | 150 (the limit) | 7659 | 2060 points |
| Vixx | survival | 5 | 300 (the limit) | 13222 | 219720 points |

The weapon was started once in each of the 18 games (and the box that explains it answered). While a laser or a psychic runs the
bot plays nothing but its shots (97 to 257 of them in a game), so several of those games end early with few points; Sprocket's and
Turbot's weapons run by themselves and their games go on as plain ones. The generator changed between about 390 and 8700 times
in these games.

**Classic games from the checkpoints** (8): the game starts at level 5, 9, 13, 17, 21, 25, 29 or 33. There is no charge patch in
these: the weapon came by itself in each of them, was started once and played as above.

| start | pilot | seed | moves | last update | end of the game |
|---|---|---|---|---|---|
| level 5 | Vector | 21 | 600 (the limit) | 25764 | level 8, 813410 points |
| level 9 | Vector | 22 | 546 | 19817 | level 10, no move left, 201350 points |
| level 13 | Sprocket | 23 | 270 | 18712 | level 14, no move left, 180380 points |
| level 17 | Vector | 24 | 447 | 13998 | level 17, no move left, 65210 points |
| level 21 | Vixx | 25 | 368 | 15560 | level 21, no move left, 222490 points |
| level 25 | Vector | 26 | 299 | 10167 | level 25, no move left, 113070 points |
| level 29 | Turbot | 27 | 215 | 14290 | level 29, no move left, 91490 points |
| level 33 | Vector | 28 | 470 | 14863 | level 33, no move left, 94290 points |

Games that are not in the tables: the first rerun of survival 5 with Sprocket was compared up to update 18027 of 18418 because
the original ran slowly and the end mark of its script was out of reach; the game is in the table after a second run that
reached the end. Every other row was compared to its last update the first time.

## The changes the pass made

Every difference was found as the first update where the original and the port disagreed, and read in the original's export
(the asm decides: the decompiler's x87 comparisons are wrong in a way that matters, see GAME_NOTES section 4).

| difference | cause in the original | the port now |
|---|---|---|
| two draws came from different generators | the game draws from a generator of its own, seeded at the end of `Init` from the first generator, and the particles draw from it | `gGameRand` seeded the same way, drawn by the particles (the pass's first commit, a9124b4f) |
| a row came one update early after a brick was retargeted | the handlers of the events 0x10 and 0x11 count the fired bricks that sit at their place in the board's `mSupportedBrickCount`; the first translation counted in `mSlowGateStage` | the two handlers count in `mSupportedBrickCount` (`BoardEvents.cpp`) |
| a brick did not hear the event of arrival when it stood exactly on its target | five float compares of `Brick` (the arrival and the 423 test of `UpdateState1`, the `Listen(0)` test of `SetTargetY`, the -5 clamp of the wobble, the 9.0 test of the mutation timer) are `<=`, read as `<` | the five tests are `<=` (`Brick.cpp`) |
| a dying brick left its state one update too early or late | `UpdateState3` steps the death animation and the effect animation, then `SetState(4)` and fires event 6 | translated as that |
| the mutator hold was set when it should not be (survival, seed 1, update 8496) | `Brick::OnDie` fires the event 0x1a ("popped") only for a mutated brick; a plain colour fires 0x27, a stone, steel or bomb its own events | no 0x1a for the other bricks |
| the pace after a spurt was always 8 | the byte at +0x4c8 of the board is the siren animation's `mStopped` (the animation sits at +0x4a0): with the siren stopped the pace is 4 and a spawn delay is drawn | the pace reads `mSirenAnim.mStopped`; the siren drawing and two sites of `BoardMode` read it too (classic, seed 2, update 7495) |
| a rainbow brick drew a colour without removing the one it had shown | `UpdateRainbow` first erases the shown colour from the list (0x405f70) and ends the effect when none is left | the same |
| a row came one update late (classic, seed 7, update 8186) | the original's x87 runs at 24 bits of precision (DirectDraw's setup never preserves the FPU): `mRowInterval + mStackDanger * 20.0f` is 1046 there and 1045 in double | the expressions that the game truncates, compares or chains are computed in `float` (the wait for the next row, the danger display, the spawn thresholds, the ramp interpolation, the stage bonuses, the speeds of streaks and particles, the warp's speed); GAME_NOTES section 4 |
| a bonus round was only its wall of stones (classic, seed 11, update 16047) | a relative path of `XcfFile` may start with the name of the cursor's own section; the bonus file is read with `LEVEL\ROWn\Brickn` right after `Parse` | `XcfFile::FindValueSection` accepts it |
| Sprocket's weapon removed a brick too many (classic, seed 7, update 6369: 19 draws against 18) | the count of the buried bricks a weapon takes away is `(int) ((1.0f - share) * count)` in the x87 at 24 bits: 18 there, 19 in double | `BeginPlay` computes it in `float` (94adb53e) |
| (no run had shown it) the brick-o-meter's ratio and positions, the speed of a warp star | read after the difference above, for the same kind of expression: the meter's fill ratio `mStageBricksCleared / mBricksToPass` is rounded to a float before it is compared with a third, two thirds and 1, and the positions that a milestone truncates to whole pixels are float arithmetic | the ratio and those positions are computed in `float` (`BoardFill`, `BoardGone`, `BoardEvents`, `BoardWarp`; 94adb53e) |

GAME_NOTES section 4 holds the x87 reading of these rows. Once they were in, the four weapons, the box that explains one, the
upgrade boxes and levels up to 33 played out the same in the original and the port in all the games above.

The tools changed as well, and one of the changes was a fault of the test setup and not of the port: the comparison runs the
original at least to the port's last update (the script used to end at the last input plus 60), the original's run is repeated when
it was too slow or got no sandbox, the bot plays the gate between two levels, the story screens, the box of a weapon, the upgrade
box and the checkpoint's box (a game used to stand still at the first warp), and the test command `unlock all` wrote an unlock value
(0x57) that a profile refuses when it is loaded again; it writes 0x44 now (the port's own check was right).

## Oddities skipped

One: the original's loading on Windows 11 (GAME_NOTES, "Oddities") is worked around in the staged copy and not chased.

Withdrawn: classic, seed 13, 600 moves, which this report listed earlier as a game in which "the original's game generator stops
at update 20512". It does not. The 4000th change that origrun logged for the generator's position was the one at update 20511, and
origrun's log stops there because of its default cap, which the option `--watch-max` did not lift in a Windows Sandbox run (shared
commit ea95b965). With the option forwarded the game is identical to its last update, 25323, with 5118 changes of the position
(the classic game of seed 13 among the plain games). The comparison had taken a log that ended for a position that stopped; `compare.py`
now refuses such a log.

## Deviations that remain

See GAME_NOTES section 6. The loading is the machine's speed (the original's loading thread is run in one go); the bonus-file
check and the signature checks are left out.

## Phase two: looks identical

The second phase CLAUDE.md names: the screens, dialogs and texts, the sound, the focus, the web build and the whole game played with
real input only. Phase one had shown that the games play out the same; phase two found where the pictures, the sounds and the pages
did not, and fixed them (the changes are listed below). The numbers are from the final build, the native exe of commit `65795b0b`
(frozen as `build/baseline-astropop`) and the web build of the same sources.

### How the screens were compared

A **tour** is a script of inputs and shots (`games/astropop/tools/tour/`: `<update> click|rclick|move|down|up|key|type|focus|shot|portcmd`)
that both sides run with the same seed: the port natively under rundesk with `-fast -drawevery -realinput`, the original on origrun's
virtual clock in a Windows Sandbox copy (`tour.sh <name> <tour>`). The shots are compared by `tools/shotdiff`; `report.py` adds the
raw pixels that shotdiff hides as faint (differences below its 3x3 blur and 32 levels: it counts those over 8 levels and says where
they are), after lifting the original's capture by the 2 levels every origrun capture lacks. Four kinds of tour:

- **the menus**: `menus`, `options`, `halls`, `users` (the title, the new player box, the main menu with each button lit, the player
  list; the options box, its hints warning, the help box and its second page, the credits; the hall of fame, both pages, and the pilot
  screens with each pilot lit; the new user box with its error box, a second player and the delete box);
- **the start of a game** (`classic1` to `classic5`: the first 3,400 updates, the warp, the first bricks, the pop effect, each
  at one-update steps where the picture moves fast), **a game to level 3** (`classic6`), the pause and the focus (`pause1`: space,
  losing and regaining the focus, space again, Escape), the level-up text's line (`line1`), the in-game Options box and what it leads
  to (`ingame1`: each button lit, Reset Hints and its warning, Help, Credits, Main Menu and its End Game box), the Quit Game box
  (`quit1`), a survival game (`surv1`) and its end (`survend`), a classic game left to itself until the Emergency panel and ACTIVATE
  (`emerg1`), and a lost game with the hints off up to the end screen, a new game and out by the options box (`loss1`);
- **the four weapons** from a full charge: `w_vec`, `w_spr`, `w_tur`, `w_vix` (the weapons of Vector, Sprocket, Turbot and Vixx, up to
  300 moves, a shot every 100 updates; the weapon's box and its OK button included);
- **a game from each checkpoint** (`ck5`, `ck9`, ... `ck33`: levels 5, 9, 13, 17, 21, 25, 29 and 33 with different pilots, 200 moves
  each, a shot every 100 updates), which show every kind of brick, the weapon upgrades and the stories.

The port's weapon charge is set as the original's patch list has it (`portcmd set chargestart 18000`, GAME_NOTES 3b).

### The counts

For each tour: the pairs of shots, those that match to the pixel, those that differ only faintly (below what shotdiff shows a reviewer;
nearly all of these are the blit arithmetic below, up to about 22 levels) and those that differ, and of the last how many differ only in
the menu's right-hand column, which has no More Games panel in the port by design (GAME_NOTES section 6). The last column is the
web build against the native one on the same tour (`webshots.sh`: the tour's `-realinput` script run under Node on `build/web`, the
two sides' shots compared): the largest difference in any channel, in levels.

| tour | pairs | match | faint | differ | menu column | other | web against native |
|---|---|---|---|---|---|---|---|
| menus | 14 | 1 | 0 | 13 | 12 | a01_title | 6 |
| options | 18 | 4 | 0 | 14 | 14 | | 4 |
| halls | 15 | 12 | 0 | 3 | 3 | | 2 |
| users | 13 | 0 | 0 | 13 | 13 | | 5 |
| classic1 | 24 | 4 | 19 | 1 | 1 | | 9 |
| classic2 | 18 | 0 | 18 | 0 | 0 | | 9 |
| classic3 | 28 | 10 | 17 | 1 | 0 | k3335 | 8 |
| classic4 | 20 | 18 | 2 | 0 | 0 | | 5 |
| classic5 | 14 | 0 | 14 | 0 | 0 | | 8 |
| classic6 | 73 | 9 | 64 | 0 | 0 | | 9 |
| pause1 | 9 | 7 | 2 | 0 | 0 | | 7 |
| ingame1 | 29 | 25 | 0 | 4 | 4 | | 4 |
| quit1 | 7 | 0 | 0 | 7 | 7 | | 4 |
| line1 | 15 | 0 | 15 | 0 | 0 | | 7 |
| surv1 | 41 | 1 | 39 | 1 | 1 | | 8 |
| survend | 30 | 8 | 19 | 3 | 3 | | 211 (see below) |
| emerg1 | 30 | 3 | 27 | 0 | 0 | | 4 |
| loss1 | 34 | 18 | 14 | 2 | 2 | | 4 |
| w_vec | 26 | 4 | 21 | 1 | 1 | | |
| w_spr | 79 | 9 | 69 | 1 | 1 | | |
| w_tur | 95 | 21 | 74 | 0 | 0 | | |
| w_vix | 45 | 8 | 37 | 0 | 0 | | |
| ck5 | 6 | 2 | 4 | 0 | 0 | | |
| ck9 | 6 | 2 | 4 | 0 | 0 | | |
| ck13 | 63 | 4 | 59 | 0 | 0 | | |
| ck17 | 6 | 1 | 5 | 0 | 0 | | |
| ck21 | 23 | 1 | 22 | 0 | 0 | | |
| ck25 | 6 | 1 | 5 | 0 | 0 | | |
| ck29 | 44 | 3 | 41 | 0 | 0 | | |
| ck33 | 6 | 1 | 5 | 0 | 0 | | |

(The weapon and checkpoint games are native only; the web build plays the same bot's games in the driver runs below.)

**Every differing shot is judged.** All the shots counted as differing except two are the menu's right-hand column (a box of
210 x 326 pixels where the original has its More Games panel and the port stretches the other three: it shows in every screen that
has the menu behind it). The two others are timing of the harness and not of the game:

- `menus` a01_title: the title screen changes its picture (Vixx, Turbot, ...) every 600 updates of its own count. The port loads at
  the machine's speed (CLAUDE.md: loading is made fast in the port itself), so its title starts at another update than the original's
  and a shot at the fixed update 3000 lands on the other picture; the next shot, 70 updates later, matches.
- `classic3` k3335: a brick thrown up at the start of a game moves some 90 pixels in the update that the shot lands on, and the
  original's shot landed "or-earlier" (`orig.landed.tsv`: origrun's counter is read between frames). `classic4` takes a shot at every
  update of the same stretch (3322 to 3345) and 18 of its 20 match, the other two faintly.

**The faint shots.** After the fixes below, nothing in them is visible to a reviewer; the raw check finds three kinds of residue:

- the 2D blits: `DDImage::NormalBlt` (0x4ac1d0, 0x4b0f50) and `AdditiveBlt` (0x4ae3f0) of 2004 round the blend of a halo, a particle
  or a faded brick in another way than the framework's blit (the GPU's), by at most 11 levels (emerg1 g22200, where sparks and halos
  pile up; at most 7 in every other shot) and in every shot that blends anything (the original in the sandbox runs without Direct3D,
  as the port's tours do with `Is3D=0`). The port's 2D screen is the GPU's, whose blend rounds to nearest where the 2004 code truncates
  (`dest * (256 - a) + src * a >> 8` on the packed channels), so the shared code would need a software screen and the 2004 arithmetic
  of the two functions above, which have about 5,500 lines of x86 between them (16 and 32 bit, with and without a colour,
  transparent or not). It is not done (GAME_NOTES section 8);
- the ring of the atomic explosion, the Atomic Bomb weapon, drawn at twice the size of a cel with `DrawCelStretched`, differed at its
  edges in a few thousand pixels by up to 91 levels (w_spr g3700 91, g3800 47; ck13 g3650 49, g3800 47). The cause was not the
  stretch's arithmetic (the framework's `SlowStretchBlt` is the original's: the same 257 and 256 weights, the same factors) but the
  fast flag: the framework's `WidgetManager` sets `SetFastStretch` on every widget's `Graphics` in a 2D run, so the ring was
  stretched in 16.16 steps (each pixel twice over), while the 2004 `WidgetManager` sets no such flag (the only caller of
  `Graphics::SetFastStretch` 0x4af630 is the level-up text's own draw, 0x40c8a0) and the ring is averaged. Fixed by the shared option
  `gWidgetsDrawWithoutFastStretch` (framework/WidgetManager.h), off by default, which AstroPop turns on; the four shots now differ by
  at most 7 levels, the residue of the blits above;
- one or two single pixels (classic1 g4500 and classic6 g7048: two pixels at (563, 296) over 200 levels).

**Web against native.** Seventeen of the eighteen tours differ by at most 9 levels in any pixel; the largest count of pixels over 6
levels in any shot is 15 (surv1). The exception is `survend`: at four shots (g7600, g7900, g8500, g8600) a mutating brick's
biohazard mark, whose alpha pulses by 3 levels per update, is at another phase (a 25 x 24 box, up to 211 levels). A web run with
`fast=1` skips frames, so a shot lands some updates away from the native one; the pulse is the only thing in the tours that moves
slowly enough to tell.

### Sound and focus

**Sound effects** (`soundtour.sh`, `soundcmp.py`): the port's own sound log (`BYOPOP_SOUNDLOG`: every sample loaded and every play with
its volume, pan and loop) against origrun's `logs/sound.log` (the duplicates of loaded buffers and their play calls, volume and pan as
the last calls set them), the samples named by their size and rate. The recorded game of the tour `ck29` (the title, the Classic
button, a game from the level-29 checkpoint through to update 9,746, from update 3,100 on): **889 plays in each, the same sample, the
same volume, pan and loop for every one, and the same update to within the constant 3 updates by which the two logs' counters
differ** (478 of the plays at exactly 3, the others within 2): no difference. The comparison cannot tell apart samples of the same
size and rate (the mouse-over samples, two zaps), and it does not cover the music (a module stream, `music\music.mo3`): the music's
start, stop and loss of the focus were **not** compared with the original in this pass.

**Focus**: the tour `pause1` (space, the focus lost and regained, space, Escape) matches the original in 7 shots and faintly in 2, with
no differing shot, natively, and by at most 7 levels in the web build; the real-input runs below lose and regain the focus five moves
into a game natively (`paused=1`, then `paused=0`) and in the page (`blur` and `focus` events: the game pauses and runs on).

### Real input only

The port's own bot (the `move` test command) chooses every move; `games/astropop/tools/realplay.py` natively (posted mouse and
key events through the window, as `visrun` does, under rundesk, `-fast`) and `games/astropop/tools/web/onegame.js` in the page
(real DOM events on the canvas through the page's test kit, in the Browser pane with `?silent=1`, `requestAnimationFrame` shimmed)
send them. Each run starts at the title, clicks the link, types the player's name (a wrong letter and a Backspace first), clicks the
mode's button, the pilot, the help's button, plays, and with `--lose N` ends the game (the test command `lose`), clicks the end
screen's Continue, clears a hint box at the start of the new game if one is up, and leaves by the options box (Escape, Main Menu, Yes).
Nothing is played by a person; a screenshot is only a check of what is drawn.

| run | result |
|---|---|
| native classic, 120 moves, Vector | level 2, 170,240 points, 5,768 updates |
| native survival, 600 moves | a long game: 2,398,860 points, 18,870 updates, no error |
| native classic, 400 moves, then `lose` | level 4, 508,120 points; Continue starts a game at the checkpoint ("Continue from level 5?"), its hint box cleared, then the menu |
| native classic, Sprocket, 60 moves; Turbot and Vixx (a profile that has them), 60 moves each | levels 2, 2 and 1 (39,860, 31,240 and 7,380 points) |
| native, focus lost and regained after 5 moves | paused 1, then 0, play goes on |
| native, full screen (Alt+Enter, 1280 x 960 to 2560 x 1920 and back), classic and survival | both play 60 moves in full screen and leave it |
| native, 3D (Direct3D) classic and survival, 60 moves | both play (level 1, 12,260 and 73,780 points) |
| native, a classic and a survival game lost after 40 moves, Continue, Main Menu, Yes | the menu |
| web page, survival, 400 moves | the game ended (over) at move 304, Continue, the menu: no error |
| web page, classic, 500 moves, `lose`, canvas at 2 x (the pointer lands at scaled points as in full screen, which the pane cannot enter) | Continue from level 5, the hint box cleared, the menu; no error |
| web page, classic, 300 moves, `focus=1`, canvas at 2 x, `lose` | the game paused when the focus was lost and ran on when it came back; the menu |

The first runs of a deep game found that it continues from its checkpoint and opens the hint box of a new brick (a paused game): both
drivers now clear it with the bot's moves, as a player clicks its OK. That is the original's behaviour; there is no difference in the
game.

**The page** itself takes the shared page check (`tools/web/pagecheck.js` with the game's table `games/astropop/tools/web/pagecheck.js`:
focus against the native build, the keys the page keeps from the browser, the pointer at other sizes, a second copy, a reload mid-game): 11
pass, 0 fail and 1 information.

### The changes the phase made

| the difference | its cause in the original | what the port does now | commit |
|---|---|---|---|
| the New User box, the player list's scroll bar and the options labels | the original draws them with its own 2004 text writers | the same writers | 32455cd6 |
| the board's overlay (the fade, the pause, hint, weapon and Emergency panels), the demo link's colours, the level text's arithmetic | the asm of those routines | translated as the asm has them | cc6a03e9 |
| the ship entered from -313, the rank medal's size, the rank names and the last quit line | the original enters from -312 and computes the medal at 24 bits; the names and the line are in the exe and were not in the port's text table | -312, the 24-bit size, the names and the line read from the exe | c0fb4503 |
| a game saved at the warp screen put the ship at y 420 | 0x4687f3 stores 423 | 423 | 30de8f29 |
| a 2D run stretched with the GPU's pixel-centre sampling: the level-up text's line and 1-pixel edges were off | `DDImage::StretchBlt` 0x4c68c0 steps in 16.16 fixed point, the column first; its non-fast path averages into a copy | AstroPop's display stretches the same way in 2D mode | 5a640255 |
| the loader's tinted strip and the rank medal had the wrong alpha | `SlowStretchBlt` renormalizes the alpha | the same | 398a897e |
| the fireball's colour faded green first | `SetFadeBlue` (0x402880) sets the blue's step (+0x5c) | the blue's | f9dec516 |
| the Quit Game box had no goodbye line | the ten lines are read once the game texts are loaded, and the port's constructor ran before | read when the texts are loaded | c7cc784a |
| float positions, bars, streaks, the scenery and the text were drawn at another rounding | the x87 control word is set to 24 bits by `UpdateFrames` (0x408b20) and the drawing code computes in single precision | the port sets `_PC_24` in `UpdateFrames` and computes the drawing and screen code in `float` as the asm does (every `TEST AH` site of the audit in GAME_NOTES section 9 read) | 4a1b9e7b |
| the main menu has a More Games panel | a link out of the game | no panel: the three panels left are stretched to fill its place (GAME_NOTES section 6), the hidden hover sequence skips its step | 4a1b9e7b |
| the siren, the lightning pair, the upgrade head and the arm were upside down | 0x4cfb50 is `MirrorImage` (left to right), the port called `FlipImage` | `MirrorImage` | 65795b0b |
| the Fusion Warhead's atoms came from the wrong bricks, and a sprocket and a turbot overlay were never switched | the brick's event listener (vtable 0x55d500 slots 25 to 28) reads events 0x16, 0x17, 0x18, 0x19 as sprocket off, turbot off, sprocket on, turbot on | the four events do that | 65795b0b |
| the weapon's box was 400 high and its OK button silent | vftable 0x56b8b8 slot 0xc8 is 0x483b90: twice the picture's height plus 0x96 (370); the button plays the sample at +0x118, which is 0 and, in the original's alphabetical numbering, the one-second timer's tick | 370; the tick, asked for by name (the port numbers by manifest order); a fix candidate for the mod | 65795b0b |
| the atomic explosion's ring was stretched pixel by pixel (up to 91 levels at its edges) | the 2004 `WidgetManager` leaves the fast stretch off; only the level-up text asks for it | the option `gWidgetsDrawWithoutFastStretch` is on (shared commit) | c2219d33 |
| every spark of a low brick flew from another place (emerg1 and ck29: tens of shots) | `Brick::UpdateSparks` 0x406e20 hands the particle's constructor its second draw first and its first draw second; the other seven call sites do not | the same order (all eight call sites read against the asm; the draws, and so the games, were always the same) | 65795b0b |

The tools changed too: `gamerun.sh` plays a recorded game and takes its shots, `freeze.sh` copies the exe for a long tour,
`webshots.sh` runs a tour on the web build, `soundtour.sh` the sound, `gametour.py --charge` and `record.py --charge` send
`set chargestart`, and `realplay.py` and `onegame.js` are the real-input drivers.

### Oddities skipped and what still differs

GAME_NOTES section 8 has each one. In short: the 2D blit and stretch arithmetic of `DDImage` (above; shared code), the title's
slide timing and the original's early landing of a fast shot (harness), the biohazard pulse of a web run (frames skipped), and
the original's font loading on Windows 11 (worked around in the staged copy). Apart from the blends above (a few levels at the edge of a
halo, up to 91 levels at the edge of the explosion ring) nothing in a screen, a sound effect or a page differs in the tours.

### The fixes mod, the baseline, the rest

**Fixes mod:** one candidate, listed in GAME_NOTES section 7 (the weapon box's OK button ticks); the mod waits for
`docs/MOD_ROADMAP.md`'s turn. **Baseline:** `build/baseline-astropop` is the exe of `65795b0b`; `tools/check.sh astropop` plays a
classic game of 120 moves with real input, compares its state with the baseline's, and passes; `tools/check.sh build` passes.
**Registration code** was out before this phase (GAME_NOTES section 5); the More Games panel went with this one.
**Waiting for shared code:** the 2004 `DDImage` blit arithmetic (NormalBlt, AdditiveBlt) and its stretch (StretchBlt,
SlowStretchBlt), the two differences above; both specs are with the coordinator. **Not done in this pass:** the music's state against
the original's, the performance numbers, the readability pass and taking out the code the game does not need
(GAME_NOTES section 9). The coverage check came after this pass and found six more differences, all in screens and boxes that no
comparison had visited (GAME_NOTES section 10).
