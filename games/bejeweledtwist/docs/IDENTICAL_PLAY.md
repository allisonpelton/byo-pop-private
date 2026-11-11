# Bejeweled Twist: the identical-play pass

This is the report of the pass that compares the port with the original exe (popcapgame1.exe from the Steam copy). It says
what was compared and how, what the pass changed, what it left alone and why, and what still differs. Phase one, "plays
identical", and phase two, "looks identical", are both below.

## Phase one: plays identical

### What was compared and how

The port ran its own autoplay (`-autoplay=<mode> -seed=<n>`) under rundesk with a fresh user folder. The original ran from a
copy under build/ with its own autoplay switch, passed by origrun's new `--arg` (commit 20ac9d8), and with origrun's patch
list for Twist, which pins every random seed to the port's (the new board's generator, the global MTRand and the CRT's
srand). Before each original run a small wrapper backed up Twist's registry key and profile folders and cleared them, so the
original also started as a first run with a fresh profile, and put them back afterwards. That matters: a player's first game
plays differently (the points' smoke is slower until the 13th good twist), and an earlier comparison against a used profile
differed for that reason alone.

Both sides were compared by the game's state, not by pictures. origrun's `--watch` read the original's memory on every
update: the score (board +0x168), the level (+0x158), the board generator's position in its Mersenne Twister and the game
mode. The port logged the same four values from its update loop. A small script paired the two lists value by value and
reported each update where the timing changed.

The games compared, each about 22,000 to 23,500 updates (close to four minutes of play), in 2D mode:

| Game | Result |
| --- | --- |
| Classic, seed 1234 | identical: the same boards, draws, scores and level-ups, update for update, apart from the original's own 15-update jumps (see "Oddities") |
| Classic, seed 99 | identical apart from two of those jumps in the original |
| Zen, seed 1234 | identical update for update to the end |
| Blitz, seed 1234 | identical update for update through three games: each ends and the next begins on the same update (8223 and 9200, 16095 and 17072) |

3D mode was compared first, with the original on the visible desktop through a script that only posts input to its window.
After the fixes below, the seeded 3D game drew the global random numbers call for call like the original through update
10282. There it parts, because in 3D the original's x87 runs at single precision (see "Oddities"), so 2D is where the
series was played.

### Changes the pass made

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| Everything after the main menu ran one update early: the new game, the hyperspace jump, the draws | the menu opens the mode select during its own update and puts it behind itself, which gives the mode select the menu's z-order, so it is first updated in the next frame | the framework's PutBehind and PutInfront can give the widget the reference's z-order first (a switch Twist turns on), so the mode select waits a frame as in the original | bb6113d (shared), a7219ee |
| The level-up's "speed" text started its sweep one update late in 3D | at a scale of 0.64 the sweep's step is -50 times the scale; at the x87's single precision the product is -32, not the -31.99999928 that truncates to -31 | in 3D the step is rounded to a float before it is truncated | 07e0e77 |
| Some match points reached the score up to 31 updates early | when a cascade's first match scores while other smoke is already on its way to the tube, the original makes the newest smoke wait 0.1 and slows it to 0.35 of its hurry, and gives the floating points a delay of 15 updates per match | the same (the port had left both steps out) | 7caeee5 |

### Oddities left alone

Each is written up under "Oddities" in NOTES.md, with how to see it.

- **The original's autoplay sometimes moves 15 updates late or early.** The same seed in the same copy gives runs that
  differ from each other by 15 updates over a stretch of moves. The port matches the original wherever the original does
  not jump. The cause comes from outside the game's own updates (it changes from run to run), so it was not chased.
- **3D mode computes at single precision.** The original's Direct3D device leaves the x87 at 24-bit precision. The port
  follows it in the three places found to decide play early on (curves, a curve string's numbers, the speed text), but
  not everywhere: native float math is SSE and the web build is WebAssembly, and neither has a precision control. A
  BlobDrift blob's drift adds up its rounding and re-enters one update apart at 10283 in the seeded 3D game.
- **A coal gem's embers start from the heap.** The original never sets a piece's update counter, so a coal gem lights its
  embers on updates that change from run to run. The port starts the counter at 0.

### Deviations that remain

The port's deviations from the original are listed under "Deviations" in NOTES.md; none of them changed the play in the
games above. The pass added none.

## Phase two: looks identical

### What was compared and how

The original ran in origrun's Windows Sandbox from a copy under build/, in 2D, on origrun's virtual clock, with the same
seed and patch list as in phase one, and took its screenshots at update counts. The port ran under rundesk with a fresh
user folder and took its own one update later, which is where the two line up. tools/shotdiff compared each pair, and
every box it flagged was looked at in its crops and either fixed or explained. Where the original's autoplay had made one
of its 15-update jumps, the port's shot was also taken 15 and 30 updates either side, and the best of those was judged.

Five sets of screens were played:

| Set | What it covers | Result |
| --- | --- | --- |
| 1 | The loader, the main menu, the mode select, the seeded Classic game's board and its first level-up | the same; the loader's device slides in on the loading thread's progress, so where it stands at a given update varies from run to run |
| 2 | A new player typed in, the menu, high scores (both tables), statistics, options, the three help pages, the quit question, the player list and deleting a player, the mode select and the tutorial's first tips | the same: all 18 screens within shotdiff's faint threshold once the rotated and matrix draws were done in software |
| 3 | The seeded Classic game played on to update 29,000: level-ups, special gems, the bomb wheel's spin | the same once the wheel's rings were fixed and the rotated draws done in software; elsewhere only the effects of the autoplay's jumps |
| 4 | Blitz from its start to the explosion at its end, the statistics, the high scores and the next game | from the explosion on every screen identical pixel for pixel; in play only the effects of the autoplay's jumps |
| 5 | Losing and regaining the focus during Classic and during Blitz | the same mute and the same pause screen (below) |

The sound was compared by its state rather than by ear. origrun's DirectSound stand-in logs every buffer the original
plays, with its volume, pan and frequency, and a small proxy for BASS (tools/bassproxy) logs its music; tools/soundcmp.py
pairs both with the port's own sound log. These runs were in real time at first, because under `-fast` the port's
mixer then played in real time and a voice outlasted its updates; with the mixer on the game's clock (a86c720) a `-fast`
run gives the same. In the seeded Classic game both sides play the same 425 sounds in the same order; all 425 have the
same frequency, 422 the same pan and 422 the same volume (419 in real time). The few left over are a voice's end
falling an update apart, which moves the ramp of the other sounds under it, and the autoplay's jump near update 8407. In Blitz both play the same
465, all with the same volume and frequency and all but seven with the same pan. The music matches event for event: every
track starts at the same order and update, every stop, and the volume steps of each fade (99, 99, 98, 98, 98, 97 ...).

The web build was run under Node in real time on the Blitz set and compared with the native build. The boards and every
screen from the explosion on are identical. The explosion's shards and smoke drift a little, from libm's last bits.
After the software draws below, set 2 was run on the web build under Node too: 12 of its 18 screens are within the
faint threshold against the original, and the other six differ only in a few particles that stand a fraction of a
pixel apart (libm again).

3D mode was compared screen by screen as well, with the original on the visible desktop through visrun (the user's leave
for this comparison, 2026-11-02; Windows Sandbox gives it no Direct3D), PrintWindow shots of its own window only, and the
port under rundesk with `-3d`; both windows 1228 by 768, the screen's aspect. Phase one had compared the hyperspace
jump and the first board (updates 1000 to 3000), which match to within a few levels per channel. This pass added set 2
in 3D: the new player, menu, high scores, statistics, options, help pages, player list, mode select and the tutorial.
With the port's shot three updates after visrun's, the tutorial's board is identical pixel for pixel. The menu scenes
keep a difference of one to seven levels on average at every offset, all of it in things that move on their own clock
in a real-time run (the nebula behind the menus, the mode select's sparkles and pillars, panels and buttons in mid-
slide) and in button highlights, because visrun posts its mouse moves to the window but does not script the original's
GetCursorPos, which then sees the user's own cursor. Not compared in 3D: the level-ups and the bomb wheel, Blitz, the
focus and the sound. A long 3D game cannot be lined up past update 10282 anyway, where the original's single-precision
x87 parts from the port (see "Oddities" in NOTES.md).

### Changes the pass made

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| A praise came up to 20 updates late, and sounds did not dip under a voice | UpdateFrames ends with 0x75f1df, which lets the voice go once it has spoken and ramps the other sounds down under it and back up after | the same (UpdateVoice) | 1712d7e |
| Scaled images in 2D were blurred, the board's "GO!" among them | in 2D the original's screen is software: a stretch averages (or samples in 16.16 steps when fast) into an image of the destination's size, which is then drawn unscaled (0x41a3e0) | the framework can stretch as a software screen did (an opt-in Twist turns on) | 5dfea01 (shared), 97d664a |
| 2D overlays were blended as in 3D | Graphics::Is3D is true only on the 3D display; the port's said yes for every screen draw | Is3D also asks the display, which Twist marks as not 3D in 2D | 225bd46 (shared) |
| A flame gem's flames were thin | DrawEffects draws an effect tied to a piece at the piece's place (0x6f6451); the port skipped them | the same as the original | 8b66c23 |
| The bomb wheel's alert and danger rings never showed in 2D | the 2D curve's text was not in the port's table of the exe's strings, so the curve was empty | the table has it, and the rings show through the spin | e56b55d |
| Half the sounds were up to 0.8 dB off, and high chain sounds played above 100,000 Hz | this DSoundInstance converts each volume factor to dB on its own and adds them (0x4fa9e0, 0x4f85b0), and sets a whole-Hz frequency capped at 100,000 (0x4fabb0) | the same, behind a switch of the framework's sound manager that Twist turns on | 5270a2f (shared), 6ae2713 |
| Some panned sounds were one step off centre | PlaySoundAt stores its pan as a float before cutting it to an int (0x767643) | the same | 6ae2713 |
| Losing the focus did not pause the board, and a paused Blitz game showed no "PAUSED" | the app's LostFocus and GotFocus (0x75f51e, 0x75f4f7) pause and unpause the board; DrawPaused (0x756925) draws the pause text | both ported | 7e6e14b |
| In 2D the cursor's spiral, the tutorial's arrows, the help pages' animations and the bomb wheel's glass were drawn on the GPU with SDL's filtering, and the tutorial's arrows went missing once SWTri drew | the software screen rotates with its own bilinear code (DDImage::BltRotated, 0x413150), draws matrices and triangles with SWTri (0x4355e0), blits at a fractional place as a rotation at no angle (0x412fd0), and its constructor registers SWTri's drawers (0x50f7e0) | the framework can draw these in software into a copy and blit it (an opt-in Twist turns on in 2D), and Twist registers the drawers; SWTri's blend no longer divides by zero onto a transparent pixel | 105440e, f8624d2 (shared), 679953e |
| A rotated cel below its sheet's first row, such as the shadow of a geode's photon arms (cels 2 and 3 of a 2x2 sheet), was read from too high in the sheet once the rotations were software | the screen's BltRotated (0x413150) finds a cel's first row by the image's width; only the original's MemoryImage::BltRotated (0x431160) keeps 1.3's offset by the cel's own width | the software copies read rows by the image's width (an opt-in; other games keep 1.3's reads). Settled from the export: set 3's port shots change only where a geode bursts, and no shot of the original lines up with one | ffbf353 (shared) |
| A start without "Is3D" was 2D, and a saved 3D on a display without WebGL2 or a GL context ended with "Failed to initialize the display" | the start-up (0x762da8) chooses 2D only for "Is3D" false; otherwise DDInterface::Init (0x504ce0) sets up Direct3D and calls 0x7606de, which chooses 2D mode silently when the display came out without it, writing nothing | 3D is the default, and a GPU display that can't be made makes the window again in 2D (DisplayInitFailed); the options' 3D box erases "Is3D" when turned on, as the original does; test runs ask for their mode | this commit and the shared one before it |

### Oddities left alone

Nothing new: the effects that differ in the autoplay games come from the original's 15-update jumps already listed in
phase one.

### Differences that remain

- **Additive matrix and triangle draws in 2D** still go through SDL: the framework's SWTri (PopCap's 1.3) has no
  additive triangles, where Twist's later one does (0x60d250's flag 0x80). None of the compared screens showed one.
- **The unfocused stretch under origrun.** While the original is unfocused, origrun sees its update counter stand still
  (on the real clock too) while its screen still changes, so those screens cannot be lined up by update. What was
  compared there matches: the mute, the hidden board and the pause text.
- **The web build under Node** places a few particles a fraction of a pixel apart from the native build, from libm's
  last bits.
- **3D in real time.** visrun runs the original in real time and does not script its cursor, so the menus' own-clock
  animations and the button under the user's cursor cannot be lined up with the port in 3D (above).

### Deviations that remain

The deviations in NOTES.md's "Deviations" are unchanged. The pass added none.

## Real play (2026-11-07)

The user played the port in the browser and natively and listed what went wrong; most of it was 3D, which the
comparisons above had run in 2D. This pass played the native game in 3D full screen with real input only: posted
mouse and key messages under rundesk, by `tools/realplay/drive.py`, which reads the board from screenshots and
chooses its twists itself. It played launch, the Classic tutorial and a Classic game to level 4 (about 370 twists,
31,050 points, past two bomb tips, the coal and lock tips and a bomb wheel), the pause menu, the quit question and the
way back to the main menu, the high scores' stats page and a best move's replay, minimising and restoring, and the
options' resolution change. The browser half (hover and twist in the Classic tutorial, the resolution change) was
played with DOM events in the Browser pane at `?silent=1`.

### Changes the pass made

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| In 3D no gem could be twisted after the first level-up, and the cursor stopped following the mouse (the browser fault) | space's state 1 clears the board's +0x55 and state 5 sets it again (0x715c21); +0x55 is the widget's mMouseVisible, which the port had read as mDisabled | the same field | ae814617 |
| The pause menu's Quit and other zooming panels vanished instead of shrinking in 3D | TwistWidget::DrawAll (0x755cd2) zooms through the renderer's transform about the screen's middle when Direct3D draws; the port zoomed only in 2D | the same | ae814617 |
| The end of a game took the 2D path in 3D | StartEndGame (0x714883) fades space in over the board only without Direct3D | the same | ae814617 |
| The options' resolution did nothing | 0x6a006a keeps the new art size, writes "ArtRes" at shutdown and restarts the game; the port's art size was fixed | the same, natively and on the web | ae814617 |
| A best move's replay from the high scores showed the score meter without its top, glass and pod in 3D | the tube's constructor clears its mClip (+0x41); with Direct3D the clip rectangle is in the screen's pixels after the replay board's 0.4 scale, so a clipped tube loses whatever it draws with a rotation or a stretch | the tube does not clip (in 2D a few pixels below it change and the suite's frozen screens were taken again, and the frozen baseline exe at the merge c6915782) | ae814617 |
| Alt-Tab out of full-screen 3D crashed | while the window is minimised no scene may begin; every draw fell to the software path and read pixels that live only on the GPU | a GPU display skips its frames while minimised | ba4b72b0 (shared) |

### Looked at and left

- **The coal tip's black box.** The tip's animation (twistpam.pam, label "coal") opens and closes each loop with a
  black curtain; a screenshot caught it shut. Over a few seconds it plays as it should.
- **The board after quitting.** For about half a second after the quit question's Yes, the board shows faded over
  space before the main menu comes in. Not yet compared with the original, which needs its 3D run (`docs/ORIGRUN.md`).
- **The windowed-mode button in 2D.** In 2D full screen the board redraws the whole screen and the button in the
  corner shows only in frames where it was redrawn after the board. Since the tube stopped clipping, three of the
  suite's Blitz frames lose it as its Classic frames already did. To compare with the original in 2D.
- **The black screen in full-screen Classic** was not reproduced. The likeliest causes were the crash above (a
  minimised full-screen game) and the 3D mouse, both fixed.
- **The Zen tutorial's background** shows natively in 3D. The browser's Zen was not played yet.

### Played by the port's own bot

Later the same day the driver stopped choosing twists itself: its `play` step reads what the game prints for the test
commands `state` and `move` (asked by a `-script` every 80 updates) and sends each twist as posted mouse messages, the
hover and then the click. `goto <mode>` starts the game through the menu's own Play and mode buttons. Three whole games
were played this way, with no script click and no autoplay:

- **Blitz in 3D full screen**: 87 twists and 100,875 points, until a bomb's counter reached zero with 3:08 left on the
  clock. That ends Blitz (a counter at zero ends Blitz and challenges, where Classic spins the wheel), and the game's
  stats page followed.
- **Zen in 3D full screen**: to level 4 and 24,650 points with the Zen background. Then the tube's menu button, Main
  Menu and Yes in "Save and Quit", which brought back the title screen.
- **Classic in 2D windowed**: to level 4 and 28,750 points. Then a bomb's wheel, which `move` stops as -autoplay does,
  landed on a skull, and the game ended at "Rank Up!".

Two faults of the test path were fixed on the way: `goto <mode>` called the menu's StartGame directly, which left the
play portal over the board and Blitz's clock stopped (mPlayPressed unset), and a `-script` run had dropped the posted
input, then let focus changes pause the game (shared commits 10a79f05 and a3b6df9c).

## Finishing (2026-11-08)

### Real play on the live channel

The drivers moved to the live channel: the game runs with `-commands=<file>` and an empty `-realinput` file, and every
line the driver appends is either a test command (`state`, `move`, `goto`, `unlock all`, `level`, `lose`) or a real
event that goes through SDL's own queue (`move`/`click game x y`, keys, focus, minimise, restore, Alt+Enter, close).
Natively that is `tools/realplay/live.py` under rundesk; on the web, `tools/web/onegame.js` in the player page
(`?player=1&assets=...&fast=1&hold=1&driver=...`), whose clicks are the page's own mouse events. The old driver that
posted window messages (`drive.py`) is retired. The bot chooses each twist and answers each screen through `move`
(the tutorial's forced twists and dialogs, the tips, the wheel's stop, the rank bar, the zen jump, the stats and the
high scores); no autoplay and no script click is involved.

Natively in 3D, from a new player's first launch: the name typed in, the Classic tutorial with its three forced twists,
then Classic through a focus loss and a minimise, two second-chance wheels, the zen jump question (answered Classic)
and level 12, ended through the game's own ending (the `lose` command, since the bot defuses every bomb and Classic
seldom ends by itself), its rank bar, stats and the top-ten high scores' Main Menu; then Alt+Enter into full screen,
Blitz to its end and stats, Zen to level 5 and out by clicks on the tube's menu button, Main Menu and Yes in "Save and
Quit", and the window closed with exit code 0. That was 491 twists.

Natively in 2D (`-nod3d9`), again from a new player: the name and the Classic tutorial, then Alt+Enter so that Classic
played in full screen through its wheel, the zen jump question and its end, its stats and high scores; Alt+Enter back to
a window, Blitz to its end, Zen for 100 twists and out by the same three clicks at the 2D screen's points, and the window
closed. That was 270 twists, exit code 0.

### Real play on the web

`tools/web/onegame.js` plays one game of a mode in the player-only page from the bot's answers, as the page's own mouse
and key events, and reads every answer from the page's log itself (the test kit's own wait for an answer missed some in
a hidden pane). Each run went to the main menu at its end with no error in the page:

| Mode | 3D or 2D | Canvas | What it played |
| --- | --- | --- | --- |
| Classic, new player | 3D | 2456x1536 for 1228x768 (`scale=2`) | the name, the tutorial, two wheels, the zen jump question after `level 10`, its own end before the bot reached 300 twists, the end screens: 195 twists |
| Zen | 3D | 1228x768 | 2153 twists, out by the tube's menu, Main Menu and Yes |
| Blitz | 3D | 1228x768 | to its end and its screens: 75 twists |
| Classic, new player | 2D | 800x600 | the name, the tutorial, a wheel, the zen jump question, its end: 180 twists |
| Zen | 2D | 1600x1200 for 800x600 (`scale=2`) | 902 twists, out by the same three clicks at the 2D points |
| Blitz | 2D | 800x600 | to its end: 109 twists |

The Browser pane cannot enter full screen (its requestFullscreen() is refused), so `?scale=2` draws the canvas at twice
the game's size instead, and every click lands at a scaled point as it would in full screen. 2D came from the site's
"3D acceleration" set to off for Twist, 3D from its default on a browser with WebGL2.

The first web runs in 3D found a fault of the page, not of the game: the bot's twists in 3D turned the square one row
above the one it chose, and a Zen game made 1121 twists for 475 points. SDL sizes the canvas to the page's stage only
on the window's resize event, and the stage takes the game's shape by its style alone when the game gives its size, so
the canvas kept SDL's first 640 by 480 while the stage was 1228 by 768. Every click's height was then mapped through a
4:3 picture letterboxed into a 16:10 one. 2D's 800 by 600 has the 640 by 480's own shape, so it was never off. A player
in 3D on the web would have met it as well. The page now sends a resize once the game has given its size (a shared
change to `web/shell.html`), and the canvas is 1228 by 768 from the start.

### Performance

NOTES.md, "Performance (native)", has the numbers: the port reaches its menu in 2.7 seconds against the original's 10.8,
and on the menu used 3.3 seconds of processor time in a minute against the original's 21. Its peaks are 213 MB on the
menu and 398 MB in Classic, the original's 316 MB on its menu. The original was measured only on its menu, since its own
autoplay crashes in this copy. The web build's numbers are under "Performance (web)".

### Changes this part made

| Difference | Cause | The port now | Commit |
| --- | --- | --- | --- |
| A focus loss during a level's jump crashed the game, or left the next level's board empty for good | the board's update holds while it is paused, but the original's Hyperspace widget ran on and passed the row count that ends the jump | space holds still while the board is paused, and the jump goes on where it was (NOTES.md, "Deviations") | 64f450e5 |
| The doom gem's placement could loop for ever and freeze the game | NextTurnGems picks random inner cells until one qualifies, with no limit | when no inner gem can take it, no doom gem comes that turn; otherwise the same picks and draws | this pass |
| Loading took about 8 seconds whatever the machine | the loader waited for its smoothed bar | the menu comes once the files are loaded | 0462fe93 |
| After the faster loading, the PopCap logo faded in behind the menu and the menu's Play button stayed invisible until clicked (the user's report) | the port handed over a few updates into the loader's opening, whose curves the original has played out by its hand-over at update 848 | the loader's clock moves on to update 848 at the hand-over; check.sh freezes the menu with Play drawn | this pass |
| Registration code and the "More Games" button | the trial, nag and buy paths | taken out (NOTES.md, "Registration and outbound buttons") | fe0f06bc |
| On the web in 3D every click landed a row too high | the page's stage changed shape by its style, which SDL never hears of | the page sends a resize once the game gives its size | this pass (shared) |
| The sound effects sounded distorted (the user's report): every match's gem_hit came out a dull thud at a fifth of its level, and lightbar_blink and bomb_explode lost their highs | the port's Ogg decoder, stb_vorbis, decodes Vorbis residue type 0 (these three files, the only ones from a 2001 encoder) with a wrong length, so only the lowest band of each block is heard | measured with -audiocapture against a re-creation from ffmpeg's decode (NOTES.md, "Sound"); the one-line fix is in shared code, sent to the coordinator; `bjt_sounddecode --compare` catches it | shared, pending |
| A game left on the web was lost: two levels played, then another game opened (the user's report) | a page never shuts down, so it keeps the files a shutdown would write by asking the game (GetShutdownSaves), which Twist did not answer | Twist gives the save its quit would write, byte for byte, and the profile (NOTES.md, "Saving on the way out"); check.sh compares them with a quit's and resumes | this pass |
| The level-up's take-off looked wrong in 3D (the user's report: "a lot happening, all very fast", the meter's landing fine and its take-off not): large white slabs swept over the screen for about two seconds while the board left | the old level's picture leaves on a glass cube that moves right, down and away from the camera; the original passes its three moves to Translate z first (0x712808 to 0x712849), and the port had read them as x, y and z, so the cube came towards the camera and through it, its outer rainbow pass filling the screen | the cube moves as the original's; compared frame by frame with the original in 3D on the user's desktop (NOTES.md, "3D mode"), the take-off, the flight and the landing match; check.sh freezes five frames of the take-off; the 2D transition already matched the original's by shotdiff, apart from the gem souls' paths, which follow the update count since launch | this pass |

The fixes mod (`mods/fixes`, NOTES.md "The fixes mod") holds one fix, `rewindSkips`: a best move's replay hides the
board's float, mirrored and smoothly turning pictures while it rewinds, as it hides the rest. The port now loads
mods (`-modsdir`, `-nomods`), and the web page installs the fixes mod on.

The test path itself was fixed on the way: `move` counts a locked gem in a line (it had read it as a gap, so the bot
found no twist where one existed), answers a tip's dialog over the wheel before the wheel, answers the zen jump and the
top-ten high scores' bar, and the state's answers go through SDL_Log, which a web page reads line by line. Where no
twist makes a line, neither the bot's grid nor the board's own FindMove (its answer from the last settle), `move` turns a
square that changes the board without a line, the one the level's twist count picks among those that can turn, so that
it never cycles one square through four turns; the game's own autoplay would draw a random square from the game's
generator there.


## Challenge mode against the original (2026-11-11)

### What was compared and how

Phase two had compared challenge mode only up to the start of a challenge. This part compared five whole challenges in
2D: a win that opens the bar's next stage; the Tube Menu and Restart Challenge, then a timed win; a three-minute
highest-single-move challenge; a one-move challenge; and a timed challenge lost to its game over. Each ran
in the original under origrun (seed 1234, 2D) and in the port under rundesk, and the shots were compared by
shotdiff and the sound by soundcmp. The 3D load, the dialogs, the win and the bar were compared on the visible desktop,
which the user allowed for Twist's 3D ("3D testing for Twist approved"). Three short silent runs did this on
2026-11-11, between 03:19 and 03:46 local time, with short probe runs between 03:12 and 03:30. Each left nothing running
and put back the game's registry key and the users folders. NOTES.md's "Challenges against the original" has the method.

### Changes this part made

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| The challenge bar's opened lock and new number moved at the wrong times | the lock wipes by the curve at +0x248 (0x6df324) and the new number fades by 1 minus the one at +0x1d8 (0x6df21c); the port had them swapped | the same as the original | this pass |
| A Twist dialog's button never looked pressed or hovered | DrawImageBox takes the whole cel (0x422980), and the button image has one column of three rows; the port took the cel modulo the columns, always 0 | the same as the original | this pass |
| "CHALLENGE COMPLETE" zoomed one update ahead in script shots | its curve counts the app's updates, and a script's shot ran after the count moved (a test artefact, not seen in play) | the framework's mid-update shots draw with the count one lower | 85bf301c, 952429de (shared) |
| The board's picture at a challenge's end drew a black square in 2D | the original's software stretch (0x41a3e0) blits with the draw's colour and additive mode; the port's dropped both | the same as the original | 85bf301c (shared) |

### Oddities left alone

None new. Smoke, sparks, the flame ring and the game-over gems' fall come out differently in every run, because they
draw from the global Rand, which in the original moves from run to run: a coal gem's ember counter starts from the heap
(already under "Oddities" in NOTES.md). The boards come from the board's own generator and match in every run,
through the game over. The port's `-fast` clock plays no part: the lost challenge rerun in real time gave the same
pictures, pixel for pixel.

### Differences that remain

- The original's mouse pointer is in its shots; the port's are taken without one.
- The picked planet's name seems to flash in over five updates. The original draws nothing for six updates after the
  click's slow update, catching up, so the same picture stands while the port's frames move on.
- In 3D on the real clock: the nebula turns on its own clock, and once the screenshots have slowed the original its
  clicks land a few updates off, so its later boards part from the port's. By state (visrun's peeks), the load matches
  update for update.
- The 3D performance pairs were not taken: a real-time visible run gives no clean timing.

### Deviations that remain

None added.
