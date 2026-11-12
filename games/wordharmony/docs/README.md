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
and the coverage check's `-inputlog=<file>` (the mouse steps that `botplay` takes, as `-script` lines), and its tools: `tools/identical` (phase one's cases, `steady.sh`, `sndcmp.py`), `tools/shots` (phase two's tours, natively
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

`memreport [file]` prints what the port holds in memory as one `MEMREPORT {...}` line (the standard line, `docs/TEST_COMMANDS.md`):
the pictures' pixels and the screen texture, the decoded sounds, the two word trees under `others`, and the process's working set
and private bytes; with a file it writes the detail there too (each picture, the heaps' block sizes). The framework answers it
and `WordHarmonyApp::AddMemReportHolders` (`src/TestCommands.cpp`) adds the game's holders. It changes nothing.

## Performance before the pass (2026-11-08)

The performance pass (2026-11-11) follows this section. Measured as `build/slots/news.md` (2026-11-08) asks, with `tools/profile/perf.py` and `run_all.sh`, the original and the
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
4 to 6%) and two to three times its memory. Nothing had been done about either yet; that was the performance pass's.

## Performance pass (2026-11-11)

The pass changes how the port works, never what it does: its changes are in `src/Render.cpp`, `src/WordHarmonyApp.cpp` and
`game/Dictionary.h`, no draw call, game rule or random draw changed, and the frozen baseline (`tools/check.sh wordharmony`:
the two scripted games, their saves and screenshots) and the pictures of phase two's tours 1, 2, 3 and 5 (74 shots and each
tour's state log, `tools/profile/pixcmp.sh <old exe> <new exe> <dir>`, the old and the new exe side by side with `-fast`
and seed 1) are byte for byte the same after each step.

**Where the time went.** `tools/profile/cpu.py` samples the running threads of the port in the scripted minute (as
Bejeweled's `cpuprof.py`; its stack walker is `cpuprof_stacks.py`, which follows the frame-pointer chain of a build made
with `-fno-omit-frame-pointer`, so that the time in Windows and the graphics driver is charged to the exe function that
called out). Of the main thread's cycles, the software blitter was about 36% (`ScreenGraphics::drawImage` and
`PixelOp::apply`, a call and a switch for every pixel), the framework's presenting of the frame (`PrepareTexture`'s
`SDL_UpdateTexture` of the 800x600 picture 4%, the Direct3D 11 driver's worker thread 14%) the next largest, and the game's own logic (`WordNode::find`, `AnagramNode::findWords`, `Container::update`) a few percent each. A first
profile that charged samples by the cycles used read "`GetTickCountMS` 24%" and "ntdll 35 to 67%": the sampler, whose long
interval puts a burst of work on whatever instruction is running when it wakes, made them up. The loop in `SexyAppBase::Process` that
looked like a spin on the clock is not one (counting its calls and the thread's cycles shows no spin), so the profiler now
counts only the samples at which the thread's cycle counter advanced.

**What the pass changed**, in the order of their gain:

1. The blitter chooses the pixel operation (copy, colour key, blend, add) once for a blit, in a template (`applyPixel<OP>`,
   `withOp`), not once for every pixel; an unrotated blit works out which source column each destination column reads once
   (`sBlitColumns`) and, when each destination column reads the next source column (an unscaled blit), does a row as one run
that the compiler makes into wide loads and stores. The blend works
   on the red and blue channels in one 32-bit multiply and divides by 255 as `(x + 1 + (x >> 8)) >> 8`, which equals `x / 255`
   for every product the blend makes (0 to 65534; checked against the old per-channel formula for every channel, alpha and
   destination value, and 400 million random pixels). A blended pixel of alpha zero is not written.
2. The screen is cleared lazily (`ScreenGraphics::clear`/`finish`): when a frame's first blit is an opaque copy of the whole
   screen (the menu's and the game's backdrops), the clear and its 1.9 MB of writes are skipped; any other first draw clears
   the screen first, so a frame never shows the last frame.
3. The screen picture is opaque and uploaded as it is (`SetImageMode(false, false)` and `mUploadAsIs` in `WordHarmonyApp`'s
   widget, as Water Bugs and Cosmic Bugs do): the framework no longer scans the 480,000 pixels for alpha and premultiplies
   them again every frame before `SDL_UpdateTexture`.
4. The two word trees (the match tree, 381,205 nodes; the hint tree, 61,691 nodes and their word lists) allocate their nodes
   from pools of 1,024 (`NodePool` in `game/Dictionary.h`), never freed, as the trees live as long as the game: no heap
   header on each of the 443,000 nodes (about 3.5 MB by count), and the nodes of a word lie together for the lookups. The
   process's private bytes vary by more than that between runs (below), so the saving does not show in them.
5. `memreport`, the standard memory report (`docs/TEST_COMMANDS.md`), reached from a script (below, "Test commands"): the
   pictures (`App::getImageMemory`), the sounds (`HostSoundSystem::getSoundMemory`) and the two trees, with the process's own
   numbers, and with a file argument the heaps' block sizes. `tools/profile/mem.py` plays the scripted minute to its middle
   and asks it, then `tools/memmap.py` reads the process's address space; with `--series` it asks eight times over the
   whole tour (frames 600 to 7450).

**Processor time** over the scripted minute (tour 2's TSUNAMI game, frames 2000 to 3800, in real time with the machine's other
sessions running, as in the table above). Processor time moves with the machine's load: run back to back, the same port took
11.1 s with the machine 83% busy and 6.5 s with it 69% busy, so the runs that count are side by side, two processes started
together that meet the same load (`tools/profile/pair.sh`, `perf.py` for each). All of them drew 30.3 frames a second.

| Side by side | Original | Port before the pass | Port after the pass |
| --- | --- | --- | --- |
| Original and port after | 4.20 s, 3.19 s | | 6.86 s, 5.69 s |
| Port before and port after | | 34.5 s, 16.3 s | 7.17 s, 6.97 s |

The port needs about 1.7 to 2.2 times the original's processor time over the minute (9 to 12% of one core against 5 to 7%),
where before the pass it needed three to four times (the table above, back to back) and, side by side, 16 to 35 s. The frames are the same. The original's own figure moves from 2.2 s (a quiet
machine, 2026-11-08) to 5.2 s with the machine busy.

**What is left** is mostly not the game's. About 38% of the running samples are the blitter, of which a third is one
instruction, the load of the source pixels in the backdrop's whole-screen copy (480,000 pixels read and written 30 times a
second, the samples sit on the memory's wait and not on the loop's work), and about 35% is the framework's presenting of each frame
(`SexyAppBase::Redraw` and `DDInterface::PrepareTexture`: `SDL_UpdateTexture` of the whole 1.9 MB picture, the copy into the
screen texture, the Direct3D 11 driver and its worker thread). Another SDL render driver does not help: Direct3D 9 costs the
same (6.2 s against 6.25 s side by side), OpenGL more (7.4 s), the software renderer far more (92 s). Two changes that would
cut both halves are not made, since they change how the screen is drawn and one is the framework's: drawing only the tiles
whose draw calls differ from the last frame's, and uploading only those rectangles. They go to the coordinator as a spec.

**Loading** (`perf.py port-load`, five runs): 0.79, 0.79, 0.82, 0.85 and 0.99 s, median 0.82 s, from the process's creation to
the main menu, the original's 3.24, 3.35 and 3.31 s (its two seconds of splash included). Before the pass it was 0.71 s; the
pass did not touch loading, and the machine was 53 to 64% busy during these runs.

**Memory.** The report at frame 2700 of the real-time minute: 119 pictures hold 15.4 MB (the screen picture, 1.83 MB, is
the only one with a texture of its own), the 55 decoded sounds 20.2 MB, the match tree 5.8 MB and the hint tree 2.1 MB, 43.5
MB the game holds. The process's private bytes were 87.5 MB (the same exe's series runs ended 88, 92, 92 and 94 MB, the exe
before the pass's 91.5 MB in two runs: the part outside the game's own holders differs from run to run); the first heap holds 50.9
MB in 43,000 blocks and the rest, about 37 MB, is outside the heaps: the graphics driver's private memory, thread stacks
(2.4 MB) and mapped files (10.2 MB). `memmap.py` sees the driver's own libraries too (`nvwgf2um.dll` 50.7 MB and
`nvgpucomp32.dll` 26.6 MB of mapped images, which are shared and not private). Over the whole tour the private bytes
grow by 0.5 to 0.7 MB between frames 600 and 7450 in each run, so nothing leaks. The original holds 53 to 57 MB private and 69 to 79 MB
working set at the same point, so the port keeps 30 to 40 MB more: the decoded sounds, 20 MB, and the Direct3D driver's
memory against the original's DirectDraw; how the original's own memory splits was not measured (`tools/memmap.py` on its copy under
origrun would). The earlier 154 MB private came from runs that also took screenshots (the PNG encoder's buffers); `perf.py`'s minute
still takes its two marking shots, so its peak is about 150 MB, and `mem.py` takes none.

The tools are in `tools/profile`: `perf.py` (the load, the minute, the original's run), `run_all.sh` (back to back),
`pair.sh` (side by side), `cpu.py` with `cpuprof_stacks.py` (the sampling profile), `mem.py` and `pixcmp.sh`.

## Performance, second round: the target (2026-11-11)

The user's rule (docs/COORDINATION.md, "The target"): a port's processor time is at most its original's, measured like for
like. This round reopened the step. **The processor-time figures above (every table of seconds, and the "1.7 to 2.2 times")
came from `GetProcessTimes`, which charges by the 15.6 ms clock tick, so they are replaced by this section's.**

**The method.** Processor time is the cycles of every thread of the process, `QueryThreadCycleTime` summed over the threads
(`perf.py`'s `ThreadSampler`: the thread list from Toolhelp every second, each thread's cycle counter every 0.1 s, the
last value kept for a thread that ends, a thread named by the module its start address is in; the figure is
`cycles_in_minute_g`, in billions, over the same scripted minute as above). The original and the port run alternately,
five pairs, the original first in odd rounds, the whole loop on one pinned core (`python -I tools/pincore.py -- bash
games/wordharmony/tools/profile/alt.sh <out dir> 5`), summarised by `altsum.py` (the median of each side, the port's against
the original's, with and without the port's audio thread). The original runs under origrun on its real clock with
`--update-ms 33` (a frame every 33 ms, as the port's 30.3 a second) and the patches in `tools/origrun/patches/wordharmony.txt`
(its screen-copy patch included; a variant without it measured the same); the port runs with `-script` and no `-commands`.
A cycle count still moves with the machine (cache and memory contention, frequency: single runs of one exe differed by up to
twice), so only the pairs count.

**Where the time goes, thread by thread** (the scripted minute, billions of cycles, medians of five pairs):

| | Total | Main thread | Graphics driver's worker | SDL audio thread |
| --- | --- | --- | --- | --- |
| Original | 11.88 | 11.88 (its only thread) | | |
| Port before this round (one early pair, HEAD's renderer) | 9.95 to 11.5 | about 8.9 | about 1.1 to 2.5 | 0.12 |
| Port now | 8.12 | 5.65 | 2.34 | 0.12 |

The original's runs ranged 10.1 to 14.1 G and the port's now 6.7 to 10.1 G; the port was lower in all five pairs
(original / port: 12.7 / 7.8, 10.1 / 6.7, 11.9 / 8.1, 11.2 / 9.6, 14.1 / 10.1). **The port's total is 0.68 of the original's
(0.67 without the audio thread, a software mixer the original's DirectSound has no counterpart of in the exe).** Before this
round the port's main thread alone was about the original's whole, and the driver's worker was the excess.

On the idle main menu (600 frames, three pairs) the original takes 2.97 G and the port 2.75 G, 0.93 (0.91 without the audio
thread): a frame that changes nothing costs the port nothing now, and what is left there is the driver's worker (about 0.85 G) and the
framework's redraw once a second.

**Where the main thread's cycles went before the cut** (one instrumented 900-frame run, per frame, in millions of cycles; the
instrumentation was reverted): RunFrame 2.66, of which the draw calls 2.44 (about 200 a frame); the whole-picture
`SDL_UpdateTexture` 0.58; `SDL_RenderPresent` 0.77; the rest about 0.4, 4.46 in all, with the driver's worker about 1.15 more.
In the TSUNAMI window only about 7,900 of the 480,000 pixels change from one frame to the next.

**What changed: retained drawing** (`src/Render.cpp`, `ScreenGraphics`; item 2 above's lazy clear and item 1's
`sBlitColumns` are gone, the pixel operations are the same templates). `drawImage` and `drawLine` no longer draw: they record
the call (`Blit`: image serial, operation, the rectangles, colour, 23 32-bit words, and for an unrotated call its
source column and row maps). `finish()` sorts the calls into per-tile lists (32 by 32 pixels, 25 by 19 tiles) and redraws a
tile (a black fill, then its calls clipped to it) only if its list differs from the last frame's, and a call counts as the
same if its record is equal or it has the same image, operation and rectangle and equal column and row maps: the same
per-pixel code on the same inputs gives the same pixels, so no tile that is kept can differ from what a redraw would give. About 52
tiles of 475 are redrawn a frame in the TSUNAMI window (a first version, comparing the raw records, redrew 97 because
fractional positions change a call's record and not its pixels). A frame in which no tile changed returns
false from `finish()`, and `RunFrame` then calls neither `BitsChanged()` nor `MarkDirty()`: the framework neither uploads the
picture nor presents, and its once-a-second redraw (`DrawDirtyStuff`) still shows the window. A frame that changes calls
`BitsChanged()` as before, so the whole 1.9 MB is still uploaded then (TipTop measured an upload at 0.3 to 0.45 ms a call
whatever its size, so uploading only the changed tiles would not pay as several calls).

**Proof that nothing visible changed.** `-framehash=<file>` writes the frame number and a 64-bit hash of the whole 800x600
picture after every frame, and `tools/profile/hashcmp.sh <old exe> <new exe> <dir>` runs tours 1, 2, 3 and 5 with both exes
(`-fast`, seed 1) and compares them frame by frame, and their state logs: 16,346 frames, identical, the old (draw at once)
renderer against the final one. `tools/check.sh wordharmony` (the frozen baseline: the two scripted games, their saves and
screenshots) is the same, and so is `tools/profile/pixcmp.sh` (74 shots).

**The two questions.** Does the screen need uploading on a frame where nothing changed? No, and it is no longer (above).
Does the original draw as many frames a second as the port? Yes: one frame every 33 ms, 30.3 a second on both sides (the
original redraws the whole root window every frame, the port's frame is the same one), so the port's saving is not fewer
frames.

**What is left, for the coordinator** (the shared costs, not changed here). The main thread's `SDL_UpdateTexture` of the whole
1.9 MB (about 0.58 Mcycles a frame) and `SDL_RenderPresent` with vsync (about 0.77, plus the driver's worker thread at about
1.15) are on every frame that changes. The framework draws the screen texture into the window in a second pass. A
dirty-rectangle upload (a `MemoryImage` rectangle that `PrepareTexture` consumes) would need a framework change and is
unproven by TipTop's measure. Vsync off (`-fast`) looked cheaper in a noisy single run, but Alchemy's notes found it
costlier; neither is a measure like for like.

The new tools in `tools/profile`: `alt.sh` (the alternating runs), `altsum.py` (their summary), `hashcmp.sh` (the per-frame
hash comparison), and in `perf.py` the thread sampler and the knobs `WH_PERF_PATCHES` (the original's patch file),
`WH_PERF_FRAMES=<from>:<to>` (a shorter window) and `WH_PERF_EXE`.

## Coverage check (2026-11-11)

`docs/COVERAGE.md` describes the check. `coverage.txt` beside this folder is its settings file (the export, the sources, the
two ranges of the exe's game code, and a `none` line with its reason for each function the port leaves out on purpose),
`tools/coverage/run.sh` runs the original with origrun's `--coverage` on the export's 3432 functions, and
`tools/extract_saf.py` unpacks `WHArchive.saf` into `re/wordharmony/saf`, where the layouts give the click positions.

**The runs.** Eighteen runs of the original on the host, each a minute or two on the virtual clock: the five phase-two
tours; three whole games that the port's bot played (`botplay`, with the new `-inputlog` writing every mouse step the bot
took as a `-script` line) and the original replayed (TSUNAMI, RELAX and a saved THINK game); two runs of a UI driver (the
help pages, the hint, wildcard and submit buttons, the spawn ball, pause, retry, save and quit, exit; the second without
the wildcard click, see below); one run on the real clock with the silent stand-in, for the sound code; three runs that tick
each mode's "don't show this screen again" box; a THINK game saved and then continued (the registry kept between the two
runs); and a THINK run that waits out the level end's bonus count. The replays are not frame-exact: the bot calls the
widgets' handlers inside an update and a script's click goes through the framework's event dispatch, so a replayed game
drifts from the recorded one within a few hundred updates. That does not matter here, which asks only what code a run
reaches; the comparisons of the identical-play pass stay the ones that prove the play.

**The numbers.** The game's code (0x401000-0x445f40 and 0x453f80-0x45eda0) is 1562 functions. The five tours reached 1073
of them and left 485 with no function in the port; the 18 runs together reached 1119 and leave none. The port defines 749
of the 1562 and leaves 457 out on purpose. Of the 485, 51 were translated functions whose address was not above them
(inlined helpers, constructors of the engine's value types, the sound system's slots, the window procedure): their
addresses are now in the comments above the functions that hold them. The other 434, and 23 that later runs found, are
`none` lines, sorted by what they are: the C++ library's templates (MSVC 7.1 `string`, `vector`, `list`, `map`, `deque`)
that the port's own library replaces, the compiler's thunks, deleting destructors and virtual-base constructor halves, the
Windows layer (window, message loop, DirectDraw, DirectSound and its wave readers, the archive's file and keyed MD5
readers) that the framework and the platform replace, the registry helper, the libpng wrappers, the beta's play time
upload and frame rate display, the exception classes' copies, and the main menu's game link. The 105 port functions that
the first report found unreached are 84 now.

**What it found.** Five differences between the port and the original, every one in a path that no comparison had played:

| The difference | The original | The port did | Now |
| --- | --- | --- | --- |
| A key held down (the window procedure, 0x454510) | a key message with the repeat bit set is dropped | every repeat reached the game as a new press | a repeat of a key already down is dropped; a focus loss forgets the held keys |
| The mouse position (0x454240) | clamped to the window, 800 by 600 | passed on as it came, so a drag outside the window gave positions beyond it | clamped for moves, presses and releases |
| The bonus word's pick of every ball (`pickAll`, 0x412ca0) | each free ball is picked 0.1 s after the one before it | all at once | staggered as in the original |
| An exception's message box (WinMain, 0x455567, 0x45559a, 0x45578a) | captioned "Exception" with the error icon; the text of the engine's own exception, else "Exception Code Honeysuckle" | captioned "Word Harmony", with the exception's text | the original's caption, icon and fixed text |
| A press on the faded-out wildcard button (0x422850) | with no wildcard held it still makes one and the count goes to -1, which the next level's start turns into 2^32 letters: an exception and the end of the game | the same, as a `std::bad_alloc` at the same update (1939 of the UI driver's script) | no wildcard held, no press: a **deviation**, a crash of the original that the port does not keep (CLAUDE.md, "Port first, unmodified") |

The first two came from reading 0x454240 and 0x454510 again for the functions that the first report listed as reached with
no counterpart. The third came from reading `pickAll` for the unreached list (it needs a found word that makes the level's
bonus word). The fourth and fifth came from the UI driver: its first run ended in the original's exception box after the
RETRY click, because an earlier click at (705, 285) had pressed the wildcard button in a TSUNAMI game that held no wildcard;
the port, given the same script, ended at the same update. `tools/check.sh wordharmony` still passes with its frozen
baseline identical, as those paths are not in it.

**The port functions that no run reached (84).** Judged one by one against the export's callers and by reading; none needs
a run of its own:

- *The screen saver* (5: `endProgram`, `startScreenSaver`, the saver's constructor, `mouseMove` and `keyDown`). The original
  starts it from its command line, with a registry value that its installer writes under HKLM (`SaverInstallPath`), and stops
  it at the first key or mouse move. CLAUDE.md allows changes only to PopCap's registry values, so it has no run; the port's
  code was read against the export again and matches.
- *Destructors* (14: `Game`, `WordBallSystem`, `BallArray`, `Ball`, `HighScores`, `WordTree`, `WordNode`, `AnagramNode`,
  `Button`, `MusicSystem`, `MusicLayer`, `ParticleStream`, `SoundBank`, `HighScoreEntryLayout`). None ran in any run, which
  between them ended levels, games and the program: the original leaves through `ExitProcess` and frees none of them.
- *The level editor* (15: `Box::touches` and `Ramp::touches`, the obstacles' getters and setters, both `toXml`,
  `Ball::setLetter`, `findObstacle`, and `randomLetters`, whose only caller is the editor's ball spawn). This build never
  reaches the editor (README, "Status").
- *Dead code of the original* (38: engine functions with no caller in the export, such as `Node::hitTest`, `dump` and
  `parentToLocal`, `Container::start` and `stop`, `TextButton`'s text and mouse handlers, `Button::setBaseScale` and
  `setFlash`, `SoundGroup`, `App::addImage`, `getFileFont` and `createFont`, `Font::loadFile`, the JitterListener's setters,
  `Graphics::drawImage(x, y)`, `getColor` and `setColor`, `IntRect::offset` and `intersect`, `HostSound::halt` and `setPan`,
  `HostSoundSystem::unload` and `unloadUnused`, `Follow::apply`, `HighScoreScreen::reset`).
- *Code that no data of this build reaches* (5: `LayoutHandler::addInclude` and `addTextSub`, since no layout has an `include`
  or a `textSub` element; `ParticleStream::setAnchor`, since none has an `anchor`; and the particle system's `stopEmitting`
  with the stream's, since the data's `autoStop` is on the streams and never on a system).
- *Read again against the export, which matches* (7): `queueLetters` with the HUD's `submitN` branch (a found word's letters
  go back into the queue by its recycle button, THINK's only; phase one's list says this was never compared, and it still
  has no run, because the recorded bot's THINK levels each take one word), `pickAll` (the slip above), `TextItem::setTime`
  and `CongratsScreen::showResults` (the screen that finishing every RELAX level shows, reached through a virtual call; the
  format and its two pad strings checked against the exe's bytes), `TextEffect::isDone` (every `allDone` call of these runs
  met an empty list), `startsWith` (the XML reader's numeric entities, which no data file uses) and the catch of the engine's
  own exception (0x455567).

**Candidates for taking out** (step 3 of this pass, before the readability pass): the editor's functions above and the
branches of live functions that only it reaches, `randomLetters`, the dead engine functions, and the layout and particle
features that no data uses. They are taken out in the next section.

**Left open.** The recycle button and the bonus word's `pickAll` are the two paths of play that no comparison has run: the
first needs a THINK level that takes several words, the second a found word that makes the bonus word. Both are read against
the export. The original's wildcard crash is not kept (above).

**Cost.** Roughly an hour of machine time for the 18 runs and the few thrown away, one to three game processes at a time,
and a long session for the sorting (the 485 functions went into about 20 groups by a script that read their names, callers
and sizes).

## The unneeded code (2026-11-11)

Step 3 of the passes: the code that nothing in this build runs, taken out. The list is the coverage check's candidates
(above), each one searched for in the port first (a function with a caller anywhere stays) and the engine's other functions
that have no caller in the port scanned for the same way, then the layout and particle features no data file of the archive
uses (every XML file was read for the attribute or element). The sources went from 20,324 to 19,150 lines (engine, game
and src); the frozen baseline writes the same state log and screenshots as before.

**Taken out.**

- *The level editor and its HUD.* `WordBallSystem`'s editor mode (the flag, the dragged ball's `Follow`, the level backup,
  the editor branches of the mouse handlers, `step`, `spawn`, `startLevel`, `update`, `clear` and the key switch, whose
  other keys only the editor used), `findObstacle`, `Ball::setLetter`, the obstacles' `touches`, `isActive`, `setOrigin`,
  `getLetter`, `getType` and `toXml`, `Level`'s assignment (the editor's backup), the HUD's editor branches (`buttonPress`'s
  editor argument, the ball count buttons), `Game`'s grid snap and `randomLetters`, the editor modes of `chooseLevel`, and
  `config\EditorHUD.xml`, which `Game::loadData` read at the start and nothing ever showed. With it went `TextButton`
  and the layout's `textButton` element (only that file used them), `Button::setBaseScale` and `setFlash`.
- *Engine functions with no caller.* `Node::hitTest(Point&)`, `parentToLocal`, `start`, `stop`, `dump`, `getParent` and
  `setRotation`, and the container's `hitTest(Point&)`, `start`, `stop` and `dump`; `App::addImage` and `getFileFont`,
  `Font::loadFile` and `createFont`'s loose-file branch (the archive is the only source); `Graphics::drawImage(x, y)`,
  `getColor`, `fillRect` and `fillRectAdditive` (with the screen's two); `IntRect::offset` and `intersect`, `Rect::intersect`;
  the jitter listener's setters; `Selector::getSelectedIndex`; the sound system's `unload` and `unloadUnused`, `getPan`
  and `setPan` (a voice is never panned, so both channels get the same gain, as before), `halt` as a virtual,
  `SoundGroup::getPlayingCount` and `getFreeSound`; `HighScoreScreen::reset`; the window's stored mouse position;
  `Exception::getMessage` and `MemoryStream::getData`.
- *Features no data of this build reaches.* The layout's `include` and `textSub` elements; a particle stream's `parent`
  attribute (its anchor), and with it the application's map from node names to nodes (`registerNode`, `unregisterNode`
  and `findNode`; setting a name no longer inserts into a map); a particle system's own automatic stop (`setDuration`, the
  countdown in its update, `stopEmitting` and the `autoStop` read of the system element; the data sets `autoStop` on the
  streams, which stays).

**Kept**, each with its reason: the 14 destructors (the port frees what it allocates; the original
leaves through `ExitProcess`), the screen saver (its code is read against the export and matches), `HostSound::halt`
(`stop`, `close` and the destructor call it), `Graphics::setColor`, `Rect::offset`, `Obstacle::getOrigin`, `getExtents`,
`setLetter` and `clone`, `Bubble::toXml` (`SavedGame` writes bubbles), `Hud::hideBallButtons` (`Hud::load` calls it),
`Rect::width` and `height` and `IntRect`'s (the geometry types' small accessors, which the original's drawing reached),
`MemoryStream::seek` (the stream interface), and the XML reader's numeric entities (`startsWith`: the reader is the original's
and a hand-edited file can carry one).

**`coverage.txt`.** Fifteen of the removed functions were reached by the original in the five tours (the editor HUD's
text buttons are made at every start, the node map's functions at every named node, the empty function the original's slots
share): they now have `none` lines, with the reasons above, and the report of the five tours lists no reached function
without a definition or a line. The 18 runs of the coverage check were not repeated (the logs were gone); the removed
functions the original did not reach in them are the candidates of the list above, so no deeper run can add to the lines.

**Checks.** The native build and `wordharmony_enginetest` (52 XML files of the archive parsed, 0 failed; the 7 particle
files with their 8 streams, the 4 music files and the 17 sounds run); `tools/check.sh wordharmony` (the THINK and TSUNAMI games, each twice with the
same screenshots, then against `build/baseline-wordharmony`: identical); the web build.

## The readability pass (2026-11-12)

Step 4 of the passes, taken to `docs/CODE_STANDARDS.md`. `python -I tools/namecheck.py games/wordharmony` reports no
findings in the 13 files of `engine`, `game` and `src` (the checks of rules 2, 5 and 6 and the stage labels of rule 8); the
rest of the rules were read for by hand over the whole code.

**Names (rules 2, 3 and 7).** The fields the first draft named by their offsets, which the checker's `hexname` does not
see in upper case, now say what they hold: `mB0` is `mStateSeconds` (the seconds left in the ball's state: the picked
ball's wait, the pop's fade, a spawned ball's growing in), `m194` is `mScoring`, `m1F0` is `mHintsStale`, `m204` is
`mLongWordsRewarded`. The ball states `BALL_PICKED_2` and `BALL_STATE_4` are `BALL_POPPING` and `BALL_SPAWNING`, and the
enum says what each state is. One-letter names used over more than ten lines became words (the bot's `BotBall`, the
rotated blit's `aCos` and `aSin`), and the host's `kUnknownException` is `OTHER_EXCEPTION_TEXT`. Numbers that mean the
same thing in more than one place have names: `BALL_CAPACITY` (105), `BALL_RADIUS` (22, which the obstacles and the bot
use as well), `SPAWN_X` and `SPAWN_Y` (where a new ball appears and the blossom opens), `PHYSICS_STEP` (0.03 seconds),
`WATER_SWING`, `DIALOG_X` and `DIALOG_Y`, `LEVEL_WORDS` (the 14 words of a level and the lines of the HUD), and the bot's
two step costs. `SCREEN_WIDTH` and `SCREEN_HEIGHT` moved from the host into `engine/App.h`, where the game's own code
reads them, and `AppConfig::mFullScreen` names the window's flag at +0x9c (false, and not read by the port).

**Unread fields (rule 3).** Four fields and two words were never read: `Window::mUnknownBC`, `Ball::mE4`,
`WordBallSystem::m1FC`, `TextEffects`' `m28` and the last two words of `RenderState`. Nothing in this port saves or
hashes them (the state log and the saves write named values, not the objects), so they came out, and the render state's
comment says that the original's 0x40 bytes end in two words nothing reads.

**Addresses (rule 6) and comments (rule 8).** Every function in `SoundHost`, `Render` and the host names its address, or
the file's first comment carries `port:` (`port(mod):` in `ModFeatures.cpp`). The comments that said "performance pass"
now say what the code does for speed; none names a stage.

**A difference found.** Reading `SoundHost::createSound` against the original's (0x453e10 sets a sound's category, which
`Game::applyVolumes` then fades: 0 for the sounds, 1 for the music) showed that the port created every clip without a
category, so the music followed the sound effects' setting instead of its own. The sound
system's `createSound` takes the category now and the host sets it. The frozen baseline does not hear sound, so it stays
unchanged; the shared mixer is the same.

**Checks.** Every rename and constant is proved by the object code: `tools/objsnap.py` compares the 40 objects (2,123
functions) before and after and finds 0 differing, for the renames and for the constants; removed fields change a size
and so are proved by play, not object code. `tools/check.sh wordharmony` (each game played twice, then against
`build/baseline-wordharmony`) passes, and `pixcmp.sh` of the tours is identical to the build before the pass (74
screenshots). Against the baseline exe itself the tours differ by at most 3 levels over a few thousand pixels of the title
screen: that exe was frozen before the shared JPEG decoder changed, and a build of the tree from before this pass (same
decoder) matches the pass exactly. No string of the sources changed, and the web build compiles
(`tools/check.sh build-web:wordharmony`).

## The PopCap Game Collection version (2026-11-12)

The port was made from the retail copy (`WordHarmony.exe`, 983,458 bytes: the launcher with the game after it, beside
`WHArchive.saf` and `res/`), and the game is also on the PopCap Game Collection disc (2009; the group
`WordHarmonyDeluxeComponent`), so the port is checked against that version too: the two compared, and the port run on the
Collection's files (`docs/COORDINATION.md`). The procedure is Bejeweled's (its GAME_NOTES, section 23) and Astro Pop's
(section 13).

**Getting the files.** `unshield` 1.5.1 (`E:\retools\unshield-build`; the user allowed it on 2026-11-10) read the disc
image's cabinet, which was mounted read-only and dismounted afterwards: 60 files, 6.7 MB, in the main checkout's
`re/_installers/WordHarmonyDeluxeCollection` (git-ignored, deleted once this result was written down;
`unshield -g WordHarmonyDeluxeComponent -d . x F:/data1.cab`, in a folder made first, brings them back).

**The game is the same build.** The Collection's `WordHarmony.exe` (771,784 bytes) has no launcher around it. Set beside the
game carved out of the retail exe (`re/wordharmony/bin/WordHarmonyGame.exe`, `tools/collection/cmpexe.py`) it has the same
link time (22 February 2006, 02:49:43), and `.text` (520,192 bytes), `.rdata`, `.data` and `.reloc` are byte for byte the same.
Only `.rsrc` differs (100 KB against 8 KB: the icons and their group, 14 more resources), with 25 header bytes (the checksum
and the resource sizes) and 5,832 bytes after the last section, the exe's signature. `WordHarmony.scr`, the screen saver
the disc adds, is the same bytes as the carved game (the same SHA-1), and `SSInstaller.exe` installs it. So the
decompilation, the identical-play pass and the coverage check hold for the Collection's game as they are, and no code was
compared further. The Collection's own exe was not run.

**The files** (`tools/collection/cmpfiles.py`). Of the retail copy's 63 files and the Collection's 60, 51 are the same
byte for byte: `WHArchive.saf` (every picture, font, screen layout, level and word list), every sound and every piece of
music, and the buttons' pictures. The differences:

- `res/splash/sproutsplash.jpg` (`tools/collection/cmpimages.py`): other artwork, 88,601 of its 480,000 pixels. The
  retail picture says "Presented by" PopCap and "Developed by" Sprout Games under the title, with the TM beside the
  title; the Collection's has only a bigger PopCap logo and the TM after "HARMONY". The port draws the file it is given.
  The Collection also has the same picture in `images/sproutsplash.jpg`, which the game does not read.
- `properties/partner.xml` (and its `.sig`): the display name "Word Harmony Deluxe", another offering number, `NoReg`,
  `DontUpdate` and `DefaultWindowed`; the registry key `PopCap\WordHarmony` and the 640 by 480 size are in both. The port
  does not read it.
- The licence and the readme, and the exe.
- The retail copy alone has the launcher's files (`drm.swf`, `drmss.jpg`, `Flash.ocx`), `PopUninstall.exe`, Windows'
  `Thumbs.db` files; the Collection alone has `SSInstaller.exe`, `WordHarmony.scr` and the splash picture's second copy.

**Saves.** The code that reads and writes them is the same bytes, so a Collection game keeps its settings and saved games
under `HKCU\Software\GameHouse\WordHarmony` in the same format, which `platform/saveimport.json`'s Word Harmony entry
reads. No save of a Collection copy was at hand, so none was imported.

**The port on the Collection's files** (`tools/collection/run.sh <the Collection's WordHarmonyDeluxeComponent folder>`).
`stage_wordharmony_assets.py` stages the folder as it does a retail one (52 files). The texts come from its exe at the
same addresses (`GameText.cpp` finds a game image after the retail launcher's marker or takes the file as the image
itself): a run logs no `GameText` line, so every text matched. The runs are the port's, `-fast`, seed 1, with the
original's splash hold, through `WH_GAMEDIR` (`tools/check.sh`, `tools/identical/port.sh` and `tools/shots/web.sh` take
the game folder from it):

- `tools/check.sh wordharmony` on the Collection's files: the THINK and TSUNAMI games end in the states it expects and
  each plays every update as the frozen baseline does on the retail files.
- The five tours of `tools/shots` (the title and menu, a TSUNAMI game, the high scores, a saved THINK game, the focus;
  tour 4 starts from the suite's saved THINK game), each once on each set of files: the state logs (75, 47,486,
  25,085, 10,195 and 4,848 lines) and the sound logs are the same, and 94 of the 95 screenshots are the same pixel for
  pixel. The last is the first shot of tour 1 (`title_30`, the splash), where the five boxes shotdiff finds are the
  logos and the title: the artwork above. The shots after it, the menu and every dialog, are equal.
- The web build under Node on each set of files, tours 1 and 2: the state logs of the two sets are the same, 41 of the 42
  shots are the same (the splash again), and each web run against the native port on the same files has the differences
  the identical-play report records (the Options box's Fullscreen check box, 6 pairs of tour 1).
- The page importer's rules (`web/gameimport.js`, an ad-hoc run on the two folders' file lists): the Collection's folder
  is taken as a Word Harmony folder, its `WHArchive.saf`, `WordHarmony.exe` and `res` (53 files) are picked, and the
  markers check passes.

**Not done.** The Collection's exe was not run, so none of its screens was compared with the port's (the code is the same
bytes, the resources and files above differ). A Collection save was not imported. The port's executable is called
`wordharmony.exe`, the name of the Collection's game on Windows: the installer that puts a native port beside the
original (`docs/STEAM_LAUNCH.md`) must not write it into a Collection folder under that name.

## Mods

The fixes mod, `mods/fixes/mod.json` (`docs/MOD_ROADMAP.md`, "The fixes mod"): `steadyClock` times every frame as one
thirtieth of a second of the game's time instead of the machine's clock, so that the same moves on the same seed play the same
game on every run (`tools/identical/steady.sh` checks it in real time), and `fairFirstWord` clears the time of the last word
that the original never sets, so the first word after starting the program is no longer always quick. The web build loads
mods from `/whpersist/mods` and the site serves the fixes mod on; natively they come from `-modsdir=<dir>` or the
`mods-wordharmony` folder next to the exe, and `-nomods` starts without any. `MOD_IDEAS.md` lists other mods worth making.

## Status (2026-10-26)

The game plays natively and in the browser. Left out: the beta's play time reporter (an online feature) and the crash
reporter's minidump; the level editor, with its HUD, which this build never reaches (taken out, "The unneeded code").
The engine's ACM reader is not needed: the music clips are MP3 inside WAV files, which the
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
`wordharmony` suite, which takes `WH_GAMEDIR` for the Collection check), `.claude/launch.json` (the `wordharmony-web` preview on port 18778) and, from the coordinator,
the store entry in `cmake/ByopopStores.cmake`.
