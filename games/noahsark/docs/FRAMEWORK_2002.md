# The 2002 SexyApp framework (Noah's Ark Deluxe) and how the port maps it

Noah's Ark Deluxe (WinNoah.exe 1.1, linked 2002-10-17 with Visual C++ 6.0) is older than every other game in this
repository. The framework inside it is the first generation of PopCap's SexyApp, before it was split into the
`SexyAppBase`, `WidgetManager` and `WidgetContainer` classes that `docs/FRAMEWORK_2004.md` describes. This note
records where it differs and what the port does about each difference. The port's own framework (`framework/`,
SexyAppBase 1.3 API) needed no change for it; every difference is handled in `games/noahsark/src` or by what
`framework/compat2004` already had.

## What the binary has

One class, `SexyApp`, holds the whole application: the window and main loop, the widget list, the dialogs, the
registry settings, the sound and music calls, the resource loading. It has 54 virtual slots and derives from
`ButtonListener`, which has 4 slots, so the application object is also the listener of every button. The port's
`NoahApp` derives from `SexyAppBase` and reaches the same behaviour through the framework's own members.

`Widget` has 44 slots and is 0x4c bytes. Widgets are only ever top-level: the app keeps one list of them and there are no
children, so the board, the title screen, the level-up panel and the high-score screen are siblings that the game
adds and removes itself. A dialog is a widget too, and so is the modal one; `Dialog` forwards the ids of its buttons
(its own id plus 2000 for the first button, plus 3000 for the second) to the app, which is what
`framework/compat2004`'s `Dialog2004` does, so the game's `ButtonDepress` handles dialog answers and sidebar buttons
in one switch.

`ButtonListener` has four slots, of which the game uses `ButtonPress(id)` (the 2004 version also gets a click
count), `ButtonDepress(id)` and `ButtonDownTick(id)`. The port overrides the same ones.

`Graphics` has no virtual methods and draws images, rectangles, text and clipped regions; the port's `Graphics`
offers the same calls. Strings are MSVC 6's `std::string`: reference counted with the count in the byte before the
data. The decompiler shows them as inline copies and releases; `games/noahsark/tools/decread.py` collapses those
idioms when reading a function.

## Assets and data

Images are GIF, JPG and PNG. An image's alpha is a second file with a leading underscore (`_name.gif`), which the
framework's image loader already pairs. Fonts are `data/*.txt` ImageFont descriptors with the 2002 syntax; the
framework reads them as they are. Sounds are Ogg Vorbis, with one 8-bit `.au` for the level-up jingle; music is four
Ogg files that the original plays through FMOD (the retail folder ships `fmod.dll`) and the port plays through the
platform's mixer (Tremor), which also decodes these early Vorbis files.

The game's texts are not in files: they sit in the exe's string table. `GameText` (`src/GameText.h`) reads them from the
player's own `WinNoah.exe` by address at run time (the same approach as BookWorm's), with a length and hash check, so
no text of the game is in the repository.

## Random numbers and time

The app seeds the C library generator with the tick count, and the game draws from it with `rand()`. The port uses
`CrtRand` and `CrtSRand` of `compat2004`, so `-seed=N` repeats a game, and `GameTickCount` replaces `GetTickCount`
so that `-fast` test runs stay deterministic.

## Saves

The original's save format is "version 2" (the board, the pieces, the app's counters). The port writes its own
`savegame.dat` in the user folder with the same field order, and `hiscores.dat` in the original layout with the
original's checksum. The registry values (options, the registered flag, the name of the last score) go to
`registry.ini`, as in the other ports.
