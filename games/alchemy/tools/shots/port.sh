#!/bin/bash
# port.sh <tour stem>: the port on a tour (fast clock), shots to build/ph2/<tour>/port/.
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift; D=build/ph2/$N
rm -rf "$D/port"; mkdir -p "$D/port"
python games/alchemy/tools/shots/make.py games/alchemy/tools/shots/$N.txt "$D" port > "$D/port.txt"
timeout 300 "$TOOLS/port.sh" "$D" -seed=1 -script="$(win "$D/port.txt")" "$@" | tail -1
ls "$D/port" | wc -l
