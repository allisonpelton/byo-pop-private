# Water Bugs: the identical-play pass (2026-11-01)

This is the report of the pass that compared the port with the original exe running, screen by screen. The pass found
and fixed two real faults of the port (the random-number function and the way the menus redraw), made the test runner
match the original's conditions, and left four small differences it did not chase. The detail of each finding is also
in `DEVIATIONS.md`, section "Compared with the original running".

## What was compared, and how

The original ran only through `tools/origrun` (`docs/ORIGRUN.md`), inside Windows Sandbox, from a copy of the install
folder, so nothing showed on the user's desktop and nothing played on its speakers. Screens were not captured from the
window: origrun reads the game's own surface from its memory at an exact update of the game (`--surface-screen` with
`--surface-eip` ranges), and the update counter at 0x460a1c says which update each shot holds. The original was started
with a fresh profile each time (its saved profile makes it open the starting-level screen instead of the crash site, so
`cb.dat` was removed before each run).

Mouse input went in through origrun's input gate (`--input-eip 41f467,41f511`): the sandbox has no real cursor, so a
script's `at N move/down/up` commands are held until the game's own input read and the log says at which update each one
was read. The port's test runner (`build/native/waterbugs_run.exe`) replayed every click at exactly that update. The
original under origrun runs about one millisecond per drawn frame, so the runner was set to the same (`WB_FRAME_MS=1`);
the port's default of 10 ms is what `tools/check.sh` froze. Shots were then compared with a replay at the original's own
counter, pixel by pixel.

What was compared: the title screen; the world map; the start of a level (its text, the beam, the fill to 84%, a long
idle with clicks); a fresh-profile run of 100000 updates over six levels, all six lives lost, the game over and the
hall of fame; the main menu with its sign; both Help pages (Instructions and Power-Ups); the Hall of Fame while it fades
in and when it is whole; Change User with its name-entry dialog; and Options. Quit leaves the game at once in the
original with no requester; the port's Quit path was not run through the same script. Sound was compared by calls (29
sound-play calls and the music's volume, load, free and play calls, equal in number); the original's sounds could not be
recorded, because BASS does not reach the sandbox's audio stand-in. Focus loss and regain were run in the original
(`focus lose`, `focus regain`): its update counter keeps running while it is lost and only the music pauses, and the port
does the same. The web build was rebuilt, loaded in the Browser pane with `?silent=1` and fed a zip of the install folder
through the page's zip input from script; it started the game and drew the main menu and Options with the same texts. A
click-through of more web screens was not finished (the page's import panel stayed in front), so no web screen was
compared pixel by pixel; the web build runs the same game code as the native build that was compared.

An Opus reviewer then read 13 functions against the Ghidra export and the disassembly (the shooter's firing, firing
checks, update, control, extents, measure and push, the level's random layout, enemy counts and cell redraw, Rand, the
menu redraw and its background draw) and found no behaviour difference; it found two false comments, which are fixed.

## What the pass changed

| Difference | Cause in the original | What the port does now | Commit |
|---|---|---|---|
| The random seed diverged from the original's once the seed had its top bit set, which changed the bubbles on boards and the beam's fill | `Rand` (0x420770) divides the seed with an unsigned `div`; the port divided it signed | `Rand` divides unsigned; both games now agree draw by draw through the whole 100000-update run | 12d8077 |
| A translucent fringe round the main menu's sign, 1568 pixels different in the sign's region | `Screen_Redraw` (0x40d230) draws the menus from the current menu to the end and then sets the current menu back to the first; the port drew every menu's images again from the first on each call, so the sign's translucent edge was drawn twice | `Screen_Redraw` walks the menus as the original does; the menu and Help screens are identical to the pixel | de7c035 |
| The ship and cursor jumped in the runner and the shots of a click were off | The game warps the cursor (`SetMousePos`) and the sandbox's cursor cannot be moved, so a replay saw different cursor positions | The runner ignores the game's cursor warps when `WB_NOWARP=1` is set; a frame shortens to reach an event instead of overshooting it, and `state` prints the frame, the draws, the seed and the ship | 580441a, f9c8f82, 44f9a92 |
| World-map water and other per-frame effects stepped at another rate | The original runs about 1 ms per drawn frame, so effects tied to a drawn frame step once per update | `WB_FRAME_MS` sets the runner's frame length; with 1 ms, 14 of 17 shots became identical and later 31 of 33 | 08443bd |
| The frozen regression suite hashed pixels that depend on the draw list's address tie-break | Two draw-list entries at the same depth are ordered by their addresses in the original | `tools/check.sh waterbugs` masks those pixels (loss-screen bubbles, the beam) and was re-frozen; this is a shared commit | bb5ced7 |
| Two comments said something the original does not | The Opus review: `gVisited` is at 0x6e5e60 (not 0x6e5de0), and `Shooter_Fire` buzzes only with an enemy on the ship and does nothing once the filled share is past the quota | The comments were corrected; the code was already right | 1cf4931 |
| Colour gradients one step too bright, and the sum of enemy x positions near the ship rounded where the original truncates | `Palette_Gradient` (0x42c0f0) and the enemy-position sum in the ship's measure (0x41c2da) convert with `fistp` after `or 0xc00` on the control word, i.e. the x87 rounding set to chop | Both truncate (`(int)` of the value) instead of `nearbyint`; the suite's screens stay as frozen and the menu, Help, Change User and Options shots stay at 0 differing pixels | this commit (after a4b7fe2) |

The origrun input gate and the `at down/up` commands (`tools/origrun`, `docs/ORIGRUN.md`) were made for this pass and
are shared tools; they were merged by the coordinator (29ed8cb).

## Oddities not chased

These were logged under "Oddities (not chased)" in `DEVIATIONS.md` (5ea36b5), as the rule of 2026-11-01 asks.

The pipe-end glints are small white and lilac sparkles near pipe ends and power-ups, on about 20 of the later-level
shots of the 100000-update run: their tint and position differ from the original's by a few dozen pixels per shot, while
the frames around them are identical. They are not made by the particle systems (no `ParticleSystem_Add` call falls in
the window) and the cause was not found after a long look; matching them would need values measured from runs. To see
them, replay the original's run with its own input counters and compare with `tools/shotdiff`.

A dark strip at x 766, y 531 to 560 of the logo panel differs in three shots because the original draws it over a bubble
where the port draws the bubble over it (the draw list's address tie-break, as with the loss screen's bubbles). One fill
cell is one update apart in one run. Three pixels of at most 9 differ in the sign's bubble on one Change User shot. None
has a visible effect.

The original's sound could not be recorded, so the sounds are compared by call counts only; to see the gap, run the
original under origrun and compare `sound.log` with the port's calls.

## Deviations that remain

Beyond those, `DEVIATIONS.md` lists the port's remaining deviations from the exe (host and system calls that do
nothing, bounds and memory the port keeps safe, the leaks the original has and the port does not, words and pointers).
Two harness conditions matter for reading the screens: the runner writes a windowed `cp.cfg` when none exists, while the
original's default is full screen, so Options matches only with `fullscreen="1"`; and the runner's default frame of
10 ms is what the frozen baseline holds, so `tools/check.sh waterbugs` and the replays differ in that setting on
purpose. A played comparison of the web build, with its screens against the original, is not done.
