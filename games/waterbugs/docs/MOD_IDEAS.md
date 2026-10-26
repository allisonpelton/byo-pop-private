# Water Bugs: mod ideas

None of these is made (the user's rule: no mods before every PopCap game works in the browser). Each would be a mod
of the port's own code or data, never another game's files or modes.

## Fixes

**A hint names 85%.** The third hint of the first level says "85%" in its own words, while a board's quota comes
from its file (`fillQuota`; the level start text already uses it). A mod would build the hint from the quota of the
board being played, so a board with another quota says the right number.

## Play

**Practice a board.** Pick any board the player has reached and play it alone, with no lives to lose. The game keeps
two records per board (the fewest shots and the best fill, each with a name: `gLevelRecords`) and has a level
selection screen; the mod would start one board from it without the run around it, and show the records on the
results requester as it does now.

**Endless random boards.** A `random_levels` tag of the game file makes a number of random boards from a seed
(`Level_StartRandom`, with `Level_EnemyCounts` giving each its bugs). A run of only random boards, each a little
faster and with one more bug, would end only with the lives, its score in a table of its own.

**Gentler bugs.** A setting below the easy difficulty: slower bugs, the ship's beams faster at the start, and
power-downs rarer. For younger players, beside the game's own settings.

**Show the fill.** While a beam pair is on its way, tint the part of the board that would fill if it closed now, so a
player can see which side the bugs are on before committing.

**Hints on every board.** The first three boards explain the game with hints put up as fills end
(`Level_Tutorial`); a setting could bring the same short hints back for any board, for a player coming back to the game.

## Accessibility

**Power-ups by shape as well as colour.** The game tells good power-ups from bad ones by blue and red; an outline or
a small mark on the bad ones would let colour-blind players tell them apart.

**Keyboard play.** Arrow keys to move the ship, a key to fire and one to switch the aim, for players who cannot use a
mouse well. The game already reads the keys held each frame (Key_Down).

**Calmer water.** The bubbles sway as they rise; a switch to hold them still would help players who find motion
tiring.

**Bigger text in the menus.** The menus, the hints and the Hall of Fame use small bitmap fonts; a scaled font for the
menu items, the hints and the requesters would help on large screens and for low vision.
