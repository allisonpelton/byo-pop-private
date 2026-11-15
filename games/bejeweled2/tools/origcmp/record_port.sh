#!/bin/bash
# Records a mode's inputs on the port (record.py) once there is room for a game process.
#   [IDLE_END=<updates>] [PROFILE=<make_profile.py's or make_save.py's folder>] [HYPER=1] [DRAWEVERY=1] record_port.sh <mode> <moves> <out dir> [seed]
mode=$1; moves=$2; out=$3; seed=${4:-1}
cd "$(dirname "$0")/../../../.." || exit 2
for i in $(seq 1 120); do
  python tools/gamecount.py --limit 6 > /dev/null && break
  sleep 15
done
rm -rf "$out.user"
if [ -n "$PROFILE" ]; then mkdir -p "$out.user" && cp -r "$PROFILE/users" "$out.user/users"; fi  # a player with the secret modes unlocked (make_profile.py)
exec python games/bejeweled2/tools/origcmp/record.py --exe build/native/bejeweled2.exe --userdir "$out.user" \
  --out "$out" --mode "$mode" --moves "$moves" --seed "$seed" ${PLAY_AT:+--play-at $PLAY_AT} ${LOWRES:+--lowres} ${NOSPARKLE:+--nosparkle} ${IDLE_END:+--idle-end $IDLE_END} ${HYPER:+--hyper} ${DRAWEVERY:+--drawevery}
