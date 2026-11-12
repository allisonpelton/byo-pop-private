# Rocket Mania Deluxe port

A port of PopCap and Nuclide's Rocket Mania Deluxe (2003) that plays natively (Windows, SDL2) and in the browser
(WebAssembly). It is a translation of the original's game classes into code on the shared framework; it ships none of the
game's pictures, sounds, music or texts, which come from your own copy of the game.

## Status (2026-11-11)

The game is translated: the loading screen, the menu, every dialog the registered game shows (difficulty, continue, quit,
options, credits, high scores, record score, hints, level up, the buy-rockets screen), the Board with its three modes
(arcade, classic, strategy), its saved games, the fireworks and sparkles (`Effects.cpp`), the talking head with its hover
tips (`TalkingHead.cpp`) and the classic mode's lanterns. It runs unmodified, with no mod hooks; mods come later through
the standard mods. Left out on purpose: the shareware pieces (the trial clock 0x42f770 and the register dialogs 0x414790,
0x415020, 0x4153b0), because the Steam copy is registered, and the partner and update links of the options dialog.

Both phases of the identical-play pass are done (`IDENTICAL_PLAY.md`: plays identical, then looks identical, in 2D and 3D, with a leak
of the port fixed), and so are the fixes mod (no candidates), the coverage check, the performance pass (the port at 0.25 to 0.78 of the
original's processor time in every pair; GAME_NOTES, "Performance") and the lite mod (none: `LITE_MOD.md`). Still to do, in the order of
CLAUDE.md: code the game does not need, the readability pass, the check against the PopCap Game Collection version, and the stores line
`BYOPOP_BUY_rocketmania` in `cmake/ByopopStores.cmake`, which the coordinator supplies.

## The binary

The game is inside `RocketMania.exe`, PopCap's 2006 wrapper (the Steam release). The game image is linked 2003-09-08 with
Visual C++ 6; its framework is the 2003 SexyApp (`FRAMEWORK_2003.md`). The port never runs the original and does not
need the wrapper's copy protection taken apart: the game's texts are read from the exe by address (`src/GameText.h`).

## Staging the game files

```bash
python games/rocketmania/tools/stage_rocketmania_assets.py ["<install folder>"]
```

copies images, data, sounds, music, properties and `RocketMania.exe` from your install (the Steam library's
`Rocket Mania Deluxe` folder by default) into the git-ignored `assets/rocketmania/`. Nothing is written into the install.

## Building and running

```bash
TARGET=rocketmania tools/build.sh native        # build/native/rocketmania.exe
build/native/rundesk.exe build/native/rocketmania.exe
```

(`rundesk` keeps test runs invisible and silent; see CLAUDE.md. For a normal game run `rocketmania.exe` directly.) Options:
`-gamedir=`, `-userdir=`, `-modsdir=`, `-seed=N`, `-fast`, `-script=<file>`.

## Testing

```bash
tools/check.sh rocketmania
```

runs `games/rocketmania/tools/menu_test.txt` (load, play link, menu, hover, difficulty dialog) and
`games/rocketmania/tools/board_test.txt` (a game of each mode played by the autoplay hook) and fails on a bad exit status,
a failed load, a missing screenshot, or screenshots that do not differ. Test scripts use the framework's `-script`
commands plus `startgame` (clicks the play link once the loading screen shows it) and the commands of
`src/testhooks/Autoplay.cpp`: `autoplay <mode> <difficulty> <updates>`, `gameover <mode> <score>`, `hint`, `levelup`,
`endlevel`, `buyrockets <over> <level> <coins> <meter>` and `effect <kind> <x> <y> <life> [<layer>]`. A script that starts
a game does it after the loading screen (update 5 `startgame`, the next command a few hundred updates later). To run a
script invisibly and silently, `games/rocketmania/tools/run_script.sh <script> <user dir> [seconds]` starts it under
`rundesk`; with `RM_BUILD=native-asan` it runs the AddressSanitizer build.

An AddressSanitizer build (2026-10-26): `cmake -S . -B build/native-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug
-DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-i686.cmake -DSDL2_DIR=... "-DCMAKE_CXX_FLAGS=-fsanitize=address
-fno-omit-frame-pointer -O1" "-DCMAKE_C_FLAGS=<the same>"`, then `cmake --build build/native-asan --target rocketmania` and
copy `libclang_rt.asan_dynamic-i386.dll` (llvm-mingw's `i686-w64-mingw32/bin`) next to the exe; set `ASAN_SYMBOLIZER_PATH`
to llvm-symbolizer for readable reports. A game of each mode (5000 updates each) and every scenario script (menu, dialogs,
hints, level up, the end of a level with the dragon, buy rockets, effects, saves, the lanterns) ran with no report. The one
report it gave was a test artifact: an `autoplay` command that started a new board while the level-up screen of the game
before was still up (that screen keeps the old board's random generator); the test commands now close the dialogs first.

## Tools (`games/rocketmania/tools`)

`decread.py` prints a function of the Ghidra export in a readable form, `strfind.py` finds strings and their users,
`vtslots.py` and `ptrtable.py` read virtual tables and pointer tables from the game image, `gen_res.py` writes
`Res.h/.cpp` from the resource table, `make_gametext.py` writes the table of text addresses `GameTextTable.inc`, and
`make_headdata.py` writes `HeadData.inc` (the talking head's moods and the hover tips' places, numbers and addresses only).
They read `re/rocketmania/` (git-ignored: Ghidra's export and the carved game image).
