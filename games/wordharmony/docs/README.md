# Word Harmony port

A port of Word Harmony (Sprout Games, published by PopCap Games; © 2006 Sprout Games) to the shared platform layer:
it plays natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of the game's exe, found
by decompiling it with Ghidra; the game's files (`WHArchive.saf`, which holds the pictures, fonts, screens, levels and
word lists, the `res` folder with the sounds, the music and the splash, and `WordHarmony.exe`, whose texts are read
at run time) come from the player's own copy and are never in the repository.

Word Harmony is not a SexyAppFramework game. It runs on Sprout Games' own engine, the C++ namespace `engine`, which
Sprout also used for Pizza Frenzy and Iggle Pop; `ENGINE.md` describes it. The port keeps the original's three
layers. `engine/` is the engine translated, free of game code so that it can become shared code when a second Sprout
game is ported, and mapped onto `platform/` through a host interface (`engine::Host` in `engine/Host.h`: the clock,
the game's files, image decoding, the registry, the screen mode, links, messages, the log and the sound system).
`game/` is the game (the exe's 0x401000-0x424000): the controller, the screens, the settings and saved games, the
levels, the physics, the ball system with the rules of the three modes, and the HUD. `src/` is the host: the
SexyAppFramework only gives it a window, a clock, the input and the 800x600 screen (`src/WordHarmonyApp.cpp`), and
the host draws the engine's images into that screen with blitters of its own (`src/Render.cpp`) and plays its sounds
through the platform's mixer (`src/SoundHost.cpp`).

Everything a player sees is there: the splash, the main menu with its falling blossoms, the three modes (TSUNAMI, the
action game with the rising water and the bonus word; RELAX, the hundred puzzle levels with their word lists; THINK,
the dealt letters that must all be used) with their level choices, help screens and level ends, pause, hints,
recycling a word, Save/Quit and Continue Game, the high scores of each mode with the name entry, the options, the
layered music, and the screen saver that the exe also is (`/s`, `/c`, `/p` on the native command line).

## The binary

<!-- private: -->
`WordHarmony.exe` is PopCap's DRM wrapper with the game exe inside it, after the marker `!popcapdrmprotect!`.
`tools/extract_game_exe.py <install>/WordHarmony.exe -o re/wordharmony/bin/WordHarmonyGame.exe` carves it out (671,744
bytes). `game/GameText.cpp` finds the same exe inside the player's `WordHarmony.exe` at run time to read its texts.
<!-- /private -->

The game exe was linked on 2006-02-22 by Visual Studio .NET 2003 (internal name WordBall, "Release-2.4.3") and has
RTTI only for three engine classes. The game is 0x401000-0x424000 (the controller up to 0x409c00, the screens, the settings and saved games to
0x40df90, the physics and balls from 0x40e100, the levels, the ball system 0x411190-0x41c2f0, the saved game's writer
0x41e760-0x41f7e0, the HUD from 0x4221c0); the engine follows it up to 0x446000, then the C runtime, the engine's
Windows layer (0x454a40-0x45ee00) and the libraries (libpng 1.2.5, zlib 1.1.4, the JPEG decoder). `WORKING_NOTES.md` has what each part does, as read.

`WHArchive.saf` is Sprout's archive. The port reads it through the engine's own reader (`engine/ResourceManager.cpp`),
and for reading the data it is extracted to the git-ignored `re/wordharmony/saf/`. The game names its texts by
address: `GT(0x...)` in the code, and `tools/make_gametext.py` writes `game/GameTextTable.inc` (each text's address,
length and hash, no text) from the carved exe.

## Reproducing the analysis

The export lives in `re/wordharmony/` (git-ignored, like everything derived from the game). Ghidra finds no vtables
in a program without RTTI, and a virtual function only ever called through its vtable is never disassembled; nor does
it know how many arguments an indirect `__thiscall` takes. `tools/vtables.py` lists the vtables
(`re/wordharmony/vtables.txt`) and `tools/ghidra_pass.sh` creates the missing functions, gives every indirect call its
argument count and exports the decompilation again (`all.c`, `functions.jsonl`, `strings.jsonl` in
`re/wordharmony/gh`). The Ghidra project is `wordharmony_game` in `E:/retools/projects`.

```
python games/wordharmony/tools/vtables.py                           # re/wordharmony/vtables.txt
build/native/rundesk.exe bash games/wordharmony/tools/ghidra_pass.sh  # the functions and calls; the export
python games/wordharmony/tools/make_gametext.py                     # game/GameTextTable.inc after a new GT()
```

## Building and running

```
tools/setup_worktree.sh                                                         # a new worktree: the git-ignored files
python games/wordharmony/tools/stage_wordharmony_assets.py <the game's folder>  # stages assets/wordharmony/
TARGET=wordharmony tools/build.sh native                                        # build/native/wordharmony.exe
build/native/rundesk.exe bash -c "build/native/wordharmony.exe -userdir=<dir>"  # a game, invisible and silent
```

`TARGET=wordharmony_enginetest` builds a test that parses every XML file of the archive with the engine's parser.
`tools/build.sh web` builds every game for the browser, `wordharmony.html` among them; to build only Word Harmony,
configure `build/web` with `emcmake cmake` as `build.sh` does and run `cmake --build build/web --target wordharmony`.
Serve `build/web` (`.claude/launch.json` has `wordharmony-web` on port 18778) and open `wordharmony.html`; in the
Browser pane always with `?silent=1`. The page imports `WHArchive.saf`, the `res` folder and `WordHarmony.exe` from
the install folder or a zip of it into its own IndexedDB folder (`/whpersist`) and keeps the settings, saved games
and high scores there.

Native options: `-gamedir=<dir>` (else `assets/wordharmony` next to the exe or up to two levels up, or the retail
install "PopCap Games/Word Harmony"), `-userdir=<dir>` (the emulated registry, `registry.ini`: the settings, the
three saved games as XML and the high scores), `-fast` (a virtual clock, which also makes the game's `srand` of the
tick count repeatable) and `-script=<file>` (the framework's scripted input: `<update> click <x> <y>`, `rclick`,
`down`, `up`, `move`, `key <name>`, `char <c>`, `screenshot <file>`, `quit`). The rope that pulls a ball follows the
mouse while the button is down, so a script drags a ball with `down`, a few `move`s around the ledges, and `up`.

## Testing

`tools/check.sh wordharmony` (about 20 seconds) plays two scripted games with `-fast`. The first starts from a THINK
game saved by the suite itself (a chain O-R-D on a ledge of level template 4 and a W below it): it continues the game,
drags the W up, over the ledge and down before the O, sends WORD with the right button, ends the level, continues to
level 2 and saves it, and reads the saved level and score back from `registry.ini`. The second starts a TSUNAMI game,
lets the balls fall for 20 seconds and saves it (the balls in play and the letters to come). Each game runs twice and
must give the same result and the same screenshots. The scripts click no link button (the menu's link is empty in
this build anyway).

Other runs (2026-10-26) looked at the splash, the menu, every mode's level choice and help screen, the options and
the high scores, Save/Quit with Save Game and with Continue Game. In the browser (the Browser pane, silent) the game
was imported from a zip, played a THINK game, saved it with Save/Quit, and offered and restored it after a reload of
the page. Not yet tested: a RELAX level played through, hints, recycling a word, a high score's name entry, the screen
saver, the options' check boxes, and listening to the sound and the music (every test runs with the silent driver).

## Mods

None (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones worth
making.

## Status (2026-10-26)

The game plays natively and in the browser. Left out: the beta's play time reporter (an online feature) and the crash
reporter's minidump; the level editor's own functions, which this build never reaches (the branches for it inside
live functions are ported). The engine's ACM reader is not needed: the music clips are MP3 inside WAV files, which the
platform's mixer now decodes (`platform/Audio.cpp`). On the web the game's full screen (its options and its
settings at the start) goes to the page's stage, as the page's Fullscreen button does; a browser grants it only soon
after a click, so the game always starts in the page.

## Where the work was hard

The engine draws through x87 code whose floats Ghidra often loses; the image blitters and the water's waves needed the
disassembly beside the decompilation. The engine's classes use virtual bases and multiple
inheritance without RTTI, so a method's `this` is often a part of the object 0x20 or more bytes in (the ball system's
container part), and its offsets must be moved before they mean anything. The saved game's writer takes each chain's
balls out of the game as it writes them, which works in the original only because the settings read the text back at
once; the port keeps that order.

## Changes outside the Word Harmony folders

`CMakeLists.txt` (the `BUILD_WORDHARMONY` option), `platform/Audio.cpp` (MP3 inside a WAV), `tools/check.sh` (the
`wordharmony` suite), `.claude/launch.json` (the `wordharmony-web` preview on port 18778) and, from the coordinator,
the store entry in `cmake/ByopopStores.cmake`.
