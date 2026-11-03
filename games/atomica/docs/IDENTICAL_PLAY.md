# Atomica Deluxe: identical play

Phase one of the identical-play pass: with the same input, the original and the port play out the same games. (Phase two,
screen by screen, sound, focus and the web build, is not done.)

## What was compared and how

The check is by game state. `tools/identical/compare.py <scenario>` runs the original (Atomica.exe 2.52, from a copy,
through origrun on its virtual clock with a scripted cursor, invisibly under rundesk) and the port (`-fast -drawevery`,
which draws every update as the original does) on one scenario, and compares 29 values at every update: the two random
generator words, score, level, molecule count, combo, the 72 cells of the grid, the meter's value, tick, countdown and
overflow, the pause count, the selected atom, the menu and board pointers and more. The original's values come from
origrun `--watch` (addresses from our export in `re/atomica/`, listed in `WATCH`); the port's from its `-statetrace` file.
Scenarios are plain "update, click x y" lines. The scripts in `tools/identical/` were recorded from the port's bot
(`record.sh`), which plays through the same clicks as a player's, so every move, hint box and level-up box is a real
click: `s2.txt` (strategy, level 1 completed, the level-up box), `a1.txt` (action, to update 6082), `g4.txt` (strategy,
game over at update 5004), `g5.txt` (two strategy games in a row: the game over, the high score entry with a typed name,
the play-again button and the second game's start), `c1.txt` (continuing a saved game, below) and `s5.txt` (an idle
game). `compare.py --scores` adds the two high score tables (scores, the first letters of each name, the best of the day). Before each original run the registry values it keeps
(hints shown, symbols, tips) are deleted so both start fresh.

Result: all 29 values agree at every update of `s2`, `a1`, `g4` and `c1`, and the 30 values of `g5` (the tables included)
up to update 6500 (see the oddity about the original's stopping), apart from start-up values the original has not set yet
(`title_p`, `rng_k` at update 1; `numcolors`, `meter_over`, `rounds` at the update the board is made: they hold whatever was
in memory, 0x80808080 and the like).

## Changes made

The original draws random numbers while drawing (Board::Draw, the swirls, the Atomicus monitor, the bolts), so every
visible effect that draws must draw the same numbers in the same order.

| Difference | Cause in the original | Port now |
|---|---|---|
| Random numbers drifted from the first frame | The Atomicus monitor's static draws one number per row of its buffer (150 a frame, 0x4220xx) | `Atomicus::Draw` draws into the monitor buffer and takes one `Get(mNoise)` per row |
| Dr. Atomicus talked and looked about at other times | Lip-sync runs from the voice's mouth data (0x421c70-0x421e10) and a pending flag (+0x45, set from the start) holds back looking about and comments | `LipStart/LipUpdate/LipNext`; the mouth data is read from the player's exe at start-up (`GameText`, `LipDataTable.inc`), none in the repository |
| Bolts drew a different number of numbers | `Bolt::DrawPixels` (0x4173f0): a bent line with two jitter points on each side (6 draws) and mini sparks around both ends (16 draws) | `Bolt::DrawPixels` rewritten to it |
| Bonds drew a different number of numbers | `Grid::DrawBond` (0x40f2e0): three angle jitters, then two mini lines | `Grid::DrawBond` rewritten to it |
| The default high score names were the port's own | The original's five names are texts of its exe (0x4b0270-0x4b02ac, read by 0x41e320) | `ScoreTable::Reset` reads them like the other texts (`GT()`), none in the repository |
| The saved game was 4 bytes longer | The original saves only the mode (0x405fc0), not the expert flag | the flag is no longer saved or loaded |
| The lip-sync varied from run to run | The port read the real clock (`GetTickCount`) where the original reads its own virtual tick | `Atomicus` uses `GetClockTime()` (virtual under `-fast`); the bot's runs are now identical to each other |

The test bot (`testhooks/Autoplay.cpp`) was changed so that its play can be replayed as a scenario: it acts before the
update as a script does, clicks through the widget manager, waits three updates between a click's two halves (the atom, then
its cell; the hint box's check mark, then Continue) and records its clicks (`-autoplayscript`). That changed the bot's games,
so `tools/check.sh atomica` was refrozen (`AT_STRATEGY`, `AT_ACTION`); all later fixes must leave those numbers unchanged.

## Saved game

`savecmp.py <scenario>` plays a scenario that ends in "save and quit" (`sv.txt` shape: the quit button at 72,464, then
"save and quit" at 386,283) in the original and the port and compares the two `SAVEGAME.DAT` files word by word. The files
have the same length and layout; 163 of their 1,261 words differ, all of them uninitialised memory the original writes
out (heap fill such as 0x80808080, pointer-like values in each atom's record and the board's tint floats), which the port
writes as zeros. Then `compare.py c1.txt --savefile <the original's file>` starts both programs with that file and chooses
"continue" on the menu: all 29 values agree for 2,222 updates after the game is picked up. (The port also reads the file
the original made, so a game saved by one is continued by the other.)

## Frozen baseline

`build/baseline-atomica/` holds the port's exe as of this pass. `tools/check.sh atomica` replays `s2.txt` and `g5.txt` in the
current port and in the baseline and requires the same non-empty state log from both, besides the bot's two games and its
rerun. A later fix that changes unmodded play refreezes it on purpose.

## Oddities (not chased)

- The original stops updating its board and drawing at a point that varies from run to run, while its update counter goes
  on: the random numbers stop changing at update 4711 (a game started at 711), 4533 (a continued game started at 2311), 6319
  and 6531 (games started at 2311). The meter's countdown stops about 70 updates later. A click or a mouse move does not
  move it; it is not 4000 updates from the start, whatever it first looked like, and the export holds no constant or idle
  counter for it (searched for 4000, 0xfa0, 40000, 15000, the tick calls). In the same runs origrun's clock log shows the game
  time per update rising (97 s for 5,200 updates in one run, 318 s for 4,600 in another), so it looks like the framework's
  catch-up logic reacting to the virtual clock, not the game's own rules. The port is unaffected. To see it, run
  `compare.py tools/identical/s5.txt --names rng_j,meter_countdown --end 4900`. Every comparison above stops before it.
- The two saved games differ in the uninitialised words described above.

## Deviations left

The geiger needle's geometry and the title logo's shimmer (README), the swirls' scan-line fill, and the missing "More games"
and "Register" links. The high score table's "best name" and the saved game's garbage words were not compared by value.
