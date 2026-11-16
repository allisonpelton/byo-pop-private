#!/bin/bash
# splitrun.sh <out dir> [runs] [exe] [extra port args...]: perfscript.py's minute of play on the port with -cyclesplit, so that the
# window's cycles come apart into the parts (update, draw, present, driver, audio, loop, other: tools/cyclesplit.py). The window is
# updates 2000 to 8000 (20 s to 80 s at 100 updates a second, as softpairs.sh's). One game at a time, on a core of its own, after a
# free slot (fewer than 6 game processes). Windowed at 1x unless the extra arguments say otherwise. From the worktree root.
set -u
cd "$(dirname "$0")/../../../.."
OUT="$1"; RUNS="${2:-3}"; EXE="${3:-build/native/bejeweled.exe}"; if [ $# -ge 3 ]; then shift 3; else shift $#; fi; ARGS=("$@"); [ ${#ARGS[@]} = 0 ] && ARGS=(-windowscale=1)
P=games/bejeweled/tools/profile
mkdir -p "$OUT"
win() { cygpath -w "$1"; }
python -I $P/perfscript.py port "$OUT/plain.txt" > /dev/null
{ cat "$OUT/plain.txt"; echo "2000 cyclesplit a"; echo "8000 cyclesplit b"; } | sort -n -s -k1,1 > "$OUT/port.txt"
for n in $(seq 1 "$RUNS"); do
	until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
	U="$OUT/u$n"; rm -rf "$U"; mkdir -p "$U"
	printf '[SOFTWARE\Steam\Bejeweled]\nScreenMode=dword:0\n' > "$U/registry.ini"
	python -I tools/pincore.py -- "$(win "$PWD/build/native/rundesk.exe")" "$(win "$PWD/$EXE")" -gamedir="$(win "$PWD/assets/bejeweled")" \
		-userdir="$(win "$PWD/$U")" -script="$(win "$PWD/$OUT/port.txt")" -cyclesplit="$(win "$PWD/$OUT/split$n.json")" "${ARGS[@]}" > "$OUT/split$n.log" 2>&1
	echo "run $n: exit $?"
	python -I tools/cyclesplit.py "$OUT/split$n.json" --from a --to b --split 2>&1 | tail -14
done
