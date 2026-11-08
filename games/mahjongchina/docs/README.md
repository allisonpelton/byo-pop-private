# Mahjong Escape: Ancient China port

This is a port of Mahjong Escape: Ancient China (SpinTop Games, 2006, part of the PopCap Game Collection) to the shared
platform layer. It plays natively (Windows, SDL2) and in the browser (Emscripten). China is the same program as
Mahjong Escape: Ancient Japan with other resources and a few changes, so the port is Japan's port
(`games/mahjongjapan`) with every difference between the two exes carried over. `engine/` is a copy of Japan's
JollyBear engine, `src/` is the game. The game's files come from the player's own copy and are never in the repository:
`Resource.dll` holds the pictures, sounds, music, layouts and texts, and `MahjongEscapeAC.exe` the menus' texts and two
pictures. The port reads both as PE resources and never runs either.

Everything a player sees is there: the splash, the main menu, the players, Dynasty Adventure (the map, its 52 levels
in 11 stages, the tips, the level's end and the ending), the classic game's 126 layouts, the in-game menu, the options,
the tile sets, the backgrounds, the instructions, pause, undo, hint, shuffle and the special tiles. `GAME_NOTES.md`
says what differs from Japan and how the differences were found, `ENGINE.md` what differs in the engine, and
`DEVIATIONS.md` every place where the port's code differs from the original.

## The binary

`MahjongEscapeAC.exe` was linked on 2006-08-21 by MSVC 6. It has no copy-protection wrapper, no RTTI and no trial code.
The code was ported from Japan's, so the comments in it give **Japan's** code addresses (the data and text
addresses are China's own); `re/mahjongchina/compare/addrmap.tsv` pairs the two exes' functions, and the comments
change to China's addresses in the readability pass. `symbols/` holds China's engine names for the Ghidra pass.

## Reproducing the analysis

The export lives in `re/mahjongchina/` (git-ignored, like everything derived from the game). The tools are Japan's,
pointed at China:

```
mkdir -p re/mahjongchina/bin && cp assets/mahjongchina/MahjongEscapeAC.exe re/mahjongchina/bin/
tools/ghidra_import.sh re/mahjongchina/bin/MahjongEscapeAC.exe mahjongchina re/mahjongchina/gh
python games/mahjongchina/tools/vtables.py
build/native/rundesk.exe bash games/mahjongchina/tools/ghidra_pass.sh
build/native/rundesk.exe bash games/mahjongchina/tools/insndump.sh   # re/<game>/insns.txt for both games
python games/mahjongchina/tools/compare_exes.py              # re/mahjongchina/compare/: the pairs, constants, diffs
```

`re/mahjongchina/diffnotes.txt` lists each difference the comparison found, by Japan's address, and how the port
carries it.

## Building and running

```
python games/mahjongchina/tools/stage_mahjongchina_assets.py "<install folder>"   # stages assets/mahjongchina/
cmake --build build/native --target mahjongchina                                # build/native/mahjongchina.exe
build/native/rundesk.exe "$(cygpath -w "$PWD/build/native/mahjongchina.exe")" -userdir=<dir>
```

The options are Japan's (`-gamedir`, `-userdir`, `-fast`, `-script`). For the browser, build `build/web` with
`--target mahjongchina`, serve it (`.claude/launch.json` has `mahjongchina-web`) and open `mahjongchina.html`, in the
Browser pane always with `?silent=1`. The page imports `Resource.dll`, the exe and the two logo pictures.

## Testing

`tools/check.sh mahjongchina` takes about 10 seconds. It loads and deals all 178 layouts (`mahjongchina_dealtest`),
then plays Dynasty Adventure's first level from a new player with the clicks `mahjongchina_dealtest --stage 1` finds,
to the level's end and the map, twice. The two runs must be identical, and their screens must match the frozen hashes
in `check.sh` (`MJC_SHOTS`), the regression check for later changes. The script clicks no link.

`dealtest --stage <n>` solves level n with its own seed and special tiles, as Japan's does. It solves every level but
11, which has six keys and eight locks: no search found an order that frees them (`MJ_SOLVE_TRIES` and
`MJ_SOLVE_LIMIT` raise the effort). `tools/adventure.py` turns the solutions into one `-script`, and with
`MJ_ADV_FIRST=<n>` starts at level n from a saved player at progress n-1 (`tools/resign.py --progress`). On
2026-11-01 the adventure was played natively in two parts, levels 1-10 from a new player and levels 12-52 from a
player at progress 11, every level won, to the ending's showcase.

The original runs in Windows Sandbox through `origrun --sandbox`, and `tools/sandbox_run.sh` prepares the copy. China
needs one stand-in more than Japan: its window class has a black background brush, and in the sandbox every shot of it
came back black. `tools/ddrawcap/` (a DDRAW.dll beside the copy) clears the brush and changes nothing the game draws
(`DEVIATIONS.md`, "The window's background"). The sound is compared as Japan's is: `tools/bassproxy/` logs the
original's BASS calls, the port writes the same events to `MJ_SOUNDLOG`, and `tools/soundlog.py` lines them up.
`tools/tour.py` and `tools/stagerun.py` write the same shots for both sides; the shots are compared with
`tools/shotdiff`.

On 2026-11-01 these matched the original. The tour (the main menu, the options, the four pages of instructions, the
player list, the classic list and its second page, a classic game with its tip, losing and regaining the focus, pause
and the in-game menu) took the same 18 screens and played the same 78 sound events. Levels 1, 2, 4, 8, 13 and 52 with
the ending matched too, each played from a player saved before it. The maps, the boards, the screens after each level
and the ending's were identical to the pixel. What differed on screen was what differs in Japan's comparison: the
clock, the classic deal and the layouts' previews (seeded from the clock), animation on its own clock (the tiles a tip
lights, the tip's arrow, the dialogs' tassels, the map's marker, the menu's figures) and the PAUSE button lit under
the mouse on the port (the original reads the cursor, which origrun's posted clicks do not move). What differed in the
sound (204 to 389 events a level) was timing: the music's jump to an order lands at the next row a multiple of four,
so its place among the samples moves; the proxy pauses and resumes samples that have already ended; and the map's
sound when a stage's name fades out (0x17e) is skipped when the fade lands exactly on 0, which depends on the frame
times, in the original as in the port. The sound comparison needs the port in real time: under `-fast` the game runs
ahead of the audio device and a music jump can be overtaken by the next.

The web build plays the suite's level under Node (`games/bejeweled/tools/verify/wasmrun.js game`) to the same screens
as the native build, apart from the full-screen corner button, which a run without `registry.ini` shows natively.

**The standard test commands and the real-input drivers (2026-11-08).** The port answers `state`, `goto`, `level`, `unlock all`,
`set`, `win` and `move` (`GAME_NOTES.md`, "The standard test commands, the fixes mod and the finishing pass"), the files being Ancient Japan's, copied by `syncbot.sh`, `move` from the port's own bot (`src/Bot.cpp`).
`games/mahjongjapan/tools/realplay/drive.py` (one script for both games: `--game japan` or `--game china`) plays the game with real SDL events
under rundesk: the menu, the help, a new player, the whole adventure to the ending, a classic game by the bot, every button
of the board and of the in-game menu, a second classic game, the focus lost and regained, Quit; `--fullscreen` does it from
Alt+Enter. `tools/web/realplay.js` plays the web build through the DOM's events (`?driver=`). `tools/realplay/modtest.py`
checks the fixes mod, `perfload.py` times the launch, `syncbot.sh` copies the bot and the commands to the other game.

The comparison found two differences the first comparison of the exes had missed: Board::Init loads its sounds in
another order, and Board::Measure has no -5 (China's board stands 5 pixels lower). Both are fixed.

## Mods

The fixes mod (`mods/fixes`: `skipLogos`, `seededDeals` and `steadyClock`, on by default on the site; `GAME_NOTES.md`, "The standard test commands, the fixes mod and the finishing pass"). No other mods (the user's rule: no new mods unless someone requests one); `MOD_IDEAS.md` lists
the ones worth making.

## Changes outside the game's folder

`CMakeLists.txt` (the `BUILD_MAHJONGCHINA` option), `tools/check.sh` (the `mahjongchina` suite),
`tools/origrun/games.tsv` (its row), `docs/ORIGRUN.md` (its status), the save import's row and `.claude/launch.json`
(the `mahjongchina-web` preview). `tools/check.sh` also holds the screens frozen again for the shared JPEG loader, `cmake/ByopopWeb.cmake` ships the fixes mod on the
site, and `web/testkit.js` has the pattern fix that `move`'s null answer needed (shared commits).
