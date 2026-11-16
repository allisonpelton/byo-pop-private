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

China's finishing pass is Japan's, done once for both (the same code, so the same files: `Bot.cpp`, `Bot.h` and
`TestCommands.cpp` are one copy in `engines/mahjongescape/src`). `games/mahjongjapan/docs/GAME_NOTES.md`
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
  The sound effects also go, in the framework's sound log form (`framework/SDLSoundManager.h`: a line a `load`, `play`,
  `stop` or `volume` with the update it happened in, 5 ms an update here), to the file named by `BYOPOP_SOUNDLOG`, for the
  site's soundboard presets (`tools/soundpreset.py --ms 5`): a sample is named `Resource <id>`, its WAVE resource in
  Resource.dll, and plays at 0 dB with no pan and no pitch, as BASS plays it. The tour run under it
  (`BYOPOP_SOUNDLOG=<file> build/native/rundesk.exe python games/mahjongjapan/tools/realplay/drive.py <out> --game
  china`) stamps each line of its `drive.log` with the update, so a stretch of the sound log is found by the step that
  made it.

## Performance (2026-11-09)

The performance pass, made once for both games (`games/mahjongjapan/docs/GAME_NOTES.md`, "Performance", describes the method, the
profile and each change; the code is the same, so the changes are the same files: `engines/jollybear/Blit.cpp`, `engines/mahjongescape/src/MahjongApp.cpp`,
`engines/mahjongescape/src/TestCommands.cpp`, and the shared `MemoryImage::mUploadAsIs`, which this port sets on the screen image too). Measured as
Japan's: the minute of play of `tools/check.sh mahjongchina` (the menu, a new player, Dynasty Adventure stage 1 won by the
solver's clicks, back to the map), the processor time in the window 20 s to 50 s, runs made in turn, the original through
origrun (3 runs), the port before and after the pass (5 runs each, alternating, the last batch of the pass).

| | original (3 runs) | port before the pass (5 runs) | port after the pass (5 runs) |
|---|---|---|---|
| Processor time in the window | 1.7 % of one core (1.3 to 6.2) | 10.4 % (6.6 to 11.8) | 7.3 % (2.4 to 9.2) |
| Processor time, whole run | 1.5 s (1.3 to 3.2) | 6.3 s (5.7 to 7.9) | 5.0 s (2.8 to 5.9) |
| Peak working set | 46.6 MB | 114 MB | 112 MB |
| Peak private bytes | 30.7 MB | 106 MB | 104 MB |
| Launch to the main menu, real time (`tools/realplay/perfload.py`, 5 runs of 2026-11-08) | | 1.29 s median (1.23 to 1.35); with the original's start (`-logos`) 7.70 s | not touched by the pass |

The runs swing with the load of the machine, China's more than Japan's: the ranges of the two port columns overlap, so the
direction (about 30 % less processor time in the window, 20 % over the whole run) is the medians', not each run's. Earlier
batches (the window): 15.9 % before and 14.0 % after the blit and Present changes; in a second batch 8.1 % for those and
11.7 % with the upload change added, a difference the ranges (4.2 to 18.1 and 5.3 to 16.5) do not support. The original's
second run (6.2 %, against 1.3 and 1.7 %) is an outlier of the same kind. Where the time goes is Japan's: the engine's drawing, the copy to the widget and the
texture upload in the game's thread, and `SDL_RenderPresent`, the driver's.

**Like for like, thread by thread (2026-11-12).** Measured as Japan's (`games/mahjongjapan/docs/GAME_NOTES.md`,
"Like for like, thread by thread": cycles per thread over the window of 30 s, the port's mixer counted apart, port and original
in turn) after the engine began to redraw only what changed from frame to frame (`engines/jollybear/ScreenLog.h`, shared with
Japan; proved with every frame drawn in full as well and compared: the suite's 2095 frames and the screen tour's 1408, none
differing, every picture as frozen). The first pair: original 3.63 G cycles, port 4.47 G (main thread 2.68, the video driver's
worker thread 1.59, the port's mixer 0.17). In the second the port's window ran 42.6 s instead of 30 (the sampler came late on a
busy machine), 4.20 G or 98.6 M cycles a second, against the original's 3.02 G in 30.1 s or 100 M a second, which the play
does not allow to compare (the window reached past the stage). The original's own runs here differ by 20 % (3.63 and 3.02).
So China's port is 1.2 times its original in the one pair that compares; the framework's present
chain (the widget's upload, `SDL_RenderPresent` and the driver's thread: 2.6 G of the port's 3.8 to 4.5) is what is left,
asked of the coordinator as Japan's notes say.

**Like for like, corrected, and the window-surface presenter (2026-11-12).** The pairs above had the port in a full-screen
window of 3840x2160 against the original's 800x600 window, which is not like for like; Japan's notes ("The window-surface
presenter, and the measurement made like for like") give the correction, the presenter (`framework/SurfaceDisplay.h/.cpp`, shared
with Japan through the engine) and the method. China's figures there, in billions of cycles in the window of 30 s (median of five
runs, the port's mixer counted apart): original, window of 800x600, 3.60 (2.81 to 5.78); port with the presenter's surface mode,
window of 800x600, 1.01 (0.46 to 1.12), and window of 1600x1200 (what a windowed player gets), 1.16 (1.15 to 1.19), which is 0.28 and 0.32 of
the original's, under the target in a window (a fresh player starts full screen, where the renderer's figure below applies); port with the renderer (a resized window, full screen), 4.0 to 4.8, where the presenter changes
nothing (1.1 to 1.3 times the original's windowed figure, the Direct3D device's driver thread 2 G of it). The device's cost that
surface mode does not pay: private bytes 34 against 61 MB, working set 54 against 68 MB, the game's 400th update 0.5 s sooner.
The cost of each way of showing a frame (software and GDI against Direct3D 9 and 11, a 15 % rect at 40 a second), the frame cap
for a renderer without vsync and the test option `-windowscale=N` (reachable only from a test) are in Japan's notes.

**Stretch mode, full screen under the original's figure (2026-11-14).** Japan's notes ("Stretch mode: full screen under the original's
figure") give the mode (`SurfaceDisplay`'s third, no renderer at any window size: GDI's nearest-neighbour stretch of the buffer into the
window, the changed rects as the clip region), how it was checked and the look it changes. China's figures, in billions of cycles in the
window of 30 s (medians of five runs, full screen 3840x2160 on rundesk's desktop): the original in its window of 800x600 3.60 (2.81 to 5.78);
the port with the renderer 4.79 (as above); the port in stretch mode 1.72 without its mixer thread and 1.89 with it (1.82 to 2.09), which is
0.48 and 0.52 of the original's. The engine is shared, so `kStretchOtherSizes` in `engines/mahjongescape/src/MahjongApp.cpp` is one
switch for both ports (`false` until the user has seen the look; a test run's `SURFACE_STRETCH=1` decides instead).

**Where full screen stands (2026-11-15).** The coordinator's word, from the user: keep today's look on full screen, so the renderer's linear-filter picture stays the default and the nearest stretch stays an option (off). The framework session's CPU presenter then landed (2026-11-15, `SurfaceDisplay::kLinearStretchByDefault`, the same look made on the processor), and full screen no longer misses the target: see "Round three" below (2026-11-16).

**The game under a modal dialog and the mod menu (2026-11-14).** The same two overrides (`MahjongApp::ModalOpen` and `ModalClose`, in the shared
`engines/mahjongescape/src/MahjongApp.cpp`) pause the manager under a modal dialog or the mod menu, as Japan's notes ("The game under a modal
dialog and the mod menu") give. Checked in China with the same script (`modal_test.sh mahjongchina`): the timer stands at 0 under the menu
and after the close (the no-menu run reads 4 and 6 at the same two game times as Japan's), and with the frames' phase lined up the first
picture after the close equals the control's of that game time pixel for pixel (`open` with K 2, and K 3 for the later one).

**Why China's and Japan's percentages differed** (README: China's port 7.3 % of a core against its original's 1.7 %, Japan's 6.0 %
against 3.1 %): five runs of each port and three of each original as percentages of one core on a busy machine, China's original
running at 1.3, 1.7 and 6.2 % in its three, so that the median of three took a quiet one. In cycles the originals' medians are
3.60 G (China) and 3.78 G (Japan), each varying from run to run by a factor of 1.7 to 2.1, and with the renderer the two ports
are within 0.4 G of each other. The difference was the original's own spread, not the games'. (With the surface China's port is
0.5 G below Japan's, the main thread's share of the drawing.)

**Memory.** Before any game file is read the process holds 52.3 MB (the framework, SDL and the video driver: shared). At the
end of the play it holds 102.5 MB private: the engine's images (9.4 MB of decoded bits and 12.1 MB of 32-bit copies, 82
images), 5.1 MB of sounds (19) and, from the menu on, the two MO3 modules the menu loads (ids 335 and 339), the rest, about
24 MB. The original keeps 31 MB. `memreport <file>` (a test command, only from a test) writes these figures.

**On the web** (`tools/profile/webperf.js`, as Japan's):

| | before | after |
|---|---|---|
| Main menu: game's time per frame, its share of the thread | 0.89 ms, 4.1 % | 0.53 ms, 2.6 % |
| Adventure map | 4.28 ms, 17.1 % | 3.05 ms, 13.2 % |
| Board, nothing moving | 0.03 ms, 0.2 % | 0.03 ms, 0.1 % |
| Board, ten pairs by the bot | 5.96 ms, 22.2 % | 2.35 ms, 13.1 % |
| Wasm heap (as it had grown), and the bytes malloc has handed out, at the end | 84.3 MB, 68.4 MB | 70.3 MB, 66.3 MB |

The bytes handed out are 2 MB fewer (the vector that went); the heap's own size depends on where its growth steps fell. What is
left is Japan's list.

### Streamed music and one copy of the pictures (2026-11-15)

As Japan's (its `GAME_NOTES.md` has the method; the user, 2026-11-15: "Merge both"): the music modules are decoded as
they play, the same to the bit, and a picture decoded from a resource keeps only its 32-bit copy (`DEVIATIONS.md`, "A
picture's decoded bits"). 5 rounds of alternating runs pinned to one core, the check's play in real time; medians with
their ranges:

| | both as before | music streamed | pictures kept once |
|---|---|---|---|
| Peak private bytes | 70.5 MB (70.5 to 73.5) | 54.7 MB (51.8 to 54.8) | 62.8 MB (62.5 to 65.5) |
| The mixer (SDL's audio thread), G cycles in 30 s | 0.154 (0.143 to 0.206) | 0.244 (0.220 to 0.326) | 0.154 (0.145 to 0.211) |
| The main thread, G cycles in 30 s | 1.34 (1.22 to 1.79) | 1.32 (1.20 to 1.82) | 1.31 (1.24 to 1.81) |
| Launch to the main menu | 0.977 s (0.956 to 1.047) | 0.971 s (0.935 to 1.011) | 0.951 s (0.941 to 1.013) |
| The map's Continue to the first board | 1.760 s | 1.762 s | 1.747 s |
| Web: WebAssembly memory after the play (it grows in steps of about a fifth) | 67.0 MB | 55.8 MB | 67.0 MB (inside one step) |

The streaming's cost: the mixer's cycles go up by 0.088 G in 30 s (each round against its own: +0.069 to +0.121), 58 %
of its own and about 6 % of the process's; offline about 0.6 ms of processor time per second of music (300 s of the
menu's and the game's modules: 185 and 183 ms decoded whole, 362 and 375 ms streamed), paid at every loop, where the
whole decode was paid once at load (29 ms for 335, 148 ms for 339, 38 to 69 ms for the classic list's three), so after
under a minute (the menu's) to about four minutes (the game's) of a module's music the port has spent more on it than
the decode took. A jump into the middle of a recording would decode it from its start inside one mixer callback; the
game's jumps land only on orders that start one. The pictures change costs nothing measurable, and no picture is decoded
again during the play.

### Round three: the presenters, the figures after fw-memory, the lite mod (2026-11-16)

The coordinator's third round for the Mahjong Escapes: confirm which presenter each size takes now that fw-memory's smooth stretch has
landed, and measure the processor time and the memory again, like for like, against the original.

**The presenters** (the game's log, the `display:` and `SurfaceDisplay:` lines of a run at each size; none of them makes a renderer, "renderer none"
in every line). The 800x600 window and the 2x window are *surface mode* (whole scales: the window's own surface, GDI to update it). Full screen,
the default start, is *stretch mode, smooth flavour*: `SurfaceDisplay::kLinearStretchByDefault` (the framework's `LinearStretch`, "the window's GDI
stretch (the screen of 800x600 into the window of 3840x2160, smooth, drawn on the processor)"), the renderer's linear-filter picture made on the
processor, with no Direct3D 11 device and no driver thread. `kStretchOtherSizes`, the GDI nearest stretch, stays an option and is off, as the coordinator
said; the renderer is made only for a drawn cursor image or a window that has had one (none here).

**The look against the renderer's.** Six screens of a scripted play (the menu, the new player's box, the map, a board, the won box, the map again), the
window's own pixels (`screenshotwindow`) in full screen 3840x2160, with the smooth stretch and with the renderer (`SURFACE_STRETCH=0`): the six
PNGs of each game are byte-identical between the two, 0 pixels differ, so `tools/shotdiff` has nothing to flag.

**Processor time, like for like** (billions of cycles of the whole process in the 30 s of the check's scripted play from 20 s to 50 s, the sum over its
threads of `QueryThreadCycleTime` (`threadprof.py`); five rounds of the original and of each size in turn, every run pinned to one performance core
(`tools/pincore.py`), the port under rundesk, the original through origrun in its own 800x600 window; medians with their ranges; `tools/profile/altconfigs.sh`,
`showconfigs.py`). The mixer thread (SDL's audio callback) is in the totals; the original has none of its own, so the ratio without it is given too.

| | original, 800x600 window | port 800x600 | port 2x window | port full screen (3840x2160) |
|---|---|---|---|---|
| Ancient China | 3.17 (2.77 to 3.62) | 1.00 (0.77 to 1.52) | 1.65 (1.30 to 3.17) | 1.34 (1.15 to 2.02) |
| of the original's | 1 | 0.32 (0.25 without the mixer) | 0.52 (0.46) | 0.42 (0.36) |

No size is above the original's. The 2x window has one run at 3.17 G (the others 1.30 to 1.96); not chased, the median does not rest on it. None of the ten original runs' `origrun.log` said "no window in the first 20 s".

**Memory** (private bytes in MB, the check's scripted play of 60 s, sampled every 50 ms by `perfmin.py`: the peak; two rounds, the two runs of a size within 0.3 MB of
each other except the full-screen runs (3 MB apart) and China's original (2.3 MB); the original through origrun):

| | original | port 800x600 | port 2x | port full screen | port at the title (12 s in, 800x600) |
|---|---|---|---|---|---|
| peak private bytes | 32.4 | 44.2 | 42.5 | 44.9 | 25.6 (renderer 56.9) |
| peak working set | 52.1 | 55.8 | 54.0 | 60.8 | 37.1 |

So the port is below the original at the title (25.6 against 32.4 MB; the README's 34 and 104 MB were before the surface display, the streamed music and one
copy of the pictures) and above it in play, by 12 MB. The original's private bytes are flat from its title to the end of the play; the port's grow by
about 15 MB between the menu and a board (`tools/realplay/memprobe.py`: 25.1 MB at the menu, 40.0 MB on a stage's board after a quarter of its tiles, `tools/memmap.py`'s
private memory 22.0 and 36.4; the libraries' 91 MB are file-backed and not counted). Where that growth goes is not known: the Mahjong engine's `memreport`
counts none of its pictures or sounds (it reports one picture, the screen), so the split into pictures, sounds and music is not available, and nothing was changed for it.
This is the one figure still above the original's; the target of the performance step is processor time, which no size misses, so it is listed under "Oddities" as
open and not chased. The likely holders are the pictures kept as 32-bit copies where the original's surfaces were 8 or 16 bits, and the sounds and the
modules' streams decoded when the board's first sounds play.

**Where full screen stands, now (2026-11-16).** The 2026-11-15 paragraph above is out of date: the framework's smooth stretch is the default and full screen is
0.42 (0.36) of the original's processor time in a window of the same play, with the renderer's look to the pixel (0 differing in the six screens).

### The lite mod (2026-11-16)

One change is taken, `quickRedeal`, the Shuffle button's deal search (`Deal::Start` with `theRedeal`) stopped after 200 clock checks of 1,001 steps instead of a
second of the machine's clock. It is in the shared engine (`engines/mahjongescape/src/Deal.cpp`, `Platform::LiteOn("quickRedeal")` in `MahjongApp.cpp`) and is the whole
of `mods/lite/mod.json` (feature `lite`, one setting, on by default). Where a shuffle's result differs, the proof, how often (10 of 45 stages, 85 of 675 offline
redeals find no deal in the original) and the gain are in `docs/LITE_MOD.md`. Everything else looked at (the free-tile scan, the move count, the hit test, the deals of a new game)
costs under 0.2 ms a call and is not touched. With the option off, or without a mods folder, the game is the port as it was: the frozen screens and the dealtest's boards are unchanged.
The tools: `tools/realplay/shuffletest.py` and `memprobe.py`, `tools/profile/{altconfigs.sh,showconfigs.py,altmem.sh,showmem.py,stageredeal.py,shufflepairs.sh}` (in the
Ancient Japan folder; both games use them), and the `dealtest`'s environment options (`STAGE_REDEAL`, `QUICK_REDEAL`, `LOGIC_BENCH`, `REDEAL_BENCH`, `DEAL_ONLY`).

## Coverage check (2026-11-08)

The first whole coverage check of Ancient China (`docs/COVERAGE.md`, "A game's whole check"), made after Ancient Japan's
(`games/mahjongjapan/docs/GAME_NOTES.md`, same title), whose runs, scripts and settings it reuses. origrun's `--coverage`
recorded which of the original's functions ran, in 24 runs on the host (12 on 2026-11-08, 12 more on 2026-11-09 for the
plays the first round could not make) (`games/mahjongjapan/tools/coverage/run.sh` and
`mkscript.py`, which write and start both games' runs; China's places are a block in `mkscript.py`; the hits are in
`build/coverage/mahjongchina/hits/`). The adventure's and the classic game's runs used origrun's virtual clock; the
tour, players, Esc, resume and ending runs ran on the real clock with the silent sound stand-in. Every audio session was
muted from its start and none made a sound.

- **The screen tour**, **players**, **Esc in every state** (`escprobe`), **the resume question**, and **the ending**
  (`chinaend`: China's map has a button that shows the ending again, and its restart box).
- **The escape adventure**: the stages before 46 in four runs (`advA` to `advD`) and stages 46 to 52 with the ending in
  `advF`, the clicks of every stage from the port's solver. **Stage 11 was not played**: `dealtest --stage 11`, the
  solver that also records the tips each move brings (which the script clicks away), gave up on its deal twice within its
  limits (160 tiles, a quarter of them fours). The port's bot does solve it, in 38 moves (it keeps the keys for the locks
  that hold the goal tiles), but the tips of that order of moves were not worked out. It is an ordinary stage with no
  start tip, so its code is what the other stages run; it stays an open item.
- **A whole classic game** on layout 441 (144 tiles), as in Japan: `classicsave` starts it with the deal's seed
  pinned (origrun's `--seed` and `--patches` replace the two `GetTickCount` calls of Board::SetLayout and Board::Start in
  the copy with 12345), makes four moves and leaves by the in-game menu, which saves it; `classicplay` resumes the
  save, plays the other 68 pairs of the solver, takes the result box's next-layout button, and ended at its 400 s
  timeout like Japan's (the next layout began and the last clicks did not reach Quit).

**The deal is checked here as well.** The saved game holds the deal that the port's solver
(`mahjongchina_dealtest --solve 441 12345 25`, `MJ_CLASSIC=1`) produces for the same seed: 0 of the 144 tiles differ
in kind. China's default share of fours is 25 percent (0x19, `User` at +0x18), not 0 as in Japan: with 0 the saved
board differed in 139 tiles, which is how the difference was found, and the first classic runs played the wrong deal's
moves.

**The second round** (2026-11-09) is Japan's, run again on China's places (the same scripts, `mkscript.py` with China's
block; Japan's notes say what each does): `drags`, `nameedit`, `nameclip` (Ctrl+C, V and X as the characters 3, 0x16
and 0x18, with the clipboard saved and put back by `build/coverage/clip-run.sh`; the first attempts were refused
because the clipboard held an image and then HTML, before the guard kept the HTML), `dclicks`, `classiclist` (the cached
sheets moved away by `build/coverage/cache-run.sh`), `nomoves0` to `nomoves2` (a dead-end play of the pinned deal, 64
pairs and 16 tiles left, with each of the box's three choices), `resumeno`, `classicplay2` and `skipfly`/`skipfly2`.
The first `dclicks` run ended at once with a DirectX error from the game's own start (887600E1, the sound device
busy); the rerun was fine. The dialogs sit at the same places as in Japan (the state command shows the same tile-set
box and new-player box), so the same coordinates serve.

`games/mahjongchina/coverage.txt` sets the game's own code as two ranges, 0x401000-0x4171f0 and 0x41dea0-0x4374f0, 1274
functions. The runs reached 1099 of them, the port defines 985, and 354 are listed as left out on purpose.

**The port's comments carry both addresses.** The code is shared with Japan's (`engines/jollybear`,
`engines/mahjongescape`), so the comment above a function gives China's address and Japan's, each with its game's name
after it (`0x420860 (mahjongchina), 0x42a9f0 (mahjongjapan)`), and `tools/coverage.py` reads the addresses tagged
`mahjongchina`. China's were set from `re/mahjongchina/compare/addrmap.tsv` when the two ports became one code: code
that only Japan's exe has keeps Japan's address alone, and a range whose end the map could not pair ends at the next
function in China's exe. The map pairs the two exes' functions (4460 rows when this began); at
first the report showed 200 functions reached in the original with no counterpart and 103 neither reached nor
defined, many of them simply missing from the map. `games/mahjongchina/tools/pairgaps.py` adds the pairs the map
lacked: the two builds keep their functions in the same order, so the functions between two mapped neighbours of one
build are paired with those between their images in the other (in order, and by size or callees when the counts
differ), and a call-graph pass pairs what is left by the callers and callees that are mapped. It added 112 rows (110
by the gaps and the call graph, 2 by hand: `TextEdit::Update` 0x428010, where China's compiler inlined
`Object::IsActive`, and the tip box's destructor 0x408c80, which the map had paired with another destructor of the
same shape). `python -I games/mahjongchina/tools/pairgaps.py --write` redoes it on the original map.

**China's exe puts the device layer and libraries inside its game ranges.** The JPEG and PNG decoders, the Direct3D
and DirectDraw wrapper and other device code sit between the game's functions (0x42ba30-0x42d4a0, 0x42d720-0x430480,
0x4328a0-0x432ed0, 0x433050-0x4342d0 and more), where Japan's exe keeps them outside. They are `none` lines with the
reason that the host and the platform replace them. The compiler-made functions and those the host replaces were
sorted for Japan, and their China addresses are the 110 `none` lines translated through the map.

**Reached in the original with no counterpart in the port: 200 at first, 0 at the end.**

- **Left out on purpose, as `none` lines with a reason (146 in all, 3 of them the ones it started with):** the
  compiler-made functions (scalar deleting destructors, static initializers and their `atexit` registrations,
  `CString` and `std::string` helpers, the crash handler), and the platform's (`WinMain`, the window, the registry,
  `LinkButton`, the Direct3D path, the sound mixer), and the device layer above.
- **Placed:** the inlined functions got their Japan address in the comment above the function that holds them, as
  in Japan (Object's defaults, the manager's list add and remove, the layout name copy, the keyed copy's row loop
  0x42e960, the user and shared folder getters); China-own addresses in `GameDialogs.cpp` and `.h`
  (the level-done scroll's functions) and two comments in `Board.cpp` and `MahjongApp.cpp` were changed to Japan's, since
  the map would translate a China-own address wrongly. The options box's `OnInit` (0x4152a0 in China), whose
  `SetUserFlag(true)` the compiler inlined, is a `none` line: the port defines it in `Options.h` under Japan's 0x401060.
- **Missing logic: the two of Japan's,** now in China's port: the window's key slot leaves full screen on Escape
  (`MahjongWidget::KeyDown`), and the exe's version string for the logo screen (`MahjongApp::Init`).

**Never reached and not defined in the port: 103 at first, 0 at the end:** two vtable-only stubs, a default virtual that
returns false (0x401040) and a scalar deleting destructor (0x432870), are `none` lines; the rest are the same sorts
as above.

**Defined in the port, never reached: 92 after the first round, 46 at the end.** Every one is a function whose Japan
counterpart (through the inverse map) was read again against the export in Japan's check (28 R rows, reachable by a play
the runs have not made, and 18 U rows, reachable by none). All matched but two, which China had as well and now has
fixed, like Japan:

- **0x42a340, `Layer::AppActive`** returned the display's flag where the original asks the manager's active flag, so
  the pause box a game shows when it starts with the window inactive never appeared. `Layer::AppActive` now returns
  `mManager->GetActive()`.
- **0x42ed70, `Image::SaveBmp`** left the header's size-of-image field zero; `Put32(34, aSize)` writes it.

The China-only functions (the map's ending button, `MapView::OnTimer`, `OnLButtonDown`, `MapLayer::ShowEnd`) were
unreached until the last runs: `advF` (the last stages and the ending) reached two, and the classic game's second run the
third. The second round reached the rows of the sliders, the name box and its clipboard keys, the double clicks, the
right click, the no-moves box, the list built from scratch and the question answered No.

**The third round** (2026-11-09) is Japan's `modkeys` and `rdclicks` (`GAME=mahjongchina`, origrun's `--keys scripted --clock
virtual`: the Shift and Ctrl state, `char 0x1a` and `rdclick`; Japan's notes say what each sends). The logs say 90 and 50 key and
button edges posted, "1 game process patched GetKeyState", 60 and 94 answers with the scripted state and 26 from Windows
before the first. They reached `Manager`'s right double click (0x421970), `Layer::FocusPrev` (0x4239b0) and Ctrl+Z's
handler (0x4324e0); `rdclicks` ended at its 300 s timeout, as in Japan.

The 43 still unreached (46 before the third round) are Japan's 22 R rows' counterparts (Japan's notes list them: the display
toggles, minimize and restore, the escape adventure's right click, wild links, the map double
click, the player list's scroll arrow, and the error box) and five that Japan's runs reached and China's did not:
`Board::SkipFlyOff` with `Layout::TakeKinds` and `TakeSpecials` (Enter during the first stage's fly-off did not reach
`Board::OnChar` in China's `skipfly` and `skipfly2`, with 3 s and 6 s waits; the reason was not found), and
`Deal::DealAny` and `DealOne` (the solver giving up on a deal: Japan's shuffle in the no-moves box reached it, China's
did not).

**Candidates for taking out unneeded code: 18**, the same as Japan's U rows that China has (taken out or kept as Japan's are, in the section "Code the port does not need" below):
the deleting destructors of `Dialog`, `SavedGame`, `MahjongGame`, `Deal`, `Layout`, `User` and `PlayerList`;
`Object::SetVisibleByParent`, `GetDirty` and `SetDirty`; `Font::LoadKeyed`, `DrawWrapped`, `SetTabStops` and
`GetTabWidth`; and `Animation::Stop`, `Hold`, `StepLoop` and `StepPingPong`. The reasons are in Japan's notes.

**Not in the address map: 147 comment addresses.** The port's comment gives Japan's address, and the map has no China
partner for it: 95 of them are functions of Japan's exe (most of them of 16 bytes or fewer, thunks and getters, and the
list operations of the splash, the display and the manager) and 52 point inside a function. The report cannot say whether
the original ran them. This is the weakest part of China's check, and a map made by a better pairing would shrink it.

**Open items.** Stage 11's play; the 28 R rows above; and the 147 unmapped addresses.

**What it cost.** The 24 runs took about 57 minutes of machine time (30 for the first 12, 27 for the 12 of the second
round; 400 s each for the two classic games' second runs), at most three game processes at a time. The sorting and the re-read were Japan's work (two Sonnet agents; tokens not counted),
reused through the map; China's own work was `pairgaps.py`, the 146 `none` lines, the classic game's deal, and the runs
of the stages and the ending.

## Code the port does not need (2026-11-09)

The survey and the table are Ancient Japan's (its section of this name: the six scans, each removal with its address and
reason, what was looked at and kept, the proof). The engine is one at the same addresses and the sources match line for
line apart from the game's own data, so every removal there was made here the same way: the registry strings and
`GetCmdInt`, `Layer::PreloadImage`, `Manager::OnSize` and `Redraw` with the flags and the global they left unused, the keyed
fonts (the `theKind` parameter of every font call), the tab stops and the dialog text kinds 2 to 4 (all 57 `AddText` calls
of China pass kind 0 or 1 and no stops), the dialog text's delay and fade, its subtitle, the button's overlay, the
`Animation` modes, `Stop`, `Hold` and the keyed draw, and the members nothing writes (`mAttachmentsUnder`, `mClassName`,
`mDesktopBits`, `mNoWidthCheck`, `mHeld` and the rest).

What the scans found here, on the 39 object files and the 38 sources of the native build: after the removals 182 functions
with no relocation, none whose name appears only at its declaration and its definition; the compiler warnings of the
list in Japan's section give 996 unused parameters (the virtual hooks' signatures) and nothing else; every one of the 84
constants is used; and the only `_WIN32` blocks are the memory report of the test commands and `main.cpp`'s search for the
Game Collection's install. The 18 U rows of the coverage check are taken out or kept as Japan's are (its "Looked at and
kept"). The test hooks are the same 1,300 lines as Japan's.

**Proof.** Both native and web builds pass. `tools/check.sh mahjongchina` after the removals: 178 layouts dealt with none
failed, 31 pairs, the three screens as frozen, a rerun identical. The screen tour of Japan's `tools/tour/` (it runs
either game) against the exe built before the removals: 20 screenshots, all byte for byte the same (16 distinct images),
and the before exe's rerun the same too.

## The readability pass (2026-11-11)

The pass is Ancient Japan's, made here the same way (its section of this name has the rules, each kind of change and the
method): the same 206 renames by scope, the tile's `mBobsAsHint` and `mLiftStart`, the magic tiles' code in the game's own
words (`TakeGold`, `TakeKeyAndLock`, `TakeJoker`, `TakeMagnet` and the names built on them), the tile kinds, the flight
kinds and phases, the screen size and the cursor kinds, the port markers, the comments and the wrapped lines.
`python tools/namecheck.py games/mahjongchina` has no finding (its `namecheck.txt` adds `engines/jollybear` and `engines/mahjongescape/src` to the check).

The resource names are China's own where its resources are: `src/Res.h` names the 350 ids its code uses, in 602 places.
An id that both games have means the same thing in both (the string tables match but for five strings Japan adds and
two China uses, and a picture of the same id is the same part at another size), so those share Japan's names. China's
own are the bamboo frame and corners of the play panel, the dialog's knots and dragons, the level-done box, its menu
lanterns, five fonts and a third piece of layout music.

**Proof.** The object code of the 39 object files of the native build (1,735 functions, names taken out, and the data
sections' bytes) is the same as before the pass. `tools/check.sh mahjongchina` passes (178 layouts, 31 pairs, the screens
as frozen, a rerun identical), the screen tour's 20 screenshots match the exe from before the pass byte for byte, and
the web target builds.

## Oddities (not chased)

**Memory above the original's in play (2026-11-16).** The port's private bytes peak at 44.2 MB in the 60 s of the scripted play (800x600 window; 42.5 at 2x, 44.9 full
screen) against the original's 32.4 MB, which is flat from its title; at the title the port is at 25.6 MB, below it. The growth of about 15 MB between the main menu and a stage's board has no
split: the Mahjong engine's `memreport` reports one picture (the screen) and no sound, so the pictures, sounds and music are not told apart. To see it: `tools/realplay/memprobe.py <out> --game china --memmap`
under rundesk (the `memreport` line and `tools/memmap.py`'s summary at the menu and on a board) and `tools/profile/altmem.sh` for the peaks. What it would take: the engine's `memreport` filled in as
`platform/MemReport.h` describes (pictures from the display's image table, sounds from the mixer, the songs), then each holder against what the original's DirectDraw surfaces and DirectSound buffers hold. The
processor-time target is met at every size, so this was not chased.
