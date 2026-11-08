#!/bin/bash
# tour.sh <tour> [port|orig|cmp]...: phase two's screen tours (games/dynomite/docs/IDENTICAL_PLAY.md). A tour is a line of
# tours.txt: its name, the game's seed, mkscript.py's rng seed and its parts (shot:<name> takes a screenshot). The tour's
# port script goes to build/ph2/<tour>/script.txt (and port/script.txt, which names port/ for its shots). "port" plays it in the port (-fast -drawevery, under rundesk) with its
# shots in build/ph2/<tour>/port/; "orig" plays it in the original inside Windows Sandbox (origrun --sandbox, on the
# virtual clock with the scripted cursor and the seed patches, the settings of settings.reg and mkhs.py's tables), with
# its shots in build/ph2/<tour>/orig/; "cmp" runs tools/shotdiff on the two and prints one line per pair that differs.
# With no step it does all three. SAVE=<file> starts both with that saved game in the first slot. Run
# identical/setup.sh once first (the patch list).
source "$(dirname "$0")/../identical/common.sh"
S=games/dynomite/tools/shots
N=$1; shift
STEPS="${*:-port orig cmp}"
L=$(grep "^$N " $S/tours.txt) || { echo "no tour $N"; exit 1; }
set -- $L
SEED=$2; R=$3; shift 3
D=build/ph2/$N
mkdir -p "$D/port"
python $TOOLS/mkscript.py "$D/port/script.txt" 25000 "$R" "$@"
cp "$D/port/script.txt" "$D/script.txt"
LAST=$(tail -1 "$D/script.txt" | cut -d' ' -f1)

for step in $STEPS; do
case $step in
port)
	rm -f "$D"/port/*.png
	SAVE="$SAVE" HS=1 $TOOLS/port.sh "$D" -seed=$SEED -script="$(win "$D/script.txt")" -statelog="$(win "$D/state.txt")" | tail -1
	ls "$D"/port/*.png | wc -l ;;
orig)
	RUN=dysb-$N; RD=build/origrun/$RUN
	if [ ! -f "$RD/game/DynomiteGame.exe" ]; then
		"$ORIGRUN" --game dynomite --run "$RUN" --copy-only > /dev/null || exit 1
		cp re/dynomite/bin/DynomiteGame.exe "$RD/game/DynomiteGame.exe"
	fi
	rm -rf "$RD/shots"
	rm -f "$RD"/game/*.HS "$RD/game/STOMPED.TMP" "$RD"/game/SLOT*.SAV
	python $TOOLS/mkhs.py "$RD/game"
	[ -z "$SAVE" ] || cp "$SAVE" "$RD/game/SLOT0.SAV"
	python $TOOLS/toorig.py "$D/script.txt" -1 shots > "$D/orig.txt"
	"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$PWD/$RD/game/DynomiteGame.exe")" --run "$RUN" --script "$(win "$PWD/$D/orig.txt")" \
		--sandbox --reg "$(win "$PWD/$TOOLS/settings.reg")" --clock virtual --cursor scripted --counter 0x530960 --update-ms 10 \
		--seed $SEED --patches tools/origrun/patches/dynomite.txt --timeout $((LAST / 1000 + 900)) ${ORIG_ARGS} > "$D/orig.log" 2>&1
	rm -rf "$D/orig"; mkdir -p "$D/orig"; cp "$RD"/shots/*.png "$D/orig/" 2>/dev/null
	grep -E "RESULT|SOUND LEAK" "$D/orig.log" | tail -3; ls "$D/orig" | wc -l ;;
cmp)
	python tools/shotdiff/shotdiff.py --game dynomite --orig "$D/orig" --port "$D/port" --run "$N" | tail -1
	python - "$N" <<'PY'
import json, sys
s = json.load(open('build/shotreview/dynomite/%s/summary.json' % sys.argv[1]))
for p in s['pairs']:
    if p['status'] != 'match':
        print(p['name'], p['status'], p.get('percent'), [(b['x'], b['y'], b['w'], b['h']) for b in p.get('boxes', [])][:6])
PY
	;;
esac
done
