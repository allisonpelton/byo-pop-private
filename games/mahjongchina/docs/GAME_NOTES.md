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
