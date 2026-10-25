# Noah's Ark Deluxe: behaviour notes (work in progress)

Addresses are virtual addresses in `re/noahsark/bin/WinNoah.exe` (version 1.1, linked 2002-10-17 with VC++ 6.0,
image base 0x400000, full RTTI, 64 classes, 2,709 functions of which 260 (0x401000-0x421dc0, 131 KB) are game
code). Offsets such as `app+0x3e0` are byte offsets in the original object; the port's classes name the fields.

## 1. Binary layout

- Game code 0x401000-0x421dc0: `Board` 0x401000-0x410bf0, DifficultyDialog (0x410c00 ctor), HighScoreDialog,
  HighScoreWidget, IntroWidget, NextLevelWidget, NoahApp, OptionsDialog (ctor 0x41c230), RecordScoreDialog,
  RegisterDialog, TitleScreen, TutorialDialog. Framework from 0x421dc0 (Widget), libraries after 0x450000.
- The framework is the SexyApp of the 2002 generation: one app class `SexyApp` (54 vtable slots, derived from
  `ButtonListener` with 4 slots), `Widget` without WidgetContainer (44 slots, sizeof 0x4c), Graphics with
  non-virtual methods, std::string of MSVC 6 (copy on write, refcount in the byte before the data).
  docs/FRAMEWORK_2004.md describes the 2004 successor; FRAMEWORK_2002.md records the differences found here.
- Data: `images/` (GIF/JPG/PNG, alpha in `_name.gif`), `data/*.txt` (ImageFont descriptors), `sounds/*.ogg` +
  `levelup.au`, `music/*.ogg`, `fmod.dll` (the original plays music through FMOD; the port does not).

## 2. Resource tables (Res.h)

83 images (table 0x494f40), 34 sounds (0x49508c), 9 font descriptors, 4 music tracks (loading, mainloop,
mainintro, gameover). Fonts (app+0x3e4..0x404): fontbig, fontmedium, fontsmall, colorlessfont, garamondfontmed,
garamondfontsmall, garamondfonttiny, scorefont, registerfont.

## 3. Application (NoahApp, vtable 0x4888ec, 54 slots)

(to be filled in as the translation goes)

## 4. Board (WinNoah.exe 0x401000-0x410bf0; object 0x1108 bytes, vtable 0x488368, ButtonListener at +0x4c)

Work notes while the translation is written (see Board.h for the final field names).

- A 9x9 grid of `Piece*` at +0x5c, index `x*9 + y` (x column, y row, y 0 = top). `FUN_0040e280(x)` is the cell's left
  pixel, `FUN_0040e2a0(y)` its top; a cell is 50x50 (0x32).
- Piece (0x68 bytes): +0 x, +4 y, +8/+c the cell it moves to, +0x10/+0x14 float pixel position, +0x18 type
  (0..12 animals, 0xd, 0xe empty, 0xf crate, 0x10 block), +0x1c alpha, +0x20/+0x24 pixel offset to the cell (a new piece
  starts at y offset -0x1db and falls), +0x28 fall state, +0x2c selected, +0x2d removed (animating out), +0x2e,
  +0x2f flash, +0x30 moving along a path (+0x3c its partner, +0x34 index into its path vector +0x48), +0x38,
  +0x59 match mark (puzzle), +0x5c blink frame.
- Second grid of 81 clones at +0x1c4 (FUN_004060d0 copies a piece): the solvability simulation of the level
  generator (FUN_00401e40) works on it. A row of 9 queued pieces at +0x1a0.
- Pairing rule: two pieces of the same type can be removed when a path of at most N straight segments joins them
  through empty cells (FUN_004036f0, depth-first with a cost bound; FUN_00403370 keeps the cheapest path in +0xe08).
  FUN_004026d0 / FUN_00402a70 are plain breadth-first reachability on the real / cloned grid (4-neighbour).
- State (+0x310): 0 start (Get Ready!), 1 playing, 2/4 level won / bonus, 3 game over, 5/6 no more moves, 7/8 puzzle
  swap, 9 hint.
- App game mode (+0x444): 1 action, 3 strategy, 4 puzzle, 5 stampede bonus.
