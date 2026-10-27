# Bookworm Adventures Volume 2: where the port stands and what milestone 2 needs

Written 2026-10-26 at the end of milestone 1, from what was measured on the Ghidra export in `re/bookwormadv2/gh/`.

## What was found while trying Volume 1's game code on Volume 2's files

The build (no option needed since 2026-10-26) makes `bookwormadv2.exe` from `src/`, which was still Volume 1's code then. Running it on the staged
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
(the next sections), and so are the locked tome's message ("NOT YET!") and the arena's TM_Invalidated warning. The trial
dialogs ("You have reached the end of your free trial!", dialog 0x16, four places in `MouseUp`) are left out on purpose:
the check (0x447b50) is false for the marketing modes 0, 1 and 4 that the start-up derives from `properties/partner.xml`
(`MarketingMode` "60Normal", "60Trailer", anything else; "Hydra" and "HydraTrailer" are 2 and 3, the trial wrappers), and a
Steam copy has no `partner.xml`, so it is mode 4. `KeyChar` is Volume 2's (0x46deb0): ten words (SEATTLE, MOUSTACHE, LOVELY, GOBBLE,
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
The Hall of Fame's ten default entries per list now come from Volume 2's builders (0x455890 powers, 0x4566e0 arena times,
0x457840 bonuses): the arena's times carry stage 2 or 1 (two bosses), not Volume 1's 11 to 1. The names and the title's
LOADING letters that `gametext.py` could not map are found by their Volume 2 addresses. The loading screen's hints are Volume 2's own list (0x425c60: 88, not Volume 1's 27). The unlock messages (0x43e440: the tome, mini-games and arena pages, in that order of precedence; Volume 1's
expansion-boss and film-reel pages do not exist in Volume 2), the new-user dialog's "CANCEL" and the profile format's
change list now read Volume 2's texts. Left in `tools/gt_todo.txt`: only the credits' lines (to be rewritten) and the Astro Pop
flash's word.

The cut scene is Volume 2's now (`CutScene`, 0x436560-0x436e60), not Volume 1's click-then-confirm. A click anywhere reads
the label the movie is at (the five movies label each scene "Nstart" and "Nend"), takes its first character and jumps
the movie to the label that character and "end" make, so a click skips the rest of the scene it is in
(`SwfMovie::GetCurrentLabel` and `GotoLabel`, the Flash control's TCurrentLabel and TGotoLabel, with a self-test in
`bwa2_swftest -selftest`). The hint "click anywhere" is not written, and a "Skip" button is made, hidden, and enabled once
the curtain is open (it is never shown, so a movie is skipped scene by scene only). `End` pauses the movie first. The
handler's skip-movie dialog (id 20) is as Volume 2 left it, but nothing opens it. Tried headless: a click in scene 3 of
`book4intro.swf` went to "3end" and the movie played on into scene 4.

The mini-games' menu is Volume 2's now (`MinigameMenu`, 0x470120-0x472530, 0xfc bytes), not Volume 1's. A mini-game has
a button only if the player has unlocked it: all once `AdventureComplete`, else by `CurrentBook` (MutaWords from 6,
GoldenCoins from 5, any other from 4), the buttons' ids being their places in the list (not the mini-game's number),
and the first button is 90 under the top of the box (Volume 1: by the number of mini-games). The mini-game that book
has just unlocked (WordUp in 4, GoldenCoins in 5, MutaWords in 6) gets the "new" mark of `IMAGE_MINIMENU_NEW`, drawn
by `DrawOverlay` over the curtain once it is open. The menu has two openers that set the app's flag +0x95a: the main
menu's mini-games item (0x443c20, flag on) and the scripts' `DoCSToMiniMenuTransition` (0x443cc0, flag off). With
the flag on the menu has the main menu button and the "Show helpful tooltips" checkbox (a `Checkbox` of id 124 at
(745, 558) on a `IMAGE_MINIMENU_TIP_BG` panel at the bottom right; the profile's `ShowMGTips`, ticked and written at
the first visit); with it off both are hidden and disabled (0x4458b0) and a mini-game is started by
`main.StartMinigameFromCS` instead of `main.StartMinigameFromMainMenu` (0x472330). A mini-game button sets the app's
`mMiniGameFromMenu` (+0x9ac) to the flag. Lex is `images/pam/Lex_normal.txt` and not `lex.pam`. Tried headless with a
profile at book 4, 5 and 6: the box, the buttons, the "new" mark, the checkbox's toggle, the Main Menu button, and a
click on WordUp, which starts the mini-game with its welcome dialog (the scripts' `DoGenericDialog` gives a `false`
for "no banner", which the binding now accepts).

Compared with Volume 2 (2026-10-26): `StartArena` (0x46c5d0), `ContinueArena` (0x46c4b0), `FillUserList` (0x46cde0) and the
user dialog's handler (0x46d130, its OK, close, new, delete and delete-confirmed branches) do the same steps in the same
order as the port's Volume 1 derived code; the new-name function 0x46cb30 and the Volume 2 offsets of the app's fields
(the timed-mode flag is +0x9aa there, the replay flag +0x9ab, ELITE's +0x95b) have not been compared line by line. The
header comments of `GameApp.h` that give offsets are Volume 1's, except the new `mBookSelectScreen` (+0x800) and
`mReplayMode` (+0x9ab), `mMiniGameMenu` (+0x870), `mMiniGameFromMenu` (+0x9ac) and `mMiniMenuFromMainMenu` (+0x95a).

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

All the methods that 0x43a850 registers by name are in now (`DoHighScoreScreenFromReplay` is 0x446860, which no script calls;
`DoReplayToHOFTransition` and `DoCSToMiniMenuTransition` too), and every `gCApp` method the 462 scripts call is bound. The
rest of the bound methods come from the registrations of the other classes (`LuaApp`, which the port has in `luaapp/`). `LoadMusic` (0x4448c0, the
file is `music\<name>.ogg`), `PlayMusic(id, offset, fadeOthers)` (0x445290) and `UnloadMusic` (0x4452f0) are in, over the
framework's `MusicInterface` by song id; Volume 1's tracker-module `SongPlayer` is not made (`mSongPlayer` stays null) and
`PlaySong(id, speed, loop, flag)` is the inlined `IsPlaying`, `FadeOutAll`, `PlayMusic` of 0x43df46 (its fourth flag was
not found; the implementation behind 0x44c490 has not been located).

The songs of Volume 2's C++ (every use of the app's music interface, the field at +0x374, was listed in the exe): the
start-up loads ids 0 to 8 (0x43b0cd..0x43b214: `fairy_tale_map`, `_level`, `_boss`, `chinese_map`, `chinese_level` twice,
`scifi_map`, `scifi_level` twice; the scripts' `MUSIC_FAIRY_MAP` and the like are these numbers, and the boss songs of the
Chinese and sci-fi books come from `book.luc`'s chapter songs, `LoadMusic(tonumber(mSongId.."0"), name.."_level")` and
`.."1"` with `mSongId` the book and the chapter number run together, so 400, 401, 410 ...). The main menu plays 1 up to
book 4, 4 for book 5 and 7 above (the constructor 0x468d46 with no look at the no-music flag, `ShowMainMenu` 0x43e38d
without it when that flag is set), and `ShowMainMenu` then (when not from the title) unloads the chapters' first songs that
are not playing, ids 400, 401, 500, 501, 600 and 601 (0x445320). The intro asks for 140 (0x8c), which nothing loads, so it
plays nothing. Nothing else in the C++ plays a song: not the credits (`Credits` is a `DataDrivenClass` with
`data/credits.xml` in Volume 2, which the port's Volume 1 `Credits` is not: to be rewritten from 0x42c160 onwards;
`CloseCredits` 0x446ed0 has no song either and does not clear the no-music flag), not the mini games' menu, and the stats
book's `PlayMusic` (bound at 0x47f5a0, called by `statsbook.luc` for a music entry) is not among the sites. `StatsBook::PlayMusic`
(0x4815e0) only notes that a song plays, and ignores the song.

The stats book is Volume 2's now (`StatsBook`, 0x47f4d0-0x481610, with `CustomListWidget` 0x481650-0x481e90 and the new
`SecretListWidget` 0x481ec0-0x482390). Against Volume 1's: the tome has a second list, of secrets (a tray for each, `???` until
found, the name written on the tray with the medium font once it is; no clicks, no hand), with its own slider (id 2) and two
arrows (ids 12 and 13) that nothing shows; the scripts have `AddNewAreaLine` (a second text for each line of the list that
`AddSecondaryLine`'s does not fill: the book draws the secondary texts after a timed game and the new area's in the tome),
`AddSecretLine`, `ReloadSecrets` (empties the secrets and calls the script's `RehupSecretList`) and `Rehup` (what
`SetLayoutFile` does after reading the layout: the "Continue Button", "List" and, in the tome, "Secret List" nodes). The
list draws a line without what follows its first `(` (and the space before it) and cuts one longer than 19 letters to 18
and `...`. The arrows of the first list show from 13 lines, `SliderVal` is by lines minus 12 (the secrets' by minus 4) and
the wheel by minus 14 (minus 4), the arrows move the first slider 0.05 and the secrets' 0.075. The continue button of the
media player stops the movie and plays no song, and `PlayMusic` plays none either (Volume 1's played the song it was given
and showed its spectrum). The constructor also keeps a copy of an app field (+0xe0) that nothing reads; the port has no
member for it.

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
4. (done 2026-10-26) Cut scenes: `swf/` reads the bitmap tags Volume 2's five movies use (DefineBits with JPEGTables,
   DefineBitsJPEG2/3, DefineBitsLossless/2, decoded with stb_image) and draws repeating, clipped, smoothed and plain bitmap
   fills (`SwfRaster::SampleBitmap`); a bitmap id of 65535 is no fill. `bwa2_swftest` plays all five with nothing skipped
   (DefineFontName is ignored), and its -verify finds the incremental drawing the same as the whole-frame one.
5. PopAnim: all 304 `.pam` files load (versions 1: 36, 2: 2, 3: 266). `bwa2_pamtest` reports 41 pictures not found in 7 files
   (2plaguedpork, 5mommabear, 5poppabear, 7smokedragon, 9tweedee, drfrank, hephaestus): the game's folder has no pictures of
   those names (nothing is named like them in `images/pam/images`) and no script or data file names the seven animations
   (the scripts use `5papabear`, `5mamabear`, `9tweedledee`), so they are leftovers that the original never plays; not a
   fault of the port. (2026-10-26)
6. Web page (`web/page.js`): markers, import folders (`music/`, `scripts/` are loose in Volume 2) and prose still say
   Volume 1; its persist folder is already `/bwa2persist`.

## The word list's flags (2026-10-26), the start of the battle milestone

Volume 2's `TileEngine.lua` reads `is_noun`, `is_adjective` and `is_verb` after `is_valid` in `ValidWordSpelled` (found when a new
profile's tutorial stopped at `TileEngine.lua:856`). They come from the word list: `data/compressed_attribs.dat` holds four bytes
(little-endian; 1 adjective, 2 verb, 4 noun) for each word of 3 to 16 letters of `data/compressed.txt`, in the list's order
(191,743 of both, which is how the format was checked), read by `0x49d650` into a hash map that `0x49dad0` looks up. `Lex` keeps
them in a `std::map`; `Board::IsValidWord` gives its callers the flags, and 0x486fa0 (`TileEngine::ValidWordSpelled`) pushes
`(is_valid, count, is_noun, is_adjective, is_verb, tiles...)` with the call's count the tiles' plus 5; a word that is not a word
(or under 3 letters) has flags 0. The board's `IsNoun`, `IsVerb` and `IsAdjective` (0x41a1a0, 0x41a1d0, 0x41a200) take the word in
capitals. The suite's new-player run (`tools/check.sh bookwormadv2`) goes through the tutorial to its first attack.

Volume 2's Board registers seven Lua methods that Volume 1's does not (0x40f970 against `Board::RegisterLua`), and the port has them
now: `GridRemoveTile` (0x4126b0, the grid's cell), `RemoveTile` (0x412750, the board's list only), `AddNewTiles` with its
bonus flag (the push of new tiles and `CreateBonusTiles` only when it is true; `SubmitWord` passes true), `SetAttackVisible`,
`GetTMEnemyAttacking`, `IsScrambling`, `GetTileBelow` (0x419550) and `GetTopTiles` (0x418ae0, with `GetRandomizedTiles` (0x418df0) on
one shared loop). `SetScrambleVisible` only stops the button's drawing, as in the exe. `BE_COMPANION` (10) is a battle state of
Volume 2 (chapter 3's end calls it); a PopAnim's labels are compared without case (the PAMs' are lower case, the scripts pass
capitals), `popanim/PopAnimData.h`.

The test hook's `soak <update>` plays the battles by itself (words from the board, potions when the tutorial asks for one, the
stunned panel, the treasure room, the chapter map's Enter) and was the way the battle milestone was shaken down: from a new
player through the prologue, chapters 1 to 8 and Skeletrox's boss fight, 219 words in 340,000 updates with no script error.
