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

## The deviations that remain

`DEVIATIONS.md` lists every place where the port's code does something other than the original, and why. Only two of
them can be seen. Full screen on a graphics driver set to stretch keeps the 4:3 shape where the original would fill
the screen. Players keep their progress when the port moves, where the original's would start over. The rest have no
visible effect. The original's 3D test always passes. The vertical-blank waits do nothing. The software cursor is the
host's. The time and number formats are fixed to an English Windows's. Three leaks are fixed. Five bounds the original
never checks are checked. A few fields the original leaves unset start at 0. The random generator is kept draw for draw.
