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
port's bot, `BoardBot.cpp`) and `set score|level|stage|charge [N]|chargestart N|stories`. `set charge` charges the pilot's
weapon (to N of its 18000, full without N); `set chargestart N` makes every new board and every `ResetState` (the end of the
demo) start with N of charge, as origrun's `tools/origrun/patches/astropop-charge.txt` does to the original (it stores 18000
in the board's constructor and in `ResetState`), below 0 turns it off; `set stories` makes the profile hold every story as seen
(the finale then follows story 9 of any pilot). All exist only in the test hooks (docs/TEST_COMMANDS.md lists the standard ones). A game starts with the
full-screen help up: a script clicks Continue (320,440) first.

`unlock all` opens the two pilots that have to be earned (Turbot and Vixx: bits 2 and 6 of the profile's value 0x30, with
its check value 0x32 = that plus 0x193). It writes 0x44: a profile is accepted on loading only with 0x13, 0x16, 0x41 or
0x44 there (`AstroPopApp::ValidateUser`), and any other value brings the "invalid setting" box and the reset to 0x13.
The command wrote 0x57 until 2026-11-09, which worked in the run that set it and failed in the next one.

The bot (`move`) plays the weapons too: it clicks the button of the box that explains a weapon (dialog 0x7b), starts a
charged weapon with Return, fires the laser (Vector) at the bottom brick of the tallest column, puts the pointer on that
brick and clicks for the psychic (Vixx; two moves inside the cell, because a move lights its cell only when the pointer's
last position was in the field already) and waits while Sprocket's and Turbot's weapons run by themselves.

A player is two things, and a test that starts with one needs both: the file `users\data<n>.usr` and the registry key
`Users\<name>` with `Users\LastUser` (the original keeps them under HKLM, the port in its `registry.ini`). `Init` deletes
every player and file when the registry's `SGFVersion` is missing or below 15, so a preset carries that value as well
(`tools/origcmp/ini_to_reg.py` writes the three for origrun's `--reg`).

## 4. Reading notes

- Ghidra's `this.NNN` is a hex byte offset; `param_1.NNN{c}` on an `int*` is a **dword index** (check the asm: `.196` was
  +0x658). Stack-passed `std::string` arguments are lost by the decompiler; read the asm for call arguments.
- A listener subobject's `this` is the owner plus 0x58 (MainMenu, dialogs): `this.28.690` in a listener method is
  owner+0x80 -> the app's `mUser`.
- `FUN_004b83f0` is the 2004 `DrawImageCel(img, x, y, cel)`: the cel counts along the rows when the image has more than one
  row, else along the columns; out-of-range cels draw nothing. The port's `Graphics::DrawImageCel` indexes `cel % cols,
  cel / cols`; the game's own helper is used where the two differ.
- Ghidra writes an x87 float compare as `a < b != (a == b)` for **a <= b** (and `a < b == (a == b)` for a > b): the two
  tests are exclusive, so `!=` is an "or". Read as a strict `<` it is wrong exactly when the two floats are equal, which a
  brick that has just been put on its target is. The asm decides (`FCOM; TEST AH,0x41; JP`: JP taken for a > b, so the code
  after it runs for a <= b; `TEST AH,0x1; JZ` taken for a >= b). Brick got five of these wrong at first (the arrival test and
  the 423 test of UpdateState1, the `Listen(0)` test of SetTargetY, the -5 clamp of the wobble, the 9.0 test of the
  mutation timer); the identical-play pass found them by one brick that did not hear event 0 after a retarget.
- **The original's x87 rounds every result to a float** (precision control 24 bits: DirectDraw's setup never preserves the
  FPU, `docs/FRAMEWORK_2004.md`, "3D and the FPU", and the game itself sets it: `AstroPopApp::UpdateFrames` (0x408b20) reads the
  control word with `__control87` and, when the precision is not `_PC_24` (0x20000 in MSVC's own constants; the port's comment
  once read it as 53 bits), sets it before every batch of updates. Every update and every draw therefore runs at 24 bits; only the
  start of the program, before the first UpdateFrames, runs at 53 bits, as the sine table of the app's constructor does). Seen in a run of the original (classic, seed 7, update 8186): the
  wait before the next row is `m19d8 = (int) (m19cc + m930 * 20.0f)` (0x46ec55: `FILD; FLD float; FMUL float; FADDP; _ftol`),
  with `m930` = 2.3f (2.29999995): in a full-precision x87 or in double the product is 45.999999 and the row waits 1045, in
  the original it is rounded to 46.0 and the row waits 1046, so a row came one update earlier. The port computes the
  expressions that the game truncates, compares or chains in `float` (an SSE float operation rounds like the 24-bit x87):
  the wait formula, the danger display (`CheckStackHeight`), the spawn thresholds (`FillTopRow`), the ramp interpolation
  (`UpdateRamp`), the stage bonuses and the speeds of the streaks and particles. What a single operation followed by a store into
  a float field computes is the same either way and was left as it was. The drawing and screen code was read in phase two for the same
  expressions and written in `float` too (positions of the ship, bricks, streaks and texts, the gauge and the bars, the scenery layers, the banner's wave). A weapon case showed one more: `BeginPlay`'s count of the buried bricks a weapon removes,
  `(int) ((1.0f - share) * count)`, was 19 in double and is 18 in the original (classic, seed 7, Sprocket, update 6369: one draw
  fewer). Read for the same kind of expression after it, and changed without a run having shown a difference: the meter's fill
  ratio `m1a14 / m1a18`, compared with a third and two thirds (`BoardFill`) and with 1 (`BoardGone`), the positions on the meter
  that a milestone truncates to whole pixels (`BoardEvents`), and the speed of a warp star (`BoardWarp`).
- A relative path of `XcfFile` (`HasKey`, `GetInt`: 0x412d00 and its neighbours) starts at the section under the cursor, and
  its first part may be the name of that section itself: a bonus level file is read with the paths `LEVEL\ROWn\Brickn` right
  after `Parse`, whose cursor stands on `LEVEL`. The first translation looked for the first part among the children only, found no
  row, and the port's bonus round was the wall of stones alone (classic, seed 11, update 16047: the generator moved by 590
  places in the port and by 231 in the original).
- The Board's byte at +0x4c8, which the pace code (`UpdateStates`, 0x475040) reads to choose the pace after a spurt, is not
  a flag of its own: the siren's `FrameAnim` sits at +0x4a0 and +0x4c8 is its `mStopped` (field +0x28, true from the
  constructor). The first translation had an unset `m4c8` there, so the pace after a spurt was always 8 and the extra
  `PickSpawnDelay` draw of the stopped-siren case (pace 4) never came (found by classic, seed 2, update 7495, where the
  original drew one number the port did not). The siren drawing and the two `m1ab8 = 8` sites of BoardMode read the same field.
- `Brick::UpdateRainbow` (0x4062d0) first erases the colour that was shown from the list (`vector::erase`, 0x405f70) and
  only then draws, when the list is not empty; with none left the brick stops being a rainbow. The first translation drew
  without erasing.
- `tools/ctorstores.py <addr>` lists a constructor's field stores compactly (zero runs folded); the Board's vectors are
  0x10 bytes each (allocator slot, begin, end, capacity).
- The decompiled app offsets that matter so far: +0x628 Board, +0x62c 43 lists (12 bytes each, made by 0x4319b0),
  +0x640 loader screen, +0x644 MainMenu, +0x64c the screen over the selector, +0x658 / +0x6f9 / +0x6fa / +0x6fb flags set from
  the profile by ValidateUser 0x4285d0, +0x684 the player names, +0x690 the current profile, +0x6a8 the descriptor.

- The particle constructor 0x4034a0 takes (speed, sideways speed, rise speed, angle, gravity, height); the call sites pass them in this
  order except the spark emitter of a low brick (`Brick::UpdateSparks`, 0x406e20), which draws the sideways speed first and passes it
  as the second argument and the speed, drawn second, as the first (the asm pushes the second draw last). The port had it the other
  way until phase two, which drew every spark at another place in the shots from the first one (the draws, and so the games, were the
  same). All eight call sites were read against the asm.

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
The small handlers of events 0x10 and 0x11 (0x433cc0, 0x433cd0) are such methods: `INC [ECX + 0x1a04]` is the Board's own +0x1a68
(`m1a68`, the number of fired bricks that sit at their place with their flag +0x188 set), which the spawn of the next row waits
to see at zero (`Board::SpawnControl`). The first translation counted in the Board's +0x1a04 (`m1a04`, a value set from the level)
instead, so a row came one update early after a retarget (found by the first different update of the identical-play pass).
The handler of event 3 (0x403fd0, `Brick::OnDie`) fires the "popped" event 0x1a only for a mutated brick: a brick that is shown and not mutated and is not a plain colour goes to state 3 at once (a plain colour fires event 0x27, a stone, steel or bomb fires its own events in its own handler), and the Board answers 0x1a (0x45d7a0) and 0x1b by setting its mutator hold (`m1b5c`, 2000 updates) when no mutating brick is left. The first translation fired 0x1a for every such brick, so the hold was set and a mutator was refused for 2000 updates where the original made one (found by the first different row of survival, seed 1, update 8496).

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
- Test-only: the `move` bot, `set charge`, `set chargestart` and `set stories` (see 3b).

- **The menu has no More Games panel** (a link out of the game, removed with its button): the three panels left in the right-hand
  column (Hall of Fame, Options, Quit Game) are stretched vertically to fill its place, made when the menu is made from the
  original's own pictures (`MainMenu`: the column, each panel's hover and press pictures and its hit mask, a linear mix of
  the two rows each new row lies between); and the hidden sequence of hovers that shows the ships (Hall of Fame, Hall of Fame,
  Classic, Hall of Fame, Survival, Survival, Hall of Fame, Options, More Games, Quit Game) loses its More Games step, so the
  sequence ends Options, Quit Game. The pictures, sample and string of the button stay in the resource tables (they come from
  the game's own manifest) and nothing reads them.

- The original numbers its sounds by the order of its resource map (by name: `SOUND_1SEC_TIMER` is number 0), the port by the order
  of the manifest. The one place where the original uses a literal number is the box that explains a weapon: the field at +0x118
  of `PowerDialog`, never written after the constructor's 0, is the sample its button plays when it is pressed (0x483b50), so
  that button plays `SOUND_1SEC_TIMER`, which the port asks for by name (`PowerDialog::ButtonPress`). The box's keys (Return,
  Space, Escape) press nothing and play nothing, as in the original.

## 7. Fix candidates (original bugs and waits, kept in the port, for the fixes mod)

- The OK button of the box that explains a weapon plays the one-second timer's tick after its click (the sample number at +0x118,
  section 6): a stray sound. Made: the fixes mod's toggle `weaponBoxTick` (`mods/fixes/mod.json`, read with `FixEnabled` in
  `src/Fixes.cpp`; on in an enabled mod, so the box's OK is silent; with the mod off, as in the port, it ticks).
  `games/astropop/tools/fixes_check.sh` proves it: the bot plays one game with real input (`realplay.py --charge 18000`, so the box
  comes up) with no mods and with the mod, and the sound logs differ by that one play of `sounds/timer1`, the other sounds and the
  game's end the same. That is the whole mod: phases one and two found no other bug or wait that only costs the player time (the
  original's own clock decides nothing in a game's play: the seed is the only thing a run takes from outside). The web page ships the mod on
  (`<game>-fixes/`, cmake/ByopopWeb.cmake).

## 8. Oddities (not chased)

- The menu's hidden hover sequence (Hall of Fame, Hall of Fame, Classic, Hall of Fame, Survival, Survival, Hall of Fame, Options,
  More Games, Quit Game) starts the screen of ships flying off (`ShipsScreen`); the port's sequence has no More Games step (section 6),
  so it ends Options, Quit Game and the screen is reached as before. The tours do not hover that way (their menus are clicked).

- **Phase two, what still differs in the pictures** (found by the tours, judged one by one; the numbers are the largest per-channel
  differences in levels, 0 to 255; `IDENTICAL_PLAY.md` has the counts of every tour):
  - The ring of the atomic explosion (the Atomic Bomb weapon: cels of `IMAGE_ATOMIC_EXPLOSION`, one drawn at twice its size with
    `DrawCelStretched`) differed at its edges by up to 91 levels (w_spr g3700, ck13 g3650 and others): the framework's
    `WidgetManager` draws every widget with the fast stretch on in a 2D run and the 2004 one with it off (only the level-up text
    asks for it, 0x40c8a0). Fixed by the shared option `gWidgetsDrawWithoutFastStretch`, which `AstroPopApp::Init` turns on; those
    shots now differ by at most 7 levels.
  - The small particles and halos (the normal and additive blits of `DDImage`: 0x4ac1d0, 0x4b0f50 `NormalBlt`, 0x4ae3f0
    `AdditiveBlt`) differ by at most 11 levels (emerg1 g22200; 7 elsewhere) where the 2004 arithmetic truncates and the GPU rounds
    (2D mode only; in 3D the card does it): that is most of the shots counted "faint" in the report. The port's 2D screen is the
    GPU's, so matching it takes a software screen and the 2004 arithmetic (about 5,500 lines of x86 in the two functions, 16 and
    32 bit, with and without a colour, transparent or not): shared code, not done, no difference that a person sees.
  - The title screen changes its picture (Vixx, Turbot, ...) every 600 updates of its own count. The port loads at the machine's speed
    (CLAUDE.md: loading is made fast in the port itself), so its title starts at another update than the original's and a shot at a fixed update
    can land on the other picture (the tour `menus` a01_title; the next shot, a02, matches). It is a timing of the loader, not a
    difference of the screen.
  - A shot of the original lands "or-earlier" (`orig.landed.tsv`): when a thing moves quickly (a brick thrown up at the start of a
    game, classic3 k3335) the two shots differ although the game does not; the tour `classic4` takes a shot at every update of the
    same stretch, and all of them match.
  - Web builds run with `fast=1` are not update-deterministic (frames are skipped, so a shot lands at an update a few later): the
    web tours are compared with the native ones at the nearest update. The one difference above 20 levels is the pulse of a
    mutating brick's biohazard mark (alpha plus or minus 3 each update, survend g7600, g7900, g8500, g8600). The Main Menu box's
    "Yes" takes 80 to 100 updates to reach the menu (the box's fade); the web driver waits until it does.

## 9. Next steps

1. The web build and its registrations (page.js, stores, profiles, saveimport, controls, sound tracks), the origrun row.
2. The identical-play pass with `games/astropop/docs/IDENTICAL_PLAY.md`: phase one (plays identical) and phase two (looks
   identical, sound, focus, the web build, real-input driver play) are done; what still differs is listed in section 8 and in the
   report. The stretch of the ring is mended by a shared option (section 8). What remains is the blit arithmetic of the 2004
   `DDImage` (`NormalBlt`, `AdditiveBlt`): at most 11 levels, in shared code, not done (section 8).
   The music (a module stream) was not compared with the original's in phase two: its start, stop and focus handling remain to be
   checked, if origrun can see them.
3. The frozen baseline `build/baseline-astropop` is made (`tools/check.sh astropop`). The fixes mod: one candidate (section 7); the mod
   waits for `docs/MOD_ROADMAP.md`'s turn. The coverage check is done (section 10). Still to do for the game to be done: the performance
   numbers and pass, taking out code the game does not need, the readability pass (`docs/CODE_STANDARDS.md`), the check against the
   PopCap Game Collection's copy, and a night run of the original in 3D for the screen wipe (section 10).
4. (Done: the gap of the removed More Games button is closed, section 6.) The app's destructor does not delete its screens (as the original's).
5. (Done in phase two, the sites it lists and the other drawing and screen functions read against the asm; the float rounding of the drawing
   code follows section 4.) Phase two of the pass: the x87 compares of the drawing and screen code that were not checked in phase one against the asm for the `<` / `<=` idiom of section 4 (0x40c8a0, 0x433500, 0x433920, 0x439c20, 0x444520, 0x461010, 0x461e50, 0x467af0, 0x487f00, 0x48a110 and the other `TEST AH` sites listed by `grep -l 'TEST AH' re/astropop/asm/*.asm`); the gameplay ones (every Brick method, the group and fall code, CheckStackHeight, OnBrickGone) were read in phase one.

## 10. The coverage check (2026-11-11; `docs/COVERAGE.md`, `games/astropop/coverage.txt`)

**The runs.** The original ran on the host under origrun's `--coverage` (`--reg-map popcap`; the PopCap registry key wiped before each
run and the player's profile put back by `reg import`) over three kinds of play: the scripted tours (menus, options and the second
tour of them, halls, users, the second and third tour of the player list, the lost-data start, the quit and pause tours, the ships
screen, the scrolling list), whole games that the port's own bot plays (`origcmp/record.py`, replayed as an origrun script: classic
and survival from the first level, from later checkpoints, with every pilot and a charged weapon, the survival end with every mode
unlocked) and the tours that had been made for the comparison itself. 43 runs between them reach 947 of the 1,383 functions of the
game's own code (0x401000 to 0x48b7a0); the port defines 720 and leaves out 305 on purpose (library templates, compiler-made
destructors, one-line helpers written inline, the framework's functions that the linker placed among the game's, overrides that nothing
calls: each with its reason in `coverage.txt`).

**What it found** (every gap in list one, the functions the original ran and the port had no counterpart for, and each fault found
by reading list two against the export; each was then compared again with the original by a tour):
- Deleting a player: the list of the box of names is rebuilt by the box's own function (0x489ba0, every player in the registry's
  order), not by the list that puts the current player first; `ListDialog::RebuildList` is pure now and each box has its own.
- The Hall of Fame's buttons click when they go down (0x435120), as the screens of the menu and of the characters do.
- A menu button called its listener's mouse-enter and mouse-leave slots twice (the framework's `ButtonWidget` already calls them),
  which made the secret order of hovers fail at its second step: the ships screen of section 8 now starts as in the original
  (tour `ships1`, where the original's script gets `origcmd` lines for the keys that the port's has no button for).
- The ships screen's background wash is half transparent (the original sets the colour's alpha to 0x80 after making it).
- The options screen's two sliders are the 2004 kind (`Slider2004`): a press on the track outside the thumb does nothing and
  `SetValue` does not clamp (`docs/FRAMEWORK_2004.md`, section 8; tour `opts2`).
- The list box's page size is a float's worth: the original's x87 rounds `(height - 8) / line height` to 24 bits, and the port's
  double put the lines of a scrolled list one pixel higher in three positions out of eight (found by the tour `scroll1`: the
  arrows, the track, the thumb and a click on a line, then NEW with ten players, which shows the "too many players" box). `ListDialog::Resize`
  rounds the page size to float and hands it to the scroll bar. Other 2004 games with a `ListWidget` may show the same shift.
- `resgen.py` writes each resource group's function with the original's address, in the original's order (by name), and
  `LoadImageById` (0x420ea0); `resgen.py --check` reads all 44 groups of the exe again against `Res.cpp` (the same resources, by
  name, with the getter of their kind): all 44 agree. The 34 groups that no run loads (the pages of the pilots' stories, the end
  scene, the More Games page) are covered by this check.
- Addresses that named the wrong function (`GetPilotLevel` 0x40e930 was cited for its one-line helper, the Ships screen's and the
  list box's destructors for their deleting wrappers) were corrected, so that the comments and the report agree.
- The tour harness has `origcmd` (a command only the original's script gets), and the tours `users2`, `users3`, `resume1`, `ships1`,
  `opts2`, `scroll1` and `lost1` (a start with players in the registry and no data files: the boxes that say a player's data was lost)
  keep the findings above from coming back.

**Read again** (list two: defined in the port and never reached by any run; each compared with the export function by function, with
nothing found that differs): the Turbot and Sprocket brick events (0x401780, 0x4017a0, 0x4026d0, 0x402670, 0x402710), the overlay of the
Turbot's power-up (0x4022f0), the brick kinds' settle, draw and activate functions (0x403420, 0x403860, 0x403930, 0x402b30, 0x402d20),
the board's `RecordStep` (0x434570), `PulsePair::Update` (0x433850), the mutation counter's event (0x438e10), the piece-brick event
(0x451430), the power brick's done event (0x45e920), the search for a move that is left (`AnalyseSecond` 0x453a50 and `RunTotal` 0x43a960 of `AnyMoveLeft`),
the pause that a full field makes at its start (0x4329c0), the weapon box's key and height (0x483a20, 0x483b90), the dialog's line setters
(0x4840c0, 0x487970, 0x4879d0, 0x4897b0) and the four steps of the 3D screen wipe with its draw and its
check (0x40b340, 0x40b430, 0x40b540, 0x40b5e0, 0x426370, 0x4264a0). The screen wipe plays only in 3D (`RedrawAfterDialog` closes the
board first when the screen is not accelerated) and the original in Windows Sandbox has no Direct3D, so a run that reaches it is a
night run on the visible desktop; it is listed under the next steps.

**Taken out** (2026-11-11, in the pass that removes what the game does not need; no play of the original reaches them and the port's
own code never called them): `Brick::SetSpeedX` and `SetSpeedY` (0x4062a0, 0x4062b0), the fuel cell's `GetColumn` and `GetRow` (0x402a70,
0x402a80: slots of the brick's table that nothing calls), the application's `SetMusicVolumeFloat` (0x408b70, a virtual of the game
that nothing calls), its `HandleCmdLineParam` (0x40e060: its one switch, `-mycrash`, sets a flag that only the crash report reads, so the
port now passes `-mycrash` on like any other argument) and `DoDialog` (0x412360, which no code of the game or framework calls). Each has
a `none` line in `coverage.txt`. Left in: the fuel cell's `SetFiredFromShip`, which `BoardInput.cpp` calls.

## 11. Performance (2026-11-11; `docs/COORDINATION.md`, "Performance pass")

**Method.** `games/astropop/tools/profile/perf.py` and `pairs.sh` (as Atomica's and Typer Shark's): the port and the original play the
same game turn about, one process at a time. The game is `scenario/classic11.json`, the inputs of a recorded classic game (seed 11: the
title's link, the name, the mode, the pilot, then 60 moves, 5,636 updates), replayed by the port as `-realinput` lines and by origrun
as script lines; the run was checked to play the recorded game (the same score watch). "The stretch" is the 2,000 updates before the last
input (20 s of play). Every run samples the game's process every 0.1 s (working set, private bytes, processor time) and records how many
game processes of other sessions ran at its start and end: the machine was never quiet (0.4 to 0.65 of it busy, 1 to 6 other game
processes), so a figure is a median of three, with its range. Real time is `PERF_CLOCK=real` (the port without `-fast`, origrun on its
real clock: what a player's machine does); without it neither waits (the port's `-fast`, origrun's virtual clock: the work a test run
does). The load is the time from the creation of the process to the title taking input (the port asks `state`; the original's
loading-thread flag is read out of its memory). Memory is private bytes in MiB.

| Three pairs, 2026-11-11 | Original | Port |
|---|---|---|
| Load to the title | 0.94 to 1.33 s (median 1.22) | 0.93 to 1.07 s (median 1.02) |
| Private bytes at the title | 68.6 to 73.3 | 93.5 to 98 |
| Real time, the stretch's processor time (20 s) | 3.70 to 4.31 s (median 3.72) | 1.83 to 2.23 s (median 1.84) |
| Real time, the whole run (58 s) | 6.33 to 7.09 s | 4.05 to 4.48 s |
| Updates a second in the stretch (real time) | 100 | 99.4 to 100.0 |
| Peak private bytes of the run | 81.7 to 84.4 | 147.9 to 150.6 |
| No waiting, the whole run's processor time | 5.03 to 5.83 s | 1.47 to 1.56 s |
| No waiting, the stretch (2,030 updates) | 2.84 to 3.48 s | 0.11 to 0.12 s (7,500 to 8,000 updates a second) |
| Peak private bytes of the run without waiting | 82.3 to 85.7 | 161 to 165 |

The port loads about as fast as the original and plays on about half of its processor time (a tenth of a core in real time); the one
cost it has more of is memory, 65 MiB more at the peak.

**Where the port's processor time goes** (`games/bejeweled/tools/profile/cpuprof.py`, cycles by module and function). The run without
waiting (4.31 G cycles, the load and 2,030 updates) is almost all load: ntdll 41% (file and memory system calls), the exe 28%, win32u 9%,
`imagehlp.dll` 5%, the C library 4.8%, the Nvidia driver 4.4%, `Windows.Storage.dll` 3.4%. Of the exe's 28%: the JPEG decoder (`_ycc_rgb_convert`
5.0%, `_jpeg_idct_islow` 2.5%, the Huffman steps another 3%), `Sexy::Quantize8Bit` 4.7% and the GIF decoder `_stbi__out_gif_code`
1.8% (the GIFs are decoded to 32 bits and quantized back to 8), the Vorbis decoder of the sounds (`_inverse_mdct`, `_vorbis_decode_packet`,
`_decode_residue`: about 2.5% together), `Platform::DecodeImage` 1.5%. The game's own code (`Brick::Retire`, `Board`, the drawing)
shows at 0.1% each: two thousand updates cost 0.12 s.
In real time (one run over the scenario, 17.0 G cycles in 59.6 s sampled, 285 million a second over every thread) the exe is 6.1% of the
cycles, ntdll 78% (system calls and waits: the frame's presentation and the sleeps), win32u 5.6%, the Nvidia Direct3D 11 driver 4.9%,
`imagehlp.dll` 1.7%. The main thread has 53% of the cycles (69% of them in ntdll, 11% in the exe) and the driver's own thread 16%; nine more
threads of 2 to 3% each are all waits. Inside the exe the biggest functions are still the load's decoders (`Quantize8Bit`, `_stbi__out_gif_code`,
the JPEG and Vorbis decoders, 0.2 to 0.6% each); what play costs is `Board::DrawFrame` and `ImageFont::DrawStringEx`, 0.1% each. What a
player's machine spends on this game is the display path, not the game's code, and the original's DirectDraw path spends more of it.

**Where the port's memory goes.** The framework's `memreport` command (a game command in a `-realinput` script) at the title, the main
menu, a game just begun and the game over (`build/logs/ap/perf/mem`, scratch): 352 pictures at the title (38.1 MB of 32-bit pixels, 4.2 MB
of palettes, 20 of them with a texture, 3.3 MB), 570 at the end (49 MB of pixels, 209 textures, 16.7 MB); the sounds are 73 (11.6 MB of
samples, all at the title); the framework's heap is 60.1 MB at the title, 71.1 MB in a game and 75.2 MB at its end (private bytes
96.6 at the title, 126.8 MB at the end of the game). What the game itself loads when a game starts: the background `nebula2` (640x960),
`help_bg` twice, about 25 pictures its code makes (mirrors, the laser, the galaxy) and the textures of the ones the board draws. The
original's private bytes are 71 MB at the title and 82 to 86 MB at the peak, so the port holds about 22 MB more at the title and
65 MB more at the peak. The title's difference is what Typer Shark found (its section 11): the Direct3D 11 device, 20 to 25 MB.
The rest of the peak is the start of a game, where private bytes rise from 98 MB to a peak of 158 MB within three seconds (about 38 s
into the run) and fall back to 132 MB: the heap grows by 9 MB and the textures' estimate by 10 MB, so most of the rise is below the game's
heap, in the display path, the same as Bejeweled Twist's jump at a level-up (`docs/COORDINATION.md`).

**Shared costs** (none of them is in this game's code; sent to the coordinator in one message):
1. *The load's decoders*, as in Typer Shark: `Quantize8Bit` and the GIF decoder together 6.5% of a run's cycles, 0.15 to 0.2 s of the load.
2. *`imagehlp.dll`*, 5% of the cycles of a run without waiting, from a call site not yet found (Typer Shark 7.6%).
3. *The Direct3D 11 device and the pixels the framework keeps after upload* (the 22 MB the port holds more at the title).
4. *The start of a game raises private memory by 60 MB for a few seconds* (AstroPop, in 2D, 2026-11-11): the same shape as Twist's
   level-up jump, below the heap. To find it: count texture creations in the display layer during the first second of a game
   (`astropop.exe -realinput=<script> -seed=11` with `memreport` before and after the first board).

**Nothing was changed in the game's code for speed.** The port already does less than the original on every figure but memory; the only
AstroPop-side memory it could save is by loading fewer pictures at the start of a game or freeing the ones that were drawn once (the
`nebula` backgrounds), which the original does not do and which would change what a game loads, so it was left as it is. The frozen
baseline (`tools/check.sh astropop`) is unchanged by this pass: it added only the profile tools and notes.

**To repeat.** `bash games/astropop/tools/profile/pairs.sh <out dir> [pairs]` from the worktree root (about 3 minutes a pair when the
machine has a free slot; it waits for one), `BEFORE_EXE=<name>` adds a run of an exe kept in `build/native` from before a change; the
memory split is the standard `memreport <file>` test command (`docs/TEST_COMMANDS.md`) put into a `-realinput` script at the points
wanted. The raw samples were deleted once these figures were written.
