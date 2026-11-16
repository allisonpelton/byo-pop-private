# Mahjong Escape: Ancient China's lite mod (2026-11-16)

**One change is taken: quickRedeal, the Shuffle button's search for a deal.** It lives in the shared engine
(`engines/mahjongescape/src/Deal.cpp`), so Ancient Japan has the same one, with its own addresses and its own figures in its
`docs/LITE_MOD.md`. The mod is `mods/lite/mod.json`: feature `lite`, one setting, `quickRedeal`, on by default
and applied at a restart, read by `Platform::LiteOn("quickRedeal")` in `MahjongApp.cpp` (or `-lite=quickRedeal` on the
command line of a test run). With the mod off, or no mods folder, the game is the port as it was, and its frozen baseline
holds. This is the lite step of `docs/MOD_ROADMAP.md`: a change may give another result, and has to say exactly where.

## The change

Shuffle (a button of the board, command 0x94 on an escape stage) calls `Board::Reshuffle` (0x40af70), which seeds the generator
from the clock and calls `Deal::Start` (0x412f80) with `theRedeal` set. That runs `Deal::Solve` (0x413810) and
`Take` (0x413a70), a backtracking search that gives the tiles still on the board kinds in an order that can be cleared (every
pair free in turn, every lock opened by its key, the gold tiles last), and it does so under a clock: every 1,001 steps
`SolveInTime` (0x413a20) reads the machine's clock and ends the search when a second has gone (two for a new game's deal), the whole
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
to the next. Seen in `mahjongchina_dealtest` (the layout and deal test) on escape stage 9: the shuffle's redeal on the stage's board
after a quarter, a half and three quarters of the moves that win it, seeds 1 to 3 (`STAGE_REDEAL=3`): the original ran out its
second at all nine and dealt nine other boards in each of two runs (the `kinds` hash on each line differs between the two runs),
and the mod dealt the same nine in each of two runs.

*A board where the search would succeed late.* A search that the original ends with a deal after more than 200 checks gets a random
deal from the mod, which may not be clearable. In the offline sweep below none did (the largest successful search was 24 checks);
in the game, on the boards a bot leaves, none of 288 shuffles needed more than 1 check. A player then has a board with fewer moves, or none, where the original's would
have been a good one, and can shuffle again.

How often a shuffle finds no deal at all, in the original, depends on the stage and the board. Offline, on the boards after a
quarter, a half and three quarters of the winning moves of 45 of the 52 escape stages (the other 7 are not
measured: the test's own solver did not win them in 25 s), 5 seeds each (675 redeals), the original ran out its second in 85
(12.6 %), on 10 stages. In the game the bot plays a stage down by a quarter, a half or three quarters of its tiles, on the ten stages that fail most offline, and
Shuffle is clicked 16 times in a row on that board (`shuffletest.py`): none of the 288 shuffles failed (the longest search was 1 check). The boards a bot
leaves are easier than the boards after a stretch of the winning moves, so what a player meets lies between. The bot's adventure never shuffles.

## The gain, measured

Offline: the 675 redeals take 85,086 ms in the original (each of the 85 that find no deal takes 1,001 ms) and 1,029 ms
with the mod, the same board and seed each (`tools/profile/stageredeal.py`; every search the mod ended, it ended at 201
checks, and the original found no deal between 25 and 200 checks).

In the game the bot's boards never needed a long search (288 shuffles, the longest 1 check), so no stretch of play on this machine shows a gain
here. The mod would cut the same second of search on a board of the offline kind, which a player can reach (they are boards after the winning moves),
and the figure for it is the offline one above.

## What was looked at and not taken

The rest of the game's logic costs nothing worth a lite change, measured in `mahjongchina_dealtest` (`LOGIC_BENCH=1`) on all 178
layouts (126 classic, 52 escape), in microseconds of this machine's time per call: `Layout::UpdateFree` (0x414260), the whole
board's free tiles, at most 148 µs (it runs once a move, and a frame is 25 ms); `CountMoves` (0x414790) at most 0.6; `UpdateNeighbours`
(0x414430 and 0x414660) of a pair at most 0.7; `HitTest` (0x4141d0) at most 0.5. The deal of a new game, which has the same kind of clock (two seconds), was
timed with the same code: the 52 escape stages, none over 1 ms (all together 4 ms), and 126 classic layouts at 8 seeds each (1,008 deals), the longest 1 ms; none came near
its 2,000 ms, so there is nothing to cut and that deal is not touched. A faster free-tile test (a grid of cells) would give the same answer
in every state, so it is not lite and would belong in the port, where it would save nothing a player sees.

## Reproduce

`STAGE_REDEAL=3 build/native/mahjongchina_dealtest.exe assets/mahjongchina --stage 9` twice, and again with `QUICK_REDEAL=1`: one line a redeal,
with its time, its clock checks and the hash of the kinds dealt. `tools/profile/stageredeal.py` runs it over the stages and prints
the totals. In the game: `tools/realplay/shuffletest.py <out> --game china --level N [--lite] [--played PERCENT] [--shuffles N]` plays a stage
down with the bot, clicks Shuffle N times in a row and prints each shuffle's processor cycles (`-cyclesplit`); `tools/profile/shufflepairs.sh`
alternates the original and the mod. The game's log says `lite: quickRedeal` when the option is read as on. All of these are under
`games/mahjongjapan/tools/` (the two games share the drivers).

## On the site

`cmake/ByopopWeb.cmake` ships `mods/fixes` only, so the web build has no lite mod yet; a build with `mods/lite` beside it would show it in
the mod menu and the Original switch (`web/modpanel.js` already knows both groups). That is the coordinator's.
