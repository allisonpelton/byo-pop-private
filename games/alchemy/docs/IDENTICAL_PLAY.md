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

**Watched values.** The original's board is the pointer at 0x516f04 (the app global at 0x5160b0, +0xe54). origrun watches four
expressions at a time, so a case runs the original three times: state (+0x60), score (+0xd18), pieces placed (+0xaf0),
pieces forged (+0xaf4), boards cleared (+0xe0c), the piece to place's shape and colour (+0x68 and +0x6c of the piece at
+0xae8), the time trial's pause (+0xdf8), the forge-full count (+0xe00), the game-over timer (+0xd90) and the time-trial flag
(+0xaed). The port writes the same names with `-statelog=<file>` ("update name value" on each change). `cmp.py` compares every
change of every value with its update.

## Phase 1 results

Each row is one game, every value of the list above compared change by change with its update.

| case | game | bot seed, game seed | pieces | forged | boards | result |
|---|---|---|---|---|---|---|
| p1 | strategic, game over at update 2165 | 7, 1 | 143 | 6 | 0 | all 11 values the same, name entry and the score table too |
| p2 | strategic, game over at 12 655 | 1, 1 | 830 | 29 | 3 | the same: 155 state changes, 806 scores, 832 moves |
| p3 | time trial, game over at 15 090 | 1, 1 | 903 | 87 | 3 | the same: 55 forge-full changes |
| s2 | strategic, over at 7 310 | 5, 2 | 484 | 15 | 2 | the same |
| t3 | time trial, over at 18 615 | 9, 3 | 1138 | 89 | 5 | the same |

The commands: `games/alchemy/tools/identical/case.sh <name> <strategic|timetrial> <bot seed> <game seed>`.

## What was changed

Nothing in the game's code so far: no difference was found in the cases above.

## Oddities (skipped)

None so far.

## Deviations left

- The port shows no "Updates" question (update checks are not ported, on purpose).
