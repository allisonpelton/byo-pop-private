# Cosmic Bugs port

A port of Cosmic Bugs (developed by Retro64, published by PopCap Games; © 2007 PopCap Games) to the shared platform
layer: it plays natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of
`CosmicBugs.exe`, found by decompiling it with Ghidra; the game's files (the art banks, the animations and boards in
`animations/`, the sounds and music in `audio/`, and `CosmicBugs.exe`, whose texts are read at run time) come from
the player's own copy and are never in the repository.

Cosmic Bugs is not a SexyAppFramework game. It runs on Retro64's own engine, which Water Bugs shares: plain C over
global state, with no classes, software blitters into a 16-bit back buffer, its own bank, font, FLIC and XML code, and
DirectDraw and BASS underneath. The port keeps that structure. `engine/` is the engine, translated function by
function (`R64*.cpp`), and `src/` is the game on top of it (`CB*.cpp`). Everything the engine needs from the machine
goes through one interface, `r64::SysHost` in `engine/R64Sys.h`: the files, the screen, the clock, the pointer, the
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
(engine/R64Sys.h; the app answers `SiteSkipLogos()`, the headless runner `CB_SKIPLOGOS=1`) is how the game code, which does not see the framework, reads
it. The logos draw no random number, so the game's draws after them are the same with the setting on and off: the runner's seed
series (`CB_WATCH=1`) is identical for a natural start, for Esc at the first logo and for the skip, and begins its first draw at update 2171, 11 and 11. With the setting off nothing changes
(the baseline and the suite hold). The loading screen's bar is not a logo and is untouched; the title's own animation stays.

## Mods

One, `mods/fixes` (`docs/MOD_ROADMAP.md`, "The fixes mod"), with a toggle for each of two fixes, both on in the site's
copy. The port itself stays the original: `CBFixes.h` holds the toggles, all off until `CosmicBugsApp::Init` has read
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
