# Mods worth making for Atomica Deluxe

None of these is made (the user's rule: no new game gets mods until every PopCap game works in the browser). They are
ordered by how much they change the game for how little work the port needs.

**Levels as data.** The 41 levels' set-ups are code in the original (`Grid::StartLevel`, 0x40f860): the number of colours,
the chance of chromium and biohazard atoms, the molecules the level needs, how many atoms come each round, the atoms placed
at the start, the arrows of the magnet levels and the cells of the radiation rounds. A level file (a table of these fields
plus a list of start atoms) would let a mod add, reorder or rebalance levels without touching the code, and a level
editor could be a mod too. The level names are texts (a `GT_` override replaces any of them).

**Rules switches.** Small numbers that the code has as constants and that change how the game feels: the size of the
block that makes a molecule (2 x 2, 2 x 3, 3 x 3), whether chromium atoms count as a wildcard in every block, how many
atoms a round adds, how fast the biohazard spreads, how much the combo is worth, whether the field is 9 x 8 or another
size (the pictures fit the cell size of 50 pixels, so only the count of cells would change), the action mode's drain
speed, and the bomb's cost. Each is a number in a mod's settings.

**A calm mode.** No game over: when the field fills, the oldest atoms fade out instead. It suits a relaxed game and
a place to try the other mods.

**Look.** The atoms' pictures (27 types) and their colours, the machine, the monitor, the doors; the original reads them
from `images/` so a mod that replaces the files already works. More colours would also need more atom types and a way to
say which pictures belong to which colour; the number of colours of a level is the limit now (11 plain atoms).

**Voices and texts.** Atomicus' clips are files (`sounds/voices/`), so a mod can give him another voice or another language.
The texts come from the exe at run time and a `GT_<address>` string property replaces any of them: a language pack is a
list of these.

**New molecule shapes.** The molecule search is one function (`Grid::FindMolecules`, 0x412280) with three block shapes. A
mod that adds shapes (an L, a plus, a ring of eight) would score by the number of atoms, and the bonus molecules of the original
(not ported yet) already have the idea of a given shape of given colours.

**Daily seed.** A fixed seed for each day and a local best score for it: the game's random numbers are one generator that the
port can seed (`-seed=N` does it now).

**More elements.** The five elements (fire, water, earth, air, ether) have three levels each. A mod that adds elements
would need the element atoms ported first, and a list of effects (blast a row, shuffle colours, freeze the timer).
