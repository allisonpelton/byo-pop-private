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
- The registration dialog was not toured (registration code comes out in a later pass); the strategy game-over panel was not reached (above).
- Music content was compared by start and stop only.
