# Venice Deluxe: the lite mod (2026-11-13)

Venice has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was taken
(`docs/MOD_ROADMAP.md`, "The lite mod"); the performance numbers are in `GAME_NOTES.md`, "Performance".

## What was looked at

The game's logic is a tree of scene objects (a level is a layer of layers holding plates, their slots, the shooter, the jewels in
flight, obstacles and effects) that is updated every 4 ms, a shot that flies and fills a slot where it lands, and a few scans run
each update. It has no solver, no hint search and no fill: the bot behind the `bot` test command is test code, not part of a
player's run. The cycle split of the port (`GAME_NOTES.md`, "Performance": 4000 updates of Journey in play, a window of the game's own
size) puts the whole update, the scenes' logic and the music's tick, at 0.036 M cycles an update, 1.4% of the process's 2.61 M; in the
port's 3D display it is 0.029 M of 0.749 M, 3.9%. The picture, its present and the GPU driver's thread are the rest, and a lite change
keeps the look, the frame rate and the pace, so it may not touch them.

| Computation | What it does | A faster method | Result |
|---|---|---|---|
| `SceneObject::Update`, `UpdateChildren` (0x426dc0, 0x426f10), `UpdateWorldValues` (0x427df0), each update, every object | walks the tree and works out each object's place from its stage and its parent's | none: the places are the game's state | |
| `Slot::CheckJewel` (0x41c5a0), from `Slot::Update` (0x41c3b0), and `Jewel`'s on-screen tests (`HitTest` 0x428bd0) | each slot of the level asks whether a jewel in flight is on it | a grid of the slots' places, rebuilt each update | exact where it names the same slots; the order in which slots meet a jewel decides which one it fills, so it is not taken |
| `Plate::CheckDanger` (0x413f90), `Plate::AllSlotsFilled` (0x414b80), `ProgressBar::CheckVictory` (0x412340) | scan the plates and slots for one that is full, sunk or not yet filled | counts kept up to date | exact |
| `ScrollPoint::Update` (0x41bc00), `FindLowestOfType` | looks for the lowest scroll point and plate among the level's children | the lowest kept | exact |
| `Level::Update`'s clouds, particles and effects | each object's own small update | none needed | |

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. The whole
update of the stretch is 1.4% of the process in the 2D display, and its parts are smaller still: no single computation in the table
reaches a tenth of a percent of it. The five-pair medians of the measurement resolve a few percent of the process at best, and the
port's own runs spread by tens of percent on a machine busy with other sessions' games (2.1 to 4.4 M cycles an update in the 2x window,
the GPU driver's thread). A change that exact (the counts kept up to date, the lowest point kept) is not lite anyway: it gives the same
result and belongs in the port's performance pass, which it would not earn. The one that could give another result, the grid for the
slots' tests, saves far less than the measurement can see and puts the order of the slot a jewel fills at risk. The cost that is left is
the picture and its present.

The lite-mod step is therefore done with no mod.
