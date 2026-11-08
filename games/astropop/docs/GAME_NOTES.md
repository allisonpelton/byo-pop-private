# AstroPop Deluxe: notes on the original (AstroPopGame.exe, 2004)

Working notes of the port, kept so a new session need not read the decompilation again. Addresses are virtual
addresses in the carved game image (base 0x400000). The game's texts are never copied here: they are named by their
address (`GT(0x...)`) and read from the player's own exe.

## 1. Setup state (2026-11-07, session start)

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

## 3. Translated so far (2026-11-08)

| Class / file | Original | State |
|---|---|---|
| `AstroPopApp` | ctor 0x427ee0, Init 0x42f170, loading thread 0x42cda0, music 0x408b90 | done up to the title; ButtonDepress hub 0x431ac0 only for the title's button (id 2200 -> OnClickToPlay 0x4319b0) |
| `LoaderScreen` | 0x436310 family | done (slides, progress, click-to-play button) |
| `UserProfile` | ctor 0x42dc60, Load 0x42eca0, Save 0x4276b0, InitNew 0x429510, ResetGame 0x424540 | done (numbered settings 0..0x73) |
| `FrameAnim` | 0x40-byte counter 0x424370 / Update 0x40e130 | done (header only) |
| `AstroButton` | 0x454c10, sizeof 0x1f0 | done (three animations, glow, tints, hit mask, hover sound) |
| `XcfFile` | 0x427d80, Parse 0x42a5d0 | done, no signature check, no key-name check |
| `MainMenu` | 0x454ee0, sizeof 0xd8 | done except the flying-ships screen (class 0x434ac0, shown after a hidden hover sequence) |
| startup flow | 0x4319b0 -> LoadDescriptor 0x42c150 -> SelectStartUser 0x4300c0 -> ShowMainMenu 0x411490 | done; the dialogs it opens are stubs that log "not translated" |
| Board | ctor 0x480020 (sizeof 0x1c0c), vtable 0x566a30 (47 slots), listener 0x566980 | not started |
| custom dialogs | builder 0x411ba0 / 0x409080, base ctors 0x482140 0x489c60 ..., vtables 0x56a9e4 .. 0x56b8b8 | not started |
| CharSelector | 0x4556c0 | not started |

The game's own classes are few (about 20 vtables, listed by `re/astropop/vtables.txt`); the Board is one class whose members
hold the vectors of every kind of object, so most of the code is Board methods.

## 4. Reading notes

- Ghidra's `this.NNN` is a hex byte offset; `param_1.NNN{c}` on an `int*` is a **dword index** (check the asm: `.196` was
  +0x658). Stack-passed `std::string` arguments are lost by the decompiler; read the asm for call arguments.
- A listener subobject's `this` is the owner plus 0x58 (MainMenu, dialogs): `this.28.690` in a listener method is
  owner+0x80 -> the app's `mUser`.
- `FUN_004b83f0` is the 2004 `DrawImageCel(img, x, y, cel)`: the cel counts along the rows when the image has more than one
  row, else along the columns; out-of-range cels draw nothing. The port's `Graphics::DrawImageCel` indexes `cel % cols,
  cel / cols`; the game's own helper is used where the two differ.
- `tools/ctorstores.py <addr>` lists a constructor's field stores compactly (zero runs folded); the Board's vectors are
  0x10 bytes each (allocator slot, begin, end, capacity).
- The decompiled app offsets that matter so far: +0x628 Board, +0x62c 43 lists (12 bytes each, made by 0x4319b0),
  +0x640 loader screen, +0x644 MainMenu, +0x64c the screen over the selector, +0x658 / +0x6f9 / +0x6fa / +0x6fb flags set from
  the profile by ValidateUser 0x4285d0, +0x684 the player names, +0x690 the current profile, +0x6a8 the descriptor.

## 5. Left out of the port (registration, links, statistics)

- The "More Games" button (id 0xfad, button index 4 of the selector). Its panel is part of the selector's background
  image, so with the button gone the picture still shows it; the gap is to be closed by drawing the neighbouring panels over
  it (check with shotdiff). Its sound, rectangle and highlight stay in the tables.
- The page counter calls on the ResourceManager (0x48f860 / 0x48f8a0: "Init", "MoreGames"): PopCap's ad statistics.
- The signature checks of the .xcf files (error 5 "Authentication failed ...").

## 6. Deviations from the original

- `XcfFile::Parse` reads the file with plain line rules and keeps every key; the original validates the key names against
  a list given before the parse (28 names for the descriptor) and checks the .sig (both left out; unmodified files give the
  same values).
- The original's Rand() for the selector's rising particles is the game's generator (`MTRand::NextNoAssert`, 0x5a4e28);
  the port draws them from `Rand()` in the same order (3D mode only).

## 7. Fix candidates (original bugs and waits, kept in the port, for the fixes mod)

None found yet.

## 8. Oddities (not chased)

- The selector's hidden hover sequence (Classic, Survival, Hall of Fame, Options... in order, then Quit) starts the screen
  of ships flying off; not translated yet (class 0x434ac0).

## 9. Next steps

1. Board: the vectors, the descriptor values it reads, level loading (0x4663f0), the bricks, ships, power-ups, the effects.
2. The dialog family (new player, select player, options, quit, continue) and the ButtonDepress hub's other ids.
3. CharSelector, Hall of Fame, high scores, story/help screens, the flying-ships screen.
4. Standard test commands (`goto`, `level`, `win`, `lose`, `set`, `seed`, `move` with the port's bot), `tools/check.sh
   astropop`, the frozen baseline, the web build; then the identical-play pass and the fixes mod.
