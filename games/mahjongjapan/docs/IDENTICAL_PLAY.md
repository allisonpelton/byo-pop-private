# Mahjong Escape: Ancient Japan: the identical-play pass

This is the report on comparing the port with the original `MahjongEscapeAJ.exe`, screen by screen and sound by sound,
natively and in the browser, finished on 2026-11-01 and merged into the trunk as 9a71788. The result is that the port
plays like the original. Four changes came out of the pass, listed below; everything else that differed in the
comparison is either the test's own doing or animation that runs on its own clock, and each of those is listed with the
reason and how to see it.

## What was compared, and how

**The original** ran from a copy of the user's install, never the install itself, inside Windows Sandbox through
`tools/origrun --sandbox`, windowed and in software mode (`FullScreen`, `HardwareAcceleration` and `WideScreen` 0 in
the game's registry key), with origrun posting the clicks and taking the screenshots. **The port** ran the same moves
from a `-script`, natively under rundesk and in the web build under Node. Both sides started each run from the same
player: the port's saved player, re-signed for the sandbox's folder (the player file's checksum covers the exe's
folder). The clicks of each stage came from the port's solver (`mahjongjapan_dealtest --stage n`), which wins a stage
with its special tiles. The screenshots were taken at points where the screen stands still and compared by
`tools/shotdiff`, which flags every box of pixels that differs; each flagged box was looked at and judged.

**The stages.** Stage 1 from a new player (the name dialog, the map, the board, the How To Play tip, the tray, the
score, the level's end and the map again), then stages 2, 3, 6, 7, 10, 13, 19, 25, 31, 37, 43, 49, 55 and 60 with
the ending (the map with its treasures, then the showcase), each from the player saved before it. Each run shot the
map, the stage's tip, the board, each timed tip, the board half way, the level's end and the map after it.

**The menus and dialogs.** A tour from a saved player: the main menu, the options, the four pages of instructions,
the player list, the classic list and its second page, a classic game with its tip, losing and regaining the focus,
pause and the in-game menu, 18 screens in all.

**The sound.** The game plays through BASS, which origrun's DirectSound stand-in cannot hear (the packed `bass.dll`
loads DirectSound for itself), and Windows Sandbox has no sound device. So a logging proxy (`tools/bassproxy`, a
`bass.dll` beside the copy that forwards to the real one, renamed `bass_real.dll`) wrote every BASS call to a file,
and the port wrote the same events to the file named by `MJ_SOUNDLOG`. `tools/soundlog.py` matched the original's
samples and music to the port's resource ids by the CRC of their data and lined the two logs up: every load, play,
stop, pause, resume and volume of a sample, and the music's loads, plays, fades, stops, pauses and jumps to an order.
The tour (78 events) and stages 2, 6, 10 and 60 with the ending (166 to 402 events each) were compared.

**The focus.** The tour took the focus away and gave it back during a classic game: both sides paused the music
with the focus and resumed it on its return, and the screens before, during and after were the same.

**The web build.** The whole 60-stage adventure under Node took 121 screenshots identical to the native run's, and
the tour's 18 shots were identical too. In the Browser pane a new player played stage 1 to the level's end with the
native score (6,420), and was still there after a reload.

**Full screen.** A native run started in full screen with "Wide Screen" on and reached stage 1's board as the
windowed runs do.

## The changes the pass made

| What differed | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| The port kept its settings under another registry key, and at the exit the framework's own MusicVolume overwrote the engine's, undoing a change made in the options | The engine names its key `Software\JollyBear\` plus the title with only its letters, digits and spaces (0x429ac0), so `Mahjong Escape Ancient Japan`, and it alone reads (0x426060) and writes (0x4266c0) its settings there | The original's key, with its spaces, and the framework's settings are kept out of it, so only the engine's values are written | 8910078 |
| A player imported from the original, or kept by a port that moved, started over | A player file ends in a checksum that also covers the exe's folder (0x416100), and the port's folder is not the original's | The port signs with the original's usual install folder and loads players signed with it, with its 32-bit Windows twin or with the port's own folder (`DEVIATIONS.md`, "The players' checksum") | 4b6c9fe |
| The web build tried to start in full screen and could not | The engine reads `FullScreen` (on for a new install), and a browser enters full screen only from a click | The web host writes the setting off before the game reads it, as every game's web build starts in the page; the page's button and the options switch it later | 02ad4a8 |
| Full screen fills a wide screen differently | The original switches the display to 800x600, unless "Wide Screen" is on and the desktop wide enough, and then the graphics driver decides the shape | No code change: the framework always keeps the desktop's resolution and shows the 4:3 screen centred, which is the original's Wide Screen look and what most drivers show; the setting is kept and saved (`DEVIATIONS.md`, "The device") | af8d0cb |

The sound comparison found no difference to fix; the logging it added (`MJ_SOUNDLOG`, a24bef7) writes nothing unless
the variable is set.

## Oddities that were not fixed, and why

Each of these differs between the runs but not between the games.

**The clock.** The original plays on the wall clock and the port's scripted runs on the framework's virtual clock,
so the time shown on the panel and at the level's end differs, and so does every count that follows the clock-seeded
deal of a classic game and the previews of the layouts list. Play a classic game twice in the original to see the
deal change.

**Animation on its own clock.** The tiles a tip lights, the arrow under the free-tiles tip, the map's marker, the
main menu's swinging lanterns and the ending's next line move by time, not by the moves, so a shot catches them at
another frame. Leave any of those screens standing in either game and watch them move.

**The PAUSE button lit on the port.** The original tests the hover with `GetCursorPos` (0x42a820), and origrun's
posted clicks do not move the sandbox's cursor, so the original's button never lights in a test run; the port's does,
under the clicks. Move the real mouse over PAUSE in the original and it lights the same way.

**The music's jumps land at different moments.** The game asks the music to jump to an order at the next row that is
a multiple of the step (0x437300), so the jump falls 46 to 330 ms after the request in the port and 239 to 416 ms in
the original, and its place among the samples played around it moves. Under the port's `-fast` clock the game can
even ask twice before the row comes, so the sound comparisons ran the port in real time.

**Samples the proxy kept playing.** With no sound device, BASS loads no sample, so the proxy answers for the samples
itself, as playing until stopped. The original then pauses and resumes samples that had really ended, at a tip or a
level's end. Nothing is heard either way.

**The map's sound when a stage's name fades.** The map plays a sound (0x17e) when the new stage's name has faded out,
but only when the fade passes below 0; when a frame's step lands exactly on 0 the sound is skipped. That depends on
the frame times, in the original and in the port alike, so a run may have it on one side only.

## The finishing pass: real play and the fixes mod (2026-11-08)

The earlier passes compared the port with the original; this one plays the port the way a player does and finishes what
a player meets. Its driver is code, not a session at the keyboard: `tools/realplay/drive.py` reads the game's state from the
standard test commands (`GAME_NOTES.md`, "The standard test commands"), asks `move` for the next pair of the port's own bot
and sends the input it names as real SDL events through `-realinput`, under rundesk (a private desktop, silent, below normal
priority). The web driver, `tools/web/realplay.js`, does the same on the page through the DOM's mouse, key, text and focus
events. Neither chooses a move.

**What was played.** Natively, windowed: the launch, the help pages, a new player's name typed, the whole of Emperor's
Adventure from level 1 to the ending (60 levels won, 4456 pairs, at most 132 in a level and no shuffle needed, in 23.6 minutes of
real time), the classic
list, a classic game by the bot to its result box (72 pairs) after every button of the board and the in-game menu was tried
before its first pair (the free-tiles option, hint, shuffle, pause, and the menu's options, tile sets, backgrounds and help),
a second classic game ended by `win` after ten real pairs, the focus lost and regained at the menu, and Quit (exit code 0).
Natively in full screen (Alt+Enter, the window going from 3200x2400 to 1600x1200 pixels of the private desktop and back,
read from `screenshotfull`): the first levels of the adventure, the classic games, and Quit. On the web (the page in the Browser pane with `?player=1&silent=1&assets=1&fast=1`, the driver `tools/web/realplay.js` asking
`move` for the bot's pair and sending its `input` steps as the DOM's mouse, key, text and focus events through the test kit's
`playMove`): the same play in one run with no error: the
focus lost and regained at the menu, at the first level's pause and in the classic games, the whole of Emperor's Adventure
(60 levels won, 4441 pairs, at most 140 in a level), the classic list, a classic game by the bot to its result box (72 pairs,
layout 437) after the board's buttons were tried before its first pair, and a second classic game ended by `win` after ten
real pairs. The pane cannot enter full screen, so `?scale=2` (the canvas drawn at 1600x1200 CSS pixels over its 800x600)
was played in its place: the first three levels and the two classic games, no error; the real full screen was tested
natively only.

**What it found.** One bug of the original, fixed in the port: a click on Shuffle while the last pair of a game still flies
cleared that flight in the redeal, so the round never closed and the board sat empty (`DEVIATIONS.md`, "A soft lock the
original has"; the driver's bot clicked Shuffle on the empty board, and a second of play showed it). In the shared code, the
test kit's pattern for an answer never matched `MOVE null`, so `move` timed out on every dialog without a move (the pause
box here); `web/testkit.js` has the fix, in a shared commit. And a note: on the page a press within 150 ms of the focus's
return is the one that activates the page and is not passed on (the framework's rule, as natively), and a fast run makes
many updates in 150 ms, so the driver lets real time pass after it regains the focus.

**What this pass changed in the port.** The commit is 3edab79b on the finishing branch (the merge carries it).

| Change | Cause in the original | What the port does now |
|---|---|---|
| Shuffle at the last pair no longer locks the board | `Board::Reshuffle` clears the flights, and the last pair's flight closes the round | `Board::OnCommand` ignores the shuffle once no tile is left |
| The logos at the main menu's corners take no click | They were link buttons that opened web sites | Pictures only; nothing opens a URL |
| `move` answers the pause box with a click on it | Not applicable (a test command) | The pause box has no button, so the answer is a click at its middle |
| JPEG backgrounds decode as the original's libjpeg does | stb_image decoded them before | The shared loader, libjpeg 6b: at most 3 levels of a channel of difference in the JPEG backgrounds, so the suite's frozen screens were taken again (`tools/check.sh`) |
| The game opens at its main menu | Two logo screens of 2 s each after 0.77 s of black, and nothing loaded meanwhile (a wait the loading rule takes out) | The first layer is the menu and the first screen has no fade-out; `-logos` keeps the original's start (the check suite passes it). The launch to the menu takes 1.3 s where it took 7.8 s (a later commit of the same pass) |
| The fixes mod's hooks | The clock is read at two places (the deal search's limit, the deals' seeds) | `Host::RealTickCount` and `Host::SeedTick`, answering as the original without the mod |

**The fixes mod** (`mods/fixes`: `seededDeals`, `steadyClock`) makes what the original leaves to chance
(`GAME_NOTES.md`, "Fix candidates and the fixes mod") and nothing else. `tools/realplay/modtest.py` checks each with the shipped
exe, and the start (the menu at update 280 as the port starts against 1540 with `-logos`): the same seed dealing the same board and shuffle whatever the start
time and another seed another, the clock's seconds equal to the updates in real time. With no mods folder the game is the
original: the suite's game makes the same 20 pairs and its rerun is identical, and its screens differ from the frozen ones only by the
JPEG decode above (at most 3 levels of a channel, checked pixel by pixel against the main checkout's older exe). Oddities of this pass: none skipped. Candidates checked and found to be none are in the notes (`AddSeconds`,
free tiles that cannot be clicked).

## The deviations that remain

`DEVIATIONS.md` lists every place where the port's code does something other than the original, and why. Five of
them can be seen. Full screen on a graphics driver set to stretch keeps the 4:3 shape where the original would fill
the screen. Players keep their progress when the port moves, where the original's would start over. The two logos at
the main menu's corners take no click, where the original opened web sites. A click on Shuffle while the last pair
flies does nothing, where the original locked the board. The game opens at its main menu, where the original showed two
logo screens for 4 s (`-logos` keeps them). The rest have no visible effect (the JPEG backgrounds are
decoded by libjpeg 6b, the original's own library, which is not a deviation). The original's 3D test always passes. The vertical-blank waits do nothing. The software cursor is the
host's. The time and number formats are fixed to an English Windows's. Three leaks are fixed. Five bounds the original
never checks are checked. A few fields the original leaves unset start at 0. The random generator is kept draw for draw.
