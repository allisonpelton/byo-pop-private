# Seven Seas Deluxe: the identical-play pass

This is the report of the pass that compares the port with the original exe, screen by screen. It is in progress: the sections
at the end say what is still to be compared.

## What was compared and how

The original (WinSS.exe, version 1.13) was run only through `tools/origrun` in the Windows Sandbox, under rundesk, from a copy.
A script clicked through the same scenario in the original and in the port, and both took screenshots at the same game update.
The original's main loop counts the updates since the last draw, which origrun reads so that a shot lands on the update it asks
for, and each click is held until the original is between updates, so the port's script puts the click on the update that read
it. `tools/shotdiff` then compared the pictures; every box it flagged was looked at and judged here.

The scenarios were the static screens (title, menu, quit and options dialogs, help screen, the first board with arrows under the
mouse), a six-move sailing game with kills, a sinking and the new ship, and games started from the original's own saved game:
Continue, the level-complete screen, the next level, and the bonus level (Treasure Island) with its nag and a dig, and the map after it (the dig's end is random, so the map was compared by its own age: the port's map shots matched the original's with 0 differing pixels, 12 shots from the first dot to the last). Saves were
made by editing the original's savegame.dat, which the original accepts.

## Changes the pass made

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| Hovered dialog button labels changed colour | the original keeps its labels the dialog's gold under the mouse; the help buttons are gold and turn black only when hovered | the same | cffedc0, 9631054 |
| Options dialog labels | the original's four extra buttons are gold | gold | 41820b3 |
| Screenshots offset | the original's window has a title bar | shotdiff crops 26 pixels (comparison setting) | 9631054 |
| After Continue some squares took no clicks and showed no arrows (the user's "controls fail at random times") | case 0xf of 0x421a10 hides the pirate's talk widget, a 200 x 200 box that otherwise takes the clicks | Continue hides it the same way | 05bd59d |
| Bonus screen: the first line of every message ("Try to find the", "The Treasure is", "You Win!") was at the bottom | the original draws the first line at the top of the board (y 31) and only the second at height - 0x1e | the same | 2dd815a |
| Treasure Island nag: button label white | the dialog's gold | gold | 2dd815a |
| The map (after level 10) never moved on: no dotted route, no second X | the map's Update counts the widget's updates and lights one more dot every 50 (0x417960) | the same | d7d0f2a |
| The map's "Click to continue" glowed | the original's button has no component image and shows its unlit picture, the lit one under the mouse | the same | d7d0f2a |
| At game over the pirate vanished from the high-score list | 0x420430 brings the talking pirate in front of the list (it does not stop him) | the same | GOCOMMIT |

## Oddities skipped

**The new ship's cell after a sinking.** The original places it with random draws, and the number of draws before the placement
varies from run to run: the same script and seed gave cell (2,2) in one run and (1,9) in another. The port's cell is one of the
original's. It can be seen by running the six-move scenario twice and looking at the shots after the sinking.

**Gun smoke on the ships of a continued game.** The original's loader does not set the nine gun-smoke timers of a ship, so a
blast or two shows in the diagonal cells a second after Continue (about 60 updates, and again at about 250), depending on what
the heap held. The port starts them at none. See it by continuing a saved game and taking shots for the first four seconds.

**The game-time on the high-score screen.** The original shows the seconds since its stored start tick, which came from the machine's tick counter in the run that wrote the save, so a loaded game shows 0:12 in the original and 2:41 in the port when the same save is loaded in a different session. It depends on the machine's uptime; see it by loading a save and losing at once.

**The pirate's mouth.** In a few shots the mouth differs by a few pixels while he talks; the steps follow the sample being played, which the silent runs do not time like the original.

**The treasure's cell on the bonus level.** Where the chest and graves lie follows from the random draws, whose count varies as
above, so the direction text after a dig differs between runs of the original as well. The layout and the logic of the dig are the
same; only the random placement was not comparable.

## Deviations that remain

The help buttons' pressed look (the original gives a button a copy of an all-black image, the port draws the flat black plate; the
result is the same), the Register button (taken out) and Is3D (the 2003 exe has no 3D mode, so there is nothing to compare; the
two renderers of the port differ by at most 3 of 255). See GAME_NOTES, section 5.

## Still to compare

The bonus level's win and lose screens, and the high-score list after a loss was compared; the name entry and what follows are not and high-score screens.
