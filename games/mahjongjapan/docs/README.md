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
(`/mjpersist`) and keeps the players there. In the browser (2026-11-01) a new player played escape stage 1 to the level's end
and the map, with the same score as natively (6,420), and was still there after a reload of the page.

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
the options, the instructions and the player list, and played matches, a hint and an undo.

`dealtest --stage <n>` solves escape stage n with its own seed, special tiles included (keys and locks, the Joker and
the Magnet), moving only tiles a click can reach. It also prints when the timed tips come and which moves fly special
tiles. `tools/adventure.py` turns the 60 solutions into one `-script`. On 2026-11-01 that script played Emperor's
Adventure from a new player through all 60 stages and the ending (the map with its treasures, then the showcase)
natively, every stage won.

The original exe, run under origrun in software mode, matched the port screen for screen through stage 1: the name
dialog, the map, the board, the tips, the tray, the score and the level's end. The only differences were the clock
and tiles caught in flight. In Windows Sandbox (`origrun --sandbox`, 2026-11-01) stage 1 matched again, and so did
stages 2, 3, 6, 7, 10, 13, 19, 25, 31, 37, 43, 49, 55 and 60 with the ending, each played from the port's player saved
before it (re-signed for the sandbox's folder: the checksum covers the exe's folder, `DEVIATIONS.md`). The maps, the
boards and the screens after each level were identical to the pixel. What differed was the clock (the original plays
on the wall clock, slower), animation on its own clock (the tiles a tip lights, the arrow under the free-tiles tip,
the map's marker, the ending's next line) and the PAUSE button lit under the mouse on the port: the original tests
the hover with `GetCursorPos` (0x42a820), and origrun's posted clicks do not move the sandbox's cursor.
A native run that started in full screen with
"Wide Screen" on reached stage 1's board as the windowed runs do (`DEVIATIONS.md` has how full screen is shown).
The web build played the same 60-stage script under Node (`games/bejeweled/tools/verify/wasmrun.js game`), and
its 121 screenshots were identical to the native run's.

A tour from a saved player (the main menu, the options, the four pages of instructions, the player list, the classic
layouts, a classic game with its tip, losing and regaining the focus, pause and the in-game menu) matched the original
in the sandbox. What differed was the menu's swinging lanterns, the classic deal and the layouts' previews (seeded from
the clock, as in the original) and the counts that follow the deal. The web build under Node took the same 18 shots
identical to the native ones.

The sound is compared by what plays, not by the audio. origrun's DirectSound stand-in hears nothing of this game:
the packed `bass.dll` loads DirectSound for itself. Instead `tools/bassproxy/` (built with llvm-mingw's
`i686-w64-mingw32-clang -shared bassproxy.c bassproxy.def -o bass.dll`) goes beside a copy of the exe (`origrun
--copy-only`, the real BASS renamed `bass_real.dll`) and logs every BASS call to `basslog.txt`. Windows Sandbox has no
sound device, so the proxy gives `BASS_Init` BASS's "no sound" device, which plays the music but loads no sample; it
answers for the samples itself, as playing until stopped. The port writes the same events to the file named by the
environment's `MJ_SOUNDLOG`, and `tools/soundlog.py <basslog.txt> <port log>` matches the two by the CRC of each
sound's data and lines them up. On 2026-11-01 the tour above (78 events) and stages 2, 6, 10 and 60 with the ending
(166 to 402 events each) played the same sounds and music in the same order: the same samples at the same volume,
the music's loads, fades (1100 ms), stops, pauses with the focus and the in-game pause, and its jumps to an order
(the sync of 0x437300, which in a real-time run of the port came 46 to 330 ms after it was set, against 239 to 416 ms
in the original: the next row of the step). The only other differences were samples the proxy kept "playing" after
they had ended, which the game then paused and resumed.

## Mods

None (the user's rule: no new mods unless someone requests one). `MOD_IDEAS.md` lists the ones worth making.

## Changes outside the game's folder

`CMakeLists.txt` (the `BUILD_MAHJONGJAPAN` option), `tools/check.sh` (the `mahjongjapan` suite) and
`.claude/launch.json` (the `mahjongjapan-web` preview on port 18790).
