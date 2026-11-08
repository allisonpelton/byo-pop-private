# Word Harmony: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. The same input gives the same games in the original and the
port, by the game's state, not by pictures. Phase two, **looks identical** (the screens, the sound, the focus, the web
build and real input), follows in its own section.

The original is the game carved out of the wrapper (`re/wordharmony/bin/WordHarmonyGame.exe`, image base 0x400000, no
relocation), run only through `tools/origrun`, from a copy under a short path (`C:\byo-og\wh\game`, staged by
`games/wordharmony/tools/identical/setup.sh`), under rundesk, on origrun's virtual clock (`--clock virtual`) and with the
scripted cursor (`--cursor scripted`). One original and one port at a time at most; the original's registry key is cleared
before each run (`orig.sh`), so every run starts the way a first run does.

## How both sides are driven

**One input for both.** A case is a port `-script` file (`<update> move|click|rclick|down|up x y`, `<update> key <vk>`): the
port plays it with `-fast` (one frame an update, the engine's clock moved 33 ms a frame) and `toorig.py` turns it into an
origrun script (`at <update - 1> ...`: origrun's `at N` holds the original after frame N, so the input is read before frame
N+1, where the port reads its line K before frame K). The port logs its state before each frame too, so an input's effect
is in both logs under the same update.

**One start and one step.** The original seeds the C library's `rand` once, from `timeGetTime()`, in the game's init; the
seed patch puts the run's seed there (origrun's `--seed`), and the port's `-seed=N` is the same `srand(N)`. The game runs a
frame of 33 ms with an accumulator (0x4552b0) and takes the frame's elapsed time from `GetTickCount` (0x4542e0); on the
virtual clock that time is 32 to 37 ms when the main thread polls the clock (origrun moves a busy reader's clock on), so a
test patch makes it the period, 33 ms, as it is on a machine that keeps up. A second test patch counts the frames (the
update counter for origrun's `at` and `--watch`). All of them are in `tools/origrun/patches/wordharmony.txt` (git-ignored;
`setup.sh` writes it); none touches the trial or the registration (the carved exe has no wrapper to pass).

**The hint search on a thread.** The original starts its hint search (0x4191e0) on a thread (0x419560) and kills a running one
with `TerminateThread` when the board changes; on the virtual clock the main thread can kill it inside the heap's lock and
hang (frames 958 and 1446 of the first cases). A test patch runs the search at once instead, as the port does; its result is
the same, since the board does not change while the search runs on a machine that keeps up.

**Watched values.** origrun watches four expressions at a time, so a case runs the original nine times. The game is the
singleton at 0x4995b8, its ball system the pointer at +0x700: the generator's state (0x49729c), the game's state and next
state, its timer, score, level, mode, level template, time, level points, letters left and used, bonus, wild cards, hints
and hints used, the level's hints and time; the ball system's ball count, balls left, step time, spawn time, next slot, balls
to come, water and fill, hint index; and ball 0's position, state and letter and ball 3's position: 33 values. The port
writes the same names with `-statelog=<file>` (`WordHarmonyApp::LogState`, floats as their bits). `cmp.py` compares every
change of every value with its update; most of the game's fields are unset in the original until a game starts, so all but
the generator and the states are compared from the first level's start.

## Phase 1 results

Each row is one case on seed 1: all 33 values compared change by change with the update each came at, up to two updates
before the script's quit. The scripts are in `build/identical/scripts/` (git-ignored; they are a few lines each, listed in
the case's folder as `script.txt`).

| case | what is played | updates | result |
|---|---|---|---|
| tsu3 | TSUNAMI from the menu, level 1, three balls released, the water to the top: game over at 6774 and its screen | 7498 | all 33 the same |
| think1 | THINK from a saved game: a ball dragged over a ledge into a word, the word sent, the level's end and bonus count-up, level 2, Save/Quit, Save Game, the menu | 3398 | all 33 the same |
| relax1 | RELAX from the menu, levels 1-10, the help, nine letters dragged (WORD made, 800 points), Save/Quit, Save Game, RELAX again, Continue, two more drags | 4798 | 32 the same; the generator's state: three single draws one update late (Oddities) |
| hs1 | TSUNAMI at level 9 (the 25 000 bonus), game over at 2495, the name typed, the score table, then a second TSUNAMI game at level 1 with a click and a right click | 5398 | all 33 the same |
| contp | the RELAX game saved in relax1 continued: Continue, the help, the level, two drags | 1998 | 32 the same; three single draws one update late, as in relax1 |
| hint1 | TSUNAMI level 1 with Get Hint clicked three times and a right click (at 0 points the button gives no hint in either program) | 2098 | all 33 the same |

Also compared, apart from the cases:

- **A saved game.** Both programs played relax1 up to its Save Game. The original's registry value (`reg export` after the
  run) and the port's `registry.ini` value hold the same 1886 characters, so the port writes the original's format with the
  same numbers. The contp case then loaded that game in both programs (it is the same text whichever program wrote it).
- **The score table.** After hs1 the original's TSUNAMI table and the port's are the same: the typed name with 25 000 on top
  and the five default names and scores below it.

## What was changed

Each difference was bisected to the first value that differed and found in the export. The commits are 0fce54f (the frame
loop, `-seed`, `-statelog`, the first precision fixes), 6a3b37b (the first word's bonus, the rope, sounds on the engine's
clock) and 4428ab6 (the streams' ends, the drag's precision, the count-up):

- **The frame loop.** The port ran a frame per wall-clock tick; it now runs the original's accumulator of 33 ms frames, one
  frame an update with `-fast`.
- **x87 precision.** The original computes at double precision and keeps intermediates on the FPU stack, rounding to float
  only where it stores. The port now does the same in the particles' emission credit, the ball system's 0.03 s step, the
  water and the fill, the physics' accumulation and integration, the collisions (dy is squared unrounded times rounded:
  `fsts` then `fmuls`, in Collisions, Distance and Stick), the bounds, the boxes and ramps, the mouse's rope, Follow and the
  chains, and the bonus count-up at a level's end (its step is kept in double for both sums).
- **The first word's bonus.** The ball system's last match (`mLastMatch`) is never set before the first word; in the original
  it holds the heap's leftover bytes "backgrou" (from a freed "background" string), so the first word is never a repeat and
  always counts as speedy (twice the points). The port starts from those bytes.
- **When a sound ends.** The music layers draw from `rand` when a clip ends. The original plays a sound of up to 2 s from one
  DirectSound buffer, which ends after its length; a longer one streams through a 2 s buffer refilled a second at a time
  (0x444270), and it stops when the cursor leaves the half that took its last data: at the whole seconds of its length plus
  one. With `-fast` the port now ends a sound by the engine's clock by the same rule (`HostSound::isPlaying`); without it, by
  the mixer as before.

## Frozen baseline

`build/baseline-wordharmony/` (git-ignored: a copy of `wordharmony.exe` and its DLLs made when this pass was done) is checked
by `tools/check.sh wordharmony`: its THINK and TSUNAMI games (seed 1) run in both the current port and the baseline must write
the same non-empty state log, so later fixes and mods leave unmodded play as compared with the original.

## Oddities (skipped)

- **The virtual clock's crawl and a sound's start.** origrun moves the clock 10 µs on each read once the main thread has read
  it 200 times without waiting. A frame that reads it often (a level's load) therefore plays its sounds a few milliseconds
  past the frame's start on the original's clock, and a stream whose end falls within about 10 ms after a frame's start ends
  a frame later than in the port. It happened after a saved RELAX level's load (relax1: three single draws one update late, at
  4093, 4318 and 4591, the same in three runs; contp: at 1293, 1523 and 1796); every value besides the generator's state was
  the same, and the clips' ends in THINK and TSUNAMI fell on the same updates. To see it: the port's clip of 8.75 s starts at
  update 3819 of relax1 and, by the rule above, ends at 3819 + 9000/33 = 4091.7, so at 4092; the original's at 4093. It is the harness, not the game: in real play the sound runs
  on the sound card's clock and the frames on the accumulator, and no two machines agree to the update.
- **The hint thread.** See above: `TerminateThread` on a thread that may hold the heap's lock, which the test patch avoids.
- **The uninitialized last match.** See "The first word's bonus": the original reads memory it never wrote; the port
  reproduces what that memory holds on every run of the original.

## Notes on the harness

- origrun waits at most 30 s of wall time for an `at` line's update (`wait_counter(N, 30000)`), and then lets the game run free.
  A TSUNAMI game left alone for 6800 updates took longer than that, so tsu3 adds moves of the mouse to where it already is
  every 1500 updates (in both programs' input).
- Git Bash turns `reg`'s switches (`/f`, `/v`) into paths, so `orig.sh` runs `reg` with `MSYS2_ARG_CONV_EXCL`; before that fix
  the original's registry key was never cleared between runs.

## Phase 2: looks identical

Phase two compared the screens, the sounds, the focus and the web build with the original, and played the game through
real input from launch to the end of a game. It ran on 2026-11-07 and 2026-11-08.

### How the screens were compared

A tour (`tools/shots/tourN.txt`) is a list of port `-script` lines with `screenshot <name>` lines among them. `make.py` turns
it into the port's script and origrun's, the same way phase one turns its cases, and the shots of each side go through
`tools/shotdiff`. The original runs only inside Windows Sandbox (`tools/shots/orig.sh`, `--sandbox`), the one place origrun
takes shots, on the virtual clock with phase one's patches. It draws into a DirectDraw back buffer that PrintWindow never
sees there, so origrun reads that buffer from memory and waits until the frame is not drawing. The port runs on `-fast`
(`tools/shots/port.sh`), and the web build runs the same script under Node (`tools/shots/web.sh`, through Bejeweled's
`wasmrun.js`). A shot of the original is taken at `at K-1`, as its input is: measured on TSUNAMI's falling balls, the
port's shot at 761 matched the original's "at 760", and no other update from 754 to 766 did.

The tours: tour 1 is the title, the main menu and its hovers, the high scores, the options with sound and music off and on
again, and THINK's levels screen. Tour 2 is phase one's tsu3, a TSUNAMI game from its levels screen and help to its game
over. Tour 3 is phase one's hs1, TSUNAMI at level 9 with its game over, the name typed and the score table, then a second
game. Tour 4 is phase one's think1: a saved THINK game continued, a ball carried over a ledge into a word, the word sent,
the level's end and bonus count, level 2, Save/Quit and Save Game. Tour 5 is the focus: TSUNAMI lost and given back the
window's focus, the P key, and the H key's help over the paused game.

### Results

| tour | shots | native port against the original | web build against the original | web build against the native port |
|---|---|---|---|---|
| 1 | 21 | 19 pairs: 5 the same, 13 faint, 1 differing (see below) | as the native port, and the Fullscreen box (below) in 6 shots | 15 the same; 6 differ only in the Fullscreen box |
| 2 | 21 | 4 the same, 17 faint | 4 the same, 17 faint | all 21 the same |
| 3 | 21 | 9 the same, 11 faint, 1 differing (a chain link) | as the native port | all 21 the same |
| 4 | 21 | 20 faint, 1 differing (a chain link) | as the native port | all 21 the same |
| 5 | 11 | the same screens, other letters (see Focus) | the same as the native port | all 11 the same |

Every faint pair was looked at in shotdiff's summary: they are the resampling of the scaled and rotated pictures (the
balls' edges, the blossoms, the water's waves) by a pixel's fraction, nothing that shows at the game's size. The flagged
boxes:

- **Tour 1, the menu at update 460.** The original's counter was still at 337 when the shot was due (its load on the
  virtual clock took longer than origrun's 30 s wait), so its shot shows the menu 120 updates early, with the falling
  blossoms elsewhere. Every later menu shot is the same. Its title shots at 150 and 300 were refused for the same reason:
  the original was inside its one long loading frame. They are not comparable and not a difference of the port.
- **The web build's Fullscreen box.** The options dialog's Fullscreen box is checked in the original and the native port,
  which start full screen, and empty in the web build, which starts inside its page. The box shows the truth on each; a web
  deviation below.
- **Tours 3 and 4, a chain link.** The ring between two chained balls is a sprite drawn rotated to the link's angle; the
  original's rotation filter leaves it a little softer than ours. It is in the same place at the same angle.

### What was changed

| difference | its cause in the original | what the port does now | commit |
|---|---|---|---|
| Chain links drawn from the first ball as well (a ring inside the first ball) | 0x412310 draws a link from each ball to the one before it, starting at the second node | starts at the second node | be330a14 |
| The hint box vanished when no hint was ready | `setHintReady(false)` fades the box to half (0x422ed3 pushes 0.5), greyed with "no hint available" | fades to 0.5 | be330a14 |
| The game link's code | the main menu's link out of the game (a button the port never showed) | removed with its action | d8bf6a6a |
| A splash held for two seconds | the splash stays two seconds of frames and the data loads after the first | loads at once and shows the menu (fast loading, a deviation below) | d41f48ec |
| On a machine with no sound device the music drew a random number every frame | (the port's: no voice meant the clip had ended) | on the engine's clock a sound plays its length whether or not a voice started; the web build under Node now plays tour 2 update for update as natively | d41f48ec |

### Sound

The original's sounds were compared by origrun's DirectSound stand-in, which logs every buffer's creation (with its size
and format), play and stop on the virtual clock (33 ms an update). The port writes the same with `-soundlog=<file>` (a test
switch: each start and stop with its frame and the sound's length), and `tools/identical/sndcmp.py` lines the two up: each
start of the original is matched with a start of the port of the same length at most two updates away. A sound longer than
2 s streams through a 2 s buffer in the original and is counted as "stream" on both sides.

| tour | starts in the original | matched | largest gap |
|---|---|---|---|
| 1 | 23 | 23 | 0.3 updates |
| 2 | 78 | 78 | 0.7 updates |
| 3 | 98 | 97 | 0.7 updates |
| 4 | 55 | 55 | 0.6 updates |

The one start left over in tour 3 is at update 5399, after the port's script had quit. The gaps under an update are the
virtual clock's crawl (Oddities). Music volumes and fades were not compared beyond this: both sides start the music
layers silent and fade them by the same code.

### Focus

Tour 5 takes the focus from a TSUNAMI game and gives it back: the original through origrun's posted WM_ACTIVATE,
WM_KILLFOCUS and WM_ACTIVATEAPP, the port through the same messages in its script (`focus lose`, `focus regain`). Both
games show GAME PAUSED with its CONTINUE button as the focus goes, stay paused when it comes back, go on with the P key,
and show TSUNAMI's help over the game with the H key and close it with H again. The
first run was in full screen, and there the original does something the port does not: it minimizes its window when the
focus goes (a full-screen DirectDraw window's own behaviour), and its main loop (0x4552b0) runs no frames while the
WM_SIZE flag says it is minimized, so the rest of the tour never came. The port's full screen is SDL's window on the
desktop's mode, which stays on screen and pauses the game as the windowed original does. That is listed with the
deviations. The tour was then run windowed on both sides (the setting `FullScreen` 0 in the original's registry and the
port's `registry.ini`). There the windowed original held the virtual clock at its splash for origrun's 30 s wait and then
ran free, so its random letters and blossoms are not the port's: every pair differs in the letter coming next, the bonus
word and the falling blossoms, and only 4 of its 17 sound starts line up. The screens themselves (the pause panel, the
help over it, the HUD, the hint box) are the same, and so is what each key does. The web build under Node played the same
tour update for update as the native port.

### Real input

Code played these games through the real input paths, with the moves chosen by the port's own bot (the test command
`move`), and judged them by the game's state and its own screenshots. Natively, `tools/realplay/play.py` starts the game
under rundesk and posts window messages: a click as one burst of move, press and release, a drag as a press, a move
every three updates and a release, and keys as WM_KEYDOWN, WM_CHAR and WM_KEYUP with the key's scan code (SDL takes the
key from the scan code; without it the name entry got no letters). In the browser, `tools/web/onegame.js` runs in the
player-only page (`?player=1&silent=1`) and sends the page's own DOM mouse and key events, holding the game between its
steps.

- **TSUNAMI, natively in full screen:** the menu, the levels screen, the help with PLAY GAME, 118 moves of play to the game
  over on level 2 at 4,800, the name typed into the name entry, the score table with the name in second place, and back
  to the menu.
- **TSUNAMI in the browser:** the same from the menu to the game over, the name typed and the score table, back to the menu.
- **RELAX, natively and windowed:** the options dialog's Fullscreen box cleared and the game switched to a window, the
  levels screen, the help, then level 1 (HARMONY made, 2,800), Save/Quit with its Save Game? box, Don't Save, the game
  over panel. The bot left WORD unmade there.
- **RELAX in the browser:** level 1 played to its end (WORD and HARMONY, 1,800), level 2's panel, then Save/Quit.
- **THINK, natively in full screen:** the levels screen, the help and level 1's field with 15 moves, then Save/Quit and
  the menu.

Every screen, button, drag, right-button submit, key and dialog that these games reached took its input as it should.
What no game finished is the bot's, not the port's: on RELAX's second level the letters lie in a maze of blocks the bot
cannot route through, and on THINK's first level it could not use a chain that had fallen over (its two balls read
backwards), so those runs stop at the level and leave by Save/Quit; the whole RELAX and THINK games came in the
finishing pass, below, once the bot could find its way. The same runs found three faults of the drivers, all fixed: a menu click taken during the menu's fade-in
was lost (the web driver now clicks again while the menu stays), the native keys lacked their scan codes, and a ball
stayed on the mouse when the page lost the focus during a drag (the framework forgets a button pressed before the focus
went); the bot now lets go of such a ball with a click, as a player's
next click does. The bot itself learned to read a chain as it lies, to slide a ball out from under a ledge, to carry
balls under a room's ceiling and to retry a plan once its anchor has moved (`README.md`, "Test commands").

### The fixes mod

`mods/fixes` holds two toggles, both on by default (`docs/README.md`, "Mods"). **steadyClock**: the original times each
frame by the machine's clock, so the same moves never play the same game twice; with the fix every frame is 33 ms of the
game's time, counted from the framework's updates, and so is the sound's tick. `tools/identical/steady.sh` plays the same
TSUNAMI input twice in real time: with the mod the two state logs are the same (2562 lines), without it they part.
**fairFirstWord**: the first word after starting the program always counts as quick (twice the points) in the original,
because the time of the last word is never set; with the fix it starts at 0 (NOSY scores 400 instead of 800). Without
mods the port still plays as the frozen baseline (`tools/check.sh wordharmony`).

### Fix candidates

None besides the mod's two. What was checked: every difference and oddity the two phases found (the virtual clock's crawl
and the hint thread are the harness's, the uninitialized last match is reproduced as the original has it, the frame clock
and the first word's bonus are the mod's two toggles); the waits a player meets (the splash and the loading are the port's
own, at the machine's speed; the screens' fades, the level end's bonus count and the water's rise are animations that the
game's own timing runs, as in the original); and the whole games of the finishing pass, which met no soft lock, stuck
control or lost save. A RELAX level whose letters lie walled in until a word breaks the stones around them is the level's
design, not a fault.

## The finishing pass (2026-11-08)

### Whole RELAX and THINK games by real input

The bot (`src/Bot.cpp`, `README.md`, "Test commands") learned what these games needed. It finds its way through the field
on a grid of 4-pixel cells, those a ball fits in apart from the blocks and the letters fixed in stone, with the shortest
way round other balls, so a ball is carried through RELAX's mazes. It uses the letters fixed in stone where they lie,
makes a RELAX word down a column as well as along a row, and fills a place over nothing last, holding the ball there
until the word counts. When no word is left in THINK and a chain lies fallen over, it lifts the chain's end over its
other end so that it reads forwards again. THINK never ends by itself (the next level comes only once a level's letters
are used), so with nothing left to make the bot leaves by Save/Quit and Don't Save, which ends the game with its game
over panel. The drivers reach a level with the test commands `goto` and `level` (`docs/TEST_COMMANDS.md`), and every
move after that is real input: natively through the shared loop `tools/playbymove` (`-realinput`, `-hold`, `-fast`),
in the browser through `tools/web/onegame.js` (`?level=`) in the player-only page with `?silent=1`, the pane hidden and its
animation frames shimmed.

| Game | Natively (playbymove) | In the browser (onegame.js) |
| --- | --- | --- |
| RELAX, level 100 (the last) | 8 moves: JAG, MEW, SMART and ZEN through a maze of blocks and stones, the level's end, the game over at 1,700 | 9 moves to the game over at 1,400, the name entry, the score table, the menu |
| RELAX from level 1 | 40 moves: levels 1 to 3 cleared (the maze of level 2 and the HORIZONTAL and VERTICAL crossing of level 3), level 4 to 9,500; the bot stops there, where the letters lie walled in by stones | |
| THINK from level 1 | 42 moves: levels 1 and 2 cleared (one chain flipped), level 3 to 4,400, Save/Quit, Don't Save, the game over | 35 moves: level 1 cleared, level 2 to 2,000, Save/Quit, Don't Save, the game over, the name entry, the score table, the menu; one focus loss on the way, paused and taken up again with P |

The browser's games are other games than the native ones (the page has no seed), and the native loop stops at the game
over (`"over": true`); the name entry and score table by real input natively are phase two's TSUNAMI run above. The
runs found three faults of the bot's, all fixed: the bot counted a drag by the game's frames while a move's waits count
the framework's updates, so in real time (a frame every three or four updates) a lift that never took was asked for
again and again on THINK's second level; it took letters fixed in stone for letters it could carry; and the shared
loop's step names are `press` and `release`, which the bot's `move` now writes (it wrote `down` and `up`; `play.py` and
`onegame.js` read the new names).

### Registration and outbound buttons

None left: the port has no registration, trial or buy code, and `openURL` (the engine's link) has no caller since the
game link went with its button.

### Performance

`README.md`, "Performance": the numbers, how they were taken and how many game processes ran.

## Deviations left

- **Fast loading.** The original shows its splash for two seconds of frames and loads its data after the first; the port
  loads at the splash's first update and shows the menu as soon as it has loaded (CLAUDE.md, "Port first, unmodified").
  `-splashhold` keeps the original's hold for runs compared with it (every case, tour and `tools/check.sh wordharmony`).
- **The hint search.** The original searches for hints on a thread; the port searches at once. Phase one's test patch makes
  the original do the same, so the comparisons agree; in real play the original's "no hint available" can show for a few
  frames at a level's start while its thread runs.
- **The game link** is gone with the button that the port never showed.
- **Full screen and the focus.** The original's full screen is DirectDraw's own mode, and its window minimizes when the
  focus goes, so its frames stop until the player comes back. The port's full screen is the framework's borderless window
  at the desktop's mode, which stays on screen: the game pauses on the focus loss as in the windowed original, and its
  frames go on behind the pause panel.
- **The web build's Fullscreen box** starts empty, because a browser page starts inside the page and grants full screen
  only after a click (`README.md`, "Status"); the native port and the original start full screen with the box checked.
- Not compared in phase one: a hint actually shown (the cases had no points to pay for one), recycling letters, the options
  dialog, the help pages, the editor, a game cut off by quitting. The last four have no effect
  on the game's state; phase two looks at their screens; the first two are left for a case with points.
