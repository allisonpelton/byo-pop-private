# Bookworm Adventures Volume 2: mod ideas

The port is first the original game, unmodified and working like its exe; it adds nothing for later mods, not even
hooks (the user's rule of 2026-10-26). Mods wait for the game's mod phase, which comes only after every PopCap game
works in the browser, and each one needs the user's approval. In that phase the session goes through the 29 standard
mods of `docs/MOD_ROADMAP.md` ("Standard mods for every game") and makes each that fits; this file maps them onto the
game and gives the reason for each that does not fit, and then lists the ideas that are particular to this game. A mod
is a folder with a `mod.json` (`docs/MOD_MENU.md`: the shared VFS and ModManager; the web shell's Mods panel takes a
folder or a .zip), with settings read from its `"features"`.

What makes this game different from the earlier ports is that most of its rules are Lua scripts: 462 compiled
scripts under `scripts/`, run by the port's own PopLua VM, which also compiles source. So a large class of mods
needs no C++ at all. A mod can ship `.lua` files that the VFS lays over the game's `.luc` files (the loader
prefers a script's source to its compiled form when the source is newer, as the original did for its developers), or a
small script that is run after `main.lua` and patches tables and functions in place. The second kind is the better
default, since it touches only what it changes and survives the game's own files.

## The 29 standard mods, mapped

| # | Standard mod | For this game |
|---|---|---|
| 1 | LiveSplit | Fits: splits at each chapter and boss of the adventure, from the shared tagged memory block, with the RNG seeded per chapter (needs 4 and 5). |
| 2 | Widescreen | Fits, as in Volume 1's port (BookWorm's): the battle screen is the only one with room to use, for example a word log beside the board. |
| 3 | High-res textures | Fits: `@2x` images made locally from the player's files; PopAnim's transforms need the scale too. |
| 4 | Replays | Fits: seed plus the per-update input, through the same update loop; the battle scripts' own random numbers have to come from the seeded generator only. |
| 5 | Seeded RNG | Fits: the seed shown after a battle and settable from the options, the command line or the URL. |
| 6 | Bot scoring | Fits: a bot that plays chapters headlessly with the solver below, to tune curated mods; needs the VM and the battle engine to run without a window (the milestone 3 test setup). |
| 7 | Replay checker | Fits once 4 is: a saved adventure or arena replay replayed headless to confirm its result. |
| 8 | Timer and input display | Fits: a timer for the whole adventure and per chapter, for speedruns in the browser. |
| 9 | Daily board | Fits: the arena with one seed for everyone on a date and its own high scores (the Hall of Fame has an arena tab). |
| 10 | Undo | Fits: the board and the battle state before each word, restored on undo; the saved game already writes most of it. |
| 11 | Hints or a solver | Fits: shows the strongest word on the board by the game's own damage formula, and the longest, as BookWorm Deluxe's solver does. |
| 12 | Level select | Fits: start at any chapter or boss, with separate scores; the book and chapter screens already have the data. |
| 13 | Game stats | Fits: a word journal and totals (words, damage, longest word, treasures), as BookWorm's. |
| 14 | Speed control | Fits: a factor on the update clock and on PopAnim's time step; battles are turn-based, so nothing depends on real time. |
| 15 | Colour-blind help | Fits: shapes on gem and special tiles as well as colours. |
| 16 | Reduced motion | Fits: no screen shake, flashes or heavy particles in the attack and defence animations. |
| 17 | Other controls | Fits: typing a word to select its tiles (as BookWorm Deluxe's mod), keyboard-only play, touch for phone browsers. |
| 18 | Larger text and high contrast | Fits: the fonts are bitmap fonts, so larger text needs a second image set or a vector font; the conversation panel and the word list are the parts that matter. |
| 19 | Rule settings | Fits: attack damage, enemy health, tile odds, gem tile frequency, the number of potions as settings, read by script mods from the tables the scripts already have. |
| 20 | Relaxed variant | Fits: no hearts lost (or no game over), for players who want the story. |
| 21 | Custom levels | Fits: custom enemies and chapters, since creatures, chapters and books are data-driven script tables; a documented format and the user's own art. |
| 22 | Randomizer | Fits: a shuffled enemy order, random chapter modifiers or generated chapters. |
| 23 | Developer leftovers | Fits: the exe holds a BETA_BUILD flag, cheat words and debug keys, as Volume 1; to be listed from the decompilation. |
| 24 | Translations | Fits: the game's texts are read from the player's exe by id; a table replaces them by id (translations made locally, never committed). |
| 25 | Local achievements | Does not fit as such: the game has no Steam achievements; its badges (the Hall of Fame and the badge manager) are its own and already saved. |
| 26 | Two players | Does not fit: battles are one word at a time against a scripted enemy with its own turn logic; a second player has no natural place. A hot-seat arena score race is the closest, and it is a curated mod, not this one. |
| 27 | Pixel-perfect scaling and scanlines | Fits: an 800x600 game, so integer scaling is limited to a few sizes; the scanline filter is optional. |
| 28 | Smooth frames on high-refresh screens | Fits: frames between updates, with the PopAnim animations interpolated, without changing the update clock so replays stay exact. |
| 29 | Jukebox | Fits: ten OGG files (a map, a level and a boss track per book, plus the dance battle); the player chooses them or the other books' tracks play elsewhere. |

## Ideas particular to this game

| Idea | What it does | How |
|---|---|---|
| Script mods | Load a mod's Lua files after the game's scripts, with a documented list of the hooks worth patching (attack damage, gem effects, enemy tables, treasures, tile odds) | VFS overlay plus a `mods/<name>/init.lua` run after `main.LoadRequiredScripts`; the compiler makes source mods possible. The base of 19, 21, 22 and the ideas below |
| Damage preview | While a word is being spelled, shows the damage it would do and the effects its gem tiles would add | A Lua hook on tile selection that calls the same damage function the attack uses |
| Bigger dictionary | Adds Collins Scrabble Words (from the user's own copy) to the game's word list | The word list is loaded by C++ (`data/compressed.txt`); merge a second list as BookWorm Deluxe's mod does |
| Faster battles | Speeds up the attack and defence animations, and skips the conversations the player has seen | A speed factor on PopAnim updates and a seen-flag per conversation in the profile |
| Battle log | A scrolling log of every word, its damage and the effects that fired, kept per chapter | Lua hooks on the battle engine's attack and defence; drawn by a small widget |
| Endless arena | Arena battles one after the other with rising enemy levels, for a high score | A script mod over the arena and boss lists |
| Treasure planner | Before a chapter, compares the treasures the player owns against the chapter's enemies | Reads the treasure and creature tables the scripts already have |
| Definitions | Shows a definition of each word played, from the user's own dictionary | As BookWorm Deluxe's definitions mod |
| Part of speech | A bonus for the right kind of word, or a word list by kind | `data/compressed_attribs.dat` gives each word of Volume 2's dictionary a noun, verb and adjective flag, which Volume 1 lacks |
| Hard mode | Stronger enemies, fewer gem tiles, no potions between chapters | Script mod over the difficulty tables (a curated mod on top of 19) |

The solver (11), the damage preview and typing a word (17) are the ones that would most change how the game feels; the
script mod support comes first because most of the rest would be built on it.
