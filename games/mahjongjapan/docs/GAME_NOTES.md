# Mahjong Escape: Ancient Japan: notes on the original

This file records what the translation found in `MahjongEscapeAJ.exe` (linked 2006-11-08), for whoever reads or changes
the port. Addresses are the original's, and the port's declarations carry them as `// 0x...` comments, so a grep finds
both sides. `ENGINE.md` maps JollyBear's engine under the game. `DEVIATIONS.md` lists where the port's code differs.

## The program

The game is SpinTop Games' and runs on JollyBear's engine: an app object (0x424a30, vtable 0x51df94) whose manager
keeps a stack of layers and sends them a frame every 25 ms. Each layer is a tree of objects. The game's layers are
the splash, the main menu (0x40f540, 0x40f5a0), the player list, the classic list of layouts, the map of the escape,
the play layer with its board, the ending, and the dialogs over them: the options, the tile sets, the backgrounds,
the help pages, the tips and the yes/no questions.

The game's resources are in two modules. `Resource.dll` holds the pictures, the sounds, the music, the 222 layouts
(`LAYOUTS` resources: 60 for the escape, 162 for the classic game) and the longer texts (`TXT`). The exe holds the
string tables of the menus and dialogs and two pictures named in its folder (`SplashLogo.bmp`, `MenuLogo.bmp`). The
port reads both modules with its own PE parser (`src/PeResources.cpp`) and never runs either. The game's texts are
read from them when the game runs; the few the port needs from the exe's code are named by address in
`src/GameTextTable.inc` (made by `tools/make_gametext.py`) and are never copied into the repository.

## Saves and settings

Each player has a file in the user folder: `list.usr` lists the players, and `<n>.usr` holds a player's settings,
best times and scores (one of each per classic layout and per escape stage), the escape's progress and the chosen
tile sets (0x415eb0; a CRC that also covers the exe's folder guards it). A game in progress is saved as a file of
its own per level. The app's settings (the volumes, full screen, wide screen, the hardware acceleration and the window's
place) are in the registry under `HKCU\Software\JollyBear\MahjongEscapeAncientJapan`, which the port emulates in
`registry.ini`.

## The rules

A tile is free when nothing lies on it and its left or right side is open (`Layout::IsFree`, 0x414c20). Two free
tiles of the same kind make a pair; the four seasons match each other and so do the four flowers. A pair scores its
kind's points, plus 500 when it is the same kind as the pair before. A hint costs 100 points. The undo takes the
pair's points back. A shuffle adds 20 seconds to the clock. "Show free tiles" shades the tiles that cannot be taken.
The autohint (an option) gives a hint after a while without a click.

**Emperor's Adventure** is 60 stages along a path through twelve periods of Japanese history, from the Jomon period to
the Taisho period. A stage is won by matching the two Magic Gold Tiles (kind 0x2b) once both are free (0x40d8b0). The
tiles still on the board then fly off: 200 points for each pair, 500 for each special tile. A level's end shows the
time, the score, the bests and the period's saying, and the map walks the marker on to the next stage. The first
stage's layout gives every tile its kind. The later stages give only some tiles their kinds and deal the rest, seeded
with the stage's own number from the stage table (TXT 0x17f), so a stage is the same every time it is played. Later
stages bring special tiles: a key (0x2c) that opens a lock (0x2d, which can be picked only with the key selected and
need not be free), and two tiles (0x2e, 0x2f) that pair with any ordinary tile that can be seen, free or not. A right
click on a special tile shows its tip. Each new thing gets a tip box the first time it appears in a level, until that
level has a best score.

**Classic** is the 162 layouts of the list, nine to a page, each dealt anew from the clock. A game is won when the
board is empty. The options' difficulty sets the share of the deal that goes in fours rather than pairs: 0 percent
for relaxed (a new player's), 25 for normal and 85 for expert.

Both modes have the same bar of buttons (undo, hint, shuffle, the menu and pause) and the same panel (the time, the
score, the bests, the moves available and the tiles left). The in-game menu goes back to the main menu, chooses a new
layout, opens the options, the tile sets, the backgrounds and the instructions, or resumes the game.

## The trial

The exe is the full game of the PopCap Game Collection. The translation found no trial code: no registration, no nag
and no buy screen.

## The standard test commands (2026-11-08)

The port answers the standard commands (`docs/TEST_COMMANDS.md`) from a test's `-script`, its live channel
(`-commands=<file>`, `byopop_test_command` on the web) or `-realinput`, through `HandleScriptCommand` in
`src/TestCommands.cpp`. A command that is never given changes nothing: no draw of the game's random numbers and no state.

| Command | What it does |
|---|---|
| `state` | `STATE {json}`: `screen` and `dialog` (the top layer and the dialog on it, null for none), `busy` (a transition or a flight is running), `player`, `progress` and `levels` (the adventure), `updates`, `layers`, and on a board `mode`, `level`, `layout`, `score`, `timer`, `moves` (the free pairs), `tiles` (left) and `board` (`started`, `over`, `escaping`, `blocked`, `selected`, `move`), and `buttons`, each `{cmd, x, y, w, h}` in game pixels, of the top layer. |
| `goto <screen>` | `menu`, `options`, `help` (from the main menu), `classic [n]` (the list, or layout n at once), `escape` or `map`. Answers once the screen has settled. |
| `level <n>` | The adventure's level n, started as the map's Continue starts it. |
| `unlock all`, `set score` or `set progress` with a number | The adventure's progress to its last level; a score or the progress. |
| `win` | The game's own moves, from the bot's way to the end of the game, until it ends. |
| `move` | `MOVE {json}`: the bot's next move and the real input that makes it (below); changes nothing. |
| `lose`, `seed <n>` | `ERROR`: a game here is never lost (a board with no pair offers the shuffle), and every deal has its seed from the game's own clock (the fixes mod's `-gameseed` is the seed). |

**The bot behind `move`** (`src/Bot.cpp`, one file for both games; `tools/realplay/syncbot.sh` copies it into China's
folder) works on a copy of the board's layout with the game's own rules for a free tile and a pair. It searches depth
first for a series of pairs that ends the game (the two Magic Gold Tiles of an escape stage, the empty board of a classic
game), remembers the positions that proved dead, and starts again with another order of the pairs and a larger budget
when one try fails, up to 60 tries. The key (0x2c) takes the lock (0x2d) it opens, and the wild tiles (0x2e, 0x2f) pair as
the game lets them; the first tries save the keys for the locks that hold a goal tile. Every one of the 60 escape stages
and every classic layout solves, the slowest stage in about 3 s. When no pair is free the bot answers with the shuffle's
click, and `MOVE null` while a flight runs, on a board with no tile left or when nothing can be played. A dialog is
answered by its default button (command 1), the pause box by a click on it, the map by Continue.

## Fix candidates and the fixes mod (2026-11-08)

The fixes mod (`mods/fixes/mod.json`, three toggles, all on by default on the site) is the original with three things
mended that cost the player or a test time, and nothing else. With the mod off, or no mods folder, the game is the
original; `-nomods` starts a native build without it and `-modsdir=<folder>` loads another folder.

- **`skipLogos`.** The game opens with the publisher's logo and Jolly Bear's, 2 s each, and loads nothing meanwhile
  (the splash's `Logo::Update`, 0x42f8c0). With the fix each logo is over at its first update. Launch to the main menu
  took 7.8 s in real time (median of 5) and takes 3.7 s with the fix; the rest is the loading and the logos' fades.
- **`seededDeals`.** The original seeds the generator from the clock at every deal (a classic game's, each shuffle's
  redeal, and an escape stage's own number from its table). With the fix the deals and shuffles of one run draw their seeds
  from `-gameseed=N` (the page's `?seed=N`): the same series on every run, whatever time each starts. The engine's own
  reseeding when it makes an object or a stream (0x41ec10, 0x424d30) is not touched. Without a seed given nothing changes.
- **`steadyClock`.** The game's timers, frame pacing and the panel's clock run on its updates (5 ms each) rather than the
  machine's clock, so the same input plays the same game on every run, at any speed of the machine. Not under `-fast`,
  whose virtual clock does the same. The deal's search keeps its time limit on the machine's clock.

`tools/realplay/modtest.py` checks all three with the shipped exe: the menu's update count with and without the mod; the
first eight pairs of a classic deal and of its shuffle equal under one seed whatever the start time, another under another
seed, and different without the mod; the panel's clock after 400, 800 and 1200 updates in real time showing 2, 4 and 6 s.

**Fix candidates that were checked and are none.** The panel's `AddSeconds` (0x409d30) adds its seconds in milliseconds
when the clock is not running: a shuffle before the first pair added 20 s as it should (the panel showed 24 s from 2 s,
with the real time between), so no case reaches the odd branch. No layout of the 222 deals a free tile that cannot be
clicked at the start (`MJ_FLOATING=1 mahjongjapan_dealtest`: no floating tile, 0 unreachable). The shuffle's cost of 20 s
and the free-tiles option are as the original's.

**Fixed in the port itself** (a bug of the original that breaks play: `docs/MOD_ROADMAP.md`, "The fixes mod"). A click on
Shuffle while the last pair of a game is still flying (a second or so) started a shuffle whose redeal cleared that flight
(`Board::Reshuffle`, 0x40b0a0), and the flight's end is what closes the round (the `mLeft == 0` test in
`Board::Update`, 0x40bd70): the board stayed empty with the clock running and no result box. The port ignores the shuffle once no
tile is left (`Board::OnCommand`, 0x40b500). The real-input driver found it, its bot clicking Shuffle on the empty board.
It is listed in `DEVIATIONS.md`.

## Registration, links and the loader (2026-11-08)

The game has no registration code (above). The two buttons that lead out of the game were taken out: the PopCap and
SpinTop logos at the main menu's bottom corners opened their web sites (`LinkButton`, `ActivateLinks`); they remain as
pictures and no longer take a click. Nothing in the program opens a URL. The pictures are decoded by the shared libjpeg 6b
loader (`platform/ImageLoader`), as the original's own libjpeg did (0x438000), where stb_image decoded them before; the
frozen screens of `tools/check.sh` were taken again for that (the old and the new shots of the suite differ by at most 3
levels of a channel, in the JPEG backgrounds only; everything else is identical pixel for pixel).

## Real input (2026-11-08)

`tools/realplay/drive.py` plays the game from the main menu with real SDL events, posted by the framework's `-realinput`
channel under rundesk: the menu and its help, a new player's name typed, the adventure from its map level by level to the
ending, the classic list and two classic games (the first by the bot to its end, with every button of the board and of the
in-game menu tried first; the second ended by `win` after ten real pairs), the focus lost and regained, and Quit. The driver
reads the state, asks `move` and sends the input it names; it chooses nothing. The whole adventure (60 levels) ran by real
input in 23.6 minutes of real time and reached the ending. `--fullscreen` plays the same after Alt+Enter, the window going
from 3200x2400 to 1600x1200 pixels of the private desktop and back (`screenshotfull`). The web driver
`tools/web/realplay.js` plays the same through the DOM's events on the page (`?driver=`), `modtest.py` checks the three
fixes, `perfload.py` times the launch.

## Performance (2026-11-08)

The port measured on this machine (a Core i7-12700F, Windows 11), windowed, in real time, from a fresh user folder, with no
mods, under rundesk (below normal priority, SDL's silent audio), for the merge report (`build/slots/news.md`). The other
sessions' games ran meanwhile (one or two besides the measured ones), so the numbers are for comparing and for the
performance pass to start from. Each run is one process from its creation to its exit, sampled every 50 ms
(`games/bejeweled/tools/profile/perfmin.py`); the input is the minute of play the suite `tools/check.sh mahjongjapan`
sends (the menu, a new player, the adventure's first stage won by the solver's clicks, back to the map: 65 s). The
processor figures are for the window 20 s to 50 s of the process's life, in the play.

| | port (3 runs) |
|---|---|
| Launch to the main menu, real time (`tools/realplay/perfload.py`, 5 runs) | 7.8 s median (7.8 to 8.0); with the fixes mod 3.7 s (3.7 to 3.7) |
| Peak working set | 220.9 MB median (212.7 to 221.6) |
| Peak private bytes | 213.2 MB median (212.3 to 214.6) |
| Processor time in the window | 5.4 s of 29.9 s: 18 % of one core (17.9 to 21.3 %) |
| Processor time, whole run | 11.2 s median (10.0 to 12.9) of 64.8 s |
| Processes and threads | 1 process, 42 to 50 threads |

**The original was not measured.** Mahjong Escape's original exes run only inside Windows Sandbox under `tools/origrun`
(`docs/ORIGRUN.md`, its status table: a packed `bass.dll` never loads the silent stand-in, so a run on the host plays its
music on the speakers, and every capture is taken in the sandbox). Working set and processor time measured from outside
a sandbox's virtual machine would not be comparable with the port's on the host, and a host run's sound would reach the
user's speakers while they sleep, so neither was done. Its loading is known from the code instead: the original shows
the two logos for 2 s each (0x42f650), loads nothing meanwhile, and the port's launch to the menu is those 4 s of logos
plus about 3.7 s of loading and fades, 0.85 s of that before the first logo is drawn. Why the port holds this much memory
and uses this much of a core to draw 800x600 in software was not examined; both are for the performance pass
(`docs/COORDINATION.md`).
