# Atomica Deluxe: behaviour notes (work in progress)

Addresses are virtual addresses in `re/atomica/bin/Atomica.exe` (version 2.52, 2003-03-28, linked with Visual C++ 7.0
= VS.NET 2002, image base 0x400000). The game classes have **no RTTI** (only the framework's do), so the Ghidra
export has them as `FUN_` functions; their vtables were found by scanning `.rdata` for runs of function addresses.
Ghidra must run with `MSVC70=1` (see README.md), or `_free` is wrongly marked as not returning and functions end early.
Offsets such as `app+0x3e0` are byte offsets in the original object; the port's classes name the fields.

## 1. Binary layout

Game code is 0x401000-0x435fe0 (453 functions, 210 KB). The 2003 framework follows (Graphics 0x436000-0x437150, SexyApp
0x437xxx-0x443xxx, widgets 0x444000-0x452000, images 0x453000-0x467000, sound 0x463000-0x467000), then the C and C++
libraries. The code uses MSVC 7.0's `std::string` (16-byte small buffer: `_Ptr` at +4, size at +0x14, capacity at +0x18).

| Range | Class |
|---|---|
| 0x401000 | `GameObj` base (vtable 0x4ae380), `Atom` (0x4ae3bc, 0xf0 bytes) 0x401060-0x4037f0, `Atom::Draw` 0x4037f0 |
| 0x4043d0 | `ElementAtom` (the five element bonus atoms; derives Atom, 0x128 bytes, vtable 0x4ae478) |
| 0x4056b0 | `Meter` (0x4ae4a8): the level, the molecules made, the meter's value and its full flag; it is the board's +0xc4 |
| 0x405ad0-0x406240 | app helpers: show/hide screens, `HS.CFG` and `SAVEGAME.DAT` I/O, pause counter |
| 0x406260-0x40b1e0 | `AtomicaApp` (ctor 0x406570, Init 0x4067b0, LoadingThreadProc 0x406f90, registry 0x406dc0/0x406ec0) |
| 0x40b200 | `Atomicus` (the professor's face: mood, comments), small classes 0x4af478/0x4af490 |
| 0x40c340 | `Grid` (Widget, 0x4af748; the 9 x 8 field, the atoms list, the rules) 0x40c340-0x414830 |
| 0x4145b0... | effects, particles, floaty texts (0x4af864 ...), 0x4167a0 .. 0x4229d0 |
| 0x4198e0 | main menu widget (0x4afd60) 0x4198e0-0x41c4b0 |
| 0x41cf90, 0x41ec90, 0x4202b0, 0x426710 | dialogs: options, high scores, level-up panel, ... |
| 0x429470-0x429f40 | `Sound`, `SpriteMgr`, `Sprite` (the image holders) |
| 0x42a310 | `HintBox` (the 28 tutorial hints, 0x8c bytes) |
| 0x42fe90 | `TitleScreen` |
| 0x431300-0x434200 | `Board` (Widget; ctor 0x432c20, 0x2e8 bytes, Draw 0x433f10, Update 0x433080) |

## 2. Support classes (Support.h)

- `PtrList` (0x4259a0): array of pointers with an iteration cursor (`Reset` 0x425c00, `Next` 0x425c10).
- `GameRand` (0x425e30): lagged Fibonacci over 55 words of 30 bits, taps 24 apart; `Get(range, signed)` takes the
  low bits for the smallest power of two >= range and reduces them modulo range. Lives at app+0x3da4.
- `Sprite` (0x429680): image plus source rectangle. `SpriteMgr::Cell/SubSprite/Blank/Load`.
- Graphics (0x436000-0x437150, a non-virtual class; +4 dest image, +8/+0xc translation, +0x10 clip, +0x20 colour,
  +0x40 font, +0x44 draw mode, +0x48 colorize flag): `SetFont` 0x436000, `SetColor` 0x436010, `SetColorizeImages`
  0x436070, `FillRect` 0x436bb0, `DrawString` 0x436920, `StringWidth` 0x436970, `DrawImage(img,x,y,srcRect)` 0x436da0.
  **Float and extra stack arguments are often lost by the decompiler** (they sit on the x87 stack or under a callee
  that pops fewer arguments): look at the disassembly (`llvm-objdump -d -M intel --start-address=...`) when a
  call has fewer arguments than the callee takes.

## 3. Application

See AtomicaApp.h: the original app object holds every picture and sound as a field; the port keeps the layout
as named arrays. The loading (0x406f90) draws the explosion's three sizes, the 27 atom types' frames (cells,
24 shrunk copies, 20 turned masks and the atom blended 29.5 percent with each) and loads about 120 images, 11
fonts and 50 sounds; progress is the count of its steps (0x60a). The original caches the atoms' frames in
`images\cached\Atoms.WAD`; the port draws them at every start (1.6 s).

Registered game: the port has no free-play counter or order dialogs, and the title screen does not draw the line "thanks for registering" that the registered path of the original puts under the logo (removed with the other registration code at the final merge; the picture's box is masked in `shotdiff.json`).

## 4. The Grid and its rules

The field is 9 columns by 8 rows of cells of 50 pixels (the widget is at (165, 40), 450 x 400); the centre of a cell is
`col * 50 + 25, row * 50 + 25` and `mCells[col][row]` holds its atom (`0x40c4c0`, `0x40c4f0`). An `Atom` has 27 types: 0 to 10
plain, 11 chromium, 12 to 26 the elements' bonus atoms; its kind is plain, chromium (a wildcard) or biohazard (it spreads
to a free neighbour every few moves until a molecule destroys it). The atoms of a round are first *pre-atoms*: their place
is reserved, they cannot be moved, and a path may cross them; a move onto a pre-atom's cell takes the cell over.

A **molecule** is a block of atoms of one type, 2 x 2 (the combo counts 1), 3 x 2 or 2 x 3 (it counts 2); chromium atoms count
for any type; a block with an empty cell or a pre-atom has the type -1 (`TypesInRect` 0x40c700). `FindMolecules` (0x412280)
scans every cell for the three shapes in this order, takes the atoms (`Collect` 0x40ef10: a new group that shakes for 50
updates and then flies away), raises the combo, fills the meter and floats the score (`(max(combo - 1, 0) * 5 + 5) * 16`,
doubled in the expert mode). `AfterMove` (0x4127c0) runs after each of the player's moves: the molecules, and if there are
none the biohazards spread and the next round's atoms come (`mAtomsPerRound`, at random free cells). A move is a path of
straight lines through free cells with as few turns as the cost allows (`PathFinder`, 0x4246a0-0x425110).

The strategy mode's **level** needs `mMax` molecules (the board's meter, set by `StartLevel`); every fifth level is a bonus
level (`SetupBonusLevel` 0x40dd10) with groups of four atoms of each colour; the radiation rounds mark cells that
must be cleared. The action mode (`SetupTimedLevel` 0x40f730) has no rounds: a clock (`mF390`, 300 to 0) adds atoms and the
meter drains; a molecule empties it, and it fills when it runs out. The field is lost when more than 71 atoms stand on it.

## 5. The board

The board draws the machine around the field and owns the meter, the atomic bomb (`mProgress` 0 to 48, `CheckBomb` 0x4322c0,
`StepBomb` 0x432360: it bursts atoms until its charge is used up), Atomicus, the effects list and the floating scores.
The level sign counts down from 0x163 (`mI16c`); the field is revealed by sparks (`Sparkle`, 0x41c5f0) that set the cells'
state from -100000 to 0x40 and the grid's update brings it to 255.

## 6. The tutorial boxes

`HintBox` (0x42a310; draw 0x42a810) is a widget over the whole screen that is also a base modal: the game is paused
(`mPauseCount`) while one is up. It is a box of its own size by hint (the constructor's switch), a green frame that fades in by 8 an
update, a title bar, a Continue button (id 0) and, for a tutorial box, a check box (id 1) whose picture is the app's `mCancelTip`;
the box of a popup (hints 0x11 to 0x16) has no check box. Ticking it writes the hint into the app's `mHintsShown` when the box
goes; a box whose hint is there already, or with the hints switched off in the options, goes at its first update. `Grid::mHintShown`
(+0x3b4, one byte per hint) is set by the constructor and is what `CheckHints` (0x40d450) tests, once per game. The registry
value `GameHints` holds the app's table (255 words), `ShowTips` is one when the hints are *off*.
The draw is a switch of 28 cases of "colour, text at a height, lit word": the lines are generated from the decompilation
(`tools/genhints.py` reads the draw and the disassembly and writes `src/HintData.inc`, numbers only; a text is the exe's, read by
address). The decompiler loses the height of a lit word (the line before it has it) and the arrows' places (read from the
disassembly: a black copy four pixels down and right, then the coloured one).
The board opens the popups on a click (0x432000): Dr. Atomicus 0x11, the bomb gauges 0x12 (not ready) or 0x13, the geiger 0x14, the
meter 0x15 (strategy) or 0x16. The bomb button is disabled (`SetDisabled`, vtable +0x28) while the bomb cannot be used.

## 7. The swirls

The tunnel behind the field. `Swirl` (0x1834 bytes; one virtual, the destructor) is a ring of twenty points at 18-degree steps
round a centre that drifts to a target; the board's list `mSwirls` (+0x110, the oldest first) is updated every update (`Update`
0x427c70) and a new one is made every 95. Each shrinks from radius 650 to its own minimum (30 to 60) and goes. `Draw` (0x428940)
fills each as the band between its ring and the ring of the non-clone before it (the first as a disc to the middle) with flat
triangles (0x4283a0, spans handed to `Image::FillScanLines`); its colour follows its size and the board's flash colour (atoms that
are absorbed flash it). An atom that leaves the field (`Atom::FlyAway`, 0x4027a0) gets a *clone* of the newest swirl (0x428870,
drawn without a ring) and circles in it by polar coordinates (`mFlyX` the angle, `mFlySize` the part of the radius); when the clone
goes, the atoms in it go with a flash and a sound. The constants that go on from one swirl to the next (the radius and angle of the
next target, the next minimum radius) are globals of the program, not of the game. A new game starts with `mB98`, which repeats the
swirl update until the first swirl has gone, so that the background is full from the first picture. Everything here uses the game's
random numbers (the ring's jitter, the flicker of an atom's brightness), so the bot's results depend on it.
The freeze (water element) walks `mGlowSwirl` along the list every ten steps; that swirl glows brighter.

## 8. The doors (main menu and high score box)

`MainMenu` (0x4198e0, `Update` 0x4191d0, `Draw` 0x41a010) and `HighScoresBox` (0x41ea40, 0x41e770, 0x41ec90) share one
animation: two halves of the picture `menudoors` that stand `mDoor` pixels apart (0x12f at first, 0 shut, 0x14a wide open), and
the panel (`menupanel`) that hangs `mPanelY` pixels above its place. The menu comes over the title screen or the game that ended
and the doors close over it, each update `door = -1 - ftol(door * -0.95)`; when the gap is gone they slam (`mSndDoorSlam`),
0x405e20 deletes the title screen and the board behind them, and 75 updates later (15 in the high score box) the panel comes
down (`panelY = -1 - ftol(panelY * -0.9)`, `mSndSwoop` when it has arrived) while its contents fade in 3 a step. A game that
starts (`ButtonPress`, the listener's slot 1; the board is made at once and stays under the menu) makes the panel go up by a tenth
more of what it has gone each step and, when it is away, plays `mSndDoorOpen` and opens the doors by a quarter more each step;
at 0x14a the app's `mB3eac` closes the menu (the board's update does it). "Main menu" after a game lets the panel drop out of
sight below the screen (a tenth more each step, `mSndSwoop` when it is under 480) and come back from above with the main page.
`ButtonDepress` (slot 2) only handles the exit. The menu's music is song 1 (order 0x1f; 0x29 after a game), stopped by the
menu's destructor; the game's is song 0. The box's "return" button plays its click on press and closes the box on release.
The widget's `mHasTransparencies` is `mDoor != 0`: while the doors are shut the menu hides what is under it.
(The original's `HighScoresBox::Update` goes on to write the widget's flag after it has deleted the widget; the port returns.)

## 9. Standard test commands

`testhooks/Commands.cpp` answers the standard commands of `docs/TEST_COMMANDS.md`, reachable only from a test (`-script`,
`-commands`, the web test page's `command`); a command that is never given changes nothing. Each answers with one line:
`OK <line>`, `ERROR <line>: <reason>`, `STATE {json}` or `MOVE {json}`.

| Command | Does |
| --- | --- |
| `state` | `STATE` with `screen` (title, menu, starting, gameover, board, hint, levelup, highscore-entry, options, quit, confirm, highscores), `mode` (strategy or action), `expert`, `update`, `paused`, `windowed`, `over`, and in a game `level`, `score`, `molecules`, `atoms`, `combo`, `meter`, `meterMax`, `levelOver`, `held`, `hints`; on the page after a game `level`, `score` and `molecules` of the game that ended. `over` is true when that page is up with its panel and the entry box (if the score earned one) is done |
| `goto menu` | from the title screen (once loaded) or the page after a game ("main menu") |
| `goto strategy`, `strategy-expert`, `action`, `action-expert` | the menu's four start buttons, from the main page of the menu |
| `goto options`, `goto highscores` | the boxes, from the menu or a game |
| `goto title`, `help`, `credits` | `ERROR`: the title comes once; the credits are in the options box and the help is the hint boxes |
| `level <1..41>` | the level box's Continue: the meter, the colours and the field start over at that level |
| `unlock all` | `ERROR`: nothing is locked, every mode is open from the start |
| `set score <n>`, `set mode <strategy\|action>`, `set expert <0\|1>` | the board's score; what the menu's `move` clicks (mode and expert cannot change in a game) |
| `win` | the meter is filled now; the level box follows as in play |
| `lose` | the field is full now; the game over follows as in play |
| `seed <n>` | seeds the game's generator and the C library's |
| `move` | `MOVE {"kind":..,"input":[..],"updates":n}` or `MOVE null` |

`move` is the port's own bot (`testhooks/Autoplay.cpp`, `BotMoveJson`), the same that `-autoplay` runs, so a later bot mod
reuses it. It reads the screen like a hand: the title screen's link, the menu's start button of the mode that `set` chose,
the hint boxes (the check mark of the odd ones, then Continue), a held atom (placed where the bot chose), an atom to pick up,
the level box's Continue, the name of the high score entry (B, O, T and its button), and "main menu" after a game; it waits
while the game is busy (a box sliding in, the doors, the meter filling, a game over). A move is clicks (`{"click":[x,y]}` in
game coordinates), keys and waits; the choice of cell is the bot's (a block of 2 x 2 that lacks one atom, else a seeded
random cell), and it changes nothing in the game. `games/atomica/tools/realplay/drive.py` (native) and
`games/atomica/tools/web/onegame.js` (the player-only page) send those moves as real mouse and key events.

## 10. The fixes mod and the fix candidates

`mods/fixes/mod.json` is the one mod `fixes` (`docs/MOD_ROADMAP.md`, "The fixes mod"): a toggle for each fix, on by default on the
site, off when a test must play as the original does (`-nomods`). The port reads a toggle with `FixEnabled("<id>")`
(`Fixes.h`, the mod manager's live value), so the original's behaviour stays in the code as it was and each fix is one branch.

| Toggle | The fault in the original | What the fix does |
| --- | --- | --- |
| `drawRandom` | Every picture that moves at random (the monitor's static, the swirls' sparks, the glow of a molecule) draws its numbers from the one generator that deals the atoms (`GameRand`). How many frames the machine draws between two updates therefore changes which atoms come next: a game of the same seed plays out differently at 60 and at 100 frames a second, under `-fast` (a picture every few updates) and in real time. | `AtomicaApp::DrawDirtyStuff` brackets the frame with `GameRand::BeginDrawing/EndDrawing`: the pictures draw from a second generator, seeded together with the first, and the game's sequence is left to the updates. |
| `lipSync` | Dr. Atomicus's mouth steps by the clock (`Atomicus::LipUpdate` reads `GetClockTime`, and each whole 100 ms that the clock is ahead of the last step makes it take another step at once). A hiccup of the machine (a window dragged, a slow disk, the focus lost for a moment) makes a clip end early, and the next message, which waits for the end of the one before (`mTalking`), starts early with it: what happens at which update then depends on the machine. | One step for each update, whatever the clock says. |

`tools/fixes_check.sh` shows the first on the bot's strategy game under `-fast` (`-seed=1`, 12,000 updates), once drawing a picture every few updates and once
every update, without the mod and with it: without it the two state traces differ at line 125, with it they are the same
57,624 lines. The second is not shown by it: the clock does not lag under `-fast`, so the extra steps never happen there, and a lag cannot be made to
happen from a test (the code is one loop run once; the check shows only that the toggle changes nothing when the clock keeps up). With every toggle off the port is the original (the
frozen baseline, `tools/check.sh atomica`, plays the same with the mod absent).

What was checked for more fixes, and left as it is. *Bugs of the original that break play* (controls that stop working, a soft lock, a
crash, a lost save): none in the real-input play-through of every mode (`IDENTICAL_PLAY.md`, "Phase three"); the port already frees what the
original leaks and returns where the original's `HighScoresBox::Update` writes after deleting itself (section 8). *Waits that only cost the
player time*: the loading is the port's own (it loads at the machine's speed, section 11); the menu's doors and panel (75 updates after the doors slam, then
the panel's fall) and the 100 updates before a high score entry are animations that the next screen follows and cost under two seconds; the hint boxes fade in
and wait for their button. The one real wait is the level made in a strategy game: from the meter filling to the level box 550 updates pass
(`Board::Update`, `mOverTick == 0x226`, 5.5 s at 100 updates a second), in which Dr. Atomicus speaks (tick 10) and the atoms fly away (tick 200),
and the player can do nothing. It is the original's level-complete scene, not dead time, so it is kept and not made a toggle; if the
scene should be shorter, the toggle would move `ShowLevelUp` up to the tick at which the atoms have gone (the state trace would then
differ from the original's after each level). Nothing else was found.

## 11. Performance

Measured on 2026-11-08 with `games/atomica/tools/profile/perf.py` (adapted from Mummy Maze's), the port and the original back to back, on a
machine that was running four to eight other games of other sessions (the machine's busy fraction was 0.6 to 0.9 in every run, so
the spread is wide). The original runs as a copy under origrun; its sound stand-in is in, nothing else is changed in it.

| | Original | Port |
| --- | --- | --- |
| To the title screen's play link (the first screen that takes input), real clock, 3 runs | 1.07 s median (0.85 to 1.46) | 1.20 s median (1.17 to 1.22) |
| Working set / private memory at that point | 26 / 14 MB | 90 / 91 MB |
| Processor time to that point | 0.44 s | 1.2 s |
| Peak working set / private memory over the scenario below | 75 / 62 MB | 182 / 184 MB (175 to 188) |
| Processor time, launch to update 4,400 of the scenario below, no waiting | 2.5 s median of 9 runs (2.1 to 5.0) | 1.7 s median of 5 runs (1.70 to 1.83) |

The scenario is `tools/identical/s2.txt` cut at update 4,400 (the title screen, the menu, and the first 2,000 updates of a strategy
game that the bot plays from update 2,310). The original runs it on origrun's virtual clock (its update counter is not
where later builds keep it, so the real clock cannot time a scenario) and the port under `-fast` with `-script`, both without waiting, so the figure is
the work done: the port does it in less processor time. The load time is the real clock's: the original's copy loads in a thread
and shows its link when the thread has finished, the port loads in the main thread
(`PumpLoading`). The port is slower to load by about 0.1 s and holds more memory. Not measured, but the likely cause of both is that it draws the 27 atom
types' frames at every start (the original reads them from `images\cached\Atoms.WAD`, section 3) and keeps them as 32-bit pictures.
Memory is the port's weak figure (2.4 times the original's at the peak); it waits for the performance pass, where the atoms' frames
are the first thing to look at.

## 12. Coverage check (2026-11-09)

The whole coverage check of Atomica (`docs/COVERAGE.md`). origrun's `--coverage` recorded which of the original's functions ran, in 31
runs on the host (`games/atomica/tools/coverage/`: `cov.py` is one run, `run.sh` waits for room and starts it, the hits are in
`build/coverage/atomica/hits/`, git-ignored). `games/atomica/coverage.txt` sets the game's own code as one range, 0x401000-0x435ff0
(453 functions); the rest of the exe is the C runtime and FMOD. Every run ended with the game exiting normally, and none of them made a
sound (origrun's RESULT line of each said so).

| Runs | Count | Reached of 453 (running total) |
| --- | ---: | ---: |
| The identical-play scenarios (`tools/identical`: s1, s2, a1, sv, g4, g5, p2-title, p2-start, p2-start2, p2-dialogs, p2-focus, p2-hs, p2-d1, p2-g5, c1), virtual clock | 15 | 321 |
| The expert modes, recorded from the port's bot (`record.sh`, `-autoplayexpert`): `x-expert-strategy`, `x-expert-action` | 2 | 321 |
| Short runs: the menu's exit button (`x-exit-menu`), closing the window from the title (`x-quit-title`), a long idle (`x-idle`) | 3 | 322 |
| Real clock, for the sound code (focus lost and regained, a strategy game, an action game) | 3 | 322 |
| Crafted saved games (`sv-*.txt`, below) | 8 | 409 |

The first twenty-three runs left 104 functions reached with no definition in the port and 69 defined but never reached; every other
way of playing the bot reaches (a game ends at level 1 or 2) misses the element atoms, the bonus molecules' later shapes, the level
box, the bomb's second hint, the action mode's meter hits and the magnet levels, so the last eight runs start from saved games with
those in them. `mksave.py` writes each from a SAVEGAME.DAT that the port saved (`save-base.txt`: the port plays to level 4 and quits
with a save, under `make-saves.sh`) by putting in the atoms, the meter and the bomb's charge the run needs: the save's format is the
port's own and the original's (the identical-play pass compares the two games' saved files, `tools/identical/savecmp.py`), so the original's Continue button loads it. A scenario carries a line
`# save: <kind>`, and `cov.py` puts the file into the staged copy as its SAVEGAME.DAT. Each was designed in the port first
(`portdbg.sh`: the scenario through `-realinput`, which has the double click that `-script` lacks, with screenshots to see where the
clicks land), then run in the original. `make-saves.sh` rebuilds them from nothing; the files it writes are not byte for byte the ones
the original's runs used (the base game's own random atom types, which the recipes replace, and the pulse and tint phases differ;
no choice of the port's `-seed` and `-drawevery` reproduced the old base, the cause was not looked for), but each scenario plays out
the same in the port with either set: the state traces of sv-elements, sv-elements-action, sv-molecule, sv-bonus, sv-over and
sv-magnet agree on every name but the cursor, the meter's tick and countdown and the play time.

| Scenario | Save | What it reaches |
| --- | --- | --- |
| `sv-elements` | one element atom of each kind on the field, the bomb charged | the hint boxes that come up (the bonus molecules', the bomb's, the wind's), each element's double click (ghost, freeze, bolts, whirlwinds, shock wave) and the bomb |
| `sv-elements-quit` | the same, quit at once | the quit box's "save and quit" with element atoms on the field: their own part of a saved game |
| `sv-elements-action` | the same, in the action mode | `Meter::Hit` (0x4058a0) from the shock wave and the bolts |
| `sv-molecule` | three atoms of a type round a cell, the meter just short of full | the bomb's hint, an atom carried over the field (the route's arrows), a molecule that fills the meter, the level box, the radiation round of level 5 and its hint, the space key's pause, the quit box |
| `sv-options` | the same | the options box in a game: both sliders dragged, a click on the sound bar, the check boxes but full screen (which would change the machine's display mode), done |
| `sv-bonus` | the first bonus shape missing one atom | the bonus molecule, its effect and the element atom it makes |
| `sv-over` | a score above the table and a field full but for two cells | a move that lets the round come, a second move that fills the field, the game over, the name entry (a name typed, save), the end page and its main menu button |
| `sv-magnet` | an arrow set as levels 17, 33 and 38 set it | `Magnetise` (0x412a80), every sixth update |

**Loading is slower under `--coverage`.** The original loads in a thread and the scenario counts updates, so the update at which the
title's link answers a click depends on the load; with coverage on, a run missed the click at update 1510 and sat on the title for the
whole scenario. `cov.py` watches the title screen's pointer (0x4d00fc, deleted 47 updates after the click) and runs the whole scenario
again 3000 updates later (up to four times). The origrun files (`tools/origrun/atomica.cfg`, `patches/atomica.txt`) are git-ignored and
belong to each worktree; `setup_worktree.sh` does not copy them (copies are in the coordinator's notes folder, `atomica-origrun/`).

**Reached in the original with no definition in the port: 104 after the first twenty-three runs, 130 after all of them, 0 at the end.**
`tools/coverage.py` lists them; each was either put in a comment of the function that holds it, or left out on purpose.

- **Placed: 44.** The address went into the comment above the function that translates it inline or in another shape (effects'
  constructors and destructors, the dialogs' and widgets' handlers, the bonus effect's). Comment edits only, checked by a diff of
  the sources with the comments taken out; one agent did 41, this session the other three (0x415c90, 0x415d20, 0x416f00).
- **Left out on purpose: 86, as `none` lines in `games/atomica/coverage.txt`, each with its reason, and every one of them reached by
  the runs.** 34 scalar deleting destructors (the compiler makes them), 18 of MSVC 7.0's `std::string` helpers (the port uses
  `std::string`), 18 destructors that only set a vtable and call the GameObj destructor (the port's classes have none or inherit
  it), and 16 others: the WinMain glue (0x405ad0, 0x406220), the framework's stubs (ButtonListener, Graphics, an empty virtual),
  `PtrList`'s destructor (a `std::vector` frees itself), the bonus patterns' destructors (the port's are a static table), the
  `Palletize` calls of the loader (0x429670), the cell-to-pixel wrapper (0x4014a0), the compiler's vector constructor iterator
  (0x4191a0), the high score table's constructor and the app pointer only its entry box used, and `SpriteMgr`'s stored Graphics
  pointer, which nothing reads.
- **Slips found by reading the functions that had no counterpart again** (each a difference the identical-play pass could not have
  seen, since the bot's games never reach it):
  - `HighScoreDialog::Update` (0x41ca50): the original brings the two buttons and the name line to the front of the manager's widgets
    on every update; the port did not. Fixed (DialogScore.cpp).
  - The sine and cosine tables (0x421a90): the original makes two 360-entry float tables once, from a float pi (0x40490fda), and the
    effects index them; the port took `sin` of a double at each call, which differs in the last bit for some angles. The port now
    makes the same tables (Atom.cpp), checked against the original's memory entry by entry for eight angles.
  - A saved game's bonus molecule (`BonusMolecules`, +0xcc): the original saves the index of the shape asked for in its table of
    thirty; the port saved the element the shape makes, which several shapes share, so a loaded game could ask for another shape.
    It saves and loads the index now.
  - The main menu and the high score box load a sheet and cut sprites from it, and the original frees them in the destructors
    (0x4190b0, 0x41e690); the port never did, a leak of each showing of the menu. Freed now (Menu.cpp), the app's sprite manager
    cleared after its delete.
  - The pause count (0x405fa0) stops at 0 when it is decremented; the port's could go below it. Clamped (Grid.cpp).

  After all of these and the comment edits, `tools/check.sh atomica` passes with "baseline identical": the strategy and action
  games' state still matches the frozen baseline, so the fixes leave play unchanged.

**Defined in the port, never reached: 69 after the first twenty-three runs, 8 at the end.** The crafted saved games reached the rest,
among them each element atom's effect, `Meter::Hit` (0x4058a0), `Magnetise` (0x412a80), the bonus molecules' effect and the level
box. The eight left are candidates for taking out unneeded code (not removed now, and not read again against the export, since
nothing in the original reaches them):

- **`ConfirmDialog`** (0x421f90 the constructor, 0x4220b0 the destructor, 0x422100 `Resize`, 0x422170 `ButtonDepress`, 0x4221d0
  `Draw`), `AtomicaApp::ShowConfirm` (0x405c40) and `AtomicaApp::EndGame` (0x405cb0), with the case of `Board::ButtonDepress` that
  calls them (Board.cpp:669, the button with id 3) and the app's `mConfirmDialog`. The board creates no button with that id, so
  the "give up" question can never come up.
- **The sorted `PtrList::Add`** (0x425a10), with `mKey`, `mSortMode` and `KeyFunc` in Support.h. Nothing in the port gives a list a sort
  key (the port translates all the game code that the runs reached), and no run reached the original's sorted add.

**Never reached and not defined in the port: 36.** The registration screen and the code around it (0x425f70-0x427a60, 14 functions:
the "REGISTER ATOMICA" dialog, the "click here to get your registration code" button, the link to the register page) and 0x422de0,
which opens it from the options box; this code is out of the port with the rest of the registration code. Six are destructors (with
their scalar deleting versions) of objects the runs never delete, and fifteen are runtime glue (exception handlers of the string
helpers, a jump stub, deleting destructors of classes whose objects the game keeps to the exit).

**Open.** The candidates above. The crafted-save runs measure coverage only: the port's own states in those scenarios were not
compared with the original's (`compare.py --savefile` takes a saved game, four watched values to a run), so the element atoms,
the bonus molecules, the magnet and the game-over code are compared with the original only by being read against the export; those
comparisons are the next test of them. Nothing shared is needed.

**What it cost.** The 31 runs took a few minutes of machine time each, one game process at a time (some runs waited for room, since
other sessions' games kept the machine at its limit of three, and some ran again after a missed title click). One Sonnet agent placed
41 functions and wrote the first 64 reasons; this session read the export for the rest, found the slips, built the run tools and the
crafted saves, and checked the agent's work. Its tokens are in `tools/game_costs.py`'s figures for this session.
