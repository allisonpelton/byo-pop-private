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
| `level <n>` | campaign level n (1 to 18) now, with the player's upgrades, without the armory or the map |
| `unlock all` | the player has reached the campaign's end: survival and every level are open |
| `set score\|lives\|nukes\|level\|timer <v>` | the app's score, tanks left, nukes, next level (1-based) or the level's time |
| `win` | before the boss, the level's time jumps to the boss's warning, so the boss comes as at the end of the waves; with the boss up it is killed and dies as by the gun (an error while it is still coming in) |
| `lose` | no tank left after this one, and the tank is killed; the game-over box follows when the tank's wait runs out (an error while no tank is on the field) |
| `seed <n>` | `Rand()` and the game's own generator seeded as at the start with `-seed=n` |
| `move` | the bot's next move (`src/testhooks/Bot.cpp`): `MOVE {x, y, fire, target, input, updates}` in a fight; between fights `MOVE {click, input, updates}` on the button that goes on (a message box's OK, a play tip's OK, the help's OK, the results' ADVANCE, the armory's plus buttons until every point is spent and then GO, the map's link); else `MOVE null` |
| `memreport [file]` | the framework's, `MEMREPORT {...}` |

**The bot.** The tank drives toward where its gun points and fires while the left button is held, so a move is one place
for the pointer and the button. The bot aims at the boss while it can be hit, otherwise at the live enemy nearest the
tank across the field, leading it by the time a bullet (5 pixels an update) takes to reach it. Before aiming it looks
for danger: an enemy bullet or a falling bomb (a bomb's fall modelled as `FallBomb` does it) that would land within 56
pixels of the tank in the next 200 updates, or a satellite charging its beam over the tank, sends the pointer low to the
side away from it, so the tank drives clear; one landing within 120 pixels on the side the tank would drive to stops the
tank. In the armory it spends each point on the lowest weapon on offer. It plays level 1 through without losing a tank
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
  update.
- **The wrecker.** Its engine plays at full volume whenever it is within 320 pixels of the middle, and its ball's slam
  throws boulders and dust only on its second encounter (level 12).

## Not translated

- **Library templates**: 0x40fc40 to 0x40fda0 (`std::locale` and container helpers), 0x41e090 to 0x41e760 (the
  credits' vector and list helpers, which the port's `std` containers replace), 0x44b830 (`std::vector<T*>::erase`).
- **0x430580**: the player list's Draw, which only calls `Dialog::Draw`. The port's class inherits it.
- **The framework**: 0x44bc80 and everything after it, which the shared framework replaces.
- **Registration and leaving the game**: the registration and update dialogs, the buy and "more games" pages, and the
  update check.
