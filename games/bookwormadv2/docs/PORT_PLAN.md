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
   as Volume 1). With them the game's C++ no longer compiles: 28 uses of ids Volume 2 does not have, which is the
   list of what the C++ has to be reworked for:

   | File | Ids Volume 2 lacks | Meaning |
   |---|---|---|
   | `TitleScreen.cpp` | `IMAGE_TS_DELUXE` | no such image in Volume 2 |
   | `MainMenu.cpp` | `IMAGE_MM_OPTIONS_OVER`, `_QUIT_OVER`, `_HS_OVER`, `_TOME_OVER`, `_FILM_REEL_OVER`, `_FILM_REEL_DOWN`, `_CNG_TITLE`, `_ADV_COMPLETE`, `_EXP_PACK_AVAILABLE`, `_LOCKED_LEFT`, `_LOCKED_RIGHT`, `IMAGE_FILM_REEL` | Volume 2's main menu is a different class (24 KB, none of it matched) and `BookSelectScreen` is new |
   | `ArenaWinDialog.cpp` | `IMAGE_FILM_REEL` | |
   | `Board.cpp` | `IMAGE_GS_SHIP1`..`4`, `IMAGE_GS_SHIP1_WARPJET`, `IMAGE_GS_GO` | (not looked at yet) |

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
2. Redo `tools/gametext.py` for Volume 2 (the `GT(0x...)` uses in `src/` are Volume 1's addresses; `GameTextTable.inc`
   is empty) and fix `LoadGameTexts()`, which returns false for an empty table.
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
