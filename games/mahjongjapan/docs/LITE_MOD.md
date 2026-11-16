# Mahjong Escape: Ancient Japan's lite mod (2026-11-16)

**One change is taken: quickRedeal, the Shuffle button's search for a deal.** It lives in the shared engine
(`engines/mahjongescape/src/Deal.cpp`), so Ancient China has the same one, with its own addresses and its own figures in its
`docs/LITE_MOD.md`. The mod is `mods/lite/mod.json`: feature `lite`, one setting, `quickRedeal`, on by default
and applied at a restart, read by `Platform::LiteOn("quickRedeal")` in `MahjongApp.cpp` (or `-lite=quickRedeal` on the
command line of a test run). With the mod off, or no mods folder, the game is the port as it was, and its frozen baseline
holds. This is the lite step of `docs/MOD_ROADMAP.md`: a change may give another result, and has to say exactly where.

## The change

Shuffle (a button of the board, command 0x94 on an escape stage) calls `Board::Reshuffle` (0x40b0a0), which seeds the generator
from the clock and calls `Deal::Start` (0x413500) with `theRedeal` set. That runs `Deal::Solve` (0x413d90) and
`Take` (0x413ff0), a backtracking search that gives the tiles still on the board kinds in an order that can be cleared (every
pair free in turn, every lock opened by its key, the gold tiles last), and it does so under a clock: every 1,001 steps
`SolveInTime` (0x413fa0) reads the machine's clock and ends the search when a second has gone (two for a new game's deal), the whole
game standing still meanwhile. When the search ends with no deal the board is dealt at random (`DealAny`) and may have no
move at all, which the game answers with its no-moves box.

With the mod on, a redeal's budget is a count and not a time: the search ends after 200 of those clock checks (200,200 steps, about
15 ms on this machine), and ends exactly as it did before. Nothing else changes: the new game's deal keeps its two seconds, the
shuffle's animation, its sound and the 20 seconds it adds are untouched, and the search is the same search with the same random
draws until the budget ends it.

## Where play differs, and the proof that it can

There are two places, both on a shuffle.

*A board where no deal exists.* The original's search runs out its second and deals at random, and where it stops depends on the
machine: how many steps it took, so how many random numbers it drew, so which random deal it ends with. The mod stops after the
same 200 checks on every run. So the two play differently from that shuffle on, and the original differs from itself from one run
to the next. Seen in `mahjongjapan_dealtest` (the layout and deal test) on escape stage 9: the shuffle's redeal on the stage's board
after a quarter, a half and three quarters of the moves that win it, seeds 1 to 3 (`STAGE_REDEAL=3`): the original ran out its
second at all nine and dealt nine other boards in each of two runs (the `kinds` hash on each line differs between the two runs),
and the mod dealt the same nine in each of two runs.

*A board where the search would succeed late.* A search that the original ends with a deal after more than 200 checks gets a random
deal from the mod, which may not be clearable. In the offline sweep below none did (the largest successful search was 24 checks);
in the game, on the boards a bot leaves, 6 of 918 shuffles of the original needed more than 200 checks (3 more than 1,000, the largest 4,512), about one in 150. A player then has a board with fewer moves, or none, where the original's would
have been a good one, and can shuffle again.

How often a shuffle finds no deal at all, in the original, depends on the stage and the board. Offline, on the boards after a
quarter, a half and three quarters of the winning moves of 53 of the 60 escape stages (the other 7 are not
measured: the test's own solver did not win them in 25 s), 5 seeds each (795 redeals), the original ran out its second in 134
(16.9 %), on 16 stages. In the game the bot plays a stage down by a quarter of its tiles (60 % on 14 of the stages) and the Shuffle button is clicked 24 to 30
times in a row on that board (`shuffletest.py`): of 918 shuffles of the original on 16 stages, 17 (1.9 %) found no deal, all on stage 23, where
17 of 144 (12 %) did; on the other 15 stages none of 774 did. The bot's adventure never shuffles, so no run of its games meets any of this; a player
meets it by pressing Shuffle.

## The gain, measured

Offline: the 795 redeals take 134,171 ms in the original (each of the 134 that find no deal takes 1,001 ms) and 1,827 ms
with the mod, the same board and seed each (`tools/profile/stageredeal.py`; every search the mod ended, it ended at 201
checks, and the original found no deal between 25 and 200 checks).

In the game, on stage 23 (30 shuffles in a row on one board, alternating pairs of the original and the mod, three pairs, pinned to one
performance core, full screen 3840x2160 as a fresh player's game starts): the processor cycles from each click to the board at rest add up to 20.57,
19.01 and 18.76 G for the original and 16.36, 14.81 and 14.68 G with the mod, about a fifth less (median 19.01 against 14.81). A shuffle that finds a
deal at once costs about 0.4 G here, mostly the full screen picture of its animation; one whose search runs out its second costs 2.14 to 2.38 G, and
one whose search the mod stops 0.59 to 0.64 G. In those runs the original found no deal in 8 of 90 shuffles and the mod in 14 of 90: the search is the
same until the budget ends it, so the mod's extra failures are the shuffles whose search needed more than 200 checks (3 of the original's 82 solved
shuffles took 689, 4,027 and 4,512), and the rest is the two series being other boards and other draws. The original's wall time for the slow shuffles
was 1.7 to 1.9 s against the mod's 0.9 to 2.1, which on a machine with other work running is no figure to use; the cycles are. A shuffle that finds a
deal at once is the same in both.

## What was looked at and not taken

The rest of the game's logic costs nothing worth a lite change, measured in `mahjongjapan_dealtest` (`LOGIC_BENCH=1`) on all 222
layouts (162 classic, 60 escape), in microseconds of this machine's time per call: `Layout::UpdateFree` (0x4147e0), the whole
board's free tiles, at most 175 µs (it runs once a move, and a frame is 25 ms); `CountMoves` (0x414d10) at most 0.8; `UpdateNeighbours`
(0x4149b0 and 0x414be0) of a pair at most 0.7; `HitTest` (0x414750) at most 0.5. The deal of a new game, which has the same kind of clock (two seconds), was
timed with the same code: the 60 escape stages, none over 1 ms (all together 12 ms), and 162 classic layouts at 8 seeds each (1,296 deals), the longest 1 ms; none came near
its 2,000 ms, so there is nothing to cut and that deal is not touched. A faster free-tile test (a grid of cells) would give the same answer
in every state, so it is not lite and would belong in the port, where it would save nothing a player sees.

## Reproduce

`STAGE_REDEAL=3 build/native/mahjongjapan_dealtest.exe assets/mahjongjapan --stage 9` twice, and again with `QUICK_REDEAL=1`: one line a redeal,
with its time, its clock checks and the hash of the kinds dealt. `tools/profile/stageredeal.py` runs it over the stages and prints
the totals. In the game: `tools/realplay/shuffletest.py <out> --game japan --level N [--lite] [--played PERCENT] [--shuffles N]` plays a stage
down with the bot, clicks Shuffle N times in a row and prints each shuffle's processor cycles (`-cyclesplit`); `tools/profile/shufflepairs.sh`
alternates the original and the mod. The game's log says `lite: quickRedeal` when the option is read as on. All of these are under
`games/mahjongjapan/tools/` (the two games share the drivers).

## On the site

`cmake/ByopopWeb.cmake` ships `mods/fixes` only, so the web build has no lite mod yet; a build with `mods/lite` beside it would show it in
the mod menu and the Original switch (`web/modpanel.js` already knows both groups). That is the coordinator's.
