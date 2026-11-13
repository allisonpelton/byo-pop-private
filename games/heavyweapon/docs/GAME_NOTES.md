# Heavy Weapon Deluxe: game notes

Heavy Weapon Deluxe (PopCap, 2005; the decompilation is named Popgun) is translated from our own Ghidra export of the
game's exe (`re/heavyweapon`, git-ignored) onto the shared framework, with the 2004 compatibility library where the
original's early-2005 framework behaves as BookWorm's does (`FRAMEWORK_2005.md`). The game's texts are not in the
repository: the port reads each from the player's own exe by its address (`src/GameText.cpp`). Its music is an MO3
module played through libopenmpt, and its three Flash cutscenes play through the shared SWF player (`engines/swf`).
The original's registration paths and every button that leads out of the game (the "more games" and buy pages, the
update check) are not in the port.

## Test commands

Reachable only from a test (`-script`, `-commands`, the web page's test call; `docs/TEST_COMMANDS.md`), in
`src/testhooks/TestCommands.cpp`. Replies go to stdout: `STATE {json}`, `MOVE {json}` or `MOVE null`, `OK <command>`,
`ERROR <command>: <reason>`.

| command | what it does |
| --- | --- |
| `state` | `{screen, dialog, mode, level, maxlevel, score, lives, nukes, over, appupdates}` and, while a board is up, `timer` (the level's time), `length`, `boss` (its place in the enemy list, -1 before it comes), `bosshealth`, `bossup` (1 while the boss can be hit), `enemies`, `tankx`, `tankdead` (updates until the next tank), `fire`, `paused`, `suspended` (the board stands still while the pointer is off it) and `updates`. `screen` is the widget on top (`title`, `menu`, `help`, `map`, `armory`, `game`, `results`, `gameover`, `highscores`, `credits`); `dialog` is the highest dialog id up; `lives` counts the tanks left after the one in play |
| `goto <screen>` | `menu`, `options`, `help`, `highscores`, `credits`, `campaign` (a new campaign at its first map), `survival`, `armory`; the screens there go as their buttons take them away. While the intro or the loading runs it ends the intro and answers "still loading: send it again". A player named "Test" is made when there is none |
| `level <n>` | campaign level n (1 to 19, the last the final boss's) now, with the player's upgrades, without the armory or the map |
| `unlock all` | the player has reached the campaign's end: survival and every level are open |
| `set score\|lives\|nukes\|level\|timer\|progress <v>` | the app's score, tanks left, nukes, next level (1-based) or the level's time; `progress` puts the player's saved campaign before level v (1-based), as after passing the one before it, and saves it, so the main menu's campaign offers to continue there |
| `win` | before the boss, the level's time jumps to the boss's warning, so the boss comes as at the end of the waves; with the boss up it is killed and dies as by the gun (an error while it is still coming in) |
| `lose` | no tank left after this one, and the tank is killed; the game-over box follows when the tank's wait runs out (an error while no tank is on the field) |
| `seed <n>` | `Rand()` and the game's own generator seeded as at the start with `-seed=n` |
| `move` | the bot's next move (`src/testhooks/Bot.cpp`): `MOVE {x, y, fire, target, input, updates}` in a fight; between fights `MOVE {click, input, updates}` on the button that goes on (a message box's OK, a play tip's OK, the help's OK, the results' ADVANCE, the armory's plus buttons until every point is spent and then GO, the map's link); else `MOVE null` |
| `memreport [file]` | the framework's, `MEMREPORT {...}` |
| `trace <file>\|off` | for the identical-play comparison (`tools/idplay`): from the next update on, a line an update in the file with what the comparison reads from the original's memory after the same update (both generators' indexes and first words, the score, level, lives, nukes, and the board's time, tank, kills and boss), each written before the update's script input runs |
| `botrecord <file> [<period> <residue>...]` | the bot plays through the script: each of its moves goes in at once as `move`, `click`, `down` or `up` script commands, and each goes into the file as `<update> <command> <x> <y>`, so that the original can be given the same input; the bot is asked again 4 updates after a move, 20 after a click and 10 when it has none; with `<period> <residue>...` after the file, only at an update whose number modulo the period is one of the residues (the next such one), the updates before which the original reads its input under origrun |

**The bot.** The tank drives toward where its gun points and fires while the left button is held, so a move is one place
for the pointer and the button. The bot aims at the boss while it can be hit, otherwise at the live enemy nearest the
tank across the field, leading it by the time a bullet (5 pixels an update) takes to reach it. Before aiming it looks
for danger: an enemy bullet or a falling bomb (a bomb's fall modelled as `FallBomb` does it) that would land within 56
pixels of the tank in the next 200 updates, or a satellite charging its beam over the tank, sends the pointer low to the
side away from it, so the tank drives clear; one landing within 120 pixels on the side the tank would drive to stops the
tank. The pointer stays 8 pixels inside the field's sides: a shake moves the board up to 7 pixels, and a pointer left
outside it suspends the game (`MouseLeave`) with the board still moved, for good while the pointer does not move. In the
armory it spends each point on the lowest weapon on offer. It plays level 1 through without losing a tank
and survival to about 11,000 updates of its clock; from level 5 on, the homing missiles (hellfire) beat its dodge.

**Drivers.** Natively `tools/playbymove` plays by `move` (the check suite's games, below). On the web,
`tools/web/onegame.js` plays one game in the player-only page with real DOM input: the intro clicked away, the title's
link, a new player's name typed in its box, the main menu's MISSION or SURVIVAL button (survival unlocked first with
`unlock all`), the bot's moves to the game over, and the game-over box's QUIT. The page's kit waits a frame for each DOM
event, and a fast frame runs several updates, so on the web the bot answers about every 40 updates instead of every 4
and loses sooner; that is the kit's pace, not the port's.

**The check suite** (`tools/check.sh heavyweapon`, about a minute without the baseline): the tour from
`tools/tour_test.txt` (the intro, the title, the menu's screens, the help, a campaign from the map through the first
game's help and play tip to level 1's boss, beaten with `win`, its results and the armory, level 5 and survival lost with
`lose` and left by the game-over box's QUIT, the options) and its rerun, whose 19 screens, state lines and player files
must match and be the frozen ones; then the bot's campaign from the title (3400 moves, into level 2), level 9 and
survival to their game overs, and a rerun of level 9, against frozen summary lines; and the frozen baseline's
(`build/baseline-heavyweapon/heavyweapon.exe`) tour and games, which must match move for move.

## The original under origrun

The identical-play comparison (`tools/idplay/idplay.py`, `docs/IDENTICAL_PLAY.md`) runs the Steam copy's game
(origrun's `heavyweapon` row) from a copy under `build/origrun/heavyweapon/game`, on origrun's virtual clock with the
scripted cursor and keys. These make it run there and play as the port does:

- **Its settings.** The original keeps them (ScreenMode, LastUser, Particles and the rest) under
  `HKLM\SOFTWARE\PopCap\HeavyWeapon`, which a run without elevation cannot write, so it would start full screen every
  time. The runs use `--reg-map popcap`, which sends them to the same key under HKCU, preset with ScreenMode 0 (windowed).
- **The focus.** The title takes clicks only once it has the focus, which a window on rundesk's desktop never gets by
  itself: a run begins with `focus regain`, and the first click then ends the intro.
- **The copy's pictures.** Eleven of the Steam copy's PNGs are 1- or 4-bit paletted (`antagonistan_bg2_`, `tracks`,
  `slidertrack` and eight more backgrounds). The original's PNG reader writes past its buffer on them: the loading stops at
  its fourth picture with an access violation in the row copy (0x4c80a7), and the crash dialog then waits for good on the
  hidden desktop. Steam's launcher rewrites a copy's pictures on its first run (as it did Bejeweled's), and this copy never
  goes through it, so `idplay.py prepare` stores those eleven again as 8-bit paletted PNGs with the same palette and
  transparency; every decoded pixel stays the same (it checks). The port reads them as they are.
- **The patches** (`tools/origrun/patches/heavyweapon.txt`, which `idplay.py prepare` writes): `Rand()` and the C
  library's `srand` are seeded from origrun's `--seed` instead of `GetTickCount` (`SexyAppBase::Init`, 0x45cb5f and
  0x45cbd8), as the port's `-seed=N`; and `StartLoadingThread` (0x44cc90) calls the loading stub on the main thread
  instead of starting a thread, as the port's framework loads, so the loading ends inside the click that ends the intro
  and the loading-done update is the same in every run. With a loading thread, which runs on the wall clock, the main
  thread runs a different number of virtual updates ahead each time. The third patch is the mouse's: the window
  procedure's `WM_TIMER` (0x44ff50; `SetTimer` stays on the wall clock under origrun) asks `WindowFromPoint` whether the
  window is under the cursor, which on the hidden desktop it sometimes is not, and the WidgetManager then has the mouse
  leave at whatever update the timer comes in: a level's board that comes up without the mouse over it waits, suspended,
  for the next mouse move. The call at 0x4506ce answers with the game's own window instead, and the rest of the test (the
  scripted cursor inside the client area) stays the game's. The fourth is the refresh rate's: `SexyAppBase` takes
  `mSyncRefreshRate` (app +0x530) from DirectDraw's refresh rate (0x44d770), 60 on origrun's desktop, and the game's
  vsync updates move the enemies in `UpdateF` by 100 / rate updates' worth a frame. The port's framework has 100, one
  `UpdateF(1.0)` after each update, and the patch at 0x44d779 gives the original 100 too (see "The refresh rate" under
  the deviations). The fifth to seventh start three values the original leaves unset as the port starts them, at 0,
  since what the heap held there changes the game: the enemies' place (the `Enemy` constructor, 0x431550, whose last
  stores move into the padding after it to make room), which the robot reads to place its arms and launchers before it
  sets its own; the robot's foot (its constructor, 0x443210, whose last stores go through `edi` to make room), whose
  stray value made the robot stamp on its first update in every robot fight of the comparison; and the head's target x
  (its constructor, 0x43f310, whose last stores go through `edx`), which its first update eases toward (see "The robot"
  and "The head" under the oddities).
- **The intro's click.** The port takes a script click in the update it is given, and the original reads a posted one
  at its wait after it, so the port is given the click that ends the intro an update later (51 against 50). Both finish
  loading at the end of update 52, and the app's update count is then the same at every frame of the two games, which
  the game reads: the Eye's and the final boss's retargets, the head's bombs and the worm's sand go by it.
- **The input's timing.** origrun posts an input at a wait after update N, and the original reads it at its next
  message check, before update N + 1. It waits, and checks its messages, after five updates in eight (a frame every
  16 ms of the virtual clock, an update every 10 ms), always after the same five counted from its loading, so an input
  posted after one of the other three is held to the next wait and read an update later. `idplay.py` has the port's bot
  give its input only before an update that follows one of the five (`botrecord`'s period and residues), so the original
  reads every input before the same update as the port. It still reads which update took each input from the
  WidgetManager's last-input count (`mLastInputUpdateCnt` +0x64, set from its own count +0x4, which also counts each
  drawn frame) and replays it there.
- **The later levels and the bosses.** A game from a later level starts from a players file the port saved before it
  (`set progress <n>`), which the original reads as it is from its copy's `userdata` with `LastUser` 0, and the game
  goes on through the continue box. A boss scenario also gives both games a `data/levels.xml` with every level 1200
  updates long (only the `length` attributes changed): the port reads it from a copy of its game folder, the original from
  its copy's `data`, put back after the run. The boss then comes 600 updates into the level, and the bot fights it.

<!-- private -->
The game's exe is carved out of the Steam launcher: stage the copy with
`build/origrun-bin/origrun.exe --game heavyweapon --copy-only`, then
`python tools/extract_game_exe.py "<copy>\Heavy Weapon Deluxe.exe" -o "<copy>\HeavyWeaponGame.exe"` (the image after the
launcher's `!popcapdrmprotect!` marker, byte for byte the one exported to `re/heavyweapon/bin`). origrun copies
`Steam.dll` into the copy itself. The carved exe needs nothing else to run: no registration patch.
<!-- /private -->

## The x87's sines

The original's sines, cosines, arc tangents and logs are the x87's `fsin`, `fcos`, `fpatan` and `fyl2x`, inline (143 of
them, the framework's drawing included), and the next operation works on the result where it lies on the x87 stack.
Those four give 64 bits of significand whatever the precision control, which the CRT sets to a double's 53 at startup
(0x4fb065 calls `_controlfp(_PC_53, _MCW_PC)` at 0x502cdd), so the next fadd, fsub, fmul or fdiv rounds once, to a
double. In the runs the original kept 53 bits throughout: the tank's X, which the comparison reads in full, agrees with
the port's in every bit. A C library's function rounds to a double first, and the product after it then differs in its
last bit in about a quarter of cases. Places differed in their last bits, which the tank's lightning, hitting only what
it lands on exactly, turned into a hit an update later (level 13's game parted at frame 2050 on a Strafer near x 0, and
level 13's boss fight on the Wrecker, also near x 0), and the gun's angle, from which the tank's speed near the cursor
comes, moved the tank's X by a few units in the last place for stretches of updates.

`src/X87.cpp` computes the four in double-double arithmetic: fsin and fcos reduce their angle by the x87's own 66-bit pi
(0xc90fdaa22168c234c times 2^-66), fyl2x multiplies by fldln2's 64-bit ln 2; the result is rounded to 64 bits, and the
operators of `X87Value` round the next operation once. Against this machine's x87 (200,000 random arguments each): the
64-bit results differ from the x87's in their last bit in about 3 % of cases (the x87 does not round them correctly),
which the next rounding almost always hides: 2 to 4 products after fcos differ, 1 of the tank's angles in degrees and
7 quotients 2 / log, against 42,000 to 50,000 with the C library. Another processor's x87 may give other last bits.
The native and web builds give the same bits: 300,000 random arguments through each function and operator hash the
same in both.
Each play site follows the original's code: a result kept on the stack is an `X87Value`, one stored and read back is
its `Rounded()`. The drawing code, the credits and the help screen's demo still use the C library; phase two of the
identical-play pass takes them.

## Memory

`memreport` along the tour (`-fast`, natively, 2D): 225 to 232 pictures throughout (41 to 43 MB of pixels) and 92 sounds
(12.5 MB), loaded once; the textures' estimate grows from 2 MB at the title to 24 MB as the levels' pictures are first
drawn. The framework's heap is 84 MB in level 1 and 86 MB at the end of the tour. Eight games in a row (`level 3`,
`lose`, QUIT) leave it at 82.65 MB from the third game on: no game leaves anything behind. The process's private bytes
are 115 MB at the title and menu, 200 MB in level 1, 251 MB at the boss and 268 MB at the end; that growth is below the
game's heap, in the display path (the Direct3D textures and the driver), the shared cost AstroPop and Bejeweled Twist
recorded (`docs/COORDINATION.md`).

## Deviations (each without a visible effect)

- **No keyboard hook.** While the board is up, the original installs a low-level keyboard hook (SetWindowsHookEx
  WH_KEYBOARD_LL, 0x412d70) that swallows the Windows keys, so they cannot pull the player out of a game. The port
  installs no hook: a port never hooks the player's system. The board's own key handling is unchanged.
- **The background animation's flag.** When the board starts a background animation, the original copies the
  animation info's one-byte "plays once" flag (+0x48) with a four-byte read into the animation's int, so the three
  padding bytes above the flag come along. The port copies the flag. Phase one's comparison of the background
  animations checks that the two play the same.
- **An unread counter.** The board's Update raises a counter at app +0xb34 every update, and nothing reads it. The port
  leaves it out.
- **The mask of a reloaded sprite.** When a sprite loads a new image (the level's backgrounds load again for every
  level), the original frees the old alpha mask but keeps the pointer. The port clears it. No reloaded image ever has a
  mask made, so nothing reads the freed mask in the original either.
- **Rand()'s seeding.** The early-2005 `SexyAppBase::Init` (0x45c970) seeds the framework's generator with the 2004
  MTRand seeding. The port seeds it the same way (`HeavyWeaponApp::Init`), and `-seed=N` gives a test run its seed.
  WinMain (0x40e210) then seeds the game's own generator (app +0x1498) from `Rand()`, and so does the port.
- **The kill percentage.** The game-over box and the results screen show (destroyed / spawned x 100) computed on the
  x87, where `_ftol` gives 0x80000000 for the infinity or NaN of a level that spawned nothing. `KillPercentage` gives
  the same value, where a plain conversion is undefined and traps in wasm.
- **The cutscenes.** The original hosts the Flash ActiveX control in its `FlashWidget`. The port plays the movies with
  the shared SWF player: one movie frame every 100 / frame-rate updates, so a test run shows the same frames every time.
- **Message boxes in test runs.** A scripted test run shows no message box, since nobody could close it on the hidden
  desktop. A normal run shows the original's boxes.
- **The window's title.** The original's constructor reads the game's name for the title from its own text (0x52a548).
  The port reads the game's texts from the exe in `Init`, after the window is made, so its constructor writes the same
  name, as the other ports write theirs.
- **The high-score lines' names.** The original builds a new high-score line on its stack without clearing it, so
  the bytes of the name after its end are whatever lay there, and they go into `hiscores.dat`. Nothing reads them. The
  port clears the line first, so the file is the same on every run.
- **Loading.** The original's loading thread loads the images and sounds a few at a time. The port loads them in one go,
  as the framework does for every SexyAppFramework game.
- **The refresh rate.** The game sets `mVSyncUpdates`, so the original moves its enemies once a drawn frame, in
  `UpdateF`, by 100 / (the screen's refresh rate) updates' worth: on a 60 Hz screen a step of 1.667 updates after every
  one or two updates, on a 100 Hz screen one step after each. Over a second they go as far, but between the steps they
  stand at other places, and where an enemy's position decides a random draw (an aircraft firing once it is in range)
  the games part from there. So the original itself plays a different game on screens of different rates. The port's
  framework always runs as on a 100 Hz screen, whatever the player's screen, and the comparison runs the original so
  too (its fourth patch above).
- **Places start at 0.** The original's `Enemy` and `Projectile` constructors leave the place (`mX`, `mY`) unset. The
  robot reads its own before it sets it, to place its arms and launchers, which keep that place until their first
  update after the level's time is up, above the field; the tank's lightning (`StaticStrike`) never sets its own and
  keeps its path in its arrays. The port starts both at 0, so it never reads what the heap held.

## Memory the original leaks, freed in the port

- **The app's resources.** The original's destructor (0x40b980) frees only its vectors and leaves the images, fonts and
  screens to the end of the process. The port frees them.
- **The level loader.** The loader of `levels.xml` leaves its parser, the level's list and the strip image behind. The
  port frees them.
- **The cutscene widget.** The cutscene screen leaves its movie widget allocated after the movie ends. The port deletes it.
- **The title screen's font.** The title screen passes `SetFont` a duplicate of the font, which `SetFont` duplicates
  again. The original never freed the first copy, and the port frees it.
- **The title screen's sprites.** The title screen frees its sprites and leaves the freed pointers in the app's table.
  The port clears them, and the app frees the table at its end.
- **The robot's eye glow.** The robot boss never deletes its eye's glow. The port deletes it with the boss.

## Fix candidates (for the fixes mod)

- **A saved score counts the level bonuses twice.** When the progress is saved between levels, the score that is saved
  already has the level's bonuses, and the bonuses are added to it again. A campaign that is continued from a save starts
  with a higher score than it had.

## Oddities (not chased)

- **BigBomber.** Its first condition reads `mVelX`, and its bomb timer starts uninitialised.
- **The dozer.** Its field at +0x70 is never read.
- **The satellite.** It kills the tank within 32 pixels, even just after it has taken the tank's shield.
- **The enemy tank.** Its step starts at 0.03.
- **The supply helicopter.** It adds `mCraft[0].mIndex` to the level's end blocks.
- **FatBoy.** It kills the tank when its flare timer reaches 6.0, even if the tank is already dead. Its test of the
  shield does not look at the shield's level.
- **The laser's burn mark.** It is drawn only when the beam's start, not its end, is below the top of the field.
- **The laser's bounds.** The laser's tests of the field's bounds run only inside the walk over the enemies.
- **One projectile a hit.** The board's collision pass stops after the first projectile that hits in an update.
- **Enemy bullets counted as missed.** The shots-fired count counts the enemies' bullets that leave the field as the
  player's misses.
- **The board's difficulty.** The board computes `mDifficulty` for every level, and nothing reads it.
- **Projectiles' mirroring.** `HitProjectiles` passes the projectile's own mirroring for the first sprite of its
  collision test.
- **The banners' characters.** A banner indexes its font's map with the text's characters as signed chars. The banners'
  texts are plain ASCII, where signed and unsigned agree.
- **The explosions' frame speed.** `AddExplosion` stores a frame speed that is overwritten before it is read.
- **The far background.** In 2D the far background's second copy is drawn 960 pixels after the first, and in 3D 959.
- **The best survival time.** A player's best survival time is replaced when the game's time before its penalties
  beats it, and what is stored is the time after them. So a game whose time after penalties is worse can still replace a
  better best.
- **The worm's head.** The worm boss takes its head's collision box from one entry past its rib arrays, so the box sits
  at the first rib's y and x 0 instead of at the head. The head almost never kills the tank by touch; the ribs do.
- **The battleship.** Its points are overwritten with 25000, and its turrets' "all turrets present" halving test can
  never pass.
- **The final boss.** It reads `mLaserFire` and never uses it.
- **Fields the data sets and nothing reads.** The head's charge delay and the worm turrets' frame speed.
- **The robot.** Its fire timer counts down only once both its arms are gone. Its foot's y and the Rainer's mine
  timer start uninitialised. The port starts both at 0, since a stray foot y of 410 or more would stomp on the first
  update. In the comparison's robot fights the original's heap held text there, a huge number, and the robot did stamp
  as it came in, above the field: the smash sound, the screen's shake and its random draws. The comparison patches the
  original to start the foot at 0 (its sixth patch). Matching what the heap holds would take the original's allocation
  history.
- **The head.** Its constructor leaves its target x unset, and its first update eases the head toward it. The port
  starts it at 0. The comparison patches the original to start it at 0 too (its seventh patch), so that the head fights
  of levels 7 and 16 start from the same place in both.
- **The wrecker.** Its engine plays at full volume whenever it is within 320 pixels of the middle, and its ball's slam
  throws boulders and dust only on its second encounter (level 12).

## Not translated

- **Library templates**: 0x40fc40 to 0x40fda0 (`std::locale` and container helpers), 0x41e090 to 0x41e760 (the
  credits' vector and list helpers, which the port's `std` containers replace), 0x44b830 (`std::vector<T*>::erase`).
- **0x430580**: the player list's Draw, which only calls `Dialog::Draw`. The port's class inherits it.
- **The framework**: 0x44bc80 and everything after it, which the shared framework replaces.
- **Registration and leaving the game**: the registration and update dialogs, the buy and "more games" pages, and the
  update check.
