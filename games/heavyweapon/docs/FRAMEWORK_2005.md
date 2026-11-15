# The early-2005 SexyApp framework in Heavy Weapon Deluxe, and how the port maps it

Heavy Weapon Deluxe (linked in 2005 with Visual C++ 7) is built on a SexyApp framework between BookWorm's of 2004
(`docs/FRAMEWORK_2004.md`) and the public 1.2 (`games/bigmoney/docs/FRAMEWORK_12.md`). Its widgets are still the 2004
ones, flat and childless, with one new virtual: `UpdateF`. Its application class is already split into `SexyAppBase`
and `SexyApp`, as in 1.2, with fewer hooks than 1.2's. This note records only what differs from those two notes. Slot
numbers are positions in a vtable (offset = 4 x slot). The names are the port's names for what the slot does, found by
matching the slot bodies against BookWorm's and Big Money's (`re/heavyweapon/slots.txt`) and by reading them. A slot
that is not named here has not been needed yet.

## Class sizes

From the RTTI scan (`re/heavyweapon/rtti.classes.json`), against the same classes in BookWorm (2004) and Big Money (1.2):

| Class | Slots | BookWorm | Big Money | Notes |
|---|---|---|---|---|
| `ButtonListener` | 7 | 6 | 7 | `ButtonMouseMove` is back (below) |
| `Widget` | 48 | 47 | 54 | the 2004 vtable plus `UpdateF` at slot 18; sizeof 0x58, as in 2004 |
| `ButtonWidget` | 50 | 49 | 57 | the Widget's one slot more |
| `Dialog` | 54 | 52 | 60 | sizeof 0x114 (2004: 0x110): a dialog's second interface sits at +0x114 |
| `FlashWidget` | 49 | none | none | the Widget's slots plus one (slot 48, 0x461780) |
| `SexyAppBase` | 80 | none | 90 | absent in 2004, where `SexyApp` is the only class |
| `SexyApp` | 86 | 67 | 96 | slots 0 to 6 are the `ButtonListener`'s |
| `BassMusicInterface` | 16 | 15 | 20 | |
| `WidgetManager` | 1 | 1 | 15 | as in 2004: only a virtual destructor, one flat list of widgets |

A game widget's own fields start at +0x58, where 2004's do. A widget with a listener has the listener's vtable at
+0x58 and its own fields after it: Board, Armory, TitleScreen (a `ButtonListener` at +0x58 and a `FlashListener` at
+0x5c) and CutsceneScreen (a `FlashListener` at +0x58). Every dialog of the game has a `ButtonListener` at +0x58, the
Dialog's own, and keeps its second interface at +0x114: the options dialog's `SliderListener`, the new-player box's
`EditListener` and the player list's `ListListener`.

## Widget slots

Slots 0 to 17 are BookWorm's (`docs/FRAMEWORK_2004.md`, section 3.1.1): 1 `AddedToManager`, 2 `RemovedFromManager`,
13 `Resize(x, y, w, h)`, 16 `Draw`, 17 `Update`. Slot 18 is new: `UpdateF(float theFrac)`, empty in the base (the
framework's empty stub 0x471be0). From slot 19 on, every slot is BookWorm's slot one higher:

| Slot | Offset | Port | BookWorm's slot |
|---|---|---|---|
| 18 | 0x48 | `UpdateF(float)` | none |
| 19, 20 | 0x4c, 0x50 | `GotFocus()`, `LostFocus()` | 18, 19 |
| 21 | 0x54 | `KeyChar(char)` | 20 |
| 22, 23 | 0x58, 0x5c | `KeyDown(KeyCode)`, `KeyUp(KeyCode)` | 21, 22 |
| 24, 25, 26 | 0x60 to 0x68 | `MouseEnter()`, `MouseLeave()`, `MouseMove(x, y)` | 23 to 25 |
| 27, 28 | 0x6c, 0x70 | `MouseDown(x, y, btn, cc)`, `MouseDown(x, y, cc)` | 26, 27 |
| 29, 30, 31 | 0x74 to 0x7c | `MouseUp(x, y, btn, cc)`, `MouseUp(x, y, cc)`, `MouseUp(x, y)` | 28 to 30 |
| 32, 33 | 0x80, 0x84 | `MouseDrag(x, y)`, `MouseWheel(delta)` | 31, 32 |
| 37 | 0x94 | `WriteString(g, text, x, y, w, just, draw, off, len)` | 36 |
| 38 | 0x98 | `WriteWordWrapped(g, rect, text, spacing, just)` | 37 |
| 46, 47 | 0xb8, 0xbc | `MarkDirtyFull()`, `MarkDirty()` | 45, 46 |

The text writers at slots 37 and 38 are the 2004 bodies, so the port calls `WriteString2004` and `WriteWordWrapped2004`
(`framework/compat2004/Compat2004.h`) wherever the original calls these slots, with the justification the original
pushes (the 2004 default is -1, left). The Widget's fields are the 2004 ones: `mX` +0x08, `mY` +0x0c, `mWidth` +0x10,
`mHeight` +0x14, `mVisible` +0x19, `mWidgetManager` +0x24, `mUpdateCnt` +0x4c.

`UpdateF` is the reason for the new slot. The board (0x419e30) and every enemy craft (0x4376f0 and 0x435350, slot 3 of
their own vtables) move by the fraction of an update that a drawn frame shows, so motion is smooth when frames come
between updates. The port's `Widget` has `UpdateF` too, and the framework calls it as the original's `DoUpdateFramesF`
(the application's slot 10) does.

## ButtonListener slots

| Slot | Port | Notes |
|---|---|---|
| 0, 1 | `ButtonMouseEnter(id)`, `ButtonMouseLeave(id)` | as 2004 |
| 2 | `ButtonMouseMove(id, x, y)` | not in 2004; the armory's weapon panel overrides it |
| 3 | `ButtonPress(id, clickCount)` | the base body (0x40b970) calls slot 4 |
| 4 | `ButtonPress(id)` | |
| 5 | `ButtonDepress(id)` | |
| 6 | `ButtonDownTick(id)` | |

As in 2004, `SexyApp` is a `ButtonListener` (its slots 0 to 6). The game's application class overrides 4 and 5.

## Application slots

The game's class overrides these slots of its 86 (vtable 0x52a570):

| Slot | Offset | Port | Game's body |
|---|---|---|---|
| 4, 5 | 0x10, 0x14 | `ButtonPress(id)`, `ButtonDepress(id)` | 0x401080, 0x40e4f0 |
| 8 | 0x20 | `UpdateFrames()` | 0x406ab0 |
| 16 | 0x40 | the destructor | 0x40cff0 (body 0x40b980) |
| 18 | 0x48 | `Init()` | 0x40d2c0 |
| 20 | 0x50 | `LoadingThreadProc()` | 0x4041b0 |
| 38 | 0x98 | `Shutdown()` | 0x406b20 |
| 42 | 0xa8 | `Start()` | |
| 46 | 0xb8 | `PlaySample(id, pan)` | 0x401150 |
| 59, 60 | 0xec, 0xf0 | `SwitchScreenMode(windowed, is3d, force)`, `SwitchScreenMode(windowed)` | 0x401740, 0x4016f0 |
| 70, 71 | 0x118, 0x11c | `GotFocus()`, `LostFocus()` | 0x401040, 0x401020 |

The other slots match Big Money's in name and order, without 1.2's extra hooks, which come in after them: slot 9
`DoUpdateFrames`, 10 `DoUpdateFramesF`, 11 `MakeWindow`, 17 `CreateMusicInterface`, 21 and 22 `WriteToRegistry` and
`ReadFromRegistry`, 27 `MsgBox`, 33 `OpenURL`, 47 `PlaySample(id)`, 50 `GetMusicVolume`, 54 `SetMusicVolume`, 55 and 56
`Mute` and `Unmute`, 58 `GetImage`, 62 `DoDialog`, 63 `GetDialog`, 65 `KillDialog`, 77 `DrawDirtyStuff`. Slots 82 to 85
are the registration and update pages (`OpenRegisterPage`, `OpenHTMLTemplate`, `OpenUpdateURL`), which the port leaves
out with the rest of the registration code.

## Application fields the game uses

Absolute offsets in the original's object. The port's base classes have other sizes, and nothing depends on them:

| Offset | Port | Evidence |
|---|---|---|
| +0x24 | `mProdName` | the constructor stores "HeavyWeapon" |
| +0x40 | `mTitle` | the constructor: the game's name and the version |
| +0x5c | `mRegKey` | the constructor: "PopCap\\" and the product name |
| +0x90, +0x98 | `mMusicVolume`, `mSfxVolume` (doubles) | the constructor stores 0.3 and 0.9 |
| +0x2fe | `mIsWindowed` | `SwitchScreenMode(windowed)` sets the options dialog's checkbox from it |
| +0x330 | `mMusicInterface` | `GotFocus` and `LostFocus` set its volume |
| +0x354 | `mProductVersion` | the constructor stores "1.0" |
| +0x414 | `mUpdateCount` | the tank's shield and the armory's orbs animate by it |
| +0x438 | `mSoundManager` | `GotFocus` and `LostFocus` set its volume |
| +0x484, +0x547, +0x548 | `mCustomCursorsEnabled`, `mAutoEnable3D`, `mTest3D` | the constructor sets all three |

The game's own fields start at +0x688 (`HeavyWeaponApp.h`).

## FlashWidget

The framework's `FlashWidget` (0x461780 to 0x469200, 0xbc bytes, vtable 0x5327dc) hosts the Flash ActiveX control
and draws it into an image of the widget's size. A `FlashSink` (8 slots) receives the control's events, and a
`FlashListener` hears from the widget: the movie ended, or the movie sent an FSCommand. Heavy Weapon plays its three
cutscenes in it. The port keeps the widget and its listener and plays the movie with the shared SWF player
(`engines/swf`): one movie frame every 100 / frame-rate updates, so a test run shows the same frames every time.
`FlashWidget.h` gives the fields the game reads.

The original's state (+0x58), id (+0x5c), listener (+0x60), pause count (+0x84) and the pause-while-inactive flags
(+0x88, +0x94) are kept with the same meaning. The control, its sink and its window are left out.
