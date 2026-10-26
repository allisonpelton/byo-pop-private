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

Registered game: the port has no free-play counter or order dialogs (the title shows "thanks for registering").

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
