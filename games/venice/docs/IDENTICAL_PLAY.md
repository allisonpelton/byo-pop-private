# Venice Deluxe: the identical-play pass

Written for the user. Phase one (the game plays out the same in the original and the port) is in progress; this file
grows with each phase.

## Phase one: plays identical

### What is compared and how

The original (`VeniceGame.exe`, the Steam copy's carved exe, run from a copy under `build/`) runs under origrun on its virtual
clock with scripted mouse and keys, so a run takes seconds and every run of a script plays the same. origrun reads the game's
memory (`peek`) at the end of every update: the tick count it has run to, the random seed (0x5b5174), the last scene's tick
count, the shooter's x, the level's tick count, hits, shots, the profile's score, the y of the middle layer and of the plate
layer, and how many unfilled slots each colour has (11 colours and the total). The port is then given the same run: the
number of ticks each frame ran (a frame that was late runs more than one tick), and every input at the frame the original's
hold of it came at. Its trace (the `trace`, `ticks` and `after` test commands in `GAME_NOTES.md`) writes the same values,
and `games/venice/tools/idplay/idplay.py` compares the two frame by frame. The seed is the strict one: a single random number
drawn in a different place, or not drawn, shows at once and stays different for good.

Two details of the harness matter. The original draws after every update, and its shooter puts the pointer under itself
when it draws (`Shooter::Draw`), so the port runs with `-drawevery`; without it the same step ran inside the next update and
the shooter followed the pointer a frame late. And the original on rundesk's desktop runs in software drawing (the layer merge
adds the backdrop repeats there), so the port runs with its registry's `Is3D` at 0; the original's 3D mode is run over
dgVoodoo2 (`docs/ORIGRUN.md`, "Hardware acceleration") by the scenario `journey3d`, below.

The scenarios that start from a player with the modes unlocked (Flood, Survival, Trick Shot) are given one by the port: it
types the name, runs `unlock all`, `goto menu` and `save` and quits, and both games then start from that profiles file (the
original's is put where it reads it, the previous one removed first, since a player left by an earlier run changes the name
screen).

The input does not come from a person: `bot` (`Bot.cpp`) is the port's player, which reads the game's state, aims at an unfilled
slot, and presses with the real input path's moves, presses and releases; its inputs are written to a file and given to both
games. The bot's own play is not what is tested (it loses the later levels), only that both games, given the same input,
play the same.

| scenario | frames compared | what it covers |
| --- | --- | --- |
| `idle` | 1501 | the loading and the name screen |
| `name` | 2801 | a name typed (key edges) and entered, up to the tutorial's start |
| `tutorial` | 6501 | the tutorial played by the bot: its lessons, 31 shots, 15 hits |
| `journey` | 60501 | the tutorial, then Journey levels played by the bot (`bot ... continue` also clicks away the summaries, the level select and the cupid's messages): first level in play, a lost level (the shooter sinks), its tries, the summaries |
| `flood` | 30001 | a player with every mode unlocked (the port makes the profile with `unlock all` and `save`, both games start from that file), the menu clicked to Flood and its new game, the bot playing the levels that follow (the level's explanation and the summaries clicked away) |
| `survival` | 20001 | the same, the menu clicked to Survival |
| `trickshot` | 20001 | the same, the menu clicked to Trick Shot; the bot gives each attempt another x, since a shot goes by way of the bumpers |
| `journey3d` | 16101 | the `journey` script with the original in its 3D mode (Direct3D 7 over dgVoodoo2) against the port's default: it matches for 16101 frames, and the two part at frame 16144 (the oddity below) |

### Differences found and fixed

| difference | cause in the original | what the port does now |
| --- | --- | --- |
| The tutorial's first plate had 8 slots, the original's 4 | `SceneObject::Reset` (0x425ce0) sets the object's selectable flag to 0; a slot is selectable only when its data tag says so, and `Plate::SetUpSlots` drops the random slots it made once the plate's own slots load | `SceneObject::mSelectable` starts at 0 |
| The tutorial's lessons found another plate than the original's (the bot made no shot) | the lessons ask for the lowest plate on screen (`FindLowestOfType`, 0x408710), the largest y; the port had asked for the highest (0x408790) | four lessons call `FindLowestOfType` |
| After a lost level with no tries left the shooter rose again and the jewel in its row was removed a frame early, so one random number of the ready jewel's sparkle was not drawn (seed differs from journey frame 17860) | `Shooter::Sink` (0x419ee0) reads the name of the shooter's last stage to see whether the sink's end is already there (`param_1[0x16]`, the tail); the port read the current stage's | reads the last stage |

(The commits are those of this pass on the branch `claude/venice-05486e`: the first after the merge request of the port.)

### Oddities skipped

**The original's 3D mode against the port (found with `journey3d`).** The same inputs on the original in its software mode
match the port for all 20001 frames of the script. In its 3D mode (Direct3D 7 over dgVoodoo2) the original matches for the
first 16143 frames and then parts: the jewel thrown from x 281 at original frame 17922 falls past the bottom edge one update
sooner than the port's (the box test of `Jewel::KeepInBounds` against the screen, with a margin of 0.24 pixels), the splash's
58 random numbers are drawn a frame earlier, and the seeds differ from then on. Before that the 3D original's jewels and plates
are the same to a float's rounding (a jewel that flies straight up agrees to 0.001 pixel) except one thing: the plate that
wobbles after a hit (plate 8) is already 0.014 degrees off at original frame 17918, a jewel's bounce off one of its slots leaves
0.05 degrees off, and that grows to 1.6 pixels in the falling jewel's height by the frame of the test. What I tried: peeks of
the plates' world y and rotation, and of three jewels' positions, around the frames; the port's plate and jewel values for the
same frames; the same inputs in the original's software mode (0 differ); and the port's sine filter rounded to float the way
a 24-bit x87 would (no change in plate 8). The framework notes infer that Direct3D 7 leaves the x87 at 24 bits (no
`DDSCL_FPUPRESERVE`), which would make every double expression of the original round like a float in 3D, but nothing here shows
that it is the cause. Not chased further: the plate's first differing value needs the original's plate 8 by update from its last
hit on, and the port would need a 24-bit mode for every double expression to follow it if that is the cause. `journey3d` ends at
frame 16100, before the difference. See `GAME_NOTES.md`, "Oddities".

### Deviations that remain

As `GAME_NOTES.md` lists them: none changes play.

### Left in phase one

Phase one ends here: the games play out the same in every mode, for the length of the scripts. The levels past the first few
of each mode, with their bumpers, tossers, warps and baskets, and the power-ups, wings and tries that the bot never reaches,
come in the coverage check (the bot loses early, and a level is reached with a profile that has it unlocked), and the focus loss
and regain with phase two.
