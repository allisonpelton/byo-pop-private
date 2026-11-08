# AstroPop Deluxe: notes on the original (AstroPopGame.exe, 2004)

Working notes of the port, kept so a new session need not read the decompilation again. Addresses are virtual
addresses in the carved game image (base 0x400000). The game's texts are never copied here: they are named by their
address (`GT(0x...)`) and read from the player's own exe.

## 1. Setup state (2026-11-07, session start; the port itself has not begun)

- Branch `claude/astropop-c6c8fd`, cut onto the local `main` (the worktree was first made from a publish snapshot and
  rebased with `git rebase --onto main d1b03ceb`).
- Game files staged by `python games/astropop/tools/stage_astropop_assets.py` into `assets/astropop/` (974 files, 20 MB:
  images, data, fonts, sounds, music, properties, and `winap.exe`, the wrapper the texts are read from). The install used is
  `A:/SteamLibrary/steamapps/common/AstroPop Deluxe` (read only).
- The carved game image is `re/astropop/bin/AstroPopGame.exe` (copied from the main checkout's `re/astropop/bin`; 1,748,992
  bytes, linked 2004-04-06, MSVC 7.0). Sections: .text 0x401000-0x55c000, .rdata 0x55d000, .data 0x59c000.
- Analysis, all git-ignored under `re/astropop/`: `rtti.jsonl` and `rtti.classes.json` (`tools/rtti_msvc70.py`: framework
  RTTI only, 128 classes), the Ghidra project `astropop_game` (E:/retools/projects) and its export `gh/`
  (`functions.jsonl`, `strings.jsonl`, `vtables.jsonl`, `decomp/<class>/<addr>_<name>.c`), `bw_match.jsonl` and
  `bw_names.jsonl` (`tools/funcmatch.py` against BookWorm: 3,175 of 8,624 functions paired, 1,684 named, applied with
  `tools/ghidra_headless.sh apply` and re-exported), `vtables.txt` (`games/astropop/tools/vtscan.py`).
- Reading tool: `python games/astropop/tools/decread.py <addr> ...` prints a function with the string boilerplate, field
  offsets (`this.5f4`) and virtual calls (`obj.v[13]`) condensed, strings with their address (`"text"@562494` -> `GT(0x562494)`).

## 2. Layout of the image

- 0x401000-0x48bcf0: the game and STL/iostream template code (MSVC 7 inlines `std::string` everywhere); 0x48bcf0 on: the
  framework (ResourceManager 0x48cbf0, PropertiesParser 0x490970, Widget 0x4e6d20 ...). 0x540000-0x55c000: CRT and the
  exception-unwind stubs (`Unwind@...`, ignore).
- The functions Ghidra found: 8,665 (about 1,800 in the game region 0x401000-0x488000, 560 KB; the survey's 1,030 counts
  after STL merging). Big generated functions: 0x4092e0 `GetStringIdById`-style switch from resource index to name
  (0..0x2b1 images, 690 ids, then fonts and sounds); 0x4181a0 is the inverse (name to id). `resources.xml` has 604 images,
  16 fonts, 74 sounds in 44 groups with `SetDefaults idprefix="IMAGE_..."`; the extra attributes `static`, `delay`,
  `lastanimframe`, `noalpha` are read by the game.
- Vtable scan (`re/astropop/vtables.txt`): game classes have no RTTI. Constructors found by who stores the vtable address:
  AstroPop app ctor 0x427ee0 (vftable 0x5629a0, 68 slots like BookWorm's: slot 4 ButtonDepress 0x431ac0, 13
  LoadingThreadCompleted 0x4242d0, 14 deleting dtor 0x42dc30, 16 Init 0x42f170, 17 LoadingThreadProc 0x42cda0, 33 Shutdown
  0x4290d0, 55 DoDialog 0x412360, 62 GotFocus 0x4253e0, 63 LostFocus 0x425380, 67 TitleScreenDone 0x408b70).
  The app calls SexyApp ctor 0x4e1030 (and 0x4e0870 in the dtor). Widget-like game classes (47-62 slots) have ctors at 0x434ac0,
  0x435640, 0x436310, 0x43c510, 0x445b10, 0x446720, 0x44e5c0, 0x44e720, 0x454c10, 0x46c040, 0x477b90, 0x482140,
  0x483170, 0x4846e0, 0x484830, 0x488760, 0x489c60, 0x48a560. The vtables at .rdata 0x55d3c0-0x55dda0 are iostream
  instantiations (ignore).
- Init (0x42f170) loads the resource groups "Init", "Required", "AllwaysLoaded", "Game", "GameSelector", "CharSelector",
  "Title"; registry keys APVersion and SGFVersion; user files `users\data%d.usr`; the old-version delete dialogs; the music
  `music\music.mo3`.

## 3. Next steps (the plan in order)

1. Skeleton: `CMakeLists.txt`, `main.cpp`, `GameText` (copy Pixelus's `GameText.cpp/h` and `make_gametext.py`), `Res` from
   resources.xml, `AstroPopApp` (copy BookWormApp.h's mapping: same 68-slot vtable), build and open the title.
2. Class inventory from the ctors above and the call graph; write each class into section 4 of this file as it is read.
3. Translate class by class; commit at verified milestones; standard test commands, `tools/check.sh astropop`, a frozen
   baseline, the web build; then the identical-play pass and the fixes mod (CLAUDE.md, "Making a port").
