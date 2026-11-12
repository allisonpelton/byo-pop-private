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

## Engine facts, second batch (stages, filters, events, loader, loop)

- Reading tips: `tools/dis.sh <lo> <hi>` (llvm-objdump, Intel syntax); the decompile drops a thiscall's `this` and stack
  arguments the callee does not name, so the slot-function calls `(**(fn**)(*obj + off))(...)` and wrappers like 0x428700
  need the disassembly when a call looks short. `decread.py` prints hex float constants as `0x..../*1200f*/` and the text of
  `&DAT_` strings. The exe uses SSE (movss, divss) in places and x87 elsewhere (float10 in the decompile).
- Containers: every list is {cursor, head, tail} (12 bytes, `CGlobalUtils`) with generic list code instantiated many times:
  begin = cursor := head (0x444400, returns head != 0), next = cursor := cursor->next (0x434f60 for objects, next at +4,
  prev at +8; 0x421980 and 0x42b100 variants with other link offsets), append = tail insert with cursor := new node,
  RemoveCurrent (0x435330 for objects: a node whose +0x130 pool flag is 0 is deleted through its virtual destructor, a
  pool node has its destructor run without free and is unlinked), cursor after a removal = the node's next (or head).
  An object's stage list is {+0x50 cursor = the CURRENT STAGE, +0x54 head, +0x58 tail}; its child list is {+0x5c cursor,
  +0x60 head, +0x64 tail}.
- Pools (never iterated in play): objects 0x148 bytes x 4000 (flag at +0x130), stages 0xf4 x 8000 (flag +0xf0), filters 0x34
  x 16000 (flag +0x24); the allocation search rotates and loops forever when full. A pool object is a plain base `Object`
  (vtable 0x58a9d8); classes with their own fields (plates etc.) are heap objects added with AddChild(obj).
- Object virtual slots (index: address base class): 0 dtor 0x41c010, 1 AddStage(ms, loopStart, loopEnd, visible, name) 0x4284b0
  (stage time unit is 4 ms: ms / 4.0 stored at +0xc0), 2 IsHot() 0x429360, 3 GoToStage(stage) 0x428920, 4 GoToStageByName 0x428970,
  5 Load(text, tag) 0x426570, 6 Save 0x426890, 7 SaveExtra 0x426db0, 8 SnapToGrid 0x426370, 9 SetAttribute(attr, start, end) 0x428610,
  10 LoadAttributeTag 0x4286a0, 11 AddFilter(type, attr, amplitude, period, offset) 0x428700, 12 LoadFilterTag 0x428810,
  13 AddEventA 0x4287d0, 14 AddEvent(trigger, action, p1..p4, altKey) 0x428790, 15 LoadEventTag 0x4288a0, 16 NewChild(obj or 0)
  0x428120 (parent := this, insert sorted by z; returns the new child), 17 AddChild at the end 0x4281e0, 18/19 emit particles
  0x429870/0x429b00, 20-22 no-ops (0x403500,0x403500,0x403510; 22 is called when a tag closes), 23 AddToAttribute(attr, delta)
  0x4260a0, 24 Move(dx, dy) 0x426110, 25 Rotate(d) 0x4262a0, 26 no-op 0x429d20, 27 Update() 0x426dc0 (returns false when
  dead), 28 UpdateChildren 0x426f10, 29 Draw 0x426f60, 30 DrawSelf 0x4270f0, 31 DrawHot 0x427760, 32 DrawChildren 0x4277d0,
  33 no-op 0x425ff0, 34 Rewind 0x426000, 35 Kill 0x429d30 (+0x14 := 1, +0x10 := 0).
- Attribute indices (0x42b4d0 names to numbers): 0 x_scale, 1 y_scale, 2 x, 3 y, 4 z, 5 rotation, 6 r, 7 g, 8 b, 9 alpha,
  10 frame_number, 11 unnamed. An Object holds two 12-float arrays: local values at +0xd0 and world values at +0xa0 (the
  local ones transformed by the parent, 0x427df0); defaults 1,1,0,0,0,0,1,1,1,1,0 (0x42aed0).
- Stage (0xf4): +4 start[12], +0x34 end[12], +0x64 flags[12], +0x94 attribute mask, +0x98/+0x9c/+0xa0 filter list, +0xa4 cursor of
  the events (+0xa8 head), +0xb0 visible, +0xb4 loop_start, +0xb8 loop_end, +0xbc frame, +0xc0 length, +0xc4 progress, +0xcc next,
  +0xd0 previous, +0xd4 owner, +0xd8 name (24 chars). Filter node (0x34): +0 prev, +4 next, +8 stage, +0xc type, +0x10
  attribute, +0x14 amplitude, +0x18 period, +0x1c offset, +0x20 once flag. Event node (0x11c): +0 next, +4 prev, +8 flag,
  +0xc stage, +0x10 trigger, +0x14 action, +0x18 alt key, +0x19/+0x59/+0x99/+0xd9 parm1..4 (64 chars).
- Filter types: 0 super_cos, 1 squared, 2 cubed, 3 square_root, 4 sined, 5 cosined, 6 decay_sined, 7 decay_cosined,
  8 loop_anim, 9 pingpong_anim, 10 hot_spot, 11 velocity (0x42a1c0). Triggers: 1 click, 2 click_anywhere, 3 (not_hot's
  neighbour), 4 not_hot, 5 start, 6 end, 7 less_than, 8 code, 9 interval. Actions (0x42bc80): 1 insert_level, 2 send_level,
  3 go_to_stage, 4 send_parent_to_stage, 5 next_stage, 7 move, 8 play_sound, 9 dim_previous, 0xa pause_previous, 0xb
  dim_previous_no_pause, 0xc brighten_previous, 0xd stop_music, 0xe fade_music, 0xf fade_previous_music, 0x10 make_sparkles,
  0x11 make_particles, 0x12 set_level_number, 0x13 new_user_name, 0x14 change_user_name, 0x15 delete_user, 0x16
  get_initial_user, 0x17 kill_all_previous, 0x18 kill_all, 0x19 enter_high_score, 0x1a clear_score, 0x1b reset_progress,
  0x1c change_image, 0x1d bump_tick_counter, 0x1e custom, 0x1f save_score, 0x20 set_mode, 0x21 bump_score, 0x22
  clear_button_state, 0x23 enable_3d_hardware. Event dispatch 0x422e20 / 0x422fd0.
- Scene loader 0x4381a0 (stack of objects, `object` tags make a class by the `type` attribute: text 0x424810, input_box
  0x42cdc0, slider 0x436ff0, checkbox 0x4376a0, user_list_box 0x43a720, layer 0x40c0f0, cursor 0x4219d0, user_data_display
  0x4364d0, emitter 0x421ca0, tooltip 0x437b20, otherwise the plate factory 0x4076b0 and then a plain pool Object 0x425b10);
  tags stage/emitter_stage/attribute/filter/event/load_object_from_file/set_game_mode/level. Tokenizer 0x4359d0: attribute
  entries of 0x300 bytes (name +0, value +0x180); the token struct has name at +0xc, content pointer +0x18c, length +0x190,
  closing flag +0x194, self-closing +0x198. The scene stack is {DAT_005ba2b8 cursor, DAT_005ba2bc head, DAT_005ba2c0 tail}
  (Level objects, 0x3dc, class 0x40e820, type 6); DAT_005ba2a4 is the top scene.
- Main loop: the game widget's Draw (0x440650) catches up 250 ticks a second from timeGetTime()/4 (at most 20 a frame),
  one tick = 0x436d80 (Update every level, removing those that return false), drawing = 0x436de0 (Draw every level).
- Editor (class 0x42dc60, global DAT_00a1c938): no caller of its constructor, so unreachable in play; everything only it uses
  (0x42dc60-0x42e560, 0x42eb20-0x434b70 and so on) is listed by address in the notes instead of translated.
- profiles.dat: {int current, 10 x 0x1278-byte profiles, 5 x 1000-byte score tables}, 0xcc3c bytes; 0x435f10 builds the default
  (high-score names Marco Polo ... Titian).
