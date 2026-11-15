# Shared engines

Sibling games that run on the same engine keep one copy of it, under `engines/<library>/`, never one copy a game (the
user, 2026-11-11: "Share engines"): Retro64 for Cosmic Bugs and Water Bugs, the Mahjong Escapes' engine, Bookworm
Adventures' libraries (PopLua, PopAnim, jefflib, its Lua app and SWF). Where two games' versions of a function behave
differently, each game sets which at start (a flag the engine reads, which the game's own code sets); where they do not,
there is one function.

## What a shared engine is

- A folder `engines/<name>/` with its sources and a `CMakeLists.txt` that makes a static library `<name>`. The top
  `CMakeLists.txt` adds the folder when one of its games is built, and each game's `CMakeLists.txt` links the library
  and names it, so `tools/check.sh --changed` finds the games to test (`engine_games`) and `tools/objsnap.py` the
  objects to compare.
- A change to it is a shared change (`CLAUDE.md`, "Sessions, ownership and git"): `tools/check.sh --changed` runs the
  suites of every game built on it and the web build of each, and every one's frozen baseline stays unchanged. A game's
  session sends the coordinator a spec for one, or a `shared:` commit.
- A game's own code calls it and sets its own switches. Nothing in the engine knows the games by name.

## The address tag

Every function's comment gives its address in each game's exe, with the game's folder name after it, in the games'
alphabetical order:

```
// 0x40c2e0 (cosmicbugs), 0x40c4d0 (waterbugs): COLOR_256 and COLOR_64 alike, without scaling
// The FLIC reader 0x40c1a0..0x40c890 (cosmicbugs), 0x40c390..0x40ca80 (waterbugs)
```

A function that only one game's exe has gives that address alone (`Image_SetMask`, 0x4235b0 (cosmicbugs)); code that is
the port's own says `port:` as everywhere. `tools/namecheck.py` (rule 6 and its `tag` check, with an `engine <name>` line
in the game's `namecheck.txt`) reads a game's own addresses and refuses an address with no tag, and `tools/coverage.py`
pairs a game's export with the addresses tagged with its name (`docs/COVERAGE.md`). Only a game folder's name is a tag.
Which address belongs to which game's exe comes from the two exes' function match (`re/waterbugs/match.jsonl` and
`addrmap.json`, in the main checkout), not from reading either one.

## Retro64 (`engines/retro64`, Cosmic Bugs and Water Bugs)

The two exes hold the same engine library (a game's own code starts at 0x401000 and ends at 0x4428b0 in Cosmic Bugs and
0x442cf0 in Water Bugs, the library in between), so the copy is the union of what each port translated: Water Bugs'
readable version (its readability pass, `games/waterbugs/docs/GAME_NOTES.md`) with what Cosmic Bugs' copy had and Water
Bugs' had left out (Water Bugs' unneeded-code pass had taken it out of its copy). That code is now in every build and
costs Water Bugs nothing at run time: its frozen baseline is unchanged. The list below says which of it any game's code
reaches.

Where a function does something different for a game, a switch the game's own code sets chooses:

| Difference | Switch | Who sets it |
|---|---|---|
| A font draws its glyphs clipped (through `mClip`) or unclipped | `Font_SetClipped` (`Font::mClipped`, +0x408) | the game's font loader, its screen redraw and `TextInput_Draw`, in the original (Water Bugs sets 1 where the original does; Cosmic Bugs' code sets it too) |
| A text input draws on the screen or on a surface of its own over a background | the surface and background given to `TextInput_Init` | the game; neither game gives one |
| The fixes mod's hooks | `Sys_FixOn` (`SysHost::FixOn`, false by default) | the host, for the games that have the mod |

`SysHost` has one more virtual than Cosmic Bugs' copy did (`FixOn`, for the fixes mod), which moves the vtable of that
game's host.

`R64ScreenDiff.h` and `R64ScreenDiff.cpp` are not the original's code. They are the hosts' helper for the shared
SurfaceDisplay (`framework/SurfaceDisplay.h`). `r64::ScreenDiff::Update(screen, dest32)` widens what changed in the
engine's 16-bit screen since the last call into the 32-bit frame; the first call widens all of it, and `Whole()` says
so. `Rects()` then gives the changed blocks as `r64::ChangedRect` for `SetFrame`. Cosmic Bugs' host uses it (merged
2026-11-12). Water Bugs' widget still has its own copy, `CollectChangedRects`, until it moves onto this one.

### What only Cosmic Bugs' copy had, taken out by its unneeded-code pass (2026-11-11)

The union had kept every function either port translated; Cosmic Bugs' pass (the commit `shared: Retro64: the engine code
no game reaches is out`) took out what neither game's code, tests or mods call, and `tools/check.sh --changed main` ran both
games' frozen baselines on it, unchanged. Out: the fonts' unclipped drawing, `Font_SetClipped` and a font's texture and mask
target (both games set the clip flag on every font they draw; nothing sets the texture), with what only those branches
called (`Image_BlitUnclipped`, `Image_BlitAlphaFadeUnclipped`, `Image_AddMaskUnclipped`, `Image_AddMask`, `Image_SetMask`,
`Image_FillAlphaRect`, `AddMask8`, `Rect_Overlap`, `Rect_Intersect`); the text input's own surface and background
(`DrawOwn`; `TextInput_Init` lost its two parameters, which both games passed as `nullptr`); and `Bank_Reconvert`,
`Xml_Reset`, `SetScreenLostCallback`, `Sys_Host` and `DrawList_Count`. The fields that went keep their place in the
layout as named padding (`mClippedUnused`, `mTextureUnused`, `mSurfaceUnused`, `mWasActiveUnused`, `mRedrawUnused`), so
the layout asserts and the games' `Text` and `FontEntry` sizes hold. Each game's `coverage.txt` names the two reached
functions that have no counterpart now (`Font_SetClipped`, `SetScreenLostCallback`).

Stay, as the coordinator decided: `Sys_Sleep` (called by Cosmic Bugs' start code) and `Key_Push` (the name entry's on-screen keys,
in both games). `gPumpMessages`, the flag that kept `PumpMessages` going while the window was not the active one (only Cosmic Bugs'
demo code set it, and that is gone), went out in Cosmic Bugs' readability pass: `PumpMessages` returns at once while the window
is not the active one, as it always did. `Sys_FixOn` is the other way
round: only Water Bugs' host calls it. The pixel-format blitters and other functions the runs did not reach stay until
Water Bugs' coverage check agrees.

## The Mahjong Escapes (`engines/jollybear` and `engines/mahjongescape`, Ancient Japan and Ancient China)

The two exes are one game on JollyBear's engine, so both the engine and the game's code are shared. `engines/jollybear`
is the engine, one library as Retro64 is. `engines/mahjongescape/src` is the game's code (the boards and layouts, the
tiles' rules, the bot, the dialogs, the map, the play layer, the host app), in namespace `mahjong`, and
`engines/mahjongescape/mahjongescape.cmake` builds it once for each game: each game's `CMakeLists.txt` calls
`mahjongescape_game(<game> ...)`, with the game's own `src/` first on the include path. Each game's `src/` holds only
what is its own: `Edition.h` (names as macros; counts, places, sizes and tables as constants; and switches for what only
one edition has, such as `EDITION_DRAGON_CORNERS` or `EDITION_RESTART_BOX`), `Res.h`, `GameTextTable.inc`, and the
functions the two exes do differently (`LevelDone`, the panel's `Draw`). So here the game sets which version runs at
build time, through its `Edition.h`, not with a flag at start. Each game's folder keeps its data, docs, tools, web page,
mods and settings files.

The address tags pair the two exes by China's function match (`re/mahjongchina/compare/addrmap.tsv`, in the main
checkout). A function only Ancient Japan's exe has (the restart box, inlined helpers, vtables) gives Japan's address
alone.

## The SWF player (`engines/swf`, library `sexyswf`)

The originals that show Flash movies play them through the Flash ActiveX control, which no port can use, so the ports
share a player of their own, `Sexy::SwfMovie`, written from the public SWF file format specification and translated
from no exe: it has no addresses, and neither `tools/namecheck.py` nor `tools/coverage.py` reads it. Bookworm
Adventures wrote it, Volume 2 added bitmaps and frame labels to its copy, and that copy is the one here (2026-11-12).
The ports whose originals show Flash movies link it (both Bookworm Adventures games; Heavy Weapon and Hammer Heads as
they come). Each keeps its own widget over it, standing in for the original's, and its own `swftest` tool
(`games/<game>/tools/swftest`), which plays every movie of that game through headless. What a movie holds that the
player skips is counted (`SwfMovie::GetUnsupported`), so a game's `swftest` shows at once what more of the format its
movies need; the player grows to cover it, and every other game's movies must play as before: their `swftest` runs
unchanged and frames rendered before and after the change identical.

## Adding another

Move the folder to `engines/<name>/` with `git mv`, make one library of it, give each function's comment every game's
address in the tag form, add the folder's name to each game's `CMakeLists.txt`, `src engines/<name>` to its
`coverage.txt` and `engine <name>` to its `namecheck.txt`, and prove each game unchanged: its frozen baseline
(`tools/check.sh <game> --inert`), the web build (`tools/check.sh build-web:<game>`) and the object code
(`tools/objsnap.py`, which also reads the engine's objects).
