# Hammer Heads Deluxe: working notes (analysis state, kept so a compaction loses nothing)

Hammer Heads Deluxe (Nuclide Interactive, "Cubby Catch" while in development; published by PopCap, Steam copy 2018 of the
2006 build). `re/hammerheads/bin/HammerHeadsGame.exe` is the game image out of its wrapper: 1,703,936 bytes, linked 2006-05-17
with VS2003, image base 0x400000, .text 0x401000-0x550000, .rdata 0x551000, .data 0x590000. Data are loose files in the
install: `properties/resources.xml` (363 ids: 238 images, 111 sounds, 14 fonts, five groups Init, LoadingScreen, Gui, MainMenu,
Game), `boards/*.xml` (54 levels: `board001..027`, `board_tough001..027`, `board_marathon`), `boards/{classic,classictough,marathon}.csv`
(per-level rules), `gnomedata/0..24.txt` (the intro text of each level), `images`, `sounds` (Ogg, the music too), `fonts`.
They are staged into `assets/hammerheads/` by `tools/stage_hammerheads_assets.py`.

## The export and its readers (re/hammerheads is git-ignored)

- `tools/ghidra_import.sh` with `MSVC70=1` (as Venice, the other VS2003 build), then `tools/rtti_msvc70.py` (223 classes,
  `re/hammerheads/rtti.jsonl`), then `tools/funcmatch.py` against Big Money (1824 of 8529 functions match, all of them framework or
  library code, 556 carrying Big Money's names) and the merged names applied with `tools/ghidra_headless.sh apply
  re/hammerheads/names_v1.jsonl` (`GHIDRA_PROJECT=hammerheads_game GHIDRA_PROGRAM=HammerHeadsGame.exe EXPORT_DIR=re/hammerheads/ghidra`).
  The Ghidra project is `hammerheads_game` in E:/retools/projects. Ghidra's decompiler times out on 0x428fd0 (the generated Gui
  extraction function, 12.8 KB) and 0x52c07d (CRT).
- `games/hammerheads/tools/decread.py <addr>...|<lo>-<hi>|--list <lo>-<hi>` prints the compact decompilation: exception-frame
  bookkeeping and local declarations dropped, virtual calls annotated with the slot's name in the SexyApp, Widget and Dialog tables
  of `games/bigmoney/docs/FRAMEWORK_12.md` (the same 1.2 layout), resource globals (`DAT_00597c34`) named by their id from
  `docs/resglobals.txt`. Names for functions and globals go in `docs/symbols.txt` (`<address> <name>`). `fn.py`, `callers.py`,
  `refs.py`, `classes.py` as in Venice's tools; `resnames.py` makes `resglobals.txt` from the machine code of the extraction
  functions (357 of the file's 363 ids); `resgen.py` makes `src/Res.h` and `src/Res.cpp` from `resources.xml`.

## Findings

- **No Nuclide code in common with Rocket Mania or Pixelus** (funcmatch, 2026-11-12): 158 of Hammer Heads' functions match Rocket Mania's
  (10 KB, C runtime and tiny library code at 0x520000-0x540000), 3 match Pixelus' (CRT). Nothing under `engines/`.
- **Layout of the code:** 0x401000-0x468e10 is the game and its own libraries, 0x469000+ the framework (RTTI names the 1.2 classes)
  and the CRT. In the game's span: 0x402950 `Object`, 0x405550-0x419c70 the editor and its property widgets (not ported), 0x41a000-0x41d9c0 the
  beta-test login and the DRM client (not ported), 0x41e660-0x422ac0 `Application`, 0x424e40 the CSV rules reader, 0x426100-0x42d081
  generated resource code, 0x42d730 `Avatar`, 0x42f350 `Creature` (the base of every gnome), 0x431c70 `Cubby`, 0x434fc0 `Gameplay`, the gnomes
  0x435b70-0x440030, 0x440830 `Popper`, shop items, `Sticker`, floaters, HUD, then the screens and dialogs 0x44c170-0x462d00, and TinyXML
  0x463240-0x468490 (the game's own copy, with a reader for its length-prefixed chunk files).
- **The boards are not plain XML on disk:** each `boards/board*.xml` is a stream of chunks, a little-endian u16 length and that many bytes
  of text (the writer wrote in pieces). Concatenated they are TinyXML text with `<Level ResX ResY>`, a `<property_list>` (Grid X, Grid Y,
  Snap Grid) and `<object Class="class Sexy::Cubby" Name X Y Width Height Angle>` / `class Sexy::Sticker` elements with a `<choice name="Image"
  index=..>` property. Only these two object classes exist in the 54 files (504 Cubbys, 159 Stickers).

## State

Nothing is translated yet.
