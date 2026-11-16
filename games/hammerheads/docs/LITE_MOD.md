# Hammer Heads Deluxe: the lite mod (2026-11-15)

Hammer Heads has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what the measurements
bound, and why nothing was taken (`docs/MOD_ROADMAP.md`, "The lite mod").

## What was looked at

The game's logic is a few dozen gnomes at a time on a board of at most a hundred cubbies: each update walks the creatures and the
cubbies once, steps each one's small state machine, tests the hammer's swing against the creatures it can reach (`Avatar::Probe`, a
walk of the board's objects with a mask test), moves the floaters, coins and debris and counts the score. There is no search, no
solver, no fill and no pathfinding: the gnomes' walks are timed scripts from the level rules, and the one place that picks among
candidates, the choice of the next cubby for a spawn, draws from the game's generator (reordering that would change every game, and
it is the strict value of identical play). The performance pass lists no "Lite candidates": it turned down no speed-up for changing
what the game does.

## What the measurements bound

The cycle split (`GAME_NOTES.md`, "Performance") puts the port's own update at 0.014 million cycles an update, in a stretch of
level 1 in play, against 2.2 to 2.7 million for the whole process: about 0.6% of it, 1.4 million cycles a second at 100 updates a
second, 0.07% of a core. The rest is the picture and the framework's present. The pairs of the same exe spread by hundreds of times
that, so no change to the logic could show in a measurement, which is what a lite change must do.

## Why none is taken

A change goes in only when it measurably cuts the processor time of a stretch of play where it runs. The whole of the game's
logic is under a thousandth of a core, and a lite change may not touch the picture or the frame rate. The lite-mod step is done
with no mod.
