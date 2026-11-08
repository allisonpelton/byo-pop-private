# Ning Po Mahjong Deluxe port

A port of Ning Po Mahjong Deluxe (Pyrogon, published by PopCap Games; © 2002 Pyrogon) to the shared platform layer:
it plays natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of `Ningpo.exe`, found by
decompiling it with Ghidra; the game's files (`ningpo.pyz`, which holds the pictures, sounds, music and layouts, and
`Ningpo.exe`, whose texts are read at run time) come from the player's own copy and are never in the repository.

Ning Po is not a SexyAppFramework game, and it shares no code with TipTop's LOWOS either. It runs on Pyrogon's own
framework (the exe still names Candy Cruncher, Pyrogon's other game on it, in one string): PGApp, PGControl, PGDialog
and PGSprite over a layer of "H" classes (HDisplay, HInput, HMixer, the cvars, the registry, a file system over the
archive). The port keeps that structure. `engine/` is the framework, translated in the original's shape and mapped
onto `platform/` through a small host interface (`pg::Host` in `engine/Pg.h`: the clock, the sleep, the screen, the
game's files, the registry, links, the clipboard, the account's name and the screen mode). `src/` is the game, and
the SexyAppFramework only hosts it (`src/NingPoApp.cpp`: the window, the clock, the input, the sound device and the
640x480 screen the game draws into).

Everything a player of the registered game sees is there: the loading screen, the title with its choice of layout,
tile set and table, the board with its controls and panel, hints and the autohint, "show moves", pause, undo,
shuffle, the no-moves dialog, the end screens, the name entry, the best times of each of the 18 layouts, the
options and the rules.

## The binary

`Ningpo.exe` was linked on 2002-09-25 by MSVC 6 (linker 6.0) and carries no copy protection wrapper and no RTTI. The
game is 0x401000-0x40c910 (the app from 0x4010a0, the board 0x405e50-0x406cd0, the controls and dialogs
0x407660-0x40bcc0, the tile 0x40c740-0x40c840), the framework's PG classes follow it up to 0x418000 and its H layer
is 0x422b10-0x429000 (the window procedure is at 0x433290); the rest is libraries (zlib with minizip, the IJG JPEG
library, an Ogg Vorbis decoder) and the C runtime. `GAME_NOTES.md` has the rules as the code has them, and `symbols/` the names the port gave the functions
(`10_framework.jsonl`, `20_game.jsonl`).

`ningpo.pyz` is an ordinary ZIP file of 235 entries with bare names: Targa and JPEG images, the layouts (`.nml`, one
cell a line), Ogg Vorbis music and sounds and three WAV clicks. `tools/ningpyz.py` lists and extracts it (into the
git-ignored `re/ningpo/pyz/`). The game names its sprites by index in a table its constructor fills (0x401330);
`tools/make_spritetable.py` copies the file names into `src/SpriteTable.inc`.

## Reproducing the analysis

The export lives in `re/ningpo/` (git-ignored, like everything derived from the game). Ghidra finds no vtables in a
program without RTTI, and a virtual function that is only ever called through its vtable is never disassembled;
nor does it know how many arguments an indirect `__thiscall` takes, so the virtual calls lose theirs. Two passes fix
that: `tools/vtables.py` finds the vtables (runs of pointers into the program's code in `.rdata`, the first one
referenced from the code) and lists the entries that are not functions yet, and `tools/ghidra_pass.sh` creates them
(`tools/ghidra/NingpoCreateFunctions.java`), applies the port's symbols, gives every indirect call a signature with
as many arguments as were pushed for it (`tools/ghidra/NingpoCallOverrides.java`) and exports the decompilation again.

```
mkdir -p re/ningpo/bin && cp assets/ningpo/Ningpo.exe re/ningpo/bin/
tools/ghidra_import.sh re/ningpo/bin/Ningpo.exe ningpo re/ningpo/gh  # the first analysis and export
python games/ningpo/tools/vtables.py                                 # re/ningpo/vtables.txt and vfuncs.txt
build/native/rundesk.exe bash games/ningpo/tools/ghidra_pass.sh      # the functions, symbols and calls; the export
python games/ningpo/tools/decread.py 4092a0                          # a function, readable: names, strings inlined
python games/ningpo/tools/make_gametext.py                           # src/GameTextTable.inc after a new GT()
```

## Building and running

```
tools/setup_worktree.sh                                               # a new worktree: the git-ignored files
python games/ningpo/tools/stage_ningpo_assets.py <the game's folder>  # stages assets/ningpo/ (ningpo.pyz, Ningpo.exe)
TARGET=ningpo tools/build.sh native                                   # build/native/ningpo.exe
build/native/rundesk.exe build/native/ningpo.exe -userdir=<dir>       # a game, invisible and silent as tests run it
```

`tools/build.sh web` builds every game for the browser, `ningpo.html` among them; to build only Ning Po, configure
`build/web` with `emcmake cmake` as `build.sh` does and run `cmake --build build/web --target ningpo`. Serve
`build/web` (`.claude/launch.json` has `ningpo-web` on port 18779) and open `ningpo.html`; in the Browser pane always
with `?silent=1`. Once the player has agreed to the game's terms, the page imports `ningpo.pyz` and `Ningpo.exe` from
their install folder or a zip of it into its own IndexedDB folder (`/nppersist`) and keeps the settings and best
times there.

Native options: `-gamedir=<dir>` (else `assets/ningpo` next to the exe or up to two levels up, or the retail install
"PopCap Games/NingPo MahJong Deluxe"), `-userdir=<dir>` (the emulated registry, `registry.ini`: the settings and the
best times), `-fast`, `-seed=N` (the game's generator, otherwise seeded from the time of its first event) and
`-script=<file>` (the framework's scripted input: `<update> click <x> <y>`, `rclick`, `key <name>`, `char <c>`,
`screenshot <file>`, `quit`).

## Testing

`tools/check.sh ningpo` (under a minute) plays a scripted game with `-seed=7`: it turns on the game's own cheat
(Backspace, q, Backspace) and presses its match key until no pair is left, ends the game in the no-moves dialog,
enters the name "Test" and reads the new best time back out of `registry.ini`; then it plays the game again, which must
give the same best time and the same screenshots. The script clicks only the start button, the no-moves dialog, the
continue arrow and the best times' OK, never a link button.

Other scripts opened the options, their best times and the rules (and closed them again), and pressed the board's
buttons: the hint (30 seconds on the clock), show moves (the free tiles light up), pause and its screen, undo after a
match (60 seconds), shuffle (120 seconds) and new game, which asks first. In the browser (2026-10-26) the game played
from the loading screen to a new best time, with the mouse and the keyboard, and the time was still in the best times
after a reload of the page. Not yet tested: the sliders, fullscreen, and a player's own matches with the mouse.

An AddressSanitizer build (2026-10-26): `cmake -S . -B build/native-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug
-DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-i686.cmake -DSDL2_DIR=... "-DCMAKE_CXX_FLAGS=-fsanitize=address
-fno-omit-frame-pointer -O1" "-DCMAKE_C_FLAGS=<the same>"`, then `cmake --build build/native-asan --target ningpo` and
copy `libclang_rt.asan_dynamic-i386.dll` (llvm-mingw's `i686-w64-mingw32/bin`) next to the exe. The check's script,
followed by the options and the rules, ran with no report.

### The standard test commands

`src/TestCommands.cpp` answers the commands of `docs/TEST_COMMANDS.md` from a `-script` file natively and from the
test page's command call on the web. Commands that change the game wait in a queue and run at the game's next event
read, through the game's own code, and only while no dialog is open (the game's dialogs run their own loops).

`state` prints `screen` (title, playing, won, gameover), `dialog` (start, splash, options, rules, besttimes, nomoves,
name, question, or null), `mode` (always mahjong), `level` and `layout` (0 to 17), `tileset`, `background`, `timer`
and `penalty` in seconds, `moves` (the free pairs), `tiles` (left on the board), `hints`, `undos`, `shuffles`, `paused`,
`cheat`, `over` and `updates`. `goto` takes title, mahjong (also play or game), options, rules (help) and besttimes
(highscores); `level n` starts layout n; `set` takes layout, tileset or background on the title screen; `win` removes
every pair but the last and matches that one, which ends the game won as a match does; `lose` ends it as running out
of moves does, with the no-moves question; `seed n` seeds the game's generator; `unlock all` answers ERROR, since the
port is the registered game with nothing locked. `move` gives the first free pair the game's own move finder
(`Board::findMove`, the one behind the hint) returns, with clicks on both tiles' faces, or the click that answers the
dialog that is open (the start button, "end game" in the no-moves question, the name "Bot" and Enter, the best times'
OK, the continue arrow), or `MOVE null` when nothing is to be done (paused, or a dialog with no answer of its own).
Ning Po adds `redraw`, which draws the game's picture at once, as the native `screenshot` command does: the test page's
shots read the canvas, which `-fast` draws only every few updates (`tools/shots/tour.py --web`'s driver asks for it
before each shot).

## Left out

The port plays as the registered original does, and that copy stays the reference for every comparison. Two kinds of
code are left out (docs/COORDINATION.md, "Order of the work on the merged games", 2026-11-07).

The trial's code, which a registered copy never reaches: the registration check (`checkRegistration`, the game's and
the engine's, and the cvars `nm_pyrogonRegistration` and `nm_maxTimesPlayed`), the count of games played, the
"full version only" signs drawn over the last layouts and tile sets on the title, the loading bar stopped at a quarter,
the buy button among the board's controls, the engine's startup dialog whose button opened the purchase page, the
loading screen's hidden thanks for registering, and the
four sprites only those used (14, 15, 74 and 75 in `SpriteTable.inc`, which the generator now leaves empty). None of
it showed in a registered copy, so nothing visible changes.

The link out of the game: the options' "Visit PopCap.com For More Games" button, with `openPartnerSite`,
`openPurchasePage`, the engine's `openURL` and its partner and purchase links, and the host's `OpenURL`. This one is
visible. The options' background has a frame for each button: Best Times and an empty one (the trial's register
button) in the first row, the visit button in the second, Done in the third. Done now sits in the second row's frame,
and the third row's frame, empty, is painted out with the wood beside it when the background loads
(`PaintOutFrame` in `src/NingPoDialogs.cpp`), so the dialog shows no gap. The empty frame beside Best Times stays, as
in the registered original. The loading screen's partner logo is a picture, not a link, and stays. Ning Po has no start-up logo for the site's "Always skip start-up
logos" (`Byopop::SiteSkipLogos`) to skip: the only screen before the title is the loading screen, which waits for
its start button.

## Mods

None (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones worth
making.

## Status (2026-10-26)

Everything plays. The trial's parts are left out (the registration dialog, the layouts and tile sets kept for the full
version, the count of games played and the nag at exit): the port plays as registered, so no screen shows a buy or
register button. The options' link to PopCap's site opens through the framework (never in a test run). The sound and
the music go through the platform's mixer as voices of their own; every test runs with the silent driver, so nobody
has listened to them in the port yet. The framework's subsystems the game does not use (the trace, journal and demo
files, the sprite cursor, DirectInput) are left out.

## Where the work was hard

The game owns its loop, as the original did: `PGApp::mainLoop` waits for events, and every dialog runs a loop of its
own (`PGDialog::doModal`) until one of its items ends it. The port lets the game keep those loops: whenever the game
sleeps, the host runs the framework (`NingPoApp::RunFramework`), which handles the window, posts the game's events
and presents the screen; on the web the sleep also yields to the browser, which is why the web build is made with
ASYNCIFY. Two things follow. When the framework is shut down underneath the game (a script's `quit`, the page
closing), the host posts the game a quit event on every sleep until its loops end, as the window's close button
would. And the rules slideshow ignores the quit event and Escape, as the original does, so a script must close it
with its own button.

The original clips by hand nearly everywhere it draws, inline and in two shapes; `src/NingPoDraw.cpp` names the
shapes once and keeps every call where the original has one.

## Changes outside the Ning Po folders

`CMakeLists.txt` (the `BUILD_NINGPO` option), `tools/check.sh` (the `ningpo` suite) and `.claude/launch.json` (the
`ningpo-web` preview on port 18779).
