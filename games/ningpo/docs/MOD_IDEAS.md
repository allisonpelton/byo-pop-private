# Mods worth making for Ning Po Mahjong Deluxe

None of these is made (the user's rule: no new game gets mods until every PopCap game works in the browser). They are
ordered by how much they change the game for how little work the port needs.

**Archive overrides.** Most of the others need it: a mod is a folder of files named as in `ningpo.pyz` (bare names
such as `classic.nml`, `tileset_1.tga`, `theme_np.ogg`), and the file system (`engine/HFileSystem.cpp`, which already
reads the archive through the host) takes a file from the mod when it has one. Replacing a layout, a tile set, a
table or the music is then a matter of files alone.

**Deals that can be cleared.** The original puts the tiles into the cells at random and never checks that the board
can be cleared, so many games end in the no-moves dialog through no fault of the player. A mod that deals backwards
(it fills the layout pair by pair, each pair into two cells that would be free at that point) gives a board that
always has a solution, and the shuffle could keep that promise in the same way. An option, since the best times of
the two kinds of game are not comparable.

**More layouts.** A layout is a text file of 144 lines, "x,y,z" in half tiles, so new ones are easy to write and to
share. The game's table of 18 layouts (0x467110), their preview pictures and the best times' keys
(`HS:<layout>x<rank>`) would grow to take a list from the mod; the title's arrows already cycle through the table.
Layouts of other sizes (72 tiles for a short game) need the board's fixed count of 144 to become the layout's.

**A relaxed mode and a timed one.** The penalties (30 seconds for a hint, 60 for an undo, 120 for a shuffle) are
constants of the game; a relaxed mode without them, with the clock hidden, suits players who want the puzzle only. The
opposite is a countdown: the board must be cleared before the time runs out, and each match gives a few seconds back.

**A daily deal.** The deal of each day seeded by the date, so that a player can retry the same board and compare
times with friends by hand; its best times kept apart. Strictly local: nothing goes online.

**Tile sets for readability.** A tile set is one sheet of 13 columns of 39 by 55 pixels. Large-print faces, colours
that a colour-blind player can tell apart, or plain Western numerals and letters in a corner of each face make the
game playable for more people; with archive overrides they are pictures alone.

**Planning help.** A panel that counts the tiles of each kind still on the board, and a softer hint that shows only
whether a selected tile has a free match, help a player plan without giving the move away.

**Statistics.** Games played, won and ended per layout, the longest run of wins and the average time, kept in the
registry beside the best times and shown in the best times dialog.

**A sharper screen.** The original draws into a 15-bit back buffer and dithers every image down from 24 bits; a back
buffer of 32 bits (`engine/HDisplay.cpp` and the blend routines) would show the art as it was painted, without the
banding of the gradients.

**Texts.** The texts come from the exe at run time and a `GT_<address>` string property replaces any of them; most of
what the player reads (the buttons, the rules) is in pictures of the archive, which archive overrides replace. A
language pack is a list of both.
