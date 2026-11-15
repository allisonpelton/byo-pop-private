# Word Harmony: the lite mod (2026-11-15)

Word Harmony has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was
taken (`docs/MOD_ROADMAP.md`, "The lite mod"); the like-for-like processor time is in `README.md`, "Performance, second round" and
"third step" (the port's 3.26 G cycles a minute against the original's 11.81 at the game's own window size, windowed).

## What was looked at

The game's logic is word lookups in two trees (the match tree, `WordNode::find`, 381,205 nodes; the hint tree, `AnagramNode`, 61,691),
the ball grouping and scoring of the board (`WordBallSystem::groupBalls`, `scoreGroup`), and the engine's per-frame updates of the
widget tree (`Container::update`, particles, selectors). The candidates a lite change would name are the two searches: a lookup
that stops at the first word instead of walking every branch, and a hint search that stops at the first anagram instead of
collecting them all.

They were timed where they run, by the sampling profile of the scripted minute (`tools/profile/cpu.py`, tour 2's TSUNAMI game in
real time, 3,020 samples of running threads, on the build after the surface display and the engine's move to `engines/sprout`):

| Computation | Share of the samples |
|---|---|
| `WordNode::find` (the match tree's lookup, run for each word a player forms and for the hint) | 1.0% |
| `WordBallSystem::groupBalls`, `scoreGroup` | 0.3% |
| `Container::update` and the other widget updates | 1.9% and under 0.5% each |
| `AnagramNode::add` (building the hint tree at load) | 0.1% |
| the drawing (`renderBlit`, `makeBlit`, `ScreenGraphics::finish`) | 21% |
| the framework's present, the graphics driver's thread and the system calls under them (the run was full screen, the renderer mode) | about 50% |
| the music's MP3 decoding | about 2% |

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. The game's
two searches together are about 1% of a minute that costs 3.3 G cycles windowed, so about 30 M cycles: under what the five-pair
medians of the measurement resolve (the port's own runs spread from 2.7 to 4.8 G in the same window), so no change to them could
show. A lookup that stops early would also give the same answer for a word the player forms (a word is in the tree or not), so it
would not be lite but a performance-pass change, not earned at 1%. The hint's list of anagrams is shown to the player, so stopping
at the first would change what a hint says; that is a rule of the game and not a speed-up, and a lite change keeps the game's rules.

The lite-mod step is therefore done with no mod.
