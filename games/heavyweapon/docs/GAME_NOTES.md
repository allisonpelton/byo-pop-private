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
| `set score\|lives\|nukes\|level\|timer\|penalty\|progress <v>` | the app's score, tanks left, nukes, next level (1-based), the level's time or survival's time penalty (in updates, as shooting the supply helicopter adds); `progress` puts the player's saved campaign before level v (1-based), as after passing the one before it, and saves it, so the main menu's campaign offers to continue there |
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

## Deviations the user asked for

- **The name without "Deluxe"** (the user, 2026-11-18: "Remove the "Deluxe" text from the main screen of any game it
  may appear in, to avoid misleading people about whose game version they're playing"). The site names the game "Heavy
  Weapon" (`TITLE` in `games/heavyweapon/CMakeLists.txt`). "Heavy Weapon Deluxe" stays where it finds the original's
  files (the save import's folder names, `platform/saveimport.json`), in the game's own copyright notice and in the
  speedrun.com name the Splits window matches (`web/autosplit.js`).
- **No "Deluxe" to cover on the screens** (the same instruction). No screen of the game shows the word: the intro
  movie, the title with its loading bar, the name box and the main menu were looked at in the screen pass, windowed in
  2D and 3D and full screen in 2D (`docs/SCREENS.md`); the logo says "Heavy Weapon". Of the exe's texts only the window's
  caption, the Windows box about a damaged players file, the registration's thanks (removed with the registration) and
  the crash report's questions hold the word. So nothing is cropped or covered.
- **The window's caption "Heavy Weapon 1.0"** (the same instruction: "Remove the "Deluxe" text from the main screen of any
  game it may appear in, to avoid misleading people about whose game version they're playing"). The original's window
  is named "Heavy Weapon Deluxe 1.0" from 0x52a548 and its version; the port's native window drops "Deluxe" alone and
  is named "Heavy Weapon 1.0" (`HeavyWeaponApp.cpp`), and the box it shows when the game's files are missing is named
  "Heavy Weapon" (`main.cpp`). The web build has no
  caption. No shot or frozen hash holds the caption: shots are of the client area alone.

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

## Fixes (the fixes mod)

`mods/fixes/mod.json` holds the fixes mod, each fix a toggle read through the options layer (`Platform::FixOn`,
`platform/GameOptions.h`; `-fix=<id>` on the command line, `-original` for none). Nothing set means every option off,
the original, which is what every test and the frozen baseline run. `main.cpp` loads the mods folder (`-modsdir=`,
`-nomods`; `mods-heavyweapon` next to the exe, or `/hwpersist/mods` on the web, where the page installs the mod).
`tools/fixes_check.sh` plays `tools/fixes_test.txt` four times (as the original, with the mod's defaults, with
`-fix=savedBonuses` and with `-fix=survivalBest`) and checks the saved score and best time in `users.dat`, the one
screen that must differ, the ones that must not, and that no state line changes; then the bot fights level 8's worm as
the original and with `-fix=wormHead`, and the two games must part (on the first run they did at both worms, levels 8
and 17: the fix's game at 17 ended 600 updates sooner). On the web the page puts the mod into the game's mods folder
before the start, and the game's log then names it (`mods: Fixes`; checked in the Browser pane on 2026-11-14).

| id | default | what the original does | what the fix does |
| --- | --- | --- | --- |
| `savedBonuses` | on | the results screen (0x426e60) adds the level's bonuses to the score as it opens, then for the save adds them again, saves the campaign (`SaveUserProgress`) and takes them back, so a campaign continued from the save starts with the bonuses twice, and the high-score line it reaches holds them too | the bonuses are not added a second time for the save (`ResultsScreen.cpp`) |
| `survivalBest` | on | at a survival game's end (`AddHighScore`, 0x407540) the player's best time is replaced when the clock beats it, and what is stored is the time less the penalties (30 seconds for each shot supply helicopter), so a game with a worse time can replace a better best | the time less the penalties is compared (`HeavyWeaponApp.cpp`) |
| `wormHead` | off | the worm's update (0x4442b0) tests each rib against the tank where it is, but its head (120 pixels square, a rib 60) at the place one past its rib arrays gives: x the head's y, y 0, where it never meets the tank; so at the head only the rib beneath it destroys the tank | the head is tested at its own place, where it is drawn (`BossesWorm.cpp`); off by default, since the fight is harder than the one players know and the fault is not known as one |

Candidates looked at and not made, from the oddities below: the enemies' bullets counted as the player's misses (no
screen shows the count); the satellite's beam, which takes a shield and kills the tank in the same update (the shield
would only last one update more, and the beam is plainly meant to beat it); FatBoy's second kill (`KillTank` does
nothing to a tank already dead) and its blast through a shield (a nuke's, by design); the laser's burn mark (a beam
ending above the field would draw it off the screen anyway); the battleship's halving test (what it was meant to test
does not show); the wrecker's engine volume and its boulders on the second encounter, the robot's fire timer and the
battleship's fixed points (each reads as the design); one projectile hit an update (the second is hit in the next).

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
  better best. The fixes mod's `survivalBest` mends it.
- **The worm's head.** The worm boss takes its head's collision box from one entry past its rib arrays, so the box sits
  at the first rib's y and x 0 instead of at the head. The head almost never kills the tank by touch; the ribs do. The
  fixes mod's `wormHead` (off by default) mends it.
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
- **Translucent pixels on the 2D screen.** Sparks, glows and the fire over the high scores blend a few levels apart
  from the original's (its software blitters against SDL's renderer); `docs/IDENTICAL_PLAY.md`, "Oddities", has what
  it would take.
- **The wrecker.** Its engine plays at full volume whenever it is within 320 pixels of the middle, and its ball's slam
  throws boulders and dust only on its second encounter (level 12).

## Not translated

- **Library templates**: 0x40fc40 to 0x40fda0 (`std::locale` and container helpers), 0x41e090 to 0x41e760 (the
  credits' vector and list helpers, which the port's `std` containers replace), 0x44b830 (`std::vector<T*>::erase`).
- **0x430580**: the player list's Draw, which only calls `Dialog::Draw`. The port's class inherits it.
- **The framework**: 0x44bc80 and everything after it, which the shared framework replaces.
- **Registration and leaving the game**: the registration and update dialogs, the buy and "more games" pages, and the
  update check. The trial's boxes at the end of the loading (0x4045d0, called from 0x406ad5, whose last step the port
  keeps), the registration dialog's button handler (0x42d140, slot 9 of the vtable at 0x52f6d0), the update check
  (0x405130, from `Init` at 0x40e0e5 and the update button at 0x40e671), and the Options box's REGISTER and UPDATE
  buttons and its "Auto Updates" checkbox (`OptionsDialog.h`), whose box keeps the original's layout for a registered
  copy. No text, picture or sound is loaded that only these used; the one text they share (0x52a320) is also the label
  of the game-over box's quit button. The main menu has no "more games" button in this version (0x425c40 places the same
  six buttons as the port), so no gap needed closing. The exe's links (the update check, the registration and beta
  pages, DirectX's) are all in what is left out, and no code in the port opens a link.

## The readability pass (2026-11-14)

The port was written to `docs/CODE_STANDARDS.md` from its first draft, so the pass is the tools' runs and a read of what
they flag. `tools/namecheck.py games/heavyweapon` finds nothing in 112 files (24 lines allowed by `namecheck.txt`).
`tools/splitfields.py heavyweapon --export re/heavyweapon/gh/decomp` finds no field declared twice and none read but
never written. Six fields are written and never read: the board's projectiles destroyed, shots fired and shots missed
of the level (the app's statistics keep the totals), its tank speed, a background strip's load flag and a player's best
score (saved, never shown). Their comments now say that nothing reads them; they stay, as the board and the player
record keep the original's layout. Its 14 places where the original's function touches a member that the translation
never names were each read, and none is a missing read or write: the eye boss's bolt delay is in a helper split out of
its update; the final boss's sprites are deleted in its destructor, whose address the tool pins on the next function;
the enemies' sprite is read through `GetCelWidth`; the map's drag (+0x68) is a call through the vtable, not a field; the
map's button handler is called with its listener's `this` (+0x58), so its offsets are other fields'; the background
animations' update starts with a call of `GetBossIndex` whose answer the original throws away, and which changes
nothing; the title's +0x60 is the copy of the app's loading count that nothing reads (left out, `TitleScreen.h`); and
the cutscene's start asks the Flash control whether the movie loaded, which the port's SWF player answers in its own
way ("The cutscenes" in the deviations).

## Coverage check (2026-11-14)

The whole check of `docs/COVERAGE.md`. origrun's `--coverage` recorded which of the original's functions ran in 36
runs on the host, all through idplay on the virtual clock (`tools/idplay/idplay.py original <scenario> --coverage
build/coverage/heavyweapon/functions.txt`, each run's hits kept beside its log as `orig_coverage.txt`): the six
phase-two tours, the 19 boss fights, the six level scenarios, the two campaigns and survival, and two scenarios made
for the check. `coverage_ui` gives the input no other scenario gives: the first name box's OK with no name and the box
that refuses it, the main menu's survival while it is locked, the options' sound sliders dragged and clicked and their
cursor and hints boxes ticked and unticked, the player list's new-player line and that box's CANCEL, the main menu's
help left up through its four turns and its box ticked and unticked, and then level 1's single shots up to the
auto-fire tip, the tip's box and OK, a typed letter, the space bar's pause and resume, the right button's nuke and the
focus lost and regained. `survival_over` is survival's game and then the game-over box's QUIT. idplay's original side
now passes `rclick`, `keydown` and `keyup` on. No run had the real clock: the mixer and the music are the shared
framework's and BASS's, outside the game's code.

`coverage.txt` sets the game's own code as 0x401000-0x44ba10, the shared framework being everything after it. Of its
1050 functions the first 13 runs reached 730 and all 36 reach 801.

Two runs (boss3 and the first `coverage_ui`) went wrong without failing: origrun began reading the game's counter about
20 s late, the game ran on to update 278 or 774, and every input up to then was posted at once. idplay ran the
original again when its first input came late until origrun's own fix reached the trunk (f2e7b161: the counter is read
at once and the game held until its first read); the retry is gone since.

**Reached in the original with no counterpart in the port: 198 after the first runs, 6 more from the later ones, none
at the end.**

- **Left out on purpose: 186, as `none` lines with their reasons.** One Sonnet agent sorted the first 198 (5 minutes).
  182 are what the compiler and the C++ library make: 104 std::vector, list, map and string helpers, 23 of the string
  streams' formatting, 6 locale facet helpers, 6 std::map<std::string> helpers, 3 more string and vector helpers, 37
  scalar deleting destructors, and the destructor bodies of the banner, the bracket button and the armory's arrow
  button, which the port's compiler writes. The other 4 are the shared framework's XMLElement constructor and
  destructor, a ButtonListener stub with no callers (0x40b970), and the player list's Draw ("Not translated").
- **Placed: 17.** ObjectList's Remove and Clear for its other kinds of item, the walk of its destructor, the base
  classes' vtable resets inside Enemy's and Projectile's destructors, and the deleting destructors of the blimp, the two
  helicopters, the air mine and the burst rocket, each address now in the comment of the port function that holds it.
- **Translated: 1.** NukeBlast's destructor (0x428a20) empties its fires and its smoke before the lists' own destructors
  run. The port's implicit destructor freed the same, so nothing changes, but it is now the original's own:
  `tools/check.sh heavyweapon` gives what it gave before it.
- **No missing game logic was found.**

**Defined in the port, never reached: 121 after the first runs, 57 at the end.** The later runs reached 64 of them, the
bosses, the craft and the dialogs above. Of the 57:

- **Read again against the export: 50, no slip.** A second Sonnet agent read 47 (8 minutes): the bosses' kills,
  destructors and sound restores (the ape, the battleship and its turret, the eye, the head, the helicopter's turret,
  the rainer and the wrecker), bosses no run beats (the weak bosses of shots_middle and shots_final are levels 8 and
  9's, 18 and 19's and the final one) and no run pauses before; the mega laser (`UseMegaLaser`, `UpdateMegaLaser`) and
  the ground flash the robot boss's bolt makes; the bolt itself (constructor, update, draw, destructor); the meteors;
  the burst rocket's burst; the kills of the supply helicopter, the blimp, the fat bomber and three ground units; and an
  armory tile's hover, which no run's cursor crossed. One remark stays: the
  bolt's draw takes its sine and cosine from the C library, as the rest of the drawing does ("The x87's sines"), so a
  corner that lands exactly on a whole pixel could stand a pixel from the original's; no run draws it. The other three
  were read here: the one-argument `SwitchScreenMode`, not run because it would change the machine's display mode;
  `FatalError`, which only a failed load reaches; and FlakBurst's destructor, which runs only for a burst still in its
  list when the list is cleared, and only resets the vtable.
- **Reached by nothing in the original, and needed by the port: 4.** The deleting destructors of the app (0x40cff0)
  and of the board's particles (0x42bc10), neither of which is ever deleted through a pointer; the port has only the
  destructor's declaration for each. Enemy's and Projectile's own destructors (0x431600, 0x447810), which only a plain
  Enemy or Projectile would run; none is made, and each derived class has them inline. C++ needs both.
- **Candidates for taking out unneeded code: 3.** The Resize overrides of the cutscene, the map and the results
  screens (0x41ede0, 0x423f40, 0x426bc0). The original's constructors call Widget's Resize and place the children
  themselves, and nothing resizes these screens later, so nothing calls the overrides; the port's map constructor
  calls its own, which does the same. They are out ("Unneeded code taken out" below).

The check took about an hour and a half of machine time, one game process at a time (the port recording the next
scenario beside the original), and the two agents' 340,000 tokens. Every change to the sources but the NukeBlast
destructor was a comment.

## Performance (2026-11-15)

**Result: in 2D, the display both games run in on rundesk's desktop, the port's processor time is about 85% of the
original's over the same stretch of play with its mixer thread counted apart (94% with it), in both window sizes.**
Medians of five alternating rounds (`games/heavyweapon/tools/profile/pairs.sh`, under `tools/pincore.py`), the
process's cycles (every thread, the ones that ended too) over 3000 updates of level 4 in play, in million cycles an
update and as a share of one core (the cycle counter ran at 2081 MHz). The port's build is 01facaa0's, before the
trunk's framework memory merge (2264f0f9, which batches 3D quads and drops pixels after upload):

| | runs | update | draw | present | driver threads | audio thread | total | share of a core | less the audio thread | share |
|---|---|---|---|---|---|---|---|---|---|---|
| original (origrun, real clock, 2D display, window 640x480) | 5 | 0.107 | 2.290 | 0.652 | 0 | 0 | 3.125 | 15.0 % | **3.125** | 15.0 % |
| port, 2D, window of the game's own size (`-windowscale=1`) | 5 | 0.142 | 0.591 | 0.581 | 1.391 | 0.268 | 2.939 | 14.1 % | **2.670** | 12.8 % |
| port, 2D, default window (2x, 1280x960) | 5 | 0.148 | 0.633 | 0.380 | 1.436 | 0.261 | 3.014 | 14.5 % | **2.738** | 13.2 % |

The runs, in round order: the original 2.745 2.924 4.413 3.238 3.125; the port at 1x, less its audio thread, 3.280
2.670 3.186 2.561 2.371; at 2x 5.329 2.738 4.547 2.585 2.313. Round by round the port at 1x is below the original in
four of the five. The port's two highest runs (5.3 and 4.5, both at 2x) are its present: 3.0 and 2.1 M cycles an
update there against 0.3 to 1.0 in the others, the driver's wait for the display on a busy machine. The port's display line says `client 640x480
px, logical 640x480, render scale 1, windowed, renderer direct3d11` (2x: `1280x960 ... render scale 2`). Both games ran
windowed with the registry's `ScreenMode` 0. The original is in 2D because its 3D test fails on rundesk's desktop
(`Test3D`'s FailureReason "DXError - CreateDevice: E_OUTOFMEMORY"), and the port was given `Is3D` 0 to match. The
port's own default is 3D (`Is3D` left unset). Its 3D display at 1x ran three times in the same batch: 5.681, 3.924 and
2.100 M less audio, the first two with presents of 3.9 and 2.0 M. It ran three more times in an earlier batch of the
same stretch, which alternated with the port's 2D runs and had no original: 2.422, 2.564 and 2.367, against 2D's median
of 2.856 in that batch. So on a quiet machine the 3D display costs about 15% less than 2D. The original's 3D display
does not start on rundesk's desktop, so it is not measured.

**How it was measured** (`docs/COORDINATION.md`, "A performance pass").

- **The stretch.** `profile.py` uses idplay's `level4` scenario: the campaign continued before level 4, the continue
  box, the armory and the map, then the port's bot. Its input is as `idplay.py record level4` wrote it. The bot ran once,
  in that recording, and is in no measured run. Both games get the same lines at the same update after their loading:
  the port at its update count from its anchor (52), the original at its update counter (0x56b5b4) from idplay's
  `ORIGINAL_ANCHOR` (52). The stretch is frames 1200 to 4200 of that input, label a to label b.
- **No test machinery.** The player's exe carries only the script's lines and a count line at each end: no
  `-commands`, no state log, no bot, no `-fast`.
- **Counting.** The port uses `-cyclesplit` (SexyApp's three sites). The original uses origrun's `--count` with
  `games/heavyweapon/tools/profile/count.txt`: DoUpdateFrames 0x44dee0, DrawDirtyStuff 0x455b80 less its Redraw, and
  Redraw 0x45d180 (the app's vtable 0x52a570, slots 9, 77 and 78).
- **Running.** Each run ran under pincore's core, the original through origrun (its game under rundesk), the port under
  rundesk, the order of the port's two windows swapped every other round, each run started when `tools/gamecount.py
  --limit 6` had room.
- **The cost of counting.** The port made 11760 readings of 7989 cycles each (1.1% of its total), the original 12994 of
  4266 each (0.6%).
- **Sound.** The port's audio thread is SDL's, mixing for the silent driver under rundesk: 0.22 to 0.28 M an update.
  origrun's stand-in DirectSound mixes nothing, so the original's figure holds no mixer. The table therefore counts the
  port's audio thread apart. The original's one other busy thread, a thread of its exe at 0.12% of a core, is counted in
  its total.

**Three things about the original took several batches to get right**, and each is set in `profile.py`.

- **The refresh patch stays.** idplay's patch sets only the step the original's UpdateF takes (mSyncRefreshRate, read
  from DDInterface +0x78): one UpdateF(1.0) an update, as the port's framework steps. The original's drawing is still
  paced as on a player's 60 Hz screen. With `WaitForVSync`, DrawDirtyStuff sleeps to the display's period between
  draws (DDInterface +0x7c, which no patch touches), which gives 1673 to 1748 draws in the 3000 updates, about 57 a
  second. Without the patch the original takes an UpdateF(1.667) once a drawn frame. Its enemies then stand elsewhere
  from the start, and the bot's recorded input loses the original's tanks at other times. The first batches' originals
  spent much of the stretch with the board stopped, their level time rising by as little as 687 of the 3000.
- **The hints are off in both games.** prep sets the player's FLAG_NO_TIPS, in the port's players file and in the
  original's copy. In the recording a play tip comes up just before frame 1833, and the bot clicks its OK there. In a
  real-time run the original's tip came a few updates later, the click missed, and the board stood still under the box
  to the end of the stretch (diagnostic peeks every 25 frames: the dialog count went to 1 and mSuspended was set
  between frames 1800 and 1850).
- **Real-time input lands late now and then.** In one run 26 of the 961 script lines were posted 2 to 8 updates late.
  The original's play therefore drifts from the port's, from before the stretch: at its start the original's score
  was 100 to 1515 against the port's 275. Its level time rose by 1837 to 2485 against the port's 2603, and its score
  ended at 2145 to 9410 against the port's 9930. One run came close: 9410 with one tank left, as in the port.
  Fewer kills mean fewer explosions to draw, so the drift lowers the original's figure if anything. The port's runs all
  play the stretch alike (level time +2603, score 275 to 9930, lives 2 to 1).

**What the numbers say.** Both games run 100 updates a second.

- **The original** draws 57 times a second at 5.1 M cycles a draw, in its software blits and its DirectDraw presents.
- **The port** presents in step with the display: 1431 to 1797 draws in the 30 s, fewer when the machine is busy.
  That is the framework's vsync pacing, as in `docs/COORDINATION.md`'s "Presenting in step with the display".
- **The driver thread** (nvwgf2um) is the port's largest part: 1.39 M an update, half of its total less audio. It is
  that item's shared cost.
- **The 2D display costs more than 3D.** In the same batches its driver thread is about 0.2 M an update more and its
  draw about 0.17 M more. The likely reason, not measured: in 2D every stretched picture is drawn through the
  framework's software stretch (`mSoftwareStretch`, as the original's 2D screen draws it). That includes each
  explosion, at its own size and frame, so many of those copies are new pictures for the GPU path, which keeps 64 of
  them (`DDInterface::GetStretchCopy`).
- **The game's own update** is 0.142 M against the original's 0.107.

**Changes made for performance:** none. Nothing in the port changed in this pass, so the frozen baseline is as it was.
The port is below the original in 2D, with the margin above, so the pass stands as a measurement.

**Lite candidates:** none added.

**Shared costs sent on:** the driver thread's present cost (already known to the coordinator) and the 2D stretch
copies.

**To measure again:** `python -I games/heavyweapon/tools/profile/profile.py prep build/perf`, then `python -I
tools/pincore.py -- bash games/heavyweapon/tools/profile/pairs.sh build/perf 5 3` and `python -I
games/heavyweapon/tools/profile/profile.py sum build/perf`. `pairs.sh`'s third argument is the number of rounds that
also run the 3D display. The run folders are deleted once this section holds their numbers.

## Unneeded code taken out (2026-11-15)

The three candidates of the coverage check are out of the port: the Resize overrides of the cutscene, the map and the
results screens (0x41ede0, 0x423f40, 0x426bc0), with the `using Widget::Resize` lines that only kept Widget's other
Resize visible beside them. No play of the original reaches any of them, and nothing in the port calls them. The
cutscene's and the results screen's constructors already did what their overrides did. The map's constructor called its
own override, so it now calls Widget's Resize and places the invade link and the briefing's button itself, as the
original's constructor (0x425500) does. 31 lines out and 5 in, over 6 files. The other 54 functions that no run
reached stay: 50 are the game's own rare paths, read again with no slip, and 4 are destructors C++ needs (the coverage
check above). The port's frozen baseline, its tour and the rest of `tools/check.sh heavyweapon` are as before (the
check ran after the removal), and the port builds natively and for the web.

## The PopCap Game Collection version (2026-11-15)

The port was made from the Steam copy (`Heavy Weapon Deluxe.exe`, Steam's launcher with the game inside; the game carved
out of it is `re/heavyweapon/bin/HeavyWeaponGame.exe`). Heavy Weapon Deluxe is on the user's PopCap Game Collection disc,
in the group `HeavyWeaponDeluxeComponent`, so it is checked against that version (`docs/COORDINATION.md`, "Done means").
`unshield` read that group from the disc image's cabinet, which was mounted read-only and dismounted afterwards: 596
files, 12 MB, in the main checkout's `re/_installers/HeavyWeaponDeluxeCollection` (git-ignored). The copy has no
launcher. `HeavyWeapon.exe` is the game itself, beside `bass.dll`, `Flash.ocx`, `license.txt`, `readme.html` and the
folders `data`, `fonts`, `images`, `music`, `properties` and `sounds`.

**It is the same game.**

- **The exe.** BookWorm's `tools/collection/cmpexe.py` set it beside the carved game. Both were linked at the same
  second (16 February 2005, 19:32:40). `.text` and `.data` are the same bytes. `.rdata` differs in seven, a build stamp
  of digits that nothing in the port reads. `.rsrc` holds other icons and version resources. The port reads its texts
  from either exe by the same addresses; `GameText` already looks for `heavyweapon.exe`.
- **The other files.** BookWorm's `cmpfiles.py` finds every one of the game's files that is not a picture byte for byte
  the same: the data, the sounds, the music and the cutscenes. The Collection has no `drm.*` files. Its `license.txt`,
  `readme.html` and `properties/partner.xml` differ, and the port reads none of them.
- **The pictures.** 242 pictures differ in their bytes, because Steam's copy stores many of them in other PNG modes
  (paletted or grey with alpha where the Collection's are RGBA). Decoded, 241 of them are the same: the same alpha in
  every pixel and the same colour in every visible one. Only the colour under fully transparent pixels differs, which
  draws nothing. The one picture that really differs is `images/title.jpg`. The Collection's title has a trademark mark
  after the name and PopCap's newer round logo in the corner, where the Steam copy's logo reads "PopCap games". Its
  layout is the same, and the port shows whichever copy the player has.

**The port runs on it.** The port played level 4's recorded stretch twice, with `-fast`, from the same players file
(the performance pass's input): once from the staged Steam copy and once from the Collection's folder. Both runs'
states at the stretch's two ends are the same in every field (score 275 to 9930, lives 2 to 1, level time 292 to
2895), and shots of the intro and of the level in play match in every pixel. No code needs a second look.

## The autosplit block (2026-11-15)

speedrun.com's "Heavy Weapon" board has one PC category, Mission Mode (PC). Its run starts when the campaign is chosen
from the main menu and ends at the final debriefing. For the site's Splits window and a LiveSplit script, the port
publishes the shared autosplit block (`platform/AutosplitBlock.h`, layout 2, game id `HWDX`) from
`src/LiveSplitObserver.cpp`, whose header comment gives every field and event. The block is written only once something
asks for it: the page's `byopop_livesplit_block`, the `livesplit` mod (`mods/livesplit`), or `-livesplitlog`. Until then
nothing is written, and when it is written it only reads the game.

The board the block reports is a game, from the click that starts it until the main menu or the high scores come back.
The game itself has no record of where a game begins: the campaign begins on the mission map, and the first Board comes
two screens later. So the app keeps a port-only counter (`HeavyWeaponApp::BeginGame` and `EndGame`, marked
`port(autosplit)`). `BeginGame` is called at each click that starts a game: the menu's Campaign for a new player, the
continue dialog's NEW GAME and CONTINUE (the latter resumed), Survival or the help's PLAY before it, and the game-over box's
CONTINUE in survival. `EndGame` is called by `ShowMainMenu` and by the game-over box's SCORES. The game-over box's CONTINUE
in a campaign keeps the same game. The test commands `goto campaign` and `goto survival` begin a game as those clicks do.
`level <n>` outside a campaign begins one as resumed, and the other `goto` screens end it.

Checked: the heavyweapon suite passes with the frozen baseline identical, and its tour and three bot games give the same
screens, states and player files with `-livesplitlog` on as off. The seven block logs made for the site's tests (a new
campaign, the final level's win, a game over continued, a game over to the scores, a saved campaign continued, survival,
a quit) are in the shared notes, `heavyweapon-site/logs`.

Making them found a fault in the test hooks. `goto menu` removed the old menu with a delayed delete, so its destructor
cleared `mMainMenu` after the new menu had set it. The new menu's continue dialog then gave its buttons no listener, and
its CONTINUE crashed (the frozen baseline does the same). The hooks now delete the old menu at once. A menu reached by
play was never affected.

The site's pane check found a second fault in the test hooks: `goto` left a cutscene up. After the final level's results,
`goto campaign` showed the map under the final cutscene, and the bot's `move` kept clicking the cutscene, so the level
never started. `goto` now removes a cutscene as it removes the other screens, and the armory and the map too (left up, they stayed over the next campaign's map and the bot kept answering their buttons). The final cutscene's end (the high scores, or
the menu with no rank) also ends the block's game now, as the high scores do elsewhere.
