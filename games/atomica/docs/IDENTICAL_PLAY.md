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
game over at update 5004) and `s5.txt` (an idle game). Before each original run the registry values it keeps
(hints shown, symbols, tips) are deleted so both start fresh.

Result: all 29 values agree at every update of `s2`, `a1` and `g4`, apart from start-up values the original has not set yet
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
| The lip-sync varied from run to run | The port read the real clock (`GetTickCount`) where the original reads its own virtual tick | `Atomicus` uses `GetClockTime()` (virtual under `-fast`); the bot's runs are now identical to each other |

The test bot (`testhooks/Autoplay.cpp`) was changed so that its play can be replayed as a scenario: it acts before the
update as a script does, clicks through the widget manager, waits three updates between a click's two halves (the atom, then
its cell; the hint box's check mark, then Continue) and records its clicks (`-autoplayscript`). That changed the bot's games,
so `tools/check.sh atomica` was refrozen (`AT_STRATEGY`, `AT_ACTION`); all later fixes must leave those numbers unchanged.

## Oddities (not chased)

- In an idle game, about 4000 updates after the board starts, the original stops drawing random numbers and then stops
  updating the meter's countdown (about 70 updates later), while its update counter goes on; the port goes on as before. See it
  with `compare.py tools/identical/s5.txt --names rng_j,meter_countdown --end 4900`. Not found in the time allowed; it
  does not occur within the games that were played to their end.
- The high score entry after a game over and a saved game's continue were not compared (they need input the recorded
  scenarios do not have).

## Deviations left

The geiger needle's geometry and the title logo's shimmer (README), the swirls' scan-line fill, and the missing "More games"
and "Register" links. The bot's games are not the original's: the original was driven by the bot's recorded clicks.
