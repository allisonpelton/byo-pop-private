# Atomica Deluxe port

A port of PopCap's Atomica Deluxe (version 2.52, 2003-03-28) to the shared SexyAppFramework port: it plays natively
(Windows, SDL2) and in the browser (Emscripten). The game code is a translation of the original's game classes, found
by decompiling `Atomica.exe` with Ghidra; the game's files (pictures, sounds, the module music and the texts, which
are read from the exe at run time) come from the player's own copy and are never in the repository.

The strategy and the action modes play from the first level to the game over. The menu, the dialogs, the hint boxes,
the high scores, the bonus molecules and the five elements are not ported yet (see Status).

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

Ported, each function with a `// 0x<addr>` comment:

- the loading (pictures, the 25 shrunk copies and the rotating masks of each atom, sounds), the title screen (without the logo's shimmer);
- the Grid: input, update and drawing, the molecule search, the move that follows each of the player's, the magnet, all
  the levels of the strategy mode (41 set-ups), the bonus levels, the action mode's set-up and drain, the radiation
  rounds, the saved game;
- the Atom (placing, paths, pre-atoms, the biohazard's spread, chromium), the path finder (0x4246a0-0x425110, a translation
  of its recursion and the arrows it draws), the effects, the Board (HUD, meter, the atomic bomb, the freeze and air counters,
  signs, floating scores) and Atomicus' voices (without the lip-sync data);
- a simplified main menu (strategy, action, the expert switches, the saved game, exit), the saved game (menu button), a bot and its suite.

Not ported yet (each is a follow-up):

- the main menu's rank, high score lists and the high score entry (0x41a010, 0x419760), and the options, help, level-up and
  quit dialogs (0x417b90, 0x421f90, 0x422f10, 0x420040): the level-up box is skipped, the Options and Help buttons do nothing;
- the hint boxes (`Grid::CheckHints` 0x40d450) and the board's help popups (0x42a310);
- the bonus molecules (0x414830-0x415d20): shapes of coloured atoms that give a bonus, and with them the five elements'
  atoms (0x4043d0-0x405700: fire, water, earth, air, ether; their effects, pictures and the freeze and ghost counters they use
  are in place, but nothing creates an element atom);
- the swirl objects (0x428100: the atoms that leave the field spiral into a vortex drawn by a small software rasteriser; the
  port flies each atom away by itself), the background clouds, the Atomicus monitor's scan lines and the geiger needle's exact
  geometry (the port draws the needle from the dial's middle at the angle the original computes);
- the registry values (options) and `HS.CFG`.

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
