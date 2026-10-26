# Pixelus Deluxe port

Pixelus Deluxe (Nuclide Games and PopCap Games, 2004) is decompiled and ported the way BookWorm Deluxe, Seven Seas Deluxe
and Noah's Ark Deluxe were (see `games/sevenseas/docs/README.md`, the closest model): the original's game classes are
translated function by function onto the portable SexyAppFramework (`framework/`, SDL2 and Emscripten), and the game data is
copied from the user's own install. Nothing from the game itself is in the repository: not its images, sounds, music or
fonts, not its puzzles, and not its texts either (they are read from the player's own `Pixelus.exe` at run time, by address:
`src/GameText.h`).

| Document | Contents |
|---|---|
| [GAME_NOTES.md](GAME_NOTES.md) | Behaviour notes of the game code: the layers, the puzzle and its files, the screen flow, the results, the story, the options |
| [MOD_IDEAS.md](MOD_IDEAS.md) | The mods worth making (none are made: the order of work forbids it until every PopCap game works in the browser) |
| `symbols/20_game.jsonl` | Names for Ghidra (`tools/make_symbols.py` writes it from the address comments in `src/`) |

## The binary

<!-- private: The retail install has one executable, `Pixelus.exe`, which holds the game's image. The port never runs it; it reads the game's texts from it (`src/GameText.h`). -->
The retail install is a PopCap wrapper around the game: `Pixelus.exe` (1,634,722 bytes) is a launcher of 282,624 bytes, then the
18-byte `!popcapdrmprotect!` marker, 400 bytes and the game's PE, unencrypted: 1,351,680 bytes at file
offset 0x451a2, linked 2004-07-29. The port never runs the wrapper; `LoadGameTexts` finds the PE after the marker and reads each
text from it by address, and a private tool (not published) copies the PE out as `PixelusGame.exe` for the analysis.
<!-- /private -->
The game is a Visual C++ 7.1 program (image base 0x400000) of three parts: the game's own classes (0x401000 to 0x43f000), the
SexyAppFramework of early 2004, and the small application layer of Nuclide's own (the "Nc" classes at 0x4e2c10 to 0x4ea000:
the pause count, the quit question, the options and credits dialogs, the loading screen) that sits between the two. The
framework part is the one BookWorm Deluxe and Typer Shark were built on, so `framework/compat2004/` maps it; the Nc layer is
translated in `src/NcApp.*` and `src/GameDialog.*`.

## Reproducing the analysis

Ghidra 12.1.4 and JDK 21 are under `E:\retools`; the commands are the other games' with this game's paths, and long runs go
under `build/native/rundesk.exe`. `re/` and `assets/` are git-ignored: the exe, Ghidra's export and the staged files contain
the game's code, strings and art.

```bash
python games/pixelus/tools/decread.py 40e1a0          # one function, readable, with the game's image and font ids inlined
python games/pixelus/tools/make_gametext.py           # the table of text addresses (src/GameTextTable.inc)
python games/pixelus/tools/make_res.py                # the resource tables (src/Res.*)
python games/pixelus/tools/make_symbols.py            # docs/symbols/20_game.jsonl, from the address comments in src/
```

## Building and running

```bash
python games/pixelus/tools/stage_pixelus_assets.py    # once: copies the game's files to assets/pixelus/
TARGET=pixelus tools/build.sh native                  # build/native/pixelus.exe (and pixelus_gridtest.exe)
tools/build.sh web                                    # build/web/pixelus.html/.js/.wasm (with the other games)
```

Native: `pixelus.exe` finds `assets/pixelus/` next to itself (or up to two levels up), or the retail install; `-gamedir=`
overrides that and `-userdir=` moves the saves. The saves are the original's own kind: `users/directory.txt` lists the players,
`users/<id>` is a player's profile and `users/his<id>` the history of the puzzles they have played, with the moves of the
ones they left unfinished. Web: `pixelus.html`, the shared page
(`web/shell.html`) with this game's settings (`games/pixelus/web/page.js`), imports the player's game folder (or a .zip of
it) into its own IndexedDB-backed folder `/pxpersist`.

Test runs follow `CLAUDE.md`: native runs only under `build/native/rundesk.exe`, web pages only with `?silent=1`.

## Testing

`tools/check.sh pixelus` (about two and a half minutes) runs four things. The grid replay test
(`tests/gridtest.cpp`, `pixelus_gridtest.exe`) loads every level file of the game, plays the best known solution of each into
the grid and checks that it ends with the picture complete: 211 solutions, 211 solved. A scripted tour of the options, the
credits and the how-to-play screens (`tests/menu_tour.txt.in`, driven by `-script=<file>`, lines of `<update> click|char|key|
screenshot|quit ...`) checks that the runs end well and leave their screenshots. The bot plays the first 25 puzzles twice and
the two runs must have the same log and the same screenshots. And the bot plays the whole game: 210 puzzles in the 12 temples
of the island, the stories between them, to the ending, with a hash of its log in the check's expected results.

The bot is `src/testhooks/Autoplay.cpp`, started with `-autoplay=<seed>` `-autoplaypuzzles=N -autoplaycap=<updates>
-autoplaylog=<file> -autoplayshots=<prefix>`. It plays as a player would: it makes a player, presses the play link, clicks
through the stories, the island, the temples and the tablet, and throws the tiles of each puzzle's best known solution with
mouse clicks. Its log says what each screen showed, so the web build under Node can be compared with the native one:
`node games/bejeweled/tools/verify/wasmrun.js game build/web/pixelus.js -gamedir=<abs> -userdir=user -fast -seed=1 -autoplay=1
-autoplaypuzzles=400 -autoplaylog=log.txt` makes the same log as the native exe (the whole game, 40 seconds).

## Status (2026-10-26)

The game plays natively and in the browser from the loading screen through the main menu, the players (new, who are you,
rename, delete), the story, the island with its 12 temples, the temple screens with the puzzles' pictures and medals, the
puzzles (throw and pull of the tiles, aim marks, undo, reset, hint, pause, saved moves) and the tablet that tells the result
(records, medals, time, the temple that has opened), to the ending. The options (the two sliders, full screen, custom cursors,
the hints), the credits and the how-to-play screen work, and the main menu is reachable from a puzzle's options.

Not done or approximate:
- The shimmer that crosses a puzzle's tiles every few thousand updates while it is played is not ported (the one at the
  end of a puzzle is). A tile's own picture-change counter (+0x20 of the tile) is counted down, but the port starts it at 0
  so it never fires; the original's constructor never writes it and the tile comes from operator new, so there it starts as
  leftover heap data and can fire.
- The streak behind a tile that has left the grid (a pulled tile, or one that flew out) vanishes at once in the port; the
  original keeps drawing it (0x42f6c0 draws the trail before it tests whether the tile is gone) and fades it over a second.
- Not a gap but a fix: the original appears never to free a tile that flies out of the grid (a throw into an empty line, move
  F, or the undo of one): it stays in state 2 with +0xb0a8 ("a tile is moving") set for good, so a puzzle finished after
  such a throw would never show its picture or its tablet. Nobody has seen that in a run; the port skips the gone tiles
  and finishes the puzzle (details in GAME_NOTES section 5).
- The island's decorations are simplified, the god figure of a temple has no speech bubbles or moving mouth, the story
  portraits are the heads of the gods without expressions of their own, and the particle effects and blinking eyes of the
  tablet are not ported. The medal's place on the tablet is the port's own.
- The how-to-play screen's demonstration is the port's: the original plays two scripted throws and pulls on real boards, the
  port slides the tiles of the picture in one after the other. The "tips" the original shows between puzzles are reduced to
  the stings of a finished temple or game.
- The tutorial's helper and teacher bubbles are not drawn. The ending goes back to the island; the credits roll is not ported.
- The Options dialog has no Hardware Acceleration box, no registration, no check for updates and no links; the main menu's
  More Games button does nothing (the original shows advertisements fetched from the network): no online or commercial
  features are ported.
- A puzzle's time is not saved with its moves: a restored puzzle restarts its clock.
- The browser build was run to the end of the game under Node and compared with the native run. The page itself was run in
  the Browser pane after the merge of 2026-10-26: a .zip of the game folder set on the page's pick (the file dialogs cannot
  be driven), the import, Play, the title screen and a click through to the new-player dialog of the main menu.

## Changes outside the Pixelus folders

Each is its own `shared:` commit: `CMakeLists.txt` (the `BUILD_PIXELUS` option), `tools/check.sh` (the `pixelus` suite, also in
the `games` set) and `.claude/launch.json` (the `pixelus-web` preview server on port 18776).
