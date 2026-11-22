#!/usr/bin/env bash
# tour-all.sh: the original's five screen tours one after another in Windows Sandbox (one run queued at a time; a queue
# can take hours, so OR_TIMEOUT is long), after mksaves.sh and tour-port.sh <tour> [save] for each tour. Then
# tour-compare.sh <run id> puts every tour's shots in one shotdiff run. Mission's original is patched to win at its
# first line (a patch list with the win line 0x412e2e `7d19` -> `eb19`, which the script builds from the usual one).
cd "$(dirname "$0")/../../../.."
export OR_TIMEOUT=21600
cp tools/origrun/patches/cosmicbugs.txt build/mission-patches.txt
printf '\nwin CosmicBugs.exe va:0x412e2e 7d19 eb19 the fill quota test (0x412e28, cmp eax,[ebx+0x3bc]) always passes: the first filled line wins the board\n' >> build/mission-patches.txt
T=games/cosmicbugs/tools/play/tour-orig.sh
PATCHES="$(cygpath -wa build/mission-patches.txt)" bash $T mission build/saves/unlocked.dat
bash $T end build/saves/onelife.dat
bash $T menus
bash $T players
bash $T board
