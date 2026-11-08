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
