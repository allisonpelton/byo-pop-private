#!/usr/bin/env bash
# The saved adventure at the last site that mkprofile.py <ngc> saved takes its users folder from: the port plays the menu's Adventure, the
# first difficulty, `level 34` and `win`, and the game saves at site 35 (build/ts-p2/mk1/ud/users). No adventure is played in the original.
#   games/typershark/tools/phase2/mksaved.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
mkdir -p build/ts-p2/mk1
python - <<'PYEOF'
lines = ["701 click 510 70", "741 state", "801 click 320 164", "861 state", "901 level 34", "961 state", "1001 win"]
lines += ["%d state" % u for u in range(1061, 4000, 120)] + ["%d char SPACE" % u for u in (1500, 1700, 1900, 2100, 2500)]
lines += ["4001 quit"]
lines.sort(key=lambda l: int(l.split()[0]))
open("build/ts-p2/mk1/port.txt", "w", newline="\n").write("\n".join(lines) + "\n")
PYEOF
USERS=medal-ngc1 bash games/typershark/tools/phase2/runport.sh mk1 1 -drawevery | tail -1
test -s build/ts-p2/mk1/ud/users/user0.dat
