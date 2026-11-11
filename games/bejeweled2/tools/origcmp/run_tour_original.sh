#!/bin/bash
# The original's side of a tour (record.py --mode tour, tourload or tour:<mode>): the port's recording becomes an origrun script, the
# player's folder (PROFILE=<make_profile.py's or make_save.py's folder>) is staged into the sandbox, the original plays the same
# inputs at the same updates inside Windows Sandbox and takes each shot at the update it was taken in the port, and the shots end up
# in <shots dir>. Compare them with the port's by tools/shotdiff (docs/SHOTDIFF.md).
#   [PROFILE=<folder>] [OPTIONS=1] run_tour_original.sh <recording dir> <shots dir> [seed] [timeout]
# OPTIONS=1: the recording is cut after its `options` shot (the original crashes in the sandbox when the options dialog closes, GAME_NOTES,
# "Oddities").
rec=$1; out=$2; seed=${3:-909}; timeout=${4:-600}
cd "$(dirname "$0")/../../../.." || exit 2
tag=$(basename "$rec")
python games/bejeweled2/tools/origcmp/to_origrun.py "$rec/events.json" "build/origrun/bej2_$tag.txt" > /dev/null || exit 2
if [ -n "$OPTIONS" ]; then
  python - "build/origrun/bej2_$tag.txt" <<'PYEOF'
import sys
lines = open(sys.argv[1]).read().split('\n')
out = []
for l in lines:
    out.append(l)
    if l.endswith('shot options'):
        break
out.append('quit')
open(sys.argv[1], 'w').write('\n'.join(out) + '\n')
PYEOF
fi
rm -rf build/origrun/bej2/sandbox-files/C build/origrun/bej2/shots
mkdir -p build/origrun/bej2/shots "build/logs/bej2/og_$tag"
if [ -n "$PROFILE" ]; then
  mkdir -p build/origrun/bej2/sandbox-files/C/ProgramData/Steam/Bejeweled2
  cp -r "$PROFILE/users" build/origrun/bej2/sandbox-files/C/ProgramData/Steam/Bejeweled2/users
fi
for i in $(seq 1 120); do
  python tools/gamecount.py --limit 6 > /dev/null && break
  sleep 15
done
WATCHES='[[0x67ccd8]+0xe68]+0x15e20 [[0x67ccd8]+0xe68]+0x15e4 [[0x67ccd8]+0xe68]+0x170c [[0x67ccd8]+0xe68]+0x15e0' \
ORIGRUN_ARGS="--sandbox --reg $PWD/build/origrun/bej2_sandbox.reg --pending-draw [0x67ccd8]+0x45a --update-state [0x67ccd8]+0x488 --draw-empty [0x67ccd8]+0x459 $ORIGRUN_EXTRA" \
  bash games/bejeweled2/tools/origcmp/run_original.sh "build/origrun/bej2_$tag.txt" "build/logs/bej2/og_$tag/origrun.log" "$seed" "$timeout"
rc=$?
rm -rf "$out"
mv build/origrun/bej2/shots "$out"
mkdir -p build/origrun/bej2/shots
echo "$tag: rc=$rc, $(ls "$out"/*.png 2>/dev/null | wc -l) shots in $out"
exit $rc
