#!/bin/bash
# check.sh <name> <bot arguments...>: one comparison, the port's bot and the original on the same input; prints identical.py's report.
# Run it under rundesk from the worktree: build/native/rundesk.exe bash games/mummymaze/tools/identical/check.sh <name> ...
N="$1"; shift
R="$(pwd -W)"; T="$R/games/mummymaze/tools/identical"; D="$R/build/phase1"; mkdir -p "$D"
bash "$T/run_port.sh" "$R" "$D/$N.port.txt" "$@"
OFF=${OFF:-2301}
python -I "$T/identical.py" script "$D/$N.port.txt" "$D/$N.script.txt" --offset "$OFF"
# the script ends with quit; the timeout is only a cap, at a worst-case 500 updates a second
LAST=$(grep -o "^at [0-9]*" "$D/$N.script.txt" | tail -1 | cut -d" " -f2)
TO=${TIMEOUT:-$(( (LAST + 3000) / 500 + 30 ))}
bash "$T/run_orig.sh" "$R" "$D/$N.script.txt" "$D/$N" "$TO"
python -I "$T/identical.py" compare "$D/$N.port.txt" "$OFF" "$D/$N.A.log:A" "$D/$N.B.log:B" "$D/$N.C.log:C" "$D/$N.D.log:D" "$D/$N.E.log:E" "$D/$N.F.log:F"
