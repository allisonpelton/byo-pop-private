# The 2003 framework in Dynomite Deluxe, and how it maps onto the port

Dynomite Deluxe's game image (carved from the 2006 wrapper, 1,277,952 bytes) was linked on 2003-07-08 with Visual
C++ .NET 2002 (MSVC 7.0, linker 7.0). Its SexyAppFramework sits between the two that ports already map: Noah's Ark's
of 2002 (`games/noahsark/docs/FRAMEWORK_2002.md`, Visual C++ 6.0) and BookWorm's of early 2004
(`docs/FRAMEWORK_2004.md`, the same compiler as Dynomite). Atomica (2003-03-28, also MSVC 7.0,
`games/atomica/docs/FRAMEWORK_2003.md`) is the nearest finished port. This note says where Dynomite's framework is
the 2004 one, where it is older, and what the port does about it.

## How the map was made

The framework classes have RTTI (115 classes, `tools/rtti_msvc70.py`); the game's classes have none. Byte matching
with `tools/funcmatch.py` against BookWorm's export named 1,163 framework, STL and C library functions
(`docs/symbols/05_bookworm_match.jsonl`); against Atomica's it paired 1,680 functions but carried few names, since
Atomica's export is mostly unnamed. The vtables of the game classes were found by scanning `.rdata` for runs of code
addresses and the functions that store them (`tools/vtscan.py`), and each was matched with the framework vtable it
extends to name its slots (`tools/vtsyms.py`, `docs/symbols/10_vtables.jsonl`). The slot tables below were read
from the bodies of the slot functions, with the 2004 tables as the guide.

The image's layout: game code 0x401000-0x441fff, the framework 0x442000-0x498fff (with the STL instances it uses),
the Ogg Vorbis decoder (Tremor) 0x49a000-0x4c8fff, then the C and C++ libraries; `.rdata` 0x4f7000, `.data`
0x52b000. Strings are MSVC 7.0's `std::string` (28 bytes: the 16-byte small buffer or the pointer at +4, the size
at +0x14, the capacity at +0x18), as in BookWorm and Atomica.

## Widget: the 2004 widget without four slots

`Widget` is 0x58 bytes, and its constructor is byte for byte Big Money's and BookWorm's: the fields are those of
`docs/FRAMEWORK_2004.md` section 3.1.2 (`mX` +8, `mY` +0xc, `mWidth` +0x10, `mHeight` +0x14, `mVisible` +0x19,
`mDisabled` +0x1d, `mHasFocus` +0x1e, `mIsDown` +0x1f, `mIsOver` +0x20, `mWidgetManager` +0x24, `mColors` +0x28,
`mDoFinger` +0x48, `mUpdateCnt` +0x4c). The widgets are flat, with no WidgetContainer: every widget is top-level and
its `mX`/`mY` are screen coordinates.

The vtable has 43 slots where 2004 has 47. The four that are missing are the two mouse handlers with a button
argument (2004 slots 26 and 28), `MouseWheel` (32) and `IsPointVisible` (33). So the slots are: 0 destructor,
1 `AddedToManager`, 2 `RemovedFromManager`, 3 `OrderInManagerChanged`, 4 `SetVisible`, 5 and 6 `SetColors`,
7 `SetColor`, 8 `GetColor`, 9 `SetTransient`, 10 `SetDisabled`, 11 `ShowFinger`, 12 and 13 `Resize`, 14 `Move`,
15 `WantsFocus`, 16 `Draw`, 17 `Update`, 18 `GotFocus`, 19 `LostFocus`, 20 `KeyChar`, 21 `KeyDown`, 22 `KeyUp`,
23 `MouseEnter`, 24 `MouseLeave`, 25 `MouseMove`, 26 `MouseDown(x, y, clickCount)` (sets `mIsDown`), 27
`MouseUp(x, y, clickCount)` (calls slot 28), 28 `MouseUp(x, y)` (clears `mIsDown`), 29 `MouseDrag`, 30 and 31
`WriteCenteredLine`, 32 `WriteString`, 33 `WriteWordWrapped`, 34 `GetWordWrappedHeight`, 35 `GetNumDigits`, 36
`WriteNumberFromStrip`, 37 `Contains`, 38 `GetInsetRect`, 39 `GetRect`, 40 `Intersects`, 41 `MarkDirtyFull`,
42 `MarkDirty`. A game widget's own virtuals start at slot 43.

As in Atomica, the click count of `MouseDown(x, y, clickCount)` carries the button: 1 for a left click, 2 for a
double click, negative for the right button. The port's handlers get `(x, y, button, clickCount)` and map them.
The text writers are Widget virtuals taking a `Graphics*`, as in 2004; the port uses `WriteString2004`,
`WriteWordWrapped2004` and `GetWordWrappedHeight2004` from `framework/compat2004`.

## ButtonListener: four slots

The 2003 `ButtonListener` has 0 `ButtonPress(id, clickCount)`, which calls slot 1, 1 `ButtonPress(id)`,
2 `ButtonDepress(id)` and 3 `ButtonDownTick(id)`; it has no mouse enter and leave slots and no destructor slot.
`SexyApp` derives from it (its slots 0-3), so the application is the listener of every button, as in Noah's Ark
and Atomica.

## ButtonWidget and Dialog

`ButtonWidget` adds two slots to the widget's 43: 43 `SetFont` and 44 `IsButtonDown`. `Dialog` adds five: 43
`SetButtonFont`, 44 `SetHeaderFont`, 45 `SetLinesFont`, 46 `GetPreferredHeight` and 47 `IsModal`, the 2004 Dialog's
slots 47-51. Its ButtonListener forwards the ids `mId + 2000` (yes, OK) and `mId + 3000` (no, cancel) to the app,
which is what `Dialog2004` in `framework/compat2004` does.

## SexyApp: one class of 61 slots

There is no SexyAppBase. `SexyApp` derives from `ButtonListener` and has 61 slots. The game's `FossilizedApp`
(vtable 0x4fa410) overrides 1 `ButtonPress`, 2 `ButtonDepress`, 5 (shutdown), 8 (the cursor), 11 (the shutdown hook,
empty in the base), 12 (the destructor), 14 `Init`, 15 (empty in the base: the game's loading), 16 and 17 (the
registry reads and writes), 18, 22 and 23 (focus lost and gained), 24 `OpenURL`, 56 and 57. The port's
`DynomiteApp` derives from `SexyAppBase` and reaches the same behaviour through its members, as Atomica's does.
