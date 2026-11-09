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
| `AstroPopApp` | ctor 0x427ee0, Init 0x42f170, loading thread 0x42cda0, music 0x408b90, hub 0x431ac0 | done (the screens it opens are in AppScreens, AppHall, AppHub, AppUsers, AppRank, AppDialogs, AppGame) |
| `LoaderScreen` | 0x436310 family | done (slides, progress, click-to-play button) |
| `UserProfile` | ctor 0x42dc60, Load 0x42eca0, Save 0x4276b0, InitNew 0x429510, ResetGame 0x424540 | done (numbered settings 0..0x73) |
| `FrameAnim` | 0x40-byte counter 0x424370 / Update 0x40e130 | done (header only) |
| `AstroButton` | 0x454c10, sizeof 0x1f0 | done (three animations, glow, tints, hit mask, hover sound) |
| `XcfFile` | 0x427d80, Parse 0x42a5d0 | done, no signature check, no key-name check |
| `MainMenu` | 0x454ee0, sizeof 0xd8 | done; the flying-ships screen (`ShipsScreen`, class 0x434ac0, `CloseGoScreen`) too |
| `Board` | ctor 0x480020 (sizeof 0x1c0c), vtable 0x566a30 (47 slots), listener 0x566980 | done (Board*.cpp: field, bricks, ships, power-ups, effects, bonus rounds, warp, end, replay recording, hints, test bot) |
| custom dialogs | builder 0x411ba0 / 0x409080, base ctors 0x482140 0x489c60 ... | done (`AstroDialog`, `ListDialog`, `HelpDialog`, `CheckpointDialog`, `PowerDialog`, upgrade box) |
| `CharSelector` | 0x4556c0 | done (the registration locks are left out) |
| `HallOfFame`, `CreditsScreen`, `FullScreenHelp` | 0x4288a0 family, credits, help | done |
| `StoryScreen`, `StoryPage` | ctor 0x4567d0, Update 0x469140, Draw 0x447400, `Board::BuildStoryText` 0x47a940, `OpenStoryScreen` 0x468e70, `LoadStoryPages` 0x45ef90, `CloseStory` 0x469030 | done (the texts of the stories are listed by address in `StoryText.inc`, made by `tools/make_storytext.py`) |
| test commands | `testhooks/TestCommands.cpp`, bot in `BoardBot.cpp` | done (see 3b) |

The game's own classes are few (about 20 vtables, listed by `re/astropop/vtables.txt`); the Board is one class whose members
hold the vectors of every kind of object, so most of the code is Board methods.

### 3b. Test commands

`state`, `goto menu|classic [level [pilot]]|survival [pilot]`, `level`, `seed`, `unlock all`, `win`, `lose`, `move` (the
port's bot, `BoardBot.cpp`) and `set score|level|stage|charge|stories`. `set charge` charges the pilot's weapon and
`set stories` makes the profile hold every story as seen (the finale then follows story 9 of any pilot); both exist only
in the test hooks (docs/TEST_COMMANDS.md lists the standard ones). A game starts with the full-screen help up: a script
clicks Continue (320,440) first.

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

## 4b. The game's event bus (the key to the Board's structure)

The app owns 43 lists (+0x62c, 12 bytes each: an STL list head and its size; made by 0x4319b0). Every game object and the
Board hold a pointer to them (Board +0x5c / +0x68, objects +4 / +0x10). A list is an event channel: `FUN_00425290(listener, id)`
registers the listener at the end of list `id` (0x424270), `FUN_0040ddc0` asks whether it is registered (0x40d7e0),
`FUN_0040e7c0` removes it (0x40dfa0), `FUN_0040e7e0` removes it from every list (0x40e000), and `FUN_0040dd40(source, id)`
fires the event (0x40cde0): for each listener of list `id`, in registration order, it calls the listener's virtual method
for that event with the source as the argument (and first the optional trace hook at source+8, which is never set).
Registered pointers are the **second subobject** (+0xc) of an object: its vtable (0x55d500, 69 slots) holds the handlers.
Event id -> handler slot of the listener's vtable (events 10, 0x16..0x19, 0x1d and 0x1f..0x2a are never fired through
0x40cde0; the lists 0x16..0x19 and 0x25 are walked by the Board itself):

| event | slot | event | slot | event | slot | event | slot |
|---|---|---|---|---|---|---|---|
| 0 | 1 | 9 | 6 | 0x10 | 15 | 0x1b | 19 |
| 1 | 2 | 0xb | 12 | 0x11 | 17 | 0x1c | 18 |
| 2 | 3 | 0xc | 7 | 0x12 | 21 | 0x1e | 16 |
| 3 | 4 | 0xd | 20 | 0x13 | 22 | 0x1a | 29 |
| 4 | 5 | 0xe | 14 | 0x14 | 23 | | |
| 5 | 8 | 0xf | 13 | 0x15 | 24 | | |
| 6 | 9 | | | | | | |
| 7 | 10 | | | | | | |
| 8 | 11 | | | | | | |

The playfield object class (sizeof 0x234 / 0x23c / 0x254, vtables 0x55d500 .. 0x55d860: 0x55d860 has a second copy of the
table, 143 slots) is two subobjects: +0 a state-machine part (vtable 0x55d5a8: slot 1 SetState(int), 5/6/7 per-state
updates ...) and +0xc the listener part. Methods reached through the +0xc vtable get `this` = object+0xc, so their field
offsets are relative to it (add 0xc for the object's own offsets).

## 5. Left out of the port (registration, links, statistics)

- Registration (the registered path is the only one): the trial and nag code, the register screen, the update check, the
  link to the publisher's site, the Options screen's REGISTER and update buttons, the registration locks of the pilot
  selector, and what a registered copy shows. The code reads the profile's flags as the registered game's.
- The "More Games" button (id 0xfad, button index 4 of the selector). Its panel is part of the selector's background
  image, so with the button gone the picture still shows it; the gap is to be closed by drawing the neighbouring panels over
  it (check with shotdiff). Its sound, rectangle and highlight stay in the tables. The main menu's hidden hover sequence
  (see 8) used that button as its first step.
- The page counter calls on the ResourceManager (0x48f860 / 0x48f8a0: "Init", "MoreGames", the story pages and the end
  scene): PopCap's ad statistics. Also the clock reset the original makes after a load (0x4ce790), the trace logs and the
  demo's marker calls (`DemoAddMarker`).
- The signature checks of the .xcf files (error 5 "Authentication failed ...").
- The bonus-file check of `LoadBonusLevels` (0x42c910: the thirty files are read and their signatures checked while the
  loading screen runs; the Board reads the file of a bonus round itself when the round starts).
- The registry-writable check at start and the box about the windowed colour depth.

## 6. Deviations from the original

- `XcfFile::Parse` reads the file with plain line rules and keeps every key; the original validates the key names against
  a list given before the parse (28 names for the descriptor) and checks the .sig (both left out; unmodified files give the
  same values).
- The tunable-constants table (0x4f1e40, a lookup of numbers by name) is replaced by its default values.
- `StartLoopSound` stops a loop with `Stop()` and starts it with `Play(false, false)`.
- `PointerTrail::Step` on the warp stars is left out (the original steps a trail that nothing reads).
- The Particle's `Draw` is not virtual in the port.
- The announcement text of a dialog is a `std::string` and `AstroDialog::GetPreferredHeight` measures with a `Graphics`
  that has no image.
- The end page does not read the profile name (a value the original does not use); the default case of the survival
  bonus' text is not translated.
- The hub of `AstroPopApp` guards `mBoard` against NULL where the original does not.
- A list dialog keeps its line colours in step with its lines.
- The pilot hint takes the spot it found (`DismissHint`) instead of searching again.
- `AnyMoveLeft` counts in vectors sized 16 with an 8-entry histogram.
- The story screen shows the finale only after the last story of a pilot when the profile has seen 33 stories, as the
  original; its "next" button strip is freed after the three buttons are copied from it, and when the pictures of a
  story cannot be loaded the game ends with the resource error box without setting the loading-failed flag.
- The `radar` attribute error box and the 3000-picture search box of `LoadStoryPages` cannot happen with the shipped
  descriptors and are not translated (the search stops at the last picture instead).
- Memory leaks fixed: the credits screen's picture strip, the story screen's button strip, the three pictures of the supa
  weapon box (freed by the box), the hall of fame's personal draw with no user (guarded).
- Test-only: the `move` bot, `set charge` and `set stories` (see 3b).

## 7. Fix candidates (original bugs and waits, kept in the port, for the fixes mod)

None found yet.

## 8. Oddities (not chased)

- The selector's hidden hover sequence (Classic, Survival, Hall of Fame, Options... in order, then Quit) starts the screen
  of ships flying off (`ShipsScreen`, translated); with the More Games button gone the sequence's first step is missing,
  so the screen cannot be reached until the sequence is decided.

- On the machine the pass runs on (Windows 11, build 26300) the original cannot load its fonts: the loading thread stops at
  its task 203 of 206 with an unhandled exception, every time, after libpng warns about a palette. The three palette PNGs
  of the game (`fonts/_Kiloton9.png`, `_Fusion16.png`, `_NeuropolX30outline.png`) are the only ones it has; the staged copy
  that origrun runs (`games/astropop/tools/origcmp/stage_original.sh`) stores them as RGBA PNGs with the same pixels, and then
  the original loads and runs. Found with origrun's `--coverage` (the functions the loading thread reached, in order: the
  palette handling, then the unpack of a 2-bit palette row, then the exception filter). The port decodes the same files without
  trouble. Not chased further (the user's copies on older Windows may be fine).

## 9. Next steps

1. The web build and its registrations (page.js, stores, profiles, saveimport, controls, sound tracks), the origrun row.
2. The identical-play pass, both phases, with `games/astropop/docs/IDENTICAL_PLAY.md`; real-input driver play.
3. The frozen baseline `build/baseline-astropop`, the fixes mod (or "none"), the performance numbers.
4. Close the gap of the removed More Games button; the app's destructor does not delete its screens (as the original's).
