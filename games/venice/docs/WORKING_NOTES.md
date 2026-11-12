# Venice Deluxe: working notes (analysis state, kept so a compaction loses nothing)

Venice Deluxe 1.0 (Retro64, published by PopCap, 2007-06-26). `re/venice/bin/VeniceGame.exe` is the game image out of its
wrapper: 1,978,368 bytes, linked 2007-06-06 with VS2003 (linker 7.10), debug-heap std containers (`std::_DebugHeapAllocator`),
image base 0x400000. Data: `main.pak` (862 files after staging: gfx, audio, levels, objects) and two loose files
(`config.xml`, `properties/partner.xml`), staged lower-case into `assets/venice/` by `tools/stage_venice_assets.py`.

## The export and its readers (re/venice is git-ignored)

- `tools/ghidra_import.sh` with `MSVC70=1` (noreturn discovery off), then `tools/rtti_msvc70.py` (RTTI classes, `rtti.jsonl`,
  `rtti.classes.json`), `tools/funcmatch.py` against Big Money and BookWorm (only about 7% of Venice's functions match: the
  compiler differs; `re/venice/match/`), the merged names applied with `tools/ghidra_headless.sh apply re/venice/names_v1.jsonl`.
- `games/venice/tools/decread.py <addr>...|<lo>-<hi>|--list` prints the compact decompilation; names from
  `games/venice/docs/symbols.txt` (`<addr> <name>`). `fn.py`, `callers.py <addr> --depth n`, `refs.py <addr>` (every operand or call
  that refers to an address), `classes.py` (vtables of the game's own classes), `pe.py` (the section table: .text raw 0x1000,
  .rdata raw 0x185000, .data raw 0x1b5000), `ghidra/DumpListing.java`.
- Ghidra leaves the 0x4a0000-0x504000 code (400 KB of the framework's software rasterizer and blitters, no strings, no calls from
  the game) without functions; it is the framework's, so the port needs none of it.

## Layout of the code

- 0x401000-0x43a900 the game (about 1,000 functions, 258 KB): 0x401000-0x424000 game objects and screens, 0x424000-0x43a000 the
  Retro64 engine (font, object pools, stages, filters, events, widgets, level editor), 0x43a900-0x43f000 STL instantiations,
  0x43fa60-0x442000 the app (VeniceApp ctor at 0x43fa60, loading state machine at 0x440930-0x440f30).
- 0x443000+ the SexyAppFramework 1.3 (RTTI names the Sexy classes), then libraries and the CRT.
- Framework vtables match Bejeweled's except one extra slot in SexyAppBase (slot 66, between SwitchScreenMode(bool) and
  SwitchScreenMode(bool,bool,bool)), SexyApp, DDImage, ImageFont, DescParser and FontData.
- VeniceApp: vtable 0x58ce70 (104 slots, SexyApp's 104), overrides slots 16 (LoadingThreadCompleted, 0x4402d0: makes the game
  widget), 17 (dtor 0x4434a0), 20 (ShutdownHook 0x43ffa0), 22 (LoadingThreadProc, empty), 49 (Init 0x440190), 82/83
  (GotFocus 0x440450 / LostFocus 0x440410), 92 (Redraw 0x440290). The game widget is a Widget plus ButtonListener at 0x4404f0
  (vtable 0x58d030, listener 0x58d010); its Draw is 0x440650.
- Loading state machine: DAT_005b51b8 = -2 (partner logo), -1, 0 (loading.jpg, loadbar.png), 1, 2..100 (a thread, 0x436cc0, loads),
  101+ the game.
- The data is a scene: `<object>` with `<stage ms loop_start loop_end visible>`, `<attribute type start end>`, `<filter type
  attribute amplitude period offset>` and `<event type action parm1..4 alt_key>`; the tags are not closed. The screens (main_menu,
  pause_menu, high_scores ...) are such scene files in `levels/`, as are the 80 tricky, 48 background, 43 relaxed and 24 bonus levels.
- The engine keeps its objects in static pools with a rotating free-slot search (0x425b10: 4000 objects of 0x148 bytes at
  0x6856a8; 0x425be0: 8000 stages of 0xf4; 0x425c60: 16000 of 0x34), so iteration order follows the pool.
- Plates: factory 0x4076b0 (name to class, 13 plate classes, vtables 0x586b70.. stride 0xa8, 41 slots); base object class 36 slots
  (first slot 0x41c010), the editor/level class at 0x58cbb0.

## State

(updated as the port proceeds)

## Engine facts read so far (the Object, 0x148 bytes; pool at 0x6856a8, in-use flags at 0x6857d8 + i * 0x148)

Reading tips: Ghidra drops trailing call arguments (check the disassembly, `ghidra/DumpListing.java` or `ExportRange`), and
`decread.py` keeps `local_8` assignments only in functions without an exception frame. `FUN_00444400()` / `FUN_00434f60()` are
list-iteration helpers (begin / next) the compiler did not inline.
- Object (reset 0x425ce0, ctor chain 0x425e70, dtor 0x425ee0): +0 vtable (36 slots, slot 0 = 0x41c010 scalar deleting dtor),
  +0x8 type/flag?, +0xc parent Object (0 for a root), +0x10 type id, +0x18 shadow flag, +0x1c/0x20/0x24/0x28 clip left/top/right/
  bottom, +0x2c name (char[]), +0x44 flag read by the draw (0 = draw), +0x48 "has started", +0x4c blit style, +0x50 current Stage,
  +0x54 first Stage (stages chain through +0xcc), +0x58 current attribute block, +0x5c last child inserted, +0x60 first child
  (children chain through +4, sorted by +0x104 z order), +0x68 Image* (the engine's own image record: +0x10c width, +0x110
  height, +0x128 frame rect table of 16 bytes, +0x134 reference count, +0x138 pixels), +0x6c particle image,
  +0xa0..0xcc "world" values copied from +0xd0.. and transformed by the parent (0x427df0): +0xa0 x scale, +0xa4 y scale, +0xa8 x,
  +0xac y, +0xb0 ?, +0xb4 rotation, +0xb8/+0xbc/+0xc0 colour, +0xc4 alpha scale, +0xc8 frame number; +0xd0..+0xfc the local
  values in the same order (+0xd8 x, +0xdc y, +0xe4 rotation), +0x104 z order, +0x10c..+0x118 source rect (l, t, r, b),
  +0x11c..+0x128 tiles left/right/up/down, +0x12c bilinear, +0x134 selectable, +0x138 snap, +0x13c/+0x13d flip x / y,
  +0x140 fullscreen only. Constructor from XML 0x426570 reads image, particle_image, name, z_order, shadow, bilinear, snap,
  flip_x, flip_y, clip_*, blit_style, fullscreen_only, tiles_*, selectable. The editor's save is 0x426890 (not reached in play).
- Text objects (0x424860 ctor from XML, 0x424960 build): a string of character Objects drawn from the engine's own bitmap fonts
  (font records at 0x424710, glyph extents from alpha columns 0x4243a0..0x424610, width 0x424660).
- Scene helpers: 0x428220 root of an Object, 0x428250 child by name, 0x4282b0 child by type, 0x428120 add child sorted by z.
