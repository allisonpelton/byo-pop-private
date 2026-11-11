#!/usr/bin/env bash
# Tour R from end to end (tour_restart.py): run A and run B of the port and of the original, run B of each from what its own
# run A left, then the two comparisons.   games/typershark/tools/phase2/restart.sh [A name] [B name]
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
A="${1:-rs_a}"; B="${2:-rs_b}"
P=games/typershark/tools/phase2
python $P/tour_restart.py a "$A" && python $P/tour_restart.py b "$B" || exit 1

# the port: run A, then run B from run A's user folder
bash $P/runport.sh "$A" 1 -drawevery | tail -1
FIX="build/ts-p1/fixtures/restart-$A"
rm -rf "$FIX"; cp -r "build/ts-p2/$A/ud" "$FIX"
USERS="restart-$A" bash $P/runport.sh "$B" 1 -drawevery | tail -1
rm -rf "$FIX"

# the original: run A, then run B from run A's users folder and registry values
# REGMAP=1 runs both of the original's runs with --reg-map popcap. Inside the sandbox the copy runs as SYSTEM and writes
# HKLM itself, so the map is not needed there, and with it on the original's timing is another (IDENTICAL_PLAY.md).
MAP=""; [ -n "${REGMAP:-}" ] && MAP="--reg-map popcap"
bash $P/runorig.sh "$A" 1 500 $MAP
REGS="build/ts-p2/$A/registry-after.reg"
: > "$REGS.list"
for f in build/origrun/typershark-p2/logs/registry-after-*.reg; do
	[ -f "$f" ] && echo "$f $(wc -c < "$f") bytes" >> "$REGS.list"
done
# the export that holds the game's key with its players (under HKLM, or under HKCU with the map)
best=""
for f in build/origrun/typershark-p2/logs/registry-after-*.reg; do
	[ -f "$f" ] || continue
	if iconv -f UTF-16 -t UTF-8 "$f" 2>/dev/null | grep -q "PopCap.TyperShark.Users"; then best="$f"; fi
done
[ -n "$best" ] || { echo "no registry export of run A holds the game's Users key"; cat "$REGS.list"; exit 1; }
cp "$best" "$REGS"
echo "run A's registry: $best ($(wc -c < "$REGS") bytes)"
grep -h "registry map" build/origrun/typershark-p2/logs/*.log | tail -2 | cut -c1-300
STAGE="build/ts-p2/$A/orig.users" REG="$R/$REGS" bash $P/runorig.sh "$B" 1 400 $MAP
grep -h "registry map\|registry.reg\|keys are there" build/origrun/typershark-p2/logs/*.log | tail -4 | cut -c1-300

for N in "$A" "$B"; do
	python tools/shotdiff/shotdiff.py --game typershark --orig "build/ts-p2/$N/orig" --port "build/ts-p2/$N/port" --run "p2-$N" | tail -1
	python $P/sumshot.py "p2-$N" | head -30
done
echo "restart done"
