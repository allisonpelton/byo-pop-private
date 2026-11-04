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

The soak (the scripted player) does not get further than the early game on its own: later chapters, the bosses and the later
mini-games were not compared. Only the first battles of chapter 1 and Golden Coins were. Phase two (screens, dialogs, sound,
focus) and the original-to-port save direction are open.

## Open item for phase two: the web build's file case

The native port finds the original-case names through `filecase.txt` next to the staged files. The web build packs the assets
without that file, so on the site `GetFilesInDirectory` lists lower-case names and the book scripts would be loaded twice,
as before d8697ae. The web packer needs to include `filecase.txt` in the preloaded data.

## Remaining deviations

None known in the logic that was compared. Everything above "Not covered" is untested, not known to differ.
