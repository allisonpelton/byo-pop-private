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
