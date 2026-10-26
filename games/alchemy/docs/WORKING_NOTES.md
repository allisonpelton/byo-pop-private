# Alchemy Deluxe: working notes (analysis state, kept so a compaction loses nothing)

Alchemy.exe is a 2007 wrapper (2.9 MB) around the game PE, linked 2004-08-23 with MSVC 7.0 (linker 7.0). Image base 0x400000.
<!-- private: The game's image is taken out of the wrapper by a private tool, to `re/alchemy/bin/AlchemyGame.exe`. -->
The game PE is at 0x1891a2-0x2ca1a2 of the wrapper (1,314,816 bytes), carved with `tools/extract_game_exe.py` to
`re/alchemy/bin/AlchemyGame.exe`.
<!-- /private -->

**The 15 `.luc` scripts are the wrapper's, not the game's** (`drm/common/scripts/` = the buy/trial screen of the 2007
wrapper: DRMApp, LuaApp, Layout...). The game itself uses no Lua, so no PopLua link is needed.

## State (2026-10-26)

The port is written, builds natively and for the web, and plays (README.md, status). `GAME_NOTES.md` is the behaviour spec as 
the code has it; `tools/check.sh alchemy` is the test. The plan section below was the plan before the code existed.
A fidelity check of every drawing function against the disassembly followed (README.md); it found one dropped position
(the forge's back picture, now at (0x28, 0x147)) and two never-triggered effects (GAME_NOTES.md section 5).
An AddressSanitizer run, the web build's saved game, the full screen switch and the sounds were checked after that
(README.md, Status): no fault in the port; the web options' full screen box is the framework's and the pane's concern.
Lessons: the decompiler drops the trailing arguments of draw calls (check them in the disassembly: the selector's base picture 
is at (10, 0x23)); `LoadProperties()` must be called in Init or every `GetString` is empty and all dialog text is blank; 
`AllTimeDialog::GetPreferredHeight` is an override returning 0x156 (the base class's height for empty lines is too small).

## Where things are

- Game code 0x401000-0x413fff (about 280 functions, 76 KB, mixed with std::string/list/vector instantiations).
- 0x414000-0x47f000 framework (2857 functions byte-match BookWorm's; the SexyApp part 0x462000-0x477000 does not).
- 0x480000-0x4b0000 libs (libpng 1.0.5, zlib 1.1.3, libjpeg, ModVal), 0x4b1000-0x4d5000 STL/CRT, 0x4d6000+ import thunks.
- RTTI has only framework and std classes (120 classes): the game's own classes have no RTTI. Their vtables were found
  from constructor stores of `0x4e3xxx-0x4e5xxx` immediates.
- The game's strings are string IDs looked up at run time in `properties/default.xml` (the user's file, 228 strings);
  no GameText reader is needed for them. The exe holds a little more game text than ids: the default names of the score
  tables, which `GameText.cpp` reads from the player's exe (GAME_NOTES.md section 3); the strings are read from the player's file.
- Files: images/*.gif|jpg|png, sounds/*.ogg, music/*.s3m (2), data/*.txt (font descriptors + diff.txt), properties/*.xml.
  The wrapper's `drm/` folder, bass.dll, readme.html, license.txt are not used by the port.

## Framework generation

Widget layout is BookWorm's 2004 map (47 slots; docs/FRAMEWORK_2004.md section 3 applies). Dialog has 53 slots (BookWorm 52),
ButtonListener 7 (BookWorm 6), SexyApp 71 (BookWorm 67). The app slots are shifted: Alchemy slot N+1 = BookWorm slot N from
slot 6 on (Process 7, UpdateFrames 8, DoUpdateFrames 9, MakeWindow 10, ... CreateMusicInterface 16, Init 17,
LoadingThreadProc 18, WriteToRegistry 19, ReadFromRegistry 20, NewDialog 21, Popup 22, SafeDeleteWidget 23), then one extra
(24), Validate 25, URLOpenFailed 26, URLOpenSucceeded 27, ...

## Tools

- `re/alchemy/dis.sh <start> <stop>` (llvm-objdump intel disassembly) for the vtable functions Ghidra missed.
- `re/alchemy/game_clean.c` is the game region's decompilation; `python /tmp/fn.py <addr>...` style extraction is just a
  split on `// <addr>  ` headers.

## Reading tools (games/alchemy/tools)

`make_game_region.py` (game.c) -> `clean_decomp.py` (game_clean.c) -> `fn.py <addr>...` prints functions. `norm.py
re/alchemy/fields_Board.txt <addr>...` rewrites this-relative accesses as `@name` / `@0xBYTEOFFSET` and squashes the
GetString pattern into `strv(-0x3c) = GetString("ID")` (FUN_0046d7d0 = SexyApp::GetString(id)); `re/alchemy/fields_App.txt`
(optional, "0xoff name" lines) names `(@mApp + 0xNN)` as `@mApp->@name`. `dis.sh <start> <stop>` disassembles functions Ghidra
missed (llvm-objdump). Ghidra loses sprintf's variadic arguments: guess them from the format strings.
Funcmatch against BookWorm: `python tools/funcmatch.py re/alchemy/bin/AlchemyGame.exe re/alchemy/gh/functions.jsonl
<bookworm worktree>/re/bookworm/bin/BookwormGame.exe <bookworm worktree>/re/bookworm/gh/functions.jsonl re/alchemy/bw_match.jsonl
--apply-names re/alchemy/bw_names.jsonl` (2857 matched, 1388 named); apply with `ghidra_headless.sh apply rtti.jsonl bw_names.jsonl`.

## Game classes (vtable = start of constructor-stored table; widget = 47 slots, dialog = 53, ButtonListener part = 7)

0x4e32d0 Board (Widget + ButtonListener at +0x58; mApp at +0x5c; dtor 0x4026fa; size >= 0xe74). Overrides: Added(401014) Removed(40105c)
  slot13 Resize 40578f, slot16 Draw 404923, slot17 Update 406f2a, slot20 KeyChar 4017a0, slot25 MouseMove 401707, slot27 MouseDown(x,y,btn,cc) 4068f1.
0x4e3ab0 widget (ctor 40858e, Draw 4086e6, Update 40844f): small effect object. 0x4e3b6c/0x4e3b88 widget+listener (ctor 408a8a, Draw 408cd7 =
  help text "USING_FORGE_IS_LIMITED..."). 0x4e3d58 dialog (ctor 4096c8/40965a, Draw 409854 = "ALL_TIME"/high-score table dialog).
0x4e417c/0x4e4198 widget+listener (ctor 40bad1, size 0x21c, Draw 40b0b3 = TIME_TRIAL_HIGH_SCORES..., Update/Mouse 40a957/40afa9;
  reads/writes hiscores.dat 40a15a/40a341, defaults: a table of names in the game). 0x4e4260/70/a0: 40c092 ctor (824 B), 40bed1.
0x4e43a8 widget (ctor 40c817 size 0xa4, Draw 40cefe "INTROSCREEN_TIME_TRIAL"...: the game selector/intro screen), 0x4e4540 dialog (ctor 40d836 size
  0x144 = options dialog: OptionsLinkText0, CUSTOM_CURSORS, FULLSCREEN, SOUND_FX, MUSIC), 0x4e4798 (ctor 40e433 size 200: a Piece, +0x68 shape,
  +0x6c colour, +0xbd bomb?, +0xc4 skull?), 0x4e4880/0x4e48b0 (ctor 40ed00 size 0x120: name entry "ENTER_YOUR_NAME"), 0x4e49a0/d0 (40f314: register
  dialog, skip), 0x4e4b40 (40f987?), 0x4e4c30/50 (4103e2 ctor, size 0x88: title screen "CLICK_HERE_TO_PLAY").
0x4e5160 AlchemyApp (72 slots; ctor 4118c9, dtor 41159c; overrides ButtonPress 410db6(slot4) ButtonDepress 413eea(5) slot8 4111cb slot14 412a8a,
  slot15 411a25 dtor, Init(17) 412d69, LoadingThreadProc(18) 4124cf, WriteToRegistry(19) 411a41, ReadFromRegistry(20) 411b43, NewDialog(21) 410bb6,
  URLOpenSucceeded(27) 410d0e, OpenURL-ish 26 412aa0, 28 412c68, SetSfxVolume-ish 49 4111d0, 61 4111fa, 63 410cb9, 64 410cd3, 71 410d2a).
  App fields seen: +0x6b0 mImages[] (66 images, indices from the rdata name table), +0xe34..+0xe48 fonts, +0xe54 mBoard, +0xe58 effects mgr, +0xe5c
  (sound/music mgr), +0xe60.. 9 gamma-corrected backdrops (level backdrops), +0xec8 mSkillLevel, +0xed0 mTimeTrial, +0x534 plays count.
Board field facts (also in re/alchemy/fields_Board.txt): +0x60 mState (0 play,1 clearing line anim,2 end anim,3 game over,4 bomb,5 board cleared,6 level
  transition), grid 8 rows x 9 cols of Piece* at +0xb08, three byte masks at +0xc28/+0xc70/+0xcb8 (valid moves), +0xae8 current piece, cell = 0x34 px,
  origin x = col*0x34+10, y by 0x401510, piece colour table at 0x4de390 (12 B per colour), NewPiece = 0x401324.
Functions read so far: Board 401130,40118b,401220,401304,401324,40153a,401682,401707,4017ba,4017dd,40182f,40187c,4019d1,401a82,401c9b,401d15,402a44(save),
402f42,402fcf(tutorial step setup),403c24(hint balloons),403ee5,403f5c,40400d(board cleared screen),406f2a(Update);
App 4118c9,41159c,41222f,411a41,411b43,412d69,4124cf,413eea,410ee0,410f28,410fef,4110bf,411138,411357.

## Plan and decisions (2026-10-26)

- Dropped as the wrapper's/DRM: registration dialogs (40f314, 40f5e0, 413846, 413ab9, 413a2f), nags (413bdc PREGAME_NAG, 413e12 TIME_TRIAL_NAG), license
  errors, internet scores (41351d, 4131ca score upload), update check (412139, UpdateCheckDialog), URL/browser dialogs (412aa0, 412c68). The port is
  a registered copy; "more games" links are not opened.
- Order: read the rest of Board (404923 Draw, 4053e0..40578f, 40589a load game, 405e31, 4061d6..406347 place piece, 4068f1 MouseDown, 4062e5, 406ee4,
  407dc4..407ebe), then the screens (408..412 classes), then write src/: AlchemyApp, Board (+BoardDraw), Piece, Screens, HighScores, Dialogs, main.cpp,
  testhooks/Autoplay.cpp; games/alchemy/CMakeLists.txt; BUILD_ALCHEMY option in the top CMakeLists.txt and a `.claude/launch.json` entry on port 18777
  (shared: commit); tools/stage_alchemy_assets.py; web/page.js+page.css; tools/check.sh suite `alchemy` (shared: commit); README.
- The port pattern is games/atomica (Sonnet, 2003 framework, SexyApp from the port framework, sexycompat2004): copy its main.cpp / CMake / web page / check suite.
  The strings are SexyApp::GetString ids read from properties/default.xml by the port framework (LoadProperties); no GameText.
