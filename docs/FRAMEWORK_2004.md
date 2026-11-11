# The 2004 SexyAppFramework: compatibility map onto our port

BookWorm Deluxe, Typer Shark! Deluxe and AstroPop Deluxe (early 2004) are built on an internal SexyAppFramework
snapshot older than any public release. For every class game code touches, this map gives the vtable slots and
fields, what each is in SAF 1.2 and in the port (`framework/`, the SAF 1.3 API on SDL2), and where the old
behaviour differs. A translation calls the port's methods by name; the map says which name a
`(**(code **)(*this + 0x34))(g)` in the decompilation is, and what the old code did that the port must reproduce.

Conventions. Addresses are virtual addresses in BookWorm Deluxe 1.03's game PE
(`re/bookworm/bin/BookwormGame.exe`, image base 0x400000, linked 2004-04-15 with VS.NET 2002 / MSVC 7.0). Every
framework function there is named in `games/bookworm/docs/symbols/10_framework_*.jsonl`, so a function this map
does not list, or an address a code comment cites, is found there. `(**(code **)(*this + 0xNN))` is slot NN/4
of the object's primary vtable. "1.2" is SAF 1.2 as built into Big Money Deluxe; its slot numbers and field
offsets are in `games/bigmoney/docs/FRAMEWORK_12.md` under the same method and field names. "port" is
`framework/`; "BW" is BookWormApp. Facts come from constructor stores, accessor bodies or the disassembly; inferred ones are
marked. Subsection numbers are local to each section.

## 1. How the map was made

No source of this framework generation is public. The map combines RTTI (`tools/rtti_msvc70.py` reads it itself,
because Ghidra 12 recovers no classes from MSVC 7.0 binaries: 159 classes, 187 vtables and a `Class::vfNN` name
for every virtual function), byte matching against Big Money Deluxe (`tools/funcmatch.py`: the same compiler, SAF
1.2, named slot by slot; of the framework's 1,388 functions 828 pair, 550 of them byte-identical, and 698 names
were carried over) and a reading of every function game code calls against the public SAF 1.2
(`E:\retools\refs\saf12`) and 1.3 trees. alula/CircleShootApp (Zuma 1.0.0, 2003, also MSVC 7.0), a decompilation
that runs a game of this era on SAF 1.3 with small patches restoring old behaviour, was read as a guide only.

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

What sets this generation apart from the public trees: one application class (`Sexy::SexyApp`, 67 virtual
slots, derived only from `ButtonListener`, holding what 1.2 splits between `SexyAppBase` and `SexyApp`); no
`WidgetContainer` (section 3); loose files opened with the C runtime, no `PakInterface`; DirectDraw 7 / D3D7
drawing, DirectSound sounds (Ogg and `.au`) and BASS 2.0 music (MO3 by order position) with an unused FMOD
fallback; a `ResourceManager` that is linked but unused, resource names being hard-coded in the game; and
ImageFont/DescParser fonts as in 1.2.

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

Symbols: `games/bookworm/docs/symbols/10_framework_widgets.jsonl`. Code comments cite differences D1 to D11 of
an earlier list; they are section 8's rows: D1 is row 1, D2 row 3, D4 row 4, D5 row 5, D7 and D8 row 18, D11
row 6.

### 0. The big structural differences (read this first)

1. **No WidgetContainer.** A 2004 `Widget` has no children, no `mParent`, no `mPriority`/`mZOrder`, no
   widget flags. `WidgetManager` derives from nothing (only a virtual destructor) and keeps one flat
   `std::list<Widget*>`; every widget is top-level and its `mX/mY` are screen coordinates. Translate
   every `mWidgetManager->AddWidget(w)` / `RemoveWidget(w)` literally (top-level `AddWidget` in the port);
   never turn them into child widgets.
2. **Dialog buttons are app buttons.** A Dialog's yes/no (or footer) buttons get ids **`mId + 2000`** and
   **`mId + 3000`** and fixed labels `"YES"/"NO"/"OK"/"CANCEL"`, and the Dialog's ButtonListener forwards
   their press/depress/downtick to **`mWidgetManager->mApp`** (SexyApp *is* a ButtonListener, vtable
   slots 3/4/5). There is no DialogListener, no `mResult`, no `WaitForResult` (subsection 6; section 8, row 1).
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

### 1. Widget

sizeof **0x58** (1.2: 0x80 incl. WidgetContainer 0x4c; port: Widget : WidgetContainer). No base class.
Primary vtable `0x5017ac`, 47 slots. ctor `0x43e4e0` (no arguments), dtor body `0x43ab10`, deleting
dtor `0x43b3f0`; Checkbox/Slider/CursorWidget share the deleting dtor `0x43e8f0`.

#### 1.1 Vtable

Offset | slot | 2004 method | port 1.3 | notes
---|---|---|---|---
+00 | 0 | ~Widget() | ~Widget | frees mColors
+04 | 1 | AddedToManager(WidgetManager*) | WidgetContainer::AddedToManager | only sets mWidgetManager
+08 | 2 | RemovedFromManager(WidgetManager*) | WidgetContainer::RemovedFromManager | mWidgetManager = NULL
+0c | 3 | OrderInManagerChanged() | OrderInManagerChanged | empty
+10 | 4 | SetVisible(bool) | SetVisible | hide: MarkDirtyFull, show: MarkDirty; RehupMouse
+14 | 5 | SetColors(int theColors[][4], int n) | SetColors([][4]) | replaces mColors, MarkDirty
+18 | 6 | SetColors(int theColors[][3], int n) | SetColors([][3]) |
+1c | 7 | SetColor(int idx, const Color&) | SetColor | grows mColors
+20 | 8 | const Color& GetColor(int idx) | GetColor(int) | grows mColors with Color() when out of range (1.2/port return a static)
+24 | 9 | SetTransient(bool) (name inferred) | not in port | +0x1c flag; hide: MarkDirtyFull first; only CursorWidget uses it
+28 | 10 | SetDisabled(bool) | SetDisabled | as 1.2 (DisableWidget, re-MousePosition)
+2c | 11 | ShowFinger(bool) | ShowFinger | no NULL check of mWidgetManager
+30 | 12 | Resize(const Rect&) | Resize(const Rect&) |
+34 | 13 | Resize(int x, int y, int w, int h) | Resize(x,y,w,h) | MarkDirtyFull, set, MarkDirty, RehupMouse
+38 | 14 | Move(int x, int y) | Move |
+3c | 15 | bool WantsFocus() | WantsFocus | constant false (no mWantsFocus field)
+40 | 16 | Draw(Graphics*) | Draw | empty
+44 | 17 | Update() | Update | mUpdateCnt++
+48 | 18 | GotFocus() | GotFocus | mHasFocus = true
+4c | 19 | LostFocus() | LostFocus |
+50 | 20 | KeyChar(char) | KeyChar(SexyChar) | empty
+54 | 21 | KeyDown(KeyCode) | KeyDown | TAB: SetFocus(shift ? mTabPrev : mTabNext)
+58 | 22 | KeyUp(KeyCode) | KeyUp | empty
+5c | 23 | MouseEnter() | MouseEnter | **sets mIsOver and ShowFinger(true) if mDoFinger** (port: WidgetManager does this)
+60 | 24 | MouseLeave() | MouseLeave | mIsOver = false, ShowFinger(false) if mDoFinger
+64 | 25 | MouseMove(int x, int y) | MouseMove | empty
+68 | 26 | MouseDown(int x, int y, int btn, int cc) | MouseDown(x,y,btn,cc) | empty
+6c | 27 | MouseDown(int x, int y, int cc) | MouseDown(x,y,cc) | **sets mIsDown = true**, then slot 26 with (2,1) for cc==3, (0,cc) for cc>=0, (1,-cc) for cc<0
+70 | 28 | MouseUp(int x, int y, int btn, int cc) | MouseUp(x,y,btn,cc) | empty
+74 | 29 | MouseUp(int x, int y, int cc) | MouseUp(x,y,cc) | calls slot 30 first, then slot 28 (same split as slot 27)
+78 | 30 | MouseUp(int x, int y) | MouseUp(x,y) | **mIsDown = false** (port: WidgetManager clears it)
+7c | 31 | MouseDrag(int x, int y) | MouseDrag | empty
+80 | 32 | MouseWheel(int delta) | MouseWheel | empty; WM sends it to the focus widget
+84 | 33 | bool IsPointVisible(int x, int y) | IsPointVisible | true
+88 | 34 | Rect WriteCenteredLine(Graphics*, int off, const string&, Color c1, Color c2, const Point& shadow) | same | shadow in c2 at +shadow, text in c1
+8c | 35 | Rect WriteCenteredLine(Graphics*, int off, const string&) | same |
+90 | 36 | int WriteString(Graphics*, const string&, int x, int y, int w=-1, int just=-1, bool draw=true, int off=0, int len=-1) | WriteString | the full colour-code writer lives here in 2004 (see 1.4)
+94 | 37 | int WriteWordWrapped(Graphics*, const Rect&, const string&, int lineSpacing, int just) | WriteWordWrapped | see 1.4
+98 | 38 | int GetWordWrappedHeight(Graphics*, int w, const string&, int lineSpacing) | GetWordWrappedHeight | WriteWordWrapped into a NULL Graphics, Rect(0,0,w,0), just -1
+9c | 39 | int GetNumDigits(int) | GetNumDigits |
+a0 | 40 | WriteNumberFromStrip(Graphics*, int n, int x, int y, Image* strip, int spacing) | WriteNumberFromStrip | identical
+a4 | 41 | bool Contains(int x, int y) | Contains |
+a8 | 42 | Rect GetInsetRect() | GetInsetRect |
+ac | 43 | Rect GetRect() | WidgetContainer::GetRect |
+b0 | 44 | bool Intersects(Widget*) | WidgetContainer::Intersects |
+b4 | 45 | MarkDirtyFull() | WidgetContainer::MarkDirtyFull | wm ? wm->MarkDirtyFull(this) : mDirty = true
+b8 | 46 | MarkDirty() | WidgetContainer::MarkDirty | wm ? wm->MarkDirty(this) : mDirty = true

Not in 2004 (present in 1.2/port): UpdateF, UpdateAll/UpdateFAll/DrawAll, MarkDirty(WidgetContainer*)
/MarkDirtyFull(WidgetContainer*) as Widget virtuals, DrawOverlay, GetColor(idx, default), all child
management, Layout, DeferOverlay. Game code cannot call them, so nothing to translate.

#### 1.2 Fields

Offset | type | name | meaning | port field
---|---|---|---|---
+00 | vfptr | | |
+04 | ulong | mLastWMUpdateCount | last WM update count seen (UpdateFrame calls Update once per frame) | WidgetContainer::mLastWMUpdateCount
+08 | int | mX | screen x | mX
+0c | int | mY | | mY
+10 | int | mWidth | | mWidth
+14 | int | mHeight | | mHeight
+18 | bool | mDirty | ctor **true** | mDirty
+19 | bool | mVisible | ctor true | mVisible
+1a | bool | mMouseVisible | ctor true; receives mouse | mMouseVisible
+1b | bool | mClip | ctor true; DrawScreen clips to the widget only when set | mClip
+1c | bool | mTransient (name inferred) | drawn on top of a buffered background every redraw; excluded from dirty propagation | none
+1d | bool | mDisabled | | mDisabled
+1e | bool | mHasFocus | | mHasFocus
+1f | bool | mIsDown | set/cleared by the base mouse handlers | mIsDown
+20 | bool | mIsOver | set/cleared by base MouseEnter/Leave | mIsOver
+21 | bool | mHasTransparencies (inferred) | stops the "fully covered" shortcut in MarkDirtyFull | mHasTransparencies
+22 | bool | mHasAlpha | MarkDirty escalates to MarkDirtyFull | mHasAlpha
+24 | WidgetManager* | mWidgetManager | | mWidgetManager
+28 | std::vector<Color> (16) | mColors | first +0x2c, last +0x30 | mColors
+38 | Insets (16) | mMouseInsets | mLeft +0x38, mTop +0x3c, mRight +0x40, mBottom +0x44 | mMouseInsets
+48 | bool | mDoFinger | | mDoFinger
+4c | int | mUpdateCnt | Update() counter | mUpdateCnt
+50 | Widget* | mTabPrev | | mTabPrev
+54 | Widget* | mTabNext | | mTabNext

No `mWantsFocus`, `mParent`, `mWidgets`, `mPriority`, `mZOrder`, `mWidgetFlagsMod`,
`mUpdateIteratorModified`.

Static data: `Widget::mWriteColoredString` at 0x52f96c = true.

#### 1.4 WriteString / WriteWordWrapped (in Widget, not Graphics)

Text layout is a set of Widget virtuals taking a `Graphics*` (slots 34 to 38 and 40); the port's equivalents are
`Graphics` methods, and the shim's `WriteString2004` / `WriteWordWrapped2004` / `GetWordWrappedHeight2004`
reproduce the 2004 bodies (section 8, row 6). Always pass the justification the binary pushes: the port's
`Graphics::WriteString` defaults to 0 (centre), the 2004 Widget's to -1.

- **WriteString** (0x43b420) is the 1.2 `Graphics::WriteString` body: justification 0 centres, 1 right-aligns,
  anything else is left, applied only when drawString (the width is measured with drawString=false).
  `^RRGGBB^` (6 hex digits of either case and one closing character) sets the colour and keeps the current
  alpha; `^^` is a literal `^`; both are gated by the static `Widget::mWriteColoredString` (0x52f96c, true). A
  `^` with fewer than 8 characters left ends the string, and that test comes before the `^^` test, so a `^^`
  in the last 7 characters stops drawing (1.3 tests `^^` first and draws it). There is no `^oldclr^`. It
  returns the width and does not restore the colour.
- **WriteWordWrapped** (0x439f70) is the 1.2 algorithm: width by `CharWidthKern`, `\n` forces a wrap, colour
  codes are skipped when measuring, y starts at ascent - ascentPadding, lineSpacing -1 means the font's, a word
  longer than the line breaks mid-word (at least one character), the colour is restored, and it returns
  `yOffset + descent - lineSpacing`. Against the port's 1.3 method: after a wrap at a space it continues at
  space+1 without skipping further spaces (1.3 skips the run), so a double space at a wrap point indents the
  next line by one space and shifts centred lines; a trailing `\n` adds no line (1.3 adds lineSpacing to the
  height); there is no clip-rect culling, no theMaxWidth/theMaxChars/theLastWidth and no indent.
- **GetWordWrappedHeight** (0x4389f0) runs WriteWordWrapped on a `Graphics(NULL)` with g's font,
  `Rect(0,0,w,0)` and justification -1.

### 2. WidgetManager

sizeof **0x174** (allocated by SexyApp at 0x48aaf0, `operator new(0x174)`; 1.2: 0x1d4). No base class;
vtable `0x5017a0` has 1 slot (deleting dtor 0x43aae0; body 0x43a9c0). App field `mWidgetManager` is at
app+0x2e4 as in 1.2. ctor `0x43eb10(SexyApp*)`.

#### 2.1 Fields

Offset | type | name | meaning | port field
---|---|---|---|---
+00 | vfptr | | |
+04 | int | mUpdateCnt | UpdateFrame counter | mUpdateCnt
+08 | Widget* | mDefaultTab | ctrl+TAB target | mDefaultTab
+0c | SexyApp* | mApp | game reads it (`mWidgetManager->mApp`) | mApp
+10 | Image* | mImage | screen image, set by the app | mImage
+14 | DDImage* | mTransientImage | background buffer for transient widgets, created in DrawScreen | mTransientImage
+18 | bool | mLastHadTransients | | mLastHadTransients
+1c | int | mWidth | | mWidth
+20 | int | mHeight | | mHeight
+24 | Widget* | mPopupCommandWidget | removed on a click outside it | mPopupCommandWidget
+28 | CursorWidget* | mCursorWidget (inferred name) | created by the ctor, deleted by the dtor | none
+2c | bool | mShowCursorWidget (inferred name) | if set: AddWidget/BringToFront keep the cursor widget on top and mouse moves move it to (mouse - hotspot) | none
+30 | std::list<Widget*> | mWidgets | head +0x34, size +0x38; back = top | mWidgets
+3c | bool | mHasFocus | ctor true | mHasFocus
+40 | Widget* | mFocusWidget | | mFocusWidget
+44 | Widget* | mLastDownWidget | | mLastDownWidget
+48 | Widget* | mOverWidget | | mOverWidget
+4c | Widget* | mBaseModalWidget | single pointer, no PreModalInfo stack | mBaseModalWidget
+50 | bool | mMouseIn | | mMouseIn
+54 | int | mLastMouseX | game reads it | mLastMouseX
+58 | int | mLastMouseY | game reads it | mLastMouseY
+5c | int | mDownButtons | bit0 left, bit1 right, bit2 middle (no separate mActualDownButtons) | mDownButtons
+60 | bool | mUpdateIteratorModified | | mUpdateIteratorModified
+64 | list iterator | mUpdateIterator | | mUpdateIterator
+68 | bool[255] | mKeyDown | VK-indexed; +0x78 = shift, +0x79 = control | mKeyDown
+168 | DWORD | mLastMouseDownTick (name inferred) | GetTickCount() at every MouseDown; **read by the game** (Lex::Update 0x42dd20: idle > 12000 ms) | **none**
+16c | int | mLastDownButtonId | 1 left, -1 right, 2 middle | mLastDownButtonId
+170 | bool | mDirty | set by MarkDirty/MarkDirtyFull, cleared and returned by UpdateFrame | mDirty

Missing vs 1.2/port: mCurG, the deferred-overlay vector, all FlagsMod members, mPreModalInfoList,
mActualDownButtons, mLastInputUpdateCnt, mWidgetFlags, mMouseDestRect/mMouseSourceRect.

#### 2.2 Functions (all direct calls)

2004 function | 1.2 / port equivalent | notes
---|---|---
WidgetManager(SexyApp*) | WidgetManager(SexyAppBase*) | creates the CursorWidget
~WidgetManager body / deleting | ~WidgetManager |
FreeResources() (inferred) | FreeResources | deletes mTransientImage only
Resize(int w, int h) | port: Resize(Rect,Rect) | drops mTransientImage
AddWidget(Widget*) | AddWidget | if absent: push_back (top), w->AddedToManager(this), MarkDirty(w); then re-raise cursor widget (if +0x2c) and RehupMouse
RemoveWidget(Widget*) | RemoveWidget | MarkDirtyFull(w), erase (update-iterator safe), DisableWidget(w), w->RemovedFromManager(this)
bool HasWidget(Widget*) | HasWidget |
SetFocus(Widget*) | SetFocus | old->LostFocus() **even if it is the same widget**; focus only if HasWidget; GotFocus if mHasFocus
BringToFront(Widget*) | BringToFront | move to back of list, OrderInManagerChanged; no dirty marking
BringToBack(Widget*) | BringToBack |
PutInfront(Widget* w, Widget* ref) | PutInfront | used by Dialog and ListWidget too; no PutBehind is linked
MarkAllDirty() | MarkAllDirty | sets every widget's mDirty (not WM+0x170)
DrawWidgetsTo(Graphics*) | DrawWidgetsTo | every visible widget, clipped when mClip
MarkDirty(Widget*) | WidgetContainer::MarkDirty(WidgetContainer*) | 1.2 algorithm; transient widgets do not propagate
MarkDirtyFull(Widget*) | WidgetContainer::MarkDirtyFull(WidgetContainer*) | 1.2 algorithm (stops at an opaque widget fully covering the rect clipped to the screen)
Rect::Intersection | TRect::Intersection | shared with Graphics
SetBaseModal(Widget*) | AddBaseModal / RemoveBaseModal | see 2.3
bool IsBelow(Widget*, Widget*) | IsBelow |
DisableWidget(Widget*) | DisableWidget | over -> MouseLeave; lastdown -> DoMouseUps; focus -> LostFocus; basemodal -> NULL
DoMouseUps(Widget*, ulong downCode) | DoMouseUps | **passes the button index 0/1/2 as click count** (1.2/port: 1/-1/3)
RemovePopupCommandWidget() | same |
Widget* GetWidgetAt(int x, int y) | GetWidgetAt | top-down; skips !mVisible, !mMouseVisible; tests GetInsetRect().Contains then IsPointVisible(local); a disabled hit returns NULL; stops after passing mBaseModalWidget
MousePosition(int x, int y) | MousePosition | as 1.2 (MouseLeave old, MouseEnter + MouseMove new); moves the cursor widget if +0x2c
RehupMouse() | RehupMouse | as 1.2
bool MouseDown(int x, int y, int cc) | MouseDown | also stores GetTickCount() in +0x168 (section 8, row 3)
bool MouseUp(int x, int y, int cc) | MouseUp | button bit from cc (cc<0 right, cc==3 middle, else left); MouseUp(x,y,cc) on mLastDownWidget, then MousePosition
bool MouseMove(int x, int y) | MouseMove | mMouseIn = true, MousePosition
bool MouseDrag(int x, int y) | MouseDrag | as 1.2 (MouseDrag on mLastDownWidget, over-state follows the cursor)
bool MouseExit(int x, int y) | MouseExit |
MouseWheel(int) | MouseWheel | to mFocusWidget
bool KeyChar(char) | KeyChar | ctrl+TAB -> mDefaultTab->KeyChar(TAB)
bool KeyDown(KeyCode) / KeyUp(KeyCode) | KeyDown / KeyUp | mKeyDown[] for 0..254; KeyUp swallows TAB while ctrl is down
GotFocus() / LostFocus() | same | LostFocus sends KeyUp for every held key
bool UpdateFrame() | UpdateFrame | mDirty = false; mUpdateCnt++; Update() on **every** widget (visible or not) whose mLastWMUpdateCount differs; returns mDirty
bool DrawScreen() | DrawScreen | see 2.4

#### 2.3 Modal handling

One pointer, `mBaseModalWidget` (+0x4c), with no PreModalInfo stack. `SetBaseModal(w)` sets it and drops the
over widget, the last-down widget (DoMouseUps) and the focus widget when they are below `w`. GetWidgetAt never
returns a widget below it; widgets below still Update and Draw, and keys still go to mFocusWidget. The app does
the rest in AddDialog and KillDialog (section 4, slots 57 and 58). The port's AddDialog/KillDialog with
Add/RemoveBaseModal give the same result.

#### 2.4 Drawing

Only dirty widgets are redrawn, as in 1.2. Each is drawn with a copy of the screen Graphics translated to
(mX, mY) and clipped to its rect **only if mClip**. Transient widgets (+0x1c) are drawn last over a buffered
copy of the others (`mTransientImage`, a DDImage the size of the manager); only the CursorWidget is one. There
is no DrawOverlay, no modal flags, no SetFastStretch/SetLinearBlend. The port's 1.3 DrawScreen draws the same
dirty widgets in the same order.

### 3. Listener interfaces

ButtonListener (vtable 0x4f93ac, 6 slots; 1.2 has 7; port `ButtonListener.h` has 7, call by name):

slot | 2004 method | port
---|---|---
0 (+00) | ButtonMouseEnter(int id) | ButtonMouseEnter
1 (+04) | ButtonMouseLeave(int id) | ButtonMouseLeave
2 (+08) | ButtonPress(int id, int clickCount) { ButtonPress(id); } | ButtonPress(id, cc)
3 (+0c) | ButtonPress(int id) | ButtonPress(id)
4 (+10) | ButtonDepress(int id) | ButtonDepress
5 (+14) | ButtonDownTick(int id) | ButtonDownTick
- | (no ButtonMouseMove) | ButtonMouseMove (never called for 2004 code)

In a Widget-derived game class this vtable is at +0x58; in SexyApp it is the primary vtable (slots 0 to 5).
`this` inside a +0x58 thunk is `object+0x58`, so `this+0xa8` there is a Dialog's `mId` (+0x100) and `this-0x34`
is `mWidgetManager` (+0x24).

Other interfaces (1 vtable each, all defaults are folded stubs):

interface | slots (2004 = 1.2 = port order) | notes
---|---|---
CheckboxListener | 0 CheckboxChecked(int id, bool checked) | same as 1.2/port
SliderListener | 0 SliderVal(int id, double val) | same
EditListener | 0 EditWidgetText(int id, const string&), 1 bool AllowKey(int id, KeyCode), 2 bool AllowChar(int id, char), 3 bool AllowText(int id, const string&) | same; Allow* default true
ListListener | 0 ListClicked(int id, int idx, int clickCount) | **only one slot**; port adds ListClosed and ListHiliteChanged (defaults are no-ops)
ScrollListener | 0 ScrollPosition(int id, double pos) | same; ListWidget implements it at +0x58
DialogListener | not present in 2004 | port Dialog uses it; see D1

A Dialog subclass puts an EditListener, ListListener, SliderListener or CheckboxListener vtable at +0x110 or
after its own members; a Widget subclass that is already a ButtonListener puts the next one at +0x5c.

### 4. ButtonWidget

sizeof **0xe0** (1.2: 0x118). Base Widget. Vtable `0x501944`, 49 slots. ctor `0x43f080(int theId,
ButtonListener* theListener)`, dtor body `0x43b170`, deleting `0x43d350`.

#### 4.1 Vtable (slots not listed = Widget's)

slot | method | port | notes
---|---|---|---
0 | ~ButtonWidget | ~ButtonWidget | deletes mFont
10 | SetDisabled(bool) | SetDisabled | MarkDirty if mDisabledImage
16 | Draw(Graphics*) | Draw | see 4.3
17 | Update() | Update | see 4.3
23 | MouseEnter() | MouseEnter | Widget::MouseEnter; over-alpha reset; MarkDirtyFull if down / over image / hilite colour differs; listener slot 0 **if !mDisabled**
24 | MouseLeave() | MouseLeave | as 1.2; listener slot 1 if !mDisabled
27 | MouseDown(int x, int y, int cc) | MouseDown(x,y,btn,cc) (port overrides the 4-arg one) | Widget::MouseDown(x,y,cc); **if !mDisabled** ButtonPress(mId, cc) (listener slot 2); MarkDirtyFull
30 | MouseUp(int x, int y) | MouseUp(x,y,btn,cc) | Widget::MouseUp(x,y); **if mIsOver && !mDisabled** ButtonDepress(mId) (slot 4); then MarkDirtyFull (port: MarkDirty first, checks mWidgetManager->mHasFocus)
47 | SetFont(Font*) | SetFont | delete old, Duplicate()
48 | bool IsButtonDown() | IsButtonDown | mIsDown && mIsOver && !mDisabled

No `DrawButtonImage` virtual (1.2 slot 54): it is non-virtual at 0x4382c0; `HaveButtonImage` 0x438290.
No MouseMove override, so no ButtonMouseMove.

#### 4.2 Fields

Offset | type | name | ctor | port
---|---|---|---|---
+58 | int | mId | theId | mId
+5c | std::string | mLabel | "" | mLabel
+78 | int | mLabelJustify | 0 (center; -1 left, 1 right) | mLabelJustify
+7c | Font* | mFont | NULL | mFont
+80 | Image* | mButtonImage | NULL | mButtonImage
+84 | Image* | mOverImage | NULL | mOverImage
+88 | Image* | mDownImage | NULL | mDownImage
+8c | Image* | mDisabledImage | NULL | mDisabledImage
+90 | Rect | mNormalRect | 0 | mNormalRect
+a0 | Rect | mOverRect | 0 | mOverRect
+b0 | Rect | mDownRect | 0 | mDownRect
+c0 | bool | mInverted | false | mInverted
+c1 | bool | mBtnNoDraw | false | mBtnNoDraw
+c2 | bool | mFrameNoDraw | false | mFrameNoDraw
+c4 | ButtonListener* | mButtonListener | theListener | mButtonListener
+c8 | double | mOverAlpha | 0 | mOverAlpha
+d0 | double | mOverAlphaSpeed | 0 (game sets 0.08) | mOverAlphaSpeed
+d8 | double | mOverAlphaFadeInSpeed | 0 | mOverAlphaFadeInSpeed

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
- MouseEnter and MouseLeave call the listener's ButtonMouseEnter / ButtonMouseLeave once. Ours already does, so a game
  override that also calls the listener doubles it (AstroPop's AstroButton did, which broke its menu's secret hover
  order; its GAME_NOTES section 10).

### 5. DialogButton

sizeof **0xf8** (1.2: 0x130). Base ButtonWidget. Vtable `0x501fa4`, 49 slots: slot 0 = shared deleting
dtor 0x43f2d0 (no own destructor), slot 16 Draw 0x43dc90; everything else ButtonWidget's. ctor
`0x43f240(Image* theComponentImage, int theId, ButtonListener* theListener)`.

Offset | type | name | ctor | port
---|---|---|---|---
+e0 | Image* | mComponentImage | arg | mComponentImage
+e4 | int | mTranslateX | 1 | mTranslateX
+e8 | int | mTranslateY | 1 | mTranslateY
+ec | int | mTextOffsetX | 0 | mTextOffsetX
+f0 | int | mTextOffsetY | 0 | mTextOffsetY

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
(the signature of 1.2 and the port); the dtor deletes both buttons and both fonts.

#### 6.1 Vtable (slots not listed = Widget's)

slot | method | port | notes
---|---|---|---
0 | ~Dialog | ~Dialog |
1 | AddedToManager(WM*) | AddedToManager | Widget's, then wm->AddWidget(yes), AddWidget(no)
2 | RemovedFromManager(WM*) | RemovedFromManager | Widget's, then RemoveWidget(yes/no)
3 | OrderInManagerChanged() | OrderInManagerChanged | PutInfront(yes, this), PutInfront(no, this)
7 | SetColor(int, const Color&) | SetColor | idx 3 -> buttons' colour 0, idx 4 -> buttons' colour 1
13 | Resize(x,y,w,h) | Resize | see 6.3
16 | Draw(Graphics*) | Draw | see 6.3
17 | Update() | Update | `jmp Widget::Update`
27 | MouseDown(x,y,cc) | MouseDown(x,y,btn,cc) | only if cc == 1: SetCursor(2), mDragging, mDragMouseX/Y. Does **not** call Widget::MouseDown
30 | MouseUp(x,y) | MouseUp(x,y,btn,cc) | if mDragging: SetCursor(0), mDragging = false
31 | MouseDrag(x,y) | MouseDrag | identical to 1.2 (keeps the dialog within -8..+8 of the WM size)
47 | SetButtonFont(Font*) | SetButtonFont | yes/no->SetFont
48 | SetHeaderFont(Font*) | SetHeaderFont | Duplicate
49 | SetLinesFont(Font*) | SetLinesFont | Duplicate
50 | int GetPreferredHeight(int w) | GetPreferredHeight | 1.2 formula; button height = yes->mComponentImage->GetHeight(); no EnsureFonts
51 | bool IsModal() | IsModal |
- | (no WaitForResult) | WaitForResult | do not use

ButtonListener at +0x58: slots 0/1 are stubs, 2 is ButtonListener::ButtonPress(id,cc), and
**3 ButtonPress(id) 0x4386e0, 4 ButtonDepress(id) 0x438720, 5 ButtonDownTick(id) 0x438760** each do
`if (id == mId + 2000 || id == mId + 3000) mWidgetManager->mApp->ButtonPress / ButtonDepress /
ButtonDownTick(id)` (app slots 3 / 4 / 5). Game dialogs override slots 2 to 4 at +0x58, call these and compare
`theId == mId + 2000` themselves; the app's ButtonDepress decodes 2000..2999 as yes/ok of dialog id-2000 and
3000.. as no/cancel of dialog id-3000.

#### 6.2 Fields

Offset | type | name | ctor | port
---|---|---|---|---
+58 | vfptr | ButtonListener | |
+5c | Image* | mComponentImage | arg | mComponentImage
+60 | DialogButton* | mYesButton | | mYesButton
+64 | DialogButton* | mNoButton | | mNoButton
+68 | std::string | mDialogHeader | | mDialogHeader
+84 | std::string | mDialogFooter | | mDialogFooter
+a0 | std::string | mDialogLines | | mDialogLines
+bc | int | mButtonMode | 0 none, 1 YES_NO, 2 OK_CANCEL, 3 FOOTER | mButtonMode
+c0 | Font* | mHeaderFont | NULL | mHeaderFont
+c4 | Font* | mLinesFont | NULL | mLinesFont
+c8 | int | mTextAlign | 0 | mTextAlign
+cc | int | mLineSpacingOffset | 0 | mLineSpacingOffset
+d0 | Insets | mBackgroundInsets | 0 | mBackgroundInsets
+e0 | Insets | mContentInsets | (24,24,24,24) | mContentInsets
+f0 | int | mSpaceAfterHeader | 10 | mSpaceAfterHeader
+f4 | bool | mDragging | false | mDragging
+f8 | int | mDragMouseX | | mDragMouseX
+fc | int | mDragMouseY | | mDragMouseY
+100 | int | mId | theId | mId
+104 | bool | mIsModal | isModal | mIsModal
+108 | int | mButtonHorzSpacing | 8 | mButtonHorzSpacing
+10c | int | mButtonSidePadding | 0 | mButtonSidePadding

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

UpdateCheckDialog (sizeof 0x120, a Dialog that polls an HTTP transfer for the update check) is not ported.

### 7. Checkbox

sizeof **0x8c** (1.2: 0xe4). Base Widget. Vtable `0x501edc`, 49 slots; slot 0 = shared 0x43e8f0.
ctor `0x43e890(Image* theUncheckedImage, Image* theCheckedImage, int theId, CheckboxListener*)` (same as
1.2/port); sets mDoFinger = true.

slot | method | port | notes
---|---|---|---
16 | Draw | Draw | if mCheckedRect.mWidth: DrawImage(mUncheckedImage, 0,0, mChecked ? mCheckedRect : mUncheckedRect); else DrawImage(mChecked ? mCheckedImage : mUncheckedImage). No NULL checks, no outline fallback
27 | MouseDown(x,y,cc) | MouseDown(x,y,btn,cc) | Widget::MouseDown; toggle; listener->CheckboxChecked(mId, mChecked) if listener; MarkDirty
47 | SetChecked(bool, bool tellListener) | SetChecked | MarkDirty
48 | bool IsChecked() | IsChecked |

Offset | type | name | port
---|---|---|---
+58 | CheckboxListener* | mListener | mListener
+5c | int | mId | mId
+60 | bool | mChecked | mChecked
+64 | Image* | mUncheckedImage | mUncheckedImage
+68 | Image* | mCheckedImage | mCheckedImage
+6c | Rect | mCheckedRect | mCheckedRect
+7c | Rect | mUncheckedRect | mUncheckedRect

No mOutlineColor/mBkgColor/mCheckColor. All verified (ctor + Draw).

### 8. Slider

sizeof **0x80** (`operator new(0x80)` in OptionsDialog; 1.2: 0xa8). Base Widget. Vtable `0x501c54`,
49 slots; slot 0 = shared 0x43e8f0. ctor `0x43e550(Image* theTrackImage, Image* theThumbImage, int theId,
SliderListener*)`. Horizontal only.

slot | method | port | notes
---|---|---|---
16 | Draw | Draw | 1.2 three-part track (track width/3 cells) + thumb at `(int)(mVal*(w - thumbW))`
24 | MouseLeave | MouseLeave | SetCursor(pointer) unless dragging
25 | MouseMove(x,y) | MouseMove | dragging cursor over the thumb
27 | MouseDown(x,y,cc) | MouseDown(x,y,cc) | starts a drag **only on the thumb**; a click on the track does nothing (1.2/port jump the thumb there)
30 | MouseUp(x,y) | MouseUp(x,y) | mDragging = false, SetCursor(0), SliderVal(mId, mVal) (on any mouse-up)
31 | MouseDrag(x,y) | MouseDrag | mVal = (x - mRelX)/(w - thumbW) clamped 0..1; SliderVal + MarkDirtyFull if changed
47 | SetValue(double) | SetValue | **no clamping**; MarkDirtyFull
48 | bool HasTransparencies() | HasTransparencies | true (folded stub)

Offset | type | name | port
---|---|---|---
+58 | SliderListener* | mListener | mListener
+60 | double | mVal | mVal
+68 | int | mId | mId
+6c | Image* | mTrackImage | mTrackImage
+70 | Image* | mThumbImage | mThumbImage
+74 | bool | mDragging | mDragging
+78 | int | mRelX | mRelX

No mRelY/mHorizontal. All verified.

### 9. EditWidget

sizeof **0xec** (1.2: 0x11c). Base Widget. Vtable `0x50186c`, 53 slots. ctor `0x43ef90(int theId,
EditListener*)`, dtor body `0x43aef0` (deletes mFont and mWidthCheckFont), deleting `0x43beb0`.

slot | method | port | notes
---|---|---|---
13 | Resize | Resize | + FocusCursor(false)
15 | WantsFocus | WantsFocus | true
16 | Draw | Draw | as 1.2: bkg mColors[0], two clipped passes (text mColors[2]; selection fill mColors[3] + text mColors[4] when focused), outline mColors[1] DrawRect; SysFont "SansSerif" 10 fallback
17 | Update | Update | blink when focused (mBlinkDelay 40)
18 / 19 | GotFocus / LostFocus | same |
20 | KeyChar(char) | KeyChar | AllowChar -> ProcessKey(0, c); no Widget::KeyChar call
21 | KeyDown(KeyCode) | KeyDown | keys outside 'A'..'Y' -> AllowKey -> ProcessKey(k, 0); Widget::KeyDown
23 / 24 | MouseEnter / MouseLeave | same | text / pointer cursor
27 | MouseDown(x,y,cc) | MouseDown(x,y,btn,cc) | cursor from GetCharAt; cc > 1 selects the word
29 | MouseUp(x,y,cc) | MouseUp | drop an empty selection
31 | MouseDrag | MouseDrag | extend selection
46 | MarkDirty | MarkDirty | MarkDirtyFull if mColors[0].mAlpha != 255
47 | ProcessKey(KeyCode, char) | ProcessKey | 1.2 logic: shift-select, ctrl-C/X (app clipboard), ctrl-V (keeps chars passing AllowChar, stops at CR/LF), ctrl-Z undo, arrows (ctrl = word jump), Home/End, Backspace/Delete, Enter -> EditWidgetText, printable 0x20..0x7e; mMaxChars and mMaxPixels truncation; AllowText veto restores the old text
48 | SetFont(Font*, Font* widthCheck) | SetFont(Font*, Font* = NULL) | duplicates both; one width-check font instead of the 1.2 list
49 | SetText(const string&) | SetText(text, leftPosToZero = true) | cursor to end, mHilitePos 0, MarkDirty; **does not touch mLeftPos** (port resets it to 0 by default)
50 | bool IsPartOfWord(char) | IsPartOfWord | [A-Za-z0-9_]
51 | int GetCharAt(int x, int y) | GetCharAt |
52 | FocusCursor(bool bigJump) | FocusCursor | no clamp of mLeftPos to len-1

Non-virtual: GetDisplayString 0x43afd0 (password masking).

Offset | type | name | ctor | port
---|---|---|---|---
+58 | int | mId | arg | mId
+5c | std::string | mString | | mString
+78 | std::string | mPasswordDisplayString | | mPasswordDisplayString
+94 | Font* | mFont | NULL | mFont
+98 | Font* | mWidthCheckFont (inferred name) | NULL | mWidthCheckList
+9c | EditListener* | mEditListener | arg | mEditListener
+a0 | bool | mShowingCursor | false | mShowingCursor
+a4 | int | mCursorPos | 0 | mCursorPos
+a8 | int | mHilitePos | -1 | mHilitePos
+ac | int | mBlinkAcc | 0 | mBlinkAcc
+b0 | int | mBlinkDelay | 40 | mBlinkDelay
+b4 | int | mLeftPos | 0 | mLeftPos
+b8 | int | mMaxChars | -1 | mMaxChars
+bc | int | mMaxPixels | -1 | mMaxPixels
+c0 | char | mPasswordChar | 0 | mPasswordChar
+c4 | std::string | mUndoString | | mUndoString
+e0 | int | mUndoCursor | 0 | mUndoCursor
+e4 | int | mUndoHilitePos | 0 | mUndoHilitePos
+e8 | int | mLastModifyIdx | 0 | mLastModifyIdx

No mDrawSelOverride. Colours `SetColors(gEditWidgetColors, 5)` (0x52f9ac) = white bkg, black outline,
black text, black hilite, white hilite text: same as the port's table. All fields verified from ctor/dtor
and the functions above except +0x98's name.

### 10. ListWidget

sizeof **0xc0** (`operator new(0xc0)`). Bases Widget, ScrollListener at **+0x58**. Vtables `0x501d24`
(60 slots) and `0x501d1c` (+0x58, 1 slot: ScrollPosition 0x437d80). ctor `0x43ee70(int theId, Font*
theFont, ListListener*)`.

slot | method | port | notes
---|---|---|---
3 | OrderInManagerChanged | same | PutInfront(mChild) and PutInfront(mScrollbar) through gSexyAppBase->mWidgetManager
13 | Resize | same | mPageSize = (h-8)/itemH if h > itemH+8 else 1; mScrollbar slot 49 (SetPageSize)
16 | Draw | same | bkg mColors[0]; rows from (int)mPosition; selected row filled mColors[4]; text mColors[3] if hilited, mColors[5] if selected and 6+ colours exist, else mLineColors[i]; x: 0 -> 4, 1 -> centred, else right-4; outline mColors[1] if mDrawOutline
24 | MouseLeave | same | hilite -1 on the whole parent/child chain, pointer cursor
25 | MouseMove(x,y) | same | row under the cursor becomes the hilite of the chain; hand cursor on a row (no mDoFingerWhenHilited, no ListHiliteChanged)
27 | MouseDown(x,y,cc) | MouseDown(x,y,btn,cc) | ListClicked(mId, mHiliteIdx, cc) if hilited and listener; no Widget::MouseDown
47 | string GetSortKey(int idx) | same | zero-padded to mMaxNumericPlaces, child key when mSortFromChild
48 | Sort(bool ascending) | same |
49 | string GetStringAt(int idx) | same |
50 | int AddLine(const string&, bool alphabetical) | same | adds "" to the child lists
51 | SetLine(int, const string&) | same |
52 | int GetLineCount() | same |
53 | int GetLineIdx(const string&) | same |
54 | SetColor(const string& line, const Color&) | SetColor(const SexyString&, const Color&) | sets mLineColors on the chain
55 | RemoveLine(int) | same | mScrollbar slot 48 (SetMaxValue) with the new count
56 | RemoveAll() | same |
57 | int GetOptimalWidth() | same | widest line + 16
58 | int GetOptimalHeight() | same | count * itemH + 8
59 | SetSelect(int) | same |
(+0x58) 0 | ScrollPosition(int id, double pos) | same | forwards to mChild, mPosition = pos

mPageSize is computed on the x87 and stored through a float, so it is rounded to 24 bits; with our double a scrolled list
box shows its lines one pixel higher in some positions. AstroPop rounds it in its ListDialog::Resize
(`(double)(float)mPageSize`, then the scroll bar's page size); a shared fix in ListWidget would cover every 2004 game
with a scrolled list (not made yet: check BookWorm's and Typer Shark's list boxes first).

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

Insets is `mLeft, mTop, mRight, mBottom`, Rect `mX, mY, mWidth, mHeight`, Point `mX, mY`, as in the port. Their
small functions are folded together (item 5 of subsection 0): read a call to 0x437e90 by where its result goes,
an Insets into +0xd0/+0xe0 of a Dialog, a Color into a Graphics. `Rect::Intersection` is 0x4387a0; Contains and
Intersects are inlined; GetRect and GetInsetRect return a Rect through a hidden pointer.

### 14. KeyCodes

KeyCode is the Win32 virtual-key code, the same values as 1.2 and the port's `KeyCodes.h`. GetKeyCodeFromName
and GetKeyNameFromCode are not linked. The manager tracks `mKeyDown[0..254]`.

## 4. The application class (SexyApp)

The merged SexyAppBase + SexyApp of 2004. Symbols: `games/bookworm/docs/symbols/10_framework_app.jsonl` (which
also covers sections 6 and 7).

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

`class SexyApp : public ButtonListener` only: one vtable at +0 and no DialogListener base (1.2:
`SexyAppBase : ButtonListener, DialogListener`, second vtable at +4), and no separate SexyAppBase class.
sizeof is 0x618, so a game's app class starts its own data there. `gSexyAppBase` (stored early in the ctor) and
`gSexyApp` (stored at its end) both hold the app. The build string is "00000190Thu Apr 15 14:52:08 2004": 8
digits of mBuildNum, then mBuildDate.

#### 1.1 Vtable (67 slots; a game's app class adds its own from slot 67)

| Slot (+off) | Method (2004 signature) | Port | Notes |
|---|---|---|---|
| 0 (+00) | `ButtonMouseEnter(int theId)` | ButtonMouseEnter | the ButtonListener base: 6 slots in the order of section 3 |
| 1 (+04) | `ButtonMouseLeave(int theId)` | ButtonMouseLeave | |
| 2 (+08) | `ButtonPress(int theId, int theClickCount)` | ButtonPress(id,cc) | calls slot 3 |
| 3 (+0c) | `ButtonPress(int theId)` | ButtonPress(id) | empty in the base |
| 4 (+10) | `ButtonDepress(int theId)` | ButtonDepress | a game's override handles every menu and dialog button; a dialog's YES arrives as 2000+dialogId, NO as 3000+dialogId |
| 5 (+14) | `ButtonDownTick(int theId)` | ButtonDownTick | dialogs forward their buttons' down-ticks here too |
| 6 (+18) | `void Process()` | Process(bool) | no allowSleep, returns nothing; see 1.4 |
| 7 (+1c) | `void UpdateFrames()` | UpdateFrames | `if(!mMinimized){++mUpdateCount; if(mWidgetManager->UpdateFrame()) ++mFPSDirtyCount;} mMusicInterface->Update(); mInternetManager->Update()` |
| 8 (+20) | `void DoUpdateFrames()` | DoUpdateFrames | returns at once while gScreenSaverActive; at the first update after the loading thread ends: thread priority normal, mLoaded=true, mYieldMainThread=false, LoadingThreadCompleted(); then UpdateFrames() |
| 9 (+24) | `void MakeWindow()` | MakeWindow | the window (windowed at 64,64 with an mWidth x mHeight client; fullscreen WS_POPUP, topmost), the DDInterface on the first call (and registry "Is3D"), DDInterface::Init with colour-depth and 3D fallbacks, `SetTimer(mHWnd,100,mFrameTime)` |
| 10 (+28) | `void EnforceCursor()` | EnforceCursor | see 1.5 |
| 11 (+2c) | `void ReInitImages()` (name inferred) | ReInitImages | MemoryImage slot 18 (frees the cached native/RLE alpha data) on every image of mMemoryImageSet, after DirectDraw is re-initialised |
| 12 (+30) | `void DeleteExtraImageData()` | DeleteExtraImageData | image slot 15 DeleteExtraBuffers on every image |
| 13 (+34) | `void LoadingThreadCompleted()` | LoadingThreadCompleted | empty |
| 14 (+38) | `~SexyApp()` | ~SexyApp | scalar deleting |
| 15 (+3c) | `MusicInterface* CreateMusicInterface(HWND theHWnd)` | CreateMusicInterface() | section 6; Init passes mInvisHWnd |
| 16 (+40) | `void Init()` | Init | see 1.3; a game's override calls it first |
| 17 (+44) | `void LoadingThreadProc()` | LoadingThreadProc | empty; runs on a real thread (1.6) |
| 18 (+48) | `void WriteToRegistry()` | WriteToRegistry | see 1.7 |
| 19 (+4c) | `void ReadFromRegistry()` | ReadFromRegistry | see 1.7 |
| 20 (+50) | `Dialog* NewDialog(int theDialogId, bool isModal, const std::string& theHeader, const std::string& theLines, const std::string& theFooter, int theButtonMode)` | NewDialog | **the base returns NULL** (1.2 builds a Dialog): the game's override builds every dialog, so the port's default is never reached |
| 21 (+54) | `void Popup(const std::string& theString)` | Popup | `if(!mShutdown) MessageBox(mHWnd, s, "FATAL ERROR", MB_ICONHAND)` (literal title, no property) |
| 22 (+58) | `void SafeDeleteWidget(Widget* theWidget)` | SafeDeleteWidget | push_back on mSafeDeleteList (a plain `list<Widget*>`, no update depth) |
| 23 (+5c) | `bool Validate(const std::string& theUserName, const std::string& theRegCode)` | SexyApp::Validate (non-virtual) | the registration check, called by ReadFromRegistry; not ported |
| 24 (+60) | `void URLOpenFailed(const std::string& theURL)` | URLOpenFailed | mIsOpeningURL=false |
| 25 (+64) | `void URLOpenSucceeded(const std::string& theURL)` | URLOpenSucceeded | mIsOpeningURL=false; if mShutdownOnURLOpen {mSkipAd=true; Shutdown()} |
| 26 (+68) | `bool OpenURL(const std::string& theURL, bool shutdownOnOpen)` | OpenURL | ignores a repeat of the URL being opened; sets mIsOpeningURL, mShutdownOnURLOpen, mOpeningURL, mOpeningURLTime; ShellExecute("open"); on failure URLOpenFailed |
| 27 (+6c) | `bool OpenHTMLTemplate(const std::string& theTemplateFile, const DefinesMap& theDefines)` | SexyApp (stub) | not ported |
| 28 (+70) | `bool OpenRegisterPage()` | SexyApp (stub) | not ported |
| 29 (+74) | `bool OpenRegisterPage(DefinesMap theDefines)` | SexyApp (stub) | not ported |
| 30 (+78) | `std::string GetProductVersion(const std::string& thePath)` | GetProductVersion | the exe's ProductVersion resource |
| 31 (+7c) | `void SEHOccured()` | SEHOccured | SetMusicVolume(0); hide window; mSEHOccured=true; EnforceCursor() |
| 32 (+80) | `std::string GetGameSEHInfo()` | GetGameSEHInfo | crash report text; not ported |
| 33 (+84) | `void Shutdown()` | Shutdown | see 1.3 |
| 34 (+88) | `void ParseCmdLine(const std::string& theCmdLine)` | ParseCmdLine | splits on spaces outside quotes, "name=value"; skipped when the external window (0x318) is set |
| 35 (+8c) | `void HandleCmdLineParam(const std::string& theName, const std::string& theValue)` | HandleCmdLineParam | -play, -recnum=N, -playnum=N, -record, -demofile=F, -version, -crash; **anything else pops up an error and exits**, so the port's test switches (-seed, -script, -fast) never pass through a translation of it |
| 36 (+90) | `void Start()` | Start | see 1.3 |
| 37 (+94) | `void PreDisplayHook()` (name inferred) | PreDisplayHook | empty; called in Init before mWidgetManager->Resize |
| 38 (+98) | `void PreDDInterfaceInitHook()` (name inferred) | PreDDInterfaceInitHook | empty; called before every DDInterface::Init |
| 39 (+9c) | `bool ChangeDirHook(const char* theIntendedPath)` | ChangeDirHook | returns false, so Init chdir()s to the exe folder |
| 40 (+a0) | `void PlaySample(int theSoundNum, int thePan)` | PlaySample(id,pan) | `inst=mSoundManager->GetSoundInstance(id); inst->SetPan(pan); inst->Play(false,true)` |
| 41 (+a4) | `void PlaySample(int theSoundNum)` | PlaySample(id) | `GetSoundInstance(id)->Play(false,true)` (autorelease) |
| 42 (+a8) | `double GetMasterVolume()` | GetMasterVolume | mSoundManager slot 9 |
| 43 (+ac) | `double GetMusicVolume()` | GetMusicVolume | mMusicVolume |
| 44 (+b0) | `double GetSfxVolume()` | GetSfxVolume | mSfxVolume |
| 45 (+b4) | `bool IsMuted()` | IsMuted | mMuteCount > 0 |
| 46 (+b8) | `void SetMasterVolume(double)` | SetMasterVolume | **stores into mSfxVolume** (as 1.2), then mSoundManager->SetMasterVolume (slot 10) |
| 47 (+bc) | `void SetMusicVolume(double)` | SetMusicVolume | mMusicVolume=v; mMusicInterface->SetVolume(muted ? 0 : v) (slot 12) |
| 48 (+c0) | `void SetSfxVolume(double)` | SetSfxVolume | mSfxVolume=v; mSoundManager->SetVolume(muted ? 0 : v) (slot 5) |
| 49 (+c4) | `void Mute(bool autoMute)` | Mute | ++mMuteCount (and ++mAutoMuteCount if autoMute); re-applies both volumes |
| 50 (+c8) | `void Unmute(bool autoMute)` | Unmute | if mMuteCount>0: --mMuteCount (and --mAutoMuteCount if autoMute); re-applies both volumes |
| 51 (+cc) | `double GetLoadingThreadProgress()` | GetLoadingThreadProgress | 1.0 if mLoaded; 0 if not started or mNumLoadingThreadTasks==0; else min(completed/num, 1.0) |
| 52 (+d0) | `void SwitchScreenMode(bool wantWindowed, bool is3d)` | SwitchScreenMode(w,3d,force) | mForceFullscreen forces fullscreen; on a change: Set3DAccelerated(is3d,false), MakeWindow, show, mSoundManager->SetCooperativeWindow, ReInitImages, mLastTime=now; else Set3DAccelerated(is3d,true) |
| 53 (+d4) | `void SwitchScreenMode(bool wantWindowed)` | SwitchScreenMode(w) | slot 52 with Is3DAccelerated() |
| 54 (+d8) | `void SetAlphaDisabled(bool)` | SetAlphaDisabled | |
| 55 (+dc) | `Dialog* DoDialog(int id, bool isModal, const std::string& header, const std::string& lines, const std::string& footer, int buttonMode)` | DoDialog | `KillDialog(id); d=NewDialog(...); AddDialog(id,d); return d` |
| 56 (+e0) | `Dialog* GetDialog(int theDialogId)` | GetDialog | mDialogMap lookup |
| 57 (+e4) | `void AddDialog(int theDialogId, Dialog* theDialog)` | AddDialog(id,d) | KillDialog(id); map insert; list push_back; mWidgetManager->AddWidget(d); if d->IsModal() (Dialog slot 51): SetBaseModal(d) and ModalOpen() |
| 58 (+e8) | `bool KillDialog(int theDialogId)` | KillDialog(id) | if modal ModalClose(); mWidgetManager->RemoveWidget(d); list remove; **SafeDeleteWidget(d)** (deleted later, not now); map erase; if no base-modal widget is left (WM+0x4c==0), the most recently added remaining modal dialog becomes base modal |
| 59 (+ec) | `int GetDialogCount()` | GetDialogCount | mDialogMap.size() |
| 60 (+f0) | `void ModalOpen()` | ModalOpen | empty |
| 61 (+f4) | `void ModalClose()` | ModalClose | empty |
| 62 (+f8) | `void GotFocus()` | GotFocus | empty |
| 63 (+fc) | `void LostFocus()` | LostFocus | empty |
| 64 (+100) | `bool DebugKeyDown(int theKey)` | DebugKeyDown | only after Ctrl+Alt+D toggled mDebugKeysEnabled: F2 perf, F3 FPS, F4 FPS mode, F8 3D toggle, F9 memory, F11 dump; not ported |
| 65 (+104) | `bool DrawDirtyStuff()` | DrawDirtyStuff | see 1.4 |
| 66 (+108) | `void Redraw(Rect* theClipRect)` | Redraw | DDInterface::Redraw; on failure re-inits DirectDraw (lost surfaces) |

Virtuals of 1.2 that do not exist here: DoUpdateFramesF (2004 has no UpdateF), DeleteNativeImageData, InitHook,
ShutdownHook, PreTerminate, BeginPopup, EndPopup, MsgBox, LogScreenSaverError, GetSEHWebParams, DoParseCmdLine
(inlined in Init), GetImage and GetSharedImage as virtuals, SwitchScreenMode(), AddDialog(Dialog*),
KillDialog(Dialog*), IsAltKeyUsed, DebugKeyDownAsync, CloseRequestAsync, Done3dTesting, NotifyCrashHook,
CheckSignature, InitPropertiesHook, DoMainLoop, UpdateAppStep, UpdateApp, ShouldCheckForUpdate,
UpdateCheckQueried and OpenUpdateURL.

#### 1.2 Fields (absolute offsets; verified from a ctor store or an accessor body unless marked inferred)

| Off | Type | Name (1.2 name where the same) | Meaning / ctor value | Port field |
|---|---|---|---|--- |
| 0x000 | vfptr | | 0x506714 (BW 0x4fbda4) | |
| 0x004 | ulong | mRandSeed | GetTickCount() in Init, SRand(seed) | mRandSeed |
| 0x008 | string | mProdName | "Product"; BW ctor "BookWorm"; partner.xml "ProdName" | mProdName |
| 0x024 | string | mTitle | "SexyApp"; BW "BookWorm Deluxe " + mProductVersion; partner "Title" | mTitle |
| 0x040 | string | mRegKey | BW "PopCap\\BookWorm"; property "RegistryKey" overrides (ReadFromRegistry) | mRegKey |
| — | | (mCompanyName) | **absent** in 2004 | mCompanyName |
| 0x05c | int | mRelaxUpdateBacklogCount | 0 | same |
| 0x060 | int | mWidth | 640 | mWidth |
| 0x064 | int | mHeight | 480 | mHeight |
| 0x068 | int | mFullscreenBits | 16 | same |
| 0x070 | double | mMusicVolume | 0.85; BW ctor 0.5 | same |
| 0x078 | double | mSfxVolume | 0.85 | same |
| 0x080 | double | mDemoMusicVolume | 0 | same |
| 0x088 | double | mDemoSfxVolume | 0 | same |
| 0x090 | bool | mNoSoundNeeded | false (true: no DirectSound window, dummy music) | same |
| 0x091 | bool | mWantFMod | false | same |
| 0x094 | int | mBuildNum | atol of the first 8 chars of the build string (190) | SexyApp::mBuildNum |
| 0x098 | string | mBuildDate | "Thu Apr 15 14:52:08 2004" | SexyApp::mBuildDate |
| 0x0b4 | HANDLE | mMutex | single-instance mutex (mProdName+"Mutex") | — |
| 0x0b8 | bool | mOnlyAllowOneCopyToRun | true | same |
| 0x0bc | UINT | mNotifyGameMessage | RegisterWindowMessage("Notify"+mProdName) | — |
| 0x0c0 | CritSect | mCritSect | 24 bytes, InitializeCriticalSection (0x466880) | — |
| 0x0d8 | InternetManager* | mInternetManager | new InternetManager (0x538 bytes) in the ctor | SexyApp::mInternetManager (void*) |
| 0x0dc | BetaSupport* | mBetaSupport | new BetaSupport(this) (0x18c) in Init | mBetaSupport (void*) |
| 0x0e0 | bool | mBetaValidate | if set, Init exits unless BetaSupport validates (0x477930) | — |
| 0x0e1 | uchar[512] | mAdd8BitMaxTable | 0..255 then 255 | same |
| 0x2e1 | bool | mSkipAd | set by URLOpenSucceeded before Shutdown; skips the exit ad in Start | SexyApp::mSkipAd |
| 0x2e2 | bool | mDontUpdate | registry/partner "DontUpdate" | SexyApp::mDontUpdate |
| 0x2e4 | WidgetManager* | mWidgetManager | **created in Init** (`new WidgetManager(this)`, 0x174 bytes), 0 before | mWidgetManager |
| 0x2e8 | map<int,Dialog*> | mDialogMap | size at 0x2f0 | same |
| 0x2f4 | list<Dialog*> | mDialogList | | same |
| 0x300 | DWORD | mPrimaryThreadId | GetCurrentThreadId() **in the ctor** | — |
| 0x304 | bool | mSEHOccured | | same |
| 0x305 | bool | mShutdown | | same |
| 0x306 | bool | mIsWindowed | true; registry ScreenMode = !mIsWindowed; partner "DefaultWindowed" | mIsWindowed |
| 0x307 | bool | mForceFullscreen | | same |
| 0x308 | bool | mForceWindowed | | same |
| 0x309 | bool | mInitialized | end of Init | same |
| 0x30c | DWORD | mTimeLoaded | GetTickCount() | same |
| 0x310 | HWND | mHWnd | main window | mHWnd (SDL_Window*) |
| 0x314 | HWND | mInvisHWnd | "InvisWindow" (music/sound owner) | — |
| 0x318 | HWND | (external window, no 1.2 name) (inferred) | 0; when set MakeWindow uses it instead of creating one, ParseCmdLine and the idle check are skipped (host/embedding window, inferred) | — |
| 0x31c | list<MSG> | mDeferredMessages | node = hwnd,msg,wParam,lParam,time,pt (0x24 bytes) | — |
| 0x328 | bool | mNoDefer | false: input messages are queued and run from Start's loop | same |
| 0x32c | HTTPTransfer* | (no 1.2 name) | `new HTTPTransfer` (0xfc) in the ctor; only Abort()ed and polled in Shutdown | — |
| 0x330 | bool | (ITSuccess) | registry "ITSuccess": that transfer finished (result 0) | — |
| 0x334 | DDInterface* | mDDInterface | created by MakeWindow (0xcf0 bytes) | mDDInterface |
| 0x338 | bool | mAlphaDisabled | | same |
| 0x33c | MusicInterface* | mMusicInterface | CreateMusicInterface in Init | same |
| 0x340 | string | mReferId | registry/property "ReferId" | SexyApp::mReferId |
| 0x35c | string | mVariation | registry "Variation" | SexyApp::mVariation |
| 0x378 | ulong | mDownloadId | registry "DownloadId" | SexyApp::mDownloadId |
| 0x37c | string | mRegSource | "ingame" | SexyApp::mRegSource |
| 0x398 | bool | mReadFromRegistry | set by ReadFromRegistry; Shutdown writes the registry if set | same |
| 0x39c | string | mRegisterLink | "http://www.popcap.com/register.php?theGame="+mProdName+"&referid="+mReferId; registry/property "RegisterLink" | same |
| 0x3b8 | string | mProductVersion | GetProductVersion(exe) in the ctor | same |
| 0x3d4 | Image*[6] | mCursorImages | **6 cursors** (1.2: 13) | mCursorImages[NUM_CURSORS] |
| 0x3ec | bool | mIsOpeningURL | | same |
| 0x3ed | bool | mShutdownOnURLOpen | | same |
| 0x3f0 | string | mOpeningURL | | same |
| 0x40c | DWORD | mOpeningURLTime | | same |
| 0x410 | DWORD | mLastTimerTime | WM_TIMER tick | same |
| 0x414 | DWORD | mLastBigDelayTime | | same |
| 0x418 | ulong | mLastVerCheckQueryTime | registry; time(NULL) if absent | SexyApp:: same |
| 0x41c | string | mUserName | = mRegUserName when read; BW writes it (0x4173d2) | SexyApp::mUserName |
| 0x438 | string | mRegUserName | registry "RegName" | same |
| 0x454 | string | mRegCode | registry "RegCode" | same |
| 0x470 | bool | mIsRegistered | partner NoReg -> true; `|= Validate(name, code)` | same |
| 0x471 | bool | mBuildUnlocked | partner NoReg -> true | same |
| 0x472-0x487 | | (unused) (inferred) | never accessed (room of 1.2's mUnmuted*Volume doubles, I) | |
| 0x488 | int | mMuteCount | registry "Muted" | same |
| 0x48c | int | mAutoMuteCount | | same |
| 0x490 | bool | mDemoMute | | same |
| 0x494 | set<MemoryImage*> | mMemoryImageSet | AddMemoryImage 0x488b50 / RemoveMemoryImage 0x4874a0 (under the DD critsect) | same |
| 0x4a0 | int | mNonDrawCount | | same |
| 0x4a4 | int | mFrameTime | 10 (ms) | same |
| 0x4a8 | DWORD | mLastTime | | same |
| 0x4ac | DWORD | mLastUserInputTick | | same |
| 0x4b0 | int | mSleepCount | | same |
| 0x4b4 | int | mDrawCount | | same |
| 0x4b8 | int | mUpdateCount | | same |
| 0x4c0 | double | mUpdateMultiplier | 1.0 | same |
| 0x4c8 | bool | mPaused | demo 'P' toggles | same |
| 0x4cc | int | mFastForwardToUpdateNum | | same |
| 0x4d0 | bool | mFastForwardToMarker | | same |
| 0x4d4 | DWORD | mLastDrawTick | | same |
| 0x4d8 | DWORD | mNextDrawTick | | same |
| 0x4dc | int | mCursorNum | | same |
| 0x4e0 | SoundManager* | mSoundManager | new DSoundManager(mNoSoundNeeded ? NULL : mHWnd, mWantFMod) (0x14a0) in Init | same |
| 0x4e4 | HCURSOR | mHandCursor | CreateCursor(11,4) | — |
| 0x4e8 | HCURSOR | mDraggingCursor | CreateCursor(15,10) | — |
| 0x4ec | list<Widget*> | mSafeDeleteList | ProcessSafeDeleteList 0x480660 deletes all (Rand disallowed meanwhile) | same (with depth) |
| 0x4f8 | bool | mMouseIn | | same |
| 0x4f9 | bool | (left button down) | set/cleared by the mouse dispatcher; picks MouseDrag vs MouseMove | — |
| 0x4fa | bool | (right button down) | | — |
| 0x4fb | bool | (middle button down) | | — |
| 0x4fc | bool | mRunning | Start | same |
| 0x4fd | bool | mActive | 1; WM_ACTIVATEAPP | same |
| 0x4fe | bool | mMinimized | WM_SIZE | same |
| 0x4ff | bool | mHasFocus | 1 | same |
| 0x500 | int | mDrawTime | | same |
| 0x504 | ulong | mFPSStartTick | | same |
| 0x508 | int | mFPSFlipCount | | same |
| 0x50c | int | mFPSDirtyCount | | same |
| 0x510 | int | mFPSTime | | same |
| 0x514 | int | mFPSCount | | same |
| 0x518 | bool | mShowFPS | | same |
| 0x51c | int | mShowFPSMode | | same |
| 0x520 | int | mScreenBltTime | | same |
| 0x524 | bool | mLoadingThreadStarted | | same |
| 0x525 | bool | mLoadingThreadCompleted | set by the thread stub | same |
| 0x526 | bool | mLoaded | | same |
| 0x527 | bool | mYieldMainThread | true | same |
| 0x528 | bool | mLoadingFailed | Process calls Shutdown | same |
| 0x529 | bool | mCursorThreadRunning | | — |
| 0x52a | bool | mSysCursor | true | same |
| 0x52b | bool | mCustomCursorsEnabled | registry "CustomCursors" | same |
| 0x52c | int | mTimesPlayed | registry + %WINDIR%\popcinfo.dat cross-check; BW increments | SexyApp::mTimesPlayed |
| 0x530 | int | mTimesExecuted | ++ in Init | SexyApp::mTimesExecuted |
| 0x534 | bool | mTimedOut | | SexyApp::mTimedOut |
| 0x535 | bool | mLastShutdownWasGraceful | registry "InProgress"==0 | same |
| 0x538 | int | mNumLoadingThreadTasks | | same |
| 0x53c | int | mCompletedLoadingThreadTasks | BW LoadingThreadProc increments | same |
| 0x540 | bool | mRecordingDemoBuffer | -record | same |
| 0x541 | bool | mPlayingDemoBuffer | -play | same |
| 0x542 | bool | mManualShutdown | WM_CLOSE | same |
| 0x544 | string | mDemoFileName | "popcap.dmo" (no mDemoPrefix) | — |
| 0x560 | Buffer | mDemoBuffer | 0x20 bytes | — |
| 0x580 | int | mDemoLength | | — |
| 0x584/0x588 | int | mLastDemoMouseX/Y | | — |
| 0x58c | int | mLastDemoUpdateCnt | | — |
| 0x590 | bool | mDemoNeedsCommand | true | — |
| 0x591 | bool | mDemoIsShortCmd | | — |
| 0x594 | int | mDemoCmdNum | | — |
| 0x598 | int | mDemoCmdOrder | -1 | — |
| 0x59c | int | mDemoCmdBitPos | | — |
| 0x5a0 | bool | mDemoLoadingComplete | | — |
| 0x5a4 | map<HANDLE,int> | mHandleToIntMap | | — |
| 0x5b0 | int | mCurHandleNum | | — |
| 0x5b4 | list | mDemoMarkerList | | — |
| 0x5c0 | bool | mDebugKeysEnabled | Ctrl+Alt+D toggles | same |
| 0x5c3 | bool | mWaitForVSync | registry "WaitForVSync" | same |
| 0x5c4 | bool | mUserChanged3DSetting | | same |
| 0x5c5 | bool | mAutoEnable3D | false; **BookWormApp ctor sets true** (0x41a4f1): MakeWindow/Do3DTest (0x458bf0) may switch 3D on when the D3D test recommends it | same |
| 0x5c6 | bool | mTest3D | | same |
| 0x5c8 | DWORD | mMinVidMemory3D | 6 | — |
| 0x5cc | DWORD | mRecommendedVidMemory3D | 14 | — |
| 0x5d0 | map<string,string> | mStringProperties | (node isnil +0x45) | mStringProperties (wstring in port) |
| 0x5dc | map<string,bool> | mBoolProperties | | same |
| 0x5e8 | map<string,int> | mIntProperties | | same |
| 0x5f4 | map<string,double> | mDoubleProperties | | same |
| 0x600 | map<string,vector<string>> | mStringVectorProperties | | same |
| 0x60c | ResourceManager* | mResourceManager | `new ResourceManager(this)` (0xb4) in the ctor | same |
| 0x610 | int | (mOldWndProc?) (inferred) | 0, never read | — |

Not present in 2004 (do not look for them): mCompanyName, mFullCompanyName, mChangeDirTo, mUpdateAppState,
mUpdateAppDepth, mPendingUpdatesAcc, mUpdateFTimeAcc, mLastTimeCheck, mHasPendingDraw, mIsDrawing,
mLastDrawWasEmpty, mSharedImageMap, mCleanupSharedImages, mOverrideCursor, mCustomCursorDirty,
mMuteOnLostFocus (always on), mAutoStartLoadingThread (always on), mVSyncUpdates and the vsync-broken
test, mSyncRefreshRate, mIsPhysWindowed, mFullScreenWindow, mIsScreenSaver, mAllowMonitorPowersave,
mDemoPrefix, mStepMode, mCtrlDown/mAltDown (not initialised, never read), mMouseSensitivity.

#### 1.3 Construction, Init, Start, Shutdown

There is no InitHook, ShutdownHook, PreTerminate or InitPropertiesHook: a game's Init, Shutdown,
LoadingThreadProc, NewDialog and ButtonDepress overrides carry that logic, and the port overrides the same
methods.

- **ctor** (0x48f810): the field defaults above; LoadLibrary of version/ddraw/dsound; the product version; the
  default strings DIALOG_BUTTON_OK, DIALOG_BUTTON_CANCEL, UPDATE_CHECK_TITLE/BODY, UP_TO_DATE_TITLE/BODY and
  NEW_VERSION_TITLE/BODY; InternetManager, HTTPTransfer and ResourceManager. **No WidgetManager,
  MusicInterface, SoundManager or DDInterface yet**, so a game's ctor cannot rely on mWidgetManager.
- **Init** (0x48aaf0), in order: return if mShutdown; DirectX DLL check; single-instance mutex (a second copy
  broadcasts mNotifyGameMessage and exits); chdir to the exe folder unless ChangeDirHook; ParseCmdLine;
  `mRandSeed = GetTickCount(); SRand(mRandSeed)`; the demo file if -play;
  `LoadProperties("properties\\partner.xml", required=false, checkSig=true)`; NoReg sets mIsRegistered and
  mBuildUnlocked; ProdName; DefaultWindowed; Title (mTitle = Title + " " + mProductVersion); InternetManager;
  BetaSupport; `srand(GetTickCount())`; **ReadFromRegistry()**; ++mTimesExecuted; RegisterLink and DontUpdate;
  window classes; the invisible window; **the WidgetManager**; cursors; PreDisplayHook;
  `mWidgetManager->Resize(0,0,mWidth,mHeight)`; windowed only if the screen is larger than mWidth x mHeight;
  MakeWindow; DSoundManager; SetSfxVolume(mSfxVolume); CreateMusicInterface(mInvisHWnd);
  SetMusicVolume(mMusicVolume); mInitialized. The port's SexyAppBase::Init and InitPropertiesHook do the same
  with the properties before the registry.
- **Start** (0x480950): cursor thread; loading thread (always; the main thread goes to ABOVE_NORMAL); show and
  focus the window; timeBeginPeriod(1); then `while(!mShutdown){ PeekMessage/Dispatch all; ProcessDemo();
  ProcessDeferredMessages(); if(mShutdown) break; Process(); }`; afterwards ProcessSafeDeleteList,
  WaitForLoadingThread, the exit ad unless mSkipAd, **WriteToRegistry()**. There is no DoMainLoop or UpdateApp.
- **Shutdown** (0x47cbf0), once: mShutdown; demo volumes restored; wait for the cursor thread; StopAllMusic;
  abort the HTTPTransfer; restore the display mode when fullscreen; hide the window; WriteToRegistry() if
  mReadFromRegistry. 1.2's cross-thread check (mLoadingFailed when called from the loading thread) is absent.
- **~SexyApp** (0x48f000): Shutdown(); if mUserChanged3DSetting a message box about hardware acceleration and
  registry "Is3D"; deletes dialogs, windows, WidgetManager, DDInterface, MusicInterface, SoundManager. The
  port shows no prompt.

#### 1.4 Update/draw timing (Process 0x480700, DrawDirtyStuff 0x486510, Redraw 0x4884f0)

`Process()`: if mLoadingFailed, Shutdown(). Unless mPaused or mUpdateMultiplier <= 0:
`aFrameTime = (int)(mFrameTime / mUpdateMultiplier)` (10 ms) and `behind = now - mLastTime`. If
`behind >= aFrameTime` and (`++mNonDrawCount < 10` or `!mLoaded`): **one** DoUpdateFrames(),
ProcessSafeDeleteList(), `mLastTime += aFrameTime`, and the backlog is clamped to **1000 ms**. Otherwise
mNonDrawCount = 0, DrawDirtyStuff(), and a Sleep for the rest of the frame; while the loading thread runs
(mYieldMainThread) an extra `Sleep(min(250, 2*busy - slept))`.

`DrawDirtyStuff()` disallows Rand for its duration (MTRand::SetRandAllowed), runs the idle check once a second,
returns false while gScreenSaverActive, and draws when `mWidgetManager->DrawScreen()` drew something or 1 s has
passed since the last present, and `now >= mNextDrawTick`. It sleeps for VSync only if mWaitForVSync &&
mIsWindowed. While the loading thread runs, `mNextDrawTick += 35 + max(drawTime, 15)` holds the loading screen
to about 20 fps.

Against the port's 1.3 state machine: the same 10 ms tick and the same "at most 10 updates, then a forced draw
once loaded" rule; the port clamps the backlog at 200 ms, calls UpdateF after each update (2004 has none) and
has no loading-screen throttle. The idle check freezes updates and draws when the user has been idle longer
than the Windows screen-saver timeout minus 2 s; the port does not copy it.

#### 1.5 Input dispatch (WindowProc 0x489310 -> mDeferredMessages -> ProcessDeferredMessages 0x47fa40)

WindowProc queues key, char, mouse, timer, size, close and activate messages (unless mNoDefer) and handles
WM_PAINT, WM_SETCURSOR (EnforceCursor), WM_ENDSESSION (Shutdown) and Alt+Enter (SwitchScreenMode(!mIsWindowed),
only if mLoaded and not forced) at once. Start's loop runs ProcessDeferredMessages before every Process():

- **Keys**: a key down sets mLastUserInputTick; Ctrl+Alt+D toggles mDebugKeysEnabled; `if (!mDebugKeysEnabled
  || !DebugKeyDown(key)) WidgetManager::KeyDown(key)`; key up and WM_CHAR (one byte) go to WidgetManager::KeyUp
  and KeyChar. As the port.
- **Mouse**: every mouse message first sends `MouseDrag(x,y)` if any of the app's button flags
  0x4f9/0x4fa/0x4fb is set, else `MouseMove(x,y)`; then the buttons: L down 1, L double-click 2, R down -1,
  R double-click -2, M down 3 go to `MouseDown(x,y,code)` with SetCapture, ups to `MouseUp(x,y,code)`, the
  wheel to `MouseWheel(delta/120)`. Mouse leave is found on WM_TIMER by polling the cursor. The port passes the
  same click codes; its WidgetManager decides drag or move from mDownButtons, with the same result.
- **Focus** (RehupFocus 0x47cf50): focus = mActive && !mMinimized; on gain `Unmute(true)`,
  WidgetManager::GotFocus, GotFocus(); on loss `Mute(true)`, WidgetManager::LostFocus, LostFocus(). The
  framework always mutes while inactive (the port's mMuteOnLostFocus, default true); a game's GotFocus and
  LostFocus overrides pause it.
- WM_CLOSE on either window: mManualShutdown = true, Shutdown().
- WM_TIMER also times an opened URL out: after 5 s of ticks URLOpenSucceeded if the browser took the focus,
  URLOpenFailed if 12 s pass while the game is still active.

**EnforceCursor** (slot 10) does nothing while mWidgetManager is NULL. With !mSEHOccured && mMouseIn,
DDInterface draws the image cursor `mCursorImages[mCursorNum]` **only if mCustomCursorsEnabled or a demo is
playing**; otherwise the Windows cursors are used. There are **6 cursor ids**: 0 arrow, 1 hand, 2 dragging,
3 text, 4 circle-slash, 5 none. The port's enum has 13: ids 0 to 4 match (POINTER, HAND, DRAGGING, TEXT,
CIRCLE_SLASH), its 5 is SIZEALL, and 2004's 5 is the port's CURSOR_NONE (11), mapped by a game-side constant.
SetCursorImage takes ids 0 to 5; EnableCustomCursors is the registry's CustomCursors.

#### 1.6 Loading thread

StartLoadingThread always runs `LoadingThreadProc()` on its own `_beginthread` thread; the main thread keeps
running Process (the title screen), and DoUpdateFrames calls LoadingThreadCompleted() on the main thread at the
first update after the thread ends. The game's LoadingThreadProc bumps mCompletedLoadingThreadTasks for the
progress bar and checks mShutdown. The port runs it synchronously, so a progress bar jumps from empty to full.

#### 1.7 Registry, properties, files

- **Registry**: HKLM\SOFTWARE\<mRegKey>[\sub], the value name being the text after the last `\`; the port
  emulates "SOFTWARE\<mRegKey>\...". RegistryWrite (type, data, len), RegistryWriteString,
  RegistryWriteInteger, RegistryWriteBoolean (a DWORD 0/1), RegistryEraseValue, RegistryRead,
  RegistryReadString (**at most 255 bytes**), RegistryReadInteger, RegistryReadBoolean (any nonzero DWORD is
  true). No RegistryReadData/WriteData/EraseKey/GetSubKeys. All return false when mRegKey is empty, and all
  are recorded into and replayed from the demo buffer.
- **WriteToRegistry** (0x483a10): MusicVolume and SfxVolume = (int)(volume*100), Muted =
  (mMuteCount - mAutoMuteCount > 0), ScreenMode = !mIsWindowed, CustomCursors, ITSuccess, TimesPlayed,
  TimesExecuted, LastVerCheckQueryTime, InProgress = 0, WaitForVSync, RegName, RegCode; and, unless a demo
  plays, %WINDIR%\popcinfo.dat with records {u16 len, mProdName, u16 TimesPlayed, u16 TimesExecuted}, the
  store that stops a trial being reset.
- **ReadFromRegistry** (0x484800): mReadFromRegistry = true; mRegKey = GetString("RegistryKey", mRegKey), and
  nothing more if it is empty; then popcinfo.dat, ReferId (registry, then property), RegisterLink,
  MusicVolume/SfxVolume (int * 0.01), Muted (to mMuteCount), ScreenMode, CustomCursors, DontUpdate, ITSuccess,
  DownloadId, Variation, WaitForVSync, TimesPlayed/TimesExecuted (both become **100**, trial over, if they
  differ from popcinfo.dat), LastVerCheckQueryTime, InProgress (mLastShutdownWasGraceful = value==0, then 1 is
  written), RegName (to mUserName too), RegCode, `mIsRegistered |= Validate(name, code)`. The port keeps the
  volumes, screen mode and cursors and drops popcinfo.dat and the trial rule.
- **Properties**: LoadProperties(file, required, checkSig) returns true for a missing file unless required;
  checkSig verifies file+".sig" (RSA/BigInt), which the port skips. Accessors: GetBoolean(id, default),
  GetInteger(id, default), GetString(id, default), GetString(id) ("" when missing), SetString, SetBoolean,
  SetInteger, SetDouble; no GetDouble or GetStringVector, though the maps exist. BookWorm's partner.xml sets
  ReferId, NoReg, DontUpdate, NoInternetScores, NumOptionsLinks, OptionsLinkText0/URL0 (a "more games" link,
  never clicked in tests), ShowTrialStatus, LoadingBarY, OptionsDialogHeight and NoIHSUpsell.
- **Files**: ReadBufferFromFile (whole file, "rb"), WriteBytesToFile, WriteBufferToFile, FileExists (fopen
  "rb"), on plain CRT paths relative to the exe folder. The port has the same methods through its VFS, and
  writes go to the user's folder.

**3D and the FPU (inferred).** Because BookWormApp sets mAutoEnable3D, a 3D-capable machine may run with
DDInterface::mIs3D on. DDInterface::Init (0x4619f0) calls SetCooperativeLevel with DDSCL_NORMAL (8) windowed or
FULLSCREEN|EXCLUSIVE (0x11), never DDSCL_FPUPRESERVE, so once the Direct3D7 device exists the x87 control word
may be at 24-bit precision on the main thread; game double math before `_ftol` would then round like float.
Only matters for bit-exact reproduction of float-derived values; the port computes in full precision.

#### 1.8 Other non-virtual SexyApp functions

Virtual in 1.2 and non-virtual here: GetImage (always commits bits; no shared images), IsAltKeyUsed,
CheckSignature, ShouldCheckForUpdate (false if mDontUpdate or checked less than 7 days ago after a clean exit)
and UpdateCheckQueried. With 1.2's names and meaning: ClearUpdateBacklog(relaxForASecond),
WaitForLoadingThread, SetCursor, SetCursorImage, EnableCustomCursors, CopyImage, FlipImage, Is3DAccelerated
(mDDInterface->mIs3D), Is3DAccelerationSupported, Is3DAccelerationRecommended, Set3DAccelerated(is3D, reinit),
CopyToClipboard, GetClipboard, ProcessSafeDeleteList, AddMemoryImage, RemoveMemoryImage, Remove3DData, the
DemoSync family and the property and registry accessors of 1.7. The image helpers are in section 5. The demo
recorder, the debug displays (F9 memory, F11 dump, FPS), the SEH catcher with its report windows and
InternetManager's update check are not ported.

## 5. Graphics, images and fonts

Symbols: `games/bookworm/docs/symbols/10_framework_graphics.jsonl`.

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
In framework code 00468620 is also `Buffer::GetDataPtr()` and other `this+8` reads.

### 1. Color (no vtable, 0x10)

Fields +0 mRed, +4 mGreen, +8 mBlue, +0xc mAlpha, and the behaviour of 1.2 and the port: `Color()` is
(0,0,0,255); `Color(int 0xAARRGGBB)` turns alpha 0 into 255; `Color(r,g,b)` and `Color(const int* rgb)` have
alpha 255; `ToInt()` is a<<24 | r<<16 | g<<8 | b. `Color(r,g,b,a)` is the folded 0x437e90.

### 2. Graphics (0x58, vtable 00502210, 1 slot)

The one virtual is the destructor.

| off | type | name | meaning | port |
|---|---|---|---|--- |
| +0x00 | vfptr | | 00502210 | |
| +0x04 | Image* | mDestImage | NULL in the ctor becomes `&mStaticImage` (005698e8, a 0x0 Image) | mDestImage |
| +0x08 | **int** | mTransX | translation (1.2 and port: float) | mTransX (float) |
| +0x0c | **int** | mTransY | | mTransY (float) |
| +0x10 | Rect | mClipRect | (0,0,dest w,dest h) | mClipRect |
| +0x20 | Color | mColor | default (0,0,0,255) | mColor |
| +0x30 | Color | (unknown) | default-constructed by both ctors, **not copied** by the copy ctor, read by no linked function | none |
| +0x40 | Font* | mFont | NULL | mFont |
| +0x44 | int | mDrawMode | 0 normal, 1 additive | mDrawMode |
| +0x48 | bool | mColorizeImages | false | mColorizeImages |
| +0x49 | bool | mFastStretch | false | mFastStretch |
| +0x4c..+0x57 | 12 bytes | probably mPFActiveEdgeList / mPFNumActiveEdges / mPFNumVertices (inferred) | never initialised, no PolyFill linked | same names |

Absent compared with 1.2/port: mWriteColoredString (a **static on Widget** here, 0052f96c, value 1),
mLinearBlend, mScaleX/Y, mIs3D, mStateStack (no PushState/PopState in the binary).

Every other function is non-virtual and has the port's name and meaning: the constructors `Graphics(Image*)`
(NULL selects the static 0x0 image, so `Graphics(NULL)` has a 0x0 clip rect) and `Graphics(const Graphics&)`;
`Create()` (= `new Graphics(*this)`); Get/SetFont, Get/SetColor, Get/SetDrawMode, Get/SetColorizeImages,
SetFastStretch, `Translate(int, int)`; `DrawString` (`if (mFont) mFont->DrawString(this,x,y,s,mColor,mClipRect)`);
`StringWidth` (no NULL check, as the port); `FillRect` (returns if mColor.mAlpha == 0); `DrawRect` (the outline of
(w+1)x(h+1)); `DrawImage(img,x,y)` (Image slot 6 Blt, colour = mColorizeImages ? mColor : White);
`DrawImage(img,x,y,src)` (returns silently if src exceeds the image); the stretched `DrawImage(img,dest,src)`
(dest is clipped here, src becomes a double rect, Image slot 9); `DrawImageRotated(img, int x, int y, rot, src)`
(00450230), which calls `DrawImageRotatedF(img, float x, float y, rot, src)`, whose centre is the **integer**
w/2, h/2 (the port's float overload uses w/2.0f); `DrawImageRotatedF(img,x,y,rot,cx,cy,src)` (Image slot 8
BltRotated with mClipRect); `ClipRect`; and `DrawImageBox(dest,img)` / `DrawImageBox(src,dest,img)` (3x3
**tiling**: corners, tiled edges and centre with clip rects, like the port).

Not in the binary: DrawImageF, DrawImageMirror (there is no BltMirror slot), DrawImageCel, DrawImageAnim,
DrawImage(img,x,y,w,h), DrawLine/DrawLineAA, PolyFill, ClearRect, SetClipRect/ClearClipRect,
PushState/PopState, DrawImageMatrix/Transform, DrawTrianglesTex, and the text methods, which are Widget's.

Translate a call to `DrawImageRotated(int ...)` (00450230) to the port's **int** overload, never to
`DrawImageRotatedF`: both use integer centres, and float centres differ by half a pixel for odd sizes. A
stretched `DrawImage(dest, src)` pre-clips dest and passes an FRect src where the port passes the clip rect to
StretchBlt, with the same pixels, and both filter linearly only when !fastStretch. `Graphics(NULL)` measures
text with a 0x0 clip in both.

### 3. Text layout lives on Widget in 2004 (WriteString, WriteWordWrapped, colour codes)

WriteCenteredLine, WriteString, WriteWordWrapped, GetWordWrappedHeight and WriteNumberFromStrip are Widget
virtuals taking a `Graphics*`: section 3, subsections 1.1 (slots 34 to 40) and 1.4.

### 4. Image (0x18, vtable 00502218, 10 slots)

| slot | 2004 method | port | notes |
|---|---|---|---|
| 0 +00 | `~Image()` | `~Image` | body 00458fa0 deletes mAnimInfo |
| 1 +04 | `FillRect(const Rect&, const Color&, int mode)` (empty) | `FillRect` | 1.2 slot 1 PolyFill3D does not exist yet |
| 2 +08 | `DrawRect(const Rect&, const Color&, int)` | `DrawRect` | 4 FillRects |
| 3 +0c | `ClearRect(const Rect&)` (empty) | `ClearRect` | |
| 4 +10 | `DrawLine(double,double,double,double, const Color&, int)` (empty) | `DrawLine` | MemoryImage does not override: only DDImage draws lines |
| 5 +14 | `FillScanLines(Span*, int, const Color&, int)` | `FillScanLines` | |
| 6 +18 | `Blt(Image*, int x, int y, const Rect& src, const Color&, int mode)` (empty) | `Blt` | |
| 7 +1c | `BltF(Image*, float x, float y, const Rect& src, const Rect& clip, const Color&, int)` (empty) | `BltF` | |
| 8 +20 | `BltRotated(Image*, float x, float y, const Rect& src, const Rect& clip, const Color&, int, double rot, float cx, float cy)` (empty) | `BltRotated` | |
| 9 +24 | `StretchBlt(Image*, const Rect& clippedDest, const FRect& src, const Color&, int mode, bool fast)` (empty) | `StretchBlt` (port: `(Image*, dest, Rect src, clip, colour, mode, fast)`) | **different signature**: no clip rect, double source rect |

Not present: PolyFill3D, BltMatrix, BltTrianglesTex, BltMirror, StretchBltMirror (and the port's
DrawLineAA, FillScanLinesWithCoverage).

Fields (verified, ctor 00446cf0 and copy ctor 00460190): +0 vfptr, +4 mWidth, +8 mHeight,
+0xc mNumRows (1), +0x10 mNumCols (1), +0x14 AnimInfo* mAnimInfo. No mNumTotal, mDrawn, mFilePath (the
same as Zuma 1.0 / CircleShootApp). Port: same names plus those three.
`CopyAttributes(Image*)` copies rows, cols and AnimInfo (0x30, laid out as 1.2: type, delay, numCels, two
vectors, total time). There is no GetCelWidth, GetCelRect or GetAnimCel.

### 5. MemoryImage (0x48, vtable 00502ab4, 26 slots)

| slot | 2004 method | port | notes |
|---|---|---|---|
| 0 +00 | `~MemoryImage()` | same | body 00459970: RemoveMemoryImage, frees buffers |
| 1 +04 | `FillRect` | same | |
| 2 +08 | `DrawRect` (Image) | same | |
| 3 +0c | `ClearRect` | same | |
| 4 +10 | `DrawLine` (Image, empty) | port draws | |
| 5 +14 | `FillScanLines` (Image) | same | |
| 6 +18 | `Blt` | same | mode 0 -> NormalBlt 00448540, 1 -> AdditiveBlt 00447af0 |
| 7 +1c | `BltF` | same | = BltRotated(..., rot 0, centre 0,0) |
| 8 +20 | `BltRotated` | same | clip helper 0044c520 |
| 9 +24 | `StretchBlt(img, dest, FRect src, colour, mode, fast)` | same name | fast -> FastStretchBlt 0044a4e0, else SlowStretchBlt 004491e0 |
| 10 +28 | `void* GetNativeAlphaData(int bitDepth, ulong rMask, ulong gMask, ulong bMask)` | `GetNativeAlphaData(NativeDisplay*)` | no NativeDisplay in 2004 |
| 11 +2c | `uchar* GetRLAlphaData()` | same | |
| 12 +30 | `uchar* GetRLAdditiveData(int, ulong, ulong, ulong)` | same (NativeDisplay*) | |
| 13 +34 | `DeleteSWBuffers()` | same | |
| 14 +38 | `Delete3DBuffers()` -> mApp->Remove3DData(this) | same | |
| 15 +3c | `DeleteExtraBuffers()` | same | |
| 16 +40 | `BitsChanged()` | same | ++mBitsChangedCount, mBitsChanged, drops alpha caches |
| 17 +44 | `CommitBits()` | same | recomputes mHasTrans/mHasAlpha unless mForcedMode |
| 18 +48 | `DeleteNativeData()` | same | |
| 19 +4c | `Clear()` | same | |
| 20 +50 | `SetBits(ulong* bits, int w, int h)` | `SetBits(bits,w,h,commitBits=true)` | **no commitBits**: always BitsChanged + CommitBits |
| 21 +54 | `Create(int w, int h)` | same | sets mHasTrans = mHasAlpha = true |
| 22 +58 | `ulong* GetBits()` | same | allocates zeroed bits or expands the palette |
| 23 +5c | `SetImageMode(bool hasTrans, bool hasAlpha)` | same | sets mForcedMode |
| 24 +60 | `SetVolatile(bool)` | same | |
| 25 +64 | `bool Palletize()` | same | Quantize32Bit 0044a800 |

Not present: PurgeBits, ReInit (1.2 slots 18, 22), BltMatrix, BltTrianglesTex, the HD API.

Fields (ctor 004598f0 `MemoryImage(SexyAppBase*)`, copy ctor 00461350, dtor 00459970):

| off | type | name | port |
|---|---|---|--- |
| +0x18 | ulong* | mBits (w*h+1, last = 0x4BEEFADE) | mBits |
| +0x1c | int | mBitsChangedCount | same |
| +0x20 | void* | mD3DData | same |
| +0x24 | DWORD | mD3DFlags (inferred) | same |
| +0x28 | ulong* | mColorTable (256 entries) | same |
| +0x2c | uchar* | mColorIndices | same |
| +0x30 | bool | mForcedMode | same |
| +0x31 | bool | mHasTrans | same |
| +0x32 | bool | mHasAlpha | same |
| +0x33 | bool | mIsVolatile | same |
| +0x34 | ulong* | mNativeAlphaData | same |
| +0x38 | uchar* | mRLAlphaData | same |
| +0x3c | uchar* | mRLAdditiveData | same |
| +0x40 | bool | mBitsChanged | same |
| +0x44 | SexyAppBase* | mApp | same |

No mPurgeBits, mWantPal. The ctor registers the image with the app (AddMemoryImage, the set at app+0x494) and
the dtor removes it. A game makes one with `new MemoryImage(app)` (0x48), Create, SetImageMode(true,true) and
`new Graphics(img)`, as in the port.

### 6. DDImage (0xc4, vtable 00502b34, 37 slots)

Slots 0 to 25 are MemoryImage's in the same order. DDImage overrides 0, 1 FillRect, 4 DrawLine, 6 Blt, 7 BltF,
8 BltRotated, 9 StretchBlt, 15 DeleteExtraBuffers, 16 BitsChanged (also releases the surface), 17 CommitBits
(only without a surface), 21 Create (which, unlike MemoryImage's and the port's, does **not** set
mHasTrans/mHasAlpha), 22 GetBits (reads the surface back) and 25 Palletize. Its own slots:

| slot | 2004 method | port | notes |
|---|---|---|---|
| 26 +68 | `SetVideoMemory(bool)` | non-virtual stub | |
| 27 +6c | `RehupFirstPixelTrans()` | not in port | |
| 28 +70 | `NormalFillRect(const Rect&, const Color&)` | not in port | |
| 29 +74 | `AdditiveFillRect` | not in port | |
| 30 +78 | `NormalBlt(Image*, int, int, const Rect&, const Color&)` | not in port | |
| 31 +7c | `AdditiveBlt` | not in port | |
| 32 +80 | `NormalDrawLine(double x4, const Color&)` | not in port | |
| 33 +84 | `AdditiveDrawLine` | not in port | |
| 34 +88 | `bool LockSurface()` | non-virtual stub | |
| 35 +8c | `bool UnlockSurface()` | non-virtual stub | |
| 36 +90 | `SetSurface(LPDIRECTDRAWSURFACE)` | non-virtual stub | |

No Normal/AdditiveBltMirror (1.2 slots 41/42). Non-virtual: `DDImage(DDInterface*)`, `GenerateDDSurface()`
(00440e40), `DeleteDDSurface()`, `GetSurface()`.

Fields: +0x48 DDInterface* mDDInterface, +0x4c mSurface, +0x50 bool mNoLock (inferred), +0x51 bool
mVideoMemory, +0x52 bool mFirstPixelTrans (inferred), +0x53 bool mWantDDSurface (set by GenerateDDSurface and
by game code), +0x54 int mLockCount, +0x58 DDSURFACEDESC mLockedSurfaceDesc (0x6c). No mSurfaceSet,
mDrawToBits. The port has mDDInterface, mVideoMemory and mWantDDSurface.

A game makes an opaque offscreen buffer with `new DDImage(app->mDDInterface)` (0xc4), Create and
`SetImageMode(false,false)`. BookWorm's helper after each image load is `if (!img->Palletize()) { if
(app->Is3DAccelerated()) ddimg->mWantDDSurface = true; else ddimg->GenerateDDSurface(); }`: the port keeps
`Palletize()` and drops the surface branch.

### 7. ImageLib and the alpha-mask convention

`ImageLib::GetImage(const std::string& theFileName, bool lookForAlphaImage)` (00491750, cdecl) opens loose
files with `fopen`. With an explicit extension only that loader runs (`.tga`, `.jpg`, `.png`, `.gif`,
case-insensitive); without one they are tried in that order (no .bmp, .jpeg, .j2k). TGA is 32-bit with image
descriptor 0x28 only; a GIF's transparent index becomes pixel 0x00000000. `ImageLib::Image` (0x10) is +4
mWidth, +8 mHeight, +0xc mBits.

With lookForAlphaImage (always true from the app's GetImage) it also loads `dir + "_" + name`, else
`theFileName + "_"` (when a path mixes `/` and `\`, dir ends at the earlier of the two last separators). A mask
of the same size replaces the colour image's alpha with the mask's **low byte (blue)**; a mask of another size
is silently ignored; a mask alone gives a white image with that alpha. There is no other variant.

The port's `platform/ImageLoader.cpp` LoadGameImage follows the same rules and adds to them (.jpeg and .bmp,
any mask extension, mods, HD). One difference: for a GIF's transparent pixels stb_image keeps the palette RGB
with alpha 0 where SAF stores 0.

### 8. App image helpers called by the game

| 2004 | port (`SexyAppBase`) | notes |
|---|---|---|
| `DDImage* GetImage(const std::string& theFileName)` (0047d930) | `GetImage(file, commitBits=true)` | ImageLib::GetImage(file, true), `new DDImage(mDDInterface)`, `SetBits(bits,w,h)`; NULL on failure; no mFilePath, no commitBits argument |
| `DDImage* CopyImage(Image*)`, `CopyImage(Image*, const Rect&)` | same | new DDImage, Create(w,h), `Graphics(img).DrawImage(src,-x,-y)`, CopyAttributes |
| `void FlipImage(Image*)` | `FlipImage` | a **vertical** flip (swap rows, BitsChanged) |

Not in the binary: GetSharedImage, CreateColorizedImage, ColorizeImage, CreateCrossfadeImage, MirrorImage,
RotateImageHue.

### 9. Font (0x14, 12 slots)

The slots are 1.2's and the port's in the same order: 0 dtor, 1 GetAscent, 2 GetAscentPadding, 3 GetDescent
(= mHeight - mAscent), 4 GetHeight, 5 GetLineSpacingOffset, 6 GetLineSpacing (= mHeight + mLineSpacingOffset),
7 StringWidth (returns 0), 8 CharWidth (= StringWidth(string(1,c))), 9 CharWidthKern (= CharWidth(c)),
10 `DrawString(Graphics*, int x, int y, const string&, const Color&, const Rect& clip)` (empty), 11 Duplicate
(pure). Fields: +4 mAscent, +8 mAscentPadding, +0xc mHeight, +0x10 mLineSpacingOffset.

### 10. ImageFont (0x48, 23 slots)

The slots are 1.2's and the port's in the same order: 0 to 6 as Font, 7 StringWidth (the sum of
CharWidthKern(c, prev)), 8 CharWidth (= CharWidthKern(c, 0)), 9 CharWidthKern (the maximum over the active
layers, scaled by mScale or the point size), 10 DrawString (calls DrawStringEx(..., &clip, NULL, NULL)),
11 Duplicate, 12 GenerateActiveFontLayers, 13 `DrawStringEx(Graphics*, int x, int y, const string&, const
Color&, const Rect* clip, RectList* drawnAreas, int* width)`, 14 SetPointSize, 15 GetPointSize, 16 SetScale,
17 GetDefaultPointSize, 18 AddTag (false if HasTag(tag), which is case-sensitive; else pushes
StringToUpper(tag)), 19 RemoveTag (finds StringToUpper(tag)), 20 HasTag (exact case), 21 GetDefine ("" if
undefined), 22 Prepare.

Constructors: `ImageFont(SexyAppBase*, const std::string& descFile)` (the one games use),
`ImageFont(Image*, const std::string&)` (legacy) and the copy constructor (shares the FontData by Ref; an owned
scaled image is duplicated with CopyImage). Fields: +0x14 FontData* mFontData, +0x18 int mPointSize
(= mDefaultPointSize), +0x1c StringVector mTagVector, +0x2c bool mActiveListValid, +0x30 ActiveFontLayerList
mActiveLayerList, +0x40 double mScale (1.0). There is **no mForceScaledImagesWhite** (1.2 +0x48), so a game
subclass starts at +0x48.

GenerateActiveFontLayers and DrawStringEx are the 1.2 algorithms (checked in the asm) and the port's, including
the **integer** division in scale = mScale * (mPointSize / layerPointSize) and the static pool of 4096
RenderCommands; on pool overflow 2004 drops the remaining characters and the port draws them directly.
DrawStringEx ignores theClipRect, and *theWidth is the final x minus the start x.

A game reports a failed font load when `font->mFontData->mInitialized` is false. BookWorm's ShadowFont (an
ImageFont subclass, 0x60) calls `ImageFont::DrawStringEx` non-virtually, once for the shadow and once for the
text, and measures with DrawStringEx on a `Graphics(NULL)`, so its widths include kerning exactly as drawing
does; all of that exists in the port.

### 11. FontData and the descriptor parser

FontData (0x198) has **no DescParser base** in 2004: the parser is part of it. Fields: +0x04 bool
mInitialized, +0x08 int mRefCount, +0x0c mApp, +0x10 int mDefaultPointSize, +0x14 uchar mCharMap[256], +0x114 a
cache of layer images by path (the port uses GetSharedImage), +0x120 mFontLayerList, +0x12c mFontLayerMap,
+0x138 mSourceFile, +0x154 mFontErrorHeader, +0x170 mCurrentLine, +0x18c mDefineMap (keys upper-cased).
FontLayer (0x1207c), CharData (0x120), ActiveFontLayer (0x1010), RenderCommand (0x34) and the DataElement
family have the fields of 1.2 and the port, except that FontLayer::mImage (+0x12044) is a raw `Image*` owned by
that cache.

`FontData::Load` returns false if already initialised or if the file is missing (no error). A `#` as first
non-blank character comments the line out; `'` and `"` quote, with backslash escapes inside; `;` outside quotes
ends a command; `\r` is ignored. An error pops up through the app's Popup (slot 21); the port's
`DescParser::LoadDescriptor` with CMDSEP_SEMICOLON reads these files the same way and logs instead. The
commands are 1.2's 25 (Define, CreateHorzSpanRectList, SetDefaultPointSize, SetCharMap, CreateLayer,
CreateLayerFrom and the Layer... setters); the port has them and adds LayerSetHeight. LayerSetImage resolves its path against the descriptor's folder, loads through `mApp->GetImage(path)` and calls
`Palletize()`.

### 12. SysFont (0x1c, 12 slots): not in the port

A GDI font, `SysFont(SexyAppBase*, const std::string& face, int pointSize, int script, bool bold, bool italics,
bool underline)`: StringWidth by DrawTextEx, DrawString by TextOut on the destination DDImage's surface with
the alpha ignored. It is created only as a fallback ("SansSerif") when a widget or dialog draws text without a
font, and by the debug overlay. The port's widgets skip text without a font, so a translation keeps every
SetFont call (section 8, row 5).

## 6. Sound and music

The sound layer is SAF 1.2's DSoundManager/DSoundInstance and BassMusicInterface/FModMusicInterface with members
removed (no sound by name, no ReleaseSound, no music unload, pause-all or maximum volume); what remains is
almost line for line 1.2. Sizes are in section 4's table; the port's classes are SDLSoundManager,
SDLSoundInstance and SDLMusicInterface.

`SexyApp::Init` makes `new DSoundManager(mNoSoundNeeded ? NULL : mHWnd, mWantFMod)`, calls SetSfxVolume, then
`CreateMusicInterface(mInvisHWnd)` and SetMusicVolume. CreateMusicInterface returns a plain MusicInterface if
mNoSoundNeeded, an FModMusicInterface if mWantFMod, otherwise a BassMusicInterface. Only the SexyApp ctor
writes the two flags (false), so these games always use DirectSound and BASS, and the FMOD classes are dead
code (no fmod.dll is installed). The default volumes are 0.85, as in the port.

#### 1. SoundManager / DSoundManager

DSoundManager : SoundManager, 14 slots (1.2 has 16, the port 18).

| slot (+off) | method | port method | notes |
|---|---|---|---|
| 0 (+00) | `~DSoundManager()` scalar deleting | `~SDLSoundManager` | body 00492e00 |
| 1 (+04) | `bool Initialized()` = mDirectSound != NULL | `Initialized()` | |
| 2 (+08) | `bool LoadSound(unsigned int theSfxID, const std::string& theFilename)` ret 8 | `LoadSound(unsigned, const std::string&)` | 1.2 slot 2 `int LoadSound(const std::string&)` does **not exist** |
| 3 (+0c) | `bool SetBaseVolume(unsigned int id, double v)` ret 0xc | `SetBaseVolume` | reordered: before SetVolume |
| 4 (+10) | `bool SetBasePan(unsigned int id, int pan)` ret 8 | `SetBasePan` | |
| 5 (+14) | `void SetVolume(double v)` ret 8 (= sfx volume) | `SetVolume` | |
| 6 (+18) | `SoundInstance* GetSoundInstance(unsigned int id)` ret 4 | `GetSoundInstance` | |
| 7 (+1c) | `void ReleaseSounds()` | `ReleaseSounds` | |
| 8 (+20) | `void ReleaseChannels()` | `ReleaseChannels` | |
| 9 (+24) | `double GetMasterVolume()` | `GetMasterVolume` | Windows mixer, see 1.4 |
| 10 (+28) | `void SetMasterVolume(double v)` ret 8 | `SetMasterVolume` | Windows mixer |
| 11 (+2c) | `void Flush()` (folded empty `ret`) | `Flush` | |
| 12 (+30) | `void SetCooperativeWindow(HWND, bool isWindowed)` ret 8 | `SetCooperativeWindow` | always DSSCL_NORMAL |
| 13 (+34) | `void StopAllSounds()` | `StopAllSounds` | keeps each instance's mAutoRelease |
| - | (absent) `int LoadSound(const std::string&)`, `void ReleaseSound(unsigned)` | exist in port | |
| - | (absent) | `GetFreeSoundId`, `GetNumSounds` (1.3) | |

It holds 256 source buffers (one per sound id, +0x4), their sizes, base volumes (+0x808, 1.0) and base pans
(+0x1008, 0), 32 channels (+0x1408), `mMasterVolume` (+0x1488: **the sfx volume** set by SetVolume, not by
SetMasterVolume) and mDirectSound (+0x1494, NULL without a device). 1.2's file-name table and primary buffer
are gone. The ctor only sets DSSCL_NORMAL (1.2 tries DSSCL_PRIORITY and a 44.1 kHz 16-bit stereo primary
buffer), so sounds play at their file's rate.

##### 1.3 How samples are loaded and named

`LoadSound(id, name)` (004954f0):
1. `id >= 256` returns false. Without a device it returns **true** and loads nothing. It does not release a
   buffer already in the slot.
2. The cache name is the directory of `name` + `"cached_"` + base name + `".wav"`.
3. It tries, stopping at the first success: `cached_<name>.wav` (valid only if its `dep ` chunk's FILETIME
   equals the source file's last-write time), `<name>.wav` (PCM only), `<name>.mp3` (FMOD, never),
   `<name>.ogg` (Tremor, 16-bit; true only if every byte decoded), `<name>.au` (`.snd`, big-endian header;
   encoding 1 = 8-bit mu-law to 16-bit, 2 = 8-bit linear).
4. After decoding an .ogg or .au it writes `cached_<name>.wav` with a `dep ` chunk naming the source and its
   FILETIME; the write fails silently on a read-only folder. An installed game has a cached WAV for every
   sound, so a normal run never decodes Ogg.
5. Every loader makes a static secondary buffer (flags 0x80ea) at the file's own rate and format.

A game loads its sounds in Init and shuts down if a LoadSound returns false, so **a missing sound file quits
the game silently**. It plays them through `SexyApp::PlaySample(id)` = `GetSoundInstance(id)->Play(false,
true)`. `GetSoundInstance` returns NULL for `id > 256` (1.2's off-by-one) or an id without a buffer, a silent
instance without a device, and otherwise an instance on a free channel with the id's base pan and volume.
FindFreeChannel sweeps released channels every 1000 ms and returns -1 when all 32 are busy; PlaySample then
does nothing.

##### 1.4 Volumes, pan, pitch

- Sfx volume: `SexyApp::SetSfxVolume` calls `mSoundManager->SetVolume(muted ? 0 : v)`, and every channel sets
  `VolumeToDB(mBaseVolume * mVolume * mMasterVolume)`, where `VolumeToDB(v) = (int)((log10(1+9v)-1)*2333)`
  centi-dB and anything below -2000 is -10000. The port's `SDLSoundManager::VolumeToGain` uses the same curve.
  Above 0 dB DirectSound refuses the call and keeps the buffer's last volume; the port clamps to 0 dB.
- Master volume is **the Windows wave-out mixer** (GetMasterVolume/SetMasterVolume read and write its volume
  control as v * 0xFFFF); `SexyApp::SetMasterVolume` also stores v as mSfxVolume. BassMusicInterface's ctor
  saves it before BASS_Init and writes it back. The port's SetMasterVolume is SetVolume.
- Pan: `buffer->SetPan(mBasePan + mPan)`, hundredths of a dB (-10000..10000), unclamped; the port clamps.
- AdjustPitch: `freq = mDefaultFrequency * 1.0594630943592953^steps`, clamped to [100, 100000] Hz; the port
  uses the ratio 2^(steps/12) without the clamp.

#### 2. SoundInstance / DSoundInstance

DSoundInstance : SoundInstance, 12 slots in the order of 1.2 and the port; every method is byte-identical to
Big Money's.

Slots: 0 dtor, 1 Release (= Stop(); mReleased = true), 2 SetBaseVolume, 3 SetBasePan, 4
`AdjustPitch(double theNumSteps)`, 5 SetVolume, 6 SetPan, 7 `bool Play(bool looping, bool autoRelease)` (Stop(),
mHasPlayed, mAutoRelease = autoRelease, then play), 8 Stop (stops, rewinds and clears mAutoRelease, only when a
buffer exists), 9 IsPlaying (mHasPlayed and the buffer is playing), 10 IsReleased (auto-releases a finished
autoRelease instance), 11 GetVolume.

Fields (0x38), as 1.2: mSoundManagerP, mSourceSoundBuffer, mSoundBuffer (a DuplicateSoundBuffer of the source),
mAutoRelease, mHasPlayed, mReleased, mBasePan, mBaseVolume, mPan, mVolume, mDefaultFrequency.

#### 3. MusicInterface / BassMusicInterface / FModMusicInterface

15, 15 and 17 slots (1.2: 20/20/22; the port's MusicInterface 20). The base's slots are folded empty stubs
whose `ret` sizes prove the signatures.

| slot (+off) | method | port method |
|---|---|---|
| 0 (+00) | `~X()` scalar deleting | dtor |
| 1 (+04) | `bool LoadMusic(int theSongId, const std::string& theFileName)` | `LoadMusic` |
| 2 (+08) | `void PlayMusic(int theSongId, int theOffset, bool noLoop)` | `PlayMusic` |
| 3 (+0c) | `void StopMusic(int)` | `StopMusic` |
| 4 (+10) | `void PauseMusic(int)` | `PauseMusic` |
| 5 (+14) | `void ResumeMusic(int)` | `ResumeMusic` |
| 6 (+18) | `void StopAllMusic()` | `StopAllMusic` |
| 7 (+1c) | `void FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop)` | `FadeIn` |
| 8 (+20) | `void FadeOut(int theSongId, bool stopSong, double theSpeed)` | `FadeOut` |
| 9 (+24) | `void FadeOutAll(bool stopSong, double theSpeed)` | `FadeOutAll` |
| 10 (+28) | `void SetSongVolume(int, double)` | `SetSongVolume` |
| 11 (+2c) | `bool IsPlaying(int)` | `IsPlaying` |
| 12 (+30) | `void SetVolume(double)` | `SetVolume` |
| 13 (+34) | `void SetMusicAmplify(int, double)` | `SetMusicAmplify` (gain table measured against BASS 2.3, c553853) |
| 14 (+38) | `void Update()` | `Update` |

FModMusicInterface adds slots 15 and 16, two LoadSample overloads. 1.2's UnloadMusic, UnloadAllMusic,
PauseAllMusic, ResumeAllMusic and SetSongMaxVolume do not exist; the port has them.

BassMusicInterface (0x14): +0x4 `std::map<int, BassMusicInfo> mMusicMap`, +0x10 `int mMaxMusicVolume` (40, used
only on the BASS 1.x path of SetVolume). BassMusicInfo (0x20): +0 HMUSIC mHMusic, +4 HSTREAM mHStream, +8
double mVolume, +0x10 double mVolumeAdd, +0x18 bool mStopOnFade; there is no mVolumeCap (the cap is the
constant 1.0). The port's `SDLMusicInterface::SongInfo` has the same members.

The games ship `bass.dll` **2.0.0.20**. The ctor calls `BASS_Init(1, 44100, 0, hwnd, NULL)` and
`BASS_SetConfig(BASS_CONFIG_BUFFER, 2000)`. BASS_ChannelSetPosition is never loaded, so playing a stream with
an offset > 0 would jump through a garbage pointer; BASS_MusicFree is not imported.

##### 3.2 Music semantics

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

#### 4. Behaviour differences and what the port must do

1. **Slots**: read calls through `mSoundManager` (app+0x4e0) and `mMusicInterface` (app+0x33c) with the tables
   above, not 1.2's, and call the port's method of the same name.
2. **Music master volume**: the BASS 2.0 path of SetVolume sets the global MOD volume to v
   (`BASS_CONFIG_GVOL_MUSIC`, v*100, linear), where the port's `SDLMusicInterface::SetVolume` uses
   v * mMaxMusicVolume / 100 = 0.4 v, the BASS 1.x rule. The game side sets `mMaxMusicVolume = 100` on the
   music interface its app creates (an override of CreateMusicInterface) and calibrates the libopenmpt render
   level against BASS 2.0 (default amplify 50, linear interpolation), by ear or with a capture: the balance of
   music and sound depends on it.
3. **Double music Update** (BookWorm): its UpdateFrames override calls the base, whose last step is
   `mMusicInterface->Update()`, and then Update() again, so every fade moves 2 x speed per tick (0.005 is a
   1.0 s full fade at 100 updates/s). Translate the override literally; do not "fix" it.
4. **MO3 order offsets**: a module's offset goes to `BASS_MusicPlayEx(h, MAKELONG(order,row), flags, TRUE)`,
   so the games' offsets are order numbers at row 0; the port maps them to `Mixer::PlaySong(order, row)` and
   `openmpt_module_set_position_order_row`. Each LoadMusic of the same file is an independent module
   instance, in BASS and in the port. BASS follows backward position jumps (Bxx) even without BASS_MUSIC_LOOP
   and stops a non-looping song only at the end of the order list, while libopenmpt with repeat count 0 stops
   at its detected song end: one-shot jingles (noLoop) are where this shows.
5. **Already matched**: the FadeIn, FadeOut, PlayMusic and Update rules of 3.2 (offset -1 is the port's
   `PlaySong(..., resetPosition=false)`). BASS volumes are whole percents, the port's continuous.
6. **Volume ramping** (minor, inferred): modules load with BASS_MUSIC_RAMP and PlayMusic keeps it, but FadeIn
   with an offset replays without it. The port does not ramp unless asked.
7. **Sound lookup order**: the port tries `.ogg`, `.wav`, `.au`, `.mp3` and has no cache, where the original
   tries the cache, `.wav`, `.ogg`, `.au`. The result differs only if a mod ships both a .wav and an .ogg of
   one name. The port never writes `cached_*` files.
8. **Load failure**: the original's LoadSound returns true without a device and false only for a missing or
   undecodable file; the port returns false for a missing file even without a device. The game's shutdown
   check is kept, so silent and headless runs need the files to resolve.
9. **Master volume**: the Windows mixer in the original, the sfx volume in the port; unused by BookWorm.
10. **DirectSound setup**: DSSCL_NORMAL only and no primary-buffer format. Nothing to do.
11. **Removed members** (LoadSound(name), ReleaseSound, UnloadMusic, UnloadAllMusic, PauseAllMusic,
    ResumeAllMusic, SetSongMaxVolume, mVolumeCap): game code cannot call them, and a game's own pause-all
    loops over its songs.
12. **FMOD**: dead code, not ported.
13. **32 channels**: the port has the same limit and rule.

## 7. Support classes and library helpers

### 7.1 Support classes

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

**MTRand** (0x9c4: `ulong mt[624]`, `int mti` at +0x9c0; the port's layout). `SRand(ulong)` (004665f0) is SAF
1.2's: seed 0 becomes 4357, `mt[0] = seed; mt[i] = mt[i-1] * 69069`, mti = 624. The port's is 1.3's
`1812433253 * (x ^ x>>30) + i`, so the same seed gives another stream; Next (masked with 0x7fffffff),
tempering, Serialize and SRand(string) are identical. A game's own generators (`new MTRand(Sexy::Rand())`) are
therefore seeded through the shim's `SeedMTRand2004` (Big Money: `SeedMTRand12`), which any seed-based
reproduction needs; the global stream (`Sexy::SRand(GetTickCount())` in Init) matters only for deterministic
tests. SRand(string) copies the words but not mti, in the original and in the port, so a loaded generator
continues from its fresh generator's mti (624): load original saves with it, not with SetState. Not in this
build: `Next(range)`, `Next(float)`, `Sexy::Rand(int)`, `Sexy::Rand(float)`; games take ranges with `% n`.
`MTRand::SetRandAllowed(bool)` is a counter the app raises while it draws and while it deletes widgets.

**Buffer** (0x20: vfptr, `std::vector<uchar> mData` +4, mDataBitSize +0x14, mReadBitPos +0x18, mWriteBitPos
+0x1c) is 1.2's and the port's, bit for bit: WriteNumBits is LSB first, a Boolean one byte, Short and Long
little-endian, a String a short length and the bytes, WriteLine appends "\r\n", ReadByte returns 0 on underflow.
`ToWebString()` is "%08X" of mWriteBitPos and ceil(bits/6) characters of `.-0-9A-Za-z`; FromWebString needs at
least 4 characters and decodes lowercase hex in the length as `c - 'f' + 10` (a 1.2 bug kept in the port and
never triggered, because ToWebString writes uppercase; do not fix it, or old saves could change meaning). Not
linked: WriteBuffer, ReadBuffer, GetCRC32, GetBitsRequired, PastEnd, GetDataLenBits, SetData(ByteVector).

**Common helpers** are 1:1 with `framework/Common.h`: StrFormat, StrFindNoCase, StringToInt, StringToDouble,
URLEncode, StringToUpper, StringToLower, Upper, Trim, CommaSeperate, XMLDecodeString, Evaluate, GetFileDate,
GetCurDir, GetPathFrom, GetFullPath, GetFileDir (no withSlash parameter: the port's `GetFileDir(path, false)`),
RemoveTrailingSlash, MkDir (splits on `\` only), Deltree. Not linked: FileExists (SexyApp has its own),
GetFileName, AddTrailingSlash, XMLEncodeString, StrPrefixNoCase, StringToWString, Lower. Details that could
matter:

- StringToInt accepts an optional leading '-', decimal digits, or hex after "0x"/"0X" (so "-x1F" is -31); no
  '+', no spaces; empty or bad input gives false and 0. StringToDouble: digits, one '.', a leading '-' only.
- XMLDecodeString knows lt, amp, gt, quot, apos, nbsp and cr; an unknown `&name;` becomes "&", and a '&' with
  no later ';' is kept.
- CommaSeperate(0) is "0" and a negative gives "", as in the port.
- StrFormat gives up at 1,000,000 characters and cuts at an embedded NUL; the port does neither.

**PropertiesParser and XMLParser** are 1.2's narrow-character code (the port's XMLParser is 1.3's wide one,
with the same grammar for these files). Only `ParsePropertiesBuffer(const Buffer&)` is linked. The root element
must be `Properties`; sections are `<String id>`, `<StringArray id>` (of `<String>`), `<Boolean id>`
(Upper(value) in 1/YES/ON/TRUE or 0/NO/OFF/FALSE), `<Integer id>` and `<Double id>`; anything else is "Invalid
Section". The XML parser accepts elements with attributes, end tags, empty elements, text (runs of whitespace
collapse to one space), `<?...?>` instructions and comments; only double quotes delimit attribute values,
unquoted values are accepted, and keys, values and text go through XMLDecodeString.

**HTTPTransfer** (0xfc, a member of game classes that show online scores) has Get(url), Get(base, rel),
Post(url, params), Abort, GetResultCode and GetContent; result codes 0 DONE, 1 NOT_STARTED, 2 NOT_COMPLETED,
3 NOT_FOUND, 4 HTTP_ERROR, 5 ABORTED, 6 SOCKET_ERROR, 7 INVALID_ADDR, 8 CONNECT_FAIL, 9 DISCONNECTED. It is not
in the port: a game drops its use (as BookWorm's Hall of Fame does) or stubs it game-side so that it never
connects. **InternetManager** (update check, ads), BetaSupport, SEHCatcher, PerfTimer, ModVal, BigInt,
DirectXErrorString and ResourceManager are dropped; partner.xml's NoReg, DontUpdate and NoInternetScores
already select the offline paths.

**DDInterface** (0xcf0, app+0x334; no NativeDisplay base) is replaced by the port's SDL DDInterface. Game code
only does `new DDImage(app->mDDInterface)`. `mIs3D` is at +0x10 (Is3DAccelerated; set from the registry's
"Is3D" before Init) and the D3DTester at +0xc: `Is3DAccelerationSupported()` is tester && mFailureReason empty,
`Is3DAccelerationRecommended()` also needs mWarning empty. Init requires a 16- or 32-bit display in a window
and calls SetDisplayMode(mWidth, mHeight, mFullscreenBits) in full screen.

### 7.2 std::string, STL, iostream and CRT helpers called by game code

The toolchain is MSVC 7.0 (VS.NET 2002) with its Dinkumware STL, the static CRT and the old pre-standard
iostream library (`ifstream`, `ios`, `filebuf` without `std::`). The port uses the same std containers, so the
answer is almost always "write the obvious C++".

##### Conventions

- Every member is plain `__thiscall` (ECX = this, the callee pops the stack). Ghidra prints a one-argument
  member as `__fastcall(int param_1)`, which is the same thing. A function returning a class object takes a
  hidden result pointer as its first stack argument.
- **Identical-COMDAT folding** (/OPT:ICF; section 3, subsection 0) also merges template code across framework
  and game: all `std::list<T>` helpers with a 4-byte T, the tree helpers of maps with the same value size. A
  body that embeds a string literal (the "too long" throws) or calls itself (`_Erase`) is not folded, so there
  is one copy per .obj; the `_2`, `_3` suffixes in the names are these copies.
- Layouts (VC7): `std::string` is 28 bytes = allocator +0, union {char buf[16]; char* ptr} +4, size +0x14,
  capacity +0x18 (heap when capacity > 15). `std::list`/`set`/`map` are 12 bytes = allocator +0, head node +4,
  size +8. `std::vector` is 16 bytes = allocator +0, first +4, last +8, end of storage +0xc.

##### How to read the decompilation (and what to write in the port)

1. **SSO buffer test**: `if (*(uint*)(s+0x18) < 0x10) p = s+4; else p = *(char**)(s+4);` (also written
   `0xf < cap`) is `s.c_str()`; `*(int*)(s+0x14)` is `s.size()`.
2. **Destructor**: `if (0xf < cap) _free(ptr); cap = 0xf; size = 0; buf[0] = 0;` (or a call to
   `std::string::_Tidy`) is `~string()`: let the object go out of scope. The same three stores without the free
   are the default constructor.
3. **Literal assignment**: an inline strlen loop followed by `assign_ptr_len(s, "abc", n)` is `s = "abc"`; the
   loop also precedes an append (`s += "abc"`), a compare and `"abc" + s`.
4. **Copy**: `assign_substr(dst, src, 0, 0xffffffff)` is `dst = src`; with other arguments it is
   `dst.assign(src, pos, n)` or `src.substr(pos, n)`.
5. **Temporaries of operator+**: `string_plus_cstr(&tmp, a, "x")` and its relatives, each followed by the
   temporary's destructor, are `a + "x"`, `a + b`, `"x" + a`; a chain of them is one expression.
6. **Comparisons**: `compare(...) == 0` is `s == p`. In map and set code the compare is inlined as a byte loop
   `(byte)*a < (byte)*b` over `min(len)` and then a length tie-break: `std::less<std::string>`, identical to
   libc++.
7. **`_free(p)` is `operator delete`** (a 5-byte `jmp free`). `operator_new(n)` + a constructor call + a store
   is `new T(...)`; `if (p) { p->vtbl[0](1); }` is `delete p`.
8. **std::list**: node {next +0, prev +4, value +8}. `for (n = *(int**)head; n != head; n = *n)` with `n[2]`
   is a loop over the list with `*it`. `_Buynode(head, head->prev, &v)` + two pointer stores + `_Incsize(1)` is
   `l.push_back(v)`; `_Buynode(head->next, head, &v)` is `push_front`; `erase_range(l, &tmp, head->next, head)`
   is `l.clear()`. Ghidra often hides the unlink stores and `--size`: trust the call, not the body.
9. **set/map (_Tree)**: node {left +0, parent +4, right +8, value +0xc, color, isnil}; `head->parent` is the
   root, `head->left` is `begin()`, `head->right` the rightmost. The **isnil byte** sits at
   `0xd + sizeof(value_type)` and is the loop terminator seen everywhere (`while (*(char*)(n + OFF) == 0)`):
   **+0x15** for an 8-byte value (`map<int,T*>`, `map<int,bool>`), **+0x29** for `set<std::string>`, **+0x45**
   for `map<std::string,std::string>`. A descent loop comparing keys is `insert`/`find`/`lower_bound`;
   `iterator::_Inc` / `_Dec` are `++it` / `--it`. Write `m[key]`, `m.find(key)`, `s.insert(v)`.
10. **std::vector**: `first == 0 ? 0 : (last - first) / sizeof(T)` is `v.size()` (the null test is part of
    VC7's size()); push_back is inlined as `if (first && size < capacity) { _Uninit_fill_n(last, 1, &v);
    last++; } else _Insert_n(v, last, 1, &v);`. Growth is 1.5x where libc++ doubles, which matters only if game
    code keeps raw pointers into a vector across a push_back.
11. **Throw helpers**: the functions Ghidra names `std::length_error::ctor_or_dtor_*` or
    `std::out_of_range::ctor_or_dtor_*` in the game region are container functions that contain a throw
    (`list::_Incsize`, `vector::_Xlen`, `_Tree::_Insert`, `_Tree::erase(iterator)`). The game never catches
    them: translate the container operation and drop the check.
12. **EH noise**: `ExceptionList`, `puStack_c`, `local_8 = N` (EH state numbers) and the `Catch@`/`Unwind@`
    funclets are compiler bookkeeping for destructors on unwind. `__chkstk` is a stack frame over 4 KB.
13. **`_ftol()`** after x87 math: Ghidra shows it without an argument because the operand is on the FPU stack;
    read the disassembly (`fld`/`fmul`/`fild`...) for the expression. It is `(int)x`, truncation toward zero,
    and it returns 64 bits of which the game uses EAX, so values outside the int range wrap instead of
    saturating.
14. **Old iostream (no `std::`)**: `ifstream::ifstream`, `istream::get_str` (= `getline(buf, n)` when the
    `_fGline` member at +4 is incremented just before) and `ios::lock/unlock` are
    `std::ifstream f(path); while (f.getline(buf, n)) ...` in the port.
15. **Ghidra hides the stores** of several small helpers (struct copies, `_Uninit_copy`, `_Uninit_fill_n`,
    `basic_streambuf::_Init`): check the disassembly before concluding a helper does nothing.

##### Behaviour the port must replicate exactly

- **CRT `rand()`** is `holdrand = holdrand * 214013 + 2531011; return (holdrand >> 16) & 0x7fff;`, seeded once
  with `srand(GetTickCount())` in SexyApp::Init. It is not `Sexy::Rand`. The shim's `CrtRand()` / `CrtSRand()`
  keep the 15-bit range, so `rand() % N` keeps its distribution (Emscripten's RAND_MAX is 2^31-1); never route
  it through Sexy::Rand, whose sequence other game code depends on.
- **`_ftol`** is `(int)x`, truncation toward zero; never `lround` or `floor`. For x87 precision see section 4,
  "3D and the FPU".
- **`_stricmp`** is ASCII and folds to *lower* case before comparing; `strcasecmp` in the C locale gives the
  same result.
- **`isalpha`/`isspace`/`toupper`/`tolower`** use the "C" locale tables (bytes >= 0x80 are not alpha and not
  case-mapped). Where a game calls `isalpha` on a Windows virtual-key code, the port must feed it the same key
  codes.
- **`atoi`** skips leading whitespace, takes an optional sign and decimal digits, and wraps on overflow.
- **`std::getline`** has standard semantics. The old `getline(char*, n)` reads at most n-1 characters and,
  unlike std, sets no state bit when it stops at the length limit (the rest of the line is read as the next
  line); a file opened in text mode delivers CRLF as LF, so the port opens in binary and strips a trailing
  `\r`.
- **fread/fwrite of raw data** (saves): little-endian 4-byte ints and 1-byte bools, the same on wasm32. Keep
  the field widths when translating (no `size_t` or `long` growth).
- **`std::multiset::insert`** puts an equal key after the existing equal ones; a port using the same predicate
  reproduces a table's order exactly.

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
