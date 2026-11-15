# Alchemy Deluxe: the lite mod (2026-11-14)

Alchemy has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at and why nothing was taken
(`docs/MOD_ROADMAP.md`, "The lite mod"); the like-for-like processor time of the port and the original is in `GAME_NOTES.md`, section 8,
"Performance" (a minute of play costs 4.6 to 5.3 s of processor time in the port, 4.2 to 7.6 s in the original, and the game's own code
is under 3% of it; the rest is the framework's present path and the graphics driver's thread).

## What was looked at

The game's logic is a board of 9 columns by 8 rows, a piece to place, a forge that fills, the rules for where a piece fits (a neighbour
of the same colour or shape, a skull, a bomb), the points, and the pictures that move: the glow of a newly lit tile, the sparkles, the
sparks of a bomb and, when a piece is discarded, its melt. It has no search and no solver: every check is a scan of the 72 cells. The
roadmap's survey named only those small scans, so they were timed where they run, and so was the one thing that draws many random
numbers, the melt.

Five temporary timers, not committed, wrapped `SexyApp::UpdateFrames` (the framework's update, which runs `Board::Update` and every widget),
`Board::Update`, the melt's branch of it, `Board::DetermineValidMoves` and `Board::MakeRandomPiece`, in a build of the port (32-bit, -O3).
They read the processor's time-stamp counter, calibrated against the clock when the game ended (2.112 GHz). The games were the check's
bot games (`tools/check.sh alchemy`: `-fast -autoplay=1 -autoplaycap=20000`, with seeds 1 and 3) of the strategic mode and of time
trial, and the tutorial; the strategic game of seed 1 twice. `-fast` plays the updates as fast as it can with a draw every tenth, so
only the cost of an update counts here, not the wall time. The machine was shared with other sessions' game processes, so the same code's
cost moves by a factor of four from run to run (the melt's update: 14.4 us in the quietest run, then 24.6, 33.7, 51.1 and 63.4 us),
as the notes' section 8 found for the whole game; the quietest run is read as the cost and the busiest as the worst case.

| Computation | What it does | Cost | A faster method | Result |
|---|---|---|---|---|
| the discard's melt, in `Board::Update` (0x406f2a, the branch that ends near 0x407c3f) | each update for 250 updates after a discard (the shrink goes up by 40 to 10,000): the 50 x 50 picture of the piece goes darker and redder, an index array of 2,500 is shuffled with 2,500 draws of the C library's `rand`, as many pixels as the shrink count (40 more each update, in the end all of them) slide down a row, and a sideways blur runs over the whole picture | 14.4 us an update in the quietest run, 63.4 us in the busiest; 3,052 melt updates in the 12,497 of the strategic game of seed 1 (the bot discards about six times a minute, 12 times in that game) | shuffling only as many indexes as will slide, or another generator | can differ: the `rand` stream moves differently, and the next game's seed is two draws from it (`Board::Board`), so the next game's boards would differ, and so would the pattern of the melt. It would be lite, but see below: not taken |
| the glow of a newly lit tile, in `Board::Update` | about 50 updates a tile: the next group of pixels of the tile's 45-group table is lit, and the last six groups are redrawn brighter | with the rest of `Board::Update` outside the melt, 2.2 us an update in the whole quietest game | none that is faster and differs: the picture is a function of the table's position | not lite |
| `Board::DetermineValidMoves` (0x401682) with `ValidMove` (0x40153a) and `CountPieces` (0x4017dd) | once a piece: every cell asks whether the piece fits there, and `ValidMove` counts the pieces on the board each time | 932 calls in that game, 1.4 us a call in the quietest run, 4.8 us in the busiest, 1.3 ms in the quietest game | count once outside the loop | the same table: the count is the same for every cell. Not lite; a performance-pass change, and not earned at 1.4 us a piece |
| `Board::MakeRandomPiece` (0x401324) | a piece of a random shape and colour, made again while it equals the last | 859 calls, 0.5 us a call (1.5 us in the busiest run) | pick among the other shapes and colours | would deal other pieces for a seed, so lite in kind, but at 0.5 to 1.5 us a piece there is nothing to save |
| the widgets' updates, the sidebar's bubbles, the title, the sparkles | a few comparisons an update | `SexyApp::UpdateFrames` less `Board::Update`: 0.6 us an update | none | |

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. All of
`Board::Update` took 5.7 us an update in the quietest run and 11.6 to 25.5 us in the others; at a player's pace of 100 updates a second,
6,000 a minute, that is 34 ms of a minute of play at the least and 153 ms at the most, 0.7% to 3% of the 5 s a minute costs, which is
the notes' "under 3%". The melt is the largest part of it and the only candidate. The bot discards about six times a minute, and a melt is 250 updates, so
a minute at the bot's rate holds 1,500 updates of melt: 22 ms at the quietest cost and 95 ms at the busiest, 0.4% to 1.9% of a minute. Taking the melt out entirely, which no cheaper shuffle comes near, would not move a
minute by more than that, and the five-pair medians resolve a few percent of a minute at best (the port's own five minutes in the notes spread from 4.58 to 5.31 s,
14%, and its runs on a busier machine reached 8.7 s). A change this size could not show in any measurement, so it
does not go in. The single slowest calls in the runs (up to 44 ms in the melt's block, 1.9 ms in `DetermineValidMoves`) are the
machine taking the processor away: the same code's other calls in the same game cost 14 to 63 us.

The melt is also what a player sees: the pixels that slide are a random pick each update, and a lite change keeps the game's look.
The scans that the survey named cost 1 to 5 us a piece, once a piece. The bot plays a piece every 0.13 s at 100 updates a second, 450 a
minute, so even at its rate the scans and `MakeRandomPiece` come to under 3 ms a minute (1.9 us a piece at the quietest cost, 6.4 us
at the busiest), and a player plays far fewer. The faster methods for the scans give the same answers, so they are not lite and belong
to the port's performance pass, where they would not earn their place either; a faster `MakeRandomPiece` would be lite in kind and
saves nothing a measurement could see.

The lite-mod step is therefore done with no mod. Nothing in the port reads `Platform::LiteOn`, and the frozen baseline is unchanged.
