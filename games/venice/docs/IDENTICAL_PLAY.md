# Venice Deluxe: the identical-play pass

Written for the user. Phase one (the game plays out the same in the original and the port) and phase two (it looks and sounds
the same, screen by screen, and keeps its focus the same) are done (the 3D original's tour last, "The 3D original"); this file
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

## Phase two: looks identical

### What is compared and how

Nine tours of the game, each a scenario of `games/venice/tools/idplay/scenarios/shots_*.py`, give 207 screenshots, and each
tour is run by the original and by the port with the same inputs at the same updates. The original runs inside Windows Sandbox
under origrun on its virtual clock (the only place origrun takes a shot, `docs/ORIGRUN.md`), and each shot is taken at the
update the script names (a `waitshot` after a focus loss, where a plain capture of the window shows nothing); the port runs
natively under rundesk with `-fast -drawevery -renderscale=1`, the original's tick count for each frame, and takes its shot
at the same update. `tools/shotdiff` compares the pairs (`docs/SHOTDIFF.md`): a pair is a match, or it differs only faintly (no
pixel group beyond the tolerance, no visible difference after a blur), or the differing groups are flagged.

| tour | shots | what it covers |
| --- | --- | --- |
| `shots_name` | 6 | a new player's first screens: the name box, the typed name, the tutorial's start |
| `shots_tutorial` | 6 | the tutorial, played by the bot, a shot every 1000 frames |
| `shots_menus` | 31 | the main menu and every screen it leads to (options, help, credits, high scores, mode select, exit confirmation, the player's menu), each while its fade runs, settled and closing |
| `shots_ingame` | 18 | a level in play: the in-game menu (fading and settled), its help, options and confirmation screens, the game after Resume |
| `shots_flood` | 30 | Flood, a shot every 1000 frames of the bot's play, with the explanation and the summaries |
| `shots_survival` | 20 | Survival, the same |
| `shots_trickshot` | 20 | Trick Shot, the same |
| `shots_journey` | 60 | Journey for 60500 frames: levels, a lost level and its tries, the summaries, the cupid's messages |
| `shots_focus` | 16 | the window losing the focus and getting it back on the main menu, at a level's explanation and in play, with a shot in the lost state (the paused sign) and after each regain |

The result: every pair is a match or a faint pair, none is flagged: 99 matches and 108 faint pairs. (The Survival tour was run again after the vertex clamp in the table below: 3 identical and 17 faint of 20, none flagged.) The faint pairs are single
pixels and thin edges: the largest is 696 differing pixels of 480,000 (the vase jewel of `survival_12500`, at most 24 levels off
in a colour), and in the 108 pairs together 1,051 pixels are more than 32 levels off, each alone. They are at the antialiased
edge of a picture drawn through a transform, at a sparkle's blend and in the glyphs of text drawn through a transform.

The sound is compared as the original's BASS calls (the logging BASS beside the exe, `tools/bassproxy`) against the port's
sound log, by `tools/idplay/soundcmp.py`: every play, every volume call of a looping sound (the music) and every stop, in
order, the sounds named by their file's checksum, and the time of every play (the original's milliseconds against the port's
update count; no play of any tour is more than one update from the original's, counted from the first play of each, except
in the focus tour, where the original's counter goes on while the window has no focus and the port's updates stand still, which
is a constant 8 updates after the first loss). The tours `shots_menus`, `shots_name`, `shots_tutorial`, `shots_ingame`,
`shots_survival`, `shots_journey` (13,673 events and 1,400 plays) and `shots_focus` (the pause and the resume of every loss and
regain, the music's volume calls on both sides of them, run with the original inside the sandbox) match with no difference;
`shots_trickshot` and `shots_flood` differ in one thing, an oddity below.

The web build is compared with the native one, not with the original: each tour runs under Node (`webshots.py`, the same
scenario, the same ticks, the web build of the port) and `tools/shotdiff` compares its shots with the native port's. Of the
207 pairs 201 are identical and 6 are faint, none is flagged: menus 30 identical and 1 faint of 31, name 6 of 6 identical,
tutorial 5 and 1, ingame 18 of 18, flood 30 of 30, survival 19 and 1, trick shot 20 of 20, journey 57 and 3, focus 16 of 16
(its losses and regains included). The web Survival tour first ended in a crash of the web build (exit status 3), and the
native Survival run's trace stopped short for the same cause; both are in the table below (the vertex clamp). The first web
Trick Shot run differed in 19 pairs of 20 for a fault of the harness, not of the game: it started from the profile the native
run had saved at its end instead of from the starting profile.

### Differences found and fixed (phase two)

| difference | cause in the original | what the port does now |
| --- | --- | --- |
| Every flipped picture of the layer merge was missing (the sky's mirrored repeats, in bands) | the original draws a flipped picture with its software triangles, which the framework draws only once the game has registered them | `SWTri_AddAllDrawTriFuncs()` when the app is made |
| The mirrored sky's repeats were not flipped as in the original | `mFlipY = aMirrored != 0` in the layer merge's repeats | as the original |
| Where no picture covered a band it was see-through, black in the original | the original's software surface has no alpha | a band on a display that is not accelerated starts opaque black |
| The layer merge drew the bands from the pictures that stay alive, not into pictures | 0x40de30 draws every picture into 800 x 1024 pictures, with the second copy of the picture draw (0x442490) | the bands are made as the original makes them (`DrawImageRecordOnTarget`, `gRenderTarget`) |
| The sky's last column was black in every band (`journey_10500` and others) | the original's software triangle clip is the rectangle's own far edges (0x504860 passes x + width and y + height), the released source's is one pixel inside | `SWTri_SetClipToFarEdges(true)`; a framework switch, off for the other games |
| The loading screen's sparkles were too bright | a transformed picture on the original's software surface blends normally whatever the draw mode says; only the accelerated target adds | `DrawImageRecord` draws it in the normal mode when the display is not accelerated |
| Text, sprites and jewels drawn through a transform sampled at other places than the original's, an edge a fraction of a pixel off | the original's software screen draws a transformed picture with its own triangle code straight on the surface | `mSoftwareTransforms` on whenever the display is not accelerated (Bejeweled 2 and Rocket Mania do the same) |
| The wings text of the wings power-up was on the wrong object | 0x417b90 calls `SceneObject::AddFilter` and `SetAttribute` (0x428610, 0x428700) directly, not the layer's virtual versions | direct calls |
| A sprite's frame was different in the web build, a hair under or over a whole number | the original's x87 sine and cosine give the correctly rounded value; the C libraries of the two builds differ from it in the last bit for about 3 % of the angles | `VeniceSin` and `VeniceCos` (`VeniceMath.cpp`), the same bits on every platform |
| The bake drew the objects that the culling had skipped | the culling test is not made when drawing into a band | `mNoCulling == 0 && gRenderTarget == nullptr` |
| A heap overflow in the framework's software triangle code that ended the Survival tour in the web build (a crash) and cut the native run's trace short | with the far-edge clip, a vertex of a transformed picture at the display's far edge lay outside the clip, and the code that copies the clipped triangle's vertices wrote past the end of its buffer (found with AddressSanitizer: `SWDrawShape`) | with that clip the vertices are clamped to the edge (`shared: SWDrawShape...`, f6417d47); only the far-edge mode changes, so the other games draw as before |

Two things of the harness were wrong and are fixed, not of the game: a scenario's end could come before its last shot, so that
the port ran frames the original had not (`SHOT_MARGIN`), and the ticks after a focus loss, which the original does not run
(the counter keeps counting while no update runs, and `GotFocus` resets it), were given to the port as one more than it should
run.

### Oddities skipped (phase two)

**The old level's destructor sets the music channel to volume 0 for one update (Trick Shot and Flood sound).** Each time a
level ends and the next one has started the same music, 0x40ea20 (the destructor) sets the volume of the music's channel to 0,
and BASS gives the new level's replay the same channel handle as the old level's stopped one, so the call reaches the music that
is playing; the next update's fade sets the volume back. The port's old level finds its own voice stopped and does nothing. It
is the 4 ms of an update, 15 times in the Trick Shot tour and 3 times in the Flood tour, and there is no other difference in the
sound of any tour. To make it the same the port's sounds would have to share one voice handle for each file.

### Deviations that remain (phase two)

As `GAME_NOTES.md` lists them.

### The 3D original (phase two, night of 2026-11-14)

Every tour above compares the original's software mode (its 2D display) with the port's 2D display. The original's 3D mode
(Direct3D 7 over dgVoodoo2) cannot run in the sandbox (the night of 2026-11-12: with `--sandbox-vgpu` its update counter stayed at
-1 and no shot came, as `docs/ORIGRUN.md` says of every original there), so `shots_menus3d` runs on the visible desktop under
`tools/visrun.py` (`docs/VISRUN.md`), between 22:00 and 08:00. Venice was visrun's first live run. The driver is
`games/venice/tools/idplay/visshots.py` (`prep`, `original`, `port`, `compare`, `diff`), the original's settings are
`games/venice/tools/visrun/venice.toml`, and one run of the original is about eight minutes: 9750 updates, 31 shots.

**What is compared.** The original loads on a thread of real time, so the update its loading ends at differs from run to run
(1132 in this run, 936 in the 2D one); the step file therefore peeks the game's values at every update and puts the inputs and
shots at update 1500 plus the scenario's frame. The first update after the loading is the port's frame 0, and the port is given
the ticks each update ran, takes each input and each shot at the update the original's step landed on, and writes the same
values (the random seed, the last scene's tick count, the ticks) for every frame. `compare` sets the two side by side: all 8251
frames agree in all three, so the port's 3D mode plays this tour out as the original's does, down to every random draw. Then
`tools/shotdiff` compares the 31 shots.

**What was different, and fixed.**

The 3D original's picture sits half a pixel from the port's: Direct3D 7 puts a pixel's centre at the whole-number coordinate
and the port's display (SDL) at the half, so every transformed picture (the clouds, the birds, the gondola, the scene's
objects) was half a pixel up and left and blurred by the filter, in all 31 shots. `VeniceDisplay` (`VeniceApp.cpp`), the
display the app creates, adds 0.5 to the position of a transformed picture when the 3D mode is on, as Typer Shark's display does;
the 2D display is untouched, which `tools/check.sh venice` (the frozen baseline) confirms. Commit `ea5baf4d` (the screens hash of `tools/check.sh`'s venice suite was refrozen in `b3926c11`; the states are unchanged).

Two things of the harness were wrong and are fixed, not of the game. The tick schedule took the tick counter's step as the
ticks an update ran only up to 100 (the widget's own limit), but the original's counter and scene advance by up to 235
between two of visrun's updates: the peek can fall inside an update, so more than one ran in between; the step is the ticks at
any size, and a jump of about 29 million is a reset. And the frame where the original resets its tick base (a scene inserted by
an action, here the loading screen's end) reads 1 in the counter, which hides how many ticks it ran. What is known is the
clock's move over that frame and the next, and the new scene's tick count in the next; for the credits' loading screen it is 22
ticks, one of them after the insertion, so the original ran 21 ticks in the frame that inserted the credits and the loading
screen ended on the same tick as in 2D (79). Before this was understood the schedule gave the port one tick there, the port
looked 20 ticks late, and the shots of the credits were taken 20 frames later as a workaround; both are gone.

**What is left.** All 31 pairs differ only faintly (shotdiff's class below the visible threshold): 0.10 to 0.85 % of the
pixels, at most 62 levels of 255 (99 in the mode select's line of description). The differences are the edges of
text sprites, and in the main menu the first row of each line of text, where the port's glyph edge is brighter by 1.7 levels on
average; between the lines of text the two pictures are the same to the pixel, and a sub-pixel fit of the text puts the best
shift at 0. It is the texture filtering of the original's Direct3D 7 over dgVoodoo2 against SDL's, and not a position or a
timing, and nothing in it is visible. The coverage check, the real-input driver with the page check and the fixes mod, which
this section used to say came after, are done (`GAME_NOTES.md`).
