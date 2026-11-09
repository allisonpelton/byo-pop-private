# Water Bugs: where the port differs from the exe

Each entry is a place where the code is not a line-for-line translation of `WaterBugs.exe`, with why it has no visible
effect. The original's behaviour is kept everywhere else, including what looks like a fault (the options' easiest
difficulty draws "5s" where a `%s` was meant, `Menu_FromXml` leaves a menu's type unset when the file has none).
Comments of the form "(the original ...)" in the code mark each one. The list was made on 2026-10-31 from them.

## Host and system

The pointer, the screen, the clock, the sounds, the music, the files and the save folder go through `r64::SysHost`, so
DirectDraw, BASS, the window's messages and the system's pointer are not translated: the port draws the same 800x600
16-bit screen and presents it. The original waits for the display's refresh when asked; the port's host paces the
update clock, so a frame's time differs but not its updates. The original lets the screen be lost and restored
(0x7081b0); the host's screen is never lost. BASS's output volume (60%) is the host's volume setting. Saves go to the
player's folder under the application data, not the game's folder (the file formats are the original's).

## Calls that did nothing

A play call is given a pan worked out from x where the sound function takes none (the pan is dropped). The original times
each part of a frame for nothing, copies the screen into an image that nothing reads (0x41eb80), calls an empty function
(0x401410) after drawing, and stops the sounds when the window closes unless a flag that nothing sets is on. None of it
reaches the screen or a sound.

## Memory and bounds

Fixed buffers the original fills without a bound (the bank's names, the XML tags' names and values, and a menu's
text buffer, which holds 16 characters) are cut to fit. Only a file or a name longer than the buffer notices, and the original
corrupts memory there. The XML reader needs its buffer to end in 0; the port's file reader adds the 0. A FLIC frame or
chunk that runs past its block ends the frame (the original reads on), and colours past the palette's end are dropped
(the original writes into the globals after it). The emitter's members past the base start cleared where the original
leaves what the allocation held; a column of cells has 38 cells where the original's has 37 (the extra one is never read
by a valid board). The original's save writer leaves a file open and closes another twice; the port closes each once.

## Leaks the original has and the port does not

The game file's buffer was never freed (every board file read leaked it) and neither was `local.xml`'s. Nothing keeps a
pointer into either, so both are freed after their last use (2026-10-31).

## Words and pointers

Types are the port's own (32-bit layouts in comments, pointers where the original has addresses). The original's
sprintf of the drawn text onto itself works on MSVC's, which copies as it goes; the port keeps that with `AppendTo`.
The original shares green's store with an object that has no colour: the port gives it tint 5, which is what is read.

## Not yet compared with the original running

None of the above has been checked against the original exe running (`tools/origrun` is to make that possible). The
screens, texts, music and sounds, and the loss and regain of the window's focus, are still to be compared one by one.

## The order of the random draws

Checked on 2026-10-31 by comparing, for each of the 42 functions of the exe that call `RandRange`, the sequence of the
calls' ranges with the port's functions. 34 have a port function with the same sequence; the other eight are the same
draws through a loop or a helper (the original writes a loop out twice or three times, or draws inline what the port
does in `Sweeper_Spark` and the power-up's sparks), except `Emitter_Update` (0x40b940), whose many branches were
read by eye against the exe and not yet by a run. One real difference was found and fixed: a random board's half
width and height were drawn from 3 to 10 and 2 to 6 where the exe draws 2 to 10 and 1 to 6, so random boards
differed. The frozen baseline of the port is in `tools/check.sh waterbugs` (the state lines and the hashes of three
screens: the board's start, after a beam, the high scores after a lost last life).


## Compared with the original running (2026-11-01, in progress)

The original runs under `tools/origrun --sandbox` with the screen read from the game's memory at a given update, its
mouse input replayed in the port by the update that read it (`WB_NOWARP=1`: the sandbox's cursor cannot be moved, so the
game's own cursor warps are ignored in both). The title screen matches pixel for pixel at three updates. The random
seed matched the original's at every draw count checked until the world map (after the title's click: 6133 draws in the
original, 6124 to 6154 in the port within the same few updates). The Rand function divided the seed signed where the
exe's `div` is unsigned; the port's seed sequence diverged from the original's once a seed had its top bit set, which
changed boards' bubbles and the beam's fill. Fixed (12d8077): both now agree draw by draw.

The original under origrun runs about a millisecond per drawn frame (its elapsed-time global read 0 or 1 at every
sample), and the world map's water and the like step once per drawn frame, so the port's test runner has to draw a frame
per update (`WB_FRAME_MS=1`; its default of 10 ms is what `tools/check.sh` froze). With that, 31 of 33 shots of the
original (the title, the world map, the level's start text, the beam, the fill to 84% and a long idle with clicks) are
identical to the port's at the update they were taken, to the pixel. The rest: a dark 2 by 30 strip at x 766 and y 531
that the original draws over a bubble where the port draws the bubble over it (three shots; the draw list's address
tie-break, as the loss screen's bubbles), and one shot taken while the fill's score read 344 against the port's 343 (a
cell's fill one update apart, back in step by the next shot). A shot taken in a burst can read a counter an update or
two later than the screen it holds, so one shot off by one update is not a fault.

A second run (a fresh profile: the original keeps its saves, and with a level reached it opens the starting-level
screen where a new player gets the crash site, so its saved profile is removed before each run) played 100000 updates of
clicks spread over the board: six levels, all six lives lost, the game over and the hall of fame. Replayed in the port by
the updates that read each click, 27 of 47 shots are identical, 15 more differ by 3 to 237 pixels, and five by 595 to
1035. Every screen from the game over to the end of the hall of fame is identical. What still differs is small white
and lilac glints (a few dozen pixels each) near pipe ends and powerups on the later levels, in different places in the
two games; the sparks the port makes through its particle systems are not in that window, so they are another object's
and are still to be found. The random draws stay in step through the whole run (the screens that follow are the
same).

Sound and focus. The exe's 29 calls of the sound-play function, and the calls of the music functions (volume 5, load 4,
free 3, play 5), are matched by the port's calls in number. BASS did not reach the sound stand-in in the sandbox, so the
sounds could not be recorded and compared by state. The focus loss and regain were run in the original (`focus lose`
and `focus regain`): its update counter kept running while the focus was lost (23165, 23411 and 24200 at three shots
taken while it was lost), so the game does not pause, and only the music pauses; the port does the same (`Sys_Activate`
pauses the music and stops the sounds, and the frame loop does not look at the focus).

The sign's translucent fringe (2026-11-01): Screen_Redraw drew every menu's images onto the level background again on each call, which piled up the sign's translucent edge. The original walks the menu list from the current menu to the end and then resets current to the head; the port now does the same, and the main menu and the Help screen match the original in every pixel (0 differing, was 1568 in the sign region).

The text and dialog screens (2026-11-01): the main menu, both Help pages (Instructions and Power-Ups), the Hall of Fame (while it fades in and fully shown), Change User with its name-entry dialog and Options match the original with 0 differing pixels at the original's own update counters (replayed input, a fresh profile, 1 ms frames), except 3 pixels of at most 9 in the sign's bubble on one Change User shot. Options matches only with fullscreen="1" in cp.cfg: the original's default is full screen, while the test runner writes a windowed cp.cfg when none exists (harness choice, not the game). The Quit button leaves the game at once in both, with no requester.

The web build (2026-11-01): the rebuilt page (`?silent=1`) loads its wasm and shows its start page with no console errors. With the game's install folder zipped and set on the page's zip input from script (189 files copied), the page started the game and drew the main menu and the Options screen with the same texts as the original ("Video: Fullscreen" and the rest); a click-through of further screens was not finished (an import panel of the page stayed in front), so no web screen is compared pixel by pixel; the web build runs the same game code as the native build that was compared.

## Added in the finishing pass (2026-11-08)

None of these shows in play with the mods off; `GAME_NOTES.md` has the detail.

- **Registration and the buttons that lead out of the game are not translated.** The trial's clock, the registered flag and the
  branches on it, the "buy now" and website buttons with the shortcut reader and the URL opened at the window's closing, the
  host's `OpenURL`. The save file keeps the trial clock's four bytes (3600000, as the registered game leaves them) so a file of
  the original and a file of the port stay interchangeable. The nag screens (levels 8 to 10) are parsed and never visited.
- **The mods are loaded.** The host reads a mods folder (`FixOn`, `Sys_FixOn`) for the fixes mod: `steadyClock` (the game steps
  10 ms for each update of the framework instead of the machine's clock) and `easyLabel` (the name of the easiest difficulty).
  With no mod the game is the original and the baseline is unchanged.
- **The focus pauses the game in a real-input run and on the page.** `IsActive()` followed the test run, which kept the game
  running when the window lost the focus; it follows the window now, as the original's `WM_ACTIVATEAPP` does. A test run
  without `-realinput` has no window events and stays active.
- **The site's "Always skip start-up logos".** With the setting on (or `-skiplogos` natively) the start-up logo is left as Esc
  leaves it, in the first update. With it off, which is every other run, the game is the original.
- **The test hooks** (`src/testhooks/`, `level`, `goto`, `set`, `win`, `lose`, `seed`, `move`, `grid`) are reached only from a
  test's script, command file or the page's `byopop.test.command`. A game that is never given a command never runs them. They
  draw no random number except `seed`.

## Oddities (not chased)

Pipe-end glints (2026-11-01). On about 20 of the later-level shots, the sparkles at the pipe ends differ slightly from the original's (tint and position, tens of pixels per shot; the frames around them are identical). They are not particles (no ParticleSystem_Add call falls in the window) and the cause was not found after a long look. To see them: replay one of the original runs under origrun with the run's own input counters (`docs/ORIGRUN.md`, `re` scratch scripts) and compare with `tools/shotdiff`. Matching them would need values measured from runs, so they stay as they are.

The logo panel's strip at x 766, y 531..560 differs by the order of two draw-list entries at the same depth (an address tie-break in the original); one fill cell differs by one update in one run (z24000). Neither has a visible effect.

The sound of the original could not be recorded (its audio library does not reach the sandbox's audio stand-in), so sound is compared by call counts only (see above).

The world map and a click without a hover (2026-11-08). In the coverage runs (`GAME_NOTES.md`, "Coverage check") the original lost a click that came with its mouse move in one update on the world map's level-set buttons three times (the level selection never started), and once ended with an access violation (`held269`). With the button hovered 300 steps before the click it did neither. The port starts the board in every case. A real mouse moves before it clicks, so it is not chased.
