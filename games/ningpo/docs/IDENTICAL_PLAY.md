# Ning Po Mahjong: the identical-play pass

This is the report on comparing the port with the original `Ningpo.exe`. Phase one, the game's logic, was finished on
2026-11-03: with the same input, a series of games plays out the same in both programs, update for update. That means
the same deals and shuffles, the same matches, hints, undos and penalties, the same clock, the same endings and the
same best times. One change came out of it, listed below. Phase two (screens, sound, focus and the web build) is
under way, from 2026-11-08: the port, natively and on the web, has been compared screen by screen and played through
with real input. The comparison with the original's screens and sound waits on origrun (see "Phase two" below).

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

## Phase two: screens, focus, real input and the web build

**The screen tour.** `tools/shots/tour.py` writes one list of 26 timed steps three ways. The native port gets it as a
`-realinput` file, with real SDL input at fixed updates. The player-only page gets it as a driver,
`tools/web/shots.js`, which holds the game at each step's update through `byopop.test.step`. The original gets it as
an origrun script (`tools/shots/orig.sh`, inside Windows Sandbox, the only place origrun takes shots). Every run uses
seed 7. The tour starts at the start screen and visits the title, the options and their best times, both rules
pages, and each of the title's arrows. It then deals and tries the hint, show moves, pause and resume, and a loss and
regain of the focus with the click that resumes. It answers the New Game question no, and the cheat then plays the
board to its no-moves end. The tour ends with the game over, the name typed in, the best times and the title again.

**Native against the web.** `tools/shotdiff` matches 24 of the 26 pairs exactly. The other two (the hint and show
moves) differ by one 15-bit step in the glow of the lit tiles. The glow pulses on the game's clock, and the two `-fast`
clocks stand a few milliseconds apart at the same update. Nothing else differs. Getting there took one fix in the web
build, listed below. A run on the page needs a clean profile. The page's profile wipe leaves the registry
(`registry.ini`) in IndexedDB and in the localStorage backup, so an earlier run's best times stayed in it; a run now
deletes both first (reported to the coordinator).

**Real input.** `tools/realplay/drive.py` plays the native port with real SDL input under rundesk, through the live
channel (`-commands`, `-realinput`). It plays the tour above and then whole games on the port's own bot (`move`): two
games windowed, and three in full screen (Alt+Enter). Each game ends in the no-moves dialog or won, then the name,
the best times and the title. Every run ends with RESULT ok. `tools/web/tour.js` plays the same tour and two games
on the page with DOM events alone. It passes with no error (full screen needs a real user gesture in the browser, so
the page's run is windowed).

**Focus.** A loss of the focus pauses the game in both builds, and a click on the board after it comes back resumes
it. On the page, a press within 150 ms of the focus coming back is taken as the click that activates the page and
never reaches the game, as SDL drops a window's activating click natively (framework/SexyAppBase.cpp). The drivers
wait past that window before the resuming click.

**The original.** It runs the tour to its end in the sandbox, but every shot comes out flat. origrun's log says
"DirectDraw/Direct3D content not reachable by GDI", on the virtual clock and the real clock and with the sandbox's
virtual GPU. Ning Po draws through windowed DirectDraw in session 0 there. Its screens are still to be compared once
origrun can capture them (asked of the coordinator, 2026-11-08). So is its sound: with origrun's sound stand-in the
original's music streamer stops the virtual clock once a game is dealt (below).

**Changes in phase two.** The trial's code and the options' button to the publisher's site came out (docs/README.md,
"Left out"); Done takes that button's row, and the frame of the emptied bottom row is painted out. The standard test
commands came in, with `move` from the game's own move finder, and `redraw` for the page's shots. Neither changes
play: `tools/check.sh ningpo` still matches the frozen baseline.

| Difference | Cause | What the port does now | Commit |
|---|---|---|---|
| On the web, with `-fast`, a dialog came up hundreds of updates after the click that opened it, so the page's shots showed the screen before. | The port's web loop (`NingPoApp::RunFramework`) stepped the framework until a frame was drawn. With `-fast` nothing is ever not due, and a still screen draws nothing, so the game's code read its events only every 1000 steps. Natively the loop returns to the game once its wait is over. | The web loop also returns once the game's wait is over, and it calls the test page's frame hook, which this blocking loop never reached. | 0fdbf211 |

**The fixes mod.** None of the original's bugs has turned up in either phase, so the mod has no toggle yet. Ning Po
stays in `BYOPOP_WEB_NO_MODS` until the first fix candidate is found.

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

The port as of f5b3502 (libjpeg; a5b98ce before it) is frozen in `build/baseline-ningpo/` (the exe and its DLLs; `COMMIT` names the commit).
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
