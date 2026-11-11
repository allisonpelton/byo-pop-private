#!/usr/bin/env bash
# Tour D from end to end (tour_missing.py): run A and run B of the port and of the original, run B of each from what its own run A
# left except for the player's data file (users/user0.dat, deleted), then the two comparisons. Built on restart.sh (same registry
# carry-over; REGMAP is on for the original's two runs).   games/typershark/tools/phase2/missing.sh [A name] [B name]
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
A="${1:-md_a}"; B="${2:-md_b}"
P=games/typershark/tools/phase2
python $P/tour_missing.py a "$A" && python $P/tour_missing.py b "$B" || exit 1

# the port: run A, then run B from run A's user folder without the player's data file
bash $P/runport.sh "$A" 1 -drawevery | tail -1
FIX="build/ts-p1/fixtures/missing-$A"
rm -rf "$FIX"; cp -r "build/ts-p2/$A/ud" "$FIX"
ls "$FIX/users" | tr '\n' ' '; echo "(port, after run A)"
rm -f "$FIX/users/user0.dat"
USERS="missing-$A" bash $P/runport.sh "$B" 1 -drawevery | tail -1
rm -rf "$FIX"

# the original: run A, then run B from run A's users folder (without user0.dat) and registry values
bash $P/runorig.sh "$A" 1 400 --reg-map popcap
REGS="build/ts-p2/$A/registry-after.reg"
best=""
for f in build/origrun/typershark-p2/logs/registry-after-*.reg; do
	[ -f "$f" ] || continue
	if iconv -f UTF-16 -t UTF-8 "$f" 2>/dev/null | grep -q "PopCap.TyperShark.Users"; then best="$f"; fi
done
[ -n "$best" ] || { echo "no registry export of run A holds the game's Users key"; exit 1; }
cp "$best" "$REGS"
ls "build/ts-p2/$A/orig.users" | tr '\n' ' '; echo "(original, after run A)"
rm -f "build/ts-p2/$A/orig.users/user0.dat"
STAGE="build/ts-p2/$A/orig.users" REG="$R/$REGS" bash $P/runorig.sh "$B" 1 400 --reg-map popcap

for N in "$A" "$B"; do
	python tools/shotdiff/shotdiff.py --game typershark --orig "build/ts-p2/$N/orig" --port "build/ts-p2/$N/port" --run "p2-$N" | tail -1
	python $P/sumshot.py "p2-$N" | head -30
done
echo "missing done"
