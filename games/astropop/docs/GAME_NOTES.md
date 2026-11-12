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
last position was in the field already) and waits while Sprocket's and Turbot's weapons run by themselves. It names the weapon's
key as a DOM key does (`Enter`): the web page's test kit throws on SDL's `Return`, and the native driver maps `Enter` to it.

A classic game cleared level by level with `win` and the bot (the Splits window's Any% check: 32 level clears in one game) needs the
bot to answer everything the clears bring. After a `win` the level-up and the warp flight take about 700 to 1000 updates, during
which the bot waits (`busy` 2, 3 and 5); then it clicks the gate (`gate`), and the boxes on the way (`upgrade`, the stories,
the hints) with their own buttons. A bonus round (the end of levels 3, 7 ... 31) starts its minute only with a click (`mBonusRoundStarted`,
`MouseDown`): the bot makes it (`bonus`), and with no group to shoot in the bonus field it waits instead of answering null, as the
minute runs out by itself (a driver stops at a null). Without those two answers a game stopped in the bonus round of level 7 (native,
null) or 19 (web) and one stayed in level 23's bonus round for good. `games/astropop/tools/livesplit/scripted.py levels` (native)
and `games/astropop/tools/web/winlevels.js` (the page, real DOM input) play it: 32 clears and level 33 on the screen, in 4,620 moves
and 84,700 updates natively and 1,900 moves and 95,000 updates in the page, with 24 upgrade boxes, 8 bonus starts and the
gate 32 times.

A game that a test command starts while the full-screen help of the one before is up (`goto classic`, `goto survival`, `level`; the
original's player cannot, the help is modal) left the help under the new Board: `StartClassicGame` deletes the old Board and adds the new
one over the help widget, so the help is not drawn, takes no click, and `mHelpScreen` stays set, which holds the Board's updates and
its input. The bot then answered "help" for good (click on a button that nothing receives), also after `win`; a driver that
restarts games over an open help (the Site session's page did) met it. The three commands close the help first
(`CloseHelpBeforeGame` in `testhooks/TestCommands.cpp`, test code only). `games/astropop/tools/web/gotohelp.js` repeats it: three
restarts with the help up, then 90 bot moves with a `win`; one "help" answer, then shots, the gate and level 2 (before: 20 of 20
answers "help").

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
  wait before the next row is `mRowTimer = (int) (mRowInterval + mStackDanger * 20.0f)` (0x46ec55: `FILD; FLD float; FMUL float; FADDP; _ftol`),
  with `mStackDanger` = 2.3f (2.29999995): in a full-precision x87 or in double the product is 45.999999 and the row waits 1045, in
  the original it is rounded to 46.0 and the row waits 1046, so a row came one update earlier. The port computes the
  expressions that the game truncates, compares or chains in `float` (an SSE float operation rounds like the 24-bit x87):
  the wait formula, the danger display (`CheckStackHeight`), the spawn thresholds (`FillTopRow`), the ramp interpolation
  (`UpdateRamp`), the stage bonuses and the speeds of the streaks and particles. What a single operation followed by a store into
  a float field computes is the same either way and was left as it was. The drawing and screen code was read in phase two for the same
  expressions and written in `float` too (positions of the ship, bricks, streaks and texts, the gauge and the bars, the scenery layers, the banner's wave). A weapon case showed one more: `BeginPlay`'s count of the buried bricks a weapon removes,
  `(int) ((1.0f - share) * count)`, was 19 in double and is 18 in the original (classic, seed 7, Sprocket, update 6369: one draw
  fewer). Read for the same kind of expression after it, and changed without a run having shown a difference: the meter's fill
  ratio `mStageBricksCleared / mBricksToPass`, compared with a third and two thirds (`BoardFill`) and with 1 (`BoardGone`), the positions on the meter
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
  original drew one number the port did not). The siren drawing and the two `mPhase = 8` sites of BoardMode read the same field.
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
(`mSupportedBrickCount`, the number of fired bricks that sit at their place with their flag +0x188 set), which the spawn of the next row waits
to see at zero (`Board::SpawnControl`). The first translation counted in the Board's +0x1a04 (`mSlowGateStage`, a value set from the level)
instead, so a row came one update early after a retarget (found by the first different update of the identical-play pass).
The handler of event 3 (0x403fd0, `Brick::OnDie`) fires the "popped" event 0x1a only for a mutated brick: a brick that is shown and not mutated and is not a plain colour goes to state 3 at once (a plain colour fires event 0x27, a stone, steel or bomb fires its own events in its own handler), and the Board answers 0x1a (0x45d7a0) and 0x1b by setting its mutator hold (`mMutatorCooldown`, 2000 updates) when no mutating brick is left. The first translation fired 0x1a for every such brick, so the hold was set and a mutator was refused for 2000 updates where the original made one (found by the first different row of survival, seed 1, update 8496).

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
  section 6): a stray sound. Made: the fixes mod's toggle `weaponBoxTick` (`mods/fixes/mod.json`, read with `Platform::FixOn` in
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
  - **The launch pad's glow in 3D** (2026-11-12, the wipe comparison, section 10): the glow and sparks above the launch pad of the main
    menu (x 216 to 417, y 383 to 443) differ by at most 25 levels between the original on its Direct3D and the port on SDL's, in every frame of
    four runs that shows the menu (180 frames, about 350 to 480 pixels once the menu is uncovered). The card does the blending on both
    sides here, so it is another blend or another rounding of the same additive halos; no one sees it (shotdiff counts it faint). Not chased.
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
   The music (a module stream) was compared with the original's after phase two (section 14): its song starts are the same; its pause and
   resume at a focus change cannot be seen in origrun's sandbox.
3. The frozen baseline `build/baseline-astropop` is made (`tools/check.sh astropop`). The fixes mod: one candidate (section 7); the mod
   waits for `docs/MOD_ROADMAP.md`'s turn. Done since: the coverage check and the removal of code the game does not need (section 10), the
   performance pass (section 11), the readability pass (section 12) and the check against the PopCap Game Collection's copy (section 13: its exe is
   another build; the port plays equal on its files) and the music's state against the original's (section 14: the port had no music; fixed).
   The original's 3D screen wipe was reached on the night of 2026-11-12 and compared frame by frame with the port's, all four kinds: the same
   pictures (section 10). The performance step is open: in 3D the port is over its original (134% at the original's window size,
   section 11c; all of it the presentation and the Direct3D 11 driver's thread, the game's own update and drawing being at or under the
   original's; 105 to 121% in the later set with the presents held by the frame loop's pace or by vsync, section 11d, the present and
   driver costing the port 1.9 to 2.4 M cycles a present against the original's 1.2, a shared cost); 2D passes (69%). The lite survey (section 11b) found no candidate worth a toggle: the game's logic is 0.6% of the process
   and every faster method keeps the answer.
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
(0x4840c0, 0x487970, 0x4879d0, 0x4897b0). The four steps of the 3D screen wipe with its draw and its check were on this list until the
night of 2026-11-12 (the next paragraph).

**The 3D screen wipe** (2026-11-12, the night run, on the visible desktop). The wipe plays only in 3D (`RedrawAfterDialog` closes the
board first when the screen is not accelerated), the original in Windows Sandbox has no Direct3D, and dgVoodoo's wrapper failed on it, so
`games/astropop/tools/origcmp/visrun3d.py` runs a copy of the staged original on the desktop (22:00 to 08:00 only: `Is3D` 1 in the
registry key, put back after, the script's input posted to the window, `--coverage` on). The wipe's kind is drawn when a board is made
(`Board::Board`: `AstroRand() % 500000000 % 4`; 0 clears pixels at random, 1 moves an edge to the left, 2 to the right, 3 clears rows), so
one run shows one kind, and `games/astropop/tools/origcmp/wipeseeds.py` runs the same tour once for each seed (it patches the seed bytes of
origrun's `seed` patch in the copy) until the coverage has reached all six functions. Seed 1 gave the right edge (0x40b430), seeds 2, 4, 7,
8 and 10 the random clearing (0x426370), 3, 5, 6, 9 and 12 the left edge (0x40b340), 11, 13 and 14 the right edge again, and seed 15 the rows
(0x40b540); the draw (0x40b5e0) and the check (0x4264a0) ran in every one. **All six functions are reached.** The port's four steps, its
draw and its check were read against the export before (nothing differed).

**The wipe, frame by frame** (the same night). The same seed gives the same kind in the port, so `origcmp/wipe.sh <seed>` runs the chain for
one kind: a classic game of the in-game tour (`tour/ingame1.txt` up to its last move), Escape, the options box's Main Menu button and the
End Game box's Yes, which starts the wipe, then a shot every few updates (`wipeshots.py`). The original runs on the desktop on the real clock
(`visrun3d.py --shots`, its window at the top right; 22:00 to 08:00); each input lands on the update it was asked for and each shot is
written with the update it landed at, a shot waiting until the original's draw flag (`[app]+0x488`, found by `probe3d.py`: the one field of
the app object that reads 1 for a moment after each update, 0 when it is drawn) says the last update was drawn and reading the client area
off the screen while the game is held 30 ms. The port's script puts the same inputs at the same
updates (the tour's own offset for each kind of input) and shoots **every** update (`-fast -drawevery`, rundesk, `Is3D` 1, the same seed), and
`wipematch.py` matches each of the original's pictures with the best of the port's pictures within four updates, so that the unknown part
of a real-clock shot (a picture shows the frame of the update it was taken at or of one or two before it) does not matter: the wipe is
monotonic, so the best match is the same state, and what is judged is how small the difference is there and that the offset does not drift
over the wipe (a wipe that ran at another speed would). The offsets were 0 or -1 update in 172 of the 180 frames of the four runs; the other
eight are the first frame of each run (the board as it was before the wipe moved, which the port's range of shots starts on) and four where
the screen showed a frame seven to nine updates old (the same picture as the shot before it: the screen had not changed since, the
compositor's doing and not the game's; the next shot is at 0 again). Seeds 1 (right edge), 2 (random), 3 (left edge) and 15 (rows) gave 180 frames, 58 of them with the
wipe under way (12 to 17 a kind): **no pixel differs visibly** (shotdiff: 17 frames equal, 163 "only faintly", none differing). Everything
that differs, at most 25 levels, lies in one place, x 216 to 417 and y 383 to 443, the launch pad's glow of the main menu that the wipe
uncovers, and is as large in the frames after the wipe
(section 8, "the launch pad's glow in 3D"); what is left out of the comparison is the menu's right column, where the port has no More
Games button (section 6), and the window's bottom corners, which Windows 11 rounds. The inputs and the draw counter: `visrun3d.py`'s
header. The wipe is therefore the original's in its picture, its speed and its start, for all four kinds, in 3D on the desktop; its start
to within the one update that a real-clock shot cannot resolve (the first frames match at offset 0 or -1, and a start two updates apart
would put the edge twelve columns or the random pixels four thousand apart).

**Taken out** (2026-11-11, in the pass that removes what the game does not need; no play of the original reaches them and the port's
own code never called them): `Brick::SetSpeedX` and `SetSpeedY` (0x4062a0, 0x4062b0), the fuel cell's `GetColumn` and `GetRow` (0x402a70,
0x402a80: slots of the brick's table that nothing calls), the application's `SetMusicVolumeFloat` (0x408b70, a virtual of the game
that nothing calls), its `HandleCmdLineParam` (0x40e060: its one switch, `-mycrash`, sets a flag that only the crash report reads, so the
port now passes `-mycrash` on like any other argument) and `DoDialog` (0x412360, which no code of the game or framework calls). Each has
a `none` line in `coverage.txt`. Left in: the fuel cell's `SetFiredFromShip`, which `BoardInput.cpp` calls.

## 11. Performance (2026-11-11 and 12; `docs/COORDINATION.md`, "Performance pass" and "The target")

**Method.** `games/astropop/tools/profile/perf.py` and `pairs.sh`: the port and the original play the same game turn about, one process
at a time, the whole alternating loop under `tools/pincore.py` (one performance core of its own; `profile/pin2.py` takes two). The game is
`scenario/classic11.json`, the inputs of a recorded classic game (seed 11: the title's link, the name, the mode, the pilot, then 60 moves,
5,636 updates), replayed by the port as `-realinput` lines and by origrun (2D) or `origcmp/visrun3d.py` (3D) as script lines; the run was
checked to play the recorded game (the same score watch). "The stretch" is the 2,000 updates before the last input (20 s of play).
**Processor time is the sum of every thread's `QueryThreadCycleTime`** (a thread that has ended keeps its last count;
`QueryProcessCycleTime`, read beside it, agrees to 1%), sampled every 0.1 s and given in G cycles (2.1 G cycles are a second at the nominal
clock). The port's SDL audio thread, its mixer, is counted apart (`audio_g`: 0.2 to 0.3 G of a stretch, 0.7 to 0.9 G of a run; the
original under origrun's stand-in mixes nothing). Real time is `PERF_CLOCK=real` (the port without `-fast`, the original on its real
clock: what a player's machine does). **2D** is run as every game's is, under rundesk (the port; the original through origrun's copy,
`Is3D` 0). **3D** is run on the visible desktop, 22:00 to 08:00 only (`PERF_3D=1 PERF_DESKTOP=1`: a window that takes no focus and SDL's
silent audio driver; the original through `perf.py orig3d`, which starts a copy of the staged original with `Is3D` 1 in the registry, put
back afterwards, reads the 3D flag out of its app object and posts the scenario's input to its window). The port's minutes used
`-realinput` and not `-commands` (the live command poll costs the port a `stat()` every update that the original never pays); the load runs
go through `playbymove`, which does use `-commands`. The machine was never quiet: three to six other sessions' games ran during every
run (0.5 to 0.9 of the machine busy), and the stretches of one build spread by plus or minus 30%, so a figure is the median of five or six
pairs with its range. The memory is private bytes in MiB; the load is the time from the creation of the process to the title taking input.

**Window state of these figures (found 2026-11-12, after the Mahjong session's report; they are to be measured again).** The port's runs had
no `ScreenMode` in their `registry.ini`, and the port then starts full screen: a `screenshotfull` in a run under rundesk (a 3840x2160
desktop), 2D and 3D, is 2560x1920, a render scale of 4 on the game's 640x480, where the original ran windowed at its own 640x480 (scale
1). With `ScreenMode=0` the port's window is 1280x960 (scale 2, the framework's default window). So the tables below set a full-screen
port against a window of the original's size: not like for like. Their renderers were Direct3D 11 in every rundesk run and, on the desktop,
Direct3D 9 for the first 3D rows and Direct3D 11 with the hint (below). The 3D desktop runs very probably started full screen too (the
visible desktop is also 3840x2160), which the display line of the next runs, with `-windowscale=1` and `ScreenMode=0`, will show.

| 2D, five pairs, rundesk, 2026-11-11 | Original | Port |
|---|---|---|
| The stretch, G cycles (median) | 7.22 | 5.78 (80%) |
| Load to the title | 1.51 s | 1.22 s |
| Peak private bytes of the run | 81 to 83 | 159 to 164 |

| 3D, six pairs a row, the desktop, 2026-11-12: the stretch in G cycles | Original | Port |
|---|---|---|
| SDL's own renderer choice (Direct3D 9 on the desktop), one core | 3.50 3.51 3.84 4.18 4.49 4.50 (median 4.01) | 2.81 2.98 4.54 4.70 4.84 4.88 (4.62) |
| With the Direct3D 11 hint, one core | 3.41 4.07 4.31 4.51 4.84 4.95 (4.41) | 4.00 4.36 4.88 4.95 5.46 5.61 (4.92, of which the mixer 0.2 to 0.3) |
| With the hint, two cores (`pin2.py`) | 3.11 3.30 4.87 5.40 5.43 5.82 (5.13) | 5.00 5.05 5.45 6.75 6.89 6.92 (6.10) |
| Load to the title (3D) | 5.1 to 13.5 s (median about 7.5) | 0.85 to 1.5 s |
| Peak private bytes of the run (3D) | 116 to 118 | 159 to 164 |

**The 3D result is that the port is over its original, by 5 to 25% in every set** (the pair-by-pair differences' median is +0.43 G, +0.33
G and +1.23 G, +0.2 G less with the mixer counted apart), with a noise the same size as the difference, so none of the three sets
proves more than "about equal, a little over". 2D passes with room. The cause is not the game's code (below) but the display path that every
game of the framework shares; what could be done about it is listed under the shared costs, and the Direct3D 11 hint (the shared commit
`4ab9c0a0`) is the one change made.

**Which renderer.** SDL's own first choice on a Windows desktop is Direct3D 9 (the NVIDIA `nvd3dum.dll` driver, a second thread busy in
system calls: about 35% of the process). Varying `SDL_RENDER_DRIVER` for three runs each, back to back on one core (the stretch, G
cycles): Direct3D 9 5.25, 5.91, 5.44; Direct3D 11 3.84, 3.88, 3.71; OpenGL 4.83, 4.70, 4.92 (the whole runs 15.2 to 16.6, 11.5 to 13.0 and
13.5 to 15.1); the 28 frames of a tour of the same play are equal on all three but one, where 15 pixels differ by one level on Direct3D 9.
**Under rundesk the game never had Direct3D 9**: asked for `direct3d` on the private desktop it gets `direct3d11` (the new `Renderer:` line
of the log says so; opengl and software are honoured), so every rundesk figure of this pass and of the other games was taken on Direct3D 11,
while a player's desktop got Direct3D 9. With the hint both are Direct3D 11. The hint costs 7 MB at the title (93.5 MB on Direct3D 9, 100.5 on
Direct3D 11; the peak of a minute +3 to +5), no load time (1.2 s either way, three runs each, 2D and 3D) and no 2D time (the 2D stretch
on the desktop: 5.99 to 6.18 G on Direct3D 9, 4.08 and 4.16 G on Direct3D 11, one run disturbed at 8.21).

**Where the port's time goes** (an instrumented build, not kept: `QueryThreadCycleTime` around the main loop's parts and counts of SDL's
calls; one run, Direct3D 11, 3D, one core). The main thread used 8.24 G in the run: the load about 2.5 G; `SDL_RenderPresent` 3.76 G (2,782
frames, **1.35 M cycles each**, with the draws SDL has queued, the swap chain's `Present(1)` and the driver's wait for the display); drawing the scene
1.05 G (80 `SDL_RenderGeometry` calls and 41 fill calls a frame: the game's own draw code is in it); `ProcessEvents` 0.44 G (9,029 calls of
48 K cycles); the updates 0.37 G (65 K cycles an update, 98 a second); the sleeps nothing. The driver's own thread used 3.8 G, the mixer
0.8 G (SDL's silent driver still runs the callback). Over the minutes of one run the process costs 0.16 G a second at the menus and 0.17
to 0.25 in play, so it is a fixed cost per frame, not the game's logic: updates and drawing together are about a seventh of the process,
and presenting (the main thread's part and the driver thread's) is about 70% of the stretch. With vsync switched off (`SDL_RENDER_VSYNC=0`)
a present cost 0.52 M cycles on the main thread, but the game then draws on every update (5,080 frames) and the driver's thread takes the
difference (5.2 G): the whole run took 14.1 G against 13.1 G. The original does not wait for the display in a window (`mWaitForVSync` is false by default), so it may draw more
often at a lower cost each. `cpuprof2.py` (the stack-scanning sampler) and `profport.py` are in `tools/profile`; their shares are a
guide only, since a thread that waits is charged what it used before the wait.

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

**Shared costs** (none of them is in this game's code; sent to the coordinator in two messages, 2026-11-11 and 12):
1. *The load's decoders*, as in Typer Shark: `Quantize8Bit` and the GIF decoder together 6.5% of a run's cycles, 0.15 to 0.2 s of the load.
2. *`imagehlp.dll`*, 5% of the cycles of a run without waiting, from a call site not yet found (Typer Shark 7.6%).
3. *The Direct3D 11 device and the pixels the framework keeps after upload* (the 22 MB the port holds more at the title; the device alone is 7 MB
   over Direct3D 9's).
4. *The start of a game raises private memory by 60 MB for a few seconds* (AstroPop, in 2D, 2026-11-11): the same shape as Twist's
   level-up jump, below the heap. To find it: count texture creations in the display layer during the first second of a game
   (`astropop.exe -realinput=<script> -seed=11` with `memreport` before and after the first board).
5. *SDL's first renderer on a Windows desktop is Direct3D 9, which rundesk's private desktop cannot start* (see "Which renderer"): the
   `shared:` commit `4ab9c0a0` asks for Direct3D 11 first (a default-priority hint, so `SDL_RENDER_DRIVER` still decides), logs the renderer's
   name once and rewords the scale-mode comment. It changes every DDInterface game's renderer on the desktop, so it is the coordinator's to merge.
6. *The framework presents in step with the display* (`InitDDInterface` passes `vsync = !mFastMode`), and a present costs 1.35 M cycles on the
   main thread and about 1.4 M on the driver's: 70% of the port's 3D stretch, against an original that, with no wait for the display in a window, may draw more often at a lower cost each.
   What might lower it, to be tried and judged against the other games' shots: a window that presents without the display's wait (0.52 M cycles a
   present) but draws no more often than the display refreshes, keeping the wait for full screen, where its absence would tear; fewer state
   changes in a frame (80 geometry and 41 fill calls a frame, one draw per change of texture); and `ProcessEvents` (48 K cycles a call, called
   more often than there are updates) and `UpdateDisplaySize` (asked every frame), which are small.

**Nothing was changed in the game's code for speed.** The port already does less than the original on every 2D figure but memory, and in 3D
what is over is the shared presentation path. The only AstroPop-side memory it could save is by loading fewer pictures at the start of a game
or freeing the ones that were drawn once (the `nebula` backgrounds), which the original does not do and which would change what a game loads,
so it was left as it is. The frozen baseline (`tools/check.sh astropop`) is unchanged by this pass: it added the profile tools and notes and,
in the framework, the renderer hint and a log line.

**To repeat.** 2D: `bash games/astropop/tools/profile/pairs.sh <out dir> [pairs]` from the worktree root (about 3 minutes a pair when the
machine has a free slot; it waits for one, and runs itself under `pincore.py`), `BEFORE_EXE=<name>` adds a run of an exe kept in
`build/native` from before a change. 3D (22:00 to 08:00, the desktop): the port is `PERF_3D=1 PERF_DESKTOP=1 PERF_CLOCK=real python -I
games/astropop/tools/profile/perf.py port-minute <dir>`, the original `python -I games/astropop/tools/profile/perf.py orig3d <dir> 1`
(`... 1 load` for its load), turn about under `pincore.py`; `PERF_ENV="SDL_RENDER_DRIVER=opengl,..."` sets variables for the port. Each
result line carries `cycles_g` and `audio_g` of the stretch, the `top_threads`, the 3D flag of the original and the graphics libraries the
port loaded. The memory split is the standard `memreport <file>` test command (`docs/TEST_COMMANDS.md`) put into a `-realinput` script at the
points wanted. The raw samples were deleted once these figures were written.

### 11b. Lite candidates (2026-11-12; `docs/MOD_ROADMAP.md`, "The lite mod")

**Result: none is worth a toggle.** AstroPop's game logic costs about 0.6% of the processor time of a stretch of play, and every faster
method found for the searches in it gives the same answer in every state, so each is an exact change, not a lite one; the gain of the best
of them is two parts in ten thousand of the process, far below what a pair of runs can resolve. The step is not applicable to this game.

**What was measured.** A temporary build of the port (an `rdtsc` count around each function below, one thread, the counts written out at
the end; reverted, it is in no commit) played the performance pass's scenario `classic11` (seed 11, `-fast`, under rundesk, 2D): 2,477
updates of `Board::Update`, 249 draws. The figures are cycles (TSC, 2.112 GHz nominal). For scale, the whole process costs about 2.9 M
cycles an update in the 2D stretch (5.78 G cycles for 2,000 updates, section 11), and the 3D stretch costs more. The start of a game costs
three single calls that are not per-update work and are left out of every mean below: a `Board::Update` of 42.5 M cycles (the level is
built inside it), a `UpdateStates` of 15.9 M and a first `Board::Draw` of 16.8 M.

*What runs every update.* Everything the board does in a quiet update, set against the 2.9 M of the process:

| Function (original address; port file) | Cycles an update | Where it goes |
|---|---|---|
| `Board::Update` as a whole (`BoardUpdate.cpp`) | 16.7 K (0.6%) | the rest of this table, and the lists of effects and texts it steps |
| `UpdateBricks` (0x46e9b0; `BoardBricks.cpp`) | 6.7 K | 36.7 `Brick::Update` calls an update at about 135 cycles each (the 190 cells of the field are walked, an empty one costs a test; the falling, fired, taken and faded lists are stepped) |
| `SpawnControl` (`BoardUpdate.cpp`) | 0.9 K | the quiet-field test and the spawn clock |
| `UpdateScenery` (0x43a1f0; `BoardCore.cpp`) | 0.4 K | the layers, stars and streaks of the background (the drawing of them is not here) |
| `UpdateStates` (0x475040; `BoardTick.cpp`) | 0.4 K | the timers, banners and fades |
| `UpdateAnimCounters` (0x433500), `UpdateExplosion`, `IsSettled` (0x43d2f0), `SweepSprockets`, `SweepTurbots` | 25 to 42 each | counters and flag tests |

`Board::Draw` is not in this list: it is the picture (0.62 M cycles a draw in this run), and a lite change keeps the look (MOD_ROADMAP), so
it is outside the mod. Its cost and the present path under it are section 11's.

*The searches and scans, which run on events (a piece landing, a clear, a fall, a new row) or while the field is quiet.*

| Function (original address; port file) | Calls | Mean, worst (cycles) | Total | What it does, the faster method, and whether it can differ |
|---|---|---|---|---|
| `AnyMoveLeft` (0x459bc0; `BoardMoves.cpp`) | 127 | 3.6 K, 9.4 K | 0.45 M | The original's counting argument for whether a group can still be made: two walks of the field, the runs and spots of each colour, and `Partition`, each part of it calling `CollectConnected` with a limit. It is a pure function of the field, the hand, the set size and the moving bricks. Memoised on a version number that any change to those bumps, it gives the same answer. **Exact.** (A real search of the moves would give other answers where the argument over-counts, but it would be slower, not faster.) |
| `CollectConnected` (0x450a30; `BoardFill.cpp`) | 240 outermost | 0.8 K, 7.0 K | 0.20 M | A recursive flood that tests each new cell against `mConnectedCells` by a scan (at most 190 cells) and against the skip list by `HasCell`. A visited grid in place of the scans, with the same direction order, gathers the same cells in the same order, also when `theLimit` cuts the flood short. **Exact.** A breadth-first flood would gather another set of `theLimit` cells at a cut, so it would be lite; the gain would be nothing and it was not tried. Part of this cost is inside `AnyMoveLeft`'s. |
| `DropUnsupported` (0x46cc20; `BoardBricks.cpp`) | 74 | 1.2 K, 24 K | 0.09 M | Sorts the emptied cells, scans each column for what has lost its support and puts each falling group before the first group of its column that starts higher, by scans of the list of groups. A per-column pass keeping that insertion order is **exact.** |
| `IsInFallingGroup94` (0x438a30), `IsInGroups` (0x4389c0), `HasCell` (0x438d60), `SortFallingGroups` (0x46cfd0; `BoardBricks.cpp`, `BoardCore.cpp`, `BoardGone.cpp`) | 22, 22, 124, 11 | under 3 K | 0.04 M | Linear scans of the lists of falling groups and cells (a few tens of bricks) and an insertion sort of the groups. A set or a grid answers the same; the insertion sort keeps equal heights in order, and any other sort would have to as well. **Exact.** |
| `NoRoomLeft` (0x438b60) and `FindBottomRow` (0x437590; `BoardCore.cpp`) | 282, 2,278 | 0.9 K, 8.2 K; 84, 3.7 K | 0.24 M, 0.19 M | `NoRoomLeft` calls `FindBottomRow` for the same column three times and then counts the filled cells of every column. One call, and a fill count kept per column, give the same value. **Exact.** |
| `CountMovingBricks` (0x439b70; `BoardCore.cpp`) | 2,698 | 43 | 0.12 M | A scan of the falling groups to count bricks not yet at their targets. A count kept up to date at each fall would give the same number, with the risk of drifting from it; it saves 43 cycles a call. **Exact.** |
| `OnBrickGone` (`BoardGone.cpp`) | 87 | 11.6 K, 196 K | 1.0 M | The handler of a brick's end: score, meters, effects, the checks for a cleared field and no move left. It is no search; its cost is in creating the effects. Not a candidate. |

The searches taken together cost about 1.4 M cycles in the 2,416 updates of the run (the overlap of `CollectConnected` and `FindBottomRow`
with their callers counted once), 0.6 K an update, against 2.9 M an update for the process.

**The one change with a visible gain.** `UpdateBricks` is 6.7 K an update (0.23% of the process), almost all of it in `Brick::Update` calls
on bricks with nothing running. A skip for those bricks is exact only if an idle `Brick::Update` changes nothing: each call runs the state
switch, `CheckParticlesDone`, the glow animation, the rainbow, fade, light, mutation, spark and wobble steps, and a shaking brick draws
`AstroRand` in its update, so the skip would have to be shown to leave every state and every random draw alone before it counted, brick state
by brick state. A skip that did leave a draw out would be a lite change (the game would draw another sequence of random numbers), but it
would gain less than 0.23% of the process, which a pair of runs cannot resolve, and its difference would be a different shake of a brick
for nothing a player could name; it was not pursued.

**Why none is taken.** (1) A lite change has to cut a stretch's processor time measurably (the mod's own rule). If the whole of the game's
logic cost nothing, the process would drop by 0.6%; the stretches of one build spread by plus or minus 30%, and the median of five pairs
resolves a few percent at best. (2) Every faster method above keeps the answer, so there is no state to show where it differs: they are not
lite, and at two parts in ten thousand they do not earn a place in the performance pass either; they are listed in case that pass is ever
asked for them. (3) The 3D stretch's excess, the one place where the port is over its original, is the present path and the driver (section
11), not the game's logic, and a lite change does not touch the picture or the pace.

**To repeat.** The instrumented build was a throwaway: a `FNPROF("name")` line at the top of each function (an RAII `rdtsc` pair adding to a
table, written to a file named by `PROF_OUT` at exit), built as a second executable, and run by the scenario's usual script
(`profile/perf.py`'s `port_script`) under rundesk with `-fast`. Nothing of it was committed.

### 11c. The cycle split, at the original's window size (2026-11-12)

**Why again.** The tables above set a port that started full screen (render scale 4 on a 3840x2160 desktop, no `ScreenMode` in its
registry file) against an original in a window of its own size, and their original side carried the sound stand-in's module rescan
(`223d9bdf`: about 1 to 1.6 G cycles of a stretch, found by Typer Shark), so they are superseded by the figures here. Both were taken
by `profile/windows.sh` (the original, the port at the game's own size and the port in the framework's default window, one process at a
time, turn about, on one core under `pincore.py`, real time, the 2,030-update stretch of section 11; `PERF_WINDOW=1` is `ScreenMode=0` and
`-windowscale=1`, `2` is `ScreenMode=0` alone) with a rebuilt `build/origrun-bin` (the rescan fix; the staged copy's `dsound.dll` is the
rebuilt one, checked by hash). The window state and renderer of every run are in its result line (`display`: `client 640x480 px, logical
640x480, render scale 1, windowed, renderer direct3d11` at 1x, `client 1280x960 px, logical 640x480, render scale 2, windowed,
renderer direct3d11` at 2x; every port run, 2D under rundesk and 3D on the desktop, got Direct3D 11 through the hint). The original is
windowed at 640x480 in every run (origrun's copy; `visrun3d.py`'s with `ScreenMode` 0). `profile/windows_report.py` prints the sets.

**Totals, no counting, G cycles of the stretch** (medians of five or six turns, the range beside them; the machine was 0.35 to 0.5 busy):

| | Original | Port at 1x | Port at 2x |
|---|---|---|---|
| 2D (rundesk, five turns) | 6.13 (5.74 to 8.14), 3.02 M an update | 4.25 (4.05 to 4.65), 2.09 M | 4.25 (3.92 to 4.75), 2.09 M |
| 3D (the desktop, six turns) | 2.76 (2.44 to 3.44), 1.36 M an update | 3.70 (3.12 to 5.07), 1.82 M | 3.83 (3.01 to 4.69), 1.89 M |

2D: the port is at 69% of the original at either window size. 3D: it is at 134% at 1x and 139% at 2x, of which the port's mixer thread,
which the original under origrun's stand-in does not have (it mixes nothing), is 0.2 G: 126% without it. The window size makes no
difference to the port's processor time within the noise (the GPU does the scaling). **The target is not met in 3D** (the earlier tables'
"over by 5 to 25%" had an original 1.6 G and a port 1.2 G higher than here).

**The split, M cycles an update** (`-cyclesplit` in the port, `--count` in the original under origrun in 2D, the same counters read from
outside by `origcmp/countsnap.py` in 3D, the three functions of `profile/count.txt`: `DoUpdateFrames` 0x4d0450, `DrawDirtyStuff` 0x4d8760
and `Redraw` 0x4da210, checked by the calls: one update call an update, 0.9 draws and 0.9 presents an update in the original, no misses;
snapshots at the stretch's first update and 20 after the last input; medians of the runs the machine was at most 0.6 busy for):

| 3D, a desktop window | Original (5 runs) | Port at 1x (4) | Port at 2x (5) |
|---|---|---|---|
| update | 0.064 | 0.039 | 0.036 |
| draw (the game's own, up to the hand-off) | 0.326 | 0.319 | 0.302 |
| present (the main thread's call) | 0.981 | 0.617 | 0.424 |
| the driver's threads (`nvwgf2um.dll`) | 0 | 0.773 | 0.717 |
| **picture: draw + present + driver** | **1.306** | **1.715** | **1.424** |
| audio (the port's mixer) | 0 | 0.103 | 0.114 |
| loop (events, sleeps, the readings) | 0.205 | 0.065 | 0.066 |
| total | 1.596 | 1.928 | 1.650 |
| draws and presents an update | 0.92 and 0.92 | 0.52 and 0.52 | 0.48 and 0.48 |

| 2D (rundesk), three runs | Original | Port at 1x | Port at 2x |
|---|---|---|---|
| update | 0.069 | 0.048 | 0.049 |
| draw | 2.073 (the original's own blitters) | 0.308 | 0.341 |
| present | 0.634 | 0.556 | 0.559 |
| the driver's threads | 0 | 0.923 | 0.854 |
| picture | 2.703 | 1.787 | 1.711 |
| audio (the port's mixer) | 0 | 0.115 | 0.119 |
| loop | 0.160 | 0.067 | 0.062 |
| total | 2.950 | 2.012 | 1.932 |

**What it shows.** The game's own code is not the excess, in 3D or 2D: the port's update costs 30 to 45% less than the original's (0.036
to 0.049 against 0.064 to 0.069), its own drawing is the same in 3D (0.30 to 0.32 against 0.33) and a seventh of the original's in 2D, and
its loop costs a third to two fifths. What is over in 3D is all in the presentation: the picture (draw, present and the driver together, which cannot be split,
the draw calls being batched until the present) is 1.42 to 1.72 against 1.31, 9 to 31% over, and the Direct3D 11 driver's own worker
thread is 0.72 to 0.78 an update on average in each set of runs, against no driver thread at all in the original. A present
and its driver work cost the port about 2.7 M cycles (the present 0.62 and the driver 0.77 over 0.52 presents an update) against the
original's 1.07 for one (0.98 over 0.92). The port presents every second update or so where the original presents nearly every one, so
per-call figures do not compare, per-update ones do. The mixer's 0.10 to 0.12 is the other thing the original does not pay in these runs.
In 2D the port is under the original in every row but the mixer, because the original's own blitting (2.07) dwarfs the port's.

**What this measurement cannot say.** The three counted parts are the same functions on both sides, but the picture is not split (the
limit in `docs/TEST_COMMANDS.md`, "The cycle split"). The counting costs the original 1.4 to 3.3% of its total and the port 0.5 to 1.4% (the
files' `counter` lines). The machine was not quiet: a stretch of the 3D runs (04:36 to 04:50) was taken with the whole machine busy and
the original rose to 3.2 to 5.0 G of a stretch (a loading time of 6 to 24 s) while the port stayed at 3.5 to 3.9; those runs are left
out of the table (`windows_report.py --busy-max 0.6`), and the runs that are in spread by plus or minus 30%. The 3D split has four to
five quiet runs a column; the totals have six. The split is for the proportions: the target's figure is the totals.

**To repeat.** From the worktree root, in the background: `MODE=3d SPLIT=1 bash games/astropop/tools/profile/windows.sh <dir> <rounds>`
(`MODE=2d` for rundesk; `SPLIT=0` for the totals; 22:00 to 08:00 for 3D; `FIRST=4 APPEND=1` adds rounds), then `python -I
games/astropop/tools/profile/windows_report.py --busy-max 0.6 <dir>`. A single split: `PERF_SPLIT=1 python -I profile/perf.py orig3d <dir> 1`
and `PERF_SPLIT=1 PERF_3D=1 PERF_DESKTOP=1 PERF_CLOCK=real python -I profile/perf.py port-minute <dir>`, then `python -I
tools/cyclesplit.py <port's split.jsonl> <original's counts.jsonl> --from a --to b`. The raw run folders (their samples and logs) were
deleted once these figures were written.

### 11d. Presents held by the frame loop's pace against the display's vsync, in 3D (2026-11-12)

**The question** (the coordinator's): the framework asks the renderer for vsync by default, and a present then costs the port a driver
thread and a wait for the blank; Bejeweled 2's `DDInterface::FrameHeldByPace` (`22aa5b7b`) holds a draw to the display's refresh interval
when the renderer has no vsync. Does presenting that way, in a window, cost the 3D stretch less? **Setting:** the 3D scenario of section
11, on the desktop (22:00 to 08:00), the window at the original's size (`PERF_WINDOW=1`, display line `client 640x480 px, logical 640x480,
render scale 1, windowed, renderer direct3d11` in every run), real time, one process at a time turn about on one core under `pincore.py`,
the original (`perf.py orig3d`) in every round (`profile/vsync.sh`, six rounds without counting, four with the cycle split). *vsync* is
the default; *pace* is `SDL_RENDER_VSYNC=0`, which takes the renderer's vsync flag away so that the pace engages. Every port run wrote
its present log (`-presentlog`, the shared commit `41636a8a`, `docs/TEST_COMMANDS.md`, "The present log"), whose header says which it had:
`renderer direct3d11 vsync_asked 1 vsync_flag 1 pace_ms 0.000` (vsync: the blank holds the presents) or `vsync_asked 1 vsync_flag 0 pace_ms
16.667` (pace). The display is 60 Hz (the vsync runs' median interval is 16.8 ms).

| Six rounds, no counting | Original | Port, vsync | Port, pace |
|---|---|---|---|
| stretch, G cycles (median, range) | 3.18 (2.43 to 3.64) | 3.64 (3.19 to 4.09) | 3.34 (3.07 to 4.10) |
| an update, M cycles | 1.57 | 1.79 | 1.65 |
| against the original | | 114% | 105% |

The pairs of one round differ by +0.23, +0.83, -0.01, -0.06, +0.18 and -0.21 G (vsync less pace): a median of 0.1 G, 3%, and the
spread of one run is 30%, so the cap's gain is somewhere between none and the 8% of the medians; this set does not resolve it.

| Four rounds, the cycle split, M cycles an update | Original | Port, vsync | Port, pace |
|---|---|---|---|
| update | 0.073 | 0.039 | 0.060 |
| draw | 0.386 | 0.327 | 0.370 |
| present (the main thread's call) | 1.150 | 0.429 | 0.294 |
| the driver's threads | 0 | 0.759 | 0.812 |
| **picture: draw + present + driver** | **1.536** | **1.542** | **1.488** |
| audio (the port's mixer) | 0 | 0.133 | 0.133 |
| loop | 0.233 | 0.070 | 0.118 |
| total | 1.874 | 1.783 | 1.795 |
| presents an update | 0.97 | 0.50 | 0.58 |
| present + driver, per update | 1.15 | 1.19 | 1.11 |
| **present + driver, per present** | **1.19** | **2.38** | **1.91** |

**The cadence** (`profile/present_intervals.py`, the intervals between the starts of consecutive presents over the last 40 seconds of
each run, medians of the six):

| | Presents a second | Interval median | 5th to 95th percentile | Within a quarter of the median | Over one and a half intervals | The call |
|---|---|---|---|---|---|---|
| vsync | 50.5 | 16.8 ms | 10.8 to 40.6 ms | 75% | 15% | 18.9 ms on average (16.4 median: it waits for the blank) |
| pace | 54.5 | 19.2 ms | 7.4 to 29.3 ms | 54% | 5% | 2.4 ms on average (0.3 to 0.65 median) |

The pace's presents do not stay even: a draw is held to the next update that finds the interval over, and the updates come every 10 ms, so
the intervals are 10, 20 and 30 ms (a median of 19 against the target 16.7) and only about half lie within a quarter of the median. The
vsync's cadence is the display's 16.7 ms for three presents in four, with a missed beat (a gap of 25 ms or more) in one in seven.

**What it shows.** (1) The cap moves the cost and does not remove it: the main thread's present falls by 0.14 M an update, the driver's
worker thread rises by 0.05, the loop by 0.05 and the update part by 0.02 (the loop wakes more often with nothing to wait for in a
present; counting costs 2.1% of the pace runs against 0.6% of the vsync's), so the total is the same in the split (1.78 and 1.80) and
about 8% lower in the medians of the totals, which is inside the noise. (2) The picture costs the port what it costs the original **per
update** (present + driver: 1.19 with vsync, 1.11 with the pace, 1.15 for the original), but the port presents about every second update
and the original nearly every one, so **per present the port spends 2.38 M cycles with vsync and 1.91 M with the pace against the
original's 1.19 M** (the main thread's present plus the driver's worker, over the presents). The cap lowers that by a fifth and does not
close it. (3) The driver thread (`nvwgf2um.dll`, 0.76 to 0.83 M an update, 1.4 to 1.5 M a present) is the largest single cost of the port's 3D
and does not depend on the window size (1x and 2x in 11c) or on the pace: it is the Direct3D 11 driver executing what a frame submits (80
geometry and 41 fill calls a frame, one draw per change of texture: section 11), so what cuts it is fewer, larger submissions per frame,
which is the framework's to do, not the game's. The mixer (0.13 M an update, 0.27 G a stretch, in the totals above) is the other cost the
original's process does not have; the totals without it are 1.66 M an update with vsync and 1.52 with the pace against the original's
1.57. (4) The cap is not recommended for the GPU path on these figures: it saves nothing the noise lets us see, and its cadence is
lumpier than the display's own.

**How far over, in this set, and why it varies.** The same-round pairs (the original's run of a round with the port's) put the port at
121% (vsync) and 107% (pace) of the original as medians of the six rounds (the medians of the stretches, 114% and 105%); the four counted
rounds' own stretches put both at 107%, with the split's window, which is not the stretch, giving 95% and 96%; 11c's set, 134%. The
machine was busy (0.42 to 0.76 of it) and the original's cost went up with it in the six totals rounds: its three stretches at 0.42 to
0.48 busy were 2.4 to 3.0 G (3.3 at 0.51) and its two at 0.61 to 0.66 were 3.6 G, while the port's (vsync) ran 3.2 to 4.1 G across the
same loads with no such rise. So in the stretch totals the port is over by about 5 to 35% in 3D, depending on the load, in every set so far
(the split's window alone has it under), and none of them is a quiet machine, which this measurement cannot make.

**To repeat.** From the worktree root, 22:00 to 08:00, in the background: `SPLIT=0 bash games/astropop/tools/profile/vsync.sh <dir> 6` (`SPLIT=1`
for the split; `FIRST=7 APPEND=1` adds rounds), then `python -I games/astropop/tools/profile/windows_report.py <dir>` and `python -I
games/astropop/tools/profile/present_intervals.py <dir>`. The raw run folders were deleted once these figures were written.

## 12. The readability pass (2026-11-11; `docs/CODE_STANDARDS.md`)

**Result.** `tools/namecheck.py games/astropop` reports no finding in 114 files, and no identifier of the game's sources is a prefix and an
offset (`m1a34`, `aFloat50`, `mFlag8E8`): every member, local, parameter and global says what it holds, and every function says which
original function it is or that it is the port's (a `// port:` line). What the pass did, in order:

1. *Helpers.* Every helper names its original function (an address) or carries a `// port: ...` line saying why it exists. Fields that
   nothing reads were taken out (the brick's flags and numbers, the application's `mFlag65b`, `UserProfile`'s `mUnknown62`), in the
   removal commit (`afe6e702`) and after it.
2. *The Board's 418 fields.* `docs/board_fields.tsv` is the table (offset, type, name, comment; `tools/boardfields.py` writes the block
   of `Board.h` from it). Six batches of about 70 rows were named by reading every use of the field, one agent at a time, each batch
   applied by a script (`whole-word` replacement in the sources and these notes) and proved before the next: the build's object code
   is the same as before the batch (`tools/objsnap.py cmp`: 84 objects, 1,632 functions, 0 differ) and the string literals of every
   changed file are the same (a small lexer over `git show HEAD:` against the working copy). Where the original's code gave no
   meaning, a name says what the code does (`mPowerBrickRowState`, `mTurbotWindUpCounter`); the table's comment says what was seen.
3. *The small classes.* `StageBg`, `HintBox`, `HintSpot`, `FloatText`, `LevelUpText`, `BlinkRec`, `AstroDialog`, `ListDialog`,
   `HelpDialog` and the speed table's `RampRecord` had members called by offset too, and the same hex names exist in several classes
   (`m10c` of the Board is not `m10c` of `StageBg`), so a whole-word replacement over the sources renamed another class's member (the
   first batch did that to three `StageBg` members and one of `HelpDialog`; found before any commit, repaired at once). Those names
   were changed with the compiler: rename in the class's declaration, build with `-k 0`, and fix each "no member named" error at its
   position, until none is left. That is also the way to rename a member of one class whose name another class uses.
4. *The stand-ins.* `aFloat50`/`aFloat52`, `aValue198c`, `theSetting5x`, `mFlag78`/`mFlag80` of the overlay (now `mEmergencyPause`,
   `mShownWhileRunning`), `gFlag5A5100` (`gStateSaved`) and the rest were named from the fields they carry.
5. *Dead members.* After naming, every Board field with no read was found by a script (a use that is not the left side of a plain
   one-line assignment counts as a read) and taken out with its stores: 36 fields, among them the original's pointer trail
   (`PointerTrail`, the whole class), the brickometer's rise (four numbers the original computes and the port never draws), the
   weapon hint text, the Hall of Fame flag (the call stays, its result goes), the bonus warp's text position, the wipe flags and
   twelve fields that only a constructor or `ResetGame` touched. In the small classes: `StreakRec::mFlag28`, `WarpStar::mField5C`,
   `Particle::mField2C`, `KillAllBrick::mField234`, `StageBg::m11c`, `CheckpointDialog::m114`, the update counter of `HintBox`, the
   done flag of `LevelUpText`, the padding of `StarRec`, and `MakeDialog`'s last parameter (the original passes a flag it does not read;
   all 14 callers passed `true`). The two numbers of `HintBox` that looked dead are read (`HideCellHint`) and are `mCellColumn` and
   `mCellRow`. A stray row `made` (a word of a comment that the generator took for the field at +0xade) went too, and the generator no
   longer adds rows unless asked (`--add`).

**The removals were proved differently**, since taking out a member moves the offset of every member after it and so changes the object
code: the build (native and web) and `tools/check.sh astropop` (the frozen baseline: a classic game of 120 moves with real input, its
state compared with the baseline exe's). Every removed store was a statement of its own whose value nothing reads; a store with a
call in it (an `AddHallOfFame`) kept the call.

**Names given for what the code does, worth a second look when a screen is read against the original again:** `mStartBannerDoneInBonus`
(set when the start banner ends in a bonus round, yet it holds back the round's end checks), `mClicksBlocked`, `mUpgradeDialogDone` and
`mUpgradeDialogSeen` (both set by `TakeFocus`), `mStartedFromCheckpoint`, `mStageTextEnabled`, `mPointerOverEasyGate` (the easy gate is
inferred from the picture's name), `mNextLevelScreenKind` / `mLevelEndScreenKind` (the 0 to 4 codes of the upgrade screen: which
level's break each belongs to is not settled), `mPowerBrickRowState`, `mFrameDimming`, `mTurbotWindUpCounter`, `mHardStage`,
`mKeepLoopSoundsMuted`, `mRowsLeftToPass`, `mRampTime`, `mPauseCount`.

**To repeat or go on.** `python -I tools/namecheck.py games/astropop` lists what is left; `tools/objsnap.py snap|cmp astropop <json>`
proves a rename by object code; a class's members are renamed by building with `-k 0` and fixing the errors at their positions.

## 13. The PopCap Game Collection version (2026-11-11)

The port was made from the Steam copy (WinAP.exe, the launcher with the game appended; the game carved out of it is
`re/astropop/bin/AstroPopGame.exe`), so it is checked against the game's version on the PopCap Game Collection disc (`docs/COORDINATION.md`):
the two versions compared, and the port run on the Collection's files. The procedure is Bejeweled's (its GAME_NOTES section 23) and Typer
Shark's (its section 15); `tools/collection/` holds the tools.

**Getting the files.** `unshield` (built for Bejeweled's check, `E:\retools\unshield-build`; the user allowed it on 2026-11-10) read the disc
image's cabinet, which was mounted read-only and dismounted afterwards. The group `AstroPopDeluxeComponent` came out: 920 files, 11 MB, in the
main checkout's `re/_installers/AstroPopDeluxeCollection` (git-ignored). The copy has no launcher: `AstroPop.exe` is the game itself, beside
`bass.dll`, `license.txt`, `readme.html` and the folders `data`, `fonts`, `images`, `music`, `properties`, `sounds` and `users`.

**The exe is another build, unlike Bejeweled's and Typer Shark's.** Set beside the carved game image (`tools/collection/cmpexe.py`), the
Collection's `AstroPop.exe` was linked on 2 July 2007 (the Steam game: 6 April 2004), is 2,295,088 bytes against 1,748,992, and has a bigger
`.text` (1.84 MB against 1.43 MB), `.rdata`, `.data` and `.rsrc`; none of them is the Steam game's bytes with other addresses. Its strings
(`tools/collection/textcmp.py`) say what changed: a newer compiler runtime (Dinkumware 1992-2004, the Steam build's is 1992-2001), RTTI
for the game's own classes (`Grid`, `GridPiece`, `BonusGridPiece`, `FuelCellGridPiece`, `KillAllGridPiece`, `RadialKillGridPiece`,
`Movement`, `StoryPanel`, `CharSelector`, `SUPAHintDialog`, `SUPAUpgradeDialog`, `GameSelector`, ...: the Steam image has it for the framework's
classes only), a pak reader (`main.pak`), a JPEG 2000 codec (`j2k-codec.dll`), a Direct3D 8 path (`d3d8.dll`,
`Direct3DCreate8`; both builds have DirectDraw's), a bigger part of BASS's interface, a screensaver mode (`-screensaver`) and the Collection's game selector. Players: both
builds name a player's file `users\data<N>.usr` and the hall of fame `users\datahs.usr`; the Collection's has a file for the player list as
well, `users\users.dat` (empty in the disc's copy), and a `users\data<N>e.usr`, where the Steam build lists the players in the registry only
(`Users\LastUserNum`, `Users\<name>\<id>`). No save of a Collection copy was at hand, so nothing tests one: the save import
(`platform/saveimport.json`'s entry for AstroPop) covers only the Steam build's format, its files and its registry, and a Collection
save is an open item. The Steam build alone has the update check and its texts.

Of the 1,703 texts the port reads from the exe (`src/GameTextTable.inc`, by address in the Steam image), 1,677 are in the Collection's
exe byte for byte, at other addresses. Of the 26 that are not, 15 are runs of three bytes of code that the table lists by accident, and the
others are framework strings that the Collection's build writes another way or no longer has: the source paths of the 2004 build's modifiable
values, the registry key formats, the version "1.0", the two error texts of an old version's cleanup, and the warning text for a profile left in
the registry by an old version (the one `UserProfile::ReadRegistry` shows for a profile below version 0.2: a player cannot meet it). Every
text the game shows is the same.

What is not known: whether the 2007 build plays exactly like the 2004 one. Its code differs throughout (another compiler and a newer
framework), so no function was compared, and the Collection's own exe was not run (section 10 and the end of this section). What speaks for
it: the game's texts, the levels (`data/bonus*.xcf`), the properties, the music and every picture and sound of the game are the same
files, so the game it plays is the Steam game's data under a newer framework.

**The files** (`tools/collection/cmpfiles.py`). Of the Steam copy's 982 files and the Collection's 920, 854 are the same byte for byte: every
level, sound, song and picture but the ones below. The Steam copy also has the launcher's files (`winap.exe`, `drm.swf`, `drm.xml`, `drmss.jpg`,
`flash.ocx`), the player's own `users/data1.usr` and 67 `sounds/cached_*.wav`, which the port never asks for; the Collection also has
`astropop.exe`, `users/users.dat` (the default player list) and the game selector's pictures (`gameselectorx.jpg`, `gs_moregames_*x*`,
`gs_mouseover_help*`) that the game does not draw. The differences:

- `images/loader_bg.jpg`, the picture of the loading screen and the title screen (`tools/collection/cmpimages.py`): another encoding (a
  hundred thousand pixels differ, mostly by the encoder's noise) with PopCap's round logo and a TM mark at the top right and the bottom
  right, where the Steam copy's has the flat "PopCap Games" logo. The port draws the file it is given.
- The 14 pictures of `fonts/` (`_fusion*.png`, `_kiloton9.png`, `_neuropolx30*.png`, `_supernova20.png`): the same pixels (no pixel differs), stored
  as RGBA where the Steam copy's are grey or palette pictures.
- `properties/resources.xml`: the Collection lists the five pictures of the game selector more. The port's resource ids are the numbers of the
  Steam image's list, so a resource that is not in it is never asked for.
- `properties/partner.xml`: the same product name, registry key, `NoReg`, size and `DefaultWindowed`. The Collection's has `DontUpdate` false,
  an `Offering` number and `VistaCompliant`; Steam's has a Steam id, a buy link and `NoPopLink`. The port does not read them.
- The 35 `.sig` files (the signatures of the data files: 24 bytes each), `bass.dll` (the port plays music itself), the licence and the readme.

**The port on the Collection's files** (`tools/collection/run.sh`). `stage_astropop_assets.py` stages a Collection folder as it does a Steam one
(916 files, `astropop.exe` in place of `winap.exe`). The texts come from that exe: `GameText.cpp` takes `AstroPop.exe` beside `WinAP.exe`
and `AstroPopGame.exe`, and a text that is not at its address in the Steam image is looked up by content (length and FNV hash, in every
zero-terminated run of the exe and every tail of one, since the compiler pools texts that end alike). A run on the Collection logs
"1703 of 1703 texts are not at their address; 1677 found by their content" and "26 not in this executable" (the 26 above); a run on the Steam
copy reads every text at its address and logs nothing, as before. `main.cpp`'s check of a game folder takes either exe too, and so does
the page importer (`games/astropop/web/page.js`: `WinAP.exe|AstroPop.exe` is one marker; an ad-hoc run of the importer's rules gave the
Steam folder, the Collection's and a folder with neither as expected; `tools/gameimporttest.js` has no AstroPop line yet).
- `tools/check.sh astropop` on the Collection's staging (`AP_EXE`): the three frozen games (classic from level 1, one from level 4, survival)
  end in the states the check expects, and each plays move for move as the frozen baseline does on the Steam files.
- The ten tours of phase two that need no original (menus, options, halls, users, classic1, ingame1, survend, quit1, ships1, emerg1: 190
  screenshots, every dialog, the Options box with its sliders, the hall of fame, the players' list and deletion, whole games to their
  ends, the secret ships screen and the Emergency panel; `tools/tour/tour.sh`, port only), each run once on each set of files from a fresh
  profile folder with the same seed and input, compared by shotdiff: 188 pairs are equal, pixel for pixel. The other two are the title screen
  and its hover (`a01_title`, `a02_title_hover` of the menus tour), which differ in the two places where the loading picture differs: the
  round logo against the flat "PopCap Games" one (101 by 100 pixels at the bottom right) and the TM mark at the top right (15 by 10). Both are
  the files' (the title screen draws `loader_bg.jpg`).
- The web build under Node on the Collection's files (`wasmrun.js`: the page's code, no canvas): the menus and options tours, 32 pictures, each
  within 6 levels of 255 of the native port's on the same files (the web build's usual difference: on the Steam files 17 of the 18 tours
  differ by at most 9 levels, `IDENTICAL_PLAY.md`).

**Not done.** The Collection's own exe was not run, so none of its screens was compared with the port's. Its code is another build's, and
a run needs the watches of `origcmp` (the app's update counter, the board's score, the generators), which are addresses of the Steam image:
a run of the Collection's game means finding them in its exe first. That exe has RTTI for the game's classes, so a Ghidra import of it
(its functions paired with the Steam image's by strings and calls) would be the route to compare the 2007 build's play; the coordinator
decided against it (2026-11-11): the definition of done asks that the port work with the Collection's files, which it does, and the
readability pass has named the classes. The Collection's Direct3D 8 drawing in 3D mode was not seen either (section 10, the screen wipe). The port's executable
is called `astropop.exe`, the name of the Collection's game: the installer that puts a native port beside the original
(`docs/STEAM_LAUNCH.md`) must not write it into a Collection folder under that name.

## 14. The music (2026-11-11)

**A bug found: the port played no music.** The original plays `music\music.mo3` (one module of 43 orders) through BASS; the port plays it
through the framework's `SDLMusicInterface` and `Mixer::LoadSong`, which decodes an MO3 with libopenmpt once
`Platform::RegisterOpenMPTModules()` has registered it. `main` never called it (the other games with an MO3 tune call it in `main`), so
`LoadSong` returned 0 with no log line and no error, and every run of the port, native and web, was silent apart from the effects. No
earlier comparison could show it (they compare pictures and the sound effects; the page's silent mode hides it too). Fixed in 9be38396: the call
follows the "game: ... | user: ... | mods: ..." log line in `main.cpp`. Checked after the fix natively (the sound log has the song starts) and in the
web build (`Module._byopop_music_song()` is 2 and the music view lists the 43 positions).

**The comparison** (`tools/tour/musictour.sh`, `tools/tour/musiccmp.py`). `PlaySong(order)` (0x408b90) puts a position of the module on the
copy that does not play (slot 0 or 1) and fades the other out; its choices are what the game decides. origrun's sandbox has no sound
device, so BASS never starts a stream there: the app's fields are read instead, by four watches (`ORIG_WATCHES` of `tour.sh`): the position
`mMusicOrder` (`[0x5dc220]+0x6dc`), the slot `mMusicSlot` (`+0x6d8`) and the two ends of the vector of paused slots (`+0x634`, `+0x638`).
The port's side is `BYOPOP_SOUNDLOG`: a `music fadein <slot> offset=<position>` line for each song it starts, and `music pause` and `music
resume` lines for the slots that a focus change stops and restarts. `musiccmp.py <orig.log> <port_sound.log>` compares the starts one by one
(position, slot, and the update: the original's counter is the port's plus 2, a difference of 1 more is allowed; the first start, at update
0, is exempt) and exits 1 on a difference.

**Result: the same everywhere.** The positions the game uses: 0 (the title, once the loading has run), 5 (the main menu, the pilot
screen, the story screens), 9 (a game; 9 and 10 are one song, and in the runs 10 followed each 0x29), 0x23 (the danger song of the Emergency
siren), 0x29 (the end of a level: the level-up text, or the warp into a bonus round) and 0x27 (the game over). Tours `pause1`, `classic1`, `ingame1`, `loss1`, `emerg1` and
`survend`: 3, 3, 4, 4, 3 and 6 starts, positions 0, 5, 9, 0x23 and 0x27, every one equal in position, slot and update. A recorded classic game of
700 moves from the fifth level (`gamerun.sh`; the profile and the recording by `origcmp/record.py`, 2,234 inputs, 26,917 updates, score
967,930 at the end, level 9): 13 starts (0, 5, 9, then 0x29 and 10 at each of four level-ups, then a story screen's 5 and 9), equal. The tour
`resume1` needs a saved game that the loop did not give it, so it reached only the menu (2 starts, equal; not a gameplay result).

**Not compared.** The pause and resume at a loss and a regain of the focus: the port pauses the slot that plays and resumes it at the update
of the focus change (`pause1` logged pauses at 3519 and 3639 and resumes at 3559 and 3679), the original's BASS stream never plays in the sandbox,
so `IsPlaying` is false for both slots when the focus is lost, the paused-slots list (`mPausedSongs`, the watches at `+0x634` and `+0x638`) stays
empty and nothing of its behaviour can be read. The code is the same translated function: `AstroPopApp::PauseMusicForFocus` (0x4252b0) pauses
each slot that plays and keeps its number in the list, and the regain resumes the listed slots and clears it. A run of the original on the real
desktop with sound, in the night window, could show it. The sound itself (the port's decoder against BASS) is not compared either.

## 15. The autosplit block (2026-11-12; `docs/WEB_PLAYER.md`, "One design for every port")

AstroPop writes the shared autosplit block, layout 2, id `APDX`, from `src/LiveSplitObserver.cpp` (after each `AstroPopApp::UpdateFrames`). It
is written once something asks: the web page (`byopop_livesplit_block`, exported in `CMakeLists.txt`), an enabled mod with the feature
`livesplit` (none exists yet; the mods are looked at once in a hundred updates), or `-livesplitlog=<file>` (a test: one line when the block
changes but for the clocks, scores and tick). Until then an update costs one test. Three inert values in the Board (`mBoardSerial`,
`mRestoredGame`, `mGameOverCause`) are the only game code it needed.

| Block field | AstroPop |
|---|---|
| mode | `Board::mGameType`: 1 classic, 2 survival; -1 off a game (the demonstration under the title and the menus is no game) |
| skill | the pilot (`Board::mMode`): 0 Vector, 1 Sprocket, 2 Turbot, 3 Vixx |
| GO | the first update of a fresh game (classic from level 1, or survival); not a saved game that goes on, not a classic game from a checkpoint or any level but 1 (both set `RESUMED`), not a game already on when the block was asked for |
| level clear | `Board::LevelComplete`, seen as `mLevelsCompleted` rising: the update in which the goal is reached and the "Level Up" text starts (the levels before a multiple of four start their bonus round at once instead); `progress` = the level just cleared (`mLevelNumber` changes only at the warp gate), 0 before the first, -1 in survival |
| game over | `Board::GameOver` (`mGameOver`): kind 0 a new row pushed a column to the ship, 1 the stack filled the field, 2 the player left (the menu came up over a game still on), 3 other (a test command's `lose`); no win: the classic game goes on after level 32 |
| gameOwn | bytes 0 to 3: SUPA weapon starts in this game (`mPowerupActive` going to 1: `BeginPlay`), u32; bytes 4 to 7: the level the game began at, i32 |
| state | 0 playing, 1 a level is complete (level-up screen, warp, gate), 2 a bonus round, 3 game over |
| displayLevel | `mLevelNumber` in classic (the "Level N" on screen), `mStageNumber` in survival (a minute of play each) |

Proof that it is inert: `games/astropop/tools/livesplit/inert.sh <dir>` plays check.sh's three bot games with the frozen baseline, the port with
the block off and the port with it on, and the driver transcripts are identical (1470, 1758 and 2002 lines). `scripted.py` plays the short games
of the replay logs (classic with two levels cleared, survival, a SUPA start; each ends in `lose`). Kinds 0 and 1 of the game over were not seen in
a log: the bot does not lose by itself in the time a driver has, and the idle game that was tried stayed paused at a hint; they are read from the
two places that call `GameOver` (`BoardRows.cpp`, `BoardFill.cpp`).
