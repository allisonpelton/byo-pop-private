# Heavy Weapon Deluxe: the identical-play pass

This is phase one of the pass that CLAUDE.md names, **plays identical**: with the same input, the original and the port
play the same games. The test is the game's state, not pictures. Phase two (screens, sound, focus, the web build) has not
started.

## What was compared, and how

**The original** is the Steam copy's game (`re/heavyweapon/bin/HeavyWeaponGame.exe`). It runs only through
`tools/origrun`, from a copy under `build/origrun/heavyweapon/game`, on origrun's virtual clock with the scripted cursor and
keys, its HKLM settings mapped to HKCU. `docs/GAME_NOTES.md`, "The original under origrun", covers what that needs: the
focus, the eleven re-encoded pictures and seven patches.

- The first patch seeds `Rand()` and `srand` from the run's seed, as the port's `-seed=N` does.
- The second makes the loading run on the main thread, as the port's does.
- The third keeps the wall-clock `WM_TIMER` from deciding at random updates that the mouse has left the window.
- The fourth gives the original the refresh rate the port's framework has, 100 Hz, in place of origrun desktop's 60
  (below, under "The refresh rate").
- The fifth to seventh start three values the original leaves unset as the port starts them, at 0: an enemy's place,
  which the robot reads before it sets its own; the robot's foot, whose stray value had the robot stamp as it came in;
  and the head's target x, which the head eases toward from its first update (below, under "Oddities").

**The port** is `build/native/heavyweapon.exe` with `-fast` and the same seed, under rundesk, from an empty user folder
or a players file the port made.

**One input for both.** `games/heavyweapon/tools/idplay/idplay.py` records a scenario's input in the port first. A fixed
opening covers the title, a typed name or a players file's continue box, and the main menu's button. After it, the
port's own bot (the `botrecord` test command, which records what the `move` command's bot does) plays on, through mouse
moves, presses and clicks, until its last tank is lost; the frames compared end 500 after that. Then:

- The original plays that input under origrun, which reads its state at each of its waits.
- The port replays the same input with its `trace` test command writing the same state after every update.
- The two are compared at every update the original was read after.

Both are counted from the update whose end first finds the loading done, 52 in both. The original is given the click
that ends the intro at update 50 and the port at 51, since the port takes a script click in the update it is given and
the original reads a posted one before the next. Nothing draws a random number before that, but the game reads the
app's update count, so it must be the same at every frame: with the port's click at 50 the two anchors were 51 and 52,
and the Eye, which picks a new place to drift to whenever the count is a multiple of 50, did so an update earlier in the
original. The final boss retargets the same way, and the head's bombs and the worm's sand go by the count too.

**The state compared:**

- both random generators: the framework's `Rand()` and the game's own, each by its index and first word;
- whether a board is up; the score, level, lives and nukes;
- the board's level time, the tank's position (the double's two words), the time to the next tank;
- the kills and spawns, the enemies' count, the boss and its health (a double);
- the board's mouse position and button;
- the counts of its bullets, explosions, projectiles, craters, special shots, shell casings, sparks and particles;
- the places (the high word of each coordinate's double) of the first three bullets, enemies and projectiles in the
  board's lists, so that a shot or an aircraft that starts in the wrong place shows at once, not when it first hits; and
  whether each of those projectiles is the tank's lightning (`StaticStrike`, by its vtable in the original), whose place
  is never set and counts as 0.

Any stray draw from either generator shows in its index at once, so a game that agrees to the end has made the same
draws in the same updates throughout.

**The input's timing.** Under origrun the original draws a frame every 16 ms of the virtual clock and updates every
10 ms, so it waits, and checks its messages, after five updates in eight, always the same five counted from its anchor.
origrun posts an input at the original's first wait after the update the script names, and the original reads it before
the next update. An input that the port took before update N + 1 is therefore read by the original before N + 1 only
when it waits after update N; otherwise one update later. The first runs replayed the port's input wherever the original
had read it, and the two games agreed, but the replay was then no longer the game the bot had played: its moves no longer
fitted, and the boards stalled under a play tip the bot never saw. So the record run asks the bot, and gives the
opening's input, only before an update that follows one of the five waits (`botrecord`'s period and residues), and
the original reads every input before the same update as the port did. `idplay.py` still measures the update each input
was read before, from the WidgetManager's last-input count, which origrun peeks with the rest, and checks the replay's
trace against the record run's: in every game below the replay is the bot's own game, update for update.

**Peeks inside an update.** origrun reads the state at the original's waits, and the sound code waits as a sound starts.
A peek taken there shows the update half done: the 3 draws of a flicker starting on the main menu as 2, or a kill's
explosion before its score. Such a peek differs from the port's state for one frame and agrees again at the next.
`compare` counts these separately; a real split never comes back, since the random draws part for good.

**The refresh rate.** The game sets `mVSyncUpdates`, so `SexyAppBase` moves the enemies once a drawn frame, in
`UpdateF`, by 100 / (the screen's refresh rate) updates' worth; the original takes the rate from DirectDraw (0x44d770).
The port's framework always has 100, one `UpdateF(1.0)` after each update. On origrun's desktop the original had 60, a
step of 1.667 updates after every one or two: the enemies stood elsewhere between the steps, and the games parted at the
first random draw that depends on an enemy's place (an aircraft that fires once it is in range, in the first level). The
fourth patch gives the original 100, the rate of a 100 Hz screen, and the games then agree. The original itself plays
other games on screens of other rates; the port plays as the original does on a 100 Hz screen, on any screen (a
deviation in `docs/GAME_NOTES.md`).

## The games

Twenty-eight games, each its own scenario in `games/heavyweapon/tools/idplay/scenarios`, and in each the bot played until
its last tank was lost:

- two first runs (`campaign` and `campaign2`, seeds 1 and 2): the new player's name typed, the help before the first
  game, level 1 with its boss, the results, the armory and level 2;
- survival (`survival`), by a player whose modes are all unlocked, with the help that comes before a first survival;
  survival scores no points;
- the campaign continued before levels 4, 7, 10, 13, 16 and 19 (`level4` to `level19`, the level's number as seed),
  from a players file the port saved there with `set progress`: the continue box, the armory, where the bot spends the
  level's points, the map and the level;
- the nineteen bosses (`boss1` to `boss19`, seed 100 plus the level's number), the first by a new player and the others
  from a players file saved before their level, with every level 1200 updates long by a copy of `data/levels.xml` that
  both games read, so that the boss comes 600 updates in. Every boss came, between updates 2024 and 2990, and was hit;
  the bot beat none.

The frames compared are the updates after which the original waits, five in eight, from the anchor to 500 updates after
the bot's last tank; the bracket gives the last. "Tank X last bits" counts the frames where only the tank's X differed,
in its last bits.

| game | start | seed | frames compared (to frame) | differ | tank X last bits | at the end |
| --- | --- | --- | --- | --- | --- | --- |
| campaign | new player | 1 | 12693 (20308) | 0 | 0 | level 2, score 40740, the game over |
| campaign2 | new player | 2 | 14679 (23487) | 0 | 0 | level 2, score 56875, the game over |
| survival | modes unlocked | 1 | 1973 (3156) | 0 | 0 | level 1, score 0, the game over |
| level4 | before level 4 | 4 | 3211 (5136) | 0 | 0 | level 4, score 8310, the game over |
| level7 | before level 7 | 7 | 3077 (4924) | 0 | 0 | level 7, score 4100, the game over |
| level10 | before level 10 | 10 | 3582 (5732) | 0 | 0 | level 10, score 40140, the game over |
| level13 | before level 13 | 13 | 3598 (5756) | 0 | 0 | level 13, score 32755, the game over |
| level16 | before level 16 | 16 | 3259 (5213) | 0 | 0 | level 16, score 11070, the game over |
| level19 | before level 19 | 19 | 3034 (4855) | 0 | 0 | level 19, score 29260, the game over |
| boss1 | new player, short levels | 101 | 3872 (6194) | 0 | 0 | level 1, score 360, the game over |
| boss2 | before level 2, short levels | 102 | 4369 (6991) | 0 | 0 | level 2, score 1290, the game over |
| boss3 | before level 3, short levels | 103 | 3456 (5528) | 0 | 0 | level 3, score 2655, the game over |
| boss4 | before level 4, short levels | 104 | 3545 (5671) | 0 | 0 | level 4, score 200, the game over |
| boss5 | before level 5, short levels | 105 | 3782 (6050) | 0 | 0 | level 5, score 1580, the game over |
| boss6 | before level 6, short levels | 106 | 4369 (6989) | 0 | 0 | level 6, score 1530, the game over |
| boss7 | before level 7, short levels | 107 | 3617 (5788) | 0 | 0 | level 7, score 2160, the game over |
| boss8 | before level 8, short levels | 108 | 2904 (4645) | 0 | 0 | level 8, score 1500, the game over |
| boss9 | before level 9, short levels | 109 | 2755 (4407) | 0 | 0 | level 9, score 2020, the game over |
| boss10 | before level 10, short levels | 110 | 4062 (6498) | 0 | 0 | level 10, score 10950, the game over |
| boss11 | before level 11, short levels | 111 | 4975 (7959) | 0 | 0 | level 11, score 5440, the game over |
| boss12 | before level 12, short levels | 112 | 3464 (5543) | 0 | 0 | level 12, score 40, the game over |
| boss13 | before level 13, short levels | 113 | 3424 (5477) | 0 | 0 | level 13, score 2160, the game over |
| boss14 | before level 14, short levels | 114 | 3539 (5661) | 0 | 0 | level 14, score 3610, the game over |
| boss15 | before level 15, short levels | 115 | 3246 (5192) | 0 | 0 | level 15, score 0, the game over |
| boss16 | before level 16, short levels | 116 | 3444 (5511) | 0 | 0 | level 16, score 5610, the game over |
| boss17 | before level 17, short levels | 117 | 3179 (5087) | 0 | 0 | level 17, score 7100, the game over |
| boss18 | before level 18, short levels | 118 | 3116 (4984) | 0 | 0 | level 18, score 5765, the game over |
| boss19 | before level 19, short levels | 119 | 3447 (5514) | 0 | 0 | level 19, score 3520, the game over |

Every game agrees in every compared frame: the random draws, the score, the counts and places of the shots, enemies and
projectiles, and the tank's X in every bit. Not even a peek that fell inside an update differs, and in every game the
replay is the bot's own game, update for update.

## Changes the pass made to the port

Seven to the game's play: four translation errors, the places the original leaves unset, which the port read as its
heap had them, the lightning's scaling and the x87's 64-bit sines. With them, the seven patches and the input's timing
above, the port plays every game below as the original does. The rest went to the port's test commands and its bot,
which no game reaches.

| change | cause in the original | the port now | commit |
| --- | --- | --- | --- |
| A Strafer from the left fired from 40 pixels behind itself | `Strafer::Update` (0x435190) starts the shot at its x plus the angle's cosine times 40 for one that is not mirrored (from the left), and minus it for a mirrored one; the port subtracted for both | adds for one from the left, as the original | 493f69cf |
| The robot's arms and launchers, and the tank's lightning, took their places from uninitialised memory | the `Enemy` and `Projectile` constructors leave `mX` and `mY` unset; the robot reads its own before it sets it, and the lightning never sets its own | both start at 0 (a deviation, and the original's fifth patch) | e28be03b |
| The Wrecker stood about 2e-4 pixels off for the whole fight | its constructor (0x447210) starts both swings at 1.570795; the port had 1.570796 | 1.570795 | f32a4ab0 |
| The lightning's head could land a last bit off its target | `StaticStrike::Update` (0x44aca0) takes 20 / length once and multiplies both parts of the speed by it; the port divided each by the length and multiplied by 20 | one scale, as the original | 913b6477 |
| The final boss's bomb fragments stayed three updates longer | `BombFragment::Update` (0x44ba10) keeps a fragment while its trail's oldest point is at y 460 or above; the port kept it to 472 | 460 | ed1031b2 |
| The final boss's swaying fragments drifted apart | the same function turns the sway back with a pull of 0.075 (the double 0x3fb3333333333333); the port had 0.1, its constructor's tenth of the drift | 0.075 | c66bfb0d |
| Places differed in their last bits, and the lightning hit a Strafer, and a Wrecker, an update later | the original's sines, cosines, arc tangents and logs are the x87's `fsin`, `fcos`, `fpatan` and `fyl2x`, whose 64-bit results the next operation rounds once to a double; the C library rounds them first, and the product after it differed in its last bit in about a quarter of cases; the lightning hits only what it lands on exactly | `X87.cpp` gives the x87's results in double-double arithmetic and rounds the next operation once, at every play site (`docs/GAME_NOTES.md`, "The x87's sines"); the tank's X, which differed in its last bits for stretches of updates, now agrees in every bit | 06d50abe |
| `level 19` reaches the final boss's level (the command stopped at 18) | the campaign has 19 levels | | 38f00fae |
| `trace <file>`: a line an update with the state the comparison reads | | the port's side of the comparison | a2bf406c |
| `botrecord <file> [<period> <residue>...]`: the bot's moves as script input, recorded | | one input for both games, given at updates where the original reads it | a2bf406c |
| `set progress <n>`: the player's campaign saved before level n | | the later levels' games start from such a players file | a2bf406c |
| The bot's pointer stays 8 pixels inside the field's sides | a shake moves the board up to 7 pixels; a still pointer it leaves outside the board has `MouseLeave` suspend the game, with the board still moved, so the shake never ends | a boss's games go on after a tank is lost at the field's edge | a2bf406c |
| The bot aims no higher than 40 | at 10, aiming at a boss above the field, the pointer was over the status bar (30 high), and the board's `MouseLeave` suspended the game in both | the robot's fights go on | fcac3b04 |
| The trace marks the tank's lightning, and the bot's threat search skips the tank's own shots | the lightning's place is never set (above) | | fcac3b04 |

The Strafer showed in level 13's first game, at frame 1514, where the score, the bullets, the explosions and the
framework's generator parted at once: a bullet had hit in one game and not in the other. The bullets' and enemies'
places, which the compared state did not hold then, would have shown the shot where it started; they were added to the
state after it.

The x87's sines showed in level 13 at frame 2050 as one spark, five particles and 17 random draws more in the port: its
lightning had hit a Strafer an update before the original's. The head of its path had landed on the Strafer near x 0,
where the difference of two places is no longer exact, and only the Strafer's last bits decided it. The Strafer's speed
is the x87's `fcos` times its speed, rounded once (0x435190); with that product kept in extended precision in a trial
build, the level's game agreed to its end, and the same cause was then found in level 13's boss fight on the Wrecker,
whose place is the sum of two such cosines. `idplay.py` had counted the tank's X apart when it differed only in its last
bits (at most 4096 units, all else equal), as an oddity; with the x87's results in, no game has such a peek.

The final boss's fragments showed in level 19's boss fight: two falling fragments left the original at frame 3569 and
the port three updates later, and once that was fixed, two swaying ones drifted apart at frame 4264.

The high-score lines' names, which the original leaves uncleared on its stack and the port clears (a deviation in
`docs/GAME_NOTES.md`), were found while the comparison was being set up and were already in the merged port.

## Oddities

- **The robot's foot.** The robot's constructor (0x443210) never sets its foot's y, and its first update stamps if the
  value is 410 or more: the smash sound, a shake of the screen and two random draws an update while it shakes. In the
  comparison's robot fights (levels 9 and 18) the original's heap held text there, which as a double is huge, and the
  robot stamped as it came in above the field; the port starts the foot at 0, so it never does. The games parted at
  that first update. The sixth patch starts the original's foot at 0 too, and the rest of each fight is compared. What
  the original does depends on what its heap held, which would take its allocation history to match.
- **The head's target.** The head's constructor (0x43f310) never sets the x it eases toward, and its first update moves
  the head toward whatever the heap held there. The port starts it at 0; the seventh patch starts the original's at 0
  too, so that the head's fights (levels 7 and 16) start from the same place. Matching the original would again take its
  allocation history.

## Deviations that remain

The deviations in `docs/GAME_NOTES.md` are unchanged but for the refresh rate, which this pass found and lists there;
none shows in the compared state.
