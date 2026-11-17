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

A second battle case, `boss-chapter2` (profile `profiles/bosschapter2`, saved by `tools/bossfocus.py --stop-level 4.1 --save`: book 4 on the map after chapter 1's boss, so the next battles are chapter 2's, Old MacDonald's; script: Adventure, the map's Enter, then the soak), gives the same picture as the first. All 6 engine state changes of the original agree with the port's within one update. The draws before the first engine are idle map draws at the same cadence in both, the port with a constant 31 more (draws made while the profile's screens come up, before the counter's start in the original), and after the engine's making the counts agree except for a constant one draw (update 8292 on) that comes with the first board, made in the update of the reseed, where the original's watch cannot see the draws (as for the mini-games above); it does not grow, and the later draws of both programs fall in the same updates. To see it: `PROFILE=<profiles/bosschapter2> CALLOAD=9000 bash tools/identical/case.sh boss-chapter2 cases/boss-chapter2.txt`.

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
| Chapter 6.5: a scramble drew one random number a letter more than the original (16 against 15 each step) | `TileEngine::ChangeLetters` (0x486d40) gives a wild tile "?" without generating a letter; the port generated one for every tile | `ChangeLetters` asks `IsWildTile` first | this pass |
| Chapter 6.5: with a wild tile on the board a scramble chose other letters than the original from the same random numbers | `GenerateLetter` (0x485dd0) counts a "?" tile in the total but neither as a vowel nor as a consonant; the port counted it as a consonant, so nine consonants shut every consonant out a tile early. The original also builds a pattern for a row of 10 tiles, which the port left out | The count skips "?"; the row-of-10 pattern is built. Checked by simulating the 13 steps of a scramble from the original's own Mersenne state (peeked before the click): the result is its board | this pass |
| (-fast only) Chapter 6.4: a whole battle came out otherwise when the chapter load was 8 updates longer; the curtain held the board 0 to 9 updates more by the update count's phase | The curtain is done only once its last frame has been drawn (`mDrawn`), and the original draws after every update | `CurtainTransition::WantFirstDraw`: with `-fast` (which draws every 10th update) the update that reaches the last frame asks for a draw after it; a `-fast` run and a `-drawevery` run of the same input now match line for line. Normal play is unchanged | 21c03eda |
| (test only) Six chapters' engine states parted where a lost turn's grid click ends the round | `EndLostTurnRound` sets state 5, then 1, in the click's update, in both programs; the original's watch polls every millisecond and sees that passing 5 only sometimes, the port logged once an update | `-statelog` writes every `BattleEngine::SetState`, and `statecmp.py` lets a state the port leaves in the update it entered go unmatched | 21c03eda |
| (test only) The port's replay made its engine one update before the original's, or up to 30,000 updates off when the startup load went unmeasured | `calib.py` gave a chapter load as a length from the port's own load start | `-chapterloadupdates=@<update>`: the load ends where the original made its engine; `case.sh` anchors on that update when the original's mark came later | 21c03eda |
| (test only) On a busy machine the original's first click was given up at update 3515 and the script ran on the wall clock (5.3, 6.6) | origrun's `at` waits 30 s of virtual time, and the startup load runs on the wall clock while the updates stand still | the original's script starts with a `waitfor` on its own update counter; a run whose input was given up is marked `ORIGINAL RAN FREE` | see the commit after 21c03eda |
| Chapter 5.4: a plague tile spread onto another letter than in the original (the R of MOZARAB instead of a J), so the word did 3 damage where the original's killed | The plague's spread (`poisonedtile.luc`, `GetRandomizedAdjacentTiles` 0x419150) shuffles with the C library's `rand`, which `SexyAppBase::Init` (0x558ef0) seeds with `srand(GetTickCount())`; the port's copy (`poplua_rand`) was never seeded, and the conversation's typing (`ConversationPanel::Update` 0x428860) draws `rand() % 10` for one of ten sounds where the port played the first always | `GameApp::Init` seeds `poplua_rand` from the tick count (1 under `-seed`); the typing sound is drawn as the original's; the patch list's `crtseed` line seeds the original's with 1 (`srand(GetTickCount())` at 0x559432 becomes `srand(1)`) | see the commit after b66ae6ce |

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
- **A scramble starts one update later in the original** (chapter 6.5, 2026-11-15). After the same Scramble click the
  original's first letter change comes two updates after the click and the port's one update after, while the scripts'
  burst of 22 draws 52 updates after the click lands on the same update in both. The letter changes then follow
  `mUpdateCnt % mScrambleStep`, so the last of the 13 steps falls after two more of the per-update animation draws than
  in the port: the first 12 steps give the same letters (proved by simulating them from the original's peeked Mersenne
  state), the last one gives others, and the battle parts there. `Board::Scramble` (0x414fb0) and the step code in
  `Board::Update` (0x4167b5) are the port's; shifting the port's inputs moves the burst with the steps, so it is no
  harness offset. Not found within the hour: where the original loses the update between the click and the first step
  (to see it: the case `after-6.5`, `drawwin.py` around +615; what it would take: the original's button and focus path
  for the Scramble button, from the click to `Board::Scramble`).
- **After a Scramble, the Year of the Rooster's attack hurts Lex in the port and not in the original** (chapter 5.3,
  2026-11-16). Lex scrambles (the case's input at replay update 14168), which passes the turn, and the Rooster attacks.
  In the port its attack animation's `causeDamage` command (frame 141 or 176 of `4YearRooster.pam`) reaches
  `PAMAttackWrapper:UpdateAttack` at 14287, which runs `PerformAttack` for each of the attack's five effects (ten
  script draws, `DefendBaseClass` and `PlayAttackSound`, and 64 more from C++), and Lex goes from 8 to 6.25. The
  original makes no draw at all from the scramble's last step to well past the turn, its attack state ends 47 updates
  sooner, and at the following companion turn the Cheshire Cat (who purifies Lex when `CanPurify` or
  `GridNeedsPurification`, every fourth turn) does nothing, where the port's purifies for 257 updates. The companion
  was a consequence: both programs count the same four companion turns. The draws inside the battle agree up to the
  attack, the scramble's steps keep the one-update lag of the 6.5 oddity, and nothing in the Lua differs (the same
  scripts run). Seeding the C library's `rand` alike in both (Changes, chapter 5.4) left it as it was, the same
  first difference in a rerun. The treasure room was checked by shots of the original: it picks the same three.
  Not found within the hour: why the original's attack never reaches `PerformAttack` (to see it: the
  case `after-5.3`, the `lua` script command in the replay around 14280; what it would take: Lex's health and the
  Rooster's `mCurrentAttack` and its `mAnim.mMessageMap` read from the original's memory during the attack, or its
  lua.log with a print in `UpdateAttack`, to tell a damage message that never came from one that was skipped).
- **The original's draws before a battle varied between runs until 2026-11-15** because this worktree's ignored
  `tools/origrun/patches/bookwormadv2.txt` had lost its reseed lines (the battle engine's and the mini-game's); the
  chapter batch before then compared against an unseeded original, and is rerun.

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

- **Memory use: the sequel's port takes about three times the working set of Bookworm Adventures 1's** (the user's observation, 2026-11-15, both running side by side; not measured here). Staged assets are 87 MB against 43 MB. Measure first: the working set at the title screen, in the first battle and after the same number of updates, per game, and find which images and sounds account for the gap. Suspects: decoded 32-bit images held per chapter (`GAME_NOTES.md` section 1: the exe never unloads a chapter's resources, and the port may keep that), sounds and music decoded whole, and a leak in the port (a working set that keeps growing in a soak).

## Played with real input (before the final merge)

Web build, 2026-11-07, in the Browser pane at `?silent=1` with requestAnimationFrame shimmed: the page's script sent a mouse move, a pause, then a press and release on the canvas for every click (no game hooks). Played so far: the launch, the intro cut scene (Skip), the prologue's conversation clicks, the tutorial battle's word P-L-A-Y and the Attack button (the enemy took the hit, the board refilled); then, from a profile with the mini-games unlocked, the main menu, the Mini Games menu and **Word Up from its welcome to its Game Over screen** (six words clicked and submitted, Submit and Scramble drawn as in the original, score 28,000, Hall of Fame entry made), and **Mutant Words** up to the end of its timer (Game Over screen, Hall of Fame). No console error in any of it. Then **Golden Coins** (five words, 30,875 points, Game Over screen with its overfill bonus, Hall of Fame entry), **Link n' Spell** (words OIL and EGO linked and submitted, tiles turned to silver, 600 points, timer to zero, Game Over), **Letter Rip** (words TOT and PINE from POTENTIAL, timer to zero, Game Over) and **Word Master** (five guesses with the gold, silver and red feedback, the Game Over screen naming the mystery word). The page's script read the board's letters from the log through F9, a key the port logs the board's tiles and Submit button state on (`GameApp::UpdateAppStep`, a test aid that only logs), and sent the clicks as real DOM events; no console error in any mini-game. A click made while the tiles still fell or the hint card was up did nothing, as in the original (the first Letter Rip word had to be clicked again). **Not played yet:** a boss through to the map song and music after a focus loss, the later chapters, timed mode, the native pass (keys and burst-posted clicks) and full screen natively. The page's own full screen cannot be tested by an agent (requestFullscreen needs a real user gesture, which dispatched events are not); it is left for the user.

## The standard test commands (`docs/TEST_COMMANDS.md`)

Ported from Bookworm Adventures' (`TestHooks.cpp`, `GameApp::HandleDriverCommand`), reachable from `-script`, `-commands` and the web test page only; a command never given changes nothing, and the baseline check stays identical. Each answers one log line. `state`: `STATE {json}` with `screen` (title, menu, minigames, minigame, battle, treasure and so on), `mode`, `level`, `curtain`, `dialogs`, `talk`, and on a board `health`, `enemy`, `board` (a row of letters per line, `+` on a tile that can be picked), `paused`, `busy`. `goto menu|adventure|arena|minigames|tome|options|highscores|credits|<mini-game class>`; `unlock all`; `set health|enemy <n>`; `win` (the scripts' `_KillEnemy`); `lose` (`_lex_near_death`); `move` (the bot's next move: a dialog's button, a conversation click, the longest word and Attack or Scramble, Word Master's guess, a treasure, the map's Enter) as `MOVE {...}` with its input in game pixels, or `MOVE null`; `level` and `seed` answer ERROR (the chapter is the map's, the seed is `-seed`). `statedump` is the old verbose board dump (F9 in a test run). Tried so far: from a saved profile `goto minigames`, `goto goldencoins`, `state`, `move` (the welcome dialog's button).

## Deviations kept (listed for the final merge)

- **The PopCap logo at start is gone.** The original fades it in, holds it for `PopCapLogoTimer` and fades it out (about 356 updates, no click leaves it) before it loads anything, a wait that only costs the player time, so the port goes straight to the curtain and the load (`TitleScreen::Update`). `-popcaplogo` brings it back for comparisons with the original; `tools/check.sh`'s runs and `tools/identical/port.sh` pass it.

## The focus loss after a chapter boss (run 2026-11-11, native, in phase one as the coordinator asked)

`games/bookwormadv2/tools/bossfocus.py` plays from the `bosschapter1` profile (book 4, chapter 1) by the port's own bot through `tools/playbymove`'s loop (real `-realinput` moves, the `state` and `move` commands), beats chapter 1's boss (Dish and Spoon) and chapter 2's (Old MacDonald), and posts `focus lost`, 300 updates, `focus gained` at two moments: when the boss's health reaches 0 and when the map that follows it is up. `state` now carries the music (`song`, `playing`: the last song the scripts asked for, and whether it or a module song plays). Result: PASS after 1,433 moves; on the map the song playing before the focus loss (song 0, playing) is playing after the regain, and at the boss's victory the state is the same before and after (song 40, not playing: the treasure song's module is not started in this run, whose audio is SDL's silent driver). The run does not stall the game loop while a song plays, which is how the original loses the map song (a hidden browser tab), so this checks that a focus loss and regain breaks nothing; the stalled-loop mechanism itself is covered by the queued-song fix (ad1245ca) and its reasoning above, not run. The user's own case stays to be confirmed in the web build with a hidden pane.

## Plan still open (written 2026-11-07, in this order)

1. The standard test commands (`docs/TEST_COMMANDS.md`, after merging the trunk): `state` (STATE {json}), `goto`, `level`, `unlock all`, `set`, `win`, `lose`, `seed`, sharing code with `TestHooks.cpp`; an ungiven command changes nothing, `tools/check.sh bookwormadv2` stays baseline-identical. F9 (a test aid, now only in a `-script` or `BYOPOP_TEST` run) goes when the live channel (`-commands`, `byopop.test.command()`) lands.
   Also a `move` test command (spec in `docs/TEST_COMMANDS.md`, main 6d7d4cb2): prints `MOVE {...}` with the word or mini-game move the bot would play and the real input for it (tile clicks and Submit click, game coordinates), or `MOVE null`; it changes nothing and draws from its own generator, so a later bot mod reuses the bot. The word choice lives in the port, not in the driver. `move` also answers what waits for a click and is not a word: the prologue's cut scene (a click that asks to skip it; the dialog after it is answered like any dialog), the first battle's potion tutorial (a click on the red, blue and green potion in turn, as the profile's `TutorialNeedsHealth`, `TutorialNeedsPower` and `TutorialNeedsPurify` flags ask, once the player holds that potion; a potion that leaves its tutorial standing is not asked again for 120 updates), the second battle's scramble tutorial (a click on Scramble while the profile's `TutorialScrambleArrow` is true; the key stays, false, afterwards), the conversation panels, arrows with a hot spot and overlays. Result: from an empty player folder the bot plays the prologue and goes on into book 4 (5,000 moves reach chapter 4's Candlestick), real events only. `tools/playbymove/playbymove.py --nulls 400 --stuck 600 --first "wait 900" --first "click game 400 435" --first "wait 300" <exe> -gamedir=... -userdir=<empty dir> -popcaplogo` plays a new player from the title through the prologue by real input only (the `--nulls` and `--stuck` limits are raised because the curtain and the panels fade while the bot asks).
2. A thin driver under `games/bookwormadv2/tools/` (run in the page; reads the log, picks the longest words, sends real DOM events; one call plays a mini-game or battle and returns a summary), reaching battles and mini-games through the commands.
   Model it on Bookworm Adventures' driver (branch `claude/bookwormadv-78ebd4`, from d32a4607): `GameApp::LogBoardForTests` in its `TestHooks.cpp` (a `-boardlog=<file>` line `board: <n> <WORD|-> x,y ... attack x,y scramble x,y` when the board comes to rest, the longest word from the game's own word list; look buttons up with `mButtons.find`, skip boards where no tile `CanSelect`), `tools/realrun.py` natively and `webdrive.js` in the page (on the web the option is added by wrapping `Module.callMain` and the log read back with `Module.FS.readFile`).
3. A boss through to the map song, the music after a focus loss, a later chapter, timed mode.
4. The native pass: burst-posted clicks and keys (approach of `games/sevenseas/tools/realinput.py`), windowed and full screen, under rundesk.
5. Before the turn ends: stop the web test server on 18790 and delete `build/web/zz_*`.

## Remaining deviations

None known in the logic that was compared. Everything above "Not covered" is untested, not known to differ.

## Registration and outbound buttons

The port has no trial, nag, unlock or register path of its own (it was ported as the full game), and its main menu, options, credits and
dialogs have no "More Games", buy or other button that leaves the game: no OpenURL or shell call exists in `src/`. What was left of the
trial is removed: the `UpsellTrailer` resource group (its "buy now" and "no thanks" buttons, the "free trial expired" picture and sounds) and
the main menu's "buy now" voice (`tools/resgen.py` leaves them out of `Res.h`/`Res.cpp`), the two trial dialogs (`DIALOG_NEW_PROFILE`,
`DIALOG_TRIAL_COMPLETE`, which were never made) with their answers, the `mQueuedHydraLockout` flag with the checks on it (always false), and the
comments about what the original did in its trial. Two functions stay because the original's scripts call them: the scripts' `QueuedHydraLockout`
and `ShouldDoHydraLockout` (constants, `false`), so a script that asks still gets an answer.

## Chapter-by-chapter batch (rerun 2026-11-16, the original reseeded, from the bot-saved profiles `after-4.1` .. `after-6.10`)

`tools/identical/chapters.sh` runs the comparison case for each saved profile: Adventure, the map's Enter, then the fast
bot (`fastkill`: the highest-scoring word, Scramble on a board without a nine-letter word or a one-hit kill, a red potion
at half health) plays until the map shows again (`chapterend`), at most 15,000 updates. The original's inputs are
anchored on its battle engine (origrun's `waitfor` and `mark`) and the port's replay is shifted to match; a case takes
four to seven minutes. The batch of 2026-11-12/13 and the first runs of 2026-11-15 compared against an unseeded original
(the ignored patch list had lost its reseed lines, see Oddities) and are void.

Each case is named by the profile it starts from, so `after-4.1` plays chapter 4.2. The third rerun (2026-11-16
evening, all 29, with the C library's `rand` seeded alike in both, see Changes, chapter 5.4) took about 60 minutes,
three cases at once (`JOBS`; one at a time a case takes five to six). Of the 29:

- **Identical engine states** (20): `after-4.1`, `after-4.2`, `after-4.3`, `after-4.4`, `after-4.8`, `after-5.1`,
  `after-5.2`, `after-5.4`, `after-5.5`, `after-5.7`, `after-5.8`, `after-5.9`, `after-5.10`, `after-6.1`, `after-6.4`,
  `after-6.6`, `after-6.7`, `after-6.8`, `after-6.9`, `after-6.10`, all within 6 updates (`after-5.9`'s original ran free
  only after its last state). `after-5.4` was a real difference until the `rand` fix: its word did 3 damage where the
  original's killed (a plague tile had spread onto another letter). Two keep a draw difference inside a battle:
  `after-5.5` (+1: a draw the scripts make every update runs one update longer in the original, before Lex's death;
  the counts agree again after the battle restarts) and `after-6.9` (+1). The six chapters that once had "the original
  goes to state 5 where the port attacks" were the lost turn's passing state (Changes).
- **One change 3 to 7 updates apart** (5): `after-4.5`, `after-4.9`, `after-5.6`, `after-6.2`, `after-6.3`. The gap
  opens at the end of an attack (state 3 to 5, 7 or 9) and closes at the next input. It is the original's own
  variance: `after-4.5`, `after-4.6` and `after-6.2` each had such a gap in one run and none in another, with the same
  port and the same input. Not a difference of the port.
- **Oddities** (2): `after-5.3` (after a scramble the enemy's attack hurts Lex in the port and not in the original,
  so the port's companion then purifies Lex where the original's does nothing) and `after-6.5` (the scramble's
  one-update lag). Both under Oddities.
- **Not comparable this run** (2): `after-4.6` and `after-4.7`, whose originals stood still before their battles (the
  counter stopped at 5,900 and 16,414). `after-4.6` was comparable in earlier runs (a 7-update gap, above); `after-4.7`
  never was: its original stops (11,514, 12,734 and 16,414 in three runs) as its Flash movies do in the sandbox on the
  virtual clock.

The full verdict lines are in `build/chapter-results.txt` and `build/chapter-logs/` while they last.
