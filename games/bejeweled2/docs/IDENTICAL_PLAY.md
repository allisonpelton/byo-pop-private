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
| Endless | 40 moves, 25 levels (score 56350), identical | 40 moves, 27 levels, identical (with `nosparkle`, below) |
| Twilight | 40 moves, 2 levels, identical; idle for 9000 updates identical | 38 moves, 2 levels (score 972), through game over, identical |
| Hyper | 40 moves, 7 levels (score 254700), identical; 20 moves and then idle through game over and 700 updates more, identical | 40 moves, 9 levels (score 395750), identical |
| Cognito | 40 moves, 2 levels, identical | 40 moves, 2 levels (score 1805), identical |
| Finity | 19 moves, 16 levels, through game over, identical (the score runs past what 32 bits hold, in the original and the port alike) | 14 moves, 13 levels, through game over, identical |
| Hidden mode | 40 moves, 2 levels (score 940), identical | not compared: the original never started the mode in the runs made (see below) |
| Classic to its ending | 158 moves, 6 levels, through "no more moves" and game over (score 15631), identical | 174 moves, 5 levels, through game over (score 7741), identical |
| A game left and resumed | identical through the load and 22 more moves, then different (see "Differences that stay") | 25 moves, the menu button, Main Menu, Classic, "Load Game?" yes, 25 more moves over three levels (score 1649), identical; the global generator's position too, as far as origrun logs it (4,000 changes) |
| A saved game at level 10 | not run | a hyper cube swapped and 40 moves from a save made at level 10 with eight flame gems and three hyper cubes on its board, levels 10 to 13 (score 87797), 83 changes of the score, identical |

The saved game at level 10 was made for the two scores that the user doubted on the site (2026-11-10: a flame gem's blast
"5,500 on level 10", a hyper cube "2400 PER GEM on level 11"). Both are the game's own numbers. `properties/config.xml`, the
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

## Not yet covered

The 2D game with the fix was run again in every mode but the hidden one, and all of them are identical to the original (the table); the
hidden mode at 640 by 480 was not: in the runs made the original never started the mode (its board only came after a stray click
of the driver, at update 2252), which the 3D game, given the same circle and the same welcome message, did not do; the cause was
not found, and what the port does there was not checked against the original. A puzzle pack beyond its first puzzles was not played; the web
build was checked under Node in 2D only (the page's 3D game in the Browser pane is phase two's). Phase two is not begun.
