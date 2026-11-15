#!/bin/bash
# run.sh <name> <origrun script> [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host: or.sh's way
# (a copy under build/origrun/<name>/game, windowed, invisibly under rundesk through origrun, the game's step counter
# 0x460a1c as the script's clock) with origrun's --coverage on the list that `python -I tools/coverage.py list cosmicbugs`
# writes. The virtual clock needs the main loop to wait: the unpatched loop waits only for Present(1), which does not wait on
# the hidden desktop, so a patch of 11 bytes at 0x41f063 (a TimeMs call and a subtraction whose result nothing reads) makes
# each pass a SleepEx(10, 0), ten updates a frame. REAL=1 runs on the wall clock instead (the sound code then runs, on the
# silent stand-in) with the loop paced at 16 ms. SAVE=<cb.dat> starts with that save; HS=1 makes every game over earn a high-score entry (the patches at 0x4135bb, in the board's own game-over click, and 0x4173cb, the game-over menu item: `setle dl`; a random game scores about 140 and the table's last is 10000); LIVES1=1 patches the original's new game to start with one life (the patch at 0x4170d8, `add edx, 6` in 0x416e10's start-game item). The hits go to
# build/coverage/cosmicbugs/hits/<name>.txt, the run's log beside them.
cd "$(dirname "$0")/../../../.."
N=$1; S=$2; T="${3:-600}"
C=build/coverage/cosmicbugs; H=$C/hits; mkdir -p $H
[ -f $C/functions.txt ] || python -I tools/coverage.py list cosmicbugs > /dev/null
MS=10; CLOCK=virtual; [ -n "$REAL" ] && { MS=16; CLOCK=real; }
printf 'test CosmicBugs.exe va:0x41f063 e8089e00002b051c656e00 6a006a%02xff156081450090 each pass of the main loop sleeps %d ms, as the wait for a screen refresh would\n' $MS $MS > $C/patches-$N.txt
[ -n "$LIVES1" ] && printf 'test CosmicBugs.exe va:0x4170d8 83c206 83c201 a new game starts with one life, not six (the lives are 6 added to 0 or 299)
' >> $C/patches-$N.txt
[ -n "$HS" ] && printf 'test CosmicBugs.exe va:0x4173cb 0f9ec2 31d290 a game over always earns a high-score entry (the score is not compared with the last of the table, 10000), in the menu item that ends a game over\n' >> $C/patches-$N.txt
[ -n "$HS" ] && printf 'test CosmicBugs.exe va:0x4135bb 0f9ec2 31d290 the same where a board that ends in a game over asks it (the click after the last life)\n' >> $C/patches-$N.txt
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
rm -rf build/origrun/$N/coverage.txt
bash games/cosmicbugs/tools/play/or.sh "$N" "$S" --patches "$(cygpath -wa $C/patches-$N.txt)" --clock $CLOCK \
	--coverage "$(cygpath -wa $C/functions.txt)" --watch-max 100000 --timeout "$T" > /dev/null
cp build/origrun/$N/coverage.txt $H/$N.txt || { echo "$N: no hits file"; exit 1; }
cp build/origrun/$N.out $H/$N.log
echo "$N: $(grep -vc '^#' $H/$N.txt) functions; $(grep '^#' $H/$N.txt | tail -1)"
