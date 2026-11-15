# The 2004 SexyAppFramework: compatibility map onto our port

BookWorm Deluxe, Typer Shark! Deluxe and AstroPop Deluxe (early 2004) are built on an internal SexyAppFramework
snapshot older than any public release. For every class game code touches, this map gives the vtable slots and
fields, what each is in SAF 1.2 and in the port (`framework/`, the SAF 1.3 API on SDL2), and where the old
behaviour differs. A translation calls the port's methods by name; the map says which name a
`(**(code **)(*this + 0x34))(g)` in the decompilation is, and what the old code did that the port must reproduce.

Conventions. Addresses are virtual addresses in BookWorm Deluxe 1.03's game PE
(`re/bookworm/bin/BookwormGame.exe`, image base 0x400000, linked 2004-04-15 with VS.NET 2002 / MSVC 7.0). Every
framework function there is named in `games/bookworm/docs/symbols/10_framework_*.jsonl`, so a function this map
does not list, or an address a code comment cites, is found there. `(**(code **)(*this + 0xNN))` is slot NN/4 of
the object's primary vtable; a slot's offset is always slot * 4. "1.2" is SAF 1.2 as built into Big Money Deluxe;
its slot numbers and field offsets are in `games/bigmoney/docs/FRAMEWORK_12.md` under the same names. "port" is
`framework/`; "BW" is BookWormApp. A port field has the same name as the 2004 one unless a row says otherwise.
Facts come from constructor stores, accessor bodies or the disassembly; inferred ones are marked. Subsection numbers
are local to each section.

## 1. How the map was made

No source of this framework generation is public. The map combines RTTI (`tools/rtti_msvc70.py` reads it itself,
because Ghidra 12 recovers no classes from MSVC 7.0 binaries), byte matching against Big Money Deluxe
(`tools/funcmatch.py`: the same compiler, SAF 1.2, named slot by slot) and a reading of every function game code
calls against the public SAF 1.2 (`E:\retools\refs\saf12`) and 1.3 trees. alula/CircleShootApp (Zuma 1.0.0, 2003,
also MSVC 7.0) was read as a guide only.

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

What sets this generation apart from the public trees: one application class (`Sexy::SexyApp`, 67 virtual slots,
derived only from `ButtonListener`, holding what 1.2 splits between `SexyAppBase` and `SexyApp`); no
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
an earlier list; they are section 8's rows: D1 is row 1, D2 row 3, D4 row 4, D5 row 5, D7 and D8 rows 14-18, D11
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
4. **Widget vtable is 47 slots** and in a different order from 1.2. Game classes start their own virtuals at
   slot 47 (Widget-derived), 49 (ButtonWidget-derived) and 52 (Dialog-derived). Secondary interface vtables sit at
   **+0x58** (= sizeof(Widget)) and, in dialogs, at **+0x110** (= sizeof(Dialog)) / +0x120 / +0x124 (after game
   members).
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

The port has the method of the same name; the notes give the port's different name or behaviour.

slot | 2004 method | notes
---|---|---
0 | ~Widget() | frees mColors
1 | AddedToManager(WidgetManager*) | only sets mWidgetManager (port: WidgetContainer's)
2 | RemovedFromManager(WidgetManager*) | mWidgetManager = NULL
3 | OrderInManagerChanged() | empty
4 | SetVisible(bool) | hide: MarkDirtyFull, show: MarkDirty; RehupMouse
5 | SetColors(int[][4], int n) | replaces mColors, MarkDirty
6 | SetColors(int[][3], int n) |
7 | SetColor(int idx, const Color&) | grows mColors
8 | const Color& GetColor(int idx) | grows mColors with Color() when out of range (1.2/port return a static)
9 | SetTransient(bool) (name inferred) | +0x1c flag; hide: MarkDirtyFull first; only CursorWidget uses it; not in port
10 | SetDisabled(bool) | as 1.2 (DisableWidget, re-MousePosition)
11 | ShowFinger(bool) | no NULL check of mWidgetManager
12 | Resize(const Rect&) |
13 | Resize(int x, int y, int w, int h) | MarkDirtyFull, set, MarkDirty, RehupMouse
14 | Move(int x, int y) |
15 | bool WantsFocus() | constant false (no mWantsFocus field)
16 | Draw(Graphics*) | empty
17 | Update() | mUpdateCnt++
18, 19 | GotFocus(), LostFocus() | GotFocus sets mHasFocus = true
20 | KeyChar(char) | empty
21 | KeyDown(KeyCode) | TAB: SetFocus(shift ? mTabPrev : mTabNext)
22 | KeyUp(KeyCode) | empty
23 | MouseEnter() | **sets mIsOver and ShowFinger(true) if mDoFinger** (port: WidgetManager does this)
24 | MouseLeave() | mIsOver = false, ShowFinger(false) if mDoFinger
25 | MouseMove(int x, int y) | empty
26 | MouseDown(int x, int y, int btn, int cc) | empty
27 | MouseDown(int x, int y, int cc) | **sets mIsDown = true**, then slot 26 with (2,1) for cc==3, (0,cc) for cc>=0, (1,-cc) for cc<0
28 | MouseUp(int x, int y, int btn, int cc) | empty
29 | MouseUp(int x, int y, int cc) | calls slot 30 first, then slot 28 (same split as slot 27)
30 | MouseUp(int x, int y) | **mIsDown = false** (port: WidgetManager clears it)
31 | MouseDrag(int x, int y) | empty
32 | MouseWheel(int delta) | empty; WM sends it to the focus widget
33 | bool IsPointVisible(int x, int y) | true
34 | Rect WriteCenteredLine(Graphics*, int off, const string&, Color c1, Color c2, const Point& shadow) | shadow in c2 at +shadow, text in c1
35 | Rect WriteCenteredLine(Graphics*, int off, const string&) |
36 | int WriteString(Graphics*, const string&, int x, int y, int w=-1, int just=-1, bool draw=true, int off=0, int len=-1) | the full colour-code writer lives here in 2004 (1.4)
37 | int WriteWordWrapped(Graphics*, const Rect&, const string&, int lineSpacing, int just) | 1.4
38 | int GetWordWrappedHeight(Graphics*, int w, const string&, int lineSpacing) | WriteWordWrapped into a NULL Graphics, Rect(0,0,w,0), just -1
39 | int GetNumDigits(int) |
40 | WriteNumberFromStrip(Graphics*, int n, int x, int y, Image* strip, int spacing) | identical
41 | bool Contains(int x, int y) |
42 | Rect GetInsetRect() |
43 | Rect GetRect() | port: WidgetContainer::GetRect
44 | bool Intersects(Widget*) | port: WidgetContainer
45 | MarkDirtyFull() | wm ? wm->MarkDirtyFull(this) : mDirty = true
46 | MarkDirty() | wm ? wm->MarkDirty(this) : mDirty = true

Not in 2004 (present in 1.2/port): UpdateF, UpdateAll/UpdateFAll/DrawAll, MarkDirty(WidgetContainer*) /
MarkDirtyFull(WidgetContainer*) as Widget virtuals, DrawOverlay, GetColor(idx, default), all child management,
Layout, DeferOverlay. Game code cannot call them, so nothing to translate.

#### 1.2 Fields

Offset | type | name | meaning
---|---|---|---
+00 | vfptr | |
+04 | ulong | mLastWMUpdateCount | last WM update count seen (UpdateFrame calls Update once per frame)
+08, +0c, +10, +14 | int | mX, mY, mWidth, mHeight | screen coordinates
+18 | bool | mDirty | ctor **true**
+19 | bool | mVisible | ctor true
+1a | bool | mMouseVisible | ctor true; receives mouse
+1b | bool | mClip | ctor true; DrawScreen clips to the widget only when set
+1c | bool | mTransient (name inferred) | drawn on top of a buffered background every redraw; excluded from dirty propagation; not in port
+1d, +1e, +1f, +20 | bool | mDisabled, mHasFocus, mIsDown, mIsOver | mIsDown/mIsOver set and cleared by the base mouse handlers
+21 | bool | mHasTransparencies (inferred) | stops the "fully covered" shortcut in MarkDirtyFull
+22 | bool | mHasAlpha | MarkDirty escalates to MarkDirtyFull
+24 | WidgetManager* | mWidgetManager |
+28 | std::vector<Color> (16) | mColors | first +0x2c, last +0x30
+38 | Insets (16) | mMouseInsets | mLeft +0x38, mTop +0x3c, mRight +0x40, mBottom +0x44
+48 | bool | mDoFinger |
+4c | int | mUpdateCnt | Update() counter
+50, +54 | Widget* | mTabPrev, mTabNext |

No `mWantsFocus`, `mParent`, `mWidgets`, `mPriority`, `mZOrder`, `mWidgetFlagsMod`, `mUpdateIteratorModified`.
Static data: `Widget::mWriteColoredString` at 0x52f96c = true.

#### 1.4 WriteString / WriteWordWrapped (in Widget, not Graphics)

Text layout is a set of Widget virtuals taking a `Graphics*` (slots 34 to 38 and 40); the port's equivalents are
`Graphics` methods, and the shim's `WriteString2004` / `WriteWordWrapped2004` / `GetWordWrappedHeight2004`
(`framework/compat2004/Compat2004.h`, which states the differences: no skipping of spaces after a wrap, no height for
a trailing newline, a `^` near the end of the string ends it before a `^^` test, the colour restored only by
WriteWordWrapped) reproduce the 2004 bodies (section 8, row 6). Always pass the justification the binary pushes: the
port's `Graphics::WriteString` defaults to 0 (centre), the 2004 Widget's to -1. WriteString is 0x43b420 (the 1.2
`Graphics::WriteString` body; colour codes are gated by the static `Widget::mWriteColoredString`, 0x52f96c, true),
WriteWordWrapped 0x439f70 (the 1.2 algorithm: no clip-rect culling, no theMaxWidth/theMaxChars/theLastWidth, no indent;
returns `yOffset + descent - lineSpacing`), GetWordWrappedHeight 0x4389f0.

### 2. WidgetManager

sizeof **0x174** (allocated by SexyApp at 0x48aaf0, `operator new(0x174)`; 1.2: 0x1d4). No base class;
vtable `0x5017a0` has 1 slot (deleting dtor 0x43aae0; body 0x43a9c0). App field `mWidgetManager` is at
app+0x2e4 as in 1.2. ctor `0x43eb10(SexyApp*)`.

#### 2.1 Fields

Offset | type | name | meaning
---|---|---|---
+00 | vfptr | |
+04 | int | mUpdateCnt | UpdateFrame counter
+08 | Widget* | mDefaultTab | ctrl+TAB target
+0c | SexyApp* | mApp | game reads it (`mWidgetManager->mApp`)
+10 | Image* | mImage | screen image, set by the app
+14 | DDImage* | mTransientImage | background buffer for transient widgets, created in DrawScreen
+18 | bool | mLastHadTransients |
+1c, +20 | int | mWidth, mHeight |
+24 | Widget* | mPopupCommandWidget | removed on a click outside it
+28 | CursorWidget* | mCursorWidget (inferred name) | created by the ctor, deleted by the dtor; not in port
+2c | bool | mShowCursorWidget (inferred name) | if set: AddWidget/BringToFront keep the cursor widget on top and mouse moves move it to (mouse - hotspot); not in port
+30 | std::list<Widget*> | mWidgets | head +0x34, size +0x38; back = top
+3c | bool | mHasFocus | ctor true
+40, +44, +48 | Widget* | mFocusWidget, mLastDownWidget, mOverWidget |
+4c | Widget* | mBaseModalWidget | single pointer, no PreModalInfo stack
+50 | bool | mMouseIn |
+54, +58 | int | mLastMouseX, mLastMouseY | game reads them
+5c | int | mDownButtons | bit0 left, bit1 right, bit2 middle (no separate mActualDownButtons)
+60 | bool | mUpdateIteratorModified |
+64 | list iterator | mUpdateIterator |
+68 | bool[255] | mKeyDown | VK-indexed; +0x78 = shift, +0x79 = control
+168 | DWORD | mLastMouseDownTick (name inferred) | GetTickCount() at every MouseDown; **read by the game** (Lex::Update 0x42dd20: idle > 12000 ms); **not in port** (section 8, row 3)
+16c | int | mLastDownButtonId | 1 left, -1 right, 2 middle
+170 | bool | mDirty | set by MarkDirty/MarkDirtyFull, cleared and returned by UpdateFrame

Missing vs 1.2/port: mCurG, the deferred-overlay vector, all FlagsMod members, mPreModalInfoList,
mActualDownButtons, mLastInputUpdateCnt, mWidgetFlags, mMouseDestRect/mMouseSourceRect.

#### 2.2 Functions (all direct calls)

The port's method has the same name unless noted.

2004 function | notes
---|---
WidgetManager(SexyApp*) | creates the CursorWidget
FreeResources() (inferred) | deletes mTransientImage only
Resize(int w, int h) | drops mTransientImage (port: Resize(Rect,Rect))
AddWidget(Widget*) | if absent: push_back (top), w->AddedToManager(this), MarkDirty(w); then re-raise cursor widget (if +0x2c) and RehupMouse
RemoveWidget(Widget*) | MarkDirtyFull(w), erase (update-iterator safe), DisableWidget(w), w->RemovedFromManager(this)
SetFocus(Widget*) | old->LostFocus() **even if it is the same widget**; focus only if HasWidget; GotFocus if mHasFocus
BringToFront(Widget*) | move to back of list, OrderInManagerChanged; no dirty marking
PutInfront(Widget* w, Widget* ref) | used by Dialog and ListWidget too; no PutBehind is linked
MarkAllDirty() | sets every widget's mDirty (not WM+0x170)
DrawWidgetsTo(Graphics*) | every visible widget, clipped when mClip
MarkDirty(Widget*), MarkDirtyFull(Widget*) | 1.2 algorithms (port: WidgetContainer::MarkDirty(WidgetContainer*)); transient widgets do not propagate; Full stops at an opaque widget fully covering the rect clipped to the screen
SetBaseModal(Widget*) | port: AddBaseModal / RemoveBaseModal; see 2.3
DisableWidget(Widget*) | over -> MouseLeave; lastdown -> DoMouseUps; focus -> LostFocus; basemodal -> NULL
DoMouseUps(Widget*, ulong downCode) | **passes the button index 0/1/2 as click count** (1.2/port: 1/-1/3)
GetWidgetAt(int x, int y) | top-down; skips !mVisible, !mMouseVisible; tests GetInsetRect().Contains then IsPointVisible(local); a disabled hit returns NULL; stops after passing mBaseModalWidget
MousePosition(int x, int y) | as 1.2 (MouseLeave old, MouseEnter + MouseMove new); moves the cursor widget if +0x2c
MouseDown(int x, int y, int cc) | also stores GetTickCount() in +0x168
MouseUp(int x, int y, int cc) | button bit from cc (cc<0 right, cc==3 middle, else left); MouseUp(x,y,cc) on mLastDownWidget, then MousePosition
MouseMove, MouseDrag, MouseExit | MouseMove: mMouseIn = true, MousePosition; MouseDrag on mLastDownWidget, over-state follows the cursor
MouseWheel(int) | to mFocusWidget
KeyChar(char) | ctrl+TAB -> mDefaultTab->KeyChar(TAB)
KeyDown / KeyUp (KeyCode) | mKeyDown[] for 0..254; KeyUp swallows TAB while ctrl is down
GotFocus() / LostFocus() | LostFocus sends KeyUp for every held key
bool UpdateFrame() | mDirty = false; mUpdateCnt++; Update() on **every** widget (visible or not) whose mLastWMUpdateCount differs; returns mDirty
bool DrawScreen() | see 2.4

Also present with the same names and the 1.2 behaviour: HasWidget, BringToBack, IsBelow, RemovePopupCommandWidget,
RehupMouse; `Rect::Intersection` is shared with Graphics.

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

ButtonListener (vtable 0x4f93ac, 6 slots; 1.2 has 7; port `ButtonListener.h` has 7, call by name): the slots are
those of subsection 0 item 3; slot 2 is `ButtonPress(id, cc) { ButtonPress(id); }`. The port's ButtonMouseMove is
never called for 2004 code. In a Widget-derived game class this vtable is at +0x58; in SexyApp it is the primary
vtable (slots 0 to 5). `this` inside a +0x58 thunk is `object+0x58`, so `this+0xa8` there is a Dialog's `mId` (+0x100)
and `this-0x34` is `mWidgetManager` (+0x24).

Other interfaces (1 vtable each, all defaults are folded stubs; same order as 1.2 and the port unless noted):
CheckboxListener 0 CheckboxChecked(id, checked); SliderListener 0 SliderVal(id, double); EditListener 0
EditWidgetText(id, const string&), 1 bool AllowKey(id, KeyCode), 2 bool AllowChar(id, char), 3 bool
AllowText(id, const string&) (Allow* default true); ListListener 0 ListClicked(id, idx, clickCount), **only one
slot** (port adds ListClosed and ListHiliteChanged, no-op defaults); ScrollListener 0 ScrollPosition(id, double pos)
(ListWidget implements it at +0x58). There is no DialogListener in 2004 (the port's Dialog uses it; D1).

A Dialog subclass puts an EditListener, ListListener, SliderListener or CheckboxListener vtable at +0x110 or
after its own members; a Widget subclass that is already a ButtonListener puts the next one at +0x5c.

### 4. ButtonWidget

sizeof **0xe0** (1.2: 0x118). Base Widget. Vtable `0x501944`, 49 slots. ctor `0x43f080(int theId,
ButtonListener* theListener)`, dtor body `0x43b170`, deleting `0x43d350`.

#### 4.1 Vtable (slots not listed = Widget's)

slot | method | notes
---|---|---
0 | ~ButtonWidget | deletes mFont
10 | SetDisabled(bool) | MarkDirty if mDisabledImage
16 | Draw(Graphics*) | see 4.3
17 | Update() | see 4.3
23 | MouseEnter() | Widget::MouseEnter; over-alpha reset; MarkDirtyFull if down / over image / hilite colour differs; listener slot 0 **if !mDisabled**
24 | MouseLeave() | as 1.2; listener slot 1 if !mDisabled
27 | MouseDown(int x, int y, int cc) | the port overrides the 4-arg one; Widget::MouseDown(x,y,cc); **if !mDisabled** ButtonPress(mId, cc) (listener slot 2); MarkDirtyFull
30 | MouseUp(int x, int y) | the port overrides the 4-arg one; Widget::MouseUp(x,y); **if mIsOver && !mDisabled** ButtonDepress(mId) (slot 4); then MarkDirtyFull (port: MarkDirty first, checks mWidgetManager->mHasFocus)
47 | SetFont(Font*) | delete old, Duplicate()
48 | bool IsButtonDown() | mIsDown && mIsOver && !mDisabled

No `DrawButtonImage` virtual (1.2 slot 54): it is non-virtual at 0x4382c0; `HaveButtonImage` 0x438290. No MouseMove
override, so no ButtonMouseMove.

#### 4.2 Fields

Offset | type | name | ctor
---|---|---|---
+58 | int | mId | theId
+5c | std::string | mLabel | ""
+78 | int | mLabelJustify | 0 (center; -1 left, 1 right)
+7c | Font* | mFont | NULL
+80, +84, +88, +8c | Image* | mButtonImage, mOverImage, mDownImage, mDisabledImage | NULL
+90, +a0, +b0 | Rect | mNormalRect, mOverRect, mDownRect | 0
+c0, +c1, +c2 | bool | mInverted, mBtnNoDraw, mFrameNoDraw | false
+c4 | ButtonListener* | mButtonListener | theListener
+c8, +d0, +d8 | double | mOverAlpha, mOverAlphaSpeed (game sets 0.08), mOverAlphaFadeInSpeed | 0

No `mDisabledRect` (1.2 +0xe8). Colours: `SetColors(gButtonWidgetColors, 2)` = {0,0,0},{0,0,0} (0x535068):
only COLOR_LABEL and COLOR_LABEL_HILITE exist; port sets 6. The ctor does not set mHasAlpha (port: true).

#### 4.3 Behaviour

- Draw (0x43d380): returns if mBtnNoDraw; creates `SysFont(app, "SansSerif", 10)` if mFont is NULL and the label is
  not empty (port: none, the game always sets fonts); isDown = IsButtonDown() ^ mInverted. Without mButtonImage and
  mDownImage it draws the 1.2 frame with **hard-coded** colours: fill (212,212,212), outlines black / white /
  (132,132,132) (port: mColors[2..5], same default values); the pressed label is always **black** at +1,+1 (port:
  hilite or label colour). With images it is 1.2's except no disabled rect (disabled: `DrawImage(mDisabledImage,
  0, 0)`) and the normal image is drawn under the over image even when mOverAlpha reaches 1.
- Update (0x4383a0): as 1.2/1.3 apart from the !mDisabled test before ButtonDownTick.
- MouseEnter and MouseLeave call the listener's ButtonMouseEnter / ButtonMouseLeave once. Ours already does, so a game
  override that also calls the listener doubles it (AstroPop's AstroButton did, which broke its menu's secret hover
  order; its GAME_NOTES section 10).

### 5. DialogButton

sizeof **0xf8** (1.2: 0x130). Base ButtonWidget. Vtable `0x501fa4`, 49 slots: slot 0 = shared deleting
dtor 0x43f2d0, slot 16 Draw 0x43dc90; everything else ButtonWidget's. ctor `0x43f240(Image* theComponentImage, int
theId, ButtonListener* theListener)`. Fields: +e0 Image* mComponentImage, +e4 mTranslateX (1), +e8 mTranslateY (1),
+ec mTextOffsetX (0), +f0 mTextOffsetY (0). The ctor also sets mDoFinger = true and `SetColors(gDialogButtonColors,
2)` = white, white (0x52fa18; port: 6 colours, label white/white too).

Draw (0x43dc90): without mComponentImage -> ButtonWidget::Draw. Otherwise it draws the image box (`mDownRect` /
`mOverRect` / `mNormalRect`, or the whole button when mNormalRect.mWidth == 0, translated by mTranslateX/Y when down),
then the label **colourised** (SetColorizeImages(true)) in mColors[1] if mIsOver else mColors[0], centred at
`(w - width)/2 + mTextOffsetX`, `(h - ascent/6 - ascentPadding - 1 + ascent)/2 + mTextOffsetY`, and when down
**Translate(-1, -1)** (hard-coded; harmless, the game never changes mTranslateX/Y). Differences from the port: no
mBtnNoDraw test on the image path, no disabled rect, no over-alpha fade; a SysFont "SansSerif" 12 fallback. Nothing to
do.

### 6. Dialog

sizeof **0x110** (1.2: 0x148). Bases Widget, ButtonListener at **+0x58**. Vtables `0x501a2c` (52 slots)
and `0x501a0c` (+0x58, 6 slots). ctor `0x43f3b0(Image* theComponentImage, Image* theButtonComponentImage,
int theId, bool isModal, const string& header, const string& lines, const string& footer, int theButtonMode)`
(the signature of 1.2 and the port); the dtor deletes both buttons and both fonts.

#### 6.1 Vtable (slots not listed = Widget's)

slot | method | notes
---|---|---
0 | ~Dialog |
1 | AddedToManager(WM*) | Widget's, then wm->AddWidget(yes), AddWidget(no)
2 | RemovedFromManager(WM*) | Widget's, then RemoveWidget(yes/no)
3 | OrderInManagerChanged() | PutInfront(yes, this), PutInfront(no, this)
7 | SetColor(int, const Color&) | idx 3 -> buttons' colour 0, idx 4 -> buttons' colour 1
13 | Resize(x,y,w,h) | see 6.3
16 | Draw(Graphics*) | see 6.3
17 | Update() | `jmp Widget::Update`
27 | MouseDown(x,y,cc) | only if cc == 1: SetCursor(2), mDragging, mDragMouseX/Y. Does **not** call Widget::MouseDown
30 | MouseUp(x,y) | if mDragging: SetCursor(0), mDragging = false
31 | MouseDrag(x,y) | identical to 1.2 (keeps the dialog within -8..+8 of the WM size)
47, 48, 49 | SetButtonFont, SetHeaderFont, SetLinesFont (Font*) | button font: yes/no->SetFont; the others Duplicate
50 | int GetPreferredHeight(int w) | 1.2 formula; button height = yes->mComponentImage->GetHeight(); no EnsureFonts
51 | bool IsModal() |

No WaitForResult (the port has it: do not use).

ButtonListener at +0x58: slots 0/1 are stubs, 2 is ButtonListener::ButtonPress(id,cc), and
**3 ButtonPress(id) 0x4386e0, 4 ButtonDepress(id) 0x438720, 5 ButtonDownTick(id) 0x438760** each do
`if (id == mId + 2000 || id == mId + 3000) mWidgetManager->mApp->ButtonPress / ButtonDepress /
ButtonDownTick(id)` (app slots 3 / 4 / 5). Game dialogs override slots 2 to 4 at +0x58, call these and compare
`theId == mId + 2000` themselves; the app's ButtonDepress decodes 2000..2999 as yes/ok of dialog id-2000 and
3000.. as no/cancel of dialog id-3000.

#### 6.2 Fields

Offset | type | name | ctor
---|---|---|---
+58 | vfptr | ButtonListener |
+5c | Image* | mComponentImage | arg
+60, +64 | DialogButton* | mYesButton, mNoButton |
+68, +84, +a0 | std::string | mDialogHeader, mDialogFooter, mDialogLines |
+bc | int | mButtonMode | 0 none, 1 YES_NO, 2 OK_CANCEL, 3 FOOTER
+c0, +c4 | Font* | mHeaderFont, mLinesFont | NULL
+c8 | int | mTextAlign | 0
+cc | int | mLineSpacingOffset | 0
+d0 | Insets | mBackgroundInsets | 0
+e0 | Insets | mContentInsets | (24,24,24,24)
+f0 | int | mSpaceAfterHeader | 10
+f4 | bool | mDragging | false
+f8, +fc | int | mDragMouseX, mDragMouseY |
+100 | int | mId | theId
+104 | bool | mIsModal | isModal
+108 | int | mButtonHorzSpacing | 8
+10c | int | mButtonSidePadding | 0

Not in 2004: mDialogListener, mNumButtons, mButtonHeight, mResult, mPriority (= 1 in the port).
ctor also sets mHasTransparencies = mHasAlpha = true and `SetColors(gDialogColors, 5)` (0x52f9e8):
header white, lines yellow, footer white, button text white, button hilite white (COLOR_HEADER..
COLOR_BUTTON_TEXT_HILITE; no COLOR_BKG/COLOR_OUTLINE). Buttons: modes 1/2 create two DialogButtons
(`new DialogButton(theButtonComponentImage, mId+2000 / mId+3000, this+0x58)`) labelled from the global
strings `DIALOG_YES_STRING` "YES" / `DIALOG_NO_STRING` "NO" (mode 1) or "OK" / "CANCEL" (mode 2)
(0x52fa30/4c/68/84, plain constants, **no GetString lookup**); mode 3 creates one button `mId+2000`
labelled with the footer.

#### 6.3 Layout and drawing

Resize (0x43a620) and GetPreferredHeight (0x438ec0) are 1.2's formulas, except that the button height is
`mYesButton->mComponentImage->GetHeight()` (a crash without a button image; the port's mButtonHeight is the image
height or 24, the same value when an image exists). Draw (0x43d950) is 1.2's with the same numbers, but it **always**
draws `DrawImageBox(Rect(bL, bT, w-bL-bR, h-bT-bB), mComponentImage)` (no grey fallback box) and creates SysFont
"SansSerif" 14 (header) / 12 (lines) when the fonts are NULL (port: no fallback, set the fonts). Header colour is
mColors[0], lines mColors[1], footer (not in mode 3, drawn with mHeaderFont) mColors[2].

UpdateCheckDialog (sizeof 0x120, a Dialog that polls an HTTP transfer for the update check) is not ported.

### 7. Checkbox

sizeof **0x8c** (1.2: 0xe4). Base Widget. Vtable `0x501edc`, 49 slots; slot 0 = shared 0x43e8f0.
ctor `0x43e890(Image* theUncheckedImage, Image* theCheckedImage, int theId, CheckboxListener*)` (same as
1.2/port); sets mDoFinger = true.

slot | method | notes
---|---|---
16 | Draw | if mCheckedRect.mWidth: DrawImage(mUncheckedImage, 0,0, mChecked ? mCheckedRect : mUncheckedRect); else DrawImage(mChecked ? mCheckedImage : mUncheckedImage). No NULL checks, no outline fallback
27 | MouseDown(x,y,cc) | port overrides the 4-arg one; Widget::MouseDown; toggle; listener->CheckboxChecked(mId, mChecked) if listener; MarkDirty
47 | SetChecked(bool, bool tellListener) | MarkDirty
48 | bool IsChecked() |

Fields: +58 CheckboxListener* mListener, +5c int mId, +60 bool mChecked, +64 Image* mUncheckedImage, +68 Image*
mCheckedImage, +6c Rect mCheckedRect, +7c Rect mUncheckedRect. No mOutlineColor/mBkgColor/mCheckColor.

### 8. Slider

sizeof **0x80** (`operator new(0x80)` in OptionsDialog; 1.2: 0xa8). Base Widget. Vtable `0x501c54`,
49 slots; slot 0 = shared 0x43e8f0. ctor `0x43e550(Image* theTrackImage, Image* theThumbImage, int theId,
SliderListener*)`. Horizontal only.

slot | method | notes
---|---|---
16 | Draw | 1.2 three-part track (track width/3 cells) + thumb at `(int)(mVal*(w - thumbW))`
24 | MouseLeave | SetCursor(pointer) unless dragging
25 | MouseMove(x,y) | dragging cursor over the thumb
27 | MouseDown(x,y,cc) | starts a drag **only on the thumb**; a click on the track does nothing (1.2/port jump the thumb there)
30 | MouseUp(x,y) | mDragging = false, SetCursor(0), SliderVal(mId, mVal) (on any mouse-up)
31 | MouseDrag(x,y) | mVal = (x - mRelX)/(w - thumbW) clamped 0..1; SliderVal + MarkDirtyFull if changed
47 | SetValue(double) | **no clamping**; MarkDirtyFull
48 | bool HasTransparencies() | true (folded stub)

Fields: +58 SliderListener* mListener, +60 double mVal, +68 int mId, +6c Image* mTrackImage, +70 Image*
mThumbImage, +74 bool mDragging, +78 int mRelX. No mRelY/mHorizontal.

### 9. EditWidget

sizeof **0xec** (1.2: 0x11c). Base Widget. Vtable `0x50186c`, 53 slots. ctor `0x43ef90(int theId,
EditListener*)`, dtor body `0x43aef0` (deletes mFont and mWidthCheckFont), deleting `0x43beb0`.

slot | method | notes
---|---|---
13 | Resize | + FocusCursor(false)
15 | WantsFocus | true
16 | Draw | as 1.2: bkg mColors[0], two clipped passes (text mColors[2]; selection fill mColors[3] + text mColors[4] when focused), outline mColors[1] DrawRect; SysFont "SansSerif" 10 fallback
17 | Update | blink when focused (mBlinkDelay 40)
20 | KeyChar(char) | AllowChar -> ProcessKey(0, c); no Widget::KeyChar call
21 | KeyDown(KeyCode) | keys outside 'A'..'Y' -> AllowKey -> ProcessKey(k, 0); Widget::KeyDown
23 / 24 | MouseEnter / MouseLeave | text / pointer cursor
27 | MouseDown(x,y,cc) | port overrides the 4-arg one; cursor from GetCharAt; cc > 1 selects the word
29 | MouseUp(x,y,cc) | drop an empty selection
31 | MouseDrag | extend selection
46 | MarkDirty | MarkDirtyFull if mColors[0].mAlpha != 255
47 | ProcessKey(KeyCode, char) | 1.2 logic: shift-select, ctrl-C/X (app clipboard), ctrl-V (keeps chars passing AllowChar, stops at CR/LF), ctrl-Z undo, arrows (ctrl = word jump), Home/End, Backspace/Delete, Enter -> EditWidgetText, printable 0x20..0x7e; mMaxChars and mMaxPixels truncation; AllowText veto restores the old text
48 | SetFont(Font*, Font* widthCheck) | port: SetFont(Font*, Font* = NULL); duplicates both; one width-check font instead of the 1.2 list
49 | SetText(const string&) | port: SetText(text, leftPosToZero = true); cursor to end, mHilitePos 0, MarkDirty; **does not touch mLeftPos** (port resets it to 0 by default)
50 | bool IsPartOfWord(char) | [A-Za-z0-9_]
51 | int GetCharAt(int x, int y) |
52 | FocusCursor(bool bigJump) | no clamp of mLeftPos to len-1

Non-virtual: GetDisplayString 0x43afd0 (password masking).

Offset | type | name | ctor
---|---|---|---
+58 | int | mId | arg
+5c, +78 | std::string | mString, mPasswordDisplayString |
+94 | Font* | mFont | NULL
+98 | Font* | mWidthCheckFont (inferred name; port: mWidthCheckList) | NULL
+9c | EditListener* | mEditListener | arg
+a0 | bool | mShowingCursor | false
+a4, +a8 | int | mCursorPos (0), mHilitePos (-1) |
+ac, +b0 | int | mBlinkAcc (0), mBlinkDelay (40) |
+b4 | int | mLeftPos | 0
+b8, +bc | int | mMaxChars, mMaxPixels | -1
+c0 | char | mPasswordChar | 0
+c4 | std::string | mUndoString |
+e0, +e4, +e8 | int | mUndoCursor, mUndoHilitePos, mLastModifyIdx | 0

No mDrawSelOverride. Colours `SetColors(gEditWidgetColors, 5)` (0x52f9ac) = white bkg, black outline, black text,
black hilite, white hilite text: same as the port's table.

### 10. ListWidget

sizeof **0xc0** (`operator new(0xc0)`). Bases Widget, ScrollListener at **+0x58**. Vtables `0x501d24`
(60 slots) and `0x501d1c` (+0x58, 1 slot: ScrollPosition 0x437d80). ctor `0x43ee70(int theId, Font*
theFont, ListListener*)`.

slot | method | notes
---|---|---
3 | OrderInManagerChanged | PutInfront(mChild) and PutInfront(mScrollbar) through gSexyAppBase->mWidgetManager
13 | Resize | mPageSize = (h-8)/itemH if h > itemH+8 else 1; mScrollbar slot 49 (SetPageSize)
16 | Draw | bkg mColors[0]; rows from (int)mPosition; selected row filled mColors[4]; text mColors[3] if hilited, mColors[5] if selected and 6+ colours exist, else mLineColors[i]; x: 0 -> 4, 1 -> centred, else right-4; outline mColors[1] if mDrawOutline
24 | MouseLeave | hilite -1 on the whole parent/child chain, pointer cursor
25 | MouseMove(x,y) | row under the cursor becomes the hilite of the chain; hand cursor on a row (no mDoFingerWhenHilited, no ListHiliteChanged)
27 | MouseDown(x,y,cc) | port overrides the 4-arg one; ListClicked(mId, mHiliteIdx, cc) if hilited and listener; no Widget::MouseDown
47 | string GetSortKey(int idx) | zero-padded to mMaxNumericPlaces, child key when mSortFromChild
48 | Sort(bool ascending) |
49 | string GetStringAt(int idx) |
50 | int AddLine(const string&, bool alphabetical) | adds "" to the child lists
51 | SetLine(int, const string&) |
52 | int GetLineCount() |
53 | int GetLineIdx(const string&) |
54 | SetColor(const string& line, const Color&) | sets mLineColors on the chain
55 | RemoveLine(int) | mScrollbar slot 48 (SetMaxValue) with the new count
56 | RemoveAll() |
57 | int GetOptimalWidth() | widest line + 16
58 | int GetOptimalHeight() | count * itemH + 8
59 | SetSelect(int) |
(+0x58) 0 | ScrollPosition(int id, double pos) | forwards to mChild, mPosition = pos

mPageSize is computed on the x87 and stored through a float, so it is rounded to 24 bits; with our double a scrolled list
box shows its lines one pixel higher in some positions. AstroPop rounds it in its ListDialog::Resize
(`(double)(float)mPageSize`, then the scroll bar's page size); a shared fix in ListWidget would cover every 2004 game
with a scrolled list (not made yet: check BookWorm's and Typer Shark's list boxes first).

Not in 2004: SetHilite, SetLineColor, MouseWheel, RemovedFromManager (port's calls ListClosed).

Offset | type | name | ctor
---|---|---|---
+5c | int | mId | arg
+60 | Font* | mFont | arg
+64 | ScrollbarWidget* | mScrollbar | NULL (no ScrollbarWidget class is linked; BookWorm leaves it NULL as far as seen)
+68 | int | mJustify | 0
+6c, +7c | vector | mLines (string), mLineColors (Color) |
+90, +98 | double | mPosition, mPageSize | 0
+a0, +a4 | int | mHiliteIdx, mSelectIdx | -1
+a8 | ListListener* | mListListener | arg
+ac, +b0 | ListWidget* | mParent, mChild | NULL
+b4 | bool | mSortFromChild (inferred) | false
+b5 | bool | mDrawOutline | true
+b8 | int | mMaxNumericPlaces | 0
+bc | int | mItemHeight | **-1** (font height used at run time; port: font height at ctor, same result)

`SetColors(gInitialListWidgetColors, 5)` (0x52f970): white, white, black, (0,192,0), (0,0,128); 1.2/port add a 6th
(select text, white). No mDrawSelectWhenHilited / mDoFingerWhenHilited.

### 11. HyperlinkWidget

sizeof **0x108** (1.2: 0x140). Base ButtonWidget. Vtable `0x50206c`, 49 slots: 0 shared 0x43f2d0,
16 Draw 0x43bd10, 23 MouseEnter 0x43a940, 24 MouseLeave 0x43a960 (ButtonWidget's + MarkDirtyFull). ctor
`0x43f300(int theId, ButtonListener*)`: mColor (+0xe0) = mOverColor (+0xf0) = (255,255,255), mUnderlineSize
(+0x100) = 1, mUnderlineOffset (+0x104) = 3, mDoFinger = true. Same fields and Draw as 1.2/port (label
centred at `(h + ascent)/2 - 1`, colour by mIsOver, mUnderlineSize 1-pixel lines at +mUnderlineOffset).
2004 Draw measures with mFont **before** its SysFont fallback, so it needs a font set (the game sets one).

### 12. CursorWidget

sizeof **0x5c**. Base Widget. Vtable `0x501e1c`, ctor `0x43e840` (mImage +0x58, mMouseVisible = false,
SetTransient(true)), Draw 0x438120, GetHotspot 0x438e70 (img w/2, h/2). Owned by the WidgetManager (+0x28) and only shown when WM+0x2c is set, which
BookWorm never does. Drop it.

### 13. Insets, Rect, Point, Color helpers

Same fields as the port (Insets `mLeft, mTop, mRight, mBottom`, Rect `mX, mY, mWidth, mHeight`, Point `mX, mY`). Their
small functions are folded together (item 5 of subsection 0): read a call to 0x437e90 by where its result goes, an
Insets into +0xd0/+0xe0 of a Dialog, a Color into a Graphics. `Rect::Intersection` is 0x4387a0.

### 14. KeyCodes

KeyCode is the Win32 virtual-key code, as 1.2 and the port (`KeyCodes.h`); the manager tracks `mKeyDown[0..254]`.

## 4. The application class (SexyApp)

The merged SexyAppBase + SexyApp of 2004. Symbols: `games/bookworm/docs/symbols/10_framework_app.jsonl` (which
also covers sections 6 and 7).

### Class sizes (2004 build vs 1.2)

| Class | sizeof 2004 | 1.2 | Vtable slots 2004 (1.2) | Notes |
|---|---|---|---|---|
| SexyApp (= SexyAppBase + SexyApp) | **0x618** | 0x5e0 + 0x6d0 | 67 (96) | single base ButtonListener (6 slots); no DialogListener; BookWormApp data from 0x618 |
| DSoundManager / DSoundInstance | 0x14a0 / 0x38 | 0x30a0 / 0x38 | 14 (16) / 12 | no file-name table, no primary buffer / identical code |
| MusicInterface / BassMusicInterface / FModMusicInterface | 4 / 0x14 / 0x20 | same | 15 / 15 / 17 (20 / 20 / 22) | no Unload*/PauseAll/ResumeAll/SetSongMaxVolume; BassMusicInfo 0x20 (1.2 0x28); FMod is dead code |
| BASS_INSTANCE / FMOD_INSTANCE | 0x64 / 0x80 | 0x68 / 0x80 | | BASS 2.0.0.20 shipped |
| MTRand, Buffer | 0x9c4, 0x20 | same | none, 1 | MTRand: 1.2 seeding (69069) |
| PropertiesParser / XMLParser, HTTPTransfer, ResourceManager | 0x2c / 0x74, 0xfc, 0xb4 | | 1 / 1, 1, 3 | only ParsePropertiesBuffer linked; HTTPTransfer used by the Hall of Fame; ResourceManager created, never used |
| InternetManager / BetaSupport, DDInterface / D3DInterface / D3DTester | 0x538 / 0x18c, 0xcf0 / 0x4c / 0x84 | | 1 each | drop; port: SDL DDInterface |

`class SexyApp : public ButtonListener` only: one vtable at +0 and no DialogListener base (1.2:
`SexyAppBase : ButtonListener, DialogListener`, second vtable at +4), and no separate SexyAppBase class. sizeof is
0x618, so a game's app class starts its own data there. `gSexyAppBase` (stored early in the ctor) and `gSexyApp`
(stored at its end) both hold the app. The build string is "00000190Thu Apr 15 14:52:08 2004": 8 digits of mBuildNum,
then mBuildDate.

#### 1.1 Vtable (67 slots; a game's app class adds its own from slot 67)

The port has the method of the same name unless the notes say otherwise. Slots 0 to 5 are the ButtonListener base
(section 3).

| Slot | Method (2004 signature) | Notes |
|---|---|---|
| 0, 1 | `ButtonMouseEnter(int)`, `ButtonMouseLeave(int)` | |
| 2 | `ButtonPress(int theId, int theClickCount)` | calls slot 3 |
| 3 | `ButtonPress(int theId)` | empty in the base |
| 4 | `ButtonDepress(int theId)` | a game's override handles every menu and dialog button; a dialog's YES arrives as 2000+dialogId, NO as 3000+dialogId |
| 5 | `ButtonDownTick(int theId)` | dialogs forward their buttons' down-ticks here too |
| 6 | `void Process()` | port: Process(bool); no allowSleep, returns nothing; see 1.4 |
| 7 | `void UpdateFrames()` | `if(!mMinimized){++mUpdateCount; if(mWidgetManager->UpdateFrame()) ++mFPSDirtyCount;} mMusicInterface->Update(); mInternetManager->Update()` |
| 8 | `void DoUpdateFrames()` | returns at once while gScreenSaverActive; at the first update after the loading thread ends: thread priority normal, mLoaded=true, mYieldMainThread=false, LoadingThreadCompleted(); then UpdateFrames() |
| 9 | `void MakeWindow()` | the window (windowed at 64,64 with an mWidth x mHeight client; fullscreen WS_POPUP, topmost), the DDInterface on the first call (and registry "Is3D"), DDInterface::Init with colour-depth and 3D fallbacks, `SetTimer(mHWnd,100,mFrameTime)` |
| 10 | `void EnforceCursor()` | see 1.5 |
| 11 | `void ReInitImages()` (name inferred) | MemoryImage slot 18 (frees the cached native/RLE alpha data) on every image of mMemoryImageSet, after DirectDraw is re-initialised |
| 12 | `void DeleteExtraImageData()` | image slot 15 DeleteExtraBuffers on every image |
| 13 | `void LoadingThreadCompleted()` | empty |
| 14 | `~SexyApp()` | scalar deleting |
| 15 | `MusicInterface* CreateMusicInterface(HWND)` | port: CreateMusicInterface(); section 6; Init passes mInvisHWnd |
| 16 | `void Init()` | see 1.3; a game's override calls it first |
| 17 | `void LoadingThreadProc()` | empty; runs on a real thread (1.6) |
| 18, 19 | `void WriteToRegistry()`, `void ReadFromRegistry()` | see 1.7 |
| 20 | `Dialog* NewDialog(id, isModal, header, lines, footer, buttonMode)` | **the base returns NULL** (1.2 builds a Dialog): the game's override builds every dialog, so the port's default is never reached |
| 21 | `void Popup(const std::string&)` | `if(!mShutdown) MessageBox(mHWnd, s, "FATAL ERROR", MB_ICONHAND)` (literal title, no property) |
| 22 | `void SafeDeleteWidget(Widget*)` | push_back on mSafeDeleteList (a plain `list<Widget*>`, no update depth) |
| 23 | `bool Validate(const string& userName, const string& regCode)` | the registration check, called by ReadFromRegistry; port: SexyApp::Validate (non-virtual), not ported |
| 24 | `void URLOpenFailed(const string&)` | mIsOpeningURL=false |
| 25 | `void URLOpenSucceeded(const string&)` | mIsOpeningURL=false; if mShutdownOnURLOpen {mSkipAd=true; Shutdown()} |
| 26 | `bool OpenURL(const string& url, bool shutdownOnOpen)` | ignores a repeat of the URL being opened; sets mIsOpeningURL, mShutdownOnURLOpen, mOpeningURL, mOpeningURLTime; ShellExecute("open"); on failure URLOpenFailed |
| 27, 28, 29 | `OpenHTMLTemplate(file, DefinesMap)`, `OpenRegisterPage()`, `OpenRegisterPage(DefinesMap)` | stubs in the port's SexyApp; not ported |
| 30 | `std::string GetProductVersion(const string& path)` | the exe's ProductVersion resource |
| 31 | `void SEHOccured()` | SetMusicVolume(0); hide window; mSEHOccured=true; EnforceCursor() |
| 32 | `std::string GetGameSEHInfo()` | crash report text; not ported |
| 33 | `void Shutdown()` | see 1.3 |
| 34 | `void ParseCmdLine(const string&)` | splits on spaces outside quotes, "name=value"; skipped when the external window (0x318) is set |
| 35 | `void HandleCmdLineParam(const string& name, const string& value)` | -play, -recnum=N, -playnum=N, -record, -demofile=F, -version, -crash; **anything else pops up an error and exits**, so the port's test switches (-seed, -script, -fast) never pass through a translation of it |
| 36 | `void Start()` | see 1.3 |
| 37, 38 | `PreDisplayHook()`, `PreDDInterfaceInitHook()` (names inferred) | empty; called in Init before mWidgetManager->Resize, and before every DDInterface::Init |
| 39 | `bool ChangeDirHook(const char*)` | returns false, so Init chdir()s to the exe folder |
| 40 | `void PlaySample(int id, int pan)` | `inst=mSoundManager->GetSoundInstance(id); inst->SetPan(pan); inst->Play(false,true)` |
| 41 | `void PlaySample(int id)` | `GetSoundInstance(id)->Play(false,true)` (autorelease) |
| 42 | `double GetMasterVolume()` | mSoundManager slot 9 |
| 43, 44 | `GetMusicVolume()`, `GetSfxVolume()` | mMusicVolume, mSfxVolume |
| 45 | `bool IsMuted()` | mMuteCount > 0 |
| 46 | `void SetMasterVolume(double)` | **stores into mSfxVolume** (as 1.2), then mSoundManager->SetMasterVolume (slot 10) |
| 47 | `void SetMusicVolume(double)` | mMusicVolume=v; mMusicInterface->SetVolume(muted ? 0 : v) (slot 12) |
| 48 | `void SetSfxVolume(double)` | mSfxVolume=v; mSoundManager->SetVolume(muted ? 0 : v) (slot 5) |
| 49, 50 | `Mute(bool autoMute)`, `Unmute(bool autoMute)` | ++mMuteCount (and ++mAutoMuteCount if autoMute); Unmute decrements only if mMuteCount>0; both re-apply both volumes |
| 51 | `double GetLoadingThreadProgress()` | 1.0 if mLoaded; 0 if not started or mNumLoadingThreadTasks==0; else min(completed/num, 1.0) |
| 52 | `void SwitchScreenMode(bool wantWindowed, bool is3d)` | port: SwitchScreenMode(w,3d,force); mForceFullscreen forces fullscreen; on a change: Set3DAccelerated(is3d,false), MakeWindow, show, mSoundManager->SetCooperativeWindow, ReInitImages, mLastTime=now; else Set3DAccelerated(is3d,true) |
| 53 | `void SwitchScreenMode(bool wantWindowed)` | slot 52 with Is3DAccelerated() |
| 54 | `void SetAlphaDisabled(bool)` | |
| 55 | `Dialog* DoDialog(id, isModal, header, lines, footer, buttonMode)` | `KillDialog(id); d=NewDialog(...); AddDialog(id,d); return d` |
| 56 | `Dialog* GetDialog(int id)` | mDialogMap lookup |
| 57 | `void AddDialog(int id, Dialog*)` | KillDialog(id); map insert; list push_back; mWidgetManager->AddWidget(d); if d->IsModal() (Dialog slot 51): SetBaseModal(d) and ModalOpen() |
| 58 | `bool KillDialog(int id)` | if modal ModalClose(); mWidgetManager->RemoveWidget(d); list remove; **SafeDeleteWidget(d)** (deleted later, not now); map erase; if no base-modal widget is left (WM+0x4c==0), the most recently added remaining modal dialog becomes base modal |
| 59 | `int GetDialogCount()` | mDialogMap.size() |
| 60-63 | `ModalOpen()`, `ModalClose()`, `GotFocus()`, `LostFocus()` | empty |
| 64 | `bool DebugKeyDown(int key)` | only after Ctrl+Alt+D toggled mDebugKeysEnabled: F2 perf, F3 FPS, F4 FPS mode, F8 3D toggle, F9 memory, F11 dump; not ported |
| 65 | `bool DrawDirtyStuff()` | see 1.4 |
| 66 | `void Redraw(Rect* clip)` | DDInterface::Redraw; on failure re-inits DirectDraw (lost surfaces) |

Virtuals of 1.2 that do not exist here: DoUpdateFramesF (no UpdateF), DeleteNativeImageData, InitHook, ShutdownHook,
PreTerminate, BeginPopup, EndPopup, MsgBox, LogScreenSaverError, GetSEHWebParams, DoParseCmdLine (inlined in Init),
GetImage and GetSharedImage as virtuals, SwitchScreenMode(), AddDialog(Dialog*), KillDialog(Dialog*), IsAltKeyUsed,
DebugKeyDownAsync, CloseRequestAsync, Done3dTesting, NotifyCrashHook, CheckSignature, InitPropertiesHook, DoMainLoop,
UpdateAppStep, UpdateApp, ShouldCheckForUpdate, UpdateCheckQueried and OpenUpdateURL.

#### 1.2 Fields (absolute offsets; verified from a ctor store or an accessor body unless marked inferred)

Names are 1.2's where the same, and the port's field has the same name (the SexyApp-layer ones sit in the port's
`SexyApp`) unless the notes say "not in port". Offsets not listed are padding.

| Off | Type | Name | Meaning / ctor value |
|---|---|---|---|
| 0x000 | vfptr | | 0x506714 (BW 0x4fbda4) |
| 0x004 | ulong | mRandSeed | GetTickCount() in Init, SRand(seed) |
| 0x008 | string | mProdName | "Product"; BW ctor "BookWorm"; partner.xml "ProdName" |
| 0x024 | string | mTitle | "SexyApp"; BW "BookWorm Deluxe " + mProductVersion; partner "Title" |
| 0x040 | string | mRegKey | BW "PopCap\\BookWorm"; property "RegistryKey" overrides (ReadFromRegistry) |
| 0x05c | int | mRelaxUpdateBacklogCount | 0 |
| 0x060, 0x064 | int | mWidth, mHeight | 640, 480 |
| 0x068 | int | mFullscreenBits | 16 |
| 0x070, 0x078 | double | mMusicVolume, mSfxVolume | 0.85 (BW ctor sets music 0.5) |
| 0x080, 0x088 | double | mDemoMusicVolume, mDemoSfxVolume | 0 |
| 0x090, 0x091 | bool | mNoSoundNeeded, mWantFMod | false (mNoSoundNeeded true: no DirectSound window, dummy music) |
| 0x094 | int | mBuildNum | atol of the first 8 chars of the build string (190) |
| 0x098 | string | mBuildDate | "Thu Apr 15 14:52:08 2004" |
| 0x0b4-0x0e0 | | mMutex (HANDLE; single-instance mutex mProdName+"Mutex"), mOnlyAllowOneCopyToRun (bool 0x0b8, true), mNotifyGameMessage (UINT 0x0bc, RegisterWindowMessage("Notify"+mProdName)), mCritSect (24 bytes 0x0c0, 0x466880), mInternetManager (0x0d8, `new InternetManager` 0x538 bytes in the ctor), mBetaSupport (0x0dc, `new BetaSupport(this)` 0x18c in Init), mBetaValidate (bool 0x0e0: Init exits unless BetaSupport validates, 0x477930) | mutex, critsect, mNotifyGameMessage, mBetaValidate not in port; the two manager pointers are `void*` there |
| 0x0e1 | uchar[512] | mAdd8BitMaxTable | 0..255 then 255 |
| 0x2e1 | bool | mSkipAd | set by URLOpenSucceeded before Shutdown; skips the exit ad in Start |
| 0x2e2 | bool | mDontUpdate | registry/partner "DontUpdate" |
| 0x2e4 | WidgetManager* | mWidgetManager | **created in Init** (`new WidgetManager(this)`, 0x174 bytes), 0 before |
| 0x2e8, 0x2f4 | | mDialogMap map<int,Dialog*> (size at 0x2f0), mDialogList list<Dialog*> | |
| 0x300 | DWORD | mPrimaryThreadId | GetCurrentThreadId() **in the ctor**; not in port |
| 0x304-0x309 | bool | mSEHOccured, mShutdown, mIsWindowed (true; registry ScreenMode = !mIsWindowed; partner "DefaultWindowed"), mForceFullscreen, mForceWindowed, mInitialized (end of Init) | |
| 0x30c | DWORD | mTimeLoaded | GetTickCount() |
| 0x310 | HWND | mHWnd | main window (port: SDL_Window*) |
| 0x314 | HWND | mInvisHWnd | "InvisWindow" (music/sound owner); not in port |
| 0x318 | HWND | (external window) (inferred) | 0; when set MakeWindow uses it instead of creating one, ParseCmdLine and the idle check are skipped; not in port |
| 0x31c | list<MSG> | mDeferredMessages | node = hwnd,msg,wParam,lParam,time,pt (0x24 bytes); not in port |
| 0x328 | bool | mNoDefer | false: input messages are queued and run from Start's loop |
| 0x32c, 0x330 | | HTTPTransfer* (`new HTTPTransfer` 0xfc in the ctor; only Abort()ed and polled in Shutdown), bool ITSuccess (registry "ITSuccess": that transfer finished) | not in port |
| 0x334 | DDInterface* | mDDInterface | created by MakeWindow (0xcf0 bytes) |
| 0x338 | bool | mAlphaDisabled | |
| 0x33c | MusicInterface* | mMusicInterface | CreateMusicInterface in Init |
| 0x340, 0x35c, 0x378, 0x37c | | mReferId (string; registry/property "ReferId"), mVariation (string; "Variation"), mDownloadId (ulong; "DownloadId"), mRegSource (string, "ingame") | |
| 0x398 | bool | mReadFromRegistry | set by ReadFromRegistry; Shutdown writes the registry if set |
| 0x39c | string | mRegisterLink | "http://www.popcap.com/register.php?theGame="+mProdName+"&referid="+mReferId; registry/property "RegisterLink" |
| 0x3b8 | string | mProductVersion | GetProductVersion(exe) in the ctor |
| 0x3d4 | Image*[6] | mCursorImages | **6 cursors** (1.2: 13; port: mCursorImages[NUM_CURSORS]) |
| 0x3ec-0x414 | | mIsOpeningURL (bool), mShutdownOnURLOpen (bool 0x3ed), mOpeningURL (string 0x3f0), mOpeningURLTime (DWORD 0x40c), mLastTimerTime (0x410, WM_TIMER tick), mLastBigDelayTime (0x414) | |
| 0x418 | ulong | mLastVerCheckQueryTime | registry; time(NULL) if absent |
| 0x41c, 0x438, 0x454 | string | mUserName (= mRegUserName when read; BW writes it, 0x4173d2), mRegUserName (registry "RegName"), mRegCode ("RegCode") | |
| 0x470, 0x471 | bool | mIsRegistered (partner NoReg -> true; `|= Validate(name, code)`), mBuildUnlocked (partner NoReg -> true) | |
| 0x472-0x487 | | (unused) (inferred) | never accessed (room of 1.2's mUnmuted*Volume doubles) |
| 0x488, 0x48c, 0x490 | | mMuteCount (int; registry "Muted"), mAutoMuteCount (int), mDemoMute (bool) | |
| 0x494 | set<MemoryImage*> | mMemoryImageSet | AddMemoryImage 0x488b50 / RemoveMemoryImage 0x4874a0 (under the DD critsect) |
| 0x4a0-0x4d8 | | mNonDrawCount, mFrameTime (int 0x4a4, 10 ms), mLastTime, mLastUserInputTick, mSleepCount, mDrawCount, mUpdateCount (0x4b8), mUpdateMultiplier (double 0x4c0, 1.0), mPaused (bool 0x4c8; demo 'P' toggles), mFastForwardToUpdateNum (0x4cc), mFastForwardToMarker (bool 0x4d0), mLastDrawTick (0x4d4), mNextDrawTick (0x4d8) | |
| 0x4dc | int | mCursorNum | |
| 0x4e0 | SoundManager* | mSoundManager | new DSoundManager(mNoSoundNeeded ? NULL : mHWnd, mWantFMod) (0x14a0) in Init |
| 0x4e4, 0x4e8 | HCURSOR | mHandCursor CreateCursor(11,4), mDraggingCursor CreateCursor(15,10) | not in port |
| 0x4ec | list<Widget*> | mSafeDeleteList | ProcessSafeDeleteList 0x480660 deletes all (Rand disallowed meanwhile); port adds an update depth |
| 0x4f8 | bool | mMouseIn | |
| 0x4f9, 0x4fa, 0x4fb | bool | (left, right, middle button down) | set/cleared by the mouse dispatcher; pick MouseDrag vs MouseMove; not in port |
| 0x4fc-0x4ff | bool | mRunning (Start), mActive (1; WM_ACTIVATEAPP), mMinimized (WM_SIZE), mHasFocus (1) | |
| 0x500-0x520 | | mDrawTime, mFPSStartTick, mFPSFlipCount, mFPSDirtyCount, mFPSTime, mFPSCount, mShowFPS (bool 0x518), mShowFPSMode (0x51c), mScreenBltTime (0x520) | |
| 0x524-0x528 | bool | mLoadingThreadStarted, mLoadingThreadCompleted (set by the thread stub), mLoaded, mYieldMainThread (true), mLoadingFailed (Process calls Shutdown) | |
| 0x529, 0x52a, 0x52b | bool | mCursorThreadRunning (not in port), mSysCursor (true), mCustomCursorsEnabled (registry "CustomCursors") | |
| 0x52c, 0x530, 0x534 | | mTimesPlayed (int; registry + %WINDIR%\popcinfo.dat cross-check; BW increments), mTimesExecuted (int; ++ in Init), mTimedOut (bool) | |
| 0x535 | bool | mLastShutdownWasGraceful | registry "InProgress"==0 |
| 0x538, 0x53c | int | mNumLoadingThreadTasks, mCompletedLoadingThreadTasks (BW LoadingThreadProc increments) | |
| 0x540-0x542 | bool | mRecordingDemoBuffer (-record), mPlayingDemoBuffer (-play), mManualShutdown (WM_CLOSE) | |
| 0x544-0x5b4 | | demo state: mDemoFileName 0x544 ("popcap.dmo", no mDemoPrefix), mDemoBuffer 0x560 (Buffer, 0x20), mDemoLength 0x580, mLastDemoMouseX/Y 0x584/0x588, mLastDemoUpdateCnt 0x58c, mDemoNeedsCommand 0x590 (true), mDemoIsShortCmd 0x591, mDemoCmdNum 0x594, mDemoCmdOrder 0x598 (-1), mDemoCmdBitPos 0x59c, mDemoLoadingComplete 0x5a0, mHandleToIntMap 0x5a4, mCurHandleNum 0x5b0, mDemoMarkerList 0x5b4 | not in port |
| 0x5c0 | bool | mDebugKeysEnabled | Ctrl+Alt+D toggles |
| 0x5c3-0x5c6 | bool | mWaitForVSync (registry "WaitForVSync"), mUserChanged3DSetting, mAutoEnable3D (false; **BookWormApp ctor sets true**, 0x41a4f1: MakeWindow/Do3DTest 0x458bf0 may switch 3D on when the D3D test recommends it), mTest3D | |
| 0x5c8, 0x5cc | DWORD | mMinVidMemory3D (6), mRecommendedVidMemory3D (14) | not in port |
| 0x5d0 | map<string,string> | mStringProperties | (node isnil +0x45); port: wstring values |
| 0x5dc, 0x5e8, 0x5f4, 0x600 | maps | mBoolProperties, mIntProperties, mDoubleProperties, mStringVectorProperties | keyed by string |
| 0x60c | ResourceManager* | mResourceManager | `new ResourceManager(this)` (0xb4) in the ctor |
| 0x610 | int | (mOldWndProc?) (inferred) | 0, never read |

Not present in 2004 (do not look for them): mCompanyName, mFullCompanyName, mChangeDirTo, mUpdateAppState,
mUpdateAppDepth, mPendingUpdatesAcc, mUpdateFTimeAcc, mLastTimeCheck, mHasPendingDraw, mIsDrawing, mLastDrawWasEmpty,
mSharedImageMap, mCleanupSharedImages, mOverrideCursor, mCustomCursorDirty, mMuteOnLostFocus (always on),
mAutoStartLoadingThread (always on), mVSyncUpdates and the vsync-broken test, mSyncRefreshRate, mIsPhysWindowed,
mFullScreenWindow, mIsScreenSaver, mAllowMonitorPowersave, mDemoPrefix, mStepMode, mCtrlDown/mAltDown (not
initialised, never read), mMouseSensitivity.

#### 1.3 Construction, Init, Start, Shutdown

There is no InitHook, ShutdownHook, PreTerminate or InitPropertiesHook: a game's Init, Shutdown, LoadingThreadProc,
NewDialog and ButtonDepress overrides carry that logic, and the port overrides the same methods.

- **ctor** (0x48f810): the field defaults above; LoadLibrary of version/ddraw/dsound; the product version; the default
  strings DIALOG_BUTTON_OK, DIALOG_BUTTON_CANCEL, UPDATE_CHECK_TITLE/BODY, UP_TO_DATE_TITLE/BODY and
  NEW_VERSION_TITLE/BODY; InternetManager, HTTPTransfer and ResourceManager. **No WidgetManager, MusicInterface,
  SoundManager or DDInterface yet**, so a game's ctor cannot rely on mWidgetManager.
- **Init** (0x48aaf0), in order: return if mShutdown; DirectX DLL check; single-instance mutex (a second copy
  broadcasts mNotifyGameMessage and exits); chdir to the exe folder unless ChangeDirHook; ParseCmdLine;
  `mRandSeed = GetTickCount(); SRand(mRandSeed)`; the demo file if -play;
  `LoadProperties("properties\\partner.xml", required=false, checkSig=true)`; NoReg sets mIsRegistered and
  mBuildUnlocked; ProdName; DefaultWindowed; Title (mTitle = Title + " " + mProductVersion); InternetManager;
  BetaSupport; `srand(GetTickCount())`; **ReadFromRegistry()**; ++mTimesExecuted; RegisterLink and DontUpdate; window
  classes; the invisible window; **the WidgetManager**; cursors; PreDisplayHook;
  `mWidgetManager->Resize(0,0,mWidth,mHeight)`; windowed only if the screen is larger than mWidth x mHeight;
  MakeWindow; DSoundManager; SetSfxVolume(mSfxVolume); CreateMusicInterface(mInvisHWnd); SetMusicVolume(mMusicVolume);
  mInitialized. The port's SexyAppBase::Init and InitPropertiesHook do the same with the properties before the
  registry.
- **Start** (0x480950): cursor thread; loading thread (always; the main thread goes to ABOVE_NORMAL); show and focus
  the window; timeBeginPeriod(1); then `while(!mShutdown){ PeekMessage/Dispatch all; ProcessDemo();
  ProcessDeferredMessages(); if(mShutdown) break; Process(); }`; afterwards ProcessSafeDeleteList,
  WaitForLoadingThread, the exit ad unless mSkipAd, **WriteToRegistry()**. There is no DoMainLoop or UpdateApp.
- **Shutdown** (0x47cbf0), once: mShutdown; demo volumes restored; wait for the cursor thread; StopAllMusic; abort the
  HTTPTransfer; restore the display mode when fullscreen; hide the window; WriteToRegistry() if mReadFromRegistry.
  1.2's cross-thread check (mLoadingFailed when called from the loading thread) is absent.
- **~SexyApp** (0x48f000): Shutdown(); if mUserChanged3DSetting a message box about hardware acceleration and registry
  "Is3D"; deletes dialogs, windows, WidgetManager, DDInterface, MusicInterface, SoundManager. The port shows no prompt.

#### 1.4 Update/draw timing (Process 0x480700, DrawDirtyStuff 0x486510, Redraw 0x4884f0)

`Process()`: if mLoadingFailed, Shutdown(). Unless mPaused or mUpdateMultiplier <= 0: `aFrameTime = (int)(mFrameTime /
mUpdateMultiplier)` (10 ms) and `behind = now - mLastTime`. If `behind >= aFrameTime` and (`++mNonDrawCount < 10` or
`!mLoaded`): **one** DoUpdateFrames(), ProcessSafeDeleteList(), `mLastTime += aFrameTime`, and the backlog is clamped to
**1000 ms**. Otherwise mNonDrawCount = 0, DrawDirtyStuff(), and a Sleep for the rest of the frame; while the loading
thread runs (mYieldMainThread) an extra `Sleep(min(250, 2*busy - slept))`.

`DrawDirtyStuff()` disallows Rand for its duration (MTRand::SetRandAllowed), runs the idle check once a second,
returns false while gScreenSaverActive, and draws when `mWidgetManager->DrawScreen()` drew something or 1 s has passed
since the last present, and `now >= mNextDrawTick`. It sleeps for VSync only if mWaitForVSync && mIsWindowed. While the
loading thread runs, `mNextDrawTick += 35 + max(drawTime, 15)` holds the loading screen to about 20 fps.

Against the port's 1.3 state machine: the same 10 ms tick and the same "at most 10 updates, then a forced draw once
loaded" rule; the port clamps the backlog at 200 ms, calls UpdateF after each update (2004 has none) and has no
loading-screen throttle. The idle check freezes updates and draws when the user has been idle longer than the Windows
screen-saver timeout minus 2 s; the port does not copy it.

#### 1.5 Input dispatch (WindowProc 0x489310 -> mDeferredMessages -> ProcessDeferredMessages 0x47fa40)

WindowProc queues key, char, mouse, timer, size, close and activate messages (unless mNoDefer) and handles WM_PAINT,
WM_SETCURSOR (EnforceCursor), WM_ENDSESSION (Shutdown) and Alt+Enter (SwitchScreenMode(!mIsWindowed), only if mLoaded
and not forced) at once. Start's loop runs ProcessDeferredMessages before every Process():

- **Keys**: a key down sets mLastUserInputTick; Ctrl+Alt+D toggles mDebugKeysEnabled; `if (!mDebugKeysEnabled ||
  !DebugKeyDown(key)) WidgetManager::KeyDown(key)`; key up and WM_CHAR (one byte) go to WidgetManager::KeyUp and
  KeyChar. As the port.
- **Mouse**: every mouse message first sends `MouseDrag(x,y)` if any of the app's button flags 0x4f9/0x4fa/0x4fb is
  set, else `MouseMove(x,y)`; then the buttons: L down 1, L double-click 2, R down -1, R double-click -2, M down 3 go
  to `MouseDown(x,y,code)` with SetCapture, ups to `MouseUp(x,y,code)`, the wheel to `MouseWheel(delta/120)`. Mouse
  leave is found on WM_TIMER by polling the cursor. The port passes the same click codes; its WidgetManager decides
  drag or move from mDownButtons, with the same result.
- **Focus** (RehupFocus 0x47cf50): focus = mActive && !mMinimized; on gain `Unmute(true)`, WidgetManager::GotFocus,
  GotFocus(); on loss `Mute(true)`, WidgetManager::LostFocus, LostFocus(). The framework always mutes while inactive
  (the port's mMuteOnLostFocus, default true); a game's GotFocus and LostFocus overrides pause it.
- WM_CLOSE on either window: mManualShutdown = true, Shutdown(). WM_TIMER also times an opened URL out: after 5 s of
  ticks URLOpenSucceeded if the browser took the focus, URLOpenFailed if 12 s pass while the game is still active.

**EnforceCursor** (slot 10) does nothing while mWidgetManager is NULL. With !mSEHOccured && mMouseIn, DDInterface draws
the image cursor `mCursorImages[mCursorNum]` **only if mCustomCursorsEnabled or a demo is playing**; otherwise the
Windows cursors are used. There are **6 cursor ids**: 0 arrow, 1 hand, 2 dragging, 3 text, 4 circle-slash, 5 none. The
port's enum has 13: ids 0 to 4 match (POINTER, HAND, DRAGGING, TEXT, CIRCLE_SLASH), its 5 is SIZEALL, and 2004's 5 is
the port's CURSOR_NONE (11), mapped by a game-side constant. SetCursorImage takes ids 0 to 5; EnableCustomCursors is
the registry's CustomCursors.

#### 1.6 Loading thread

StartLoadingThread always runs `LoadingThreadProc()` on its own `_beginthread` thread; the main thread keeps running
Process (the title screen), and DoUpdateFrames calls LoadingThreadCompleted() on the main thread at the first update
after the thread ends. The game's LoadingThreadProc bumps mCompletedLoadingThreadTasks for the progress bar and checks
mShutdown. The port runs it synchronously, so a progress bar jumps from empty to full.

#### 1.7 Registry, properties, files

- **Registry**: HKLM\SOFTWARE\<mRegKey>[\sub], the value name being the text after the last `\`; the port emulates
  "SOFTWARE\<mRegKey>\...". RegistryWrite/WriteString/WriteInteger/WriteBoolean (a DWORD 0/1), RegistryEraseValue,
  RegistryRead/ReadString (**at most 255 bytes**)/ReadInteger/ReadBoolean (any nonzero DWORD is true). No
  RegistryReadData/WriteData/EraseKey/GetSubKeys. All return false when mRegKey is empty, and all are recorded into and
  replayed from the demo buffer.
- **WriteToRegistry** (0x483a10) writes MusicVolume and SfxVolume (int volume*100), Muted (mMuteCount - mAutoMuteCount >
  0), ScreenMode (!mIsWindowed), CustomCursors, ITSuccess, TimesPlayed, TimesExecuted, LastVerCheckQueryTime, InProgress =
  0, WaitForVSync, RegName and RegCode; unless a demo plays it also writes %WINDIR%\popcinfo.dat (records {u16 len,
  mProdName, u16 TimesPlayed, u16 TimesExecuted}), the store that stops a trial being reset. **ReadFromRegistry**
  (0x484800) sets mReadFromRegistry, takes mRegKey = GetString("RegistryKey", mRegKey) (nothing more if empty) and reads
  the same values back, plus ReferId (registry, then property), RegisterLink, DontUpdate, DownloadId and Variation;
  TimesPlayed/TimesExecuted both become **100** (trial over) if they differ from popcinfo.dat; InProgress sets
  mLastShutdownWasGraceful = (value == 0) and then writes 1; RegName also sets mUserName; `mIsRegistered |=
  Validate(name, code)`. The port keeps the volumes, screen mode and cursors and drops popcinfo.dat and the trial rule.
- **Properties**: LoadProperties(file, required, checkSig) returns true for a missing file unless required; checkSig
  verifies file+".sig" (RSA/BigInt), which the port skips. Accessors: GetBoolean/GetInteger/GetString(id, default),
  GetString(id) ("" when missing), SetString, SetBoolean, SetInteger, SetDouble; no GetDouble or GetStringVector,
  though the maps exist. BookWorm's partner.xml sets ReferId, NoReg, DontUpdate, NoInternetScores, NumOptionsLinks,
  OptionsLinkText0/URL0 (a "more games" link, never clicked in tests), ShowTrialStatus, LoadingBarY,
  OptionsDialogHeight and NoIHSUpsell.
- **Files**: ReadBufferFromFile (whole file, "rb"), WriteBytesToFile, WriteBufferToFile, FileExists (fopen "rb"), on
  plain CRT paths relative to the exe folder. The port has the same methods through its VFS, and writes go to the
  user's folder.

**3D and the FPU (inferred).** Because BookWormApp sets mAutoEnable3D, a 3D-capable machine may run with
DDInterface::mIs3D on. DDInterface::Init (0x4619f0) calls SetCooperativeLevel with DDSCL_NORMAL (8) windowed or
FULLSCREEN|EXCLUSIVE (0x11), never DDSCL_FPUPRESERVE, so once the Direct3D7 device exists the x87 control word may be
at 24-bit precision on the main thread; game double math before `_ftol` would then round like float. Only matters for
bit-exact reproduction of float-derived values; the port computes in full precision.

#### 1.8 Other non-virtual SexyApp functions

Virtual in 1.2 and non-virtual here: GetImage (always commits bits; no shared images), IsAltKeyUsed, CheckSignature,
ShouldCheckForUpdate (false if mDontUpdate or checked less than 7 days ago after a clean exit) and UpdateCheckQueried.
With 1.2's names and meaning: ClearUpdateBacklog(relaxForASecond), WaitForLoadingThread, SetCursor, SetCursorImage,
EnableCustomCursors, CopyImage, FlipImage, Is3DAccelerated (mDDInterface->mIs3D), Is3DAccelerationSupported,
Is3DAccelerationRecommended, Set3DAccelerated(is3D, reinit), CopyToClipboard, GetClipboard, ProcessSafeDeleteList,
AddMemoryImage, RemoveMemoryImage, Remove3DData, the DemoSync family and the property and registry accessors of 1.7.
The image helpers are in section 5. The demo recorder, the debug displays (F9 memory, F11 dump, FPS), the SEH catcher
with its report windows and InternetManager's update check are not ported.

## 5. Graphics, images and fonts

Symbols: `games/bookworm/docs/symbols/10_framework_graphics.jsonl`.

### 0. Summary

| class | sizeof 2004 | 1.2 (Big Money) | port 1.3 | base classes 2004 |
|---|---|---|---|---|
| Graphics | 0x58 | 0x54 | larger (GraphicsState + state stack + scale) | none (1 virtual: dtor) |
| Color, Font | 0x10, 0x14 | same | same | none |
| Image | 0x18 | 0x18 (see note) | larger (mDrawn, mFilePath, mNumTotal) | none |
| MemoryImage | 0x48 | 0x4c (see note) | larger (+ HD fields) | Image |
| DDImage | 0xc4 | 0xd0 | different (no DirectDraw) | MemoryImage |
| ImageFont, SysFont | 0x48, 0x1c | 0x50, 0x1c? | 0x50 +, not in port | Font |
| FontData | 0x198 | DescParser + FontData | DescParser + FontData | **none** (no DescParser class in 2004) |
| FontLayer, CharData, ActiveFontLayer, RenderCommand | 0x1207c, 0x120, 0x1010, 0x34 | same | same | none |
| ImageLib::Image | 0x10 | 0x10 | replaced by platform/ImageLoader | none |
| ShadowFont (game) | 0x60 | n/a | game class | ImageFont, Font |

`FRAMEWORK_12.md` lists Image as 0x1c with `mNumTotal` (generated from the public header), but Big Money's real Image is
0x18 and its MemoryImage offsets are 4 lower than FRAMEWORK_12.md says. BookWorm's MemoryImage is a further 4 bytes
shorter (no mPurgeBits / mWantPal).

#### Folded getters: read these carefully in the game decompilation

MSVC merged identical tiny functions, and Ghidra named the merged copy after one of its users. In the game code:

| address | Ghidra name | what the game call means |
|---|---|---|
| 00446d10 | `get_field_04` | `Image::GetWidth()` (on images), `Font::GetAscent()` (on fonts, slot 1), `Color::GetGreen()` |
| 00468620 | `Sexy::Font::vf02` | `Image::GetHeight()` (on images; with 68 callers, almost always this), `Font::GetAscentPadding()` (slot 2), `Color::GetBlue()`; also `Buffer::GetDataPtr()` and other `this+8` reads in framework code |
| 00496c00 | `get_field_0c` | `Font::GetHeight()` (slot 4), `Color::GetAlpha()`, `ImageLib::Image::GetBits()` |
| 0041d910 | `?` (game region) | `Font::GetLineSpacingOffset()` (slot 5) |
| 00440270 | `FUN_00440270` | `Color::GetRed()` |
| 0041e8c0 | `Sexy::Effect::vf01` | also `FontData::Ref()` (`++[this+8]`) |
| 00446d50 | `?` | empty `ret 0x18`: Image::Blt, Image::StretchBlt, Font::DrawString |
| 0041dfa0 / 00401570 | `?` / `FUN_00401570` | empty `ret 0xc` / `ret 4`: Image::FillRect / Image::ClearRect (also other empty virtuals) |
| 004dc641 | `?` | `_purecall` (Font::Duplicate, DataElement::Duplicate) |

### 1. Color (no vtable, 0x10)

Fields +0 mRed, +4 mGreen, +8 mBlue, +0xc mAlpha, and the behaviour of 1.2 and the port: `Color()` is
(0,0,0,255); `Color(int 0xAARRGGBB)` turns alpha 0 into 255; `Color(r,g,b)` and `Color(const int* rgb)` have
alpha 255; `ToInt()` is a<<24 | r<<16 | g<<8 | b. `Color(r,g,b,a)` is the folded 0x437e90.

### 2. Graphics (0x58, vtable 00502210, 1 slot)

The one virtual is the destructor.

| off | type | name | meaning |
|---|---|---|---|
| +0x00 | vfptr | | 00502210 |
| +0x04 | Image* | mDestImage | NULL in the ctor becomes `&mStaticImage` (005698e8, a 0x0 Image) |
| +0x08, +0x0c | **int** | mTransX, mTransY | translation (1.2 and port: float) |
| +0x10 | Rect | mClipRect | (0,0,dest w,dest h) |
| +0x20 | Color | mColor | default (0,0,0,255) |
| +0x30 | Color | (unknown) | default-constructed by both ctors, **not copied** by the copy ctor, read by no linked function; not in port |
| +0x40 | Font* | mFont | NULL |
| +0x44 | int | mDrawMode | 0 normal, 1 additive |
| +0x48, +0x49 | bool | mColorizeImages, mFastStretch | false |
| +0x4c..+0x57 | 12 bytes | probably mPFActiveEdgeList / mPFNumActiveEdges / mPFNumVertices (inferred) | never initialised, no PolyFill linked |

Absent compared with 1.2/port: mWriteColoredString (a **static on Widget** here, 0052f96c, value 1),
mLinearBlend, mScaleX/Y, mIs3D, mStateStack (no PushState/PopState in the binary).

Every other function is non-virtual with the port's name and meaning: the constructors `Graphics(Image*)` (NULL selects
the static 0x0 image, so `Graphics(NULL)` has a 0x0 clip rect) and `Graphics(const Graphics&)`; `Create()`; the
getters and setters; `Translate(int, int)`; `DrawString` (`if (mFont) mFont->DrawString(...)`); `StringWidth` (no NULL
check); `FillRect` (returns if mColor.mAlpha == 0); `DrawRect` (the outline of (w+1)x(h+1)); `DrawImage(img,x,y)`
(Image slot 6 Blt, colour = mColorizeImages ? mColor : White); `DrawImage(img,x,y,src)` (returns silently if src exceeds
the image); the stretched `DrawImage(img,dest,src)` (dest is clipped here, src becomes a double rect, Image slot 9);
`DrawImageRotated(img, int x, int y, rot, src)` (00450230), which calls `DrawImageRotatedF(img, float x, float y, rot,
src)`, whose centre is the **integer** w/2, h/2 (the port's float overload uses w/2.0f); `DrawImageRotatedF(img,x,y,rot,
cx,cy,src)` (Image slot 8 BltRotated with mClipRect); `ClipRect`; and `DrawImageBox(dest,img)` /
`DrawImageBox(src,dest,img)` (3x3 **tiling**, like the port).

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

| slot | 2004 method | notes |
|---|---|---|
| 0 | `~Image()` | body 00458fa0 deletes mAnimInfo |
| 1 | `FillRect(const Rect&, const Color&, int mode)` (empty) | 1.2 slot 1 PolyFill3D does not exist yet |
| 2 | `DrawRect(const Rect&, const Color&, int)` | 4 FillRects |
| 3 | `ClearRect(const Rect&)` (empty) | |
| 4 | `DrawLine(double x4, const Color&, int)` (empty) | MemoryImage does not override: only DDImage draws lines |
| 5 | `FillScanLines(Span*, int, const Color&, int)` | |
| 6 | `Blt(Image*, int x, int y, const Rect& src, const Color&, int mode)` (empty) | |
| 7 | `BltF(Image*, float x, float y, const Rect& src, const Rect& clip, const Color&, int)` (empty) | |
| 8 | `BltRotated(Image*, float x, float y, const Rect& src, const Rect& clip, const Color&, int, double rot, float cx, float cy)` (empty) | |
| 9 | `StretchBlt(Image*, const Rect& clippedDest, const FRect& src, const Color&, int mode, bool fast)` (empty) | **different signature** from the port's `(Image*, dest, Rect src, clip, colour, mode, fast)`: no clip rect, double source rect |

Not present: PolyFill3D, BltMatrix, BltTrianglesTex, BltMirror, StretchBltMirror (and the port's DrawLineAA,
FillScanLinesWithCoverage).

Fields (verified, ctor 00446cf0 and copy ctor 00460190): +0 vfptr, +4 mWidth, +8 mHeight, +0xc mNumRows (1), +0x10
mNumCols (1), +0x14 AnimInfo* mAnimInfo. No mNumTotal, mDrawn, mFilePath (the same as Zuma 1.0 / CircleShootApp).
`CopyAttributes(Image*)` copies rows, cols and AnimInfo (0x30, laid out as 1.2: type, delay, numCels, two vectors,
total time). There is no GetCelWidth, GetCelRect or GetAnimCel.

### 5. MemoryImage (0x48, vtable 00502ab4, 26 slots)

Slots 0 to 9 are Image's (0 dtor 00459970: RemoveMemoryImage, frees buffers; 1 FillRect; 3 ClearRect; 4 DrawLine is
Image's empty one; 6 Blt: mode 0 -> NormalBlt 00448540, 1 -> AdditiveBlt 00447af0; 7 BltF = BltRotated(..., rot 0,
centre 0,0); 8 BltRotated, clip helper 0044c520; 9 StretchBlt: fast -> FastStretchBlt 0044a4e0, else SlowStretchBlt
004491e0). Then:

| slot | 2004 method | notes |
|---|---|---|
| 10 | `void* GetNativeAlphaData(int bitDepth, ulong rMask, ulong gMask, ulong bMask)` | port: `(NativeDisplay*)`; no NativeDisplay in 2004 |
| 11 | `uchar* GetRLAlphaData()` | |
| 12 | `uchar* GetRLAdditiveData(int, ulong, ulong, ulong)` | port: (NativeDisplay*) |
| 13 | `DeleteSWBuffers()` | |
| 14 | `Delete3DBuffers()` | -> mApp->Remove3DData(this) |
| 15 | `DeleteExtraBuffers()` | |
| 16 | `BitsChanged()` | ++mBitsChangedCount, mBitsChanged, drops alpha caches |
| 17 | `CommitBits()` | recomputes mHasTrans/mHasAlpha unless mForcedMode |
| 18 | `DeleteNativeData()` | |
| 19 | `Clear()` | |
| 20 | `SetBits(ulong* bits, int w, int h)` | port: SetBits(bits,w,h,commitBits=true); **no commitBits**: always BitsChanged + CommitBits |
| 21 | `Create(int w, int h)` | sets mHasTrans = mHasAlpha = true |
| 22 | `ulong* GetBits()` | allocates zeroed bits or expands the palette |
| 23 | `SetImageMode(bool hasTrans, bool hasAlpha)` | sets mForcedMode |
| 24 | `SetVolatile(bool)` | |
| 25 | `bool Palletize()` | Quantize32Bit 0044a800 |

Not present: PurgeBits, ReInit (1.2 slots 18, 22), BltMatrix, BltTrianglesTex, the HD API.

Fields (ctor 004598f0 `MemoryImage(SexyAppBase*)`, copy ctor 00461350, dtor 00459970), all same-named in the port:
+0x18 ulong* mBits (w*h+1, last = 0x4BEEFADE), +0x1c mBitsChangedCount, +0x20 void* mD3DData, +0x24 DWORD mD3DFlags
(inferred), +0x28 ulong* mColorTable (256 entries), +0x2c uchar* mColorIndices, +0x30 bool mForcedMode, +0x31 mHasTrans,
+0x32 mHasAlpha, +0x33 mIsVolatile, +0x34 ulong* mNativeAlphaData, +0x38 uchar* mRLAlphaData, +0x3c uchar*
mRLAdditiveData, +0x40 bool mBitsChanged, +0x44 SexyAppBase* mApp. No mPurgeBits, mWantPal. The ctor registers the
image with the app (AddMemoryImage, the set at app+0x494) and the dtor removes it. A game makes one with
`new MemoryImage(app)` (0x48), Create, SetImageMode(true,true) and `new Graphics(img)`, as in the port.

### 6. DDImage (0xc4, vtable 00502b34, 37 slots)

Slots 0 to 25 are MemoryImage's in the same order. DDImage overrides 0, 1 FillRect, 4 DrawLine, 6 Blt, 7 BltF,
8 BltRotated, 9 StretchBlt, 15 DeleteExtraBuffers, 16 BitsChanged (also releases the surface), 17 CommitBits
(only without a surface), 21 Create (which, unlike MemoryImage's and the port's, does **not** set
mHasTrans/mHasAlpha), 22 GetBits (reads the surface back) and 25 Palletize. Its own slots: 26 SetVideoMemory(bool)
(port: non-virtual stub), 27 RehupFirstPixelTrans, 28 NormalFillRect(const Rect&, const Color&), 29 AdditiveFillRect,
30 NormalBlt(Image*, int, int, const Rect&, const Color&), 31 AdditiveBlt, 32 NormalDrawLine(double x4, const Color&),
33 AdditiveDrawLine (27 to 33 are not in the port), 34 bool LockSurface(), 35 bool UnlockSurface(), 36
SetSurface(LPDIRECTDRAWSURFACE) (34 to 36 are non-virtual stubs in the port). No Normal/AdditiveBltMirror (1.2 slots
41/42). Non-virtual: `DDImage(DDInterface*)`, `GenerateDDSurface()` (00440e40), `DeleteDDSurface()`, `GetSurface()`.

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

| 2004 | notes (port: same names on `SexyAppBase`) |
|---|---|
| `DDImage* GetImage(const std::string&)` (0047d930) | ImageLib::GetImage(file, true), `new DDImage(mDDInterface)`, `SetBits(bits,w,h)`; NULL on failure; no mFilePath, no commitBits argument (port: `GetImage(file, commitBits=true)`) |
| `DDImage* CopyImage(Image*)`, `CopyImage(Image*, const Rect&)` | new DDImage, Create(w,h), `Graphics(img).DrawImage(src,-x,-y)`, CopyAttributes |
| `void FlipImage(Image*)` | a **vertical** flip (swap rows, BitsChanged) |

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
CreateLayerFrom and the Layer... setters); the port has them and adds LayerSetHeight. LayerSetImage resolves its
path against the descriptor's folder, loads through `mApp->GetImage(path)` and calls `Palletize()`.

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
SDLSoundInstance and SDLMusicInterface. Read calls through `mSoundManager` (app+0x4e0) and `mMusicInterface`
(app+0x33c) with the tables below, not 1.2's, and call the port's method of the same name.

`SexyApp::Init` makes `new DSoundManager(mNoSoundNeeded ? NULL : mHWnd, mWantFMod)`, calls SetSfxVolume, then
`CreateMusicInterface(mInvisHWnd)` and SetMusicVolume. CreateMusicInterface returns a plain MusicInterface if
mNoSoundNeeded, an FModMusicInterface if mWantFMod, otherwise a BassMusicInterface. Only the SexyApp ctor
writes the two flags (false), so these games always use DirectSound and BASS, and the FMOD classes are dead
code (no fmod.dll is installed; not ported). The default volumes are 0.85, as in the port.

#### 1. SoundManager / DSoundManager

DSoundManager : SoundManager, 14 slots (1.2 has 16, the port 18). The port's methods have the same names (its
destructor is `~SDLSoundManager`).

| slot | method | notes |
|---|---|---|
| 0 | `~DSoundManager()` scalar deleting | body 00492e00 |
| 1 | `bool Initialized()` | = mDirectSound != NULL |
| 2 | `bool LoadSound(unsigned int id, const std::string& file)` ret 8 | 1.2 slot 2 `int LoadSound(const std::string&)` does **not exist** |
| 3 | `bool SetBaseVolume(unsigned int id, double v)` ret 0xc | reordered: before SetVolume |
| 4 | `bool SetBasePan(unsigned int id, int pan)` ret 8 | |
| 5 | `void SetVolume(double v)` ret 8 | = sfx volume |
| 6 | `SoundInstance* GetSoundInstance(unsigned int id)` ret 4 | |
| 7, 8 | `ReleaseSounds()`, `ReleaseChannels()` | |
| 9, 10 | `GetMasterVolume()`, `SetMasterVolume(double v)` ret 8 | Windows mixer, see 1.4 |
| 11 | `void Flush()` | folded empty `ret` |
| 12 | `void SetCooperativeWindow(HWND, bool isWindowed)` ret 8 | always DSSCL_NORMAL |
| 13 | `void StopAllSounds()` | keeps each instance's mAutoRelease |

Absent here, present in the port: `int LoadSound(const std::string&)`, `ReleaseSound(unsigned)`, `GetFreeSoundId`,
`GetNumSounds`.

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
does nothing (the port has the same 32-channel limit and rule).

##### 1.4 Volumes, pan, pitch

- Sfx volume: `SexyApp::SetSfxVolume` calls `mSoundManager->SetVolume(muted ? 0 : v)`, and every channel sets
  `VolumeToDB(mBaseVolume * mVolume * mMasterVolume)`, where `VolumeToDB(v) = (int)((log10(1+9v)-1)*2333)`
  centi-dB and anything below -2000 is -10000. The port's `SDLSoundManager::VolumeToGain` uses the same curve.
  Above 0 dB DirectSound refuses the call and keeps the buffer's last volume; the port clamps to 0 dB.
- Master volume is **the Windows wave-out mixer** (GetMasterVolume/SetMasterVolume read and write its volume
  control as v * 0xFFFF); `SexyApp::SetMasterVolume` also stores v as mSfxVolume. BassMusicInterface's ctor
  saves it before BASS_Init and writes it back. The port's SetMasterVolume is SetVolume; unused by BookWorm.
- Pan: `buffer->SetPan(mBasePan + mPan)`, hundredths of a dB (-10000..10000), unclamped; the port clamps.
- AdjustPitch: `freq = mDefaultFrequency * 1.0594630943592953^steps`, clamped to [100, 100000] Hz; the port
  uses the ratio 2^(steps/12) without the clamp.

#### 2. SoundInstance / DSoundInstance

DSoundInstance : SoundInstance, 12 slots in the order of 1.2 and the port; every method is byte-identical to Big
Money's: 0 dtor, 1 Release (= Stop(); mReleased = true), 2 SetBaseVolume, 3 SetBasePan, 4
`AdjustPitch(double theNumSteps)`, 5 SetVolume, 6 SetPan, 7 `bool Play(bool looping, bool autoRelease)` (Stop(),
mHasPlayed, mAutoRelease = autoRelease, then play), 8 Stop (stops, rewinds and clears mAutoRelease, only when a
buffer exists), 9 IsPlaying (mHasPlayed and the buffer is playing), 10 IsReleased (auto-releases a finished
autoRelease instance), 11 GetVolume.

Fields (0x38), as 1.2: mSoundManagerP, mSourceSoundBuffer, mSoundBuffer (a DuplicateSoundBuffer of the source),
mAutoRelease, mHasPlayed, mReleased, mBasePan, mBaseVolume, mPan, mVolume, mDefaultFrequency.

#### 3. MusicInterface / BassMusicInterface / FModMusicInterface

15, 15 and 17 slots (1.2: 20/20/22; the port's MusicInterface 20). The base's slots are folded empty stubs
whose `ret` sizes prove the signatures. The port has the method of the same name.

slot: 0 `~X()` scalar deleting; 1 `bool LoadMusic(int songId, const std::string& file)`; 2 `void PlayMusic(int songId,
int offset, bool noLoop)`; 3 `StopMusic(int)`; 4 `PauseMusic(int)`; 5 `ResumeMusic(int)`; 6 `StopAllMusic()`; 7
`FadeIn(int songId, int offset, double speed, bool noLoop)`; 8 `FadeOut(int songId, bool stopSong, double speed)`;
9 `FadeOutAll(bool stopSong, double speed)`; 10 `SetSongVolume(int, double)`; 11 `bool IsPlaying(int)`; 12
`SetVolume(double)`; 13 `SetMusicAmplify(int, double)` (the port's gain table is measured against BASS 2.3,
c553853); 14 `Update()`.

FModMusicInterface adds slots 15 and 16, two LoadSample overloads. 1.2's UnloadMusic, UnloadAllMusic,
PauseAllMusic, ResumeAllMusic and SetSongMaxVolume do not exist; the port has them, so a game's own pause-all
loops over its songs.

BassMusicInterface (0x14): +0x4 `std::map<int, BassMusicInfo> mMusicMap`, +0x10 `int mMaxMusicVolume` (40, used
only on the BASS 1.x path of SetVolume). BassMusicInfo (0x20): +0 HMUSIC mHMusic, +4 HSTREAM mHStream, +8
double mVolume, +0x10 double mVolumeAdd, +0x18 bool mStopOnFade; there is no mVolumeCap (the cap is the
constant 1.0). The port's `SDLMusicInterface::SongInfo` has the same members.

The games ship `bass.dll` **2.0.0.20**. The ctor calls `BASS_Init(1, 44100, 0, hwnd, NULL)` and
`BASS_SetConfig(BASS_CONFIG_BUFFER, 2000)`. BASS_ChannelSetPosition is never loaded, so playing a stream with
an offset > 0 would jump through a garbage pointer; BASS_MusicFree is not imported.

##### 3.2 Music semantics

- **LoadMusic** (00495f50): lower-cased extension; `wav`/`ogg`/`mp3` -> `BASS_StreamCreateFile(FALSE, name, 0, 0, 0)`;
  anything else (`.mo3`, `.it`, `.mod`...) -> BASS 2: `BASS_MusicLoad(FALSE, name, 0, 0, BASS_MUSIC_LOOP|BASS_MUSIC_RAMP
  = 0x204, 0)`; BASS 1: flags 4. Inserted with `map::insert`, so a second LoadMusic for the same id is ignored (port:
  replaces it; BookWorm never reloads).
- **PlayMusic(id, offset, noLoop)** (00494450): mVolume = 1.0, mVolumeAdd = 0, mStopOnFade = noLoop;
  `BASS_ChannelSetAttributes(h, -1, 100, -101)`; module: `BASS_MusicPlayEx(h, offset, (noLoop ? 0 : BASS_MUSIC_LOOP)
  | BASS_MUSIC_RAMP, TRUE)` (BASS 1: no RAMP). The offset goes to BASS unchanged: BASS 2.0's `pos` is
  MAKELONG(order, row), so BookWorm's offsets are **order numbers, row 0**. Unlike 1.2 there is no BASS_ChannelStop
  first. Stream: `BASS_StreamPlay(h, offset != -1, flags)`.
- **FadeIn(id, offset, speed, noLoop)** (00494620): mVolumeAdd = speed, mStopOnFade = noLoop;
  `ChannelSetAttributes(h, -1, (int)(mVolume*100), -101)` (starts from the current volume: 0 for a song that never
  played or was stopped/faded out); offset == -1 -> `BASS_MusicPlay(h)` (continue from the current position; the
  port's `PlaySong(..., resetPosition=false)`); else `BASS_MusicPlayEx(h, offset, noLoop ? 0 : BASS_MUSIC_LOOP, TRUE)`,
  **without BASS_MUSIC_RAMP** even on BASS 2 (1.2 adds it). No ChannelStop first (1.2 has one).
- **FadeOut(id, stopSong, speed)** (00494710): if mVolume != 0, mVolumeAdd = -speed; mStopOnFade = stopSong always.
  With stopSong = false the song keeps playing at volume 0 and still counts as playing. **FadeOutAll** (00493fd0):
  every song. **StopMusic** / **StopAllMusic**: mVolume = 0 + BASS_ChannelStop. **PauseMusic**/**ResumeMusic**:
  BASS_ChannelPause / BASS_ChannelResume.
- **Update** (00494040), per song with mVolumeAdd != 0: mVolume += mVolumeAdd; above 1.0 -> 1.0 and stop fading; below
  0 -> 0, stop fading, and BASS_ChannelStop if mStopOnFade; then `ChannelSetAttributes(h, -1, (int)(mVolume*100), -101)`
  (volume quantised to whole percent by truncation; the port's volumes are continuous).
- **SetSongVolume** (00494760): mVolume = v and ChannelSetAttributes. **IsPlaying** (004947c0):
  `BASS_ChannelIsActive(h) == BASS_ACTIVE_PLAYING` (paused or stopped = false). **SetMusicAmplify** (00494810):
  `BASS_MusicSetAmplify(h, (int)(amp*100))` (BASS default 50). The game calls neither.
- **SetVolume(v)** (00492790): BASS 2: `BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC = 6, (int)(v*100))` only (1.2 also sets
  GVOL_STREAM 5); BASS 1: `BASS_SetGlobalVolumes(v*40, v*40, v*40)`. So with BookWorm's BASS 2.0, mMaxMusicVolume = 40
  is unused and the music master gain is v (0..1), not 0.4 v.

#### 4. Behaviour differences and what the port must do

1. **Music master volume**: the BASS 2.0 path of SetVolume sets the global MOD volume to v
   (`BASS_CONFIG_GVOL_MUSIC`, v*100, linear), where the port's `SDLMusicInterface::SetVolume` uses
   v * mMaxMusicVolume / 100 = 0.4 v, the BASS 1.x rule. The game side sets `mMaxMusicVolume = 100` on the
   music interface its app creates (an override of CreateMusicInterface) and calibrates the libopenmpt render
   level against BASS 2.0 (default amplify 50, linear interpolation), by ear or with a capture: the balance of
   music and sound depends on it.
2. **Double music Update** (BookWorm): its UpdateFrames override calls the base, whose last step is
   `mMusicInterface->Update()`, and then Update() again, so every fade moves 2 x speed per tick (0.005 is a
   1.0 s full fade at 100 updates/s). Translate the override literally; do not "fix" it.
3. **MO3 order offsets**: a module's offset goes to `BASS_MusicPlayEx(h, MAKELONG(order,row), flags, TRUE)`,
   so the games' offsets are order numbers at row 0; the port maps them to `Mixer::PlaySong(order, row)` and
   `openmpt_module_set_position_order_row`. Each LoadMusic of the same file is an independent module
   instance, in BASS and in the port. BASS follows backward position jumps (Bxx) even without BASS_MUSIC_LOOP
   and stops a non-looping song only at the end of the order list, while libopenmpt with repeat count 0 stops
   at its detected song end: one-shot jingles (noLoop) are where this shows.
4. **Volume ramping** (minor, inferred): modules load with BASS_MUSIC_RAMP and PlayMusic keeps it, but FadeIn
   with an offset replays without it. The port does not ramp unless asked.
5. **Sound lookup order**: the port tries `.ogg`, `.wav`, `.au`, `.mp3` and has no cache, where the original
   tries the cache, `.wav`, `.ogg`, `.au`. The result differs only if a mod ships both a .wav and an .ogg of
   one name. The port never writes `cached_*` files.
6. **Load failure**: the original's LoadSound returns true without a device and false only for a missing or
   undecodable file; the port returns false for a missing file even without a device. The game's shutdown
   check is kept, so silent and headless runs need the files to resolve.

## 7. Support classes and library helpers

### 7.1 Support classes

| Area | Verdict for the port |
|---|---|
| Common helpers, Buffer | 1:1 with `framework/Common.h` and `framework/Buffer.h`; differences below |
| MTRand / Sexy::Rand / SRand | same generator, **different seeding** (SAF 1.2 Knuth 69069, port 1.3 1812433253) |
| PropertiesParser / XMLParser | 1.2 code, only `ParsePropertiesBuffer` linked (partner.xml after a signature check); the port's 1.3 XMLParser has the same grammar for these files |
| ResourceManager | constructed in the SexyApp ctor (app+0x60c), deleted in the dtor, never touched: **linked but unused** |
| HTTPTransfer, InternetManager | game code uses them (Hall of Fame scores, "check for updates"); port: drop or stub game-side |
| BetaSupport, SEHCatcher, PerfTimer/SexyPerf, ModVal, DirectXErrorString, BigInt | droppable |
| DDInterface / D3DInterface / D3DTester | replaced by the port's SDL `DDInterface`; the game only does `new DDImage(app->mDDInterface)` |

**MTRand** (0x9c4: `ulong mt[624]`, `int mti` at +0x9c0). `SRand(ulong)` (004665f0) is SAF 1.2's: seed 0 becomes 4357,
`mt[0] = seed; mt[i] = mt[i-1] * 69069`, mti = 624. The port's is 1.3's `1812433253 * (x ^ x>>30) + i`, so the same
seed gives another stream; Next (masked with 0x7fffffff), tempering, Serialize and SRand(string) are identical. A
game's own generators (`new MTRand(Sexy::Rand())`) are therefore seeded through the shim's `SeedMTRand2004` (Big Money:
`SeedMTRand12`); the global stream (`Sexy::SRand(GetTickCount())` in Init) matters only for deterministic tests.
SRand(string) copies the words but not mti, in the original and in the port, so a loaded generator continues from its
fresh generator's mti (624): load original saves with it, not with SetState. Not in this build: `Next(range)`,
`Next(float)`, `Sexy::Rand(int)`, `Sexy::Rand(float)`; games take ranges with `% n`. `MTRand::SetRandAllowed(bool)` is
a counter the app raises while it draws and while it deletes widgets.

**Buffer** (0x20) is 1.2's and the port's, bit for bit. `ToWebString()` is "%08X" of mWriteBitPos and ceil(bits/6)
characters of `.-0-9A-Za-z`; FromWebString decodes lowercase hex in the length as `c - 'f' + 10` (a 1.2 bug kept in the
port and never triggered, because ToWebString writes uppercase; do not fix it, or old saves could change meaning).

**Common helpers**: GetFileDir has no withSlash parameter (the port's `GetFileDir(path, false)`); MkDir splits on `\`
only; StringToInt accepts an optional leading '-', decimal digits, or hex after "0x"/"0X" (so "-x1F" is -31; no '+', no
spaces; bad input gives false and 0); XMLDecodeString knows lt, amp, gt, quot, apos, nbsp and cr, and an unknown
`&name;` becomes "&"; CommaSeperate(0) is "0" and a negative gives ""; StrFormat gives up at 1,000,000 characters and
cuts at an embedded NUL (the port does neither). Not linked: GetFileName, AddTrailingSlash, XMLEncodeString,
StrPrefixNoCase, StringToWString, Lower.

**PropertiesParser**: the root element must be `Properties`; sections are `<String id>`, `<StringArray id>` (of
`<String>`), `<Boolean id>` (Upper(value) in 1/YES/ON/TRUE or 0/NO/OFF/FALSE), `<Integer id>` and `<Double id>`;
anything else is "Invalid Section". Only double quotes delimit attribute values and unquoted ones are accepted.

**HTTPTransfer** (0xfc) has Get(url), Get(base, rel), Post(url, params), Abort, GetResultCode and GetContent; result
codes 0 DONE, 1 NOT_STARTED, 2 NOT_COMPLETED, 3 NOT_FOUND, 4 HTTP_ERROR, 5 ABORTED, 6 SOCKET_ERROR, 7 INVALID_ADDR,
8 CONNECT_FAIL, 9 DISCONNECTED. It is not in the port: a game drops its use (as BookWorm's Hall of Fame does) or stubs
it game-side so that it never connects; partner.xml's NoReg, DontUpdate and NoInternetScores already select the
offline paths.

**DDInterface** (0xcf0, app+0x334; no NativeDisplay base): `mIs3D` is at +0x10 (Is3DAccelerated; set from the
registry's "Is3D" before Init) and the D3DTester at +0xc: `Is3DAccelerationSupported()` is tester && mFailureReason
empty, `Is3DAccelerationRecommended()` also needs mWarning empty. Init requires a 16- or 32-bit display in a window and
calls SetDisplayMode(mWidth, mHeight, mFullscreenBits) in full screen.

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
11. **Throw helpers and EH noise**: functions Ghidra names `std::length_error::ctor_or_dtor_*` or
    `std::out_of_range::ctor_or_dtor_*` in the game region are container functions that contain a throw
    (`list::_Incsize`, `vector::_Xlen`, `_Tree::_Insert`, `_Tree::erase(iterator)`); the game never catches them:
    translate the container operation and drop the check. `ExceptionList`, `local_8 = N` and the `Catch@`/`Unwind@`
    funclets are destructor-on-unwind bookkeeping; `__chkstk` is a stack frame over 4 KB.
12. **`_ftol()`** after x87 math: Ghidra shows it without an argument because the operand is on the FPU stack; read the
    disassembly. It is `(int)x`, truncation toward zero, 64 bits returned of which the game uses EAX, so values outside
    the int range wrap instead of saturating. Ghidra also hides the stores of several small helpers (struct copies,
    `_Uninit_copy`, `_Uninit_fill_n`, `basic_streambuf::_Init`): check the disassembly.
13. **Old iostream (no `std::`)**: `ifstream::ifstream`, `istream::get_str` (= `getline(buf, n)` when the `_fGline`
    member at +4 is incremented just before) and `ios::lock/unlock` are `std::ifstream f(path); while (f.getline(buf,
    n)) ...` in the port.

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

Game code written against the 2004 framework is translated onto the port's SAF 1.3 method by method; the table lists
every old behaviour that would otherwise change what the player sees or hears, and where the port handles it. "Game
shim" means `framework/compat2004/Compat2004.*` (CMake target `sexycompat2004`), which every game of this generation
links: `Dialog2004`, `Slider2004`, `WriteString2004` / `WriteWordWrapped2004` / `GetWordWrappedHeight2004`,
`SeedMTRand2004`, `CrtRand` / `CrtSRand`, `GameTickCount`, `GetLastMouseDownTick`. Details are in the sections cited.

| # | 2004 behaviour (port) | Handling | Section |
|---|---|---|---|
| 1 | Dialog yes/no (footer) buttons have ids `mId+2000` / `mId+3000`, labels "YES"/"NO"/"OK"/"CANCEL", and forward to the **app**; no DialogListener, no mResult (port: ids 1000/1001, GetString labels, DialogListener) | game shim: a Dialog subclass that renumbers and relabels the buttons and forwards their events to the app | 3 (D1), 4 |
| 2 | ButtonListener 6 slots in another order; Widget 47 slots, Dialog 52, SexyApp 67 | none: call methods by name | 3, 4 |
| 3 | WidgetManager records the tick of the last mouse-down (WM+0x168; 0 before the first press); Lex's idle animation compares it with GetTickCount() | framework `WidgetManager::mLastMouseDownTick`, set in MouseDown from `SexyAppBase::GetClockTime()`; the game compares it with GetClockTime() and treats 0 as "never pressed" | 3 (D2) |
| 4 | Slider: a click on the track does not move the thumb; SetValue does not clamp (port: 1.3 jumps) | game shim: Slider subclass ignoring track clicks | 3 (D4) |
| 5 | Widgets without a font fall back to `SysFont("SansSerif", ...)` (port: none) | a translation must keep every SetFont call | 3 (D5), 5 |
| 6 | Text layout is Widget virtuals taking a Graphics*; WriteWordWrapped keeps extra spaces after a wrap and adds no height for a trailing newline | game shim `WriteString2004` etc. for every Widget text write (BookWorm: 41 word definitions have double spaces) | 3 (D11), 5 |
| 7 | `MTRand::SRand` uses 1.2 seeding (port: 1.3) | game shim `SeedMTRand2004` (Big Money: `SeedMTRand12`); `CrtRand` / `CrtSRand` reproduce MSVC's `rand()` | 7.1 |
| 8 | Music: BASS 2.0 MO3 by order position; UpdateFrames calls the music Update twice per tick; BASS sets the module global volume to v (port: libopenmpt via `platform/openmpt`, SDLMusicInterface) | translate the double Update literally; set the music interface's maximum volume as the game does; calibrate loudness by ear | 6 |
| 9 | Sounds: `cached_<name>.wav` decode cache, then .wav/.ogg/.au; a failed load ends Init | never write a cache; keep the failure check | 6 |
| 10 | Cursor ids 0-5 (5 = none) (port: CURSOR_* enum) | map id 5 to CURSOR_NONE | 4 |
| 11 | Unknown command-line switches pop up an error and exit (port: -seed, -script, -fast) | do not translate the base HandleCmdLineParam | 4 |
| 12 | Timing: 100 updates/s; backlog clamp 1000 ms; idle "screen saver" freeze; focus loss mutes; real loading thread; game clocks read GetTickCount() (port: 200 ms clamp, synchronous loading; GetTickCount()/timeGetTime() are real time even under -fast, `SexyAppBase::GetClockTime()` is the clock -fast advances) | keep the port's loop; mute on focus loss; game timing that must repeat in scripted -fast runs (level clocks, idle timers) reads GetClockTime() | 4 |
| 13 | Online code (HTTPTransfer, InternetManager, registration, BetaSupport, SEH reporting, demo record/playback) | not ported: drop (partner.xml selects the offline, registered paths) | 4, 7.1 |
| 14-18 | Folded getters (read by the object's type); `DrawImageRotated(int ...)` rotates about integer centres (use the port's int overload); image loading (.tga/.jpg/.png/.gif, masks `_name` and `name_`) and ImageFont behave as the port's `platform/ImageLoader` and ImageFont; ButtonWidget calls ButtonDepress before MarkDirty without a focus check, and the base Widget sets mIsOver/mIsDown/the finger cursor (port: WidgetManager) | none | 5, 7.1, 3 (D7, D8) |
| 19 | No WidgetContainer: one flat widget list in screen coordinates | translate AddWidget/RemoveWidget literally as top-level widgets | 3 |
| 20 | CursorWidget, transient widgets, UpdateCheckDialog, ResourceManager, FModMusicInterface | drop | 3, 6, 7 |
| 21 | Direct3D texture addressing: a game sets the address mode for u and v (`SetTextureStageState` 0xd and 0xe; 1 wrap, 3 clamp) round its `DrawTrianglesTex` calls (Bejeweled 2's Hyperspace tunnel wraps); SDL's renderer has no address mode (clamp) | framework: `DDInterface::SetTextureAddress(u, v)` with `TEXTURE_ADDRESS_WRAP` or `_CLAMP` (default); with a wrap set, `DrawTrianglesTex` of both displays cuts the triangles at whole-number u and v (`framework/TriangleWrap.cpp`, tested by `tools/wraptest`) and draws the pieces clamped; a game calls it before and after the draws, as the original did | none |

## Reuse for Typer Shark and AstroPop

Typer Shark! Deluxe (game PE linked 2004-04-20) and AstroPop Deluxe (2004-04-06) were built with the same compiler from
the same framework snapshot (identical framework RTTI sets); byte matching with `tools/funcmatch.py` pairs 1,018 of
BookWorm's 1,388 framework functions with Typer Shark's. The slot tables and field offsets therefore carry over class
by class, but each class should be checked against the new binary before its offsets are trusted: the larger functions
differ, so some layouts may too.
