# Hammer Heads Deluxe: the identical-play pass

Written for the user. Phase one (the game plays out the same in the original and the port) is done for the scenarios below;
phase two (it looks and sounds the same) is under way: the sound and the music are compared by state (below), the looks are not yet. This file grows with each phase.

## Phase one: plays identical

### What is compared and how

The original (`HammerHeadsGame.exe`, the user's copy staged under `E:\byo-og\hh2\game`) runs under origrun on its virtual clock
with scripted mouse and key input, with its random seed pinned (`tools/origrun/patches.txt` stands a fixed value in for
`GetTickCount`, which both of the exe's generators are seeded from). origrun counts the application's update counter
(Application +0x44c) and holds the game after each update; at every hold it reads, with `peek`, 318 values of the game's
memory: the Mersenne generator's index and two of its words, the 50 numbers of the stats, the game screen's level, mode and
cycle, the gameplay's phase and its timers (the next pop, the clover, the strawberry, the double hit, the five times), the level's
timer, and for each of the first ten cubbies its state, its rise, its creature's kind, state, age, height and its velocity,
health, hit stage and frame, and its position, and the avatar's swing, its frame and the count of clicks. The port is given the
same run through `-script` and writes the same values with its `trace` test command (`GAME_NOTES.md`), and
`games/hammerheads/tools/idplay/idplay.py` compares the two update by update. The random generator is the strict value: a
number drawn in another place or not drawn shows at once and stays different.

The framework runs a frame as three steps (the first runs one tick, the second the ticks the frame still owes and then the
frame's `UpdateF`, the third waits), and origrun's hold lands between steps. A record taken after the first step's tick (the
exe's update state 2) is the one the port can write at the same place, so only those are compared: the first tick of each frame,
3 of every 5 updates (16790 records for 28000 updates). What the frame's later ticks did shows in the next record. The
comparison is of values, never of the pictures.

The input does not come from a person. `botclick` (in `TestCommands.cpp`) is the port's player: it reads the game's state, picks
the first creature that is standing up and not a bomb, aims at the point the avatar would strike it at (the cubby's strike
point, where a solid pixel of the creature lies five pixels all round, so that a click does not fall on an edge the two
games' pictures would not both have), waits for the hammer to be idle, and clicks. It also presses the level introduction's
button and the one button of each box that waits (the "Ready?" box, a bonus round, a finished challenge, the game over and
game won boxes, the Tough Cookie notice, which it presses only after the box has rested for six steps of its own), picks the
highest point on the page of restart points, buys three things in the shop and leaves it, and now and then, from a random
generator of its own seeded by the scenario (`BOT_SEED`), clicks a bomb or bare ground instead (3 and 1.5 in a hundred). Its
clicks are written to a file (`scenarios/<name>.clicks`, one `update x y` per line) and both games get them at the same update
counts, a mouse move first; the port runs an input at the update after the one the original reads it at (the original reads
the message when the frame is over). The bot draws no random number of the game's. Its play is not what is tested; the aim
is that both games, given the same clicks, play the same.

Three details of the harness matter. The port runs `-fast` with its refresh rate fixed at 60 (the exe reads the display's, 60
on rundesk's desktop). The original's frame rate gives `1000 / (10 * 60)` ticks to a frame, kept as a double in the exe
(0x4d5d30) where the 2004 framework rounds it to a float, so the first frame owes 1 tick, the next 2 and the third 2; the port's
framework had the float and every tick after the first frames fell in another frame (`mUpdatesPerUpdateFExact`, a flag in
`SexyAppBase` that only this game sets). And a scenario that starts at a later level does not play the earlier ones: the original
through a patched copy, in which the game screen's constructor (0x4527c0) loads the level that the cheat word `jumpto<n>` would
(a patch at 0x4529c4), the port through its `cheat` command, the same cheat typed (the main menu clears the cheats when it is
made, so the command goes after the menu is up). Tough Cookie and Marathon are unlocked the same way: the original's
`IsToughUnlocked` (0x41f2d0) and `HasFewRestartPoints` (0x41f290) are patched to answer yes and no, the port gets `unlock all`
and `unlock marathon`. The `nomissdamage` and `noawaydamage` cheats, made true in the original by patching the two tests of the
cheat flag (0x44a2c5, 0x44a39c), keep the bot's game from being lost in the `classic_nd*` scenarios.

| scenario | T records compared | what it covers |
| --- | --- | --- |
| `classic1` | 17991 (30000 updates) | Classic Bash from the loading screen: the new player's name, the how-to-play page, the first level's introduction, then levels 1 to 4 played by the bot, each ended by the level's clock (the pause after the last cubby, the summary, the next level) |
| `classic_j5`, `_j10`, `_j15`, `_j20`, `_j25` | 16788, 16787, 16791, 16790, 16791 | Classic Bash started at level 5, 10, 15, 20 and 25 (the `jumpto` cheat), the bot playing it until the game is lost (hearts gone, the game-over box, the page of restart points, the same level again), over and over; the creatures of those levels (bombs, bubble heads, silvers, strongs, switchers, ducks, napoleons, strawberries), the shop, and the bot's bombs and misses |
| `tough1` | 16790 | Tough Cookie unlocked and started from the main menu, levels 1 to 3 |
| `marathon1` | 16791 | Marathon unlocked and started from the main menu, levels 1 to 4 of the marathon, a shop between them |
| `classic_nd1` | 16791 | as `classic1` with the two damage cheats on: levels 1 to 3 |
| `classic_nd5`, `_nd10`, `_nd15`, `_nd20`, `_nd25` | 16791, 16790, 16787, 16789, 16789 | Classic Bash started at level 5, 10, 15, 20 and 25 with the cheats on, so that the bot is not lost: levels 5 and 6, 10 and 11 (a challenge screen entered and left), 15 and 16 (a challenge), 20 and 21 (a shop), 25 (the last level's kings; it does not end in the time of the script) |
| `classic_nd22` | 16652 (28000 updates) | the cheats on, started at level 22 (the test command `startlevel 22`; the original gets the same patched start), levels 22 and 23: the multiplier gnome (struck, so that its x5 timer ran 1246 updates), a challenge and a shop |
| `classic_sp25` | 19704 (33000 updates) | the cheats on and `superpower` on (a hammer of level 3 on fire, 7 hearts, 50000 coins), started at level 25: the kings brought down, the level ended, the zoom wipe, the game-won page (its "Main Menu" button pressed by the bot), the main menu and the Tough Cookie notice (the original's screen pointers were read at the same updates: the game-won page stands at 30750, the main menu from 31000, in both) |
| `classic_save1` | 9514 (16000 updates) | `classic1` left in the middle of level 1 (the in-game menu, "Main Menu", "Yes" of "Leave Game?", which saves the game) and resumed from the main menu ("Classic Bash", "Continue" of the box that offers the saved game, "Go!" of the "Ready?" box): the saved game's level, score, stats, gnomes and random generator come back, and levels 1 and 2 play on identically |
| `classic_save22` | 13187 (22000 updates) | the same at level 22 with the cheats on (typed again in the port after the menu clears them): the multiplier gnome, silvers, strongs, switchers, napoleons, bubble heads, a shop and the clover stand in the saved game, and the resumed level plays on identically |

Together the runs stood up every kind of creature in a cubby (the gnome, the woman, the strong, the bomb, the
bubble head, the hide, the forky chap, the silver, the cupido, the heart, the coin and the big coin, the chest, the clover, the
ice cream and the duck of the bonus, the strawberry, the rabbit, the king, the save, the napoleon, the power-up, the switcher, the
shop's and the challenge's gnomes, and the multiplier), and the screens game, shop, challenge, the game-won page and the menu. Each run's own coverage (levels, phases,
creatures, swings that struck something, the stats at its end) is written by `idplay.py` to `coverage.txt` in its run folder.
The state log lists the cubbies only: the flying and the bouncing gnomes, which are objects outside them, and the stickers show in
it through the score, the stats and the random generator when the bot or a miss reaches them.

### Differences found and fixed

| difference | cause | what the port does now | commit |
| --- | --- | --- | --- |
| The first gnome popped at another update (the pop timer's wait) | the pop test in `Gameplay::FixedUpdate` asked for the timer above zero; the exe's test is at or below zero | `mNextPopTimer <= 0.0` | fbafe16f |
| Every tick after the first frames fell in another frame, so the level's clock and the pops drifted one tick at a time | the framework rounds the ticks a frame owes (1000 / (10 * 60)) to a float; the exe keeps a double, so its second frame owes exactly 2 and the sum of owed ticks reaches 1 at another frame | `mUpdatesPerUpdateFExact`, set by this game | 01654778 (shared) |
| A click on a sticker, a flying gnome or a bouncing gnome found no target (or the wrong one) | these three answer `MouseOver` (0x443290) with a test of their bounds alone (no solid-pixel test, as a cubby's creature has); the port's objects answered false | `SimpleObject::PointInBounds` and the three `MouseOver` overrides | 3458c287 |
| A click beside a sticker struck the gnome behind it, where the original struck the sticker | the original's `Avatar::Probe` (0x42e120) asks every object `Hit` with the hammer's mask as well as `MouseOver` and uses the first `Hit` when none answers `MouseOver`; the port had left `Hit` out, its comment saying the original never used the answer | `Probe` keeps both and falls back to the first `Hit` | 85afce3d |
| Every sticker lay half its picture up and to the left of the original's, so clicks that struck one in the original missed it, and the picture was drawn in the wrong place | the board's editor tool makes a sticker at the middle of its box (its corner plus half the width and height, 0x408d60); the port made it at the box's corner | `GameScreen::Start` adds half the box; `BoardFile` reads Width and Height | 85afce3d |

The original's own crash is not a difference: see the first deviation below.

### Oddities skipped

**The "Ready?" box's first update at rest.** In Tough Cookie the box slides in from above and the port's reached its resting
place by update 3000 of the first scenario; a click at that update was lost in the original (the level did not start and the
game stayed under the box), where the same click is taken by the port. The bot now waits six of its steps (60 updates) after the
box rests, and both games then agree. The slide runs in frames (`NcMovingDialog::UpdateF`), the logic does not read it, and so
it is no difference of play; whether the box settles at the same update in both belongs to phase two (the look and the timing of
the slide).

### Deviations that remain

As `GAME_NOTES.md` lists them. The one the runs found is "A swing whose target is gone": a click that has begun a swing on the
shop's gnome while the shop is left deletes the gnome, and the original then strikes through a null pointer at frame 3
(`Avatar::FixedUpdate`, 0x42eb80; an access violation at 0x42ebb2, found when the bot's click on the shop's arrow fell in that
frame); the port tests the pointer and the swing ends with no strike. No scenario reaches it any more, since the bot waits for an
idle hammer before it leaves the shop.

### Left in phase one

The runs do not record which boxes came up (a challenge's or a bonus round's finish, the high score entry after a lost game), only
that the challenge screen was entered and left in `classic_nd10` and `classic_nd15`. The flying and the bouncing gnomes' own state
is not in the log (only its effects). A game left and resumed was run twice (`classic_save1`, `classic_save22`: the saved game,
the continue box and the resumed level play on identically); a game that is lost with a saved game waiting, and the saved games of
Tough Cookie and Marathon (the marathon's own clock), were not. These belong to the coverage check before the final merge, with
the focus loss and regain and the Tough Cookie and Marathon runs at later levels (`jumpto` works in both modes); the looks and the
sound belong to phase two.

## Phase two: the sound and the music, by state

### What is compared and how

The runs of phase one carry the sound too, so that what is compared is what the same input makes the game play.

The sounds go through DirectSound. origrun's stand-in for it logs every buffer the original plays and stops, with the virtual
clock's time, the buffer's volume, pan and frequency, and which sound the buffer is (a duplicate of a sound's own buffer); the
port's framework logs the same for its samples (`BYOPOP_SOUNDLOG`, the update count first). `tools/idplay/soundcmp.py` lines the
two up: the plays are compared in order, sound by sound, with volume, pan, pitch and whether it loops. A stop is not compared
where the sound ended by itself, since the port's instance ends where the original's buffer does and the original's stop is the
game's own for a buffer it still thinks is playing. When the plays are the same, how the times fall can be listed as information
(`--times`): the port's plays that an input made come one or two updates after the original's, because the harness posts the port
its input an update later (as in phase one). Both sides are cut where the run ends; the original's shutdown runs the virtual clock
ahead (marathon1's by 20 seconds), and what it plays then is not the run's.

The music goes through BASS (four ogg files, one stream each). `tools/bassproxy` is a stand-in `bass.dll` that sits in a copy of
the original beside the real one (renamed `bass_real.dll`), logs every call with the update count (read from the game's own
counter) and its arguments, and answers for a stream that BASS's no-sound device cannot make (the only device the stand-in
DirectSound leaves it) with a handle of its own that plays and stops as BASS's would. `tools/idplay/musiccmp.py` reduces that log
and the music lines of the port's sound log to the same events: a file made into a stream, a song's volume set (at every update
of a fade), a song started, stopped, paused or resumed, and the master volume. It compares them in order with the update of each.
Two differences in how BASS is asked, with the same result, are not events: the original stops every other song whenever one is
faded in and stops a song just before it plays it again (a stop of a song that is not playing, and a stop followed at once by the
same song's play, are dropped), and the port's mixer starts at full volume before the game sets its own (the last of several master
volumes at one update stays). When the three other songs are made into streams is the loader's business (the original's loading
thread does it a little after the start, the port's at 0), so the files are compared as a list. A master volume that a focus change sets comes
one update later in the port, as every input does.

Each scenario below is one `idplay.py run`: the original and the port with the same input, the state compared as in phase one, then
the sound, then the music.

| scenario | plays, music events | what it reaches |
| --- | --- | --- |
| `classic1` | 1152 plays | a first game of Classic Bash from the loading screen: the buttons' mouse-over and click, the dialogs' woosh, the hammer's and the gnomes' sounds, the level's ending |
| `classic_save1` | 396 plays; 1613 music events (the first 16000 updates) | the saved game left and resumed: the menu and in-game music swapped, with the fades, to the update |
| `classic_nd10`, `classic_nd22` | 1547 and 1901 plays; 2016 music events each | levels 10 and 11 and levels 22 and 23: the challenge and the shop (the shop's mole and the hint popups), the multiplier gnome's timer |
| `classic_sp25` | 2479 plays; 2680 music events | the kings, the zoom wipe, the game-won page and its voice, the Tough Cookie notice |
| `tough1` | 1349 plays; 1939 music events | Tough Cookie from its first level |
| `marathon1` | 1043 plays; 1874 music events | Marathon through the game over box, "Try Again" and a new game |
| `classic_save22` | 1413 plays; 2009 music events | the saved game of level 22 resumed |
| `classic_focus1` | 333 plays; 996 music events | the window losing and regaining the focus on the loading screen, on a level's introduction and twice in a level (the second time for four seconds with the bot's clicks arriving): state, sounds and music identical, the music's master volume going to 0 on the loss and back to 85 on the regain |

### Differences found and fixed

| difference | cause | what the port does now | commit |
| --- | --- | --- | --- |
| The original played the mouse-over sound for every button and the port for some | the six screens' `ButtonListener` tables share one `MouseEnter` function (0x454bc0, which plays sound 0); the port had no override on the screens that did not make it themselves | `ButtonMouseEnter` on the main menu, the how-to-play page, the level introduction, the game screen, the game-won page and the high score table | f4d747a4 |
| The in-game menu opened at the press of its button and the original's opens when the button is let go | the game screen's handler (0x450e10) is the listener's `ButtonDepress`; the port's was `ButtonPress` | renamed, same body | f4d747a4 |
| The ice cream gnome played its gloop twice | `GnomeBonus::Hit` played it, and so does `StartIceCreamPower` (0x435fd0) | the extra play removed | f4d747a4 |
| "Try Again" and "Main Menu" of the game over box played no click | the box's `ButtonListener` table points at 0x44fe90, which plays the click for any button pressed (the options box's own handler is the same function); the port's box had none, and the moving dialog's handler it inherited (0x468bb0) clicks only for ids that no dialog's own buttons have | `GameOverDialog::ButtonPress` plays the click | 7e802bbd |

The harness had one fault of its own that looked like a game difference and is fixed in it, not in the port: the original played the
mouse-over sound at nearly every click (twice at some). The cause was origrun's shared desktop. A click was posted to the game's
window with the cursor set to its point, and the original's own timer test for "is the mouse still in the window" (0x4d7000) asks
which window is under the cursor; on rundesk's desktop another run's window was found there, the original took that for the mouse
leaving, and entered again at the next move. The original now runs with origrun's `--cursor scripted`, which answers that question
from the scripted point, and with a test patch for the one comparison left that does not use it (0x4d7805, listed in
`tools/origrun/patches.txt`).

### Deviations that remain

How BASS is asked is not the same: the original calls `BASS_ChannelStop` on a song before it plays it, and on the other three
whenever one is faded in; the port's mixer stops what is playing as it plays what is next. What is heard is the same, and the
comparison is of the state. `GAME_NOTES.md` lists it with the others.

### Also run

The phase-one scenarios `classic_nd1`, `_nd5`, `_nd15`, `_nd20`, `_nd25` and `classic_j5`, `_j10`, `_j15`, `_j20`, `_j25` (28000 updates
each) are identical in state, sound and music too.

### Left in phase two's sound

The sounds of a game lost with a saved game waiting, the high score entry and Tough Cookie's and Marathon's saved games are not
reached yet. The looks (every screen and dialog against the original's, natively and on the page) are phase two's next part: the
shots may use the visible desktop or Windows Sandbox at any hour (CLAUDE.md, "Desktop and Sandbox at any hour").

## Phase two: the looks (begun)

### What is compared and how

`tools/idplay/looks.py <scenario> <first> <last> <step>` plays a scenario's inputs and takes a shot every `step` updates, in the
original (inside Windows Sandbox, the only place origrun captures; the registry preset `ScreenMode` 0 makes it windowed, as the
host runs have it) and in the port (`-renderscale=1 -fast -drawevery`, `screenshot` at the same update), then runs `tools/shotdiff`.
The sandbox loads more slowly than the host, so the original's inputs count from update `mark` (about 6000) and the port's from the
same; the port's inputs go in one update after the original's, as in phase one, and its shots at the original's own update. A shot
is pixel-identical where no input, particle effect or popup is in motion. Shots taken: `classic1` every 250 updates from 1000
to 9000, and `classic_nd10`, `_nd22`, `_sp25`, `tough1` and `marathon1` every 500 from 1500 to their ends (the shop and
challenge, the trophy pages, Tough Cookie's notice, the game-won page, the game over box, Marathon's tutorial boxes).

### Differences found and fixed

| difference | cause | what the port does now | commit |
| --- | --- | --- | --- |
| A black strip under each hole of a level | the cubby's cover (the backdrop under the hole, 0x433080) was made from the cubby's own back image: the board-file path in `GameScreen` passed `GetCubbyBackImage()` where the original's container passes the level's backdrop | `GetBackdrop()` | see the log: "looks pass" |
| The "Health" label of the hearts bar 40 pixels too far left | the original puts it at the hearts' x plus the box width (`rowWidth + 0x28`) minus the label's width minus 0x26 (0x449c40); the port used the row width | the box width | same |
| The coin hint ("Pick up", level 1 and Marathon) 66 pixels too low | `GnomeCoin::Update` (0x438e20) makes the hint's floater at (400, 150); the port made it at (400, 216) | 150 | "looks pass" |
| The "Snapshot Saved!" photo (the camera trophy's floater) faded out as it came in, with no text | `FloaterPhoto::Draw` (0x4485c0): the colour's alpha is `0x80 - ftol(-128 * move)`, so 128 up to 256 (the port had `0x80 - 128 * move`); the text's alpha comes from `(1 - width) * 255`, below 0 while the picture comes in, which the original's shots show as the text whole (the port had it hidden) | alpha `0x80 + 128 * move`; a negative text alpha is drawn opaque | "looks pass" |

### Judged, not changed

- **"More Games" on the main menu** is in the original and not in the port: the outbound buttons are removed on purpose (CLAUDE.md,
  "Registration and outbound buttons are removed"), and the Options and Quit buttons close the gap.
- **The how-to-play page** draws in the sandbox (the Flash control shows its title, hearts, three captions and the gnome's demo), but its
  demo loops on the original's wall clock while the port's runs on updates, so a shot at the same update shows different moments of
  the loop (the title and hearts being drawn in the port, the gnome up in the original). The captions, holes, plants and buttons are
  identical. Compared by the movie's frames instead of by update (`scenarios/howto.py`: 20 original shots across 9000 updates against 641
  port frames every 10 updates): the same elements in the same order (the page fading in with its title and hearts, the hammer and
  ring over the gnome, the MISS burst, the hearts going from five to four to three, the caption lit in turn, the ring moving to the other
  holes). A frame-exact phase match is not possible, because the original's movie restarts on the wall clock and each of its shots
  lands on an arbitrary frame (the best matches differ by about 2.6 on a 0-255 scale, the floor of the sprites' edge differences, with
  the periodic loop making the best frame ambiguous). Judged equal: the demo's content and order; its timing is not compared.
- **Particle effects and rotated sprites** (the smash debris, the stars, a rising creature's glow): a shot series every five updates
  from a hit shows the pieces at the same places in the same updates and differing only along their edges, the same
  one-pixel rims as below.
- **Edge pixels:** a creature's or platform's rim differs by one pixel row in a few updates and every colour by one level; both
  are the framework's GPU blending against the original's software blit, below the tolerance except at a rim.

- **The menus' boxes and screens** (`menus` scenario: Options, its Credits box, High Scores, the Trophy Room, each shot open and
  closed): High Scores, the Credits box after it opens, and the closed main menu match pixel for pixel apart from the removed "More
  Games" button and the closed gap. The Options box differs only in the Hardware Acceleration and Full Screen ticks (the sandbox
  runs the original windowed with no acceleration, the port full screen: the player's settings, not the game) and in which button
  the mouse rests on (the highlight), both scenario effects; the Trophy Room differs in its button's highlight.

- **The page check** (`tools/web/pagecheck.js` through `games/hammerheads/tools/web/pagecheck.js`, in the Browser pane): 11 checks pass,
  none fail. The focus line matches the native build's answer (the pause count changes while the focus is away and when it is
  back), the keys and the pointer at four canvas sizes are right, a second copy waits for the first, and the saves come back after
  a reload. The table plays the loading screen's click and a new profile's name box itself, as the game waits for both. The bot made
  no moves before the reload (the level was still on its Ready box), so the "game in progress" line has no save to restore.

### Real input, natively

`games/hammerheads/tools/realplay.py` plays the port under rundesk with `-realinput` (docs/REAL_INPUT.md): SDL events pushed into
SDL's own queue, the moves from the port's bot (`move`), the state (`state`) saying which box is up. From a clean user folder it
clicks the loading screen, types "X", a Backspace and "Test" into the name box, clicks OK, clicks the mode's button and then every
button the game shows: the how-to-play page, each introduction's and Ready box's Go, the shop's Back to Game, the bonus round's
opening and the summaries. A screenshot is never used to choose. Runs (seed 1, `--heal`, which sets the hearts back to 5 when
they are down to 2.5 so that the weak bot reaches later levels; a test command, not an input):

| run | result |
| --- | --- |
| Classic, windowed | launch to level 25 (the king's level): 3698 moves, 25 levels, the shops and bonus rounds in between; the game is lost to the king, which the bot does not defeat |
| Classic, full screen after the first level, with the focus lost and regained | 8 levels, the clicks and moves landing as in the window (an earlier run of the driver, before `--heal` took 2.5) |
| Tough Cookie | to level 25, 4765 moves, lost to the king |
| Marathon | 3000 moves to level 23 |
| a lost game (`lose` after 6 moves) | the Game Over box (dialog 15) |

The driver's clicks on the summaries, the trophy award and the shop are found by trying the buttons the first run showed in turn, so
a screen where none changes the state stops the run (none did after the driver waited out the wipes). The king's defeat and the
game-won screen are reached by the test commands (`set kings`, `win`) in the autosplit logs, not by the bot.

Run again on 2026-11-22 for the screens pass's branch (seed 1, `--heal`, the current build): Classic from launch to the king, 3698 moves, lost there
(434 s); Tough Cookie 4765 moves, lost to the king (529 s); Marathon 3000 moves, the limit (281 s); all exit 0 and no screen where the driver
stopped. (A first Marathon run was left uncapped by mistake and ran to 25606 moves, level 159, before it was stopped; Marathon has no end.)

### Real input, on the page

`games/hammerheads/tools/web/onegame.js` is the same driver as a page driver (the Browser pane, `hammerheads.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/hammerheads/tools/web/onegame.js&mode=<m>&heal=1&moves=<n>`):
DOM mouse and key events only, the bot's moves from the test command `move`, the same screens and buttons as the native driver.
Classic ran from the loading screen to its last level and the king (3767 moves, lost there like the native run), Tough Cookie the
same (3736 moves), Marathon 3000 moves, and a Classic run to level 6 lost and regained the focus once; no page error in any of them,
and the screens met are the native run's (name box, how-to-play, introductions, Ready boxes, shops, bonus rounds with their finished
box, Game Over). The Browser pane cannot enter full screen, so the page is checked at the sizes `pagecheck` draws it at (11 pass).

Run again on 2026-11-22 on this branch's web build (`wordharmony-webtest`'s server, `serve.py`, in the Browser pane, `?silent=1`): Classic
from the loading screen to the king, 4027 moves, ended on the Game Over box (388 s, no page error); Tough Cookie 3747 moves, lost, Game
Over box (509 s, none); Marathon 3000 moves, the limit (318 s, none); `pagecheck.js` 11 pass, 0 fail (1 info: the browser keys the
game takes). The pane's behaviour to know: a `player=1&assets=1` page keeps the renderer busy for about five minutes after the load
starts (the wasm's compile and the import in a background page on a loaded machine: `hammerheads.wasm` arrived at 83 s and the file
list was asked for at 304 s), so every probe in that time times out at 45 s; Bejeweled's page, the control, did the same and answered
at about 330 s. It is the pane and the machine, not this game: wait the five minutes, then probe with short calls (timers in the page
run, so a driver's result is there when it is done). `?silent=1` alone, with the files imported by hand, answered at once.

### Open

- The 3D variant and full screen of every screen are not compared: the sandbox runs the original windowed in software 2D (the
  address of its 3D flag is unknown, and it cannot run full screen there), so the screens pass is 2D and windowed, against the
  port with `3d off`. Every other screen is in `SCREENS.md`.

### The web build against the native one

`games/hammerheads/tools/idplay/webshots.py <scenario> <first> <last> <step>` plays a scenario's port inputs in the native port and in
the web build under Node (the same script, seed, `-fast -drawevery`, a clean user folder) and compares the shots with `tools/shotdiff`.
Run: `menus` (21 pairs), `classic1` (30, every 1000 updates), `tough1` (14) and `marathon1` (14). Both builds play the same game:
every shot shows the same screen, the same gnomes, scores, hearts and pieces at the same places. Shotdiff flags most pairs (6 of 21,
8 of 30, 1 of 14 and 0 of 14 match exactly), for three causes, all of the back end and none of the play:

- **Edges of rotated and fractionally placed pictures** (the title's hammer, which `DrawImageF` puts at a fractional position, the
  trophy medal's rays, the smash debris, the stars): the native build draws them through SDL's renderer with filtering, the web build
  through SDL's software renderer, which snaps and does not smooth. Blurred 8 pixels wide, the shots agree to within a channel value
  of about 20 to 36 (the debris of `classic1` at 11000 is the largest), with no shift of any piece.
- **The Options box's Hardware Acceleration and Full Screen ticks**: the web build has neither (the player's setting; the native run
  starts full screen with acceleration), the same two ticks that differ against the original.
- **The loading screen** (`classic1` at 1000): the title's glow is the loading thread's, on the wall clock.

Left as it is: a fractional-position snap in the software renderer is the framework's, shared by every game.

## The screens pass (2026-11-21)

`SCREENS.md` lists every screen and dialog of the game, one row each, with the shots that compare it, and `tools/check.sh screens`
checks the list against the newest shotdiff run (`build/shotreview/hammerheads/screens`: the original in Windows Sandbox against
the port, 800 by 600, about 760 pairs). The audit of 2026-11-21 named gaps; each is reached now.

### What was added

New scenarios (`tools/idplay/scenarios/`, run by `looks.py <scenario> 0 0 0 --shots`; the player each starts as is made by
`mkprofile.py`, in the original's own file format, so both games read the same one):

| scenario | screens |
| --- | --- |
| `scr_menu` | the name box empty and typed, the player list, the other name box, Rename, Delete's "Are You Sure?", Quit's question and its No |
| `scr_errors` | the error boxes: a name is needed, a name is taken, a new name is taken |
| `scr_rich`, `scr_trophy` | a player with a past: every mode open, the restart-point screen, the saved game's "Continue?" box, the Trophy Room with eleven trophies won and the close-up page of each |
| `scr_lv1` to `scr_lv5` | the introduction, the board and the game over box of every Classic level from 2 to 25 (a player whose restart points start the game on each), and the trophy award pages of levels 6 and 11 |
| `scr_over` | the game over box of Tough Cookie, of Classic Bash and of Marathon Bash, with Marathon's box, board and wave |
| `scr_wipes`, `scr_wipes2` | every zoom wipe (the hammer, to and from the Trophy Room and the High Scores, the loading screen's, leaving a game, the level-up wipe, the shop's), a shot in each of its first frames |

The earlier scenarios were run again so that one `summary.json` covers all of them (`classic1`, `classic_nd10`, `classic_nd22`,
`classic_sp25`, `tough1`, `marathon1`, `classic_save1`, `classic_save22`, `menus`, `howto`; their shots are named by the update).
The tour (`tools/tour_test.txt`) takes eight shots more (the menu after a lost Marathon game, the player list, Quit's question, a
board, the game menu, the leave question, the menu after leaving, a Classic game over box); `HH_SHOTS_ALL` and `HH_FROZEN` in
`tools/check.sh` are renewed, and the frozen baseline (`build/baseline-hammerheads`) is the port of this branch.

### Differences found and fixed

| difference | cause | what the port does now | commit |
| --- | --- | --- | --- |
| The original could not read a player's file the port wrote, nor the port one the original wrote | the original writes its XML in pieces, each with its length in front (a little-endian 16-bit number); the port wrote and read plain XML | `XmlNode::ToChunkedFileText` and `FileTextOf`: the port reads both forms and writes the original's | 30710ae9 |
| The High Scores page was gone the moment the player left it: the first five frames of the zoom wipe showed the hammer on black, where the original's screen still shows the page behind the hammer (its uncovered pixels are not cleared) | the port killed the table's dialog before it started the wipe (the export does the same, but the original's screen is not cleared between frames); the wipe's callback, `HighScoresToMainMenu`, kills it again | the table stays until the callback | this branch |
| `goto <mode>` (the site's screen saver) stayed on the loading screen | `StartTestGame` left the loading screen up | it drops a loading screen whose files are loaded (checked on the web build, 2026-11-22, `?silent=1` in the Browser pane: on the loading screen `goto classic` went to the Classic level 1 introduction) | this branch |

### The word "Deluxe" removed (the user, 2026-11-18)

"Remove the "Deluxe" text from the main screen of any game it may appear in, to avoid misleading people about whose game version
they're playing." The word under the logo, on the loading screen and on the main menu, is not drawn (`DeluxeCover`: the cover is
made as the screen is drawn, no edited picture is committed), and the site's listing, the window's title and the page's title say
"Hammer Heads" (`games/hammerheads/CMakeLists.txt`). Where "Deluxe" finds files (the short name, the import folder names) it
stays. A deviation with a visible effect; its pairs are masked in `shotdiff.json` with this reason, and so are the removed
"More Games" button's row and bubble, the other deviation that shows on the menu.

### Judged, not changed (every flagged difference that is left)

- **A. The removed "More Games" button and "Deluxe"**: masked, above.
- **B. Edges of pictures the game stretches or turns.** The gnome's soft glow is a small copy drawn stretched (`GetGlowImage`), the
  trophy medal's rays and the light behind a gnome are turned (`DrawImageRotated`), the zoom wipe's hammer is scaled and a few
  captions are scaled: the original's blit and the framework's filter them differently, so a rim of pixels, a channel value or two
  to some tens apart, runs along every such edge (the same as the "Edge pixels" above, for the larger pictures). A pair flagged for
  this alone shows the same picture in the same place; the trophy pages, the middle frames of every wipe and the boards have it.
- **C. An animation driven by a counter, seen at another age.** The light behind a gnome turns 0.0075 radian for each update of
  the game over box and 0.0005 for each update of an introduction, the question mark over the next gnome pulses with
  `cos(0.025 t)`, the shop sign lights on `age % 200`, the figures of a finished level count up, a floater slides in: where the
  sandbox's original and the port are at another update of the same thing, they show another phase. Measured: the first
  scenario's introductions (level 2) match the port's at the very update (the best of the port's shots every two updates around
  it is the same update, for two shots); but the introduction of level 6 in `scr_lv1`, four cycles later, matches the port's at
  age 1240 where the shot was taken at 1495 (the best of a hundred shots every five updates, a clear minimum), and the Marathon
  clock of `scr_over` stands at 0:20 in the original where the port's, at the same scheduled update, is at 0:34. So a long
  scenario's timeline in the sandbox slips against the port's. **The cause is the sandbox's, found afterwards (2026-11-22):** its
  shots do not land on the update they are labelled with. `landed.tsv`, beside the sandbox shots, lists the counter each shot was
  taken at, and in `scr_over` (shots asked for at the mark 6003 plus the script's update) `classic_over` was asked for at 28003
  and landed at 25458, `marathon_over` at 42003 and landed at 37300 (4703 updates early), `marathon_wave_cleared` 36003 at
  34623, `tough_over` 16503 at 14375; the shots that landed on their update (`tough_points`, `classic_board` and the like) match.
  The game read through its own draw surface on the host (origrun's in-game frame reader with `reader_waits=hold`, `docs/ORIGRUN.md`)
  holds every shot at its update: the menu's 15 shots and the wipes' 129 are identical to the sandbox's, a Classic game's 33 differ
  only in the how-to-play movie, the loading screen and a particle, and `scr_over`'s eight differing pairs are exactly the shots
  the sandbox landed early. So the pictures differ in age because the sandbox's shot was taken at another update, not because the
  game counts differently: **not a difference of the port**. A sandbox shot is compared only where `landed.tsv` says it landed on
  its update, or against the port's shot of the update it did land at. To see it: `scr_lv1`'s `intro_level06` against
  `scr_phase6_port`'s shots (`looks.py scr_phase6_port 0 0 0 --shots --only port`).
- **D. The how-to-play movie** runs on the original's wall clock: compared by its frames (above).
- **E. Particles and sprites in motion** differ along their edges (above).

### Found on the way

- **The original crashes when a restart point holds a hammer of level 4 or 5.** The player files made for the scenarios first gave
  a hammer level of up to 5 for the late levels; the original crashed on the board (an access violation) where the port played on.
  No game gives a hammer above level 3, so it is a state no play reaches: nothing to fix, the files use at most 3.
- **A list's hash is printed without its leading zeros** (`HashProfileText`, as the original does): a made file with the full
  digest was dropped by both games as altered, which showed as a Marathon lock that came and went. `mkprofile.py` writes it the
  game's way.
- **The sandbox's lock** can be held by other sessions for more than ten minutes, and `looks.py` then gives up: a run whose
  original has no shots is run again (`build/logs/scr/rerun*.sh`).

### Left

- The 3D variants and full screen (see "Open"), the options notices (they follow the machine's display) and the camera trophy's
  photo (a screenshot of the board at its moment) are not compared beyond what the rows say.
