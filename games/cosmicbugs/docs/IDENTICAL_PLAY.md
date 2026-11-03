# Cosmic Bugs: identical play

## Phase one: plays identical (2026-11-03)

With the same input, a series of games now plays out the same in the original exe and in the port: the same random
numbers drawn at the same updates, the same score, lives and filled cells at every update, the same boards, levels
and endings. Three changes to the port were needed, and one of them also removes a run-to-run variation the original
has.

### How the games were compared

The original ran through origrun (`docs/ORIGRUN.md`) from a copy under `build/origrun/<run>/game`, invisibly under
rundesk, windowed, on origrun's virtual clock with its scripted cursor. Two test patches went into the copy, kept in
the git-ignored `tools/origrun/patches/cosmicbugs.txt`: the game seeds the C library's generator from the time at
start-up, which the first patch replaces by a seed of 0 (the runner's clock starts at 0, so both draw the same); the
second makes each frame wait exactly 10 ms of the virtual clock and then read its elapsed time, so that every frame
runs ten updates, as the port's runner does. With both, a 400-second game of the original runs in about a minute and
plays the same way every time it is run (but see the beams below). origrun watched the generator's seed (0x460a64),
the score (0x46e5e8), the lives (0x460040) and the board's filled cells (`[0x473138]+0x3b8`) at every frame, and
`--watch-max` (added for this) let it log every change of the seed over a whole game.

The port ran in its headless runner (`cosmicbugs_run`) with two options added for this: `CB_WATCH=1` prints the same
four values after each frame, in origrun's format, and `CB_NOWARP=1` ignores the game's cursor warps, as origrun's
scripted cursor does for the original's `SetCursorPos` (the ship's appearance warps the pointer to the middle every
update). A click the runner makes at T ms, with the pointer moved there 10 ms before, is the original's
`at T-19 click x y 30`: the original reads the cursor in the frame it is held at and the button at the next frame's
message pump. A right click and a key are mapped the same way, and a key is held 30 updates, as a click is
(`at N key vk 30`, added to origrun for this: a key pressed and let go within one update never reaches this game).

The scripts are in `games/cosmicbugs/test/plays` (`<name>.txt` for the runner, `<name>.orig.txt` for origrun), and
`games/cosmicbugs/tools/play` has what writes and runs them: `gen.py` writes a random play (a line every 1.5 to 3.5
seconds at a random place, the aim switched by a right click 40% of the time), `pair.sh <name>` plays it on both and
`cmp.py` compares the four watches' changes, value and update.

| Game | What it covers | Result |
|---|---|---|
| g1 | 200 s from a fresh start, the first boards | the same at every update (seed, score, lives, fill) |
| g2, g3, g4 | 400 s each from a fresh start: several boards finished, their bonuses and the next boards, lives lost | the same at every update, every seed change of the whole game included |
| a1 | Options, the shooter mode switched to automatic, Back, then 300 s of play | the same at every update |
| k1 | 300 s with the keys: Space to turn the ship, Enter to pause and again to go on, the '.' key (which ends the board), Esc to the title in the middle of a board (the game is saved), Start Game and the requester's OK to resume it, then more play | the same at every update as two of the original's three runs (see the beams below) |
| c7 | a second launch with a save from the original (board 4, one life left): Start Game, OK to resume, then play through the game's end | the same at every update |

The title left alone for 15 seconds and the suite's own scripted game (two lines on the first board) were compared
the same way and also matched. Both saves' contents matched except the player's name, which the original takes from
the Windows user name and the runner's host gives as GUEST.

`tools/check.sh cosmicbugs` now plays the series (all but c7, whose save comes from the original) on the runner and
checks the checksum of their watch lines against `build/baseline-cosmicbugs/`, a copy of the runner frozen at
commit 1d7e5e0 once the series matched the original. A change that alters play fails the suite; a deliberate one
refreezes the baseline and says why here.

### The changes

| Difference | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| The seed stepped differently whenever its top bit was set, so the series parted after a few hundred draws | `Rand` (0x420290) divides the seed with an unsigned `div` | divides unsigned | 085e7de |
| A rider (the spark that runs along a beam to the ship when an enemy touches the beam) was made one frame apart, and everything after differed | `Dist3` (0x420300) stores each difference and the sum of the squares as floats before the square root; the port worked them in double, and a rider is made at a distance just under its threshold (19.98 against 20 in one case) | stores them as floats | 085e7de |
| The same input played out differently from run to run, in the original and in the port, from one rider on | an enemy makes a rider on a beam it touches unless it is the beam it touched last (its pointer at +0x100), and nothing clears that pointer when the beam is freed. A later beam that the heap happens to put at the same address therefore makes no rider on that enemy, and Windows' heap picks among freed blocks at random | an enemy forgets a beam when the beam is deleted, which is what the original does whenever the address is not reused; every run of the port is now the same | 1d7e5e0 |

The runner's options are in 085e7de; the series, the scripts and the suite's baseline check are in 36676a0 and
c1eaab8.

Shared commits: 4cc3eec (`origrun --watch-max`, to log more than 4000 changes of a watch), 51a0a6e (`at N key vk k`
holds a key for k updates, and `keydown`/`keyup` post one edge) and c1eaab8 (the suite's series and baseline check).

### Oddities

**The original plays the same input differently from run to run.** This is the beam pointer above: whether a new beam
lands where an enemy's last one was is the heap's chance, so a rider is sometimes not made. To see it, run k1 on the
original three or four times (`bash games/cosmicbugs/tools/play/pair.sh k1` after copying `test/plays/k1.orig.txt` to
`build/origrun/k1.txt` and `k1.txt` to `build/cbw/`): one run in our three parted from the others at update 69541 and
joined them again some 160 frames later, and the other two parted from the first at 115061 for good. An early run of
g1 also parted at 30201, where three later runs did not. The port now always takes the outcome where the address is
not reused; reproducing the chance would take a model of the Windows heap, which is not worth it.

**The ship's open span is left uninitialised until the ship first moves.** The four floats at +0x4b0 hold whatever
the allocation held until the ship's first update in play (one run read 516 in the first). Nothing reads them before
then; the port starts them at 0.

### The user's report

The user saw yellow flickering on the title on a second launch, then numbers on the bottom panel's displays drawn
offset after resuming a game, staying so. It was not reproduced: the windowed exe launched twice in a row (the second
time with the first's save and settings, and with a save at board 4 from the original), the runner resuming the same
save and playing on, and the runner with every allocation filled with 0 and then 0xCD (to bring out any field left
uninitialised) all showed a steady title and the displays in place; the only difference the fills made was one pixel
of a particle in the board at one moment. The offset digits would fit a particle centring the handle of an image it
should not reach (particles call `Image_SetMidHandle` on every image they draw, and the displays' font images come
from the same art bank), but no run drew one. The report is most likely from the web build, so phase two's
comparison of the web build looks for it again, with a second launch in the same browser.

### Deviations left

None in play. The saves differ only in the player's name, which the site's player name hooks supply. The one pixel
that changed with the fill above means some particle field the original leaves uninitialised is still read; it is
for phase two's screen comparison.
