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
- **Not a fix: the ship's appearance.** After a board starts, and after a life is lost, the ship comes in over 70 updates and
  fires nothing meanwhile. It is an animation of the game, not a wait on loading, and stays.

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
