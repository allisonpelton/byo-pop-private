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

The high-score file is five records of `[u16 length][name][i32 score][i32 check]`, the check being the score xor
0x42beef xor each name byte (a signed char) times 0x11. The loader (0x411ea0) keeps a record whose check does not match
but stores its score as `score % 256`, and reads on; a mod set that changes play gets the file name's score key
(`hiscores-<key>.dat`, `savegame-<key>.dat`).

## Dialogs

`Dialog` (0x428360 to 0x4290e0) is the 2002 class that the port's `Dialog2004` stands in for: a header, a block of
word-wrapped lines, then either yes/no buttons or one footer button (button mode 3), inside content insets. Its preferred
height (`GetPreferredHeight`, slot 47, 0x428580) is the two insets, plus the header font's height, plus the wrapped
lines' height when there are lines (10 below the header), plus a row of yes/no buttons unless the mode is the footer
one, plus the footer button's font height with 8 above and 8 below. The dialogs with a fixed size do not use it:

The dialogs that have a fixed size override slot 47 with a function that only returns a constant, and Ghidra does not
make functions of those (two instructions, `mov eax, N; ret 4`), so they are missing from the decompilation: found by
reading the vtables (find a dialog class's vtable from the address of one of its functions, then slot 47 at offset
0xbc). The sizes, and the calls that place the dialogs (the app's `Show...` functions, each ending in a virtual
`Resize(x, y, w, h)` with y centred as `(480 - h) / 2` where the table says so):

| Dialog | Slot 47 | Height | Placed by |
|---|---|---|---|
| Options | 0x4209f0 | 0x1b6 (438) | 0x418c40: Resize(0xd5, centred, 0x17c, h) |
| Tutorial | 0x4209f0 (the same) | 0x1b6 | 0x418d70: Resize(0xd5, centred, 0x17c, h) |
| Difficulty | 0x411130 | 0x140 (320) | |
| High scores | 0x411730 | 0x12c (300) | 0x41ae80: Resize(0xf5, 0x5a, 0x12c, h) |
| Record a score | 0x411730 (the same) | 0x12c | 0x41add0: Resize(0x96, centred, 0x154, h) |
| Register | 0x41e570 | 0x160 (352) | |

The decompiler hides what happens between two calls of one slot: `h = GetPreferredHeight(w)` is followed by a second
call of the same slot with the width as its argument, to centre the dialog, and the width of the dialog is pushed for the
`Resize` before that call, so the width it passes to `Resize` can differ from the one it passes to the height
function (the record-score dialog asks for the height with 0x13c and is 0x154 wide). Disassemble the creator
(`llvm-objdump -d --x86-asm-syntax=intel --start-address=...` on the exe reads PE files directly) when a decompiled
`Resize` call has lost its arguments.

## Reading this compiler's output

The decompiler shows Visual C++ 6.0 code with idioms that are easy to translate wrongly (the review of the port
against the decompilation found the first three as bugs):

A loop that starts its counter at -1 and tests `limit < i` before incrementing runs one iteration more than the `for`
loop with the same bound: the abandon penalty's partner search (0x4067e0) covers cells 0 to 80, not 0 to 79.

`x & 0x800000ff` followed by `(v - 1 | 0xffffff00) + 1` for negative v is C's signed `x % 256`, not `x & 0xff`.

Moving a piece or an animal between two cells swaps *every* field the original copies, including the pixel position
(`mPixelX`/`mPixelY`, +0x10/+0x14); the cell that is vacated gets the other cell's old position cut to whole pixels
(`(float)(int)`, the two `__ftol` calls). An animal that is flying to the ark keeps flying when the board rises or the
columns close in only because its pixel position moves with it.

`(end - begin) & 0xfffffffc == 0` on a vector of 4-byte elements means "empty": a guard that reads "fewer than 2" in a
translation is wrong. Strings and vectors are MSVC 6's, shown as inline copies; `decread.py` collapses them.

A comment about what the original does is a claim: check it against the decompilation before writing it (the port had a
comment that said five rows where the loop fills four, and one that said a bad high-score check ends the load).
