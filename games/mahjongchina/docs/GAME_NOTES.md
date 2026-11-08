# Mahjong Escape: Ancient China: notes on the original

This file records what the comparison with Ancient Japan found in `MahjongEscapeAC.exe` (linked 2006-08-21, three
months before Japan's), for whoever reads or changes the port. China is Japan's program with other resources and a
few changes of its own, so `games/mahjongjapan/docs/GAME_NOTES.md` describes the program, its saves and its rules,
and this file only what differs. `ENGINE.md` says where China's engine differs from Japan's map, and `DEVIATIONS.md`
lists where the port's code differs from the original. Each change is listed with its address in
`re/mahjongchina/diffnotes.txt` (git-ignored, with the exes' exports).

## How the port was made

The two exes were compared function by function (`tools/compare_exes.py`): the functions paired by their calls and
their shape (`re/mahjongchina/compare/addrmap.tsv`), then for each pair the constants each side has that the other
has not (`compare/immediates.txt`) and the decompiled difference (`compare/diff/`). Most of the differences are the
classes' fields a word lower, the sizes of what is allocated and engine calls that China's compiler inlined. The
others are the game's own: other places, fonts and pictures in nearly every screen, other counts, and a few changes
of behaviour, all ported. Two kinds of change hide from a comparison of constants as sets, and both turned up late:
the same constants in another order (Board::Init loads its sounds in another order, which changes nothing but the
log; `re/mahjongchina/order.py` finds such pairs, and that was the only one) and a constant that is simply gone
(Board::Measure has no -5, so China's board stands 5 pixels lower than Japan's; the stage-2 comparison with the
original found it).

## What is different

**Dynasty Adventure** takes the place of Emperor's Adventure: 52 levels in 11 stages (stages of 3, then 4, then 5
levels; 0x410ef0), through China's dynasties, against Japan's 60 levels in 12. A stage ends after levels 2 and 6 and
then every fifth (0x411060). The rules of a level, the special tiles and how a level is dealt and won are Japan's.
The map has its own places, path and bar, all read from the stage tables in `Resource.dll`. Once every level is done
the map no longer offers to start the escape again, and a level's end is a dialog with the time, the score and the
next level's text on a scroll, without Japan's bests and records. The timed tips come at levels 3, 7 and 12 (2, 5 and
9 in Japan), and Japan's rule that skipped the tips once a level had a best score is gone.

**Classic** has 126 layouts (162 in Japan) on 14 pages of nine, and fifteen backgrounds. The in-game menu of a classic
game has no restart. The restart comes only from the second button of a classic game's end, and asks before it
throws away a game in progress. A new player's difficulty is normal (a quarter of the deal in fours; Japan's starts at relaxed), and
a player keeps 150 classic records and 52 of the adventure (175 and 60 in Japan).

**Elsewhere.** The dialogs have dragon corners (an animation on two of them), the main menu has its own buttons and
two animations, the music takes other orders on the map and while keys are in play, and the ending has eleven lines
drawn centred on their places. `Resource.dll` holds 178 layouts: 52 for the adventure and 126 for the classic game.

## The standard test commands, the fixes mod and the finishing pass (2026-11-08)

China's finishing pass is Japan's, done once for both (the same code, so the same files: `src/Bot.cpp`, `src/Bot.h` and
`src/TestCommands.cpp` are copied from Japan's by `games/mahjongjapan/tools/realplay/syncbot.sh`). `games/mahjongjapan/docs/GAME_NOTES.md`
describes each of them; this section says what differs here.

- **The standard test commands** (`docs/TEST_COMMANDS.md`) are Japan's: `state`, `goto`, `level`, `unlock all`, `set`, `win`,
  `move`, and `lose` and `seed` as errors. The levels are 52 and the classic layouts 126 (`goto classic 1` to `126`). The
  bot solves every one of the 52 stages and every classic layout. Stage 11 of China (layout 395) was the hard one: its keys
  and locks need the keys saved for the locks that hold the goal tiles, which the bot's first tries now do, and it solves in
  38 moves. An earlier note of this pass that said the solver finds no solution for it was about the first solver.
- **The fixes mod** (`mods/fixes`, `seededDeals` and `steadyClock`, on by default on the site) is Japan's with
  China's own addresses; `tools/realplay/modtest.py --game china` passes all its checks (the menu at update 260 as the port
  starts and 1550 with `-logos`; the same seed and another start time dealing the same classic board and shuffle; the panel's clock
  showing 2, 4 and 6 s after 400, 800 and 1200 updates). The native folder is `mods-mahjongchina` beside the exe and the
  web copy `mahjongchina-fixes`. The candidates that were checked and are none are Japan's too: the panel's `AddSeconds`
  never reaches its odd branch, and no layout of the 178 deals a free tile that cannot be clicked at the start (0
  unreachable). The soft lock after a Shuffle at the last pair is fixed in the port as in Japan (`Board::OnCommand`).
- **The start.** Japan's (`games/mahjongjapan/docs/GAME_NOTES.md`, "The start"), at China's addresses (`re/mahjongchina/compare/addrmap.tsv`):
  the two logo screens (2 s each) and the 0.77 s of black before them are the original's wait before the game can start, so the
  port opens at its main menu and `-logos` keeps the original's start (`tools/check.sh` passes it). The launch to the menu
  took 7.7 s and takes 1.29 s. The rest of the launch has no wait of the original's making, as Japan's.
- **Registration and links.** China has no registration code either; the PopCap and SpinTop logos at the main menu's
  corners are pictures that take no click, as in Japan, and the shared libjpeg 6b loader decodes the backgrounds (the screens
  of `tools/check.sh mahjongchina` were taken again for it: at most 3 levels of a channel, in the JPEG backgrounds only).
- **Real input.** `tools/realplay/drive.py --game china` plays the same: the menu and its help, the name, all 52 levels of
  the Dynasty Adventure to the ending by real SDL events (16.0 minutes of real time), two classic games, the focus lost and
  regained, Quit; and again in full screen. The web driver (`mahjongjapan/tools/web/realplay.js`, taken by the China page
  too) plays the same through the DOM's events.

## Performance (2026-11-08)

Measured as Japan's (`games/mahjongjapan/docs/GAME_NOTES.md`, "Performance"): the port only, windowed, in real time, with no
mods, three runs of the minute of play of `tools/check.sh mahjongchina` (the menu, a new player, Dynasty Adventure stage 1 won
by the solver's clicks, back to the map: 74 s), the processor time in the window 20 s to 50 s.

| | port (3 runs) |
|---|---|
| Launch to the main menu, real time (`tools/realplay/perfload.py`, 5 runs) | 1.29 s median (1.23 to 1.35); with the original's start (`-logos`, 3 runs) 7.70 s (7.69 to 7.78) |
| Peak working set | 172.3 MB median (172.1 to 180.9) |
| Peak private bytes | 170.9 MB median (170.4 to 172.5) |
| Processor time in the window | 5.0 s of 29.9 s: 16.7 % of one core (16.1 to 22.1 %) |
| Processor time, whole run | 10.9 s median (10.7 to 14.0) of 74.4 s |
| Processes and threads | 1 process, 42 to 50 threads |

The original was not measured, for Japan's reasons: its exe runs only inside Windows Sandbox under `tools/origrun`, and a run on
the host would play its music on the speakers. Its two logos and the black before them hold the menu back about 6.5 s (0x42f650 in Japan): the
port takes 7.7 s to the menu with `-logos` and 1.3 s without.
