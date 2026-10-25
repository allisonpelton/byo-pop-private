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
