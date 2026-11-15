# Cosmic Bugs: the lite mod (2026-11-12)

Cosmic Bugs has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was taken
(`docs/MOD_ROADMAP.md`, "The lite mod"). The game's logic costs a few percent of the process, and the one part of it a lite change could
speed up costs about a percent of it, which no measurement can see.

## What was looked at

A throwaway cycle-counting build of the port (measured on 2026-11-12 and again on the merged main of 2026-11-15; the figures are the later ones) (`rdtsc` counters around the game's update functions, never committed), run by the headless runner
over two scripted games of 400 s each from a fresh start, `test/plays/g2.txt` and `g3.txt` (the games the identical-play pass compared with the
original: several boards, bonuses, lives lost). A step is one of the original's millisecond steps; the exe runs 1000 of them a second and the
runner drew 100 frames a second.

| Computation | What it costs | Faster method | Result |
|---|---|---|---|
| A whole step (`GameStep`, 0x40d6a0) | 3.3 to 4.4 K cycles (g2), 4.4 to 6.0 K (g3), 1000 steps a second: 3.3 to 6.0 M cycles a second, 3 to 5% of the port's 115 M a second on a board | | |
| The star list (`Star_Update`, 40 stars a board, each a call of `Obj_Update`) | 0.2 to 1.1 K cycles a step (switching the list off took a step from 3.9 and 4.4 K down to 3.2 and 3.5 K in g2, and from 5.6 and 6.0 K down to 4.8 and 4.9 K in g3): 0.2 to 1% of the process | one batched update a frame | lite (below), gain not measurable |
| `ParticleSystem_Update`, `Obj_Update`, `Fill_Update`, `Cell_UpdateFill` | 38.5, 48, 5.7 and 4.9 calls a step; each call tens of cycles | none worth a change | |
| `Enemy_Update`, `Shooter_Update`, `Beam_Update` | 2.6, 1.0 and 0.2 calls a step; 140 to 680 cycles a call (a counter is about 45 of them); `Weapon_Update` was not called in either game | none | |
| `Cell_Flood`, the flood a closed beam starts (the candidate the roadmap named) | 19 and 20 calls in 400 s, 8 to 11 K cycles a call: 0.14 and 0.21 M cycles over the 9 and 20 G of the two games | one flood is all the game does | |
| `Level_StartRandom`, a random board's making | 3.4 M cycles a board, once a board (2 and 4 calls) | none | |
| `GameDraw` into the 16-bit screen | 150 to 430 K cycles a frame in the runner | outside the lite mod (the picture) | |

The counters cost about 45 cycles a call, and a step makes about 146 of them, so the per-function figures above are upper bounds for the small
functions; the step's own total comes from a build with only the step, the level's update and the frame counted. The runs shared the machine
with other sessions' games, and the same build gave 3.3 and 3.8 K cycles a step in g2 on two runs, so a figure is good to about 15%.

## Why none is lite

The roadmap calls a change lite when it may give another result and a state the game can reach shows it. The only change here that does is
the star list's: the 40 background stars (`Level_AddStars`) move a few hundredths of a pixel a step (0.028 to 0.07: `RandRange(20, 50) / 25 *
0.035`), `Star_Update` adds their velocity every step, wraps a star at the right edge to x = -2 and one that left the top to the bottom, and
nothing but `Star_Draw` reads a star. One update a frame of ten steps' velocity would give the same positions up to float rounding and would
wrap a star up to nine steps (under 0.7 px) later than the original does, so a frame of any board can differ by a star's sub-pixel position,
and never by anything the game's rules, random draws or timing see (stars draw no random number after their placement at the board's start).
That difference is reachable on every board (the starfield wraps all the time), but it gains at most 1% of the process and often a fifth of that.
Every other computation in the table is a single walk of a short list or one flood over a board a few dozen cells wide, executed at most a few
times a second, so a faster method would give the same answer and gain nothing a measurement can see.

## Why none goes in the performance pass

The port's processor time is already under its original's (README, "The processor-time target"; measured again on the merged main of 2026-11-15, seven alternating rounds, 800x600, sound off: a median of 115 M
cycles a second on a board against the original's 118 M, the rounds ranging from 65 to 157 M for the port and 101 to 144 M for the original), and what is left is the display's and the audio mixer's, not the game's logic. A change goes in only when alternating pairs of runs can
show it cuts the processor time, and the process's cycles spread by tens of percent between runs of one build on this machine (the seven port rounds above:
65 to 157 M a second), a hundred times and more the star list's share.

The lite-mod step is therefore done with no mod.
