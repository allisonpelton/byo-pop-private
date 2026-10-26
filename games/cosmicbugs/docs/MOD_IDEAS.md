# Cosmic Bugs: mod ideas

None of these is made (the user's rule: no mods before every PopCap game works in the browser). Each would be a mod
of the port's own code or data, never another game's files or modes.

## Fixes

**The easy setting's label.** The difficulty item formats its value with the string at 0x45930c, which has a
literal `5s` where the others have `%s`, so the easy setting reads "5s" instead of its name (the normal and hard ones
are right). The port keeps the original's string; a fix would format it like the other two (CBMenu.cpp, the item
kind 0x10).

## Play

**Practice a board.** Pick any board the player has reached and play it alone, with no lives to lose. The game
already keeps two records per board (the fewest shots and the best fill, each with a name: `gLevelRecords`) and has
a level selection screen; the mod would start one board from it without the run around it, and show the records.

**Endless random boards.** A `random_levels` tag of the game file makes a number of random boards (twelve times as
many when registered). A run of only random boards, each a little faster and with one more bug, would end only with
the lives, its score in a table of its own.

**Gentler bugs.** A setting below the easy difficulty: slower bugs, the ship's beams faster at the start
(`gBeamSpeed`), and power-downs rarer. For younger players, beside the game's own three settings.

**Show the fill.** While a line is being drawn, tint the part of the board that would fill if it closed now, so a
player can see which side the bugs are on before committing.

## Accessibility

**Power-ups by shape as well as colour.** The game tells good power-ups from bad ones by blue and red; an outline or
a small mark on the bad ones would let colour-blind players tell them apart.

**Keyboard play.** Arrow keys to move the ship, a key to fire and one to switch the aim, for players who cannot use
a mouse well. The game already reads the keys held each frame (Key_Down).

**Bigger text in the menus.** The menus and the Hall of Fame use small bitmap fonts; a scaled font for the menu
items and the requesters would help on large screens and for low vision.
