# Big Money Deluxe: the identical-play pass

This is the report of the pass that compares the port with the original exe: the game carved out of the Steam
`WinBM.exe` (`tools/extract_game_exe.py`), the same image our Ghidra export in `re/bigmoney/` was made from. It says what
was compared and how, what the pass changed, what it left alone and why, and what still differs. Only the first phase,
**plays identical**, has been done so far; the second, **looks identical** (screens with shotdiff, sound, focus, the web
build), waits until every game plays identical (CLAUDE.md, "Two phases").

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
