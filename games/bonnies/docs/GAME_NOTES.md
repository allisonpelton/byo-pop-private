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
logo still stands 1.5 s and fades), so the whole sequence takes several seconds whatever the player does. The bug that
changes play is the clock: every timer of the game is the machine's, read each frame (`Game::update` moves the falling
tiles by the time since the last frame, capped at 120 ms; the action game's clock and the end of a level's count run
the same way), so the same seed and the same input play out differently from one run to the next, and more so on a busy
machine. The one fault that broke play, the players panel's Delete that removed nothing, is fixed in the port itself
(`DEVIATIONS.md`, "Faults of the original that break play").

| Toggle | The original | With the fix |
|---|---|---|
| `steadyClock` | The game reads the machine's clock (`ticks`), so its frame times, and with them where the falling tiles are, when a level's count ends and how fast the action game's clock runs, vary with the machine's speed and load. | The host's clock (`BonniesHost.cpp`, `GameClock`) is the framework's updates, 10 ms each, and the game's sleeps run to it, so a frame of the game always spans the same updates: the same seed and the same input play the same game on every run. The game still runs in real time. A `-fast` run is steady already. |
| `skipLogos` | The New Crayon and PopCap logos run for several seconds before the menu; a click shortens them but each still stands 1.5 s. | The menu comes as soon as the game's resources are loaded (`LogoScreen::update`). |

STEADY_RESULT

## Performance

PERF_RESULT
