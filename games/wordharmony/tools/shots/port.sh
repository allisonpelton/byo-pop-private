#!/bin/bash
# port.sh <tour> [port arguments]: the port on a tour (-fast), shots to build/ph2/<tour>/port/. PORTREG=<registry.ini>
# starts it with those values (a saved game); GS=<seed> as orig.sh. Its sounds go to build/ph2/<tour>/sound.txt
# (identical/sndcmp.py lines them up with the original's, build/origrun/whsb-<tour>/logs/sound.log).
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift; D=build/ph2/$N
rm -rf "$D/port" "$D/user"; mkdir -p "$D/port" "$D/user"
[ -z "$PORTREG" ] || cp "$PORTREG" "$D/user/registry.ini"
python games/wordharmony/tools/shots/make.py games/wordharmony/tools/shots/$N.txt "$(cygpath -m "$PWD/$D")" port > "$D/port.txt"
KEEP_USER=1 timeout 300 bash "$TOOLS/port.sh" "$D" -seed=${GS:-1} -script="$(win "$D/port.txt")" -statelog="$(win "$D/state.txt")" -soundlog="$(win "$D/sound.txt")" "$@" > /dev/null
echo "$(ls "$D/port" | wc -l) shots"
