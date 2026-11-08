# Bookworm Adventures Volume 2: identical play, phase one

Phase one compares the game's state, not its pictures: with the same input, do the original and the port draw the same
random numbers, in the same order, and reach the same engine states at the same updates?

## What was compared and how

The original ran through `tools/origrun` on the virtual clock (`--clock virtual --cursor scripted`), with the seed patched
(`tools/origrun/patches/bookwormadv2.txt`) and with its saves and registry values in the Windows Sandbox. The port ran with
`-fast -statelog`, the same input script (translated by `tools/identical/toorig.py`). The original's watches are the
Mersenne Twister index of Sexy::Rand, the app's update counter and the battle engine's state; the port logs the same through
`LogState`. `tools/identical/rngcmp.py` and `statecmp.py` compare the two logs; `case.sh` runs a whole case.

Three cases, all identical to the end of the compared stretch:

| Case | What it plays | Identical to update |
| --- | --- | --- |
| prologue | new player, intro, tutorial battle, first battles | 35691 |
| chapter1 | the same, then on into chapter 1's battles | 46251 |
| minigame-goldencoins | a profile written by the port and continued by the original, then Golden Coins | 9838 |

Five more cases cover the other mini-games, each from the same port-written profile (`profiles/goldencoins`, loaded by the original in
its sandbox), each identical in draw counts and timing to the end of the compared stretch:

| Mini-game | Identical to update |
| --- | --- |
| Mutant Words | 9545 |
| Word Up | 13988 |
| Link n' Spell | 10038 |
| Letter Rip | 10945 |
| Word Master | 11145 |

The original makes a mini-game (`Board::InitMiniGame`, 0x411370) after its loading screen, and the loading screen's letters, whose number
depends on how long the real-time loader ran, are drawn before it. Both programs therefore reseed Sexy::Rand when the mini-game's init
returns (the original through a patch in `tools/origrun/patches/bookwormadv2.txt`, the port under `-reseedengine`), and the comparison counts
only the draws after that. The mini-game's own init draws (its tiles) are made in the same update as the reseed, where the original's watch
cannot see them, so only Golden Coins' (the first case, which had no reseed) vouch for those.

A battle case, `boss-chapter1` (profile `profiles/bosschapter1`: book 4, chapter 1, three enemies before and including Dish and Spoon, the
boss), is partly done. Its first two battles (the Space Cow and one more) match: every engine state change within one update and the draws
after each engine's making (the engine states are `watch 2` of origrun). It does **not** show the boss: after the second victory (update
16437) the original's engine watch reports no further change while its draw counts go on rising as in the port, so the third and later battles
are compared by draw counts only, and those only up to each victory (the fireworks draw per frame, see below). Why the engine watch stops is not
found (the pointer behind `[0x7870a4]+0x30` may be another object by then). To see it:
`PROFILE=<profiles/bosschapter1> CALLOAD=9000 bash tools/identical/case.sh boss-chapter1 cases/boss-chapter1.txt`.

One difference this case found was the port's, in the test setup: the board updates only every eighth update while the kill-zoom runs
when the display is 3D accelerated (`Board::UpdateF`, from the original), which made the attack state last 629 updates in the port and 314 in
the original, whose sandbox has no 3D (`Is3D=0`). `tools/identical/port.sh` now runs the port on SDL's software renderer
(`SDL_RENDER_DRIVER=software`; `NO3D=0` keeps the normal one), after which the states agree. The 3D path cannot be compared with the original
(the sandbox has none), so it is a gap of the pass.

The draws of a victory's fireworks and effects are one per frame, which the port's draw rate (`-fast`) does not match: `rngcmp.py` compares
the counts from each engine's making to its victory and not after it, and rebases at the next engine's making.

The profile of the third case was written by the port and loaded by the original (its saved fields, the unlocked mini-games
and the book), so the save format is read the same way by the original. The other direction (a save written by the original,
continued by the port) could not be tested: the sandbox does not give its files back.

## Tolerances

The original's loading screens run on real time, so their lengths vary a lot from run to run (startup 682 to 1041 updates,
a chapter 1757 to 15080). The port's test options `-startloadupdates`, `-chapterloadupdates` (comma lists) and
`-reseedengine` take the lengths measured in the original run (`calib.py`) and reseed Sexy::Rand where the battle engine is
built, so the boards do not depend on the load lengths. The loading screen draws one random letter every 50 updates; the
port's timers that follow draws are quantised by `-fast`'s draw rate, so timings are compared with a tolerance of six
updates. Victory effects and fireworks draw per frame, so they are not comparable and the reseed isolates them.

## Changes made by the pass

| Difference | Cause in the original | What the port does now | Commit |
| --- | --- | --- | --- |
| After the first battles the random draws drifted (a book listed twice) | `LoadBookScripts` skips the books it already has by comparing names with the case the pak and folder held ("Book4.luc"); the staged files are lower case, so the port saw them as new | `GetFilesInDirectory` lists the original's case, from `filecase.txt` written by the stager | d8697ae |
| The cut scene's Skip button never showed and could not be clicked | The original shows it as soon as the curtain is open | `CutScene::Update` makes it visible when it enables it | 002309c |
| (test only) No way to match the original's load lengths, to log inputs or to start a battle on a fixed seed | n/a | `-startloadupdates`, `-chapterloadupdates`, `-reseedengine`, `-statelog`, input lines in the log | d8697ae, 002309c |

## The baseline guard

`build/baseline-bookwormadv2/` holds the port as it was at the end of this phase. The `bookwormadv2` suite of
`tools/check.sh` replays `cases/replay-prologue.txt` (a new player to the first battles) and `cases/replay-goldencoins.txt`
(Golden Coins from `profiles/goldencoins`, a profile with the mini-games unlocked) in the current exe and in the baseline,
and requires the two state logs to be non-empty and identical. Later fixes and mods that leave unmodded play unchanged keep
it green.

## Oddities (not chased)

- **The original stalls at the title on the host.** Under rundesk on the main machine the original never gets past its
  title load (lua.log stops after the title, the Mersenne index stays at 624); the Steam client running there is the likely
  cause but it was not confirmed. The Windows Sandbox runs it fine, so all original runs use the sandbox.
- **The Flash movies stand still in the sandbox on the virtual clock**, so cut scenes in the original are passed by the Skip
  button or by the clock.
- **Victory-effect draws depend on the draw rate**, so they differ between runs of the original too.
- **The original's load lengths vary** (above); the port copies the measured lengths rather than modelling them.

## Not covered

The soak (the scripted player) does not get further than the early game on its own: later chapters and the bosses were not compared (the boss case above stops at its first difference). Phase two (screens, dialogs, sound,
focus) and the original-to-port save direction are open.

## Open item for phase two: the web build's file case

The native port finds the original-case names through `filecase.txt` next to the staged files. The web build packs the assets
without that file, so on the site `GetFilesInDirectory` lists lower-case names and the book scripts would be loaded twice,
as before d8697ae. The web packer needs to include `filecase.txt` in the preloaded data.

## Faults the user reported after phase one (2026-11-07)

Found by playing, not by the comparison; each was checked against the original's decompilation, and the Word Up screen against a shot of the original (the greyed Submit plate, the Scramble and Menu buttons match).

| Fault | Cause in the original | What the port does now |
| --- | --- | --- |
| Every mini-game ended in "Invalid Lua Result: Expected boolean in LuaClassRet" | `CurtainDone` (0x442700) reads the result of the curtain's script callback only when it returned something; the mini-games' `MiniGame.DoConvPanel` returns nothing | `GameApp::CurtainDone` keeps the curtain only when the callback returned a boolean true |
| Word Up's Submit button drew nothing | `SetAttackDisabled` sets the button's cel rectangles from its current picture; a mini-game's is `btn_submit` with 238-wide cels, and the port used the attack button's 244 | the cel size comes from the button's own picture |
| Word Up had no Scramble button | the Board makes it in mini-games too; the scripts of the other mini-games hide it | `Board::CreateButtons` makes it for every board |
| Rainbow tiles never formed a word | `0x4129e0` leaves a wild tile out of the word, tries A to Z in its place until the word is in the list, and gives the tile that letter | `Board::GetWord` does the same (an earlier wild tile in the same word is left out, as there) |

The **curtain lag** is a hitch at the start of every transition, not a slow cel rate: a native real-time run drew the curtain at 60 frames a second, but the constructor of `CurtainTransition` (made anew for each curtain, as in the original) copied and flipped or mirrored the curtain picture three times, 24 to 30 ms, and the first frame uploaded three new textures, another 22 to 32 ms; on slower machines and in the web build it costs more. The port now makes the three copies once and keeps them (not compared against the original's frame times; the original's own code does the same work, so it probably stalls too, but a hitch that breaks nothing is fixed here anyway). Web frame times were not measured. The **music that stops after a chapter boss** is a bug of the original that breaks play (found by Bookworm Adventures' session, which reproduced it with a stalled game loop): after the boss the scripts play the treasure song and queue the map song for a given order of it; `SongPlayer::Update` starts the queued song only while the treasure song is at that order, so when the loop does not run while the song plays (a hidden or minimized browser tab stops the frames, the music goes on) the song passes the order and ends unseen, and nothing plays until the next song. The port starts the queued song as soon as the song has stopped, whatever order it waited for (not run to the user's exact case: a hidden-tab run was not made here). The web sound delay is another session's.

## Performance items (for the performance pass)

- **The first Board's `main.LoadResources` stalls the web build for about 1.5 s** (smaller stalls of 190 to 290 ms around it). `Board.cpp` calls the script `main.LoadResources`, which makes about 35 `LoadPAMFile` calls (the tile and effect animations: `tilefx_*`, `fx_*`, `potion_*`, `grid_*`), hundreds of JPEG and GIF-mask decodes, in one call, once per Board. The original makes the same call on its main thread, so a one-time stall before the first battle is its behaviour too; the port's is longer because decoding is slower in wasm. Possible fix: decode the PAMs' images ahead of the call, on the title screen's time-sliced load (hold them in the shared-image cache), which needs the list of the PAMs the scripts name. Natively the call took 5 s before the shared image-lookup fix (8a73a4fe) and well under a second after it. To go with the JPEG decoder question.

## Played with real input (before the final merge)

Web build, 2026-11-07, in the Browser pane at `?silent=1` with requestAnimationFrame shimmed: the page's script sent a mouse move, a pause, then a press and release on the canvas for every click (no game hooks). Played so far: the launch, the intro cut scene (Skip), the prologue's conversation clicks, the tutorial battle's word P-L-A-Y and the Attack button (the enemy took the hit, the board refilled); then, from a profile with the mini-games unlocked, the main menu, the Mini Games menu and **Word Up from its welcome to its Game Over screen** (six words clicked and submitted, Submit and Scramble drawn as in the original, score 28,000, Hall of Fame entry made), and **Mutant Words** up to the end of its timer (Game Over screen, Hall of Fame). No console error in any of it. Then **Golden Coins** (five words, 30,875 points, Game Over screen with its overfill bonus, Hall of Fame entry), **Link n' Spell** (words OIL and EGO linked and submitted, tiles turned to silver, 600 points, timer to zero, Game Over), **Letter Rip** (words TOT and PINE from POTENTIAL, timer to zero, Game Over) and **Word Master** (five guesses with the gold, silver and red feedback, the Game Over screen naming the mystery word). The page's script read the board's letters from the log through F9, a key the port logs the board's tiles and Submit button state on (`GameApp::UpdateAppStep`, a test aid that only logs), and sent the clicks as real DOM events; no console error in any mini-game. A click made while the tiles still fell or the hint card was up did nothing, as in the original (the first Letter Rip word had to be clicked again). **Not played yet:** a boss through to the map song and music after a focus loss, the later chapters, timed mode, the native pass (keys and burst-posted clicks) and full screen natively. The page's own full screen cannot be tested by an agent (requestFullscreen needs a real user gesture, which dispatched events are not); it is left for the user.

## Plan still open (written 2026-11-07, in this order)

1. The standard test commands (`docs/TEST_COMMANDS.md`, after merging the trunk): `state` (STATE {json}), `goto`, `level`, `unlock all`, `set`, `win`, `lose`, `seed`, sharing code with `TestHooks.cpp`; an ungiven command changes nothing, `tools/check.sh bookwormadv2` stays baseline-identical. F9 (a test aid, now only in a `-script` or `BYOPOP_TEST` run) goes when the live channel (`-commands`, `byopop.test.command()`) lands.
2. A driver under `games/bookwormadv2/tools/` (run in the page; reads the log, picks the longest words, sends real DOM events; one call plays a mini-game or battle and returns a summary), reaching battles and mini-games through the commands.
   Model it on Bookworm Adventures' driver (branch `claude/bookwormadv-78ebd4`, from d32a4607): `GameApp::LogBoardForTests` in its `TestHooks.cpp` (a `-boardlog=<file>` line `board: <n> <WORD|-> x,y ... attack x,y scramble x,y` when the board comes to rest, the longest word from the game's own word list; look buttons up with `mButtons.find`, skip boards where no tile `CanSelect`), `tools/realrun.py` natively and `webdrive.js` in the page (on the web the option is added by wrapping `Module.callMain` and the log read back with `Module.FS.readFile`).
3. A boss through to the map song, the music after a focus loss, a later chapter, timed mode.
4. The native pass: burst-posted clicks and keys (approach of `games/sevenseas/tools/realinput.py`), windowed and full screen, under rundesk.
5. Before the turn ends: stop the web test server on 18790 and delete `build/web/zz_*`.

## Remaining deviations

None known in the logic that was compared. Everything above "Not covered" is untested, not known to differ.
