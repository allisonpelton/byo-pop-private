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
| `PumpMessages` goes on while the window is not the active one | `gPumpMessages` (0 by default) | Cosmic Bugs' demo code |
| The fixes mod's hooks | `Sys_FixOn` (`SysHost::FixOn`, false by default) | the host, for the games that have the mod |

`SysHost` has one more virtual than Cosmic Bugs' copy did (`FixOn`, for the fixes mod), which moves the vtable of that
game's host.

### What only Cosmic Bugs' copy had, for its removal pass

The union keeps every function either port had translated, so Cosmic Bugs' later unneeded-code pass finds here what it
may take out. Checked against both games' `src`, `test` and `mods` (2026-11-11):

- Called by Cosmic Bugs' game code and by no Water Bugs code: `Sys_Sleep` (`CBStart.cpp`) and `gPumpMessages` (set in its
  demo code, `CBPowerup.cpp`). These stay for as long as that code does. `Sys_FixOn` is the other way round: only Water
  Bugs' host calls it.
- Called by no game's code in either port, so the shared copy holds them only because Cosmic Bugs' translation did:
  - the fonts' unclipped drawing, the branches taken when a font's `mClipped` is 0 (both games set 1 on every font they
    draw), and a font's texture and mask target (`mTexture`, `mTextureX`, `mTextureY`, `mMaskTarget`, which nothing sets),
    with what only those branches call: `Image_BlitUnclipped`, `Image_BlitAlphaFadeUnclipped`, `Image_AddMaskUnclipped`,
    `Image_AddMask`, `Image_SetMask`, `Image_FillAlphaRect`, `AddMask8`, `Rect_Overlap` and `Rect_Intersect`;
  - the text input's own surface and background (`DrawOwn`, `mSurface`, `mBackground`, the two parameters of
    `TextInput_Init`): both games pass `nullptr` for both;
  - `Bank_Reconvert`, `Xml_Reset`, `SetScreenLostCallback`, and the port's `Sys_Host` and `DrawList_Count`.

Cosmic Bugs' pass takes one of these out only after checking that none of its tests or its mods call it, and a function
that no game reaches comes out of the shared copy for both games at once (a shared change, `tools/check.sh --changed`).

## Adding another

Move the folder to `engines/<name>/` with `git mv`, make one library of it, give each function's comment every game's
address in the tag form, add the folder's name to each game's `CMakeLists.txt`, `src engines/<name>` to its
`coverage.txt` and `engine <name>` to its `namecheck.txt`, and prove each game unchanged: its frozen baseline
(`tools/check.sh <game> --inert`), the web build (`tools/check.sh build-web:<game>`) and the object code
(`tools/objsnap.py`, which also reads the engine's objects).
