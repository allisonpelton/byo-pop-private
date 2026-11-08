# Rocket Mania Deluxe: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. The same input gives the same games in the original and the
port, judged by the game's state and not by pictures. Phase two (screens, dialogs, sound, focus, the web build) is not done.

The original is the game carved out of `RocketMania.exe` (`re/rocketmania/bin/RocketManiaGame.exe`), run only through
`tools/origrun` from a copy under a short path (`C:\byo-og\rm\game`), under rundesk (invisible, silent, below normal priority),
on origrun's virtual clock (`--clock virtual`: the game is held between updates, so an input lands on an exact update) and with
the scripted cursor. One game process at a time, the original or the port, never both together. The port runs with `-fast
-drawevery`.

## How both sides are driven

**One input for both.** The port's bot (`src/testhooks/UiBot.cpp`, `UiBotPlay.cpp`; `-uibot=<seed>`) plays whole games through
the mouse only: the loading screen's link, the selector's mode button, the difficulty and continue dialogs, the hint dialog,
the board's tiles (left and right clicks, the arcade drag, the top strip that drops the next tile), the quit button, the game-over
dialog, the name box of the high-score table and its menu link. What it does is written as script lines (`-uiscript`), which
`tools/identical/toorig.py` turns into an origrun script (`at <update - 1> ...`: origrun reads an input at counter N between
updates N and N+1, the port runs a script line in update K before its widgets). Replaying that script reproduces the bot's own
state log, so the script is the whole input. It never clicks a link button of the Options dialog or a "buy" or "more games" screen.

**One start.** The original's `srand(GetTickCount())` in `SexyAppBase::Init` is patched to a constant (origrun `--seed`, a
one-line patch list that `tools/identical/setup.sh` writes); the port's `-seed=N` is the same `srand(N)`.

**The values.** `tools/identical/values.txt` names 28 values by their address in the original (origrun `--watch`, four per run,
so the original runs once per four values) and by the board's member in the port (`-statelog`, which writes every change of
each value with the update it came at): the mode and difficulty, the board's update counter, state, score, level, the rockets
left and wanted, the moves left, coins, the drop timer, the fuse and its countdown, the burn path, the flames lit, the tiles
dropped, the pause count, the end-of-level timer, both generators' index and first word (the board's and the global one), the
effects count and the three launch counters. `values_grid.txt` adds the rotation and the kind of each of the 54 cells,
`values_right.txt` the number of each launcher on the right edge, `values_more.txt` the moves left, the strategy countdown and the
size of the list of moving pieces. `cmp.py` compares them change by change; `first.py` finds the first update at which a
value differs; `window.py` prints a window of updates, both sides side by side.

## What was compared

Every comparison below ends with every value the same at every update, except for the oddities listed at the end.

| Case | What it covers |
| --- | --- |
| c1, c2, c3 | One game each of classic (difficulty 2), arcade (2) and strategy (3), 3000 board updates, quit by the quit button |
| a1, a3, k1, k3, s1, s2 | Difficulties 1 and 3 of arcade and classic, 1 and 2 of strategy, 4000 updates |
| m3 | Three games in a row: arcade, classic, strategy, with the selector in between |
| g0, g1, g2 | Difficulty 3, two games each of arcade, classic and strategy played until they ended by themselves (about 60,000 updates a game): rockets launched, levels, buying rockets, game over, the high-score table with its name entry, a second game after it |
| h0, h1, h2 | The same at difficulty 1 |
| h3 | Three games in a row (arcade, classic, strategy) at difficulty 2 |
| cont1 to cont4 | A saved game written by one program and continued by both: classic written by the port, classic written by the original, arcade written by the port, strategy written by the original. Both programs read the same file and play the same input on from its Continue button |

The two programs' saved games have the same size in every case; the file contents are not compared byte for byte (they hold heap
pointers and uninitialised fields).

`-fast` is checked: a real-time run of the classic game without `-fast` and `-drawevery` gives the same script and the same
values, except for the global generator's index (the number of draws the menu's falling leaves make depends on how often the
screen is drawn; under origrun the original draws once per update, which `-drawevery` copies) and effects that start or end one
update apart for the same reason. Nothing that decides play depends on it.

A frozen baseline of the port is `build/baseline-rocketmania/` (never rebuilt; copy it to new checkouts). `tools/check.sh
rocketmania` plays three bot games (one per mode) in the current exe and the baseline and requires the same, non-empty state log
from both.

## What the pass changed

All the differences were found by bisection to the first update at which a value differed, then by reading the original's
code in our export or its disassembly. The commit is on branch `claude/frosty-poincare-77c4d9`.

| Difference | Cause in the original | What the port does now | Commit |
| --- | --- | --- | --- |
| The global generator and the board's generator drew other numbers | The original seeds its Mersenne twister the 2003 way (`mt[0] = seed`, `mt[i] = mt[i-1] * 69069`, 31-bit draws) from `srand`; the board's own generator is seeded from the global one's draws. The effects, the head and the tiles' sideways jitter draw from the global one; the CRT's `rand` only draws the picture | A game-local global generator seeded that way (`GlobalRand`); every call in the effects, head and pieces uses it; the board's generator is seeded from its draws | 4f774d4 |
| The global generator's draws came in another order | The original puts the board at the back of the widget list (`BringToBack`), so the board updates before the head and the effects | `StartBoard` does the same | 4f774d4 |
| Flames crossed a cell at the wrong speed | The two steps of an igniting flame are 0.166666 and 0.333332, and the rocket's is 0.015 (Ghidra showed the doubles stored as two dword immediates wrongly) | Those constants | 4f774d4 |
| A first board and the timed modes' rhythm were off | Two board members have starting values: -1 for the fuse's launch and its countdown, the arcade drop delay `Rows20()` (20 / rows) and 2 for the timed modes' drop interval | The constructor sets them before the state is set | 4f774d4 |
| Strategy mode's hinted first board had a wrong tile | The 18-entry rotation table's last entry is 2 | Fixed | 4f774d4 |
| A click on the arcade top strip dropped the next tile late | The click sets the drop timer to the whole delay (the next update drops the tile); after a launch it is the delay less 100 | As the original (`m2dc = m444`; the launch keeps `m444 - 100`) | 4f774d4 |
| A scripted click did not drag the pressed tile | The original's window procedure sends the board a move (a drag while a button is down) before every mouse message, so the release is preceded by a drag at the same point. A tile pressed earlier and still remembered moves to the next column when the click lands more than half a cell away. The script runner did press and release without the move in between | `-script` clicks move again before the release, as a real click does | de03ce5 (`shared:`) |
| A click on a score text did not reach the board | The floating score texts are not mouse targets (the constructor clears the flag) | The same | 0afca39 |
| A rocket on the right edge could get the wrong number | A new launcher takes its number from its row (the generator's fourth argument is the caller's row); the port passed the column | Row | 978675f |
| Strategy mode ended by itself when the night timer ran out | The night timer is for the classic mode only (the original tests the mode first) | Classic only | 7b7a74e |

Harness only (not in the port): the PNG loader of the carved exe overflows a buffer (about 26 KB) on the 1-bit `_Arial8.png`
and dies on a fresh heap, so origrun's patch list for the copy sizes it right (`png` line of `tools/origrun/patches/rocketmania.txt`,
written by `tools/identical/setup.sh`, git-ignored). `setup.sh` carves the exe from the installed game with the private
`extract_game_exe`, so it is in `.publishignore` (62e5f58).

## Oddities skipped

- **The fuse's countdown before the first launch** (`burnt`): the original never sets it before the first launch, so it holds
  what the heap held. The port's value is its own; the comparison leaves that first value out.
- **Pieces' fields the original leaves uninitialised** (`mFromRow` and the like) differ; nothing reads them.
- **The global generator's index follows the number of screen draws** (see above): in real play the effects' random positions
  differ from run to run, as in the original. It touches no rule.
- **origrun's watch keeps 4000 changes**: a value that changes every update (the global generator's index) is compared only for
  the first 4000 updates of a run; its first word and the board's generator cover the rest.

## Phase two: how it looks, sounds and takes input

**Sound.** Three whole games (arcade, classic, strategy, 4571 plays in the original) were played in the original and the port under
the same input with origrun's virtual clock and `--sound wav`, which logs each DirectSound play, and the port's `BYOPOP_SOUNDLOG`.
Sample by sample, the plays come in the same order at the same update with the same volume and pan; the original made 4571 and the
port 4569, the two that differ being a sample that the original plays at the very end of a run, after the port's last update. The
music's fade-ins, volume curves and stops match as well. One difference was found and fixed: the original's menu buttons beep when
pressed (`GameSelector::ButtonPress`, 0x405ec0), the port's did not (commit 83dc5e84).

**Focus.** A focus loss and regain in the middle of a game, on the host under rundesk: the pause count, the state and the sound match
the original's (the head slides into its hole, the music fades out and back, the fuse clock stops).

**Screens.** The tour u2 (menu, difficulty dialogs, hints, a game, level-up, high scores, game over) was played in the original inside
Windows Sandbox and in the port, and shotdiff compared the shots. The menu, the dialogs, the hints and the end screens come out the
same to within the sandbox's display (a faint noise on the textured menu buttons, the same in every shot, so it is the sandbox's colour
handling and not the game). Frames of a moving board cannot be lined up: this framework of 2003 has no update-state variable, and the
draw-pending flag stays set, so origrun cannot wait for the draw of update N, and the original's frame can be many updates older than
the label (the rockets' fade-in, which depends only on the board's age, showed the original 90 updates early in one shot). The
board's logic was compared by state in phase one (the board's update counter agrees with the app's in the original: it starts at
40181 and the app counter reads 40181 as the board's 1). What moves on the board is therefore judged by the logic and not by the
pixels.

**Web.** The web build under Node played the same tours as the native one (the log, and its shots compared with the native port's):
the only differences are sub-pixel edges of rotated tiles and of the head, which Emscripten's software drawing and the native build
draw a fraction apart. Not chased.

**Real input.** `games/rocketmania/tools/driver/play.py` plays whole games natively under rundesk with `-commands` and `-realinput`:
it asks `state` and `move` (the port's bot, `testhooks/Commands.cpp`: the play link, a dialog's button, the next mode button, the
clicks that turn a path across the board), sends each step as a real SDL event and, after 90 seconds of play, `lose`, so the game
over flow runs by real clicks too. Three games (arcade, classic, strategy) from launch through every hint dialog, windowed (231 s) and
with Alt+Enter to full screen (126 s) both finished at the menu. On the web, `?player=1&silent=1&driver=move` played 250 moves of
arcade through DOM events to level 2 (score 2727, no errors). Two faults of the test side were found: a bare `move` under
`-realinput` reached the framework as a mouse move (fixed on main by the coordinator in 4cf35432), and the bot's constructor
marked the input as scripted, which dropped every real click that came after a `move` (the command now keeps the flag as it was).

**Registration and links.** The port has no trial clock, no nag, no register, unlock or buy dialog, and the Options dialog has none of
the original's register, update, partner and web-page buttons; the loading bar's line under it is empty as it is in the registered
copy. There is no "More Games" button in the port.

**Loading.** The port loads in one go (the loading thread runs through, 1.7 s on the machine of the tests), and the play link shows
the update the game has loaded: the original looks at the thread's progress only every fourth update, the port at every one (a
deviation, `LoadingScreen::Update`; nothing else about the game's start changes).

**Fixes mod.** The phase found no behaviour of the original that is a bug changing how the game plays (scores, timing, rules) and
that the port keeps: the faults found were fixed in the port as they broke play or were leaks (phase one's table). So the game has no
fixes mod yet and nothing is listed under "Fix candidates".

## Not done, and what remains

- The Options dialog and the credits screen were not compared screen by screen (the original's link buttons are never clicked), and focus in full screen was not tested.
- The hint flags: the original's registry has some hint flags set, the port starts with all clear; the bot clicks every hint
  dialog it sees, in both programs, and the results were the same, but a run with different hint flags was not compared.
- The high-score table was entered and shown in the same runs, and its names and scores come out of the same state, but the
  two high-score files were not compared byte for byte.
- Leaks of the original (memory the original keeps using after leaking it) have not been looked for yet.
