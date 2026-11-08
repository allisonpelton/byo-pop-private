# Brief for translating Typer Shark! Deluxe 1.02 onto the port

Repository: the git worktree you are started in (paths below are relative to it). The original's code is exported
by Ghidra to `re/typershark/gh/` (git-ignored: it holds the game's code and strings, never copy it into the
repository). The port is a translation of the original's game classes, function by function, onto the portable
SexyAppFramework (`framework/`, SDL2, Emscripten), the way `games/pixelus/src/` and `games/bookworm/src/` were made.
Typer Shark (game PE linked 2004-04-20) uses the same early-2004 framework snapshot as BookWorm, so
`docs/FRAMEWORK_2004.md` (BookWorm's map) and `framework/compat2004/Compat2004.h` apply. Read `games/pixelus/src/`
for the style (it is the freshest and smallest port of this generation).

## Reading the original

```bash
python games/typershark/tools/decread.py 4092a0 [00404600 ...]   # functions by address, compact and readable
python games/typershark/tools/decread.py -r 404600 405e10         # every function from the first to the last address
```

`decread.py` strips the exception-frame boilerplate, shows `this->field` accesses as `this.d4` (hex offset; a suffix
`{c}` char, `{s}` short, `{f}` float, `{d}` double marks the type), `param_1[0x18d]` stays an *index* (offset =
index x 4), virtual calls as `obj.v[N]` (slot N, offset / 4), and string literals as `"text"@52d870` (the address
in the game image: a translation names it `GT(0x0052d870)`). The raw export is `re/typershark/gh/decomp/<Class>/<addr>_<name>.c`
(unnamed functions are under `_global`); `re/typershark/gh/functions.jsonl` has, for each function, `addr`, `name`, `ns`
(class), `size`, `callers`, `callees` and `strings`; `re/typershark/gh/vtables.jsonl` the vftables and their slots.

Classes are mostly laid out in link order, so a class's non-virtual methods (unnamed `FUN_` functions) sit between its
virtual ones; the address map of the game code (0x401000-0x4623f0):

| Range | What is there |
|---|---|
| 0x401000-0x412330 | the entities: GameObject, Enemy, Shark and its variants, Piranha and variants, Projectile, Torpedo, TorpedoBoss, ProjectileBoss, GhostShip, Mecha*, Clam, BonusCreature, BackgroundFish, Effect, plus Dinkumware STL instances (std::string, map, vector, exceptions: not translated, use the standard library) |
| 0x412330-0x417000 | WinWSApp's first helpers and STL instances |
| 0x4172f0-0x428000 | UserProfileMgr (users\user.idx, users\user%d.dat: load, save, registry), UserProfile, LessonManager |
| 0x428500-0x437960 | WinWSApp virtuals (Init 0x428500, ButtonDepress 0x431630, LoadingThreadProc 0x435560, destructor 0x4366a0, constructor 0x4373b0), the difficulty and abyss XML config parser (0x42bfb0), gem and ship names, the word lists (0x430d20, 0x434740), themes (0x434d60) |
| 0x437990-0x43fbd0 | the dialogs (Win, AbyssMode, DeleteUser, Difficulty, GameOver, Hint, Money, NewUser, Options, Register, SelectUser, Key) |
| 0x43fbd0-0x462290 | the widgets and screens: FadeButton, XWidget, TooltipWidget, SoundSlider, WSTimer, StatsScreen, StatusBar, TitleScreen, MenuScreen, MapScreen, HighScoreScreen, TabbedWidget/TabPanel/TutorialPanel/AbyssPanel/AdventurePanel, Board (0x43fbf0-0x45c140), TypingTutor, then Player, Bubble, BonusText |

Names in the export: BookWorm's framework names were carried over and RTTI gave `Class::vfNN` for virtual slots (NN =
slot, so `Board::vf16` is slot 16). `ctor_or_dtor_<addr>` is a constructor or destructor (look at what it stores).
Many functions are still `FUN_<addr>`: name them in your translation (a comment with the address says what it is).

## Where things go and who owns what

All game code is in `games/typershark/src/` (flat; one `.h`/`.cpp` per class, named after the class; the names are
the original's: `WinWSApp`, `Board`, `MapScreen`...). `Res.h`/`Res.cpp` (image, sound and treasure-name ids),
`GameText.h` and `GameTextTable.inc`, `main.cpp` and the CMake file exist; do not edit them. You write your own
group's files. A header or field of another group that you need and that does not exist yet: add it yourself in
that group's header under a `// added for <your group>` banner, with the original's offset in a comment, and say so
in your final reply (groups are done one after another, so nothing is edited at the same time).

The application object (`WinWSApp`, offset `+0x618` is where its own data starts, after SexyApp's 0x618 bytes) is read
everywhere as `app + offset`; `gApp` is the one global. Keep the original's field order and put its offset in a
comment on each field (`// +0x8e8 ...`); nothing depends on the offsets, the names are what other classes use.
`grep -n "+0x8e8" games/typershark/src/*.h` finds a field named by an earlier group.

## The framework

- Framework headers: `framework/*.h` (included as `"Widget.h"`), platform layer `platform/` (`"platform/VFS.h"`).
  `SexyString` is `std::string`. Read a header before using a call: signatures differ from 2004 in places
  (`KeyChar(SexyChar)`; `CreateMusicInterface()` takes no HWND). The decompilation's virtual calls are by 2004 slot
  number: translate them to the port's methods by name (`docs/FRAMEWORK_2004.md` section 2 has the slot tables).
- Game dialogs derive from `Dialog2004` (`framework/compat2004/Compat2004.h`), sliders from `Slider2004`; text with
  `WriteString2004`/`WriteWordWrapped2004`/`GetWordWrappedHeight2004`; `GameTickCount()` for `GetTickCount()`;
  `CrtRand()`/`CrtSRand()` for the C library's `rand()`/`srand()`; `Sexy::Rand()` only where the original calls the
  framework's; `SeedMTRand2004` where it seeds an `MTRand`. Never call the host `rand()`.
- Every widget the game makes needs an ImageFont (the port has no system-font fallback).
- Floating point: x87 doubles are SSE2 doubles in the port. Keep float versus double exactly as the original (a float
  store rounds), `(int)` for `_ftol` (truncation), and the original's operation order.
- Files are read through the framework (`p_fopen`, `Platform::VFS`) and written to the user dir
  (`SexyAppBase::WriteBufferToFile`, `GetAppDataFolder()`); keep every byte format identical to the original so
  its save files load. The registry is the framework's emulation (`RegistryReadInteger`...), with the original's
  value names (key `PopCap\TyperShark`).
- Random numbers: every draw of the original goes through the same generator in the same order (a later pass compares
  draws with the original's). Find the generator the original draws from (the static `Rand`/`MTRand`) and keep each call.
- Web build: no threads; `LoadingThreadProc` runs synchronously.

## What is not ported (left out as the code is translated)

The retail game is registered and has no network features (`properties/partner.xml`: NoReg, DontUpdate). Leave out,
with a one-line `// 0xADDR ... not ported: <what>` where the code would be:
registration (RegisterDialog, KeyDialog, the BigInt key check, the trial limits MaxPlays / MaxTime / MaxExecutions
and the "please register" prompts, the Register image), the update check (UpdateCheck, `UPDATES`, the popcap.com
URLs), BetaSupport, crash and HTTP reports, ads, "More Games" and every other button that opens a web page or a
program (neighbouring buttons are widened or moved to close the gap; list it in your reply), demo recording and
playback, the profiler and MODVAL. Take the registered path everywhere a registration or trial test decides.

## Style

- Tabs, Hungarian names (`mField`, `theParam`, `aLocal`), namespace `Sexy`, LF line endings.
- Every function starts with a comment `// 0xADDR` (the original's address) and a short note where the behaviour is
  not obvious. A class header comment: `Original: class Sexy::X : public Y, vftable 0x..., sizeof 0x...`; methods
  carry `// 0xADDR slot N` for virtuals.
- Translate loops in their original shape (a test-then-increment loop does not become a `for` loop); do not "clean
  up" the logic; check what a comment claims about the original before writing it. Keep the original's bugs and quirks
  (with a comment) unless they crash or leak; a leak is fixed with a `port:` note. Mark every override with `override`.
- Deviations are marked `// port: <reason>`.
- User-visible text (every dialog, message, tutorial and level text, button label that is more than a short token) is
  `GT(0x0052d870)` with its address from `decread` (`GameText.h`; `GameTextTable.inc` lists the addresses). File
  names, registry value names, XML tags and format strings of the code itself may be literals. Never paste long game
  text, Ghidra output or asset data into the repository.

## Checking your work

There is no full build until every group exists. Syntax-check each of your `.cpp` files on its own:

```bash
E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang++.exe -std=c++17 -fsyntax-only \
  -Wall -Wno-unused-private-field -Wno-deprecated-declarations -Wno-switch \
  -IE:/retools/SDL2-2.32.10/i686-w64-mingw32/include/SDL2 -I. -Iframework -Iplatform -Igames/typershark/src \
  -Ithird_party/stb games/typershark/src/<File>.cpp
```

Missing headers of groups not yet written are expected; everything else compiles cleanly.

## Rules

- Never run a game exe, Ghidra, or anything that opens a window or plays sound. Do not commit. Do not download
  anything. Do not edit files outside `games/typershark/src/` and `games/typershark/docs/` (notes). Do not run
  anything longer than about four minutes.
- No references: work from our own Ghidra export, the framework headers in `framework/`, PopCap's released SAF
  source (`E:/retools/refs`) and BookWorm's port; no other project's decompilation.
- Final reply, short: the files you wrote, what you could not translate or verify, the cross-group names you
  assumed or added, and any framework behaviour difference you found.
