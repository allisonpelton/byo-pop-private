#!/bin/bash
# port.sh <tour name>: the port on a tour (fast clock), shots to build/ph2/<tour>/port/. GS=<game seed>.
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift; D=build/ph2/$N
rm -rf "$D/port" "$D/user"; mkdir -p "$D/port" "$D/user"
python games/rocketmania/tools/shots/make.py games/rocketmania/tools/shots/$N.txt "$D" port > "$D/port.txt"
KEEP_USER=1 timeout 600 "$TOOLS/port.sh" "$D" -seed=${GS:-1} -script="$(win "$D/port.txt")" "$@" | tail -1
ls "$D/port" | wc -l
