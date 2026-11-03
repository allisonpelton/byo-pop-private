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
| 1 | The loader, the main menu, the mode select, the seeded Classic game's board and its first level-up | the same (shotdiff shifted the loader by 3 pixels to align it; unshifted, its logo matches), apart from the rotated draws below |
| 2 | A new player typed in, the menu, high scores (both tables), statistics, options, the three help pages, the quit question, the player list and deleting a player, the mode select and the tutorial's first tips | the same; the help pages' animations and the cursor spiral differ only at their edges (below) |
| 3 | The seeded Classic game played on to update 29,000: level-ups, special gems, the bomb wheel's spin | the same once the wheel's rings were fixed; the wheel's lower glass is the rotated-draw item below |
| 4 | Blitz from its start to the explosion at its end, the statistics, the high scores and the next game | from the explosion on every screen identical pixel for pixel; in play only the effects of the autoplay's jumps |
| 5 | Losing and regaining the focus during Classic and during Blitz | the same mute and the same pause screen (below) |

The sound was compared by its state rather than by ear. origrun's DirectSound stand-in logs every buffer the original
plays, with its volume, pan and frequency, and a small proxy for BASS (tools/bassproxy) logs its music; tools/soundcmp.py
pairs both with the port's own sound log. These runs were in real time, because under `-fast` the port's mixer still
plays in real time and a voice outlasts its updates. In the seeded Classic game both sides play the same 425 sounds in
the same order; all 425 have the same frequency, 422 the same pan and 419 the same volume. In Blitz both play the same
465, all with the same volume and frequency and all but seven with the same pan. The music matches event for event: every
track starts at the same order and update, every stop, and the volume steps of each fade (99, 99, 98, 98, 98, 97 ...).

The web build was run under Node in real time on the Blitz set and compared with the native build. The boards and every
screen from the explosion on are identical. The explosion's shards and smoke drift a little, from libm's last bits.

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

### Oddities left alone

Nothing new: the effects that differ in the autoplay games come from the original's 15-update jumps already listed in
phase one.

### Differences that remain

- **Rotated and matrix draws in 2D.** In 2D the original draws these in software (BltRotated, 0x41c5c0, and its
  triangle rasteriser for BltMatrix), while the port draws them on the GPU with linear filtering. Their edges differ by a
  level or two over a pixel: the cursor spiral, the help pages' animations, and the bomb wheel's glass, whose lower
  slices are opaque in the original and see-through in the port. Fixing it is the same kind of work as the stretches: the
  framework would rasterise such a draw in software for a game whose screen was software. It is left for that shared
  change.
- **The unfocused stretch under origrun.** While the original is unfocused, origrun sees its update counter stand still
  (on the real clock too) while its screen still changes, so those screens cannot be lined up by update. What was
  compared there matches: the mute, the hidden board and the pause text.
- **The web build under Node** samples rotated images without filtering, because Node gives SDL no WebGL; a browser
  filters them as the native build does.

### Deviations that remain

The deviations in NOTES.md's "Deviations" are unchanged. The pass added none.
