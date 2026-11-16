#!/bin/bash
# softpairs.sh <out dir> [rounds]: the performance target's measurement (docs/COORDINATION.md, "The target") for the software
# screen (GAME_NOTES.md, "The software screen"): each round runs the original paced as a 60 Hz screen paces it (PERF_PACED=16,
# counting its updates by the app's +0x3ec, as paced.sh) and then each configuration of the port, over the same stretch of play
# in real time (tools/identical/s2.txt cut at update 4,400; perf.py). The configurations (CONFIGS, in the order each round
# starts from the next one on, so that none always runs right after the original):
#   soft1x      the software screen in the game's own 640x480 window (-windowscale=1)
#   gpu1x       the GPU path in the same window, the build kept as build/native/atomica-before.exe
#   soft2x      the default 2x window (the software screen hands itself to the GPU above 1x)
#   scaled2x    the default 2x window with SOFTSCREEN_SCALED=1 (the software screen stays, GDI stretches its frames)
#   gpu2x       the GPU path in the default 2x window (atomica-before.exe)
# PORT_EXE names the build of the port in build/native (atomica.exe). A run of the original that missed the title's click
# (under 30 frames a second in the stretch: it sat on the title) is written as orig-missed and run again, up to three times.
# Run the whole loop on one core of its own: python -I tools/pincore.py -- bash games/atomica/tools/profile/softpairs.sh <out> 5
# It waits for room (fewer than 3 game processes on the machine) before each run, up to WAIT_S seconds (3600) each, and
# writes one line a run to <out dir>/runs.txt (the kind, then perf.py's JSON). It ends by itself: run it in the background.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; WANT="${2:-5}"; WAIT_S="${WAIT_S:-3600}"
CONFIGS=(${CONFIGS:-soft1x gpu1x scaled2x})
P=games/atomica/tools/profile/perf.py; SC=games/atomica/tools/identical/s2.txt
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"; : > "$OUT/runs.txt"
room() {
	local start; start=$(date +%s)
	until python -I tools/gamecount.py --limit 3 > /dev/null 2>&1; do
		[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
		sleep 10
	done
}
echo "start $(date '+%F %T')" >> "$OUT/runs.txt"
one() {  # <kind> <command...>: a run's last JSON line with its kind
	local kind="$1"; shift
	echo "$kind $("$@" 2>> "$OUT/errors.txt" | grep others_start | tail -1)" >> "$OUT/runs.txt"
}
orig() {  # <round>: the paced original, again when it missed the click
	local t line
	for t in 1 2 3; do
		room
		line="$(PERF_CLOCK=real PERF_PACED=16 PERF_COUNTER=0x4d004c PERF_COUNTER_SHIFT=1 python -I $P orig "$OUT/orig-paced$1-$t" $SC 1 2>> "$OUT/errors.txt" | grep others_start | tail -1)"
		if python -I -c 'import json,sys; sys.exit(0 if (json.loads(sys.argv[1]).get("frames_per_s_in_stretch") or 0) >= 30 else 1)' "$line" 2> /dev/null; then
			echo "orig-paced $line" >> "$OUT/runs.txt"; return
		fi
		echo "orig-missed $line" >> "$OUT/runs.txt"
	done
}
port() {  # <round> <config>
	local exe="${PORT_EXE:-atomica.exe}" args="" envv="X=1"
	case "$2" in
		soft1x) args="-windowscale=1";;
		gpu1x) exe=atomica-before.exe; args="-windowscale=1";;
		soft2x) ;;
		scaled2x) envv="SOFTSCREEN_SCALED=1";;
		gpu2x) exe=atomica-before.exe;;
	esac
	room
	env "$envv" PERF_CLOCK=real PERF_EXE="$exe" PERF_ARGS="$args" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/$2-$1" $SC 2>> "$OUT/errors.txt" | grep others_start | tail -1 | sed "s/^/$2 /" >> "$OUT/runs.txt"
}
n=${#CONFIGS[@]}
for i in $(seq 1 "$WANT"); do
	orig "$i"
	for k in $(seq 0 $((n - 1))); do
		port "$i" "${CONFIGS[$(( (k + i - 1) % n ))]}"
	done
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
