# Alchemy Deluxe port

A port of PopCap's Alchemy Deluxe (version 1.6, linked 2004-08-23) to the shared SexyAppFramework port: it plays
natively (Windows, SDL2) and in the browser (Emscripten). The game code is a translation of the original's game classes,
found by decompiling the game inside `Alchemy.exe` with Ghidra; the game's files (pictures, sounds, the two modules and
the texts in `properties/default.xml`) come from the player's own copy and are never in the repository.

What the original's player sees is ported: the title screen and its loading bar, the selector (skill, strategic or time
trial, the tutorial), the board with its runes, bombs, skulls and the forge, the wizard's messages, the tutorial's steps,
the cleared-board statistics, the game over and the score table with its name entry, the all-time table, the options
(volumes, full screen, custom cursors), the help, the quit and new-game questions, and the saved game. Not ported (the
port is a registered copy with no online features): the registration, trial and update-check dialogs, the nags, the
internet scores and every link to a web page.

## The binary

`Alchemy.exe` is a 2007 wrapper (a "drm" folder with Lua scripts, a trial and buy screen) around the 2004 game, a plain
MSVC 7.0 build.
<!-- private: The analysis works on a copy of the game's image that a private tool takes out of the wrapper (`re/alchemy/bin/AlchemyGame.exe`, image base 0x400000). -->
The image is 1,314,816 bytes at 0x1891a2 inside the wrapper; `tools/extract_game_exe.py` carves it to
`re/alchemy/bin/AlchemyGame.exe` (image base 0x400000).
<!-- /private -->
The scripts belong to the wrapper, so the port needs no PopLua.
The game's classes have no RTTI; their vtables were found from the constructors' stores. The framework in the game is the
2004 SexyAppFramework (BookWorm's generation: 47 widget slots, 53 dialog slots), so the port uses `sexycompat2004`
(`Dialog2004`, `Slider2004`, the 2004 word wrap, the C library's `rand`). `GAME_NOTES.md` has the class map and the rules
as the code has them; `WORKING_NOTES.md` has how the analysis went.

## Reproducing the analysis

<!-- private: The game's image is first taken out of the wrapper by a private tool; the commands below work on that copy. -->
```
python tools/extract_game_exe.py ...                  # carve the game image out of the wrapper
```
<!-- /private -->
```
tools/ghidra_import.sh ...                            # then funcmatch against BookWorm (WORKING_NOTES.md)
python games/alchemy/tools/make_game_region.py        # the game's functions only (0x401000-0x415000) into re/alchemy/game.c
python games/alchemy/tools/fn.py 40d836               # one function of the export
python games/alchemy/tools/norm.py re/alchemy/fields_Board.txt 4068f1   # the same with this-relative fields named
bash re/alchemy/dis.sh 0x40cf2a 0x40cf60              # disassembly, where the decompiler drops stack arguments
python games/alchemy/tools/ann.py 0x4086e6 0x408938   # a function's calls with the operands pushed before each, named
python games/alchemy/tools/cmpfn.py 40e773 games/alchemy/src/Piece.cpp Piece::Draw   # constants the port lacks or adds
python games/alchemy/tools/va.py 4e36d8:d             # a double (d), float (f), int (i) or string (s) at an address
```

The export goes to `re/alchemy/` (git-ignored, like everything derived from the game). The decompiler drops the trailing
arguments of draw calls (the selector's base picture was first drawn at (0, 0); the disassembly has (10, 0x23)), so every
coordinate was checked in the disassembly.

## Building and running

```
tools/setup_worktree.sh                                    # a new worktree: copies the git-ignored files
python games/alchemy/tools/stage_alchemy_assets.py <the game's folder>   # stages assets/alchemy/
TARGET=alchemy tools/build.sh native                       # build/native/alchemy.exe
cmake --build build/web --target alchemy                   # build/web/alchemy.html (after tools/build.sh web has configured it)
build/native/rundesk.exe build/native/alchemy.exe -userdir=<dir>   # a game, invisible and silent as the tests run it
```

`games/alchemy/tools/run_script.sh <script> <user dir> [seconds]` runs the game on a `-script` file (`<update> click x y`,
`<update> screenshot <png>`, `<update> quit`) under rundesk. In the browser pane open the web build with `?silent=1`.

## Testing

`tools/check.sh alchemy` plays a strategic game, a time trial game and the tutorial with the bot
(`src/testhooks/Autoplay.cpp`, started by `-autoplay=<seed> -autoplaymode=strategic|timetrial|tutorial`), compares the last
line of each with the expected one in `tools/check.sh`, runs the strategic game again (same log, same screenshots), and quits
a game half way so that the next run offers the saved game and loads it. The bot puts each rune where a row or column
fills, otherwise next to the most runes, with a tenth of random cells; it forges when no cell fits, uses bombs, dismisses
the wizard's boxes, types a name and clicks through the tutorial. The game's clock is the framework's `GameTickCount`
(the virtual clock under `-fast`), so a rerun is identical down to the statistics' game time.

## Mods

None yet (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones worth making.

## Status (2026-10-26)

Everything above under "ported" is in, each function with its `// 0x<addr>` comment. Checked: the native bot runs (a game
of about 12,000 updates takes two seconds with `-fast`), the screens were looked at in screenshots (title, selector,
board, tutorial boxes, wizard messages, cleared-board panel, score table, name entry, options, all-time table, help, quit
question), and the web build was played in the browser pane (import, title, selector, a game, the wizard's messages).

Not done or not checked:

- no AddressSanitizer run yet (no crash has been seen);
- the web build's saved game (the `GetShutdownSaves` backup) and the options' full screen switch were not exercised;
- the sounds and the music were never heard (every run is silent), only that they load;
- `cmake/ByopopStores.cmake` has no `BYOPOP_BUY_alchemy` entry: the web build stops at configuration without it (a shared
  file, the coordinator's);
- the original was never run next to the port: the drawing was compared with the disassembly instead (the next section), so
  what is left unchecked is only what a comparison of code cannot show (timing on a real machine, the sound).

## Fidelity check (2026-10-26)

Every drawing function was compared with the original's disassembly, call by call (the arguments the decompiler drops, the
constants read from the exe, the wizard's lip-sync table byte for byte): the board, the cleared-board statistics, the effects
overlay (score popups, sparkles), the wizard's head, the selector, the title screen, the side panel, the score table and
its picture, the all-time table, the options dialog and the piece. The help screen and the name entry were compared by
their constants only (every number the original uses is in the port, with no extra ones). Its one finding was the forge's back
picture, which the port drew at (0, 0) under the opaque backdrop instead of (0x28, 0x147) (the decompiler had dropped the
position), so the tank showed black. Two effects of the original are never triggered, since nothing sets their flags
(`GAME_NOTES.md` section 5), and are not ported. `tools/cmpfn.py`, `immcmp.py`, `ann.py` and `va.py` are the helpers.

## Changes outside the Alchemy folders

- `CMakeLists.txt`: the `BUILD_ALCHEMY` option (a `shared:` commit).
- `tools/check.sh`: the `alchemy` suite (a `shared:` commit).
- `.claude/launch.json`: `alchemy-web` on port 18777.
