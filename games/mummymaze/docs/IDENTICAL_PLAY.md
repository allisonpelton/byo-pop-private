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
plays against 290) is the same in order, sample, volume, pan and update, except for the double click below. The music is
not compared by state: the original's module is rendered inside BASS and reaches the log only as mixed audio, so its order
changes and fades cannot be read as events; the port's own log lists them (`music fadein/fadeout`, offsets as the
translated calls give).

## Changes the pass made

| Difference | Cause in the original | Port now | Commit |
|---|---|---|---|
| The side panel had no pyramid map, pyramid number or ankh position, and its buttons' lit and pressed pictures were wrong | `0x42a2c0` draws the map block (chamber marks and map head from two 15-entry tables, the small treasures clipped to the pyramid, the "Pyramid" text and number in classic); the buttons load only their over, down and (world map) disabled pictures | the same | c6930e8 |
| Dialog headers and buttons were in each other's fonts, and their buttons were drawn with the framework's box | `NewDialog` (`0x41aa90`): button and lines font mFont1, header font mHeaderFont; `DialogButton` (`0x430050`) draws with the 2003 `DrawImageBox` | the same (`Button2003.h`) | 4c1c655 |
| Walls and the skull trap's block were missing in front of the explorer and the mummies | the second loop of `DrawBoard` (`0x4095f0`) draws the trap block and the near walls over the figures | `Board::DrawNearWalls`, `DrawTrapBlock` | 91c42f7 |
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
- Not compared: the unregistered build's nag and register screens, the trap-death animation frame by frame (its first
  frames and block are drawn as in the original, found by shotdiff), and the music by state (above).
