# Feeding Frenzy port

A port of Feeding Frenzy Deluxe (Sprout Games, published by PopCap Games; © 2004-2005 Sprout Games) to the shared
platform layer, natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of the game's exe,
found by decompiling it with Ghidra; the game's files (`FFArchive.saf` with the pictures, fonts, screen layouts, stages
and fish, the loose `resources/` sounds and music, `properties/` and `FeedingFrenzy.exe`, whose texts the game reads) come
from the player's own copy and are never in the repository.

Feeding Frenzy is not a SexyAppFramework game. It runs on Sprout Games' own first-generation engine, the C++ namespace
`engine`, in a build older than Pizza Frenzy's and Word Harmony's: `engines/sprout` is the shared translation of it
(`docs/ENGINES.md`, "Sprout"), and where the older build behaves differently the game sets which (below, "The engine's
older build").

## The binary

`FeedingFrenzy.exe` (the PopCap Game Collection's copy, 992,976 bytes, a plain exe with no wrapper) was linked on
2005-07-18. Its layout, by the export's functions:

- 0x401000-0x43e000: the game, about 1,340 functions (234 KB): the fish and their configuration, the stages and their
  manifest, the player fish, the bonus bubbles, the map, the menus, `StageMgr` (the vtable at 0x4a7060, 131 slots) and
  `FishFood`, the game's controller (0x260 bytes, constructor 0x430ee0, vtable 0x4a6640).
- 0x43e000-0x478000: Sprout's engine and the layer above it (fonts, particles, splats, the layout reader and its
  widgets, the application's run loop at 0x46d060 and 0x46d0e0, the sound system over DirectSound, "SimpleSoundDX").
- 0x478000-0x49d000: the C runtime, libpng, zlib and Ogg/Vorbis (100% pairs with the other Sprout exes).

The window is 640x480 at 30 frames a second (the controller's configure, 0x42b8e0, says so; the title is "Feeding
Frenzy", the icon 5). The controller has nine virtual slots: the destructor (0x431310), configure (0x04, 0x42b8e0), init
(0x08, 0x430b80: the archive FFarchive.saf, `config\gameStrings.xml` and `config\stages\stageManifest.xml`), the screen
saver's init (0x0c, 0x430cb0, the same with `config\stages\ssManifest.xml`), and four more (0x10..0x20). The exe is also
the screen saver (`/s`, `/c`, `/p`): the port has no screen saver, so those paths are left out.

## The export

Everything derived from the game is in the worktree's git-ignored `re/feedingfrenzy/`: the exe (`bin/`), the Mac build
(`mac/`), the archive extracted for reading (`saf/`, by Word Harmony's `tools/extract_saf.py`; the format is the same
`FFAS` version 1), the Ghidra export (`gh/`), the vtables, the function matches (`match/`) and the symbol files applied to
the export (`symbols/`). The Ghidra project is `feedingfrenzy_game` in `E:/retools/projects`.

```
python -I games/feedingfrenzy/tools/vtables.py                           # re/feedingfrenzy/vtables.txt, vfuncs.txt
python -I games/feedingfrenzy/tools/port_names.py pizzafrenzy re/feedingfrenzy/match/pizzafrenzy_port_names.jsonl
python -I tools/funcmatch.py re/feedingfrenzy/bin/FeedingFrenzy.exe re/feedingfrenzy/gh/functions.jsonl \
    re/pizzafrenzy/bin/PizzaFrenzyGame.exe re/pizzafrenzy/gh/functions.jsonl re/feedingfrenzy/match/pf.match.jsonl \
    --symbols re/feedingfrenzy/match/pizzafrenzy_port_names.jsonl
REF=pizzafrenzy python -I games/feedingfrenzy/tools/enginematch.py re/feedingfrenzy/match/pf.match.jsonl \
    re/feedingfrenzy/match/pizzafrenzy_port_names.jsonl re/feedingfrenzy/match/engine.match.jsonl \
    re/feedingfrenzy/symbols/pizzafrenzy_engine.jsonl
build/native/rundesk.exe bash games/feedingfrenzy/tools/ghidra_pass.sh   # the virtuals, the calls, the symbols
python -I games/feedingfrenzy/tools/classmap.py > re/feedingfrenzy/classmap.txt   # the classes, for reading
python -I games/feedingfrenzy/tools/fn.py <addr>...                      # functions of the export, compacted
```

Ghidra finds no vtables in a program with RTTI for three classes only, so `vtables.py` finds 208 (runs of code pointers in
`.rdata` whose first entry the code refers to) and the pass creates their 702 virtual functions and gives 2,836 indirect
calls their argument counts.

Only 1,160 of the export's 4,130 functions are the same, after masking addresses, as a function of Pizza Frenzy's, Word
Harmony's or Iggle Pop's exe, and nearly all of them are the C runtime and the libraries: the engine is an older build
(1,000 pairs after the string, vtable and call passes, 40 with a name from the shared engine). The Mac build
(`re/feedingfrenzy/mac/`, Feeding Frenzy Deluxe 1.0.0) is stripped of function names as Iggle Pop's is, so
`tools/macmatch.py` finds nothing to carry over; its class names, bases and vtable sizes (148 classes, 36 of the engine's)
are the names of the game's classes (the fish, `StageMgr`, `FishFood`, the screens and their document handlers).

## The engine's older build

The Mac build's classes show where the older build stands between the two the shared engine knows. It has the signal
build's `Scene` (`engine::Scene : Container, sigslot::HasSlots`), `FadeContainer`, `ScreenLayout : FadeContainer,
IButtonListener` and `Splat`, `SplatFactory`, `ParticleSystem` and `MusicSystem` as game-layer classes, and the layout
reader's `addSignalImage`, `addSignalText` and `startSignalElement`; but no `GenericScreen`, `StateMgr`, `ScreenMgr`
or `Transition`: the game's own `StageMgr` and screens (`MainMenuScreen`, `StageChooserLayout`, `StageEndScreen`,
`HighScoreLayout`...) are `ScreenLayout`s that tell `ILayoutActionListener` and `IButtonListener` interfaces. Its sound
system is `SimpleSoundDX` (a `SimpleSound` over DirectSound with ACM and WAVE file readers), loose WAV files in
`resources/` rather than Ogg streams in the archive.

What the exe's own engine does, by the functions the port translates (`gEarlyBuild`, `engines/sprout/Build.h`):

- **The layout reader** (0x457ee0) tries the elements `image`, `text`, `imageButton`, `checkBox`, `textButton`, `group`,
  `particleSystem`, `textSub` and `include`, in that order, and no others: no screen, key action, edit box, list box,
  table, column or selector element. Its image and text are the listener build's (`addImage` 0x453120, `addText`
  0x4562f0), the check box has no pivot and no start state, the image button sets no base scale, a group does not lay
  itself out, and its point reader is the listener build's `"%f,%f"`. `textButton` (0x455440), `textSub` (0x457120) and
  `include` (0x457740) are new in the shared engine (`TextButton.*`, `Layout.cpp`).
- **The scene** (vtable 0x4a8888) takes the application's mouse signals and gives them to the nodes' mouse slots with a
  point, as the earlier build's window does: the node the hit test finds, or the node that has the mouse, gets
  mouseDown, mouseUp, rightDown, rightUp or mouseMove, and the node under the mouse changes by mouseExit and mouseEnter
  (0x44cef0). A button is pressed by its mouse down (`Button::onMouseDown`, 0x4527e0), where the signal build's is
  pressed by the release. Nodes have no `refreshMouse`, so a screen that comes in under a still mouse does not hover
  what is under it until the mouse moves.
- **The key handlers** are a list the original keeps in an object of the engine (+0x108, added to by 0x444c00 and
  removed from by 0x443640); the port keeps it in the controller, `FishFood`, which passes the keys on to the handlers,
  the last added first. The app has no window node (the earlier build's root), so there is nothing else to hold them.
