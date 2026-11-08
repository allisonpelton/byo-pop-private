# Alchemy Deluxe port

A port of PopCap's Alchemy Deluxe (version 1.6, linked 2004-08-23) to the shared SexyAppFramework port: it plays
natively (Windows, SDL2) and in the browser (Emscripten). The game code is a translation of the original's game classes,
found by decompiling the game inside `Alchemy.exe` with Ghidra; the game's files (pictures, sounds, the two modules and
the texts in `properties/default.xml`, and the ten default names of the score tables, which `GameText.cpp` reads from the
player's `Alchemy.exe`) come from the player's own copy and are never in the repository.

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

The standard test commands (`state`, `goto`, `level`, `set`, `win`, `lose`, `seed`, `move`; `GAME_NOTES.md`, section 6) are in
`src/testhooks/Commands.cpp`; `move` is the same bot. The real-input drivers are `tools/realplay/drive.py` (native, posted window
messages under rundesk), `tools/realplay/live.py` (native, the live channel with `-realinput`: real SDL events, so focus loss, minimize
and Alt+Enter too) and `tools/web/onegame.js` (the player-only page, DOM events); `IDENTICAL_PLAY.md`, "Real play" and "On the trunk's
live channel", says what they played.

## Mods

The game's fixes mod, `mods/fixes` (`GAME_NOTES.md`, section 7): two toggles, on by default on the site, for the bugs of the original that
the port keeps (a missing tile rim on the first column and row, and the end of a continued game drawn off the board). The port
without the mod is the original. No other mods until the user asks for one (`MOD_IDEAS.md` lists the ones worth making).

## Status (2026-10-26)

Everything above under "ported" is in, each function with its `// 0x<addr>` comment. Checked: the native bot runs (a game
of about 12,000 updates takes two seconds with `-fast`), the screens were looked at in screenshots (title, selector,
board, tutorial boxes, wizard messages, cleared-board panel, score table, name entry, options, all-time table, help, quit
question), and the web build was played in the browser pane (import, title, selector, a game, the wizard's messages).

Checked after that, with the bot and a click script under rundesk and in the browser pane:

- **AddressSanitizer.** An i686 build with `-fsanitize=address` (`cmake -S . -B build/asan -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-i686.cmake -DSDL2_DIR=<SDL2> -DBUILD_ALCHEMY=ON "-DCMAKE_C_FLAGS=-fsanitize=address
  -fno-omit-frame-pointer -g -O1" "-DCMAKE_CXX_FLAGS=<the same>" -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address`; the toolchain's
  `libclang_rt.asan_dynamic-i386.dll` goes next to the exe, and `ASAN_OPTIONS=detect_leaks=0`, since leak checking does not
  exist on Windows) ran the five games of `tools/check.sh alchemy` (strategic, time trial, tutorial, a game quit half way and
  the saved game's load) with the same results as the normal build, and a click script through the title, the selector, the
  options (the full screen box, Help) with no report.
- **The saved game in the browser.** A game in progress leaves a backup in `localStorage` (`sexy-userdata-backup:/alpersist/userdata`,
  with `savegame.dat` inside) within a second of the first rune; reloading the page, which runs no shutdown, brings the
  "Continue?" question and "Yes" restores the board, score and piece positions; the backup is still there after a second
  reload and is gone within two seconds of the player's answer "No". The backup holds the file while the game runs because
  `AlchemyApp::GetShutdownSaves` returns `Board::SerializeGame`, which touches no clock.
- **Full screen.** Natively the options' box switches the window (the box starts checked, since the game's `partner.xml`
  says `DefaultWindowed` false) and the setting is written (`ScreenMode`) and shown again the next time the dialog opens. In the
  browser the game always starts in the page; the options' box asks SDL for full screen, and the Browser pane has no document
  full screen (the page's own Fullscreen button does nothing there either), so there it leaves the canvas at the screen's size
  until the page is reloaded; in a browser that allows full screen the framework's `SDL_WINDOWEVENT_SIZE_CHANGED` handler
  is meant to set the box right again when the player leaves it (Esc), which was not run. The page's Fullscreen button is
  the usual way on the web.
- **Sound.** All 15 samples and the two songs load with no error (a failed load would open the framework's popup). With
  `AlchemyApp::PlaySample` logging for one run, every sample was played at the right place: thunk (a
  placement), melt (a discard), skull, forge down, final vanish (twice in the table), end of game, final gong, warning, gem
  change, click, bad, welcome (the selector, a second in), level complete and game over. The bot clicks "next board" at once, before the
  wizard has faded in, so the level complete sound is not in its runs, as in the original (0x407130 plays it only
  when the wizard is fully in); with the click delayed by 150 updates it plays once per cleared board.

Not done or not checked:

- the sounds and the music were never heard (every run is silent), only that they load and play;
- the full screen switch in a browser that allows it (the pane does not);
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
