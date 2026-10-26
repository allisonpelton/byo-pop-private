# Mods worth making for Alchemy Deluxe

None of these is made (the user's rule: no new game gets mods until every PopCap game works in the browser). They are
ordered by how much they change the game for how little work the port needs.

**Rules switches.** Numbers the code has as constants and that change how the game feels: the size of the board (9 x 8
cells of 52 pixels; the pictures fit the cell size, so only the count of cells would change), how many symbols and colours
are in play at each skill level (`Board::SetDifficulty`, 0x401220, with the four constants of `data/diff.txt`), the
chance of bombs and skulls per mille, how fast the forge fills and how many pieces may be forged, and the points of a
row, a column and a cleared board. Each is a number in a mod's settings; `data/diff.txt` already shows the idea, and a
mod can replace it today because the game reads it from the game folder.

**More symbols and colours.** The 22 symbols come from one strip (`images/symbols.gif`, 50 x 50 pixels each) and the seven
colours from a table in the code (12 bytes a colour, 0x4de390). A mod that reads the colour table from a file and the
number of symbols from the strip's width could add symbols without changing the code, and the matching rule (the
neighbour shares a colour or a shape) would work for any number.

**A calm mode.** No game over: a piece that has nowhere to go melts away without filling the forge. It suits a relaxed game
and a place to try the other mods.

**A different matching rule.** The rule of the game is one function (`Board::CanPlace`, 0x40153a): the cell must touch a
piece, and every piece it touches must share the new piece's colour or its shape (a skull matches anything). A mod could
ask for "any one neighbour matches" or "both colour and shape" and score by how many neighbours matched; the tutorial's
texts would need new lines for it.

**Board layouts as data.** The board starts with one piece in the middle (cell 4, 3) and the cells light up as pieces
land on them. A mod with a start layout (blocked cells, pieces already placed, cells that count double) would give puzzle
boards without a new picture; the lit-cell logic (`mLit`) already tells when a board is cleared.

**Look.** The backdrops (`level2` to `level9`, each a board's background), the tiles, the pieces, the wizard and his mouth
shapes, the forge, the fonts: the game reads them from `images/` and `data/`, so a mod that replaces the files already works.
The look of the pieces is the most obvious one: an alphabet or a set of animals instead of the alchemical symbols.

**Voices and texts.** The wizard's lines are strings in `properties/default.xml` and the sounds are files in `sounds/`,
so a mod can give him another voice or another language. The 228 strings are all in that one file.

**Daily seed.** A fixed seed for each day and a local best score for it: the board's random numbers are one generator that
the port can seed (`-seed=N` does it now), and the score table already has a "best of today" line.

**Time trial variants.** The time trial's forge fills by itself at a rate the code computes from the skill level
(`mProgRate`, `mProgDecay`); a mod could make it slower for children, or make the forge fill only when the player is
idle.
