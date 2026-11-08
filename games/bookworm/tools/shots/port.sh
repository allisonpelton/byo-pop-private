#!/bin/bash
# port.sh <tour> [port args]: the port on a tour (-fast), shots to build/ph2/<tour>/port/. GS=<seed> (default 1).
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift; D=build/ph2/$N
rm -rf "$D/port" "$D/user"; mkdir -p "$D/port" "$D/user"
python games/bookworm/tools/shots/make.py games/bookworm/tools/shots/$N.txt "$D" port > "$D/port.txt"
KEEP_USER=1 PORT_TIMEOUT=300 "$TOOLS/port.sh" "$D" -seed=${GS:-1} -script="$(win "$D/port.txt")" "$@" | tail -1
ls "$D/port" | wc -l
