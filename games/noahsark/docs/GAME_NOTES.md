# Noah's Ark Deluxe: behaviour notes (work in progress)

Addresses are virtual addresses in `re/noahsark/bin/WinNoah.exe` (version 1.1, linked 2002-10-17 with VC++ 6.0,
image base 0x400000, full RTTI, 64 classes, 2,709 functions of which 260 (0x401000-0x421dc0, 131 KB) are game
code). Offsets such as `app+0x3e0` are byte offsets in the original object; the port's classes name the fields.

## 1. Binary layout

- Game code 0x401000-0x421dc0: `Board` 0x401000-0x410bf0, DifficultyDialog (0x410c00 ctor), HighScoreDialog,
  HighScoreWidget, IntroWidget, NextLevelWidget, NoahApp, OptionsDialog (ctor 0x41c230), RecordScoreDialog,
  RegisterDialog, TitleScreen, TutorialDialog. Framework from 0x421dc0 (Widget), libraries after 0x450000.
- The framework is the SexyApp of the 2002 generation: one app class `SexyApp` (54 vtable slots, derived from
  `ButtonListener` with 4 slots), `Widget` without WidgetContainer (44 slots, sizeof 0x4c), Graphics with
  non-virtual methods, std::string of MSVC 6 (copy on write, refcount in the byte before the data).
  docs/FRAMEWORK_2004.md describes the 2004 successor; FRAMEWORK_2002.md records the differences found here.
- Data: `images/` (GIF/JPG/PNG, alpha in `_name.gif`), `data/*.txt` (ImageFont descriptors), `sounds/*.ogg` +
  `levelup.au`, `music/*.ogg`, `fmod.dll` (the original plays music through FMOD; the port does not).

## 2. Resource tables (Res.h)

83 images in the original's table 0x494f40 (the port's has 82: the registration picture, id 47, is left out, section 9), 34 sounds
(0x49508c), 9 font descriptors (the port loads 8: the register font is left out), 4 music tracks (loading, mainloop, mainintro,
gameover). Fonts (app+0x3e4..0x404): fontbig, fontmedium, fontsmall, colorlessfont, garamondfontmed, garamondfontsmall,
garamondfonttiny, scorefont, registerfont.

## 3. Application (NoahApp, vtable 0x4888ec, 54 slots)

The app (`NoahApp.h/.cpp`) owns the images (82; the original 83) and fonts (8; the original 9), the board and the screens (title, intro menu, level-up
panel, high-score screen), and the game's counters: level, difficulty (0 easy, 4 medium, 8 hard), game mode, bonus round,
score and shown score, animals saved, combo count and best combo, the hint and tutorial settings. The sidebar's Options
button is at (5,390) and Quit at (5,431). Dialog buttons arrive at `ButtonDepress` with the dialog's id plus 2000 (see
FRAMEWORK_2002.md): difficulty (ids 0x3c, 0x3d, 0x3e for easy, medium, hard), options, the tutorial, the high-score
list and the score-entry dialog. Ranks: 24 thresholds up to 999,999,999 and 23 rank names, shown on the level-up panel
("Your Rank", "Next Rank at"). The music is the menu loop, then the game's intro and its loop, then the game-over piece;
`NoahApp::UpdateFrames` starts the loop when the intro ends.

## 4. Board (WinNoah.exe 0x401000-0x410bf0; object 0x1108 bytes, vtable 0x488368, ButtonListener at +0x4c)

Work notes while the translation is written (see Board.h for the final field names).

- A 9x9 grid of `Piece*` at +0x5c, index `x*9 + y` (x column, y row, y 0 = top). `FUN_0040e280(x)` is the cell's left
  pixel, `FUN_0040e2a0(y)` its top; a cell is 50x50 (0x32).
- Piece (0x68 bytes): +0 x, +4 y, +8/+c the cell it moves to, +0x10/+0x14 float pixel position, +0x18 type
  (0..12 animals, 0xe empty, 0xf block, 0x10 crate), +0x1c alpha, +0x20/+0x24 pixel offset to the cell (a new piece
  starts at y offset -0x1db and falls), +0x28 fall state, +0x2c selected, +0x2d removed (animating out), +0x2e,
  +0x2f flash, +0x30 moving along a path (+0x3c its partner, +0x34 index into its path vector +0x48), +0x38,
  +0x59 match mark (puzzle), +0x5c blink frame.
- Second grid of 81 clones at +0x1c4 (FUN_004060d0 copies a piece): the solvability simulation of the level
  generator (FUN_00401e40) works on it. A row of 9 queued pieces at +0x1a0.
- Pairing rule: two pieces of the same type can be removed when a path of at most N straight segments joins them
  through empty cells (FUN_004036f0, depth-first with a cost bound; FUN_00403370 keeps the cheapest path in +0xe08).
  FUN_004026d0 / FUN_00402a70 are plain breadth-first reachability on the real / cloned grid (4-neighbour).
- State (+0x310): 0 get ready, 1 playing, 2 won (Noah climbs aboard), 3 lost, 4 level done (the ark leaves), 5 no more
  moves, 6 pair abandoned, 7/8 puzzle swap and swap back, 9 settle (puzzle refill).
- App game mode (+0x444): 1 action, 3 strategy, 4 puzzle, 5 stampede bonus.
- A pair is removed by a path of empty cells (`Reachable`, breadth-first; `SearchPath`, depth-first with a cost bound,
  gives the trail that is drawn). The pieces walk along the trail toward each other and are taken in within 5 pixels;
  removed pieces then fly to the ark (speed 5.3). Combos lower the water. Strategy mode has gravity and pushes a new row
  in (`PushRow`); puzzle mode is match-3 (`MarkRuns`); the stampede bonus removes groups of three or
  more while the columns close in on the middle.
- Drawing Noah (`Board::Draw`): two flags choose his picture, the board's `mNoahWorried` (+0x35c) and `mArkLeaving`
  (+0xe15). Normal play uses his idle sprites (first image 62 plus a frame); once worried, his climbing frames
  (images 48 to 57); after a win or while the ark leaves, only the last climbing frame, and only until the ark's ramp stage
  moves on.

## 5. The standard test commands (`src/testhooks/TestCommands.cpp`, `Bot.cpp`)

The commands of `docs/TEST_COMMANDS.md`, reached only from a test (`-script`, `-commands`, the web test page's `command`). A command
that is never given changes nothing and draws no random number. Each answers `STATE {...}`, `MOVE {...}`, `OK <command>` or
`ERROR <command>: <reason>`.

| Command | Does |
|---|---|
| `state` | one JSON line: `update`, `screen` (loading, title, menu, newgame, difficulty, tutorial, modeintro, game, nextlevel, entername, highscores, gameover, quit, options, scores, dialog), `mode` (action, strategy, puzzle, stampede), `difficulty`, `level`, `score`, `focus` (the window has the focus: without it the board stops and draws PAUSED), `windowed` (false in full screen), `over`; with a board `phase` (getready, playing, won, lost, leveldone, nomoves, abandon, swap, swapback, settle), `busy` (something moves or is being removed), `water` and `waterTarget` (the water's height and where it is going), `ark`, `animals` (left on the board), `paused` (a dialog has paused the board) and `board` (9 strings, one per row: a digit or letter for an animal, `.` empty, `X` block, `Y` crate) |
| `goto menu\|title\|options\|highscores` | the screen, by the game's own link or dialog function |
| `goto <mode> [easy\|medium\|hard]` | a game of that mode from the menu: the menu's own button, the answer to the saved-game question, the difficulty dialog's button |
| `level <n>` | level n of the game in progress, as the level-up panel's link starts a level |
| `unlock all` | nothing in this game is locked (answers `OK`) |
| `set score <n>`, `set level <n>` | the score and the level counter (for the next level) |
| `set mode <mode>`, `set difficulty <d>` | the game the bot starts from the menu; `move` has no arguments here, since under `-realinput` `move <x> <y>` is the framework's pointer move |
| `seed <n>` | the C library generator (every draw of the game) and the bot's own |
| `win`, `lose` | ends the round of an action or strategy game that is being played: every animal off the board (`win`), or the water at the ark (`lose`) |
| `move` | the next input toward playing a game of the mode set, from the bot of `Bot.cpp` (the one `-autoplay` clicks): `MOVE {"kind", "input": [...], "updates": n}`, `input` in the order of `docs/TEST_COMMANDS.md` (`{"move":[x,y]}`, `{"click":[x,y]}`, `{"key":"B"}`), game coordinates; `"kind":"wait"` (with an empty `input`) when the game needs time, `MOVE null` when there is nothing to do |

`move` answers each screen: the title's play link, the menu button of the mode set (`MOVE null` if none is set), the saved-game
question (Yes: a new game), the difficulty button, the tutorial and the how-to-play dialogs' Yes button, the level-up panel's link, the high-score
name entry (the keys B, O, T and the Yes button) and the list's menu link. On the board it waits while a round is getting ready or
anything moves, and then clicks the bot's move: a pair of animals that can reach each other (action, strategy), a swap that makes a
run of three (puzzle), or a group of three or more in the stampede round. The bot is the autoplay's, with its linear generator and
its seed (`seed <n>`), so a `-seed` run plays the same game by `move` as by `-autoplay`. `over` is true on the high-score list after a
game.

## 6. Fix candidates

What the original does that the fixes mod (`mods/fixes`, below) mends, and what it leaves alone.

- **Made: the clock of the level's start and the level-up panel.** The original lets the animals in 400 ms of the machine's clock
  after a level starts (`Board::Update`, the release of the waiting animals) and counts the seconds on the level-up panel
  (`NextLevelWidget`) by `GetTickCount`, so a slow moment or a pause moves the animals' entrance and costs seconds, and two
  runs of the same seed part by a frame or two. `steadyClock` counts the
  game's own updates instead (40 updates, 100 to the second), which is the same time on a machine that keeps up.
- **Made: the water after an early click.** The first click of a level ends its get-ready. The water rises by steps of 3 to its level
  and ends a pixel above it (417 for 416 on level 2); the get-ready's own end, when no click comes, snaps it to the level, but a click
  leaves it. `snapWater` snaps it at the click too (strategy has no rising water and is left alone).
- **Made: the game-over screen.** The high-score screen fades in over 255 updates (one step of 1 an update) and checks the rank, and
  so asks for the name, when the fade reaches 175: a wait of 1.75 s on a game that is over. `quickScores` runs the fade three times
  faster (a step of 3).

## 7. The fixes mod (`mods/fixes`)

`mod.json` with the feature `fixes` and one bool setting for each fix (`steadyClock`, `snapWater`, `quickScores`), all on by default.
The game reads them where it needs them (`Platform::FixOn`); without the mod every fix is off and the game is the
original. The site ships the mod beside the page and starts the game with it on (`cmake/ByopopWeb.cmake`); natively it is a folder
in `mods-noahsark`, or in a `-modsdir`. It does not split the high scores (a feature with no rules or tunables). Checked by
`tools/run_mod_tests.sh`: the bot's games with the mod on give the lines of the unmodded game; the water is one pixel nearer its level
after a click with `snapWater` and not without it; the game-over screen asks for the name at update 1525 with `quickScores` and at
1700 without it.

## 8. The drivers that play by `move`

`tools/realplay/drive.py` plays a game natively: it starts the exe under rundesk with `-commands` and `-realinput`, asks `move` for
each step, and sends it as real SDL events (`move game X Y`, `click game X Y`, `type B`); it reads `state` to know where the game is
and when it is over (`--mode action|strategy|puzzle`, `--diff`, `--fast`, `--steps`, `--at N:<lines>` for focus and full screen in
the middle of a game, `--finish`). On the web the test page's `byopop.test.playByMove` plays by the same `move` as DOM events
(`?player=1&silent=1&driver=move`). The game runs in real time for the native runs: with `-fast` it runs ahead of the driver, and the
water, which rises with the game's updates, ended a first action game after 18 moves.

## 9. Registration and the buttons that lead out of the game (taken out 2026-11-08)

The port was the registered game already (the shareware branches were never translated); what remained of the registration, and the
buttons that open a page outside the game, is gone, and each removal leaves the baseline's logs and screenshots alone except where it
is said.

- The Options dialog's Register button (id 1, the registration message) and Web button (id 3, which opened popcap.com in the
  browser). The Help and High Scores buttons now share the first row and Credits takes the second; the dialog is a row shorter (it
  was 438 high, it is 396). This is the one visible change; the suite has no Options photograph.
- `ShowRegisteredDialog`, the registered flag and the play counter (`mRegistered`, `mTimesPlayed`), the register dialog's id and the
  other dialog ids nothing uses, the register picture (`IMAGE_REGISTER`, so every later image id moves down by one: Noah's sprites are
  named by the enum now, and a saved game keeps the original's numbering) and the register font with its file entry, and four texts
  of the table (the register, the order and the web page messages).
- What a registered copy shows (the user, 2026-11-08: "Thanks for registering" is part of the registration code): the title's line in the
  middle, drawn in black with a yellow face at y 0x170, and with it the title's tip counter (`mTip`, which only picked among the
  shareware copy's lines) and its redraw every 300 updates; and the registration texts the table still held without a use: the invalid
  registration message, the puzzle mode's register question, "Click Here to Register Now!", the thanks, and the plays-remaining and
  register-to-unlock lines of the shareware title (nine entries of `GameTextTable.inc`). Nothing else in the table, the images or the
  data shows a registered copy: there is no stamp, no "registered to" line and no status line (the original's other registered
  branches are the puzzle game's limit of 6000 updates and the high-score dialog's offer to register, neither of them ported). The title's frozen screens change here: the line is gone from the middle of the title.
- The drawn "popcap.com" under the title (`IMAGE_POPCAPURL`) is a picture, not a button, and stays.

## 10. Performance (measured 2026-11-08, original and port back to back)

`tools/profile/perf.py` takes the numbers (its docstring has the commands; adapted from Mummy Maze's). It samples the game's process
from outside every 0.1 s (working set, private bytes, kernel plus user time) and takes the load time from the process's creation to the
title screen's play link: the port answers `move` with the kind "title", and for the original the script reads the title's
link-shown byte out of the copy's memory. The original ran under origrun on its real clock, from a fresh copy for each run, with the
copy's git-ignored patch list applied: `srand` stores the run's seed (so it starts its random numbers where the port does) and the copy
counts as registered like the port (an unregistered copy showed no play link in 40 s of watching it, so a first try measured nothing).
The port ran natively under rundesk without `-fast`, the
same way. No speed-ups were made to this port, so these are the port as merged. The scripted minute is the last 6000 updates of an
action game played by the port's bot (`-autoplay`, seed 1, easy, 7500 updates in all) and replayed in the original as clicks by
`tools/origrun_replay.py`; the port marks the minute's first update with a screenshot's file time and the original's is found from
its update counter.

**The machine was not quiet.** The runs share the machine with other sessions: 3 to 9 game processes of other sessions were running
when a run started or ended (counted by `tools/gamecount.py`; the table says how many each time), and the whole machine was 67 to
100 % busy, so every figure is slower and noisier than on a quiet machine. The performance pass measures again later.

| First screen that takes input (the title's play link) | Original under origrun | Port native |
|---|---|---|
| Seconds from the process's creation, five runs | 2.42, 2.13, 2.09, 2.12, 2.05 | 0.78, 0.82, 0.92, 0.74, 0.80 |
| Median | 2.12 s | 0.80 s |
| Working set / private bytes at that point | 63.8 to 64.0 MB / 50.6 to 51.0 MB | 56.0 to 56.4 MB / 55.8 to 56.0 MB |
| Processor time until then | 1.27 to 1.52 s | 0.62 to 0.86 s |
| Other sessions' game processes, start / end | 5 to 7 / 6 to 8 | 5 to 6 / 5 to 7 |
| Machine busy | 67 to 72 % | 70 to 85 % |

The original's figure leaves out the 0.9 s origrun takes to copy and patch the folder and start the process. The web build's
load was not measured: it depends on the host that serves it and on the browser, and the Browser pane is hidden and runs no
animation frames (the title's link appears in a game update, which the web build runs from `requestAnimationFrame`), so a timing from
it would measure the shim.

| The scripted minute (6000 updates) | Original, run B | Original, run C | Port, run A | Port, run B |
|---|---|---|---|---|
| Updates per second | 99.8 | 95.6 | 102.5 | 105.1 |
| Peak working set | 80.6 MB | 80.6 MB | 68.0 MB | 67.8 MB |
| Peak private bytes | 67.4 MB | 67.1 MB | 67.7 MB | 67.9 MB |
| Processor time (kernel plus user) | 18.97 s | 18.12 s | 5.00 s | 4.94 s |
| Other sessions' game processes, start / end | 5 / 7 | 4 / 7 | 4 / 6 | 4 / 5 |
| Machine busy | 93 % | 95 % | 88 % | 88 % |

Two more runs of the original (A and D, left out of the table) did not play the game: the machine was 93 to 100 % busy and the
original took 11.5 and 16.0 s to show its link, so the script's click at update 301 came before the title was ready and the
rest of the script found no game to click on (a working set that stays at 63 MB and 1.1 s of processor time over the minute); they
were kept as files and not used. The ones in the table kept to the script (5993 to 5995 of 6000 updates, the counter reaching
7500). Over the minute the port used about a quarter of the original's processor time (5.0 and 4.9 s against 18 to 19 s) and 12 MB
less working set, with the same private bytes.

`tools/profile/perf_all.sh` runs the whole pass (the bot's click log, the original's script, the load runs and the minutes) in
`build/nt/perf`. The numbers above are from one pass of 2026-11-08: the loads and the minutes A and B of the first run, with the
port's minutes repeated after a path error in `perf.py` (the first try of them never started) and two more minutes of the
original (C and D) after A did not play. The saved script is that pass put in one file; its pieces were run separately.

<!-- private -->
The patch list used for the original's copy (`tools/origrun/patches/noahsark.txt`, git-ignored) has three entries: the call to
`time` before `srand` at file offset 0x2f40b becomes `mov eax, seed` (the same pin the identical-play passes used); the registration
check, the virtual at 0x42b3d0 (`vf16`, `ret 8`), starts with `mov al, 1` and returns; and the access mask of the `RegOpenKeyEx` in
the registry reader at 0x42d229 is `KEY_READ` instead of read and write: with only the first two, the registered flag at app+0x328
stayed 0 and the title showed no link, and it became 1 once the open asked for read access only (the open for read and write
presumably failed without elevation; not checked). With the three, the title shows its link at update 104.
<!-- /private -->

### The performance pass (2026-11-09)

**What the profile showed.** `tools/profile/prof.sh` runs Bejeweled's sampling profiler (`cpuprof.py`: every thread's instruction
pointer about once a millisecond, weighted by cycles) on the port under rundesk. During play the game's own code is a small part of
the processor time: in a real-time minute the exe is about 2 %, the rest is the operating system's and the graphics driver's work
(ntdll 87 %, nvwgf2um 5 %), which the game's code cannot make cheaper. At start-up it was different: before the title screen the
port decoded all 82 pictures (72 ms) and all 34 sounds (about 100 ms) whether the game used them or not, which was also
18 MB of memory at the title. The rest of the 0.8 s is the framework's start-up (`SexyApp::Init`: the window and the
graphics device, 250 to 310 ms) and the process's own start, which are shared code (see the spec below).

**What changed: pictures and sounds are loaded at their first use** (the way Mummy Maze's port does it, commit b5d51336). `NoahApp`'s
`mImages` is an `ImageTable`: indexing it gives the picture, decoded on the first request, and the loading bar still counts the
sounds as before (`LoadingThreadProc` adds their number to the bar's completed tasks at once). `NoahApp::PlaySample` loads
the sound on its first play (`LoadSoundOnce`); the mixer decodes outside its lock and registers the sound inside it
(`Platform::Mixer::LoadSoundData`), so a first play in the middle of a game is safe. Which pictures and sounds are used, and in what
order, is the game's, so every draw, every random number and every screen is the same as before.

**Deviations of this change** (each listed because the original loads all of them first): a picture that is missing or bad is told at
its first use instead of at start (the same popup, the game ends, and a blank 1x1 picture stands in for that one frame), and a sound
that is missing stays silent instead of being told at start; the first use of a picture or sound costs its decode in that frame,
once. Nothing shows in the game's files as shipped.

**Proof that play is unchanged.** `tools/check.sh noahsark` passes with the frozen baseline identical (the bot's action, strategy and
puzzle games end on the same updates with the same scores, the rerun is identical and the baseline is identical). Besides
that, the same action game with `BYOPOP_SOUNDLOG` set played the same sounds on the same updates before and after (417 plays;
`tools/soundcmp.py` lines up the two logs and takes the new `load` lines out), and the autoplay log and the state log of the
bot's game are byte for byte the same.

**Numbers, native, before and after.** The first measurements (section 10 above) were taken with other sessions' games running, and a
before-block and an after-block taken minutes apart were not comparable, so these are twelve pairs run in turn (before, after,
before, ...; `tools/profile/loadpairs.sh`) with other sessions' games running (the machine busy 33 to 100 %, median 45 % before and 41 % after). "Before" is the port as
merged at 6d7b2b79, "after" this pass's build; `PORT_EXE` makes `perf.py` run another exe.

| First screen that takes input (the title's play link), 12 pairs | Before | After |
|---|---|---|
| Seconds from the process's creation, median | 0.99 | 0.60 |
| Fastest run | 0.62 | 0.46 |
| Pairs in which the after run was faster | | 11 of 12 (median difference -0.37 s) |
| Private bytes at the title, median | 56.0 MB | 38.2 MB |
| Processor time until the title, median | 0.62 s | 0.44 s |

Four runs of the 24 took 3.9 to 50 s because another session's heavy run was on the machine at that moment (the before runs of
pairs 1, 2 and 4 and the after run of pair 12); they stay in the medians. The medians of the eight pairs in which both runs were under 3 s are
0.93 s before and 0.54 s after. The original's 2.12 s in the table above was measured in another hour and is not compared to
these directly; both of the port's figures stay below it.

The scripted minute (the table above, 6000 updates of the bot's game on the real clock), run twice before and twice after: updates
per second 105.2 and 105.2 before, 104.3 and 103.8 after (the clock paces them, so this is the same); processor time in the minute
8.8 and 7.3 s before, 7.95 and 7.95 s after (the machine was busier than at the first measurement, which gave 5 s, and the
spread is the machine's, not a difference of the change); peak private bytes in the minute 70.8 and 69.1 MB before, 63.0 and
66.1 MB after (the pictures and sounds the minute never reaches stay undecoded). An uncapped run of the whole bot game (`-fast`,
7401 updates, five pairs, the framework's own "Seconds" figure) took a median of 1.57 s before and 1.34 s after, inside the
spread of the runs (1.28 to 1.75 s before, 1.10 to 1.77 s after).

**Numbers, web build under Node.** The Browser pane is hidden and runs no animation frames, so the web build was timed as in the
other ports: `tools/profile/webpairs.sh` plays a whole autoplay action game with `-fast` under `wasmrun.js game` (Node's WebAssembly,
no canvas, no audio device), the earlier sources' wasm and this pass's in turn, three times each. The framework's own "Resource
Loading Time" line (the app's creation to the sounds done, in the web build's file system) was 415, 287 and 256 ms before and
84, 126 and 67 ms after (medians 287 and 84 ms). It is Node's load, not a browser's: a browser also fetches and decodes the
files, which the lazy loading moves to the first use, so the wait before the title shortens by at least this much. The three
games are identical in their autoplay logs (the same updates, the same scores: level 4 done at update 17369, score 6074), before and
after. The wall time of a whole game, 17.4, 21.0 and 37.7 s before and 19.9, 60.9 and 32.0 s after, says nothing about the change:
the machine's other runs moved it by a factor of three between runs, and the same work is done in both.

**Spec for the coordinator (shared code, not done here).** About 0.3 s of the port's 0.6 s start is `SexyApp::Init`: creating the window
and the graphics device (250 to 310 ms in the profile). A start that shows the window at once and creates the device after it, or
in the background while the game builds its tables, would take part of that from every game; it changes what the framework does at
start and is the coordinator's.

**Tools of the pass** (all in `tools/profile/`): `prof.sh <name> <seconds> <script>` (the sampling profile of the port under rundesk,
waiting for room), `perfpass.sh` (the pairs of the minute and the load, before and after), `loadpairs.sh [pairs]` (the interleaved load
pairs), `webpairs.sh <before dir> <pairs>` (the web build's pairs under Node) and `perf.py`'s `PORT_EXE`.

**Code taken out** (the second part of the pass, after the coverage check's candidates): `Board::RefillPuzzle` (0x4061d0) and
the puzzle branch of `Board::EnsureMoves`, which the new level code never reaches (it calls `EnsureMoves` only for modes other than
puzzle and Stampede); the comment in `EnsureMoves` says so. The suite passes with the baseline identical. The rest of the port's
functions were all reached by the original's runs (section 11), apart from `AbandonPair` and `TriggerNoMoves`, which a board with no
pair left reaches and which were read again against the export; the fields named `Unused` stay because the original's objects and
save files keep them.

## 11. Coverage check (2026-11-08)

The coverage check of `docs/COVERAGE.md`, the second game's whole one after Ning Po's. origrun's `--coverage` recorded which of the
original's functions ran in 15 runs on the host (`tools/coverage/run.sh` starts a run, `all.sh` is the list of them; the hits are in
`build/coverage/noahsark/hits/`). Fourteen were on the virtual clock with no sound device, one on the real clock with the
silent stand-in, and every one of them ended with the game exiting:

- the identical-play cases: an easy and a hard action game (`act_e`, `act_h`), strategy (`str_e`, `str_h`) and puzzle (`puz_e`, `puz_m`),
  each played by the port's bot and replayed in the original as clicks (`tools/origrun_replay.py`);
- two games lost on purpose (`lose_act`, `lose_str`: the bot stops moving at update 800, the water comes up and the game over screen
  follows) and two that end in a ranked score, so that the name entry and the high score list run (`rank_act`, `rank_puz`: the bot
  plays on to a score the table keeps; `REPLAY_TEXT_DELAY=3` types the name three updates later in the original, which takes the name
  one update later than the port);
- the Stampede (`stamp`): a Stampede is never started by a menu, so the run loads a saved game of it (`SAVE=` in `run_original.sh`
  stages a save file as the copy's `data/savegame.dat`) and plays the three levels out;
- the dialogs tour (`dialogs`): both sliders, the hint box, Help (which closes Options in the original: see below), High Scores and
  Credits from Options, the quit question from play and from the menu answered no and then yes, Continue, a lost and regained
  focus, then the strategy and puzzle intros;
- the real-clock tour (`rt`): a played minute of an action game on the real clock, for the sound code;
- the drags (`drag_puz`, `drag_act`; `tours/drag_puz.txt`, `tours/drag_act.txt`), added after the first thirteen, once origrun's moves
  carried the held button's flag (865a602e): a `down`, moves and an `up`, each at an update of its own. In the puzzle: a drag onto a
  neighbour (the swap, and the swap back), a move during the swap, a drag two cells away that drops the pick, then a drag that picks the
  animal it reaches and goes on to swap it. In the action mode: a link dragged from (0,1) to (0,0), a move inside the cell, back onto
  the first animal (the link taken back), on to (0,2) (linked again), onto another kind and onto an animal it cannot reach (nothing), and
  the release (the pair joined); then the Options sliders, each pressed at its thumb and dragged to the left.

`games/noahsark/coverage.txt` sets the game's own code as one range, 0x401000-0x420d10 (Board, the dialogs, the screens, NoahApp and
the compiler's copies of the standard library): 258 functions. The rest of the exe is the 2002 SexyApp framework (from Widget at
0x421d60), the C runtime and the libraries. The first report, from ten of the runs, had the runs reach 205 of the 258 and the port
define 142 of them; the last one, from all 15, has them reach 221 and the port define 169.

**Reached in the original with no counterpart in the port: 79 at first, 0 at the end.**

- **Placed: 24.** Functions the port translates inline or in another shape got their address in the comment above the function that
  holds them: the destructors and manager hooks of the dialogs and widgets (`AddedToManager`, `RemovedFromManager`, `Resize`,
  `GetPreferredHeight`), the clearing of the fish and bubbles inside `NewLevel`, the combo flags in `ConnectPair`, `WinMain` in
  `main`, `UpdateFrames`, `LostFocus`, and the nine-patch box draw that the original has twice (0x4126d0 and 0x416160, byte for byte)
  and the port holds once as `DrawImageBox2003`. Comments only.
- **Left out on purpose: 53 at first, as `none` lines in coverage.txt with their reasons** (the later runs added a few more; 78 functions
  are left out in all, the registration dialog's nine among them): the registration dialog and its two helpers (taken out, section 9);
  the ten compiler-made scalar deleting destructors; the framework's default overrides, which the port's classes inherit; the
  original's own `std::string`, `std::vector`, `std::list` and `std::map` instantiations (MSVC 6), which the port's standard library
  replaces; the global NoahApp object's start-up and exit glue (the port's `main` makes and deletes the app); and 0x41d3b0, the Options
  check boxes' handler for a display on which windowed mode cannot be restored (the port's SDL window can always switch back).
- **Missing: three, all translated.**
  - **Help in the Options dialog closes Options first** (the original's Help button reaches the mode intro's creator): it applies the
    full screen and hint boxes, closes the dialog and then shows the intro for the current mode. The port's `ShowModeIntroDialog` left
    Options open under the intro, so the click chain of the dialogs tour (Help, OK, Options, High Scores...) went another way. Now the
    same, at the top of `ShowModeIntroDialog`.
  - **A game in progress is saved when the app is destroyed** (`NoahApp`'s destructor, 0x4169e0). The original's destructor runs at the
    end of `WinMain`, so closing the window during a game saves it and the menu then offers Continue. The port's `~NoahApp` deleted
    the board unsaved (`SaveGame` already returns early on the game over screen). Now it saves first, as the original does.
  - **`HighScoreWidget`'s destructor writes hiscores.dat** (0x411e20). The original's `AddScore` does not save: the file is written
    when the widget is destroyed, on leaving the screen or at exit, and it creates the file when none exists. The port's `AddScore`
    saved at once and its destructor only deleted the link. The destructor now saves too; the `Save()` in `AddScore` stays, for the web
    build, which never deletes the app. The table in memory and in the file after a recorded score is the same either way.
- **Left open: none.**

**Defined in the port, never reached: 3 at the end** (the later runs and the placed addresses cleared the rest):

- **`Board::MouseDrag` (0x40f710): rewritten, and now run in the original.** The re-read found six slips against the export. In puzzle
  mode a drag from the picked animal onto a neighbour swaps the two at once in the original; the port ignored the drag, so the player had
  to release and click the neighbour. The link drag left the animal dragged from selected where the original selects the one dragged
  onto, played no sound, neither took the hint away nor noted the time of the move, and let a path that `BuildTrail` fails to make stop
  the link. All six are now as the original, and the puzzle swap is one helper (`Board::PuzzleSwap`) for the click and the drag. The
  suite and the frozen baseline are unchanged (the clicks and the bot do not drag). `drag_puz` and `drag_act` reached 0x40f710 and
  0x40f3a0 (the two statements that note the mouse position, placed above `MouseDrag`), and the same scripts played in the port
  (`-script`, the same updates; the port's `state` shows the board, the phase and the score at the updates asked for, not the selection
  or the link) agree with what the original's watched values show: the puzzle's swap comes the update after the move
  (the two cells' kinds trade at counter 746 in the original), the swap back follows 25 updates later and the board plays again 26
  after that; the pick dropped by the move two cells away, the animal picked by the next move and the swap with the one below it come
  at the same updates; the link takes the animal dragged onto as the selected one, the move inside its cell changes nothing, the move
  back onto the first animal empties the link and selects the first again, the move on to (0,2) links from it, the other kind and the
  unreachable animal change nothing, and the release joins the pair (the score 0 to 10, the board's two cells gone) in both. Before the
  rewrite the port did none of the puzzle drag. `tools/coverage/dragtest.py` also drags the port's board with real `down`, `move` and
  `up` events through the real-input path (a swap, and nothing for a drag two cells away). Two things the first try of the scripts
  showed: the get-ready click picks the animal under it in both, so the tours click outside the board to put it back before dragging;
  and a press on the board with an animal picked drops the pick (the click path's second step), which is the original's too.
- **`OptionsDialog::SliderVal` (0x41d370): reached by `drag_act`**, which presses each slider at its thumb (the thumbs are near the
  right end: the music's at about (517,113), the effects' at (517,145)) and drags it to the left. The original calls it, and it
  matches the export (the music volume for slider 0, the effects' for slider 1). The port's own slider is the framework's, shared with
  every game; its dialog is laid out differently from the original's (21 px lower, 42 px shorter), so the original's tour is not
  replayed in it.
- **`Board::AbandonPair` (0x4067e0) and `Board::TriggerNoMoves` (0x4074c0):** reached only by a board with no pair left, which none of
  the 15 runs came to. Both were read again line by line against the export and match, including the points taken, the water's move and
  the sample played.
- **Taken out (2026-11-09, the performance pass's second part): `Board::RefillPuzzle` (0x4061d0) and the puzzle branch of
  `Board::EnsureMoves`.** The original's only caller of `EnsureMoves` is the new level code, which calls it only for modes
  other than puzzle (4) and Stampede (5), so the branch that calls `RefillPuzzle` never runs in either. The port had the same guard;
  the function and the branch are gone, with a comment where the branch was, and the suite passes with the baseline identical.
  (The read also found that the port moves a falling animal up by swapping two pointers where the original makes a
  new piece and deletes the old one, and sets `mY` of both cells, which the original does not: it cannot matter while the code is
  unreached.)

**Tool specs for the coordinator.** None left. The first version of this section asked for a `drag` command; origrun's `down`, `move` and
`up` at updates of their own do it once its moves carry the held button's flag (865a602e), and that is what the two drag runs use. The
host takes no screenshots, so the original's Options dialog, which is taller than the port's drawing suggested, had to be measured in
Windows Sandbox (`SANDBOX=1 tools/run_original.sh`) before the clicks of the dialogs tour landed.

**Other differences found while reading, none with a visible effect and none changed:**

- The original's `Board` destructor leaks the elements of the texts and sparkles vectors and calls `InitGrid` at the end, which allocates
  90 pieces that nothing frees; the port's `~Board` frees more (the leaks of the original are fixed in the port).
- 0x402de0 (clear the fish) ends with a call of Widget's ninth slot (the 2002 dirty-rectangle drawing's flag) with 0, and the fish's
  creator 0x408df0 with 1; the port has no such flag and calls `MarkDirty()` instead.
- `OptionsDialog::SetColor` (0x41cd40) colours five buttons in the original and three in the port (the Register and Web buttons are gone,
  section 9). `~TitleScreen` deletes the "register now" link too.
- `NoahApp::UpdateFrames` (0x418c00) sets a flag at +0x450 that nothing reads.
- `Board.h` had the fish vector at +0xd94; it is at +0xda4 in the original (the one at +0xd94 is never used). Comment corrected.

**Cost.** The original ran for about 20 minutes of machine time all together, and the wait for room (at most 3 game processes on the
machine) took more than that. Sonnet agents sorted the first report's 79 rows (17 minutes, 338k tokens) and read the five port
functions again (10 minutes, 217k tokens); the rest was this session's, with a few sandbox runs for the Options layout.

## 12. Code the port does not need (2026-11-10)

The step after the performance pass (`docs/COORDINATION.md`). Registration and the buttons that lead out were already out
(section 9), and `Board::RefillPuzzle` with the puzzle branch of `EnsureMoves` went with the performance pass (section 10). This
survey looked for what no path of the shipped game reaches or needs, by seven scans of the game's `src/`
(`tools/unneeded/scan.py` runs the first four):

1. **Functions.** The 19 object files of the native build, every function they define against every relocation in them
   (`llvm-nm --defined-only`, `llvm-objdump -r`): 41 of 376 have no reference. Twenty-seven are the aliased forms of constructors
   and destructors and one is `SDL_main`; the other thirteen were searched in the source and all are called, inlined by the
   compiler (`RankOf`, `PlaceBlocks`, `PlaceCrates`, `AllClonesGone`, `TryRemoveClonePair`, `DrawTrailArrow`, `ExchangeContents`,
   `RemoveIntro`, `PlayGameMusic`, `ShowNewGameDialog`, `RemoveHighScoreWidget` and the bot's `BotRand` and `BotQuiet`). Every
   function a header declares is defined.
2. **Compiler warnings.** Every source compiled with `-Wall -Wextra -Wunused -Wunused-member-function -Wunused-private-field
   -Wunused-template -Wunused-macros -Wunreachable-code`: 245 warnings, all of them in the framework's headers (243 unused
   parameters of its empty listener methods and two deprecated copies), none in the game's files.
3. **Fields.** Every member `m...` of the game's headers (227) by how often code reads and writes it: eleven were written and
   never read, or never touched at all. Each was then looked up in the original's disassembly (0x401000-0x420d10), so that a
   field the port never reads is not a sign of something left untranslated: the original only writes them too.
4. **Constants.** Every enumerator, `#define` and `const` (206) by use. The ones named once are members of a run that code reaches
   by adding to its first (see 5), and `PIECE_ANIMAL_LAST` and `PIECE_UNUSED`, which the code tests as numbers; they stay for
   the readability pass, which gives those numbers their names.
5. **Pictures and sounds.** Every id of `Res.h`: all 82 pictures and all 34 sounds are named by code the game runs, or reached
   from a named one: the second rain picture (`IMAGE_RAIN1 + (tick & 1)`), the ark's five ramps (`IMAGE_ARKRAMP_1 + stage`),
   Noah's frames (his sprite's first picture plus the frame, and `IMAGE_NOAH_ASCEND1` plus the frame), the thirteen animals'
   sounds (the kind plus one), the eight combo sounds (`SOUND_COMBO1 + n`) and the second thunder (`SOUND_THUNDER3 + (rand & 1)`).
6. **Texts.** Every entry of `GameTextTable.inc` (the addresses, lengths and hashes of the texts the port reads from the player's
   exe) against the addresses the sources name: 177 of 866 were named.
7. **Windows leftovers, switched-off code, comments.** `_WIN32` appears once (the search for the retail install folder, which a
   Windows player of the unpacked game needs), no `#if 0`, no commented-out code, no `TODO`.

| What | Why it is not needed | Verdict |
|---|---|---|
| `Board::mFlag14` (+0xe14), `mPathStartX`, `mPathStartY` (+0xe18, +0xe1c), `mPathValid` (+0xe20) | the original only writes them (the constructor, the new level and `ClearLink` 0x403910); no instruction of the exe reads those offsets | removed |
| `Board::mState31cUnused` | a field of the port with no offset in the original and no use | removed |
| `PathNode`'s fields at +0x08 and +0x14 to +0x1c, with the working out of each trail node's kind of bend (0 to 3) or arrow direction in `BuildTrail` (0x402ec0, about 45 lines and five locals) | the trail's builder stores the kind at +0x18, the path search and the hints store 0 and -1 there and a path's copy carries it along, and nothing in 0x401000-0x410bf0 reads it: the drawing (0x40c8a0, 0x40c460) takes bends and the arrow from the neighbouring nodes | removed |
| `Piece::mUnused60` (+0x60) | set to 0 by the constructors and carried by the copy (0x4060d0); the only reads of an offset 0x60 in the Board's code are of the picture table | removed |
| `NoahApp::mFloodStart` (+0x47c) | written by the new level and at the end of a strategy level, never read (the app is the global object at 0x49c518 and no code addresses a field inside it directly) | removed |
| `NoahApp::mGameStartTick` (+0x494) | set when a game starts (0x41858e), never read: the level's time comes from the Board's own `mStartTick` | removed, with its clock read |
| `NextLevelWidget::mBonusScore` (+0x68) | set to the game mode after a bonus round (0x414f71), never read | removed |
| the third parameter of `Board::AddFloatingText` (0x407bd0) | the original's callers pass the link's size less one, 0 or 1, and the function never reads it | removed from the function and its five calls |
| 689 entries of `GameTextTable.inc` | strings of the original that no source names: the framework's own messages and registry names, the resource file names, the registration texts | removed: `make_gametext.py` now lists only the texts the sources name (866 to 177) |

The exe is 3,055,616 bytes (3,064,320 before).

**Looked at and kept.** `Board`'s fields at +0x314, +0x320, +0x324 and +0x330 are written to the saved game and read back and
nothing else reads them, so the save's format keeps them (the readability pass names them for what they hold).
`Board::AbandonPair` and `TriggerNoMoves` run only on a board with no pair left, which no run of the coverage check reached;
both match the export (section 11). The overrides of the framework's hooks (`LoadingThreadProc`, `LoadingThreadCompleted`,
`NewDialog`, `LostFocus`, `HandleCmdLineParam`, `HandleScriptCommand`, the registry pair, the two `PlaySample`) are called by
the framework. `Mods.cpp` and `ModFeatures.cpp` are the code of the four mods and the fixes mod, and `SiteProfiles.cpp` the
site's hook. The test hooks (`src/testhooks/`, about 1,100 lines: the test commands, `-autoplay`, the state log, the bot) are
compiled into the one executable that players get and are reachable only from a test; taking them out of a shipped build is a
question for every game, which Alchemy's notes raise (section 10 there). The texts in the player's exe that the port no longer
lists are the game's own data and stay where they are.

**Proof.** After the removals `tools/check.sh noahsark` passes with the frozen baseline identical (the bot's action, strategy and puzzle games, their rerun, the mods' 13 checks and the high scores' 3), and
`tools/check.sh build` passes (the whole native build, tools included). The suite plays the game's logic and compares the bot games' screens with the
baseline's; for the dialogs and their texts a screen tour was run with a control (`tools/unneeded/screens.sh` and `tour.txt`:
the title, the menu with a hover line, the difficulty and tutorial boxes, an action game, the Options dialog with both sliders
and the hint box, Help, High Scores, Credits, the quit question answered no and yes, Continue, the focus lost and regained,
the new-game question, strategy and puzzle games, the menu's quit question; same script, seed 1, `-fast`). It ran on a build of
the sources as they were before the removals and on the build after them, and all 26 screenshots are byte for byte the same (25
different pictures; a contact sheet showed each dialog was reached). For the texts a script also checked that every line the
new table keeps is a line of the old one, and that no address the sources name is missing from it.

## 13. The readability pass (2026-11-11)

The pass of `docs/CODE_STANDARDS.md`: `python tools/namecheck.py games/noahsark` has no finding and needs no exception file
(110 findings before: 55 functions without an address, 38 one-letter names, 17 uses of a name with an offset in it). Noah's Ark
has no Mac build, so every name comes from how the code uses the thing. Nothing the port does changed, apart from the two
faults the pass found and fixed (below).

**Locals and parameters.** About 195 one- and two-letter names got words with the framework's prefixes: a `Piece* p` is
`aPiece`, the second of a pair `aOther` (or `aSecond` and `aThird` in a run of three), `a` and `b` are `theFrom` and `theTo`
or `aFirst` and `aSecond`, `d` is `aDir`, `nx`, `ny` `aNextX`, `aNextY`, `dx`, `dy` `aStepX`, `aStepY` in the path search,
`aOffsetX`, `aOffsetY` around a text's outline and `aDistX`, `aDistY` where an animal flies, the save's `w` and `r` are
`aWriter` and `aReader`; `x1`, `y1`, `x2`, `y2` are `theFromX` to `theToY`; the mods' date helpers spell out the civil
calendar's terms (`anEra`, `aDayOfYear`, ...); `main.cpp`'s locals take the prefix. Coordinates stay `x` and `y`.

**Fields.** Named for what they hold, from their use in the port and in the original's disassembly:

| Was | Is | What it holds |
|---|---|---|
| `mState314` | `mLastThunderUpdate` | the update count at the last thunder; saved, and nothing reads it |
| `mState320`, `mState324`, `mState330` | `mUnsetAfterStartTick`, `mUnsetBeforeLastMoveTick`, `mUnsetAfterShowGetReady` | three fields the original never sets and writes to the saved game as they are; 0 in the port; nothing reads them |
| `mFade` (Board) | `mLostFade` | counts up to 0xff once the game is lost; saved, and nothing reads it |
| `mSetUp3ac` | `mLeftBehindFading` | the animals still on the board when the ark leaves have been set to fade |
| `mPulseA`, `mPulseADir` | `mDangerPulse`, `mDangerPulseDir` | the red of the water near the ark, 0..0xab and back |
| `mPulseB`, `mPulseBDir` | `mFlashPulse`, `mFlashPulseDir` | the grey of an animal that flashes for a hint |
| `mWavePhaseA`, `mWavePhaseB` | `mBackWavePhase`, `mFrontWavePhase` | the scroll of the wave behind the board and of those in front |
| `mComboShownAt`, `mComboShown` | `mShownCombo`, `mComboTextTimer` | the number in the combo line, and how long the line stays (the old names read the other way round) |
| `mSpawnRate` | `mNewRowKinds` | how many kinds of animal a pushed row is drawn from |
| `Piece::mMatchSwap` | `mInSwap` | one of the two animals of the puzzle swap in progress |

The saved game's layout keeps the five that nothing reads (rule 3); each one's comment says so. `TitleScreen::mClicked` is at
+0x70 (its comment had "+0x70-ish").

**The two obstacles.** `PIECE_BLOCK` and `PIECE_CRATE` named each other's kind: the one called block fell and showed
crate.jpg, the port's numbers were the original's the other way round, and the saved game's reader and writer swapped them back
(`FileType`). The names were first swapped at every use (object code identical), then the values put the original's way:
`PIECE_BLOCK` is 15, the original's 0xf, block.jpg, which stays where it is (0x401ce0 places it, the draw at 0x409d13 takes
picture 7 for it), and `PIECE_CRATE` is 16, 0x10, crate.jpg, which falls. `FileType` is gone. In the state log and the `state`
command's board, `X` is and was the crate and `Y` the block (their comments said it the wrong way round too).

**Numbers.** `NUM_ANIMALS` and `PIECE_ANIMAL_LAST` for the 0xd and 0xc of the kind tests (`PIECE_UNUSED`, a kind no code ever
sets, went); a kind's sound is `SOUND_WINPIG + kind`; `Board::GRID_SIZE`, `GRID_LAST` and `CELL_SIZE` for the 9, 8 and 50
(0x32) of the grid's loops, bounds and pixel offsets, about 165 places (a 0x32 that is a timer, a frame or a colour stays a
number); `DIALOG_YES` and `DIALOG_NO` for the 2000 and 3000 a dialog's buttons add to its id; `GT_EMPTY` for the address of
the original's empty string; `DIFFICULTY_EASY`, `_MEDIUM`, `_HARD` (0, 4, 8) and the difficulty dialog's `BUTTON_EASY`,
`_MEDIUM`, `_HARD` (60 to 62).

**Addresses and comments.** Every function names its address, the original function it is part of, or a `port:` marker. The
overrides the dialogs and screens share in the original got their addresses from the vtables (0x4165d0 and 0x4126b0 for the
level-up panel's and the game-over screen's manager hooks, 0x420a30 for the name entry's and the tutorial's, 0x411730 and
0x4209f0 for the heights). `GameText.cpp`, `ModFeatures.cpp`, `main.cpp` and the three test hook files carry the marker in
their first comment. The file headers no longer speak of a `Board.cpp` (the Board is six files), comments name functions by
address and no longer by the decompiler's names (`FUN_004060d0`, `DAT_00494120`, `vf17`), and a few comments that told what
had been taken out, or who decided it, now say what the code does.

**Found by the pass, and fixed.**

- *Enter in the name box.* Giving `RecordScoreDialog::EditWidgetText` its address (0x41de20) showed that the port's was empty
  while the original's calls the app's `ButtonDepress` with the dialog's id plus 2000, the OK button: Enter records the name.
  No run of the identical-play passes or of the coverage check had pressed Enter there (the bot and the replays click OK), and
  the coverage report could not show it, since the port's function had no address to pair. The port's now does the same. A
  script that sets a score, loses, types ABC and presses Enter ends on the high-score list, with ABC and its score in
  `hiscores.dat` and in the registry's last name; before, it stayed on the name entry. The original's side was read, not run: its edit widget's `KeyDown`
  (0x4491a0) hands every key but the letters to its key function (0x447ee0), which calls the listener's `EditWidgetText` for
  key 13, and the name dialog's `EditWidgetText` (0x41de20, eight instructions) calls the app's `ButtonDepress`; PopCap's
  released framework source has the same chain. Runs of the original with Enter in place of the click on OK decided
  nothing: on the host no scripted key or character reaches the original at all (none of the edit widget's key functions is
  ever hit, in the ranked-score run of 2026-11-08 either, whose typed name never arrived), and in two runs in Windows Sandbox
  the name was recorded neither after Enter nor after a click on OK that followed it, so the typed name had not arrived
  there either. Those runs also showed why a replay of the original can stay on its title: the link appears at a different
  update each run (104 on 2026-11-08; 124 to 364 on 2026-11-11, later under the coverage recorder, a cold file cache or a
  loaded machine), because the loading thread works in real time while the updates run ahead on the virtual clock, and a
  script that clicks the link at update 301 misses it when it comes later. A script for this game waits for the link first
  (`waitfor [[0x4d125c]+0x414]+0x68 != 0`, then `mark` and `at +N`).
- *A crash on closing the game with the name entry open.* The same script without the Enter, quitting at the name entry, ended
  in a segmentation fault: `NoahApp`'s destructor deleted its screens while they were still in the widget manager, and the
  framework, removing the dialog afterwards, gave the focus back to the game-over screen that was gone. The destructor now takes
  each screen out of the manager before deleting it (it already did for the board), and the script exits 0. This is the
  port's own fault, not the original's: listed here with the deviations.
- *An override that only called its base*, `NoahApp::LoadingThreadCompleted`, went (it belongs with section 12).

**Proof.** A rename is proved by the object code: `tools/readability/objcode.sh` writes every unit's instructions, relocations
and data, and after each batch (the locals and parameters, the fields, the kind numbers and the obstacles' names, the grid and
cell sizes, the dialog and difficulty names) all 19 units were the same as before it. `tools/readability/rename.py` renames
inside one scope and leaves literals and comments alone, after a first try that also renamed the `%u` of a format string showed
up at once as a different unit. The changes the object code cannot prove were proved otherwise. The obstacles' values: a
strategy game on hard played by the bot for 2500 updates and closed gives the same saved game byte for byte (7,637 bytes), the
same bot log and the same state log before and after (`tools/readability/savecheck.sh`). That saved game, written before the change, also
opens the same in a build from before the change and in the build after it: the same state lines with crates and blocks on the
board, the same two screenshots byte for byte and the same game saved again at closing (`tools/readability/loadcheck.sh`), so a
player's saved game still opens. `Board::HintLine` answers a line out
of range with `GT_EMPTY` instead of address 0, the only unit that differs in that batch, and the removed override shifts the
addresses of `NoahApp.cpp` and nothing else. At the end `tools/check.sh noahsark` passes with the frozen baseline identical,
the screen tour of section 12 gives the same 26 screenshots byte for byte as before the pass, the web build plays the bot's
action game to the same line under Node (level 4 done at update 17369, score 6074, the native build's line), and `tools/check.sh build` passes.
