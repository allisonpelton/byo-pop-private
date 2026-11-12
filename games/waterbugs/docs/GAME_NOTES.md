# Water Bugs: notes of the finishing pass (2026-11-08)

What the finishing pass added to the port: the standard test commands and the bot behind `move`, the drivers that play a whole
game of every mode by real input (natively and in the page), the fixes mod, the registration and outbound buttons taken out,
and the performance numbers of the original and the port back to back. How the engine and the game code work is in `README.md`
and `ENGINE.md`, the differences from the exe in `DEVIATIONS.md`, and the comparison with the running original in
`IDENTICAL_PLAY.md`.

## The game's screens, as the test commands name them

The engine moves between "levels" of a kind (`Level::kind`): 0 a board, or a world's story screen when it has no cells
(`mTotalCells` is 0), 1 the main menu, 2 the name entry, 3 the high scores, 4 the credits, 5 and 6 the world map, 7 the Options,
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
update), `score`, `lives`, `level` (the board's number, -1 off a board), `reached` (`mLevelReached` of the player), `difficulty`,
`shootermode`, `rand` (the generator's state), `over` (the game has ended: the name entry, the high scores or the results after a
board was up) and `busy` (the bot has nothing to press and waits). On a board: `percent` and `quota` (the fill and the fill to win),
`board` (`cols`, `rows`, `state`, `gameover`, `shots`, `lines`, `cells`, `filled`, `weapons`, `texts`, and `ship`: `x`, `y`,
`state`, `across`, `noauto`, `speed`), `mouse` and `pointer`, `powerups` as `[x, y, state]` and `enemies` as `[x, y, vx, vy, kind,
state]`. On every other screen, and on a board with a requester up: `dialog` (a requester is up) and `items` as `[kind, x, y,
name, mLockedLevel]`, with `sets` as `[start, end]` for each world.

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
applied at a restart (the game reads them once, at startup). `Platform::FixOn(name)` in the host and `Sys_FixOn` in the game's code
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

## Unneeded code taken out (2026-11-11)

The coverage check's list of candidates (44, below) is dealt with: 1,099 lines out of 34 files, 168 in. What counted as unneeded
is code that no input reaches: nothing in the shipped files, nothing the player or a test key can do. Code that only some input
reaches (a value of the game file's own language that the shipped files do not use) stays, as `docs/COVERAGE.md` says; the
second list below gives each case and why.

**Taken out.**

- *The demo mode.* The original's `gDemo` flag is never assigned in its code, so the branches that let the program play for
  the player are dead: the ship's own steering and guides (`Shooter_Control` and its neighbours), the pointer's, the menu
  item's, the Level update's and the won-level test's demo branches, the demo's lives and elapsed time, and the flag that kept
  the message pump running for it (`gPumpMessages`).
- *The sweepers power-up* (kind 15, reachable only through the demo): `Powerup_Sweepers` 0x4189b0, `Sweeper_Push` 0x41c4a0,
  `Sweeper_Draw` 0x41cfb0, `Sweeper_Spark`, the ship's flag 4 branches that only it set, and the ship's sweeper Obj and
  particle member.
- *The water mine and the freeze* (power-up kinds 16 and 17: the kinds table holds 0 to 13 and the test keys make 0 to 13,
  so nothing makes either): the weapons of kind 0 and 3 (their set-up, move and launch-sound cases), `Ice_Freeze` 0x41a430,
  `Enemy_Freeze` 0x406e20, `Enemy_UpdateIce` 0x406f50, the enemy's ice capsule (the member at +0x10c and the dozen places that
  tested it), and the two cases in `Powerup_Collect` and `Powerup_Init`.
- *The fonts' unclipped drawing.* Every font that is drawn has its flag for clipping set (the loader and the text input set it,
  and the four fonts that never get it are never drawn), so the three drawing functions lost their unclipped loops, and the flag
  (0x422330) with them. The font's texture and mask target (+0xd50 to +0xd5f) are never set to anything but null, so their
  branches went too. With them went what only they called: `Image_BlitUnclipped` 0x423d80, `Image_BlitAlphaFadeUnclipped`
  0x424910, `Image_SetMask` 0x423a90, `Rect_Overlap` 0x42bdb0, `Rect_Intersect` 0x42c0a0, `Image_AddMaskUnclipped` 0x424ed0,
  `Image_AddMask` 0x424f30, `AddMask8` 0x42e240 and `Image_FillAlphaRect` 0x425270.
- *The text input's own surface* (`DrawOwn` 0x42cad0): the game always passes null for it, so the surface, its background, the
  active-last-time flag and the redraw flag are gone, and `TextInput_Init` has two parameters fewer.
- *The display re-open* (F7, Alt+Enter, F8 and the VIDEO button in the original make it convert every image again):
  `Animation_ReloadAll` 0x402670, `Animation_FreeMasks` 0x4015c0 and `Bank_Reconvert` 0x428640, and the reload flag of
  `Animation_Load`. The port's host switches the window itself and never loses the screen; `Screen_SetFullscreen` (0x4293c0) is
  that switch and stays.
- *Functions with no caller:* `Obj_ClearChildren` 0x403480, `ParseHex3` 0x4206d0, `Log` 0x429240 (an empty function; its one
  call is gone), `Sys_Sleep` 0x428d00 (the same), `DrawList_Count`, `Sys_Host`, `SetScreenLostCallback` 0x4293a0, `Xml_Reset`
  0x41ee70 (what it did is `Xml_NextTag`'s start and the destructor's free; the address now sits above the destructor), and
  `Particles_Init` with `ParticleSystem_Init` (0x40c2c0: the original's start-up construction of the pools, which the port's
  zeroed statics replace; a pooled particle or system is set up in full by `ParticleSystem_Add` and `ParticleSystem_New`, and
  only `Obj_Update` reads the base's other defaults, which particles never go through).

The structs that mirror the original's lose the members above, so `Shooter`, `Enemy`, `Font`, `TextInput` and `Text` (which
ends in a `TextInput`) are smaller than the original's, and their `static_assert`s and the notes beside them say by how much.
`coverage.txt` gains three `none` lines (0x40c2c0, 0x422330 and 0x4293a0, the three the original runs).

**Kept, because some input reaches it.** The draw kinds 3 and 6 (the game file's `blit_type="solid"` and `"add"`) with the
add-blend family and the fill blitter they call (0x432440, 0x434520, 0x4359e0, 0x436490, 0x424cc0, 0x4241f0); the blitters for
image formats the shipped art does not use (0x42fee0, 0x430020, 0x434d00, 0x435bf0, 0x432970, 0x434a80, 0x401a80), since
the art's formats are the game file's; the enemy kinds 0x10, 0x12, 0x13 and 0x14 and their `Enemy_FromXml` branches (0x408de0,
0x409160, 0x409480); hex colours (`ParseHex4` 0x4205f0, in `Obj_FromXml`) and swapped colours (`SwapColours` 0x420be0); the
panel tag (0x418050); the `.tga` branch and `Image_ConvertTo` (0x423b20); the FLI delta chunk (0x40c670); and the name entry's
on-screen keys (`MenuItem_TypeKey` 0x4159c0 and `Key_Push` 0x428fb0), which an item of type `hs_stuff` in any screen of the
game file reaches (the shipped screen that holds them is never shown: the original's screen change, 0x40d5e0, turns the
name-entry screen into the high-score table).

**Found on the way: the level's destructor was never called.** The original's list of levels (0x4011a0) runs the level's
destructor, 0x413b70, on each level it drops: it unlinks the fills and particle systems, deletes the cells' columns and the
background, and empties every list. The port had translated it (`Level_Destroy`) but deleted a level as a plain object, so
a dropped level's columns, background and lists were never freed (the coverage check matched the destructor by its address
and missed that nothing called it, as it had for the shooter's). `List_DeleteNode(Level*)` now calls it. Play does not change.

**Proof.** Every Water Bugs target builds, `tools/check.sh waterbugs` passes with the frozen baseline unchanged (the scripted
game, its rerun, the windowed exe twice, the lost game, the three screen hashes), and a six-game cycle of the headless runner
(start a game, lose it, the high scores, Esc, again; about 3.5 minutes of game time) plays out with the same state lines as the runner
from before this phase. With a temporary count in the new delete, that run dropped 3,666 boards and six of each other
screen kind with no crash. There is no AddressSanitizer for the i686 llvm-mingw build, so a heap error in the new
destructor path would show only as a crash; the web build (`tools/check.sh build`) is built before the merge request.

## The readability pass (2026-11-11)

The pass of `docs/CODE_STANDARDS.md`. Nothing the port does changed (the proof is at the end), and
`python tools/namecheck.py games/waterbugs` finds nothing. It has two exceptions, in `namecheck.txt`: `engine retro64`
(Retro64 is the game's own engine, now the one copy under `engines/retro64` that Cosmic Bugs shares, so it is checked
like the rest) and `allow offset gScreenFormat565`
(the pixel format's name: red 5 bits, green 6, blue 5, which is not an offset). These notes use the new names; the
sections above that report a past run (the performance figures, the coverage check) keep the names of their time.

**Names.** Water Bugs has no Mac build, so every name comes from how the code uses the thing. The rules ran in this
order. Rule 5: every one-letter name past ten lines of its declaration is a word (a blitter's `dstPitch`, `srcX`,
`maskY`; `theWidth` and `theHeight` for the old `w` and `h`; the loops keep `i`, `j`, `k` and the coordinates
`x`, `y`). Rule 2: no member or local is called unknown, and no field is named by its offset. Rule 4: the framework's
prefixes on every member (`m`), parameter (`the`, `an`), local (`a`, `an`), global (`g`) and static (`s`), the
constants and enum values in UPPER_CASE (`SCREEN_CLIP`, `OUTPUT_VOLUME`, `RAND_A`, `TRIAL_TIME_FIELD`). Rule 6: the
host and mod files carry the port marker, and every function has its address or says it is the port's.

**The prefixes came from clang-tidy.** `readability-identifier-naming` with the prefixes and `CamelCase` (without
`*Case: CamelCase` it writes `thedst`) renamed every declaration and its uses, then a person repaired what it
mangled. Four things to know for the next game: run it once for each translation unit with `--export-fixes` and apply
all the files together with a small script (applying one file after the other, or exporting all units in one run, loses
uses); `--header-filter` needs `.*waterbugs.*` (the paths have backslashes); a name that begins with its prefix's letter
comes out wrong (`mask` became `mAsk`, `menu` `mEnu`, `mode` `mOde`, `max...` `mAx...`: a dozen names, repaired and
checked by the compiler); and it does not reach `offsetof(...)` arguments, members of anonymous-namespace structs,
or a member reached through a template (`aNode->mNext` in the list templates), which fail to compile and take a
minute each to fix. It also does not compile what the native build leaves out: the `__EMSCRIPTEN__` blocks of
`WBMain.cpp`, `WBMenu.cpp`, `WBText.cpp` and the whole of `SiteProfiles.cpp` kept their old names (`.name`, `->current`,
`->font`) until the web build, `tools/check.sh build-web:waterbugs`, broke on them. The comments that named a renamed
identifier were changed with a script that touches comments only.

**Fields named for what they hold (rule 3).** `tools/splitfields.py waterbugs` finds no member declared twice at one
offset, none that nothing writes, and three that nothing reads: the ship's `mSlowMotionTime` (+0x490) and
`mHyperMotionTime` (+0x494), which the two power-ups set to 5000 and the original reads nowhere (the flags beside them
are what its code tests), and a text's `mDormant` (+0x1c0, which `mLifetime` starts from). They are the original's and
stay, with their comments saying that nothing reads them. The padding that keeps a struct's layout is named for the
field it aligns (`mBonusPadding`, `mItemsPadding`, `mDragPadding`, `mLaunchSpeedPadding`, `mInputPadding`,
`mWingsPadding`, `mPaletteNumberPadding`, `mWidthPadding`, `mEndPadding`, `mFontPadding`, `mOwnedPadding`,
`mOffsetPadding`, `mCountPadding`). The tool's check of members that the export's functions touch under another name
had nothing to compare: it reads definitions written `Class::method` with an address, and Water Bugs' are free
functions (it found one).

**Numbers named (rule 7).** Where one value meant one thing in many places it has a name:

| names | what they are | uses |
|---|---|---|
| `SCREEN_WIDTH`, `SCREEN_HEIGHT`, `SCREEN_CENTRE_X`, `SCREEN_CENTRE_Y` (`engines/retro64/R64.h`) | the 800 by 600 screen and its middle | 140, in the game, the host and the engine |
| `CELL_SIZE`, `CELL_HALF`, `GRID_COLUMNS`, `GRID_ROWS`, `BOARD_WIDTH`, `BOARD_HEIGHT` (`WBGame.h`) | the 16-pixel cell, half of it, the 50 by 37 board and its 800 by 592 pixels | 270: every position over 16 is a cell index |
| `CellKind`: `CELL_OPEN`, `CELL_BEAM`, `CELL_LINE`, `CELL_EDGE`, `CELL_FILLED`, `CELL_SOLID` | what a cell is (`Cell::mKind`) | 145 |
| `EnemyKind` (twenty names: `ENEMY_FROG`, `ENEMY_BUMP_ROLLER`, `ENEMY_CANNON_BALL`...) and `EnemyState`: `ENEMY_APPEARING`, `ENEMY_DYING`, `ENEMY_FADING`, `ENEMY_ALIVE` | what an enemy is and where it is in its life | 90 |
| `PowerupKind` (fourteen names) and `WeaponKind`: `WEAPON_MISSILE`, `WEAPON_TIME_BOMB`, `WEAPON_MAGNET`, `WEAPON_ANCHOR` | what a power-up and a weapon are | 80 |
| `TEXT_IDLE_FOREVER`, `TEXT_EMPTY_PROFILE` | a text's idle time of 0x986fc8 ms (about three hours: it stays until clicked away), and the text id of an unused profile slot's name | 30 |

The kinds' meanings come from how the code uses them and from the comments the earlier passes wrote over the structs;
two are judgements. `CELL_LINE` is what a beam leaves (the walkers follow it and the bump rollers push it), and a
blast's rim is made of lines too; `CELL_BEAM` is a cell a growing beam crosses, which the capture turns into a line.
The power-up kinds are named for the function each calls (the kind 1, `Powerup_FlipFlopper`, plays the "power down"
sound); the test keys `0` to `9` and `A` to `D` make kinds 0 to 13, and any other number is the fast shot. The
screen's size appears twice in `Obj_FromXml` and the text and emitter loaders as `x * 800 / 800`: the original scales the authored position by the
screen's size over itself, and the port keeps both factors, now named. The shooter's range test (`16 < x < 784`, `16 < y < 584`) is the screen
less a cell, not the board (592 is the board's bottom).

What stays a number, each where its comment or the line makes it plain: the level kinds and the menu items' kinds (the
`switch` cases of `MenuItem_Update` and the screens' own tables), the ship's and the objects' states, the draw kinds,
the particle systems' colours and counts, times in milliseconds, speeds, scores and the points of a chain. They are
the original's own tuning and appear once or twice each; naming them would be a list of constants nobody reads twice.

**Lines.** Over 120 columns (tabs of 4) are wrapped at a comma or an operator, or the comment moved above the line;
a few comment lines that begin with an address stay long.

**The proof.** `tools/objsnap.py` took the game's objects before the first change (31 objects, 706 functions) and
compares them after every part: each function's instructions with the names taken out, and the data bytes, which
hold every string, log line and table. All of them are identical except one thing: the unit `WBPowerup.cpp` is one
byte longer, in `.llvm_addrsig`, the table of the symbols whose address is taken (linker metadata, no instruction
and no data; it lists symbols by their index in the object's symbol table, and the renames changed that table's
order, which makes the table a byte longer). The comparison caught a slip on the way: the shooter's 584
is the screen's height less a cell, not the board's 592 less a cell. `tools/check.sh waterbugs` (the suite against the frozen baseline) passes, and so do `tools/check.sh build` (every target
and tool) and `build-web:waterbugs`; the web build matters because the native build compiles none of the
`__EMSCRIPTEN__` blocks nor `SiteProfiles.cpp`, which is where the old names were still found.

## The shared engine (2026-11-11)

The engine moved from `games/waterbugs/engine` to `engines/retro64`, the one copy that Cosmic Bugs is built on too
(`docs/ENGINES.md`; the user, 2026-11-11: "Share engines"). Its files are this game's readable version, with Cosmic Bugs'
copy folded in, and each function's comment gives its address in both exes, tagged with the game's folder name
(`0x40c2e0 (cosmicbugs), 0x40c4d0 (waterbugs)`). The two exes hold the same library, so nothing needed a switch between
the games: what only Cosmic Bugs' copy had is there for both and costs this game nothing, and what the game's own code
chooses it still chooses.

**What came back.** The unneeded-code pass above took out what no Water Bugs input reaches; that is in the shared engine
again, because Cosmic Bugs' copy had it (`docs/ENGINES.md` lists which of it any game's code calls: only `Sys_Sleep` and
`gPumpMessages`, both Cosmic Bugs'), and every function of it is one of this exe's own: the fonts' unclipped
drawing and the flag that chooses it (`Font_SetClipped`, 0x422330), a font's texture and mask target (+0xd50 to +0xd5f),
`Image_BlitUnclipped`, `Image_BlitAlphaFadeUnclipped`, `Image_SetMask`, `Rect_Overlap`, `Rect_Intersect`,
`Image_AddMaskUnclipped`, `Image_AddMask`, `AddMask8` and `Image_FillAlphaRect`; the text input's own surface and
background (`DrawOwn`, 0x42cad0, and the two parameters `TextInput_Init` has again); `Bank_Reconvert` (0x428640),
`Xml_Reset` (0x41ee70), `Sys_Sleep` (0x428d00) and `SetScreenLostCallback` (0x4293a0); the message pump's flag
`gPumpMessages` (0 here, set by Cosmic Bugs' demo code, as the original's own demo set it); and the port's
`DrawList_Count` and `Sys_Host`. The game's own code is what the original's is: it sets a font's clipped flag to 1 where
the original does (`Font_LoadXml`, `Screen_Redraw`; `TextInput_Draw` does it in the engine), and it gives the text input
no surface and no background. Nothing else of what came back runs, so the game plays as before. The structs that mirror the
original's are its size again (`Font` 0xd6c, `TextInput`, and `Text` 0x6c8 with it). `coverage.txt` reads
`src engines/retro64` and loses its `none` lines for 0x422330 and 0x4293a0, and `namecheck.txt` reads
`engine retro64`. The coverage count of this game's exe goes from 522 of 742 functions with a counterpart to 536: twelve more,
all of them what came back.

**What stays in the game, and what the unneeded-code pass above still says.** The demo mode and its branches, the sweepers,
the water mine and the freeze, the unreachable display re-open and the other game code are still out of `src`; only the
engine's functions listed here are back, and the pass's list of what it took from the engine stands as a list of what
only the other game reaches.

**The proof.** The engine's objects before and after, function by function with positions taken out (the engine now has
more functions, so their addresses in the object move): of the 236 functions of the old objects 199 have the same
instructions. The 36 that differ are the ones that read a member of a struct whose layout is the original's again (every
`Font` and `TextInput` function: a member's offset), `PumpMessages` (the pump's flag), and `Image_BlitAlphaFade`, which the
compiler now builds around a helper that two functions call. In `src` seven objects differ: the three functions that gained a call and the ones that use a struct holding a `Font` or a
`TextInput` (the size of `Text`, and the offset of the members after a font). `tools/check.sh waterbugs` passes with the frozen baseline unchanged, `tools/check.sh build`
(every target and tool) and `build-web:waterbugs` pass.

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
  were taken out on 2026-11-11 (`Screen_SetFullscreen`, the host's switch, stays).
- **Candidates for taking out unneeded code: 44.** No play of the original can reach these; they were not removed now (taken
  out or kept on 2026-11-11, "Unneeded code taken out" above).
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

## The PopCap Game Collection version (2026-11-11)

The port was staged from a PopCap Game Collection install (`stage_waterbugs_assets.py` looks first in `Program Files (x86)/Popcap Game
Collection/Water Bugs`), and no other edition of the game is on this machine (Steam has none), so the check against the Collection's
version (`docs/COORDINATION.md`) compares the disc's files with the staged copy. The procedure is AstroPop's (its GAME_NOTES section 13);
`tools/collection/` holds the file comparison and the run.

**Getting the files.** `unshield` (`E:\retools\unshield-build`) read the disc image's cabinet, mounted read-only and dismounted afterwards;
the group `WaterBugsComponent` came out: 196 files, 24 MB, a flat folder with the names and layout of the staged copy (`WaterBugs.exe`,
the four `water_art*` files, `audio/`, `wanims/`). **189 of them are the staged copy's byte for byte, `WaterBugs.exe` included** (513,320
bytes, linked 2007-10-30, version 1.0.0.1), so the version is the same build and the port's reads of the exe (the texts) are the same
reads. The other seven are files the port never reads and the stager never copies: `bass.dll`, `license.txt`, `readme.html`,
`properties/partner.xml` and its `.sig` (product name, registry key `PopCap\WaterBugs`, offering number, 640 by 480), and the developer's
tools `wanims/encrypto.exe` and `wanims/i.bat` (the batch file encrypts two of the XML files; neither was run).

**The port on them** (`tools/collection/run.sh`). The stager makes the same 189-file tree from the disc's folder as from the install
(`cmpfiles.py`: 189 the same, none only in one). The suite's three plays, once on each tree under rundesk with `-fast`: the headless
runner's game (score 0 at 20 s, 494 with the beam at 32 s), the loss and the high scores, and the windowed exe's state after the beam, are
the same on both, and the three screenshots' pixels are the frozen ones (`2a54908cecb8 42df2d86a04f 983a569fb1b6`). The page's importer
rules (`web/page.js`: markers `water_art.bbk`, `water_art8.bbk` and `WaterBugs.exe`, the tools in `wanims/` skipped) on the disc's folder
as loose files, as `Water Bugs/` and as `Popcap Game Collection/Water Bugs/` find the game's root and pick exactly those 189 files, so
the site's import takes the Collection's folder as it is (checked ad hoc under Node; `tools/gameimporttest.js` has no Water Bugs line
because the game is not yet in the finished list). The page's hint named `Program Files/PopCap Games/Water Bugs`, which is not where the
Collection installs the game; it now names `Popcap Game Collection/Water Bugs`. Not run: the web build under Node on these files, since
its input would be the same bytes.

## Processor time, like for like (2026-11-12)

The target of `docs/COORDINATION.md` ("The target"): the port's processor time at most the original's over the same stretch of
play, the original paced as on a player's 60 Hz screen, cycles per thread (QueryThreadCycleTime), the port driven by a
`-script` (no `-commands` channel), alternating runs and medians under `tools/pincore.py`. The README's 58 s a minute for the
original was the original spinning (its main loop waits for a vertical blank that the hidden desktop never gives); paced, it
takes about 4 to 5 s a minute.

**The tools** (`tools/perf/`, from Cosmic Bugs'). `perf.py` runs one side over the scripted play and writes `result.json`: the
clicks at game updates 2600, 3600 and 5000 (Start Game, the first world, the story's button; the port's at the same counts, its
framework update U being game update 10 U + 6), then the first board with the ship at rest, the window the game updates 16000 to
76000 (the scripted minute); memory, processor seconds and the cycles of every thread, sampled every 20 ms, with the busiest
thread apart. `orig --paced 16` patches the original's main loop (VA 0x41f562, a timer read whose result nothing uses, 11 bytes
→ `push 0; push 16; call [SleepEx]; nop`, the import at 0x458160) so that a pass lasts 16 ms, and holds each click 40 updates
(a paced pass is 16 updates, so a click held 5 goes down and up inside one pass and is lost). Its `board_reached` checks that every
click came through and that the level was entered after the map's click. `--nosound` runs the port with its sound and music off,
which is how origrun's original (a stand-in that mixes nothing) is counted; with sound on, the audio thread is a row of its own.
`alt.sh N label=kind[:exe]...` alternates the runs (every run of a round, then the next round) and prints the medians; it is run as
`python -I tools/pincore.py -- bash games/waterbugs/tools/perf/alt.sh 5 orig=orig before=port:<exe> after=port:<exe>`. `shots.py A B`
compares two exes' screenshots over one `-fast -drawevery` play (`--long SEED`: 90 s of random clicks, a picture every 300 ms).

**What the port's main thread spends** (a scratch build with QueryThreadCycleTime timers around the calls; 60 presented frames
and 100 updates a second, steady over 60 s; Mcycles a second). Total 198: the game's steps and its drawing into the 16-bit screen
37; the host's scan for what changed 38 (before the vector compare below); widening the changed blocks about 12; the framework's
upload of the whole 800x600 picture inside `DrawImage` 77; the present and the rest 34. Besides, the graphics driver's worker
thread (`nvwgf2um.dll`) takes about 80 to 96 and the audio thread 7 to 11. So about 111 of the main thread's 198 and all of the
driver thread's are the shared renderer path (docs/COORDINATION.md, "Shared costs waiting for a fix"), which only a presenter that
needs no texture can remove.

**What changed in the host.** The same change Cosmic Bugs made (its README): the engine's update rectangles say what it copies
from its back buffer, not everything it draws (sprites, beams, text and fades go straight into the screen), so the host no longer
widens them. It compares each presented screen with the copy its image was last widened from, sixteen bytes (eight pixels) at a
time, a row first and then 16-pixel blocks, widens only the blocks that differ, copies them into the last copy, and keeps them as
rectangles (`mChangedRects`, runs two blocks apart joined, equal runs of consecutive rows joined) for a presenter that takes a
dirty rectangle; the framework still uploads the whole image. The engine's `AddUpdateRect` is no longer used by the host.

**Proof that the pictures are unchanged.** A scratch build compared the image with a full widening of the engine's screen after
every presented frame: 0 frames differed in about 14,000 (the 5100-update play of `shots.py` and 90 s of random play from two
seeds). Screenshots of two builds are a weaker proof here, because a frozen exe differs from itself between two runs in a few pixels
(three to eight pixels in a box near x 758 to 788, y 484 to 500, and a few at x 42 to 57 on the left edge; the bubbles and the
fade of a loss are drawn from an unseeded generator, as `tools/check.sh` notes for the loss screen): of 16 and 250 pictures the old
and new exes differed in 1 and 15 pictures, by up to 8 pixels, and the same exe against itself in 1 and 2 to 6.
`tools/check.sh waterbugs build build-web:waterbugs` passes, the frozen screens and states unchanged.

**The window each side ran in (2026-11-12, after Mahjong's note).** The first two sets below (the 262.1/235.4 and 221.6/105.8
medians) were not like for like: the port's window started at 1600x1200 (render scale 2, the framework's default of twice the
game's size where the desktop has room) while the original runs windowed at its own 800x600 (the staged `cp.cfg` has
`fullscreen="0"`; the engine sizes its client area to 800x600, 0x42ea10; origrun's log prints no window size, so this is
from the code and the settings, not a measured size). Every run now logs the framework's display line, and `perf.py port` takes
`--scale N` (default 1, passed as `-windowscale=1`; 0 leaves the default) and stores the line in `result.json`: `display: client
800x600 px, logical 800x600, render scale 1, windowed, renderer direct3d11` for the 1x runs and `client 1600x1200 px, logical
800x600, render scale 2, windowed, renderer direct3d11` for the default window. Every port run here ran on rundesk's private
desktop, so every one got Direct3D 11; the original has no renderer of the framework's (DirectDraw).

**Numbers** (medians of five rounds under pincore, each round one run of each variant, the board's scripted minute, Mcycles a
second, the trunk of 2026-11-12 (cc8a7c35); the port with sound on unless it says muted):

| | All threads | Main thread |
|---|---|---|
| Original, paced, windowed 800x600 | 102.8 | 102.8 |
| Port, the host before the change, 800x600 | 205.7 | 120.6 |
| Port, the host change, 800x600 | 200.8 | 114.0 |
| Port, the host change, 800x600, sound off | 187.3 | 111.3 |
| Port, the host change, default window (1600x1200) | 195.5 | 110.7 |

The window's size does not move the total (the framework's render target is the logical size either way; the 2x window only
changes what the GPU stretches), and the host change takes about 5% off the main thread. The threads of one round (the third):
the original's one thread 102.8; the port's main thread 110 to 111, the graphics driver's worker (`nvwgf2um.dll`) 73 to 76 at
the original's window size and 53 at 2x in the one round read, the audio thread 6.5 with sound on, and a second driver thread
and the text-services monitor about 1 each. So the main thread is within 8% of the original's and the gap, about 85 to 100 of
the 200, is the driver thread that the SDL renderer path brings, plus the sound's 6.5 that the original's number does not hold
(origrun's stand-in mixes none; the muted row counts the port the same way). The spread was narrower than before (the port 171
to 217, the original 94 to 114), the machine being quieter. What stands between the port and the target is the renderer path:
the presenter that draws into the window surface and takes the host's changed rectangles (`SurfaceDisplay`, not on the trunk
yet at that time; the next entry is the port with it).

**The first set, at the framework's default window instead of the original's size** (medians of five pairs under pincore, the board's scripted minute, Mcycles a second):

| | Original, paced | Port before | Port after the host change |
|---|---|---|---|
| All threads | 128.8 | 262.1 | 235.4 |
| Busiest (main) thread | 128.8 | 160.5 | 140.1 |

(The runs spread widely with the other sessions' games: the port before 208 to 332, after 225 to 289, the original 110 to 154.)
A second set on the trunk of 2026-11-12 (the framework asking SDL for Direct3D 11 first) gave the port 221.6 over 6 runs against
the original's 105.8 over 3: the driver thread's 80 and the main thread's 120 to 130 did not move. So the port is about twice
the paced original, and what stands between it and the target is the renderer path: the presenter that draws into the window
surface and takes the host's changed rectangles. Opting in to it, and the numbers with it, are the next entry here.

**The surface presenter (2026-11-12): the target met at the original's window size.** The native build opts in to the framework's
`SurfaceDisplay` (`framework/SurfaceDisplay.h`), as Bonnie's does: `WaterBugsApp::CreateDDInterface` returns one (a test run's
`SURFACE_DISPLAY=off`, asked after `IsTestRun` as Mahjong's is, keeps the framework's own display and the widget's picture, the
path before this), and the widget keeps no `MemoryImage` then: `Draw` widens the changed rectangles (the 16-pixel blocks of the
comparison above) into its own 32-bit buffer `mScreen32`, and hands the display that buffer and `mChangedRects` with `SetFrame`
(everything for the first picture), before the framework draws the widgets; the display copies just those rects into the window's
SDL surface and has the window update them, with no SDL renderer, texture, Direct3D device or driver thread. `~WaterBugsApp`
takes the buffer back from the display first. The web build is as it was (`#ifndef __EMSCRIPTEN__`). The rectangles are exact by
construction (every pixel that differs from the last copy is inside one), so the pictures cannot differ from the old path's.

**Proof.** `tools/check.sh waterbugs build build-web:waterbugs` passes, the screens as frozen. `SURFACE_VERIFY=1` (every present
compared with the engine's screen) over the measured play: 4,503 frames at the original's window size and 4,498 at the default 2x
window, none differing, no frame through a renderer. A scripted F7 (the game's full-screen key, 0x76) with the check on: the
window starts in surface mode, F7 moves it to the 3840x2160 full screen in the presenter's renderer mode (render scale 4; the
menu's screenshot is whole) and F7 again to the window, which stays in renderer mode by the presenter's rule (a Direct3D 11 swap
chain keeps its last frame on the window); 486 frames through the surface, 800 through the renderer, none differing. The same
exe's `shots.py` play with `SURFACE_DISPLAY=off` against the default: 14 of 16 pictures byte-identical, and the other two
differ in 8 and 1 pixels (up to 8 and 4 in a channel, in the menu's bubbles: the unseeded generator that a frozen exe's own two
runs differ in, above).

**Numbers** (ten rounds under pincore, five in the first set and five in the second, each round one run of each side, the board's
scripted minute, Mcycles a second, the trunk of 2026-11-12 with the presenter (1faa4ed3); the port's window 800x600 unless said
(`display: client 800x600 px, logical 800x600, render scale 1, windowed, renderer none`; the default window `client 1600x1200
px, render scale 2`, also surface mode); the original paced, windowed at its own size):

| | All threads, median (range) | Main thread, median | Per round against the original: rounds at or below, median ratio |
|---|---|---|---|
| Original | 135.8 (109.5 to 152.3) | 135.5 | |
| Port, sound on | 117.8 (97.9 to 138.3) | 108.5 | 10 of 10, 0.86 |
| Port, sound off (as origrun's original, which mixes none, is counted) | 107.8 (82.1 to 126.0) | 106.3 | 9 of 10, 0.78 |
| Port, sound on, the default 2x window | 128.0 (104.6 to 153.9) | 118.9 | 6 of 10, 0.92 |

The machine was busier than for the first measurement above (the original 109 to 152 where it had been 94 to 114), so the
rounds' ratios are the comparison, not the absolute figures. In the first five rounds the port on the framework's own display
(the exe before this change, on the same trunk) ran 1.46 to 1.74 times the original in each round (median 1.69). Threads of
one round with the surface: the main thread 115 and the audio thread 9.8 with sound on, 109 and 1.5 muted: the graphics driver's
worker thread (67 to 76 before) is gone. Memory: working set 113 MB, private 104 MB (the original 110 and 97; the old display
134 and 136). The 2x default window repeats each pixel into a 2x2 block in the blit and costs about 10% more than 1x, so at the
default window the port is at about the original's cost, not below it.
