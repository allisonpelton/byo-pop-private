#!/bin/bash
# Records a mode's inputs on the port (record.py) once there is room for a game process.
#   record_port.sh <mode> <moves> <out dir> [seed]
mode=$1; moves=$2; out=$3; seed=${4:-1}
cd "$(dirname "$0")/../../../.." || exit 2
for i in $(seq 1 120); do
  python tools/gamecount.py --limit 3 > /dev/null && break
  sleep 15
done
rm -rf "$out.user"
exec python games/bejeweled2/tools/origcmp/record.py --exe build/native/bejeweled2.exe --userdir "$out.user" \
  --out "$out" --mode "$mode" --moves "$moves" --seed "$seed" ${PLAY_AT:+--play-at $PLAY_AT} ${LOWRES:+--lowres}
