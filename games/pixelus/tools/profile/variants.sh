#!/bin/bash
# variants.sh <out dir> [rounds]: the memory pass's measurement (GAME_NOTES.md, "Memory"): each round runs the original and the port on its two
# paths at 1x and 2x (the variables go through `env`: a plain VAR=x before the function `one` did not reach the port, and a first run
# of this script measured the same build fifteen times), the player who has 3D off (Is3D 0 saved, as the original ran under origrun), one game process at a time, on one
# performance core of its own (tools/pincore.py), the order rotating each round. Needs build/native/pixelus-soft.exe (the build with
# CreateDDInterface returning the software screen) and pixelus-gpu.exe (the same without it, so the GPU draws 2D too).
#   soft1x  gpu1x  gpu2x (the software screen is for 1x only, the user, 2026-11-15: the GPU takes every larger window)
cd "$(dirname "$0")/../../../.." || exit 2
if [ -z "${PINCORED:-}" ]; then
	export PINCORED=1
	exec python -I tools/pincore.py -- bash "$0" "$@"
fi
OUT="$1"; WANT="${2:-5}"
P=games/pixelus/tools/profile/perf.py
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"; : > "$OUT/runs.txt"
export PERF_2D=1
until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 15; done
one() { local kind="$1"; shift; until python -I tools/gamecount.py --limit 5 > /dev/null 2>&1; do sleep 15; done  # a run starts only with 4 or fewer game processes running (the coordinator, 2026-11-16)
	 echo "$kind $("$@" 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"; }
run() {  # <variant> <round>
	case "$1" in
	orig) one orig python -I $P orig play "$OUT/orig$2" build/perf/orig-play.txt
		# pinned, the original's window can take over 20 s to appear, its first click is lost and it idles (BookWorm, 2026-11-15)
		grep -q "no window in the first" "$OUT/orig$2/origrun.log" 2>/dev/null && echo "orig-no-window-run $2 (the run above idled: do not use it)" >> "$OUT/runs.txt" ;;
	soft1x) one soft1x env PERF_EXE=pixelus-soft.exe build/native/rundesk.exe "$PY" -I $P port play "$OUT/soft1x$2" build/perf/port-play.txt ;;
	gpu1x) one gpu1x env PERF_EXE=pixelus-gpu.exe build/native/rundesk.exe "$PY" -I $P port play "$OUT/gpu1x$2" build/perf/port-play.txt ;;
	gpu2x) one gpu2x env PERF_2X=1 PERF_EXE=pixelus-gpu.exe build/native/rundesk.exe "$PY" -I $P port play "$OUT/gpu2x$2" build/perf/port-play.txt ;;
	esac
}
ALL=(orig soft1x gpu1x gpu2x)
for i in $(seq 1 "$WANT"); do
	for k in 0 1 2 3; do run "${ALL[$(( (k + i) % 4 ))]}" "$i"; done
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
