# Word Harmony: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. The same input gives the same games in the original and the
port, by the game's state, not by pictures. Phase two (screens, sound, focus, the web build) is not done.

The original is the game carved out of the wrapper (`re/wordharmony/bin/WordHarmonyGame.exe`, image base 0x400000, no
relocation), run only through `tools/origrun`, from a copy under a short path (`C:\byo-og\wh\game`, staged by
`games/wordharmony/tools/identical/setup.sh`), under rundesk, on origrun's virtual clock (`--clock virtual`) and with the
scripted cursor (`--cursor scripted`). One original and one port at a time at most; the original's registry key is cleared
before each run (`orig.sh`), so every run starts the way a first run does.

## How both sides are driven

**One input for both.** A case is a port `-script` file (`<update> move|click|rclick|down|up x y`, `<update> key <vk>`): the
port plays it with `-fast` (one frame an update, the engine's clock moved 33 ms a frame) and `toorig.py` turns it into an
origrun script (`at <update - 1> ...`: origrun's `at N` holds the original after frame N, so the input is read before frame
N+1, where the port reads its line K before frame K). The port logs its state before each frame too, so an input's effect
is in both logs under the same update.

**One start and one step.** The original seeds the C library's `rand` once, from `timeGetTime()`, in the game's init; the
seed patch puts the run's seed there (origrun's `--seed`), and the port's `-seed=N` is the same `srand(N)`. The game runs a
frame of 33 ms with an accumulator (0x4552b0) and takes the frame's elapsed time from `GetTickCount` (0x4542e0); on the
virtual clock that time is 32 to 37 ms when the main thread polls the clock (origrun moves a busy reader's clock on), so a
test patch makes it the period, 33 ms, as it is on a machine that keeps up. A second test patch counts the frames (the
update counter for origrun's `at` and `--watch`). All of them are in `tools/origrun/patches/wordharmony.txt` (git-ignored;
`setup.sh` writes it); none touches the trial or the registration (the carved exe has no wrapper to pass).

**The hint search on a thread.** The original starts its hint search (0x4191e0) on a thread (0x419560) and kills a running one
with `TerminateThread` when the board changes; on the virtual clock the main thread can kill it inside the heap's lock and
hang (frames 958 and 1446 of the first cases). A test patch runs the search at once instead, as the port does; its result is
the same, since the board does not change while the search runs on a machine that keeps up.

**Watched values.** origrun watches four expressions at a time, so a case runs the original nine times. The game is the
singleton at 0x4995b8, its ball system the pointer at +0x700: the generator's state (0x49729c), the game's state and next
state, its timer, score, level, mode, level template, time, level points, letters left and used, bonus, wild cards, hints
and hints used, the level's hints and time; the ball system's ball count, balls left, step time, spawn time, next slot, balls
to come, water and fill, hint index; and ball 0's position, state and letter and ball 3's position: 33 values. The port
writes the same names with `-statelog=<file>` (`WordHarmonyApp::LogState`, floats as their bits). `cmp.py` compares every
change of every value with its update; most of the game's fields are unset in the original until a game starts, so all but
the generator and the states are compared from the first level's start.

## Phase 1 results

Each row is one case on seed 1: all 33 values compared change by change with the update each came at, up to two updates
before the script's quit. The scripts are in `build/identical/scripts/` (git-ignored; they are a few lines each, listed in
the case's folder as `script.txt`).

| case | what is played | updates | result |
|---|---|---|---|
| tsu3 | TSUNAMI from the menu, level 1, three balls released, the water to the top: game over at 6774 and its screen | 7498 | all 33 the same |
| think1 | THINK from a saved game: a ball dragged over a ledge into a word, the word sent, the level's end and bonus count-up, level 2, Save/Quit, Save Game, the menu | 3398 | all 33 the same |
| relax1 | RELAX from the menu, levels 1-10, the help, nine letters dragged (WORD made, 800 points), Save/Quit, Save Game, RELAX again, Continue, two more drags | 4798 | 32 the same; the generator's state: three single draws one update late (Oddities) |
| hs1 | TSUNAMI at level 9 (the 25 000 bonus), game over at 2495, the name typed, the score table, then a second TSUNAMI game at level 1 with a click and a right click | 5398 | all 33 the same |
| contp | the RELAX game saved in relax1 continued: Continue, the help, the level, two drags | 1998 | 32 the same; three single draws one update late, as in relax1 |
| hint1 | TSUNAMI level 1 with Get Hint clicked three times and a right click (at 0 points the button gives no hint in either program) | 2098 | all 33 the same |

Also compared, apart from the cases:

- **A saved game.** Both programs played relax1 up to its Save Game. The original's registry value (`reg export` after the
  run) and the port's `registry.ini` value hold the same 1886 characters, so the port writes the original's format with the
  same numbers. The contp case then loaded that game in both programs (it is the same text whichever program wrote it).
- **The score table.** After hs1 the original's TSUNAMI table and the port's are the same: the typed name with 25 000 on top
  and the five default names and scores below it.

## What was changed

Each difference was bisected to the first value that differed and found in the export. The commits are 0fce54f (the frame
loop, `-seed`, `-statelog`, the first precision fixes), 6a3b37b (the first word's bonus, the rope, sounds on the engine's
clock) and 4428ab6 (the streams' ends, the drag's precision, the count-up):

- **The frame loop.** The port ran a frame per wall-clock tick; it now runs the original's accumulator of 33 ms frames, one
  frame an update with `-fast`.
- **x87 precision.** The original computes at double precision and keeps intermediates on the FPU stack, rounding to float
  only where it stores. The port now does the same in the particles' emission credit, the ball system's 0.03 s step, the
  water and the fill, the physics' accumulation and integration, the collisions (dy is squared unrounded times rounded:
  `fsts` then `fmuls`, in Collisions, Distance and Stick), the bounds, the boxes and ramps, the mouse's rope, Follow and the
  chains, and the bonus count-up at a level's end (its step is kept in double for both sums).
- **The first word's bonus.** The ball system's last match (`mLastMatch`) is never set before the first word; in the original
  it holds the heap's leftover bytes "backgrou" (from a freed "background" string), so the first word is never a repeat and
  always counts as speedy (twice the points). The port starts from those bytes.
- **When a sound ends.** The music layers draw from `rand` when a clip ends. The original plays a sound of up to 2 s from one
  DirectSound buffer, which ends after its length; a longer one streams through a 2 s buffer refilled a second at a time
  (0x444270), and it stops when the cursor leaves the half that took its last data: at the whole seconds of its length plus
  one. With `-fast` the port now ends a sound by the engine's clock by the same rule (`HostSound::isPlaying`); without it, by
  the mixer as before.

## Frozen baseline

`build/baseline-wordharmony/` (git-ignored: a copy of `wordharmony.exe` and its DLLs made when this pass was done) is checked
by `tools/check.sh wordharmony`: its THINK and TSUNAMI games (seed 1) run in both the current port and the baseline must write
the same non-empty state log, so later fixes and mods leave unmodded play as compared with the original.

## Oddities (skipped)

- **The virtual clock's crawl and a sound's start.** origrun moves the clock 10 µs on each read once the main thread has read
  it 200 times without waiting. A frame that reads it often (a level's load) therefore plays its sounds a few milliseconds
  past the frame's start on the original's clock, and a stream whose end falls within about 10 ms after a frame's start ends
  a frame later than in the port. It happened after a saved RELAX level's load (relax1: three single draws one update late, at
  4093, 4318 and 4591, the same in three runs; contp: at 1293, 1523 and 1796); every value besides the generator's state was
  the same, and the clips' ends in THINK and TSUNAMI fell on the same updates. To see it: the port's clip of 8.75 s starts at
  update 3819 of relax1 and, by the rule above, ends at 3819 + 9000/33 = 4091.7, so at 4092; the original's at 4093. It is the harness, not the game: in real play the sound runs
  on the sound card's clock and the frames on the accumulator, and no two machines agree to the update.
- **The hint thread.** See above: `TerminateThread` on a thread that may hold the heap's lock, which the test patch avoids.
- **The uninitialized last match.** See "The first word's bonus": the original reads memory it never wrote; the port
  reproduces what that memory holds on every run of the original.

## Notes on the harness

- origrun waits at most 30 s of wall time for an `at` line's update (`wait_counter(N, 30000)`), and then lets the game run free.
  A TSUNAMI game left alone for 6800 updates took longer than that, so tsu3 adds moves of the mouse to where it already is
  every 1500 updates (in both programs' input).
- Git Bash turns `reg`'s switches (`/f`, `/v`) into paths, so `orig.sh` runs `reg` with `MSYS2_ARG_CONV_EXCL`; before that fix
  the original's registry key was never cleared between runs.

## Deviations left

- Not compared in phase one: a hint actually shown (the cases had no points to pay for one), recycling letters, the options
  dialog, the help pages, the editor, a game cut off by quitting. The last four have no effect
  on the game's state; phase two looks at their screens; the first two are left for a case with points.
