# Mods worth making for Word Harmony

None of these is made (the user's rule: no new game gets mods until every PopCap game works in the browser). They are
ordered by how much they change the game for how little work the port needs.

**Archive overrides.** Most of the others need it: a mod is a folder of files named as in `WHArchive.saf` (such as
`config\HUD.xml`, a level file or `res\ball.png`), and the engine's resource manager (`engine/ResourceManager.cpp`,
which already reads the archive through the host) takes a file from the mod when it has one. Replacing a level, a
screen's layout, a ball's picture or a sound is then a matter of files alone.

**The level editor.** The exe carries Sprout's own level editor, never reachable in this build: the ball system's
editor flag (+0x1f8) is never set and `EditorHUD.xml` is loaded but never shown. Its parts are still there. In the
editor a click picks up any ball or obstacle and drags it on a 44-pixel grid (the grid can be turned off), the keys
give the held ball its letter (A-Z), make it part of a chain, a wildcard, a stone or a breakable block, give it a x2
to x4 badge or delete it, and the HUD adds letter balls, blocks, rocks, boxes and ramps in four turns, enters the
level's words, and saves or cancels the changes (a backup of the level is kept until then). The port has the branches
for it inside the live functions; a mod would port the editor's own functions that were left out (0x413af0 and
0x413c40 add a box or a ramp, 0x416920 writes the balls back as bubbles, 0x416ba0 leaves the editor, 0x417680 the
HUD's actions, the word entry 0x40dcd0-0x40df90 and the level file's writer 0x41c450), open the editor from a menu
entry of the mod, and write the level files into the mod's folder. Players could then make RELAX puzzles and THINK
deals of their own and share them as mods. An option, since the editor's HUD is Sprout's and unpolished.

**Puzzle packs.** With the editor and archive overrides, a pack is a set of level files with their word lists. The
level choice of RELAX shows ten groups of ten; a pack would bring its own count of levels and its own high score
table, so that its scores and the original's are never mixed.

**A calmer TSUNAMI and a gentler THINK.** The water's rise, the spawn delay and the hint's cost are numbers in the
level files and the controller; options for a slower rise, more hints, or a THINK where a dead end may be dealt again
without losing the level suit players who want the words without the pressure. Off by default, and their scores kept
apart.

**Readable balls.** Larger letters, a high-contrast face for the letter balls, and chain and selection colours that a
colour-blind player can tell apart (the joined balls turn blue against the pink ones); and keyboard play, where typing
a letter picks the nearest free ball with it and the arrow keys move it, for players who cannot drag a ball with the
mouse.

**A daily THINK deal.** The deal of each day seeded by the date, so that a player can retry the same letters and
compare scores with friends by hand; its scores kept apart. Strictly local: nothing goes online.
