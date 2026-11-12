# Dynomite Deluxe: notes on the game code (work in progress)

Addresses are those of the game image carved from the 2006 wrapper (`re/dynomite/bin/DynomiteGame.exe`). Reading
aids: `tools/decread.py <addr>` prints a function of our Ghidra export readably (field accesses as `this.i40` for an
int at +0x40, `.p` pointers, `.b` bytes, `.f` floats, `.d` untyped dwords); `tools/loadtable.py` folds the loader into
one line per resource.

## 1. Classes

The game classes have no RTTI. Their vtables were found by `tools/vtscan.py`; the classes are named by the texts they
use and by the link order, which follows the source files' names (the comments of `properties/default.xml` list them).
The storer is a function that writes the vtable: a constructor or the destructor (the TitleScreen's 0x4388d0 is its
destructor; its constructor is 0x438b20).

| Vtable | Storer | Class | Base | Notes |
|---|---|---|---|---|
| 0x4f7470 | 0x404030 | `Dynomite` | Widget (62 slots) | the board: eggs, slingshot, the game rules |
| 0x4f9a10 | 0x408e40 | `DynomiteFossil` | Dynomite | the Fossil (adventure) mode |
| 0x4f9ce0 | 0x40ca40 | `DynomitePanic` | Dynomite | the endless mode ("Panic"; whirleys) |
| 0x4f9e68 | 0x40ea50 | `DynomiteStomped` | Dynomite | rounds of a puzzle pack (`.EGG`) |
| 0x4fa100 | 0x411ff0 | `DynomiteTimeTrial` | Dynomite | time trial |
| 0x4fa298 | 0x413d40 | `EnterNameDialog` | Dialog | |
| 0x4fa410 | 0x419320 | `FossilizedApp` | SexyApp | the application |
| 0x4fb4d8 | 0x420dc0 | `GetSaveNameDialog` | Dialog | |
| 0x4fb688 | 0x421b40 | `HighScoreDialog` | Dialog | the tables per mode |
| 0x4fb7f0 | 0x422410 | (high scores) | Dialog | reads and writes `STOMPED.HS`, `ENDLESS.HS`, `FOSSIL.HS`, `TIMETRIAL.HS` |
| 0x4fb970 | 0x424e90 | `IntroScreen` | Widget | |
| 0x4fc3e8 | 0x42ea10 | ? | Dialog | between Museum and OptionsDialog in link order |
| 0x4fc4c0 | 0x42eca0 | `OptionsDialog` | Dialog | |
| 0x4fc744 | 0x430c90 | `PuzzleListWidget` | Widget | |
| 0x4fc890 | 0x432640 | `RegisterDialog` | Dialog | not ported (registration) |
| 0x4fca40 | 0x433420 | `SaveGameDialog` | Dialog | |
| 0x4fcb98 | 0x434dc0 | ? (`SideBar`) | Widget | |
| 0x4fcca0 | 0x437520 | a button | ButtonWidget | `mId` +0x58, listener +0x90 |
| 0x4fcd98 | 0x4388d0 | `TitleScreen` | Widget, ButtonListener (0x4fcd88) | 0xa8 bytes; ported (`src/TitleScreen.cpp`) |
| 0x4fd068 | 0x43aad0 | ? (`Tutor`) | Widget (47 slots) | |
| 0x4ff5e0 | 0x441020 | `UpsellDialog` | Dialog | not ported (purchase pitch) |

## 2. The sprite toolkit (0x435b00-0x437500)

The game draws through its own small classes on top of the framework's images:

- A current `Graphics*` (0x5304dc, set by 0x4360f0, read by 0x436100) and a drawing offset (0x5304e0/e4: set 0x436110,
  get 0x436130, add 0x436150) that every Draw below adds.
- `Pic` (vtable 0x4fcc88, 0x18 bytes): +4 the image (a DDImage), +8 the app, +0xc width, +0x10 height, +0x14 "shared"
  (the destructor does not delete the image). Made from a file (0x437210 `Pic(app, path, palletize)`, loading through
  `SexyApp::GetImage` 0x480930 and palletizing with 0x457f00), from pixels (0x435ee0), as a copy (0x435fc0), or blank
  (0x4368d0). `Draw(x, y)` 0x436170, a line of copies 0x436a40 (Bresenham, centred), a stretched blit 0x436b50,
  `MultiplyAlpha(pic, x, y)` 0x4361a0 (the colour channels scaled by the other pic's alpha), an outline/glow copy
  0x436260.
- `Cell` (vtable 0x4fcc8c, 0x20 bytes): a source rectangle (+4 x, +8 y, +0xc w, +0x10 h) of a Pic's image (+0x1c).
  0x436cb0 `Cell(pic, cellW, cellH, index)` cuts cell `index` of a grid; 0x436d40 `Rect(pic, x1, y1, x2, y2)` an
  inclusive rectangle (+0x14 and +0x18 are its width and height plus one). `Draw(x, y)` 0x4365a0, a part of it
  0x436d90, stretched 0x436df0.
- `PicFont` (vtable 0x4fcc90): a Pic whose glyphs are cut by `0x436f70` from a strip (each glyph runs to the next
  column with no opaque pixel), +0x18 256 Cell pointers, +0x418 256 advance widths, +0x818 the width of a space.
  Constructor 0x437250 `(app, path, charset, spaceWidth, palletize)`; `Draw(text, x, y)` 0x436670, `Width(text)`
  0x4366d0, 0x436650 adds a value to every advance.
- `MaskedPic` (vtable 0x4fcc94): a Pic and, at +0x18, the Pic of the same name with `_` appended; 0x436800 copies the
  mask's colour into the alpha. Constructor 0x4372e0, copy 0x436720. The fossils are these.
- `Sound` (vtable 0x4fcc5c, 0xc bytes): +4 the app, +8 its sample id (a counter at 0x5304d8). The constructor 0x435b50
  loads `sounds\<name>` through the sound manager when sound is on (app +0x10b8); `Play` 0x435b10 plays it (app slot
  37) under the same test.
- 0x4fcc78 (0x24 bytes, 0x435cd0): a frame-rate governor that halves or raises a factor by the measured speed.

## 3. The application (`FossilizedApp`)

`Init` (0x41f820) calls the base `Init` and then reads the dialog buttons' texts (`DIALOG_BUTTON_YES` ...) through
`GetString` (0x489100) into four global strings, loads the title and the green bar, the three white ImageFonts
(`data\whitefont.txt`, `whitefontsmall.txt`, `WhiteFont2.txt`), the unregistered banner, two sounds, the dialog
pictures (`images\dialog`, `dbutton`, `waitbar`), makes the `TitleScreen` and adds it, loads the two modules
(music ids 0, 2 and 4 are `amazonas.it`, 1 is `omniphilia.it`) and starts music 2 at order 0x16, and loads the four
cursors (`cursor1`, `pointer2`, `overhand`, `typer`). The loading (`InitHook`, slot 15, 0x419c30) makes 394 steps
(+0x4e4) of Pics, Cells, PicFonts, MaskedPics and Sounds into fields of the app; `tools/loadtable.py` lists them.

The constructor is 0x4198b0 (0x419320 is the destructor): product "Dynomite", title "Dynomite Deluxe", version
"2.71" (+0x374), registry key `PopCap\Dynomite`, 640 x 480, music volume 0.5 (+0x68) and sound volume 0.85 (+0x70);
it makes the angle table (0x42bb50, 0xb48 bytes: sin and -cos of each whole degree at +4 and +0x5a4) at +0x5d4, seeds the MTRand at +0x5d8 with
`GetTickCount()` and the game's own generator at +0xf9c (class 0x431d30, seeded by 0x431d50 with `GetTickCount() >> 1`),
and sets +0x1084 = 1, +0x1088 = 0, +0x108c = 2 (settings read later), +0x1094, +0x1aac, +0x10b4, +0x10ec, +0x1830 to 0.

Other app fields seen so far: +0x10b8 sound initialised (the sound manager's slot 1), +0x10d8-0x10e4 the four cursor
Pics, +0x19d4/0x19d8/0x19dc the three white ImageFonts, +0x19e0 the TitleScreen, +0x5bc a list of every image loaded
through `GetImage` (0x419ad0), +0x2cc the WidgetManager, +0x314 the music interface, +0x490 the sound manager.
App slot 19 (+0x4c) is `Popup(string)`, slot 37 (+0x94) `PlaySample(id)`.

## 3a. The main menu (`IntroScreen`, vtable 0x4fb970, 0x284 bytes)

Constructor 0x424e90 (destructor 0x425570; 0x424970 frees the saved-game slots). Bases: Widget, ButtonListener at
+0x58 (vtable 0x4fb95c; ButtonDepress 0x424740), and the one-slot interface at +0x5c (0x4fb958). Fields: +0x60 the
app, +0x68, three arrays of 8 at +0x70/+0x90/+0xb0 (per saved-game slot: two objects deleted by their destructors
and a widget at +0xb0+4i), +0xd0 the slot count, +0xd4 the chosen slot (-1), +0x238 (-1), the CellButtons +0x23c
(id 10), +0x240 (11), +0x244 (12) of page 0, +0x248/+0x24c/+0x250 (ids 0-2, skill), +0x254..+0x260 (ids 3-6, the
modes), +0x264 (id 7, pictures app+0x12bc/0x12c0, or 0x12cc/0x12d0 on pages 2, 3, 6) and +0x268 (id 9, app+0x12c4/
0x12c8); +0x26c a ScrollbarWidget-like 0xb0-byte widget (0x47f610, id 1), +0x270 a 0x194-byte list widget (0x4318f0,
the puzzle list), +0x278 a 0x1c-byte object (0x422d90(app, this, 1 or 3); its slot 2 runs in Update), +0x27c the
page (0 start: buttons 10-12; 1 the four modes; 2 skill 1-3, disabled by the bits of app+0x1218; 3 the saved games
"SLOT%d.SAV" (0x424a20); 5 the puzzle list; 6 the fossil mode's two skills), +0x280. The global 0x52b320 (cleared
by the destructor) skips the music change and passes 1 instead of 3 to 0x422d90. The constructor fades out songs 0
and 1, sets song 2's volume to 0 and fades it in from order 0x16. ButtonDepress: ids 10-12 pick +0x10 of the
listener (+0x68 of the menu), ids 0-2 the skill (app+0x1084, or app+0x1088 on page 6), ids 3-6 the mode (app+0x108c
1-4); the same choice again plays egglands and acts as id 7, a new one plays click. Draw (0x425790) is 1851
decompiled lines.

## 4. How the port is being written

`src/Sprite.h/.cpp` is the toolkit of section 2. The app's resource fields and the loading are drafted by
`tools/genres.py` from the folded loader (`decread.py 419c30 | loadtable.py > re/dynomite/inithook.tbl`; then
`genres.py re/dynomite/inithook.tbl`): one field per resource, named after its file with the original's offset in a
comment (a translated function that reads app+0x1298 finds its field by grepping the offset), and `// RAW:` lines for
the parts loadtable.py does not fold (arrays filled with one picture, the PicFonts, the fossils and their copies,
the masks multiplied into the egg sheets), which are translated by hand from `decread.py 419c30`.

State (2026-10-26, evening): the native build runs (`TARGET=dynomite tools/build.sh native`; run it under rundesk).
Translated and checked on screen: the app's Init, the loader (`src/AppLoad.cpp`), `GetGameImage`, the dialog
(`NewDialog` 0x418a00 on `Dialog2004`), the title screen (`src/TitleScreen.cpp`, 0x438b20), the side bar
(`src/SideBar.cpp`, 0x435420) with its `CellButton`s (0x437520) and the small Inchy (`src/InchySmall.cpp`), and the
game's pointer list (`src/PtrList.h`, 0x430ea0). Then the main menu (`src/IntroScreen.cpp`, 0x424e90) with its
pages (start, modes, skill, fossil kind, saved games, puzzle packs), the large Inchy (`src/MenuTalker.cpp`,
0x422d90), the pack list (`src/PuzzleListWidget.cpp`, 0x4318f0, reading DEFAULT.EGG and STOMPED\*.EGG) and the
binary file (`src/BinFile.cpp`, 0x414a00; reads only so far). The app's +0x1084 is the skill, +0x1088 the fossil
challenge's kind, +0x1094 is set for the tutorial (start choice 2 calls 0x419040). `src/Dynomite.h/.cpp` is a stub with the board fields known so far
(constructor 0x404030: lists at +0xb8, +0xd0, +0xe8, +0x10c, a `BinFile` at +0x164 (0x414a00), the pause count +0x150,
the dynamite +0x154).

The board's `Init` (slot 46, 0x407ac0: app, skill) reads the string list `gScoreNames` (0x48c0c0) into a global
(0x5304a4), copies PLAY_THIS_PUZZLE_AGAIN to +0x208, sets +0x78 to the skill and +0x68/+0x6c/+0x70 to 306/153/250,
and makes five objects, none with more than a virtual destructor (their methods are plain functions beside their
constructors): +0x5c a 0xab0-byte one (0x401050, vtable 0x4f73ac, lists at +0xa74 and +0xa8c; its code is
0x401000-0x4034b0, with small classes 0x4f73e4, 0x4f7420, 0x4f742c and 0x4f7444 up to 0x404030), +0x8c a 0x46e8-byte
one (0x415770, vtable 0x4fa408, lists at +0x466c, +0x4694, +0x46b8; destructor 0x417420), +0x60 a 0x5c-byte one
(0x431fc0, vtable 0x4fc828), +0x84 a 0x3c-byte one (0x437aa0, vtable 0x4fcd60) and +0x88 a 0x78-byte one (0x438400,
vtable 0x4fcd80); then it calls the small Inchy's 0x423010 with 100 and `SetActive(true)` (0x4047e0). `StartGame`
(0x418810) takes the menu down, takes the old board down (`SetActive(false)`, `SafeDeleteWidget`, without clearing
+0x19ec), news the mode's board, resizes it to (150, 0, 490, 480), calls `Init(app, +0x1084)` (`+0x1088` for the
fossil mode), adds it, puts it at the back, gives it the focus, keeps it at +0x19ec and calls the side bar's
`GameStarted`. The board's own slots: 43 0x404f30 (empty), 44 0x404c30, 45 0x404a70, 46 `Init`, 47 0x404660,
48 0x406d60, 49 and 52 and 61 return 0, 50 0x404f40, 51 `MakeDynamite` 0x405010, 53 0x405070, 54 0x406f70,
55 0x406160, 56 and 58 empty, 57 0x406040, 59 0x405140, 60 0x405590.

The eggs (`src/Egg.cpp`): the base (0x4155a0, 0x20 bytes) has its colour at +4 (the cell at app +0x13c0 plus
four times it: 0 is the bomb, 1 to 20 the egg sheet), a bonus picture at +8 (app +0x1830 plus four times it), the
bomb's spark frame at +0xc, the app, the board, and its middle at +0x18/+0x1c (floats). The launcher's egg is
0x401000; the egg in flight (0x414d10, 0x38 bytes) adds +0x20, its velocity +0x24/+0x28, +0x2c and its cell
+0x30/+0x34. Its `Update` (0x4150b0, not in the Ghidra export) feels the grid at eight points per step, up to ten
steps a frame; the grid's answer is a set of bits: 1 the left wall, 2 the right wall, 4 the ceiling, 8 an object
of the list at +0x4694, 16 an egg.

The grid (board +0x8c, 0x415770) is a hex grid of 33-pixel cells: two arrays of 15 x 100 four-byte cells at +0x14
and +0x2ef4, indexed column * 100 + row, the odd rows shifted right by 16 (0x415920 cell to pixel, 0x415890 pixel
to cell). 0x4158f0 turns screen x into grid x by adding board +0x6c minus +0x70 (153 - 250); 0x415950 is its
inverse. +4 (float) is the ceiling's offset and +8 (float) its row count (0x4159f0 sets +4 to -((+8 + 1) * 33 +
16)). `Probe` (0x4169c0) answers 1 left of grid x 9, 2 right of board +0x68 minus 23, 4 above row 1, then 8 for a
rectangle of the list at +0x4694 (fields +4..+0x10, flag +5 set unless the egg is a ghost) and 16 for an
occupied cell near the point (0x416550). +0x4664 is the board, +0x4668 the app, +0x10 starts at 4, +0x46b0 and
+0x46b4 at 200, +0x46e0 at 100.

`EggGrid::Break` (0x415a20, x and y the cell's corner, the cell's bits) fills the board's effect list (+0xd0):
40 confetti (0x415360) when the board's +0x12c has a bit of 0xe, a bonus item (0x403c30 then 0x403c60, at
(x+16, y+6), kind bits >> 9 & 0xf, with board slot 43's two outputs; sound app +0x1a20) when the cell has bits
0x1e00, then either a baby (0x403540/0x4035d0, rand % 3, sound app +0x1a18 or +0x1a1c, board +0x1d4 + 1, board
slot 44 (x+16, y+6, 25, 0, 1)) when the cell has bit 0x100, or a 0x24-byte effect (0x4424b0/0x4424e0, rand & 1);
then 2 to 11 sparks (0x442320/0x442350, rand % 360) and four 0x28-byte pieces (0x434b90/0x434cc0) at angles
-45, 45, 225 and 135 with the colour and their index. 0x403750 (vtable 0x4f7420, 0x2c bytes) is a power-up:
kinds 1 to 10 set a time (+0x14: 6000, 3000, 3000, 3000, 400, 33, 33, 150, 300, 450) and a flag bit (+0x28: 1 << (kind
- 1)); its update (0x4038d0) counts down, blinks for the last +0x18 frames (500, or 20 for kinds 8-10), and the bits
0x380 end in 40 confetti and board slot 44 with 5, 10 or 15.

The board's slot 43 (0x404f30, empty in the base) gives the centre of the power-ups' circle, where a bonus item
(type 8) flies in 20 frames before 0x4060c0 makes it a power-up 20 degrees on from the last (+0x124). Slot 44
(0x404c30) adds points: times the multiplier when positive, plus 3, 4, 6, 8, 10 or 15 for a chain (+0x14c) of 2 to
7, a fifth (at least 1) on skill 1 and double on skill 3, then ten times over and doubled, tripled or quadrupled by
the bonus bits 2, 4 and 8 of +0x12c (the popup's text "+N" gets "x2", "x3", "x4"); it adds to the side bar's score
(+0x68), makes a score popup (0x4346a0, type 6, on the list +0xe8) and moves +0x1c0 to the highest of the 20
thresholds at +0x170 the score has passed, setting +0x1c4 when it moved. The tutorial (app +0x1094) scores nothing.
The constructor (0x404030) gives the lists: +0xb8 the flying eggs, +0xd0 the effects, +0xe8 the popups, +0x10c the
power-ups; +0x164 is a BinFile; +0x240 the tick count at construction. The destructor fades out the music +0x238
and song 4, stops the rumble, grind and tick-tock sounds, and deletes the lists' items, the grid and the objects at
+0x5c (the launcher), +0x60, +0x84, +0x88, +0x144, +0x154, +0x1d8 and the widget +0x204 (those not translated yet
are TODO in Dynomite.cpp). The board's Draw (0x4044a0) draws the jungle back (or white while the dynamite's +0x10 is
set), the grid (0x417760), the rope from +0x70 - +0x6c to +0x6c + +0x70 at +0x74 + 33, the launcher (0x4033a0), the
flying eggs that have not crossed the middle, the launcher's 0x401940, the others, its 0x401a10, slots 47, 49 and
48, the +0x60 object (0x432090), slot 54 when +0x132 is set, and +0x144's slot 1.

An egg that lands (`FlyingEgg::Land`, 0x414d60) backs out along its path until its cell is free; a bomb there
blasts (0x418100: a big boom, and every egg within two cells broken, scored ten times over, then `DropLoose`),
anything else is placed (0x416590) and matched (0x417bd0). Placing an egg next to one of its colour that has no
stick of dynamite lays a stick (0x438270, the table at +0x2ef4 and the list +0x466c) and clears +0x46e4; if
+0x46e4 is still set after the match, the board's slot 56 hears the colour, and slot 61 follows every landing.
`Match` floods the colour's group through the copy at +0x1784 (marking 100); three or more break (`Break`, then
0x416800 empties the cell and booms its stick), score through slot 44 with the count of eggs that then fall
(`DropLoose` 0x416b80: an egg with both cells above it empty and no path to the top row, found by the search
object 0x430c90 in at most 400 steps, falls as effect type 1), and the board's slot 50 counts the chain (Inchy
calls it out, 0x423990, not translated). Colours from 18 up never match, from 50 up are the mode's pieces (the
board's slots 57 and 58 draw them falling and hear them land). A baby egg (0x4349d0, the list +0x46b8) sets a
random egg's bit 0x100 for 200 to 900 frames; breaking it then hatches a baby. 0x418020 pushes every row down
two and adds two random rows (0x416b00). The grid's update (0x4174c0) spawns baby eggs (one chance in +0x46b0
a frame while +0x46ac is set), slides the sticks, sets +0xc to the screen y of the lowest row and blinks a warning
(+0x4688, every 25 frames while the board's +0x149 is set) while that is past the board's line (+0x74), breaks the
eggs past the line when +0x46d1 is set (0x416e30), and counts the matchable eggs of the lowest ten rows into
+0x46e0. Its draw (0x417760) hides the rows past the line while the warning is on, shakes a baby egg by the C
library's rand (CrtRand; whether the 2003 framework seeds it is not checked yet), breaks an egg past the right
edge (unless +0x46e5) or in row 90 or below, and draws the ceiling's foot and, when +0x46d0 is set, the ceiling.
The next colour (0x4163e0) is one of those in the lowest six rows, by 50 tries of the app's second generator at
+0xf9c (0x431d30, an additive lagged Fibonacci generator of 55 30-bit numbers, seeded by the board), else any
colour up to the highest placed. The puzzle load 0x4171c0 makes colour 20 a picked colour. Still to translate in
the grid: the save and load (0x415f70, 0x416150). The launcher (Launcher.cpp; vtable 0x4f73ac has only
the destructor) holds the loaded egg at +0xc (Load 0x401210 makes one at (100, 100)) and fires it (0x4012a0:
the board's +0x15c and +0x160 take its colour, the rubber-shot sound, a flying egg with the angle's unit vector
as its step, and a recoil of 20 pixels the other way at +0x28/+0x2c); 0x4013f0 and 0x401470 move a point toward
or away from another along the angle between them (0x42bbc0), and 0x4014f0 freezes it with an object at +0xaa8.
Translated since: the slingshot and the band with the hand (0x401940 and 0x401a10, which the board draws), the
aim (0x401e40 aims at a point by AngleTo 0x42bbc0, and 0x401d30 puts the loaded egg 25 pixels back against it), the
aiming arrow (0x401ed0) and the aim path's band (0x4022f0, with 0x4021e0 and the line crossing 0x42bcd0; its right
side goes on +0xa74 and its left on +0xa8c). Still to translate there: the update 0x402c10 (read, not written: a
frozen launcher holding an egg counts +0xaac up and past 55 loads the held egg, otherwise counts down and unfreezes
at 0; the arrow's colour fades toward red or white by bit 1 of the board's +0x12c; +0xa60 eases toward +0xa64; the
recoil shrinks by 0.85 a frame), the path 0x402d90 (it runs a flying egg to find the bounces, fills the band, and
draws the app's +0x18a4 picture on the landing cell), the draw 0x4033a0, and the save and load 0x401510 and
0x4016c0. Next: the rest of the launcher, then the base
board's remaining slots (Init 0x407ac0, Update, Draw 0x4044a0, the input slots), the sub-objects at +0x60
(0x431fc0), +0x84 (0x437aa0) and +0x88 (0x438400), and DynomitePanic.

The four modes are translated. `StartGame` (0x418810) makes DynomiteStomped (0x40ea50), DynomitePanic (0x40ca40),
DynomiteFossil (0x408e40) or DynomiteTimeTrial (0x411ff0) for the mode at +0x108c, resizes it to (150, 0, 490, 480)
and calls slot 46, `Init(app, level)`, with the skill at +0x1084, or the fossil kind at +0x1088 for the fossil mode.
Stomped copies its pack to STOMPED.TMP in the game's folder and reads the puzzles from that copy; the port keeps the
copy in memory (`BinFile::SetMemoryFile`). A pack starts with its name, the number of puzzles each skill skips and
its picture; each puzzle is a true byte, the width, the launcher's place, the shots per drop, the baby eggs' chance,
its name, the colours and kinds of its first 256 eggs, and a grid as `EggGrid::Load` (0x4171c0) reads it. A false
byte ends the pack, which then starts over at the skill's first puzzle with two shots a drop fewer. The fossil
mode's fragments and museum are in Fossil.cpp and Museum.cpp. `EggGrid::CountRows` (0x415980) compares with "less
or equal" (`testb $0x41`); with "less" the deepest row, where a fragment lies, was lost when the eggs were pushed
down. Translated since: the options dialog (0x42ee20), the saved games (slots 59 and 60 of the boards and of
their parts, the save dialog 0x433910, the name dialog 0x420fb0 and the load of SLOTn.SAV at 0x418fd0) and the
tutorial (0x419040); `README.md` says what is left out. App
slot 51 is `DoDialog`, 53 `AddDialog`, 20 `SafeDeleteWidget`, 31 `Shutdown`; WidgetManager 0x47ec90 is
`BringToBack`.

**Registration removed.** Everything an unregistered copy shows, and the registered copy's "thanks for registering" on
the title (2026-11-08), is out of the port; `README.md`, "Registration and loading", lists each piece. No other string
the port draws is a registered copy's (no stamp, name or status line).

## 5. Fix candidates

None so far (docs/MOD_ROADMAP.md, "The fixes mod"). Both phases of the identical-play pass (`IDENTICAL_PLAY.md`) found no
bug of the original that changes play (its scores, timing or rules) and no wait that only costs the player time; the
loading already runs at the machine's speed in the port. So the port has no fixes mod yet, and no mod loading. One
thing could become a fix of the port itself rather than of the mod: if the original's tutorial also stalls after a
match at step 45 that clears the eggs past the line, that is a soft lock. A three-egg match there goes on in both
programs; the clearing match was not found for the original (`IDENTICAL_PLAY.md`, "Oddities").

## 6. Performance (measured 2026-11-08, original and port back to back)

`tools/profile/perf_all.sh` runs `tools/profile/perf.py` (adapted from Noah's Ark's): five loads of each program, then
a scripted minute twice in each, one run at a time, on the same machine while other sessions ran three to seven game
processes (the machine was 72 to 97 percent busy), so these numbers are rough; the performance pass measures again on
a quiet machine. The original runs under origrun on the real clock, with its hooks and the scripted cursor; the port
runs natively without `-fast`, both under rundesk.

**Loading**, from the creation of the process to the title with its loading done (the play link up, the first screen
that takes input): the port 0.96 s (median; 0.93 to 1.08), the original 2.2 s (median; 2.09 to 2.61). At that point
the port's working set is 76 MB with 76 MB private, the original's 75 MB with 60 MB private, each after about 0.9 s of
processor time. The web build's load was not timed: the page's own import of the game's files comes first, and the
pane is hidden.

**The scripted minute** is the same input in both: the title's link at update 1500, Time Trial on Normal from the main
menu, then 150 shots at random points of the board, one every 40 updates, from update 2900; the minute is the 6000
updates from there (origrun quit the original at its update 8,864, just short of the minute's end at update 8,900, so
its window is the 5,964 updates up to there, 59.8 s at 100 a second).

| | peak working set | peak private bytes | processor time in the minute |
|---|---|---|---|
| original, run A | 87.9 MB | 65.3 MB | 11.6 s |
| original, run B | 88.1 MB | 65.4 MB | 13.2 s |
| port, run A | 100.8 MB | 93.9 MB | 4.9 s |
| port, run B | 92.0 MB | 91.8 MB | 4.6 s |

The port uses well under half the original's processor time for the same minute of play, and holds about 28 MB more
private memory. These processor times come from GetProcessTimes in single runs; "Processor time like for like" below
measures them again and replaces them.

### The performance pass (2026-11-11)

The machine was never quiet that day (other sessions ran three to six game processes throughout), so the comparison
above stands, and what follows is the port alone.

**Where the port's time goes.** Bejeweled's sampler (`games/bejeweled/tools/profile/cpuprof.py`) ran the port under
rundesk on the scripted minute's input with `-fast -drawevery`, every update drawn and nothing waited: 10.0 G cycles in
41 s. 48% are in ntdll (the system's calls, two thirds of them on the main thread and the rest on the GPU driver's own
thread), 17% in the NVIDIA driver, 6% in win32u and 4% in SDL. 16% are the port's exe, and most of that is loading's decoders (the GIFs 2.6%,
Ogg Vorbis about 3%, JPEG under 1%) and the framework's drawing (`PolyFill` 1.5%, `DrawImage` 1%, the textured quads
0.6%). The game's own code is under 2%: the board's draw 0.4% and its update 0.2%, the cells and the side bar less.
(`CellButton::Update` shows 1.3%, but it only forwards a held button's tick, so those are samples landing beside it.)
Nothing in the game's own code is worth speeding up. The costs that are there are shared ones already listed in
`docs/COORDINATION.md` (the GIF decode and its quantizing back to 8 bits, imagehlp, `SDL_RenderPresent`), for the
framework to fix once for every game.

**Memory.** The port's 28 MB of private bytes over the original's are two of those shared costs: the Direct3D 11 device
(20 to 25 MB a process) and the images' pixels, which the framework keeps after their upload.

**Leaks.** `tools/profile/leakcheck.sh` builds the port with `tools/profile/leakhook.cpp` (a scratch build, never
shipped), plays each of its cases once with one round and once with two, and lists the callers whose live blocks grew
with the second round. Two leaks of the original were found and are fixed in the port: the stick of dynamite that a
placed egg replaces (`EggGrid::Place`, 0x416590, takes it off the list and never deletes it; 2d9eb014), and the save
dialog's empty slot picture and its cell, Save button and Cancel button, which its destructor (0x433420) never deletes,
so each showing of the dialog leaked them. The growth left is caches (the registry's text, the file finder's names, the
string tables) and blocks of the screen each run ends on (the eggs of the board in play).

**The web build.** `tools/profile/webperf.js` drove the player-only page in the Browser pane (Chrome 156, the pane
hidden, its frames given by a 16 ms timer) at the game's own speed: the title clicked through, eight seconds on the main
menu, then twenty seconds of Endless played by the bot through `move` (54 shots). On the menu 23 of 382 frames drew
(only what changed is drawn), at 0.11 ms a frame and 1.9 ms at most, 0.5% of the real time. In play every frame drew, at
1.9 ms a frame and 19.4 ms at most (on a machine busy with other sessions' games), 9.4% of the real time. The wasm
heap stands at 46 MB, 44 of it in use: the 149 images' pixels are 34.6 MB and the 41 sounds 3.8 MB, the same images kept
after their upload as natively. The JavaScript heap is 12 MB. No error was logged. Nothing there needs speeding up.

### Processor time like for like (2026-11-12)

The target in `docs/COORDINATION.md` ("The target", the user, 2026-11-11) is that a port's processor time is at most
its original's, measured like for like. The minute above was read from GetProcessTimes, which Windows charges by
sampling at the 15.6 ms clock tick, in single runs with the original windowed. So it was measured again, by
`tools/profile/pairs.sh` and `tools/profile/cycles.py`.

**How the original paces itself.** Its main loop (SexyApp vf04, 0x4834b0) times its 100 updates a second with
timeGetTime. When an update is due it makes one; otherwise it draws and sleeps out the rest of the 10 ms, so it never
spins. Windowed, its redraw (0x469f50) blits to the primary surface with DDBLT_WAIT, which waits for the blitter, not
for a vertical blank. So it draws after nearly every update, as it does in a window on a player's screen. In full screen
its Flip waits for the vertical blank, so it draws at most 60 times a second on a 60 Hz screen. Full screen is a
player's default (`properties/partner.xml` sets DefaultWindowed false), but it cannot run on rundesk's desktop. The game
has its own way of pacing a window like that, though: with WaitForVSync set (the app's +0x559), its draw step (SexyApp
vf06, 0x488920) sleeps out the screen's refresh period, 16 ms here. So the original was measured both ways, with no
patch: windowed as the user's own settings have it, and windowed with WaitForVSync on in place of full screen.

The original reads its settings from HKLM\SOFTWARE\PopCap\Dynomite, which a run on the host cannot write. The 60 Hz set
therefore ran with origrun's `--reg-map popcap`, which opens those keys under HKCU, with the run's values set there. In
those runs the original drew 3,329 to 3,451 times in the minute, against 5,809 to 5,997 windowed. The port presents
with vsync whenever it is not running `-fast`, and its present waits on rundesk's desktop too. It drew 3,523 and 3,575
times in the two runs where its counters were read (its app's mDrawCount and mUpdateCount, read from outside). Both
sides made 6,000 updates in 60.0 s in every run.

**The same play, and nothing more.** The input is the scripted minute's, in real time on both sides. The stretch is the
6,000 updates from the first shot, and it ends 300 updates before the quit, so no shutdown is counted. The port reads
its input from a `-script` and gets no `-commands`. Two `state` lines of that script mark the stretch's ends, and their
replies are its only log lines. The original's update counter (0x530960) is read out of its memory every 10 ms. At each
end the process's cycles are read with QueryProcessCycleTime, and each thread's with QueryThreadCycleTime.

Sound runs on both sides and plays on neither. origrun's stand-in takes the original's DirectSound buffers, whose
cursors run on the real clock, and BASS mixes into them (the original's second-busiest thread, very likely BASS's
mixer, ran 1.6 to 2.3 G cycles in the minute, counted in the original's figure). The
port's mixer runs on SDL's audio thread, pulled by the silent driver under rundesk (0.4 to 0.7 G, counted in the port's
figure; its medians without it are given below).

Each set's loop was pinned by `tools/pincore.py` to one performance core of its own, with the original and the port
taking turns, five pairs a set. Other sessions ran three to eight game processes throughout. The second set was meant to
be the 60 Hz one, but its WaitForVSync never reached the copy (the flag read 0), so it is a second windowed set.

| Set | Original, G cycles in the minute | Port, G cycles in the minute | Medians |
|---|---|---|---|
| Original windowed (1) | 18.75, 20.48, 16.72, 19.06, 19.39 | 12.78, 7.37, 8.94, 9.14, 11.34 | 19.06 against 9.14 |
| Original windowed (2) | 28.94, 21.92, 21.64, 26.09, 20.70 | 13.77, 20.75, 24.27, 16.34, 25.62 | 21.92 against 20.75 |
| Original at 60 draws a second | 16.38, 13.34, 12.60, 12.16, 14.96 | 10.22, 15.87, 9.75, 11.19, 15.03 | 13.34 against 11.19 |

Over the ten windowed pairs the medians are 20.59 G for the original and 13.27 for the port, 36% less. Against the
original at 60 draws a second they are 13.34 and 11.19, 16% less. Without its sound thread the port's medians are
12.73 and 10.74. But in all three sets the port ran in its default window, 1280 by 960 at render scale 2 (twice the
game's size, which fits rundesk's 3840 by 2160 desktop), against the original's 640 by 480. So these sets do not
compare like for like, and a fourth set took the port at the game's own size as well ("The same window", below).

**The spread is the port's.** The original's main thread cost 14.8 to 26.2 G windowed and 10.0 to 13.6 G at 60 Hz,
while the port's main thread ranged from 3.7 to 20.2 G over the same input. The port was over the original in two of
the five 60 Hz pairs (by 2.5 G and by 0.07 G) and in two of the ten windowed ones. Its second-busiest thread, the
graphics driver's by section 6's profile, stayed between 3.1 and 5.3 G. Bejeweled's sampler, over one real-time run,
put 83% of the port's main thread in system calls and 7% in the exe. (A 32-bit sampler sees a WOW64 system call as the
stub in ntdll; the game's own code is under 2%, as section 6 found.) What varies, then, is the framework's GPU path:
it draws the whole screen each frame and presents it through the driver, on a GPU that other sessions' games were
using at the same time. The original draws in software on rundesk's desktop (`docs/ORIGRUN.md`) and never touches the
GPU. This is the driver cost of a presented frame that `docs/COORDINATION.md` already lists among the shared costs
(SDL_RenderPresent). Nothing in Dynomite's own code is worth speeding up for it.

**The same window.** The target's method (`docs/COORDINATION.md`, "The same window on both sides") measures a port
windowed at the game's own size in pixels, with its default window beside it, and names the renderer of each run. So
the fourth set ran rounds of three in turn, five rounds pinned to one performance core (`tools/profile/pairs.sh`, with
CYC_VSYNC=1), with the original at 60 draws a second as in the third set. Each round ran the original, then the port at
640 by 480 (`-windowscale=1`), then the port in its default window. Every port run's display line read "client 640x480
px, logical 640x480, render scale 1, windowed, renderer direct3d11" or "client 1280x960 px, logical 640x480, render
scale 2, windowed, renderer direct3d11". The original drew windowed in software with DirectDraw on the same private
desktop. Each run made 6,000 updates in 60.0 s. The original drew 3,355 to 3,465 times, the port 3,434 to 3,581 times
at 640 by 480 and 3,439 to 3,541 times in its default window.

| Round | Original at 60 Hz | Port at 640x480 | Port at 1280x960 |
|---|---|---|---|
| 1 | 14.97 | 12.11 | 11.97 |
| 2 | 10.49 | 15.56 | 10.06 |
| 3 | 10.64 | 12.25 | 11.09 |
| 4 | 10.07 | 12.67 | 12.15 |
| 5 | 10.98 | 14.57 | 10.18 |
| Median | 10.64 | 12.67 | 11.09 |

The figures are G cycles in the minute. By medians the port is over its original: by 19% at the game's own size and by
4% in its default window. Without its sound thread its medians are 12.22 and 10.61. The original's figures were
lower in this set than in the third (10.07 to 14.97, against 12.16 to 16.38) for the same input and settings, so only
the rounds of one set compare.

**Where the cycles go.** One more run of each, with CYC_SPLIT=1, split the stretch into update, draw and present: the
port with `-cyclesplit`, the original with origrun's `--count` over `tools/profile/count.txt` (SexyApp's
DoUpdateFrames at 0x481110, DrawDirtyStuff at 0x488920 and Redraw at 0x48a200). These are figures from
`tools/cyclesplit.py`, M cycles an update, the port at 640 by 480:

| Part | Port | Original |
|---|---|---|
| Update | 0.023 | 0.023 |
| Draw | 0.378 | 0.749 |
| Present | 0.499 | 0.401 |
| The graphics driver's threads (nvwgf2um.dll) | 0.789 | none |
| Sound and the other threads | 0.074 | 0.332 |
| The loop (events, sleeps, the readings) | 0.033 | 0.128 |
| Total | 1.796 | 1.633 |

The game's own code costs the same on both sides. Everything that differs is the picture. A GPU renderer batches its
draw calls until the present, so the port's draw, present and driver threads are one figure: 1.67 M cycles an update,
2.9 M a drawn frame, against the original's software drawing and Blt at 1.15 M an update, 2.0 M a frame. Of that, the
port's present and the driver's thread come to 1.29 M an update, where the original's Blt is 0.40 M. The port's
default window costs about the same (1.87 M an update in all), since the driver's cost is a present's, not a pixel's.
The port's other threads and loop are cheaper than the original's and take back about a third of the difference.

This is the shared cost listed in `docs/COORDINATION.md` under "Presenting in step with the display": the present and
the Direct3D 11 driver's thread, 2.2 M cycles a drawn frame here, as 1.7 to 2.7 M in Typer Shark and 1.9 to 2.4 M in
AstroPop. The software-surface presenter that came to the trunk for it (`framework/SurfaceDisplay.h`, a299640e) cannot
serve Dynomite. It shows a screen that the app draws in software into a buffer of its own. Dynomite's screen is the
framework's screen DDImage, and every draw into it (`DDImage::Blt`, `FillRect` and the rest, through `Check3D`) goes to
the display as an SDL call on the GPU, so no CPU picture of the screen exists to hand over, as in Alchemy. So the 19%
stays with that shared cost. A way out would be shared too: a SexyAppFramework screen drawn by the framework's own
blitters into its bits, as the original's DirectDraw path draws, and shown by the surface presenter. Dynomite's board,
490 by 480 of the 640 by 480 screen (77%), is marked dirty on every update in play, and the sidebar now and then.

## 7. The PopCap Game Collection version (2026-11-11)

The Collection disc (the user's ISO, 2009) has Dynomite Deluxe as the group `DynomiteDeluxeComponent`: 227 files, 4.1 MB.
`unshield` (`E:\retools\unshield-build`, built for Bejeweled's check) read it from the image mounted read-only, dismounted
afterwards, into the main checkout's `re/_installers/DynomiteDeluxeCollection` (git-ignored).

**The same game.** The Collection's `Dynomite.exe` is the game itself, with no wrapper around it. Set beside the game
image carved out of Steam's wrapper (`re/dynomite/bin/DynomiteGame.exe`; Bejeweled's `tools/collection/cmpexe.py`), it
has the same link time (2003-07-08 21:48:08), the same entry point and the same sections. `.text` and `.data` are byte for
byte the same; `.rdata` differs in 7 bytes, the digits of a partner stamp at 0x5039a1; `.rsrc` is larger (143,360 bytes
against 28,672: more icons), a few header bytes differ and a signature of 5,424 bytes follows the last section. So
everything the decompilation, the identical-play pass and the coverage check found holds for the Collection's game too.
The port finds the game image in either file (`LoadGameTexts` takes the image whose texts match), and the texts are the
same by address and hash.

**The files** (Bejeweled's `tools/collection/cmpfiles.py`, against the Steam install). Of the Steam folder's 272 files
and the Collection's 227, 218 are the same byte for byte: every sound, song, data file, the puzzle pack and every picture
but these:

- `images/title.jpg`, the loading screen: the Steam copy's carries the flat "PopCap Games" logo with the web address at the
  bottom right; the Collection's is another encoding with PopCap's round logo and two TM marks. The port draws the file it
  is given.
- `images/whitefont.png`, `whitefont2.png` and `whitefontsmall.png`: palette pictures in Steam's copy, RGBA in the
  Collection's, with the same alpha and the same colour wherever a pixel shows.
- `properties/partner.xml`: the same product name, registry key (`PopCap\Dynomite`), `NoReg` and `DefaultWindowed`. The
  Collection's sets `DontUpdate` false and links "Play More Great Games!" to PopCap's site, with an `Offering` number;
  Steam's has its store id and buy link and one "More Games" link. The update check and the links are not in the port.
- Only in the Steam folder: the wrapper's files (`drm.swf`, `drm.xml`, `drmss.jpg`, `Flash.ocx`), the user's high scores
  (`fossil.hs`, `timetrial.hs`), `stomped/readme.txt`, and 38 `sounds/cached_*.wav`. Those are the original's own cache:
  its sound manager (0x495db0) loads `cached_<name>.wav` when it exists, else the sound's own file, and passes a decoded
  Ogg or MP3 to 0x4943e0 under the cached name, so a Collection install makes them on its first run. The port decodes the Ogg files and reads no cached file.

## 8. Removal of code the port does not need (2026-11-11)

The registration code and the buttons that lead out of the game went earlier (`README.md`, "Registration and loading").
A scan of the port for what nothing calls or uses (every function defined in `src/` whose name appears nowhere but its
declaration, every picture the app loads and nothing draws, every field nothing reads, checked against the export)
found, and took out:

- `FossilizedApp::PumpLoading`, the port's own, which nothing called; `CeilingSpeed::ResetSpeed` (0x435dc0), which
  nothing calls in the port or in the original (Ghidra found no caller and made no function there).
- The update check's wait bar, `images/waitbar` (+0x1224): the original draws it only in its online update check
  (0x41f4c0), which the port does not have.
- Two cells of the main menu's signs that the original cuts and never draws ("select the game you want to play" and
  "select a difficulty level", +0x12fc and +0x1300). Their two steps of the loading count stay, so the bar reaches the
  original's 394.
- The count of games that lasted a minute (+0x4d8), which only the unregistered copy's screens read.
- Fields nothing touches or only clears: the main menu's +0x64, +0x238 and +0x274, the title's +0x90 and the
  two spare ints of each cell the ceiling check visits.
- The parameter of Stomped's `LoadGrid` (0x40eef0) and `NextPuzzle` (0x40fc30), which the original's functions do not
  take (Ghidra's listing gave them one, and every call passed 1).

Kept on purpose: the fields that only a saved game holds (the board's +0x64, the launcher's +0x1c, +0x30 and +0xa5c,
the fire prompt's +0xc and +0x1c, Steggy's +0x28 and +0x2c, the Fossil Challenge's +0x264), since the saved game's
format stays the original's; each is named for what it holds and says that nothing reads it. The staging tool no
longer copies the 38 `sounds/cached_*.wav` the original writes on its first run (section 7), which the port never read.

## 9. The readability pass (2026-11-11)

`tools/namecheck.py games/dynomite` went from 1130 findings to none. There is no Mac build of Dynomite, so every name
comes from how the code uses the thing, read in the port and checked in the export where the port's uses left it
open.

- **61 stand-in names** (`mField70` and the like, 1085 uses) named for what they hold: the board's play field
  (`mPlayWidth`, `mPlayHalfWidth`, `mPlayMiddleX`, `mLineY`), its game's end (`mLosing`, `mGameOver`, `mGameOverShown`,
  `mGameOverDelay`, `mPanelTop`), the shot (`mFireBlocked`, `mPlaying`, `mShotCount`, `mFiredThisRound`, `mAvoidColor`,
  `mLastFiredColor`, `mInputLocked` for the tutorial), the ceiling (`mCeilingGoal`, `mCeilingAtGoal`), the grid's
  `mAlarmDue`, `mCeilingDrawn`, `mKeepPastRight` and `mShowPastLine`, the launcher's hand, band, aim path and arrow,
  and the rest. A field that is saved and loaded and that nothing reads (section 8) says so; three hold 0 throughout
  and are `mSavedZero`, after Alchemy's and Atomica's.
- **The picture cells** named for what they show (`mPicNextButton`, `mPicBackButton`, `mPicPlayButton` and their
  hover cells, `mPicNewGameIcon`, `mPicPanicIcon`, `mPicWelcomeSign`, `mPicStartNewGameSign`, the side bar's
  `mPicMenuButton`, `mPicOptionsButton`, `mPicExitButton`, `mPicMuseumButton`), and the side bar's numbered buttons
  (`mButton1`, `mButton2`, `mButton5`) as `mOptionsButton`, `mExitButton` and `mMuseumButton`.
- **Nine functions** given their original's address or the port marker: the helpers split out of `Dynomite::Load`
  (0x405590), the main menu's constructor (0x424e90), `PuzzleListWidget::LoadPacks` (0x4312b0) and `Tutorial::Step`
  (0x43b2c0) name it; `WarpToBoard` names the two places the original does it inline; the memory file and the game
  texts are the port's.
- **Seven one-letter names** that lived past ten lines, and the comments that named a renamed field by its offset.

The renames are proved by the object code (`tools/objsnap.py`): every object of the game compiles to the same
instructions before and after, but for `DynomiteStomped.cpp`, where the dropped parameter of section 8 changes the
calls. `tools/splitfields.py dynomite` finds no member declared twice; its "never written" members are character
arrays filled by `strcpy`, and its read-only offsets are calls through the virtual table.

## 10. Coverage check (2026-11-11)

The whole coverage check (`docs/COVERAGE.md`). origrun's `--coverage` recorded which of the original's functions ran in
19 runs, every one of them the original's side of a phase-two sound comparison (`tools/shots/sound.sh <tour> orig` with
`ORIG_ARGS="--coverage ..."`, the hits in `build/coverage/dynomite/hits/`): on the host, on the virtual clock, with the
recording sound stand-in, so the mixer's and the music's code run. They are the tutorial, the recorded games of every
mode (Endless, Stomped, Fossil, Panic and Time Trial, the last recorded twice, `IDENTICAL_PLAY.md`) and tours t1 to
t12. Each reached 1259 to 1532 of the exe's 6531 functions, and every one ended with the game exiting by itself.

`games/dynomite/coverage.txt` sets the game's own code as 0x401000-0x442690 (the game classes in link order, to the
whirley and its small effects); the rest of the exe is the 2003 SexyApp framework, its libraries and the C runtime. The
game's code has 670 functions; the runs reached 593 of them, the port defines 556, and 114 are left out on purpose.

**Reached in the original with no counterpart in the port: 65 at first (the first four runs), none at the end.**

- **The compiler's scalar deleting destructors: 45**, each a call of the destructor proper and a free, as `none` lines.
- **Left out on purpose: 15 more**, each a `none` line with its reason: the C++ library's string and vector code that the
  linker put among the game's (`std::string` assignment, append and compare, a vector of strings' copy), two of the
  framework's inline defaults in the name dialog's table, `CRect`'s constructor, the cursor override that always calls
  SexyApp's own (its flag is never set), and the update check on quitting (it goes online). The registration code, the
  update check's question and download, opening a URL and the credits' two links were `none` lines already (sections
  7 and 8).
- **Placed: 5.** Functions the port has inline got their address in the comment over the code that holds them: the
  angle table's lookup, the list's constructor, a picture's size, Steggy's draw helper and the program's entry.
- **No missing game logic was found.**

**Defined in the port, never reached: 135 after the first four runs, 33 at the end.** The tours t1 to t9 and the
Panic and Time Trial recordings reached the save and load of Endless, the options, credits and high scores, the exit
question, the pause and museum keys and Time Trial's scoring (50 left). Three tours were then added for what no run had
done, t10 to t12 (`IDENTICAL_PLAY.md`): a save and Continue Game in Stomped, in Fossil with the museum open and in Time
Trial, Return in the high score's name box and in the save's, and the options' Music slider. Time Trial was recorded
again on the current exe, since the first recording's replay stopped one update short of clearing the board and so never
drew Time Trial's results (0x412cb0, 3.2 KB). Those runs reached 17 more, among them the three modes' and the museum's
save and load, both name boxes' Return, the slider and the results.

Every one of the 33 left, and nine that the last runs then reached, was read again against the export: 41 in two Sonnet
passes, where a save was also checked against its load, field by field and width by width, and `EggGrid::PickColor`
earlier in the session. All match but one,
which differs only where nothing reaches it:

- **`PicFont::~PicFont` (0x4365d0)**: the original deletes the font's image itself, shared or not and without clearing
  the pointer, and then `~Pic` deletes it again, a double delete. Neither program ever deletes a font (all three live as
  long as the app), so it never runs; the port deletes the image once, and its comment says so.

What reaches the 33, none of which a run of the original met:

- **A save with something alive that the runs' saves never caught**: five effects' save and load (a baby dinosaur, a
  bonus egg's prize in flight, a dynamite's boom, confetti, a broken egg's shell quarters), the thunderbolt's, Panic's
  whirleys' and the Rex attack's (16).
- **The Rex attack** on the museum's last wall (level 15, round 3, after the Beware button): its constructor, destructor,
  update and draw (4), and `EggGrid::PickColor`, for a level's cells of kind 0x14, which no run's levels had (1).
  Since then the soundboard's sound logs added a run that plays it in the port (botrec.py with `--level 14 --winafter
  5 --museumwait 450`: `win` in the Fossil Challenge takes every egg and lets the fossil fall), not yet one in the
  original.
- **Input no run gave**: a bomb's blast on the fossil board (`DynomiteFossil::Blasted`, its kick), Stomped's pack list
  under the mouse (its scrolling, hover and click: no run moved the mouse over the list), and the dialog that the
  options show when the desktop allows only full screen (`ShowNoWindowed`), and the destructors of a fossil and of the side bar's small Inchy's bubble
  (`~Fossil`, `~InchyBubble`, through their scalar deleting destructors): every run quits from the game it played, and
  the original exits without deleting its objects (7).
- **Nothing in either program**: the destructors of a font and of a sound, which live as long as the app (`~PicFont`,
  `~GameSound`), the board's empty `DrawPiece` (only the fossil board's own runs, and fossil pieces exist only there),
  `CellButton::SetFont` and `PtrList::SortedAdd` (5).

**Candidates for taking out**, none removed now: `PtrList::SortedAdd` with the list's sort mode and offset (no list of the
game is sorted: the constructor zeroes the mode, nothing sets it, and `Add` runs in every run while `SortedAdd` never
does), and `CellButton::SetFont` (nothing calls it on a cell button; the original's calls are on plain buttons). The
destructors stay, as a class needs them, and so does the rest: code that only some input or a long game reaches.

The coverage check found one fault of the port's, though not through a function: the first comparison of the Fossil and
Panic recordings' screens showed the museum coming up over a black board, and the port now keeps its screen as the
original's does (`IDENTICAL_PLAY.md`, "Phase 2: what was changed", d012ac13). The extra runs cost about 40 minutes of
machine time (the new recording, three tours, each in both programs with screens and sound), and the two rereads about
275,000 Sonnet tokens.
