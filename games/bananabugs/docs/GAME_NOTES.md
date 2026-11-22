# Banana Bugs (PopCap, 2010)

The port is a translation of the original's game classes from our own Ghidra export, one class at a time, on the
shared framework, its 2004 compatibility layer and the OpenJPEG decoder (the backdrops are JPEG 2000). Every class
the game's code reaches is translated; nothing stands in for an untranslated part any more. How the work was done, the
register conventions of the export, the class list and the notes made on the way are in `WORKING_NOTES.md`.

## Building, staging and running

The game's files are staged from the user's own copy with `games/bananabugs/tools/stage_bananabugs_assets.py` into the
git-ignored `assets/bananabugs/` (main.pak unpacked, names in lower case; the DRM launcher, the codec DLLs and the
installer's leftovers are not copied). `bananabugs.exe` finds the folder by itself from `build/native`; the web build
imports the install folder or a .zip of it through the page (`web/page.js`). Saves go to the user folder
(`-userdir=` natively, `/bananabugspersist/userdata` on the web).

## Test commands and the bot

`src/TestCommands.cpp` answers the standard commands (`docs/TEST_COMMANDS.md`): `state`, `move`, `seed`, `goto`,
`level`, `win`, `lose`, `player` and `set`, each described in the file's header and in `WORKING_NOTES.md` ("Test
commands and the bot"). The bot (`src/Bot.cpp`) shoots the rope along the stretch between two walls that cuts off the
most ground with no bug in it, rejecting a line a bug is near (a margin that grows with the line's length, and wider
for the kinds that fly); it turns the rope with the right button when the best line lies across the way the game aims,
places a held bonus where bugs are, and clicks through the menu, the name window, the map, the tally and the hints. It
reads the game and draws from no generator of the game's, so asking for a move changes nothing. A click is seen only
when the button is down during an update (the game reads a held flag), so its moves press, wait and release.

The game seeds its own generator at each level's start (LevelObj::InitCells, from the level's number), so a `seed`
given before a level is overwritten; the slots seed both generators from the clock, as the original does.

## Deviations

Each is marked "port" in the code and has no visible effect in normal play.

The port initialises fields the original leaves as the allocator gave them until it writes them itself (the bugs'
brains before Setup and PlaceAt, the Player before Reset, the pipes before the level's links, the score board's spare
icon slots, the slots' payout and delays, MetaGame's offers, BugImg's freeze byte, an unused word of BugObj). It
checks for null or out-of-range where the original would read or write wild memory but never does in normal play: a
bug with no body yet, a place with no pipe or the 0x7fff of a bug that has none, a heading outside 0 to 3, the level
or the rope when there is none, a list index of -1, a division by a count of zero (cells, wall places, open ground,
bonus kinds, a picture's size), and the level picture's mask read past its edge. It frees what the original leaks
(LevelObj's two pictures, the spin stars' fourth key, the walk-through's last page, a track freed with free() after
Clear). The scripted test run shows no error box, and the test hooks run only when a test command was given.

## Changes still to check against the original

These change what the original does in a case that may or may not happen; each needs a look at the original under
origrun before it is kept as a port fix (a crash, a hang or a soft lock) or moved to the fixes mod.

The bolt's update with no steps writes one place before its piece list in the original, which clears the last piece's
pointer so that Stop crashes later; the port skips that write. The fly's grid read for the 41st column takes the
memory beside the table in the original; the port takes the place as not blocked. PathTrace::Load (0x454ab0) searches
for ever on a path picture with a pixel that has no neighbour, and has no bound on its points; the port stops at both
(neither should happen with the shipped pictures). The dragonfly's body shares Notify 0x4237d0 with the skeeter's and
the bee's, which starts the track numbered by the word at body +0x2a4, an Anim pointer on the dragonfly's body; the
port's BugImgDragonfly::Notify does nothing, where the original's StartTrack may simply ignore so high a number. The
parrot reads the link of a null next pipe at a wall's end; the port ends the bonus there. The Zuma frog restarts its
end fade every frame its cell stays cut, so the bonus never ends while that lasts; the port ends it once.

## Fix candidates (for the fixes mod)

These are the original's own and stay in the port. UserSelect's Enter while renaming checks the name against every
profile, the player's own among them, so an unchanged name is "already used", and it takes a one-letter name, which OK
turns down. The Options window skips BBWindow's update, so its OK and Cancel never swell. A fire ball that lands once
the level has reached its target, or with no bugs alive, is never put away and stays where it landed. The score board
blanks every leading 0 of the score, so a score of 0 shows an empty field (to check that the original shows the same).
The beetle's Zap never passes a pea's hit on, so a beetle killed by a pea makes no bunch of bananas. A bee freed from a
freeze goes right or down again along each side it was going left or up (a negative zero read as forward). The moth's
constructor gives it the fly's kind number. FlyPaper's Reset releases the bugs but leaves the zap area set; the
earthquake's Reset does not put the shaken backdrop and board back; the pea shooters' End leaves the current bonus set.

## Oddities

None chased yet. Last-bit differences are possible in the level's morph table, which the original sums on the x87 and
the port in doubles, and in BananaBonus's float constants (12.5664f, 6.37045f, 1.5708f), which may be the decompile's
rounding of the exe's values; both are for the logic pass.

## Status

The first playable merge: every screen runs, natively and on the web, and the bot plays by real input through the
menu, the name window, the introduction, the hints, the levels, the tally and the map.
