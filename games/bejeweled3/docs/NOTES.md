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
  (6,628, of which 157 have an empty id and are skipped, as the exe skips them), `Sound` (189), `Font` (128), `PopAnim` (64), `PIEffect` (118), `RenderEffect` (14) and `File` (36).
- Fonts are DescParser scripts of the commands `framework/ImageFont.cpp` already reads (layers, tags, kerning, colour
  multipliers, draw modes).
- PopAnim files are versions 4 (47 files) and 5 (17); Twist's player reads up to 3.
- Music is one MO3 suite, `music/bejeweled3_suite.mo3`, with `properties/music.xml` naming each song by its order
  offset and its events (fades as curve strings, row-synchronised switches).
- 487 curve strings sit in the exe (`b+0,1,0.01,1,~###  ~####` and the like), as in Twist.

## Resources

- The resource manager (`engines/prime/PrimeResourceManager`, the exe's 0x45b540-0x465600, vtable 0x871c8c) is the
  framework's of this generation, with three more kinds (PopAnim, PIEffect, RenderEffect), art sizes and composite
  groups. A group's `res` makes each of its ids "ID|res"; `GetBaseRes` tries "ID|<the app's art size>" before "ID".
  Loading a composite group loads its parts whose `res` is 0 or the app's art size, in order. `<File>` entries are
  skipped. Animations are scaled by the app's art size over the manifest's base size.
- Not translated, because the PC build never reaches them: the resource streams (app+0x85c, no streams file ships), the
  decoded-image cache the manager keeps under the name "ResMan", reparsing the manifest while running, system fonts, and
  an image font's flag at +0x35 of the image.
- The generated code (ResourceGen3, 0x6e8ca0-0x70e2ff) is `src/Res.h` and `src/Res.cpp`, written by
  `tools/resgen.py` from the exe's id table (2,636 ids, GetStringIdById's jump table at 0x6eca80), the manifest's kinds
  and the export's extraction functions (3,935 getter calls). The game calls four of them:
  `ExtractLoadingThreadResources` (0x70e0f0, from the loading thread), `ExtractLoaderResources` (0x70e1b0),
  `ExtractInitResources` (0x70e260) and `Extract2DOnlyResources` (0x704c10, from Bej3App at 0x7748e0). The first three
  take the part for the app's art size (600, 768 or 1200) and then the part every size shares. Each part first gives
  every global to the manager (0x6efb20), then gets its resources in a try block that returns false on a
  ResourceManagerException. The getters also record each resource's id under a lock (0x8e1f98).
- `b3_restest` (tools/restest) parses the manifest and loads and extracts the groups in the game's order at each art
  size: at 600, 768 and 1200 the groups Init, Loader and 2DOnly load and extract; LoadingThread stops at its first
  animation until the PopAnim player is translated. The pictures are mostly JPEG 2000 (4,557 `.jp2` files), read by
  the platform's OpenJPEG decoder (`sexyopenjpeg`).
- Three fonts (flaregothic32 at each size) are UTF-8 with a byte order mark, which the exe's EncodingParser honours;
  the framework's DescParser reads such a file as UTF-8 now, each character as its Latin-1 byte and the euro sign, the
  only character above U+00FF, as 0x80. The others are Windows-1252 bytes.

## Particles

- `engines/prime/PIEffect` reads every `.ppf` file of the game to its last byte (`b3_pieffecttest`: 118 files, 276
  emitters, 298 particle definitions, 253 layers). The random generator is framework/MTRand at PIEffect+0x78.
- Not yet translated: the update and the drawing (Update 0x573290, UpdateParticleDef 0x5707d0, UpdateParticleGroup
  0x571ad0, CalcParticleTransform 0x56fe50, GetGeomPos 0x569f80, GetEmitterPos 0x56fda0, DrawParticleGroup 0x574060,
  DrawLayer 0x574700, Draw 0x575650, ResetAnim 0x56f730, Clear 0x56fab0, LoadState 0x56e4f0, SaveState 0x56eed0) and
  the run-time classes (layers, emitter instances, particles, groups, free emitters); the textures' images (0x5679b0,
  through the app's shared images); the seeding of the generator. Eleven files end with a cached starting state that
  LoadState reads.
- Names left to check against the update and draw code: several flags and values of the particle definitions, the
  emitters and the emitter instances, and the placeholders mFlagA to mFlagC and mValueA to mValueH (`PIEffectDef.h`).
