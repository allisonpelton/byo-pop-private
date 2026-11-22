#!/bin/bash
# The original's side of a 2D tour on the host, shots through origrun's in-game frame reader (docs/ORIGRUN.md, "The in-game frame
# reader"): the same script as run_tour_original.sh, no Windows Sandbox. READER_TRIAL=1 for the trial run (a game whose config has
# no `reader_trial=passed` line); the surface and the waits are the config's, or these (the 2005 framework's layout: mDDInterface
# at 0x36c of the app at 0x67ccd8, mDrawSurface at +0x68 (found by peeking the DDInterface: 640 and 480 follow it); Process (0x4bb9b0) waits in its Sleeps, return addresses 0x4bbedb
# and 0x4bbf1f).
#   [PROFILE=<folder>] [READER_TRIAL=1] run_tour_reader.sh <recording dir> <shots dir> [seed] [timeout]
rec=$1; out=$2; seed=${3:-909}; timeout=${4:-1200}
cd "$(dirname "$0")/../../../.." || exit 2
RUN=${ORIG_RUN:-bej2h}; export ORIG_RUN=$RUN
tag=$(basename "$rec")
python games/bejeweled2/tools/origcmp/to_origrun.py "$rec/events.json" "build/origrun/bej2_$tag.txt" ${GATE:+--gate "[[0x67ccd8]+0xe78]+0xa0"} > /dev/null || exit 2
rm -rf build/origrun/$RUN/shots
mkdir -p build/origrun/$RUN/shots "build/logs/bej2/og_$tag"
WATCHES='[[0x67ccd8]+0xe68]+0x15e20 [[0x67ccd8]+0xe68]+0x15e4 [[0x67ccd8]+0xe68]+0x170c [[0x67ccd8]+0xe68]+0x15e0' REG2D=1 \
ORIGRUN_ARGS="--counter [0x67ccd8]+0x484 --pending-draw [0x67ccd8]+0x45a --update-state [0x67ccd8]+0x488 --draw-empty [0x67ccd8]+0x459 --reader-surface [[0x67ccd8]+0x36c]+0x68 --reader-waits ${READER_WAITS:-4bbf1f,4bbedb} ${READER_TRIAL:+--reader-trial}" \
  bash games/bejeweled2/tools/origcmp/run_original.sh "build/origrun/bej2_$tag.txt" "build/logs/bej2/og_$tag/origrun.log" "$seed" "$timeout"
rc=$?
rm -rf "$out"
mv build/origrun/$RUN/shots "$out"
mkdir -p build/origrun/$RUN/shots
echo "$tag: rc=$rc, $(ls "$out"/*.png 2>/dev/null | wc -l) shots in $out"
exit $rc
