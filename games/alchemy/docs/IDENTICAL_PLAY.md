# Alchemy Deluxe: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. The same input gives the same games in the original and the
port, by the game's state, not by pictures. Phase two (screens, sound, focus, the web build) is not done.

The original is the game carved out of `Alchemy.exe` (`re/alchemy/bin/AlchemyGame.exe`, image base 0x400000), run only through
`tools/origrun`, from a copy under a short path (`C:\byo-og\alch\game`), under rundesk (invisible, silent, below normal
priority), on origrun's virtual clock (`--clock virtual`: the game is held between updates, so an input lands on an exact
update) and with the scripted cursor (`--cursor scripted`: the board reads the pointer with `GetCursorPos`). Two game
processes at a time at most (the original and the port); no shot is taken (origrun takes none on the host).

## How both sides are driven

**One input for both.** The port's bot (`testhooks/Autoplay.cpp`, `-autoplayui`) plays a game through the mouse only: it
clicks the title's link, the selector's mode and start buttons, the board's cells (a move, then a click on the cell where the
piece sits), right-clicks for the forge, clicks the board-cleared panel's button, types the name and clicks the score table's
button. What it does goes to `-autoplayscript=<file>` as `-script` lines (`<update> click|rclick|move x y`, `<update> char c`).
`games/alchemy/tools/identical/toorig.py` turns that file into an origrun script (`at <update - 1> ...`: the original reads
an input posted at counter N before update N+1, the port runs a script line before update K's widgets). A replay of the file
in the port (`-script`) gives the bot's own state log exactly, so the script is the whole input. The bot itself decides
with its own random numbers from the board it sees, and the board is the same on both sides, so one bot game is a test of the
whole game; the cases below use different bot seeds, game seeds and both kinds of game.

**One start.** The original's `srand(GetTickCount())` (call at 0x47198c in `SexyApp::Init`) is patched to a constant, origrun's
`--seed` with the one-line patch list `tools/origrun/patches/alchemy.txt` (git-ignored; `tools/identical/setup.sh` writes it); the
port's `-seed=N` is the same `srand(N)`. The board's own Mersenne twister is seeded from two of those draws, and the C library's
`rand` also feeds the effects (sparkles, bubbles), so the draws depend on the exact updates of the inputs: the pieces of every
game agree only if the inputs land on the same update everywhere, which they did.

**Two things the original does that the port does not, made equal on the original's side.** The original asks "Updates" (the
property `DontUpdate` is false in `partner.xml`, and `partner.xml.sig` forbids changing the file, so the question is answered, not
turned off): the script clicks No at update 310, long before the title's link is clicked at 450; the port has no such question
(update checks are not ported). The original keeps its score table and a saved game in its folder, and a run with `hiscores.dat`
from an earlier run offers a different name-entry message (the message depends on the best score of the day), so `orig.sh` removes
the file before each run; the port starts on an empty user folder.

**Mid-game inputs too.** Besides moves and forges the bot can put a piece on a cell it cannot go to (`-autoplaymiss=n`: the
wizard's messages), click the side panel's hint button and press a key, which pauses the game and another key goes on
(`-autoplayextra=n`), pick the skill level (`-autoplayskill=0..2`) and play several games in a row (`-autoplaygames=n`, each
new game draws from the generators where the last left them), and in the tutorial it clicks every step's button.

**Watched values.** The original's board is the pointer at 0x516f04 (the app global at 0x5160b0, +0xe54). origrun watches four
expressions at a time, so a case runs the original three times: state (+0x60), score (+0xd18), pieces placed (+0xaf0),
pieces forged (+0xaf4), boards cleared (+0xe0c), the piece to place's shape and colour (+0x68 and +0x6c of the piece at
+0xae8), the time trial's pause (+0xdf8), the forge-full count (+0xe00), the game-over timer (+0xd90) and the time-trial flag
(+0xaed), the multi-move count (+0xaf8), the run and the longest run (+0xe04, +0xe08), the pause (+0xdf4), the shapes,
colours, bomb and skull chances in play (+0xd98 to +0xda4), the elapsed and total seconds (+0xdec, +0xdf0), the number of cells
that fit (+0xd00), the last shape (+0xe60), the tutorial's flag, step, count and wait (+0xdd0 to +0xdd8, +0xe44) and the hint's
flag and timer (+0xe30, +0xe34): 29 values. The port writes the same names with `-statelog=<file>` ("update name value" on each change). `cmp.py` compares every
change of every value with its update.

## Phase 1 results

Each row is one game: all 29 values compared change by change with the update each came at (the number of changes is in
`build/identical/<case>/orig*.log`; a long game has 800 to 1200 score changes). **Every value of every case was the same.**

| case | game | bot seed, game seed | pieces | forged | boards | extra input |
|---|---|---|---|---|---|---|
| p1 | strategic, over at update 2165 | 7, 1 | 143 | 6 | 0 | name entry, score table |
| p2 | strategic, over at 12 655 | 1, 1 | 830 | 29 | 3 | |
| p3 | time trial, over at 15 090 | 1, 1 | 903 | 87 | 3 | |
| s2 | strategic, over at 7 310 | 5, 2 | 484 | 15 | 2 | |
| t3 | time trial, over at 18 615 | 9, 3 | 1138 | 89 | 5 | |
| k0 | strategic, skill 0, over at 6 813 | 3, 4 | 433 | 23 | 2 | skill button |
| k2 | strategic, skill 2, over at 918 | 3, 5 | 46 | 7 | 0 | skill button |
| m1 | strategic, over at 11 234 | 2, 6 | 703 | 18 | 2 | a piece on a bad cell one time in 6 |
| m2 | time trial, skill 1, over at 1 687 | 4, 7 | 77 | 22 | 0 | a bad cell one in 4; the wizard's box |
| x1 | strategic, over at 11 992 | 6, 9 | 482 | 31 | 2 | hint button or pause key before one piece in 5 (248 of them) |
| x2 | time trial, skill 0, over at 11 191 | 8, 10 | 520 | 33 | 2 | the same, one in 6 |
| g2 | three strategic games, skill 2 | 3, 8 | 86, 67, 66 | 22, 13, 16 | 0 | the second and third game's draws |
| tu | the tutorial | 1, 1 | | | | 16 steps, its forge and its buttons |

Also compared, apart from the cases:

- **A saved game.** The port played 1500 updates of a strategic game and wrote `savegame.dat` (3580 bytes); that file was put in
  the original's folder (`KEEP_SAVE=1 orig.sh`), and the original, started on the script that clicks Continue and plays on,
  loaded it and played 4500 updates more with the same score, moves and forges at every update as the port's second run on the
  same file. So the port's save is the original's format and restores the board, the generators and the timers alike.
- **The score table.** The file the original wrote after a game (x2) and the port's are the same 175 bytes, including the ten
  default names the port reads from the player's `Alchemy.exe`.

## What was changed

Nothing in the game: no difference was found, so the oracle-style baselines (`tools/check.sh alchemy`) are not refrozen. What
the pass added is the comparison: `testhooks/Autoplay.cpp` (`-autoplayui`, `-autoplayscript`, `-autoplaystart`,
`-autoplayskill`, `-autoplaymiss`, `-autoplayextra`) and `-statelog` in `AlchemyApp`, and `games/alchemy/tools/identical/`
(`setup.sh` stages the original, `case.sh` runs one case, `port.sh`, `orig.sh`, `toorig.py`, `cmp.py`). The bot's old
mode (direct calls, `tools/check.sh alchemy`) is unchanged. Commit 38eaccc, the ten default score names read from the player's
`Alchemy.exe`, goes in with this phase's merge.

## Oddities (skipped)

- **The original's "Updates" question.** It appears at the first start (about update 115) with `partner.xml`'s `DontUpdate` false and
  is modal, so every click is lost until it is answered. The file cannot be edited (a changed `partner.xml` fails its
  `partner.xml.sig`, and the game then ignores the whole file: the registration flag is off and the title shows its "register"
  link). `toorig.py` answers No. How to see it: run origrun with a click at the title and watch the widget manager's modal
  pointer, `[0x51639c]+0x4c`, which is not 0 from update 115.
- **The original's loading thread takes real time.** Its title link appears at update 44 to 156 depending on the run (the
  loading thread's waits take their real time on the virtual clock), so the first click is at update 450 in every script.
  Nothing after it depends on it: the same input at the same updates gave the same games in every case.

## Deviations left

- The port shows no "Updates" question (update checks are not ported, on purpose) and no registration or trial dialog.
- Not compared in phase one, on purpose: the original's saved game loaded by the port (`-importsaves` covers the format), the
  options dialog's sliders and full-screen box, the help, the quit and new-game questions, a game cut off by quitting.
  Their effects are not on the game's state; phase two looks at their screens.
