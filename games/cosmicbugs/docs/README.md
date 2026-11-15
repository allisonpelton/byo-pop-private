# Cosmic Bugs port

A port of Cosmic Bugs (developed by Retro64, published by PopCap Games; © 2007 PopCap Games) to the shared platform
layer: it plays natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of
`CosmicBugs.exe`, found by decompiling it with Ghidra; the game's files (the art banks, the animations and boards in
`animations/`, the sounds and music in `audio/`, and `CosmicBugs.exe`, whose texts are read at run time) come from
the player's own copy and are never in the repository.

Cosmic Bugs is not a SexyAppFramework game. It runs on Retro64's own engine, which Water Bugs shares: plain C over
global state, with no classes, software blitters into a 16-bit back buffer, its own bank, font, FLIC and XML code, and
DirectDraw and BASS underneath. The port keeps that structure. `engines/retro64/` is the engine, translated function by
function (`R64*.cpp`, shared with Water Bugs, `docs/ENGINES.md`), and `src/` is the game on top of it (`CB*.cpp`).
Everything the engine needs from the machine goes through one interface, `r64::SysHost` in `engines/retro64/R64Sys.h`: the files, the screen, the clock, the pointer, the
sounds, the music, links, the save folder and the player's name. `src/CosmicBugsApp.cpp` implements it over the
platform, with the SexyAppFramework hosting the window, the clock, the input and the sound device, and presenting the
800x600 screen the game draws into; `test/gamerun.cpp` implements it over plain memory for the headless runner.
`ENGINE.md` maps the engine, for this port and for Water Bugs.

Everything a player sees plays: the PopCap logo, the title with its demo and tips, the instructions, the options,
the Hall of Fame with the player's name, the credits, and the boards with their lines, fills, bugs, power-ups, lives
and level intros, up to the game's end and the new high score. The settings keys work as in the original (F5 the
sound, F6 the music, F7 the full screen, F8 stretched or centred), and the settings and saves are the original's
files (`cp.cfg`, `cb.dat`).

## The binary

`CosmicBugs.exe` was linked on 2007-11-20 by MSVC 2005 (linker 8.0). It carries no copy protection wrapper (its
sections are `.text`, `.rdata`, `.data` and `.rsrc`) and no RTTI, and it is the full version: the registered flag at
0x460004 is set, so the trial's parts (the time limit, the boards kept for the full version) never ran, and the port
leaves them out (see "Registration and links"). The code is 0x401000-0x457af6, the game's functions and the engine's interleaved, then the
MSVC runtime; BASS is the separate `bass.dll`. The port translates about 560 functions (294 of the game, 269 of the
engine, each with its address in a comment). `docs/symbols/10_engine.jsonl` holds the names given in Ghidra.

The art is two banks, each an XML index (`.bbk`) over a file of pixels (`.dat`): `art8` holds 730 8-bit images (the
fonts and the loading screen) with three palettes, `art` 219 32-bit images that the engine converts to the screen's
16 bits. `animations/` holds 124 FLIC files (bugs.xml names 91 animations, nine of them in files the game does not
ship, which load no frame in the original either; masks are FLICs of their own) and the XML
game files: the menus and screens (`bugs.xml`, which names the boards' `level1.xml` as its next file), and the
localised strings (`local.xml`). `audio/` holds 33 WAV sounds and five MO3 modules.

The game's texts in the code (the default high-score names, the menus' values, the power-ups' names, the requesters)
are read from the player's exe by address: `GT(0x...)` in the code, `src/GameTextTable.inc` (address, length and
hash, made by `tools/make_gametext.py`) and `src/CBGameText.cpp`, which checks each one against the exe at start-up.

## Reproducing the analysis

The export lives in `re/cosmicbugs/` (git-ignored, like everything derived from the game). `tools/ghidra_pass.sh`
applies the port's symbols, fixes what Ghidra gets wrong in this exe (the MSVC floating-point helpers' signatures,
the blit wrappers' tail calls, see `ENGINE.md`) and exports the decompilation again.

```
mkdir -p re/cosmicbugs/bin && cp assets/cosmicbugs/CosmicBugs.exe re/cosmicbugs/bin/
tools/ghidra_import.sh re/cosmicbugs/bin/CosmicBugs.exe cosmicbugs re/cosmicbugs/gh  # the first analysis and export
bash games/cosmicbugs/tools/ghidra_pass.sh                                           # symbols, fixes, the export again
python games/cosmicbugs/tools/decread.py 41ee60                                      # a function, readable
bash games/cosmicbugs/tools/dis.sh 41ee60 41ef00                                     # its disassembly
python games/cosmicbugs/tools/make_gametext.py                                       # GameTextTable.inc after a new GT()
```

## Building and running

```
tools/setup_worktree.sh                                                        # a new worktree: the git-ignored files
python games/cosmicbugs/tools/stage_cosmicbugs_assets.py "<the game's folder>"  # stages assets/cosmicbugs/
tools/build.sh native                                                          # every native target, as below
TARGET=cosmicbugs tools/build.sh native                                        # build/native/cosmicbugs.exe alone
TARGET=cosmicbugs_run tools/build.sh native                                    # the headless runner (the check needs it)
TARGET=cosmicbugs_banktest tools/build.sh native                               # the bank test (the check needs it)
tools/build.sh web                                                             # build/web/cosmicbugs.html, .js, .wasm
```

`tools/check.sh cosmicbugs` needs all three native targets: build them one by one, or run the whole native build.

`cosmicbugs.exe [-gamedir=<folder>] [-userdir=<folder>]` finds `assets/cosmicbugs/` or the retail install by itself
and keeps the player's files in its own folder of the user's application data. The web page imports the game's
files from the install folder or a .zip of it into the browser's storage (`/cbpersist`); `.claude/launch.json`'s
`cosmicbugs-web` serves `build/web` on port 18788 (open `cosmicbugs.html?silent=1` in tests).

## Testing

`tools/check.sh cosmicbugs` (about 20 seconds) loads both art banks, the fonts and every animation the game ships
(`cosmicbugs_banktest`, which fails on any font or shipped animation that does not load), then plays a scripted game headless twice (`cosmicbugs_run`, the game on a clock of its own
with scripted input and PNG screenshots): Start Game, the instructions' Play, a line down the first board, the aim
switched with the right button and a line across. Both runs must reach the expected score and fill and the same
screenshots. Then it runs the windowed exe twice with the same clicks (`-fast -script`); its state after the first
line must be the runner's, and its screen the same both times. The scripts click only the menu, the instructions and
the board, never a link.

The suite also plays the series of games that play out the same in the original (`test/plays`; IDENTICAL_PLAY.md
says how they were compared) and checks them against the frozen runner in `build/baseline-cosmicbugs/`.

Other runs (2026-10-26) toured the options (and their Back), the Hall of Fame, the credits and Quit, and played five
minutes of game time with a line every 2.5 seconds at varying places: several boards finished (their bonuses, the
next boards' intros and tile sets), lives lost down to the game's end, and the Hall of Fame with the new score under
the player's name, saved in `cb.dat`. In the browser (the Browser pane, `?silent=1`) the game imported its files,
showed the title and played a board with the mouse.

Not yet tested: the player requesters (a new name, deleting a player), the level selection, switching the screen
mode natively (F7, F8 and Alt+Enter on the real desktop), and the sound and music by ear: every test runs with the
silent driver.

## Test commands

The standard commands of `docs/TEST_COMMANDS.md` live in `src/CBTestCommands.cpp`, reached only from a test: the exe's
`-script` and live command channel (`CosmicBugsApp::HandleScriptCommand`), the web page's `byopop.test.command` and the
headless runner's `cmd` event. A normal run never calls them, and none but `seed` draws from the game's generator. Each
answers with `STATE {...}`, `MOVE {...}` or `null`, `OK <command>` or `ERROR <command>: <reason>`.

| Command | What it does |
|---|---|
| `state` | The game's state as JSON: `screen` (logo, title, instructions, options, board, highscores, highscore_entry, credits, levelselect, help, change_user, mission_complete), `kind`, `mode` and `difficulty`, `shooter` (manual or auto), `score`, `lives`, `requester`, `fullscreen`, `stretched`, `updates` (the game's step count). On a board also `fill`, `quota`, `shots`, `cells`, `filled`, `playing`, `busy`, `paused`, `won`, `over`, `ship` (x, y, state, across, weapons) and `enemies` with `enemyCount`. |
| `goto <screen>` | `title`, `options`, `help`, `instructions`, `highscores`, `credits`, `levelselect`, `changeuser`; or `game`, `manual` and `auto` to start board 1 in the shooter mode. `easy`, `normal` and `hard` set the difficulty the game file does not offer, and are for a test that wants the setting saved in the profile. |
| `level <n>` | Starts board n of the game (from 1), with its own intro. |
| `unlock all` | Sets the profile's reached board to the last one and saves it. |
| `set <name> <value>` | `score`, `lives`, `reached`, `difficulty` (easy, normal, hard) or `shooter` (manual, auto). |
| `win` / `lose` | On a board being played: fills to the quota for the board's own ending, or ends the game through `Level_GameOver`. |
| `seed <n>` | Seeds the game's generator. |
| `move` | The bot's next step, below. |

`move` answers `MOVE {"kind": ..., "input": [...]}` when there is something to do and `MOVE null` while the game is busy
or over. The input steps are those the drivers send as real events: `{"move": [x, y]}`, `{"click": [x, y]}` (with
`"button": "right"` for the aim switch), `{"press": ...}`/`{"release": ...}`, `{"key": "Escape"}` and `{"wait": n}`
(n game updates). The `kind` says why: `hover` (put the pointer on the board, since a pointer over the bottom panel stops
the play), `aim` (move to the line's start), `turn` (the right click that switches across and down), `shot` (the
click), `launch` (the button a title, options or instructions screen needs), `continue` (a click through a won board's
texts), `wait` (nothing to do for 10 updates) and null. The bot searches the cells where a line fills most of the board without crossing an enemy's
path in the time the line takes (`Safe`), and when it finds none for 30 asks in a row on a board it takes the best gain
anyway, so a board always ends. It plays the game's own rules from the game's own state, one shot at a time, and a mod
that adds a bot can reuse it as it is.

The drivers send `move`'s steps as real mouse and key events and never pick a move themselves. Natively
`games/cosmicbugs/tools/play/drive.py` runs the exe under rundesk with `-commands -realinput` (SDL's own queue, the
path a hand's input takes) through a whole game of each configuration the player has (the shooter manual or auto, in the
full screen the game starts in and in a window): the title, the options, Start Game, the instructions page, Play, the boards
on the bot's shots to the game over, the Hall of Fame and back to the title, with Esc and Enter and a focus loss and regain on
the first board. On the web `games/cosmicbugs/tools/web/play.js` is a page driver (`?player=1&assets=cosmicbugs&silent=1&hold=1
&driver=games/cosmicbugs/tools/web/play.js`, with `&shooter=manual|auto`, `&scale=2` to draw the canvas at twice its size, as
a full screen does, and `&fast=1` for a hidden pane) that does the same through `byopop.test`'s DOM events and its `blur` and
`focus`; its result says how many boards it played and how often the focus had to be given back (`regains`, which counts
a window that lost its focus by itself, as the pane can between two calls of the tool that drives it: the game stands still
then, as the original's does, until the focus is back). The results are in IDENTICAL_PLAY.md, "Phase three".

## Registration and links

The original's data section starts with `gRegistered` at 1 and the trial flag at 0, so the exe the port was made from
already ran as the full version, and taking the trial out changes nothing a player sees (the frozen baseline proves it).
Gone from the port: the registered, trial and trial-time globals and the trial clock on a board (the nag it brought in
the last ten updates), its time drawn at the foot of a board, the 48-board cap of `Level_Won`, the code of the opening,
closing and expiring nag screens (the game file's levels of those types are still loaded, as inert screens nothing goes
to, because loading them draws from the game's generator and leaving them out would move every draw after them; the
opening one is already commented out in the file), the menu item types `buy_now`,
`more_games`, the opening nag's "maybe later" and the closing nag, the readers of the `website.url` and `website3.url`
shortcuts and the page the game asked for at exit (`gUrl`, `GameExit`'s `OpenURL`), the app's `OpenURL` override, and the
text and menu entries for the full version only (`fv`, `fv_type` and `fv_name` are the only ones read). The title's
fourth item, "BUY NOW" in the trial, is the full version's HELP in the original, so no button leaves a gap; its
"MORE GAMES" item is already commented out in the game file. The title's QUIT quits. What stays is the third field of
`cb.dat`, where the original kept the trial's time left, as `gSaveFormatField`: a four-byte placeholder for the file's
format, written back as it was read (3600000 in a new file, as the original's registered copy writes it), so saves move between the
port and the original. Nothing reads it: the clamp the original's loader applied to it (at least a minute left) is
gone, so a file saved by a trial copy that had used up its time keeps that value where the original's registered copy
would have raised it to 60000 at its next save, which no registered save holds. The other trial and registration
leftovers checked in this pass and kept on purpose: the nag screens' levels, which the game file still holds and whose loading draws
from the generator, with the `nag_logo` animation their menus name and the game file's `closing_nag` item type on the
title's QUIT (kind 3, which quits); `gDemo`, the attract mode of the original that nothing ever starts (it is not a
registration path; the readability pass may take it out); and the `fv` text and item attributes the loader skips. The logo screens
show the text "www.retro64.com" on them; it is text, not a button, and nothing in the game follows it.

## Start-up logos

The site's "Always skip start-up logos" (`Byopop::SiteSkipLogos()`, `framework/WebSettings.h`; the exe takes `-skiplogos`)
skips the game's logo screens: the PopCap logo, which is level kind 0xe, shown again for each of the game file's portal
animation files and ending by itself after about two seconds each (the title is up at update 2162 of a normal start). With the
setting on, `GameStep` goes on from a logo screen as Esc on it does, the original's own skip, straight to the main menu
(`gGoTo = 1`), so the title is up from the first update instead of at update 2162. The host's `SkipLogos()`
(engines/retro64/R64Sys.h; the app answers `SiteSkipLogos()`, the headless runner `CB_SKIPLOGOS=1`) is how the game code, which does not see the framework, reads
it. The logos draw no random number, so the game's draws after them are the same with the setting on and off: the runner's seed
series (`CB_WATCH=1`) is identical for a natural start, for Esc at the first logo and for the skip, and begins its first draw at update 2171, 11 and 11. With the setting off nothing changes
(the baseline and the suite hold). The loading screen's bar is not a logo and is untouched; the title's own animation stays.

## Mods

One, `mods/fixes` (`docs/MOD_ROADMAP.md`, "The fixes mod"), with a toggle for each of two fixes: `shieldFlash` starts on and
`walkerTurnMemory` starts off (the coordinator's judgement of 2026-11-11 under the rule that a fix starts on unless the
original's behaviour looks intended and is not widely known as a bug: the up-and-down walkers' quick turns read as a
designed enemy pattern, so the player turns that fix on). The port itself stays the original: `CBFixes.h` holds the toggles, all off until `CosmicBugsApp::Init` has read
the enabled mod's settings (`LoadFixes`), so the headless runner, the suite and the frozen baseline never see them.
`walkerTurnMemory`: a walking bug that comes to a side way turns onto it half the time, and not again for five seconds
(`Walker_TurnAtJunction`); the original keeps that turn's time only for turns off a horizontal way, so a bug going up
or down could turn at every junction. The fix keeps it for both. It changes the enemies' paths and so the random draws
after it, which is what a fix to play is. `shieldFlash`: a beam touching an enemy starts its shield flash at the
enemy's x for its y (`Enemy_Shield`, "sic" in the code) and the next update puts it right, so a frame drawn between
shows it in the wrong place. The fix starts it on the enemy. It changes no draw of the game's random numbers.

### Fix candidates

What was checked for the mod: the waits that only cost the player time, and the bugs of the original that do not break
play. The waits: the logo screens at launch (the title is up about two seconds after the launch, loading included), the
first run's instructions page (left by a click), a won board's fill and bonus texts (about 2.2 seconds, each text
clicked away) and the ship coming in after a life (1.4 seconds while the enemies run, which is the game's own pace);
none is a wait that costs the player time for nothing, so none is a fix. The bugs that are kept in the port and not in the mod, because no player sees them in this game file:

- The easy difficulty's label reads "5s" for its name (`MOD_IDEAS.md`), but the game file has no difficulty item (the
  options list video, LCD mode, sound, music and the shooter mode), so nothing shows it or sets a difficulty: the
  setting is saved in the eleventh player's slot and stays normal.
- The power-ups "explode the floaters" and "explode the walkers" do nothing (their enemy ranges can never hold), and
  `Powerup_Init` never deals them.
- The cannon's look-ahead test has a reset that can never happen (`c < 0 && cols <= c ...`), so near an edge it keeps
  counting; whether that was meant is not clear from the code, so it is left as it is.

The original's own chances are not fixes here: whether a beam an enemy touched gets a rider (the heap's chance,
IDENTICAL_PLAY.md, "Oddities") is already one outcome in the port, so a run plays the same every time.

## Performance (measured 2026-11-08, `tools/perf/perf.py`)

The original and the port back to back on one machine (20 logical cores), in real time, invisibly and silently, windowed
and on a fresh save: the port under rundesk (`build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port`),
the original through origrun from a copy under `build/origrun` (`perf.py orig`), unpatched (the game is the full version
already). Each side runs the same timeline in the game's own updates (its step counter, one a millisecond, read from the
process's memory: 0x460a1c in the original, the port's `cb::gFrame`): the load, the title idle from update 3000 to 12000
(the logo's two seconds come first), Start Game at 13000, Play at 14500, and the first board with no input from 16000 to 46000
(30 s). `--verify` checks that the clicks reach it (the original's current level kind watched in memory: the logo, the
title at update 2162, the instructions page at 13000, a board at 14505; the port's `state`). The machine was not quiet: 2 to 5 other game
processes of other sessions ran at the start and the end of each run (each `result.json` has the count), so the numbers
compare the two sides; the performance pass measures again on a quiet machine. The web build was not timed: the Browser pane is
hidden here and draws about every third of a second, so a time there measures the pane.

**The original's numbers:** it paces its frames with `Present(1)`, which waits for the display's refresh, and the hidden
desktop the tests run it on has none, so the unpatched original spins at 75 to 96 per cent of a core. Both are shown. The paced
rows make its main loop sleep 16 ms a pass (`perf.py orig --paced 16`; a patch of 11 bytes at 0x41f063, where the loop timed
its `Present` call for a value nothing reads, as a 60 Hz screen's wait would), which is what a player sees.

*Loading*, seconds from the process's creation to the first update (the step counter leaves 1: the game loads everything
before its first frame, in one go, so nothing here is sliced and no loading change was needed), and the processor time
spent until then:

| | Launch to the first update | Processor time |
|---|---|---|
| Original | 0.41, 0.42, 0.43, 1.03 (1.58 in the very first run after its copy was made) | 0.36 to 0.52 s |
| Port | 0.57, 0.62, 0.86 | 0.55 to 0.78 s |

The port's log says "Resource Loading Time" 0.5 s of that: the game's own start (`GameStart`: the art banks, the fonts and
the game file, as the original's does). The two are within noise of each other (a cold file cache adds a second to
either), so the port's loading stands as it is.

*Memory and processor time*, the same timeline on both sides (the table is processor time as a share of one core, and as
seconds in the window):

| | Peak working set | Peak private bytes | Title, 9 s | First board, 30 s |
|---|---|---|---|---|
| Original, unpaced (spinning) | 101.0, 111.2 MB | 88.8, 92.3 MB | 95 % (8.5 s), 76 % (6.8 s) | 96 % (28.9 s), 89 % (26.8 s) |
| Original, a pass every 16 ms (60 Hz) | 101.0, 101.0 MB | 88.4, 88.8 MB | 13.9 % (1.25 s), 10.7 % (0.97 s) | 5.6 % (1.69 s), 4.7 % (1.41 s) |
| Port | 129.7, 129.5, 129.4 MB | 130.4, 130.1, 130.2 MB | 19.9 % (2.0 s), 23.4 % (2.1 s), 26.5 % (2.4 s) | 18.4 % (5.5 s), 18.6 % (5.6 s), 18.3 % (5.5 s) |

Each window held 9000 or 30000 updates on both sides in the same time (30.0 s), so the compared work is the same. Read it
plainly: the port loads as fast as the original and, unlike the unpatched original, does not spin; against the original at 60 Hz it
holds about 40 MB more and uses about three times the processor time (a fifth of a core against a twentieth), which is for the
performance pass (the framework's frame loop, the renderer's full-frame upload and the audio mixer are the places to look).
The port's runs also have the test channel's two input files polled.

## The performance pass (2026-11-11)

**What the profile showed.** `tools/perf/cpuprof.py` (every thread's instruction pointer about once a millisecond, weighted by the
cycles it used, with a `CALLERS=1` addition that names the exe function and the DLL on a sample's stack) under rundesk, on the port
in real time over the load and 25 s of the first board: 68 to 77% of the cycles are in ntdll, 10 to 14% in the exe, and the exe's
share is mostly `DDInterface::PrepareTexture` (5 to 7%) and `CosmicBugsWidget::Draw` (2 to 3%). The ntdll samples have d3d11.dll and
the Nvidia driver's nvwgf2um.dll on their stacks and no frame of the game's, and one address, the return from
`ZwWaitForSingleObject`, holds 54% of the cycles on two threads. That address is where a thread that works in short bursts stands
when the sampler looks, not where the cycles were spent (a `SPIN=1` run that looks every few microseconds gave the same picture),
so the profile alone could not say what to change. Timers around the calls (QueryThreadCycleTime before and after, in a scratch
build that was not kept) did. The game ran 60 frames a second (the framework's updates are 100 Hz, but it presents with the
display), and a frame cost the main thread about 4.4M cycles: the 565 to 32-bit widening in `CosmicBugsWidget::Draw` about 1.0M
(the dirty rectangle covers 87% of the screen on a board, so uploading only the changed part would save little), the framework's
`PrepareTexture` 2.0M (0.8M the eight strips' `SDL_UpdateTexture` calls and 1.2M the copy and premultiply of every pixel that
comes before them), `SDL_RenderPresent` 0.37M, the game's own steps and drawing into the 16-bit screen about 0.4M, and the polling below 0.6M. The rest of
the process's cycles, 40 to 45%, are other threads: the Nvidia driver's worker, which the uploads and presents feed, and the audio
mixer. The polling is the two input files that a test channel looks at (`PollLiveCommands`: 0.34M cycles a call, 100 calls a second), an
eighth of the main thread's cycles; a player's run has no such files, so `perf.py` now drives the port by a
`-script`, which the framework reads once.

**What changed.** The widget's screen picture is opaque and uploaded as it is, as Water Bugs' and the Mahjong ports' screens are
(`SetImageMode(false, false)`, `mUploadAsIs`): the framework no longer looks through the 480000 pixels for alpha after each change
or copies and premultiplies them in strips before the upload (every alpha byte is 0xff, where premultiplying changes nothing), and
the 16-bit pixels are widened by a 65536-entry table (256 KB, made once) instead of three shifts and two ORs a pixel. The pixels
that reach the screen are the same.

**Proof that play is unchanged.** `tools/perf/shots.py` runs the frozen exe from before and the new one over one scripted play
(`-fast -drawevery`: the logo, the title, the instructions page, the first board and nine clicks into it) and compares 14
screenshots byte for byte: 14 of 14 the same. `tools/check.sh cosmicbugs` passes with every frozen value as before (the scripted
game and its rerun, the window run twice, the plays against the baseline).

**Numbers.** `tools/perf/alt.sh 3 orig=orig before=port:<the frozen exe> after=port` alternated the original (a pass every 16 ms),
the port before and the port after, three rounds, in real time on the same timeline as above and with 3 to 6 other sessions' games
running; the cycles are QueryThreadCycleTime's, summed over every thread of the process (`perf.py summary`):

| Median of 3 | Original, 60 Hz | Port before | Port after |
|---|---|---|---|
| Title, 9 s: Mcycles a second | 312 | 540 | 400 |
| First board, 30 s: Mcycles a second | 113 | 426 | 276 |
| The board's busiest thread (main), Mcycles a second | 113 | 230 | 166 |
| The board's processor time, share of a core | 5.4% | 20.3% | 13.1% |
| Launch to the first update (s) | 0.75 | 1.06 | 0.99 |
| Peak working set, private bytes (MB) | 85, 72 | 130, 131 | 131, 132 |

A board now takes 35% fewer cycles (the main thread 28% fewer) and the title 26% fewer. The runs: board 426, 419, 440 before and
240, 276, 318 after; the original 107, 113, 114. The original's cycles are the main thread's only: origrun's stand-in mixes no sound
and the original's display is a software blit into DirectDraw, with no graphics driver worker. Its memory is smaller, as in the earlier
measurement; the port's did not change (the table is 0.25 MB).
Loading stands as it was: the port loads in about a second where the original takes 0.75 s, within the machine's noise.

**What is left, and for the coordinator.** The port still takes about 2.4 times the original's cycles on a board, and the part that
is not the main thread (100 Hz of the Nvidia driver's worker, which the whole-screen upload and the present feed) cannot be cut from
the game. The shared costs that remain are the ones in docs/COORDINATION.md: `PrepareTexture` still hands the whole 800x600
texture to SDL for each drawn frame (here the changed part is 87% of it, so a partial upload would cut only a little; a streaming
texture, or an RGB565 one if the renderers took it, would cut the bytes), and a test run's `PollLiveCommands` costs 0.34M cycles
at every 10 ms update to look at a file that does not change (a poll every tenth update, or on a change notification, would spare a
`-fast` test run that cost; a player's run never has it). A frame cap or a lower present rate would cut the driver's share, but it
would change what the player sees, so it would be a mod.

## The processor-time target (2026-11-11, met 2026-11-12)

The user's rule (docs/COORDINATION.md, "The target"): a port's processor time is at most its original's, measured like for like.
With the window's own surface showing the screen (`SurfaceDisplay`, below) Cosmic Bugs is under it: on a board the port at the
original's window size, sound off as origrun's original mixes none, takes 86 Mcycles a second against the original's 110, with the
sound on 92. Before it, the port took 190 to 264, a good part of it the Direct3D driver's worker thread, which no game code of
ours can cut. This section is the final measurement, how it was taken, where the cycles went before and what is left.

**How it is measured.** `tools/perf/alt.sh` alternates the original (its main loop patched to sleep 16 ms a pass, as the wait for a
60 Hz screen would; unpatched it spins and takes a whole core) and the port, one round after another, each run on a performance core
of its own (`tools/pincore.py`: no efficiency core, no hyperthread shared with another pinned run), in real time, windowed, invisibly and
silently under rundesk, with 3 to 7 other sessions' games running. The player's exe with no test machinery: the clicks come from a
`-script` read at the start, no commands channel and no state log run during the stretch. The board window is the 30 s that begin
8 s after the first board has started. Three kinds of port run: the port at the original's window size with the sound off
(`portmute@1`; origrun's stand-in mixes no sound, so the original's cycles are its main thread's alone, and the port is counted the
same way), the same in the default window (`portmute`) and the port at the original's size with the sound on (`port@1`). Two traps
in the measurement, both closed in `perf.py`: an original whose click was lost spends its window on the instructions page (280 to 305
Mc/s, not 110), so every original run watches the game's level kind and the summary skips a run that did not reach the board (one of
seven did not); and a thread's cycles are read per thread (`perf.py threads <run dirs>` prints the table), since the process total hides
that the Nvidia driver's worker is a thread of its own.

**The window each run had** (the Mahjong session's point of 2026-11-12: a window state that differs between the two sides puts a
measurement 20 to 30% off). The original is windowed at 800x600: `cp.cfg` has `fullscreen="0"`, and its window is created and sized
by `AdjustWindowRectEx` for a client area of 800x600 (0x42e530, called with 800 and 600 from 0x42ae60). The port, with the same
`cp.cfg` and a fresh user directory, starts its window at 2x the game's size when it fits the desktop, which it does on rundesk's
3840x2160; `-windowscale=1` (a test option, honoured in test runs only) puts it at 800x600. Each port run's log has the framework's
display line, which `alt.sh` collects: "display: client 800x600 px, logical 800x600, render scale 1, windowed, renderer none" for the
`@1` runs, 14 of 14, and "client 1600x1200 px, ... render scale 2, windowed, renderer none" for the default window, 7 of 7 ("renderer
none": the window's own surface shows the screen, so there is no Direct3D device). Every run, original and port, was on rundesk's
hidden desktop; the original draws through origrun's DirectDraw stand-in, which has no device thread.

| Median, a board (Mcycles a second), 7 rounds | Original, 800x600 | Port, 800x600, sound off | Port, 2x window, sound off | Port, 800x600, sound on |
|---|---|---|---|---|
| All threads | 110 (6 runs) | 86 | 102 | 92 |
| Main thread | 110 | 85 | 101 | 83 |
| SDL audio thread | none | 1.2 | 1.2 | 9.5 |
| Title, all threads | 274 | 191 | 185 | 202 |
| Load, seconds | 0.60 | 0.38 | 0.33 | 0.35 |

The pairs (the original of a round against the port of the same round) fall under 1 in five of six rounds for the port at the original's
size, sound off: 0.80, 0.87, 1.15, 0.77, 0.69 and 0.66, a median of 0.79; the runs of one kind spread by 25% on this machine (the
port's 79 to 120, the original's 92 to 120), so the figure to hold is the median. With the sound on the port also runs its mixer
and synthesizer in the audio thread (8 to 13 Mc/s in the six runs), which the original's number holds none of (origrun's stand-in
mixes nothing); counted apart, the sound-on port is where the muted one is. The default window costs more (102): each pixel is
repeated 2x2 into a surface four times the area and handed to GDI, presumably, which was not split further.

**Where the cycles went before.** In the first alternations of 2026-11-11 and 2026-11-12 the port took 190 to 264 on a board against
the original's 106 to 119, in three threads: the main thread 114 to 163, the Nvidia driver's worker (`nvwgf2um.dll`) 70 to 105, and the
audio thread 1 to 13. The main thread's time was the game's updates (the same code as the original's, 18 to 39 Mc/s) and the host (the
screen compared with its last copy 34, the changed blocks widened 9), and the renderer path, `SDL_UpdateTexture` of the whole image 70 to
78 and `SDL_RenderPresent` 27, from timers around the calls in a scratch build that was not kept, measured under a heavier load than the
later runs (the port's threads were 25 to 45% higher in the first alternation, the original's 12%; I found no cause beyond the
machine's load). A bare SDL presenter with no game in it (800x600, D3D11, vsync, one texture upload and one present a frame) costs
105 to 150 Mc/s in the same threads, the original's whole budget: the driver's worker costs about 70 Mc/s at 60 presents a second whatever
the window's size (its cost is per present, not per pixel or byte: a 1600x1200 picture cost the same as an 800x600 one, and a partial texture upload took
the main thread from 137 to 89 and the total from 221 to 181 and left the worker where it was; that patch stays out of the trunk, and is in
the shared notes folder as `cosmicbugs-partial-texture-upload.patch`). The D3D9 and OpenGL drivers cost the same and SDL's software
renderer does not wait for the display and spins. So nothing in the game could cut the renderer path, and the fix was the surface
display (framework/SurfaceDisplay.h, the Mahjong session's), which copies the changed rectangles into the window's own SDL surface and has
SDL update those rectangles: no renderer, no device, no driver thread.

**What the host does now.** `CosmicBugsApp::CreateDDInterface` makes the `SurfaceDisplay` on the desktop builds (the web build keeps the
framework's display; `SURFACE_DISPLAY=off` in the environment keeps it on the desktop too, for a test that compares the two). The widget
then keeps no image of the screen: `r64::ScreenDiff` (`engines/retro64/R64ScreenDiff`, the engine's one copy) finds what changed, widens just that into the 32-bit buffer the
display shows, and the widget hands the display the buffer and the rectangles with `SetFrame`, everything on the first frame.

The rectangles are not the engine's. The engine's update rectangles (`Sys_AddUpdateRect`) are what its `Dirty_CopyTo` copies from the
back buffer, not everything it draws: sprites, beams, text and fades are drawn straight into the 800x600 screen (`DrawList_Flush`,
`Beam_DrawTo`, `Alpha16MaskFade`, `Font_*`, ...) and some of their pixels lie outside every rectangle the game reported. One column of
one, at x 0, y 490 to 498, changing every frame on the title after the Options screen is left, never was in a rectangle for the 1183
frames that followed; the bounding box of the engine's rectangles, which covers 87% of the screen, had hidden it. So `ScreenDiff`
compares each presented screen with the copy its pixels were last widened from, a row at a time and the rows that differ a 16-pixel
block at a time, 16 bytes at a time, widens just the blocks that differ, joins them into rectangles and keeps the copy up to date. The
pixels equal a full widening of the screen after every update: a scratch check (not kept) compared them on every drawn frame, 0 pixels
differ over 54000 frames of the menu tour (`test/tours/menus.txt`, up to its last board), 27500 frames of the end tour and scripted
90 s plays of random clicks (seeds 1 to 5, with and without `-drawevery`), and the same check with a deliberately skipped rectangle
flagged 8781 of 9000 frames, so it can fail.

**Checked with the surface display.** `SURFACE_VERIFY=1` (the display compares the window's surface with the buffer after every present)
over a whole play: 2699 frames in an 800x600 window and 2687 in 1600x1200, none differing. The pictures of the short scripted play and of
90 s random plays (`tools/perf/shots.py`, seeds 1 to 3) on the surface display against `SURFACE_DISPLAY=off` on the same exe: seed 1 is
the same in all 290 pictures; seeds 2 and 3 fall in two groups of runs, each group holding both displays and identical in itself (the
six runs of seed 3: 0 pixels differ between three of them, off and on among them, and 1 to 5 pictures of 292 between the other three),
and the two groups differ in about 20 pictures, small regions where an enemy is in another animation frame. The game does that between
runs of one exe, on either display (the oddity below), so the display adds no difference. `tools/check.sh --changed` (the build, the web
build, the unit suites, and the Cosmic Bugs and Water Bugs suites; Cosmic Bugs "plays as the baseline", Water Bugs "screens as
frozen") passes. The checks a done game takes on the new display follow (2026-11-12).

**Screen by screen (the display's own part).** The menus tour (`test/tours/menus.txt`, 47 pictures: the title, Options, the hall of
fame, the instructions, the first board and the ways back) was played by the headless runner, by the exe under rundesk (surface display, `-fast`)
and by the web build under Node, at the original's window (`-windowscale=1`) and at the default 2x window, with `games/cosmicbugs/tools/play/webtour.sh`
and `tools/shotdiff`. The exe's pictures equal the runner's in 47 of 47 at both sizes except one pair of the title ship's animation frame, which
differs between any two runs of one exe on either display (the bimodal oddity below); the web build's equal the exe's 47 of 47 at 1x and 46
of 47 at 2x, the one a faint difference in the same animation. The runner is the engine that phase two compared with the original screen by
screen (and its frozen baseline keeps it so), and the new display only changes how that screen reaches the window, so the chain
original, runner, exe, web holds. A fresh run of the original beside it, in Windows Sandbox, was not made: origrun's patch file for this game
(`tools/origrun/patches/cosmicbugs.txt`: the seed and the frame wait) is git-ignored and was not in this worktree, the main checkout or the
backup mirror, so the sandbox run could not start; recreating it is possible from the export but was not needed for the display's part.

**Real input through the window.** `tools/play/drive.py` (posted SDL events through `-realinput`, the bot's moves through `move`, under rundesk,
in real time) now also starts a game in a window (`manual-win1x`: 800x600, `auto-win2x`: the framework's 1600x1200), where the window's own
surface shows the screen, and ends it with Alt+Enter to full screen and back. Six games, each from the title through Options, Start Game, the
instructions, boards until every life was gone, the game over, the hall of fame and back to the title, with the surface display's
`SURFACE_VERIFY=1` on (it compares the window's surface with the screen after every present and aborts at a difference):

| Game | Boards | Last score | Real time | Frames through the surface / the renderer |
|---|---|---|---|---|
| Manual, window from the start, 800x600 | 11 | 31320 | 169 s | 7492 / 8 (compared: none differing) |
| Auto, window from the start, 1600x1200 | 14 | 46969 | 219 s | 3845 / 2 (compared: none differing) |
| Manual, full screen | 15 | 47473 | 230 s | 0 / 11565 |
| Manual, window by Alt+Enter | 15 | 44166 | 218 s | 0 / 5998 |
| Auto, full screen | 18 | 54612 | 268 s | 0 / 5473 |
| Auto, window by Alt+Enter | 12 | 24513 | 202 s | 0 / 4764 |

All six ended on the title with no error. A game that starts in full screen, as the player's does by default, shows its screen through the
display's renderer from the first frame, and a window reached from full screen stays in the renderer mode (the display keeps a renderer once it
has had one, since the renderer's last frame would stay on the window otherwise), so the two surface games are the ones that play through the
window's own surface. The pause and focus checks inside `drive.py` (Esc and Enter on the first board, the focus taken and given back) passed in all six.

**Window checks.** `tools/play/windowcheck.py` sends the window's changes as real events (`resize`, `minimize` and `restore` after `focus
lost` and before `focus gained`, `maximize`, Alt+Enter as keys) with `SURFACE_VERIFY=1`, and after every step clicks the title's Options and Back
at the game's points as the window maps them, takes a screenshot of the game's screen and checks that the updates run. Three runs, all passing:
an 800x600 window taken through 2x, 3x, 1x (each a new surface, `the screen at Nx`), a minimise and restore at 1x, a resize to 1000x750 (no whole
multiple: the renderer, 1000x750), a request for 700x500 (the window stays 800x600, the game's minimum), 800x600 again (the renderer stays),
a minimise and restore in the renderer, a maximise (3840x2089, the renderer's render scale 4 with the picture centred) and its restore, and
1600x1200; the 1600x1200 window taken to full screen (3840x2160, the renderer), minimised and restored there, Alt+Enter back to the window
and to full screen and back again, then to 800x600 and a minimise and restore there; and a 1600x1200 window that only ever has the surface,
minimised and restored at 2x and at 4x (3200x2400), then 1x. Every minimise left the game's updates standing still (the same count three times
over a second) and they ran again after the restore. SURFACE_VERIFY compared 3288, 102 and 493 frames of the surface with the screen in the
three runs and none differed. Not checked: the window's pixels as Windows shows them (the runs are on rundesk's hidden desktop, so nothing
captures a window); what is checked is the window's surface (which the display hands to SDL) and the pointer's mapping to the game.

**What is left, for margin.** The target is met by 22% on the median and not in every pair (one round in six is over), so the host's
own cost matters. The comparison of the screen with its copy reads 1.9 MB a frame (34 Mc/s in the first measurement, memory-bound at
3.4 bytes a cycle, which making it 16 bytes at a time did not change), a third of the original's budget. Ways to cut it, all of which need
the engine or Windows: report every screen write from the engine's blit routines (dozens of them in R64Blit.cpp, and the game's Beam, Obj and
Font draws), which is exact and costs nothing a frame but is a large shared change; hash each block instead of keeping a copy, which reads
half the bytes; or Windows' write watch on the screen buffer, which says which 4 KB pages were written (a page is 2.5 rows, and the engine
rewrites a page for most rectangles, so it saves about a third). None is done; the performance pass can take them if the margin is wanted.

**Oddities noticed, not chased.** In `tools/perf/shots.py`, two runs of the same exe over the same script differ at update 250 (a title
ship's animation frame) in about one run of three, with the same exe on both sides, under load: something on the title runs on a real
clock under `-fast`; and over a 90 s random play the pictures of runs of one exe fall in two groups that differ in about 20 of 290 pictures
(an enemy in another animation frame), the same on either display and with the same seed; `shots.py` and the playback agree everywhere
else. Whether the game's own timers (an animation that reads the time) or the run's start explains it was not found.

## Coverage check (2026-11-11)

The check of `docs/COVERAGE.md`: origrun's `--coverage` recorded which of the original's functions ran in 33 runs of a copy under
`build/origrun`, each invisible and silent under rundesk and on the virtual clock (the main loop's wait patched as for the
performance runs). `games/cosmicbugs/tools/coverage` holds the drivers (`run.sh` for one run, `worlds.sh`, `over.sh`,
`powerups.sh`, their helpers `hover.py` and `addkeys.py`, and the hand-written origrun scripts in `scripts/`). The runs were the
suite's own plays (`a1`, `g1` to `g4`, `k1`); `menus` (the title's items and every screen behind them); `users` (the player list:
add, rename, delete with one player and with two, choose another); `quit` (the closing screen, answered MAYBE LATER, never
BUY NOW); `esc` (Esc on the title, the high scores, the options and the credits); `options` (the options screen's items one
by one, never VIDEO, which changes the display mode); `focus` (focus lost and regained on the title, a board, the pause and a
game over); `w0` to `w8` and `wu0` to `wu8` (from a save with every board unlocked, one run for each world of the select-world
screen with 400 s of `tools/play/gen.py`'s random play, the `wu` runs after twelve clicks on the slider page's up arrow, which
starts the world at its last level, where the enemy and power-up kinds are most; `w4` was lost to an edited script and `wu4`
covers its world); `over1` and `over8` (games that end: a copy patched to start with one life and to always earn a high-score
entry, so the game over, the entry and the Hall of Fame are shown); `pw1` and `pw2` (the original's test keys 0 to 9 and A to D
held in turn, which set a power-up's kind while one is made; neither run made one, since a power-up is chance, so they added
nothing).

What the runs showed about driving the original: a click that arrives in the update of its own mouse move is lost, so every menu
click has the pointer on its button 300 updates earlier (`hover.py`); a font item takes a click only inside its box, which starts
a pixel under its y in the game file and is as tall as the text, so clicks go about 10 pixels under y (the options items, the
instructions' PLAY at 563, the entry's done button at 520); and a held drag of the select-level slider's handle never set the
game's sliding flag (0x6e5bf8), cause not found, so the up arrow's clicks step the level instead.

The game's own code (`game 00401000-004428b0` in `coverage.txt`) has 741 functions. The runs reached 532 of them (494 after the
first ten runs), the port defines 547 and 56 are left out on purpose.

**Reached in the original with no counterpart in the port: 67 at the first look, 0 now.** Most were code the port translated
inline or in another shape, now with its address in the comment above the function that holds it (the list templates' instances
in `CBGame.h`, the emitter and particle set-up, the text maker, the start-up code above `CosmicBugsApp::Init`, the Obj
destructors); comments only. 56 are left out on purpose as `none` lines in `coverage.txt`, each with its reason (the engine is
Water Bugs' too, so these are the reasons of that game's check, found through the two exes' function match: std::list and
container instances, the per-format blitter wrappers, the DirectDraw display layer and the window code, the timer object, `FixPath`,
the trial's text, an empty function the main loop calls). One was missing logic: the Shooter's destructor (0x411d70) and its
list delete (0x411f50). The original's list of shooters runs the destructor on each node when a level is left, destroying
the five Objs the ship holds and deleting the beams still in flight, where the port deleted a shooter as a plain Obj and a beam
in the air when the player left a board was never freed. `Shooter_Destroy` and `List_DeleteNode(Shooter*)` (`CBShooter.cpp`) are
new, and `Beam_Destroy` (which writes through the level's current shooter for a lone beam, as the original does) skips that write
when there is no shooter. Play does not change: `tools/check.sh cosmicbugs` passes unchanged.

**Defined in the port, never reached: 71.** One Sonnet agent read each of the 41 that hold Cosmic Bugs' own logic again against
the export: all match. Looking at them it found two boundary differences among the code around the list that use the x87 compare
Ghidra prints as `a < b != (b == a)` (that is `a <= b`): `Enemy_Step` (0x40a650) leaves when the enemy's y is 592.0 or more (the
port only when it was above 592.0) and `Rider_Update` (0x402a00) goes on only while y is under 592.0 (the port while it was 592.0 or
under). Both are fixed (`CBEnemy.cpp`, `CBBeam.cpp`); it matters only for a float of exactly 592.0, and the suite and the frozen
baseline are unchanged. The other uses of the pattern (0x41ba40, 0x41e2c0, 0x419d20, 0x41c390) were already right. It also
found two stale comments (`Log`'s and the name key's), now corrected: the original's 0x428d60 only formats a string and hands it to
the debugger (the log.txt append is 0x428db0, whose callers are in the untranslated network code), and `MenuItem_TypeKey` clears
one byte after its 16-character buffer, the port four, with the same string. The 71, by why no run reached them:

- Values of the game file's own language that the shipped files do not use (they stay, as the game reads any file): the enemy
  kinds jellyfish (0x4057c0), frog (0x406790), teleporter (0x407e60), crab (0x409e60), lobster (0x40a100) and turtle walkers
  (0x40a1f0), which no shipped level uses; the panel tag (0x417d40); hex colours and swapped colours (`ParseHex4` 0x420110,
  `ParseHex3` 0x4201f0, `SwapColours` 0x420700); the FLIC chunk 0x40c480 (the art's own FLICs never held it).
- Power-ups no run made: the effects of the kinds 0, 3, 4, 5, 7, 8, 9, 10, 12 and 13 (0x418440 to 0x418610, 0x418810, 0x418860,
  the ice shot 0x419fb0 with `Enemy_Freeze` 0x406d70, the sweepers 0x41c390 and 0x41cea0). The kinds table and the test keys
  make kinds 0 to 13, so a run that gets a power-up made and touched by a beam would reach them; one is made when an enemy
  dies while the fill is below the quota (30 %), and the random play found kind 6 (the missile, 0x418720) only. Re-read:
  all match.
- The display-mode change, which the runs left alone because it changes the machine's display mode: `Animation_ReloadAll`
  0x4025c0, `Animation_FreeMasks` 0x401510, `Bank_Reconvert` 0x428160, the fonts' reload 0x40cef0, and `Screen_SetFullscreen`
  0x428ee0 (F7, Alt+Enter and the VIDEO item reach them). The port's host switches the window itself and never loses the
  screen, so these are candidates for taking out.
- Engine code that no game's code calls, which `docs/ENGINES.md` ("What only Cosmic Bugs' copy had") lists for the removal pass: the fonts' unclipped drawing and their texture and mask target (every drawn font has its clip flag set; `Image_BlitUnclipped` 0x4238a0, the unclipped alpha and mask blits 0x423d10, 0x4247e0, 0x424d90, 0x42dd60, `Image_SetMask` 0x4235b0, the rectangle tests 0x42b8d0 and 0x42bbc0), the text input's own surface (0x42c5f0) and `Bank_Reconvert` (0x428160, in the display-mode item above).
- Engine code the runs did not reach that is not on that list, and so stays until Water Bugs' own check agrees: the blitters of the pixel formats the shipped art does not use (0x42fa00 to 0x435fb0 and the image functions that feed them), the 32-bit palette packing 0x4019d0 (the 16-bit one is reached), `Image_ConvertTo` 0x423640, the glyph cutting `Font_StripCut` 0x422cf0 with 0x422c60 and 0x424f90, and the font bounded draw's helpers 0x424430, 0x4249f0 and 0x424a50.
- Code no input reaches: the power-up kinds 14 to 17 (anchor, the two explode-alls, magnet: the kinds table gives 0 to 10, the
  test keys 0 to 13): `Powerup_ExplodeWalkers` 0x4185d0, `Powerup_ExplodeFloaters` 0x418590, the two weapons they hand out
  (the anchor, weapon kind 6: 0x4188b0, 0x419be0; the magnet, kind 5: 0x418770, 0x419a60, 0x419d20) and their cases. The
  demo mode: `gDemo` is never assigned in the original, so its 17 branches in the port are dead. (The name entry's on-screen
  keys, `MenuItem_TypeKey` 0x415580 and `Key_Push` 0x428ad0, were listed here as well; that was a mistake, see "The unneeded
  code": the entry screen has 52 of them and a player can click any, by the code.) `Sys_Sleep` (0x428820, which the patched wait skipped)
  and `gPumpMessages` stayed (it went in the readability pass, below).
- The quit requester's other half (0x416d70), the empty-list clear (0x40f040, whose callers are unwind funclets) and `Log`
  (0x428db0, which only the untranslated network code calls).

These went to the unneeded-code pass, in the next section.

Fixes mod: the review of the mod before this pass ended with the coordinator's ruling that `walkerTurnMemory` starts off (the
walkers' quick turns can read as a designed pattern) and `shieldFlash` stays on; `mods/fixes/mod.json` has both.

## The unneeded code (2026-11-11)

The pass took out what no input of the original reaches and what nothing in the port calls, as the coverage check found
it; `docs/COVERAGE.md` says what the check is. It was two commits, so that the shared engine had its own: the game's
(3180c141) and the shared one (`shared: Retro64: ...`, which `docs/ENGINES.md` lists the functions of). The result is
proved by `tools/check.sh cosmicbugs` (the frozen baseline: "plays as the baseline", unchanged) and by `tools/check.sh
--changed main`, which also runs Water Bugs' suite and both web builds.

Out of the game: the demo mode (`gDemo`, 17 branches: the original never assigns it, so it never plays itself); the
power-up kinds 14 to 17 (an anchor, the two explode-alls and a magnet) with the anchor and the magnet weapons (weapon
kinds 5 and 6), their emitter, sounds, names and the four texts that only they showed (the kinds table gives 0 to 10 and
the test keys 0 to 13, so nothing can make them); the part of the kinds table that only they used; the power-up's
follower (`attached`, which nothing sets: the field stays for the layout); the reload of the art after a change of the
display mode (`Animation_ReloadAll`, `Animation_FreeMasks` and `Animation_Load`'s reload flag: the port's host changes the
window and never loses the screen); `Log`, which did nothing; and four functions nothing called (`Particles_Init`,
`ParticleSystem_Init`, `Obj_ClearChildren`). `coverage.txt` names the original's constructor of the particle systems
(0x40c0d0), which the port's pool replaces.

Out of the shared engine (Water Bugs has the same dead code and runs the same change): the fonts' unclipped drawing and
`Font_SetClipped` (both games set the clip flag on every font they draw), a font's texture and mask target, the text input's
own surface and background (`TextInput_Init` has two parameters fewer), `Bank_Reconvert`, `Xml_Reset`,
`SetScreenLostCallback`, and the port's `Sys_Host` and `DrawList_Count`, with the image functions only those used. The
two reached functions that therefore have no port counterpart, 0x421e50 and 0x428ec0, are `none` lines in `coverage.txt`
(and Water Bugs' 0x422330 and 0x4293a0 in its own).

Kept, with the reason: the name entry's on-screen keys (`MenuItem_TypeKey`, `Key_Push`: the first pass wrote that a
click on one could not happen, but `HighScore_Entered` only runs when Enter is pressed in the name box, and the entry
screen of `bugs.xml` has 52 key items, which the menu code takes as clicks, so a player can click them; read from the code
and the data, since the one run of the original that was to click one never reached the screen: its random play did not
lose its last life); `Sys_Sleep` and
`gPumpMessages` (the coordinator's decision, although nothing sets `gPumpMessages` now that the demo is gone: it was its
only setter; the readability pass then took it out, below); `Screen_SetFullscreen` (the options item and F7 reach the host through it); the effects of the power-up
kinds 0 to 13 and the enemy kinds no shipped level uses (a game file can use them); and the engine blitters of the pixel
formats the shipped art does not use, until Water Bugs' own check agrees. After the pass the port defines 523 of the
original's 741 functions, 50 of them never reached by the runs (71 before; the list is the coverage section's), and 57
are left out on purpose.

## The readability pass (2026-11-11)

The pass of `docs/CODE_STANDARDS.md`. Nothing the port does changed (the proof is at the end), and
`python tools/namecheck.py games/cosmicbugs` finds nothing. It has two exceptions, in `namecheck.txt`: `engine retro64`
(Retro64 is the engine Cosmic Bugs shares with Water Bugs, `engines/retro64`, so it is checked like the rest) and
`allow offset gScreenFormat565` (the pixel format's name: red 5 bits, green 6, blue 5, which is not an offset). These
notes use the new names; the sections above that report a past run (the performance figures, the coverage check, the
unneeded code) keep the names of their time.

**Names.** Cosmic Bugs has no Mac build, so every name comes from how the code uses the thing. Rule 5: a one-letter
name past ten lines of its declaration is a word (`theWidth` and `theHeight` for `w` and `h`, a blitter's `srcX`,
`maskY`; the loops keep `i`, `j`, `k` and the coordinates `x`, `y`). Rule 2: no member or local is called unknown, and
none is named by its offset. Rule 4: the framework's prefixes on every member (`m`), parameter (`the`, `an`), local
(`a`, `an`), global (`g`) and static (`s`), the constants and enum values in UPPER_CASE. Rule 6: the host
(`CosmicBugsApp.cpp`) and the mod and test files carry the port marker, and every function has its address or says it
is the port's.

**The prefixes came from clang-tidy, as in Water Bugs** (its notes, "The readability pass", have the four things to know
about the tool). `tools/readability/tidyrename.py` runs `readability-identifier-naming` once for each unit and applies
every fix file together (6884 places), with its own builder for the new names, since clang-tidy mangles a name that
begins with its prefix's letter (`mask` becomes `mAsk`). It does not reach `offsetof` arguments, members of
anonymous-namespace structs or a member read through a template, nor the `__EMSCRIPTEN__` blocks and
`SiteProfiles.cpp`, which the native build leaves out: those were found by the compiler and by
`tools/check.sh build-web:cosmicbugs`, and fixed by hand. `tools/readability/scoperename.py` renames a local or a
parameter inside one function (it skips strings and comments), for the long-lived one-letter names. A renamed
parameter `aX` to `theX` can shadow an outer `theX`; the object comparison caught one, in the lambda of
`Weapon_TakeSlot` (now `theOther`). The comments that named a renamed identifier were changed with a script that
touches comments only.

**Fields named for what they hold (rule 3).** `tools/splitfields.py cosmicbugs` finds no member declared twice at one
offset, none that nothing writes, and three that nothing reads: the ship's `mSlowMotionTime` (+0x490) and
`mHyperMotionTime` (+0x494), which the two power-ups set and the original reads nowhere (the flags beside them are what
its code tests), and a text's `mDormant` (+0x1c0). They are the original's and stay, with their comments saying that
nothing reads them, as do two objects built and destroyed and never used (a beam's `mHarpoon`, which Water Bugs draws a
tip with, and the ship's `mIdleObj`). The padding that keeps a struct's layout is named for the field it aligns
(`mBonusPadding`, `mItemsPadding`, `mDragPadding`, `mLaunchSpeedPadding`, `mInputPadding`, `mWingsPadding`).

**Numbers named (rule 7).** Where one value meant one thing in many places it has a name, the same names as Water Bugs
where the thing is the same:

| names | what they are | uses |
|---|---|---|
| `SCREEN_WIDTH`, `SCREEN_HEIGHT`, `SCREEN_CENTRE_X`, `SCREEN_CENTRE_Y` (`engines/retro64/R64.h`) | the 800 by 600 screen and its middle | 141, in the game and the host |
| `CELL_SIZE`, `CELL_HALF`, `GRID_COLUMNS`, `GRID_ROWS`, `BOARD_WIDTH`, `BOARD_HEIGHT` (`CBGame.h`) | the 16-pixel cell, half of it, the 50 by 37 board and its 800 by 592 pixels | 259: every position over 16 is a cell index |
| `CellKind`: `CELL_OPEN`, `CELL_BEAM`, `CELL_LINE`, `CELL_EDGE`, `CELL_FILLED`, `CELL_SOLID`; `CellState`: `CELL_STILL`, `CELL_CAPTURING`, `CELL_WAITING`, `CELL_IN_WAVE` | what a cell is and where it is in its life | 181 |
| `EnemyKind` (twenty names: `ENEMY_FROG`, `ENEMY_BUMP_ROLLER`, `ENEMY_CANNON_BALL`...) and `EnemyState`: `ENEMY_APPEARING`, `ENEMY_DYING`, `ENEMY_FADING`, `ENEMY_ALIVE` | what an enemy is and where it is in its life | 108 |
| `ShipState`: `SHIP_HIT`, `SHIP_IDLE`, `SHIP_CONTROLLED`, `SHIP_COMING_IN`, `SHIP_FIRING`, `SHIP_DYING` | the player's ship | 28 |
| `PowerupKind` (thirteen names), `POWERUP_FALLING`, `WeaponKind`: `WEAPON_MINE`, `WEAPON_MISSILE`, `WEAPON_ICE_SHOT`, `WEAPON_TIME_BOMB`, and `WeaponState`: `WEAPON_ORBITING`, `WEAPON_LAUNCHED` | what a power-up and a weapon are | 85 |
| `LevelKind` (seventeen screens, from `LEVEL_BOARD` to `LEVEL_CHANGE_USER`), `LevelState`, `ItemKind` (what a menu item does: `ITEM_START` to `ITEM_RESUME_NO`) and `RequesterKind` (`REQUESTER_DELETE_PLAYER` to `REQUESTER_RESUME`) | the screens, their states, the menu's items and its requesters | 268 |
| `TextKind`, `TextState`, `TEXT_IDLE_FOREVER`, `TEXT_EMPTY_PROFILE` | what a text shows and its life; its idle time of 0x986fc8 ms (about three hours: it stays until it is clicked away); the text id of an unused profile slot's name | 86 |
| `EmitterKind`: `EMITTER_ANGULAR`, `EMITTER_SCATTERED`, `EMITTER_TRAILING` | how an emitter places its particles | 7 |

The kinds and states were found from the AST, not by reading: `tools/readability/kindscan.py` (clang's
`-ast-dump=json`) lists every integer literal that is compared with, assigned to or switched on a member named `mKind`
or `mState`, with the struct the member belongs to, which a text search cannot tell (a cell's `mKind` 4 is not an
enemy's). A rules file, `kinds.rules`, maps each (struct, member, value) to a name, and the same tool rewrites the
literals; the arguments that take a kind (`Cell_StartCapture`'s) are listed in the file too. `CBTestCommands.cpp`,
whose AST is larger than the tool takes, was done by hand. The kinds' meanings come from how the code uses them and from
the comments the earlier passes wrote over the structs; two are judgements. `CELL_LINE` is what a beam leaves (the
walkers follow it and the bump rollers push it), and a blast's rim is made of lines too; `CELL_BEAM` is a cell a growing
beam crosses, which the capture turns into a line. The power-up kinds are named for the function each calls; the test
keys `0` to `9` and `A` to `D` make kinds 0 to 13, and any other number is the fast shot. The screen's size appears
twice in `Obj_FromXml` and the text and emitter loaders as `x * 800 / 800`: the original scales the authored position by
the screen's size over itself, and the port keeps both factors, now named. The shooter's range test (`16 < x < 784`,
`16 < y < 584`) is the screen less a cell, not the board (592 is the board's bottom).

What stays a number, each where its comment or the line makes it plain: the board's mirror (`528 - y`, the board's height
less four rows), a level file's 32 rows (the boards that have a background image), the level numbers that choose a
board's background and song, the draw kinds, the particle systems' colours and counts, times in
milliseconds, speeds, scores and the points of a chain. They are the original's own tuning and appear once or twice
each; naming them would be a list of constants nobody reads twice.

**One shared change.** `gPumpMessages` is out of the engine (`shared: Retro64: gPumpMessages out`, 838ee5e4): nothing
has set it since the demo mode went in the unneeded-code pass, and `PumpMessages` now returns at once while the window
is inactive, which is what the flag was for. `docs/ENGINES.md` says so, and `tools/check.sh --changed main` runs Water
Bugs' suite with it. Water Bugs' own notes still name the global in the sections about its past runs.

**Lines.** None is over 120 columns (tabs of 4). Code lines were wrapped by `clang-format` limited to the long lines
(`--lines`), with a style written to match the code's (Allman braces, tabs, a break after a comma, no reflow of
comments); a script wrapped the comments, and a trailing comment that did not fit went above its line. The object
comparison proves that no wrap changed anything.

**Comments.** They name the kinds by their enums (`ENEMY_LASER_SHOT` for "kind 0xd") and describe the code as it is: no
stage or phase labels (the `stage` check of `namecheck` finds none), and nothing about what a line used to say.

**The proof.** `tools/objsnap.py` took the game's objects before the first change (31 objects, 711 functions) and
compared them after every part: each function's instructions with the names taken out, and the data bytes, which hold
every string, log line and table. All of them are identical except two things. The unit `CBShooter.cpp` is one byte
shorter, in `.llvm_addrsig`, the table of the symbols whose address is taken (linker metadata, no instruction and no
data; it lists symbols by their index in the object's symbol table, and the renames changed that table's order). The
engine's `R64Sys.cpp` differs because `gPumpMessages` went (the shared change above). The comparison caught one slip on
the way, the lambda parameter that shadowed an outer name. `tools/check.sh cosmicbugs` (the suite against the frozen
baseline) passes, and so do `tools/check.sh build` (every target and tool), `build-web:cosmicbugs` (the native build
compiles none of the `__EMSCRIPTEN__` blocks nor `SiteProfiles.cpp`) and `tools/check.sh --changed main`.

## The autosplit block (2026-11-12)

Cosmic Bugs has a speedrun.com board (Any%: the clock starts at the click on Play on the instructions screen and stops when the
"Congratulations! Mission complete" screen comes up), so the port writes the shared autosplit block the site's Splits window reads
(`platform/AutosplitBlock.h`, layout 2; `docs/WEB_PLAYER.md`, "One design for every port"). The block's game id is `CBDX`. The observer
is `src/LiveSplitObserver.cpp`, in the exe's host only (the game library and the headless runner do not have it; Water Bugs has no
board and uses none): `CosmicBugsApp::RunFrame` calls `LiveSplitObserver::AfterUpdate` after each pass of the game's loop and the app's
destructor calls `AppDestroyed` for the last write. It reads the game and writes nothing of it: until something asks for the block it only
tests a flag (the mod list is looked at every 64 updates). The page asks by calling `byopop_livesplit_block` (exported in the web build,
`byopop_web_game(... EXPORTS _byopop_livesplit_block)`); natively the `livesplit` feature of an enabled mod does
(`mods/livesplit/mod.json`, a manifest only). `-livesplitlog=<file>` writes one line for each write that changes what a splitter acts on,
and the events themselves go to the log as `livesplit: board 1 GO at tick 321 (board 1, difficulty 0, shooter 1)` and the like.

**What it reads.** Two globals the port added in `CBMain.cpp` (`gGameSerial`, `gGameFromInstructions`, set in `ChangeScreen`; nothing in
the game reads them, as the inert state of the standing rule): a game is the run of boards entered from a non-board screen, and the
mission-complete screen keeps the serial of the game it ends. `mode` is always 0 (the one game mode), `skill` is `gDifficulty` (-1 easy,
0 normal, 1 hard; the game has no screen that changes it, the profile's value counts), `level` is the board's `mNumber - 1` and
`displayLevel` the number on screen (`mNumber`, 1 to 577), `state` is `Level::mState`, `score` and `displayScore` are `gScore`, `tick` is
`mUpdateCount`, and `progress` is -1 (the game has no measure between the clears). The shooter (`gAutoFire`: 0 manual, 1 auto) is in the
block's own words. A paused board has `AUTOSPLIT_FLAG_PAUSED`; a game that starts from a screen other than the instructions page has
`AUTOSPLIT_FLAG_RESUMED`, and `AUTOSPLIT_FLAG_VANILLA` is on while the score key is empty (no mod changes the score).

**Events.**
- *GO*: the update in which a board is entered from the instructions page (`LEVEL_HELP2`): the click on Play is consumed in that update,
  `ChangeScreen` makes the board current in it, and the first frame of the board is drawn right after, so GO is the speedrun's start with
  no update between. The instructions page reached from the title's Instructions button has the same Play button and raises GO too.
  A board entered any other way (Continue saved game's Yes, a board chosen on the level selection, the `level` test command from any
  screen) raises no GO and is flagged RESUMED (the log says "starts from another screen than the instructions page: GO not counted").
- *Level clear*: the board's state becomes `LEVEL_WON` (state 3, `Level_Won`: the fill reaches its quota, the profile's best level is
  saved). The board's own texts and fills follow and the player clicks through them before the next board starts, so a level clear
  precedes the next board by the time the player takes (221 updates in the scripted mission test, which clicks every 0.6 s).
- *Game over*: kind 0 (out of lives) when `Level::mGameOver` is set (`Level_GameOver`, from the shooter's last life): the board is in
  state 4, "Game Over" is drawn. Kind 1 (left) when the game is left for another screen before it ends (Esc, Esc to the title); the
  fields of the game's last update are published with it, then the block shows no board. Kinds 2 and 3 are not used.
- *Win*: the mission-complete screen (`LEVEL_MISSION_COMPLETE`, state 5) becomes the current level, the update after the last board's
  texts are clicked away. It is the screen the rule names (the frame it is first drawn in). A mission has 577 boards (one hand-made and
  48 random boards for each of 12 steps; `level 577` reaches the last), so a won mission is one GO, 577 level clears and a win.
- A board that is left for the title is not a game over of the player's: it is kind 1 and the block shows no board afterwards.

**Proof.** `games/cosmicbugs/tools/livesplit/run_tests.sh <out dir>` (under rundesk, about 10 minutes): games played through real input
at `-fast` (`games.py`: a new game with two boards won and the third lost, kind 0; board 577 won to the mission-complete screen; a game left with
Esc and a second one lost, kinds 1 and 0; a saved game continued, RESUMED with no GO; a board chosen on the level selection, RESUMED with
no GO) and the inert proof: one scripted game (seeded, clicks at fixed updates, a win and a loss) three times, with the block on and with
no mod twice, whose state lines are byte-identical. The block logs of the `game` and `mission` runs went to the site session.

## The PopCap Game Collection version (2026-11-11)

The port's files come from a PopCap Game Collection install (the staging script's default folder is the Collection's
`Cosmic Bugs`), so the check against the disc is a comparison and one port run, as Ning Po's and Water Bugs' were
(`docs/COORDINATION.md`). `unshield` read the group `CosmicBugsComponent` from the user's disc image, mounted read-only and
dismounted afterwards: 178 files, 18.2 MB, into a folder under `build/` that was deleted once this was written. The layout
is the install's: `CosmicBugs.exe`, the two art banks, `animations/`, `audio/`, `bass.dll`, a licence and a readme.

**Every file the port reads is the same.** All 171 files the staging script copies (the exe, the banks, the 124 animation
files and the XML game files, the sounds and the five modules) are the same byte for byte as the staged copy's, the exe
among them, so no text and no table that `CBGameText.cpp` reads from it moves, and the start-up check of every text passes
on it. The seven other files are the ones the script leaves out: `bass.dll`, the licence, the readme, the launcher's
`properties/partner.xml` and its `.sig` (the product's name, the registry key `PopCap\CosmicBugs` and a window of 640 by
480, which the game never reads: its exe has no string "partner" or "properties"), and the authors' two tools in
`animations/`, `encrypto.exe` and `i.bat` (which names the three XML files it was run on). The tools are never run and
never staged.

**The port on the Collection's files** (`cosmicbugs_run` under rundesk, the suite's scripted game: Start Game, the
instructions' Play, a line down the first board, the aim switched with the right button and a line across, 28 seconds
of game time). It gives the same two state lines as the run on the staged copy (score 526, filled 426 of 85% at 17 s;
score 729, filled 629 of 85% at 28 s) and the same two screenshots, byte for byte. No change to the port was needed, and
the frozen baseline is unchanged.

## Where the work was hard

Time. The original steps the game once for every millisecond that passed, then draws once (GameFrame, 0x41ee60
split into GameStart, GameFrame and GameEnd), so the port runs one pass of that loop per framework update and lets
the game do its own stepping. The framework's `-fast` clock does not advance by exactly 10 ms on every update (the
updates it draws do not wait), so a windowed run and a headless run with the same input end in the same game state
but not with the same animation phases; the check compares the state between them and the pixels only between runs
of the same kind.

The draw order. Sprites of equal depth are drawn in the order MSVC 2005's qsort leaves them, after a comb sort, with
a comparator that never answers 0; the port translates the runtime's qsort from the exe (CrtQsort) so the order is
the original's.

The mouse. The game samples the buttons once per pass, after all waiting messages, so a press and release between
two passes is lost, in the original as in the port. Scripts hold a button for a few updates; the framework's
one-update `click` is too short.

The web. The game's settings default to the full screen, and SDL's request for it from outside a click left the
canvas black; on the web the game's screen mode now leaves the window alone, and the page's Fullscreen button owns
the browser's full screen.

## Changes outside the Cosmic Bugs folders

`CMakeLists.txt` (the `BUILD_COSMICBUGS` option), `tools/check.sh` (the `cosmicbugs` suite) and `.claude/launch.json`
(the `cosmicbugs-web` preview on port 18788). The web page also needs `BYOPOP_BUY_cosmicbugs` in
`cmake/ByopopStores.cmake`, which the coordinator keeps. The finishing pass added two shared commits: `cmake/ByopopWeb.cmake`
takes Cosmic Bugs off `BYOPOP_WEB_NO_MODS` (so its page installs the fixes mod), and `web/testkit.js` had a backspace character
where its ANSWER pattern meant a word boundary, so a command answered `MOVE null` was never matched and
`byopop.test.command` timed out after every busy move.
