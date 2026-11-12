#!/bin/bash
# vsync.sh <out dir> [rounds]: the 3D stretch with the display's vertical blank holding the presents (the framework's default: vsync
# asked of the renderer) against the same with vsync off and the presents held by the frame loop's own pace (DDInterface::FrameHeldByPace,
# the display's refresh interval; `SDL_RENDER_VSYNC=0` takes the renderer's vsync away, so the pace engages), one game process at a time, in
# real time, on the visible desktop (22:00 to 08:00 only), at the original's own window size (PERF_WINDOW=1), the order rotating every round,
# the whole loop on one performance core (tools/pincore.py), the original (perf.py orig3d) in every round as the reference. Every port run
# writes its present log (-presentlog: the start and length of each SDL_RenderPresent call; header: renderer, vsync asked and granted, pace);
# `present_intervals.py <dir>` reads the intervals, `windows_report.py` the processor time and, with SPLIT=1, the split.
#   SPLIT=1  the cycle split as well (a counted run is for the proportions; the totals come from SPLIT=0 runs)
#   FIRST=4 APPEND=1  more rounds into the same folder
cd "$(dirname "$0")/../../../.." || exit 2
if [ -z "${PINCORED:-}" ]; then
	PINCORE=tools/pincore.py
	[ -f "$PINCORE" ] || PINCORE=C:/Users/Administrator/Documents/claude/byo-pop/tools/pincore.py
	export PINCORED=1
	exec python -I "$PINCORE" -- bash "$0" "$@"
fi
OUT="$1"; WANT="${2:-3}"; WAIT_S="${WAIT_S:-3600}"
P=games/astropop/tools/profile/perf.py
mkdir -p "$OUT"; [ -n "${APPEND:-}" ] || : > "$OUT/runs.txt"
start=$(date +%s)
until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do
	[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
	sleep 15
done
echo "start $(date '+%F %T'), split ${SPLIT:-0}, waited $(( $(date +%s) - start )) s" >> "$OUT/runs.txt"
one() {  # <kind> <command...>: a run's last JSON line with its kind
	local kind="$1"; shift
	echo "$kind $("$@" 2>> "$OUT/errors.txt" | grep others_start | tail -1)" >> "$OUT/runs.txt"
}
export PERF_CLOCK=real PERF_SPLIT="${SPLIT:-0}" PERF_WINDOW=1 PERF_3D=1 PERF_DESKTOP=1 PERF_PRESENTLOG=1
orig() { PERF_PRESENTLOG=0 one orig python -I $P orig3d "$OUT/orig$1" 1; }
vsync() { one port-vsync python -I $P port-minute "$OUT/port-vsync-$1"; }
pace() { PERF_ENV="SDL_RENDER_VSYNC=0" one port-pace python -I $P port-minute "$OUT/port-pace-$1"; }
for i in $(seq "${FIRST:-1}" "$WANT"); do
	case $((i % 3)) in
	1) orig $i; vsync $i; pace $i ;;
	2) pace $i; orig $i; vsync $i ;;
	0) vsync $i; pace $i; orig $i ;;
	esac
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
