# Ning Po Mahjong Deluxe: notes on the original

What the translation found in `Ningpo.exe` (linked 2002-09-25), for whoever reads or changes the port. Addresses are
the original's; the port's declarations carry them as `// 0x...` comments, so a grep finds both sides.

## How the program is built

The program is C++ on Pyrogon's framework. `WinMain` (0x40c890) makes the one game object (the global at 0x475550,
0xb9c4 bytes, a subclass of PGApp) and calls `PGApp::run`, which starts the subsystems (the display, the input, the
sound, the cvars, the registry, the archive) and then runs `PGApp::mainLoop` until the game says to quit. The loop
takes the window's events from a queue, gives each first to the app (activation, painting, the controls) and then to
the game, and when the queue is empty redraws the dirty rectangles (the game's render, then the controls) and flips
them to the window. Every dialog is a modal loop of its own (`PGDialog::doModal`) that draws the game under it and
every open dialog over it. Time is the time of the event being handled; the game takes the time spent paused off
its clock.

The display is a 640x480 GDI back buffer of 15-bit pixels (5-5-5). Images become surfaces of the same pixels,
dithered down from 24 bits, with an 8-bit alpha plane when they have transparency, and every drawing call blends a
surface into the back buffer. The sound is a software mixer of 8 voices and one music stream (Ogg Vorbis decoded as
it plays) into a DirectSound buffer of 22050 Hz; a voice's pan is kept but never applied.

## The game

The game is one screen: the title, where arrows choose the layout (18), the tile set and the table (the background),
the board, and the two end screens (won, and no more moves). The controls run down the left side and a panel of
numbers (the time, the moves available, the tiles left) along the bottom. Everything else is a dialog: the options,
the best times, the name entry, the yes/no questions, the no-moves dialog and the rules.

A layout (a `.nml` file of the archive) gives 144 cells, each x and y on a grid of half tiles and z the level. A new
game puts the 144 tiles of the set into the cells at random (0x4066d0); nothing checks that the board can be
cleared. The set is the 34 suits and honours four times each, and the four flowers and four seasons, one of each,
which match any of their kind. The tiles are drawn in an order sorted once per game (0x4067f0, a bubble sort): the
lower levels first, then from the right to the left and from the top down.

A tile is free when nothing is on top of it and its left or right side is open (0x4064a0). The left button selects a
free tile (0x4045c0); a second free tile of the same kind takes the pair off, while one of another kind drops the
selection with the sound of a blocked tile, as a click on a blocked tile has. A click on the selected tile, or the
right button, drops the selection. The moves available are, for each kind with a free tile, half of its free tiles
rounded up (0x405ef0).

The clock counts the seconds played (the time paused not counted) plus penalties: 30 seconds for a hint asked for,
60 for an undo and 120 for a shuffle. A hint (the button, or `f` or `h`) shows a move, the next one at each
hint; with a tile selected it shows that tile's matches that can be seen, or the "no match" sign for 3 seconds when
there are none. The autohint (an option) gives one after 45 seconds without a match, without the penalty.
Show moves (the button, or Space) lights the free tiles over half a second, keeps them lit for 3.5 seconds (or while
Space is held) and lets them go back over half a second; how far it lights them depends on the tile set. `s`
shuffles, and `u` undoes. A shuffle swaps the tiles' faces at random until there is a move (0x406c00); the undo list
holds every pair of the game.

When no move is left, a dialog offers to shuffle (only when more than one tile is free), to undo, or to end the
game. A game ends won (no tile left) or ended in that dialog; its time goes into the layout's best times if it
ranks among the 8 (0x4081d0): fewer tiles left first, then the lower time with the penalties, then the lower time
played. The name entry suggests the account's name (none when it is longer than 23 characters) and takes up to 12
characters.

Typing Backspace, q, Backspace toggles a cheat (0x403e20): with it on, `m` plays the first move the board has and
`n` shows the "no match" sign, and the game cannot be paused.

## The registry

The game keeps everything under `HKEY_CURRENT_USER\Software\Pyrogon\NingPo` (the port's emulated registry,
`registry.ini` in the user folder). Its own values are binary: `Layout`, `Tileset`, `Background`, `Autohint`,
`Fullscreen`, `Sound Volume` and `Music Volume` (floats) and the best times, one value of 0x30 bytes per layout and
rank named `HS:<layout>x<rank>`, with the name at +4 (0x20 bytes), the time with the penalties at +0x24, the time
played at +0x28 and the tiles left at +0x2c. The other values in the file are the framework's.

## The trial

The trial's code is in the same exe, behind `checkRegistration` (0x40edb0): the "full version only" layouts and tile
sets refused at a new board (0x404c90), the count of games played against a maximum (the registry's `InstallFlags`),
the loading screen's progress bar stopped at a quarter, a buy button among the controls, a register button in the
options and a page asking to buy at exit. The loading screen also carries a thanks for registering
(`demo_menu_thanks_np.tga`), loaded and placed but never shown. The port plays as registered and leaves all of it out,
the thanks included (the user, 2026-11-08: a thanks for registering is part of the registration code). No text of the
game's tables is shown only to a registered copy.

## Fix candidates

None. The fixes mod (`docs/MOD_ROADMAP.md`, "The fixes mod") takes a bug of the original or a wait that only costs the
player time, and both phases of the identical-play pass and a read of the game's code for the usual places found
neither. What was checked:

- Waits. The game has no timed screen: the loading screen ends with its button, the end screens and the best times
  with a click, and loading has no wait of its own (`PGApp::loadAssets` loads everything in one go). The no-moves
  fade and the tiles' highlights take a fraction of a second and do not hold the player.
- The clock. Every dialog (options, rules, best times, the questions, no moves) stops the game's clock while it is
  open, and so does losing the focus, so none of them costs the player time on the best times.
- Dead boards. The shuffle deals again until the board has a move, so it never leaves one without; running out of
  moves is offered as the no-moves question, by design.
- The cheat (Backspace, q, Backspace) turns pause off while it is on. That is the cheat's own rule, not a bug.
- The exit question shows the restart question's picture (`restart_areyousure.jpg`); the archive has no picture for
  exit, so there is nothing of the original's to show instead, and a new picture would be new art.
- The layouts (`.nml`) are read as strings without a terminating zero; the port gives the buffer one (a memory read
  past the end, with no effect seen in the original), listed with the deviations.
- The mixer has 8 voices, and a sound started while all 8 play is dropped. With the game's few, short sounds that
  does not happen in play.

## Performance (2026-11-08)

The original and the port, measured back to back on this machine (Windows 11) in real time, from fresh settings, for
the final merge (`build/slots/news.md`, 2026-11-08), by `tools/perf/perf.sh`. Before each run it waited until at most
one other game process was running on the machine (other sessions' tests), so each was measured beside one other game
at most. Each run is one process from its creation to its exit, sampled every 50 ms
(`games/bejeweled/tools/profile/perfmin.py`). The play is the screen tour of the identical-play pass: the title, the
options, the rules, the arrows, a deal, hint, show moves, pause, a loss of the focus, the cheat's 74 matches, no moves,
the name and the best times (65 s). The processor and memory figures in the window are for 20 s to 50 s of the
process's life, the game and the matches. The original ran under origrun on its real clock with no sound device
(`--sound nodriver`, as in the screen tour): with origrun's sound stand-in its mixer spins from the deal on and takes
more than a core (113 % in three runs, `IDENTICAL_PLAY.md`, "Oddities"), which is no measure of the game. The port
ran natively under rundesk without `-fast`, with SDL's silent audio driver, so it still mixes its sounds.

| | original (3 runs) | port (3 runs) |
|---|---|---|
| Launch to the start screen, real time | 0.82 s median (0.78 to 1.03) | 0.36 s median (0.26 to 0.58), 5 runs of their own |
| Peak working set | 39.1 MB median (38.8 to 39.4) | 62.1 MB median (53.9 to 62.2) |
| Peak private bytes | 21.4 MB median (21.2 to 22.9) | 54.2 MB median (52.4 to 54.5) |
| Processor time in the window | 14.9 % of one core (13.7 to 17.2) | 6.3 % of one core (6.2 to 7.4) |
| Processor time, whole run | 5.8 s median (5.5 to 6.2) of 62.8 s | 2.9 s median (2.8 to 4.3) of 68.7 s |
| Threads | up to 17 (DirectDraw's and the system's) | up to 50 (SDL's and the system's) |

The loading time is read from the samples the same way for both (`tools/perf/perfload.py`): Ning Po loads everything
before its start screen takes a click and then waits for events, so the load is over at the first sample after which
the process uses less than a tenth of a core for a second. In the tour runs the port's first readings came late on a
busy machine (the first sample two seconds after the process started, in one run), so its loading time is taken from
five runs that only launch and quit (`-realinput` with a quit at update 500). On the web, with `fast=1`, the start
screen took 282 ms from `t.start()` in the tour's run (`tools/web/tour.js`); the page's loading of the game's files
before that was not measured, since the test page reads them from the test server rather than from the player's own
folder.

The port holds about 23 MB more at its peak than the original and uses less than half its processor time. Why was
not examined; both are for the performance pass (`docs/COORDINATION.md`).

### The performance pass (2026-11-11)

Where the port's memory goes, from the test command `memreport` (`NingPoApp::AddMemReportHolders`; the framework's
own figures plus Ning Po's holders), in a game: private bytes 54.0 MB, of which 30.0 MB were there before the game
began (the framework, SDL, the D3D11 device and the process's libraries: shared costs, `docs/COORDINATION.md`). The
rest, about 24 MB, holds Ning Po's sprite surfaces (114 of them, 15-bit pixels and the alpha bytes: 6.4 MB), the
whole music track decoded (5.6 MB, only once a game has started), the decoded sounds (0.3 MB), the 15-bit back buffer
(0.6 MB), the framework's picture of the screen (1.2 MB) and the heap and code. Graphics are thus about 8 MB, sound
about 6 MB, game state under 1 MB. The title screen holds 52.7 MB (the track not yet decoded).

Changed: `NingPoWidget::Draw` converts the 15-bit back buffer to the framework's 32-bit picture with a 32768-entry
table instead of per-pixel shifts (the same values, so every screenshot is unchanged). `tools/check.sh ningpo`:
baseline identical. Not changed: decoding the music track whole (streaming it needs the shared audio code: a spec
for the coordinator if wanted) and the 22 MB the framework itself holds, which are known shared costs.


## Coverage check (2026-11-08)

The first whole coverage check of a game (`docs/COVERAGE.md`). origrun's `--coverage` recorded which of the original's
functions ran, in seven runs on the host (`tools/coverage/run.sh`, the hits in `build/coverage/ningpo/hits/`):

- the four identical-play cases (cheat7, won39, mouse23, layouts), on the virtual clock with no sound device, each
  about 900 functions;
- the screen tour on the real clock with the silent sound stand-in, so that the mixer's code runs (904);
- two short runs (`tools/coverage/extra1.txt`, `extra2.txt`): a tile selected and then a hint (`isVisible`), the
  right click that drops the selection, the options' autohint, the best times' next and previous and their clear
  question answered no and then yes, and closing the window from play and from the title (925 and 822).

Every run but the tour ended with the game exiting normally; the tour ends at origrun's timeout. Closing the window
ends the original at once, from the title and from play: it asks no question.

`games/ningpo/coverage.txt` sets the game's own code as three ranges: the game and Pyrogon's PG framework
(0x401010-0x417b10), the H layer (0x422b10-0x428fb0) and the window procedure (0x433290). The rest of the exe is the C
runtime, Lua 4, the Ogg Vorbis decoder, minizip and zlib, and the IJG JPEG library. The game's own code has 815
functions, and the runs reached 574 of them.

**Reached in the original with no counterpart in the port: 272 at first, 1 at the end.**

- **The tool: 32.** `tools/coverage.py` did not count a constructor or destructor declared in its class, a
  declaration over several lines, or a declaration with a default argument or an inline body. The spec went to the
  coordinator, who fixed it on main (9d12ecc5).
- **Placed: 54.** Functions the port translates inline or in another shape (the loaders' small objects, the
  registry's key, the display's teardown, the PyTime helpers, the six title arrows' click handlers) got their address
  in the comment above the function that holds them. These were comment edits only, and play is unchanged:
  `tools/check.sh ningpo` still matches the frozen baseline.
- **Left out on purpose: 185, as `none` lines in coverage.txt, each with its reason** (beside the three it started with: `WinMain` and the two registration checks). The mechanisms the port does
  another way: the cvars kept in a Lua table, the framework's object list and its stack of open dialogs, the
  command line parsed into cvars, and the H layer's window, DirectSound mixer and streams, DirectInput device and ZIP
  reader, which the host and the platform replace. Also the trial's code (the count of games played, and the big
  numbers of the registration key's check), the sprite cursor (Ning Po's is empty), and what the compiler makes on
  its own (implicit destructors, vector constructor iterators, stubs).
- **No missing game logic was found.**
- **One stays open.** 0x40e840 is a 7-byte getter of the field at +0x410 of the object whose constructor is
  0x40e300 (vtable 0x463144, an HObject). It is reached only through that vtable, and which port class it belongs to
  was not found.

**Defined in the port, never reached: 28 at first.** The extra runs reached four of them: the right click, the hint's
`isVisible`, the question dialog's setup and the best times' `fillRows`. Of the rest:

- **Read again against the export.** `drawFaded` and `drawNoMatch` are reached only by a hint for a selected tile
  that has no free match. They match the export, including the alpha's scale of 255 (the constant at 0x462384).
  `toggleFullscreen` was not run, since the original would change the machine's display mode; the port's version is
  its own (the host switches the window, a listed deviation).
- **Code only an error reaches, which stays.** `PGAlert`.
- **Candidates for taking out unneeded code: 22.** No play of the original reaches these, and they were not removed
  now:
  - `NingPo::~NingPo`: the original exits without running its global's destructor; the port needs its own.
  - The dialog items' default virtuals that nothing calls: `draw`, `canFocus` (twice), `getName`, `getUserData` and
    `setUserData`, and two more at 0x412580 and 0x412900.
  - `PGApp::removeControl`.
  - `PGSprite::create` from an image, and `PGSprite::clone` with `HDisplay::copySurface`.
  - The framework message dialog's timeout.
  - `PGRandom::nextFloat`.
  - The registry function 0x4136d0.
  - `HCvarManager::setVariableString`, called only by the registration check.
  - `HDisplay`'s draw of a surface at a point (0x4231f0, 0x423000 and 0x423140), its `setBlendFlag` and `clear`.
  - The PNG loader (0x428590, 0x428070): the archive holds no PNG.

**Never reached and not defined in the port: 213.** These are mostly the trial's code, the framework's trace,
journal, demo and DirectInput code, and parts of the H layer the game never calls. None looked like something the
runs should have reached.

**What it cost.** The seven runs took about 15 minutes of machine time, one game process at a time. One Sonnet agent
placed and sorted the 253 functions in 7.5 minutes and 178,000 tokens. This session read the export, wrote the runs
and checked the agent's work over about two hours; its own tokens were not counted.


### Unneeded code taken out (2026-11-11)

Of the 22 candidates above, taken out: `PGRandom::nextFloat`, `PGApp::removeControl`,
`HCvarManager::setVariableString`, `HDisplay::copySurface` and `setBlendFlag`, `PGSprite::clone`, and the dialog
items' `getUserData` and `setUserData`. Each had no caller. `tools/check.sh ningpo` (baseline identical) and
`build-web:ningpo` pass. Kept: `PGSprite::create` from an image (the options dialog's background uses it), the dialog
items' other default virtuals (they fill vtable slots the original's items fill; no code is behind them but a
return), the PNG loader (one branch of the shared image loader, a few lines), `NingPo::~NingPo` and `PGAlert`.


### Readability pass (2026-11-11)

`python tools/namecheck.py games/ningpo` (with `folder engine` in `namecheck.txt`, since the Pyrogon engine sits beside
`src/`) found 39 names and missing markers: 26 port-only functions (the widget, the application, the text lookup) now
carry a `port:` comment, eight one-letter names in `HDisplay.cpp`, `PGApp.cpp` and `Board.cpp` are named, and the four
gaps of `PGEvent` are named for where they sit. Now 0 findings. `splitfields` finds no member declared twice. A rename
changes no layout; `tools/check.sh ningpo build-web:ningpo` pass with the baseline identical. A fixes mod is not needed:
no fix candidate was found in any pass.


### The sound log (2026-11-11)

`BYOPOP_SOUNDLOG=<file>` makes `PGSoundManager` and `HMixer` (`engine/HAudio.cpp`) write the framework's lines: `load`
(with the sound's slot), `instance` and `play` (volume byte, pan byte, pitch 1.0000, loop), and `stop` (from
`stopAll`; a sound that ends by itself writes none). The music writes no lines. With the log on and off
`tools/check.sh ningpo` stays baseline identical. A recording: `BYOPOP_SOUNDLOG=<file> build/native/rundesk.exe python
games/ningpo/tools/realplay/drive.py <dir> --games 2` (two bot-played games, 259 lines, 119 plays of the 9 sounds).


## The PopCap Game Collection version (2026-11-11)

The user's PopCap Game Collection disc holds Ning Po (`unshield` on the read-only mounted image, group
`NingPoMahJongDeluxeComponent`: four files, `Ningpo.exe`, `ningpo.pyz`, `license.txt`, `readme.txt`; the layout and the
names are the staged copy's, so the site's import needs no second name). Compared with the staged copy:

- `ningpo.pyz` is the same file byte for byte (3,115,811 bytes).
- `Ningpo.exe` has the same size (491,520 bytes) and the same link time stamp, and differs in 31 bytes of code only:
  no text or table the port reads moves, since `GameText.cpp` reads the texts from the exe by address and the two exes
  agree on all of them.
<!-- private -->
  The 31 bytes are one run at file offset 0x5ce3 in `.text`. There the Steam copy's function tests a call's result
  and goes on to the registration dialog or the next call; the Collection's returns true at once and pads with nops:
  it is the registration check (the one the port never had), taken out in a copy the disc registers.
<!-- /private -->

The port on the Collection's files (`-gamedir` the extracted folder, the suite's scripted game, `-fast -seed=7`, under
rundesk): the first best time (`HS:0x0`) and the four screenshots (over, name, best, title) are byte for byte those of
the same run on the staged copy. No change to the port was needed, and the frozen baseline is unchanged.
