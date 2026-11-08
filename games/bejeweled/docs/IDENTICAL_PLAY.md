# Bejeweled Deluxe: the identical-play pass

This is the report on comparing the port with the original exe, screen by screen and sound by sound, on 2026-11-01. The
original is the game carved out of the Steam `WinBej.exe` (`tools/extract_game_exe.py`), run under `tools/origrun` inside
Windows Sandbox from a copy in `build/origrun/`.

The pass has the two phases CLAUDE.md names. In the first, **plays identical**, a series of games driven by the same input
played out the same on both sides by the game's state: the same boards, moves, scores, levels and timers, through
level-ups up to level 4 and both kinds of ending. In the second, **looks identical**, every screen, dialog and text it
reached matched the original pixel for pixel (shotdiff's tolerance of 8 a channel), every sound and every music event
matched by order with its volume, pan and pitch, losing and regaining the focus matched, and the web build matched the
original as the native build does.

The pass found no fault in the port. It changed no game code, so the oracle's baselines were not refrozen. What it added
are the comparison's tools (`games/bejeweled/tools/origcmp/`) and this report; the Mac build's names are compared in
`MAC_COMPARISON.md`.

A second part, on 2026-11-08, added what CLAUDE.md asks of a finished game beyond the comparison: the standard test
commands, a play-through with real mouse and key events only ("Real input", below), and the fixes mod ("The fixes mod").
It changed no play of the unmodded game (its code took the registration and "more games" screens out, which the first part
did not do); "Changes the pass made" lists what it did change.

## How both sides are driven

**Driving both sides alike.** The original's update counter, draw flags and input gate are given to origrun in
`tools/origrun/bejeweled.cfg` (git-ignored; the app object is the static at 0x53b570, its update count at 0x53b9f4). The
gate lets input through only in `UpdateAppStep`'s message pump (0x44c900) and around the frame's sleep and draw in
`Process` (0x44c340), so an input posted "at counter K" is read before update K+1, which is the port's script line
`K <command>`: inputs land on the same update on both sides, exactly. `orig.sh` runs the original on an origrun script,
`toport.py` turns that run's log (the counter each input landed at) and `shots/landed.tsv` (the update each shot shows)
into the port's `-script`, and `port.sh` runs the port on it under rundesk. For a long game the direction is reversed:
`fromport.py` turns a port bot's game (`-autoplay`, logged) into an origrun script with slack for each input, and the
original's run then gives the port's script as before.

**The same boards.** The original's `SRand(GetTickCount())` at startup is patched to a constant (0x44e9a1, origrun's
`--seed`, the patch list in the git-ignored `tools/origrun/patches/bejeweled.txt`); the port's `-seed=N` is the same
`SRand(N)`, so both deal the same boards and the same refills for the whole game.

**Two settings for the test, on both sides.** Both run windowed: the first run's `partner.xml` asks for full screen, which
the sandbox cannot open, so the original would fall back to a window while its options still said full screen
(`windowed.reg`, and the port's `registry.ini` in `port.sh`). The original's hover check in its timer handler
(`ProcessDeferredMessages`, 0x44ae8e) reads the cursor with `GetCursorPos`, which reads 0,0 inside the sandbox and makes it
send a mouse-leave every tick; a test-only patch skips that check, so the buttons' and gems' hover states can be compared.
Neither is a difference of the game.

## Phase 1: plays identical

**By the game's state.** The original's runs watch four values of its board through origrun's `--watch` (the app's
`mBoard` pointer at 0x53bcf8, then the score at +0x1ec, the 0-based level at +0x8c, the state at +0x194 and the bar's
width at +0x1d8), each logged with the update it changed at. The port runs with `-trace`, which prints the level, the
score and the whole board each time the board settles after a move, with the moves it then has. `statecmp.py` checks
every settle of the port against the original's score and level after the same update, and prints the original's states
and levels in order, so the game's course (level-ups with their whirlpool, state 7, and the level transition, state 8)
can be read beside it. The boards themselves are compared in the shots taken every 50 updates (below), and the timed
mode's timer is the bar in those shots.

**Long games.** The port's bot plays like a person (misclicks, wrong swaps, hints), and in the original's pace a game of it
reaches no level-up. `greedy.py` makes long games instead: the port plays the first move its trace lists for each settled
board, 60 and 100 updates after the settle (the next listed move when the first board's instructions refuse one), and
writes the input as an origrun script for the original, from whose run the port's script then comes as for every run.

| game | input | settles | result |
|---|---|---|---|
| gr1 | normal, seed 3, greedy to level 3 (68 moves, 16 350 updates) | 69 | every score and level the same; the level-ups at updates 5162 and 15009 on both sides; 311 of 311 shots match |
| gr2 | normal, seed 5, greedy for 260 moves (level 6 in the port); the original's run stops at origrun's cap of 240 s, at update 23 950 | 111 | the same to there; level-ups at 6108 and 14294; 463 of 463 shots match |
| gt1 | timed, seed 4, greedy to level 4 (90 moves, 20 450 updates) | 89 | the same; level-ups at 5331, 8766 and 19154; 393 of 393 shots match, the timer's bar in each |
| p-g1 | normal, seed 1, the port's bot (27 moves) | 27 | the same; 521 of 531 shots match, 10 faint (below) |
| p-t1 | timed, seed 2, the port's bot until the time is up | 18 | the same; Time Up, the record dialog and Play Again in the shots, 283 of 284, 1 faint |
| nm1 | normal, a saved game with one move left, to "no more moves" and the game over | | 84 of 84 shots; the ending and the name entry the same (phase 2's table) |

Over these games the two sides drew the same boards and refills, took the same moves, scored the same points, levelled
up on the same updates and ended the same way (the timed game's time running out, the normal game's last move), so the
first phase found nothing to fix.

## Phase 2: looks identical

**Shots.** `tools/shotdiff` compares each pair; no part of any picture is masked (the game has no `shotdiff.json`).
Every flagged box was looked at. A shot the original takes on the update of an input is left out (`toport.py`): it
answers the capture in the same message pump that reads the input, and shows the input's effect only when that forces a
draw. A shot origrun marks "or-earlier" may show an older frame; the port also shoots the three updates before it, and
`earlier.py` checks a pair that differs against them.

**Sound.** The original's sounds are logged by origrun's DirectSound stand-in and its music (BASS) by Bookworm Adventures'
BASS proxy (`ORIG_SOUND=on`), the port's by the framework's sound log (`BYOPOP_SOUNDLOG`). `soundcmp.py` compares every
play of a sound in order with its volume, pan and pitch, and the music's plays, stops and volume changes. Stops of sounds
are not compared (the original's DirectSound manager frees a finished duplicate when it next needs one, the port logs a
stop only where the game asks for one), nor volume changes of a song that is stopped (at quit the port's fade sets one
once more; the original has exited).

**The runs.** All with the original and the port on the same seed:

| run | what it covers | shots | sound |
|---|---|---|---|
| cal | the title, the first board with the instructions and the hint arrows | 7 of 7 match | |
| ui1 | hint and hover, Options, Help, the custom cursors on and off, High Scores, New Game answered No, a move, Timed answered No, Quit answered No, Timed answered Yes, pause and unpause by key, the focus lost and regained, Normal answered Yes | 44 of 44 | 51 plays, 575 music events, the same; the focus mute too |
| g1 | a whole normal game by the port's bot, seed 1, through the level-up and its whirlpool to level 2 (14100 updates) | 516 of 526 match, 10 faint | 141 plays, 209 music events, the same |
| t1 | a timed game, seed 2: the warning ticks, Time Up, the high-score list, the record dialog, typing a name, Enter, Play Again | 282 of 283, 1 faint | 99 plays, 228 music events, the same |
| nm1 | a saved game loaded (the "continue" question), its last move, "no more moves", the game over and the name entry | 84 of 84 | the same |
| m1 | a gem selected and deselected with the right button, a swap by dragging, a pause by key and a click to resume, the focus lost and regained, both Options sliders, New Game answered Yes and the board falling out and in, Timed and Normal, Quit answered Yes | 52 of 53, 1 faint | 45 plays, 515 music events, the same |

nm1 starts from a save written by `nomoves.py`: the port's own save with a board that has exactly one move left. The
original reads it from its game folder's `userdata` in the sandbox (`ORIG_SAVE`), the port from its user folder
(`PORT_SAVE`).

**The faint pairs** are all timing of the capture, not of the game. Eleven of the twelve show the original's frame of one
update before the shot's label, as `earlier.py` confirms. The twelfth, g1's `g007126`, is origrun's documented release
race, where the original's shot shows the update after its label: it equals the port's frame one update later, checked
with a run of the port that shot the updates around it (an earlier run of g1 had one more, `g001901`, the same way).

**The web build.** The same five port scripts were run on the web build under Node (`wasmrun.js game`, each in a fresh
folder with the same settings and save): ui1 44 of 44, g1 517 of 526 with the same faint pairs (and the same `g007126`),
t1 282 of 283, nm1 84 of 84, m1 52 of 53, all against the original's shots. The web build's pictures differ from the
native build's by at most 3 levels a channel (its software renderer's rounding of blended edges), below the tolerance and
not visible. In the Browser pane, opened with `?silent=1` (the status line ended in "silent"), the page loaded the game,
showed the title and dealt the first board with its instructions and hint arrows. Its sound was not logged: under Node
there is no audio device, and in the pane it is silenced on purpose; the web build plays the same game code, so the
native comparison stands for it.

## Real input

Autoplay, script commands and runs under Node never pass through the input path a hand uses, so the game was also played from
launch to the end of a game with real events only (CLAUDE.md, "Done means identical"). Code played and the session judged;
nobody played. Both drivers are thin loops around the standard test commands (`GAME_NOTES.md` section 15): ask `state`, ask
`move` (the port's own autoplay bot chooses a swap and answers the pointer steps that make it), send the steps as events, wait
for the next answer. The coordinates are the original's 640x480 pixels, so the same steps work in a window of any size and in
full screen.

**Native**, `tools/realplay/drive.py`, under rundesk with the `-commands` and `-realinput` channels (SDL events pushed on the
game's queue: pointer moves, presses and releases at game coordinates, keys, the Alt key, focus lost and gained). Its last
run:

| step | what it did | result |
|---|---|---|
| launch | the loading screen, the title, the play link, the first game's hint (the tutorial arrows, from board update 150), Options, Help inside it, both Done buttons | ok |
| Normal | a whole game: 31 moves, levels 1 and 2, 51 s; the hint gone at the first scoring move; the game over (the original's no-move rule), the name entry typed with key events and Enter | score 830 |
| Timed | a whole game in real time: 897 moves, levels 1 to 16, 1356 s; Time Up, the name entry | score 144 945 |
| focus | focus lost in a game: paused; focus regained: running again (ticks 958 and 1009) | ok |
| full screen | Alt+Enter to full screen, 8 moves there, Alt+Enter back, 8 moves in the window | ok |

A Timed game needs real time (`--realtime`): its clock is the board's update count, and a `-fast` run does thousands of
updates in the time the channel takes to carry one move, so the clock ran out during the first moves.

**Web**, `tools/web/onegame.js`, run by the player-only page (`?player=1&assets=1&silent=1&driver=...`) in the Browser pane with
the page's DOM events: the same route. A whole Normal game (129 moves, levels 1 to 4), whole Timed games (167 and 220 moves,
levels up to 7), the name entry typed, the "Continue old game?" question of a profile that had left a game answered No with a
click, the focus lost and regained (the board paused and ran again), and a canvas drawn at twice its size (`&scale=2`, 40
moves) so that the clicks land at scaled points as they do in full screen. No page error in any run. The fixes mod, which the
page installs on by default, was on in these runs.

What the real input did not find: no fault of the port. The faults it met were the drivers' own (the name entry, which comes a
moment after the game-over screen; the "end the current game?" question that a mode button asks in a game; the screen a
`state` names while Help lies over Options), and each is fixed in the driver or in the state command.

What stays unproven:

- A Normal game's length depends on the board (the bot has no say in what the board brings): the run above ended by the
  original's no-move rule after 31 moves, an earlier run of the same driver played 127 moves, and the web run's early game over
  (22 moves, not examined further) fits the same rule. The verifier and the oracle cover that rule against the original; the real input shows only that the
  game over, the name entry and the high scores work.
- The native table above is the run on this branch after the shared channels (`-commands`, `-realinput`, and the bare `move`
  that reaches the bot) were merged in; the first run, on a scratch merge, played 127 and 497 moves and found nothing either.
- Full screen on the web: the Browser pane cannot enter it (`requestFullscreen()` neither resolves nor rejects there), so the
  scaled canvas stands in for the scaled mouse mapping; the browser's own full screen waits for the user's Chrome.
- The windowed and full-screen runs were not compared with the original's, which the sandbox cannot run in full screen; they
  show that the game plays and that input lands.

## The fixes mod

`games/bejeweled/mods/fixes` is the game's fixes mod (`docs/MOD_ROADMAP.md`, "The fixes mod"): one mod, one toggle for each fix,
all on by default, on by default on the site, shipped beside the page as `bejeweled-fixes/`. It shares the high scores and the
saved game with the original (no rules change, so no variant board, no v2 save). The port without it is the original, and the
verifier's replays never carry a fix. Two fixes (`GAME_NOTES.md` section 16 has the code and how to see each):

| fix | the original | with the fix | commit |
|---|---|---|---|
| `selection` | When Timed mode's board falls out and refills (no move left), a gem you had selected stays selected though it is no longer on the board; your next click either loses a gem (the stale gem is put back in the old gem's place) or selects nothing | the selection is dropped at the refill and the click selects the gem you clicked | e59ace0a |
| `resumeLook` | A saved game of level 2 or later comes back with the first level's background tint and texture until the next level-up (`LoadGame` does not set the level up) | it comes back with its own level's look | e59ace0a |

`tools/fixes_test.sh` plays each fix with the mod off and on under rundesk and checks that the runs differ the way the fix says
and in nothing else (the resumed board's redness: 24.2 in the game the save came from, 12.3 loaded with the fix off, 21.2 on).
The mod's menu entry shows its description and both toggles with no warning (`1bbb10f3` fixed a help text that was cut).
Memory faults belong to the port, not the mod (`GAME_NOTES.md` section 16).

## Changes the pass made

The first part changed no game code. It added the comparison's tools in `games/bejeweled/tools/origcmp/` (`cae739f`;
`statecmp.py` and `greedy.py` for the first phase in the commit of this report), this report and `MAC_COMPARISON.md`. The
origrun configuration and the test patches are git-ignored (`tools/origrun/bejeweled.cfg`, `tools/origrun/patches/bejeweled.txt`).

The second part (branch `claude/bejeweled-finish-bf5c89`):

- The standard test commands (`state`, `goto`, `level`, `unlock`, `set`, `win`, `lose`, `seed`, `move`), in
  `src/testhooks/TestCommands.cpp`, reached only from `-script`, `-commands` and the web test channel, listed in
  `GAME_NOTES.md` section 15; `move` asks the port's own autoplay bot (`e59ace0a`, `86835386`).
- The fixes mod and the two hooks it needs in the game (`Board::DropStaleSelection`, `Board::ResumeLevelLook`, behind
  `GameFeatures::mFixes`, off unless the mod is on) (`e59ace0a`).
- The real-input drivers (`tools/realplay/drive.py`, `tools/web/onegame.js`) and the `state` fields they read (`86835386`).
- Tests: `tools/commands_test.sh` and `tools/fixes_test.sh`.
- The registration code and the "Play More Great Games!" button, taken out by the user's rule of 2026-11-07
  (`GAME_NOTES.md` section 18): the Options dialog's link, Register and update buttons, the register link and nag timer
  of the title screen, the register, update-check and browser dialogs, the browser opening, the trial circle and wait bar
  images. The one visible change is the Options dialog without the link, its Help / High Scores row 12 px lower; its
  height and Done button stay, so every script still clicks where it did. The oracle accepts a difference from the frozen
  baselines in its four shots that show the dialog, inside the dialog's box only. The oracle with the removal in
  (variants default, forcevariant and novariants, 2026-11-08): 443, 443 and 293 cases, no difference; its notes are the
  Options-dialog ones and, in the default variant, one draw-count tolerance (63 against 62 draws, within the 8 wall-clock
  seconds of the run); the frozen baselines were not refrozen.

## Oddities skipped

None was found that changes play. The original's sandbox-only behaviours (the hover check's mouse-leave every tick, the
fall-back from full screen to a window) are the sandbox's, worked around as described above, not oddities of the game.

## Deviations that remain

None was found in what was compared. Loading (the user's rule of 2026-11-07, fast loading is part of every port) is not a
deviation: the registered original's loading had no wait to take out (the bar showed the thread's real progress; the
nag's delay clamp was the trial's and is taken out with it), and the port loads at the machine's speed
(`GAME_NOTES.md` section 18). What was not compared:

- Full screen: the sandbox cannot open it, so both sides ran windowed. The port alone was played in full screen natively
  (Alt+Enter, moves, back; "Real input"); the original's full screen was not compared, and the web's only as a scaled canvas.
- Registration, the nags and the update check: the Steam exe runs registered, so they never show; the port leaves them out
  (`GAME_NOTES.md`, the dialog table).
- The "Play More Great Games!" button in Options and any other link: never clicked in a test run (CLAUDE.md). It is taken out
  since (the second part), so the comparison above, made with it, no longer matches the port's Options dialog in the link's
  row and in the row of Help and High Scores; every other screen still matches. Repeating the comparison's Options shots
  (ui1, m1) would flag exactly those two places, which is the change `GAME_NOTES.md` section 18 lists.
- Levels after 3 in a normal game and after 4 in a timed one: origrun caps a run at 240 s (about 24 000 updates of
  Bejeweled), and no run of the original reached them. A saved game at a later level would reach them.
- The web build's sound, for the reason given above.

To repeat a run: `orig.sh --prepare` once, then for example
`ORIG_SOUND=on games/bejeweled/tools/origcmp/orig.sh m1 <script> 1`, `toport.py build/origrun/m1 > port.txt`,
`port.sh m1 port.txt 1`, then `tools/shotdiff/shotdiff.py --game bejeweled --orig build/origrun/m1/shots --port
build/bejcmp/m1/shots --run m1`, `earlier.py` and `soundcmp.py` (each file's header has its use).
