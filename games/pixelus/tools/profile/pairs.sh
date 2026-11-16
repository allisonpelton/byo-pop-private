#!/bin/bash
# pairs.sh <out dir> [pairs]: the performance pass's measurement (GAME_NOTES.md, "Performance"): the original and the port play the
# same scripted stretch (build/perf/port-play.txt and orig-play.txt, made as the notes say), in real time, windowed at the game's
# own size, one game process at a time, the order flipping every pair, on one performance core of its own (tools/pincore.py: this
# script starts itself under it). Processor time is cycle counts (QueryProcessCycleTime; perf.py), per update over 6000 updates.
# Needs the port and origrun built and the original staged (games/pixelus/tools/identical/setup.sh). Run it in the background.
cd "$(dirname "$0")/../../../.." || exit 2
if [ -z "${PINCORED:-}" ]; then
	export PINCORED=1
	exec python -I tools/pincore.py -- bash "$0" "$@"
fi
OUT="$1"; WANT="${2:-5}"; WAIT_S="${WAIT_S:-3600}"
P=games/pixelus/tools/profile/perf.py
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"; : > "$OUT/runs.txt"
start=$(date +%s)
until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do
	[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
	sleep 15
done
echo "start $(date '+%F %T'), waited $(( $(date +%s) - start )) s" >> "$OUT/runs.txt"
one() { local kind="$1"; shift; echo "$kind $("$@" 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"; }
orig() { one orig python -I $P orig play "$OUT/orig$1" build/perf/orig-play.txt; }
port() { one port build/native/rundesk.exe "$PY" -I $P port play "$OUT/port$1" build/perf/port-play.txt; }
for i in $(seq 1 "$WANT"); do
	if [ $((i % 2)) -eq 1 ]; then orig $i; port $i; else port $i; orig $i; fi
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
