# Bookworm Adventures Volume 2 port

A port of Bookworm Adventures Volume 2 (PopCap Games, 2009; `BookwormAdventuresVol2.exe` v1.0.4.8465, linked
2009-07-22) to the shared framework and platform layer. It is built on the engine that Bookworm Adventures Deluxe's
port made (`games/bookwormadv/`): this folder starts as a copy of that folder, with every target renamed (`bwa2_...`)
so that both ports build together, and is changed here where Volume 2's exe differs. The game's files (`main.pak`, the
loose music, cut scenes and properties, and the exe, whose texts are read at run time) come from the player's own copy
and are never in the repository. Volume 1's folder is never edited from here.

Most of the game is not in the exe's C++ but in 462 Lua scripts compiled for PopCap's fork of Lua 5.0 (PopLua), and in
304 PopAnim animations (`.pam`, versions 1 to 3) and 5 Flash cut scenes. The folders are Volume 1's: `poplua/` (the VM
and compiler, `POPLUA.md`), `lualib/` and `luaapp/` (the layer that links C++ objects to Lua), `jefflib/` (the
helper library the game's classes use), `popanim/`, `swf/` and, to be reworked for Volume 2, `src/` (the game's C++
classes). `ENGINE.md` maps the exe's regions to them; its addresses are Volume 1's.

## The binary

`BookwormAdventuresVol2.exe` is 4,146,504 bytes, with no launcher, wrapper, packer or SteamStub, and no Steam imports;
its PopCap DRM strings are inactive. Visual Studio 2005 on a 2008-09 build of SexyAppFramework 1.3, with RTTI. 16,596
functions, 3.03 MB of code. Where things are (survey 3.3): the game's C++ at 0x401000-0x490080, the generated
`Res.cpp` at 0x490080, XWidget and JeffLib around 0x4aaf80-0x4c0000, PopAnim at 0x510c00, the Lua binding glue at
0x5ffe00 and PopLua itself from about 0x659000 (`luaV_execute` is `FUN_0067b640`, 7,879 bytes).

Against Volume 1 (masked instruction streams, `tools/funcmatch.py`): 4,771 of Volume 2's 16,502 decompiled functions
have a counterpart there; of the 529 functions at 0x659000-0x680000, where PopLua is, 252 do.

The port's source began as Volume 1's, so the address comments in `src/` began as Volume 1's too. A plain `0x4xxxxx` in a
comment is now a Volume 2 address: carried over from Volume 1's by those pairs, the pairs of the vtable slots of classes both
games have, and the call lists of paired functions (the ones checked against the decompilation agree), or written for
Volume 2 in the first place. `V1:0x4xxxxx` is a Volume 1 address that has no counterpart that was found: do not look it up
in Volume 2's export. `docs/symbols/` holds the names that came over the same way (`tools/decread.py` and `tools/asm.py`
read them). A comment's address only says where to look in Volume 2's export; it does not promise that the function is a
line-for-line copy of it (`docs/PORT_PLAN.md` says which parts were read against Volume 2's decompilation).

Volume 2 drops Volume 1's `CursorWidget`, `TextWidget`, `MoreGamesAd`, `FModSoundInstance` and two JeffLib developer
dialogs, and adds `BookSelectScreen`, `EncodingParser`, `SecretListWidget` and `WinInetHTTPTransfer`. Its music is 10
OGG files (`music/`), so it needs no tracker module; its dictionary has an attribute file
(`data/compressed_attribs.dat`, one flag word per word); its animations include PopAnim versions 2 and 3.

## Reproducing the analysis

The export lives in `re/bookwormadv2/gh/` (git-ignored, like everything derived from the game): it was made with
`tools/ghidra_headless.sh export` from a copy of the survey's analysed project (`E:/retools/projects/survey_ba2`,
program `BookwormAdventuresVol2.exe`) as project `bookwormadv2_game`. The tools in `tools/` are Volume 1's, pointed at
`re/bookwormadv2/gh`:

```
python games/bookwormadv2/tools/decread.py 67b640       # a function of the export, readable
python games/bookwormadv2/tools/asm.py 0x67b640         # its disassembly
python games/bookwormadv2/tools/fnfind.py -r 0xA 0xB    # the functions in a range (or by string or namespace)
python games/bookwormadv2/tools/luc.py dis <file.luc>   # a compiled script, disassembled (also: names, stats)
```

(`asm.py`, `bindings.py`, `regtables.py` and `resgen.py` read the exe from `re/bookwormadv2/bin/`, a copy of the
Steam folder's.)

## Building and running

```
tools/setup_worktree.sh                                                    # a new worktree: the git-ignored files
python games/bookwormadv2/tools/stage_bookwormadv2_assets.py               # stages assets/bookwormadv2/ from the Steam folder
cmake --build build/native --target bwa2_luarun                            # PopLua, headless
build/native/bwa2_luarun.exe -gamedir=assets/bookwormadv2 -load -runall    # every script undumped, then run
```

The staging unpacks `main.pak` (`tools/popcap_pak.py`, XOR 0xF7), copies `images/`, `music/`, `properties/` and
`scripts/` (the loose `scripts/common`), names every file in lower case, and leaves out `images/upsell` (the buy
screen's trailer). The game's 34 JPEG 2000 pictures are decoded at run time by the platform's OpenJPEG.

The game's own exe target, `bookwormadv2`, is part of every configure (it was behind `-DBWA2_GAME=ON` until 2026-10-26). Run it only under
`build/native/rundesk.exe` (invisible, silent, low priority), e.g. with a `-script` that clicks and takes screenshots:
`build/native/rundesk.exe $PWD/build/native/bookwormadv2.exe -gamedir=$PWD/assets/bookwormadv2 -userdir=<empty folder> -script=<file> -fast`.

`bwa2_luarun -runall` runs the main chunk of each script in a state of its own whose missing globals are a stub that
answers every index, call and operator with itself: the chunks define classes and functions and need the game's C++
side only when called.

## Testing

`tools/check.sh bookwormadv2` (3 seconds so far) runs the PopLua compiler's 150 dialect checks, undumps and runs all 462
compiled scripts, and recompiles the 8 `scripts/common/*.lua` sources the game ships beside their `.luc` files,
requiring chunks identical to the shipped ones in every table (`tests/poplua/pairs.py`, `cmpluc.py`). It fails on a bad
exit status or a count that is not the whole set. The game's own runs join it as the port grows.

## Mods

None. The port is the original game, unmodified and working like its exe, and adds nothing for later mods, not even hooks
(the user's rule of 2026-10-26); mods wait for the mod phase, after every PopCap game works in the browser. `MOD_IDEAS.md`
maps the 29 standard mods of `docs/MOD_ROADMAP.md` onto this game (27 fit; local achievements and two players do not, with
the reasons) and lists the ideas particular to it.

## Status (2026-10-26)

Milestone 1 of 5 is done: PopLua runs all 462 scripts headless. Volume 1's VM and compiler undump all of them, run each
main chunk, and compile the shipped sources to the shipped chunks. `luaV_execute` differs in layout from Volume 1's (the
later compiler), and its handlers have not been read case by case against `POPLUA.md`; the battles of milestone 3 are
the real test of the VM.

Milestone 2 (the title, menus, dialogs and cut scenes) is in place for a game that is not yet finished: 
`bookwormadv2.exe` builds from `src/` reworked for Volume 2's
resource ids and classes, and runs headless under `-script`: the PopCap logo, the title with its curtain, the New User
dialog and, after OK, the intro cut scene and the prologue battle of book 0, drawn by Volume 2's scripts (`PORT_PLAN.md`);
the in-game Menu dialog with Options and Quit To Main Menu; the main menu (rebuilt from the original's: seven items, the
animated portal, Lex and his bubble); the Options dialog and the Hall of Fame; and the portal's click, which takes a game
saved in the prologue back into the battle; and the replay of a finished adventure (the "continue your current adventure?"
dialog, `BookSelectScreen` with its three books, and the curtain's states 9, 10 and 12 up to the replay's chapter map and the
Hall of Fame after it), tried with a profile set to a finished adventure; with a treasure in its `Treasures` key the soak plays a replay
through Chapters 1 to 5 of Book 0 with no script error, and after a quit in the middle of one the menu's "Continue your current
adventure?" Yes brings it back where it was (a replay adds the saved treasures back to Lex, and the last one is the picture of the
altar after a win: a profile with no `Treasures` key gives the scripts' own error there, `PORT_PLAN.md`). The music plays from the ten loose OGG files over the framework's music
interface, by song id. Also Volume 2's own: the mini-games' menu (the buttons by what is unlocked, the "new" mark, the
tooltips checkbox, the two ways in and out), the cut scene (a click skips to the end of the scene), the credits (with the
midground animation and the finale's tiles) and the stats book (the tome of knowledge with its list of secrets, the media
player and the page after a timed game; the tome opens and its lists scroll and select, tried with a profile that has the
tome unlocked, but it shows only the "????" lines of a game with nothing seen yet). The word list is Volume 2's too: each
word has the noun, verb and adjective marks of `data/compressed_attribs.dat`, which the board hands the tile engine's script,
and a new player's tutorial runs to its first attack (P, L, A, Y clicked, the "Click to attack!" hint, the attack and the
"Well done!" panel). The battles (milestone 3) play natively: the test hook's `soak <update>` (a bot that spells the longest
word of the board, uses the tutorial's potions, ends a stunned turn, picks treasures and the companion, and clicks the chapter
map's Enter) takes a new player from the prologue through the adventure's books (its bosses included) to the end of Chapter 10
of Book 6 and back to the main menu (441 words in about 480,000 updates, no script error); it is not yet a check in the suite.
The same bot plays the six mini-games (started with `main.StartMinigameFromMainMenu`) for 60,000 to 70,000 updates each, no script
error; Golden Coins and Word Up end in the Hall of Fame with their entry. The arena plays too: with a profile at book 7 the
soak plays all 24 stages (the checkpoints' treasure rooms included) and beats Bigger Brother in 87,000 updates; the win
dialog's Continue calls the scripts' callback, which opens the stats book (the "Game Over" page), and its Continue ends in the
Hall of Fame's arena list with the run's entry. `tools/wasmrun.js` runs the web build under Node
with the switches of a native run (the soak in the browser build: 485 words, and the whole arena, no script error). In the Browser
pane the real install folder imports through the page (9,255 files) and plays from the New User dialog through the intro to the
first battle and its first attack. A saved arena run
comes back through the main menu's "Continue game?" question in the middle of a battle and in a checkpoint's treasure room (native). A crash of the native Windows
build writes where it was to the log (`src/CrashLog.cpp`, for the soak's memory checks and the next bug).
The cut scene player draws the bitmaps (JPEG and lossless) of Volume 2's five movies, so the comic panels of the intros and
the ending show their art. PopAnim loads all 304 animations; the 41 pictures that are not found belong to seven animations that no script plays and
whose pictures the game's folder lacks (`PORT_PLAN.md`). The end-of-game credits run from the Hall of Fame's Continue (`lua gCApp:DoEndGame()` from the main menu, 31,000 updates): every section, then the finale ("Thanks for playing!" in bobbing tiles, Lex, the flash) and back to the main menu. (The trial wrapper's dialogs are left out on purpose.)

## Changes outside the Bookworm Adventures Volume 2 folder

`CMakeLists.txt` (the `BUILD_BOOKWORMADV2` option), `cmake/ByopopStores.cmake` (the `BYOPOP_BUY_bookwormadv2`
placeholder; its stores are checked at the web milestone), `.claude/launch.json` (`bookwormadv2-web` on port 18792) and
`tools/check.sh` (the `bookwormadv2` suite).

In `framework/`, for Volume 2's descriptors and files: `DescParser` (the original's escape rules for quotes and
backslashes), `ImageFont` (the `LayerSetExInfo` and `LayerSetAlphaCorrection` commands), `PropertiesParser` and
`XMLParser` (a byte order mark at the start of a file).
