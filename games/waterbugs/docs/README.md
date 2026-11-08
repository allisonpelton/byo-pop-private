# Water Bugs port

A port of Water Bugs (developed by Retro64, published by PopCap Games; © 2007 PopCap Games) to the shared platform
layer: it plays natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of
`WaterBugs.exe`, found by decompiling it with Ghidra; the game's files (the art banks, the animations and boards in
`wanims/`, the sounds and music in `audio/`, and `WaterBugs.exe`, whose texts are read at run time) come from the
player's own copy and are never in the repository.

Water Bugs is the sequel to Cosmic Bugs and runs on the same engine, Retro64's own: plain C over global state, with
no classes, software blitters into a 16-bit back buffer, its own bank, font, FLIC and XML code, and DirectDraw and
BASS underneath. The two executables differ in addresses (the same function sits 0x30 to 0x4e0 bytes further on, by
region) and in what the game does, not in how the engine works, so this port is the Cosmic Bugs port with Water
Bugs' code: `engine/` is the engine (`R64*.cpp`) and `src/` the game on top of it (`WB*.cpp`, namespace `wb`).
Everything the engine needs from the machine goes through one interface, `r64::SysHost` in `engine/R64Sys.h`: the
files, the screen, the clock, the pointer, the sounds, the music, links, the save folder and the player's name.
`src/WaterBugsApp.cpp` implements it over the platform, with the SexyAppFramework hosting the window, the clock, the
input and the sound device, and presenting the 800x600 screen the game draws into; `test/gamerun.cpp` implements it
over plain memory for the headless runner. `ENGINE.md` says how the engine maps to the one in
`games/cosmicbugs/docs/ENGINE.md`, which describes it.

What plays: the title with its resume question, the world map, the boards with their beams, fills, bugs, power-ups,
lives and the hints of the first levels, the results requester of a won level and the quit requester that Esc opens.
The settings and saves are the original's files (`cp.cfg`, `cb.dat`).

## What differs from Cosmic Bugs

The theme and the art; the bubbles that sway as they rise (the stars of Cosmic Bugs drifted sideways); beams with a
harpoon at the head; a ship that comes in over 700 updates with sparkles circling in; the enemy kinds and the
power-up kinds renumbered, with their tables; the enemy table of the levels (`Level_EnemyCounts`) with a fixed tint
for pairs and singles; a fill that starts from the far corner of the board; hints for the first three levels; Esc on
a board asks whether to quit (and saves the game) where Cosmic Bugs paused; a won level shows "level finished" and
then a results requester where Cosmic Bugs showed its bonus texts; the panel's numbers (percent, score, lives) and a
quit button; the random levels are as many as the file says, with no extra ones for the registered game.
`tools/fdiff.py` shows any function's differences with its Cosmic Bugs twin.

## The binary

`WaterBugs.exe` (1.15) was linked on 2007-10-30 by MSVC 2005 (linker 8.0). It carries no copy protection wrapper (its
sections are `.text`, `.rdata`, `.data` and `.rsrc`), and its registered flag at 0x460004 is set, so the trial's
parts (the time limit, the boards kept for the full version) never run, though the port translates them. The code is
0x401000-0x457d26, the game's functions and the engine's interleaved with the MSVC runtime; BASS is the separate
`bass.dll`. Each translated function has its address in a comment. `docs/symbols/10_engine.jsonl` holds the names
given in Ghidra.

The art is two banks, each an XML index (`.bbk`) over a file of pixels (`.dat`): `water_art8` holds the 8-bit images
(the fonts and the loading screen) and `water_art` the 32-bit ones that the engine converts to the screen's 16 bits.
`wanims/` holds 140 FLIC files and the XML game files: `water_bugs.xml` (the menus and screens, which names the
boards' `water_level1.xml` as its next file) and `local.xml` (the localised strings). `audio/` holds 36 WAV sounds
and five MO3 modules (the music's files are still called `cosmo*.mo3`).

The game's texts in the code (the default high-score names, the menus' values, the power-ups' names, the hints and the
requesters) are read from the player's exe by address: `GT(0x...)` in the code, `src/GameTextTable.inc` (address,
length and hash, made by `tools/make_gametext.py`) and `src/WBGameText.cpp`, which checks each one against the exe at
start-up.

## Reproducing the analysis

The export lives in `re/waterbugs/` (git-ignored, like everything derived from the game). Cosmic Bugs' export
(`re/cosmicbugs/`) is the other half of the comparison tools.

```
mkdir -p re/waterbugs/bin && cp assets/waterbugs/WaterBugs.exe re/waterbugs/bin/
tools/ghidra_import.sh re/waterbugs/bin/WaterBugs.exe waterbugs re/waterbugs/gh  # the first analysis and export
bash games/waterbugs/tools/ghidra_pass.sh                                        # symbols, fixes, the export again
python games/waterbugs/tools/decread.py 412f50                                   # a function, readable
bash games/waterbugs/tools/dis.sh 412f50 412fd0                                  # its disassembly
python games/waterbugs/tools/fdiff.py 412f50 [412960]                            # it against its Cosmic Bugs twin
python games/waterbugs/tools/make_gametext.py                                    # GameTextTable.inc after a new GT()
```

## Building and running

```
tools/setup_worktree.sh                                                          # a new worktree: the git-ignored files
python games/waterbugs/tools/stage_waterbugs_assets.py "<the game's folder>"      # stages assets/waterbugs/
tools/build.sh native                                                            # every native target, as below
TARGET=waterbugs tools/build.sh native                                           # build/native/waterbugs.exe alone
TARGET=waterbugs_run tools/build.sh native                                       # the headless runner (the check needs it)
TARGET=waterbugs_banktest tools/build.sh native                                  # the bank test (the check needs it)
tools/build.sh web                                                               # build/web/waterbugs.html, .js, .wasm
```

`tools/check.sh waterbugs` needs all three native targets: build them one by one, or run the whole native build.

`waterbugs.exe [-gamedir=<folder>] [-userdir=<folder>]` finds `assets/waterbugs/` or the retail install by itself and
keeps the player's files in its own folder of the user's application data. The web page imports the game's files from
the install folder or a .zip of it into the browser's storage; `.claude/launch.json`'s `waterbugs-web` serves
`build/web` on port 18789 (open `waterbugs.html?silent=1` in tests).

## Testing

`tools/check.sh waterbugs` loads both art banks, the fonts and every animation the game ships (`waterbugs_banktest`,
which fails on any font or shipped animation that does not load), then plays a scripted game headless twice
(`waterbugs_run`, the game on a clock of its own with scripted input and PNG screenshots): Start Game, the world map's
first area, the first board's start, a beam across the board and a second one. Both runs must reach the expected score
and fill and the same screenshots. Then it runs the windowed exe twice with the same clicks (`-fast -script`); its state
after the first beam must be the runner's, and its screen the same both times. The scripts click only the menus and the
board, never a link. Every run of a game exe goes through `build/native/rundesk.exe` (a private desktop, no sound).

Other runs (2026-10-26) played a level 1 board to its win (the hints of the first beams, "level finished", the results
requester with the records), went on into level 2 (its bug, its hints) and opened the quit requester with Esc and
cancelled it. The screens were read from screenshots; the sound and music were not heard (every test runs with the
silent driver).

Not yet tested: the options, the Hall of Fame and the credits, the player requesters (a new name, deleting a player),
the later levels' tables (levels 4 on, whose enemy kinds and tints come from `Level_EnemyCounts`), the power-ups of
every kind, the game's end with a new high score, and switching the screen mode natively.

The finishing pass (2026-11-08) added the standard test commands and the bot behind `move` (`src/testhooks/`), and drivers
that play a whole game of each of the six modes (shooter mode auto or manual, difficulty 0, 1 and -1) by real input, natively
under rundesk (windowed and full screen, with the focus lost and regained, minimized and restored) and in the page
(`tools/realplay/`, `tools/web/`); `GAME_NOTES.md` lists them with the test commands and the performance numbers.

## Deviations

`DEVIATIONS.md` lists where the port is not a line-for-line translation of the exe, and why none shows.

## Mods

One: `mods/fixes`, the fixes mod (`steadyClock`, which steps the game from the updates and not the machine's clock, and
`easyLabel`, which mends the Options screen's "5s"), on by default on the site and off in the port itself, where the game is the
original. `GAME_NOTES.md` describes it and the fix candidates it left alone. No other mod is made; `MOD_IDEAS.md` maps the 29
standard mods of `docs/MOD_ROADMAP.md` onto the game and says why any is skipped.

## Changes outside the Water Bugs folders

`CMakeLists.txt` (the `BUILD_WATERBUGS` option), `tools/check.sh` (the `waterbugs` suite) and `.claude/launch.json`
(the `waterbugs-web` preview on port 18789). The web page also needs `BYOPOP_BUY_waterbugs` in
`cmake/ByopopStores.cmake`, which the coordinator keeps.
