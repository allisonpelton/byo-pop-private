#!/bin/bash
# pairs.sh <out dir> [rounds]: processor time like for like (GAME_NOTES.md section 6, "Processor time like for like"):
# the original, the port in a window of the game's own size (640x480, -windowscale=1, as the original's window) and the
# port in its default window (2x where the desktop has room, what a player gets) in turn, over the same scripted minute
# in real time (cycles.py), [rounds] rounds (default 5), each run started when the machine has room for one more game
# process (tools/gamecount.py --limit 6). Run the whole of
# it on a performance core of its own, in the background:
#   python -I tools/pincore.py -- bash games/dynomite/tools/profile/pairs.sh build/cyc 5
# with CYC_VSYNC=1 before it, the original draws at most once a refresh, as in full screen on a 60 Hz screen (cycles.py).
# The input is perf_all.sh's minute: the title's link at update 1500, Time Trial on Normal from the main menu, then 150
# shots at random points of the board, one every 40 updates. The stretch is the 6000 updates from the first shot, and
# 300 updates follow it before the quit, so it never holds the shutdown. Each run's JSON line goes to <out dir>/runs.txt.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; N="${2:-5}"
C=games/dynomite/tools/profile/cycles.py
T=games/dynomite/tools/identical
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"
python -I $T/mkscript.py "$OUT/port.txt.0" 1500 3 title menu:timetrial:normal wait:300 shots:150:40 wait:300 quit > /dev/null
# origrun quits the original right after the script's last input, so a move to where the cursor already is, 50 updates
# before the quit, keeps it running past the stretch's end
awk '$2 == "move" {x = $3; y = $4} $2 == "quit" {print $1 - 50, "move", x, y} {print}' "$OUT/port.txt.0" > "$OUT/port.txt"
rm -f "$OUT/port.txt.0"
python -I $T/toorig.py "$OUT/port.txt" -1 > "$OUT/orig.txt"
# the first shot: the first move after the title's click and the menu's five (Let's Play the last)
START=$(awk '$2 == "click" {n++} n == 6 && $2 == "move" {print $1; exit}' "$OUT/port.txt")
SEED=10
room() { until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done; }
echo "start $(date '+%F %T'), the stretch from update $START" >> "$OUT/runs.txt"
for i in $(seq 1 "$N"); do
	room
	echo "orig $(python -I $C orig "$OUT/o$i" "$OUT/orig.txt" $SEED "$START" 6000 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"
	room
	echo "port1x $(CYC_WINDOWSCALE=1 build/native/rundesk.exe "$PY" -I $C port "$OUT/q$i" "$OUT/port.txt" $SEED "$START" 6000 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"
	room
	echo "port $(build/native/rundesk.exe "$PY" -I $C port "$OUT/p$i" "$OUT/port.txt" $SEED "$START" 6000 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
