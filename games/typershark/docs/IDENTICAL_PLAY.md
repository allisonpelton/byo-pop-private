# Typer Shark: identical play

The report of the identical-play pass, for the user. Phase 1 compared how the game plays (its state, update by update), phase 2
how it looks and sounds (screens by shotdiff, sound, focus, the web build, play with real mouse and key events, and the 3D drawing). The working
notes of phase 1 are in `PHASE1.md`; the code facts are in `GAME_NOTES.md` (deviations in section 5, oddities in section 7).

## Phase 1: the game plays the same

### What was compared, and how

The original (Typer Shark Deluxe 1.02, the exe carved from the Steam copy) ran under origrun on its virtual clock with its random
seed pinned; its memory was read at chosen updates (`at <N> peek`). The port ran with `-fast -seed=S -statelog`. Both got the same
input at the same updates, from the same empty `users` folder and the same menu flow (a new player, no typing test, the mode's
button). `tools/phase1/cmp.py` set the two side by side update by update: the update count, score, lives, the level timer's three raw
values, the pause and hint flags, the letters typed, the dive, the diver's state, the zapper's charge, the game generator's index
(every number drawn), and for each creature its kind, word, position, state and letters typed, in list order, the clam pairs,
the end-of-level countdown, the difficulty, the typo counter and the wave type. No pictures in this phase.

The games played: the menu to the first board; level 1 with three seeds; levels 1 to 3 with three seeds; an idle game to the game
over; typos in levels 1 and 2; black sharks in levels 3 and 4; the first 25 levels of an Easy adventure and on to level 34 (every
shark and piranha kind, the torpedo boss, the mecha shark, the mecha squid; 153190 updates, none of 11064 reads differ); the zapper;
the abyss with three seeds (to 484 and 544 feet); the typing tutor's lessons 1, 12, 13, 15 and 17 to their ends, two of them with
wrong keys; a treasure level with its 22 clams; Hard with every fifth letter wrong; Expert and Hard penalty piranhas; and the whole
Easy adventure by a second route through every boss, the last level, the end-of-adventure dialog and the endless levels, to update
200000. Every field read was equal, and where the scenario compared them the `users` folders were equal byte for byte at the end.

### Changes

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| A black or red shark took its second word again once it was typed and never died, so a level with one never ended | `BlackShark::TypeLetter` 0x409fb0 and `RedShark::TypeLetter` 0x40b600 empty the word they take; Ghidra's export left that statement out | empties it (`mSecondWord = ""`, `mThirdWord = ""`) | be952da8 |
| The abyss went to the next level every 100 feet | `Board::Update` 0x45c140 compares depth / 200 | `/ 200` | be952da8 |
| The end-of-level word picker hung once every word of the list was used (found as an oddity, then fixed: it is a soft lock) | `GetLongWord` loops until it finds an unused word | stops when none is left | ce5f631b |

### Oddities (not chased) and what was not compared

A new profile's file holds an uninitialised number in the original (`UserProfile` +0x1c8) where the port writes 0: the file is 8 to
10 bytes longer in the original, and nothing reads the number back before it is set. To see it: a new player, one tutor lesson,
compare `user0.dat`. The other oddities and the quirks the port keeps are listed in `GAME_NOTES.md`, sections 5 and 7.

Not compared in phase 1: the round-2 scenarios with seeds 2 and 3, Normal and X-Treme (the same code with other numbers from the
level file), a level jump on a fresh profile, and the registry values. A saved game and the "continue?" question were left for phase
2 (below).

## Phase 2: it looks and sounds the same

### Method

A tour is one list of steps (`tools/phase2/p2lib.py`) written twice: an origrun script for the original and a `-script` for the
port, with the shots at the same updates. The original runs inside Windows Sandbox (the only place origrun takes shots) on the
virtual clock with the scripted cursor; the port runs under rundesk with `-fast -drawevery`, windowed and without 3D as a sandbox
run is (the 3D drawing was compared apart, on the desktop: "The 3D drawing" below). The 640x480 client is cut from the original's capture and the pairs go through `tools/shotdiff`; only the boxes it flags
were looked at. The original's capture is 2 levels of 255 darker on every channel than the port's picture everywhere (white is 253),
which shotdiff's tolerance takes in.

Three things about the method had to be put right before the game tours meant anything, and each first showed as a difference:

- *The original was given the port's users folder* (an environment variable that both run scripts read), found no `users/user.idx`,
  wrote one and drew a random number for it, and so played another game from the first board on. `runorig.sh` now takes the
  original's folder from its own variable (862ab070).
- *The port's shots were one input ahead* in the tours written before the shot-before-input order was fixed in the generator.
- *The framework's `screenshot` command draws the screen once more.* In this game a draw is a step: the sea's two strips, the
  rating popup and the map's last scroll ease once a draw, so every shot put the port a step ahead (the arch on the map one pixel
  off, sea pixels a level apart). The port's shots are now taken by its own test command `shot`, which reads the screen back
  without drawing (3897691a). With it the map's eased scroll reads the same values in both programs at all 16 updates sampled.

*How the typed input reached the original.* Every character a tour types is a `WM_CHAR` that origrun posts to the original's window
at an update of the virtual clock (`at <update> text <c>`, one character an update; a key the game reads as a key down goes as `key`).
It needs no focus and nothing from the real keyboard. In the sandbox it arrived in every text box and every mode: the new-player
box (the name's letters appear one an update in the original's shots as in the port's, and a wrong letter followed by Backspace
comes out the same), the ten players of tour U, the tutor's lessons, and the letters typed at the creatures, for which the port's
own typist wrote the recording that both sides were then given (the original's boards, scores and lives then agree with the port's
update for update: phase 1, by `--watch` values). Typing was not taken as delivered because the message was posted. It was checked
by what it did: the pictures after each typed character, the player files each side wrote (tour R: the same byte for byte) and the
game state after the typing. On the host the first characters were lost until the script waited for the game's loading flag: the
original's loading thread runs on the wall clock, so a busy machine let the characters of update 1000 reach the title screen. The
host runs (the sound comparison, the restart tour's reruns) take phase 2's loading patch, which makes the copy load in one go on its
main thread, so the input starts on a loaded game.

### Tours and results

| Tour | What it shows | Pairs | Equal | Faint only |
| --- | --- | --- | --- | --- |
| M | title, new player, the WPM question, the menu and every hover, options (every box, both sliders, credits), hall of fame, statistics, the difficulty dialog | 98 | 98 | 0 |
| S | a saved adventure: the board, pause, options and quit in a game, the menu's "continue?" with No and Yes | 33 | 31 | 2 |
| U | the players: ten players made one after another, the "who are you?" list as it grows, its delete line, the delete dialog, a player picked | 70 | 70 | 0 |
| R | quit, start again, carry on (two runs of each program, below) | 18 | 18 | 0 |
| H | an abyss game over, the Hall of Fame with the player's line, the abyss button's continue list, the board from 400 feet | 24 | 24 | 0 |
| T | the statistics screen with a lesson played: the "view stats?" box at the end of tutor lesson 18, Yes, each of the three views, Close, the tutor after it | 17 | 17 | 0 |
| P | what the coverage check found the tours had not reached (GAME_NOTES, "Coverage check"): the Return key in the new-player box, ten players, a player picked with its OK, a player deleted (the question answered No, then Yes), the tutor's Next and Previous Lesson and Back To Menu's question (No, then Yes), the hint box closed with the space bar, the focus lost and regained on the board, the resting mouse | 32 | 32 | 0 |
| T2 | tour T with the statistics page's Previous Lesson (twice) and Next Lesson | 23 | 23 | 0 |
| Q | the typing test typed to its end: the "Test Completed!" message and its button | 6 | 6 | 0 |
| D | a player whose data file is gone (two runs of each program, as tour R): the error message at the start, its OK | 9 | 9 | 0 |
| I | the idle game's game-over box answered with PLAY AGAIN: the board restarts | 8 | 8 | 0 |
| idle | an idle game to the game over | 78 | 78 | 0 |
| abyss | the abyss with its bonus creatures and hints | 81 | 79 | 2 |
| tutor 17, tutor 18 | two typing tutor lessons (the keyboard picture, the hands, the finger rings, the results) | 283 | 283 | 0 |
| expert | Expert with penalty piranhas, stealth sharks and piranhas | 210 | 149 | 61 |
| bonus | Hard with treasure levels, clams, torpedoes, the level-end tally and the rank | 364 | 257 | 107 |
| whole adventure | every level of an Easy adventure: every map, the bosses, the end-of-adventure dialog, the endless levels | 772 | 460 | 312 |

No pair differs visibly. "Faint" is a box below shotdiff's visibility threshold; every faint box is one of three kinds, all
judged:

- *The status bar's meter edge* (404 boxes; 250 pixels, 10 to 24 levels of 255). While the board is not repainted and the bar is,
  the original paints the bar's half-transparent picture over itself again; the port draws every frame whole, the board first, and
  blends the bar once. A deviation of the shared framework, listed in `GAME_NOTES.md` section 5 with the measured colours.
- *A few pixels of the sea* (183 boxes; at most 22 pixels, at most 15 levels). The same cause: the sea's strips ease once a draw,
  the original stops easing when the board is not repainted and can rest a pixel short.
- *The window's last column* (12 boxes; 1 to 10 pixels in column 638 or 639 where a creature is entering at the right edge). Two
  runs of the port itself differ in these same pixels when a shot was taken the update before, so the column is not stable in the
  port's own screen read-back. Not chased: an oddity of the comparison, below visibility (see Oddities).

### Changes

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| The cannon balls of the ghost ship and of the squid showed on one frame in twelve, and a ball sent back was never seen | 0x407a90 draws frame `mFrame` of a picture whose frames lie one below the other (74 wide, 44 apart) | the same rectangle (the port had its x and y swapped) | 3897691a |
| The typing tutor's ring for the right middle and ring fingers sat 8 and 5 pixels too far left | 0x45e810 adds 8 and 5 to the ring's x for those two finger groups | adds them | 3897691a |
| After a lesson the tutor ran on under the statistics screen (its creatures swam and drew random numbers), and after Close it was a few updates ahead | the 2004 framework gives no widget the focus back when a modal dialog closes, and the tutor stands still while it has no focus (`TypingTutor::LostFocus` 0x442020); the shared framework is 1.3, whose `RemoveBaseModal` hands the focus back | `WinWSApp::AddDialog` forgets the widget that would get it, as the dialog is added | 84775dfa |

The last change was found by tour T: its views were equal, and the two pictures after Close differed (the tutor's creatures in other
places). Its cause is in the framework, not in the tutor's code, so the change is small, and it was checked the other way as well:
the tours with a dialog in them (M, S, U, H, the idle game, the abyss, tutor lesson 18: 547 shots) give the same pictures with the
exe before the change and the one after, and only the two pictures of tour T differ between them, to the original's. With the
change tour T is equal in all 17 pairs, and `regress.py` still matches the frozen baseline in all 13 games.

Three candidates from the reading were judged against the original's pictures and are no faults: the options dialog's Credits button
(equal in every pair), the keyboard picture's lit keys (equal; the mod's `tutorKeyboard` toggle mends what both show wrong) and the
map's final panel (both scroll differences are -60, the ship shows, equal in 52 pairs).

### Quit, start again, carry on

Tour S asks "continue?" within one run. Tour R ends the program in between (`tools/phase2/tour_restart.py`, `restart.sh`). Run A
plays an Easy adventure into level 2, presses QUIT and Yes, and the program ends on the menu. Run B starts each program again
from what its own run A left: the port from its user folder, the original from its `users` folder and the registry values origrun
exported after run A, put back as presets. In run B the menu greets the player without the new-player dialog, Adventure asks
"continue?" with the saved level and difficulty, and Yes goes on in level 2 with the same score (47801) and lives (2). All 4 pairs
of run A and all 14 of run B are equal, and the original's save file after run A is the port's byte for byte.

origrun's registry map (`--reg-map popcap`, built in this pass after the user's OK of 2026-11-10: a copy's HKLM\SOFTWARE\PopCap
keys go to HKCU, so that an unelevated copy on the host keeps its values between runs; `docs/ORIGRUN.md`) was tested on the host:
a first run wrote 108 values, a second run of the same copy found 95 of the 103 it read. The pictures above were made without it,
because inside the sandbox the copy writes HKLM itself. One run A with the map on in the sandbox ended a key press behind (still on
the map when the port was in level 2), with the same registry values at its end; one run is not enough to say the map caused it,
and it was not chased.

Two faults of the tools showed up as differences on the way, none of the game: `runorig.sh` copied the users folder a run left
into the folder of an earlier run, so run B first started from a stale save and offered no "continue?" (fixed: the folder is
cleared first); and on the host the original's loading thread runs on the wall clock, so with a busy machine the recording's first
keys (update 1000) reached the title screen instead of the new-player dialog (the host runs now take phase 2's loading patch).

### Sound

The same input on both sides: a new player, the menu, an Easy adventure through level 1, the map and the start of level 2
(14200 updates). The original ran on the host with origrun's recording stand-in, which logs every sample play, and with a logging
stand-in for its BASS, which logs the music calls (`tools/phase2/sound.sh`; Big Money's `bassproxy`, built without the one function
this game's BASS lacks). The port wrote its own sound log.

- *Sound effects:* 40 sounds loaded on both sides, none differs; 1169 plays on both sides, the same sounds in the same order with
  the same volume, pan and pitch.
- *Music:* the port played none. Its one module (`music\Wordshar.mo3`, played from seven order positions on two alternating
  songs) never loaded, because the game did not register the module player, and nobody had heard it: every test run is silent.
  Once it loaded, the second fault showed: at a change of music the original fades the playing song out under the new one, the port
  cut it off. After both fixes the two sides are equal event for event: 850 music events each (the plays with their order
  positions, every volume step of every fade, the stops).

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| No music at all, natively and on the web | BASS loads the MO3 module | `main` registers the module player (libopenmpt), as the other MO3 games' do | 955c6812 |
| The song that played stopped at once when the music changed | `ChangeMusic` 0x414350 fades it out at the new song's speed and stops it when it is silent (0x4144e7) | the same (the test of the speed was the wrong way round) | 955c6812 |

### Focus

Losing and regaining the focus was compared in two ways. In the tours the window's focus was taken and given back with the game in
play (tour S and the real-input runs below): the game pauses, shows its pause state and goes on, and the pairs around it are equal.
In the page check the web page hidden and shown again changes the same state keys as the native window's `focus lose` and
`focus regain` (paused and the timer while away; the hint box afterwards).

### The web build

*Pictures.* The web build ran four tours under Node from the same scripts as the native port, and the shots were compared with
no tolerance at all (`tools/phase2/webshots.sh`): the menus (98 pairs), the whole adventure (772), tutor lesson 18 (163) and the
abyss (81). Every pair is equal to within 6 levels of 255 (most within 3), which is the two builds' picture decoding. Three frames
hold one to four pixels that differ more, all of the "last column" kind described under Oddities; the one in the adventure tour
did not come back when its frame was shot alone.

*Music on the page.* After the fix the page's mixer follows the module (its title, 19 channels, the order position moving).

*The page check* (`tools/web/pagecheck.js` with `games/typershark/tools/web/pagecheck.js`): 11 lines pass, none fails. The page
hidden and shown again mid-game changes the same state as the native window losing and regaining the focus; the keys the game takes
stay out of the browser (the space bar, Backspace, the quote and the slash among them); the pointer lands on the right game pixel
with the canvas drawn at 0.75, 1.5 and 2 times its size; a second copy of the page waits for the first; and after a reload mid-game
the saves are back and the game in progress is kept. Return and Escape are not in the table's key list: the shared page does not
cancel their keydown, and a browser does nothing with either on the canvas. That finding is with the coordinator.

### Play with real mouse and key events

Scripts, the typing bot and runs under Node never pass the real input path, so code played every mode through it: a driver reads the
game's state from the test command `state`, takes its move from the port's own bot (`move`) and sends it as real events
(`tools/realplay.py`: SDL events into the native window under rundesk; `tools/web/onegame.js`: DOM events in the page). From a first
start each run types the player's name (a wrong letter and a Backspace first), answers the dialogs and plays the mode to its end.

| Run | Where | Result |
| --- | --- | --- |
| Adventure, Easy, to the end | native, windowed | won: 8068 moves |
| Adventure, Hard, to the end | native, full screen (Alt+Enter), focus lost and regained | won: 6862 moves |
| Abyss to the game over | native, windowed | 2500 moves, levels 0 to 7 |
| Abyss to the game over | native, full screen, focus lost and regained | 1200 moves |
| Typing tutor, all 18 lessons, the statistics screen, back to the menu | native, windowed | 321 moves |
| Typing tutor, 3 lessons | native, full screen, focus lost and regained | passed |
| Adventure, Easy, to the end, and the endless levels | web page | won |
| Adventure, Hard, to the end | web page | won: 6884 moves, 11 levels, no errors |
| Abyss to the game over | web page, the canvas at twice its size, focus lost and regained | 600 moves |
| Typing tutor, all 18 lessons | web page | 321 moves, the same count as natively |

One fault of the driver, none of the game: a `#` in a commands file starts a comment, so the native driver typed nothing for the
tutor's first `#` and lesson 10 stood still; it now sends such characters in quotes.

### The 3D drawing

The original draws the sea (and anything else) through Direct3D when the Options dialog's "3D acceleration" box is on, and the port does it
through the SDL accelerated renderer; every tour above ran with the box off, as the sandbox's display gave the original no 3D device. The user
allowed two routes ("Yes, both routes", 2026-11-11).

*The sandbox.* Four variants of origrun's sandbox run (with the host's GPU, with dgVoodoo, with the connect option, with both) ran the original in
2D with 3D asked for: the game's 3D flag was 0 at update 5 in each, and origrun ended with its exit code 3.

*The desktop.* A copy of the original runs on the user's visible desktop (`tools/phase2/visrun3d.py`) with the registry values for 3D on and windowed
(the key put back, or deleted if it was not there, afterwards), silent (the stand-in's stub sound), with the scripted cursor so the real cursor
never moves, its window shown without activation and kept topmost. The input is posted to that window only; each shot is the window's own
content (`PrintWindow`), refused if another window overlaps it. The process is suspended while an input is posted and a shot read, so a
step lands on a whole update. The game's 3D flag was 1 at update 5 and at the end of every run. The original runs on the real clock (100
updates a second), so the runner records the update each step landed at and the port's script is made from those, the port running as
in the tours with a `port-3d` profile (`Is3D` 1): `vis3d.sh <tour>`. The same tours were run once more with the original in 2D on the desktop
(`vis3d.sh <tour> 2d`), to tell what the real clock does from what the 3D drawing does: a difference that the 2D run shows too is not the
3D's.

| Tour | What it shows | Pairs | Equal | Faint only | Differ |
| --- | --- | --- | --- | --- | --- |
| M | the menus and every hover, options, hall of fame, statistics, difficulty dialog | 98 | 80 | 17 | 1 |
| U | ten players, the list as it grows, delete, a player picked | 70 | 70 | 0 | 0 |
| Q | the typing test to its end | 6 | 6 | 0 | 0 |
| T | the statistics screen with a lesson played: the three views | 17 | 12 | 1 | 4 |
| P | what the coverage check added (Return in the new-player box, the player list, delete, the tutor's lesson buttons and Back To Menu, the hint box, the focus lost and regained) | 32 | 16 | 7 | 9 |
| W | a running Easy-adventure board, the sea shot every 60 updates | 20 | 2 | 5 | 13 |
| pause 300, 500, 750, 1000 | the board paused at that update: 4, 30 and 80 updates later (the sea strips, sharks, diver, text) | 12 | 0 | 12 | 0 |

Every flagged box was judged:

- *The pause tours.* The paused sea, the sharks, the diver and the text are equal. Of the 568 pixels over shotdiff's tolerance in a paused frame,
  250 are the status bar's meter edge, the faint kind the 2D tours have (404 boxes there: the port draws the bar once), and 318 are the sea's
  leftmost column, below. The sea's strips draw as the original's, so the translated `BuildWaterImage` is right as far as pictures show.
- *The real clock's differences* (same set in the 2D desktop run of the same tour, so none is the 3D's): tour M's `newuser_empty_610` (the
  caret's blink phase, 2 x 19 pixels) and the faint hover and difficulty boxes (a hover fades over several updates and the shot caught a different
  one); tour T's three bars (their growing animation reads the original's millisecond timer: 4 pairs, as in 2D); tour P's player list (its scroll
  offset after ten players: `select_nine` and four more pairs), the diver's bar at rest, the tutor's two keys one update apart; tour W's sharks,
  diver and bubbles, which move by a fraction of an update on the original's real clock (the run is 23 boxes in 3D and 42 in 2D), and
  what follows a pause and a resume on a board.
- *One fault of the port, fixed.* The original's Direct3D line drawing leaves out a line's last pixel; with 3D on the port drew the whole line,
  one pixel too long, in the tab strip's divider and in the typing tutor's lines. `TyperSharkDisplay`
  (`WinWSApp.cpp`) overrides `DrawLine` for a 3D screen: a horizontal or vertical line of more than one pixel stops one short, the other lines are as
  they were. After it every box of those lines is equal; with 3D off nothing changed.
- *Faint, not fixed* (listed under Oddities): the sea's leftmost pixel column and the 3D picture's colours, below.

**Not compared in 3D:** the abyss and bonus levels, an idle game and the boards after a pause and a resume (the real clock again), and the
original's single-precision rounding of x87 code in 3D (Twist found it in its own comparison; nothing in this game's pictures showed it). The
paused boards, the menus, the dialogs and the statistics screen cover every 3D-specific piece of drawing the port has (the sea's strips and the
lines).

### Oddities (not chased)

- *The window's last column.* Where a creature crosses the right edge, up to 10 pixels of column 639 differ between the original
  and the port, and between two runs of the port when one took a shot an update earlier. To see it: the abyss tour's shots
  `012348_hint_25` and `016800_grid`. It would take finding what in the port's screen read-back touches that column; the pixels are
  below what shotdiff counts as visible.
- *The sea's leftmost pixel column in 3D.* The port's column x = 0 (about 320 rows, one pixel wide) is up to 55 levels darker in blue than the
  original's. To see it: the 3D tour W's shots or the pause tours' `paused_*_4` (318 of the 568 pixels over the tolerance in a paused frame, the
  others being the status bar's meter edge). A candidate cause is the texture's addressing at u close to 0 in `DDInterface::DrawTrianglesTex` (the framework adds 0.5 to the
  vertices; the original's device pads where the framework wraps), a shared-code change, and the column is one pixel wide.
- *The 3D picture's colours.* Flat areas of the sea differ by 0.5 levels on average and at most 6 to 9 levels of 255 between the original in 3D
  and the port in 3D, under the tolerance. The original's 3D picture is 2 to 3 levels brighter than its own 2D one, a sign of 16-bit textures
  on its side; the cause is not proven and nothing is done.
- *The custom cursor in a shot.* The original's capture holds the cursor the game draws itself when "Custom Cursors" is on; the
  port's shot does not. The menu tour turns the option off again after the options shots. A spec for a shot that includes the
  cursor is with the coordinator.

### Deviations that remain

Each is without a visible effect and is listed with its reason in `GAME_NOTES.md`, section 5: the memory leaks fixed, loading in one
go, the frame drawn whole (the two faint kinds above), the three fixes of phase 1 that mend a level that could not end, and the
registration code that is gone.
In 3D, the sea's leftmost column and the picture's colour offset above, and the line change of this section (the last pixel of a line),
which is the original's own and without a visible effect.

### For the user's review

Nothing: no pair is left that the session could not judge.
