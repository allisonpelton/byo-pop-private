# BookWorm Deluxe 1.03: game-code behaviour spec

The behaviour of BookWorm Deluxe's game code, precise enough to translate it function by function onto the
port's framework without going back to the binary. It was written from the carved game PE (linked
2004-04-15 with VS.NET 2002 / MSVC 7.0) by reading Ghidra's decompilation and the disassembly; nothing was
run. Addresses are virtual addresses in that PE (`re/bookworm/bin/BookwormGame.exe`, image base 0x400000);
every game function is listed in [INVENTORY.md](INVENTORY.md) and named in Ghidra from
[symbols/](symbols/). The framework the game was built against is an internal SexyAppFramework from early
2004, mapped onto the port in [docs/FRAMEWORK_2004.md](../../../docs/FRAMEWORK_2004.md).

Contents:
- [Overview](#overview): structure, the game in brief, random numbers, files, what is not ported
- [Part 1. The application](#part-1-the-application-bookwormapp-math-user-profiles) (BookWormApp, Math,
  user profiles, resources, music, screen flow, dialogs, registry, high scores, saved-game header)
- [Part 2. Board logic](#part-2-board-logic-boardcpp-with-piece-statkeeper-wordscore) (Board, Piece,
  StatKeeper, WordScore: rules, letters, scoring, special tiles, burning tiles, levels, saves)
- [Part 3. Board drawing and effects](#part-3-board-drawing-and-effects-board-drawing-effectcpp-pointscpp)
  (Board::Draw, Effect, Sparkle, FlyingPieceList, EffectMgr, EmberListMgr, Points)
- [Part 4. Lex and the balloons](#part-4-lex-and-the-balloons-lex-biglex-wordballoon-definitionator)
  (Lex, BigLex, WordBalloon, Definitionator)
- [Part 5. Hall of Fame and level-up](#part-5-hall-of-fame-and-level-up-famewidget-levelupwidget-labelwidget)
  (FameWidget, LevelUpWidget, LabelWidget)
- [Part 6. Screens](#part-6-screens-and-small-ui-classes) (TitleScreen, UserManager, BWGameSelector,
  HelpWidget, CreditsDialog, OverlayWidget, ColorizedButton, ImagelessButton, ShadowFont, WaitDialog)
- [Part 7. Dialogs](#part-7-dialogs) (MoneyDialog, OptionsDialog, NewUserDialog, SelectUserDialog,
  DeleteUserDialog, GetStringDialog, HintDialog; RegisterDialog for reference)
- [Part 8. Cross-module names](#part-8-cross-module-names-and-decisions) (the names every part uses for what
  crosses module boundaries, and the decisions taken where the parts disagreed)

Each part keeps its own section numbering; "section N" inside a part refers to that part.

## Overview

### Structure

The game code is 0x401000-0x437800: 852 functions in source files linked in alphabetical order
(INVENTORY.md lists the modules and their address ranges). The largest are `Board.cpp` (53 KB),
`BookWormApp.cpp` (45 KB), `FameWidget.cpp` (30 KB, about half of it iostream templates) and
`TitleScreen.cpp` (11 KB, with the user-profile manager linked after it).

| Class | sizeof | Base | Created by | Part |
|---|---|---|---|---|
| BookWormApp | 0x1348 | SexyApp, ButtonListener | WinMain (a static object in the port) | 1 |
| Math | 0xb48 | none | embedded in the app | 1 |
| UserManager / User | 0x2c / 0x38 | none | the app (registry profiles) | 1, 6 |
| Board | 0x3f0 | Widget, ButtonListener | StartGame | 2 |
| Piece | 0xf0 | none | Board | 2 |
| StatKeeper, WordScore | 0x78, 0x48 | none | Board | 2 |
| Effect, Sparkle, FlyingPieceList, EffectMgr, EmberListMgr (Ember) | 0x10, 0x48, 0x20, 0x1c, 0x2c (0x60) | | Board | 3 |
| Points | 0x84 | Widget | Board | 3 |
| Lex, BigLex, WordBalloon | 0xd0, 0x70, 0x120 | Widget | Board, BWGameSelector | 4 |
| Definitionator | 0x14 | none | the app | 4 |
| FameWidget, LevelUpWidget, LabelWidget | 0x2a0, 0xe0, 0x94 | Widget | the app, Board | 5 |
| TitleScreen, BWGameSelector, HelpWidget, OverlayWidget | 0xc8, 0xb0, 0xa0, 0x90 | Widget | the app, Board | 6 |
| CreditsDialog | 0x138 | Dialog | the app | 6 |
| ColorizedButton, ImagelessButton | 0xf0, 0xe8 | ButtonWidget | LevelUpWidget, FameWidget | 6 |
| ShadowFont | 0x60 | ImageFont | TitleScreen | 6 |
| MoneyDialog and its five subclasses, HintDialog, GetStringDialog | 0x120-0x188 | Dialog | the app | 7 |

### The game in brief

- **Start-up.** The app loads 17 images, 5 fonts and all 47 sounds, shows the title screen with its loading
  bar, and loads the rest on the loading thread: the MO3 module as five songs, 132 more images, 14 fonts,
  nine pre-drawn tile sets, then the dictionary (`wordlist.txt`, prefix-compressed, 151,381 words of 3 to
  12 letters kept), the bonus-word list (`bonus_word_dictionary.txt`, 1,785 words) and the definitions
  (`popdefs.txt`, 2,249 entries). Clicking the title continues to the player check (a new player is asked
  for a name) and the game selector.
- **The game selector** offers Classic, Action, the Hall of Fame and Quit, with Lex in a speech bubble.
- **The board** is a hexagonal grid of 7 columns (odd columns 8 tiles, even columns 7: 49 tiles of 57 px).
  The player selects a chain of adjacent letter tiles (click, drag, double-click to submit, right-click to
  clear) and submits it; valid words (3+ letters, in the dictionary) score
  `(length + 2*green + 4*gold + 7*sapphire + 10*diamond [+ n-1 if every tile is fire]) * (letter values +
  level + 1) * 10`, the tiles are eaten by Lex and replaced from above. Long or high-scoring words award
  special tiles; red **fire tiles** appear (by chance and from a "danger" meter) and burn down a column a
  step at a time; one reaching the bottom ends the game. A **bonus word** gives extra points. Clicking Lex
  scrambles the board at a cost (fire tiles appear). Levels advance with a progress meter; the level-up
  screen shows the rank and best words.
- **Classic vs Action.** In Classic, fire tiles come with refills (a per-level chance plus a "fire meter"
  that short words drain and long words fill), every submitted word or scramble burns them a step down, and a
  fire tile on the bottom row ends the game at the next step. In Action, a timer turns a top tile to fire
  every 1,000 updates (10 s) and burns all fire tiles a step; a fire tile on the bottom starts a 10-second
  countdown. Each mode has its own music and Lex greeting (Part 2 section 4.9).
- **Game over** shows the final score, adds it to the Hall of Fame (top 10 per mode, plus the three best and
  three longest words) and returns to the selector.

### Random numbers

| Generator | Seeded by | Used for |
|---|---|---|
| Framework `Sexy::Rand()` (SAF MT19937) | `SRand(GetTickCount())` in SexyApp::Init | only seeds for the generators below |
| The game's global `MTRand*` (0x5347d0) | WinMain: `new MTRand(Sexy::Rand())` | letters, special tiles, fire tiles, scramble, bonus words (Part 2) |
| Board's own `MTRand` (Board+0x6c) | Board ctor: `Sexy::Rand()` | tile drop heights, Lex chatter choices, every particle the Board spawns (Parts 2, 3) |
| EffectMgr / EmberListMgr `MTRand`s | their ctors: `Sexy::Rand()` | the particles inside those managers (Part 3) |
| CRT `rand()` | `srand(GetTickCount())` in SexyApp::Init | Lex's idle animation choices only (Part 4) |

Every MTRand is seeded the SAF 1.2 way (`mt[i] = mt[i-1] * 69069`), which the port reproduces with
`SeedMTRand2004`; the CRT generator is MSVC's LCG (`CrtRand`). With the same seed, the port draws the same
numbers in the same order as long as every draw listed in Parts 2-4 happens in the same update.

### Files, registry and properties

| What | Where (original) | Port |
|---|---|---|
| Player profiles | registry `HKLM\SOFTWARE\PopCap\BookWorm\Users\<name>`: Alias, ShowHelp, ShowHints, ShowDefs, TimesPlayed; `Users\User` = last player (Part 1 section 8, Part 6 UserManager) | the emulated registry (`registry.ini` in the user dir), same key and value names |
| Saved game in progress | `players\<name>\classic.dat` / `action.dat` next to the exe, version 5 (2-4 still load); single-use (deleted when continued or declined) (Part 1, Part 2 section 6) | the user dir, byte-identical format |
| High scores | registry key `Scores`, REG_MULTI_SZ values Classic, Action, Best, Longest (Part 1 section 9, Part 5) | the emulated registry, same layout |
| Options | registry (music and sound volume, ...) and `properties/partner.xml` (NoReg, DontUpdate, NoInternetScores, LoadingBarY, OptionsDialogHeight, ...) | the same |
| Game data | `wordlist.txt`, `bonus_word_dictionary.txt`, `popdefs.txt`, `images/`, `fonts/`, `sounds/`, `music/Bookworm1.mo3` | staged into `assets/bookworm/` (never committed) |

### What is not ported

The retail 1.03 build is registered and offline (`properties/partner.xml`: NoReg, DontUpdate,
NoInternetScores = True), so these paths are never taken and are not translated (each part names them in
one line): registration and its dialogs (RegisterDialog, BigInt codes, `popcinfo.dat`, the 100-play trial
rule), the update check (UpdateCheckDialog, InternetManager), internet high scores (FameWidget's server
code, GetStringDialog for the alias, WaitDialog, ImagelessButton), ads, BetaSupport, crash reporting, and the
framework's demo record/playback. The CD-protect launcher is not part of the game image.

### Registration taken out (2026-11-08)

The user moved the removal of all registration code into the current phase (2026-11-07), with what a registered
copy shows, such as a "thanks for registering" line (2026-11-08), and every button that leads out of the game
(CLAUDE.md, "Order of the work"). Every branch on registration was folded into the registered path, which is how
the retail copy plays, and the trial, nag, register and buy paths went with the texts and images only they used.
What came out:

- **Title screen.** The register link (id 0, its green pulse and "one moment" label), the trial's nag timer and
  shown play count, the trial expiry (the quit instead of play, dialog 4), the registered and trial stamps (behind
  the property ShowTrialStatus, False in the retail `partner.xml`), and the "thanks for registering" status text
  (shown only when registered but not unlocked, never in the retail build). The status line is now the welcome, or
  on a first run while loading the unpacking of the sounds, at the registered build's place.
- **Board and bots.** Three tests of `mIsRegistered` (the held letter key lighting its tiles, the streak of
  rewarded words, and in Submit the words of 9 letters or more, which the trial sent to a dictionary upsell, dialog
  17) now always take the registered branch, in `Board.cpp` and `GreedyBots.cpp`.
- **The trial's play counting.** The properties MaxExecutions, MaxPlays and MaxTime, the levels-reached count
  (`mLevelReached`, `mPlaysToCount`) and the Board's per-play count (`mPlayCounted`, which counted only with MaxTime
  and MaxPlays both 0). Shutdown keeps the retail path, MaxTime's default of 60: the minutes played go to
  TimesPlayed, which the title reads for its first-run text.
- **Options dialog.** The register button (id 34, created but never shown in the registered build) and its
  property RegisterLabel; the help and credits row keeps the registered build's two-button layout.
- **Dialogs and texts.** The handlers of dialogs 2 (RegisterDialog), 4 (trial expired), 9 (invalid code) and 17
  (upsells), `ShowRegisterDialog`, and the texts GT_TITLE_REGISTER_NOW, GT_TITLE_ONE_MOMENT,
  GT_TITLE_THANKS_FOR_REGISTERING, GT_OPTIONS_REGISTER and the unused GT_OPTIONS_MORE_GAMES.
- **Images.** backdrop_register, title_stamp_trial, title_stamp_registered, button_reg_register, button_reg_cancel
  and button_reg_regcode are no longer loaded: their slots stay empty, so every image id keeps its number (Lex's
  animation table and the effects in a saved game hold image ids as numbers).

Deviations: none that shows in the retail build, since every path taken out was one the registered copy never
takes. A `partner.xml` that set MaxTime to 0, MaxPlays, ShowTrialStatus or RegisterLabel would change the original
and no longer changes the port. The options dialog's "more games" partner link was already left out (the user,
2026-10-24) and its row closed up; the original's dialog is one row taller for it.

### The standard test commands

The port answers the standard test commands (`docs/TEST_COMMANDS.md`) in `BookWormApp::HandleScriptCommand`
(`src/testhooks/Commands.cpp`), reachable only from a test: `-script` lines and the live channel (`-commands`, the
web's `?driver=move`). A run that sends none plays as before (the suites and the frozen baseline are unchanged). Each
answers with one line: `STATE {...}`, `MOVE {...}`, `OK <command>` or `ERROR <command>: <reason>`.

| Command | What it does |
|---|---|
| `state` | `STATE {json}`: `screen` (title, menu, help, highscores, game, none), `dialog` (options, continue, quit, hint, credits, message, users, deleteuser, confirmdelete, newuser, leave, other; null without one), `mode` (classic, action), `player`, and in a game `level` (from 1), `score`, `timer` (Action's fire timer), `fire` (the fire meter), `bonus` (the bonus word), `board` (`state`, `busy`, `letters` and `types` as eight strings of seven, `.` where a column is short, and the `selection`), then `over` (the game-over table is up) and `busy`. |
| `goto <screen>` | `menu`, `options`, `help`, `highscores`, `credits`, `classic` or `action` through the game's own functions. `title` is an error: the title screen does not come back. |
| `level <n>` | Level n (1 to 42) of the game in play: the level before it with its score threshold reached, then the game's own `NextLevel`. |
| `unlock all` | `ERROR`: the game has nothing locked. |
| `set <name> <value>` | `score`, `timer`, `fire`, or `mode classic`/`action` (outside a game: the mode the next game starts in). |
| `win` | The level's threshold reached and the level completed as a word would complete it (the summary follows). |
| `lose` | The game lost as it loses one: the first unselected tile of the bottom row turns to fire (in Action the fire countdown also runs out), then the game's own fire scan (`BurnTiles(false)`) starts the shake, the burn-down, the jingle, the high scores and the game-over summary. `lose <n>` instead arms it: the next `move` after the driver has played n words answers `lose` and starts it, so a driver plays a whole game that ends. It is the one case where `move` changes anything, and only after a test armed it. |
| `seed <n>` | The C library generator, the framework's reseed and the game's generator (`gGameRand`) from it, as a start seeds them. |
| `move` | `MOVE {json}`: the next input of the port's word solver (`Solver::FirstHint`, the first hint the hint button would give), or `MOVE null` (loading, the board busy, an animation running). Its `what`: `play` (the title's link), `name` (the new-player dialog: "Driver" and Enter), `ok` (a hint or message dialog; the continue question is answered no), `help`, `newgame` (the selector's mode button), `word` (with `word`, `score` and `kind`: the path's tiles clicked, then Submit, or a second click on the last tile when a definition balloon covers Submit), `scramble` (Lex clicked when the board spells nothing), `summary`/`gameover` (the level summary's button), `fame` (the Hall of Fame's continue). Every click goes to a point of its widget that the widget manager reaches. |


### The fixes mod

`mods/fixes/mod.json` (`docs/MOD_ROADMAP.md`, "The fixes mod"): one toggle per bug of the original that the port
keeps, each on by default, read through `FixEnabled` (`ModFeatures.h`). It changes no play: with it on, a game plays
as it does without it, update for update.

| Toggle | The original's bug | With the toggle on |
|---|---|---|
| `scoreGlow` | `Board::DrawScore` 0x405580 truncates the glow (0 to 0.5) to an int before multiplying it by 255, so the heated score (`FONT_GOLDSCOREHEATED`, a font loaded for nothing else) is drawn with alpha 0 and never shows. | The glow is multiplied first: the score turns pale gold while points are added and fades back. |

Fix candidates looked at and left out: the high-score tie that sets `mNewHighScore` for a pruned entry (nothing the
player sees reads the flag); the music FadeIn skipped while the song fades out (a player cannot reach it, Part 1
section 4); the endless loop on an odd count of Best-word strings in the registry (Part 1 section 9.2) and
`Ember::Load`'s fwrite, which can read a saved game out of step (Part 3's load bug): a hang and a broken save, so the
port itself mends them, as CLAUDE.md asks of bugs that break play.
---

## Part 1. The application (BookWormApp, Math, user profiles)

Scope (brief `re/bookworm/analysis/BRIEF.md`, group **app**): `BookWormApp.cpp` (0x4100f0-0x41b3e0, class
`Sexy::BookWormApp : SexyApp`, 157 functions) and `Math.cpp` (0x42e330-0x42e3d0). Three neighbours that
belong to the app are covered too, because nobody else owns them and the app depends on them:

- **0x40fe80-0x4100f0** (the tail of INVENTORY's Board.cpp range): `WinMain` 0x40fe80, the shutdown helper
  `BookWormApp::ShutdownWidgets` 0x40ffa0 and the free helper `DrawShadowedString` 0x410090. They are app code
  (probably the head of BookWormApp.cpp or a `BookWorm.cpp` that sorts between Board.cpp and BookWormApp.cpp).
- **0x434800-0x435e20** (the tail of INVENTORY's TitleScreen.cpp range) and the inline getter 0x402090:
  the player-profile manager (`UserManager`, 0x2c bytes, no vtable, owned by the app at +0x1178). It is
  probably its own source file (`UserManager.cpp` or similar sorts between TitleScreen.cpp and
  WaitDialog.cpp).
- The saved-game files (`players\<user>\classic.dat` / `action.dat`, written by Board 0x406c40, read by
  0x408ab0, deleted by 0x4078b0): the header and path rules are specified here; the Board fields in the
  body belong to the board group.

Sources: the Ghidra export (`re/bookworm/gh/`), the disassembly where Ghidra lost arguments, the 2004
framework map (`docs/FRAMEWORK_2004.md`, `widgets.md`, `graphics.md`), the shipped data in
`assets/bookworm/`. Nothing was run. Offsets are absolute from the object start. `slot N` = 2004 vtable slot
(docs/FRAMEWORK_2004.md 1.1 for SexyApp, docs/FRAMEWORK_2004.md for Widget/Dialog). Proposed C++: `app.h`; symbols:
`app_symbols.jsonl`.

### 0. Summary

- **sizeof(BookWormApp) = 0x1348** (`operator new(0x1348)` in WinMain), vtable 0x4fbda4 with 68 slots
  (SexyApp's 67 + its own slot 67 `TitleScreenDone`). Data from +0x618: an embedded `Math` (0xb48 bytes, sine
  tables nobody reads), widget pointers, four high-score multisets, the image table (`Image** ` of 149), 19
  fonts inline, derived images, the dictionary `set<string>`, bonus words, per-user options, trial counters.
- **Resources are three hard-coded name tables** (images 0x52e410 x149, fonts 0x52e3c0 x19, sounds
  0x52e668 x47). Init loads images 0-16, fonts 0-4 and all 47 sounds; the loading thread loads the 5 songs,
  fonts 5-18, images 17-148, builds 9 pre-drawn tile-set images, two checkbox halves and three copies, then
  `wordlist.txt` (prefix-compressed), `bonus_word_dictionary.txt` and `popdefs.txt`. Any missing image or
  font: `Popup("Failed to load <path>")` and shutdown; a missing sound: silent shutdown.
- **Music**: one MO3 loaded as songs 0-4; title/menu = song 0 at order 27, classic = song 1 order 0, action =
  song 2 order 37, game over = song 3 order 61/59 (no loop), hall of fame = song 3 order 54, danger = song 4
  order 51/65. `UpdateFrames` runs the music fades twice per tick.
- **Player profiles live in the registry** (`SOFTWARE\PopCap\BookWorm\Users\<name>`: Alias, ShowHelp,
  ShowHints, ShowDefs, TimesPlayed; `Users\User` = last user). The `players\` folder only holds saved games
  (`players\<name>\classic.dat` / `action.dat`, version 5, single-use: deleted when continued or declined).
- **High scores** are 4 tables in the registry key `Scores` as REG_MULTI_SZ: Classic and Action (top 10,
  name + score), Best (top 3, name + word + points), Longest (top 3, name + word; the length is the score
  after a reload). Defaults are filled in when a table is short.
- **Dialogs**: the app builds every dialog (`NewDialog`, `DoDialog` override sets fonts/colours). Ids 1-28
  plus 10008/20008; `ButtonDepress` decodes `id % 10000` in 2000..2999 as YES of dialog `id-2000`, 3000..3999
  as NO of `id-3000`. Table in section 7.
- **RNG**: the module makes exactly one draw: WinMain seeds the global game generator with
  `new MTRand(Sexy::Rand())` (framework MT, after Init's `SRand(GetTickCount())`). No CRT `rand()` here.
- **Not ported**: registration (dialogs 2, 4, 9, 17, "THANK YOU!"), internet high scores (6, 23-27),
  update check (7, 8, 10008, 20008), crash info, trial counters' effect. The registered retail build
  (partner.xml NoReg/DontUpdate/NoInternetScores = True) never reaches them.

### 1. Class layouts

#### 1.1 BookWormApp (+0x618 .. +0x1348)

SexyApp fields the app writes in its ctor: mProdName (+0x8) = "BookWorm"; mTitle (+0x24) = "BookWorm Deluxe "
+ mProductVersion (+0x3b8); mRegKey (+0x40) = "PopCap\\BookWorm"; mMusicVolume (+0x70) = 0.5 (double
0x3fe0000000000000); mMusicInterface (+0x33c) = NULL; mCustomCursorsEnabled (+0x52b) = false;
mAutoEnable3D (+0x5c5) = true. WinMain sets mWidth (+0x60) = 640 and mHeight (+0x64) = 480 before Init.

| Off | Type | Name | Meaning | Ctor | Readers / writers |
|---|---|---|---|---|---|
| 0x618 | Math (0xb48) | mMath | sine/-cosine tables (1.2) | Math() | nobody reads the tables |
| 0x1160 | Board* | mBoard | current game widget | 0 | StartGame creates; RemoveBoard; Board, Fame, LevelUp, Lex, Effect, Help read it (list in 10) |
| 0x1164 | TitleScreen* | mTitleScreen | title/loading screen | **not initialised** (Q4) | Init creates; TitleScreenDone, ShutdownWidgets remove |
| 0x1168 | BWGameSelector* | mGameSelector | main menu | 0 | ShowGameSelector; selector, Fame, SelectUser/NewUser dialogs |
| 0x116c | HelpWidget* | mHelpWidget | help screen | 0 | ShowHelp/HideHelp; Board 0x403ae0/0x404b40/0x406c40 read |
| 0x1170 | FameWidget* | mFameWidget | hall of fame | 0 | ShowHallOfFame; ButtonDepress; Fame |
| 0x1174 | Definitionator* | mDefinitionator | popdefs.txt definitions | 0 | loading thread; Board 0x40f5b0 |
| 0x1178 | UserManager* | mUserManager | player profiles | **not initialised** (Q4) | ReadFromRegistry creates; 25 readers |
| 0x117c | multiset<HighScoreEntry,ByScore> | mClassicScores | top 10 classic | empty | 1.4 |
| 0x1188 | multiset<..,ByScore> | mActionScores | top 10 action | empty | 1.4 |
| 0x1194 | multiset<..,ByScore> | mBestWords | top 3 best-scoring words | empty | 1.4 |
| 0x11a0 | multiset<..,ByWordLength> | mLongestWords | top 3 longest words | empty | 1.4 |
| 0x11ac | Image** | mImages | `new Image*[149]`, zeroed, filled by LoadImage | 0 (Init allocates) | 60+ readers |
| 0x11b0 | ImageFont*[19] | mFonts | font table (section 2.2) | 0 | everyone |
| 0x11fc | Image* | mCheckboxOnImage | right half of `checkboxes` (DDImage, CopyImage) | 0 | OptionsDialog 0x42fe90 |
| 0x1200 | Image* | mCheckboxOffImage | left half of `checkboxes` | 0 | OptionsDialog |
| 0x1204 | MemoryImage*[9] | mTileSetImages | pre-drawn tiles A-Z per tile kind (2.4) | 0 | Board 0x4044f0, Effect 0x41fba0 |
| 0x1228 | Image*[4] | (unused) | zeroed and deleted only | 0 | - |
| 0x1238 | list<MemoryImage*> | mCopiedImages | owns the three copies below (head +0x123c, size +0x1240) | empty | loading thread, dtor |
| 0x1244 | MemoryImage* | mTileSapphireImage | copy of `tile_sapphire` | 0 | Effect 0x41fba0 |
| 0x1248 | MemoryImage* | mTileDiamondImage | copy of `tile_diamond` | 0 | Effect 0x41fba0 |
| 0x124c | MemoryImage* | mRankStripImage | copy of `rankstrip` (image 54 is deleted) | 0 | LevelUpWidget 0x42c210 |
| 0x1250 | bool | mInitDone | Init finished (title may draw) | false | TitleScreen::Update 0x433e80 |
| 0x1254 | int | mGameMode | 0 classic, 1 action | 0 | selector vf47 writes; LoadGame writes; Board (12 functions), LevelUp |
| 0x1258 | int | mSelectedGameMode | mode clicked in the selector | -1 | SelectGameMode; ButtonDepress 2005 |
| 0x125c | int | mScoringMethod | word-score formula selector | 1 | Board 0x406a10 (0 -> 0x4040e0, 1 -> 0x404f60) |
| 0x1260 | set<string> | mDictionary | wordlist.txt | empty | Board 0x409c60/0x40b540/0x40ba80/0x40bd40/0x40ed70 |
| 0x126c | vector<string> | mBonusWords | bonus_word_dictionary.txt in file order | empty | Board 0x408030 |
| 0x127c | set<string> | mUsedBonusWords | bonus words already completed | empty | Board 0x40abd0 inserts |
| 0x1288 | string | (unused) | | "" | dtor only |
| 0x12a4 | string | (unused) | | assigned "" | dtor only |
| 0x12c0 | string | mBonusWord | current bonus word | "" | Board 0x408030 writes/reads, 0x40bd40 |
| 0x12dc | int | mGameOption | round-trips through BWGameSelector +0x80, otherwise unused | 1 | selector 0x401460/0x401600 |
| 0x12e0 | int | mUserTimesPlayed | per-user "TimesPlayed" | 0 | Board 0x402a50/0x40ed70/0x40f5b0, UserManager |
| 0x12e4 | bool | (hint flag) | set by Board 0x40ed70 | false | Board |
| 0x12e5 | bool | mWelcomePlayed | selector said lex_welcome once | false | selector 0x402190 |
| 0x12e6 | bool | (unused) | | true | - |
| 0x12e7 | bool | mShowHelp | option "show help at game start" | true | per-user registry; Board, Help, Options |
| 0x12e8 | bool | mShowHints | option "hints" | true | per-user registry; saved in games; HintDialog id 26 clears |
| 0x12e9 | bool | mShowDefs | option "definitions" | true | per-user registry; Board, Options |
| 0x12ea | bool | mInternetOK | "Connect" pressed once (not ported) | false | 0x415a20, ButtonDepress 2006 |
| 0x12eb | bool | (unused) | | false | - |
| 0x12ec | bool[16] | (one-shot hint flags) | +0x12ec Board 0x40bd40, +0x12ed/+0x12ee Board 0x40b540, +0x12f0 Board 0x40ed70; others unused | false | Board |
| 0x12fc | vector<int> | mPausedSongs | songs paused by PauseAllMusic | empty | 0x4184d0 |
| 0x130c | int | mLastInputUpdateCnt | update count of the last board input | 0 | Board 0x409de0/0x40a0c0/0x40fc40 (idle > 14 updates); GotFocus sets = mUpdateCount (+0x4b8) |
| 0x1310 | string | mSEHInfo | added to the crash report, never written | "" | GetGameSEHInfo |
| 0x132c | int | mPlayTicks | ++ every Board update in play | 0 | Board 0x40bd40; Shutdown (/6000 = minutes) |
| 0x1330 | bool | mLevelReached | set by Board::LevelUp 0x404b40 | false | StartGame clears, ShutdownWidgets counts |
| 0x1334 | int | mPlaysToCount | ++ in ShutdownWidgets if mLevelReached | 0 | Shutdown |
| 0x1338 | int | mMaxExecutions | property MaxExecutions | 0 (Init) | trial (unused here) |
| 0x133c | int | mMaxPlays | property MaxPlays | 0 (Init) | Shutdown, Board 0x40ed70/0x40f5b0 |
| 0x1340 | int | mMaxTime | property MaxTime (minutes) | 60 (Init) | Shutdown, Board, TitleScreen 0x433900/0x433e80/0x434380 |
| 0x1344 | | | 4 bytes never touched | - | - |

The ctor does not initialise +0x1164 (mTitleScreen) or +0x1178 (mUserManager) (open question 4). Normal
runs set both before any read (SexyApp::Init calls ReadFromRegistry; BookWormApp::Init creates the title), but
an Init that fails before the title exists (registry check, missing image/font/sound) reaches
`ShutdownWidgets`, which tests +0x1164. Port: initialise both to NULL.

#### 1.2 Math (0x42e330-0x42e3d0)

sizeof 0xb48, vtable 0x50074c (slot 0 = scalar deleting dtor 0x42e3a0; body 0x42e390 only resets the
vptr). Ctor 0x42e330: mPi (+0xb44) = 0x40490fda (3.1415925f); for i = 0..359: `mSin[i]` (+0x4 + 4i) =
(float)fsin((float)mPi * i * (1/180)) and `mNegCos[i]` (+0x5a4 + 4i) = (float)-fcos(i * (float)mPi * (1/180)),
computed in x87 extended precision with the double constant 0.005555555555555556 (0x500740), stored as
float. A disassembly search finds no other reference to +0x61c, +0xbbc or +0x115c of the app: the tables are
dead. Port: keep the member for fidelity or drop it; nothing reads it.

#### 1.3 UserManager and User (registry profiles)

`UserManager` (0x2c, ctor 0x435b50, dtor body 0x434880, created in ReadFromRegistry with `new(0x2c)`):

| Off | Type | Name | Meaning |
|---|---|---|---|
| 0x00 | BookWormApp* | mApp | for mRegKey (+0x40) and the option flags |
| 0x04 | list<User*> | mUsers | head +0x08, size +0x0c (BWGameSelector reads +0x0c = user count) |
| 0x10 | string | mCurrentUser | current player name |

`User` (0x38, ctor 0x434960(name) copies the name into both strings): +0x00 string mName, +0x1c string mAlias.

All registry access is direct Win32 on `HKEY_LOCAL_MACHINE\SOFTWARE\<mRegKey>\Users` (KEY_READ 0x20019 to read,
KEY_SET_VALUE|KEY_CREATE_SUB_KEY 0x20006 to write). Port: the emulated registry with value names
`Users\<name>\<value>` and `Users\User`, `RegistryGetSubKeys("Users")` for the enumeration (Windows returns
subkeys sorted case-insensitively: keep that order), `RegistryEraseKey("Users\<name>")` for DeleteUser.

- **ctor 0x435b50**: list empty, mCurrentUser ""; open `Users`; RegQueryInfoKey for the subkey count; for
  index 0.. (stop at the first RegEnumKeyEx failure or at the count): `LoadUser(subkeyName)` and push the
  result (even NULL) to the back of mUsers; close; `mCurrentUser = GetLastUser()`.
- **LoadUser(name) 0x435470**: open `Users\<name>`; if it fails return NULL; read "Alias" (REG_SZ, buffer 256);
  `new User(name)`; mAlias = the alias read ("" if absent; overrides the ctor's copy of the name).
- **GetLastUser() 0x4359b0**: read `Users\User` (REG_SZ, 256); if empty: walk mUsers and take the first
  name for which `SetCurrentUser` succeeds. Returns the name (not checked against the list when read).
- **SetCurrentUser(name) 0x4356d0**: find a user with `_stricmp(user.mName, name) == 0`; open `Users`
  (write); `RegSetValueEx("User", REG_SZ, 256 bytes copied from the name)`; on success mCurrentUser = name;
  returns success. Not found or no key: false.
- **FindUser(name) 0x434800**: first user with `_stricmp(user.mName, name) == 0`, else NULL.
- **GetCurrentUserName() 0x402090** (inline, folded into BWGameSelector.obj): copy of mCurrentUser.
- **GetCurrentAlias() 0x435910**: name = mCurrentUser; if not empty and FindUser(name) has a non-empty mAlias,
  return the alias, else the name.
- **SetCurrentAlias(alias) 0x435880**: if the current user exists: mAlias = alias, SaveUser(user); returns
  whether it existed.
- **ReadUserInt(user, value, int*) 0x4349e0**: FindUser, open `Users\<user.mName>`, RegQueryValueEx (4 bytes),
  store on success; returns success. **WriteUserInt 0x434d10**: REG_DWORD. **WriteUserString 0x434b70**:
  REG_SZ, always 256 bytes (the string copied into a 256-byte stack buffer, garbage after the NUL).
- **SaveUser(User*) 0x434e90**: open `Users\<mName>` for writing, else RegCreateKeyEx it (fail: return);
  write "Alias" = mAlias, "ShowHelp" = app mShowHelp (DWORD 0/1), "ShowHints" = mShowHints, "ShowDefs" =
  mShowDefs, "TimesPlayed" = app mUserTimesPlayed. The option values are the app's current ones.
- **DeleteUser(name) 0x4352e0**: open `Users`; for every user matching (_stricmp): RegDeleteKey(Users, name)
  (the argument string, not the stored one); on success delete the User and erase the node. The player's
  `players\<name>` folder is not removed.
- **~UserManager 0x434880**: delete every User (string dtors), clear the list, free mCurrentUser.

#### 1.4 High-score entries and predicates

`HighScoreEntry` (0x40, copy ctor 0x412080): +0x00 string mName, +0x1c string mWord, +0x38 int mScore,
+0x3c bool mIsNew. Both predicates take their arguments **by value** (copy, compare, destroy):
- ByScore 0x4115d0: `a.mScore != b.mScore ? a.mScore > b.mScore : strcmp(a.mWord, b.mWord) < 0` (byte
  compare). Equivalent entries = same score and same word (the name is ignored).
- ByWordLength 0x411570: `a.mWord.size() != b.mWord.size() ? a.size > b.size : a.mScore > b.mScore`.

`multiset::insert` places a new entry after the existing equivalent ones (VC7 and libc++ agree). `find` is
lower_bound + equivalence.

### 2. Resources

#### 2.1 Tables and loaders

- **LoadImage(id) 0x4144b0**: `static std::string sImagePath = GetString("ImagePath", "images\\")` (0x534828,
  guard 0x534844, atexit dtor). `mImages[id] = GetImage(sImagePath + gImageNames[id])` (SexyApp::GetImage
  0x47d930 = ImageLib with alpha lookup, `.png/.gif/.jpg` plus `_name` alpha files, always commits the bits).
  NULL: `Popup("Failed to load " + path)` (slot 21, MessageBox "FATAL ERROR"), mLoadingFailed (+0x528) =
  true, return false.
- **LoadFont(id) 0x4146d0**: `static sFontPath = GetString("FontPath", "fonts\\")` (0x534848, guard 0x534864);
  `mFonts[id] = new ImageFont(this, sFontPath + gFontNames[id])` (stored even when it fails); if
  `!font->mFontData->mInitialized` (+0x14 -> +0x4): Popup("Failed to load " + path), mLoadingFailed = true,
  return false.
- **Sounds** (Init): `static gSoundPath = GetString("SoundPath", "sounds\\")` (0x534868, guard 0x534884);
  `mSoundManager->LoadSound(i, gSoundPath + gSoundNames[i])` for i = 0..46 (DSoundManager tries
  `cached_<name>.wav`, `.wav`, `.ogg`, `.au`, docs/FRAMEWORK_2004.md section 6, 1.3). False, or mShutdown set: Shutdown(), return.
- **OptimizeImage(img, name) 0x411220** (EDI = image, EAX = &name; after every image and tile set):
  `if (img->Palletize()) OutputDebugString("<name> - PALLETIZED!\r\n") else { "<name> - 32 Bit Only\r\n";
  if (DDImage* d = dynamic_cast<DDImage*>(img)) { if (gSexyAppBase->Is3DAccelerated()) d->mWantDDSurface
  (+0x53) = true; else d->GenerateDDSurface(); } }`. Port: Palletize only (graphics.md).

#### 2.2 Fonts (gFontNames 0x52e3c0; `mFonts[id]` at app+0x11b0+4*id)

| id | app offset | descriptor | files in fonts/ | loaded by |
|---|---|---|---|---|
| 0 | 0x11b0 | Verdana10Bold.txt | _verdana10bold.png, verdana10bold.txt | Init |
| 1 | 0x11b4 | Verdana12Bold.txt | _verdana12bold.png, verdana12bold.txt | Init |
| 2 | 0x11b8 | Verdana14Bold.txt | _verdana14bold.png, verdana14bold.txt | Init |
| 3 | 0x11bc | Verdana18BoldColor.txt | _verdana18boldcolor.gif, verdana18boldcolor.gif, verdana18boldcolor.txt | Init |
| 4 | 0x11c0 | Lockergnome14Outline.txt | _lockergnome14outline.gif, lockergnome14outline.gif, lockergnome14outline.txt | Init |
| 5 | 0x11c4 | Verdana18Bold.txt | _verdana18bold.png, verdana18bold.txt | loading thread |
| 6 | 0x11c8 | ScrippsCollegeOS13Bold.txt | _scrippscollegeos13bold.gif, scrippscollegeos13bold.txt | loading thread |
| 7 | 0x11cc | ScrippsCollegeOS15Bold.txt | _scrippscollegeos15bold.gif, scrippscollegeos15bold.txt | loading thread |
| 8 | 0x11d0 | ScrippsCollegeOS18Bold.txt | _scrippscollegeos18bold.gif, scrippscollegeos18bold.txt | loading thread |
| 9 | 0x11d4 | Lockergnome18.txt | _lockergnome18.gif, lockergnome18.txt | loading thread |
| 10 | 0x11d8 | Lockergnome24.txt | _lockergnome24.gif, lockergnome24.txt | loading thread |
| 11 | 0x11dc | CourierFinalDraft20Bold.txt | _courierfinaldraft20bold.gif, courierfinaldraft20bold.gif, courierfinaldraft20bold.txt | loading thread |
| 12 | 0x11e0 | GoldScore.txt | _goldscore.gif, goldscore.gif, goldscore.txt | loading thread |
| 13 | 0x11e4 | GoldScoreHeated.txt | _goldscoreheated.gif, goldscoreheated.gif, goldscoreheated.txt | loading thread |
| 14 | 0x11e8 | LockergnomeSmallColor.txt | _lockergnomesmallcolor.gif, lockergnomesmallcolor.gif, lockergnomesmallcolor.txt | loading thread |
| 15 | 0x11ec | LegacySerifMdSCITCTT12.txt | _legacyserifmdscitctt12.gif, legacyserifmdscitctt12.txt | loading thread |
| 16 | 0x11f0 | MyriadWeb10.txt | _myriadweb10.gif, myriadweb10.txt | loading thread |
| 17 | 0x11f4 | MyriadCondensedWeb11.txt | _myriadcondensedweb11.png, myriadcondensedweb11.txt | loading thread |
| 18 | 0x11f8 | MyriadWeb12Bold.txt | _myriadweb12bold.gif, myriadweb12bold.txt | loading thread |

Usage in this module: dialogs built by `NewDialog` use font 0 for buttons, header and lines; `DoDialog`
switches to header 3 (Verdana18BoldColor), lines 1 (Verdana12Bold), buttons 1. HintDialog uses 14 for its
header (dialog group).

#### 2.3 Images (gImageNames 0x52e410; `mImages[id]` at `*(app+0x11ac) + 4*id`)

Init loads 0-16, the loading thread 17-148. Files are the ones staged in `assets/bookworm/images` (`_name` =
alpha). Not in the table and never loaded: `lex_wow_1..7`. `font_tile_letters` has only an alpha file
(white image).

| id | name | files (images/) | read by (direct index; other uses are computed) |
|---|---|---|---|
| 0 | backdrop_title | backdrop_title.jpg | TitleScreen (index 0) |
| 1 | backdrop_title_loadmeter | backdrop_title_loadmeter.gif, _backdrop_title_loadmeter.gif | TitleScreen |
| 2 | backdrop_register | backdrop_register.gif, _backdrop_register.gif | BookWormApp, RegisterDialog |
| 3 | logo_title | logo_title.gif, _logo_title.gif | TitleScreen |
| 4 | title_loadmeter | title_loadmeter.gif, _title_loadmeter.gif | TitleScreen |
| 5 | title_stamp_trial | title_stamp_trial.gif, _title_stamp_trial.gif | TitleScreen |
| 6 | title_stamp_registered | title_stamp_registered.gif, _title_stamp_registered.gif | TitleScreen |
| 7 | dialog | dialog.gif, _dialog.gif | BookWormApp, CreditsDialog, DeleteUserDialog, HintDialog, NewUserDialog, OptionsDialog, OverlayWidget, RegisterDialog, SelectUserDialog |
| 8 | dialogbutton | dialogbutton.gif, _dialogbutton.gif | Board, BookWormApp, CreditsDialog, DeleteUserDialog, HintDialog, NewUserDialog, RegisterDialog, SelectUserDialog |
| 9 | button_reg_register | button_reg_register.gif, _button_reg_register.gif | RegisterDialog |
| 10 | button_reg_cancel | button_reg_cancel.gif, _button_reg_cancel.gif | RegisterDialog |
| 11 | button_reg_regcode | button_reg_regcode.gif, _button_reg_regcode.gif | RegisterDialog |
| 12 | editbox | editbox.gif | NewUserDialog |
| 13 | cursor_pointer | cursor_pointer.gif, _cursor_pointer.gif | cursor 0 |
| 14 | cursor_hand | cursor_hand.gif, _cursor_hand.gif | cursor 1 |
| 15 | cursor_dragging | cursor_dragging.gif, _cursor_dragging.gif | cursor 2 |
| 16 | cursor_text | cursor_text.gif, _cursor_text.gif | cursor 3 |
| 17 | backdrop_board | backdrop_board.gif | Board |
| 18 | backdrop_board_scorched | backdrop_board_scorched.gif | Board |
| 19 | backdrop_bonus | backdrop_bonus.gif, _backdrop_bonus.gif | OverlayWidget |
| 20 | backdrop_menu | backdrop_menu.gif | BWGameSelector |
| 21 | backdrop_fame | backdrop_fame.gif | FameWidget |
| 22 | backdrop_help | backdrop_help.gif | HelpWidget |
| 23 | backdrop_help_box | backdrop_help_box.gif, _backdrop_help_box.gif | HelpWidget |
| 24 | backdrop_ihs | backdrop_ihs.gif | FameWidget |
| 25 | backdrop_wb_speech | backdrop_wb_speech.gif, _backdrop_wb_speech.gif | (computed index or not found) |
| 26 | backdrop_wb_thought | backdrop_wb_thought.gif, _backdrop_wb_thought.gif | Lex |
| 27 | backdrop_wb_text | backdrop_wb_text.gif, _backdrop_wb_text.gif | (computed index or not found) |
| 28 | gameselect_action | gameselect_action.gif, _gameselect_action.gif | BWGameSelector |
| 29 | gameselect_action_over | gameselect_action_over.gif, _gameselect_action_over.gif | BWGameSelector |
| 30 | gameselect_classic | gameselect_classic.gif, _gameselect_classic.gif | BWGameSelector |
| 31 | gameselect_classic_over | gameselect_classic_over.gif, _gameselect_classic_over.gif | BWGameSelector |
| 32 | gameselect_hall | gameselect_hall.gif, _gameselect_hall.gif | BWGameSelector |
| 33 | gameselect_hall_over | gameselect_hall_over.gif, _gameselect_hall_over.gif | BWGameSelector |
| 34 | gameselect_quit | gameselect_quit.gif, _gameselect_quit.gif | BWGameSelector |
| 35 | gameselect_quit_over | gameselect_quit_over.gif, _gameselect_quit_over.gif | BWGameSelector |
| 36 | gameselect_bubble | gameselect_bubble.gif, _gameselect_bubble.gif | BWGameSelector |
| 37 | dialog_levelup | dialog_levelup.gif, _dialog_levelup.gif | LabelWidget, LevelUpWidget |
| 38 | button_fame_continue | button_fame_continue.gif, _button_fame_continue.gif | FameWidget |
| 39 | button_help_continue | button_help_continue.gif, _button_help_continue.gif | HelpWidget |
| 40 | button_goto_ihs | button_goto_ihs.gif, _button_goto_ihs.gif | FameWidget |
| 41 | button_goto_fame | button_goto_fame.gif, _button_goto_fame.gif | FameWidget |
| 42 | button_submit_ihs | button_submit_ihs.gif, _button_submit_ihs.gif | FameWidget |
| 43 | button_submit_ihs_hilite | button_submit_ihs_hilite.gif, _button_submit_ihs_hilite.gif | (computed index or not found) |
| 44 | button_help_topic | button_help_topic.gif, _button_help_topic.gif | HelpWidget |
| 45 | button_help_topic_hilite | button_help_topic_hilite.gif, _button_help_topic_hilite.gif | HelpWidget |
| 46 | ihs_tab_names | ihs_tab_names.gif, _ihs_tab_names.gif | FameWidget |
| 47 | ihs_tab_names_hilite | ihs_tab_names_hilite.gif, _ihs_tab_names_hilite.gif | FameWidget |
| 48 | ihs_tab_indicator | ihs_tab_indicator.gif, _ihs_tab_indicator.gif | FameWidget |
| 49 | ihs_alias_warning | ihs_alias_warning.gif | BookWormApp |
| 50 | bar_levelup_header | bar_levelup_header.gif, _bar_levelup_header.gif | LevelUpWidget |
| 51 | bar_gameover_header | bar_gameover_header.gif, _bar_gameover_header.gif | LevelUpWidget |
| 52 | bar_footer_scroll | bar_footer_scroll.gif, _bar_footer_scroll.gif | LevelUpWidget |
| 53 | bar_footer_continue | bar_footer_continue.gif, _bar_footer_continue.gif | LevelUpWidget |
| 54 | rankstrip | rankstrip.jpg | deleted after the copy to mRankStripImage (+0x124c); BookWormApp |
| 55 | rankframe | rankframe.gif | LevelUpWidget |
| 56 | level_up_star | level_up_star.gif, _level_up_star.gif | LevelUpWidget |
| 57 | fx_ashflake | fx_ashflake.gif, _fx_ashflake.gif | (computed index or not found) |
| 58 | fx_ember_sm | fx_ember_sm.gif, _fx_ember_sm.gif | Effect |
| 59 | fx_ember_med | fx_ember_med.gif, _fx_ember_med.gif | Effect |
| 60 | fx_ember_lrg | fx_ember_lrg.gif, _fx_ember_lrg.gif | Effect |
| 61 | fx_ember_smoke_sm | fx_ember_smoke_sm.gif, _fx_ember_smoke_sm.gif | Effect |
| 62 | fx_ember_smoke_med | fx_ember_smoke_med.gif, _fx_ember_smoke_med.gif | Effect |
| 63 | fx_ember_smoke_lrg | fx_ember_smoke_lrg.gif, _fx_ember_smoke_lrg.gif | Effect |
| 64 | fx_particle_sm | fx_particle_sm.gif, _fx_particle_sm.gif | (computed index or not found) |
| 65 | fx_particle_med | fx_particle_med.gif, _fx_particle_med.gif | (computed index or not found) |
| 66 | fx_particle_lrg | fx_particle_lrg.gif, _fx_particle_lrg.gif | (computed index or not found) |
| 67 | fx_spark_sm | fx_spark_sm.gif | Effect |
| 68 | fx_spark_med | fx_spark_med.gif | Effect |
| 69 | fx_spark_lrg | fx_spark_lrg.gif | Effect |
| 70 | fx_smoke_sm | _fx_smoke_sm.gif | Board, Effect |
| 71 | fx_flame_lrg | fx_flame_lrg.gif | Effect |
| 72 | fx_fireline | fx_fireline.gif, _fx_fireline.gif | Board, OverlayWidget |
| 73 | fx_lex_sweat_seq | fx_lex_sweat_seq.gif, _fx_lex_sweat_seq.gif | OverlayWidget |
| 74 | fx_lex_sweat_trickle | fx_lex_sweat_trickle.gif, _fx_lex_sweat_trickle.gif | (computed index or not found) |
| 75 | tile | tile.gif, _tile.gif | tile-set base; BookWormApp |
| 76 | tile_hilite | tile_hilite.gif, _tile_hilite.gif | tile set 1 |
| 77 | tile_fire | tile_fire.gif, _tile_fire.gif | tile set 6 |
| 78 | tile_fire_hilite | tile_fire_hilite.gif, _tile_fire_hilite.gif | tile set 7 |
| 79 | tile_gold | tile_gold.gif, _tile_gold.gif | tile set 4 |
| 80 | tile_gold_hilite | tile_gold_hilite.gif, _tile_gold_hilite.gif | tile set 5 |
| 81 | tile_green | tile_green.gif, _tile_green.gif | tile set 2 |
| 82 | tile_green_hilite | tile_green_hilite.gif, _tile_green_hilite.gif | tile set 3 |
| 83 | tile_sapphire | tile_sapphire.gif, _tile_sapphire.gif | Board, BookWormApp |
| 84 | tile_sapphire_hilite | tile_sapphire_hilite.gif, _tile_sapphire_hilite.gif | Board |
| 85 | tile_diamond | tile_diamond.gif, _tile_diamond.gif | Board, BookWormApp |
| 86 | tile_diamond_hilite | tile_diamond_hilite.gif, _tile_diamond_hilite.gif | Board |
| 87 | tile_charred | tile_charred.gif, _tile_charred.gif | tile set 8; Board |
| 88 | tile_gemhilite | tile_gemhilite.gif | Board |
| 89 | tile_spin_fx | tile_spin_fx.gif | Board |
| 90 | tile_x | tile_x.gif, _tile_x.gif | Board |
| 91 | font_tile_letters | _font_tile_letters.gif | Board, BookWormApp, Effect |
| 92 | font_tile_digits | font_tile_digits.gif, _font_tile_digits.gif | Board, BookWormApp |
| 93 | tile_green_glow | tile_green_glow.gif | (computed index or not found) |
| 94 | tile_gold_glow | tile_gold_glow.gif | (computed index or not found) |
| 95 | tile_fire_glow | tile_fire_glow.gif | (computed index or not found) |
| 96 | tile_fire_fx1 | tile_fire_fx1.gif | (computed index or not found) |
| 97 | tile_fire_fx2 | tile_fire_fx2.gif | (computed index or not found) |
| 98 | arrows | arrows.gif, _arrows.gif | Board |
| 99 | bonusarrow_left | bonusarrow_left.gif, _bonusarrow_left.gif | OverlayWidget |
| 100 | bonusarrow_right | bonusarrow_right.gif, _bonusarrow_right.gif | HelpWidget, OverlayWidget |
| 101 | bonusarrow_top | bonusarrow_top.gif, _bonusarrow_top.gif | OverlayWidget |
| 102 | lex_shadow | _lex_shadow.gif | Board |
| 103 | lex_headless | lex_headless.gif, _lex_headless.gif | Lex |
| 104 | lex_glow_idle | lex_glow_idle.gif, _lex_glow_idle.gif | Lex |
| 105 | lex_smile_idle | lex_smile_idle.gif, _lex_smile_idle.gif | Lex |
| 106 | lex_smile_blink | lex_smile_blink.gif, _lex_smile_blink.gif | (computed index or not found) |
| 107 | lex_giggle_1 | lex_giggle_1.gif, _lex_giggle_1.gif | (computed index or not found) |
| 108 | lex_giggle_2 | lex_giggle_2.gif, _lex_giggle_2.gif | (computed index or not found) |
| 109 | lex_slurp_1 | lex_slurp_1.gif, _lex_slurp_1.gif | (computed index or not found) |
| 110 | lex_speak_ah | lex_speak_ah.gif, _lex_speak_ah.gif | (computed index or not found) |
| 111 | lex_speak_ee | lex_speak_ee.gif, _lex_speak_ee.gif | (computed index or not found) |
| 112 | lex_speak_oh | lex_speak_oh.gif, _lex_speak_oh.gif | (computed index or not found) |
| 113 | lex_speak_pb | lex_speak_pb.gif, _lex_speak_pb.gif | (computed index or not found) |
| 114 | lex_sweat | lex_sweat.gif, _lex_sweat.gif | (computed index or not found) |
| 115 | lex_sweat_facing | lex_sweat_facing.gif, _lex_sweat_facing.gif | (computed index or not found) |
| 116 | lex_burnt | lex_burnt.gif, _lex_burnt.gif | Lex |
| 117 | lex_burnt_blink | lex_burnt_blink.gif, _lex_burnt_blink.gif | (computed index or not found) |
| 118 | lex_eating | lex_eating.gif, _lex_eating.gif | Board, Lex |
| 119 | lexbig_speak_ah | lexbig_speak_ah.gif, _lexbig_speak_ah.gif | (computed index or not found) |
| 120 | lexbig_speak_ee | lexbig_speak_ee.gif, _lexbig_speak_ee.gif | (computed index or not found) |
| 121 | lexbig_speak_mm | lexbig_speak_mm.gif, _lexbig_speak_mm.gif | (computed index or not found) |
| 122 | lexbig_speak_oo | lexbig_speak_oo.gif, _lexbig_speak_oo.gif | (computed index or not found) |
| 123 | lexbig_speak_pb | lexbig_speak_pb.gif, _lexbig_speak_pb.gif | (computed index or not found) |
| 124 | wb_text_shout_tail | wb_text_shout_tail.gif, _wb_text_shout_tail.gif | (computed index or not found) |
| 125 | wb_text_talk_tail | wb_text_talk_tail.gif, _wb_text_talk_tail.gif | (computed index or not found) |
| 126 | wb_text_think_tail | wb_text_think_tail.gif, _wb_text_think_tail.gif | (computed index or not found) |
| 127 | progress_green | progress_green.gif, _progress_green.gif | (computed index or not found) |
| 128 | progress_lavender | progress_lavender.gif, _progress_lavender.gif | (computed index or not found) |
| 129 | progress_lime | progress_lime.gif, _progress_lime.gif | (computed index or not found) |
| 130 | progress_magenta | progress_magenta.gif, _progress_magenta.gif | (computed index or not found) |
| 131 | progress_multi | progress_multi.gif, _progress_multi.gif | (computed index or not found) |
| 132 | progress_navy | progress_navy.gif, _progress_navy.gif | (computed index or not found) |
| 133 | progress_orange | progress_orange.gif, _progress_orange.gif | (computed index or not found) |
| 134 | progress_purple | progress_purple.gif, _progress_purple.gif | (computed index or not found) |
| 135 | progress_red | progress_red.gif, _progress_red.gif | (computed index or not found) |
| 136 | progress_tan | progress_tan.gif, _progress_tan.gif | (computed index or not found) |
| 137 | progress_teal | progress_teal.gif, _progress_teal.gif | (computed index or not found) |
| 138 | progress_yellow | progress_yellow.gif, _progress_yellow.gif | (computed index or not found) |
| 139 | progress_burnt | progress_burnt.gif, _progress_burnt.gif | (computed index or not found) |
| 140 | sparkle | sparkle.gif | (computed index or not found) |
| 141 | sparkle_large | sparkle_large.gif | (computed index or not found) |
| 142 | dialogbutton_plain | dialogbutton_plain.gif | OptionsDialog |
| 143 | slidertrack | slidertrack.gif | OptionsDialog |
| 144 | sliderthumb | sliderthumb.gif, _sliderthumb.gif | OptionsDialog |
| 145 | checkboxes | checkboxes.gif | BookWormApp |
| 146 | checkboxes_help | checkboxes_help.gif | HelpWidget |
| 147 | waitbar | waitbar.gif | (computed index or not found) |
| 148 | clouds | _clouds.jpg | CreditsDialog |

#### 2.4 Derived images (loading thread)

- **Checkbox halves**: `w = width(checkboxes)/2` (int division), `h = height`; `mCheckboxOffImage (+0x1200) =
  CopyImage(img145, Rect(0,0,w,h))`, `mCheckboxOnImage (+0x11fc) = CopyImage(img145, Rect(w,0,w,h))`
  (SexyApp::CopyImage 0x47d9f0 -> DDImage).
- **Tile sets** `mTileSetImages[s]` for s = 0..8: `new MemoryImage(app)` (0x48), `Create(width(tile) * 26,
  height(tile))` with `tile` = image 75 (Create sets mHasTrans/mHasAlpha). Per set a background image B and a
  letter grey level c:

  | s | B (image) | c | meaning (inferred) |
  |---|---|---|---|
  | 0 | 75 tile | 0 | normal |
  | 1 | 76 tile_hilite | 255 | normal, selected |
  | 2 | 81 tile_green | 0 | green bonus |
  | 3 | 82 tile_green_hilite | 255 | green, selected |
  | 4 | 79 tile_gold | 0 | gold |
  | 5 | 80 tile_gold_hilite | 176 | gold, selected |
  | 6 | 77 tile_fire | 0 | fire |
  | 7 | 78 tile_fire_hilite | 255 | fire, selected |
  | 8 | 87 tile_charred | 176 | charred |

  For each letter k = 0..25 (A..Z), with `x = width(B) * k`, L = image 91 (font_tile_letters, 26 rows), D =
  image 92 (font_tile_digits, 10 rows): `DrawImage(B, x, 0)`; `SetColorizeImages(true)`; `lh =
  height(L)/26`; `SetColor(Color(c,c,c))`; `DrawImage(L, x, 0, Rect(0, lh*k, width(L), lh))`; value v = 10
  for k = 16 (Q, the "Qu" tile) else gLetterValues[k] (0x4fa480: A1 B4 C4 D3 E1 F5 G3 H5 I1 J8 K7 L2 M4 N2
  O1 P4 Q7 R2 S1 T2 U2 V5 W6 X8 Y6 Z10); `dh = height(D)/10`; `SetColor(Color(255,255,255,255))`;
  `DrawImage(D, x + 54 - width(D), 53 - dh, Rect(0, (v-1)*dh, width(D), dh))`; `SetColorizeImages(false)`.
  Then `OptimizeImage(set, StrFormat("Tile set %d", s))`.
- **Copies** (only if the field is still NULL): `new MemoryImage(app)`, `Create(w, h)` of the source,
  `DrawImage(src, 0, 0)`, push_back on mCopiedImages: +0x1244 from image 83 tile_sapphire, +0x1248 from 85
  tile_diamond, +0x124c from 54 rankstrip, after which `delete mImages[54]; mImages[54] = NULL`.

#### 2.5 Sounds (gSoundNames 0x52e668; id 24 repeats buttonclick)

| id | name | file in sounds/ |
|---|---|---|
| 0 | buttonclick | buttonclick.ogg |
| 1 | gameovertone | gameovertone.ogg |
| 2 | bad | bad.ogg |
| 3 | blank | blank.au |
| 4 | up1 | up1.ogg |
| 5 | up2 | up2.ogg |
| 6 | up3 | up3.ogg |
| 7 | up4 | up4.ogg |
| 8 | up5 | up5.ogg |
| 9 | up6 | up6.ogg |
| 10 | up7 | up7.ogg |
| 11 | up8 | up8.ogg |
| 12 | up9 | up9.ogg |
| 13 | nextselect | nextselect.ogg |
| 14 | wordcomplete | wordcomplete.ogg |
| 15 | bonuscomplete | bonuscomplete.ogg |
| 16 | buzzer | buzzer.ogg |
| 17 | piece_hit | piece_hit.ogg |
| 18 | chomp | chomp.ogg |
| 19 | flametile | flametile.ogg |
| 20 | warning | warning.ogg |
| 21 | tileburn | tileburn.ogg |
| 22 | appear2 | appear2.ogg |
| 23 | goldtile | goldtile.ogg |
| 24 | buttonclick | buttonclick.ogg |
| 25 | redtilecleared | redtilecleared.ogg |
| 26 | spinsuperbonus | spinsuperbonus.ogg |
| 27 | bonusword | bonusword.ogg |
| 28 | screenburngameover | screenburngameover.ogg |
| 29 | lex_astonishing | lex_astonishing.ogg |
| 30 | lex_awesome | lex_awesome.ogg |
| 31 | lex_burp | lex_burp.ogg |
| 32 | lex_careful | lex_careful.ogg |
| 33 | lex_danger | lex_danger.ogg |
| 34 | lex_excellent | lex_excellent.ogg |
| 35 | lex_fantastic | lex_fantastic.ogg |
| 36 | lex_getready | lex_getready.ogg |
| 37 | lex_go | lex_go.ogg |
| 38 | lex_incredible | lex_incredible.ogg |
| 39 | lex_letsgo | lex_letsgo.ogg |
| 40 | lex_levelup | lex_levelup.ogg |
| 41 | lex_lookout | lex_lookout.ogg |
| 42 | lex_scramble | lex_scramble.ogg |
| 43 | lex_unbelievable | lex_unbelievable.ogg |
| 44 | lex_verygood | lex_verygood.ogg |
| 45 | lex_watchout | lex_watchout.ogg |
| 46 | lex_welcome | lex_welcome.ogg |

This module plays only `PlaySample(0)` (ButtonPress), `PlaySample(2)` (invalid code, not ported) and
`PlaySample(26)` (thank-you dialog, not ported).

#### 2.6 Cursors

Init: `SetCursorImage(0..3, images 13..16)` (pointer, hand, dragging, text). They are drawn only when custom
cursors are enabled (registry CustomCursors, OptionsDialog; BookWormApp's ctor sets mCustomCursorsEnabled
false). Ids 4 (circle-slash) and 5 (none) have no image. Port: `SetCursorImage` the same four; id 5 is
CURSOR_NONE.

### 3. Start-up and loading

#### 3.1 WinMain 0x40fe80

`gHInstance = hInstance; gApp (0x5347d4) = new BookWormApp` (0x1348); `ParseCmdLine(lpCmdLine)` (slot 34)
**before** Init; `mWidth = 640; mHeight = 480`; `Init()` (slot 16); `gGameRand (0x5347d0) = new
MTRand(Sexy::Rand())` (one draw from the framework's global MTRand, seeded by SexyApp::Init with
GetTickCount; MTRand(ulong) seeds with SAF 1.2's 69069 rule: `SeedMTRand2004` in the port); `Start()` (slot
36); `Shutdown()` (slot 33); `delete gApp` (slot 14); `delete gGameRand`; return 0. gGameRand is used by the
Board group (MTRand::Next at 0x403d10, 0x404150, 0x408030, 0x40b540, 0x40ba80, 0x40bd40, 0x40ed70).
Port: the framework's main creates the app; create gGameRand right after Init with the same seeding.

#### 3.2 Constructor 0x41a0f0

SexyApp::SexyApp(); vptr; Math(); the four multisets, the list, both sets, the vector<string> and vector<int>
empty, the strings empty; `mMusicInterface = NULL`; fonts and +0x1204..+0x1234 zeroed; +0x11fc, +0x1200,
+0x11ac, +0x1244..0x124c, +0x1160, +0x1168, +0x1170, +0x1174 = 0; mSelectedGameMode = -1; mGameMode = 0;
mScoringMethod = 1; mGameOption = 1; mUserTimesPlayed = 0; flags +0x12e4..+0x12fb false except mFlag12e6,
mShowHelp, mShowHints, mShowDefs = true; +0x130c, +0x132c, +0x1334..+0x1340 = 0, mLevelReached = false;
mInitDone = false; the SexyApp fields listed in 1.1.

#### 3.3 ReadFromRegistry 0x41a540 (slot 19, called inside SexyApp::Init)

`SexyApp::ReadFromRegistry()`; `mUserManager = new UserManager(this)` (1.3); `LoadHighScores()` (9.2).

#### 3.4 Init 0x414a10 (slot 16)

1. `SexyApp::Init()` (docs/FRAMEWORK_2004.md 1.3: properties, ReadFromRegistry, WidgetManager, window, sound, music).
2. `CheckRegistryAccess()` 0x414910: `RegistryWriteInteger("test", 1)`; on failure: if fullscreen
   `SwitchScreenMode(true, Is3DAccelerated())` (slot 52), MessageBox(NULL, `GT_ADMIN_REQUIRED`, `GT_ADMIN_REQUIRED_HEADER`, MB_ICONHAND), return false ->
   `Shutdown()` and return. Port: the emulated registry always succeeds; keep the "test" value or skip it.
3. mMaxExecutions = GetInteger("MaxExecutions", 0); mMaxPlays = GetInteger("MaxPlays", 0); mMaxTime =
   GetInteger("MaxTime", 60).
4. `SetMusicVolume(mMusicVolume)` (slot 47, the override).
5. `mImages = new Image*[149]`, all NULL.
6. For i = 0..16: `LoadImage(i)` (false -> Shutdown, return); `OptimizeImage(mImages[i], name)`.
7. For i = 0..4: `LoadFont(i)` (false -> Shutdown, return).
8. The 47 sounds (2.1); failure or mShutdown -> Shutdown, return.
9. SetCursorImage 0..3 (2.6). mInitDone = true.
10. `mTitleScreen = new TitleScreen(this)` (0xc8 bytes, ctor 0x434380), `Resize(0, 0, mWidth, mHeight)`,
    `mWidgetManager->AddWidget(mTitleScreen)`.

SexyApp::Start then starts the loading thread (always) and the main loop.

#### 3.5 LoadingThreadProc 0x41a8a0 (slot 17)

`mNumLoadingThreadTasks (+0x538) = 216`; every step below does `++mCompletedLoadingThreadTasks (+0x53c)`
unless noted:
1. For song i = 0..4: `mMusicInterface->LoadMusic(i, "music\\Bookworm1.mo3")` (+1 each).
2. `mMusicInterface->PlayMusic(0, 27, false)`: the title theme starts while loading.
3. Fonts 5..18: LoadFont(i); stop (mLoadingFailed = true, return) on failure or mShutdown; +1 each.
4. Images 17..148: LoadImage(i), same stop rule; OptimizeImage; +1 each.
5. `GetString("SoundPath", "sounds\\")` (result unused). If `!mSoundManager->Initialized()` (slot 1): +47.
6. Checkbox halves; the 9 tile sets (+1 each) (2.4).
7. `LoadDictionary("wordlist.txt", &mDictionary)`; false: mLoadingFailed = true, return. +1.
8. `LoadBonusWords("bonus_word_dictionary.txt", &mBonusWords)`; +1.
9. `mDefinitionator = new Definitionator(this)` (0x14, ctor 0x41dd30); `mDefinitionator->Load("popdefs.txt")`
   (0x41d920, lex group); +1.
10. The three copies (2.4), +1 each.

Progress: with a sound device the counter ends at 166 of 216 (0.77) because step 5 adds 47 only without a
device; the bar then jumps to 1.0 when mLoaded is set (GetLoadingThreadProgress returns 1.0 once loaded).
Without a device it ends at 213. Port: the loading thread runs synchronously, so the bar jumps anyway;
keep 216 and the same increments.

mLoadingFailed makes SexyApp::Process call Shutdown (docs/FRAMEWORK_2004.md 1.4).

#### 3.6 Word lists: LoadWordList(file, set*, vector*) 0x41a5b0

Wrappers: `LoadDictionary(file, set)` 0x41a860 = LoadWordList(file, set, NULL); `LoadBonusWords(file, vec)`
0x41a880 = LoadWordList(file, NULL, vec). Both are thiscall on the app.

Format (both files are read with this parser; `wordlist.txt` uses the prefix numbers, the bonus list does
not): `fopen(file, "r")` (text mode, so CRLF reads as LF), then character by character with getc:
- state: `cur[256]` (current word), `prev[256]` (previous word), `len` (chars in cur), `n` (prefix count, the
  last number parsed, **sticky** across lines), `inNumber`.
- digit: if !inNumber { n = 0; inNumber = true }; `n = n*10 + digit`.
- '\r' or '\n': if len > 0: `word = cur[0..len)`; `prev = cur[0..len)`; if `word.size() > 2`:
  `UpperCaseString(word)` (CRT toupper per char, 0x4112f0) and insert into the set / push_back into the
  vector; `len = 0`. Always `inNumber = false`.
- other char: if `n > 0 && len == 0`: copy `prev[0..n)` into cur, `len = n`; then `cur[len++] = c`.
- before each character: if app->mShutdown, close and return **false**.
- EOF: close, `OutputDebugString(StrFormat("Time to load word list (%s): %d\r\n", file, ms))`, return true.

Example: `aa`, `2h`, `3ed`, `ing`, `s` -> AA (skipped, < 3), AAH, AAHED, AAHING (n = 3 kept), AAHS.
Words shorter than 3 letters are skipped but still become `prev`. No bounds check on the 256-byte buffers.
A missing file: getc(NULL) faults; the catch-all handler 0x41a790 prints "Exception raised: " to the
console and calls `exit(1)` (port: treat a missing word list as a load failure). Sizes: wordlist.txt 151477
lines, bonus_word_dictionary.txt 1784 lines (plain words, all >= 3 letters).

`popdefs.txt` (lex group, Definitionator::Load 0x41d920): one `WORD\t(pos.) definition` per line.

#### 3.7 LoadingThreadCompleted 0x4169d0 (slot 13)

`if (!GetBoolean("DontUpdate", true) && ShouldCheckForUpdate()) AskForUpdateCheck()`. Default true: never in
the retail build (and partner.xml sets DontUpdate=True). Port: empty.

#### 3.8 TitleScreenDone 0x414ef0 (slot 67, called by TitleScreen's ButtonDepress 0x433500)

`RemoveWidget(mTitleScreen)`; `SafeDeleteWidget(mTitleScreen)`; `mTitleScreen = NULL`; `name =
mUserManager->mCurrentUser` (copy); if not empty `LoadUserSettings()`; `ShowGameSelector()`.

### 4. Music

Songs 0-4 are five BASS channels of `music\Bookworm1.mo3` (3.5). Speeds below are per Update call;
**UpdateFrames 0x4100f0 runs `SexyApp::UpdateFrames()` (which ends with `mMusicInterface->Update()`) and then
`mMusicInterface->Update()` again**, so every fade moves twice per 10 ms tick: 0.005 = 1.0 s, 0.008 = 0.63 s,
0.01 = 0.5 s, 1.0 = instant. FadeIn starts from the song's current volume (0 if never played or stopped);
offset -1 continues from the current position.

| When | Calls (in order) | Where |
|---|---|---|
| loading | PlayMusic(0, 27, loop) | LoadingThreadProc 0x41a963 |
| main menu (ShowGameSelector) | FadeOut(1..4, stop, 0.01); if !IsPlaying(0): FadeIn(0, 27, 0.005, loop) | 0x410f00 |
| new game (StartGame) | FadeOut(0, 3, 4, stop, 0.005); FadeOut(27, stop, 0.0075) (no song 27: no-op, Q2); action: if !IsPlaying(2) FadeIn(2, 37, 0.005, loop), FadeOut(1, stop, 0.005); classic: FadeOut(2, stop, 0.005), if !IsPlaying(1) FadeIn(1, 0, 0.005, loop) | 0x410d10 |
| danger on (Board) | FadeOut(mode song, no stop, 0.008); FadeIn(4, 65 action / 51 classic, 0.008, loop) | Board 0x40bd40, LoadGame 0x408ab0 |
| danger off (Board) | FadeOut(4, stop, 0.008); FadeIn(mode song, -1, 0.008, loop) | Board 0x40bd40 |
| game over (Board 0x4038c0) | classic: StopMusic(1), StopMusic(4), FadeIn(3, 61, 1.0, **no loop**); action: StopMusic(2), StopMusic(4), FadeIn(3, 59, 1.0, no loop) | Board |
| hall of fame opens | FadeOut(0, no stop, 0.008); FadeIn(3, 54, 0.008, loop) | FameWidget::AddedToManager 0x421cb0 |
| hall of fame closes | FadeOut(3, stop, 0.008); FadeIn(0, -1, 0.008, loop) | FameWidget 0x426c10 |
| focus lost / board pause | PauseAllMusic(true) | 0x419040, Board 0x405d60/0x409de0 |
| focus regained / unpause | PauseAllMusic(false) | 0x419000, Board |
| exit | StopAllMusic | SexyApp::Shutdown |

**PauseAllMusic(pause) 0x4184d0**: pause: for song 0..4, if `IsPlaying(i)` (slot 11, BASS active and not
paused): `mPausedSongs.push_back(i)`, `PauseMusic(i)`. Resume: `ResumeMusic(id)` for every stored id in order,
then free the vector storage. A second pause before a resume finds the paused songs not playing and keeps
the old ids. **SetMusicVolume(v) 0x410110** (slot 47): SexyApp::SetMusicVolume(v); then again
`mMusicInterface->SetVolume(mMuteCount > 0 ? 0 : mMusicVolume)` if the interface exists (a repeat of what
the base already did; BookWormApp::Init calls it after SexyApp::Init made the interface).

Quirk: a FadeIn guarded by `!IsPlaying(n)` is skipped while a previous FadeOut(n, stop) of the same song is
still running (it still counts as playing), so the song finishes fading out and stays silent (menu -> game
within 0.5 s). Translate literally. A player cannot reach it (measured 2026-11-08): leaving a game fades its song
out in 49 updates, and the selector takes no click until it has slid in, about 70 updates later.

### 5. Screen flow and app functions

Flow: Title (loading, song 0) -> click -> `TitleScreenDone` -> **game selector** (forces a NewUserDialog when
there is no user) -> Classic/Action button -> `SelectGameMode` -> (saved game? `GT_CONTINUE_HEADER` dialog) ->
selector slot 47 -> `StartGameForUser` -> **Board** (help first if ShowHelp) -> level up (LevelUpWidget, board
group) -> game over (Board -> SubmitHighScores; LevelUpWidget offers the hall of fame -> `ShowHallOfFame`) ->
back to the selector. Options (Board button 17), Credits and Help from Options, Hall of Fame and Quit from
the selector, `GT_LEAVE_GAME_HEADER` from the Board's menu button (18).

| Address | Function | Behaviour |
|---|---|---|
| 0x40ffa0 | `ShutdownWidgets()` | KillDialog(15); remove + SafeDeleteWidget + NULL the title (+0x1164), selector (+0x1168), fame (+0x1170); if mBoard: if mLevelReached { ++mPlaysToCount; mLevelReached = false }; if `Board::IsGameInProgress()` (0x403720: Board +0x194 != 2) `Board::SaveGame(mGameMode)` (0x406c40); remove + SafeDeleteWidget + NULL the board. Only caller: Shutdown. |
| 0x410180 | `RemoveBoard()` | if mBoard: RemoveWidget, SafeDeleteWidget, NULL. Callers: StartGame, BWGameSelector::Update 0x402190 (end of its slide). |
| 0x4101c0 | `ShowHelp()` | `if (Dialog* d = GetDialog(1)) d->slot52()` (OptionsDialog close); if mHelpWidget == NULL: `mHelpWidget = new HelpWidget(this)` (0xa0, ctor 0x429cd0); `Resize(mWidth, mHeight, mWidth, mHeight)`; AddWidget; `Resize(0, 0, mWidth, mHeight)`; if mBoard: `Board::Pause(true)` (0x405be0, counted pause), `mBoard->SetVisible(false)`; if Board +0x15c (LevelUpWidget*): its SetDisabled(true) and SetVisible(false). Callers: StartGameForUser, Options 0x42fbe0, LoadGame 0x408ab0. |
| 0x4102d0 | `HideHelp()` | if mHelpWidget: RemoveWidget, SafeDeleteWidget, NULL; if mBoard: Pause(false), SetVisible(true); if Board +0x15c and Board +0x3c0 (bool): SetDisabled(false), SetVisible(true). Callers: StartGame, SaveGame, LoadGame, HelpWidget 0x429c70. |
| 0x410360 | `RemoveGameSelector()` | if mGameSelector: RemoveWidget, SafeDelete, NULL. |
| 0x4103a0 | `RemoveFameWidget()` | same for mFameWidget. Callers: ShowHallOfFame, FameWidget 0x426c10. |
| 0x410d10 | `StartGame()` | KillDialog(3); HideHelp(); music (section 4); RemoveBoard(); `mBoard = new Board(this)` (0x3f0, ctor 0x40ae90); Resize(0, 0, mWidth, mHeight); AddWidget; BringToBack; SetFocus(mBoard); `Board 0x403d80(false)`; `Board 0x408030()` (picks the bonus word); `if (GetDialog(5)) mBoard +0x194 = 3` (dead in practice: dialog 5 is killed before); if mGameSelector: slot 49 (0x4011f0, starts its slide-out); mLevelReached = false. |
| 0x410f00 | `ShowGameSelector()` | KillDialog(3); FadeOut(1..4, stop, 0.01); RemoveGameSelector(); `mGameSelector = new BWGameSelector(this)` (0xb0, ctor 0x401600); Resize(0,0,w,h); AddWidget; if mBoard: `Board 0x405cd0()` (removes the board's child widgets 0x405a80, Board +0x194 = 3), RemoveWidget(mBoard) (**not deleted**), selector slot 48 (0x4010c0, snapshot + slide-in); if !IsPlaying(0): FadeIn(0, 27, 0.005, loop). Callers: TitleScreenDone, GoToMainMenu, ButtonDepress 2000/2028, FameWidget 0x426c10. |
| 0x411020 | `ShowHallOfFame()` | RemoveFameWidget(); `mFameWidget = new FameWidget(this)` (0x2a0, ctor 0x428520); Resize(0,0,w,h); AddWidget. Callers: selector button 3, LevelUpWidget 0x42bc90. |
| 0x413a40 | `LoadUserSettings()` | name = current user; if not empty and FindUser(name): ReadUserInt "ShowHelp" -> mShowHelp = (v != 0), "ShowHints" -> mShowHints, "ShowDefs" -> mShowDefs, "TimesPlayed" -> mUserTimesPlayed; each only when the value exists. Callers: StartGameForUser, TitleScreenDone, SelectUserDialog 0x432720. |
| 0x413c80 | `StartGameForUser()` | LoadUserSettings(); StartGame(); if mShowHelp: ShowHelp(). Caller: BWGameSelector slot 47 (0x401460: mGameMode = selector +0x84 (-1 -> 0), mGameOption = selector +0x80 (-1 -> 1), this, then selector SetDisabled(true)). |
| 0x4153c0 | `SelectGameMode(mode)` | mSelectedGameMode = mode; if `HasSavedGame(currentUser, mode)`: `d = DoDialog(5, modal, `GT_CONTINUE_HEADER`, `GT_CONTINUE_LINES`, "", YES_NO)`; SetLinesFont(font 0); mTextAlign (+0xc8) = -1 (left); mContentInsets = (36, 32, 36, 36); yes label `GT_CONTINUE_YES`, no label `GT_CONTINUE_NO` (ButtonWidget::mLabel +0x5c); `h = GetPreferredHeight(w)`; Resize(((mWidth - w) + 233) / 2, (mHeight - h) / 2, w, GetPreferredHeight(w)) (w = 348). Else: selector slot 47 (starts the game). Caller: selector buttons 1 (classic, sets selector +0x84 = 0) and 2 (action, = 1). |
| 0x412190 | `HasSavedGame(user, mode)` | path = "players" + "\\" + user, + "\\" if user not empty, + "classic.dat" (mode 0) / "action.dat" (1); other modes false; `FileExists(path)` (0x47e700, fopen "rb"). |
| 0x4157e0 | `GoToMainMenu()` | (Board ButtonDepress id 18 when the game is in progress) if mBoard == NULL: if mGameSelector and selector +0x99 (bool) is false: selector slot 47, return; else fall to ShowGameSelector. If mBoard and Board +0x194 != 2: `d = DoDialog(28, modal, `GT_LEAVE_GAME_HEADER`, `GT_LEAVE_GAME_LINES`, "", YES_NO)`, labels yes "Quit!" no "Play!", Resize(((mWidth - w) + 233)/2, (mHeight - h)/2, w, h). Else ShowGameSelector(). |
| 0x415660 | `ShowQuitDialog()` | lines = `GT_QUIT_LINES` + (mBoard && IsGameInProgress ? `GT_QUIT_IN_PROGRESS` : ""); `DoDialog(13, modal, `GT_QUIT_HEADER`, lines, "", YES_NO)`; Move(290, 138). Caller: selector button 4. |
| 0x410520 | `ShowSelectUserDialog()` | `new SelectUserDialog(this)` (0x130, ctor 0x432c90); SetColor(0, white); Resize(0, 17, 400, GetPreferredHeight(400)); AddDialog(19). Caller: selector button 0. |
| 0x4108c0 | `ShowNewUserDialog(text)` | if mGameSelector: SetDisabled(true); `new NewUserDialog(this, text)` (0x130, ctor 0x42e7c0); Resize(238, 174, 400, GetPreferredHeight(400)); AddDialog(22). Callers: selector Update (no user: text `GT_SELECTOR_ENTER_NAME`, then prefills the edit box with mUserName (+0x41c) if it only has letters/spaces), SelectUserDialog 0x432f00. |
| 0x418590 | `ShowDeleteUserDialog(names)` (vector by value) | `new DeleteUserDialog(this, names)` (0x130, ctor 0x41e370); SetColor(0, white); Resize(0, 17, 400, GetPreferredHeight(400)); AddDialog(20); destroys the argument. Caller: SelectUserDialog 0x432f00. |
| 0x4105d0 | `ShowHint(hintId, header, lines, footer, mode)` | if `!HintDialog::WasShown(hintId)` (0x42b3e0, static map<int,bool>) && mShowHints: KillDialog(15); `new HintDialog(this, hintId, header, lines, footer, mode)` (0x120, ctor 0x42af50); Resize(0, 0, mWidth, mHeight); Resize(233, 110, 400, GetPreferredHeight(400)); AddDialog(15). Caller: Board::Update (hint id = Board +0x34c; texts `GT_HINT_HOW_TO_PLAY_HEADER`, `GT_HINT_WATCH_OUT_HEADER`, `GT_HINT_BONUS_TILE_HEADER`, ...). |
| 0x4106c0 | `ShowCredits()` | `new CreditsDialog(this)` (0x138, ctor 0x41c0b0); SetColor(0, white), (1, white), (3, black); Resize(0, 47, 388, GetPreferredHeight(388)); Move(((mWidth - w) + 233)/2, mY); AddDialog(16). Caller: Options 0x42fbe0. |
| 0x4107c0 | `ShowOptions()` | `new OptionsDialog(this)` (0x188, ctor 0x42fe90); SetColor(0, white), (1, white), (3, black); h = GetPreferredHeight(388); Resize(mWidth, (mHeight - h)/2, 388, h) (starts off-screen right); dialog +0x170 = 240 (slide target, dialogs group); AddDialog(1). Caller: Board button 17. |
| 0x4110b0 | `KillDialog(id)` (slot 58) | SexyApp::KillDialog(id); if it existed and the dialog map is now empty (+0x2f0) and mBoard: SetFocus(mBoard); returns whether it existed. |
| 0x410ad0 / 0x410af0 | `ModalOpen()` / `ModalClose()` (slots 60/61) | if mBoard: `Board::Pause(true)` / `Board::Pause(false)`. |
| 0x410b30 | `ButtonPress(id)` (slot 3) | `PlaySample(SOUND_BUTTONCLICK)` for every forwarded press (dialog buttons). |
| 0x410090 | `DrawShadowedString(g, x, y, s, color, shadow, shadowColor)` | cdecl. If shadow: (dead Color::GetAlpha call), `g->SetColor(shadowColor)`, `DrawString(s, x+1, y+1)`; `g->SetColor(color)`; `DrawString(s, x, y)`. FameWidget 0x424c30/0x425880. |
| 0x412da0 | `TruncateToWidth(g, s, maxWidth, ellipsis)` | cdecl. `cur = s`, n = s.size(); while `g->GetFont()->StringWidth(cur) > maxWidth && n > 0`: `cur = s.substr(0, --n)`; if n < s.size(): `cur += ellipsis`. FameWidget 0x424c30. |
| 0x4122e0 | `TrimString(s, chars)` | cdecl. Erase after `find_last_not_of(chars)+1` (or pad with NUL if longer: unreachable), then erase `[0, find_first_not_of(chars))`. ButtonDepress 2022/2023 with " \t\r\n". |
| 0x411cd0 | `FormatTime(seconds)` | "" when mBoard is NULL; else d = s/86400, h = (s/3600)%24, m = (s/60)%60, sec = s%60: `[%d: if d>0][%02d: if h>0 or d>0]%02d:%02d`. |
| 0x411ea0 / 0x411f50 | `GetLevelTimeString(prefix)` / `GetTotalTimeString(prefix)` | prefix alone without a board, else prefix + FormatTime(Board +0x188 / +0x18c). LevelUpWidget 0x42c210 (`GT_SUMMARY_TIME`, `GT_SUMMARY_TOTAL`). |
| 0x4152a0 | `IsInternetScoresEnabled()` | `!GetBoolean("NoInternetScores", true) && (!GetBoolean("NoIHSUpsell", true) || mIsRegistered)`; false in the retail build. Callers: FameWidget ctor, HelpWidget 0x42a170, TitleScreen::Draw 0x433900. |
| 0x412100 | `SaveCurrentUser()` | if mUserManager: name = current; if not empty and FindUser: SaveUser(user), return true; else false. Caller: OptionsDialog 0x42ed00. |

### 6. Focus and pause

- **GotFocus 0x419000** (slot 62, called by SexyApp::RehupFocus after `Unmute(true)`): (base empty) if mBoard:
  `Board::Pause(false)`; if the board's pause count (+0x3c8) is now 0: `PauseAllMusic(false)`;
  `mLastInputUpdateCnt = mUpdateCount` (+0x4b8).
- **LostFocus 0x419040** (slot 63, after `Mute(true)`): if mBoard: if pause count == 0: `PauseAllMusic(true)`;
  `Board::Pause(true)`.
- **ModalOpen/ModalClose**: Board::Pause(true/false) (counted: Board +0x3c8 never below 0; the board stops
  its clock while > 0; Board group).
- The framework mutes all sound while the window is inactive (docs/FRAMEWORK_2004.md 1.5).

### 7. Dialogs

#### 7.1 NewDialog 0x410970 (slot 20) and DoDialog 0x4103e0 (slot 55)

- `NewDialog(id, modal, header, lines, footer, mode)`: `new Dialog(mImages[7] dialog, mImages[8]
  dialogbutton, id, modal, header, lines, footer, mode)` (0x110); SetButtonFont, SetHeaderFont, SetLinesFont
  all font 0 (Verdana10Bold); SetColor(0 header, white), (1 lines, (0,255,0)), (3 button text, (0,255,0));
  mContentInsets (+0xe0) = Insets(36, 36, 36, 36); Resize(206, 42, 348, GetPreferredHeight(348)).
- `DoDialog(...)`: `SexyApp::DoDialog` (KillDialog(id) [the override], NewDialog, AddDialog); if a dialog came
  back: SetHeaderFont(font 3 Verdana18BoldColor), SetLinesFont(font 1 Verdana12Bold), SetButtonFont(font 1);
  SetColor(0, white), (1, white), (3, black); yes button (+0x60) and no button (+0x64), if present,
  SetColor(1, white) (label hilite); Resize(mX, mY, mWidth, GetPreferredHeight(mWidth)). Every game
  `DoDialog` call goes through this (dialogs made with NewDialog directly, 11 and 12, keep the green
  Verdana10 look).
- Button modes: 0 none, 1 YES/NO, 2 OK/CANCEL, 3 footer button (one button `mId+2000` labelled with the
  footer). Labels can be replaced through the button's `mLabel` (+0x5c).

#### 7.2 Dialog ids

| Id | Created by | Header / lines / footer (mode) | Layout | YES (2000+id) | NO (3000+id) |
|---|---|---|---|---|---|
| 0 | nobody | - | - | KillDialog(0), ShowGameSelector | KillDialog(0) |
| 1 | ShowOptions | OptionsDialog | ShowOptions (5) | dialog slot 52 (close) | slot 52 |
| 2 | ShowRegisterDialog (not ported) | RegisterDialog, image 2 size, centred | | validate name/code (below) | KillDialog(2) |
| 3 | nobody | - | | KillDialog(3) | KillDialog(3) |
| 4 | TitleScreen 0x4336f0 (trial over, not ported) | "PLEASE REGISTER!" | | ShowRegisterDialog | Shutdown() |
| 5 | SelectGameMode | `GT_CONTINUE_HEADER` (YES_NO `GT_CONTINUE_YES`/`GT_CONTINUE_NO`) | SelectGameMode (5) | KillDialog(5); selector slot 47 (new Board); `Board::LoadGame(mSelectedGameMode)` (0x408ab0); `Board::DeleteSavedGame(mGameMode)` (0x4078b0) | KillDialog(5); selector slot 47; DeleteSavedGame(mGameMode); Board +0x194 = 0 |
| 6 | ConfirmInternetConnect (not ported) | "INTERNET REQUIRED" (YES_NO "Connect"/"Cancel"), align 0, insets (34,24,34,32), Resize(0,0,400,h), Move((mWidth-w)/2, 132) | | KillDialog(6); mInternetOK = true; FameWidget 0x4241d0 | KillDialog(6) |
| 7 | AskForUpdateCheck (not ported) | "UPDATES" (YES_NO), align -1, insets (36,32,36,32), Resize(165, 99, 330, h) | | KillDialog(7); DoUpdateCheck | KillDialog(7) |
| 8 | DoUpdateCheck (not ported) | UpdateCheckDialog(images 7, 8, 147 waitbar), header "UPDATE CHECK", fonts 3/0/0, Resize(146, 50, 420, prefH(348)) | | KillDialog(8) | KillDialog(8) |
| 9 | ShowInvalidCodeDialog (not ported) | "Invalid Code" / a "license code not valid for that name" message / `GT_DIALOG_OK` (footer); PlaySample(2) | | KillDialog(9) | |
| 11 | OpenURL | NewDialog(11, modal, "", `GT_OPENING_BROWSER`, "", none); insets 36; Resize(164, 140, 312, 98) | | | |
| 12 | URLOpenFailed | NewDialog(12, modal, `GT_OPEN_BROWSER_HEADER`, `GT_OPEN_BROWSER_LINES` + url, `GT_DIALOG_OK`, footer); Resize(100, 50, 440, prefH(440)); AddDialog | | KillDialog(12) | |
| 13 | ShowQuitDialog | `GT_QUIT_HEADER` (YES_NO) | Move(290, 138) | KillDialog(13); Shutdown() | KillDialog(13) |
| 15 | ShowHint; Board 0x40ed70 DoDialog `GT_CANT_SCRAMBLE` (footer) | HintDialog | | KillDialog(15) | KillDialog(15) |
| 16 | ShowCredits | CreditsDialog | | KillDialog(16) | KillDialog(16) |
| 17 | Board 0x40f5b0 "Dictionary Upgrade!", FameWidget 0x426c10 "SORRY!" (YES_NO, not ported) | | | KillDialog(17); ShowRegisterDialog | KillDialog(17) |
| 18 | messages (footer `GT_DIALOG_OK`): `GT_ENTER_YOUR_NAME`, `GT_NAME_CONFLICT_HEADER`, "Already Registered", "THANK YOU!", Options 0x42fa50 `GT_OPTIONS_3D_WARNING_HEADER`/`GT_OPTIONS_3D_NOT_SUPPORTED_HEADER` | | | KillDialog(18) | |
| 19 | ShowSelectUserDialog | SelectUserDialog | | KillDialog(19) | KillDialog(19) |
| 20 | ShowDeleteUserDialog | DeleteUserDialog | | KillDialog(21) (the dialog then opens 21 itself) | KillDialog(20) |
| 21 | DeleteUserDialog 0x41e020 | `GT_ARE_YOU_SURE` / `GT_CONFIRM_DELETE_FORMAT` (YES_NO) | | KillDialog(21); `if (d = GetDialog(20)) DeleteUserDialog::DeleteSelected(d)` (0x41e620); KillDialog(20) | KillDialog(21) |
| 22 | ShowNewUserDialog | NewUserDialog | | new user (7.3) | 7.3 |
| 23 | ShowAliasDialog (not ported) | GetStringDialog "YOUR NAME?" | | set alias (7.3) | KillDialog(23) |
| 24 | FameWidget 0x427f30 (not ported) | "Resubmit Detected" ... | | KillDialog(24); ShowAliasDialog(GetCurrentAlias()) | KillDialog(24) |
| 25 | FameWidget 0x423fd0 "CONNECTING" (not ported) | | | FameWidget +0x74 = 0; KillDialog(25) | KillDialog(25) |
| 26, 27 | FameWidget "ERROR!", "WARNING!" (not ported) | | | KillDialog | KillDialog |
| 28 | GoToMainMenu | `GT_LEAVE_GAME_HEADER` (YES_NO `GT_LEAVE_GAME_YES`/`GT_LEAVE_GAME_NO`) | | KillDialog(28); if mBoard && IsGameInProgress: `Board::SaveGame(mGameMode)`; ShowGameSelector() | KillDialog(28) |
| 10008 | UpdateCheckDialog (framework, not ported) | update available | | KillDialog(10008); KillDialog(8); `OpenURL(mInternetManager->GetUpdateURL(), true)` | KillDialog(10008); KillDialog(8) |
| 20008 | UpdateCheckDialog | up to date / failed | | KillDialog(20008); KillDialog(8) | KillDialog(20008) |

10 and 14 are never used.

#### 7.3 ButtonDepress 0x417080 (slot 4)

Called for every forwarded dialog button (`mId+2000` / `mId+3000`) and for app-listened buttons.
1. `id == 26` (HintDialog's `GT_HINT_DISABLE` button): KillDialog(15); mShowHints = false.
   (HintDialog handles id 26 itself because its listener is the dialog, so this branch is not reached from
   it; keep it.)
2. `r = id % 10000`. If `2000 <= r < 3000`: `x = id - 2000` (the full id, so 12008 -> 10008), YES of
   dialog x per the table. If `3000 <= r < 4000`: `x = id - 3000`, NO of dialog x. Otherwise nothing.
3. Unlisted ids: `KillDialog(x)`.

YES/NO details not in the table:
- **2022 (new user OK)**: `name = NewUserDialog edit (+0x128) mString`, `TrimString(name, " \t\r\n")`. Empty:
  `DoDialog(18, modal, `GT_ENTER_YOUR_NAME`, `GT_ENTER_NAME_LINES`, "OK", footer)` (22 stays). `FindUser(name)` exists: `DoDialog(18, modal, `GT_NAME_CONFLICT_HEADER`, `GT_NAME_CONFLICT_LINES`, "OK", footer)`. Else:
  KillDialog(22); selector SetDisabled(false); `u = new User(name)`; mUserTimesPlayed = 0; SaveUser(u) (with
  the current mShowHelp/mShowHints/mShowDefs, i.e. inherited from the previous player); push_back(u);
  SetCurrentUser(name); KillDialog(19).
- **3022 (new user cancel)**: if the current user name is "": the same `GT_ENTER_YOUR_NAME` message (a user is
  mandatory). Else KillDialog(22); if dialog 19 exists, Resize(its mX, mY, 400, GetPreferredHeight(400));
  selector SetDisabled(false).
- **2023 (alias OK, not ported)**: if mUserManager and dialog 23: alias = edit (+0x118) text trimmed; if not
  empty: SetCurrentAlias(alias) and, if mFameWidget, ConfirmInternetConnect(). KillDialog(23).
- **2002 (register OK, not ported)**: name/code from RegisterDialog edits (+0x118, +0x11c); either empty:
  the dialog's ButtonListener slot 4 is called with 0 (odd, not traced); `Validate(name, code)` (slot 23)
  false: ShowInvalidCodeDialog; true: mUserName = name, mRegCode = code, mIsRegistered = true,
  KillDialog(2), KillDialog(4), MarkAllDirty, FameWidget ButtonDepress(8), "THANK YOU!" message (two texts
  by NoInternetScores) with header font 3, insets (36,32,36,32), Resize(160, 100, w, h), PlaySample(26).
- **2001/3001**: `GetDialog(1)->slot52()` (OptionsDialog's close/slide, dialogs group).

### 8. Registry, properties and files

#### 8.1 Registry (HKLM\SOFTWARE\PopCap\BookWorm; mRegKey may be overridden by the property RegistryKey)

| Value | Type | Writer / reader | Meaning |
|---|---|---|---|
| `test` | DWORD 1 | Init (CheckRegistryAccess) | write-access probe |
| `Users\<name>\Alias` | REG_SZ (256 bytes) | SaveUser / LoadUser | internet alias |
| `Users\<name>\ShowHelp`, `ShowHints`, `ShowDefs` | DWORD 0/1 | SaveUser / LoadUserSettings | options |
| `Users\<name>\TimesPlayed` | DWORD | SaveUser / LoadUserSettings | mUserTimesPlayed (Board increments) |
| `Users\User` | REG_SZ (256 bytes) | SetCurrentUser / GetLastUser | current player |
| `Scores\Classic`, `Scores\Action` | REG_MULTI_SZ | SaveHighScores / LoadHighScores | `name\0score\0` per entry, then `\0` |
| `Scores\Best` | REG_MULTI_SZ | same | `name\0word\0score\0` per entry, then `\0` |
| `Scores\Longest` | REG_MULTI_SZ | same | `name\0word\0` per entry, then `\0` |
| SexyApp's values | | docs/FRAMEWORK_2004.md 1.7 | MusicVolume, SfxVolume, Muted, ScreenMode, CustomCursors, TimesPlayed (minutes, see 8.4), ... |

**WriteToRegistry 0x412000** (slot 18): `SexyApp::WriteToRegistry()`; if mUserManager: name = current; if
FindUser(name): SaveUser(user). (High scores are written when they change, not here.)

#### 8.2 Properties (partner.xml and defaults)

Read here: `MaxExecutions` (int, 0), `MaxPlays` (int, 0), `MaxTime` (int, 60), `ImagePath` ("images\\"),
`FontPath` ("fonts\\"), `SoundPath` ("sounds\\"), `DontUpdate` (bool, true), `NoInternetScores` (bool, true),
`NoIHSUpsell` (bool, true). Elsewhere: LoadingBarY (TitleScreen 0x434380), OptionsDialogHeight /
NumOptionsLinks / OptionsLinkText<n> / OptionsLinkURL<n> (OptionsDialog), ShowTrialStatus (TitleScreen),
GetScoresURL / SubmitScoreURL (FameWidget, not ported); the framework's NoReg, DontUpdate, ReferId, ProdName,
Title, DefaultWindowed, RegistryKey.

#### 8.3 Saved games (`players\<user>\classic.dat` / `action.dat`)

Path: "players" (a per-object `static std::string` initialised from the literal at 0x4f9588; app copy
0x52e818, Board copy 0x52e2b4) + "\\" + current user name, + "\\" only if the name is not empty, +
"classic.dat" (mode 0) or "action.dat" (mode 1); other modes: no file. Relative to the exe folder. Saving
`_mkdir("players")` and `_mkdir("players\<user>")` (errno EEXIST accepted) first. Written by
`Board::SaveGame(mode)` 0x406c40 (called by ShutdownWidgets and ButtonDepress 2028), read by
`Board::LoadGame(mode)` 0x408ab0 (2005), deleted with DeleteFile by 0x4078b0 (2005 and 3005): a saved game is
used once. `HasSavedGame` 0x412190 (section 5) only tests that the file opens.

Byte layout (little endian, `fwrite` of raw fields; "str" = int32 length + bytes, no NUL):

| # | Bytes | Content |
|---|---|---|
| 1 | int32 | version = **5** |
| 2 | str | app mProductVersion (+0x3b8) (v >= 5) |
| 3 | int32 | app mBuildNum (+0x94) = 190 (v >= 5) |
| 4 | str | app mBuildDate (+0x98) "Thu Apr 15 14:52:08 2004" (v >= 5) |
| 5 | int32 | game mode (the SaveGame argument); LoadGame stores it in **app mGameMode** |
| 6 | uint8 | app mShowHints (+0x12e8); LoadGame restores it into the app |
| 7 | str | `MTRand::Serialize()` of Board +0x6c (0x9c0 = 2496 bytes, mt[624] without mti); LoadGame: SRand(string) |
| 8 | 56 x (uint8 [+ 191 bytes]) | the 8 x 7 board cells (Board +0x78 + 4*(col*7+row)): present flag, then the Piece record (Piece::Save 0x403050 / Load 0x403370, 191 bytes) |
| 9 | int32 x3 | Board +0x188 (level seconds; read only if v >= 3), +0x18c, +0x190 |
| 10 | ... | Board +0x158 widget record (0x430ab0 / 0x430b20), EffectMgr +0x70 (0x41faa0 / 0x420b70), EmberListMgr +0x74 (0x41ff80 / 0x420e70) |
| 11 | ... | Board +0x194, +0x198; selected-tile list (count, then col,row int32 pairs); the remaining Board fields in the order of 0x406c40 (+0x1a8 .. +0x3ec, two StatKeeper records 0x404e10, two WordScore records 0x404fe0, strings as str, optional piece references as uint8 + col,row, the Lex record through Lex slot 47/48) |
| 12 | str | Board game log (Board +0x160 Buffer) as `ToWebString()`; LoadGame: Clear + FromWebString when length > 0 |
| 13 | uint8 | "help was open with ShowHelp off" (mShowHelp == false && mHelpWidget != NULL at save); LoadGame (v >= 4): ShowHelp() if mShowHelp or this byte |

Version handling in LoadGame: v <= 1: closed, not loaded (returns false). v 2..4:
`OutputDebugString(a debug line about converting an obsolete saved game)`, rows 2-4 absent; v 2 also
lacks the +0x188 field (row 9) and v 2..3 the final byte. No other conversion. LoadGame also overrides a few
fields after reading (Board +0x378 = 0, +0x37c = 0, +0x384 = 3) and restarts the danger music (section 4).
The board group owns rows 8-12; the port must keep every byte so original saves load.

#### 8.4 Trial counters (not ported, but they touch the registry)

`Shutdown`: if mMaxTime > 0 (default 60!): `mTimesPlayed (+0x52c) += mPlayTicks / 6000` (minutes in play);
else if mMaxPlays > 0: `mTimesPlayed += mPlaysToCount`. SexyApp then writes TimesPlayed. The registered port
keeps the arithmetic (harmless) or drops it; the framework already drops the TimesPlayed = 100 rule.

### 9. High scores

#### 9.1 SubmitHighScores(stats) 0x419cd0 (Board::Update at game over)

`e = { mUserManager->GetCurrentUserName(), "", stats +0x8 (score), mIsNew = true }`:
1. mode 1: insert into mActionScores; if size > 10, walk in order and erase every entry after the 10th.
   mode 0: the same with mClassicScores. Then if `find(e) != end` -> Board +0x357 = true ("made the list").
   Other modes skip to 2.
2. `e.mWord = stats +0x38`, `e.mScore = stats +0x54`: insert into mBestWords; if size > 3: prune to 3 and
   `find(e) != end` -> Board +0x357 = true.
3. `e.mWord = stats +0x58`, `e.mScore = stats +0x74`: insert into mLongestWords; if size > 3: prune to 3 and
   find -> Board +0x357 = true.
4. SaveHighScores().

Quirk: `find` uses equivalence (same score and same word), so tying the 10th classic score with an existing
entry sets the flag even though the new entry was pruned.

#### 9.2 LoadHighScores 0x419080, AddDefaultHighScores 0x418bd0

Open `Scores` (read); for each of Classic, Action, Best, Longest: query the size, accept only type 7
(REG_MULTI_SZ), read, split at NULs into a list of strings (stops at an empty string), then consume:
Classic/Action: name, `atoi(score)`; Best: name, word, `atoi(score)`; Longest: name, word, score =
**word.size()**. Each entry (mIsNew false) is inserted (no size limit on load). Quirk: the loop advances
twice per entry with `node = node->next` and only tests for the end after the second step, so a value whose
string count is not a multiple of 2 (3 for Best) wraps from the list head to the first node and loops for
ever; a missing score/word is otherwise left empty/0. Port: stop at the end. Then AddDefaultHighScores: for each default row, `if (table.size() < N && table.find(row) ==
end) table.insert(row)`:

| Table | N | Defaults (name, word, score) (table address) |
|---|---|---|
| Classic | 10 | `GT_HS_NAME_WORDPIG` 5000, `GT_HS_NAME_SENHEN` 4500, `GT_HS_NAME_ORCUS` 4000, `GT_HS_NAME_GRAVYTRAIN` 3500, `GT_HS_NAME_TUCO` 3000, `GT_HS_NAME_BRENNXOR` 2500, `GT_HS_NAME_ALLY` 2000, `GT_HS_NAME_ZELDA` 1500, `GT_HS_NAME_GREENMAN` 1000, `GT_HS_NAME_DELIVERATOR` 42 (0x4fa4e8) |
| Action | 10 | `GT_HS_NAME_DANIELLEE` 5000, `GT_HS_NAME_DANSMOM` 4500, `GT_HS_NAME_ARCHITEKT` 4000, `GT_HS_NAME_JAN` 3500, `GT_HS_NAME_PAMELA` 3000, `GT_HS_NAME_ALIEN` 2500, `GT_HS_NAME_ZAK` 2000, `GT_HS_NAME_WORDWEASEL` 1500, `GT_HS_NAME_CRAZYHORSE` 1000, `GT_HS_NAME_MRILLITERATE` 500 (0x4fa588) |
| Longest | 3 | `GT_HS_NAME_WORDPIG` `GT_HS_WORD_DESIGN` 0, `GT_HS_NAME_DELIVERATOR` `GT_HS_WORD_CODE` 0, `GT_HS_NAME_SENHEN` `GT_HS_WORD_ART` 0 (0x4fa628) |
| Best | 3 | `GT_HS_NAME_GRAVYTRAIN` `GT_HS_WORD_PLAY` 300, `GT_HS_NAME_ORCUS` `GT_HS_WORD_POPCAP` 200, `GT_HS_NAME_WORDPIG` `GT_HS_WORD_GAMES` 100 (0x4fa658) |

Rows are 16 bytes `{char* name, char* word, int score, int 0}`.

#### 9.3 SaveHighScores 0x412ec0

Open `Scores` for writing, else create it (fail: return). For Classic, Action, Best: size = sum over entries of
the strings' lengths + 1 each (score text `StrFormat("%d", score)`), skip the value if 0, else write
REG_MULTI_SZ of size+1 bytes: the strings in set order, each NUL-terminated, then one more NUL. Longest: name
and word only (the score is not saved; reload uses the word length). Close.

### 10. Cross-module references

Fields of other classes this module touches:
- Board (`mBoard`): +0x15c LevelUpWidget* (SetDisabled/SetVisible), +0x188 / +0x18c level/total seconds,
  +0x194 game state (2 = over, 3 = suspended in menu/continue), +0x34c hint id, +0x357 "made the high-score
  list", +0x3c0 bool (level-up widget active), +0x3c8 pause count, +0x6c MTRand*.
  Calls: ctor 0x40ae90(app), Pause(bool) 0x405be0, IsGameInProgress 0x403720, SaveGame(mode) 0x406c40,
  LoadGame(mode) 0x408ab0, DeleteSavedGame(mode) 0x4078b0, 0x405cd0 (remove children, state 3), 0x403d80(bool),
  0x408030 (pick bonus word). Board calls into the app: ShowHint 0x4105d0, SubmitHighScores 0x419cd0,
  ShowOptions 0x4107c0, GoToMainMenu 0x4157e0, PauseAllMusic 0x4184d0, ShowHelp/HideHelp, RemoveBoard; reads
  mGameMode, mDictionary, mBonusWords, mUsedBonusWords, mBonusWord, mImages, fonts, mTileSetImages,
  mShowHelp/Hints/Defs, mUserTimesPlayed, mLastInputUpdateCnt, mPlayTicks, mLevelReached, mMaxPlays,
  mMaxTime, mScoringMethod, mDefinitionator, hint flags; gGameRand.
- BWGameSelector: +0x80 (mGameOption copy), +0x84 selected mode, +0x99 bool (transition running);
  slots 47 (start game), 48 (show / snapshot), 49 (hide); ctor 0x401600(app). It calls RemoveBoard,
  RemoveGameSelector, ShowSelectUserDialog, SelectGameMode, ShowHallOfFame, ShowQuitDialog,
  ShowNewUserDialog, StartGameForUser; reads mUserManager (+0x0c count), mUserName (+0x41c).
- TitleScreen: ctor 0x434380(app); calls TitleScreenDone (slot 67), ShowRegisterDialog; reads mInitDone,
  mMaxTime, images 0-6.
- FameWidget: ctor 0x428520(app); reads the four score sets, mUserManager; calls RemoveFameWidget,
  ShowGameSelector, ShowAliasDialog, ConfirmInternetConnect, IsInternetScoresEnabled, TruncateToWidth,
  DrawShadowedString; +0x74 (wait flag), 0x4241d0 (internet fetch/submit).
- HelpWidget ctor 0x429cd0(app); CreditsDialog 0x41c0b0(app); OptionsDialog 0x42fe90(app) (+0x170 slide
  target; slot 52 close); SelectUserDialog 0x432c90(app); NewUserDialog 0x42e7c0(app, text) (edit at +0x128);
  DeleteUserDialog 0x41e370(app, names), DeleteSelected 0x41e620; HintDialog 0x42af50(app, hintId, header,
  lines, footer, mode), WasShown 0x42b3e0; GetStringDialog 0x4292d0(app, id, modal, compImg, btnImg, header,
  lines, footer, headerFont, linesFont, editFont, label, labelFont, editImage, mode) (edit +0x118, label
  +0x11c, +0x124 extra image); Definitionator 0x41dd30(app), Load 0x41d920; LevelUpWidget uses
  GetLevelTimeString/GetTotalTimeString and mRankStripImage; Effect uses mTileSetImages, the sapphire and
  diamond copies; OptionsDialog uses SaveCurrentUser, ShowCredits, ShowHelp, mCheckbox images,
  mShowHints/mShowDefs.
- Framework: SexyApp fields listed in 1.1, mUserName +0x41c, mRegCode +0x454, mIsRegistered +0x470,
  mLoadingFailed +0x528, mNumLoadingThreadTasks +0x538, mCompletedLoadingThreadTasks +0x53c, mShutdown +0x305,
  mIsWindowed +0x306, mUpdateCount +0x4b8, mTimesPlayed +0x52c, mMuteCount +0x488, mDialogMap size +0x2f0,
  mInternetManager +0xd8, mDDInterface +0x334, mSoundManager +0x4e0, mMusicInterface +0x33c.

### 11. Shutdown and destruction

- **Shutdown 0x410ca0** (slot 33): return if mShutdown; ShutdownWidgets() (saves a game in progress); trial
  counters (8.4); SexyApp::Shutdown() (StopAllMusic, WriteToRegistry if read, ...).
- **~BookWormApp 0x4187b0**: if mUserManager: ~UserManager + delete; delete the 19 fonts, the 13 pointers
  +0x1204..+0x1234, +0x11fc, +0x1200, every image in mCopiedImages, every mImages[i] (149) then `delete[]
  mImages`; mDefinitionator (its slot 1, deleting dtor); then the containers and strings in reverse order,
  ~Math, ~SexyApp. The widgets (+0x1160..+0x1170) are not deleted here (ShutdownWidgets did it).
- **GetGameSEHInfo 0x416090** (slot 32, not ported): base text + "\r\n" + mSEHInfo + "\r\n".
- **URLOpenSucceeded 0x410b10** (slot 25): base (mIsOpeningURL = false, shutdown if requested);
  KillDialog(11). **URLOpenFailed 0x414fa0** (slot 24): base; KillDialog(11); dialog 12. **OpenURL 0x415100**
  (slot 26): dialog 11, then SexyApp::OpenURL(url, shutdownOnOpen). Port: keep the dialogs; the framework
  never opens URLs in test runs.

### 12. What is not ported (one line each)

- `CheckRegistryAccess` 0x414910: admin-rights MessageBox; the emulated registry cannot fail.
- `ShowRegisterDialog` 0x413cb0, `ShowInvalidCodeDialog` 0x413f30, ButtonDepress 2002/2004/2017, dialog 4:
  registration (the registered build has mIsRegistered = true; "Already Registered" would show if reached).
- `IsInternetScoresEnabled` 0x4152a0 (returns false), `ShowAliasDialog` 0x414080, `ConfirmInternetConnect`
  0x415a20, ButtonDepress 2006/2023/2024/2025, dialogs 6, 23-27: internet high scores.
- `LoadingThreadCompleted` 0x4169d0, `AskForUpdateCheck` 0x415c40, `DoUpdateCheck` 0x415de0, dialogs 7, 8,
  10008, 20008: update check (DontUpdate defaults to true).
- `GetGameSEHInfo` 0x416090: crash report.
- Trial: mMaxExecutions/mMaxPlays/mMaxTime and the Shutdown TimesPlayed arithmetic (keep or drop; harmless).
- `OptimizeImage`'s DirectDraw surface branch (keep Palletize).

### 13. Function list (BookWormApp.cpp, Math.cpp and the neighbours)

Index of the game functions (behaviour in the section named):

| Address | Function | Slot | Section |
|---|---|---|---|
| 0x40fe80 | `WinMain` | | 3.1 |
| 0x40ffa0 | `BookWormApp::ShutdownWidgets()` | | 5, 11 |
| 0x410090 | `DrawShadowedString(g, x, y, s, color, shadow, shadowColor)` (cdecl) | | 5 |
| 0x4100f0 | `BookWormApp::UpdateFrames()` | 7 | 4 |
| 0x410110 | `BookWormApp::SetMusicVolume(double)` | 47 | 4 |
| 0x410180 | `RemoveBoard()` | | 5 |
| 0x4101c0 | `ShowHelp()` | | 5 |
| 0x4102d0 | `HideHelp()` | | 5 |
| 0x410360 | `RemoveGameSelector()` | | 5 |
| 0x4103a0 | `RemoveFameWidget()` | | 5 |
| 0x4103e0 | `DoDialog(id, modal, header, lines, footer, mode)` | 55 | 7.1 |
| 0x410520 | `ShowSelectUserDialog()` | | 5 |
| 0x4105d0 | `ShowHint(hintId, header, lines, footer, mode)` | | 5 |
| 0x4106c0 | `ShowCredits()` | | 5 |
| 0x4107c0 | `ShowOptions()` | | 5 |
| 0x4108c0 | `ShowNewUserDialog(const string&)` | | 5 |
| 0x410970 | `NewDialog(id, modal, header, lines, footer, mode)` | 20 | 7.1 |
| 0x410ad0 / 0x410af0 | `ModalOpen()` / `ModalClose()` | 60 / 61 | 6 |
| 0x410b10 | `URLOpenSucceeded(const string&)` | 25 | 11 |
| 0x410b30 | `ButtonPress(int)` | 3 | 5 |
| 0x410ca0 | `Shutdown()` | 33 | 11 |
| 0x410d10 | `StartGame()` | | 5, 4 |
| 0x410f00 | `ShowGameSelector()` | | 5, 4 |
| 0x411020 | `ShowHallOfFame()` | | 5 |
| 0x4110b0 | `KillDialog(int)` | 58 | 5 |
| 0x411220 | `OptimizeImage(img /*EDI*/, name /*EAX*/)` | | 2.1 |
| 0x4112f0 | `UpperCaseString(string&)` (cdecl) | | 3.6 |
| 0x411570 / 0x4115d0 | `HighScoreByWordLength` / `HighScoreByScore` predicates | | 1.4 |
| 0x411cd0 | `FormatTime(int)` | | 5 |
| 0x411ea0 / 0x411f50 | `GetLevelTimeString(prefix)` / `GetTotalTimeString(prefix)` | | 5 |
| 0x412000 | `WriteToRegistry()` | 18 | 8.1 |
| 0x412080 | `HighScoreEntry(const HighScoreEntry&)` | | 1.4 |
| 0x412100 | `SaveCurrentUser()` | | 5 |
| 0x412190 | `HasSavedGame(const string& user, int mode)` | | 5, 8.3 |
| 0x4122e0 | `TrimString(string&, const string& chars)` (cdecl) | | 5 |
| 0x412da0 | `TruncateToWidth(g, s, maxWidth, ellipsis)` (cdecl) | | 5 |
| 0x412ec0 | `SaveHighScores()` | | 9.3 |
| 0x413a40 | `LoadUserSettings()` | | 5 |
| 0x413c80 | `StartGameForUser()` | | 5 |
| 0x413cb0 | `ShowRegisterDialog()` (not ported) | | 7.2, 12 |
| 0x413f30 | `ShowInvalidCodeDialog()` (not ported) | | 7.2 |
| 0x414080 | `ShowAliasDialog(const string&)` (not ported): GetStringDialog(app, 23, modal, img 7, img 8, "YOUR NAME?", a request to choose an Internet High Scores name, "", font 3, font 1, font 1, <an alias field label>, font 0, img 12 editbox, OK_CANCEL); colours 5..19 from 0x52e728; button font 1; insets 38; +0x124 = img 49; edit mMaxChars 22, mMaxPixels 200; text = alias or GetCurrentAlias(); centred 400 wide; AddDialog(23) | | 12 |
| 0x4144b0 | `LoadImage(int)` | | 2.1 |
| 0x4146d0 | `LoadFont(int)` | | 2.1 |
| 0x414910 | `CheckRegistryAccess()` | | 3.4 |
| 0x414a10 | `Init()` | 16 | 3.4 |
| 0x414ef0 | `TitleScreenDone()` | 67 | 3.8 |
| 0x414fa0 | `URLOpenFailed(const string&)` | 24 | 11, 7.2 |
| 0x415100 | `OpenURL(const string&, bool)` | 26 | 11, 7.2 |
| 0x4152a0 | `IsInternetScoresEnabled()` | | 5 |
| 0x4153c0 | `SelectGameMode(int)` | | 5 |
| 0x415660 | `ShowQuitDialog()` | | 5 |
| 0x4157e0 | `GoToMainMenu()` | | 5 |
| 0x415a20 | `ConfirmInternetConnect()` (not ported): `if (!mInternetOK)` dialog 6, else FameWidget 0x4241d0 | | 7.2 |
| 0x415c40 | `AskForUpdateCheck()` (not ported): UpdateCheckQueried(); dialog 7 | | 7.2 |
| 0x415de0 | `DoUpdateCheck()` (not ported): KillDialog(8); dialog 8; InternetManager::DoUpdateCheck("http://www.popcap.com/win32updatecheck.php?prod=" + mProdName + "&ver=" + mProductVersion + "&referid=" + mReferId) | | 7.2 |
| 0x416090 | `GetGameSEHInfo()` | 32 | 11 |
| 0x4169d0 | `LoadingThreadCompleted()` | 13 | 3.7 |
| 0x417080 | `ButtonDepress(int)` | 4 | 7.3 |
| 0x4184d0 | `PauseAllMusic(bool)` | | 4 |
| 0x418590 | `ShowDeleteUserDialog(vector<string>)` | | 5 |
| 0x4187b0 / 0x41a510 | `~BookWormApp` body / scalar deleting dtor | 14 | 11 |
| 0x418bd0 | `AddDefaultHighScores()` | | 9.2 |
| 0x419000 / 0x419040 | `GotFocus()` / `LostFocus()` | 62 / 63 | 6 |
| 0x419080 | `LoadHighScores()` | | 9.2 |
| 0x419cd0 | `SubmitHighScores(StatKeeper*)` | | 9.1 |
| 0x41a0f0 | `BookWormApp()` | | 3.2 |
| 0x41a540 | `ReadFromRegistry()` | 19 | 3.3 |
| 0x41a5b0 / 0x41a860 / 0x41a880 | `LoadWordList` / `LoadDictionary` / `LoadBonusWords` | | 3.6 |
| 0x41a790 | catch-all handler of LoadWordList (console + exit(1)) | | 3.6 |
| 0x41a8a0 | `LoadingThreadProc()` | 17 | 3.5 |
| 0x42e330 / 0x42e390 / 0x42e3a0 | `Math()` / `~Math` body / scalar deleting dtor | 0 | 1.2 |
| 0x402090, 0x434800-0x435b50 | UserManager / User | | 1.3 |

The rest of the range is library code. Library instantiations in the range (names as in docs/FRAMEWORK_2004.md section 7.2, kept):
multiset<HighScoreEntry> helpers 0x410b40 _Max, 0x410b60 _Min, 0x410b80 _Rrotate, 0x410be0 iterator++,
0x411190 _Lrotate, 0x411460 _Buyhead, 0x416180 erase(it), 0x416850 _Buynode, 0x416cf0 _Insert, 0x41a0b0
ctor; ByScore: 0x412560 _Lbound, 0x412ba0 _Erase, 0x416ad0 find, 0x416c30 erase(first,last), 0x417ef0
insert, 0x4186f0 dtor; ByWordLength: 0x412600, 0x412bf0, 0x416b60, 0x416eb0, 0x417fb0, 0x418730;
set<string>: 0x410c40 iterator++, 0x4114f0 _Destroyval, 0x412870 erase(it), 0x412c40 _Erase, 0x416f70
erase(range), 0x418770 dtor; list<string>: 0x411440, 0x4114c0, 0x412420, 0x4124c0, 0x412820, 0x416a90;
list<T*>: 0x412780 _Incsize, 0x416bf0 push_back; vector<int>: 0x4164d0, 0x416590, 0x417d50; vector<string>:
0x416530, 0x417dc0, 0x417e70, 0x418160, 0x418680, 0x41a020; string/algorithm helpers 0x411100, 0x4111f0,
0x4114a0, 0x411520, 0x411c00, 0x411c30, 0x411c60, 0x411c90, 0x412360, 0x4123a0, 0x412b80, 0x412c90,
0x412cc0, 0x412d00, 0x416830, 0x4168f0, 0x416930, 0x417030, 0x417050, 0x417ec0; ostream 0x411350,
0x4113b0, 0x411670, 0x4116e0, 0x4118ad, 0x411970 (the word-list catch handler's console output).

### 14. Open questions

1. Who creates dialogs 0 and 3 (killed by StartGame/ShowGameSelector and handled in ButtonDepress)? No
   creator was found in this binary; treated as dead ids.
2. `FadeOut(27, true, 0.0075)` in StartGame addresses a song that does not exist (no-op in BASS's map). Was
   it meant to be song 0 (already faded by the loop)? Translate literally.
3. The loading progress ends at 166/216 with a sound device (47 sound tasks only counted without one). Kept
   literally; only visible if the port ever animates the loading bar.
4. The ctor leaves +0x1164 (mTitleScreen) and +0x1178 (mUserManager) uninitialised. Normal runs set them
   before use; an Init that fails before the title screen exists calls Shutdown -> ShutdownWidgets, which
   tests +0x1164 (garbage unless the fresh 0x1348-byte heap block happened to be zeroed). Port: NULL.
5. `GoToMainMenu` with no board calls the selector's slot 47 (start game) when the selector is idle; the only
   caller is the Board's own button, so the branch looks unreachable. Literal translation.
6. The meaning of mGameOption (+0x12dc / selector +0x80) and of the 16 one-shot hint flags at +0x12ec: the
   board and screens groups should name them.
7. ButtonDepress 2002 with an empty name or code calls the RegisterDialog's ButtonListener slot 4 with id 0
   (not ported, not traced).
8. HintDialog::WasShown (0x42b3e0) appears to look up key 0 in Ghidra's output although the caller passes the
   hint id; the dialogs group should confirm the key.

---

## Part 2. Board logic (Board.cpp, with Piece, StatKeeper, WordScore)

Module: `Board.cpp` 0x402ff0-0x4100f0 (INVENTORY.md). Classes: `Board : Widget, ButtonListener`
(sizeof **0x3f0**, `operator new(0x3f0)` in BookWormApp::NewGame 0x410d10), `Piece` (**0xf0**), `WordScore`
(**0x48**, embedded twice in Board), `StatKeeper` (**0x78**, `operator new(0x78)` x2 in the Board ctor).
This file owns the Board layout (authoritative) and every Board function except drawing. Drawing functions
are listed by address in section 8.1 and are specified by **board_draw**.

Conventions: `+0xNN` = offset in the object named; `app+0xNN` = BookWormApp field; "gGameRand" = the game's
global `MTRand*` at 0x5347d0 (created in WinMain from `Sexy::Rand()`), "mRand" = the Board's own `MTRand`
(Board+0x6c, seeded with `Sexy::Rand()` in the ctor), `Sexy::Rand()` = the framework's gMTRand. MTRand is
SAF 1.2's (SRand: `mt[i]=mt[i-1]*69069`; `Next()` returns `y & 0x7fffffff`, so `%` is on a non-negative
value). The CRT `rand()` is **never** used by this module. All `%` below are unsigned on `Next()`'s result.
"tick" = one `Board::Update` (100 per second). `mUpdateCnt` = Widget+0x4c (incremented by Widget::Update
inside Board::Update). Sound ids: `PlaySample(n)` = app vtable +0xa4, names from `gSoundNames` (0x52e668):
0 buttonclick, 1 gameovertone, 4..11 up1..up8, 13 nextselect, 14 wordcomplete, 15 bonuscomplete, 16 buzzer,
17 piece_hit, 19 flametile, 20 warning, 21 tileburn, 22 appear2, 25 redtilecleared, 26 spinsuperbonus,
27 bonusword, 28 screenburngameover, 29 lex_astonishing, 30 lex_awesome, 31 lex_burp, 32 lex_careful,
33 lex_danger, 34 lex_excellent, 35 lex_fantastic, 36 lex_getready, 37 lex_go, 38 lex_incredible,
39 lex_letsgo, 40 lex_levelup, 41 lex_lookout, 42 lex_scramble, 43 lex_unbelievable, 44 lex_verygood,
45 lex_watchout. Images: `app->mImages` = `*(Image***)(app+0x11ac)`, written `IMG(+0xNN)` with the name from
the image-name table at 0x52e410 (index = NN/4), e.g. IMG(+0x1d8) = lex_eating.

---------------------------------------------------------------------------------------------------------------

### 1. The board geometry

- The grid is `Piece* mBoard[8][7]` at Board+0x78, **[row][col]**, row 0 = top, index `row*7+col`.
  7 columns; **odd columns hold rows 0..7 (8 tiles), even columns rows 1..7 (7 tiles)**; slot `[0][even
  col]` is always NULL (49 tiles). Loops use "top row" `topRow(col) = (col even) ? 1 : 0`.
- Tile size 57x57 (`0x39`). Column x: `ColX(col) = col*57 + 233` (0x403990). Row y: `RowY(col,row) = row*57
  - 17` for even columns, `row*57 + 11` for odd columns (0x4039b0). Odd columns are 28 px lower (hex layout).
  Board widget is at (0,0) and 640x480 (app Resize).
- Adjacency `IsAdjacent(a, b)` (0x402f60, cdecl, in the BigLex.cpp range but a Board helper): `dc = a.col -
  b.col`, `dr = a.row - b.row`; adjacent if (|dc| == 1 and ((b.col even and dr in {0,-1}) or (b.col odd and dr
  in {0,+1}))) or (dc == 0 and |dr| == 1). Symmetric; a tile is not adjacent to itself.
- Hit test `PieceAt(x, y)` (0x4039e0): scans rows 0..7, cols 0..6 in memory order and returns the first
  non-NULL tile with `p.mX <= x < p.mX+57` and `p.mY <= y < p.mY+57` (double compares on the tile's
  *current* animated position).
- `PieceBelow(p)` (0x403bd0): `p.mRow < 7 ? mBoard[p.mRow+1][p.mCol] : NULL`.

### 2. Class layouts

#### 2.1 Board (sizeof 0x3f0; vtables: primary 47 slots `vftable_Sexy_Board`, ButtonListener at +0x58)

Ctor `Board::Board(BookWormApp*)` 0x40ae90; dtor body 0x4062e0, deleting dtor slot 0 0x406c10.
"R/W" = who reads/writes besides the ctor (functions by address; U = Update 0x40bd40, D = Draw/board_draw,
S/L = SaveGame/LoadGame).

| Off | Type | Name | Meaning | Init | R/W |
|---|---|---|---|---|---|
| 0x00 | Widget | (base) | 0x58 bytes, see widgets.md | | |
| 0x58 | vfptr | ButtonListener | `vftable_Sexy_Board_sub88` (slots 2 = 0x4081d0, 4 = 0x40fdc0, others empty) | | |
| 0x5c | BookWormApp* | mApp | | arg | all |
| 0x60 | std::list<DialogButton*> | mButtons | head +0x64, size +0x68; SUBMIT, OPTIONS, LEAVE GAME | 3 buttons | 0x405810/50/90, dtor |
| 0x6c | MTRand* | mRand | `new MTRand(Sexy::Rand())` (0x9c4) | | U, refill, NewBoard, S/L (state string) |
| 0x70 | EffectMgr* | mEffectMgr | `new EffectMgr(app)` (0x1c) | | U, D, S/L |
| 0x74 | EmberListMgr* | mEmberMgr | `new EmberListMgr(app, 1000)` (0x2c) | | U, D, S/L |
| 0x78 | Piece*[8][7] | mBoard | [row][col] | NULL then NewBoard | everything |
| 0x158 | OverlayWidget* | mOverlay | bonus-word panel/foreground (0x90), created in AddedToManager | 0 | U, 0x403d80, S/L |
| 0x15c | LevelUpWidget* | mLevelUpWidget | level/game summary (0xe0), 0x404b40 | 0 | app |
| 0x160 | Buffer | mGameLog | event log (section 7) 0x20 bytes | see 7 | app 0x42xxxx (score submit) |
| 0x180 | DWORD | mStartTick | GetTickCount() in ctor; never read | tick | |
| 0x184 | DWORD | mLevelStartTick | tick when the level clock last (re)started | tick | 0x405be0, 0x404b40, 0x408000 |
| 0x188 | int | mLevelSeconds | level duration (s) computed at summary | 0 | 0x404b40, app 0x41xxxx, S/L |
| 0x18c | int | mTotalSeconds | sum of mLevelSeconds | 0 | 0x404b40, app, S/L |
| 0x190 | int | mLevelSecondsBanked | seconds banked by pauses (`(now-start)/1000`, truncated each pause) | 0 | 0x405be0 |
| 0x194 | int | mState | `BoardState` (2.6) | 0 | U, app, LevelUpWidget |
| 0x198 | int | mSelectMode | `SelectMode` (2.6) | 0 | mouse, Submit |
| 0x19c | std::list<Piece*> | mSelection | selected tiles in order; head +0x1a0, size +0x1a4 | empty | |
| 0x1a8 | double | mFireMeter | classic "fire debt": a fire tile is due when <= 0 | **5.25** | Submit, Refill, S/L |
| 0x1b0 | bool | mSkipRandomFire | next chance-fire is skipped (set by bonus word) | **1** | Refill, Submit |
| 0x1b1 | bool | mBurnedDown | board burned (game-over visuals) | 0 | U, D, Lex, Overlay |
| 0x1b2 | bool | mUnused1b2 | **never initialised**, only saved/loaded | garbage | S/L |
| 0x1b3 | bool | mPiecesFlying | cleared each Update, set by FlyingPieceList (0x4209xx) | 0 | |
| 0x1b4 | bool | mFlameSoundPlayed | flametile (19) played once per refill burst | 0 | Refill, U, Scramble, Submit |
| 0x1b8 | int | mRewardStreak | consecutive words that earned a special tile | 0 | 0x40ba80 |
| 0x1c0 | double | mScrambleProgress | 0..1, state 7 | 0 | U, D, Scramble |
| 0x1c8 | double | mLevelProgress | target fraction of the level bar | 0 | Submit, LevelUp, U |
| 0x1d0 | double | mShownProgress | animated bar value | 0 | U, D |
| 0x1d8 | WordScore | mWord | current selection's word (2.3) | Clear | |
| 0x220 | WordScore | mLastWord | word shown above the board (D 0x407c60) | | U, Submit, mouse |
| 0x268 | Color | mLastWordColor | colour of mLastWord (set by D) | Color() | D, S/L |
| 0x278 | double | mLastWordAlpha | 1.0 while selecting, fades 0.08 per 8 ticks | 0 | U, D |
| 0x280 | double | mScoreGlow | score glow 0..0.85 | 0 | U, D |
| 0x288 | int | mBonusWordsGiven | bonus words handed out (length table index) | 0 | 0x408030 |
| 0x28c | std::string | mBonusWord | bonus word on the panel; "" when none | "" | U, Submit, 0x40abd0 |
| 0x2a8 | StatKeeper* | mGameStats | whole game | new | |
| 0x2ac | StatKeeper* | mLevelStats | current level | new | |
| 0x2b0 | std::string | mUnusedStr2b0 | only ctor/save/load | "" | S/L |
| 0x2cc | int | mUnused2cc | only ctor/save/load | 0 | S/L |
| 0x2d0 | std::string | mUnusedStr2d0 | only ctor/save/load | "" | S/L |
| 0x2ec | int | mUnused2ec | only ctor/save/load | 0 | S/L |
| 0x2f0 | int | mUpSoundIdx | rising "up" notes played so far | 0 | U, Submit |
| 0x2f4 | int | mUpSoundCount | notes to play (= word length) | 0 | |
| 0x2f8 | int | mUpSoundDelay | ticks to the next note | 0 | |
| 0x2fc | Piece* | mGameOverPiece | fire tile that reached the bottom | 0 | 0x408700, D, S/L (as row,col) |
| 0x300 | int | mDeadSparkDelay | spark burst timer; **never set > 0** (dead) | 0 | U |
| 0x304 | int | mBoardBurnDelay | ticks until the whole board burns | 0 | 0x408700 (60), U |
| 0x308 | double | mBoardBurnAlpha | 1.0 when the board burns, -0.01/tick | 0 | U, D |
| 0x310 | int | mBlinkDelay | ticks to the next blink | 0 | U, 0x408700 (210) |
| 0x314 | int | mBlinkRepeats | quick re-blinks left | 0 | U, 0x408700 (1) |
| 0x318 | int | mGameOverMusicDelay | ticks until game-over jingle | 0 | 0x408700 (160), U |
| 0x31c | int | mFireLineDelay | ticks until the screen-burn sound | 0 | 0x408700 (1), U |
| 0x320 | Piece* | mFireLinePiece | bottom fire tile driving the fire line | 0 | U, D, S/L |
| 0x328 | double | mFireLineAlpha | 0..1, +0.0125/tick | 0 | U, D |
| 0x330 | int | mFireLineScroll | scroll offset into IMG(+0x120) fx_fireline | 0 | U, D |
| 0x334 | int | mFireTileCount | fire tiles on the board | 0 | refill, Submit, Scramble, U |
| 0x338 | int | mActionFireTimer | action mode: ticks to the next timed fire tile | **1000** | U, LevelUp |
| 0x33c | int | mActionCountdown | action mode: seconds left while a fire tile is on the bottom row | 0 | U, 0x408700 |
| 0x340 | int | mWarnBeeps | warning beeps left (state 0) | 0 | U, 0x4042c0 |
| 0x344 | int | mWarnBeepDelay | | 0 | U |
| 0x348 | int | mHintDelay | ticks until hint mHintId pops | 0 | U, refill, Scramble |
| 0x34c | int | mHintId | `HintId` | 0 | |
| 0x350 | Lex* | mLex | the bookworm (0xd0), created in AddedToManager | 0 | many |
| 0x354 | bool | mLexGreetPending | greet when Lex stops walking | **1** | U, LevelUp (action) |
| 0x355 | bool | mLexDangerTalkReady | | 0 | U |
| 0x356 | bool | mLevelUpPending | `GT_LEVEL_UP` to announce | 0 | Submit, U |
| 0x357 | bool | mNewHighScore | set by app high-score code (0x419cd0), read by LevelUpWidget | 0 | app, S/L |
| 0x358 | bool | mLexPraisePending | Lex comments a special tile | 0 | 0x40ba80, U |
| 0x35c | int | mLexPraiseType | tile type for the comment | 0 | |
| 0x360 | int | mBlinkFrames | blink in progress (D) | 0 | U |
| 0x364 | bool | mBurpPending | word had >= 2 fire tiles | 0 | Submit, U |
| 0x365 | bool | mBurping | burp puff visible (D) | 0 | U |
| 0x368 | int | mBurpDelay | set to 50 by FlyingPieceList when a fire tile reaches Lex | 0 | U |
| 0x370 | double | mBurpAlpha | 1.0, -0.005/tick | 0 | U, D |
| 0x378 | int | mHighlightKey | letter key held (registered builds), tiles with that letter drawn lit | 0 | KeyDown/Up, D |
| 0x37c | bool | mCtrlDown | VK_CONTROL held | 0 | |
| 0x380 | DialogButton* | mSubmitButton | | | U, 0x407bf0, 0x409c60 |
| 0x384 | int | mSubmitBlinkWords | SUBMIT label blinks while > 0; -1 per valid word | **3** | U, Submit, L (reset 3) |
| 0x388 | bool | mPlayCounted | times-played counted for this board | 0 | Submit, Scramble, L (reset 0) |
| 0x38c | int | mLastLandSoundTick | mUpdateCnt of the last piece_hit | 0 | U |
| 0x390 | int | mCols | 7 (NewBoard clear loop) | 7 | |
| 0x394 | int | mRows | 8 | 8 | |
| 0x398 | int | mPointsAwarded | sum of scored words (popup values) | 0 | 0x409970 |
| 0x39c | int | mShownScore | rolling score display | 0 | U, D, Save |
| 0x3a0 | double | mScoreScale | multiplier for popups/roll; always 1.0 | **1.0** | |
| 0x3a8 | std::list<Points*> | mPopups | floating texts; head +0x3ac, size +0x3b0 | | U, 0x405a80, Pause |
| 0x3b4 | Points* | mLastMessage | last ShowMessage popup (stacking) | 0 | 0x409b70, U |
| 0x3b8 | int | mMouseX | MouseMove; never read here | 0 | |
| 0x3bc | int | mMouseY | | 0 | |
| 0x3c0 | bool | mSummaryShown | LevelUpWidget up | 0 | 0x404b40, U, ButtonDepress, app |
| 0x3c1 | bool | mPiecesMoving | a non-shaking tile fell this tick | 0 | U |
| 0x3c4 | int | mLevel | 0-based (`GT_BOARD_LEVEL_FORMAT` shows +1) | 0 | all |
| 0x3c8 | int | mPauseCount | > 0 = paused | 0 | Pause, input, Lex, app |
| 0x3d0 | double | mPauseFade | 0..1 (D: action-mode tiles hidden while > 0) | 0 | 0x403ae0, D |
| 0x3d8 | int | mUnused3d8 | | 0 | S/L |
| 0x3dc | bool | mPauseNoDialogs | set while paused: no dialog and no help widget | 0 | 0x403ae0, S/L |
| 0x3e0 | int | mBestWordThisLevel | max popup value | 0 | 0x409970, LevelUp |
| 0x3e4 | int | mBestWordThisGame | | 0 | 0x409970 |
| 0x3e8 | int | mUnused3e8 | zeroed by LevelUp | 0 | S/L |
| 0x3ec | int | mUnused3ec | | 0 | S/L |

Padding: 0x1b5-0x1b7, 0x1bc, 0x324, 0x32c (8-aligned doubles), 0x366, 0x36c, 0x374 (0x370 double), 0x3a4,
0x3cc. The ctor writes `0x3ff00000` to +0x3a4 (high half of mScoreScale) and `0x40150000` to +0x1ac (5.25).

#### 2.2 Piece (sizeof 0xf0; one virtual: slot 0 deleting dtor 0x404050)

Ctor 0x403f20 (`++gLivePieceCount` 0x534720), dtor 0x402ff0 (`--gLivePieceCount`, debug counter).

| Off | Type | Name | Meaning | Init |
|---|---|---|---|---|
| 0x08 | double | mX | screen x (top-left) | 0 |
| 0x10 | double | mY | screen y | 0 |
| 0x18 | double | mFlySpeed | FlyingPieceList speed factor | 0 |
| 0x20 | double | mVY | fall velocity (+0.2/tick) | 0 |
| 0x28,0x2c | int | mUnk28, mUnk2c | not referenced in Board.cpp; saved | 0 |
| 0x30 | int | mCol | 0..6 | 0 |
| 0x34 | int | mRow | 0..7 | 0 |
| 0x38 | bool | mUnk38 | saved | 0 |
| 0x3c | int | mUnk3c | saved | **1** |
| 0x40,0x41 | bool | mUnk40, mUnk41 | saved | 0 |
| 0x48,0x50 | double | mUnk48, mUnk50 | saved | 0 |
| 0x58,0x5c | int | | saved | 0 |
| 0x60,0x61 | bool | | saved | 0 |
| 0x64,0x68,0x6c | int | | saved | 0 |
| 0x70 | bool | | saved | 0 |
| 0x78,0x80 | double | | saved | 0 |
| 0x88..0x94 | int x4 | | saved | 0 |
| 0x98 | bool | | saved | 0 |
| 0x9c,0xa0 | int | | saved | 0 |
| 0xa4 | char | mLetter | 'A'..'Z' ('Q' = "QU") | 0xff |
| 0xa5 | char | mNextLetter | letter shown after a flip (scramble / spin) | 0xff |
| 0xa6 | bool | mSelected | in mSelection | 0 |
| 0xa8 | double | mScale | FlyingPieceList shrinks it (x0.98) | 1.0 |
| 0xb0 | bool | mBurning | burning away (charred by fire above) | 0 |
| 0xb4 | int | mBurnProgress | 0..57 px sunk | 0 |
| 0xb8 | int | mBurnDelay | ticks before sinking starts | 0 |
| 0xbc | bool | mPendingFire | action mode: becomes fire soon | 0 |
| 0xc0 | int | mUnkC0 | saved | 0 |
| 0xc4 | bool | mFlicker | frame animation on (board burn / game over) | 0 |
| 0xc8 | int | mFlickerFrame | 0..7 | 0 |
| 0xcc | bool | mShaking | jitter (tile under fire, bottom fire tile) | 0 |
| 0xd0 | double | mShakeAmount | jitter amplitude | 0 |
| 0xd8 | bool | mFlipping | letter flip / type-change animation | 0 |
| 0xe0 | double | mFlipProgress | 0..1, +0.01/tick | 0 |
| 0xe8 | int | mType | `TileType` | 0 |
| 0xec | int | mBurnCountdown | advances left before a tile under a fire tile catches | 0 |

Padding 0x04 (vfptr then double alignment), 0x44, 0x74, 0xa7, 0xdc. The fields marked "saved" are written by
Piece::Save and read back but never used by Board.cpp (FlyingPieceList/board_draw may; open question 1).

Piece methods (all in Board.cpp): `SetType(int type, bool resetCountdown)` 0x403000 (only `0 <= type < 6`;
reset -> mBurnCountdown = 0); `StartBurn()` 0x403030 (mBurning = 1, mBurnProgress = 0, mBurnDelay = 20);
`ResetBurnCountdown()` 0x404080 (mBurnCountdown = by type: Normal 1, Green 2, Gold 3, Sapphire 4, Diamond 5,
Fire 0); static `Deselect(Piece*)` 0x403f10 (mSelected = 0; used with `std::for_each`); `Save(FILE*)`
0x403050 / `Load(FILE*)` 0x403370 (section 6.2).

#### 2.3 WordScore (sizeof 0x48; one virtual: slot 0 deleting dtor 0x4059f0)

| Off | Type | Name | Meaning |
|---|---|---|---|
| 0x04 | BookWormApp* | mApp | set by the Board ctor |
| 0x08 | std::string | mWord | upper case; 'Q' tiles append "QU" (buf +0xc, size +0x1c, cap +0x20) |
| 0x24 | bool | mValid | in the dictionary |
| 0x28 | int[6] | mTypeCount | tiles of each TileType (Normal, Green, Gold, Fire, Sapphire, Diamond) |
| 0x40 | int | mNumPieces | tiles in the word |
| 0x44 | int | mScore | |

Ctor 0x405190 (all zero, string ""), copy-ctor 0x406560 (used only by board_draw 0x407c60), dtor 0x4051d0,
`operator=` 0x407b90 (field by field). Methods: `Clear()` 0x4069c0 (word "", valid 0, counts 0, mNumPieces
0; **mScore not cleared**); `Build(list<Piece*>*)` 0x406a10; `CalcScore()` 0x404f60; `CalcScoreByLength()`
0x4040e0 (dead); `CheckValid(set<string>*)` 0x406130; `CommonPrefix(string* out, const string& s)` 0x406b00;
`Save(FILE*)` 0x404fe0; `Load(FILE*)` 0x406180.

- **Build(list)** 0x406a10: Clear(); if mApp and list: mNumPieces = list size; for each non-NULL piece in
  order: `mTypeCount[clamp(type,0,5)]++`, append mLetter, and if the letter is 'Q' or 'q' append 'U'. Then
  `mScore = (app+0x125c == 0) ? CalcScoreByLength() : (app+0x125c == 1) ? CalcScore() : 0`. app+0x125c is
  set to 1 in the app ctor (0x41a3f8) and never changed: **CalcScore is the live formula**.
- **CalcScore** 0x404f60 (all int): `len = mWord.size()` (QU counts 2); if len < 3 -> 0. `sum = Σ
  gLetterValues[c-'A']` over every char of mWord (the 'U' of QU counts). `mult = len + 2*Green + 4*Gold +
  7*Sapphire + 10*Diamond`; if `mTypeCount[Fire] == mNumPieces` (every tile is fire) `mult += mNumPieces -
  1`. Score = `mult * (sum + board->mLevel + 1) * 10` (board = app+0x1160).
- gLetterValues (0x4f9888, indexed letter-'A'): A1 B4 C4 D3 E1 F5 G3 H5 I1 J8 K7 L2 M4 N2 O1 P4 Q7 R2 S1
  T2 U2 V5 W6 X8 Y6 Z10. (The code indexes `0x4f9784 + c*4`, i.e. the same table by raw char.)
- **CalcScoreByLength** 0x4040e0 (dead): len < 3 -> 0; `base = level + gLengthBonus[min(len,8)]`
  (0x4f98f0: 0,0,0,50,100,200,400,600,1000), `+ (len-8)*500` if len > 8; result `(2*Green + 1 + 4*Gold +
  7*Sapphire + 10*Diamond) * base * 10`.
- **CheckValid(set)** 0x406130: `mValid = len > 2 && set non-empty && set.find(mWord) != end`; returns
  mValid. The set is app+0x1260 (`std::set<std::string>`, the dictionary; case-sensitive `std::less`, so it
  must hold upper-case words). find = 0x405ea0 (lower_bound 0x405900 + compare).
- **CommonPrefix** 0x406b00 (thiscall on a WordScore, called from 0x431080): returns the longest common
  prefix of mWord and s (byte compare). Caller is the bonus-word panel (overlay agent).

#### 2.4 StatKeeper (sizeof 0x78; one virtual: slot 0 deleting dtor 0x4059c0)

| Off | Type | Name | Meaning |
|---|---|---|---|
| 0x04 | BookWormApp* | mApp | |
| 0x08 | int | mScore | words + bonus words |
| 0x0c | int | mWords | |
| 0x10 | int | mLetters | sum of word lengths (QU = 2) |
| 0x14 | int | mTiles | sum of mNumPieces |
| 0x18 | int | mFireTilesUsed | |
| 0x1c | int | mGreenTilesUsed | |
| 0x20 | int | mGoldTilesUsed | |
| 0x24 | int | mSapphireTilesUsed | **not cleared by Reset nor the ctor** (uninitialised heap) |
| 0x28 | int | mDiamondTilesUsed | **not cleared** (same) |
| 0x2c | int | mTilesBurned | |
| 0x30 | int | mScrambles | |
| 0x34 | int | mBonusWords | |
| 0x38 | std::string | mBestWord | highest-scoring word |
| 0x54 | int | mBestWordScore | |
| 0x58 | std::string | mLongestWord | |
| 0x74 | int | mLongestWordScore | score of mLongestWord |

Ctor 0x408690(app): strings empty, mApp, Reset(). Dtor 0x405140. `Reset()` 0x406950: zero +8..+0x20, +0x2c,
+0x30, +0x34, both strings "", +0x54, +0x74. `AddWord(const WordScore&)` 0x4068a0: mWords++, mScore +=
w.mScore, mLetters += len, mTiles += w.mNumPieces, Green/Gold/Fire/Sapphire/Diamond += the type counts; if
`mLongestWord.size() < len` -> mLongestWord = word, mLongestWordScore = w.mScore; if `mBestWordScore <
w.mScore` -> mBestWord = word, mBestWordScore = w.mScore (strict, first wins). `AddBonusWord(int pts)`
0x403690: mScore += pts, mBonusWords++. `IncBurned()` 0x4036b0, `IncScrambles()` 0x4036c0. `Save` 0x404e10 /
`Load` 0x405fa0 (6.3). Port: zero-initialise +0x24/+0x28 (open question 2).

#### 2.5 Tables (rdata)

| Address | Content |
|---|---|
| 0x4f97e0 | gLevelThresholds int[42]: 3000, 7000, 12000, 18000, 25000, 34000, 44000, 56000, 69000, 80000, 100000, 120000, 140000, 160000, 190000, 220000, 250000, 290000, 330000, 380000, 430000, 490000, 560000, 630000, 710000, 800000, 900000, 1000000, 1100000, 1200000, 1300000, 1500000, 1700000, 1900000, 2100000, 2300000, 2600000, 2900000, 3200000, 3500000, 3900000, 4300000 |
| 0x4f9888 | gLetterValues int[26] (2.3) |
| 0x4f98f0 | gLengthBonus int[9] (dead formula) |
| 0x4f9918 | gLetterWeights int[26]: A9000 B2200 C2200 D4200 E12300 F2300 G3000 H2000 I7000 J900 K900 L4000 M2300 N6000 O7000 P2300 Q700 R6000 S4400 T6000 U4400 V1600 W1800 X500 Y2000 Z500 (sum 95500); also read as `0x4f9814 + c*4` |
| 0x4f9980 | gFireChanceByLevel int[13] (percent, index min(level,12)): 0,0,0,2,3,4,6,8,12,16,24,32,30 |
| 0x4f99b8 | gFireMeterRefill double[12] (index min(level,11)): 5.25, 4.0, 3.5, 3.0, 2.5, 2.0, 1.8, 1.6, 1.5, 1.4, 1.3, 1.2 |
| 0x4f9a18 | gActionSpeed int[10] (index min(level,9)): 550,500,450,400,350,300,250,200,150,125 (written to a dead global) |
| 0x4f9a40 | gFireMeterByLength double[6] (index min(len-3,5)): -0.5, 0.35, 1.45, 1.85, 3.0, 4.75 |
| 0x4f9a70 | gBonusWordPoints int[11] (index min(bonus words found this game,10)): 1000, 2000, 4000, 6000, 10000, 15000, 20000, 25000, 30000, 40000, 50000 |
| 0x4f9a9c | gBonusWordLength int[10] (index min(bonus words given,9)): 3,3,3,3,3,4,4,4,5,5 |
| 0x52e280 | int[2][3] {221,152,83},{255,200,14}: SUBMIT label colours (normal, hover) when the word is invalid |
| 0x52e298 | int[2][3] {255,200,14},{255,200,14}: SUBMIT label colours when the word is valid |

`LevelThreshold(n)` 0x403880 (unsigned n): `gLevelThresholds[min(n,41)] + (n > 41 ? (n-22)*(n-41)*25000 :
0)` = cumulative game score at which level n (0-based) is complete.

Globals: 0x5347d0 gGameRand (MTRand*), 0x5347d4 gBookWormApp, 0x534720 gLivePieceCount, 0x52e2b0
gLastLetterIdx (int, init -1), 0x52e3a0 gLastRefillLevel (int, init -1), 0x52e278 gActionSpeed (double, init
1.0, **write-only**), 0x5347c8 gLen6Gate (int, bss 0, **never written**), 0x52e2b4 gPlayersDir (std::string
"players", static init 0x4f6c50).

#### 2.6 Enums

- `TileType` (Piece+0xe8): 0 Normal (tile), 1 Green (tile_green), 2 Gold (tile_gold), 3 Fire (tile_fire, "red/
  burning"), 4 Sapphire (tile_sapphire), 5 Diamond (tile_diamond). Names from the image table indices the
  draw code uses for each type (0x4b/0x4c, 0x51/0x52, 0x4f/0x50, 0x4d/0x4e, 0x53/0x54, 0x55/0x56).
- `BoardState` (+0x194): 0 Playing; 1 GameOverMusic (one tick: summary next); 2 GameOver (final);
  3 Inactive (Update returns at once; set by App 0x410f00 via `Abandon()` 0x405cd0 and by NewGame when
  dialog 5 is up); 4 GameOverPending (fire reached the bottom); 5 LevelComplete; 6 NextLevel (request);
  7 Scrambling.
- `SelectMode` (+0x198): 0 None, 1 Clicked (selection made by clicks), 2 Dragging, 3 SubmitRequested.
- Button ids: 16 SUBMIT, 17 OPTIONS, 18 LEAVE GAME. Dialog ids used: 15 (hint / `GT_CANT_SCRAMBLE`), 17
  (Dictionary Upgrade, trial only).
- `HintId` (+0x34c) cases of the hint switch (dialog title, body string address): 0 `GT_HINT_HOW_TO_PLAY_HEADER` 0x4fa1e8,
  1 `GT_HINT_WATCH_OUT_HEADER` 0x4fa158, 2 `GT_HINT_BONUS_TILE_HEADER` 0x4fa114, 3 `GT_HINT_SUPER_BONUS_TILE_HEADER` 0x4fa0a0, 4 `GT_HINT_WARNING_HEADER` 0x4fa040,
  5 `GT_HINT_SUBMIT_BUTTON_HEADER` 0x4f9fe8, 6 `GT_HINT_SUBMIT_CLICKING_HEADER` 0x4f9f88, 7 `GT_HINT_SUBMIT_DRAGGING_HEADER` 0x4f9ee8, 8 `GT_HINT_SUBMIT_KEYBOARD_HEADER` 0x4f9e80, 9 `GT_HINT_BONUS_WORD_HEADER` 0x4f9e28, 10 `GT_HINT_FIRE_ALERT_HEADER` (action body 0x4f9dc8 "...Use it
  soon...", classic body 0x4f9d68 "...Use it this turn..."). Footer always `GT_HINT_CONTINUE`
  (0x4fa234). **Only 1, 2, 4 and 10 are ever scheduled** (nothing writes 0, 3, 5-9 to mHintId).
- Lex animations (`Lex::PlayAnim(id, loop)` 0x42d320, ids index the Lex anim table 0x4fd9f0; meanings for the
  Lex agent): 0/1 classic greeting, 2/3/4/5 danger talk, 7 bonus word / time bonus, 8..0xe praise, 0xf level
  up, 0x11 action greeting, 0x12 scramble, 0x13 danger idle loop, 0x14 green tile appeared, 0x16 burp end,
  0x17 burp, 0x19 burnt (board burned).

---------------------------------------------------------------------------------------------------------------

### 3. Random numbers: who draws what

- **gGameRand** (0x5347d0): letters (`RandomLetter`), green-tile roll, chance-fire roll, special-tile awards,
  scramble fire picks, action-mode timed fire pick, bonus-word pick.
- **mRand** (Board+0x6c): tile drop heights (NewBoard, Refill), Lex chatter choices, and **all particle
  effects the Board spawns** (embers, sparkles, shakes). Every particle draw advances mRand, so the board
  stream depends on frame-exact effect spawning; translate every draw listed below in order.
- **Sexy::Rand()** (gMTRand): only the seeds: Board ctor (mRand), then EffectMgr and EmberListMgr ctors
  (each `new MTRand(Sexy::Rand())`, effects agent).
- EffectMgr / EmberListMgr own MTRands are used inside their code (effects agent); `NewEmber`/`AddEmber`
  calls listed here do not draw from mRand themselves.

**RandomLetter()** 0x403d10 (static): `do { x = gGameRand.Next() % 95500; i = first index with x <
cumsum(gLetterWeights[0..i]) (25 = 'Z' if none); } while (i == gLastLetterIdx); gLastLetterIdx = i; return
'A'+i;` - no two consecutively generated letters (program-wide) are equal; each rejection costs a draw.

---------------------------------------------------------------------------------------------------------------

### 4. Functions

#### 4.1 Construction, widget plumbing

**Board::Board(BookWormApp* app)** 0x40ae90. Widget ctor; vtables; lists; Buffer; two WordScores (mApp =
app); strings; mApp. Fields as in 2.1 (ctor column). WordScore mWord.Clear(). Grid slots zeroed. Then three
buttons through `MakeButton` 0x40ad00 (`DialogButton* MakeButton(int id, const Rect& r, Image* btn = 0,
Image* over = 0, Image* down = 0, const std::string& label)`):
`new DialogButton(IMG(+0x20) dialogbutton, id, this+0x58 listener)`; Resize(rect) (slot 12); mButtonImage =
mOverImage = mDownImage = 0 (+0x80/+0x84/+0x88); mDoFinger = 1; mLabel = label; SetFont(app+0x11c0) (slot
47); SetColors(0x52e280, 2) (slot 6, int[][3]); `app->mWidgetManager->AddWidget`; push_back into mButtons.
- SUBMIT id 16 Rect(6, 100, 211, 30) -> mSubmitButton; OPTIONS id 17 Rect(6, 181, 211, 30); LEAVE GAME id 18
  Rect(6, 446, 211, 30).
Then: `seed = Sexy::Rand(); mRand = new MTRand(seed)`; mStartTick = mLevelStartTick = GetTickCount();
mEffectMgr = new EffectMgr(app); mEmberMgr = new EmberListMgr(app, 1000); remaining fields; mGameStats,
mLevelStats = new StatKeeper(app); game log header (7); `NewBoard()`; mUnused3d8 = 0; if action mode
`gActionSpeed = (double)gActionSpeed[min(level,9)]` (dead).

**~Board** 0x4062e0 (slot 0 0x406c10): RemoveLevelUpWidget (0x403ea0); `operator delete(mRand)`; overlay:
RemoveWidget + delete; delete mEffectMgr, mEmberMgr (vtable); DeleteAllPieces 0x4036e0 (delete every grid
piece, NULL the slot); DeleteButtons 0x405de0 (RemoveWidget if present, delete); delete both StatKeepers;
Lex: RemoveWidget + delete; mPopups nodes freed (**popup widgets not deleted**: they were SafeDeleted when
removed); strings, WordScores, selection list, Buffer, button list; Widget dtor. Pieces held by a
FlyingPieceList are owned by it.

**AddedToManager(WidgetManager*)** slot 1, 0x403730: base; `mOverlay = new OverlayWidget(app)`;
Resize(0,0,mWidth,mHeight); AddWidget(overlay). If no Lex: `mLex = new Lex(app)`; Resize(0, 0, w, h) of
IMG(+0x1d8) lex_eating; AddWidget; Move(-200, 0) (slot 14); Lex+0x80 = 60, Lex+0x84 = 210 (walk target).

**RemovedFromManager(WidgetManager* wm)** slot 2, 0x405b00: base; RemoveButtons 0x405810 (RemoveWidget each
button from app WM); if Lex/overlay present in wm: RemoveWidget; every popup: wm->RemoveWidget,
app->SafeDeleteWidget (slot 22); clear mPopups.

**OrderInManagerChanged()** slot 3, 0x405240: if Lex && mVisible: Lex->SetVisible(true); for each button:
`wm->PutInfront(mLex, button)`; overlay: `PutInfront(mOverlay, mLex ? mLex : this)`. Resulting order:
board < buttons < Lex < overlay.

**SetVisible(bool v)** slot 4, 0x405a50 and **SetDisabled(bool d)** slot 10, 0x405a20: base call, then
**SetVisible(arg)** (slot 4) on every button and on Lex - both overrides pass their argument to the
children's SetVisible (SetDisabled(true) shows them). Translate literally.

**MouseLeave()** slot 24, 0x403a70: jumps to Widget::MouseLeave.

**MouseMove(x, y)** slot 25, 0x403a80: base (empty); mMouseX = x, mMouseY = y.

**KeyChar(char c)** slot 20, 0x405d60: if c == ' ': `app->PauseMusic(mPauseCount == 0)` (0x4184d0: true pauses
songs 0..4, false resumes) then `Pause(mPauseCount == 0)` - space toggles pause.

**KeyDown(int vk)** slot 21, 0x403df0 / **KeyUp** slot 22, 0x403e40: vk 0x11 (VK_CONTROL): mCtrlDown = 1 / 0.
If app+0x470 (mIsRegistered) && mState == 0 && `isalpha(vk)`: mHighlightKey = vk / 0, MarkDirty. No base call
(TAB focus not handled). No Enter handling exists in Board.cpp (open question 3).

**Pause(bool pause)** 0x405be0: `old = mPauseCount`; pause ? ++ : (--, clamp 0). Every popup SetVisible
(mPauseCount == 0). If now 0 and old > 0: SetButtonsDisabled(false) (0x405890: slot 10 on every button),
mLevelStartTick = GetTickCount(). If now > 0 and old < 1: SetButtonsDisabled(true), mLevelSecondsBanked +=
(GetTickCount() - mLevelStartTick)/1000 (unsigned, truncated). MarkDirty.

**UpdatePauseFade()** 0x403ae0 (first thing in Update, also while paused): if mPauseCount < 1: if mPauseFade >
0: -= 0.05 (clamp 0), MarkDirty. Else: mPauseNoDialogs = (app->GetDialogCount() == 0 && app+0x116c (help
widget) == 0); if mPauseFade < 1.0: += 0.05 (clamp 1.0), MarkDirty.

**ShowOverlay(bool show)** 0x403d80 (bonus-word panel): if mOverlay: if overlay+0x6c == 0: overlay+0x6c =
mHeight; show ? (overlay+0x68 = 406, MarkDirty, overlay+0x70 = 1) : (overlay+0x68 = mHeight); overlay+0x78 =
10.0. (+0x68 target y, +0x6c current y, +0x78 slide speed: overlay agent.)

**RemoveLevelUpWidget()** 0x403ea0; **ShowSummary()** 0x404b40: app+0x1330 = 1; RemoveLevelUpWidget;
mLevelSeconds = mLevelSecondsBanked + (GetTickCount()-mLevelStartTick)/1000; mTotalSeconds += mLevelSeconds;
`mLevelUpWidget = new LevelUpWidget(app)` (0x42bd50, 0xe0); Resize(0,0,w,h); AddWidget; SetFocus; mSummaryShown
= 1; if app+0x116c (help open): widget->SetDisabled(this->mDisabled), SetVisible(this->mVisible).
LevelUpWidget's continue button (0x42bc90, other agent) hides it, focuses the board, sets Lex target
(60,210) and then either (mState == 2) DeleteSavedGame + back to title, or mState = 6.

**ClearPopups()** 0x405a80: for each popup: wm->RemoveWidget, app->SafeDeleteWidget; clear list.
**Abandon()** 0x405cd0 (called by app 0x410f00): ClearPopups(); mState = 3.
**IsInProgress()** 0x403720: `mState != 2`.

#### 4.2 Board creation and refill

**NewBoard()** 0x404150 (ctor only): mLevelProgress = mShownProgress = 0. Zero `mBoard[r][c]` for r < mRows,
c < mCols (no delete). For col 0..6: `y = -64 - (mRand.Next() & 63)`; for row 7 down to top row: `p = new
Piece`; p.mCol = col; p.mRow = row; p.mX = ColX(col); p.mY = y; p.mLetter = RandomLetter(); p.mVY = 0;
mBoard[row][col] = p; `y += -57 - (mRand.Next() % 80)`. Draw order per column: 1 mRand, then per tile
RandomLetter (gGameRand, >= 1 draw) then 1 mRand. All tiles Normal, they fall in (section 4.4).

**Refill()** 0x40b540 (after removals; also called by Scramble and the action timer even without holes):
1. `levelUp = mLevel > gLastRefillLevel; gLastRefillLevel = mLevel` (static, survives games; -1 at start).
2. `chance = gFireChanceByLevel[min(level,12)]`; `fire = 0`. If not action mode and chance > 0: `r =
   gGameRand.Next() % 100` (only then); if r < chance: if mSkipRandomFire: mSkipRandomFire = 0 else fire = 1.
3. If !levelUp and mFireMeter <= 0 and classic mode: fire++, mFireMeter += gFireMeterRefill[min(level,11)].
4. For col 0..6:
   - Compact: `top = 7`; scan slots row 7..0: each non-NULL tile: if its mRow != top -> moved = 1; mRow =
     top; `mBoard[top][col] = tile`; top--. Slots above are overwritten below (stale pointers are not
     cleared here).
   - `y = -64 - (mRand.Next() & 63)` (always drawn, even if the column is full).
   - For row = top down to topRow(col): `p = new Piece`, mRow = row, mCol, mX = ColX(col), mY = y, mLetter =
     RandomLetter(), mVY = 0. If `fire > 0 && top == topRow(col)` (the column needed **exactly one** tile;
     compares the column's first empty row, not the current row): SetType(Fire, true); mFireTileCount++; if
     mSelection non-empty: mWord.Build + CheckValid; if !mFlameSoundPlayed: = 1, PlaySample(19 flametile);
     p.mLetter = RandomLetter() (second letter draw); fire--. Then if p.mType == Normal and `gGameRand.Next()
     % 20 == 0`: SetType(Green, true); if selection non-empty rebuild word; PlaySample(22 appear2);
     Lex::PlayAnim(0x14, 0); if !app+0x12ee (green hint seen) and mHintDelay == 0: app+0x12ee = 1,
     mHintDelay = 80, mHintId = 2. Store p; `y += -57 - (mRand.Next() % 80)`.
     Leftover `fire` is discarded (the meter was refilled anyway).
   - Second pass rows 7..0 of this column: for each Fire tile: if row < 7: below = PieceBelow; if below and
     below.mBurnCountdown <= 0: below.ResetBurnCountdown(). If row == 7 and !app+0x12ed: mHintDelay = 30,
     mHintId = 10, app+0x12ed = 1.
5. If moved: validate the selection chain (consecutive pairs prev,cur from the front): stop at the first pair
   where `!cur.mSelected || !IsAdjacent(prev, cur)`; then TruncateAfter(prev) (0x405cf0: deselect and erase
   all nodes after prev), pop the last node (prev itself, not deselected); if the list is still non-empty
   AddLetter(prev) (re-appends it; plays 13) else prev.mSelected = 0 (mWord is left stale).

**CheckBottomFire()** 0x4042c0: for each column, rows 7..top: if a Fire tile is in row 7 and mWarnBeeps == 0
and classic mode: mWarnBeeps = 3.

**GetNormalPieces(vector<Piece*>*)** 0x40b9e0: every tile (columns 0..6, rows 7..top) with mType Normal,
!mBurning, !mPendingFire.

#### 4.3 Selection, mouse and buttons

Input gate used by MouseDown/MouseUp/MouseDrag: not paused (mPauseCount < 1), `app->mUpdateCount(+0x4b8) -
app+0x130c >= 15` (15-update debounce after unpausing), mState == 0.

**AddLetter(Piece* p)** 0x409c60: if p is in mSelection: deselect it and every later node and erase them.
`wasSelected = p.mSelected; p.mSelected = 1`; push_back(p); mWord.Build(mSelection); `special = Green+Gold+
Sapphire+Diamond counts > 0` (types 1,2,4,5); mWord.CheckValid(app+0x1260); SUBMIT SetColors(valid ?
0x52e298 : 0x52e280, 2); `PlaySample(valid && !wasSelected && special ? 15 bonuscomplete : 13
nextselect)`; MarkDirty.

**ClearWord()** 0x407bf0: clear mSelection (no deselect), mWord.Clear(), mSelectMode = 0, SUBMIT
SetColors(0x52e280, 2), SUBMIT+0x20 (mIsOver) = 0, MarkDirty.

**MouseDown(x, y, cc)** slot 27, 0x409de0: Widget::MouseDown. If paused: Pause(false); if mPauseCount == 0:
app->PauseMusic(false), app+0x130c = app->mUpdateCount. Return. Else, behind the input gate:
- cc < 0 (right button): deselect all, ClearWord, mLastWord = mWord; return.
- p = PieceAt(x,y); none -> return. If any tile in p's column from p.mRow to 7 has mBurning -> return.
- If mSelectMode == 0: AddLetter(p).
- Else if cc > 1 (double click): size < 2: deselect all, clear list, mLastWord = mWord (stale word); else
  mSelectMode = 3.
- Else if selection empty: AddLetter(p).
- Else if p == front: size < 2 -> deselect all, clear, mSelectMode = 0; else AddLetter(p) (selection becomes
  [p]), mSelectMode = 0.
- Else if p == back: size < 2 -> deselect all, clear (mode unchanged); else mSelectMode = 3.
- Else walk from back to front: first node q with `IsAdjacent(q, p) || p.mSelected`: TruncateAfter(q),
  AddLetter(p), return. None: deselect all, clear, AddLetter(p).

**MouseDrag(x, y)** slot 31, 0x40a0c0: base (empty). Gate + selection non-empty: p = PieceAt; if p and p !=
back: mSelectMode = 2; if IsAdjacent(p, back) && !p.mBurning: AddLetter(p) (no selected check: dragging back
over a selected tile truncates to it).

**MouseUp(x, y, cc)** slot 29, 0x40fc40: Widget::MouseUp; return if paused. Behind the debounce: if mState == 0:
if cc < 0 or no tile under the cursor: deselect all, clear list, mLastWord = mWord. Else if cc > 0: mode 0 &&
selection non-empty -> mSelectMode = 1, MarkDirty, return; mode 2 or 3 && selection non-empty -> Submit(
(int)(back.mX + 28.0), (int)(back.mY + 28.0)) (`_ftol`; if back is NULL: Submit(x, y)), MarkDirty, return.
MarkDirty (whenever the debounce passed).

**ButtonPress(id, cc)** listener slot 2, 0x4081d0 (this = Board+0x58): PlaySample(0 buttonclick); if cc < 0 &&
id == 16: deselect all, ClearWord, mLastWord = mWord.

**ButtonDepress(id)** listener slot 4, 0x40fdc0: if mState != 2 && mSummaryShown: mState = 6, return (any
button continues to the next level while the summary is up). id 16: if selection non-empty and back != NULL:
mSelectMode = 3; Submit((int)(back.mX+28), (int)(back.mY+28)); mSelectMode = 1. id 17: App::ShowOptions
(0x4107c0). id 18: if IsInProgress(): App::AskLeaveGame (0x4157e0).

#### 4.4 Submitting a word

**Submit(int x, int y)** 0x40f5b0:
- Trial gate: if `mWord.size() >= 9 && !app->mIsRegistered`: "Dictionary Upgrade!" (not ported, 9).
  Registered builds always take the normal path.
- Invalid word (!mWord.mValid): if mSelectMode == 3 && selection non-empty: PlaySample(16 buzzer).
  mSelectMode = 1.
- Valid word:
  1. Definitions: if app+0x12e9 (show definitions option) and app+0x1174 (Definitionator): `def =
     Definitionator::Lookup(mWord.mWord)` (0x41d2b0); if def non-empty: `Lex::ShowDefinition(word,
     font app+0x11f8, def, font app+0x11f0, def.size()*3 + 400)` (0x42d8d0).
  2. mSubmitBlinkWords-- if > 0.
  3. If !mPlayCounted && app+0x1340 == 0 && app+0x133c == 0: app->mTimesPlayed(+0x52c)++, app+0x12e0++,
     mPlayCounted = 1.
  4. mLevelStats.AddWord(mWord); mGameStats.AddWord(mWord).
  5. Log (7): 0xd/4, WriteLong(score), len/4, each char `c-'A'`/5 bits (QU logs 'Q','U'); if
     mTypeCount[Normal] < mNumPieces: 0xc/4 then each selected tile's mType/3 bits.
  6. If fire tiles in word > 1: mBurpPending = 1.
  7. Fire meter: `n = len-3`; if (fire tiles == 0 || gFireMeterByLength[min(n,5)] > 0) mFireMeter +=
     gFireMeterByLength[min(n,5)] (3-letter words without fire tiles lower it by 0.5; with fire tiles the
     negative entry is skipped).
  8. Bonus word: if `_stricmp(mWord, mBonusWord) == 0`: `pts = gBonusWordPoints[min(mGameStats.mBonusWords,
     10)]` (read before the increment); BonusWordScored(pts, x, y); mSkipRandomFire = 1; mFireMeter += len;
     log 0xb/4 + WriteLong(pts). (mBonusWord is "" when no panel: never matches a real word.)
  9. mLastWordAlpha = 1.0; mLastWord = mWord (field copy).
  10. BurnTiles(classic mode).
  11. mUpSoundIdx = 0; mFireMeter -= 1.0; mUpSoundCount = len; mUpSoundDelay = 20.
  12. PlaySample(fire tiles in word ? 25 redtilecleared : 14 wordcomplete).
  13. RemovePieces(mSelection) 0x4052d0: for each piece: grid slot = NULL; if Fire: mFireTileCount--, and the
      tile below (if any) ResetBurnCountdown().
  14. `new FlyingPieceList(overlay's EffectMgr (overlay+0x5c), &mSelection)` (0x420920, 0x20) handed to
      OverlayWidget::AddEffect (0x430a20). The pieces now belong to it (they fly to Lex's mouth).
  15. `prev = level ? LevelThreshold(level-1) : 0; cur = LevelThreshold(level)`; mLevelProgress =
      (double)mLevelStats.mScore / (cur - prev). If mGameStats.mScore >= cur && mState == 0:
      overlay+0x60 (word balloon) slot 49(0); mState = 5; mLevelUpPending = 1.
  16. Refill(); AwardSpecialTile(mWord) (4.5); AddScorePopup(mWord.mScore, x, y); ClearWord();
      CheckBottomFire(); mSelectMode = 0.
- Always: mFlameSoundPlayed = 0; MarkDirty.

**BonusWordScored(int pts, int x, int y)** 0x40abd0: PlaySample(27 bonusword); Lex::PlayAnim(7, 0);
ShowMessage(StrFormat(`GT_BOARD_BONUS_FORMAT`, pts), y + 60); mLevelStats.AddBonusWord(pts); mGameStats.AddBonusWord
(pts); if mBonusWord != "": app's found-words set (app+0x127c `set<string>`) insert mBonusWord;
PickBonusWord(); mBonusWord = "". If overlay: ShowOverlay(false); overlay+0x68 = mHeight.

**PickBonusWord()** 0x408030: `want = gBonusWordLength[min(mBonusWordsGiven, 9)]`; list = app+0x1270
(`vector<string>`, 28-byte elements); `tries = list.size()`; while tries-- > 0: `w = list[gGameRand.Next() %
list.size()]`; if `_stricmp(w, app+0x12c0 current) != 0 && w.size() == want`: app+0x12c0 = w;
mBonusWordsGiven++; stop. (Gives up silently after size() failed tries; the old word stays.) Also called by
App::NewGame 0x410d10 right after creating the board. The panel picks the word up in Update (4.7 step 3).

**AddScorePopup(int score, int x, int y)** 0x409970: `shown = (int)(score*mScoreScale)`; `life =
clamp((int)(score*(1/3.0)), 150, 600)`; `new Points(app, font app+0x11e0, StrFormat("+%d", shown), mX+x,
mY+y, life, 0)` (0x431a50, 0x84); push to mPopups; AddWidget; mBestWordThisLevel/Game = max(.., shown);
mPointsAwarded += shown.

**ShowMessage(const string& s, int yOff)** 0x409b70: if mLastMessage and its life (+0x5c) > 0 and yOff <
mLastMessage->mY + 36: yOff = mLastMessage->mY + 36 (stack under it); `new Points(app, font app+0x11e8, s,
233, mY + yOff, 200, -1)`; push to mPopups; AddWidget; mLastMessage = it.

#### 4.5 Special tiles (rewards)

**AwardSpecialTile(const WordScore& w)** 0x40ba80 (gGameRand draws only where written):
1. `t` by length (L = w.mWord.size(), QU = 2): L >= 11: 5; L == 10: `(r & 1) | 4` (4 or 5); L == 9: 4; L == 8:
   `r & 1 ? 4 : 2`; L == 7: `r & 3 ? 4 : 2`; L == 6: 2 (the `gLen6Gate >= 6` branch with a draw is dead);
   L == 5: `r % 5 == 0 ? 2 : 0`; L == 4: `r % 100 == 0 ? 2 : 0`; else 0. (r = one gGameRand.Next() where a
   draw is shown.)
2. If w's Diamond count >= 2: mLexPraisePending = 1, mLexPraiseType = t (value at this point).
3. Combination upgrades: Sapphire count >= 2 -> t = 5; else Gold count >= 3 -> t = 5; else Gold count == 2 ->
   t = max(t, 4) (t = 4 unless t is 5); else if Green count >= 3: `r1 = gGameRand.Next()`; if t <= (r1 & 1 ? 4 :
   2): `r2 = gGameRand.Next()`, t = (r2 & 1 ? 4 : 2).
4. Streak: if t != 0 and app->mIsRegistered: mRewardStreak++, else mRewardStreak = 0. Streak >= 5: t = 5 (if t <
   6); streak 3..4: t = 4 if t < 5. If t == 0 -> return.
5. Candidates = GetNormalPieces; if non-empty: `p = cand[gGameRand.Next() % cand.size()]`; p.SetType(t, true);
   if selection non-empty rebuild word; PlaySample(26 spinsuperbonus); mLexPraisePending = 1, mLexPraiseType
   = t; p.mNextLetter = p.mLetter; p.mFlipProgress = 0; p.mFlipping = 1.
Green tiles come only from Refill (5% of new Normal tiles, both modes).

#### 4.6 Fire tiles, burning and game over

**BurnTiles(bool advance)** 0x408700 (classic: after each submitted word and each scramble; action: from the
timer, `advance = action mode`). For col 0..6, rows 7..top, every Fire tile f:
- Row < 7: `b = PieceBelow(f)`; if b && advance && b.mBurnCountdown > 0 && (action || !f.mSelected): if
  `--b.mBurnCountdown == 0`: action: if f or b selected -> deselect all + ClearWord; classic: if b.mSelected
  -> skip (countdown stays 0 until Refill resets it). Then mLevelStats/mGameStats.IncBurned(); b.StartBurn().
- Row 7: action: if mActionCountdown < 0 -> GameOver; classic: if !f.mSelected -> GameOver.
- **GameOver**: mBlinkRepeats = 1, mFireLineDelay = 1, mGameOverPiece = f, mBoardBurnDelay = 60, mBlinkDelay
  = 210, mGameOverMusicDelay = 160, f.mShaking = 1; if overlay: overlay+0x60 slot 49(0), ShowOverlay(false);
  SetDisabled(true) (slot 10); mState = 4. The scan continues (another bottom fire tile repeats the same
  writes).
Burning tiles (mBurning) sink in Update (4.7) and are removed at 57 px; the fire tile then falls into the gap.

**Scramble()** 0x40ed70 (Lex::MouseUp 0x42d4e0 calls it when Lex is clicked). Only in mState 0:
1. If mFireTileCount > 9: PlaySample(16); `d = app->DoDialog(15, true, `GT_CANT_SCRAMBLE`, `GT_CANT_SCRAMBLE_TOO_MANY_FIRE`, "OK", 3)`; d->Move((mWidth - d.mWidth + 233)/2, (mHeight -
   d.mHeight)/2). Return.
2. Candidates = the top tile of each column (row topRow(col)) if Normal. None: PlaySample(16); dialog 15
   `GT_CANT_SCRAMBLE` / `GT_CANT_SCRAMBLE_FIRE_AT_TOP` / `GT_DIALOG_OK`, same placement.
   Return.
3. Classic only: if any column has a Fire tile in row 7: PlaySample(16), return (no dialog); if any tile is
   mBurning: return.
4. If !mPlayCounted && app+0x1340 == 0 && app+0x133c == 0: app->mTimesPlayed++, app+0x12e0++, mPlayCounted =
   1. app+0x12e4 = 1.
5. Deselect all, clear mSelection. mLevelStats.IncScrambles(), mGameStats.IncScrambles().
6. `n = min(candidates, mGameStats.mScrambles)` (k-th scramble of the game makes up to k fire tiles); n times:
   `i = gGameRand.Next() % cand.size()`; p = cand[i]; p.SetType(Fire, true); mFireTileCount++; (word rebuild,
   never: list empty); if !mFlameSoundPlayed: = 1, PlaySample(19), Lex::PlayAnim(0x12, 0), PlaySample(42
   lex_scramble); erase cand[i] (vector erase, order kept); 100 embers, each with six mRand draws in this
   order: `a40 = r1%1000*0.001*0.05 + 0.02; vy = (r2%1000 - 500)*0.004; vx = (r3%1000 - 500)*0.004; y =
   r4%57 + p.mY; x = r5%57 + p.mX; type = (r6%100 < 80) ? 0 : 3;` then `AddEmber(type, x, y, vx, vy, false,
   1.0, a40)` (vx -> ember+0x20, vy -> +0x28, 1.0 -> +0x38, a40 -> +0x40; verified from the push order at
   0x40f310..0x40f460).
7. All tiles (cols 0..6, rows 7..top): Fire: mNextLetter = mLetter; others: SetType(Normal, false)
   (**specials are lost**), mNextLetter = RandomLetter().
8. If !app+0x12f0 and mHintDelay == 0: mHintDelay = 100, mHintId = 4, app+0x12f0 = 1.
9. BurnTiles(classic); Refill(); CheckBottomFire(); mState = 7; mScrambleProgress = 0; mFlameSoundPlayed =
   0; MarkDirty. (Letters swap to mNextLetter when the state-7 animation ends, 4.7.)

**StartGameOverMusic()** 0x4038c0: log 0x0/4 + WriteLong(mGameStats.mScore); mState = 1; mLevelProgress = 0.
Classic: music StopMusic(1), StopMusic(4), FadeIn(3, 61, 1.0, true). Action: StopMusic(2), StopMusic(4),
FadeIn(3, 59, 1.0, true).

#### 4.7 Board::Update (slot 17, 0x40bd40)

Phases in order (`cnt` = mUpdateCnt after the base Update; "Lex idle" = `Lex+0xb4 == -1 && (double)Lex+0x80 ==
Lex+0x88 && (double)Lex+0x84 == Lex+0x90` (0x404340, i.e. no anim and not walking); "Lex calm" = Lex idle or
`Lex+0xb4 == 0x13`; "Lex still" = not walking (0x403ab0)):

1. mState == 3: return. UpdatePauseFade(). Paused: return.
2. `wm->BringToBack(this)`; mPiecesFlying = 0; Widget::Update; app+0x132c++ (game ticks). A local list of
   dead pieces is created.
3. **mState == 0 only**:
   - Bonus-word panel: if mBonusWord empty and (mLevel > 1 or (mLevel == 1 and mLevelStats.mWords >= 1)) and
     the overlay is not sliding (0x430a00: overlay+0x6c == +0x68): mBonusWord = app+0x12c0; if non-empty
     ShowOverlay(true).
   - mHighlightKey != 0 and `cnt & 31 == 0`: MarkDirty.
   - Lex danger talk: if !Lex+0xc5 (danger off): mLexDangerTalkReady = 1. Else if ready and Lex still: ready
     = 0; `r = mRand.Next() & 3`: 0 -> anim 4 + sound 32 lex_careful; 1 -> anim 5 + 33 lex_danger; 2 -> anim
     2 + 41 lex_lookout; 3 -> anim 3 + 45 lex_watchout (PlayAnim(id, false) then PlaySample). Then, still
     with danger on (talk or not), if Lex idle: PlayAnim(0x13, true).
   - Action mode: if Lex danger off: mActionCountdown = max(10, mActionCountdown). Else every `cnt % 100 ==
     0`: ShowMessage(StrFormat("%d", mActionCountdown), 180); PlaySample(20 warning); if --mActionCountdown <
     0: deselect all + ClearWord.
   - Greeting: if mLexGreetPending and Lex still: = 0; action: anim 0x11 + sound 36 lex_getready; classic: `r =
     mRand.Next() % 100`: r < 50 -> anim 0 + 37 lex_go, else anim 1 + 39 lex_letsgo.
4. mPiecesMoving = 0. If mState == 6 and no dialogs: mSummaryShown = 0; NextLevel() (0x408000); if
   mActionCountdown < 10: = 10 and, in action mode with Lex danger on: ShowMessage(`GT_BOARD_TIME_BONUS`, 160),
   Lex anim 7, PlaySample(14). mState = 0; skip to step 6.
5. Else if mState in {0, 5, 4, 2}: **tile pass**. Flags burn = rf = fireBottom = 0.
   - Action and mState 0: mActionFireTimer--; if mActionCountdown < 0: burn = rf = 1. If timer <= 0: burn =
     rf = 1, timer = 1000, pick a top-row tile weighted by letter frequency: `total = Σ gLetterWeights[letter]`
     over the 7 top tiles; `r = gGameRand.Next() % total`; walk columns 0..6 subtracting weights; the hit
     tile (r <= 0) becomes mPendingFire = 1 if Normal or Green, else the scan restarts from column 0 (up to 7
     passes) where the first non-NULL top tile is tested again.
   - SUBMIT blink: if mSubmitBlinkWords > 0, mWord.mValid, mSelectMode != 2 and `cnt & 31 == 0`: if
     `SUBMIT->GetColor(0) == Color(221,152,83)` SetColors(0x52e298, 2) else SetColors(0x52e280, 2); MarkDirty.
   - For col 0..6: `target = RowY(col, 7)`; rows 7..0, each tile t (NULL rows are skipped, target unchanged):
     a. t.mType != Normal: anySpecial = 1.
     b. t.mPendingFire: if `mRand.Next() & 7 == 0`: smoke ember pair: `e1 = AddEmber(type = (r6%100 >= 80),
        x = r5%57 + t.mX, y = t.mY + 57 - 3, vx = (r4%1000 - 500)*0.00125, vy = (r3%1000 - 1000)*(1/1500),
        flag 1, +0x38 = 1.0, +0x40 = r2%1000*0.001*0.025 + 0.01)` with draws r2..r6 in that order; then
        `e2 = AddEmber(10, e1.x - 8, e1.y - 8, 0, -0.25, 0, +0x38 = r8%1000*0.001, +0x40 = r7%1000*0.001*0.01
        + 0.004)`, e2+0x30 = 0. If mActionFireTimer < 666: t.mPendingFire = 0; SetType(Fire, true); rebuild
        word if selection; t.mNextLetter = t.mLetter; mFlipProgress = 0; mFlipping = 1; if !mFlameSoundPlayed:
        = 1, PlaySample(19); mFireTileCount++; rf = 1; 20 bursts i = 0..19: `a = i*0.05*-3.14159`; `+0x40 =
        r1%1000*0.001*0.01 + 0.02`; `x = r2%57 + t.mX`; `E = AddEmber(2, x, t.mY+57, vx = cos(a)*1.5, vy =
        sin(a)*2.5, 1, 1.0, +0x40)`, E+0x48 = 8; copy of E as type 1 (+0x48 = 4); copy as type 2 (+0x48 = 0);
        then `AddEmber(10, r5%57 + t.mX, t.mY + 57 - 3, 0, -0.25, 0, +0x38 = r4%1000*0.001, +0x40 =
        r3%1000*0.001*0.01 + 0.004)` (+0x30 = 0); draws r1, r2, r3, r4, r5 in that order.
     c. Sapphire/Diamond and `cnt & 15 == 0`: if `mRand.Next() % 10 == 0`: `s = new Sparkle(mEffectMgr)`
        (0x41f2b0, 0x48); s.x(+0x10) = mRand%57 + t.mX - 28; s.y(+0x18) = mRand%57 + t.mY - 28;
        mEffectMgr->AddEffect(s) (0x4207f0).
     d. Fire tile with row < 7: `b = PieceBelow`; if b is Gold/Sapphire/Diamond (not Normal, Green, Fire):
        b.mShaking = 1, b.mShakeAmount = 2.5 / max(b.mBurnCountdown, 1). Else (not that case) if t.mShaking:
        if the tile **above** (`mBoard[row-1][col]`) exists and is not Fire: stop shaking (amount 0) and snap
        mX/mY to ColX/RowY.
     e. t.mFlipping: mFlipProgress += 0.01; >= 1.0 -> 0, mFlipping = 0; MarkDirty.
     f. t.mFlicker and `cnt & 3 == 0`: mFlickerFrame = (frame+1) & 7; MarkDirty.
     g. mState == 4 and mBurnedDown and !t.mShaking: if `(mRand.Next() & 0xff) == 0`: t.mFlicker = 1.
     h. Fire tile in row 7: if mState == 2: twice: `E = NewEmber()`; E.type(+0x58) = (mRand%100 >= 80); E.x =
        mRand%57 + t.mX; E.y = t.mY + 57; E.vx = (mRand%1000 - 500)/300; E.vy = (mRand%500 - 500)/300; E+0x38 =
        0.5; E+0x40 = (mRand%1000*0.001 + 1)*0.002; E+0x4c = 1; then mFireLinePiece = t, t.mShakeAmount +=
        0.075. If mState == 0: if `mRand.Next() & 7 == 0`: E = NewEmber, type 0, x = mRand%57 + t.mX, y = t.mY
        + 54, vx = (mRand%1000 - 500)*0.00125, vy = (mRand%1000 - 1000)/1500, +0x38 = 0.65, +0x40 =
        (mRand%1000*0.001 + 1)*0.002; and if `mRand.Next() & 7 == 0`: E = NewEmber, type 10, x = mRand%57 +
        t.mX, vx = 0, vy = -0.25, +0x30 = 0, y = t.mY + 54, +0x38 = mRand%1000*0.001, +0x40 =
        mRand%1000*0.001*0.01 + 0.004. Then if !t.mShaking: mShaking = 1, mShakeAmount = 1.5. fireBottom = 1.
     i. If t.mY >= target (at rest): if t.mShaking and `cnt & 1 == 0`: t.mX = ColX + (mRand%1000 -
        500)*mShakeAmount*0.002; t.mY = RowY(col,row) + (mRand%1000 - 500)*mShakeAmount*0.002; MarkDirty. If
        t.mBurning: if mBurnDelay > 0: mBurnDelay--; else { if mBurnProgress == 0 and no tileburn this tick:
        PlaySample(21 tileburn); mBurnProgress++ }; if mBurnProgress >= 57: grid slot = NULL, t into the dead
        list, rf = 1; then 8 embers: E = NewEmber; type = (mRand & 15) ? 0 : 3; `k = mRand % 57`; E.x = k +
        t.mX; E.y = t.mY + mBurnProgress; E.vx = (k - 28)*(4/57.0); E.vy = (mRand%1000 - 300)*0.001; +0x38 =
        1.0; +0x40 = mRand%1000*0.001*0.05 + 0.02. (Embers every tick while burning, delay included.)
        Else (falling): t.mVY += 0.2; t.mY += t.mVY; MarkDirty; if !t.mShaking: mPiecesMoving = 1.
     j. If t.mY > target (landed): if t.mVY > 4.0 and `cnt - mLastLandSoundTick >= 4`: PlaySample(17
        piece_hit), mLastLandSoundTick = cnt. mVY = 0, mY = target, MarkDirty, if !mShaking mPiecesMoving = 1.
        If t.mY == target: mVY = 0.
     k. `target += t.mBurnProgress - 57` (tiles above a burning tile sink with it).
   - After the loop: anySpecial and `cnt % 12 == 0`: MarkDirty. If !mPiecesMoving and mFireTileCount > 0 and
     !app+0x12ec and mHintDelay == 0: mWarnBeeps = 1, mHintDelay = 80, mHintId = 1, app+0x12ec = 1.
   - burn: BurnTiles(action mode). rf: Refill() then CheckBottomFire(); else if burn: CheckBottomFire().
   - Danger music: fireBottom: Lex::SetDanger(true) (0x42d3c0); if !IsPlaying(4): action FadeOut(2, false,
     0.008) + FadeIn(4, 65, 0.008, false); classic FadeOut(1, false, 0.008) + FadeIn(4, 51, 0.008, false).
     Else: SetDanger(false); if IsPlaying(4): FadeOut(4, true, 0.008), then FadeIn(2 action / 1 classic, -1,
     0.008, false).
6. If mState in {1, 2} and !mBurnedDown and Lex idle: SetDanger(true).
7. `flying = overlay EffectMgr has effects of type 1` (0x430a90). mState == 1: if Lex idle and Lex danger and
   !mBurnedDown: PlayAnim(0x13, 1). If !mSummaryShown and !flying and Lex calm: app->KillDialog(15); App::
   RecordScore(mGameStats) (0x419cd0); ShowSummary(). mState = 2 (always).
8. If !flying: mState == 4 and Lex calm: (if Lex idle and danger and !mBurnedDown: PlayAnim(0x13, 1)); mState =
   2. Burp: if mBurpDelay > 0 and Lex idle and `--mBurpDelay == 0` and mBurpPending: if !mBurnedDown: mBurping
   = 1, mBurpAlpha = 1.0, PlayAnim(0x17, 0), PlaySample(31 lex_burp); mBurpPending = 0.
9. mBurpAlpha > 0: -= 0.005; if < 0.9 and Lex anim == 0x17: PlayAnim(0x16, 0); if < 0: mBurping = 0;
   MarkDirty.
10. Warning beeps: mWarnBeeps > 0 and mState == 0: mWarnBeepDelay-- if > 0; at 0: mWarnBeeps--, PlaySample(20),
    if mWarnBeeps > 0: delay = 64.
11. Up notes: if mUpSoundIdx < mUpSoundCount: mUpSoundDelay-- if > 0; at 0: PlaySample(4 + min(mUpSoundIdx,
    7)) (up1..up8), mUpSoundIdx++, delay = 10.
12. `pending = Σ (+0x18)` over the list returned by OverlayWidget::GetEffects(1) (0x430a70, a new list the
    caller frees).
13. Level bar: if mShownProgress != mLevelProgress and `cnt & 1 == 0` and pending <= 3: MarkDirty; `d =
    mBurnedDown ? -mShownProgress : mLevelProgress - mShownProgress`; d > 0: shown += d*0.04 + 0.01; d < 0:
    shown -= 0.05, then if shown < target: shown = target. Then if target > 0 and shown > target: shown =
    target.
14. Score roll: if mShownScore != mGameStats.mScore and `cnt & 1 == 0`: if shown < score: `shown += (int)
    (((score-shown)/mScoreScale*0.01 + 4.0)*mScoreScale)`, mScoreGlow = min(mScoreGlow + 0.00375, 0.85); if
    shown > score: shown = score; MarkDirty. Else if mScoreGlow > 0 and `cnt & 3 == 0`: glow = max(glow -
    0.00675, 0); MarkDirty.
15. Score sparks: if mScoreGlow > 0.385 and `cnt & 7 == 0`: `w = font(app+0x11e0).StringWidth(StrFormat("%d",
    mShownScore))`; `x = mRand%w + (108 - w/2)`; `r2 = mRand%100`; if mShownScore != score: `AddEmber(r2 >= 80,
    x, 31.0, vx = (r5%1000-500)*0.00125, vy = (r4%1000-1000)/1500, 1, 1.0, r3%1000*0.001*0.025 + 0.01)`
    (draws r3, r4, r5 in order); then `AddEmber(10, x-8, 23.0, 0, -0.25, 0, r7%1000*0.001, r6%1000*0.001*0.01
    + 0.004)` (+0x30 = 0; draws r6 then r7).
16. mEffectMgr->Update() (0x41f990) and mEmberMgr->Update() (0x420fd0): MarkDirty when they return true.
17. Popups: each: MarkDirty; +0x80 = (+0x80 + 7) % 255; if `cnt & 1 == 0` and +0xc > 0: Move(x, y-1); if
    --life(+0x5c) <= 0: (mLastMessage = 0 if it) erase, RemoveWidget, SafeDeleteWidget.
18. Last word: selection non-empty: mLastWord = mWord, mLastWordAlpha = 1.0. Else if alpha > 0 and `cnt & 7 ==
    0`: alpha -= 0.08, MarkDirty.
19. Hints: mHintDelay > 0, mSelectMode != 2, mState == 0, no dialogs: if --mHintDelay == 0: App::ShowHint(
    mHintId, title, body, `GT_HINT_CONTINUE`, 3) (0x4105d0: only when app+0x12e8 hints option; dialog
    id 15, placed at (233,110) width 400). Strings in 2.6.
20. mState == 7: mScrambleProgress += 0.01; >= 1.0: every tile mLetter = mNextLetter; progress 0; mState = 0.
    MarkDirty.
21. Blink: if mBlinkDelay > 0: if --mBlinkDelay == 0: mBlinkFrames = 20, MarkDirty. Else if mBlinkRepeats > 0:
    mBlinkRepeats--, mBlinkDelay = 40; else mBlinkDelay = mRand.Next() % 300. Then if mBlinkFrames > 0 and
    --mBlinkFrames == 0: MarkDirty.
22. mDeadSparkDelay (dead): if > 0 and reaches 0: 20 x 3 embers around mGameOverPiece (never happens).
23. Game-over sequence (each only while Lex calm): mBoardBurnAlpha > 0: -= 0.01, MarkDirty. mGameOverMusicDelay
    > 0: at 0 StartGameOverMusic(). mBoardBurnDelay > 0: at 0 **burn the board**: rows 0..7 x cols 0..6 in
    memory order, each non-NULL tile: `r = mRand.Next(); if ((r & 15) == 0 || row == 0) { mFlicker = 1;
    mFlickerFrame = mRand.Next() & 7; }` (the draw is taken before the row test); snap mX/mY to ColX/RowY,
    mBurning = 0, mShaking = 0, mType = Normal. Then 700 embers: E = NewEmber, type 0, x = mRand %
    mWidth, y = mRand % mHeight, vx = vy = 0, +0x30 = mRand%1000*0.001*0.005 + 0.002, +0x38 = 1.0, +0x40 =
    mRand%1000*0.001*0.001 + 0.00075; if mRand%30 == 0: type 2, +0x30 = mRand%1000*0.001*0.005 + 0.008;
    else if mRand%20 == 0: type 1, +0x30 = (mRand%1000*0.001 + 1)*0.005. Then buttons SetVisible(false);
    ClearPopups(); mFireLinePiece = 0; mFireLineAlpha = 0; mBoardBurnAlpha = 1.0; mBurnedDown = 1; if Lex
    calm: PlayAnim(0x19, 0); MarkDirty.
24. Fire line: if mFireLineDelay > 0 and (Lex idle or action mode or pending == 0): if --mFireLineDelay == 0:
    PlaySample(28 screenburngameover); MarkDirty. Else if mFireLinePiece and mFireLineDelay == 0:
    mFireLineAlpha += 0.0125 (clamp 1.0 when it passes 1); if `cnt & 7 == 0`: mFireLineScroll =
    (mRand%48 + 48 + mFireLineScroll) % width(IMG(+0x120) fx_fireline); MarkDirty.
25. Delete the dead pieces (burned away this tick).
26. mState == 5: mLexPraisePending = 0; if no dialogs, Lex idle and !flying: if !mLevelUpPending: if
    !mSummaryShown ShowSummary(); else PlaySample(40 lex_levelup), PlayAnim(0xf, 0), Lex::Say(`GT_LEVEL_UP`,
    font app+0x11b4, 75) (0x42d6c0), mLevelUpPending = 0.
27. Lex praise: mLexPraisePending, !flying, Lex idle, mState in {0, 5, 4}: = 0; `r = mRand.Next() % 100`;
    type Gold: r < 33 -> 44 lex_verygood anim 0xe; r < 66 -> 34 lex_excellent anim 10; else 30 lex_awesome
    anim 9. Sapphire: r < 50 -> 35 lex_fantastic anim 0xb; else 29 lex_astonishing anim 8. Diamond: r < 50 ->
    38 lex_incredible anim 0xc; else 43 lex_unbelievable anim 0xd. Other types: nothing (draw spent).

#### 4.8 Levels

**LevelUp()** 0x407a50: mLevelStats.Reset(); mLevelStats.mScore = mGameStats.mScore - LevelThreshold(mLevel)
(excess carried over); mLevel++; log 0xe/4 + WriteShort(level); mLevelProgress = 0; mActionFireTimer = 1000;
action: gActionSpeed (dead), mLexGreetPending = 1; mState = 0; mShownProgress = 0; mBestWordThisLevel = 0;
mUnused3e8 = 0; mLevelProgress = (double)mLevelStats.mScore / (LevelThreshold(level) - (level ?
LevelThreshold(level-1) : 0)).
**NextLevel()** 0x408000: mSummaryShown = 0; mLevelSecondsBanked = 0; mLevelStartTick = GetTickCount();
LevelUp().
Flow: Submit reaches the threshold -> mState 5 -> (Lex) `GT_LEVEL_UP` -> ShowSummary -> continue button (or any
board button) -> mState 6 -> Update: NextLevel. The board is **not** regenerated between levels.

#### 4.9 Classic vs Action

| | Classic (app+0x1254 = 0) | Action (1) |
|---|---|---|
| Random fire on refill | gFireChanceByLevel % + fire meter | none |
| Timed fire | none | every 1000 ticks a top tile becomes pending, fire 334 ticks later; BurnTiles(true) at each mark |
| BurnTiles on submit / scramble | yes | no |
| Fire at the bottom | game over at the next advance unless that tile is selected | 10 s countdown (shown each second); then game over |
| Warning beeps | 3 when a fire tile lands on row 7 | no |
| Music | song 1 (danger: song 4 order 51) | song 2 (danger: song 4 order 65) |
| Greeting | lex_go / lex_letsgo | lex_getready, also at every level |
| Pause | | tiles drawn letterless while mPauseFade > 0 (board_draw) |

---------------------------------------------------------------------------------------------------------------

### 5. Cross-module references

BookWormApp fields read/written (app agent): +0x8 mProdName (trial text), +0x60/+0x64 width/height, +0x94
mBuildNum, +0x98 mBuildDate, +0x3b8 mProductVersion (save header), +0x2e4 mWidgetManager, +0x33c
mMusicInterface, +0x470 mIsRegistered (letter highlight, reward streak, 9+ letter words), +0x4b8 mUpdateCount,
+0x52c mTimesPlayed, +0x1160 Board*, +0x116c HelpWidget*, +0x1174 Definitionator*, +0x1178 current user
profile (std::string name at +0x10), +0x11ac Image** table, fonts +0x11b4 (Lex `GT_LEVEL_UP`), +0x11b8, +0x11c0
(buttons), +0x11c4, +0x11d8, +0x11e0 (score/popups), +0x11e4, +0x11e8 (messages), +0x11f0/+0x11f8
(definitions), +0x1254 game mode (0 classic, 1 action), +0x1258 mode to load, +0x125c score formula (always 1),
+0x1260 set<string> dictionary, +0x1270 vector<string> bonus words, +0x127c set<string> bonus words found,
+0x12c0 std::string current bonus word, +0x12e0 games-played counter, +0x12e4 scrambled flag, +0x12e7 (show
help on load), +0x12e8 hints enabled (also saved), +0x12e9 show definitions, +0x12ec/+0x12ed/+0x12ee/+0x12f0
hint-seen flags (1, 10, 2, 4), +0x130c update count at unpause, +0x132c game ticks, +0x1330 summary-seen flag,
+0x133c/+0x1340 (block mTimesPlayed counting; trial/demo?), vtable +0x58 SafeDeleteWidget, +0xa4 PlaySample,
+0xdc DoDialog, +0xe0 GetDialog, +0xe8 KillDialog, +0xec GetDialogCount. App functions called:
0x4184d0 PauseMusic(bool), 0x4101c0 ShowHelp, 0x4102d0 CloseHelp, 0x4105d0 ShowHint, 0x4107c0 ShowOptions,
0x4157e0 AskLeaveGame, 0x419cd0 RecordScore(StatKeeper*).

Other classes: Lex (+0x68 app, +0x80/+0x84 target x/y int, +0x88/+0x90 position doubles, +0xb0, +0xb4 anim,
+0xc5 danger; 0x42d320 PlayAnim, 0x42d3c0 SetDanger, 0x42d680 StopAnim, 0x42d6c0 Say, 0x42d8d0
ShowDefinition, slots 47/48 Save/Load(FILE*)); Lex inline helpers emitted in this module: 0x403ab0
`Lex::IsMoving()`, 0x404340 `Lex::IsBusy()`. OverlayWidget (0x430c20 ctor; +0x5c EffectMgr, +0x60 word balloon
(slot 49 = hide), +0x68/+0x6c/+0x70/+0x78; 0x430a00 IsSliding, 0x430a20 AddEffect, 0x430a70 GetEffects(type),
0x430a90 HasEffects(type), 0x430ab0/0x430b20 Save/Load). EffectMgr 0x4201c0 ctor, 0x41f930 Clear, 0x41f990
Update, 0x4207f0 AddEffect, 0x41faa0/0x420b70 Save/Load. EmberListMgr 0x421070 ctor, 0x4202d0 Clear, 0x420d60
NewEmber, 0x420e00 AddEmber(type, x, y, vx, vy, bool, a38, a40), 0x420fd0 Update, 0x41ff80/0x420e70 Save/Load;
Ember fields +0x10 x, +0x18 y, +0x20 vx, +0x28 vy, +0x30, +0x38, +0x40, +0x48, +0x4c, +0x58 type. Sparkle
0x41f2b0 ctor (+0x10/+0x18 x/y). FlyingPieceList 0x420920 ctor (reads Piece +0x8/+0x10/+0x18/+0x20/+0xa8/
+0xe8, writes Board+0x1b3 and +0x368). Points 0x431a50 ctor(app, font, text, x, y, life, flag) (+0x5c life,
+0x80, +0xc). LevelUpWidget 0x42bd50 (reads +0x357, +0x188, +0x18c...). Definitionator 0x41d2b0 Lookup.
Buffer WriteByte/WriteShort/WriteLong/WriteNumBits/ToWebString/FromWebString/Clear.

### 6. Files: players\<name>\classic.dat and action.dat

Path (Save 0x406c40, Load 0x408ab0, Delete 0x4078b0): `"players" + "\\" + profile name (app+0x1178 ->
+0x10)`, then `"\\"` if the name is non-empty, then `"classic.dat"` (mode 0) or `"action.dat"` (mode 1);
other modes fail. Save first `_mkdir("players")` and `_mkdir(dir)` (EEXIST ok). Binary, little-endian, raw
`fwrite` of fields; "str" = int32 length + bytes (no terminator); bool = 1 byte; double = 8 bytes.

#### 6.1 Record (version 5)

Save prologue (not in the file): mEffectMgr.Clear, mEmberMgr.Clear, overlay EffectMgr.Clear, Lex StopAnim,
mShownScore = mGameStats.mScore, `helpOpen = !app+0x12e7 && app+0x116c`, CloseHelp, Pause(true),
Pause(false) (banks the level clock).

| # | Bytes | Field |
|---|---|---|
| 1 | 4 | version = 5 |
| 2 | str | app->mProductVersion (+0x3b8) |
| 3 | 4 | app->mBuildNum (+0x94) |
| 4 | str | app->mBuildDate (+0x98) |
| 5 | 4 | mode (0/1) |
| 6 | 1 | app+0x12e8 (hints enabled) |
| 7 | str | mRand.Serialize() |
| 8 | 56 x (1 [+191]) | grid [row 0..7][col 0..6] in memory order: present byte, then Piece::Save (191 bytes) if present |
| 9 | 4,4,4 | mLevelSeconds, mTotalSeconds, mLevelSecondsBanked |
| 10 | var | OverlayWidget::Save |
| 11 | var | EffectMgr::Save, EmberListMgr::Save |
| 12 | 4,4 | mState, mSelectMode |
| 13 | 4 + n*8 | selection count, then per piece (row, col) |
| 14 | 8,1,1,1,1,1 | mFireMeter, mSkipRandomFire, mBurnedDown, mUnused1b2, mPiecesFlying, mFlameSoundPlayed |
| 15 | 4, 8,8,8 | mRewardStreak, mScrambleProgress, mLevelProgress, mShownProgress |
| 16 | var x2 | WordScore::Save mWord, mLastWord |
| 17 | 16, 8, 1, 4 | mLastWordColor (raw 4 ints), mLastWordAlpha, mNewHighScore, mBonusWordsGiven |
| 18 | str | mBonusWord |
| 19 | var x2 | StatKeeper::Save game, level |
| 20 | str,4,str,4 | mUnusedStr2b0, mUnused2cc, mUnusedStr2d0, mUnused2ec |
| 21 | 4,4,4 | mUpSoundIdx, mUpSoundCount, mUpSoundDelay |
| 22 | 1 [+4+4] | mGameOverPiece present, then (row, col) |
| 23 | 4,4,8,4,4,4,4 | mDeadSparkDelay, mBoardBurnDelay, mBoardBurnAlpha, mBlinkDelay, mBlinkRepeats, mGameOverMusicDelay, mFireLineDelay |
| 24 | 1 [+4+4] | mFireLinePiece present, (row, col) |
| 25 | 8, 4 x8 | mFireLineAlpha, mFireLineScroll, mFireTileCount, mActionFireTimer, mActionCountdown, mWarnBeeps, mWarnBeepDelay, mHintDelay, mHintId |
| 26 | 1 + var | Lex present byte, then **always** Lex slot 47 Save(FILE*) (null deref if absent) |
| 27 | 1,1,1,4,1,1,4,8 | mLexGreetPending, mLexDangerTalkReady, mLevelUpPending, mBlinkFrames, mBurpPending, mBurping, mBurpDelay, mBurpAlpha |
| 28 | 4,1,4,4,4,4,4,8 | mHighlightKey, mCtrlDown, mSubmitBlinkWords, mCols, mRows, mPointsAwarded, mShownScore, mScoreScale |
| 29 | 1,1,4,4,8,4,1,4,4,4,4 | mSummaryShown, mPiecesMoving, mLevel, mPauseCount, mPauseFade, mUnused3d8, mPauseNoDialogs, mBestWordThisLevel, mBestWordThisGame, mUnused3e8, mUnused3ec |
| 30 | str | mGameLog.ToWebString() |
| 31 | 1 | helpOpen |

Load (returns false if the file is missing or version <= 1): calls CloseHelp first; reads the same order into
the existing (freshly constructed) board. Version 2..4: `OutputDebugString("BookWorm: loading and converting
obsolete saved game.\n")`, fields 2-4 absent; version <= 2: mLevelSeconds absent; version <= 3: field 31
absent. Fields 2-4 are discarded; field 5 goes to **app+0x1254**, field 6 to app+0x12e8; field 7 ->
`mRand.SRand(state string)`. Grid: each slot's old piece is deleted (vtable slot 0) and replaced by the loaded
piece or NULL. Field 10-11: overlay Load; mEffectMgr Clear + Load; mEmberMgr Clear + Load. Selection: list
cleared, pieces looked up as `mBoard[row][col]`, NULL skipped. Lex block: if the byte is set, the current Lex
is removed and deleted, a new Lex(app) is created, Resize(0,0,width of IMG(+0x1d8)), AddWidget, Lex slot 48
Load(FILE*), SetVisible(mVisible). Overrides after reading: mHighlightKey = 0, mCtrlDown = 0,
mSubmitBlinkWords = 3. Field 30: if length > 0: mGameLog.Clear(), FromWebString. Post-load: mPlayCounted = 0;
if Lex danger and !IsPlaying(4): danger music as in 4.7 step 5; action: gActionSpeed (dead); if
mSummaryShown: ShowSummary() keeping mLevelSeconds/mTotalSeconds; if app+0x12e7 or helpOpen: ShowHelp().
App calls (other agent): resume = Load(app+0x1258) then **DeleteSavedGame** (the save is one-shot); quit
while IsInProgress(): Save(mode) (0x40ffa0, 0x417080).

#### 6.2 Piece::Save / Load (0x403050 / 0x403370), 191 bytes

mType 4, mX 8, mY 8, mFlySpeed 8, mVY 8, +0x28 4, +0x2c 4, mCol 4, mRow 4, +0x38 1, mScale 8, +0xc0 4, mLetter 1,
mNextLetter 1, mSelected 1, mBurning 1, mBurnProgress 4, mBurnDelay 4, mPendingFire 1, mBurnCountdown 4, mFlicker
1, mFlickerFrame 4, mShaking 1, mShakeAmount 8, mFlipping 1, mFlipProgress 8, +0x3c 4, +0x40 1, +0x41 1, +0x48 8,
+0x50 8, +0x58 4, +0x5c 4, +0x60 1, +0x61 1, +0x64 4, +0x68 4, +0x6c 4, +0x70 1, +0x78 8, +0x80 8, +0x88 4, +0x8c
4, +0x90 4, +0x94 4, +0x98 1, +0x9c 4, +0xa0 4.

#### 6.3 WordScore and StatKeeper records

WordScore (0x404fe0 / 0x406180): str mWord, mValid 1, mTypeCount 6x4, mNumPieces 4, mScore 4.
StatKeeper (0x404e10 / 0x405fa0): +0x08..+0x34 as 12 int32 (mScore, mWords, mLetters, mTiles, Fire, Green,
Gold, Sapphire, Diamond, Burned, Scrambles, BonusWords), str mBestWord, mBestWordScore, str mLongestWord,
mLongestWordScore.

### 7. The game log (Board+0x160 Buffer)

Bit-packed with `Buffer::WriteNumBits(value, bits)`; read elsewhere (online score submission, dropped) and
saved as a web string. Header (ctor): WriteByte(3), WriteByte(mode), WriteLong(mRand seed), WriteLong(time()),
then a level record. Records start with a 4-bit tag: 0xe level start + WriteShort(level); 0xd word +
WriteLong(score) + 4-bit length + 5-bit letters; 0xc tile types (3 bits per tile) following a word with special
tiles; 0xb bonus word + WriteLong(points); 0x0 game over + WriteLong(score); 0xf written by another module
(0x4251xx). Port: keep it (cheap) or drop with the online code.

### 8. Static helpers and neighbours in the range

- 0x402f60 `IsAdjacent(Piece*, Piece*)`, 0x402fd0 `SetActionSpeed(double)` (writes the dead 0x52e278): in
  the BigLex.cpp range of INVENTORY but Board statics (module boundary probably at 0x402f60).
- 0x40fe80 **WinMain** (`BookWorm.cpp` probably; between Board.cpp and BookWormApp.cpp): gHInstance; `app =
  new BookWormApp` (0x1348) -> 0x5347d4; ParseCmdLine(cmdline) (slot 34); mWidth = 640, mHeight = 480; Init
  (slot 16); `gGameRand = new MTRand(Sexy::Rand())`; Start (slot 36); Shutdown (slot 33); delete app (slot
  14); delete gGameRand.
- 0x40ffa0 `BookWormApp::EndGame()` (called by App::Shutdown 0x410ca0): KillDialog(15); remove and
  SafeDelete app widgets +0x1164/+0x1168/+0x1170; if a board: if app+0x1330: app+0x1334++, app+0x1330 = 0;
  if board->IsInProgress() board->Save(app+0x1254); remove and SafeDelete the board; app+0x1160 = 0.
- 0x410090 `DrawShadowString(Graphics* g, int x, int y, const string& s, const Color& c, bool shadow, const
  Color& sc)` (cdecl, used by 0x424c30/0x425880): if shadow: SetColor(sc), DrawString(s, x+1, y+1); SetColor(c),
  DrawString(s, x, y).

#### 8.1 Drawing functions (owned by board_draw)

0x40a980 Board::Draw (slot 16), 0x40a160 DrawPieces, 0x4044f0 DrawPiece, 0x404380 DrawPieceBlank (action
pause), 0x405340 DrawSelectionArrows, 0x4056e0 DrawLevelText (`GT_BOARD_LEVEL_FORMAT`), 0x403c00 DrawProgressBar, 0x405580
DrawScore, 0x407c60 DrawLastWord (`GT_BOARD_POINTS_FORMAT`/`GT_BOARD_ZERO_POINTS`, sets mLastWordColor), and their private helpers 0x406560
(WordScore copy ctor), 0x408a70 (list push_back), 0x405e70 (string::resize). Fields they read are in 2.1
(mPauseFade, mBurnedDown, mLastWord, mLastWordAlpha, mScoreGlow, mShownScore, mShownProgress, mLevel,
mBurping/mBurpAlpha, mHighlightKey, mBlinkFrames, mFireLine*, mBoardBurnAlpha, Piece fields).

### 9. Not ported

- **Dictionary Upgrade** (Submit, unregistered and word length >= 9): DoDialog(17, modal, "Dictionary
  Upgrade!", text + app->mProdName + " Deluxe! ... register now?", "", YES_NO); sets app+0x37c mRegSource =
  "dictionary". Registered build (app+0x470 true): never.
- mTimesPlayed / app+0x12e0 counting (trial limit) in Submit/Scramble: harmless in the registered build;
  keep or drop with the app's trial logic.
- Game log upload (Buffer consumer in other modules). gActionSpeed (dead global), gLen6Gate (dead),
  CalcScoreByLength (dead), mDeadSparkDelay (dead), hint cases 0/3/5-9 (unreachable).

### 10. Open questions

1. Piece fields +0x28..+0xa0 (and +0xc0) are saved but unused in Board.cpp: check FlyingPieceList and
   board_draw; mUnk3c initialised to 1.
2. StatKeeper +0x24/+0x28 (sapphire/diamond used) are never zeroed (heap garbage) and never reset per level:
   who displays them (LevelUpWidget?) and what does the original show? Port: zero.
3. The `GT_HINT_SUBMIT_KEYBOARD_HEADER` hint promises ENTER submits, but no Enter handler reaches Board::Submit in this
   build (and the hint is never scheduled).
4. Board+0x1b2 is uninitialised but saved; Board+0x3dc/+0x3d8/+0x2b0/+0x2cc/+0x2d0/+0x2ec/+0x3e8/+0x3ec have
   no readers here (check other modules).
5. Refill's fire-tile placement compares the column's first empty row with the top row (a fire tile only
   spawns in a column that lost exactly one tile); looks like a bug but it is what ships.
6. Board::SetDisabled passes its argument to the buttons' and Lex's **SetVisible** (slot 4). Verified in the
   code; behaviourally odd.
7. Module boundary: 0x402f60/0x402fd0 (Board helpers) sit in BigLex.cpp's range; 0x40fe80/0x40ffa0/0x410090
   probably belong to a main file (BookWorm.cpp).
8. Lex animation ids, Lex+0xb0 and the overlay's +0x60 "word balloon" slot 49 are named from their use here
   only; the Lex and overlay agents own their meaning.

---

## Part 3. Board drawing and effects (Board drawing, Effect.cpp, Points.cpp)

Scope: `Board::Draw` (Board vtable slot 16, 0x40a980) and every Board.cpp helper reached only from it;
all of Effect.cpp (0x41e880-0x421860: Effect, Sparkle, FlyingPieceList, EffectMgr, Ember, EmberListMgr);
all of Points.cpp (0x431950-0x431b50). Symbols: `board_draw_symbols.jsonl`; proposed header: `board_draw.h`.
Binary: `re/bookworm/bin/BookwormGame.exe` (image base 0x400000). Every `_ftol` operand, FPU comparison and
`.rdata` constant below was read in the disassembly (llvm-objdump), not only in Ghidra's output.

Conventions used in this file:
- `ftol(x)` = MSVC `_ftol`: double to int, **truncation toward zero**. `a / b` on ints = C division
  (truncation toward zero; the binary uses `cdq; sub; sar` or `idiv`). `%` = C remainder (sign of the dividend).
- `IMG(n)` = `app->mImages[n]` = `*(Image**)(*(app+0x11ac) + 4n)`; the file is `images\` + name from the table
  at 0x52e410 (loader 0x4144b0, owned by the app group). Sizes are those of the staged files in `assets/bookworm/images`.
- `FONT(n)` = `*(ImageFont**)(app + 0x11b0 + 4n)`; descriptor `fonts\` + name from the table at 0x52e3c0.
- `TILESET(k)` = `*(MemoryImage**)(app + 0x1204 + 4k)`, the 26-letter tile strips built by the loading thread
  (0x41a8a0, app group; summary in section 1.3).
- `app` = `Board+0x5c` (BookWormApp*), `lex` = `Board+0x350` (Lex*), `cnt` = `Board::mUpdateCnt` (Widget +0x4c,
  incremented once per Board::Update; the game updates at 100 Hz, so 1 tick = 10 ms).
- "colorize" = `Graphics::SetColorizeImages`; "additive" = `SetDrawMode(1)`, "normal" = `SetDrawMode(0)`.
  When colorize is off, `DrawImage` ignores the Graphics colour (2004 Graphics::DrawImage uses White).
- `DrawString(s, x, y)`: y is the baseline (ImageFont), same as the port.

### 0. Summary

| class | sizeof | vtable | allocation site(s) |
|---|---|---|---|
| Effect | 0x10 | 0x4fc450 (8 slots) | abstract in practice (never allocated alone) |
| Sparkle : Effect | 0x48 | 0x4fc474 | `new(0x48)` 0x40c7c8 (Board::Update), 0x421477 (FlyingPieceList::Update), 0x420b70 (EffectMgr::Load) |
| FlyingPieceList : Effect | 0x20 | 0x4fc4a0 | `new(0x20)` in 0x40f5b0 (Board), 0x420b70 (EffectMgr::Load) |
| EffectMgr | 0x1c | 0x4fc498 (1 slot: deleting dtor) | `new(0x1c)` 0x40ae90 (Board ctor, Board+0x70), 0x430c20 (OverlayWidget ctor) |
| Ember (no vtable) | 0x60 | none | `new(0x60)` 0x421070 (pool), 0x420d60, 0x420e70 |
| EmberListMgr | 0x2c | 0x4fc4c4 (1 slot: deleting dtor) | `new(0x2c)` 0x40ae90 (Board ctor, Board+0x74, pool of 1000) |
| Points : Widget | 0x84 | 0x500e0c (47 slots; overrides 0 dtor, 16 Draw) | `new(0x84)` 0x409970, 0x409b70 (Board) |
| WordScore (read only here) | 0x48 | (Board group) | copy-constructed on the stack by Board::DrawWord |

Board::Draw draw order (all coordinates are Board-local; the Board widget covers the 640x480 screen):

1. background `IMG(17) backdrop_board` (or `IMG(18) backdrop_board_scorched` when scorched) at (0,0);
2. Lex's shadow; 3. "LEVEL n" text; 4. progress bar; 5. score; 6. the current word and its points;
7. Lex's burp smoke; 8. the 7x8 tiles, then the tile glows, fire animation, selection arrows and "X" marks;
9. EffectMgr effects (sparkles, flying tiles); 10. embers/particles. Lex itself is a separate widget.

No random number is drawn by any draw function. Effect.cpp draws from the EffectMgr's own MTRand only in
FlyingPieceList::Update (section 3.7); the effect managers' constructors each draw one `Sexy::Rand()` for their seed.

### 1. Shared data used by the drawing code

#### 1.1 Images

| idx | app+0x11ac off | file | size | cels / use |
|---|---|---|---|---|
| 17 | 0x044 | backdrop_board.gif | 640x480 | board background |
| 18 | 0x048 | backdrop_board_scorched.gif | 640x480 | background when Board+0x1b1 (scorched) |
| 58 | 0x0e8 | fx_ember_sm | 250x25 | ember type 3, 10 horizontal cels 25x25 |
| 59 | 0x0ec | fx_ember_med | 400x40 | ember type 4, 10 cels 40x40 |
| 60 | 0x0f0 | fx_ember_lrg | 500x50 | ember type 5, 10 cels 50x50 |
| 61 | 0x0f4 | fx_ember_smoke_sm | 250x25 | ember type 6 |
| 62 | 0x0f8 | fx_ember_smoke_med | 400x40 | ember type 7, and the overlay of type 4 |
| 63 | 0x0fc | fx_ember_smoke_lrg | 500x50 | ember type 8 |
| 67 | 0x10c | fx_spark_sm | 5x30 | ember type 0, 6 **vertical** cels 5x5 |
| 68 | 0x110 | fx_spark_med | 13x91 | ember type 1, 7 vertical cels 13x13 |
| 69 | 0x114 | fx_spark_lrg | 17x119 | ember type 2, 7 vertical cels 17x17 |
| 70 | 0x118 | fx_smoke_sm | 32x32 (mask only) | Lex burp smoke; ember type 10 |
| 71 | 0x11c | fx_flame_lrg | 500x60 | ember type 9, W/H = 8 horizontal cels 60x60 |
| 75 | 0x12c | tile | 57x57 | blank tile (action-mode pause view) |
| 76 | 0x130 | tile_hilite | 57x57 | |
| 77 | 0x134 | tile_fire | 57x282 | only the top 57x57 is ever drawn |
| 78 | 0x138 | tile_fire_hilite | | |
| 79 / 80 | 0x13c / 0x140 | tile_gold / tile_gold_hilite | 57x57 | |
| 81 / 82 | 0x144 / 0x148 | tile_green / tile_green_hilite | 57x57 | |
| 83 / 84 | 0x14c / 0x150 | tile_sapphire / tile_sapphire_hilite | 57x57 | |
| 85 / 86 | 0x154 / 0x158 | tile_diamond / tile_diamond_hilite | 57x57 | |
| 88 | 0x160 | tile_gemhilite | 57x57 | additive overlay on gem tiles |
| 89 | 0x164 | tile_spin_fx | 57x399 | 7 vertical cels 57x57 (letter-change spin) |
| 90 | 0x168 | tile_x | 57x57 | "too long" mark on selected tiles |
| 91 | 0x16c | font_tile_letters | 59x1534 (mask only = white) | 26 vertical cels 59x59 ('A'..'Z'; 'Q' cel shows "Qu") |
| 92 | 0x170 | font_tile_digits | 15x150 | 10 vertical cels 15x15 (values 1..10) |
| 93 | 0x174 | tile_green_glow | 71x71 | |
| 94 | 0x178 | tile_gold_glow | 80x80 | |
| 95 | 0x17c | tile_fire_glow | 71x71 | |
| 96 / 97 | 0x180 / 0x184 | tile_fire_fx1 / tile_fire_fx2 | 59x295 | 295/57 = 5 vertical cels, 57x57 used |
| 98 | 0x188 | arrows | 75x83 | 3x2 cels of 25x41 (W/3, H/2) |
| 102 | 0x198 | lex_shadow | 143x46 (mask only) | |
| 127..138 | 0x1fc..0x228 | progress_green, _lavender, _lime, _magenta, _multi, _navy, _orange, _purple, _red, _tan, _teal, _yellow | 193x30 | progress bar per level |
| 139 | 0x22c | progress_burnt | 193x30 | progress bar when scorched |
| 141 | 0x234 | sparkle_large | 560x40 | Sparkle default (`mImage = 0x8d`), 14 horizontal cels 40x40 |

Cel counts are computed at run time from the image sizes (Height/26, Height/10, Height/57, W/3, H/2, H/W or
W/H, image height as the square Sparkle cel); the sizes above give the values for the shipped files.

#### 1.2 Fonts

| n | app offset | descriptor | used for |
|---|---|---|---|
| 2 | 0x11b8 | Verdana14Bold | word display (> 10 chars), `GT_BOARD_POINTS_FORMAT` |
| 5 | 0x11c4 | Verdana18Bold | word display (<= 10 chars) |
| 10 | 0x11d8 | Lockergnome24 | `GT_BOARD_LEVEL_FORMAT` |
| 12 | 0x11e0 | GoldScore | score; Points "+%d" (Board 0x409970) |
| 13 | 0x11e4 | GoldScoreHeated | score glow overlay (invisible, see 2.5) |
| 14 | 0x11e8 | LockergnomeSmallColor | Points bonus messages (Board 0x409b70) |

#### 1.3 Tile sets (built by BookWormApp::LoadingThreadProc 0x41a8a0, app group)

`TILESET(k)`, k = 0..8, each a MemoryImage of (26 * 57) x 57. For letter i (0 = 'A'): the base image is drawn at
(57i, 0) (whole image, clipped to the strip), the letter cel i of IMG(91) (src (0, 59i, 59, 59)) at (57i, 0)
colorized grey (c,c,c,255), then the value digit cel v-1 of IMG(92) (15x15) at (57i + 54 - 15, 53 - 15) colorized
white, v = letter value table 0x4fa480 (A..Z = 1 4 4 3 1 5 3 5 1 8 7 2 4 2 1 4 7 2 1 2 2 5 6 8 6 10, 'Q' forced to
10). Board.cpp has an identical copy of the table at 0x4f9888 (`gLetterValues`, used for gem tiles).

| k | base image | letter grey c | k | base image | letter grey c |
|---|---|---|---|---|---|
| 0 | tile | 0 | 5 | tile_gold_hilite | 0xb0 |
| 1 | tile_hilite | 255 | 6 | tile_fire | 0 |
| 2 | tile_green | 0 | 7 | tile_fire_hilite | 255 |
| 3 | tile_green_hilite | 255 | 8 | tile_charred | 0xb0 |
| 4 | tile_gold | 0 | | | |

`app+0x1244` / `app+0x1248`: MemoryImage copies of IMG(83) tile_sapphire / IMG(85) tile_diamond (used by flying gems).

#### 1.4 Piece types (Piece+0xe8) as the drawing code treats them

0 normal, 1 green, 2 gold, 3 fire (burning red tile), 4 sapphire, 5 diamond. Highlighted ("hilite") variants are used
when `Piece+0xa6` (selected) is set or when the piece's letter equals the held letter key `Board+0x378`.

### 2. Board drawing (Board.cpp)

#### 2.1 `Board::Draw(Graphics* g)` 0x40a980, slot 16

1. `g->DrawImage(scorched ? IMG(18) : IMG(17), 0, 0)` (scorched = `Board+0x1b1`).
2. Non-virtual call of the base `Widget::Draw(g)` (0x401570, empty).
3. **Lex shadow**, if `lex+0xb0` (bool, ctor true; name: mShowShadow):
   - `W = IMG(102).width` (143); `dx = lex+0x80 - ftol(lex+0x88)`, `dy = lex+0x84 - ftol(lex+0x90)` (+0x80/+0x84 =
     Lex home x/y ints, +0x88/+0x90 = current x/y doubles, see Lex::Move 0x42d370);
     `d = sqrt((double)(dy*dy + dx*dx))` (int sum, then double sqrt); `d = min(d, 100.0)`;
     `f = 1.0 - d * 0.01`;
   - `x = ftol(lex+0x88 - 25.0)`, `x = max(x, 0)`, `x = min(x, 0xd0 - W)` (0..65); y = 0x17c (380);
   - colorize on, colour (0,0,0, ftol(f * 255.0)), `DrawImage(IMG(102), x, 380)`, colorize off.
4. `DrawLevelText(g)` 0x4056e0 (2.3).
5. `DrawProgressBar(g)` 0x403c00 (2.4).
6. `DrawScore(g)` 0x405580 (2.5).
7. `DrawWord(g)` 0x407c60 (2.6).
8. **Lex burp smoke**, if `Board+0x365` (bool): `t = Board+0x370` (double, 1.0 down to <0);
   colorize on, colour (255,255,255, ftol(t * 255.0)); `H = IMG(70).height` (32);
   `DrawImage(IMG(70), lex.mX + 100, lex.mY - ftol((1.0 - t) * 48.0) - H/2 + 0x32)`; colorize off.
   (Board::Update 0x40d488: started with t = 1.0, Lex anim 23 and sound 31 lex_burp when the countdown
   `Board+0x368` hits 0 after a fire tile was eaten; t -= 0.005 per tick; at t < 0.9 Lex anim 23 -> 22; flag
   cleared when t < 0. So the puff rises 48 px over 2 s and fades out.)
9. `DrawPieces(g)` 0x40a160 (2.7).
10. `EffectMgr::Draw(g)` on `Board+0x70` (3.4), then `EmberListMgr::Draw(g)` on `Board+0x74` (3.6).

#### 2.2 Graphics state

The helpers draw on `g` directly except where noted ("copy" = `Graphics(const Graphics&)` on the stack, used for
clip rects and translations). Colour and font set by one helper stay set for the next; colorize and draw mode are
always restored to off / normal. No helper reads colour/font state it did not set itself, so the leak is harmless.

#### 2.3 `Board::DrawLevelText(Graphics* g)` 0x4056e0

Only when not scorched. `a = 255 - ftol(Board+0x278 * 255.0)`, clamped to 0..255 (`+0x278` = word display fade,
so the level text fades out while a word is shown). Font FONT(10) Lockergnome24 (set on g).
`s = StrFormat(`GT_BOARD_LEVEL_FORMAT`, Board+0x3c4 + 1)` (`+0x3c4` = 0-based level), `w = font->StringWidth(s)`.
Shadow colour (0,0,0,a) at (0x71 - w/2, 0x4b) = (113 - w/2, 75); text colour (0x81,0x5d,0x34,a) = (129,93,52,a)
at (112 - w/2, 74).

#### 2.4 `Board::DrawProgressBar(Graphics* g)` 0x403c00

Only when `p = Board+0x1d0` (displayed progress, double) > 0.0.
- `lvl = Board+0x3c4`; if `Board+0x1c8` (target progress) < p: `lvl -= 1` (the bar drains in the previous level's
  colour after a level-up).
- image `IMG(0x8a - lvl % 12)`: level%12 = 0 yellow, 1 teal, 2 tan, 3 red, 4 purple, 5 orange, 6 navy, 7 multi,
  8 magenta, 9 lime, 10 lavender, 11 green; scorched: `IMG(139) progress_burnt`. (Quirk: lvl = -1 gives
  `IMG(0x8b)` = progress_burnt too.)
- `W = width`, `x = ftol(W * p + 16.0) - W`, `x = min(x, 16)`.
- Copy of g; `ClipRect(16, 0x90, W, H)`; `DrawImage(img, x, 0x90)` (y = 144). The bar slides in from the left.

#### 2.5 `Board::DrawScore(Graphics* g)` 0x405580

`s = StrFormat("%d", Board+0x39c)` (score). Colour (255,255,255,255), font FONT(12) GoldScore,
`x = 0x6c - w/2` (108 - w/2), `DrawString(s, x, 0x1f)` (y = 31).
Glow overlay when `Board+0x280` (score glow, double, 0..0.85; Board::Update raises it by 0.00375 per counting
step and lowers it by 0.00675 every 4 ticks) > 0: font FONT(13) GoldScoreHeated,
alpha = `ftol(min(Board+0x280, 0.5)) * 255` (truncation **before** the multiply), additive, same position, normal.
**Quirk:** min(glow, 0.5) <= 0.5 truncates to 0, so the overlay is always drawn with alpha 0, i.e. invisible.
Port: keep the call with alpha 0 (or skip it: same pixels). The font stays GoldScoreHeated on g afterwards.

#### 2.6 `Board::DrawWord(Graphics* g)` 0x407c60

Returns if scorched or `Board+0x278 <= 0.0`. Copies the WordScore at `Board+0x220` (copy ctor 0x406560) and the
word string `Board+0x228` (= WordScore::mWord, "QU" for a Qu tile) into locals.
1. Colour of the word, stored into `Board+0x268` (Color), only if the WordScore's word is not empty:
   - length < 3: (0xcc,0xad,0x6e) = (204,173,110,255);
   - else if `ws+0x28` (count of type-0 tiles) == `ws+0x40` (tile count): (255,221,123) (all plain tiles);
   - else the first of: diamond count `ws+0x3c` > 0 (255,255,255); sapphire `ws+0x38` > 0 (45,195,255);
     gold `ws+0x30` > 0 (255,220,0); green `ws+0x2c` > 0 (0,255,0); fire `ws+0x34` > 0 (255,100,0);
     none: colour unchanged. All with alpha 255 (overwritten below).
   (`ws+0x28 + 4*type` are per-type tile counts, see Board 0x406a77.)
2. If the word is not empty:
   - `ptsStr = sprintf(ws.mValid ? `GT_BOARD_POINTS_FORMAT` : `GT_BOARD_ZERO_POINTS`, ws+0x44)` (`ws+0x24` = valid word, `ws+0x44` = points);
   - `k = ftol(Board+0x278 * 255.0)`; `Board+0x274` (alpha of the word colour) = k > 127 ? 255 : 2k;
   - font = length > 10 ? FONT(2) Verdana14Bold : FONT(5) Verdana18Bold (decided on the full length);
   - if length > 12: resize the copy to 12; if its char [11] is 'Q', resize to 11;
   - `w = font->StringWidth(word)`; shadow colour (0,0,0, Board+0x274) at (113 - w/2, 0x41 = 65); text colour
     `Board+0x268` at (112 - w/2, 64);
   - only if valid: font FONT(2) Verdana14Bold, `w2 = StringWidth(ptsStr)`, shadow (0,0,0,k) (k, not the doubled
     alpha) at (113 - w2/2, 0x55 = 85), text `Board+0x268` at (112 - w2/2, 84). (`GT_BOARD_ZERO_POINTS` is never drawn.)

`WordScore::WordScore(const WordScore&)` 0x406560 (only caller DrawWord): copies +4, the string at +8, +0x24, the six
ints +0x28..+0x3c, +0x40, +0x44. Destroyed with 0x4051d0 (WordScore dtor body, shared).

#### 2.7 `Board::DrawPieces(Graphics* g)` 0x40a160

Four temporary `std::list<Piece*>`: A (fire glow), B (green glow), C (gold glow), D (unused, freed only).
1. `blank = (app+0x1254 == 1) && Board+0x3d0 > 0.0 && !scorched` (`app+0x1254` = game mode 1 = action,
   `Board+0x3d0` = pause fade 0..1, +0.05/tick while paused: the letters are hidden while an action game is paused).
2. Pre-pass over the selected list `Board+0x1a0` (std::list<Piece*>, size `+0x1a4`): `extra = NULL`; for each
   non-null piece: type 1 and extra == NULL -> extra = B; type 2, 4 or 5 and (extra == NULL or extra == B) -> extra = C.
3. Grid scan: rows r = 7 down to 0 (`Board+0x78 + 0x1c*r`, outer), columns c = 0..6 (`+4c`, inner) of
   `Piece* Board+0x78[8][7]` (row r+1 is the row below row r, see `Board::GetPieceBelow` 0x403bd0). For each
   non-null piece p:
   - `blank ? DrawPieceBlank(g, p) : DrawPiece(g, p)` (2.9, 2.8);
   - if !scorched and `p+0xb0` == 0: if type 3: append p to A, then `q = GetPieceBelow(p)` (grid[p.row+1][p.col],
     NULL for row 7); if q and `q+0xec` >= 1 append q to A. Else if `p+0xbc` (bool) append p to A. Else type 1 ->
     B; type 2/4/5 -> C; any other type: if extra and `p+0xa6` (selected) push_back p to extra. (Pieces can appear
     twice in A; they are then drawn twice.)
4. List B (green glow), for each piece in order: `v = 0xb0 - abs((cnt*2) % 256 - 0x80)` (48..176, period 128 ticks);
   colour Color(v,v,v) (alpha 255), colorize, additive; `o = (IMG(93).w - IMG(81).w) / 2` (= 7);
   `DrawImage(IMG(93), ftol(p.x) - o, ftol(p.y) - o)`; normal, colorize off. (p.x = Piece+0x8, p.y = Piece+0x10,
   doubles, tile top-left.)
5. List C (gold glow): `v = 0x80 - abs(cnt % 128 - 0x40)` (64..128, period 128); same with IMG(94) and
   `o = (IMG(94).w - IMG(79).w) / 2` (= 11).
6. List A (fire): `v = 0xff - abs((cnt*4) % 0x1fe - 0xff)` (0..255, period 127.5 ticks); additive; colorize,
   colour (v,v,v,v); `o = (IMG(95).w - IMG(77).w) / 2` (= 7); `DrawImage(IMG(95), ftol(p.x) - o, ftol(p.y) - o)`;
   colorize off. Then if p.type == 3, the flame animation on a copy of g in additive mode:
   `n = IMG(96).height / 57` (5); `f1 = ((cnt/6) % n) * 57`; `f2 = ((cnt/8) % n) * 57` (also modulo IMG(96)'s
   count); `X = ftol(p.x)`, `Y = ftol(p.y)`; `ClipRect(X, Y, 57, 57)`;
   `DrawImage(IMG(96), X, Y, Rect(0,f1,57,57))`; `DrawImage(IMG(97), X, Y, Rect(0,f2,57,57))`;
   `DrawImage(IMG(96), X+28, Y, Rect(0,f1,28,57))`; `DrawImage(IMG(96), X, Y, Rect(28,f1,28,57))`
   (fx1 a second time with its halves swapped); then normal on g.
7. If !scorched and `Board+0x1a4` (selection size) != 0: `DrawSelection(g)` (2.10).

#### 2.8 `Board::DrawPiece(Graphics* g, Piece* p)` 0x4044f0

`L = (int)(signed char)p+0xa4` (letter, 'A'..'Z'), `set = TILESET(0)`.
- **Scorched**: `set = TILESET(8)` (charred), default draw with L.
- `hl = (L == Board+0x378)` (held key, uses +0xa4 even after a spin).
- **Spin frame**: if `p+0xd8` (piece spinning): `t = p+0xe0`; else if `Board+0x194 == 7` (board-wide spin state) and
  type != 3: `t = Board+0x1c0`; else no spin (frame = 0, go to the type switch with L).
  `frame = ftol((t * 4.75 - (p.col + p.row) * 0.25) * 8.0)` (`p+0x30` col, `p+0x34` row; a diagonal wave:
  t rises 0.01/tick, so frame = trunc(38t - 2(col+row)); each tile shows frames 1..7 for about 18-19 ticks and
  every tile is past frame 8 by t = 1.0).
  - in the `p+0xd8` case, if frame >= 8 and type == 3: `set = TILESET(6)`;
  - `L = frame > 0 ? p+0xa5 (new letter) : p+0xa4`;
  - if 0 < frame < 8: `DrawImage(IMG(89) tile_spin_fx, ftol(p.x), ftol(p.y), Rect(0, 57*frame - 57, 57, 57))`
    and return;
  - if `p+0xd8`: default draw with `set` and L (no hilite, no gem drawing; a spinning fire tile shows the normal
    set until frame 8);
- **Type switch** (not spinning, or the board-wide spin outside frames 1..7):
  - 0: `set = (sel || hl) ? TILESET(1) : TILESET(0)`; 1: TILESET(3)/(2); 2: TILESET(5)/(4); 3: TILESET(7)/(6)
    (`sel` = `p+0xa6`); other types > 5: TILESET(0) default; then the **default draw**:
    `DrawImage(set, ftol(p.x), ftol(p.y), Rect((L - 'A') * 57, 0, 57, 57))`.
  - 4 (sapphire) / 5 (diamond), drawn from parts: `img = sel||hl ? IMG(84) : IMG(83)` (5: IMG(86)/IMG(85));
    `SetColor(255,255,255,0xb0)` (**no effect**: colorize is off) and `DrawImage(img, X, Y)`;
    letter: `h = IMG(91).height / 26`, colour (0,0,0,255), colorize, `DrawImage(IMG(91), X, Y,
    Rect(0, (L-'A')*h, IMG(91).width, h))`, colorize off;
    digit: `v = (L == 'Q') ? 10 : gLetterValues[L - 'A']` (0x4f9888), `h2 = IMG(92).height / 10`,
    colour (255,255,255,255), colorize, `DrawImage(IMG(92), X - IMG(92).width + 0x36, Y - h2 + 0x35,
    Rect(0, (v-1)*h2, IMG(92).width, h2))`, colorize off;
    additive `DrawImage(IMG(88) tile_gemhilite, X, Y)`, normal. (X = ftol(p.x), Y = ftol(p.y).)

#### 2.9 `Board::DrawPieceBlank(Graphics* g, Piece* p)` 0x404380

Action-mode pause view: the plain tile image without letter. `hl = sel || (L == Board+0x378)`.
Image: type 0 `hl ? IMG(76) : IMG(75)`; 1 IMG(82)/IMG(81); 2 IMG(80)/IMG(79); 3 IMG(78)/IMG(77);
4 IMG(84)/IMG(83); 5 IMG(86)/IMG(85); other types IMG(75).
`DrawImage(img, ftol(p.x), ftol(p.y), Rect(0,0,57,57))`; types 4/5 then additive `DrawImage(IMG(88), X, Y)`, normal.

#### 2.10 `Board::DrawSelection(Graphics* g)` 0x405340

Walks the selected list `Board+0x1a0` in order with a letter count `n = 0`. For each non-null piece p:
- `n += (p.letter == 'Q') ? 2 : 1`;
- **arrow to the next piece** q (next node), if q's node is not the end, `n < 12`, q != NULL and
  (q.letter != 'Q' or `n < 11`): `dx = q.col - p.col`, `dy = q.row - p.row`; if dx != 0 and dy == 0:
  `dy = (p.col % 2 == 0) ? 1 : -1` (odd columns sit half a tile lower); `img = IMG(98) arrows`,
  `cw = W/3` (25), `ch = H/2` (41). On a copy of g: `Translate(ftol(p.x) + (57 - W)/2, ftol(p.y) + (57 - H)/2)`
  (= -9, -13), `ClipRect((dx + 1) * cw, (dy >= 0 ? 1 : 0) * ch, cw, ch)`, `DrawImage(img, 0, 0)`.
  The arrows image is a 3x2 sheet laid around the tile centre; the cel in the neighbour's direction is shown.
- **too-long mark**: if `app+0x470` (mIsRegistered) then `n > 12`, else `n > 8`:
  `DrawImage(IMG(90) tile_x, ftol(p.x), ftol(p.y))`. Registered build: threshold 12.

#### 2.11 `DrawShadowedString` 0x410090 (not reached from Board::Draw)

A cdecl helper at the end of Board.cpp, used by FameWidget (0x424c30, 0x425880):
`(Graphics* g, int x, int y, const std::string& s, const Color& colour, bool shadow, const Color& shadowColour)`:
if shadow: `shadowColour.GetAlpha()` (0x496c00, result unused), SetColor(shadowColour), DrawString(s, x+1, y+1);
then SetColor(colour), DrawString(s, x, y). Listed here because it is a draw helper in Board.cpp.

### 3. Effect.cpp

#### 3.1 Effect (0x10)

| off | type | name | meaning | init |
|---|---|---|---|---|
| +0x00 | vfptr | | 0x4fc450 | |
| +0x04 | EffectMgr* | mEffectMgr | owner (debug string "warning: (theEffectMgr == NULL)!\n" when NULL) | ctor arg |
| +0x08 | int | mUpdateCnt | ++ by Effect::Update | 0 |
| +0x0c | bool | mDead | removed and deleted by EffectMgr::Update | false |

Vtable (Effect / Sparkle / FlyingPieceList):

| slot | Effect | Sparkle | FlyingPieceList | meaning |
|---|---|---|---|---|
| 0 | 0x41e950 | same | same | `DeleteEffect(Effect* e)`: `if (e) delete e` (thiscall, ignores this; always called as `e->DeleteEffect(e)`) |
| 1 | 0x41e8c0 | 0x41e970 | 0x421150 | `Update()` (Effect: `++mUpdateCnt`; folded with FontData::Ref) |
| 2 | 0x401570 (empty) | 0x41e9e0 | 0x41fba0 | `Draw(Graphics*)` |
| 3 | 0x41f260 (-1) | 0x4ba8e0 (0, shared stub) | 0x4201b0 (1) | `int GetType()` |
| 4 | 0x451af0 | same | same | `bool IsDead()` = mDead (folded getter) |
| 5 | 0x41e8d0 | 0x41eb00 | 0x41fee0 | `Save(FILE*)` |
| 6 | 0x41e920 | 0x41eba0 | 0x4209f0 | `Load(FILE*)` |
| 7 | 0x41f280 | 0x41f8a0 | 0x4202a0 | scalar deleting destructor |

- 0x41e880 `Effect(EffectMgr*)`; 0x41f270 `~Effect` body (resets the vfptr).
- `Effect::Save` 0x41e8d0: writes `int type = GetType()` (4), mUpdateCnt (4), mDead (1).
- `Effect::Load` 0x41e920: reads mUpdateCnt (4), mDead (1) (the type was consumed by EffectMgr::Load).

#### 3.2 Sparkle : Effect (0x48)

| off | type | name | init | meaning |
|---|---|---|---|---|
| +0x10 | double | mX | 0 | top-left of a 57x57 box the sparkle is centred in |
| +0x18 | double | mY | 0 | |
| +0x20 | double | mVX | 0 | added to mX per update |
| +0x28 | double | mVY | 0 | |
| +0x30 | double | mGravity | 0 | added to mVY per update |
| +0x38 | int | mDelay | 0 | updates to wait before moving/animating |
| +0x3c | bool | (unknown) | false | only saved/loaded |
| +0x40 | int | mImage | 0x8d (141 sparkle_large) | app image index |
| +0x44 | int | mFrame | 0 | 0..13 |

- 0x41f2b0 `Sparkle(EffectMgr*)`; 0x41f8a0 deleting dtor.
- `Update` 0x41e970: Effect::Update (mUpdateCnt++); if `mDelay > 0`: `mDelay--`, return.
  `mX += mVX; mVY += mGravity; mY += mVY;` if `mUpdateCnt % 8 == 0`: `mFrame = (mFrame + 1) % 14`; if
  `mFrame % 14 == 0` mDead = true; then `app->mBoard->MarkDirty()` (`mgr->mApp + 0x1160`, Widget slot 46).
  So a sparkle lives 14 frames x 8 ticks = 112 updates after its delay.
- `Draw` 0x41e9e0: `img = IMG(mImage)`, `s = img.height` (40); `o = (57 - s) / 2` (8);
  `x = ftol(mX) + o`, `y = ftol(mY) + o`; copy of g, `ClipRect(x, y, s, s)`, additive, colour (255,255,255,200),
  colorize, `DrawImage(img, x - mFrame * s, y)`. Drawn during the delay too (frame 0, start position).
- `Save` 0x41eb00: Effect::Save, then mX, mY, mVX, mVY, mGravity (8 each), mDelay (4), +0x3c (1), mImage (4),
  mFrame (4). `Load` 0x41eba0: the same order.

#### 3.3 FlyingPieceList : Effect (0x20)

The selected tiles of a submitted word flying into Lex's mouth.

| off | type | name | init | meaning |
|---|---|---|---|---|
| +0x10 | std::list<Piece*> | mPieces | copy of the non-null entries of the ctor list | head +0x14, size +0x18 |
| +0x1c | int | mLastChompCnt | mUpdateCnt (0) | mUpdateCnt of the last "chomp" sound |

- 0x420920 `FlyingPieceList(EffectMgr* mgr, std::list<Piece*>* pieces)` (pieces may be NULL; used by Load).
- 0x4200f0 dtor body: deletes every remaining piece (Piece slot 0 = deleting dtor), clears the list.
- `Update` 0x421150 (`board = mgr->mApp->mBoard`, `lex = board+0x350`):
  1. Effect::Update (mUpdateCnt++).
  2. If mPieces is empty: mDead = true, `Lex::PlayAnim(lex, 22, false)` (0x42d320), return.
  3. If the first piece is non-null, its x < 233.0 and (`!Lex::IsBusy(lex)` (0x404340: anim -1 and at home) or
     `lex+0xb4` (current anim) == 19): `Lex::PlayAnim(lex, 21, false)` (mouth opens).
  4. For each node in order (`prev` = piece of the previous node):
     - target: if the node is the current `begin()` (re-read each iteration, so after the first piece is removed the
       next one targets Lex in the same tick): `tx = lex.mX + 0x22`, `ty = lex.mY + 0x2c`; else
       `tx = ftol(prev.x)`, `ty = ftol(prev.y)` (prev may be a piece that arrived this tick).
     - `p.scale (Piece+0xa8) *= 0.98`; `k = ftol(p.scale * -57.0)`; `tx -= k; ty -= k` (i.e. + trunc(57*scale)).
     - if `p.x != (double)tx`: `p.x = ftol((double)tx - p.x) * p.fx + p.x`; `p.fx (Piece+0x18) += 0.003`;
       `board+0x1b3 = true`; `board->MarkDirty()`.
     - same for y with `p.fy (Piece+0x20)`.
     - **arrived** if `abs(ftol(p.y - ty)) < 2` and `abs(ftol(p.x - tx)) < 2`: if type 3: `board+0x368 = 50`
       (burp countdown); `p.fx = p.fy = 0.0`; move p to a temporary list, erase the node; if
       `mUpdateCnt - mLastChompCnt >= 4`: `app->PlaySample(18)` (chomp) and `mLastChompCnt = mUpdateCnt`.
     - **else** (in flight): type 3 and `mUpdateCnt % 3 == 0`: one spark ember (RNG, 3.7) from
       `board->mEmberListMgr (+0x74)->NewEmber()`: type 0; x = r1 + p.x; y = r2 + p.y;
       vx = (r3 - 500.0) * 0.00125; alpha (+0x38) = 0.95; vy = (r4 - 1000.0) * 0.0006666666666666666;
       decay (+0x40) = r5 * 0.001 * 0.01 + 0.006 (gravity stays 0.01, not looping).
       Types 4/5 and `mUpdateCnt % 6 == 0`: `new Sparkle(mgr)`, mX = (r1 + p.x) - 28.0, mY = (r2 + p.y) - 28.0
       (other fields default: sparkle_large, no motion), `mgr->AddEffect(s)`.
  5. Delete every piece of the temporary list (Piece slot 0, flag 1).
- `Draw` 0x41fba0, for each non-null piece: `s = ftol(p.scale * 57.0)`, dest `Rect(ftol(p.x), ftol(p.y), s, s)`,
  fast stretch on:
  - type 4 / 5: `DrawImage(app+0x1244 / app+0x1248 (gem copies), dest, Rect(0,0,57,57))`; letter:
    `h = IMG(91).height / 26`, colour (0,0,0,255), colorize, `DrawImage(IMG(91), dest, Rect(0, (L-'A')*h,
    IMG(91).width, h))` (stretched to s x s), colorize off;
  - else `DrawImage(TILESET(k), dest, Rect((L-'A')*57, 0, 57, 57))` with k = 7 (type 3), 3 (type 1), 5 (type 2),
    1 (anything else): the hilite sets;
  - fast stretch off.
- `Save` 0x41fee0: Effect::Save, `int count` (list size), per node `u8 present` + `Piece::Save` (0x403050, Board
  group) when present, then mLastChompCnt (4). `Load` 0x4209f0: Effect::Load, count, per entry u8, when set
  `new Piece` (0xf0, ctor 0x403f20) + `Piece::Load` (0x403370) and push_back; then mLastChompCnt.

#### 3.4 EffectMgr (0x1c)

| off | type | name | init |
|---|---|---|---|
| +0x00 | vfptr | | 0x4fc498 |
| +0x04 | BookWormApp* | mApp | ctor arg |
| +0x08 | MTRand* | mRand | `new MTRand(Sexy::Rand())` (0x9c4) |
| +0x0c | int | mUpdateCnt | 0, ++ per Update |
| +0x10 | std::list<Effect*> | mEffects | head +0x14, size +0x18 |

| address | method | behaviour |
|---|---|---|
| 0x4201c0 | `EffectMgr(BookWormApp*)` | list, mApp; `seed = Sexy::Rand()` (global gMTRand, 1 draw); `mRand = new MTRand(seed)` (SAF 1.2 seeding); clears the list; mUpdateCnt = 0 |
| 0x420060 | `~EffectMgr` body | `delete mRand` (plain operator delete), Clear(), frees the list |
| 0x420270 | deleting dtor (slot 0) | |
| 0x41f930 | `Clear()` | for each node: `delete effect` (slot 7, flag 1), unlink and free the node, size-- |
| 0x41f990 | `bool Update()` | `had = size != 0`; for each node: `e->Update()`; if `e->IsDead()`: `e->DeleteEffect(e)` and erase the node; then `mUpdateCnt++`; returns had |
| 0x41fa10 | `Draw(Graphics*)` | `e->Draw(g)` for each node in order (no NULL check) |
| 0x4207f0 | `AddEffect(Effect*)` | push_back |
| 0x41fa50 | `bool HasEffectOfType(int)` | true if any non-null effect's GetType() equals the argument |
| 0x420830 | `std::list<Effect*>* GetEffectsOfType(int)` | NULL if empty; else `new std::list` (0xc) of the matching effects (caller deletes) |
| 0x41faa0 | `Save(FILE*)` | section 6 |
| 0x420b70 | `Load(FILE*)` | section 6 |

Users: Board (+0x70: Update in Board::Update 0x40bd40, Draw in Board::Draw, Clear/Save/Load in the save code
0x406c40/0x408ab0) and OverlayWidget (0x430a20-0x431080: AddEffect, GetEffectsOfType, HasEffectOfType, Save, Clear+Load,
Update, Draw).

#### 3.5 Ember (no vtable, 0x60)

| off | type | name | reset value (0x41ec50) | meaning |
|---|---|---|---|---|
| +0x00 | EmberListMgr* | mMgr | arg | owner (`mMgr->mApp` for images and screen size) |
| +0x04 | int | mUpdateCnt | 0 | **never incremented** by any code (only saved) |
| +0x08 | bool | mDead | false | |
| +0x10 | double | mX | 0 | centre |
| +0x18 | double | mY | 0 | |
| +0x20 | double | mVX | 0 | |
| +0x28 | double | mVY | 0 | |
| +0x30 | double | mGravity | 0.01 | added to mVY per update |
| +0x38 | double | mAlpha | 0 | life 1..0; frame selection, death, type 10 alpha |
| +0x40 | double | mAlphaDecay | 0 | subtracted from mAlpha per update |
| +0x48 | int | mDelay | 0 | |
| +0x4c | bool | mLoop | false | cycle frames instead of following mAlpha |
| +0x50 | int | mFrame | 0 | |
| +0x54 | int | mNumFrames | 0 | computed on the first Update |
| +0x58 | int | mType | 0 | 0..10, see the table below |

Methods: 0x41f300 `Ember(EmberListMgr*)` (calls 0x41ec50 `Reset(EmberListMgr*)`, which writes every field above;
used again when a pooled ember is reused); 0x41ec40 `~Ember` (sets mMgr = NULL); 0x41f0a0 `Save`, 0x41f180 `Load`.

`Update()` 0x41ecb0:
1. If `mNumFrames == 0`: types 0-2: `H / W` of the spark image (vertical strip); types 3-9: `W / H`
   (horizontal strip); type 10: 1. (Computed before the delay check.)
2. If `mDelay > 0`: `mDelay--`, return.
3. Frame (types 0..9): if mLoop: when `mUpdateCnt % 4 == 0` (always, mUpdateCnt stays 0): `mFrame = (mFrame + 1)
   % mNumFrames` (so looping embers advance one frame every update); else
   `v = min(mNumFrames * mAlpha - 1.0, mNumFrames - 1)`, `mFrame = mNumFrames - ftol(v) - 1`
   (0 at full alpha, mNumFrames-1 when v is in (-1, 1); mNumFrames if v is exactly -1.0). Type 10: mFrame = 0.
4. `mX += mVX; mVY += mGravity; mY += mVY; mAlpha -= mAlphaDecay;`
5. mDead = true if `mAlpha < 0`, or `mX < 0 && mVX <= 0`, or `mX > app->mWidth (640) && mVX >= 0`, or
   `mY < 0 && mVY <= 0`, or `mY > app->mHeight (480) && mVY >= 0` (comparisons on the new values, ints converted
   to double).

`Draw(Graphics* g)` 0x41f320 (`X = ftol(mX)`, `Y = ftol(mY)`, `c` = cel size, all additive unless noted, mode
restored to normal):

| type | image | cel | src rect | position |
|---|---|---|---|---|
| 0 / 1 / 2 | IMG(67/68/69) fx_spark_sm/med/lrg | c = W | (0, mFrame*c, c, c) | (X - c/2, Y - c/2) |
| 3 / 5 / 6 / 7 / 8 / 9 | IMG(58/60/61/62/63/71) | c = H | (mFrame*c, 0, c, c) | (X - c/2, Y - c/2) |
| 4 | IMG(59) fx_ember_med additive, then **normal** colorized (255,255,255,0xb0) IMG(62) fx_ember_smoke_med with its own c2 = H, src (mFrame*c2, 0, c2, c2) at (X - c2/2, Y - c2/2) | | | |
| 10 | IMG(70) fx_smoke_sm, normal, colorized (255,255,255, ftol(mAlpha*255.0)) | whole | | (X - 15, Y - 15) |

#### 3.6 EmberListMgr (0x2c)

| off | type | name | init |
|---|---|---|---|
| +0x00 | vfptr | | 0x4fc4c4 |
| +0x04 | BookWormApp* | mApp | ctor arg |
| +0x08 | MTRand* | mRand | `new MTRand(Sexy::Rand())`; never drawn from (only saved/restored) |
| +0x0c | std::vector<Ember*> | mEmbers | active, draw order (first +0x10, last +0x14, end +0x18) |
| +0x1c | std::vector<Ember*> | mFreeEmbers | pool (first +0x20, last +0x24, end +0x28) |

| address | method | behaviour |
|---|---|---|
| 0x421070 | `EmberListMgr(BookWormApp*, int poolSize)` | vectors empty, mApp, `Sexy::Rand()` (1 draw) seeds `mRand`, then poolSize times `new Ember(this)` pushed to mFreeEmbers (Board passes 1000) |
| 0x4203a0 | dtor body | Clear(), delete mRand, mApp = NULL, free both buffers |
| 0x420520 | deleting dtor (slot 0) | |
| 0x4202d0 | `Clear()` | deletes every ember of mEmbers then of mFreeEmbers (repeated erase of the first element), frees both buffers (the pool is empty afterwards) |
| 0x420d60 | `Ember* NewEmber()` | if mFreeEmbers is not empty: take its **last** element, `Reset(this)`, push_back to mEmbers; else `new Ember(this)` (NULL is pushed if new fails); returns it |
| 0x420e00 | `AddEmber(int type, double x, double y, double vx, double vy, bool loop, double alpha, double decay)` | NewEmber, then sets mType, mX, mY, mVX, mVY, mLoop, mAlpha, mAlphaDecay (gravity 0.01, delay 0) |
| 0x420fd0 | `bool Update()` | `had = size != 0`; `for (i = 0; i < size; i++)`: `Update()`; if dead: push to mFreeEmbers, `mEmbers[i] = mEmbers.back()`, pop_back, `i--` (the moved ember is updated in the same pass; **the order changes**) |
| 0x41f8d0 | `Draw(Graphics*)` | `Draw(g)` of each non-null ember in vector order |
| 0x41ff80 | `Save(FILE*)` | section 6 |
| 0x420e70 | `Load(FILE*)` | section 6 |

Draw order matters only for the normal-blend types (4's smoke overlay, 10); additive saturating blits commute.

#### 3.7 Random numbers in Effect.cpp (all MTRand, SAF 1.2 generator; `Next()` is 31-bit; `%` is unsigned `div`)

| where | generator | draws, in call order |
|---|---|---|
| EffectMgr ctor 0x4201c0 | global `Sexy::Rand()` (gMTRand) | 1 (seed of mRand) |
| EmberListMgr ctor 0x421070 | global `Sexy::Rand()` | 1 (seed of mRand) |
| FlyingPieceList::Update, per in-flight (not arrived this tick) piece in list order, type 3, `mUpdateCnt % 3 == 0` | `mEffectMgr->mRand` | r1 = Next()%57 (x), r2 = Next()%57 (y), r3 = Next()%1000 (vx), r4 = Next()%1000 (vy), r5 = Next()%1000 (decay) |
| same, types 4/5, `mUpdateCnt % 6 == 0` | `mEffectMgr->mRand` | r1 = Next()%57 (x), r2 = Next()%57 (y) |

Board ctor order (0x40ae90): Board+0x6c `new MTRand(Rand())`, then the EffectMgr, then the EmberListMgr, so three
consecutive gMTRand draws. The CRT `rand()` is not used by this group.

#### 3.8 `MapTableIndex(int)` 0x4216e0 (linked in Effect.cpp, used by FameWidget)

Free function: 0 -> 2, 1 -> 0, 2 -> 1, anything else -> -1. Callers 0x4249d0, 0x427650 (FameWidget group).
The STL/locale functions at 0x41f900, 0x420400-0x4206ef, 0x420b00, 0x421710-0x421850 are library code.

### 4. Points (Points.cpp, 0x84)

The floating "+N" / bonus texts. Same class as Big Money's Points (`games/bigmoney/src/Points.h`), smaller Widget.

| off | type | name | meaning | init |
|---|---|---|---|---|
| +0x00..+0x57 | Widget | | `mMouseVisible` (+0x1a) = false, `mHasAlpha` (+0x22) = true | |
| +0x58 | BookWormApp* | mApp | | arg 1 |
| +0x5c | int | mLife | updates left; Board::Update decrements it and deletes the widget at <= 0 | arg 6 |
| +0x60 | std::string | mText | | arg 3 |
| +0x7c | Font* | mFont | | arg 2 |
| +0x80 | int | mHue | Board::Update: `(mHue + 7) % 255` per tick; never drawn | 0 |

- 0x431a50 `Points(BookWormApp* app, Font* font, const std::string& text, int x, int y, int life, int justify)`:
  `w = font->StringWidth(text) + 5`; justify 0: `x -= w/2`; 1: `x -= w`; else x is the left edge;
  then if `x + w > 0x27e` (638): `x = 638 - w`, else if `x < 0xe2` (226): `x = 226`;
  `Resize(x, y - font->GetAscent(), w, font->GetHeight())`.
- 0x431950 `Draw(Graphics*)` slot 16: base Widget::Draw (empty); `a = min(255, mLife * 255 / 10)` (int, no lower
  clamp); colour (255,255,255,a); `SetFont(mFont)`; `DrawString(mText, 0, mFont->GetAscent())`.
- 0x4319e0 dtor body, 0x431a20 deleting dtor (slot 0).
- Board-side behaviour (Board group, for reference): Board::Update (0x40da8c) per tick: MarkDirty, mHue cycle,
  every 2 ticks (`cnt % 2 == 0`) `Move(mX, mY - 1)` while mY > 0, `mLife--`, delete at <= 0. Creators: 0x409970
  (`Board::AddPoints(int base, int dx, int dy)`: text "+%d" of `ftol(base * Board+0x3a0)` in FONT(12) GoldScore,
  centred at (Board.mX + dx, Board.mY + dy), life = clamp(ftol(base * 0.3333333333333333), 150, 600)) and 0x409b70
  (bonus text in FONT(14) LockergnomeSmallColor at x = 0xe9 left-aligned, life 200). Draw alpha fades over the last
  10 updates.

### 5. Cross-module references

#### 5.1 Board fields read or written by this group (Board group owns the layout)

| off | type | guess | access here |
|---|---|---|---|
| +0x08 / +0x0c | int | Widget mX / mY | read (Points creators only) |
| +0x4c | int | Widget mUpdateCnt (glow/fire animation clock) | read |
| +0x5c | BookWormApp* | mApp | read |
| +0x6c | MTRand* | Board's RNG (not used here) | |
| +0x70 | EffectMgr* | mEffectMgr | read (Draw) |
| +0x74 | EmberListMgr* | mEmberListMgr | read (Draw; FlyingPieceList::Update NewEmber) |
| +0x78 | Piece*[8][7] | mBoard[row][col], row stride 0x1c | read |
| +0x194 | int | state; 7 = board-wide letter spin | read |
| +0x1a0 / +0x1a4 | std::list<Piece*> | selected tiles (head / size) | read |
| +0x1b1 | bool | scorched (board burnt, game over) | read |
| +0x1b3 | bool | "tiles flying" dirty flag | written true by FlyingPieceList::Update |
| +0x1c0 | double | board-wide spin time 0..1 (+0.01/tick in state 7) | read |
| +0x1c8 | double | progress target 0..1 | read |
| +0x1d0 | double | progress shown 0..1 | read |
| +0x220 | WordScore (0x48) | current word: +4 ?, +8 string mWord (size at +0x1c, Q -> "QU"), +0x24 bool mValid, +0x28 + 4*type tile counts (types 0..5), +0x40 tile count, +0x44 points | read (copied) |
| +0x268 | Color | word colour | written by DrawWord (rgb from the counts, alpha at +0x274) |
| +0x278 | double | word display fade (1.0 when set, -0.08 every 8 ticks) | read |
| +0x280 | double | score glow 0..0.85 | read |
| +0x350 | Lex* | Lex widget | read |
| +0x365 | bool | burp smoke active | read |
| +0x368 | int | burp countdown | written 50 by FlyingPieceList::Update when a fire tile arrives |
| +0x370 | double | burp smoke time 1..0 | read |
| +0x378 | int | held letter key (KeyDown 0x403df0, registered only, state 0) | read |
| +0x39c | int | displayed score | read |
| +0x3a8 / +0x3b4 | list<Points*> / Points* | live Points widgets / last bonus Points | (Board group) |
| +0x3c4 | int | level, 0-based | read |
| +0x3d0 | double | pause fade 0..1 | read |

#### 5.2 Piece fields (Piece, 0xf0, Board group)

+0x08 / +0x10 double x / y (top-left, screen = board coordinates); +0x18 / +0x20 double flight speed fractions
(FlyingPieceList: x/y step = trunc(distance) * fraction, +0.003 per moving tick, zeroed on arrival); +0x30 int col,
+0x34 int row; +0xa4 char letter, +0xa5 char next letter (after a spin; ctor 0xff); +0xa6 bool selected;
+0xa8 double scale (ctor 1.0; *0.98 per flight tick); +0xb0 bool (excludes the piece from the glow lists; meaning
unknown, maybe "falling/removed"); +0xbc bool (forces the fire glow; maybe "about to burn"); +0xd8 bool spinning,
+0xe0 double spin time 0..1; +0xe8 int type (1.4); +0xec int (fire-below level: the tile below a fire tile gets the
fire glow when >= 1). Piece slot 0 = scalar deleting dtor. `Piece::Save` 0x403050, `Piece::Load` 0x403370,
ctor 0x403f20.

#### 5.3 Other classes

- App (BookWormApp): +0x11ac image array (149 entries, names at 0x52e410), +0x11b0 fonts (19), +0x1160 Board*,
  +0x1204..+0x1224 tile sets, +0x1244/+0x1248 gem tile copies, +0x1254 game mode (0 classic, 1 action),
  +0x470 mIsRegistered, +0x60/+0x64 mWidth/mHeight (640x480), slot 41 `PlaySample(int)` (sound 18 = chomp).
- Lex: +0x08/+0x0c mX/mY, +0x80/+0x84 home x/y (int), +0x88/+0x90 current x/y (double), +0xb0 bool shadow
  visible, +0xb4 current animation (-1 idle). Functions: 0x42d320 `Lex::PlayAnim(int anim, bool)`, 0x404340
  `Lex::IsBusy()` (linked in Board.cpp; false when anim == -1 and at home). Animations used: 21, 22 (19 tested).
- Board functions shared with the Board group: 0x403bd0 `Board::GetPieceBelow(Piece*)` (grid[row+1][col] or NULL
  for row >= 7), 0x4051d0 `~WordScore` body.
- Framework: Graphics (DrawImage 0x44ba30 / 0x44bad0 / 0x44bbb0, ClipRect 0x44bdb0, Translate 0x446cc0, SetColor,
  SetFont, SetDrawMode, SetColorizeImages, SetFastStretch 0x446c80, DrawString), Color ctors, Image::GetWidth /
  GetHeight (folded getters), `MTRand::MTRand(ulong)` 0x467030, `MTRand::Next` 0x467060, `Sexy::Rand` 0x467070,
  `MTRand::Serialize` 0x469b50, `MTRand::SRand(const std::string&)` 0x4673d0, `StrFormat` 0x46e5d0, CRT
  `sprintf`, `fread`/`fwrite`, `OutputDebugStringA`, `Widget::Resize` 0x439e90, `WidgetManager::AddWidget`.

### 6. Files and formats

The effect managers are serialised inside the game save `classic.dat` / `action.dat` (Board save 0x406c40 opens
`"wb"`, load 0x408ab0 opens `"rb"`; order: EffectMgr then EmberListMgr, each Load preceded by Clear). All values
little-endian, raw struct bytes:

EffectMgr (Save 0x41faa0 / Load 0x420b70):
```
u32 n (= 0x9c0)  u8 mt[n]          MTRand::Serialize (624 words, no mti); Load: SRand(string), mti kept
u32 count                          list size
count x { u8 present;
          if present (and GetType() is 0 or 1): u32 type, u32 mUpdateCnt, u8 mDead, class data }
  type 0 Sparkle:          f64 x, y, vx, vy, gravity; i32 delay; u8 (+0x3c); i32 image; i32 frame
  type 1 FlyingPieceList:  u32 n; n x { u8 present; Piece::Save if present }; i32 mLastChompCnt
```
Load creates `new Sparkle(this)` / `new FlyingPieceList(this, NULL)`, calls Load (slot 6), sets mEffectMgr = this
and appends; any other type value is skipped without reading its data (cannot happen in practice).

EmberListMgr (Save 0x41ff80 / Load 0x420e70):
```
u32 n  u8 mt[n]                    as above
u32 count                          active vector size
count x { u8 present;
          if present: i32 mUpdateCnt, u8 mDead, f64 x, y, vx, vy, gravity, alpha, decay,
                      i32 delay, u8 loop, i32 numFrames, i32 frame, i32 type }   (79 bytes with the flag)
```
**Load bug** (`Ember::Load` 0x41f180): the first two fields are passed to `fwrite`, not `fread`. On the "rb" stream
the MSVC 7 CRT `fwrite` copies into the read buffer while it holds unread bytes (`_cnt != 0`), which advances
the position: normally the 5 bytes are skipped and mUpdateCnt / mDead keep their Reset values (0 / false). If fewer
than 5 unread bytes remain in the 4096-byte stdio buffer, the rest of the write fails (`_flsbuf` on a read-only
stream) and the stream falls out of step by the missing bytes. Port: read and discard 5 bytes (matches the normal
case); see open question 2. EmberListMgr::Load also frees the active vector's buffer without deleting its embers
(harmless: the Board calls Clear() first).

### 7. What is not ported / build-dependent branches

- `DrawSelection` tile_x threshold: unregistered (trial) builds mark letters beyond 8, registered beyond 12.
  The retail build is registered (`app+0x470` true): use 12.
- The held-letter highlight (`Board+0x378`) is only set when registered (Board::KeyDown); the registered build
  has it.
- No trial, money, update-check or internet code in this group.

### 8. Port notes and quirks to keep

1. Score glow overlay is invisible (alpha `ftol(min(glow, 0.5)) * 255` = 0).
2. Gem tiles: the (255,255,255,176) colour before the gem image has no effect (colorize off).
3. A tile's letter glyph (59x59) is drawn unclipped at the tile's top-left on gem tiles; in flight it is stretched
   to the tile size with fast (nearest) stretching.
4. Fire tiles inside a per-piece spin show the normal tile set until frame 8; the board-wide spin skips fire
   tiles.
5. Glow lists can contain a piece twice (fire tile + the "below" tile), giving a double additive glow.
6. Ember `mUpdateCnt` is never incremented: looping embers advance every update (the `% 4` gate is always open).
7. EmberListMgr::Update reorders the active vector (swap-with-last removal); draw in that order.
8. FlyingPieceList targets `prev` even when `prev` arrived in the same tick, and the first remaining piece targets
   Lex immediately after the first one is removed.
9. Sparkles draw at their start position during their delay.
10. Progress bar uses progress_burnt for level index -1 (only if the shown progress exceeds the target at level 0).
11. 2004 integer/double details: the Lex-shadow distance sums squares as int; flight steps use `ftol(distance)`;
    all positions are `ftol` of doubles (truncation toward zero, so negative coordinates round up).

### 9. Open questions

1. Effect vtable slot 0 (0x41e950) takes the effect as an explicit argument and deletes it; its source name is
   unknown (`DeleteEffect(Effect*)` proposed). Port: `delete e`.
2. Ember::Load's fwrite bug: whether real saves ever hit the buffer-boundary case. Each saved ember has about a
   5-in-4096 chance (the 5 skipped bytes straddle the end of a 4096-byte stdio buffer); with hundreds of active
   embers at save time a desynchronised load becomes likely. A port that wants identical loading of original saves
   must emulate the CRT buffer position (file offset of the record mod 4096); reading and skipping 5 bytes is
   correct otherwise.
3. Meaning of Sparkle +0x3c (saved, never read), WordScore +4, Piece +0xb0 / +0xbc / +0xec (names above are
   guesses; the Board group should confirm).
4. `Board+0x1b3` is set by FlyingPieceList::Update; its reader is in the Board group.
5. Lex animation ids 19, 21, 22, 23 (meaning from the Lex group).
6. Sparkle mImage is always 0x8d (sparkle_large) in the Board and FlyingPieceList code; check whether OverlayWidget
   ever sets 0x8c (sparkle, 280x20).
7. Whether `font_tile_letters` (mask-only file, white glyphs with alpha) shows "Qu" on its 'Q' cel (affects only
   asset expectations, not code).

---

## Part 4. Lex and the balloons (Lex, BigLex, WordBalloon, Definitionator)

Behaviour spec for translating `Lex.cpp`, `BigLex.cpp`, `WordBalloon.cpp` and `Definitionator.cpp` of BookWorm
Deluxe 1.03 (game PE 2004-04-15, MSVC 7.0, early-2004 SexyAppFramework). Proposed header: `lex.h`; Ghidra
symbols: `lex_symbols.jsonl`. Addresses are virtual addresses of `re/bookworm/bin/BookwormGame.exe`.
All numbers below were checked against the disassembly (every `_ftol`, x87 compare and constant).

Conventions: "update" = one `Update()` call; the app runs 100 updates per second (`mFrameTime` 10 ms).
"slot N" = 2004 Widget vtable slot (`docs/FRAMEWORK_2004.md`). `(int)x` = `_ftol` = truncation toward zero.
`app` = the BookWormApp, `board` = `app+0x1160` (Board*), images = `app+0x11ac` (Image* array, see 1.1).

### 0. Module boundaries (differences from INVENTORY.md)

| Function | Inventory range it sits in | Really belongs to |
|---|---|---|
| 0x404340 `Lex::IsBusy`, 0x403ab0 `Lex::IsMoving` | Board.cpp | Lex (inline methods from Lex.h, emitted in Board.obj; take a `Lex*` in ECX) |
| 0x402f60, 0x402fd0 | BigLex.cpp | called only by Board; a hex-adjacency test and a global-double setter (section 6) |
| 0x41d2b0 `Definitionator::GetDefinition` | CreditsDialog.cpp (among the `map<string,string>` COMDATs) | Definitionator |
| 0x436260 `WordBalloon::Init`, 0x436310 `WordBalloon::SetJustification` | WaitDialog.cpp | WordBalloon (the WordBalloon module starts at 0x436260) |
| 0x437540-0x4377ff | WordBalloon.cpp | framework (WidgetManager, Graphics::operator=); widgets group |

No random-number draw exists in BigLex, WordBalloon or Definitionator. Lex::Update draws from the **CRT
`rand()`** only (never `Sexy::Rand` / MTRand); see 2.4.

### 1. Shared data

#### 1.1 Images

`app+0x11ac` is an `Image*[149]` filled by the app's loader (0x4144b0): image *i* = `GetImage(ImagePath + name[i])`
with the name table at 0x52e410 (`ImagePath` property, default `images\`). Ids used here (all `.gif` with an
`_name.gif` alpha mask unless noted; sizes from the staged files):

| id | name | size | used by |
|---|---|---|---|
| 0 | backdrop_title | | BigLex frame 0 = "draw nothing" (never drawn by BigLex) |
| 26 | backdrop_wb_thought | 188x137 | Lex::Think plain balloon |
| 27 | backdrop_wb_text | 27x27 | speech box (9-slice) for Say/SayDefinition and the OverlayWidget's balloon |
| 73 | fx_lex_sweat_seq | 96x384 (8 rows of 48) | drawn by OverlayWidget::Draw with Lex::mSweatFrame |
| 102 | lex_shadow (alpha only) | 143x46 | drawn by Board::Draw when Lex::mShowShadow |
| 103 | lex_headless | 101x202 | body under head overlays |
| 104 | lex_glow_idle | 107x208 | mouse-over glow |
| 105 | lex_smile_idle | 101x202 | idle pose (full body) |
| 106 | lex_smile_blink | 101x52 | blink overlay |
| 107 / 108 | lex_giggle_1 / lex_giggle_2 | 101x127 / 101x125 | overlays |
| 109 | lex_slurp_1 | 101x110 | overlay |
| 110-113 | lex_speak_ah / _ee / _oh / _pb | 101x107 | mouth overlays |
| 114 / 115 | lex_sweat / lex_sweat_facing | 101x107 | overlays |
| 116 | lex_burnt | 101x202 | burnt full pose (also the body when the board is burnt) |
| 117 | lex_burnt_blink | 101x54 | overlay |
| 118 | lex_eating | 107x208 | full eating pose; its size is Lex's widget size |
| 119-123 | lexbig_speak_ah / ee / mm / oo / pb | 133x123 | BigLex (oo and pb unused) |
| 125 | wb_text_talk_tail | 40x20 | tail of the speech box (left 20 px half drawn) |

`lex_wow_1..7.gif` are in the game folder but not in the name table: never loaded, never used.

#### 1.2 Fonts and sounds

Fonts are `app+0x11b0 + 4*k` (ImageFont, table 0x52e3c0): 0x11b4 = k1 `Verdana12Bold.txt`, 0x11b8 = k2
`Verdana14Bold.txt`, 0x11f0 = k16 `MyriadWeb10.txt`, 0x11f8 = k18 `MyriadWeb12Bold.txt`. Font slots called:
+0x18 GetLineSpacing, +0x1c StringWidth.

Sounds go through `app` vtable slot 41 `PlaySample(id)` (0x47db50); this group plays only id 0 `buttonclick`
(Lex::MouseDown). The voice samples played together with Lex animations are played by the callers (2.2).

### 2. Lex (Lex.cpp)

`class Sexy::Lex : public Widget`, **sizeof 0xd0** (`operator new(0xd0)` in Board::AddedToManager 0x403730 and
in Board's game load 0x408ab0), vftable 0x5005b4, 49 slots (Widget's 47, slot 14/16/17/23/24/27/29 overridden,
new 47 Save, 48 Load). One instance, `Board::mLex` = board+0x350, top-level widget (screen coordinates).

#### 2.1 Layout

| Off | Type | Name | Meaning | Ctor | Read / written by |
|---|---|---|---|---|---|
| +0x00..0x57 | Widget | | 2004 Widget | | ctor sets mHasTransparencies(+0x21)=1, mHasAlpha(+0x22)=1, mDoFinger(+0x48)=1, mMouseInsets.mTop(+0x3c)=28, mMouseInsets.mBottom(+0x44)=18 |
| +0x58 | double | mHintLevel | scramble-hint progress | 0.0 | Update (+= stats, +0.2), MouseEnter/Leave (> 1.0 test) |
| +0x60 | bool | mHintPending | hint armed by MouseEnter | false | MouseEnter, Update, MouseLeave, MouseUp |
| +0x61 | bool | mHintShowing | the balloon shows Lex's hint | false | Update, MouseLeave/Down/Up, MouseEnter (test) |
| +0x64 | int | mHintDelay | updates before the hint appears | **not initialised** (always written by MouseEnter before use) | MouseEnter/Leave (0 or 75), Update (-1 per update, 150) |
| +0x68 | BookWormApp* | mApp | | arg | everything |
| +0x6c | | padding | | | |
| +0x70 | double | mWobble | spring value (2.3, Update step 5) | 0.0 | Update, Save/Load; **never read anywhere else** |
| +0x78 | double | mWobbleVel | | 0.004 (0x3f70624dd2f1a9fc) | Update, Save/Load |
| +0x80 | int | mTargetX | where Lex slides to | 0 | Move; Board writes 60 (0x403730); Board::Draw reads (shadow) |
| +0x84 | int | mTargetY | | 0 | Move; Board writes 210 |
| +0x88 | double | mCurX | current x | 0.0 | Move, Update; IsMoving; Board::Draw reads |
| +0x90 | double | mCurY | current y | 0.0 | as mCurX |
| +0x98 | double | mVelX | | 0.0 | Update |
| +0xa0 | double | mVelY | | 0.0 | Update |
| +0xa8 | double | mGlow | mouse-over glow 0..1 | 0.0 | Update, Draw, MouseDown (= 0) |
| +0xb0 | bool | mShowShadow | Board::Draw draws lex_shadow | true | never cleared |
| +0xb1 | bool | mIdle | no mouse press for > 12 s | true | Update only (and Save/Load): **no reader** |
| +0xb4 | int | mAnim | gLexAnims row, -1 none | -1 | PlayAnim, Update, Reset; Board reads (== -1, 19, 23); OverlayWidget reads (== 19) |
| +0xb8 | int | mAnimStep | | 0 | |
| +0xbc | int | mAnimTick | updates on the current step | 0 | |
| +0xc0 | int | mFrame | image id drawn as pose/head | 105 | Update, PlayAnim, Draw |
| +0xc4 | bool | mAnimLoop | | false | |
| +0xc5 | bool | mSweating | danger state | false (via SetSweating) | SetSweating; Board, OverlayWidget, MouseEnter read |
| +0xc8 | int | mSweatFrame | 0..7 sweat-drop row | 0 | Update, SetSweating, Reset; OverlayWidget::Draw reads |
| +0xcc | | padding to 0xd0 | | | |

Ctor 0x42d560 `Lex(BookWormApp* theApp)`: Widget(); the values above; then `SetSweating(false)` (which also sets
mDoFinger = true) and mSweatFrame = 0 (the decompiler shows `extraout_EDX`; EDX is 0 there).

Destructor 0x42d050 (body: vftable, `~Widget`), 0x42d650 deleting (slot 0).

#### 2.2 Animations

Table `gLexAnims` at 0x4fd9f0: `int[28][50][2]`, `{image id, duration}` pairs ending with `{-1, 0}`; a step is shown
for **duration x 2 updates**. The full table is in `lex.h`. Totals are in updates.

| row | frames (image/duration) | total | who plays it (call site: sample played with it) |
|---|---|---|---|
| 0 | smile_idle/1, ah/8, blink/6, idle/3, blink/3, idle/2 | 46 | Board::Update 0x40c032 (game mode app+0x1254 != 1, board MTRand%100 < 50): sample 37 lex_go |
| 1 | idle/1, ah/3, idle/3, ah/4, ee/2, idle/2, ah/3, oh/9, idle/2 | 58 | Board::Update 0x40c048 (mode != 1, MTRand%100 >= 50): sample 39 lex_letsgo |
| 2 | idle/1, ah/8, idle/6, ah/8, pb/8, sweat/8, pb/8, idle/2 | 98 | Board::Update 0x40bec0 (sweating, MTRand&3 == 2): 41 lex_lookout |
| 3 | idle/1, ah/5, idle/6, ah/8, idle/2 | 44 | 0x40bedd (&3 == 3): 45 lex_watchout |
| 4 | idle/1, ah/4, idle/5, oh/6, ah/4, idle/2 | 44 | 0x40be91 (&3 == 0): 32 lex_careful |
| 5 | idle/1, ah/3, idle/4, oh/5, ah/3, idle/2 | 36 | 0x40beae (&3 == 1): 33 lex_danger |
| 6 | ah/3, idle/3, ah/4, idle/4, ah/4, idle/4, ah/4, idle/3, idle/5 | 68 | no caller |
| 7 | pb/3, oh/7, ee/4, ah/7, idle/5 | 52 | 0x40ac06 `GT_BOARD_BONUS_FORMAT` (27 bonusword); 0x40c0f5 `GT_BOARD_TIME_BONUS` (14 wordcomplete) |
| 8 | ah/3, idle/3, pb/3, ah/3, idle/2, ah/2, idle/2, ah/3, idle/3, blink/3, idle/3 | 60 | 0x40ecf8 word praise: 29 lex_astonishing |
| 9 | ah/2, idle/2, pb/2, ah/2, idle/2, ah/2, idle/1, oh/3, idle/3, blink/3, idle/3 | 50 | praise: 30 lex_awesome |
| 10 | idle/1, ah/3, idle/3, oh/2, idle/3, ah/3, idle/2, ah/2, idle/3 | 44 | praise: 34 lex_excellent |
| 11 | idle/1, blink/4, giggle_2/4, slurp_1/1, idle/2, oh/8, blink/3, ah/8, idle/4, giggle_2/2, idle/2, ah/4, idle/3 | 92 | praise: 35 lex_fantastic |
| 12 | idle/1, blink/2, giggle_1/2, blink/2, idle/1, ah/4, idle/3, ah/4, idle/3, oh/6, idle/2, ah/3, idle/3 | 72 | praise: 38 lex_incredible |
| 13 | blink/2, giggle_2/2, giggle_1/1, oh/3, ah/3, idle/3, ah/2, idle/2, ah/2, idle/2, ah/3, idle/4, ah/3, idle/3 | 70 | praise: 43 lex_unbelievable |
| 14 | ah/4, idle/4, ah/4, idle/4, ah/3, oh/2, idle/3, idle/3 | 54 | praise: 44 lex_verygood |
| 15 | giggle_2/1, giggle_1/1, blink/1, ah/4, ee/4, ah/5, idle/4, ee/2, ah/5, idle/5 | 64 | 0x40eb3c `GT_LEVEL_UP`: 40 lex_levelup |
| 16 | idle/1, ee/3, ah/19, pb/1, oh/10, ah/5, idle/5 | 88 | no caller |
| 17 | ah/5, idle/3, blink/3, ah/7, idle/3, giggle_2/2, ah/3, idle/3 | 58 | Board::Update 0x40c00b (mode == 1): 36 lex_getready |
| 18 | ah/4, idle/5, ah/5, idle/4, ah/4, idle/5 | 54 | Board::Scramble 0x40f2dd: 19 flametile then 42 lex_scramble |
| 19 | sweat/50, sweat_facing/10 (looped) | 120 | Board (0x40bf06, 0x40d2f3, 0x40d40e), loop = true |
| 20 | oh/9, ah/3 | 24 | 0x40b818: 22 appear2 |
| 21 | eating/100000 | 200000 | FlyingPieceList (Effect.cpp 0x421209), stays until replaced |
| 22 | eating/1, pb/5, idle/1 | 14 | FlyingPieceList 0x421199; Board 0x40d4fc (bonus fade < 0.9 while row 23) |
| 23 | eating/100000 | 200000 | Board 0x40d49f (bonus word eaten): 31 lex_burp |
| 24 | burnt/1 | 2 | no caller |
| 25 | burnt_blink/5, burnt/1 | 12 | Lex::Update random (board burnt); Board 0x40e92b when the board burns |
| 26 | smile_idle/1 | 2 | Lex::Update random |
| 27 | smile_blink/5, smile_idle/1 | 12 | Lex::Update random |

Praise rows 8-14 (Board 0x40ec40..): word class 2: MTRand%100 < 33 row 14, < 66 row 10, else row 9; class 4: < 50
row 11 else row 8; class 5: < 50 row 12 else row 13 (Board's MTRand, board group).

#### 2.3 Functions

**0x42d320 `void PlayAnim(int theAnim, bool theLoop)`** (6 callers): mAnim = theAnim, mAnimStep = 0, mAnimTick = 0,
mFrame = gLexAnims[theAnim][0].mImage, mAnimLoop = theLoop, MarkDirty (slot 46). Restarts even the same row.

**0x42d3c0 `void SetSweating(bool on)`**: if on != mSweating then mSweatFrame = 0; mSweating = on; mDoFinger = !on.

**0x42d680 `void Reset()`** (only caller Board 0x406c40, which also stops the effects and then writes the save file): mAnim = -1, mAnimStep = mAnimTick = 0, mFrame = 105,
mAnimLoop = false, SetSweating(false), mSweatFrame = 0. Position, glow and hint state are kept.

**0x42d370 `Move(int x, int y)` slot 14**: Widget::Move(x, y); mTargetX = mX, mTargetY = mY, mCurX = (double)mX,
mCurY = (double)mY (velocities unchanged). Board creates Lex with `Resize(0, 0, 107, 208)` (lex_eating's
size), `AddWidget`, `Move(-200, 0)`, then writes mTargetX = 60, mTargetY = 210: Lex slides in from the left.

**0x404340 `bool IsBusy()`** (inline): true unless `mAnim == -1 && (double)mTargetX == mCurX && (double)mTargetY == mCurY`.
**0x403ab0 `bool IsMoving()`**: the same without the mAnim test.

**0x42d060 `Save(FILE*)` slot 47 / 0x42d1c0 `Load(FILE*)` slot 48**: `fwrite`/`fread` of the 88-byte record of
8.1, field by field. Load writes mX/mY directly (no Resize) and stores mSweating without SetSweating (mDoFinger is
not updated). Called by Board 0x406c40 (writes a 1-byte "has Lex" flag, then slot 47) and Board's load 0x408ab0 (new Lex,
Resize(0,0,107,208), AddWidget, slot 48 Load, SetVisible(board->mVisible)).

**0x42db60 `Draw(Graphics* g)` slot 16.** Coordinates are widget-local.
1. `yOff` = 0 if mFrame == 118 (eating), else H(lex_eating) - H(lex_smile_idle) = 208 - 202 = **6**.
2. Body: if `board->mBurnt` (board+0x1b1): DrawImage(lex_burnt, 0, yOff). Else, unless mFrame is 105 or 118:
   DrawImage(lex_headless, 0, yOff).
3. If mFrame == 106 (blink): DrawImage(lex_smile_idle, 0, yOff).
4. Glow: if mGlow > 0.0 and !board->mBurnt and !IsBusy(): SetColorizeImages(true); SetColor(255, 255, 255,
   a) with a = (int)clamp(mGlow * 255.0, 0, 255); DrawImage(lex_glow_idle, 0, yOff - 6); SetColorizeImages(false)
   (the Graphics colour is not restored).
5. DrawImage(images[mFrame], 0, yOff).
So a burnt idle Lex draws lex_burnt twice (body + frame 116): keep it, the edges blend twice.

**0x42dd20 `Update()` slot 17.** Whole body skipped when `board != NULL && board+0x3c8 > 0` (a board freeze
counter). Otherwise, in order:
1. `Widget::Update()` (mUpdateCnt++).
2. Hint (balloon = `board->mOverlay(+0x158)->mWordBalloon(+0x60)`): if mHintPending and !balloon->mVisible:
   - if mHintDelay > 0: mHintDelay--;
   - else: mHintPending = false; name = player name (`app+0x1178` profile, accessor 0x402090 copies the
     std::string at profile+0x10); mHintLevel += (double)(int)`board->+0x2a8 (StatKeeper*) ->+0x30`;
     if mHintLevel > 1.0 **or** the name is empty: `Think(`GT_LEX_SCRAMBLE_SHORT`, app font
     Verdana14Bold, -1)`, mHintDelay = 150;
     else: `Say(StrFormat(`GT_LEX_SCRAMBLE_LONG_FORMAT`, name), Verdana12Bold, -1)`, `balloon->SetJustification(0, 0, 0)`,
     mHintLevel += 0.2. Then mHintShowing = true.
3. `burnt` = board != NULL && board->mBurnt.
4. Idle latch: if `GetTickCount() - wm->mLastMouseDownTick (WM+0x168)` (unsigned) <= 12000: mIdle = false; else if
   !mIdle && board && !board->mBurnt && !IsBusy(): mIdle = true. Nothing reads mIdle (dead state; keep for the save).
5. Spring (all double):
   - w = mWobble + mWobbleVel; mWobble = w; mWobbleVel = mWobbleVel - w * 0.0004; s = w * 0.99;
   - dx = ((double)mTargetX - mCurX) * 0.0515; mVelX += dx; if |mVelX| <= 0.1: mCurX = (double)mTargetX,
     mVelX = 0; else mVelX *= 0.54;
   - mWobble = dx * 0.01 + s;
   - dy = ((double)mTargetY - mCurY) * 0.0515; mVelY += dy; if |mVelY| <= 0.1: mCurY = (double)mTargetY,
     mVelY = 0; else mVelY *= 0.54;
   - mCurX += mVelX; mCurY += mVelY;
   - if |mVelX| > 1.0 or |mVelY| > 1.0: MarkDirtyFull (slot 45);
   - `Resize((int)mCurX, (int)mCurY, mWidth, mHeight)` (slot 13) every update.
6. Sweat drops: if mSweating and mUpdateCnt % 6 == 0: mSweatFrame = (mSweatFrame + 1) % 8; MarkDirty.
7. Glow: if mIsOver: if !burnt and !IsBusy(): mGlow = clamp(mGlow + 0.075, 0, 1), mFrame = 105, MarkDirty,
   **return** (steps 8-9 are skipped). If !mIsOver and mGlow > 0.0: mGlow = clamp(mGlow - 0.075, 0, 1),
   mFrame = 105, MarkDirty, **return** (even while an animation plays: it resumes afterwards). Glow values:
   0.075, 0.15, 0.22499999999999998, 0.3, ... 0.9749999999999998, 1.0 (14 updates up); alpha (int)(g*255).
8. Animation step: if mAnim == -1: mFrame = burnt ? 116 : 105 (no MarkDirty). Else: if ++mAnimTick >=
   2 * gLexAnims[mAnim][mAnimStep].mDuration: mAnimStep++, mAnimTick = 0, mFrame = gLexAnims[mAnim][mAnimStep].mImage;
   if mFrame == -1 and mAnimLoop: mAnimStep = 0, mFrame = row[0].mImage; if mFrame is still -1: mAnim = -1,
   mFrame = board->mBurnt ? 116 : 105; MarkDirty.
9. Random idle animations (2.4).

**0x42d9e0 `MouseEnter()` slot 23** (skipped when board+0x3c8 > 0): Widget::MouseEnter (mIsOver, finger); if
`board+0x334 < 10` (fire-tile count; Board::Scramble refuses at > 9) and (!balloon->mVisible or !mHintShowing):
for each of the 7 pointers board+0x94, +0x7c, +0x9c, +0x84, +0xa4, +0x8c, +0xac (Piece*; with the board's
`Piece* grid[8][7]` at +0x78 (probably [row][col]) these are columns 0..6 at row 1 for even columns and row 0
for odd ones, the same set Board::Scramble walks): if the piece exists, `piece+0xe8 != 3` and !mSweating:
mHintPending = true. Then (always) mHintDelay = mHintLevel > 1.0 ? 75 : 0.

**0x42d3f0 `MouseLeave()` slot 24** (skipped when board+0x3c8 > 0): Widget::MouseLeave; mHintPending = false; if
mHintShowing: mHintShowing = false, `balloon->Show(0)`; mHintDelay = mHintLevel > 1.0 ? 75 : 0.

**0x42d460 `MouseDown(int x, int y, int cc)` slot 27** (skipped when board+0x3c8 > 0): Widget::MouseDown(x, y, cc);
mGlow = 0.0; `app->PlaySample(0)` (buttonclick, any button); if mHintShowing: mHintShowing = false,
`balloon->Show(0)`.

**0x42d4e0 `MouseUp(int x, int y, int cc)` slot 29** (skipped when board+0x3c8 > 0): Widget::MouseUp(x, y, cc);
if mIsOver: `Board::Scramble()` (0x40ed70; it may refuse with the `GT_CANT_SCRAMBLE` dialog), mHintPending = false,
and if mHintShowing: mHintShowing = false, `balloon->Show(0)`. Any mouse button scrambles.

**0x42d6c0 `Say(const string& text, Font* font, int duration)`** (Lex::Update hint; Board 0x40bd40 `GT_LEVEL_UP`
with Verdana12Bold, 75): returns if board->mOverlay is NULL or the balloon is visible. Width
`W = clamp(font->StringWidth(text), 106, 213)` (StringWidth called up to 4 times; it measures the whole string,
newlines included). Then on the balloon: `SetBoxImages(backdrop_wb_text, wb_text_talk_tail, Point((int)(W *
0.75), -2))`, `SetText(text, font, 0)` (centred), `SetColors(gLexSayColors, 4)`, `ClearHeader()`,
`ClearFooter()`, mTextInsets = (8, 0, 8, 0), `Resize(0, 0, W, GetPreferredHeight(W))`, `Show(duration)`.

**0x42d800 `Think(const string& text, Font* font, int duration)`** (Lex::Update hint; OverlayWidget::Update bonus
word message, which afterwards sets mTextInsets = (-5, -10, -5, 0) and its own colours): returns if no overlay or
the balloon is visible. `SetPlainImage(backdrop_wb_thought, Insets(20, 25, 20, 40), true)` (resizes the
balloon to 188x137 at its current position), `SetText(text, font, 0)`, `SetColors(gLexThinkColors, 4)`,
mTextInsets = (0, 0, 0, 0), `ClearHeader()`, `ClearFooter()`, `Show(duration)`.

**0x42d8d0 `SayDefinition(const string& word, Font* wordFont, const string& def, Font* defFont, int duration)`**
(Board 0x40f5b0 after a word, when `app+0x12e9` (show definitions) is set and the definition is not empty:
fonts MyriadWeb12Bold / MyriadWeb10, duration = def.length() * 3 + 400): returns if no overlay or the balloon is
visible. `SetBoxImages(backdrop_wb_text, wb_text_talk_tail, Point((int)(0.75 * 213.0) = 159, -2))`,
`SetHeader(word, wordFont, -1)`, `SetText(def, defFont, -1)` (both left-justified), `SetColors(gLexSayColors,
4)`, `ClearFooter()`, mTextInsets = (12, 0, 12, 0), `Resize(0, 0, 213, GetPreferredHeight(213))`, `Show(duration)`.

Placement of the balloon is not Lex's: `OverlayWidget::Update` (0x430e20) moves a visible balloon every update to
`((lex.mWidth - b.mWidth) / 2 + lex.mX, lex.mY - b.mHeight + 30)` (slot 14 Move, C int division). Say, Think and
SayDefinition all do nothing while the balloon is visible, so a message never replaces another one.

#### 2.4 Random numbers (Lex::Update step 9) - CRT `rand()` (0x4dc094)

Reached only when the update is not frozen (board+0x3c8 <= 0) and did not return in the glow step, i.e. not
while the mouse rests on an idle Lex and not while the glow fades. In call order:
1. `r1 = rand()`; if `r1 % 500 == 0` and !IsBusy(): PlayAnim(board && board->mBurnt ? 25 : 27, false).
2. `r2 = rand()`; if `r2 % 4000 == 0` and !IsBusy() and (board == NULL or !board->mBurnt):
   `r3 = rand()`; PlayAnim(26 + r3 % 2, false) (r3 >= 0, so r3 & 1).
So 2 draws per update, 3 when r2 % 4000 == 0 and the conditions hold (IsBusy is re-evaluated after step 1; step 1
playing an animation blocks step 2's PlayAnim but r2 is still drawn).

#### 2.5 Timing notes

- PlayAnim resets mAnimTick to 0 and the step check runs in Lex::Update. The Board updates before Lex
  (Board::Update calls `BringToBack(board)` each update and Board::OrderInManagerChanged 0x405240 then puts Lex
  above the board's other widgets and the OverlayWidget just above Lex; the WordBalloon, added when the overlay
  was added, stays below Lex). Steady list order: board, ..., WordBalloon, Lex, OverlayWidget: the balloon is
  drawn under Lex and updated before it; the overlay moves the balloon after Lex's update.
- A row step of duration d is visible for 2d updates (20d ms).

### 3. BigLex (BigLex.cpp)

`class Sexy::BigLex : public Widget`, **sizeof 0x70** (`operator new(0x70)` in the BWGameSelector ctor 0x401600,
which then calls `Resize(70, 231, 133, 123)`), vftable 0x4f9724, 47 slots (0 dtor, 16 Draw, 17 Update).
The big Lex mouth on the game-selector screen.

| Off | Type | Name | Meaning | Ctor |
|---|---|---|---|---|
| +0x58 | BookWormApp* | mApp | | arg |
| +0x5c | int | mAnim | gBigLexAnims row, -1 none | -1 |
| +0x60 | int | mAnimStep | | 0 |
| +0x64 | int | mAnimTick | | 0 |
| +0x68 | int | mFrame | image id, drawn when > 0 | -1 |
| +0x6c | bool | mAnimLoop | | false |

Ctor 0x402df0 also sets mDoFinger = false, mHasAlpha = mHasTransparencies = true. Dtor 0x402e30 / deleting 0x402f30.

**0x402e40 `PlayAnim(int anim, bool loop)`**: as Lex's with table `gBigLexAnims` (0x4f9590, one row):
ee/3, ah/4, mm/4, ah/4, (0)/4, ah/4, (0)/3, ah/7, mm/6, (0)/8 = 94 updates; image 0 = nothing drawn. Called once
by BWGameSelector::Update (0x402190) when its mUpdateCnt == 100 and `app+0x12e5` is false: it sets the flag,
plays sample 46 lex_welcome and PlayAnim(0, false).

**0x402eb0 `Update()` slot 17**: Widget::Update; if mAnim == -1: mFrame = 0 (no MarkDirty); else the same step
logic as Lex (++tick >= 2 * duration; step, frame; loop; on -1: mAnim = -1, mFrame stays -1) and MarkDirty.

**0x402e80 `Draw(g)` slot 16**: if mFrame > 0: DrawImage(images[mFrame], 0, 0).

### 4. WordBalloon (WordBalloon.cpp)

`class Sexy::WordBalloon : public Widget`, **sizeof 0x120** (`operator new(0x120)` in OverlayWidget::AddedToManager
0x430d10), vftable 0x5016c4, 52 slots. One instance, owned by the OverlayWidget (overlay+0x60), shared by every
Lex message. The OverlayWidget creates it with `WordBalloon(app, backdrop_wb_text, wb_text_talk_tail,
Point((int)(0.75 * 213.0) = 159, -2))`, then `Resize(0, 0, 213, GetPreferredHeight(213))` and `AddWidget`.

#### 4.1 Layout

| Off | Type | Name | Meaning | Init |
|---|---|---|---|---|
| +0x58 | std::string | mHeader | first text block (definition word) | "" |
| +0x74 | std::string | mText | second block (main text) | "" |
| +0x90 | std::string | mFooter | third block; only ever cleared | "" |
| +0xac | Insets | mImageInsets | l, t, r, b from the images (4.3) or the plain image's insets | 0 |
| +0xbc | Insets | mTextInsets | extra padding chosen by the caller | 0 |
| +0xcc | Point | mTailPos | tail x, y (y relative to the box bottom) | 0 |
| +0xd4 | BookWormApp* | mApp | | Init |
| +0xd8 | Image* | mPlainImage | whole-image balloon (thought) | **not initialised** before SetBoxImages tests it (benign: 4.3) |
| +0xdc | Image* | mBoxImage | 9-slice box | NULL |
| +0xe0 | Image* | mTailImage | | NULL |
| +0xe4 / +0xe8 / +0xec | Font* | mHeaderFont / mTextFont / mFooterFont | | NULL |
| +0xf0 / +0xf4 / +0xf8 | int | mHeaderJustify / mTextJustify / mFooterJustify | -1 left, 0 centre, 1 right | -1 |
| +0xfc | | padding | | |
| +0x100 | double | mAlpha | current opacity | 0.0 |
| +0x108 | double | mTargetAlpha | | 0.0 |
| +0x110 | double | mFadeSpeed | per update | 0.1 |
| +0x118 | int | mTimer | updates to stay; -1 = until Show(0) | 0 |
| +0x11c | bool | mClicked | dismissed by a click | false |

Colours (Widget mColors via `SetColors(int[][4], 4)`): 0 = backdrop tint, 1 = header, 2 = text, 3 = footer. All
three tables (0x52f8f0 Init, 0x52f428 Say/SayDefinition, 0x52f468 Think) are white, black, black, black.

#### 4.2 Construction

Ctor 0x4372a0 `WordBalloon(BookWormApp* app, Image* box, Image* tail, Point tailPos)`: Widget(); empty strings;
both Insets zero; mTailPos = (0, 0); `Init(app)`; `SetBoxImages(box, tail, tailPos)`.

**0x436260 `Init(app)`**: mApp = app; mHasAlpha = true; mDoFinger = true; box/tail/fonts NULL; justifications -1;
mAlpha = mTargetAlpha = 0.0; mFadeSpeed = 0.1; mTimer = 0; mClicked = false; `SetColors(gWordBalloonColors, 4)`;
`SetVisible(false)`; `SetDisabled(true)`.

Dtor 0x4373a0 (frees the three strings, `~Widget`), deleting 0x437370 (slot 0). Slots 1, 2, 4, 10 (0x4363b0,
0x4363c0, 0x436350, 0x436340) and 13 (0x4334f0, shared with TitleScreen/BWGameSelector) only jump to the Widget
implementation.

#### 4.3 Content setters

- **0x436440 `SetPlainImage(Image* img, Insets insets, bool resize)`**: if mBoxImage or mTailImage is set:
  `SetBoxImages(NULL, NULL, Point(0, 0))`. mImageInsets = insets; mPlainImage = img; if resize and img:
  `Resize(mX, mY, img->GetWidth(), img->GetHeight())`.
- **0x4364d0 `SetBoxImages(Image* box, Image* tail, Point tailPos)`**: if mPlainImage: `SetPlainImage(NULL,
  Insets(0, 0, 0, 0), false)`. mBoxImage = box, mTailImage = tail, mTailPos = tailPos, mImageInsets = (0, 0, 0, 0);
  if box: e = (int)(((double)box->GetHeight() + 0.5) * 0.33), computed twice: mImageInsets.mTop += e,
  mImageInsets.mBottom += e; if tail: mImageInsets.mBottom += tail->GetHeight(). With backdrop_wb_text (27) and the
  talk tail (20): top 9, bottom 29. Left/right stay 0.
- **0x4374e0 `SetHeader(text, font, just)`**, **0x437510 `SetText(text, font, just)`**: assign the string, font and
  justification. **0x437430 `ClearHeader()`**, **0x437480 `ClearFooter()`**: erase the string, font = NULL
  (justification kept). No footer setter exists.
- **0x436310 `SetJustification(a, b, c)`**: mTextJustify = a, mHeaderJustify = b, mFooterJustify = c (only
  called with 0, 0, 0).

#### 4.4 Show, click, update

- **0x436360 `Show(int duration)` slot 49**: SetVisible(true); SetDisabled(false); mTimer = duration;
  mClicked = false; mTargetAlpha = duration != 0 ? 1.0 : 0.0. `Show(0)` is the "hide" call (fade out).
- **0x436420 `MouseDown(x, y, cc)` slot 27**: mTimer = 1; mClicked = true (no Widget::MouseDown call).
- **0x437080 `Update()` slot 17**: Widget::Update; return if mDisabled, or board is NULL, or board->mLex is NULL.
  1. If mTimer > 0 and !board+0x3c0 (bool) and board+0x3c8 <= 0: mTimer = max(mTimer - 1, 0); if mIsOver and
     !mClicked: mTimer = max(mTimer, 50); if mTimer == 0: mTargetAlpha = 0.0.
  2. Fade (t = mTimer after step 1): if mTargetAlpha > mAlpha: mAlpha = min(mAlpha + mFadeSpeed, mTargetAlpha)
     (computed as `1.0 * speed + alpha`, then compared), MarkDirty. Else if mTargetAlpha < mAlpha and t == 0:
     mAlpha = max(mAlpha - mFadeSpeed, mTargetAlpha), MarkDirty. Sequences: up 0.1, 0.2, 0.30000000000000004, 0.4,
     0.5, 0.6, 0.7, 0.7999999999999999, 0.8999999999999999, 0.9999999999999999, 1.0 (11 updates); down from 1.0:
     0.9, 0.8, 0.7000000000000001, ..., 0.10000000000000014, 1.3877787807814457e-16, 0.0 (11 updates).
  3. If mVisible and mTargetAlpha == 0.0 and mAlpha == 0.0 and mTimer == 0: SetDisabled(true), SetVisible(false).
  So `Show(n)`, n > 0: the 11-update fade-in and the n-update countdown start together; while the mouse is over
  the balloon and it was not clicked the countdown cannot drop below 50; at 0 it fades out over 11 updates and
  hides. `Show(-1)` never counts down (only `mTimer > 0` counts): it stays until `Show(0)` or a click.

#### 4.5 Measuring and drawing

- **0x4365d0 `int GetTextHeight(int width, const string& text, Font* font, int just, int lineSpacing)` slot 50**:
  0 if the text is empty or font is NULL. Else a `Graphics(NULL)` with `SetFont(font)`; lineSpacing = -1 means
  `font->GetLineSpacing()`; returns `WriteWordWrapped(g, Rect(0, 0, width, 0), text, lineSpacing, just)` (Widget slot
  37; with a NULL image nothing is drawn).
- **0x436680 `int GetPreferredHeight(int width)` slot 51**: 0 if width <= 0. Inner width `iw = width -
  (mTextInsets.right + mTextInsets.left + mImageInsets.right + mImageInsets.left)`. h = sum over header, text,
  footer of `GetTextHeight(iw, s, font, just, font->GetLineSpacing())` for each block whose string is not empty
  **and** font not NULL; returns h + mTextInsets.bottom + mTextInsets.top + mImageInsets.bottom + mImageInsets.top.
- **0x4363d0 `Draw(g)` slot 16**: only if board and board->mLex exist and mAlpha > 0.0: `DrawPlain(g)` (slot 47)
  when mPlainImage is set, else `DrawBox(g)` (slot 48).
- Common: `a = clamp(mAlpha, 0.0, 1.0)`; colour k = `GetColor(k)` with alpha `(int)(colorAlpha * a)` (int x double,
  truncated). Text rect x = mImageInsets.left + mTextInsets.left, y0 = mImageInsets.top + mTextInsets.top,
  W = mWidth - mTextInsets.right - mImageInsets.right - mImageInsets.left - mTextInsets.left,
  H = mHeight - mTextInsets.bottom - mImageInsets.bottom - mImageInsets.top - mTextInsets.top.
  Each block (header, text, footer, in that order) is drawn only if its string is not empty and its font not NULL:
  SetColor(colour 1 / 2 / 3), SetFont(font), `h = WriteWordWrapped(g, rect, s, font->GetLineSpacing(), just)`,
  rect.mY += h.
- **0x4367a0 `DrawPlain(g)` slot 47**: SetColorizeImages(true); SetColor(colour 0 with alpha); DrawImage(mPlainImage,
  0, 0). Measure total = sum of `GetTextHeight(W, s, font, just, font->GetLineSpacing())` for every non-empty block
  (here **no font NULL test**: a block with text but no font would crash; never happens). rect = (x, y0 + (H -
  total) / 2 (int division toward zero), W, total): text centred vertically. Blocks as above;
  SetColorizeImages(false).
- **0x436c40 `DrawBox(g)` slot 48**: SetColorizeImages(true); c0 = colour 0 with alpha; boxH = mHeight -
  (mTailImage ? tail->GetHeight() : 0); if mBoxImage: SetColor(c0), `DrawImageBox(Rect(0, 0, mWidth, boxH),
  mBoxImage)` (0x44f820: 3x3 tiling of the 27x27 image in 9x9 cells). If mTailImage: `DrawImage(mTailImage,
  mTailPos.x, mTailPos.y + boxH, Rect(0, 0, tailW / 2, tailH))` (the colour is c0 only if a box was drawn). Text
  rect = (x, y0, W, H): top-aligned, no centring. Blocks as above; SetColorizeImages(false).

Resulting sizes: Say(`GT_LEVEL_UP`) is 106 wide (minimum); the long scramble hint and definitions are 213 wide;
height = text height + 38 (9 + 29 image insets) for Say/SayDefinition; Think is always 188x137.

### 5. Definitionator (Definitionator.cpp)

`class Sexy::Definitionator` (no base), **sizeof 0x14** (`operator new(0x14)` in the app's loading code 0x41a8a0,
stored at **app+0x1174**), vftable 0x4fc24c with 2 slots: 0 = 0x41d910 `int GetNumDefinitions()` (`return
mDefinitions.size()`, COMDAT-folded with Font::GetLineSpacingOffset, never called through the vtable), 1 =
deleting dtor 0x41dd80.

| Off | Type | Name | Meaning |
|---|---|---|---|
| +0x04 | BookWormApp* | mApp | ctor arg, unused |
| +0x08 | std::map<string,string> | mDefinitions | VC7: +0x0c head, +0x10 size; default `std::less` (byte compare) |

- **0x41dd30 ctor(app)**, **0x41d8d0 dtor body** (erase all nodes, free the head).
- **0x41d920 `bool Load(const string& path)`**: t0 = GetTickCount(); `ifstream(path, ios::in)` (text mode: the CRT
  turns CRLF into LF); if it did not open: return false. Loop: `getline(buf, 4096, '\n')`; if fail/bad: stop
  when eof is set, otherwise loop again (an over-long line, >= 4095 chars, sets failbit without eof and loops
  forever; popdefs.txt's longest line is 326). For each line `s` (non-empty): `p = s.find_first_of(" \t")`; if
  found: key = s.substr(0, p), def = s.substr(p); trim def of `" \t\r\n"` on the right then the left; if key and
  def are not empty and def != "*": `mDefinitions[key] = def` (a later duplicate overwrites). No case change.
  After eof: close, `OutputDebugStringA(StrFormat("Time to load %d definitions (%s): %d\n", size, path,
  GetTickCount() - t0))`, return size > 0. Called once by the app loader with "popdefs.txt" (relative to the
  game folder); the return value is ignored; `app+0x53c` (loading progress) is incremented around it.
- **0x41d2b0 `string GetDefinition(const string& word)`**: `find(word)`; returns a copy of the value, or "" when
  absent. Exact, case-sensitive. Its only caller is Board 0x40f5b0 with the played word (board+0x1e0, uppercase
  A-Z as in the Board's 5-bit word encoding), when `app+0x12e9` is set and the Definitionator exists; a non-empty
  result goes to `Lex::SayDefinition`.

### 6. Helpers in BigLex.cpp's range (used only by Board)

- **0x402f60 `bool PiecesAdjacent(Piece* a, Piece* b)`** (__cdecl; Board 0x409de0, 0x40a0c0, 0x40b540): dc =
  a.col - b.col, dr = a.row - b.row (Piece +0x30 column, +0x34 row). True when |dc| == 1 and either (b.col % 2 == 0
  and dr is 0 or -1) or (b.col % 2 == 1 and dr is 0 or 1); or when dc == 0 and |dr| == 1. Otherwise false (C `%`,
  columns are >= 0).
- **0x402fd0 `void SetLevelSpeedGlobal(double v)`** (__cdecl; Board 0x407a50, 0x408ab0, 0x40ae90, only in game mode
  `app+0x1254 == 1`, with `(double)table[min(level, 9)]`, table 0x4f9a18 = 550, 500, 450, 400, 350, 300, 250, 200,
  150, 125): stores v into the global double at 0x52e278 (initial 1.0). **Nothing reads 0x52e278** (searched the
  whole .text): dead store; the port can keep it as a no-op.

### 7. Cross-module references

Fields of other classes read or written here (guesses in parentheses):

| Where | Type | Use |
|---|---|---|
| app+0x1160 | Board* | (mBoard) everything; NULL tests in Lex::Update/WordBalloon |
| app+0x11ac | Image*[149] | (mImages) by image id |
| app+0x11b4, +0x11b8, +0x11f0, +0x11f8 | ImageFont* | Verdana12Bold, Verdana14Bold, MyriadWeb10, MyriadWeb12Bold |
| app+0x1174 | Definitionator* | written by the loader 0x41a8a0, read by Board 0x40f5b0 |
| app+0x1178 | player profile* | Lex::Update reads the name via 0x402090 (std::string at +0x10) |
| app+0x12e5 | bool | (welcome already played) BWGameSelector before BigLex::PlayAnim |
| app+0x12e9 | bool | (show definitions option) Board before SayDefinition |
| app slot 41 | PlaySample(int) | Lex::MouseDown(0) |
| WM+0x168 | DWORD | (mLastMouseDownTick) Lex::Update idle latch; port `GetLastMouseDownTick(wm)` |
| board+0x78.. | Piece*[8][7] (grid) | Lex::MouseEnter reads the 7 top-row pointers and piece+0xe8 (type; 3 excluded) |
| board+0x158 | OverlayWidget* | (mOverlay); overlay+0x60 = WordBalloon* |
| board+0x1b1 | bool | (mBurnt: board burnt / scorched backdrop) Lex::Draw, Update |
| board+0x2a8 | StatKeeper* | +0x30 int added to Lex::mHintLevel (also compared in Board::Scramble) |
| board+0x334 | int | (fire-tile count) MouseEnter hint only if < 10 |
| board+0x350 | Lex* | (mLex) WordBalloon::Draw/Update require it |
| board+0x3c0 | bool | (paused) stops the balloon countdown |
| board+0x3c8 | int | (freeze counter) > 0 skips Lex::Update/Mouse*, stops the balloon countdown |
| Board::Scramble 0x40ed70 | | called by Lex::MouseUp |

Lex fields used by other modules: Board writes mTargetX/Y, reads mCurX/Y, mShowShadow, mAnim, mSweating; calls
Move, SetVisible, Save/Load, PlayAnim, SetSweating, Reset, Say, SayDefinition, IsBusy, IsMoving. OverlayWidget
reads mX/mY/mWidth, mSweating, mAnim == 19 and mSweatFrame (draws fx_lex_sweat_seq row mSweatFrame, 48 px rows, at
(lex.mX, lex.mY) unless the board is burnt), calls Think, and moves/hides the WordBalloon (Move, `Show(0)`) and
overrides its mTextInsets/colours after Think. LevelUpWidget::Update (0x42bee0) and FlyingPieceList (0x421150) call
IsBusy / PlayAnim.

### 8. Files and formats

#### 8.1 Lex record in the board's save file (Save/Load slots 47/48, 88 bytes, little-endian, no padding)

| Bytes | Field |
|---|---|
| 0-3 | int mX |
| 4-7 | int mY |
| 8-15 | double mWobble |
| 16-23 | double mWobbleVel |
| 24-27 | int mTargetX |
| 28-31 | int mTargetY |
| 32-39 | double mCurX |
| 40-47 | double mCurY |
| 48-55 | double mVelX |
| 56-63 | double mVelY |
| 64 | bool mShowShadow |
| 65 | bool mIdle |
| 66-69 | int mAnim |
| 70-73 | int mAnimStep |
| 74-77 | int mAnimTick |
| 78-81 | int mFrame |
| 82 | bool mAnimLoop |
| 83 | bool mSweating |
| 84-87 | int mSweatFrame |

Not saved: mHintLevel, mHintPending/Showing/Delay, mGlow.

#### 8.2 popdefs.txt

Game folder, text, CRLF, 2249 lines, one definition per line: `WORD<TAB>(part of speech) text.` Keys are uppercase
A-Z, no duplicates; the separator can be the first space or tab; the value keeps everything after it, trimmed of
spaces, tabs, CR, LF. A value of `*` means "no definition". Two bytes > 0x7e occur (0xa1, 0xd7). 41 lines contain
double spaces, and the CARET/CARETS definitions contain `[^]`, which the colour-code parser of WriteWordWrapped sees
(open question 4).

No registry values or properties are read or written by this group.

### 9. Not ported

Nothing in this group is trial, registration, money, update-check or internet code. The `OutputDebugStringA` load
time message in Definitionator::Load can become a log line or be dropped.

### 10. Port notes

- Lex/BigLex/WordBalloon are top-level widgets (`AddWidget` by Board / BWGameSelector / OverlayWidget).
- 2004 slot 27 `MouseDown(x, y, cc)` and slot 29 `MouseUp(x, y, cc)` = the port's 3-argument overloads; call the
  Widget base first as the original does (the port's WidgetManager sets mIsDown/mIsOver itself).
- `WriteWordWrapped` is the Widget virtual in 2004; use the port's `WriteWordWrapped(g, rect, text, spacing, just)`
  with the same justification. `Graphics(NULL)` measuring = any measuring path that does not draw.
- `rand()` -> `CrtRand()`; GetTickCount -> the framework tick; WM+0x168 -> `GetLastMouseDownTick(mWidgetManager)`.
- Definitionator::Load: open through the framework's file layer; keep the key/value split and trimming exactly.

### 11. Open questions

1. `mWobble`/`mWobbleVel` (+0x70/+0x78) are integrated every update and saved, but no code reads them (only
   double reads at +0x70 in the game are Lex::Update and other classes). `mIdle` (+0xb1) is likewise write-only.
   Both look like removed features (a head wobble and an idle reaction). Kept for the save format.
2. Animation rows 6, 16 and 24 have no caller (16 looks like a long sentence lip-sync, maybe the unused small-Lex
   "welcome"); `lex_wow_1..7` and `lexbig_speak_oo/pb` are never used.
3. 0x402f60 and 0x402fd0 sit in BigLex.cpp's range but only Board uses them; the double at 0x52e278 directly
   follows BigLex.obj's data. Either BigLex.cpp really holds these helpers or the module split is off by two
   functions. The global is never read.
4. Definitions are drawn with WriteWordWrapped, which parses `^` colour codes (CARET/CARETS contain `[^]`) and, in
   the 2004 version, does not skip a run of spaces at a wrap point (41 definitions contain double spaces). The port's
   1.3 wrapper differs on the second point; the exact rendering of `[^]` depends on the 2004 `^` handling (widgets
   group, `WriteString` 0x43b420).
5. `StatKeeper+0x30` (board+0x2a8): added to mHintLevel each time the hint shows; its meaning (scrambles used?) is
   for the board group. If it stays 0, the long "Hey, %s!" hint shows 6 times (levels 0, 0.2, 0.4,
   0.6000000000000001, 0.8, 1.0 are not > 1.0), then always the short one; any value >= 1 shortens that.
6. `piece+0xe8 == 3` excludes a tile from enabling the hint; probably the tile type (board group).
7. WordBalloon's ctor tests mPlainImage (+0xd8) before anything initialises it (operator new does not zero); with
   a garbage non-NULL value it calls `SetPlainImage(NULL, 0, false)`, whose only effects are overwritten
   immediately, so the outcome is the same. The port should initialise it to NULL.

---

## Part 5. Hall of Fame and level-up (FameWidget, LevelUpWidget, LabelWidget)

Behaviour spec for `FameWidget.cpp` (0x421860-0x428f50), `LabelWidget.cpp` and `LevelUpWidget.cpp`
(0x42b730-0x42d050) of the carved game PE (`re/bookworm/bin/BookwormGame.exe`, image base 0x400000).
Proposed header: [fame.h](fame.h); Ghidra names: [fame_symbols.jsonl](fame_symbols.jsonl).

Method: every game function of the three modules was read in the Ghidra export and the x87/`_ftol` code,
the static colour initialisers and the `.rdata` constants in the disassembly. Framework slot names are the
2004 ones (`re/bookworm/fw/*.md`). "verified" = read in the code; "inferred" = meaning deduced from use.

### 0. Summary

| class | sizeof | allocated at | bases (vtables) | what it is |
|---|---|---|---|---|
| FameWidget | **0x2a0** | `operator new(0x2a0)` in BookWormApp::ShowHallOfFame 0x411020 | Widget (0x4fc8dc, 53 slots), ButtonListener at +0x58 (0x4fc8c0), CheckboxListener at +0x5c (0x4fc8b8) | the Hall of Fame screen (local high scores); also the dormant internet high-score page |
| LevelUpWidget | **0xe0** | `operator new(0xe0)` in Board::ShowLevelUp 0x404b40 | Widget (0x4fd894, 47 slots), ButtonListener at +0x58 (0x4fd878) | the "level complete" / "game over" stats panel that drops in, bounces and gets a header and footer |
| LabelWidget | **0x94** | `operator new(0x94)` in GetStringDialog ctor 0x4292d0 | Widget (0x4fd5ec, 47 slots) | a one-line text label with alignment and insets |
| HighScoreEntry (app data, shown here) | 0x40 | value of the app's multisets | none | `{std::string mName +0; std::string mWord +0x1c; int mScore +0x38; bool mIsNew +0x3c}` |

Module boundary correction for INVENTORY.md: `FUN_0042b980` and `FUN_0042b9c0` operate on the LevelUpWidget
(its panel image, callers 0x42be40 / 0x42baa0), so **LabelWidget.cpp is 0x42b730-0x42b980 and LevelUpWidget.cpp
0x42b980-0x42d050**.

What the retail (registered, `NoInternetScores=True`) build does with these classes:

- **Hall of Fame** (FameWidget, mode 0): full-screen `backdrop_fame` with four tables from the app: Classic top
  10 (left column), Action top 10 (right column), the 3 best words and the 3 longest words (centre). Entries
  added by the game just played pulse with a black drop shadow. One `GT_CONTINUE_YES` button. Opening it pauses the
  Board and cross-fades the music to the hall-of-fame theme. Nothing is read or written on disk by FameWidget
  in this path; the tables live in BookWormApp and are persisted by the app in the registry (section 6).
- The internet high-score page, its tab buttons, period checkboxes, the "Submit"/"View internet scores"
  button, the HTTP code, the reply parser, the wait/error dialogs and the hidden RESUBMIT button
  (`thing.txt`) are never reachable: the internet button is only created when
  `BookWormApp::ShowInternetScores()` (0x4152a0) is true, which needs `NoInternetScores=false`. Section 3.5
  describes each piece in one line.
- **LevelUpWidget**: after each level (Board state 5, inferred) and at game over (Board state 2) a 490x390 panel with
  best word, longest word, level time, total time and the rank (picture + name + "Next Rank at N pts") falls
  from the top with two independently bouncing corners (tilted while falling), then a header bar
  (level-up or game-over, with the level number) slides down and a footer bar with a `GT_CONTINUE_YES` button slides
  up. Continue resumes the game (state 6) or, after game over, deletes the saved game and opens the Hall of
  Fame.

No random numbers are drawn anywhere in the three modules (no `rand()`, no `Sexy::Rand`, no MTRand).

### 1. FameWidget

#### 1.1 Layout (sizeof 0x2a0, ctor 0x428520)

| Off | Type | Proposed name | Meaning | Ctor value | Read / written by |
|---|---|---|---|---|---|
| +0x00 | Widget | (base) | 0x58 bytes, see widgets.md | Widget() | |
| +0x58 | vfptr | ButtonListener | 0x4fc8c0: slot 3 ButtonPress 0x4220c0, slot 4 ButtonDepress 0x426c10 | | |
| +0x5c | vfptr | CheckboxListener | 0x4fc8b8: slot 0 CheckboxChecked 0x421f50 | | |
| +0x60 | BookWormApp* | mApp | | ctor arg | everything |
| +0x64 | int | mMode | 0 = local Hall of Fame, 1 = internet scores | 0 | Draw, Resize, ButtonDepress(8), Update (net state 2) |
| +0x68 | int | mTab | internet table: 0 classic, 1 action, 2 best words | 0 | SetTab, DrawInternetScores |
| +0x6c | int | mPeriod | internet period: 0 today, 1 this month, 2 all-time | 0 | SetPeriod, CheckboxChecked, ButtonDepress(0x13-0x15), DrawInternetScores |
| +0x70 | int | mMonth | month of the "this month" table, 1..12 from the reply's `:MONTH`, else 0 | 0 | ParseServerReply, DrawInternetScores |
| +0x74 | int | mNetState | 0 idle, 1 downloading, 2 got scores, 3 download failed, 4 posting, 5 posted, 6 connect failed (code in +0xd0), 7 scores shown | 0 | StartNetTransfer, Update, ButtonDepress(8) |
| +0x78 | bool | (unused) | written only by the ctor | false | ctor |
| +0x7c | std::string | mErrorText | text after `:ERROR` in a reply | "" | ParseServerReply, ShowErrorDialog, Update(5) (compares with "DIRTYNAME", "Resubmit Detected") |
| +0x98 | std::string | mWarningText | text after `:WARNING` | "" | ParseServerReply, ShowWarningDialog, Update |
| +0xb4 | std::string | mWarningHeader | text after `:WARNINGHEADER` | "" | ParseServerReply, ShowWarningDialog |
| +0xd0 | int | mNetErrorCode | HTTPTransfer result 1..9 of a failed post | 0 | Update(4, 6) |
| +0xd4 | bool | mHasGameToSubmit | `board && board->mGameLog.GetDataLen() > 0 && board->mGameStats->mScore(+8) > 0` at construction | see left | Resize, ButtonDepress(8), StartNetTransfer, Update(5) |
| +0xd5 | bool | mGameLogSigned | user name and reg code already appended to the game log | false | StartNetTransfer |
| +0xd8 | HTTPTransfer (0xfc) | mTransfer | internet only | ctor | StartNetTransfer, ButtonDepress, Update |
| +0x1d4 | multiset<HighScoreEntry,ByScore>[3] | mNetClassic[period] | downloaded classic scores (12 bytes each: comparator, head +4, size +8) | empty | AddNetScore, DrawInternetScores, MarkNewPlaces, Update(2) |
| +0x1f8 | multiset<HighScoreEntry,ByScore>[3] | mNetAction[period] | downloaded action scores | empty | same |
| +0x21c | multiset<HighScoreEntry,ByScore>[3] | mNetBestWords[period] | downloaded best words | empty | same |
| +0x240 | Image*[3] | mTabImages | `CopyImage(ihs_tab_names, Rect(0, 23*i, w, 23))`, i = 0..2: tab labels, normal | see left | tab buttons' mButtonImage |
| +0x24c | Image*[3] | mTabOverImages | `CopyImage(ihs_tab_names_hilite, Rect(0, 23*i, w, 23))`, i = 0..2 | | tab buttons' mOverImage |
| +0x258 | Image*[3] | mTabSelectedImages | `CopyImage(ihs_tab_names, Rect(0, 69 + 23*i, w, 23))`, rows 3..5 | | SetTab |
| +0x264 | Image* | mIndicatorOffImage | `CopyImage(ihs_tab_indicator, Rect(0, 0, w, 16))` | | checkboxes |
| +0x268 | Image* | mIndicatorOnImage | `CopyImage(ihs_tab_indicator, Rect(0, 16, w, 16))` | | checkboxes |
| +0x26c | ImagelessButton* | mResubmitButton | "RESUBMIT", id 0, hidden for ever | see 1.3 | Resize, Added/Removed |
| +0x270 | ButtonWidget* | mContinueButton | id 7, image button_fame_continue | see 1.3 | Resize, SetVisible, SetDisabled |
| +0x274 | ButtonWidget* | mInternetButton | id 8, only when ShowInternetScores() | NULL in the retail build | Resize, Update, SetVisible, SetDisabled |
| +0x278 | ButtonWidget*[3] | mTabButtons | ids 0x13, 0x14, 0x15, hidden in mode 0 | | ShowInternetControls, SetTab, Resize |
| +0x284 | Checkbox*[3] | mPeriodBoxes | ids 0x17, 0x18, 0x19, hidden in mode 0 | | ShowInternetControls, CheckboxChecked, Resize, Update(2) |
| +0x290 | WaitDialog* | mWaitDialog | the "CONNECTING" dialog (id 25) while a transfer runs | NULL | ShowWaitDialog, Update, dtor |
| +0x294 | std::list<PlaceInfo> | mNewPlaces | 12-byte items `{int mTab; int mServerPeriod; int mPlace}` from `:PLACE` (alloc +0x294, head +0x298, size +0x29c) | empty | AddNewPlace, MarkNewPlaces, ParseServerReply, dtor |

All verified (ctor/dtor stores and the functions named). +0xd6/+0xd7 are padding.

#### 1.2 Vtable

| slot | address | proposed method | notes |
|---|---|---|---|
| 0 | 0x428f20 | `~FameWidget()` (scalar deleting; body 0x4270d0) | |
| 1 | 0x421cb0 | `AddedToManager(WidgetManager*)` | |
| 2 | 0x421dc0 | `RemovedFromManager(WidgetManager*)` | |
| 4 | 0x421990 | `SetVisible(bool)` | |
| 10 | 0x421950 | `SetDisabled(bool)` | |
| 13 | 0x421a50 | `Resize(int x, int y, int w, int h)` | the layout |
| 16 | 0x421f00 | `Draw(Graphics*)` | mode 0 -> slot 47, mode 1 -> slot 48 |
| 17 | 0x427f30 | `Update()` | |
| 47 | 0x424c30 | new: `DrawLocalScores(Graphics*)` | the Hall of Fame page |
| 48 | 0x425880 | new: `DrawInternetScores(Graphics*)` | not ported |
| 49 | 0x4219e0 | new: `ShowInternetControls(bool)` | SetVisible on the 3 tab buttons and 3 checkboxes |
| 50 | 0x421860 | new: `SetTab(int)` | |
| 51 | 0x421910 | new: `SetPeriod(int)` | |
| 52 | 0x421920 | new: `SetTabAndPeriod(int tab, int period)` | |
| (+0x58) 3 | 0x4220c0 | `ButtonPress(int id)` | not a function in the Ghidra export (no symbol); 14 bytes |
| (+0x58) 4 | 0x426c10 | `ButtonDepress(int id)` | |
| (+0x5c) 0 | 0x421f50 | `CheckboxChecked(int id, bool checked)` | |

All other slots are Widget's (slot 3 OrderInManagerChanged, 5-9, 11, 12, 14, 15, 18-46 inherited).

#### 1.3 Construction and destruction

**FameWidget(BookWormApp* app)** 0x428520. Created only by `BookWormApp::ShowHallOfFame` 0x411020, which first
removes an old one (0x4103a0), stores the new one at app+0x1170, `Resize(0, 0, app->mWidth, app->mHeight)`
(640x480) and `AddWidget`s it (no SetFocus). Callers of ShowHallOfFame: the game selector's Hall-of-Fame button
(BWGameSelector ButtonDepress 0x4014c0) and the LevelUpWidget's Continue after game over (0x42bc90).

In order:
1. Widget(); vfptrs; three empty strings; HTTPTransfer ctor; nine empty multisets; list head; mApp = app;
   mMode = mTab = mPeriod = mMonth = 0; +0x78 = false; mNetErrorCode = 0; mNetState = 0.
2. `mHasGameToSubmit = app->mBoard (+0x1160) && board->mGameLog (Buffer at board+0x160).GetDataLen() > 0 &&
   board->mGameStats (+0x2a8)->(+8) > 0`; mGameLogSigned = false; all image and widget pointers NULL.
3. Image slices with `SexyApp::CopyImage(Image*, const Rect&)` (0x47d9f0, returns a new DDImage):
   ihs_tab_names (app image 46, 133x138) rows y = 0, 23, 46 -> +0x240..+0x248 and y = 69, 92, 115 ->
   +0x258..+0x260; ihs_tab_names_hilite (47) rows 0, 23, 46 -> +0x24c..+0x254; ihs_tab_indicator (48, 17x32)
   y = 0 and 16, height 16 -> +0x264 / +0x268. Width = the source image width.
4. **Continue button**: `new ButtonWidget(7, this+0x58)` (0xe0), `Resize(0, 0, w, h)` of button_fame_continue
   (image 38, 305x42), mDoFinger = true, mButtonImage = button_fame_continue, mHasAlpha = true.
5. Internet button only if `ShowInternetScores()` (0x4152a0: `!GetBoolean("NoInternetScores", true) &&
   (!GetBoolean("NoIHSUpsell", true) || app->mIsRegistered)`): `ButtonWidget(8, this+0x58)`, image
   button_goto_ihs (40, 95x83), same setup. **Retail: not created.**
6. Tab buttons, if their image exists: `ButtonWidget(0x13 + i, this+0x58)`, mButtonImage = mTabImages[i],
   mOverImage = mTabOverImages[i], `Resize(0, 0, w, h)` of mTabImages[i], mDoFinger, mHasAlpha,
   `SetVisible(false)`.
7. Period checkboxes, if both indicator images exist: `new Checkbox(mIndicatorOffImage, mIndicatorOnImage,
   0x17 + i, this+0x5c)` (0x8c), `Resize(0, 0, 17, 16)`, `SetVisible(false)`, mHasAlpha = true,
   `mMouseInsets.mRight = -96` (widget +0x40: the click area extends 96 px right, over the label text).
8. `SetTabAndPeriod(0, 0)` (direct call), then `mPeriodBoxes[0]->SetChecked(true, true)` (runs
   CheckboxChecked(0x17, true): unchecks boxes 1 and 2, SetPeriod(0); no sound because the box is not
   under the mouse).
9. `new ImagelessButton("RESUBMIT", 0, this+0x58, app font 16 MyriadWeb10)` (0xe8, ctor 0x42b500) ->
   mResubmitButton; `SetColor(0, (0,0,0))`, `SetColor(1, (255,0,0))`, `SetColor(3, (100,100,100))`,
   `Resize(0, 0, 100, 20)`, mDoFinger = true, `SetVisible(false)`. Nothing ever shows it.
10. mWaitDialog = NULL.

**~FameWidget()** 0x4270d0 (deleting wrapper 0x428f20): deletes (virtual destructor) the eight images
+0x240, +0x244, +0x248, +0x24c, +0x250, +0x258, +0x25c, +0x260, then +0x254, +0x264, +0x268, then the
widgets +0x26c (resubmit), +0x270..+0x28c (continue, internet, tabs, checkboxes) (they were already removed
from the WidgetManager by RemovedFromManager). Then **clears `mIsNew` (node+0x48 = value+0x3c) of every entry
of the app's four tables** app+0x117c, +0x1188, +0x1194, +0x11a0: the pulsing highlight of new entries lasts
exactly one visit of the Hall of Fame. Then if mWaitDialog: `app->KillDialog(mWaitDialog->mId (+0x100))`;
empties the place list, destroys the multisets, the HTTPTransfer, the strings and the Widget base.
The FameWidget is deleted through SexyApp::SafeDeleteWidget (deferred), see ButtonDepress(7).

#### 1.4 Functions (local path in full)

**AddedToManager(WidgetManager* wm)** 0x421cb0: if app->mBoard: `Board::Pause(true)` (0x405be0: increments the
board's pause count; at 0 -> 1 it stops the level clock, hides the board's pausable widgets and marks dirty).
Then `Widget::AddedToManager(wm)`; `wm->AddWidget` of, in this order and when non-NULL: mResubmitButton,
mContinueButton, mInternetButton, mTabButtons[0..2], mPeriodBoxes[0..2] (all on top of the FameWidget).
Music (app+0x33c, MusicInterface slots): `FadeOut(0, false, 0.008)` then `FadeIn(3, 54, 0.008, false)`
(song 3 from MO3 order 54, the hall-of-fame theme; with the double music Update each fade takes about
0.63 s).

**RemovedFromManager(WidgetManager* wm)** 0x421dc0: `Widget::RemovedFromManager(wm)`; `wm->RemoveWidget` of the
same widgets in the same order; then if app->mBoard: `Board::Pause(false)`.

**SetVisible(bool v)** 0x421990: `Widget::SetVisible(v)`; mContinueButton->SetVisible(v) and
mInternetButton->SetVisible(v) when non-NULL; `ShowInternetControls(v)` (slot 49).

**SetDisabled(bool d)** 0x421950: `Widget::SetDisabled(d)`; same for mContinueButton and mInternetButton.

**ShowInternetControls(bool v)** 0x4219e0: SetVisible(v) on the three tab buttons and the three checkboxes
(when non-NULL).

**SetTab(int tab)** 0x421860: mTab = tab; every tab button's mButtonImage = mTabImages[i]; then the selected
one's = mTabSelectedImages[tab] (tab 0, 1 or 2; other values leave all three normal). No MarkDirty.

**SetPeriod(int p)** 0x421910: mPeriod = p. **SetTabAndPeriod(int t, int p)** 0x421920: virtual SetTab(t),
virtual SetPeriod(p).

**Resize(int x, int y, int w, int h)** 0x421a50 (all `/2` are signed integer divisions):
1. `Widget::Resize(x, y, w, h)`.
2. mResubmitButton->Move(275, 140).
3. mContinueButton->Move((mWidth - btnW) / 2, mHeight - btnH - 16) = (167, 422) at 640x480.
4. mMode 0: if mInternetButton: if !mHasGameToSubmit its mButtonImage = button_goto_ihs (40); else if it is
   button_goto_ihs it becomes button_submit_ihs (42). Then `ShowInternetControls(false)`.
   mMode 1: mInternetButton (if any) gets button_goto_fame (41); `ShowInternetControls(true)`.
5. mInternetButton->Resize(592 - imgW/2, 40 - imgH/2, imgW, imgH) with its current mButtonImage.
6. Tab buttons Move(140 - w/2, 83 - h/2), (322 - w/2, 83 - h/2), (495 - w/2, 83 - h/2).
7. Checkboxes Move(102, 117), (277, 117), (436, 117).

**Draw(Graphics* g)** 0x421f00: mMode 0 -> `DrawLocalScores(g)` (slot 47); mMode 1 -> `DrawInternetScores(g)`
(slot 48); other values draw nothing.

**DrawLocalScores(Graphics* g)** 0x424c30, the Hall of Fame page. Constants:
- `pulse = 255 - abs((mUpdateCnt * 4) % 510 - 255)` (int, C `%`; a triangle wave 0..255 with a period of
  127.5 updates).
- `shadow = Color(0, 0, 0, pulse)`.
- `DrawShadowString(g, x, y, s, colour, isNew, shadow)` = app helper 0x410090: if isNew:
  `g->SetColor(shadow); g->DrawString(s, x + 1, y + 1)`; then always `g->SetColor(colour);
  g->DrawString(s, x, y)`. y is the text baseline.
- `Truncate(g, s, maxW, "...")` = app helper 0x412da0: `n = len; t = s; while (StringWidth(t) > maxW && n > 0)
  { --n; t = s.substr(0, n); } if (n < len) t += "...";` (measured with g's font; the result may exceed maxW
  by the width of "...").
- Every entry is copied (HighScoreEntry copy ctor 0x412080) before use.

Steps:
1. `g->DrawImage(backdrop_fame (app image 21, 640x480), 0, 0)`; `Widget::Draw(g)` (empty base call).
2. `g->SetFont(app font 15, LegacySerifMdSCITCTT12)`; `h = font->GetHeight()` (the layer image height, 18).
3. **Classic top 10** (app+0x117c, iteration order, at most 10, index i from 0): score `"%d"` (StrFormat) drawn
   with DrawShadowString at `x = 96 - StringWidth(score)`, `y = 152 - _ftol(h * -1.5) * i` (the product
   `(double)h * -1.5` is truncated toward zero, then multiplied by i: 152 + 27 i for h = 18), colour
   (255,0,0); then the name truncated to 90 px at x = 106, same y, colour (140,90,0).
4. **Action top 10** (app+0x1188, at most 10): name truncated to 90 px at `x = 538 - StringWidth(truncated)`,
   `y = _ftol(152.0 + (double)h * 1.5 * i)` (one truncation of the whole double expression), colour
   (140,90,0); then the score `"%d"` at x = 545, `y = 152 - _ftol(h * -1.5) * i`, colour (255,0,0). For odd h
   the name and the score of row i differ by up to i/2 px; with h = 18 both are 152 + 27 i.
5. **Best words** (app+0x1194, at most 3, i = 0..2): word (entry.mWord) centred at `x = (mWidth -
   StringWidth(word)) / 2`, `y = 122 + 2h i`, colour (160,60,0); score `"%d"` at `x = 400 - scoreW`,
   `y = 122 + h + 2h i`, colour (160,60,0); name truncated to `140 - scoreW` px at x = 242, same y, colour
   (255,130,80).
6. **Longest words** (app+0x11a0, at most 3): identical with base y 287 instead of 122, colours word
   (160,60,0), number (160,60,0), name (255,130,80); **the number is the length of the word** (`"%d"` of
   `mWord.size()`), not the stored score.
   With h = 18: best words at y 122/140, 158/176, 194/212; longest words at 287/305, 323/341, 359/377.
7. isNew for every string of an entry is that entry's mIsNew (+0x3c).
The page titles ("Classic", "Action", "Best Words"...) are part of backdrop_fame; the code draws none.

Static colours (file-scope `Sexy::Color`s built by dynamic initialisers 0x4f71d0-0x4f7300): 0x534a70
classic score (255,0,0); 0x534a60 classic name (140,90,0); 0x534a90 action score (255,0,0); 0x534aa0
action name (140,90,0); 0x534a80 best word (160,60,0); 0x534ad0 best score (160,60,0); 0x534a50 best name
(255,130,80); 0x534ab0 longest word (160,60,0); 0x534b30 longest length (160,60,0); 0x534ac0 longest name
(255,130,80).

**Update()** 0x427f30:
1. `Widget::Update()` (mUpdateCnt++).
2. If `app->mHasFocus` (app+0x4ff) and `mUpdateCnt % 12 == 0`: MarkDirty (slot 46). This is what animates
   the pulse: about 8.3 redraws per second, the alpha being taken from mUpdateCnt at draw time.
3. Internet button blink (only if mInternetButton): `(mUpdateCnt % 64) < 24` (signed) -> if its image is
   button_submit_ihs (42) it becomes button_submit_ihs_hilite (43) and `Resize(mX, mY, mWidth, mHeight)`
   re-lays out; otherwise if it is the hilite it goes back to 42, same Resize.
4. `switch (mNetState)`: 0 and 7 nothing (the only states of the retail build); 1..6 see 3.5.

**ButtonPress(int id)** 0x4220c0 (+0x58 slot 3): `app->PlaySample(0)` (buttonclick) for every button.

**ButtonDepress(int id)** 0x426c10 (+0x58 slot 4):
1. If mResubmitButton and id == its mId (0): internet resubmit, see 3.5 (unreachable: the button is hidden).
2. id 7 (Continue): music `FadeOut(3, true, 0.008)`, `FadeIn(0, -1, 0.008, false)` (song 0 continues where it
   was, back to full volume); `SetVisible(false)` and `SetDisabled(true)` (virtual, so the buttons too);
   `app->RemoveFameWidget()` (0x4103a0: `wm->RemoveWidget(this)`, `SafeDeleteWidget(this)`, app+0x1170 = NULL);
   then **if app->mGameSelector (app+0x1168) is NULL** `app->ShowGameSelector()` (0x410f00; the game-over path:
   KillDialog(3), FadeOut songs 1..4, creates the BWGameSelector and starts song 0 from order 27 if it is not
   playing). From the selector the selector is still there underneath, so the widget just disappears.
3. id 8 (internet button): see 3.5.
4. ids 0x13, 0x14, 0x15 (tabs): `SetTabAndPeriod(id - 0x13, mPeriod)` (slot 52).
5. Anything else: nothing.

**CheckboxChecked(int id, bool checked)** 0x421f50 (+0x5c slot 0; `this` there is object+0x5c), the period
radio group:
- checked: id 0x17 -> the other two boxes `SetChecked(false, true)`, `SetPeriod(0)`; 0x18 -> boxes 0 and 2,
  `SetPeriod(1)`; 0x19 -> boxes 0 and 1, `SetPeriod(2)`; then if the box that was clicked has mIsOver
  (Widget +0x20): `app->PlaySample(0)`.
- unchecked: if that box has mIsOver, `SetChecked(true, true)` again (clicking the selected box keeps it
  selected; a box unchecked by code, not under the mouse, stays unchecked).

#### 1.5 The internet high-score code (not ported; one line each)

The retail build cannot reach any of this (no internet button, the RESUBMIT button is hidden, mNetState stays 0).

- `StartNetTransfer()` 0x4241d0: state 0 without a game to submit: HTTP GET `GetString("GetScoresURL",
  "http://www.popcap.com/bookworm_getscores.php") + "?clientver=" + app->mProductVersion`, state 1, wait dialog
  a "retrieving scores" status; with a game: once, appends 4 bits (value 15), mUserName and mRegCode to
  the board's game log Buffer, then POSTs `name=<user name, spaces as %20>&data=<log.ToWebString()>&clientver=`
  to `GetString("SubmitScoreURL", ".../bookworm_submitscore.php")`, state 4, a "posting scores" status;
  states 5/6: a fresh GET as in state 0. Also called by BookWormApp::ButtonDepress (0x417080) and
  IHS entry 0x415a20.
- `ShowWaitDialog(const std::string&)` 0x423fd0: KillDialog(25); `new WaitDialog(app, 25, dialog, dialogbutton,
  waitbar, "CONNECTING", text)` (0x120), fonts Verdana18BoldColor / Verdana12Bold, colours 0,1 white, 3 black,
  width 340 centred, AddDialog(25).
- `Update` states: 1 polls `mTransfer.GetResultCode()`; on completion kills the wait dialog, parses the reply
  (failure -> state 3) and shows `:WARNING` text; 2 MarkDirty, state 7, switches to mMode 1 once and selects the
  tab/period that holds an entry marked new (checks that period's box); 3 a connection-failure
  error dialog; 4 polls the post (codes 1..9 -> state 6); 5 parses the post
  reply: "SUCCEEDED" -> MarkNewPlaces, mHasGameToSubmit = false, state 2; `:ERROR DIRTYNAME` -> "SORRY!" dialog
  24 (a "name already taken" message, button "Retry"); "Resubmit Detected" clears
  mHasGameToSubmit; others -> the "could not be reached" error; 6 a "failed to connect"
  error with the code.
- `ParseServerReply(const std::string&)` 0x427650: whitespace-token state machine: `SUCCEEDED`/`FAILED`
  (return value), `:ERROR`, `:WARNING`, `:WARNINGHEADER` (rest of the line into +0x7c/+0x98/+0xb4), `:MONTH n`
  (mMonth, 0 unless 1..12), `:SECTION table period` (period code via 0x4216e0: 0 -> 2, 1 -> 0, 2 -> 1),
  `:PLACE tab period place` (AddNewPlace), `:ENTRY score name` (plus a quoted word in section 2) ->
  AddNetScore; clears mNewPlaces first.
- `AddNetScore(int tab, int period, const std::string& name, int score, const std::string& word)` 0x427550:
  inserts into mNetClassic/mNetAction/mNetBestWords[period]; its range test `period > -1 || period < 3` is
  always true (bug).
- `AddNewPlace(int tab, int serverPeriod, int place)` 0x426ab0: push_back when place > 0.
- `MarkNewPlaces()` 0x4249d0: sets mIsNew on entry number place-1 of table [tab][map(serverPeriod)].
- `ShowErrorDialog(const std::string&, bool useErrorText)` 0x4262b0 / `ShowWarningDialog(const std::string&)`
  0x4264f0: unless dialog 26 / 27 exists: KillDialog via 0x421f30 / 0x421f40, DoDialog(26, modal, "ERROR!", text,
  `GT_DIALOG_OK`, mode 3) or (27, "WARNING!" or mWarningHeader), header font Verdana18BoldColor, mContentInsets
  (36,32,36,32), centred; empties the consumed strings. Default text an "unknown error" message.
- `DrawInternetScores(Graphics*)` 0x425880: backdrop_ihs; period labels "For Today" (124,130), "For <month>"
  (298,130, month names "(MISSING)", "January".."December" at 0x52ee68), "All-Time" (458,130) in green
  (70,184,0) when selected else red (195,42,42) (function statics 0x534a28 / 0x534a38); tabs 0/1: top 20 in two
  columns of 10 (score right-aligned, name); tab 2: 10 best words in two columns of 5 (word lower-cased then
  capitalised, score, "pts", "by", name); a "no scores yet today" line (Scripps18, centred) when the table is
  empty.
- ButtonDepress(8): unregistered -> sets app->mRegSource = "ihs" and shows "SORRY!" dialog 17 (a "register to upload scores" message, buttons `GT_OPTIONS_REGISTER`/"Cancel"); registered, mode
  0: state 0 -> `app 0x415a20` (INTERNET REQUIRED confirmation) or, with a game to submit, `app 0x414080(user)`
  (a GetStringDialog asking for the internet alias);
  state 3 -> retry StartNetTransfer; state 7 -> mMode = 1; mode 1 -> mMode = 0; then re-runs Resize.
- ButtonDepress(RESUBMIT): opens **`thing.txt`** (relative to the current directory = the exe folder) with
  `std::fstream(name, ios::in)`; if open, reads the first line (`getline(..., '\n')`, bytes as they are),
  closes, appends `"&clientver=" + version` and POSTs it to SubmitScoreURL (state 4). A developer back door to
  resend a saved submission; never reachable.
- `StringToLowerInPlace(std::string&)` 0x422f20 (`std::transform` with CRT `tolower`), used by
  DrawInternetScores only.

### 2. LevelUpWidget

#### 2.1 Layout (sizeof 0xe0, ctor 0x42bd50)

| Off | Type | Proposed name | Meaning | Ctor value | Read / written by |
|---|---|---|---|---|---|
| +0x00 | Widget | (base) | mHasAlpha (+0x22) set true by the ctor | | |
| +0x58 | vfptr | ButtonListener | 0x4fd878: slot 3 0x42bc80, slot 4 0x42bc90 | | |
| +0x5c | BookWormApp* | mApp | | ctor arg | all |
| +0x60 | ColorizedButton* | mContinueButton | id 6, created in AddedToManager | NULL | Added/Removed, SetVisible, SetDisabled, Update, ButtonDepress |
| +0x64 | | (padding) | double alignment | | |
| +0x68 | double | mPanelY | y of the panel's top edge = (mLeftY + mRightY) / 2 | -150.0 | Update, Draw |
| +0x70 | double | mPanelAngle | rotation while falling, radians | 0.0 | Update, Draw |
| +0x78 | double | (unused) | never read | 0.03 | ctor |
| +0x80 | bool | mFalling | phase 1 | true | Update, Draw |
| +0x88 | double | mLeftY | left corner y | -150.0 | Update |
| +0x90 | double | mLeftVel | | 0.0 | Update |
| +0x98 | double | mRightY | right corner y | -174.0 | Update |
| +0xa0 | double | mRightVel | | 0.0 | Update |
| +0xa8 | double | (unused) | never read | 189.0 | ctor |
| +0xb0 | bool | (never set) | when true Update stops after phase 1 and Draw draws nothing after the fall; nothing writes it | false | Update, Draw |
| +0xb8 | double | mSlide | header/footer slide factor 0..1 | 0.0 | Update |
| +0xc0 | double | mSlidePhase | += 0.03 per update | 0.0 | Update |
| +0xc8 | int | mHeaderY | | 0 | Update, Draw |
| +0xcc | int | mFooterY | | 0 | Update, Draw |
| +0xd0 | MemoryImage* | mPanelImage | off-screen copy of the painted panel | NULL | CreatePanel, Draw |
| +0xd4 | Graphics* | mPanelGraphics | Graphics on mPanelImage | NULL | CreatePanel, Draw |
| +0xd8 | bool | mPanelDirty | repaint the panel at the next Draw | false | CreatePanel (true), DrawPanel (false) |
| +0xd9 | bool | mLexArrived | Lex finished walking in during board state 5 | false | Update |

All verified. The ctor (no Widget-level changes besides mHasAlpha) takes `BookWormApp*`.

#### 2.2 Vtable

| slot | address | method |
|---|---|---|
| 0 | 0x42beb0 | `~LevelUpWidget()` (scalar deleting; body 0x42be40: DeletePanel, delete mContinueButton, ~Widget) |
| 1 | 0x42bb10 | `AddedToManager(WidgetManager*)` |
| 2 | 0x42bc20 | `RemovedFromManager(WidgetManager*)` |
| 4 | 0x42baf0 | `SetVisible(bool)` |
| 10 | 0x42bad0 | `SetDisabled(bool)` |
| 13 | 0x42baa0 | `Resize(int, int, int, int)` |
| 16 | 0x42cdd0 | `Draw(Graphics*)` |
| 17 | 0x42bee0 | `Update()` |
| (+0x58) 3 | 0x42bc80 | `ButtonPress(int id)` |
| (+0x58) 4 | 0x42bc90 | `ButtonDepress(int id)` |

Non-virtual: `DeletePanel()` 0x42b980, `CreatePanel()` 0x42b9c0, `DrawPanel(Graphics*)` 0x42c210.

#### 2.3 Lifecycle (who creates it)

`Board::ShowLevelUp` 0x404b40 (called from Board 0x408ab0 and Board::Update 0x40bd40): app+0x1330 = true,
Board 0x403ea0 (removes and deletes any old LevelUpWidget immediately), level clock: `board+0x188 = board+0x190
+ (GetTickCount() - board+0x184) / 1000` (seconds this level), `board+0x18c += board+0x188` (total);
`new LevelUpWidget(app)`, board+0x15c = it, `Resize(0, 0, board->mWidth, board->mHeight)`, AddWidget,
SetFocus(it), board+0x3c0 (mLevelUpShown) = true; if app+0x116c (the HelpWidget is open) it copies the
board's mDisabled/mVisible onto it. Board 0x403ea0 removes it later (RemoveWidget + immediate delete).
BookWormApp ShowHelp 0x4101c0 disables and hides it, CloseHelp 0x4102d0 re-enables and shows it when
board+0x3c0 is set.

#### 2.4 Functions

**LevelUpWidget(BookWormApp* app)** 0x42bd50: Widget(); mApp = app; mHasAlpha = true; mFalling = true; the
values of the table above.

**Resize(x, y, w, h)** 0x42baa0: `Widget::Resize`; `CreatePanel()`.

**CreatePanel()** 0x42b9c0: `DeletePanel()`; `mPanelImage = new MemoryImage(app)`;
`mPanelImage->Create(w, h)` of dialog_levelup (app image 37, 490x390); `SetImageMode(true, true)`;
`mPanelGraphics = new Graphics(mPanelImage)`; mPanelDirty = true.
**DeletePanel()** 0x42b980: delete mPanelGraphics, then mPanelImage (virtual dtors), both NULL.

**AddedToManager(wm)** 0x42bb10: `Widget::AddedToManager(wm)`; if no button yet: `new ColorizedButton(6,
this+0x58)` (0xf0, ctor 0x41bf50), `Resize(mWidth, mHeight, w, h)` of bar_footer_continue (image 53, 319x46)
(i.e. parked off-screen at the bottom-right until the slide moves it), mButtonImage = bar_footer_continue,
`SetFont(app font 1, Verdana12Bold)` (slot 47), mDoFinger = true. `wm->AddWidget(button)`. Then moves Lex's
walk target: `lex (board+0x350)->mTargetY (+0x84) -= 6`, `mTargetX (+0x80) -= 26`.

**RemovedFromManager(wm)** 0x42bc20: base; `wm->RemoveWidget(button)` if any; if app->mBoard and its Lex:
Lex target = (60, 210).

**SetVisible(bool)** 0x42baf0 / **SetDisabled(bool)** 0x42bad0: base, then the same on mContinueButton
(no NULL check: only called after AddWidget).

**ButtonPress(int id)** 0x42bc80: `app->PlaySample(0)` (buttonclick).

**ButtonDepress(int id)** 0x42bc90: only if mContinueButton and id == its mId (6): `SetVisible(false)`,
`SetDisabled(true)`, `wm->SetFocus(board)`, Lex target = (60, 210); then if `board->mState (+0x194) == 2`
(game over): `Board::DeleteSavedGame(app->mGameMode (+0x1254))` (0x4078b0, deletes `<save dir>\<user>\classic.dat`
or the action file) and `app->ShowHallOfFame()` (0x411020), return; otherwise `board->mState = 6`.

**Update()** 0x42bee0 (100 updates per second):
1. `Widget::Update()`. Nothing else while mDisabled.
2. **Phase 1, falling** (mFalling): two corners fall independently under gravity and bounce on y = 75, all in
   double precision:
   - `mLeftVel += 0.15; mLeftY += mLeftVel; if (mLeftY > 75.0) { mLeftY = 75.0; mLeftVel *= -0.4; if (mLeftVel >
     -0.5) mLeftVel = 0.0; }` (the velocity multiplied is the one just incremented);
   - the same for mRightVel / mRightY;
   - `mPanelY = (mRightY + mLeftY) * 0.5`;
   - `mPanelAngle = atan2(mRightY - mLeftY, (double)dialog_levelup->mWidth)` (x87 `fpatan`, width 490);
   - if both velocities are exactly 0.0: mFalling = false, mPanelAngle = 0.0;
   - MarkDirty.
   Timeline from the ctor values: left corner bounces at updates 55, 99, 116 (then rests: each later update
   re-adds 0.15, overshoots 75 and is zeroed), right corner at 58, 104, 122, 129; the fall ends at **update
   129** (1.29 s), mPanelY = 75.0. The same count results with 24-bit x87 precision.
3. **Phase 2** (when !mFalling and !(+0xb0)), in the same update the fall ends:
   - Lex/board bookkeeping: if !mLexArrived: when `board->mState == 5 && !Lex::IsBusy(lex)` (0x404340: busy unless
     at its target with no animation, `lex+0xb4 == -1`) mLexArrived = true; else (mLexArrived): when
     `board+0x357` (a flag the app sets when the finished game entered a high-score table, 0x419cd0) is set and
     Lex is not busy, clear it.
   - If `mSlide != 1.0`: `mSlidePhase += 0.03`; `mSlide = mSlidePhase < 1.570795 ? sin(mSlidePhase) : 1.0`
     (53 updates, 0.53 s, to reach 1.0);
     `hh = bar_levelup_header->mHeight` (image 50, 106; the level-up header is used for the size even at game
     over); `mHeaderY = _ftol(hh * mSlide) - hh` (from about -103 to 0);
     `mFooterY = mHeight - _ftol((mHeight - (mPanelImage->mHeight + mPanelY - bar_footer_scroll->mHeight)) *
     mSlide)` (all double, one truncation; bar_footer_scroll is image 52, 468x94: from 480 to 480 - 109 = 371);
     `mContinueButton->Move((mWidth - panelW - 5) + panelW / 2 - btn->mWidth / 2, mFooterY + 27)` (= (231, 398)
     at the end); MarkDirty; return.
   - Else (slide done): the board's word balloon (`board+0x158` OverlayWidget, `+0x60` WordBalloon): if its
     mVisible, call its slot 49 with false (hides Lex's speech balloon; WordBalloon is another group's).

**Draw(Graphics* g)** 0x42cdd0:
1. If mPanelImage, mPanelGraphics and mPanelDirty: `DrawPanel(mPanelGraphics)`.
2. `x0 = mWidth - panelW - 5` (145 at 640 wide).
3. Falling: `g->DrawImageRotated(mPanelImage, x0, _ftol(mPanelY), mPanelAngle, NULL)` (0x450230; rotation centre =
   integer (w/2, h/2) = (245, 195); angle passed as is). Nothing else.
4. Else, unless +0xb0: `cx = x0 + panelW / 2` (390); `hdr = board->mState == 2 ? bar_gameover_header (51) :
   bar_levelup_header (50)` (both 468x106);
   - `g->DrawImage(hdr, cx - hdrW/2 + 4, mHeaderY)` (x = 160);
   - level text `StrFormat("%d", board->mLevel (+0x3c4) + 1)`, font app 3 (Verdana18BoldColor), colour
     (255,255,255,255), `WriteString(g, text, x, y, hdrW, 0 /*centred*/, true, 0, -1)` (Widget slot 36) with
     `x = cx - hdrW/2 + (text.size() == 1 ? 1 : -1)` and `y = mHeaderY + (state == 2 ? 55 : 52)`;
   - `g->DrawImage(mPanelImage, x0, _ftol(mPanelY))`;
   - `g->DrawImage(bar_footer_scroll, cx - hdrW/2 + 4, mFooterY)` (x uses the **header's** width).
5. `Widget::Draw(g)` (empty base call).
Order matters: the header is drawn before the panel, the footer after it.

**DrawPanel(Graphics* g)** 0x42c210, painted once into the 490x390 panel image (coordinates are panel-local;
y are baselines; `WriteString(g, s, x, y, w, 0, true, 0, -1)` centres s in [x, x+w]; font heights h6/h7/h8 =
ScrippsCollegeOS13Bold 20 / ScrippsCollegeOS15Bold 23 / ScrippsCollegeOS18Bold 28 from their strip images):
1. `g->DrawImage(dialog_levelup, 0, 0)`; `Widget::Draw(g)` (empty).
2. `stats = board->mState != 2 ? board->mLevelStats (+0x2ac) : board->mGameStats (+0x2a8)` (StatKeeper*).
3. Strings: `GT_SUMMARY_BEST_WORD`; `stats->mBestWord` (+0x38); ``GT_SUMMARY_POINTS_FORMAT`` of `stats->mBestWordScore` (+0x54);
   `GT_SUMMARY_LONGEST_WORD`; `stats->mLongestWord` (+0x58); ``GT_SUMMARY_LETTERS_FORMAT`` of `stats->mLongestWord.size()` (+0x6c);
   ``GT_SUMMARY_TIME` + FormatDuration(board+0x188)` (0x411ea0) and ``GT_SUMMARY_TOTAL` + FormatDuration(board+0x18c)` (0x411f50)
   (app helpers; FormatDuration 0x411cd0 = `[d:][hh:]mm:ss` with `%02d`, hours shown when hours or days > 0;
   without a board just `GT_SUMMARY_TIME` / `GT_SUMMARY_TOTAL`).
4. Left column, x = 34, w = 206, black (0,0,0) unless noted:
   - font 7: `y = 2 h7 + 18` (64): `GT_SUMMARY_BEST_WORD`;
   - font 8, `y += h8` (92), blue (0,0,255): best word; black;
   - font 6, `y += h6` (112): "(N points)";
   - font 7, `y += 2 h7` (158): `GT_SUMMARY_LONGEST_WORD`;
   - font 8, `y += h8` (186), blue: longest word; black;
   - font 6, `y += h6` (206): "(N letters)";
   - font 7, `y += 2 h7` (252): `g->DrawString(time, 74, y)`; font 7: `g->DrawString(total, 74, y + h7)` (275).
5. Rank: `rank = board->mLevel + (state != 2 ? 1 : 0)`; `next = Board::GetRankPoints(rank)` (0x403880, table
   0x4f97e0, quadratic above 41); name = `gRankNames[min(rank, 31)]` (0x52f368: `GT_RANK_BOOK_BURNER`, `GT_RANK_ENCYCLOPEDIA_SALESMAN`, `GT_RANK_TRAINEE`, ... `GT_RANK_BOOKWORM_SUPREME`, 32 entries).
   Right column x = 256, w = 204, font 7 throughout unless noted:
   - `GT_SUMMARY_YOUR_RANK` at `y1 = h7 + 18` (41), black;
   - name at `y2 = y1 + h7` (64), blue (0,0,255); then black;
   - `y3 = y2 + _ftol(h7 * 0.5)` (75); `g->DrawImage(rankframe (image 55, 177x174), 256 + (204 - frameW)/2, y3 - 2)`
     (269, 73);
   - rank picture from the app's rank strip copy (app+0x124c, a MemoryImage of rankstrip.jpg 150x4800 made by the
     loader 0x41b289): `cells = H / W` (32), `cell = rank` clamped to `cells - 1`; `g->DrawImage(strip, cx, cy,
     Rect(0, cell * W, W, W))` with `cx = 256 + (204 - W)/2` (283), `cy = y3 + 10` (85);
   - if rank > cell (rank 32 and up): `level_up_star` (image 56, 57x51) at `sx = cx - starW/2 + W - 7`, `sy = cy -
     starH/2 + W - 7` (398, 203), then `"%d"` of `rank - cell` in font 16 (MyriadWeb10), black, at `sx + (starW -
     textW)/2 + 1`, baseline `sy + (starH + fontH)/2`;
   - `GT_SUMMARY_NEXT_RANK` at `y4 = cy + W - _ftol(h7 * -1.5)` (85 + 150 + 34 = 269), black, font 7; ``GT_SUMMARY_NEXT_RANK_FORMAT`` of next at
     `y4 + h7` (292).
6. mPanelDirty = false.

### 3. LabelWidget

#### 3.1 Layout (sizeof 0x94, ctor 0x42b8c0)

| Off | Type | Proposed name | Meaning | Ctor value |
|---|---|---|---|---|
| +0x58 | int? | (unknown) | never written by the ctor nor read by the class | uninitialised |
| +0x5c | std::string | mLabel | the text | ctor arg |
| +0x78 | Font* | mFont | **not owned, not duplicated** (the dtor does not delete it) | ctor arg |
| +0x7c | int | mHAlign | -1 left (x = mInsets.mLeft), 0 centre, 1 right | -1 |
| +0x80 | int | mVAlign | -1 top, 0 centre, 1 bottom | -1 |
| +0x84 | Insets | mInsets | mLeft +0x84, mTop +0x88, mRight +0x8c, mBottom +0x90 | (0,0,0,0) (Insets() then Color_rgba(0,0,0,0), which is the folded Insets(l,t,r,b)) |

Colours: `SetColors(gLabelWidgetColors (0x52f334), 1)` = one colour, black (0,0,0,255). Only the GetStringDialog
ctor (0x4292d0, dialog+0x11c) creates labels (then `SetColor(0, dialog->GetColor(0x13))`); BookWormApp 0x414080
(the internet alias prompt) sets `label->mHAlign = -1` and resizes it, BookWormApp::ButtonDepress 0x417080 reads
mLabel.

#### 3.2 Functions

- `LabelWidget(const std::string& theLabel, Font* theFont)` 0x42b8c0; `~LabelWidget()` 0x42b830 (string, ~Widget),
  deleting 0x42b890.
- **Draw(Graphics* g)** 0x42b730 (slot 16; `fh = mFont->GetHeight()`, `sw = mFont->StringWidth(mLabel)`, `/2` signed):
  `x = mHAlign == -1 ? mLeft : mHAlign == 0 ? (mLeft - mRight + mWidth - sw) / 2 : mHAlign == 1 ? mWidth - mRight - sw :
  0`; `y = mVAlign == -1 ? fh + mTop : mVAlign == 0 ? (mTop - mBottom + mHeight - fh) / 2 : mVAlign == 1 ?
  mHeight - mBottom - fh : 0` (y is the baseline: the centred and bottom cases use the full height, not the
  ascent, so "centred" text sits high; reproduce as is); `g->SetFont(mFont); g->SetColor(GetColor(0));
  g->DrawString(mLabel, x, y)`.
- **MarkDirty()** 0x42b870 (slot 46): `mColors[0].mAlpha != 255 ? MarkDirtyFull() : Widget::MarkDirty()`. Linked in
  LabelWidget.cpp and identical to EditWidget's slot 46, which the linker folded into this copy (the framework
  symbol file calls it `Sexy::EditWidget::MarkDirty`; both classes use it).

### 4. Files, registry and properties

**FameWidget, LevelUpWidget and LabelWidget read and write no file in the retail path.** The only file access of
the group is the unreachable `thing.txt` read (section 1.5). The data they display belongs to BookWormApp and is
persisted by it (app group; summarised here because the brief asks for the score formats):

- Key `HKEY_LOCAL_MACHINE\SOFTWARE\<app->mRegKey (+0x40)>\Scores`, four `REG_MULTI_SZ` (type 7) values, written
  by `SaveHighScores` 0x412ec0 (called at the end of `AddHighScores` 0x419cd0) and read by `LoadHighScores`
  0x419080 (from ReadFromRegistry 0x41a540):
  - `Classic` (app+0x117c) and `Action` (app+0x1188): for every entry in table order `name NUL score NUL`, score
    as `"%d"`, then one final NUL (value length = sum + 1). Read: pairs; a missing score is 0; `atoi`.
  - `Best` (app+0x1194): `name NUL word NUL score NUL` per entry (read as triples).
  - `Longest` (app+0x11a0): `name NUL word NUL` per entry; on reading **the score is set to the word's length**
    (AddHighScores inserts with `stats+0x74` instead).
  - Strings are the raw bytes of std::string (Latin-1 as typed); mIsNew is not stored.
  - Sizes: Classic/Action keep 10 entries, Best/Longest 3 (AddHighScores inserts with mIsNew = true, trims to
    the size, and sets `board+0x357` when `find(newEntry)` still succeeds after the trim, i.e. the game made the
    table; for Best/Longest only when the table was over 3 before trimming). Empty tables are pre-filled by
    0x418bd0 (`GT_HS_WORD_DESIGN`, `GT_HS_NAME_DANSMOM`... per Part 1).
  - Port: the framework's `RegistryWriteData`/`RegistryReadData` can carry the identical multi-sz bytes.
- No per-player statistics are shown or stored by this group (the user profiles live behind app+0x1178,
  0x402090 / 0x434800 / 0x434e90, another group).
- Properties read through the group: `NoInternetScores`, `NoIHSUpsell` (0x4152a0), `GetScoresURL`, `SubmitScoreURL`
  (internet only).

### 5. Cross-module references

BookWormApp (app group):
- Fields: +0x60/+0x64 mWidth/mHeight (640x480); +0x33c mMusicInterface; +0x37c mRegSource; +0x3b8 mProductVersion;
  +0x41c mUserName; +0x454 mRegCode; +0x470 mIsRegistered; +0x4ff mHasFocus; +0x1160 Board* mBoard; +0x1168
  BWGameSelector* mGameSelector; +0x1170 FameWidget* mFameWidget; +0x1178 user/profile manager (current name via
  0x402090 / 0x435910); +0x117c/+0x1188/+0x1194/+0x11a0 the four high-score multisets (ByScore x3, ByWordLength);
  +0x11ac `Image** mImages` (149, names at 0x52e410); +0x11b0 `ImageFont* mFonts[19]` (names at 0x52e3c0);
  +0x124c rank strip MemoryImage; +0x1254 game mode (0 classic, 1 action).
- Images used (index: name): 7 dialog, 8 dialogbutton, 21 backdrop_fame, 24 backdrop_ihs, 37 dialog_levelup,
  38 button_fame_continue, 40 button_goto_ihs, 41 button_goto_fame, 42 button_submit_ihs, 43
  button_submit_ihs_hilite, 46 ihs_tab_names, 47 ihs_tab_names_hilite, 48 ihs_tab_indicator, 50
  bar_levelup_header, 51 bar_gameover_header, 52 bar_footer_scroll, 53 bar_footer_continue, 55 rankframe, 56
  level_up_star, 147 waitbar.
- Fonts used (index: file): 1 Verdana12Bold, 3 Verdana18BoldColor, 6 ScrippsCollegeOS13Bold, 7
  ScrippsCollegeOS15Bold, 8 ScrippsCollegeOS18Bold, 15 LegacySerifMdSCITCTT12, 16 MyriadWeb10.
- Sound: PlaySample(0) = `buttonclick` (gSoundNames[0]).
- Virtuals: slot 41 PlaySample(int), 55 DoDialog, 56 GetDialog, 57 AddDialog, 58 KillDialog (dialog ids 17, 24,
  25, 26, 27 internet only).
- Functions: 0x4103a0 RemoveFameWidget, 0x410f00 ShowGameSelector, 0x411020 ShowHallOfFame, 0x4152a0
  ShowInternetScores, 0x415a20 / 0x414080 internet entry points, 0x410090 DrawShadowString, 0x412da0
  TruncateString, 0x411ea0 / 0x411f50 / 0x411cd0 time strings, 0x412080 HighScoreEntry copy ctor, 0x419cd0
  AddHighScores, 0x412ec0 SaveHighScores, 0x419080 LoadHighScores.

Board (board group): +0x158 OverlayWidget* (+0x60 its WordBalloon*, slot 49 called with false), +0x15c
LevelUpWidget*, +0x160 Buffer mGameLog, +0x184/+0x188/+0x18c/+0x190 level clock (start tick, level seconds,
total seconds, accumulated), +0x194 mState (2 game over, 5 level complete with Lex walking, 6 set by Continue
= go on), +0x2a8 game StatKeeper*, +0x2ac level StatKeeper*, +0x350 Lex*, +0x357 bool "new high score" flag,
+0x3c0 bool level-up panel shown (set by ShowLevelUp, tested by CloseHelp 0x4102d0), +0x3c4 mLevel (0-based).
BookWormApp+0x116c is the HelpWidget (ShowHelp 0x4101c0 / CloseHelp 0x4102d0 toggle the LevelUpWidget). Functions 0x405be0 Board::Pause(bool), 0x404340
Lex::IsBusy, 0x403880 GetRankPoints(int), 0x4078b0 DeleteSavedGame(int mode), 0x404b40 ShowLevelUp, 0x403ea0
RemoveLevelUp.

StatKeeper: +8 score, +0x38 std::string best word, +0x54 best word score, +0x58 std::string longest word
(+0x6c its size), +0x74 longest word score (AddHighScores).

Lex: +0x80/+0x84 int walk target x/y, +0x88/+0x90 double position, +0xb4 current animation (-1 none).

Other classes created here: ButtonWidget (0xe0), Checkbox (0x8c), ImagelessButton (0xe8, ctor 0x42b500(label,
id, listener, font)), ColorizedButton (0xf0, ctor 0x41bf50(id, listener)), WaitDialog (0x120, 0x436110),
MemoryImage (0x48), Graphics (0x58), HTTPTransfer (0xfc, member).

### 6. What is not ported

- Everything in 1.5 (internet scores, wait/error/warning dialogs, the register nag for internet scores,
  RESUBMIT / `thing.txt`). Port: keep mMode = 0, do not create mInternetButton (ShowInternetScores() is false
  with the shipped partner.xml), leave mNetState at 0; the tab buttons, period checkboxes and the RESUBMIT button
  can be dropped or kept hidden (they are never visible in mode 0). If the members are kept for layout fidelity,
  an HTTPTransfer stub is not needed when the internet branches are compiled out.
- The registered build takes: FameWidget local page only; LevelUpWidget fully.

### 7. Port notes

- Rotation: `DrawImageRotated(img, x, y, rot)` with the 2004 integer centre (w/2, h/2); the port's int overload
  must not use w/2.0f (graphics.md).
- The panel MemoryImage is drawn into once (Create, SetImageMode(true, true), DrawImage of a transparent-edged GIF
  onto cleared ARGB, then text): blending onto a transparent destination must give the 2004 alpha so that the
  panel's soft edges look the same when blitted and rotated (graphics group).
- `_ftol` = truncation toward zero; keep the exact expressions (section 1.4 step 4, section 2.4 Update/DrawPanel).
- Font heights are the strip image heights (LegacySerif 18, Scripps 20/23/28, MyriadWeb10 15); call GetHeight().
- Music fades rely on the double `mMusicInterface->Update()` in BookWormApp::UpdateFrames.

### 8. Open questions

1. LevelUpWidget +0x78 (0.03), +0xa8 (189.0) and +0xb0 (bool) are initialised but never read/written elsewhere in
   the binary as far as a scan of `[reg+0x78/0xa8/0xb0]` and of every `board+0x15c` use shows; +0xb0 would freeze
   the widget after the fall. Leftovers or set through an unseen alias.
2. FameWidget +0x78 (bool) is written only by the ctor; LabelWidget +0x58 is never written by its ctor (4 bytes of
   unknown purpose).
3. Board state numbers 5 and 6 and the purpose of board+0x357 (cleared here once Lex is idle) need the Board
   group's names; WordBalloon slot 49's meaning (called with false to hide the balloon) needs the WordBalloon
   group.
4. Whether rank > 31 can occur: GetRankPoints extends beyond 41 levels, so the "star + number" overlay is real for
   very long games; not verified in play.
5. The order of the five header-static colours (0x534ae0..0x534b20: red, (0,148,255), yellow, green, magenta)
   and the "players" string (0x52ee48) initialised just before FameWidget's colours: they look like statics of a
   shared header compiled into this TU and are not used by FameWidget.
6. x87 precision (app group question): the level-up fall gives 129 updates at both 53- and 24-bit precision;
   mPanelAngle values would differ in the last bits only.

---

## Part 6. Screens and small UI classes

Group "screens": TitleScreen.cpp (title / loading screen, plus the user-profile manager linked in the same
range), BWGameSelector.cpp (main menu), HelpWidget.cpp, CreditsDialog.cpp, OverlayWidget.cpp (the board's
overlay layer), ColorizedButton.cpp, ImagelessButton.cpp, ShadowFont.cpp and WaitDialog.cpp of the carved
game PE (`re/bookworm/bin/BookwormGame.exe`, image base 0x400000, MSVC 7.0). Written from the Ghidra
export (`re/bookworm/gh/`) and checked against the disassembly wherever Ghidra hid x87 operands, `_ftol`
arguments or stack arguments. Framework map: `re/bookworm/fw/{widgets,graphics,app}.md`.
Header: `screens.h`; Ghidra names: `screens_symbols.jsonl`.

Closest finished analogues: Big Money's `TitleScreen`, `GameSelector`, `HelpWidget`, `CreditsDialog`,
`OverlayWidget` (`bigmoney-1912c8/games/bigmoney/src/`, spec `re/bigmoney/analysis/screens.md`). The
BookWorm TitleScreen and CreditsDialog are near copies of Big Money's (same fields and formulas).

### Conventions

- Offsets are absolute from the object start, hex. `app` is the `BookWormApp*` (a `SexyApp`, 0x618 bytes,
  plus game data) that every class keeps.
- `IMG[n]` = the app image table: `Image** mImages` at `app+0x11ac`, entry `n` at `*(app+0x11ac) + 4n`,
  loaded from `images\<name>` with the name table at 0x52e410 (149 names). `FONT[n]` = `ImageFont*` at
  `app+0x11b0 + 4n`, loaded from `fonts\<name>` with the table at 0x52e3c0 (19 names). Fonts 0-4 and
  images 0-16 are loaded in `BookWormApp::Init` (0x414a10) before the title screen exists; the others by
  the loading thread (0x41a8a0).
- Widget slot numbers are the 2004 ones (`widgets.md`): 1 AddedToManager, 2 RemovedFromManager,
  3 OrderInManagerChanged, 4 SetVisible, 5 SetColors([][4]), 6 SetColors([][3]), 7 SetColor, 10
  SetDisabled, 12 Resize(Rect), 13 Resize(x,y,w,h), 14 Move, 16 Draw, 17 Update, 23 MouseEnter, 24
  MouseLeave, 27 MouseDown(x,y,cc), 30 MouseUp(x,y), 35 WriteCenteredLine(g,y,s), 37 WriteWordWrapped,
  45 MarkDirtyFull, 46 MarkDirty; ButtonWidget 47 SetFont; Dialog 47 SetButtonFont, 48 SetHeaderFont,
  49 SetLinesFont, 50 GetPreferredHeight, 51 IsModal. ButtonListener (at +0x58 of the widgets): 2
  ButtonPress(id, cc), 3 ButtonPress(id), 4 ButtonDepress(id).
- App virtuals used: +0x58 SafeDeleteWidget, +0x84 Shutdown, +0xa4 PlaySample(id), +0xcc
  GetLoadingThreadProgress, +0xdc DoDialog, +0xe0 GetDialog, +0xe4 AddDialog, +0xe8 KillDialog, +0x10c
  BookWormApp slot 67 (here: TitleScreenDone). Field names follow `Part 1` (framework) and this file
  (BookWormApp data, see Cross-module references).
- 100 updates per second. `mUpdateCnt` (+0x4c) is incremented by `Widget::Update` (0x4378d0), which every
  Update override here calls first (directly or via `Dialog::Update` 0x4386c0).
- x87: MSVC 7 runs the FPU at 53-bit precision, so formulas are plain `double` arithmetic unless noted;
  `_ftol` truncates toward zero. `fsin`/`fcos` are x87 instructions (the port uses `sin`/`cos`).
- **RNG: none.** No function in this group draws a random number (no `rand`, `Sexy::Rand` or MTRand).
- Ported build = registered build: partner.xml `NoReg=True` makes `app->mIsRegistered` (+0x470) and
  `app->mBuildUnlocked` (+0x471) both true; partner.xml also sets `ShowTrialStatus=False`,
  `LoadingBarY=330`, `NoInternetScores=True`.

### Images, fonts and sounds used by this group

| n | +off | file (`images\`) | size | used by |
|---|---|---|---|---|
| 0 | 0x00 | backdrop_title.jpg | 640x480 | TitleScreen background |
| 1 | 0x04 | backdrop_title_loadmeter.gif | 388x45 | TitleScreen bar background |
| 3 | 0x0c | logo_title.gif | 583x184 | TitleScreen logo |
| 4 | 0x10 | title_loadmeter.gif | 389x45 | TitleScreen bar fill |
| 5 | 0x14 | title_stamp_trial.gif | 165x69 | TitleScreen (trial, ShowTrialStatus only) |
| 6 | 0x18 | title_stamp_registered.gif | 131x110 | TitleScreen (ShowTrialStatus only; off in retail) |
| 7 | 0x1c | dialog.gif | 240x240 | CreditsDialog / WaitDialog frame; OverlayWidget pause box |
| 8 | 0x20 | dialogbutton.gif | 90x31 | CreditsDialog / WaitDialog button |
| 19 | 0x4c | backdrop_bonus.gif | 204x33 | OverlayWidget bonus-word banner |
| 20 | 0x50 | backdrop_menu.gif | 640x480 | BWGameSelector background |
| 22 | 0x58 | backdrop_help.gif | 640x480 | HelpWidget background |
| 23 | 0x5c | backdrop_help_box.gif | 381x94 | HelpWidget topic text box |
| 27 | 0x6c | backdrop_wb_text.gif | 27x27 | OverlayWidget's WordBalloon frame |
| 28/29 | 0x70/0x74 | gameselect_action(_over).gif | 249x77 | BWGameSelector Action button |
| 30/31 | 0x78/0x7c | gameselect_classic(_over).gif | 249x77 | BWGameSelector Classic button |
| 32/33 | 0x80/0x84 | gameselect_hall(_over).gif | 199x99 | BWGameSelector Hall of Fame button |
| 34/35 | 0x88/0x8c | gameselect_quit(_over).gif | 199x99 | BWGameSelector Quit button |
| 36 | 0x90 | gameselect_bubble.gif | 261x143 | BWGameSelector speech bubble |
| 39 | 0x9c | button_help_continue.gif | 197x19 | HelpWidget continue button |
| 44/45 | 0xb0/0xb4 | button_help_topic(_hilite).gif | 81x18 / 111x18 | HelpWidget topic buttons |
| 53 | 0xd4 | bar_footer_continue.gif | 319x46 | LevelUpWidget's ColorizedButton |
| 72 | 0x120 | fx_fireline.gif | 640x80 | OverlayWidget fire column |
| 73 | 0x124 | fx_lex_sweat_seq.gif | 96x384 (8 frames of 48) | OverlayWidget Lex sweat |
| 99/100/101 | 0x18c/0x190/0x194 | bonusarrow_left/right/top.gif | 22x26 / 32x27 / 30x41 | OverlayWidget banner arrows; 100 also copied by HelpWidget |
| 125 | 0x1f4 | wb_text_talk_tail.gif | 40x20 | OverlayWidget's WordBalloon tail |
| 146 | 0x248 | checkboxes_help.gif | 24x12 | HelpWidget checkbox (two 12x12 halves) |
| 147 | 0x24c | waitbar.gif | 32x16 | WaitDialog |
| 148 | 0x250 | clouds (only `_clouds.jpg` 512x512 exists) | 512x512 | CreditsDialog (white image whose alpha is the jpg, SAF's lone-alpha-file rule) |

Fonts (`fonts\`): FONT[0] +0x11b0 Verdana10Bold, [1] +0x11b4 Verdana12Bold, [2] +0x11b8 Verdana14Bold,
[3] +0x11bc Verdana18BoldColor, [4] +0x11c0 Lockergnome14Outline, [9] +0x11d4 Lockergnome18, [11] +0x11dc
CourierFinalDraft20Bold, [14] +0x11e8 LockergnomeSmallColor, [15] +0x11ec LegacySerifMdSCITCTT12, [16]
+0x11f0 MyriadWeb10, [17] +0x11f4 MyriadCondensedWeb11.

Sounds (`app->PlaySample(id)`, names from `gSoundNames` 0x52e668): 0 `buttonclick` (every button press
in this group), 46 `lex_welcome` (game selector, once per run).

### Screen flow (who creates what)

- `BookWormApp::Init` (0x414a10) ends with `app+0x1250 = true`, `new TitleScreen(app)` (0xc8 bytes) into
  `app->mTitleScreen` (+0x1164), `Resize(0, 0, mWidth, mHeight)`, `mWidgetManager->AddWidget`.
- Title `GT_TITLE_CLICK_TO_PLAY` (id 1) -> `BookWormApp::TitleScreenDone` (slot 67, 0x414ef0): RemoveWidget
  + SafeDeleteWidget the title screen, `mTitleScreen = NULL`; if the user manager's current user name is
  not empty, `LoadUserSettings` (0x413a40); then `ShowGameSelector` (0x410f00).
- `ShowGameSelector` (0x410f00): KillDialog(3); music FadeOut(song i, stop, 0.01) for i = 1..4;
  RemoveGameSelector (0x410360); `new BWGameSelector(app)` (0xb0) into `app+0x1168`, `Resize(0, 0, mWidth,
  mHeight)`, AddWidget; if a Board exists (`app+0x1160`): `Board::FUN_00405cd0`, RemoveWidget(board),
  `selector->StartOpenTransition()` (slot 48; the board is deleted when the transition ends); if song 0
  is not playing: FadeIn(0, order 27, 0.005, loop).
- Selector Classic/Action -> `SelectGameMode(mode)` (0x4153c0): sets `app+0x1258 = mode`; if the user has
  a saved game for that mode, DoDialog(5) `GT_CONTINUE_HEADER` (Continue / New Game, both end in
  `selector->StartGame()`); else `selector->StartGame()` (slot 47) -> `app+0x1254 = mode`,
  `BookWormApp::StartGameForUser` (0x413c80: LoadUserSettings, `StartGame` 0x410d10 (new Board), and `ShowHelp` when
  `app->mShowHelp`). StartGame adds the board at the back and calls `selector->StartCloseTransition()`
  (slot 49), which removes the selector when done.
- Selector Hall of Fame -> `ShowHallOfFame` (0x411020); Quit -> `ShowQuitDialog` (0x415660, dialog `GT_QUIT_HEADER`); `GT_SELECTOR_NOT_YOU` -> `ShowSelectUserDialog` (0x410520, dialog 19).
- `ShowHelp` (0x4101c0, also Options button id 33): `new HelpWidget(app)` (0xa0) into `app+0x116c`,
  `Resize(mWidth, mHeight, mWidth, mHeight)`, AddWidget, `Resize(0, 0, mWidth, mHeight)`, and hides /
  pauses the board. Continue -> `HideHelp` (0x4102d0).
- `ShowCredits` (0x4106c0, Options button id 35): `new CreditsDialog(app)` (0x138), colours 0 and 1
  white, 3 black, `Resize(0, 47, 388, 396)`, `Move((mWidth - 388 + 233)/2, 47)` = (242, 47),
  AddDialog(16). DONE (id 2016) -> app ButtonDepress default -> KillDialog(16).
- `Board::AddedToManager` (0x403730): `new OverlayWidget(app)` (0x90) into `board+0x158`,
  `Resize(0, 0, board.mWidth, board.mHeight)`, AddWidget (then the Lex at board+0x350).
- `LevelUpWidget::AddedToManager` (0x42bb10): `new ColorizedButton(6, this+0x58)` (0xf0).
- `FameWidget` ctor (0x428520): `new ImagelessButton("RESUBMIT", 0, this+0x58, FONT[16])` (0xe8);
  `FameWidget` 0x423fd0: `new WaitDialog(app, 25, IMG[7], IMG[8], IMG[147], "CONNECTING", text)` (0x120).
  Both belong to the internet high-score UI (not ported).

---

### ShadowFont (ShadowFont.cpp 0x4330c0-0x433300)

`class ShadowFont : public ImageFont`, sizeof **0x60** (`operator new(0x60)` in TitleScreen ctor and in
Duplicate), vtable 0x50117c (23 slots: ImageFont's with 0, 7, 10, 11 replaced). An ImageFont that draws
itself twice: a solid-colour copy at an offset, then the text.

| off | type | name | meaning | init |
|---|---|---|---|---|
| +0x00..+0x47 | ImageFont | | (2004 ImageFont is 0x48: no mForceScaledImagesWhite) | copy of the source font |
| +0x48 | Color | mShadowColor | shadow colour | ctor arg |
| +0x58 | Point | mShadowOffset | shadow offset (x +0x58, y +0x5c) | ctor arg |

- **0x433250 `ShadowFont(const ImageFont& theFont, const Color& theShadowColor, const Point& theOffset)`**:
  `ImageFont(const ImageFont&)` (0x462ec0, shares the FontData), stores colour and offset.
- **0x433200 `ShadowFont(const ShadowFont&)`**: ImageFont copy, copies +0x48..+0x5f.
- **0x4332a0 `Duplicate()` (slot 11)**: `new ShadowFont(*this)` (0x60).
- **0x4330c0 `~ShadowFont`** = `~ImageFont` body; 0x4331d0 is the scalar deleting wrapper (slot 0).
- **0x433160 `DrawString(Graphics* g, int x, int y, const string& s, const Color& c, const Rect& clip)`
  (slot 10)**: if `mShadowOffset.mX != 0 || mShadowOffset.mY != 0`: non-virtual
  `ImageFont::DrawStringEx(g, x + dx, y + dy, s, mShadowColor, &clip, NULL, NULL)`; then
  `ImageFont::DrawStringEx(g, x, y, s, c, &clip, NULL, NULL)`. The shadow ignores the text colour's alpha.
- **0x4330d0 `StringWidth(const string& s)` (slot 7)**: `Graphics aG(NULL)` (static 0x0 image, draws
  nothing); `width` from the virtual `DrawStringEx` (slot 13) `(&aG, 0, 0, s, Color(), NULL, NULL,
  &width)`; returns `abs(mShadowOffset.mX) + width`. (Width therefore includes kerning exactly as drawn.)

Instances: TitleScreen +0xb8/+0xbc/+0xc0 (see there). Port: a normal subclass of the port's ImageFont
(`ImageFont(const ImageFont&)` and `DrawStringEx` exist); `graphics.md` section 14.

---

### TitleScreen (TitleScreen.cpp 0x433300-0x434800)

`class TitleScreen : public Widget, public ButtonListener`, sizeof **0xc8** (`operator new(200)` in
BookWormApp::Init 0x414a10), vtables 0x5011fc (49 slots) and 0x5011dc (+0x58). Same design as Big Money's
TitleScreen: background, logo sliding down, loading bar sliding up and filling, the play link appearing
when loading is done. Registered build: no stamp (ShowTrialStatus false), no register link, no trial text.

#### Layout

| off | type | name | meaning | ctor | read / written by |
|---|---|---|---|---|---|
| +0x58 | vfptr | | ButtonListener (0x5011dc) | | |
| +0x5c | BookWormApp* | mApp | | arg | all |
| +0x60 | int | mLoadingBarY | property `LoadingBarY` (default 389; partner.xml 330) | GetInteger | Draw, Update |
| +0x64 | | (pad) | | | |
| +0x68 | double | mLoadProgress | loading fraction 0..1, exactly 1.0 when done | 0 | Update, DrawLoadingBar, Draw |
| +0x70 | double | mNagProgress | trial nag timer, += 0.01/mNagSeconds per update, max 1.0 | 0 | Update |
| +0x78 | bool | mLoadDone | bar complete, play link shown | false | Update |
| +0x7c | int | mShownTimesPlayed | trial plays counter shown in nag text; only MarkDirty here | see ctor | Update |
| +0x80 | bool | mWantRegister | register link clicked; Update opens the register dialog | false | ButtonDepress, Update |
| +0x81 | bool | mTrialExpired | always false when registered; copied to `app->mTimedOut` | false | Update, Draw, ButtonDepress |
| +0x84 | int | mNagSeconds | 10, or mTimesPlayed + 5 after 10 plays, 1 when registered | see ctor | Update |
| +0x88 | int | mMaxPlays | property `MaxPlays` (default 10) | GetInteger | Update, Draw |
| +0x8c | int | (unused) | never written or read | - | - |
| +0x90 | int | mLogoY | logo y, -4*184 -> 0 | -4 * h(IMG[3]) | Update, Draw |
| +0x94 | | (pad) | | | |
| +0x98 | double | mLogoAngle | += 0.016 per update from update 25 | 0 | Update |
| +0xa0 | int | mBarDY | bar y offset, 2*app->mHeight -> 0 | 2 * app+0x64 | Update, Draw, DrawLoadingBar |
| +0xa4 | | (pad) | | | |
| +0xa8 | double | mBarAngle | += 0.025 per update from update 50 | 0 | Update |
| +0xb0 | HyperlinkWidget* | mRegisterLink | id 0 `GT_TITLE_REGISTER_NOW` | new | |
| +0xb4 | HyperlinkWidget* | mPlayLink | id 1 `GT_TITLE_CLICK_TO_PLAY` | new | |
| +0xb8 | ShadowFont* | mPlayFont | FONT[4] Lockergnome14Outline, shadow (255,0,0,255) at (1,1) | new | play link |
| +0xbc | ShadowFont* | mTextFont | FONT[1] Verdana12Bold, shadow (0,0,0,255) at (1,1) | new | welcome text, register link |
| +0xc0 | ShadowFont* | mVersionFont | FONT[0] Verdana10Bold, shadow (0,0,0,255) at (1,1) | new | version text |
| +0xc4 | | (pad to 0xc8) | | | |

#### TitleScreen::TitleScreen(BookWormApp* theApp) - 0x434380

1. `Widget()`, vtables, `mApp = theApp`.
2. Fonts: `mPlayFont = new ShadowFont(*FONT[4], Color(255,0,0,255), Point(1,1))`, `mTextFont = new
   ShadowFont(*FONT[1], Color(0,0,0,255), Point(1,1))`, `mVersionFont = new ShadowFont(*FONT[0],
   Color(0,0,0,255), Point(1,1))`.
3. `mMaxPlays = GetInteger("MaxPlays", 10)`; `mLoadingBarY = GetInteger("LoadingBarY", 389)`.
4. `mLoadProgress = mNagProgress = 0.0`; `mLoadDone = mWantRegister = mTrialExpired = false`.
5. `mNagSeconds`: if `!mIsRegistered && !mBuildUnlocked`: `mTimesPlayed < 11 ? 10 : mTimesPlayed + 5`;
   else 1 (registered).
6. `mShownTimesPlayed`: if `app+0x1340 < 1`: `mTimesPlayed < 2 ? 0 : (mTimesPlayed < 10 ? mTimesPlayed :
   2)`; else `mTimesPlayed`. (Trial display only.)
7. `mRegisterLink = new HyperlinkWidget(0, this+0x58)` (0x108); `mUnderlineSize (+0x100) = 0`;
   `SetFont(mTextFont)` (slot 47, duplicates); `mColor (+0xe0) = (32,255,32)`;
   `SetVisible(!mIsRegistered)`; `mDoFinger = true`.
8. `mPlayLink = new HyperlinkWidget(1, this+0x58)`; `mUnderlineSize = 0`; `SetFont(mPlayFont)`;
   `SetVisible(false)`; `mColor = (255,255,0)`; `mDoFinger = true`. (mOverColor stays the framework's
   white.)
9. `mLogoAngle = 0.0`; `mLogoY = -4 * h(IMG[3])` = -736; `mBarAngle = 0.0`; `mBarDY = app->mHeight * 2`
   = 960.
10. Calls `Update()` once (positions the links; mUpdateCnt becomes 1).

#### ~TitleScreen - 0x433300 (0x433540 scalar deleting, slot 0)

Deletes mPlayFont, mTextFont, mVersionFont, mRegisterLink, mPlayLink (in that order), then deletes and
NULLs the title-only app images IMG[4], IMG[1], IMG[0], IMG[3], IMG[6], IMG[5]; `~Widget`.

#### AddedToManager / RemovedFromManager - 0x433470 / 0x4334b0 (slots 1 / 2)

Widget base, then `AddWidget` / `RemoveWidget` of mRegisterLink and mPlayLink (both above the screen).
Slot 13 Resize = 0x4334f0, a shared thunk to `Widget::Resize(x,y,w,h)` (also BWGameSelector, WordBalloon).

#### ButtonListener

- **0x401450 `ButtonPress(int id, int cc)` (+0x58 slot 2; shared with BWGameSelector)**:
  `app->PlaySample(0)` (buttonclick).
- **0x433500 `ButtonDepress(int id)` (+0x58 slot 4)**: id 0 (register link): `mWantRegister = true`.
  Otherwise (id 1, play link): `mTrialExpired ? app->Shutdown() : app->TitleScreenDone()` (slot 67).

#### Update() - 0x433e80, slot 17

In order:
1. `Widget::Update()`.
2. `mNagProgress += 0.01 / (double)mNagSeconds`; if `> 1.0` set 1.0.
3. If `app+0x1340 == 0`: `v = _ftol(mTimesPlayed * mNagProgress)`; if `v <= mTimesPlayed && v >
   mShownTimesPlayed`: `mShownTimesPlayed = v`, MarkDirty. (Trial display only.)
4. Every 8 updates (`mUpdateCnt % 8 == 0`): `c = 32 - |((4*cnt) % 32) - 16|`, `g = 255 - |((4*cnt) % 382)
   - 191|`; `mRegisterLink->mColor = Color(c, g, c)`; MarkDirty. (Green pulse of the hidden register
   link; harmless when registered.)
5. Logo: if `mUpdateCnt >= 25 && mLogoY != 0`: `mLogoAngle += 0.016`; if `mLogoAngle < 1.570795`:
   `mLogoY = -_ftol(h(IMG[3]) * (1.0 - sin(mLogoAngle)))`, else `mLogoY = 0`; MarkDirty. The first step
   jumps from -736 to -181 (184 * (1 - sin 0.016)); 0 at the 99th step (update 123).
6. Bar: if `mUpdateCnt >= 50 && mBarDY != 0`: `mBarAngle += 0.025`; if `< 1.570795`: `mBarDY =
   _ftol(3 * h(IMG[1]) * (1.0 - sin(mBarAngle)))` (3*45 = 135; first step 960 -> 131), else 0. Then
   `w = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel)`; `mRegisterLink->Resize(331 - (w+4)/2,
   mLoadingBarY - mRegisterLink->mHeight - 10 + mBarDY, w + 4, 22)`; MarkDirty. (Only inside this
   block.)
7. Trial (skipped when `gSexyAppBase->mBuildUnlocked`, i.e. always in the port): recompute
   mTrialExpired (`app+0x1340 > 0 ? app+0x1340 - mTimesPlayed <= 0 : mTimesPlayed >= mMaxPlays`, never
   when registered); when it turns true hide both links, and at update 250 (or later, once no dialog 4
   exists) with no dialog 2: `app->mRegSource = "timeout"`, `ShowTrialExpiredDialog()` (slot 47).
8. Play link (every update): label = `GT_TITLE_CLICK_TO_PLAY`; `w = font->StringWidth(label) + 8`, `h =
   font->GetHeight() + 4` (font = the link's duplicated mPlayFont); `mPlayLink->Resize(331 - w/2,
   mLoadingBarY + h(IMG[4]), w, h)` (331 = bar centre 137 + 388/2; y = 330 + 45 = 375 in retail).
9. Register link label = `GT_TITLE_REGISTER_NOW`, or `GT_TITLE_ONE_MOMENT` when
   `!mBuildUnlocked && mWantRegister && !mIsRegistered`.
10. If `!mLoadDone && mUpdateCnt % 4 == 0`: `mLoadProgress = app->GetLoadingThreadProgress()`; if
    `mLoadProgress == 1.0 && app->mLoaded (+0x526)`: `mPlayLink->SetVisible(true)`, `mLoadDone = true`,
    `mLoadProgress = 1.0`. MarkDirty (every 4th update while not done).
11. `mRegisterLink->SetVisible(!mIsRegistered)` (every update).
12. If `app+0x1250 && mWantRegister`: `app->mRegSource = "title"`, `ShowRegisterDialog` (0x413cb0),
    `mWantRegister = false`. (Trial only: the link is hidden when registered.)
13. `app->mTimedOut (+0x534) = mTrialExpired`.

#### Draw(Graphics* g) - 0x433900, slot 16

1. `Widget::Draw` (empty); `DrawImage(IMG[0] backdrop_title, 0, 0)`; `DrawImage(IMG[3] logo_title, 35,
   mLogoY)`.
2. If `GetBoolean("ShowTrialStatus", true)` (false in retail): registered: `DrawImage(IMG[6], mWidth -
   w(IMG[6]) - 15, h(IMG[3]) + mLogoY - 80)`; else `DrawImage(IMG[5], mWidth - w(IMG[5]), h(IMG[3]) +
   mLogoY - 60)`.
3. If `!mTrialExpired`: `DrawLoadingBar(g)` (slot 48).
4. Text selection into a 256-byte buffer, `g->SetFont(FONT[0])`, text `GT_TITLE_WELCOME`; `textY =
   mBuildUnlocked ? mLoadingBarY - 32 : mRegisterLink->mY - 16`. Registered: `g->SetFont(FONT[2])`, text
   `GT_TITLE_THANKS_FOR_REGISTERING` only if `!mBuildUnlocked` (so the port keeps `GT_TITLE_WELCOME`), `textY
   = mLoadingBarY - 16` (314 in retail). Unregistered: a 25 s cycle (`mUpdateCnt % 2500`) of trial
   messages (UNREGISTERED VERSION, plays/minutes left, "Register now ..." lines) - not ported.
5. If `mTimesPlayed == 0 && mLoadProgress < 1.0`: text = `GT_TITLE_UNPACKING_SOUNDS` (first run;
   the original decodes the sounds to `cached_*.wav` meanwhile).
6. If `app->GetDialog(4) == NULL`: `g->SetFont(mTextFont)` (overrides the font set above),
   `g->SetColor(255,255,255)`; if `!mTrialExpired`: `w = g->GetFont()->StringWidth(text)`;
   `g->DrawString(text, 331 - w/2, mBarDY + textY)`. Then `s = `GT_TITLE_VERSION` + app->mProductVersion`
   ("1.03" from the exe's version resource), `g->SetFont(mVersionFont)`, `g->DrawString(s, mWidth -
   StringWidth(s) - 35, mLogoY + 260)` (still white).

#### DrawLoadingBar(Graphics* g) - 0x433570, slot 48

`y = mLoadingBarY + mBarDY`. `DrawImage(IMG[1], 137, y)`. If `mLoadProgress >= 1.0`: `DrawImage(IMG[4],
137, y)`. Else if `mLoadProgress > 0`: `w = 25 - _ftol(mLoadProgress * -348.0)` (= 25 + trunc(348 p));
`DrawImage(IMG[4], 137, y, Rect(0, 0, w, h(IMG[4])))`; then a 16-pixel soft edge: for `i = 0..15`:
colorize on, colour `(255,255,255, _ftol(cos(i * 0.0981746875) * 255.0))` (0.0981746875 ~ pi/32: 255 ->
25), `DrawImage(IMG[4], 137 + w + i, y, Rect(w + i, 0, 1, h))`, colorize off.

#### ShowTrialExpiredDialog() - 0x4336f0, slot 47 (trial only, not ported)

`DoDialog(4, modal, "PLEASE REGISTER!", the trial-status text + mProdName + " Deluxe has expired!..."
, "", YES_NO)`; buttons relabelled `GT_OPTIONS_REGISTER` / "Quit"; `mContentInsets = Insets(36,32,36,32)`;
`Resize(164, 190, 360, GetPreferredHeight(360))`. App: 2004 -> ShowRegisterDialog, 3004 -> Shutdown.

#### Port notes (TitleScreen)

- Port only the registered paths (as Big Money did): no stamp, no register link (create it hidden or drop
  it), no nag/trial texts, no expiry dialog.
- The loading bar polls `GetLoadingThreadProgress()` every 4 updates and needs `mLoaded`; a port that
  loads synchronously must still feed a progress value for the bar to fill (Big Money's port solved the
  same).
- HyperlinkWidget (framework) draws the label centred at `(mHeight + ascent)/2 - 1` in mColor, or
  mOverColor (white) under the mouse; with mUnderlineSize 0 no underline. The fonts are ShadowFonts, so
  the play link has a red shadow and the welcome/version texts black shadows.

---

### UserManager (no RTTI; linked inside TitleScreen.cpp's range, 0x434800-0x435e20)

A game class without virtuals (so no RTTI name; the name is invented), sizeof **0x2c** (`operator
new(0x2c)` in `BookWormApp::ReadFromRegistry` 0x41a540), kept in `app->mUserManager` (+0x1178). It lists
the user profiles stored in the registry and remembers the current one. Probably its own source file
linked between TitleScreen.cpp and WaitDialog.cpp. One inline getter of it was emitted in
BWGameSelector.cpp (0x402090). The app group (`app.h`) declares the same class with the same names
(`UserManager`, `User`); the dialogs group calls it the profile manager.

| off | type | name | meaning |
|---|---|---|---|
| +0x00 | BookWormApp* | mApp | ctor arg (mRegKey at app+0x40 builds the key paths) |
| +0x04 | std::list<User*> | mUsers | head +0x08, size +0x0c (BWGameSelector tests `+0x0c == 0` = "no users") |
| +0x10 | std::string | mCurrentUser | current user's key name (buffer +0x14, size +0x24, capacity +0x28) |

`User` (0x38 bytes, `new` in LoadUser / app ButtonDepress 2022) is laid out as
`std::pair<const std::string, std::string>`: +0x00 `mName` (registry subkey = name as typed), +0x1c
`mAlias` (display alias; its size is +0x30). `~pair` destroys both.

Registry layout (all under **HKEY_LOCAL_MACHINE**, `SOFTWARE\<mRegKey>\Users`, mRegKey =
"PopCap\BookWorm"):
- value `User` (REG_SZ, always written as a 256-byte buffer) = current user's name;
- one subkey per user `Users\<name>` with values `Alias` (REG_SZ, 256-byte buffer), `ShowHelp`,
  `ShowHints`, `ShowDefs`, `TimesPlayed` (REG_DWORD). Other modules add more values under the same key
  through WriteUserInt / ReadUserInt (e.g. HelpWidget writes ShowHelp).
Port: the emulated registry (`RegistryGetSubKeys`, `RegistryRead/Write*` with value paths
`Users\<name>\<value>`, `RegistryEraseKey`); keep case-insensitive name matching.

Functions:
- **0x435b50 `UserManager(BookWormApp* app)`**: empty list, `mCurrentUser = ""`; open `Users` (KEY_READ); if
  it opens, `RegQueryInfoKey` for the subkey count, then `RegEnumKeyEx(i)` for i = 0.. (name buffer 200)
  until the count or the first failure; each name -> `LoadUser(name)` pushed to the back of mUsers
  (registry enumeration order). Close. `mCurrentUser = GetLastUser()`.
- **0x434880 `~UserManager`**: for every node: `~pair` + delete the User, unlink, delete node; free
  mCurrentUser; free the list head.
- **0x434800 `User* FindUser(const string& name)`**: first node whose `mName` equals `name` with
  `_stricmp`; NULL if none.
- **0x434960 `User::User(const string& name)`**: `mName = mAlias = name`.
- **0x435470 `User* LoadUser(const string& keyName)`**: open `Users\<keyName>` (KEY_READ); if it fails
  return NULL (the ctor would then push a NULL; never happens for enumerated keys). Read `Alias`
  (up to 256 bytes); `new User(keyName)`; `mAlias = the Alias value, or "" when absent` (the empty
  string overrides the ctor's copy). Close.
- **0x4349e0 `bool ReadUserInt(const string& name, const string& valueName, int* out)`**: user must exist
  (FindUser); open `Users\<User.mName>` read; RegQueryValueEx 4 bytes; on success `*out = value` (if
  out). Returns success.
- **0x434d10 `bool WriteUserInt(const string& name, const string& valueName, int value)`**: as above,
  KEY_SET_VALUE, REG_DWORD.
- **0x434b70 `bool WriteUserString(const string& name, const string& valueName, const string& value)`**:
  copies the value into a 256-byte stack buffer (no length check) and writes REG_SZ with size 256.
- **0x434e90 `void SaveUser(User* u)`**: open `Users\<u->mName>` (KEY_SET_VALUE), creating it with
  RegCreateKeyEx if needed (return if that fails); WriteUserString "Alias" = u->mAlias; WriteUserInt
  "ShowHelp" = app+0x12e7, "ShowHints" = app+0x12e8, "ShowDefs" = app+0x12e9, "TimesPlayed" =
  app+0x12e0 (the app's current-user settings); close.
- **0x4352e0 `void DeleteUser(const string& name)`**: open `Users`; for each user matching (stricmp):
  `RegDeleteKey(Users, name)`; on success `~pair` the record and unlink the node (neither the User
  nor the node is freed: leak). Close.
- **0x4356d0 `bool SetCurrentUser(const string& name)`**: if a user matches: write `Users\User` = name;
  on success `mCurrentUser = name`; return success. False if no match or the key cannot be opened.
- **0x435880 `bool SetCurrentAlias(const string& alias)`**: if mCurrentUser is not empty and found:
  `u->mAlias = alias`, `SaveUser(u)`, true.
- **0x435910 `string GetCurrentAlias()`**: mCurrentUser; replaced by the user's alias when the user is
  found and the alias is not empty.
- **0x4359b0 `string GetLastUser()`**: read `Users\User`; if that gives "" (missing), the first user in
  mUsers for which SetCurrentUser succeeds.
- **0x402090 `string GetCurrentUserName()`** (inline, in BWGameSelector.cpp): copy of mCurrentUser.

---

### BWGameSelector (BWGameSelector.cpp 0x401000-0x402df0)

`class BWGameSelector : public Widget, public ButtonListener`, sizeof **0xb0** (`operator new(0xb0)` in
ShowGameSelector 0x410f00), vtables 0x4f93e4 (50 slots) and 0x4f93c8 (+0x58). The main menu: backdrop,
big Lex (BigLex widget), four image buttons, a speech bubble greeting the current user with a "not you"
link, and a scroll-like open/close transition.

#### Layout

| off | type | name | meaning | ctor |
|---|---|---|---|---|
| +0x58 | vfptr | | ButtonListener | |
| +0x5c | BookWormApp* | mApp | | arg |
| +0x60 | BigLex* | mBigLex | `new BigLex(app)` (0x70), Resize(70, 231, 133, 123) | new |
| +0x64 | ButtonWidget* | mClassicButton | id 1 | NULL (AddedToManager) |
| +0x68 | ButtonWidget* | mActionButton | id 2 | NULL |
| +0x6c | ButtonWidget* | mHallButton | id 3 | NULL |
| +0x70 | ButtonWidget* | mQuitButton | id 4 | NULL |
| +0x74 | HyperlinkWidget* | mNotYouLink | id 0 | NULL |
| +0x78 | int | mBubbleX | 14 | 14 |
| +0x7c | int | mBubbleY | 114 | 114 |
| +0x80 | int | mGameOption | copied from/to `app+0x12dc` (default 1); meaning unknown | app+0x12dc |
| +0x84 | int | mGameMode | 0 Classic, 1 Action; copied from/to `app+0x1254` | app+0x1254 |
| +0x88 | int | mBackdropY | backdrop y | 0 |
| +0x8c | int | (unused) | | - |
| +0x90 | double (or 2 ints) | (unused) | zeroed, never read | 0 |
| +0x98 | bool | mOpening | open transition running | false |
| +0x99 | bool | mClosing | close transition running | false |
| +0x9c | DDImage* | mSnapshot | screen snapshot for the transitions | NULL |
| +0xa0 | int | mBandHeight | height the middle band is drawn at, 0..362 | (set by transitions) |
| +0xa8 | int | mStripY | y offset of the whole snapshot | (set by transitions) |

Ctor also sets `mHasAlpha (+0x22) = true`.

#### BWGameSelector::BWGameSelector(BookWormApp* app) - 0x401600

Widget(), vtables, `mApp = app`, mHasAlpha, `mBigLex = new BigLex(app)` (0x402df0) +
`mBigLex->Resize(70, 231, 133, 123)`, `mBubbleX = 14`, `mBubbleY = 114`, buttons/link NULL, `mGameMode =
app+0x1254` (inline setter 0x401340), `mGameOption = app+0x12dc` (inline setter 0x401350), `+0x90 = 0`,
`mBackdropY = 0`, `mOpening = mClosing = false`, `mSnapshot = NULL`.

#### ~BWGameSelector - 0x401000 (0x401720 scalar deleting, slot 0)

Deletes mBigLex, the four buttons, mNotYouLink (NULLed), mSnapshot (NULLed); `~Widget`.

#### AddedToManager(WidgetManager* m) - 0x402430, slot 1

`Widget::AddedToManager`; `app->mWidgetManager->AddWidget(mBigLex)` if set. Then for each button, if not
yet created: `new ButtonWidget(id, this+0x58)` (0xe0), `Resize(Rect(x, y, w(img), h(img)))` (slot 12),
`mDoFinger = true`, `mButtonImage = img`, `mOverImage = over`, `mOverAlphaSpeed (+0xd0) = 0.08` (fade-out
speed; fade-in speed stays 0 = instant), `mHasAlpha = true`; then `AddWidget` (always):

| field | id | image / over | rect |
|---|---|---|---|
| mClassicButton | 1 | IMG[30] gameselect_classic / IMG[31] | (360, 50, 249, 77) |
| mActionButton | 2 | IMG[28] gameselect_action / IMG[29] | (340, 145, 249, 77) |
| mHallButton | 3 | IMG[32] gameselect_hall / IMG[33] | (376, 240, 199, 99) |
| mQuitButton | 4 | IMG[34] gameselect_quit / IMG[35] | (352, 350, 199, 99) |

No labels or fonts (image-only). Then mNotYouLink, if not created: `new HyperlinkWidget(0, this+0x58)`,
label `GT_SELECTOR_NOT_YOU`, `mDoFinger = true`, `mUnderlineSize = 1`,
`SetVisible(false)`, `SetFont(FONT[16] MyriadWeb10)`, `mColor = (32,255,32)`, `mOverColor = (216,216,0)`;
`tw = FONT[16]->StringWidth(label)`, `fh = FONT[16]->GetHeight()`; `Resize((w(IMG[36]) - tw)/2 + mBubbleX,
_ftol((double)(h(IMG[36]) + mBubbleY) - fh * 3.5), tw, fh)` (bubble 261x143: x = (261 - tw)/2 + 14, y =
trunc(257 - 3.5 fh)). AddWidget(mNotYouLink).

#### RemovedFromManager - 0x401370, slot 2 / SetDisabled(bool) - 0x4013f0, slot 10

RemovedFromManager: Widget base; remove mBigLex (through app->mWidgetManager), the four buttons and the
link. SetDisabled(d): Widget base, then `SetDisabled(d)` on the four buttons and the link.

#### ButtonListener

- **0x401450 `ButtonPress(int id, int cc)` (+0x58 slot 2)**: `app->PlaySample(0)`.
- **0x4014c0 `ButtonDepress(int id)` (+0x58 slot 4)**:
  - 0 (not-you link): `ShowSelectUserDialog` (app 0x410520: `new SelectUserDialog` 0x130, colour 0
    white, `Resize(0, 17, 400, prefH(400))`, AddDialog(19)).
  - 1 Classic: `mGameMode = 0`; `SelectGameMode(0)` (app 0x4153c0).
  - 2 Action: `mGameMode = 1`; `SelectGameMode(1)`.
  - 3 Hall of Fame: `ShowHallOfFame` (app 0x411020: RemoveFameWidget, `new FameWidget` 0x2a0 at app+0x1170,
    Resize(0,0,w,h), AddWidget).
  - 4 Quit: `ShowQuitDialog` (app 0x415660).

#### StartGame() - 0x401460, slot 47

`if (mGameMode == -1) mGameMode = 0; if (mGameOption == -1) mGameOption = 1;` (dead: the app fields start at 0
and 1); `app+0x1254 = mGameMode`; `app+0x12dc = mGameOption`; `BookWormApp::StartGameForUser` (0x413c80);
`SetDisabled(true)` (slot 10). Called by SelectGameMode (no saved game) and by the app for dialog 5
(2005 Continue / 3005 New Game).

#### StartOpenTransition() - 0x4010c0, slot 48 (coming back from a game)

If `!mOpening`: delete mSnapshot; `mSnapshot = new DDImage(app->mDDInterface)` (0xc4),
`Create(mWidth, mHeight)` (slot 21), `SetImageMode(false, false)` (slot 23); `mNotYouLink->SetVisible
(false)`; `Graphics(mSnapshot)`; `mWidgetManager->DrawWidgetsTo(g)` (every visible widget, i.e. this
screen drawn normally with buttons and Lex; the board was already removed); then hide the four buttons;
`mHasTransparencies = mHasAlpha = true`; `mBandHeight = 0`; `mStripY = -100`; `mOpening = true`.

#### StartCloseTransition() - 0x4011f0, slot 49 (starting a game)

If `!mClosing`: `mBigLex->SetVisible(false)` (if any); new snapshot as above but hiding mNotYouLink
before the draw (buttons are in the snapshot), then hide the four buttons; `mHasTransparencies =
mHasAlpha = true`; `mMouseVisible (+0x1a) = false`; `mBandHeight = 362`; `mStripY = 0`; `mClosing = true`;
`BringToFront(this)`.

#### Update() - 0x402190, slot 17

1. `Widget::Update()`.
2. If `mUpdateCnt == 100 && !app+0x12e5`: `app+0x12e5 = true`; `PlaySample(46)` (lex_welcome);
   `mBigLex->FUN_00402e40(0, 0)` (BigLex starts talking; BigLex group). Once per run.
3. Closing: `BringToFront(this)`; `mStripY -= 2`; if `mBandHeight > 0`: `mBandHeight -= 8`; if
   `mBandHeight < 1`: `mBandHeight = 0`, `RemoveGameSelector` (app 0x410360: RemoveWidget + SafeDelete,
   app+0x1168 = NULL). MarkDirty; return. (46 updates: 362 - 8*45 = 2 -> -6.)
4. Opening: `mStripY += 2`; if it became `> 0`: `mStripY = 0`, `mOpening = false`, `SetVisible(true)` on
   mNotYouLink and the four buttons, slot 3 (empty), `RemoveBoard` (app 0x410180: RemoveWidget +
   SafeDelete app+0x1160). Then `mBandHeight = min(mBandHeight + 8, 362)`. MarkDirty; return. (Ends on
   the 51st update; the band is full after 46.)
5. Normal: if the user manager has no users (`+0x0c == 0`) or `GetCurrentUserName() == ""`: if no dialog 22
   exists: `ShowNewUserDialog(`GT_SELECTOR_ENTER_NAME`)`
   (app 0x4108c0, dialog 22); if dialog 22 now exists and `app->mUserName` (+0x41c) contains only
   letters and spaces (`find_first_not_of(" AaBb...Zz") == npos`): `dialog22->mEditWidget(+0x128)->
   SetText(app->mUserName)` (EditWidget slot 49).
6. Link visibility: visible iff the user list is not empty (SetVisible + MarkDirty only on a change).

#### Draw(Graphics* g) - 0x402cb0, slot 16

Normal (`!mOpening && !mClosing`): `DrawImage(IMG[20] backdrop_menu, 0, mBackdropY)`; `Widget::Draw`
(empty); `DrawBubble(g)`.

Transition: `SetFastStretch(true)`; three stretched copies of mSnapshot (dest <- src):
`Rect(0, mStripY, mWidth, 58) <- Rect(0, 0, mWidth, 58)`;
`Rect(0, mStripY + 58, mWidth, mBandHeight) <- Rect(0, 58, mWidth, 362)`;
`Rect(0, mBandHeight + 58 + mStripY, mWidth, 57) <- Rect(0, 420, mWidth, 57)`; `SetFastStretch(false)`.
The middle 362-pixel band is squashed (rolled up / unrolled like a scroll); what is not covered keeps the
previous screen contents (the screen is not cleared) or shows the board below.

#### DrawBubble(Graphics* g) - 0x402a50

Only when not transitioning. `name = userManager->GetCurrentUserName()` (the key name, not the alias). If
`name` is not empty: `DrawImage(IMG[36] gameselect_bubble, mBubbleX, mBubbleY)` (14, 114). Greeting:
`app+0x12e0 == 0` (the user's TimesPlayed) ? (`GT_SELECTOR_WELCOME_NEW`, FONT[1] Verdana12Bold) :
(`GT_SELECTOR_WELCOME_BACK`, FONT[2] Verdana14Bold); `x = (261 - StringWidth)/2 + mBubbleX`; `y = mBubbleY -
_ftol(font->GetHeight() * -2.25)` (= 114 + trunc(2.25 fh)); white (255,255,255,255); DrawString.
Second line `name + "!"` in FONT[2]: `x = (261 - StringWidth)/2 + 14`, `y = y1 + FONT[2]->GetHeight()`,
colorize on (no effect on text), white, DrawString, colorize off.

---

### HelpWidget (HelpWidget.cpp 0x4297e0-0x42a820)

`class HelpWidget : public Widget, public ButtonListener, public CheckboxListener`, sizeof **0xa0**
(`operator new(0xa0)` in ShowHelp 0x4101c0), vtables 0x4fcd24 (47), 0x4fcd04 (+0x58), 0x4fccfc (+0x5c).
Full-screen help: the backdrop (with four pictures of the board) and four captions, five topic buttons
on the left, the topic text in a box, a `GT_HELP_DONT_SHOW` checkbox and a continue
button.

#### Layout

| off | type | name | meaning | ctor |
|---|---|---|---|---|
| +0x58 | vfptr | | ButtonListener (0x4fcd04) | |
| +0x5c | vfptr | | CheckboxListener (0x4fccfc) | |
| +0x60 | BookWormApp* | mApp | | arg |
| +0x64 | bool | mRefreshTopics | set by a topic click; Update swaps the topic button images | false |
| +0x68 | int | (unused) | | 0 |
| +0x6c | int | (unused) | | 0 |
| +0x70 | bool | mTopicChosen | a topic was clicked (stops the attention cycling) | false |
| +0x74 | int | mCurTopic | button id 28..32 of the shown topic | -1 |
| +0x78 | Checkbox* | mShowHelpCheckbox | id 27 | NULL |
| +0x7c | ButtonWidget* | mContinueButton | id 5 | NULL |
| +0x80 | DDImage* | mCheckedImage | right half of IMG[146] | NULL |
| +0x84 | DDImage* | mUncheckedImage | left half of IMG[146] | NULL |
| +0x88 | DDImage* | mFlippedArrow | IMG[100] copy flipped vertically, palletized; **never drawn** | NULL |
| +0x8c | ButtonWidget*[5] | mTopicButtons | ids 28..32 | NULL |

Topic names `gHelpTopicNames` (0x52f128): `GT_HELP_BUTTON_BONUS_WORDS`, `GT_HELP_BUTTON_REWARD_TILES`, `GT_HELP_BUTTON_SCORING`, `GT_HELP_BUTTON_ACTION_MODE`,
`GT_HELP_BUTTON_HALL_OF_FAME`. Topic texts `gHelpTopicText[5]` (0x52f1d0, global std::strings, stride 0x1c, built by
the static initializer 0x4f7710 from 0x4fd2f0, 0x4fd1d8, 0x4fd118, 0x4fd028, 0x4fcf68; 284, 272, 184, 237
and 190 chars, one paragraph each, no double spaces, no newlines, no colour codes).
Colours (int[][3]): `gHelpTopicColors` 0x52f13c = {0,0,0}, {0,128,0} (label, label under the mouse);
`gHelpTopicAttentionColors` 0x52f154 = {126,105,78}, {0,128,0}.

#### HelpWidget(BookWormApp* app) - 0x429cd0

Widget(), the two listener vfptrs, `mApp = app`, all fields as in the table (`mCurTopic = -1`).

#### ~HelpWidget - 0x4297e0 (0x429d60 scalar deleting, slot 0)

`mTopicChosen = false`; delete and NULL the checkbox, continue button, the three images and the five
topic buttons; `~Widget`.

#### AddedToManager(WidgetManager* m) - 0x429d90, slot 1

1. `Widget::AddedToManager`; if a board exists: `Board::FUN_00405be0(board, true)` (pause, Board group).
2. `iw = w(IMG[146])`, `ih = h(IMG[146])`: `mCheckedImage = app->CopyImage(IMG[146], Rect(iw/2, 0, iw/2,
   ih))`; `mUncheckedImage = CopyImage(IMG[146], Rect(0, 0, iw/2, ih))`; `mFlippedArrow =
   CopyImage(IMG[100])`, `app->FlipImage(it)` (vertical), `Palletize()` (slot 25). (Each only if NULL.)
3. Checkbox (if NULL and both halves exist): `new Checkbox(mUncheckedImage, mCheckedImage, 27,
   this+0x5c)` (0x8c), `Resize(0, 0, 12, 12)`, `mChecked (+0x60) = !app->mShowHelp (+0x12e7)`,
   `mHasAlpha = true`.
4. Continue button (if NULL): `new ButtonWidget(5, this+0x58)`, `Resize(0, 0, 197, 19)`, `mButtonImage =
   IMG[39]`, `SetFont(FONT[1])`, `mDoFinger = true` (no label).
5. AddWidget(checkbox), AddWidget(continue).
6. Topic buttons i = 0..4 (if NULL): `new ButtonWidget(28 + i, this+0x58)`; label `gHelpTopicNames[i]`;
   `SetFont(FONT[17] MyriadCondensedWeb11)`; `Resize(74, 18*i + 284, 81, 18)` (size of IMG[44]);
   `mDoFinger = mHasAlpha = true`; `mButtonImage = IMG[44]`, or IMG[45] (hilite, 111 wide but clipped to
   the 81-wide button) when `id == mCurTopic`; `SetColors(gHelpTopicColors, 2)` (slot 6). AddWidget.

#### RemovedFromManager - 0x4298d0, slot 2 / OrderInManagerChanged - 0x429940, slot 3

Removed: if a board exists `FUN_00405be0(board, false)`; `mTopicChosen = false`; Widget base; remove
checkbox, continue, topic buttons. OrderInManagerChanged: `PutInfront(child, this)` for the checkbox,
continue and the five topic buttons.

#### Resize(int x, int y, int w, int h) - 0x429990, slot 13

`Widget::Resize`; `mShowHelpCheckbox->Move(mX + 215, mY + 395)`; `mContinueButton->Move((mWidth -
btn.mWidth)/2 + mX, mHeight - btn.mHeight - 24 + mY)` (221, 437); topic button i:
`Resize(mX + 74, btn.mHeight * i + 284 + mY, btn.mWidth, btn.mHeight)`.

#### Update() - 0x429a50, slot 17

`Widget::Update()`. Then:
- If `mRefreshTopics && mTopicChosen`: `mRefreshTopics = false`; each topic button's `mButtonImage =
  (mId == mCurTopic) ? IMG[45] : IMG[44]`; MarkDirty; return.
- Else if `mTopicChosen`: return.
- Else (no topic chosen yet): every 32 updates: `hot = ((mUpdateCnt >> 6) % 5) + 28`; for every topic
  button with `mId != hot`: `SetColors(gHelpTopicColors, 2)`, and note if it `mIsOver`; the button with
  `mId == hot` gets `SetColors(gHelpTopicAttentionColors, 2)` unless some other button is under the
  mouse. Each topic's label turns brown in turn for 64 updates. (After a click, the colours stay as they
  were at that moment.)

#### ButtonListener / CheckboxListener

- **ButtonPress(int id)** (+0x58 slot 3) is the shared function 0x4220c0 in FameWidget.cpp:
  `app->PlaySample(0)`.
- **0x429c70 `ButtonDepress(int id)` (+0x58 slot 4)**: if `id == mContinueButton->mId` (5):
  `SetVisible(false)`, `SetDisabled(true)`, `HideHelp` (app 0x4102d0: RemoveWidget + SafeDelete, board
  shown/unpaused). If `28 <= id <= 32`: `mCurTopic = id`, `mTopicChosen = true`, `mRefreshTopics = true`.
- **0x42a740 `CheckboxChecked(int id, bool checked)` (+0x5c slot 0)**: `PlaySample(0)`; if the checkbox
  exists and `id == 27`: `app->mShowHelp = !checked`; if the current user name is not empty:
  `userManager->WriteUserInt(name, "ShowHelp", app->mShowHelp)`.

#### Draw(Graphics* g) - 0x42a420, slot 16

1. `DrawImage(IMG[22] backdrop_help, 0, 0)`; `DrawTopicText(g)`; `Widget::Draw` (empty).
2. Colour black, `SetFont(FONT[17])`; four captions with `WriteWordWrapped(g, rect, text, -1, 0)` (slot
   37, centred, font line spacing):
   `GT_HELP_CAPTION_LINK` in `Rect(mX+72, mY+218, 116, 48)`;
   `GT_HELP_CAPTION_SUBMIT` in `Rect(mX+201, mY+218, 114, 48)`;
   `GT_HELP_CAPTION_BURNING` in `Rect(mX+328, mY+212, 118, 48)`;
   `GT_HELP_CAPTION_SCRAMBLE` in `Rect(mX+456, mY+212, 116, 48)`.
3. If the checkbox exists: black, `DrawString(`GT_HELP_DONT_SHOW`, cb.mWidth + 8 +
   cb.mX, cb.mHeight - 1 + cb.mY)` (235, 406) in FONT[17].

#### DrawTopicText(Graphics* g) - 0x42a170

- No topic (`!mTopicChosen` or mCurTopic outside 28..32): colour (126,105,78), `SetFont(FONT[0])`,
  `WriteWordWrapped(g, Rect(mX + 192, mY - _ftol(h(IMG[23]) * -0.4) + 282, w(IMG[23]) - 12, h(IMG[23]) -
  12), `GT_HELP_CHOOSE_TOPIC`, -1, 0)` = Rect(192, 319, 369, 82), centred, no box.
- Topic t = mCurTopic - 28: `DrawImage(IMG[23], mX + 186, mY + 282)`; black; `SetFont(FONT[17])`; text =
  `gHelpTopicText[t]`, and for t = 4 (Hall of Fame) a sentence pointing to the internet high scores through the Hall of Fame
  link is appended when internet scores are on
  (`BookWormApp::FUN_004152a0`: property NoInternetScores false; true in retail, so not appended);
  `WriteWordWrapped(g, Rect(mX + 192, mY + 288, 369, 82), text, -1, -1)` (left); then
  `DrawImage(IMG[45], btn.mX, btn.mY)` for the chosen topic button (the full 111-pixel hilite; the
  button draws its clipped copy and label on top).

Port: `WriteWordWrapped` is the Widget slot 37 version (`graphics.md` section 3); these texts have no
double spaces or trailing newlines, so the port's Graphics::WriteWordWrapped gives the same lines; pass
the justification explicitly.

---

### CreditsDialog (CreditsDialog.cpp 0x41bfc0-0x41d8d0)

`class CreditsDialog : public Dialog`, sizeof **0x138** (`operator new(0x138)` in ShowCredits
0x4106c0), vtables 0x4fc07c (52) and 0x4fc060 (+0x58, Dialog's ButtonPress/Depress/DownTick). A copy of
Big Money's credits dialog with BookWorm's names.

| off | type | name | meaning | ctor |
|---|---|---|---|---|
| +0x110 | BookWormApp* | mApp | | arg |
| +0x118 | double | mCloudRot | rotation of the top cloud layer, += 0.004 | 0 |
| +0x120 | double | mCloudX | horizontal scroll, += 0.2, wraps by -512 above 0 | 0 |
| +0x128 | double | mCloudY | vertical scroll, += 0.3, wraps by -512 above 0 | 0 |
| +0x130 | ImageFont* | mFont | FONT[15] LegacySerifMdSCITCTT12 | app+0x11ec |
| +0x134 | | (pad) | | |

- **0x41c0b0 `CreditsDialog(BookWormApp* app)`**: `Dialog(IMG[7] dialog, IMG[8] dialogbutton, 16, modal
  true, header "", lines "", footer "DONE", mode 3 FOOTER)` -> one button id 2016 labelled "DONE";
  `mContentInsets = Insets(36,36,36,36)`; `SetButtonFont(FONT[0])` (no header/lines fonts: port must set
  them, as Big Money's port did, because the port Dialog draws with them); fields as above.
- **0x41bfc0 `~CreditsDialog`** = `~Dialog`; 0x41c080 scalar deleting (slot 0).
- **0x41bfe0 `GetPreferredHeight(int)` (slot 50)**: returns 396 (constant).
- **0x41bff0 `Update()` (slot 17)**: `Dialog::Update`; `mCloudRot += 0.004`; `mCloudX += 0.2`, if `> 0`
  `-= 512.0`; `mCloudY += 0.3`, if `> 0` `-= 512.0`; MarkDirty. (The wrap subtracts from the unrounded
  x87 sum; at 53-bit precision identical to double arithmetic.)
- **0x41c2a0 `Draw(Graphics* g)` (slot 16)**:
  1. `Dialog::Draw(g)` (frame + DONE button area).
  2. `area = Rect(39, 34, mWidth - 78, mHeight - 110)` (388x396: 310x286); `Graphics g2(*g)`,
     `g2.ClipRect(area)`; colour (210,170,15), `g2.FillRect(0, 0, mWidth, mHeight)` (gold, clipped).
  3. Additive: `DrawImage(IMG[148], _ftol(mCloudX), 0)`, `(_ftol(mCloudX) + 512, 0)`, `(0,
     _ftol(mCloudY))`, `(0, _ftol(mCloudY) + 512)`; normal: `DrawImageRotated(IMG[148], -64, -64,
     mCloudRot, NULL)` (int overload: integer centre 256,256).
  4. Inner shadow: for `a = 1530; a > 0; a -= 255` (6 rings, i = 0..5): colour `(48,48,48, a/7)` (218,
     182, 145, 109, 72, 36), `g2.DrawRect(39 + i, 34 + i, (mWidth-78) - 2i, (mHeight-110) - 2i)` (the
     right/bottom edges of the outer rings fall outside the clip).
  5. On `g` (unclipped): `SetFont(mFont)`, black; `H = mFont->GetHeight()`, `gap = _ftol(H * 1.5)`;
     `y = 60`; six sections, each `WriteCenteredLine(g, y, heading)`, `WriteCenteredLine(g, y + H,
     names)`, `y += H + gap`: `GT_CREDITS_DESIGN` / `GT_CREDITS_DESIGN_NAMES`; `GT_CREDITS_PROGRAMMING` / `GT_CREDITS_PROGRAMMING_NAMES`; `GT_CREDITS_ART` / `GT_CREDITS_ART_NAMES`; `GT_CREDITS_MUSIC` / `GT_CREDITS_MUSIC_NAMES`; `GT_CREDITS_BUSINESS`
     / `GT_CREDITS_BUSINESS_NAMES`; `GT_CREDITS_TESTING` / `GT_CREDITS_TESTING_NAMES`. (WriteCenteredLine:
     x = (mWidth - StringWidth)/2, baseline y, current colour.) The dialog colours set by the app
     (header/lines white, button text black) are not used for these lines.
- DONE (id 2016) goes to BookWormApp::ButtonDepress, whose default case KillDialog(16)s.

Also in this range: **0x41d2b0 `Definitionator::GetDefinition(const string& word)`** (inline of the
Definitionator, which follows in link order): looks `word` up in the `std::map<string,string>` at this+8
and returns the mapped value or "". Caller: Board 0x40f5b0 (definition balloon when `app+0x12e9`
ShowDefs; definitionator at `app+0x1174`). For the Definitionator analysis.

---

### OverlayWidget (OverlayWidget.cpp 0x430970-0x431950)

`class OverlayWidget : public Widget`, sizeof **0x90** (`operator new(0x90)` in Board::AddedToManager
0x403730), vtable 0x500cbc (47). The board's top layer: owns the effect manager (drawn first), the
bonus-word banner and its arrows, the `GT_BONUS_WORD_BALLOON_FORMAT` balloon, Lex's sweat drops, the pause box, the
fire column of a burning tile and the full-screen flash. `mMouseVisible = false` (never takes the mouse).

#### Layout

| off | type | name | meaning | ctor |
|---|---|---|---|---|
| +0x58 | BookWormApp* | mApp | | arg |
| +0x5c | EffectMgr* | mEffectMgr | `new EffectMgr(app)` (0x1c, ctor 0x4201c0) | new |
| +0x60 | WordBalloon* | mBalloon | created in AddedToManager | NULL |
| +0x64 | bool | mShowBalloon | request: open the bonus-word balloon | false |
| +0x65 | bool | mBalloonUp | the balloon is showing the bonus text | false |
| +0x68 | int | mBannerTargetY | banner target y (406 shown, board.mHeight hidden) | 0 |
| +0x6c | int | mBannerY | banner y (0 = never placed) | 0 |
| +0x70 | bool | mBannerActive | banner shown (set by Board, cleared on an overshooting slide down) | false |
| +0x78 | double | mArrowFade | arrow alpha 0..1+; Board sets 10.0, -0.025 per update | 0 |
| +0x80 | Rect | mBannerRect | (10, mBannerY, w(IMG[19]), h(IMG[19])) hover area | 0 |

Ctor also sets `mHasAlpha = true`, `mMouseVisible = false`. Board writes +0x68/+0x6c/+0x70/+0x78 in
`Board::ShowBonusBanner(bool show)` (0x403d80): `if (mBannerY == 0) mBannerY = board.mHeight; mBannerTargetY
= show ? 406 : board.mHeight; if show {board MarkDirty; mBannerActive = true}; mArrowFade = 10.0`.

#### Functions

- **0x430c20 `OverlayWidget(BookWormApp* app)`**: as above.
- **0x430970 `~OverlayWidget`** (0x430ce0 scalar deleting): delete mEffectMgr (NULL); if mBalloon:
  `app->mWidgetManager->RemoveWidget(mBalloon)`, delete, NULL; `~Widget`.
- **0x430d10 `AddedToManager(m)` (slot 1)**: Widget base; if no balloon: `mBalloon = new WordBalloon(app,
  IMG[27] backdrop_wb_text, IMG[125] wb_text_talk_tail, 159, -2)` (0x120; the last two are
  `_ftol(0.75 * 213.0)` and -2, passed as one 8-byte struct, probably the tail position);
  `mBalloon->Resize(0, 0, 213, mBalloon->slot51(213))` (height for width 213); AddWidget. Then
  `mBannerRect = Rect(10, mBannerY, 204, 33)`.
- **0x430bb0 `RemovedFromManager` (slot 2)**: base, RemoveWidget(mBalloon). **0x430b90
  `OrderInManagerChanged` (slot 3)**: `PutInfront(mBalloon, this)`.
- **0x430a00 `bool IsBannerMoving()`**: `mBannerY != mBannerTargetY`.
- **0x430a20 `AddEffect(Effect* e)`**: if no manager `OutputDebugString("(mEffectMgr == NULL)!")` and
  delete `e` (Effect slot 7 with 1); else `e+4 = mEffectMgr; mEffectMgr->Add(e)` (0x4207f0).
- **0x430a70 `GetEffects(int type)`**: `mEffectMgr ? EffectMgr::FUN_00420830(type) : NULL` (returns a
  list of effects of a type; Effect group). **0x430a90 `bool HasEffects(int type)`**: `mEffectMgr ?
  EffectMgr::FUN_0041fa50(type) : false`.
- **0x430ab0 `SaveState(FILE* f)`**: `fwrite` +0x68 (4 bytes), +0x6c (4), +0x70 (1), +0x78 (8), +0x80
  (16), then `EffectMgr::FUN_0041faa0(f)`. **0x430b20 `LoadState(FILE* f)`**: `fread` the same 33 bytes
  in the same order, `EffectMgr::FUN_0041f930()` (clear), `EffectMgr::FUN_00420b70(f)`. Part of the
  board's saved-game file (Board 0x406c40 / 0x408ab0); little-endian int, int, byte, double, 4 ints.
- **0x430be0 `Rect::Contains(const Point& p)`** (inline helper): `mX <= p.x < mX+mWidth && mY <= p.y <
  mY+mHeight`.

#### Update() - 0x430e20, slot 17

1. `board = app->mBoard`; if NULL return; `lex = board+0x350`; if `board+0x3c8 > 0` return (both returns
   skip `Widget::Update`).
2. `Widget::Update()`.
3. If lex and mBalloon: `mouse = (mWidgetManager->mLastMouseX, mLastMouseY)`.
   - Balloon visible (+0x19): `mBalloon->Move((lex.mWidth - bal.mWidth)/2 + lex.mX, lex.mY -
     bal.mHeight + 30)`; if `mBalloonUp && !mBannerRect.Contains(mouse)`: `mBalloonUp = false`,
     `mBalloon->slot49(0)` (WordBalloon: close).
   - Hidden: if `mBannerActive && !mShowBalloon && !board+0x3c0 && mBannerRect.Contains(mouse)`:
     `mShowBalloon = true`.
4. `mEffectMgr->Update()` (0x41f990).
5. If `mArrowFade > 0`: `mArrowFade = max(mArrowFade - 0.025, 0)`; MarkDirty.
6. Banner slide, every 2nd update while `mBannerY != mBannerTargetY`: moving down (`target > y`): `y += 2`,
   if `y > target` {`mBannerActive = false`; `y = target`}; moving up: `y -= 2`, if `y < target` `y =
   target`. Then `mBannerRect.mX = 10`, `mBannerRect.mY = mBannerY`; MarkDirty. (406 <-> 480 is 37 even
   steps, so the hide never overshoots and mBannerActive stays true at y 480, off screen.)
7. If `mShowBalloon`: `mShowBalloon = false`; `text = StrFormat(`GT_BONUS_WORD_BALLOON_FORMAT` (the word in dark green colour codes),
    board->mBonusWord (+0x28c))` (word in dark green); `lex->Think(text,
   FONT[1], -1)` (0x42d800, Lex's thought image; Lex group); `mBalloon+0xbc (Insets) = Insets(-5, -10, -5, 0)`;
   `mBalloon->SetColors(gBonusBalloonColors 0x52f640, 4)` ([][4]: white, black, black, black);
   `mBalloonUp = true`.

#### Draw(Graphics* g) - 0x431080, slot 16

1. `mEffectMgr->Draw(g)` (0x41fa10) if set. Return if no app or no board.
2. Banner, if `mBannerActive`: `DrawImage(IMG[19] backdrop_bonus, 10, mBannerY)`. If the bonus word
   (board+0x28c, size +0x2a0) is not empty: `s = Upper(word)`; `SetFont(FONT[11]
   CourierFinalDraft20Bold)`; colour (216,194,46); `x = 112 - StringWidth(s)/2`; `DrawString(s, x,
   mBannerY + 25)`. If `board+0x1f4 != 0`: `p = Board::FUN_00406b00(board+0x1d8, s)` (the part of the
   word already matched; Board group); if `p` is not empty: white (255,255,255,255),
   `DrawString(Upper(p), x, mBannerY + 25)` (same left edge, overprints the matched letters in white).
   Arrows, if `board->mState (+0x194) == 0 && mArrowFade > 0 && mBannerY != app->mHeight`: `c = cos((mUpdateCnt % 48)
   * 0.020833333333333332 * -6.28318) * 2.5`; `a = min(255, _ftol(mArrowFade * 255.0))`; colorize, colour
   (255,255,255,a); `DrawImage(IMG[99], _ftol(4.0 + c), mBannerY)`, `DrawImage(IMG[100], _ftol(200.0 -
   c), mBannerY)`, `DrawImage(IMG[101], 100, _ftol((mBannerY - 38) + c))`; colorize off. (The three sums
   are rounded to double before `_ftol`; 6.28318 is not 2 pi, so at phase 12 the right arrow is at 199
   and at phase 36 the left one at 3.)
3. Lex sweat: if lex and `lex->mSweating` (+0xc5) and `lex->mAnim == 19` (+0xb4) and `!board->mScorched` (+0x1b1): `fh = h(IMG[73]) / 8` (48);
   `DrawImage(IMG[73], lex.mX, lex.mY, Rect(0, lex+0xc8 * fh, w(IMG[73]), fh))` (frame `lex->mSweatFrame`, +0xc8, 0..7).
4. Pause box, if `board+0x3d0 > 0` (fade, double) and `board+0x3dc` (paused): `DrawImageBox(Rect(233,
   150, mWidth - 233, 150), IMG[7])`; colorize on, colour (255,255,255, `_ftol(fade * 255)`);
   `SetFont(FONT[14] LockergnomeSmallColor)`, `GT_PAUSED` at `(((mWidth - w) + 233)/2, 210)`;
   `SetFont(FONT[9] Lockergnome18)`, `GT_PAUSED_CONTINUE` at `(((mWidth - w) + 233)/2, 250)`;
   colorize off.
5. Fire column, if `T = board+0x320` (burning tile; +0x30 column, +0x34 row): `W = _ftol(mWidth *
   board+0x328 * 2)` (`fild; fmul double; fadd st,st`); `x0 = Board::ColumnX(T.col) + 28 - W/2`
   (ColumnX 0x403990 = col*57 + 233); `img = IMG[72] fx_fireline` (640x80); `off = board+0x330`; `H =
   mHeight`. For k = 3, 2, 1 with `xl = x0 + 16 - 8(3-k)`, `xr = x0 + W - 24 + 8(3-k)`, for side xs in
   {xl, xr}: `Graphics g2(*g)`, `ClipRect(x0, 0, W, H)`, `ClipRect(xs, 0, 8, H)`; `sh = (h*k)/4` (int,
   toward zero); `yy = 57 - (sh*50)/h + Board::TileY(T.col, T.row)` (TileY 0x4039b0 = row*57 + (col even
   ? -17 : 11)); FastStretch on; `DrawImage(img, Rect(-off, yy, 640, sh), Rect(0,0,640,80))` and
   `DrawImage(img, Rect(640 - off, yy, 640, sh), Rect(0,0,640,80))`; FastStretch off. Then the core:
   `ClipRect(x0 + 24, 0, W - 48, H)`, `yy = 57 - 50 + TileY` (= TileY + 7), `DrawImage(img, -off, yy,
   Rect(0,0,640,80))`, `DrawImage(img, 640 - off, yy, Rect(0,0,640,80))`. (A horizontally scrolling
   flame strip, clipped to a column W wide, with 8-pixel edge strips at 3/4, 2/4, 1/4 height.)
6. Flash, if `board+0x308 > 0`: `v = _ftol(board+0x308 * 255)`; additive mode; colour (v,v,v);
   `FillRect(0, 0, mWidth, mHeight)`; normal mode.

---

### ColorizedButton (ColorizedButton.cpp 0x41b3e0-0x41bfc0)

`class ColorizedButton : public ButtonWidget`, sizeof **0xf0** (`operator new(0xf0)` in
LevelUpWidget::AddedToManager 0x42bb10), vtable 0x4fbf7c (53). An image button whose images are drawn
colorized by a per-state colour list, and that repaints on every hover/press change.

| off | type | name | meaning |
|---|---|---|---|
| +0xe0 | std::vector<Color> | mImageColors | first +0xe4, last +0xe8, end +0xec; index 0 normal, 1 over, 2 down, 3 disabled |

- **0x41bf50 `ColorizedButton(int id, ButtonListener* l)`**: `ButtonWidget(id, l)`; empty vector;
  `SetImageColors(gColorizedButtonColors 0x52e8e0, 4)` (4 x (255,255,255,255): no tint by default; no
  caller changes them).
- **0x41b650 `~ColorizedButton`**: free the vector; `~ButtonWidget` (0x41bd60 scalar deleting).
- **0x41bed0 `SetImageColors(int theColors[][4], int n)` (slot 49)** / **0x41be50 `SetImageColors(int
  theColors[][3], int n)` (slot 50)**: clear, push `Color(r,g,b,a)` / `Color(r,g,b)` for each; MarkDirty.
- **0x41b510 `SetImageColor(int i, const Color& c)` (slot 51)**: `mImageColors[i] = c` (no bounds
  check); MarkDirty. **0x41b550 `const Color& GetImageColor(int i)` (slot 52)**.
- **MouseEnter 0x41b3e0 (23), MouseLeave 0x41b400 (24), MouseUp(x,y) 0x41b420 (30), MouseDown(x,y,cc)
  0x41b450 (27)**: the ButtonWidget version, then MarkDirty.
- **0x41b6a0 `Draw(Graphics* g)` (slot 16)**:
  1. Return if mBtnNoDraw. If no mButtonImage and no mDownImage: `ButtonWidget::Draw(g)`; return.
  2. If mFont is NULL and the label is not empty: `mFont = new SysFont(app, "SansSerif", 10, 0, false,
     false, false)` (port: no SysFont; never reached, the game sets a font).
  3. `down = (mIsDown && mIsOver && !mDisabled) != mInverted`. Label position (when mFont): `x = (mWidth -
     StringWidth(mLabel))/2`, `y = (mHeight - ascent/6 - 1 + ascent)/2`. `g->SetFont(mFont)`.
  4. Down: colorize on, colour `GetImageColor(2)`; image = mDownImage at (0,0), else mOverImage at (1,1),
     else mButtonImage at (1,1); colorize off; colour `mColors[1]` (label hilite); `DrawString(mLabel,
     x+1, y+1)`.
  5. Not down: colour `c = GetImageColor(mDisabled ? 3 : mIsOver ? 1 : 0)`. If mDisabled and
     mDisabledImage: draw it colorized with c. Else if mIsOver and mOverImage: mOverImage with c. Else if
     `mOverAlpha > 0` and mOverImage: mOverImage with c whose alpha = `_ftol(mOverAlpha * 255.0)` (the
     normal image is **not** drawn under it). Else if mButtonImage: mButtonImage with c. (All at (0,0),
     colorize on/off around.) Then, if the label is not empty: colour `mColors[0]`, `DrawString(mLabel,
     x, y)`.
Use: LevelUpWidget's continue button (id 6, IMG[53] bar_footer_continue 319x46, FONT[1], mDoFinger).

### ImagelessButton (ImagelessButton.cpp 0x42b4c0-0x42b730)

`class ImagelessButton : public ButtonWidget`, sizeof **0xe8** (`operator new(0xe8)` in the FameWidget
ctor 0x428520), vtable 0x4fd524 (49). A text-only button.

| off | type | name | meaning |
|---|---|---|---|
| +0xe0 | int | mJustify | -1 left, 0 centre (ctor), 1 right |
| +0xe4 | | (pad) | |

- **0x42b500 `ImagelessButton(const string& label, int id, ButtonListener* l, Font* font)`**:
  `ButtonWidget(id, l)`; `mLabel = label`; `mJustify = 0`; `SetColors(gImagelessButtonColors 0x52f2d0,
  4)` ([][4]: (0,0,0,255) normal, (255,255,255,255) over, (255,255,255,255) down, (128,128,128,255)
  disabled); `SetFont(font)`.
- **0x42b4c0 `~ImagelessButton`** = `~ButtonWidget` (0x42b4d0 scalar deleting).
- **0x42b590 `Draw(Graphics* g)` (slot 16)**: if `!mBtnNoDraw` and the label is not empty: SysFont
  "SansSerif" 10 fallback if no font; `x` = 0 / `mWidth - w` / `(mWidth - w)/2` for mJustify -1 / 1 /
  other; `y = (mHeight - ascent/6 - 1 + ascent)/2`; `SetFont(mFont)`; colour `mIsDown ? mColors[2] (and
  x+1, y+1) : mDisabled ? mColors[3] : mIsOver ? mColors[1] : mColors[0]` (note: plain mIsDown, not
  IsButtonDown); `DrawString(mLabel, x, y)`.
Use: only FameWidget's "RESUBMIT" button (internet high scores; colours set there 0 black, 1 red, 3
(100,100,100); Resize(0,0,100,20); hidden). Not needed unless that UI is ported.

### WaitDialog (WaitDialog.cpp 0x435e20-0x436260)

`class WaitDialog : public Dialog`, sizeof **0x120** (`operator new(0x120)` in FameWidget 0x423fd0),
vtables 0x5015cc (52) and 0x5015b0 (+0x58). A modal "please wait" dialog with a scrolling barber-pole bar
and a Cancel footer button. Used only while the Hall of Fame talks to the internet score server (not
ported; never shown in retail because NoInternetScores is true).

| off | type | name | meaning | ctor |
|---|---|---|---|---|
| +0x110 | BookWormApp* | mApp | stored, not read here | arg |
| +0x114 | Image* | mBarImage | IMG[147] waitbar (32x16) | arg |
| +0x118 | bool | mStopped | stops the animation; never set by any code | false |
| +0x11c | int | mBarOffset | 0..31 | 0 |

- **0x436110 `WaitDialog(BookWormApp* app, int id, Image* comp, Image* btnComp, Image* bar, const
  string& header, const string& lines)`**: `Dialog(comp, btnComp, id, modal true, header, lines,
  footer = gSexyAppBase->GetString("DIALOG_BUTTON_CANCEL"), mode 3)`; fields; `mHasAlpha = true`;
  `mYesButton->SetColor(1, white)`. (FameWidget: id 25, header "CONNECTING", fonts header FONT[3], lines
  and button FONT[1], colours 0/1 white, 3 black, width 340 centred, AddDialog(25); 2025 Cancel ->
  `FameWidget+0x74 = 0`, KillDialog(25).)
- **0x435e20 `~WaitDialog`** = `~Dialog` (0x435fb0 scalar deleting). **0x435e40 / 0x435e50
  AddedToManager / RemovedFromManager** = Dialog's.
- **0x435e60 `Resize(x,y,w,h)` (slot 13)**: `Dialog::Resize`; if mYesButton: temporarily
  `mContentInsets = Insets(72, 36, 72, 36)`; `bh = h(mYesButton->mComponentImage)`;
  `mYesButton->Resize(mX + 72, mY + mHeight - 36 - bh, mWidth - 144, bh)`; restore the insets.
- **0x435f30 `GetPreferredHeight(int w)` (slot 50)**: `Dialog::GetPreferredHeight(w) + h(mBarImage) +
  20`.
- **0x435f60 `Update()` (slot 17)**: `Dialog::Update`; if `!mStopped && mUpdateCnt % 3 == 0`:
  `mBarOffset = (mBarOffset + 1) % 32`; MarkDirty.
- **0x435fe0 `Draw(Graphics* g)` (slot 16)**: `Dialog::Draw`; `r = Rect(cL + 24, mHeight - cB - 72,
  mWidth - cR - cL - 48, 16)` (c = mContentInsets); `Graphics g2(*g)`, `ClipRect(r)`; for `i = 0 ..
  r.w/32 + 1`: `DrawImage(mBarImage, 32i - mBarOffset + r.x, r.y)`; then on `g` black `DrawRect(r.x - 1,
  r.y - 1, r.w + 1, r.h + 1)`.

Also in this range (WordBalloon helpers placed before WordBalloon.cpp's first vtable function; for the
WordBalloon analysis): **0x436260 `WordBalloon::Init(BookWormApp* app)`** (called by the WordBalloon ctor
0x4372a0): `+0xd4 = app`, `mHasAlpha = true`, `mDoFinger = true`, +0xdc..+0xec = 0, +0xf0/+0xf4/+0xf8 =
-1, +0x100..+0x10c = 0, `+0x110 (double) = 0.1`, +0x118 = 0, +0x11c (bool) = false,
`SetColors(0x52f8f0, 4)` ([][4]: white, black, black, black), `SetVisible(false)`, `SetDisabled(true)`.
**0x436310 `WordBalloon::FUN_00436310(int a, int b, int c)`**: `+0xf4 = a`, `+0xf0 = b`, `+0xf8 = c`
(callers Lex::Update 0x42dd20 and LevelUpWidget with (0,0,0)).

---

### Cross-module references

#### BookWormApp / SexyApp fields (framework names from Part 1; others mine)

| field | type | meaning (guess) | used here |
|---|---|---|---|
| app+0x08 | string | mProdName "BookWorm" | trial dialog text |
| app+0x40 | string | mRegKey "PopCap\BookWorm" | UserManager key paths |
| app+0x60 / +0x64 | int | mWidth / mHeight (640x480) | screens |
| app+0x2e4 | WidgetManager* | mWidgetManager | |
| app+0x334 | DDInterface* | mDDInterface | BWGameSelector snapshot |
| app+0x37c | string | mRegSource | TitleScreen "title"/"timeout" (trial) |
| app+0x3b8 | string | mProductVersion ("1.03") | TitleScreen version text |
| app+0x41c | string | mUserName | BWGameSelector pre-fills the new-user name |
| app+0x470 / +0x471 | bool | mIsRegistered / mBuildUnlocked (via gSexyAppBase 0x56b8a8 for +0x471) | TitleScreen |
| app+0x526 | bool | mLoaded | TitleScreen |
| app+0x52c | int | mTimesPlayed (app-wide) | TitleScreen |
| app+0x534 | bool | mTimedOut | TitleScreen writes |
| app+0x1160 | Board* | mBoard | Overlay, Help, selector transition |
| app+0x1164 | TitleScreen* | mTitleScreen | |
| app+0x1168 | BWGameSelector* | mGameSelector | |
| app+0x116c | HelpWidget* | mHelpWidget | |
| app+0x1170 | FameWidget* | mFameWidget | |
| app+0x1174 | Definitionator* | mDefinitionator | Definitionator::GetDefinition |
| app+0x1178 | UserManager* | mUserManager | Title, selector, help |
| app+0x11ac | Image** | mImages[149] | |
| app+0x11b0 | ImageFont*[19] | mFonts | |
| app+0x1250 | bool | mInitDone: set at the end of Init (gates the title's register click) | TitleScreen |
| app+0x1254 | int | mGameMode (0 Classic, 1 Action; ctor 0) | BWGameSelector reads/writes |
| app+0x1258 | int | mSelectedGameMode (mode picked in the selector, for the continue dialog; ctor -1) | SelectGameMode |
| app+0x12dc | int | mGameOption (ctor 1; only the selector copies it; no reader found) | BWGameSelector |
| app+0x12e0 | int | mUserTimesPlayed (current user's registry TimesPlayed) | bubble greeting, SaveUser |
| app+0x12e5 | bool | mWelcomePlayed (lex_welcome already played) | BWGameSelector |
| app+0x12e7 / +0x12e8 / +0x12e9 | bool | mShowHelp / mShowHints / mShowDefs | Help checkbox, SaveUser |
| app+0x1340 | int | mMaxTime (property MaxTime; > 0 selects the timed trial) | TitleScreen (trial) |

(Names for the BookWormApp data follow the app group's `app.h`.)

#### Board / Lex / WordBalloon / EffectMgr fields read by OverlayWidget

Board (`app->mBoard`): +0x158 mOverlay (this), +0x194 int (0 = arrows allowed; set 3 when the continue
dialog is up), +0x1b1 bool (suppresses the Lex sweat), +0x1d8 (object of FUN_00406b00: current
selection), +0x1f4 int (non-zero: draw the matched prefix), +0x28c std::string mBonusWord, +0x308 double
flash, +0x320 Piece* burning tile (+0x30 col, +0x34 row), +0x328 double fire width fraction, +0x330 int
fire scroll offset, +0x350 Lex*, +0x3c0 bool (blocks the balloon), +0x3c8 int (> 0: overlay not
updated), +0x3d0 double mPauseFade, +0x3dc bool paused (board_draw.h names: +0x194 mState, +0x1b1
mScorched). Lex (lex.h names): +0xb4 mAnim (19 = sweating anim), +0xc5 mSweating, +0xc8 mSweatFrame;
mX/mY/mWidth/mHeight. The Lex fills this overlay's balloon itself (`Say` 0x42d6c0 / `Think` 0x42d800 use
`board->mOverlay->mBalloon`). WordBalloon: +0x19 mVisible, +0xbc Insets, slots 5 (SetColors), 49
(close, arg 0), 51 (height for a width).

#### Functions of other modules called

App: 0x410180 RemoveBoard, 0x4101c0 ShowHelp, 0x4102d0 HideHelp, 0x410360 RemoveGameSelector, 0x410520
ShowSelectUserDialog, 0x4106c0 ShowCredits, 0x4108c0 ShowNewUserDialog(text), 0x410f00
ShowGameSelector, 0x411020 ShowHallOfFame, 0x413a40 LoadUserSettings, 0x413c80 StartGameForUser, 0x410d10 StartGame (new Board), 0x413cb0
ShowRegisterDialog, 0x4152a0 IsInternetScoresEnabled, 0x4153c0 SelectGameMode(mode), 0x415660 ShowQuitDialog,
0x414ef0 TitleScreenDone (slot 67), ButtonDepress 0x417080 (dialogs 4, 5, 16, 25).
Board: 0x403990 ColumnX(col), 0x4039b0 TileY(col,row), 0x403d80 ShowBonusBanner(bool), 0x405be0
(pause/unpause, called by HelpWidget), 0x405cd0 (before the selector opens), 0x406b00 (matched part of
the bonus word). Lex: 0x42d800 Think(text, font, -1). BigLex: ctor 0x402df0, 0x402e40 (start talking).
WordBalloon: ctor 0x4372a0. EffectMgr: ctor 0x4201c0, 0x41f990 Update, 0x41fa10 Draw, 0x4207f0 Add,
0x420830, 0x41fa50, 0x41faa0 Save, 0x41f930 Clear, 0x420b70 Load. Framework: HyperlinkWidget ctor
0x43f300, ButtonWidget ctor 0x43f080, Checkbox ctor 0x43e890, Dialog ctor 0x43f3b0, DDImage ctor,
SexyApp::CopyImage(img, rect) 0x47d9f0 / CopyImage(img) 0x47e330 / FlipImage 0x47dab0, StrFormat
0x46e5d0, Upper 0x46a2e0, WidgetManager::DrawWidgetsTo / BringToFront / PutInfront.

Functions of this group called from elsewhere: OverlayWidget 0x430a00/0x430a20/0x430a70/0x430a90
(Board), 0x430ab0/0x430b20 (Board save/load); UserManager functions (app, dialogs, FameWidget);
BWGameSelector slots 47/48/49 (app); HelpWidget via ShowHelp/HideHelp; Definitionator::GetDefinition
(Board).

### Files, registry, properties

- Properties read: `LoadingBarY` (int, default 389; partner 330), `MaxPlays` (int, default 10),
  `ShowTrialStatus` (bool, default true; partner False), `NoInternetScores` (via 0x4152a0; partner True).
- Registry (HKLM `SOFTWARE\PopCap\BookWorm\Users`): see UserManager.
- File: OverlayWidget's 33-byte block plus the effect manager's data inside the board's saved game
  (written/read by the Board through 0x430ab0 / 0x430b20).
- Images and fonts listed at the top; credits and help texts are hard-coded in the exe.

### What is not ported

- TitleScreen trial paths (registered build takes the other branch): trial stamp; register link
  (`GT_TITLE_REGISTER_NOW` -> RegisterDialog, which can open
  `http://www.popcap.com/register.php?theGame=...` with mRegSource "title"); nag timer and trial
  messages; trial-expired dialog (Register / Quit) and the `mTimedOut` flag.
- WaitDialog and ImagelessButton: internet high-score UI only (FameWidget "CONNECTING" / "RESUBMIT";
  partner NoInternetScores=True). Port them only if that UI is ported (it is not).
- HelpWidget's appended internet-high-score sentence (only when internet scores are on).
- The "more games" link (partner `OptionsLinkURL0`, a popcap.com URL) lives in OptionsDialog, not in this
  group; nothing here opens a URL in the registered build.
- SysFont fallbacks in ColorizedButton/ImagelessButton (never reached).

### Open questions

1. `app+0x12dc` (selector mGameOption, default 1): copied back and forth by BWGameSelector, no reader found
   in the game code (maybe an unused difficulty/players setting). Check with the app/board analyses.
2. BWGameSelector +0x90 (8 bytes zeroed, never read) and TitleScreen +0x8c, HelpWidget +0x68/+0x6c:
   unused fields, names unknown.
3. HelpWidget's flipped copy of IMG[100] bonusarrow_right (+0x88) is created, palletized and freed but
   never drawn; probably left over. The port can skip it.
4. The UserManager class name (no RTTI). Its functions sit after TitleScreen.cpp's last vtable function
   and before WaitDialog.cpp, so it is probably its own source file (alphabetically between "TitleScreen"
   and "WaitDialog", e.g. "User...").
5. OverlayWidget: once the bonus banner slides off (y = board height) `mBannerActive` stays true (no
   overshoot); harmless (drawn off screen, hover rect unreachable) but the port should keep it for saved
   games (it is part of the 33-byte save block).
6. During the selector's open transition BigLex is visible and is drawn on top of the squashed snapshot
   at its fixed rect (it is above the selector in the widget list); to confirm with the BigLex analysis
   whether BigLex hides itself meanwhile.
7. WordBalloon ctor's 4th/5th arguments (159, -2) are passed as one 8-byte value; meaning (tail position
   / offset) to be confirmed by the WordBalloon analysis, as are slots 49 and 51.
8. `mProductVersion` comes from the version resource of the running exe; the carved game PE says
   "1.03". If the original ran the game from its launcher process the shown version could differ; check
   a screenshot.

---

## Part 7. Dialogs

Scope: `MoneyDialog.cpp` (0x42e3d0-0x42e480), `OptionsDialog.cpp` (0x42eb40-0x430970), `NewUserDialog.cpp`
(0x42e480-0x42eb40), `SelectUserDialog.cpp` (0x4325f0-0x4330c0), `DeleteUserDialog.cpp` (0x41ddb0-0x41e880),
`GetStringDialog.cpp` (0x428f50-0x4297e0), `HintDialog.cpp` (0x42a820-0x42b4c0) and, for reference only,
`RegisterDialog.cpp` (0x431b50-0x4325f0). Sources: Ghidra export, llvm-objdump for the x87 code and lost
arguments, the PE's .rdata tables, the 2004 framework map (`re/bookworm/fw/*.md`), SAF 1.2 source. Nothing
was run. Proposed header: `dialogs.h`; Ghidra symbols: `dialogs_symbols.jsonl`.

Conventions: offsets are absolute from the object start. "app" is the BookWormApp. Coordinates passed to
`Resize`/`Move` are screen coordinates (2004 widgets are all top-level); coordinates inside `Draw` are
dialog-relative. `W`/`H` = the dialog's mWidth/mHeight, `X`/`Y` = its mX/mY, `cL/cT/cR/cB` =
mContentInsets (background insets are 0 in every dialog here). Slot numbers are the 2004 ones (Widget 47,
Dialog 52; ButtonListener at +0x58: 2 ButtonPress(id,cc), 3 ButtonPress(id), 4 ButtonDepress, 5 DownTick).
**No dialog in this group draws a random number** (no Rand, MTRand or CRT rand anywhere).

### 1. Summary

| Class | Base | sizeof | Vtables (primary / +0x58 / extra) | Dialog id | Created by |
|---|---|---|---|---|---|
| MoneyDialog | Dialog | 0x120 (inferred: never allocated, subclasses start at +0x120) | 0x500774 / 0x500758 | - | base only |
| OptionsDialog | MoneyDialog, SliderListener (+0x120), CheckboxListener (+0x124) | 0x188 (`new(0x188)` 0x4107c0) | 0x500b4c (53) / 0x500b2c / 0x500b24, 0x500b1c | 1 | app 0x4107c0 (Options button on the board) |
| NewUserDialog | MoneyDialog, EditListener (+0x120) | 0x130 (`new(0x130)` 0x4108c0) | 0x50087c / 0x50085c / 0x500848 | 22 | app 0x4108c0 |
| SelectUserDialog | MoneyDialog, ListListener (+0x120) | 0x130 (`new(0x130)` 0x410520) | 0x501004 / 0x500fe8 / 0x500fe0 | 19 | app 0x410520 (BWGameSelector button) |
| DeleteUserDialog | MoneyDialog, ListListener (+0x120) | 0x130 (`new(0x130)` 0x418590) | 0x4fc2b4 / 0x4fc298 / 0x4fc290 | 20 | app 0x418590 (from SelectUserDialog) |
| GetStringDialog | Dialog, EditListener (+0x110) | 0x128 (`new(0x128)` 0x414080) | 0x4fcbe4 (53) / 0x4fcbc8 / 0x4fcbb4 | 23 (its only use) | app 0x414080 (internet high-score alias) |
| HintDialog | Dialog | 0x120 (`new(0x120)` 0x4105d0) | 0x4fd434 / 0x4fd414 | 15 | app 0x4105d0 (Board::Update hints) |
| RegisterDialog (not ported) | Dialog, EditListener (+0x110) | 0x138 (`new(0x138)` 0x413cb0) | 0x500efc / 0x500ee0 / 0x500ecc | 2 | app 0x413cb0 |

Related app-built dialogs used by this group (app group code, listed for reconciliation): 18 = generic
message box (`DoDialog(18, modal, header, lines, `GT_DIALOG_OK`, FOOTER)`), 21 = `GT_ARE_YOU_SURE` delete confirmation
(YES/NO), 16 = CreditsDialog.

Every dialog here is modal (isModal true) and uses the dialog images `dialog.gif` (240x240, IMAGE 7,
app images +0x1c) as component image and `dialogbutton.gif` (90x31, IMAGE 8, +0x20) as button component
image, except OptionsDialog whose buttons use `dialogbutton_plain.gif` (90x26, IMAGE 142, +0x238).

#### 1.1 Shared facts

- App image table: `app+0x11ac` is `Image*[149]` loaded from the name table at 0x52e410 (`images\` +
  name). Indices used here: 7 `dialog`, 8 `dialogbutton`, 2 `backdrop_register`, 9/10/11
  `button_reg_register`/`_cancel`/`_regcode`, 12 `editbox` (96x32: three 32x32 cells: left cap, middle,
  right cap), 49 `ihs_alias_warning` (325x73), 142 `dialogbutton_plain`, 143 `slidertrack` (189x22), 144
  `sliderthumb` (25x25), 145 `checkboxes` (52x26). Checkbox cells are cut by the loading thread
  (0x41a8a0): `app+0x1200` = CopyImage(checkboxes, Rect(0,0,w/2,h)) = **unchecked**, `app+0x11fc` =
  CopyImage(checkboxes, Rect(w/2,0,w/2,h)) = **checked** (both 26x26).
- App fonts (`app+0x11b0 + 4*i`, ImageFonts from the table at 0x52e3c0): +0x11b0 Verdana10Bold, +0x11b4
  Verdana12Bold, +0x11b8 Verdana14Bold, +0x11bc Verdana18BoldColor, +0x11e8 LockergnomeSmallColor. Metrics
  from the descriptors (graphics group to confirm): Verdana10Bold ascent 13 / height 16, Verdana12Bold 16 /
  19, Verdana14Bold 19 / 23, Verdana18BoldColor 29 / 39 with ascent padding 5 and line spacing offset -5.
- Sounds: `app->PlaySample(0)` = `buttonclick`, `PlaySample(23)` = `goldtile` (gSoundNames 0x52e668).
  BookWormApp::ButtonPress (0x410b30, app slot 3) is `PlaySample(0)`, so every dialog yes/no/footer button
  press clicks once through the app.
- Dialog 2004 behaviour this group relies on (docs/FRAMEWORK_2004.md section 3, 6): yes/footer button id `mId+2000`,
  no `mId+3000`, labels "YES"/"NO" (mode 1), `GT_DIALOG_OK`/"CANCEL" (mode 2), footer text (mode 3); Dialog's
  +0x58 ButtonPress(id)/ButtonDepress(id)/DownTick(id) forward only those two ids to the app's slots 3/4/5;
  button height = `mYesButton->mComponentImage->GetHeight()` (31 for dialogbutton, 26 for
  dialogbutton_plain); default mContentInsets (24,24,24,24), mSpaceAfterHeader 10, mButtonHorzSpacing 8;
  `Dialog::SetColor(3, c)` / `(4, c)` set colour 0 / 1 of both buttons; default colours header white,
  lines yellow, footer white, buttons white/white. Dialog::Resize and GetPreferredHeight are the 1.2
  formulas (docs/FRAMEWORK_2004.md section 3, 6.3). DialogButtons draw their label colourised with mColors[1] when hovered,
  else mColors[0].
- Folded (identical-COMDAT) bodies shared across classes: 0x429290 (`jmp Dialog::Update`) is slot 17 of
  MoneyDialog, Select/Delete/NewUser (inherited), GetStringDialog, HintDialog and RegisterDialog: an
  `Update() { Dialog::Update(); }` override, behaviourally Dialog::Update (Widget::Update: mUpdateCnt++).
  0x42e440 (`Dialog::Draw(g)`) is MoneyDialog::Draw and HintDialog::Draw. 0x432670/0x432680/0x4326b0/
  0x4326e0 are SetColor/AddedToManager/RemovedFromManager/Draw of both SelectUserDialog and DeleteUserDialog.
  0x41df60 is ButtonPress(id) of both. 0x42e5f0 is AllowChar of NewUserDialog and GetStringDialog.

#### 1.2 The edit-box frame (NewUserDialog 0x42e650, GetStringDialog slot 52 0x4295c0)

`DrawEditBox(Graphics* g, const Rect& r)` (only r.mX, r.mY, r.mWidth used; the frame is 32 high) with image
`img` (= app IMAGE_EDITBOX for NewUserDialog, `mEditBoxImage` +0x120 for GetStringDialog):
1. `g->DrawImage(img, r.x, r.y, Rect(0,0,32,32))` (left cap).
2. `g->DrawImage(img, r.x + r.w - 32, r.y, Rect(64,0,32,32))` (right cap).
3. Copy the Graphics, `ClipRect(r.x + 32, r.y, r.w - 32, 32)`; `n = (r.w - 33) / 32` (C int division,
   truncation toward zero); for i in 0..n-1: `DrawImage(img, r.x + 32 + 32*i, r.y, Rect(32,0,32,32))`.
   n = ceil((w-64)/32) tiles, so the last middle tile overhangs into the right cap (the clip ends at
   r.x + r.w, not r.x + r.w - 32) and is drawn **after** the cap: with r.w = 312 the last tile covers
   x+256..x+288 over the cap at x+280..x+312 (8 px). Keep the order.

### 2. MoneyDialog (MoneyDialog.cpp)

The name and size come from Big Money (where it drew a chain of running lights); in BookWorm it is a bare
Dialog with an app pointer. It adds no behaviour.

| Off | Type | Name | Meaning / ctor | Read/written by |
|---|---|---|---|---|
| +0x000..0x10f | Dialog | | | |
| +0x110 | BookWormApp* | mApp | ctor arg | never read by BookWorm code (subclasses keep their own copy) |
| +0x114 | | (pad) | | |
| +0x118 | double | (mLightPhase in Big Money) | **never initialised, never used**; forces 8-byte alignment (subclass sizes 0x130/0x188 are multiples of 8) | - |

Functions:
- **0x42e3d0 `MoneyDialog(BookWormApp* theApp, Image* theComponentImage, Image* theButtonComponentImage,
  int theId, bool isModal, const string& theHeader, const string& theLines, const string& theFooter, int
  theButtonMode)`**: `Dialog(...)` with the last eight arguments, vtables, `mApp = theApp`. Nothing else.
- 0x42e420 dtor body: restore vtables, `Dialog::~Dialog` body. 0x42e450 slot 0 scalar deleting dtor.
- 0x42e440 slot 16 `Draw(g)`: `Dialog::Draw(g)`.
- slot 17 `Update()`: 0x429290 = Dialog::Update.

### 3. OptionsDialog (OptionsDialog.cpp, dialog 1)

Modal options panel that slides in from the right edge, with music and sound sliders, five checkboxes,
Help / Register / Credits buttons, the partner "more games" links and a `GT_OPTIONS_RETURN_TO_GAME` footer button.

#### 3.1 Layout

| Off | Type | Name | Meaning / ctor value | Read/written by |
|---|---|---|---|---|
| +0x110 | BookWormApp* | (MoneyDialog::mApp) | app | - |
| +0x118 | double | (MoneyDialog pad) | | |
| +0x120 | vfptr | SliderListener | 0x500b24 | |
| +0x124 | vfptr | CheckboxListener | 0x500b1c | |
| +0x128 | BookWormApp* | mApp2 | app (all OptionsDialog code uses this copy) | everything |
| +0x12c | Slider* | mMusicSlider | id 42, track/thumb images, value app->GetMusicVolume() | Resize, SliderVal |
| +0x130 | Slider* | mSfxSlider | id 43, value app->GetSfxVolume() | Resize, SliderVal (reads +0x74 mDragging) |
| +0x134 | Checkbox* | mFullscreenCheckbox | id 37, checked = !app->mIsWindowed (+0x306) | ApplyAndClose, CheckboxChecked, Draw |
| +0x138 | Checkbox* | mCustomCursorsCheckbox | id 38, checked = app->mCustomCursorsEnabled (+0x52b) | ApplyAndClose |
| +0x13c | Checkbox* | mShowHintsCheckbox | id 39, checked = app+0x12e8 (show hints) | ApplyAndClose |
| +0x140 | Checkbox* | mShowDefsCheckbox | id 40, checked = app+0x12e9 (show definitions) | ApplyAndClose |
| +0x144 | Checkbox* | m3DCheckbox | id 41, checked = app->Is3DAccelerated(); always created (the NULL checks are dead) | ApplyAndClose, CheckboxChecked |
| +0x148 | DialogButton* | mRegisterButton | id 34 | added/laid out only when !app->mBuildUnlocked |
| +0x14c | DialogButton* | mHelpButton | id 33 | |
| +0x150 | DialogButton* | mCreditsButton | id 35 | |
| +0x154 | std::list<DialogButton*> | mLinkButtons | allocator byte +0x154, head +0x158 (`_Buyhead` in ctor), size +0x15c = 0 | |
| +0x160 | std::vector<std::string> | mLinkURLs | first +0x164, last +0x168, end +0x16c = 0 | ButtonDepress |
| +0x170 | int | mSlideOffset | 0 in ctor; creator sets 240; ApplyAndClose sets -240 | Update |
| +0x174 | | (pad) | | |
| +0x178 | double | mSlidePhase | 0.0 | Update |
| +0x180 | bool | mClosing | false | Update |
| +0x181..0x187 | | (pad to 0x188) | | |

The ctor also sets `mHasAlpha` (+0x22) = true (already true from Dialog).

#### 3.2 Construction

- **Creator (app 0x4107c0, cross-module)**: `new OptionsDialog(app)`; `SetColor(0, white)`, `SetColor(1,
  white)`, `SetColor(3, black)` (virtual: OptionsDialog::SetColor); `h = GetPreferredHeight(388)`;
  `Resize(app->mWidth, (app->mHeight - h) / 2, 388, h)` (starts off-screen at x = 640);
  `mSlideOffset = 240`; `app->AddDialog(1, dlg)`.
- **0x42fe90 `OptionsDialog(BookWormApp* theApp)`**:
  1. `MoneyDialog(app, IMAGE_DIALOG, IMAGE_DIALOGBUTTON_PLAIN, id 1, modal, header `GT_OPTIONS`, lines "",
     footer `GT_OPTIONS_RETURN_TO_GAME`, mode 3 FOOTER)`: one footer button, id 2001, label `GT_OPTIONS_RETURN_TO_GAME`.
  2. Listener vtables, list head, zero the fields above, mApp2 = app, mHasAlpha = true.
  3. `SetHeaderFont(Verdana18BoldColor)`, `SetLinesFont(Verdana14Bold)`, `SetButtonFont(Verdana12Bold)`.
  4. Register button: `new DialogButton(IMAGE_DIALOGBUTTON_PLAIN, 34, this+0x58)`, `SetFont(Verdana12Bold)`,
     label = `app->GetString("RegisterLabel", `GT_OPTIONS_REGISTER`)`, `SetColors(gOptionsButtonColors, 2)`
     (int[2][3] at 0x52f5c0: (221,152,83) label, (255,255,0) hilite).
  5. Help button: id 33, same font, label `GT_OPTIONS_HELP`, same colours. Credits button: id 35, label `GT_OPTIONS_CREDITS`.
  6. Links: `n = app->GetInteger("NumOptionsLinks", 1)`, and if n >= 4 then n = 3 (min(n,3); a negative n
     makes no links). For i in 0..n-1: `new DialogButton(IMAGE_DIALOGBUTTON_PLAIN, **1036**, this+0x58)`
     (the same id for every link), `SetFont(Verdana12Bold)`, `SetColors(gOptionsButtonColors, 2)`, label =
     `app->GetString(StrFormat("OptionsLinkText%d", i), `GT_OPTIONS_MORE_GAMES`)`, push_back on
     mLinkButtons; `mLinkURLs.push_back(app->GetString(StrFormat("OptionsLinkURL%d", i),
     "http://www.popcap.com"))`.
  7. Sliders: `new Slider(IMAGE_SLIDERTRACK, IMAGE_SLIDERTHUMB, 42, this+0x120)`,
     `SetValue(app->GetMusicVolume())` (app slot 43); sound: id 43, `SetValue(app->GetSfxVolume())`
     (slot 44). Slider is `new(0x80)`.
  8. Checkboxes (`new(0x8c)` each, unchecked image app+0x1200, checked image app+0x11fc, listener
     this+0x124), mChecked written directly (no listener call): 37 fullscreen = `!app->mIsWindowed`; 38
     custom cursors = `app->mCustomCursorsEnabled`; 39 show hints = `app+0x12e8`; 40 show definitions =
     `app+0x12e9`; 41 hardware acceleration = `app->Is3DAccelerated()`.
  9. Footer button (mYesButton): `SetColors(gOptionsButtonColors, 2)`.
  Final button colours after the creator's SetColor(3, black): label black, hilite (255,255,0) on the
  footer, Register/Help/Credits and link buttons (slot 7 below).
- 0x42fc90 dtor body: deletes Register, Credits, music, sfx, the five checkboxes, Help (all through their
  slot 0), every link button (and its list node), destroys the URL strings and frees both containers, then
  MoneyDialog's dtor body. 0x42fe60 slot 0 deleting dtor.

#### 3.3 Functions

- **0x42f450 slot 50 `GetPreferredHeight(int theWidth)`**: ignores the width and the Dialog formula.
  Function-local statics (guard bits in 0x534de0) computed on the first call only: `gNumOptionsLinks`
  (0x534ddc) = min(GetInteger("NumOptionsLinks", 1), 3) and `gOptionsDialogHeight` (0x534dd8) =
  GetInteger("OptionsDialogHeight", 330). Returns `gOptionsDialogHeight + 36 * gNumOptionsLinks`. With the
  shipped partner.xml (315, 1 link): 351, so the dialog is 388x351 at y = (480-351)/2 = 64.
- **0x42f090 slot 13 `Resize(x, y, w, h)`** (also runs every slide frame):
  1. `Dialog::Resize(x, y, w, h)`.
  2. Footer button: `sw = mYesButton->mFont->StringWidth(mYesButton->mLabel)`;
     `mYesButton->Resize(X + (W - (sw + 40)) / 2, Y + H - 60, sw + 40, 32)`.
  3. `rowY = mYesButton->mY - 36`. `linkW = max over links of (link->mFont->StringWidth(link->mLabel) + 30)`
     (starting from 0). Each link in list order: `Resize(X + (W - linkW) / 2, rowY, linkW, 32)`,
     `rowY -= 36`.
  4. Button row at rowY, height 32. If `!app->mBuildUnlocked`: `bw = (W - cR - cL - 6) / 3`; Help at
     `X + cL`, Register at `X + cL + 3 + bw`, Credits at `X + cL + 6 + 2*bw` (each skipped if NULL). Else
     (registered build): `bw = (W - cR - cL - 3) / 2`; Help at `X + cL`, Credits at `X + cL + 6 + bw` (the
     pair is 3 px wider than the content area: kept).
  5. `mMusicSlider->Resize(X + 152, Y + 68, W - 192, 36)`; `mSfxSlider->Resize(X + 152, Y + 100, W - 192, 36)`.
  6. Checkboxes 26x26: fullscreen `(X+30, Y+138)`, show hints `(X+30, Y+164)`, hardware acceleration
     `(X+30, Y+190)`, custom cursors `(X+190, Y+138)`, show definitions `(X+190, Y+164)`.
  With the defaults (W 388, cL = cR = 24, registered): footer at Y+291, link row Y+255, Help/Credits row
  Y+219 (Help 24..192, Credits 198..366); with no links the footer is at Y+255 and the button row is still
  Y+219.
- **0x42f600 slot 16 `Draw(g)`**: `Dialog::Draw(g)` (frame, header `GT_OPTIONS` in mColors[0] = white with
  Verdana18BoldColor; empty lines; no footer text in mode 3). Then `SetFont(Verdana12Bold)`,
  `SetColorizeImages(true)`, colour black: `GT_OPTIONS_MUSIC` at (49, 89), `GT_OPTIONS_SOUND_FX` at (49, 121) (shadows); colour
  `mColors[1]` (white): `GT_OPTIONS_MUSIC` at (48, 88), `GT_OPTIONS_SOUND_FX` at (48, 120); each checkbox label (no shadow) at
  `(cb->mX - X + 28, cb->mY - Y + 18)`: `GT_OPTIONS_FULL_SCREEN`, `GT_OPTIONS_CUSTOM_CURSORS`, `GT_OPTIONS_HARDWARE_ACCELERATION` (if
  m3DCheckbox), `GT_OPTIONS_SHOW_HINTS`, `GT_OPTIONS_SHOW_DEFINITIONS`; `SetColorizeImages(false)`. DrawString y is the baseline.
- **0x42ebe0 slot 17 `Update()`**: `Dialog::Update()`. If `mSlideOffset == 0`: if mClosing,
  `app->KillDialog(mId)` (app slot 58); return. Else `mSlidePhase += 0.055` (double, stored). If
  `mSlideOffset > 0` (sliding in): `mSlideOffset = phase < 1.570795 ? (int)((1.0 - sin(phase)) * W) : 0`
  and `Resize(240 + mSlideOffset, Y, W, H)`. Else (sliding out): `mSlideOffset = phase < 1.570795 ?
  -(int)((1.0 - sin(phase)) * W) : 0` and `Resize(W + 240 + mSlideOffset, Y, W, H)`. Then `MarkDirty()`.
  x87: `fsin` of the stored double, `fsubr 1.0`, `fimul` by the int mWidth, `_ftol` (truncation). The 240
  is a constant (not derived from the screen). Slide-in: frame 1 x = 240 + (int)(0.945028 * 388) = 606,
  reaches x = 240 when the truncated product hits 0 (frame 28, phase 1.54), then stops. Slide-out (after
  ApplyAndClose): frame 1 x = 388 + 240 - 366 = 262 ... until offset 0 puts it at x = 628 (12 px still
  on screen); the next Update kills the dialog. Constants: 0.055 at 0x500960, 1.570795 at 0x4fd950, 1.0 at
  0x4f9ae8, 0.0 at 0x4f9ad8.
- **0x42ed00 slot 52 (new virtual) `ApplyAndClose()`**: called by the app on 2001/3001 and by
  ShowHelp 0x4101c0.
  1. `windowed = !mFullscreenCheckbox->IsChecked()`; `want3D = m3DCheckbox && m3DCheckbox->IsChecked()`.
  2. If `app->mIsWindowed != windowed || app->Is3DAccelerated() != want3D`:
     `app->SwitchScreenMode(windowed, want3D)` (app slot 52).
  3. `app->EnableCustomCursors(mCustomCursorsCheckbox->IsChecked())`.
  4. `app+0x12e8 = mShowHintsCheckbox->IsChecked()`; `app+0x12e9 = mShowDefsCheckbox->IsChecked()`.
  5. `app->SaveCurrentProfile()` (0x412100: writes the current user's registry values, section 8).
  6. `mClosing = true`, `mSlidePhase = 0.0`, `mSlideOffset = -240` (any non-zero negative: it only selects
     the slide-out branch). A second click during the slide-out repeats all of this (literal).
- **0x42ee20 slot 7 `SetColor(int idx, const Color& c)`**: `Dialog::SetColor(idx, c)`; if idx == 3 also
  `SetColor(0, c)` on Register, Help, Credits (if non-NULL) and every link button.
- **0x42eeb0 slot 1 `AddedToManager(wm)`**: `Dialog::AddedToManager(wm)` (adds the footer button), then
  AddWidget in this order: Help, music slider, sfx slider, custom cursors, fullscreen, show hints, show
  definitions, hardware acceleration (if non-NULL), Credits, Register (only if
  `!gSexyAppBase->mBuildUnlocked`, +0x471), every link button.
- **0x42efa0 slot 2 `RemovedFromManager(wm)`**: `Dialog::RemovedFromManager(wm)`, then RemoveWidget: Help,
  music, sfx, fullscreen, custom cursors, hints, definitions, hardware (if non-NULL), Credits, Register
  (if !mBuildUnlocked), links.
- **0x42eba0 +0x58 slot 3 `ButtonPress(int id)`**: `Dialog::ButtonPress(id)` (2001 goes to the app, which
  clicks); for any other id `app->PlaySample(0)`.
- **0x42fbe0 +0x58 slot 4 `ButtonDepress(int id)`**: `Dialog::ButtonDepress(id)` (2001 to the app). Then:
  `id % 1000 > 35` -> `app->OpenURL(mLinkURLs[id % 1000 - 36], false)` (link id 1036 -> URL 0, for every
  link button); id 33 -> `app->ShowHelp()` (0x4101c0: ApplyAndClose on dialog 1, then creates the
  HelpWidget); id 34 -> `app->mRegSource (+0x37c) = "options"`, `app->OpenRegisterDialog()` (0x413cb0);
  id 35 -> `app->ShowCredits()` (0x4106c0, CreditsDialog 16 on top of the options).
- **0x42eb40 +0x120 `SliderVal(int id, double val)`**: id 42 -> `app->SetMusicVolume(val)` (app slot 47,
  BookWormApp override 0x410110); id 43 -> `app->SetSfxVolume(val)` (slot 48), then if
  `!mSfxSlider->mDragging` -> `app->PlaySample(23)` ("goldtile"). The 2004 Slider calls SliderVal while
  dragging (mDragging true: silent) and once on every mouse-up after clearing mDragging (sound), also on
  a plain click (no jump: Slider2004).
- **0x42fa50 +0x124 `CheckboxChecked(int id, bool checked)`**:
  1. `app->PlaySample(0)`.
  2. If `!checked && app->mForceFullscreen` (+0x307): `mFullscreenCheckbox->SetChecked(true, true)` (for
     **any** checkbox id; the nested call re-enters with (37, true) and clicks again).
  3. If m3DCheckbox and id == 41: `m3DCheckbox->mChecked = checked`; if checked: if
     `app->Is3DAccelerationSupported()`: if `Is3DAccelerationRecommended()` return; else
     `app->DoDialog(18, true, `GT_OPTIONS_3D_WARNING_HEADER`, <0x500a00 `GT_OPTIONS_3D_WARNING`> , "OK", 3)`. If not supported: `m3DCheckbox->mChecked = false` and `DoDialog(18, true,
     `GT_OPTIONS_3D_NOT_SUPPORTED_HEADER`, <0x500a90 `GT_OPTIONS_3D_NOT_SUPPORTED`>, `GT_DIALOG_OK`, 3)`.
  Nothing else is applied until ApplyAndClose.

What the controls change: music/sfx volume immediately (app fields mMusicVolume/mSfxVolume; written to the
registry as MusicVolume/SfxVolume by SexyApp::WriteToRegistry at shutdown); on RETURN TO GAME: screen mode
(registry ScreenMode at shutdown), 3D (DDInterface, registry "Is3D" in the framework), custom cursors
(registry CustomCursors at shutdown), show hints / show definitions (app+0x12e8/+0x12e9, saved at once into
`Users\<current>\ShowHints` / `ShowDefs`).

### 4. NewUserDialog (NewUserDialog.cpp, dialog 22)

`GT_NEW_USER_HEADER` dialog with one edit box for the player name.

| Off | Type | Name | Meaning / ctor | Read/written by |
|---|---|---|---|---|
| +0x110 | BookWormApp* | (MoneyDialog::mApp) | app | |
| +0x118 | double | (MoneyDialog pad) | | |
| +0x120 | vfptr | EditListener | 0x500848 | |
| +0x124 | BookWormApp* | mApp2 | app | Draw, EditWidgetText |
| +0x128 | EditWidget* | mNameEdit | `new EditWidget(0, this+0x120)` | Resize, Draw, app 0x417080 (reads mString +0x5c) |
| +0x12c | | (pad to 0x130) | | |

- **Creator (app 0x4108c0, `ShowNewUserDialog(const string& theLines)`)**: if `app+0x1168` (the
  BWGameSelector) exists: `SetDisabled(true)` (Widget slot 10). `new NewUserDialog(app, theLines)`;
  `h = GetPreferredHeight(400)`; `Resize(238, 174, 400, h)`; `AddDialog(22)`. Callers: SelectUserDialog
  `GT_USER_LIST_CREATE` with lines `GT_SELECT_USER_ENTER_NAME`
  (0x501138), and BWGameSelector::Update (0x402190) on first run (no profiles, dialog 22 not open) with
  "Enter your name.\r\n\r\n(Example: ...)" (0x4f94e4).
- **0x42e7c0 `NewUserDialog(BookWormApp* theApp, const string& theLines)`**: `MoneyDialog(app,
  IMAGE_DIALOG, IMAGE_DIALOGBUTTON, 22, modal, `GT_NEW_USER_HEADER`, theLines, "", mode 2 OK_CANCEL)` (buttons 2022
  `GT_DIALOG_OK`, 3022 "CANCEL"); listener vtable; mApp2 = app; `SetHeaderFont(Verdana18BoldColor)`,
  `SetButtonFont(Verdana12Bold)`, `SetLinesFont(Verdana10Bold)`; `mContentInsets = Insets(38, 38, 38,
  38)`; `SetColor(1, white)` (lines); `SetColor(3, black)` (button labels; hilite stays white). Edit:
  `new EditWidget(0, this+0x120)` (`new(0xec)`), `SetFont(Verdana12Bold, NULL)`,
  `SetColors(gNewUserEditColors, 5)` (int[5][4] at 0x52f530: bkg (0,0,0,0), outline (0,0,0,0), text
  (240,240,255,255), selection (255,255,255,255), selected text (0,0,0,255)), `mMaxPixels = 200`,
  `mMaxChars = 22`. The header colour stays the Dialog default (white).
- 0x42e480 dtor body: deletes mNameEdit (slot 0), MoneyDialog body. 0x42e620 slot 0. It does not
  re-enable the game selector (the app does, on 2022/3022).
- **0x42e4f0 slot 50 `GetPreferredHeight(w)`** = `Dialog::GetPreferredHeight(w) + 56`. Dialog part: 76
  (insets) + header (39 - 5) + 10 + word-wrapped height of the lines in Verdana10Bold at width w - 80 + 8 +
  31 + 8.
- **0x42e580 slot 13 `Resize(x, y, w, h)`**: `Dialog::Resize` (OK at (X+38, Y+H-69, 158, 31), CANCEL at
  (X+204, Y+H-69, 158, 31) for W 400), then `mNameEdit->Resize(X + 48, Y + H - 110, W - 96, 24)`.
- **0x42ea10 slot 16 `Draw(g)`**: `MoneyDialog::Draw(g)`; `r = mNameEdit->GetRect()` made dialog-relative
  and grown: `(r.x - X - 4, r.y - Y - 4, r.w + 8, r.h + 8)` = (44, H-114, 312, 32);
  `DrawEditBox(g, r)` with IMAGE_EDITBOX (section 1.2); `SetFont(Verdana10Bold)`, colour (255,173,17),
  `DrawString(`GT_ENTER_YOUR_NAME`, mNameEdit->mX - X, mNameEdit->mY - Y - 8)` = (48, H-118).
- **0x42e510 slot 1 `AddedToManager(wm)`**: `Dialog::AddedToManager(wm)`, `wm->AddWidget(mNameEdit)`,
  `wm->SetFocus(mNameEdit)`. **0x42e550 slot 2**: `Dialog::RemovedFromManager(wm)`,
  `wm->RemoveWidget(mNameEdit)`.
- **0x42e5d0 +0x120 `EditWidgetText(int id, const string&)`** (Enter in the edit box):
  `app->ButtonDepress(mId + 2000)` (app slot 4, i.e. exactly the OK path, without the click sound).
- **0x42e5f0 +0x120 `AllowChar(int id, char c)`**: `isalpha((unsigned char)c) || c == ' '` (CRT C
  locale: A-Z, a-z and space only; digits and punctuation refused). AllowKey/AllowText are the default
  "true" stubs. The EditWidget also applies AllowChar to pasted text.
- ButtonPress/ButtonDepress are Dialog's (2022/3022 go to the app; see section 8 for what the app does).

### 5. SelectUserDialog (SelectUserDialog.cpp, dialog 19)

`GT_SELECT_USER_HEADER` roster with a list of up to five profiles and the special lines `GT_USER_LIST_CREATE` /
`GT_USER_LIST_DELETE`.

| Off | Type | Name | Meaning / ctor | Read/written by |
|---|---|---|---|---|
| +0x110 | BookWormApp* | (MoneyDialog::mApp) | | |
| +0x118 | double | (MoneyDialog pad) | | |
| +0x120 | vfptr | ListListener | 0x500fe0 | |
| +0x124 | BookWormApp* | mApp2 | app | all |
| +0x128 | ListWidget* | mUserList | `new ListWidget(0, Verdana10Bold, this+0x120)` | all; DeleteUserDialog via app |
| +0x12c | | (pad to 0x130) | | |

- **Creator (app 0x410520, `ShowSelectUserDialog()`)**: `new SelectUserDialog(app)`; `SetColor(0, white)`
  (header); `Resize(0, 17, 400, GetPreferredHeight(400))`; `AddDialog(19)`.
- **0x432c90 `SelectUserDialog(BookWormApp* theApp)`**: `MoneyDialog(app, IMAGE_DIALOG,
  IMAGE_DIALOGBUTTON, 19, modal, `GT_SELECT_USER_HEADER`, "", "", mode 2)` (2019 "OK", 3019 "CANCEL"); listener
  vtable; mApp2; `SetHeaderFont(Verdana18BoldColor)`, `SetLinesFont(Verdana14Bold)`, `SetColor(3, black)`,
  `SetButtonFont(Verdana10Bold)`; a dead call `app->mProfileMgr->mProfiles.size()` (0x496c00, result
  unused); list: `new ListWidget(0, Verdana10Bold, this+0x120)` (`new(0xc0)`),
  `SetColors(gUserListColors, 5)` (int[5][3] at 0x52f788: bkg (53,43,29), outline (0,0,0), text
  (255,225,73), hilite text (255,255,255), selection fill (188,22,22)), `mDrawOutline = true` (+0xb5),
  `mJustify = 1` (centred, +0x68); `UpdateList()`; game selector (`app+0x1168`) `SetDisabled(true)`.
  Content insets stay (24,24,24,24).
- 0x4325f0 dtor body: game selector `SetDisabled(false)`; MoneyDialog body. **mUserList is never
  deleted** (0xc0-byte leak per dialog; `port:` delete it). 0x4326f0 slot 0.
- **0x432810 `UpdateList()`** (non-virtual): if mUserList:
  1. Destroy and free `mUserList->mLines` (vector set to empty). mLineColors, mSelectIdx, mHiliteIdx,
     mPosition are not touched (port: `mLines.clear()`, not RemoveAll).
  2. Walk `app->mProfileMgr->mProfiles` (std::list<Profile*>) with a counter i = 0: on the first node, if
     `mProfiles.size() < 5`: `AddLine(`GT_USER_LIST_CREATE`, false)`, i = 1. For a non-NULL profile: if
     i < 5: `idx = AddLine(profile->mName, false)`; if `profile->mName == mgr->GetCurrentUser()` (exact,
     case-sensitive compare) `SetSelect(idx)`; else `OutputDebugString("ERROR: Too many user profiles.
     '%s' skipped.\n")`. i++ per node.
  3. If `mProfiles.size() == 5` exactly: `AddLine(`GT_USER_LIST_DELETE`, false)`.
  4. `MarkDirty()`.
  So: fewer than 5 profiles -> `GT_USER_LIST_CREATE` first, then the names, no delete line; exactly 5 ->
  the 5 names then `GT_USER_LIST_DELETE`; 6+ (only by registry editing) -> the first 5, no special lines; 0
  profiles -> an empty list. If the current user is not found, mSelectIdx keeps its old value.
- **0x432b00 slot 50 `GetPreferredHeight(w)`**: `h = Dialog::GetPreferredHeight(w)` (= 24+24 + 34 + 8 +
  31 + 8 = 129 with the fonts above: header only); `UpdateList()`; return `h + 16 +
  mUserList->GetOptimalHeight()` (lines * 16 + 8).
- **0x432b40 slot 13 `Resize(x, y, w, h)`**: `Dialog::Resize`; `UpdateList()`; `optH =
  mUserList->GetOptimalHeight()`; `mUserList->Resize(X + 40, mYesButton->mY - (optH + 16), W - 80,
  optH)` (yes button already placed by Dialog::Resize at Y + H - cB - 31); then the buttons again: with
  both buttons `bw = (W - cR - cL - 48) / 2`, `bh = mYesButton->mComponentImage->GetHeight()` (31): OK at
  `(X + 16 + cL, Y + H - cB - bh, bw, bh)`, CANCEL at `(X + cL + 32 + bw, same y, bw, bh)` (W 400: OK
  40..192, CANCEL 208..360, list 40..360); with only a yes button `(X + cL, Y + H - cB - bh, W - cR - cL,
  bh)`.
- 0x4326e0 slot 16 `Draw(g)`: MoneyDialog::Draw (= Dialog::Draw: header `GT_SELECT_USER_HEADER` white).
- 0x432680 slot 1: `Dialog::AddedToManager(wm)`; AddWidget(mUserList) if non-NULL. 0x4326b0 slot 2: the
  reverse. 0x432670 slot 7 `SetColor` = `Dialog::SetColor`.
- **0x41df60 +0x58 slot 3 `ButtonPress(int id)`** (shared with DeleteUserDialog):
  `Dialog::ButtonPress(id)`; if id is not mId+2000/mId+3000: `app->PlaySample(0)`.
- **0x432720 +0x58 slot 4 `ButtonDepress(int id)`**: `Dialog::ButtonDepress(id)` (the app kills dialog 19
  for both 2019 and 3019; the delete is deferred so the object stays valid). If id == mId+2000 and
  mUserList and `0 <= mSelectIdx < mLines.size()`: `p = mgr->FindProfile(mLines[mSelectIdx])`; if p:
  `mgr->SetCurrentUser(p->mName)`, `app->LoadCurrentProfile()` (0x413a40). Selecting a special line and
  pressing OK just closes the dialog.
- **0x432f00 +0x120 `ListClicked(int id, int idx, int clickCount)`**: only if mUserList and
  `mUserList->mId == id`: if `idx != mSelectIdx`: `app->PlaySample(0)`, `mUserList->SetSelect(idx)`.
  `name = mLines[idx]`. Then:
  - name == `GT_USER_LIST_CREATE` (any click count): `app->ShowNewUserDialog(GameText(GT_SELECT_USER_ENTER_NAME))` (dialog 22 on top; this dialog stays open);
  - name == `GT_USER_LIST_DELETE`: copy `mLines[0..4]` (always five entries: only offered with exactly five
    profiles) into a vector and call `app->ShowDeleteUserDialog(names)` (0x418590, dialog 20 on top);
  - otherwise, if clickCount == 2 (double click): `p = mgr->FindProfile(name)`; if p
    `mgr->SetCurrentUser(p->mName)`; `app->KillDialog(mId)`. **No LoadCurrentProfile here** (the OK button
    does call it), so a double-click switches the user without reloading their ShowHints/ShowDefs/
    ShowHelp/TimesPlayed (original behaviour; open question 1).

### 6. DeleteUserDialog (DeleteUserDialog.cpp, dialog 20)

`GT_DELETE_USER_HEADER` list of the five names with OK/CANCEL; OK (or a double-click) asks `GT_ARE_YOU_SURE` (dialog
21, YES/NO), whose YES makes the app call `DeleteSelectedUser()`.

Layout identical to SelectUserDialog: +0x120 ListListener (0x4fc290), +0x124 mApp2, +0x128 mUserList,
pad to 0x130.

- **Creator (app 0x418590, `ShowDeleteUserDialog(std::vector<std::string> theNames)` by value)**: `new
  DeleteUserDialog(app, names)`; `SetColor(0, white)`; `Resize(0, 17, 400, GetPreferredHeight(400))`;
  `AddDialog(20)`.
- **0x41e370 `DeleteUserDialog(BookWormApp* theApp, std::vector<std::string> theNames)`**: `MoneyDialog(app,
  IMAGE_DIALOG, IMAGE_DIALOGBUTTON, 20, modal, `GT_DELETE_USER_HEADER`, `GT_DELETE_USER_LINES`, "", mode 2)` (2020 "OK", 3020 "CANCEL"); fonts and colours as
  SelectUserDialog (header Verdana18BoldColor, lines Verdana14Bold in the default lines colour - yellow,
  since the creator only sets colour 0 -, button labels black, buttons Verdana10Bold); list `new
  ListWidget(0, Verdana10Bold, this+0x120)`, `SetColors(gDeleteListColors, 5)` (0x52ea00, same five
  colours as 0x52f788), mDrawOutline = true, mJustify = 1; `AddLine(name, false)` for every name in order
  (mSelectIdx stays -1); the by-value vector is destroyed. No UpdateList, no game-selector change.
- 0x41ddb0 dtor body: MoneyDialog body only (**list leaked** as in SelectUserDialog). 0x41dfb0 slot 0.
- **0x41ddd0 slot 50 `GetPreferredHeight(w)`** = `Dialog::GetPreferredHeight(w) + 16 +
  mUserList->GetOptimalHeight()` (if the list exists). **0x41de10 slot 13 `Resize`**: same code as
  SelectUserDialog::Resize without UpdateList. Slots 1/2/7/16 and +0x58 slot 3: the shared Select/Delete
  functions (section 5).
- **0x41e150 +0x120 `ListClicked(id, idx, clickCount)`**: if mUserList and `mUserList->mId == id`: if
  `idx != mSelectIdx`: `PlaySample(0)`, `SetSelect(idx)`; `name = mLines[idx]`; if clickCount == 2:
  `ConfirmDelete(name)`.
- **0x41e220 +0x58 slot 4 `ButtonDepress(int id)`**: `Dialog::ButtonDepress(id)` (the app: 2020 ->
  KillDialog(21) - a no-op normally -, 3020 -> KillDialog(20)). Then if id == mId+2000: if mUserList and
  `mSelectIdx >= 0`: `ConfirmDelete(mLines[mSelectIdx])` (nothing happens with no selection; the dialog
  stays). If id == mId+3000: `dlg = app->GetDialog(19)`; if dlg: `dlg->UpdateList()` (refreshes the
  roster underneath, no resize).
- **0x41e020 `ConfirmDelete(const string& theName)`**: `app->DoDialog(21, true, `GT_ARE_YOU_SURE`,
  StrFormat(`GT_CONFIRM_DELETE_FORMAT`, theName), "", 1 YES_NO)` (the
  app's DoDialog override 0x4103e0 sets its fonts/colours and re-fits its height).
- **0x41e620 `DeleteSelectedUser()`** (called only by the app on 2021, the confirmation's YES; afterwards
  the app kills dialogs 21 and 20): if mUserList and `mSelectIdx >= 0`:
  1. `name = mLines[mSelectIdx]` (copy).
  2. Erase from `mUserList->mLines` every line equal to name (exact compare; direct vector erase, no
     RemoveLine; mLineColors and mSelectIdx untouched).
  3. If `app->mProfileMgr` (+0x1178): `mgr->DeleteProfile(name)` (0x4352e0: deletes the registry key
     `Users\<name>` and, only if that succeeds, the Profile from the list; section 8). Then if `name ==
     mgr->GetCurrentUser()`: `first = mLines[0]` (the first remaining name); if first is not empty:
     `mgr->SetCurrentUser(first)`.
  4. In all cases: `dlg = app->GetDialog(19)`; if dlg: `dlg->Resize(dlg->mX, dlg->mY, dlg->mWidth,
     dlg->GetPreferredHeight(dlg->mWidth))` (GetPreferredHeight and Resize both run UpdateList, so the
     select dialog shrinks and shows `GT_USER_LIST_CREATE` again).
  Note: no LoadCurrentProfile after switching the current user here either.

### 7. GetStringDialog (GetStringDialog.cpp, dialog 23)

A generic "enter a string" dialog: header, lines, a LabelWidget caption, an EditWidget drawn inside the
edit-box frame, optional icon. BookWorm's only use is the internet high-score alias ("YOUR NAME?"), reached
from FameWidget's internet-submit button (0x426c10, case 8) and from the app's 2024 handler: internet
high-score code, unreachable with NoInternetScores=True (to confirm with the fame group). Port optional.

| Off | Type | Name | Meaning / ctor | Read/written by |
|---|---|---|---|---|
| +0x110 | vfptr | EditListener | 0x4fcbb4 | |
| +0x114 | BookWormApp* | mApp | ctor arg 1 | EditWidgetText |
| +0x118 | EditWidget* | mEditWidget | `new EditWidget(0, this+0x110)` | app 2023 handler (mString) |
| +0x11c | LabelWidget* | mLabel | `new LabelWidget(theLabelText, theLabelFont)` | |
| +0x120 | Image* | mEditBoxImage | ctor arg (the creator passes IMAGE_EDITBOX) | Draw |
| +0x124 | Image* | mIconImage | 0; the creator sets IMAGE_IHS_ALIAS_WARNING | Draw, GetPreferredHeight |

Colour indices (Widget mColors, game-defined, 5..19, defaults int[15][4] at 0x52efd8 applied in the ctor):
5 (unused) white, 6 header black, 7 lines black, 8 footer black, 9 (unused) black, 10 (unused)
(0,0,0,128), 11 button text black, 12 button hilite (0,0,255), 13 (unused) (100,100,100), 14 edit bkg
(128,128,128), 15 edit outline black, 16 edit text black, 17 edit selection (0,128,0), 18 edit selected
text black, 19 label text black.

- **0x4292d0 `GetStringDialog(BookWormApp* theApp, int theId, bool isModal, Image* theComponentImage,
  Image* theButtonComponentImage, const string& theHeader, const string& theLines, const string&
  theFooter, Font* theHeaderFont, Font* theLinesFont, Font* theEditFont, const string& theLabelText, Font*
  theLabelFont, Image* theEditBoxImage, int theButtonMode)`**: `Dialog(compImg, btnImg, id, modal,
  header, lines, footer, mode)`; mApp, mEditWidget = mLabel = mIconImage = 0, mEditBoxImage; for idx
  5..19 `GetStringDialog::SetColor(idx, defaults[idx-5])` (non-virtual call; edit and label do not exist
  yet); `Dialog::SetColor(3, GetColor(11))`, `(0, GetColor(6))`, `(1, GetColor(7))`, `(2, GetColor(8))`;
  edit: `new EditWidget(0, this+0x110)`, `SetFont(theEditFont, NULL)`, colours 0..4 = mColors[14], [15],
  [16], [17], [18] (set in the order 0, 3, 4, 1, 2); label: `new LabelWidget(theLabelText,
  theLabelFont)` (`new(0x94)`, fame group's class), colour 0 = mColors[19]; `SetHeaderFont(theHeaderFont)`,
  `SetLinesFont(theLinesFont)`, `SetButtonFont(theLinesFont)`; yes and no buttons colour 0 = mColors[11],
  colour 1 = mColors[12] (assumes both buttons exist: mode 1 or 2 only).
- 0x428f50 dtor body: deletes mEditWidget, mLabel; Dialog body. 0x429590 slot 0.
- **0x428fd0 slot 7 `SetColor(int idx, const Color& c)`**: `Widget::SetColor(idx, c)` (store), then
  6/7/8 -> `Dialog::SetColor(0/1/2, c)`; 11 -> yes and no buttons colour 0; 12 -> their colour 1; 14..18
  -> mEditWidget colour 0..4 (if non-NULL); 19 -> mLabel colour 0 (if non-NULL); others: stored only.
- **0x4291d0 slot 50 `GetPreferredHeight(w)`** = `Dialog::GetPreferredHeight(w) + 70 + (mIconImage ?
  mIconImage->GetHeight() + 12 : 0)`.
- **0x429210 slot 13 `Resize(x, y, w, h)`**: remember the old X, Y; `Dialog::Resize`; move mLabel and
  mEditWidget by (x - oldX, y - oldY) keeping their sizes (the creator places them; see below).
- **0x429710 slot 16 `Draw(g)`**: `Dialog::Draw(g)`; if mIconImage and mEditWidget: `DrawImage(mIconImage,
  cL, edit->mY - edit->mHeight - Y - iconH - 12)`; if mEditBoxImage: rect = edit rect relative, grown
  (x-4, y-4, w+8, h+8), `DrawEditBox(g, rect)` (slot 52).
- **0x4295c0 slot 52 (new virtual) `DrawEditBox(Graphics*, const Rect&)`**: section 1.2 with mEditBoxImage.
- 0x429140 slot 1: `Dialog::AddedToManager`; AddWidget(mLabel) if any; AddWidget(mEditWidget) and
  SetFocus(mEditWidget) if any. 0x429190 slot 2: remove both.
- **0x4292a0 +0x110 `EditWidgetText`**: `mApp->ButtonDepress(mId + 2000)` (Enter = OK).
  **0x42e5f0 `AllowChar`**: letters and space only (shared with NewUserDialog). AllowKey/AllowText: true.
- slot 17 = 0x429290 (Dialog::Update); ButtonListener = Dialog's.
- The alias creator (app 0x414080, `ShowAliasDialog(const string& theName)`, cross-module):
  `new GetStringDialog(app, 23, true, IMAGE_DIALOG, IMAGE_DIALOGBUTTON, <a "your name" header>, <a request to choose
  an Internet High Scores name>, "", Verdana18BoldColor, Verdana12Bold,
  Verdana12Bold, <an alias field label>, Verdana10Bold, IMAGE_EDITBOX, 2)`; colours 5..19 from int[15][4] at
  0x52e728 through the virtual SetColor (header/lines/footer white, button text black, hilite white, edit
  bkg/outline (0,0,0,0), text (240,240,255), selection white, selected text black, label (255,173,17));
  `SetButtonFont(Verdana12Bold)`; insets (38,38,38,38); mIconImage = IMAGE_IHS_ALIAS_WARNING; `Resize(122,
  80, 400, prefH(400))`; label mJustify (+0x7c) = -1; `label->Resize(X+45, Y+H-135, W-96, 24)`; edit
  mMaxPixels 200, mMaxChars 22, `SetText(theName.empty() ? mgr->GetCurrentAlias() : theName)`;
  `edit->Resize(X+45, Y+H-110, W-96, 24)`; finally `Resize((app->mWidth-400)/2, (app->mHeight-prefH)/2,
  400, prefH)` (moves label and edit by the delta); `AddDialog(23)`. On 2023 the app trims " \t\r\n",
  and if not empty `mgr->SetCurrentAlias(alias)` (+ internet connect prompt 0x415a20); kills 23.

### 8. HintDialog (HintDialog.cpp, dialog 15)

Board tutorial hints (`GT_HINT_HOW_TO_PLAY_HEADER`, `GT_HINT_WATCH_OUT_HEADER`, ...) with a footer button `GT_HINT_CONTINUE`
and a second button `GT_HINT_DISABLE`.

| Off | Type | Name | Meaning / ctor | Read/written by |
|---|---|---|---|---|
| +0x110 | BookWormApp* | mApp | ctor arg | ButtonPress/Depress |
| +0x114 | DialogButton* | mDisableButton | id 26, label `GT_HINT_DISABLE` | Resize, Add/Removed |
| +0x118 | int | mHintId | ctor arg (the board's hint index) | never read |
| +0x11c | int | (unused) | 0 | never read |

- **Creator (app 0x4105d0, `ShowHint(int theHintId, header, lines, footer, int theButtonMode)`,
  called from Board::Update 0x40bd40 with mode 3 and footer `GT_HINT_CONTINUE`)**: if
  `!HintDialog::WasHintShown(theHintId)` (always false, below) and `app+0x12e8` (show hints):
  `KillDialog(15)`; `new HintDialog(app, theHintId, header, lines, footer, mode)`; `Resize(0, 0,
  app->mWidth, app->mHeight)`; `Resize(233, 110, 400, GetPreferredHeight(400))`; `AddDialog(15)`.
- **0x42af50 `HintDialog(BookWormApp* theApp, int theHintId, const string& theHeader, const string&
  theLines, const string& theFooter, int theButtonMode)`**: `Dialog(IMAGE_DIALOG, IMAGE_DIALOGBUTTON, 15,
  modal, header, lines, footer, mode)`; mApp; mHasAlpha = true; mHintId; +0x11c = 0;
  `SetHeaderFont(LockergnomeSmallColor)` (app+0x11e8), `SetLinesFont(Verdana12Bold)`,
  `SetButtonFont(Verdana12Bold)`; `SetColor(0, white)`, `SetColor(1, white)`, `SetColor(3, black)`; yes
  and no buttons (if any) colour 1 = white; `mDisableButton = new DialogButton(IMAGE_DIALOGBUTTON, 26,
  this+0x58)`, label `GT_HINT_DISABLE`, mDoFinger = true,
  `SetColors(gHintDisableButtonColors, 2)` (int[2][3] at 0x52f27c: black, white),
  `SetFont(mYesButton->mFont)` (duplicates the yes button's font; needs a yes button). Content insets
  stay 24.
- 0x42a820 dtor body: deletes mDisableButton; Dialog body. 0x42a990 slot 0.
- **0x42b160 slot 13 `Resize(x, y, w, h)`**: `mDialogHeader = Upper(mDialogHeader)` (every call; ASCII
  toupper); `Dialog::Resize`; if mDisableButton: `yes->Resize(yes.x + 12, yes.y - 4, yes.w - 24, yes.h)`;
  `mDisableButton->Resize(yes.x, yes.y, yes.w, yes.h)` (the adjusted rect); `yes->Move(yes.x, yes.y -
  mDisableButton->mHeight - 4)`. For (233, 110, 400, H): disable button (269, 110+H-59, 328, 31),
  continue button (269, 110+H-94, 328, 31).
- **0x42a8f0 slot 50 `GetPreferredHeight(w)`** = `Dialog::GetPreferredHeight(w) + 4 +
  (yes->mHeight < 33 ? 32 : yes->mHeight)` (= +36 with dialogbutton.gif).
- slot 16 Draw = 0x42e440 (Dialog::Draw: header uppercased in LockergnomeSmallColor, lines Verdana12Bold
  white; the footer is the button). slot 17 = Dialog::Update.
- 0x42a890 slot 1 / 0x42a8c0 slot 2: Dialog's, plus Add/RemoveWidget(mDisableButton).
- **0x42a920 +0x58 slot 2 `ButtonPress(int id, int clickCount)`**: `Dialog::ButtonPress(id)` then
  `mApp->PlaySample(0)`. Overriding slot 2 means every button press clicks here; the footer button (2015)
  also clicks through the app, so it plays "buttonclick" **twice** (two instances); the disable button
  once.
- **0x42a950 +0x58 slot 4 `ButtonDepress(int id)`**: `Dialog::ButtonDepress(id)` (2015 -> the app's
  default: KillDialog(15)); if id == 26: `app+0x12e8 = false` (show hints off, not saved to the registry
  here) and `app->KillDialog(15)`. (BookWormApp::ButtonDepress also handles id 26 the same way, but the
  disable button's listener is the dialog, so that app branch is unreachable from here.)
- **0x42b3e0 `static bool WasHintShown(int theHintId)`** (cdecl): ignores its argument: looks up key 0 in
  the global `std::map<int,bool> gHintShownMap` (0x534c38, static-init 0x42b480, atexit dtor 0x42b440);
  if found returns `gHintShownMap[0]`, else false. Nothing ever inserts into the map, so it **always
  returns false**: a hint shows whenever show hints is on. Port: a function returning false (or keep the
  map).

### 9. Player names and profiles (how the dialogs use the app's ProfileMgr and the players folder)

The dialogs never touch files. They call the profile manager `app->mProfileMgr` (app+0x1178, `new(0x2c)`
in BookWormApp::ReadFromRegistry 0x41a540, ctor 0x435b50; code in TitleScreen.cpp, screens group):
+0x0 BookWormApp*, +0x4 `std::list<Profile*> mProfiles` (head +0x8, size +0xc), +0x10 `std::string
mCurrentUser`. Profile (`new(0x38)`, ctor 0x434960): +0x0 `std::string mName`, +0x1c `std::string mAlias`
(= the name at creation). Calls used here (names proposed):

| Address | Call | Behaviour |
|---|---|---|
| 0x434800 | `Profile* FindProfile(const string& name)` | first profile whose mName matches **case-insensitively** (`_stricmp`) |
| 0x402090 | `string GetCurrentUser()` | copy of mCurrentUser (folded getter "string at +0x10") |
| 0x4356d0 | `bool SetCurrentUser(const string& name)` | for the first case-insensitive match: open `HKLM\SOFTWARE\<mRegKey>\Users` (KEY_WRITE), write REG_SZ value **"User"** = name (a 256-byte buffer, length 256), and only on success `mCurrentUser = name`. Callers pass a `Profile*` where a string is expected: Profile's first member is mName |
| 0x434e90 | `SaveProfile(Profile*)` | create/open `Users\<mName>`; values Alias (string, mAlias), ShowHelp (app+0x12e7), ShowHints (app+0x12e8), ShowDefs (app+0x12e9), TimesPlayed (app+0x12e0) as DWORDs |
| 0x4352e0 | `DeleteProfile(const string& name)` | open `Users` (KEY_READ); for each case-insensitive match `RegDeleteKey(Users, name)`; only if that succeeds delete the Profile and its list node |
| 0x435880 | `bool SetCurrentAlias(const string&)` | current profile's mAlias = arg; SaveProfile |
| 0x435910 | `string GetCurrentAlias()` | current profile's mAlias if not empty, else the current user name |
| 0x412100 | `BookWormApp::SaveCurrentProfile()` | `SaveProfile(FindProfile(GetCurrentUser()))` if a current user exists |
| 0x413a40 | `BookWormApp::LoadCurrentProfile()` | reads TimesPlayed, ShowDefs, ShowHints, ShowHelp of the current user into the app |

Registry layout (HKEY_LOCAL_MACHINE, mRegKey = "PopCap\\BookWorm"): `SOFTWARE\PopCap\BookWorm\Users` value
`User` (current player); one subkey per player named exactly as typed, values `Alias`, `ShowHelp`,
`ShowHints`, `ShowDefs`, `TimesPlayed`. At startup the manager enumerates the subkeys (RegEnumKeyEx order,
alphabetical on Windows) into mProfiles and reads `User`; a profile created in the session is appended at
the end of the list.

**The players folder.** Saved games live in `players\<name>\classic.dat` and `players\<name>\action.dat`
(BookWormApp 0x412190 builds `"players" + "\\" + name + ("\\" if name not empty) + file`; the board
group writes them; "players" is a per-TU static string in every module). The name is the profile's
mName with its typed case. Consequences for these dialogs: deleting a player removes only the registry
key; **players\<name>\ and its saved games stay**, and a new player with the same name (any case on
Windows) picks them up again. Port: the emulated registry plus `players/<name>/` under the user dir;
on a case-sensitive file system use the stored mName consistently (the original only ever builds the path
from mName / mCurrentUser, which carry the typed case).

**Name rules** (new player, and the alias):
1. Typing: EditWidget mMaxChars 22, mMaxPixels 200 (measured with Verdana12Bold), AllowChar = letters
   A-Z a-z and space only (so no digits, punctuation, '(' ')' - the special list lines can never be
   names - nor '\\', which would break the registry key and the folder path).
2. On OK (2022, or Enter): the app copies `mNameEdit->mString` and trims " \t\r\n" from both ends
   (0x4122e0: `resize(find_last_not_of + 1)`, then `erase(0, find_first_not_of)`); inner spaces are kept.
3. Empty -> `DoDialog(18, modal, `GT_ENTER_YOUR_NAME`, <0x4fbc00 `GT_ENTER_NAME_LINES`>, "OK", 3)`; the NEW USER dialog stays open.
4. `FindProfile(name)` (case-insensitive) finds one -> `DoDialog(18, modal, `GT_NAME_CONFLICT_HEADER`, `GT_NAME_CONFLICT_LINES`, "OK", 3)`; the dialog stays.
5. Otherwise (app 0x417080 case 2022): `KillDialog(22)`; game selector `SetDisabled(false)`; `p = new
   Profile(name)`; `app+0x12e0 (TimesPlayed) = 0`; `SaveProfile(p)` (the new player inherits the app's
   current ShowHelp/ShowHints/ShowDefs); `mProfiles.push_back(p)`; `SetCurrentUser(p)`; `KillDialog(19)`.
6. CANCEL (3022): if `GetCurrentUser()` is empty (first run, nobody to fall back to) the `GT_ENTER_YOUR_NAME`
   message appears and the dialog stays; else `KillDialog(22)`, `dlg19 = GetDialog(19)` and if it exists
   `Resize(dlg19->mX, dlg19->mY, 400, dlg19->GetPreferredHeight(400))` (refresh), game selector
   `SetDisabled(false)` (even while dialog 19 is still open).

At most five profiles can be created through the UI: `GT_USER_LIST_CREATE` is listed only while there are
fewer than five, and `GT_USER_LIST_DELETE` only when there are exactly five.

### 10. Dialog and button ids (what the app does with them)

From BookWormApp::ButtonDepress 0x417080 (app group) for the ids this group produces. The range test
uses `id % 10000`: below 2000 (other than 26) and 4000..9999 nothing happens; an id 2000+n or 3000+n not
listed below ends in `KillDialog(n)`.

| Id | Source | App action |
|---|---|---|
| 2001 / 3001 | Options footer `GT_OPTIONS_RETURN_TO_GAME` | `GetDialog(1)->ApplyAndClose()` (slot 52); the dialog kills itself after sliding out |
| 33 / 34 / 35 / 1036 | Options Help / Register / Credits / link | handled by OptionsDialog::ButtonDepress (dialog listener) |
| 2015 | Hint `GT_HINT_CONTINUE` | KillDialog(15) |
| 26 | Hint `GT_HINT_DISABLE` | HintDialog::ButtonDepress (dialog listener); app has a duplicate branch (top of 0x417080) |
| 2019 / 3019 | Select user OK / CANCEL | KillDialog(19) (OK: the dialog also switches the user) |
| 2020 | Delete user OK | KillDialog(21) (then the dialog opens 21) |
| 3020 | Delete user CANCEL | KillDialog(20) (the dialog refreshes 19) |
| 2021 | `GT_ARE_YOU_SURE` YES | KillDialog(21); `GetDialog(20)->DeleteSelectedUser()`; KillDialog(20) |
| 3021 | `GT_ARE_YOU_SURE` NO | KillDialog(21) |
| 2022 / 3022 | New user OK / CANCEL | section 9 |
| 2023 / 3023 | Alias OK / CANCEL | alias set (section 7) / KillDialog(23) |
| 2002 / 3002 / 0 | Register dialog | not ported (section 11) |

Widget ids inside dialogs: sliders 42 music, 43 sound; checkboxes 37 Full Screen, 38 Custom Cursors, 39
Show Hints, 40 Show Definitions, 41 Hardware Acceleration; edit boxes 0; lists 0.

### 11. RegisterDialog (reference only, not ported)

Dialog 2, `new(0x138)` by app 0x413cb0 (`OpenRegisterDialog`) only when `!app->mIsRegistered`; the
registered build instead shows `DoDialog(18, modal, "Already Registered", an "already registered" message
+ mProdName + ".", "OK", 3)`. Fields: +0x110 EditListener, +0x114 app, +0x118 name EditWidget (id 0),
+0x11c code EditWidget (id 1; both Verdana12Bold, 337x32, colours 0x52f6f8, tab chain to each other),
+0x120 "get a registration code" ButtonWidget (id 0, image button_reg_regcode, listener the dialog),
+0x124 cancel ButtonWidget (id 3002, button_reg_cancel, listener the app), +0x128 register ButtonWidget
(id 2002, button_reg_register, listener the app), +0x12c bool "switched to windowed", +0x130/+0x134 0.
Draw = `DrawImage(backdrop_register, 0, 0)` only; GetPreferredHeight = the backdrop height (356); the
creator centres it at 454x356. Resize moves the edits to (X+54, Y+178)/(X+54, Y+230), cancel (X+44,
Y+265), register (X+222, Y+265), code button (X+45, Y+100). Enter in the name box focuses the code box,
in the code box = 2002. ButtonDepress(0) leaves fullscreen if needed and opens the registration web page
(OpenRegisterPage); the dtor returns to fullscreen. The app validates on 2002 (Validate(name, code)),
an empty field reopens the web page. Port: drop; the Options Register button is never added in the
registered build, and anything else that opens it goes to the "Already Registered" message.

### 12. Cross-module references

App fields read/written (app group to reconcile; offsets absolute in BookWormApp):

| Field | Meaning (guess) | Used by |
|---|---|---|
| app+0x60 / +0x64 | mWidth / mHeight (640x480) | creators |
| app+0x306 / +0x307 | mIsWindowed / mForceFullscreen | Options |
| app+0x37c | mRegSource (string) | Options Register -> "options" |
| app+0x470 / +0x471 | mIsRegistered / mBuildUnlocked (via gSexyAppBase 0x56b8a8 in Options) | Options, Register |
| app+0x52b | mCustomCursorsEnabled | Options |
| app+0x1160 | mBoard (focus after the last dialog closes, pause on ModalOpen) | app |
| app+0x1168 | mGameSelector (BWGameSelector*): SetDisabled(true) while Select/NewUser are up | Select ctor/dtor, NewUser creator, app 2022/3022 |
| app+0x1170 | mFameWidget | app alias path |
| app+0x1178 | mProfileMgr | Select, Delete, app |
| app+0x11ac | Image** mImages (149) | all |
| app+0x11b0..0x11f8 | Font* mFonts[19] (table 0x52e3c0) | all |
| app+0x11fc / +0x1200 | checkbox checked / unchecked images | Options |
| app+0x12e0 | current player's TimesPlayed | new user = 0 |
| app+0x12e7 / +0x12e8 / +0x12e9 | ShowHelp / ShowHints / ShowDefs | Options, Hint, SaveProfile |

App functions called: PlaySample (slot 41), Get/SetMusicVolume, Get/SetSfxVolume (slots 43/44, 47/48),
SwitchScreenMode(bool,bool) (52), DoDialog (55, BookWormApp override 0x4103e0), GetDialog (56),
AddDialog (57), KillDialog (58, override 0x4110b0: afterwards focuses the board when no dialog is
left), ButtonDepress (4), OpenURL (26), GetInteger 0x480f30, GetString 0x480f70, Is3DAccelerated
0x47ddb0, Is3DAccelerationSupported 0x47e380, Is3DAccelerationRecommended 0x47e3a0, EnableCustomCursors
0x47d910; BookWormApp 0x4101c0 ShowHelp, 0x4106c0 ShowCredits, 0x413cb0 OpenRegisterDialog, 0x412100
SaveCurrentProfile, 0x413a40 LoadCurrentProfile, 0x4108c0 ShowNewUserDialog, 0x418590
ShowDeleteUserDialog; ProfileMgr functions (section 9); LabelWidget ctor 0x42b8c0 (fame group).
Framework: Dialog 0x43f3b0 / dtor body 0x43b1f0 / Draw 0x43d950 / Resize 0x43a620 / GetPreferredHeight
0x438ec0 / AddedToManager 0x43f200 / RemovedFromManager 0x43a980 / SetColor 0x43f190 / SetButtonFont
0x4384a0 / SetHeaderFont 0x4384d0 / SetLinesFont 0x438500 / ButtonPress 0x4386e0 / ButtonDepress
0x438720; DialogButton 0x43f240; EditWidget 0x43ef90; ListWidget 0x43ee70; Slider 0x43e550; Checkbox
0x43e890; ButtonWidget 0x43f080; WidgetManager AddWidget 0x43ebe0 / RemoveWidget 0x43a7a0 / SetFocus
0x4392c0; Graphics DrawImage(img,x,y,src) 0x44bad0, DrawImage(img,x,y) 0x44ba30, ClipRect 0x44bdb0,
SetFont, SetColor, SetColorizeImages, DrawString; Upper 0x46a2e0; StrFormat 0x46e5d0; CRT isalpha.

### 13. Files, registry, properties

- Files: none read or written by this group (saved-game files: section 9).
- Registry: indirectly through ProfileMgr (section 9) and the app's WriteToRegistry (volumes, screen
  mode, custom cursors).
- Properties (partner.xml): `NumOptionsLinks` (1 shipped; capped at 3), `OptionsLinkText%d` /
  `OptionsLinkURL%d` (i from 0), `OptionsDialogHeight` (315 shipped, default 330), `RegisterLabel`
  (default `GT_OPTIONS_REGISTER`).

### 14. Not ported / port notes

- **Options links** (partner.xml "more games"): not ported. Without them use NumOptionsLinks = 0, which
  gives the dialog height 315 (OptionsDialogHeight) and leaves every other position unchanged (section
  3.3). The 1036 / OpenURL branch goes away; under rundesk OpenURL must never fire anyway.
- **Register button**: created but never added or laid out in the registered build (mBuildUnlocked);
  keep the object or drop it with a `// not ported` note. RegisterDialog: not translated.
- **Hardware Acceleration** checkbox and its two warning dialogs: the port has Is3DAccelerated /
  Is3DAccelerationSupported / Recommended / SwitchScreenMode(w, 3d); keep the code path as is.
- **GetStringDialog**: only used by the internet high-score alias (NoInternetScores=True): translate for
  completeness or leave a stub; it is self-contained.
- Base class: every dialog here derives from `Dialog2004` (Compat2004.h) for the 2000/3000 ids and the
  forwarding to the app; sliders are `Slider2004`. Port-side Dialog differences that matter: the port's
  Dialog uses mButtonHeight (same value), port ListWidget/EditWidget/Checkbox member names match (mLines,
  mSelectIdx, mJustify, mDrawOutline, mMaxChars, mMaxPixels, mChecked); `EditWidget::SetText` in the
  port resets mLeftPos by default (harmless here).
- Leaks: SelectUserDialog and DeleteUserDialog never delete mUserList (fix with a `port:` note).

### 15. Open questions

1. SelectUserDialog: OK calls `LoadCurrentProfile()` (0x413a40) after switching the user, the
   double-click path and DeleteSelectedUser do not. Kept literal; confirm with the app group that nothing
   else (e.g. KillDialog(19) or the title screen) reloads the profile.
2. `WasHintShown` ignores its argument (key 0) and its map is never filled: original bug or an inlined
   remnant? Either way it always returns false.
3. MoneyDialog +0x118: inferred to be an unused double (Big Money's mLightPhase) from the alignment of the
   subclasses; no code touches +0x114..+0x11f.
4. Ghidra's GetPreferredHeight / Dialog font metrics: the concrete heights quoted (129, +36, 351...) use
   the font metrics from the .txt descriptors (image height as font height); the graphics group should
   confirm ImageFont GetHeight/GetAscentPadding/GetLineSpacing for Verdana18BoldColor and
   LockergnomeSmallColor.
5. NewUserDialog lines from BWGameSelector contain "\r\n\r\n": the 2004 WriteWordWrapped breaks only on
   '\n'; how a '\r' glyph measures/draws in the ImageFont (probably zero width) is for the graphics group.
6. OptionsDialog `SetColorizeImages(true)` around its labels: assumed to make no difference for ImageFont
   text (ImageFont colourises with the Graphics colour anyway); confirm with the graphics group.
7. ProfileMgr::DeleteProfile only removes the Profile when RegDeleteKey succeeds; in the emulated registry
   make deleting a missing key succeed, or a profile created without a key could never be deleted.

---

## Part 8. Cross-module names and decisions

The single source of truth for everything that crosses module boundaries in the BookWorm Deluxe port
(`games/bookworm/src/`). Read with `TRANSLATE_BRIEF.md` (who writes which file) and your own group's spec
(`<group>.md`, behaviour) and proposed header (`<group>.h`, module-internal layout).

### 0. Rules

1. **This file wins** over the seven analyses and their proposed headers for every class name, file, field,
   method, enum and constant used by more than one group. Module-internal names stay as in the group's header.
2. **Function names are the Ghidra export's** (`re/bookworm/gh/functions.jsonl`, files
   `decomp/<namespace>/<addr>_<name>.c`), which already merged the groups' symbol files; so every translator
   finds a function's decompilation under the name used here. The few places where an analysis text uses
   another name are listed in 2.12 and section 5.
3. **Field names are the owning group's header names** unless section 2 says otherwise. Every field read or
   written by another module is listed in section 2 with its canonical name.
4. **Shared constants are defined once** (section 4): resource, dialog, music and game-mode ids only in `Res.h`
   (app); board enums in `Board.h`; Lex animations in `Lex.h`; effect and ember types in `Effect.h`. A group
   header must not redefine them (a second enumerator with the same name in `namespace Sexy` does not compile):
   delete the local copies the proposed headers carry (`board_logic.h` sounds, `board_draw.h` images, fonts,
   `TileSet`, `PieceType`, `gLetterValues`, `screens.h` images/fonts/sounds/dialogs, `fame.h` `FameImageIdx`,
   `FameFontIdx`, `HighScoreEntry`, `lex.h` `LexImageId`, `dialogs.h` `BookWormDialogId`, the class-scoped
   `GAMEMODE_*` in `BWGameSelector`).
5. Port types in signatures: `SexyString` (= `std::string`), `Graphics*`, `Image*`, `MemoryImage*`, `DDImage*`,
   `Font*` / `ImageFont*`, `Color`, `Rect`, `Point`, `Insets`, `MTRand`, `Buffer`, `FILE*` for the saved-game
   records. Resources are reached directly: `mApp->mImages[IMAGE_X]`, `mApp->mFonts[FONT_X]`.
6. Offsets (`+0xNN`) are the original's, for the `// +0xNN` comments only. Never compute with them.

### 1. Classes and file ownership

#### 1.1 Every game class

"Port base" is what the translation derives from. vftables: primary (slot count), then the secondary tables at
their offsets. "Alloc" is the `operator new` site(s).

| Class | Group: files | Port base | sizeof | vftable(s) | Ctor (alloc) |
|---|---|---|---|---|---|
| BookWormApp | app: BookWormApp.h/.cpp | SexyApp | 0x1348 | 0x4fbda4 (68: SexyApp's 67 + 67 TitleScreenDone) | 0x41a0f0 (WinMain 0x40fe80) |
| Math | app: BookWormApp.h/.cpp (not Math.h, see D19) | none | 0xb48 | 0x50074c (1) | 0x42e330 (embedded in the app at +0x618) |
| HighScoreEntry (+ HighScoreByScore 0x4115d0, HighScoreByWordLength 0x411570) | app: BookWormApp.h | none (struct) | 0x40 | none | copy 0x412080 |
| UserManager | app: UserManager.h/.cpp | none | 0x2c | none | 0x435b50 (ReadFromRegistry 0x41a540) |
| User | app: UserManager.h | none (struct) | 0x38 | none | 0x434960 (LoadUser 0x435470, ButtonDepress 2022) |
| Board | board_logic: Board.h, Board.cpp; board_draw: BoardDraw.cpp (Draw and helpers) | Widget, ButtonListener | 0x3f0 | 0x4f9b7c (47); +0x58 0x4f9b60 | 0x40ae90 (StartGame 0x410d10) |
| Piece | board_logic: Piece.h (methods in Board.cpp) | none | 0xf0 | 0x4f9ac8 (1) | 0x403f20 (NewBoard, Refill, Piece/FlyingPieceList Load) |
| WordScore | board_logic: WordScore.h/.cpp | none | 0x48 | 0x4f9b1c (1) | 0x405190 (embedded twice in Board) |
| StatKeeper | board_logic: StatKeeper.h/.cpp | none | 0x78 | 0x4f9b14 (1) | 0x408690 (Board ctor, twice) |
| Effect | board_draw: Effect.h/.cpp | none | 0x10 | 0x4fc450 (8) | 0x41e880 (base only) |
| Sparkle | board_draw: Effect.h/.cpp | Effect | 0x48 | 0x4fc474 (8) | 0x41f2b0 (Board::Update, FlyingPieceList::Update, EffectMgr::Load) |
| FlyingPieceList | board_draw: Effect.h/.cpp | Effect | 0x20 | 0x4fc4a0 (8) | 0x420920 (Board::Submit, EffectMgr::Load) |
| EffectMgr | board_draw: Effect.h/.cpp | none | 0x1c | 0x4fc498 (1) | 0x4201c0 (Board ctor, OverlayWidget ctor) |
| Ember | board_draw: Effect.h/.cpp | none (struct) | 0x60 | none | 0x41f300 (EmberListMgr pool) |
| EmberListMgr | board_draw: Effect.h/.cpp | none | 0x2c | 0x4fc4c4 (1) | 0x421070 (Board ctor, pool 1000) |
| Points | board_draw: Points.h/.cpp | Widget | 0x84 | 0x500e0c (47) | 0x431a50 (Board 0x409970, 0x409b70) |
| Lex | lex: Lex.h/.cpp | Widget | 0xd0 | 0x5005b4 (49) | 0x42d560 (Board::AddedToManager, Board::LoadGame) |
| BigLex | lex: BigLex.h/.cpp | Widget | 0x70 | 0x4f9724 (47) | 0x402df0 (BWGameSelector ctor) |
| WordBalloon | lex: WordBalloon.h/.cpp | Widget | 0x120 | 0x5016c4 (52) | 0x4372a0 (OverlayWidget::AddedToManager) |
| Definitionator | lex: Definitionator.h/.cpp | none | 0x14 | 0x4fc24c (2) | 0x41dd30 (LoadingThreadProc) |
| FameWidget | fame: FameWidget.h/.cpp | Widget, ButtonListener, CheckboxListener | 0x2a0 | 0x4fc8dc (53); +0x58 0x4fc8c0; +0x5c 0x4fc8b8 | 0x428520 (ShowHallOfFame) |
| LevelUpWidget | fame: LevelUpWidget.h/.cpp | Widget, ButtonListener | 0xe0 | 0x4fd894 (47); +0x58 0x4fd878 | 0x42bd50 (Board::ShowSummary) |
| LabelWidget | fame: LabelWidget.h/.cpp (not ported, D38) | Widget | 0x94 | 0x4fd5ec (47) | 0x42b8c0 (GetStringDialog ctor only) |
| TitleScreen | screens: TitleScreen.h/.cpp | Widget, ButtonListener | 0xc8 | 0x5011fc (49); +0x58 0x5011dc | 0x434380 (Init 0x414a10) |
| BWGameSelector | screens: BWGameSelector.h/.cpp | Widget, ButtonListener | 0xb0 | 0x4f93e4 (50); +0x58 0x4f93c8 | 0x401600 (ShowGameSelector) |
| HelpWidget | screens: HelpWidget.h/.cpp | Widget, ButtonListener, CheckboxListener | 0xa0 | 0x4fcd24 (47); +0x58 0x4fcd04; +0x5c 0x4fccfc | 0x429cd0 (ShowHelp) |
| CreditsDialog | screens: CreditsDialog.h/.cpp | **Dialog2004** | 0x138 | 0x4fc07c (52); +0x58 0x4fc060 | 0x41c0b0 (ShowCredits) |
| OverlayWidget | screens: OverlayWidget.h/.cpp | Widget | 0x90 | 0x500cbc (47) | 0x430c20 (Board::AddedToManager) |
| ColorizedButton | screens: ColorizedButton.h/.cpp | ButtonWidget | 0xf0 | 0x4fbf7c (53) | 0x41bf50 (LevelUpWidget::AddedToManager) |
| ShadowFont | screens: ShadowFont.h/.cpp | ImageFont | 0x60 | 0x50117c (23) | 0x433250 (TitleScreen ctor), copy 0x433200 (Duplicate) |
| ImagelessButton | screens: not ported (D38) | ButtonWidget | 0xe8 | 0x4fd524 (49) | 0x42b500 (FameWidget ctor only: hidden RESUBMIT) |
| WaitDialog | screens: not ported (D38) | Dialog2004 | 0x120 | 0x5015cc (52); +0x58 0x5015b0 | 0x436110 (FameWidget::ShowWaitDialog only) |
| MoneyDialog | dialogs: MoneyDialog.h/.cpp | **Dialog2004** | 0x120 (inferred) | 0x500774 (52); +0x58 0x500758 | 0x42e3d0 (base only) |
| OptionsDialog | dialogs: OptionsDialog.h/.cpp | MoneyDialog, SliderListener, CheckboxListener; sliders are **Slider2004** | 0x188 | 0x500b4c (53); +0x58 0x500b2c; +0x120 0x500b24; +0x124 0x500b1c | 0x42fe90 (ShowOptions) |
| NewUserDialog | dialogs: NewUserDialog.h/.cpp | MoneyDialog, EditListener | 0x130 | 0x50087c (52); +0x58 0x50085c; +0x120 0x500848 | 0x42e7c0 (ShowNewUserDialog) |
| SelectUserDialog | dialogs: SelectUserDialog.h/.cpp | MoneyDialog, ListListener | 0x130 | 0x501004 (52); +0x58 0x500fe8; +0x120 0x500fe0 | 0x432c90 (ShowSelectUserDialog) |
| DeleteUserDialog | dialogs: DeleteUserDialog.h/.cpp | MoneyDialog, ListListener | 0x130 | 0x4fc2b4 (52); +0x58 0x4fc298; +0x120 0x4fc290 | 0x41e370 (ShowDeleteUserDialog) |
| HintDialog | dialogs: HintDialog.h/.cpp | **Dialog2004** | 0x120 | 0x4fd434 (52); +0x58 0x4fd414 | 0x42af50 (ShowHint) |
| GetStringDialog | dialogs: not ported (D38) | Dialog2004, EditListener | 0x128 | 0x4fcbe4 (53); +0x58 0x4fcbc8; +0x110 0x4fcbb4 | 0x4292d0 (ShowAliasDialog only) |
| RegisterDialog | dialogs: not ported | Dialog2004, EditListener | 0x138 | 0x500efc (52); +0x58 0x500ee0; +0x110 0x500ecc | 0x431e80 (ShowRegisterDialog) |
| UpdateCheckDialog | framework class, not ported | (Dialog) | | 0x501b1c (52); +0x58 0x501b00 | DoUpdateCheck 0x415de0 |

Every game dialog derives from `Dialog2004` (`Compat2004.h`), never from `Dialog` (screens.h's CreditsDialog and
WaitDialog say `Dialog`: wrong). Every slider is a `Slider2004`.

#### 1.2 Code found outside the module the inventory gives it

| Address(es) | Function(s) | Inventory range | Owner and file |
|---|---|---|---|
| 0x402090 | `UserManager::GetCurrentUserName` (inline) | BWGameSelector.cpp | app: UserManager.h (inline) |
| 0x402f60, 0x402fd0, data 0x52e278 | `Board::IsAdjacent` (static), `Board::SetActionSpeed` (static), `gActionSpeedValue` | BigLex.cpp | board_logic: Board.h/.cpp (lex does **not** translate them) |
| 0x403ab0, 0x404340 | `Lex::IsMoving`, `Lex::IsBusy` | Board.cpp | lex: inline in Lex.h |
| 0x40fe80, 0x40ffa0, 0x410090 | `WinMain`, `BookWormApp::ShutdownWidgets`, `DrawShadowedString` | Board.cpp | app: BookWormApp.cpp (WinMain's body: see D17; `main.cpp` stays the lead's) |
| 0x41d2b0 | `Definitionator::GetDefinition` | CreditsDialog.cpp | lex: Definitionator.cpp |
| 0x4216e0 | `MapTableIndex` | Effect.cpp | board_draw; only FameWidget's internet code calls it: may be omitted |
| 0x42b980, 0x42b9c0 | `LevelUpWidget::DeletePanel`, `CreatePanel` | LabelWidget.cpp | fame: LevelUpWidget.cpp |
| 0x430be0 | `Rect::Contains(const Point&)` (inline) | OverlayWidget.cpp | framework (port `Rect::Contains`) |
| 0x434800-0x435e20 | `UserManager`, `User` | TitleScreen.cpp | app: UserManager.h/.cpp |
| 0x436260, 0x436310 | `WordBalloon::Init`, `WordBalloon::SetJustification` | WaitDialog.cpp | lex: WordBalloon.cpp |
| 0x437540-0x4377ff | WidgetManager / Graphics::operator= | WordBalloon.cpp | framework |

#### 1.3 Not ported

| Class / code | Why |
|---|---|
| RegisterDialog | registration (registered build: partner.xml NoReg) |
| UpdateCheckDialog (framework), dialogs 7, 8, 10008, 20008 | update check (DontUpdate) |
| WaitDialog | only `FameWidget::ShowWaitDialog` 0x423fd0 (internet scores) creates it |
| ImagelessButton | only FameWidget's never-visible RESUBMIT button (ctor 0x428520) creates it; FameWidget drops `mResubmitButton` |
| GetStringDialog, LabelWidget | only `ShowAliasDialog` 0x414080 (internet alias; callers ButtonDepress 2024 and FameWidget::ButtonDepress case 8, both internet) creates them (open question 1) |
| FameWidget internet members and functions, `MapTableIndex`, `StringToLowerInPlace` | internet scores (NoInternetScores) |
| HTTPTransfer, InternetManager, BetaSupport | framework, internet/beta |

#### 1.4 Globals

| Global | Original | Owner: declared in / defined in | Users |
|---|---|---|---|
| `MTRand* gGameRand` | 0x5347d0 | app: BookWormApp.h / BookWormApp.cpp (created at the end of `BookWormApp::Init`, D17) | Board (letters, fire, specials, bonus word) |
| `BookWormApp* gApp` | 0x5347d4 | app (only WinMain used it; the port may drop it) | none |
| `const int gLetterValues[26]` | 0x4f9888 (copy 0x4fa480 in the app) | board_logic: Board.h / Board.cpp | WordScore::CalcScore, BoardDraw::DrawPiece, app tile-set builder (D21) |
| `int gLastLetterIdx`, `gLastRefillLevel`, `gLivePieceCount`, `double gActionSpeedValue`, tables `gLevelThresholds` ... | 0x52e2b0, 0x52e3a0, 0x534720, 0x52e278, 2.5 of Part 2 | board_logic: Board.cpp | Board only |
| `"players"` directory string | per-TU statics (0x52e818 app, 0x52e2b4 Board, ...) | each TU its own `static` | app HasSavedGame, Board Save/Load/Delete (same path, see open question 4) |
| `gImageNames[149]`, `gFontNames[19]`, `gSoundNames[47]` | 0x52e410, 0x52e3c0, 0x52e668 | app: Res.h / BookWormApp.cpp | app loaders |
| `gLexAnims[28][50]`, `gBigLexAnims` | 0x4fd9f0, 0x4f9590 | lex: Lex.h / BigLex.h | Lex, BigLex |
| `gRankNames[32]` | 0x52f368 | fame: LevelUpWidget.cpp | LevelUpWidget |
| `gHintShownMap` | 0x534c38 | dialogs: HintDialog.cpp (or drop, D2) | HintDialog::WasHintShown |
| Free functions `DrawShadowedString` 0x410090, `TruncateToWidth` 0x412da0, `TrimString` 0x4122e0, `UpperCaseString` 0x4112f0, `OptimizeImage` 0x411220 | | app: BookWormApp.h / .cpp | FameWidget (the first two) |

### 2. Cross-module fields

Columns: original offset, type, canonical name, meaning, the modules outside the owner that use it (r = read,
w = write). Widget base fields (mX, mY, mWidth, mHeight, mVisible, mDisabled, mUpdateCnt, mIsOver) keep the port's
names.

#### 2.1 BookWormApp (app)

| Off | Type | Name | Meaning | Used outside the app |
|---|---|---|---|---|
| +0x618 | Math | mMath | sine tables, never read | none |
| +0x1160 | Board* | mBoard | current board; after a game it is unparented under the selector until `RemoveBoard` | everyone (r) |
| +0x1164 | TitleScreen* | mTitleScreen | port: initialise NULL | none |
| +0x1168 | BWGameSelector* | mGameSelector | main menu or NULL | FameWidget::ButtonDepress (r: NULL test), SelectUserDialog ctor/dtor (SetDisabled) |
| +0x116c | HelpWidget* | mHelpWidget | help screen or NULL | Board UpdatePauseFade, ShowSummary, SaveGame (r) |
| +0x1170 | FameWidget* | mFameWidget | hall of fame or NULL | none in the ported code |
| +0x1174 | Definitionator* | mDefinitionator | popdefs.txt, NULL until loaded | Board::Submit (r) |
| +0x1178 | UserManager* | mUserManager | profiles; port: initialise NULL | Board Save/Load/Delete, Lex::Update, BWGameSelector, HelpWidget, Select/DeleteUserDialog (r) |
| +0x117c | HighScoreByScoreSet | mClassicScores | top 10 classic | FameWidget (r; clears mIsNew) |
| +0x1188 | HighScoreByScoreSet | mActionScores | top 10 action | FameWidget |
| +0x1194 | HighScoreByScoreSet | mBestWords | top 3 best words | FameWidget |
| +0x11a0 | HighScoreByLengthSet | mLongestWords | top 3 longest words | FameWidget |
| +0x11ac | Image** | mImages | `new Image*[NUM_IMAGES]`; `mImages[IMAGE_RANKSTRIP]` is NULL after loading | everyone (r) |
| +0x11b0 | ImageFont*[NUM_FONTS] | mFonts | font table | everyone (r) |
| +0x11fc | Image* | mCheckboxOnImage | right half of IMAGE_CHECKBOXES = checked | OptionsDialog (r) |
| +0x1200 | Image* | mCheckboxOffImage | left half = unchecked | OptionsDialog (r) |
| +0x1204 | MemoryImage*[NUM_TILESETS] | mTileSetImages | 26 pre-drawn letter tiles per TileSetId | BoardDraw (DrawPiece), FlyingPieceList::Draw (r) |
| +0x1244 | MemoryImage* | mTileSapphireImage | copy of IMAGE_TILE_SAPPHIRE | FlyingPieceList::Draw |
| +0x1248 | MemoryImage* | mTileDiamondImage | copy of IMAGE_TILE_DIAMOND | FlyingPieceList::Draw |
| +0x124c | MemoryImage* | mRankStripImage | copy of rankstrip.jpg | LevelUpWidget::DrawPanel |
| +0x1250 | bool | mInitDone | end of Init | TitleScreen (trial branch only) |
| +0x1254 | int | mGameMode | GameMode of the current/last game | Board (r; LoadGame w), BoardDraw::DrawPieces, LevelUpWidget::ButtonDepress, BWGameSelector::StartGame (w) |
| +0x1258 | int | mSelectedGameMode | mode picked in the selector (ctor -1) | none (board_logic's "mode to load" is the app passing it to LoadGame) |
| +0x125c | int | mScoringMethod | 1; 0 would select the dead CalcScoreByLength | WordScore::Build (r) |
| +0x1260 | std::set<SexyString> | mDictionary | wordlist.txt, upper case | Board (WordScore::CheckValid) |
| +0x126c | std::vector<SexyString> | mBonusWords | bonus_word_dictionary.txt (board_logic cites +0x1270, the vector's first pointer) | Board::PickBonusWord |
| +0x127c | std::set<SexyString> | mUsedBonusWords | bonus words completed; written, never read | Board::BonusWordScored (w) |
| +0x12c0 | SexyString | mBonusWord | next bonus word (PickBonusWord); the board copies it to `Board::mBonusWord` | Board (r/w) |
| +0x12dc | int | mGameOption | ctor 1; only copied to/from the selector; meaning unknown | BWGameSelector (r/w) |
| +0x12e0 | int | mUserTimesPlayed | per-user "TimesPlayed" | Board Submit/Scramble (++), BWGameSelector::DrawBubble, UserManager::SaveUser |
| +0x12e4 | bool | mScrambled | set by every Scramble, never read | Board::Scramble (w) |
| +0x12e5 | bool | mWelcomePlayed | lex_welcome played once per run | BWGameSelector::Update |
| +0x12e6 | bool | mUnused12e6 | ctor true, no reader | none |
| +0x12e7 | bool | mShowHelp | option: help at game start | Board Save/Load, HelpWidget, UserManager::SaveUser |
| +0x12e8 | bool | mShowHints | option: hints | Board Save/Load, OptionsDialog, HintDialog (id 26), UserManager |
| +0x12e9 | bool | mShowDefs | option: definitions | Board::Submit, OptionsDialog, UserManager |
| +0x12ea | bool | mInternetOK | not ported | none |
| +0x12eb | bool | mUnused12eb | no reader | none |
| +0x12ec | bool | mWatchOutHintShown | HINT_WATCH_OUT was scheduled (once per run) | Board::Update (r/w) |
| +0x12ed | bool | mFireAlertHintShown | HINT_FIRE_ALERT | Board::Refill (r/w) |
| +0x12ee | bool | mGreenTileHintShown | HINT_GREEN_TILE | Board::Refill (r/w) |
| +0x12ef | bool | mUnused12ef | no reader | none |
| +0x12f0 | bool | mScrambleHintShown | HINT_SCRAMBLE | Board::Scramble (r/w) |
| +0x12f1 | bool[11] | mUnusedHintFlags | ctor false, no reader | none |
| +0x12fc | std::vector<int> | mPausedSongs | PauseAllMusic | none |
| +0x130c | int | **mResumeUpdateCnt** (app.h: mLastInputUpdateCnt) | mUpdateCount when the board was last resumed; board input is ignored for 15 updates after (D10) | Board::MouseDown (w on unpause; r), MouseDrag, MouseUp (r) |
| +0x1310 | SexyString | mSEHInfo | crash report, not ported | none |
| +0x132c | int | mPlayTicks | ++ per Board::Update in play | Board::Update (w) |
| +0x1330 | bool | mLevelReached | a level summary was shown this game | Board::ShowSummary (w) |
| +0x1334 | int | mPlaysToCount | trial | none |
| +0x133c | int | mMaxPlays | property MaxPlays (0) | Board Submit/Scramble (r) |
| +0x1340 | int | mMaxTime | property MaxTime (60) | Board Submit/Scramble (r), TitleScreen (trial) |

Framework (port SexyApp / SexyAppBase) members the game modules use, with the port's names: mWidth, mHeight,
mWidgetManager, mMusicInterface, mProdName, mProductVersion, mBuildNum, mBuildDate, mRegKey, mUserName, mRegCode,
mRegSource, mIsRegistered, mBuildUnlocked, mTimesPlayed, mTimedOut, mLoaded, mUpdateCount, mHasFocus, mIsWindowed,
mForceFullscreen, mCustomCursorsEnabled, mDDInterface; methods PlaySample, SafeDeleteWidget, GetDialog, AddDialog,
KillDialog, DoDialog, GetDialogCount, CopyImage, FlipImage, GetString/GetInteger/GetBoolean,
Get/SetMusicVolume, Get/SetSfxVolume, SwitchScreenMode(windowed, is3d), EnableCustomCursors,
Is3DAccelerated/Supported/Recommended, OpenURL, Shutdown, GetLoadingThreadProgress.

#### 2.2 Board (board_logic)

| Off | Type | Name | Meaning | Used outside board_logic |
|---|---|---|---|---|
| +0x4c | int | mUpdateCnt (Widget) | animation clock | BoardDraw |
| +0x5c | BookWormApp* | mApp | | BoardDraw |
| +0x6c | MTRand* | mRand | board RNG (particles, drop heights, Lex chatter) | none (seeding order: 3.11) |
| +0x70 | EffectMgr* | mEffectMgr | board's effects (sparkles) | BoardDraw (Draw) |
| +0x74 | EmberListMgr* | mEmberMgr (board_draw: mEmberListMgr) | particles | BoardDraw (Draw), FlyingPieceList::Update (NewEmber) |
| +0x78 | Piece*[BOARD_ROWS][BOARD_COLS] | mBoard | **[row][col]**; top tile of column c = `mBoard[c % 2 == 0 ? 1 : 0][c]` | BoardDraw, Lex::MouseEnter (the 7 top tiles, columns 0..6 in order) |
| +0x158 | OverlayWidget* | mOverlay | top layer, created in AddedToManager | Lex (`mOverlay->mBalloon`), LevelUpWidget::Update |
| +0x15c | LevelUpWidget* | mLevelUpWidget | summary panel or NULL | app ShowHelp/HideHelp |
| +0x160 | Buffer | mGameLog | event log | FameWidget ctor (`GetDataLen()`, internet only) |
| +0x188 | int | mLevelSeconds | seconds of the level (set by ShowSummary) | app GetLevelTimeString |
| +0x18c | int | mTotalSeconds | | app GetTotalTimeString |
| +0x194 | int | mState | BoardState | app (r 2; w BOARD_INACTIVE in StartGame, BOARD_PLAYING on 3005), LevelUpWidget (r 2/5, w BOARD_NEXT_LEVEL), OverlayWidget::Draw (r 0), BoardDraw (r 7) |
| +0x19c | std::list<Piece*> | mSelection (board_draw: mSelected) | selected tiles in order (list head +0x1a0, size +0x1a4) | BoardDraw |
| +0x1b1 | bool | **mBurnedDown** (board_draw/screens: mScorched, lex: mBurnt) | the board burned at game over | BoardDraw, Lex::Draw/Update, OverlayWidget::Draw |
| +0x1b3 | bool | mPiecesFlying | set when a flying tile moved | FlyingPieceList::Update (w true) |
| +0x1c0 | double | mScrambleProgress (board_draw: mSpinTime) | 0..1 in BOARD_SCRAMBLING | BoardDraw |
| +0x1c8 | double | mLevelProgress (board_draw: mProgressTarget) | | BoardDraw |
| +0x1d0 | double | mShownProgress (board_draw: mProgressShown) | | BoardDraw |
| +0x1d8 | WordScore | mWord | current selection's word | OverlayWidget::Draw (`!mWord.mWord.empty()`, `mWord.CommonPrefix(s)`) |
| +0x220 | WordScore | mLastWord (board_draw: mWordScore) | word shown above the board | BoardDraw::DrawWord (copies it) |
| +0x268 | Color | mLastWordColor (board_draw: mWordColor) | rgb and alpha (+0x274) written by DrawWord | BoardDraw (w) |
| +0x278 | double | mLastWordAlpha (board_draw: mWordFade) | | BoardDraw |
| +0x280 | double | mScoreGlow | | BoardDraw |
| +0x28c | SexyString | mBonusWord | word on the banner, "" when none | OverlayWidget Update/Draw |
| +0x2a8 | StatKeeper* | mGameStats | whole game | Lex::Update (`mScrambles`), FameWidget ctor (`mScore`), LevelUpWidget::DrawPanel, app SubmitHighScores (argument) |
| +0x2ac | StatKeeper* | mLevelStats | current level | LevelUpWidget::DrawPanel |
| +0x308 | double | mBoardBurnAlpha | 1.0 when the board burns, -0.01 per update | OverlayWidget::Draw (additive white flash) |
| +0x320 | Piece* | mFireLinePiece | bottom fire tile under the fire column | OverlayWidget::Draw |
| +0x328 | double | mFireLineAlpha | 0..1; the column width fraction | OverlayWidget::Draw |
| +0x330 | int | mFireLineScroll | scroll into IMAGE_FX_FIRELINE | OverlayWidget::Draw |
| +0x334 | int | mFireTileCount | fire tiles on the board | Lex::MouseEnter (< 10) |
| +0x34c | int | mHintId | BoardHintId | passed to app ShowHint |
| +0x350 | Lex* | mLex | | FlyingPieceList, LevelUpWidget, WordBalloon (NULL test), OverlayWidget, BoardDraw |
| +0x357 | bool | mNewHighScore | the game entered a high-score table | app SubmitHighScores (w true), LevelUpWidget::Update (w false) |
| +0x365 | bool | mBurping (board_draw: mBurpSmoke) | burp smoke visible | BoardDraw |
| +0x368 | int | mBurpDelay (board_draw: mBurpCountdown) | | FlyingPieceList::Update (w 50) |
| +0x370 | double | mBurpAlpha (board_draw: mBurpSmokeTime) | | BoardDraw |
| +0x378 | int | mHighlightKey (board_draw: mHeldKey) | | BoardDraw |
| +0x39c | int | mShownScore (board_draw: mDisplayedScore) | | BoardDraw |
| +0x3c0 | bool | **mSummaryShown** (fame: mLevelUpShown, lex: "paused") | the LevelUpWidget is up | app HideHelp, WordBalloon::Update (stops its timer), OverlayWidget::Update (no bonus balloon) |
| +0x3c4 | int | mLevel | 0-based | LevelUpWidget, BoardDraw, WordScore::CalcScore |
| +0x3c8 | int | mPauseCount | > 0 paused (counted) | app GotFocus/LostFocus, Lex (Update, Mouse*), WordBalloon::Update, OverlayWidget::Update |
| +0x3d0 | double | mPauseFade | 0..1 | BoardDraw, OverlayWidget::Draw |
| +0x3dc | bool | mPauseNoDialogs (screens: "paused") | while paused: no dialog and no help widget, so the pause box is drawn | OverlayWidget::Draw |

No other module reads +0x1b2, +0x2b0, +0x2cc, +0x2d0, +0x2ec, +0x3d8, +0x3e8, +0x3ec (saved only).

#### 2.3 Piece (board_logic, Piece.h)

| Off | Type | Name | Meaning | Used outside board_logic |
|---|---|---|---|---|
| +0x08 / +0x10 | double | mX / mY | top-left on screen | BoardDraw, FlyingPieceList |
| +0x18 | double | mFlySpeed (board_draw: fx) | FlyingPieceList: step factor of **both** x and y (+0.003 after the x step), 0 on arrival (D4) | FlyingPieceList |
| +0x20 | double | mVY (board_draw: fy) | fall velocity; FlyingPieceList adds 0.003 per y step and zeroes it on arrival, never multiplies by it | FlyingPieceList |
| +0x30 / +0x34 | int | mCol / mRow | | BoardDraw, OverlayWidget::Draw (mFireLinePiece) |
| +0xa4 | char | mLetter | 'A'..'Z' ('Q' = "Qu") | BoardDraw, FlyingPieceList::Draw |
| +0xa5 | char | mNextLetter | letter after a flip | BoardDraw |
| +0xa6 | bool | mSelected | | BoardDraw |
| +0xa8 | double | mScale | flight shrink | FlyingPieceList |
| +0xb0 | bool | mBurning (board_draw: unknown) | sinking away | BoardDraw (excluded from glow lists) |
| +0xbc | bool | mPendingFire | action mode: turns into fire soon | BoardDraw (fire glow) |
| +0xd8 | bool | mFlipping (board_draw: spinning) | letter flip | BoardDraw |
| +0xe0 | double | mFlipProgress (board_draw: spin time) | 0..1 | BoardDraw |
| +0xe8 | int | mType | TileType | BoardDraw, FlyingPieceList, Lex::MouseEnter (!= TILE_FIRE) |
| +0xec | int | mBurnCountdown | the tile below a fire tile gets the fire glow when >= 1 | BoardDraw |

The other fields (+0x28..+0xa0, +0xc0..+0xd0) keep board_logic.h's names; no module reads them (saved only).
Methods used outside: ctor 0x403f20, `virtual ~Piece()`, `Save(FILE*)` 0x403050 / `Load(FILE*)` 0x403370
(FlyingPieceList::Save/Load write the same 191-byte record).

#### 2.4 WordScore and StatKeeper (board_logic)

WordScore: +0x04 `BookWormApp* mApp`, +0x08 `SexyString mWord` (QU tiles add "QU"), +0x24 `bool mValid`,
+0x28 `int mTypeCount[NUM_TILE_TYPES]` (indexed by TileType), +0x40 `int mNumPieces`, +0x44 `int mScore`. Read by
BoardDraw (copy of mLastWord) and OverlayWidget (board's mWord).

StatKeeper, fields read outside board_logic: +0x08 `int mScore` (FameWidget, app), +0x30 `int mScrambles` (Lex hint
level), +0x38 `SexyString mBestWord` (LevelUpWidget, app), +0x54 `int mBestWordScore` (LevelUpWidget, app),
+0x58 `SexyString mLongestWord` (LevelUpWidget uses its size, app), +0x74 `int mLongestWordScore` (app).
+0x24 `mSapphireTilesUsed` and +0x28 `mDiamondTilesUsed` are never shown by any module: port zero-initialises.

#### 2.5 Lex (lex, Lex.h)

| Off | Type | Name | Meaning | Used outside lex |
|---|---|---|---|---|
| +0x80 / +0x84 | int | mTargetX / mTargetY (board_draw: home x/y) | where Lex slides to; `Move` sets them to the position | Board (= 60/210 after creation), LevelUpWidget (-= 26/-= 6 on add, = 60/210 on remove and on continue), BoardDraw (r, shadow) |
| +0x88 / +0x90 | double | mCurX / mCurY | current position | BoardDraw (shadow) |
| +0xb0 | bool | mShowShadow | | BoardDraw |
| +0xb4 | int | mAnim | LexAnim, -1 none | Board, FlyingPieceList (== LEXANIM_SWEAT), OverlayWidget::Draw (== LEXANIM_SWEAT) |
| +0xc5 | bool | mSweating (board_logic: "danger") | danger state | Board (r), OverlayWidget::Draw |
| +0xc8 | int | mSweatFrame | 0..7 | OverlayWidget::Draw |

Widget mX/mY/mWidth/mHeight of the Lex are read by BoardDraw (burp smoke) and OverlayWidget (balloon placement,
sweat drops).

#### 2.6 WordBalloon (lex) and OverlayWidget (screens)

WordBalloon: +0xbc `Insets mTextInsets` (OverlayWidget::Update writes (-5, -10, -5, 0) after `Think`); Widget
`mVisible` read by Lex, LevelUpWidget, OverlayWidget; colours through Widget `SetColors(int[][4], 4)`.

OverlayWidget:

| Off | Type | Name | Meaning | Used outside screens |
|---|---|---|---|---|
| +0x5c | EffectMgr* | mEffectMgr | overlay's effects (flying tiles) | Board::Submit (passes it to `new FlyingPieceList`) |
| +0x60 | WordBalloon* | **mBalloon** (lex: mWordBalloon) | Lex's shared balloon | Lex (Say, Think, SayDefinition, Update, MouseLeave/Down/Up), LevelUpWidget::Update, Board (`Show(0)` at level up / game over) |
| +0x68 | int | mBannerTargetY (board_logic: target y) | 406 shown, board height hidden | Board ShowOverlay, BonusWordScored (w) |
| +0x6c | int | mBannerY (board_logic: current y) | 0 = never placed | Board ShowOverlay (w when 0) |
| +0x70 | bool | mBannerActive | | Board ShowOverlay(true) (w true) |
| +0x78 | double | **mArrowFade** (board_logic: "slide speed") | arrow alpha, Board writes 10.0, -0.025 per update (D6) | Board ShowOverlay (w 10.0) |

#### 2.7 Effect.h classes and Points (board_draw)

- Effect: +0x04 `EffectMgr* mEffectMgr` (OverlayWidget::AddEffect sets it), +0x08 `int mUpdateCnt`, +0x0c `bool mDead`.
- Sparkle: +0x10 `double mX`, +0x18 `double mY` (Board::Update sets both after `new Sparkle(mEffectMgr)`).
- FlyingPieceList: +0x10 `std::list<Piece*> mPieces` (Board::Update sums `mPieces.size()` over
  `mOverlay->GetEffects(EFFECT_FLYING_PIECES)`; the list returned is new and the caller deletes it).
- EffectMgr: +0x04 `mApp`, +0x08 `MTRand* mRand`, +0x10 `std::list<Effect*> mEffects`.
- Ember (Board::Update and Scramble fill it after `NewEmber()`): +0x10 `mX`, +0x18 `mY`, +0x20 `mVX`, +0x28 `mVY`,
  +0x30 `mGravity`, +0x38 `mAlpha`, +0x40 `mAlphaDecay` (doubles), +0x48 `int mDelay`, +0x4c `bool mLoop`,
  +0x58 `int mType` (EmberType).
- Points: +0x5c `int mLife`, +0x80 `int mHue` (Board::Update cycles it and decrements mLife), Widget mX/mY (Board
  moves it up).

#### 2.8 Front-end classes

| Class | Off | Type | Name | Used by |
|---|---|---|---|---|
| BWGameSelector | +0x99 | bool | mClosing (app: "transition running") | app GoToMainMenu (r) |
| OptionsDialog | +0x170 | int | mSlideOffset (app: "slide target") | app ShowOptions (w 240) |
| NewUserDialog | +0x128 | EditWidget* | mNameEdit | app ButtonDepress 2022 (r `mString`), BWGameSelector::Update (`SetText(mUserName)`) |
| FameWidget | +0x74 | int | mNetState | app ButtonDepress 2025 (internet, not ported) |

#### 2.9 UserManager, User, HighScoreEntry (app)

- UserManager: +0x00 `BookWormApp* mApp`, +0x04 `std::list<User*> mUsers` (BWGameSelector: `mUsers.size()`;
  SelectUserDialog::UpdateList iterates it), +0x10 `SexyString mCurrentUser` (others read it only through
  `GetCurrentUserName()`).
- User: +0x00 `SexyString mName` (registry subkey, as typed), +0x1c `SexyString mAlias`.
- HighScoreEntry: +0x00 `SexyString mName`, +0x1c `SexyString mWord`, +0x38 `int mScore`, +0x3c **`mutable bool
  mIsNew`** (`mutable` so that FameWidget's destructor can clear it through the multiset's const iterators: it is not
  part of either ordering). Containers: `typedef std::multiset<HighScoreEntry, HighScoreByScore>
  HighScoreByScoreSet; typedef std::multiset<HighScoreEntry, HighScoreByWordLength> HighScoreByLengthSet;`.

#### 2.10 Saved-game record (players\<user>\classic.dat / action.dat)

Board::SaveGame / LoadGame (board_logic) drive one file; each owner writes its own record and must keep it byte
exact (Part 2 6, Part 1 8.3):

| Order | Record | Owner (functions) |
|---|---|---|
| 1-7 | header: version 5, mProductVersion, mBuildNum, mBuildDate, mode, app mShowHints, Board mRand serialised | board_logic (reads app fields) |
| 8 | 56 cells: present byte + `Piece::Save` (191 bytes) | board_logic |
| 9 | level clock | board_logic |
| 10 | `OverlayWidget::SaveState` / `LoadState` (33 bytes, then the overlay's `EffectMgr::Save` / Clear + `Load`) | screens (+ board_draw) |
| 11 | Board `mEffectMgr->Save` / Clear + `Load`, then `mEmberMgr->Save` / Clear + `Load` | board_draw |
| 12-25 | Board fields, `WordScore::Save` x2, `StatKeeper::Save` x2 | board_logic |
| 26 | Lex present byte + `Lex::Save` (slot 47, 88 bytes) / new Lex + `Lex::Load` (slot 48) | lex |
| 27-31 | Board fields, `mGameLog.ToWebString()`, help-open byte | board_logic |

`FlyingPieceList::Save/Load` (inside the EffectMgr records) call `Piece::Save/Load`.

#### 2.11 RNG and construction order (cross-module)

`Sexy::Rand()` draws, in this order, must stay as they are: WinMain/Init `gGameRand` seed (app); then per game
`Board::Board`: `mRand` seed, then `EffectMgr` ctor, then `EmberListMgr` ctor (board_logic calls board_draw's ctors
in this order); then `Board::AddedToManager` creates the OverlayWidget whose ctor creates an EffectMgr (a fourth
draw). Every one seeds with `SeedMTRand2004(*rand, Sexy::Rand())` (the original's `new MTRand(Sexy::Rand())`).
`gGameRand` is drawn only by Board; `EffectMgr::mRand` only by FlyingPieceList::Update; `EmberListMgr::mRand`
never (serialised only); `Board::mRand` by Board only. Lex::Update uses `CrtRand()` only.

#### 2.12 Name map: what the analyses call it -> canonical

| Analysis text | Canonical |
|---|---|
| App::RecordScore, AddHighScores | `BookWormApp::SubmitHighScores(StatKeeper*)` |
| App::PauseMusic | `BookWormApp::PauseAllMusic(bool)` |
| CloseHelp | `BookWormApp::HideHelp()` |
| AskLeaveGame | `BookWormApp::GoToMainMenu()` |
| NewGame | `BookWormApp::StartGame()` |
| EndGame (0x40ffa0) | `BookWormApp::ShutdownWidgets()` |
| DrawShadowString | `DrawShadowedString(...)` |
| TruncateString, Truncate | `TruncateToWidth(...)` |
| FormatDuration | `BookWormApp::FormatTime` / `GetLevelTimeString` / `GetTotalTimeString` |
| ShowInternetScores | `BookWormApp::IsInternetScoresEnabled()` |
| OpenRegisterDialog | `BookWormApp::ShowRegisterDialog()` |
| SaveCurrentProfile | `BookWormApp::SaveCurrentUser()` |
| LoadCurrentProfile | `BookWormApp::LoadUserSettings()` |
| mProfileMgr / ProfileMgr / Profile / mProfiles | `mUserManager` / `UserManager` / `User` / `mUsers` |
| FindProfile, GetCurrentUser, SaveProfile, DeleteProfile | `FindUser`, `GetCurrentUserName`, `SaveUser`, `DeleteUser` |
| DeleteUserDialog::DeleteSelected | `DeleteUserDialog::DeleteSelectedUser()` |
| OptionsDialog slot 52 "close" | `OptionsDialog::ApplyAndClose()` |
| Board::ShowLevelUp, "Board::LevelUp 0x404b40" | `Board::ShowSummary()` 0x404b40 (`Board::LevelUp` is 0x407a50) |
| RemoveLevelUp | `Board::RemoveLevelUpWidget()` |
| Board::GetRankPoints | `Board::LevelThreshold(unsigned int)` |
| Board 0x405cd0 "remove children, state 3" | `Board::Abandon()` |
| ShowBonusBanner | `Board::ShowOverlay(bool)` |
| ColumnX / TileY | `Board::ColX(int)` / `Board::RowY(int, int)` |
| GetPieceBelow | `Board::PieceBelow(Piece*)` |
| DrawLastWord | `Board::DrawWord(Graphics*)` 0x407c60 |
| DrawSelectionArrows | `Board::DrawSelection(Graphics*)` 0x405340 |
| AddPoints | `Board::AddScorePopup(int, int, int)` |
| IsGameInProgress | `Board::IsInProgress()` |
| PiecesAdjacent / SetLevelSpeedGlobal / gLevelSpeedGlobal | `Board::IsAdjacent` / `Board::SetActionSpeed` / `gActionSpeedValue` |
| Lex::StopAnim | `Lex::Reset()` |
| Lex::SetDanger, Lex "danger" | `Lex::SetSweating(bool)`, `Lex::mSweating` |
| Lex::ShowDefinition | `Lex::SayDefinition(...)` |
| Definitionator::Lookup | `Definitionator::GetDefinition(const SexyString&)` |
| OverlayWidget Save/Load | `OverlayWidget::SaveState(FILE*)` / `LoadState(FILE*)` |
| EffectMgr::Add | `EffectMgr::AddEffect(Effect*)` |
| FUN_00436310 | `WordBalloon::SetJustification(int, int, int)` |
| ByScore / ByWordLength (fame.h) | `HighScoreByScore` / `HighScoreByWordLength` |
| PieceType / PIECE_* (board_draw.h) | `TileType` / `TILE_*` |
| TileSet (board_draw.h) | `TileSetId` (Res.h) |

### 3. Cross-module methods

Signature, original address, 2004 slot when virtual, and the modules that call it.

#### 3.1 BookWormApp (app; BookWormApp.h)

| Signature | Addr | Slot | Called by |
|---|---|---|---|
| `void ShowHelp()` | 0x4101c0 | | OptionsDialog (id 33), Board::LoadGame |
| `void HideHelp()` | 0x4102d0 | | HelpWidget::ButtonDepress, Board::SaveGame/LoadGame |
| `void RemoveBoard()` | 0x410180 | | BWGameSelector::Update |
| `void RemoveGameSelector()` | 0x410360 | | BWGameSelector::Update |
| `void RemoveFameWidget()` | 0x4103a0 | | FameWidget::ButtonDepress |
| `void ShowSelectUserDialog()` | 0x410520 | | BWGameSelector::ButtonDepress |
| `void ShowHint(int theHintId, const SexyString& theHeader, const SexyString& theLines, const SexyString& theFooter, int theButtonMode)` | 0x4105d0 | | Board::Update |
| `void ShowCredits()` | 0x4106c0 | | OptionsDialog (id 35) |
| `void ShowOptions()` | 0x4107c0 | | Board::ButtonDepress (17) |
| `void ShowNewUserDialog(const SexyString& theText)` | 0x4108c0 | | BWGameSelector::Update, SelectUserDialog::ListClicked |
| `void ShowGameSelector()` | 0x410f00 | | FameWidget::ButtonDepress |
| `void ShowHallOfFame()` | 0x411020 | | BWGameSelector::ButtonDepress, LevelUpWidget::ButtonDepress |
| `void StartGameForUser()` | 0x413c80 | | BWGameSelector::StartGame |
| `void LoadUserSettings()` | 0x413a40 | | SelectUserDialog::ButtonDepress |
| `bool SaveCurrentUser()` | 0x412100 | | OptionsDialog::ApplyAndClose |
| `void ShowRegisterDialog()` | 0x413cb0 | | (OptionsDialog id 34, TitleScreen trial: unreachable in the registered build; not ported) |
| `bool IsInternetScoresEnabled()` | 0x4152a0 | | FameWidget ctor, HelpWidget::DrawTopicText, TitleScreen::Draw (false in the port) |
| `void SelectGameMode(int theMode)` | 0x4153c0 | | BWGameSelector::ButtonDepress |
| `void ShowQuitDialog()` | 0x415660 | | BWGameSelector::ButtonDepress |
| `void GoToMainMenu()` | 0x4157e0 | | Board::ButtonDepress (18) |
| `void PauseAllMusic(bool pause)` | 0x4184d0 | | Board::KeyChar, Board::MouseDown |
| `void ShowDeleteUserDialog(std::vector<SexyString> theNames)` (by value) | 0x418590 | | SelectUserDialog::ListClicked |
| `void SubmitHighScores(StatKeeper* theStats)` | 0x419cd0 | | Board::Update (game over) |
| `SexyString GetLevelTimeString(const SexyString& thePrefix)` | 0x411ea0 | | LevelUpWidget::DrawPanel (`GT_SUMMARY_TIME`) |
| `SexyString GetTotalTimeString(const SexyString& thePrefix)` | 0x411f50 | | LevelUpWidget::DrawPanel (`GT_SUMMARY_TOTAL`) |
| `virtual void TitleScreenDone()` | 0x414ef0 | 67 | TitleScreen::ButtonDepress |
| `virtual void ButtonPress(int theId)` | 0x410b30 | 3 | Dialog2004 forwarding |
| `virtual void ButtonDepress(int theId)` | 0x417080 | 4 | Dialog2004 forwarding; NewUserDialog/GetStringDialog `EditWidgetText` call `mApp->ButtonDepress(mId + 2000)` |
| `virtual Dialog* DoDialog(int theDialogId, bool isModal, const SexyString& theHeader, const SexyString& theLines, const SexyString& theFooter, int theButtonMode)` | 0x4103e0 | 55 | Board::Scramble, DeleteUserDialog::ConfirmDelete, OptionsDialog::CheckboxChecked |
| `virtual bool KillDialog(int theDialogId)` (add `using SexyApp::KillDialog;`) | 0x4110b0 | 58 | many |
| `void DrawShadowedString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, bool theShadow, const Color& theShadowColor)` (free) | 0x410090 | | FameWidget::DrawLocalScores |
| `SexyString TruncateToWidth(Graphics* g, const SexyString& theString, int theMaxWidth, const SexyString& theEllipsis)` (free) | 0x412da0 | | FameWidget::DrawLocalScores |

#### 3.2 UserManager (app; UserManager.h)

| Signature | Addr | Called by |
|---|---|---|
| `UserManager(BookWormApp* theApp)` / `~UserManager()` | 0x435b50 / 0x434880 | app |
| `SexyString GetCurrentUserName()` (inline, returns a copy) | 0x402090 | Board Save/Load/Delete, BWGameSelector, HelpWidget, Lex::Update, Select/DeleteUserDialog |
| `User* FindUser(const SexyString& theName)` (_stricmp) | 0x434800 | SelectUserDialog |
| `bool SetCurrentUser(const SexyString& theName)` (callers that hold a `User*` pass `user->mName`) | 0x4356d0 | Select/DeleteUserDialog |
| `void DeleteUser(const SexyString& theName)` | 0x4352e0 | DeleteUserDialog::DeleteSelectedUser |
| `bool WriteUserInt(const SexyString& theName, const SexyString& theValueName, int theValue)` | 0x434d10 | HelpWidget::CheckboxChecked |
| `bool ReadUserInt(...)`, `bool WriteUserString(...)`, `void SaveUser(User*)`, `User* LoadUser(...)`, `bool SetCurrentAlias(...)`, `SexyString GetCurrentAlias()`, `SexyString GetLastUser()` | 0x4349e0, 0x434b70, 0x434e90, 0x435470, 0x435880, 0x435910, 0x4359b0 | app (GetCurrentAlias, SetCurrentAlias: internet only) |
| `User(const SexyString& theName)` | 0x434960 | app |

#### 3.3 Board (board_logic; Board.h)

| Signature | Addr | Slot | Called by |
|---|---|---|---|
| `Board(BookWormApp* theApp)` | 0x40ae90 | | app StartGame |
| `void Pause(bool pause)` (counted) | 0x405be0 | | app (ShowHelp, HideHelp, ModalOpen/Close, GotFocus, LostFocus), FameWidget and HelpWidget Added/RemovedFromManager |
| `bool IsInProgress()` (`mState != BOARD_GAMEOVER`) | 0x403720 | | app |
| `bool SaveGame(int theMode)` | 0x406c40 | | app (ShutdownWidgets, 2028) |
| `bool LoadGame(int theMode)` | 0x408ab0 | | app (2005) |
| `bool DeleteSavedGame(int theMode)` | 0x4078b0 | | app (2005, 3005), LevelUpWidget::ButtonDepress |
| `void Abandon()` (ClearPopups, state 3) | 0x405cd0 | | app ShowGameSelector |
| `void ShowOverlay(bool show)` | 0x403d80 | | app StartGame |
| `void PickBonusWord()` | 0x408030 | | app StartGame |
| `void Scramble()` | 0x40ed70 | | Lex::MouseUp |
| `int ColX(int theCol)` / `int RowY(int theCol, int theRow)` (non-static: thiscall) | 0x403990 / 0x4039b0 | | OverlayWidget::Draw |
| `static int LevelThreshold(unsigned int theLevel)` (cdecl) | 0x403880 | | LevelUpWidget::DrawPanel |
| `static bool IsAdjacent(Piece* a, Piece* b)`, `static int RandomLetter()`, `static void SetActionSpeed(double)` | 0x402f60, 0x403d10, 0x402fd0 | | Board only |
| `Piece* PieceBelow(Piece* thePiece)` | 0x403bd0 | | BoardDraw |
| Widget overrides `SetVisible(bool)` 0x405a50 slot 4, `SetDisabled(bool)` 0x405a20 slot 10 | | 4 / 10 | app ShowHelp/HideHelp |
| Draw functions, declared in Board.h by board_logic, written by board_draw in BoardDraw.cpp: `virtual void Draw(Graphics* g)` 0x40a980 slot 16, `void DrawPieces(Graphics*)` 0x40a160, `void DrawPiece(Graphics*, Piece*)` 0x4044f0, `void DrawPieceBlank(Graphics*, Piece*)` 0x404380, `void DrawSelection(Graphics*)` 0x405340, `void DrawLevelText(Graphics*)` 0x4056e0, `void DrawProgressBar(Graphics*)` 0x403c00, `void DrawScore(Graphics*)` 0x405580, `void DrawWord(Graphics*)` 0x407c60 | | | board_draw |

WordScore: `WordScore(const WordScore&)` 0x406560 (BoardDraw::DrawWord), `SexyString CommonPrefix(const SexyString&
theOther)` 0x406b00 (OverlayWidget::Draw). Piece: ctor, `Save/Load(FILE*)` (FlyingPieceList).

#### 3.4 Lex, BigLex, WordBalloon, Definitionator (lex)

| Signature | Addr | Slot | Called by |
|---|---|---|---|
| `Lex(BookWormApp* theApp)` | 0x42d560 | | Board AddedToManager, LoadGame |
| `virtual void Move(int theNewX, int theNewY)` | 0x42d370 | 14 | Board (Move(-200, 0)) |
| `void PlayAnim(int theAnim, bool theLoop)` | 0x42d320 | | Board, FlyingPieceList |
| `void SetSweating(bool isSweating)` | 0x42d3c0 | | Board::Update |
| `void Reset()` | 0x42d680 | | Board::SaveGame |
| `void Say(const SexyString& theText, Font* theFont, int theDuration)` | 0x42d6c0 | | Board::Update (`GT_LEVEL_UP`) |
| `void Think(const SexyString& theText, Font* theFont, int theDuration)` | 0x42d800 | | OverlayWidget::Update |
| `void SayDefinition(const SexyString& theWord, Font* theWordFont, const SexyString& theDefinition, Font* theDefFont, int theDuration)` | 0x42d8d0 | | Board::Submit |
| `bool IsBusy()` (inline: anim playing or not at target) | 0x404340 | | Board, FlyingPieceList, LevelUpWidget |
| `bool IsMoving()` (inline: not at target) | 0x403ab0 | | Board |
| `virtual void Save(FILE* theFile)` / `virtual void Load(FILE* theFile)` | 0x42d060 / 0x42d1c0 | 47 / 48 | Board SaveGame / LoadGame |
| `BigLex(BookWormApp* theApp)`, `void PlayAnim(int theAnim, bool theLoop)` | 0x402df0, 0x402e40 | | BWGameSelector (ctor, Update) |
| `WordBalloon(BookWormApp* theApp, Image* theBoxImage, Image* theTailImage, Point theTailPos)` | 0x4372a0 | | OverlayWidget::AddedToManager (IMAGE_BACKDROP_WB_TEXT, IMAGE_WB_TEXT_TALK_TAIL, Point(159, -2)) |
| `virtual void Show(int theDuration)` (0 = fade out and hide) | 0x436360 | 49 | Lex, LevelUpWidget::Update, Board (level up, game over) |
| `virtual int GetPreferredHeight(int theWidth)` | 0x436680 | 51 | OverlayWidget::AddedToManager |
| `Definitionator(BookWormApp* theApp)`, `bool Load(const SexyString& theFileName)` | 0x41dd30, 0x41d920 | | app LoadingThreadProc |
| `SexyString GetDefinition(const SexyString& theWord)` | 0x41d2b0 | | Board::Submit |

#### 3.5 Effects and Points (board_draw; Effect.h, Points.h)

| Signature | Addr | Called by |
|---|---|---|
| `EffectMgr(BookWormApp* theApp)` | 0x4201c0 | Board ctor, OverlayWidget ctor |
| `void Clear()`, `bool Update()` (true if it had effects), `void Draw(Graphics* g)` | 0x41f930, 0x41f990, 0x41fa10 | Board, OverlayWidget |
| `void AddEffect(Effect* theEffect)` | 0x4207f0 | Board::Update (sparkles), OverlayWidget::AddEffect |
| `bool HasEffectOfType(int theType)`, `std::list<Effect*>* GetEffectsOfType(int theType)` (new list or NULL; caller deletes) | 0x41fa50, 0x420830 | OverlayWidget::HasEffects / GetEffects |
| `void Save(FILE*)`, `void Load(FILE*)` (appends: Clear first) | 0x41faa0, 0x420b70 | Board Save/LoadGame, OverlayWidget SaveState/LoadState |
| `EmberListMgr(BookWormApp* theApp, int thePoolSize)` | 0x421070 | Board ctor (1000) |
| `Ember* NewEmber()` | 0x420d60 | Board::Update |
| `void AddEmber(int theType, double theX, double theY, double theVX, double theVY, bool theLoop, double theAlpha, double theAlphaDecay)` | 0x420e00 | Board::Update, Scramble |
| `bool Update()`, `void Draw(Graphics*)`, `void Clear()`, `void Save(FILE*)`, `void Load(FILE*)` | 0x420fd0, 0x41f8d0, 0x4202d0, 0x41ff80, 0x420e70 | Board |
| `Sparkle(EffectMgr* theEffectMgr)` | 0x41f2b0 | Board::Update |
| `FlyingPieceList(EffectMgr* theEffectMgr, std::list<Piece*>* thePieces)` (takes ownership of the pieces) | 0x420920 | Board::Submit (`mOverlay->mEffectMgr`, `&mSelection`) |
| `Points(BookWormApp* theApp, Font* theFont, const SexyString& theText, int theX, int theY, int theLife, int theJustify)` | 0x431a50 | Board AddScorePopup (JUSTIFY_CENTER), ShowMessage (-1: left) |

#### 3.6 Screens

| Signature | Addr | Slot | Called by |
|---|---|---|---|
| `TitleScreen(BookWormApp*)` | 0x434380 | | app Init |
| `BWGameSelector(BookWormApp*)` | 0x401600 | | app ShowGameSelector |
| `virtual void StartGame()` | 0x401460 | 47 | app SelectGameMode, ButtonDepress 2005/3005, GoToMainMenu |
| `virtual void StartOpenTransition()` | 0x4010c0 | 48 | app ShowGameSelector |
| `virtual void StartCloseTransition()` | 0x4011f0 | 49 | app StartGame |
| `virtual void SetDisabled(bool)` | 0x4013f0 | 10 | app (new user), SelectUserDialog |
| `HelpWidget(BookWormApp*)` | 0x429cd0 | | app ShowHelp |
| `CreditsDialog(BookWormApp*)` | 0x41c0b0 | | app ShowCredits |
| `OverlayWidget(BookWormApp*)` | 0x430c20 | | Board::AddedToManager |
| `bool IsBannerMoving()` | 0x430a00 | | Board::Update |
| `void AddEffect(Effect* theEffect)` | 0x430a20 | | Board::Submit |
| `std::list<Effect*>* GetEffects(int theType)` (caller deletes; NULL when none) | 0x430a70 | | Board::Update |
| `bool HasEffects(int theType)` | 0x430a90 | | Board::Update |
| `void SaveState(FILE*)` / `void LoadState(FILE*)` | 0x430ab0 / 0x430b20 | | Board SaveGame / LoadGame |
| `ColorizedButton(int theId, ButtonListener* theListener)` | 0x41bf50 | | LevelUpWidget::AddedToManager |

#### 3.7 Fame and dialogs

| Signature | Addr | Slot | Called by |
|---|---|---|---|
| `FameWidget(BookWormApp*)` | 0x428520 | | app ShowHallOfFame |
| `LevelUpWidget(BookWormApp*)`; `SetVisible(bool)` 0x42baf0, `SetDisabled(bool)` 0x42bad0 | 0x42bd50 | 4 / 10 | Board::ShowSummary; app ShowHelp/HideHelp |
| `OptionsDialog(BookWormApp*)` | 0x42fe90 | | app ShowOptions |
| `virtual void ApplyAndClose()` (app: `((OptionsDialog*)GetDialog(DIALOG_OPTIONS))->ApplyAndClose()`) | 0x42ed00 | 52 | app ButtonDepress 2001/3001, ShowHelp |
| `NewUserDialog(BookWormApp*, const SexyString& theDialogLines)` | 0x42e7c0 | | app ShowNewUserDialog |
| `SelectUserDialog(BookWormApp*)`; `void UpdateList()`; `virtual int GetPreferredHeight(int)` | 0x432c90; 0x432810; 0x432b00 | 50 | app; DeleteUserDialog (UpdateList, Resize with GetPreferredHeight); app 3022 |
| `DeleteUserDialog(BookWormApp*, std::vector<SexyString> theNames)` (by value) | 0x41e370 | | app ShowDeleteUserDialog |
| `void DeleteSelectedUser()` | 0x41e620 | | app ButtonDepress 2021 |
| `HintDialog(BookWormApp*, int theHintId, const SexyString& theHeader, const SexyString& theLines, const SexyString& theFooter, int theButtonMode)` | 0x42af50 | | app ShowHint |
| `static bool WasHintShown(int theHintId)` (always false, D2) | 0x42b3e0 | | app ShowHint |

### 4. Shared enums and constants

#### 4.1 Res.h (app)

Image, font and sound ids are the indices of the original name tables (0x52e410, 0x52e3c0, 0x52e668); the enum
name is the table name upper-cased. Paths: `ImagePath` (default `images\`) + name, `FontPath` (`fonts\`) + name,
`SoundPath` (`sounds\`) + name. `enum ImageId { ... NUM_IMAGES = 149 }`:

| Id | Name | Files in images/ (`_x` = alpha) |
|---|---|---|
| 0 | `IMAGE_BACKDROP_TITLE` | backdrop_title.jpg |
| 1 | `IMAGE_BACKDROP_TITLE_LOADMETER` | backdrop_title_loadmeter.gif, _backdrop_title_loadmeter.gif |
| 2 | `IMAGE_BACKDROP_REGISTER` | backdrop_register.gif, _backdrop_register.gif |
| 3 | `IMAGE_LOGO_TITLE` | logo_title.gif, _logo_title.gif |
| 4 | `IMAGE_TITLE_LOADMETER` | title_loadmeter.gif, _title_loadmeter.gif |
| 5 | `IMAGE_TITLE_STAMP_TRIAL` | title_stamp_trial.gif, _title_stamp_trial.gif |
| 6 | `IMAGE_TITLE_STAMP_REGISTERED` | title_stamp_registered.gif, _title_stamp_registered.gif |
| 7 | `IMAGE_DIALOG` | dialog.gif, _dialog.gif |
| 8 | `IMAGE_DIALOGBUTTON` | dialogbutton.gif, _dialogbutton.gif |
| 9 | `IMAGE_BUTTON_REG_REGISTER` | button_reg_register.gif, _button_reg_register.gif |
| 10 | `IMAGE_BUTTON_REG_CANCEL` | button_reg_cancel.gif, _button_reg_cancel.gif |
| 11 | `IMAGE_BUTTON_REG_REGCODE` | button_reg_regcode.gif, _button_reg_regcode.gif |
| 12 | `IMAGE_EDITBOX` | editbox.gif |
| 13 | `IMAGE_CURSOR_POINTER` | cursor_pointer.gif, _cursor_pointer.gif |
| 14 | `IMAGE_CURSOR_HAND` | cursor_hand.gif, _cursor_hand.gif |
| 15 | `IMAGE_CURSOR_DRAGGING` | cursor_dragging.gif, _cursor_dragging.gif |
| 16 | `IMAGE_CURSOR_TEXT` | cursor_text.gif, _cursor_text.gif |
| 17 | `IMAGE_BACKDROP_BOARD` | backdrop_board.gif |
| 18 | `IMAGE_BACKDROP_BOARD_SCORCHED` | backdrop_board_scorched.gif |
| 19 | `IMAGE_BACKDROP_BONUS` | backdrop_bonus.gif, _backdrop_bonus.gif |
| 20 | `IMAGE_BACKDROP_MENU` | backdrop_menu.gif |
| 21 | `IMAGE_BACKDROP_FAME` | backdrop_fame.gif |
| 22 | `IMAGE_BACKDROP_HELP` | backdrop_help.gif |
| 23 | `IMAGE_BACKDROP_HELP_BOX` | backdrop_help_box.gif, _backdrop_help_box.gif |
| 24 | `IMAGE_BACKDROP_IHS` | backdrop_ihs.gif |
| 25 | `IMAGE_BACKDROP_WB_SPEECH` | backdrop_wb_speech.gif, _backdrop_wb_speech.gif |
| 26 | `IMAGE_BACKDROP_WB_THOUGHT` | backdrop_wb_thought.gif, _backdrop_wb_thought.gif |
| 27 | `IMAGE_BACKDROP_WB_TEXT` | backdrop_wb_text.gif, _backdrop_wb_text.gif |
| 28 | `IMAGE_GAMESELECT_ACTION` | gameselect_action.gif, _gameselect_action.gif |
| 29 | `IMAGE_GAMESELECT_ACTION_OVER` | gameselect_action_over.gif, _gameselect_action_over.gif |
| 30 | `IMAGE_GAMESELECT_CLASSIC` | gameselect_classic.gif, _gameselect_classic.gif |
| 31 | `IMAGE_GAMESELECT_CLASSIC_OVER` | gameselect_classic_over.gif, _gameselect_classic_over.gif |
| 32 | `IMAGE_GAMESELECT_HALL` | gameselect_hall.gif, _gameselect_hall.gif |
| 33 | `IMAGE_GAMESELECT_HALL_OVER` | gameselect_hall_over.gif, _gameselect_hall_over.gif |
| 34 | `IMAGE_GAMESELECT_QUIT` | gameselect_quit.gif, _gameselect_quit.gif |
| 35 | `IMAGE_GAMESELECT_QUIT_OVER` | gameselect_quit_over.gif, _gameselect_quit_over.gif |
| 36 | `IMAGE_GAMESELECT_BUBBLE` | gameselect_bubble.gif, _gameselect_bubble.gif |
| 37 | `IMAGE_DIALOG_LEVELUP` | dialog_levelup.gif, _dialog_levelup.gif |
| 38 | `IMAGE_BUTTON_FAME_CONTINUE` | button_fame_continue.gif, _button_fame_continue.gif |
| 39 | `IMAGE_BUTTON_HELP_CONTINUE` | button_help_continue.gif, _button_help_continue.gif |
| 40 | `IMAGE_BUTTON_GOTO_IHS` | button_goto_ihs.gif, _button_goto_ihs.gif |
| 41 | `IMAGE_BUTTON_GOTO_FAME` | button_goto_fame.gif, _button_goto_fame.gif |
| 42 | `IMAGE_BUTTON_SUBMIT_IHS` | button_submit_ihs.gif, _button_submit_ihs.gif |
| 43 | `IMAGE_BUTTON_SUBMIT_IHS_HILITE` | button_submit_ihs_hilite.gif, _button_submit_ihs_hilite.gif |
| 44 | `IMAGE_BUTTON_HELP_TOPIC` | button_help_topic.gif, _button_help_topic.gif |
| 45 | `IMAGE_BUTTON_HELP_TOPIC_HILITE` | button_help_topic_hilite.gif, _button_help_topic_hilite.gif |
| 46 | `IMAGE_IHS_TAB_NAMES` | ihs_tab_names.gif, _ihs_tab_names.gif |
| 47 | `IMAGE_IHS_TAB_NAMES_HILITE` | ihs_tab_names_hilite.gif, _ihs_tab_names_hilite.gif |
| 48 | `IMAGE_IHS_TAB_INDICATOR` | ihs_tab_indicator.gif, _ihs_tab_indicator.gif |
| 49 | `IMAGE_IHS_ALIAS_WARNING` | ihs_alias_warning.gif |
| 50 | `IMAGE_BAR_LEVELUP_HEADER` | bar_levelup_header.gif, _bar_levelup_header.gif |
| 51 | `IMAGE_BAR_GAMEOVER_HEADER` | bar_gameover_header.gif, _bar_gameover_header.gif |
| 52 | `IMAGE_BAR_FOOTER_SCROLL` | bar_footer_scroll.gif, _bar_footer_scroll.gif |
| 53 | `IMAGE_BAR_FOOTER_CONTINUE` | bar_footer_continue.gif, _bar_footer_continue.gif |
| 54 | `IMAGE_RANKSTRIP` | rankstrip.jpg |
| 55 | `IMAGE_RANKFRAME` | rankframe.gif |
| 56 | `IMAGE_LEVEL_UP_STAR` | level_up_star.gif, _level_up_star.gif |
| 57 | `IMAGE_FX_ASHFLAKE` | fx_ashflake.gif, _fx_ashflake.gif |
| 58 | `IMAGE_FX_EMBER_SM` | fx_ember_sm.gif, _fx_ember_sm.gif |
| 59 | `IMAGE_FX_EMBER_MED` | fx_ember_med.gif, _fx_ember_med.gif |
| 60 | `IMAGE_FX_EMBER_LRG` | fx_ember_lrg.gif, _fx_ember_lrg.gif |
| 61 | `IMAGE_FX_EMBER_SMOKE_SM` | fx_ember_smoke_sm.gif, _fx_ember_smoke_sm.gif |
| 62 | `IMAGE_FX_EMBER_SMOKE_MED` | fx_ember_smoke_med.gif, _fx_ember_smoke_med.gif |
| 63 | `IMAGE_FX_EMBER_SMOKE_LRG` | fx_ember_smoke_lrg.gif, _fx_ember_smoke_lrg.gif |
| 64 | `IMAGE_FX_PARTICLE_SM` | fx_particle_sm.gif, _fx_particle_sm.gif |
| 65 | `IMAGE_FX_PARTICLE_MED` | fx_particle_med.gif, _fx_particle_med.gif |
| 66 | `IMAGE_FX_PARTICLE_LRG` | fx_particle_lrg.gif, _fx_particle_lrg.gif |
| 67 | `IMAGE_FX_SPARK_SM` | fx_spark_sm.gif |
| 68 | `IMAGE_FX_SPARK_MED` | fx_spark_med.gif |
| 69 | `IMAGE_FX_SPARK_LRG` | fx_spark_lrg.gif |
| 70 | `IMAGE_FX_SMOKE_SM` | _fx_smoke_sm.gif |
| 71 | `IMAGE_FX_FLAME_LRG` | fx_flame_lrg.gif |
| 72 | `IMAGE_FX_FIRELINE` | fx_fireline.gif, _fx_fireline.gif |
| 73 | `IMAGE_FX_LEX_SWEAT_SEQ` | fx_lex_sweat_seq.gif, _fx_lex_sweat_seq.gif |
| 74 | `IMAGE_FX_LEX_SWEAT_TRICKLE` | fx_lex_sweat_trickle.gif, _fx_lex_sweat_trickle.gif |
| 75 | `IMAGE_TILE` | tile.gif, _tile.gif |
| 76 | `IMAGE_TILE_HILITE` | tile_hilite.gif, _tile_hilite.gif |
| 77 | `IMAGE_TILE_FIRE` | tile_fire.gif, _tile_fire.gif |
| 78 | `IMAGE_TILE_FIRE_HILITE` | tile_fire_hilite.gif, _tile_fire_hilite.gif |
| 79 | `IMAGE_TILE_GOLD` | tile_gold.gif, _tile_gold.gif |
| 80 | `IMAGE_TILE_GOLD_HILITE` | tile_gold_hilite.gif, _tile_gold_hilite.gif |
| 81 | `IMAGE_TILE_GREEN` | tile_green.gif, _tile_green.gif |
| 82 | `IMAGE_TILE_GREEN_HILITE` | tile_green_hilite.gif, _tile_green_hilite.gif |
| 83 | `IMAGE_TILE_SAPPHIRE` | tile_sapphire.gif, _tile_sapphire.gif |
| 84 | `IMAGE_TILE_SAPPHIRE_HILITE` | tile_sapphire_hilite.gif, _tile_sapphire_hilite.gif |
| 85 | `IMAGE_TILE_DIAMOND` | tile_diamond.gif, _tile_diamond.gif |
| 86 | `IMAGE_TILE_DIAMOND_HILITE` | tile_diamond_hilite.gif, _tile_diamond_hilite.gif |
| 87 | `IMAGE_TILE_CHARRED` | tile_charred.gif, _tile_charred.gif |
| 88 | `IMAGE_TILE_GEMHILITE` | tile_gemhilite.gif |
| 89 | `IMAGE_TILE_SPIN_FX` | tile_spin_fx.gif |
| 90 | `IMAGE_TILE_X` | tile_x.gif, _tile_x.gif |
| 91 | `IMAGE_FONT_TILE_LETTERS` | _font_tile_letters.gif |
| 92 | `IMAGE_FONT_TILE_DIGITS` | font_tile_digits.gif, _font_tile_digits.gif |
| 93 | `IMAGE_TILE_GREEN_GLOW` | tile_green_glow.gif |
| 94 | `IMAGE_TILE_GOLD_GLOW` | tile_gold_glow.gif |
| 95 | `IMAGE_TILE_FIRE_GLOW` | tile_fire_glow.gif |
| 96 | `IMAGE_TILE_FIRE_FX1` | tile_fire_fx1.gif |
| 97 | `IMAGE_TILE_FIRE_FX2` | tile_fire_fx2.gif |
| 98 | `IMAGE_ARROWS` | arrows.gif, _arrows.gif |
| 99 | `IMAGE_BONUSARROW_LEFT` | bonusarrow_left.gif, _bonusarrow_left.gif |
| 100 | `IMAGE_BONUSARROW_RIGHT` | bonusarrow_right.gif, _bonusarrow_right.gif |
| 101 | `IMAGE_BONUSARROW_TOP` | bonusarrow_top.gif, _bonusarrow_top.gif |
| 102 | `IMAGE_LEX_SHADOW` | _lex_shadow.gif |
| 103 | `IMAGE_LEX_HEADLESS` | lex_headless.gif, _lex_headless.gif |
| 104 | `IMAGE_LEX_GLOW_IDLE` | lex_glow_idle.gif, _lex_glow_idle.gif |
| 105 | `IMAGE_LEX_SMILE_IDLE` | lex_smile_idle.gif, _lex_smile_idle.gif |
| 106 | `IMAGE_LEX_SMILE_BLINK` | lex_smile_blink.gif, _lex_smile_blink.gif |
| 107 | `IMAGE_LEX_GIGGLE_1` | lex_giggle_1.gif, _lex_giggle_1.gif |
| 108 | `IMAGE_LEX_GIGGLE_2` | lex_giggle_2.gif, _lex_giggle_2.gif |
| 109 | `IMAGE_LEX_SLURP_1` | lex_slurp_1.gif, _lex_slurp_1.gif |
| 110 | `IMAGE_LEX_SPEAK_AH` | lex_speak_ah.gif, _lex_speak_ah.gif |
| 111 | `IMAGE_LEX_SPEAK_EE` | lex_speak_ee.gif, _lex_speak_ee.gif |
| 112 | `IMAGE_LEX_SPEAK_OH` | lex_speak_oh.gif, _lex_speak_oh.gif |
| 113 | `IMAGE_LEX_SPEAK_PB` | lex_speak_pb.gif, _lex_speak_pb.gif |
| 114 | `IMAGE_LEX_SWEAT` | lex_sweat.gif, _lex_sweat.gif |
| 115 | `IMAGE_LEX_SWEAT_FACING` | lex_sweat_facing.gif, _lex_sweat_facing.gif |
| 116 | `IMAGE_LEX_BURNT` | lex_burnt.gif, _lex_burnt.gif |
| 117 | `IMAGE_LEX_BURNT_BLINK` | lex_burnt_blink.gif, _lex_burnt_blink.gif |
| 118 | `IMAGE_LEX_EATING` | lex_eating.gif, _lex_eating.gif |
| 119 | `IMAGE_LEXBIG_SPEAK_AH` | lexbig_speak_ah.gif, _lexbig_speak_ah.gif |
| 120 | `IMAGE_LEXBIG_SPEAK_EE` | lexbig_speak_ee.gif, _lexbig_speak_ee.gif |
| 121 | `IMAGE_LEXBIG_SPEAK_MM` | lexbig_speak_mm.gif, _lexbig_speak_mm.gif |
| 122 | `IMAGE_LEXBIG_SPEAK_OO` | lexbig_speak_oo.gif, _lexbig_speak_oo.gif |
| 123 | `IMAGE_LEXBIG_SPEAK_PB` | lexbig_speak_pb.gif, _lexbig_speak_pb.gif |
| 124 | `IMAGE_WB_TEXT_SHOUT_TAIL` | wb_text_shout_tail.gif, _wb_text_shout_tail.gif |
| 125 | `IMAGE_WB_TEXT_TALK_TAIL` | wb_text_talk_tail.gif, _wb_text_talk_tail.gif |
| 126 | `IMAGE_WB_TEXT_THINK_TAIL` | wb_text_think_tail.gif, _wb_text_think_tail.gif |
| 127 | `IMAGE_PROGRESS_GREEN` | progress_green.gif, _progress_green.gif |
| 128 | `IMAGE_PROGRESS_LAVENDER` | progress_lavender.gif, _progress_lavender.gif |
| 129 | `IMAGE_PROGRESS_LIME` | progress_lime.gif, _progress_lime.gif |
| 130 | `IMAGE_PROGRESS_MAGENTA` | progress_magenta.gif, _progress_magenta.gif |
| 131 | `IMAGE_PROGRESS_MULTI` | progress_multi.gif, _progress_multi.gif |
| 132 | `IMAGE_PROGRESS_NAVY` | progress_navy.gif, _progress_navy.gif |
| 133 | `IMAGE_PROGRESS_ORANGE` | progress_orange.gif, _progress_orange.gif |
| 134 | `IMAGE_PROGRESS_PURPLE` | progress_purple.gif, _progress_purple.gif |
| 135 | `IMAGE_PROGRESS_RED` | progress_red.gif, _progress_red.gif |
| 136 | `IMAGE_PROGRESS_TAN` | progress_tan.gif, _progress_tan.gif |
| 137 | `IMAGE_PROGRESS_TEAL` | progress_teal.gif, _progress_teal.gif |
| 138 | `IMAGE_PROGRESS_YELLOW` | progress_yellow.gif, _progress_yellow.gif |
| 139 | `IMAGE_PROGRESS_BURNT` | progress_burnt.gif, _progress_burnt.gif |
| 140 | `IMAGE_SPARKLE` | sparkle.gif |
| 141 | `IMAGE_SPARKLE_LARGE` | sparkle_large.gif |
| 142 | `IMAGE_DIALOGBUTTON_PLAIN` | dialogbutton_plain.gif |
| 143 | `IMAGE_SLIDERTRACK` | slidertrack.gif |
| 144 | `IMAGE_SLIDERTHUMB` | sliderthumb.gif, _sliderthumb.gif |
| 145 | `IMAGE_CHECKBOXES` | checkboxes.gif |
| 146 | `IMAGE_CHECKBOXES_HELP` | checkboxes_help.gif |
| 147 | `IMAGE_WAITBAR` | waitbar.gif |
| 148 | `IMAGE_CLOUDS` | _clouds.jpg |

`enum FontId { ... NUM_FONTS = 19 }` (`mFonts[id]`, app offset +0x11b0 + 4 id):

| Id | App off | Name | Descriptor | Loaded by |
|---|---|---|---|---|
| 0 | 0x11b0 | `FONT_VERDANA10BOLD` | Verdana10Bold.txt | Init |
| 1 | 0x11b4 | `FONT_VERDANA12BOLD` | Verdana12Bold.txt | Init |
| 2 | 0x11b8 | `FONT_VERDANA14BOLD` | Verdana14Bold.txt | Init |
| 3 | 0x11bc | `FONT_VERDANA18BOLDCOLOR` | Verdana18BoldColor.txt | Init |
| 4 | 0x11c0 | `FONT_LOCKERGNOME14OUTLINE` | Lockergnome14Outline.txt | Init |
| 5 | 0x11c4 | `FONT_VERDANA18BOLD` | Verdana18Bold.txt | loading thread |
| 6 | 0x11c8 | `FONT_SCRIPPSCOLLEGEOS13BOLD` | ScrippsCollegeOS13Bold.txt | loading thread |
| 7 | 0x11cc | `FONT_SCRIPPSCOLLEGEOS15BOLD` | ScrippsCollegeOS15Bold.txt | loading thread |
| 8 | 0x11d0 | `FONT_SCRIPPSCOLLEGEOS18BOLD` | ScrippsCollegeOS18Bold.txt | loading thread |
| 9 | 0x11d4 | `FONT_LOCKERGNOME18` | Lockergnome18.txt | loading thread |
| 10 | 0x11d8 | `FONT_LOCKERGNOME24` | Lockergnome24.txt | loading thread |
| 11 | 0x11dc | `FONT_COURIERFINALDRAFT20BOLD` | CourierFinalDraft20Bold.txt | loading thread |
| 12 | 0x11e0 | `FONT_GOLDSCORE` | GoldScore.txt | loading thread |
| 13 | 0x11e4 | `FONT_GOLDSCOREHEATED` | GoldScoreHeated.txt | loading thread |
| 14 | 0x11e8 | `FONT_LOCKERGNOMESMALLCOLOR` | LockergnomeSmallColor.txt | loading thread |
| 15 | 0x11ec | `FONT_LEGACYSERIFMDSCITCTT12` | LegacySerifMdSCITCTT12.txt | loading thread |
| 16 | 0x11f0 | `FONT_MYRIADWEB10` | MyriadWeb10.txt | loading thread |
| 17 | 0x11f4 | `FONT_MYRIADCONDENSEDWEB11` | MyriadCondensedWeb11.txt | loading thread |
| 18 | 0x11f8 | `FONT_MYRIADWEB12BOLD` | MyriadWeb12Bold.txt | loading thread |

`enum SoundId { ... NUM_SOUNDS = 47 }`. Id 24 is a second `buttonclick` entry; the rising notes are played as
`SOUND_UP1 + min(n, 7)`:

| Id | Name | File in sounds/ |
|---|---|---|
| 0 | `SOUND_BUTTONCLICK` | buttonclick.ogg |
| 1 | `SOUND_GAMEOVERTONE` | gameovertone.ogg |
| 2 | `SOUND_BAD` | bad.ogg |
| 3 | `SOUND_BLANK` | blank.au |
| 4 | `SOUND_UP1` | up1.ogg |
| 5 | `SOUND_UP2` | up2.ogg |
| 6 | `SOUND_UP3` | up3.ogg |
| 7 | `SOUND_UP4` | up4.ogg |
| 8 | `SOUND_UP5` | up5.ogg |
| 9 | `SOUND_UP6` | up6.ogg |
| 10 | `SOUND_UP7` | up7.ogg |
| 11 | `SOUND_UP8` | up8.ogg |
| 12 | `SOUND_UP9` | up9.ogg |
| 13 | `SOUND_NEXTSELECT` | nextselect.ogg |
| 14 | `SOUND_WORDCOMPLETE` | wordcomplete.ogg |
| 15 | `SOUND_BONUSCOMPLETE` | bonuscomplete.ogg |
| 16 | `SOUND_BUZZER` | buzzer.ogg |
| 17 | `SOUND_PIECE_HIT` | piece_hit.ogg |
| 18 | `SOUND_CHOMP` | chomp.ogg |
| 19 | `SOUND_FLAMETILE` | flametile.ogg |
| 20 | `SOUND_WARNING` | warning.ogg |
| 21 | `SOUND_TILEBURN` | tileburn.ogg |
| 22 | `SOUND_APPEAR2` | appear2.ogg |
| 23 | `SOUND_GOLDTILE` | goldtile.ogg |
| 24 | `SOUND_BUTTONCLICK_2` | buttonclick.ogg |
| 25 | `SOUND_REDTILECLEARED` | redtilecleared.ogg |
| 26 | `SOUND_SPINSUPERBONUS` | spinsuperbonus.ogg |
| 27 | `SOUND_BONUSWORD` | bonusword.ogg |
| 28 | `SOUND_SCREENBURNGAMEOVER` | screenburngameover.ogg |
| 29 | `SOUND_LEX_ASTONISHING` | lex_astonishing.ogg |
| 30 | `SOUND_LEX_AWESOME` | lex_awesome.ogg |
| 31 | `SOUND_LEX_BURP` | lex_burp.ogg |
| 32 | `SOUND_LEX_CAREFUL` | lex_careful.ogg |
| 33 | `SOUND_LEX_DANGER` | lex_danger.ogg |
| 34 | `SOUND_LEX_EXCELLENT` | lex_excellent.ogg |
| 35 | `SOUND_LEX_FANTASTIC` | lex_fantastic.ogg |
| 36 | `SOUND_LEX_GETREADY` | lex_getready.ogg |
| 37 | `SOUND_LEX_GO` | lex_go.ogg |
| 38 | `SOUND_LEX_INCREDIBLE` | lex_incredible.ogg |
| 39 | `SOUND_LEX_LETSGO` | lex_letsgo.ogg |
| 40 | `SOUND_LEX_LEVELUP` | lex_levelup.ogg |
| 41 | `SOUND_LEX_LOOKOUT` | lex_lookout.ogg |
| 42 | `SOUND_LEX_SCRAMBLE` | lex_scramble.ogg |
| 43 | `SOUND_LEX_UNBELIEVABLE` | lex_unbelievable.ogg |
| 44 | `SOUND_LEX_VERYGOOD` | lex_verygood.ogg |
| 45 | `SOUND_LEX_WATCHOUT` | lex_watchout.ogg |
| 46 | `SOUND_LEX_WELCOME` | lex_welcome.ogg |

Music: one MO3 (`music\Bookworm1.mo3`) loaded as five songs. `enum SongId { SONG_MENU = 0, SONG_CLASSIC = 1,
SONG_ACTION = 2, SONG_JINGLE = 3, SONG_DANGER = 4, NUM_SONGS = 5 }`. `enum MusicOrder { ORDER_CLASSIC = 0,
ORDER_MENU = 27, ORDER_ACTION = 37, ORDER_DANGER_CLASSIC = 51, ORDER_FAME = 54, ORDER_GAMEOVER_ACTION = 59,
ORDER_GAMEOVER_CLASSIC = 61, ORDER_DANGER_ACTION = 65 }` (MO3 order numbers, row 0). The calls are the port's
`MusicInterface::PlayMusic(song, offset, noLoop)`, `FadeIn(song, offset, speed, noLoop)`, `FadeOut(song, stopSong,
speed)`: the last bool of PlayMusic/FadeIn is **noLoop** (D22). Calls by module: app (menu, loading, new game),
Board (danger on/off, game over: `FadeIn(SONG_JINGLE, ORDER_GAMEOVER_*, 1.0, true)`), FameWidget (open:
`FadeOut(SONG_MENU, false, 0.008)`, `FadeIn(SONG_JINGLE, ORDER_FAME, 0.008, false)`; close: `FadeOut(SONG_JINGLE,
true, 0.008)`, `FadeIn(SONG_MENU, -1, 0.008, false)`).

`enum GameMode { GAMEMODE_CLASSIC = 0, GAMEMODE_ACTION = 1 }` (app mGameMode, selector, Board, BoardDraw,
LevelUpWidget).

`enum TileSetId { TILESET_NORMAL = 0, TILESET_NORMAL_HILITE = 1, TILESET_GREEN = 2, TILESET_GREEN_HILITE = 3,
TILESET_GOLD = 4, TILESET_GOLD_HILITE = 5, TILESET_FIRE = 6, TILESET_FIRE_HILITE = 7, TILESET_CHARRED = 8,
NUM_TILESETS = 9 }` (`mTileSetImages`; built by the app, drawn by BoardDraw and FlyingPieceList).

`enum DialogId` (SexyApp dialog map; YES/OK/footer reach `ButtonDepress` as `Dialog2004::ID_YES_2004 + id` = 2000 +
id, NO/CANCEL as 3000 + id; button modes are the port's `Dialog::BUTTONS_NONE/YES_NO/OK_CANCEL/FOOTER` = 0..3):

| Id | Name | Created by | Ported |
|---|---|---|---|
| 0 | DIALOG_UNUSED_0 | nobody (2000: KillDialog + ShowGameSelector) | dead code kept |
| 1 | DIALOG_OPTIONS | app ShowOptions (OptionsDialog) | yes |
| 2 | DIALOG_REGISTER | RegisterDialog | no |
| 3 | DIALOG_UNUSED_3 | nobody (killed by StartGame/ShowGameSelector) | dead code kept |
| 4 | DIALOG_TRIAL_EXPIRED | TitleScreen trial | no |
| 5 | DIALOG_CONTINUE (screens.h: DIALOG_CONTINUE_GAME) | app SelectGameMode | yes |
| 6 | DIALOG_INTERNET_REQUIRED | app ConfirmInternetConnect | no |
| 7 | DIALOG_ASK_UPDATE_CHECK | app | no |
| 8 | DIALOG_UPDATE_CHECK | app (UpdateCheckDialog) | no |
| 9 | DIALOG_INVALID_CODE | app | no |
| 11 | DIALOG_OPENING_BROWSER | app OpenURL | yes (app) |
| 12 | DIALOG_URL_FAILED | app URLOpenFailed | yes (app) |
| 13 | DIALOG_QUIT | app ShowQuitDialog | yes |
| 15 | DIALOG_HINT (board_logic.h: HINT_DIALOG_ID) | app ShowHint (HintDialog); Board::Scramble DoDialog `GT_CANT_SCRAMBLE` | yes |
| 16 | DIALOG_CREDITS | app ShowCredits | yes |
| 17 | DIALOG_REGISTER_UPSELL (board_logic.h: DICTIONARY_UPGRADE_DIALOG_ID; fame: FAME_DIALOG_REGISTER_NAG) | Board (trial), FameWidget (internet) | no |
| 18 | DIALOG_MESSAGE | app, OptionsDialog (`GT_OPTIONS_3D_WARNING_HEADER`, `GT_OPTIONS_3D_NOT_SUPPORTED_HEADER`) | yes |
| 19 | DIALOG_SELECT_USER | app ShowSelectUserDialog | yes |
| 20 | DIALOG_DELETE_USER | app ShowDeleteUserDialog | yes |
| 21 | DIALOG_CONFIRM_DELETE_USER (dialogs.h: DIALOG_CONFIRM_DELETE) | DeleteUserDialog::ConfirmDelete | yes |
| 22 | DIALOG_NEW_USER | app ShowNewUserDialog | yes |
| 23 | DIALOG_ALIAS (dialogs.h: DIALOG_GET_ALIAS) | app ShowAliasDialog | no |
| 24 | DIALOG_NAME_TAKEN (fame: FAME_DIALOG_DIRTY_NAME) | FameWidget | no |
| 25 | DIALOG_CONNECTING (screens: DIALOG_WAIT; fame: FAME_DIALOG_WAIT) | FameWidget (WaitDialog) | no |
| 26 | DIALOG_IHS_ERROR (fame: FAME_DIALOG_ERROR) | FameWidget | no |
| 27 | DIALOG_IHS_WARNING (fame: FAME_DIALOG_WARNING) | FameWidget | no |
| 28 | DIALOG_LEAVE_GAME | app GoToMainMenu | yes |
| 10008 / 20008 | DIALOG_UPDATE_AVAILABLE / DIALOG_UPDATE_NONE | UpdateCheckDialog | no |

Button ids handled by more than one module: `BUTTON_HINT_DISABLE = 26` (Res.h; HintDialog's `GT_HINT_DISABLE` button, also decoded by the app's ButtonDepress; replaces `HintDialog::DISABLE_HINTS_BUTTON_ID`). All other
widget ids are heard only by their own class and stay class-scoped in that class's header (Board.h 16/17/18,
BWGameSelector 0-4, TitleScreen 0/1, HelpWidget 5/27/28-32, LevelUpWidget 6, FameWidget 7/8/0x13-0x19,
OptionsDialog 33-43/1036, edit/list widgets 0).

#### 4.2 Board.h (board_logic)

- `enum TileType { TILE_NORMAL = 0, TILE_GREEN = 1, TILE_GOLD = 2, TILE_FIRE = 3, TILE_SAPPHIRE = 4, TILE_DIAMOND = 5,
  NUM_TILE_TYPES = 6 }` (Piece::mType; WordScore::mTypeCount index; BoardDraw, FlyingPieceList, Lex).
- `enum BoardState { BOARD_PLAYING = 0, BOARD_GAMEOVER_MUSIC = 1, BOARD_GAMEOVER = 2, BOARD_INACTIVE = 3,
  BOARD_GAMEOVER_PENDING = 4, BOARD_LEVEL_COMPLETE = 5, BOARD_NEXT_LEVEL = 6, BOARD_SCRAMBLING = 7 }`
  (app, LevelUpWidget, OverlayWidget, BoardDraw).
- `enum SelectMode { SELECT_NONE, SELECT_CLICKED, SELECT_DRAGGING, SELECT_SUBMIT }` (Board only).
- `enum BoardButtonId { BOARD_BUTTON_SUBMIT = 16, BOARD_BUTTON_OPTIONS = 17, BOARD_BUTTON_LEAVE = 18 }`.
- `enum BoardHintId { HINT_HOW_TO_PLAY = 0, HINT_WATCH_OUT = 1, HINT_GREEN_TILE = 2, HINT_GOLD_TILE = 3,
  HINT_SCRAMBLE = 4, HINT_SUBMIT_BUTTON = 5, HINT_SUBMIT_CLICK = 6, HINT_SUBMIT_DRAG = 7, HINT_SUBMIT_KEY = 8,
  HINT_BONUS_WORD = 9, HINT_FIRE_ALERT = 10 }` (Board::mHintId, passed to `ShowHint` and HintDialog's mHintId; only
  1, 2, 4, 10 are scheduled; app flags: HINT_WATCH_OUT -> mWatchOutHintShown, HINT_FIRE_ALERT ->
  mFireAlertHintShown, HINT_GREEN_TILE -> mGreenTileHintShown, HINT_SCRAMBLE -> mScrambleHintShown).
- `enum { BOARD_COLS = 7, BOARD_ROWS = 8, TILE_SIZE = 57 }`.
- Tables `gLevelThresholds`, `gLetterValues` (shared, 1.4), `gLetterWeights`, `gFireChanceByLevel`, ... as in
  board_logic.h.

#### 4.3 Lex.h (lex)

`enum LexAnim` (rows of gLexAnims; Lex::mAnim):

| Id | Name | Played by |
|---|---|---|
| -1 | LEXANIM_NONE | |
| 0 / 1 | LEXANIM_GO / LEXANIM_LETSGO | Board classic greeting (+ lex_go / lex_letsgo) |
| 2 / 3 / 4 / 5 | LEXANIM_LOOKOUT / LEXANIM_WATCHOUT / LEXANIM_CAREFUL / LEXANIM_DANGER | Board danger talk |
| 6 | LEXANIM_UNUSED_6 | nobody |
| 7 | LEXANIM_BONUS | Board bonus word, time bonus |
| 8..14 | LEXANIM_ASTONISHING, _AWESOME, _EXCELLENT, _FANTASTIC, _INCREDIBLE, _UNBELIEVABLE, _VERYGOOD | Board praise |
| 15 | LEXANIM_LEVELUP | Board `GT_LEVEL_UP` |
| 16 | LEXANIM_UNUSED_16 | nobody |
| 17 | LEXANIM_GETREADY | Board action greeting |
| 18 | LEXANIM_SCRAMBLE | Board::Scramble |
| 19 | LEXANIM_SWEAT (looped) | Board danger idle; tested by FlyingPieceList and OverlayWidget |
| 20 | LEXANIM_OH | Board::Refill green tile (+ appear2) |
| 21 | LEXANIM_EAT | FlyingPieceList (mouth open) |
| 22 | LEXANIM_EAT_END | FlyingPieceList (all eaten), Board (burp fade) |
| 23 | **LEXANIM_BURP** (lex.h: LEXANIM_EAT_BONUS) | Board::Update burp (+ lex_burp) (D5) |
| 24 | LEXANIM_BURNT | nobody |
| 25 | LEXANIM_BURNT_BLINK | Lex random (burnt board), Board when the board burns |
| 26 / 27 | LEXANIM_IDLE / LEXANIM_BLINK | Lex random |

#### 4.4 Effect.h and Points.h (board_draw)

- `enum EffectType { EFFECT_NONE = -1, EFFECT_SPARKLE = 0, EFFECT_FLYING_PIECES = 1 }` (Board::Update,
  OverlayWidget::GetEffects/HasEffects; also the type word of the effect records).
- `enum EmberType { EMBER_SPARK_SM = 0, EMBER_SPARK_MED = 1, EMBER_SPARK_LRG = 2, EMBER_EMBER_SM = 3,
  EMBER_EMBER_MED = 4, EMBER_EMBER_LRG = 5, EMBER_SMOKE_SM = 6, EMBER_SMOKE_MED = 7, EMBER_SMOKE_LRG = 8,
  EMBER_FLAME_LRG = 9, EMBER_SMOKE_PUFF = 10 }` (Board writes types 0, 1, 2, 3, 10).
- `Points::JUSTIFY_CENTER = 0`, `Points::JUSTIFY_RIGHT = 1`, any other value = left (Board passes -1).

### 5. Discrepancies resolved

D1. **UserManager::DeleteUser frees the User** (app, dialogs) - screens said it leaks both the User and the node.
Disassembly 0x4353ff-0x435432: after `RegDeleteKeyA` succeeds it calls `~pair` 0x411520 and `operator delete`
0x4da392 on the User, unlinks the node, deletes it and decrements the size. Only when RegDeleteKey succeeds.

D2. **HintDialog::WasHintShown ignores its argument** (dialogs; app's open question 8): 0x42b3e6 pushes `&[ebp-4]`
and 0x42b3f3 stores 0 there before `map<int,bool>::find`, so the key is always 0; nothing inserts into the map:
always false. Port: `return false` (keeping the map is optional).

D3. **LevelUpWidget Continue after game over** calls `Board::DeleteSavedGame(app->mGameMode)` then
`app->ShowHallOfFame()` (fame; decomp 0x42bc90), not "back to title" (board_logic 4.1). Otherwise it sets
`mState = BOARD_NEXT_LEVEL`.

D4. **FlyingPieceList multiplies the y step by Piece+0x18**, not +0x20 (board_draw 3.3 is wrong): 0x4212c8 and
0x421328 are both `fmul qword ptr [esi+0x18]`; +0x18 gets +0.003 after the x step (0x4212d1-0x4212da), +0x20
gets +0.003 after the y step (0x421331-0x42133a) and is only zeroed on arrival. Names stay board_logic's:
+0x18 `mFlySpeed`, +0x20 `mVY`. Translate the quirk literally.

D5. **Lex animation 23 is the burp**: Board::Update plays it when `mBurpDelay` counts down to 0 with `mBurpPending`
(decomp of 0x40bd40 lines 897-910, PlaySample(31 lex_burp) at 0x40d4ab), not on a bonus word (lex 2.2). Name
`LEXANIM_BURP`.

D6. **OverlayWidget +0x78 is `mArrowFade`** (screens: Update subtracts 0.025, Draw uses it as the arrows' alpha),
not a slide speed (board_logic 4.1). Board writes 10.0 in ShowOverlay.

D7. **Board+0x1b1 = `mBurnedDown`** (board_logic) for board_draw/screens `mScorched` and lex `mBurnt`.

D8. **Board+0x3c0 = `mSummaryShown`** (the LevelUpWidget is up) for fame `mLevelUpShown`, app "level-up widget
active", lex "paused": WordBalloon's timer and the bonus balloon stop while the summary is shown, not while paused.

D9. **Board+0x3dc = `mPauseNoDialogs`**: UpdatePauseFade sets it while paused to "no dialog and no help widget";
OverlayWidget draws the pause box only then (screens called it "paused").

D10. **App+0x130c renamed `mResumeUpdateCnt`** (app.h: mLastInputUpdateCnt): the only writes are Board::MouseDown's
unpause branch (0x409e2e) and GotFocus (0x419038), both `= mUpdateCount`; MouseDown/MouseDrag/MouseUp ignore input
until 15 updates later. It does not track the last input.

D11. **App flags named individually**: +0x12ec/+0x12ed/+0x12ee/+0x12f0 are the one-shot hint flags of
HINT_WATCH_OUT/HINT_FIRE_ALERT/HINT_GREEN_TILE/HINT_SCRAMBLE (reads/writes at 0x40d09e, 0x40b8ca, 0x40b820,
0x40f512); +0x12ef and +0x12f1..+0x12fb are written only by the ctor. +0x12e4 (`mScrambled`) is written by
Scramble (0x40f18f) and the ctor only: no reader (app called it a hint flag, board_logic "scrambled flag").

D12. **Function names follow the export** (rule 2). The analyses' other names are mapped in 2.12.

D13. **Board drawing helpers**: `DrawWord` 0x407c60 and `DrawSelection` 0x405340 (export, board_draw) over
board_logic.h's DrawLastWord / DrawSelectionArrows; `PieceBelow` 0x403bd0 over board_draw's GetPieceBelow.
board_logic declares them in Board.h with these names; board_draw defines them in BoardDraw.cpp.

D14. **Board helpers in BigLex.cpp's range belong to board_logic** as Board statics: `IsAdjacent` 0x402f60 and
`SetActionSpeed` 0x402fd0 (both cdecl; only Board calls them) and the write-only double 0x52e278
(`gActionSpeedValue`); lex.h's `PiecesAdjacent`, `SetLevelSpeedGlobal`, `gLevelSpeedGlobal` are dropped.
`LevelThreshold` 0x403880 and `RandomLetter` 0x403d10 are cdecl: static members. `ColX`/`RowY` are thiscall
(0x403990, 0x4039b0 take `this`): non-static members (OverlayWidget calls `board->ColX(...)`).

D15. **Lex::IsBusy / IsMoving** (emitted in Board.cpp) are lex's, inline in Lex.h; Board, FlyingPieceList and
LevelUpWidget call them.

D16. **UserManager and User are app's** (UserManager.h/.cpp); screens.h's copy and dialogs' ProfileMgr/Profile names
are dropped. **HighScoreEntry and its predicates are app's** (BookWormApp.h, names `HighScoreByScore`,
`HighScoreByWordLength`, export); fame.h's copy and `ByScore`/`ByWordLength` are dropped; `mIsNew` is `mutable`
(2.9). The predicates take their arguments by value in the original; `const&` gives the same results.

D17. **WinMain, ShutdownWidgets, DrawShadowedString (0x40fe80-0x4100f0) are app's**. board_logic's `EndGame` and
`DrawShadowString` names are dropped. Port: the framework's main creates and runs the app (`main.cpp`, lead); the
app creates `gGameRand` (`new MTRand` + `SeedMTRand2004(*gGameRand, Sexy::Rand())`) as the last step of
`BookWormApp::Init` (WinMain did it right after Init returned; nothing draws Sexy::Rand in between) and deletes it
in its destructor.

D18. **Definitionator::GetDefinition (0x41d2b0) and WordBalloon::Init / SetJustification (0x436260, 0x436310) are
lex's**, although they sit in CreditsDialog's and WaitDialog's ranges.

D19. **No `Math.h` in games/bookworm/src**: on a case-insensitive file system it shadows `<math.h>` for every file
compiled with `-Igames/bookworm/src` (framework/SexyVector.h includes `<math.h>`). The app keeps class Math (dead
tables) in BookWormApp.h/.cpp, or names the files `BWMath.h/.cpp`. Game code uses `<cmath>`.

D20. **Resource, dialog, music and game-mode constants only in Res.h** (rule 4): the group-local enums listed in rule 4
are deleted; the names in section 4 apply (screens.h/fame.h `FONT_LEGACYSERIF12` -> `FONT_LEGACYSERIFMDSCITCTT12`,
fame `FONT_SCRIPPS13BOLD`/15/18 -> `FONT_SCRIPPSCOLLEGEOS13BOLD`/15/18, fame `IMG_*` and lex `LEXIMG_*` ->
`IMAGE_<table name>`, e.g. `LEXIMG_WB_THOUGHT` -> `IMAGE_BACKDROP_WB_THOUGHT`, `LEXIMG_SWEAT_SEQ` ->
`IMAGE_FX_LEX_SWEAT_SEQ`, `LEXIMG_WB_TALK_TAIL` -> `IMAGE_WB_TEXT_TALK_TAIL`, `LEXIMG_BIG_SPEAK_AH` ->
`IMAGE_LEXBIG_SPEAK_AH`). BigLex's frame 0 means "draw nothing": use a literal 0, not IMAGE_BACKDROP_TITLE.

D21. **One `gLetterValues`** (Board.cpp, 0x4f9888); board_draw.h's inline copy is dropped; the app's tile-set builder
uses it too (its own copy at 0x4fa480 holds the same 26 values; 'Q' is forced to 10 by both callers).

D22. **Music calls: the last bool of PlayMusic/FadeIn is noLoop** (FRAMEWORK_2004.md: `FadeIn(id, offset, speed,
noLoop)`). Part 1's music table writes "loop"/"no loop" for noLoop false/true; board_logic and fame give the literal
argument. They agree: game over `FadeIn(3, 61|59, 1.0, true)`; danger `FadeIn(4, 51|65, 0.008, false)`; hall of fame
`FadeIn(3, 54, 0.008, false)`, back `FadeIn(0, -1, 0.008, false)`; loading `PlayMusic(0, 27, false)`.

D23. **OverlayWidget::SaveState / LoadState** (export, screens) over board_logic's Save/Load.

D24. **OverlayWidget+0x60 is `mBalloon`** (screens, owner) over lex's mWordBalloon.

D25. **WordBalloon's 4th constructor argument is the tail position** `Point(159, -2)` (screens open question 7:
`(int)(0.75 * 213.0)` and -2 passed as one 8-byte struct = lex's `mTailPos`). Signature in 3.4.

D26. **WordBalloon::SetJustification is called only by Lex::Update** (export callers), with (0, 0, 0); screens'
"and LevelUpWidget" is wrong: LevelUpWidget calls the balloon's slot 49, `Show(0)`.

D27. **0x404b40 is `Board::ShowSummary`** (export, board_logic); fame's ShowLevelUp and app's "Board::LevelUp
0x404b40" refer to it. `Board::LevelUp` is 0x407a50 (next-level bookkeeping).

D28. **0x403880 is `Board::LevelThreshold`** (fame: GetRankPoints): cumulative score at which level n is complete; the
LevelUpWidget's `GT_SUMMARY_NEXT_RANK` uses it.

D29. **OptionsDialog**: the app's "slot 52 close" is `ApplyAndClose()` (slot 52, new virtual) and "+0x170 slide
target" is `mSlideOffset` (the creator writes 240 = start offset; ApplyAndClose writes -240).

D30. **DeleteUserDialog::DeleteSelectedUser** (export) for app's DeleteSelected.

D31. **0x405cd0 is `Board::Abandon()`** (clear popups, `mState = BOARD_INACTIVE`), called by ShowGameSelector.

D32. **0x403d80 is `Board::ShowOverlay(bool)`** (export) for screens' ShowBonusBanner; it writes OverlayWidget's
mBannerY (when 0), mBannerTargetY, mBannerActive and mArrowFade.

D33. **EffectMgr::AddEffect / HasEffectOfType / GetEffectsOfType** (export, board_draw) for screens' Add / FUN_ names.

D34. **Lex names**: `Reset` (board_logic: StopAnim), `SetSweating` / `mSweating` (board_logic: SetDanger, "danger"),
`SayDefinition` (board_logic: ShowDefinition), `mTargetX/Y` (board_draw: home x/y: Lex::Move sets the target to the
new position, so it is the home only at rest). `Definitionator::GetDefinition` (board_logic: Lookup).

D35. **App method names** in board_logic, fame and dialogs texts map as in 2.12 (RecordScore/AddHighScores ->
SubmitHighScores, PauseMusic -> PauseAllMusic, CloseHelp -> HideHelp, AskLeaveGame -> GoToMainMenu, NewGame ->
StartGame, ShowInternetScores -> IsInternetScoresEnabled, TruncateString -> TruncateToWidth, FormatDuration ->
FormatTime, OpenRegisterDialog -> ShowRegisterDialog, Save/LoadCurrentProfile -> SaveCurrentUser /
LoadUserSettings).

D36. **Board field names**: board_draw's proposals (mSelected, mScorched, mSpinTime, mProgressTarget,
mProgressShown, mWordScore, mWordColor, mWordFade, mBurpSmoke, mBurpCountdown, mBurpSmokeTime, mHeldKey,
mDisplayedScore, mEmberListMgr) are replaced by board_logic's (2.2).

D37. **Piece field meanings** for board_draw's unknowns: +0xb0 mBurning, +0xbc mPendingFire, +0xd8 mFlipping, +0xe0
mFlipProgress, +0xec mBurnCountdown (board_logic). +0x28..+0xa0 and +0xc0 are saved only: neither
FlyingPieceList nor the draw code reads them (board_logic open question 1).

D38. **Not ported**: WaitDialog (only caller of its ctor 0x436110: FameWidget::ShowWaitDialog 0x423fd0), ImagelessButton
(ctor 0x42b500, only caller FameWidget ctor 0x428520, the hidden RESUBMIT button), GetStringDialog (ctor 0x4292d0,
only caller ShowAliasDialog 0x414080, reached only from ButtonDepress 2024 and FameWidget::ButtonDepress case 8)
and LabelWidget (ctor 0x42b8c0, only caller GetStringDialog), RegisterDialog, UpdateCheckDialog. FameWidget does not
create mResubmitButton. See open question 1 for GetStringDialog/LabelWidget.

D39. **StatKeeper +0x24/+0x28** (sapphire/diamond tiles used): never zeroed in the original, read by no module
(LevelUpWidget shows only best/longest word and times): port zero-initialises (board_logic open question 2).

D40. **No other module reads** Board +0x1b2, +0x2b0, +0x2cc, +0x2d0, +0x2ec, +0x3d8, +0x3e8, +0x3ec (board_logic open
question 4); +0x3dc is read by OverlayWidget (D9).

D41. **Board+0x357 `mNewHighScore`** (fame open question 3): written true by SubmitHighScores, cleared by
LevelUpWidget::Update once `mLexArrived` and Lex is not busy (decomp 0x42bee0 lines 60-70); nothing else reads it,
so it has no visible effect. Keep both writes. Board states 5/6 are BOARD_LEVEL_COMPLETE / BOARD_NEXT_LEVEL.

D42. **Lex open questions answered by the board**: board+0x2a8 -> +0x30 is `mGameStats->mScrambles` (number of
scrambles this game); piece+0xe8 == 3 is `mType == TILE_FIRE`; the 7 pointers Lex::MouseEnter reads are the top tile
of columns 0..6 (`mBoard[c % 2 == 0 ? 1 : 0][c]`, [row][col]).

D43. **BigLex during the selector's open transition** (screens open question 6): BigLex has no hide logic (lex 3);
StartCloseTransition hides it, StartOpenTransition does not; it stays visible above the squashed snapshot. Literal.

D44. **SelectUserDialog double-click** switches the user without `LoadUserSettings` (dialogs open question 1): keep;
`StartGameForUser` reloads the settings at every game start, and TitleScreenDone at start-up.

D45. **CreditsDialog and every MoneyDialog subclass derive from Dialog2004** (TRANSLATE_BRIEF), not Dialog.

D46. **Shared listener bodies**: 0x4220c0 (`PlaySample(0)`) is both FameWidget's and HelpWidget's ButtonPress(id);
0x401450 both TitleScreen's and BWGameSelector's ButtonPress(id, cc). Each class implements its own copy.

D47. **The app's tile-set table and board_draw's agree** (grey levels 0/255/0/255/0/176/0/255/176, digit at
(57k + 54 - 15, 53 - 15)); the builder is app code, the strips are drawn by board_draw.

D48. **Sizes**: every class size agrees across the analyses (table 1.1); MoneyDialog's 0x120 is inferred from its
subclasses (never allocated alone).

D49. **Play counting is off in the retail build**: Board::Submit/Scramble count a play only when app mMaxTime == 0
and mMaxPlays == 0 (decomp 0x40f5b0 lines 74-77, 0x40ed70 lines 240-243); MaxTime defaults to 60 and partner.xml
does not set it, so mUserTimesPlayed and mTimesPlayed never grow in play. Translate literally (open question 6).

### 6. Open questions for the lead

1. **GetStringDialog and LabelWidget**: TRANSLATE_BRIEF assigns them to dialogs and fame, but only the internet alias
   prompt (not ported) creates them (D38). Decide: skip them (proposed: no files, a `// not ported` note in
   ShowAliasDialog) or translate them for completeness (self-contained, nothing else depends on them).
2. **Options "more games" link**: partner.xml has NumOptionsLinks = 1, so the original shows a `GT_OPTIONS_MORE_GAMES` button and a 351-pixel dialog; Part 7 proposes dropping it (NumOptionsLinks = 0, 315 pixels).
   Keep the button (faithful look; the framework never opens a URL in test runs; random input must avoid it) or
   drop it? Affects OptionsDialog (dialogs) and the app's OpenURL/dialogs 11/12. Decided by the user on
   2026-10-24: dropped (L2).
3. **gGameRand**: D17 puts its creation at the end of `BookWormApp::Init`; confirm `main.cpp` does not create it too.
4. **Saved-game paths**: app `HasSavedGame` and Board `SaveGame/LoadGame/DeleteSavedGame` must agree. Proposed: both
   build the original relative path `players\<name>\classic.dat` / `action.dat` and go through the framework (VFS
   puts writes of relative paths in the user dir; FileExists/EraseFile); `_mkdir` becomes the framework's directory
   creation. Also: the original's file system is case-insensitive (a new player "bob" picks up "Bob"'s saves); the web
   FS is not. Confirm the helper and the case policy.
5. **Emulated registry semantics** used by UserManager: subkey enumeration order (Windows: case-insensitive
   alphabetical; the roster lists users in that order) and `DeleteUser` deleting the profile only when the key erase
   succeeds (a key exists once SaveUser wrote a value). Framework/lead to confirm `RegistryGetSubKeys` order and
   `RegistryEraseKey` result.
6. **Play counting** (D49): literal translation leaves `GT_SELECTOR_WELCOME_BACK` to registry values from an original install.
   Keep literal (proposed) or set MaxTime = 0 in the port's properties?
7. **GetLastMouseDownTick**: the port's framework had no last-press field (WM+0x168), which Lex::mIdle (saved
   byte 65 of the Lex record) depends on; no visible effect. Resolved, see L7.
8. **mGameOption** (app +0x12dc / selector +0x80): copied back and forth, never read; meaning unknown. Kept.
9. **Dialogs 0 and 3** are killed/handled but never created (app open question 1): dead branches kept literally.

### 7. Lead decisions on the open questions (binding for translation)

L1. **GetStringDialog and LabelWidget are not ported** (internet alias only), like RegisterDialog,
    UpdateCheckDialog, WaitDialog and ImagelessButton. The callers keep a one-line `// 0xADDR not ported`
    note (ShowAliasDialog and its internet callers are not ported either).
L2. **The Options links are not ported** (the user's decision, 2026-10-24; first decided the other way):
    OptionsDialog builds no buttons from `NumOptionsLinks` / `OptionsLinkText%d` / `OptionsLinkURL%d` and
    never opens a URL. The dialog is `OptionsDialogHeight` high (315 px with the retail partner.xml, where
    the original's one link made it 351); every other widget keeps its place from the top, as the help /
    credits row stays 36 px above the footer button. Nothing in the port calls OpenURL any more; the app's
    OpenURL and dialogs 11/12 stay translated.
L3. `gGameRand` is created at the end of `BookWormApp::Init` (`new MTRand` seeded with `SeedMTRand2004(...,
    Sexy::Rand())`); `main.cpp` does not create it.
L4. Saved-game paths: `players/<name>/classic.dat` / `action.dat` relative to the user dir, built the same
    way everywhere (one helper in the app, `GetSaveGamePath(name, mode)`), with the name exactly as stored in
    the profile; reads go through the framework helpers (case-insensitive VFS lookups).
L5. Emulated registry: `UserManager` sorts the subkey names from `RegistryGetSubKeys` case-insensitively
    (Windows' RegEnumKeyEx order). `RegistryEraseKey` returns true when the profile key had values, which a
    profile always has, so `DeleteUser` behaves as in the original.
L6. Play counting stays literal (MaxTime 60: never counts in the retail build).
L7. `GetLastMouseDownTick` (Compat2004) returns `WidgetManager::mLastMouseDownTick` (framework commit
    abfb135), which every WidgetManager::MouseDown sets from `GetClockTime()`; 0 means no press yet. Lex's
    idle test compares it with `GameTickCount()` and treats 0 as idle, as the original's GetTickCount()
    (milliseconds since boot) always did, so the idle byte follows the original and repeats under -fast.
L8. `mGameOption` and the dead dialog-0/3 branches are kept as in the original.

## Coverage check (2026-11-11)

The whole coverage check of the game (`docs/COVERAGE.md`). origrun's `--coverage` recorded which of the original's
functions ran, in 18 runs on the host (`games/bookworm/tools/coverage/`; the hits went to `build/coverage/bookworm/hits/`
and were deleted once judged). Each ran under rundesk with origrun's silent stand-in, so none made a sound, and all but one
on the virtual clock.

- **The identical-play cases** (`tools/coverage/run.sh case` and `save`): Classic seed 1 (`c1`), Classic and Action in one
  run (`c2`), three modes in a row (`m1`), and two saved-game pairs: `s1` (a Classic game quit at update 3000, then continued
  by the bot) and `s2` (Action, quit at 4000, continued), each pair two runs. The input is the port bot's, turned into
  origrun's by `identical/toorig.py`, 600 updates later for the loading.
- **The screen tours** of `tools/shots`: `menus`, `classic`, `action`, and the two focus tours `focusa` and `focusc` (the
  window losing and regaining the focus on the board and with a dialog up).
- **Extra runs for what those miss** (`tools/coverage/tours/` and `extras/`): `users` (five players made through the
  select dialog, the delete dialog with its question answered no and then yes, CANCEL, a double click that switches
  player); `keep` (the same tour started again on the registry the run before left, so the players are loaded at startup,
  `UserManager::LoadUser`); `x1` (a Classic game of the bot with the options dialog opened on the board: both sliders, the
  hints, definitions and cursors boxes, its Help button, the help screen's "don't show again" box and a topic; the keys E,
  Ctrl and Space; and Lex hovered seven times, which makes him think his balloon aloud, then a click on that balloon);
  and `realclock`, the `menus` tour on the real clock, the one run that is not on the virtual clock: it reached 1754 functions, two of them (in the libraries' code, 0x4476b0 and
  0x47f340) reached by no other run, and none of the game's own.
- **Probes for effects in a saved game** (`effects.sh`: a Classic game quit 4 updates after a word was scored, with the
  sparkles and flying tiles up): two runs, see the candidates below. Two more died when the stage's exe was still running as
  the next run began (`run.sh` now waits for it), and the rest of the series was stopped once the code showed why the
  first two found nothing.

Every run ended with the game exiting normally.

`games/bookworm/coverage.txt` sets the game's own code as one range (0x401000-0x437510, from `BWGameSelector`'s first
function to `WordBalloon::SetText`; what follows is the framework, the C runtime and the libraries). It holds 576
functions, and the runs reached 467 of them.

**Reached in the original with no counterpart in the port: 23 at first, none at the end.** None was missing game logic.
They were compiler- and library-made code and stubs: a `ButtonListener` stub, five std::string helpers, three empty or
constant stubs, a copy constructor of a high-score entry, `ios::lock` and `unlock`, `WinMain`; the hidden `ImagelessButton`
of the online scores (three functions); `EditWidget::MarkDirty`, which the linker shares with `LabelWidget` and which only
picks how the widget is marked dirty; `Rect::Contains`, which the framework's `Rect` replaces; and four slots of
`WordBalloon` that only jump to `Widget`'s. Each is now a `none` line in coverage.txt with its reason, among the 78 lines
it holds (the Catch funclets of the exception handler, the registration and update-check code taken out earlier, the online
high scores and the C++ library's code). Seven of the lines are for functions the port names but nothing in the original
calls from play: the deleting variants of four destructors (`WordScore`, `Math`, `Effect` and `MoneyDialog` objects are
never deleted through their vtables), `FameWidget::DrawInternetScores`, `GetGameSEHInfo` (the crash report's text) and
`TitleScreen::ShowTrialExpiredDialog`.

**Defined in the port, never reached: 49 at first, 19 at the end.** The extra runs reached 23 of them: the
`DeleteUserDialog` (nine functions), `BookWormApp::ShowDeleteUserDialog`, `SelectUserDialog::ListClicked`,
`UserManager::DeleteUser` and `LoadUser` (by `users` and `keep`); `Board::KeyChar`, `KeyDown` and `KeyUp`,
`OptionsDialog::SliderVal` and `CheckboxChecked`, `HelpWidget::CheckboxChecked`, `Lex::Think`,
`WordBalloon::SetJustification`, `MouseDown` and `DrawPlain` (by `x1`). Seven more became `none` lines (above). The
options dialog's fullscreen box and its 3D box were left alone:
the footer button applies them, and either switches the display mode. The 3D box's own branch in
`OptionsDialog::CheckboxChecked` is a few lines of a function the other boxes reached.

**The 19 left.** No play of the original reaches them. Each was read again against the export and matches it. They were
the candidates for taking out unneeded code, and came out in the step after (see "Code taken out", below):

- **Effects in a saved game, 9 functions**: `Effect::Save`, `Effect::Load`, `Sparkle::Save`, `Sparkle::Load`, `Ember::Save`,
  `Ember::Load`, `FlyingPieceList::Save` and `FlyingPieceList::Load`, and the 4-byte `Effect::GetType`. `Board::SaveGame`
  (0x406c40) starts by clearing the effect, ember and overlay lists, so what `EffectMgr::Save` and `EmberListMgr::Save`
  write is always a count of zero. Both probes reached `EffectMgr::Save` and none of these.
- **Links, 3 functions**: `BookWormApp::OpenURL`, `URLOpenSucceeded` and `URLOpenFailed`. Only a link button leads to them,
  and the options dialog's "more games" link and the other links are out. The port's dictionary link (a mod) calls
  `OpenURL`, which would then go to the framework's.
- **Nothing calls them, 7 functions**: `WordScore::CalcScoreByLength` (the other scoring method of `WordScore`, which no
  mode selects), `ColorizedButton::SetImageColor` and `SetImageColors_3`, `UserManager::SetCurrentAlias` and
  `GetCurrentAlias` (the online scores' alias), `Font::GetLineSpacingOffset` (the 4-byte override in the definitions' font
  wrapper) and `HelpWidget::OrderInManagerChanged` (the help widget is added and removed, never moved in its manager).

**Never reached and not defined in the port: none.**

**The list box question** (from the coordinator, about the 2004 `ListWidget` setting its `mPageSize` through a float, a pixel
off): BookWorm has no scrolled list box. Its only lists are the select-player and delete-player lists, each sized to its
lines by `GetOptimalHeight` (the line count times the line height, plus 8), so the page size is an exact integer, no
scrollbar shows and `mPosition` stays 0.

**The Collection check.** The port was made from the retail 1.03 install (`C:\Program Files\PopCap Games\BookWorm Deluxe`:
`Bookworm.exe` 1,425,425 bytes, the game carved out of it 1,318,912), and the user's PopCap Game Collection holds the game as
the group `BookwormDeluxeComponent` with a `Bookworm.exe` of 1,770,800 bytes: another build. The port reads its texts from the
exe, so the check against that version applies after every other step and before the site features (coordinator's correction,
2026-11-11; AstroPop's GAME_NOTES section 13 is the model). It is not done yet.

**Not covered, and why.** The options dialog's fullscreen and 3D boxes (applying either switches the display mode), every
link, "buy" and "more games" button (a script never clicks one), the Hall of Fame's internet tab (needs the network, which
origrun makes fail at once), and the original's 3D mode as such.

**What it cost.** The runs took about an hour of machine time, 2 or 3 game processes at a time. No agent was used: this
session wrote the tools (`run.sh`, `all.sh`, `port.sh`, `extra.py`, `effects.sh`), read the reports and placed the `none`
lines.

## Code taken out (2026-11-11)

The step after the coverage check and the performance pass: what the game does not need comes out of `games/bookworm/src`.
Each removal below is one the original's own code allows (nothing in the port can reach it), and the frozen baseline proves
it (`tools/check.sh bookworm`: the same state at every update in the two-mode, three-mode and saved-and-continued games;
`build-web:bookworm` builds). Every function that came out has a `none` line in `coverage.txt` with its reason, so the
coverage report lists no function the port defines and no run reaches (`coverage.txt` holds 100 `none` lines now, the
78 of the coverage check and these 22).

**The 19 functions of the coverage check.**

- *Effects in a saved game, 9 functions*: `Effect::Save` and `Load`, `Sparkle::Save` and `Load`, `Ember::Save` and `Load`,
  `FlyingPieceList::Save` and `Load`, and the base `Effect::GetType` (now pure: no object is a bare `Effect`).
  `Board::SaveGame` clears the effect, ember and overlay lists before it writes, so `EffectMgr::Save` and
  `EmberListMgr::Save` (both kept) always write the random generator's state and a count of 0, and the records that follow a
  count never exist. `Save` now writes the 0 and `Load` reads the count and nothing after it; a file the game wrote loads
  as before (the s1 and s1b games of the baseline save, quit and continue). The file keeps its format byte for byte
  (checked below). Taken out with them: `Sparkle::mUnknown3c` (a flag only the save code touched), the `EFFECT_NONE`
  value and the `ReadBool` and `WriteBool` helpers of `Effect.cpp`. `Piece::Save` and `Load` stay (the board's grid).
- *Links, 3 functions*: `BookWormApp::OpenURL`, `URLOpenSucceeded` and `URLOpenFailed`, with dialogs 11 and 12 (the wait
  message and the URL to open by hand), their three texts in `GameTextTable.inc` (`GT_OPENING_BROWSER`,
  `GT_OPEN_BROWSER_HEADER`, `GT_OPEN_BROWSER_LINES`; `tools/make_gametext.py` writes the table from a git-ignored working
  list, so a rerun from a list that still has them brings the three rows back) and the two names in the test hooks'
  `DialogName`. The port's dictionary link (a mod; a left click on a word's balloon) now calls the framework's `OpenURL`
  directly: it no longer shows the original's "opening your browser" message while the framework opens the page. Nothing else
  of the game leads out of it.
- *Nothing calls them, 7 functions*: `WordScore::CalcScoreByLength` (the formula `mScoringMethod` 0 selected; the field
  itself, always 1, and the table `gLengthBonus` it used came out, and `WordScore::Build` now calls `CalcScore` alone),
  `ColorizedButton::SetImageColor` and the three-value `SetImageColors`, `UserManager::SetCurrentAlias` and
  `GetCurrentAlias` (the alias stays in a user's registry record), `Font::GetLineSpacingOffset`, which is
  `Definitionator::GetNumDefinitions` at the same address (the linker folded the two), and `HelpWidget::OrderInManagerChanged`.
  The framework calls that hook only from `BringToFront`, `BringToBack`, `PutBehind` and `PutInfront`, and the game puts
  only the board, Lex, the overlay and its balloon, the journal and the selector through them: the help widget is added to
  its manager and removed, never moved.

**Dead state and branches found on the way** (nothing wrote or read them, or only their own copies):

- The `Math` class (the sine and cosine tables embedded in `BookWormApp` that "nobody reads"; its constructor and
  destructor are the two `none` lines 0x42e330 and 0x42e390) and the app's fields: four image pointers, three unused flags
  and eleven unused hint flags, two unused strings, `mScrambled` (set by every scramble, never read), `mInitDone`,
  `mInternetOK`, `mSEHInfo` and `mUsedBonusWords` (an insert on every bonus word, never read).
- `BookWormApp::IsInternetScoresEnabled` (always false; only the Hall of Fame's internet button, which is not ported,
  asked it) and the button handler's cases for the dialogs of the internet and update-check features, which no code
  creates (6 and 7, the internet alias 23, 24 and 25, and the framework's update dialogs): each did only what the `default`
  case does, a `KillDialog` of itself. Their ids (and 8, 26, 27, 10008 and 20008) went from `Res.h`, where a comment
  names the numbers.

**What stays, and why.** Saved-game fields the original writes (the count fields above, `Board`'s unused fields, the
`Alias` of a user), so that a file stays the original's format and an original's saves load. The app's `mGameOption` and the
selector's copy of it, which only copy each other, wait for the readability pass, which has to name or remove them.

**What it came to.** 22 functions, the `Math` class with its member and 12 other fields, a table, 3 data rows, 8 dialog
cases and 12 dialog ids: 496 lines of source out and 85 in (mostly comments), no behaviour changed but one wait message
of a mod.

**The saved file.** The s1 session of the baseline test (a Classic bot game quit at update 3000, so the game saves as it
shuts down) was run with the frozen baseline exe and with the current one, and the two `classic.dat` files (20,883 bytes
each) compared: they are the same but for one or two low bytes of a value 169 bytes from the end, at offset 20714. Two runs
of the baseline itself differ at the same place by as much, so it is a value that changes from run to run (not looked into:
the state log of every update is identical in all four runs), and none of the bytes that the effect and ember code wrote
differ.

## Performance pass (2026-11-11; `docs/COORDINATION.md`, "A performance pass before the readability pass")

**Method.** `games/bookworm/tools/profile/` holds the tools: `perf.py` (samples the game's process every 0.1 s: working set, private
bytes, processor time; the load is the time from the creation of the process to the title's play link, the port's through its `move`
command, the original's from the title's `mLoadDone` flag read out of its memory), `pairs.sh` (the port and the original turn about,
one process at a time, each waiting for a free game slot), `summary.py` (a median with its range), `variants.sh` (the port with SDL's
renderer settings changed), `memrun.sh` (the port's `memreport` at four points) and `cpuprof.py` (Bejeweled's sampler, with the system
DLLs named by their export tables). The game played is a Classic game of the port's bot (seed 1, 7,060 updates: the title's link, the
new player's name, a Classic game to its end, back to the selector), the same script for both: the port as `-script` lines, the
original through origrun (`identical/toorig.py`) on its real clock. "The stretch" is the 2,000 updates before the last input, 20 s
of play. The machine was never quiet (0.54 to 0.90 of it busy, 3 to 5 other game processes), so a figure is three pairs, with its range.

| Three pairs, real time | Original | Port |
|---|---|---|
| Load to the title | 2.71 to 3.69 s (median 3.02) | 1.21 to 1.81 s (median 1.51) |
| Processor time to the title | 1.08 to 1.42 s | 1.11 to 1.38 s |
| Private bytes at the title | 38.9 to 41.0 MB | 77.7 to 79.9 MB |
| Processor time of the stretch (20 s) | 3.73 to 4.59 s (18.6 to 22.9 % of one core) | 1.91 to 2.39 s (9.5 to 11.9 % of one core) |
| Updates a second in the stretch | 99.9 to 100.0 | 97.8 to 99.3 |
| Peak private bytes of the run | 41.0 to 41.2 MB | 95.8 to 97.9 MB |

Without waiting (the port's `-fast`, origrun's virtual clock: the work a test run does) the whole run took 1.73 s of processor time in the
port (one run) against 3.75 and 5.08 s in the original (two), and the stretch 0.23 s against 0.39 and 0.62 s. A first real-time run of the
original that read 0.84 s for the stretch (its load took 40 s on a busy machine) was dropped; the three pairs agree with each other.

The port loads in half the original's time on the same processor time (it loads in one go, as the framework does for every
SexyAppFramework game) and plays on about half of its processor time. The one figure it loses is memory: 39 MB more at the title and
55 MB more at the peak.

**Where the port's processor time goes** (`cpuprof.py`, 21.5 G cycles in 77.8 s of the real-time run, every thread). ntdll 85.7 %, the exe
4.9 %, the Direct3D 11 driver 2.7 %, the C library 2.4 %, win32u 1.9 %, `imagehlp.dll` 0.7 %. The sampler charges the cycles a thread used since
the last look to where the thread stands, so a thread that works for a moment and then waits is charged to its wait: `ZwWaitForSingleObject`
60.9 %, `ZwDelayExecution` 11.0 % (the main loop's `SDL_Delay`), `ZwWaitForWorkViaWorkerFactory` 8.2 % (idle pool threads). What it does show
is whose cycles they are: the main thread 40.6 % (72 % of it in ntdll, 11 % in the exe), the graphics driver's own thread 26.0 %, one more
thread 9.5 % and fifteen others of 0.1 to 2.9 %. In the exe nothing of the game's own code reaches 0.2 %: its largest entries are the load's
decoders (`Sexy::Quantize8Bit` 0.5 %, `_inverse_mdct` 0.3 %, `_stbi__out_gif_code` 0.3 %, the module reader `ReadMO3` 0.2 %). What a
player's machine spends on this game is the display path, not BookWorm's code.

**Renderer settings** (`variants.sh`, two runs of the stretch each, real time): the share of one core was 8.3 to 15.8 % with the default
(Direct3D 11, vsync), 12.8 to 15.5 % with vsync off, 9.7 to 13.4 % on Direct3D 9, 6.5 to 9.7 % on OpenGL and 82 % on SDL's software renderer.
OpenGL's lower figure came with one run at 56.8 updates a second and a peak of 143 MB, so no setting beats the default clearly and nothing
is sent to the coordinator about the renderer.

**Where the port's memory goes** (`memrun.sh`: the framework's `memreport` at updates 400, 1,500, 3,500 and 6,800 of the scenario). Private bytes
were 86.3, 91.7, 95.2 and 104.3 MB. The libraries and devices before the first picture (`privateAtStart`) are 32.3 MB; the pictures are 176 to 184
(10.6 MB of pixels, 0.8 MB of copies; the graphics device's textures an estimate of 2.5 MB at the title, 8.5 MB in the first game and 16.5 MB at its
end); 47 sounds are 4.5 MB and the 5 module files 1.1 MB. The original's whole process is 39 MB at the title and 41 MB at its peak, so the
port holds about 32 MB of devices and libraries that the original does not (the Direct3D 11 device is 20 to 25 MB of it) and about 10 MB of
pixels kept beside their textures. Both are the shared costs of `docs/COORDINATION.md`.

**The web build** under Node (`-fast`, the same scenario): the wasm memory was 56.6 MB at the first report, 68.0 MB in the first game and
81.6 MB at its end (it only grows), and 7,060 updates with 627 draws took 3.76 s, about 0.5 ms an update without a display. The load and
the frame time in a browser were not measured: the Browser pane is hidden and its timers are throttled, so `requestAnimationFrame` does
not run (as Bejeweled's notes say).

**Shared costs** (none of them in BookWorm's code, and the coordinator's list holds each): the GIF decoder and `Quantize8Bit` lead the
exe's load costs (276 GIFs); `imagehlp.dll` 0.7 % of the real-time run; the Direct3D 11 device and the pixels kept after upload in memory.

**Nothing was changed in the game's code for speed.** The port is already ahead of the original on load and on processor time, and the
profile shows no function of the game's own above 0.2 %; the memory it holds more is the display layer's. The frozen baseline is
unchanged by this pass, which added only the profile tools and these notes.

**To repeat.** `OG_STAGE=C:/byo-og/bwpp games/bookworm/tools/identical/setup.sh` makes the original's stage; copy `games/bookworm/tools/profile/scenario.txt` (the
port's `-autoplaylog` of the bot game, shifted as `identical/case.sh` does) to `<out>/scenario.txt` and run
`OG_STAGE=C:/byo-og/bwpp bash games/bookworm/tools/profile/pairs.sh <out> 3` (about 4 minutes a pair), then
`python -I games/bookworm/tools/profile/summary.py <out>`; `variants.sh <out> 2` and `summary.py <out> v` for the renderer settings.
