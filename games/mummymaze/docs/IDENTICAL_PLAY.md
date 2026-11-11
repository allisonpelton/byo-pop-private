# Mummy Maze Deluxe: the identical-play pass, phase one (2026-11-03)

Phase one asks whether, given the same input, the original and the port play out the same: the same mazes, the same
random draws, the same moves, scores, timers and endings. Phase two (screens by shotdiff, focus, the web build) is
written up at the end of this file.

## What was compared and how

The port's autoplay bot (`games/mummymaze/src/testhooks/Autoplay.cpp`) plays a game and logs, once per update, the values
`board`, `state`, the explorer, both mummies and the scorpion's cells, `death2`, the score, the maze and file numbers, the
chamber and pyramid, the game mode, the level clock's base, the undo count and the C library generator's state word.
`games/mummymaze/tools/identical/identical.py` turns the bot's input log (every click and key with its update) into an
origrun script. The original, run through origrun on its virtual clock with a scripted pointer, gets the same input at the
same updates (its updates are the port's plus 2301, the menu click being at 2400) and `--watch` reads the same values
inside the game. The two change lists are compared value by value. The original's C library generator (`rand`) was given a
static state word by two patches to the copy, because the original's `rand` keeps its state in thread storage and could not
be watched; the copy is seeded the way the port's generator is.

Compared and identical, to the end of each list that was read:

- classic, seed 21, 16000 updates, and then the whole game: all values to the original's last logged update (about 200000
  updates, several pyramids; the original's runs end at origrun's 240 s timeout, and the generator's watch is slower than
  the others and got to 21600 updates);
- **the whole classic game, seed 21, to its victory** (about 1,008,000 updates, 225 chambers, 15 pyramids; origrun's runs of up to an
  hour): the mummies' and the scorpion's cells (3098, 2797, 1005, 996, 871 and 840 changes), the death timer, the score, maze,
  file, chamber, pyramid, game mode and clock base (706 changes) are the same at every update, to the victory screen. origrun
  keeps about 4000 changes of a watched value, so the board, state, explorer and undo lists are compared to their first 3961
  state changes (update 125029) and the first 3900 explorer and 3998 undo changes; the generator's state is compared to the first
  21000 updates (its watch is slow). What is compared of the rest of the game is every other value above, which an out of step
  explorer or a drifted generator would have thrown off within the chamber;
- classic, seed 77, 60000 updates, into pyramid 1;
- adventure, seed 5, 16000 updates (chambers, the map, every draw);
- the tutorial, seed 5, to its end and the classic game that follows it (1368 generator changes);
- **a game's end and the high scores:** a classic game ended by the bot being caught and clicking Abandon Hope, then the name
  entry and the table (seeds 1 and 4: ranks 1 and 5), and the victory path (a saved game in the last chamber of pyramid 14,
  played to the victory screen, its name entry and table). The original's `hiscores.dat` and the port's are byte for byte the
  same in all three;
- **resume** (`tools/identical/resume.sh`): a saved game written by the port and continued by both programs, the original given
  the file in its copy and the clicks on its Continue question: mid-level (saved 1500 updates in), with the level done (the next
  chamber's maze picked by the generator, `0x41c600`), and in pyramid 14 (a crafted file: the same layout with other numbers).
  Every watched value matches, the level clock's base and the generator included. The reverse direction, a game saved by the
  original with its Quit to Main button, was compared through the file: the original's `classic.sav` and the port's, from the
  same input, are the same bytes except the time of day that the game writes into the Internet score buffer (about six
  characters of its web string).

The tests of the port's own suite (`tools/check.sh mummymaze`) pass with the same lines as before the pass: a classic game,
the pyramid-3 start, a whole game to the victory, an adventure game, the tutorial and a saved game that is continued (level
time -90, the signed elapsed time of a save made before the clock starts). A frozen baseline of the port,
`build/baseline-mummymaze/mummymaze.exe` (a copy of the exe made after the last fix of this pass), plays the classic, the
pyramid-3 and the tutorial games again in the suite; their logs and screenshots must stay byte for byte the same, so later
fixes and mods leave unmodded play unchanged. It is refrozen only on purpose, with the reason written here.

## Changes the pass made

| Difference | Cause in the original | Port now | Commit |
|---|---|---|---|
| The adventure's level-complete screen had the next-chamber and map links the other way round | `0x424f00`: in adventure the next link is at height - 0x5f and the map link at height - 0x37 (none past chamber 13); in classic the next link is at - 0x37 | the same positions | d46d16e |
| The tutorial drew 12 numbers fewer than the original when it started, so every later random draw (the trap animation's) was out of step | the tutorial's start (`0x41c3c0`) calls `0x41b7b0` first, which starts a board when no world map is up (its ten generator draws and the classic maze pick's three), and then replaces it | `StartTutorial` calls `RemoveLevelComplete()` first | 3309121 |
| The game panel's two right-hand links were the wrong way round: the port's Abandon Hope click landed on the original's Quit | `0x4137a0`: the link made third (id 2) is the lower one, the fourth (id 3, Abandon Hope) the upper | the same places | 5da17f8 |
| The Quit to Main dialog had a computed height (and no header when it was sized), so its buttons stood about 36 px higher than the original's | `0x41cfb0` gives it its header and a fixed height: 0xdc for the question about quitting the game, 0xf0 for the others | the same heights | a312ab0 |
| (not a play fault) The level data was in the repository | | read from the player's own WinMM.exe at start-up, checked by hashes (`GameLevels`) | c6a22b1 |

The first two were found by the comparison; the earlier ones only in tooling (the bot logs the real button centres, the
origrun script moves the pointer before a click), in 8020783 and cb85566.

## Oddities skipped

- **The maze and file numbers of the end scenes and of the tutorial board.** The original's board for the game-over and
  victory scenes and for the tutorial never sets them, so it shows whatever its heap block held (the board before's, or
  random bytes); the port holds 1 and 0. Nothing reads them there. The comparison skips those two values while such a board
  is up. To see it: watch `[board]+0x2b0` in the tutorial.
- **Registered and unregistered.** The original is the unregistered build: it counts its plays in the registry
  (`TimesPlayed`); at ten plays it nags, and between ten and nineteen it asks the generator for one more number before
  deciding, and at a high score it asks to register where the registered build asks for a name. The port is the registered
  build. origrun's patch list for the copy makes the copy answer its registration check with yes (the list is git-ignored), so
  both are the registered build, and the comparison also resets the count to 0. The unregistered build's own flow (the nag, the
  register question) is not compared.
- **The original's runs are long.** The whole-game comparison is cut at the original's timeout rather than at the game's
  end; it is not an oddity of play.

## Deviations that remain

No deviation in play is known. Not compared by running the original: a saved game written by the original and continued by the
port (compared as files instead: the formats are the same bytes), how the dark maze of pyramid 14 is drawn (phase two; its
play was compared through the crafted save), the caught and try-again paths beyond what the whole classic and adventure games
hit, and adventure beyond 16000 updates. A comparison longer than origrun's 240 s limit is cut where the original's log ends
(the generator's watch is the slowest and gets to about 21000 updates).

# Phase two: looks identical (2026-11-03)

## What was compared and how

`games/mummymaze/tools/identical/p2.sh <scenario> <name> <seed>` runs one scenario in the port (under rundesk, `-fast`, a
fresh user folder) and in the original (origrun in Windows Sandbox, virtual clock, scripted pointer), each taking
screenshots at the same updates (the original's are the port's plus 2301; `p2.py` turns one scenario file into both scripts).
`tools/shotdiff` pairs the shots (the original cropped to its client area); every flagged box was looked at in its crop.
A scenario is either written by hand or `p2.py fromlog` replays a phase-one bot log (its keys and clicks) with a shot every
few hundred updates. Scenarios (in `tools/identical/scenarios/`):

- menu and title, classic start (s1); a classic game of 6000 updates, 20 shots (c6); an adventure game, 28 shots (adv);
- a saved game in pyramid 14, the dark pyramid, from the Continue question on, 12 shots (dark);
- the Options, Quit to Main, High Scores and Credits dialogs (d1, d2, d3);
- the tutorial from its menu link to its end, 16 shots (tut);
- a classic game to its end: caught, the Abandon Hope click, the game-over scene, the name entry and the table, 24 shots
  (hs1);
- losing and regaining the focus on the board and with the Options dialog opened (focus), 7 shots.

Every pair ends faint or matching with no box left to judge. The web build (rebuilt from this tree) was run under Node
(`wasmrun.js game`): the classic bot game of the suite (312 log lines) gives a log byte for byte the native one, and
`tools/identical/web.sh` runs a scenario in the web build, whose shots (the tutorial, the end of a game, the classic
game and the dark pyramid, 72 shots in all) are pixel for pixel the native port's (`--allow 0`).

**Sound.** The original plays its samples through DirectSound (the music is a module mixed by BASS into one streaming
buffer). `origrun --sound wav` logs every buffer, and `tools/identical/soundcmp.py` sets its plays (a duplicate of a loaded
buffer, named by size and rate, with volume and pan, time in virtual ms) against the port's `BYOPOP_SOUNDLOG` plays, in
order and by update (within 2). The classic game c6 (28 plays) is the same in both; the end-of-game scenario (hs1, 286 port
plays against 290) is the same in order, sample, volume, pan and update, except for the double click below. The music is FMOD 3 (`fmod.dll`, 16 imported functions), not BASS: `tools/fmodproxy` is a logging stand-in for it (the
bassproxy of Big Money, adapted: placed beside a copy with the real DLL renamed `fmod_real.dll`, it logs every call with
the game's update counter and passes it on; the song is loaded from `music\game.it`). `tools/identical/musiccmp.py` sets the
original's `FMUSIC_SetOrder` calls (the song and its order) and `StopSong` calls, by update, against the port's music events
(`play` and `fadein` with the order, `stop`): in the end-of-game run all 26 orders are the same in update (within 2) and
order number, and the 41 stops are the same in update (within 1); the port's two further stops are the shutdown's, after the
original's run ended. The master volumes follow the same ramps (the port's volume scale is its own mapping).

## Changes the pass made

| Difference | Cause in the original | Port now | Commit |
|---|---|---|---|
| The side panel had no pyramid map, pyramid number or ankh position, and its buttons' lit and pressed pictures were wrong | `0x42a2c0` draws the map block (chamber marks and map head from two 15-entry tables, the small treasures clipped to the pyramid, the "Pyramid" text and number in classic); the buttons load only their over, down and (world map) disabled pictures | the same | c6930e8 |
| Dialog headers and buttons were in each other's fonts, and their buttons were drawn with the framework's box | `NewDialog` (`0x41aa90`): button and lines font mFont1, header font mHeaderFont; `DialogButton` (`0x430050`) draws with the 2003 `DrawImageBox` | the same (`Button2003.h`) | 4c1c655 |
| Walls and the skull trap's block were missing in front of the explorer and the mummies | the second loop of `DrawBoard` (`0x4095f0`) draws the trap block and the near walls over the figures | `Board::DrawNearWalls`, `DrawTrapBlock` | 91c42f7 |
| The music faded in and out at half the original's speed | `MummyApp`'s update (`0x41e9b0`) calls the framework's update and then the music interface's update a second time, so every fade step happens twice per update (found by the FMOD call log: 125 updates for a fade of 250 steps) | the same second call | (this commit) |
| The dark pyramids' window of light was put at the explorer's cell corner, not around the explorer | `0x40cb70` places the window at the cell's pixel plus half a cell minus half the window, a size-dependent offset and 10 | the same | (this commit) |
| (a test run only) The level-complete bonus read the wall clock when the port ran `-fast` from a script, so the score differed from the original's by a few ten points | the original reads its clock, which origrun runs virtually | `GameTick` counts updates (10 ms each) under `-fast` as it already did under the bot; normal play still reads the clock | (this commit) |

The baseline `build/baseline-mummymaze` was refrozen after the second and third rows: the screenshots of the tutorial game changed
(the logs did not), and the suite reports "baseline identical" again.

## Oddities and remaining deviations

- A click while the window has lost focus is dropped by the port's script path but acted on by the original; a real click
  first reactivates the window, so no player meets it. Not chased.
- **A double click sound at a dialog's Yes button.** In the end-of-game scenario the original plays the click sample twice
  at the same update whenever the bot's Yes click lands on a dialog's button (4 of 4 times; all other buttons click once);
  the port plays it once. The press path in our export (`0x42e7f0`, the dialog's listener forwarding to the application's
  `ButtonPress`) shows one call, and the second was not found in about 25 minutes. Not chased.
- Not compared: the unregistered build's nag and register screens. (The trap death was compared frame by frame on 2026-11-11,
  below: this pass had only seen its first frames.)

# Phase three: real input, the fixes mod and the registration code (2026-11-08)

## Real play: what was played and how

The test commands (`GAME_NOTES.md`, "The standard test commands") let a driver ask the port's own solver for its next move and send it
as real input. Nothing here goes through the script path: natively `tools/realplay/drive.py` starts the exe under rundesk with
`-commands` and `-realinput`, so each move reaches `HandleEvent`, the window-to-game mapping and the widget manager as SDL events (a
click on a cell, or an arrow key held for one update); on the web the test page's `playByMove` sends DOM mouse and key events to the
canvas. The driver reads the game's `state` to know when a step is taken, and records a step that left the explorer anywhere but the
cell the move meant ("off track"). It never chooses a move itself, and nobody played.

| Where | Input | What was played | Result |
|---|---|---|---|
| Native | arrow keys, `-fast` | the tutorial from the menu link to the hand-over to classic (55 moves) | played to the end, no error |
| Native | clicks on cells, `-fast` | a whole classic game: 225 chambers (7919 moves), the end-of-game name box (`bot`), the victory screen | played to the victory screen in 18 minutes, no error, no step off track |
| Native | clicks on cells, `-fast` | a whole adventure: the name box, 225 chambers over 15 pyramids with the world map and 15 treasures (8958 moves), no deaths, the victory screen, then the menu link | played to the end in 21 minutes, no error |
| Native, windowed and full screen, real time | arrow keys and Space | 120 moves over 4 chambers with `Alt+Enter` into full screen and out again, the focus lost for 200 updates and regained, the window minimised and restored, a resize to 800 x 600, a screenshot at each | no error, no step off track; the frames show the board drawn as it was before each event |
| Web, `?player=1&silent=1&fast=1&hold=1`, fixes mod on | arrow keys | the tutorial (240 moves, handed to classic), then 200 moves of classic (124 steps, 8 chambers) | no error, no step off track |
| Web | clicks on cells | a whole classic game: 225 chambers (31112 moves), the name box and the victory screen | played to the victory screen in 15 minutes (2.7 million updates), no error |
| Web | clicks on cells | a whole adventure: the name box, 225 chambers over 15 pyramids with the world map and the treasures (18011 moves), no deaths, the victory screen | played to the end in 3 minutes, no error |
| Web, the page's canvas | clicks and keys | the focus lost (the page's `blur`, the document hidden) for 200 updates and regained; the canvas drawn at 0.6 and at 1.5 times its size, 20 moves each | the game kept updating while the focus was lost and played on after it came back; every click landed on its cell; no step off track |

What the runs cannot cover, and why. The bot always wins its chamber, so no run met the caught screens; the original's end-of-game,
caught and Abandon Hope scenes are the phase-two scenario `hs1`. The browser refuses the Fullscreen API without a hand's own gesture
(`Permissions check failed` from a script's call), so full screen was played natively only; the page draws the canvas at any size, which
the two scaled runs cover. A key reaches the game as a press of one frame in the page and as a `keydown`, one update and a `keyup`
natively. In a native `-fast` run the driver is slower than the game and a key held through its answer takes a second step, so
the long `-fast` runs use clicks, which go through `Board::MouseDown`, the same code a hand's click on the cell next to the explorer
reaches; arrow keys were played in real time (one double step in an earlier run under heavy machine load, which a late `keyup` explains,
and none in the last run). The scores of the native runs are lower than the page's for the same moves (170915 against 249665 in
a classic game): the `-fast` game runs on while the driver thinks, so the level-complete bonus, which is read from the game's clock,
runs out. It is a property of the run and not of the game.

Two things worth knowing about the web runs. The hidden Browser pane gives the page about 60 frames a second, and each frame runs
about four updates under `?fast=1`, so a whole game would take hours; a frame source that does not wait for the display
(`requestAnimationFrame` replaced by a `MessageChannel` post in the page, before the game starts) made the same page run at several
thousand updates a second and the whole classic game take minutes, with the same states (the game's clock is virtual there). The
`move`s of this port print the documented step form (`{"click":[x,y]}`, `{"key":"ArrowUp"}`).

The two whole native games were played on the build before the last merge of the trunk; after it the suite (baseline identical), the
tutorial, 150 moves of an adventure and both whole web games were played again.

## Real play found nothing to fix in the port

No run found a control that stopped working, a soft lock, a crash or a lost save. The checks that did fail on the way were all the
test harness's: the `-realinput` channel answered a bare `move` with its own "needs <x> <y>" error (the trunk
has the same fix now, and its `playMove` plays the documented step forms), `FindWay` gave another first step each time it was asked and walked a chamber in
circles (the bot now keeps its plan while the board keeps to it), and `setup_worktree.sh` does not copy the game's baseline.

## The fixes mod

`mods/fixes` holds two fixes, each a toggle, both on in the site's build and off in a plain native run (`GAME_NOTES.md`, "Fix
candidates" and "The fixes mod"). `gameClock` counts a chamber's time and the adventure's total by updates, as the game's own clock on
the board does, instead of by `GetTickCount`; `solverFight` makes the search of Abandon Hope and the Ankh flip the gate in the case the
board flips it (a fight of two mummies that ends on the key stone), which they missed. Checked as follows: the suite's baseline (the
unmodded game) still gives the same logs and photographs, the bot's classic game gives the same log with the mod on and off (no game of
the suite meets the fight, so `solverFight` is checked against the board's code and not by a played game), and every web run above
had the mod on. A fix that was left out on purpose: the Ankh on the larger mazes is a mod, not a fix.

## Registration and the buttons that lead out of the game

Taken out with this pass (`GAME_NOTES.md` lists them): the Options dialog's Register button and its web page button, the title's
register link and the play counters, the unused registration pictures and the register dialog's id. The only visible change is the
Options dialog, whose Tutorial and High Scores buttons now share the first row, with Credits across the second; the suite's three
Options photographs changed with it and the baseline was refrozen there, and every log and every other photograph is the
baseline's.

# The user's faults of 2026-11-10: the title's thanks and the falling block

The user: "Mummy Maze still thanks you for registering, and the falling block trap doesn't seem to be animated properly."

## What was compared and how

A death on a skull trap had never been compared past its first frames: no bot walks onto a trap, and no scenario of phase two
did. `tools/identical/trapscn.py` makes the scenario. The port saves a classic game in a chamber whose maze has traps (the
`level` test command twice, so that the board is of the chamber's own pool, then Quit to Main; seed 2 gives maze 99 of file 6,
with a trap one step above the explorer). The script reads the board from the `state` command, which now reports the traps, the
death's timer and kind and the walls, lists every walk of up to six steps from the explorer to a trap, and has the port try each
one with a few seeds until the death is of the kind asked for: the game draws it at random, the block or the pit. It then writes
the scenario for `p2.sh`: the saved game continued, the traps' tip closed, the walk, and a photograph of both programs every
second update from ten updates before the trap to the lost panel, 150 of each. The original ran in Windows Sandbox on the
virtual clock from the same file.

Both kinds were compared, the block (seed 21) and the pit (seed 3), 300 pairs with `tools/shotdiff`. The web build ran the
block's scenario and the title under Node and gave the native port's pictures pixel for pixel (150 of 150 and 1 of 1,
`--allow 0`).

## Changes

| The difference | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| The title showed a medallion reading "registered version" and the line that thanks for registering | `0x42aaf0` draws the registration state: the `trialcircle` picture when registered and one of four lines over the bar | neither is drawn; the picture is not in the port's list and the line is not in its text table | a4c8aaab |
| The block stood on the explorer from the first update it was drawn, 139 of 150 frames differing | `0x40abdc`: the block's height above the cell is three pixels for every unit that the square of the death's timer over 85 is short of 152, so it comes down from above the window and lands at the 114th update; the port had left the fall out and drew the block at the cell's top | `Board::DrawTrapBlock` computes the same height (in doubles; checked against exact arithmetic for every timer value and position) and stands the block on the cell's lower edge | c41698f9 |
| The lost panel blacked out the board and the side panel behind the rising mummy, in every death | `0x412ea0` ends by setting the panel's fade to 1 (the two halves of the double, 0 and 0x3ff00000, read as "0" in the port); `0x413730` paints black only below 1, which happens when the player leaves for the main menu | the panel's fade starts at 1: the board stays in view behind it | c41698f9 |
| The adventure's level-complete screen had no plural endings in its time line and lacked the two lines about the next chamber (found in the readability pass, 2026-11-11, by a comment that said "not ported yet") | `0x422b00` adds the text at 0x4923f4 to "hour", "minute" and "second" unless the number is 1 and the punctuation at 0x494f54 after the minutes, then a switch over the chamber draws two lines at 0x13f and 0x15d (one at 0x14e before the pyramid's last) | `NextLevelWidget::Draw` does the same; five photographs of the screen match the original pixel for pixel | the readability pass's last commit |
| Quit to Main in an adventure went to the main menu and saved nothing: the time played in the chamber since the last save was lost (found by `tools/splitfields.py`, 2026-11-11: the export's handler touches the app's time and the map, the port's did not) | `0x4201e0` case 0x7d3: in an adventure the saved game is read into a map, the time played in this maze is added unless the chamber's result is up (which has added it), and the map goes again, which writes the file | `MMApp::ButtonDepress` does the same (`AdventureLeave`, or a map made and removed when the result is up); compared with the original in Windows Sandbox (a new player, a chamber entered, 20 seconds played, Quit to Main, the slot opened again): the statistics read 0 hours, 0 minutes and 20 seconds in both, and the four photographs match apart from faint differences of one level; the web build under Node matches the native port's four exactly | this commit |

After the changes all 300 pairs are faint only. Two small boxes are left in every frame, before the death as well: a strip
of 7 pixels at the maze's right edge (below) and 5 pixels of at most 12 levels in the side panel.

## Oddity

- **A wall's top face is one pixel wider in the original at the maze's right edge.** In maze 99 of file 6 the south wall of
  the cell in the last column (5, 2) reaches screen column 576 in the original and 575 in the port, 7 pixels in all, in every
  frame. The export shows the same clip rectangle for the maze in both drawing loops (`0x4095f0` and `0x40b2f0`: 0x32, 0x42,
  0x16e, 0x177) and the same rectangles for the wall, so the port cuts where the export says; the cause was not found in about
  twenty minutes and shotdiff does not count it as visible. To see it: `trapscn.py`'s scenario, any frame, the crop at 566, 238.
