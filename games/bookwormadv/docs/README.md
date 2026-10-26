# Bookworm Adventures Deluxe port

A port of Bookworm Adventures Deluxe (PopCap Games, 2006; the exe linked 2007-06-20) to the shared framework and
platform layer: it plays natively (Windows, SDL2) and in the browser (Emscripten). The game code is a translation of
`BookwormAdventures.exe`, found by decompiling it with Ghidra; the game's files (`main.pak`, the loose cut scenes and
properties, and the exe, whose texts are read at run time) come from the player's own copy and are never in the
repository.

Much of the game is not in the exe's C++: its rules, its battles and its books are 621 Lua scripts, compiled for
PopCap's own fork of Lua 5.0 (PopLua), and its creatures and effects are PopAnim animations (`.pam`); the cut scenes
are Flash movies. The port therefore has, beside the game's C++ classes in `src/`, a PopLua virtual machine and
compiler (`poplua/`, `docs/POPLUA.md`), the layer that links C++ objects to Lua (`lualib/`, `luaapp/`), JeffLib, the
library of helpers in the exe that the game's classes use (`jefflib/`), a PopAnim player (`popanim/`) and a
Flash player for the cut scenes (`swf/`). `docs/ENGINE.md` maps the exe's regions to these folders and explains how
C++ objects appear in Lua.

## The binary

`BookwormAdventures.exe` is linked by Visual Studio 2005 on SexyAppFramework 1.3 or close to it, with RTTI for the
game's classes. The game's C++ is 0x401000-0x4c8000, the framework 0x4c8000-0x5a0000, LuaApp 0x650000-0x6a0000,
JeffLib 0x6a0000-0x6c9a00, LuaLib 0x6c9a00-0x6ee000 and PopLua 0x6ee000-0x71f000 (`docs/ENGINE.md`, "Code
regions"). The scripts call C++ methods through 358 kinds of method wrappers; `tools/bindings.py` lists the bound
methods from the exe into `re/bookwormadv/bindings.txt`, and `src/GameAppLua.cpp` binds the port's under the same
names.

## Reproducing the analysis

The export lives in `re/bookwormadv/` (git-ignored, like everything derived from the game).

```
python games/bookwormadv/tools/decread.py 44ad10       # a function of the export, readable
python games/bookwormadv/tools/asm.py 0x44ad10         # its disassembly
python games/bookwormadv/tools/fnfind.py -r 0xA 0xB    # the functions in a range (or by string or namespace)
python games/bookwormadv/tools/regtables.py            # the luaL_reg tables of the Lua libraries
python games/bookwormadv/tools/luc.py dis <file.luc>   # a compiled script, disassembled (also: names, stats)
python games/bookwormadv/tools/gametext.py             # src/GameTextTable.inc: the texts read from the exe
```

The decompiler drops x87 operands and some stack arguments, so floating point code and some calls were read in the
disassembly. Every ported function names its address in a comment (`// 0x...`).

## Building and running

```
tools/setup_worktree.sh                                                    # a new worktree: the git-ignored files
python games/bookwormadv/tools/stage_bookwormadv_assets.py "<the game's folder>"   # stages assets/bookwormadv/
cmake --build build/native --target bookwormadv                            # build/native/bookwormadv.exe
build/native/rundesk.exe build/native/bookwormadv.exe -userdir=<dir>       # a game, invisible and silent as tests run it
```

The staging unpacks `main.pak` (`tools/popcap_pak.py`) and names every file in lower case. The game's 42 JPEG 2000
pictures are decoded at run time by the platform's OpenJPEG; its music, `sounds/music.mo3`, is one tracker module in
which each song starts at an order, played by the platform's libopenmpt player and queued by JeffLib's song player
(`jefflib/SongPlayer.cpp`).

For the browser, configure `build/web` with `emcmake cmake` as `tools/build.sh` does and run
`cmake --build build/web --target bookwormadv`. Serve `build/web` (`.claude/launch.json` has `bookwormadv-web` on port
18775) and open `bookwormadv.html`; in the Browser pane always with `?silent=1`. The page imports the game from its
install folder or a zip of it: `main.pak` (unpacked into the game's folders), `BookwormAdventures.exe` and the
`images` and `properties` folders, into its own IndexedDB folder (`/bwapersist`), where the profiles are kept too.

Native options: `-gamedir=<dir>` (else `assets/bookwormadv` next to the exe or up to two levels up), `-userdir=<dir>`
(the profiles and `registry.ini`), `-modsdir=<dir>` and `-nomods`, and the framework's test hooks `-fast`, `-seed=N`,
`-trace` and `-script=<file>`. Scripts take the framework's commands (`<update> move <x> <y>`, `click <x> <y>`,
`key <name>`, `screenshot <file>`, `quit`) and two of the game's own (`src/TestHooks.cpp`): `word [attack]` spells the
longest word the battle's grid allows (and attacks with it), and `lua <chunk>` runs a line of Lua in the game's state,
for example `lua profile.Set('UnlockedArena', true)` once the main menu is up (about update 950).

## Testing

`tools/check.sh bookwormadv` (about two minutes) runs the PopLua compiler's dialect checks, loads every compiled
script of the game with the port's VM, every PopAnim animation, and four Flash cut scenes to their end, then plays:
from the title to the main menu, whose quit asks (Play! stays, Quit quits), and the test profile's first chapter,
spelling with `word attack` until it is won and saved. The unit suite
(`tools/check.sh unit`) checks the JPEG 2000 decoder against Pillow's pictures in `re/bookwormadv/jp2ref` and the
pak reader against the staged files.

What the suite does not reach was checked with scripts: the arena from the main menu (its treasure room, the tutorial
and a timed fight), the hall of fame, the tome and its media player, the credits from the options, the players dialog
(choosing, adding, deleting, a name in use), the mini-games' menu and its three games. In the browser: the import,
the main menu, the map, a battle, the credits, the players dialog, the mini-games' menu, a game of Word Master left
for the hall of fame, the tome, and the window losing and regaining the focus.

## Mods

None (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones worth
making.

## Status (2026-10-26)

The adventure, the arena, the mini-games, the hall of fame, the tome, the credits and the players dialog play
natively and in the browser (where the arena was not reached, as only a script command unlocks it for a test
profile). Tiles and letters are chosen by clicking, as in the original: no widget of the exe chooses them by typed letters
(the board's keys are only 1 to 4, Enter and space; the scripts' KeyChar handlers are empty). Left out: the media player's music visualiser (it needs the music's spectrum, which the
platform's player does not give yet), the trial and its buy screens (the port plays as bought), and the developers'
tools in the exe (the DProps editor, the Lua debugger console). Notes starting `port:` in the code mark every place
where the port differs from the original.

## Where the work was hard

PopLua is Lua 5.0.2 with PopCap's changes, read from the exe since nothing documents them: its arrays and its table
constructors start at 0, `string.sub` takes a start and a length, and it has a class dialect of its own
(`docs/POPLUA.md`). The port's VM runs the game's compiled scripts as they are, and its compiler takes source in the
same dialect (the suite's 150 dialect checks), which is what script mods would need.

## Changes outside the Bookworm Adventures folders

`CMakeLists.txt` (the `BUILD_BOOKWORMADV` option), `tools/check.sh` (the `bookwormadv` suite and the unit suite's
jp2test), `.claude/launch.json` (the `bookwormadv-web` preview on port 18775) and the platform's JPEG 2000 images
(`platform/openjpeg`, registered with the framework's image loader).
