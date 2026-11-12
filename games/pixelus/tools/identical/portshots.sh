#!/bin/bash
# portshots.sh <case dir> <original run id> <delta>: runs the port on the case's script with a screenshot at every update the
# original's shots landed at (build/origrun/<run>/shots/landed.tsv, made by shots.sh with that DELTA), for tools/shotdiff:
#   python tools/shotdiff/shotdiff.py --game pixelus --orig build/origrun/<run>/shots --port <case dir>/shots-port --run <name>
source "$(dirname "$0")/common.sh"
D="$1"; RUN="$2"; DELTA="$3"
S="$ROOT/build/origrun/$RUN/shots"
cp "$D/script.txt" "$D/script-shots.txt"
rm -rf "$D/shots-port"; mkdir -p "$D/shots-port"
LASTU=0
while IFS=$'\t' read -r name upd how; do
	echo "$((upd - DELTA)) screenshot $(cygpath -m "$D/shots-port/$name.png")" >> "$D/script-shots.txt"
	LASTU=$((upd - DELTA))
done < "$S/landed.tsv"
echo "$((LASTU + 5)) quit" >> "$D/script-shots.txt"
sort -n -s -k1,1 "$D/script-shots.txt" -o "$D/script-shots.txt"
rm -rf "$D/portshots/user"; mkdir -p "$D/portshots"
timeout 300 "$RUNDESK" "$(win build/native/pixelus.exe)" -userdir="$(win "$D/portshots/user")" -fast -seed="${SEED:-1}" -script="$(win "$D/script-shots.txt")" > "$D/portshots/out.txt" 2>&1
echo "port exit $?"; ls "$D/shots-port" | wc -l
