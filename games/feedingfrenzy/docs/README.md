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

## Removed from the port

Nothing is removed yet. The registration and outbound-button pass comes after the identical-play pass; its list is:
the main menu's More Games button and `gameLink`/`ShowLink`/`VisitURL`, `MSNZone`, `ScoreBroadcast`, `ScoreSubmit`,
`GameBreak`, `doGameLink` (0x42de30), `getFull`/`GetFullVersion`, and the screen saver and its unlock popup (`/s /c /p`,
controller slots 0x0c and 0x10, `ssManifest`, `SaverSelection`, states 10, 0xb and 0xc, 0x42ca80, 0x42d4a0, 0x42d750,
0x430cb0, `FishFood::0x42be40`, the partner slot 0x42eef0, `ssUnlock.xml`, 0x427200, 0x429010), with the screen saver
sentence of the game-won screen.

## Deviations

- **The cursor** while a stage is played is hidden and put back at (325, 265) after each reading, as the player fish
  does, through the Host's cursor API (`warpMousePosition`); the original reads and resets the system cursor.
- **The splash** is taken out of the scene after its fade.
- **The key handlers** are kept in the controller (see "The engine's older build").
- **Debug text** the original keeps behind flags nothing sets (the player's thought bubble, the spawner's +0x170) is
  left out, and so are fields nothing reads.
- **Uninitialised fields** of Barracuda, PufferFish and the screens are zeroed. The HUD's menu-bubble array is zeroed.
- **JohnDory** purges a dangling pointer the original keeps; `Container::update` guards an out-of-range index; the
  Barracuda's Blinker and the GoldenMinnow's Selector no longer leak a reference.
- **Duplicate calls** of `getSpecialScore` and `getScore` are collapsed; the stage manager's dead timer call (0x438e50)
  is not made; the stun stars the poison blend hides are not drawn.
- **A guarded node** that the original would use unchecked and crash on is checked; the bonus percent has a zero guard;
  the unused fun fact 49 is dropped; the high-score layout's else branch over-read is left out.

## Fix candidates

For the fixes mod (`FixOn`), not the port: the pickup invincibility check is asymmetric between the two fish kinds; the
high score qualification at state 6 uses the normal list and rank below 10, where the game-won screen uses rank below 11;
the player's shield effect moves between groups while a group is iterated.

## Oddities

Not chased: the fun-fact text on the stage end screen is clipped on one line; the default settings come up full
screen. Both to be compared with the original in the looks pass.

## Unreachable code

Not translated (nothing reaches them): 0x405cd0, 0x40d770, 0x406050, 0x406090, 0x403650, 0x401ae0, 0x41f6e0, 0x41dc30,
0x43e560, the debug branch (+0x2cc) of 0x43bb60, the debug draw 0x410140 and its flag 0x4bffa0, 0x405770, 0x408980,
0x408ba0, 0x408e80, 0x40c7f0, 0x40cdc0, 0x40d890, 0x40d8a0, the debug-view slot 0x2bc (0x409a40, 0x40b190, 0x40cfb0),
the controller's setState cases 3, 7 and 8, state 3 in `goToStage`, the codec-missing screens, the tooltip screen (+0xa0)
and the map layout's slot 0x100 (0x423bef). The Angler, Lion and Puffer players are to be checked against the stages.

## Test commands

The standard commands of `docs/TEST_COMMANDS.md`, in `games/feedingfrenzy/src/TestCommands.cpp`.

- `state`: `STATE {...}` with `screen` (menu, options, highscores, credits, entername, gamewon, pause, map, loadmap,
  stagestart, stageend, continue, gameover), `over`, `mode`, `furthest`, and in a stage `level`, `score`, `lives`,
  `continues`, `stage`, `type`, `phase` (start, playing, paused, mermaid, ending, end), `timer`, `eaten`, `toEat`,
  `player {x,y,size,score,state,chain}` and `fish [{type,x,y,level,seen,edible}]` for every fish in play.
- `move`: the bot's next move as real input (menu: New Game; a stage: steer toward the nearest edible fish, away from
  fish that can eat the player; name entry: types BOT and clicks Continue; other screens: the first of Continue, Done,
  OK, Resume). `over` becomes true when a started game is back at the menu.
- `goto normal|timed|menu|options|highscores|credits`, `level <1..40>`, `unlock all`, `set score|lives|timer <n>`,
  `win` (the mermaid ending), `lose` (the player is eaten), `seed <n>`.

The suite (`games/feedingfrenzy/tools/suite/suite.sh`, `tools/check.sh feedingfrenzy`) has the bot play the normal game
(1500 moves) and the timed game, each twice with the same transcript, then loads and plays all 40 stages.
