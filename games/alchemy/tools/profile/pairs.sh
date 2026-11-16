#!/bin/bash
# pairs.sh <out dir> [rounds] [configs]: processor time and memory like for like (GAME_NOTES.md, "The software screen at 1x
# (2026-11-15)"): the original and the port in several configurations in turn, over the same scripted minute in real time
# (cycles.py), [rounds] rounds (default 5), each run started when the machine has room for one more game process
# (tools/gamecount.py --limit 3). Run the whole of it on a performance core of its own, in the background:
#   python -I tools/pincore.py -- bash games/alchemy/tools/profile/pairs.sh build/cyc/pairs 5
# The configurations (default: orig port1x gpu1x; add port2xgpu port2xsoft for the default window):
#   orig        the original, windowed at its own size, paced as a 60 Hz screen (CYC_VSYNC=0: drawing after every update)
#   port1x      the port at the game's own size (-windowscale=1), as it is now: the software screen
#   gpu1x       the same with SURFACE_DISPLAY=off: the GPU path, as the port was before the opt-in
#   port2xgpu   the default window (2x), as it is now: the software screen hands the window to the GPU there
#   port2xsoft  the default window with SOFTSCREEN_SCALED=1: the software screen stays in software at 2x (GDI's stretch)
# The input is the bot's strategic game (seed 11; <out dir>/bot.txt, which gen.sh writes): the stretch is the 6000 updates
# from update 600 (the board is up from update 460), the script runs to update 7000 and quits there, so no shutdown is
# counted. Each run's JSON line goes to <out dir>/runs.txt.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; N="${2:-5}"; CONFIGS="${3:-orig port1x gpu1x}"
C=games/alchemy/tools/profile/cycles.py
T=games/alchemy/tools/identical
PY="$(cygpath -w "$(command -v python)")"
R=build/native/rundesk.exe
FIRST=600; COUNT=6000; SEED=11
mkdir -p "$OUT"
[ -f "$OUT/bot.txt" ] || { echo "no $OUT/bot.txt (games/alchemy/tools/profile/gen.sh writes it)"; exit 2; }
awk '$1 <= 7000' "$OUT/bot.txt" > "$OUT/port.txt"
echo "7000 quit" >> "$OUT/port.txt"
python -I $T/toorig.py "$OUT/port.txt" -1 > "$OUT/orig.txt"
room() { until python -I tools/gamecount.py --limit 3 > /dev/null 2>&1; do sleep 20; done; }
echo "start $(date '+%F %T'), the stretch from update $FIRST, $COUNT updates, configs: $CONFIGS" >> "$OUT/runs.txt"
for i in $(seq 1 "$N"); do
	for cfg in $CONFIGS; do
		room
		case $cfg in
		orig)       r="$(python -I $C orig "$OUT/o$i" "$OUT/orig.txt" $SEED $FIRST $COUNT 2>> "$OUT/errors.txt" | tail -1)" ;;
		port1x)     r="$(CYC_WINDOWSCALE=1 $R "$PY" -I $C port "$OUT/q$i" "$OUT/port.txt" $SEED $FIRST $COUNT 2>> "$OUT/errors.txt" | tail -1)" ;;
		gpu1x)      r="$(CYC_WINDOWSCALE=1 SURFACE_DISPLAY=off $R "$PY" -I $C port "$OUT/g$i" "$OUT/port.txt" $SEED $FIRST $COUNT 2>> "$OUT/errors.txt" | tail -1)" ;;
		port2xgpu)  r="$(unset CYC_WINDOWSCALE; $R "$PY" -I $C port "$OUT/p$i" "$OUT/port.txt" $SEED $FIRST $COUNT 2>> "$OUT/errors.txt" | tail -1)" ;;
		port2xsoft) r="$(SOFTSCREEN_SCALED=1 $R "$PY" -I $C port "$OUT/s$i" "$OUT/port.txt" $SEED $FIRST $COUNT 2>> "$OUT/errors.txt" | tail -1)" ;;
		*) r="unknown configuration $cfg" ;;
		esac
		echo "$cfg $r" >> "$OUT/runs.txt"
	done
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
