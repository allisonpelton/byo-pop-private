# Mahjong Escape: Ancient Japan: notes on the original

This file records what the translation found in `MahjongEscapeAJ.exe` (linked 2006-11-08), for whoever reads or changes
the port. Addresses are the original's, and the port's declarations carry them as `// 0x...` comments, so a grep finds
both sides. `ENGINE.md` maps JollyBear's engine under the game. `DEVIATIONS.md` lists where the port's code differs.

## The program

The game is SpinTop Games' and runs on JollyBear's engine: an app object (0x424a30, vtable 0x51df94) whose manager
keeps a stack of layers and sends them a frame every 25 ms. Each layer is a tree of objects. The game's layers are
the splash, the main menu (0x40f540, 0x40f5a0), the player list, the classic list of layouts, the map of the escape,
the play layer with its board, the ending, and the dialogs over them: the options, the tile sets, the backgrounds,
the help pages, the tips and the yes/no questions.

The game's resources are in two modules. `Resource.dll` holds the pictures, the sounds, the music, the 222 layouts
(`LAYOUTS` resources: 60 for the escape, 162 for the classic game) and the longer texts (`TXT`). The exe holds the
string tables of the menus and dialogs and two pictures named in its folder (`SplashLogo.bmp`, `MenuLogo.bmp`). The
port reads both modules with its own PE parser (`src/PeResources.cpp`) and never runs either. The game's texts are
read from them when the game runs; the few the port needs from the exe's code are named by address in
`src/GameTextTable.inc` (made by `tools/make_gametext.py`) and are never copied into the repository.

## Saves and settings

Each player has a file in the user folder: `list.usr` lists the players, and `<n>.usr` holds a player's settings,
best times and scores (one of each per classic layout and per escape stage), the escape's progress and the chosen
tile sets (0x415eb0; a CRC that also covers the exe's folder guards it). A game in progress is saved as a file of
its own per level. The app's settings (the volumes, full screen, wide screen, the hardware acceleration and the window's
place) are in the registry under `HKCU\Software\JollyBear\MahjongEscapeAncientJapan`, which the port emulates in
`registry.ini`.

## The rules

A tile is free when nothing lies on it and its left or right side is open (`Layout::IsFree`, 0x414c20). Two free
tiles of the same kind make a pair; the four seasons match each other and so do the four flowers. A pair scores its
kind's points, plus 500 when it is the same kind as the pair before. A hint costs 100 points. The undo takes the
pair's points back. A shuffle adds 20 seconds to the clock. "Show free tiles" shades the tiles that cannot be taken.
The autohint (an option) gives a hint after a while without a click.

**Emperor's Adventure** is 60 stages along a path through twelve periods of Japanese history, from the Jomon period to
the Taisho period. A stage is won by matching the two Magic Gold Tiles (kind 0x2b) once both are free (0x40d8b0). The
tiles still on the board then fly off: 200 points for each pair, 500 for each special tile. A level's end shows the
time, the score, the bests and the period's saying, and the map walks the marker on to the next stage. The first
stage's layout gives every tile its kind. The later stages give only some tiles their kinds and deal the rest, seeded
with the stage's own number from the stage table (TXT 0x17f), so a stage is the same every time it is played. Later
stages bring special tiles: a key (0x2c) that opens a lock (0x2d, which can be picked only with the key selected and
need not be free), and two tiles (0x2e, 0x2f) that pair with any ordinary tile that can be seen, free or not. A right
click on a special tile shows its tip. Each new thing gets a tip box the first time it appears in a level, until that
level has a best score.

**Classic** is the 162 layouts of the list, nine to a page, each dealt anew from the clock. A game is won when the
board is empty. The options' difficulty sets the share of the deal that goes in fours rather than pairs: 0 percent
for relaxed (a new player's), 25 for normal and 85 for expert.

Both modes have the same bar of buttons (undo, hint, shuffle, the menu and pause) and the same panel (the time, the
score, the bests, the moves available and the tiles left). The in-game menu goes back to the main menu, chooses a new
layout, opens the options, the tile sets, the backgrounds and the instructions, or resumes the game.

## The trial

The exe is the full game of the PopCap Game Collection. The translation found no trial code: no registration, no nag
and no buy screen.

## The standard test commands (2026-11-08)

The port answers the standard commands (`docs/TEST_COMMANDS.md`) from a test's `-script`, its live channel
(`-commands=<file>`, `byopop_test_command` on the web) or `-realinput`, through `HandleScriptCommand` in
`src/TestCommands.cpp`. A command that is never given changes nothing: no draw of the game's random numbers and no state.

| Command | What it does |
|---|---|
| `state` | `STATE {json}`: `screen` and `dialog` (the top layer and the dialog on it, null for none), `busy` (a transition or a flight is running), `player`, `progress` and `levels` (the adventure), `updates`, `layers`, and on a board `mode`, `level`, `layout`, `score`, `timer`, `moves` (the free pairs), `tiles` (left) and `board` (`started`, `over`, `escaping`, `blocked`, `selected`, `move`), and `buttons`, each `{cmd, x, y, w, h}` in game pixels, of the top layer. |
| `goto <screen>` | `menu`, `options`, `help` (from the main menu), `classic [n]` (the list, or layout n at once), `escape` or `map`. Answers once the screen has settled. |
| `level <n>` | The adventure's level n, started as the map's Continue starts it. |
| `unlock all`, `set score` or `set progress` with a number | The adventure's progress to its last level; a score or the progress. |
| `win` | The game's own moves, from the bot's way to the end of the game, until it ends. |
| `move` | `MOVE {json}`: the bot's next move and the real input that makes it (below); changes nothing. |
| `lose`, `seed <n>` | `ERROR`: a game here is never lost (a board with no pair offers the shuffle), and every deal has its seed from the game's own clock (the fixes mod's `-gameseed` is the seed). |

**The bot behind `move`** (`src/Bot.cpp`, one file for both games; `tools/realplay/syncbot.sh` copies it into China's
folder) works on a copy of the board's layout with the game's own rules for a free tile and a pair. It searches depth
first for a series of pairs that ends the game (the two Magic Gold Tiles of an escape stage, the empty board of a classic
game), remembers the positions that proved dead, and starts again with another order of the pairs and a larger budget
when one try fails, up to 60 tries. The key (0x2c) takes the lock (0x2d) it opens, and the wild tiles (0x2e, 0x2f) pair as
the game lets them; the first tries save the keys for the locks that hold a goal tile. Every one of the 60 escape stages
and every classic layout solves, the slowest stage in about 3 s. When no pair is free the bot answers with the shuffle's
click, and `MOVE null` while a flight runs, on a board with no tile left or when nothing can be played. A dialog is
answered by its default button (command 1), the pause box by a click on it, the map by Continue.

## Fix candidates and the fixes mod (2026-11-08)

The fixes mod (`mods/fixes/mod.json`, two toggles, both on by default on the site) is the original with two things
mended that leave the play to chance or the timing to the machine, and nothing else. With the mod off, or no mods folder, the game is the
original; `-nomods` starts a native build without it and `-modsdir=<folder>` loads another folder.

- **`seededDeals`.** The original seeds the generator from the clock at every deal (a classic game's, each shuffle's
  redeal, and an escape stage's own number from its table). With the fix the deals and shuffles of one run draw their seeds
  from `-gameseed=N` (the page's `?seed=N`): the same series on every run, whatever time each starts. The engine's own
  reseeding when it makes an object or a stream (0x41ec10, 0x424d30) is not touched. Without a seed given nothing changes.
- **`steadyClock`.** The game's timers, frame pacing and the panel's clock run on its updates (5 ms each) rather than the
  machine's clock, so the same input plays the same game on every run, at any speed of the machine. Not under `-fast`,
  whose virtual clock does the same. The deal's search keeps its time limit on the machine's clock.

`tools/realplay/modtest.py` checks both with the shipped exe, and the start (the menu at update 280, and at 1540 with
`-logos`): the first eight pairs of a classic deal and of its shuffle equal under one seed whatever the start time, another under another
seed, and different without the mod; the panel's clock after 400, 800 and 1200 updates in real time showing 2, 4 and 6 s.

**Fix candidates that were checked and are none.** The panel's `AddSeconds` (0x409d30) adds its seconds in milliseconds
when the clock is not running: a shuffle before the first pair added 20 s as it should (the panel showed 24 s from 2 s,
with the real time between), so no case reaches the odd branch. No layout of the 222 deals a free tile that cannot be
clicked at the start (`MJ_FLOATING=1 mahjongjapan_dealtest`: no floating tile, 0 unreachable). The shuffle's cost of 20 s
and the free-tiles option are as the original's.

**Fixed in the port itself** (a bug of the original that breaks play: `docs/MOD_ROADMAP.md`, "The fixes mod"). A click on
Shuffle while the last pair of a game is still flying (a second or so) started a shuffle whose redeal cleared that flight
(`Board::Reshuffle`, 0x40b0a0), and the flight's end is what closes the round (the `mLeft == 0` test in
`Board::Update`, 0x40bd70): the board stayed empty with the clock running and no result box. The port ignores the shuffle once no
tile is left (`Board::OnCommand`, 0x40b500). The real-input driver found it, its bot clicking Shuffle on the empty board.
It is listed in `DEVIATIONS.md`.

## The start (2026-11-08)

The original opens with two logo screens (`Splash`, 0x425320, holds `Logo`, 0x42f650; `Logo::Update`, 0x42f8c0, counts 2 s
each and reads no click or key) and lets 0.77 s of black pass before the first of them (`Manager::SwitchTo`, 0x42ae80, fades
out of the screen there is none to fade out of), with nothing loaded meanwhile. That is a wait the original put in before the
game can start, so the loading rule takes it out of the port itself (CLAUDE.md, "Making a port"): the first layer is the main
menu (`MahjongGame::CreateFirstLayer`, 0x40f540) and the first `SwitchTo` has no fade-out (`jb::gFastStart`). `-logos` keeps
the original's start, for runs compared with the original: `tools/check.sh` passes it, so that its scripts keep their timing,
and `modtest.py` and `perfload.py` take it. It is a deviation that can be seen, made on purpose (`DEVIATIONS.md`); until
2026-11-08 it was `skipLogos` of the fixes mod. The launch to the main menu took 7.8 s and takes 1.3 s.

The rest of the launch was looked at for other waits of the original's making and has none. The game sleeps only to pace its
frames (`Manager::Frame`, 0x42bd40), a layer loads whole when it is entered (`Layer::SetManager` calls `Load`; no slices over
frames, no minimum time), and what remains is about 1.0 s from the process's start to the menu's first frame (the exe and
Resource.dll opened, the texts, the display, the menu's pictures and music) and the menu's fade-in, 0.8 s, which takes clicks
while it runs (the layer being shown is active), so it is an animation that does not hold the player and stays. A `-fast` run, which never waits, takes 0.7 to 0.9 s from the launch's return to the menu's first frame on this busy
machine (three runs; the real-time runs 0.4 to 1.4 s), so that part is loading work and not a wait; it is for the performance
pass (`docs/COORDINATION.md`).

## Registration, links and the loader (2026-11-08)

The game has no registration code (above). The two buttons that lead out of the game were taken out: the PopCap and
SpinTop logos at the main menu's bottom corners opened their web sites (`LinkButton`, `ActivateLinks`); they remain as
pictures and no longer take a click. Nothing in the program opens a URL. The pictures are decoded by the shared libjpeg 6b
loader (`platform/ImageLoader`), as the original's own libjpeg did (0x438000), where stb_image decoded them before; the
frozen screens of `tools/check.sh` were taken again for that (the old and the new shots of the suite differ by at most 3
levels of a channel, in the JPEG backgrounds only; everything else is identical pixel for pixel).

## Real input (2026-11-08)

`tools/realplay/drive.py` plays the game from the main menu with real SDL events, posted by the framework's `-realinput`
channel under rundesk: the menu and its help, a new player's name typed, the adventure from its map level by level to the
ending, the classic list and two classic games (the first by the bot to its end, with every button of the board and of the
in-game menu tried first; the second ended by `win` after ten real pairs), the focus lost and regained, and Quit. The driver
reads the state, asks `move` and sends the input it names; it chooses nothing. The whole adventure (60 levels) ran by real
input in 23.6 minutes of real time and reached the ending. `--fullscreen` plays the same after Alt+Enter, the window going
from 3200x2400 to 1600x1200 pixels of the private desktop and back (`screenshotfull`). The web driver
`tools/web/realplay.js` plays the same through the DOM's events on the page (`?driver=`), `modtest.py` checks the three
fixes, `perfload.py` times the launch.

## Performance (2026-11-09)

The performance pass (`docs/COORDINATION.md`, "A performance pass before the readability pass"): the original and the port
measured over the same minute of play, what the profile shows, what was changed and what is left. The machine is a Core
i7-12700F with Windows 11; the port ran windowed, in real time, with no mods, under rundesk (below normal priority, SDL's silent
audio), and the original through origrun on the host (its own private desktop and silent stand-in: every audio session was
muted from its start and none made a sound; the user's saves were put back after each batch). The input is the minute of
play the suite `tools/check.sh mahjongjapan` sends (the menu, a new player, the adventure's first stage won by the solver's
clicks, back to the map). `tools/profile/perfscript.py` turns the suite's script into the port's `-script` and into the
original's origrun script (the original's two logos and the fades move its play 6 s later), `tools/profile/perfrun.sh` runs
either under `games/bejeweled/tools/profile/perfmin.py`, which samples the process every 50 ms, and `abstats.py` takes the
medians of runs made in turn. The processor figures are for the window 20 s to 50 s of the play. Other sessions' games ran on
the machine meanwhile, and the clock of a lightly used core moves with the load: the same build measured 12.0 s of processor
time over the whole run in one batch and 5.9 s in a quieter one. So only runs made one after the other compare, the two builds
alternating, and that is how the "before" and "after" columns were taken (5 runs each, the last batch of the pass).

| | original (3 runs) | port before the pass (5 runs) | port after the pass (5 runs) |
|---|---|---|---|
| Processor time in the window | 3.1 % of one core (2.7 to 4.1) | 10.0 % (9.2 to 12.1) | 6.0 % (3.6 to 10.1) |
| Processor time, whole run | 2.3 s (1.9 to 2.6) | 5.9 s (5.1 to 6.6) | 4.3 s (2.9 to 5.8) |
| Peak working set | 56.8 MB | 155 MB | 153 MB |
| Peak private bytes | 40.1 MB | 148 MB | 146 MB |
| Launch to the main menu, real time (`tools/realplay/perfload.py`, 5 runs of 2026-11-08) | | 1.31 s median (1.28 to 1.32); with the original's start (`-logos`) 7.80 s | not touched by the pass |

Earlier batches of the pass, made while the machine was busier, show the same direction with larger numbers (the window,
medians of 5 runs of each build in turn: 22.6 % before and 16.5 % after the blit and Present changes; in a second batch 15.0 %
after those and 9.3 % after the upload change). The port still takes about twice the original's processor time and nearly four times its memory; the rest is listed
under "What is left" below.

**Where the time goes.** The first tool, `tools/profile/callprof.py`, which suspends the main thread and reads its
instruction pointer, put 86 % of the samples in ntdll whatever the thread was doing (the suspension itself), so its
function table is not used. What the pass relies on are temporary counters of the calling thread's cycles
(`QueryThreadCycleTime`) around the sections of a frame, in builds that were never committed, over 60 s of the same play, and
`tools/profile/threadcpu.py` (`GetThreadTimes` of every thread, no suspension). The main thread's cycles, per 60 s of play:

| Build | thread | game updates (engine Draw and Update inside) | of which the copy of the engine's buffer into the widget | widget draw (the texture upload) | `DDInterface::Present` (`SDL_RenderPresent`) |
|---|---|---|---|---|---|
| after the blit and Present changes | 9.4 to 12.9 G | 4.8 to 6.7 G | 0.74 to 1.08 G | 2.3 to 3.2 G | 1.4 to 1.8 G (1.3 to 1.8) |
| after the upload change | 7.9 to 10.6 G | 4.8 to 5.9 G | 0.74 to 0.93 G | 1.1 to 1.4 G | 1.1 to 2.3 G (1.0 to 2.2) |
| the same, with the engine's own probes | 6.6 to 6.8 G | 4.4 G (Draw 3.7 to 3.8, Update 0.8) | 0.64 to 0.67 G | 1.0 to 1.1 G | 0.43 to 0.46 G (0.38 to 0.41) |

Two things follow. The game's own work is the engine's software drawing of 800x600 pixels: 3.7 G cycles of the 6.6, three
blit functions making up most of it (the alpha blend 1.3 to 1.4 G cycles for 330 to 350 Mpixels, the copy that keeps an
image's colours 0.9 to 1.0 G, the colour multiply 0.5 G). And `SDL_RenderPresent`, the video driver's, costs 0.4 G cycles in one play and 2.2 G in
another of the same build, as the GPU is shared with whatever else runs: that, not the game, is why the percentages of a
single run swing (3.6 to 10.1 % above) and why runs have to alternate. The process's other threads (the sound mixer, the driver's own) are not
the game's code and were not changed.

**What the pass changed**, each proved to leave every drawn byte and every random draw as it was (`tools/check.sh` against the
frozen baseline passes with every picture and checksum the same):

- **The engine's blits** (`engines/jollybear/Blit.cpp`): the blend of one row went a pixel at a time through three per-channel sums
  (`Div255`), and a source pixel that is opaque or empty, nearly all of them in this game's art, now skips them: an opaque
  pixel (alpha 255) is stored as it is, which is what the sums give (`Div255(x * 255) == x`), an empty one (all four bytes
  0) leaves the pixel under it. `CopyRgb` and `SetRgb` take a pixel as one word instead of three bytes. 12000 random cases
  of `tools/profile/blittest.cpp` compare each function with the original loops byte for byte.
- **The screen's way to the widget** (`src/MahjongApp.cpp`, `MahjongHost::Present`): the engine's buffer went into a vector
  first and from there into the widget's image with a pass over 480000 pixels for alpha; it now goes in one pass, the alpha
  byte set as it is written, and the vector is gone (1.92 MB less).
- **`MemoryImage::mUploadAsIs`** (shared: commit de23422d, off by default, so no other game changes): `DDInterface::PrepareTexture`
  copied every pixel of an image into a buffer and premultiplied it, a strip of 65536 pixels at a time, for any image on the
  renderer the machine has; for the screen image, which is opaque, a premultiplied pixel is the same pixel, so both ports
  set the flag and the image goes up in one `SDL_UpdateTexture` call as it is. The widget's draw went from 2.3 to 3.2 G
  cycles per 60 s to 1.0 to 1.4.
- **`memreport <file>`** (a test command of this port, `src/TestCommands.cpp`, only from a test): what the engine holds,
  by image and by sound (decoded bits and 32-bit copies, sounds and their bytes), the process's working set and private bytes,
  and which music modules are loaded. It prints no number of the game's state and draws nothing.

**Memory.** At the start, before one game file is read, the process holds 54.6 MB (the framework, SDL and the video driver's
libraries: shared). After the menu, the map and a board it holds the engine's images (14.9 MB of decoded bits and 17.7 MB
of 32-bit copies, 81 images), 5.5 MB of sounds (19) and, once the music plays, its two MO3 modules (ids 335 and 339,
decoded by libopenmpt): about 51 MB, measured as the difference of a build that never loads them (93 MB private at the end
of the play, against 146 MB with them). The original keeps 40 MB private; why it holds less was not examined beyond these
owners.

**On the web** (the Browser pane, Chrome 152, the page's own timers, no `fast=1`; `tools/profile/webperf.js` is the driver:
8 s of each phase, the game's frame time from the framework's frame hook, the memory from `byopop_perf_json`; one run each
of the build of 2026-11-08, before, and of the rebuilt one, after; a page left hidden for minutes has its timers throttled,
so every run used a fresh tab):

| | before | after |
|---|---|---|
| Main menu: game's time per frame, its share of the thread | 0.85 ms, 4.0 % | 0.58 ms, 2.7 % |
| Adventure map | 3.81 ms, 16.1 % | 3.36 ms, 13.9 % |
| Board, nothing moving (7 frames drawn in 8 s) | 0.03 ms, 0.2 % | 0.03 ms, 0.2 % |
| Board, ten pairs by the bot | 4.34 ms, 24 % | 3.26 ms, 13.6 % |
| Wasm heap, and the bytes malloc has handed out, at the end | 122.5 MB, 109.4 MB | 122.4 MB, 107.3 MB |

The game takes at most a quarter of the page's main thread, and a drawn frame (4 to 6 ms) is well inside the 25 ms a frame has.
The 2 MB the build holds less is the vector that went. The heap steps from 49 MB at the menu to 122 MB at the map.

**What is left, for the coordinator** (shared code, or larger than a speed-up): `SDL_RenderPresent` and the texture upload
(1.0 G cycles per 60 s) are the framework's: a partial update of the screen image or a streaming texture would cut the
upload, and the driver's spinning in Present is out of the game's hands; the 54.6 MB the process holds before any file
is read is the same in every game; the music modules decoded whole into memory are 51 MB here (a streaming render would
hold one buffer); the engine keeps each image's decoded bits (14.9 MB) beside its 32-bit copy, and which of the two a blit
needs after the upload has not been examined; the engine redraws its whole 800x600 from the topmost opaque layer whenever
anything moves, at about 4 cycles a pixel in the blend; redrawing only the changed rectangle would be a larger change of
the engine than the pass made.

## Coverage check (2026-11-08)

The first whole coverage check of Ancient Japan (`docs/COVERAGE.md`, "A game's whole check"). origrun's `--coverage`
recorded which of the original's functions ran, in 27 runs on the host (16 on 2026-11-08, 11 more on 2026-11-09 for the
plays the first round could not make) (`games/mahjongjapan/tools/coverage/run.sh`
stages a copy and runs it under rundesk; `mkscript.py` writes the scripts; the hits are in
`build/coverage/mahjongjapan/hits/`). The adventure's and the classic game's runs used origrun's virtual clock (no
waiting); the tour, the players, Esc, resume and end-box runs and the first stage's second run (`adv-1-real`) ran on
the real clock, with the silent sound stand-in, so that the mixer's code runs. Every audio session was muted from its
start and none made a sound.

- **The screen tour** (233 script lines): the logos, every menu and dialog (help, options with the sliders, the
  auto hint and the three fours percentages, the quit question, the player list, the classic list and its pages), a
  classic board and the in-game menu (1172 functions).
- **Players** (new, list, switch, delete), **Esc** pressed in every state (`escprobe`), **the resume question**
  (a saved classic game and the Yes and No answers), **the end boxes** (the quit question and the restart box).
- **The escape adventure** from the first stage to the ending in five runs (`advA` to `advE`, 261 to 3493 clicks
  each), the clicks of every stage from the port's solver, with the start tips, the timed tips and the special
  tiles; the first stage again on the real clock (`adv-1-real`), and three short probes made while the scripts were
  written (`adv-0`, `adv-3`, `adv-test`).
- **A whole classic game** on layout 441 (144 tiles): `classicsave` starts it with the deal's seed pinned (see below),
  makes four moves, leaves by the in-game menu and so saves it; `classicplay` resumes the save (the resume question
  answered Yes), plays the other 68 pairs of the port's solver, waits for the result box (`ResultBox`, the best-times
  line) and takes its second button, the next layout (`PlayNext`, `StartLayout`). That run ended at its timeout, not by
  the game exiting: the next layout began, and the script's last clicks did not reach Quit.

Every other run ended with the game exiting normally.

**The second round** (2026-11-09) used what origrun gained for this game, which has no update counter: `down`, `up`,
`rdown` and `rup` as plain script lines (a `wait` between them holds a press, and a `move` in between is a drag),
`dclick`, and the characters `text` posts. Each is a run of its own:

- **`drags`**: a press, a short drag and a release at every 12 pixels along the options dialog's two volume sliders (the
  thumb is wherever the saved volume put it). It needs a player first: on a first run every menu button but Quit asks
  for a name.
- **`nameedit`**: the new-player box of a first run. A click in the name box, the typed name, a drag over part of it,
  a double click on a word, Home, End, the arrows, Delete and Backspace, OK with the name cleared (the message) and OK
  with a name, clicked (the other runs press Enter).
- **`nameclip`**: the same with Ctrl+C, Ctrl+V and Ctrl+X, which this game reads as the characters 3, 0x16 and 0x18
  (`TextEdit::OnChar`), so `text` sends them. They reach the host's clipboard, so `build/coverage/clip-run.sh` saves
  its text formats first (plain text and HTML; it refuses a clipboard with an image or files in it, which it did
  twice), sets "Zeta Eta" for the paste, and puts the saved formats back; the round trip was checked identical.
  Ctrl+Z is the character 0x1a, which ends a script file read in text mode, so Undo was not sent (the third round sends it, with `char`).
- **`dclicks`**: a double click on a layout preview of the classic list (it starts the game), Tab, a right click and a
  right button press and release on the board, a double click on a picture of the in-game menu's backgrounds box and
  one on the styles box (the state command gives no usable place for its pictures: a grid of double clicks over its
  middle), the click on the menu's free part (it closes the menu), and a double click on the board.
- **`classiclist`**: the classic list with its cached sheets moved away (`build/coverage/cache-run.sh`; the files are
  `0` and `9` in the shared folder under ProgramData, put back after the run, the one the paging made removed), so each
  page is built a preview at a time and saved; **`resumeno`**: Play on a layout with a saved game, the question answered
  No.
- **`nomoves0`, `nomoves1`, `nomoves2`**: a classic game of the pinned deal played into a dead end (the port's solver
  with `MJ_DEADEND`: 71 pairs and two tiles that do not match), the no-moves box, and one of its three choices (undo,
  shuffle, start again) and OK.
- **`classicplay2`**: the classic game again, with Enter 3 s after the last pair (`Board::SkipFlyOff` turned out to be
  for the adventure's fly-off only, `mEscaping`), and **`skipfly`**: the first stage with Enter during its fly-off.

**The deal of the classic game is checked, not assumed.** A classic game seeds its deal from `GetTickCount` (Board::SetLayout
0x40aa40 and Board::Start 0x40ae60), which no script can pin. origrun's `--seed` with `--patches` (`docs/ORIGRUN.md`,
"Patching the copy") replaces the two calls in the copy with the seed 12345. The saved game of that run holds the deal
that the port's solver (`mahjongjapan_dealtest --solve 441 12345 0`, with `MJ_CLASSIC=1`) produces for the same seed:
0 of the 144 tiles differ in kind. So the first phase of identical play holds for a whole classic game (the RNG draws of
the deal, in order) as well as for the adventure's stages.

`games/mahjongjapan/coverage.txt` sets the game's own code as two ranges, 0x401000-0x4175b0 (the game) and
0x41e240-0x4319e0 (the engine layer), 1280 functions; the rest of the exe is the C
runtime and the libraries. The runs reached 1156 of the 1280, the port defines 1039, and 261 are listed as left out.

**Reached in the original with no counterpart in the port: 140 at first, 0 at the end.** Sorted by one Sonnet agent
and checked by this session:

- **Compiler- or runtime-made: 97**, as `none` lines with their reason: scalar deleting destructors, the CString and
  STL helpers, static initializers and their `atexit` registrations, the crash handler.
- **The platform or the host replaces them: 75**, as `none` lines: `WinMain`, the window and its message loop, the
  registry, `LinkButton`, the Direct3D path, the CPU test, the DirectSound mixer.
- **Placed: 18**, an address comment above the port function that holds the logic: Object's default virtuals (0x420490,
  0x4204a0), the attachment base destructor, the Resource.dll module getter, the layout name copy, `MahjongGame`'s
  constructor, `AppBase::Init`, the keyed copy's row loop (0x42e960), the user and shared folder getters, and the
  manager's eight list add and remove functions.
- **Logic the port lacked: 2.** Both are now in the port:
  - **0x42fa00**, the window's key slot: Escape in full screen leaves full screen before the game sees the key.
    `MahjongWidget::KeyDown` now does it.
  - **0x426850**, the version string "v.A.B.C.D" from the exe's file version, which the logo screen (`-logos`) draws at
    its bottom right. The port declared `jb::gVersion` and nothing set it; `MahjongApp::Init` now reads the version
    resource of the exe.

**Never reached and not defined in the port: 52 at first, 0 at the end.** All 52 went the same ways (compiler-made,
platform, or placed), and two of them turned out to have a port function that no run reached.

**Defined in the port, never reached: 126 after the sorting, 110 after the classic game, 46 at the end.** One Sonnet agent
read all 126 again against the export (the decompile that shows `this`, the vtables and the callers): 124 match, and two
differed, both now fixed:

- **0x42a340, `Layer::AppActive`.** The port returned the display's own flag (`Manager::IsAppActive`, a field nothing
  changes) where the original asks the manager's active flag, which `Manager::Activate` sets on a focus change. Its one
  user, `PlayLayer::Begin` (0x405500), shows the pause box when a game starts while the window is not active; the
  port never did. `Layer::AppActive` now returns `mManager->GetActive()`. The single name `IsAppActive` still stands in
  for the original's 3D test (0x42a340, always true in the port, a listed deviation) in the two places that use it as
  that; renaming it is left for the readability pass.
- **0x42ed70, `Image::SaveBmp`.** The port left the header's size-of-image field (offset 34) zero; the original
  writes the pixel data's size. The bitmap of every classic list page sheet and of a saved screen differed by those
  four bytes. `Put32(34, aSize)` now writes it. (The original's own sheets in the shared folder have that field set;
  the second round's `classiclist` run wrote new ones with the same header and text file, and different pixels where
  the saved games' thumbnails differ.)

**The third round** (2026-11-09) used origrun's `--keys scripted` (the user's OK of 2026-11-09 to hook `GetKeyState`: this
game asks it whether Shift and Ctrl are down while it handles a key, 0x42c660, and a posted key leaves them up) and its
`char` and `rdclick` lines, in two runs (`mkscript.py` kinds `modkeys` and `rdclicks`; `GAME=mahjongchina` writes China's). A key
with Shift or Ctrl is the modifier's edge, a pause, the key, a pause and the modifier's release, so that the state origrun
posted last is the one the game reads.

- **`modkeys`**: in the new-player box, "Alpha Beta Gamma" typed, Home, Shift+Right five times (the selection), Ctrl+Insert
  (copy), End, Shift+Home, Shift+Left, Shift+End, Shift+Delete (cut), Shift+Insert (paste), Ctrl+Left and Ctrl+Right twice
  each (by words), a click with Shift held (the selection extends), Shift+Backspace, Shift+Tab, and Ctrl+Z twice (`char
  0x1a`), then OK with the name cleared and a name typed. It reaches the host's clipboard, so it goes through
  `build/coverage/clip-run.sh`.
- **`rdclicks`**: Shift+Tab on the main menu and on the classic list, a right double click on the main menu, on the classic
  list and on a board (after a double click on a layout and the tip's OK), Ctrl+Z and Shift+Tab on the board. It ended at
  its 300 s timeout, the game still running after the script's last click; the hits are written when origrun stops it.

Both ran with `--keys scripted --clock virtual`. Their logs end with the line the option writes: for `modkeys` "scripted
keys: 90 key and button edges posted; 1 game process patched GetKeyState, which answered 60 times with the scripted state
and 27 from Windows (before the first)", for `rdclicks` 50 edges and 94 answers, so the hook took in this game's copy. They
reached the three functions no earlier run had: `Layer::FocusPrev` (0x4211f0, Shift+Tab), Ctrl+Z's handler (0x428a40) and
the right double click's (0x42b9b0). China's counterparts are 0x4239b0, 0x4324e0 and 0x421970 (its logs: 60 answers and 94,
26 from Windows before the first).

Of the 126 rows the agent judged 105 reachable by some play the runs had not made (R) and 21 reachable by none (U).
The classic game reached 16 of the R rows, and the second round 64 more (the sliders, the name box, the double clicks,
the Ctrl characters, the no-moves box and its three choices, the list built from scratch, the question answered No,
and the right click). Nothing the runs reached differed from the port in the agent's reading, so the plays confirm
that those paths run in the original; what they do is compared by the identical-play pass. The 43 still unreached (46 before the third round), 22
R and 21 U:

- **Sent in the third round.** Ctrl+Z (0x428a40), Shift+Tab (0x4211f0) and a right double click (0x42b9b0) are reached
  now. The Shift and Ctrl branches inside `TextEdit::OnKeyDown`, where function-level coverage cannot see them, ran in
  `modkeys` (the original reads Shift and Ctrl with `GetKeyState`, 0x42c660), and what they do is for the identical-play
  pass to compare.
- **Changes the machine's display, left alone.** Full screen, wide screen, hardware acceleration and the 3D test
  (0x42c220, 0x42c2a0, 0x42c190, 0x421370, 0x42a340), and minimize and restore (0x42c090, 0x42c110, 0x42c140).
- **Not yet made.** The tip box of a right click on a special tile (0x404ba0, 0x404ec0: the escape adventure's), undo,
  save and load of a move that involves a wild-linked tile (0x40d460, 0x40d510, 0x415770, 0x415790), the double click on
  the adventure map (0x40ffa0: it needs a player with progress and the place of a level's marker), the player list's
  scroll arrow (0x4271f0: it needs more players than fit), and `Layer::SetFocus` from a click in the name box
  (0x421280).
- **Code only an error reaches.** The error box (0x412110-0x412240) for a layout resource that fails to load.

**Candidates for taking out unneeded code: 21 (the U rows).** Taking any of them out changes no play of the
original. The section "Code the port does not need" below took out those the port can do without (`Font::LoadKeyed`,
`DrawWrapped`, `SetTabStops` and `GetTabWidth`, `Animation::Stop`, `Hold`, `StepLoop` and `StepPingPong`, `GetDirty` and
`SetDirty` with the overlay branch) and says why it kept the others.

- The deleting destructors of `Dialog`, `SavedGame`, `Deal`, `Layout`, `User`, `PlayerList` and `MahjongGame`
  (0x404c00, 0x40a3a0, 0x4134d0, 0x414510, 0x415f10, 0x4164c0, 0x424a60): the classes are members, stack objects or
  globals, never deleted through a pointer. `Layout`'s stays (the language rule for a class with virtuals).
- `Object::SetVisibleByParent` (0x41ef30, with the child loop of `Object::SetVisible`), `Object::GetDirty` and
  `SetDirty` (0x41ff50, 0x41ff70, with the overlay branch of `Object::Draw`), the default virtuals 0x4204a0 and 0x4204d0,
  and the dialog's default slot 0x4209e0.
- `Font::LoadKeyed` (0x423bd0), `DrawWrapped` (0x424420, the dialog text of kind 2), `SetTabStops` and `GetTabWidth`
  (0x4248b0, 0x4248f0): no dialog text uses a colour key, kind 2 or tab stops.
- `Animation::Stop`, `Hold`, `StepLoop` and `StepPingPong` (0x4257d0, 0x4257e0, 0x425a50, 0x425a70): every animation is
  mode 3 with infinite cycles.

`Layer::FocusNext` and `FocusPrev` (0x421160, 0x4211f0) run on any Tab but their bodies past the size test never run in
the original either, since no layer holds two focus objects.

**Open items.** The 22 R rows above that can still be run: the escape adventure's right click, wild links and map double
click, and the player list's scroll arrow. Origrun now sends every key the name box reads (the third round); the
function-level report cannot tell whether the Shift and Ctrl branches inside its key handler ran, so the identical-play pass
compares them.

**What it cost.** The 27 runs took about 50 minutes of machine time (22 for the first 16, 27 for the 11 of the second
round; the third round's two took 10), at most three game processes at a time (400 s each for the two classic games' second runs, which ended at their
timeouts). Two Sonnet agents did the sorting of 192 functions and
the re-read of 126; their tokens and this session's were not counted. Most of the session's time went to the
scripts: finding the places on each screen, solving the stages for the adventure's clicks, and getting a classic game
to its end with a pinned seed.

## Code the port does not need (2026-11-09)

The last finishing item before the readability pass (`docs/COORDINATION.md`: "figure out what code does not need to be
present in the decomps"), done as Alchemy's section 10 and Bejeweled's section 22 do it. Registration and the buttons
that lead out were already out. The survey covers `engines/jollybear/`, `engines/mahjongescape/src/` and `src/` (the test hooks, `TestCommands.cpp` and `Bot.cpp`,
which tests need, apart) by six scans:

1. **Functions.** The 39 object files of the native build, every defined function against every relocation in all of them
   (`llvm-nm --defined-only`, `llvm-objdump -r`): 191 had no reference. The aliased forms of constructors and the deleting
   destructors are the compiler's, and the default virtuals are the interface; every other name was searched in the
   sources, and all but six are called, inlined by the compiler or reached by a virtual table. The six with no caller at all
   are in the table. After the removals the scan lists 182, and no name appears only at its declaration and its definition.
2. **Compiler warnings.** All 38 sources with `-Wall -Wextra -Wunused -Wunused-member-function -Wunused-private-field
   -Wunused-template -Wunused-macros -Wunreachable-code -Wunused-const-variable`: besides unused parameters (994, the
   signatures of virtual hooks and handlers) the one finding was the unused template `FindLayer` of the test commands.
3. **Members.** Every `m...` member of the headers by whether any code reads it, and by whether any code writes it other than
   in its initialiser (a member nothing writes is a constant, and the branch it decides is dead; containers and structs that
   are filled through helper functions were checked by hand): the ones in the table, apart from five that the application
   sets once for the framework (below).
4. **Constants.** The 84 enumerators, `#define`s and `const`s by use: all are used.
5. **Pictures and sounds.** The port has none of its own. The game's pictures, fonts and sounds are its files, loaded by
   id from Resource.dll or the exe when the code asks, so there is no list that an id could be missing from.
6. **Windows leftovers, switched-off code.** `_WIN32` appears in `memreport`'s process counters (a test) and in `main.cpp`'s
   search for the Game Collection's install (which a Windows player of the unpacked game needs); no `#if 0`, no `TODO`.

The coverage check's 21 U rows (the section above) were the other starting point. Most were taken out; the ones that stay
are below. The removals, which are the same in Ancient China (the engine is one, at the same addresses):

| What | Why it is not needed | Verdict |
|---|---|---|
| `AppBase::RegGetString` (0x429230), `RegSetString` (0x4293c0), `GetCmdInt` (0x429b90), and `Host`'s string and deletion registry calls (`RegReadString`, `RegWriteString`, `RegDeleteValue`, `RegDeleteKey`, with `MahjongApp`'s) | no caller: every setting of the game is a DWORD (`RegGetInt`, `RegGetBool`, `RegSetInt`, `RegSetBool` stay) and no command line word takes a number | removed |
| `Layer::PreloadImage` (0x420a50) | no caller: the screens preload through `Manager::PreloadDllImage` | removed |
| `Manager::OnSize` (0x42c090), `Redraw` (0x42c0f0), `gManagerAlive` (0x5d8e9e), and the members `mCursorDirty`, `mMinimized`, `mSuspended` | the window's size and paint messages belong to the framework in the port: nothing calls the two functions (`OnSize` was the only code to set `mMinimized`), nothing reads the global or `mCursorDirty`, and nothing sets `mSuspended` | removed, with the `!mSuspended` of `Manager::Frame` and the desktop's depth test of `SetFullScreen` (`0xf < mDesktopBits`, always true for the 32 it starts with; `Display::mDesktopBits` too) |
| `SliderBar::mValue`, `PlayerButton::mTextWidth` (+0xa0, +0xf4: written, never read; the `GetWidth` call that filled the second had no effect), `AppBase::mClassName`, the `FindLayer` template of the test commands | never read or never used | removed |
| `Font::LoadKeyed` (0x423bd0), the keyed branch of `Font::DrawLeft`, `Font::mKey` and `mKeyed`, and the `theKind` parameter of `Display::GetFont`, `Manager::GetFont`, `GetExeFont` and `GetDllFont` | every font of the game is loaded with -1, an image with alpha (36 calls in the engine and the sources) | removed |
| tab stops: `Font::SetTabStops` (0x4248b0), `GetTabWidth` (0x4248f0), `Font::mTabCount`, `mTabs`, the dialog text's `mTabs` and `mTabCount`, and the last two parameters of `DialogFrame::AddText` and `Dialog::AddText` | all 54 calls pass none, so no font ever has a stop and a tab character has the width 0, which `Font::GetWidth`, `FitCount` and `DrawLeft` now say directly | removed |
| dialog text kinds 2, 3 and 4 (wrapped, wrapped centred, right-aligned) and `Font::DrawWrapped` (0x424420, kind 2's only user) | no `AddText` call passes a kind other than 0 (centred lines) or 1 (lines) | removed |
| the dialog text's delay and fade-in (`Text::mDelay`, `mFade`, `mFadeTime`, `DialogFrame::mAnimating`, the loop of `DialogFrame::Update` that counted them down) and the dialog's subtitle (`mSubtitle` and its draw) | nothing sets any of them: `AddText` zeroed the delay and the fade, `mAnimating` is never raised and the subtitle never given | removed |
| the button's overlay picture (`Button::mOverlay`, `mOverlayX`, `mOverlayY`, and the branch of `Button::Draw`) and `Object::GetDirty`, `SetDirty` (0x41ff50, 0x41ff70) that only that branch used | nothing sets an overlay | removed |
| `Animation::Stop` (0x4257d0), `Hold` (0x4257e0), `StepLoop` (0x425a50), `StepPingPong` (0x425a70), the empty `Step` and `AfterDraw` hooks, the modes 0 to 2 (`mMode`) with their switch, `mHeld`, `mStopAtEnd`, `mKeyed` and `mKey` (the keyed draw), `mRepeatDelay` and `mRepeatCycles` (the branch of `EndCycle` that waits and starts again), `mDrawStopped`, and `Attachment::Hold` | the only two animations are the main menu's, `Setup` (mode 3) with infinite cycles; nothing stops, holds, repeats or keys one, and no class derives from `Animation` | removed |
| `Object::mAttachmentsUnder` and the draw order it chose | never set: an object is drawn before its attachments | removed |
| `TextEdit::mNoWidthCheck` | never set: the width check of `TextEdit::Fits` always runs | removed |

About 420 lines go from each game (the changed files are the same 33 in both). A removal changed no draw of a random
number, no state and no saved byte: nothing that writes a save or answers a test command used a removed member (the
build would stop).

**Looked at and kept.** The deleting destructors of `Dialog`, `SavedGame`, `Deal`, `Layout`, `User`, `PlayerList` and
`MahjongGame` (0x404c00 and the rest) and the aliased constructor forms are the compiler's, not source. The default
virtuals 0x4204a0, 0x4204d0 and 0x4209e0 are the empty hooks of the base class: the interface that the screens override.
`Object::SetVisibleByParent` (0x41ef30, with the child loop of `Object::SetVisible`) runs when an object with children is
shown or hidden (the slider bar's slider and the list box's arrows are children): no play of the original did it, but
what shows or hides such an object is the game's own flow, so the code stays. `Layer::FocusNext` and `FocusPrev` (0x421160, 0x4211f0) run on any
Tab. A tab character in a text is data, so the branch that gives it a width stays. `Animation::EndCycle`'s counting of cycles
and `SetDelay`'s random delay are what `Setup`'s parameters mean, though the menu passes -1 and 0. The unused parameters
of the virtual hooks and handlers (`theX`, `theY`, `theFlags`, `theScan`...) are the original's signatures.
`PeResources::Key::operator<` is the `std::map`'s order for the resource directory. `mProdName`, `mReadFromRegistry`,
`mRegKey`, `mUploadAsIs` and `mWantsFocus` are set once in `MahjongApp` for the framework and read there.
The two `_WIN32` blocks and the `__EMSCRIPTEN__` ones are the port's own.

**Doubtful, for the user.** The test hooks (`TestCommands.cpp`, `Bot.cpp` and `Bot.h`, about 1,300 lines) are compiled into
the one executable that players get, reachable only from a test (`docs/TEST_COMMANDS.md`); taking them out of the shipped
build would need a build switch that the tests then turn on, and nothing in the game depends on them. The framework's
`mUploadAsIs` and the registration members of `SexyApp` are shared code, the coordinator's.

**Proof.** Both ports build (native and web). `tools/check.sh mahjongjapan mahjongchina` after the removals: Japan 222
layouts dealt with none failed, 20 pairs, the three screens as frozen, a rerun identical; China 178 layouts, 31 pairs, the
three screens as frozen, a rerun identical. That suite plays a board and the map, so the screens it does not reach were
compared with a control: the exes built before the removals, and the screen tour of `tools/tour/` (`tour.sh` runs one
script on one exe, `compare.sh` a "before" folder against `build/native` and the before exe against a second run of itself):
20 screenshots of each game, under `-fast` on the virtual clock: the main menu at three times (the swinging animations), the
hovered button, the player name box as typed and as edited, the map, options, help, the classic list, the quit question, a
board with the hover and the pause box, the escape stage and the map again. All 20 of each game are byte for byte the
same (16 distinct images each), and the before exe's own rerun is the same too. The scans, run again on both games after
the removals, find no further dead function, member or constant.

## The readability pass (2026-11-11)

The rules are `docs/CODE_STANDARDS.md`. `python tools/namecheck.py games/mahjongjapan` has no finding; the game's
`namecheck.txt` adds `engines/jollybear` and `engines/mahjongescape/src` to the check with `folder` lines. The two exceptions, `Div255` and `AppendCp1252`, are numbers that belong to their
names. The game has no Mac build, so every name comes from how the code uses the thing and from the game's own words.

**Nothing the port does changed, proved by the object code.** The whole pass is renames, named constants, comments and
line breaks, so every unit must compile to the same instructions as before it. `llvm-objdump -d -r` of the 39 object
files of the native build (the game, the engine, the deal test) with the symbol names taken out, 1,741 functions, and
the bytes of the data sections: none differs from the build before the pass. The suites agree: `tools/check.sh build
mahjongjapan mahjongchina` passes (every native target builds; 222 layouts, 20 pairs, the screens as frozen, a rerun
identical), the screen tour of `tools/tour/` gives the same 20 screenshots byte for byte as the exe from before the
pass, and the web target builds.

**Prefixes and words for letters (rules 4 and 5).** clang-tidy's naming check, set to the framework's prefixes, listed
249 names. 204 of them were renamed, each within its own scope, with two more it let through (a green channel `g`, a
layout `aTemp`): locals and parameters take `a`, `an` and `the`, a one-letter pointer or counter takes a word (`p` is
`aSlot`, `aCell`, `aCountSlot`..., `n` is `aRemaining` or `aSeen`, `r` is `aRandom` or `aRowIndex`), the `k` constants are
UPPER_CASE (`STAGE_RECTS`, `MAP_PATH`, `SCREEN_RECT`...), the `Module` values are `EXE` and `DLL`, and `PeResources.cpp`,
written in another style, follows the rest. Left as they were: `Rect`'s and `Point`'s members, which are Win32's RECT and
POINT; `main.cpp`, which every game shares in that form; and `tools/dealtest.cpp`, which is not under `src`.

**Members.** The tile's `mFlag39` and `m3c` are `mBobsAsHint` and `mLiftStart` (when its lift began: a hint's bob or an
undo's settle). The layer's two flags that nothing reads are `mXLowByteSet` and `mXLowByteSetToo`, named for what the
constructor puts in them, and their comments say that nothing reads them, as do those of `Board::mModeByte` and the result
box's `mClockFont`.

**The magic tiles in the game's own words.** The code called the Magic Gold tiles "key tiles", the key and lock "halves",
the joker "wild" and the magnet "linked wild", which read wrongly beside the game's real Key tile. `TakeKeys`, `EndKeys`
and `EndKeysInTray` are `TakeGold`, `EndGold` and `EndGoldInTray`; `TakeHalves`, `JoinHalves`, `FlyHalves` and `EndHalves`
are `TakeKeyAndLock` and so on; `TakeWild` is `TakeJoker`; `TakeLinked`, `FlyLinked`, `EndLinked`, `UndoLinked` and
`EndUndoLinked` are `TakeMagnet` and so on; `GetHalves` is `GetLocks`, `ApplyWildLinks` `ApplyMagnetLinks`, `mWildLink`
`mMagnetLink`, `mTipKeys` `mTipGold`, the bot's `mKeyGoal` `mGoldGoal`, and the deal's `gHalves`, `gHalvesLeft`,
`gHalvesCount` and `gSolve2b` are `gLocks`, `gLocksLeft`, `gLockCount` and `gSolveGold`. The comments follow. The test
commands' answers keep their field names, which are a format.

**Numbers (rule 7).**
- *Resources.* The original passes its resource ids as numbers, and so did the port: `AddText(0x6a, ...)`,
  `DrawImage(0, 0, 0x6d, ...)`. `src/Res.h` and `engines/jollybear/EngineRes.h` now name the 336 ids the code uses (pictures, the
  fonts' pictures and character maps, sounds, music and strings; the engine's click, the exe's pictures and the splash
  font), in 598 places: `STRING_NO_LAYOUT_TEXT`, `IMAGE_MENU_BACKGROUND`, `FONT_HEADING` with `FONT_HEADING_MAP`,
  `SOUND_SHUFFLE`. A picture is named for what it shows (every picture of Resource.dll and the exe was looked at, on
  contact sheets that stay out of the repository), a string for its place and role, never by its text.
- *Tile kinds.* `KIND_GOLD`, `KIND_KEY`, `KIND_LOCK`, `KIND_JOKER`, `KIND_MAGNET` and the bounds of the seasons and the
  flowers (`Layout.h`), `NUM_COUNTED_KINDS`, and the deal's places `DEAL_SEASONS`, `DEAL_FLOWERS` and `NUM_DEAL_KINDS`
  (`Deal.cpp`), on about 170 lines of the board, the deal, the layout, the bot and the help pictures.
- *The board's states.* The twelve flight kinds (`FLIGHT_TAKEN` to `FLIGHT_GOLD_TO_TRAY`, each with the step that follows
  its landing) and the three phases (`Board.h`).
- *The screen and the cursor.* `SCREEN_WIDTH`, `SCREEN_HEIGHT` and the `CURSOR_KIND` values (`engines/jollybear/JBBase.h`).
- *Left as numbers.* Positions, sizes and frame counts, whose line says what they are; the scale in percent (100) and the
  alpha (0xff) that every draw call ends with; the resource ids inside tables (`STAGE_IMAGES`, `BACKGROUNDS`), whose
  comment names the table; and each dialog's and layer's command and timer ids, which are local to one class (the
  button that sends a command and the `case` that takes it sit in the same file). The last is the one place where rule 7
  would still ask for names.

**Addresses and comments (rules 6, 8 and 9).** Every function names its address or its file carries the port marker
(`MahjongApp.cpp`, `PeResources.cpp`, `GameText.cpp`, `ModFeatures.cpp` got it; the test commands and the bot had it).
A member's comment that named its font or string by number names the constant. Two comments that told a development
stage or an open check are plain, and the lines the longer names pushed past 120 columns are wrapped. Loops keep the
original's shape throughout.

**How it was done,** for the next game on this engine: clang-tidy's `readability-identifier-naming` with the prefixes as
its options gives the list for rule 4; a script renames each declaration within its scope (comments and strings left
alone), another finds the number literals passed for parameters that hold ids (by the parameter's name) and writes the
constants; and a snapshot of every object's disassembly before the pass is compared after each step.
