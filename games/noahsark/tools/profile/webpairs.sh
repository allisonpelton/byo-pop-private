#!/bin/bash
# webpairs.sh <before dir> <pairs>: the performance pass's web numbers (GAME_NOTES.md, section 10). From the worktree root. Plays a whole
# autoplay action game with -fast under Node (wasmrun.js game), the web build in <before dir> (noahsark.js and .wasm of the earlier sources)
# and build/web's, alternately, <pairs> times each: the wall time of each run, and whether the two builds' autoplay logs are identical.
# Each run waits for room (three game processes on the machine) first. Writes build/perf2/res/webpairs.txt.
cd "$(dirname "$0")/../../../.."
BEFORE="$(cd "$1" && pwd)"; PAIRS="$2"
NODE="${NODE:-/e/retools/emsdk/node/24.19.0_64bit/node.exe}"
WASMRUN="$(pwd)/games/bejeweled/tools/verify/wasmrun.js"
GAMEDIR="$(pwd -W)/assets/noahsark"
AFTER="$(pwd)/build/web"
OUT=build/perf2/res/webpairs.txt
mkdir -p build/perf2/res
: > "$OUT"
run() {  # <label> <js> <n>
	local d="build/perf2/webrun_$1_$3"
	rm -rf "$d"; mkdir -p "$d"
	until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
	local t0=$(date +%s.%N)
	(cd "$d" && timeout 900 "$NODE" "$WASMRUN" game "$2" -gamedir="$GAMEDIR" -userdir=user -fast -seed=1 -autoplay=1 -autoplaymode=action \
		-autoplaycap=20000 -autoplaylog=log.txt > out.txt 2>&1)
	local rc=$?
	local t1=$(date +%s.%N)
	echo "$1 $3 exit $rc wall $(python -c "print(round($t1 - $t0, 2))") s: $(grep -E '^(level [0-9]+ done|game [0-9]+ over)' "$d/log.txt" 2>/dev/null | tail -n 1)" >> "$OUT"
}
for n in $(seq 1 "$PAIRS"); do
	run before "$BEFORE/noahsark.js" "$n"
	run after "$AFTER/noahsark.js" "$n"
done
if cmp -s build/perf2/webrun_before_1/log.txt build/perf2/webrun_after_1/log.txt; then echo "autoplay logs identical" >> "$OUT"; else echo "AUTOPLAY LOGS DIFFER" >> "$OUT"; fi
echo done >> "$OUT"
