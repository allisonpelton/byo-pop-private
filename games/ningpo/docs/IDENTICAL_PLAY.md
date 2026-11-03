# Ning Po Mahjong: the identical-play pass

This is the report on comparing the port with the original `Ningpo.exe`. Phase one, the game's logic, was finished on
2026-11-03: with the same input, a series of games plays out the same in both programs, update for update. That means
the same deals and shuffles, the same matches, hints, undos and penalties, the same clock, the same endings and the
same best times. One change came out of it, listed below. Phase two (screens, sound, focus and the web build) has not
started.

## What was compared, and how

**The original** ran from a copy of the user's install, staged under `C:/byo-og/np` by
`games/ningpo/tools/identical/setup.sh`, and never from the install itself. It ran through `tools/origrun` under
rundesk, on origrun's virtual clock (`--clock virtual`) with its scripted cursor (`--cursor scripted`). Its game time
moved only at its own waits, and every input arrived at an exact game time. The copy is patched in memory at its
start: `PGApp::mainLoop` (0x40d820) seeds the generator with the run's seed instead of the first event's time, as the
port's `-seed=N` does.

<!-- private: The copy is also patched to play as the registered game, as the port plays; the patch list is in tools/origrun/patches (git-ignored), written by setup.sh. -->

The copy is also patched to play as the registered game, as the port plays: `NingPo::checkRegistration` (0x405cd0)
answers true (`568bf1` becomes `b001c3`, mov al,1; ret). Unregistered, the original refuses layouts above 7 and tile
sets above 2. The patch list is `tools/origrun/patches/ningpo.txt` (git-ignored), and setup.sh writes it, so setup.sh
belongs in `.publishignore`.

<!-- /private -->

Before each run, `orig.sh` deletes the game's own values from `HKCU\Software\Pyrogon\NingPo` (its settings and best
times; the installer's URLs stay), so every run starts as a first run does, windowed. The original ran with
`--sound nodriver`. With origrun's working sound stand-in, the game stops reaching its waits once the title's music
starts, and the run times out; that is a matter for origrun, reported to the coordinator. With no device the game
says once that DirectSound failed. origrun's hook answers that box with no window and no sound, and the game plays on
silently. origrun's clock guard is off (`ORIGRUN_CLOCK_GUARD=0`): the exe's own mixing thread would keep it on, and
the main thread's waits would then take their real time.

**The port** ran with `-fast` (10 ms of game time per update) and `-seed=N`, under rundesk, from fresh settings, with
the test hook `-statelog=<file>`. That hook writes a line `<event time> <name> <value>` whenever one of twenty values
changes. It checks them each time the game reads its event queue (`HInput::getEvent`), which is where origrun reads
the original's watched values. It also writes the whole board at each deal and shuffle. The time is
`PGApp::mEventTime`, the time of the event handled last, at 0x47be70 in the original, which origrun uses as its
counter. The twenty values, with the original's addresses (the game is the global at 0x475550):

| Name | Original | What it is |
|---|---|---|
| state | +0x81c0 | title, playing, won, game over |
| tilesleft, moves | +0x81b8, +0x81bc | the counters under the board |
| penalty | +0x6b1c | the seconds added by hints, undos and shuffles |
| hints, undos, shuffles | +0x81c4, +0x81c8, +0x81cc | the game's counts |
| finaltime, finalraw | +0x81d0, +0x81d4 | a finished game's time, with and without penalties |
| rngindex | +0x68fc | the generator's index (it moves with every draw) |
| clock | 0x475544 | the clock's seconds as last drawn |
| layout | +0x8254 | the layout chosen on the title |
| undoindex | +0x6d68 | the undo list's top |
| tile0, tile71, tile143 | +0x6fb4, +0x7894, +0x8194 | three tiles' types (the deal) |
| best0tiles, best0time, best1tiles, best1time | +0xacf0, +0xace8, +0xad20, +0xad18 | the first layout's first two best times |

**A case** is a port script ("update command" lines). `toorig.py` turns it into origrun's script, each command after
a wait of the game time since the one before. origrun holds the game as it enters the sleep that would carry its
clock to a wait's end, before that sleep moves the clock, so the game reads the next input 1 ms early, and each wait
but the first is 1 ms longer to make up for it. `case.sh` runs the port once and the original once for every three
values: origrun watches four values a run, and every run also watches rngindex. `cmp.py` lines each run up on its own
first deal, because each run of the original starts its clock at another time. It then compares every value's
changes, in order, with their times. A run whose generator went another way than the port's is run again, once, and
the case's output says so (see the oddities). `cases.sh` writes the cases:

- **cheat7** (seed 7): the cheat's matches (Backspace q Backspace, then `m`) until no pair is left, End Game in the
  no-moves dialog, the name entry, the best times, the title. This is tools/check.sh's game.
- **won39** (seed 39): a game won with the cheat on layout 0, the name, the best times, then a second game on the
  next deal ended with tiles left through End Game, whose entry goes second, and the title.
- **mouse23** (seed 23): matches made with the mouse, including wrong pairs, on the deal; two hints, two undos, show
  moves, pause and unpause, a shuffle; more matches on the shuffled board; 50 seconds with no move (the idle hint);
  New Game with its question; matches on the new deal. `mouse.py` picks the pairs from the port's board, by the
  original's rules for free tiles (`Board::isFree`, 0x406270) and matches.
- **layouts** (seed 11): all 18 layouts in turn, each with a deal, a hint, two cheat matches, an undo, a shuffle,
  New Game with its question and the layout arrow.
- **xsave** (`xsave.sh`): Ning Po saves only its settings and best times, in the registry; it keeps no game in
  progress. Both programs play won39 from fresh settings, and their saved values (151 of them) must be the same byte
  for byte. Then the original plays cheat7 on the port's saved values, the port plays cheat7 on the original's, and
  what each saves must again be the same. cheat7's entry, 40 tiles in 12 seconds, lands between won39's two.

All five cases now give the same values in both programs. The only exception is the tick time described under the
deviations.

## The changes the pass made

| Difference | Cause in the original | What the port does now | Commit |
|---|---|---|---|
| After the first game, the hint, undo and shuffle counts and the final times went on from the game before. The hint count picks which pair a hint shows, so a second game's hints pointed at other pairs. | `NingPo::newGame` (0x404c90) clears +0x81c4 to +0x81d4 (hints, undos, shuffles, both final times) and the "no match" sign's time (+0x6b08) at every deal. | It clears them in the same place and order. | a5b98ce |

The state log (`-statelog`, and `pg::Host::EventRead` called from `HInput::getEvent`) is a test hook. It changes
nothing in play.

## Oddities that were not fixed, and why

None in the game. Two things belong to the test harness:

- In about one run in thirty, the original barely ran its loop. It read the cursor 18 times instead of about 600 and
  went 31 times faster than real time instead of about 3. It missed the cheat's keys, so the name's "s" later
  shuffled a board still in play. It happened twice, both times in cheat7's second run (the clock, penalty and hints),
  and the run went the port's way when it ran again. `case.sh` reruns such a run once and says so in its output;
  `<run log>.stray` keeps the stray log. The cause was not looked for: the same script plays the same way in every
  other run.
- With origrun's working sound stand-in the original hangs once its title music starts (above). It is reported to
  the coordinator; phase two needs it fixed to compare the sound.

## The deviations that remain

**A tick after an input comes up to one update later in the original.** The original's loop posts a tick about every
millisecond (`HInput::getEvent`, 0x426a50, sleeps 1 ms when the queue is empty), so the tick after an input comes 1 ms
after it. The port runs one framework update (10 ms) per sleep, so its tick after an input carries the input's own
time. Values that only a tick changes, such as the clock's seconds on returning to the title, can therefore change
1 ms apart in the two programs. In general they can be up to one update apart, since the port's ticks fall on its
updates. `cmp.py` counts such a change as the same when the value matches and the times are less than 10 ms apart,
and it lists how many there were (one, in cheat7). Nothing that is shown changes, and no time that is kept, such as a
final time or a best time, comes from a tick: those come from the event's own time.

## The regression check

The port as of a5b98ce is frozen in `build/baseline-ningpo/` (the exe and its DLLs; `COMMIT` names the commit).
`tools/check.sh ningpo` plays the layouts and won39 cases in the build and in the frozen port, and it requires the
same non-empty state log from both (commit 677cfc1), besides its earlier seed-7 game and rerun.

## Running it again

```bash
games/ningpo/tools/identical/setup.sh
```

```bash
games/ningpo/tools/identical/cases.sh layouts build/identical/scripts/layouts.txt
```

```bash
games/ningpo/tools/identical/case.sh layouts build/identical/scripts/layouts.txt 11
```

```bash
games/ningpo/tools/identical/xsave.sh
```

A case takes from one to three minutes and runs one game process at a time. Its files are in `build/identical/<case>/`
(`cmp.txt` is the comparison). The seeds are cheat7 7, won39 39, mouse23 23 and layouts 11.
