# Bejeweled 2 Deluxe: identical play, phase one (plays identical)

Phase one asks that, given the same input, the port and the original play the same: the same boards and random draws in the same
order, the same moves, scores, levels, timers and endings. This report covers that phase. It was started 2026-11-08; phase two
(looks identical: screens, sound, focus, the web build) has not begun.

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

## What was found to be identical

| Mode | 3D game | 2D game, 640 by 480 |
| --- | --- | --- |
| Classic | 40 moves, 2 levels, identical | 40 moves, 2 levels (score 1020), identical |
| Action | 40 moves, 3 levels, identical; then idle through the end of the time (20 moves, 9000 idle updates) identical | 40 moves, 3 levels (score 19950), identical |
| Puzzle | 40 moves of the first puzzle, identical | 40 moves of the first puzzle, identical |
| Endless | 40 moves, 25 levels (score 56350), identical | 40 moves, 27 levels, identical (with `nosparkle`, below) |
| Twilight | 40 moves, 2 levels, identical; idle for 9000 updates identical | 38 moves, 2 levels (score 972), through game over, identical |
| Hyper | 40 moves, 7 levels (score 254700), identical; 20 moves and then idle through game over and 700 updates more, identical | 40 moves, 9 levels (score 395750), identical |
| Cognito | 40 moves, 2 levels, identical | 40 moves, 2 levels (score 1805), identical |
| Finity | 19 moves, 16 levels, through game over, identical (the score runs past what 32 bits hold, in the original and the port alike) | 14 moves, 13 levels, through game over, identical |
| Hidden mode | 40 moves, 2 levels (score 940), identical | not compared: the original never started the mode in the runs made (see below) |

## What the pass changed in the port

| Difference | Cause in the original | What the port does now | Commit |
| --- | --- | --- | --- |
| After the carried-over flame gem of a later level, the next clear scored 450 where the original scored 200 | `Board::ExplodeAt` (`0x5a68b0`) and the shock wave of `UpdateEffects` (`0x5a7712`) test their accumulators with `>= 1.0` (the decompiler shows it as `!NAN(x) && 1.0 < x != (x == 1.0)`); the port had translated it as `> 1.0`. The tables of shard and explosion increments hold exact 1.0, 0.5 and 0.25, so a burst draws one more number in the original at the update where the sum is exactly 1.0. The global generator was then one draw behind, the next level change put the flame gem on another square, and a later clear exploded where the original's did not | the three accumulators and the shock wave progress test `>= 1.0` | e03dc153 |

The same audit looked at every other float comparison of that shape in the board, the effects overlay, Hyperspace, the main menu, the
star field and the puzzle overlay; the others already agreed with the original.

Nothing else of the game changed in this phase; the other commits of the pass are the comparison tools and test commands
(`origcmp/`, `state`'s new keys, the `nosparkle` command, `-trace`).

## Differences that stay, and why

These are listed under "Oddities" in `GAME_NOTES.md` with how to see each one.

The 2D comparison runs with `nosparkle`, which makes `Board::AddAlphaSparkle` return at once in both games (a patch in the original's
copy, a test command in the port): its pixel test reads the 640 by 480 scaled copies of the gem images that the original makes when it
starts at that size, and the port keeps one set of 1024 by 768 images and scales on the screen, so the pixels it finds, and so the
numbers it draws, differ. The 3D game needs no such step and was compared without it. `Hyperspace::Update2d` draws one number per star
line from `Rand() % mWidth`, and the width is 640 in the original at 640 by 480 and 1024 in the port's 2D (scaled down), so a 2D game in
which a jump takes place would draw differently from there on; no 2D run so far has shown it, and the 3D game, where both are 1024 wide,
needs no such care. In the 3D original a click on a dialog reaches the board one update later than in the
port, which the comparison allows for by giving those inputs one update early (the Escape key that closes a hint dialog excepted). The
original, started again and again, now and then never makes its board, or only makes it late, or ignores its first clicks; the
comparison runs it again when the board is late (a run that ended without a board or after the first moves is not a difference of the
game). Bejeweled 2's `Done3dTesting` is never called by the port (a deviation already in the notes).

## Not yet covered

The 2D game with the fix was run again in every mode but the hidden one, and all of them are identical to the original (the table); the
hidden mode at 640 by 480 was not: in the runs made the original never started the mode (its board only came after a stray click
of the driver, at update 2252), which the 3D game, given the same circle and the same welcome message, did not do; the cause was
not found, and what the port does there was not checked against the original. Long games of the modes that end only when the board
has no move (Classic's real ending), a puzzle pack beyond its first puzzles, a saved game resumed, and the web build under Node were
not played. Phase two is not begun.
