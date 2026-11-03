# Noah's Ark Deluxe port

Noah's Ark Deluxe (PopCap Games, 2002) is decompiled and ported the way Bejeweled Deluxe, Big Money Deluxe and
BookWorm Deluxe were: the original's game classes are translated function by function onto the portable
SexyAppFramework (`framework/`, SDL2 and Emscripten), and the game data is copied from the user's own install.
Nothing from the game itself is in the repository: not its images, sounds, music or fonts, and not its texts either
(they are read from the player's `WinNoah.exe` at run time).

| Document | Contents |
|---|---|
| [GAME_NOTES.md](GAME_NOTES.md) | Behaviour notes of the game code: binary layout, resources, the board's data and rules |
| [FRAMEWORK_2002.md](FRAMEWORK_2002.md) | The 2002 SexyApp framework the game was built on, and how the port maps it |
| [IDENTICAL_PLAY.md](IDENTICAL_PLAY.md) | The identical-play pass against the original: how it was compared, the changes, the oddities left |
| [MOD_IDEAS.md](MOD_IDEAS.md) | The brainstorm for this game's mods, what was made and how each mod works |
| `symbols/*.jsonl` | Names for Ghidra (`ApplySymbols.java`): the framework's core classes |

## The binary

The retail install (`Program Files\PopCap Games\Noah's Ark Deluxe`, version 1.1) has a plain `WinNoah.exe`: no
wrapper, no copy protection. It was linked on 2002-10-17 with Visual C++ 6.0 (image base 0x400000) and has full RTTI:
64 classes, 2,709 functions, of which 260 (0x401000 to 0x421dc0, 131 KB) are the game's own. The game folder holds
`images` (GIF, JPG and PNG), `data` (font descriptors), `sounds` (Ogg Vorbis and one `.au`) and `music` (four Ogg
files, played by FMOD in the original).

## Reproducing the analysis

Ghidra 12.1.4 and JDK 21 are under `E:\retools`. The commands are those of BookWorm's analysis (see
`games/bookworm/docs/README.md`) with this game's paths; long runs go under `build/native/rundesk.exe`.

```bash
mkdir -p re/noahsark/bin && cp "<install folder>/WinNoah.exe" re/noahsark/bin/
python tools/rtti_msvc70.py re/noahsark/bin/WinNoah.exe re/noahsark/rtti
SYMBOLS=re/noahsark/rtti.jsonl build/native/rundesk.exe bash tools/ghidra_import.sh re/noahsark/bin/WinNoah.exe noahsark re/noahsark/gh
python tools/rtti_msvc70.py re/noahsark/bin/WinNoah.exe re/noahsark/rtti --functions re/noahsark/gh/functions.jsonl
GHIDRA_PROJECT=noahsark GHIDRA_PROGRAM=WinNoah.exe EXPORT_DIR="$(pwd -W)/re/noahsark/gh" build/native/rundesk.exe bash tools/ghidra_headless.sh apply games/noahsark/docs/symbols/*.jsonl
python games/noahsark/tools/decread.py 4092a0            # one function, readable, with the game's strings inlined
python games/noahsark/tools/make_gametext.py             # the table of text addresses (src/GameTextTable.inc)
```

`re/` and `assets/` are git-ignored: the exe, Ghidra's export and the staged files contain the game's code, strings and art.
The 64 classes and their vtables came from the RTTI scan, and the framework's core classes were named in
`symbols/10_framework_core.jsonl` (the 2002 framework is a different generation from the 2004 one that the other games'
symbol files carry over: see FRAMEWORK_2002.md). The game functions were then read one class at a time with `decread.py`,
and the notes in GAME_NOTES.md and the port's classes were written from that reading. Nothing was learned by running
the original.

## Building and running

```bash
python games/noahsark/tools/stage_noahsark_assets.py   # once: copies the game's files to assets/noahsark/
TARGET=noahsark tools/build.sh native                  # build/native/noahsark.exe
tools/build.sh web                                     # build/web/noahsark.html/.js/.wasm (with the other games)
```

Native: `noahsark.exe` finds `assets/noahsark/` next to itself (or up to two levels up), or the retail install;
`-gamedir=` overrides that. Saves go to `%APPDATA%\byo-pop\Noahs Ark Deluxe\`: `savegame.dat` for the game in
progress, `hiscores.dat` (the original's layout) and `registry.ini` for the options. `-importsaves[=<install folder>]` brings in the saves of the original game installed on this computer (`docs/WEB_PLAYER.md`, "Importing saves from an installed game"). Web: `noahsark.html`, every game's
page (`web/shell.html`) with this game's settings (`games/noahsark/web/page.js`), imports the player's game folder (or a
.zip of it) into its own IndexedDB-backed folder `/napersist`, separate from the other games', and saves as the other pages do.

Test runs follow `CLAUDE.md`: native runs only under `build/native/rundesk.exe`, web pages only with `?silent=1`.

## Testing

`tools/check.sh noahsark` runs the bot in all three modes and one mode again, and compares the screenshots of the
two runs, then the mods' tests and the high-score loader's test (`tools/run_hiscores_test.sh`: tampered, NUL-named and cut
`hiscores.dat` files made by `tools/make_hiscores.py`), about 30 seconds in all. The bot is `src/testhooks/Autoplay.cpp`, started with `-autoplay=<seed>`
`-autoplaymode=action|strategy|puzzle -autoplaydiff=0|4|8 -autoplaygames=N -autoplaycap=<updates>
-autoplaylog=<file> -autoplayshots=<prefix>`. It has its own random generator, so a run depends only on `-seed` and its
arguments: the same run gives the same log and the same screenshots, and the suite's expected results (the last level or
game-over line of each run) are in `tools/check.sh`. `-script=<file>` drives the menus by update number
(`<update> click|screenshot|quit ...`); `games/noahsark/tools/run_script.sh` runs one under rundesk. A debug
build with AddressSanitizer (llvm-mingw has the i386 runtime) is the way to find heap errors: configure a second build
folder with `-fsanitize=address -g` and run the bot in it.

The bot's log has a "pair joined at update N" line whenever animals start to walk. `tools/run_walk_shots.sh <dir> [N]`
takes screenshots of the first walking pair of the action bot (seed 1) a few updates apart, to compare two builds.
The rain animates on every frame, so whole-picture diffs always differ; compare the board region (x 178 to 628, y 16 to
466), where only the animals and the flood move. The game's drawing is not changed for this.

## Mods

Four mods are in `games/noahsark/mods/` (the shared mod system, docs/MOD_MENU.md; F2 opens the mod menu in the game):
`undo` (Ctrl+Z or U takes back the last move), `helper` (H shows a move, H again plays it), `tide` (the flood's pace as a
percentage, 0 to 400) and `daily-board` (every level starts from the same animals for everyone on a day). Natively copy a
folder into `build/native/mods-noahsark/` or pass `-modsdir=`; on the web give the Mods panel the folder or a .zip. A mod
set that changes play gives the games played with it their own high scores and saved game (`hiscores-<key>.dat`,
`savegame-<key>.dat`). MOD_IDEAS.md has the brainstorm and the design of each. `tools/check.sh noahsark` tests them
(`games/noahsark/tools/run_mod_tests.sh`).

## Status (2026-10-25)

The game plays natively and in the browser from the title screen through the menu, the difficulty dialog, the tutorial,
all three modes (action with its stampede bonus levels, strategy, puzzle), the level-up screens, game over and the
high-score entry, with save and Continue. The bot plays action to level 15 (where its cap stops it), strategy to level 9 and puzzle
to its game over, and none of those runs shows an AddressSanitizer report. Not done: a close comparison against the
original's play on screen (nothing could be run to compare with), and the registration screen: the port plays as the
registered game (the title shows "Thanks for registering") and has no trial limit or order dialog.

Milestones (2026-10-25, this machine's clock): the session started about 06:28; Ghidra's import and export were done at
06:31; the title screen, the menu and the text reader were committed at 07:25; the whole game with its bot and the check suite
at 07:41; the web page and a played web game at 07:45; the four mods, their tests and a web run of two of them at 08:00.

## Where the work was hard

The first stretch was cheap: RTTI, the Ghidra export and the 260 game functions were readable enough that the app, the
title screen and the menu came up in about an hour. The board took the rest. A few things took several tries or were redone.

Reading a decompiled branch tree wrongly cost the most. The board's `Draw` chooses Noah's picture with a goto tree over
two flags, and the first translation swapped what the flags meant, so after a won level it indexed Noah's sprite
table past its end. That showed up as a crash some thousand updates later, after the stampede bonus level, with no pointer
to the drawing code. I spent a long stretch auditing the game logic (the path trail, the group removal, the pieces' path
vectors) for heap corruption that did not exist. An AddressSanitizer build found the line in one run. The lesson for
the next game: build the sanitizer configuration first, when the first crash that is not at its cause appears.

The first versions of the tutorial and name-entry dialogs were stand-ins that looked reasonable: the tutorial's picture
stepped through a counter, and the name box was a plain white field. Neither drew the picture that the original puts
behind it (images 82 and 77), which the repository's sweep for pictures a player cannot reach found on 2026-10-26;
translating the two functions properly also changed the dialogs' constructors (heading, button text and mode, colours),
positions and the animation (FRAMEWORK_2002.md, Dialogs). An image id that no draw call of the port uses is worth a
look in any game's port.

Smaller redos: the strings' addresses had to be taken from the string list, not from nearby references (two wrong tables);
the crate and block piece types were named the wrong way round; the checkbox image halves and the tutorial dialog's layout
were off; the bot stalled on a stale pixel offset of empty pieces; writing files with backslashes through a shell
heredoc corrupted them, so those were written with the editor tools; and a Windows crash filter conflicted with the framework's
typedefs and was dropped. The puzzle and strategy modes were translated before they could be run, so their bot results
(puzzle ends in a game over on level 1 after about 120 moves) say that they run, not that they play the way the original does.

## Changes outside the Noah's Ark folders

Each is its own `shared:` commit: `CMakeLists.txt` (the `BUILD_NOAHSARK` option), `tools/check.sh` (the `noahsark`
suite, also in the `games` set) and `.claude/launch.json` (the `noahsark-web` preview server on port 18769).
