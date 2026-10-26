# Bookworm Adventures Volume 2: where the port stands and what milestone 2 needs

Written 2026-10-26 at the end of milestone 1, from what was measured on the Ghidra export in `re/bookwormadv2/gh/`.

## What was found while trying Volume 1's game code on Volume 2's files

`-DBWA2_GAME=ON` builds `bookwormadv2.exe` from `src/`, which is still Volume 1's code. Running it on the staged
files got this far, each step a fix that is now in the tree:

1. It stopped at once, silently: `main.cpp` looked for `bookwormadventures.exe` (Volume 2's is `BookwormAdventuresVol2.exe`)
   and showed an error box, which nobody can close on rundesk's private desktop. The box is now skipped when `-script`
   is given.
2. `properties/partner.xml` (the only XML file of the game with a UTF-8 byte order mark) failed with "Element Not
   Expected": the framework's XMLParser left the mark in the stream. Fixed in the framework (a `shared:` commit);
   the original handles it in its `EncodingParser`, a class Volume 1 lacks.
3. `Image resource not found: IMAGE_MM_ADV_COMPLETE`: `src/Res.cpp` and `Res.h` were Volume 1's. They are now
   generated from Volume 2's `properties/resources.xml` (`tools/resgen.py`: 550 resources in 9 groups, the same group names
   as Volume 1). With them the game's C++ no longer compiled: 28 uses of ids Volume 2 does not have. `TitleScreen`,
   `ArenaWinDialog` and `Board` (the film reel, the title's "Deluxe" picture and the ship pictures of the Flash
   minigame) are reworked; `MainMenu` was rewritten from Volume 2's code (below); `BookSelectScreen` was ported later (below).
4. Four more start-up failures, found by running the game headless (`-script`, see below), each fixed in the shared
   framework (a `shared:` commit): the framework's `DescParser::Unquote` did not match Volume 2's (its fonts write a quote
   as `'''` and `"\""`: the original keeps the backslash in the token and `Unquote` takes the next character literally,
   and skips the closing quote; the framework dropped the backslash and counted the closing quote, which only worked by
   luck), the font descriptor commands `LayerSetExInfo` and `LayerSetAlphaCorrection` are new (the first is checked and
   dropped, the second is kept in `FontLayer::mAlphaCorrection` but not used by the drawing: the original adds it to a
   glyph's alpha before a 256-entry table in its draw routine at 0x4eeb10), and `properties/partner.xml`'s byte order
   mark reached the parser through `PropertiesParser::ParsePropertiesBuffer` (the earlier fix covered `OpenFile` only).
5. `StartIntro` selected book 1; Volume 2's selects book 0 (0x43de80), as it has no books 1 to 3 (its books are 0, 4, 5, 6
   and the timed mode).

## MainMenu as Volume 2 has it (rewritten from 0x4690xx-0x46dxxx)

Seven items: options, quit, adventure, arena, high scores, mini games and tome; the film reel is gone. The adventure,
arena and mini games are PopAnim animations (`images/pam/mm_adventure.pam` and `mm_adventure_replay.pam` for a finished
adventure, `mm_arena.pam`, `mm_minigames.pam`), driven by labels (`BookLabel()` picks the book's label), the others are
cel images with a hit mask (quit and high scores have a mask image, the rest use the cel's rectangle). The tome shows
when `CurrentBook` is set and above 4; arena and mini games when their profile flags are set (`RefreshUnlocks`). The
tooltips are made but the original never draws them (inferred, not proven). The replay of a finished adventure is in
(the next sections). Not ported yet: the locked-tome dialog and the trial dialogs. `KeyChar` is Volume 2's (0x46deb0): ten words (SEATTLE, MOUSTACHE, LOVELY, GOBBLE,
CELEBRATE, PIRATE, WINTER, ELITE, PANCAKES, CONE), each calling `main.SecretUnlocked` with its hint text, Lex's animation for
six of them, `main.SetHatMode` (hats 5, 4, 6, 7 and 3 for SEATTLE, MOUSTACHE, CELEBRATE, PANCAKES and CONE) and, for ELITE,
`main.SetLeetMode`; the slash plays the "unlocked" animation of the arena and the mini games; the ninth `_CompleteBook`
case of the original's switch cannot be reached (its table holds ten words), so it is left out; `StartArena`, `ContinueArena` and the user-dialog functions are still Volume 1's and have to be
compared with 0x46c5d0, 0x46c4b0, 0x46cb30, 0x46cde0 and 0x46d130.

## What runs now (2026-10-26)

Headless, `build/native/rundesk.exe build/native/bookwormadv2.exe -gamedir=assets/bookwormadv2 -userdir=<empty> -script=<file> -fast`
(script lines `<update> click <x> <y> | screenshot <png> | key | char | quit`; the first run makes `users/Lex.bwa`, later
runs with the same userdir reuse it). Seen in screenshots: the PopCap logo, the title with its curtain, the New User
dialog (OK at (400, 435)), the intro cut scene, then the prologue battle of book 0 ("Is it Stuffy In Here?": Lex,
Stuffy, the tile board, the conversation panel; clicks at (400, 300) page through the conversation), the in-game Menu
dialog (button at (650, 580): Options, Quit To Main Menu at (405, 313), Return To Game) with its "Are you sure?" (YES at
(330, 389)), and after it the main menu of Volume 2: the logo, the adventure portal, Lex with his welcome bubble, Hall
of Fame, Quit, Options, the Tome; arena and mini games show only once the profile unlocks them. The Options dialog
(Options at (110, 520)) and the Hall of Fame (at (655, 410), its three tabs and ten default entries) draw as well. No
tile has been clicked yet.

Clicking the adventure portal of that profile (quit in the prologue, so book 0, chapter 1, stage 1) resumes the prologue
battle (`InitSavedGame` selects the book and the chapter, `StartGame` loads the chapter's resources). It failed once with a
script error in `CreateArenaAnim` (`book0.luc:278`): the scripts name a resource group as "Book0Chapter1" and the resources
file as `book0Chapter1`, and the framework's `DeleteResources` and `GetNumResources` compared the names with case, so
quitting to the menu left the chapter's images loaded (while the Lua side forgot them), and the next load skipped them and
exported nothing. The group names are now compared without case (`shared:` commit; the group list was already).
The Hall of Fame's default names are Volume 1's code path: compare with Volume 2's `HallOfFame` (6.1 KB) too.

Compared with Volume 2 (2026-10-26): `StartArena` (0x46c5d0), `ContinueArena` (0x46c4b0), `FillUserList` (0x46cde0) and the
user dialog's handler (0x46d130, its OK, close, new, delete and delete-confirmed branches) do the same steps in the same
order as the port's Volume 1 derived code; the new-name function 0x46cb30 and the Volume 2 offsets of the app's fields
(the timed-mode flag is +0x9aa there, the replay flag +0x9ab, ELITE's +0x95b) have not been compared line by line. The
header comments of `GameApp.h` that give offsets are Volume 1's, except the new `mBookSelectScreen` (+0x800) and
`mReplayMode` (+0x9ab).

## The curtain's states in Volume 2 (CurtainDone, 0x442700)

The port's enum had Volume 1's numbering for the title screen's curtain (9); in Volume 2 that is 14 (0xe, set by 0x442430,
called from 0x48a7a0), and 9 is the replay of a finished adventure. Volume 2's states: 1 cut scene, 2 opening, 3 saved game
(0x4408c0), 4 game to menu (0x443b70), 5 treasure (0x443be0), 6 and 7 to menu (0x443a90, 0x443ad0), 8 new game (0x443950),
9 replay (0x4439b0), 10 the book chosen on the replay's screen (0x443a00, called by `BookSelectScreen`'s button handler
0x4254e0), 12 the Hall of Fame from a replay (0x443a40), 14 the title's curtain. All of them are ported now (9, 10 and 12
as `TRANSITION_REPLAY`, `TRANSITION_REPLAY_BOOK` and `TRANSITION_REPLAY_HOF`; the starters are `StartReplayTransition`,
`StartReplayBookTransition` and `StartReplayHallOfFameTransition`), and so is state 8's replay branch: when the profile's
`AdventureComplete` is set and the timed mode is off, `ReplayCurrentBook` is read in place of `CurrentBook`, and
`main.SetReplayMode(true)` and `SetScoreFile` are called, and the replay flag (+0x9ab, `mReplayMode`; the timed flag is
+0x9aa) is set. States 4, 6 and 7 clear the replay flag and call `main.SetReplayMode(false)`; 6 and 7 also remove the
`BookSelectScreen`. State 9 puts the screen under the curtain (`mFromTitle` of the main menu is false, and unlike state 8 it
does not reset the nuke flag of the profile), 10 is state 8 without the timed block after the screen is removed, and 12 is
state 4's removals and a `HallOfFame` (`MODE_ADVENTURE`, the end-of-game flag false) with `mFromArena` set, which `bookmanager.luc` reaches
through `DoReplayToHOFTransition` when a replayed book ends. `BookSelectScreen`'s ids 0 (back), 1 (options) and 2 (begin) are
the ids of `BookManager`'s back, continue and options buttons: `ButtonDepress` decides by which of the two screens exists (the
original does the same). Begin (`StartReplayOfSelectedBook`, 0x4254e0) writes the `Replay*` keys of the picked book and
starts state 10; the picked book is 4, 5 or 6 (4 when none). `ResumeSavedReplayGame` is the "Yes" answer's path when
`ReplayInGame` is set: `InitSavedGame(true)` on the book manager and the saved-game curtain. A finished adventure's click on the portal (MainMenu::MouseUp, 0x46b3b0, case 2) does: with a
replay in progress (`ReplayInGame`) the "Continue your current adventure?" dialog (id 0x1a, "CONTINUE?"); with none,
`main.SetReplayMode(true)`, `ReplayBookComplete` set to false if absent, the menu disabled, and either the same dialog (when
`ReplayCompletedChapters` exists and `ReplayBookComplete` is false) or the state-9 curtain, which opens the
`BookSelectScreen` (class at 0x423e50..0x425630, script `bookselectscreen.luc`). The profile's booleans read as false when
the key is missing (0x47aca0 with its flag: the port's `ProfileBool`).

## Bound methods of Volume 2's GameApp (the registrations of 0x43a850 against `GameAppLua.cpp`)

Missing: `DoHighScoreScreenFromReplay`, and four more whose names the registration code takes from its own strings and
still have to be read (`DoReplayToHOFTransition` and `DoCSToMiniMenuTransition` are in now). `LoadMusic` (0x4448c0, the
file is `music\<name>.ogg`), `PlayMusic(id, offset, fadeOthers)` (0x445290) and `UnloadMusic` (0x4452f0) are in, over the
framework's `MusicInterface` by song id; Volume 1's tracker-module `SongPlayer` is not made (`mSongPlayer` stays null) and
`PlaySong(id, speed, loop, flag)` is the inlined `IsPlaying`, `FadeOutAll`, `PlayMusic` of 0x43df46 (its fourth flag was
not found; the implementation behind 0x44c490 has not been located). The ids the port's C++ plays (0x8c for the menu and
the intro; the credits', mini games' and stats book's are still Volume 1's order numbers and play nothing) have to be
checked against Volume 2's code.

## Why the exact-match counts look worse than the survey's

`tools/funcmatch.py` (masked byte streams) pairs only 539 of Volume 2's 1,984 game-region functions with Volume 1's
(79 KB of 557 KB), and for the game's own classes next to nothing: MainMenu 0%, Board 0%, BookManager 0%, HallOfFame
0%, GameApp 1%, ConversationPanel 7%; the helper classes are better (FadeButton 90%, ArrowHint 76%, TooltipWidget 66%,
OptionsDialog 36%). The later compiler's register passing changes the bytes of nearly every function, so "unmatched"
here does not mean "changed", and the survey's 28% identical / 19% near-identical figures need a looser comparison to
reproduce. The same holds for `luaV_execute`: of its 45 handlers, 17 match Volume 1's text once names are masked and 28
differ in the text; the two looked at (NEWTABLE and case 37) differ only in an inlined GC check and a shared label.

## Size of the game region by class (bytes of code, thunks left out; `none` = methods the export assigns to no class)

`(none)` 391 KB in 1,468 functions (STL, library and non-virtual methods; most of the game's C++ is in here, as the
export's class recovery is by RTTI only), `MainMenu` 24 KB, `GameApp` 20 KB, `Board` 14 KB, `ConversationPanel` 11 KB,
`Credits` 8.6 KB, `MinigameMenu` 8.5 KB, `HallOfFame` 6.1 KB, `ChapterLoadScreen` 5.9 KB, `BookSelectScreen` 5.9 KB (new),
`TitleScreen` 4.7 KB, `NewUserDialog` 4.3 KB, `MyFlashWidget` 4.1 KB, `BookManager` 3.5 KB, `StatsBook` 3.2 KB,
`OptionsDialog` 3.1 KB, `TreasureScreen` 2.8 KB, `SecretListWidget` 1.2 KB (new), `UpsellTrailer` 1.5 KB (left out).

## The order that follows (milestone 2: the title, menus, dialogs, cut scenes)

1. Read Volume 2's `TitleScreen`, `GameApp` (31 bound methods; its start-up in `GameApp::Init`), `MainMenu` and
   `BookSelectScreen` with `tools/decread.py` and rework the port's classes of the same names until the title and the
   main menu draw; `GameApp::LoadingThreadProc` and the first `main.*` script calls are the path to follow.
2. (done) `tools/gametext.py` and `tools/gtmap.py` for Volume 2: `GameTextTable.inc` has 238 entries; 41 addresses of the
   port's sources still have no Volume 2 counterpart (`tools/gt_todo.txt`).
3. `tools/bindings.py` finds 1 method in Volume 2 (the registration code is compiled differently, with register
   passing); until it works, the 233 bound methods have to be read from `fnfind.py -s <method name>` on the
   `MethodWrapper` RTTI names, or the scripts' calls compared with `src/GameAppLua.cpp`'s table (a script calling an
   unbound method is a runtime error with its name).
4. Cut scenes: `swf/` lacks bitmap fills and the tags Volume 2's five movies use (DefineBits, JPEGTables, DefineBitsJPEG2/3,
   DefineBitsLossless/2, bitmap fills); `bwa2_swftest` lists them per movie as "unsupported".
5. PopAnim: all 304 `.pam` files load (versions 1: 36, 2: 2, 3: 266) but `bwa2_pamtest` reports 41 pictures not found in
   7 files; whether version 3's records are read as the original reads them has to be checked against the PopAnim
   loader at 0x510c00 before the battles.
6. Web page (`web/page.js`): markers, import folders (`music/`, `scripts/` are loose in Volume 2) and prose still say
   Volume 1; its persist folder is already `/bwa2persist`.

## Found while testing the replay (2026-10-26), for the battle milestone

`TileEngine::ValidWordSpelled` passes `(is_valid, count, tiles...)` to the script, but Volume 2's `TileEngine.lua` reads
`is_noun`, `is_adjective` and `is_verb` after `is_valid`; with a new profile the tutorial's first tile click (the tutorial now shows,
as the profile defaults are Volume 2's) stops with `TileEngine.lua:856 attempt to index local ttable`. The original's 0x49b1e0 is
to be read for where the three flags come from (they are the word list's part-of-speech marks). Until then the game runs headless
with a profile whose `NeedsIntroTileHelp` is false (the recipe: an existing user's folder and `lua profile.Set(...)` script lines).
