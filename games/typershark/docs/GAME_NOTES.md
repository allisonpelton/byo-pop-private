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
| BlackShark | 0x52979c | 0x410f50 | sizeof 0x240; a second word (`mWord2`); dtor 0x409d70 |
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
the boss by its type. Black and red sharks get further words from the lists (`mWord2`, `mWord3`), a white piranha one more
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

**Scoring.** A finished word scores the creature's points (`Enemy::mField54`), times `max(2, score / 600)` in an abyss game,
and the Board adds them to the diver, the rating and a floating text; the EMP charge grows by 0.01. The diver gets a life
each time the score reaches the next threshold (the first is 450000, 525000, 650000, 700000 or 900000 by difficulty, then
larger steps; `Player::AddScore`). A group of words ends when every creature of the lists has finished dying; the Board shows
a rating popup, counts a ramp (`mField0f38`) up to 240, and the diver enters the streak game: long words (`GetLongWord`,
lengths from the level's `MinEndDiff` to `MaxEndDiff`) come one after another, each finished word bringing the next, against
a countdown (`mField0f38` again, 0.1 an update, 0.12 in a treasure level, shortened by typing) and earn a streak bonus; then
come the level bonus, the treasure picture and the space bar. Typing speed and accuracy go into the profile's records at the end (`SaveProgress` 0x440580).

**Bonus (treasure) level.** The floor holds 22 clams in four rows, each with a letter and a kind (0 empty, 1 and 2 pearls
that pair up). The diver is in state 5: a letter opens the clam it is written on; two clams of the same pearl kind make a pair
(100000 points for the first pair, 200000 for the next ones), an empty clam closes at once with the wrong sound and takes the
open clam with it; after two pairs the next pearl that opens shows the treasure (`ShowTreasure` 0x442760). `KeyChar` does the
pairing itself; the clam handler's event 3 only repeats it and never fires on its own (section 7, to check).

**The abyss.** No map: the dive goes down in feet (`mScore` of the Board is the depth), the level changes every 200 feet
(`SetDepth` 0x452cf0, `Update` 0x45c140), `SetDepth` makes the depth ruler's labels, a boss comes by `mField0d2c % 4`, the
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
`mAlpha` unset (0x461320; the one place where play could differ), the rating popup's `mBonus` and `mField34`, `Projectile`'s
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
`mWord2 = ""` and `mWord3 = ""`. `Board::Update` (0x45c140): the abyss goes to the next level every 200 feet, the port did it
every 100; fixed (`/ 200` in the two tests).

**Quirks of the original that the port keeps** (address, what it does, how to see it):

- Lives read -1 after the last life (`PlayerEaten` 0x44fe30): it subtracts one and a value below 0 ends the game. An idle game
  reads 2, 1, 0, -1.
- The abyss starts with 0 lives (`SetLessonManager` 0x451cb0). Play an abyss game and read `lives`.
- The game timer keeps counting while a hint dialog pauses play. The first hint opens at board update 139 and stays about 22
  updates; read `time` in the state.
- `BackgroundFish`'s copy constructor (0x401df0) computes the follower's x as `-(rand % width + width)` in unsigned arithmetic
  and stores it as a double (about 4.29e9), so `Board::Update` deletes it at its first tick: only the leader of a school ever
  shows (section 10, F-1).
- `Player::Update` (0x461740) state 1 has a branch for `y < 235` that the preceding test makes unreachable; the BlackShark
  state-5 branch in `SetState` is unreachable the same way.
- `UserProfile::LoadDataFromDisk` (0x422630) appends the records it reads after the 35 the constructor made, so a loaded
  profile holds 70 per group and `WriteDataToDisk` (0x4172f0) writes only the first 35; `WriteDataToDisk` stores `mField1ec`
  where the loader reads `mField1b4`; `RegisterDefaultUserName` (0x419da0) always returns false. See a profile reloaded
  and saved again (section 10, F-7).
- Map (`MapScreen.cpp`): `ResetScroll` 0x441030 sets both scroll targets from the width difference where the final panel's
  draw test (0x447cd3) compares with the height difference; site 35 has the link -1 and is kept away from `Update` 0x45af60 and
  `SetVisible` 0x450ec0 only by the final-level flag; `OnLevelChosen` 0x4538e0 reads the list at size - 2; a dead end moves
  the site before it behind it (F-6).
- Typing tutor: `KeyChar` 0x460680 takes a Return, newline or space at the first or last character of a line as neither right
  nor wrong; `Update` 0x4601a0 has a creature branch above y = 160 in state 1 that the loop never takes; the seconds timer
  restarts at each second so an overshoot is lost; a lesson that ends within its first second divides by zero in `Finish`
  and both speeds read 0. Phase 1 played lessons 13, 15 and 17 (lines that start or end with a space) and each ended when the key
  shown was typed (`PHASE1.md`, H1 item 7); what a player sees when a different key is typed there was not tried.
- Tutor pictures: `SetupHighlight` 0x45e810 lights the 9 key's picture for the keys 8 and `*`, leaves `{ } _ |` without the shift
  key, gives `\ _ |` no finger, and the draw 0x44b400 places the lines before and after the typed line by their index in the
  task, so from the fifth line of a task on (when `mFirstLine` is above 0) they sit too low (F-4, F-5).
- Lesson and config readers: the character read last is handled twice; a tutor text file's line count for the random pick
  includes the failed read, so the line picked can be the one past the end; a `<HEADING>` number is not checked; the level
  config is read a character at a time and a line reaches the XML parser only once it ends, so the file's last line counts only
  if it ends; `LevelNum` is parsed and not kept.
- `LessonManager`: `NextWave` divides by a count that is 0 for an empty list; `LastWaveIsBoss` reads the element before the
  first of a level without waves; `SetLevel(36 or more)` starts `NextLevel` at index -1, so the level reached is the argument
  minus 36 (the map never passes a larger one).
- `OptionsDialog`: the button text colour (0x438660) goes to the credits button as index 3 of its own colours, not its label
  colour; its `ButtonPress` (0x438a80) plays the click and does not call the base (F-8).
- `DrawWavyImage` 0x441ac0 draws (height - y) rows when the picture ends below the screen; `_ftol` (0x509890) keeps the low 32
  bits of the truncated 64-bit value and gives 0 for a NaN or out-of-range value (the port's `Ftol` helper does the same).
- `HighScoreScreen::Draw`: a mode other than 0 to 4 repeats the label of the line before. The abyss lines have no label.
- `DeleteUserDialog::DeleteSelectedUser` (0x43df60) reads the first line of the list unchecked after the deletion (section 7).
- `AvoidWordOverlap` 0x446110 counts its loop by the size of the bonus-text list (+0xdb8) and indexes the bonus-creature list
  (+0xdc8) with it; the port stops at the end of the list it indexes (section 7).
- Events 9, 0xb, 0xd and 0x14 have no handler in the Board; `HandleClamEvents` event 3 never scores on its own.

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

**Phase 2 (looks identical).** Not done yet: the screens by shotdiff, sound, focus, the web build, and real mouse and key play
through the page and the window are all still to do.

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
  and not leaked (section 5, F-7).
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
`docs/ORIGRUN.md` has no status row for Typer Shark yet (to add).

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
| `set score\|lives\|timer\|depth\|emp <n>` | the diver's score, lives (`mField110`), the game timer (milliseconds), the depth, the EMP charge (0 to 1); `set hints 0\|1` and `set tutorial 0\|1` need a player. `enemy` is read-only |
| `win` | the level is won now: the diver goes to the level-won state (0xc) as `Update` does at the bottom, with the level bonus; ERROR in the abyss, in a won or lost game |
| `lose` | the last life goes now: `mField110` is set to 0 and `PlayerEaten` runs, which subtracts one and opens the game-over dialog |

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

## 10. Fix candidates (2026-11-09)

The rule: a bug of the original that does not break play, or a wait that only costs the player time, is kept in the port and
fixed in the fixes mod; a bug that breaks play is fixed in the port itself (the two that phase 1 found and the picker hang are
in section 5, and are not candidates). These come from a reading of the port's source and the working records, not from a line
by line audit of the export or a play-through; a later find is added here. "Draws" says whether the fix changes the game
generator's sequence.

- **F-1 Schools of background fish.** `BackgroundFish` copy constructor 0x401df0 puts a follower at x = `-(rand % width +
  width)` computed unsigned, about 4.29e9 as a double; `Board::Update` (0x45c140) deletes anything with x above the width at its
  first tick, so only the leader of a school (0 to 4 fish every 400 updates) ever shows. Fix: compute the x signed, so that the
  followers swim in behind the leader. Draws: yes, indirectly. The constructor's own draws stay the same, but the living
  followers raise the fish count that the 400-update test (`size < 8`) reads, so some spawn ticks skip their `% 5` draw and the
  generator's sequence shifts for the rest of the game.
- **F-2 The start of a level.** `Update` counts the black fade-in `mFadeAlpha` down from 255 by 2 every other update (about
  2.5 s; the level intro box opens at 0x75), and the diver climbs down in state 1 (`Player::Update` 0x461740: 2 pixels an
  update from y = 0 to 235, 1 pixel below the top picture's height; about 1.2 to 2.4 s) while `KeyChar` ignores every key in that
  state. Fix: shorten or skip both. Draws: yes, indirectly (the background bubbles and fish draw by update count, and the waves
  start at other updates). To check whether the first wave already comes during the fade.
- **F-3 The end of a group.** After the last creature of a wave has died and the rating popup has faded, `Update` adds 0.96 an
  update to `mField0f38` until 240 (250 updates, 2.5 s) before the diver enters the streak game or the treasure. Fix: skip the
  ramp. Draws: yes, indirectly, as F-2.
- **F-4 The tutor's lines.** The draw 0x44b400 places the lines before and after the line being typed by their index in the
  task, not their place among the four shown, so from the fifth line of a task on, when `mFirstLine` is above 0, they are drawn
  that many lines too low. Fix: use the place among the lines shown. Draws: no. Not seen in a run; to check with a long task.
- **F-5 The tutor's keyboard picture** (0x45e810). The keys 8 and `*` light the 9 key's picture at the 8 key's place; `{ } _ |` are
  typed with shift but do not light the shift key; `\ _ |` get no finger or ring. Fix: use the right pictures and shift test.
  Draws: no.
- **F-6 The map's final panel** (`ResetScroll` 0x441030 against the draw test 0x447cd3). Both scroll targets are set from the
  width difference where the draw test compares with the height difference, so the ship of the final panel shows only if the two
  differences are equal. Fix: set the vertical target from the height difference, or compare like for like. Draws: no. Whether
  the ship ever shows in the original is for the phase 2 screen comparison to say.
- **F-7 The profile's records** (`LoadDataFromDisk` 0x422630, `WriteDataToDisk` 0x4172f0). A loaded profile appends its 35
  records per statistic group after the constructor's 35 zero ones and the writer writes only the first 35, so the per-level
  speed, accuracy and streak records of earlier sessions are not kept: each session writes only its own. Nothing in the port
  reads `mRecords` except the writer and the loader (a search of `src`; to check in the export), so nothing on screen changes.
  Fix: load into the first 35. Draws: no. It changes the bytes of `user<N>.dat`, so a comparison of saved files after a reload
  would differ from the original's.
- **F-8 The options dialog's credits button** (0x438660 `SetColor`, 0x438a80 `ButtonPress`). The text colour reaches the button
  as index 3 of its own colours, not as its label colour, and the click handler does not call the base. Fix: pass the label
  colour; call the base. Draws: no. A small visible difference in the button's text colour, to check by shotdiff.

Checked and not candidates: the piranha event 5 (no dangling target, section 7); lives reading -1, abyss lives 0 and the hint
pause not stopping the timer (the same in the original, and they do not hold the player); the streak game and the treasure
level's own timers (the game's rules, not waits); the registration code (gone). Nothing found in the reading done for the
sharks, piranhas, clams, bonus creatures and bosses (equal to the original in phase 1's runs, which does not exclude a wait)
and the high score tables; the dialogs and screens other than the options dialog were not read for waits or faults.

## 11. Performance

To be measured by the coordinator (2026-11-09).
