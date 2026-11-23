# Hammer Heads Deluxe: game notes

What the port does differently from the original, what it leaves out, and what is odd. The analysis state is in
`WORKING_NOTES.md`; the code is the translation of `HammerHeadsGame.exe` (PopCap, 2010, SexyAppFramework 1.2 layout, the same
as Big Money's), written to `docs/CODE_STANDARDS.md` from its first draft.

## Test commands

Reachable only from a test (`-script`, `-commands`, the web page's test call; `docs/TEST_COMMANDS.md`). Replies go to stdout:
`STATE {json}`, `OK <command>`, `ERROR <command>: <reason>`. A script line is `<update> <command>`.

| command | what it does |
| --- | --- |
| `state` | `{screen, zoom, mode, level, score, health, coins, timer, phase, paused, inplay, dialogs, enemy, gnomes}`; `gnomes` lists each cubby with a gnome: `{cubby, kind, state, cubbyState, rise, health, x, y}` |
| `goto <screen>` | `menu`, `highscores`, `trophies`, `shop`, `challenge`, or a mode: `classic`, `tough`, `marathon` |
| `level <n>` | starts level n of the current mode over the board there |
| `unlock all\|marathon` | Tough Cookie is unlocked, or Marathon (the classic list gets restart points up to the three it asks for); a menu that is already up is told |
| `set score\|health\|coins\|level <v>` | the score, health, coins or level of the game in play |
| `win`, `lose` | the level in play goes to its end at once (the pause after the last cubby), or the health runs out |
| `seed <n>` | seeds the game's random numbers the way a new game does |
| `move` | `MOVE {"kind":"smash","cubby":i,"input":[...]}` for the bot's next swing (the first cubby with a gnome up that a hammer can hit and that is not a bomb), or `MOVE null` |
| `memreport` | the framework's, `MEMREPORT {...}` |
| `trace <file>` | writes a `TRACE T\|F ...` line after each update's first step (T) and each frame's `UpdateF` (F) to the file: the state phase one compares (the random generator, the stats, the pop timer, the level timer, each cubby and its creature, the avatar's swing and clicks) |
| `cheat <word>` | types a cheat word into the application a letter at a time, as the main menu's key handler does (`jumpto5` to `jumpto25` start the next game at that level); the main menu clears the cheats when it is made, so it goes after the menu is up |
| `startlevel <n>` | the next game starts at level n, as the cheat words do for 5, 10, 15, 20 and 25 (`Application::mTestStartLevel`, read by the game screen's constructor; for the levels between, such as 22 where the multiplier gnome first comes) |
| `botclick [bomb\|miss]` | the bot's click made as a `click` is (the first creature standing up that is not a bomb; a bomb; bare ground), or the button of the level's introduction or the "Ready?" box when one waits; logged as `BOTCLICK <update> <x> <y>` |

The bot reads the game's state and draws no random number. Its swing is a `move` to the gnome's strike point, two updates' wait,
a `click` and ten updates' wait, which is what a player's click does.

## Deviations (each without a visible effect)

- **The refresh rate is fixed at 60.** The original reads the display's rate (60 under its test runs) and runs its logic once a
  refresh in `UpdateF(theFrac)`, with `theFrac = 1000 / (10 * rate)`; the port fixes `mSyncRefreshRate` at 60 so that every machine
  takes the original's 60 Hz steps (the framework's own default is 100).
- **XML.** The player roster, the saved games and the settings are read and written with `XmlLite`, a small reader and writer; the
  original uses TinyXML, which is not available. The files keep the original's element names and order.
- **How to play.** The original shows its how-to-play movie (`images\howtoplay.swf`, 800 by 500, 24 frames a second, looping)
  through a Flash control in "Transparent" window mode; the port plays it through the shared SWF player (`engines/swf`), which
  draws the movie's own background (a green), not what is behind it. The shot comparison must judge whether the page differs.
  The movie's 25 JPEG bitmaps are written as two streams (the tables, then the image), which the player now joins before it decodes
  them (`shared:` commit in `engines/swf`); `hammerheads_swftest` plays the movie headless and lists what the player skips (nothing).
- **Game text kept out of the repository.** The default high score names and the trophy names and descriptions are tables built by
  the exe's static initialisers; `tools/highscorenames.py` and `tools/trophytext.py` read them from the exe and write the
  git-ignored `src/HighScoreNames.inc` and `src/TrophyText.inc`, which the build includes.
- **The loading screen** no longer moves on by itself, as the stand-in did; its co-branding Flash widget is not made.
- **The desktop's size** (Application +0x70c and +0x710), which only the options box reads (a full screen that is turned off must
  leave a window that fits), is asked of Windows in the original before the display is made (`GetSystemMetrics`); the port reads
  SDL's usable bounds right after the framework has made the display.
- **The options box's tests of the card.** The original asks its 3D tester twice (the card can run 3D; the card is one it
  recommends); the framework's two answers are one in the port (the renderer's `mCan3D`), so the "Warning" notice (3D works but is
  not recommended) never comes. `mForceFullscreen` (a desktop that is not in 16 or 32 bit colour) is never set, so the notice about
  it, and the full screen box that cannot be cleared then, cannot come either.
- **Debugger lines.** The original writes lines to the debugger (`OutputDebugStringA`) where the options box pauses and un-pauses
  the game, where the leave box does, and in the credits' Flash listener; the port writes none.
- **A swing whose target is gone.** `Avatar::FixedUpdate` (0x42eb80) strikes the swing's target at frame 3 without testing the
  reference for null; the original crashes there (an access violation at 0x42ebb2, found by a scenario in which the bot clicked
  the shop's arrow while a swing had begun, so the shop's gnome was deleted before the strike). The port tests it (a crash is
  fixed in the port, as the rules say), so the swing ends with no strike.
- **The trophy page buttons** (the announcement of a new trophy and the close-up) are one class here, `TrophyPageButton`; the
  original has two classes with the same code and fields. A trophy's shadow picture (the unearned ones) is deleted with its
  button; the original leaks it (a memory leak, fixed here as the rules say).

- **How BASS is asked:** the original calls `BASS_ChannelStop` on a song before it plays it, and on the other three songs at every
  fade-in; the port's mixer stops what plays as it plays the next. What is heard is the same (the music comparison, `musiccmp.py`,
  drops those stops).

## Deviations with a visible effect

- **The word "Deluxe" is not drawn** under the logo on the loading screen and the main menu (the user, 2026-11-18: "Remove the
  "Deluxe" text from the main screen of any game it may appear in, to avoid misleading people about whose game version they're
  playing"). `DeluxeCover` makes the cover as the screen is drawn; no edited picture is committed. The site's listing, the window's
  title and the page's title say "Hammer Heads".
- **The player's file is read in both forms and written in the original's** (the XML in pieces, each with its length in front), so
  either game reads the other's files (`XmlLite`, `ProfileFile.cpp`).
- **A High Scores page left for the menu stays under the zoom wipe's first frames** (the original's uncovered pixels are not cleared
  between frames, the port's are), as the original shows it.
- **Not a deviation (screens pass, C): an animation driven by a counter in a long sandbox scenario** showed another age than the
  port's because the sandbox's shot landed at an earlier update than its label (`landed.tsv`); read through the game's own draw
  surface on the host the shots agree (`IDENTICAL_PLAY.md`, "The screens pass", C).

## Left out (nothing in play can reach it)

- The level **editor**: the original's GameScreen is a tool widget with the game on top; the port keeps the widget and none of the
  editor.
- The **DRM client** (a window broadcast to the Steam wrapper, a three-second wait at start), the **beta tools** object at
  Application +0x77c (a login and update check, 0x41a000-0x41bac0) and its menu items.
- **Registration**: the demo limit, the level-allowed check (0x41e520, always true), the upsell dialog, the buy and register
  buttons and every string that only they used. **More Games**, the Steam link and every other button that leaves the game.
  The pictures that only they used (the upsell page and its five shots, the "More Games" button's) are listed in
  `tools/resdrop.txt`: `resgen.py` leaves them out of `Res.h` and `Res.cpp`, and `stage_hammerheads_assets.py` takes them out of the
  staged `resources.xml` and does not copy their files, so the game never loads them.
- **State that is written and never read.** The trophy room's fade (+0xb0, set to 3.0 and counted down), its hover alpha (+0xa8)
  and trophy count, the trophy button's +0x150 flag, an unused "Click to continue" string; the game-won page's counter
  (DAT_00598084, one more while a page exists); none of it is in the port.
- **The resurrect page's flag.** `ResurrectScreenWidget` takes a flag whose only call (0x422a72) passes false; with it set the
  page has a "Back to Game" button and two lines about a new savepoint. That page, its button, the pointer at
  +0x84 (a button that nothing creates) and `Application::ResurrectBackToGame` are not in the port, and `ResurrectOrPlay` takes no
  argument.
- **The credits' Flash widget.** `CreditsDialog` holds a pointer to a Flash widget (+0x170) that nothing ever makes; it is also a
  `FlashListener` whose two functions (0x44f300, 0x44f370) only write to the debugger. The code that places and adds the widget,
  and the listener, are not in the port. The options box and the quit box have no registration or outbound button, so nothing was
  taken out of them.

## Fixes (the fixes mod)

`mods/fixes/mod.json` has the three fixes; the port asks `Platform::FixOn(id)` at each one and keeps the original's path beside it
(every option is off in a test run with no `-modsdir`, so the frozen baseline is the original). The page ships the mod as
`hammerheads-fixes/` and installs it on. `games/hammerheads/tools/fixes_check.sh` runs `tools/fixes_test.txt` as the original, with the mod
at its defaults and with each fix alone (under rundesk, `-fast -seed=1`).

- **quitEscape** (on): the quit box (`QuitDialog`) sets no escape answer, as the options box does ("Done"); with the fix Escape answers
  "No". Checked: the box stays open on Escape without it, and closes with it and with the mod.
- **menuTrophySting** (on): `TrophyAppearButton`'s constructor (0x45f5e0) plays the medal sting when the trophy room opens from the main
  menu as well as when a game awards one; with the fix it is silent from the menu. Checked in the sound log: the sting plays once
  without it and not with it or the mod.
- **restartCoinCount** (on): on a restart point's card (`ResurrectButton::Draw`) the number's width is measured in the font of the
  line above (Eurofurence 14) and drawn in Arial 10 bold, so the coin and the number sit off the middle; with the fix it is measured in
  the font it is drawn in. Not in the tour (the card needs a Marathon game to its first restart point): the change is the one font
  set before the measurement.

## Fix candidates

Behaviour of the original that looks like a mistake and is kept in the port, with no fix yet.

- **The button sound of a moving dialog** never plays (see Oddities); **a check box that cannot be ticked** plays its sound twice.

## Oddities (not chased)

- **The pictures of the restart point buttons** are taken from the exe's table of pictures from `IMAGE_PHOTO01` on, and only five
  of its entries are photos; a sixth point (if a game can have one) is drawn on the high score background. The port keeps the
  table.
- **The game menu's buttons** are in the order Main Menu, then the options, then Continue at the bottom.
- **The button sound of a moving dialog** (`NcMovingDialog::ButtonPress`) compares the id with the id plus 2000 and 3000, so it
  never plays.
- **The ready box** (`ReadyGoDialog::Closed`) un-pauses the game twice.
- **High scores** that were changed in the file load as their value modulo 256.
- **The game-over box's gnome** is not set when its file cannot be read (the original uses it uninitialised).
- **The main menu's locked-mode box** has the id 0x14.
- **The swollen trophy on the announcement page** is placed by half its width where its height was meant (0x45f6a0).
- **The level introduction** takes the gnome's pointer before it tests it for null in its destructor, and its update and draw
  assume it is set (a level file that names a gnome the factory does not know would crash).
- **The quit box** draws its picture box a second time over the one the dialog draws, and its 3D hammer is put by its middle at a
  different place from the 2D one's.
- **The options box's notices** hold runs of tab characters in the middle of their lines (the exe's strings); they show as
  nothing in the port.
- **A check box that cannot be ticked** (hardware acceleration, or full screen when the window would not fit) is put back by a
  second answer to the listener, so the check sound plays twice.
- **"Reset Help"** sets the flag of the current player without testing that there is one.
- **The shop's and the challenge screen's cubbies** have the same names ("Cubby_9000N"), and the object manager removes by name, so
  a game that had both screens up would delete a cubby twice when it cleared them. The original's own flow leaves one screen
  before the other opens; the test commands `goto shop|challenge` refuse when the other is up.
- **A creature outside a cubby** (the game-over box's gnome, the level introduction's) is advanced through the creature's
  second update slot (+0xc, `UpdateWithoutCubby`, base 0x42f400: `mAge++`), never the one that reads the cubby; the shop's and
  the challenge screen's gnomes' `Remove` (0x437620) sets the flags at 0x85 and 0x87, not the finished flag at 0x86.
- **The quit and game-over boxes' button code** (0x44fe50) and the options box's press sound (0x44fe90) are functions the Ghidra
  export lacks (code the linker shared); they are found in the boxes' tables of the button listener (`tools/vtable.py` does not
  print them: read the table at the address in the class's constructor).

## Autosplit

`src/LiveSplitObserver.cpp` fills the shared autosplit block (game id HHDX, layout 2) after each update; it only reads the game. The
site asks through `byopop_livesplit_block`, a mod's "livesplit" feature or `-livesplitlog=<file>`. Hooks: `CreateGameScreen` (a board,
resumed for a saved game or restart point and a test game), Go on the first level's introduction card or a "Ready?" box, `LevelFinished`
(clear; Win for the last row of Classic and Tough Cookie), `Gameplay::GameOver`, `GameToTrophyRoom` (Regicist). Classic and Tough
Cookie's end is `progress` 1: Go on the "Ready?" box after the trophy room that awarded Regicist. Test: `set kings 1` once a game is
in play makes that trophy. Both classic and tough have 25 levels. The frozen baseline is unchanged with the block off.

With the block on (`-livesplitlog` on the frozen tour, 19 block lines) all 15 screens, the state lines and the player files are byte-identical to the run with it off, so the frozen baseline holds with the block either way (2026-11-15).

## Coverage check

`games/hammerheads/coverage.txt` names the export, the sources and the game's own code (0x401000-0x468e30, 1837 functions); the
level editor (0x405550-0x419c70), the beta login and DRM client (0x41a000-0x41d9c0), the compiler's deleting destructors, the debug
`std::` instantiations, the original's own growable-array template, TinyXML (the port reads XML through `XmlLite.cpp` and boards
through `BoardFile.cpp`) and some one-field accessors are `none` lines with their reasons (732 functions left out on purpose).

Runs of the original (`tools/coverage/run.py <scenario>`, origrun on the host, the virtual clock, `--coverage`): `menus`, `howto`,
`dialogs` (the change-player box, the quit box with No and with Yes), `classic1`, `classic_save1`, `classic_focus1`, `tough1`,
`marathon1`, `classic_sp25`, `classic_j5`, `classic_nd10`, `classic_nd25`, `classic_j25`, `classic_save22`. Between them they reach
1235 of the 1837 functions; the first run of `menus` came out of a cold start with its inputs a little late and reached 1531, the
second 1757, so a run that reaches much less than its sisters is run again.

Reached in the original with no definition in the port: 268 at the first report. 12 were code the port holds under another shape
(the string splitter of `ValueList`, `IsValid`, the ordinal ending, the board file's loader and so on; their addresses now stand
in the comments above the functions that hold them) and the rest were library code, destructors and the editor. Two were logic the
port lacked, both found and fixed here:
- **0x45dcf0**, the shop's and the challenge screen's mouse handler (`MouseMove` and `MouseDrag` of both): the hammer follows the
  pointer there, as it does in the game screen.
- **0x405450**, the ordinal in the bonus round's opening text: "2nd Bonus Round!", where the port printed "2 Bonus Round!"
  (`ChallengeScreen::Draw`; the shot comparison had passed, because the identical-play runs compare the first round's card only
  at a few updates).
The first list is now empty.

A crash found by the `dialogs` run: the change-player box ("If this is not you, click here") ended the port in a null read, since
`UserDialog` never set the lines' font that `Dialog::Draw` measures (the original's box has the framework's default font, which the
port's `Dialog` does not make). Found with AddressSanitizer (`build/asan-hh`), fixed in the constructor.

Defined in the port and never reached in these runs: 137 functions. 35 are the gnomes' and shop items' `SaveExtra` and `LoadExtra`
(vf09 and vf10, 0x435de0 and the like): the saved game's per-creature state, which only a save made while one of those creatures is on
the board reaches (`classic_save1` and `classic_save22` saved on boards of simple gnomes). They are not dead code; a run that saves
with each kind on the board would reach them. 14 are Application functions at 0x41e960-0x422460 (the dialog and screen plumbing of
rare flows); 21 are in the Resurrect page, the credits, the player list's rename and delete, the quit and options boxes' rarer
paths; the rest are small drawing and update helpers of the shop's gift box, the rotating floater, the hammer display, the challenge
meter and the high score entry box, which only certain rounds reach. None is removed; the rare flows still need runs of their own.

## Performance

`tools/perf/perf.py`, `pairs.sh`, `pairsum.py` and `count.txt` measure the processor time like for like (docs/COORDINATION.md, "The
target"): the port and the original play updates 4000 to 8000 of the `classic1` inputs (level 1 in play, 40 s of real time) in a window
of 800x600 each (the port's `ScreenMode=0` in a fresh `registry.ini`, `-windowscale=1`), turn about, each pinned to a performance core
(`tools/pincore.py`), with the counting paused inside the window (`cyclesplit pause`, `count pause`) so that the process cycles are a
player's. The original is paced: its main loop spins at about 67 passes an update on rundesk's desktop (DirectDraw does not wait for a
blank), so `perf.py` patches a `Sleep(4)` stub into the padding of its code (`paced_patches`: 4 ms keeps the 59 draws a second of a 60 Hz
screen; 8 ms gives 41, 13 ms gives 10), and the cycle counter of origrun's `--count` follows the framework's `UpdateAppStep` (0x4d7d60),
`DrawDirtyStuff` (0x4dcf40) and `Redraw` (0x4e4bf0). The first boundary, 0x4d3290, is a once-a-pass housekeeping function and not the
update (67 calls an update), so the original's "update" part in the split is not the update; draw and present are.

Five pairs, million cycles an update, paced original: 2.17, 2.28, 2.28, 2.20, 2.21 (median 2.21). The port: 1.42, 2.68, 2.53, 2.42, 1.46
(median 2.42, ratio 1.10), two modes with the same code. The split of two pairs: the port's picture (draw, present and the driver's
threads) is 2.2 M, nearly all of it in the present, against the original's 1.73 M draw and 0.48 M Blt; the port's own update is
0.014 M against 0.004 M... (not comparable, see above). With the renderer's vertical sync off (`SDL_RENDER_VSYNC=0` in the
environment; the framework turns it on for every game unless `-fast`, `DDInterface::Init`) three port runs cost 1.30, 1.30 and 1.44
M an update, 0.59 to 0.65 of the original, in one mode: the Direct3D 11 driver's wait for the blank is a busy wait that the cycle
counts charge, in the part of the 16.7 ms frame where it lands. So the port's processor time is below the original's when the wait
is not counted, and above it by a tenth in the runs where it is. The fix is the framework's, the same for every game with a GPU
present (a present that does not wait, the frame loop's own pace, which `DDInterface` already has as `pace_ms`): sent to the
coordinator with these numbers. Nothing in the game's own code is slower than the original's: the draw is 0.09 M against 1.73 M.

Measured again on 2026-11-22 after the screens pass (the Deluxe cover, the High Scores wipe, the chunked player files), three pairs, the
port against the paced original, `pairs.sh 3 total`: 1.44, 1.76 and 2.81 M cycles an update against the original's 2.19, 2.00 and 1.99
(ratios 0.66, 0.88, 1.41; median 0.88). The spread is the present's busy wait (above), the same as in the first measurement; the
median stays under the original's. Coverage was not run again: the changes add no function the original has and remove none.

## Unneeded code

The linker's list of discarded sections (an -O0 build with `-ffunction-sections -fdata-sections` and `--gc-sections --verbose`,
`build/gc-hh`, docs/COVERAGE.md) names, in the game's own code, `Application::GameWonToMainMenu` (0x421c30: the game-won page's button
goes another way and the original never reaches it either), `RulesSheet::GetRowCount`, `NcMovingDialog::IsMoving` (0x468bf0),
`RuleDouble`, `Score::SetScore` and `GetScore` (0x44b170, 0x44b190) and `ObjectManager`'s destructor (kept: a singleton's). The
first six are out. Everything else it lists is the framework's (the 2004 dialog classes, string helpers, the XML parser), not this
game's to cut.

## The PopCap Game Collection version (2026-11-15)

The port was made from the Steam copy (`HammerHeadsGame.exe`, the game inside the Steam launcher), so it is checked against the game's
version on the PopCap Game Collection disc (`docs/COORDINATION.md`). `unshield` read the group `HammerHeadsDeluxeComponent` (543 files)
out of the disc image's cabinet into `E:/byo-og/hhcoll`; it came out with the folder `images/gui/main_menu` where the game's resources
name `main menu`, an extraction artifact (the check renames it back).

**The exe is another build** (`HammerHeads.exe`, 2,057,528 bytes against the Steam game's 2,072,994, with no Steam or DRM files). The
port reads no texts from the exe, so unlike Bookworm's it is not touched by that: the port's data is the files.

**The files.** Of the Collection's 543 and the Steam copy's 659 (the rest of its 659: the DRM and Steam files, the launcher exe, the
runtime sound cache), 274 are the same byte for byte and 246 differ. Of the 239 pictures that differ, 238 are the same pixels in another
encoding (the Steam copy's are recompressed). The one that is not is `loading_publisher_logo.png` (the publisher logo on the loading
screen: the Steam copy's and the Collection's logos are different pictures; both are 250 by 250). The others that differ are
`bass.dll` and `Flash.ocx` (the port uses neither), `license.txt`, `readme.html`, and `properties/partner.xml` (the Collection's has
`NoReg` true, DefaultWindowed false and the registry key `PopCap\HammerHeads`, with no Steam id or store link). `resources.xml` differs
by the registration's upsell pictures, which the staging script takes out as it does for the Steam copy.

**The port on the Collection's files.** `stage_hammerheads_assets.py` staged the Collection's folder (537 files) and the tour of
`tools/tour_test.txt` ran on it and on the Steam copy's staged files: the 31 state lines, the 15 screenshots (byte for byte) and the
player files are identical. What is not known: whether the Collection's own exe plays the same as the Steam one (its code was not
compared and it was not run); what speaks for it is that its data is the same pictures, boards, texts, sounds and fonts.

### The software screen: tried and dropped (2026-11-15)

The software screen (`SoftwareScreenDisplay`, the framework's opt-in for a port whose original draws its 2D mode in software) was tried
as a way under the default's 1.10 of the earlier pairs: `Application::CreateDDInterface` returned `SoftwareScreenDisplay::Create(this, true)`
(the original enables 3D itself, `mAutoEnable3D`, so the 3D offer). At the default (3D on) the display hands the screen to the GPU at once,
so a default player never reaches it; with `Is3D=0` it draws in software and presents through the surface. Measured in the five rounds
below, the two 3D-off rows are the GPU path 0.68 of the original and 178 MB, and the software screen 1.05 and 157 MB: 21 MB less memory for
a 3D-off player, at a processor time over the original's, which the performance rule does not allow. The override was taken out (the
coordinator, 2026-11-15); the default path was 0.71 of the original in the same rounds, so nothing needed it. The test switch
`HAMMERHEADS_SCREEN=gpu` went with it; `PERF_REG` in `perf.py` stays (extra registry lines for a port run).

### The screens against each other, and the presents without vsync (2026-11-15, after the coordinator's questions)

`tools/perf/screens.sh` (summed by `screensum.py`; `peak.py` samples the peak private bytes) took five rounds of one paced original and
the port four ways, one process at a time on a pinned core, updates 4000 to 8000. The original's median was 3.17 M cycles an update.
`HAMMERHEADS_SCREEN=gpu` (a test run only) keeps the framework's display in place of the software screen.

| way | cycles an update (median) | against the round's original | peak private bytes (median) |
|---|---|---|---|
| default, 3D on, vsync on (GPU) | 2.26 M | 0.71 (0.47 to 1.03) | 142 MB |
| 3D on, vsync off | 1.51 M | 0.47 (0.40 to 0.73) | 143 MB |
| 3D off, GPU path | 2.43 M | 0.68 (0.48 to 1.09) | 178 MB |
| 3D off, software screen | 2.98 M | 1.05 (0.83 to 1.22) | 157 MB |

The memory the software screen saves is real against its own comparison: with 3D off the GPU path holds the pictures in memory as well
as on the card (178 MB), and the software screen 157 MB, 21 MB less. The 21 MB higher of the first reading (159.7 against 138.6) was
the software screen with 3D off against the GPU path with 3D on, two modes. The software screen's price is processor time: 2.98 M an
update against 2.43 M with 3D off, since the framework's blitters draw what the GPU draws for nothing.

The 1.10 of the earlier pairs (same default, the same window) did not come back: this sweep's default is 0.71 of the original. The runs
vary by round from 0.47 to 1.03, and the original's own cycles moved from 3.67 M to 3.17 M between the two sweeps, so the machine's state
(it had a kernel pool leak, which the user restarted for) moves both. Read the default as under the original on this machine today and over
it in some rounds, and the vsync wait as the largest part of what is above it.

**Presents without vsync** (`SDL_RENDER_VSYNC=0`, `-presentlog`; the framework's frame loop paces at the display's 16.667 ms when the
renderer has no vsync flag, DDInterface.cpp's pace code): the intervals of the five runs have a median of 16.7 ms and 76 to 83 percent
within 2 ms of it, p90 18.4 to 19.2 ms, p99 42 to 50 ms; with vsync on (one run, `presents-v.txt`) the median is 16.7, 75 percent are
within 2 ms, p90 19.4 and p99 37.7. Both runs have a stall of a second or two at the start (the level's loading). So the presents come
as evenly without vsync as with it, with none of AstroPop's quantising to the updates' 10 ms.

No vsync opt-out is needed (the coordinator, 2026-11-15: at the default the port is at 0.71 of the original, so the framework's rule of vsync on
the GPU path stands); the figures without vsync (0.47, 143 MB) and the present intervals above are kept in case the question returns.

## Readability pass (2026-11-16)

`tools/namecheck.py games/hammerheads`: 0 findings in 185 files (43 allowed, each with its reason in `namecheck.txt`). The other rules were
read against the sources:

- **Fields nobody reads (rule 3).** `tools/splitfields.py` listed 42 places. The members it called read-only in an exported function
  (the main menu's buttons, `Application::mGameScreen`, `mGameMode`, `mUpgradeTimer`, `Avatar::mTarget`, the restart-point lists and the
  rest) are all assigned in other functions, so they are false findings. Two are written and never read: `Creature::mCubbyReleased`
  and `ObjectManager::mFixedStepCount`; both comments now say that nothing reads them. The one same-offset finding, `ParticleEffect`'s
  `mFragments` and `mPool` at +0x08, is the original's two lists (+0x08 free, +0x0c in the air) against the port's one vector that
  owns the fragments and a vector of pointers to the free ones; the comments now say so.
- **Numbers (rule 7).** Constants that several files wrote out or defined for themselves (`Rand()`'s 1 / 2^31, pi, half and twice pi, pi
  as a float and its halves, degrees to radians) are one set of names in `src/MathConstants.h`, replacing 12 per-file definitions and
  about 40 written-out values. Each keeps its literal, so the object code is the same: `objsnap` before and after, 101 objects, 2,180
  functions, 0 differ. The 47 other long decimals are each used once or twice, a reciprocal the exe keeps as a constant (a division
  would round differently); they stay as the exe's constants, and most of their lines have no comment saying what they scale (not done).
- **Comments (rule 8):** no stage, phase or milestone labels and no history in the sources.
