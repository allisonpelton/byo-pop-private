#!/bin/bash
# lag.sh <tour name> <shot names, comma separated> [window]: which update does the original's shot really show? The port runs the
# tour with a shot at every update of each window before the label (lagscan.py), and each of the original's shots is compared
# with those frames; the best update and its differing pixels print, the best frame is copied to build/ph2/<tour>lag/best/<name>.png.
# The original's shots (orig.sh) must be in build/ph2/<tour>/orig. RM_REG as in port.sh.
source "$(dirname "$0")/../identical/common.sh"
N=$1; NAMES=$2; W=${3:-100}; D=build/ph2/${N}lag
mkdir -p "$D"
python games/rocketmania/tools/shots/lagscan.py games/rocketmania/tools/shots/$N.txt build/ph2/$N/orig "$D" --window $W --names "$NAMES" --prepare
until python tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done
rm -rf "$D/port" "$D/best"; mkdir -p "$D/port"
timeout 1800 "$TOOLS/port.sh" "$D" -seed=${GS:-1} -script="$(win "$D/lag.txt")" | tail -1
python games/rocketmania/tools/shots/lagscan.py games/rocketmania/tools/shots/$N.txt build/ph2/$N/orig "$D" --window $W --names "$NAMES" --compare --pick "$D/best"
