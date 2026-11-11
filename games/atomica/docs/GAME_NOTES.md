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
must be cleared. The action mode (`SetupTimedLevel` 0x40f730) has no rounds: a clock (`mRoundTimer`, 300 to 0) adds atoms and the
meter drains; a molecule empties it, and it fills when it runs out. The field is lost when more than 71 atoms stand on it.

## 5. The board

The board draws the machine around the field and owns the meter, the atomic bomb (`mProgress` 0 to 48, `CheckBomb` 0x4322c0,
`StepBomb` 0x432360: it bursts atoms until its charge is used up), Atomicus, the effects list and the floating scores.
The level sign counts down from 0x163 (`mGetReadyAlpha`); the field is revealed by sparks (`Sparkle`, 0x41c5f0) that set the cells'
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
next target, the next minimum radius) are globals of the program, not of the game. A new game starts with `mSwirlsCatchUp`, which repeats the
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
at 0x14a the app's `mMenuDoorsOpen` closes the menu (the board's update does it). "Main menu" after a game lets the panel drop out of
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
| `state` | `STATE` with `screen` (title, menu, starting, gameover, board, hint, levelup, highscore-entry, options, quit, highscores), `mode` (strategy or action), `expert`, `update`, `paused`, `windowed`, `over`, and in a game `level`, `score`, `molecules`, `atoms`, `combo`, `meter`, `meterMax`, `levelOver`, `held`, `hints`; on the page after a game `level`, `score` and `molecules` of the game that ended. `over` is true when that page is up with its panel and the entry box (if the score earned one) is done |
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

**A candidate, for the user to decide (2026-11-11).** The original never removes `SAVEGAME.DAT`, so a saved game can be continued,
played to its end or left without saving, and continued again from the same point as often as the player likes; the menu offers it
until another game is saved over it. The port does the same now (`IDENTICAL_PLAY.md`, "The saved game stays"; before, it emptied
the file when the game was continued, which no one had asked of it). A toggle of the fixes mod could make a continued game use its
file up. It is not made: whether a second chance from a saved game is a fault is the user's call.

## 11. Performance

The performance pass (2026-11-10 and 11). The port and the original were measured turn about over the same scripted play, the
port was profiled, and one change came of it: a ring of the tunnel is filled in one call (below). The machine was never quiet: one to
four games of other sessions ran during every measurement and 74 to 100% of the machine was in use, so the figures are for comparing the
two side by side, run by run. The original runs as a copy under origrun with its silent sound stand-in; nothing else is changed in it.

**How.** `tools/profile/perf.py` starts a game and samples its process every 0.1 s (working set, private bytes, processor time);
`tools/profile/pairs.sh` runs the original and the port turn about, one game process at a time. The scenario is
`tools/identical/s2.txt` cut at update 4,400: the title screen, the menu, and a strategy game that the bot's recorded clicks play
from update 2,310. "The stretch" is its last 2,000 updates, 20 seconds of play at the game's 100 updates a second. In real time the
port runs without `-fast` and the original on origrun's real clock, its `at` lines following the game's own update counter (the
app's +0x3e4, 0x4d0044). A run of the original counts only if its title screen went (`game_started`, the pointer at 0x4d00fc):
the original loads in a thread in real time, so on a busy machine the link is sometimes not up at the click's update, and the run
then sits on the title, where an update costs almost nothing.

**The figure of 2026-11-08 was wrong.** This section used to say that the original needed 2.5 s of processor time from launch to
update 4,400 without waiting and the port 1.7 s. The original's runs behind that figure had missed the title's click (the check
above did not exist): of six such runs now, three missed it and used 0.2 to 0.8 s for the stretch, and the three that played used
19 to 28 s.

**In real time** (six pairs: three from 23:43 on 2026-11-10 and three from 00:07 on 2026-11-11):

| | Original | Port |
| --- | --- | --- |
| Processor time in the stretch | 12.9, 19.8, 28.2, 16.1, 20.3 and 17.0 s | 5.8, 5.4, 5.1, 5.0, 4.5 and 5.2 s |
| Updates a second it held in the stretch | 85, 44, 30, 54, 52 and 67 | 98.1, 97.5, 98.8, 98.6, 99.5 and 98.3 |
| Processor time, whole run | 17.7 to 35.4 s in 53 to 104 s | 7.0 to 8.7 s in 45 to 48 s |
| Peak private bytes / working set | 65 to 66 / 78 to 79 MB | 163 to 168 / 161 to 173 MB |
| Private bytes / working set at the title's play link | 14.4 to 15.3 / 26 MB | 90 / 90 MB |
| Processor time to the play link | 0.34 to 0.58 s | 1.14 to 1.25 s (3.1 in one run) |
| Launch to the play link | 0.8 to 8.8 s (18 runs) | 1.7 to 9.4 s (6 runs) |

The port plays the stretch on about a quarter of a core and holds the game's pace. The original took between two thirds of a core
and all of it, and did not hold 100 updates a second on this machine as it was: the busier the machine, the more processor time
each of its updates cost and the further it fell behind (30 updates a second in the worst run). Why the original costs that much was
not looked for. Loading takes the two about as long, and the spread between runs is far larger than any difference (on 2026-11-08,
a calmer moment, the medians were 1.07 s for the original and 1.20 s for the port); the port uses about 0.7 s more processor time
to get there, since it draws the atoms' frames at every start where the original reads them from `images\cached\Atoms.WAD`
(section 3). Memory is the port's weak figure, 2.5 times the original's in play and six times at the title; where it goes is below.

**Where the port's time goes.** Bejeweled's `cpuprof.py` on the bot's strategy and action games under `-fast` (20,000 updates, about
2,000 frames), because a test run is where the port's speed counts: system calls and the graphics driver took 40 to 60% of the cycles
and SDL 9 to 17%; of the game's own code only two functions stood out, `Swirl::DrawTriangle` (4 to 13%) and the framework's
`MemoryImage::NormalBlt` (4 to 7%).

- *The tunnel.* Counted with a build made for it: a frame draws about 600 triangles (15 rings of 40), walks 38,000 rows for them and
  fills 20,900 of those, each a rectangle one pixel high. The port filled each triangle's rows with a call of their own, from a
  list it allocated for each triangle.
- *The professor's monitor.* `NormalBlt` is `Atomicus::Draw`: the head, the coat and the mouth are drawn into the monitor's buffer
  on every frame and every other row is dimmed at random, as in the original, so no frame can be kept. It is four blits of about
  56,000 pixels in all, at about 7 cycles a pixel; nothing was found to ask of the framework.

**What was changed: a ring's rows are filled in one call** (`Swirl::Draw`, `Swirl::DrawTriangle`). The triangles of a ring add their
rows to one list, kept between frames, and the ring fills it once: the same rows, in the same order, in the same colour, in 15 calls
a frame where there were 600, and no allocation. Over three rounds of the two bot games, the old build and the new one turn about,
the cycles of a game fell from 18.2, 20.4 and 18.9 G to 17.4, 14.6 and 16.2 G in the strategy game (the medians: 14% fewer) and from
22.8, 21.0 and 19.3 G to 17.1, 15.2 and 16.1 G in the action game (23% fewer). What the player sees is unchanged: three scripted games
(`s2`, `a1` and `g5`, a screenshot every 40 updates, 422 in all) give the same files in the build from before the change and in the
one with it, and the same state traces, and `tools/check.sh atomica` matches the frozen baseline. The web build
with the change, run under Node, draws `p2-s2` as it did before (28 of its 68 pictures differ visibly from the native port's
under `-fast` and 40 only faintly, the count `IDENTICAL_PLAY.md` records for the shared loop's difference between `-fast` and real
time) and `p2-symbols` as the native port does (four pictures, faint differences only).

**Where the port's memory goes.** A copy of Alchemy's `memreport` command, run on the scenario and then dropped (the framework is
getting one for every game), gave the heap: 61 MB at the title and 63 MB in play, of which 43.7 MB are the pixels of 1,290 images at
4 bytes a pixel (83 read from files, 20.2 MB; 1,207 made by the game, 23.5 MB, most of them the atoms' frames of 50 by 50) and
6.5 MB the samples of 49 sounds. The images' pixels stay after the driver has its textures (8 at the title, 252 in play, about
13.5 MB of texels), a matter of the framework's (Bejeweled's `PROFILE.md`, "Pixels after upload"). The other 100 MB of a game are
outside the heaps, in the graphics driver: the same scenario peaks at 87 MB of private bytes under SDL's software renderer, 162 MB
under OpenGL, 170 MB under Direct3D 9 and 163 to 182 MB under Direct3D 11, which SDL picks here (296 MB in one run without
waiting). More than half of that is the tunnel: a build with the fill of its rows left out peaks at 112 MB against 170 MB, in real
time and without waiting alike, and uses 2.9 s of processor time in the stretch against 4.1 s. So 21,000 one-pixel rectangles a
frame cost the port about 58 MB and more than a quarter of its processor time in play.

**Open.** (1) The rows could be fewer: the rows of a ring that touch on a line could go as one rectangle (a ring's colour is
opaque), perhaps half as many. It was not done: at a window scale that is not a short binary fraction the right edge of a joined
row can round another way than the row it replaces, so the picture could differ by a pixel there, and that needs a proof this pass
did not make. (2) How the framework hands rows to SDL (`DDInterface::FillScanLines`, one rectangle a row) is shared code; the
measurement went to the coordinator. (3) The pixels kept after upload are the framework's, as for every game. (4) The web build's
memory and frame time were not measured; its memory waits for the framework's `memreport`.

**To repeat.** `bash games/atomica/tools/profile/pairs.sh <out dir> [pairs]` from the worktree root, after `tools/origrun/build.sh`
(the original needs origrun's git-ignored `atomica.cfg` and `patches/atomica.txt`); `BEFORE_EXE=<name>` adds a run of another
build of the port. The raw samples were deleted once these figures were written.

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
box. The eight left were candidates for taking out unneeded code (not read again against the export, since nothing in the original
reaches them; they were taken out on 2026-11-11, section 13):

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

## 13. Code the port does not need (2026-11-11)

The last finishing item before the readability pass (`docs/COORDINATION.md`: what code need not be present in the port).
Registration and the buttons that lead out were already out (`IDENTICAL_PLAY.md`, "Registration and the buttons that lead out").
This survey looked for what no path of the game reaches or needs, by seven scans of `src/` (`tools/unused.py` does the first
four; the test hooks, which the tests need, stay apart):

1. **The coverage check's list.** The eight functions the port defined and no run of the original reached (section 12).
2. **Functions.** Every function defined in the native build's object files against every relocation in all of them
   (`unused.py functions`): 80 of 586 had no reference. Most are the two versions of a constructor or destructor; every other name
   was searched in the source, and all but one are called, inlined by the compiler.
3. **Compiler warnings.** Every source compiled again with `-Wall -Wextra -Wunused -Wunused-member-function
   -Wunused-private-field -Wunused-template -Wunused-macros -Wunreachable-code` (`unused.py warnings`): one unused function. Two
   warnings about the order of a constructor's initialisers are the readability pass's.
4. **Fields and constants.** Every member the sources name at most twice (`unused.py members`: its declaration and one other
   place), each then looked up in the export by its offset; every enumerator, `#define` and constant named only once
   (`unused.py constants`: none).
5. **Pictures, fonts and sounds.** Each of the app's 153 picture, font and sound members by whether anything but the loader names
   it; for those nothing names, the original's code by the member's offset and by its absolute address (the app is static at
   0x4cfc60), and for an access by arithmetic near it (an index times four added to an offset of that range).
6. **Windows leftovers, switched-off code, notes to self.** `_WIN32` once (the search for the retail install folder in `main.cpp`,
   which a Windows player of the unpacked game needs), no `#if 0`, no `TODO`.
7. **The slip the fifth scan found.** See below: it was not unneeded code but a missing text.

| What | Why it is not needed | Verdict |
|---|---|---|
| `ConfirmDialog` (0x421f90, 0x4220b0, 0x422100, 0x422170, 0x4221d0), `AtomicaApp::ShowConfirm` (0x405c40) and `EndGame` (0x405cb0), the board's case for button 3 in `ButtonDepress` (0x433c40), `mConfirmDialog` (+0x3e94), and the screen name `confirm` of the `state` command | nothing opens the box: `ShowConfirm`'s only caller is the board's handler for a button with id 3, and the original's board creates three buttons, with ids 0, 2 and 1 (0x432c20) | removed; a comment at the case keeps the id |
| the give-up flag `mB3ea0` (+0x3ea0) with its two reads, in `ShowGameOver` (0x406200: the menu's main page instead of the end page) and in the game over's voice (0x412cd0) | only `EndGame` sets it | removed; comments at both places say what the original tests |
| the sorted `PtrList::Add` (0x425a10) with the key and the mode (+0x10, +0x14) | no list of the game is given a key: nothing in the port sets one, and no run reached 0x425a10 | removed; `Add` appends |
| `SpriteMgr::Wrap` (0x429bc0) | no caller in the port: the original's one caller is the loader's blend of the spinning atoms, where the port draws into a blank sprite | removed; the address is in `LoadAtoms`' comment |
| `DrawCentered` in `BoardDraw.cpp` | a helper of the port's own with no caller | removed |
| `Atom::mFlyPhase` (+0xa4) | the bonus molecule's effect keeps the shrinking frame in its own piece; nothing reads the atom's, and it is not saved | removed |
| `PathFinder::mD38` and `mB3c` (+0x38, +0x3c) | set by 0x424880 and never read, in the port and in the export | removed |
| `GameDialog::mCentered` (+0xa8) | the constructor stores its flag there and nothing reads it (the constructor's own test of the flag stays) | removed |
| `SpriteMgr::mGraphics` (+0x20) | the port never set it; the original's setter (0x429960) is left out, nothing reads it | removed |
| `TitleScreen::mHighest` (+0x74) | the trial's count of games played, which the original's title shows as free plays left (0x42fe90, 0x430450): registration code that had stayed behind as a field | removed |

**The colour-blind option drew nothing.** The fifth scan found that nothing in the port names the stencil font, while the original's
`Atom::Draw` (0x4037f0) writes each plain atom's chemical symbol with it when the option is on. Two faults stood behind that. The
port wrote the symbols with the medium monitor font (the app's +0x47c for +0x474); and it wrote nothing at all, here and on the bonus
molecule's sign (0x415830), because the twelve symbols are texts of one or two letters, too short for the string list that
`make_gametext.py` builds its table from, so `GT()` gave an empty text for each. Both are fixed: the addresses of the symbols (the
table at 0x4cb070) are in `tools/extra_strings.txt`, and the atoms use the stencil font. `tools/identical/p2-symbols.txt` turns the
option on in a game; its comparison with the original is in `IDENTICAL_PLAY.md` ("The colour-blind option").

**A button's label stood a pixel to the left.** The same comparison showed "Credits" in the options box one pixel left of the
original's. `GameDialog::DrawButton` (0x416c30) halved the difference of the button's width and the text's, where the original
halves each by itself, so a text of an odd width came out a pixel left in every box. Fixed.

**Looked at and kept.**
- *Pictures, fonts and sounds: nothing was taken out.* Two things are loaded and never used by the original either: the level font
  (+0x46c) and the two top-lights cells with their sheet (+0x26ec to +0x26f4). Only the loader (0x406f90) names them, by offset or by
  address, and no code reaches them by arithmetic: no index times four is added to an offset between 0x450 and 0x47f or between
  0x26d0 and 0x271f anywhere outside the loader. They stay all the same: the loading bar counts them among its steps, and what they
  hold is two small pictures. Every sheet member is named only by the loader because the game draws its cells, which other code names.
- `Board::mStat1c8` is one of seven counters the saved game writes in one block.
- The test hooks (`src/testhooks`), the fixes mod (`Fixes.cpp`) and the site's hooks (`SiteProfiles.cpp`) are the port's own and in use.

**Proof.** After the removals `tools/check.sh build` passes and `tools/check.sh atomica` matches the frozen baseline (the strategy
and action games, the rerun). The pictures were checked with a control: three scripted games (`s2`, `a1`, `g5`, a screenshot every
40 updates) in the build from before this pass and in the build with the removals give the same 422 files and the same state
traces. The coverage report on the 31 runs of section 12 now has no function reached without a counterpart and none defined and
never reached; the port defines 323 of the game's 453 functions, and 44 are neither reached nor defined (the 36 of section 12 and
these eight).


## 14. Readability pass (2026-11-11)

The pass to `docs/CODE_STANDARDS.md`, after the code the port does not need was out (section 13). No Mac build of Atomica exists
(`re/atomica` has none), so every name comes from how the code uses the thing, and the Mac-name step has nothing to give.

**Names.** The board, the grid, the atoms and the app had 84 members that carried a letter and an offset (`mI148`, `mF8c`,
`mStat374`, `mB4b3`, `mM30c`); each now says what it holds, with a comment on its line: `mBombTimer`, `mFreezeSwirlTimer`,
`mLevelUpdates` (the level's totals, which the board takes at the end of the level), `mCheckMolecules`, `mReadyElementX`. The
fields the original writes and nothing reads (`mSavedZero`, `mStatSavedZero` and `mElementSavedZero`, three zeros the saved
game keeps; `mPickedCol` and `mPickedRow`; `mElementReady`; `mLevelSetUp`) keep a name and say in their comment that nothing reads
them, since the save's layout depends on some of them. The tails that several levels of `Grid::StartLevel` jump to were
`LAB_00410bd9` and the like and are `chancesThree`, `chromiumSixArrow`, `radiationCells`, `arrowLeft`, `arrowDown` and
`biohazardFour`. `Wrap360` is `WrapDegrees`, the `FILE*` of the Save and Load functions is `theFile`, and three locals without the
framework's prefix (`a`, `b`, `h`, `anyFlown`) have one. The one-letter names that `namecheck.py` flagged (the autoplay's, the
state trace's and `GameText.cpp`'s loops) and the grid pointers called `g`, which is the graphics, are `aState`, `aTable`,
`aBoard`, `aGrid`, `theGrid`, `aLipEntry` and `aTextEntry`. Text that files and logs carry keeps its spelling: the state trace's
keys (`f8c`, `i90`, `m31c` and the others, which `tools/identical/compare.py` reads) and the `state` command's field names.

**Addresses.** Thirty-five functions had neither an address nor the port marker. Where the original has the function it now
names it: `LoadingThreadCompleted` (0x4061e0, a jump to the base's), `ReadFromRegistry` (0x406dc0), `HighScoreDialog::EditWidgetText`
(0x41cbd0, the name line's listener: a jump to the box's `ButtonDepress` with 2, which is why Enter does what "okay" does),
`Grid::MouseDrag` (0x40c530, a jump to `MouseMove`), `TitleScreen::RemovedFromManager` (0x42fb70), `Vortex::~Vortex` (0x431270),
`SpriteMgr::~SpriteMgr` (0x429890) and `EarthBolt::Draw` (0x405cf0, the empty function that every empty slot of the original
points to). `Grid::Draw` is the same empty function, and the address that stood on it, 0x413880, is the field's draw that the
board's draw calls: it is `Grid::DrawField` now. The helpers the original has in line say `port:`, and the test hooks, the fixes
mod and the game texts carry the marker in their first comment. `namecheck.py` reports no finding in the 51 files.

**Code found on the way.** `ScoreTable::Place` and `GameDialog::Close` had no caller (the survey of section 13 had looked at
members, constants and warnings, and a function the compiler does not warn about slipped through), and the path finder's search
counter (+0x300) is counted at every step of the search and reset at its start, in the original as in the port, and read nowhere:
the three are out. `Atom::mPathNode` (+0x38) is the same kind of field, but the loop that sets it also walks the path's
cursor to its end, so it stays, with a comment that nothing reads it.

**The coverage report after the addresses.** The functions that now carry an address count as the port's, so the report on the 31
runs of section 12 lists the port's definitions of five that no run of the original reached: `Grid::MouseDrag` (0x40c530), the
sprite manager's destructors (0x429890 and the deleting one, 0x429c30) and the vortex's (0x431270, 0x4314e0). None was compared
by a run (the runs never drag a held atom's way across the field with a button down, and neither the manager nor a vortex is
destroyed before the game exits), so each was read again against the export: `MouseDrag` is a jump to the `MouseMove` slot with the
same arguments, as the port's is; the manager's destructor deletes each of its sprites through their own destructors and then
frees its list, and the vortex's frees each particle and its list, as the port's do. The report has no function reached without a
counterpart; the port defines 329 of the game's 453 functions, 86 are left out on purpose and 39 are neither reached nor defined
(the 44 of section 13, less these five).

**Numbers.** The field's size was written as 9, 8, 7, 0x32 and 0x19 in a score of places; they are `Grid::COLS`, `ROWS` and `CELL`
(the middle of a cell is `CELL / 2`). A cell's reveal state is `CELL_REVEALED` (0xff, shown in full) and `CELL_ATOM_SHOWN` (0x96,
from which its atom is drawn), and the 0x120 bytes of it that the saved game holds are `sizeof(mCellState)`. The 360 degrees of
an angle are `FULL_TURN`, in the angle tables, the wraps and the random angles. A number that stands once with a plain meaning (a
draw offset, a colour, an alpha) stays as it is.

**The fields `tools/splitfields.py` finds.** No member is declared twice under two names, none is read and never assigned, and none
is assigned and read nowhere without an offset. Its 22 export findings are all reads the translation makes another way:
- the app's title screen (+0x49c) is read by the loading thread's progress updates, which the port makes in `PumpLoading`; the
  atoms' shrunk copies (+0x5a8) are made in `LoadAtoms`, the part of the loader (0x406f90) that the port keeps apart;
- the board's +0xc0, read by its draw, is the detail setting: the constructor sets it to 1 and nothing else writes it, the draw
  clears the screen first when it is not 1 (the comment in `Board::Draw` says so), and the swirls read it as their step;
- the seven counters of the end page (+0x1c4 to +0x1dc) are saved and loaded as one block, which the translation writes as one;
- `GameDialog::mApp` (+0x90) is read at the end of `AddedToManager` and of `RemovedFromManager` for a call of the app's virtual
  functions 51 and 52, which are the empty function (0x405b00) in this app, so the port leaves the calls out;
- the grid's 17 reads in 0x413880 were the field's draw under the wrong address (above); and the one read of the main menu's +0x84
  in 0x41a010 is a virtual call at that offset, not the field.
Its seven write-only members with an offset are fields the original writes and nothing reads (`mLevelFont` and `mTopLights`, loaded
and unused in the original too: section 13; `mPickedCol` and `mPickedRow`; the atom's path node; the path finder's searches,
now out; and Dr. Atomicus's blink counter, which is read only by its own decrement and test).

**Proof.** After each step every object of the native build was compared with the one before it (`tools/objsnap.py`, with the
build's names taken out): the 84 renames, the addresses and comments, the named numbers and the `FILE*` renames leave the 36 objects,
716 functions in all, as they were, with their data. Only the removals above differ: the two functions without a caller and the
search counter (its stores, and the offsets of the path finder's later members). `tools/check.sh atomica` matches the frozen
baseline ("baseline identical").
