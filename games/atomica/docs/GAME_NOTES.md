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
| 0x4056b0 | `Meter` (0x4ae4a8): a small counter with a speed |
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
