# AstroPop Deluxe: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. With the same input, a series of games plays out the same in the
original and the port, by the game's state and not by pictures: 38 games, with all four pilots' weapons and levels up to 33, came
out identical. Phase two (screen by screen, sound, focus, the web build, the
real-input play of the whole game) is listed at the end and has not begun.

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
| a row came one update early after a brick was retargeted | the handlers of the events 0x10 and 0x11 count the fired bricks that sit at their place in the board's `m1a68`; the first translation counted in `m1a04` | the two handlers count in `m1a68` (`BoardEvents.cpp`) |
| a brick did not hear the event of arrival when it stood exactly on its target | five float compares of `Brick` (the arrival and the 423 test of `UpdateState1`, the `Listen(0)` test of `SetTargetY`, the -5 clamp of the wobble, the 9.0 test of the mutation timer) are `<=`, read as `<` | the five tests are `<=` (`Brick.cpp`) |
| a dying brick left its state one update too early or late | `UpdateState3` steps the death animation and the effect animation, then `SetState(4)` and fires event 6 | translated as that |
| the mutator hold was set when it should not be (survival, seed 1, update 8496) | `Brick::OnDie` fires the event 0x1a ("popped") only for a mutated brick; a plain colour fires 0x27, a stone, steel or bomb its own events | no 0x1a for the other bricks |
| the pace after a spurt was always 8 | the byte at +0x4c8 of the board is the siren animation's `mStopped` (the animation sits at +0x4a0): with the siren stopped the pace is 4 and a spawn delay is drawn | the pace reads `mSirenAnim.mStopped`; the siren drawing and two sites of `BoardMode` read it too (classic, seed 2, update 7495) |
| a rainbow brick drew a colour without removing the one it had shown | `UpdateRainbow` first erases the shown colour from the list (0x405f70) and ends the effect when none is left | the same |
| a row came one update late (classic, seed 7, update 8186) | the original's x87 runs at 24 bits of precision (DirectDraw's setup never preserves the FPU): `m19cc + m930 * 20.0f` is 1046 there and 1045 in double | the expressions that the game truncates, compares or chains are computed in `float` (the wait for the next row, the danger display, the spawn thresholds, the ramp interpolation, the stage bonuses, the speeds of streaks and particles, the warp's speed); GAME_NOTES section 4 |
| a bonus round was only its wall of stones (classic, seed 11, update 16047) | a relative path of `XcfFile` may start with the name of the cursor's own section; the bonus file is read with `LEVEL\ROWn\Brickn` right after `Parse` | `XcfFile::FindValueSection` accepts it |
| Sprocket's weapon removed a brick too many (classic, seed 7, update 6369: 19 draws against 18) | the count of the buried bricks a weapon takes away is `(int) ((1.0f - share) * count)` in the x87 at 24 bits: 18 there, 19 in double | `BeginPlay` computes it in `float` (94adb53e) |
| (no run had shown it) the brick-o-meter's ratio and positions, the speed of a warp star | read after the difference above, for the same kind of expression: the meter's fill ratio `m1a14 / m1a18` is rounded to a float before it is compared with a third, two thirds and 1, and the positions that a milestone truncates to whole pixels are float arithmetic | the ratio and those positions are computed in `float` (`BoardFill`, `BoardGone`, `BoardEvents`, `BoardWarp`; 94adb53e) |

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

## Phase two, not begun

The screens one by one with `tools/shotdiff` (the New User dialog's line and height among them), the sound and the focus, the
web build, the Hall of Fame and credits screens (their star draws have not been compared), the x87 compares of the drawing and
screen code (GAME_NOTES section 9), the frozen baseline `build/baseline-astropop`, the fixes mod (or "none"), the performance
numbers and the whole game played with real input only, natively and in the browser, windowed and full screen, in 2D and 3D.
