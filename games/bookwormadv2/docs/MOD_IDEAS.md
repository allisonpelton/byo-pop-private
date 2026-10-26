# Bookworm Adventures Volume 2: mod ideas

This is the brainstorm the coordinator asked for: the mods worth making for this game, none of them made yet.
By the project's order of work no game gets new mods until every PopCap game works in the browser, so each
of these waits for the user's approval. They would build on what the other ports already have: the shared VFS
and ModManager (`docs/MOD_MENU.md`: a mod is a folder with a `mod.json`; the web shell's Mods panel takes a
folder or a .zip), HD `@2x` images, and settings read from a mod's `"features"`.

What makes this game different from the earlier ports is that most of its rules are Lua scripts: 462 compiled
scripts under `scripts/`, run by the port's own PopLua VM, which also compiles source. So a large class of mods
needs no C++ at all. A mod can ship `.lua` files that the VFS lays over the game's `.luc` files (the loader
already prefers a script's source to its compiled form when the source is newer, as the original did for its
developers), or a small script that is run after `main.lua` and patches tables and functions in place. The
second kind is the better default, since it touches only what it changes and survives the game's own files.

| Idea | What it does | How |
|---|---|---|
| Script mods | Load a mod's Lua files after the game's scripts, with a documented list of the hooks worth patching (attack damage, gem effects, enemy tables, treasures, tile odds) | VFS overlay plus a `mods/<name>/init.lua` run after `main.LoadRequiredScripts`; the compiler makes source mods possible |
| Word solver | Shows the strongest word on the board by the game's own damage formula (letter values, gem tiles, treasures, the enemy's weaknesses), and the longest | C++ search over the grid as BookWorm Deluxe's solver does, scored by calling the battle scripts' damage function |
| Damage preview | While a word is being spelled, shows the damage it would do and the effects its gem tiles would add | A Lua hook on tile selection that calls the same damage function the attack uses |
| Bigger dictionary | Adds Collins Scrabble Words (from the user's own copy) to the game's word list | The word list is loaded by C++ (`data/compressed.txt`); merge a second list as BookWorm Deluxe's mod does |
| HD art | Uses `@2x` images where a mod has them | The shared image loader already looks for them; PopAnim's transforms need the scale too |
| Faster battles | Speeds up the attack and defence animations, and skips the conversations the player has seen | A speed factor on PopAnim updates and a seen-flag per conversation in the profile |
| Battle log | A scrolling log of every word, its damage and the effects that fired, kept per chapter | Lua hooks on the battle engine's attack and defence; drawn by a small widget |
| Endless arena | Arena battles one after the other with rising enemy levels, for a high score | A script mod over the arena and boss lists |
| Custom enemies | New creatures with their own word lists, attacks and portraits, made from the user's own art | Script mod plus images; the creature scripts are data-driven tables |
| Treasure planner | Before a chapter, compares the treasures the player owns against the chapter's enemies | Reads the treasure and creature tables the scripts already have |
| Colour-blind gems | Marks gem and special tiles with shapes as well as colours | An extra image layer per tile type in the tile drawing |
| Type to play | Type a word on the keyboard and the board selects the tiles for it | As BookWorm Deluxe's type-word mod: the solver's search finds a path for the typed letters |
| Definitions | Shows a definition of each word played, from the user's own dictionary | As BookWorm Deluxe's definitions mod |
| Replays and LiveSplit | Records a game's inputs for replay, and drives LiveSplit for speedruns of the adventure | The shared replay and LiveSplit support of the other ports, with the RNG seeded per chapter |
| Hard mode | Stronger enemies, fewer gem tiles, no potions between chapters | Script mod over the difficulty tables |

Volume 2 adds one thing Volume 1 lacks: `data/compressed_attribs.dat` gives each word of its dictionary a noun, verb
and adjective flag, which makes a part-of-speech mod possible (a word list or a bonus for the right kind of word).

The solver, the damage preview and type-to-play are the ones that would most change how the game feels; the
script mod support comes first because most of the rest would be built on it.
