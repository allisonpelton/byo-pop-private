# Mummy Maze Deluxe: the identical-play pass, phase one (2026-11-03)

Phase one asks whether, given the same input, the original and the port play out the same: the same mazes, the same
random draws, the same moves, scores, timers and endings. Phase two (screens by shotdiff, sound, focus, the web build) has
not been done.

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
| The Quit to Main dialog had a computed height (and no header when it was sized), so its buttons stood about 36 px higher than the original's | `0x41cfb0` gives it its header and a fixed height: 0xdc for the question about quitting the game, 0xf0 for the others | the same heights | (this pass's last commit) |
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

## Not compared

Phase two: screens, dialogs and texts by shotdiff, music and sound by state, losing and regaining the focus, and the web
build.
