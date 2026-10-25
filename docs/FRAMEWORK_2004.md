# The 2004 SexyAppFramework: compatibility map onto our port

PopCap's early-2004 games (BookWorm Deluxe, Typer Shark! Deluxe, AstroPop Deluxe) are built on an internal
SexyAppFramework snapshot that is older than any public release. This document maps that framework onto
the port's framework (`framework/`, the SexyAppFramework 1.3 API on SDL2): every vtable slot and field the
game code uses, what it is in SAF 1.2 and in the port, and where the old behaviour differs. Game
translations call the port's methods by name; this map tells the translator which name a
`(**(code **)(*this + 0x34))(g)` in the decompilation is, and what the old code did that the port must
reproduce.

It was derived from BookWorm Deluxe 1.03 (game PE linked 2004-04-15 with VS.NET 2002 / MSVC 7.0,
`games/bookworm/`). Typer Shark (linked 2004-04-20) and AstroPop (2004-04-06) use the same snapshot; see
[Reuse for Typer Shark and AstroPop](#reuse-for-typer-shark-and-astropop).

Contents:
1. [How the map was made](#1-how-the-map-was-made)
2. [The framework at a glance](#2-the-framework-at-a-glance)
3. [Widgets](#3-widgets)
4. [The application class (SexyApp)](#4-the-application-class-sexyapp)
5. [Graphics, images and fonts](#5-graphics-images-and-fonts)
6. [Sound and music](#6-sound-and-music)
7. [Support classes and library helpers](#7-support-classes-and-library-helpers)
8. [Behaviour differences and how the port handles them](#8-behaviour-differences-and-how-the-port-handles-them)
9. [Open questions](#9-open-questions)
- [Reuse for Typer Shark and AstroPop](#reuse-for-typer-shark-and-astropop)

Conventions: addresses are virtual addresses in BookWorm's carved game PE (`re/bookworm/bin/BookwormGame.exe`,
image base 0x400000). `(**(code **)(*this + 0xNN))` in the decompilation is slot NN/4 of the object's primary
vtable. "1.2" is SexyAppFramework 1.2 as built into Big Money Deluxe (`games/bigmoney/docs/FRAMEWORK_12.md`);
"port" is this repository's `framework/`. Verified facts come from constructor stores, accessor bodies or the
disassembly; inferred ones are marked.

---

## 1. How the map was made

No source of this framework generation is public. The map combines four sources:

1. **RTTI.** Ghidra 12 recovers no classes from MSVC 7.0 binaries (docs/POPCAP_GAMES_SURVEY.md 1.4), so
   `tools/rtti_msvc70.py` reads the RTTI itself: 159 classes, 187 vtables, and a `Class::vfNN` name for
   every virtual function.
2. **Byte matching against Big Money Deluxe.** Big Money (2005-11, SAF 1.2) was built with the same
   compiler, and its Ghidra project is named slot by slot (`games/bigmoney/docs/FRAMEWORK_12.md`).
   `tools/funcmatch.py` pairs functions whose bytes are identical once addresses are masked, then pairs the
   functions those identical functions call at the same place, and carries Big Money's names over. Of the
   framework's 1,388 functions, 828 pair with Big Money's (550 byte-identical); 698 names were carried
   (framework, STL and CRT).
3. **Reading the code** against the public SAF 1.2 tree (`E:\retools\refs\saf12`) and SAF 1.3, for every
   function the game code calls and every vtable slot, with the field offsets taken from constructors and
   accessors.
4. **alula/CircleShootApp** (Zuma 1.0.0, 2003, also MSVC 7.0), a decompilation that runs a game of this
   era on SAF 1.3 with small patches restoring old behaviour. It was read as a guide only.

Reproducing it (all outputs go to the git-ignored `re/bookworm/`):

<!-- private: First the game image is copied out of `Bookworm.exe` to `re/bookworm/bin/BookwormGame.exe` with a private tool (not published). -->

```bash
python tools/extract_game_exe.py "C:/Program Files/PopCap Games/BookWorm Deluxe/Bookworm.exe" -o re/bookworm/bin/BookwormGame.exe
```

<!-- /private -->

```bash
python tools/rtti_msvc70.py re/bookworm/bin/BookwormGame.exe re/bookworm/rtti
MSVC70=1 SYMBOLS=re/bookworm/rtti.jsonl build/native/rundesk.exe bash tools/ghidra_import.sh re/bookworm/bin/BookwormGame.exe bookworm_game re/bookworm/gh
python tools/rtti_msvc70.py re/bookworm/bin/BookwormGame.exe re/bookworm/rtti --functions re/bookworm/gh/functions.jsonl
python tools/funcmatch.py re/bookworm/bin/BookwormGame.exe re/bookworm/gh/functions.jsonl <bigmoney>/re/bigmoney/bin/BigMoneyGame.exe <bigmoney>/re/bigmoney/ghidra/functions.jsonl re/bookworm/bm_match.jsonl --symbols <bigmoney>/games/bigmoney/docs/symbols/05_framework_slots.jsonl <bigmoney>/games/bigmoney/docs/symbols/10_framework_direct.jsonl --fuzzy --apply-names re/bookworm/bm_names_all.jsonl
GHIDRA_PROJECT=bookworm_game GHIDRA_PROGRAM=BookwormGame.exe EXPORT_DIR="$(pwd -W)/re/bookworm/gh" build/native/rundesk.exe bash tools/ghidra_headless.sh apply re/bookworm/rtti.jsonl games/bookworm/docs/symbols/*.jsonl
```

The framework symbol files are `games/bookworm/docs/symbols/10_framework_*.jsonl` (names for Ghidra's
`ApplySymbols.java`).

## 2. The framework at a glance

What sets this generation apart from the public trees:

- **One application class.** `Sexy::SexyApp` (67 virtual slots, derived only from `ButtonListener`)
  holds everything that SAF 1.2 splits between `SexyAppBase` (90 slots) and `SexyApp` (96). The game's app
  class derives from it.
- **No `WidgetContainer`.** `Widget` is a root class (47 slots, 0x58 bytes); its children, dirty flags and
  `WidgetManager` links live in `Widget` itself. `WidgetManager` has no vtable of its own beyond its
  destructor.
- **Smaller widgets and dialogs.** sizeof(Widget) 0x58 (1.2: 0x80), sizeof(Dialog) 0x110 (1.2: 0x148).
  A game class derived from Widget starts its data at +0x58, or +0x5c after a `ButtonListener` vfptr.
- **Loose files.** There is no `PakInterface`; files are opened with the C runtime.
- **DirectDraw 7 / D3D7 drawing**, DirectSound sounds (`DSoundManager`, Ogg and `.au`), BASS 2.0 music
  (`BassMusicInterface`, MO3 by order position) with an FMOD fallback (`FModMusicInterface`).
- **`ResourceManager` is linked but unused**: resource names are hard-coded in the game.
- **ImageFont/DescParser** fonts as in 1.2.

Vtable sizes (primary vtable slots) compared with SAF 1.2 as built into Big Money:

| Class | 2004 (BookWorm) | 1.2 (Big Money) | Notes |
|---|---|---|---|
| Widget | 47 | 54 (15 from WidgetContainer) | no WidgetContainer base |
| ButtonWidget, DialogButton, HyperlinkWidget | 49 | 57 | |
| Checkbox, Slider | 49 | 56 | |
| Dialog | 52 | 60 | |
| EditWidget | 53 | 60 | |
| ListWidget | 60 | (not linked) | derives from ScrollListener; no ScrollbarWidget class |
| CursorWidget | 47 | (not linked) | |
| ButtonListener | 6 | 7 | |
| SexyApp | 67 | 90 (SexyAppBase) + 6 | monolithic |
| Image, MemoryImage, DDImage | 10, 26, 37 | 15, 33, 46 | |
| Font, ImageFont, SysFont | 12, 23, 12 | 12, 23, 12 | same slot counts |
| SoundManager, SoundInstance | 14, 12 | 16, 12 | |
| MusicInterface, BassMusicInterface, FModMusicInterface | 15, 15, 17 | 20, 20, 22 | |
| ResourceManager | 3 | 14 | |

## 3. Widgets

Widget, WidgetManager, ButtonWidget, DialogButton, Dialog, Checkbox, Slider, EditWidget, ListWidget,
HyperlinkWidget, CursorWidget, the listener interfaces, Insets/Rect/Point and KeyCodes. Symbols:
`games/bookworm/docs/symbols/10_framework_widgets.jsonl`. Subsection numbers and "see N.M" references
inside this section are local to it.

### 0. The big structural differences (read this first)

1. **No WidgetContainer.** A 2004 `Widget` has no children, no `mParent`, no `mPriority`/`mZOrder`, no
   widget flags. `WidgetManager` derives from nothing (only a virtual destructor) and keeps one flat
   `std::list<Widget*>`; every widget is top-level and its `mX/mY` are screen coordinates. Translate
   every `mWidgetManager->AddWidget(w)` / `RemoveWidget(w)` literally (top-level `AddWidget` in the port);
   never turn them into child widgets.
2. **Dialog buttons are app buttons.** A Dialog's yes/no (or footer) buttons get ids **`mId + 2000`** and
   **`mId + 3000`**, fixed labels `"YES"/"NO"/"OK"/"CANCEL"`, and the Dialog's ButtonListener forwards
   their press/depress/downtick to **`mWidgetManager->mApp`** (SexyApp *is* a ButtonListener, vtable
   slots 3/4/5). There is no DialogListener, no `mResult`, no `WaitForResult`. Identical to the Zuma
   (CircleShootApp) patches; see section 6 and difference D1.
3. **ButtonListener slot order differs from 1.2** (no ButtonMouseMove): 0 ButtonMouseEnter,
   1 ButtonMouseLeave, 2 ButtonPress(id, clickCount), 3 ButtonPress(id), 4 ButtonDepress, 5 ButtonDownTick.
4. **Widget vtable is 47 slots** and in a different order from 1.2 (which had 54 incl. WidgetContainer).
   Game classes start their own virtuals at slot 47 (Widget-derived), 49 (ButtonWidget-derived) and 52
   (Dialog-derived). Secondary interface vtables sit at **+0x58** (= sizeof(Widget)) and, in dialogs, at
   **+0x110** (= sizeof(Dialog)) / +0x120 / +0x124 (after game members).
5. **Identical-COMDAT folding** merges unrelated tiny functions. `0x437e90` ("Color::Color_rgba") is also
   `Insets(l,t,r,b)` (the game writes `Dialog::mContentInsets` with it: `Color_rgba(.., 0x24,0x24,0x24,0x24)`
   copied to dialog+0xe0 = `mContentInsets = Insets(36,36,36,36)`); `0x437e80` is `Insets()`/`Rect()`;
   `0x437eb0` is the 16-byte copy constructor of Insets/Rect/Color. The empty-virtual stubs
   (`0x401570` ret 4, `0x4220f0` ret 8, `0x41dfa0` ret 12, `0x492cd0` ret 16, `0x4036d0` ret) and the
   "return true" stubs (`0x421850`, `0x4292c0` ret 8) fill many slots of many classes: name a call through
   them by the **slot**, never by the stub.

Graphics/app functions referred to below (owned by the other groups): `0x44b840` Graphics(Image*),
`0x44b790` Graphics(const Graphics&), `0x446bd0` ~Graphics, `0x437670` Graphics::operator=,
`0x446be0` GetFont, `0x446bf0` SetFont, `0x446c00` SetColor, `0x446c30` GetColor, `0x446c60`
SetColorizeImages, `0x446c90` DrawString, `0x446cc0` Translate, `0x44b8c0` Create, `0x44b8e0` FillRect,
`0x44b940` DrawRect, `0x44ba30` DrawImage(img,x,y), `0x44bad0` DrawImage(img,x,y,srcRect),
`0x44bdb0` ClipRect, `0x44f820` DrawImageBox(dest,img), `0x44be30` DrawImageBox(src,dest,img),
`0x44f950` SysFont ctor, **`0x446d10` (named `get_field_04`) = Image::GetWidth, `0x468620` (named
`Font::vf02`) = Image::GetHeight** (both misnamed in the current export), `0x47d8f0` SexyApp::SetCursor(int)
(0 pointer, 1 hand, 2 dragging, 3 text), `0x47e810`/`0x4873c0` app clipboard copy/get (inferred).

Font slots used by widget code (same as 1.2): +0x04 GetAscent, +0x08 GetAscentPadding, +0x0c GetDescent,
+0x10 GetHeight, +0x18 GetLineSpacing, +0x1c StringWidth, +0x24 CharWidthKern, +0x2c Duplicate.

### 1. Widget

sizeof **0x58** (1.2: 0x80 incl. WidgetContainer 0x4c; port: Widget : WidgetContainer). No base class.
Primary vtable `0x5017ac`, 47 slots. ctor `0x43e4e0` (no arguments), dtor body `0x43ab10`, deleting
dtor `0x43b3f0`; Checkbox/Slider/CursorWidget share the deleting dtor `0x43e8f0`.

#### 1.1 Vtable

Offset | slot | address | 2004 method | 1.2 slot | port 1.3 | notes
---|---|---|---|---|---|---
+00 | 0 | 0043b3f0 | ~Widget() | 0 | ~Widget | frees mColors
+04 | 1 | 00437800 | AddedToManager(WidgetManager*) | 7 | WidgetContainer::AddedToManager | only sets mWidgetManager
+08 | 2 | 00437810 | RemovedFromManager(WidgetManager*) | 8 | WidgetContainer::RemovedFromManager | mWidgetManager = NULL
+0c | 3 | 004036d0 | OrderInManagerChanged() | 15 | OrderInManagerChanged | empty
+10 | 4 | 00439e50 | SetVisible(bool) | 16 | SetVisible | hide: MarkDirtyFull, show: MarkDirty; RehupMouse
+14 | 5 | 0043ecf0 | SetColors(int theColors[][4], int n) | 17 | SetColors([][4]) | replaces mColors, MarkDirty
+18 | 6 | 0043ec70 | SetColors(int theColors[][3], int n) | 18 | SetColors([][3]) |
+1c | 7 | 0043ed70 | SetColor(int idx, const Color&) | 19 | SetColor | grows mColors
+20 | 8 | 0043ee00 | const Color& GetColor(int idx) | 21 | GetColor(int) | grows mColors with Color() when out of range (1.2/port return a static)
+24 | 9 | 00437860 | SetTransient(bool) (name inferred) | - | not in port | +0x1c flag; hide: MarkDirtyFull first; only CursorWidget uses it
+28 | 10 | 00439f00 | SetDisabled(bool) | 22 | SetDisabled | as 1.2 (DisableWidget, re-MousePosition)
+2c | 11 | 004378e0 | ShowFinger(bool) | 23 | ShowFinger | no NULL check of mWidgetManager
+30 | 12 | 00437820 | Resize(const Rect&) | 24 | Resize(const Rect&) |
+34 | 13 | 00439e90 | Resize(int x, int y, int w, int h) | 25 | Resize(x,y,w,h) | MarkDirtyFull, set, MarkDirty, RehupMouse
+38 | 14 | 00437840 | Move(int x, int y) | 26 | Move |
+3c | 15 | 004378a0 | bool WantsFocus() | 27 | WantsFocus | constant false (no mWantsFocus field)
+40 | 16 | 00401570 | Draw(Graphics*) | 13 | Draw | empty
+44 | 17 | 004378d0 | Update() | 9 | Update | mUpdateCnt++
+48 | 18 | 004378b0 | GotFocus() | 29 | GotFocus | mHasFocus = true
+4c | 19 | 004378c0 | LostFocus() | 30 | LostFocus |
+50 | 20 | 00401570 | KeyChar(char) | 31 | KeyChar(SexyChar) | empty
+54 | 21 | 00439880 | KeyDown(KeyCode) | 32 | KeyDown | TAB: SetFocus(shift ? mTabPrev : mTabNext)
+58 | 22 | 00401570 | KeyUp(KeyCode) | 33 | KeyUp | empty
+5c | 23 | 00437910 | MouseEnter() | 34 | MouseEnter | **sets mIsOver and ShowFinger(true) if mDoFinger** (port: WidgetManager does this)
+60 | 24 | 00437930 | MouseLeave() | 35 | MouseLeave | mIsOver = false, ShowFinger(false) if mDoFinger
+64 | 25 | 004220f0 | MouseMove(int x, int y) | 36 | MouseMove | empty
+68 | 26 | 00492cd0 | MouseDown(int x, int y, int btn, int cc) | 37 | MouseDown(x,y,btn,cc) | empty
+6c | 27 | 00437950 | MouseDown(int x, int y, int cc) | 38 | MouseDown(x,y,cc) | **sets mIsDown = true**, then slot 26 with (2,1) for cc==3, (0,cc) for cc>=0, (1,-cc) for cc<0
+70 | 28 | 00492cd0 | MouseUp(int x, int y, int btn, int cc) | 39 | MouseUp(x,y,btn,cc) | empty
+74 | 29 | 004379b0 | MouseUp(int x, int y, int cc) | 40 | MouseUp(x,y,cc) | calls slot 30 first, then slot 28 (same split as slot 27)
+78 | 30 | 004379a0 | MouseUp(int x, int y) | 41 | MouseUp(x,y) | **mIsDown = false** (port: WidgetManager clears it)
+7c | 31 | 004220f0 | MouseDrag(int x, int y) | 42 | MouseDrag | empty
+80 | 32 | 00401570 | MouseWheel(int delta) | 43 | MouseWheel | empty; WM sends it to the focus widget
+84 | 33 | 004292c0 | bool IsPointVisible(int x, int y) | 44 | IsPointVisible | true
+88 | 34 | 004388e0 | Rect WriteCenteredLine(Graphics*, int off, const string&, Color c1, Color c2, const Point& shadow) | 45 | same | shadow in c2 at +shadow, text in c1
+8c | 35 | 00438870 | Rect WriteCenteredLine(Graphics*, int off, const string&) | 46 | same |
+90 | 36 | 0043b420 | int WriteString(Graphics*, const string&, int x, int y, int w=-1, int just=-1, bool draw=true, int off=0, int len=-1) | 47 | WriteString | the full colour-code writer lives here in 2004 (see 1.4)
+94 | 37 | 00439f70 | int WriteWordWrapped(Graphics*, const Rect&, const string&, int lineSpacing, int just) | 48 | WriteWordWrapped | see 1.4
+98 | 38 | 004389f0 | int GetWordWrappedHeight(Graphics*, int w, const string&, int lineSpacing) | 49 | GetWordWrappedHeight | WriteWordWrapped into a NULL Graphics, Rect(0,0,w,0), just -1
+9c | 39 | 00437a10 | int GetNumDigits(int) | 50 | GetNumDigits |
+a0 | 40 | 00437a40 | WriteNumberFromStrip(Graphics*, int n, int x, int y, Image* strip, int spacing) | 51 | WriteNumberFromStrip | identical
+a4 | 41 | 00437b40 | bool Contains(int x, int y) | 52 | Contains |
+a8 | 42 | 00438a80 | Rect GetInsetRect() | 53 | GetInsetRect |
+ac | 43 | 00438ac0 | Rect GetRect() | 1 | WidgetContainer::GetRect |
+b0 | 44 | 00438af0 | bool Intersects(Widget*) | 2 | WidgetContainer::Intersects |
+b4 | 45 | 0043a1f0 | MarkDirtyFull() | 6 | WidgetContainer::MarkDirtyFull | wm ? wm->MarkDirtyFull(this) : mDirty = true
+b8 | 46 | 004398b0 | MarkDirty() | 4 | WidgetContainer::MarkDirty | wm ? wm->MarkDirty(this) : mDirty = true

Not in 2004 (present in 1.2/port): UpdateF, UpdateAll/UpdateFAll/DrawAll, MarkDirty(WidgetContainer*)
/MarkDirtyFull(WidgetContainer*) as Widget virtuals, DrawOverlay, GetColor(idx, default), all child
management, Layout, DeferOverlay. Game code cannot call them, so nothing to translate.

#### 1.2 Fields

Offset | type | name | meaning | 1.2 offset | port field | status
---|---|---|---|---|---|---
+00 | vfptr | | | 0x00 | | verified
+04 | ulong | mLastWMUpdateCount | last WM update count seen (UpdateFrame calls Update once per frame) | 0x20 | WidgetContainer::mLastWMUpdateCount | verified (0x439820)
+08 | int | mX | screen x | 0x2c | mX | verified
+0c | int | mY | | 0x30 | mY | verified
+10 | int | mWidth | | 0x34 | mWidth | verified
+14 | int | mHeight | | 0x38 | mHeight | verified
+18 | bool | mDirty | ctor **true** | 0x28 | mDirty | verified
+19 | bool | mVisible | ctor true | 0x4c | mVisible | verified
+1a | bool | mMouseVisible | ctor true; receives mouse | 0x4d | mMouseVisible | verified (GetWidgetAt)
+1b | bool | mClip | ctor true; DrawScreen clips to the widget only when set | 0x3d | mClip | verified (0x4394a0)
+1c | bool | mTransient (name inferred) | drawn on top of a buffered background every redraw; excluded from dirty propagation | - | none | verified behaviour
+1d | bool | mDisabled | | 0x4e | mDisabled | verified
+1e | bool | mHasFocus | | 0x4f | mHasFocus | verified
+1f | bool | mIsDown | set/cleared by the base mouse handlers | 0x50 | mIsDown | verified
+20 | bool | mIsOver | set/cleared by base MouseEnter/Leave | 0x51 | mIsOver | verified
+21 | bool | mHasTransparencies | stops the "fully covered" shortcut in MarkDirtyFull | 0x52 | mHasTransparencies | inferred (MarkDirtyFull + Dialog ctor)
+22 | bool | mHasAlpha | MarkDirty escalates to MarkDirtyFull | 0x3c | mHasAlpha | verified
+24 | WidgetManager* | mWidgetManager | | 0x10 | mWidgetManager | verified
+28 | std::vector<Color> (16) | mColors | first +0x2c, last +0x30 | 0x54 | mColors | verified
+38 | Insets (16) | mMouseInsets | mLeft +0x38, mTop +0x3c, mRight +0x40, mBottom +0x44 | 0x64 | mMouseInsets | verified (GetInsetRect)
+48 | bool | mDoFinger | | 0x74 | mDoFinger | verified
+4c | int | mUpdateCnt | Update() counter | 0x24 | mUpdateCnt | verified
+50 | Widget* | mTabPrev | | 0x78 | mTabPrev | verified (KeyDown)
+54 | Widget* | mTabNext | | 0x7c | mTabNext | verified

No `mWantsFocus`, `mParent`, `mWidgets`, `mPriority`, `mZOrder`, `mWidgetFlagsMod`,
`mUpdateIteratorModified`.

Static data: `Widget::mWriteColoredString` at 0x52f96c = true.

#### 1.3 Direct calls the game makes

address | function | game calls
---|---|---
0043e4e0 | Widget::Widget() | 12 (base ctor of game widgets)
0043ab10 | ~Widget body | 12 (base dtor)
00437800 / 00437810 | Widget::AddedToManager / RemovedFromManager (base calls) | 8 / 8
004378d0 | Widget::Update (base call) | 10
00437840 | Widget::Move | 1
00437910 / 00437930 | Widget::MouseEnter / MouseLeave (base calls from Lex, Board) | 1 / 2
00437950 / 004379b0 | Widget::MouseDown(x,y,cc) / MouseUp(x,y,cc) (base calls) | 2 / 2
00439e50 / 00439e90 / 00439f00 | SetVisible / Resize / SetDisabled (qualified calls) | 4 / 5 / 5
0043ecf0 / 0043ed70 / 0043ee00 | SetColors([][4]) / SetColor / GetColor | 2 / 1 / 1

#### 1.4 WriteString / WriteWordWrapped (in Widget, not Graphics)

- `WriteString` (0x43b420) is the 1.2 `Graphics::WriteString` body moved into Widget: justification 0
  centres, 1 right-aligns (measured with drawString=false), `^^` is a literal `^`, `^RRGGBB?` (the `^`,
  6 hex digits, and one more character, normally the closing `^`, i.e. `^006000^`) sets the colour keeping
  the current alpha; a `^` closer than 8 chars to the end stops the string. Always uses the static
  `Widget::mWriteColoredString` (true). Port `Widget::WriteString` sets `g->mWriteColoredString` from the
  same static and calls Graphics: same output, except the port also understands `^oldclr^`.
- `WriteWordWrapped` (0x439f70) = 1.2 algorithm (width by `CharWidthKern`, `\n` forces a wrap, colour codes
  skipped when measuring, returns `yOffset + descent - lineSpacing`, restores the colour). Two differences
  from the port's 1.3 `Graphics::WriteWordWrapped`: after wrapping at a space it does **not** skip further
  spaces (1.3 skips the whole run), and it never culls lines outside the clip rect. Only texts with double
  spaces at a wrap point differ; BookWorm's own texts have none (only framework/library strings do).

### 2. WidgetManager

sizeof **0x174** (allocated by SexyApp at 0x48aaf0, `operator new(0x174)`; 1.2: 0x1d4). No base class;
vtable `0x5017a0` has 1 slot (deleting dtor 0x43aae0; body 0x43a9c0). App field `mWidgetManager` is at
app+0x2e4 as in 1.2. ctor `0x43eb10(SexyApp*)`.

#### 2.1 Fields

Offset | type | name | meaning | 1.2 offset | port field | status
---|---|---|---|---|---|---
+00 | vfptr | | | | | verified
+04 | int | mUpdateCnt | UpdateFrame counter | 0x24 | mUpdateCnt | verified
+08 | Widget* | mDefaultTab | ctrl+TAB target | 0x4c | mDefaultTab | verified (KeyChar)
+0c | SexyApp* | mApp | game reads it (`mWidgetManager->mApp`) | 0x54 | mApp | verified
+10 | Image* | mImage | screen image, set by the app | 0x58 | mImage | verified
+14 | DDImage* | mTransientImage | background buffer for transient widgets, created in DrawScreen | 0x5c | mTransientImage | verified
+18 | bool | mLastHadTransients | | 0x60 | mLastHadTransients | verified
+1c | int | mWidth | | 0x34 | mWidth | verified
+20 | int | mHeight | | 0x38 | mHeight | verified
+24 | Widget* | mPopupCommandWidget | removed on a click outside it | 0x64 | mPopupCommandWidget | verified
+28 | CursorWidget* | mCursorWidget (inferred name) | created by the ctor, deleted by the dtor | - | none | verified
+2c | bool | mShowCursorWidget (inferred name) | if set: AddWidget/BringToFront keep the cursor widget on top and mouse moves move it to (mouse - hotspot) | - | none | verified behaviour; never set by BookWorm (unverified for indirect writes)
+30 | std::list<Widget*> | mWidgets | head +0x34, size +0x38; back = top | 0x04 | mWidgets | verified
+3c | bool | mHasFocus | ctor true | 0x7c | mHasFocus | verified
+40 | Widget* | mFocusWidget | | 0x80 | mFocusWidget | verified
+44 | Widget* | mLastDownWidget | | 0x84 | mLastDownWidget | verified
+48 | Widget* | mOverWidget | | 0x88 | mOverWidget | verified
+4c | Widget* | mBaseModalWidget | single pointer, no PreModalInfo stack | 0x8c | mBaseModalWidget | verified
+50 | bool | mMouseIn | | 0xb4 | mMouseIn | verified
+54 | int | mLastMouseX | game reads it | 0xb8 | mLastMouseX | verified
+58 | int | mLastMouseY | game reads it | 0xbc | mLastMouseY | verified
+5c | int | mDownButtons | bit0 left, bit1 right, bit2 middle (no separate mActualDownButtons) | 0xc0 | mDownButtons | verified
+60 | bool | mUpdateIteratorModified | | 0x18 | mUpdateIteratorModified | verified
+64 | list iterator | mUpdateIterator | | 0x1c | mUpdateIterator | verified
+68 | bool[255] | mKeyDown | VK-indexed; +0x78 = shift, +0x79 = control | 0xcc | mKeyDown | verified
+168 | DWORD | mLastMouseDownTick (name inferred) | GetTickCount() at every MouseDown; **read by the game** (Lex::Update 0x42dd20: idle > 12000 ms) | - | **none** | verified
+16c | int | mLastDownButtonId | 1 left, -1 right, 2 middle | 0x1cc | mLastDownButtonId | verified
+170 | bool | mDirty | set by MarkDirty/MarkDirtyFull, cleared and returned by UpdateFrame | 0x28 | mDirty | verified

Missing vs 1.2/port: mCurG, the deferred-overlay vector, all FlagsMod members, mPreModalInfoList,
mActualDownButtons, mLastInputUpdateCnt, mWidgetFlags, mMouseDestRect/mMouseSourceRect.

#### 2.2 Functions (all direct calls)

address | 2004 function | 1.2 / port equivalent | notes
---|---|---|---
0043eb10 | WidgetManager(SexyApp*) | WidgetManager(SexyAppBase*) | creates the CursorWidget
0043a9c0 / 0043aae0 | ~WidgetManager body / deleting | ~WidgetManager |
00437540 | FreeResources() (inferred) | FreeResources | deletes mTransientImage only
004375b0 | Resize(int w, int h) | port: Resize(Rect,Rect) | drops mTransientImage
0043ebe0 | AddWidget(Widget*) | AddWidget | if absent: push_back (top), w->AddedToManager(this), MarkDirty(w); then re-raise cursor widget (if +0x2c) and RehupMouse. **23 game calls**
0043a7a0 | RemoveWidget(Widget*) | RemoveWidget | MarkDirtyFull(w), erase (update-iterator safe), DisableWidget(w), w->RemovedFromManager(this). **28 game calls**
00439280 | bool HasWidget(Widget*) | HasWidget | 3 game calls
004392c0 | SetFocus(Widget*) | SetFocus | old->LostFocus() **even if it is the same widget**; focus only if HasWidget; GotFocus if mHasFocus. 8 game calls
0043e2a0 | BringToFront(Widget*) | BringToFront | move to back of list, OrderInManagerChanged; no dirty marking. 2 game calls
0043e360 | BringToBack(Widget*) | BringToBack | 2 game calls
0043e410 | PutInfront(Widget* w, Widget* ref) | PutInfront | 3 game calls (Dialog/ListWidget use it too). No PutBehind linked
004393b0 | MarkAllDirty() | MarkAllDirty | sets every widget's mDirty (not WM+0x170). 1 game call
004393d0 | DrawWidgetsTo(Graphics*) | DrawWidgetsTo | every visible widget, clipped when mClip. 2 game calls (BWGameSelector)
00439310 | MarkDirty(Widget*) | WidgetContainer::MarkDirty(WidgetContainer*) | 1.2 algorithm; transient widgets do not propagate
00439980 | MarkDirtyFull(Widget*) | WidgetContainer::MarkDirtyFull(WidgetContainer*) | 1.2 algorithm (stops at an opaque widget fully covering the rect clipped to the screen)
004387a0 | Rect::Intersection | TRect::Intersection | shared with Graphics
00439200 | SetBaseModal(Widget*) | AddBaseModal / RemoveBaseModal | see 2.3
004391b0 | bool IsBelow(Widget*, Widget*) | IsBelow |
004375f0 | DisableWidget(Widget*) | DisableWidget | over -> MouseLeave; lastdown -> DoMouseUps; focus -> LostFocus; basemodal -> NULL
00437560 | DoMouseUps(Widget*, ulong downCode) | DoMouseUps | **passes the button index 0/1/2 as click count** (1.2/port: 1/-1/3)
0043a830 | RemovePopupCommandWidget() | same |
00439b00 | Widget* GetWidgetAt(int x, int y) | GetWidgetAt | top-down; skips !mVisible, !mMouseVisible; tests GetInsetRect().Contains then IsPointVisible(local); a disabled hit returns NULL; stops after passing mBaseModalWidget
00439bd0 | MousePosition(int x, int y) | MousePosition | as 1.2 (MouseLeave old, MouseEnter + MouseMove new); moves the cursor widget if +0x2c
00439cb0 | RehupMouse() | RehupMouse | as 1.2
0043a850 | bool MouseDown(int x, int y, int cc) | MouseDown | see D6
00439d00 | bool MouseUp(int x, int y, int cc) | MouseUp | button bit from cc (cc<0 right, cc==3 middle, else left); MouseUp(x,y,cc) on mLastDownWidget, then MousePosition
00439d70 | bool MouseMove(int x, int y) | MouseMove | mMouseIn = true, MousePosition
00439d90 | bool MouseDrag(int x, int y) | MouseDrag | as 1.2 (MouseDrag on mLastDownWidget, over-state follows the cursor)
00437710 | bool MouseExit(int x, int y) | MouseExit |
00437730 | MouseWheel(int) | MouseWheel | to mFocusWidget
00437750 | bool KeyChar(char) | KeyChar | ctrl+TAB -> mDefaultTab->KeyChar(TAB)
00437790 / 004377c0 | bool KeyDown(KeyCode) / KeyUp(KeyCode) | KeyDown / KeyUp | mKeyDown[] for 0..254; KeyUp swallows TAB while ctrl is down
00437650 / 00438830 | GotFocus() / LostFocus() | same | LostFocus sends KeyUp for every held key
00439820 | bool UpdateFrame() | UpdateFrame | mDirty = false; mUpdateCnt++; Update() on **every** widget (visible or not) whose mLastWMUpdateCount differs; returns mDirty
004394a0 | bool DrawScreen() | DrawScreen | see 2.4

#### 2.3 Modal handling

One pointer, `mBaseModalWidget` (+0x4c). `SetBaseModal(w)` sets it and drops the over widget, the
last-down widget (DoMouseUps) and the focus widget when they are below `w`. GetWidgetAt never returns a
widget below it; widgets below still Update and Draw; keys still go to mFocusWidget. The app does the rest
(app group): `SexyApp::AddDialog` (0x489220) = KillDialog(id), add to its map/list, `AddWidget`, and if
`IsModal()` `SetBaseModal(dialog)` + ModalOpen; `KillDialog` (0x482f90) = ModalClose if modal,
`RemoveWidget` (which clears mBaseModalWidget through DisableWidget), deferred delete, then if no base
modal is left it re-scans its dialog list from the newest and `SetBaseModal`s the first modal one.
Port: SexyAppBase::AddDialog/KillDialog with Add/RemoveBaseModal give the same result; nothing to do.

#### 2.4 Drawing

Only dirty widgets are redrawn (as in 1.2). Transient widgets (+0x1c) are drawn last over a buffered copy
of the non-transient widgets (`mTransientImage`, a DDImage the size of the WM). Each widget is drawn with
a copy of the screen Graphics translated to (mX, mY) and clipped to its rect **only if mClip**. No
DrawOverlay, no modal flags, no SetFastStretch/SetLinearBlend. Port: the 1.3 DrawScreen draws the same
dirty widgets in the same order; the transient layer is only used by the CursorWidget, which BookWorm
never shows. Nothing to do.

### 3. Listener interfaces

ButtonListener (vtable 0x4f93ac, 6 slots; 1.2 has 7; port `ButtonListener.h` has 7, call by name):

slot | address (default) | 2004 method | 1.2 slot | port
---|---|---|---|---
0 (+00) | 00401570 | ButtonMouseEnter(int id) | 4 | ButtonMouseEnter
1 (+04) | 00401570 | ButtonMouseLeave(int id) | 5 | ButtonMouseLeave
2 (+08) | 00401560 | ButtonPress(int id, int clickCount) { ButtonPress(id); } | 0 | ButtonPress(id, cc)
3 (+0c) | 00401570 | ButtonPress(int id) | 1 | ButtonPress(id)
4 (+10) | 00401570 | ButtonDepress(int id) | 2 | ButtonDepress
5 (+14) | 00401570 | ButtonDownTick(int id) | 3 | ButtonDownTick
- | - | (no ButtonMouseMove) | 6 | ButtonMouseMove (never called for 2004 code)

In Widget-derived game classes this vtable is at +0x58 (`Board`, `TitleScreen`, `BWGameSelector`,
`LevelUpWidget`, `FameWidget`, `HelpWidget`, all dialogs); in SexyApp/BookWormApp it is the primary vtable
(slots 0-5 of the 67/68-slot app vtable; BookWormApp overrides 3 ButtonPress and 4 ButtonDepress).
`this` inside a +0x58 thunk is `object+0x58`, so `this+0xa8` there is the Dialog's `mId` (+0x100) and
`this-0x34` is `mWidgetManager` (+0x24).

Other interfaces (1 vtable each, all defaults are folded stubs):

interface | vtable | slots (2004 = 1.2 = port order) | notes
---|---|---|---
CheckboxListener | 004fc4f4 | 0 CheckboxChecked(int id, bool checked) | same as 1.2/port
SliderListener | 0050095c | 0 SliderVal(int id, double val) | same
EditListener | 004fcba0 | 0 EditWidgetText(int id, const string&), 1 bool AllowKey(int id, KeyCode), 2 bool AllowChar(int id, char), 3 bool AllowText(int id, const string&) | same; Allow* default true
ListListener | 004fc288 | 0 ListClicked(int id, int idx, int clickCount) | **only one slot**; port adds ListClosed and ListHiliteChanged (defaults are no-ops)
ScrollListener | 00501798 | 0 ScrollPosition(int id, double pos) | same; ListWidget implements it at +0x58
DialogListener | - | not present in 2004 | port Dialog uses it; see D1

Game layouts seen: dialogs with an EditListener/ListListener/SliderListener/CheckboxListener put it at
+0x110 (Dialog subclass, e.g. GetStringDialog, RegisterDialog) or after the MoneyDialog members at +0x120 /
+0x124 (NewUserDialog, SelectUserDialog, DeleteUserDialog, OptionsDialog); FameWidget/HelpWidget put
CheckboxListener at +0x5c.

### 4. ButtonWidget

sizeof **0xe0** (1.2: 0x118). Base Widget. Vtable `0x501944`, 49 slots. ctor `0x43f080(int theId,
ButtonListener* theListener)`, dtor body `0x43b170`, deleting `0x43d350`.

#### 4.1 Vtable (slots not listed = Widget's)

slot | address | method | 1.2 slot | port | notes
---|---|---|---|---|---
0 | 0043d350 | ~ButtonWidget | 0 | ~ButtonWidget | deletes mFont
10 | 0043a480 | SetDisabled(bool) | 22 | SetDisabled | MarkDirty if mDisabledImage
16 | 0043d380 | Draw(Graphics*) | 13 | Draw | see 4.3
17 | 004383a0 | Update() | 9 | Update | see 4.3
23 | 0043a4b0 | MouseEnter() | 34 | MouseEnter | Widget::MouseEnter; over-alpha reset; MarkDirtyFull if down / over image / hilite colour differs; listener slot 0 **if !mDisabled**
24 | 0043a550 | MouseLeave() | 35 | MouseLeave | as 1.2; listener slot 1 if !mDisabled
27 | 00438300 | MouseDown(int x, int y, int cc) | 38 (same override in Big Money's 1.2) | MouseDown(x,y,btn,cc) (port overrides the 4-arg one) | Widget::MouseDown(x,y,cc); **if !mDisabled** ButtonPress(mId, cc) (listener slot 2); MarkDirtyFull
30 | 00438350 | MouseUp(int x, int y) | 41 | MouseUp(x,y,btn,cc) | Widget::MouseUp(x,y); **if mIsOver && !mDisabled** ButtonDepress(mId) (slot 4); then MarkDirtyFull (port: MarkDirty first, checks mWidgetManager->mHasFocus)
47 | 00438240 | SetFont(Font*) | 55 | SetFont | delete old, Duplicate()
48 | 00438270 | bool IsButtonDown() | 56 | IsButtonDown | mIsDown && mIsOver && !mDisabled

No `DrawButtonImage` virtual (1.2 slot 54): it is non-virtual at 0x4382c0; `HaveButtonImage` 0x438290.
No MouseMove override, so no ButtonMouseMove.

#### 4.2 Fields

Offset | type | name | ctor | 1.2 offset | port | status
---|---|---|---|---|---|---
+58 | int | mId | theId | 0x80 | mId | verified
+5c | std::string | mLabel | "" | 0x84 | mLabel | verified
+78 | int | mLabelJustify | 0 (center; -1 left, 1 right) | 0xa0 | mLabelJustify | verified (Draw)
+7c | Font* | mFont | NULL | 0xa4 | mFont | verified
+80 | Image* | mButtonImage | NULL | 0xa8 | mButtonImage | verified
+84 | Image* | mOverImage | NULL | 0xac | mOverImage | verified
+88 | Image* | mDownImage | NULL | 0xb0 | mDownImage | verified
+8c | Image* | mDisabledImage | NULL | 0xb4 | mDisabledImage | verified
+90 | Rect | mNormalRect | 0 | 0xb8 | mNormalRect | verified
+a0 | Rect | mOverRect | 0 | 0xc8 | mOverRect | verified
+b0 | Rect | mDownRect | 0 | 0xd8 | mDownRect | verified
+c0 | bool | mInverted | false | 0xf8 | mInverted | verified
+c1 | bool | mBtnNoDraw | false | 0xf9 | mBtnNoDraw | verified
+c2 | bool | mFrameNoDraw | false | 0xfa | mFrameNoDraw | verified
+c4 | ButtonListener* | mButtonListener | theListener | 0xfc | mButtonListener | verified
+c8 | double | mOverAlpha | 0 | 0x100 | mOverAlpha | verified
+d0 | double | mOverAlphaSpeed | 0 (game sets 0.08) | 0x108 | mOverAlphaSpeed | verified
+d8 | double | mOverAlphaFadeInSpeed | 0 | 0x110 | mOverAlphaFadeInSpeed | verified

No `mDisabledRect` (1.2 +0xe8). Colours: `SetColors(gButtonWidgetColors, 2)` = {0,0,0},{0,0,0} (0x535068):
only COLOR_LABEL and COLOR_LABEL_HILITE exist; port sets 6. The ctor does not set mHasAlpha (port: true).

#### 4.3 Behaviour

- Draw (0x43d380): returns if mBtnNoDraw; creates `SysFont(app, "SansSerif", 10)` if mFont is NULL and the
  label is not empty (port: no SysFont fallback, the game always sets fonts); label position as 1.2
  (`x` by mLabelJustify, `y = (h + ascent - ascent/6 - 1)/2`); isDown = IsButtonDown() ^ mInverted.
  Without mButtonImage and mDownImage it draws the 1.2 frame with **hard-coded** colours: fill (212,212,212),
  outlines black / white / (132,132,132) (port takes them from mColors[2..5], same default values); the
  pressed label is always **black** at +1,+1 (port: hilite or label colour); the unpressed label uses
  mColors[1] if mIsOver else mColors[0]. With images: identical to 1.2 except no disabled rect (disabled:
  `DrawImage(mDisabledImage, 0, 0)`) and, for the over-alpha fade, the normal image is drawn under the
  over image even when mOverAlpha reaches 1.
- Update (0x4383a0): Widget::Update; if mIsDown && mIsOver && !mDisabled -> ButtonDownTick(mId); over-alpha
  fade out (not down, not over) or fade in (over), each followed by MarkDirty. Same as Zuma and 1.2/1.3
  apart from the !mDisabled test.

### 5. DialogButton

sizeof **0xf8** (1.2: 0x130). Base ButtonWidget. Vtable `0x501fa4`, 49 slots: slot 0 = shared deleting
dtor 0x43f2d0 (no own destructor), slot 16 Draw 0x43dc90; everything else ButtonWidget's. ctor
`0x43f240(Image* theComponentImage, int theId, ButtonListener* theListener)`.

Offset | type | name | ctor | 1.2 offset | port
---|---|---|---|---|---
+e0 | Image* | mComponentImage | arg | 0x118 | mComponentImage
+e4 | int | mTranslateX | 1 | 0x11c | mTranslateX
+e8 | int | mTranslateY | 1 | 0x120 | mTranslateY
+ec | int | mTextOffsetX | 0 | 0x124 | mTextOffsetX
+f0 | int | mTextOffsetY | 0 | 0x128 | mTextOffsetY

ctor also sets mDoFinger = true and `SetColors(gDialogButtonColors, 2)` = white, white (0x52fa18;
port: 6 colours, label white/white too).

Draw (0x43dc90): without mComponentImage -> ButtonWidget::Draw. Otherwise: SysFont "SansSerif" 12 if no
font; down = IsButtonDown(); with mNormalRect.mWidth == 0: translate by (mTranslateX, mTranslateY) when
down, `DrawImageBox(Rect(0,0,w,h), mComponentImage)`; else DrawImageBox from mDownRect / mOverRect /
mNormalRect, then translate when down. Then **SetColorizeImages(true)**, colour mColors[1] if mIsOver else
mColors[0], label centred at `(w - width)/2 + mTextOffsetX`, `(h - ascent/6 - ascentPadding - 1 + ascent)/2
+ mTextOffsetY`, SetColorizeImages(false), and when down **Translate(-1, -1)** (hard-coded; the game never
changes mTranslateX/Y, so harmless). Differences from the port: no mBtnNoDraw test on the image path, no
disabled rect, no over-alpha fade. Nothing to do.

### 6. Dialog

sizeof **0x110** (1.2: 0x148). Bases Widget, ButtonListener at **+0x58**. Vtables `0x501a2c` (52 slots)
and `0x501a0c` (+0x58, 6 slots). ctor `0x43f3b0(Image* theComponentImage, Image* theButtonComponentImage,
int theId, bool isModal, const string& header, const string& lines, const string& footer, int theButtonMode)`
(same signature as 1.2/port), dtor body `0x43b1f0` (deletes both buttons and both fonts), deleting
`0x43d920`. Game dialog classes: CreditsDialog, HintDialog, MoneyDialog (base of NewUser/SelectUser/
DeleteUser/OptionsDialog), WaitDialog, GetStringDialog, RegisterDialog; framework: UpdateCheckDialog.

#### 6.1 Vtable (slots not listed = Widget's)

slot | address | method | 1.2 slot | port | notes
---|---|---|---|---|---
0 | 0043d920 | ~Dialog | 0 | ~Dialog |
1 | 0043f200 | AddedToManager(WM*) | 7 | AddedToManager | Widget's, then wm->AddWidget(yes), AddWidget(no)
2 | 0043a980 | RemovedFromManager(WM*) | 8 | RemovedFromManager | Widget's, then RemoveWidget(yes/no)
3 | 0043e930 | OrderInManagerChanged() | 15 | OrderInManagerChanged | PutInfront(yes, this), PutInfront(no, this)
7 | 0043f190 | SetColor(int, const Color&) | 19 | SetColor | idx 3 -> buttons' colour 0, idx 4 -> buttons' colour 1
13 | 0043a620 | Resize(x,y,w,h) | 25 | Resize | see 6.3
16 | 0043d950 | Draw(Graphics*) | 13 | Draw | see 6.3
17 | 004386c0 | Update() | 9 | Update | `jmp Widget::Update`
27 | 00438530 | MouseDown(x,y,cc) | 38 | MouseDown(x,y,btn,cc) | only if cc == 1: SetCursor(2), mDragging, mDragMouseX/Y. Does **not** call Widget::MouseDown
30 | 00438690 | MouseUp(x,y) | 41 | MouseUp(x,y,btn,cc) | if mDragging: SetCursor(0), mDragging = false
31 | 00438570 | MouseDrag(x,y) | 42 | MouseDrag | identical to 1.2 (keeps the dialog within -8..+8 of the WM size)
47 | 004384a0 | SetButtonFont(Font*) | 54 | SetButtonFont | yes/no->SetFont
48 | 004384d0 | SetHeaderFont(Font*) | 55 | SetHeaderFont | Duplicate
49 | 00438500 | SetLinesFont(Font*) | 56 | SetLinesFont | Duplicate
50 | 00438ec0 | int GetPreferredHeight(int w) | 57 | GetPreferredHeight | 1.2 formula; button height = yes->mComponentImage->GetHeight(); no EnsureFonts
51 | 004386d0 | bool IsModal() | 58 | IsModal |
- | - | (no WaitForResult) | 59 | WaitForResult | do not use

ButtonListener at +0x58 (vtable 0x501a0c): slots 0/1 stubs, 2 = ButtonListener::ButtonPress(id,cc),
**3 ButtonPress(id) 0x4386e0, 4 ButtonDepress(id) 0x438720, 5 ButtonDownTick(id) 0x438760**: each does
`if (id == mId + 2000 || id == mId + 3000) mWidgetManager->mApp->ButtonPress / ButtonDepress /
ButtonDownTick(id)` (app vtable slots 3 / 4 / 5). Game dialogs override slots 2-4 at +0x58 and call these
(e.g. HintDialog 0x42a920/0x42a950, DeleteUserDialog 0x41e220, SelectUserDialog 0x432720, OptionsDialog,
RegisterDialog) and compare `theId == mId + 2000` themselves; BookWormApp::ButtonDepress (0x417080)
decodes 2000..2999 as "yes/ok of dialog id-2000" and 3000.. as "no/cancel of dialog id-3000".

#### 6.2 Fields

Offset | type | name | ctor | 1.2 offset | port | status
---|---|---|---|---|---|---
+58 | vfptr | ButtonListener | | 0x80 | | verified
+5c | Image* | mComponentImage | arg | 0x88 | mComponentImage | verified
+60 | DialogButton* | mYesButton | | 0x8c | mYesButton | verified
+64 | DialogButton* | mNoButton | | 0x90 | mNoButton | verified
+68 | std::string | mDialogHeader | | 0x98 | mDialogHeader | verified
+84 | std::string | mDialogFooter | | 0xb4 | mDialogFooter | verified
+a0 | std::string | mDialogLines | | 0xd0 | mDialogLines | verified
+bc | int | mButtonMode | 0 none, 1 YES_NO, 2 OK_CANCEL, 3 FOOTER | 0xec | mButtonMode | verified
+c0 | Font* | mHeaderFont | NULL | 0xf0 | mHeaderFont | verified
+c4 | Font* | mLinesFont | NULL | 0xf4 | mLinesFont | verified
+c8 | int | mTextAlign | 0 | 0xf8 | mTextAlign | verified (Draw)
+cc | int | mLineSpacingOffset | 0 | 0xfc | mLineSpacingOffset | verified
+d0 | Insets | mBackgroundInsets | 0 | 0x104 | mBackgroundInsets | verified
+e0 | Insets | mContentInsets | (24,24,24,24) | 0x114 | mContentInsets | verified
+f0 | int | mSpaceAfterHeader | 10 | 0x124 | mSpaceAfterHeader | verified
+f4 | bool | mDragging | false | 0x128 | mDragging | verified
+f8 | int | mDragMouseX | | 0x12c | mDragMouseX | verified
+fc | int | mDragMouseY | | 0x130 | mDragMouseY | verified
+100 | int | mId | theId | 0x134 | mId | verified
+104 | bool | mIsModal | isModal | 0x138 | mIsModal | verified
+108 | int | mButtonHorzSpacing | 8 | 0x140 | mButtonHorzSpacing | verified (Resize)
+10c | int | mButtonSidePadding | 0 | 0x144 | mButtonSidePadding | verified (Resize)

Not in 2004: mDialogListener, mNumButtons, mButtonHeight, mResult, mPriority (= 1 in the port).
ctor also sets mHasTransparencies = mHasAlpha = true and `SetColors(gDialogColors, 5)` (0x52f9e8):
header white, lines yellow, footer white, button text white, button hilite white (COLOR_HEADER..
COLOR_BUTTON_TEXT_HILITE; no COLOR_BKG/COLOR_OUTLINE). Buttons: modes 1/2 create two DialogButtons
(`new DialogButton(theButtonComponentImage, mId+2000 / mId+3000, this+0x58)`) labelled from the global
strings `DIALOG_YES_STRING` "YES" / `DIALOG_NO_STRING` "NO" (mode 1) or "OK" / "CANCEL" (mode 2)
(0x52fa30/4c/68/84, plain constants, **no GetString lookup**); mode 3 creates one button `mId+2000`
labelled with the footer.

#### 6.3 Layout and drawing

- Resize (0x43a620): as 1.2 but the button height is `mYesButton->mComponentImage->GetHeight()` (crashes
  without a button image; port uses mButtonHeight = image height or 24: same value when an image exists).
- Draw (0x43d950): creates SysFont "SansSerif" 14 (header) / 12 (lines) when the fonts are NULL (port: no
  fallback, set the fonts); **always** `DrawImageBox(Rect(bL, bT, w-bL-bR, h-bT-bB), mComponentImage)` (no
  grey fallback box); header at `cT+bT+ascent-ascentPadding` with mColors[0] via WriteCenteredLine;
  `+ height - ascent + mSpaceAfterHeader`; lines with mLinesFont, mColors[1], WriteWordWrapped in
  `Rect(bL+cL+2, y, w-cL-cR-bL-bR-4, 0)`, spacing `GetLineSpacing()+mLineSpacingOffset`, mTextAlign; footer
  (if not empty and mode != 3) with mHeaderFont, mColors[2], at `+8 + lineSpacing`. Same numbers as 1.2.
- GetPreferredHeight (0x438ec0): identical formula to 1.2 (header `height - ascentPadding`, lines
  `+mSpaceAfterHeader` + wrapped height, footer `+8 + lineSpacing`, buttons `+8 + buttonHeight + 8`).

UpdateCheckDialog (vtables 0x501b1c / 0x501b00, sizeof 0x120, ctor 0x43f650, Update 0x43b730 polls an
HTTP transfer, Draw 0x43df80, GetPreferredHeight 0x4398d0 = Dialog's + 32; strings UPDATE_CHECK_TITLE /
UPDATE_CHECK_BODY / DIALOG_BUTTON_CANCEL via GetString): created by BookWormApp 0x415de0 as dialog id 8 for
"check for updates". Drop it in the port.

### 7. Checkbox

sizeof **0x8c** (1.2: 0xe4). Base Widget. Vtable `0x501edc`, 49 slots; slot 0 = shared 0x43e8f0.
ctor `0x43e890(Image* theUncheckedImage, Image* theCheckedImage, int theId, CheckboxListener*)` (same as
1.2/port); sets mDoFinger = true.

slot | address | method | 1.2 | port | notes
---|---|---|---|---|---
16 | 00438190 | Draw | 13 | Draw | if mCheckedRect.mWidth: DrawImage(mUncheckedImage, 0,0, mChecked ? mCheckedRect : mUncheckedRect); else DrawImage(mChecked ? mCheckedImage : mUncheckedImage). No NULL checks, no outline fallback
27 | 004381f0 | MouseDown(x,y,cc) | 38 | MouseDown(x,y,btn,cc) | Widget::MouseDown; toggle; listener->CheckboxChecked(mId, mChecked) if listener; MarkDirty
47 | 00438140 | SetChecked(bool, bool tellListener) | 54 | SetChecked | MarkDirty
48 | 00438180 | bool IsChecked() | 55 | IsChecked |

Offset | type | name | 1.2 offset | port
---|---|---|---|---
+58 | CheckboxListener* | mListener | 0x80 | mListener
+5c | int | mId | 0x84 | mId
+60 | bool | mChecked | 0x88 | mChecked
+64 | Image* | mUncheckedImage | 0x8c | mUncheckedImage
+68 | Image* | mCheckedImage | 0x90 | mCheckedImage
+6c | Rect | mCheckedRect | 0x94 | mCheckedRect
+7c | Rect | mUncheckedRect | 0xa4 | mUncheckedRect

No mOutlineColor/mBkgColor/mCheckColor. All verified (ctor + Draw).

### 8. Slider

sizeof **0x80** (`operator new(0x80)` in OptionsDialog; 1.2: 0xa8). Base Widget. Vtable `0x501c54`,
49 slots; slot 0 = shared 0x43e8f0. ctor `0x43e550(Image* theTrackImage, Image* theThumbImage, int theId,
SliderListener*)`. Horizontal only.

slot | address | method | 1.2 | port | notes
---|---|---|---|---|---
16 | 00438b70 | Draw | 13 | Draw | 1.2 three-part track (track width/3 cells) + thumb at `(int)(mVal*(w - thumbW))`
24 | 00437d60 | MouseLeave | 35 | MouseLeave | SetCursor(pointer) unless dragging
25 | 00437c00 | MouseMove(x,y) | 36 | MouseMove | dragging cursor over the thumb
27 | 00437ba0 | MouseDown(x,y,cc) | 38 | MouseDown(x,y,cc) | starts a drag **only on the thumb**; a click on the track does nothing (1.2/port jump the thumb there)
30 | 00437d30 | MouseUp(x,y) | 41 | MouseUp(x,y) | mDragging = false, SetCursor(0), SliderVal(mId, mVal) (on any mouse-up)
31 | 00437c70 | MouseDrag(x,y) | 42 | MouseDrag | mVal = (x - mRelX)/(w - thumbW) clamped 0..1; SliderVal + MarkDirtyFull if changed
47 | 00437b80 | SetValue(double) | 54 | SetValue | **no clamping**; MarkDirtyFull
48 | 00421850 | bool HasTransparencies() | 55 | HasTransparencies | true (folded stub)

Offset | type | name | 1.2 offset | port
---|---|---|---|---
+58 | SliderListener* | mListener | 0x80 | mListener
+60 | double | mVal | 0x88 | mVal
+68 | int | mId | 0x90 | mId
+6c | Image* | mTrackImage | 0x94 | mTrackImage
+70 | Image* | mThumbImage | 0x98 | mThumbImage
+74 | bool | mDragging | 0x9c | mDragging
+78 | int | mRelX | 0xa0 | mRelX

No mRelY/mHorizontal. All verified.

### 9. EditWidget

sizeof **0xec** (1.2: 0x11c). Base Widget. Vtable `0x50186c`, 53 slots. ctor `0x43ef90(int theId,
EditListener*)`, dtor body `0x43aef0` (deletes mFont and mWidthCheckFont), deleting `0x43beb0`.

slot | address | method | 1.2 slot | port | notes
---|---|---|---|---|---
13 | 0043a450 | Resize | 25 | Resize | + FocusCursor(false)
15 | 00421850 | WantsFocus | 27 | WantsFocus | true
16 | 0043bf20 | Draw | 13 | Draw | as 1.2: bkg mColors[0], two clipped passes (text mColors[2]; selection fill mColors[3] + text mColors[4] when focused), outline mColors[1] DrawRect; SysFont "SansSerif" 10 fallback
17 | 00437f80 | Update | 9 | Update | blink when focused (mBlinkDelay 40)
18 / 19 | 00437f40 / 00437f60 | GotFocus / LostFocus | 29 / 30 | same |
20 | 00438010 | KeyChar(char) | 31 | KeyChar | AllowChar -> ProcessKey(0, c); no Widget::KeyChar call
21 | 004398f0 | KeyDown(KeyCode) | 32 | KeyDown | keys outside 'A'..'Y' -> AllowKey -> ProcessKey(k, 0); Widget::KeyDown
23 / 24 | 004380e0 / 00438100 | MouseEnter / MouseLeave | 34 / 35 | same | text / pointer cursor
27 | 0043b050 | MouseDown(x,y,cc) | 38 | MouseDown(x,y,btn,cc) | cursor from GetCharAt; cc > 1 selects the word
29 | 00438050 | MouseUp(x,y,cc) | 40 | MouseUp | drop an empty selection
31 | 00438090 | MouseDrag | 42 | MouseDrag | extend selection
46 | 0042b870 | MarkDirty | 4 | MarkDirty | MarkDirtyFull if mColors[0].mAlpha != 255
47 | 0043c3a0 | ProcessKey(KeyCode, char) | 54 | ProcessKey | 1.2 logic: shift-select, ctrl-C/X (app clipboard), ctrl-V (keeps chars passing AllowChar, stops at CR/LF), ctrl-Z undo, arrows (ctrl = word jump), Home/End, Backspace/Delete, Enter -> EditWidgetText, printable 0x20..0x7e; mMaxChars and mMaxPixels truncation; AllowText veto restores the old text
48 | 00437ee0 | SetFont(Font*, Font* widthCheck) | 55 | SetFont(Font*, Font* = NULL) | duplicates both; one width-check font instead of the 1.2 list
49 | 0043bee0 | SetText(const string&) | 56 | SetText(text, leftPosToZero = true) | cursor to end, mHilitePos 0, MarkDirty; **does not touch mLeftPos** (port resets it to 0 by default)
50 | 00437fd0 | bool IsPartOfWord(char) | 57 | IsPartOfWord | [A-Za-z0-9_]
51 | 0043d050 | int GetCharAt(int x, int y) | 58 | GetCharAt |
52 | 0043d1a0 | FocusCursor(bool bigJump) | 59 | FocusCursor | no clamp of mLeftPos to len-1

Non-virtual: GetDisplayString 0x43afd0 (password masking).

Offset | type | name | ctor | 1.2 offset | port
---|---|---|---|---|---
+58 | int | mId | arg | 0x80 | mId
+5c | std::string | mString | | 0x84 | mString
+78 | std::string | mPasswordDisplayString | | 0xa0 | mPasswordDisplayString
+94 | Font* | mFont | NULL | 0xbc | mFont
+98 | Font* | mWidthCheckFont (inferred name) | NULL | (0xc0 WidthCheckList) | mWidthCheckList
+9c | EditListener* | mEditListener | arg | 0xcc | mEditListener
+a0 | bool | mShowingCursor | false | 0xd0 | mShowingCursor
+a4 | int | mCursorPos | 0 | 0xd4 | mCursorPos
+a8 | int | mHilitePos | -1 | 0xd8 | mHilitePos
+ac | int | mBlinkAcc | 0 | 0xdc | mBlinkAcc
+b0 | int | mBlinkDelay | 40 | 0xe0 | mBlinkDelay
+b4 | int | mLeftPos | 0 | 0xe4 | mLeftPos
+b8 | int | mMaxChars | -1 | 0xe8 | mMaxChars
+bc | int | mMaxPixels | -1 | 0xec | mMaxPixels
+c0 | char | mPasswordChar | 0 | 0xf0 | mPasswordChar
+c4 | std::string | mUndoString | | 0xf4 | mUndoString
+e0 | int | mUndoCursor | 0 | 0x110 | mUndoCursor
+e4 | int | mUndoHilitePos | 0 | 0x114 | mUndoHilitePos
+e8 | int | mLastModifyIdx | 0 | 0x118 | mLastModifyIdx

No mDrawSelOverride. Colours `SetColors(gEditWidgetColors, 5)` (0x52f9ac) = white bkg, black outline,
black text, black hilite, white hilite text: same as the port's table. All fields verified from ctor/dtor
and the functions above except +0x98's name.

### 10. ListWidget

sizeof **0xc0** (`operator new(0xc0)`). Bases Widget, ScrollListener at **+0x58**. Vtables `0x501d24`
(60 slots) and `0x501d1c` (+0x58, 1 slot: ScrollPosition 0x437d80). ctor `0x43ee70(int theId, Font*
theFont, ListListener*)`, dtor body `0x43e590`, deleting `0x43ef60`. Used by SelectUserDialog and
DeleteUserDialog (mJustify 1, mDrawOutline 1, 5 colours from the game, AddLine(name, false)).

slot | address | method | port | notes
---|---|---|---|---
3 | 0043e800 | OrderInManagerChanged | same | PutInfront(mChild) and PutInfront(mScrollbar) through gSexyAppBase->mWidgetManager
13 | 0043a210 | Resize | same | mPageSize = (h-8)/itemH if h > itemH+8 else 1; mScrollbar slot 49 (SetPageSize)
16 | 0043ab90 | Draw | same | bkg mColors[0]; rows from (int)mPosition; selected row filled mColors[4]; text mColors[3] if hilited, mColors[5] if selected and 6+ colours exist, else mLineColors[i]; x: 0 -> 4, 1 -> centred, else right-4; outline mColors[1] if mDrawOutline
24 | 00437df0 | MouseLeave | same | hilite -1 on the whole parent/child chain, pointer cursor
25 | 00438d90 | MouseMove(x,y) | same | row under the cursor becomes the hilite of the chain; hand cursor on a row (no mDoFingerWhenHilited, no ListHiliteChanged)
27 | 00437dc0 | MouseDown(x,y,cc) | MouseDown(x,y,btn,cc) | ListClicked(mId, mHiliteIdx, cc) if hilited and listener; no Widget::MouseDown
47 | 0043bb20 | string GetSortKey(int idx) | same | zero-padded to mMaxNumericPlaces, child key when mSortFromChild
48 | 0043f890 | Sort(bool ascending) | same |
49 | 0043ab50 | string GetStringAt(int idx) | same |
50 | 0043fcb0 | int AddLine(const string&, bool alphabetical) | same | adds "" to the child lists
51 | 0043bce0 | SetLine(int, const string&) | same |
52 | 00438d10 | int GetLineCount() | same |
53 | 0043a2a0 | int GetLineIdx(const string&) | same |
54 | 0043a360 | SetColor(const string& line, const Color&) | SetColor(const SexyString&, const Color&) | sets mLineColors on the chain
55 | 0043e610 | RemoveLine(int) | same | mScrollbar slot 48 (SetMaxValue) with the new count
56 | 0043e760 | RemoveAll() | same |
57 | 0043a3e0 | int GetOptimalWidth() | same | widest line + 16
58 | 00438d40 | int GetOptimalHeight() | same | count * itemH + 8
59 | 00437e40 | SetSelect(int) | same |
(+0x58) 0 | 00437d80 | ScrollPosition(int id, double pos) | same | forwards to mChild, mPosition = pos

Not in 2004: SetHilite, SetLineColor, MouseWheel, RemovedFromManager (port's calls ListClosed).

Offset | type | name | ctor | port
---|---|---|---|---
+5c | int | mId | arg | mId
+60 | Font* | mFont | arg | mFont
+64 | ScrollbarWidget* | mScrollbar | NULL (no ScrollbarWidget class is linked; BookWorm leaves it NULL as far as seen) | mScrollbar
+68 | int | mJustify | 0 | mJustify
+6c | std::vector<string> | mLines | | mLines
+7c | std::vector<Color> | mLineColors | | mLineColors
+90 | double | mPosition | 0 | mPosition
+98 | double | mPageSize | 0 | mPageSize
+a0 | int | mHiliteIdx | -1 | mHiliteIdx
+a4 | int | mSelectIdx | -1 | mSelectIdx
+a8 | ListListener* | mListListener | arg | mListListener
+ac | ListWidget* | mParent | NULL | mParent
+b0 | ListWidget* | mChild | NULL | mChild
+b4 | bool | mSortFromChild | false | mSortFromChild (inferred)
+b5 | bool | mDrawOutline | true | mDrawOutline
+b8 | int | mMaxNumericPlaces | 0 | mMaxNumericPlaces
+bc | int | mItemHeight | **-1** (font height used at run time) | mItemHeight (port: font height at ctor; same result)

`SetColors(gInitialListWidgetColors, 5)` (0x52f970): white, white, black, (0,192,0), (0,0,128); 1.2/port add
a 6th (select text, white). No mDrawSelectWhenHilited / mDoFingerWhenHilited.

### 11. HyperlinkWidget

sizeof **0x108** (1.2: 0x140). Base ButtonWidget. Vtable `0x50206c`, 49 slots: 0 shared 0x43f2d0,
16 Draw 0x43bd10, 23 MouseEnter 0x43a940, 24 MouseLeave 0x43a960 (ButtonWidget's + MarkDirtyFull). ctor
`0x43f300(int theId, ButtonListener*)`: mColor (+0xe0) = mOverColor (+0xf0) = (255,255,255), mUnderlineSize
(+0x100) = 1, mUnderlineOffset (+0x104) = 3, mDoFinger = true. Same fields and Draw as 1.2/port (label
centred at `(h + ascent)/2 - 1`, colour by mIsOver, mUnderlineSize 1-pixel lines at +mUnderlineOffset).
2004 Draw measures with mFont **before** its SysFont fallback, so it needs a font set (the game sets one).

### 12. CursorWidget

sizeof **0x5c**. Base Widget. Vtable `0x501e1c`: 0 shared 0x43e8f0, 16 Draw 0x438120 (DrawImage(mImage,0,0)
if set). ctor `0x43e840`: mImage (+0x58) = NULL, mMouseVisible = false, SetTransient(true). Non-virtual
GetHotspot 0x438e70 = (img w/2, img h/2). Owned by the WidgetManager (+0x28) and only shown when WM+0x2c
is set, which BookWorm never does. Drop it.

### 13. Insets, Rect, Point, Color helpers

- Insets: `mLeft, mTop, mRight, mBottom` (as port). `Insets()` = 0x437e80 (folded with `Rect()`),
  `Insets(l,t,r,b)` folded into 0x437e90 (`Color(r,g,b,a)`), copy = 0x437eb0 (folded with Rect/Color copy).
  Read a call to 0x437e90 by where its result goes: into +0xe0/+0xd0 of a Dialog it is an Insets, into
  a Graphics colour it is a Color.
- Rect: `mX, mY, mWidth, mHeight`; `Intersection` 0x4387a0 (shared with Graphics); Contains/Intersects are
  inlined. Widget::GetRect/GetInsetRect return a Rect through a hidden pointer.
- Point: `mX, mY`; only used by value (WriteCenteredLine shadow offset, CursorWidget::GetHotspot); no
  out-of-line code.
- Color: `0x4401b0` Color() = (0,0,0,255), `0x440210` Color(r,g,b), `0x437e90` Color(r,g,b,a),
  `0x4402e0` operator!= (graphics group).

### 14. KeyCodes

KeyCode = Win32 virtual-key code, the same values as 1.2 and the port's `KeyCodes.h` (TAB 9, RETURN 13,
SHIFT 16, CONTROL 17, ESCAPE 27, SPACE 32, PRIOR..DOWN 33..40, DELETE 46, F1 112...). The name table
`aKeyCodeArray` (0x52ff68, 87 entries of `{char name[12]; int code}`) is identical to 1.2, but
GetKeyCodeFromName / GetKeyNameFromCode are not linked (no reference to the table). WM tracks
`mKeyDown[0..254]`. Nothing to do.

## 4. The application class (SexyApp)

The merged SexyAppBase + SexyApp of 2004, the DDInterface/D3D pieces it touches, and its dialogs, registry,
properties, cursor, input, timing and loading thread. Symbols: `games/bookworm/docs/symbols/10_framework_app.jsonl`
(which also covers sections 6 and 7). Subsection numbers and "see N.M" references inside sections 4, 6 and
7 are local to each section.

### Class sizes (2004 build vs 1.2)

| Class | sizeof 2004 | 1.2 | Vtable slots 2004 (1.2) | Notes |
|---|---|---|---|---|
| SexyApp (= SexyAppBase + SexyApp) | **0x618** | 0x5e0 + 0x6d0 | 67 (96) | single base ButtonListener (6 slots); no DialogListener; BookWormApp data from 0x618 |
| DSoundManager | 0x14a0 | 0x30a0 | 14 (16) | no file-name table, no primary buffer |
| DSoundInstance | 0x38 | 0x38 | 12 (12) | identical code |
| MusicInterface | 4 | 4 | 15 (20) | no Unload*/PauseAll/ResumeAll/SetSongMaxVolume |
| BassMusicInterface | 0x14 | 0x14 | 15 (20) | BassMusicInfo 0x20 (1.2 0x28, no mVolumeCap) |
| FModMusicInterface | 0x20 | 0x20 | 17 (22) | dead code |
| BASS_INSTANCE / FMOD_INSTANCE | 0x64 / 0x80 | 0x68 / 0x80 | | BASS 2.0.0.20 shipped |
| MTRand | 0x9c4 | 0x9c4 | none | 1.2 seeding (69069) |
| Buffer | 0x20 | 0x20 | 1 | same |
| PropertiesParser / XMLParser | 0x2c / 0x74 | same | 1 / 1 | only ParsePropertiesBuffer linked |
| ResourceManager | 0xb4 | | 3 | created, never used |
| HTTPTransfer | 0xfc | 0xfc | 1 | used by the game's Hall of Fame |
| InternetManager / BetaSupport | 0x538 / 0x18c | | 1 / 1 | drop |
| DDInterface / D3DInterface / D3DTester | 0xcf0 / 0x4c / 0x84 | | 1 / 1 / 1 | port: SDL DDInterface |

### Top behaviour differences and what the port must do

1. **Slot numbers differ everywhere.** SexyApp has 67 slots with no Init/Shutdown/PreTerminate/InitProperties
   hooks; SoundManager 14, MusicInterface 15. Use the tables (A.1.1, B.1, B.3). Cheat sheet for the calls game
   code makes most: app +0xa4 `PlaySample(id)`, +0xdc `DoDialog`, +0xe0 `GetDialog`, +0xe4 `AddDialog`,
   +0xe8 `KillDialog`, +0x84 `Shutdown`, +0x68 `OpenURL`, +0xbc/+0xc0 `SetMusicVolume`/`SetSfxVolume`,
   +0x5c `Validate`; mSoundManager (app+0x4e0) +0x08 `LoadSound(id,name)`; mMusicInterface (app+0x33c) +0x04
   LoadMusic, +0x08 PlayMusic, +0x0c StopMusic, +0x10 Pause, +0x14 Resume, +0x1c FadeIn, +0x20 FadeOut,
   +0x2c IsPlaying, +0x30 SetVolume, +0x38 Update.
2. **Music.** (a) BookWormApp::UpdateFrames (0x4100f0) runs `mMusicInterface->Update()` twice per tick, so
   fades run at 2x the nominal speed: translate literally. (b) BASS 2.0 sets the MOD global volume to v (not
   1.2's 0.4 v): game-side `mMaxMusicVolume = 100` on the port's music interface, then calibrate loudness.
   (c) One MO3 loaded as five songs; PlayMusic/FadeIn offsets are MO3 **order numbers** (row 0).
3. **Random numbers.** MTRand::SRand(ulong) is SAF 1.2's (mt[i] = mt[i-1]*69069), the port's is 1.3's: the game's
   own generators (`new MTRand(Sexy::Rand())` for the Board, effect managers and a global) need a game-side
   1.2 seeding shim (like Big Money's SeedMTRand12) for any seed-based reproduction. CRT `rand()` is also used
   (seeded `srand(GetTickCount())` in Init): section 7.2.
4. **No default dialog, no DialogListener.** `NewDialog` returns NULL in the base (the game builds every
   dialog); dialog buttons report to the app's ButtonListener as ButtonPress/ButtonDepress(2000+dialogId) (yes)
   and (3000+dialogId) (no): same ids as the port's DialogButtonDepress mapping. KillDialog defers the delete.
5. **Command line.** The base HandleCmdLineParam pops up "Invalid Command Line Param" and exits for anything
   but the demo switches: the port's own test switches must not be routed through a translation of it.
6. **Online and registration code.** HTTPTransfer is used by the game's Hall of Fame (Get/Post/GetResultCode/
   GetContent): game-side stub that never connects. InternetManager (update check), registration (Validate,
   popcinfo.dat, TimesPlayed=100 trial rule), ads, BetaSupport, SEH reporting, demo record/playback,
   ResourceManager: drop. partner.xml (NoReg, DontUpdate, NoInternetScores = True) already selects these paths.
7. **Sound loading.** `cached_<name>.wav` (decoded copies, timestamp-checked) then .wav/.ogg/.au; a failed load
   makes BookWormApp::Init shut down silently; no device = success. Port: never write cache files; keep the
   game's check; make sure all 47 ids resolve.
8. **Cursors.** Six cursor ids (0 pointer, 1 hand, 2 dragging, 3 text, 4 circle-slash, 5 none); custom image
   cursors only when "CustomCursors" is on. Map 2004 id 5 to the port's CURSOR_NONE.
9. **Timing and focus.** 100 updates/s like the port; backlog clamp 1000 ms (port 200 ms), loading-screen draw
   throttle and a Windows idle "screen saver" freeze that the port should not copy; losing focus always mutes.
   Loading thread is a real thread in 2004 (progress bar animates); the port runs it synchronously.
10. **Folded getters in game code.** 0x468620 (`return this+8`) is `Image::GetHeight()` at every game call site;
    `get_field_04` is GetWidth; 0x496c00 calls are dead. Many STL helpers are shared (ICF) between framework and
    game types: section 7.2 gives the C++ they stand for.

**Bases / size.** `class SexyApp : public ButtonListener` only: one vtable (0x00506714, 67 slots) at +0;
there is **no DialogListener** base (1.2: `SexyAppBase : ButtonListener, DialogListener`, second vtable
at +4) and no separate SexyAppBase class (RTTI has a `Sexy::SexyAppBase` namespace only for a few
helpers Big Money's names were carried onto). **sizeof(SexyApp) = 0x618** (last member 0x610;
BookWormApp's own data starts at 0x618 with `Math` at 0x618, ctor 0x41a0f0). 1.2: SexyAppBase 0x5e0 +
SexyApp to 0x6d0. Port: `SexyApp : SexyAppBase` (framework/SexyApp.h, SexyAppBase.h), members by name,
no binary layout.

Globals: `gSexyAppBase` 0x56b8a8 (stored early in the ctor, as 1.2's SexyAppBase ctor) and `gSexyApp` 0x56ad30
(stored at its end); both = the app; game code uses 0x56b8a8 in TitleScreen/OptionsDialog;, `gHInstance` 0x56a918, `gVersionDLL` 0x56b8b4,
`gDDrawDLL` 0x56b8ac, `gDSoundDLL` 0x56b8b0, `gGetLastInputInfoFunc` 0x56b8b8, `gScreenSaverActive`
0x56b8bc (byte), `gIsFailing` 0x56b8bd (Redraw), `gFPSImage` 0x56b8c0, `gDemoTimeLeftImage` 0x56b8c8,
build-info pointer 0x530a30 -> "00000190Thu Apr 15 14:52:08 2004" (first 8 chars = mBuildNum 190,
rest = mBuildDate), product-id pointer 0x530a34 -> "47202849..." (signature check), hand/drag cursor
bitmaps 0x530a40/0x530ac0 and 0x530b40/0x530bc0, 0x56ada8 (byte, read-only here: input is ignored while
set; set by the SEH catcher, inferred).

#### 1.1 Vtable (0x00506714; BookWormApp 0x004fbda4 has 68 = these 67 + its own slot 67)

Reading: `(**(code **)(*app + 0xNN))` = slot NN/4. "1.2" = slot number in Big Money's SexyApp vtable
(FRAMEWORK_12.md). "Port" = framework/SexyAppBase.h / SexyApp.h. BW = BookWormApp override (address).
Stubs: 0x401570 = `ret 4` (empty, one arg), 0x4036d0 = `ret` (empty, no args), 0x47d7f0 = `xor al,al;
ret 4` (returns false), 0x47c9d0 = `xor eax,eax; ret 0x18` (returns NULL, 6 args).

| Slot (+off) | Address | Method (2004 signature) | 1.2 | Port | Notes |
|---|---|---|---|---|---|
| 0 (+00) | 0x401570 | `ButtonMouseEnter(int theId)` | 4 | ButtonMouseEnter | ButtonListener has 6 slots here (1.2: 7, no ButtonMouseMove); order as in section 3 |
| 1 (+04) | 0x401570 | `ButtonMouseLeave(int theId)` | 5 | ButtonMouseLeave | |
| 2 (+08) | 0x401560 | `ButtonPress(int theId, int theClickCount)` -> calls slot 3 | 0 | ButtonPress(id,cc) | verified (`ret 8`, calls +0xc) |
| 3 (+0c) | 0x401570 | `ButtonPress(int theId)` | 1 | ButtonPress(id) | BW 0x410b30 = `PlaySample(0)` (click sound) |
| 4 (+10) | 0x401570 | `ButtonDepress(int theId)` | 2 | ButtonDepress | BW 0x417080: all menu and dialog buttons; dialog YES = 2000+dialogId, NO = 3000+dialogId (Dialog 0x438720 forwards those ids to the app's slot 4, slot 3 for presses) |
| 5 (+14) | 0x401570 | `ButtonDownTick(int theId)` | 3 | ButtonDownTick | dialogs forward 2000/3000+id down-ticks here too (section 3) |
| 6 (+18) | 0x480700 | `void Process()` (no allowSleep, returns nothing) | 7 | Process(bool) | see 1.4 (timing) |
| 7 (+1c) | 0x47cce0 | `void UpdateFrames()` | 8 | UpdateFrames | `if(!mMinimized){++mUpdateCount; if(mWidgetManager->UpdateFrame()) ++mFPSDirtyCount;} mMusicInterface->Update(); mInternetManager->Update()` (0x479e10, ads). BW 0x4100f0 chains here |
| 8 (+20) | 0x47e220 | `void DoUpdateFrames()` | 9 | DoUpdateFrames | returns at once while gScreenSaverActive; first update after the loading thread ends: thread priority normal, mLoaded=true, mYieldMainThread=false, LoadingThreadCompleted(); then UpdateFrames() |
| — | | DoUpdateFramesF(float) | 10 | DoUpdateFramesF | **absent**: no UpdateF in 2004 |
| 9 (+24) | 0x488640 | `void MakeWindow()` | 11 | MakeWindow | creates "MainWindow" (windowed: style 0x82ca0000 at 64,64 sized to mWidth x mHeight client; fullscreen: WS_POPUP, topmost), DDInterface on first call (+ registry "Is3D"), DDInterface::Init, colour-depth/3D fallbacks, `SetTimer(mHWnd,100,mFrameTime)` |
| 10 (+28) | 0x47d710 | `void EnforceCursor()` | 12 | EnforceCursor | see 1.5 |
| 11 (+2c) | 0x47f340 | `void ReInitImages()` | 13 | ReInitImages | calls MemoryImage slot 18 (+0x48, frees the cached native/RLE alpha data) on every image of mMemoryImageSet. Name inferred from its call sites (after DD re-init in Redraw/SwitchScreenMode) |
| — | | DeleteNativeImageData() | 14 | DeleteNativeImageData | absent |
| 12 (+30) | 0x47f3b0 | `void DeleteExtraImageData()` | 15 | DeleteExtraImageData | under mDDInterface->mCritSect (+0x14), image slot 15 DeleteExtraBuffers |
| 13 (+34) | 0x4036d0 | `void LoadingThreadCompleted()` | 16 | LoadingThreadCompleted | empty; BW 0x4169d0 |
| 14 (+38) | 0x4905c0 | scalar deleting `~SexyApp()` | 17 | ~SexyApp | body 0x48f000; BW 0x41a510 (body 0x4187b0) |
| 15 (+3c) | 0x47d800 | `MusicInterface* CreateMusicInterface(HWND theHWnd)` | 18 | CreateMusicInterface() | mNoSoundNeeded -> plain MusicInterface (4 bytes); mWantFMod -> FModMusicInterface (0x20); else `new BassMusicInterface(mInvisHWnd)` (0x14). Init passes mInvisHWnd. Not overridden by BW (so Bass) |
| 16 (+40) | 0x48aaf0 | `void Init()` | 45 | Init | see 1.3. BW 0x414a10 calls it first |
| — | | InitHook / ShutdownHook / PreTerminate | 19-21 | yes | **absent**: no hooks; game init goes in its Init override, exit work in its Shutdown override |
| 17 (+44) | 0x4036d0 | `void LoadingThreadProc()` | 22 | LoadingThreadProc | runs on a real thread (see 1.6); BW 0x41a8a0 loads everything |
| 18 (+48) | 0x483a10 | `void WriteToRegistry()` | 23 | WriteToRegistry | see 1.7. BW 0x412000 |
| 19 (+4c) | 0x484800 | `void ReadFromRegistry()` | 24 | ReadFromRegistry | see 1.7. BW 0x41a540 |
| 20 (+50) | 0x47c9d0 | `Dialog* NewDialog(int theDialogId, bool isModal, const std::string& theHeader, const std::string& theLines, const std::string& theFooter, int theButtonMode)` | 25 | NewDialog | **base returns NULL** (1.2 builds a Dialog). BW 0x410970 builds the BookWorm dialog |
| — | | BeginPopup / EndPopup / MsgBox / LogScreenSaverError | 27-29, 31 | yes | absent |
| 21 (+54) | 0x47e7d0 | `void Popup(const std::string& theString)` | 30 | Popup | `if(!mShutdown) MessageBox(mHWnd, s, "FATAL ERROR", MB_ICONHAND)` (literal title, no property) |
| 22 (+58) | 0x4892d0 | `void SafeDeleteWidget(Widget* theWidget)` | 32 | SafeDeleteWidget | push_back on mSafeDeleteList (a plain `list<Widget*>`, no update-depth) |
| 23 (+5c) | 0x4830b0 | `bool Validate(const std::string& theUserName, const std::string& theRegCode)` | non-virtual in 1.2 SexyApp | SexyApp::Validate (non-virtual) | BigInt registration check. Called by ReadFromRegistry and by BW 0x417080 (RegisterDialog OK). Drop: treat as registered |
| 24 (+60) | 0x47ca30 | `void URLOpenFailed(const std::string& theURL)` | 33 | URLOpenFailed | mIsOpeningURL=false. BW 0x414fa0 |
| 25 (+64) | 0x47ca40 | `void URLOpenSucceeded(const std::string& theURL)` | 34 | URLOpenSucceeded | mIsOpeningURL=false; if mShutdownOnURLOpen {mSkipAd=true; Shutdown()}. BW 0x410b10 |
| 26 (+68) | 0x483410 | `bool OpenURL(const std::string& theURL, bool shutdownOnOpen)` | 35 | OpenURL | ignores a repeat of the URL being opened; sets mIsOpeningURL, mShutdownOnURLOpen, mOpeningURL, mOpeningURLTime; ShellExecute("open"); on failure URLOpenFailed. BW 0x415100 |
| 27 (+6c) | 0x48a330 | `bool OpenHTMLTemplate(const std::string& theTemplateFile, const DefinesMap& theDefines)` | 94 | SexyApp (stub) | writes temp\tpl%04d.html; drop |
| 28 (+70) | 0x48eb20 | `bool OpenRegisterPage()` | 92 | SexyApp (stub) | drop |
| 29 (+74) | 0x48df70 | `bool OpenRegisterPage(DefinesMap theDefines)` | 93 | SexyApp (stub) | register.tpl; drop |
| 30 (+78) | 0x4834c0 | `std::string GetProductVersion(const std::string& thePath)` | 36 | GetProductVersion | VerQueryValue "\StringFileInfo\040904E4 (or 040904B0)\ProductVersion" of the exe |
| 31 (+7c) | 0x47cbc0 | `void SEHOccured()` | 37 | SEHOccured | SetMusicVolume(0); hide window; mSEHOccured=true; EnforceCursor() |
| 32 (+80) | 0x485430 | `std::string GetGameSEHInfo()` | 38 | GetGameSEHInfo | crash report text; BW 0x416090. Drop |
| — | | GetSEHWebParams | 39 | no | absent |
| 33 (+84) | 0x47cbf0 | `void Shutdown()` | 40 | Shutdown | see 1.3. BW 0x410ca0 (TimesPlayed bookkeeping) chains here |
| — | | DoParseCmdLine | 41 | DoParseCmdLine | absent (inlined in Init) |
| 34 (+88) | 0x487220 | `void ParseCmdLine(const std::string& theCmdLine)` | 42 | ParseCmdLine | splits on spaces outside quotes, "name=value"; skipped when 0x318 (external window) is set |
| 35 (+8c) | 0x48dab0 | `void HandleCmdLineParam(const std::string& theName, const std::string& theValue)` | 43 | HandleCmdLineParam | -play, -recnum=N, -playnum=N, -record, -demofile=F, -version (MessageBox, exit), -crash; **anything else: Popup("Invalid Command Line Param: "+name) and exit(0)** |
| 36 (+90) | 0x480950 | `void Start()` | 44 | Start | see 1.3 |
| 37 (+94) | 0x4036d0 | `void PreDisplayHook()` | 26 | PreDisplayHook | empty; called in Init before mWidgetManager->Resize (position = 1.2's PreDisplayHook call; name inferred) |
| 38 (+98) | 0x4036d0 | `void PreDDInterfaceInitHook()` | 46 | PreDDInterfaceInitHook | empty; called before every DDInterface::Init (MakeWindow, Set3DAccelerated, Redraw). Name inferred |
| 39 (+9c) | 0x47d7f0 | `bool ChangeDirHook(const char* theIntendedPath)` | 47 | ChangeDirHook | returns false -> Init chdir()s to the exe folder |
| 40 (+a0) | 0x47db80 | `void PlaySample(int theSoundNum, int thePan)` | 48 | PlaySample(id,pan) | `inst=mSoundManager->GetSoundInstance(id); inst->SetPan(pan); inst->Play(false,true)` |
| 41 (+a4) | 0x47db50 | `void PlaySample(int theSoundNum)` | 49 | PlaySample(id) | `GetSoundInstance(id)->Play(false,true)` (autorelease) |
| 42 (+a8) | 0x47dd50 | `double GetMasterVolume()` | 50 | GetMasterVolume | mSoundManager slot 9 |
| 43 (+ac) | 0x47dc70 | `double GetMusicVolume()` | 51 | GetMusicVolume | mMusicVolume |
| 44 (+b0) | 0x47dce0 | `double GetSfxVolume()` | 52 | GetSfxVolume | mSfxVolume |
| 45 (+b4) | 0x47dbc0 | `bool IsMuted()` | 53 | IsMuted | mMuteCount > 0 |
| 46 (+b8) | 0x47dd60 | `void SetMasterVolume(double)` | 54 | SetMasterVolume | **stores into mSfxVolume** (as 1.2), then mSoundManager->SetMasterVolume (slot 10) |
| 47 (+bc) | 0x47dc80 | `void SetMusicVolume(double)` | 55 | SetMusicVolume | mMusicVolume=v; mMusicInterface->SetVolume(muted ? 0 : v) (slot 12). BW 0x410110 |
| 48 (+c0) | 0x47dcf0 | `void SetSfxVolume(double)` | 56 | SetSfxVolume | mSfxVolume=v; mSoundManager->SetVolume(muted ? 0 : v) (slot 5) |
| 49 (+c4) | 0x47dbd0 | `void Mute(bool autoMute)` | 57 | Mute | ++mMuteCount (and ++mAutoMuteCount if autoMute); re-applies both volumes |
| 50 (+c8) | 0x47dc20 | `void Unmute(bool autoMute)` | 58 | Unmute | if mMuteCount>0: --mMuteCount (and --mAutoMuteCount if autoMute); re-applies volumes |
| 51 (+cc) | 0x47cb60 | `double GetLoadingThreadProgress()` | 59 | GetLoadingThreadProgress | 1.0 if mLoaded; 0 if not started or mNumLoadingThreadTasks==0; else min(completed/num, 1.0) |
| — | | GetImage / GetSharedImage (virtual) | 60-61 | yes | GetImage is non-virtual here (0x47d930), no shared images |
| 52 (+d0) | 0x488a80 | `void SwitchScreenMode(bool wantWindowed, bool is3d)` | 62 (3 args) | SwitchScreenMode(w,3d,force) | mForceFullscreen -> windowed=false; if changed: Set3DAccelerated(is3d,false), (demo -> windowed), MakeWindow, show, mSoundManager->SetCooperativeWindow, ReInitImages, mLastTime=now; else Set3DAccelerated(is3d,true) |
| 53 (+d4) | 0x47e310 | `void SwitchScreenMode(bool wantWindowed)` | 63 | SwitchScreenMode(w) | slot 52 with Is3DAccelerated() |
| — | | SwitchScreenMode() | 64 | yes | absent |
| 54 (+d8) | 0x47d6c0 | `void SetAlphaDisabled(bool)` | 65 | SetAlphaDisabled | |
| 55 (+dc) | 0x47c9e0 | `Dialog* DoDialog(int id, bool isModal, const std::string& header, const std::string& lines, const std::string& footer, int buttonMode)` | 66 | DoDialog | `KillDialog(id); d=NewDialog(...); AddDialog(id,d); return d`. BW 0x4103e0 |
| 56 (+e0) | 0x47f890 | `Dialog* GetDialog(int theDialogId)` | 67 | GetDialog | mDialogMap lookup |
| — | | AddDialog(Dialog*) | 68 | yes | absent |
| 57 (+e4) | 0x489220 | `void AddDialog(int theDialogId, Dialog* theDialog)` | 69 | AddDialog(id,d) | KillDialog(id); map insert; list push_back; mWidgetManager->AddWidget(d); if d->IsModal() (Dialog slot 51): WidgetManager::AddBaseModal(d) (0x439200) + ModalOpen() |
| — | | KillDialog(Dialog*) | 70 | yes | absent |
| 58 (+e8) | 0x482f90 | `bool KillDialog(int theDialogId)` | 71 | KillDialog(id) | if modal ModalClose(); mWidgetManager->RemoveWidget(d); list remove; **SafeDeleteWidget(d)** (deleted later, not now); map erase; if no base-modal widget left (WM+0x4c==0) the most recently added remaining modal dialog becomes base modal. BW 0x4110b0 |
| 59 (+ec) | 0x47e210 | `int GetDialogCount()` | 72 | GetDialogCount | mDialogMap.size() |
| 60 (+f0) | 0x4036d0 | `void ModalOpen()` | 73 | ModalOpen | empty; BW 0x410ad0 |
| 61 (+f4) | 0x4036d0 | `void ModalClose()` | 74 | ModalClose | empty; BW 0x410af0 |
| 62 (+f8) | 0x4036d0 | `void GotFocus()` | 75 | GotFocus | empty; BW 0x419000 |
| 63 (+fc) | 0x4036d0 | `void LostFocus()` | 76 | LostFocus | empty; BW 0x419040 |
| — | | IsAltKeyUsed (virtual) | 77 | no | non-virtual 0x47cf40 here |
| 64 (+100) | 0x48d670 | `bool DebugKeyDown(int theKey)` | 78 | DebugKeyDown | only after Ctrl+Alt+D toggled mDebugKeysEnabled: F2 perf, F3 FPS, F4 FPS mode, F8 3D toggle, F9 memory, F11 dump, Alt+R (0x49a360). Drop |
| — | | DebugKeyDownAsync, CloseRequestAsync, Done3dTesting, NotifyCrashHook, CheckSignature (virtual) | 79-83 | yes | absent (CheckSignature non-virtual 0x486c90) |
| 65 (+104) | 0x486510 | `bool DrawDirtyStuff()` | 84 | DrawDirtyStuff | see 1.4 |
| 66 (+108) | 0x4884f0 | `void Redraw(Rect* theClipRect)` | 85 | Redraw | DDInterface::Redraw; on failure re-inits DirectDraw (lost surfaces) |
| — | | InitPropertiesHook, DoMainLoop, UpdateAppStep, UpdateApp, ShouldCheckForUpdate, UpdateCheckQueried, OpenUpdateURL | 86-91, 95 | yes | absent (ShouldCheckForUpdate 0x47cad0 / UpdateCheckQueried 0x47cb30 are non-virtual) |
| BW 67 (+10c) | 0x414ef0 | BookWormApp's own new virtual | 96 (WinBMApp's) | game | game code |

#### 1.2 Fields (absolute offsets; V = verified from a ctor store or an accessor body, I = inferred)

| Off | Type | Name (1.2 name where the same) | Meaning / ctor value | Port field | |
|---|---|---|---|---|---|
| 0x000 | vfptr | | 0x506714 (BW 0x4fbda4) | | V |
| 0x004 | ulong | mRandSeed | GetTickCount() in Init, SRand(seed) | mRandSeed | V |
| 0x008 | string | mProdName | "Product"; BW ctor "BookWorm"; partner.xml "ProdName" | mProdName | V |
| 0x024 | string | mTitle | "SexyApp"; BW "BookWorm Deluxe " + mProductVersion; partner "Title" | mTitle | V |
| 0x040 | string | mRegKey | BW "PopCap\\BookWorm"; property "RegistryKey" overrides (ReadFromRegistry) | mRegKey | V |
| — | | (mCompanyName) | **absent** in 2004 | mCompanyName | |
| 0x05c | int | mRelaxUpdateBacklogCount | 0 | same | V |
| 0x060 | int | mWidth | 640 | mWidth | V |
| 0x064 | int | mHeight | 480 | mHeight | V |
| 0x068 | int | mFullscreenBits | 16 | same | V |
| 0x070 | double | mMusicVolume | 0.85; BW ctor 0.5 | same | V |
| 0x078 | double | mSfxVolume | 0.85 | same | V |
| 0x080 | double | mDemoMusicVolume | 0 | same | V |
| 0x088 | double | mDemoSfxVolume | 0 | same | V |
| 0x090 | bool | mNoSoundNeeded | false (true: no DirectSound window, dummy music) | same | V |
| 0x091 | bool | mWantFMod | false | same | V |
| 0x094 | int | mBuildNum | atol of the first 8 chars of the build string (190) | SexyApp::mBuildNum | V |
| 0x098 | string | mBuildDate | "Thu Apr 15 14:52:08 2004" | SexyApp::mBuildDate | V |
| 0x0b4 | HANDLE | mMutex | single-instance mutex (mProdName+"Mutex") | — | V |
| 0x0b8 | bool | mOnlyAllowOneCopyToRun | true | same | V |
| 0x0bc | UINT | mNotifyGameMessage | RegisterWindowMessage("Notify"+mProdName) | — | V |
| 0x0c0 | CritSect | mCritSect | 24 bytes, InitializeCriticalSection (0x466880) | — | V |
| 0x0d8 | InternetManager* | mInternetManager | new InternetManager (0x538 bytes) in the ctor | SexyApp::mInternetManager (void*) | V |
| 0x0dc | BetaSupport* | mBetaSupport | new BetaSupport(this) (0x18c) in Init | mBetaSupport (void*) | V |
| 0x0e0 | bool | mBetaValidate | if set, Init exits unless BetaSupport validates (0x477930) | — | V |
| 0x0e1 | uchar[512] | mAdd8BitMaxTable | 0..255 then 255 | same | V |
| 0x2e1 | bool | mSkipAd | set by URLOpenSucceeded before Shutdown; skips the exit ad in Start | SexyApp::mSkipAd | V |
| 0x2e2 | bool | mDontUpdate | registry/partner "DontUpdate" | SexyApp::mDontUpdate | V |
| 0x2e4 | WidgetManager* | mWidgetManager | **created in Init** (`new WidgetManager(this)`, 0x174 bytes), 0 before | mWidgetManager | V |
| 0x2e8 | map<int,Dialog*> | mDialogMap | size at 0x2f0 | same | V |
| 0x2f4 | list<Dialog*> | mDialogList | | same | V |
| 0x300 | DWORD | mPrimaryThreadId | GetCurrentThreadId() **in the ctor** | — | V |
| 0x304 | bool | mSEHOccured | | same | V |
| 0x305 | bool | mShutdown | | same | V |
| 0x306 | bool | mIsWindowed | true; registry ScreenMode = !mIsWindowed; partner "DefaultWindowed" | mIsWindowed | V |
| 0x307 | bool | mForceFullscreen | | same | V |
| 0x308 | bool | mForceWindowed | | same | V |
| 0x309 | bool | mInitialized | end of Init | same | V |
| 0x30c | DWORD | mTimeLoaded | GetTickCount() | same | V |
| 0x310 | HWND | mHWnd | main window | mHWnd (SDL_Window*) | V |
| 0x314 | HWND | mInvisHWnd | "InvisWindow" (music/sound owner) | — | V |
| 0x318 | HWND | (external window, no 1.2 name) | 0; when set MakeWindow uses it instead of creating one, ParseCmdLine and the idle check are skipped (host/embedding window, inferred) | — | V/I |
| 0x31c | list<MSG> | mDeferredMessages | node = hwnd,msg,wParam,lParam,time,pt (0x24 bytes) | — | V |
| 0x328 | bool | mNoDefer | false: input messages are queued and run from Start's loop | same | V |
| 0x32c | HTTPTransfer* | (no 1.2 name) | `new HTTPTransfer` (0xfc) in the ctor; only Abort()ed and polled in Shutdown | — | V |
| 0x330 | bool | (ITSuccess) | registry "ITSuccess": that transfer finished (result 0) | — | V |
| 0x334 | DDInterface* | mDDInterface | created by MakeWindow (0xcf0 bytes) | mDDInterface | V |
| 0x338 | bool | mAlphaDisabled | | same | V |
| 0x33c | MusicInterface* | mMusicInterface | CreateMusicInterface in Init | same | V |
| 0x340 | string | mReferId | registry/property "ReferId" | SexyApp::mReferId | V |
| 0x35c | string | mVariation | registry "Variation" | SexyApp::mVariation | V |
| 0x378 | ulong | mDownloadId | registry "DownloadId" | SexyApp::mDownloadId | V |
| 0x37c | string | mRegSource | "ingame" | SexyApp::mRegSource | V |
| 0x398 | bool | mReadFromRegistry | set by ReadFromRegistry; Shutdown writes the registry if set | same | V |
| 0x39c | string | mRegisterLink | "http://www.popcap.com/register.php?theGame="+mProdName+"&referid="+mReferId; registry/property "RegisterLink" | same | V |
| 0x3b8 | string | mProductVersion | GetProductVersion(exe) in the ctor | same | V |
| 0x3d4 | Image*[6] | mCursorImages | **6 cursors** (1.2: 13) | mCursorImages[NUM_CURSORS] | V |
| 0x3ec | bool | mIsOpeningURL | | same | V |
| 0x3ed | bool | mShutdownOnURLOpen | | same | V |
| 0x3f0 | string | mOpeningURL | | same | V |
| 0x40c | DWORD | mOpeningURLTime | | same | V |
| 0x410 | DWORD | mLastTimerTime | WM_TIMER tick | same | V |
| 0x414 | DWORD | mLastBigDelayTime | | same | V |
| 0x418 | ulong | mLastVerCheckQueryTime | registry; time(NULL) if absent | SexyApp:: same | V |
| 0x41c | string | mUserName | = mRegUserName when read; BW writes it (0x4173d2) | SexyApp::mUserName | V |
| 0x438 | string | mRegUserName | registry "RegName" | same | V |
| 0x454 | string | mRegCode | registry "RegCode" | same | V |
| 0x470 | bool | mIsRegistered | partner NoReg -> true; `|= Validate(name, code)` | same | V |
| 0x471 | bool | mBuildUnlocked | partner NoReg -> true | same | V |
| 0x472-0x487 | | (unused) | never accessed (room of 1.2's mUnmuted*Volume doubles, I) | | I |
| 0x488 | int | mMuteCount | registry "Muted" | same | V |
| 0x48c | int | mAutoMuteCount | | same | V |
| 0x490 | bool | mDemoMute | | same | V |
| 0x494 | set<MemoryImage*> | mMemoryImageSet | AddMemoryImage 0x488b50 / RemoveMemoryImage 0x4874a0 (under the DD critsect) | same | V |
| 0x4a0 | int | mNonDrawCount | | same | V |
| 0x4a4 | int | mFrameTime | 10 (ms) | same | V |
| 0x4a8 | DWORD | mLastTime | | same | V |
| 0x4ac | DWORD | mLastUserInputTick | | same | V |
| 0x4b0 | int | mSleepCount | | same | V |
| 0x4b4 | int | mDrawCount | | same | V |
| 0x4b8 | int | mUpdateCount | | same | V |
| 0x4c0 | double | mUpdateMultiplier | 1.0 | same | V |
| 0x4c8 | bool | mPaused | demo 'P' toggles | same | V |
| 0x4cc | int | mFastForwardToUpdateNum | | same | V |
| 0x4d0 | bool | mFastForwardToMarker | | same | V |
| 0x4d4 | DWORD | mLastDrawTick | | same | V |
| 0x4d8 | DWORD | mNextDrawTick | | same | V |
| 0x4dc | int | mCursorNum | | same | V |
| 0x4e0 | SoundManager* | mSoundManager | new DSoundManager(mNoSoundNeeded ? NULL : mHWnd, mWantFMod) (0x14a0) in Init | same | V |
| 0x4e4 | HCURSOR | mHandCursor | CreateCursor(11,4) | — | V |
| 0x4e8 | HCURSOR | mDraggingCursor | CreateCursor(15,10) | — | V |
| 0x4ec | list<Widget*> | mSafeDeleteList | ProcessSafeDeleteList 0x480660 deletes all (Rand disallowed meanwhile) | same (with depth) | V |
| 0x4f8 | bool | mMouseIn | | same | V |
| 0x4f9 | bool | (left button down) | set/cleared by the mouse dispatcher; picks MouseDrag vs MouseMove | — | V |
| 0x4fa | bool | (right button down) | | — | V |
| 0x4fb | bool | (middle button down) | | — | V |
| 0x4fc | bool | mRunning | Start | same | V |
| 0x4fd | bool | mActive | 1; WM_ACTIVATEAPP | same | V |
| 0x4fe | bool | mMinimized | WM_SIZE | same | V |
| 0x4ff | bool | mHasFocus | 1 | same | V |
| 0x500 | int | mDrawTime | | same | V |
| 0x504 | ulong | mFPSStartTick | | same | V |
| 0x508 | int | mFPSFlipCount | | same | V |
| 0x50c | int | mFPSDirtyCount | | same | V |
| 0x510 | int | mFPSTime | | same | V |
| 0x514 | int | mFPSCount | | same | V |
| 0x518 | bool | mShowFPS | | same | V |
| 0x51c | int | mShowFPSMode | | same | V |
| 0x520 | int | mScreenBltTime | | same | V |
| 0x524 | bool | mLoadingThreadStarted | | same | V |
| 0x525 | bool | mLoadingThreadCompleted | set by the thread stub | same | V |
| 0x526 | bool | mLoaded | | same | V |
| 0x527 | bool | mYieldMainThread | true | same | V |
| 0x528 | bool | mLoadingFailed | Process calls Shutdown | same | V |
| 0x529 | bool | mCursorThreadRunning | | — | V |
| 0x52a | bool | mSysCursor | true | same | V |
| 0x52b | bool | mCustomCursorsEnabled | registry "CustomCursors" | same | V |
| 0x52c | int | mTimesPlayed | registry + %WINDIR%\popcinfo.dat cross-check; BW increments | SexyApp::mTimesPlayed | V |
| 0x530 | int | mTimesExecuted | ++ in Init | SexyApp::mTimesExecuted | V |
| 0x534 | bool | mTimedOut | | SexyApp::mTimedOut | V |
| 0x535 | bool | mLastShutdownWasGraceful | registry "InProgress"==0 | same | V |
| 0x538 | int | mNumLoadingThreadTasks | | same | V |
| 0x53c | int | mCompletedLoadingThreadTasks | BW LoadingThreadProc increments | same | V |
| 0x540 | bool | mRecordingDemoBuffer | -record | same | V |
| 0x541 | bool | mPlayingDemoBuffer | -play | same | V |
| 0x542 | bool | mManualShutdown | WM_CLOSE | same | V |
| 0x544 | string | mDemoFileName | "popcap.dmo" (no mDemoPrefix) | — | V |
| 0x560 | Buffer | mDemoBuffer | 0x20 bytes | — | V |
| 0x580 | int | mDemoLength | | — | V |
| 0x584/0x588 | int | mLastDemoMouseX/Y | | — | V |
| 0x58c | int | mLastDemoUpdateCnt | | — | V |
| 0x590 | bool | mDemoNeedsCommand | true | — | V |
| 0x591 | bool | mDemoIsShortCmd | | — | V |
| 0x594 | int | mDemoCmdNum | | — | V |
| 0x598 | int | mDemoCmdOrder | -1 | — | V |
| 0x59c | int | mDemoCmdBitPos | | — | V |
| 0x5a0 | bool | mDemoLoadingComplete | | — | V |
| 0x5a4 | map<HANDLE,int> | mHandleToIntMap | | — | V |
| 0x5b0 | int | mCurHandleNum | | — | V |
| 0x5b4 | list | mDemoMarkerList | | — | V |
| 0x5c0 | bool | mDebugKeysEnabled | Ctrl+Alt+D toggles | same | V |
| 0x5c3 | bool | mWaitForVSync | registry "WaitForVSync" | same | V |
| 0x5c4 | bool | mUserChanged3DSetting | | same | V |
| 0x5c5 | bool | mAutoEnable3D | false; **BookWormApp ctor sets true** (0x41a4f1): MakeWindow/Do3DTest (0x458bf0) may switch 3D on when the D3D test recommends it | same | V |
| 0x5c6 | bool | mTest3D | | same | V |
| 0x5c8 | DWORD | mMinVidMemory3D | 6 | — | V |
| 0x5cc | DWORD | mRecommendedVidMemory3D | 14 | — | V |
| 0x5d0 | map<string,string> | mStringProperties | (node isnil +0x45) | mStringProperties (wstring in port) | V |
| 0x5dc | map<string,bool> | mBoolProperties | | same | V |
| 0x5e8 | map<string,int> | mIntProperties | | same | V |
| 0x5f4 | map<string,double> | mDoubleProperties | | same | V |
| 0x600 | map<string,vector<string>> | mStringVectorProperties | | same | V |
| 0x60c | ResourceManager* | mResourceManager | `new ResourceManager(this)` (0xb4) in the ctor | same | V |
| 0x610 | int | (mOldWndProc?) | 0, never read | — | I |

Not present in 2004 (do not look for them): mCompanyName, mFullCompanyName, mChangeDirTo, mUpdateAppState,
mUpdateAppDepth, mPendingUpdatesAcc, mUpdateFTimeAcc, mLastTimeCheck, mHasPendingDraw, mIsDrawing,
mLastDrawWasEmpty, mSharedImageMap, mCleanupSharedImages, mOverrideCursor, mCustomCursorDirty,
mMuteOnLostFocus (always on), mAutoStartLoadingThread (always on), mVSyncUpdates and the vsync-broken
test, mSyncRefreshRate, mIsPhysWindowed, mFullScreenWindow, mIsScreenSaver, mAllowMonitorPowersave,
mDemoPrefix, mStepMode, mCtrlDown/mAltDown (not initialised, never read), mMouseSensitivity.

#### 1.3 Construction, Init, Start, Shutdown (what a port's game Init/Start must reproduce)

- **ctor 0x48f810** (called first by BookWormApp's ctor 0x41a0f0): the field defaults above; LoadLibrary
  of version/ddraw/dsound; product version; default strings via SetString (0x48a810): DIALOG_BUTTON_OK
  "OK", DIALOG_BUTTON_CANCEL "CANCEL", UPDATE_CHECK_TITLE/BODY, UP_TO_DATE_TITLE/BODY,
  NEW_VERSION_TITLE/BODY; creates InternetManager, HTTPTransfer (0x32c) and ResourceManager. **No
  WidgetManager, no MusicInterface/SoundManager/DDInterface yet.**
- **Init 0x48aaf0** order: return if mShutdown; DirectX DLL check; single-instance mutex (+ broadcast
  mNotifyGameMessage to the running copy, exit); chdir to the exe folder unless ChangeDirHook; parse the
  command line (ParseCmdLine, slot 34); `mRandSeed = GetTickCount(); SRand(mRandSeed)`; demo file read if
  -play; `LoadProperties("properties\\partner.xml", required=false, checkSig=true)` (0x486fe0; signature
  partner.xml.sig, BigInt check 0x486c90); NoReg -> mIsRegistered=mBuildUnlocked=true; ProdName;
  DefaultWindowed -> mIsWindowed; Title -> mTitle = Title+" "+mProductVersion; InternetManager init
  (adlist.txt); BetaSupport; `srand(GetTickCount())` (CRT rand used by game code); **ReadFromRegistry()**;
  ++mTimesExecuted; RegisterLink, DontUpdate properties; window classes; InvisWindow; **WidgetManager
  created here**; cursors; beta check; PreDisplayHook (slot 37); `mWidgetManager->Resize(0,0,mWidth,
  mHeight)` (0x4375b0); windowed only if the screen is larger than mWidth x mHeight (SM_CXFULLSCREEN);
  MakeWindow; DSoundManager; SetSfxVolume(mSfxVolume); CreateMusicInterface(mInvisHWnd);
  SetMusicVolume(mMusicVolume); mInitialized=true. BookWormApp::Init (0x414a10) calls this first and then
  does its own setup (cursor images via SetCursorImage, properties via GetInteger/GetString, ...).
  Port: SexyAppBase::Init + SexyApp::InitPropertiesHook do the same in a different order (properties
  first, then registry); BookWorm's partner.xml keys are covered by the port except RegisterLink (unused
  by the port) and DontUpdate (port forces true). Nothing to change, but the game's Init override must
  not rely on mWidgetManager in its ctor.
- **Start 0x480950** (slot 36): StartCursorThread (0x47d680), StartLoadingThread (0x47d570, always,
  raises the main thread to ABOVE_NORMAL), show + focus window, timeBeginPeriod(1), mRunning, mLastTime =
  mLastUserInputTick = mLastTimerTime = now; loop `while(!mShutdown){ PeekMessage/Dispatch all;
  ProcessDemo(); ProcessDeferredMessages(); if(mShutdown) break; Process(); }`; ProcessSafeDeleteList;
  WaitForLoadingThread; timing printout; the exit ad (InternetManager, unless mSkipAd: unregistered 1 in
  2, registered 1 in 10); **WriteToRegistry()**. There is no DoMainLoop/UpdateApp and no PreTerminate.
  Port: the port's Start/MainLoopFinished; ads are not ported.
- **Shutdown 0x47cbf0** (slot 33): once: mShutdown=true; demo volumes restored; wait for the cursor
  thread; mMusicInterface->StopAllMusic(); HTTPTransfer abort (result -> ITSuccess); restore display mode
  when fullscreen; hide window; WriteToRegistry() if mReadFromRegistry. No ShutdownHook, no
  cross-thread check (1.2 sets mLoadingFailed when called from the loading thread). BookWormApp::
  Shutdown (0x410ca0) adjusts mTimesPlayed first and chains here.
- **~SexyApp 0x48f000**: Shutdown(); if mUserChanged3DSetting, the "Hardware Acceleration was switched on
  ..." MessageBox and registry "Is3D"; deletes dialogs (RemoveWidget + delete), windows, WidgetManager,
  DDInterface, MusicInterface, SoundManager, etc. Port: nothing (no prompts).

#### 1.4 Update/draw timing (Process 0x480700, DrawDirtyStuff 0x486510, Redraw 0x4884f0)

`Process()`: if mLoadingFailed Shutdown(). Demo fast-forward (mute, run DoUpdateFrames back to back,
draw once a second). Then, unless mPaused or mUpdateMultiplier <= 0:
`aFrameTime = (int)(mFrameTime / mUpdateMultiplier)` (x87, _ftol truncation; 10 ms). `behind = now -
mLastTime` (measured before a pending ClearUpdateBacklog relax resets mLastTime). If `behind >=
aFrameTime` and (`++mNonDrawCount < 10` or `!mLoaded`): **one** DoUpdateFrames(), ProcessSafeDeleteList(),
`mLastTime += aFrameTime`, and the backlog is clamped to **1000 ms** (`if now-mLastTime > 1000: mLastTime
= now-1000`). Otherwise: mNonDrawCount=0, DrawDirtyStuff(), and if still ahead `Sleep(aFrameTime -
elapsed)` (++mSleepCount). While mYieldMainThread (loading thread running): extra `Sleep(min(250,
2*busy - slept))`. ProcessSafeDeleteList at the end.

`DrawDirtyStuff()` (Rand disallowed for its duration, MTRand::SetRandAllowed 0x4665c0): FPS image if
mShowFPS; once a second the idle check 0x47cd40 (below); returns false while gScreenSaverActive; draws
when `mWidgetManager->DrawScreen()` (0x4394a0) drew something or 1 s passed since the last present, and
`now >= mNextDrawTick`; counters; VSync sleep only if mWaitForVSync && mIsWindowed (mDDInterface+0x50 ms
per frame); `Redraw(NULL)`; while the loading thread runs, `mNextDrawTick += 35 + max(drawTime, 15)`
(throttles the loading screen to <= ~20 fps).

Differences vs port (SexyAppBase::Process, 1.3 state machine): same 10 ms tick and the same "at most 10
updates, then a forced draw once loaded" rule; the port clamps the backlog at 200 ms (2004: 1000 ms),
calls DoUpdateFramesF/UpdateF after each update (no 2004 equivalent; BookWorm has no UpdateF overrides,
so harmless), and has no loading-screen draw throttle. **Port: nothing required.** The idle check
(gScreenSaverActive: when the user has been idle longer than the Windows screen-saver timeout minus 2 s,
DoUpdateFrames and DrawDirtyStuff stop until input) is a Windows-only freeze: **do not port**.

#### 1.5 Input dispatch (WindowProc 0x489310 -> mDeferredMessages -> ProcessDeferredMessages 0x47fa40)

WindowProc queues key, char, mouse, timer, size, close and activate messages (unless mNoDefer) and handles
WM_PAINT (Redraw if mRunning), WM_SETCURSOR (EnforceCursor), WM_ENDSESSION (Shutdown), Alt+Enter
(SwitchScreenMode(!mIsWindowed) only if mLoaded and not forced), demo playback keys. Start's loop then
runs ProcessDeferredMessages before every Process():
- **Keys**: WM_KEYDOWN/WM_SYSKEYDOWN -> mLastUserInputTick; Ctrl+Alt+D toggles mDebugKeysEnabled; `if
  (!mDebugKeysEnabled || !DebugKeyDown(key)) WidgetManager::KeyDown(key)` (0x437790); KEYUP ->
  WidgetManager::KeyUp (0x4377c0); WM_CHAR -> WidgetManager::KeyChar (0x437750), one byte. Same as port.
- **Mouse**: every mouse message first sends `MouseDrag(x,y)` (0x439d90) if any of the app's button
  flags 0x4f9/0x4fa/0x4fb is set, else `MouseMove(x,y)` (0x439d70); then sets mMouseIn (+EnforceCursor)
  if needed; then buttons: L down 1, L dblclk 2, R down -1, R dblclk -2, M down 3 -> `MouseDown(x,y,code)`
  (0x43a850) + SetCapture; ups -> `MouseUp(x,y,code)` (0x439d00), ReleaseCapture when all are up. Wheel
  -> `MouseWheel(delta/120)` (0x437730). Mouse leave is detected on WM_TIMER by polling the cursor
  (WidgetManager::MouseExit 0x437710). Port: MouseMoveTo + MouseDown/MouseUp with the same click codes;
  the 2004 app decides drag-vs-move itself from its own flags (1.3's WidgetManager decides from
  mDownButtons): same result for the game. Nothing to do.
- **Focus (RehupFocus 0x47cf50)**: focus = mActive && !mMinimized; on gain `Unmute(true)`,
  WidgetManager::GotFocus (0x437650), GotFocus(); on loss `Mute(true)`, WidgetManager::LostFocus
  (0x438830), LostFocus(). Always muted while inactive (port: mMuteOnLostFocus, default true: same).
  BookWormApp's GotFocus/LostFocus (0x419000/0x419040) pause the game.
- WM_CLOSE on either window: mManualShutdown=true, Shutdown().
- WM_TIMER also drives the URL-open timeout: after 5 s of continuous timer ticks, if the browser
  stole focus URLOpenSucceeded(mOpeningURL), if 12 s pass while still active URLOpenFailed.

**EnforceCursor (slot 10)**: nothing if mWidgetManager is NULL. If !mSEHOccured && mMouseIn: a custom
image cursor `mCursorImages[mCursorNum]` is drawn by DDInterface (SetCursorImage 0x44f530, moved by the
cursor thread every 10 ms) **only if mCustomCursorsEnabled or a demo is playing** (1.2 also allows
CURSOR_CUSTOM); otherwise Windows cursors: 0 arrow, 1 mHandCursor, 2 mDraggingCursor, 3 IBEAM, 4 NO
(circle-slash), 5 none. **6 cursor ids** (port enum has 13: the first six match: POINTER, HAND,
DRAGGING, TEXT, CIRCLE_SLASH, then 1.3 SIZEALL=5; 2004 id 5 = "none" = port CURSOR_NONE (11)): the
port must map id 5 -> CURSOR_NONE (game-side constant). SetCursor 0x47d8f0, SetCursorImage 0x47caa0
(0..5), EnableCustomCursors 0x47d910 (game OptionsDialog 0x42ed00, registry CustomCursors).

#### 1.6 Loading thread

StartLoadingThread 0x47d570 always runs `LoadingThreadProc()` on a separate `_beginthread` thread
(stub 0x47d4f0 sets mLoadingThreadCompleted when it returns); the main thread keeps running Process
(title screen) and DoUpdateFrames calls LoadingThreadCompleted() on the main thread at the first update
after it finished. The game's LoadingThreadProc (0x41a8a0) bumps mCompletedLoadingThreadTasks (0x53c) for
the progress bar and checks mShutdown. Port: StartLoadingThread runs it synchronously; the title screen's
progress bar therefore jumps from 0 to full (port behaviour, accepted for Bejeweled) - nothing to do
unless an animated loading bar is wanted.

#### 1.7 Registry, properties, files

- **Registry** (HKLM\SOFTWARE\<mRegKey>[\sub], value = text after the last '\\'; port: emulated
  "SOFTWARE\<mRegKey>\..."): RegistryWrite 0x483650 (type, data, len), RegistryWriteString 0x483980,
  RegistryWriteInteger 0x4839c0 (game 0x414910), RegistryWriteBoolean 0x4839e0 (stored as DWORD 0/1),
  RegistryEraseValue 0x4841c0, RegistryRead 0x4843b0, RegistryReadString 0x4846f0 (**at most 255 bytes**),
  RegistryReadInteger 0x484780, RegistryReadBoolean 0x4847d0 (any nonzero DWORD = true). No
  RegistryReadData/WriteData/EraseKey/GetSubKeys in 2004. All return false when mRegKey is empty;
  recorded into / replayed from the demo buffer.
- **WriteToRegistry 0x483a10** values: MusicVolume and SfxVolume = (int)(volume*100), Muted =
  (mMuteCount - mAutoMuteCount > 0), ScreenMode = !mIsWindowed, CustomCursors, ITSuccess, TimesPlayed,
  TimesExecuted, LastVerCheckQueryTime, InProgress = 0, WaitForVSync, RegName (mRegUserName, filled from
  mUserName when a code is present), RegCode. Plus, unless playing a demo, **%WINDIR%\popcinfo.dat**:
  records {u16 len, mProdName, u16 TimesPlayed, u16 TimesExecuted} (anti-trial-reset store).
- **ReadFromRegistry 0x484800**: mReadFromRegistry=true; mRegKey = GetString("RegistryKey", mRegKey);
  nothing else if mRegKey is empty; popcinfo.dat -> TimesPlayed/TimesExecuted; ReferId (registry, then
  property), mRegisterLink built from it, RegisterLink, MusicVolume/SfxVolume (int * 0.01), Muted ->
  mMuteCount, ScreenMode, CustomCursors (EnableCustomCursors), DontUpdate, ITSuccess, DownloadId,
  Variation, WaitForVSync, TimesPlayed/TimesExecuted (if they differ from popcinfo.dat they become
  **100**, i.e. trial over), LastVerCheckQueryTime, InProgress (mLastShutdownWasGraceful = value==0, then
  InProgress=1 is written), RegName (-> mUserName too), RegCode, `mIsRegistered |= Validate(name,code)`.
  Port: SexyApp::Read/WriteFromRegistry keep the volumes/screen mode/cursors; **drop popcinfo.dat and the
  TimesPlayed=100 rule** (port runs registered).
- **Properties** (partner.xml in the install: ReferId, NoReg=True, DontUpdate=True, NoInternetScores,
  NumOptionsLinks, OptionsLinkText0/URL0 (a "more games" link: never click it in tests), ShowTrialStatus,
  LoadingBarY=330, OptionsDialogHeight=315, NoIHSUpsell). LoadProperties 0x486fe0(file, required,
  checkSig): missing file -> true unless required (then Popup "Unable to open properties file: ");
  checkSig -> CheckSignature 0x486c90 (file+".sig", RSA/BigInt) else Popup "Signature check failed on
  '...'"; parser = PropertiesParser (0x479c30). Accessors: GetBoolean(id, default) 0x480ef0, GetInteger(id,
  default) 0x480f30, GetString(id, default) 0x480f70 (returns std::string by hidden pointer), GetString(id)
  0x4871a0 (returns "" when missing; used by framework dialogs), SetString 0x48a810, SetBoolean
  0x48a8d0, SetInteger 0x48a970, SetDouble 0x48aa10 (called by the parser). No GetDouble /
  GetStringVector accessor in this build (the maps exist). Port: same names (port strings are
  std::string-typed SexyString; mStringProperties stores wstring internally, GetString converts) -
  nothing to do; load partner.xml without signature check (port already does).
- **Files**: ReadBufferFromFile 0x47e550 (whole file, "rb"), WriteBytesToFile 0x47edb0,
  WriteBufferToFile 0x47ef50, FileExists 0x47e700 (fopen "rb"; game 0x412190). Plain CRT paths relative to
  the exe folder (Init chdir). Port: same methods through the VFS (writes go to the user dir).

**3D and the FPU (inferred).** Because BookWormApp sets mAutoEnable3D, a 3D-capable machine may run with
DDInterface::mIs3D on. DDInterface::Init (0x4619f0) calls SetCooperativeLevel with DDSCL_NORMAL (8) windowed or
FULLSCREEN|EXCLUSIVE (0x11), never DDSCL_FPUPRESERVE, so once the Direct3D7 device exists the x87 control word
may be at 24-bit precision on the main thread; game double math before `_ftol` would then round like float.
Only matters for bit-exact reproduction of float-derived values; the port computes in full precision.

#### 1.8 Other direct (non-virtual) SexyApp functions

| Address | Name and signature | 1.2 / port | Game callers / notes |
|---|---|---|---|
| 0x48f810 | `SexyApp::SexyApp()` | SexyAppBase()+SexyApp() / port ctors | BW ctor 0x41a0f0 |
| 0x48f000 | `SexyApp::~SexyApp()` body | ~SexyAppBase+~SexyApp | BW dtor 0x4187b0 |
| 0x47c980 | `void ClearUpdateBacklog(bool relaxForASecond)` | same | mLastTime=now; relax -> mRelaxUpdateBacklogCount = 1000/mFrameTime |
| 0x47ca70 | `void WaitForLoadingThread()` | same | Sleep(20) loop |
| 0x47caa0 | `void SetCursorImage(int theCursorNum, Image* theImage)` | same | BW Init 0x414a10; ids 0..5 only |
| 0x47cad0 | `bool ShouldCheckForUpdate()` | SexyApp::ShouldCheckForUpdate (virtual in 1.2) | BW 0x4169d0; false if mDontUpdate or checked < 7 days ago after a clean exit |
| 0x47cb30 | `void UpdateCheckQueried()` | SexyApp (virtual in 1.2) | BW 0x415c40; mLastVerCheckQueryTime = time(NULL) |
| 0x47cd40 | idle / screen-saver check (no 1.2 name) | — | sets gScreenSaverActive; drop |
| 0x47ce70 | demo 'S' key: toggle demo mute | — | drop |
| 0x47cf40 | `bool IsAltKeyUsed(WPARAM)` | virtual in 1.2 | WM_SYSCHAR: true for 'r' |
| 0x47cf50 | `void RehupFocus()` | same | see 1.5 |
| 0x47cfc0 | `void WriteDemoTimingBlock()` | same | demo |
| 0x47d060 | `bool PrepareDemoCommand()` | same | demo |
| 0x47d100 | `void ProcessDemo()` | same | demo playback input injection |
| 0x47d4f0 | `static void LoadingThreadProcStub(void*)` | same | |
| 0x47d570 | `void StartLoadingThread()` | same | |
| 0x47d5b0 / 0x47d660 / 0x47d680 | `CursorThreadProc()` / `CursorThreadProcStub` (CoInitialize) / `StartCursorThread()` | same | software cursor position, 10 ms |
| 0x47d8f0 | `void SetCursor(int theCursorNum)` | same | WidgetManager 0x4378e0..0x437d60 (cursor follows the widget under the mouse) |
| 0x47d910 | `void EnableCustomCursors(bool)` | same | game 0x42ed00 |
| 0x47d930 | `DDImage* GetImage(const std::string& theFileName)` (always commits bits) | virtual GetImage in 1.2 | game 0x4144b0; ImageLib::GetImage 0x491750 + new DDImage + SetBits: see section 5 |
| 0x47d9f0 | `DDImage* CopyImage(Image*, const Rect&)` | same | game 0x41a8a0, 0x428520, 0x429d90 (section 5) |
| 0x47e330 | `DDImage* CopyImage(Image*)` | same | game 0x429d90 |
| 0x47dab0 | `void FlipImage(Image*)` (vertical flip, then BitsChanged) | same | game 0x429d90 |
| 0x47dd90 | `void Remove3DData(MemoryImage*)` | same | |
| 0x47ddb0 | `bool Is3DAccelerated()` (mDDInterface->mIs3D, +0x10) | same | game 0x411220, 0x414910, 0x42ed00, 0x42fe90 |
| 0x47e380 | `bool Is3DAccelerationSupported()` | same | game 0x42fa50 (Options) |
| 0x47e3a0 | `bool Is3DAccelerationRecommended()` | same | game 0x42fa50 |
| 0x487530 | `void Set3DAccelerated(bool is3D, bool reinit)` | Set3DAcclerated | Options 3D checkbox path |
| 0x47e550 | `bool ReadBufferFromFile(const std::string&, Buffer*, bool dontWriteToDemo)` | same | |
| 0x47e700 | `bool FileExists(const std::string&)` | same | game 0x412190 |
| 0x47e810 / 0x4873c0 | `CopyToClipboard(const std::string&)` / `std::string GetClipboard()` | same | EditWidget 0x43c3a0 |
| 0x47ebe0 | `void WriteDemoBuffer()` | same | |
| 0x47edb0 / 0x47ef50 | `WriteBytesToFile(file, data, len)` / `WriteBufferToFile(file, Buffer*)` | same | |
| 0x47e0c0 / 0x47e190 / 0x482ee0 | `DemoSyncBuffer(Buffer*)` / `DemoSyncBool(bool*)` / `DemoSyncString(std::string*)` | same | callers: BetaSupport, ImageFont 0x458530 |
| 0x480660 | `void ProcessSafeDeleteList()` | same | |
| 0x47fa40 | `bool ProcessDeferredMessages()` | same | see 1.5 |
| 0x489310 | `static LRESULT WindowProc(HWND, UINT, WPARAM, LPARAM)` | same | |
| 0x488b50 / 0x4874a0 | `AddMemoryImage(MemoryImage*)` / `RemoveMemoryImage(MemoryImage*)` | same | MemoryImage ctor/dtor |
| 0x486fe0 | `bool LoadProperties(const std::string&, bool required, bool checkSig)` | same | |
| 0x486c90 | `bool CheckSignature(const Buffer&, const std::string& theFileName)` | SexyApp::CheckSignature (virtual in 1.2) | BigInt |
| 0x480ef0 | `bool GetBoolean(const std::string& theId, bool theDefault)` | same | game 0x4152a0, 0x4169d0, 0x417080, 0x433900 |
| 0x480f30 | `int GetInteger(const std::string& theId, int theDefault)` | same | game 0x414a10, 0x42f450, 0x42fe90, 0x434380 |
| 0x480f70 | `std::string GetString(const std::string& theId, const std::string& theDefault)` | same | game 0x4144b0, 0x4146d0, 0x414a10, 0x41a8a0, 0x4241d0 |
| 0x4871a0 | `std::string GetString(const std::string& theId)` | same | 0x436110, UpdateCheckDialog 0x43b730/0x43f650 |
| 0x48a810 / 0x48a8d0 / 0x48a970 / 0x48aa10 | `SetString` / `SetBoolean` / `SetInteger` / `SetDouble` | same | PropertiesParser 0x478e90 |
| 0x483650 .. 0x4847d0 | registry family (1.7) | same | game 0x414910 (RegistryWriteInteger) |
| 0x489e90 | `bool ReadDemoBuffer(std::string& theError)` | same | |
| 0x48b370 / 0x48cfd0 | `DumpProgramInfo()` (F11, HTML) / `ShowMemoryUsage()` (F9) | same | debug |
| 0x485d20 / 0x485ff0 / 0x486230 | `CalculateFPS()` / `FPSDrawCoords()` / `CalculateDemoTimeLeft()` | same | debug |
| 0x48d910 | demo-file numbering helper (popcap*.dmo, used by -recnum/-playnum) | — | demo |
| 0x4867d0 / 0x486aa0 / 0x47ef80 / 0x47f210 | demo "Jump To Time" and "Marker List" dialogs | same | demo |
| 0x47c4d0 / 0x47c960 | unhandled-exception filter / its installer (SEHCatcher) | SEHCatcher | drop |
| 0x47a250 .. 0x47bdf0 | SEHCatcher crash-report windows ("SEHProgressWindow", report submit) | SEHCatcher | drop |
| 0x47c7a0 | `std::string InternetManager::GetUpdateURL()` (inferred) | — | game 0x417080 (dialog 8 YES -> OpenURL(url, true)); drop |
| 0x479df0 | `InternetManager::DoUpdateCheck(const std::string& theURL)` (Big Money 0x473120 exact) | — | game 0x415de0 ("win32updatecheck.php?prod=..&ver=..&referid=.."); drop |

STL instantiations living in this range that game code also calls (identical code folding): the
`std::map<int,T*>` red-black-tree helpers used by mDialogMap and by game maps:
0x47ddc0 `_Rrotate`, 0x47de20 `_Max`, 0x47de40 `_Min`, 0x47de60 `const_iterator::_Dec`,
0x47dec0 `iterator::_Inc`, 0x47e3d0 `_Lrotate`, 0x47e9f0 `_Buynode` (game 0x42aab0, 0x42ac70, 0x42b260);
`std::list<std::string>::erase(first, last)` 0x489140 and `~list` (`_Tidy`) 0x48b330 (game 0x419080).
Everything else in the range is listed in app_symbols.jsonl (map/set/list insert/erase/_Buynode/_Erase
for the property maps, dialog map, memory-image set, deferred-message list; iostream num_put pieces used
by DumpProgramInfo).

#### 1.9 What the port must do for SexyApp (summary)

1. **Slot map**: translate `app->vtbl[+off]` with the table in 1.1 (2004 slot != 1.2 slot != port).
2. **No hooks**: InitHook/ShutdownHook/PreTerminate/InitPropertiesHook do not exist; BookWorm's Init,
   Shutdown, LoadingThreadProc, NewDialog, ButtonDepress overrides carry that logic. Port: override the
   same port methods (Init, Shutdown, LoadingThreadProc, NewDialog, ButtonDepress/DialogButtonDepress).
3. **NewDialog**: the base returns NULL; the game's NewDialog builds every dialog, so the port's
   SexyAppBase::NewDialog default is never reached.
4. **HandleCmdLineParam**: do not chain BookWorm to a version that exits on unknown params; the port's
   test params (-seed, -script, ...) must keep working.
5. **Cursor ids**: 2004 has 6 cursors; id 5 = none -> port CURSOR_NONE.
6. **Registration / trial / ads / update check / HTTP / beta / SEH / demo**: drop (port is registered;
   mDontUpdate true; never open URLs in tests).
7. **Timing**: port's 1.3 loop is equivalent at 100 Hz; differences (1000 ms backlog, loading draw
   throttle, idle freeze) are not worth reproducing.

## 5. Graphics, images and fonts

Graphics, Color, Image, MemoryImage, DDImage, Font, ImageFont and its data classes, SysFont, the
descriptor parser, ImageLib and the alpha masks, the app's image helpers, and what BookWorm's ShadowFont
relies on. Symbols: `games/bookworm/docs/symbols/10_framework_graphics.jsonl`. Subsection numbers and
"section N" references inside this section are local to it.

### 0. Summary

| class | sizeof 2004 | 1.2 (Big Money) | port 1.3 | base classes 2004 |
|---|---|---|---|---|
| Graphics | 0x58 | 0x54 | larger (GraphicsState + state stack + scale) | none (no GraphicsState split visible; 1 virtual: dtor) |
| Color | 0x10 | 0x10 | 0x10 | none |
| Image | 0x18 | 0x18 (see note) | larger (mDrawn, mFilePath, mNumTotal) | none |
| MemoryImage | 0x48 | 0x4c (see note) | larger (+ HD fields) | Image |
| DDImage | 0xc4 | 0xd0 | different (no DirectDraw) | MemoryImage |
| Font | 0x14 | 0x14 | 0x14 | none |
| ImageFont | 0x48 | 0x50 | 0x50 + | Font |
| SysFont | 0x1c | 0x1c? | not in port | Font |
| FontData | 0x198 | DescParser + FontData | DescParser + FontData | **none** (no DescParser class in 2004) |
| FontLayer | 0x1207c | similar (SharedImageRef) | similar (SharedImageRef) | none |
| CharData | 0x120 | 0x120 | 0x120 | none |
| ActiveFontLayer | 0x1010 | 0x1010 | same | none |
| RenderCommand | 0x34 | 0x34 | 0x34 | none |
| DataElement / Single / List | 0x8 / 0x24 / 0x18 | same | same | |
| ImageLib::Image | 0x10 | 0x10 | replaced by platform/ImageLoader | none |
| ShadowFont (game) | 0x60 | n/a | game class | ImageFont, Font |

Note on 1.2: `FRAMEWORK_12.md` lists Image as 0x1c with `mNumTotal` (generated from the public header),
but Big Money's Image ctor 00432680 is byte-identical to BookWorm's 00446cf0 and stores only 5 fields,
and Big Money's MemoryImage(app) ctor stores mApp at +0x48. So Big Money's real Image is 0x18 and its
MemoryImage offsets are 4 lower than FRAMEWORK_12.md says. BookWorm's MemoryImage is a further 4 bytes
shorter (no mPurgeBits / mWantPal).

#### Folded getters: read these carefully in the game decompilation

MSVC merged identical tiny functions, and Ghidra named the merged copy after one of its users. In the
game code:

| address | Ghidra name | what the game call means |
|---|---|---|
| 00446d10 | `get_field_04` | `Image::GetWidth()` (on images), `Font::GetAscent()` (on fonts, slot 1), `Color::GetGreen()` |
| 00468620 | `Sexy::Font::vf02` | `Image::GetHeight()` (on images), `Font::GetAscentPadding()` (slot 2), `Color::GetBlue()` |
| 00496c00 | `get_field_0c` | `Font::GetHeight()` (slot 4), `Color::GetAlpha()`, `ImageLib::Image::GetBits()` |
| 0041d910 | `?` (game region) | `Font::GetLineSpacingOffset()` (slot 5) |
| 00440270 | `FUN_00440270` | `Color::GetRed()` |
| 0041e8c0 | `Sexy::Effect::vf01` | also `FontData::Ref()` (`++[this+8]`) |
| 00446d50 | `?` | empty `ret 0x18`: Image::Blt, Image::StretchBlt, Font::DrawString |
| 0041dfa0 / 00401570 | `?` / `FUN_00401570` | empty `ret 0xc` / `ret 4`: Image::FillRect / Image::ClearRect (also other empty virtuals) |
| 004dc641 | `?` | `_purecall` (Font::Duplicate, DataElement::Duplicate) |

`Sexy::Font::vf02(img)` with 68 callers is almost always **GetHeight of an image**.

### 1. Color (no vtable, 0x10)

Fields: +0 mRed, +4 mGreen, +8 mBlue, +0xc mAlpha (verified, same as 1.2 and port).

| address | function | port | notes |
|---|---|---|---|
| 004401b0 | `Color()` = (0,0,0,255) | same | |
| 004401d0 | `Color(int 0xAARRGGBB)`, alpha 0 becomes 255 | `Color(int)` same | |
| 00440210 | `Color(int r, int g, int b)` alpha 255 | same | 31 game calls |
| 00437e90 | `Color(int r, int g, int b, int a)` | same | 42 game calls |
| 00440240 | `Color(const int* rgb)` alpha 255 | same | |
| 00440270 | `GetRed()` | same | other getters are folded (section 0) |
| 00440280 | `ToInt()` (a<<24 | r<<16 | g<<8 | b) | same | |
| 004402a0 / 004402e0 | `operator==` / `operator!=` | same | |
| 005698c8 | `Color::White` (data) | `Color::White` | Color::Black not located (unused) |

Behaviour: identical to the port.

### 2. Graphics (0x58, vtable 00502210, 1 slot)

| slot | address | method | 1.2 slot | port | notes |
|---|---|---|---|---|---|
| 0 +00 | 0044b810 | `~Graphics()` (scalar deleting) | 0 | `~Graphics` | body 00446bd0 only resets the vfptr |

#### Fields

| off | type | name | meaning | port | status |
|---|---|---|---|---|---|
| +0x00 | vfptr | | 00502210 | | verified |
| +0x04 | Image* | mDestImage | NULL in the ctor becomes `&mStaticImage` (005698e8, a 0x0 Image) | mDestImage | verified |
| +0x08 | **int** | mTransX | translation (1.2 and port: float) | mTransX (float) | verified (Translate adds ints, DrawImageRotatedF does `fild`) |
| +0x0c | **int** | mTransY | | mTransY (float) | verified |
| +0x10 | Rect | mClipRect | (0,0,dest w,dest h) | mClipRect | verified |
| +0x20 | Color | mColor | default (0,0,0,255) | mColor | verified |
| +0x30 | Color | (unknown) | default-constructed by both ctors, **not copied** by the copy ctor, read by no linked function | none | verified layout, meaning unknown |
| +0x40 | Font* | mFont | NULL | mFont | verified |
| +0x44 | int | mDrawMode | 0 normal, 1 additive | mDrawMode | verified |
| +0x48 | bool | mColorizeImages | false | mColorizeImages | verified |
| +0x49 | bool | mFastStretch | false | mFastStretch | verified |
| +0x4c..+0x57 | 12 bytes | probably mPFActiveEdgeList / mPFNumActiveEdges / mPFNumVertices | never initialised, no PolyFill linked | same names | inferred |

Absent compared with 1.2/port: mWriteColoredString (a **static on Widget** here, 0052f96c, value 1),
mLinearBlend, mScaleX/Y, mIs3D, mStateStack (no PushState/PopState in the binary).

#### Direct functions (all linked Graphics code) and draw calls

| address | 2004 function | port 1.3 | notes |
|---|---|---|---|
| 0044b840 | `Graphics(Image* theDestImage)` | same | NULL -> static image (so `Graphics(NULL)` has a 0x0 clip rect) |
| 0044b790 | `Graphics(const Graphics&)` | same | colour at +0x30 re-defaulted |
| 00446bd0 / 0044b810 | `~Graphics` body / deleting | | |
| 0044b8c0 | `Graphics* Create()` = `new Graphics(*this)` | same | used by Widget::WriteNumberFromStrip, EditWidget::Draw |
| 00446be0 / 00446bf0 | `GetFont()` / `SetFont(Font*)` | same | inline accessors (14 / 33 game calls) |
| 00446c00 / 00446c30 | `SetColor(const Color&)` / `GetColor()` | same | |
| 00446c40 / 00446c50 | `SetDrawMode(int)` / `GetDrawMode()` | same | |
| 00446c60 / 00446c70 | `SetColorizeImages(bool)` / `GetColorizeImages()` | same | |
| 00446c80 | `SetFastStretch(bool)` | same | |
| 00446cc0 | `Translate(int, int)` | same | |
| 00446c90 | `DrawString(const std::string&, int x, int y)` | same | `if (mFont) mFont->DrawString(this,x,y,s,mColor,mClipRect)` |
| 00446ce0 | `int StringWidth(const std::string&)` | same | no NULL check (same as port) |
| 0044b8e0 | `FillRect(int x,int y,int w,int h)` | same | returns if mColor.mAlpha==0; rect translated and clipped; Image slot 1 |
| 0044b940 | `DrawRect(int x,int y,int w,int h)` | same | outline of (w+1)x(h+1); unclipped -> Image slot 2, else 4 FillRects |
| 0044ba30 | `DrawImage(Image*, int x, int y)` | same | Image slot 6 Blt, colour = mColorizeImages ? mColor : White |
| 0044bad0 | `DrawImage(Image*, int x, int y, const Rect& src)` | same | returns silently if src exceeds the image |
| 0044bbb0 | `DrawImage(Image*, const Rect& dest, const Rect& src)` | same | stretch: dest is clipped here, src converted to a double rect, Image slot 9 `StretchBlt(img, clippedDest, FRect src, colour, mode, mFastStretch)` |
| 00450230 | `DrawImageRotated(Image*, int x, int y, double rot, const Rect* src = NULL)` | same | -> 0044f7a0 with (float)x,(float)y |
| 0044f7a0 | `DrawImageRotatedF(Image*, float x, float y, double rot, const Rect* src)` | `DrawImageRotatedF` | centre = **integer** w/2,h/2 (port's float overload uses w/2.0f) |
| 0044bce0 | `DrawImageRotatedF(Image*, float x, float y, double rot, float cx, float cy, const Rect* src)` | same | Image slot 8 BltRotated with mClipRect |
| 0044bdb0 / 0044be10 | `ClipRect(int,int,int,int)` / `ClipRect(const Rect&)` | same | intersect with translated rect |
| 0044f820 | `DrawImageBox(const Rect& dest, Image*)` | same | |
| 0044be30 | `DrawImageBox(const Rect& src, const Rect& dest, Image*)` | same | 3x3 **tiling** (corners, tiled edges and centre with clip rects), like the port |

Not in the binary (unused by BookWorm): DrawImageF, DrawImageMirror (no BltMirror slot exists),
DrawImageCel, DrawImageAnim, DrawImage(img,x,y,w,h), DrawLine/DrawLineAA, PolyFill, ClearRect,
SetClipRect/ClearClipRect, PushState/PopState, DrawImageMatrix/Transform, DrawTrianglesTex,
Graphics::WriteString / WriteWordWrapped / DrawStringColor / GetWordWrappedHeight (they are Widget
methods here, section 3). The game code never reads Graphics fields directly (heuristic scan); it uses
the inline accessors above.

#### Behaviour differences (Graphics)

1. Integer translation: identical results for integer Translate calls; nothing to do.
2. `DrawImageRotated(int ...)`: integer rotation centre. The port's int overload also uses integer
   centres. Translate 00450230 to the **int** overload; do not switch to `DrawImageRotatedF` (float
   centres differ by 0.5 px for odd sizes). Game callers: 0041c2a0 (CreditsDialog::Draw), 0042cdd0.
3. Stretch `DrawImage(dest, src)`: 2004 pre-clips dest and passes an FRect src; the port passes the clip
   rect to StretchBlt. Same pixels; fastStretch honoured in both (`DDInterface::StretchBlt` uses linear
   filtering only when !fastStretch). Nothing to do.
4. Graphics(NULL) measures text with a 0x0 clip (ShadowFont relies on it, section 14): same in port.

### 3. Text layout lives on Widget in 2004 (WriteString, WriteWordWrapped, colour codes)

These are Widget virtuals taking a `Graphics*` (slot tables in section 3); listed here because they
are the text-drawing code. Port equivalents are the `Graphics` methods.

| Widget slot | address | 2004 | port |
|---|---|---|---|
| 34 +0x88 | 004388e0 | `Rect WriteCenteredLine(Graphics*, int offset, const string&, Color, Color, const Point& shadowOffset)` | `Widget::WriteCenteredLine` |
| 35 +0x8c | 00438870 | `Rect WriteCenteredLine(Graphics*, int offset, const string&)`: x = (mWidth - w)/2, draws at y=offset, returns (x, offset-ascent, w, height) | same |
| 36 +0x90 | 0043b420 | `int WriteString(Graphics* g, const string&, int x, int y, int width, int justification, bool drawString, int offset, int length)` | `g->WriteString(s,x,y,width,just,draw,offset,length)` |
| 37 +0x94 | 00439f70 | `int WriteWordWrapped(Graphics* g, const Rect&, const string&, int lineSpacing, int justification)` | `g->WriteWordWrapped(rect,s,spacing,just)` |
| 38 +0x98 | 004389f0 | `int GetWordWrappedHeight(Graphics* g, int width, const string&, int lineSpacing)`: WriteWordWrapped on `Graphics(NULL)` with g's font, justification -1 | `g->GetWordWrappedHeight(width, s, spacing)` |
| 40 +0xa0 | 00437a40 | `WriteNumberFromStrip(Graphics*, int number, int x, int y, Image* strip, int spacing)` | Widget method in port |

WriteString (0043b420), same as 1.2 Graphics::WriteString:
- justification 0 = centre, 1 = right, anything else = left; only applied when drawString.
- `^RRGGBB^` (6 hex digits, either case) sets the colour keeping the current alpha; `^^` is a literal
  `^`; gated by the static `Widget::mWriteColoredString` (always true in BookWorm).
- Order of tests: a `^` with fewer than 8 characters left **ends the string** (checked before `^^`),
  so a `^^` in the last 7 characters stops drawing; the port (1.3) tests `^^` first and draws it.
- No `^oldclr^` (port supports it; harmless). Returns the width; the colour is **not** restored.

WriteWordWrapped (00439f70) differs from the port's 1.3 `Graphics::WriteWordWrapped`:
- After a wrap at a space it continues at **space+1 without skipping further spaces** (1.2 and the
  port skip all following spaces). A double space at a wrap point therefore indents the next line by one
  space width (and shifts centred lines).
- A trailing `\n` adds no line (the port adds `lineSpacing` to the returned height).
- No clip-rect culling, no theMaxWidth/theMaxChars/theLastWidth, no indent.
- Same: `\n` forces a wrap, colour codes are skipped when measuring, y starts at
  ascent - ascentPadding, lineSpacing -1 = font->GetLineSpacing(), a word longer than the line breaks
  mid-word (at least one char), colour restored at the end, returns
  `aYOffset + font->GetDescent() - lineSpacing`.
- Port action: game-side helper `BWWriteWordWrapped(Graphics*, Rect, string, spacing, just)` that
  reproduces the 2004 loop on top of `g->WriteString` (about 40 lines), used wherever the game calls
  Widget slot 37/38. Only needed for pixel-exact wrapping of texts with double spaces or a trailing
  `\n`; otherwise the port method is fine.
- Port default arguments differ (`Graphics::WriteString` theJustification defaults to 0 = centre, 1.2
  Widget's default was -1): always pass the justification the binary pushes.

### 4. Image (0x18, vtable 00502218, 10 slots)

| slot | address | 2004 method | 1.2 slot | port | notes |
|---|---|---|---|---|---|
| 0 +00 | 00460160 | `~Image()` | 0 | `~Image` | body 00458fa0 deletes mAnimInfo |
| 1 +04 | 0041dfa0 | `FillRect(const Rect&, const Color&, int mode)` (empty) | 2 | `FillRect` | 1.2 slot 1 PolyFill3D does not exist yet |
| 2 +08 | 0044c290 | `DrawRect(const Rect&, const Color&, int)` | 3 | `DrawRect` | 4 FillRects |
| 3 +0c | 00401570 | `ClearRect(const Rect&)` (empty) | 4 | `ClearRect` | |
| 4 +10 | 00446d20 | `DrawLine(double,double,double,double, const Color&, int)` (empty) | 5 | `DrawLine` | MemoryImage does not override: only DDImage draws lines |
| 5 +14 | 0044c360 | `FillScanLines(Span*, int, const Color&, int)` | 6 | `FillScanLines` | |
| 6 +18 | 00446d50 | `Blt(Image*, int x, int y, const Rect& src, const Color&, int mode)` (empty) | 7 | `Blt` | |
| 7 +1c | 00446d30 | `BltF(Image*, float x, float y, const Rect& src, const Rect& clip, const Color&, int)` (empty) | 8 | `BltF` | |
| 8 +20 | 00446d40 | `BltRotated(Image*, float x, float y, const Rect& src, const Rect& clip, const Color&, int, double rot, float cx, float cy)` (empty) | 9 | `BltRotated` | |
| 9 +24 | 00446d50 | `StretchBlt(Image*, const Rect& clippedDest, const FRect& src, const Color&, int mode, bool fast)` (empty) | 10 | `StretchBlt` (port: `(Image*, dest, Rect src, clip, colour, mode, fast)`) | **different signature**: no clip rect, double source rect |

Not present: PolyFill3D, BltMatrix, BltTrianglesTex, BltMirror, StretchBltMirror (and the port's
DrawLineAA, FillScanLinesWithCoverage).

Fields (verified, ctor 00446cf0 and copy ctor 00460190): +0 vfptr, +4 mWidth, +8 mHeight,
+0xc mNumRows (1), +0x10 mNumCols (1), +0x14 AnimInfo* mAnimInfo. No mNumTotal, mDrawn, mFilePath (the
same as Zuma 1.0 / CircleShootApp). Port: same names plus those three.

Direct: 00446cf0 Image(), 00460190 Image(const Image&), 00458fc0 CopyAttributes(Image*) (rows, cols,
AnimInfo), 00446d10 GetWidth, 00468620 GetHeight (folded, section 0). No GetCelWidth/GetCelRect/
GetAnimCel in the binary. AnimInfo is 0x30 (same layout as 1.2: type, delay, numCels, two vectors,
total time); 00458f30 copy ctor, 004556a0 dtor.

### 5. MemoryImage (0x48, vtable 00502ab4, 26 slots)

| slot | address | 2004 method | 1.2 slot | port | notes |
|---|---|---|---|---|---|
| 0 +00 | 00461320 | `~MemoryImage()` | 0 | same | body 00459970: RemoveMemoryImage, frees buffers |
| 1 +04 | 00447850 | `FillRect` | 2 | same | |
| 2 +08 | 0044c290 | `DrawRect` (Image) | 3 | same | |
| 3 +0c | 00447a40 | `ClearRect` | 4 | same | |
| 4 +10 | 00446d20 | `DrawLine` (Image, empty) | 5 | port draws | |
| 5 +14 | 0044c360 | `FillScanLines` (Image) | 6 | same | |
| 6 +18 | 00449160 | `Blt` | 7 | same | mode 0 -> NormalBlt 00448540, 1 -> AdditiveBlt 00447af0 |
| 7 +1c | 004491b0 | `BltF` | 8 | same | = BltRotated(..., rot 0, centre 0,0) |
| 8 +20 | 0044c7e0 | `BltRotated` | 9 | same | clip helper 0044c520 |
| 9 +24 | 0044a700 | `StretchBlt(img, dest, FRect src, colour, mode, fast)` | 10 | same name | fast -> FastStretchBlt 0044a4e0, else SlowStretchBlt 004491e0 |
| 10 +28 | 00447020 | `void* GetNativeAlphaData(int bitDepth, ulong rMask, ulong gMask, ulong bMask)` | 15 | `GetNativeAlphaData(NativeDisplay*)` | no NativeDisplay in 2004 |
| 11 +2c | 00447240 | `uchar* GetRLAlphaData()` | 16 | same | |
| 12 +30 | 00447440 | `uchar* GetRLAdditiveData(int, ulong, ulong, ulong)` | 17 | same (NativeDisplay*) | |
| 13 +34 | 00447650 | `DeleteSWBuffers()` | 19 | same | |
| 14 +38 | 00447690 | `Delete3DBuffers()` -> mApp->Remove3DData(this) | 20 | same | |
| 15 +3c | 004476a0 | `DeleteExtraBuffers()` | 21 | same | |
| 16 +40 | 00446e80 | `BitsChanged()` | 23 | same | ++mBitsChangedCount, mBitsChanged, drops alpha caches |
| 17 +44 | 00446ec0 | `CommitBits()` | 24 | same | recomputes mHasTrans/mHasAlpha unless mForcedMode |
| 18 +48 | 004476b0 | `DeleteNativeData()` | 25 | same | |
| 19 +4c | 00447ab0 | `Clear()` | 26 | same | |
| 20 +50 | 004476e0 | `SetBits(ulong* bits, int w, int h)` | 27 | `SetBits(bits,w,h,commitBits=true)` | **no commitBits**: always BitsChanged + CommitBits |
| 21 +54 | 00447760 | `Create(int w, int h)` | 28 | same | sets mHasTrans = mHasAlpha = true |
| 22 +58 | 004477a0 | `ulong* GetBits()` | 29 | same | allocates zeroed bits or expands the palette |
| 23 +5c | 00446ff0 | `SetImageMode(bool hasTrans, bool hasAlpha)` | 30 | same | sets mForcedMode |
| 24 +60 | 00447010 | `SetVolatile(bool)` | 31 | same | |
| 25 +64 | 0044e450 | `bool Palletize()` | 32 | same | Quantize32Bit 0044a800 |

Not present: PurgeBits, ReInit (1.2 slots 18, 22), BltMatrix, BltTrianglesTex, the HD API.

Fields (ctor 004598f0 `MemoryImage(SexyAppBase*)`, copy ctor 00461350, dtor 00459970):

| off | type | name | port | status |
|---|---|---|---|---|
| +0x18 | ulong* | mBits (w*h+1, last = 0x4BEEFADE) | mBits | verified |
| +0x1c | int | mBitsChangedCount | same | verified (BitsChanged) |
| +0x20 | void* | mD3DData | same | verified (copy ctor zeroes it) |
| +0x24 | DWORD | mD3DFlags | same | inferred |
| +0x28 | ulong* | mColorTable (256 entries) | same | verified |
| +0x2c | uchar* | mColorIndices | same | verified |
| +0x30 | bool | mForcedMode | same | verified (SetImageMode) |
| +0x31 | bool | mHasTrans | same | verified |
| +0x32 | bool | mHasAlpha | same | verified |
| +0x33 | bool | mIsVolatile | same | verified |
| +0x34 | ulong* | mNativeAlphaData | same | verified |
| +0x38 | uchar* | mRLAlphaData | same | verified |
| +0x3c | uchar* | mRLAdditiveData | same | verified |
| +0x40 | bool | mBitsChanged | same | verified |
| +0x44 | SexyAppBase* | mApp | same | verified |

No mPurgeBits, mWantPal. The ctor registers the image with `SexyAppBase::AddMemoryImage` (00488b50,
set at app+0x494), the dtor removes it (004874a0).

Game use: `new MemoryImage(app)` (0x48) then Create, SetImageMode(true,true), `new Graphics(img)`
(0042b9c0, 0041a8a0 tile sets). Port: identical calls.

### 6. DDImage (0xc4, vtable 00502b34, 37 slots)

| slot | address | 2004 method | 1.2 slot | port | notes |
|---|---|---|---|---|---|
| 0 +00 | 00461890 | `~DDImage()` | 0 | same | body 0045aa70 releases mSurface |
| 1 +04 | 00455360 | `FillRect` | 2 | same | DDraw path when !alpha, (!trans or firstPixelTrans), not 3D |
| 2 +08 | 0044c290 | `DrawRect` (Image) | 3 | same | |
| 3 +0c | 00447a40 | `ClearRect` (MemoryImage) | 4 | same | |
| 4 +10 | 00455410 | `DrawLine` | 5 | same | -> slots 32/33 |
| 5 +14 | 0044c360 | `FillScanLines` (Image) | 6 | same | |
| 6 +18 | 0045aad0 | `Blt` | 7 | same | -> slots 30/31 or MemoryImage::Blt |
| 7 +1c | 0045abd0 | `BltF` | 8 | same | |
| 8 +20 | 0045acb0 | `BltRotated` | 9 | same | |
| 9 +24 | 0045e200 | `StretchBlt(img, dest, FRect src, colour, mode, fast)` | 10 | same name | |
| 10 +28 | 00447020 | `GetNativeAlphaData(int, ulong, ulong, ulong)` (MemoryImage) | 15 | same (NativeDisplay*) | |
| 11 +2c | 00447240 | `GetRLAlphaData()` (MemoryImage) | 16 | same | |
| 12 +30 | 00447440 | `GetRLAdditiveData(int, ulong, ulong, ulong)` (MemoryImage) | 17 | same | |
| 13 +34 | 00447650 | `DeleteSWBuffers()` (MemoryImage) | 19 | same | |
| 14 +38 | 00447690 | `Delete3DBuffers()` (MemoryImage) | 20 | same | |
| 15 +3c | 0044b4e0 | `DeleteExtraBuffers()` (jmp MemoryImage's) | 21 | same | |
| 16 +40 | 0044b500 | `BitsChanged()` + release surface | 23 | same | |
| 17 +44 | 0044b4f0 | `CommitBits()` only without a surface | 24 | same | |
| 18 +48 | 004476b0 | `DeleteNativeData()` (MemoryImage) | 25 | same | |
| 19 +4c | 00447ab0 | `Clear()` (MemoryImage) | 26 | same | |
| 20 +50 | 004476e0 | `SetBits(ulong*, int, int)` (MemoryImage) | 27 | same (+commitBits) | |
| 21 +54 | 00443050 | `Create(int, int)` | 28 | same | **does not** set mHasTrans/mHasAlpha (port's does via MemoryImage::Create) |
| 22 +58 | 0044b520 | `GetBits()` (reads the surface back) | 29 | same | |
| 23 +5c | 00446ff0 | `SetImageMode(bool, bool)` (MemoryImage) | 30 | same | |
| 24 +60 | 00447010 | `SetVolatile(bool)` (MemoryImage) | 31 | same | |
| 25 +64 | 0044f780 | `Palletize()` | 32 | same | |
| 26 +68 | 00441830 | `SetVideoMemory(bool)` | 33 | non-virtual stub | |
| 27 +6c | 00441860 | `RehupFirstPixelTrans()` | 34 | not in port | |
| 28 +70 | 00443090 | `NormalFillRect(const Rect&, const Color&)` | 35 | not in port | |
| 29 +74 | 00443540 | `AdditiveFillRect` | 36 | not in port | |
| 30 +78 | 00443830 | `NormalBlt(Image*, int, int, const Rect&, const Color&)` | 37 | not in port | |
| 31 +7c | 00445a50 | `AdditiveBlt` | 38 | not in port | |
| 32 +80 | 00441950 | `NormalDrawLine(double x4, const Color&)` | 39 | not in port | |
| 33 +84 | 00442760 | `AdditiveDrawLine` | 40 | not in port | |
| 34 +88 | 0044b490 | `bool LockSurface()` | 43 | non-virtual stub | |
| 35 +8c | 00440db0 | `bool UnlockSurface()` | 44 | non-virtual stub | |
| 36 +90 | 00440de0 | `SetSurface(LPDIRECTDRAWSURFACE)` | 45 | non-virtual stub | |

Not present: Normal/AdditiveBltMirror (1.2 slots 41/42), PurgeBits/ReInit/DeleteNativeData overrides.
Direct: 0045aa00 `DDImage(DDInterface*)`, 00440e40 `GenerateDDSurface()`, 004417d0
`DeleteDDSurface()`, 00441930 `GetSurface()`, 00440d90 `Check3D()` (inferred), 00441800 non-virtual
PurgeBits-like "GenerateDDSurface then free mBits" (inferred; app debug overlay only).

Fields (ctor 0045aa00, SetSurface, GenerateDDSurface):

| off | type | name | status | port |
|---|---|---|---|---|
| +0x48 | DDInterface* | mDDInterface | verified | mDDInterface |
| +0x4c | LPDIRECTDRAWSURFACE | mSurface | verified | none |
| +0x50 | bool | mNoLock (SetSurface clears it) | inferred | none |
| +0x51 | bool | mVideoMemory (SetVideoMemory; DDSCAPS_VIDEOMEMORY) | verified | mVideoMemory |
| +0x52 | bool | mFirstPixelTrans | inferred | none |
| +0x53 | bool | mWantDDSurface (set by GenerateDDSurface and by the game) | verified | mWantDDSurface |
| +0x54 | int | mLockCount | verified | none |
| +0x58 | DDSURFACEDESC (0x6c) | mLockedSurfaceDesc | verified | none |

No mSurfaceSet, mDrawToBits.

Game use: `new DDImage(app->mDDInterface)` (0xc4) + Create + `SetImageMode(false,false)` for an opaque
offscreen buffer (BWGameSelector 004010c0/004011f0); and the game helper 00411220 (called after each
image load): `if (!img->Palletize()) { if (app->Is3DAccelerated()) ddimg->mWantDDSurface = true; else
ddimg->GenerateDDSurface(); }` with debug strings "%s - PALLETIZED!" / "%s - 32 Bit Only". Port: keep
`Palletize()`, drop the surface branch (or call the port's no-op `GenerateDDSurface()`).

### 7. ImageLib and the alpha-mask convention

`ImageLib::GetImage(const std::string& theFileName, bool lookForAlphaImage)` 00491750 (cdecl):
- Extension = text after the last '.' if that is after the last path separator. With an explicit
  extension only that loader runs (`.tga`, `.jpg`, `.png`, `.gif`, case-insensitive); without one they
  are tried in that order: **.tga, .jpg, .png, .gif** (no .bmp, .jpeg, .j2k). All loaders use `fopen`
  on loose files (no PakInterface).
- Loaders: 004908b0 TGA (32-bit, image descriptor 0x28 only), 00491550 JPEG, 004906c0 PNG (libpng
  1.0.5), 00490a10 GIF (transparent index -> pixel 0x00000000). 004913c0 / 004911e0 are PNG / JPEG
  writers used by the app's screenshot code (inferred).
- Alpha (`lookForAlphaImage`, always true from SexyAppBase::GetImage): load `dir + "_" + name` (same
  extension text as given), else `theFileName + "_"`, both with lookForAlphaImage=false.
  - colour and mask found, same size: alpha byte of each colour pixel = **low byte (blue)** of the mask
    pixel; any alpha the colour file had is replaced. Different size: the mask is silently ignored.
  - only the mask found: white image, alpha = mask's blue byte (`p = p << 24 | 0xFFFFFF`).
  - There is no other variant (no "a" prefix, no "_a" suffix) in this framework; BookWorm ships 127
    `_name.gif` / `_name.png` / `_clouds.jpg` masks and no `name_` files.
- Quirk: the separator used for "dir" is chosen as the *earlier* of the last '/' and last '\\' when a
  path mixes both (only one kind ever occurs in BookWorm paths).

Port: `platform/ImageLoader.cpp` LoadGameImage implements the same rules (order .tga .jpg .png .gif,
then .jpeg .bmp; `_name` then `name_`; blue byte; size mismatch ignored; mask-only = white) and is a
superset (any mask extension, mods, HD). Nothing to do. One low-risk difference: for GIF transparent
pixels SAF stores 0 (black) while stb_image keeps the palette RGB with alpha 0; only 2 colour GIFs use a
transparent index (fonts/LockergnomeSmallColor.gif, which has a mask anyway, and
images/checkboxes_help.gif).

ImageLib::Image (0x10, vtable 1 slot, dtor 00490690): +4 mWidth, +8 mHeight, +0xc mBits.

### 8. App image helpers called by the game

| address | 2004 | port (`SexyAppBase`) | notes |
|---|---|---|---|
| 0047d930 | `DDImage* GetImage(const std::string& theFileName)` | `GetImage(file, commitBits=true)` | ImageLib::GetImage(file,true), `new DDImage(mDDInterface)` (app+0x334), `SetBits(bits,w,h)`, deletes the ImageLib image; no mFilePath, no commitBits arg. NULL on failure. Game: 004144b0 (name = property "ImagePath" default `images\\` + table name; on failure Popup "Failed to load ..." and sets app+0x528) |
| 0047e330 | `DDImage* CopyImage(Image*)` | same | |
| 0047d9f0 | `DDImage* CopyImage(Image*, const Rect&)` | same | new DDImage, Create(w,h), `Graphics(img).DrawImage(src,-x,-y)`, CopyAttributes |
| 0047dab0 | `void FlipImage(Image*)` | `FlipImage` | **vertical** flip (dynamic_cast to MemoryImage, swap rows, BitsChanged). Game: HelpWidget 00429d90. MirrorImage is not in the binary |
| 00488b50 / 004874a0 / 0047dd90 | AddMemoryImage / RemoveMemoryImage / Remove3DData | same | framework-internal |

Not in the binary: GetSharedImage, CreateColorizedImage, ColorizeImage, CreateCrossfadeImage,
MirrorImage, RotateImageHue (nothing uses them).

### 9. Font (0x14, vtable 005021dc, 12 slots)

| slot | address | method | 1.2 | port | notes |
|---|---|---|---|---|---|
| 0 | 0044b760 | `~Font()` | 0 | same | |
| 1 | 00446d10 | `int GetAscent()` | 1 | same | folded with Image::GetWidth |
| 2 | 00468620 | `int GetAscentPadding()` | 2 | same | folded with Image::GetHeight |
| 3 | 00446b90 | `int GetDescent()` = mHeight - mAscent | 3 | same | |
| 4 | 00496c00 | `int GetHeight()` | 4 | same | |
| 5 | 0041d910 | `int GetLineSpacingOffset()` | 5 | same | |
| 6 | 00446ba0 | `int GetLineSpacing()` = mHeight + mLineSpacingOffset | 6 | same | |
| 7 | 00446bb0 | `int StringWidth(const string&)` (returns 0) | 7 | same | |
| 8 | 00455610 | `int CharWidth(char)` = StringWidth(string(1,c)) | 8 | same | |
| 9 | 00446bc0 | `int CharWidthKern(char, char prev)` = CharWidth(c) | 9 | same | |
| 10 | 00446d50 | `DrawString(Graphics*, int x, int y, const string&, const Color&, const Rect& clip)` (empty) | 10 | same | |
| 11 | 004dc641 | `Font* Duplicate()` (pure) | 11 | same | |

Fields (verified, ctor 00446b30): +4 mAscent, +8 mAscentPadding, +0xc mHeight, +0x10
mLineSpacingOffset. Identical to 1.2 and the port.

### 10. ImageFont (0x48, vtable 00502a4c, 23 slots)

| slot | address | method | 1.2 | port |
|---|---|---|---|---|
| 0 | 00459200 | `~ImageFont()` (body 00458670: FontData DeRef, frees layers/tags) | 0 | same |
| 1 | 00446d10 | `GetAscent()` (Font) | 1 | same |
| 2 | 00468620 | `GetAscentPadding()` (Font) | 2 | same |
| 3 | 00446b90 | `GetDescent()` (Font) | 3 | same |
| 4 | 00496c00 | `GetHeight()` (Font) | 4 | same |
| 5 | 0041d910 | `GetLineSpacingOffset()` (Font) | 5 | same |
| 6 | 00446ba0 | `GetLineSpacing()` (Font) | 6 | same |
| 7 | 0044f8e0 | `StringWidth` = sum of CharWidthKern(c, prev) | 7 | same |
| 8 | 00446db0 | `CharWidth(c)` = CharWidthKern(c, 0) | 8 | same |
| 9 | 004502c0 | `CharWidthKern(c, prev)` (Prepare; max over active layers, scaled by mScale or point size) | 9 | same |
| 10 | 00446dd0 | `DrawString(g,x,y,s,colour,clip)` -> DrawStringEx(..., &clip, NULL, NULL) | 10 | same |
| 11 | 00462f70 | `Duplicate()` = `new ImageFont(*this)` (0x48) | 11 | same |
| 12 | 00460cc0 | `GenerateActiveFontLayers()` | 12 | same |
| 13 | 00459230 | `DrawStringEx(Graphics*, int x, int y, const string&, const Color&, const Rect* clip, RectList* drawnAreas, int* width)` | 13 | same |
| 14 | 00446e00 | `SetPointSize(int)` (invalidates layers) | 14 | same |
| 15 | 00446e40 | `int GetPointSize()` | 15 | same |
| 16 | 00446e20 | `SetScale(double)` (invalidates layers) | 16 | same |
| 17 | 00446e50 | `int GetDefaultPointSize()` = mFontData->mDefaultPointSize | 17 | same |
| 18 | 00462fd0 | `bool AddTag(const string&)`: false if HasTag(tag) (case-sensitive), else push StringToUpper(tag) | 18 | same |
| 19 | 00459820 | `bool RemoveTag(const string&)` (finds StringToUpper(tag)) | 19 | same |
| 20 | 004503f0 | `bool HasTag(const string&)` (exact-case find) | 20 | same |
| 21 | 00455c90 | `string GetDefine(const string&)` ("" if undefined) | 21 | same |
| 22 | 00446e60 | `Prepare()` (GenerateActiveFontLayers if !mActiveListValid) | 22 | same |

Direct: 00465d60 `ImageFont(SexyAppBase*, const std::string& descFile)` (the one the game uses),
00465e30 `ImageFont(Image*, const std::string&)` (legacy, ResourceManager only), 00462ec0
`ImageFont(const ImageFont&)` (FontData Ref; copies the active layer list when valid, owned scaled
images are duplicated with SexyAppBase::CopyImage).

Fields (verified, ctors): +0x14 FontData* mFontData, +0x18 int mPointSize (= mDefaultPointSize),
+0x1c StringVector mTagVector (first +0x20, last +0x24, end +0x28), +0x2c bool mActiveListValid,
+0x30 ActiveFontLayerList mActiveLayerList (head +0x34, size +0x38), +0x40 double mScale (1.0).
**No mForceScaledImagesWhite** (1.2 +0x48), so subclasses (ShadowFont) start at +0x48.

Behaviour: GenerateActiveFontLayers and DrawStringEx are the 1.2 algorithms (checked in the asm):
- layer chosen when minPointSize <= mPointSize <= maxPointSize (-1 = open) and every required tag is in
  mTagVector and no excluded tag is;
- when mScale==1.0 and (layer point size 0 or equal) the layer image is used directly; otherwise a
  `new MemoryImage(app)` is filled with `Graphics::DrawImage(layerImg, scaledRect, charRect)` (stretch)
  and owned by the ActiveFontLayer;
- font metrics: max ascent, max height (layer mHeight if non-zero else mDefaultHeight), min ascent
  padding, max line spacing offset, all scaled by pointSize/layerPointSize (computed as x*pt*(1/L));
- DrawStringEx: scale = mScale * (mPointSize / layerPointSize) with **integer** division (same in 1.2
  and the port); characters go through mCharMap; next-char kerning + layer spacing; colour = min(255,
  layer.mColorMult*colour/255 + layer.mColorAdd) per channel; commands are sorted into 256 order
  buckets (charData.mOrder + layer.mBaseOrder + 128, clamped) from a static pool of 4096
  RenderCommands under a critical section, then drawn with colorize on, each with its layer draw mode
  (-1 = keep), via `Graphics::DrawImage(img, x, y, srcRect)`; colour and colorize flag restored;
  *theWidth = final x - start x; theClipRect is ignored.
- Pool overflow: 2004 drops the remaining characters; the port draws them directly. Irrelevant.
- Tags: no BookWorm descriptor uses LayerRequireTags/LayerExcludeTags and the binary contains no tag
  strings ("outline", "noshadow" do not occur); the game never calls AddTag/SetPointSize/SetScale as
  far as a scan of the font-array uses shows. Shadows are done by the game's ShadowFont instead.

Game use: 004146d0 builds the path from property "FontPath" (default `fonts\\`) + one of 20
descriptor names (Verdana10Bold.txt ... MyriadWeb12Bold.txt, table 0052e3c0), `new ImageFont(app,
path)`, stores it at app+0x11b0+i*4, and reports "Failed to load ..." when
`font->mFontData->mInitialized` is false. Port: same constructor and field.

### 11. FontData, FontLayer, CharData, ActiveFontLayer, RenderCommand

FontData (0x198, vtable 005030c0 with only the destructor; **no DescParser base class exists in
2004**, the parser is part of FontData):

| off | type | name | port | status |
|---|---|---|---|---|
| +0x04 | bool | mInitialized | same | verified |
| +0x08 | int | mRefCount (Ref 0041e8c0 folded, DeRef 00446d80) | same | verified |
| +0x0c | SexyAppBase* | mApp | same | verified |
| +0x10 | int | mDefaultPointSize | same | verified |
| +0x14 | uchar[256] | mCharMap (identity) | same | verified |
| +0x114 | map<string, Image*> | layer image cache (path -> image, owned; new in 2004) | none (port: GetSharedImage) | verified |
| +0x120 | list<FontLayer> | mFontLayerList | same | verified |
| +0x12c | map<string, FontLayer*> | mFontLayerMap | same | verified |
| +0x138 | string | mSourceFile | same | verified |
| +0x154 | string | mFontErrorHeader ("Font Descriptor Error in " + file + "\r\n") | same | verified |
| +0x170 | string | mCurrentLine ("Line N:\r\n\r\n" + command) | DescParser::mCurrentLine | verified |
| +0x18c | map<string, DataElement*> | mDefineMap (keys upper-cased) | DescParser::mDefineMap | verified |

Missing vs port DescParser: mCmdSep, mError, mCurrentLineNum.

Parser (FontData::Load 00465150): returns false if already initialised; opens a std::ifstream (no
error if missing: just false); a `#` as first non-blank character comments out the line; `'` and `"`
quote (backslash escapes inside quotes); `;` outside quotes ends a command; `\r` ignored. Each command
goes to HandleCommand 00463100 (which itself calls ParseToList). Errors: FontData::Error 004557e0 ->
`mApp->Popup(header + msg + " on " + mCurrentLine)` (app slot 21). Port: `DescParser::LoadDescriptor`
with CMDSEP_SEMICOLON behaves the same for these files; the port logs instead of a popup.

Descriptor commands (HandleCommand, all 25 of 1.2): Define, CreateHorzSpanRectList,
SetDefaultPointSize, SetCharMap, CreateLayer, CreateLayerFrom, LayerRequireTags, LayerExcludeTags,
LayerPointRange, LayerSetPointSize, LayerSetImage, LayerSetDrawMode, LayerSetColorMult,
LayerSetColorAdd, LayerSetAscent, LayerSetAscentPadding, LayerSetLineSpacingOffset, LayerSetOffset,
LayerSetCharWidths, LayerSetSpacing, LayerSetImageMap ("Layer image not set", "Image rectangle out of
bounds"), LayerSetCharOffsets, LayerSetKerningPairs, LayerSetBaseOrder, LayerSetCharOrders. Port: the
same 25 plus LayerSetHeight. BookWorm's 20 descriptors use only Define, CreateLayer, LayerSetImage,
LayerSetImageMap, LayerSetCharWidths, LayerSetCharOffsets, LayerSetKerningPairs, LayerSetAscent,
LayerSetAscentPadding, LayerSetLineSpacingOffset, LayerSetPointSize, SetDefaultPointSize (single
"Main" layer each). LayerSetImage: path = GetPathFrom(name, GetFileDir(mSourceFile)) (e.g.
`'../fonts/Verdana12Bold'`), looked up in the +0x114 cache, else `mApp->GetImage(path)` then
`Palletize()` (slot 25), "Failed to Load Image" on NULL. Port: GetSharedImage + Palletize: same pixels.

Parser helpers (FontData members here, DescParser members in 1.2/port): Dereference 00450ee0,
GetValues 00460620, DataToString 00455a40, DataToList 00460900, DataToStringVector 004629d0,
DataToIntVector 00462c20, DataToDoubleVector 00462d70, ParseToList 00460950, DataElementToString
004558c0, DataToLayer 00455b70, Unquote 00450f90 (free function); LoadLegacy 004655a0.

FontLayer (0x1207c, list node 0x12084; ctor 004590d0, copy 00460490):

| off | field | default |
|---|---|---|
| +0x0 | FontData* mFontData | |
| +0x4 | StringVector mRequiredTags | |
| +0x14 | StringVector mExcludedTags | |
| +0x24 | CharData mCharData[256] | |
| +0x12024 | Color mColorMult | White |
| +0x12034 | Color mColorAdd | (0,0,0,0) |
| +0x12044 | **Image* mImage** (raw, owned by the FontData cache; port: SharedImageRef) | NULL |
| +0x12048 | int mDrawMode | -1 |
| +0x1204c | Point mOffset | 0 |
| +0x12054 | int mSpacing | |
| +0x12058 / +0x1205c | int mMinPointSize / mMaxPointSize | -1 / -1 |
| +0x12060 | int mPointSize | |
| +0x12064 | int mAscent | |
| +0x12068 | int mAscentPadding | |
| +0x1206c | int mHeight | |
| +0x12070 | int mDefaultHeight | |
| +0x12074 | int mLineSpacingOffset | |
| +0x12078 | int mBaseOrder | |

CharData (0x120, ctor 0044c3f0): +0 Rect mImageRect, +0x10 Point mOffset, +0x18 char
mKerningOffsets[256], +0x118 int mWidth, +0x11c int mOrder.

ActiveFontLayer (0x1010, vtable 00502250 dtor only, list node 0x1018): +4 FontLayer* mBaseFontLayer,
+8 Image* mScaledImage, +0xc bool mOwnsImage, +0x10 Rect mScaledCharImageRects[256]. Ctors 0044c420 /
0044c480 (copy, CopyImage of an owned image), dtor body 00446d90.

RenderCommand (0x34): mImage, mDest[2], mSrc[4], mMode, Color mColor, mNext. Pool 005358b0 (4096),
heads 00535488, tails 00535080, critical section 005698b0.

### 12. DataElement family (same as 1.2 / port DescParser.h)

| class | sizeof | vtable | slots |
|---|---|---|---|
| DataElement | 8 (+4 bool mIsList) | 00502244 | 0 `~DataElement` 0044c3c0, 1 `Duplicate` (pure, 004dc641) |
| SingleDataElement | 0x24 (+8 std::string mString) | 00502334 | 0 00450870, 1 `Duplicate` 004556e0 |
| ListDataElement | 0x18 (+8 ElementVector) | 00502604 | 0 004585b0, 1 `Duplicate` 00460430 |

Ctors: DataElement 00446d60, SingleDataElement() 00450260, SingleDataElement(string) 00450df0, copy
00450e70; ListDataElement() 00459080, copy 00460250, operator= 00460340; the List dtor deletes its
children. Port: identical API.

### 13. SysFont (0x1c, vtable 0050226c, 12 slots) — not in the port

| slot | address | method |
|---|---|---|
| 0 | 0044e4f0 | `~SysFont()` (DeleteObject) |
| 1-6 | Font | 00446d10, 00468620, 00446b90, 00496c00, 0041d910, 00446ba0 (as Font) |
| 8 | 00455610 | `CharWidth` (Font) |
| 9 | 00446bc0 | `CharWidthKern` (Font) |
| 7 | 0044fa30 | `StringWidth`: DrawTextEx(DT_CALCRECT) on the window DC |
| 10 | 0044fac0 | `DrawString`: GetDC of the destination DDImage's surface, TextOut at (x + transX, y + transY - mAscent + 1), SetTextColor(RGB) (alpha ignored), clipped to theClipRect; nothing drawn if the destination is not a DDImage |
| 11 | 0044a7e0 | `Duplicate` (copy ctor 0044a750) |

Fields: Font + 0x14 HFONT mHFont, +0x18 SexyAppBase* mApp. Ctor 0044f950 `SysFont(SexyAppBase*, const
std::string& face, int pointSize, int script, bool bold, bool italics, bool underline)`: CreateFontA
height -MulDiv(pt, LOGPIXELSY, 72), weight 700/400, OUT_TT_PRECIS, ANTIALIASED_QUALITY; mHeight =
tmHeight, mAscent = tmAscent (padding/offset 0). (Bejeweled's 1.3 SysFont was 0x20 with mDrawShadow.)

Who creates one: only as a fallback when a widget has no font: game ColorizedButton::Draw 0041b6a0 and
ImagelessButton::Draw 0042b590 ("SansSerif", 10, ANSI, plain) when the label is non-empty and mFont is
NULL; framework ButtonWidget/DialogButton/HyperlinkWidget/EditWidget Draw ("SansSerif" 10), Dialog::Draw
("SansSerif" 14 header, 12 lines, DEFAULT_CHARSET, not bold); the app's debug overlay ("Tahoma" 8).
Port action: the port has no SysFont (its widgets skip text without a font). Game side: make sure every
button/dialog gets an ImageFont (the game sets fonts in 00414080, 00429d90, 00434380); if any label
can reach Draw without one, add a game-side fallback (e.g. the Verdana10Bold ImageFont).

### 14. ShadowFont (game class) and the framework parts it relies on

ShadowFont (vtable 0050117c, 23 slots, sizeof 0x60) : ImageFont. Own fields +0x48 Color mShadowColor,
+0x58 Point mShadowOffset. Overrides slot 0 (dtor 004331d0), 7 StringWidth 004330d0, 10 DrawString
00433160, 11 Duplicate 004332a0 (new 0x60, copy ctor 00433200). Ctor 00433250 `ShadowFont(const
ImageFont&, const Color& shadow, const Point& offset)`; created in 00434380 (shadow colours red and
black from the app's font table at app+0x11b0).

Framework dependencies:
- `ImageFont(const ImageFont&)` 00462ec0 (shares FontData by Ref, copies tags/scale/active layers) and
  `~ImageFont` body 00458670.
- `sizeof(ImageFont) == 0x48`: the subclass fields start right after mScale (no
  mForceScaledImagesWhite). In the port, ShadowFont is a normal C++ subclass; offsets do not matter.
- Non-virtual call to `ImageFont::DrawStringEx` (slot 13 body 00459230) twice: shadow at
  (x+dx, y+dy) in mShadowColor when the offset is non-zero, then the text in the given colour, both with
  theClipRect passed through, drawnAreas/width NULL.
- StringWidth = `|dx| + width` where width comes from DrawStringEx on a `Graphics(NULL)` (0x0 static
  image, nothing drawn) with Color(): so widths include kerning exactly like drawing, not
  ImageFont::StringWidth.
- Inherits CharWidth / CharWidthKern / metrics / tags from ImageFont.
Port: `ImageFont(const ImageFont&)`, `DrawStringEx` (same signature), `Graphics(NULL)` with a static
image all exist; ShadowFont ports as game code unchanged.

## 6. Sound and music

BookWorm Deluxe's sound layer is SAF 1.2's DSoundManager/DSoundInstance and BassMusicInterface/FModMusicInterface
with some members removed (no sound-by-name, no ReleaseSound, no music unload / pause-all / max volume).
The code that remains is almost line for line 1.2. Everything below was checked against the decompilation
and, for ret sizes, call arguments and constants, against the disassembly. "Inferred" marks the rest.

Sizes (from `operator new` at the creation sites and from the ctors):

| class | BookWorm | SAF 1.2 (Big Money / source) | port |
|---|---|---|---|
| SoundManager (base, vptr only) | 4 | 4 | abstract, 18 virtuals (`framework/SoundManager.h`) |
| DSoundManager | **0x14a0** (new at 0048b262) | 0x30a0 | SDLSoundManager |
| SoundInstance (base) | 4 | 4 | abstract, 12 virtuals |
| DSoundInstance | **0x38** (new at 00493d90) | 0x38 | SDLSoundInstance |
| MusicInterface (base, stub) | **4** (new at 0047d826) | 4 | `framework/MusicInterface.h`, 20 virtuals |
| BassMusicInterface | **0x14** (new at 0047d89b) | 0x14 | SDLMusicInterface |
| BassMusicInfo (map value) | **0x20** | 0x28 | `SDLMusicInterface::SongInfo` |
| FModMusicInterface | **0x20** (new at 0047d864) | 0x20 | not in port |
| FModMusicInfo | **0x20** | 0x28 | not in port |
| BASS_INSTANCE | **0x64** (new at 004926d0) | 0x68 | not in port (Platform::Mixer) |
| FMOD_INSTANCE | **0x80** (new at 00492bd0) | 0x80 | not in port |

Globals: `gBass` 0056b97c, `gBassLoadCount` 0056b980, `gFMod` 0056b984, `gFModLoadCount` 0056b988, `gDSoundDLL`
0056b8b0 (dsound.dll handle, loaded by the SexyApp ctor at 0048fb76).

Who creates what (SexyApp, see section 4 (SexyApp) for the fields): `SexyApp::Init` (slot 16, 0048aaf0) does
`mSoundManager(+0x4e0) = new DSoundManager(mNoSoundNeeded(+0x90) ? NULL : mHWnd(+0x310), mWantFMod(+0x91))`
(0048b262), then `SetSfxVolume(mSfxVolume +0x78)` (slot 48), `mMusicInterface(+0x33c) = CreateMusicInterface(mInvisHWnd +0x314)`
(slot 15, 0047d800), then `SetMusicVolume(mMusicVolume +0x70)` (slot 47). CreateMusicInterface returns a plain
MusicInterface if mNoSoundNeeded, FModMusicInterface if mWantFMod, otherwise BassMusicInterface(hwnd). Both flags are
only ever written by the SexyApp ctor (0048ff8e, 0048ff94: zero), so **BookWorm always uses DSoundManager without
FMOD and BassMusicInterface**; FModMusicInterface and FMOD_INSTANCE are dead code (and no fmod.dll is installed).
Default volumes: mMusicVolume = mSfxVolume = 0.85 (0048fefa), same as the port.

---------------------------------------------------------------------------------------------------------------

#### 1. SoundManager / DSoundManager

Bases: DSoundManager : SoundManager (vtables 00507328 / 005069f4, 14 slots; 1.2 has 16, the port 18).

##### Vtable (DSoundManager 00507328; the SoundManager base has `_purecall` 004dc641 in slots 1-13)

| slot (+off) | address | method | 1.2 slot | port method | notes |
|---|---|---|---|---|---|
| 0 (+00) | 00493250 | `~DSoundManager()` scalar deleting | 0 | `~SDLSoundManager` | body 00492e00 |
| 1 (+04) | 00491d90 | `bool Initialized()` = mDirectSound != NULL | 1 | `Initialized()` | |
| 2 (+08) | 004954f0 | `bool LoadSound(unsigned int theSfxID, const std::string& theFilename)` ret 8 | 3 | `LoadSound(unsigned, const std::string&)` | 1.2 slot 2 `int LoadSound(const std::string&)` does **not exist** |
| 3 (+0c) | 00491de0 | `bool SetBaseVolume(unsigned int id, double v)` ret 0xc | 6 | `SetBaseVolume` | reordered: before SetVolume |
| 4 (+10) | 00491e10 | `bool SetBasePan(unsigned int id, int pan)` ret 8 | 7 | `SetBasePan` | |
| 5 (+14) | 00492f10 | `void SetVolume(double v)` ret 8 (= sfx volume) | 5 | `SetVolume` | |
| 6 (+18) | 00493d90 | `SoundInstance* GetSoundInstance(unsigned int id)` ret 4 | 8 | `GetSoundInstance` | |
| 7 (+1c) | 00491e40 | `void ReleaseSounds()` | 9 | `ReleaseSounds` | |
| 8 (+20) | 00491e70 | `void ReleaseChannels()` | 10 | `ReleaseChannels` | |
| 9 (+24) | 00491f10 | `double GetMasterVolume()` | 11 | `GetMasterVolume` | Windows mixer, see 1.4 |
| 10 (+28) | 00492010 | `void SetMasterVolume(double v)` ret 8 | 12 | `SetMasterVolume` | Windows mixer |
| 11 (+2c) | 004036d0 | `void Flush()` (folded empty `ret`) | 13 | `Flush` | |
| 12 (+30) | 00492110 | `void SetCooperativeWindow(HWND, bool isWindowed)` ret 8 | 14 | `SetCooperativeWindow` | always DSSCL_NORMAL |
| 13 (+34) | 00491ee0 | `void StopAllSounds()` | 15 | `StopAllSounds` | keeps each instance's mAutoRelease |
| - | - | (absent) `int LoadSound(const std::string&)`, `void ReleaseSound(unsigned)` | 2, 4 | exist in port | |
| - | - | (absent) | - | `GetFreeSoundId`, `GetNumSounds` (1.3) | |

##### Fields (DSoundManager, 0x14a0)

| off | type | name | meaning | port field | |
|---|---|---|---|---|---|
| +0x0 | vptr | | | | |
| +0x4 | LPDIRECTSOUNDBUFFER[256] | mSourceSounds | one static buffer per sound id (MAX_SOURCE_SOUNDS 256) | `mSounds[256]` (shared_ptr<SoundData>) | verified (ctor, LoadSound, GetSoundInstance) |
| +0x404 | ulong[256] | mSourceDataSizes | PCM bytes per id, set by every loader, read by WriteWAV | - | verified |
| +0x804 | (pad) | | 1.2's mSourceFileNames[256] (std::string) and mPrimaryBuffer are gone | - | verified (never accessed) |
| +0x808 | double[256] | mBaseVolumes | 1.0 | `mBaseVolumes` | verified |
| +0x1008 | int[256] | mBasePans | 0 | `mBasePans` | verified |
| +0x1408 | DSoundInstance*[32] | mPlayingSounds | channels (MAX_CHANNELS 32) | `mInstances[32]` | verified |
| +0x1488 | double | mMasterVolume | **the sfx volume** set by SetVolume (not by SetMasterVolume), 1.0 | `mMasterVolume` | verified |
| +0x1490 | DWORD | mLastReleaseTick | FindFreeChannel's 1000 ms sweep | - | verified |
| +0x1494 | LPDIRECTSOUND | mDirectSound | NULL when no device / mNoSoundNeeded | - | verified |
| +0x1498 | bool | mHaveFMod | = mWantFMod, always false | - | verified |

##### Direct (non-virtual) functions

| address | name / signature | 1.2 / port | notes |
|---|---|---|---|
| 00492ce0 | `DSoundManager(HWND theHWnd, bool haveFMod)` ret 8 | 1.2 ctor / `SDLSoundManager()` | `DirectSoundCreate` via gDSoundDLL; with FMod: LoadFModDLL, FSOUND_SetHWND, SetBufferSize(200), FSOUND_Init(44100,64,2). Then **SetCooperativeLevel(hwnd, DSSCL_NORMAL)** only; 1.2 tries DSSCL_PRIORITY and sets a 44.1 kHz/16-bit/stereo primary buffer. |
| 00492e00 | `~DSoundManager()` body | | ReleaseChannels, ReleaseSounds, FSOUND_Close+FreeFModDLL if FMod, mDirectSound->Release |
| 00491da0 | `int VolumeToDB(double v)` thiscall ret 8 | port `VolumeToGain` | `(int)((log10(1+9v)-1)*2333)` centi-dB; `< -2000` gives -10000 (consts 00506a68=9, 004f9ae8=1, 00506a60=2333) |
| 00491ea0 | `void ReleaseFreeChannels()` | `ReleaseFinishedChannels` | deletes channels whose IsReleased() is true |
| 00492e90 | `int FindFreeChannel()` | `FindFreeChannel` | sweep every >1000 ms (GetTickCount), then first NULL or released slot; -1 if all 32 busy |
| 004954f0 | LoadSound (slot 2) | | see 1.3 |
| 00495110 | `bool LoadWAVSound(unsigned id, const std::string& file)` ret 8 | 1.2 same | RIFF/WAVE, PCM only (format tag 1); `dep ` chunk: string + FILETIME must equal the source file's last-write time, else fail |
| 00493470 | `bool LoadOGGSound(unsigned id, const std::string&)` | 1.2 same | Tremor: ov_open 004a4200, ov_info 004a3db0, ov_pcm_total 004a39e0, ov_read 004a3df0, ov_clear 004a38e0 (inferred names). 16-bit output; true only if all bytes were decoded |
| 00493630 | `bool LoadAUSound(unsigned id, const std::string&)` | 1.2 same; port `DecodeAU` | `.snd`, big-endian header; encoding 1 = 8-bit mu-law to 16-bit with the framework's own segment table, 2 = 8-bit linear; else fail |
| 00493280 | `bool LoadFModSound(unsigned id, const std::string&)` | 1.2 same | only if mHaveFMod (never) |
| 00493ac0 | `bool WriteWAV(unsigned id, const std::string& file, const std::string& depFile)` ret 0xc | 1.2 same | RIFF size = data+36 (ignores the dep chunk, as 1.2); writes `fmt `, `dep ` (len, path, FILETIME), `data` |
| 00493a50 | `bool GetTheFileTime(const std::string&, FILETIME*)` | 1.2 same | **bug**: tests CreateFile's result against NULL, not INVALID_HANDLE_VALUE (a missing file yields time 0) |

##### 1.3 How samples are loaded and named (answer to Q1)

`DSoundManager::LoadSound(id, name)` (004954f0):
1. `id >= 256` returns false. If `mDirectSound == NULL` it returns **true** without loading (no device is not an error).
   It does **not** release a buffer already in that slot (1.2 calls ReleaseSound and stores the file name).
2. Cache name: the directory part of `name` (up to the last `/` or `\`, whichever is later) + `"cached_"` + base name +
   `".wav"`, e.g. `sounds\buttonclick` gives `sounds\cached_buttonclick.wav`.
3. Try in order, stop at the first success:
   `cached_<name>.wav` (LoadWAVSound, valid only if its `dep ` timestamp matches the source file) →
   `<name>.wav` → [`<name>.mp3` via FMOD, only if mHaveFMod: never] → `<name>.ogg` (Tremor) → `<name>.au`.
4. After decoding an `.mp3`/`.ogg`/`.au` it calls WriteWAV(id, cachedName, sourceName): the decoded PCM is written as
   `cached_<name>.wav` with a `dep ` chunk naming the source and its last-write FILETIME (strings 005073b4 RIFF,
   005073ac WAVE, 005073a4 `fmt `, 0050739c `dep `, 00507394 data; extensions 005073d0 .wav, 005073c8 .mp3,
   005073c0 .ogg, 005073bc .au). So the cache is a decoded copy, rebuilt whenever the .ogg/.au changes; the write fails
   silently on a read-only folder. The installed game folder has a `cached_*.wav` for every sound (e.g.
   `cached_blank.wav`: 8012 Hz mono 16-bit, dep `sounds\blank.au`; `cached_chomp.wav`: 44.1 kHz stereo, dep
   `sounds\chomp.ogg`), so a normal run loads the cached WAVs and never decodes Ogg.
5. All loaders create a static secondary buffer with flags 0x80ea (CTRLFREQUENCY|CTRLPAN|CTRLVOLUME|STATIC|
   LOCSOFTWARE|GLOBALFOCUS) at the file's own rate/format.

Sound ids are fixed by the game: `BookWormApp::Init` (slot 16, 00414a10) loads ids 0..46 at 00414dbd as
`mSoundManager->LoadSound(i, GetString("SoundPath", "sounds\\") + gSoundNames[i])` (vtable +0x08; property lookup
FUN_00480f70; the path string is cached in the static std::string 00534868; name table 0052e668). If any LoadSound
returns false (or mShutdown +0x305 is set) it calls `Shutdown()` (slot 33, 0047cbf0) and returns: **a missing sound
file quits the game silently**. The loading thread (`BookWormApp` slot 17, 0041a8a0) later calls
`mSoundManager->Initialized()` (vtable +0x04, 0041ab31) only to add 47 to the progress counter when there is no device.

Sound ids (index = id, file `sounds\<name>.ogg`, except 3 = `blank.au`; 24 is `buttonclick` again):
0 buttonclick, 1 gameovertone, 2 bad, 3 blank, 4-12 up1..up9, 13 nextselect, 14 wordcomplete, 15 bonuscomplete,
16 buzzer, 17 piece_hit, 18 chomp, 19 flametile, 20 warning, 21 tileburn, 22 appear2, 23 goldtile, 24 buttonclick,
25 redtilecleared, 26 spinsuperbonus, 27 bonusword, 28 screenburngameover, 29 lex_astonishing, 30 lex_awesome,
31 lex_burp, 32 lex_careful, 33 lex_danger, 34 lex_excellent, 35 lex_fantastic, 36 lex_getready, 37 lex_go,
38 lex_incredible, 39 lex_letsgo, 40 lex_levelup, 41 lex_lookout, 42 lex_scramble, 43 lex_unbelievable,
44 lex_verygood, 45 lex_watchout, 46 lex_welcome. Game folder: `sounds/` = 46 `.ogg` + `blank.au` + 47 `cached_*.wav`;
`music/` = `Bookworm1.mo3` only.

How the game plays them: only through `SexyApp::PlaySample(int id)` (slot 41, +0xa4, 0047db50 =
`GetSoundInstance(id)->Play(false, true)`), 56 game call sites, e.g. 004021d4 (46 lex_welcome), 00409d99 (15),
0040abf6 (27 bonusword), 0040b786 (19), 0040c101 (14), 0040fc05 (16 buzzer), 0042eb75 (23 goldtile, Options sfx
slider), many `PlaySample(0)` button clicks (00401457, 0041df93, 0042a939...). Computed ids at 0040d5b0 and 00432f4a.
The game never calls `PlaySample(id, pan)` (slot 40, 0047db80), never touches a SoundInstance, never uses
AdjustPitch, SetBaseVolume/Pan, looping sounds or GetMasterVolume/SetMasterVolume.

##### 1.4 Volumes, pan, pitch (answer to Q2)

- Sfx volume: `SexyApp::SetSfxVolume` (slot 48, 0047dcf0) stores +0x78 and calls `mSoundManager->SetVolume(muted ? 0 : v)`
  (slot 5) → mMasterVolume (+0x1488) → every channel's RehupVolume (00492180):
  `buffer->SetVolume(VolumeToDB(mBaseVolume * mVolume * mMasterVolume))` with the curve above. Port: identical curve
  (`SDLSoundManager::VolumeToGain` converts the same centi-dB to linear gain).
- Master volume = **the Windows wave-out mixer**, not a software gain: GetMasterVolume/SetMasterVolume open mixer 0,
  find line MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT (0x1008), control MIXERCONTROL_CONTROLTYPE_VOLUME (0x50030001), and
  read/write `dwValue` = v * 0xFFFF (read: dwValue / 65535, const 1.5259e-05). Reached only through
  `SexyApp::GetMasterVolume` (slot 42, 0047dd50) / `SetMasterVolume` (slot 46, 0047dd60, which also stores v as
  mSfxVolume); no game code calls either. BassMusicInterface's ctor reads the same control before BASS_Init and writes
  it back afterwards (protects the system volume from BASS). Port: SetMasterVolume = SetVolume; nothing to do.
- Pan (RehupPan 004921b0): `buffer->SetPan(mBasePan + mPan)`, DirectSound hundredths of a dB of attenuation of the
  opposite channel (-10000..10000), no clamping. Port: same rule (with clamping). Unused by the game.
- AdjustPitch (004922f0): `freq = mDefaultFrequency * 1.0594630943592953^steps` (2^(1/12)), clamped to [100, 100000] Hz
  (DSBFREQUENCY_MIN/MAX, consts 004f9d08, 00506ab8), `SetFrequency`. Port: ratio 2^(steps/12) without the clamp. Unused.
- DSoundInstance slot 11 (004923d0) is `double GetVolume()` = mVolume (+0x28), as in 1.2.

---------------------------------------------------------------------------------------------------------------

#### 2. SoundInstance / DSoundInstance

Bases: DSoundInstance : SoundInstance (vtables 00506a84 / 00506a30, 12 slots, same order as 1.2 and the port).
All DSoundInstance methods are byte-identical to Big Money's 1.2 ones.

| slot (+off) | address | method | 1.2 slot | port | notes |
|---|---|---|---|---|---|
| 0 (+00) | 00493070 | `~DSoundInstance()` scalar deleting | 0 | `~SDLSoundInstance` | body 00492130 releases mSoundBuffer |
| 1 (+04) | 004921d0 | `void Release()` = Stop(); mReleased = true | 1 | `Release` | |
| 2 (+08) | 00492220 | `void SetBaseVolume(double)` | 2 | same | RehupVolume |
| 3 (+0c) | 00492240 | `void SetBasePan(int)` | 3 | same | RehupPan |
| 4 (+10) | 004922f0 | `void AdjustPitch(double theNumSteps)` | 4 | same | see 1.4 |
| 5 (+14) | 004921e0 | `void SetVolume(double)` | 5 | same | |
| 6 (+18) | 00492200 | `void SetPan(int)` | 6 | same | |
| 7 (+1c) | 00492260 | `bool Play(bool looping, bool autoRelease)` ret 8 | 7 | same | Stop(), mHasPlayed=1, mAutoRelease=autoRelease, buffer->Play(0,0,looping?DSBPLAY_LOOPING:0) |
| 8 (+20) | 004922c0 | `void Stop()` | 8 | same | buffer Stop + SetCurrentPosition(0), mAutoRelease=0 (only when a buffer exists) |
| 9 (+24) | 00492370 | `bool IsPlaying()` | 9 | same | mHasPlayed && GetStatus & DSBSTATUS_PLAYING |
| 10 (+28) | 004923a0 | `bool IsReleased()` | 10 | same | auto-releases a finished autoRelease instance |
| 11 (+2c) | 004923d0 | `double GetVolume()` | 11 | same | |

Fields (0x38, all verified in the ctor 00492f50):

| off | type | name | meaning | port field |
|---|---|---|---|---|
| +0x4 | DSoundManager* | mSoundManagerP | | `mManager` |
| +0x8 | LPDIRECTSOUNDBUFFER | mSourceSoundBuffer | the manager's source buffer (NULL without a device) | `mData` |
| +0xc | LPDIRECTSOUNDBUFFER | mSoundBuffer | DuplicateSoundBuffer of the source (error: MessageBox "Hey" + exit) | `mVoice` |
| +0x10 | bool | mAutoRelease | | `mAutoRelease` |
| +0x11 | bool | mHasPlayed | | `mHasPlayed` |
| +0x12 | bool | mReleased | | `mReleased` |
| +0x14 | int | mBasePan | 0 | `mBasePan` |
| +0x18 | double | mBaseVolume | 1.0 | `mBaseVolume` |
| +0x20 | int | mPan | 0 | `mPan` |
| +0x28 | double | mVolume | 1.0 | `mVolume` |
| +0x30 | DWORD | mDefaultFrequency | 44100, then GetFrequency of the duplicate | (`mPitch` ratio instead) |

Direct functions: ctor `DSoundInstance(DSoundManager*, LPDIRECTSOUNDBUFFER)` 00492f50 (ret 8); RehupVolume 00492180;
RehupPan 004921b0; body dtor 00492130. GetSoundInstance (00493d90): `id > 256` returns NULL (1.2's off-by-one),
FindFreeChannel, `new DSoundInstance(this, NULL)` when there is no device (a silent instance), NULL when the id has no
buffer, then SetBasePan(mBasePans[id]) and SetBaseVolume(mBaseVolumes[id]).

---------------------------------------------------------------------------------------------------------------

#### 3. MusicInterface / BassMusicInterface / FModMusicInterface

Bases: BassMusicInterface : MusicInterface (005073f4, 15 slots), FModMusicInterface : MusicInterface (00507434, 17),
MusicInterface 005072d4 (15). 1.2 has 20/20/22, the port's MusicInterface 20.

##### Vtable

The base MusicInterface slots are COMDAT-folded empty stubs; their `ret` sizes prove the signatures.

| slot (+off) | MusicInterface | BassMusicInterface | FModMusicInterface | method | 1.2 slot | port method |
|---|---|---|---|---|---|---|
| 0 (+00) | 00493100 | 004969d0 | 00496aa0 | `~X()` scalar deleting | 0 | dtor |
| 1 (+04) | 00492cb0 (returns false, ret 8) | 00495f50 | 00496610 | `bool LoadMusic(int theSongId, const std::string& theFileName)` | 1 | `LoadMusic` |
| 2 (+08) | 0041dfa0 (ret 0xc) | 00494450 | 00494860 | `void PlayMusic(int theSongId, int theOffset, bool noLoop)` | 2 | `PlayMusic` |
| 3 (+0c) | 00401570 (ret 4) | 00494540 | 004949f0 | `void StopMusic(int)` | 3 | `StopMusic` |
| 4 (+10) | 00401570 | 004945a0 | 00494950 | `void PauseMusic(int)` | 4 | `PauseMusic` |
| 5 (+14) | 00401570 | 004945e0 | 004949a0 | `void ResumeMusic(int)` | 5 | `ResumeMusic` |
| 6 (+18) | 004036d0 (ret) | 00493f60 | 00494150 | `void StopAllMusic()` | 6 | `StopAllMusic` |
| 7 (+1c) | 00492cc0 (ret 0x14) | 00494620 | 00494a50 | `void FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop)` | 11 | `FadeIn` |
| 8 (+20) | 00492cd0 (ret 0x10) | 00494710 | 00494b60 | `void FadeOut(int theSongId, bool stopSong, double theSpeed)` | 12 | `FadeOut` |
| 9 (+24) | 0041dfa0 (ret 0xc) | 00493fd0 | 004941d0 | `void FadeOutAll(bool stopSong, double theSpeed)` | 13 | `FadeOutAll` |
| 10 (+28) | 0041dfa0 | 00494760 | 00494ba0 | `void SetSongVolume(int, double)` | 14 | `SetSongVolume` |
| 11 (+2c) | 0047d7f0 (returns false, ret 4) | 004947c0 | 00494c20 | `bool IsPlaying(int)` | 16 | `IsPlaying` |
| 12 (+30) | 004220f0 (ret 8) | 00492790 | 004941f0 | `void SetVolume(double)` | 17 | `SetVolume` |
| 13 (+34) | 0041dfa0 | 00494810 | 0041dfa0 (base) | `void SetMusicAmplify(int, double)` | 18 | `SetMusicAmplify` (no-op) |
| 14 (+38) | 004036d0 | 00494040 | 004942b0 | `void Update()` | 19 | `Update` |
| 15 (+3c) | - | - | 004962b0 | `bool LoadSample(int, const std::string& intro, const std::string& repeat, bool repeat)` ret 0x10 | 20 | not in port |
| 16 (+40) | - | - | 004961b0 | `bool LoadSample(int, const std::string&, bool repeat)` ret 0xc | 21 | not in port |
| - | | absent | | `UnloadMusic`, `UnloadAllMusic`, `PauseAllMusic`, `ResumeAllMusic`, `SetSongMaxVolume` | 7-10, 15 | exist in port |

##### Fields

BassMusicInterface (0x14; ctor 00496810):

| off | type | name | meaning | port field | |
|---|---|---|---|---|---|
| +0x4 | std::map<int, BassMusicInfo> | mMusicMap | allocator +4, head +8, size +0xc | `mSongs` (std::map<int, SongInfo>) | verified |
| +0x10 | int | mMaxMusicVolume | 40; used only on the BASS 1.x path of SetVolume | `mMaxMusicVolume` (40) | verified |

BassMusicInfo (0x20, ctor 00492770; map node 0x40: left +0, parent +4, right +8, key +0x10, value +0x18, color +0x38,
isnil +0x39):

| off in value (node) | type | name | port `SongInfo` | |
|---|---|---|---|---|
| +0x0 (+0x18) | HMUSIC | mHMusic | `mHandle` | verified |
| +0x4 (+0x1c) | HSTREAM | mHStream | (Mixer song) | verified |
| +0x8 (+0x20) | double | mVolume (0) | `mVolume` | verified |
| +0x10 (+0x28) | double | mVolumeAdd (0) | `mVolumeAdd` | verified |
| +0x18 (+0x30) | bool | mStopOnFade | `mStopOnFade` | verified |
| - | | (no mVolumeCap: the cap is the constant 1.0) | `mVolumeCap` (1.0) | verified |

FModMusicInterface (0x20; ctor 00496a00): +0x4 std::map<int, FModMusicInfo> mMusicMap, +0x10 double mMasterVolume
(1.0), +0x18 int mMaxMusicVolume (100), +0x1c int mMaxSampleVolume (100). FModMusicInfo (0x20, ctor 00492c70):
+0 FSOUND_SAMPLE* mHSample, +4 FMUSIC_MODULE* mHMusic, +8 double mVolume, +0x10 double mVolumeAdd, +0x18 bool
mStopOnFade, +0x19 bool mRepeats (no mVolumeCap). The sample channel used by FSOUND calls is the song id.

##### Direct functions

| address | name | notes |
|---|---|---|
| 00492c90 | `MusicInterface::MusicInterface()` | sets vftable |
| 00492ca0 | `MusicInterface::~MusicInterface()` body | |
| 00496810 | `BassMusicInterface(HWND)` ret 4 | LoadBassDLL; saves wave-out mixer volume; BASS 2: `BASS_Init(1, 44100, 0, hwnd, NULL)` + `BASS_SetConfig(BASS_CONFIG_BUFFER, 2000)`; BASS 1: `BASS_Init(-1, 44100, 0, hwnd)`; restores mixer volume; `BASS_Start`; mMaxMusicVolume = 40 |
| 004966c0 | `~BassMusicInterface()` body | BASS_Stop, BASS_Free, FreeBassDLL, clears the map (no BASS_MusicFree: never imported) |
| 00496a00 | `FModMusicInterface(HWND)` ret 4 | LoadFModDLL; volumes as above |
| 00496790 | `~FModMusicInterface()` body | FreeFModDLL, clears the map |
| 004958a0 | `FSOUND_SAMPLE* FModMusicInterface::LoadFMODSample(const std::string&)` | `cached_<name>.wav`, else `<name>.ogg` and writes the cached WAV (no dep chunk), as 1.2 |
| 00492440 | `BASS_INSTANCE(const char* dllName)` ret 4 | see 3.2 |
| 004926d0 / 00492730 | `LoadBassDLL()` / `FreeBassDLL()` | refcounted gBass; "Can't find bass.dll." + exit |
| 004923e0 | `CheckBassFunction(unsigned, const char*)` | "%s function not found in bass.dll" + exit |
| 00492830, 00492bd0, 00492c30, 004927d0 | FMOD_INSTANCE ctor, LoadFModDLL, FreeFModDLL, CheckFModFunction | the 27 `_FSOUND_*`/`_FMUSIC_*` imports are exactly 1.2's list |
| std::map helpers | see the jsonl | find 004943e0, insert 00495d10, _Insert 00494c70, erase(it) 00494e30, erase(first,last) 00495dd0 (Bass) / 00495e90 (FMod), _Erase 00493ee0 / 00493f20, _Buynode 00493e90, _Lrotate 00493130, _Rrotate 00493190, iterator-- 004931f0, ~map 00496680 / 00496750. Both maps have 0x40-byte nodes and share the folded find/insert/_Insert/erase code. |

##### 3.1 BASS version and imports

The game ships `bass.dll` **2.0.0.20** (Un4seen, 1999-2004). BASS_INSTANCE (0x64) = 1.2's struct minus `BASS_MusicFree`:
+4 mModule, +8 bool mVersion2, +0xc BASS_Init, +0x10 BASS_Init2, +0x14 Free, +0x18 Stop, +0x1c Start, +0x20
SetGlobalVolumes (optional), +0x24 SetVolume (optional; one of the two is required), +0x28 MusicSetAmplify, +0x2c
SetConfig (optional), +0x30 ChannelStop, +0x34 ChannelPause, +0x38 ChannelResume, +0x3c ChannelSetAttributes,
**+0x40 BASS_ChannelSetPosition: field present but never filled** (no GetProcAddress, no name string; the object comes
from an uninitialised `operator new`), +0x44 ChannelIsActive, +0x48 MusicLoad, +0x4c MusicLoad2, +0x50 MusicPlay,
+0x54 MusicPlayEx, +0x58 StreamCreateFile, +0x5c StreamFree, +0x60 StreamPlay. `mVersion2 = (BASS_SetConfig != NULL)`,
true with 2.0; then Init/MusicLoad move to the Init2/MusicLoad2 slots (2.0 signatures). Calling a stream with an
offset > 0 would jump through the garbage +0x40 pointer; BookWorm only plays one MO3, so it never happens.

##### 3.2 Music semantics (answer to Q3)

- **LoadMusic** (00495f50): lower-cased extension (Sexy::StringToLower 00469f70); `wav`/`ogg`/`mp3` →
  `BASS_StreamCreateFile(FALSE, name, 0, 0, 0)`; anything else (`.mo3`, `.it`, `.mod`...) →
  BASS 2: `BASS_MusicLoad(FALSE, name, 0, 0, BASS_MUSIC_LOOP|BASS_MUSIC_RAMP = 0x204, 0)`; BASS 1: flags 4. Inserted
  with `map::insert`, so a second LoadMusic for the same id is ignored (port: replaces it; BookWorm never reloads).
- **PlayMusic(id, offset, noLoop)** (00494450): mVolume = 1.0, mVolumeAdd = 0, mStopOnFade = noLoop;
  `BASS_ChannelSetAttributes(h, -1, 100, -101)`; module: `BASS_MusicPlayEx(h, offset, (noLoop ? 0 : BASS_MUSIC_LOOP)
  | BASS_MUSIC_RAMP, TRUE)` (BASS 1: no RAMP). The offset goes to BASS unchanged: BASS 2.0's `pos` is
  MAKELONG(order, row), so BookWorm's offsets are **order numbers, row 0**. `reset = TRUE`. Unlike 1.2 there is no
  BASS_ChannelStop first (MusicPlayEx with reset restarts anyway). Stream: `BASS_StreamPlay(h, offset != -1, flags)`
  and, for offset > 0, the unloaded ChannelSetPosition.
- **FadeIn(id, offset, speed, noLoop)** (00494620): mVolumeAdd = speed, mStopOnFade = noLoop;
  `ChannelSetAttributes(h, -1, (int)(mVolume*100), -101)` (starts from the current volume: 0 for a song that never
  played or was stopped/faded out); offset == -1 → `BASS_MusicPlay(h)` (continue from the current position with the
  current flags); else `BASS_MusicPlayEx(h, offset, noLoop ? 0 : BASS_MUSIC_LOOP, TRUE)`, **without BASS_MUSIC_RAMP**
  even on BASS 2 (1.2 adds it). No ChannelStop first (1.2 has one).
- **FadeOut(id, stopSong, speed)** (00494710): if mVolume != 0, mVolumeAdd = -speed; mStopOnFade = stopSong always.
  With stopSong = false the song keeps playing at volume 0 and still counts as playing.
- **FadeOutAll(stop, speed)** (00493fd0): every song. **StopMusic** / **StopAllMusic**: mVolume = 0 + BASS_ChannelStop.
  **PauseMusic**/**ResumeMusic**: BASS_ChannelPause / BASS_ChannelResume.
- **Update** (00494040), per song with mVolumeAdd != 0: mVolume += mVolumeAdd; above 1.0 → 1.0 and stop fading; below
  0 → 0, stop fading, and BASS_ChannelStop if mStopOnFade; then `ChannelSetAttributes(h, -1, (int)(mVolume*100), -101)`
  (volume quantised to whole percent by truncation).
- **SetSongVolume** (00494760): mVolume = v and ChannelSetAttributes. **IsPlaying** (004947c0):
  `BASS_ChannelIsActive(h) == BASS_ACTIVE_PLAYING` (paused or stopped = false). **SetMusicAmplify** (00494810):
  `BASS_MusicSetAmplify(h, (int)(amp*100))` (BASS default 50). The game calls neither.
- **SetVolume(v)** (00492790): BASS 2: `BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC = 6, (int)(v*100))` only (1.2 also sets
  GVOL_STREAM 5); BASS 1: `BASS_SetGlobalVolumes(v*40, v*40, v*40)`. So with BookWorm's BASS 2.0, mMaxMusicVolume = 40
  is unused and the music master gain is v (0..1), not 0.4 v.
- Fade timing: `BookWormApp::UpdateFrames` (slot 7, 004100f0) calls `SexyApp::UpdateFrames` (0047cce0, which ends
  with `mMusicInterface->Update()`) **and then calls `mMusicInterface->Update()` again**, so every fade moves 2 x speed per
  update tick: speed 0.005 = 1.0 s for a full fade at 100 updates/s, 0.008 = about 0.63 s, 0.01 = 0.5 s, 1.0 = instant.
- FModMusicInterface (never used): PlayMusic sets FMUSIC master volume `mMasterVolume*mVolume*100`, looping
  `noLoop ? FSOUND_LOOP_OFF : FSOUND_LOOP_NORMAL`, PlaySong, then **FMUSIC_SetOrder(offset)** (FMOD's order
  equivalent); samples play on channel = song id. FadeOutAll only touches the first song (1.2's `if` bug). IsPlaying
  uses FMUSIC_IsPlaying even for samples. Update clamps at 1.0 (no cap). Otherwise identical to 1.2.

Songs: `BookWormApp` loading thread (slot 17, 0041a8a0) loads **the same file five times**,
`LoadMusic(i, "music\\Bookworm1.mo3")` for i = 0..4 (0041a919, string 004fbf64), giving five independent BASS
channels of one MO3 (220 KB, MO3 version 3), then `PlayMusic(0, 27, false)` (0041a963). Order numbers per song
(meaning inferred from the callers; app+0x1254 is the game mode, 0 = classic, 1 = action, see section 4 (SexyApp)):

| song | starts at order | used for (inferred) | call sites |
|---|---|---|---|
| 0 | 27 | title/menu theme | PlayMusic(0,27,false) 0041a963; FadeIn(0,27,0.005,false) 0041100a (FUN_00410f00, back to title, after FadeOut(1..4,true,0.01) 00410f49); FadeOut(0,false,0.008) 00421d91 + FadeIn(0,-1,0.008,false) 00426e6d (hall of fame open/close; song id and noLoop come from ebx, 0 at that point, inferred) |
| 1 | 0 | classic-mode theme | FadeIn(1,0,0.005,false) 00410dfc (FUN_00410d10, game start: FadeOut(0,3,4,true,0.005) 00410d5f); FadeOut(1,true,0.005) 00410e0f (action mode); FadeIn(1,-1,0.008,false) 0040d22e |
| 2 | 37 | action-mode theme | FadeIn(2,37,0.005,false) 00410db3; FadeOut(2,true,0.005) 00410dc6 (classic mode); FadeIn(2,-1,0.008,false) 0040d22e |
| 3 | 59 / 61 (one-shot), 54 | game-over jingle; hall-of-fame theme | StopMusic(2 or 1) + StopMusic(4) + FadeIn(3, 59 action / 61 classic, 1.0, **noLoop=true**) 00403921..00403989 (FUN_004038c0); FadeIn(3,54,0.008,false) 00421daf; FadeOut(3,true,0.008) 00426e51 |
| 4 | 65 action / 51 classic | danger theme | IsPlaying(4) 004097b6/0040d130/0040d1cf; FadeOut(mode song, false, 0.008) + FadeIn(4, 65/51, 0.008, false) 004097e1..0040982c, 0040d15e..0040d1ba; FadeOut(4,true,0.008) 0040d1ef |

Other game calls on mMusicInterface: FadeOut(0x1b = 27, true, 0.0075) at 00410d7e (song 27 does not exist: a no-op,
probably meant for song 0); `FUN_004184d0(bool pause)` is the game's own PauseAll/ResumeAll for songs 0..4
(IsPlaying 004184e9 → remember the id in a vector at app+0x12fc and PauseMusic 0041850b; resume: ResumeMusic
0041854f for each remembered id), called from 00405d60, 00409de0, 00419000, 00419040. `BookWormApp::SetMusicVolume`
(slot 47, 00410110) calls the base and then `mMusicInterface->SetVolume(muted ? 0 : mMusicVolume)` (00410151) again.
`SexyApp::Shutdown` (slot 33) calls StopAllMusic. No game call uses LoadMusic outside 0041a919, SetSongVolume,
SetMusicAmplify, FadeOutAll or StopAllMusic.

---------------------------------------------------------------------------------------------------------------

#### 4. Behaviour differences and what the port must do

1. **Slot remapping** (all classes): read game calls through `mSoundManager` (+0x4e0) and `mMusicInterface` (+0x33c)
   with the slot tables above, not 1.2's: SoundManager +0x04 Initialized, +0x08 LoadSound(id, name); MusicInterface
   +0x04 LoadMusic, +0x08 PlayMusic, +0x0c StopMusic, +0x10 PauseMusic, +0x14 ResumeMusic, +0x1c FadeIn, +0x20 FadeOut,
   +0x2c IsPlaying, +0x30 SetVolume, +0x38 Update. Port: call the same-named methods. Nothing else.
2. **Music master volume** (00492790): BookWorm's BASS 2.0 path sets the global MOD volume to v (BASS_CONFIG_GVOL_MUSIC
   v*100, linear), the port's `SDLMusicInterface::SetVolume` uses v * mMaxMusicVolume / 100 = 0.4 v (the BASS 1.x
   rule). Port: game-side, set `mMaxMusicVolume = 100` on the SDLMusicInterface the BookWorm app creates (override
   CreateMusicInterface), then calibrate the libopenmpt render level against BASS 2.0 (default amplify 50, linear
   interpolation) by ear or with a capture: the music/sfx balance depends on it (open question 1).
3. **Double music Update** (004100f0): every fade runs at twice the nominal speed. Port: translate the override
   literally (base UpdateFrames, then `mMusicInterface->Update()`); do not "fix" it.
4. **MO3 order offsets**: offsets 0, 27, 37, 51, 54, 59, 61, 65 are order numbers (row 0) passed to
   `BASS_MusicPlayEx(h, MAKELONG(order,row), flags, TRUE)`; the port maps them to `Mixer::PlaySong(order, row)` →
   `openmpt_module_set_position_order_row` (platform/openmpt, in progress). Five songs = five independent module
   instances of one file; the port's Mixer already creates one Song per LoadMusic. Nothing else, but check loop/stop
   semantics (open question 2): BASS follows backward position jumps (Bxx) even without BASS_MUSIC_LOOP (no
   BASS_MUSIC_STOPBACK) and stops a non-looping song only at the end of the order list, while libopenmpt with
   repeat count 0 stops at its detected song end; song 3's one-shot jingles (orders 59/61, noLoop) are where this shows.
5. **FadeIn/PlayMusic details** the port already matches: FadeIn starts from the song's current volume (0 if never
   played), offset -1 continues from the current position (port: `PlaySong(..., resetPosition=false)`), FadeOut with
   stopSong = false leaves the song playing silently and IsPlaying true (port: `IsSongPlaying` = playing && !paused),
   PlayMusic sets volume 1.0, Update clamps at 1.0 (port's mVolumeCap stays 1.0: SetSongMaxVolume does not exist here).
   Minor: BASS volumes are whole percents (truncated), the port's are continuous: nothing.
6. **Volume ramping** (minor): BookWorm loads with BASS_MUSIC_RAMP and PlayMusic keeps it, but FadeIn(offset != -1)
   replays with flags = LOOP only, i.e. without ramping (inferred from BASS 2.0's MusicPlayEx flag semantics). The
   port's OpenMPTPlayer comment assumes "no ramping unless asked"; BookWorm does ask for the songs started by PlayMusic
   (song 0). Port: optional; libopenmpt's volume-ramping render parameter could follow the flag.
7. **Sound lookup order**: original `cached_<name>.wav` (dep timestamp must match) → `.wav` → `.ogg` → `.au`; port
   `.ogg` → `.wav` → `.au` → `.mp3`, no cache. Same result for the shipped files (46 .ogg + blank.au); differs only if a
   mod ships both `<name>.wav` and `<name>.ogg` (original: .wav wins). The cached WAVs are Tremor decodes, the port
   decodes with stb_vorbis (Tremor fallback): a few LSBs at most. Port: nothing required; never write `cached_*` files
   (the original's write fails silently on read-only installs); optionally put `.wav` first for BookWorm.
8. **Load failure policy**: BookWormApp::Init (00414dbd) shuts down silently when LoadSound returns false. The original
   returns true when there is no sound device, false only for missing/undecodable files; the port returns false for
   missing files even with no device and true otherwise. Port: keep the check (it is game code); for headless/silent
   runs make sure the files resolve, or treat no-device like the original (LoadSound true). Small.
9. **Master volume** = Windows wave-out mixer in the original (GetMasterVolume/SetMasterVolume, plus the save/restore
   around BASS_Init); unused by BookWorm. Port: nothing (its SetMasterVolume is the sfx volume).
10. **DirectSound setup**: DSSCL_NORMAL only, no primary-buffer format; sounds play at their file rate. Pan/pitch rules
    as 1.2 (pitch clamp 100..100000 Hz, pan not clamped) but unused by the game. Port: nothing.
11. **Removed members** (LoadSound(name), ReleaseSound, UnloadMusic, UnloadAllMusic, PauseAllMusic, ResumeAllMusic,
    SetSongMaxVolume, BassMusicInfo::mVolumeCap, BASS_MusicFree, BASS_ChannelSetPosition): the game cannot call them;
    its own PauseAll/ResumeAll lives in FUN_004184d0. Port: nothing (extra port methods are harmless).
12. **FMOD**: dead code (flags never set, no fmod.dll). Port: drop FModMusicInterface/FMOD_INSTANCE/LoadFModSound.
13. **32 channels**: PlaySample silently does nothing when 32 instances are busy (released ones are reclaimed every
    1000 ms or on demand). Port: same constants and rule; nothing.

#### Open questions (section 6)

1. Loudness: BASS 2.0 MO3 rendering (amplify 50, GVOL_MUSIC = v*100) vs libopenmpt's output level; needs a capture
   comparison to pick the BookWorm music gain (mMaxMusicVolume or a render gain).
2. Bookworm1.mo3's order list: what orders 27, 37, 0, 51, 54, 59, 61, 65 end with (Bxx loop jumps, `---`/end markers),
   to confirm that libopenmpt loops/stops like BASS 2.0 for each song (especially song 3's one-shot jingles at 59/61).
3. BASS 2.0 details taken from its documentation, not from bass.dll's code: BASS_MusicPlay = continue at the current
   position with the current flags; MusicPlayEx flags replace the channel's flags (affects only ramping).
4. Answered in section 4: the update rate is 100 Hz (mFrameTime = 10 ms, verified in the ctor and Process). app+0x1254
   is beyond SexyApp (sizeof 0x618): a BookWormApp field, for the game group to name.

## 7. Support classes and library helpers

### 7.1 Support classes

Ranges: 0x465f30-0x479c00 (whole), 0x496ad0-0x499300 (BigInt + STL helpers), plus the DDInterface /
D3DInterface / D3DTester functions SexyApp and the game touch. Symbols: app_symbols.jsonl (404 functions,
11 data labels; every function Ghidra knows in the two ranges is named, STL and BigInt internals generically).
"verified" = read in the decompilation/disassembly; "inferred" = from SAF 1.2 source order, call graph or shape.

Big picture:

| Area | Verdict for the port |
|---|---|
| Common helpers (StrFormat, StringToInt, Upper, Trim, XMLDecodeString, file helpers) | 1:1 with `framework/Common.h`; no behaviour difference the game can see |
| MTRand / Sexy::Rand / SRand | same generator, **different seeding** (SAF 1.2 Knuth 69069, port 1.3 1812433253) |
| Buffer | 1:1 with `framework/Buffer.h` (same bit layout, same web-string format) |
| PropertiesParser / XMLParser | 1.2 code; only `ParsePropertiesBuffer` is linked (partner.xml after a signature check) |
| ResourceManager | constructed in the SexyApp ctor (app+0x60c) and deleted in the dtor; nothing else ever touches it: **linked but unused** |
| HTTPTransfer | used by game code: FameWidget online scores. Port: game-side stub |
| InternetManager | app+0xd8; game uses it for "check for updates". Port: drop / stub |
| BetaSupport, SEHCatcher, PerfTimer/SexyPerf, ModVal, DirectXErrorString, BigInt | droppable |
| DDInterface / D3DInterface / D3DTester | replaced by the port's SDL `DDInterface`; the game only does `new DDImage(app->mDDInterface)` |

##### 1. Functions the game calls directly (callees.md rows in this slice)

| addr | identification | game call sites | port equivalent | difference / note |
|---|---|---|---|---|
| 00466970 | `Buffer::GetDataLen()` ((mDataBitSize+7)/8) | 00428520 (FameWidget ctor: `app+0x1160 (the Board, inferred) ->+0x160 game log .GetDataLen() > 0`) | `Buffer::GetDataLen` | none |
| 00466eb0 | `std::_Tree<28-byte value>::_Max(node)` (isnil +0x29; set<std::string> shape) | 00412870 (a game `_Tree::erase(iterator)` instance) | std::set | STL, nothing to port |
| 00466f50 | `std::_Tree<28-byte value>::_Min(node)` | 00412870 | std::set | STL |
| 00467030 | `MTRand::MTRand(unsigned long seed)` (SRand(seed); returns this) | 0040ae90 (Board ctor, Board+0x6c), 0040fe80 (WinMain, global MTRand* 0x5347d0), 004201c0 (EffectMgr+8), 00421070 (EmberListMgr+8); always `new MTRand(Sexy::Rand())` | `MTRand(ulong)` | **seeding differs**, see 5.1 |
| 00467060 | `MTRand::Next()` (a `jmp` to NextNoAssert 00466640; assert compiled out) | 00403d10 00404150 00408030 0040b540 0040ba80 0040bd40 0040ed70 00421150 (each on 0x5347d0, Board+0x6c or EffectMgr/EmberListMgr+8) | `MTRand::Next()` | identical (31-bit result); sites use `Next() % n` (div; one idiv at 00403d21, same values since result < 2^31) |
| 00467070 | `Sexy::Rand()` = gMTRand(0056add8).Next() | 0040ae90 0040fe80 004201c0 00421070 (only to seed the `new MTRand`s) | `Sexy::Rand()` | same generator, seeding see 5.1 |
| 004673d0 | `MTRand::SRand(const std::string&)` (624 words if size == 0x9c0 else SRand(4357); mti untouched) | 00408ab0 (load classic.dat/action.dat), 00420b70, 00420e70 (effect managers' load) | `MTRand::SRand(const std::string&)` | identical incl. "mti kept" |
| 004679a0 | `std::_Tree<28-byte value>::_Buynode()` head node (0x2c bytes) | 0041a0f0 (BookWormApp ctor: two set-like members at +0x1260 / +0x127c) | std::set ctor | STL |
| 00467fc0 | `HTTPTransfer::HTTPTransfer()` | 00428520 (FameWidget ctor, member at FameWidget+0xd8) | not in port | game-side stub, see 4.6 |
| 00468070 | `HTTPTransfer::~HTTPTransfer()` (Abort, wait for thread) | 004270d0 (FameWidget dtor) | not in port | stub |
| 00468620 | folded `return *(this+8)` (4 bytes) | 40 game functions (68 sites overall), e.g. 00402430 00403730 0041ecb0 00428520 00431e80 00436c40 | `Image::GetHeight()` at every game site | see 4.10 |
| 00469b50 | `MTRand::Serialize()` (0x9c0-byte string of mt[], no mti) | 00406c40 (save classic.dat/action.dat), 0041faa0, 0041ff80 (effect managers' save; fwrite raw) | `MTRand::Serialize()` | identical |
| 0046a2e0 | `Sexy::Upper(const std::string&)` (std::transform toupper) | 0042b160, 00431080 | `Sexy::Upper` | identical |
| 0046a3d0 | `Buffer::~Buffer()` body (sets vftable 00503544, frees mData) | 004062e0 (Board dtor) | `Buffer::~Buffer` | none |
| 0046a400 | `Buffer::ToWebString()` | 00406c40 (save game), 004241d0 (score submit "data=") | `Buffer::ToWebString` | identical format |
| 0046a520 | `Buffer::Clear()` | 00408ab0 | `Buffer::Clear` | none |
| 0046e5d0 | `Sexy::StrFormat(const char*, ...)` | 00405580 004056e0 00409970 0040abd0 0040bd40 00411cd0 00412ec0 0041a790 0041a8a0 0041d920 0041e020 00424c30 00425880 00427f30 0042c210 0042cdd0 0042dd20 0042fe90 00430e20 00432810 | `Sexy::StrFormat` | same output (see 5.4) |
| 004714a0 | `Buffer::Buffer()` | 0040ae90 (Board ctor: Board+0x160 game log) | `Buffer::Buffer` | none |
| 00472660 | `Buffer::WriteByte(uchar)` | 0040ae90 | `Buffer::WriteByte` | none |
| 004726e0 | `Buffer::WriteNumBits(int theNum, int theBits)` | 004038c0 00407a50 0040ae90 0040f5b0 004241d0 | `Buffer::WriteNumBits` | none |
| 004727f0 | `Buffer::WriteShort(short)` | 00407a50 0040ae90 | `Buffer::WriteShort` | none |
| 00472820 | `Buffer::WriteLong(long)` | 004038c0 0040ae90 0040f5b0 | `Buffer::WriteLong` | none |
| 00472860 | `Buffer::WriteString(const std::string&)` (WriteShort(len) + raw bytes) | 004241d0 | `Buffer::WriteString` | none (no UTF-8) |
| 00474800 | `HTTPTransfer::GetResultCode()` (UpdateStatus; return mResult) | 00427f30 (FameWidget::Update states 1 and 4) | not in port | stub |
| 00474810 | `HTTPTransfer::GetContent()` ("" while RESULT_NOT_COMPLETED, else mContent) | 00427f30 | not in port | stub |
| 004748a0 | `Buffer::FromWebString(const std::string&)` | 00408ab0 (load game: restores the Board game log) | `Buffer::FromWebString` | identical incl. the 1.2 lowercase-hex bug (5.3) |
| 00476ac0 | `HTTPTransfer::Get(const std::string& theURL)` | 004241d0 (two sites: GetScoresURL + "?clientver=") | not in port | stub |
| 00476b10 | `HTTPTransfer::Post(const std::string& theURL, const std::string& theParams)` | 004241d0, 00426c10 (SubmitScoreURL) | not in port | stub |
| 00479df0 | `InternetManager::DoUpdateCheck(const std::string& theURL)` (BM 00473120 exact; clears +0x4f4, Get on the +8 transfer) | 00415de0 (BookWormApp "check for updates": `http://www.popcap.com/win32updatecheck.php?prod=..&ver=..&referid=..` + UpdateCheckDialog) | not in port | drop the update check (partner.xml DontUpdate=True) |
| 00496c00 | folded `return *(this+0xc)` | 00410090 (on a `Color&`: mAlpha), 00432c90 (on app+0x1178) | none needed | both game sites discard the result: dead calls, drop |
| 00499030 | `std::fill<16-byte POD>` (Color) | 0041bc5f (catch handler of vector<Color>::_Insert_n 0041bab0) | std::vector<Color> | STL |
| 004990b0 | `std::_Uninit_fill_n<16-byte POD>` | 0041b5a0 0041bab0 0041bde0 (vector<Color> code used by Widget::SetColors / ListWidget) | std::vector<Color> | STL |
| 004990f0 | `std::_Uninit_copy<16-byte POD>` | 0041b570 0041bab0 | std::vector<Color> | STL |

Also reached from game code but outside the ranges: `InternetManager::GetUpdateURL` 0047c7a0 (returns +0x4f8
after processing the reply 0047c500) from 00417080 (dialog 10008 "update available": KillDialog x2, then
`OpenURL(mInternetManager->GetUpdateURL(), true)`), `SexyApp::Is3DAccelerationSupported/Recommended`
0047e380/0047e3a0 (D3DTester results) from OptionsDialog 0042fa50, and `new DDImage(app->mDDInterface)` at
0040111f / 0040125d (DDImage ctor 0045aa00, app+0x334 = mDDInterface).

##### 2. MTRand, Sexy::Rand, SRand

###### MTRand (no vtable)
sizeof 0x9c4 (1.2 0x9c4, port same layout).

| offset | type | name | meaning | port field | |
|---|---|---|---|---|---|
| 0x000 | ulong[624] | mt | state | mt | verified (NextNoAssert) |
| 0x9c0 | int | mti | next index; 624 after seeding | mti | verified |

Direct functions:

| address | name | 1.2 / port | notes |
|---|---|---|---|
| 004665f0 | `void MTRand::SRand(unsigned long seed)` | 1.2 identical; port **differs** | seed 0 -> 4357 (0x1105); `mt[0]=seed; mt[i]=mt[i-1]*69069`; mti=624 |
| 00466640 | `unsigned long MTRand::NextNoAssert()` | identical | regenerate when mti >= 624; tempering masks 0x9d2c5680/0xefc60000 (compiled as pre-shift masks 0xff3a58ad/0xffffdf8c); `& 0x7fffffff`; mag01 table 0053093c |
| 00467060 | `unsigned long MTRand::Next()` | identical | `jmp NextNoAssert` |
| 00467030 | `MTRand::MTRand(unsigned long seed)` | `MTRand(ulong)` | |
| 00467050 | `MTRand::MTRand()` | identical | SRand(4357); not a Ghidra function; called only by the gMTRand initializer 004f8300 |
| 004673d0 | `void MTRand::SRand(const std::string&)` | identical | |
| 00469b50 | `std::string MTRand::Serialize()` | identical | |
| 004665c0 | `static void MTRand::SetRandAllowed(bool)` | identical | counter 0056ada0; callers SexyApp 0047cd30/00480660/vf65 |
| 00467070 | `int Sexy::Rand()` | `Sexy::Rand()` | gMTRand 0056add8 |
| 004668a0 | `void Sexy::SRand(ulong)` | `Sexy::SRand` | callers: SexyApp::Init 0048aaf0 (`SRand(GetTickCount())`, also CRT `srand(GetTickCount())`), demo loader 00489e90 (seed from the demo file) |

Not present in this build: `MTRand::Next(ulong range)`, `Next(float)`, `NextNoAssert(range)`, `Sexy::Rand(int)`,
`Sexy::Rand(float)`, `UnsafeRand` (all 1.3 additions or unreferenced). The game draws ranges inline with `% n`.

Game RNG owners (all `new MTRand(Sexy::Rand())`, 0x9c4 bytes): global `MTRand*` at 0x5347d0 (WinMain 0040fe80,
after app Init, so time-seeded), `Board+0x6c` (Board ctor 0040ae90; its seed is also written into the Board game
log with WriteLong; saved/restored with Serialize/SRand_state in classic.dat/action.dat), `EffectMgr+8`
(004201c0) and `EmberListMgr+8` (00421070) (saved/restored by 0041faa0/0041ff80 and 00420b70/00420e70). The game
also calls CRT `rand()` (004dc094) from 0042dd20 (outside this slice).

##### 3. Buffer

sizeof 0x20, vtable 00503544 (1 slot), no bases. Same as 1.2 and the port.

| slot (+off) | address | method | 1.2 slot | port | notes |
|---|---|---|---|---|---|
| 0 (+0) | 0046e8c0 | `~Buffer()` scalar deleting (calls body 0046a3d0) | 0 | `virtual ~Buffer()` | |

| offset | type | name | meaning | port field | |
|---|---|---|---|---|---|
| 0x00 | vfptr | | 00503544 | | verified |
| 0x04 | std::vector<uchar> (16) | mData | first +0x08, last +0x0c, end +0x10 | mData | verified |
| 0x14 | int | mDataBitSize | | mDataBitSize | verified |
| 0x18 | int | mReadBitPos | | mReadBitPos | verified |
| 0x1c | int | mWriteBitPos | | mWriteBitPos | verified |

| address | method | port | notes |
|---|---|---|---|
| 004714a0 | `Buffer()` | same | |
| 0046a3d0 | `~Buffer()` body | same | |
| 00466960 | `SeekFront()` | same | |
| 00466970 | `GetDataLen()` | same | |
| 00466980 | `AtEnd()` | same | `mReadBitPos >= mDataBitSize` |
| 0046a520 | `Clear()` | same | |
| 00472660 | `WriteByte(uchar)` | same | |
| 004726e0 | `WriteNumBits(int, int)` | same | LSB first |
| 004727d0 | `WriteBoolean(bool)` | same | one byte |
| 004727f0 | `WriteShort(short)` | same | little endian |
| 00472820 | `WriteLong(long)` | same | little endian |
| 00472860 | `WriteString(const std::string&)` | same | short length + bytes |
| 004728b0 | `WriteBytes(uchar*, int)` | `WriteBytes(const uchar*, int)` | |
| 00474a70 | `WriteLine(const std::string&)` | same | appends "\r\n" |
| 00470bf0 | `SetData(uchar*, int)` | same | |
| 00468420 | `ReadByte()` | same | returns 0 on underflow (`(pos+7)/8 >= size`) |
| 004684b0 | `ReadNumBits(int, bool isSigned)` | same | |
| 00468580 | `ReadBoolean()` | same | |
| 00468590 | `ReadShort()` | same | |
| 004685b0 | `ReadLong()` | same | |
| 004685f0 | `ReadBytes(uchar*, int)` | same | |
| 0046a550 | `ReadString()` | same | |
| 0046a610 | `ReadLine()` | same | |
| 0046a400 | `ToWebString()` | same | "%08X" of mWriteBitPos + ceil(bits/6) chars of `.-0-9A-Za-z` (gWebEncodeMap 005304e0) |
| 004748a0 | `FromWebString(const std::string&)` | same | needs size >= 4; decode table 005304e8 |
| 00468620 | (folded) `GetDataPtr()` | same | `return mData._Myfirst` (NULL when empty) |

Not linked: WriteBuffer, ReadBuffer, GetCRC32, GetBitsRequired, PastEnd, GetDataLenBits, SetData(ByteVector).

Game use: the Board owns a Buffer at Board+0x160 ("game log"): the ctor writes a header (WriteByte 3, game mode
byte from app+0x1254, WriteLong(seed), WriteLong(time), ...), play code appends 4-bit event codes plus data
(004038c0, 00407a50, 0040f5b0, 0040bd40 via 004038c0). The log is saved as a web string inside
classic.dat/action.dat (00406c40), restored with Clear+FromWebString (00408ab0) and posted to PopCap as
`data=<web string>` by the score submission (004241d0). SexyApp uses Buffer for registry/demo/file I/O
(0047e0c0, 0047e550 ReadBufferFromFile, 0047ebe0, 0047fa40...; section 4).

##### 4. Other classes

###### 4.1 Common helpers (Common.cpp / DirectXErrorString / CritSect)

| address | name | port | notes |
|---|---|---|---|
| 0046e5d0 | `std::string StrFormat(const char* fmt, ...)` | same | via vformat 0046e480 |
| 0046e480 | `std::string vformat(const char*, va_list)` | same output | 160-char stack attempt, then heap doubling to 1,000,000 chars |
| 004668c0 | `int StrFindNoCase(const char*, const char*)` | same | |
| 004681c0 | `bool StringToInt(std::string, int*)` | same | see 5.5 |
| 00468300 | `bool StringToDouble(std::string, double*)` | same | digits, one '.', leading '-' only |
| 00469c10 | `std::string URLEncode(const std::string&)` | same | |
| 00469e90 | `std::string StringToUpper(const std::string&)` | same | |
| 00469f70 | `std::string StringToLower(const std::string&)` | same | |
| 0046a2e0 | `std::string Upper(const std::string&)` | same | |
| 0046d820 | `std::string Trim(const std::string&)` | same | |
| 0046d8a0 | `std::string CommaSeperate(int)` | same | 0 -> "0", negatives -> "" (port identical) |
| 0046e670 | `std::string XMLDecodeString(const std::string&)` | same | entities lt amp gt quot apos nbsp cr; an unknown `&name;` becomes "&"; a '&' with no later ';' is kept |
| 004723e0 | `std::string Evaluate(const std::string&, const DefinesMap&)` | same | used by SexyApp::OpenHTMLTemplate |
| 00467420 | `time_t GetFileDate(const std::string&)` | same | |
| 0046da40 | `std::string GetCurDir()` | same | |
| 0046dac0 | `std::string GetPathFrom(const std::string&, const std::string&)` | same | |
| 00470b80 | `std::string GetFullPath(const std::string&)` | same | |
| 0046e340 | `std::string GetFileDir(const std::string&)` | `GetFileDir(path, false)` | 2004 form has no withSlash parameter |
| 0046e400 | `std::string RemoveTrailingSlash(const std::string&)` | same | |
| 0046e230 | `void MkDir(const std::string&)` | same | splits on '\\' only |
| 0046a050 | `bool Deltree(const std::string&)` | same | |
| 0046c7f0 | `std::string GetDirectXErrorString(HRESULT)` | not in port | droppable |
| 00466880 / 00466890 | `CritSect::CritSect()` / `~CritSect()` | `CritSect` (port: no-op/SDL) | AutoCrit is inlined (Enter/LeaveCriticalSection on +0) |
| 00469850 | `std::string operator+(char, const std::string&)` | STL | |

Not linked: FileExists (SexyApp has its own 0047e700), GetFileName, AddTrailingSlash, XMLEncodeString,
StrPrefixNoCase, StringToWString, Lower. The note in the brief "CritSect / AutoCrit (00466870/00466880)" is half
right: 00466870 is `HTTPTransfer::Abort()`, 00466880 is the CritSect ctor.

###### 4.2 PropertiesParser
sizeof 0x2c, vtable 005034b4 (1 slot), no bases. 1.2 and port: same members (port mError is SexyString).

| slot | address | method | 1.2 slot | port | notes |
|---|---|---|---|---|---|
| 0 (+0) | 00469040 | `~PropertiesParser()` scalar deleting (body 00467b70) | 0 | `virtual ~PropertiesParser()` | |

| offset | type | name | port | |
|---|---|---|---|---|
| 0x00 | vfptr | | | verified |
| 0x04 | SexyApp* | mApp | mApp | verified (ctor) |
| 0x08 | XMLParser* | mXMLParser | mXMLParser | verified |
| 0x0c | std::string | mError | mError | verified (GetErrorText 004699a0) |
| 0x28 | bool | mHasFailed | mHasFailed | verified |

| address | method | notes |
|---|---|---|
| 00467b40 | `PropertiesParser(SexyApp*)` | |
| 00479c30 | `bool ParsePropertiesBuffer(const Buffer&)` | the only entry point: new XMLParser (0x74), string from GetDataPtr/GetDataLen, SetStringSource, DoParseProperties. **ParsePropertiesFile is not linked.** Caller: SexyApp::LoadProperties 00486fe0 ("Unable to open properties file: ", "Signature check failed on '") |
| 004799d0 | `bool DoParseProperties()` | root element must be `Properties` |
| 00478e90 | `bool ParseProperties()` | `<String id>`, `<StringArray id>` (of `<String>`), `<Boolean id>` (Upper(value) in 1/YES/ON/TRUE or 0/NO/OFF/FALSE), `<Integer id>` (StringToInt), `<Double id>` (StringToDouble); anything else: "Invalid Section" |
| 004750c0 | `bool ParseStringArray(StringVector*)` | |
| 00474f20 | `bool ParseSingleElement(std::string*)` | |
| 0046c2f0 | `void Fail(const std::string&)` | adds " on Line N" and " in File '...'" |
| 004699a0 | `std::string GetErrorText()` | |

The shipped `properties/partner.xml` (602 bytes, plus `partner.xml.sig`): one `<Properties>` element with
String `ReferId` (empty), Booleans `NoReg`, `DontUpdate`, `NoInternetScores` = True, `ShowTrialStatus` = False,
`NoIHSUpsell` = True, Integers `NumOptionsLinks` = 1, `LoadingBarY` = 330, `OptionsDialogHeight` = 315, Strings
`OptionsLinkText0` / `OptionsLinkURL0` (the URL contains a raw `&referid=`: kept verbatim because no ';'
follows). No default.xml exists in the install and the binary has no "default.xml" string; only
`properties\partner.xml` is loaded (from SexyApp::Init 0048aaf0).

###### 4.3 XMLParser / XMLElement
XMLParser sizeof 0x74, vtable 005034d4 (1 slot). XMLElement sizeof 0x70 (no vtable). Same as 1.2. The port's
XMLParser is the 1.3 wide-char one (SexyString members, BOM detection, mGetCharFunc).

| slot | address | method | 1.2 | port |
|---|---|---|---|---|
| 0 (+0) | 004698f0 | `~XMLParser()` scalar deleting (body 00469760: fclose(mFile)) | 0 | `virtual ~XMLParser()` |

| offset | type | name | port | |
|---|---|---|---|---|
| 0x04 | std::string | mFileName | mFileName | verified (GetFileName 00469960) |
| 0x20 | std::string | mErrorText | mErrorText (SexyString) | verified |
| 0x3c | int | mLineNum | mLineNum | verified (00465f40) |
| 0x40 | FILE* | mFile | mFile (PFILE*) | verified |
| 0x44 | bool | mHasFailed | mHasFailed | verified (00465f30) |
| 0x45 | bool | mAllowComments | mAllowComments | verified |
| 0x48 | std::vector<char> | mBufferedText | mBufferedText (wide) | verified |
| 0x58 | std::string | mSection | mSection | verified |

XMLElement: +0x00 int mType (0 NONE, 1 START, 2 END, 3 ELEMENT, 4 INSTRUCTION, 5 COMMENT), +0x04 mSection,
+0x20 mValue, +0x3c mInstruction, +0x58 mAttributes (map<string,string>), +0x64 mAttributeIteratorList.

| address | method | notes |
|---|---|---|
| 004713d0 | `XMLParser()` | mFile=0, mLineNum=0, mAllowComments=false |
| 0046bd40 | `void Init()` | |
| 004719f0 | `void SetStringSource(const std::string&)` | |
| 00473190 | `bool NextElement(XMLElement*)` | 1.2 algorithm, narrow chars |
| 004718c0 | `bool AddAttribute(XMLElement*, key, value)` | |
| 0046bd20 | `void Fail(const std::string&)` | |
| 00469920 / 00469960 / 00465f40 / 00465f30 | GetErrorText / GetFileName / GetCurrentLineNum / HasFailed | |
| 004743f0 / 00474490 | `XMLElement()` / `~XMLElement()` | |

Not linked: OpenFile, EndOfFile, AllowComments (inline). Syntax the 2004 parser accepts: `<name attr="v" ...>`,
`</name>`, empty elements `<name/>` or `<name attr="v"/>` (a fake end tag is pushed back), text elements (runs
of whitespace collapse to one space), `<?...?>` instructions, `<!-- -->` comments (skipped unless
mAllowComments). Only double quotes delimit attribute values (a `'` is an ordinary character); unquoted values are
accepted; keys/values/text go through XMLDecodeString. Errors: "Unexpected '<'", "Unexpected '>'", "Unexpected
End", "End 'x' Doesn't Match Start 'y'", "Illegal Character" (a byte <= 32 that is not whitespace),
"Unexpected End of File".

###### 4.4 ResourceManager (linked, unused)
sizeof 0xb4 (`operator new(0xb4)` in the SexyApp ctor 0048f810), vtable 00504b0c with **3 slots** (1.2 has
many more; the port's 1.3 class has ~20 virtuals).

| slot | address | method | 1.2 slot (BM) | port |
|---|---|---|---|---|
| 0 (+0) | 004728e0 | `bool DoLoadImage(ImageRes*)` ("Failed to load image: %s"; alpha / alpha-grid helpers 0046f160 / 0046ef40) | DoLoadImage | `DoLoadImage` |
| 1 (+4) | 0046f2e0 | `bool DoLoadFont(FontRes*)` ("!ref:" fonts via 0046a6e0) | 3 | `DoLoadFont` |
| 2 (+8) | 004755c0 | `~ResourceManager()` scalar deleting (body 00474cb0) | 4 | `~ResourceManager` |

Fields (from the ctor 00475490, inferred names): +0x04/+0x10/+0x1c mImageMap/mSoundMap/mFontMap (map<string,
BaseRes*>), +0x28/+0x2c/+0x30 iterators into those maps (older LoadNext* design), +0x34, +0x38 (mXMLParser?),
+0x3c std::string (mError?), +0x58 bool mHasFailed, +0x5c SexyApp* mApp, +0x60/+0x7c/+0x98 strings
(mCurResGroup / mDefaultPath / mDefaultIdPrefix?). No mLoadedGroups.

Confirmed unused: app+0x60c is written by the SexyApp ctor (00490591) and read only by the SexyApp dtor
(0048f298, delete through slot 2). No other code reads app+0x60c, DoLoadImage/DoLoadFont have no direct callers, and no
manifest parser is linked (no "Resources"/"Image"/"Sound"/"Font" section strings, no resources.xml). Port:
nothing to do (do not create one).

###### 4.5 CritSect, SEHCatcher, PerfTimer, SexyPerf, SexyMatrix, ModVal (droppable)
- SEHCatcher (crash.txt, StackWalk via dbghelp, crash report POST): 00466160 LoadImageHelp, 00466300
  UnloadImageHelp, 00466320 GetLogicalAddress, 0046bda0 GetFilename, 0046be40 GetSysInfo, 00470190 IntelWalk,
  00470390 ImageHelpWalk, 00471a50 WriteToFile, 004759b0 GetSymbolsFromMapFile, 00477bd0 SubmitReportThread,
  00467360 hex helper. The handler itself (0047bdf0) and its windows (0047a400...) live in the SexyApp range.
  Confirmed droppable: no game code reaches any of it.
- PerfTimer 004664c0/00466510/00466520/00466540, CPU speed 004663d0/00466560; SexyPerf 00469070 BeginPerf,
  00470890 EndPerf, 004699e0 GetResults, 004665b0 IsPerfOn: debug only (SexyApp vf64), droppable.
- SexyMatrix3 / SexyTransform2D 00465f50-00466110: used by DDImage rotated blits (graphics group).
- ModVal (0x498c70-0x4992e0 helpers, 0049a360 in SexyApp vf64): debug "reparse modifiable values", droppable.

###### 4.6 HTTPTransfer
sizeof 0xfc, vtable 005034c4 (1 slot: 004690e0 scalar deleting dtor, body 00468070). Same layout as 1.2.
Not in the port.

| offset | type | name | |
|---|---|---|---|
| 0x04 | int | mTransferId (= gCurTransferId 005304d8 ++) | verified |
| 0x08 | SOCKET | mSocket | verified (SocketWait) |
| 0x0c / 0x28 / 0x44 / 0x60 / 0x7c / 0x98 | std::string | mSendStr / mSpecifiedBaseURL / mSpecifiedRelURL / mURL / mProto / mHost | verified (ctor, Get/Post) |
| 0xb4 | int | mPort | inferred |
| 0xb8 / 0xd4 | std::string | mPath / mContent | verified (GetHelper uses +0xb8, GetContent +0xd4) |
| 0xf0 / 0xf1 / 0xf2 / 0xf3 | bool | mTransferPending / mThreadRunning / mExiting / mAborted | verified |
| 0xf4 / 0xf8 | int | mResult / mDemoLastKnownResult (both 1 = RESULT_NOT_STARTED) | verified |

Functions: 00467fc0 ctor, 00468070 dtor, 00466870 Abort, 00466740 Fail, 00466760 SocketWait, 0046c500 GetAbsURL,
004763d0 PrepareTransfer, 004723a0 StartTransfer (no thread while gSexyAppBase(0056b8a8)->mPlayingDemoBuffer
+0x541), 00472390 GetThreadProcStub, 00471b30 GetThreadProc, 00476670 GetHelper, 004767d0 PostHelper,
00476ac0 Get(url), 00476b60 Get(base, rel), 00476b10 Post(url, params), 00476bf0 SendRequestString, 00475410
Reset, 004745d0 UpdateStatus (demo record/playback of results), 004747d0 WaitFor, 00474800 GetResultCode,
00474810 GetContent. Result codes as 1.2 (0 DONE, 1 NOT_STARTED, 2 NOT_COMPLETED, 3 NOT_FOUND, 4 HTTP_ERROR,
5 ABORTED, 6 SOCKET_ERROR, 7 INVALID_ADDR, 8 CONNECT_FAIL, 9 DISCONNECTED).

What the game does with it (FameWidget = the Hall of Fame / online high-score screen, member at +0xd8):
- 004241d0: `Get(GetString("GetScoresURL", "http://www.popcap.com/bookworm_getscores.php") + "?clientver=" +
  app->(+0x3b8 version string))` with a "retrieving scores" status text; and
  `Post(GetString("SubmitScoreURL", "http://www.popcap.com/bookworm_submitscore.php"), "name=<name, spaces as
  %20>&data=<Board game log ToWebString>&clientver=<ver>")` with a "posting scores" status text.
  00426c10 (FameWidget ButtonDepress) also Posts to SubmitScoreURL.
- 00427f30 (FameWidget::Update) polls `GetResultCode()`: 2 keeps waiting; 0 reads `GetContent()` and parses it
  (00427650); any other code ends in a "server could not be reached" error (state 3) or stores the code
  (state 6, submit path; the reply can report a resubmitted score or a name already taken).
- These paths are presumably gated by the `NoInternetScores` property (read at 004152a0 and 00417080; not traced
  here), which the shipped partner.xml sets to True.

Not register, not "more games", not ads: those go through SexyApp OpenURL / InternetManager.

What the port must stub (game side, e.g. in a Compat header like Big Money's Compat12.h): a tiny
`HTTPTransfer` with the members the game reads/calls: ctor, dtor, `Get(const std::string&)`,
`Post(const std::string&, const std::string&)`, `GetResultCode()`, `GetContent()`. Get/Post must not touch the
network; simplest is to leave `mResult = RESULT_NOT_STARTED` (or set RESULT_CONNECT_FAIL = 8) so Update takes the
"server could not be reached" path immediately, and GetContent returns "". Alternatively remove the online
branch entirely under NoInternetScores=true. SexyApp also owns one HTTPTransfer at app+0x32c (ctor 0048f810,
Shutdown vf33 0047cbf0 aborts it and checks GetResultCode == 0 -> app+0x330); no Get/Post on it was found
(section 4, open question 3).

###### 4.7 InternetManager (droppable)
sizeof 0x538, vtable 00505468 (1 slot: 0047c440 scalar deleting dtor, body 0047baf0); ctor 00479d10 (BM 00473040
exact), created by the SexyApp ctor at app+0xd8. Fields (inferred from the ctor): +0x04 int (4 = ad transfer
count), +0x08 HTTPTransfer (update check), +0x104 HTTPTransfer[4] (ads), +0x4f4 bool, +0x4f5 bool (= 1;
"up to date" flag returned by 0047c780), +0x4f6 bool, +0x4f8 std::string (update URL), +0x514/+0x520 lists of
ads (0x5c-byte nodes), +0x52c list. Functions: 00479df0 DoUpdateCheck (game 00415de0), 0047c500 process the
update-check reply, 0047c780 IsUpToDate, 0047c7a0 GetUpdateURL (game 00417080), 0047c810 / 004753e0
(UpdateCheckDialog 0043b730), ads: 0047b290 LoadAdList ("adlist.txt", from Init via thunk 0047bc30),
0047b3d0 / 004709c0 SaveAdList, 0047a930 / 0047aed0 / 00479e10 ad parsing and download ("AdFile: %s"),
00467ba0..00467d20 ad queries / ShowAd (from SexyApp vf36, uses Rand()).
Port: drop. Game-side, stub `DoUpdateCheck` / `GetUpdateURL` or remove the "check for updates" path (DontUpdate
is True in partner.xml).

###### 4.8 BetaSupport (droppable)
sizeof 0x18c, vtable 005056e0 (1 slot: 0047bdc0 scalar deleting dtor, body 0047b530), ctor 0047bc40. Always
created in SexyApp::Init (app+0xdc), but it only acts when the beta flag app+0xe0 is set: then 00477930 shows the
"PopCap Beta Login" window (0046ea00, wndproc 00466a90, 004669a0 / 00466a20 helpers), reads/writes
`HKCU\Software\PopCap\Beta` Name/Password (0046e8f0 / 004674c0), checks the login over HTTP (00476c40:
"&username=", "&pass=", "&version=", ":ERROR", ":COMMENTURL") and on failure Init shuts down and exits. Release
BookWorm never sets app+0xe0. No game code touches it. Port: drop.

###### 4.9 BigInt (registration codes, droppable)
0x496ad0-0x498c70 is a big-integer class (vtable 00507558; +0x04 sign byte, +0x08 ushort* blocks, +0x0c
length, +0x10 capacity; McCutchen-style NumberlikeArray<ushort>) used only by `SexyApp::Validate` (004830b0,
slot 23) and 00486c90 (registration): 00498430 ctor from string, 00498a20 to a code string over the alphabet
"234679ACDEFGHJKLMNPQRTUVWXYZ", the rest arithmetic. Droppable (partner.xml NoReg=True).
The four game-called addresses in this range are not BigInt: 00496c00 is the linker-folded `return this+0xc`
(Font::GetHeight / BigInt length / Color::mAlpha), and 00499030 / 004990b0 / 004990f0 are folded STL helpers for
16-byte POD elements (vector<Color>; also ModVal's vector). 0x498c70-0x4992e0 is ModVal's map / parse helpers.

###### 4.10 The folded getters
- 00468620 (`mov eax,[ecx+8]; ret`, 68 direct callers). Every direct caller is a non-virtual use: in game code it
  is always `Image::GetHeight()` (e.g. 00428520: `button->Resize(0, 0, img->GetWidth() /*get_field_04*/,
  img->GetHeight() /*00468620*/)` on app image slots app+0x11ac+N and widget image members); in framework code
  also `Buffer::GetDataPtr()` (00479c30) and other `this+8` reads. It is also Font vtable slot 2
  (`GetAscentPadding`), but that is only reached through the vtable. Port: `theImage->GetHeight()` (port
  `Image::GetHeight()` returns mHeight).
- 00496c00 (`mov eax,[ecx+0xc]`): Font vtable slot 4 GetHeight; direct game callers 00410090 (`Color& c`:
  c.mAlpha) and 00432c90 (on app+0x1178) both throw the value away. Port: nothing.
- 00496bf0 (`mov al,[ecx+4]`): BigInt sign and other byte getters.

###### 4.11 DDInterface / D3DInterface / D3DTester (what SexyApp and the game touch)
DDInterface: sizeof 0xcf0 (`operator new(0xcf0)` in SexyApp vf09 00488640), vtable 00502b28 (1 slot: 00461710
scalar deleting dtor, body 00459ef0), no NativeDisplay base (1.2 derives from NativeDisplay; here its fields are
inline at +0x54..+0x78). app->mDDInterface is app+0x334.

| offset | type | name | used by | port | |
|---|---|---|---|---|---|
| 0x004 | SexyApp* | mApp | | mApp | verified |
| 0x008 | D3DInterface* | mD3DInterface | Remove3DData | (SDL renderer) | verified |
| 0x00c | D3DTester* | mD3DTester | app 0047e380/0047e3a0, vf09 (+0x82 mResultsChanged) | not in port | verified |
| 0x010 | bool | mIs3D | app 0047ddb0 (Is3DAccelerated), 00488819 (set from registry "Is3D" before Init) | not in port | verified |
| 0x014 | CritSect | mCritSect | app AutoCrit at 0047f3d4, 004874c6, 00488b75 | | verified |
| 0x02c / 0x030 | IDirectDraw* / IDirectDraw7* | mDD / mDD7 | app 0047cc9c / 0048cfff | | verified |
| 0x034 / 0x038 / 0x03c | surfaces | mPrimarySurface / mSecondarySurface / mDrawSurface | | | inferred order |
| 0x040 / 0x044 / 0x048 | int | mWidth / mHeight / mFullscreenBits (copied from app+0x60/0x64/0x68 in Init) | | mWidth/mHeight | verified |
| 0x04c / 0x050 | DWORD | mRefreshRate (60) / mMillisecondsPerFrame (16) | app vf65 00486702 reads +0x50 | | verified |
| 0x054.. | int/ulong | mRGBBits (16 or 32 required in windowed mode), masks, bits, shifts | | | verified (+0x54/+0x58), rest inferred |
| 0x07c/0x080/0x084 | int* | mRed/Green/BlueAddTable | | | verified |
| 0x088 | ulong[3][256] | mRed/Green/BlueConvTable | | | inferred |
| 0xc88 | bool | mInitialized | | | verified |
| 0xc8c / 0xc90 | HWND / bool | mHWnd / mIsWindowed | | mIsWindowed | verified |
| 0xc94 | DDImage* | mScreenImage | GetScreenImage 00440010 (app -> WidgetManager+0x10) | mScreenImage | verified |
| 0xc98 | std::set<DDImage*> | mDDImageSet | | | verified |
| 0xca4 | bool | mVideoOnlyDraw | SetVideoOnlyDraw 00461740 (app vf54, value also kept at app+0x338) | | verified |
| 0xca8 | ulong | mInitCount | | | inferred |
| 0xcac / 0xcb0 | int | mCursorWidth / mCursorHeight (64) | | | verified |
| 0xcb4 / 0xcb8 | int | mCursorX / mCursorY (no mNextCursorX/Y in this build) | SetCursorPos 0044f5b0 | mCursorX/Y | verified |
| 0xcbc | Image* | mCursorImage | SetCursorImage 0044f530 (app vf10) | mCursorImage | verified |
| 0xcc0 | bool | mHasOldCursorArea | | | verified |
| 0xcc4..0xcd0 | | mOldCursorArea, mNewCursorArea, mOldCursorAreaImage, mNewCursorAreaImage | | | inferred |
| 0xcd4 | std::string | mErrorString | app appends it to "Failed to initialize DirectDraw: " | | verified |

Methods SexyApp calls: 004615a0 ctor (vf09), 004619f0 `int Init(HWND, bool isWindowed)` (00487530, vf09,
vf66; windowed: display depth must be 16 or 32 else RESULT_INVALID_COLORDEPTH; fullscreen: SetDisplayMode(mWidth,
mHeight, mFullscreenBits); with mIs3D it runs Do3DTest 00458bf0 and D3D init, errors "3D init error: "),
004536a0 ResultToString, 004538f0 Redraw(Rect*) (vf66), 00440010 GetScreenImage, 00461740 SetVideoOnlyDraw,
0044f530 SetCursorImage, 0044f5b0 SetCursorPos, 00459ff0 Remove3DData (via app 0047dd90). The resolution is the
app's (BookWorm 640x480; D3DInterface's own default is also 640x480 at +8/+0xc).
D3DInterface: sizeof 0x4c, vtable 00502aac (1 slot 0045a090), ctor 0045a000. D3DTester: sizeof 0x84, vtable
0050232c (slot 0 00450840), ctor 00450140: mFailureReason +0x40, mWarning +0x5c, mMinVidMemory +0x78,
mRecommendedVidMemory +0x7c, mCheckRegistry +0x80, mShouldWriteToRegistry +0x81, mResultsChanged +0x82. App:
`Is3DAccelerationSupported()` = mD3DTester && mFailureReason empty; `Is3DAccelerationRecommended()` = also
mWarning empty (OptionsDialog 0042fa50 uses both for the 3D checkbox).
Game: only `new DDImage(app->mDDInterface)` (0040111f, 0040125d) -> port `new DDImage(gSexyAppBase->mDDInterface)`.
Port: everything else is the SDL DDInterface; mIs3D / D3DTester have no equivalent (section 4 decides what
Is3DAccelerated returns).

###### 4.12 STL helpers in the slice (for recognising them)
Tree helpers are named by node value size because identical instantiations are folded: v28 (isnil +0x29,
set<std::string>), v32 (+0x2d, map<string,ptr/int>, ResourceManager maps), v36 / v40 (+0x31 / +0x35, SEHCatcher
map-file maps keyed by pair<ulong,ulong>), v44 (+0x39, set<PerfInfo> and map<string, vector<string>> =
mStringVectorProperties), v60 (+0x49, map<string, Buffer>, BetaSupport), v196 (+0xd1, ModVal). vector<uchar>
helpers serve Buffer (0046f910 _Insert_n, 00471520 push_back, 0046b920 insert range); wstring helpers
(00467700, 004677b0, 00468860, 00468b00, 00469500, 004695e0) serve the CRT iostreams.

##### 5. Behaviour differences that matter for the port

1. **MTRand seeding (5.1).** BookWorm's `MTRand::SRand(ulong)` (004665f0) is SAF 1.2's Knuth initialiser
   (`mt[i] = mt[i-1] * 69069`, seed 0 -> 4357, mti = 624); the port's `MTRand::SRand` / `MTRand(ulong)` /
   `Sexy::SRand` use 1.3's `1812433253 * (x ^ x>>30) + i`. The same seed gives a different stream. Next(),
   tempering, Serialize and SRand(string) are identical. Port action: game-side shim exactly like Big Money's
   `SeedMTRand12` (games/bigmoney/src/Board.cpp in the bigmoney worktree: build the 624 words with 69069 and
   `SetState(words, MTRAND_N)`) wherever BookWorm does `new MTRand(Rand())` (Board, EffectMgr, EmberListMgr,
   global 0x5347d0). Needed for loading original classic.dat/action.dat? No (those restore the words), but needed
   for any seed-based reproduction (the seed is in the Board game log) and for tests comparing against the
   original. The global stream (`Sexy::SRand(GetTickCount())`) only matters for demo playback / deterministic
   tests; if that is wanted the app must seed gMTRand the 1.2 way too (small framework option or app-side shim).
2. **Loaded MTRand keeps its old mti.** SRand(string) copies the words but not mti, so after loading a save the
   Board RNG continues from wherever its fresh `MTRand(seed)` was (mti 624 = new block). Port SRand(string) has
   the same semantics; keep using it (not SetState) when loading original saves.
3. **Buffer web strings (5.3).** Identical encoding; FromWebString decodes lowercase hex in the 8-char length
   as `c - 'f' + 10` (1.2 bug, kept in BookWorm and the port). ToWebString always writes uppercase, so it never
   triggers. Nothing to do; do not "fix" it in the port or old saves could change meaning.
4. **StrFormat (5.4).** Same output for anything under 1,000,000 chars (BookWorm gives up doubling at 1 MB and
   would then index a short buffer; the port has no limit). Strings with embedded NULs are cut at the NUL in
   BookWorm (built with strlen) but kept by the port (built with the returned length); BookWorm never formats
   %c with 0. Nothing to do.
5. **StringToInt (5.5).** Accepts optional leading '-', decimal digits, or hex after "0x"/"0X" where the 'x' is
   at index 1 and the value so far is 0 (so "-x1F" is also accepted as -31); no '+', no spaces, empty -> false and
   *val = 0; on failure *val = 0. Port is the same code. Nothing to do.
6. **Properties.** BookWorm parses partner.xml from a Buffer after a signature check against partner.xml.sig
   (SexyApp::LoadProperties 00486fe0; section 4). The port's PropertiesParser::ParsePropertiesBuffer does the same
   parsing (ASCII, widened). Types String/StringArray/Boolean/Integer/Double, Boolean words as 1.2. Port action:
   skip the signature check (or accept), nothing else.
7. **XML parser.** 1.2 narrow parser vs port 1.3 wide parser: same grammar for these files (double-quoted
   attributes only, comments skipped, entity decoding via XMLDecodeString with the same 7 entities). Differences
   only for non-ASCII bytes and BOMs (port may treat a file with a UTF-8/16 BOM differently; BookWorm reads from a
   Buffer, so the port's buffer path matches). Nothing to do.
8. **HTTPTransfer** is game-visible (FameWidget member at +0xd8, Get/Post/GetResultCode/GetContent). Port:
   game-side stub that never connects (section 4.6), or compile the online branch out under NoInternetScores.
9. **InternetManager** is reached by game code for the update check (00415de0 DoUpdateCheck, 00417080
   GetUpdateURL). Port: stub/drop (DontUpdate=True).
10. **ResourceManager / BetaSupport / SEHCatcher / PerfTimer / ModVal / BigInt / DirectXErrorString**: no game
    use; drop. `get_field_0c` calls at 00410090 / 00432c90 are dead and can be dropped.
11. **DDInterface**: the game only constructs DDImages on `app->mDDInterface`; port `DDImage(DDInterface*)` exists.
    The 2004 DDInterface has no mNextCursorX/Y, no mInRedraw and no NativeDisplay vtable base (irrelevant to game
    code).

##### 6. Open questions (section 7.1)
1. Whether the port needs 1.2 seeding for the global `Sexy::Rand` (only demo playback / deterministic test runs
   depend on it). Recommendation: game-side `SeedMTRand12` for the game's own MTRands; leave the global alone
   unless demo/replay parity with the original is a goal.
2. Exact names of ResourceManager fields +0x34..+0x98 (unused class; not worth more time).
3. Answered in section 4: SexyApp's own HTTPTransfer at app+0x32c is never started (no Get/Post anywhere in the binary;
   only the ctor, the dtor and Shutdown touch app+0x32c); Shutdown's Abort/GetResultCode always sees "not started", so
   registry ITSuccess stays false. Dead: drop.
4. Answered in section 4: 0x56b8a8 = gSexyAppBase (stored first in the ctor, like 1.2's SexyAppBase ctor), 0x56ad30 =
   gSexyApp (stored last, like 1.2's SexyApp ctor). Both point at the app.
5. 00468620 has 68 callers; I checked the game sites by context (image slots, Resize(w, h) pairs) rather than
   one by one; any site whose receiver is not an Image would be some other `this+8` field (none found).

### 7.2 std::string, STL, iostream and CRT helpers called by game code

Scope: every library function the game code (0x401000-0x437800) calls directly, and every STL template
instantiation that lives inside the game region. Toolchain: MSVC 7.0 (VS.NET 2002), Dinkumware STL of that
release, static CRT (LIBCMT), plus the old pre-standard iostream library (LIBCI: `ifstream`, `ios`,
`filebuf` without `std::`), which VC7.0 still shipped.

Result: **368 functions identified** (270 of them inside the game region): std::string 44, std::list 24,
set/multiset/map 77, std::vector 23, algorithms and uninitialized-memory helpers 25, iostream/locale 122
(most are internals nobody in the game calls), CRT 38, EH/RTTI/exception classes 15. 252 were `FUN_`
before, 42 get a better name (mostly the RTTI-scan names `std::length_error::ctor_or_dtor_*` /
`std::out_of_range::ctor_or_dtor_*`, which are really container functions that contain a throw, and
`_free` 004da392, which is really `operator delete`). Symbols: app_symbols.jsonl (one line per
function in the tables below).

Verified = read the decompilation (and, where Ghidra hides something, the disassembly) and matched it
with the VC7 Dinkumware source pattern; "(inferred)" = identified from its role/callees only.

##### Conventions you need first

- **No LTCG register conventions.** Every member is plain `__thiscall` (ECX = this, callee pops stack args;
  the `ret N` sizes were checked). Ghidra prints one-argument members as `__fastcall(int param_1)`; that is
  the same thing (ECX = this). Functions returning a class object (`substr`, `operator+`, `begin()`,
  `find`, `insert`...) take a hidden result pointer as their first stack argument (`param_1` in Ghidra).
- **Identical-COMDAT folding is on** (/OPT:ICF). One body serves every instantiation that compiles to the
  same bytes: all `std::list<T>` helpers with a 4-byte T share one `_Buynode`/`_Buyhead`; the tree leaf
  helpers of `std::map<int,bool>` (HintDialog) are the framework's `std::map<int,Dialog*>` ones
  (0047ddc0...); the node-value destructor 0041ccb0 serves both `map<string,string>` and the high-score
  multiset; 00411520 destroys any `{string,string,...}`. Bodies that embed a string literal (the
  `"list<T> too long"`, `"vector<T> too long"`, `"map/set<T> too long"` throws) or call themselves
  (`_Erase`) are NOT folded, so there is one copy per .obj: e.g. `list<T*>::_Incsize` in Board.obj
  (004066e0), BookWormApp.obj (00412780), EffectMgr.obj (00420400) and OptionsDialog.obj (0042f3b0).
  The suffixes `_2`, `_3` in the names are only these per-object copies.
- **Template code in the game region is shared with the framework.** The linker kept the first COMDAT,
  which was often in a game .obj: the framework calls into 0041d670 (`map<string,string>::insert`),
  00432470 (DefinesMap copy-ctor), 00426b10 (`std::fstream` ctor), 004233c0, 00404c50, ... and the game
  calls framework-region copies (0047f470 `~string`, 0047de60, 00466eb0, 00489140, 0048b330, 00499030...).
- Layouts (VC7): `std::string` 28 bytes = allocator byte +0, union {char buf[16]; char* ptr} +4, size +0x14,
  capacity +0x18 (heap when capacity > 15). `std::list`/`set`/`map` 12 bytes = allocator +0, head node
  pointer +4, size +8. `std::vector` 16 bytes = allocator +0, first +4, last +8, end-of-storage +0xc.

##### Framework-region rows of callees.md checked for this slice

address | callees.md name | verdict
---|---|---
0047f470 | std_string_Tidy | STL: `std::string` destructor / `_Tidy(true)` (BM 0045d100 exact). 1416 callers, 17 game call sites. Listed under std::string.
00496c00 | get_field_0c | not STL (ICF-folded `return this->field_0xc` accessor; Font::GetHeight-style)
00499030, 004990b0, 004990f0 | FUN_ | STL: `std::fill` / `uninitialized_fill_n` / `uninitialized_copy` for 16-byte `Sexy::Color` (vector<Color> of ColorizedButton). Listed under algorithms.
00467030 | FUN_ | not STL: `Sexy::MTRand::MTRand(unsigned long seed)` (seed 0 -> 4357)
00467060 | thunk_FUN_00466640 | not STL: `Sexy::MTRand::Next()` (jmp to the MT19937 generator 00466640)
00467070 | FUN_ | not STL: `Sexy::Rand()` = the global MTRand at 0056add8 `.Next()`
00468620 | Sexy::Font::vf02 | not STL: ICF-folded 4-byte accessor `return this->field_8` (68 callers; Font::GetAscent and every other `+8` getter)
00466970 | FUN_ | not STL: `Sexy::Buffer` bit size to bytes, `(mDataBitSize + 7) / 8` (GetDataLen)
00466eb0, 00466f50 | FUN_ | STL: `std::set<std::string>::_Max` / `_Min` (framework copies used by the game's set<string>::erase 00412870)
00472660, 004726e0, 004727f0, 00472820, 00472860 | FUN_ | not STL: `Sexy::Buffer::WriteByte`, `WriteNumBits(int value, int bits)`, `WriteShort`, `WriteLong`, `WriteString` (inferred names; another helper owns Buffer)
00474800, 00474810 | FUN_ | not STL: HTTPTransfer result accessors (result code at +0xf4, content string at +0xd4)
00476ac0, 00476b10 | FUN_ | not STL: HTTPTransfer Get(url) / Post(url, params) style starters
0047ddc0, 0047de20, 0047de40, 0047de60, 0047dec0, 0047e3d0, 0047e9f0 | FUN_ (not in the request list) | STL: tree helpers of the int-keyed 8-byte-value map (framework `std::map<int,Dialog*>`, inferred from SexyApp vf57/vf58), ICF-shared with the game's `std::map<int,bool>`. Listed under map/set.
00489140, 0048b330 | FUN_ (not in the request list) | STL: `std::list<std::string>::erase(first,last)` and `~list`, called by game 00419080. Listed under std::list.
0047ddb0 | FUN_ | not STL: SexyApp accessor (byte at +0x334)

##### Containers the game uses (what to declare in the port)

- `std::set<std::string>` x2 in BookWormApp (+0x1260, +0x127c): one is the **dictionary**, filled by the
  word-list loader 0041a5b0 (prefix-compressed word list read with fopen/getc, words uppercased with CRT
  toupper, length >= 3 kept); Board 0040abd0 inserts into a set<string> too. Ordering is std::less<string>
  (unsigned byte compare, then length): identical to libc++.
- `std::multiset<HighScoreEntry, Pred>` x4 in BookWormApp: three with **ByScore** (+0x117c, +0x1188,
  +0x1194) and one with **ByWordLength** (+0x11a0); default tables built by 00418bd0 (the
  default names and scores, BookWorm's `GT_HS_NAME_*` texts) with `if (size < N && find(e) == end()) insert(e)`, N = 10 for +0x117c and +0x1188, 3 for
  +0x1194 and for the ByWordLength table +0x11a0. HighScoreEntry (name invented) is 0x40 bytes: `std::string name` +0, `std::string word` +0x1c, `int score` +0x38, `bool` +0x3c.
  The predicates are game code, taking both entries BY VALUE (hence the copy-ctor/dtor pairs around every
  comparison): ByScore 004115d0 = `a.score != b.score ? a.score > b.score : strcmp(a.word, b.word) < 0`;
  ByWordLength 00411570 = `a.word.size() != b.word.size() ? a.word.size() > b.word.size() : a.score > b.score`.
  multiset::insert puts an equal key after the existing equal ones (upper position).
- `std::map<std::string,std::string>`: Definitionator (+8; `defs[word] = definition`), and the framework's
  DefinesMap (RegisterDialog / SexyApp::OpenRegisterPage copy it with 00432470).
- `std::map<int,bool>`: one static at DAT_00534c38 used by HintDialog (0042b3e0: `find(0)`, `insert`).
- `std::list<T*>` (pointer lists in Board, EffectMgr, FlyingPieceList, OptionsDialog, WidgetManager...),
  `std::list<std::string>` (00419080), `std::list<Item12>` (FameWidget +0x294, a 12-byte struct).
- `std::vector<Piece*>` (Board), `std::vector<Effect*>` pools (EffectMgr), `std::vector<int>` (BookWormApp
  +0x12fc), `std::vector<std::string>` (word lists, SelectUserDialog, OptionsDialog), `std::vector<Sexy::Color>`
  (ColorizedButton +0xe0).
- Streams: `std::fstream` + `std::getline(stream, string, delim)` in FameWidget vf04_sub88 (00426c10);
  the old `ifstream` + `getline(char*, 4096)` in Definitionator::Load (0041d920); `operator<<` to a
  standard ostream (DAT_0056be74, cerr or cout) in the word-list loader's catch block (then `exit(1)`).

##### Tables (one per family)

Column "game callers" = distinct callers inside 0x401000-0x437800 (call-site count from callees.md in
parentheses when it differs); the sample prefers callers that are not themselves STL helpers.
"(STL-internal only)" = only other library helpers in the game region call it; "fw/unwind only" = no
direct game caller (framework, EH unwind funclets or vtables).

###### std::string (std::basic_string<char>) (44)

address | name (proposed) | C++ it stands for | game callers (# distinct, sample) | Big Money | notes
---|---|---|---|---|---
00401360 | `std::char_traits<char>::assign` | `static void char_traits<char>::assign(char& dst, const char& src)` | 2: 00416180 0041cf10 (STL-internal only) | 004010c0 std::char_traits<char>::assign (call) | dst = src (1 byte)
00401580 | `std::string::compare_cstr` | `int compare(size_type pos, size_type n, const char* p, size_type len) const` | 13: 00405ea0 0040abd0 0041d920 0041e620 | 00419780 std::string::compare_cstr (call) | memcmp-style compare then length tie-break; throws _Xran if pos > size()
00401750 | `std::string::find_first_not_of_ptr` | `size_type find_first_not_of(const char* set, size_type pos, size_type setlen) const` | 3: 004122e0 0041d920 | 0045bd40 (exact) | was `FUN_00401750`; memchr per char; returns npos (-1) if none
004017e0 | `std::string::_Eos` | `void _Eos(size_type n)  // size = n, buf[n] = 0` | 0 (fw/unwind only) | 00402520 std::string::_Eos (call) | 
00401810 | `std::string::_Tidy` | `void _Tidy(bool built)  // free heap buffer if built && cap>15, then empty (cap 15)` | 0 (fw/unwind only) | 00402540 std::string::_Tidy (call) | only 1 stack arg in this build
00401850 | `std::string::max_size` | `size_type max_size() const  // 0xfffffffe` | 1: 00423b60 (STL-internal only) |  | was `FUN_00401850`; used by std::getline (00423b60)
00401860 | `std::string::compare_cstr0` | `int compare(const char* p) const` | 2: 00427650 00427f30 |  | was `FUN_00401860`; strlen loop + compare_cstr(0,size,p,len); Ghidra shows it void (result in EAX)
00401890 | `std::string::_Tidy0` | `_Tidy(false)  // reset to empty without freeing (cap=15,size=0,buf[0]=0)` | 0 (fw/unwind only) | 004025d0 std::string::_Tidy0 (exact) | also the inline default ctor body
004018b0 | `std::string::find_first_not_of_cstr` | `size_type find_first_not_of(const char* set, size_type pos = 0) const` | 1: 00402190 |  | was `FUN_004018b0`
004018e0 | `std::string::_Grow_alloc` | `_Copy(size_type newsize) part 1: new buffer of (newsize\|15)+1 bytes` | 5: 00401a40 00401b10 00401c10 00401cf0 (STL-internal only) | 00402670 std::string::_Grow_alloc (exact) | Ghidra splits _Copy at the EH try; continues in 0040195e; 1.5x growth branch exists in _Copy
0040195e | `std::string::_Grow_copy` | `_Copy(size_type) part 2: copy old chars, free old heap buffer, install new` | 1: 004018e0 (STL-internal only) | 004026ee std::string::_Grow_copy (exact) | chunk of _Copy (not a real function)
00401a00 | `std_string_equals_cstr` | `bool operator==(const string& s, const char* p)` | 2: 00402190 00417080 | 0041c200 std_string_equals_cstr (call) | 
00401a40 | `std::string::_Grow` | `bool _Grow(size_type n, bool trim)` | 3: 00401dd0 00401e90 00423220 (STL-internal only) | 004030b0 std::string::_Grow (call) | throws _Xlen if n==npos
00401b10 | `std_string_append_substr` | `string& append(const string& s, size_type pos, size_type n)  // += s with (s,0,npos)` | 8: 00411cd0 00412da0 00416090 004241d0 | 00412cb0 std_string_append_substr (call) | 
00401c10 | `std::string::erase` | `string& erase(size_type pos = 0, size_type n = npos)` | 15: 00405fa0 00406180 00408ab0 004122e0 | 00403ba0 std::string::erase (call) | memmove tail down
00401cf0 | `std::string::append_n_chars` | `string& append(size_type n, char c)` | 17: 00405fa0 00406180 00406a10 00406b00 | 00403180 std::string::append_n_chars (call) | also used by resize()
00401dd0 | `std::string::assign_substr` | `string& assign(const string& s, size_type pos, size_type n)` | 71: 00402090 00402a50 00406560 004068a0 | 00403c60 std::string::assign_substr (call) | (s,0,npos) == operator=(const string&) / copy-ctor body; the most common string call (206 callers)
00401e90 | `std::string::assign_ptr_len` | `string& assign(const char* p, size_type n)` | 91: 00402430 00406950 004069c0 00406b00 | 00403d70 std::string::assign_ptr_len (call) | preceded by an inline strlen loop it is s = "literal" / string(const char*)
00401f40 | `std_string_append_chars` | `string& append(const char* p, size_type n)` | 8: 00406c40 004078b0 00408ab0 00412190 | 00412da0 std_string_append_chars (call) | with a preceding strlen loop = operator+=(const char*)
00402060 | `std::string::string_copy` | `string(const string& s)` | 2: 00417080 00432720 | 00403e40 std::string::string_copy (call) | 
004020d0 | `std::string::string_cstr` | `string(const char* p)` | 15: 00402190 00407c60 0040bd40 00417080 | 00404150 std::string::string_cstr (call) | 
00402110 | `std::string::assign_str` | `string& operator=(const string& s)  // assign(s,0,npos)` | 1: 0040bd40 |  | was `FUN_00402110`; BM 00404190 assign_str is the same template
00402130 | `std::string::assign_cstr` | `string& operator=(const char* p)` | 4: 00402a50 00426c10 00431080 00433e80 | 00403ea0 std::string::assign_cstr (call) | 
00402160 | `std::string::append_cstr` | `string& operator+=(const char* p)  // append(p, strlen(p))` | 2: 00406c40 0042a170 |  | was `FUN_00402160`
004029a0 | `std::string_plus_cstr` | `string operator+(const string& a, const char* b)` | 26: 00402a50 00406c40 004078b0 00408ab0 | 00419800 std::string_plus_cstr (call) | returns into hidden result (param_1)
00404c50 | `std::string::operator_index` | `char& operator[](size_type pos)  // &_Myptr()[pos], unchecked` | 1: 00427650 | 004231d0 (exact) | was `FUN_00404c50`; also used by EditWidget::ProcessKey
00404c70 | `std::string::_Myptr` | `const char* c_str() / data() / _Myptr()` | 2: 00416180 0041cf10 (STL-internal only) | 00402510 std::string::_Myptr (exact) | cap>15 ? heap ptr : inline buffer at +4
00404dd0 | `std::string::compare_str` | `int compare(const string& s) const` | 1: 0041e620 |  | was `FUN_00404dd0`; Ghidra shows void; result in EAX
00405e70 | `std::string::resize` | `void resize(size_type n)  // pads with '\0'` | 1: 00407c60 | 00403e70 std::string::resize (call) | 
00405f10 | `std::string_plus_string` | `string operator+(const string& a, const string& b)` | 15: 00406c40 004078b0 00408ab0 00411ea0 | 004198b0 std::string_plus_string (call) | 
00406800 | `std_string_concat_cstr_str` | `string operator+(const char* a, const string& b)` | 22: 0040f5b0 00412ec0 00413cb0 00414fa0 | 0041c6e0 std_string_concat_cstr_str (call) | 
00411100 | `std::string::find_last_not_of_ptr` | `size_type find_last_not_of(const char* set, size_type pos, size_type setlen) const` | 2: 004122e0 0041d920 |  | was `FUN_00411100`; pos=npos means from the end
00411c60 | `std::string::_Tidy_free` | `~string() body as used in EH unwind funclets (free heap buffer only)` | 2: 00412d66 00416996 | 00403e10 std::string::_Tidy_free (call) | callers are Catch/Unwind blocks
00412360 | `std_string_substr` | `string substr(size_type pos = 0, size_type n = npos) const` | 3: 00412da0 0041d920 00427650 | 0041c5f0 std_string_substr (call) | result via hidden param_1
0041cb80 | `std::string::find_first_of_ptr` | `size_type find_first_of(const char* set, size_type pos, size_type setlen) const` | 1: 0041d920 | 0043df30 (exact) | was `FUN_0041cb80`
004224a0 | `std::string::begin` | `iterator begin()  // returned via hidden pointer` | 1: 004237d0 (STL-internal only) | 0043dbd0 (exact) | was `FUN_004224a0`; used by basic_filebuf and the framework
004224c0 | `std::string::end` | `iterator end()` | 0 (fw/unwind only) | 0043dbf0 (exact) | was `FUN_004224c0`
00423220 | `std::string::assign_n_chars` | `string& assign(size_type n, char c)` | 1: 00423390 (STL-internal only) | 0043fcf0 (exact) | was `FUN_00423220`
00423390 | `std::string::string_n_chars` | `string(size_type n, char c)` | 0 (fw/unwind only) | 004403f0 (exact) | was `FUN_00423390`; Ghidra hides the cap/size init stores
004233c0 | `std::string::erase_range` | `iterator erase(iterator first, iterator last)` | 5: 004262b0 004264f0 00437430 00437480 | 0045f030 (exact) | was `FUN_004233c0`; result iterator via hidden param_1
00423550 | `std::string::erase_all` | `s.erase(s.begin(), s.end())  // == clear()` | 1: 00427650 |  | was `FUN_00423550`
0047f470 | `std_string_Tidy` | `~string() == _Tidy(true): free heap buffer if cap>15, then cap=15,size=0,buf[0]=0` | 17: 00402190 00406c40 00407c60 0040bd40 | 0045d100 std_string_Tidy (exact) | framework region; 1416 callers (every std::string destructor that was not inlined)
004cab34 | `std::_String_base::_Xran` | `[[noreturn]] void _Xran()  // throw out_of_range("invalid string position")` | 4: 00401580 00401b10 00401c10 00401dd0 (STL-internal only) | 004c8ce4 std::out_of_range::ctor_or_dtor_4c8ce4 (call) | old name std::out_of_range::ctor_or_dtor_4c8ce4; called from compare/erase/assign/append/substr when pos > size()
004cab74 | `std::_String_base::_Xlen` | `[[noreturn]] void _Xlen()  // throw length_error("string too long")` | 6: 00401a40 00401b10 00401c10 00401cf0 (STL-internal only) | 004c8d24 std::length_error::ctor_or_dtor_4c8d24 (call) | old name std::length_error::ctor_or_dtor_4c8d24

###### std::list (24)

address | name (proposed) | C++ it stands for | game callers (# distinct, sample) | Big Money | notes
---|---|---|---|---|---
00404da0 | `std::for_each_list_ptr` | `Fn for_each(list<T*>::iterator first, list<T*>::iterator last, Fn f)  // f is a plain function pointer` | 7: 00405cf0 004081d0 00408700 00409c60 |  | was `FUN_00404da0`; e.g. delete-all helpers (callers Board)
00405090 | `std::list<T*>::begin` | `iterator begin()  // head->_Next (null-safe), via hidden pointer` | 3: 00408700 00409de0 0040bd40 |  | was `FUN_00405090`
004050c0 | `std::list<T*>::front` | `T& front()  // &head->_Next->_Myval` | 1: 00409de0 |  | was `FUN_004050c0`
004050e0 | `std::list<T*>::_Buynode` | `_Nodeptr _Buynode(_Nodeptr next, _Nodeptr prev, const T& val)  // new node(0xc)` | 18: 00408ab0 00409970 00409b70 00409c60 | 0045cbd0 (exact) | was `FUN_004050e0`; thiscall, `this` unused; ICF-shared by all 4-byte-value lists; callers inline the link + _Incsize(1) = push_back/insert
00405da0 | `std::list<T*>::~list` | `~list()  // erase(begin,end) + free head` | 0 (fw/unwind only) | 0047b120 (call) | was `FUN_00405da0`; called from EH unwind funclets (31 callers)
004066e0 | `std::list<T*>::_Incsize` | `void _Incsize(size_type n)  // throw length_error("list<T> too long") if > max_size (0x3fffffff), else size += n` | 11: 00408ab0 00409970 00409b70 00409c60 |  | old name std::length_error::ctor_or_dtor_4066e0; Board.obj copy (each .obj has its own literal, so copies are not folded)
00408a70 | `std::list<T*>::push_back` | `void push_back(const T& val)` | 1: 0040a160 |  | was `FUN_00408a70`; copy instantiated in Board.obj (uses _Incsize 004066e0)
00411440 | `std::list<std::string>::_Buyhead` | `_Nodeptr _Buyhead()  // node 0x24` | 1: 00419080 | 00479ab0 (exact) | was `FUN_00411440`; also used by the framework (InternetManager, SexyApp)
004114c0 | `std::list<std::string>::_Destroyval` | `node->_Myval.~string()  // string at node+8` | 1: 00412820 (STL-internal only) | 0045df10 (exact) | was `FUN_004114c0`; used by erase/clear
00412420 | `std::list<std::string>::_Buynode` | `_Nodeptr _Buynode(_Nodeptr next, _Nodeptr prev, const string& val)  // node 0x24 = next,prev,string` | 2: 00419080 | 00461230 (exact) | was `FUN_00412420`
004124c0 | `std::list<std::string>::_Incsize` | `void _Incsize(size_type n)  // max_size 0x9249249` | 2: 00419080 | 004612d0 std::length_error::ctor_or_dtor_4612d0 (group) | old name std::length_error::ctor_or_dtor_4612d0
00412780 | `std::list<T*>::_Incsize_2` | `void _Incsize(size_type n)  // list of 4-byte values` | 3: 0041a8a0 00435b50 |  | old name std::length_error::ctor_or_dtor_412780; BookWormApp.obj copy
00412820 | `std::list<std::string>::erase` | `iterator erase(iterator where)` | 0 (fw/unwind only) | 00440580 (group) | was `FUN_00412820`; unlink/--size hidden by Ghidra
00416a90 | `std::list<std::string>::push_back` | `void push_back(const string& val)` | 0 (fw/unwind only) |  | was `FUN_00416a90`
00416bf0 | `std::list<T*>::push_back_2` | `void push_back(const T& val)` | 1: 00417080 |  | was `FUN_00416bf0`; copy in BookWormApp.obj (uses _Incsize 00412780)
00420400 | `std::list<T*>::_Incsize_3` | `void _Incsize(size_type n)  // list of 4-byte values` | 3: 004207f0 00420830 00420b70 |  | old name std::length_error::ctor_or_dtor_420400; EffectMgr.obj copy
00422c90 | `std::list<Item12>::_Buyhead` | `_Nodeptr _Buyhead()  // node 0x14: list of a 12-byte struct` | 1: 00428520 |  | was `FUN_00422c90`; FameWidget list at +0x294 (element type name unknown; "Item12" is a placeholder)
00422dd0 | `std::list<Item12>::_Buynode` | `_Nodeptr _Buynode(next, prev, const Item12& val)  // copies 3 dwords` | 1: 00426ab0 |  | was `FUN_00422dd0`
00423e50 | `std::list<Item12>::_Incsize` | `void _Incsize(size_type n)  // max_size 0x15555555` | 1: 00426ab0 |  | old name std::length_error::ctor_or_dtor_423e50; FameWidget.obj
0042ee00 | `std::list<T*>::_Buyhead` | `_Nodeptr _Buyhead()  // new node(0xc), next = prev = self` | 10: 0040a160 0040ae90 0040bd40 0041a0f0 | 00479a90 (exact) | was `FUN_0042ee00`; ICF-shared by every list of 4-byte values (pointers/ints): Board, EffectMgr, FlyingPieceList, OptionsDialog, BookWormApp, WidgetManager
0042f3b0 | `std::list<T*>::_Incsize_4` | `void _Incsize(size_type n)  // list of 4-byte values` | 1: 0042fe90 |  | old name std::length_error::ctor_or_dtor_42f3b0; OptionsDialog.obj copy
004347b0 | `std::list<T*>::erase_range` | `iterator erase(iterator first, iterator last)  // (begin,end) == clear()` | 23: 004058d0 00405a80 00405b00 00405cf0 | 0043fca0 (exact) | was `FUN_004347b0`; Ghidra hides the unlink and --size; nodes freed one by one
00489140 | `std::list<std::string>::erase_range` | `iterator erase(iterator first, iterator last)` | 1: 00419080 | 00461070 (call) | was `FUN_00489140`; framework region (BM 00461070); game caller 00419080
0048b330 | `std::list<std::string>::~list` | `~list()` | 1: 00419080 | 004610d0 (call) | was `FUN_0048b330`; framework region (BM 004610d0); game caller 00419080

###### std::set / std::multiset / std::map (_Tree) (77)

address | name (proposed) | C++ it stands for | game callers (# distinct, sample) | Big Money | notes
---|---|---|---|---|---
00404c80 | `std::set<std::string>::_Lrotate` | `void _Lrotate(_Nodeptr where)` | 2: 00408230 00412870 (STL-internal only) | 0045c180 (exact) | was `FUN_00404c80`; isnil byte +0x29
00404ce0 | `std::set<std::string>::_Rrotate` | `void _Rrotate(_Nodeptr where)` | 2: 00408230 00412870 (STL-internal only) | 0045c1e0 (exact) | was `FUN_00404ce0`
00404d40 | `std::set<std::string>::iterator::_Dec` | `void const_iterator::_Dec()  // operator--` | 1: 00408920 (STL-internal only) | 0045c240 (exact) | was `FUN_00404d40`
004065c0 | `std::set<std::string>::_Buynode` | `_Nodeptr _Buynode(_Nodeptr left, _Nodeptr parent, _Nodeptr right, const string& val, char color)  // node 0x2c` | 1: 00408230 (STL-internal only) | 00461720 (exact) | was `FUN_004065c0`; node: left,parent,right, string +0xc, color +0x28, isnil +0x29
00408230 | `std::set<std::string>::_Insert` | `iterator _Insert(bool addleft, _Nodeptr where, const string& val)  // + red-black rebalance` | 1: 00408920 (STL-internal only) |  | old name std::length_error::ctor_or_dtor_408230 (throws "map/set<T> too long")
00408920 | `std::set<std::string>::insert` | `pair<iterator,bool> insert(const string& val)  // unique: no-op if present` | 2: 0040abd0 0041a5b0 |  | was `FUN_00408920`; the dictionary word set (0041a5b0) and Board 0040abd0
00410b40 | `std::multiset<HighScoreEntry>::_Max` | `static _Nodeptr _Max(_Nodeptr)` | 1: 00416180 (STL-internal only) | 00478960 (exact) | was `FUN_00410b40`; isnil +0x4d
00410b60 | `std::multiset<HighScoreEntry>::_Min` | `static _Nodeptr _Min(_Nodeptr)` | 1: 00416180 (STL-internal only) | 00478980 (exact) | was `FUN_00410b60`
00410b80 | `std::multiset<HighScoreEntry>::_Rrotate` | `void _Rrotate(_Nodeptr)` | 2: 00416180 00416cf0 (STL-internal only) | 004789a0 (exact) | was `FUN_00410b80`
00410be0 | `std::multiset<HighScoreEntry>::iterator::_Inc` | `void iterator::_Inc()  // operator++` | 8: 00412ec0 00419cd0 004249d0 00424c30 | 00478a00 (exact) | was `FUN_00410be0`; FameWidget and BookWormApp walk the score tables with it
00410c40 | `std::set<std::string>::iterator::_Inc` | `void const_iterator::_Inc()  // operator++` | 1: 00412870 (STL-internal only) | 0045bb60 (exact) | was `FUN_00410c40`
00411190 | `std::multiset<HighScoreEntry>::_Lrotate` | `void _Lrotate(_Nodeptr)` | 2: 00416180 00416cf0 (STL-internal only) | 00479360 (exact) | was `FUN_00411190`
00411460 | `std::multiset<HighScoreEntry>::_Buyhead` | `_Nodeptr _Buyhead()  // node 0x50` | 2: 0041a0f0 | 00479b40 (exact) | was `FUN_00411460`; HighScoreEntry = 0x40 bytes {string name +0, string word +0x1c, int score +0x38, bool +0x3c} (struct name invented)
004114f0 | `std::set<std::string>::_Destroyval` | `node->_Myval.~string()  // string at node+0xc` | 1: 00412c40 (STL-internal only) | 0045d040 (exact) | was `FUN_004114f0`; also used by framework set<string> code
00411520 | `std::pair<const_std::string,std::string>::~pair` | `~pair()  // destroy strings at +0 and +0x1c` | 6: 00411570 004115d0 00419080 00427f30 | 0047b060 (exact) | was `FUN_00411520`; ICF-shared: also ~HighScoreEntry and any {string,string,...} struct (e.g. the user record freed in 00434880); 45 callers
00412080 | `HighScoreEntry::HighScoreEntry_copy` | `HighScoreEntry(const HighScoreEntry&)  // implicit copy-ctor: 2 strings + int + bool` | 12: 00412ec0 004249d0 00424c30 00425880 |  | was `FUN_00412080`; compiler-generated; Ghidra hides the int/bool copy (+0x38,+0x3c); struct name invented
00412560 | `std::multiset<HighScoreEntry,ByScore>::_Lbound` | `_Nodeptr _Lbound(const HighScoreEntry& key)  // lower_bound` | 1: 00416ad0 (STL-internal only) |  | was `FUN_00412560`; compares with ByScore (004115d0, entries passed BY VALUE)
00412600 | `std::multiset<HighScoreEntry,ByWordLength>::_Lbound` | `_Nodeptr _Lbound(const HighScoreEntry& key)` | 1: 00416b60 (STL-internal only) |  | was `FUN_00412600`; compares with ByWordLength (00411570)
00412870 | `std::set<std::string>::erase` | `iterator erase(iterator where)  // throws out_of_range("invalid map/set<T> iterator") on end()` | 1: 00416f70 (STL-internal only) |  | old name std::out_of_range::ctor_or_dtor_412870
00412ba0 | `std::multiset<HighScoreEntry,ByScore>::_Erase` | `void _Erase(_Nodeptr root)` | 1: 00416c30 (STL-internal only) |  | was `FUN_00412ba0`; recursive (not folded with 00412bf0 because it calls itself)
00412bf0 | `std::multiset<HighScoreEntry,ByWordLength>::_Erase` | `void _Erase(_Nodeptr root)` | 1: 00416eb0 (STL-internal only) |  | was `FUN_00412bf0`
00412c40 | `std::set<std::string>::_Erase` | `void _Erase(_Nodeptr root)  // recursive subtree delete` | 1: 00416f70 (STL-internal only) |  | was `FUN_00412c40`
00416180 | `std::multiset<HighScoreEntry>::erase` | `iterator erase(iterator where)` | 3: 00419cd0 |  | old name std::out_of_range::ctor_or_dtor_416180; shared by both predicates; game caller 00419cd0
00416850 | `std::multiset<HighScoreEntry>::_Buynode` | `_Nodeptr _Buynode(left, parent, right, const HighScoreEntry& val, char color)  // node 0x50, isnil +0x4d` | 1: 00416cf0 (STL-internal only) | 00482d90 (call) | was `FUN_00416850`
00416ad0 | `std::multiset<HighScoreEntry,ByScore>::find` | `iterator find(const HighScoreEntry& key)` | 2: 00418bd0 00419cd0 |  | was `FUN_00416ad0`
00416b60 | `std::multiset<HighScoreEntry,ByWordLength>::find` | `iterator find(const HighScoreEntry& key)` | 2: 00418bd0 00419cd0 |  | was `FUN_00416b60`
00416c30 | `std::multiset<HighScoreEntry,ByScore>::erase_range` | `iterator erase(iterator first, iterator last)` | 2: 004187b0 |  | was `FUN_00416c30`; BookWormApp tables at +0x117c,+0x1188,+0x1194
00416cf0 | `std::multiset<HighScoreEntry>::_Insert` | `iterator _Insert(bool addleft, _Nodeptr where, const HighScoreEntry& val)` | 2: 00417ef0 00417fb0 (STL-internal only) | 00483b90 std::length_error::ctor_or_dtor_483b90 (exact) | old name std::length_error::ctor_or_dtor_483b90; shared by both predicates
00416eb0 | `std::multiset<HighScoreEntry,ByWordLength>::erase_range` | `iterator erase(iterator first, iterator last)` | 2: 004187b0 |  | was `FUN_00416eb0`; BookWormApp table at +0x11a0
00416f70 | `std::set<std::string>::erase_range` | `iterator erase(iterator first, iterator last)  // (begin,end) -> _Erase(root)` | 2: 004187b0 |  | was `FUN_00416f70`
00417ef0 | `std::multiset<HighScoreEntry,ByScore>::insert` | `iterator insert(const HighScoreEntry& val)  // multiset: always inserts (upper position)` | 4: 00418bd0 00419080 00419cd0 00427550 |  | was `FUN_00417ef0`; callers 00418bd0 00419080 00419cd0 00427550
00417fb0 | `std::multiset<HighScoreEntry,ByWordLength>::insert` | `iterator insert(const HighScoreEntry& val)` | 3: 00418bd0 00419080 00419cd0 |  | was `FUN_00417fb0`; predicate inlined here
004186f0 | `std::multiset<HighScoreEntry,ByScore>::~multiset` | `~multiset()` | 0 (fw/unwind only) |  | was `FUN_004186f0`; EH unwind callers
00418730 | `std::multiset<HighScoreEntry,ByWordLength>::~multiset` | `~multiset()` | 0 (fw/unwind only) |  | was `FUN_00418730`; EH unwind callers
00418770 | `std::set<std::string>::~set` | `~set()  // erase(begin,end) + free head` | 0 (fw/unwind only) |  | was `FUN_00418770`; EH unwind callers; BookWormApp dtor inlines it (+0x1260, +0x127c)
0041a0b0 | `std::multiset<HighScoreEntry>::multiset` | `multiset()  // _Buyhead + size 0` | 0 |  | was `FUN_0041a0b0`; no direct callers (inlined in BookWormApp ctor 0041a0f0)
0041c9e0 | `std::map<std::string,std::string>::_Lrotate` | `void _Lrotate(_Nodeptr)` | 2: 0041cf10 0041d4b0 (STL-internal only) | 004182b0 (call) | was `FUN_0041c9e0`
0041ca40 | `std::map<std::string,std::string>::_Rrotate` | `void _Rrotate(_Nodeptr)` | 2: 0041cf10 0041d4b0 (STL-internal only) | 00418310 (call) | was `FUN_0041ca40`
0041caa0 | `std::map<std::string,std::string>::_Max` | `static _Nodeptr _Max(_Nodeptr)` | 1: 0041cf10 (STL-internal only) |  | was `FUN_0041caa0`
0041cac0 | `std::map<std::string,std::string>::_Min` | `static _Nodeptr _Min(_Nodeptr)` | 1: 0041cf10 (STL-internal only) |  | was `FUN_0041cac0`
0041cae0 | `std::map<std::string,std::string>::iterator::_Inc` | `void iterator::_Inc()  // operator++` | 1: 0041cf10 (STL-internal only) | 00418370 (exact) | was `FUN_0041cae0`
0041cc10 | `std::map<std::string,std::string>::iterator::_Dec` | `void iterator::_Dec()  // operator--` | 1: 0041d670 (STL-internal only) | 0045b7c0 (exact) | was `FUN_0041cc10`
0041cc70 | `std::map<std::string,std::string>::_Buyhead` | `_Nodeptr _Buyhead()  // node 0x48` | 3: 0041dd30 00432500 | 004183f0 (exact) | was `FUN_0041cc70`; node: left,parent,right, key string +0xc, value string +0x28, color +0x44, isnil +0x45; Definitionator (+8) and the framework DefinesMap
0041ccb0 | `std::map<std::string,std::string>::_Destroyval` | `node->_Myval.~pair()  // strings at node+0xc and node+0x28` | 3: 00412ba0 00412bf0 0041d260 (STL-internal only) | 00418430 (exact) | was `FUN_0041ccb0`; ICF-shared with multiset<HighScoreEntry> (same two-string layout at +0xc)
0041cd00 | `std::map<std::string,std::string>::_Lbound` | `_Nodeptr _Lbound(const string& key)  // lower_bound` | 1: 0041cdc0 (STL-internal only) | 0045e480 (exact) | was `FUN_0041cd00`
0041cdc0 | `std::map<std::string,std::string>::find` | `iterator find(const string& key)` | 1: 0041d2b0 | 0045f930 (group) | was `FUN_0041cdc0`; game caller: Definitionator lookup 0041d2b0
0041ce30 | `std::pair<const_std::string,std::string>::pair` | `pair(const string& key, const string& value)` | 1: 0041d7c0 (STL-internal only) | 0045f9a0 (call) | was `FUN_0041ce30`; used by operator[] (0041d7c0)
0041cea0 | `std::pair<const_std::string,std::string>::pair_copy` | `pair(const pair&)  // value_type copy` | 1: 0041d350 (STL-internal only) | 00424c50 (exact) | was `FUN_0041cea0`
0041cf10 | `std::map<std::string,std::string>::erase` | `iterator erase(iterator where)` | 1: 0041d3f0 (STL-internal only) |  | old name std::out_of_range::ctor_or_dtor_41cf10
0041d260 | `std::map<std::string,std::string>::_Erase` | `void _Erase(_Nodeptr root)` | 2: 004323c5 |  | was `FUN_0041d260`
0041d350 | `std::map<std::string,std::string>::_Buynode` | `_Nodeptr _Buynode(left, parent, right, const value_type& val, char color)` | 2: 0041d4b0 00432330 (STL-internal only) | 00418e00 (exact) | was `FUN_0041d350`
0041d3f0 | `std::map<std::string,std::string>::erase_range` | `iterator erase(iterator first, iterator last)` | 3: 0041d8d0 00432500 | 00418ea0 (call) | was `FUN_0041d3f0`; also used by the framework (RegisterDialog/OpenRegisterPage)
0041d4b0 | `std::map<std::string,std::string>::_Insert` | `iterator _Insert(bool addleft, _Nodeptr where, const value_type& val)` | 1: 0041d670 (STL-internal only) | 00468840 std::length_error::ctor_or_dtor_468840 (exact) | old name std::length_error::ctor_or_dtor_468840
0041d670 | `std::map<std::string,std::string>::insert` | `pair<iterator,bool> insert(const value_type& val)` | 1: 0041d7c0 (STL-internal only) | 004692f0 (exact) | was `FUN_0041d670`; framework callers too (004718c0 0048a810 0048df70)
0041d7c0 | `std::map<std::string,std::string>::operator_index` | `string& operator[](const string& key)  // insert(value_type(key, string())).first->second` | 1: 0041d920 | 0046b9f0 (exact) | was `FUN_0041d7c0`; Definitionator::Load 0041d920: defs[word] = def
0041d890 | `std::map<std::string,std::string>::~map` | `~map()` | 1: 004324e5 | 00419130 (call) | was `FUN_0041d890`
0042a9c0 | `std::map<int,bool>::_Buyhead` | `_Nodeptr _Buyhead()  // node 0x18` | 1: 0042b480 (STL-internal only) | 00479c90 (exact) | was `FUN_0042a9c0`; node: left,parent,right, int key +0xc, bool value +0x10, color +0x14, isnil +0x15; static map at DAT_00534c38 (HintDialog)
0042aa00 | `std::map<int,bool>::_Erase` | `void _Erase(_Nodeptr root)` | 1: 0042b320 (STL-internal only) |  | was `FUN_0042aa00`
0042aa40 | `std::map<int,bool>::find` | `iterator find(const int& key)` | 1: 0042b3e0 | 0047b0b0 (exact) | was `FUN_0042aa40`; game caller 0042b3e0
0042aab0 | `std::map<int,bool>::_Insert` | `iterator _Insert(bool addleft, _Nodeptr where, const value_type& val)` | 1: 0042b260 (STL-internal only) |  | old name std::length_error::ctor_or_dtor_42aab0
0042ac70 | `std::map<int,bool>::erase` | `iterator erase(iterator where)` | 1: 0042b320 (STL-internal only) |  | old name std::out_of_range::ctor_or_dtor_42ac70
0042b260 | `std::map<int,bool>::insert` | `pair<iterator,bool> insert(const value_type& val)` | 1: 0042b3e0 |  | was `FUN_0042b260`; game caller 0042b3e0
0042b320 | `std::map<int,bool>::erase_range` | `iterator erase(iterator first, iterator last)` | 1: 0042b440 (STL-internal only) |  | was `FUN_0042b320`
0042b440 | `std::map<int,bool>::~map` | `~map()  // registered with atexit` | 0 |  | was `FUN_0042b440`
0042b480 | `std::map<int,bool>::map` | `map()  // static-init of DAT_00534c38` | 0 |  | was `FUN_0042b480`; no direct callers (static initializer)
00432330 | `std::map<std::string,std::string>::_Copy_node` | `_Nodeptr _Copy(_Nodeptr src, _Nodeptr parent)  // recursive` | 1: 004323e0 (STL-internal only) | 00418f60 (exact) | was `FUN_00432330`
004323e0 | `std::map<std::string,std::string>::_Copy` | `void _Copy(const _Tree& right)  // root copy + leftmost/rightmost fix-up` | 1: 00432470 (STL-internal only) | 00419010 (call) | was `FUN_004323e0`
00432470 | `std::map<std::string,std::string>::map_copy` | `map(const map& right)  // DefinesMap copy-ctor` | 1: 00432500 | 004190a0 (exact) | was `FUN_00432470`; callers RegisterDialog vf04_sub88 (00432500) and SexyApp::OpenRegisterPage
00466eb0 | `std::set<std::string>::_Max` | `static _Nodeptr _Max(_Nodeptr)  // rightmost` | 1: 00412870 (STL-internal only) | 0045ba90 (exact) | was `FUN_00466eb0`; framework region; shared with the framework set<string> (0046b150)
00466f50 | `std::set<std::string>::_Min` | `static _Nodeptr _Min(_Nodeptr)  // leftmost` | 1: 00412870 (STL-internal only) | 0045bab0 (exact) | was `FUN_00466f50`; framework region
0047ddc0 | `std::map<int,Sexy::Dialog*>::_Rrotate` | `void _Rrotate(_Nodeptr)` | 2: 0042aab0 0042ac70 (STL-internal only) | 00478a60 (exact) | was `FUN_0047ddc0`; framework region; ICF-shared with std::map<int,bool> (0042aab0/0042ac70); framework type inferred (SexyApp dialog map: vf57/vf58)
0047de20 | `std::map<int,Sexy::Dialog*>::_Max` | `static _Nodeptr _Max(_Nodeptr)` | 1: 0042ac70 (STL-internal only) | 00478920 (exact) | was `FUN_0047de20`; ICF-shared (int key, 4-byte value)
0047de40 | `std::map<int,Sexy::Dialog*>::_Min` | `static _Nodeptr _Min(_Nodeptr)` | 1: 0042ac70 (STL-internal only) | 00478940 (exact) | was `FUN_0047de40`; ICF-shared
0047de60 | `std::map<int,Sexy::Dialog*>::iterator::_Dec` | `void iterator::_Dec()` | 1: 0042b260 (STL-internal only) | 00478ac0 (exact) | was `FUN_0047de60`; ICF-shared; game caller 0042b260
0047dec0 | `std::map<int,Sexy::Dialog*>::iterator::_Inc` | `void iterator::_Inc()` | 1: 0042ac70 (STL-internal only) | 00478b80 (exact) | was `FUN_0047dec0`; ICF-shared
0047e3d0 | `std::map<int,Sexy::Dialog*>::_Lrotate` | `void _Lrotate(_Nodeptr)` | 2: 0042aab0 0042ac70 (STL-internal only) | 00479300 (exact) | was `FUN_0047e3d0`; ICF-shared
0047e9f0 | `std::map<int,Sexy::Dialog*>::_Buynode` | `_Nodeptr _Buynode(left, parent, right, const value_type& val, char color)  // node 0x18, copies 8 bytes` | 1: 0042aab0 (STL-internal only) | 00479cd0 (exact) | was `FUN_0047e9f0`; ICF-shared

###### std::vector (23)

address | name (proposed) | C++ it stands for | game callers (# distinct, sample) | Big Money | notes
---|---|---|---|---|---
00403ee0 | `std::vector<std::string>::size` | `size_type size() const  // (last-first)/28` | 1: 00418160 (STL-internal only) | 00401cd0 std::vector<std::string>::size (exact) | 
00406780 | `std::vector<T>::_Xlen` | `[[noreturn]] void _Xlen()  // throw length_error("vector<T> too long")` | 1: 004083f0 (STL-internal only) |  | old name std::length_error::ctor_or_dtor_406780; Board.obj copy
004083f0 | `std::vector<T*>::_Insert_n` | `void _Insert_n(iterator where, size_type n, const T& val)  // grows to max(cap + cap/2, size + n)` | 1: 0040ae20 (STL-internal only) |  | was `FUN_004083f0`; Board.obj copy (vector<Piece*>); uses _Xlen 00406780
0040ae20 | `std::vector<T*>::push_back` | `void push_back(const T& val)` | 2: 0040b9e0 0040ed70 |  | was `FUN_0040ae20`; Board (vector<Piece*>; callers 0040b9e0 0040ed70)
004123a0 | `std::vector<T>::_Xlen_2` | `[[noreturn]] void _Xlen()` | 4: 004164d0 00416530 00416590 00418160 (STL-internal only) |  | old name std::length_error::ctor_or_dtor_4123a0; BookWormApp.obj copy
004164d0 | `std::vector<int>::_Buy` | `bool _Buy(size_type n)  // operator new(n*4), throws _Xlen if n > 0x3fffffff` | 0 (fw/unwind only) | 00406ec0 std::vector<int>::_Buy (call) | ICF-shared by all 4-byte vectors
00416530 | `std::vector<std::string>::_Buy` | `bool _Buy(size_type n)  // operator new(n*28), max 0x9249249` | 1: 00417dc0 (STL-internal only) | 00406f20 std::vector<std::string>::_Buy (call) | 
00416590 | `std::vector<int>::_Insert_n_2` | `void _Insert_n(iterator where, size_type n, const T& val)` | 1: 00417d50 (STL-internal only) |  | was `FUN_00416590`; BookWormApp.obj copy; uses _Xlen 004123a0
00417d50 | `std::vector<int>::push_back_2` | `void push_back(const int& val)` | 1: 004184d0 |  | was `FUN_00417d50`; BookWormApp vector<int> at +0x12fc (004184d0)
00417dc0 | `std::vector<std::string>::vector_copy` | `vector(const vector& right)` | 2: 00418590 00432f00 | 00429e00 (exact) | was `FUN_00417dc0`; callers 00418590 00432f00 (SelectUserDialog)
00417e70 | `std::vector<std::string>::_Tidy` | `void _Tidy()  // destroy all + free  (== ~vector, clear+shrink)` | 2: 00417e50 00432f00 | 00406fa0 std::vector<std::string>::_Tidy (call) | 30 callers
00418160 | `std::vector<std::string>::_Insert_n` | `void _Insert_n(iterator where, size_type n, const string& val)` | 1: 00418680 (STL-internal only) | 0042a7b0 (call) | was `FUN_00418160`
00418680 | `std::vector<std::string>::insert` | `iterator insert(iterator where, const string& val)` | 1: 0041a020 (STL-internal only) | 0042af10 (exact) | was `FUN_00418680`
0041a020 | `std::vector<std::string>::push_back` | `void push_back(const string& val)` | 3: 0041a5b0 0042fe90 00432f00 | 0042afc0 (exact) | was `FUN_0041a020`; callers 0041a5b0 0042fe90 00432f00
0041b5d0 | `std::vector<T>::_Xlen_4` | `[[noreturn]] void _Xlen()` | 2: 0041ba50 0041bab0 (STL-internal only) | 004254a0 std::length_error::ctor_or_dtor_4254a0 (call) | old name std::length_error::ctor_or_dtor_4254a0; ColorizedButton.obj copy
0041ba50 | `std::vector<Sexy::Color>::_Buy` | `bool _Buy(size_type n)  // n*16, max 0xfffffff` | 0 (fw/unwind only) |  | was `FUN_0041ba50`; ColorizedButton colour list at +0xe0
0041bab0 | `std::vector<Sexy::Color>::_Insert_n` | `void _Insert_n(iterator where, size_type n, const Color& val)` | 1: 0041bd90 (STL-internal only) | 00428380 (group) | was `FUN_0041bab0`; BM 00428380 (group)
0041bd90 | `std::vector<Sexy::Color>::insert` | `iterator insert(iterator where, const Color& val)` | 1: 0041bde0 (STL-internal only) |  | was `FUN_0041bd90`
0041bde0 | `std::vector<Sexy::Color>::push_back` | `void push_back(const Color& val)` | 2: 0041be50 0041bed0 |  | was `FUN_0041bde0`; ColorizedButton vf49/vf50 (SetColors)
0041e310 | `std::vector<std::string>::erase` | `iterator erase(iterator where)  // copy down, destroy last` | 0 (fw/unwind only) | 00428f70 (exact) | was `FUN_0041e310`; no direct callers recorded (DeleteUserDialog path)
004204a0 | `std::vector<T>::_Xlen_3` | `[[noreturn]] void _Xlen()` | 1: 00420550 (STL-internal only) |  | old name std::length_error::ctor_or_dtor_4204a0; EffectMgr.obj copy
00420550 | `std::vector<T*>::_Insert_n_3` | `void _Insert_n(iterator where, size_type n, const T& val)` | 1: 00420b00 (STL-internal only) |  | was `FUN_00420550`; EffectMgr.obj copy (vector<Effect*>); uses _Xlen 004204a0
00420b00 | `std::vector<T*>::push_back_3` | `void push_back(const T& val)` | 4: 00420d60 00420e70 00420fd0 00421070 |  | was `FUN_00420b00`; EffectMgr/EmberListMgr effect pools (vector<Effect*>)

###### Algorithms and uninitialized-memory helpers (25)

address | name (proposed) | C++ it stands for | game callers (# distinct, sample) | Big Money | notes
---|---|---|---|---|---
00405110 | `std::copy_backward_ptr` | `T* copy_backward(T* first, T* last, T* destEnd)  // 4-byte T` | 3: 0040858f 0041672f 004206ef | 0043f3d0 (exact) | was `FUN_00405110`; BM 0043f3d0; result via hidden param_1
004111f0 | `std::_Uninit_copy_ptr` | `T* uninitialized_copy(T* first, T* last, T* dest)  // 4-byte T` | 4: 004083f0 00411c00 00416590 00420550 (STL-internal only) | 0043e8b0 (exact) | was `FUN_004111f0`; BM 0043e8b0; Ghidra hides the store
004114a0 | `std::fill_ptr` | `void fill(T* first, T* last, const T& val)  // 4-byte T` | 3: 0040858f 0041672f 004206ef | 0043f3b0 (exact) | was `FUN_004114a0`; BM 0043f3b0; Ghidra hides the store
00411c00 | `std::uninitialized_copy_ptr` | `wrapper of 004111f0 (vector::_Ucopy)` | 3: 0040858f 0041672f 004206ef |  | was `FUN_00411c00`; called from catch funclets
00411c30 | `std::uninitialized_fill_n_ptr` | `T* _Ufill(T* dest, size_type n, const T& val)  // returns dest+n` | 3: 0040858f 0041672f 004206ef | 00440420 (exact) | was `FUN_00411c30`
00411c90 | `std::_Destroy_strings` | `void _Destroy_range(string* first, string* last)  // ~string each` | 9: 00418590 0041e370 0041e620 0042fc90 | 00403ed0 std::_Destroy_strings (call) | 21 callers
00412c90 | `std::fill_strings` | `void fill(string* first, string* last, const string& val)` | 1: 00418417 | 00425620 (exact) | was `FUN_00412c90`; BM 00425620
00412cc0 | `std::_Copy_backward_strings` | `string* copy_backward(string* first, string* last, string* destEnd)` | 1: 004168f0 (STL-internal only) | 00425690 (exact) | was `FUN_00412cc0`; BM 00425690
00412d00 | `std::_Uninit_copy_strings` | `string* uninitialized_copy(const string* first, const string* last, string* dest)` | 4: 00417030 00417050 00417dc0 00418160 (STL-internal only) | 00407530 std::_Uninit_copy_strings (call) | string ctor init hidden by Ghidra
00416830 | `std::_Destroy_strings_wrap` | `wrapper of 00411c90` | 2: 00418358 00418417 |  | was `FUN_00416830`; catch funclets
004168f0 | `std::copy_backward_strings` | `wrapper of 00412cc0 returning the iterator` | 1: 00418417 | 00423c10 (group) | was `FUN_004168f0`
00416930 | `std::_Uninit_fill_n_strings` | `void uninitialized_fill_n(string* dest, size_type n, const string& val)` | 3: 00417ec0 00418160 0041a020 (STL-internal only) | 00428d20 (exact) | was `FUN_00416930`; BM 00428d20
00417030 | `std::_Uninit_copy_strings_wrap` | `wrapper of 00412d00 (vector<string>::_Ucopy)` | 0 (fw/unwind only) |  | was `FUN_00417030`
00417050 | `std::_Uninit_copy_strings_wrap_2` | `wrapper of 00412d00` | 2: 00418358 00418417 |  | was `FUN_00417050`; catch funclets
00417ec0 | `std::_Ufill_strings` | `string* _Ufill(string* dest, size_type n, const string& val)  // returns dest+n` | 1: 00418358 | 00429eb0 (exact) | was `FUN_00417ec0`; BM 00429eb0
0041b480 | `std::_Copy_backward_Color` | `Color* copy_backward(Color* first, Color* last, Color* destEnd)` | 1: 0041b4d0 (STL-internal only) | 0045c630 (exact) | was `FUN_0041b480`; BM 0045c630
0041b4d0 | `std::copy_backward_Color` | `wrapper of 0041b480` | 1: 0041bc5f | 00428ce0 (group) | was `FUN_0041b4d0`
0041b570 | `std::uninitialized_copy_Color` | `wrapper of 004990f0` | 1: 0041bc5f | 0045e9c0 (call) | was `FUN_0041b570`
0041b5a0 | `std::_Ufill_Color` | `Color* _Ufill(Color* dest, size_type n, const Color& val)  // returns dest+n` | 1: 0041bc5f | 0045f770 (exact) | was `FUN_0041b5a0`
0041dfe0 | `std::copy_strings` | `string* copy(string* first, string* last, string* dest)` | 2: 0041e620 | 00425650 (exact) | was `FUN_0041dfe0`; BM 00425650; used by vector<string>::erase
0041f900 | `std::_Uninit_fill_n_ptr` | `void uninitialized_fill_n(T* dest, size_type n, const T& val)  // 4-byte T` | 7: 004083f0 0040ae20 00411c30 00416590 (STL-internal only) | 0043f4c0 (exact) | was `FUN_0041f900`; BM 0043f4c0; Ghidra hides the store
00422e10 | `std::transform_chars` | `char* transform(const char* first, const char* last, char* dest, int (*f)(int))` | 1: 00422f20 | 0045c550 (exact) | was `FUN_00422e10`; only use: 00422f20 lowercases a string in place with CRT tolower
00499030 | `std::fill_Color` | `void fill(Color* first, Color* last, const Color& val)` | 1: 0041bc5f | 0045c590 (exact) | was `FUN_00499030`; framework region; Ghidra drops the first dword store (disassembly copies all 4)
004990b0 | `std::_Uninit_fill_n_Color` | `void uninitialized_fill_n(Color* dest, size_type n, const Color& val)` | 3: 0041b5a0 0041bab0 0041bde0 (STL-internal only) | 00423cb0 (exact) | was `FUN_004990b0`; framework region; Ghidra drops dword 0
004990f0 | `std::_Uninit_copy_Color` | `Color* uninitialized_copy(const Color* first, const Color* last, Color* dest)` | 2: 0041b570 0041bab0 (STL-internal only) | 004233c0 (exact) | was `FUN_004990f0`; framework region; Ghidra drops dword 0

###### iostreams and locale (Dinkumware std:: and the old LIBCI ifstream) (122)

address | name (proposed) | C++ it stands for | game callers (# distinct, sample) | Big Money | notes
---|---|---|---|---|---
00411350 | `std::basic_ostream<char>::flush` | `ostream& flush()  // badbit if pubsync()==-1` | 3: 004113b0 00411670 00422ad0 (STL-internal only) | 0043ece0 (exact) | was `FUN_00411350`
004113b0 | `std::basic_ostream<char>::sentry::sentry` | `sentry(ostream& os)  // lock + flush tie()` | 2: 004116e0 00411970 (STL-internal only) | 0045ca20 (exact) | was `FUN_004113b0`
00411670 | `std::basic_ostream<char>::sentry::~sentry` | `~sentry()  // if (!uncaught_exception()) _Osfx(); unlock` | 0 (fw/unwind only) | 0045df40 (exact) | was `FUN_00411670`
004116e0 | `std::operator<<_cstr` | `ostream& operator<<(ostream& os, const char* s)  // with width padding` | 1: 0041a790 | 0045e730 (exact) | was `FUN_004116e0`; game caller: dictionary loader catch 0041a790 (std ostream global DAT_0056be74)
004118ad | `std::operator<<_cstr_tail` | `chunk of 004116e0` | 1: 004116e0 (STL-internal only) | 0045e8fd (exact) | was `FUN_004118ad`
00411970 | `std::operator<<_char` | `ostream& operator<<(ostream& os, char c)` | 1: 0041a790 |  | was `FUN_00411970`; game caller: 0041a790
0041c9c0 | `ifstream::vbase_dtor` | ``vbase destructor' of old ifstream (~ifstream + ~ios)` | 0 (fw/unwind only) |  | was `FUN_0041c9c0`; EH unwind of 0041d920
0041cb40 | `ios::lock` | `inline void ios::lock()  // if (LockFlg < 0) _mtlock(&x_lock)` | 0 (fw/unwind only) |  | was `FUN_0041cb40`; old iostream inline instantiated in game obj
0041cb60 | `ios::unlock` | `inline void ios::unlock()` | 0 (fw/unwind only) |  | was `FUN_0041cb60`
00421710 | `std::_Locinfo::_Getctype` | `_Ctypevec _Getctype() const` | 0 (fw/unwind only) | 0042b050 (exact) | was `FUN_00421710`
00421750 | `std::locale::id::operator_unsigned_int` | `locale::id::operator size_t()  // lazily numbers facet ids` | 0 (fw/unwind only) | 0042b090 std::locale::id::operator_unsigned_int (exact) | BM name kept
00421790 | `std::locale::facet::_Incref` | `void _Incref()  // under _Lockit` | 2: 004228c0 004232a0 (STL-internal only) | 0042b0d0 (exact) | was `FUN_00421790`
004217c0 | `std::locale::facet::_Decref` | `facet* _Decref()  // returns this when the count hits 0` | 1: 00421800 (STL-internal only) | 0042b100 (exact) | was `FUN_004217c0`
00421800 | `std::locale::~locale` | `~locale()  // delete _Ptr->_Decref()` | 0 (fw/unwind only) | 0042b140 (exact) | was `FUN_00421800`; 77 callers
00421820 | `std::locale::facet::vf00` | `facet scalar deleting dtor` | 0 | 0042b160 std::locale::facet::vf00 (fuzzy:1.00) | BM name kept
00421c70 | `std::_Locinfo::_Getctype_2` | `second copy of _Getctype (result discarded)` | 0 (fw/unwind only) | 0042b1b0 (exact) | (inferred) was `FUN_00421c70`
00421e90 | `std::ios_base::getloc` | `locale getloc() const` | 1: 00422ad0 (STL-internal only) | 0042b1f0 (exact) | was `FUN_00421e90`
00421ed0 | `std::ios_base::vf00` | `ios_base scalar deleting dtor` | 0 | 0042b230 std::ios_base::vf00 (fuzzy:1.00) | BM name kept
004220d0 | `std::basic_ios<char>::~basic_ios` | `~basic_ios() -> ios_base::~ios_base()` | 6: 00422140 00422780 004227c0 00422830 (STL-internal only) | 00437650 (call) | was `FUN_004220d0`
004220e0 | `std::basic_ostream<char>::~basic_ostream` | `dtor body (only resets the vftable at the virtual base)` | 0 (fw/unwind only) | 00437660 (exact) | (inferred) was `FUN_004220e0`; istream or ostream variant
00422100 | `std::basic_iostream<char>::dtor_adjust` | `this-adjusting dtor helper` | 1: 00422830 (STL-internal only) | 0045b6a0 (exact) | (inferred) was `FUN_00422100`
00422140 | `std::basic_ios<char>::scalar_deleting_dtor` | `vtable slot 0` | 0 |  | was `FUN_00422140`
00422170 | `std::basic_streambuf<char,std::char_traits<char>_>::setg` | `void setg(char* first, char* next, char* last)` | 1: 00423a10 (STL-internal only) | 004377c0 std::basic_streambuf<char,std::char_traits<char>_>::setg (exact) | BM name kept
004221a0 | `std::basic_streambuf<char>::getloc` | `locale getloc() const` | 1: 00426990 (STL-internal only) | 00437890 (exact) | was `FUN_004221a0`
004221e0 | `std::ctype<char>::ctype` | `ctype(const _Locinfo&, size_t refs)  // copies the CRT ctype table` | 1: 004223c0 (STL-internal only) | 00437ba0 (exact) | (inferred) was `FUN_004221e0`
004222a0 | `std::ctype<char>::do_tolower` | `virtual char do_tolower(char)` | 0 | 00437c60 (group) | was `FUN_004222a0`; BM 00437c60 group
004222c0 | `std::ctype<char>::do_tolower_range` | `virtual const char* do_tolower(char*, const char*)` | 0 | 00437c80 (group) | was `FUN_004222c0`
004222f0 | `std::ctype<char>::do_toupper` | `virtual char do_toupper(char)` | 0 | 00437cb0 (group) | was `FUN_004222f0`
00422310 | `std::ctype<char>::do_toupper_range` | `virtual const char* do_toupper(char*, const char*)` | 0 | 00437cd0 (group) | was `FUN_00422310`
00422340 | `std::ctype<char>::do_widen` | `virtual char do_widen(char)` | 0 | 00437d00 (exact) | was `FUN_00422340`
00422350 | `std::ctype<char>::do_widen_range` | `virtual const char* do_widen(const char*, const char*, char*)` | 0 | 00437d10 (exact) | was `FUN_00422350`
00422380 | `std::ctype<char>::do_narrow` | `virtual char do_narrow(char, char)` | 0 | 00437d40 (exact) | was `FUN_00422380`
00422390 | `std::ctype<char>::do_narrow_range` | `virtual const char* do_narrow(const char*, const char*, char, char*)` | 0 | 00437d50 (exact) | was `FUN_00422390`
004223c0 | `std::ctype<char>::_Getcat` | `static size_t _Getcat(const facet** pp)  // new ctype<char>(_Locinfo())` | 1: 004228c0 (STL-internal only) | 00437d80 (exact) | was `FUN_004223c0`
00422430 | `std::ctype<char>::scalar_deleting_dtor` | `vtable slot 0` | 0 |  | was `FUN_00422430`
00422460 | `std::ctype<char>::~ctype` | `~ctype()  // free table if owned` | 1: 00422430 (STL-internal only) | 00437e20 std::locale::facet::ctor_or_dtor_437e20 (exact) | was `std::locale::facet::ctor_or_dtor_437e20`; BM name std::locale::facet::ctor_or_dtor_437e20
004224f0 | `std::basic_istream<char>::_Sentry_base::~_Sentry_base` | `~_Sentry_base()  // rdbuf()->_Unlock()` | 0 (fw/unwind only) | 0043dc20 (exact) | was `FUN_004224f0`; EH unwind callers
00422510 | `std::basic_streambuf<char>::~basic_streambuf` | `~basic_streambuf()  // delete _Plocale, ~_Mutex` | 2: 00422800 00423d40 (STL-internal only) | 0043dc40 (exact) | was `FUN_00422510`
004225a0 | `std::basic_streambuf<char>::uflow` | `virtual int_type uflow()  // default` | 0 | 0043dce0 (exact) | was `FUN_004225a0`
004225d0 | `std::basic_streambuf<char>::xsgetn` | `virtual streamsize xsgetn(char*, streamsize)` | 0 | 0043dd10 (exact) | was `FUN_004225d0`
00422670 | `std::basic_streambuf<char>::xsputn` | `virtual streamsize xsputn(const char*, streamsize)` | 0 | 0043ddb0 (exact) | was `FUN_00422670`
00422720 | `std::basic_streambuf<char>::seekoff` | `virtual pos_type seekoff(...)  // returns -1` | 0 | 0043de60 (exact) | was `FUN_00422720`
00422750 | `std::basic_streambuf<char>::seekpos` | `virtual pos_type seekpos(...)  // returns -1` | 0 | 0043de90 (exact) | was `FUN_00422750`
00422780 | `std::basic_istream<char>::scalar_deleting_dtor` | `vtable slot 0 (vbase-adjusted)` | 0 | 0045bcd0 (exact) | (inferred) was `FUN_00422780`
004227c0 | `std::basic_ostream<char>::scalar_deleting_dtor` | `vtable slot 0 (vbase-adjusted)` | 0 | 0043dec0 (exact) | (inferred) was `FUN_004227c0`
00422800 | `std::basic_streambuf<char>::scalar_deleting_dtor` | `vtable slot 0` | 0 |  | was `FUN_00422800`
00422830 | `std::basic_iostream<char>::scalar_deleting_dtor` | `vtable slot 0` | 0 | 0045bd10 (exact) | (inferred) was `FUN_00422830`
00422860 | `std::basic_streambuf<char>::_Init` | `void _Init()  // point get/put pointers at the internal slots, all null` | 6: 00422d60 00423050 00423b00 004267f0 (STL-internal only) | 0043e080 (exact) | was `FUN_00422860`; Ghidra shows an empty body; read the disassembly
004228c0 | `std::use_facet<std::ctype<char>>` | `const ctype<char>& use_facet<ctype<char>>(const locale&)  // throws bad_cast` | 1: 00422ad0 (STL-internal only) | 0043e1e0 (call) | was `FUN_004228c0`
004229b0 | `std::codecvt<char,char,int>::codecvt` | `codecvt(const _Locinfo&, size_t refs)` | 1: 00422e50 (STL-internal only) | 0043e3b0 (exact) | (inferred) was `FUN_004229b0`
00422a20 | `std::codecvt<char,char,int>::do_in_out` | `virtual result do_in/do_out(...)  // returns noconv (3)` | 0 | 0043e420 (exact) | (inferred) was `FUN_00422a20`; ICF-folded do_in/do_out/do_unshift
00422a50 | `std::codecvt<char,char,int>::do_length` | `virtual int do_length(state&, const char* f, const char* l, size_t max)` | 0 | 0043e450 (exact) | (inferred) was `FUN_00422a50`
00422a70 | `std::basic_streambuf<char>::snextc` | `int_type snextc()` | 2: 00422ad0 00423b60 (STL-internal only) | 0043ed40 (exact) | was `FUN_00422a70`
00422ad0 | `std::basic_istream<char>::_Ipfx` | `bool _Ipfx(bool noskip)  // flush tie(), skip whitespace via ctype<char>` | 1: 00422eb0 (STL-internal only) | 0043eda0 (call) | was `FUN_00422ad0`
00422c38 | `std::basic_istream<char>::_Ipfx_tail` | `chunk of 00422ad0` | 1: 00422ad0 (STL-internal only) | 0043ef08 (exact) | was `FUN_00422c38`
00422cb0 | `std::basic_istream<char>::basic_istream` | `basic_istream(streambuf* sb, bool isstd)` | 0 |  | was `FUN_00422cb0`
00422d60 | `std::basic_streambuf<char>::basic_streambuf` | `basic_streambuf()  // new _Mutex, new locale, _Init()` | 2: 00423050 00426b10 (STL-internal only) | 0043f000 (exact) | was `FUN_00422d60`
00422e50 | `std::codecvt<char,char,int>::_Getcat` | `static size_t _Getcat(const facet**)` | 1: 004232a0 (STL-internal only) | 0043f460 (exact) | was `FUN_00422e50`
00422eb0 | `std::basic_istream<char>::sentry::sentry` | `sentry(istream& in, bool noskip)  // lock rdbuf + _Ipfx` | 1: 00423b60 (STL-internal only) | 0043fa60 (exact) | was `FUN_00422eb0`
00422f80 | `std::basic_iostream<char>::basic_iostream` | `basic_iostream(streambuf* sb)  // basic_ios::init: fill=" ", badbit if sb==0` | 1: 00426b10 (STL-internal only) | 0045e150 (exact) | was `FUN_00422f80`
00423050 | `std::basic_filebuf<char>::basic_filebuf` | `basic_filebuf(FILE* f = 0)` | 0 |  | was `FUN_00423050`
004230c0 | `std::basic_filebuf<char>::setbuf` | `virtual streambuf* setbuf(char*, streamsize)  // setvbuf` | 0 | 0043fb40 (exact) | (inferred) was `FUN_004230c0`
00423110 | `std::basic_filebuf<char>::sync` | `virtual int sync()  // fflush` | 0 | 0043fb90 (exact) | (inferred) was `FUN_00423110`
00423140 | `std::basic_filebuf<char>::pbackfail` | `virtual int_type pbackfail(int_type)` | 0 | 0043fbc0 (exact) | (inferred) was `FUN_00423140`; calls ungetc
004231d0 | `std::basic_streambuf<char>::gptr` | `char* gptr() const` | 1: 00423140 (STL-internal only) | 0043fc50 (exact) | was `FUN_004231d0`
004231e0 | `std::basic_filebuf<char>::underflow` | `virtual int_type underflow()  // uflow + pbackfail` | 0 | 0043fc60 (exact) | (inferred) was `FUN_004231e0`
004232a0 | `std::use_facet<std::codecvt<char,char,int>>` | `const codecvt<char,char,int>& use_facet<...>(const locale&)` | 2: 00426940 00426990 (STL-internal only) | 00440040 (call) | was `FUN_004232a0`
00423420 | `std::basic_filebuf<char>::_Endwrite` | `bool _Endwrite()  // flush + codecvt unshift + fwrite` | 3: 00423900 00423a10 00423b00 (STL-internal only) | 00440450 (call) | was `FUN_00423420`
00423590 | `std::basic_filebuf<char>::overflow` | `virtual int_type overflow(int_type)` | 0 |  | (inferred) was `FUN_00423590`; from its fputc/fwrite use
004237d0 | `std::basic_filebuf<char>::uflow` | `virtual int_type uflow()  // getc` | 0 | 004410a0 (exact) | (inferred) was `FUN_004237d0`
00423900 | `std::basic_filebuf<char>::seekoff` | `virtual pos_type seekoff(off_type, seekdir, openmode)` | 0 | 004411d0 (exact) | (inferred) was `FUN_00423900`; calls fseek/fgetpos
00423a10 | `std::basic_filebuf<char>::seekpos` | `virtual pos_type seekpos(pos_type, openmode)` | 0 | 004412e0 (exact) | (inferred) was `FUN_00423a10`; calls fsetpos
00423b00 | `std::basic_filebuf<char>::close` | `basic_filebuf* close()  // _Endwrite + fclose` | 2: 00423d00 00423d40 (STL-internal only) | 00441430 (exact) | was `FUN_00423b00`
00423b60 | `std::getline_string` | `istream& getline(istream& in, string& s, char delim)  // Dinkumware: clears s, stops at delim (extracted) or EOF, failbit if nothing read` | 1: 00426c10 |  | was `FUN_00423b60`; FameWidget vf04_sub88; continues in chunk 00423c8e
00423c8e | `std::getline_string_tail` | `chunk of 00423b60 (setstate + sentry dtor)` | 1: 00423b60 (STL-internal only) |  | was `FUN_00423c8e`; not a real function
00423d00 | `std::basic_fstream<char>::close` | `void close()  // setstate(failbit) if rdbuf()->close() fails` | 1: 00426c10 |  | was `FUN_00423d00`; FameWidget vf04_sub88
00423d40 | `std::basic_filebuf<char>::~basic_filebuf` | `~basic_filebuf()  // close() if owned, delete _Str, ~streambuf` | 2: 00423e20 00426760 (STL-internal only) | 004423f0 (exact) | was `FUN_00423d40`
00423e20 | `std::basic_filebuf<char>::scalar_deleting_dtor` | `vtable slot 0` | 0 |  | was `FUN_00423e20`
00423ef0 | `std::locale::locale_facet` | `locale(const locale& loc, const Facet* fac)  // copy _Locimp + _Addfac` | 1: 004267f0 (STL-internal only) | 00444750 (exact) | was `FUN_00423ef0`
00426760 | `std::basic_fstream<char>::~basic_fstream` | `~basic_fstream()  // this = ios_base subobject; destroys filebuf` | 3: 00426c10 | 004685c0 (exact) | was `FUN_00426760`
004267f0 | `std::basic_filebuf<char>::_Initcvt` | `void _Initcvt(codecvt* cvt)` | 2: 00426940 00426990 (STL-internal only) | 00448bd0 (exact) | was `FUN_004267f0`
00426920 | `std::basic_fstream<char>::vbase_dtor` | ``vbase destructor' (~basic_fstream + ~basic_ios)` | 0 (fw/unwind only) | 00469160 (exact) | was `FUN_00426920`; EH unwind callers
00426940 | `std::basic_filebuf<char>::imbue` | `virtual void imbue(const locale&)` | 0 | 0044b610 (exact) | was `FUN_00426940`
00426960 | `std::basic_fstream<char>::scalar_deleting_dtor` | `vtable slot 0` | 0 | 00469670 (exact) | was `FUN_00426960`
00426990 | `std::basic_filebuf<char>::open` | `basic_filebuf* open(const char* name, ios_base::openmode mode)  // _Fiopen + _Init + _Initcvt` | 1: 00426b10 (STL-internal only) | 0044b8e0 (exact) | was `FUN_00426990`
00426b10 | `std::basic_fstream<char>::basic_fstream` | `basic_fstream(const char* name, ios_base::openmode mode, [vbase flag])` | 1: 00426c10 | 004694a0 (exact) | was `FUN_00426b10`; filebuf at +0xc, ios_base at +0x6c; sets badbit\|failbit if open fails; FameWidget vf04_sub88 + framework (OpenHTMLTemplate...)
004cabd6 | `std::ios_base::_Addstd` | `void _Addstd()  // register a standard stream (_Lockit 2)` | 1: 00422cb0 (STL-internal only) |  | was `FUN_004cabd6`
004cac6d | `std::ios_base::~ios_base` | `~ios_base()  // _Tidy + ~locale` | 3: 00426c10 | 004c95c9 std::ios_base::ctor_or_dtor_4c95c9 (exact) | old name std::ios_base::ctor_or_dtor_4c95c9
004cad55 | `std::ios_base::clear` | `void clear(iostate state, bool reraise)  // throws failure if state & exceptions()` | 14: 00411881 00411b0e 00422c0c 00423c62 | 004c96b1 std::ios_base::failure::ctor_or_dtor_4c96b1 (exact) | old name std::ios_base::failure::ctor_or_dtor_4c96b1; every setstate() call in the decompilation is this
004cae94 | `std::ios_base::_Init` | `void _Init()` | 2: 00422cb0 00422f80 (STL-internal only) | 004c97f0 std::ios_base::_Init (exact) | BM name kept
004caeda | `std::_Mutex::_Mutex` | `_Mutex()  // new CRITICAL_SECTION + InitializeCriticalSection` | 1: 00422d60 (STL-internal only) | 004c9836 (exact) | was `FUN_004caeda`
004caef2 | `std::_Mutex::~_Mutex` | `~_Mutex()` | 1: 00422510 (STL-internal only) | 004c984e (exact) | was `FUN_004caef2`
004caf07 | `std::_Mutex::_Lock` | `void _Lock()  // EnterCriticalSection` | 2: 004113b0 00422eb0 (STL-internal only) | 004c9863 (call) | was `FUN_004caf07`
004caf10 | `std::_Mutex::_Unlock` | `void _Unlock()  // LeaveCriticalSection` | 5: 00411670 004118ad 00411970 004224f0 (STL-internal only) | 004c986c (call) | was `FUN_004caf10`
004caf19 | `std::uncaught_exception` | `bool uncaught_exception()  // jmp 004dea4c` | 3: 00411670 004118ad 00411970 (STL-internal only) |  | was `thunk_FUN_004dea4c`; thunk
004cb067 | `std::_Getctype` | `_Ctypevec _Getctype()  // CRT: copy of the 256-entry ctype table` | 3: 00421710 00421c70 004221e0 (STL-internal only) | 004c8e58 (exact) | was `FUN_004cb067`
004cb119 | `std::_Tolower` | `int _Tolower(int c, const _Ctypevec*)` | 2: 004222a0 004222c0 (STL-internal only) | 004c8f0a (exact) | was `FUN_004cb119`
004cb21b | `std::_Getcvt` | `_Cvtvec _Getcvt()` | 1: 004229b0 (STL-internal only) | 004c900c (exact) | was `FUN_004cb21b`
004cb387 | `std::_Lockit::_Lockit` | `_Lockit(int kind)  // EnterCriticalSection(lock[kind&3])` | 13: 00421750 00421790 004217c0 00421e90 (STL-internal only) | 004c9178 (exact) | was `FUN_004cb387`; 75 callers (facet refcounts)
004cb3aa | `std::_Lockit::~_Lockit` | `~_Lockit()` | 13: 00421750 00421790 004217c0 00421e90 (STL-internal only) | 004c919b (exact) | was `FUN_004cb3aa`
004cb437 | `std::locale::_Getfacet` | `const facet* _Getfacet(size_t id) const` | 2: 004228c0 004232a0 (STL-internal only) | 004c9228 std::locale::_Getfacet (exact) | BM name kept
004cb4af | `std::locale::facet::_Register` | `void _Register()` | 2: 004228c0 004232a0 (STL-internal only) | 004c92a0 std::locale::facet::_Register (exact) | BM name kept
004cb56e | `std::_Locinfo::~_Locinfo` | `~_Locinfo()  // restore setlocale` | 2: 004221e0 004229b0 (STL-internal only) | 004c935f (exact) | was `FUN_004cb56e`
004cb62b | `std::_Locinfo::_Locinfo` | `_Locinfo(const char* name = "C")` | 2: 004221e0 004229b0 (STL-internal only) | 004c941c (exact) | was `FUN_004cb62b`
004cb75b | `std::locale::locale` | `locale()  // global locale copy` | 3: 00422d60 00423050 00426b10 (STL-internal only) | 004c954c std::locale::locale (exact) | BM name kept
004cb85c | `std::_Toupper` | `int _Toupper(int c, const _Ctypevec*)` | 2: 004222f0 00422310 (STL-internal only) | 004c995d (exact) | was `FUN_004cb85c`
004cb8e6 | `std::locale::_Locimp::_Addfac` | `void _Addfac(facet*, size_t id)` | 1: 00423ef0 (STL-internal only) | 004c99e7 std::locale::_Locimp::_Addfac (exact) | BM name kept
004cbc61 | `std::locale::_Locimp::_Locimp_copy` | `_Locimp(const _Locimp&)` | 1: 00423ef0 (STL-internal only) | 004c9d62 std::locale::_Locimp::ctor_or_dtor_4c9d62 (exact) | (inferred) old name std::locale::_Locimp::ctor_or_dtor_4c9d62
004cbd52 | `std::_Fiopen` | `FILE* _Fiopen(const char* name, ios_base::openmode mode)  // mode table -> fopen; ate -> fseek end` | 1: 00426990 (STL-internal only) | 004c9e9c (exact) | was `FUN_004cbd52`
004ee070 | `ios::ios` | `ios::ios()  // old iostream` | 0 (fw/unwind only) |  | was `ios::ctor_or_dtor_4ee070`
004ee0ce | `ios::~ios` | `ios::~ios()  // old iostream` | 2: 0041d920 |  | old name ios::ctor_or_dtor_4ee0ce; game callers 0041c9c0 0041d920
004ee163 | `ifstream::ifstream` | `ifstream::ifstream(const char* name, int mode = ios::in, int prot = filebuf::openprot)  // old iostream` | 1: 0041d920 |  | was `FUN_004ee163`; game caller 0041d920
004ee218 | `ifstream::~ifstream` | `ifstream::~ifstream()  // old iostream` | 2: 0041d920 |  | was `FUN_004ee218`
004ee22b | `ios::clear` | `void ios::clear(int state = 0)  // old iostream (lock/unlock around it)` | 0 (fw/unwind only) |  | was `FUN_004ee22b`
004ee247 | `ifstream::close` | `void ifstream::close()  // old iostream; failbit if filebuf::close fails` | 1: 0041d920 |  | was `FUN_004ee247`; game caller 0041d920
004ee2a6 | `_mtlockinit` | `void _mtlockinit(CRITICAL_SECTION*)` | 0 (fw/unwind only) | 004c9fae (call) | was `FUN_004ee2a6`; old iostream lock helper
004ee2b1 | `_mtlockterm` | `void _mtlockterm(CRITICAL_SECTION*)` | 0 (fw/unwind only) | 004c9fb9 (call) | was `FUN_004ee2b1`
004ee2bc | `_mtlock` | `void _mtlock(CRITICAL_SECTION*)  // EnterCriticalSection` | 2: 0041d920 | 004c9fc4 (call) | was `FUN_004ee2bc`; inline ios::lock() in 0041d920
004ee2c7 | `_mtunlock` | `void _mtunlock(CRITICAL_SECTION*)` | 2: 0041d920 | 004c9fcf (call) | was `FUN_004ee2c7`; inline ios::unlock()
004ee2d2 | `istream::unlockbuf` | `void unlockbuf()  // old iostream` | 0 (fw/unwind only) |  | (inferred) was `FUN_004ee2d2`
004ee2ea | `istream::isfx` | `void isfx()  // unlockbuf(); unlock()` | 0 (fw/unwind only) |  | (inferred) was `FUN_004ee2ea`
004ee30b | `istream::get_str` | `istream& istream::get(char* buf, int len, char delim)  // with _fGline set by inline getline(): extracts the delimiter` | 1: 0041d920 |  | was `FUN_004ee30b`; game caller 0041d920: getline(buf, 4096, '\n')

###### CRT (38)

address | name (proposed) | C++ it stands for | game callers (# distinct, sample) | Big Money | notes
---|---|---|---|---|---
00401540 | `operator_delete_thunk` | `void operator delete(void*) local wrapper -> free` | 0 (fw/unwind only) | 00401d00 operator_delete_thunk (call) | BM name kept
004cab10 | `operator_new` | `void* operator new(size_t)  // malloc loop + new_handler, throws via _Nomemory` | 89: 004010c0 004011f0 00401600 00401935 | 004c8cc0 operator_new (exact) | 224 callers
004caf1e | `operator_new_array` | `void* operator new[](size_t)  // jmp operator new` | 2: 00414a10 0041a5b0 |  | was `operator_new`; thunk; game callers 00414a10 0041a5b0
004da392 | `operator_delete` | `void operator delete(void*)  // jmp free` | 234: 00401720 004019cd 00402190 00402a50 |  | was `_free`; 234 callers: every "_free(p)" in string/container code is allocator::deallocate == operator delete
004da3a0 | `_memchr` | `void* memchr(const void*, int, size_t)` | 3: 00401750 00411100 0041cb80 (STL-internal only) | 004d9c20 _memchr (exact) | string find_* helpers
004da450 | `_memmove` | `void* memmove(void*, const void*, size_t)` | 1: 00401c10 (STL-internal only) | 004d81d0 _memmove (call) | string::erase
004da818 | `_ftol` | `int64 _ftol(long double st0)  // truncate toward zero; operand on the x87 stack, result EDX:EAX (game uses EAX)` | 40: 00402430 00402a50 00403c00 00404380 | 004d8798 _ftol (exact) | algorithm: fistp (round-nearest) then +-1 correction = C cast semantics
004da96d | `_atexit` | `int atexit(void (*)(void))` | 3: 004144b0 004146d0 00414a10 | 004d970a _atexit (exact) | registers dtors of function-local statics (004144b0 004146d0 00414a10)
004daa86 | `_fwrite` | `size_t fwrite(const void*, size_t, size_t, FILE*)` | 15: 00403050 00404e10 00404fe0 00406c40 | 004d8749 _fwrite (call) | save files: raw little-endian ints/bytes
004dabbb | `_fread` | `size_t fread(void*, size_t, size_t, FILE*)` | 12: 00403370 00405fa0 00406180 00408ab0 | 004d85f6 _fread (call) | load files
004dac07 | `_isalpha` | `int isalpha(int)  // CRT locale table, mask 0x103` | 4: 00403df0 00403e40 00427650 0042e5f0 |  | callers Board vf21/vf22 (virtual key codes), 00427650, 0042e5f0
004dad33 | `_isspace` | `int isspace(int)` | 1: 00427650 | 004daf8f _isspace (call) | caller 00427650
004daf70 | `_fclose` | `int fclose(FILE*)` | 5: 00406c40 00408ab0 0041a5b0 0041a790 | 004d810f _fclose (call) | 
004db01d | `fopen` | `FILE* fopen(const char*, const char*)` | 3: 00406c40 00408ab0 0041a5b0 | 004d81bc fopen (exact) | game callers 00406c40 00408ab0 0041a5b0
004db030 | `_errno` | `int* _errno()  // &_getptd()->_terrno` | 1: 00406c40 | 004dab4a (exact) | was `FUN_004db030`; game caller 00406c40 (after _mkdir)
004db0b5 | `sprintf` | `int sprintf(char*, const char*, ...)` | 3: 00407c60 00411220 00433900 | 004d8f75 sprintf (call) | game formats only %d / %s / %02d (no floats)
004db10d | `_time` | `time_t time(time_t*)  // 32-bit` | 1: 0040ae90 | 004d989c FID_conflict:__time32 (exact) | was `FID_conflict:__time32`; game caller 0040ae90
004db5a7 | `_toupper` | `int toupper(int)  // CRT locale ("C" unless setlocale)` | 2: 004112f0 00425880 | 004d9d9f _toupper (call) | callers 004112f0 (uppercase dictionary words), 00425880
004db5c9 | `_free` | `void free(void*)  // the real CRT free` | 4: 00412ec0 00419080 | 004d9e8d _free (call) | direct callers 00412ec0 00419080 (malloc/free pairs)
004db6e1 | `_malloc` | `void* malloc(size_t)` | 2: 00412ec0 00419080 | 004dc4aa _malloc (exact) | game callers 00412ec0 00419080
004db6f3 | `operator_delete_array` | `void operator delete[](void*)  // jmp operator delete` | 3: 004187b0 |  | was `_free`; callers 004187b0 004221e0 00422460
004db6f8 | `_atol` | `long atol(const char*)` | 0 (fw/unwind only) | 004d9e00 _atol (call) | 
004db780 | `_atoi` | `int atoi(const char*)  // jmp atol` | 2: 00419080 00427650 |  | was `_atol`; thunk; game callers 00419080 00427650
004db8ff | `exit` | `void exit(int)  // doexit(code, 0, 0)` | 1: 0041a790 | 004dc038 (exact) | was `FUN_004db8ff`; caller: dictionary-load catch 0041a790 (exit(1))
004db93f | `FID_conflict:_getc` | `int getc(FILE*)  // == fgetc` | 2: 0041a5b0 | 004da7fb FID_conflict:_getc (exact) | dictionary loader 0041a5b0 reads char by char
004db990 | `__chkstk` | `_alloca_probe / __chkstk  // stack probe for frames > 4 KB` | 1: 0041d920 | 004da420 __chkstk (exact) | 0041d920 has a 4096-byte line buffer
004dba95 | `_tolower` | `int tolower(int)` | 0 (fw/unwind only) | 004db1a3 _tolower (call) | address taken by 00422f20 (std::transform)
004dbab7 | `_setvbuf` | `int setvbuf(FILE*, char*, int, size_t)` | 1: 004230c0 (STL-internal only) | 004da45d _setvbuf (exact) | filebuf only
004dbcf3 | `_fflush` | `int fflush(FILE*)` | 1: 00423110 (STL-internal only) | 004da699 _fflush (exact) | filebuf only
004dbdb8 | `_ungetc` | `int ungetc(int, FILE*)` | 1: 00423140 (STL-internal only) | 004da75e _ungetc (exact) | filebuf only
004dbdfd | `FID_conflict:_fputc` | `int fputc(int, FILE*)` | 1: 00423590 (STL-internal only) | 004da7a3 FID_conflict:_fputc (exact) | filebuf only
004dbe55 | `_fgetpos` | `int fgetpos(FILE*, fpos_t*)` | 2: 00423900 00423a10 (STL-internal only) | 004da84b _fgetpos (exact) | filebuf only
004dbf06 | `_fseek` | `int fseek(FILE*, long, int)` | 2: 00423900 00423a10 (STL-internal only) | 004da8fc _fseek (exact) | filebuf only
004dbf4f | `_fsetpos` | `int fsetpos(FILE*, const fpos_t*)` | 1: 00423a10 (STL-internal only) | 004da945 _fsetpos (exact) | filebuf only
004dc087 | `_srand` | `void srand(unsigned seed)` | 0 (fw/unwind only) | 004d9365 (exact) | was `FUN_004dc087`; framework caller 0048aaf0 only
004dc094 | `_rand` | `int rand()  // holdrand = holdrand*214013 + 2531011; return (holdrand>>16) & 0x7fff` | 1: 0042dd20 | 004d9372 _rand (exact) | per-thread; seeded once by SexyApp::Init 0048aaf0 with srand(GetTickCount()); game caller Lex::vf17 0042dd20
004ed116 | `__stricmp` | `int _stricmp(const char*, const char*)  // ASCII fold to LOWER case in the "C" locale` | 5: 00408030 0040f5b0 00434800 004352e0 | 004eb2bc __stricmp (exact) | callers 00408030 0040f5b0 00434800 004352e0 004356d0
004eef12 | `_mkdir` | `int _mkdir(const char*)  // CreateDirectoryA, errno via __dosmaperr` | 1: 00406c40 | 004ec394 (exact) | was `FUN_004eef12`; caller 00406c40

###### EH / RTTI / exception classes (15)

address | name (proposed) | C++ it stands for | game callers (# distinct, sample) | Big Money | notes
---|---|---|---|---|---
00405200 | `std::logic_error::what` | `virtual const char* what() const` | 0 |  | was `std::logic_error::vf01`; vtable slot 1
00405210 | `std::logic_error::scalar_deleting_dtor` | `vtable slot 0` | 0 | 00418480 std::logic_error::vf00 (fuzzy:0.55) | was `std::logic_error::vf00`
00406240 | `std::logic_error::logic_error` | `logic_error(const string& what)  // exception() + string copy at +0xc` | 18: 004066e0 00406780 00408230 004123a0 (STL-internal only) | 004040f0 std::logic_error::ctor_or_dtor_4040f0 (call) | old name std::logic_error::ctor_or_dtor_4040f0; 63 callers (every throw helper)
004062a0 | `std::logic_error::~logic_error` | `~logic_error()` | 1: 00405210 (STL-internal only) | 004028e0 std::logic_error::ctor_or_dtor_4028e0 (exact) | old name std::logic_error::ctor_or_dtor_4028e0
00406670 | `std::logic_error::logic_error_copy` | `logic_error(const logic_error&)` | 2: 00407b70 00412b80 (STL-internal only) | 00447320 Sexy::D3DTester::TestException::ctor_or_dtor_447320 (group) | old name std::logic_error::ctor_or_dtor_406670
00407b70 | `std::length_error::length_error_copy` | `length_error(const length_error&)` | 0 |  | old name std::length_error::ctor_or_dtor_407b70 (copy-ctor used by the throw info)
00412b80 | `std::out_of_range::out_of_range_copy` | `out_of_range(const out_of_range&)` | 0 |  | old name std::out_of_range::ctor_or_dtor_412b80
004da7dd | `__CxxThrowException@8` | `throw expr  // _CxxThrowException(obj, ThrowInfo*)` | 36: 004019cd 0040664b 0040858f 0041249c | 004d8932 __CxxThrowException@8 (exact) | 36 game-region call sites: all container/string throw helpers
004dad6d | `exception::exception` | `exception()` | 1: 00406240 (STL-internal only) | 004d8dbe exception::ctor_or_dtor_4d8dbe (exact) | old name exception::ctor_or_dtor_4d8dbe
004dadbb | `exception::exception_copy` | `exception(const exception&)  // strdup of what()` | 1: 00406670 (STL-internal only) | 004d8e0c exception::ctor_or_dtor_4d8e0c (exact) | old name exception::ctor_or_dtor_4d8e0c
004dae05 | `exception::~exception` | `~exception()` | 1: 004062a0 (STL-internal only) | 004d8e56 exception::ctor_or_dtor_4d8e56 (exact) | old name exception::ctor_or_dtor_4d8e56
004dae28 | `bad_cast::bad_cast` | `bad_cast(const char* what = "bad cast")` | 2: 004228c0 004232a0 (STL-internal only) | 004d8e79 bad_cast::ctor_or_dtor_4d8e79 (exact) | old name bad_cast::ctor_or_dtor_4d8e79
004db3fe | `__RTDynamicCast` | `dynamic_cast<T*>(p)` | 1: 00411220 | 004d9285 __RTDynamicCast (exact) | game caller 00411220 (dynamic_cast to DDImage for the "32 Bit Only"/"PALLETIZED" debug dump)
004dbfc5 | ``eh_vector_destructor_iterator'` | `delete[]/array member destruction helper` | 1: 004270d0 | 004d9933 `eh_vector_destructor_iterator' (exact) | caller FameWidget ctor 004270d0
004dc025 | ``eh_vector_constructor_iterator'` | `array member construction helper` | 1: 00428520 | 004d9993 `eh_vector_constructor_iterator' (exact) | caller 00428520

##### Game-region functions met in the sweep that are NOT library code (left to the game helpers)

These wrap or inline STL but are game logic; they are not in the symbols file.

address | what it is
---|---
00402090 | returns a copy of the `std::string` member at +0x10 (a `GetName()`-style accessor, 17 callers)
004122e0 | trim helper: `s.erase(s.find_last_not_of(chars) + 1); s.erase(0, s.find_first_not_of(chars))` (caller 00417080)
004112f0 | in-place uppercase of a std::string with CRT `toupper` (dictionary words)
00422f20 | in-place lowercase: `std::transform(s.begin(), s.end(), s.begin(), tolower)` (caller FameWidget vf48)
0041d2b0 | Definitionator lookup: `map.find(word)`, returns the definition or "" (caller 0040f5b0)
0041a5b0 | word-list loader (fopen "r", getc, prefix-count digits, `toupper`, `set<string>::insert` / `vector<string>::push_back`, words >= 3 letters)
0041d920 | Definitionator::Load (old `ifstream`, `getline(buf, 4096)`, split at first space/tab, trim " \t\r\n", skip "*")
00434800 | walks a `std::list<User*>` comparing the user name with `_stricmp` (11 callers)
00434960 | ctor of a two-string record with both strings = the argument
004207f0, 00420830, 00426ab0, 004184d0 | game methods that inline a list/vector push_back
004115d0, 00411570 | the HighScoreEntry predicates (ByScore / ByWordLength, see above)
0042b3e0 | HintDialog static map query
00401570 (`ret 4`), 00403f10, 004220f0 (`ret 8`) | empty functions (ICF-folded no-ops), not library
00437540..004377c0 | WidgetManager/Widget framework code that happens to sit below 0x437800 (no game callers)

##### How to read the decompilation (and what to write in the port)

The port uses the same std containers (`SexyString` = `std::string`), so in almost every case the answer is
"write the obvious C++". Patterns, from most to least frequent:

1. **SSO buffer test**: `if (*(uint*)(s+0x18) < 0x10) p = s+4; else p = *(char**)(s+4);` (also written
   `0xf < cap`) is `s.c_str()` / `s.data()`. `*(int*)(s+0x14)` is `s.size()`; `== 0` is `s.empty()`.
2. **Destructor**: `if (0xf < cap) _free(ptr); cap = 0xf; size = 0; buf[0] = 0;` (or a call to
   `std_string_Tidy` 0047f470 / `std::string::_Tidy(s,1)`) is `~string()`. Nothing to write: let the
   object go out of scope. The same three stores without the free (`cap=0xf; size=0; buf[0]=0`) are the
   default constructor `std::string s;` (often directly followed by an assign = copy construction).
3. **Literal assignment**: `p = "abc"; do { c = *p++; } while (c); assign_ptr_len(s, "abc", p - ("abc"+1))`
   is `s = "abc"` (or `std::string s("abc")` after the ctor stores). The inline strlen loop also precedes
   `std_string_append_chars` (`s += "abc"`), `compare_cstr` (`s.compare("abc")`) and
   `std_string_concat_cstr_str` (`"abc" + s`).
4. **Copy**: `assign_substr(dst, src, 0, 0xffffffff)` is `dst = src` (copy-ctor when preceded by the ctor
   stores). `assign_substr(dst, src, pos, n)` / `std_string_substr(src, &tmp, pos, n)` are
   `dst.assign(src, pos, n)` / `src.substr(pos, n)`.
5. **Temporaries of operator+**: `std::string_plus_cstr(&tmp, a, "x")`, `std::string_plus_string(&tmp, a, b)`,
   `std_string_concat_cstr_str(&tmp, "x", a)` followed shortly by `if (0xf < tmp.cap) _free(tmp.ptr)` are
   `a + "x"`, `a + b`, `"x" + a`; chains of them are one expression (`"a" + s + "b"`). Write the expression.
6. **Comparisons**: `compare_cstr(s, 0, s.size, p, len) == 0` is `s == p`; `std_string_equals_cstr(s, p)`
   is `s == p`; `compare_str` / `compare_cstr0` return <0/0/>0 like `s.compare(...)`. In the map/set code the
   compare is inlined as a byte loop `(byte)*a < (byte)*b` over `min(len)` then a length tie-break: that is
   `std::less<std::string>`.
7. **`_free(p)` (004da392) is `operator delete`** (it is a 5-byte `jmp free`; I propose renaming it).
   `operator_new(n)` + constructor call + store = `new T(...)`; `if (p) { p->vtbl[0](1); }` = `delete p`.
   `004caf1e` = `new[]`, `004db6f3` = `delete[]`; the real `free` 004db5c9 / `malloc` 004db6e1 are only
   called directly by 00412ec0 and 00419080 (malloc/free pairs in game code).
8. **std::list**: object {alloc, head +4, size +8}; node {next +0, prev +4, value +8}.
   `for (n = *(int**)head; n != head; n = *n)` with `n[2]` = `for (auto it = l.begin(); it != l.end(); ++it)`
   with `*it`. `_Buynode(head, head->prev, &v)` + two pointer stores + `_Incsize(1)` = `l.push_back(v)`;
   `_Buynode(head->next, head, &v)` = `push_front`. `erase_range(l, &tmp, head->next, head)` = `l.clear()`.
   Ghidra often hides the unlink stores and `--size`; trust the call, not the body.
9. **set/map (_Tree)**: object {alloc, head +4, size +8}; node {left +0, parent +4, right +8, value +0xc,
   color, isnil}. `head->parent` (+4) is the root, `head->left` = `begin()`, `head->right` = rightmost. The
   **isnil byte** sits at `0xd + sizeof(value_type)` and is the loop terminator you see everywhere
   (`while (*(char*)(n + OFF) == 0)`): **+0x15** = `map<int,bool>` / `map<int,Dialog*>` (8-byte value),
   **+0x29** = `set<std::string>`, **+0x45** = `map<std::string,std::string>`, **+0x4d** =
   `multiset<HighScoreEntry>`. A descent loop comparing keys then `--`/compare = `insert`/`find`/`lower_bound`;
   `iterator::_Inc` / `_Dec` = `++it` / `--it`; `(node + 0x28)` returned from 0041d7c0 = `&m[key]`.
   Write `m[key]`, `m.find(key)`, `s.insert(v)`.
10. **std::vector**: {first +4, last +8, end +0xc}; `first == 0 ? 0 : (last - first) / sizeof(T)` is
    `v.size()` (the null test is part of VC7's size()); `first[i]` is `v[i]`. push_back is inlined as
    `if (first && size < capacity) { _Uninit_fill_n(last, 1, &v); last++; } else _Insert_n(v, last, 1, &v);`.
    Growth is 1.5x (`cap + cap/2`); libc++ doubles. Only matters if game code keeps raw pointers into a
    vector across a push_back (none seen).
11. **Throw helpers**: the functions named `std::length_error::ctor_or_dtor_*` / `std::out_of_range::ctor_or_dtor_*`
    in the game region are container functions that *contain* a throw (`list::_Incsize`, `vector::_Xlen`,
    `_Tree::_Insert`, `_Tree::erase(iterator)`), see the tables for the real names. The game never catches
    these; translate the container operation and drop the check. `std::ios_base::failure::ctor_or_dtor_4c96b1`
    (004cad55) is `ios_base::clear(state, reraise)`, i.e. every `setstate(failbit|badbit)`.
12. **EH noise**: `ExceptionList`, `puStack_c`, `local_8 = N` (EH state numbers) and the `Catch@`/`Unwind@`
    funclets are compiler bookkeeping for destructors on unwind; ignore them. `__chkstk` = a >4 KB stack frame.
13. **`_ftol()`** after x87 math: Ghidra shows `_ftol()` without an argument because the operand is on the
    FPU stack; read the disassembly (`fld`/`fmul`/`fild`...) for the expression. It is `(int)x`, truncation
    toward zero (C cast semantics; the routine rounds with `fistp` and corrects by +-1). It returns 64 bits
    in EDX:EAX and the game uses EAX, so values outside the int range wrap (low 32 bits) instead of
    saturating; no in-range difference.
14. **rand()** (004dc094) is the CRT LCG, NOT `Sexy::Rand` (00467070, MTRand at 0056add8). Only Lex::vf17
    (0042dd20: `rand() % 500`, `rand() % 4000`, `rand() & 1` idle animations) and the framework's
    OpenHTMLTemplate call it.
15. **Old iostream (no `std::`)**: `ifstream::ifstream`, `istream::get_str` (= `getline(buf, n)` when the
    `_fGline` member at +4 is incremented just before), `ios::lock/unlock` (inline critical sections) =
    `std::ifstream f(path); while (f.getline(buf, 4096)) ...` in the port.
16. **Ghidra hides stores** in several small helpers: the int/bool copy in 00412080, dword 0 of the Color
    copies (00499030/004990b0/004990f0), the whole body of `basic_streambuf::_Init` (00422860), the element
    store of `_Uninit_copy_ptr`/`_Uninit_fill_n_ptr`/`fill_ptr` (004111f0/0041f900/004114a0), the ctor stores
    of 00423390. Check the disassembly before concluding a helper "does nothing".

##### Behaviour the port must replicate exactly

- **CRT `rand()`**: `holdrand = holdrand * 214013 + 2531011; return (holdrand >> 16) & 0x7fff;` (RAND_MAX
  32767), per-thread, seeded once with `srand(GetTickCount())` in SexyApp::Init (0048aaf0). The sequence is
  not reproducible anyway (time seed), but keep the 15-bit range so `rand() % N` keeps the same
  distribution (Emscripten's RAND_MAX is 2^31-1): give the port a `CrtRand()` with the LCG above.
  Do not route it through Sexy::Rand (MTRand), which other game code uses and whose sequence matters for
  replays/state (MTRand::Serialize is saved by 0041faa0).
- **`_ftol` = truncation toward zero** (`(int)x`). Do not use `lround`/`floor` when translating. Caveat in
  the open questions about x87 precision.
- **`_stricmp`**: ASCII, folds to *lower* case before comparing (the "C" locale is never changed). Use a
  portable ASCII `stricmp` (lowercase fold), e.g. the framework's own helper if present; `strcasecmp` in
  the C locale gives the same result. Affects user-name matching (00434800, 004352e0, 004356d0) and
  00408030 / 0040f5b0.
- **`isalpha`/`isspace`/`toupper`/`tolower`**: "C" locale tables (ASCII only; bytes >= 0x80 are not alpha
  and are not case-mapped). 0042e5f0 passes the byte as unsigned (safe); 004112f0 passes a sign-extended
  `char`, harmless for the ASCII word list. Board vf21/vf22 call `isalpha` on Windows virtual-key codes
  ('A'..'Z' = 0x41..0x5a), so the port must feed them the same key codes.
- **`sprintf`/StrFormat formats** used by game code are only `%d`, `%s`, `%02d`: no float formatting
  differences to worry about.
- **`atoi`** (004db780, a jump to `atol`): skips leading whitespace, optional sign, decimal digits, no
  overflow check (wraps). `std::atoi` matches for in-range input.
- **std::getline (Dinkumware, 00423b60)**: standard semantics (erases the string, stops at the delimiter
  and extracts it, failbit only if nothing was extracted); libc++ is equivalent. The **old**
  `getline(char*, 4096)` (004ee30b) reads at most 4095 chars and, unlike std, sets no state bit when it stops at
  the length limit (the rest of the line is read as the next line); the file is opened in text mode so CRLF arrives as LF. In the port, open the definitions file in
  binary and strip a trailing `\r` (0041d920 already trims " \t\r\n" from the definition, but the word key is
  only split at space/tab).
- **fopen "r" text mode** for the word list (0041a5b0): the loader treats both `\r` and `\n` as line ends,
  so binary reading in the port is fine.
- **fread/fwrite of raw data** (save/load: writers 00403050 00404e10 00404fe0 0041f0a0 0041faa0 00430ab0,
  readers 00403370 00405fa0 00406180 0041f180 00430b20, 00408ab0...): little-endian 4-byte ints and 1-byte bools, the same on
  wasm32. Keep field widths (no `size_t`/`long` growth) when translating.
- **Multiset ordering of the high-score tables** (predicates above): ties on score are broken by `strcmp` of
  the word, ties on word length by score; equal keys go after existing ones. A port using
  `std::multiset<HighScoreEntry, ByScore>` with the same predicate reproduces it exactly.
- **`exit(1)`** after printing "Exception raised: ..." when the word list throws (0041a790): the port should
  report the error and quit the same way (or simply not throw).

##### Open questions (section 7.2)

- x87 precision: if the D3D7 device is created without FPU-preserve, the x87 control word is left at 24-bit
  precision and all `double` math feeding `_ftol` runs at single precision. Worth checking DDInterface /
  D3DInterface init (not in this slice) before chasing 1-unit differences in positions/scores.
- DAT_0056be74 (the ostream written in 0041a790) is `std::cerr` or `std::cout`; not determined (irrelevant
  for the port).
- Names `HighScoreEntry`, `ByScore`, `ByWordLength` and `Item12` (the 12-byte element of FameWidget's list
  at +0x294) are placeholders; the game-group helper should replace them with the real struct names.
- 004cbc61 (`_Locimp` copy-ctor), 004220e0, 00422100, 00421c70 and the basic_filebuf virtuals marked
  "(inferred)" were named from structure only; none is called by game logic.
- Proposed renames that change many decompiled call sites: `_free` 004da392 -> `operator_delete`,
  `_free` 004db6f3 -> `operator_delete_array`, `_atol` 004db780 -> `_atoi`. Apply only if the coordinator
  agrees (other slices may refer to the old names).

## 8. Behaviour differences and how the port handles them

The port's framework is SAF 1.3 (built for Bejeweled). Game code written against the 2004 framework is
translated onto it method by method; the table lists every old behaviour that would otherwise change what
the player sees or hears, and where the port handles it. "Game shim" means the shared compatibility
library `framework/compat2004/Compat2004.*` (CMake target `sexycompat2004`), which every game of this
generation links: `Dialog2004`, `Slider2004`, `WriteString2004` / `WriteWordWrapped2004` /
`GetWordWrappedHeight2004`, `SeedMTRand2004`, `CrtRand` / `CrtSRand`, `GameTickCount`,
`GetLastMouseDownTick`. Details and addresses are in the sections cited.

| # | 2004 behaviour | Port | Handling | Section |
|---|---|---|---|---|
| 1 | Dialog yes/no (footer) buttons have ids `mId+2000` / `mId+3000`, labels "YES"/"NO"/"OK"/"CANCEL", and forward ButtonPress/ButtonDepress/ButtonDownTick to the **app**; no DialogListener, no mResult | ids 1000/1001, GetString labels, DialogListener | game shim: a Dialog subclass that renumbers and relabels the buttons and forwards their events to the app (Zuma's decompilation patches the framework the same way) | 3 (D1), 4 |
| 2 | ButtonListener has 6 slots in another order (no ButtonMouseMove); Widget 47 slots, Dialog 52, SexyApp 67 in 2004 order | 1.3 order | none: translations call methods by name; the slot tables say which | 3, 4 |
| 3 | WidgetManager records the tick of the last mouse-down (WM+0x168, GetTickCount(), 0 before the first press); Lex's idle animation compares it with GetTickCount() | no such field | framework addition `WidgetManager::mLastMouseDownTick`, set in MouseDown from `SexyAppBase::GetClockTime()`; the game compares it with GetClockTime() and treats 0 as "never pressed" (the original's GetTickCount() is milliseconds since boot, so it reads as idle until the first press) | 3 (D2) |
| 4 | Slider: clicking the track does not move the thumb; SetValue does not clamp | 1.3 jumps to the click | game shim: Slider subclass whose MouseDown ignores track clicks | 3 (D4) |
| 5 | Widgets without a font fall back to `SysFont("SansSerif", ...)` | no SysFont | none for BookWorm (it sets every font); a translation must keep every SetFont call | 3 (D5), 5 |
| 6 | Text layout (WriteString, WriteWordWrapped, GetWordWrappedHeight, WriteCenteredLine) are Widget virtuals taking a Graphics*; WriteWordWrapped keeps extra spaces after a wrap and adds no height for a trailing newline | Graphics methods, 1.3 wrapping | game shim: `WriteString2004` / `WriteWordWrapped2004` / `GetWordWrappedHeight2004` for every Widget text write (BookWorm routes all of them through it; 41 of its word definitions have double spaces, so the difference shows) | 3 (D11), 5 |
| 7 | `MTRand::SRand` uses SAF 1.2 seeding (`mt[i] = mt[i-1] * 69069`) | 1.3 seeding | game shim: `SeedMTRand2004` seeds the game's own MTRand objects the 1.2 way (Big Money has its own `SeedMTRand12`); `CrtRand` / `CrtSRand` reproduce MSVC's CRT `rand()` | 7.1 |
| 8 | Music: BASS 2.0 MO3 by order position; the game's UpdateFrames calls the music Update twice per tick (fades at double speed); BASS sets the module global volume to the full volume (1.2: 0.4x) | libopenmpt for MO3 (`platform/openmpt`), SDLMusicInterface | translate the double Update literally; set the music interface's maximum volume as the game does; calibrate loudness by ear | 6 |
| 9 | Sounds: `cached_<name>.wav` decode cache, then .wav/.ogg/.au; a failed load ends Init | Mixer decodes .ogg/.au/.wav | never write a cache; keep the game's failure check | 6 |
| 10 | Cursor ids 0-5 (5 = none); custom image cursors only with "CustomCursors" | CURSOR_* enum | map id 5 to CURSOR_NONE | 4 |
| 11 | Command line: unknown switches pop up an error and exit | port test switches (-seed, -script, -fast) | do not translate the base HandleCmdLineParam | 4 |
| 12 | Timing: 100 updates/s; backlog clamp 1000 ms; Windows idle "screen saver" freeze; losing focus mutes; the loading thread is a real thread; game clocks read GetTickCount() | 100 Hz, 200 ms clamp, synchronous loading; GetTickCount()/timeGetTime() are real time even under -fast, `SexyAppBase::GetClockTime()` is the clock -fast advances | keep the port's loop; mute on focus loss as the game does; game timing that must repeat in scripted -fast runs (level clocks, idle timers) reads GetClockTime() | 4 |
| 13 | Online code: HTTPTransfer (used by the Hall of Fame), InternetManager (update check, ads), registration, BetaSupport, SEH reporting, demo record/playback | none | none: the online code is not ported (BookWorm's Hall of Fame drops its HTTPTransfer use); drop the rest too (partner.xml already selects the offline, registered paths) | 4, 7.1 |
| 14 | Folded getters: one function serves Image::GetWidth, Font::GetAscent and Color::GetGreen (and similar) | separate methods | read by the object's type when translating | 5, 7.1 |
| 15 | `DrawImageRotated(int ...)` rotates about integer centres | int and float overloads | use the port's int overload | 5 |
| 16 | Image loading: `GetImage(file)` tries .tga/.jpg/.png/.gif, alpha masks `_name` and `name_` (blue channel) | `platform/ImageLoader` does the same | none | 5 |
| 17 | ImageFont: the same descriptor commands, metrics and rendering (including the integer point-size scale) | same | none | 5 |
| 18 | ButtonWidget calls ButtonDepress before MarkDirty and without a focus check (Zuma order); the base Widget sets mIsOver/mIsDown/the finger cursor | WidgetManager does it | none (no visible difference) | 3 (D7, D8) |
| 19 | No WidgetContainer: one flat widget list in screen coordinates | widgets can have children | translate AddWidget/RemoveWidget literally as top-level widgets | 3 |
| 20 | CursorWidget, transient widgets, UpdateCheckDialog, ResourceManager, FModMusicInterface | not needed | drop | 3, 6, 7 |

## 9. Open questions

### Widgets

1. Original names of Widget slot 9 / +0x1c ("SetTransient"/"mTransient"), WM +0x28/+0x2c
   ("mCursorWidget"/"mShowCursorWidget") and WM +0x168 ("mLastMouseDownTick"): behaviour verified, names
   inferred. Typer Shark and AstroPop may use the cursor widget (custom cursors); recheck there.
2. WM +0x2c was not found being set anywhere in BookWorm (searched direct field stores through
   `mWidgetManager`); an indirect write cannot be excluded.
3. SysFont ctor argument meaning for the defaults (`(app, "SansSerif", 14, 1, 0, 0, 0)` header,
   `(.., 12, 1, ..)` lines and DialogButton, `(.., 10, 0, ..)` others): which flag is bold is for the
   graphics group.
4. ListWidget: GetSortKey/Sort (slots 47/48) and mSortFromChild (+0xb4) identified from shape and the 1.2
   analogue, not traced line by line; ScrollbarWidget slots 48/49 called on mScrollbar are assumed to be
   SetMaxValue/SetPageSize (no scrollbar class is linked, so probably dead in BookWorm).
5. EditWidget ProcessKey's mMaxPixels check is assumed to measure with mWidthCheckFont when set, else
   mFont (1.2 behaviour); not traced in detail.
6. UpdateCheckDialog ctor arguments not decoded (to be dropped).

### Graphics

- Graphics +0x30: a second Color, default (0,0,0,255), not copied by the copy ctor and read by no
  linked code. Purpose unknown (a removed state field?). Harmless.
- Graphics +0x4c..+0x57 are assumed to be the PolyFill fields (no PolyFill linked to confirm).
- DDImage +0x50/+0x52 names (mNoLock / mFirstPixelTrans) are inferred from 1.2 code shape.
- 0044c520 (BltRotatedClipHelper), 0044a990/0044a9f0 (FRect helpers), 00441800 (PurgeBits-like),
  004911e0/004913c0 (image writers) are named from call shape only.
- FontData::Popup on descriptor errors goes through app slot 21; confirm with the app group that slot
  21 is `Popup(const std::string&)`.
- Color::Black's address was not located (no reference in linked code).

### Application, sound, music and support

1. Music loudness: BASS 2.0 MO3 rendering (amplify 50, GVOL_MUSIC = v*100) vs libopenmpt; needs a capture to set the
   BookWorm music gain (section 6, 4.2).
2. Bookworm1.mo3 order list (orders 59/61 one-shot jingles): does libopenmpt stop/loop exactly like BASS 2.0 (section 6).
3. SexyApp slots 37/38 (PreDisplayHook / PreDDInterfaceInitHook) and 11 (ReInitImages) are named from their call
   positions; all three are empty or framework-only, no game impact.
4. Whether the port should also seed the global Sexy::Rand the 1.2 way (only demo/replay parity depends on it) (section 7.1).
5. x87 precision once Direct3D is on (mAutoEnable3D is set by BookWormApp; no DDSCL_FPUPRESERVE): only bit-exact
   float reproduction is affected (section 4, 1.7).
6. section 7.2 renames library functions other maps may reference by their old names (`_free` 0x4da392 -> operator delete,
   0x4db6f3 -> operator delete[], `_atol` 0x4db780 -> atoi): confirm before applying them together with other groups'
   symbol files.

## Reuse for Typer Shark and AstroPop

Typer Shark! Deluxe (game PE linked 2004-04-20) and AstroPop Deluxe (2004-04-06) were built with the same
compiler from the same framework snapshot (the survey found identical framework RTTI sets). Byte matching
with `tools/funcmatch.py` against the survey's Typer Shark export pairs 1,018 of BookWorm's 1,388 framework
functions (802 byte-identical, about 38% of the framework's bytes). The slot tables and field offsets
therefore carry over class by class, but each class should be checked against the new binary before its
offsets are trusted: the larger functions differ, so some layouts may too.

Steps for the next 2004 game:
1. Carve and import as in [section 1](#1-how-the-map-was-made).
2. Carry BookWorm's names over: `tools/funcmatch.py <game>.exe <game>/functions.jsonl re/bookworm/bin/BookwormGame.exe re/bookworm/gh/functions.jsonl ...`
   (BookWorm's export is named from `games/bookworm/docs/symbols/`).
3. Compare the vtable sizes with section 2 and the field stores in the constructors with the field tables
   here; record the differences in that game's notes.
4. Link `sexycompat2004` (`framework/compat2004`, section 8) and check each shim's addresses against the new
   binary.
