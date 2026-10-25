# Seven Seas Deluxe port

Seven Seas Deluxe (PopCap Games, 2003) is decompiled and ported the way Noah's Ark Deluxe was (see
`games/noahsark/docs/README.md`, the model for this port): the original's game classes are translated function by
function onto the portable SexyAppFramework (`framework/`, SDL2 and Emscripten), and the game data is copied from the
user's own install. Nothing from the game itself is in the repository: not its images, sounds, music or fonts, and not
its texts either (they are read from the player's `WinSS.exe` at run time).

| Document | Contents |
|---|---|
| [GAME_NOTES.md](GAME_NOTES.md) | Behaviour notes of the game code: binary layout, the classes, the board's data and rules |
| [FRAMEWORK_2003.md](FRAMEWORK_2003.md) | The 2003 SexyApp framework the game was built on, and how the port maps it |
| [MOD_IDEAS.md](MOD_IDEAS.md) | The mods worth making (none are made: the order of work forbids it until every PopCap game works in the browser) |
| `symbols/*.jsonl` | Names for Ghidra (`ApplySymbols.java`): the framework's core classes and string helpers |

## The binary

The retail install (`Program Files\PopCap Games\Seven Seas Deluxe`, version 1.1) has a plain `WinSS.exe`: no wrapper, no
copy protection. It was linked on 2003-01-29 with Visual C++ 6.0 (image base 0x400000), 596 KB of code, 3,085 functions
once the missing ones were found. The game folder holds `images` (GIF, JPG and PNG), `data` (font descriptors and
`anims.txt`), `sounds` (23 Ogg Vorbis files) and `music` (one Impulse Tracker module, `game.it`, that the original plays
through FMOD and the port through the platform's mixer).

Unlike Noah's Ark, the exe has RTTI only for the 50 framework and library classes: the game's own 17 classes were
compiled without it. Their vtables came from the constructors (a store of an address in .rdata into `[this]`) and the
classes were named by what their functions do. Ghidra also missed about 125 of the game's functions, the virtual ones,
which are reached only through those vtables; `tools/gapfuncs.py` finds them and `ApplySymbols.java` creates them.

## Reproducing the analysis

Ghidra 12.1.4 and JDK 21 are under `E:\retools`. The commands are Noah's Ark's with this game's paths; long runs go
under `build/native/rundesk.exe`.

```bash
mkdir -p re/sevenseas/bin && cp "<install folder>/WinSS.exe" re/sevenseas/bin/
python tools/rtti_msvc70.py re/sevenseas/bin/WinSS.exe re/sevenseas/rtti
SYMBOLS=re/sevenseas/rtti.jsonl build/native/rundesk.exe bash tools/ghidra_import.sh re/sevenseas/bin/WinSS.exe sevenseas re/sevenseas/gh
python games/sevenseas/tools/gapfuncs.py                  # the functions Ghidra missed
GHIDRA_PROJECT=sevenseas GHIDRA_PROGRAM=WinSS.exe EXPORT_DIR="$(pwd -W)/re/sevenseas/gh" build/native/rundesk.exe bash tools/ghidra_headless.sh apply re/sevenseas/gapfuncs.jsonl games/sevenseas/docs/symbols/*.jsonl
python games/sevenseas/tools/decread.py 401580            # one function, readable, with the game's strings inlined
python games/sevenseas/tools/make_gametext.py             # the table of text addresses (src/GameTextTable.inc)
```

`re/` and `assets/` are git-ignored: the exe, Ghidra's export and the staged files contain the game's code, strings and
art. `tools/funcmatch.py` against Noah's Ark's export named 210 framework and string-helper functions (the symbol file
here); the game's own classes matched nothing, as expected. Nothing was learned by running the original.

## Building and running

```bash
python games/sevenseas/tools/stage_sevenseas_assets.py   # once: copies the game's files to assets/sevenseas/
TARGET=sevenseas tools/build.sh native                   # build/native/sevenseas.exe
tools/build.sh web                                       # build/web/sevenseas.html/.js/.wasm (with the other games)
```

Native: `sevenseas.exe` finds `assets/sevenseas/` next to itself (or up to two levels up), or the retail install;
`-gamedir=` overrides that. Saves go to `%APPDATA%\wasm-bejeweled\Seven Seas Deluxe\`: `savegame.dat` for the game in
progress, `hiscores.dat` (the original's layout and check values) and `registry.ini` for the options. Web:
`sevenseas.html`, the shared page (`web/shell.html`) with this game's settings (`games/sevenseas/web/page.js`), imports
the player's game folder (or a .zip of it) into its own IndexedDB-backed folder `/sspersist`.

Test runs follow `CLAUDE.md`: native runs only under `build/native/rundesk.exe`, web pages only with `?silent=1`.

## Testing

`tools/check.sh sevenseas` runs the bot three times (a beginner's game from level 1, a game that starts on the bonus
screen at level 10 and one that starts at level 21) and a rerun of the first, and compares the logs and the screenshots
of the two runs (about 6 seconds). The bot is `src/testhooks/Autoplay.cpp`, started with `-autoplay=<seed>`
`-autoplaydiff=1|2|3 -autoplaylevel=N -autoplaygames=N -autoplaycap=<updates> -autoplaylog=<file>
-autoplayshots=<prefix>`. It has its own random generator, so a run depends only on `-seed` and its arguments, and the
suite's expected results (the last game-over line of each run) are in `tools/check.sh`. `-script=<file>` drives the menus
by update number (`<update> click|screenshot|quit ...`); `games/sevenseas/tools/run_script.sh` runs one under rundesk.
A debug build with AddressSanitizer (llvm-mingw has the i386 runtime; set `ASAN_SYMBOLIZER_PATH` to its
`llvm-symbolizer`) is the way to find heap errors: configure a second build folder with `-fsanitize=address -g` and run
the bot and the scripts in it. The bot's games, the tutorial, the bonus screens and the map were run that way: no reports.

## Status (2026-10-25)

The game plays natively and in the browser from the title screen through the menu (difficulty, tutorial), the tutorial's
fourteen steps, the beginner's hint box, all the kinds of ship (pirates, red and blue pirates, ghost ships, the sea
monster) with their warnings, whirlpools, the bonus screens (digging for treasure), the map every ten levels, extra lives,
undo, game over, the high-score list with its name entry, and save and Continue.

Not done or approximate:
- The beginner's demo screen (`WinSS.exe` 0x4132b0, a self-playing board with a "click here to start" button, shown before
  a beginner's first game) is not ported: a beginner's game starts on the board, with the original's hint box over it.
- Options → Help (0x40d330, the "return to game" help pages) does nothing; the registration and "submit to the Internet
  High Score List" dialogs, the trial's loading delay and the shareware counters are not ported (the port plays as the
  registered game). The internet score check's object (the +0x6c field of the board) is not either.
- The pirate's mouth (`TalkWidget`) moves through random frames while a line plays; the original has a table of frames and
  times per line. The skull's pupils follow the mouse with an estimated formula, and the text lines of the bonus screen's
  messages are placed by estimate. Nothing could be run to compare with the original, so these are the places where the port
  may differ on screen.
- The high-score screen (0x40ff70 constructor, 0x412af0 update, 0x411260 draw, 0x412cc0 insert) follows the original's
  positions, colours and the slide-in from the top, and says "You got ... place" in the name dialog after it (0x410ae0).
  The original's second button, "Submit to Internet High Score List", and the timer that re-enables it are not ported.
- The order of the tutorial's own steps, the sound and the music orders (the module's start orders 0..0x1f select the music
  for the menu, the levels and the bonus screens) were taken from the code, not heard.

## Where the work was hard

The board's code is one 11 x 11 turn-based game, but its functions are the largest of the game (the level generator is
4 KB, the enemy planner 2.6 KB, the drawing 12 KB) and the decompiler shows their loops as `goto` webs; the placement
loops were rebuilt from their conditions and checked against the generated levels. The second cost was the missing RTTI:
every game class had to be found from its constructor, and about 125 virtual functions had to be created in Ghidra
before the board could be read at all. Two bugs were found by running, not by reading: the app's destructor deleted
widgets that the widget manager still held (an AddressSanitizer report at shutdown), and the board's water scroll was never
initialised (the bot's rerun test differed). The decompiler also lost the stack structures of the anims and the ports in the
tutorial setup; the assembly (`llvm-objdump -d`) gave their fields.

## Changes outside the Seven Seas folders

Each is its own `shared:` commit: `CMakeLists.txt` (the `BUILD_SEVENSEAS` option), `tools/check.sh` (the `sevenseas` suite,
also in the `games` set) and `.claude/launch.json` (the `sevenseas-web` preview server on port 18771).
