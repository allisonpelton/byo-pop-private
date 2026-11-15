# Pizza Frenzy port

A port of Pizza Frenzy (Sprout Games, published by PopCap Games and GameHouse; © 2006 Sprout Games) to the shared
platform layer, natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of the game's exe,
found by decompiling it with Ghidra; the game's files (`PizzaFrenzy.saf`, which holds the pictures, fonts, screens,
cities, levels, stories and sounds, the loose `splash/` and `properties/`, and `PizzaFrenzy.exe`, whose string table
the game reads its texts from) come from the player's own copy and are never in the repository.

Pizza Frenzy is not a SexyAppFramework game. It runs on Sprout Games' own first-generation engine, the C++ namespace
`engine`, which Word Harmony also runs on; the port shares Word Harmony's translation of it, `engines/sprout`
(`docs/ENGINES.md`, "Sprout"; Word Harmony's `docs/ENGINE.md` describes it). Pizza Frenzy's exe has a different build
of the engine from Word Harmony's, though it was linked a day earlier: its node classes have more virtual slots, its
application, scene and screens are wired with sigslot's signals, its sounds are Ogg Vorbis streams. The engine grows
with what Pizza Frenzy's build adds, and where the two builds' versions of a function behave differently the game
sets which (below, "The signal build").

## The binary

<!-- private -->
`PizzaFrenzy.exe` (the Steam copy, 1,620,898 bytes) is PopCap's DRM wrapper with the game exe appended unencrypted
after the marker `!popcapdrmprotect!` (offset 352,256; the PE image starts at 352,674 and runs to the file's end).
`tools/extract_game_exe.py "<install>/PizzaFrenzy.exe" -o re/pizzafrenzy/bin/PizzaFrenzyGame.exe` carves it out
(1,268,224 bytes).
<!-- /private -->

The game exe (v5.8.30.1) was linked on 2006-02-21 by Visual Studio .NET 2003 and has RTTI only for three engine
classes. Its layout, in link order:

- 0x401000-0x461680: the game, 1,845 functions (354 KB): the controller and its states, the cities and their tiles,
  the vehicles, the customers and their orders, the pizzas and toppings, the three modes, the special events, the
  stories, the pizza designer and the topping book, the users, the settings and the high scores.
- 0x461680-0x49c000: Sprout's engine, about 1,200 functions: objects, the scene graph, screen layouts and their
  widgets, fonts, particles, the sound and music systems over DirectSound, the archive and the resource manager.
- 0x49c000-0x4ac440: libogg and libvorbis (the platform decodes Ogg Vorbis itself).
- 0x4ac440-0x4bcc50: the C runtime.
- 0x4bcc50-0x4d0a00: the engine's Windows layer: the application and its window (class "Gatsu"), the registry, the
  crash handler, the DirectDraw display and its surfaces, the image loader.
- 0x4d0a00 on: libpng 1.2.5 and zlib.

The game's texts are in the exe's string table (ids 104-105 and 201-235: the titles of the modes and their
descriptions, the level ends, the frenzy names, the credits); everything else a player reads is in the archive's XML
files. The window is 800x600 (the controller's configure, 0x4050d0, says so; `properties/partner.xml`'s 640x480 is the wrapper's), at 30 frames a second.

## The export

Everything derived from the game is in the main checkout's git-ignored `re/pizzafrenzy/`: the carved exe (`bin/`),
the archive extracted for reading (`saf/`, by Word Harmony's `tools/extract_saf.py`), the Ghidra export (`gh/`), the
vtables, the function match with Word Harmony (`match/`) and the symbol files applied to the export (`symbols/`). The
Ghidra project is `pizzafrenzy_game` in `E:/retools/projects`.

```
CLASS_RECOVERY=0 build/native/rundesk.exe bash tools/ghidra_import.sh re/pizzafrenzy/bin/PizzaFrenzyGame.exe \
    pizzafrenzy_game re/pizzafrenzy/gh                                    # import, analysis, first export
python -I games/pizzafrenzy/tools/vtables.py                              # re/pizzafrenzy/vtables.txt, vfuncs.txt
build/native/rundesk.exe bash games/pizzafrenzy/tools/ghidra_pass.sh     # the virtuals, the calls, the symbols
python -I games/pizzafrenzy/tools/port_names.py wordharmony re/pizzafrenzy/match/wordharmony_port_names.jsonl
python -I tools/funcmatch.py re/pizzafrenzy/bin/PizzaFrenzyGame.exe re/pizzafrenzy/gh/functions.jsonl \
    re/wordharmony/bin/WordHarmonyGame.exe re/wordharmony/gh/functions.jsonl re/pizzafrenzy/match/wh.match.jsonl
python -I games/pizzafrenzy/tools/enginematch.py re/pizzafrenzy/match/wh.match.jsonl \
    re/pizzafrenzy/match/wordharmony_port_names.jsonl re/pizzafrenzy/match/engine.match.jsonl \
    re/pizzafrenzy/symbols/wordharmony_engine.jsonl                       # then ghidra_pass.sh again
python -I games/pizzafrenzy/tools/classmap.py 401000 461680               # the game's classes, for reading
python -I games/pizzafrenzy/tools/fn.py <addr>...                         # functions of the export, compacted
```

Ghidra finds almost no vtables in a program without RTTI, and a virtual function only ever called through its vtable
is never disassembled; `vtables.py` finds 393 vtables (runs of code pointers in `.rdata` whose first entry the code
refers to; a slot may point anywhere in the code, to `_purecall` or to a tiny body the linker folded, 0x4d0ca0 among
libpng's), and the pass creates their 923 functions and gives the 4,400 indirect calls their argument counts.

The engine's functions are named from Word Harmony's port through the two exes' function match: `tools/funcmatch.py`
pairs the bodies that are the same after masking addresses (692) and their callees (151), and `enginematch.py` adds
the pairs that a string only they use, a vtable's slots or the order of their calls make (929 in all, 105 of them
functions Word Harmony's port names). There is no Mac build of Pizza Frenzy. Iggle Pop's Mac build
(`re/igglepop/mac/`) shares the engine's build and its RTTI names the engine's classes and their bases
(`engine::Component`, `Container`, `Scene`, `DrawContext`, `Application`...), which the shared engine knows by Word
Harmony's names (`Node`, `Container`, `Window`, `Graphics`, `App`); its symbols carry no function names, so
`tools/macmatch.py` finds nothing to carry over.

A virtual function Ghidra never disassembled shows in the export as a `PTR_LAB_` vtable; `re/pizzafrenzy/labels.txt`
lists such functions (the signal handlers the code passes as data, and the vtable slots that pointed at no function,
found by reading every `PTR_LAB_` and `PTR_FUN_` table of the export from the exe), and the pass creates them.

`fn.py`'s compacted output was wrong until 2026-11-14 in two ways, both fixed: it dropped `break;` and `continue;`
lines (its filter of unused declarations took them for one), and every constant store to a `uStack_` temporary
(which is how Ghidra shows arguments built on the stack, a key's time or a point). Where a function's floats pass
through the x87 stack Ghidra shows no value at all (the splash state's load bar share, stored into its frame
argument's slot); `llvm-objdump -d` (in the toolchain's `bin/`) shows them.

## The signal build

Iggle Pop's Mac build has the same build of the engine as Pizza Frenzy's exe, and its RTTI names the classes the
listener build (Word Harmony's) does not have, so the shared engine takes those names: `Scene`, `GenericApplication`,
`State` and `StateMgr`, `GenericScreen` (on `ScreenLayout`), `ScreenMgr`, `Transition`, `FadeTransition` and
`SlideTransition`, `PositionInterpolator`, `GenericGameOptions`, `SoundMgr` and `SoundMgrDocHandler`, `ImageButton`,
`CheckBox`, `EditBox`, `ListBox`, `Table`, `BaseDocHandler`, `UserInfo`; the game's own `UserGameData`,
`GameMusicPlayer` and `GameMusicClip` too. Classes both
builds have keep Word Harmony's names (`Node` for `engine::Component`, `Button` for `ButtonBase`, `LayoutHandler` for
`ScreenDocHandler`). The game sets `engine::gSignalBuild` (`engines/sprout/Build.h`) in its application's constructor;
a shared function that behaves differently in the two builds reads it:

| Function | The signal build's difference |
|---|---|
| `Node::isVisible`, `setEnabled`, `isEnabled` | visible also needs the drawable flag; enabled is the node's own mouse flag, not masked |
| `FadeContainer::FadeContainer`, `fade`, `update` | no name; a fade in starts from clear; a fade out can end with the container removed |
| `ScreenLayout::ScreenLayout`, `loadLayout` | no mouse flag cleared, no name; the key actions dropped and the screen named after its file first |
| `LayoutHandler` | the signal build's elements and attributes |
| `Button`, `ImageButton`, `CheckBox` | the mouse through the node's event slots, a state field, missing images made from the up image |
| `ImageNode`, `TextItem` | a clip and the image's size; the text's colour, shadow and word wrap; no names |
| `Image::setPivot(mode)` | a mode other than the centre puts the pivot back at the corner |

The signal build's parts of `ScreenLayout` (key actions, the default and cancel actions, connecting and disconnecting
while shown) and of `Node` (connect and disconnect, the mouse mask, removing a listener) are additions the listener
build never calls. The screen's slot 0x148 (0x465d60) would connect a button's signal, but nothing reaches it: the
layout reader hears buttons through `listenTo`, so it is left out. The players (`Players`, the registry key's `users`
subkey, a subkey a player and the key's default value the current one) and the local scoreboards are engine classes;
the online scoreboard and its posting are outbound and left out.

Exception texts (a manifest that does not parse, an entry registered twice) stay literal in the code, as the shared
engine's already are: they are the original's messages to its developers, not texts a game shows.

## Progress

The engine's signal build is translated as far as the splash screen and the layouts need: the application's signals
and run loop, the scene, states, the generic screen with its transitions, the screen manager, the settings, the sound
manager, the layout reader's elements and their widgets, the players and the local scoreboards. The game has its
application's configure and init, the tile manifest (every entry the file has), the player's progress and save, the
music player, and the loading state's stages for the manifest, the pause menu, the confirmations, the restore dialog
and the music. Next: the signal build's drawing (the node's colour modes and smoothing, scaled images, the text's
drawing), then the main menu, the options, the player select and entry, the high scores and the menu state, then the
game itself.

Left for later: a topping's preview image (0x41dc60 draws the topping onto a copy of the plain pizza; the port's
images cannot yet be copied or drawn into), the particle effects' systems (0x47bc40), and the image operations the
decorations use (`cropImage`, `shiftImageColors` in the host), which copy pixels where the original blends them.
