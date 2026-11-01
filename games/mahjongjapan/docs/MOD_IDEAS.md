# Mods worth making for Mahjong Escape: Ancient Japan

None of these is made. The user's rule is no new mods unless someone requests one, and these become the first entries
of the game's feedback page. They are ordered by how much they change the game for how little work the port needs.

**Resource overrides.** Most of the others need this one. A mod would be a folder of files named by resource type and
id (`LAYOUTS/385.lay`, `TXT/383.txt`, an image by its id), and the host's `FindResource` (`src/MahjongApp.cpp`) would
take a file from the mod when it has one. Replacing a layout, a background, a tile set or the music is then a matter of
files alone.

**More layouts.** A layout is a short binary stream of cells ("LM", version 1, then x, y, z and an optional kind per
tile; `Layout::Load`). A converter from a text form, and a list page that grows past the 162 layouts, would let players
write and share their own. The classic list already pages nine at a time.

**Wide screen in a window.** The original offers wide screen only in full screen (the option is hidden in a window).
The browser and a window could offer it too.

**A free-play escape.** The 60 stages with their special tiles (the key and its lock, the wild tiles) can be played
only in order. A mod could open them all from the map, or deal a classic layout with the escape's specials and its
Magic Gold Tiles, so that a game ends when the gold pair is matched. The best times would be kept apart from the
original's.

**Deals that can be won.** A classic deal fills the layout at random from the clock (`Deal::Start`), and a board can
run out of moves before it is empty. A deal made backwards (pair by pair into cells that would be free) always has a
solution. This would be an option, since its best times are not comparable with the original's.

**Undo without the score back.** The undo takes back the points of the pair it returns. A relaxed rule could keep them,
for players who want to explore a board.

**Ancient China's sets in Japan.** Mahjong Escape: Ancient China runs on the same engine with other tile sets and
backgrounds. With resource overrides, a mod could bring its sets into this game's options for a player who owns both
games.
