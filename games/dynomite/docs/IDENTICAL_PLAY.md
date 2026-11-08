# Dynomite Deluxe: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. The same input gives the same games in the original and the
port, judged by the game's state, not by pictures. Phase two, **looks identical**, compares the screens, the sound, the
focus and the web build; its sections follow phase one's.

The original is the game carved out of `Dynomite.exe` (`re/dynomite/bin/DynomiteGame.exe`, image base 0x400000). It runs only
through `tools/origrun`, from a copy under a short path (`C:\byo-og\dyn\game`, made by `games/dynomite/tools/identical/setup.sh`),
under rundesk (invisible, silent, below normal priority). It runs on origrun's virtual clock (`--clock virtual`: the game is held
between updates, so an input lands on an exact update) with the scripted cursor (`--cursor scripted`: the launcher aims by
`GetCursorPos`). The sound stand-in served the game's DirectSound throughout; `--sound nodriver` was never needed. At most two game
processes ran at a time (the original and the port).

## How both sides are driven

**One input for both.** `tools/identical/mkscript.py` writes a port `-script` from a list of parts: the title's click, a mode
from the main menu with its page's choice, shots at random points of the board (seeded, one every 40 updates), waits, typed
text, single clicks, the side bar's save with its dialogs, and Continue Game. `toorig.py` turns the script into an origrun
script: a port line "K command" runs before update K's widgets, and the original reads an input posted at counter N before
update N+1, so the line becomes `at K-1`. A screenshot line becomes a move to where the cursor already is, so the original
also runs past the last real input. No script clicks a link, a Buy or a More Games button; the shots stay inside the board
(x 180 to 620, y 40 to 330) and the other clicks are the menus', the dialogs' and the end screen's own buttons.

**One start.** The game seeds its generators from `GetTickCount`: the app's Mersenne twister (+0x5d8) and its Fibonacci
picker (+0xf9c, seeded with the count shifted right once) in the app's constructor, the launcher's own twister, and the
framework's global twister and `srand` in `SexyApp::Init`. `setup.sh` writes five patches to
`tools/origrun/patches/dynomite.txt` (git-ignored) that give origrun's `--seed` to each: a `mov eax, seed` in place of
the `GetTickCount` call at 0x4010c5, 0x48c5d2 and 0x48c83c, and at 0x419a3f the constructor's import pointer becomes a stub
in padding at 0x443054 that returns the seed. The port's `-seed=N` stands for the same count. None of the patches touches
registration; `setup.sh` copies the carved exe in, it carves nothing itself.

**One clock.** The game's loader thread runs on wall time while the virtual clock races ahead, so a click on the title while
the loader still works is lost. Every case starts its input at update 25000, by which the loader is done; the title and the
menus draw no random numbers, so the wait changes nothing else. The port runs with `-fast -drawevery`: Endless, Fossil and
Time Trial draw from the twister in their `Draw` (the shake of a baby egg and the like), so the port must draw every update
as the original does.

**One folder.** `orig.sh` removes the high-score files, `STOMPED.TMP` and the saved games beside the exe, deletes
`HKCU\Software\PopCap\Dynomite` and imports `settings.reg`, the values `port.sh` writes into the port's fresh user folder.
`mkhs.py` writes the same low tables (Ann to Eve, 50 to 10) into both, so every game ends in the name dialog.

**Watched values.** The original's board is the pointer at 0x531ee4 (the app at 0x5304f8, +0x19ec), the grid at board+0x8c
and the launcher at board+0x5c. origrun watches four expressions at a time, so a case runs the original seven times for 28
values: the twister's index and first word (0x531490, 0x530ad0), the picker's two indices (0x531498, 0x53149c), the side
bar's score (`[0x531edc]+0x68`), the board's type, chain, longest chain, babies, bonus bits, rank, pause count and last
colour fired (+0x7c, +0x14c, +0x1d0, +0x1d4, +0x12c, +0x1c0, +0x150, +0x15c), the grid's egg count, colours in play,
bottom, ceiling, rows and frames past the line (+0x46e0, +0x10, +0xc, +4, +8, +0x4684), the launcher's angle, the loaded
egg's and the held egg's colour and its frozen flag (+0x14, +0xc, +0xaa8, +0xaa4), the colour of the egg Steggy holds
(the next to come, `[[board+0x84]+0x20]+4`), and four of the mode's own: Stomped's round, shots left, egg index and shots;
Endless's whirley level, whirleys hit, kick and whirley count; Fossil's level, round, pushes and fossil-fell flag; Time
Trial's eggs broken, eggs dropped and rows cleared. The port writes the same names with `-statelog=<file>`
(`FossilizedApp::LogState`, "update name value" on each change, logged at the start of the next update so that the update's
`Draw` is in it). `cmp.py` compares every change of every value with its update; a value the watch sees twice in one update
counts with its last.

**The board's cells and the saved game.** The watch cannot read the grid's cells, so the cells are compared through the
saved game: `sv1` saves in both programs and `savecmp.py` compares the two files byte by byte, apart from the picture of the
board (two renderers) and the tick counts at the save and at the board's start.

## Phase 1 results

All eight cases are in `games/dynomite/tools/identical/cases.sh`; `bash games/dynomite/tools/identical/cases.sh` runs them
(after `setup.sh`), or `cases.sh en1 sv1` runs some. Each row is one run of the input in the port and seven runs of the
original, four values at a time: all 28 values (27 in Time Trial) compared change by change with the update each came at.
**Every value of every case was the same**, and the saved games of `sv1` were the same field by field apart from the
picture, the clock and the original's unset fields (Oddities). The changes column counts the twister's index (one change
for every draw), the picker's index and the score.

| case | seed | what is played | twister, picker, score changes | compared to update |
|---|---|---|---|---|
| en1 | 2 | Endless: 80 shots, game over at 27 559, the name dialog (typed into, Okay), the table, Play Again, 80 more shots, game over at 31 405, the name and the table again | 4178, 78, 14 | 33 510 |
| st2 | 4 | Stomped Puzzle, the first pack: game over at 27 242, the name and the table, Play this puzzle again, 60 shots of a second game | 2878, 27, 3 | 31 580 |
| fs1 | 6 | Fossil, Strategic: game over at 27 642, the name and the table | 2118, 32, 4 | 28 909 |
| fp1 | 8 | Fossil, Panic: game over at 27 885, the name and the table | 2291, 37, 4 | 29 109 |
| tt1 | 10 | Time Trial, Normal: 150 shots (the clock, the rows to go, eggs broken and dropped) | 108, 151, 6296 | 32 399 |
| sv1 | 12 | Endless: 40 shots, then the side bar's save (slot, Save, the name, Save, OK) back to the main menu; the two saved games compared | 1874, 41, 13 | 27 944 |
| sv2 | 12 | The port's saved game of sv1, continued in both (Continue Game, the slot, Let's Play) for 60 shots | 2902, 60, 25 | 28 429 |
| sv3 | 12 | The original's saved game of sv1, continued in both for 60 shots, to game over at 27 775 | 2250, 40, 12 | 28 429 |

en1's table (`ENDLESS.HS`) was the same in both programs, the names typed, the scores and the order, apart from the new
rows' ids (the board's start ticks); the other cases' tables were compared only through the score and the dialogs' pause. Time Trial was not played to its end: it ends only when
every egg is gone, which random shots do not manage, so its table was not reached. The tutorial was not played.

The port played en1 twice more with byte-identical state logs, so its runs repeat. Of the original's runs (about seventy in
all), two differed from their repeats: see the Oddities.

## What was changed

| difference | cause in the original | what the port does now | commit |
|---|---|---|---|
| The twister's first word differed from the first update on, so every draw differed. | The 2003 framework's `MTRand::SRand` (0x442d90) fills the state with `mt[i] = mt[i-1] * 69069`, seed 0 standing for 4357; the port's framework seeds the later way. | The app's and the launcher's twisters are seeded with `SeedMTRand2004` (`framework/compat2004`), and the launcher takes `-seed` as the original takes the patched count. | 7fb4da2 |
| The twister fell behind in the port during play. | The modes draw from the twister in `Draw` (a baby egg's shake), and the original draws every update; with `-fast` the port skips draws. | The cases run the port with `-drawevery` (a run option, not a change to the game). | 632d1b6 |
| The board's pause count rose to 2 at game over in the original and to 1 in the port, and it stayed one higher in the original until the name dialog closed. | The app overrides `ModalOpen` and `ModalClose` (vtable slots 56 and 57, 0x418b60 and 0x418b80): a modal dialog pauses the side bar (`SetPauseState(1)`) and its closing resumes it (`SetPauseState(2)`). The port had neither. | `FossilizedApp::ModalOpen` and `ModalClose`, as the original. | b5c3aef |
| A saved game's byte at FirePrompt's +0x1c could change from run to run of the port. | The original never sets the field; it only saves and loads it. | Zeroed in the constructor, as the port already did for its +0xc. | c8da6e1 |

## Frozen baseline

`build/baseline-dynomite/` (git-ignored: a copy of `dynomite.exe` and its DLLs made when this pass was done, at c8da6e1)
is checked by `tools/check.sh dynomite`: three of the cases above (en1 without its table shots, st2's first game, fp1) run
in both the current port and the baseline with `-statelog` must write the same non-empty state log, so later fixes and
mods leave unmodded play as compared with the original.

## Phase 2: how it was compared

Seven tours (`games/dynomite/tools/shots/tours.txt`, run by `tour.sh`) play the same input in the port and in the
original and take a screenshot at the same update in both. The original runs in Windows Sandbox (`origrun --sandbox`),
where its DirectDraw window can be captured while nothing shows on the user's desktop; its shots carry the window's frame,
which shotdiff cuts away. shotdiff (`docs/SHOTDIFF.md`) compared every pair with no tolerance for a difference I had not
judged, and I looked at the crop of every box it flagged. The tours are:

| tour | seed | what it shows |
|---|---|---|
| t1 | 2 | the title, the main menu and each of its pages (tutorial, continue, the modes, back), the options dialog, the high scores, the credits, the options' third page, the exit question and staying |
| t2 | 2 | Endless: the board, eight shots, pause and resume, the focus lost and regained, the side bar's New Game question, a new game played to its end, the name dialog and the table |
| t3 | 4 | Stomped Puzzle: the pack pages, forty shots to the end, the name typed, the table, Play Again |
| t4 | 6 | Fossil Challenge, Strategic: forty shots to the end, the name dialog and the table |
| t5 | 8 | Fossil Challenge, Panic: the same |
| t6 | 10 | Time Trial, Normal: thirty shots, the focus lost for two seconds and regained, pause and resume, thirty more |
| t7 | 12 | Endless: twenty shots, the side bar's save (the slots, the name, saved), Continue Game from the main menu, ten shots |

**Sound.** `sound.sh` plays each tour again with the original's sound logged by origrun's stand-in (`--sound wav`, and
Big Money's BASS proxy for the music) and the port's by `BYOPOP_SOUNDLOG`, and `soundcmp.py` lines up the two: every
sample played (named by its length, rate and channels), its update, volume and pan, and every music call (play, stop,
pause, resume, volume). t1, t2, t4, t5 and t6 are identical. In t3 one sample is named by its length only, because the
original streams that buffer (the footfall), at the same updates; its last music volume of 0 is the port quitting. t7 is
identical up to the continued game, where the shots differ (below).

**Focus.** t2 and t6 take the focus away and give it back (origrun's `focus` and the port's script); the board pauses and
resumes in both, the screens match, and the sound logs are the same (neither program calls the music while the focus is
away).

**The web build.** `web.sh` plays each tour's script in the web build under Node and compares its shots with the native
port's, with no difference allowed. t1, t2, t6 and t7 match, but for the options' Full Screen box (the native runs start
full screen, a page cannot). In t3, t4 and t5 the board of Stomped and the Fossil Challenge shows a one-pixel difference
over its whole background: those modes shake the board while the grind sound plays, and under Node there is no Web Audio,
so the grind never plays and the web board holds still. In a browser it plays as natively.

## Phase 2: what was changed

| difference | cause in the original | what the port does now | commit |
|---|---|---|---|
| The first main menu had no welcome sign. | `gIntroReturning` starts true in the exe. | It starts true. | 28f139c7 |
| The dialogs' frames and buttons were cut differently at their edges. | The 2003 framework's `DrawImageBox` (0x463450) cuts the edges and the middle a third of the picture each. | `Graphics::mDrawImageBox2003`, set by the game (a shared commit, d3177375). | 28f139c7 |
| The mouse stayed where it was when a game started. | The main menu puts the cursor on the board's launcher (`SetCursorPos`). | `FossilizedApp::WarpToBoard`; the framework does nothing for it in a test run or in a browser. | 838294ea |
| The header of every game dialog (New Game, Exit, the save messages) was in the small font, so the dialogs were shorter. | `NewDialog` (0x418a00) gives the header the white font (+0x19d4); Ghidra's listing drops that argument. | The header is in the white font. | 5899db40 |
| The Okay and Cancel buttons of the name dialog and the save dialogs lacked their bottom shadow. | They are plain `ButtonWidget`s (0x47f3b0) drawing the button picture whole, not `DialogButton`s cutting it. | They are `ButtonWidget`s with the picture as their image. | 5899db40 |
| Stomped's end panel counted the round being played among the rounds cleared. | 0x410880 prints the round less one. | It prints the round less one. | 5899db40 |
| An empty save slot's picture was clear, showing the dialog behind it. | The dialog fills it with opaque black (0x433910). | Opaque black. | 5899db40 |
| The web build crashed when a board started without a sound device. | The three looping sounds' instances are made once when loading; without sound there are none, and every board calls through them. | A shared silent instance stands in (a deviation, below). | 5899db40 |

## Phase 2: what remains

In t1 the options dialog is shorter in the port: the original has a "More Games" link above Done, which the port leaves
out (no button leads out of the game), and the credits' two names are plain text where the original has links. The
credits' edges differ only where the taller options dialog shows behind them. The options' Full Screen box is ticked in
the port and clear in the original: both read the same registry value, but origrun runs the original in a window and the
port's test runs start in full screen.

t7's continued game differs from the original's from its first egg: `Dynomite::Load` (0x405590) seeds the egg picker with
the tick count the save wrote, so a continued game's colours depend on the moment of saving, in the original as in the
port. The two programs saved at different clock values, so their continued games differ; phase one's sv cases, which
load one file in both, play the same. Native and web runs of the port differ there for the same reason.

The name box's caret in t5's name dialog is on in one shot and off in the other: the caret blinks on the edit box's own
count from when it got the focus. The same dialog matches in t3 and t4. It is masked in t5's over shot.

Natively the board of Stomped and the Fossil Challenge differs faintly in some shots (t3's s2) by the same one-pixel
shake as in the web build: whether the grind sound counts as playing differs between the original's sound stand-in and
the port's silent driver. shotdiff rates it below visible.

One stick of dynamite in t2's Endless game is drawn a few pixels from the original's after the pause (Oddities).

## Phase 2: played with real input

Autoplay and the script commands never pass through the game's own input, so the port was also played with real mouse
and key events only, from launch: the tutorial and a whole game of every mode, natively and in the browser. No session
plays a game itself; the moves come from the port's own bot through the standard `move` command, which the driver
sends as real input.

**Natively** `tools/realplay/play.sh` runs the port under rundesk with Bejeweled Twist's driver, which posts the clicks
and keys to the game's window on the real clock (the game's script only asks `state` and `move`). Each run starts from
a fresh user folder with low high-score tables, so every game ends in the name dialog: the driver types the name,
presses Okay and photographs the table. Every run was made windowed and again in full screen (the window filled the
3840 by 2160 display):

| run | windowed | full screen |
|---|---|---|
| tutorial | 128 moves in 146 s, every step to its end on the main menu | 126 moves in 156 s, the same |
| Endless | 248 moves, whirley level 2, 9510 | 280 moves, 8140 |
| Stomped Puzzle | 584 moves, round 15, 16230 | 446 moves, round 12, 12120 |
| Fossil Challenge | 1027 moves, eight fossils, 70160 | 1072 moves, nine fossils, 81110 |
| Fossil Panic | 243 moves, fossil 5, 39590 | 162 moves, fossil 3, 22480 |
| Time Trial | 256 moves, 4:51 | 423 moves |

Each game ended in its end panel, the name went into the table, and no run stalled, crashed or logged an error. The
tutorial needed three things of `move` beyond a shot: its Continue button, the side bar's museum button at the step that
waits for the museum (and the museum's own close and continue buttons, which every Fossil game also presses after each
fossil), and at the step that asks for the whirley to be shot, the shot whose flight meets it. The bot's flight takes
its probes as ghosts, so it changes nothing in the game.

**In the browser** `tools/web/onegame.js` drives the player-only page (`?player=1&assets=1&fast=1&silent=1`, in the
Browser pane, with the shared test kit's real DOM events): it clicks through the title and the main menu's pages with the
same clicks as the native step files, then plays by `move` through the kit's `playByMove`, types the name and presses
Okay. The page runs on the virtual clock (`fast=1`), so the bot's every move lands in step with the game and it plays
much better than natively:

| run | result |
|---|---|
| tutorial | 130 moves in 64 s, every step to its end on the main menu |
| Endless | 1500 moves, whirley level 2, 38070 |
| Stomped Puzzle | 697 moves, round 15, 16630 |
| Fossil Challenge | 2407 moves, twelve fossils, 113360 |
| Fossil Panic | 1000 moves of the bot, fossil 8, then 9 shots straight up to end it, 61250 |
| Time Trial | 268 moves, 2:40 |

Every game reached its end panel, the name dialog and the table, with no error logged. Panic was still being won at
the driver's limit in a first run (3000 moves, fossil 14, 140250), so the driver ends a game the bot is still winning
with real shots straight up. A page cannot start in full screen, and the hidden pane cannot be put there, so full
screen was played natively, and on the web `?scale=2` stood in for it: the canvas drawn at 1280×960, so that every DOM
event lands at a scaled point and is scaled back as in full screen. After the trunk's merge (its MessageChannel frames)
the tutorial at scale 2 played its 132 moves to the main menu and Time Trial its 281 moves to the table, both with no
error logged. In that tutorial five of the bot's answers did not come within the kit's ten seconds, and the same `move`
answered at once when asked again, so the driver asks again after such a timeout too.

Two things in the tutorial needed care in the page, where the bot's moves come faster than natively. At step 45 the
tutorial has made the eggs sit past the line and waits for the board to be lost; a shot there that makes a match clears
the rows, and the step never ends (the board is no longer past the line, and the launcher stays empty). A player could
do the same, so the bot plays that step as it plays step 43, landing low without a match. And between messages the bot
has no move for a while, which the kit's `playByMove` takes for the end after two asks, so the driver asks again while
the game goes on.

## The fixes mod

The pass found no fix candidate: no bug of the original that changes play and no wait that only costs time (loading is
already made fast in the port). So Dynomite has no fixes mod yet; `GAME_NOTES.md`, "Fix candidates", keeps the list.

## Oddities (skipped)

- **A match at the tutorial's step 45 stalls it.** Step 45 waits for the board it has pushed past the line to be lost
  (`Tutorial::Update`, the step's wait on the board's end). A shot fired in that wait that makes a match clears the
  rows above the line, the launcher stays empty, and the tutorial waits for good; the side bar's buttons still leave
  it. The port's step follows the original's code, but whether the original stalls the same way was not run. To see
  it: in the tutorial, fire a matching shot as soon as step 45 begins. It would take a run of the original's tutorial
  under origrun with the same shot; if it stalls too, it is a soft lock for the port to fix.
- **The original sometimes plays a run differently.** Two of about seventy runs of the original differed from the port
  and from their own repeats. In one, en1's run that watches the eggs' colours had other colours from the first egg
  (Steggy's first was 2 where every other run had 1). In one run of sv1 the saved game's launcher+0xa68 (the last landed
  egg's height against the aim) was 0xffffff44 where the port and the repeats had 0xffffff63. Both runs had their seed
  patches applied before the game started (origrun's log), and repeats of the same runs, twice each, matched the port
  exactly. The cause was not found; candidates are origrun's virtual clock letting an update or a draw slip
  against the loader thread, which runs on wall time, and a `GetCursorPos` answered from Windows rather than the script.
  To see it: run `cases.sh en1` or `cases.sh sv1` several times. It would take logging every update's draw count in the
  original.

- **The original saves fields it never set.** A saved game holds the board's +0x64 and +0x140, the grid's warning flag and
  its blink count (+0x4688, +0x468c), its blast sums (+0x46d4 to +0x46dc), the launcher's 0x50 bytes at +0x38 and the side
  bar prompt's +0xc and +0x1c; no constructor or `Init` of the original sets them before the first save, so the file holds
  whatever the heap held there (in `sv1`, 0x01010101 in the board's two and pixel colours in the launcher's block). The port
  zeroes them. Nothing reads them before they are set in play, except the blink below. To see it: `savecmp.py` with the
  fields taken out of its `SKIP` list. Making the port's files equal would mean copying heap leftovers.
- **The warning's blink starts from leftover memory.** The grid's blink count (+0x468c) is not set by its constructor
  (0x415770); in `sv1` the original's started at 0xff6b9a32, a pixel left on the heap. Counting up from a negative number it
  never passes 25, so the warning flag does not toggle in the original until the eggs first pass the line; at that moment
  both programs set the flag and zero the count (`EggGrid::Update`), and from then on they blink alike. The warning is drawn
  only while the eggs are past the line, so nothing on screen differs. The port starts the count at 0, so its flag toggles
  unseen before then. To see it: watch `[[0x531ee4]+0x8c]+0x468c` and `+0x4688`.
- **One stick of dynamite in t2 is drawn a few pixels off.** In t2's Endless game, after the pause, one stick near the
  top (about x 378, y 30) and its fuse's spark sit about four pixels lower against their eggs in the port than in the
  original; every other stick and egg matches, and the game plays the same. The stick's picture and slide come from the
  app's twister when it is made, and the twister's index follows the same course in both programs through the pause
  (watched at 0x531490), so the cause is not a draw of random numbers. It was not found within the hour. To see it: t2's
  resumed, e2 and e3 shots (`tour.sh t2 port orig cmp`). It would take watching the grid's list of sticks (+0x466c) in
  the original.

## Deviations left

None in play. The port leaves out what only an unregistered copy shows and the credits' two links to web sites, and it
has no wait of its own in loading (`README.md`, "Registration and loading"); none of these shows in a registered copy
but the credits' two names, which are plain text. Without a sound device the three looping sounds get a silent stand-in
where the original would call through a null pointer. In a browser the main menu cannot move the mouse onto the
launcher, and a page cannot start in full screen.
