# Atomica Deluxe: identical play

Phase one of the identical-play pass: with the same input, the original and the port play out the same games. Phase two
("looks identical", screen by screen) is at the end of this file.

## What was compared and how

The check is by game state. `tools/identical/compare.py <scenario>` runs the original (Atomica.exe 2.52, from a copy,
through origrun on its virtual clock with a scripted cursor, invisibly under rundesk) and the port (`-fast -drawevery`,
which draws every update as the original does) on one scenario, and compares 29 values at every update: the two random
generator words, score, level, molecule count, combo, the 72 cells of the grid, the meter's value, tick, countdown and
overflow, the pause count, the selected atom, the menu and board pointers and more. The original's values come from
origrun `--watch` (addresses from our export in `re/atomica/`, listed in `WATCH`); the port's from its `-statetrace` file.
Scenarios are plain "update, click x y" lines. The scripts in `tools/identical/` were recorded from the port's bot
(`record.sh`), which plays through the same clicks as a player's, so every move, hint box and level-up box is a real
click: `s2.txt` (strategy, level 1 completed, the level-up box), `a1.txt` (action, to update 6082), `g4.txt` (strategy,
game over at update 5004), `g5.txt` (two strategy games in a row: the game over, the high score entry with a typed name,
the play-again button and the second game's start), `c1.txt` (continuing a saved game, below) and `s5.txt` (an idle
game). `compare.py --scores` adds the two high score tables (scores, the first letters of each name, the best of the day). Before each original run the registry values it keeps
(hints shown, symbols, tips) are deleted so both start fresh.

Result: all 29 values agree at every update of `s2`, `a1`, `g4` and `c1`, and the 30 values of `g5` (the tables included)
up to update 6500 (see the oddity about the original's stopping), apart from start-up values the original has not set yet
(`title_p`, `rng_k` at update 1; `numcolors`, `meter_over`, `rounds` at the update the board is made: they hold whatever was
in memory, 0x80808080 and the like).

## Changes made

The original draws random numbers while drawing (Board::Draw, the swirls, the Atomicus monitor, the bolts), so every
visible effect that draws must draw the same numbers in the same order.

| Difference | Cause in the original | Port now |
|---|---|---|
| Random numbers drifted from the first frame | The Atomicus monitor's static draws one number per row of its buffer (150 a frame, 0x4220xx) | `Atomicus::Draw` draws into the monitor buffer and takes one `Get(mNoise)` per row |
| Dr. Atomicus talked and looked about at other times | Lip-sync runs from the voice's mouth data (0x421c70-0x421e10) and a pending flag (+0x45, set from the start) holds back looking about and comments | `LipStart/LipUpdate/LipNext`; the mouth data is read from the player's exe at start-up (`GameText`, `LipDataTable.inc`), none in the repository |
| Bolts drew a different number of numbers | `Bolt::DrawPixels` (0x4173f0): a bent line with two jitter points on each side (6 draws) and mini sparks around both ends (16 draws) | `Bolt::DrawPixels` rewritten to it |
| Bonds drew a different number of numbers | `Grid::DrawBond` (0x40f2e0): three angle jitters, then two mini lines | `Grid::DrawBond` rewritten to it |
| The default high score names were the port's own | The original's five names are texts of its exe (0x4b0270-0x4b02ac, read by 0x41e320) | `ScoreTable::Reset` reads them like the other texts (`GT()`), none in the repository |
| The saved game was 4 bytes longer | The original saves only the mode (0x405fc0), not the expert flag | the flag is no longer saved or loaded |
| The lip-sync varied from run to run | The port read the real clock (`GetTickCount`) where the original reads its own virtual tick | `Atomicus` uses `GetClockTime()` (virtual under `-fast`); the bot's runs are now identical to each other |

The test bot (`testhooks/Autoplay.cpp`) was changed so that its play can be replayed as a scenario: it acts before the
update as a script does, clicks through the widget manager, waits three updates between a click's two halves (the atom, then
its cell; the hint box's check mark, then Continue) and records its clicks (`-autoplayscript`). That changed the bot's games,
so `tools/check.sh atomica` was refrozen (`AT_STRATEGY`, `AT_ACTION`); all later fixes must leave those numbers unchanged.

## Saved game

`savecmp.py <scenario>` plays a scenario that ends in "save and quit" (`sv.txt` shape: the quit button at 72,464, then
"save and quit" at 386,283) in the original and the port and compares the two `SAVEGAME.DAT` files word by word. The files
have the same length and layout; 163 of their 1,261 words differ, all of them uninitialised memory the original writes
out (heap fill such as 0x80808080, pointer-like values in each atom's record and the board's tint floats), which the port
writes as zeros. Then `compare.py c1.txt --savefile <the original's file>` starts both programs with that file and chooses
"continue" on the menu: all 29 values agree for 2,222 updates after the game is picked up. (The port also reads the file
the original made, so a game saved by one is continued by the other.)

## Frozen baseline

`build/baseline-atomica/` holds the port's exe as of this pass. `tools/check.sh atomica` replays `s2.txt` and `g5.txt` in the
current port and in the baseline and requires the same non-empty state log from both, besides the bot's two games and its
rerun. A later fix that changes unmodded play refreezes it on purpose.

## Oddities (not chased)

- The original stops updating its board and drawing at a point that varies from run to run, while its update counter goes
  on: the random numbers stop changing at update 4711 (a game started at 711), 4533 (a continued game started at 2311), 6319
  and 6531 (games started at 2311). The meter's countdown stops about 70 updates later. A click or a mouse move does not
  move it; it is not 4000 updates from the start, whatever it first looked like, and the export holds no constant or idle
  counter for it (searched for 4000, 0xfa0, 40000, 15000, the tick calls). In the same runs origrun's clock log shows the game
  time per update rising (97 s for 5,200 updates in one run, 318 s for 4,600 in another), so it looks like the framework's
  catch-up logic reacting to the virtual clock, not the game's own rules. The port is unaffected. To see it, run
  `compare.py tools/identical/s5.txt --names rng_j,meter_countdown --end 4900`. Every comparison above stops before it.
- The two saved games differ in the uninitialised words described above.

## Deviations left

The geiger needle's geometry and the title logo's shimmer (README), the swirls' scan-line fill, and the missing "More games"
and "Register" links. The high score table's "best name" and the saved game's garbage words were not compared by value.

# Phase two: looks identical

## What was compared and how

`tools/identical/shots.py <scenario> --run <id>` plays one scenario (the same `N command` lines as phase one, plus `N shot
<name>`) in the original (origrun, `--sandbox`, virtual clock, scripted cursor, its own copy of the game) and in the port
(`-script`, `-fast`, under rundesk), takes a picture at each `shot` line and hands both sets to `tools/shotdiff` with the
masks of `games/atomica/shotdiff.json`. The scenarios are `p2-title.txt` (the title screen and its fade), `p2-start.txt` and
`p2-start2.txt` (menu pages, start of a game), `p2-s2.txt` and `p2-g5.txt` (the bot's games: board, atoms, molecules, level
up, game over page), `p2-dialogs.txt` (options, credits, high scores box, reset hints, quit box) and `p2-hs.txt` (a game over
that earns a place: the entry box, `--hs` starts both games with a table every score beats). Each flagged box was looked at
in its crop and judged; what is left is below.

## Changes made

| Screen | Difference in the original | Port now |
| --- | --- | --- |
| Title | the logo's colour waves, the play link | waves move as the original's (same count of updates, same picture); the play link as the original |
| Main menu | expert fields, portrait position, atom picture drawn onto its mask, the spinning-atom sprites made by a blend that is copied into a blank sprite (0x438b00, 0x437f40) | the same |
| Board | the geiger needle, sparkles' alpha, molecule overlay alpha, Atomicus's nod | the same |
| Game over page | the rank label and comment (22 phrases, 0x4cbd08), the score rows' colour cycle (256 colours, a start in the page's spin, one more per row) | the same |
| High scores box | the title, "TODAY'S BEST" and Return button positions, the colour cycle of the rows (0x41ec90) | the same |
| Options box | the label rows' y, the colour of each name in the credits | the same |
| Quit box | texts and the red third button | the same |
| Reset hints | clears the hints shown only, not the grid's flag (0x424430) | the same |
| Title music | the title screen plays the song's order 6 from update 0 and stops it when it goes (0x42fe90, 0x42fd10) | the same (it was silent) |
| Default volumes | music 0.5, sound 0.85 (0x4066d4) | the same |
| Level over atoms | x, then y, then the wobble from the game's generator; the plain sprite under the glow (0x4037f0) | the same: the whole board matched, it had shown stronger highlights |
| High score entry box | six shadow copies of "A NEW HIGH SCORE!!!" climbing from faint, the score above it between two arrows that come and go (the tick runs 0 to 300 in fours), the monitor's line and the prompt in the cycling colours, the name on a picture of a field with no field of its own (light text, white highlight, 15 characters, 140 pixels), SAVE and CANCEL as green fields (0x41cf90, 0x41ca50, 0x41cc00) | the same (it was a simplified drawing with white field and OKAY) |
| High scores file | HS.CFG is read whenever the file holds two tables (416 bytes) | the port asked for 448 bytes and never read back what it wrote: fixed |

The fixes are drawing and box state only; the frozen baseline replays the same non-empty logs (`tools/check.sh atomica`).

## Masks (`shotdiff.json`)

- The title logo's waves (`t*.png`): they move once for every 20 title updates; the original loads on a thread and runs
  about 4000 updates in its first 300 frames, the port loads in the main thread and runs one a frame. The port's waves equal
  the original's for the same update count (both pictures fitted to the same formula).
- The loading bar at frame 300 of the title run: the original is still loading, the port has finished (the finished picture
  is compared at `t1000.png`).
- The halo of a molecule that has just formed (`g3350`, `g7400`) and the shaken atoms of the game over state (`hs8300`): the
  original's draw functions take numbers from the game's generator (0x413880), so offsets depend on how many frames were
  drawn; no input lines that up.
- The options box's "Register" and "More games" rows (`a3310`, `a3330`, `a4510`): shareware links, left out on purpose.
- Atoms drawn from the rotated glow sprites (`g2650`, `g2900`, `g3150`, `g6150`, `g7150`): see the oddities.

## Oddities skipped (phase two)

- Rotated glow-sprite atoms sit a pixel up and left of the original's (`g4050`, `g3750`, the flying atoms in `g5350`), and
  their orbit rings are off by a rotation step in some pictures (`g2900`). The sprites are the mask rotated by the
  framework's software rotation; the state (glow, aim angle) is identical in both traces. To see it, run `shots.py
  p2-g5.txt --run h1` and open those pictures. A fix would need the original's rotation rounding.
- The edge antialiasing of the atom symbol icons (`g2650`, `g6150`) differs by a few levels.
- A molecule's halo shake (`g3350`, `g7400`): see the masks.
- Pictures of the native port in real time and under `-fast` differ at the same update (the web build equals the real-time ones), while the
  state is equal; the `-fast` pictures match the original's. Cause not found (the menu panel and doors at update 2341, `p2-web.txt`, `web.sh`
  after `shots.py ... --port-only`, and a native run without `-fast`); it belongs to the shared frame loop, not to Atomica.

## Sound, focus and the web build

- **Focus:** `p2-focus.txt` loses the focus in a strategy game's first hint box, regains it, and loses and regains it
  in a row (`focus lose` / `focus regain`, as the window gets the deactivate and activate messages). The pictures after each
  step are the same in both (the box stays, no pause picture is drawn, the board is not redrawn differently).
- **Sound and music, against the original's own calls.** `tools/identical/sound_orig.sh <scenario> <name>` runs the original on the host
  (rundesk, virtual clock) with origrun's recording stand-in (`--sound wav`: every DirectSound buffer create and play, in
  `logs/sound.log`) and with `tools/fmodproxy` as its `fmod.dll` (a logging stand-in for FMOD 3, the Big Money bassproxy adapted:
  the real DLL is renamed `fmod_real.dll`; every call of the 25 imported functions is written to `fmodlog.txt` with the game's update
  counter). `sound_cmp.sh` then runs the port on the same input with `BYOPOP_SOUNDLOG` and sets the two logs side by side:
  `soundcmp.py` the sample plays (the effects are decoded by FMOD and played through DirectSound; a play is named by its
  buffer's size, electric2 and electric3 being one size, and set against the port's by order, sample, volume, pan and update within
  two; the original's time is the virtual clock's ms/10 with the load as an offset) and `musiccmp.py` the music (the song is played
  through FMUSIC: `PlaySong`/`SetOrder`/`StopSong` by update). The original's registry key is deleted before each run, since the
  host registry keeps the game's hints off between runs and a run without hint boxes is not the port's fresh one. Results:
  the strategy bot's game `s2.txt` (4,000 updates: 214 plays) and an action game `a1.txt` (4,400 updates: 154 plays) have the same
  plays in the same order, sample, volume, pan and update (0 differences), and the same five music events (the title's tune at 0,
  the menu's song at 1510, the title's stop at 1556, the game's tune at 2310, the menu's stop at 2375). **The music log found two
  faults, both fixed:** the title screen starts its own tune (order 6, 0x42fe90) and stops it when it goes (0x42fd10), which the port
  did not do (the title and the first 1,500 updates were silent; the song also went on playing under the menu's). The original's
  runs are deterministic (two runs of `s2.txt` gave the same 478 plays at the same times). Later than update 4,400 the original's
  board stops updating at a point that varies (Oddities), so what it plays after is not compared. Not compared: the game over and
  high score entry voices (they come after that point in every scenario; the entry box's drawing is compared in pictures).
- **Web build pictures.** `tools/identical/web.sh <scenario> <run>` plays a scenario in the web build under Node
  (`wasmrun.js game`) and compares its pictures with the native port's by shotdiff. Against the native port **in real time** (without
  `-fast`) all 68 pictures of `p2-s2.txt` match (57 exactly, 11 within the faint tolerance): the web build draws what the native
  game does. Against the native port under `-fast` they do not (28 of 68 differ, the menu panel and the doors by a few updates'
  motion, atoms at other places): the same difference appears between the native port's `-fast` and real-time runs (a
  picture of `p2-web.txt` at update 2341: 78,000 pixels). The game's state at those updates is the same in both (the panel's y, the
  door and the alpha, printed at each shot, are equal), the pictures are not. The pictures of this pass's comparisons with the original
  are `-fast` ones that match the original's (virtual clock), so this is a finding for the shared loop (`-fast` against real time) and
  not a deviation from the original that was found; it is not chased here (Oddities).

## Deviations left in phase two

The options box's "Register" and "More games" buttons (shareware links, left out on purpose), the atom sprites of the oddities
above, and the shimmer of the title logo when the original's load order differs. The frozen baseline was refrozen at the end of
phase two: the only difference from the first freeze is five more names in the state trace (the cursor, the hold counters, the
glow); the traces of `s2.txt` and `g5.txt` without those names are equal to the old baseline's, byte for byte.
