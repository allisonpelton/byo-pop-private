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

Measured 2026-11-08 with `tools/perf.py` (its docstring has the commands), the original and the port back to back:
original, port, original, port. The original ran under origrun on its real clock, from a fresh copy of the installed
game each run, with its seed pinned as the port's `-seed=1`. The port ran natively under rundesk without `-fast` and
without mods. Both played the same tour: the port's `tools/tour_test.txt` by its updates, the original's
`tools/origrun/tour.txt` by the clock. The script samples the game's process every 0.1 s from outside: working set,
private bytes, and kernel plus user time. The original's menu and board are read from its memory, the application at
0x4752a0 with its state at +0x200; the port's come from the live channel's `state`.

**The machine was not quiet.** One or two game processes of other sessions ran throughout, and the machine was 28 to 49 %
busy. The performance pass will measure again on a quiet machine.

**Loading.** "Loaded" here means from the process's creation to the main menu, the first screen that takes input other
than the click that hurries a logo. Nobody clicks the logos, so both sides sit through the same logo times, which come
from the game's own data.

| To the main menu | Original | Port |
|---|---|---|
| With the tour's script (the measuring runs) | 6.24 s, 6.36 s | 12.89 s, 6.81 s |
| Without a script (`build/modtest/startup.py`, two runs) | not measured | 6.11 s, 6.15 s |
| With "Always skip start-up logos" (`-skiplogos`) | (no such setting) | 0.98 s, 1.17 s |

Most of that is the logos (about 5.6 s). The port's own start takes about 0.5 s to its first frame, and its
`loadResources` 0.24 to 0.37 s, read from its log. Its 12.89 s run was the busy machine: in the script-free runs straight
after, the port reached the menu in 6.1 s both times.

**The scripted minute.** The minute is the 60 s from the first board, while the tour plays the tutorial's words.

| The minute from the first board | Original A | Original B | Port A | Port B |
|---|---|---|---|---|
| Peak working set | 69.9 MB | 79.6 MB | 97.0 MB | 93.8 MB |
| Peak private bytes | 56.0 MB | 58.3 MB | 89.4 MB | 87.4 MB |
| Processor time | 3.28 s | 3.41 s | 8.50 s | 7.08 s |
| Other game processes (start, end) | 1, 1 | 1, 2 | 1, 1 | 2, 1 |

The port uses more of both. It holds about 30 MB more private memory, and it takes twice the processor time or more:
the causes were not looked into. That belongs to the performance pass, as does a quiet machine.

An earlier round anchored the minute at the menu, and it was dropped. The original's clock-timed tour clicked before a
slow menu (12.4 s) had appeared, so that run idled through its minute (2.1 s of processor time). The files are in
`build/perf/old-*`.

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
- **Code only an error reaches, which stays.** `systemMessageBox` and the application's `messageBox`.
- **Candidates for taking out unneeded code: 17.** No play of the original reaches these, and they were not removed
  now: the platform base class's defaults that the application overrides, as the only platform object
  (`getWindowPos`, the four `messageBox` layouts, `Platform::messageBox`, `isRunning`, `getSoundVolume`, `windowMoved`,
  `menuCommand`); `App::isModal`, a slot nothing calls; `BonniesApp::menuCommand`, the window menu's commands (SDL's
  window has no menu); `openURL`, whose only callers were the registration panel's links; `Widget::setImage`, a slot
  nothing calls; and the `Picture` widget's destructor, `init` and `draw`, a widget the game never makes.

**Never reached and not defined in the port: 12, besides those left out.** The application's four message box layouts
(overridden slots), the registration panel's code, the stream-music branch of the music player and the original's wait
loop, which the host replaces.

**What it cost.** The nine runs took about 25 minutes of machine time, one game process at a time, and the port's three
saves and its probe screenshots a few more. The agent took 3.4 minutes and 135,000 tokens.
