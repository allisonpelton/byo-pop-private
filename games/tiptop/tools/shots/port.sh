#!/bin/bash
# port.sh <tour> [port arguments]: the port on a tour (-fast), shots to build/ph2/<tour>/port/. SAVE=<file> starts it
# with that saved game; GS=<seed> as orig.sh.
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift; D=build/ph2/$N
rm -rf "$D/port" "$D/user"; mkdir -p "$D/port" "$D/user"
[ -z "$SAVE" ] || cp "$SAVE" "$D/user/savegame.dat"
python games/tiptop/tools/shots/make.py games/tiptop/tools/shots/$N.txt "$D" port > "$D/port.txt"
KEEP_USER=1 timeout 300 "$TOOLS/port.sh" "$D" -seed=${GS:-1} -script="$(win "$D/port.txt")" -statelog="$(win "$D/state.txt")" "$@"
echo "$(ls "$D/port" | wc -l) shots"
