# Seven Seas Deluxe: the identical-play pass

This is the report of the pass that compares the port with the original exe (WinSS.exe, version 1.13), screen by screen. It
says what was compared and how, what the pass changed, what it left alone and why, and what still differs.

## What was compared and how

The original ran only through tools/origrun in the Windows Sandbox, from a copy, under rundesk. A script clicked through the
same scenario in the original and in the port, and both took screenshots on the same game update. The original's main loop
counts the updates since its last draw, which origrun reads, so a shot lands on the update it asks for, and each click is
held until the original is between updates; the port's script puts the click on the update that read it. tools/shotdiff
compared the pictures, and every box it flagged was looked at and judged here. Scenarios that need a saved game used the
original's own savegame.dat, edited: the original accepts the edited file.

Screens: the title, menu, quit, new-game, continue and options dialogs, the help screen, the first board with arrows under
the mouse; a six-move sailing game with kills, a sinking and the new ship; Continue from a saved game, the level-complete
screen and the next level; the bonus level (Treasure Island) with its nag, the digging states and the map that follows it;
and, from a one-life save with an enemy broadside lined up, the loss, the high-score list with the pirate, and typing a name
into the record dialog. Static screens match pixel for pixel; board states match update for update, with only faint
differences (the corner whirls differ by at most 8 of 255).

Focus: the original and the port lost and regained the focus on a board (origrun's focus lose and focus regain, the same
commands in the port's script) and five shots were compared. The original's picture changes when the focus is lost, and the
port's matches it (three shots exact, two only faintly different).

Web build: the same scripted game (the one-life loss, 744 shots) was run on the web build under Node (wasmrun.js) and its
shots compared with the native run's. 741 shots are identical to within 8 of 255 everywhere except the game-time digits; the
other three differ in fewer than 100 pixels beside those digits. The web build is therefore the native game drawn by SDL's
software renderer.

Sound: not compared by state, because the original's sounds cannot be seen. They go through DirectSound and the music through
FMOD, and the sandbox's sound stand-in recorded no plays for Seven Seas (its log holds only the attach line, with and without
the wav option). What was done instead is a read of the decompilation: the port's PlaySample and SetMusicOrder calls were
taken from call sites in the export, with their sample ids and pans. Nothing the pass changed touches sound. The original can
be run with sound on, as the user allowed, to check by ear.

## Changes the pass made

Each row: the difference, its cause in the original, what the port does now, and the commit.


| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| Hovered dialog button labels changed colour | the original keeps the dialog's gold under the mouse; the help buttons are gold and turn black only when hovered | the same | cffedc0, 9631054 |
| Options dialog labels | the four extra buttons are gold | gold | 41820b3 |
| After Continue some squares took no clicks and showed no arrows (the user's "controls fail at random times") | case 0xf of 0x421a10 hides the pirate's talk widget, a 200 x 200 box that otherwise takes the clicks | Continue hides it the same way | 05bd59d |
| Bonus screen: the first line of every message was at the bottom | the first line is drawn at the top of the board (y 31), only the second at height - 0x1e | the same | 2dd815a |
| Treasure Island nag: button label white | the dialog's gold | gold | 2dd815a |
| The map after level 10 never moved on: no route, no second X | the map's Update lights one more dot every 50 updates (0x417960) | the same | d7d0f2a |
| The map's "Click to continue" glowed | the button has no component image and shows its unlit picture, the lit one under the mouse | the same | d7d0f2a |
| Arrows showed only above and below the ship (the user's "movement arrows don't show up properly") | 0x409840 draws the arrow of each of the eight neighbouring cells; the port drew only the vertical ones | all eight, as the original (nine hovers, shots match) | 87cbbda |
| At game over the pirate vanished from the high-score list | 0x420430 brings the talking pirate in front of the list and does not stop him | the same | 1d82ddd |
| After "No" in the Continue dialog (Average, Hard) the squares under the pirate's box took no clicks and showed no arrows (the user's "column 4, rows 1-3") | 0x421a10 case 3000 + 0xf never puts the pirate's invisible 200 x 200 box away; the original has the same fault | the pirate is put away as for Continue, a fix of the original's bug made at the user's request | dcc842e6 |
| The last tutorial hint ("That's it! ...") never showed | its text is the 15th of each hint table; the port's table held 14 | all 15 | dcc842e6 |
| The first box's text sat 17 pixels too high | 0x406400 draws its four lines at 0x12, 0x23, 0x34, 0x45 below the row | the same | dcc842e6 |


The comparison itself changed one setting: the original's window has a title bar, so shotdiff crops 26 pixels (9631054).

## Oddities skipped

The new ship's cell after a sinking. The original places it with random draws, and the number of draws before the placement
varies from run to run: the same script and seed gave cell (2,2) in one run and (1,9) in another, and the port's cell is the
second. See it by running the six-move scenario twice and looking at the shots after the sinking.



Gun smoke on the ships of a continued game. The original's loader does not set the nine gun-smoke timers of a ship, so a
blast or two shows in the diagonal cells about a second after Continue (about 60 updates, and again at about 250), depending
on what the heap held. The port starts them at none. See it by continuing a saved game and taking shots for the first four
seconds.



The treasure's cell on the bonus level. The chest and the graves come from the random draws, whose count varies as above, so
the direction text after a dig differs between runs of the original as well, and the win and lose screens of the bonus level
(which need the right dig) could not be lined up with the port's. The layout of their text is the same as the dig states that
were compared.



The game time on the high-score screen. The original shows the seconds since a start tick stored in the save, taken from the
machine's tick counter in the run that wrote it, so the same save shows 0:12 in the original and 2:41 in the port in
different sessions. It depends on the machine's uptime. See it by loading a save and losing at once.



The pirate's mouth. In a few shots it differs by a few pixels while he talks; its steps follow the sample being played, which
the silent runs do not time like the original.

## Deviations that remain

The help buttons' pressed look (the original gives each a copy of an all-black image, the port draws the flat black plate;
the result is the same), the Register button and the "Submit to Internet High Score List" button (taken out: no registration,
no online features) and Is3D (the 2003 exe has no 3D mode, so there is nothing to compare; the port's two renderers differ by
at most 3 of 255). See GAME_NOTES, section 5.

## Not compared

The list of high scores after the name is accepted (the click on OK in the run did not leave the dialog in either game, so
only the typing was compared, and it matches), the demo and the tutorial's later steps, and the bonus level's win and lose
screens, for the reason given under the oddities.
