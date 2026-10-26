# Content a player cannot reach

TipTop's session found a whole game mode, Timed, that its configuration places off the edge of the 640x480 screen. The
user wondered whether other games hide something the same way, so on 2026-10-25 one Sonnet agent swept the eight
games merged at the time (Bejeweled, Big Money, BookWorm, Noah's Ark, Seven Seas, Atomica, TipTop and Bookworm
Adventures) for three kinds of unreachable content: widgets placed off screen or never given a size, modes, options
and keys that the code supports but nothing offers, and resources that ship with the game and that nothing loads or
draws. It read the decompilations, the ports, the Ghidra exports and the player's game files, and ran no game. The
coordinator checked the findings that would change a port against the decompilations. Mummy Maze was merged while the
sweep ran and is not in it.

Each finding says how it can be reached: **never** (no path in the code), **config edit** (only by changing one of the
game's files), **key or switch** (a debug key or a command-line switch), or **unsure**. Where a game's own notes
already record a finding, this page says so and keeps it short. Trial and registration code (nag screens, buy and
update dialogs) is dead in the copies we have and is left out unless it hides something else. Texts are named by
their ids or paraphrased; the secret words of Bookworm Adventures are deliberately not written down.

The short answer to the user's question is that TipTop is not alone, but only Bookworm Adventures hides anything a
player would call a feature: a developer cheat script that ships switched off, a set of secret words typed on the main
menu, three enemies that were cut from the first book, and the maps of two books that nothing loads. TipTop's
configuration switches off two more screens besides the Timed mode. The other games hide only leftovers: sounds that
are loaded and never played, pictures that nothing draws, and help pop-ups that are never raised.

## Port gaps found on the way

A finding that first rested on "the port never uses this file" was checked against the original, and several turned
out to be used by the original and missing from the port. They are fidelity faults, not hidden content, and went to
the games' sessions on 2026-10-26.

In Atomica the original plays `swoop` and `doorslam` from the main menu's update (0x4191d0) and from two other places
(0x419520 and 0x41e770); the port loads both sounds (`AppLoad.cpp:478-479`) and never plays them. It also reads the
`HSTextEdit` art (`mTextEdit`, +0x30c0) in the dialog at 0x41cf90, which the port loads and never draws. In Noah's Ark
the original's tutorial dialog (0x420d10) draws `tutorialbox` (image 82) as the panel behind the animals, and its
edit-box helper (0x41dca0) draws `editbox` (image 77) behind the score name; the port draws neither. In Seven Seas the
board's "extra life" flash (`Board::Draw`, 0x406400) draws `bonusship` (image 37), where the port draws `extralife`
(image 31, `BoardDraw.cpp:724`), and the score-name dialog (around 0x41a480) draws `editbox` (image 7), which the port
does not. The original Seven Seas also has a "submit to the Internet high-score list" button and its dialog 11 (with
the pictures `internethighscore` and `dbuttonclear`); the port leaves them out on purpose, as the servers are gone and
the project has no online features.

## The shared framework

The SexyAppBase of the later framework (found in Bejeweled's exe; the same code is in Big Money, BookWorm and
Bookworm Adventures) has a debug-key chord that no help text mentions. Ctrl+Alt+D turns the debug keys on, and then
F2 starts and stops a timing profile and shows the result in a message box, F3 shows the frame rate, F8 switches 3D
acceleration on and off, and F11 runs a dump (a screenshot or a program-information dump; unsure which).
`SexyAppBase::DebugKeyDown` is at 0x44a3c0 in Bejeweled. The port keeps the toggle (`framework/SexyAppBase.cpp:2001`)
but its `DebugKeyDown` does nothing (`:1683`). The same exes take command-line switches that no shortcut uses:
`-version` shows the product, version and build in a message box, `-play`, `-record`, `-recnum`, `-playnum` and
`-demofile` record and play back demos, `-crash` crashes on purpose, and some exes add `-screensaver` and `-changedir`.
The port supports only `-changedir` and its own test switches. The 2002-2003 framework of Noah's Ark, Seven Seas and
Atomica has none of these strings, nor does TipTop's LOWOS engine.

A few settings live only in the registry: `WaitForVSync`, the window position `PreferredX`/`PreferredY`, and the
registration values. `NoPopLink` is read by Bejeweled and Big Money and used by neither.

## Bejeweled

The original has nothing hidden in its interface: Normal and Timed are the only modes, both on the main screen, and
there is no cheat key or screen that the menus do not offer. What remains is leftovers. The options dialog creates a
"check for updates" button that is never given a size, and handles a "visit PopCap" button that is never created
(both already noted in `GAME_NOTES.md` section 8). `WinDMApp::Init` builds an app-level high-score widget that is never
added; the game-over screen makes its own (already noted). The property `IsDiamondMine` (off in `partner.xml`) would
switch the trial texts to Bejeweled's earlier name, Diamond Mine, and take another branch of the title screen's draw
(0x4d6399, 0x4d370b; reach: config edit).

Among the files, the sounds `goodbye1`, `welcome1` and `sidebarclick2` are named nowhere in the exe. `combo62` and
`combo72` ship, but the sound table repeats `combo52` for the sixth slot and has no seventh (already noted in `Res.h`),
and `click2` is loaded and never played. The image `bagholder` (with its mask) is in no name table; `blackcover` is
never used (already noted); and three alpha masks, `_dm-back`, `_real-back` and `_yahoo-back`, have no colour image at
all, leftovers of partner and Diamond Mine backgrounds. Of the 126 text ids in `properties/default.xml`, 19 never occur
in the exe: the bug-report texts, most of the high-score widget's placement texts, a cross-sell for another game and
the update-check prompts.

## Big Money

`games/bigmoney/docs/GAME_NOTES.md` already records most of what is dead in the interface: three selector buttons that
are never sized, the second help page and its "more instructions" link, the never-moving selector panel, a coin
lifetime timer that would make coins blink and vanish, and the trial and cobrand variants that `partner.xml` turns off.

New are the resources. Mr. Money has a "mega money" line that never plays: sound 41 (`megamoney`) is loaded, and
there is no call that plays it in the decompilation or the port, nor any caller of his speech animation 18 that would
go with it. Two songs, `SONG_UNUSED10` and `SONG_UNUSED13`, are start points in the one music module that nothing
starts. Four files are named by nothing: `data/bigmoneyfont2.txt` (a descriptor for a font that is not loaded),
`data/new_bigmoneyfont1.txt` (a draft in the newer descriptor syntax), and two pieces of placeholder art,
`images/XXXfont1.gif` and `images/XXXoutlinefont1.gif`. Sixteen of the 146 text ids never occur in the exe, among them
the bug-report texts, cross-sells for other PopCap games and a few trial messages.

## BookWorm

The board knows eleven hint pop-ups, each with a heading and a body (`Board.cpp:110-130`), but only four are ever
raised: the green bonus tile, the fire alert, the scramble and the "watch out" warning. Seven are never shown: how to
play, the gold super-bonus tile, the four ways to submit a word (the button, clicking, dragging and the keyboard) and
the bonus word. The notes record the unreachable ids in outline; which hints they are is new. The keyboard hint is the
only text that would have explained a real feature: in a registered copy, holding a letter key lights every tile with
that letter (`Board::KeyDown` 0x403df0, `Board.cpp:1039-1065`).

Three sounds are loaded and never played: `gameovertone` (id 1), `blank.au` (id 3) and a second entry for the button
click (id 24); there is no play call with these ids in the disassembly. The seven `lex_wow` frames and three Lex
animation rows, one of them an 88-tick lip-sync that looks like a lost welcome speech, are already noted, as is the
internet high-score page, which `partner.xml` switches off and whose server no longer exists. A few effect images
(`fx_ashflake`, the `fx_particle` sizes, `fx_lex_sweat_trickle`, two speech-bubble tails and the small `sparkle`) are
loaded and never drawn by the port; the original indexes some images by arithmetic, so whether it draws them is
unsure.

## Noah's Ark

Nothing of substance is unreachable. Every widget is on screen, and every image and sound in the original's tables is
used (the apparent gaps were the port's, above). The game-mode field takes 1 (Flood), 3 (Freefall), 4 (Puzzle) and 5
(the stampede bonus), so 0 and 2 look like removed modes. The level-complete panel has four tips but rolls only three
in Freefall, and a leftover `if (mDifficulty == 0 && false)` in the same place is a difficulty rule compiled out
(`Screens.cpp:299-302`).

## Seven Seas

The intro screen has a first-run variant that no player sees. When the app byte at +0x4c4 is set, `IntroWidget::Draw`
(0x416df0) writes a two-line "welcome" heading instead of the usual one, and `IntroWidget::Update` (0x417200) has the
pirate speak his welcome, sound 15 (`talkwelcometosevenseas`). Nothing in the decompilation ever sets that byte; it is
only cleared when a game starts and read in these two places (reach: never). The port leaves the sound unused.

`data/anims.txt` defines ten animations, but only six are ever started by `Board::AddAnim`. `WAVE2`, `PIXELLIGHT`,
`PIXELDARK` and `BLOTCH` are named nowhere else, so their four pictures are loaded with the table and never drawn (a
search for file names alone would count them as used, because `anims.txt` names them). Two images of the table,
`font2` and `optionback`, are drawn neither by the original nor by the port, and `images/_titlefont.gif` is a mask for
a picture that does not exist. Not hidden, but announced by no button: the ship can be steered with the arrow keys,
the number pad and Home, End, Page Up and Page Down, and fires with Space, Insert or the keypad's 5 and 0.

## Atomica

Every menu button is on screen, and all 28 tutorial hints are raised by some path (in strategy games only).
`MainMenu` has a branch for a game-over page without a board that fills the statistics with fixed numbers (a score of
999999, level 5, 50 atoms; `Menu.cpp:67-75`, 0x419760). Every caller passes a live board, so it is a placeholder or
test page (reach: never). `Meter::Init` (0x4056c0) would start a game at level 3 or 6 when one of two globals at
0x4d3b04 and 0x4d3b08 is set, but no instruction writes either (already noted in the port); a third byte at 0x4d40d1,
which decides whether the grid forwards a click to a widget, is never written either.

Eight image files are named by nothing, in the exe or in any data file: `badface` (with its mask; probably a cross face
for Dr. Atomicus), `tipdialog` (with its mask; probably an earlier hint box), `horizontalboardtile`,
`levelmonitormask`, `menucontinue`, and a copy of the machine picture whose file name is cut short. Four voice clips
are never loaded: `awesome`, `beautiful`, `biohazardalert` and `levelcomplete`.

## TipTop

The Timed mode was the starting point: four positions in `tiptap.cfg` (inside `TipTop.dat`) are set to 1000, each with
a comment that Timed was moved off screen, while the command, its level list and its high-score rows are all present
(`games/tiptop/docs/GAME_NOTES.md`). The sweep adds that Timed has no bonus rounds of its own: its first round in
`bonus.txt` sits at level 500.

The same file switches off two more screens, and their art was taken out of `TipTop.dat`. `DELETEGAMEWARNING_DISABLE
= TRUE` skips the dialog that asks before a new game overwrites a saved one (`DeleteGameDialog`, 0x425ffa,
`App.cpp:1720`), whose three pictures are missing from the data file though its positions are still set.
`LEVELCOMPLETE_DISABLE = TRUE` skips the "Level Complete" animation (`App.cpp:964, 1169, 3683`), whose picture is
missing too. Turning either back on would ask for art that is not there (reach: config edit, and then probably an
error). `TURNBASED = FALSE` hides a complete turn-based variant of the board, in which fallers stay locked from release
until the board rests (`Board.cpp:1404, 1465`), and the related switches `LOCK_FLOATERS`, `LOCK_FALLERS`,
`DISABLE_STOPS`, `RANDOM_BLOCKS` and a "sequential" floater method are used by no mode (reach: config edit).

The exe also has a logger at 0x43b780 that appends to `c:\debug.txt`, which nothing calls or points to, and a matching
`Art\Debug.pcx` in its resource table that is not in the data file. The table names more than the data file holds:
animation, combo, instruction and score-font slots beyond the ones used, and ten sound cues (among them `tap`,
`entermainmenu`, `blockcollision` and four explosion clips) that are silent because the loader returns nothing for a
missing file. That is the engine's capacity rather than hidden screens. The other way round, `Art\BricksSav.PCX` is in
the data file and named by no code, and `Art\BonusComplete.PCX` is in both but no code asks for it (unsure, since some
ids are reached by arithmetic).

## Bookworm Adventures

This is the game with real hidden features. Its default pack loads `scripts/cheat.luc` in the retail game. The script
defines about 25 developer commands (kill the enemy, fill Lex's health, level him up, complete a chapter, give a
potion, wipe the profile and so on) whose messages name TAB and DEL as their keys. They are gated by two Lua flags,
and `GameApp::Init` (0x445a50) and the function at 0x4481a0 set those flags with constants
(`main.SetDisableCheats(true)`, `main.SetBetaBuild(false)`), so the battle engine's cheat hooks never run (reach:
never). No script binds TAB or DEL; if they were ever bound, it was in C++ debug-key code (unsure). The port leaves the
cheats out on purpose (`GameApp.cpp:64,117`).

What does work is a set of seven secret words typed on the main menu (`MainMenu::KeyChar`, 0x47da70;
`MainMenu.cpp:464-507`). Six make Lex play an animation, and one calls the cheat script's `_DefaultMP`, which hands the
player the media player (the film-reel menu item) before the arena would unlock it. Nothing in the game's texts
mentions them. The original also has a `-wait` switch (0x65f750) that hands a value to its Lua debugging console, a
text window with breakpoints that nothing in the menus opens, and an in-game layout editor script
(`common/dprops_editor.luc`, with `DPropsWidget` on the C++ side) whose trigger the sweep could not find; every
shipped layout sets its `debug` attribute to false (reach: unsure for both).

Three enemies of the first book were cut: `creatures/book1/enchantedboar.luc`, `enchantedwolf.luc` and `hippogriff.luc`
have scripts and dialog scripts but no art, no word list, no lore entry and no place in any book's spawn list. A
dialog script for a toxophilite archer exists for a creature no script defines, perhaps one a summoner was meant to
call up. Developer helpers that list every enemy of a book, a superseded `common/app.luc`, and the Link 'N Spell data
script (which its mini-game never loads) are dead too.

Among the resources, books 2 and 3 have ten map pictures each (`minimap_2-1..10`, `minimap_3-1..10`) that nothing
loads, since only book 1's resource file defines its minimaps. `widescreen_left` and `widescreen_right` are art for a
wide-screen frame that was never wired up. `lex_unbelievable` is a tenth praise line that no script plays. 29 pieces of
animation art are named by none of the 269 animation files, among them Medusa's snake arms, a red Cyclops eye, extra
sea-serpent and hydra frames, and backgrounds such as a desert sky and whitecaps; they look like leftover exports of the
animation tool. A few board pictures (`heart_small`, `ui_treasure_pedestal`) and the font descriptor
`humanst521bt10italics` are unused as well.

<!-- private: The player's `partner.xml` names a trial configuration that the original exe would honour; the port ignores it and always runs the full game (`MainMenu.cpp:467`). -->
The player's `properties/partner.xml` sets `MarketingMode = HydraTrailer`. `GameApp` (0x4471f0) maps the property to a
field at +0x980: `60Normal` is 0, `60Trailer` 1, `Hydra` 2, `HydraTrailer` 3 and anything else 4. Modes 0 and 1 talk
to a launcher window by `SendMessage` (a 60-minute trial wrapper), modes 2 and 3 end the free trial when the Hydra is
beaten, and 3 adds a line about quitting to the main menu. The secret-word handler also depends on this field. The
port ignores the property and always behaves as the full game (`MainMenu.cpp:467`).
<!-- /private -->

## For later games

Most of these finds took a reference chain rather than a file-name search: an animation table, a data file or a
`.pam` can name a picture that nothing starts, and an image can be used by the original through a constant offset
from its image array while the port never names it. The sweep's method was to list every file and table entry, look
for each name in the exe, the data files and the port, and then check every "nothing uses it" against the original's
decompilation. A game session that finds such content can note it in its `GAME_NOTES.md` under its own heading, which
makes a later sweep a matter of reading.
