# AstroPop Deluxe: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. With the same input, a series of games plays out the same in the
original and the port, by the game's state and not by pictures. Phase two (screen by screen, sound, focus, the web build, the
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

A comparison counts only when the original has really been at every step of the script: when origrun reports "counter N not
reached within 30000 ms" (the machine was busy and the original ran slowly) the step was done at an earlier counter, and
`run_original.sh` makes the run again (the last step, the mark that ends the script, is allowed to be out of reach). The
original's watch log is raised to 400000 changes (origrun's default of 4000 cut a long game short once, and that looked like a
difference).

## What was compared

Each row is one comparison with the final build: identical in all four watches over the whole game.

| game | seed | moves played | last update | end of the game |
|---|---|---|---|---|
| classic | 2 | 60 (the limit) | 8433 | level 1, 12440 points |
| classic | 3 | 37 | 7929 | no move left (game over), 13580 points |
| classic | 1 | 100 (the limit) | 10962 | through the first warp, level 2, 104330 points |
| classic | 7 | 200 (the limit) | 15308 | through two warps and the story of level 3, 224810 points |
| classic | 11 | 221 | 16080 | level 3 and a bonus round, no move left, 297790 points |
| survival | 1 | 57 | 8604 | no move left, 44210 points |
| survival | 3 | 60 (the limit) | 8647 | 77600 points |
| survival | 4 | 100 | 9970 | no move left, 105650 points |
| survival | 5 | 152 | 12268 | no move left, 223110 points |
| survival | 12 | 81 | 9769 | no move left, 153060 points |
| survival | 14 | 57 | 8690 | no move left, 95220 points |

All eleven were run on the build that carries every change below (merged with the trunk of 2026-11-09), and each came out identical.

Not identical: classic, seed 13, 600 moves (see "Oddities skipped").

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

The tools changed as well: the comparison runs the original at least to the port's last update (the script used to end at the last
input plus 60), the original's run is repeated when it was too slow, its watch log is large, and the bot plays the gate between
two levels and the story screens (a game used to stand still at the first warp).

## Oddities skipped

Two. The original's loading on Windows 11 (GAME_NOTES, "Oddities") is worked around in the staged copy and not chased. And
classic, seed 13, 600 moves: from update 15579 the bot's take-and-fire loop stands still on 18 bricks in both games, and at
update 20512 the original's game generator stops moving for good while the port's keeps drawing 246 numbers for every hand brick, as
the original did for the 170 rounds before (the hand brick is built, its particles exist, no draw happens). The investigation
found the exact update, the generator's pointer and first word unchanged, the hand brick's fields as in the earlier rounds and
the framework's generator code as in the port; the cause was not found within the hour, and it is written under "Oddities" in
GAME_NOTES with what it would take. Reproduce it with `games/astropop/tools/origcmp/run_mode.sh classic 600 13`.

## Deviations that remain

See GAME_NOTES section 6. The loading is the machine's speed (the original's loading thread is run in one go); the bonus-file
check and the signature checks are left out.

## Phase two, not begun

The screens one by one with `tools/shotdiff` (the New User dialog's line and height among them), the sound and the focus, the
web build, the Hall of Fame and credits screens (their star draws have not been compared), the x87 compares of the drawing and
screen code (GAME_NOTES section 9), the frozen baseline `build/baseline-astropop`, the fixes mod (or "none"), the performance
numbers and the whole game played with real input only, natively and in the browser, windowed and full screen, in 2D and 3D.
