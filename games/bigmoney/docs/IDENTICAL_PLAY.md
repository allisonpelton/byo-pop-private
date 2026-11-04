# Big Money Deluxe: the identical-play pass

This is the report of the pass that compares the port with the original exe: the game carved out of the Steam
`WinBM.exe` (`tools/extract_game_exe.py`), the same image our Ghidra export in `re/bigmoney/` was made from. It says what
was compared and how, what the pass changed, what it left alone and why, and what still differs. It has two phases
(CLAUDE.md, "Two phases"): **plays identical** (the game's state) and **looks identical** (screens with shotdiff, sound
and music, focus, the web build).

## Phase one: plays identical

### How both sides are driven

**The original** runs through `tools/origrun` on the host, invisibly under rundesk, from a copy in
`build/origrun/<run>/game` (made once by `orig.sh --prepare`, without the install's `savegame.dat` and `hiscores.dat`),
on origrun's virtual clock (`--clock virtual`): a minute of play takes five or six seconds, update for update as in real
time, and every input lands on the update it names. `--cursor scripted` answers the framework's `GetCursorPos` with the
script's mouse. The script gives the window the focus first (`focus regain`).

**The same random numbers.** Everything that shapes play comes from the board's MTRand, which the Board constructor seeds
with `rand() * 0xffff`; the CRT's `rand()` is seeded once, by `srand(GetTickCount())` in `SexyAppBase::Init`. The copy's
`srand` (0x4d9365, whose only caller is that one) is patched to store origrun's `--seed` instead of its argument (the
patch list is the git-ignored `tools/origrun/patches/bigmoney.txt`). The port's `-seed=N` is the same `srand(N)`
(`CrtSRand`), so with the same input both sides draw the same CRT numbers before each board and the same boards.

**The same settings.** The copy reads `HKLM\SOFTWARE\PopCap\BigMoney` (the 32-bit view) without registry virtualization,
and origrun is not elevated, so the original's writes there fail and every run starts from the same values (on this
machine Strategy, skill 3, three games played). `port.sh` copies that key into the port's `registry.ini`, so both sides
start with the same game type, skill, help flags and play count.

**The same input on the same updates.** The port's bot (`-autoplay`) plays a game and logs its input as a script;
`fromport.py` turns it into an origrun script, moving the title click to update 1000 (the original's title loads on a
thread that runs in real time) and everything after it by the same amount. origrun holds the original at its wait after
update K and posts the input there, so the original reads it before update K+1. `toport.py` turns origrun's log of where
each input landed into the port's script, "K+1 <command>": the port counts the update and then runs that update's
script lines, so the input reaches both before the same update (checked: a click at counter 1400 starts the board in
update 1401 on both sides). `game.sh` runs the three steps and the comparison for one game.

**By the game's state.** origrun's `--watch` logs four values of the original's board at every change, with the update:
the score (`mBoard` at app 0x52ddc8 + 0x6d0, then +0x490), the state (+0xa0), the board MTRand's index (+0x88, then
+0x9c0) and the level (+0x5b8). The index moves at every draw, so it shows the board's random stream draw for draw. In
Action it moves at every update, which would pass origrun's 4000 lines per watch, so long Action games watch the MTRand's
first state word instead, which changes each time the generator refills its 624 words. The port logs the same values
with `-statelog=<file>` (a test hook in `WinBMApp::UpdateFrames`), and `statecmp.py` compares the two, the value after
each update.

### The games compared

Ten games, each run once on each side, with the original and the port on the same seed. "Changes" counts the values each
side logged; every one matched, on the same update. The bot changes the selector from the settings' Strategy, skill 3
to the game it is asked for, so the selector's buttons were played too.

| Game | Input | To update | Result |
|---|---|---|---|
| a1 | Action, skill 1, seed 1 | 4 596 | identical: a level won; 32 score, 5 state and 3 074 RNG index changes |
| a2 | Action, skill 2, seed 7 | 57 781 | identical: four levels won, three dollar bombs, the overflow countdown and the game over, the name entry; 769 score changes, 136 refills of the RNG |
| a3 | Action, skill 3, seed 2 | 61 151 | identical: three levels won, two dollar bombs; 1 005 score changes, 155 refills |
| s1 | Strategy, skill 1, seed 3 | 4 301 | identical: a level won, the game over and the name entry; 430 RNG index changes |
| s2 | Strategy, skill 2, seed 8 | 2 698 | identical to the game over |
| s3 | Strategy, skill 3, seed 4, two games | 5 501 | identical: both games, the name entry and "play again" between them |
| p1 | Puzzle, skill 1, seed 5 | 4 872 | identical: a level won and the game over |
| p2 | Puzzle, skill 2, seed 9, two games | 7 712 | identical: a level won, both games and "play again" |
| p3 | Puzzle, skill 3, seed 6 | 3 710 | identical to the game over |
| sv1 | a3's own saved game (Action, level 4) continued, then clicks at fixed cells every 150 updates | 11 111 | identical: the load, the RNG index draw for draw to update 6 093 (where origrun's 4 000 lines ran out), three game overs and two restarts |

The CRT's `rand()` cannot be watched in the original (it lives in the main thread's CRT data on the heap), but it is
checked all the same: each new board takes its seed from it, so the second board of a two-game run (s3, p2, sv1) starts
from the same seed only when every CRT draw of the game before it (Mr. Money's idle animations, the flying coins, the
selector's voice) came in the same number and order.

Over these games the two sides dealt the same boards and rows, drew the same random numbers, scored the same points on the
same updates, dropped the same bags and bombs, levelled up and ended on the same updates in every mode and skill, so the
first phase found nothing to fix.

### Changes the pass made

None to the game. The pass added:

- the `-statelog=<file>` test hook (`WinBMApp::LogState`), which only writes a file;
- the comparison's tools in `games/bigmoney/tools/origcmp/` (`orig.sh`, `port.sh`, `fromport.py`, `toport.py`,
  `statecmp.py`, `game.sh`; each file's header has its use);
- the seed patch in the git-ignored `tools/origrun/patches/bigmoney.txt`, and Big Money's row in `docs/ORIGRUN.md`'s
  per-game status.

The bot's game and the suites (`tools/check.sh bigmoney livesplit`) give the same results as before, so the frozen
expectations stand and nothing was refrozen.

### Oddities skipped

None was found that changes play.

### Deviations that remain, and what was not compared

None was found in what was compared. What was not compared:

- The game time on the stats panel. The original takes it from `GetTickCount`, which on the virtual clock moves 10 ms per
  update; the port's `GetTickCount` is the wall clock even under `-fast`, so a fast run shows less time. In a normal run
  both are the wall clock. It is only shown, never used by play.
- Full screen and the 3D mode: both sides ran windowed in software.
- Registration and the trial: the Steam exe runs registered (`partner.xml`'s NoReg), and the port leaves those paths out
  (`GAME_NOTES.md`, 0.10). The Options dialog's "More Games" link was never clicked (CLAUDE.md).
- Real-time stretches: the original's title loads on a thread that runs in real time, so its title click comes later
  than the port's; the runs line up from that click on, and nothing before it shapes play.
- Long Action games are checked by the RNG's refills rather than its index after origrun's 4 000 lines (above).

To repeat a game: `games/bigmoney/tools/origcmp/orig.sh --prepare` once, then for example
`games/bigmoney/tools/origcmp/game.sh s3 1 3 4 60000 -autoplaygames=2` (Strategy, skill 3, seed 4, cap 60 000 updates;
`WATCH3=mt0` for a long Action game). The saved game: `ORIG_SAVE=<savegame.dat> orig.sh sv1 <script> 1`, `toport.py`,
then `PORT_SAVE=<savegame.dat> port.sh sv1 <script> 1` and `statecmp.py`. Run one original at a time.

## Phase two: looks identical

### How both sides are driven

The original runs inside Windows Sandbox (the only place origrun takes shots) on origrun's virtual clock, with
`--cursor scripted` and the input gate of the git-ignored `tools/origrun/bigmoney.cfg`, from the same settings each time:
`games/bigmoney/tools/origcmp/settings.txt` is imported into the sandbox's registry, and the port gets the same values in
its `registry.ini` (`regini.py`). A shot lands on an update origrun records (`shots/landed.tsv`); `toport.py` gives the
port the original's input on the same updates and takes the port's shot at the landed update + 1, the frame that is on
screen when the original's shot is taken. `tour.sh` runs one origrun script that way and compares four things: the state
(`statecmp.py`, as in phase one), the shots (`tools/shotdiff`), the sound and music (`soundcmp.py`), and the web build
(`webport.sh`: the same port script under Node with `wasmrun.js`, compared with the native port by its state log and its
shots). `gametour.sh` does the same for a game the port's bot plays, with a shot every N updates and at each level's end
and the game's end (`fromport.py --shots`).

Sound: the original plays its sounds through DirectSound, which origrun's stand-in logs, and its music through an older
BASS (2004), which a logging proxy (`games/bigmoney/tools/bassproxy`, after Bookworm Adventures') records, passing every
call on. The port logs both through `BYOPOP_SOUNDLOG`. `soundcmp.py` compares every sound play in order with its volume,
pan and pitch, and the music by what can be heard: which song plays from which order, its volume while it plays (each
step of every fade), pauses and stops. Both runs were silent: every RESULT line said no audio session was seen.

### What was compared

| Run | What it covers | Result |
|---|---|---|
| menus (`tours/menus.txt`) | the title while loading, finished and hovered; the selector arriving, its types, skills and side buttons hovered and clicked; Options with the music slider dragged; Scores; Credits (twice, the clouds moving); Help from Options, hovered, with the focus lost and regained; an Action board from the help: its vault, "get ready", "go"; the new-game confirmation (No); Options over the running board (a modal pause) and its Help over the board; the focus lost and regained on the board; New Game (Yes), the selector sliding over the frozen board; the quit confirmation (No) | state identical to update 3 141; 34 shots: 7 identical, 18 only faintly different, 9 with visible boxes (below); 25 sound plays and 864 music events identical |
| gs3 | Strategy, skill 3, seed 4, two games: the help, both boards, the move digits, both game overs, the high-score table and "play again" (25 shots) | state, 501 sound plays and 508 music events identical; visible boxes: the play time (below) and the move digit's zoom |
| gp2 | Puzzle, skill 2, seed 9, two games: a level won with its stats and the level-up sign, both game overs (30 shots) | state, 357 sound plays and 710 music events identical; no visible box |
| ga2 | Action, skill 2, seed 7 (the bot lost early from these settings): a board and its game over (6 shots) | state, 246 plays and 255 music events identical; visible boxes: the play time |
| ga1 | Action, skill 1, seed 1: four levels won, each with its stats panel, the "next level" button and the level-up sign, to update 41 571 (36 shots) | state, 2 553 sound plays and 1 014 music events identical; every shot identical or only faintly different |

Every one of these runs was also replayed by the web build under Node: its state log is identical to the native port's in
all five, and its shots differ from the native ones only at the rotated sprites (below).

### Every visible box, judged

- **The Options dialog's size** (menus: Options, its music slider, the board's Options, the shot after Scores, and the
  Options dialog's light column behind Scores and Credits). The original's Options dialog has a fourth row, a "More
  Games" button for the Steam partner link (`partner.xml`), which the port leaves out (`OptionsDialog.cpp`, the partner
  links; as Alchemy's port does), so the original's dialog is one row taller and its Done button sits where the port's
  More Games would be. A deviation by choice: the button opens a store page. Because of it no script presses the Options
  dialog's Done: the port's Done is where the original's More Games is. The tour leaves Options through Scores, Credits or
  Help, which sit in the same place on both sides.
- **The play time on the stats panel** (gs3, ga2). The original takes it from `GetTickCount`, which moves 10 ms an update
  on origrun's virtual clock; the port's `GetTickCount` is the wall clock (shared `framework/Common.cpp`), so a `-fast` run
  shows 0 seconds. Replayed in real time (`PORT_REALTIME=1 port.sh`), the port shows the original's times in every shot
  of gs3 and the boxes go away. It is not a difference of the game.
- **Transformed sprites sampled differently** (menus: Mr. Money in one selector shot, the selector's slide; gs3: the
  move digit's zoom and the flying coins at the board's left edge). Mr. Money is drawn rotated (`DrawImageRotated`, his
  wobble), the selector slides as a stretched snapshot, the digit zooms and the loose coins are drawn scaled and edge-on.
  In each pair the sprite sits in the same place at the same size and angle; only the edge pixels differ, as the two
  renderers sample a transformed image differently. In every other shot the same sprites came out only faintly
  different. No mask was used.

### Changes the pass made

| Difference | Cause in the original | What the port does now | Commit |
|---|---|---|---|
| Over a frozen board, and whenever the selector came back, the port faded in the selector's song (song 12, order 89); the original never plays it | `ShowGameSelector` fades it in only when `IsPlaying(title)` is false. The original's BASS (2004) answers `BASS_ChannelIsActive` with 1 for a music it played and stopped since (its log: 1 after the title's `BASS_ChannelStop`), and the title plays from the start, so the answer is always yes | keeps the original's answer: the selector's song is never faded in (`WinBMApp::ShowGameSelector`) | ed0666e |

The change touches only the music, which the frozen baseline does not hold: `tools/check.sh bigmoney` passes with its
frozen score (66 646), so nothing was refrozen. The pass also added the tools named above (`tour.sh`, `gametour.sh`,
`webport.sh`, `soundcmp.py`, `regini.py`, `settings.txt`, the bassproxy, `tours/menus.txt`), `ORIG_SANDBOX`, `ORIG_SOUND`
and `ORIG_NOCACHE` in `orig.sh`, and `PORT_SETTINGS` and `PORT_REALTIME` in `port.sh`.

### Oddities skipped

- **The "bad" sound is cut short in the original.** The original decodes its Ogg sounds with FMOD and keeps the result
  in `cached/sounds/<name>.wav`, which it loads from then on. The install's cache (made in 2018) holds only the first
  4 096 frames of `bad.ogg` (0.19 s of 0.52 s); the other 47 are whole. (A fresh decode cannot be checked under origrun:
  without a sound device FMOD writes silence for every sound, `ORIG_NOCACHE=1`.) The port plays the whole sound. "bad" is played only for an invalid
  registration code, a path the Steam build never takes and the port leaves out, so it is never heard. `soundcmp.py`
  lists it as known. To see it: compare `cached/sounds/bad.wav` in the install with `sounds/bad.ogg`. Matching it would
  mean cutting that one sound to FMOD's length.

### Deviations that remain, and what was not compared

- The Options dialog's "More Games" button (above).
- **The web build under Node places rotated sprites about a pixel off.** Its state and every other pixel match the native
  port, but Mr. Money (rotated, his wobble), the "Level up!" sign and the zoomed digits come out about one pixel from where
  the native port and the original draw them: under Node the web build draws through SDL's software renderer, whose
  rasterization of a rotated quad differs from the native renderer's. The code is shared (`framework/DDInterface.cpp`,
  SDL), so it went to the coordinator. A browser with WebGL may not take that path; it was not checked there.
- The play time under `-fast` (above).
- Full screen and the 3D mode; the registration and trial paths; the Options dialog's checkboxes for full screen and
  custom cursors were not clicked.
- Losing the focus over an open dialog was not compared (the focus was lost and regained on the help screen and on a
  running board, and both sides paused and showed the same).

To repeat phase two: `orig.sh --prepare` once, build the proxy
(`i686-w64-mingw32-clang -O2 -shared bassproxy.c bassproxy.def -o build/bassproxy-bm/bass.dll`) and the web build
(`TARGET=bigmoney tools/build.sh web`), then `games/bigmoney/tools/origcmp/tour.sh menus
games/bigmoney/tools/origcmp/tours/menus.txt` and, for a game, `gametour.sh ga1 0 1 1 40000 1500` (name, mode, skill,
seed, cap, a shot every N updates; `-autoplaygames=2` after them for two games). One sandbox runs at a time on the machine;
origrun queues for it.
