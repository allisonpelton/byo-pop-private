# Mahjong Escape: Ancient China: the identical-play pass

This is the report on comparing the port with the original `MahjongEscapeAC.exe`, screen by screen and sound by sound,
finished on 2026-11-01 and merged into the trunk as a9957e6. China's port is Ancient Japan's port with every difference
between the two exes carried over, so it began with the fixes of Japan's pass (`games/mahjongjapan/docs/IDENTICAL_PLAY.md`)
already in it. China's own pass found two differences that the comparison of the exes had missed, and fixed both.
Everything else that differed is the test's own doing or animation that runs on its own clock, listed below with the
reason and how to see it.

## What was compared, and how

The method is Japan's. **The original** ran from a copy of the user's install inside Windows Sandbox through
`tools/origrun --sandbox`, windowed and in software mode. **The port** ran the same moves from a `-script` under
rundesk. `tools/sandbox_run.sh` prepares the original's copy: the game's three display settings set to 0, the
players re-signed for the sandbox's folder, and two stand-in DLLs beside the exe.

The first stand-in is new for China. In the sandbox every screenshot of China's original came back black, while
Japan's were fine. The cause is in China's `WinMain` (0x4295a0), which registers its window class with a black
background brush; Japan's has none. In the sandbox the software display cannot draw into the window the way a capture
reads it, and the brush paints over what the capture sees. `tools/ddrawcap` is a `DDRAW.dll` that forwards to the real
one and clears the brush when the game first creates DirectDraw. It changes nothing the game draws. The second
stand-in is Japan's BASS proxy, which logs every sound call.

**The levels.** The clicks came from the port's solver (`mahjongchina_dealtest --stage n`). Level 1 was played from a
new player at progress 0. Levels 2, 4, 8, 13 and 52 with the ending were each played from a player saved just before
them (`tools/resign.py --progress`). Each run shot the map, the level's start tip, the board, each timed tip, the
board half way, the level's end and the map after it; level 52 also shot the ending's screens. `tools/stagerun.py`
writes both sides' scripts, and `tools/shotdiff` compared the shots. Every flagged box was looked at.

**The menus and dialogs.** `tools/tour.py` took the same 18 screens as Japan's tour: the main menu, the options, the
four pages of instructions, the player list, the classic list and its second page, a classic game with its tip,
losing and regaining the focus, pause and the in-game menu.

**The sound.** The BASS proxy's log of the original was lined up with the port's `MJ_SOUNDLOG` by
`tools/soundlog.py`, as for Japan. That covered the tour (78 events) and levels 1, 2, 4, 8, 13 and 52 with the ending
(198 to 389 events each). The level runs of the port played in real time, because under `-fast` the game runs ahead
of the audio device.

**The focus.** The tour took the focus away during a classic game and gave it back. The music paused and resumed as
in the original, and the screens matched.

**The whole adventure.** The adventure was played natively to the ending in two parts. Levels 1 to 10 came from a new
player, and levels 12 to 52 from a player at progress 11. Every level was won. The solver of that pass found no
solution for level 11, which has six keys and eight locks; the finishing pass's bot solves it (below).

**The web build.** Under Node it played the suite's first level to the same screens as the native build.

## The changes the pass made

| What differed | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| The board stood 5 pixels higher than the original's | Japan's Board::Measure (0x40b690) centres the board and then moves it up 5 pixels. China's (0x40b650) does not move it | The -5 is gone, in the port and in the solver's click points. The boards now match to the pixel | 9bd95f9 |
| Board::Init loaded the board's ten sounds in another order | China's Board::Init (0x40a940) loads sounds 0x17b and 0x15e in other places than Japan's (0x40aa70). The constants are the same, so a comparison of the two exes' constants as sets cannot see it | The original's order (0x160, 0x15b, 0x15c, 0x17a, 0x17b, 0x15d, 0x15e, 0x15f, 0x179, 0x178). Nothing is heard differently, but the loads now match the original's log | 9bd95f9 |

Both slipped through the first comparison of the exes. The -5 was in its list of constants but was overlooked. The
sound order was invisible to it. Afterwards every constant the comparison had flagged was checked again. The rest are
the classes' fields a word lower, allocation sizes or engine calls the compiler inlined. `re/mahjongchina/order.py`
now looks for constants that appear in another order, and the sound loads were the only case.

## Oddities that were not fixed, and why

Each of these differs between the runs but not between the games.

**The clock.** The original plays on the wall clock and the port's scripted runs on the framework's virtual clock. So
the time shown on the panel and at the level's end differs. So do the clock-seeded deal of a classic game, the layout
previews and the counts that follow them.

**Animation on its own clock.** These move with time, not with the moves, so a screenshot catches them at another
frame:
- the tiles a tip lights;
- the bouncing arrow of the How To Play tip;
- the tassels on the level-end dialog's dragons;
- the map's pulsing marker;
- the main menu's two figures.

Leave any of those screens standing in either game to see it move.

**The PAUSE button lit on the port.** The original reads the cursor with `GetCursorPos` to light a button, and
origrun's posted clicks do not move the sandbox's cursor. Move the real mouse over PAUSE in the original and it lights
as in the port.

**The music's jumps land at different moments.** A jump to a new order of the music happens at the next row that is a
multiple of four. So the jump's place among the samples around it moves with the timing; on level 4, for one, the
jump came before a sample in one game and after it in the other.

**Samples the proxy kept playing.** With no sound device in the sandbox, the proxy answers for the samples as playing
until stopped. So the original pauses and resumes samples that had really ended, at a tip or a level's end.

**The map's sound when a stage's name fades.** After level 52 the port played the map's sound for the faded stage name
(0x17e) and the original did not. The original plays it only when the fade passes below 0 (a `JNS` at 0x40fd9e), and
skips it when a frame's step lands exactly on 0. That depends on the frame times, and the port keeps the same test.

**The full-screen corner button.** A run with no `registry.ini` starts with the original's default of full screen on.
The native build then shows the button that leaves full screen in the corner, and the web build, which always starts
in the page, does not. With the same settings the two builds' screens are identical.

**Not reached by a script.** The restart from the second button of a classic game's end dialog was not played. The
in-game menu of a classic game has no restart, so a scripted tour cannot reach it. Level 11 was not compared with the
original in that pass, because its solver had no solution for it; the finishing pass plays it with the port's bot, and
the comparison with the original was not repeated.

## The finishing pass: real play and the fixes mod (2026-11-08)

This pass is Ancient Japan's, done once for both games (`games/mahjongjapan/docs/IDENTICAL_PLAY.md`, "The finishing pass"
describes the method and the findings; the code is the same files). A driver that is code (`tools/realplay/drive.py` of
Ancient Japan's folder, `--game china`) reads the state from the standard test commands, asks `move` for the port's own bot's
next pair and sends the input it names as real SDL events under rundesk; `tools/web/realplay.js` does the same on the page.

**What was played.** Natively, windowed: the launch, the help pages, a new player's name typed, the whole of Dynasty
Adventure from level 1 to the ending (52 levels won, 3055 pairs, at most 105 in a level and no shuffle needed, in 16.0 minutes
of real time), the classic list, a classic game by the bot to its result box (72 pairs, layout 437) after every button of the
board and the in-game menu was tried before its first pair, a second classic game ended by `win` after ten real pairs, the focus
lost and regained, and Quit (exit code 0). In full screen (Alt+Enter, the window going from 3200x2400 to 1600x1200 pixels of the
private desktop and back, read from `screenshotfull`): the first levels of the adventure, the classic games and Quit. On the web (the page in the Browser pane with `?player=1&silent=1&assets=1&fast=1`, `tools/web/realplay.js` asking `move` for the
bot's pair and sending its `input` steps as the DOM's mouse, key, text and focus events through the test kit's `playMove`):
the same play in one run with no error: the focus lost and
regained at the menu, at the first level's pause and in the classic games, the whole of Dynasty Adventure (52 levels won,
3057 pairs, at most 105 in a level), the classic list, a classic game by the bot to its result box (72 pairs) after the
board's buttons were tried before its first pair, and a second classic game ended by `win` after ten real pairs. The pane
cannot enter full screen, so `?scale=2` (the canvas drawn at 1600x1200 CSS pixels over its 800x600) was played in its
place: the first three levels and the two classic games, no error; the real full screen was tested natively only.

**What it found for China.** Level 11 (layout 395, six keys and eight locks), which the pass of 2026-11-01 skipped because its solver
found no solution, is solved by the bot (38 moves, 17070 points) and was played by real input with the rest. The soft lock of
Ancient Japan's pass is China's too: a click on Shuffle while the last pair flies cleared that flight in the redeal, so the
round never closed; `Board::OnCommand` ignores the shuffle once no tile is left (`DEVIATIONS.md`).

**What this pass changed in the port.** The same changes as Japan's, in the same commit (3edab79b): the shuffle at the last pair, the
logos at the main menu's corners taking no click, `move` answering the pause box, the shared libjpeg 6b loader (the suite's
screens at most 3 levels of a channel from the old ones, in the JPEG backgrounds; `tools/check.sh` has the new hashes) and the
hooks of the fixes mod; a later commit of the pass took the two logo screens and the black before them out of the start
(the loading rule; `-logos` keeps them, the check suite passes it), and the launch to the menu takes 1.29 s where it took 7.7 s.
`modtest.py --game china` passes: the menu at update 260 as the port starts against 1550 with `-logos`, the same seed dealing the same board and shuffle whatever the start time and another seed another, the clock's seconds
equal to the updates in real time. Oddities of this pass: none skipped.

## The deviations that remain

`DEVIATIONS.md` lists them. They are Japan's, with China's addresses, plus the window's background brush, which the
port has no window class to carry and which nothing shows. Five of them can be seen. Full screen on a graphics driver
set to stretch keeps the 4:3 shape. A player whose port moved keeps its progress. The two logos at the main menu's
corners take no click, where the original opened web sites. A click on Shuffle while the last pair flies does nothing,
where the original locked the board. The game opens at its main menu, where the original showed two logo screens for 4 s
(`-logos` keeps them). The rest have no visible effect: the
device stand-ins, three leaks fixed, five bounds the original never checks, a few fields the original leaves unset,
and the random generator kept draw for draw.
