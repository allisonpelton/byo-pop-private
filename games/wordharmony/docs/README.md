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

<!-- private -->
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

The identical-play pass (`IDENTICAL_PLAY.md`) adds the test switches `-statelog=<file>` (the game's values before and
after every frame), `-soundlog=<file>` (each sound's start and stop with its frame and length) and `-splashhold` (the
original's two seconds of splash, which the port otherwise skips, so that a run's updates line up with the original's),
and its tools: `tools/identical` (phase one's cases, `steady.sh`, `sndcmp.py`), `tools/shots` (phase two's tours, natively
with `port.sh`, the web build under Node with `web.sh`, the original with `orig.sh`), `tools/realplay/play.py` (real
window messages natively) and `tools/web/onegame.js` (real DOM input in the player-only page).

Other runs (2026-10-26) looked at the splash, the menu, every mode's level choice and help screen, the options and
the high scores, Save/Quit with Save Game and with Continue Game. In the browser (the Browser pane, silent) the game
was imported from a zip, played a THINK game, saved it with Save/Quit, and offered and restored it after a reload of
the page. Not yet tested: a RELAX level played through, hints, recycling a word, a high score's name entry, the screen
saver, the options' check boxes, and listening to the sound and the music (every test runs with the silent driver).

### Test commands

The standard test commands (`docs/TEST_COMMANDS.md`) are in `src/TestCommands.cpp`, reached only from a `-script` (or
the player-only page's command call); a run that never gives one is unchanged. Each answers on stdout with `OK`,
`ERROR`, `STATE {...}` or `MOVE {...}`.

`state` prints `screen` (menu, levels, help, game, paused, quit, gameover, nameentry, highscores, congrats, options,
and the like), `state` (the game's own number), `mode` (relax, think or tsunami), `level` (from 1), `score`,
`lettersleft`, `hints`, `over` and `frame`; while a game is on also `water`, `submit` (the submit button shows),
`held` (a ball on the mouse), `shortest`, `bonus`, `words` (RELAX's words still to make) and `balls`, each with its
index, letter, place in screen coordinates, radius, state, whether it is fixed and whether it is in a chain.

`goto options|highscores|help|relax|think|tsunami` (from the menu), `level <n> [mode]` (through the levels screen's own
action), `unlock all`, `set score|hints|lettersleft|water <value>`, `win` (the level ends as done), `lose` (the game
over), `seed <n>` and `move`. `botplay <updates>` lets the bot play on its own for that many updates, through the
widget's mouse calls (a test of the bot, not real input).

`move` answers from the port's bot (`src/Bot.cpp`): the help screens' PLAY GAME, the level panel's and level end's
CONTINUE, Save/Quit's Don't Save, and in a game the next drag or right click. The bot picks the word it can make with the
fewest drags from the dictionary's own search (RELAX: the level's words), in a row or, in RELAX, a column read from the
top. It anchors the word on a ball that rests, a chain's ball or a letter fixed in stone, and fills each place from the
anchor outwards with a ball of its letter that the mouse can reach. The field is a grid of 4-pixel cells, those a ball
fits in apart from the blocks and the letters fixed in stone; a ball is carried along the shortest way through it (round
other balls where it can) to its place, and a place over nothing comes last and is held until the word counts (RELAX
scores a word while its balls touch). A wrong letter on a place is first carried to the top of its own part of the field.
A word on a chain uses all of the chain, and when a chain's ball hangs past a ledge's end the whole row is moved along
until every place stands on something; a chain is read as it lies, left to right, as the game reads a row, and when no
word is left and a chain lies fallen over (reading backwards) its end is lifted over its other end. A plan that failed is
tried again once its anchor has moved, eight times a word at most, and a ball left on the mouse (a release lost to a focus
change) is let go with a click. In TSUNAMI it spawns the next ball only while the water is below 0.45. THINK never ends by
itself, so when nothing is left to make for twelve asks the bot leaves by Save/Quit and Don't Save, which ends the game.
It still stops on some RELAX levels whose letters lie walled in until another word breaks the stones (level 4).
`WH_BOTDEBUG=1`, or the command `botdebug` (`botdebug off`
ends it; for the web build, which has no environment), prints its plans on stderr.

## Performance (2026-11-08)

Measured as `build/slots/news.md` (2026-11-08) asks, with `tools/profile/perf.py` and `run_all.sh`, the original and the
port back to back on this machine, every run started only once `tools/gamecount.py --limit 3` had room. The original ran
under origrun on its real clock from a fresh copy of the staged game with phase one's patches (the update counter and the
seed), its registry key cleared first; the port ran natively without `-fast`. Both were sampled every 0.1 s: working set,
private bytes and processor time (kernel and user).

**Loading**, from the process's creation to the main menu, the first screen that takes input. The original: 2.92, 2.86
and 3.40 s (and 2.81, 2.57, 2.81 s in the minute's runs); that is its splash, held for two seconds of frames, and its
loading after the splash's first frame. The port as a player gets it (no `-splashhold`): 0.74, 0.71, 0.69, 0.68 and
0.86 s, median 0.71 s, with 93.5 MB working set and 87.7 MB private at the menu (the original: 78 and 56 MB). The web
build was not timed: the only browser here is the hidden pane, which runs a page's frames on a shimmed timer and reads
the game's files from the page's own store, so its time would measure the pane rather than the build.

**The scripted minute**: tour 2's TSUNAMI game (`tools/shots/tour2.txt`), frames 2000 to 3800, 60 seconds of play at the
game's 30 frames a second, in real time on both sides. The port's script counts frames, and in real time the framework
updates every 10 ms with a frame every third or fourth update, so `perf.py` puts frame k at update ceil(3.3 k); the
minute starts 67 s after launch on both sides.

| Run | Peak working set | Peak private | Processor time | Machine busy | Other game processes |
| --- | --- | --- | --- | --- | --- |
| Original A | 67.3 MB | 52.6 MB | 3.41 s | 67% | 2 |
| Port A | 161.3 MB | 155.1 MB | 13.56 s | 82% | 2, then 4 |
| Original C | 67.3 MB | 52.6 MB | 2.23 s | 54% | 2 |
| Port C | 153.3 MB | 154.1 MB | 10.28 s | 36% | 2, then 3 |

A second pair (B) was taken too; its port run met six game processes of other sessions and a machine 99% busy (its
frames fell to 22.8 a second), so it is left out and pair C replaces it. The original's B run: 77.8 MB, 55.5 MB, 5.05 s.
The port needs about three to four times the original's processor time over the minute (17 to 23% of one core against
4 to 6%) and two to three times its memory. Nothing has been done about either yet; that is the performance pass's.

## Mods

The fixes mod, `mods/fixes/mod.json` (`docs/MOD_ROADMAP.md`, "The fixes mod"): `steadyClock` times every frame as one
thirtieth of a second of the game's time instead of the machine's clock, so that the same moves on the same seed play the same
game on every run (`tools/identical/steady.sh` checks it in real time), and `fairFirstWord` clears the time of the last word
that the original never sets, so the first word after starting the program is no longer always quick. The web build loads
mods from `/whpersist/mods` and the site serves the fixes mod on; natively they come from `-modsdir=<dir>` or the
`mods-wordharmony` folder next to the exe, and `-nomods` starts without any. `MOD_IDEAS.md` lists other mods worth making.

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
