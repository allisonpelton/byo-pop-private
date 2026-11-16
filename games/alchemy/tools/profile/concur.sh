#!/bin/bash
# concur.sh <out dir> [rounds]: the original and the port at 1x at the same time, each on a performance core of its own
# (tools/pincore.py), over the short stretch of pairs.sh's QUICK mode (2500 updates from update 600, the script quits at update
# 3200). A pair then sees the same machine (the clock speed, the other sessions' load) where alternating runs see different ones,
# which is most of what moved the cycles of identical runs by a third (GAME_NOTES.md, "The software screen at 1x"). <out dir> holds
# what pairs.sh QUICK=1 wrote (bot.txt, port.txt, orig.txt, ref.json: copy them); each round's two JSON lines go to runs.txt.
#   bash games/alchemy/tools/profile/concur.sh build/cyc/pairs6 30
# Do not wrap it in pincore: each of the two runs is. LIMIT (default 5) is the count of game processes gamecount.py waits to be
# under before a round, which adds two.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; N="${2:-30}"; LIMIT="${LIMIT:-5}"
C=games/alchemy/tools/profile/cycles.py
PY="$(cygpath -w "$(command -v python)")"
R=build/native/rundesk.exe
FIRST=600; COUNT=2500; SEED=11
for f in port.txt orig.txt ref.json; do [ -f "$OUT/$f" ] || { echo "no $OUT/$f"; exit 2; }; done
export CYC_REF="$OUT/ref.json" CYC_MIN_MARKS=0
echo "start $(date '+%F %T'), concurrent, $N rounds, from update $FIRST, $COUNT updates" >> "$OUT/runs.txt"
for i in $(seq 1 "$N"); do
	until python -I tools/gamecount.py --limit $LIMIT > /dev/null 2>&1; do sleep 20; done
	python -I tools/pincore.py -- python -I $C orig "$OUT/o$i" "$OUT/orig.txt" $SEED $FIRST $COUNT > "$OUT/o$i.json" 2>> "$OUT/errors.txt" &
	PO=$!
	CYC_WINDOWSCALE=1 python -I tools/pincore.py -- $R "$PY" -I $C port "$OUT/q$i" "$OUT/port.txt" $SEED $FIRST $COUNT > "$OUT/q$i.json" 2>> "$OUT/errors.txt" &
	PP=$!
	wait $PO $PP
	echo "orig $(tail -1 "$OUT/o$i.json")" >> "$OUT/runs.txt"
	echo "port1x $(tail -1 "$OUT/q$i.json")" >> "$OUT/runs.txt"
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
