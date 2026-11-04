#!/bin/bash
# cont.sh <case dir> <game seed> <update N> <writer: port|orig> <bot seed> [bot arguments]: a saved game written by one program and
# continued by both. Both programs replay the case's input up to update N and quit there (snap.sh), the writer's saved game
# is put where both programs find it, then the port's bot starts a new run that takes CONTINUE SAVED GAME (-uicontinue=1) and
# plays on; the original plays the same input from the same saved game, and the original's values (values.txt) are compared
# with the port's state log. Prints one line per value, as case.sh does.
source "$(dirname "$0")/common.sh"
D="$1"; GS="$2"; N="$3"; W="$4"; BS="$5"; shift 5
S="$D/snap$N"
mkdir -p "$S"; bash "$TOOLS/snap.sh" "$D" "$GS" "$N" > "$S/snap.out" 2>&1
B="$D/cont$N$W"; rm -rf "$B"; mkdir -p "$B"
cp -r "$S/user" "$B/user"
mkdir -p "$B/save"
if [ "$W" = port ]; then
	cp "$S/user"/Players/save?.dat "$B/save/"
else
	rm -f "$B/user"/Players/save?.dat; cp "$OG_GAME"/Players/save?.dat "$B/save/"; cp "$B/save"/save?.dat "$B/user/Players/"
fi
ls -la "$B/user/Players" "$OG_GAME/Players" | grep save
KEEP_USER=1 "$TOOLS/port.sh" "$B" -seed=$GS -uibot=$BS -uistart=300000 -uicontinue=1 -uiscript="$B/script.txt" -statelog="$B/state.txt" "$@" > /dev/null
python "$TOOLS/toorig.py" "$B/script.txt" -1 > "$B/orig.txt"
mapfile -t LINES < <(grep -v '^#' "$TOOLS/${VALUES:-values.txt}")
NAMES=(); EXPRS=()
for l in "${LINES[@]}"; do NAMES+=("${l%% *}"); EXPRS+=("${l#* }"); done
LAST=$(tail -2 "$B/script.txt" | head -1 | cut -d' ' -f1)
TO=$((60 + LAST / 1500))
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	# (every run of the original leaves its own saved game: each starts from the writer's)
	rm -f "$OG_GAME"/Players/save?.dat; cp "$B/save"/save?.dat "$OG_GAME/Players/"
	KEEP_SAVE=1 "$TOOLS/orig.sh" "$B/orig.txt" $GS $TO "${EXPRS[@]:i:4}" > "$B/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${NAMES[*]:i:4}")"
	python "$TOOLS/cmp.py" "$B/state.txt" "$B/orig$((i / 4)).log" "$names" || st=1
done
exit $st
