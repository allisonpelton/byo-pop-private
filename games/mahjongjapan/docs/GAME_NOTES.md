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

## Performance (2026-11-08)

The port measured on this machine (a Core i7-12700F, Windows 11), windowed, in real time, from a fresh user folder, with no
mods, under rundesk (below normal priority, SDL's silent audio), for the merge report (`build/slots/news.md`). The other
sessions' games ran meanwhile (one or two besides the measured ones), so the numbers are for comparing and for the
performance pass to start from. Each run is one process from its creation to its exit, sampled every 50 ms
(`games/bejeweled/tools/profile/perfmin.py`); the input is the minute of play the suite `tools/check.sh mahjongjapan`
sends (the menu, a new player, the adventure's first stage won by the solver's clicks, back to the map: 65 s). The
processor figures are for the window 20 s to 50 s of the process's life, in the play.

| | port (3 runs) |
|---|---|
| Launch to the main menu, real time (`tools/realplay/perfload.py`, 5 runs) | 1.31 s median (1.28 to 1.32); with the original's start (`-logos`, 3 runs) 7.80 s (7.76 to 7.80) |
| Peak working set | 220.9 MB median (212.7 to 221.6) |
| Peak private bytes | 213.2 MB median (212.3 to 214.6) |
| Processor time in the window | 5.4 s of 29.9 s: 18 % of one core (17.9 to 21.3 %) |
| Processor time, whole run | 11.2 s median (10.0 to 12.9) of 64.8 s |
| Processes and threads | 1 process, 42 to 50 threads |

**The original was not measured.** Mahjong Escape's original exes run only inside Windows Sandbox under `tools/origrun`
(`docs/ORIGRUN.md`, its status table: a packed `bass.dll` never loads the silent stand-in, so a run on the host plays its
music on the speakers, and every capture is taken in the sandbox). Working set and processor time measured from outside
a sandbox's virtual machine would not be comparable with the port's on the host, and a host run's sound would reach the
user's speakers while they sleep, so neither was done. Its loading is known from the code instead: the original shows
the two logos for 2 s each (0x42f650) after 0.77 s of black, loads nothing meanwhile, and the port's launch to the menu
with `-logos` (the original's start) is 7.8 s, of which the logos, the black and the fades between the screens make
about 6.5 s. The port without them takes 1.3 s ("The start", above). Why the port holds this much memory
and uses this much of a core to draw 800x600 in software was not examined; both are for the performance pass
(`docs/COORDINATION.md`).

## Coverage check (2026-11-08)

The first whole coverage check of Ancient Japan (`docs/COVERAGE.md`, "A game's whole check"). origrun's `--coverage`
recorded which of the original's functions ran, in 16 runs on the host (`games/mahjongjapan/tools/coverage/run.sh`
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

**The deal of the classic game is checked, not assumed.** A classic game seeds its deal from `GetTickCount` (Board::SetLayout
0x40aa40 and Board::Start 0x40ae60), which no script can pin. origrun's `--seed` with `--patches` (`docs/ORIGRUN.md`,
"Patching the copy") replaces the two calls in the copy with the seed 12345. The saved game of that run holds the deal
that the port's solver (`mahjongjapan_dealtest --solve 441 12345 0`, with `MJ_CLASSIC=1`) produces for the same seed:
0 of the 144 tiles differ in kind. So the first phase of identical play holds for a whole classic game (the RNG draws of
the deal, in order) as well as for the adventure's stages.

`games/mahjongjapan/coverage.txt` sets the game's own code as two ranges, 0x401000-0x4175b0 (the game) and
0x41e240-0x4319e0 (the engine layer), 1280 functions; the rest of the exe is the C
runtime and the libraries. The runs reached 1085 of the 1280, the port defines 1039, and 261 are listed as left out.

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

**Defined in the port, never reached: 126 after the sorting, 110 after the classic game.** One Sonnet agent read all
126 again against the export (the decompile that shows `this`, the vtables and the callers): 124 match, and two
differed, both now fixed:

- **0x42a340, `Layer::AppActive`.** The port returned the display's own flag (`Manager::IsAppActive`, a field nothing
  changes) where the original asks the manager's active flag, which `Manager::Activate` sets on a focus change. Its one
  user, `PlayLayer::Begin` (0x405500), shows the pause box when a game starts while the window is not active; the
  port never did. `Layer::AppActive` now returns `mManager->GetActive()`. The single name `IsAppActive` still stands in
  for the original's 3D test (0x42a340, always true in the port, a listed deviation) in the two places that use it as
  that; renaming it is left for the readability pass.
- **0x42ed70, `Image::SaveBmp`.** The port left the header's size-of-image field (offset 34) zero; the original
  writes the pixel data's size. The bitmap of every classic list page sheet and of a saved screen differed by those
  four bytes. `Put32(34, aSize)` now writes it.

Of the 126 rows the agent judged 105 reachable by some play the runs did not make (R) and 21 not reachable by any
play of the original (U). The classic game then reached 16 of the R rows (the result box, the next layout, the
resume answers). The 110 still unreached:

- **Plays no run could make, on the posted-input path.** A held press (a drag of a volume slider's thumb, a drag in
  the name box, the manager's mouse move during a drag: `Slider::OnDrag` and the rest, 0x430a30-0x430ab0, 0x420150,
  0x420190, 0x42bb10; origrun posts `down` and `up` only at an update of the game's counter, and this game has none), a
  double click or a right double click (0x4020a0, 0x401250, 0x4159f0, 0x42b6e0, 0x42b9b0, 0x420560, 0x40ffa0), a
  key with a modifier (Ctrl+C, X, V, Z and Ctrl+Insert in the name box: 0x428840-0x428b70, 0x42c550, 0x42c5c0), and
  Shift+Tab (0x4211f0).
- **Plays not made.** Right click on the board (the tip box: 0x404ba0, 0x404ec0, 0x40c1b0, 0x42b910, 0x42ba70), the
  no-moves box and its three buttons (a deal that leaves no pair: 0x409d90-0x40a090, 0x405530-0x405580, 0x40b650,
  0x414fd0), OK or Cancel clicked in the new-player box (the runs press Enter), OK with an empty name, the
  player list's scroll arrows, a click in the name box, the styles and backgrounds boxes' OK and double clicks, the
  options' hardware-acceleration, full-screen and wide-screen toggles, minimize and restore, the classic list
  built from scratch (no cached sheet), a deal the solver gives up on, a saved game with a wild-linked tile, and Enter
  during the fly-off after a level. None of them is reached by the original's play without a run for it, and each was
  read against the export: they match.
- **Code only an error reaches.** The error box (0x412110-0x412240) for a layout resource that fails to load.

**Candidates for taking out unneeded code: 21 (the U rows).** None was removed. Taking any of them out changes no
play of the original.

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

**Open items.**

- The plays above, if the user wants every R row run: they need origrun to hold a button across updates (a
  sequence of `down`, `move` and `up` that does not depend on an update counter), a modifier key, and a double click.

**What it cost.** The 16 runs took about 22 minutes of machine time, at most three game processes at a time (400 s of
it the classic game's second run, which ended at its timeout). Two Sonnet agents did the sorting of 192 functions and
the re-read of 126; their tokens and this session's were not counted. Most of the session's time went to the
scripts: finding the places on each screen, solving the stages for the adventure's clicks, and getting a classic game
to its end with a pinned seed.
