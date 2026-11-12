# TipTop Deluxe: notes on the original

What the translation found in `TipTop.exe` (2003-02-01), for whoever reads or changes the port. Addresses are the
original's; the port's declarations carry them as `// 0x...` comments, so a grep finds both sides.

## How the program is built

The program is C-style code in C++: global objects, structs with methods and a few vtables, no class hierarchy to
speak of. The start-up (0x412b90 and around) reads the preferences, starts LOWOS and the GUI and builds the app;
then `App::Frame` (0x41b9fe) runs once a frame until the game sets its quit flag.

LOWOS (0x42d000-0x438000) is the engine: the screen (DirectDraw, a 640x480 mode of 32 bits for TipTop), the blitter,
input queues of 16 events for the keyboard, the typed characters and the mouse, the sound buffers and the clock. The
resource manager (0x438390-0x439d70) loads named entries of `TipTop.dat` on first use, converts them by type (PCX and
JPEG to images of the display's format, Ogg Vorbis to WAV images, text) and counts references; the game loads and frees
them by groups.

The GUI (`src/Gui.cpp`) is a list of handlers, each a procedure with a few slots of its own, a rectangle, a layer and
a pass.
Messages go to every handler: create, destroy, mouse down, move and up, key, update, draw, typed characters, and
the set and get value of sliders and text fields. Layers can be pushed as modal; then input goes only to the top
modal layer, and drawing runs in groups from the layers outside every modal one up to the top, six passes within each
group. The screens are built from the handlers of a few procedures: images, shaped buttons (their hit test is the
image's color key), plain buttons, labels, sliders, check boxes, text fields and the many animations of
`src/App.cpp`.

The commands (0x41f922) are what a button sends: its second create argument holds the command in the low bits, and
flag 0x80000000 makes Enter press it, 0x40000000 Escape (both: 0xc0000000).

## The application

One `App` object (0x488710, 0x198 bytes) holds the screen in play and its layer, the board, the fonts, the bar
images and the handlers that need to be found again. The screens are numbered: 1 splash, 2 main menu, 3 options,
4 instructions, 5 credits, 6 game, 7 level summary, 8 new game, 9 high scores, 10 game over. `App::SetScreen`
(0x41c8f5) frees the old screen's layer and builds the new one.

The game modes are set by commands 0x20-0x23, each with its own list of levels: 0x20 regular (mode 0), 0x21 speed
(mode 2), 0x22 timed (mode 1) and 0x23 strategy (mode 3). The configuration places the timed mode's button at
(1000, 1000), off the screen, so TipTop Deluxe shows three modes; the timed mode's rules (a meter that empties with
time, the level won when it reaches the goal, the game lost when it runs out) are all in the board. Its high score
rows on the main menu and the high scores screen are at y=1000 too. Drawn on a larger screen (the port's `-canvas`),
they are the names, scores and levels alone, since the panels and the mode's names are parts of the background
pictures, which have none for the timed mode; and its button's picture (`ButtonNewGameTimed.pcx`) is a 16x16
placeholder, two states of 16x8 like the other buttons' stacked pictures, where theirs are about 130 pixels wide.

The game's random numbers are MSVC's `rand` (0x44ccaf), seeded once with the tick count (0x411fff); the port seeds it
with `-seed` when given, which makes the bot's games repeatable.

## The board

`src/Board.h` describes the four structures with their original layouts: the piece (0x5c bytes), the field (0x96c
bytes, the grid and the rules), the state (0x12b4 bytes, the level in play and the board's own copy of the
configuration, into which each level's values are written) and the board (0x1658 bytes, the previews, the layout,
the level changes, the bonus rounds and the save). The grid is indexed by row * columns + column. Row 0 is where the
pieces settle, drawn at the top of the playfield; the player's block enters at the bottom and rises into the column
it is sent to. Kinds 0-9 are the fruit; 10-16 are the special blocks (rocket, exploder, hammer, magnet, rainbow,
color bomb, column nuke) and 0x11 is none.

Outside the timed mode a level is won when no block is left and lost when a column is full; the configuration's
winning precedence decides which of the two is checked first when both happen. The modes differ in their lists of
levels (each level's record sets the level's values in the board's configuration) and in the ramps that make the
special blocks and the random floaters more frequent once the list has run out. The bonus round has its own timer,
layout and queue of blocks, which the save file keeps.

## The save file and the preferences

`savegame.dat` (0x41efbb, read by 0x41f499 and by the new game screen's summary) is a sequence of tagged fields: the
date, the mode, the level, the best chain, the score, the bonus round's time, score, queue and layout, and then the
board. Continuing a game deletes the file. The preferences (volumes, full screen, tips) are a file of their own
written at exit. Both went to the working directory; the port puts them in the user dir.

## Data records worth knowing

An effect (0x24 bytes in the configuration's effect lists) is a particle kind, a delay, a count, a speed, the levels
it is shown at (max at +0x10, min at +0x11, bytes), a pulse period (+0x14) and a fade-in, hold and fade-out
(+0x18, +0x1c, +0x20). A particle kind gives a size and, at +0x1c, whether its image has an alpha byte per pixel.

The blitter (0x437e3c) picks one of 32 routines: (format class << 3) | (mode << 1) | translucent, with mode 0 plain, 1
an alpha byte before every pixel, 2 a color key (0xfefe01 in the display's format) and 3 additive. Two tables made at
start drive the blending: 0x47ebc0[(difference << 8) | alpha] = difference * (255 - alpha) / 255 and
0x47ebc4[b * 256 + c] = b * c / 255.

## Test commands (docs/TEST_COMMANDS.md; `src/testhooks/Commands.cpp`, `Bot.cpp`)

Script commands of the port (`-script`, and the live channel `-commands` natively and `byopop_test_command` on the web), each
answered on stdout with one line: `OK <command>`, `ERROR <command>: <reason>`, `STATE {...}` or `MOVE {...}`. A command that is
never given changes nothing, and none but `seed` touches the game's random numbers. `tools/check.sh tiptop` (the bot's games,
and the frozen baseline) gives the same results as before them.

| Command | Does |
|---|---|
| `state` | `STATE {"screen","mode","over","dialog","busy","paused","inactive","updates"}` (`inactive`: paused because the window lost the focus, which a driver ends by giving it back; `updates`: the game's update count, for a driver that waits n updates), and on the board screens `"level","score","bonus","blocksleft","levelComplete","gameOver"` and `"board":{"columns","rows","next":[4 kinds],"grid":[rows of characters]}`. `screen` is title, menu, options, help, credits, newgame, board, bonus, summary, highscores or gameover; `mode` regular, strategy, speed or timed. A grid character is the kind of the block at that cell (`0`-`9` the fruit, `a`-`g` the specials 10-16, `.` empty), row 0 first. `over` is true once the game over screen's Play again button is up (after the high score's name, if any). |
| `goto <screen>` | menu, options, help, highscores, credits (the game's own screens), or a mode to start: regular, strategy, speed or **timed**, through the game's own start commands (0x20-0x23). The timed mode's button is off the screen of TipTop Deluxe (configuration), so this is the one way in. From the title screen it goes through the title's button first (ERROR while the title still loads); ERROR while a dialog is open. |
| `level <n>` | level n of the game in play, as the game goes on after a bonus round (`NextLevel`, then the game screen built again). ERROR with a dialog open, an animation running, in a bonus round or at the end of a level. |
| `set <name> <value>` | `score`, `blocksleft` (the board's), `mode regular\|strategy\|speed` (the mode `move` picks on the new game screen), `botseed <n>` |
| `win`, `lose` | the board's own flags, `mLevelComplete` or `mGameOver` (and for the timed mode `SetCrossed`), that `Board::Draw` sets itself; the level summary or the game over screen follow as in play. ERROR in a bonus round and while the game is busy. |
| `seed <n>` | `tiptop::SRand(n)`: the C library generator (the game reseeds it from its clock when it chooses the first block, so a seed set after a game began holds only until then; the fixes mod's `seededBlocks` removes that) |
| `unlock all` | ERROR: the game has nothing locked |
| `move` | `MOVE {"kind","input":[{"move":[x,y]},{"click":[x,y]}],"updates"}` (the steps are docs/TEST_COMMANDS.md's ordered list; the high score's name is `{"key":"B"},{"key":"o"},{"key":"t"}` before its click): what the bot does next on this screen, in game coordinates. `kind` is start (the title's button), play, mode, dialog (the tips dialog's default button), drop (a column; `column` is the number), continue (the level summary's button), unpause (the pause image is up: a click on it; while the window is inactive it is wait), name (types "Bot" and clicks OK), or wait (no input, `updates` to wait: the game is busy). It answers for the whole game, from the title to the game over screen, and is `MOVE null` only once the game is over. The column is `-autoplay`'s choice (`BotChooseColumn`: the biggest group the block joins, else one of the emptiest columns), with dice that are a function of the update and `botseed`, so asking twice gives the same move. |

**`move` and the title screen.** `move` waits (a wait move of 10 updates) while the title's load bar fills and offers the
play button's click once it is full, and `goto` from the title goes through it to the main menu (it answered ERROR "click
its button first" from the title, which stopped a driver that began with `goto <mode>`).

## The sound log (BYOPOP_SOUNDLOG, for the site's presets)

`BYOPOP_SOUNDLOG=<file>` writes the framework's sound log (`tools/soundpreset.py` reads it): a `load` line for each of
the 36 sounds, by the name TipTop.dat stores it under (`sounds/MouseOver.ogg`), then `play`, `stop` and `volume` lines
with the update they happen on. LOWOS has no instances, so each PlaySound call of mode 1 or 2 that starts a sound is a
play, with the amplitude it was asked for in hundredths of a decibel (the loudest play of a log is the reference, so the
sound effects level drops out), and mode 0 or a restart is a stop. Whether a sound is still playing (mode 2 starts it
only when it is not) comes from its length in updates, not from the mixer, which keeps the machine's clock in a `-fast`
run: the board's shake and time warning sounds, restarted every update with mode 2, come out as a play each time the
sound ends, as they sound at the game's own speed. Nothing changes without the variable, and the old `-soundlog`, which
the sound comparison with the original reads, is what it was. `games/tiptop/tools/soundlog/` has `run.sh` (one run,
through `realplay.py`: a whole game of a mode by the bot's `move` from the title, a phase-two tour, or `options.py`, the
options' two sliders dragged), `all.sh` (every run the site was given) and `index.py` (a table of contents of a log: each
sound's plays, and the bursts of plays at most 30 updates apart, which are the sequences a preset is made of).

## Deviations from the original

The port is the original except for what is listed here, each with its reason.

**Loading at the machine's speed** (`LoadBarProc`, `src/App.cpp`; the user, 2026-11-07: "Let's make faster loading a core part of
the recomps instead of a mod"). The original's title screen loads one of its 229 sounds, music tracks and texts a frame, and only
when more than the bar's step (1 ms) has passed since the last, so its bar fills at a crawl however fast the entries load. The port
loads, each frame the bar is drawn, as many entries as fit 10 ms of the game clock (`kLoadSliceMs`), in the original's order, so the
bar still draws between the slices and the window stays alive; like the original it waits while a dialog is over the bar; on a clock
that stands still (under `-fast`, or while the game is paused) it loads them all at once. The bar and the "click here to play"
button come sooner, and nothing else changes: loading draws no random number. The numbers are under "Performance" below.
One consequence for the tests: the game seeds `rand` from the game clock when the first block is chosen (`seededBlocks` is the
mod's fix for that), so a game after a title screen of another length is another game. With the blocks following the seed (the
fixes mod on), the check's three mouse-played cases give the old build's state logs, update for update, shifted by the 202 updates
the title takes less under `-fast` (`tools/perf/equiv.sh`, which compares the pair with `tools/perf/cmp_offset.py`). Without the
mod the three cases play other games than before, so `tools/check.sh`'s expected results (`TT_REGULAR`, `TT_STRATEGY`, `TT_SPEED`)
and the frozen baseline moved with it: `build/baseline-tiptop/` is the port at the commit of this change
(`FROZEN_AT.txt`), the phase-one baseline kept beside it as `build/baseline-tiptop-phase1/`.

**No "Visit PopCap.com for More Games" button on the options screen** (the user, 2026-11-07, "Yes, now too": every button that
leads out of a game comes out of the ports). The original's options screen has an oval button between the TipTop logo and the
Resume button that opens the publisher's page (command 0x37, the configuration's `PUBLISHER_URL`). The port drew it and opened the
page through the framework; now the button, the command, the address and the two settings of its position, `OpenBrowser` (the
helper that opened the page) and the button's image (0x5b, `BUTTONOPTIONSVISITPOPCAP.JPG`, left out of the resource table with
the registration's) are gone, so the port opens no web page at all. Nothing needed moving or widening: the button floated on the
screen's pattern with no row beside it, so without it the pattern runs on, and the Resume button keeps its place. The frames that
show the options screen differ from the original's only in the button's box (407, 313 to 584, 396): in tour 1 `options`,
`options_tips_toggled` and `options_tips_back`, in tour 3 `game_options`; every other frame of every tour is as before.

## Registration out (2026-11-08)

The original is the trial of a game that is sold: it asks `gRegistered` (0x48870a, set from a registry value and a file in the
Windows folder unless the configuration's `DISABLE_UNREGISTERED` byte says otherwise) at eight places, and everything behind a
zero is a trial. The port always played as registered and left out the registration dialog from the first translation; this
pass takes out what was left, so that no branch of the registration remains. The flag is gone, every branch is folded into its
registered path, and what only the other path used went with it:

- **The splash** (`App::SetScreen`, `SCREEN_SPLASH`): the register button (image 0x4d, command 0x30), the two unregistered
  pictures ("Unregistered" and "Trial Version", 0x55 and 0x65), the "N more trial plays remain" line and the "You have played
  N times!" line (the trial's count of plays; the original's splash shows no text of it, and the port never kept the count),
  with their positions in the configuration and the call to the "no more plays" check. The picture that the registered path
  drew (image 0x66, at `SPLASH_REGISTERED`) stays: the user's rule (2026-11-08, "'Thanks for registering' should be considered
  part of the registration/trial code to be removed from every game") is about a thank-you, and this picture thanks no one. It
  is the word "Deluxe" of the title's logo, which the trial replaced by "Trial Version"; without it the title would read "TipTop"
  and not the game's name. The title is now the picture, the logo's word, the load bar, and (once it is full) the play button.
- **The load bar** (`LoadBarProc`): the trial added a wait to the bar (the configuration's `INITIAL_WAIT` and `ADDITIONAL_WAIT`
  times the count of plays); folded, the bar is as many steps wide as there are entries to load.
- **The options screen:** the register button (image 0x5a, command 0x30).
- **The quit and save questions:** `AreYouSureRegDialog` and `SaveGameRegDialog` (the questions with a register button, images
  0x70, 0x7b and the buttons 0x6d, 0x6e, 0x6f, 0x7e) were the ones the original asked of an unregistered player; the port asked
  the plain ones (`AreYouSureDialog`, `SaveGameDialog`) and now calls them directly.
- **The "no more plays" check:** `App::TrialCheck` and its dialog (images 0x6a, 0x6b, 0x6c), its five callers (the four game
  modes, play again and continue) and the count of plays (`CountPlay`, `gRunCount`, `App::mDialog`).
- **Commands** 0x2f and 0x30 (the registration dialog's cancel and register) and 0x36 (open the registration page): nothing sends
  them. Command 0x37, the options screen's link to the publisher, went the same day under its own rule (see "Deviations").
- **The configuration** (`Config`): the splash's and options screen's register positions, the unregistered and registered
  pictures', the executions-left and played-times lines', the "no more executions" dialog's, the two register questions', the
  registration dialog's (sixteen members the port never used), the three execution-limit settings, `DISABLE_UNREGISTERED`, the
  registration page's address, the registry key and file names of the run count (`RIS_*`) and, with the link out, the publisher's
address and the options screen button's position. Their reads are gone. Board
  reaches `Config` by offset, so each run of removed members stays as a block of padding of the same size
  (`mRegistrationUnused<offset>` in `Config.h`) and `sizeof(Config)` is still the original's.
- **The images:** twenty entries left the resource table (`engine/ResTable.inc`; `tools/restable.py` has the list, `OMIT`): the
  ones named above, the registration dialog's background and its four buttons, the played-times font and the options screen's
  link. The load bar loads 209 entries where it loaded 229, so the title comes a little sooner still.

Looked for and not found: a "registered to <name>" stamp, a thank-you, a registered status line or a nag. The exe's strings (the port carries
none: `GameTextTable.inc` lists addresses) name the registration only in the configuration keys above, the dialogs' file names and
the two trial lines (the format of "N more trial plays remain" and the played-times line, 0x46ee78 and 0x46ee98); the data file's
texts (credits, the sixteen instruction pages, tips and bonus settings) say nothing of it. `lowos::BlitRegistered` is the blitter
of a registered surface and has nothing to do with it. No random number, clock read or state of the game depended on any of this,
so the frozen baseline's state logs are unchanged (`tools/check.sh tiptop` reports the baseline identical).

## Performance (measured 2026-11-08, `tools/perf/`)

The original and the port back to back on one machine, in real time, invisibly and silently (the port under rundesk, the original
through origrun, which puts it under rundesk), by `tools/perf/perf.py` (`port` and `orig`; `gate.sh` waits for a quiet slot;
`patches.sh` makes the original's patch list). The machine was not quiet: 4 to 8 other game processes of other sessions ran during
each run (the run's own list is in its `result.json`), so the numbers are for comparing the two sides, and the performance pass
re-measures on a quiet machine. The original's copy is patched only to play as the registered game, to pin its seed and to leave the
MIDI music out (its music player runs, its device calls are skipped); the port plays its music through SDL's silent driver.

**The one thing to know about the original's numbers:** it paces its frames by waiting for the screen's vertical blank, and the
hidden desktop the tests run it on has none, so the unpatched original spins: its bar fills at about 260 frames a second and it
uses three quarters of a core. Both are shown. The second original row has its main loop sleep 16 ms a pass (`perf.py orig
--paced 16`), as a 60 Hz screen's blank would, which is what a player sees: 229 frames at about 55 a second.

*Loading*, seconds from the game process's creation to the load bar being full (the bar's loaded count equals its total, read from the
process's memory every 2 ms; from then on the title takes a click), and the bar's own fill, from its creation to full. Two runs each:

| | Launch to bar full | The bar's fill |
|---|---|---|
| Original, unpaced (spinning) | 1.51, 1.80 | 0.89, 0.86 |
| Original, a pass every 16 ms (60 Hz) | 4.82, 4.66 | 4.31, 4.20 |
| Port before this change (one entry a frame) | 4.32, 4.16 | 3.38, 3.77 |
| Port now | 0.63, 0.93 (0.83 in a third run) | 0.20, 0.25 |

The port's fill now takes about 20 frames of 10 ms of loading; what is left of its 0.6 to 0.9 s is the process, the audio start and
the framework's synchronous resource load before the splash (the log's "Resource Loading Time", 0.4 to 0.8 s), none of it the bar.

*Memory and processor time* over the same scripted minute, the same real clicks on both sides: the title's button, Play, Strategy,
the tips' button, then a click on a column every 2.4 s, 25 of them (blind: the strategy mode waits for the player, a regular game
piles up and ends about 33 s after its first click however slowly it is clicked). The window starts when the board appears and
ends 60 s later or at the game over, which came about 30 to 38 s in, in four of the seven runs that reached the board; processor time is user plus kernel, per
second of the window.

| | Window | Peak working set | Peak private bytes | Processor time |
|---|---|---|---|---|
| Original, unpaced (spinning) | 60 s, 60 s | 61.4, 61.3 MB | 46.8, 46.7 MB | 44.8 s, 45.3 s (0.75 of a core) |
| Original, a pass every 16 ms | 31.6 s | 60.8 MB | 45.9 MB | 1.6 s (0.05 of a core) |
| Port before this change | 38.3 s, 29.9 s | 82.2, 82.4 MB | 82.3, 83.0 MB | 14.5 s, 8.8 s (0.38, 0.29) |
| Port now | 30.9 s, 60 s | 82.0, 90.4 MB | 81.9, 84.1 MB | 10.1 s, 17.3 s (0.33, 0.29) |

One of the paced original's two runs (`orig16_1`) clicked the title before its bar was full, so it never reached the board: its
load numbers stand, it has no window. Read it plainly: the loading change takes the time from launch to the first screen that takes
input from about 4.2 to 4.7 s (what the original needs at 60 Hz, and the port needed) to under a second. The port holds about
20 MB more than the original and uses about six times the original's processor time at 60 Hz (a third of a core against a
twentieth), which the loading change does not touch; both were left to the performance pass (the next section: the processor time halved, the memory did not move; the framework's frame loop and renderer,
SDL's audio buffers and the soundfont the music player holds are the places to look).

*The web build*, in the Browser pane (`tools/web/loadtime.js`): the pane was hidden (this session is open in no window), so the
page drew about every 0.3 to 0.6 s and ran its timers slowly, and a time in milliseconds would measure the pane. Not timed. What
does not depend on the machine is the game's own update count when the title takes its first click (`state`'s `updates`): 64 for the
build before this change with the site's fixes mod on (its `fastLoading` loaded 8 entries a frame), 37 now, the page's start and
the driver's latency included. The old build without the mod (one entry a frame) was not run on the web: the player page's
"Play without mods" switch did not take there.

## Performance pass (2026-11-11, `tools/perf/`)

The port's processor time fell by about half: a scripted minute of the strategy mode took 18.8 and 22.4 s of processor time (0.31
and 0.37 of a core) before the pass and 12.1 s, and 4.2 s in a game that ended after 31 s (0.20 and 0.14), after it; 3500 updates
of the bot in real time took 9.2 s on average (7.8 to 11.4) and now take 4.85 s (4.8 to 5.0). Two changes did it, one of them nearly all of it.

**How it was measured.** A sampler that stops each thread every millisecond and charges the cycles since the last look to the
instruction it stopped at (`tools/perf/cpuprof.py` of Bejeweled, and `cpuprof2.py` here, which reads the 32-bit ntdll through
SysWOW64 and prints the process's own times too) put 60 to 77% of the main thread in `ZwWaitForSingleObject`: the sampler's own
suspends and a syscall's cycles charged to the wait it returns from (the list of shared costs in docs/COORDINATION.md says the same),
so its split cannot be trusted. What found the cost was timers around the calls (`SDL_GetPerformanceCounter`, temporary and
removed): around `RunFrame` (the game's pass), around the screen copy and `DrawImage` in `TipTopWidget::Draw`, and around
`lowos::BlitPixels` by the flags of the call. `cpuprof2.py` with seconds 0 reads only each thread's cycle count every quarter
second and the process's user and kernel time at the end, which perturbs nothing. The runs were `-autoplay=7
-autoplaymode=strategy` (or regular) for 3500 updates in real time under rundesk, the before and after exes alternating, with one
other game process on the machine; the scripted minute is `perf.py port --mode strategy --every 2.4 --count 25`, the one of
2026-11-08.

**Where the time went (before, per 3500 updates, about 36 s).** The main thread held about 75% of the cycles, the Direct3D driver's
thread about 17% and the audio mixer's about 6%. On the main thread the game's pass took 4.7 to 7.2 s, and 5.1 s of that was
`BlitPixels`: the plain copies of a surface (flags `SURF_SCREENFORMAT` only) took 4.2 s for 1756 million pixels in 38,000 calls
(2.4 ns a pixel, about 1.6 screens of pixels every 10 ms update: the original's pass paints its whole display each time), the
color-keyed copies 0.8 s for 266 million in 248,000 calls, everything else (translucent, additive, alpha byte, other formats) under
0.15 s together. The framework's present of the screen (`Draw`: the copy into the screen image 0.3 s, `DrawImage` with its upload
0.6 to 0.9 s over 2090 frames) took about 1.2 s.

**What changed.**

- `BlitPixels` (`engine/Lowos.cpp`): the generic loop does every pixel a byte, a switch on the pixel size and a branch at a time. A
  call that copies 32-bit pixels of one format with no alpha byte, blend or add (nearly all of them) now takes a `memcpy` a row,
  and the color-keyed one a pixel at a time with a select instead of a branch (an unchanged destination pixel is written back, the
  same bytes); a source that overlaps the destination keeps the generic loop, whose order of reads and writes the result may depend
  on. The blits took 1.85 s, not 5.1, and the pass 2.2 s, not 6.0, in the same run.
- `TipTopWidget`'s screen image is declared opaque (`SetImageMode(false, false)`) and `mUploadAsIs`, as the other software-drawn
  ports' are (the framework then does not look through the pixels for alpha after each change, and uploads in one call instead of
  copying and premultiplying them in strips): the same pixels, 0.1 to 0.3 s less over a run.

**Proof that nothing differs.** `tools/check.sh build build-web:tiptop tiptop` passes with the rerun and the frozen baseline
identical. `tools/perf/blitcheck.py` builds the committed `BlitPixels` and the new one into one program and runs 3,000,000 random
calls on copies of one buffer (every flag combination, 1 to 4 bytes a pixel, three pixel formats, color keys that hit, levels 0 to
255, padded pitches, 600,000 with the source overlapping the destination) and compares every byte after each: no difference; with
the key's test turned round or the overlap test removed it finds 27,852 and 1,807 differences in 300,000 calls. And the screen
itself: the shots of the six phase-two tours (172) and of two games in each mode with `-autoplayshots` (21) are byte for byte the
same from the exe before the pass and the one after, and so are their state logs but for four lines that the original's
uninitialised memory decides (the meter's start value outside the timed mode, and the level and blocks left before the first board
exists; see the fixes mod's section).

**Tried and not kept.** Sending only the changed part of the screen: the display as a grid of images, each uploaded
(`BitsChanged`) only when its pixels differed from the last frame's (6 of 25 tiles of 128x96 changed in an average frame, and 5 to
7% of frames not at all). Tiles of 128x96, 320x240 and 64x48 against one image: 0.73 to 0.77 s, 0.78 to 0.80 s and 1.24 to 1.28 s
of drawing a run against 0.79 to 0.99 s for one image (about 0.1 s of 36 s at best, and the 100 draws of the small tiles cost more
than the bytes they save). The upload costs about 0.3 to 0.45 ms a frame whatever its size, a cost of the call and the driver, so
a rectangle upload in the framework would not help TipTop. Nor can the passes whose frame is not shown be skipped (about 4 of 10,
since the framework draws about 60 times a second and updates 100): each pass is the original's whole main-loop iteration, and
what the next pass finds in the display and its other state may depend on it.

**The scripted minute** (`perf.py port --mode strategy --every 2.4 --count 25`, 60 s from the first board; one other game process
of another session ran; the before exe is the tree of commit `34e98816`):

| | Window | Processor time | Peak working set | Peak private bytes | Load, to the bar full |
|---|---|---|---|---|---|
| Before the pass | 60 s, 60 s | 22.4 s, 18.8 s (0.37, 0.31 of a core) | 89.1, 88.4 MB | 82.8, 81.8 MB | 0.72, 0.59 s |
| After | 60 s, 30.9 s (the game ended) | 12.1 s, 4.2 s (0.20, 0.14) | 88.3, 89.3 MB | 81.7, 82.7 MB | 0.73, 0.71 s |

The paced original used 0.05 of a core in 2026-11-08's table, so the port is still about three to four times it. What is left on
the main thread: the plain copies, about 1.5 s of 36 s for the same 1756 million pixels (0.87 ns a pixel, where a screen's rows
copied in a loop between two buffers measure 0.2 ns on this machine: the sources are many different images, cold in the cache, so
it is memory traffic), the present about 1 s, the rest of the pass and the framework's loop. Then the Direct3D driver's thread
(about 2.5 G cycles a run, whatever the upload: `SDL_RenderPresent` and the device, the shared costs in docs/COORDINATION.md) and
the audio mixer's (about 1 G, the music's software synthesizer and the sounds' mixing). Memory did not move: the 20 MB more than the
original is the framework's and the renderer's (the Direct3D 11 device is the shared 20 to 25 MB; the soundfont file is 3.3 MB).
The load time is as before (0.6 to 0.7 s). The web build was not timed (the pane is hidden, see the loading section above); it
runs the same C++, so the blits gain the same, and it builds (`build-web:tiptop`).

## Performance, second round (2026-11-12, `tools/perf/`)

The target of `docs/COORDINATION.md` ("The target", 2026-11-11) is a port at its original's processor time or below, measured like
for like. After the first round the port was at about twice the paced original's. This round measured the original and the port the
same way, found where the rest is, changed what the game itself can change, and wrote the rest up as a shared spec.

**Measuring like for like.** `tools/perf/perf.py` reads each thread's cycle count (`QueryThreadCycleTime`) every quarter second.
`perf.py portscript` plays the scripted minute from the port's `-script` with no `-commands` or `-realinput` file: the framework
polls that channel every update (a `stat()`, about 0.34 M cycles a call, the coordinator's figure), which a player's copy never
pays, and the runs through the channel measured 0.148 to 0.153 of a core against 0.12 to 0.13 without it. `perf.py orig --paced 16`
is the original under origrun with a 16 ms sleep a pass (as a 60 Hz screen's blank would; it has no music in these runs), and the
port runs without music too (`--port-arg=-soundfont=<no such file>`), so neither side counts sound; with its music the port's
synthesizer thread adds 34 to 42 M cycles a second (about 0.5 s of a 30 s window), which the paced original's MIDI never shows.
`--menu` leaves the main menu alone for the window, `--menu --menu-click 265,157` the options screen. **The machine was busy** (2 to 10
other game processes, each result's `others_before` and `others_after`), and the same exe gave 1.6 to 3.3 s of processor time at
the main menu, so only large differences are read and the table gives ranges.

| | Paced original | Port at the start of the round | Port now |
|---|---|---|---|
| Strategy minute, 30 s window (clicks as in the first round) | 1.9 s (3.89 s in 60 s, 2.20 s in 34.65 s: 0.065 and 0.064 of a core) | 3.64 s clean (0.12), 3.75 to 3.81 with the music | the same: the screen changes every frame |
| Main menu left alone, 20 s | 1.20 s (2.43 G cycles) | 1.6 to 3.4 s (3.5 to 6 G) | the same: the menu animates every pass |
| Options screen left alone, 20 s | 1.39 and 1.31 s (2.8 and 3.2 G) | 2.25, 2.89, 3.11 s (5.9 to 6.2 G) | **1.17, 1.09, 1.30 s (2.6 to 3.3 G)** |

**What is left is the framework's present.** The port's cycles split into the main thread and the Direct3D driver's thread (2.0
to 2.7 G of 4 to 6 G a 30 s window in the strategy minute, and 1.4 to 2.2 G in 20 s at the menu), against the original's single
thread. `tools/perf/presentbench.cpp` presents a 640x480 picture with no game in it: SDL's path costs 2.0 to 2.9 M cycles a frame
(main thread 0.8 to 1.35 M, the driver's threads 0.95 to 1.97 M) whichever driver SDL picks, and 1.8 to 1.9 M even with no
upload at all; a 1x1 or a 160x76 upload, a streaming texture and vsync off cost the same, so it is the frame and not the upload
or its size. A picture copied into `SDL_GetWindowSurface` and `SDL_UpdateWindowSurface` costs 0.9 to 1.2 M a frame on both the
hidden and the visible desktop, with no driver thread. At 60 frames a second the first is 110 to 175 M cycles a second; the
paced original's whole process at the main menu takes about 120 M. So the port cannot reach its original through this present
whatever the game does. It is the "SDL_RenderPresent, a driver cost per presented frame" item of `docs/COORDINATION.md`; the
spec for presenting an opaque software-drawn frame through the window's surface went to the coordinator (shared code, not
changed here).

**What the game changed.** The framework now draws a frame only when the screen shows something new. `lowos::HostScreenWrites`
counts the writes to the screen (a blit, a fill, a stretched blit, a new canvas); after a pass that wrote and when no frame is
already due, `TipTopWidget::Update` compares the screen with the pixels last shown (`ScreenDiffersFromShown`, in blocks of 16
pixels with an early exit, the alpha byte aside) and marks the widget dirty only if they differ. Where the framework has no dirty
widget it draws no frame (one a second at least, and at once on an expose, a restore or a device reset: its own rules in
`DrawDirtyStuff`), so a screen that stands still, the options, the instruction pages and the credits, costs no upload, no
present and no driver thread: the options screen fell from 2.25 to 3.11 s to 1.09 to 1.30 s per 20 s, the original's 1.31 to
1.39 s. The main menu, the high scores and the game animate every pass (the main menu changes about 3300 pixels a pass in
the rectangle 430,362 to 590,438), so they gain nothing. The picture is the same: the screenshots of the six tours (172) are
byte for byte those of the exe before this round, and their state logs are identical but for the first lines' uninitialised
memory (below); `tools/check.sh build build-web:tiptop tiptop` passes with the rerun and the baseline identical.

**The pass rate** (written up for the coordinator before any change): the original's pass is one trip of the main loop, whose
`lowos_EndFrame` waits for the vertical blank, so it runs 60 passes a second on a 60 Hz screen (144 on a 144 Hz one). The game's
speed follows the clock, not the pass count: `GameTime()` is milliseconds from the performance counter, `Board::Draw` (0x40e766)
adds the elapsed time to the field (`mTotalTime += now - clock`), and the 35 ms tick clock keeps its remainder; only the load
bar's entries are counted per pass. The port runs the framework's 100 passes a second (`mFrameTime` 10, an int), which is a rate the
original never had; a pass costs about 0.34 ms (0.7 M cycles) of the main thread. Matching it would be `mFrameTime` 16 (62.5 Hz)
in `TipTopApp`'s constructor, about 0.4 s of 3.7 s in 30 s of play; but `-fast`'s virtual clock advances `mFrameTime` a pass,
so the state logs, the frozen baseline and the identical-play scripts (the original patched to 10 ms a pass) would all change
unless `-fast` stays at 10 ms. **Not changed: the rate waits for the coordinator's answer.**

**Why the paint of a pass that is not shown is not skipped** (about 4 passes in 10 at 100 passes against 60 frames a second):
`Board::Draw` is the game's update and its paint in one function, so only the pixel writes could be left out, and the original
does not paint the whole screen every pass. `-scramble` fills the screen with noise before each pass and `-screenhash=<file>`
logs a hash of the screen after it (`src/testhooks/Canvas.cpp`): the six tours (33,000 passes) give the same hash after every
pass with and without the noise, and the same state logs; three bot games through the menus in the regular mode (50,054 passes)
give 13 passes whose picture depends on the screen before them, and the three I looked at are the pass that switches to screen 7
(the level summary), which paints only part of it and leaves the rest to the next pass. A pass skipped just before one of those
would show different pixels for a frame. The speed and strategy modes were started and stopped unfinished.

**The first lines of a state log** read `level`, `blocksleft` and `meter` of a board that exists before its first pass and holds
whatever the heap held: with the screen noise, or in another run, they differ (it is the original's uninitialised memory, as
in the first round). `meter` in the regular mode stays garbage for the game, adding to it as it goes.

**Tried and not kept** (each on the 20 s menu, alternating with the exe without it, the driver's cycles read): the framework drawing
straight into the window's back buffer with no screen texture (no change); the screen image as a streaming texture
(`SDL_LockTexture`; no change); uploading only the changed rectangle (mean 8% of the screen: no change); SDL's other render
drivers (OpenGL and Direct3D 11 no better; the software renderer used a whole core, 29.98 s in 30 s, with the framework's
present); a compare-and-copy that finds the changed rectangle. The audio synthesizer is already compiled `-O3 -msse2
-mfpmath=sse`; its 1 ms sequencer steps are the music's timing and stay.

## Fix candidates and the fixes mod (`mods/fixes`, `src/ModFeatures.cpp`)

The port is the original; these are what the original wastes or leaves to chance, kept in the port and mended by the fixes mod
(docs/MOD_ROADMAP.md, "The fixes mod"), one toggle each, all on by default on the site. The port loads mods (`-modsdir=<dir>`, or
`mods-tiptop` beside the exe; `-nomods`; on the web `/ttpersist/mods`), and without the mod, or with it off, nothing differs from
the original: `tools/check.sh tiptop` and the frozen baseline are unchanged. Slow loading is not among the fixes: it is made
fast in the port itself (the user, 2026-11-07), see "Deviations from the original" above.

| Toggle | The original | What the fix changes |
|---|---|---|
| `seededBlocks` | The first time the field chooses a block's kind (`Field::RandomKind`, 0x40428c) it calls `srand` with the game clock again, discarding the seed of the start. A seed given at the start (`-seed`, the site's `?seed=`) therefore changes nothing, and the blocks depend on the millisecond that first block was chosen. | The second seeding is left out, so the seed of the start decides the blocks of every game. |
| `steadyClock` | The game's two clocks are the machine's (`GetTickCount`), so the frame times, and with them the blocks' positions, the spawn delays and the draw of `rand` after each, vary from run to run: the same seed and the same moves do not play the same game twice. | The clocks run on the updates, 10 ms each (what `-fast` does), so the same seed and the same moves play the same game on every run, however fast the machine. A pause or a lost focus still stops the game clock, as in the original. |

With `seededBlocks` and `steadyClock` on, three real-time runs of one bot game (`-seed=5 -autoplay=3`, 2500 updates) wrote the same
state log; with the mod off, two such runs differed from the title screen on (the title screen's end, the first game's start, and so
on). The meter's start value outside the timed mode (an oddity of the original, nothing reads it) still differs from run to run.

## Coverage check (2026-11-11)

The coverage check of `docs/COVERAGE.md`: origrun's `--coverage` recorded which of the original's functions ran, and
`tools/coverage.py` laid the hits beside the port's sources. `games/tiptop/coverage.txt` is its settings file and
`tools/coverage/` the scripts (`run.sh`, `all.sh` and the extra runs' `extra1.txt` to `extra4.txt`).

**The runs: eleven, on the host,** each one game process on origrun's virtual clock with the scripted cursor, phase one's
patch list with the music's (its player runs, its MIDI calls are skipped) and origrun's recording sound stand-in, so that
LOWOS's sound code and the music sequencer run too:

- the phase-two tours 1, 2 and 3 as origrun scripts without their shots (`run.sh tour`): the menus and their hovers, the
  high scores with the clear question answered No, the options, the instruction pages, the credits, the quit question
  answered No and the new game screen; phase one's whole regular game with its summaries, the name and the high score
  screen; pause, the window losing and regaining the focus, the options and instructions from the game, End game with its
  save, and the saved game continued and left;
- the four bot cases of phase one (`str`, `spd`, `save`, `reg2`: a strategy, a speed and a regular game, the saved game,
  two regular games in a row), played from the script of the phase-one baseline's bot (`build/baseline-tiptop-phase1`).
  The first set of these played the current port's script against the original, whose title takes 202 updates longer, so
  their clicks came before the title took one and they never left the splash (576 functions each); they were run again
  with the baseline's script and reached 743 to 765 functions;
- four short runs for what those miss: the options' two sliders pressed and dragged, then the main menu's quit question
  answered Yes (`extra1`); a game started and the window closed in the middle of it (`extra2`); the high scores' clear
  question answered Yes (`extra3`); tour 2's whole game followed by the summary's Play Again (`extra4`).

All but the two runs that end in the quit question ended at origrun's timeout, since the original asks "Are you sure?"
when its window is closed and nothing answers; the two that click Yes end with the game's own exit, code 0.

**The settings.** The game's own code is 0x401000-0x418b51 and 0x4197c6-0x42cfae (the board, the application, the GUI,
the configuration reader and the MIDI sequencer), LOWOS 0x42d020-0x4382b0 and the resource manager 0x438390-0x439d70:
462 functions. Between the configuration reader and the application the linker put the MSVC library's `std::string`
(42 small functions at 0x418f68-0x4197b5, which the export leaves unnamed); the range leaves them out. The runs reached
367 of the 462, and the port has a counterpart for 370.

**Reached in the original with no counterpart in the port: 62 at first, none at the end.** No game logic was missing.

- **Placed: 8.** The blitter's pixel loops. The original has 44 specialised routines (0x4346f0-0x437b73) and the port
  one function, `lowos::BlitPixels`, that does them all. The 8 the runs reached are a plain copy, the alpha-byte copies,
  the color key, translucency and the additive blend, all for the 32-bit display the game sets. All 44 addresses stand
  in the comment above `BlitPixels` (the other 36, for other pixel formats, show in the report's list of functions never
  reached).
- **Left out on purpose: 54, as `none` lines with a reason each.** 47 are LOWOS's plumbing that the platform and the host
  replace: DirectDraw's modes, surfaces and window update (`CompleteScreen`, `UpdateScreen`, `UpdateSurfaces`, the video
  surface search), the message pump, DirectInput's devices, DirectSound's buffers, COM, LOWOS's own heap and its
  start-up, a thread start, Winsock's cleanup (the game never uses the network) and a stopwatch around DirectDraw's
  blits whose total nothing reads. Five are the registration's, which the registered flag turns into pass-throughs (the
  quit and save questions' unregistered flavours, `TrialCheck`, and `CountPlay`'s file and registry writes, reached with
  the flag read as set). One is `SetCurrentDirectory` to the exe's folder and one is `std::string::size`.

**Defined in the port, never reached: 57 at the end** (the report's second list), 36 of them the blitter's routines
above. The other 21, sorted:

- *Reached after the first runs, by an extra run:* `ResetHighScores` (0x411c85), by `extra3`.
- *Candidates for taking out (no play of the original can reach them),* not removed in this step:
  - the hidden timed mode (mode 1): `GetMeter` (0x4103dd), the time trial's gauge (0x41c599) and `App::SetCrossed`
    (0x41ee83), and every branch of it inside shared functions. Its button sits at (1000, 1000), off the 640x480 screen,
    and no key presses it;
  - what the shipped configuration (`tiptap.cfg` in `TipTop.dat`) switches off: `DeleteGameDialog` (0x425ffa) and the
    commands 0x25-0x28 that call it (`DELETEGAMEWARNING_DISABLE` is TRUE), and `AnimationProc` (0x42cbe4), the level
    complete animation (`LEVELCOMPLETE_DISABLE` is TRUE);
  - code with no sender: `PlayAgainDialog` (0x4264a8) is the other branch of command 0x2a, which asks only when the
    high scores screen came from a screen other than the main menu, and the main menu's button is the only way into that
    screen (nothing else calls `SetScreen(SCREEN_HIGHSCORES)`; the summary's own Play Again button sends 0x2d);
    `SlideImageProc` (0x421f9f) is made by command 0x1c, which no literal of the export sends; `ScrollBarProc`
    (0x429fd5) with its helper 0x429e79 and `InflateDirty` (0x428d6d, called only by it) belong to a layer whose
    content is bigger than its window, which none of the game's screens is (every text and picture comes from
    `TipTop.dat`).
    (Dealt with in "Unneeded code taken out" below: the dead ones are out; the ones a game-file value reaches stay.)
- *Stay, since some input reaches them:* the Tab keys of the name field (`NextHandler`, `PrevHandler`) and the full
  screen key (`lowos::KeyIsDown`), which origrun cannot post (the original reads its keys through DirectInput); the error
  exits (`lowos_MessageBox_`, `lowos_ScreenScope`: "Not enough memory..."); `Board::Ramp` (0x40b3ad), the levels past the
  end of a mode's list, which a very long game reaches (the bot's best was level 12 of a speed game); and
  `SetInputState` (0x431130), the original's DirectInput queue writer, which the posted window messages of these runs
  bypass and which the port's input path takes (read against the export in phase one, "How both sides are driven").
- *Read against the export, not run:* `App::ToggleFullScreen` (0x41b36c), `OnRestore` (0x41b34b) and `ReloadSurfaces`
  (0x4274d9), since the original would switch the machine's display mode. The port's full screen played in phase two's
  real-input run (windowed and full screen).

**Never reached and not defined: 38** (the third list). The registration's helpers (`ScrambleRegistration` 0x428437,
the run count file's reader and writer 0x412bc6 and 0x411f2a), four of `std::string`'s, LOWOS's DirectDraw,
DirectInput, DirectSound and Winsock code (the rest of what the `none` lines name), and the resource manager's requests
for background loading (0x4382b0, 0x438410, 0x438480, 0x439cf0, which the game never fills) and its BMP loader
(0x4392f0, 0x439440; the archive holds PCX and JPEG images). Nothing that a play of the game reaches.

**What changed:** comments only (the blitter's routine addresses above `BlitPixels`, and a range in `Lowos.h` that named
a function of the Winsock code as the sound code's end), `games/tiptop/coverage.txt` and the run scripts.
`tools/check.sh tiptop` still matches the frozen baseline. The runs took about an hour and a half of wall time, most of
it waiting for a free game slot (4 to 7 game processes ran on the machine the whole time); the runs themselves held a
slot for about 40 minutes in all.

## Unneeded code taken out (2026-11-11)

The coverage check's candidates (above) and a link-time sweep are dealt with: 235 lines out of 13 files, 15 in (a comment, the
padding and the `none` lines of `coverage.txt`). What counts as unneeded is the rule of `docs/COVERAGE.md` and Water Bugs' pass:
code that no input reaches, nothing in the shipped files and nothing the player or a test command can do. Code that only some
input reaches (a value of the game file's own language that the shipped files do not use) stays.

**How it was found.** The coverage check's list of functions that no play of the original reaches, and a second look from the
port's side: a build of the port with `-O0 -ffunction-sections -fdata-sections` linked with `--gc-sections --verbose`, whose
"Discarded" lines name every function and variable that nothing references (at `-O0`, so that an inlined function does not look
dead), repeated after each cut until it named none of the game's own. The test commands, the bot and the site's hooks count as
callers.

**Taken out.**

- *The original's exit teardown, which the port's host does not run:* `App::Shutdown` (0x41b98a: screen none, music stop, the
  board, the images, the GUI and LOWOS's devices) with `App::FreeImages` (0x41b771), `GuiShutdown` (0x428fa3),
  `lowos::UninitScreen` (0x42f750) and `lowos::UninitSound` (0x432e70). The host stops the music and ends the process; what
  the port does at the exit is `GameShutdown` (the preferences, the configuration's tables), which stays.
- *The configuration's string readers,* `StringLength` (0x417cfd) and `ReadString` (0x417d63): only the registration's and the
  links' strings (`REGISTER_URL`, `PUBLISHER_URL`, `RIS_*`) were read through them, and the registration pass took those out.
- *`SlideImageProc`* (0x421f9f), the image that slides across the screen, and command 0x1c that makes it: nothing sends the
  command (every button and handler sends a literal, and the configuration holds positions, not commands).
- *`PlayAgainDialog`* (0x4264a8) with command 0x2c, its "No", and the other branch of command 0x2a. The high scores screen is
  entered by command 0x2b alone, which only the main menu's button sends, so the previous screen is the main menu whenever 0x2a
  arrives and the question was never asked; 0x2a now goes to the main menu at once. The six values of the dialog's place in the
  configuration (`mPlayagainX` to `mPlayagainNoy`) are padding of the same size, as the registration's are (Board reaches the
  configuration by offset), and their reads are gone.
- *`res::FreeImage`* (a helper nothing called) and *`gModal2`* (0x4886f8, a global nothing read or wrote).

**Kept, because some input reaches it.**

- The hidden timed mode, its button and rows: the configuration's positions (1000, 1000) are values of the game's own file, and
  `docs/MOD_IDEAS.md` has a mod that moves them on screen; the driver's `goto timed` also plays it.
- `DeleteGameDialog` (0x425ffa) with commands 0x25 to 0x28 (reached when the configuration's `DELETEGAMEWARNING_DISABLE` is
  FALSE) and `AnimationProc` (0x42cbe4), the level complete animation (`LEVELCOMPLETE_DISABLE` FALSE).
- `ScrollBarProc` (0x429fd5) with its step function (0x429e79) and `InflateDirty` (0x428d6d): the text area sets its layer's
  content to the height of its text and a longer text than the window (an instruction or credit page of `TipTop.dat` another
  copy might hold) makes the scroll bar. The coverage list had these as candidates; they are not.
- The ones the coverage section already keeps: the name field's Tab keys, the full screen key, the error exits, `Board::Ramp`,
  `SetInputState`, `ToggleFullScreen`, `OnRestore` and `ReloadSurfaces`.

`coverage.txt` gains nine `none` lines for the functions taken out (the original runs all of them; `res::FreeImage` and `gModal2`
had no counterpart in the export).

## Readability pass (2026-11-11)

`docs/CODE_STANDARDS.md`'s rules, with `python tools/namecheck.py games/tiptop` at no finding: it began at 312 in `src/` and
`games/tiptop/namecheck.txt` (one line, `folder engine`) now adds the port's own LOWOS layer in `engine/`, which held twelve more
(a parameter and the locals of `BlitPixels` in `Lowos`, the locals of the readers in `Res`); every one is fixed, and the file allows nothing. The
object code is the proof that none of it changed the game: `tools/objsnap.py` before and after the renames reports 0 of 588
functions different (the five switch cases taken out below are the one code change, and with them put back for the comparison it
was 0 as well), the web build compiles, and the suite's rerun and frozen baseline are identical.

**Names.**

- *The configuration's records.* Every `Level`, `Particle` and `Effect` field was a letter and a number. They are named from the
  original's own configuration file (`tiptap.cfg` in the game's data, which documents the particle and effect fields) and from
  how the code uses them: `mStartRows`, `mStopRow`, `mFloatsPerRemoval`, `mRange`, `mGoal`, `mNumColors`, `mSpawnThreshold`, the
  three meter fields and the particle's size, opacity and color curves, blend type and the effect's delay, count, speed, fade
  times and oscillation. The configuration's padding (the bytes the board reaches by offset, which `static_assert`s the size
  of) was `mUnknown*` and `mRegistrationUnused*`: the ones that copy a level's values are `mLevel*Copy`, the rest say which
  setting they stood where (`...Options`, `...SplashRegister`, `...ExecutionLimit`), and the header's comment says that nothing
  reads them (checked against the export for every one).
- *The application and board.* The fields that were an offset (`m134` to `m144`, `m13x`, `m1618`) are `mBonusClock`,
  `mBonusElapsed`, `mBonusClockStopped`, `mBonusTipShown`, `mBonusBlocksRanOut`, `mBonusStartAnimations` and
  `mGlowBufferUnused`, the column cursor `mEC` is `mColumnCursor`, and the three meter fields the board reads are
  `CfgMeterStart`, `CfgMeterLoss` and `CfgMeterDoublingTime`. The twenty `goto` labels of the translated board (the loops the
  original wrote with jumps) say what they jump to: `AfterCollision`, `StopAtHighest`, `QueueMagnet`, `NextPiece`.
- *One-letter and typed locals.* The functions of `Gui.cpp`, `Config.cpp`, `Res.cpp`, `Music.cpp`, `MusicSynth.cpp` and the test
  hooks name each local for what it holds (`aGlyph`, `aHandler`, `aScroll`, `aFirstDataByte`, `aShortRadius`, `aGlowWidth`).
  Where a name carried a guessed size (`aRadius16`, `aWidth32`, `aTime64`) the size left it.

**Addresses.** Each file of the port holds the `// port:` marker in its first comment, so the function-by-function address
comments stay where the translation put them and need no more (`TipTopApp.cpp`, `GameText.cpp`, `ModFeatures.cpp` and the five
test hooks were the files without one).

**Numbers.** What repeated and meant one thing is a constant: `GAME_WIDTH` and `GAME_HEIGHT` (the 640 by 480 screen, `Game.h`),
`FIXED_SHIFT` and `FIXED_ONE` (positions on the field are fixed point with 8 bits of fraction; about 70 places in
`Board.cpp`), `CHAIN_COUNT` and `NO_CHAIN` (the ring of 0x400 chain counters and the group number of a piece in none),
`FLASH_TIME` (0xfa ms, a block's flash before a column nuke's or a color bomb's effect removes it), `MSG_CURSOR_COLUMN` and
`MSG_CURSOR_TAPPED` (the two messages the column cursor answers, in the numbers of the generic ones) and `COMMAND_MASK` with
`BUTTON_ENTER`, `BUTTON_ESCAPE`, `POP_LONGER`, `POP_UNTIL_CLICK` and `ANIMATION_ADDITIVE` (the flag bits in the top of a
button's, pop image's and animation's command). The commands of `App::Command` and every place that sends one are the `CMD_*`
enum of `App.h` (the original's numbers, 0x01 to 0x3e).

**Code found on the way.** The pass over the commands found five cases of `App::Command` that the unneeded-code survey missed,
because it looked at functions and not at the cases of a switch: 0x1e, 0x1f, 0x34, 0x35 and 0x3d. They were the registration
dialogs' answers, and no sender of them was left once that pass took the dialogs out. They are gone, and the switch's jump table
is five entries smaller (which is why the object code comparison above needed the cases put back).

**Not done, and why.**

- The 481 `GTC(0x...)` calls that read a text from the exe by its address stay as they are. Reading the game's texts from the
  player's own `TipTop.exe` by address is how `GameText.cpp` works (`tools/make_gametext.py` makes it from the export), and a
  name for each of 481 strings the code never says anything else about would only be a second table of the same numbers; the
  comment beside a call says what the text is where it matters.
- The sound ids of `Play(gSounds[0x..])` (58 uses) and the brightness values of 0x100 in the fades are the original's own
  sound table indices and brightness scale; the sound log (above) names each sound as `TipTop.dat` stores it, and a constant for
  each id would be a second copy of that.
- `tools/splitfields.py` (`--export re/tiptop/gh/decomp --all`) reports the members the port writes and never reads
  (`mRight` and `mBottom` of the board, `mContentWidth` and `mContentHeight` of a GUI layer, `mGlowBufferUnused`, the padding);
  they stay, each with a comment saying that nothing reads them. It also flags `mPieces` and
  `mColumnWeights` against the destructor at 0x40147e; that is the tool's pairing of the destructor's address with the function
  after it, and the port's destructor frees both.

**Proof.** `tools/check.sh build build-web:tiptop tiptop` passes with the rerun and the frozen baseline identical. The shots of
the six phase-two tours (172) and of two games in each mode with `-autoplayshots` (21) are byte for byte the same from an exe built
from the commit before this step and from the one after, and so are their state logs but for the lines the original's uninitialised
memory decides (the meter's start value and the level and blocks left at update 1). None of those plays reaches the code taken
out except the high scores screen's way back (command 0x2a), which tour 1 clicks (the main menu's options button works right after it).

## The PopCap Game Collection version (2026-11-12)

TipTop is ported from the player's own retail copy; the check of `docs/COORDINATION.md` asks that the build also work with the
files of the PopCap Game Collection's copy (the user's ISO in their Downloads folder, the file group `TipTopDeluxeComponent`,
extracted with `E:/retools/unshield-build/unshield.exe g data1.cab` into the scratch folder, not the repository). It holds four
files: `TipTop.exe`, `TipTop.dat`, `Readme.txt` (the game's version 1.1 of 16 December 2002) and `license.txt`. The port reads the
first two.

**The files.** `TipTop.dat` is byte for byte the retail one (3,226,562 bytes, `cmp` finds no difference). `TipTop.exe` has the
same size (565,248 bytes) and the same link time stamp, and differs in 4 bytes in three places, all in the code (`.text`, none
in the data, so none of the texts the port reads from the exe by address): three conditional jumps on the registered flag
(`gRegistered`, "Registration out" above), whose targets the Collection's exe sets to the next instruction, so that the flag
is set whatever the configuration byte and the registry say. The port has no registration code, so none of that reaches it.

<!-- private
The three places, as file offset and virtual address: 0x1208f (0x41208f, the byte 0x0c -> 0x00), 0x123c8 (0x4123c8, 0x09 -> 0x00)
and 0x206dd (0x4206dd, the two bytes 0x94 0x01 -> 0x00 0x00): each the displacement of a `je` on the configuration byte or the
registered flag.
-->

**The run.** Tours 1 and 2 of `tools/shots/` (`-fast -seed=1`, the main menu, options, instructions, credits, high scores and the
games' screens by the script) ran once with `-gamedir` the retail staged copy and once with `-gamedir` the Collection's folder, in
the same way as the passes' other comparisons: tour 1 gives 31 screenshots and 76 lines of state log, tour 2 54 screenshots and 41
lines, and every screenshot is byte for byte the same on the two sets of files, and so are the state logs. The build needs nothing
for it (`-gamedir` or the install folder finds the files); the web page imports the same two files by name, which the Collection's
folder has. **The check passes; no change to the port, and nothing to list as a deviation.**
