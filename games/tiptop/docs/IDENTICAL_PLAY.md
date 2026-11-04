# TipTop Deluxe: the identical-play pass

This is the pass that CLAUDE.md describes. Phase one, **plays identical**: given the same input, the original and the port
play the same games, judged by the game's state rather than by pictures. Phase two, **looks identical** (at the end): the
screens with shotdiff, the sound and the music, the loss of the focus, and the web build.

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
game's logic. Phase two found none on the screen or in the sound either (below).

## Phase two: looks identical (2026-11-04)

**How.** The original ran inside Windows Sandbox (`tools/origrun --sandbox`) on the virtual clock with the scripted cursor,
from a copy staged in `build/origrun/ttsb-<tour>` with phase one's patch list. `games/tiptop/tools/shots/orig.sh <tour>`
runs a tour on it, `port.sh` runs the same tour in the port (`-script` with `screenshot` lines, `-fast`), and `cmp.sh`
runs `tools/shotdiff` and lists every pair that is not identical. A tour (`tour1.txt` to `tour6.txt`) is one script in the
port's format; `make.py` turns it into both sides' scripts. Tours 4 to 6 were made by `mktour.py` from phase one's bot
scripts, with shots added.

TipTop draws through DirectDraw, and inside the sandbox PrintWindow sees none of its picture, so origrun reads the frame
from the game's memory (`--surface-screen`). For that, a static patch has LOWOS's Blit (0x430190), which locks the screen
for its software blits, also store the locked pixels' address and pitch in the data section's zero tail (0x48cfe0,
0x48cfe4), and the frame is read while the game is held between two of its frames (`--surface-eip` on 0x41b9fe-0x41bbce).
Phase one's typed name never reached the original, which turned out to be two things in the copy's LOWOS, both patched:
the sandbox has no DirectInput keyboard, so the keyboard was switched off (0x431c66); and a hidden window never owns the
keyboard, so every poll found it "lost" and threw the typed characters away (0x4317b2). Input is posted only while the
game is held at its input reads or in its frame (`--input-eip`), so a click never lands halfway through a pass.

**Lining up the updates.** Three alignments were measured, not assumed. A click in the port's update K is posted at K-1 in
the original, as in phase one. A shot is taken at the same number on both sides: the frame read when the original's
counter shows N is the port's screenshot at update N (with K-1 the board's falling blocks were one frame apart). A focus
change also goes at the same number: at K-1 the "Paused!" picture's zoom was one update ahead in the original. And
origrun's click posts the move with the press, but the original reads the move a pass before the press: its rollover
sound and the board's column sound played at K-1. The port's side of every tour therefore moves the cursor at K-1 before
each click at K. That leaves the port's game unchanged (tour 2's state log is the same with and without the move), and
it makes every hover and sound fall on the original's update.

**Tours.** 1: the splash and its loading bar, the main menu and the hover of each button, the high scores and the clear
question (answered No), the options and the tips check box, all seven pages of the instructions and back, the credits,
the quit question (No) and the new game screen with its hovers (31 shots). 2: phase one's whole regular game (bot seed
1, game seed 1): the tips over the board, the board every 250 updates, both level summaries, the game over, the name
entry and the high score screen (54 shots). 3: a regular game's start with the tips, pause and unpause, the window losing
the focus at update 900 and getting it back at 1000, the options and instructions from the game, End game with its
question and the save, the new game screen with the saved game, continuing it and leaving it unsaved (24 shots). 4:
phase one's strategy game (bot seed 2, game seed 3), the board every 300 updates, the level summary and the game over
(22 shots). 5: phase one's saved game (bot seed 2, game seed 2): End game after 30 blocks, saved and continued, level 3's
summary and the bonus round after it, every 100 updates, to level 4's start (25 shots). 6: a speed game's first two
levels (phase one's case spd), the board every 200 updates and the level summary (16 shots).

In all, 172 shots, and **every pair is identical**: shotdiff flags no box in any tour. Until the alignments above were
in place, the flags were all timing (a falling block a frame apart, a column's glow lit a pass early, the "Paused!" zoom
an update ahead) or input (the name never typed), and none needed a change to the port. One box was the saved game's
date on the new game screen, when the two shots were taken on different days; tours are run on one day.

### Changes

The game's code needed no change. The port's test hooks gained `-buttonlog` (every control of the screen, with its box and
command, used to find the click points), `-soundlog` (every sound loaded, started and stopped, and the music's state)
and a getter for the music's pause count. The tools are in `games/tiptop/tools/shots/`; the original's new patches are
written by `tools/identical/setup.sh`. Nothing changed unmodded play, so the frozen baseline stands, and
`tools/check.sh tiptop` reports it identical.

### Oddities and masks

No mask was needed. Two things were seen and not chased, both in the harness rather than the game:

- **The loss of the focus on the host.** In the sandbox, losing and regaining the focus gives the same pictures as the
  port (tour 3). On the host, a "focus regain" at the start of a run made the original's tour 2 game go another way from
  update 790, its random draws coming five updates late, while without it every draw comes at the port's update. The host
  runs for sound therefore leave the focus alone (below).
- **The original's clock while it is inactive.** During tour 3's loss of the focus, origrun's virtual clock jumped ahead
  (by 515 s in one run and 751 s in another), though the update counter went on as before. The sound stand-in times
  sounds by that clock, so tour 3's later sounds are compared by order and by a constant shift.

### Sound

The original's sound was recorded on the host without playing (`shots/sound.sh`: origrun with `--sound wav`, whose
DirectSound stand-in logs every buffer it makes, every start and every stop, by the virtual clock), and the port's with
`-soundlog`. `soundcmp.py` names each sound by its place in the order of creation, checks that the sizes agree (all 36
do), and compares the starts and stops in order and by update. A test run's mixer does not play, so whether a sound is
still playing (which decides whether a stop happens and whether a "start unless playing" starts) is worked out from the
sound's length, as the stand-in works it out for the original.

Two patches were needed for this. LOWOS plays a sound only while the window is in front (0x43300c), which a hidden
window never is; the port asks only whether it is active, so the check is lifted. And with no patch the original would
send its music to Windows' MIDI mapper, which origrun's silence does not cover, so phase one never started it; for the
sound runs, the music starts and its player thread runs, but its calls to the MIDI device are skipped (the open answers
success with no device; a separate patch list, `tiptop-music.txt`). The music's state is then watched: its playing flag
(0x48a098), its pause count (0x489f08) and its volume (0x47e298), against the same values in the port's log.

| Tour | Starts | Stops | Music |
| --- | --- | --- | --- |
| 1 | 49 | 20 | starts at 430 |
| 2 | 346 | 115 | starts at 406 |
| 3 | 44 | 10 | starts at 406; paused 800-850 and while the focus is lost |
| 4 | 193 | 70 | starts at 406 |
| 5 | 473 | 152 | starts at 406 |
| 6 | 88 | 36 | starts at 406 |

Every start and stop is the same sound at the same update on both sides, and every run's RESULT line says the game made
no sound. The music's playing flag and volume change at the same updates. Its pause count differs by one update when the
focus is lost: the original's goes 0, 1, 2 over updates 899 and 900 (the window's message in one pass, the game's own
pause in the next), the port's goes 0 to 2 in update 900, and back to 0 at 999 against 1000. That is the same one-pass
offset of a focus change measured for the pictures; the music is silent either way, so nothing can be heard.

Two harness faults were found and fixed on the way. A script that ends early posts the close too soon, and after the
close the original plays nothing (tour 4's game-over sound was missing until the script was held to the tour's end); and
the stand-in's starts had first seemed to stop at update 1455, which was the focus line above sending the game another
way.

### The web build

Every tour was shot again from the web build under Node (`shots/web.sh`, `games/bejeweled/tools/verify/wasmrun.js`) and
compared with the native port's shots by shotdiff (`webcmp.sh`): all 172 pairs are identical. TipTop draws no rotated or
scaled sprites, so the web build's known 1 px offset on those does not arise. The web build's `-soundlog` on every tour
(`websound.sh`) has the same sounds, starts, stops and music states as the native port's.

### Not compared

The hidden timed mode, whose button lies off the screen (as in phase one). The sound and music volume sliders, which no
tour moves, and the full screen check box. The music's notes themselves: the original's player is a thread that runs on
real time with 20 ms sleeps, so its notes cannot be lined up with updates; its state is compared instead. The publisher
link on the options screen (command 0x37), which is never clicked. The Browser pane was not used: the web build was run
under Node.
