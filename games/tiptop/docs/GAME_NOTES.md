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
twentieth), which the loading change does not touch; both are for the performance pass (the framework's frame loop and renderer,
SDL's audio buffers and the soundfont the music player holds are the places to look).

*The web build*, in the Browser pane (`tools/web/loadtime.js`): the pane was hidden (this session is open in no window), so the
page drew about every 0.3 to 0.6 s and ran its timers slowly, and a time in milliseconds would measure the pane. Not timed. What
does not depend on the machine is the game's own update count when the title takes its first click (`state`'s `updates`): 64 for the
build before this change with the site's fixes mod on (its `fastLoading` loaded 8 entries a frame), 37 now, the page's start and
the driver's latency included. The old build without the mod (one entry a frame) was not run on the web: the player page's
"Play without mods" switch did not take there.

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
