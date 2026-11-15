# TipTop Deluxe: the lite mod (2026-11-14)

TipTop has no lite mod: nothing in its game logic is worth a toggle, and the one shortcut that `docs/MOD_ROADMAP.md` names
for it, the skipped paint, no longer pays. This file says what was looked at, what each candidate would gain and where it
could differ, and why nothing is taken (`docs/MOD_ROADMAP.md`, "The lite mod"). The performance numbers are in
`GAME_NOTES.md`, "Performance, second round" and "The surface display".

## What was looked at

TipTop's play is a board of blocks that fall into columns and join, and a menu system on a small GUI of layers and handler
procedures. It has no solver and no hint search: the bot behind the `move` test command is test code,
not part of a player's run. The pass the game runs (62.5 a second in real time) is its update and its painting in one
function, `Board::Draw` (0x40e766), and the cycle split of the port (`perf.py portscript --split`, the scripted strategy
minute at 640x480) puts the whole pass at 1.15 M cycles of the process's 1.64 M an update (71%), the picture's copy and
present at 0.34 M (21%) and the loop at 0.12 M (7%). A flat sample of the same minute (`tools/perf/cpuprof2.py`, 160
samples that found the thread running) puts 69% of the pass in `memcpy` under `lowos::BlitPixels`, the plain copies of
the display's pictures into the screen, 7.5% in `BlitPixels` itself, and under 4% in everything the game's rules do
(`Board::Draw`, `Field::Move`, the clock, the GUI's dispatch): the board's logic costs about a thousandth of a core. What the
pass spends is drawing, which a lite change may not touch, since it keeps the look.

| Computation | What it does | A faster method | Result |
|---|---|---|---|
| The board's update inside `Board::Draw` (0x40e766) and its field (0x401000-0x410d35), the 35 ms tick | block positions, joins, the counts and timers | none needed | under 4% of the pass |
| The GUI's dispatch (`Dispatch`, the handlers' procedures) | six passes of the layers' handlers a frame | none needed | inside the same few percent |
| The pass's painting: `lowos::Blit`, `FillRectangle`, `BlitScaled` | the whole display is painted every pass: the backdrop, the board, the glows, the particles | skip a pass's paint | below |
| The bot's choice of a move (`src/testhooks/Bot.cpp`) | the biggest group of its kind, else an emptiest column | test code, not in a player's run | |

## The skipped paint

`MOD_ROADMAP.md` names it as the example of a lite shortcut: a pass whose picture is not going to be shown does not
need painting, "wrong in 13 of 50,054 bot passes". It was a candidate while the port ran 100 passes a second against a
60 Hz screen, when 4 passes in 10 were never shown. The port now runs 62.5 passes a second (`GAME_NOTES.md`, "62.5 passes a
second in real time"), so at most about 1 pass in 25 goes unshown (62.5 against 60), and painting is 70% of a pass:
skipping every one of those would save under 3% of the process, which the measurement does not resolve (the same exe's
pairs differ by 20 to 40%). That settles it.

A second form of it was tried, because the scripted strategy minute shows 39% of its passes leaving the picture as it
was (the board waits for the player between clicks): skip a pass's painting when the list of its screen writes (every
`Blit`, `FillRectangle` and `BlitScaled` call with its arguments and its source's address) is the list of the pass
before. A scratch build hashed each pass's writes and compared the screen after the pass with the screen after the one
before (never committed), in the bot's games with `-fast` (`-autoplay=7 -seed=2`, one game each):

| Game | Passes | Same writes, same screen (skippable) | Same writes, screen changed (the skip is wrong) | Other passes |
|---|---|---|---|---|
| Strategy | 7,230 | 503 (7.0%) | 0 | 6,727 |
| Regular | 15,991 | 899 (5.6%) | 1 | 15,091 |
| Speed | 67,789 | 2,308 (3.4%) | 4 | 65,477 |

**It can differ.** The five wrong skips are in the regular game at pass 10,481 (bot seed 7, game seed 2; 12,123 pixels in
the box 443,61 to 478,407) and in the speed game at passes 10,688, 11,157, 11,167 and 11,177 (6,894 to 11,733 pixels in
335,65 to 370,407 and 300,60 to 333,407): in each, one column of the board, 34 pixels wide, changes between two passes whose
screen writes are the same calls with the same arguments. The game draws a column's glow from a buffer that it fills itself
(`Board::DrawGlow`, through `BlitPixels`), so the blit's source address is the same and its contents are not.
A pass skipped there would leave the column's glow a pass behind, and the next pass would show it.

**What it would gain.** The repeat passes are 3.4 to 7.0% of a bot's passes, so painting none of them would save 2 to 5%
of the pass (70% of it is painting) and 1.4 to 3.5% of the process: below what the pairs resolve, as above. The gain would be
larger only while the screen stands still, a paused game, the options, the instruction pages, the credits, or a strategy
board waiting for a player (39% of the scripted minute left the picture as it was): there the port costs 1.27 M cycles a
pass (the pass 1.15 and the loop 0.12; no frame is presented), 3.9% of a core at 62.5 a second, against the original's
6.4% (its `Frame` 1.1 M and its `EndFrame` blit 1.1 M a pass, at 60 a second), so it is already 40% under its original, and
skipping every repeat pass's painting would bring it to about 1.4% of a core. The skip would also have to hold a pass's
writes back until they are known to differ, and the game reads the screen back in the middle of a pass for its glows, so it
is an engine change that the size of the saving does not earn.

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. The
logic is under 4% of a pass and has no search to replace; the paint skips gain 2 to 5% of a pass in play, and what they
gain on a still screen is a part of a few percent of a core the port is already well under its original's on. The cost that is
left is the drawing, which a lite change may not touch.

The lite-mod step is therefore done with no mod.
