# AstroPop Deluxe: the lite mod (2026-11-12)

AstroPop has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was taken
(`docs/MOD_ROADMAP.md`, "The lite mod"); the detail, with every function's address and measured cost, is GAME_NOTES section 11b.

## What was looked at

The logic that runs every update and the searches that run on events, measured with a throwaway cycle-counting build of the port (the
performance pass's scenario `classic11`, 2D, `-fast`, 2,477 updates; the whole process costs about 2.9 M cycles an update):

| Computation | Cycles | Faster method | Result |
|---|---|---|---|
| `Board::Update` as a whole | 16.7 K an update (0.6% of the process) | none needed | |
| `UpdateBricks`, `Brick::Update` | 6.7 K an update (36.7 calls at about 135) | skip bricks with nothing running | exact only if an idle update changes nothing; unproved, under 0.23% |
| `AnyMoveLeft` | 3.6 K a call, 127 calls | memo on a version of the field | exact |
| `CollectConnected` | 0.8 K a call, 240 calls | visited grid, same direction order | exact |
| `DropUnsupported` | 1.2 K a call, 74 calls | per-column pass, same insertion order | exact |
| `NoRoomLeft`, `FindBottomRow` | 0.9 K and 84 a call | one `FindBottomRow` call, a fill count per column | exact |
| `CountMovingBricks`, `IsInGroups`, `IsInFallingGroup94`, `HasCell`, `SortFallingGroups` | 43 a call and under 3 K | a count kept up to date, sets or grids | exact |

The searches together cost about 1.4 M cycles in the run's 2,416 updates: 0.6 K an update, two parts in ten thousand of the process.

## Why none is lite

A lite change is one that may give another result, proved by a state the game can reach. Each method above gives the same result as the
original's in every state: `AnyMoveLeft` is a pure function of the field, the hand, the set size and the moving bricks; the visited grid
and the per-column pass keep the original's order, so they gather the same cells and insert the groups in the same places (also when
`CollectConnected`'s limit cuts the flood short); the other scans return values that do not depend on how they are looked up. So there is
no state to show where they differ: they are exact changes, which belong in the port's performance pass if they earn a place there.

The variants that would differ (a breadth-first flood, which gathers another set of cells at a cut; a skip of idle `Brick::Update` calls
that leaves out a shaking brick's `AstroRand` draw) gain nothing a measurement can see, so no difference was looked for.

## Why none goes in the performance pass

The most any of them gains is 0.45 M cycles over a run of about 7 G (`AnyMoveLeft`), under one part in ten thousand of the process. A
stretch of one build spreads by plus or minus 30% between runs and the median of five pairs resolves a few percent at best, so no change
could be shown to cut the processor time, and the roadmap requires that of every change. The one place where the port costs more than its
original is the 3D stretch's present path and the display driver (GAME_NOTES section 11), not the game's logic, and a lite change keeps
the picture and the pace. The drawing (`Board::Draw`, 0.62 M cycles a draw) is outside the lite mod for the same reason.

The lite-mod step is therefore done with no mod.
