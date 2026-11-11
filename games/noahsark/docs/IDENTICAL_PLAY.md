# Noah's Ark Deluxe: the identical-play pass

"A game is done when it plays identically to the original" (CLAUDE.md, 2026-10-27). This file records the pass in
phases. Phase one is play: with the same input, a game plays out the same in the original and in the port (boards,
random draws, moves, scores, levels, timers, endings). Phase two (screens, sounds, focus, the web build) is the last section.

## Phase one (2026-11-03)

### How it was compared

The original (`WinNoah.exe`) runs only through `tools/origrun` (`docs/ORIGRUN.md`), from a copy, on the virtual clock
(`--clock virtual`, exactly 10 ms per update, measured: the game's tick at a level's start is 10 x update + a constant
in all three levels of a run). Its C library generator is pinned by `--seed` (a patch note kept out of the
repository, `tools/origrun/patches/noahsark.txt`), and the port runs with `-seed=N -fast`; the two generators are the
same MSVC `rand`, so the same seed gives the same draws.

What is compared is the game's state, read from the original's memory by `--watch`: the board's state
(`board+0x310`: get ready, playing, won, lost, level done, ...), the score (`app+0x474`), the level (`app+0x43c`) and
the mode (`app+0x444`). The port writes the same four values in `-statelog=<file>` whenever one changes. The port's
bot plays a game by itself (`-autoplay`, with `-autoplaymenu` so that a script does the menus); its clicks are written to its log,
`games/noahsark/tools/origrun_replay.py script` turns them into an origrun script (the bot's update U is the
original's counter U+1, because the port's log is written after the update), and the original gets exactly those
clicks at the same counter values. `origrun_replay.py compare` then checks, update by update, that the two logs hold
the same rows, in both directions. `games/noahsark/tools/compare_mode.sh <name> <menu script> <mode> <cap>` does the whole
series in one command (the port's run, the conversion, the original's run inside Windows Sandbox, the comparison).
`-statelogcells=` adds a few cells' types and the water's level to the log.

Runs of the original on the host stopped receiving input at some point in the session (cause not found: other
sessions ran their own origruns at the time), so the original runs inside Windows Sandbox (`SANDBOX=1`, one minute
per run).

Series compared: action 9000 updates (levels 1 and 2), strategy to its first level and a half (since the walker fix
below: to its first two levels and up to the start of the third), puzzle 9000 updates up to its game over, the name entry
and the high-score screen, and the first level of an action game on medium and on hard difficulty (the same clicks,
the difficulty button of the dialog being the only change). Past the get-ready difference, with the resync below: strategy to its third level and action on medium and on hard to their third. Not compared: stampede levels (level 4 of action on easy, behind the click-dependent get-ready end) and a series of several games.

The random draws are compared too, since a draw taken by another consumer is the first thing that shows when two games
part. The port counts its draws (`CrtRandCount()`, `CrtRandSnapshot()`) and `-statelogcells=r` or `c` writes the
generator's state after every 512th draw, or the number of draws made at each update; the original's `rand` and
`srand` are replaced in its copy by ones that keep the state, a count and the same snapshots (a patch note of the
copy, kept out of the repository with the other patches). `games/noahsark/tools/rngcmp.py` maps a snapshot to its
draw number (running the generator from the seed) and says at which update the two games reached it, and
`games/noahsark/tools/rngc.py` lists the updates in which the two made a different number of draws. The original keeps at
most about 4000 rows of a watch, so a run is read in windows (the snapshot from a chosen draw on).

### Comparing past the get-ready (the resync)

The get-ready of a level (the waiting animals' release, see Oddities) does not end the same way in the two games, which
moves the generator and with it every later board. So that the rest of a level can still be compared, the port takes over
the original's generator state at two points: at each level's start (right after the level counter went up) and at the
end of each get-ready (`-resync=<file>`, lines `n <level> <state> <update>` and `r <level> <state> <update>`; at an `r`
event the port also holds its own get-ready end until the update the original's ended). The original's states are read
from a run of its copy with the generator patch (`games/noahsark/tools/resync.py`), and
`games/noahsark/tools/resync_mode.sh <name> <menu script> <mode> <cap> [seed] [difficulty] [rounds]` repeats port run,
original run and conversion until the event list stops changing, then compares a final pair of runs. Level boards and
everything after a get-ready then agree again, and a comparison reaches one level further for each round.

What the resync cannot give: it assumes that the original plays out the same in two runs of the same clicks. That fails
where a click decides the original's get-ready end (the end moves with the clicks, which move with the port's end), as in
the third level of an action game on easy, and where the original's timing differs between two runs (the end of strategy's
third level came at 12193 in one run and 12197 in another, so the next level's generator state, taken from another run,
no longer fits). The comparison stops there; it is not a difference of the port.

### A series of games, and a saved game (puzzle)

The two-game series is one run: the bot plays a puzzle game to its game over, enters a name, the script returns to the main
menu, picks puzzle again and the bot plays a second game to its game over and name entry
(`compare_mode.sh` with a menu script that holds the second game's clicks; the clicks are merged with the bot's by update). A
saved game is made by the in-game Quit to Main at update 1500 and its Yes at 1560 (the script's clicks), resumed with the menu's
Continue button at 1800 and played on to update 14000; a second run stops right after the save and the two games' `savegame.dat`
are compared as files. The original's copy needs two things for this, both patches of the copy and not of the repository: the
unregistered game stops a puzzle game after 6000 board updates ("Time up, register now?"), and its high-score dialog offers
to register instead of asking for a name, so the copy counts as registered, as the port always does. An earlier run
of the first game compared only the part up to that stop (the compare tool skips what the original no longer logs) and
missed it; the compare tool now prints a note when the port has changes after the original's last logged update. The name entry's text is typed 3
updates later in the original than the port's bot types it (the dialog is not yet up in that update; the
`REPLAY_TEXT_DELAY` of the replay tool).

The saved game differed in two places, both from the file's format: the generator's state is a string of 2496 bytes (a length,
then the 624 words) and no index follows it, and a piece's float position (`mPixelX`, `mPixelY`) is the float's four bytes,
not the integer part. The port wrote the index and the integer parts, so its saves differed from the original's and could
not be read by it (the port reads the original's file now: a game saved by the original resumes in the port with the same
score and the same play afterwards). The level's start tick in the file is the tick count of the game's clock and differs
between the original's clock and the port's (two bytes), which is not a difference of play.

### Result

| Mode | Result |
|---|---|
| Puzzle, two games in one run (874 changes of state, score, level) | identical: game over, name entry, the high-score screen, back to the menu, a second game to its game over and name entry; the high-score files are byte for byte the same |
| Strategy, levels 1 to 3 (resync) | identical (every state, score, level, board); level 4 not comparable (see above) |
| Action, levels 1 and 2 | identical, except the start of level 2 without the resync |
| Action on medium and on hard, levels 1 to 3 (resync) | identical (342 and 519 changes of the original, none different) |
| Puzzle, a save by Quit to Main, Continue and play on (576 changes) | identical to update 14000; the saved game written at the quit is the original's byte for byte except the level's start tick (the two clocks) |
| Action on easy, level 3 | not comparable (the get-ready end depends on the clicks, see above); stampede (level 4) not reached |

### Changes (each is the original's own logic, from the Ghidra export)

| Change | Cause in the original | Commit |
|---|---|---|
| The saved game (`savegame.dat`) has the original's format | 0x40dbe0: the generator is saved as a string of 2496 bytes with no index after it, and the pieces' `mPixelX` and `mPixelY` are written as the floats' own four bytes; the port wrote a count of words, an index and the integer parts | 3010c88 |
| A falling animal's cell gets a **new piece** copied from the lower one; the old lower piece is deleted | 0x40407c: it copies the position, type, alpha, offsets, flags, fall speed and path into a new piece (blink, scale, flash, pending and match fields start at their defaults) and keeps the partner link only outside puzzle and stampede. The port swapped the two pieces, so a swapped piece kept its blink and the per-update random draws (one per piece with blink 0) came out in another number | ace140c |
| Flights to the ark and the fading of abandoned animals use the original's arithmetic | 0x405408 and 0x405440: int-truncated distances, a double square root, the step `(dx / dist) * 5.3` with a floor of 1, a clamp written with an unordered compare (the x87 treats a NaN as "not greater", so a zero distance snaps the animal to its target); on a zero or NaN position the piece becomes empty | ace140c |
| The ark rises to water - 0x37 in the get-ready | the original takes the water level before the +3 step and subtracts 0x34 | ace140c |
| The next-level screen adds 1 to its drawn tip in strategy | `mTip = rand() % kinds; if (mode == strategy) mTip++` (the port had a dead `mDifficulty == 0 && false`) | ace140c |
| A walking pair keeps its own cell when the animal above it falls past it | 0x40407c copies the lower piece, position included, into the falling animal's slot, and moves only the old upper piece (its `mY` + 1, its pixel and offset one cell down). A walker that has left its cell holds the cell it walks to in `mY`, and the copy keeps it. The port also set the old piece's `mY` to the slot it sits in, so a walker was moved back into its first cell: its walk, its partner link and its blink draws (one draw per piece with blink 0) went wrong, and every later draw of the game came out in another place. This is the "animals move around bizarrely when matched" fault | dead376 |

The changes alter the bot's games, so the pins of `tools/check.sh noahsark` and the tide-0 line of the mod tests moved
(`shared:` commit 0513cf6; the old pins were the port's own earlier output, which proved nothing about the original),
and the strategy pin moved again with the walker fix (level 5 done at update 19538, score 6380; action and puzzle did
not change). The frozen baseline now exists: `build/baseline-noahsark/` holds a copy of the port as of this pass (not
committed, like the other baselines) and the noahsark suite plays the action, strategy and puzzle games with it too and
needs the same lines and screens ("baseline identical"). The four mods' tests pass.

The "Animals move around bizarrely when matched" fault: confirmed against the original by state. Watching a pair's cells
in the original (the type, and the piece's path index and offsets in the walking pair's cell) showed a walker staying in
the cell it walks to while the animal above falls past; the port moved it back into the slot it was in. The walker fix
above makes the pieces' cells, and with them every later draw of the game, the same: the strategy run's board of level 2
was another one before it, and is the original's after it. The drawing of a walking animal (pixel positions) is still for
the screen comparison of phase two.

### Oddities (not fixed; how to see each)

- **The get-ready of the second level on differs from the original's.** This is what stops the replay at level 2 (action)
  or 3 (strategy); everything below was measured on strategy level 2 (`compare_mode.sh st3 build/noahtest/og/menu_s.txt
  strategy 20000`, the scripts live under build/ and are not committed). The original's get-ready lasts 323 updates from
  the new level to play, the port's 321 (level 1 takes 322 in both). Found by the draws:
  - The two games make the same number of draws in every update of level 1 and of the first 30 updates after level 2's
    release of waiting animals starts (the 41st update after the level's start: `400 < tick - start`). Then the original makes
    one more draw in a few updates (a retry of the release's column pick, which draws again when the column has no waiting
    animal), and from about 15 updates on the difference grows (3 draws by update 4575, 12 by 4838). The board of level 3,
    drawn from the generator at its start, is then another one, so the replayed clicks no longer fit.
  - The release of the waiting animals is where it starts: in the original the first animal of the level is released 4 updates
    later than the port's (the pending flag of the first column's lowest animal comes at the 45th update after the start, the
    port's at the 41st), the animals of the column picked are released every second update, and the order in which the
    columns are first picked is another one. With the generator's values the picks of the original come from the draw one
    before the port's pick (the 81st draw of the update, after 80 blink draws; the port's is the 82nd, after 81), which
    fits seven picks watched in the original at one offset, but nothing in the export puts the pick before the last
    animal's blink draw (the release is called after the loop that makes the blink draws, in both). The equal
    draws per update (81 blink + 162 for the get-ready loop, +1 for the pick) hide this. No cause found in the time given
    (several hours; the generator's snapshots, the get-ready loop, `FUN_00406750` and `FUN_00406710` were read against
    the port's code and equal).
  - Not the clock (10 ms per update in both), not the board's update counter (the original's `mUpdateCnt` is the app's
    counter minus 230 throughout), not the grid (the states of the level agree), not the water in strategy (it agrees).
  - Seen through the water in action: the original's click that ends the get-ready sets the state without snapping the water
    to its target, so the water is 1 higher for the rest of the level and the won state lasts one update longer (an extra
    +2 of bonus score). See it with `compare_mode.sh act1 build/noahtest/og/menu.txt action 9000` (4 rows differ: update 4804
    and the end of level 2).
  - A click on the "next level" button acts one update later in the original than a click on the board (the button acts on
    the release; origrun delivers it one update after the press). Putting the replay's click at the update it was made in
    brought the draws of the release into step (the replay with that mapping makes the draws of the first 30 updates after the
    release equal) but broke the action replay of level 2 (the clicks that end the get-ready found no pair), so the script
    keeps its +1 for every click.
  Because the bot clicks by update, the levels after it (action 3 and up, stampede at level 4, a series of games) cannot be
  compared by replay until this is explained; the same fix may be what the user's reported faults in later levels need.
- The original's registered state and its high-score file are untouched by these runs: each run starts from a copy.

### Deviations left

- The port's win-state water goes down at the same pace, but the original's level-end check one update later (above).
- The comparison watches 4 values; pixel positions, the fish and bubbles, the sparkles and the texts are not compared
  (phase two compares screens).
- Stampede, the later levels of medium and hard, and a series of games remain to compare once the get-ready difference is understood.

## Phase two: looks identical (2026-11-04)

### How it was compared

Screens: `tools/tour.py` turns a list of "update, command" lines into a script for the port (`screenshot` at the update) and one for
the original (origrun in Windows Sandbox on the virtual clock, `at N shot`), and `tools/phase2.sh` runs both and then `tools/shotdiff`.
Bot games (`phase2_bot.sh`) replay the port bot's clicks in the original, so whole games to their game-over, level-up and
high-score screens are compared. The original's capture at counter N shows the state the port shows at update N; the clicks go
in at N+1. Tours covered the title, intro, difficulty and tutorial dialogs, options, quit, the three modes' boards (get-ready, play,
swaps, matches, pairs walking to the ark, popups, sparkles, obstacles), the level-up panel, the game-over screen, name entry and
the high-score screen. Every flagged box was looked at in its crop and fixed or explained.

Sound: the original on the host under rundesk with `--sound wav` (its `logs/sound.log`), the port with `BYOPOP_SOUNDLOG`, for one
puzzle bot game (8800 updates): `tools/soundcmp.py` pairs the plays by sample and order. All 382 plays agree, the original's
update (ms / 10) 1 to 2 later than the port's. The music (ogg streamed by the game) was compared by its start and end only: the
stream starts at update 1190 and stops at 8603, as in the port's log. No run said SOUND LEAK.

Focus: `focus lose` at update 1720 in an action game and `focus regain` at 1900/1901; shots during the pause (the paused board) and
after match, apart from the one-update offset between the two clocks' regain (a shot two updates after it shows the original still
paused when the regain is sent one update later, and the matching frame when it is not).

Web build: the web build under Node (`wasmrun.js`) played the puzzle bot game to its game-over and the name entry: all 10 shots equal
the native port's.

### Masks (shotdiff.json, each with its reason there)

After the first level the bot's boards and Noah's sprite come from the random generator, which a bot run does not resynchronise;
the board and sprite are masked in those shots (`b_done3`, `t2` to `t5`), everything else is compared.

### Changes

| Change | Cause in the original | Commit |
|---|---|---|
| 2003-style image boxes and buttons; quit dialog from the menu; title logo order and outline; sidebar behind modal dialogs; tutorial X | the original's own drawing of dialogs (DrawImageBox of 2003) | 3fada6f |
| High-score dialog button label; block and crate pictures chosen by the original's groups | picture indices by group | 59cf27c |
| Game-over screen: capital ranks, line positions, additive bubbles | the original's draw | b3eecb9 |
| Level-up panel: title shadow loop, rank lines, speed bonus number, bubbles | draw at 0x416... | aef2158 |
| Walking animals from row 0 of the picture; ark-bound animals stretched with the framework's fast stretch (a temporary software image); popup text "+%d" only in puzzle and stampede and the number is points times the combo; sparkle life, frame and draw order | the original's draw and AddFloatingText / AddSparkle | 7a198a5 |
| High-score screen: opaque ground and fish; the bot types the name key by key | widget draw | c14d84b |
| Presses on the title, level-up and high-score links and on the difficulty, options and tutorial dialogs click through the app | the original's listener slot 1 (0x420550, 0x411290, 0x41d3e0) calls the app's button press | 4c6e418 |

The frozen baseline was made again from this build (screens changed, the logic lines did not: action level 4 done 17369 score 6524,
strategy level 5 done 19538 score 6380, puzzle game 1 over 6750 score 1450).

### Added after the coordinator's review (2026-11-04): the screens you get by losing, Stampede through a saved game

**The flood.** An action game left alone (no input) floods to the end: `lose.tour` shoots it from the first level's play to the
ark sailing off, the sinking board and the game-over panel (original in the sandbox, port with -drawevery). Two faults found and
fixed: the foreground waves of the flood's danger colour take the pulse itself (`mPulseA + shift`, the back wave alone takes 0x48 off:
0x4092a0), and the sway of the sinking pieces is `sin(y * 0.5 + tick * 0.0667) * (64 - alpha / 4)`, so it grows as they fade
(0x409d1e: the port had a constant 20). The sea, ark and Noah match from then on. One difference stays, under Oddities.

**Stampede, through a saved game.** The bot cannot reach the bonus round with the original (the get-ready divergence after level 3), but
the port writes the original's savegame.dat: `tools/phase2_save.sh` has the port's bot play to the Stampede round, a script quits to the
main menu (the save), and both exes resume that one file (the menu's Continue, then the round's bot clicks replayed in the original).
The saved game carries the generator state, so both play the same boards. It found two real faults, both fixed, and the pins moved
(`tools/check.sh noahsark` action line 6524 to 6074, the mod test's tide 0 line 6871 to 6421):

| Change | Cause in the original | Commit |
|---|---|---|
| A Stampede click took the clicked animal into the group twice (group of 8 scored as 9) | 0x40e340 takes the group the move highlighted as it is; the port's port(mod) push of the clicked piece added a duplicate | ed3d7f4 |
| The saved game numbers the two obstacle kinds the other way round from the port's internal numbers (a strategy save from the original loaded with block and crate pictures exchanged, and the port's differed from it in 3 bytes) | the files are written and read with the 0xf and 0x10 exchanged now; compared with the original's file of the same strategy game: equal but for the level start tick and two bytes of two animals' animation fields | ed3d7f4 |
| The bot logs its Stampede clicks (to replay them) | test hook only | ed3d7f4 |

The Stampede round from the save then matches the original: the same boards, scores (3,000 updates in, equal), the round's end;
differences left are the animals' walk to the ark one update off (the replayed click reaches the original one update after the
port's) and the next level's board (the random generator is not resynchronised after the round).

**The strategy game-over panel.** A strategy game of the bot's loses on hard at level 9 after 25,000 updates (the original cannot be
replayed that far: the bot's play diverges after level 3). A saved game of that level resumed in both exes stands on a board with no
move left in both (frozen alike at update 8000, the same screens but for the crate pictures the fix above corrected); the original only
looks for "no moves" after a move, as the port does, so no loss follows. The panel itself is the one shown for the other modes
(compared there). Not toured in strategy.

### Oddities (not fixed)

- **Ark-bound animals' path and size.** In puzzle and strategy an animal walking to the ark is 1 to 3 pixels off in y after about 30
  updates and about 4 px off in size after its column falls; x is identical and the formulas equal the disassembly's. See it with
  `phase2_bot.sh pz4 ... "7790:u90 7795:u95 7798:u98 7800:u00 7802:u02 7805:u05"` (puzzle) and strategy shots b10/b20. Not found after the hour.
- **The sinking board of a flood.** When the ark sails off with animals left, the original shows the board 27 px to the left and about
  1 px lower at the update after the port's (the port: +14 px down, no sideways move). The sway block of 0x409d1e (x plus the sway,
  y plus 14) cannot give that with the pieces' alpha of 250; the pieces' fields could not be watched in the original (the watch
  chain gave nothing). The sinking then goes on at the same 1 px per update. See it with `tools/phase2.sh` on a tour that waits
  8,880 to 9,050 updates in an action game on easy with no input (`lose.tour` in the notes).
- Rotated sprites are about 1 px off (the framework's rotation), as in the other games.
- The crossed numbering of block and crate pictures of the original is kept.

### Deviations left

- Levels past the first of bot runs could not be compared (the Stampede is compared through a saved game, above): past update 10300 of an
  action game the bot's play diverges from the original even with the generator resync. Its screens are compared only by the
  shared draw code with the other modes.
- The registration dialog was not toured; it is gone now (phase three). The strategy game-over panel was not reached (above).
- Music content was compared by start and stop only.

## Phase three: real input, the fixes mod and the registration code (2026-11-08)

### Real play: what was played and how

The test commands (`GAME_NOTES.md`, section 5) let a driver ask the port's own bot for its next move and send it as real input. Nothing
here goes through the script path: natively `tools/realplay/drive.py` starts the exe under rundesk with `-commands` and `-realinput`, so
each move reaches `HandleEvent`, the window-to-game mapping and the widget manager as SDL events (`move game X Y`, then `click game X Y`,
and `type B` for the name); on the web `tools/realplay/web.js`, a driver for the player-only page, sends DOM mouse and key events to the
canvas through `byopop.test.playMove`. The driver reads the game's `state` to know where the game is and when it is over (the high-score
list), and it never chooses a move itself: the bot's move for the screen it is on comes back from `move`. The title's link, the menu's
mode button, the saved-game question, the difficulty, the tutorial or mode dialog, the level-up panel, the name entry (the keys B, O, T
and the Yes button) and the list's link back to the menu are all clicked this way. Nobody played.

The bot wins every level of an easy action or strategy game, so a whole game of those two would not end by itself (an action game
reached level 15 with three Stampede rounds in twelve minutes, and a strategy game level 19, with no error, and were stopped by hand). The
driver therefore plays up to a level (6 in action, 5 in strategy) and sends the one test command `lose` there, which brings the water to
the ark; the game-over screen, the name entry, the list and the way back to the menu are played by real input like everything else.
A puzzle game ends by itself.

| Where | What was played | Result |
|---|---|---|
| Native, real time, rundesk, plain game | a whole puzzle game: title, menu, difficulty, mode dialog, the game (82 swaps), game over, name, list, menu link (618 moves) | played to the end in 162 s (16,000 updates), no error |
| Native, real time | a whole action game: the tutorial, levels 1 to 6 with the Stampede round at 4 (its own dialog), `lose` at level 6, game over, name, list, menu link (1075 moves) | played to the end in 269 s (26,700 updates), no error |
| Native, real time | a whole strategy game: the tutorial, levels 1 to 5, `lose` at level 5, game over, name, list, menu link (881 moves) | played to the end in 200 s (19,600 updates), no error |
| Native, real time | action to level 15 (Stampede rounds at 4, 8 and 12) and strategy to level 19, to see whether a long game keeps working | 12 minutes each, no error; stopped by hand |
| Native, real time, the fixes mod on (`-modsdir`) | an action game to level 5 with the Stampede round, `lose`, the name, the list (896 moves) | played to the end in 223 s, no error; the name was asked 55 updates after the game-over screen came up, against 196 without the mod |
| Native, real time, windowed and full screen | a puzzle game for 215 moves with `Alt+Enter` into full screen and out again, the focus lost for 200 updates and regained, and the focus lost and regained in full screen | every step checked in the game's `state`: `focus` false then true, `windowed` false in full screen and true again; a screenshot at each shows the board drawn |
| Web, real time, the fixes mod on | a whole puzzle game from the title's link to the list and the menu (608 moves) | played to the end in 98 s (9,800 updates), no error |
| Web, real time | a puzzle game with the focus lost (the page's blur, the document hidden) for 200 updates and regained (1594 moves) | played to the end (25,200 updates), no error; the game had its focus back at once |
| Web, `?fast=1`, the game held between the driver's calls, on the final build | a whole action game to level 6 with the Stampede round (1885 moves), a strategy game to level 5 (1479 moves), a puzzle game (1063 moves), each with the focus lost for 200 updates (the document hidden too) and regained, `state` telling `focus` true at once | each played to the high-score list and the menu link, no error (4, 180 and 12 s; the strategy game ran on a busy machine) |
| Web, real time | 80 cycles of focus lost (half of them with the document hidden) for 30 to 90 updates and regained, between bot moves of one puzzle game | the game had its focus back after each, no second call needed |

What the runs cannot cover, and why. The bot wins every level it plays, so no run met a loss by the flood on its own; the
loss, the name and the list are covered by the `lose` command and the phase-two scenarios, and the original's own end-of-game scenes
are in phase two. The browser's Fullscreen API needs a call from a hand's gesture, so the full screen was played natively only; the
page's own Fullscreen check box was not tried.

Two things worth knowing about the runs. A native run in `-fast` is not usable for play: the game's water runs on while the driver
asks for the state, and a first action game ended in 18 moves, so the native games were played in real time and the web games
with the game held between the driver's calls (`stepped=1`, built on `byopop.test.pause` and `step`) when they ran on the virtual clock. And the
level-up panel does not come after every level: in `Board::Update` it comes when the board is cleared or the bonus round ends, and
a level can end in the ark's departure without it, as in the screens of the runs.

### Real play found nothing to fix in the port

No run found a control that stopped working, a soft lock or a crash. What failed on the way was the test harness's: the three loops of
the Options dialog that I had left at five buttons after taking two out (a crash in the high-score test, found by the suite and fixed
in the same pass: the loops now run over the three buttons that are left), and image ids written as numbers (see "Registration").
One thing was seen once and could not be explained. In the first real-time web run with the document hidden for a while (a puzzle
game, `blurAt=100`), the game showed PAUSED with the board frozen in the settle phase and the update counter running, and the bot
waited until the driver gave up after 20,000 updates without a change; a second `byopop.test.focus()` from the page brought the game
back at once. It did not come again: the same real-time run once more, a run with the game held between calls, a blur in the settle
phase and 80 cycles of focus lost and regained between bot moves (half of them with the document hidden) all had the focus back after
the first call, and the native
runs' `expect focus true` checks all held. The machine was busy when it happened (a native play-through and a build were running). The
driver now reads `state.focus` after each regain and says so in its result if it had to call twice. It is listed here, and left to the
owner of the shared test kit and of the web's focus handling to look at, since the port's own focus code is the framework's.

### The fixes mod

`mods/fixes` holds three fixes, each a toggle, all on in the site's build and off in a plain native run (`GAME_NOTES.md`, sections 6 and
7). `steadyClock` counts the animals' entrance (400 ms after a level starts) and the level-up panel's seconds by the game's updates
instead of the machine's clock; `snapWater` puts the water on its level when the first click ends a level's get-ready (the original
leaves it a pixel above it for the rest of the level, 417 against 416 on level 2); `quickScores` runs the game-over screen's fade three
times faster, so the name is asked at update 59 of it instead of 175. Checked as follows: `tools/run_mod_tests.sh` (part of
`check.sh noahsark`) plays the puzzle and the action bot's games with the mod on and off and compares the lines of the game, the
level and game lines are the same; the water is one pixel nearer its level after a click with `snapWater` (414 against 415) and the same
without a click; the game-over screen asks for the name 175 updates earlier with `quickScores` (at the sample of update 1525 against
1700, in samples taken every 25 updates). The suite's baseline (the unmodded game) still gives the same logs and photographs, and the
web runs above ran with the mod on (the page's status line says "1 mod active").

### Registration and the buttons that lead out of the game

Taken out with this pass (`GAME_NOTES.md`, section 9 lists them): the Options dialog's Register and Web (popcap.com) buttons, the
registration message, the registered flag and the play counter, the register picture and font and their entries, four texts, and the
dialog ids nothing uses. The only visible change is the Options dialog: Help and High Scores share the first row, Credits takes the
second and the dialog is a row shorter. The suite has no photograph of it, so every log and every photograph is the baseline's.

Taking the register picture out moved every later image id down by one, which showed two places that named an image by its number:
Noah's idle sprites (`0x3a`, `0x3c`, `0x3e`, `0x40`, `0x43`, `0x45`, `0x47` in the idle-look code of `Board::Update`, and the level
reset) and the sprite number a saved game holds. The first check after the change failed on five strategy photographs where Noah
was one picture off; the code names the pictures now and a saved game keeps the original's numbering (its file is read and written
with the number the original uses).

### The trunk's baseline

After the trunk's libjpeg change the frozen screens were refrozen there; this branch's baseline was copied from the trunk's and the
suite passes against it (the logs of the three bot games are unchanged and every photograph is the trunk's).

### Changes of this phase

| Difference | Cause in the original | What the port does now | Commit |
|---|---|---|---|
| Options dialog: Register and Web buttons | registration code and a button that opens popcap.com | gone; Help and High Scores share a row, Credits takes the next, the dialog is a row shorter | b79c205f |
| Registered flag, play counter, register dialog, picture, font, four texts | registration code | gone; the image ids after the register picture move down by one | b79c205f |
| The title's "Thanks for registering" line (black outline, yellow face, y 0x170) and the title's tip counter, with nine registration texts the table held unused | what a registered copy shows (the user, 2026-11-08: part of the registration code) | gone; nothing else sat beside it, so there is no gap to close; the title's frozen screens lose the line | (this commit) |
| Noah's idle pictures one off after that move (found by the suite's strategy photographs) | the original names them by number in `Board::Update` and the level reset | named by the enum; the saved game still writes the number the original uses | b79c205f |
| Water a pixel above its level after a click ends the get-ready | the original snaps it only when the get-ready ends by itself | mod `snapWater` | b79c205f |
| Start of a level and level-up seconds by the machine's clock | `GetTickCount` | mod `steadyClock` counts updates | b79c205f |
| Game-over screen waits 175 updates before it asks the name | the fade's step of 1 | mod `quickScores` (a step of 3) | b79c205f |
| The standard test commands, the bot behind `move`, the drivers | | `testhooks/TestCommands.cpp`, `Bot.cpp`, `tools/realplay/drive.py` and `web.js`; `state` also tells the focus, the window mode and, on a board, `paused` | 8936856e, 0e6ae0d2, cf5896c9 |

The original's and the port's load time, memory and processor time over a scripted minute were measured back to back (`GAME_NOTES.md`, section 10): the
play link shows 0.8 s after the port's start against 2.1 s after the original's (medians of five runs), and over the minute the port used about
a quarter of the original's processor time and 12 MB less working set.

## Found later, by the readability pass (2026-11-11)

One difference from the original came to light when every function was given its address (`GAME_NOTES.md`, section 13): pressing
Enter in the high-score name box did nothing in the port. The original's handler (0x41de20) clicks the dialog's OK button through
the app, so Enter records the name; the port's was an empty function, and no run of the three phases, nor of the coverage check,
had pressed Enter there (the bot and the replays click OK). The port now does the same as the original. It was checked with a
script in the port (a score set, the game lost, a name typed, Enter: the high-score list follows, with the name in `hiscores.dat`);
the original's side was read in the disassembly (the edit widget's key functions and the dialog's handler; `GAME_NOTES.md`, section 13) and not run: scripted keys did not reach the original in the runs that tried. The same script, quitting at the name entry
without pressing Enter, also showed a crash of the port at exit, which is fixed (the app's destructor takes its screens out of the
widget manager before deleting them).
