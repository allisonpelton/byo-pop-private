#!/bin/bash
# case.sh <name> <bot seed> <game seed> [bot arguments]: one comparison. The port's bot plays through the mouse (-uibot=<bot
# seed>; -uimodes, -uidiffs, -uigames, -uicap as extra arguments, testhooks/UiBot.h), its input is written as a script, the
# original plays the same input on the same game seed, and the original's values (values.txt) are compared with the port's
# state log, change by change with the update it came at. origrun watches four values at a time, so the original runs once per
# four values. Prints one line per value (and what differs).
source "$(dirname "$0")/common.sh"
N="$1"; BS="$2"; GS="$3"; shift 3
D="$OUT/$N"
START="${START:-300000}"
"$TOOLS/port.sh" "$D" -seed=$GS -uibot=$BS -uistart=$START -uiscript="$D/script.txt" -statelog="$D/state.txt" "$@" > /dev/null
python "$TOOLS/toorig.py" "$D/script.txt" -1 > "$D/orig.txt"
mapfile -t LINES < <(grep -v '^#' "$TOOLS/${VALUES:-values.txt}")
NAMES=(); EXPRS=()
for l in "${LINES[@]}"; do NAMES+=("${l%% *}"); EXPRS+=("${l#* }"); done
LAST=$(tail -2 "$D/script.txt" | head -1 | cut -d' ' -f1)
TO=$((60 + LAST / 1500))
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	"$TOOLS/orig.sh" "$D/orig.txt" $GS $TO "${EXPRS[@]:i:4}" > "$D/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${NAMES[*]:i:4}")"
	python "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "$names" || st=1
done
exit $st
