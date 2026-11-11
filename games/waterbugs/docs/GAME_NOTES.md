# Water Bugs: notes of the finishing pass (2026-11-08)

What the finishing pass added to the port: the standard test commands and the bot behind `move`, the drivers that play a whole
game of every mode by real input (natively and in the page), the fixes mod, the registration and outbound buttons taken out,
and the performance numbers of the original and the port back to back. How the engine and the game code work is in `README.md`
and `ENGINE.md`, the differences from the exe in `DEVIATIONS.md`, and the comparison with the running original in
`IDENTICAL_PLAY.md`.

## The game's screens, as the test commands name them

The engine moves between "levels" of a kind (`Level::kind`): 0 a board, or a world's story screen when it has no cells
(`totalCells` is 0), 1 the main menu, 2 the name entry, 3 the high scores, 4 the credits, 5 and 6 the world map, 7 the Options,
8 to 10 the nag screens, 0xb the results of a won level, 0xc and 0xd the help, 0xe a logo (about 2.15 s, and it ends by itself),
0x10 the player list. `state` calls them `board`, `story`, `menu`, `nameentry`, `highscores`, `credits`, `map`, `map2`,
`options`, `nag`, `complete`, `help`, `help2`, `logo`, `changeuser` and `loading` (before the first screen).

The levels of a world are a set (`LevelSet`); `gLevelInSet` counts the levels of the set that were left behind, so board n is the
state `gLevelInSet = n - 1` (the original's Start after n boards goes to board n + 1). The game reads its game file again
(`FreeAll`, `LoadGameFile`) when a screen is left and no level of the next kind remains; that frees every level set, so a pointer
into one is dangling after it.

## The standard test commands (`src/testhooks/TestCommands.cpp`)

The commands of `docs/TEST_COMMANDS.md`, reached only from a test (`-script`, `-commands`, the web page's `byopop.test.command`).
A command that is never given changes nothing and draws no random number. Each answers on the log with one line, `STATE {...}`,
`MOVE {...}` or `MOVE null`, `GRID`, `OK <command>` or `ERROR <command>: <reason>`, and the driver reads that line
(`INFO: ` in front of it in the native log; the test kit's answer pattern takes both).

| Command | Does |
|---|---|
| `state` | one JSON line (the keys are below) |
| `goto <screen>` | `menu`, `options`, `help`, `help2`, `highscores`, `credits`, `map`, `changeuser`, `logo`, through the game's own screen change |
| `level <n>` | board n of the game, n from 1 (0 too is the first), as Start reaches it: from a menu or the map it sets the lives to 6 and the score to 0; from a board it goes through the main menu first (as Esc's quit does), so that the boards are read afresh. The story screen of a world's first board comes first, its button is `move`'s next click. `ERROR` when the game has no such board |
| `unlock all` | every level of the campaign counts as reached in the current profile |
| `set <name> <v>` | `score`, `lives`, `difficulty` (-1, 0, 1), `shootermode` (0 manual, 1 auto) and `reached` (the levels finished) |
| `win`, `lose` | the board ends as the game ends it: `Level_Won` with every cell filled, or `Level_GameOver` with no life left. `ERROR` when no board is being played |
| `seed <n>` | seeds the game's generator (`SeedRand`) and the C library's (`CrtSrand`) |
| `move` | the bot's next move as `MOVE {"kind", "input": [...], "updates": n, ...}` in game coordinates (`{"move":[x,y]}`, `{"press":[x,y],"button":...}`, `{"release":...}`, `{"wait":n}`), or `MOVE null` when there is none |
| `grid` | the board as text, one line per row: `.` open, `+` open and marked by a pending beam, `#` not open, `e` a bug |

**`state`.** Always: `screen`, `mode` (always `campaign`), `update` (the framework's), `frame` (the game's steps, ten to an
update), `score`, `lives`, `level` (the board's number, -1 off a board), `reached` (`levelReached` of the player), `difficulty`,
`shootermode`, `rand` (the generator's state), `over` (the game has ended: the name entry, the high scores or the results after a
board was up) and `busy` (the bot has nothing to press and waits). On a board: `percent` and `quota` (the fill and the fill to win),
`board` (`cols`, `rows`, `state`, `gameover`, `shots`, `lines`, `cells`, `filled`, `weapons`, `texts`, and `ship`: `x`, `y`,
`state`, `across`, `noauto`, `speed`), `mouse` and `pointer`, `powerups` as `[x, y, state]` and `enemies` as `[x, y, vx, vy, kind,
state]`. On every other screen, and on a board with a requester up: `dialog` (a requester is up) and `items` as `[kind, x, y,
name, lockedLevel]`, with `sets` as `[start, end]` for each world.

**`move`.** On a board the bot looks at every open cell and both directions of the beam through it and chooses the shot that
fills the most, weighing how long the two beams take to stop and how early each bug could reach a beam that is still growing;
a line that cost a life is banned for the rest of the board. It moves the ship to the cell (`move`), turns it with a right
click when it faces the wrong way, fires with a click, and takes up a falling power-up in the ship's way. The kept aim stops it
from sending the ship back and forth between two nearly equal shots. Off a board it presses what gets the game going: the main
menu's Start (and the last button of the "resume your previous game?" requester, since the first is the yes), a story screen's
button, the furthest open world on the map, the results' button, a game over's click, and nothing on a logo (those end by
themselves, `busy` is true). It draws no random number and changes nothing; `state` asks it for `busy` without moving it.

## Fix candidates

What the original does that the fixes mod (`mods/fixes`, below) mends, and what it left alone.

- **Made: the clock of the game steps (`steadyClock`).** The game steps through the milliseconds the machine's clock says have
  passed since the last frame (`GameFrame` in `WBStart.cpp`, capped at 0x200 ms) and reads the mouse once a frame, so a frame of
  9 or 11 ms instead of 10 moves the ship's aim by a step against the bugs, and the same input does not play the same game twice.
  With the fix the game steps 10 ms for each update of the framework (a `-fast` run is steady already), so a seed and a list of
  moves play the same game on every run. The result of `determinism.sh` is below.
- **Made: the Easy label (`easyLabel`).** The Options screen's difficulty button writes `5s` for the easiest value where its
  name belongs: the original's format string has the text `5s` in place of `%s`. With the fix it writes the name as it does for
  the other two. Only the label changes.
- **Not a fix: the logos.** They last about 2.15 s together and end by themselves (`Level_Update` in `WBLevel.cpp`, which
  counts them in `gLogoCount`). They are not a wait on loading, and Esc leaves them: the update's rule for a screen that is
  not a board is to go to the main menu (read from the code, not played). The port loads at the machine's speed behind them.
  The site's "Always skip start-up logos" skips them (below).
- **Not a fix: the ship's appearance.** After a board starts, and after a life is lost, the ship comes in over 70 updates and
  fires nothing meanwhile. It is an animation of the game, not a wait on loading, and stays.

## The site's "Always skip start-up logos"

The setting (`Byopop::SiteSkipLogos()`, `framework/WebSettings.h`; on the site the page's `skipLogos`, natively `-skiplogos`)
skips the game's one start-up logo screen, the level of kind 0xe in front of the main menu (the PopCap logo, about 2.15 s; the
game finds no `portal*.tga` files, so `PortalCount()` is 0 and a second logo never follows it). `GameStep` in `WBMain.cpp` does
for it exactly what Esc on the first logo does, the original's own skip: `gGoTo = 1`, `GoToScreen()`, `Screen_Redraw()`, in the
same update that would have run the logo. The host answers through `SysHost::SkipLogos()` (`Sys_SkipLogos()` for the engine and
game code), which `WaterBugsApp` implements over the site's call. The skip is for the start-up only: a flag
(`gStartupLogosOver`) is set once a screen that is not a logo has been up, so the logo the menu's hidden item can ask for later
plays as the game has it. The setting off changes nothing (`tools/check.sh waterbugs`'s frozen screens and logs are the same).

The logo draws no random number (the game's generator stays where `seed` put it for the whole 2 s of the logo, and the menu's own
animations begin to draw once the menu is up, as they do after the logo), so the skip changes no draw. Natively with
`-skiplogos` the first `state` (update 4) reads `menu` where a run without it reads `logo` until update 215, and the menu's state is
the same as the one reached without the setting except for the update, the frame and the generator's state (the menu's animations
have been drawing since the menu came up). On the page, with the setting on in the site's settings (the full page, with the
game's files imported; the player-only test page never sets it) the main menu is up 1.5 s after Play, and with it off the PopCap
logo is.

## The fixes mod (`mods/fixes`)

`mod.json` with the feature `fixes` and one bool setting for each fix, `steadyClock` and `easyLabel`, both on by default and
applied at a restart (the game reads them once, at startup). `Sexy::FixOn(name)` in the host and `Sys_FixOn` in the game's code
read them; without the mod every fix is off and the game is the original. The mod changes neither the rules, the scores nor the
saved games. Natively the mod is a folder in `mods-waterbugs` (or in a `-modsdir`); the page ships it beside the game and starts
it with the fixes on (Water Bugs is out of `BYOPOP_WEB_NO_MODS` in `cmake/ByopopWeb.cmake`, shared commit `29f77e35`, and the
page's `modsIntro` says so). The page's status line reads "1 mod active" and a whole game played to its end on it.

`tools/realplay/determinism.sh [runs]` plays one short fixed game (the story button, then three shots at fixed updates) in real
time N times with the mod off and N times with it on, and compares the game's state at four updates. With the mod off the five runs were three different games (the same moves, the same score of 598 and the same fill of 52 %,
but other random states, so other bug positions); with it on all five were the same game, and it is one of the three the mod off
plays, so the fix removes the machine's timing and changes nothing else.

## The drivers

All of them are code that plays; no session plays the game (`docs/COORDINATION.md`). Every native one goes through
`build/native/rundesk.exe` (a private desktop, no sound, below normal priority).

- `tools/realplay/drive.py` is the real-input driver: it starts the exe under rundesk with `-commands` and `-realinput`, asks
  `move` for the next step and sends it as real SDL events (`click game X Y`, `down`, `up`, `move`, `keydown`, `keyup`,
  `focus lost`, `focus gained`, `minimize`, `restore`), and reads `state` to know when the game is over. `tools/playbymove/
  playbymove.py` is the shared loop it is built on; Water Bugs passes `--nulls 40 --stuck 200` because the ship's appearance
  gives about 70 updates of `MOVE null`.
- `tools/realplay/modes.sh` plays a whole game of each of the six modes (shooter auto or manual, difficulty 0, 1 and -1) from the
  launch: the logos, the main menu, the Options screen's SHOOTER MODE and DIFFICULTY set by real clicks (the difficulty button
  cycles 0, 1, -1), back to the menu, then the bot from Start through the story screens and boards until the game is over, and
  prints one line per mode with the settings read back from `state`.
- `tools/realplay/screens.sh` plays the same game from the fourth board with the window changed on the way: the plain run, the
  windowed run (Alt+Enter at the main menu), the toggle run (Alt+Enter twice on a board), a focus loss and regain on a board
  and a minimize and restore; each ends in a game over.
- `tools/web/modes.js` is the same for the page (`?player=1&silent=1&assets=1&fast=1&driver=games/waterbugs/tools/web/modes.js
  &shooter=0&diff=-1`, optionally `&focus=1` for a focus loss and regain on board 4, `&fullscreen=1` for Alt+Enter twice at the
  main menu, `&scale=2` for the scaled canvas), clicking and pressing as DOM events; `?driver=move` plays one mode with the
  built-in loop.
- `tools/profile/perf.py` takes the numbers of the performance section.

## Focus and the full screen

The window starts in full screen (the original's default, `fullscreen` in `cp.cfg`); Alt+Enter toggles it. The original pauses
when it loses the focus (`PumpMessages` returns 0 while the game is not the active application) and goes on when it gets it
back. The port's `IsActive()` returned true for a test run, so a focus loss changed nothing in a driver's run (the same for
Cosmic Bugs). It now follows the window, as the original does, except in a test run without `-realinput`, where nothing
changes the window (`docs/REAL_INPUT.md`): a real-input run's `focus lost` pauses the game and `focus gained` goes on, and the
page's blur and focus do the same. The game steps with the last frame's time on the first frame after it goes on
(`GameFrame` caps an elapsed time at 0x200 ms and otherwise reuses the last pass's), so a run with a pause is not step for step
the game of a run without one: `screens.sh` reports the windowed run as the same game as the plain one (the display changed
at the menu) and the others as other games, all ending in a game over.

On the web the canvas cannot go full screen in the hidden Browser pane (the page refuses it). The driver sends Alt+Enter at the
main menu twice and the game goes on in its window; the whole games of every mode are played with the page's normal scale and
once with `?scale=2` (the 800 x 600 canvas doubled by the page), which stands in for a bigger screen.

## Registration and the buttons that lead out of the game (taken out 2026-11-08)

The port was the registered game already (`gRegistered` is 1 in the player's copy, so the shareware branches never ran); what
was still translated is gone now, and the baseline's logs and screenshots are unchanged (`f6c9025b`).

- The trial's clock: `gTrialLeft` and its decrement on a board, the "expiring nag" it started, `gTrial`, `Level_DrawTrialTime`
  and its text (id 0x458e3c) and the trial's cap on the levels reached (0x30). The save file still holds the clock's 4 bytes,
  written as the registered game leaves them (3600000) and read and dropped, so a file of the original and a file of the port
  stay interchangeable.
- The registered flag itself (`gRegistered`) and every branch on it: the texts marked `fv` (the trial's) are left out of the
  menu files as the registered game does, the `fv_type` and `fv_name` of a menu item are taken, a power-up kind is no longer
  redrawn for an unregistered game, and the closing nag Esc on the main menu showed to an unregistered game is gone (Esc leaves
  the game, as it does for the registered one).
- The buttons: "buy now" (a menu item kind 0, which read `website3.url`) and the website button (kind 6, `website.url`), the
  shortcut reader behind them (`ReadShortcut`), the URL buffer (`gUrl`) and `OpenURL` at the window's closing, with the host's
  `OpenURL`. The nag screens (levels 8, 9, 10) are still parsed from the game file, and the game never goes to one.

## Performance (measured 2026-11-08, original and port back to back)

`tools/profile/perf.py` takes the numbers (its docstring has the commands). It samples the game's process every 0.1 s (working
set, private bytes, processor time) and takes the load time from the creation of the process to the main menu, the first screen
that takes input after the two logos (about 2.15 s of the figure). The port ran natively under rundesk in real time, without
`-fast`; the original ran under origrun on its real clock, from a copy of the install folder under `C:/byo-og`. Both in a window
(`fullscreen="0"`: the original's full screen is DirectDraw's own mode, which a private desktop opens only through dgVoodoo2, and
the numbers should be the game's and not a wrapper's). The scripted minute is the 60 s that begin 10 s after the first board has
started (Start, the first world, the story's button, the ship at rest, the same clicks in both). No speed-ups were made to this
port, so these are the port as merged.

**The machine was not quiet.** 2 to 5 game processes of other sessions were running, and the machine was 33 to 65 % busy, so
every figure is slower and noisier than on a quiet machine.

| First screen that takes input (seconds from launch) | Original under origrun | Port native |
|---|---|---|
| Five runs | 3.31, 3.13, 3.91, 3.16, 3.12 | 2.94, 2.75, 2.73, 2.81, 2.76 |
| Median | 3.16 | 2.76 |

At the main menu the original holds 102 to 112 MB of working set and 90 to 93 MB of private bytes, the port 122 to 131 MB and
125 to 127. The logos are most of the figure (about 2.15 s); the rest, about 1.0 s for the original and 0.6 s for the port, is
the loading.

| The scripted minute | Original A | Original B | Port A | Port B |
|---|---|---|---|---|
| Game steps in the minute (1000 a second is real time) | 59963 | 60038 | 60148 | 60164 |
| Peak working set | 109.0 MB | 119.5 MB | 164.2 MB | 163.9 MB |
| Peak private bytes | 96.7 MB | 100.5 MB | 160.4 MB | 159.8 MB |
| Processor time | 57.5 s | 58.2 s | 10.1 s | 10.3 s |
| Machine busy / other game processes | 47 % / 3 to 4 | 35 % / 2 | 39 % / 2 | 33 % / 2 |

The original uses a whole core: its message loop never waits (it polls the clock and redraws as fast as it can), so its
processor time is the minute's length. The port sleeps between updates and used 10 s of the 60. The port holds 45 to 55 MB more
working set than the original during the minute (about 20 MB of it at the menu already, and the first board adds about 33 MB to
the port's against about 7 MB to the original's); the performance pass looks at where it goes. Neither run fell behind real
time. The page's load time was not measured.

## The performance pass (2026-11-11)

**What the profile showed.** `games/bejeweled/tools/profile/cpuprof.py` (every thread's instruction pointer about once a
millisecond, weighted by cycles) under rundesk, on the port in real time over the load and 36 s of the first board: 68% of the
cycles are in ntdll (the system's calls on the game's behalf: the present, the driver), 13% in the exe. Of the exe's share the
game's own code is small: `DDInterface::PrepareTexture` (the framework's upload of the 800x600 screen, 6.6% of all cycles) and
`WaterBugsWidget::Draw` (the 16-bit screen widened to the framework's 32 bits, 3.0%) come first, then the engine's blitters
(`Alpha16MaskFade` 0.9%). On a dense late board under `-fast` (level 300, 9200 updates in 5.4 s, about 1700 updates a second) the
game's own code is 62% of the cycles and `Alpha16MaskFade`, the fading cell blit, is 24.5% of them; that matters only to how fast a
test runs (a real-time minute spends about 3 to 4% of a core there), and it was not changed. The start (the process to the first
frame) is the framework's window and graphics device for about 300 ms, then the game's own `GameStart` for 175 ms (the art banks
45 to 60 ms, the game file with its 77 animations 120 to 136 ms; five runs); the original took 1.0 s for its loading and the port
0.5 to 0.6 s of "Resource Loading Time", so the loading stands as it is.

**What changed.**
- The widget's screen picture (`WaterBugsWidget`) is opaque and uploaded as it is, as the Mahjong ports' screen is: `SetImageMode(false,
  false)` so that the framework does not look through the 480000 pixels for alpha after each change, and `mUploadAsIs` so that it
  does not premultiply them pixel by pixel in strips before the upload (every alpha byte is 0xff, where premultiplying changes
  nothing). The 16-bit pixels are widened by a 65536-entry table (`gPixelsFromScreen16`, 256 KB, made once) instead of three shifts
  and two ORs a pixel. The pixels that reach the screen are the same.
- `memreport` (docs/TEST_COMMANDS.md): the framework answers it (the screen picture, the sounds and songs); the engine's own
  holders are added by `wb::AddMemReportHolders` (`src/testhooks/TestCommands.cpp`, from `WaterBugsApp::AddMemReportHolders`): the
  art banks' pixels and masks, the animations' frames and masks, the run-length masks, the palettes and animation records, and the
  back buffer.
- `tools/profile/cputime.py` (one real-time scripted run's processor time, every thread, and the peaks) and `pairs.sh` (two exes
  alternated over one script) for the comparison below.

**Proof that play is unchanged.** A windowed `-fast` run of 5000 updates (the start, a world, a board, random shots; seed 11) gave
byte-identical PNGs at 8 points (updates 100 to 4900) from the build before and the build after, and `tools/check.sh waterbugs`
passes with every frozen value as before (the scripted game and its rerun, the window run twice, the loss, the three screens' hashes).

**Numbers.** One real-time scripted run (the start, a world, the first board, random shots; 25.7 s of wall time, with 1 to 3 other sessions'
games running), the processor time of the process (kernel plus user, every thread), the build before
and the build after alternated:

| Processor time of the run | Before | After |
|---|---|---|
| Runs (s) | 5.77, 5.80, 6.80, 5.42, 7.28 | 4.08, 5.03, 4.80, 4.64 |
| Median | 5.80 s (22% of a core) | 4.72 s (18% of a core) |

That is 18% less processor time (the 7.28 and the 4.08 ran while a second chain of this session's runs overlapped them; without them
the medians are 5.77 and 4.80). The pairs of the alternation alone: 5.77 against 5.03, 6.80 against 4.80, 5.42 against 4.80. Peak
working set (158 to 167 MB) and peak private bytes (157 to 163 MB) are the same in both.

**Where the memory is.** `memreport` at the main menu (private bytes 132.8 MB; 35.6 MB of them were there before the game read a
file, the libraries, the window and the graphics device): the art banks 27.5 MB (the converted pixels and their 8-bit masks, one
block of 26.2 MB), the animations' 1058 frames 16.1 MB and their masks 5.7 MB, the run-length masks 4.5 MB, palettes and animation
records 1.9 MB, the back buffer 1.0 MB, the framework's screen picture 1.9 MB, the sounds 0.15 MB (the game file names only three)
and one song; the song's player holds about 3.5 MB (measured by leaving the load out). That is about 62 MB of the 97 MB the game added; the
rest, about 35 MB, is what the report cannot see: the heap's slack between the many frames, the driver's and the libraries' own
allocations after the start (`tools/memmap.py` finds 99.6 MB of private memory in 134 allocations, the largest 26.2 MB the art
bank and 15.8 and 12.7 MB the heap segments of the frames, the 22 allocations between 256 KB and 1 MB 20 MB). The original holds
90 to 93 MB at the same place with no graphics device, so the port's extra is mostly that device (35.6 MB before the first file)
and the real-time frames' driver memory: the 33 MB that the first screen after the menu adds in a real-time run is not the game's
(a `-fast` run, which presents far fewer frames, adds 4 MB there). The game's own data is the original's.

**Not done, and why.**
- *Decoding the animations when first used (as Mummy Maze's and Noah's Ark's pictures are).* A count of the animations whose frames
  a first board asks for found 42 of the 77, 14.6 of their 21.8 MB, so the gain would be about 7 MB of memory (4% of the private
  bytes) and about 40 ms of the start; `Animation_Find` also has to give the frame count at once (a FLIC's own count replaces the
  file's `num_frames`, and the mask file's count replaces that), so the headers would have to be read at the start anyway. Left
  as it is.
- *Sounds on first play:* three sounds, 0.15 MB; nothing to win.
- *The blitters* (`Alpha16MaskFade`, `RlePal16Alpha`), only a faster `-fast`.

**Shared costs seen here, for the coordinator** (the ones already listed in docs/COORDINATION.md hold for this game too):
`DDInterface::PrepareTexture` still hands the whole 800x600 texture to SDL for each drawn frame (1.9 MB, at 60 frames a second),
though the game knows which rectangle changed (`mDirtyLeft` to `mDirtyBottom`); a partial upload (or a streaming texture) in the
framework would help the games that draw from a software screen. imagehlp.dll takes 9.4% of the start's cycles on the main thread
(the profile of the first 2.2 s), also still without a call site. The graphics device is 35.6 MB of private bytes before the game
reads a file.

## Coverage check (2026-11-08)

The whole coverage check of `docs/COVERAGE.md`, after Ning Po's. origrun's `--coverage` recorded which of the original's
functions ran in 37 runs on the host, all on the real clock with the silent sound stand-in (the virtual clock runs this
engine at about 30 steps a second, so it was not used; the real clock also reaches the sound code). The scripts are
written by `tools/coverage/scripts.py` (one list of steps for each run, an origrun script for the original and a
`-realinput` file for the port, which checks a click's place by `state`), `tools/coverage/run.py` runs one (it waits for
room among the 3 game processes, stages the copy under `C:/byo-og/wbcov`, windowed, and keeps the hits in
`build/coverage/waterbugs/hits/`) and `tools/coverage/runall.sh` is the first batch. Hand-made saves
(`scripts.py save <levels reached>`) put a run on a late world at once. 18 of the runs ended by themselves and 19 at
origrun's timeout (150 or 200 s: the last Esc of a script that ends on the map or in a dialog does not leave the game; each
had played its whole script):

- `tour`: the Options screen button by button, the credits, the help pages, the high scores and the player list.
  `users`: the player list (pick, rename, delete with the question answered both ways). `typing`: the rename box typed
  with key events, because the game reads typed keys from its key queue and ignores character messages (shifted and plain
  letters, backspace, the arrows, home, end, delete, a name longer than the box, Return). `leave`: the main menu's quit
  button, answered no and then yes.
- `play1`: a new game from Start (the story screen, 60 random shots, Esc's quit question cancelled and then answered).
  `resumeyes` and `resumeno`: Start with a game in progress.
- `deep9` to `deep530` and the `more*` runs (17 in all): a saved game on a late world, Start, the world button, the level
  selection's start, then 45 or 70 shots at random places (the ship is sent with `move`, a click fires it, and every ninth
  shot also clicks the story screen's and the results requester's buttons). Many of them reached game over (the ship is
  lost on the later worlds), so the loss of lives, `Level_GameOver` and the high scores' table ran. `deepman`, `deepeasy`,
  `deephard` and `deepfocus` repeat one with the manual shooter, the easy and the hard difficulty, and the focus lost and
  regained in the middle of a board. `over149`, `over399` and `over519` start a saved game with one life; `held74` to
  `held269` hold the original's own test keys (`Powerup_Init` reads keys 0 to 9 and A to D: a power-up that appears is of
  the key's kind), a third of the shots each.

`games/waterbugs/coverage.txt` sets the game's own code as one range, 0x401000 to 0x442cf0: the game, Retro64's engine,
the system layer and the blitters of every pixel format; the rest of the exe is the C runtime and the libraries. It has
742 functions, and the runs reached 542 of them.

**Reached in the original with no counterpart in the port: 75 at first, 0 at the end.**

- **Placed: 18.** Functions the port translates inline or in another shape got their address in the comment above the
  function that holds them: the list templates' instances (0x401000, 0x4010a0, 0x4011a0, 0x401220, 0x40b260, 0x410810,
  0x412540, 0x411030, 0x411150, 0x4109b0, 0x41c130, 0x4107e0: the comment sat above `template <class T>`, which `tools/coverage.py` does
  not read as the block above the function, so the addresses are now on the line under it; a spec went to the
  coordinator), the multi-line header declarations of 0x40afa0 and 0x40b360, the destructor copy 0x419c00, the wrapper
  0x40ad50, the text input's setup 0x42c3b0 and the start-up code 0x42b340 (above `WaterBugsApp::Init`). These were comment edits only.
- **Left out on purpose: 56, as `none` lines in coverage.txt, each with its reason.** The std::list and container
  instances (the port uses `std::vector` and arrays), the per-format blitter wrappers (the port's screen is 16-bit only, so
  its `Image_Blit` and the like call the 16-bit blitter at once), the DirectDraw display object and the window code (the
  host and the framework do those: the window's size and place, the surface lock, the present, the mode change), the timer
  object behind `TimeMs`, the palette realize of an 8-bit display, the trial's "Free Trial Remaining" clock text, `FixPath`,
  the start-up globals the compiler makes, and an empty function that the main loop calls.
- **One was missing logic: 0x412360, the Shooter's destructor.** The original's list of shooters runs it on each node when a
  level is left: it deletes the beams still in the air, destroys the five Objs the ship holds and the base. The port deleted
  a shooter as a plain Obj, so a beam in flight when the player left a board (Esc and OKAY, or the game closing) was never
  freed. `Shooter_Destroy` and `List_DeleteNode(Shooter*)` are new (WBShooter.cpp); play does not change (the beams of a
  level that ends normally are removed already), and `tools/check.sh waterbugs` still matches (the frozen baseline).

**Defined in the port, never reached: 61 at the end** (the first two runs left 165). The port's function was read again against
the export for each, by one Sonnet agent over the whole list (`build/coverage/waterbugs/reread.md`; I read its verdicts on
the anchor's drop and the name entry against the export myself): **59 match, 2 differ**, both in `ParseHex4` and
`ParseHex3` (0x4205f0, 0x4206d0): for text whose first pair is not hexadecimal the original returns the text's address
where the port returns 0, which no shipped data reaches (no colour is written in hex). They are left as they are. The agent
also found stale comments (the enemy kinds in WBEnemy.cpp and above `struct Enemy` carried an older numbering, and `Log`'s
comment named the wrong function), which are corrected.

- **Reachable in the original, but no run collected them: 13.** The power-up kinds 1, 5, 8, 9, 10 (A), 11 (B) and 13 (D)
  at collection (0x4188d0, 0x418920, 0x418930, 0x418980, 0x4189d0, 0x4189f0, 0x418a30), the GiveWeapon helpers of the
  kinds 4, 12 and 7 (0x418c20, 0x418c70, 0x418d10) and the weapons' updates of the anchor,
  the missile and the time bomb (0x41a060, 0x41a2f0, 0x41a650). The held-key runs made the kinds appear, but the ship has
  to run into the power-up, and a script that sends it to random places did so for kinds 0, 2, 3 and 6 only. A run that
  steers the ship to a power-up needs the original's positions (origrun's `--watch` of the power-up list); a later pass can
  add it. Re-read: all match (the anchor's drop checked by hand against the export).
- **Reachable, not run: the display re-open, 4.** F7, Alt+Enter, F8 and the Options screen's VIDEO button open the display
  again, and the original then converts all its images (0x4015c0, 0x402670, 0x428640, and the button's handler 0x4293c0).
  Not run: it changes the machine's display mode, which the user is working on. The port's host switches the window itself
  (listed under the full screen above), never loses the screen, and does not call these three; they match the export, and
  are candidates for taking out (or for wiring in, if the port is to keep a screen format of its own).
- **Candidates for taking out unneeded code: 44.** No play of the original can reach these; they were not removed now.
  - The sweepers power-up (kind 15; no key reaches it and the data never gives it): 0x4189b0, `Sweeper_Push` 0x41c4a0,
    `Sweeper_Draw` 0x41cfb0, and the ship flag 4 branches that only it sets.
  - The mine and ice weapons (kinds 16 and 17, which no table gives) with `Ice_Freeze` 0x41a430, `Enemy_Freeze` 0x406e20
    and the two GiveWeapon helpers 0x418b80 and 0x418cc0.
  - The enemy kinds 0x12, 0x13, 0x14 and 0x10 (snake shooter, snake, smart walker, line walker): the random boards draw
    kinds 7 to 13 and 0 to 6 only (`Level_EnemyCounts`), and the data defines no placed enemy of these kinds (0x408de0,
    0x409160, 0x409480 and the `Enemy_FromXml` branches).
  - Draw kinds 3 and 6, the whole add-blend family (0x432440, 0x434520, 0x4359e0, 0x436490, 0x424cc0, 0x4241f0), the
    unclipped font paths (0x423d80, 0x424910, 0x423a90, 0x42bdb0, 0x42c0a0, 0x424ed0, 0x424f30, 0x42e240, 0x425270), and the
    blitters of sources and destinations the game never draws (0x42fee0, 0x430020, 0x434d00, 0x435bf0, 0x432970, 0x434a80,
    0x401a80).
  - Data features the shipped files do not use: hex colours and swapped colours (0x4205f0, 0x4206d0, 0x420be0), the panel
    tag (0x418050), the .tga branch and `Image_ConvertTo` (0x423b20: the portal pictures are not in the install), the FLI
    delta chunk (0x40c670), the text input's own surface (0x42cad0), `Log` (0x429240) and `Sys_Sleep` (0x428d00).
  - The name entry's on-screen keys, `MenuItem_TypeKey` and `Key_Push` (0x4159c0, 0x428fb0): the original's screen change
    (0x40d5e0) turns the name-entry screen into the high-score table, putting the score in under the player's name, so
    that screen is never shown (the agent's reachability verdict said otherwise; the export says this). The player's name is
    typed only in the player list.
  - `Obj_ClearChildren` (0x403480), an unreferenced copy of `List_Clear`.

**Never reached and not defined in the port: 139.** Mostly the exception-unwind funclets (`Unwind@...`), the trial and
registration code the port left out, the paths of the formats the game never uses, and the C++ library instances the engine
does not call. None looked like something the runs should have reached.

**Open or odd.**
- On the original's world map, a click that comes with its move in one update was lost three times (a level selection that
  did not start: `deep120`, `deep120b`, `held74`) and once crashed the original (`held269`, an access violation at the click
  on a world button). With each button hovered 300 steps before its click, all three ran to the end; the crashing run is
  `build/coverage/waterbugs/hits_old/held269.log`. A real mouse always moves first, so it is listed as an oddity
  (`DEVIATIONS.md`) and not chased.
- Windowed, the original sets its 16-bit screen to 5-5-5 (0x42ee60) and the full screen takes DirectDraw's format; the port
  draws in 5-6-5 in both. A windowed screenshot of the original therefore keeps 5 bits of green where the port's keeps 6,
  and the second phase of the identical-play pass (shotdiff) may see a one-bit difference in green there. No logic is
  affected.

**Spec for the coordinator (the tool).** `tools/coverage.py` does not read a comment above a `template <class T>` line as the
block above the function that follows it (the address counts as a mention only). Twelve of the 18 placements were this; the
fix is to treat the template line as part of the declaration. The report's "How it ended" cannot tell origrun's timeout from
a crash (`held269` crashed, and the report says "origrun stopped it (a timeout), or it crashed" for it as for the timed out
ones); the log's line "game exited (its process's exit code ...)" does.

**What it cost.** The first batch of 17 runs took about 50 minutes of wall time, mostly waiting for room among the 3 game
processes (a run itself takes 40 to 150 s); the 20 extra runs about 40 minutes. One Sonnet agent sorted the first 70 reached
functions (16 minutes, 300,000 tokens) and another read the 62 unreached ones (36 minutes, 340,000 tokens). This session's
own work was about five hours.
