# Typer Shark! Deluxe: behaviour notes (work in progress)

Addresses are virtual addresses in the game program of Typer Shark! Deluxe 1.02 (image base 0x400000, linked 2004-04-20 with
VS.NET 2002 / MSVC 7.0; the Ghidra export is in `re/typershark/gh`, git-ignored). Game code is 0x401000-0x4623f0, about 1,200
functions: the entity classes come first (0x401000-0x412200), then the application and its dialogs, the Board and the screens
from 0x43fbf0 on, and the typing tutor with the small sprite classes (Player, Bubble, BonusText) at the end. The program has
RTTI, which Ghidra did not read; a scan of its own (`re/typershark/rtti.classes.json`) gave every class that has virtual
functions its name, bases and vtables. The classes without virtual functions (the level table, the profile and its manager,
the HUD strip) have no RTTI name: the profile's names come from the game's own trace texts (`UserProfile::...`,
`UserProfileMgr::...`), the others are the port's. The game is built on an internal early-2004 snapshot of the
SexyAppFramework that is older than any public release (BookWorm Deluxe and AstroPop use the same one); `docs/FRAMEWORK_2004.md`
maps it onto the port's framework and `framework/compat2004` holds what the port needs to behave like it.

The port is in `games/typershark/src`: every translated function carries its address in a `// 0xADDR` comment and every field
of a class its original offset. `ASSUMED.md`, `BOARD.md`, `ENTITIES.md`, `JOBS.md`, `LEAKS.md` and `PHASE1.md` in this folder are
the working records the notes below summarise; they are named where they hold more.

## 1. Classes (vtable, constructor, sizeof)

The Board, the screens and the dialogs derive from the 2004 framework's classes (end of this section). The creatures are the
unusual part: the original builds every one of them from two virtual bases, `Enemy` and `GameObject`, so a complete object is
its own part (a vptr, a vbptr, its own fields) followed by the GameObject part (0xf0 bytes) and the Enemy part (0xf0 bytes),
at offsets that differ from class to class; the compiler's `vfNN_subMM` thunks and vtordisp adjusters only fix `this`. The
port uses one plain chain `GameObject <- Enemy <- Shark / Piranha / Projectile / ...` and keeps `virtual` only where a class
is reached twice (`Shark` under `BlackShark`, `StealthShark` and `ToxicShark`'s stealth combinations, `Piranha` under
`WhitePiranha` and `StealthPiranha`); the Board, which holds every creature as an `Enemy*`, reaches a `Shark` or `Piranha`
with `dynamic_cast` where the original adds a virtual-base offset it reads from the vbtable. `ENTITIES.md` has the twelve
GameObject slots, the three Enemy slots, the states and the x87 compare rules; `ENTITIES_BRIEF.md` what the compiler did.

| Class | vtable | ctor | notes |
|---|---|---|---|
| GameObject | 0x529394 (12 virtuals) | 0x4611b0 | the object part is 0xf0 bytes: position, velocity, state, frame counter for the 20-frame sprite sheets, the last six positions |
| Enemy | 0x5298fc (3 virtuals: GetPoints, GetSpeed, TypeLetter) | 0x40a3f0 | dtor 0x4078b0; the word, its colours and fonts, the event queue `mEvents`; Enemy part 0xf0 bytes |
| Shark | own part 1 virtual (0x401200) | 0x4101d0 | sizeof 0x210; GameObject part at +0x30, Enemy at +0x120; states 0 swim, 1 back, 2 dead, 3 dying, 4 chased off, 5 shocked, 6 hit |
| BlackShark | 0x52979c | 0x410f50 | sizeof 0x240; a second word (`mSecondWord`); dtor 0x409d70 |
| RedShark | 0x529838 | 0x411400 | sizeof 0x260; a third word; dtor 0x409970 |
| ToxicShark | 0x5296e8 | 0x4109e0 | sizeof 0x238; plain Shark base; the word scrambles; dtor 0x409bc0 |
| GhostShark | vbtable 0x529fec | 0x4111b0 | Shark, Enemy and GameObject all virtual; only Draw is its own; dtor thunk 0x411350 |
| StealthShark | 0x5299d0 | 0x410770 | sizeof 0x248; half transparent until x = 400 or a hit; dtor 0x409a10 |
| StealthBlackShark | 0x52a05c | 0x411710 | sizeof 0x278; StealthShark + BlackShark; dtor 0x4119b0 |
| StealthRedShark | 0x52a0c8 | 0x411b00 | sizeof 0x290; RedShark + StealthShark; dtor 0x411e00 |
| Piranha | 0x529600 (1 virtual 0x401770) | 0x40fca0 | sizeof 0x210; the part-constructors are 0x401730 and 0x409930 |
| RedPiranha | 0x529ecc | 0x40ff50 | sizeof 0x210; the penalty piranha a typo adds on Hard and above |
| WhitePiranha | 0x529f80 | 0x410c70 | sizeof 0x240; a second word; dtor 0x410ed0 |
| StealthPiranha | 0x529a28 | 0x410530 | sizeof 0x248; dtor 0x409ab0 |
| StealthWhitePiranha | 0x52a140 | 0x411f50 | sizeof 0x278; dtor 0x412200 |
| Projectile | 0x529458 (2 virtuals 0x401820, 0x4018c0) | 0x40acc0 | sizeof 0x208; a shot that flies across the board and tells the Board what it hit |
| Torpedo | GameObject part 0x529638 | 0x40c1a0 | sizeof 0x210; carries a word; the last letter sends it back |
| CannonBall | 0x529c30 | 0x40e400 | sizeof 0x218; one letter on it; dtor 0x40ebb0 |
| SquidCannonBall | 0x529dbc | 0x40f150 | a CannonBall with another hit box |
| MechaTorpedo | 0x529cec | 0x40ea50 | sizeof 0x218; a Torpedo with another hit box |
| ProjectileBoss | 0x52996c (5 virtuals) | see header | Enemy part at +0; bobs, fires, owns its shots (`mShots`, `mFinishedShots`) |
| TorpedoBoss | 0x529a70 (7 virtuals) | 0x40c630 | GameObject part at +0x1b8; part dtor 0x408d00; fires Torpedos |
| GhostShip | 0x529ca8 | 0x40e600 | GameObject part at +0x1d0; fires CannonBalls with one letter each; part dtor 0x409920 |
| MechaShark | 0x529d78 | 0x40ebe0 | no fields of its own; fires MechaTorpedos; part dtor 0x40ef10 |
| MechaSquid | 0x529e78 | 0x40f390 | GameObject part at +0x1d8; fires SquidCannonBalls; part dtor 0x40f6d0 |
| Clam | 0x529bfc | 0x40e100 | sizeof 0x1f0; the sea-floor clam of the treasure level |
| BonusCreature | 0x529bb0 | 0x40db40 | sizeof 0x1f8; the jellyfish with a word; dtor 0x40dd60 |
| BackgroundFish | 0x5296fc | 0x405fc0 (new), 0x401df0 (copy) | GameObject only; sizeof 0xf8; deleting dtor 0x461e80 is shared with Bubble and Player |
| Bubble | 0x5319d4 | 0x461d50 | GameObject only; sizeof 0x100 |
| BonusText | 0x531a08 | 0x4621a0 | GameObject only; sizeof 0x120; the floating "+N PTS" text |
| Player | 0x53198c | 0x461320 | the diver and the score; GameObject only; sizeof 0x148; the Board keeps it at +0xd68 |
| HudBar | none | 0x441730 | sizeof 0x1c; the strip across the top; Update 0x441760, Draw 0x449910 |
| WinWSApp | 0x52d734 (67 slots) | 0x4373b0 | the application; sizeof 0x930; dtor body 0x4366a0, deleting 0x437960 |
| Board | 0x5309a4 (47 slots) | 0x4544b0 | `new(0xf98)`; a Widget; 53 methods in 0x43fbf0-0x45e755 (`BOARD.md`); dtor body 0x451700, deleting 0x454e10 |
| LessonManager (the level table) | none | 0x4270e0 | sizeof 0xc0; dtor body 0x421c50; the config reader is 0x42bfb0; not the typing tutor's class of the same RTTI name |
| TitleScreen | 0x52f2cc | 0x448e10 | sizeof 0x7c; the loading screen |
| MenuScreen | 0x52f04c | 0x4436e0 | dtor body 0x4410f0; six buttons and two links |
| MapScreen | 0x530a7c | 0x458d10 | sizeof 0x238; Draw 0x447400, Update 0x45ad20; deleting dtor 0x45acf0 |
| MapNode | none | 0x450e80 | 0x3c bytes; one dive site |
| HighScoreScreen | 0x52b0c4 | built inline in 0x41bbd0 | sizeof 100; no constructor or destructor of its own |
| StatsScreen | 0x52f20c | 0x45baa0 | Widget and ButtonListener; dtor body 0x4412a0 |
| TabbedWidget, TabPanel | 0x5308e4, 0x52f6cc | 0x451090, 0x450cd0 | sizeof 0xc4 and 0x8c |
| TutorialPanel, AdventurePanel, AbyssPanel | 0x52fdd4, 0x530804, 0x53072c | 0x453ba0, 0x450df0, 0x450d40 | the stats screen's three tabs; sizeof 0x174, 0x90, 0x90 |
| StatusBar | 0x52f3c4 | 0x448a40 | sizeof 0x88; the strip under the board with the pause, quit and options buttons |
| XWidget, TooltipWidget, WSTimer | 0x52f60c, 0x52f484, 0x529464 | 0x4435a0, 0x443010, see header | the map's site widget (to check), the tooltip (sizeof 0xa8), and the timer widget (sizeof 0x64; nine of them in the Board) |
| FadeButton, SoundSlider | 0x52f544, 0x52f10c | 0x443440, 0x441230 | sizeof 0xfc and 0x88 |
| TypingTutor | 0x531544 | 0x460b30 | sizeof 0x264; Widget and ButtonListener (at +0x58); dtor body 0x45bd30 |
| TutorLessons (RTTI `Sexy::LessonManager`) | 0x52c728 | inlined in 0x460b30 | the tutor's lesson list; dtor body 0x427300, deleting 0x429900 |
| UserProfile | none | 0x421f50, 0x4222d0 | sizeof 0x2c8; one player |
| UserProfileMgr ("UserManager") | none | 0x427440 | sizeof 0x34c; dtor 0x41f090; the players, the current name, the two score tables |
| KeyDialog | 0x52ec4c (52 slots) | 0x43ec30 | the plain message and question box |
| HintDialog | 0x52da6c | 0x439ef0 | Dialog and CheckboxListener; dtor 0x4380e0 |
| OptionsDialog | 0x52dd7c | 0x43bf10 | Dialog, SliderListener, CheckboxListener |
| NewUserDialog, SelectUserDialog, DeleteUserDialog, AbyssModeDialog | 0x52dc74, 0x52e364, 0x52e074, 0x52df7c | 0x43bcc0, 0x43edb0, 0x43dcb0, 0x43e7a0 | the original's MoneyDialog base with an edit or list listener |
| DifficultyDialog | 0x52e46c | 0x43f0f0 | KeyDialog and EditListener; sizeof 0x158 |
| GameOverDialog | 0x52e61c | 0x43adb0 | Dialog |
| WinGameDialog | 0x52d94c | 0x43a4f0 | Dialog; the end-of-adventure box |

The framework classes under all of this are the 2004 snapshot's, not the public 1.2 or 1.3 ones: `SexyApp` (the early-2004
`SexyAppBase`, with the registry under `HKLM\SOFTWARE\PopCap\TyperShark` and the loading thread), a `Widget` of 0x58 bytes with
the 2004 vtable order (`docs/FRAMEWORK_2004.md`, section 3), `Dialog` (52 slots;
its buttons answer `id + 2000` for yes, ok or the footer and `id + 3000` for no or cancel, sent to the app's
`ButtonListener`), `ButtonWidget`, `DialogButton`, `Checkbox`, `Slider`, `EditWidget`, `ListWidget`, `HyperlinkWidget`,
`MTRand` (the 2004 `SRand` seeds by `mt[i] = mt[i-1] * 69069`) and the C library's `rand`. The port keeps its own 1.3-based
framework and links `sexycompat2004` for what differs: `Dialog2004` and `Slider2004` (the old button ids and layout),
`CrtRand` / `CrtSRand` (MSVC's `rand`, which the piranha's first frame uses), `SeedMTRand2004` (the game generator's
seeding). The dialogs are `Dialog2004`; the two generators are the game's `gGameRand` (the original's `DAT_00565384`, an
`MTRand` made in `Init`) and the C library's, and each draw keeps the one the original used.

## 2. Application (WinWSApp, 0x4373b0)

`WinMain` (0x401000) makes the app at 640 x 480, calls `Init`, `Start` and `Shutdown` and deletes it; `main.cpp` does the
same through the port's framework and passes the remaining `-key=value` arguments to the app. The app owns everything long
lived: the image table (one `Image*` for each id of `Res.h`, which `make_res.py` writes from the resource table of the
program), 14 fonts, the sound instances, a second module-music interface (`mMusic`, +0x658, for the two songs
`ChangeMusic` 0x414350 alternates), the player list, the current player, the Board and the screens. The game's texts are not
in the repository: `GameText.h` reads them at start-up from the player's own `WinTS.exe` by address (`GT(0x0052xxxx)`) through
a table made by `make_gametext.py` (598 entries; the 40 that only registration, trial, update or outbound-link code used are
left out, see section 5). Game data (pictures, sounds, music, the level and word files, `properties`) are staged from the
player's install by `stage_typershark_assets.py` into the git-ignored `assets/typershark`.

**Files.** `WinWSApp.cpp` (constructor, `Init`, the fonts, the loading screen, `LoadingThreadProc` 0x435560),
`WinWSAppLoad.cpp` (the data-file loaders), `WinWSAppButtons.cpp` (`ButtonDepress` 0x431630, the one switch that every
button and dialog answer reaches), `WinWSAppScreens.cpp` (`ShowMenuScreen` 0x41d5c0, `ShowMapScreen` 0x426440,
`StartBoard` 0x431320, `ShowOptions` 0x414150, the dialog openers), `WinWSAppGame.cpp` (`ResetGame` 0x42a870,
`CreateBoard` 0x42f3c0, `RestartBoard` 0x42ee10, `GetLongWord` 0x430b20, the word and theme pickers), `main.cpp`, and
`testhooks/` (section 9).

**Loading.** The program loads the images, the derived images, the sounds, the five data files (the level config for the
chosen difficulty, `waves`, `twords`, the gem and ship names, `themes`) in `LoadingThreadProc`; `NUM_LOADING_TASKS` is 0x137
and the title screen's bar is `GetLoadingThreadProgress()`. When it ends the app presses button 0x191, which is what the
title's continue button pressed, and goes on to the menu by itself (the registered copy of the original does the same; the
continue button is gone from the port, section 5). The port runs the same procedure to the end in the framework's loading
step, with no wait added.

**Button ids** (a click comes back to `ButtonDepress`): menu 0x64 Adventure, 0x65 Abyss, 0x66 typing tutor, 0x67 high scores,
0x68 quit (asks first), 0x6b options, links 0x69 (the player list) and 0x6a (the statistics); the difficulty dialog's five
buttons 600 + difficulty (0 Easy, 1 Normal, 2 Hard, 3 Expert, 4 X-Treme); the board's status bar 0xc8 pause, 0xc9 quit,
0xca the Enter key, 0xcb options; the continue link on the board 300 (0x12c, back to the map); the map's start buttons 0x1cf3
and 0x85d; tutor buttons 0x58 next, 0x59 previous, 0x5a menu, 0x5b options; a dialog with id `D` answers `D + 2000` and
`D + 3000`.

**Saved games and profiles.** A player is a `UserProfile` (sizeof 0x2c8) in the file `users\user<N>.dat` (a Buffer of
strings: the game version, every number as text; six statistic groups of four ints and 35 records of twelve; the 19
tutor-lesson marks; gems, secrets and treasure words; the saved-game fields; the map pairs and visited sites; the kill
counts), the registry key `Users\<name>\...` (values such as the space-bar-pauses setting, the hints, the last lesson, the
tutorial mark and the deepest abyss depth, and `ad<N>` doubles written as `%f`; `Users\User` names the current player) and the high-score tables in `users\user.idx` (the version, one
number drawn from the game generator, then ten entries of the adventure table and ten of the abyss table). A saved adventure
game is the profile's level (1 to 35), difficulty and site; a saved abyss game uses numbers above those; the menu's Adventure
button asks whether to continue one. The registry lives in the port's emulation (`registry.ini`, `docs/WEB_PLAYER.md`).
Under origrun the original cannot keep a registry value (section 8).

## 3. The Board (0x43fbf0-0x45e755; `Board.h` has the original's offsets on every field)

The Board is a Widget of 0xf98 bytes; `BOARD.md` maps its 53 methods to the files `Board.cpp`, `BoardUpdate.cpp`,
`BoardEvents.cpp`, `BoardWaves.cpp`, `BoardKeys.cpp`, `BoardGame.cpp` and `BoardDraw.cpp`, and lists the 51 fields still
named by their offset with what the code does with each. The game is driven by one 10 ms update (origrun's
`update_ms=10`).

**Levels and waves.** `LessonManager` (not the tutor's) reads `data\<difficulty>-cfg.xml` or `abyss.xml` at the start of a
game: the default settings, then levels, each with waves (`Sharks`, `Piranhas`, `Bonus` or `Boss`) and each wave with enemy
records (a type from blue to mecha squid, a speed range, a word length range `MinDiff`..`MaxDiff`, how many come, a theme, a
boss's hit points and shot numbers). The adventure has 36 sites (0 to 35) on the map and then endless levels (level -1); the
cursor is `NextWave` 0x41eb40 and `NextLevel`. A bonus (treasure) level is a level with a `BonusLevel` tag. `SpawnWave`
(0x455bf0) builds the creatures of the wave: it makes the class for each record, draws the word from the word lists by length
and theme (`GetWord` 0x428d80, `GetThemeWord`), places them (`PlaceEnemies` 0x452050, `PlaceBonusCreatures` 0x442540) and builds
the boss by its type. Black and red sharks get further words from the lists (`mSecondWord`, `mThirdWord`), a white piranha one more
letter.

**Creatures.** Sharks swim in from the right, speed up near the left edge and bite the diver when they reach him (a life
is lost); piranhas dash across; stealth variants start half transparent and turn solid at x = 400; toxic sharks scramble the
letters not yet typed; ghost sharks are drawn with a glow; bonus creatures (jellyfish) drift up and are worth a bonus; the
bosses (TorpedoBoss, MechaShark, GhostShip, MechaSquid) float in, bob, wait and fire shots that carry words or single letters
and must be typed to turn them back; a boss has hit points. Typing is `KeyChar` 0x454e40 with `TryLetter` 0x443ae0: the first
letter locks on the first creature in the order sharks, piranhas, bonus creatures, boss shots whose word begins with it;
afterwards only that word's next letter counts, a wrong letter is a typo (a banner, the creature flashes, and in a piranha wave
`NumForPenalty` typos without a kill add a penalty piranha, `AddPiranha` 0x452a60). The Return key fires the EMP when the charge
(0.01 for each scored word) has reached 1.0.

**The diver (Player).** His state drives the Board: 0 normal, 1 climbing down, 2 and 10 beaten picture, 3 beaten (dust cloud),
4 on the floor, 5 the end-of-level streak game (or the clam game of a treasure level), 6 waiting (or the treasure shown), 7 and
0xc level won (space returns to the map), 8 game over, 9 afraid (a boss is on the screen). The states are read from
`Player.cpp` and `BoardUpdate.cpp`; the numbers are the original's, the names are the notes' (to check against the
export for 7, 0xb and 0xc).

**Events.** Creatures talk to the Board only by pushing small integers on `Enemy::mEvents`, which the five handlers
(`HandleSharkEvents` 0x457480, `HandlePiranhaEvents` 0x457ad0, `HandleClamEvents` 0x458030, `HandleBonusCreatureEvents`
0x4582c0, `HandleBossEvents` 0x4586b0) drain: 1 the word is finished (scoring), 2 reached the diver, 3 a pearl clam opened,
4 and 7 the left-edge reaction, 5 left the screen (the creature is deleted), 6 died (the group may be over), 8 a stealth
creature shows up (30 bubbles), 0xa a boss exists, 0xc a boss is beaten, 0x13 a shot hit the diver, 0x15 a shot is gone.
Codes 9, 0xb, 0xd and 0x14 have no handler. `BOARD.md`, part 5, has the table.

**Scoring.** A finished word scores the creature's points (`Enemy::mWordPoints`), times `max(2, score / 600)` in an abyss game,
and the Board adds them to the diver, the rating and a floating text; the EMP charge grows by 0.01. The diver gets a life
each time the score reaches the next threshold (the first is 450000, 525000, 650000, 700000 or 900000 by difficulty, then
larger steps; `Player::AddScore`). A group of words ends when every creature of the lists has finished dying; the Board shows
a rating popup, counts a ramp (`mAirGauge`) up to 240, and the diver enters the streak game: long words (`GetLongWord`,
lengths from the level's `MinEndDiff` to `MaxEndDiff`) come one after another, each finished word bringing the next, against
a countdown (`mAirGauge` again, 0.1 an update, 0.12 in a treasure level, shortened by typing) and earn a streak bonus; then
come the level bonus, the treasure picture and the space bar. Typing speed and accuracy go into the profile's records at the end (`SaveProgress` 0x440580).

**Bonus (treasure) level.** The floor holds 22 clams in four rows, each with a letter and a kind (0 empty, 1 and 2 pearls
that pair up). The diver is in state 5: a letter opens the clam it is written on; two clams of the same pearl kind make a pair
(100000 points for the first pair, 200000 for the next ones), an empty clam closes at once with the wrong sound and takes the
open clam with it; after two pairs the next pearl that opens shows the treasure (`ShowTreasure` 0x442760). `KeyChar` does the
pairing itself; the clam handler's event 3 only repeats it and never fires on its own (section 7, to check).

**The abyss.** No map: the dive goes down in feet (`mScore` of the Board is the depth), the level changes every 200 feet
(`SetDepth` 0x452cf0, `Update` 0x45c140), `SetDepth` makes the depth ruler's labels, a boss comes by `mAbyssBossIndex % 4`, the
diver has no lives (0), and the deepest depth is kept in the profile (`AnnoDomini`). The unregistered copy stopped an abyss
game at a depth limit; that code is gone.

## 4. Screens and dialogs

| Screen | What it is |
|---|---|
| Title (loading) | the loading bar over the title picture; hands over to the menu when loading ends |
| Menu | the six buttons (Adventure, Abyss, typing tutor, high scores, quit, options) and the two links (the player list, the statistics); the first run opens the new-player dialog |
| Map | the 36 dive sites joined by links; the ship sails to the chosen site; sites 5, 18, 20 and 22 are dead-end treasure levels; site 35 is the last; a dead end takes the link out of the site before it so the ship sails back |
| Board | the game; status bar under it; the HUD strip on top (level or depth, points, lives) |
| Stats | the tabbed statistics screen: tutorial, adventure and abyss panels (`TabbedWidget`, `TabPanel`) and a close button (0x450) |
| High scores | the adventure table (score, mode label, name) and the abyss table (name, depth), from `users\user.idx`; close button 0xccf |
| Typing tutor | lessons 0 to 18 (lesson 0 is the typing test) from `data\lesson<N>.xml`: tasks of lines to type with the hands picture showing the finger and key, creatures swimming behind, words per minute and accuracy at the end |

Dialogs (ids): new player 0x44d, player list 0x44e, delete player 0x44f, options 0x30a (music and effect sliders, the check
boxes, a credits button that opens the credits box 0x30c), difficulty 0x309, the abyss continue question 0x451, hint box
0x3f3 (with a check box to switch hints off), plain message or question 500 (the quit question, the continue-a-saved-game
question, the typing-test question), game over 0x16062, end-of-adventure 0x8d (answers: return home or keep going into the
endless levels), and 0x30b, the message shown when the desktop is not 16 or 32 bit colour (0x41d030). The test hooks name
these `newuser`, `selectuser`, `deleteuser`, `options`, `credits`, `difficulty`, `abyssmode`, `hint`, `message`, `gameover`,
`wingame` and `nowindowed`.

## 5. Deviations from the original (each without a visible effect)

**Memory leaks fixed** (`LEAKS.md`, with the proof). The original never frees the profile a `SaveUser` replaces (0x420c60;
fix F1: the port deletes it), a boss shot that finished (0x40b360 takes it off the boss's list and does not delete it; F2:
the port keeps it on `Board::mRetired` and deletes it with the Board, because `mTarget` may still point to it), a bonus
creature that left through the top (F3, same list), the game generator (F4: deleted at the end of `~WinWSApp`), a typing-tutor
exercise whose `<TASK>` is not closed on a line of its own (0x429930; F6: lesson 4 has two, so every load leaked two
exercises), the Courier font, the Board's timer at +0x58, the abyss depth markers it forgets, the delete-player dialog's list,
the timers of `ProjectileBoss`, `TutorialPanel` and `TypingTutor`, and the lesson file it never closes. F5 was a record growth
that turned out not to be a leak (section 7). Each fix only frees memory the game never reads again; the batches in `LEAKS.md`
(12 scenarios, the whole adventure of 200,600 updates, the tutor, the menus, the select-player flow) give state logs and
screenshots identical to the tree before the fixes. What is left: the graphics driver's own memory, SDL's 100 or so blocks
that the shared `SexyAppBase` never frees (no `SDL_Quit`; a spec for the coordinator is in `LEAKS.md`), and the framework's
two statics.

**Fields the original leaves unset, which the port zeroes.** `Player`'s constructor leaves `mHoseFrameUp`, `mHoseY` and
`mAlpha` unset (0x461320; the one place where play could differ), the rating popup's `mBonus` and `mNeverWritten`, `Projectile`'s
`+0x14` and `+0x18`, `GameOverDialog`'s `+0x11c` until the Board fills it, `FadeButton`'s `+0xe4`, members of `MapScreen`,
`StatusBar` (+0x64), `TitleScreen` (+0x58 to +0x70), `TabbedWidget`, `TutorialPanel` and `StatsScreen` until their later
functions set them, the typing tutor's fields until `ResetLesson`, `OptionsDialog`'s `+0x124` (its `RemovedFromManager` hands it
to `RemoveWidget`; the port passes NULL), `UserProfile` `+0x1c8` (neither constructor stores it; this one reaches the file,
section 7), the boss numbers (fire interval, hit points, missile counts, bob and shot speeds) and field 0x04 of the enemy
records and the bonus flag and first-wave index of the level records in the copies of levels 30 to 35 that the config reader
makes for the endless play (the original's heap blocks are not cleared; see section 7), and the stack slot that holds a wave's
creature count when the wave type is not 0, 1 or 2 (`BoardUpdate.cpp` near 0x45c140 and `SpawnWave`; the port uses 0). In the
same group, a kind of creature the wave maker does not know keeps the pointer of the pass before in the original; the port
skips it.

**Registration, trial, update-check and outbound-link code removed** (commit a99fde67; CLAUDE.md "Order of the work"). Gone, not
disabled: the register dialog and its picture, the trial's play-limit and play-time count, the update-check flags, the Board's
`mMaxUnregAbyssDepth` and the block of `Update` that stopped an unregistered abyss game, the trial limit in the tutor's next
lesson, the register prompt of the high score screen, the title screen's hidden continue button and its pictures (the
registered path is the only one), and 40 texts that only that code used (`EXCLUDE` in `make_gametext.py`; 598 are left;
their pictures are not staged). No button leads out of the game. Sixteen scenarios (menus, levels 1 to 3, three abyss games,
the tutor, an idle game, the adventure to level 34) gave byte-identical state logs and hash-identical screenshots before
and after.

**Loading** runs in one go: the same `LoadingThreadProc`, no wait, no minimum time (CLAUDE.md, "Making a port").

**Test hooks never change a normal run.** `-statelog`, `-typist`, the standard commands and the counting build
(`-DTS_LEAKCOUNT=ON`, off by default) are reached only from a test; without their options the cost is one integer test in
`DoUpdateFrames`.

**Fixes of the original that the port makes (a bug that breaks play):** `GetLongWord` (0x430b20) draws a word of the level's
end lengths until it finds one not yet used; after 1882 words (Easy) every word is used and the loop never ends in the
original. The port empties the set of used words when it holds every word of the lengths that can be drawn, draws no number
for it, and changes nothing while an unused word is left; the state log is byte-identical to the unfixed port's for the first
153195 updates of the adventure (the coordinator, 2026-11-09; `PHASE1.md`, oddity 1).

**Translation errors found and fixed by phase 1** (the port had differed from the original; now it matches). `BlackShark` and
`RedShark` `TypeLetter` (0x409fb0, 0x40b600): the original assigns the empty string to the further word it just took, which
Ghidra's export leaves out, so the port's shark kept loading its second word for ever and the level never ended; fixed with
`mSecondWord = ""` and `mThirdWord = ""`. `Board::Update` (0x45c140): the abyss goes to the next level every 200 feet, the port did it
every 100; fixed (`/ 200` in the two tests).

**Quirks of the original that the port keeps** (address, what it does, how to see it):

- Lives read -1 after the last life (`PlayerEaten` 0x44fe30): it subtracts one and a value below 0 ends the game. An idle game
  reads 2, 1, 0, -1.
- The abyss starts with 0 lives (`SetLessonManager` 0x451cb0). Play an abyss game and read `lives`.
- The game timer keeps counting while a hint dialog pauses play. The first hint opens at board update 139 and stays about 22
  updates; read `time` in the state.
- `BackgroundFish`'s copy constructor (0x401df0) computes the follower's x as `-(rand % width + width)` in unsigned arithmetic
  and stores it as a double (about 4.29e9), so `Board::Update` deletes it at its first tick: only the leader of a school ever
  shows (section 10, the `schools` toggle).
- `Player::Update` (0x461740) state 1 has a branch for `y < 235` that the preceding test makes unreachable; the BlackShark
  state-5 branch in `SetState` is unreachable the same way.
- `UserProfile::LoadDataFromDisk` (0x422630) appends the records it reads after the 35 the constructor made, so a loaded
  profile holds 70 per group and `WriteDataToDisk` (0x4172f0) writes only the first 35; `WriteDataToDisk` stores `mSavedScoreBackup`
  where the loader reads `mSavedScore`; `RegisterDefaultUserName` (0x419da0) always returns false. See a profile reloaded
  and saved again (section 10: a record fix would be invisible, so no toggle).
- Map (`MapScreen.cpp`): `ResetScroll` 0x441030 sets both scroll targets from the width difference where the final panel's
  draw test (0x447cd3) compares with the height difference; site 35 has the link -1 and is kept away from `Update` 0x45af60 and
  `SetVisible` 0x450ec0 only by the final-level flag; `OnLevelChosen` 0x4538e0 reads the list at size - 2; a dead end moves
  the site before it behind it (section 10: the final panel was judged in phase 2 and is no fault).
- Typing tutor: `KeyChar` 0x460680 takes a Return, newline or space at the first or last character of a line as neither right
  nor wrong; `Update` 0x4601a0 has a creature branch above y = 160 in state 1 that the loop never takes; the seconds timer
  restarts at each second so an overshoot is lost; a lesson that ends within its first second divides by zero in `Finish`
  and both speeds read 0. Phase 1 played lessons 13, 15 and 17 (lines that start or end with a space) and each ended when the key
  shown was typed (`PHASE1.md`, H1 item 7); what a player sees when a different key is typed there was not tried.
- Tutor pictures: `SetupHighlight` 0x45e810 lights the 9 key's picture for the keys 8 and `*`, leaves `{ } _ |` without the shift
  key, gives `\ _ |` no finger (section 10, the `tutorKeyboard` toggle), and the draw 0x44b400 places the lines before and after the typed line by their index in the
  task, so from the fifth line of a task on (when `mFirstLine` is above 0) they sit too low (section 10: no lesson shows it).
- Lesson and config readers: the character read last is handled twice; a tutor text file's line count for the random pick
  includes the failed read, so the line picked can be the one past the end; a `<HEADING>` number is not checked; the level
  config is read a character at a time and a line reaches the XML parser only once it ends, so the file's last line counts only
  if it ends; `LevelNum` is parsed and not kept.
- `LessonManager`: `NextWave` divides by a count that is 0 for an empty list; `LastWaveIsBoss` reads the element before the
  first of a level without waves; `SetLevel(36 or more)` starts `NextLevel` at index -1, so the level reached is the argument
  minus 36 (the map never passes a larger one).
- `OptionsDialog`: the button text colour (0x438660) goes to the credits button as index 3 of its own colours, not its label
  colour; its `ButtonPress` (0x438a80) plays the click and does not call the base (section 10: no visible difference, not a toggle).
- `DrawWavyImage` 0x441ac0 draws (height - y) rows when the picture ends below the screen; `_ftol` (0x509890) keeps the low 32
  bits of the truncated 64-bit value and gives 0 for a NaN or out-of-range value (the port's `Ftol` helper does the same).
- `HighScoreScreen::Draw`: a mode other than 0 to 4 repeats the label of the line before. The abyss lines have no label.
- `DeleteUserDialog::DeleteSelectedUser` (0x43df60) reads the first line of the list unchecked after the deletion (section 7).
- `AvoidWordOverlap` 0x446110 counts its loop by the size of the bonus-text list (+0xdb8) and indexes the bonus-creature list
  (+0xdc8) with it; the port stops at the end of the list it indexes (section 7).
- Events 9, 0xb, 0xd and 0x14 have no handler in the Board; `HandleClamEvents` event 3 never scores on its own.

**A frame is drawn whole (the shared framework; found by phase 2).** The original's widget manager repaints only the widgets
that are marked dirty, on top of the picture that is already there (`WidgetManager::DrawScreen` 0x47d790). The shared
framework draws a frame whole or not at all (`SexyAppBase::DrawDirtyStuff`: a GPU back buffer does not keep the last frame),
so whenever anything is dirty every widget's `Draw` runs. Two things follow, both below what shotdiff counts as visible:

- *The status bar's meter edge.* While the board itself is not repainted but the bar is (the mouse rests on one of the
  bar's three buttons, whose `FadeButton::Update` marks what lies under it dirty every update; the bonus level's intro and
  the level-end tally, where `StatusBar::Update` marks the bar and `Board::Update` returns early), the original paints the
  bar's picture over itself again and again, and its half-transparent pixels (one row under the charge meter and one
  column right of it, 250 pixels) end up as the picture's own colour. The port paints the board first and the bar once,
  so those pixels keep one blend with what lies under them: 10 to 24 levels of 255 darker. Measured on the bonus intro:
  the bar's picture there is (144, 157, 158) at 83% opacity over black, the port shows (121, 132, 133), one blend, the
  original (141, 154, 155), two or more.
- *The sea's two strips, the rating popup and the map's last scroll ease once a draw* (`Board::Draw` 0x44d200,
  `MapScreen::Draw` 0x447400: the average of the last six changes of the target, a draw at a time). In the original a
  strip stops easing when the board is no longer repainted and can rest a pixel short; in the port the board is drawn
  with every frame and the strip reaches its place. In the phase 2 tours that is a handful of sea pixels a shot (at most
  22, at most 15 levels apart). With one draw an update the port's eased values equal the original's exactly: the map's
  last scroll read -4.0 at target -5 up to -60/-58 at rest in both, at all 16 updates sampled.

**No focus comes back when a modal dialog closes** (`WinWSApp::AddDialog`; found by phase 2's statistics tour). The 2004
framework's `SetBaseModal` (0x439200) takes the focus from the widget under the dialog and `KillDialog` (0x482f90) removes
the dialog, handing the focus to nobody; the game sets it itself where it wants one. The shared framework is 1.3, whose
`RemoveBaseModal` returns the focus to the widget that had it before, so the typing tutor, which stands still while it has no
focus (`TypingTutor::LostFocus` 0x442020), ran on under the statistics screen. `AddDialog` now forgets the widget the
framework would refocus. No other screen of the 547 shots of the dialog tours changed; a spec for the shared framework (a
flag for the 2004 behaviour) is with the coordinator.

## 6. Comparison with the original

**Phase 1 (plays identical), `PHASE1.md`.** The original ran on origrun's virtual clock with the seed pinned; its memory was
read at chosen updates (`at <N> peek`); the port ran with `-fast -seed=S -statelog`; `tools/phase1/cmp.py` compared the two
update by update, field by field: update count, score, lives, the level timer's three raw values, pause and hint flags, the
typed letters, the dive, the diver's state, the EMP charge, the game generator's index (every number drawn), and for each
creature its kind, word, x, y, state and letters typed, in list order, plus the clam pairs, the end-of-level countdown, the
difficulty, the typo counter and the wave type. No pictures. Both started from the same `users` folder and the same menu
flow (new player "Tester", no typing test, Adventure, Easy).

Round 1 (equal unless noted): the menu to the first board; level 1 with seeds 1, 2 and 3 (about 7900 updates each); levels 1 to
3 (seeds 1, 2, 3); an idle game to game over; typos in levels 1 and 2; black sharks in levels 3 and 4 (equal after fix 1);
the first 25 levels of an Easy adventure to update 100000 (TorpedoBoss, MechaShark, all the shark and piranha variants; equal
after fix 1) and on to level 34 (MechaSquid, 153190 updates, 0 of 11064 reads differ, including the 264 numbers the generator
draws for the last unused end-of-level word); the EMP; the abyss (seed 1 to 484 feet, equal after fix 2; seeds 2 and 3 to 544
feet, 8965 updates each); the typing tutor lessons 1, 13, 15 and 17 to their ends. Round 2: a treasure level with its 22
clams (pearls, pairs, the 100000 and 200000 bonuses, the treasure, the way back); an idle Easy game; the tutor's lesson 12
with a wrong key every seventh key; Hard with every fifth letter wrong and mixed clam play; Expert penalty piranhas (up to 17
on the screen); Hard with two wrong letters in a row (penalty piranhas); the tutor's lesson 15 with wrong keys; and the whole
Easy adventure by a route of its own through every boss and the final level 35, the end-of-adventure dialog, and the
endless levels (to update 200000). Equal in every field read; the `users` folders at the end of the scenarios that compared them
(Expert and Hard penalty runs, the whole adventure) are equal byte for byte.

Two differences were found and fixed (section 5): the second and third word of the black and red shark, and the abyss's
level step. One oddity was found and later fixed in the port: the end-of-level word picker that never ends (section 5). The
H1 review's bug list was judged against the original: lives read -1 after the last life (the original does the same), abyss
lives start at 0 (the same), the game timer counts through a hint pause (the same), the clam bonus's score tick (not reached
in round 1; round 2's treasure level is equal, but the run that leaves the diver waiting long on the floor, `bonwait_s1`,
had not finished: open), the map after level 5 landing on level 1 (not reproduced), a fresh
profile without the tutorial mark never getting waves from level 5 (the first group of level 1 sets the mark; a level jump on a
fresh profile was not compared), and the tutor soft-lock candidate (not a soft lock, section 5).

Not compared (as `PHASE1.md` stands): seeds 2 and 3 of the round-2 scenarios (written, not finished), Normal and X-Treme (the
same code with other config numbers), a saved adventure and the continue question (the original under origrun keeps no
registry value), the registry values the port writes, hint dialogs, pause, option values, switching and deleting players, a
fresh profile with a level jump, and the files written at the end of a game other than by the byte comparison of the scenarios
named above.

**Phase 2 (looks identical), `IDENTICAL_PLAY.md`.** Tours written once and run on both programs (the original in Windows
Sandbox on origrun's virtual clock, the port with `-fast -drawevery`, shots at the same updates, compared by shotdiff): the menus
and dialogs, a saved adventure, an idle game, the abyss, two tutor lessons, Expert with penalty piranhas, Hard with treasure
levels, and the whole Easy adventure with its maps, bosses, end dialog and endless levels: 1919 pairs, none differs visibly. The
599 faint boxes are of three kinds, all judged (section 5's "a frame is drawn whole" for two of them; the window's last column
where a creature enters is an oddity, `IDENTICAL_PLAY.md`). Port faults the tours found and fixed: the cannon balls' frame
rectangle, the tutor's finger rings. The sound comparison (origrun's recording stand-in and a logging BASS against the port's
sound log) found the sound effects equal play for play and the music missing: the module player was never registered, and
`ChangeMusic` cut a song off where the original fades it out; after both fixes the music is equal event for event (850 events).
The web build draws the same pictures as the native port to within 6 levels of 255 (1114 pairs under Node), plays the module, and
passes the page check. Code played every mode with real mouse and key events, natively (windowed and full screen) and in the
page. The port's shots come from the test command `shot` (section 9), because the framework's `screenshot` draws once more and a
draw is a step in this game.

## 7. Oddities (what the original does that the port does not copy)

- **An uninitialised number in a new profile's file.** `UserProfile` `+0x1c8` (the level of a saved adventure game) is never
  stored by either constructor and `WriteDataToDisk` writes it as it is; a profile that only ran the typing tutor holds the
  heap's leftover (for example 137381704, or -2139062144, the heap's 0x80808080 fill), the port holds 0, so the file is 8 to 10 bytes longer in
  the original and one number differs. It is read back only together with a saved-game level, which is set in the same call that
  stores it, so nothing plays differently. To see: a new player, run a tutor lesson, compare `user0.dat` (`PHASE1.md`, round 2
  oddity 1; `endcmp.py`).
- **The endless levels' copies.** After the config of the adventure is read, the original copies levels 30 to 35 (index 0x1e
  on) into the table the endless play draws from, and the copies' heap blocks are not cleared: the boss numbers of an enemy
  record (fire interval, hit points, missile counts, speeds) and the bonus flag and first-wave index of a level are whatever
  the heap held; the port's are 0. Levels 34 and 35 hold a boss wave (the MechaSquid and the ghost ship on Easy). The
  endless `NextWave` draws only non-boss waves, but `NextLevel` draws the first wave of the first endless level without that
  test, so a boss wave with leftover numbers can come there (the copies of levels 34 and 35 have 10 and 15 waves, one boss wave
  in each, so the chance is 1 in 36 for a game that reaches the endless play, in each of the five config files). The
  whole-adventure run of phase 1 went on into the endless levels to update 200000 and was equal, which
  does not say the leftovers were not zero there; to check in a run that draws more endless levels. The bonus flag and
  first-wave index of an endless copy are not read (`IsBonusLevel` reads the main table), so only the boss numbers matter.
- **The tutor's speed and accuracy before a lesson ends.** The words per minute (raw and adjusted) and the accuracy (+0x8c,
  +0x90, +0x9c) are written when the lesson ends; until then the original reads its allocation's contents and the port holds
  0. Nothing shows them earlier; the comparer leaves them out until the lesson's state is 2.
- **The load finishes at an update that depends on the machine.** The loading thread runs on the wall clock, so the original's
  menu is ready between about update 800 and 1550 on a quiet machine and later on a busy one (section 8).
- **The original keeps no registry value under origrun** (section 8); not a game difference.
- **`UserProfile::LoadDataFromDisk` record growth** (the `LEAKS.md` item F5): the 35 default records plus 35 loaded ones, held
  and not leaked (section 5, section 10).
- **Deleting the only player.** After `DeleteSelectedUser` erases the deleted name from the list it reads `mLines[0]`; if
  that player was the only one the list is empty and the original reads past the vector (its first element pointer is
  NULL), so a crash or garbage is likely; the port's `std::vector::operator[]` on an empty vector is undefined as well. To
  check whether the dialog offers the last player for deletion, and what the original does; if it crashes it is a bug that
  breaks play and belongs in the port, not in the mod.
- **`AvoidWordOverlap` (0x446110) in a bonus wave.** The original loops `size(bonusTexts)` times over the bonus-creature
  list; with more floating texts than bonus creatures it indexes past the list's end. The port stops at the end. The
  adventure and treasure scenarios of phase 1 (which run bonus waves) are equal, so none of those runs showed a difference;
  what the original reads there was not looked at.
- **A clam event that never fires.** `HandleClamEvents` event 3 (a pearl clam began to open) pairs it with `mOpenClam`, but
  `KeyChar` has already paired or recorded the clam and cleared `mOpenClam`; the event then finds `mOpenClam` NULL (or the
  clam itself, or a clam of another kind) and does nothing. No run of the port's showed it scoring (to check in the export).
- **Piranha event 5 leaves `mTarget` alone** while the shark loop clears it. Not a dangling pointer: `KeyChar` resets `mTarget`
  to NULL for a piranha at once ("a piranha takes one letter"), so a piranha is never the target when it leaves.

## 8. Running the original through origrun

The original runs only through `tools/origrun` (`docs/ORIGRUN.md`), from a copy under `build/origrun/typershark/game` that
origrun prepares from the player's install (`tools/origrun/games.tsv` has the row); an install folder is never run in place.
`docs/ORIGRUN.md`, "Per-game status", has its row.

<!-- private: The program file in that copy comes from the install; tools/origrun prepares it. -->

The install's `WinTS.exe` is the 2006-08-22 PopCap DRM wrapper (marker `!popcapdrmprotect!`, Flash trial screens, the Steam2
API). The game program is the PE image that starts at file offset 0x591a2 and runs to the end of the file (1,495,040 bytes);
`tools/extract_game_exe.py` carves it unchanged into `re/typershark/bin/TyperSharkGame.exe`, and the copy under
`build/origrun/typershark/game` gets the same file next to the install's other files. The wrapper's own screens and registry
use are not needed for a run.

<!-- /private -->

`games/typershark/tools/run_original.sh <script> <log> [seed] [timeout] [origrun options...]` is the one way the notes' tools
start it. It waits, for up to 30 minutes, until `tools/gamecount.py --limit 3` says the machine has room for another game
process, then runs `build/origrun-bin/origrun.exe --run typershark --clock virtual --seed S --counter "[0x565380]+0x4b8"
--patches tools/origrun/patches/typershark.txt --script <script> --timeout T`, and appends `exit=N` to the log. The clock is
virtual (no waiting; the game spins through updates), the run is invisible and silent, and the original counts as a game
process. `tools/origrun/typershark.cfg` and the patch list are git-ignored.

*The update counter* is `[0x565380]+0x4b8`: the app global `gApp` (0x565380) and `SexyAppBase::mUpdateCount` (+0x4b8,
incremented in `SexyApp::UpdateFrames`); one update is 10 ms (`update_ms=10` in the cfg). *The scripts* are origrun scripts:
`at <N> click|key|text ...` at the counter, `at <N> peek <expr>...` to log values read from the game's memory at update N
(exact on the virtual clock; `phase1/peek_expr.py` reads expressions such as `BRD+0xf24`), `quit` last. An input at counter N
is read by update N+1; a port `-script` line N runs before update N (`tsp.PORT_SHIFT` makes the two line up). A `Space` key
has to be sent as `key 32` (origrun's `text` trims it).

*The seed* is pinned by the patch list, which replaces the two places in `SexyApp::Init` that seed the generators from
`GetTickCount()` with the value of `--seed`, so a run repeats; the port's `-seed=N` (or the `seed` command) seeds `gGameRand`
through `SeedMTRand2004` and the C library generator through `CrtSRand` the same way.

<!-- private: The patch list is git-ignored; tools/origrun applies it to the copy. -->

The patch list `tools/origrun/patches/typershark.txt` holds four lines. Two are "test" patches: `CheckRegistryAccess`
(0x41ac70) has its first six bytes `558bec6aff68` replaced by `b801000000c3` (`mov eax,1; ret`), because the game checks that
it can write `HKLM\SOFTWARE\PopCap\TyperShark` and refuses to start when origrun, which is not elevated, cannot; and in
`RegistryGetSubKeys` (0x4bc235) the conditional jump `0f858a010000` becomes `0f8574010000`, so that a registry key that is not
there answers true with an empty list. The other two are "seed" patches at 0x4b8ece and 0x4b910e, each replacing the six bytes
`ff15bc905200` (a call of `GetTickCount` through the import at 0x5290bc) with `b8{seed}90` (`mov eax, seed; nop`) for
`SRand` and `srand` in `SexyApp::Init`.

<!-- /private -->

*The registry.* The original's `RegistryWrite` (0x4b1be0) opens and creates its key under `HKEY_LOCAL_MACHINE`
(`SOFTWARE\PopCap\TyperShark`); origrun is not elevated, and the patch list answers the key checks as if all were fine, so no
registry value of the original (the settings, the current player, the per-player values such as the last lesson or the tutorial
mark) survives a run or shows in `reg export`. The original always starts with an empty registry, and a "quit and start again,
then continue" run cannot find the player. The scenarios start both programs from an empty registry, so none of this shows in
a state, but the port's registry values cannot be compared with the original's here, and a saved game cannot be continued in
the original. A spec for origrun is in the phase 1 report.

*The loading thread's flake.* The original's loading thread runs on the wall clock while its main thread spins through
updates (tens of thousands a second on the virtual clock), so the load ends at an update that depends on the machine and its
load. On a busy machine the menu clicks of a scenario, scheduled for update 4000, can come before the game has loaded and are
lost: the original then draws no number (generator index 624) and has no board. `cmp.py` runs the original again, up to three
times, when it saw no board (or no tutor) at any peek, and says the original never loaded the menu if nothing helped;
`TSBASE=<update>` moves the whole menu flow later (12000, 24000 or 40000 were used), `TSORIGTIMEOUT=<seconds>` lengthens the
240 s limit of one run (a long adventure under load needs 600 s or more).

*The three-process limit.* At most three game processes at a time, counted across every session (`tools/gamecount.py`).
`cmp.py` waits up to 20 minutes for room before each game process. Two runs of the original at once would share the copy's
`users` folder and registry key, so `queue.sh <jobs file>` runs the jobs one after another; `runorig.sh` restores the copy's
`users` folder from `build/ts-p1/fixtures/users-clean` and clears its key before each run. A stopped python or bash parent leaves
its origrun and game running: `mine.ps1 -Kill` lists and stops the processes of such runs.

## 9. Test commands (docs/TEST_COMMANDS.md; `src/testhooks/TestCommands.cpp`)

Script commands of the port (`-script`, `-commands`, and on the web the test page's command call), answered on stdout with
one line (`OK <command>`, `ERROR <command>: <reason>`, `STATE {...}`, `MOVE {...}`). A command that is never given changes
nothing and draws no random number; a comparison with the original never uses one inside the stretch it compares.

| Command | Does |
|---|---|
| `state` | `STATE {...}`, see below |
| `move` | `MOVE {...}` or `MOVE null`: the bot's next move |
| `seed <n>` | seeds `gGameRand` (`SeedMTRand2004`) and the C library generator (`CrtSRand`) |
| `goto <screen>` | `title`, `menu`, `options`, `credits`, `highscores`, `stats`, `tutor`, `map`, `adventure [0..4]` (the difficulty; default 2, Hard) and `abyss` (from the surface), through the game's own buttons; a player named Tester is made (or loaded) when there is none |
| `level <n>` | in the adventure site 0 to 35 (the map is sailed as `OnLevelChosen` and the start button do; a fresh profile is given the tutorial mark first), in the abyss a depth in feet (`SetDepth`), in the tutor a lesson 0 to 18 (the next and previous buttons) |
| `unlock all` | the 19 tutor-lesson marks, the tutorial mark, and a deepest abyss depth of at least 2400 |
| `set score\|lives\|timer\|depth\|emp <n>` | the diver's score, lives (`mLives`), the game timer (milliseconds), the depth, the EMP charge (0 to 1); `set hints 0\|1` and `set tutorial 0\|1` need a player. `enemy` is read-only |
| `win` | the level is won now: the diver goes to the level-won state (0xc) as `Update` does at the bottom, with the level bonus; ERROR in the abyss, in a won or lost game |
| `lose` | the last life goes now: `mLives` is set to 0 and `PlayerEaten` runs, which subtracts one and opens the game-over dialog |
| `shot <file.png>` | the screen as the last draw left it, read back and saved without drawing again (needs `-drawevery`). The framework's `screenshot` draws every widget once more, and here a draw is a step: `Board::Draw` eases the two strips of sea and the rating popup once a draw, `MapScreen::Draw` the final scroll, and a widget painted over itself blends its half-transparent pixels again. The phase 2 tours take their shots with it |

*The state.* `screen` (the topmost dialog's name, else `title`, `stats`, `highscores`, `tutor`, `board`, `map`, `menu`, `none`)
and `base` (the screen under a dialog); `dialogHeader` and `dialogLines`; `user`; `mode` (`adventure`, `abyss`, `tutor`);
`difficulty`, `level` (the site; `endless` the endless count when the level is -1) and `wave`; with a Board: `score`, `lives`,
`depth` (feet), `timer` (ms), `typeTime`, `diver` (the diver's state), `emp`, `paused`, `won`, `enemy` (the creatures
on the screen), and `board`: `width`, `typed`, `clear` (no wave on the screen), `divePos`, `floorY`, `oceanDepth`, `canType`,
`banner`, and `creatures`, each `{kind, list (sharks, piranhas, clams, bonus, boss, shots), word, typed, x, y, state, target,
type (a clam's kind), hp (a boss)}`; in the tutor `tutor`: `state`, `task`, `errors`, `correct`, `wpm`, `adjustedWpm`,
`accuracy`, `timeLeft`, `expected` (the key that is next); on the map `map`: `chosen`, `arrived`, `final`, `chests`; and `over`
(a game-over or end-of-adventure dialog, or a finished tutor lesson).

*The state log* (`-statelog=<file>`, `testhooks/StateLog.cpp`; the file's own header comment lists only its first fields)
writes one line per update while a Board exists, fields separated by spaces, then ` |` and the creatures:
`u` the app's update count; `lvl` the level (-1 endless, -2 none); `score`; `time` the game timer in ms; `sharks`, `piranhas`,
`clams`, `bonus` the list sizes and `boss` 0 or 1; `typed` the letters typed on the target; `lives`; `bupd` the Board's own
update count; `bscore` its score (the depth in an abyss game); `tcnt`, `tstart`, `tpaused` the game timer's raw update count,
start and paused total; `paused`, `hint` (a hint dialog is open); `ty` the typed string; `dive` the dive position (six
decimals); `dstate` the diver's state; `emp` the EMP charge (six decimals); `rng` the game generator's index (numbers drawn;
624 before the first draw); `gameover` the game-over dialog exists; `sw`, `swp` the streak word and the position typed; `pairs`
the clam pairs found; `open` the index of the open clam or -1; `f38` the end-of-level countdown (six decimals); `diff` the
difficulty; `e94` the typos since the last kill; `cwt` the lesson's current wave type. Each creature is
`kind:word@x,y,state,typed` (x and y with six decimals); a clam's last number packs its kind, frame, opened flag, pulse
direction and tick (`kind*100000 + frame*1000 + 100*opened + 10*pulseUp + tick%4`). The order is sharks, piranhas, clams, bonus
creatures, the boss, the boss's shots, each list in creation order. With no Board the line is
`u=N noboard=1 rng=N`, and while the tutor is open ` tut=1 tles tst ttask terr tclean tcorr trwpm tawpm tsec tacc texp tleft
tfirst tcre tpause`.

*The typist* (`-typist=<file>[;every=K][;start=U][;stop=U][;typo=N][;burst=B][;enter=U][;want=a+b+c][;streak=N][;wait=N]
[;tutorerr=N][;win=yes|no][;over=yes|no][;confirm=yes|no][;clams=mixed]`, `testhooks/Typist.cpp`) is the port's bot playing
by script lines, which it writes to the file as `<update> char <c>` and `<update> click <x> <y>`, so the same input can be
replayed in the original: a letter every K updates; no input before update `start` or from `stop` on; every Nth letter wrong
(`typo`), the last B of every N wrong (`burst`, wrong letters in a row for a penalty piranha); Enter once at update U (the
EMP); at a fork of the map take the chest of the first site of the list that is on offer (`want`); at most N words of the
streak game a level (`streak`); N updates before the space bar that leaves a won level (`wait`); every Nth tutor key wrong
(`tutorerr`); the button of the end-of-adventure, game-over and two-button dialogs (`win`, `over`, `confirm`); and a fixed
pseudo-random clam pattern in a treasure level (`clams=mixed`) in place of the bot's ideal play.

*The move bot* (`testhooks/Bot.cpp`, `ChooseMove`) is a pure function of a snapshot of the Board (`Snapshot.cpp`), draws no
random number and holds no state, so a bot mod can reuse it as it is. It continues the word of the creature the Board is locked
on; otherwise it takes the creature that threatens soonest (the shark, piranha or boss shot nearest the left edge that can be
typed), a bonus creature only when none of those can be typed, the letter that `KeyChar` would pick; in the streak game it
types the streak word from where it is; in a treasure level it opens clams in pairs; between games it answers the dialog, the
map and the "continue" screens. `MOVE {"what","word","typing","input":[{"key":"A"}...]}` lists real keys (`" "` for the space,
`"Enter"`) and clicks in game coordinates; `MOVE null` when there is nothing to type.

*How `tools/phase1` drives both programs.* A scenario (`sc_*.py`) lists inputs at update numbers (`STEPS`), the range to compare
(`LO`, `HI`) and optionally a typist option string. `cmp.py <scenario> --seed S [--step K] [--ranges lo:hi:step,...]` runs the
original (through `runorig.sh` and `run_original.sh`: two passes, one for the creature lists' sizes and each creature's class,
one for the fields at that class's offsets) and the port (`-fast -seed -script -statelog` under rundesk, from a copy of the same
`users` fixture), turns the typist's written input into `at <update-1> text c` lines for the original, and prints the first
differing updates and a verdict line. `queue.sh` runs a jobs file one job at a time; `endcmp.py <tag>` compares the registry
and `users` folders at the end; `show.py`, `summ.py`, `peek_expr.py` and `peek_more.py` are for finding the cause of a
difference. `tools/leak` holds the counting build's scripts (`build.sh`, `scenarios.py`, `runall.sh`, `leaksites.py`), whose
scratch commands (`leak`, `press`, `dyes`, `dno`, `dlist`, `dname`) exist only in that build. `tools/regress.py` is a
regression check on the port's own play (it was uncommitted when these notes were written; to check its state).

## 10. The fixes mod and the fix candidates (2026-11-09)

`games/typershark/mods/fixes/mod.json` is the game's fixes mod (`docs/MOD_ROADMAP.md`, "The fixes mod"): one mod, each setting a
toggle for one bug of the original that the port keeps. A toggle starts on, except where the fix changes something that may be meant
(the user's rule for every fixes mod, 2026-11-11: "Off by default for functionality that seems intended but where the bug is not well
known to be a bug"): `schools` starts off, since a lone background fish reads as the intended look and the missing followers are not a
known bug (`"default": false` in mod.json; the player turns it on in the mod menu); `tutorKeyboard` starts on, since a key lighting
the wrong picture is a plain error. The site serves the game with the mod on (`typershark-fixes/` beside the page); a native build
finds it when the folder is copied into the mods folder (`-modsdir=<dir>` with `fixes/mod.json` inside). The tools that need every
fix on (`tools/fixshots.py` and the phase-1 comparison's `TSMODSDIR`, `tools/phase1/cmp.py`) write a `mod-settings.json` into the
run's user folder that turns every toggle of the mod on. The port without the mod is the original: `FixEnabled` (`src/Fixes.cpp`) is false for every
toggle unless the mod is enabled, and each toggle is read where its fix matters, as a live value. With the mod on the game
plays as it does without it: `tools/regress.py` with `TSMODSDIR=<dir>` (tools/phase1/cmp.py gives it as `-modsdir`) hashes the
same state logs as without it, so no fix changes what a game draws from the generator. `tools/fixshots.py schools|tutor` takes
shots at the same updates with the mod off and on and lists the rectangles that differ.

| Toggle | The original | With the fix |
|---|---|---|
| `schools` | The follower constructor of `BackgroundFish` (0x401df0) puts a follower at x = `-(rand % width + width)` computed unsigned, about 4.29e9 as a double, and `Board::Update` (0x45c140) deletes anything past the right edge at its first tick, so only the leader of a school (0 to 4 fish every 400 updates) ever shows. | The x is computed signed: the followers swim in behind the leader. The fish count that the 400-update test (`size < 8`) reads counts only leaders (the original's followers are gone in the tick they are made), so every draw of the generator stays where it was. |
| `tutorKeyboard` | The keyboard picture (0x45e810): the keys 8 and `*` light the 9 key's picture at the 8 key's place; `{ } _ |` are typed with shift but do not light the shift key; the backslash, `_` and `|` get no finger or ring. Lessons 9, 10, 14, 16 and 17 hold an 8, lessons 10, 14 and 16 a `*`, lesson 14 the braces and `_` (lesson 16 one `_` too); no lesson holds a bar or a backslash. | The 8 key lights its own picture; the braces and `_` light the shift key and `_` gets the right little finger and its ring (the bar and the backslash are left alone: no lesson shows them). `tools/fixshots.py tutor --lesson 14` shows the pictures change. |

**Looked at, not made into toggles.**
- *The tutor's lines* (the draw 0x44b400 places the lines before and after the line being typed by their index in the task, not their
  place among the four shown): reachable only in a task of six lines or more, since `mFirstLine` is set at every fourth line and the lines
  drawn run from it. The longest task of the 19 lessons has five lines, so the original never shows it. Not a toggle.
- *The start of a level* (the black fade-in that `Update` counts down from 255 by 2 every other update, about 2.5 s, and the diver's
  climb down in state 1, 1.2 to 2.4 s, while `KeyChar` ignores every key) and *the end of a group* (`mAirGauge` ramps 0.96 an update
  to 240 before the diver enters the streak game or the treasure, 2.5 s): the game's own animations and rhythm, part of its pace, as
  in Alchemy's notes. They also move every later update of a level, so the draws of the background bubbles and fish and the wave
  starts would move with them. They stay.
- *The profile's records* (`LoadDataFromDisk` 0x422630, `WriteDataToDisk` 0x4172f0): a loaded profile appends its 35 records per
  statistic group after the constructor's 35 zero ones and the writer writes only the first 35, so the per-level speed, accuracy and
  streak records of earlier sessions are not kept. Nothing in the port reads `mRecords` except the writer and the loader, so nothing
  on screen changes, and a fix would only change the bytes of `user<N>.dat`. Not a toggle: nothing a player can see.
- *The options dialog's Credits button* (0x438660, 0x438a80; the text colour reaches the button as index 3 of its own colours and the
  click handler does not call the base): the phase 2 menu tour finds every Options picture, hovered and pressed, equal to the
  original's, so the label is the same white as Done's and there is nothing to fix.
- Checked and not candidates: the piranha event 5 (no dangling target, section 7); lives reading -1, abyss lives 0 and the hint pause
  not stopping the timer (the same in the original, and they do not hold the player); the streak game and the treasure level's own
  timers (the game's rules, not waits); the registration code (gone).

**Judged in phase 2.** *The map's final panel* (`ResetScroll` 0x441030 against the draw test 0x447cd3): both scroll targets
are set from the width difference where the draw test compares with the height difference. The map picture is 700 by 540 and the
window 640 by 480, so both differences are -60, the test holds and the ship shows. The original's final panel and the port's are
equal in the whole-adventure tour (the panel after level 35 and through the endless levels, 52 pairs), and the eased scroll reads
the same values in both (section 5). No fault, nothing for the mod. Nothing was found in the reading done for the sharks, piranhas, clams,
bonus creatures and bosses (equal to the original in phase 1's runs, which does not exclude a wait) and the high score tables; the
dialogs and screens other than the options dialog were not read for waits or faults.

## 11. Performance

The performance pass (2026-11-11). The port and the original were measured turn about over the same scripted play, the port was
profiled in real time and without waiting, its memory was split by `memreport` and `tools/memmap.py`, and the web build was measured under
Node and in the Browser pane. **No change was made to the port**: the profile finds no cost in the game's own code (below), and what it
does find is in shared code and in the graphics driver; those went to the coordinator as a spec (section "Shared costs"). The machine was
never quiet: one to three games of other sessions ran during every measurement and 24 to 66% of it was in use (`machine_busy`), so the
figures are for comparing the two side by side, run by run; each cell lists the three pairs.

**How.** `tools/profile/perf.py` starts a game and samples its process (every 0.1 s in real time, every 0.02 s without waiting): working
set, private bytes, processor time; `tools/profile/pairs.sh` runs everything turn about, one game process at a time, and
`tools/profile/summary.py` prints a line a run. The scenario is `build/ts-p2/rec/rt2.in.txt` (the adventure recording that phase 2 made with
`tools/phase2/rec.py`: a new player named Tester, the Adventure button, Easy, the hint box, then the port's own typist playing the first
levels from update 1,269), cut at update 7,000 in real time (70 s, "the stretch" is its last 2,000 updates, 20 s) and at 30,000 without
waiting (the stretch is its last 10,000). In real time the port runs without `-fast` and the original on origrun's real clock; without
waiting the port runs with `-fast` (it draws every tenth update) and again with `-fast -drawevery` (every update, which is what the
original does on origrun's virtual clock, so that is the comparison of like with like) and the original on the virtual clock. The port is
started under rundesk, the original by origrun (host run, scripted cursor, `--reg-map popcap`, a fresh users folder and registry key each
run). The original loads in a thread on the wall clock: its script waits for the app's loading-completed flag (the byte at +0x525; `waitfor`
reads a dword, so the wait is on the dword at +0x522 being 0x1000000 or more), marks the counter there and counts every update from the
mark, and the load time is the time from the process's creation to that flag, read out of the game's memory. Each original run logs the
board's state at the stretch's start and end (the peeks of phase 2); every run that counts shows the same four values, and a run whose
game never got into play (no sharks) says `NOT PLAYED`.

Two pitfalls of the first attempt, kept so they are not repeated. On the virtual clock the counter runs on while the loading thread works,
so the counter at the flag was 1,556 in one run, 2,279 in another and 48,415 in a third, and in the last two the game sat in a state in
which the scripted name and clicks did nothing (no sharks; 0.05 ms an update, 20,000 updates a second): the no-waiting runs of the original
use phase 2's loading patch (`build/ts-p2/patches.txt`: the copy loads on the main thread, so that the first typed letters meet a loaded
game; the stretch is the same game, checked by the state values) and only the load times come from the unpatched copy. And one of the nine
real-time runs of the port (in the first, discarded set of pairs) ran at 74.5 updates a second instead of 98 to 99.5, the whole run
taking 89.7 s instead of 71 s and using less processor time, on an otherwise idle machine; it was not seen again in the nine runs of the
final set and is not explained (Windows' timer resolution on the hidden desktop is the first suspect).

**In real time** (three pairs, 04:15 to 04:45 on 2026-11-11; "software" is the port with `SDL_RENDER_DRIVER=software`, a test of the
renderer's cost and not a setting of the game):

| | Original | Port | Port, software renderer |
| --- | --- | --- | --- |
| Processor time in the stretch (20 s) | 1.48, 1.62 and 1.55 s | 3.33, 1.28 and 2.22 s | 17.6, 16.9 and 17.6 s |
| Updates a second in the stretch | 99.8, 99.5 and 100.0 | 99.5, 97.3 and 99.4 | 99.7, 98.8 and 99.4 |
| Processor time, whole run (71 s) | 5.5, 5.9 and 5.7 s | 9.9, 4.4 and 8.1 s | 53.3, 53.2 and 54.7 s |
| Peak private bytes / working set | 49.4 to 51.4 / 64.5 to 73.6 MB | 86.2 to 89.0 / 93.6 to 96.6 MB | 64.2 to 67.6 / 84.3 to 87.5 MB |

**Without waiting** (30,000 updates; the stretch is updates 20,000 to 30,000):

| | Original (virtual clock, loading patch) | Port `-fast` | Port `-fast -drawevery` | Port `-fast -drawevery`, software |
| --- | --- | --- | --- | --- |
| Processor time in the stretch | 7.4, 8.0 and 14.8 s | 0.47, 0.45 and 0.86 s | 2.1, 2.6 and 2.9 s | 103, 115 and 134 s |
| Updates a second | 1,092, 972 and 567 | 17,398, 19,841 and 7,965 | 5,912, 4,912 and 4,481 | 96, 86 and 73 |
| Processor time, whole run | 22.7, 23.8 and 33.8 s | 2.3, 2.1 and 3.7 s | 7.3, 8.8 and 11.3 s | 293, 302 and 407 s |
| Peak private bytes / working set | 54.5 to 55.1 / 76.7 to 77.7 MB | 95.5 to 101.8 / 103 to 109 MB | 97.9 to 109.8 / 105 to 117 MB | 76.7 to 77.0 / 96.8 to 96.9 MB |

An update with its draw costs the original about 0.8 ms of processor time (0.74 to 1.48 ms; the third run was on a machine 49% busy), the
port 0.26 ms with Direct3D 11 (3 times less) and 0.047 ms when only every tenth update is drawn (17 times less): the port's `-fast` runs,
which are the test runs, are far faster than the original's. With SDL's software renderer the same draw costs 10 to 13 ms, 14 times the
original's, and holds 100 frames a second in real time only by using 88% of a core of this machine (below, "Shared costs").

**Loading** (the original: process creation to the loading-completed flag, six runs, the first three with the real-time pairs; the port:
creation to the first update, whose screenshot's file time marks it):

| | Original | Port | Port, software renderer |
| --- | --- | --- | --- |
| Launch to loaded | 0.76, 0.81, 0.87, 0.95, 1.00 and 1.28 s | 0.72, 0.82 and 0.78 s | 0.53, 0.58 and 0.57 s |
| Processor time to there | 0.75 to 0.95 s | 0.75 to 0.83 s | 0.58 to 0.62 s |
| Working set / private bytes there | 65 to 73 / 49.6 to 51.0 MB | 84 to 93 / 83.5 to 85.9 MB | 70 to 81 / 58 to 61 MB |

Loading takes the two about as long (the port's own log says "Resource Loading Time: 714 ms"); creating the Direct3D 11 device costs the
port about 0.2 s and 20 to 25 MB.

**Where the port's time goes.** `games/bejeweled/tools/profile/cpuprof.py` (cycles per thread, a flat profile by module and function)
on the whole adventure recording (215,005 updates in 13 s under `-fast`): ntdll 51%, the Nvidia Direct3D 11 driver 19%, SDL 5%, win32u 4%,
the game's exe 12%. Of the exe's 12%, a share is the load's decoders (below) and the biggest function of play is the framework's
`ImageFont::DrawStringEx` at 2.5% of all cycles; `Board::Update` is 0.4% and `Board::Draw` 0.4%. In real time (5,000 updates, 52 s sampled,
12.5 G cycles, 240 million a second) the main thread has 71% of the cycles, 83% of them in ntdll (system calls: the
presentation of the frame and the sleeps), the driver's own thread 17.5%, the mixer's thread 3.5% and the game's exe 5%. What the
player's machine spends is the display path, and the original's DirectDraw path spends about as much (1.5 s a stretch against the
port's median of 2.2 s). The load (1.1 s sampled, 1.49 G cycles) is 46% in the exe: `_stbi__out_gif_code` 13.3% and `Sexy::Quantize8Bit`
11.8% (the images are GIFs that are decoded to 32 bits and quantized back to 8), the audio and JPEG decoders about 10%, `imagehlp.dll`
7.6% (called from a place not looked for), ntdll 29% (file and memory system calls).

**Where the port's memory goes.** The framework's `memreport` command (`tools/profile/memmaps.py memreport`) at update 12,000 (in play):
the process holds 85.2 MB private (working set 93.3 MB), of which 32.8 MB are libraries and devices (`privateAtStart`, before a game file
is read), 20.6 MB the pictures' pixels (307 of them: 7.6 MB of 32-bit pixels, 12.1 MB of 8-bit indices and palettes, no second copies),
5.2 MB the samples of 40 sounds and 1.0 MB the two module files of the music (libopenmpt keeps its own decoded copy), and 25.6 MB the rest:
the game's own state, and the graphics driver (the same play under SDL's software renderer peaks at 64 to 68 MB private, about 20 MB less,
so the driver is most of it). The framework's heap holds 43.5 MB in 56,693 blocks (163 of 64 KB or more hold 30.1 MB), 40.2 MB at the title
(update 300) and 41.2 MB at update 1,600, so play adds about 3 MB (the pictures that get a texture, 13 at the title and 36 in play, and
small blocks). The graphics device holds about 9 MB of textures more, which no process can read. `tools/memmap.py` of the original in
play: 49.2 to 49.9 MB private bytes, 65 to 66 MB working set; its address space holds 57.1 MB of images (75 modules, shared with other
processes: windows.storage 7.2, shell32 6.5, ntdll 4.2 ...), 10.2 MB of mapped files, 0.8 MB of stacks and 45 to 46 MB of private memory.
What the original's private memory is made of (its pictures and sounds) was not split: it needs a hook in the original. The port holds
about 37 MB more private bytes than the original in play (86 to 89 against 49 to 51 MB; the web build's wasm heap is 47 MB): the Direct3D
11 driver is about 21 of them, and the rest is SDL, the framework's libraries and the pictures that it keeps after they are uploaded as
textures (shared, section "Shared costs").

**On the web.** Under Node (`perf.py web-minute`; the web build as the browser runs it, with no canvas, sound or GPU, SDL on its software
renderer, `-fast -drawevery`): 30,000 updates in 93 s (88 s of processor time, 3.1 ms an update), the wasm heap 47.2 MB at the title
(update 300), 68.0 MB at update 20,000 (the textures of the pictures are kept in the heap with no GPU). In the Browser pane (Chrome 156,
the page's own timers, `tools/profile/webperf.js`, one run): the first frame takes 1.33 s (the load) and then the menu costs next to nothing
(6 of its 319 frames draw anything); the map 0.81 ms a frame (3.7% of the thread); a level played by the port's bot, which types
the sharks' words as real key events, 1.10 ms a frame, 3.1 ms at most, 5.0% of the thread, 916 frames for 2,006 updates in 20 s; the wasm
heap 47.2 MB (37.5 MB in use) in every phase, 20.6 MB of it pictures and 5.2 MB sounds, the JavaScript heap 12 MB, no errors. The web build
runs Typer Shark at a twentieth of a thread.

**Shared costs** (sent to the coordinator; none of them is in this game's code):
1. *The load's decoders.* A quarter of the load's processor time is two functions of shared code: the GIF decoder's per-pixel output
   (`_stbi__out_gif_code`, 13%, stb) and `Sexy::Quantize8Bit` (12%, a binary search for every pixel; a cache of the last colour would answer
   most pixels with the same table), and the quantizing is needed only because the GIF was decoded to 32 bits first. About 0.15 to 0.2 s of
   a 0.75 s load, for every game that ships GIFs.
2. *The software renderer.* 10 to 13 ms of processor time a frame at 640 by 480 (88% of a core in real time) against 0.26 ms with
   Direct3D 11. A player whose machine has no usable GPU driver (a virtual machine, some remote sessions) would have the port fall back to
   it (`DDInterface::Init`) and a slower machine would not hold 100 frames a second. Where the time goes in it was not looked for.
3. *The Direct3D 11 driver* costs the process 20 to 25 MB and 0.2 s of start (see the loading table); a driver choice, no default changes
   without the user.
4. *The pixels kept after upload* (the framework's, known: here 20.6 MB of the 85).
5. *`imagehlp.dll`* takes 7.6% of the load's cycles; the call is not in the game's code that was looked at.

**Open.** (1) The original's private memory is not split into pictures, sounds and the rest. (2) The browser's frame time was measured once,
in a pane that may have been hidden (the page's timers; the frame hook counts the game's own milliseconds, so the figures hold). (3) The
one slow real-time run of the port (above).

**To repeat.** `bash games/typershark/tools/profile/pairs.sh <out dir> [pairs]` from the worktree root (about 11 minutes a pair; the
software-renderer run without waiting takes 5 minutes; `WORK_END` and `WORK_STRETCH` change the no-waiting scenario), then
`python -I games/typershark/tools/profile/summary.py <out dir>/runs.txt`; `memmaps.py memreport|port|orig <out dir>` for the memory,
`perf.py web-minute <out dir>` for Node and `webperf.js` in the Browser pane (`python tools/webtest/serve.py --port <p>`, then
`typershark.html?player=1&assets=1&silent=1&driver=games/typershark/tools/profile/webperf.js`). The original needs the copy under
`build/origrun/typershark/game`, origrun's `typershark.cfg` and `patches/typershark.txt` (section 8) and phase 2's `build/ts-p2/patches.txt`.
The raw samples were deleted once these figures were written.

## 12. Coverage check (2026-11-11)

The whole coverage check of the game (`docs/COVERAGE.md`): origrun's `--coverage` recorded which of the original's functions ran, in 23
runs on the host (`games/typershark/tools/coverage/run.sh`, `all.sh`; the hits in `build/coverage/typershark/hits/`), all on the virtual
clock with the scripted cursor, the registry map (`--reg-map popcap`) and phase 2's loading patch unless said:

- phase 2's tours replayed without their shots: M, S, U, H, T, the idle game, tutor lessons 17 and 18, the abyss, the explore, bonus and
  whole-adventure games (g_exp, g_bon1, g_rt2) and the restart pair (rs_a, rs_b);
- the sound comparison's run on the real clock with the silent recording stand-in (`snd`), so that the sound code runs;
- a run with no users folder at all (`nouser`), and a run with the loading patch off, which lets the title screen draw (`title`);
- five new tours, each also compared with the original in the sandbox by shotdiff (all equal, `IDENTICAL_PLAY.md`, rows P, T2, Q, D, I) and
  then run on the host: `tour_more.py` (P), `tour_stats.py st2 more` (T2), `tour_test.py` (Q), `missing.sh` (D: the run on the host is `mda`,
  then `users/user0.dat` deleted, then `mdb`) and `tour_idle2.py` (I).

Every run ended with the game exiting normally. The game's own code is the range 0x401000-0x462290 of the exe, 1095 functions; the rest is
the C runtime and the libraries the exe holds. The first report of the phase 2 runs gave 863 functions reached and the last gives 914.

**Reached in the original with no counterpart in the port: 308 at first, none at the end.**

- **Placed: 5.** Functions the port translates inside another function (the export cuts three constructors in two, at 0x40a413, 0x410c93 and
  0x40ea73; a second cut of one at 0x427967; and `CopyImage` before `Palletize` at 0x412870) got their address in the comment above the
  function that holds them. Comment edits only (Enemy.cpp, MechaTorpedo.cpp, WhitePiranha.cpp, UserProfileMgr.cpp, WinWSApp.cpp).
- **Left out on purpose: 306, as `none` lines in `coverage.txt` with their reasons.** The vtordisp thunks and base-class forwarders of
  virtual functions, the parts and deleting forms of the entity classes' destructors, standard library code for the game's types
  (strings, vectors, the maps of pictures, streams), small helpers the port writes in line (each named in the comment of the function that
  holds it), the registration, trial and update-check code (removed), and `WinMain`'s callee, which the platform layer's `main` replaces.
- **No missing game logic was found.** No function of the original that ran was missing from the port.

**Reached by neither the runs nor the port: 159, all `none` lines too** (the report lists them as the third kind): the compiler's exception
handler blocks (44), vtordisp thunks (24), destructors (7), standard library helpers (64), the registration, money and update dialogs and
what only they call (19: 0x414020, 0x41b030, 0x41b230, 0x41c1c0 and the `RegisterDialog` and `MoneyDialog` members), and the rect test that
the port's `Rect` has (0x402020). Nothing in this group is game logic the port needs.

**Defined in the port, never reached: 32 at first, 22 at the end.** The five new tours were made to reach part of this list:

- *Reached by the new runs (10).* The hint box's `KeyChar` (the space bar closes it; found by tour P, whose first version failed, since
  the port's scripted `key 32` sends no character and the typed space is `char SPACE`), the status bar's resting tooltip, the statistics
  page's Previous Lesson (0x44b070), the plain message dialog 0x41bd80 and `KeyDialog`'s `AddOkKey` and `RemoveCancelKey` (the typing
  test's "Test Completed!", the error of a missing data file), `UserProfileMgr::SaveUser` and `GetDefaultUserNumber` (the same error),
  `TitleScreen::Draw`, and `RestartBoard` (the game-over box's PLAY AGAIN in an adventure). All of them are equal to the original in the
  pictures. The runs also showed that tour U's delete dialog answers never ran in the sandbox (its clicks missed the OK button, so the
  question box was never opened); tour P runs the whole flow, equal in all its pairs.
- *Read again against the export, equal.* `CountKill` (0x4123a0, a boss kill of the abyss: no run killed one), `ResetScroll` (0x441030,
  reached by continuing a finished adventure, level 100 or later), `XWidget::MouseEnter` and `MouseLeave` (a chest of the map screen under
  the resting mouse: the runs click the chests, none rests on one). `ShowNoWindowedModeMessage` (0x41d030) shows the message dialog when the
  framework's `mForceFullscreen` is set, which the original's window code sets only when the desktop's colour depth cannot give a window;
  no machine of ours does. Its body (the pause, the counter, the dialog 0x30b, the fonts, the spacing and the insets) agrees with the export.
- *Compiler-made, kept.* The deleting destructors `ProjectileBoss` 0x4083c0, `Projectile` 0x4098e0 and `TabPanel` 0x444700: the objects
  they free are not freed through that slot in a run (the stats pages live until the exit).
- *Candidates for taking out code the port does not need (13), no play of the original reaches them and the port has no caller; taken out
  in the removal pass, section 13:* `GameObject::SetState` 0x4010e0 and `Projectile::HitsTarget` 0x4018c0 (bases that every class overrides),
  `BlackShark::GetWord2` 0x407030 and `RedShark::GetWord3` 0x407170 (the original reads the fields where the port calls the accessor: replace
  by the field), `ActivateTypingTutor` 0x412c90 and `FocusTypingTutor` 0x412d00 (their only caller was the register dialog), the
  `GetPreferredHeight` of `HintDialog` 0x438170 and `OptionsDialog` 0x438650 (no one asks them), `StatsScreen::ButtonPress` 0x441320
  (nothing registers the screen as a listener; the comment says so), `FadeButton::Resize(const Rect&)` 0x441c10, `TabPanel::GetChild`
  0x4446f0 and `AddChild` 0x4516a0, and `GameOverDialog::OrderInManagerChanged` 0x4380a0 (nothing reorders the dialog; the framework calls
  it only from `PutInfront` and its relatives).

**Was open, now closed (section 16).** `Board::BuildWaterImage` (0x440330, the sea as strips of triangles on a 3D accelerated screen) was
the one function neither reached nor read again: every run of the original in the sandbox had the Options dialog's 3D box off (`Is3D` 0),
because the sandbox's display never gave the original a Direct3D device. The original with 3D on ran later on the user's desktop (section 16):
the sea, the board, the dialogs and the menus were compared in 3D, and `BuildWaterImage` was read against the original's pictures.

**Cost.** The 23 runs took about 1 to 3 minutes of machine time each on the virtual clock. The five tours took the most work: their click
positions come from the port's own pictures, and the host takes no shots. Every change to the sources was a comment; `tools/check.sh
typershark` was run for them.

## 13. Removal of code the port does not need (2026-11-11)

The 13 functions that section 12 lists as candidates are out of the port: the original's own code for each of them ran in none of the
23 coverage runs, and nothing in the port called it. Each has a `none` line in `coverage.txt` with its reason, and the report now
lists 478 functions left out on purpose (306 and 159 as before, and these 13), none reached by the original without a counterpart,
and 9 defined in the port and never reached (22 before): the deleting destructors, `UserProfile::CountKill`, `ShowNoWindowedModeMessage`,
`Board::BuildWaterImage`, `ResetScroll` and `XWidget::MouseEnter` and `MouseLeave`, all of section 12's other lists.

| Original | What the port did | Now |
|---|---|---|
| `GameObject::SetState` 0x4010e0 (`mState = theState`) | the base's body | no `SetState` in `GameObject`; `Enemy` declares it pure (every creature class overrides it) and `Player` declares its own. Bubbles, background fish and bonus texts are never given a state |
| `Projectile::HitsTarget` 0x4018c0 | the base's box test | pure; `Torpedo` and the shots below it have their own |
| `BlackShark::GetWord2` 0x407030, `RedShark::GetWord3` 0x407170 | accessors that return a copy | the four callers (`BoardUpdate.cpp`, `BoardWaves.cpp`) read `mSecondWord` and `mThirdWord`, as the original does |
| `ActivateTypingTutor` 0x412c90, `FocusTypingTutor` 0x412d00 | `WinWSApp` members | gone (their only caller was the register dialog, removed in the registration phase) |
| `HintDialog` 0x438170 and `OptionsDialog` 0x438650 `GetPreferredHeight` | constants 400 and 438 | gone: `SexyAppBase::AddDialog` asks only a dialog whose width is 0, and both boxes are sized when they are made |
| `StatsScreen::ButtonPress` 0x441320 | the button-click sound | gone, and with it the class's `ButtonListener` base: nothing registers the screen as a listener, so the port's class is a `Widget` alone |
| `FadeButton::Resize(const Rect&)` 0x441c10 | an override that placed the tooltip | gone: `Widget::Resize(const Rect&)` forwards to the four-argument one, which does it; `PositionTooltip`, now with one caller, went back into it |
| `TabPanel::GetChild` 0x4446f0 and `AddChild` 0x4516a0 | two virtuals | gone (the panels fill `mChildren` themselves) |
| `GameOverDialog::OrderInManagerChanged` 0x4380a0 | put the two buttons in front | gone: `Dialog`'s does the same |

Every change keeps the draw order and every other behaviour: `tools/check.sh build typershark` passes (the whole native build, tools
included, and the 13 games' state-log hashes equal to `regress_expected.txt` and to the frozen baseline's), and the web build compiles. The
other games' builds are unaffected (nothing outside `games/typershark/` changed). A removed function that a later mod needs comes back from
section 12's addresses.

## 14. Readability pass (2026-11-11)

The port's names now come from the original or from what the code does with them (`docs/CODE_STANDARDS.md`); nothing the port does
changed. `tools/namecheck.py games/typershark` found 1,362 faults before the pass (1,265 stand-in names such as `mField1c0`, `mFlag13c`,
`mVector1b4` and `theField1b0`; 47 names ending in an offset; 20 functions with no address or `port:` marker; 14 one-letter names
kept over more than 10 lines; 10 comments that name a development stage; 6 that say "unknown") and finds none now; the three
lines it still allows are the names `PHASE1.md` and `tools/phase1` (`namecheck.txt`).

What changed, by kind:

| Kind | What |
|---|---|
| Members | 177 renamed (declarations and about 1,300 uses), among them every stand-in and offset-named member of `Board`, `Player`, `Enemy` and its subclasses, `LessonManager`, `MapScreen`, `HudBar`, `OptionsDialog`, `HintDialog`, `TypingTutor`, `TutorialPanel`, `UserProfile` and `WinWSApp`. A name says what the member holds (`mLevelTitleTimer`, `mSonarTopRightImage`, `mRouteDots`, `mSavedScoreBackup`); the offset stays in the comment on its line, where every field already carried it. |
| Functions | `UserProfile`'s `SetField1e4`, `ClearFields1cc`, `SetGroupField8` and the other six that were named for an offset are now `SetAbyssBossIndex`, `ClearSavedGameTotals`, `RecordGroupLevel`, `SetEndlessLevelsPlayed`, `SetGameSharksKilled`, `SetGamePiranhasKilled`, `GetAbyssBossIndex`, and two getters that return `mLessonOutcome[2]` and `mSecretsFound[2]` (below). |
| Parameters and locals | the `UserProfile` parameters named for fields (`theField1b0`, `theDouble1b8`, ...) are the field they store; `aUnusedWord`, `aField24` and `aWord2` in `BoardUpdate.cpp` and `UserProfileMgr.cpp`; the one-letter names in `testhooks/*` and `GameObject.cpp` that ran over 10 lines. |
| Addresses | every function now names its address or carries a `port:` marker; `GameObject::Draw`'s `// 0x461270` comment is on `RecordHistory`, where the export puts that body, and `Draw` says so. |
| Comments | the stage numbers ("phase 1", "milestone 2") and the word "unknown" are out of the comments; the old names in comments and in `docs/*.md` (over 200 places) follow the renames, except in `ASSUMED.md` lines 15 and 28, which say which export name each role replaced. The headers of `WinWSAppButtons.cpp` and `WinWSAppGame.cpp` no longer say the other classes' fields are named for their offsets. |

The renames were made by the compiler, not by search and replace: `tools/readability/rename.py` renames a member at its declaration, compiles
all 90 units with `-fsyntax-only` and replaces the old name exactly where the compiler reports it missing, with the class taken from the
error (two classes that share a member name never mix); `scoped.py` renames a local or a parameter inside its scope and skips comments and
strings; `comments.py` gives the new names to the comments and docs that still carry the old ones and lists the names two classes
declared differently, which were edited by hand (`mField1c0`, `mFlag13c`, `mField124`, `mField1f8`). Names came from subagents (one at a time, reading only) that
worked from each field's uses, its comment and the screen text it feeds; their maps were reviewed before they were applied.

**Proof.** `python tools/objsnap.py cmp typershark` against a snapshot taken before the pass: 90 objects, 1,689 functions, 0 objects
differ (code, `.rdata` and `.data` are identical byte for byte). `tools/check.sh build typershark`: the native build and the 13 games'
state-log hashes match `regress_expected.txt` and the frozen baseline. `build-web:typershark` builds. No string literal of the sources
changed (the changed lines' literals were compared one against the other: the differences are quotes inside comments).

**Names that are guesses**, each marked in its comment: `mAlwaysTrueA`, `mAlwaysTrueB`, `mAlwaysTrueC`, `mAlwaysFalseA`, `mAlwaysFalseB`
and `mAlwaysZero` (members that the original sets once and never reads; named for the value the constructor gives them), `mAlpha` in `BonusCreature`,
`mSpareButton`, `mSpareDialogFlag` (`TypingTutor`'s `+0x260`, see below), `mLevelNumber` and `mEnemyType` (the spawn's kind, also the boss kind in the
abyss), `mAlignmentGap`, `mCloseCallSeen`, `mUnreadTimer`, `mTreasureTimer` and `mBonusWord` (the air-tank round's word, named from the
neighbours `mBonusWordPos` and `mBonusWordCount` and from the screen text "Bonus words"), `mNodesById`, `mRouteDots` and `mRouteNodes`
(the code calls a map site a node and a site alike). `UserProfile::GetLessonTwoOutcome` and `GetNecklacesFound` stand where the port once had
`GetField9c` and `GetField8c` at 0x412340 and 0x412350: the original's functions at those addresses are `LessonManager`'s `GetCurrentWaveType` and `GetLevel`
(the port has them there already), and nothing calls the `UserProfile` copies, which read two unrelated fields. They are kept, named for what they return,
until a removal pass takes them out.

**`tools/splitfields.py`** (the check for two members of a class at one offset, never written and written without being read) reports
three members as written in the export and none as a fault: `Board`'s `mUnreadTimer` +0x58 (the function at 0x45c140 writes `param_1 + 0x58`
on an `int*`, which is byte 0x160, the wave strength `mWaveAmp`, not the board's timer), `TutorialPanel`'s `mAxisMax` +0xa8 (written in
`ButtonDepress`, which the export places on the listener sub-object at +0x140, so its `this + 0xa8` is +0x1e8) and `TypingTutor`'s
`mSpareDialogFlag` (+0x260: `KeyChar` writes `this[0x98]` on a one-byte `TypingTutor` type, the byte at +0x98, a different field). The 11 it lists
as read only are fields that the export reaches by another path (a pointer the port keeps in a member, a sub-object's `this`); none is
a field that the original writes and the port never sets, which is what the check is for.

## 15. The PopCap Game Collection version (2026-11-11)

The port was made from the Steam copy (WinTS.exe, the launcher with the game appended), so it is checked against the game's version on
the PopCap Game Collection disc (2009; `docs/COORDINATION.md`): the two versions compared, and the port run on the Collection's files. The
procedure is Bejeweled's (its GAME_NOTES section 23); `tools/collection/` holds the tools.

**Getting the files.** `unshield` (built for Bejeweled's check, `E:\retools\unshield-build`; the user allowed it on 2026-11-10) read the
disc image's cabinet, which was mounted read-only and dismounted afterwards. The group `TyperSharkDeluxeComponent` came out: 617 files, 6.7 MB,
in the main checkout's `re/_installers/TyperSharkDeluxeCollection` (git-ignored). The copy has no launcher: `TyperShark.exe` is the
game itself, beside `bass.dll`, `license.txt`, `readme.html`, `register.tpl` and the folders `data`, `images`, `music`, `properties`, `sounds`
and `users`.

**The exe is the same build.** Set beside the game carved from Steam's WinTS.exe (`re/typershark/bin/TyperSharkGame.exe`;
`tools/collection/cmpexe.py`), the Collection's `TyperShark.exe` has the same link time (20 April 2004, 20:00:28) and the same
sections. `.text` and `.data` are byte for byte the same, `.rdata` differs in 8 bytes (an eight-digit stamp at 0x535f68), and the resources differ: the
Collection's exe carries more icon sizes (`.rsrc` is 159,744 bytes against 28,672). The header's checksum and a few other header bytes differ, and a signature of
5,432 bytes follows the last section. So everything the decompilation, the identical-play pass and the coverage check found holds for the
Collection's game too, and no code was compared further.

**The files** (`tools/collection/cmpfiles.py`). Of the Steam copy's 619 files and the Collection's 617, 606 are the same byte for byte:
every picture, sound, song and data file but the ones below. The differences:

- `images/titlescreen.jpg`, `barandlogo.gif` and `_barandlogo.gif`, which `TitleScreen::Draw` draws (`tools/collection/cmpimages.py`).
  The Steam copy's title picture has no logo and its bar picture carries the flat "PopCap Games" logo at the bottom right; the Collection's
  title picture is another encoding with PopCap's round logo and a TM mark (at the top right and the bottom right), and its bar picture has no logo. The
  port draws the files it is given.
- `images/_statusbarfont.png`: the same 6,110 pixels, stored as RGBA where Steam's is a palette picture.
- `properties/partner.xml`: the registry key is the same (`PopCap\TyperShark`), as are `NoReg`, the product name and `DefaultWindowed`, the only
  settings the port reads (`SexyApp::InitPropertiesHook`). The Collection's `DontUpdate` is false and it has an `Offering` number; Steam's has a
  Steam id, a buy link and `NumOptionsLinks`. The update check, the links and the buy link are not in the port.
- The licence and readme, `register.tpl` and the launcher's own files (`WinTS.exe`, `Flash.ocx`, `drm.*`) differ or exist on one side only.
- `users/user.idx`, which only the Collection ships: the default score table. It is the file the port writes at its first run in everything but
  the random number after the version (the port's run: 101; the Collection's: 128; both are in the range 0x60 to 0x84 that
  `SaveHighScores` draws from), so the tables the port makes (`LoadDefaultHighScores`) are the Collection's, name for name and score for score.
  The staging tool does not copy `users`: the port keeps the players' data in its own folder.

**The port on the Collection's files** (`tools/collection/run.sh`). `stage_typershark_assets.py` stages a Collection folder as it does a
Steam one (597 files, no `wints.exe`: the texts come from `typershark.exe`, which `GameText.cpp` and `main.cpp` now accept beside `WinTS.exe`).
- `tools/regress.py` with the Collection's staging: the 13 games' state-log hashes are `regress_expected.txt`'s, as on the Steam files.
- The tours M (98 screenshots: the first start, the menu and its hovers, the dialogs, Options, the credits, the statistics, the Hall of Fame), H (24: an abyss game to its game over, the Hall of
  Fame) and I (8: the idle adventure game to its game over, and the second game), each run once on each set of files from a fresh profile folder, compared by shotdiff:
  all 130 pairs are equal, pixel for pixel. A `-fast` run's loading ends before its first update, so the title screen is never drawn in it; the
  test command `goto title` shows it, and its three shots (`tools/collection/tour_title.py`) differ in the two places where the pictures differ: the
  logo at the bottom right (a flat "PopCap Games" against the round logo) and the TM mark at the top right. Both are the files'.
- The web build under Node on the Collection's files (the page's code, no canvas): the title and menu tours, 101 pictures, equal to the native
  port's on the same files to within 3 levels of 255, the web build's usual difference (section 6). The page's importer
  (`web/page.js`) takes the Collection's folder: `TyperShark.exe` or `WinTS.exe` at the top is copied, and neither is a marker any more, since a
  marker cannot name two files; a folder with neither would import and show no texts (logged), where it used to be refused. A shared marker
  with alternatives (`WinTS.exe|TyperShark.exe`) would restore the check; it is waiting for the coordinator.

**Not done, and one thing to know.** The Collection's own exe was not run: its code is the Steam game's, and a run could only show the
title pictures again. The port's executable is also called `typershark.exe`, the name of the Collection's game: the installer that puts a
native port beside the original (`docs/STEAM_LAUNCH.md`) must not write it into a Collection folder under that name.

## 16. The 3D drawing (2026-11-11)

The user allowed two routes for the original with 3D on ("Yes, both routes", 2026-11-11): Windows Sandbox with the host's GPU first, the
visible desktop if the original still fell back to 2D there. `docs/IDENTICAL_PLAY.md` ("The 3D drawing") has the results; this section has the
facts a later session needs.

**The sandbox did not give the original 3D.** Four variants of origrun's sandbox run (`--sandbox-vgpu`; with `--dgvoodoo`; with
`--sandbox-connect`; both) all ran the original in 2D with `--3d on` asked: the byte `[[app+0x334]+0x10]` was 0 at update 5, and origrun
ended with its exit code 3 ("the other mode", `docs/ORIGRUN.md`). Nothing more was tried there.

**The desktop route** (`tools/phase2/visrun3d.py`, a run in the manner of BookWorm Adventures' `visrun.py`). A copy of the carved, patched game
(`build/origrun/typershark-p2/game`, loading on the main thread, seed 1) runs in `build/ts-3d` with the stand-in's own variables and no host:
`ORIGRUN_SOUND=stub` (silent), `ORIGRUN_REGMAP=popcap` (what the game writes under `HKLM\SOFTWARE\PopCap` lands under
`HKCU\Software\PopCap\TyperShark`) and `ORIGRUN_CURSOR=scripted` (the script keeps the shared block `Local\origrun-cursor-<id>` that
`tools/origrun/cursor.h` describes, so `GetCursorPos` returns the scripted position and the real cursor never moves). The runner stages the
registry (`Is3D` 1, `ScreenMode` 0, the key exported first and put back afterwards, or deleted if it was not there), starts the game's one
window without activating it, keeps it topmost, and for each step of a tour: waits for the update counter (`APP` 0x565380, `+0x4b8`) to reach
the step's update, suspends the process (`NtSuspendProcess`), posts the input to the game's window (`PostMessage`: moves, clicks, keys,
text, and the focus messages), lets it run, and for a shot waits until the draw counter shows the frame drawn (`+0x4a0` is 0), suspends
again and reads the client with `PrintWindow` (`PW_RENDERFULLCONTENT`) of that window alone. It refuses a shot if another window overlaps the game's.
The 3D flag was read at update 5 and at the end of every run: 1 each time.
The original runs on the real clock, 100 updates a second (300 updates took 3.01 s in 3D and 3.05 s in 2D), so no step lands on its exact
update: the runner logs the update each step landed at (`vis3d.steps.tsv`), and `mkport3d.py` makes the port's script from the *landed*
updates (plus the port's one-update shift), so both get each input and each shot at the same update. A shot after an input waits one
update, because a real-clock redraw at the input's own update would show the result of the input. `vis3d.sh <tour>` runs the whole chain
(original on the desktop, the port's script, the port with `-fast -drawevery` and the fixture `port-3d`, whose registry.ini sets `Is3D` 1,
and shotdiff); `vis3d.sh <tour> 2d` does the same with the original in 2D, for the differences that are the real clock's and not the
3D drawing's. `tour_water.py` (a board with the sea shot every 60 updates) and `tour_pause.py` (one pause at an update, shots 4, 30 and
80 updates after it) were written for it.

**What the port does in 3D.** The port's 3D mode is the SDL accelerated renderer (`DDInterface::mIs3D`), chosen by `Is3D` in
`registry.ini`. The sea's strips of triangles (`BuildWaterImage`, translated as read) draw as in the original, and the whole of tours M (menus,
dialogs, options, hall, statistics), U, Q and T and the paused boards equal the original's.

**One change, and its cause.** The original's Direct3D line drawing leaves out a line's last pixel (Direct3D's line rule: the end point is
not drawn), where a 2D line draws it. The game draws lines in two places: the tab strip's divider (`TabbedWidget::Draw`) and the typing tutor's
lines. With 3D on, a horizontal or vertical line was one pixel longer in the port. `TyperSharkDisplay` (in `WinWSApp.cpp`, made by
`WinWSApp::CreateDDInterface`) overrides `DDInterface::DrawLine` when `mIs3D`: a horizontal line of more than one pixel ends one short on X, a
vertical one on Y; a line of one pixel and a slanted line are left as the framework draws them (none of the game's lines is slanted).
Without 3D nothing changes: `tools/check.sh typershark` is unchanged.

**Two things found with the medal screens (2026-11-12), and what they did to the list below.** The first 3D tours ran the port at the render
scale its window gave it, 2 (`DisplayGeometry::UpdateDisplaySize` renders at the next whole multiple of the 640 by 480 screen, and `ReadScreen`
box-downsamples a shot back), where a player's windowed 640 by 480 game renders at 1. A 3D picture drawn at 2 and averaged down is not the 3D
picture at 1: it is a little blurred, and the sea's leftmost column differed by up to 55 levels. Every 3D run of the port now passes
`-renderscale=1` (`vis3d.sh` does). The second is the blend. The original's Direct3D 9 draws with straight alpha: it filters a texture's
red, green, blue and alpha apart, so a texel under zero alpha still colours the edge of a stretched picture. The framework's accelerated
display uploads premultiplied textures whenever the renderer allows a custom blend mode (`DDInterface::SetupBlendModes`), so the same edge
came out different: up to 44 levels on the medal and the ten pictures of the end-of-adventure dialog. Measured: the original's 3D capture of
the medal equals a straight bilinear sample of its picture and alpha file (`medal_silver.gif` and `_medal_silver.gif`, 97 by 133, into the
66 by 90 rectangle, sampled at the pixel centres) to 0.16 levels on average; the premultiplied sample is 0.9 off. `TyperSharkDisplay`
(`WinWSApp.cpp`) now sets the framework's own fallback modes (straight `SDL_BLENDMODE_BLEND` and `_ADD`, textures uploaded as they are) at
the start of each frame while 3D is on (`mIs3D`), and puts the premultiplied modes back, with the textures released and uploaded again, while it
is off: the 2D mode's pictures equal the original's with the premultiplied modes, and with the straight ones 5810 pixels of the 2D dialog tour
moved by a level. The check of that: the 2D dialog tour (48 shots) with the exe that has the straight modes in 3D only against the exe
without the change: 0 shots differ.

**Open, and listed as oddities in `IDENTICAL_PLAY.md`.** (1) The sea's leftmost pixel column was the render scale's, and is gone: at scale 1
the paused board's column 0 differs by at most 3 levels (it was up to 55). (2) Flat areas of the sea still differ in 3D (the original's 3D
picture is a little brighter than its own 2D picture; the cause is not proven, 16-bit textures are the candidate): at scale 1 by at most 3
levels, 1 on average, in the paused board's sea, and up to 7 in the map screen's sea; under shotdiff's tolerance of 8. (3) The original's
texture pieces, below. (4) The real clock's differences, below.

**The original's texture pieces.** The original's 3D stretch (`DDImage::StretchBlt` 0x4a2fb0 hands it to `FUN_0049f480`, a scaled copy through
`FUN_00498ce0`) draws one quad for each piece of the picture's texture (`FUN_004947f0` takes the piece of a pixel from a uniform grid, each
piece with its own texture and so its own clamped addressing), and the pieces of a tall picture are cut at a power of two. The one row of the
medal (97 by 133) that differs is where source row 128 lands: 150 + 128 * 90 / 133 = 236.6 in the dialog (row 236), and the same row of
the statistics screen's copy (row 402). The original does not blend across that seam, and the port, with one texture, does. It is 10
pixels of the dialog's medal (two runs of five) and 12 of the statistics screen's, up to 53 levels, and nothing else of the two screens
differs. The piece size itself (128 rows and the 5 left) is read from that position, not from the export. To make it equal the port would cut each stretch at the
pieces' edges and draw each piece from a copy of its own: code for about ten pixels, so it is an oddity (`IDENTICAL_PLAY.md`).

**What the real clock does.** The runs show differences that the same tours in 2D on the desktop show too, so they are not the 3D
drawing's: the hover fade (a shot 3 updates after a move catches it half-faded), the dialog-open animation, the sharks', diver's and
bubbles' sub-update phase on a running board, the caret's blink, the player list's scroll offset after ten players, the tutor's words per minute
and the stats bars' growth (they read `mSecondTimer->Elapsed()`, milliseconds), and a board that has run on after a pause and a resume.
A virtual-clock origrun with a window of its own would make all of them equal; none is a rendering difference.

**Not compared in 3D:** the abyss and bonus levels, an idle game, the boards after a pause and a resume (the real clock again), and the
original's single-precision rounding of x87 code in 3D (Twist found it in its own comparison; nothing here depends on it).

## 17. Widgets drawn without the fast stretch (2026-11-11)

AstroPop found that the 2004 `WidgetManager` never sets the fast stretch on a widget's Graphics, where the shared 1.3 one sets it on every
widget (three places in `framework/WidgetManager.cpp`), so a stretch drawn by a widget came out nearest-neighbour. Typer Shark is on the same
2004 framework, and its export shows the same: `WidgetManager::DrawScreen` (0x47d790) copies the screen's Graphics (`Graphics_copy` 0x48ff00),
clips and translates it and calls the widget's DrawAll, and no function of the export sets the 2004 Graphics' fast-stretch byte (+0x49: the constructor
0x48ffb0 clears it; `DrawImage_destsrc` 0x490570 hands it to the image's StretchBlt as the last argument). The only two writes of a byte at
+0x49 in the export are in `SexyAppBase`'s constructor and a `std::map` head node. So `WinWSApp::Init` sets
`gWidgetsDrawWithoutFastStretch` (framework/WidgetManager.h, off by default; AstroPop's) before `SexyApp::Init`.

*Result of the 2D tours.* Run with the exe before and the exe after (`tools/phase2/stretchcheck.sh`, pixel for pixel by `pngdiff.py`; the
frozen exe with the flag off against the one with it on), 17 tours, 2779 shots: M 98, S 33, U 70, P 32, T 17, T2 23, Q 6, I 8, idle 78,
H 24, the abyss 81, tutor lessons 17 and 18 (120 and 163), Expert 210, the bonus levels 364 and the two whole-adventure runs (680 and 772); the two-run tours R and D were not run again (their screens are tour M's).
**No shot changed in any picture.** Five shots differ in a column or two at the window's right edge (639 or 638: the abyss' `012348_hint_25` and
`016800_grid`, one Expert shot, one bonus and one adventure shot); the same shots differ between two runs of the same exe (the oddity "the
window's last column" in `IDENTICAL_PLAY.md`: two runs of the old exe against each other gave 4 of those 5, and not the same ones in
each run), so they are that, not the stretch. Nothing matches the original better or worse.

*Why nothing changed, and what the tours do not show.* The game stretches a picture in two places only (a picture drawn into a rectangle of
another size): the medal (97 by 133, drawn 66 by 90) on the statistics screen's adventure panel (`AdventurePanel::Draw`, a player who has
finished an adventure) and in the end-of-adventure dialog (`WinGameDialog::Draw` 0x439390, the same medal and the ten gem and treasure pictures
squeezed into 30 by 30 squares). No tour shows either: the statistics tours play a new player with no finished adventure, and the whole-adventure
tours' shots (a grid every 1000 updates, and at the named events) caught no end-of-adventure dialog. The flag changes only these two draws and
no picture of any tour, so the 2D tours cannot tell whether the original's averaged stretch is the port's. That check was made with profiles that
have finished an adventure, and it found that the port's stretch was not the original's (a soft edge on the medal); section 18 has the cause, the
fix and the results. The flag stays on: it is what the original does.

## 18. The medal screens: the original's software stretch (2026-11-11)

Section 17 found that the game stretches a picture in two places only, and that no tour showed either. Both are drawn with a profile that has
finished an adventure: the statistics screen's adventure panel (`AdventurePanel::Draw`, the medal) and the end-of-adventure dialog
(`WinGameDialog::Draw` 0x439390: the medal and the ten gem and treasure pictures, each squeezed into 30 by 30). The medal is bronze for 1 or 2 finished
adventures (`mGamesCompleted`, registry `NGC`), silver for 3 or 4, gold from 5.

**Cause of the difference that was found.** In 2D the original's screen stretch (`DDImage::StretchBlt` 0x4a2fb0, the path taken when the fast stretch is
off, as for every widget) does not stretch on the surface: it averages the source into a transparent temporary `MemoryImage` with
`MemoryImage::SlowStretchBlt` (0x48d9b0: each destination pixel is the average of the source pixels it covers, and in the last loop a source pixel's
weight is `255 * a / (destAlpha + a)`, so the alpha is renormalised as pixels are added) and blits that. The port drew the stretch with the framework's
texture stretch (a linear filter), which put a soft edge on the medal: the first comparison of the statistics screen found 620 pixels that differed from the original by up to 24
levels, and the dialog's medal and pictures differed by 8801 pixels and up to 39. In 3D the original draws textured quads, so its 3D picture needs the GPU's filter.
So `TyperSharkDisplay` (the `DDInterface` subclass in `WinWSApp.cpp`) sets `mSoftwareStretch` for every stretch when 3D is off (the framework's
`GetStretchCopy`/`StretchCopy`, 0x433ca0, the same averaging), as AstroPop's does, and `WinWSApp::Init` sets `gSlowStretchRenormalizesAlpha`
(`framework/MemoryImage.h`, AstroPop's), which is the weight above. The flag alone changed no pixel; `mSoftwareStretch` is the fix, and the flag
stays because it is what the original's code does. With 3D on nothing changes. `tools/check.sh typershark` (the 13 games' state logs against the
frozen baseline) is unchanged.

**The profiles.** A new player cannot show the medal, and playing a whole adventure to the dialog takes 154,000 updates in the port and 25 minutes
of play in the original, which the sandbox runs at about a tenth of real time (an earlier try ran 21,884 updates in 40 minutes). So no adventure is
played. `tools/phase2/mkprofile.py <ngc>` writes the values the game itself writes for a player (the registry preset the original imports and the
port's `registry.ini`, with `NGC` set and every counter of the statistics screen a distinct number) and the two files of a users folder.
`mkprofile.py <ngc> saved` adds a saved adventure at the last site, made by the port (the menu's Adventure, the difficulty, `level 34`, `win`: the
port saves the game at site 35) with its totals set to distinct numbers (score 47250, typing time 305 s, 37 wrong and 412 right letters; the
file is a list of numbers, each a two-byte length and its text, `UserProfile::WriteDataToDisk`) and `RNGC` (registry, "this game's finish is not
counted yet") on, as a game in progress has it. The original reads the same two files and the same registry values.

**The statistics screen** (`tour_medal.py stats`): the menu, the Statistics link, the adventure panel, the abyss and tutor views, the adventure
view again and Close, 14 shots, for four players: `NGC` 1 (bronze), 3 (silver), 5 and 12 (gold). All four equal the original's pixel for pixel.

**The dialog** (`tour_medal.py dialog`): the menu's Adventure offers the saved game, YES, and the board of site 35 comes. Winning that level needs it
to be played, so the board is left the way a won level is left: the original's copy has one patched byte (`Board::KeyChar` 0x454e9b, `cmp eax, 7`
to `cmp eax, 1`: the space bar's step back to the map, `ReturnToMap` 0x429450, which a won level (the diver's state 7) takes, is taken in the
diver's first state, 1) and the script presses the space bar; the port has its test command `goto map`, which calls the same `ReturnToMap`. The map
screen then runs its final scroll (the saved game of site 35 switched it on, `SetFinalLevel`), counts the game finished (`NGC` + 1, so a player with
`NGC` 0, 2 or 4 gets the bronze, silver or gold medal) and opens the dialog at update 868. The tour shoots the board, the map every few updates
through the scroll, the dialog at every update from 864 to 876 and four times later, the hover over RETURN HOME and KEEP GOING, then RETURN HOME (the
menu, and the statistics screen of the player, whose medal is the new one): 49 shots. With `dialogkeep` it presses KEEP GOING instead (the endless
levels: the board of an endless level, its sea and the diver's dive for 600 updates): 40 shots. Run for `NGC` 0, 2 and 4 on both programs (the
original in the sandbox, `chain.sh` with `PATCHES=` the list plus the patch): the 49 shots of each equal the original's pixel for pixel, and so
do the 40 shots of KEEP GOING (silver). The text of the dialog (the score, the adjusted speed, the accuracy), the ten pictures with their counts
and the medal are therefore the original's.

**The control.** The same dialog tour with the exe built without the two changes against the exe with them: 14 of the 49 shots differ (8801
pixels, up to 39 levels, in the box x 130 to 479, y 156 to 334: the medal and the ten pictures), so the tour sees the stretch (the statistics screen of that silver player is the same with and without them).

**The web build.** The same tours under Node (`tools/phase2/webshots.sh <tour>`, with `USERS=<fixture>`: the wasm of `build/web/typershark.js` run by
`wasmrun.js game` from the same script, its shots against the native port's with no tolerance): the dialog for bronze, silver and gold (49 shots
each), KEEP GOING (40) and the statistics screens of the players with `NGC` 1 and 12 (14 each). Every pair differs by at most 3 levels of 255 and
by nothing stronger, over the whole picture: the two builds' picture decoding, as in every web tour (`IDENTICAL_PLAY.md`, "The web build"). The
medal's edge, which the missing software stretch had moved by up to 39 levels, is the native port's in the web build too. Not run in a browser.

**In 3D** (2026-11-12, the desktop route of section 16 in the overnight window; `tools/phase2/medal3d.sh <dialog|dialogkeep|stats> <N>` does a whole
tour, and refuses between 08:00 and 22:00). The original runs through `visrun3d.py` with three options made for this: `--users <dir>` puts the
profile's two files over the clean users folder, `--reg <file.reg>` imports the profile's registry preset (the `HKLM` values of `mkprofile.py`)
as `HKCU\Software\PopCap` after `Is3D` and `ScreenMode` are set, `--patch "va:0x454e9b 83f807 83f801"` patches the copy's exe (the byte of the
dialog route above); the port runs with the `-3d` fixtures (`mkprofile.py <ngc> [saved] 3d`: `medal-sv<N>-3d`, `medal-ngc<N>-3d`) at `-renderscale=1`.
Seven tours, each about two minutes of real time: the statistics screen for `NGC` 1, 5 and 12 (14 shots each), the dialog for bronze, silver and
gold (47, 48, 48), KEEP GOING (40). Result (section 16 has the two changes it led to, the straight blend and the render scale): the dialog box
(130 to 479, 156 to 334) of every shot from the one where the dialog is open on equals the original's except 10 or 11 pixels of one row
of the medal's bottom (the original's texture pieces, section 16); the statistics screens have no visible box and 12 pixels of that row; the
other boxes are the real clock's (the map's final scroll, a shot before the dialog opens; KEEP GOING's board at updates 300 and 600; the menu's
hover fade 4 updates after the statistics screen closes), the first two with the same boxes in the tour run with the original in 2D on the desktop
(`vis3d.sh m3d_dialog_2 2d`, `m3d_dialogkeep_2 2d`). How the model was found: the medal's source picture and its alpha file (`medal_silver.gif`,
`_medal_silver.gif`) bilinear-sampled at the destination's pixel centres, over the dialog's grey, is the original's capture to 0.16 levels on average
(`premultiplied`: 0.9; the port before the two changes: 2.2). The port's own 2D dialog tour is unchanged by the straight blend: 48 shots, 0 differ
between the exe with it and the exe without it, so the 2D result above stands.

**Not compared.** The board of site 35 itself (its boss and treasure are not played in the original: a saved game starts it and the space bar leaves it).

Tools (`tools/phase2`): `mksaved.sh` makes the saved adventure, `mkprofile.py` the profiles, `tour_medal.py` the tours (`stats`, `dialog`, `dialogkeep`,
and `win`, the first try: the whole-adventure route up to the dialog, which the original could not finish in the sandbox), `medal.sh <ngc: 0, 2 or 4>
[dialog|dialogkeep]` runs a dialog tour on both programs with the patched byte (`runorig.sh` takes `PATCHES=<file under build/ts-p2>`: the list of
`tools/origrun/patches/typershark.txt` plus the line `test TyperSharkGame.exe va:0x454e9b 83f807 83f801 <reason>`) and `shotdiff`.
