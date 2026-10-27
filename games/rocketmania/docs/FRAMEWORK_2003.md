# The 2003 SexyApp framework in Rocket Mania Deluxe, and how the port maps it

Rocket Mania Deluxe (the game inside PopCap's 2006 wrapper, linked 2003-09-08 with Visual C++ 6.0) is built on the
SexyApp framework of 2003, a generation after Noah's Ark (`games/noahsark/docs/FRAMEWORK_2002.md`) and a little after
Atomica (`games/atomica/docs/FRAMEWORK_2003.md`). What those two notes say about flat widgets, a non-virtual
`Graphics`, the application as the listener of every button, MSVC 6's `std::string` and the dialog ids holds here too;
this note records what is different. Slot numbers are positions in the vtable (offset = 4 x slot); the names are the
port's (the 2004 framework's) names for what the slot does, found by reading the base-class bodies, and a slot not
named here has not been needed yet.

## Class sizes

The base classes of this build, from the RTTI scan (`re/rocketmania/rtti.classes.json`):

| Class | Slots | Notes |
|---|---|---|
| `ButtonListener` | 4 | destructor, `ButtonPress(id)`, `ButtonDepress(id)`, `ButtonDownTick(id)` |
| `Widget` | 44 | the same count as Noah's Ark's, no children |
| `Dialog` | 49 | `Widget` plus five (the same count as Noah's Ark's `Dialog`) |
| `SexyApp` | 62 | Noah's Ark has 54 and Atomica 55: eight more virtual hooks |
| `WinTubeTricksApp` | 63 | the game's application class (the working title was Tube Tricks) adds one |

`SexyApp` derives from `ButtonListener`, so slots 0 to 3 of the application are the listener's (slot 0 is its destructor and
the application's own destructor is slot 12). The game's application class overrides slots 1 and 2 (`ButtonPress`,
`ButtonDepress`), 11, 12 (the destructor), 14 (`Init`), 15 (`LoadingThreadProc`), 16, 17, 18 (`NewDialog`), 31, 55 (`KillDialog`),
57 to 60 (four hooks that pause the board, 57 and 60 passing true and 58 and 59 false) and 61, and adds slot 62.

## Application slots the game calls

| Slot | Offset | Port | What the base body does |
|---|---|---|---|
| 18 | 0x48 | `NewDialog(id, modal, header, lines, footer, buttonMode)` | the game overrides it for its own dialog classes |
| 19 | 0x4c | `ShowError(text)` | a message box titled FATAL ERROR |
| 20 | 0x50 | `SafeDeleteWidget(widget)` | queues the widget on the app's delete list (+0x424) |
| 38 | 0x98 | `PlaySample(id)` | `SoundManager::GetSoundInstance(id)->Play(false, true)` through the manager at +0x418 |
| 52 | 0xd0 | `DoDialog(id, ...)` | kills any dialog with that id, `NewDialog`, then `AddDialog` |
| 53 | 0xd4 | `GetDialog(id)` | looks the id up in the dialog map at +0x2ac |
| 54 | 0xd8 | `AddDialog(id, dialog)` | |
| 55 | 0xdc | `KillDialog(id)` | removes the dialog from the widget manager and the map |
| 56 | 0xe0 | `GetDialogCount()` | the map's size at +0x2b8 |

## Widget slots

The 44 slots of this `Widget`, as far as read: 0 destructor, 1 and 2 the manager hooks (`AddedToManager` stores the
manager at +0x24, `RemovedFromManager` clears it), 3 an empty hook, 4 an empty hook, 5 and 6 `SetColors` (four-int and
three-int tables), 7 `SetColor(i, color)`, 8 `GetColor(i)`, 9 `SetVisible`, 10 `SetDisabled` (it also clears the pointer-over state), 12 and 13 `Resize` (rect and x, y, w, h),
14 `Move`, 15 `WantsFocus` (returns false), 16 `Draw(g)`, 17 `Update` (the base counts at +0x4c), 31 to 37 the
text-writing helpers (`WriteCenteredLine`, `WriteString`, `WriteWordWrapped`, `GetWordWrappedHeight`, `GetNumDigits`,
`WriteNumberFromStrip`), 38 `Contains(x, y)`, 42 and 43 `MarkDirty` and `MarkDirtyFull`. The mouse, key and focus
handlers sit between 18 and 30; the board overrides 24 to 27 and 29, the buttons add two slots after 43. They are named
as the board and the dialogs are read (`GAME_NOTES.md`).

## Resources

The image, sound and music names are not in an XML: one table of 183 pointers to file names at 0x4e6a80 serves
all three kinds. Entries 0 to 113 are image names (loaded with `images\` in front), 114 to 178 sound names (`sounds\`),
179 to 182 the music files. `games/rocketmania/tools/ptrtable.py 4e6a80` prints it. The port's `Res.cpp` holds the same
three lists of names (file names are not game text), indexed by the original's ids.

The music is not a single file per piece: the in-game and the menu pieces are made of segments A to I (menu A to C),
joined at run time according to a `PLAYLIST.txt` with in and out times by the game's `MySound` class (0x4299b0), and the
original caches the joined result as a `cached_*.wav` next to the pieces. The port does not need the caches.

## Texts

Every text of the game is a string in the exe; the port reads them from the player's own `RocketMania.exe` (the wrapper,
with the game found inside it) by address, as Pixelus's `GameText` does, and no text is in the repository.
