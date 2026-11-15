#!/bin/bash
# windows.sh <out dir> [rounds]: the performance measurement at the original's own window size and at the default window, side by side
# (docs/COORDINATION.md, "The target"; GAME_NOTES.md section 11, "Window state"). Each round runs the original, the port at the game's own
# size (PERF_WINDOW=1: ScreenMode=0 and -windowscale=1, render scale 1) and the port in the framework's default window (PERF_WINDOW=2:
# twice the size), one game process at a time, in real time, the order rotating every round, the whole loop on one performance core
# (tools/pincore.py). Environment:
#   MODE=2d  (default) rundesk, Direct3D 11 on its private desktop; the original through origrun (perf.py orig)
#   MODE=3d  the visible desktop, 22:00 to 08:00 only: the port with PERF_3D=1 PERF_DESKTOP=1, the original through perf.py orig3d
#   FIRST=4 APPEND=1  more rounds into the same folder (rounds FIRST to the last asked for; runs.txt is added to, not cleared)
#   SPLIT=1  the cycle split (perf.py PERF_SPLIT): -cyclesplit for the port, --count / countsnap for the original; a run's file
#            split.jsonl or counts.jsonl is kept in its folder, read with tools/cyclesplit.py <port> <orig> --from a --to b
#            (a split run is for the proportions; the target's total comes from a run without counting)
# Each run's last JSON line (with the port's `display:` line, which names the window and the renderer) goes to <out dir>/runs.txt after its
# kind. Waits first for room (at most 6 game processes on the machine). Run it in the background: it ends by itself.
cd "$(dirname "$0")/../../../.." || exit 2
if [ -z "${PINCORED:-}" ]; then
	PINCORE=tools/pincore.py
	[ -f "$PINCORE" ] || PINCORE=C:/Users/Administrator/Documents/claude/byo-pop/tools/pincore.py
	export PINCORED=1
	exec python -I "$PINCORE" -- bash "$0" "$@"
fi
OUT="$1"; WANT="${2:-3}"; MODE="${MODE:-2d}"; WAIT_S="${WAIT_S:-3600}"
P=games/astropop/tools/profile/perf.py
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"; [ -n "${APPEND:-}" ] || : > "$OUT/runs.txt"
start=$(date +%s)
until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do
	[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
	sleep 15
done
echo "start $(date '+%F %T'), mode $MODE, split ${SPLIT:-0}, waited $(( $(date +%s) - start )) s" >> "$OUT/runs.txt"
one() {  # <kind> <command...>: a run's last JSON line with its kind
	local kind="$1"; shift
	echo "$kind $("$@" 2>> "$OUT/errors.txt" | grep others_start | tail -1)" >> "$OUT/runs.txt"
}
export PERF_CLOCK=real PERF_SPLIT="${SPLIT:-0}"
orig() {
	if [ "$MODE" = 3d ]; then one orig python -I $P orig3d "$OUT/orig$1" 1
	else one orig python -I $P orig "$OUT/orig$1" 1; fi
}
port() {  # <window> <round>
	if [ "$MODE" = 3d ]; then PERF_WINDOW=$1 PERF_3D=1 PERF_DESKTOP=1 one "port-w$1" python -I $P port-minute "$OUT/port-w$1-$2"
	else PERF_WINDOW=$1 one "port-w$1" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-w$1-$2"; fi
}
for i in $(seq "${FIRST:-1}" "$WANT"); do
	case $((i % 3)) in
	1) orig $i; port 1 $i; port 2 $i ;;
	2) port 2 $i; orig $i; port 1 $i ;;
	0) port 1 $i; port 2 $i; orig $i ;;
	esac
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
