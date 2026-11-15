# Typer Shark! Deluxe: the lite mod (2026-11-15)

Typer Shark has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was taken
(`docs/MOD_ROADMAP.md`, "The lite mod"); the measurements are GAME_NOTES section 11, "Second pass".

## What was looked at

The game's logic is a small share of what the port spends. In the performance pass's scenario (3D, 2,000 updates, 20 s) the whole process costs
3.5 to 4 G cycles and the game's update, `UpdateFrames`, 66 M of them (under 2%); the sampler puts `Board::Update` at about 0.1% of the cycles. What
the update does, and why none of it is a candidate:

| Computation | Where | Cost | Faster method | Result |
|---|---|---|---|---|
| Creature, diver and projectile updates | `Board::Update` and the creature classes | part of the 66 M, a few hundred cycles a creature | none needed | |
| Which creature a typed letter picks, the clam list | `Board::KeyChar` | one short scan a key | none needed | |
| `WinWSApp::GetWord`, a random word that is not in the used set and whose first letter is not taken | `WinWSAppGame.cpp` | a few draws a spawned creature, a few hundred spawns a minute at most | choose from the words left | another stream of random draws, so another word order: lite in principle, but unmeasurable |
| The wave spawner (`SpawnWave`) | `BoardWaves.cpp` | once a wave | none needed | |

Every list the logic walks is short (a screen holds a dozen creatures, 26 letters, at most 7 word lists), and nothing searches or solves.

## Why none is lite, and why none is taken

`GetWord` is the one place where a faster method would give another result (the rejection loop draws until it meets a free word and a free
letter, and every draw is part of the game's random stream, which the identical-play pass keeps in order). A change there is a lite change
only if it saves processor time, and it cannot: a spawn costs a few hundred cycles of the 3.5 G in 20 s, so no round of alternating pairs could show a gain,
and the roadmap requires one of every change. The other computations give the same result in every state under any faster method, so they
would be exact changes for the performance pass, and none gains anything a measurement can see either.

The processor time that is above the original's is the 3D present path and the Direct3D 11 driver's thread (GAME_NOTES section 11), which is not the game's logic, and a lite change keeps the picture
and the pace. The drawing is outside the lite mod for the same reason.

The lite-mod step is therefore done with no mod.
