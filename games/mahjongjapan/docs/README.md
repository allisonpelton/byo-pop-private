# Mahjong Escape: Ancient Japan port

This is a port of Mahjong Escape: Ancient Japan (SpinTop Games, 2006, part of the PopCap Game Collection) to the shared
platform layer. It plays natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of
`MahjongEscapeAJ.exe`, found by decompiling it with Ghidra. The game's files come from the player's own copy and are
never in the repository: `Resource.dll` holds the pictures, sounds, music, layouts and texts, and the exe holds the
menus' texts and two pictures. The port reads both as PE resources and never runs either.

The game runs on JollyBear's engine, which no other port has yet. `engine/` is that engine (the app, the manager and
its layers, the object tree, the fonts, the streams, the widgets and the software display), translated in the
original's shape and mapped onto `platform/` through a small host interface (`jb::Host` in `engine/Host.h`: the clock,
the sleep, the screen, the resources, the registry, links, the clipboard and the cursor). The sound object's BASS
calls become the platform's mixer, with the MO3 music played through `platform/openmpt`. `src/` is the game. The
SexyAppFramework only hosts it (`src/MahjongApp.cpp`: the window, the clock, the input, the sound device and the 800x600
screen the engine presents), as it hosts TipTop and Ning Po. `ENGINE.md` maps the engine; Mahjong Escape: Ancient
China runs on the same code, so its port can reuse the map and most of `engine/`.

Everything a player sees is there: the splash, the main menu, the players, Emperor's Adventure (the map, its 60
stages, the tips, the level's end and the ending), the classic game's 162 layouts, the in-game menu, the options, the
tile sets, the backgrounds, the instructions, pause, undo, hint, shuffle and the special tiles. `GAME_NOTES.md` has the
rules as the code has them, and `DEVIATIONS.md` lists every place where the port's code differs from the original.

## The binary

`MahjongEscapeAJ.exe` was linked on 2006-11-08 by MSVC 6. It has no copy-protection wrapper, no RTTI and no trial code.
The game is 0x401000-0x417680, followed by the C runtime and JollyBear's engine (0x41e240-0x438000). `ENGINE.md` has
the map of the rest. `symbols/` holds the names the port gave the functions.

## Reproducing the analysis

The export lives in `re/mahjongjapan/` (git-ignored, like everything derived from the game):

```
mkdir -p re/mahjongjapan/bin && cp assets/mahjongjapan/MahjongEscapeAJ.exe re/mahjongjapan/bin/
tools/ghidra_import.sh re/mahjongjapan/bin/MahjongEscapeAJ.exe mahjongjapan re/mahjongjapan/gh
python games/mahjongjapan/tools/vtables.py                                  # re/mahjongjapan/vtables.txt, vfuncs.txt
build/native/rundesk.exe bash games/mahjongjapan/tools/ghidra_pass.sh       # functions, symbols, call signatures; export
bash games/mahjongjapan/tools/dis.sh 40a5b0 40a700                          # a range disassembled
python games/mahjongjapan/tools/make_gametext.py                            # src/GameTextTable.inc after a new GT()
```

## Building and running

```
tools/setup_worktree.sh                                                     # a new worktree: the git-ignored files
python games/mahjongjapan/tools/stage_mahjongjapan_assets.py "<install folder>"  # stages assets/mahjongjapan/
cmake --build build/native --target mahjongjapan                            # build/native/mahjongjapan.exe
build/native/rundesk.exe "$(cygpath -w "$PWD/build/native/mahjongjapan.exe")" -userdir=<dir>  # invisible, silent
```

rundesk needs the exe's absolute Windows path. Native options are as follows. `-gamedir=<dir>` names the game's folder;
without it the exe looks in `assets/mahjongjapan` next to itself or up to two levels up, then in the PopCap Game
Collection's install folder. `-userdir=<dir>` holds the players, their saved games and `registry.ini`. `-fast` runs on
the framework's virtual clock, so a scripted run repeats exactly. `-script=<file>` is the framework's scripted input
(`<update> click <x> <y>`, `key <name>`, `char <c>`, `screenshot <file>`, `quit`; 200 updates a second).

For the browser, build `build/web` with `--target mahjongjapan`, serve it (`.claude/launch.json` has
`mahjongjapan-web` on port 18790) and open `mahjongjapan.html`, in the Browser pane always with `?silent=1`. The page
imports `Resource.dll` and `MahjongEscapeAJ.exe` (and the two logo pictures) into its own IndexedDB folder
(`/mjpersist`) and keeps the players there.

## Testing

`tools/check.sh mahjongjapan` takes about 15 seconds. It loads and deals every one of the 222 layouts
(`mahjongjapan_dealtest`: every kind dealt in pairs). Then it plays escape stage 1 from a new player: the name, the
map, the board, the stage won with the clicks `mahjongjapan_dealtest --solve 385` finds (the two Magic Gold Tiles
last), the tip, the level's end and the map again. It plays the same game twice and compares the screenshots with each
other and with the frozen hashes in `check.sh` (`MJ_SHOTS`), which are the regression check for later changes. The
script clicks no link.

`dealtest --solve <layout> [seed quad]` finds the pairs that win a stage. A stage after the first deals most of its
tiles, so it takes the stage's seed and difficulty as well; stage 2 (layout 386, seed 524627644, difficulty 0) dealt
exactly as the solver predicted. Other scripted runs reached the classic list, a classic game, the in-game menu, pause,
the options, the instructions and the player list, and played matches, a hint and an undo. Not yet tested: the
special tiles, a whole adventure, the ending, wide screen and full screen, the web build, and the screen-by-screen
comparison with the original exe.

## Mods

None (the user's rule: no new mods unless someone requests one). `MOD_IDEAS.md` lists the ones worth making.

## Changes outside the game's folder

`CMakeLists.txt` (the `BUILD_MAHJONGJAPAN` option), `tools/check.sh` (the `mahjongjapan` suite) and
`.claude/launch.json` (the `mahjongjapan-web` preview on port 18790).
