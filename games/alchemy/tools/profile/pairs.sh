#!/bin/bash
# pairs.sh <out dir> [rounds] [configs]: processor time and memory like for like (GAME_NOTES.md, "The software screen at 1x
# (2026-11-15)"): the original and the port in several configurations in turn, over the same scripted minute in real time
# (cycles.py), [rounds] rounds (default 5), each run started when the machine has room for one more game process
# (tools/gamecount.py --limit 6). Run the whole of it on a performance core of its own, in the background:
#   python -I tools/pincore.py -- bash games/alchemy/tools/profile/pairs.sh build/cyc/pairs 5
# The configurations (default: orig port1x gpu1x port2xgpu; port2xsoft is for a look only: software drawing is for 1x,
# the GPU takes every larger window, the user's decision of 2026-11-15, docs/RULES.md):
#   orig        the original, windowed at its own size, paced as a 60 Hz screen (CYC_VSYNC=0: drawing after every update)
#   port1x      the port at the game's own size (-windowscale=1), as it is now: the software screen
#   gpu1x       the same with SURFACE_DISPLAY=off: the GPU path, as the port was before the opt-in
#   port2xgpu   the default window (2x), as it is now: the software screen hands the window to the GPU there
#   port2xsoft  the default window with SOFTSCREEN_SCALED=1: the software screen stays in software at 2x (GDI's stretch)
# The input is the bot's strategic game (seed 11; <out dir>/bot.txt, which gen.sh writes): the stretch is the 6000 updates
# from update 600 (the board is up from update 460), the script runs to update 7000 and quits there, so no shutdown is
# counted. Each run's JSON line goes to <out dir>/runs.txt.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; N="${2:-5}"; CONFIGS="${3:-orig port1x gpu1x port2xgpu}"
C=games/alchemy/tools/profile/cycles.py
T=games/alchemy/tools/identical
PY="$(cygpath -w "$(command -v python)")"
R=build/native/rundesk.exe
# QUICK=1: a short stretch (COUNT 2500, the script quits at update 3200), every run once (no retries; CYC_MIN_MARKS 0: summarize.py
# takes from each original run the segments it was in step for), many rounds; LIMIT is the count of game processes
# gamecount.py waits to be under (a quieter machine for a timing run)
FIRST=600; COUNT=6000; SEED=11; QUIT=7000; LIMIT="${LIMIT:-6}"
if [ -n "$QUICK" ]; then COUNT=2500; QUIT=3200; export CYC_MIN_MARKS=0; fi
mkdir -p "$OUT"
[ -f "$OUT/bot.txt" ] || { echo "no $OUT/bot.txt (games/alchemy/tools/profile/gen.sh writes it)"; exit 2; }
awk -v q=$QUIT '$1 <= q' "$OUT/bot.txt" > "$OUT/port.txt"
echo "$QUIT quit" >> "$OUT/port.txt"
python -I $T/toorig.py "$OUT/port.txt" -1 > "$OUT/orig.txt"
room() { until python -I tools/gamecount.py --limit $LIMIT > /dev/null 2>&1; do sleep 20; done; }
echo "start $(date '+%F %T'), the stretch from update $FIRST, $COUNT updates, configs: $CONFIGS" >> "$OUT/runs.txt"
# the port's boards at the marks (its game is the same every run): the reference the original is held to (cycles.py, CYC_REF)
if [ ! -f "$OUT/ref.json" ]; then
	room
	CYC_WINDOWSCALE=1 $R "$PY" -I $C port "$OUT/ref" "$OUT/port.txt" $SEED $FIRST $COUNT 2>> "$OUT/errors.txt" | tail -1 |
		python -I -c "import json,sys; json.dump(json.loads(sys.stdin.read())['level_score'], open(sys.argv[1], 'w'))" "$OUT/ref.json"
fi
export CYC_REF="$OUT/ref.json"
for i in $(seq 1 "$N"); do
	for cfg in $CONFIGS; do
	# a run whose JSON says "invalid" (the original fell behind the bot's script before update 3600, or started late, or a
	# counter moved) is run again, the original up to eight times, a port up to three; each line stays in runs.txt, so the
	# discards are counted
	max=3; [ "$cfg" = orig ] && max=8; [ -n "$QUICK" ] && max=1
	for attempt in $(seq 1 $max); do
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
		case "$r" in *'"invalid"'*) ;; *) break ;; esac
	done
	done
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
