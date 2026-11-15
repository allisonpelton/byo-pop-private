# Hammer Heads Deluxe: the identical-play pass

Written for the user. Phase one (the game plays out the same in the original and the port) is done for the scenarios below;
phase two (it looks and sounds the same) has not started. This file grows with each phase.

## Phase one: plays identical

### What is compared and how

The original (`HammerHeadsGame.exe`, the user's copy staged under `C:\byo-og\hh2\game`) runs under origrun on its virtual clock
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

Together the runs stood up every kind of creature in a cubby but the multiplier (the gnome, the woman, the strong, the bomb, the
bubble head, the hide, the forky chap, the silver, the cupido, the heart, the coin and the big coin, the chest, the clover, the
ice cream and the duck of the bonus, the strawberry, the rabbit, the king, the save, the napoleon, the power-up, the switcher, the
shop's and the challenge's gnomes), and the screens game, shop, challenge and the menu. Each run's own coverage (levels, phases,
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

The game won sequence (the end of level 25: the Tough Cookie notice, the high scores, the game-won page) is not reached: level 25
did not end in the 28000 updates of `classic_nd25` (why was not looked into), and a run started at it would need a cheat or a
patch that ends the level. The multiplier gnome was not seen in any run. The runs do not record which boxes came up (a
challenge's or a bonus round's finish, the high score entry after a lost game), only that the challenge screen was entered and
left in `classic_nd10` and `classic_nd15`. The flying and the bouncing gnomes' own state is not in the log (only its effects). A
save, a continue and a game left and resumed (the saved game's file) were not run. These belong to the coverage check before
the final merge, with the focus loss and regain and the Tough Cookie and Marathon runs at later levels (`jumpto` works in both
modes); the looks and the sound belong to phase two.
