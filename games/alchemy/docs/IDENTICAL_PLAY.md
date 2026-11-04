# Alchemy Deluxe: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. The same input gives the same games in the original and the
port, by the game's state, not by pictures. Phase two (screens, sound, focus, the web build) is described at the end.

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

## Frozen baseline

`build/baseline-alchemy/` (git-ignored: a copy of `alchemy.exe` and its DLLs made when this pass was done) is checked by
`tools/check.sh alchemy`: three mouse-played games (strategic, time trial, strategic at skill 2) run in both the current port and the
baseline must write the same state log, so later fixes and mods leave unmodded play as compared with the original.

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

## Phase two: looks identical (2026-11-03)

**How.** The original ran inside Windows Sandbox (`tools/origrun --sandbox`), on the virtual clock with the scripted cursor and the
seed patch, from a copy staged in `build/origrun/alsb-<tour>`; `games/alchemy/tools/shots/orig.sh <tour>` runs one tour, `port.sh`
runs the same tour in the port (`-script`, `screenshot` lines, `-fast`), and `cmp.sh` runs `tools/shotdiff` and prints one line per
differing pair. A tour (`tour1.txt` to `tour7.txt`) is one script in the port's format with `screenshot <name>` lines; `make.py`
turns it into the port's script and into origrun's (`at <update - 1>`, as in phase one; a line marked `@orig` or `@port` is for one side
only). Every tour starts at update 3000, after the original's loading thread (which takes real time, so a title shot at update
2990 can still show the loading bar in a slow run: tour 6's did once; a rerun matched). The "Updates" question is answered first,
as in phase one.

**Tours.** 1: title (with the link lit), selector (skill and mode buttons, lit and chosen), options, help, high scores (both tabs),
the quit question and "No". 2: a whole strategic game (40 shots: a grid of boards, the three board-cleared panels, the forge warning,
the game over, the name entry, the score table, the quit question). 3: a whole time trial game with bad cells and hint or pause
inputs (44 shots: the time trial's pause, the hint, the window losing and regaining the focus twice, the game over, the name entry).
4: the forge's bubbles every four updates at the end of tour 2's game. 5: the tutorial, every step at its start and forty updates
later (35 shots). 6: a saved game: the continue question, the loaded board, the side panel's new game question, options, help, mute
and discard, the quit question (17 shots). 7: six of the wizard's messages for a piece on a bad cell, before and after the answer.
In all, 167 shots; every pair matches or differs only faintly except the items below.

**The C library's rand, every update.** The original's `_rand` keeps its state in the thread's data, which origrun cannot watch, so
`tools/origrun/patches/alchemy-rand.txt` (git-ignored, written by hand from `identical/randcase.sh`'s notes) jumps from `_rand`'s
store into a cave in the padding of `.text` that also writes the state to 0x54f800, and `randcase.sh` compares that value after every
update with the port's `rand` field of `-statelog`. Two whole games (143 and 830 pieces: 733 and 4641 changes) gave the same state at
every update, so the effects' draws (sparkles, bubbles, popups) are in the original's order.

### Changes

| Difference | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| A newly lit tile dissolved in with a smooth round edge; the original's has a dotted one | The order in which the 2704 pixels of a tile light up is a table in the exe (0x4de400: 45 groups of pixel numbers, 2749 values); the port had invented an order from a fixed seed | The table is read from the player's own `Alchemy.exe` like the default score names (address, count and hash in `GameText.cpp`, no data in the repository); the invented order is the fallback without the exe | 1629d18 |

After it tour 2 has no visible difference. The change is to what is drawn, not to play: the frozen baseline's state logs are unchanged.

### Oddities and masks

- **The options dialog.** The original's holds two more buttons, "Check for Updates" and "Play More Great Games!", below High Scores
  (partner.xml's update and web settings); the port omits them on purpose (update checks and links to the web are not ported, `Dialogs.cpp`
  at 0x40de70), so the dialog is 0x44 shorter and its DONE button sits higher: a tour clicks DONE at 395,338 in the original and 395,270 in
  the port (`@orig` and `@port` lines). Never click the original's two new buttons; the first starts an update check.
- **The fullscreen box.** The port's options show the box ticked (partner.xml says `DefaultWindowed` false), the original's unticked: the
  sandbox has no 16-bit full screen, so the original falls back to a window. A test environment difference.
- **The forge's rim.** A faint 14-pixel edge difference (largest change 47 of 255) on the forge's oval clip in every shot; not visible.
- **The forge's bubbles in a held shot.** A shot of the original shows update N or an earlier one (`landed.tsv` says "or-earlier": the
  game has no pending-draw flag origrun knows). A frame taken one update early puts a few bubbles a pixel or two off; at updates where the
  frame is exact (15160, 15180, 15200 of tour 4) the two sides match, and the rand state matches at every update.
- **The wizard on the options and score dialogs** moves by its own clock; no shot of it differs visibly.

### Sound

The original plays through BASS 1.x, whose software mixing writes music and samples into one DirectSound stream, but each sample play makes
a duplicate buffer in origrun's stand-in, which logs it (`--sound wav`; `shots/sound_orig.sh`). `soundcmp.py` names each play by the
size and rate of the buffer it duplicates (set against the port's load lines) and compares it, with its update (virtual ms / 10), with the
port's `BYOPOP_SOUNDLOG`. **A game's window must be active first:** an original that never got the focus plays no sample (the app mutes while
inactive), so the host script starts with `focus regain`.

Over tours 2, 3, 5, 6 and 7 the two sides made **the same 2378 sample plays in the same order, with the same sample, at the same
update** (within two). The music was compared on the tutorial tour: the intro's fade out (updates 3001 to 3250) and the play song's
fade in (3001 to 3347, to the same volume) have the same updates and lengths on both sides. The original's "bad" sample is a 16384-byte
buffer in the stand-in's log where the port's load line says 46080 bytes (`soundcmp.py` maps it by hand). A logging `bass.dll` (as Mahjong's
`bassproxy`) was tried and dropped: Alchemy's 2003 BASS answers `BASS_Init` with error 23 when it is loaded as `bass_real.dll` behind a proxy.

### The web build

`shots/web.sh` runs `build/web/alchemy.js` under Node (`wasmrun.js game`, `-fast`, the same script), and `webcmp.sh` sets its shots against
the native port's. The whole strategic game (tour 2, 40 shots), the whole time trial with its focus losses (tour 3, 44), the tutorial (35)
and the wizard's messages (14) are **pixel for pixel identical to the native port's**, nothing faint. Tour 1 and tour 6 differ only
in the options dialog: the web build starts in a window, so its Fullscreen box is unticked (the web host writes the setting off, as in every game's
web build), and a scripted click on the options dialog's DONE button right after the high scores' DONE button (tour 1) or after
the help screen (tour 6) is lost on the web, where the same script works natively; a second click an update later closes it, and the
dialogs that follow are the same. This is listed as an oddity: the framework's script clicks are not what a mouse sends, the cause was not
found in the time given, and it was not seen with a mouse.

### Not compared

The loading screen (its length follows the real time of the original's loading thread, so a shot of its bar cannot be lined up), the
wizard's other messages (skull, four-way, no moves: the same dialog and text properties as the six compared), the web build's music in a
browser, and the time trial's score table screens.
