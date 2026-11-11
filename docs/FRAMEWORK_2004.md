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
- [Reuse for Typer Shark and AstroPop](#reuse-for-typer-shark-and-astropop)

Conventions: addresses are virtual addresses in BookWorm's carved game PE (`re/bookworm/bin/BookwormGame.exe`,
image base 0x400000). `(**(code **)(*this + 0xNN))` in the decompilation is slot NN/4 of the object's primary
vtable. "1.2" is SexyAppFramework 1.2 as built into Big Money Deluxe (`games/bigmoney/docs/FRAMEWORK_12.md`);
"port" is this repository's `framework/`. Verified facts come from constructor stores, accessor bodies or the
disassembly; inferred ones are marked.

---

## 1. How the map was made

No source of this framework generation is public. The map combines four sources:

1. **RTTI.** Ghidra 12 recovers no classes from MSVC 7.0 binaries (docs/POPCAP_GAMES_SURVEY.md 3.4, "Findings
   that apply to many games"), so `tools/rtti_msvc70.py` reads the RTTI itself: 159 classes, 187 vtables, and a
   `Class::vfNN` name for every virtual function.
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
  (`SDLSoundManager::VolumeToGain` converts the same centi-dB to linear gain). Above 0 dB DirectSound refuses the call
  and keeps the buffer's last volume; the port clamps to 0 dB (4fe27d4).
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
| 13 (+34) | 0041dfa0 | 00494810 | 0041dfa0 (base) | `void SetMusicAmplify(int, double)` | 18 | `SetMusicAmplify` (gain table measured against BASS 2.3, c553853) |
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
   interpolation) by ear or with a capture: the music/sfx balance depends on it.
3. **Double music Update** (004100f0): every fade runs at twice the nominal speed. Port: translate the override
   literally (base UpdateFrames, then `mMusicInterface->Update()`); do not "fix" it.
4. **MO3 order offsets**: offsets 0, 27, 37, 51, 54, 59, 61, 65 are order numbers (row 0) passed to
   `BASS_MusicPlayEx(h, MAKELONG(order,row), flags, TRUE)`; the port maps them to `Mixer::PlaySong(order, row)` →
   `openmpt_module_set_position_order_row` (platform/openmpt, in progress). Five songs = five independent module
   instances of one file; the port's Mixer already creates one Song per LoadMusic. Nothing else, but check loop/stop
   semantics: BASS follows backward position jumps (Bxx) even without BASS_MUSIC_LOOP (no
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
Shutdown vf33 0047cbf0 aborts it and checks GetResultCode == 0 -> app+0x330); no Get/Post on it was found.

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

### 7.2 std::string, STL, iostream and CRT helpers called by game code

Scope: every library function the game code (0x401000-0x437800) calls directly, and every STL template
instantiation that lives inside the game region. Toolchain: MSVC 7.0 (VS.NET 2002), Dinkumware STL of that
release, static CRT (LIBCMT), plus the old pre-standard iostream library (LIBCI: `ifstream`, `ios`,
`filebuf` without `std::`), which VC7.0 still shipped.
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
    `_Tree::_Insert`, `_Tree::erase(iterator)`); the symbols file has the real names. The game never catches
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
- **`_ftol` = truncation toward zero** (`(int)x`). Do not use `lround`/`floor` when translating. Caveat about
  x87 precision: section 4, "3D and the FPU".
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
| 21 | Direct3D texture addressing: a game sets the stage's address mode for u and v (`SetTextureStageState` 0xd and 0xe; 1 wrap, 3 clamp) round its `DrawTrianglesTex` calls (Bejeweled 2's Hyperspace tunnel wraps) | SDL's renderer and the GPU path have no address mode (clamp) | framework: `DDInterface::SetTextureAddress(u, v)` with `TEXTURE_ADDRESS_WRAP` or `_CLAMP` (the default, so no other game changes); with a wrap set, `DrawTrianglesTex` of both displays cuts the triangles at whole-number u and v (`framework/TriangleWrap.cpp`, tested by `tools/wraptest`) and draws the pieces clamped; a game calls it before and after the draws, as the original did | none |

## Reuse for Typer Shark and AstroPop

Typer Shark! Deluxe (game PE linked 2004-04-20) and AstroPop Deluxe (2004-04-06) were built with the same
compiler from the same framework snapshot (the survey found identical framework RTTI sets). Byte matching
with `tools/funcmatch.py` against the survey's Typer Shark export pairs 1,018 of BookWorm's 1,388 framework
functions (802 byte-identical, about 38% of the framework's bytes). The slot tables and field offsets
therefore carry over class by class, but each class should be checked against the new binary before its
offsets are trusted: the larger functions differ, so some layouts may too.
