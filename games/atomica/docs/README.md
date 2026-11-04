# Atomica Deluxe port

A port of PopCap's Atomica Deluxe (version 2.52, 2003-03-28) to the shared SexyAppFramework port: it plays natively
(Windows, SDL2) and in the browser (Emscripten). The game code is a translation of the original's game classes, found
by decompiling `Atomica.exe` with Ghidra; the game's files (pictures, sounds, the module music and the texts, which
are read from the exe at run time) come from the player's own copy and are never in the repository.

Everything the original's player sees is ported: the main menu, the strategy and the action modes from the first level to
the game over, the dialogs, the tutorial boxes, the high scores, the bonus molecules, the five elements and the swirls
(see Status for what is not the original's exact picture).

## The binary

`Atomica.exe` is a 2003 MSVC 7.0 (Visual Studio .NET 2002) build with no copy protection wrapper and no RTTI for the
game's classes. `GAME_NOTES.md` has the class map, the vtables (found by scanning `.rdata` for runs of function
addresses), the data layouts and the rules as the code has them. The 2003 SexyApp is one class (`SexyApp : ButtonListener`
with 55 virtual slots) and the widgets are flat; `FRAMEWORK_2003.md` has what the port changes to run it on the shared
framework.

## Reproducing the analysis

```
tools/ghidra_import.sh ...            # with MSVC70=1, or `_free` is taken for a function that does not return
python games/atomica/tools/decread.py 412cd0 40ef10      # a function of the export, readable: names, game strings, MSVC string idioms folded
python games/atomica/tools/stringaddr.py "Version "      # the address of a game text (to write GT(0x...))
```

The export goes to `re/atomica/gh/` (git-ignored, like everything derived from the game). Functions that Ghidra did not
decompile (some vtable-only ones) are read with `llvm-objdump -d --no-show-raw-insn -M intel --start-address=...`. The
decompiler drops stack arguments and x87 operands, and sometimes a term of an array index: check coordinates, alphas and
indices of a draw call in the disassembly (see "Where the work was hard").

## Building and running

```
tools/setup_worktree.sh               # a new worktree: copies the git-ignored files from the main checkout
python games/atomica/tools/stage_atomica_assets.py <the game's folder>   # stages assets/atomica/
TARGET=atomica tools/build.sh native  # build/native/atomica.exe
TARGET=atomica tools/build.sh web     # build/web/atomica.html (serve build/web; the page imports the player's game files)
build/native/rundesk.exe build/native/atomica.exe -userdir=<dir>         # a game, invisible and silent as the tests run it
```

`games/atomica/tools/run_script.sh <script> <user dir> [seconds]` runs the game on a script of `<update> screenshot <file>`,
`<update> click <x> <y>` and `<update> quit` lines under rundesk. In the browser pane open the web build with
`?silent=1`.

## Testing

`tools/check.sh atomica` plays a strategy game and an action game with the bot (`src/testhooks/Autoplay.cpp`, started by
`-autoplay=<seed> -autoplaymode=strategy|action`) and compares the last line of each with the expected one in
`tools/check.sh`; a rerun of the strategy game must have the same log and the same screenshots. The bot looks for a block of
four atoms of one colour that lacks one and sends it an atom, otherwise it makes a random move; it is weak (it reaches
the game over of level 1 in the strategy mode) but it runs everything a player runs: selecting, paths, molecules, the
meter, new atoms, the end of a game, the menu.

An AddressSanitizer build (2026-10-25): `cmake -S . -B build/native-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-i686.cmake -DSDL2_DIR=... "-DCMAKE_CXX_FLAGS=-fsanitize=address -fno-omit-frame-pointer -O1" "-DCMAKE_C_FLAGS=<the same>"`, then `cmake --build build/native-asan --target atomica` and copy `libclang_rt.asan_dynamic-i386.dll` (llvm-mingw's `i686-w64-mingw32/bin`) next to the exe. The bot's strategy and action games (with and without `-autoplayfinish`, which takes it through 23 levels), and a script that saves a game and continues it, ran with no report.

## Mods

None yet (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones worth making.

## Status (2026-10-25)

Everything the original's player sees is in, each function with a `// 0x<addr>` comment: the loading (pictures, the 25
shrunk copies and rotating masks of each atom, sounds), the title screen, the Grid (input, update, drawing, the molecule search,
the move that follows each of the player's, the magnet, all 41 set-ups of the strategy mode, the bonus levels, the action
mode's set-up and drain, the radiation rounds, the saved game), the Atom (placing, paths, pre-atoms, the biohazard, chromium),
the path finder, the effects, the Board (HUD, meter, atomic bomb, freeze and air counters, signs, floating scores, the help
popups a click opens), Atomicus' voices, the bonus molecules (30 shapes in three tiers) and the five element atoms with their
effects, the tutorial boxes (`HintBox`, 28 hints; their lines are generated from the decompilation by `tools/genhints.py`, numbers only)
and `Grid::CheckHints`, the swirls (the tunnel behind the field and the atoms that circle in it, `Swirl.cpp`), the main menu
(the doors that close over the title screen or the ended game, the panel, ranks, high scores, the saved game; `GAME_NOTES.md` section 8),
the high score entry and tables, the options, quit, new-game and level-up boxes, the
registry values (hints shown, hints off, colour-blind symbols, music and sound volumes), a bot and its suite.

Where the port is not the original's exact picture (each marked in the code):

- the atoms drawn from the pre-rotated glow sprites sit a pixel up and left of the original's in some pictures, and the edges of the
  atom symbol icons differ by a few levels (`IDENTICAL_PLAY.md`, phase two, oddities); the geiger needle and the title logo's waves
  were brought to the original's (the logo's waves match for the same count of updates, but the original's longer load makes the
  two screens show other phases at the same frame);
- the swirls' triangles go through `Image::FillScanLines` instead of the original's raw writes into the display buffer (the same
  spans, the same colours; the original's rounding of a colour to the display's bit depth is not repeated);
- the "More games" and "Register" buttons and every link to a web page are left out (the port has no online features).

## Where the work was hard

- **Fields hidden in the Meter.** The board's level, its molecule count, the meter's value and its "full" flag
  (board +0xc8 to +0xec) are the fields of the Meter object at +0xc4: `Meter::Init` starts the level at 1. The first reading took them for
  separate board fields.
- **The decompiler drops terms.** `cellState[col][row]` came out as `cellState[row]` in the sparkle that lights the field up (the
  disassembly has `lea edx, [ecx + 8*eax]`); the first port revealed only the first column. Check every array index and every
  draw call's trailing arguments in the disassembly.
- **`b1e` and `b1f` of a 2003 widget are `mIsDown` and `mIsOver`.** The grid deselects its atom on a mouse release when the mouse is
  *not over* the field, so a bot that calls the handlers directly must set `mIsOver`.
- **The board draws the field.** The Grid is a widget (it gets the mouse and the updates) but the board draws it in the
  middle of its own picture, below the machine's rim; the port's `Grid::Draw` draws nothing and `Grid::DrawField` is what the board calls.
- **Flags named by what they gate.** `+0x4c1` of the grid is "this radiation round can no longer be solved" (the text is
  drawn while it counts up), not a bonus; `+0x33c` is the reveal being finished.
- **Strings that are not in the string table.** The combo names, the signs' texts and the atoms' hover texts are literals in the code;
  they are written in the port's code (a few words each). The level names are read from the exe through the table at 0x4cbc58.

## Changes outside the Atomica folders

- `CMakeLists.txt`: the `BUILD_ATOMICA` option (a separate `shared:` commit).
- `tools/check.sh`: the `atomica` suite (a separate `shared:` commit).
- `.claude/launch.json`: `atomica-web` on port 18772.
