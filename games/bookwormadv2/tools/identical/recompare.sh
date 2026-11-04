#!/bin/bash
# recompare.sh <name> [seed]: case.sh's last steps again on the original's log a case already made (build/identical/<name>/orig.log):
# the port replays the case's input with the load lengths of that log, and the draws and engine states are compared
source "$(dirname "$0")/common.sh"
N="$1"; SEED="${2:-1}"; D="$OUT/$N"
[ -n "$PROFILE" ] && export PROFILE="$D/profile"
portrun() { if [ -n "$PROFILE" ]; then rm -rf "$1/user"; mkdir -p "$1"; cp -r "$PROFILE" "$1/user"; KEEP_USER=1 bash "$TOOLS/port.sh" "$@"; else bash "$TOOLS/port.sh" "$@"; fi; }
portrun "$D/cal" -seed=$SEED -reseedengine=$SEED -startloadupdates=820 -chapterloadupdates=${CALLOAD:-4107} -script="$D/script.txt" -statelog="$D/cal/state.txt" > /dev/null
OPTS=$(python "$TOOLS/calib.py" "$D/orig.log" "$D/cal/state.txt")
echo "load lengths of the original: $OPTS"
portrun "$D/rep" -seed=$SEED -reseedengine=$SEED $OPTS -script="$D/replay.txt" -statelog="$D/state.txt" > /dev/null
python "$TOOLS/rngcmp.py" "$D/orig.log" "$D/state.txt"
