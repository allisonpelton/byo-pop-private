# Bonnie's Bookstore Deluxe: notes

How the port is built is in `ENGINE.md`, every way it differs from the original in `DEVIATIONS.md` (with the test
commands), and the identical-play pass in `IDENTICAL_PLAY.md`. This file holds the fixes mod and the performance
numbers.

## Fix candidates and the fixes mod (`mods/fixes`, `src/ModFeatures.cpp`)

The port is the original; the fixes mod changes it only while it is on (the site turns it on, one toggle a fix), and with
the mod off or absent the game plays as the original, which the frozen baseline shows (`tools/check.sh bonnies` runs
without a mod folder). Before this mod Bonnie's loaded no mods: `src/main.cpp` now reads them from `-modsdir` (by default
`mods-bonnies` beside the exe, `/bbpersist/mods` on the web; `-nomods` loads none), and the page offers them.

What was looked for: the waits that only cost the player time, and the bugs of the original that change how it plays
without breaking it. The game's waits were read one by one. The level's count at its end, the story's typing and the
pauses between a level's screens all end on a click or a key (`GameScreen::handleEvent`), so they cost no one time.
The tutorial's hint waits (2 to 7 s before the hint shows, shorter the fewer tiles are left) are the tutorial's pace,
not a wait on the player. The logos are the one wait left: a click only shortens them (`LogoScreen::update`: each
logo still stands 1.5 s and fades), so the whole sequence takes several seconds whatever the player does. They are not a
fix of this mod but the site's own setting, "Always skip start-up logos" (below). The bug that
changes play is the clock: every timer of the game is the machine's, read each frame (`Game::update` moves the falling
tiles by the time since the last frame, capped at 120 ms; the action game's clock and the end of a level's count run
the same way), so the same seed and the same input play out differently from one run to the next, and more so on a busy
machine. The one fault that broke play, the players panel's Delete that removed nothing, is fixed in the port itself
(`DEVIATIONS.md`, "Faults of the original that break play").

| Toggle | The original | With the fix |
|---|---|---|
| `steadyClock` | The game reads the machine's clock (`ticks`), so its frame times, and with them where the falling tiles are, when a level's count ends and how fast the action game's clock runs, vary with the machine's speed and load. | The host's clock (`BonniesHost.cpp`, `GameClock`) is the framework's updates, 10 ms each, and the game's sleeps run to it, so a frame of the game always spans the same updates: the same seed and the same input play the same game on every run. The game still runs in real time. A `-fast` run is steady already. |

How it was checked (2026-11-08): the first 7000 updates of the port's tour (`tools/tour_test.txt`: the logos, a new
player, the story, the tutorial's words, the game's options and the high scores) were played in real time, without
`-fast`, natively under rundesk. Two runs with the fix on gave the same 13 screens pixel for pixel and the same saves.
A run with the mod off ended on other high scores than the `-fast` run of the same script, its boards differing in five
screens, which is the original's clock at work. The fix's own runs are not the `-fast` run's either: `-fast` spaces the
game's frames over the updates in its own way, so the board's sparkles, which draw from `rand` every frame, end on other
letters. What the fix promises is the same game on every run, and that held.

Two faults of the fix's first version were found that way and mended before the commit. The engine's `waitUntil` does
not sleep when 3 ms or less are left; it spins on `pumpEvents` and the clock, which under the steady clock never moved,
so the game stopped at its start. The host now runs exactly one update after 1000 calls of `pumpEvents` with no sleep
between them, so a spin always ends after the same update. And once the framework has shut down (a script's quit) no
update comes, so the game's clock is then the machine's again, or its last wait would never end.

## Always skip start-up logos

The site's setting (`Byopop::SiteSkipLogos`, `framework/WebSettings.h`; natively `-skiplogos`) skips the whole logo
screen: New Crayon's logo with its falling letters, PopCap's logo, and the distributor's when the game has its image
(`BonniesApp.cpp`, `LogoScreen`). The screen still loads every resource on its first update, as it always does, and then
goes on to the menu the way the last logo's own end goes (`mDone`, then `setState(2)` from its draw), with no jingle.
The logos draw no random numbers, and the timers and flags they set (`gLogoSkip`, `gLogoFade`, the logo times) are read by
the logo screen only, so the menu starts as it does after the logos. With the setting off nothing changes: the frozen
baseline runs without it.

## The sound log (2026-11-11)

`BYOPOP_SOUNDLOG` names a file, as in every port, and the FMOD 3 stand-in (`src/BonniesHost.cpp`, "FMOD 3 on the
platform's mixer") writes the lines the framework's sounds write (`framework/SDLSoundManager.h`; its soundboard presets
are made from them by `tools/soundpreset.py`): `<update> load <id> <file> frames= rate= ch=` when a sample is loaded,
`instance <id> #<n>` when a channel takes it (`FSOUND_PlaySoundEx`), `play <id> #<n> vol= pan= pitch= loop=` when
`FSOUND_SetPaused` starts it, then `pitch`, `volume` and `stop <id> #<n>` for that channel. FMOD 3's volume (0 to 255) is
a straight gain, logged as the hundredths of a decibel DirectSound takes, as the framework's sounds are, so a full-volume
play is `vol=0`; FMOD 3 has no pan (always `pan=0`) and the game plays no loop (`loop=0`). The sample's number is never
reused after a free, so a file's name in the log is unambiguous. The songs (`FMUSIC`) are not logged: the site's Music
places come from the code. The log is read only, and it changes nothing else: `tools/check.sh bonnies` gives the same
screens and saves, and matches the frozen baseline, with `BYOPOP_SOUNDLOG` set and without it. A classic game played to
level 34 by `tools/realplay.py --live` wrote 12,879 plays of 29 sounds.

## The real-input driver (`tools/realplay.py`, 2026-11-11)

`realplay_classic.txt`'s `play` stopped on the tutorial's first word: its log said "the tutorial asks for FROG, 0 of 44"
every 2.4 s and never got further. The driver found the word and clicked its tiles, but it clicked as posted window
messages, which the board never takes: each click is a move, press and release in one burst (a press held longer is
dragged to the real cursor, `IDENTICAL_PLAY.md`, "Real input natively"), and the board follows a held press from frame to
frame, so no tile was ever picked (the old driver reproduced the same log on 2026-11-11, under `-fast`). The
driver now has a second input, `--live`: the game is started with `-realinput` (an empty file) and `-commands`, and every
step, click, key, typed text and focus change, goes into SDL's own event queue through the live channel, which holds a
press to the next frame as a hand does. `play` takes its moves from the port's own bot through the test command `move`
(`GameScreen::testMove`: a balloon's Continue button, the word the tutorial's balloon names over the tiles it marks, else
the best word, else the game's hint) instead of working the tutorial's word out itself from the state file, so that a
later bot mod reuses the bot, and it scrambles by the rule `tools/realmove.py` has (three words with three cells or
fewer left, eight otherwise, or the same word refused twice). Without `--live` the driver still posts window messages
(`realplay_tour.txt`, `realplay_window.txt`), and `play` there refuses to run, naming the reason.

    build/native/rundesk.exe <python.exe> games/bonnies/tools/realplay.py games/bonnies/tools/realplay_classic.txt <out> --live --         build/native/bonnies.exe -gamedir=assets/bonnies -userdir=<dir> -fast -seed=1

On 2026-11-11 `realplay_classic.txt` with `play 900` went from the menu through the name, Normal, the story, the tutorial
(FROG, then CAKE) and levels 1 to 34 (791 words, 62 scrambles, a screenshot at each level) in 15 minutes, `-fast`. The
whole story takes about 36 minutes (`IDENTICAL_PLAY.md`); the file's own `play 7200` allows for it. The game's log is
`<out>/game.log`, the driver's `<out>/play.log`, and `BYOPOP_SOUNDLOG=<file>` in the environment writes the sound log of
the game it played.

## Performance

### The performance pass (2026-11-11)

`tools/perf.py` (its docstring has the commands) plays the same tour in the original and the port, in real time and
without mods: the port's `tools/tour_test.txt` by its updates, the original's `tools/origrun/tour.txt` by the clock,
the original under origrun on its real clock from a fresh copy of the installed game with its seed pinned as the port's
`-seed=1`. Every 0.1 s it reads the game's process from outside: working set, private bytes, kernel plus user time and,
since this pass, the cycles the process's threads ran (`QueryProcessCycleTime`). "Loaded" is the time from the
process's creation to the main menu, the first screen that takes input other than the click that hurries a logo;
nobody clicks the logos, so both sides sit through the same logo times, which come from the game's own data. "The
minute" is the 60 s from the first board, while the tour plays the tutorial's words.

**The machine was not quiet and could not be made so.** Every run had 3 to 6 game processes of other sessions beside it
and the machine was 56 to 91 % busy (median 72 %). A busy machine stretches a process's seconds (a shared core, a
slower clock) and not the cycles it runs, so the comparison is in cycles, in runs alternated in rounds (original, the
port after this pass, the port before it), four rounds, with the median and the range of the four:

| | Original | Port before | Port after |
|---|---|---|---|
| To the main menu | 7.8 s (6.7 to 15.2) | 6.4 s (6.1 to 8.7) | 6.7 s (6.4 to 7.1) |
| The minute: cycles | 10.8 G (9.6 to 12.6) | 21.6 G (21.4 to 24.4) | 15.4 G (12.5 to 17.9) |
| The minute: processor time | 5.1 s (4.9 to 5.9) | 10.1 s (9.6 to 11.8) | 7.3 s (5.6 to 8.6) |
| Peak private bytes in the minute | 54.8 MB | 83.2 MB | 82.9 MB |

"Port before" is the build of the commit before this pass (`b6573bea`). On this machine the original's 3.3 s of
processor time a minute (2026-11-08) was 5.1 s, which is why the table says what the same runs gave, not the old
figures. The port now runs 1.4 times the original's cycles, where it ran 2.0 times.

**Where the port's cycles went.** A sampler of the threads' instruction pointers (Bejeweled's
`tools/profile/cpuprof.py`) says only where a thread is when it is stopped, and the game's threads are mostly in waits,
so it showed little but waits (its per-thread totals did hold: the main thread ran about 70 % of the process's cycles,
the graphics driver's thread 15 %). What worked was counting cycles inside the host's own calls
(`QueryThreadCycleTime` around the sleep, the pump and the screen's `Draw`, in a temporary build). Per second of the
first board the main thread ran 200 M cycles. 113 M were the game's own update and drawing (the translated code, which
the original runs too), and 86 M were inside the sleeps, where the framework runs: 41 M of them the copy of the game's
800x600 screen into a picture, 31 times a second at 1.3 M cycles. Two things were wasted there.

- **Looking through the picture for alpha.** The framework's picture of the screen was an ordinary one, so after each
  change it looked through all its 480,000 pixels for transparency and premultiplied and converted it in strips on its
  way to the graphics device. Every pixel is drawn opaque (the copy sets alpha 255), so the picture is now said to be
  opaque and uploaded as it is (`SetImageMode(false, false)`, `mUploadAsIs`, as the Mahjong ports' screen is). That took
  the minute from 21.6 G to about 18.5 G cycles (six runs, 14.7 to 21.6).
- **Showing a screen that had not changed.** The game presents its screen about 30 times a second whether or not
  anything moved, and in the tutorial's board, which waits for the player, two presents in three matched the one before
  (1,513 of 2,364). The widget now compares the game's screen with the picture's last copy (`BonniesWidget::Update`,
  in blocks the vector instructions handle) and, when they match, marks nothing dirty, so nothing is copied, uploaded
  or presented. The comparison is on the pixels, so what is on screen is the same as before; the frozen baseline is
  identical. That took it to 15.4 G.

What is left is the shared code's: the framework's 100 updates a second, the presents of the screens that did change,
the graphics driver's thread (10 % of the cycles after this, 16 % before), the audio mixer's thread (about 4 %), and
the 34 MB of private bytes the process holds before it reads a game file. A dirty-rectangle copy of the screens that
did change was left out: the game's own rectangles cover most of the screen, and a stale rectangle would show what the
original's own SDL screen would not.

**Loading.** The port loads at the original's speed: from the process's creation to the menu it takes the logos'
5.6 s and about a second more, and its `loadResources` takes 0.24 to 0.37 s on a quiet machine (0.5 to 1.2 s on this
one), read from its log. With "Always skip start-up logos" (`-skiplogos`) the menu comes after about 1 s. There was
nothing to take out of it. The 12.9 s of the first measurement was the busy machine; the run-to-run range of the original
here is as wide (6.7 to 15.2 s).

**Memory.** The `memreport` test command (`DEVIATIONS.md`, "Test commands") splits the port's memory at the first
board of the tour: 83 MB private, of which 34 MB were there before any game file was read (SDL, the graphics device and
its driver, the audio device, libopenmpt) and 49 MB are the game's. In that: the resource cache's images 15.4 MB (82
pictures at four bytes a pixel, which are the original's own SDL surfaces too), the dictionary 2.9 MB (121,664 words, two
tables), the samples 0.9 MB (decoded to 16 bits), the engine's screen buffer 1.8 MB, the framework's picture of the
screen 1.8 MB with its texture 1.8 MB, the music's module file 1.8 MB and its player. The process heap held 44.7 MB, its
largest blocks pictures. Nothing in it is a leak or a copy to take out; the 28 MB the port holds beyond the original's
55 MB is about the 34 MB of libraries and devices that the framework's graphics and audio start with, which is shared
code.

### The first measurement (2026-11-08)

The original and the port back to back, original, port, original, port, on a machine 28 to 49 % busy:

| To the main menu | Original | Port |
|---|---|---|
| With the tour's script (the measuring runs) | 6.24 s, 6.36 s | 12.89 s, 6.81 s |
| Without a script (`build/modtest/startup.py`, two runs) | not measured | 6.11 s, 6.15 s |
| With "Always skip start-up logos" (`-skiplogos`) | (no such setting) | 0.98 s, 1.17 s |

| The minute from the first board | Original A | Original B | Port A | Port B |
|---|---|---|---|---|
| Peak working set | 69.9 MB | 79.6 MB | 97.0 MB | 93.8 MB |
| Peak private bytes | 56.0 MB | 58.3 MB | 89.4 MB | 87.4 MB |
| Processor time | 3.28 s | 3.41 s | 8.50 s | 7.08 s |

The original's menu and board are read from its memory, the application at 0x4752a0 with its state at +0x200; the
port's come from the live channel's `state`. An earlier round anchored the minute at the menu and was dropped: the
original's clock-timed tour clicked before a slow menu (12.4 s) had appeared, so that run idled through its minute.

**Not measured: the web build's load.** The only place to time a page is the Browser pane, which is hidden. There a
page's frames and timers run only through the test shims, so no load time taken there would mean anything. The web
build's play was checked instead (`IDENTICAL_PLAY.md`).

## Coverage check (2026-11-08)

The whole check of `docs/COVERAGE.md`. origrun's `--coverage` recorded which of the original's functions ran, in nine
runs on the host (`tools/coverage/run.sh`: the original on its real clock with the silent sound stand-in, so that FMOD's
code runs, its seed pinned as the port's `-seed=1`, in one copy that keeps its saves from run to run; the hits in
`build/coverage/bonnies/hits/`). Every run ended with the game exiting normally, and none made a sound.

- The identical-play scripts: the tour (`tools/origrun/tour.txt`, 869 functions), the action game over its saves
  (787), the resumed classic game (736) and the logos with the menu (700).
- `tools/coverage/extra1.txt` (866): the player dialog (New, Delete answered No and then Yes, Accept), the main
  menu's options with both sliders clicked, a new classic game over the action one with Scramble and a wait for the
  game's own hint, the high scores' page arrow, Back To Game, and the window closed on the board.
- `tools/coverage/logos.txt` (656): a click on each logo.
- Three runs on saves the port made, since the original's boards by the clock cannot be planned: the port's own
  driver (`tools/realmove.py`, with `BB_STOP`, and `BB_LEVEL` for the `level` test command) played to the moment and
  closed the window, which saves the game, and wrote down the word its bot would play next. The original resumed each
  save, whose board it loads exactly as the port's: `lastturn.txt` (742), a classic game at one turn left, resigned
  through the Resign button that takes Scramble's place, to the game's end; `levelend.txt` (794), the word that fills a
  chapter's last cell, through the chapter's end, the story's page and the next chapter's board; and `bookend.txt`
  (807), the same at the end of chapter 4, the first book's last, through the book's end, the publisher's letter and
  the next book.

The full screen checkbox was left unclicked, since the original would change the machine's display mode. The positions
of the clicks came from the port's screenshots of the same screens; origrun takes no shots on the host.

`games/bonnies/coverage.txt` sets the game's own code as one range, the game and New Crayon's engine
(0x401000-0x42f6d0; `ENGINE.md`, "The binary's layout"). The rest of the exe is the import thunks, the C runtime and
the static initializers. The game's own code has 566 functions, and the runs reached 499 of them.

**Reached in the original with no counterpart in the port: 35 at first, then 1 more from the extra runs, 0 at the end.**

- **Placed: 7.** Three are the tails of functions that the export splits off at a jump into their middle (0x404c02 of
  `Dictionary::isWord`, 0x41756b of `NamePanel::handleEvent`, 0x42f35b of `Pak::contains`). Four are small functions the
  port has inline: 0x40e250 (always 1), 0x41f5b0 (a fill of 32-bit words, in `fillRect`), 0x41f4b0 (the getter of the
  last active time) and 0x417b20 (a sound at full volume, in the options' slider). These were comment edits only:
  `tools/check.sh bonnies` still matches the frozen baseline.
- **Left out on purpose: 30 functions, as `none` lines with their reasons.** The engine's string class (the port's `Str` is
  `std::string`), operator new and delete, two swaps, the log function that is empty in the release build and the
  exit's line to it, the handler of the window messages SDL passes on (the host sends none), and implicit constructors
  and destructors. The registration panel and its key check, taken out earlier and never reached, have `none` lines too.
- **No missing game logic was found.**

**Defined in the port, never reached: about 100 at first, 56 at the end.** The extra runs reached the player list and
its list box, Scramble and `shuffle`, the game's hint, the end of a game and `HighScores::add`, a level's and a book's
end with the book end screen, the next level's start, and the logo's click. One Sonnet agent read the 42 of the rest
that hold logic against the export.

- **Read again: 38 match.** Among them the interstitial screen (shown only when Portal.cfg names one), the super-wild
  tile's sparkle, the tutorial hints' other branches (`Book::text`, `pathKind`, `replace`), `HighScores::save` (a
  score that enters the table), the level dialog's `showLevel`, the checkbox, the slider's drag, the list box's
  scroll, the clipboard in a text field, the Print Screen key's event, the window's move and its default place.
- **One difference, now listed.** `drawLine` returns where the original divides by zero (a line along y whose ends
  round to one row), which would crash the original; no play reaches it (`DEVIATIONS.md`, "Faults of the original
  that break play").
- **Three could not be settled from the export**, since its decompiler hides values on the floating-point stack:
  `sparkleTile`'s rounding of its count in one branch (the port adds 0.5 before truncating), `playStream`'s volume
  factors and `framesPerSecond`'s average. The disassembly would settle them; `playStream` and `framesPerSecond` were
  never reached (the music player streams only a track that is not a module, and the frame counter draws only while
  `mShowFPS` is set), and `sparkleTile`'s branch only for a super-wild tile.
- **Code only an error reaches, which stays.** The application's `messageBox`, the slot the original's error paths
  reach (the port has no caller of it).
- **The candidates for taking out unneeded code: 17, taken out on 2026-11-11** (see "Unneeded code taken out" below).

**Never reached and not defined in the port: 12, besides those left out.** The application's four message box layouts
(overridden slots), the registration panel's code, the stream-music branch of the music player and the original's wait
loop, which the host replaces.

**What it cost.** The nine runs took about 25 minutes of machine time, one game process at a time, and the port's three
saves and its probe screenshots a few more. The agent took 3.4 minutes and 135,000 tokens.

## Unneeded code taken out (2026-11-11)

The 17 candidates of the coverage check, none of which any play of the original reaches, are out, and the frozen baseline
is identical (`tools/check.sh bonnies`). The platform base class's defaults that the application overrides, the game's
app being the only platform object, are now pure virtuals with no body: `messageBox` (0x42a580), `isRunning`
(0x4217d0), `getSoundVolume` and `getMusicVolume` (0x4217e0), `windowMoved` (0x4217f0), `getWindowPos` and
`getWindowPos2` (0x401050) and `menuCommand` (0x421800); each declaration's comment says what the default was. With
`Platform::messageBox`, the only caller of `systemMessageBox` (0x41f9d0) went, and with that `Host::messageBox` and its
host side (an SDL message box, answered with the first button in a test run). `isModal` is gone from `Platform` and
`App` (0x421240, a slot nothing calls); `BonniesApp::menuCommand` (0x401e00, the debug menu's commands, which SDL's
window has no menu to give) went with the event type that carried it, which the host never sends; `openURL` is gone from
the engine and the host (its only callers were the registration panel's links); and so are `Widget::setImage` (0x42c410,
a slot nothing calls) and the `Picture` widget (type 2, 0x42f530 to 0x42f560, which the game never makes). The
application's own `messageBox`, which only an error path of the original reaches, stays.

## The readability pass (2026-11-11)

`python tools/namecheck.py games/bonnies` finds nothing now (it found 852 before, once `namecheck.txt` added the game's
`engine/` and `game/` folders beside `src/`; the file holds those two `folder` lines and no exception). The object code
of all 31 units is what it was (`tools/objsnap.py`: 840 functions, 0 objects differ), no string literal of the sources
changed, and the frozen baseline is identical (`tools/check.sh bonnies`).

**Names.** Bonnie's has no Mac build, so every name comes from how the code uses the thing, read against the original's
function. The 98 members that were named by their offsets (`mFieldC5C`, `mStrCC8`, `mDebug1E8`) are words now, each with
a comment saying what it holds, in `Game.h` (the player's record and the game), `GameScreen.h` and the smaller classes.
The ones worth knowing when reading the game:

| Name | What it is |
|---|---|
| `Game::mClassic` | 1 in the classic game (no clock: `mGoal` counts the moves left), 0 in the action game (the clock runs, `mClock` falls from 1 to 0) |
| `Game::mClock`, `mClockLag` | the action game's time left, and the time a long word gave that the shown clock has not caught up with yet (the shown clock is their sum) |
| `Game::mMovesLag`, `mCountTime` | the classic game's moves given and not yet counted into the display, and when the count last ticked |
| `Game::mLevelBestWord`, `mBestWord` | the longest word of the level (the summary shows it) and of the game (the high score table's "best word") |
| `Game::mWordsPlayed`, `mGameWords`, `mShuffles` | counters the tutorial's hints test: words on this level, in this game, shuffles on this level |
| `Game::mChangeTimes`, `mOldLetters` | when a tile last changed and the letter it had (a stone counted up, a copycat copied, a fixed tile freed, `'^'`), for the animations |
| `GameScreen::mSequenceState` | the screen's state: 1 in play, 2 the level won, 3 to 16 the steps of the summary after it, 0x11 to 0x13 the end |
| `GameScreen::mBoardFade`, `mSummaryFade`, `mLeaveFade`, `mBookOpen`, `mSummaryDark` | the fades the sequence's steps run, 0 to 1 |
| `Record::mWords`, `mLetters`, `mLongestWord`, `mBestStreak`, `mStreak` | the player's statistics page: the average word is `mLetters / mWords` |

Names that are judgements from the code's use and not from anything the original says are `Game::mWildFell` (a `?` tile
came from the super wild odds and its hint is due), `Game::mMovesAtHint`, `Book::mTwoFills` (a tile takes two words to
mark, the flags 0x10 then 0x20), `Book::mFastOdds` (a long word adds more to the special tiles' odds), `GameScreen::mClicks`
and `Platform::mShimmerWaits`. The three debug menu flags (`BonniesApp::mFreeMoves`, `mAnyWord`, `mNoHints`) keep the
menu item they belonged to in their comments; nothing in the port sets them.

**Fields nothing reads (rule 3).** These stay, named for what they hold or sit beside, with a comment saying that nothing
reads them: `Record::mSpare` (saved with the record), `Options::mWindowSpare` (saved with the options), `Game::mWordFrame`,
`Game::mPathClearTime`, `Graphics::mModeFrame`, `BonniesApp::mPrevState`, `BonniesApp::mRedrawCount`, `Music::mAlive`,
`MusicTrack::mSecondName`, `Pak::mInUse`, `GameScreen::mSummaryStart`, the `Platform`, `Input`, `ListItem`, `Sample`,
`SoundSystem` and screen spares, and the startup scratch object (`gStartupScratch`, made and freed and never used).
`tools/splitfields.py` finds no member declared twice at one offset and none that is never written; its export check
lists six members (`GameScreen::update`'s picture and chapter, and `Game`'s path rows, path length and level), which the
original's functions touch directly and the port's reach through the accessors it translated separately (`pathRow`,
`pathLetter`, `currentBook`) or through the helper that holds the code.

**One-letter names (rule 5).** Fifty-nine locals of the drawing code (`Draw.cpp`, `Graphics.cpp`), `Str.cpp`'s `addColor`
and a few parsers were one letter over many lines; they are words (`aPixelPtr`, `anAlpha`, `aSrcPixel`, `aCornerRadius`,
`aRamp`, `aCornersX`) and the colour components beside them follow (`aRed`, `aGreen`, `aBlue`). The unused parameters
that were `theUnused` are `thePassedOn` (the sound call's flag, which the original passes through and never reads) and
`theIgnored`; the rest of the stand-ins are `aNewFile`, `aNotUnderstood`, `aSecondFlag`, `aSkipped` and, for the intro's
two animation clocks at 0x478538 and 0x47853c, `gHandsTime` and `gHandsFrameTime` (beside the mouth's and the head's
`...Time` and `...FrameTime`). `BonniesHost.cpp` and `ModFeatures.cpp` carry the port marker in their first comment
(rule 6).

**Numbers named (rule 7).** `game/Board.h`: `BOARD_COLUMNS` and `BOARD_ROWS` (9 each: a cell is
`column * BOARD_ROWS + row`), `BOARD_CELLS` (81), `PATH_LETTERS` (20, the most tiles on the path and in a hint's word)
and `LEVEL_COUNT` (50), about 190 uses in the game's sources. Left as numbers: `Book::mBannedIn[9]` (what the nine are
is not settled), the spawn store's `2 * (0x13 + 9)` in `refill`, and every other 0x14 and 0x32 (a flag, a colour step, a
50 ms step, a window position).

**How it was done.** Compiler-driven, as `docs/CODE_STANDARDS.md` says: the declarations were renamed class by class,
every translation unit was compiled with `-fsyntax-only -ferror-limit=0`, and the old name was replaced exactly where the
compiler reported it missing (the class named in the message keeps two classes' `mFieldAC` apart; an unqualified use
takes its class from the function around it). The one-letter locals were renamed over the block that holds them by a
second script that reads code only, not comments or strings, and refuses a new name that the function already has. The
five sites in a constructor that the first script could not place (`Input::Input`, `Music::Music`,
`ListItem::ListItem`) were fixed by hand. Neither script is in the repository; the shared notes folder holds them.
