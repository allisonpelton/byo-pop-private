# TipTop Deluxe port

A port of TipTop Deluxe (ASAP Games, published by PopCap Games; the build of 2003-02-01) to the shared platform layer:
it plays natively (Windows, SDL2) and in the browser (Emscripten). The game code is a translation of `TipTop.exe`,
found by decompiling it with Ghidra; the game's files (`TipTop.dat`, which holds the pictures, sounds, music and
configuration, and `TipTop.exe`, whose texts are read at run time) come from the player's own copy and are never in the
repository.

TipTop is not a SexyAppFramework game. It has its own engine, LOWOS ("LOWOS Screen" is its window class), a C-style
layer over DirectDraw, DirectInput and DirectSound, and a small GUI of layers and handler procedures on top of it. The
port keeps that structure: `engine/` is LOWOS and the resource manager on `platform/`, `src/` is the game, and the
framework only hosts it (`src/TipTopApp.cpp`: the window, the clock, the input and the 640x480 screen the game draws
into).

All three modes (Regular, Strategy and Speed) play from the first level to the game over, with every screen of the
original: the splash, the menus, the options, the instructions, the credits, the high scores, the tips, the level
summaries, pause, the save and continue of a game in progress, and the high score name. The music is missing: it is a
General MIDI file that the original sent to Windows' MIDI mapper, and the port has no MIDI synth yet (see Status).

## The binary

`TipTop.exe` is linked by MSVC 7.0 (Visual Studio .NET 2002) and carries no copy protection wrapper; the only RTTI is
the C++ runtime's (its exception classes).
The game code is 0x401000-0x428437 (the board and its field 0x401000-0x410d35, the application 0x4197c6-0x426fc7, the
GUI, the configuration reader and the MIDI sequencer around them), LOWOS is 0x42d000-0x438000 and the resource manager
0x438390-0x439d70; the rest is the C runtime. `GAME_NOTES.md` has the layouts and the rules as the code has them, and
`symbols/` the names the port gave the engine's functions.

`TipTop.dat` is an archive whose every byte has its bits permuted; `tools/tiptopdat.py` lists and extracts it (into the
git-ignored `re/tiptop/dat/`). It holds PCX and JPEG images, Ogg Vorbis sounds, the MIDI music and the configuration,
a text file of named values (positions, timings, the levels, the effects) that `src/Config.cpp` reads as the original
did. The exe's table of resources (0x477060) is copied into `engine/ResTable.inc` by `tools/restable.py`.

## Reproducing the analysis

The export lives in `re/tiptop/` (git-ignored, like everything derived from the game). It was made with Ghidra's
headless analyzer on `re/tiptop/bin/TipTop.exe` with two scripts of `tools/ghidra/` beside the shared ones:
`TipTopEhProlog.java`, a pre-script that marks `_EH_prolog` (0x44d1b0) so the C++ functions that call it get their
own frames, and `TipTopSignatures.java`, a post-script that applies the port's names and prototypes (written by
`python games/tiptop/tools/ghsigs.py > re/tiptop/ghscripts/sigs.txt` from the `// 0x...` comments of the port's
declarations), so that the decompilation passes every argument at its calls.

```
python games/tiptop/tools/decread.py 4092a0          # a function of the export, readable: names, game strings inlined
python games/tiptop/tools/cleandec.py <in> > <out>   # frame-pointer locals of the _EH_prolog functions named
python games/tiptop/tools/make_gametext.py           # src/GameTextTable.inc: addresses, lengths and hashes only
```

The decompiler drops x87 operands and some stack arguments: floating point code (the bounces, the fades, the particle
motion) was read in the disassembly (`llvm-objdump -d --no-show-raw-insn --start-address=... --stop-address=...`).

## Building and running

```
tools/setup_worktree.sh                                             # a new worktree: the git-ignored files
python games/tiptop/tools/stage_tiptop_assets.py <the game's folder> # stages assets/tiptop/ (TipTop.dat, TipTop.exe)
TARGET=tiptop tools/build.sh native                                 # build/native/tiptop.exe
build/native/rundesk.exe build/native/tiptop.exe -userdir=<dir>     # a game, invisible and silent as tests run it
```

`tools/build.sh web` builds every game for the browser, `tiptop.html` among them; to build only TipTop, configure
`build/web` with `emcmake cmake` as `build.sh` does and run `cmake --build build/web --target tiptop`. Serve `build/web`
(`.claude/launch.json` has `tiptop-web` on port 18774) and open `tiptop.html`; in the Browser pane always with
`?silent=1`. Once the player has agreed to the game's terms, the page imports `TipTop.dat` and `TipTop.exe` from their
install folder or a zip of it into its own IndexedDB folder (`/ttpersist`) and keeps the saves there.

Native options: `-gamedir=<dir>` (else `assets/tiptop` next to the exe or up to two levels up, or the retail install),
`-userdir=<dir>` (the preferences and the saved game; the original kept them in its own folder), `-fast`, `-seed=N`
(the C library generator's seed, otherwise the clock) and `-script=<file>` (the framework's scripted input:
`<update> click <x> <y>`, `key <name>`, `char <c>`, `screenshot <file>`, `quit`).

## Testing

`tools/check.sh tiptop` (about two and a half minutes) plays two regular games, a strategy game and a speed game with the bot
(`src/testhooks/Autoplay.cpp`, started by `-autoplay=<seed> -autoplaymode=regular|strategy|speed`), compares the last
line of each log with the expected one in `tools/check.sh`, and plays the regular games again, which must give the same
log and the same screenshots. The bot drops each block where it joins the biggest group of its kind, otherwise into one
of the emptiest columns; it clicks only in the board and on the menus' play buttons and presses Enter for everything
else, so it never touches a link button. It reaches level 7 in a regular game and level 11 in a speed game, and ends a
strategy game in its first level (it does not plan).

What the bot does not reach was checked with scripts: the options, instructions, credits and high score screens, the
end of a game through the save dialog, and the saved game's panel and continue on the new game screen (natively, and
in the browser across a page reload).

An AddressSanitizer build (2026-10-25): `cmake -S . -B build/native-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug
-DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-i686.cmake -DSDL2_DIR=... "-DCMAKE_CXX_FLAGS=-fsanitize=address
-fno-omit-frame-pointer -O1" "-DCMAKE_C_FLAGS=<the same>"`, then `cmake --build build/native-asan --target tiptop` and
copy `libclang_rt.asan_dynamic-i386.dll` (llvm-mingw's `i686-w64-mingw32/bin`) next to the exe. The bot's two regular
games and a speed game (the same results as the normal build) and the scripts that save a game and continue it ran
with no report.

## Mods

None (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones worth
making.

## Status (2026-10-25)

Everything plays but the music. The sequencer (`src/Music.cpp`, 0x4276f0-0x428031) is translated and runs once a frame
instead of on a 20 ms thread; its MIDI messages go to an output the host sets, and there is none yet. The registration
and the trial (the count of plays, the "no more plays" dialog, the registration dialog and its code check) are left
out: the port plays as registered, so no dialog shows a register button. The links of the options screen open through
the framework (never in a test run).

## Where the work was hard

The blitter (0x437e3c) chooses one of 32 routines by the pixel format, a mode (plain, an alpha byte in front of every
pixel, a color key, additive) and translucency; the alpha-byte images (the particles among them) were drawn
wrong until the port read the alpha byte as the original does and stepped the source by five bytes a pixel. One drawing call of
the progress bar passes the width as the pitch; the port keeps that, as its comment says.

The GUI's dispatch is easy to get subtly wrong: with modal layers it draws the handlers in groups, from the layers
outside every modal one up to the top modal layer, six passes within each group, and input goes only to the top modal
layer. Freeing a handler takes effect at once.

The original opened its files by bare names in the working directory; the port routes them through the user dir
(`src/UserFile.cpp`), which also tells the web page when to save.

## Changes outside the TipTop folders

`CMakeLists.txt` (the `BUILD_TIPTOP` option), `tools/check.sh` (the `tiptop` suite) and `.claude/launch.json` (the
`tiptop-web` preview on port 18774).
