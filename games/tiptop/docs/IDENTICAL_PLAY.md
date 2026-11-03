# TipTop Deluxe: the identical-play pass

This is phase one of the pass that CLAUDE.md describes: **plays identical**. Given the same input, the original and the port
play the same games, judged by the game's state rather than by pictures. Phase two has not started: screens with shotdiff,
sound, focus and the web build.

The original is the installed `TipTop.exe`. It runs only through `tools/origrun`, from a copy under a short path
(`C:\byo-og\tt\game`, staged by `games/tiptop/tools/identical/setup.sh`), under rundesk, which keeps it invisible, silent
and at below-normal priority. It runs on origrun's virtual clock (`--clock virtual`) with the scripted cursor
(`--cursor scripted`), because TipTop reads the pointer with `GetCursorPos`. The port runs with `-fast`. Only one game
process ran at a time.

## How both sides are driven

**One input for both.** The port's bot (`testhooks/Autoplay.cpp`) plays through the mouse alone when given
`-autoplayui`. It clicks the menu buttons, the mode's button on the new game screen, each dialog's default button and the
board's columns, and it types its name for the high score. Everything it does goes to `-autoplayscript=<file>` as
`-script` lines. `tools/identical/toorig.py` turns that file into an origrun script. A port line `K` becomes `at K-1`,
because the original reads an input posted at counter N in the pass that the port numbers N+1. Replaying the file in the
port (`-script`) gives the bot's own state log exactly, so the file is the whole input. The bot decides with its own
random numbers from the board it sees, so each bot game tests the whole game.

**One start and one clock.** The copy is patched statically through origrun's patch list (`tools/origrun/patches/tiptop.txt`,
git-ignored; `setup.sh` writes it). No Windows function is hooked beyond the clock, the waits and `GetCursorPos`, which the
user has allowed. These are the patches:

- The start's `srand(GetTickCount())` takes origrun's `--seed`, as the port's `-seed` does.
- The music is not started. The original sends it to Windows' MIDI mapper, which origrun's silent sound does not cover.
- The run count's registry write is skipped.
- The main loop waits 10 ms a pass in place of the vertical blank, and it counts its passes at 0x48cff0, the counter that
  origrun's `--counter` reads.
- LOWOS's performance counter reads that count as 10 ms a pass. Without this, origrun's virtual clock moves on at each read
  past 200 in a pass, and TipTop reads it up to about 500 times, so its clock ran about 9% fast. With the patch, every
  read in a pass sees the same time, just as the port's clock under `-fast` moves 10 ms an update. The game reseeds `rand`
  from its own clock when it picks the kinds of blocks, so the two clocks have to agree to the millisecond, and now they do.
- `rand` also copies its state and counts its draws through a small code cave. The CRT keeps `rand`'s state per thread,
  which a watch cannot reach. The port's state log has the same two values (`gRandState`, `gRandDraws`, test hooks only).
- The copy plays as the registered game, as the port does.

<!-- private -->
The registered path: the configuration's DisableUnregistered byte (0x489b48) is read as set at its three reads (0x411d13,
0x412085, 0x412535). That also keeps the game from writing its run count into a file in the Windows folder. Because
`setup.sh` holds these patches, it belongs in `.publishignore`.
<!-- /private -->

**What is compared.** `tools/identical/case.sh <name> <mode> <bot seed> <game seed> <games> [port arguments]` does four
things:

1. It runs the bot in the port with `-statelog`, which logs every change of 35 values with the update it came at.
2. It converts the script.
3. It runs the original on that script once per four values, the most origrun can watch at a time.
4. It compares each value change by change with `cmp.py`. The original's watch at update N is the port's state after
   update N-1.

These are the values:

- the screen, the game mode, `rand`'s state and its draw count;
- the score, the level, the blocks left, game over, level complete and the ending;
- the two next blocks and the field's next block, the lowest and highest rows, and the pending spawns and floaters;
- the best chain and best match, the total time and the level time, and the spawn delay;
- the pause flags, the bonus round flag and the busy count;
- the queue's index and its shifting flag, and the field's fall and spawn locks;
- the first four scores of the mode's high score table and the first bytes of its top name.

The original's run is killed at its timeout, so it never writes `Preferences.dat`. That is why the table is read from
memory (0x4884b0) rather than compared in the file.

Before the first game, the board holds leftovers: the original's uninitialised heap against the port's zeroes. Those
values are compared from the first game's start. Three of them (the spawn delay, the queue index and the shifting flag)
keep the original's leftover until the game first writes them, so the original's value at the start is taken as the
port's. origrun logs at most 4000 changes of a watch; the clocks reach that in the longer games, and they are compared up
to the last change the original logged.

## Phase 1 results

Four cases, each one bot game or more, all mouse-played:

| Case | Mode | Bot seed, game seed | What it covers | Result |
| --- | --- | --- | --- | --- |
| reg2 | regular | 1, 1 | two games in a row (level 3, 9120; level 8, 50280), bonus rounds, two high scores | identical |
| str | strategy | 2, 3 | a strategy game to level 2, 4050 | identical |
| spd | speed | 3, 4 | a speed game to level 9, 64010, 830 moves | identical |
| save | regular | 2, 2, `-autoplaysave=30` | a game ended and saved after 30 moves, continued from the new game screen, played to level 5, 27810; a bonus round | identical |

In every case each value changed at the same update to the same value on both sides, through game over, the high score
dialog and the next game. The high score tables took the same entries at the same updates: in the regular
games, the saved game and the speed game, the bot's score went in and the lower scores moved down. The strategy game's
4050 earned no place in the first four.

The hidden timed mode (mode 1) was not played. Its button sits off the screen, so a mouse-played game cannot reach it.
The game has no skill levels.

**The one difference found was the harness.** In the speed game, the original made one tap fewer at update 10471, and
from there the two games went apart: the original was over at level 4 while the port went on to level 8. Bisecting by
update showed the following. The bot had clicked as a level ended, while the game was busy. The column cursor
(ColumnCursorProc, 0x4259b7) holds such a press and taps it as soon as the game is free (update 10470). The bot's next
click came one update later. origrun posts a scripted click while the game is held in the middle of a pass, after the
pass has read its input but before the board's update. So the original's held press, tapped in that pass, already saw
the cursor at the new click's column, and the new click's own tap a pass later went to the same column and was refused.
In the port, the held press tapped the old column and the new click tapped the new one.

Moving the port's cursor one update earlier in a replay (`-script` with an added `move`) made the port play the
original's game through game over. Since this is when the cursor moves relative to the pass, and not something in the
game, the bot no longer clicks while the column cursor holds a press. Before this, both sides' mouse queues
(0x4310f0-0x431ee0), the main loop's input (0x411fff), Tap, CanPlace and the column cursor were read against the port,
and they agree.

## What was changed

The game's code needed no change. Phase one added the following, all test hooks:

- the state log (`-statelog`), with the `rand` mirror it reads;
- `-autoplayui`, the bot playing by mouse;
- `-autoplayscript`, the bot's input as a script;
- `-autoplaysave=N`, which ends the first game through End game after N moves, saves it and continues it;
- the handler table accessor (`HandlerTable`) that the bot uses to find buttons;
- the comparison scripts in `games/tiptop/tools/identical/`.

`tiptop::SRand` now seeds through the mirror, so the state log sees the seed.

## Frozen baseline

`build/baseline-tiptop/` holds the port as it stood when phase one was done. When it is there, `tools/check.sh tiptop`
(not the quick form) plays three of the cases above in both the current port and the baseline: the two regular games,
the strategy game, and the saved and continued game. It requires the same state logs, non-empty, from both, apart from the meter (see the oddities: it
changes from run to run of the same exe), and reports "baseline identical" or "BASELINE DIFFERS". The suite's own speed game
changed its reference result with this pass, because the bot now waits for a held press.

## Oddities (skipped)

- **The meter's start outside the timed mode.** The regular, strategy and speed level lists never set the meter's
  starting value, so the board's meter holds whatever the heap held: garbage in the original, other garbage in the port, and
  different garbage from one run of the same exe to the next.
  It matters only in the timed mode, which cannot be reached, and nothing reads it in the other modes. It is not
  compared. To see it, watch the board's +0x364 in a regular game. Matching it would mean copying the original's heap
  contents.
- **The board's leftovers before the first game.** The spawn delay (field +0x900), the queue index (board +0x348) and
  the shifting flag (board +0x344) are uninitialised until the game first writes them, as are the board's other values
  before a game starts. Every later value is the same.

## Deviations left

None in play. The port's deviations from the original, where they exist, are in the game's notes and are outside the
game's logic. Phase two will compare screens, sound, focus and the web build.
