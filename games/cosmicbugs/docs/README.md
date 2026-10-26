# Cosmic Bugs port

A port of Cosmic Bugs (developed by Retro64, published by PopCap Games; © 2007 PopCap Games) to the shared platform
layer: it plays natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of
`CosmicBugs.exe`, found by decompiling it with Ghidra; the game's files (the art banks, the animations and boards in
`animations/`, the sounds and music in `audio/`, and `CosmicBugs.exe`, whose texts are read at run time) come from
the player's own copy and are never in the repository.

Cosmic Bugs is not a SexyAppFramework game. It runs on Retro64's own engine, which Water Bugs shares: plain C over
global state, with no classes, software blitters into a 16-bit back buffer, its own bank, font, FLIC and XML code, and
DirectDraw and BASS underneath. The port keeps that structure. `engine/` is the engine, translated function by
function (`R64*.cpp`), and `src/` is the game on top of it (`CB*.cpp`). Everything the engine needs from the machine
goes through one interface, `r64::SysHost` in `engine/R64Sys.h`: the files, the screen, the clock, the pointer, the
sounds, the music, links, the save folder and the player's name. `src/CosmicBugsApp.cpp` implements it over the
platform, with the SexyAppFramework hosting the window, the clock, the input and the sound device, and presenting the
800x600 screen the game draws into; `test/gamerun.cpp` implements it over plain memory for the headless runner.
`ENGINE.md` maps the engine, for this port and for Water Bugs.

Everything a player sees plays: the PopCap logo, the title with its demo and tips, the instructions, the options,
the Hall of Fame with the player's name, the credits, and the boards with their lines, fills, bugs, power-ups, lives
and level intros, up to the game's end and the new high score. The settings keys work as in the original (F5 the
sound, F6 the music, F7 the full screen, F8 stretched or centred), and the settings and saves are the original's
files (`cp.cfg`, `cb.dat`).

## The binary

`CosmicBugs.exe` was linked on 2007-11-20 by MSVC 2005 (linker 8.0). It carries no copy protection wrapper (its
sections are `.text`, `.rdata`, `.data` and `.rsrc`) and no RTTI, and it is the full version: the registered flag at
0x460004 is set, so the trial's parts (the time limit, the boards kept for the full version) never run, though the
port translates them. The code is 0x401000-0x457af6, the game's functions and the engine's interleaved, then the
MSVC runtime; BASS is the separate `bass.dll`. The port translates about 560 functions (294 of the game, 269 of the
engine, each with its address in a comment). `docs/symbols/10_engine.jsonl` holds the names given in Ghidra.

The art is two banks, each an XML index (`.bbk`) over a file of pixels (`.dat`): `art8` holds 730 8-bit images (the
fonts and the loading screen) with three palettes, `art` 219 32-bit images that the engine converts to the screen's
16 bits. `animations/` holds 124 FLIC files (bugs.xml names 91 animations; masks are FLICs of their own) and the XML
game files: the menus and screens (`bugs.xml`, which names the boards' `level1.xml` as its next file), and the
localised strings (`local.xml`). `audio/` holds 33 WAV sounds and five MO3 modules.

The game's texts in the code (the default high-score names, the menus' values, the power-ups' names, the requesters)
are read from the player's exe by address: `GT(0x...)` in the code, `src/GameTextTable.inc` (address, length and
hash, made by `tools/make_gametext.py`) and `src/CBGameText.cpp`, which checks each one against the exe at start-up.

## Reproducing the analysis

The export lives in `re/cosmicbugs/` (git-ignored, like everything derived from the game). `tools/ghidra_pass.sh`
applies the port's symbols, fixes what Ghidra gets wrong in this exe (the MSVC floating-point helpers' signatures,
the blit wrappers' tail calls, see `ENGINE.md`) and exports the decompilation again.

```
mkdir -p re/cosmicbugs/bin && cp assets/cosmicbugs/CosmicBugs.exe re/cosmicbugs/bin/
tools/ghidra_import.sh re/cosmicbugs/bin/CosmicBugs.exe cosmicbugs re/cosmicbugs/gh  # the first analysis and export
bash games/cosmicbugs/tools/ghidra_pass.sh                                           # symbols, fixes, the export again
python games/cosmicbugs/tools/decread.py 41ee60                                      # a function, readable
bash games/cosmicbugs/tools/dis.sh 41ee60 41ef00                                     # its disassembly
python games/cosmicbugs/tools/make_gametext.py                                       # GameTextTable.inc after a new GT()
```

## Building and running

```
tools/setup_worktree.sh                                                        # a new worktree: the git-ignored files
python games/cosmicbugs/tools/stage_cosmicbugs_assets.py "<the game's folder>"  # stages assets/cosmicbugs/
TARGET=cosmicbugs tools/build.sh native                                        # build/native/cosmicbugs.exe
tools/build.sh web                                                             # build/web/cosmicbugs.html, .js, .wasm
```

`cosmicbugs.exe [-gamedir=<folder>] [-userdir=<folder>]` finds `assets/cosmicbugs/` or the retail install by itself
and keeps the player's files in its own folder of the user's application data. The web page imports the game's
files from the install folder or a .zip of it into the browser's storage (`/cbpersist`); `.claude/launch.json`'s
`cosmicbugs-web` serves `build/web` on port 18788 (open `cosmicbugs.html?silent=1` in tests).

## Testing

`tools/check.sh cosmicbugs` (about 5 seconds) loads both art banks, the fonts and every animation
(`cosmicbugs_banktest`), then plays a scripted game headless twice (`cosmicbugs_run`, the game on a clock of its own
with scripted input and PNG screenshots): Start Game, the instructions' Play, a line down the first board, the aim
switched with the right button and a line across. Both runs must reach the expected score and fill and the same
screenshots. Then it runs the windowed exe twice with the same clicks (`-fast -script`); its state after the first
line must be the runner's, and its screen the same both times. The scripts click only the menu, the instructions and
the board, never a link.

Other runs (2026-10-26) toured the options (and their Back), the Hall of Fame, the credits and Quit, and played five
minutes of game time with a line every 2.5 seconds at varying places: several boards finished (their bonuses, the
next boards' intros and tile sets), lives lost down to the game's end, and the Hall of Fame with the new score under
the player's name, saved in `cb.dat`. In the browser (the Browser pane, `?silent=1`) the game imported its files,
showed the title and played a board with the mouse.

Not yet tested: the player requesters (a new name, deleting a player), the level selection, switching the screen
mode natively (F7, F8 and Alt+Enter on the real desktop), and the sound and music by ear: every test runs with the
silent driver.

## Mods

None (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones worth
making, and the fix of the easy setting's label.

## Where the work was hard

Time. The original steps the game once for every millisecond that passed, then draws once (GameFrame, 0x41ee60
split into GameStart, GameFrame and GameEnd), so the port runs one pass of that loop per framework update and lets
the game do its own stepping. The framework's `-fast` clock does not advance by exactly 10 ms on every update (the
updates it draws do not wait), so a windowed run and a headless run with the same input end in the same game state
but not with the same animation phases; the check compares the state between them and the pixels only between runs
of the same kind.

The draw order. Sprites of equal depth are drawn in the order MSVC 2005's qsort leaves them, after a comb sort, with
a comparator that never answers 0; the port translates the runtime's qsort from the exe (CrtQsort) so the order is
the original's.

The mouse. The game samples the buttons once per pass, after all waiting messages, so a press and release between
two passes is lost, in the original as in the port. Scripts hold a button for a few updates; the framework's
one-update `click` is too short.

The web. The game's settings default to the full screen, and SDL's request for it from outside a click left the
canvas black; on the web the game's screen mode now leaves the window alone, and the page's Fullscreen button owns
the browser's full screen.

## Changes outside the Cosmic Bugs folders

`CMakeLists.txt` (the `BUILD_COSMICBUGS` option), `tools/check.sh` (the `cosmicbugs` suite) and `.claude/launch.json`
(the `cosmicbugs-web` preview on port 18788). The web page also needs `BYOPOP_BUY_cosmicbugs` in
`cmake/ByopopStores.cmake`, which the coordinator keeps.
