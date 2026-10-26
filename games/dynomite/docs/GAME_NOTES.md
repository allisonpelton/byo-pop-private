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
it makes a 0xb48-byte object (0x42bb50, link order: Museum.cpp) at +0x5d4, seeds the MTRand at +0x5d8 with
`GetTickCount()` and the game's own generator at +0xf9c (class 0x431d30, seeded by 0x431d50 with `GetTickCount() >> 1`),
and sets +0x1084 = 1, +0x1088 = 0, +0x108c = 2 (settings read later), +0x1094, +0x1aac, +0x10b4, +0x10ec, +0x1830 to 0.

Other app fields seen so far: +0x10b8 sound initialised (the sound manager's slot 1), +0x10d8-0x10e4 the four cursor
Pics, +0x19d4/0x19d8/0x19dc the three white ImageFonts, +0x19e0 the TitleScreen, +0x5bc a list of every image loaded
through `GetImage` (0x419ad0), +0x2cc the WidgetManager, +0x314 the music interface, +0x490 the sound manager.
App slot 19 (+0x4c) is `Popup(string)`, slot 37 (+0x94) `PlaySample(id)`.

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
game's pointer list (`src/PtrList.h`, 0x430ea0). `src/Dynomite.h/.cpp` is a stub with the board fields known so far
(constructor 0x404030: lists at +0xb8, +0xd0, +0xe8, +0x10c, an object at +0x164 (0x414a00), the pause count +0x150,
the dynamite +0x154).

Next, in this order: the main menu (`IntroScreen`, 0x424e90, 0x284 bytes; its methods 0x423d10-0x424e90 and on into
0x425000-0x42a000), `ShowMainMenu`'s TODO (0x4186d0), `StartGame` (0x418810: mode +0x108c makes DynomiteStomped
0x40ea50 (0x62c bytes), DynomitePanic 0x40ca40 (0x274), DynomiteFossil 0x408e40 (0x328) or DynomiteTimeTrial
0x411ff0 (0x254), resized to (150, 0, 490, 480), slot 46 `Init(app, level)` with +0x1084, or +0x1088 for the fossil
mode), then the board (0x401000-0x408e40; 0x401000-0x404030 holds other classes), the modes, the app's
ButtonPress/ButtonDepress (0x418e90, 0x4201b0), the dialogs (options 0x42ee20, save 0x433910, high scores), the
museum (0x42bb50 at app+0x5d4) and the tutor. App slot 51 is `DoDialog`, 53 `AddDialog`, 20 `SafeDeleteWidget`,
31 `Shutdown`; WidgetManager 0x47ec90 is `BringToBack`.
