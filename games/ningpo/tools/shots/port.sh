#!/bin/bash
# port.sh <tour dir>: the native port on the screen tour (tour.py's port.txt: real SDL input at fixed updates, -fast,
# seed 7, fresh settings), under rundesk. Its shots go to <tour dir>/port/, its log (the SOUND lines that
# tools/sound/cmp.py reads) to <tour dir>/port.log.
source "$(dirname "$0")/../identical/common.sh"
D="$1"; U="$D/user"
rm -rf "$U"; mkdir -p "$U" "$D/port"
"$RUNDESK" "$(win "$ROOT/build/native/ningpo.exe")" -gamedir=assets/ningpo -userdir="$U" -seed=7 -fast \
	-realinput="$(win "$PWD/$D/port.txt")" > "$D/port.log" 2>&1
echo "port rc=$? $(grep -c ' SOUND ' "$D/port.log") sounds, $(ls "$D/port" | wc -l) shots"
