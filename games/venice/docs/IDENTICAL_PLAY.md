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
adds the backdrop repeats there), so the port runs with its registry's `Is3D` at 0; a comparison against the original's 3D mode
needs dgVoodoo (`docs/ORIGRUN.md`) and is listed below under what is left.

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

### Differences found and fixed

| difference | cause in the original | what the port does now |
| --- | --- | --- |
| The tutorial's first plate had 8 slots, the original's 4 | `SceneObject::Reset` (0x425ce0) sets the object's selectable flag to 0; a slot is selectable only when its data tag says so, and `Plate::SetUpSlots` drops the random slots it made once the plate's own slots load | `SceneObject::mSelectable` starts at 0 |
| The tutorial's lessons found another plate than the original's (the bot made no shot) | the lessons ask for the lowest plate on screen (`FindLowestOfType`, 0x408710), the largest y; the port had asked for the highest (0x408790) | four lessons call `FindLowestOfType` |
| After a lost level with no tries left the shooter rose again and the jewel in its row was removed a frame early, so one random number of the ready jewel's sparkle was not drawn (seed differs from journey frame 17860) | `Shooter::Sink` (0x419ee0) reads the name of the shooter's last stage to see whether the sink's end is already there (`param_1[0x16]`, the tail); the port read the current stage's | reads the last stage |

(The commits are those of this pass on the branch `claude/venice-05486e`: the first after the merge request of the port.)

### Oddities skipped

None yet.

### Deviations that remain

As `GAME_NOTES.md` lists them: none changes play.

### Left in phase one

Flood, Survival and Trick Shot played the same way (a profile with every mode unlocked, the menu clicked to each mode), more
Journey levels (the bot loses after the first few), the 3D mode of the original against the port's default, a run with a
window that loses and regains the focus, and a run with tries, wings and the power-ups.
