# Heavy Weapon Deluxe: the identical-play pass

This is the pass that CLAUDE.md names, **plays identical**, in its two phases. In phase one, with the same input, the
original and the port play the same games; the test is the game's state, not pictures. In phase two ("Phase two: it looks
the same", below) the same games are compared by what they show and play: the screens against the original's, the web
build's against the native port's, the sound by state, the focus, and play with real mouse and key events.

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

## Phase two: it looks the same

### Method

A tour is a scenario of `idplay.py` (`games/heavyweapon/tools/idplay/scenarios/shots_*.py`) with shots: its input is
recorded in the port as in phase one, and `idplay.py shots <tour>` plays it in both programs.

- **The original** runs inside Windows Sandbox (the only place origrun takes shots), windowed and in 2D, without the
  cursor the game draws itself (`CustomCursors` 0), on the virtual clock. Each shot is labelled with the update it
  shows: origrun is given the app's count of updates not yet drawn (0x56b580, which 0x451307 raises and 0x45136d clears
  before the draw at 0x45137d) as both the pending draw and the update state, so a shot is taken only once its update
  has been drawn.
- **The port** replays the same input under rundesk with `-fast -drawevery -renderscale=1`, in a window, in 2D and
  without the drawn cursor, and takes each shot one update after the update the original's landed on: the port's
  `screenshot` draws the state the update before it left. A shot comes before the input of its own update, as the
  original's shot is drawn before it reads the input it takes before the next update.
- **The states** are compared at every frame as in phase one, so a shot pair is only judged once both games agree.
- **The pictures** go through `tools/shotdiff` (the 640x480 client cut from the original's capture); every flagged
  box was looked at.

`idplay.py web <tour>` then plays the same script in the web build under Node (`wasmrun.js`), compares its trace with
the native port's line for line and shotdiffs its shots against the native port's.

### Tours and results

| Tour | What it shows | Frames compared | Shots | Equal | Faint only | Differ |
| --- | --- | --- | --- | --- | --- | --- |
| menus | the title and a first run's name box, the main menu with a button's description, the options box and its credits and help, the high scores (both lists), the help, the player list, the quit box | 1251 | 21 | 20 | 0 | 1 |
| in-game | the map, the way into level 1, the status bar's MENU box, its options and back, the game resumed, the focus lost and regained, the MAIN MENU's warning answered NO | 1501 | 13 | 13 | 0 | 0 |
| campaign | a first campaign by the bot: the help before the first game, the map, level 1 with its play tips, boss and results, the armory, level 2 to the game-over box | 12693 | 51 | 48 | 3 | 0 |
| middle | levels 8 to 10 with short levels and weak bosses: two boss warnings and deaths, results and armories, the middle cutscene, the game-over box | 18874 | 200 | 189 | 9 | 2 |
| final | levels 18 and 19 from a saved game: the final boss beaten, the final cutscene, the win's high score and the high scores after it | 18873 | 200 | 44 | 154 | 2 |
| survival | a player with every mode unlocked: the help before a first survival, the bot's game to its last tank, the game-over box | 1973 | 11 | 11 | 0 | 0 |

The one differing menus shot is the title 100 updates in: the loading glow and the title's frame stand a pixel lower in
the original. The intro is ended by a click that the port takes in the update it is given and the original reads
before the next, so the title's fade (two steps of alpha an update) runs one update ahead in the port; the port's
picture after update 151 equals the original's after 152 exactly. It is the comparison's timing, not the game's.

The two differing middle shots are frames of the middle cutscene. The original's Flash control plays on its own clock,
which origrun's virtual clock does not move, so the original is earlier in the movie at the same update; the port
plays one movie frame every 100 / frame-rate updates (GAME_NOTES, "The cutscenes"). To keep the two games together the
bot clicks a movie away once it has played 300 updates, the same update in both; a movie left to end by itself ends at
no update the port can know, and the middle tour's first run parted there (the armory's clicks met the movie in the
original).

The two differing final shots are frames of the final cutscene, for the same reason.

Every faint box was looked at; none is visible at the game's size:

- *The cutscenes' vector edges* (two shots, the first frame of each cutscene: at most 162 levels on 404 pixels of the
  shapes' outlines): the shared SWF player and the Flash control rasterize the movie's shapes with different
  anti-aliasing, and the port draws one faint hairline.
- *Glow and spark edges* (fourteen shots, from 1 to about 800 pixels, at most 76 levels on a few edge pixels): the
  orange hit sparks fading over a boss and the white of an explosion. Their places and counts agree (the trace has
  them); the sparks' fading alpha is drawn by the original's software blit and by SDL's blend, which round apart. The
  largest, the level 2 boss at frame 12500 of the campaign tour, is under "Oddities".
- *The fire over the high scores* (149 shots of the final tour, the high scores after the win): the fire bubbles that
  rise over the nuke's picture are drawn additively and coloured by their life, and the port's sums are up to 12
  levels brighter, by the same rounding. The diff traces the bubbles' circles.
- *A corner the shake uncovers* (one shot, three pixels): a nuke's flash shakes the board, and the screen kept from the
  last frame shows at the right edge below the status bar. What it shows there is what an earlier frame of the shake
  left, which depends on the frames each program drew, not on the game.

### Changes

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| The main menu's "If this isn't you" link was orange | its constructor files the colour under the widget's colour list (`Widget::SetColor`, slot 7), which the link never reads, so it is white | sets colour 0 of the list, as the original does (the map's invade link the same way) | 28a43dc8 |
| The options box's title stood 4 pixels low | its constructor (0x42a540) sets the content insets to 20; the port kept the default 24 | sets them to 20 | 28a43dc8 |
| The main menu's button descriptions were aligned left | the justification 0 is pushed before the font's `GetHeight` call, which Ghidra took for that call's argument | centres them | 28a43dc8 |
| The mission banner's letters, the explosions and the zoomed map differed by a pixel along their edges | the 2D screen stretches in software (`DDImage::StretchBlt`, 0x48dc10): a fast stretch samples in 16.16 steps from the corner, any other averages into an image of the destination's size (0x4740b0, which divides the colour's weight by the alpha already there); the WidgetManager never turns the fast stretch on | `HeavyWeaponDisplay` turns on the framework's software stretch for every stretch in 2D; `gWidgetsDrawWithoutFastStretch` and `gSlowStretchRenormalizesAlpha` are set | a5575ee0 |
| A shaking screen showed a black edge where the board had moved off (a boss's death, a nuke) | the 2D surface keeps what the last frame drew, so the uncovered edge keeps the last frame's pixels | the display keeps the screen in 2D (`DDInterface::mKeepScreen`) | 7d82ac6d |
| Survival showed the points of every kill floating up | `Board::AddScore` (0x417230) makes the floating text only inside its test that the points are not 0 and the game is not survival | shows none in survival, nor for no points | e3e13b1d |
| The main menu and the armory faded the music out and the next song in within an update or two (the sound step) | their constructors (0x425e20, 0x40f190) give the fades the speeds 0.004 and 0.002 as doubles, whose high halves the port had read as floats (0.939 and 0.8765) | fades at 0.004 and 0.002, as the title and the board do | e3e13b1d |
| The status bar's MENU played no click as it went down | the bar's listener (0x52f9f8) has the board's ButtonPress (0x410670) in slot 4, which plays the button-down sound; the port's bar had none | the bar's `ButtonPress` plays it | e3e13b1d |
| Moving onto an armory weapon's plus or minus, and off it, played the map's hover sound | the arrow button's MouseEnter and MouseLeave (0x430220, 0x430240) call Widget's (0x45f950, 0x45f970), not ButtonWidget's, so the weapon's tile never hears of it; the main menu's buttons (0x4259e0, 0x425a20) do the same | both call Widget's | e3e13b1d |
| The armory faded the menu's song in (the sound step) | the armory (0x40f190) fades it in only when `IsPlaying` is false, and the original's BASS (2.0) answers `BASS_ChannelIsActive` yes for a song of the module that has played and been stopped since (its log: 0 before the song first plays, 1 after its `BASS_ChannelStop`), so after the first main menu the answer is always yes | `HeavyWeaponMusicInterface` gives BASS's answer for the module's songs, as Big Money's port does for its title song: the armory never fades its song in, nor does any main menu after the first | e3e13b1d |
| The flak's burst sounded three times as far to the side as the original's (the sound step) | the burst (0x44a220) gives the sound its x as the pan, where the other shots scale theirs by 3 | gives its x | a1d1c996 |

### Focus

The in-game tour loses the focus in a game in play and regains it, in both programs, by origrun's and the port's
`focus lose` and `focus regain`: the game pauses (`LostFocus`, 0x401020), stays paused when the focus comes back
(`GotFocus`, 0x401040, only redraws and keeps the sound off), and goes on at a click on the pause screen. The shots of
all three states are equal. The web page does the same (the page check's focus line and the web driver, below).

### The web build

`idplay.py web` played every tour's script in the web build under Node. Its trace was the native port's line for line
in every tour, as far as the run went (to the end, or to one or two updates before it, where the web run's last line is
cut). Its shots against the native port's:

| Tour | Shots | Equal | Faint only | Differ |
| --- | --- | --- | --- | --- |
| menus | 21 | 21 | 0 | 0 |
| in-game | 13 | 13 | 0 | 0 |
| campaign | 51 | 31 | 20 | 0 |
| middle | 200 | 178 | 22 | 0 |
| final | 200 | 72 | 128 | 0 |
| survival | 11 | 6 | 5 | 0 |

The faint boxes are translucent smoke, glows, sparks and the fire over the high scores: mostly under 13 levels, at
most 37 on a few edge pixels. The web build blends them in WebGL and the native port through SDL's Direct3D renderer,
and the two round apart. The cutscenes' frames are equal, both builds playing them with the same SWF player.

### Play with real mouse and key events

The native port was played under rundesk by `tools/playbymove.py`, which posts mouse and key events to the game's
window and takes its moves from the port's bot: a first campaign from launch (the intro clicked away, the title, a new
player's name typed, the help before the first game, the map) to the game-over box, once in 2D and once in 3D, both in
a window (2540 moves each, both games ending alike with 13130 points), and a survival in 2D full screen (913 moves, to
the game-over box). The web build was played in the Browser pane by `games/heavyweapon/tools/web/onegame.js`, which
sends the page's own mouse and key events: a first campaign in 2D from launch, with the focus lost and regained in play
and the pause screen's CONTINUE clicked (753 moves to the game-over box), the same in 3D with the page at twice the
game's size (801 moves), and a survival at one and a half times (84 moves). No run logged an error. The page check
(`tools/web/pagecheck.js`, with the game's table in `games/heavyweapon/tools/web/pagecheck.js`) passed its 11 lines,
none failing; its one line for information is the bot's moves after the focus check, none, as the game stays paused
once the focus is back, as the original's does.

### Sound

`idplay.py sound <tour>` plays a tour's input in the original on the host (origrun, silenced, on the virtual clock),
with a logging `bass.dll` beside the copy's exe (built from `games/bigmoney/tools/bassproxy`, which passes every call on
and gives BASS_Init its "no sound" device) and origrun's DirectSound stand-in logging the sounds. The port replays the
same input with its sound log (`BYOPOP_SOUNDLOG`), the states are compared as in phase one, and
`games/heavyweapon/tools/idplay/soundcmp.py` compares the two logs: the sounds paired by load order with their length,
rate and channels, every play of a sound in order with its volume, pan, pitch and looping, and the music's plays (with
the order each starts at), its volumes as they change, its stops, pauses and resumes.

| Tour | Sound plays | Music events | Result |
| --- | --- | --- | --- |
| menus | 50, the same | 204, one in another place | the same but for that event (below) |
| in-game | 32, the same | 370, the same | the same |
| campaign | 1318, the same | 978, the same | the same |
| middle | 613, the same | 1725, the same | the same |
| final | 481, the same | 1016, the same | the same |

On the "no sound" device the original's love theme, the one song it loads as a stream (`BASS_StreamCreateFile`, its
file in the copy), comes back 0, so the original plays nothing after the intro there; `soundcmp.py` keeps the song's
place in the load order and sets the port's events for it apart (102 in every tour), and the love theme is not compared.
The menus tour's one music event out of place is a step of the intro tune's fade-out, which the original takes in the
update the main menu starts its song and the port one update before: the intro is ended by a click the port takes one
update before the original (the title shot above), so every fade begun there runs one update ahead in the port.

The sound step found five of the changes above: the main menu's and the armory's fade speeds, the status bar's click,
the armory's hover sounds, the armory's song and the flak's pan. The armory's song rests on what BASS answered on the
"no sound" device: a stopped song "active". Whether it answers so on a real sound device was not measured (with one,
BASS_Init fails under origrun's DirectSound stand-in), but the same device answers 0 for a song not yet played, so the
answer follows what the song has done, not the device; Big Money's port, on the same BASS, keeps it too.

No tour plays 37 of the 92 sounds (the supply helicopter's calls, the mega laser, several bosses' weapons). Where the
port plays one of them with a pan, the pan was checked against the original's code at the call: each scales its x by 3,
as the port does.

## The screen pass (2026-11-21)

The screen audit of 2026-11-21 asked every game for a list of every screen and box it can show, each compared with the
original. `docs/SCREENS.md` is that list for Heavy Weapon: 60 rows taken from our export and the port's source, from
the title to the game-over box, each with the shots that compare it or the reason it has none. It is written by
`tools/idplay/screensmd.py` from the merged shotdiff run `screens` (`idplay.py screens`, every tour's pairs in one run),
and `tools/screencheck.py` passes it: 544 shots compared, none failing.

**What was added.** Six tours joined the six of phase two, each played in both programs as before: `dialogs` (the quit
box's ten quotes, survival locked, the player list with a new player, a taken name, a rename and a delete answered NO and
YES, both lists of high scores and their reset questions), `boxes` (the campaign's continue box, the armory's box about
points left, the status bar's MENU box and the short retreat question once the last tank is lost, the game-over box),
`options3d` (the options' full screen and 3D boxes and the boxes they bring), `levels3` and `levels11` (every level of
the campaign, from saved players before levels 3 and 11), and `fullscreen` (a first run started full screen). The menus
tour gained the intro movie's first and a late frame, and the campaign tour a play tip shot the update before the bot's
click on its OK (`SHOT_BEFORE_CLICKS`). The coverage check's `coverage_ui` was compared too, for the first time. In 3D,
neither the sandbox nor rundesk can run the original, so `idplay.py shots3d` has `tools/visrun.py` play each tour's
script for the original on the visible desktop, on the virtual clock, and replays the port in 3D at the updates those
shots landed on. Each port shot now writes the game's state beside it (`port*/states.tsv`), which sorts the play tours'
periodic shots into the rows by screen and level.

| Tour | Frames compared | 2D shots | Equal | Faint | Differ | 3D shots kept | Equal | Faint | Differ |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| menus | 1251 | 23 | 20 | 0 | 3 | 23 | 17 | 3 | 3 |
| dialogs | 1626 | 33 | 28 | 2 | 3 | 33 | 27 | 3 | 3 |
| coverage_ui | 4001 | 22 | 21 | 0 | 1 | 15 | 13 | 0 | 2 |
| options3d | 476 | 7 | 7 | 0 | 0 | 7 | 7 | 0 | 0 |
| in-game | 1501 | 13 | 13 | 0 | 0 | 2 | 1 | 0 | 1 |
| boxes | 3630 | 23 | 13 | 10 | 0 | 5 | 1 | 4 | 0 |
| campaign | 12693 | 54 | 51 | 3 | 0 | 1 | 1 | 0 | 0 |
| survival | 1973 | 11 | 11 | 0 | 0 | 1 | 1 | 0 | 0 |
| levels3 | 18686 | 99 | 87 | 11 | 1 | 2 | 2 | 0 | 0 |
| levels11 | 19126 | 102 | 89 | 11 | 2 | 3 | 1 | 2 | 0 |
| fullscreen | 688 | 11 | 10 | 0 | 1 | | | | |

The game's state agreed at every compared frame in every tour, but for a stretch in dialogs and coverage_ui (below). A 3D
shot is kept only before the original's 3D game parts from its 2D one, and never inside a level ("Oddities"); the
full-screen tour was not run in 3D, since the original in full screen on the visible desktop would change the user's
display mode.

**Changes.**

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| The boss warning's arrows pointed away from a boss at the side: the battleship's, the rainer's, the wrecker's, the ape's and the head's (`BOSS_INFOS`' quarter turns; the worm's half turn comes out the same either way) | `TurnPoints` (0x40e730) turns them with the early-2005 framework's `SexyTransform2D::RotateRad` (0x4964c0): cos, -sin over sin, cos of the angle itself; the shared framework's negates the sine first, so it turns the other way | turns them with the 2005 matrix (`RotateRad2005`), the game's only rotation | 4509c816 |
| The satellite's beam burned the ground 12 pixels low, its lines running past it (two shots of levels 9 and 19) | the beam (0x434ac0) ends its lines and draws its burn at y 0x1cc, 460, the ground's y | ends them at `CRAFT_GROUND_Y`, without the 12 the port had added | 4509c816 |

The input of a tour is now read in update order (the boxes tour's late MENU clicks follow the bot's in input.txt); that
was the tooling's, not the game's (b2defaec).

**Differences explained, none of the game's.**

- *A box that waits for its answer, opened by a scripted click.* The name box's refusals (an empty name, a taken one)
  and the high scores' reset question wait in an update loop of their own (`Dialog::WaitForResult`). The port's test
  script delivers a click inside an update, after the update's count and before the widgets update, so the update in
  which the box opens has its widget update only after the box closes; the original, and the port with real input, take
  the click between updates. In the trace, the main menu's flicker lights (0x423e50, a draw of `Rand()` each update and
  more while they flicker) are an update's draws behind while the box is up and catch up when it closes: 42 frames of
  dialogs and 49 of coverage_ui, nothing else differing and the generators agreeing again afterwards. In the pictures,
  the fire over the high scores stands an update off around the reset question, the port's picture an update later
  during it and an update earlier after it the nearest, the rest the fire's rounding. The shared fix would deliver a
  script's input between updates; it is with the coordinator.
- *The custom cursors ticked* (coverage_ui): the sandbox starts with the game's own cursors off, so the tour's click turns
  them on; the original draws its hand into the frame and the port's `screenshot` never draws the cursor.
- *The options in full screen* (fullscreen): on rundesk's headless desktop the original cannot open its full-screen
  mode and falls back to a window, so its box is unticked; the ten other full-screen shots match.
- *The title, the intro and the cutscenes* (menus, levels3): as in phase two, the click that ends the intro is taken an
  update apart, and the movies play on the original's wall clock.
- *The zoomed map in 3D* (in-game, coverage_ui): Direct3D 7 and SDL's renderer filter the stretched map apart, by at most
  72 levels on a few hundred pixels; in 2D it matches.
- *Translucent smoke over a fire* (two shots of levels 13 and 14, at most 97 levels): the translucent-pixel rounding
  under "Oddities", in smoke as well as sparks and glows.

The runs: the tours' shotdiff runs `shots_<tour>` and `coverage_ui`, their 3D runs `<tour>_3d`, and the merged `screens`.

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
- **Translucent pixels on the 2D screen.** Where the 2D screen blends a translucent or additive picture, the port's
  pixels can differ from the original's by a few levels: the orange hit sparks fading over a boss (at most 76 levels on
  a few edge pixels; the most pixels over the level 2 boss at frame 12500 of the campaign tour), an explosion's white
  glow, the fire bubbles over the high scores after a win (at most 12), and smoke drawn over a fire (at most 97 levels,
  over the level 14 boss in shot 14800 of the levels11 tour). Their places and counts agree; the
  original's 2D surface blends them with the 2005 framework's software blitters, the port with SDL's renderer. To see
  it: the review of `idplay.py shots shots_campaign`, shot 12500, or any shot of the final tour's high scores. To match
  it, the 2D screen would be drawn by the framework's software blitters (`SoftwareScreenDisplay`, as Dynomite's is),
  which cannot yet hand the screen back to the GPU for 3D (a shared change), and whose blitters would need checking
  against this game's; the web build would keep SDL's.

- **The original's 3D game is not its 2D game.** With the same input and seed, the original in 3D (on the visible
  desktop through visrun, on the virtual clock) parts from its sandbox run in 2D inside the first level it plays: the
  shells, smoke and bullets stand elsewhere within a few hundred updates, then the tank's X differs in its last bits
  (the last agreeing peek between updates 756 and 2456 of the play tours' 3D runs, 5256 in coverage_ui, whose level
  starts late), then a hit lands or misses and the score parts. The menus,
  dialogs and screens between levels agree. The port's 3D game is its 2D game, update for update. One candidate is the
  x87's precision: the DDInterface's init (0x484dc0) sets the cooperative level with `DDSCL_NORMAL` alone, so its
  Direct3D device runs the x87 at single precision. A test patch adding `DDSCL_FPUPRESERVE` to that call did not keep the
  games together (that run parted at update 856, the unpatched one at 1256), and two runs on the same input parted at
  different updates, so the cause is not found; visrun's runs leave a few real cursor reads too, which may matter. To
  see it: `idplay.py shots3d shots_survival`, then the run's `orig3d/steps.tsv` beside the sandbox run's peeks. For the
  screen pass every 3D shot inside a level, and every one after the last peek that agrees, is left out (the survival
  tour's 3D original is the patched run, whose three shots kept are of the menus before the parting).

## Deviations that remain

The deviations in `docs/GAME_NOTES.md` are unchanged but for the refresh rate, which this pass found and lists there;
none shows in the compared state. The screen pass adds the two the user asked for (GAME_NOTES, "Deviations the user asked
for"): the site names the game "Heavy Weapon" without "Deluxe" (the user, 2026-11-18: "Remove the "Deluxe" text from the
main screen of any game it may appear in, to avoid misleading people about whose game version they're playing"), the
native window's caption drops the word too ("Heavy Weapon 1.0"), and no screen needed the word covered, since none shows it.
