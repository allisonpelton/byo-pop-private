# Mummy Maze Deluxe port

Mummy Maze Deluxe (PopCap Games, 2002) is decompiled and ported the way Noah's Ark Deluxe and Seven Seas Deluxe were (see
`games/noahsark/docs/README.md` and `games/sevenseas/docs/README.md`, the models for this port): the original's game
classes are translated function by function onto the portable SexyAppFramework (`framework/`, SDL2 and Emscripten), and
the game data is copied from the user's own install. Nothing from the game itself is in the repository: not its images,
sounds, music or fonts, and not its texts either (they are read from the player's `WinMM.exe` at run time, by address).

| Document | Contents |
|---|---|
| [GAME_NOTES.md](GAME_NOTES.md) | Behaviour notes of the game code: binary layout, the classes, the board's data and rules |
| [MOD_IDEAS.md](MOD_IDEAS.md) | The mods worth making (none are made: the order of work forbids it until every PopCap game works in the browser) |
| `symbols/*.jsonl` | Names for Ghidra (`ApplySymbols.java`): the framework's core classes and string helpers |

## The binary

The retail install (`Program Files\PopCap Games\Mummy Maze Deluxe`, version 1.1) has a plain `WinMM.exe`: no wrapper, no
copy protection. It was linked on 2002-06-21 with Visual C++ 6.0 (image base 0x400000); the game's own code is the 424
functions from 0x401000 to 0x42d5b0. The game folder holds `images` (237 GIF, 30 JPG and 5 PNG), `data` (five font
descriptors and the 101 maze files `mazes/B-<n>.dat`), `sounds` (24 Ogg Vorbis files and one MP3) and `music` (one Impulse
Tracker module, `game.it`, that the original plays through FMOD and the port through the platform's mixer).

The exe has RTTI only for the 46 framework and library classes: the game's own classes were found from their
constructors (a store of an address in .rdata into `[this]`) and named by what their functions do. Ghidra also missed
about 120 of the game's functions, the virtual ones, which are reached only through those vtables;
`tools/gapfuncs.py` finds them.

## Reproducing the analysis

Ghidra 12.1.4 and JDK 21 are under `E:\retools`. The commands are Seven Seas' with this game's paths; long runs go under
`build/native/rundesk.exe`.

```bash
mkdir -p re/mummymaze/bin && cp "<install folder>/WinMM.exe" re/mummymaze/bin/
python tools/rtti_msvc70.py re/mummymaze/bin/WinMM.exe re/mummymaze/rtti
SYMBOLS=re/mummymaze/rtti.jsonl build/native/rundesk.exe bash tools/ghidra_import.sh re/mummymaze/bin/WinMM.exe mummymaze re/mummymaze/gh
python games/mummymaze/tools/gapfuncs.py                  # the functions Ghidra missed
python games/mummymaze/tools/decread.py 4067f0            # one function, readable, with the game's strings inlined
games/mummymaze/tools/rd.sh 4067f0 406f10                 # the same, shorter (needs the export)
python games/mummymaze/tools/make_gametext.py             # the table of text addresses (src/GameTextTable.inc)
python games/mummymaze/tools/make_res.py                  # the picture and sound ids (src/Res.h, Res.cpp)
python games/mummymaze/tools/make_tables.py               # the numeric tables of the data section (src/Tables.inc)
python games/mummymaze/tools/make_tutorial.py             # the tutorial's five mazes and its step table (src/TutorialMazes.inc)
```

`re/` and `assets/` are git-ignored: the exe, Ghidra's export and the staged files contain the game's code, strings and
art. The generators' outputs (`GameTextTable.inc`, `Res.h`, `Tables.inc`, `TutorialMazes.inc`) hold addresses, names and
numbers only. Nothing was learned by running the original.

## Building and running

```bash
python games/mummymaze/tools/stage_mummymaze_assets.py   # once: copies the game's files to assets/mummymaze/
TARGET=mummymaze tools/build.sh native                   # build/native/mummymaze.exe
tools/build.sh web                                       # build/web/mummymaze.html/.js/.wasm (with the other games)
```

Native: `mummymaze.exe` finds `assets/mummymaze/` next to itself (or up to two levels up), or the retail install;
`-gamedir=` overrides that. Saves go to `%APPDATA%\byo-pop\Mummy Maze Deluxe\`: `classic.sav` for the classic game
in progress, `adv<n>.sav` for the five adventure slots (the original's layout, format 2), `hiscores.dat` (the original's
layout and check values) and `registry.ini` for the options. `-importsaves[=<install folder>]` brings in the saves of the original game installed on this computer (`docs/WEB_PLAYER.md`, "Importing saves from an installed game"). Web: `mummymaze.html`, the shared page (`web/shell.html`) with
this game's settings (`games/mummymaze/web/page.js`), imports the player's game folder (or a .zip of it) into its own
IndexedDB-backed folder `/mmpersist`. The preview server for it is `mummymaze-web` in `.claude/launch.json` (port 18773).

Test runs follow `CLAUDE.md`: native runs only under `build/native/rundesk.exe`, web pages only with `?silent=1`.

## Testing

`tools/check.sh mummymaze` plays the game with a bot (about a minute): a classic game of 8 chambers, with the name box,
the options and their dialogs photographed, and a rerun whose log and screenshots must be identical; a game that starts at
pyramid 3, where the tips come; a whole classic game (225 chambers to the victory screen); a whole adventure (new game,
the world map, 15 pyramids, the saved game, to the victory screen); and the tutorial. The bot is
`src/testhooks/Autoplay.cpp`, started with `-autoplay=<seed>` and `-autoplaymode=tutorial|adventure`
`-autoplaygames=N -autoplaylevels=N -autoplaystart=<pyramid>,<chamber> -autoplaycap=<updates> -autoplaylog=<file>
-autoplayshots=<prefix>`. It plays each chamber either with the game's own solver (the one behind Abandon Hope and the
Ankh, `Board::Solve`, pressed as keys) or as a short random walk, undo included, so that the mummies catch it now and then;
it has its own random generator, and in its runs the game's time is counted in updates (`MummyApp::GameTick`), so a run
depends only on its arguments. A fight between the two mummies stops the explorer for a while; the bot waits for it, and
when the board is not where the search said it would be after a move (the original's search leaves out that a mummy that
wins such a fight on the key stone flips the gate) it searches again from where the explorer stands.

## Status

Everything in the game is ported and plays natively and in the browser: the title, the main menu, the classic game (225
chambers in 15 pyramids, the score, the ranks, the saved game that Continue loads, the high scores with their name box), the
tutorial, the adventure (five saved games, the world map with its walking marker, the pyramids that open as others are
solved, the treasures, the statistics, the last pyramid and the victory screen), the options and the tips. The original's
online parts are not: the shareware nags and counters (the port plays as the registered game), the registration with the Options
dialog's Register button, its button to the game's web page, and "Submit to Internet High Score List".

Not done or approximate:
- The one MP3, `sounds/pummel.mp3` (the sound of the fight between the mummies), is silent: the platform's mixer has no MP3
  decoder yet. `third_party/libopenmpt/include/minimp3` is in the repository; the coordinator was sent the spec.
- The text of the tutorial's boxes is drawn where the decompiled code puts it, but its right end lies under the box's MORE
  button, as it does in the code (the box is 109 pixels high on the 6 x 6 board). Nothing could be run to compare it with the
  original.
- The world map's three corners (statistics, the player's name, the treasure chest) are invisible buttons here; in the
  original they carry a copy of the map's picture under them, which draws the same pixels.
- The slot buttons' text colour and the exact place of the name dialogs were taken from the code, not compared with a screen.

## Where the work was hard

The decompiler. Its output for the game's functions is full of structured-exception bookkeeping (`ExceptionList = ...`), and
a first reading helper removed every line that mentioned it, which also removed conditions written as a comma expression
(`if (a || (ExceptionList = &x, b))`); several functions were read twice because of that. It also drops the loop
increments of `do { } while` loops that compare through a pointer, the arguments of many calls (the Graphics' source
rectangles of `DrawImageSrc`, the colours of `SetColor`) and all the x87 arithmetic of the draw functions; those came from
the assembly (`llvm-objdump -d`), sometimes call by call. The second cost was the missing RTTI, and the third a swapped
pair: the original's `BringToBack` (a widget moved to the front of the list) was first read as bringing the board to the
front, which hid every dialog that the board's constructor made (the tips). Running found three bugs that reading had not:
the side panel and the check boxes never freed the pictures they cut from the sheets (350 KB per chamber, which
AddressSanitizer's allocator ran out of in the middle of the second game), the bot's keys that a fighting mummy ignored,
and the original's own search that is wrong when a mummy wins a fight on the key stone.

## Changes outside the Mummy Maze folders

Each is its own `shared:` commit: `CMakeLists.txt` (the `BUILD_MUMMYMAZE` option), `tools/check.sh` (the `mummymaze` suite,
also in the `games` set) and `.claude/launch.json` (the `mummymaze-web` preview server on port 18773).
