# Bejeweled Twist: the identical-play pass

This is the report of the pass that compares the port with the original exe (popcapgame1.exe from the Steam copy). It says
what was compared and how, what the pass changed, what it left alone and why, and what still differs. Phase one, "plays
identical", is done; phase two, "looks identical", follows and will be added here.

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
