#!/bin/bash
# sound_port.sh <tour>: the port on a tour (-fast) with BYOPOP_SOUNDLOG, to build/ph2/<tour>/port_sound.log. GS=<seed>.
source "$(dirname "$0")/../identical/common.sh"
N=$1; D=build/ph2/$N; S=$D/sound
rm -rf "$S"; mkdir -p "$S/user"
python games/bookworm/tools/shots/make.py games/bookworm/tools/shots/$N.txt "$S" port | grep -v screenshot > "$S/port.txt"
BYOPOP_SOUNDLOG="$(win "$PWD/$D/port_sound.log")" KEEP_USER=1 PORT_TIMEOUT=300 "$TOOLS/port.sh" "$S" -seed=${GS:-1} \
	-script="$(win "$S/port.txt")" | tail -1
wc -l "$D/port_sound.log"
