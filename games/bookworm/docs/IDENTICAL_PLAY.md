# BookWorm Deluxe: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. The same input gives the same games in the original and the
port, by the game's state, not by pictures. Phase two (screens, sound, focus, the web build) follows under "Phase 2".

The original is the retail BookWorm Deluxe 1.03 (the version the port was decompiled from; the Steam copy wraps a later build
and refuses to start outside Steam), the game carved out of its `Bookworm.exe` (`re/bookworm/bin/BookwormGame.exe`). It runs
only through `tools/origrun`, from a copy under a short path (`C:\byo-og\bw\game`, staged by `setup.sh`), under rundesk
(invisible, silent, below normal priority), on origrun's virtual clock (`--clock virtual`: the game is held between updates, so
an input lands on an exact update) and with the scripted cursor (`--cursor scripted`). One original and one port at a time at
most; no shot is taken.

## How both sides are driven

The port's bot (`testhooks/Autoplay.cpp`, `-autoplay`) plays a session: the new-player dialog, the mode, every word and
scramble, the level summaries, the game over, the Hall of Fame, and the next game when `-autoplaymode` lists more than one
(`classic,action,classic`, or `both`). It writes its input as a `-script` file. `case.sh` shifts that input 600 updates later
(the title's loading runs on a thread at the machine's speed: the original unpacks its sounds until about update 130 under
origrun, the port is done at 23, so the play link is clicked once both surely are), replays it in the port with `-fast` and
`-statelog`, and turns it into an origrun script (`toorig.py`: a port script's line at update K is origrun's `at K-1`). The
original runs on that script with the same seed, watching four values per run (origrun's `--watch` limit), so a case runs it
four times. `cmp.py` compares each value's series of changes, update by update, to the last update both reach.

The sixteen values (`values.txt` gives each one's address in the original; the port's `-statelog` writes the same names): the
score, the board's state, the level, the game generator's index and first state word (`gGameRand`, the letters), the board
generator's index (`Board::mRand`: special tiles, fire tiles, praise), the words and letters played, the fire tiles on the
board, the Action fire timer, the bonus words given, the fire meter, the level's score, the tiles burned, the game mode and
Lex's praise flag. Every letter drawn moves one of the two generators, so a single draw in another order or another update
shows.

Three patches make the original comparable (`setup.sh` writes them to origrun's patch list, git-ignored):

- **The seed.** Both `GetTickCount()` calls in `SexyApp::Init` (0x48adbe, which seeds the framework's generator, and 0x48affe,
  which seeds the C library's) return the run's seed, as the port's `-seed=N` does.
- **The praise flag.** The Board constructor never sets `mLexPraisePending` (Oddities, below); a store there is widened so
  that it clears the flag, as the port's constructor does.
- **The registry.** The original keeps its settings, players and high scores under `HKLM\SOFTWARE\PopCap\BookWorm`, which
  this machine's user can read but not write: it started as the machine's last player, with the machine's tables, and kept
  nothing. Its twenty `RegOpenKeyEx`/`RegCreateKeyEx` calls are pointed at `HKEY_CURRENT_USER` instead, the key `orig.sh`
  clears before each run (exporting it first to `build/identical/regbackup/`), so it starts as a first run does and keeps what
  it writes, as the port does in its own user folder.

`save.sh` covers a saved game. The bot plays a mode and quits at a given update, mid-game, so each program saves the game as
it shuts down (`players/<name>/classic.dat` or `action.dat`). Both files must be the same, byte for byte, except for the
fields the original never initialises and the game log's start time (`savecmp.py`). Then the bot continues the saved game
(`-autoplaycontinue=1`), and its input is replayed twice: by the port from the original's file, and by the original from
the port's. Their values are compared as above. After every session that ends in a game over, `hiscore.py` compares the four
high-score tables (Classic, Action, best word, longest word) the port left in its registry file with those the original left
in HKCU.

## Phase 1 results

Every value was the same, update for update, in every case, once the two fixes below were in. Every case matched completely
in at least one run of the original. Some other runs differed only where the original's Lex fidgeted at another moment
(Oddities, below):

| Case | Input | Games | Compared to update |
| --- | --- | --- | --- |
| c1 | Classic, bot 1, seed 1 | one Classic game | 30,000 |
| c2 | both modes, bot 2, seed 7 | Classic, then Action, to the Hall of Fame each time | 30,156 |
| m1 | `classic,action,classic`, bot 7, seed 9 | three games in one session | 22,325 |
| s1 | Classic quit at 3,000 (bot 3, seed 11), continued by bot 4 | one game across two sessions | 4,402 |
| s2 | Action quit at 4,000 (bot 5, seed 13), continued by bot 6 | one game across two sessions | 15,587 |

Within those, c2 alone made 161 score changes, 9,133 board-generator draws and 12,744 changes of the Action fire timer; m1
played 107 words in three games. The high-score tables were the same after m1, s1 and s2 (the bot's new entries among the
default names, its best and longest words). Both saved games were the same files apart from the original's garbage and the
time, and each program continued the other's game to its end with the same state as continuing its own.

## What was changed

| Difference | Cause in the original | The port now | Commit |
| --- | --- | --- | --- |
| Every game's letters differed from the original's on the same seed. | The 2004 framework's `SRand` seeds its generator the SAF 1.2 way (x69069); the port's framework seeds the newer way. The game generator takes its seed from that generator's first draw, so every letter depended on it. | `BookWormApp::Init` reseeds the framework generator the 2004 way (`SeedMTRand2004`, and the new shared setter `SetRandMTState`). | e8c0ce5 (shared), 4a2cb5a |
| The Board constructor's comment said the praise flag is set before it is read. | It is read every update once Lex rests (0x40ebc6). | The comment says so and points to the oddity below; the code (clearing the flag) is unchanged. | 4a2cb5a |

The fix changes how the port plays, so `tools/check.sh bookworm`'s expected bot results moved (Classic now ends at level 5 with
19,230, Action at level 11 with 87,580; 84eb801). What the pass added besides: `-statelog` (`testhooks/StateLog`), the bot's list
of games, and `games/bookworm/tools/identical/` (`setup.sh`, `case.sh`, `save.sh`, `port.sh`, `orig.sh`, `toorig.py`,
`cmp.py`, `savecmp.py`, `hiscore.py`, `values.txt`). No script uses the private exe extractor.

## Frozen baseline

`build/baseline-bookworm/` (git-ignored: a copy of `bookworm.exe` and its DLLs, built at 84eb801) is checked by `tools/check.sh
bookworm`. Four bot runs with no mods (the two-game session c2, the three-game session m1, and s1's Classic game quit at
update 3,000 and continued from its file) must write the same non-empty state log in the current port and in the baseline.
That way later fixes and mods are shown to leave unmodded play as it was compared with the original.

## Phase 2: screens

The original takes shots only inside Windows Sandbox (`origrun --sandbox`), so each tour runs there on a fresh copy and a
fresh registry, on the virtual clock with the scripted cursor; the port runs the same tour under rundesk with `-fast`.
The tools are in `tools/shots/`: a tour is a list of `<update> <command>` lines, with `@port` or `@orig` on a line one side
alone needs (`make.py` turns it into the port's `-script` or origrun's script). The original's loading thread runs in
real time while its clock races, so its script waits 60 real seconds, marks, and times every later line from the mark.
`fromlog.py` turns a bot game's log into a tour, with a shot after every tenth word, at every level summary, at each
dialog, at the game over and at the Hall of Fame. Every click a script sends the original was checked against that run's
own shot before it was trusted (one early menu click landed on the original's "More Games" button, which the port no
longer has; it opened only an "Open Browser" box in the sandbox and was moved off it). `tools/shotdiff` compares the
pictures; every box it flags was looked at and judged.

| Tour | Pairs | Same | Faint | Flagged, and why |
|---|---|---|---|---|
| menus (title, new player, selector, help pages, Hall of Fame, options, credits, players, quit and leave questions) | 37 | 33 | 0 | 4: the options and credits dialogs, which in the original have one more row for the "More Games" button (taken out with the registration code) |
| Classic (seed 7: 59 words, 7 levels, 3 dialogs, game over, Hall of Fame) | 17 | 2 | 9 | 6: flying word tiles (below) |
| Action (seed 8: 112 words, 12 levels, 8 dialogs, game over, Hall of Fame) | 33 | 9 | 13 | 11: flying word tiles (below) |

**Flying word tiles.** A submitted word's tiles fly to Lex shrinking, drawn by a stretched blit with fast stretch on
(`FlyingPieceList::Draw`). The sandbox has no 3D, so the original runs in its software mode (its Hardware Acceleration box
is unchecked there), and its software stretch (`DDImage::StretchBlt` at 0x45e200) steps its 16.16 source position before
it reads, so the first column it draws is the source's second and the last is the next letter's first. It also comes out
two levels darker. The port draws every frame on the GPU, as the original's 3D mode does, and samples texel centres. The
tiles are in the same place and of the same size on both sides; only which source column lands on which pixel differs.
The framework's software `MemoryImage::FastStretchBlt` already samples as the original's does, so a port drawn in
software would match the sandbox. Kept: the port matches the original's 3D mode, which is what the game uses on a machine
with a graphics card.

**The web build** ran the menu and Classic tours under Node (`tools/shots/web.sh`, `webcmp.sh`) and was compared with the
native port's shots: 33 of 37 menu shots identical, 2 faint, and 2 differing only in the options dialog's Hardware
Acceleration box, which is unchecked under Node (no 3D there) as it is in the sandbox's original; all 17 Classic shots
identical or faint (the flying tiles again, a level or two apart).

## Phase 2: sound

The original's samples go through DirectSound, which origrun's stand-in serves and records (`--sound wav`: each play is
a duplicate of a loaded buffer, logged with its virtual time, volume and pan); its music goes through BASS, whose calls a
logging `bass.dll` records (Big Money's `tools/bassproxy`, built into `build/bassproxy-bw`). Both runs are on the host
(`tools/shots/sound_orig.sh`), since origrun records sound only there; every run's RESULT line says no sound reached the
speakers. The port writes its own log (`BYOPOP_SOUNDLOG`, `sound_port.sh`). `soundcmp.py` names each play by its
buffer's size and compares them in order; `musiccmp.py` compares each song's volume over time, which is what can be
heard of the music (the original sets the volume on every update of a fade, so repeated values are dropped).

| Tour | Sample plays | Music |
|---|---|---|
| Classic (seed 7) | 1,166 on each side, all the same sample, volume and pan on the same update | 603 volume changes the same, on the same update, to the game over |
| Action (seed 8) | 2,409 on each side, all the same, on the same update | 603 volume changes the same, on the same update, to the game over |

From the game over on, the original's music differs: under the game-over jingle the mode's own song fades back in from
silence, and fades out with the jingle when the Hall of Fame closes; in the port it stays stopped. It is listed under
"Oddities".

## Phase 2: focus

Two tours (`focusc.txt`, `focusa.txt`) play to the tenth word of the Classic and Action tours, then take the focus away
for 50 updates (origrun's `focus lose`, the port's script `focus lose`), take shots while it is away, give it back and
shoot again. Both sides pause the same way: the board's tiles turn blank, Action shows its "Game Paused" panel, the clock
stands, and play goes on from the same update once the focus is back. These tours compared pictures; the sound while
the focus is away (the original's `LostFocus` pauses the music, and the framework mutes an inactive window) was not
recorded on them. All twelve focus shots are identical or faint; the other
boxes flagged on those tours are the flying tiles above. One run of the original reached the first Action summary late,
the run-to-run fidget of Lex under "Oddities"; run again, it matched.

## Phase 2: real input

The port's own driver plays through the real input path: `tools/playbymove` sends each move the port's word solver
proposes (the test command `move`) as posted mouse clicks and key presses to the window under rundesk, reading the game's
state from its log. Natively it played, by real input only: Classic and Action each for 4,000 moves (levels 55 and 62,
no move refused, no stall), and a whole game of each from launch: the new-player dialog (typed), the help screen, 40
words with their level summaries, the loss (`lose 40`, the game's own fire scan), the burn-down, the game-over summary,
the Hall of Fame and the selector, then a second game.

## The fixes mod

`games/bookworm/mods/fixes` has one toggle, `scoreGlow`: the original draws a heated version of the score while points
are added, but truncates its alpha to 0, so it never shows (GAME_NOTES, "The fixes mod", with the candidates left out
and why). With the toggle on the score glows; play, sound and every other picture stay as they were.

## Oddities (skipped)

- **The praise flag starts as the heap left it.** The original's Board constructor never sets `mLexPraisePending` (+0x358), so
  the first board of a session starts with leftover memory (nonzero in every run seen: 0xFF and 0x7E), and a later board with
  what the board before it left. A nonzero flag costs one draw of the board's generator, with no praise, the first time Lex
  rests (seen at update 1,053), and the timings after it near the end of Classic moved by about 20 updates. How to see it: run
  without `setup.sh`'s patch at 0x40b021 and watch `[[0x5347d4]+0x1160]+0x358`. The port starts it false; the patch makes the
  original do the same. The patch clears the flag and two of the three padding bytes after it, so `cmp.py` compares the
  flag's byte alone.
- **Lex's random fidgets differ between runs of the original.** Lex picks his idle animations (a blink, a look around, a
  blink on a burnt board) from the C library's `rand()`, two or three draws per update (`Lex::Update`). On the same script and
  seed, some runs of the original fidget at other moments than others, and the port fidgets as some runs of the original do.
  A fidget makes Lex busy for a few updates, and three things wait for him: Lex's praise for a special tile, which draws from
  the board's generator (seen moved from update 8,514 to 8,521 after a scramble in s2), and the game-over delays that count only while
  Lex is calm (the 160-update music delay was seen 12 updates off at game over in c1 and s1). Nothing else moves: the letters, the scores,
  the levels and the generators' other draws stay the same. In c1, two of five runs of the original matched the port
  completely and three did not. How it was narrowed: watching Lex's animation, step, target and position
  (`[[0x5347d4]+0x1160]+0x350`, then +0xb4, +0xb8, +0x80/+0x84 and the doubles at +0x88/+0x90) showed the runs part at a
  fidget (animation 27 at update 5,326 in one run, none in another), and watching the music delay (board +0x318) showed it
  held while the port's Lex fidgeted. The original's `rand()` keeps its state per thread (0x4dc094 reads it through
  `_getptd`), and its only other caller (0x48a4a4) names a temporary HTML file when a URL is opened, which these runs never
  do. So what consumes or reseeds the main thread's draws in some runs is not found. Finding it would mean logging every
  call of 0x4dc094 in the original, with its thread and return address. A case that fails only by such a shift is run again
  before anything is concluded.
- **Uninitialised fields in the saved game.** The original writes `mUnused1b2` (a byte nothing reads; a test with the port's
  0 against the original's 0xFF gave the same game) and the sapphire and diamond counters of both StatKeepers (never zeroed,
  only incremented, never shown) as the heap left them. The port writes zeros and its own counts. `savecmp.py` skips them.
- **The game log's start time.** The record ends with the game's log, which begins with `time(NULL)`, the wall clock.
  `savecmp.py` skips those 32 bits.
- **The last word's colour is set by drawing.** `Board::DrawWord` stores the colour of the last word in a field the saved
  game holds, so a port that skips draws (`-fast` draws every tenth update) can save the colour of an earlier word. `save.sh`'s
  first session runs the port with `-drawevery`. Nothing shows the stored colour: the next draw sets it again.
- **The original still reads three counters from HKLM.** With the registry patched, its first run wrote `TimesExecuted`,
  `TimesPlayed` and the version-check time into HKCU with HKLM's values, so some helper still reads the machine key. The players,
  the settings that matter and the tables come from HKCU, as the new-player dialog and the tables showed. Nothing the comparison
  watches depends on those counters.

- **The mode's song comes back under the game-over jingle in the original.** At the game over the original stops the
  danger song and the mode's song and starts the jingle, but on every update after it `IsPlaying(danger)` still answers
  true (BASS's `ChannelIsActive` returns 1 for the stopped song, on 547 calls out of 547 after it first played), so the
  board keeps calling FadeOut(danger) and FadeIn(mode song), and the mode's song plays again, rising from silence.
  The recording runs BASS on its "no sound" device (the logging `bass.dll` gives BASS_Init device 0; with the real
  device BASS_Init fails under origrun's DirectSound stand-in and the game plays no music at all), and whether a stopped
  song stays "active" on a real device is not known, so this may be the recording's own effect. How to see it:
  `musiccmp.py classic`, from change 604. What it would take: a run with BASS on a real sound device and its output
  recorded, which origrun cannot do yet.

## Deviations left

- The port shows no registration, trial or update-check dialog and has no internet high scores (not ported, on purpose).
- Not compared in phase one: the options dialog, the hint dialogs' "don't show again" box, the help screens, deleting and
  switching players, the quit question. Their effects are not on the game's state, and phase two looks at their screens.
