# Bejeweled 3: working notes

What the analysis has found about the exe, for reading the export. The plan is in `PORT_PLAN.md`.

## Reading the export

- `games/bejeweled3/tools/re3.py` prints a function of `re/bejeweled3/gh2` compacted and named (`dec`), its callers and
  callees (`calls`), its disassembly (`asm`), a table of pointers (`table`), the classes placed by their own vtable slots
  (`classes`), a class's vtable (`vt`), the strings matching a pattern with the functions that use them (`str`), and the
  names matching a pattern (`name`). Names come from `re/bejeweled3/names/*.jsonl` and the Mac build's pairs.
- `games/bejeweled3/tools/autoname.py` names the functions that use the texts of `properties/default.xml`: each text's
  AUTOGEN comment names the `Class::Method` that uses it (160 functions; a text several methods share, or one used in
  more than three places, names nothing).
- The exe's `operator delete` is 0x64dbcf, `operator new` 0x64dca8, `std::string::assign(const char*, n)` 0x401710;
  the stack cookie is `DAT_008bdf24`.
- Link-time code generation passes `this` and arguments in whatever register suits the caller: the decompiler shows
  them as `unaff_EDI`, `in_EAX` and the like, and a call whose arguments go in registers shows none (`FUN_00401710()`).
  Read such a function's callers in `asm` to see what each register holds. Turning the 1,080 functions' stack
  alignments (`and esp, -8`) into NOPs before the analysis was tried and changed nothing.
- Zuma's Revenge's Mac build kept its symbols and has the same framework: `tools/macmatch.py` with it against this
  export pairs 399 functions (234 of the framework: PopAnim, PIEffect, the resource manager), in
  `re/bejeweled3/macmatch_zr`, which `re3.py` reads after the session's own names.

## Sources

- `properties/default.xml` is the game's text table (`<String id>`), each with the method that uses it; the 45 source
  files it names are the game's own (Badge.cpp, Bej3App.cpp, Board.cpp, the boards of each mode, the dialogs, the menus,
  Profile.cpp, HighScoreMgr.cpp, QuestBoard.cpp, DigGoal.cpp...).
- The Mac build names 2,191 functions, mostly inline templates; macmatch pairs 38 (`PORT_PLAN.md`).

## The data

- `resources.xml` is ResourceGen3's manifest version 2: `<Resources id="1200_Main" res="1200" parent="Main">` groups by
  art size (600, 768, 1200) under `<CompositeResources id="Main">`, which lists the size groups; resource kinds `Image`
  (6,628), `Sound` (189), `Font` (128), `PopAnim` (64), `PIEffect` (118), `RenderEffect` (14) and `File` (36).
- Fonts are DescParser scripts of the commands `framework/ImageFont.cpp` already reads (layers, tags, kerning, colour
  multipliers, draw modes).
- PopAnim files are versions 4 (47 files) and 5 (17); Twist's player reads up to 3.
- Music is one MO3 suite, `music/bejeweled3_suite.mo3`, with `properties/music.xml` naming each song by its order
  offset and its events (fades as curve strings, row-synchronised switches).
- 487 curve strings sit in the exe (`b+0,1,0.01,1,~###  ~####` and the like), as in Twist.
