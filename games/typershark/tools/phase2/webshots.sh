#!/usr/bin/env bash
# The web build's pictures against the native port's: the same tour script run by build/web/typershark.js under Node
# (games/bejeweled/tools/verify/wasmrun.js game: no window, no sound, no URL) and by build/native/typershark.exe, the
# shots compared by shotdiff (the native port's are the ones compared with the original).
#   games/typershark/tools/phase2/webshots.sh <tour name> [seed]
# Reads build/ts-p2/<name>/port.txt, writes build/ts-p2/<name>/web/*.png and build/ts-p2/<name>/port/*.png, and prunes both
# once they are compared (KEEP=1 keeps them).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
N="$1"; SEED="${2:-1}"
D="build/ts-p2/$N"; W="$D/webrun"
[ -s "$D/port.txt" ] || { echo "no port script for $N"; exit 1; }
bash games/typershark/tools/phase2/waitroom.sh || exit 3
rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$D/web"
cp -r build/ts-p1/fixtures/port-win "$W/ud"
# the script with the shots inside the run's own folder (Node writes back only what lies in the working directory)
sed -E "s#^([0-9]+) shot .*/([^/]+\.png)\$#\1 shot shots/\2#" "$D/port.txt" > "$W/script.txt"
NODE="$(ls "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe 2>/dev/null | head -n 1)"
[ -n "$NODE" ] || { echo "no node.exe under EMSDK_ROOT"; exit 1; }
( cd "$W" && "$NODE" "$R/games/bejeweled/tools/verify/wasmrun.js" game "$R/build/web/typershark.js" -fast -drawevery -seed="$SEED" \
	-gamedir="$R/assets/typershark" -userdir=ud -script=script.txt --timeout 1500 > run.out 2>&1; echo "web exit=$? shots=$(ls shots | wc -l)" )
mv "$W/shots/"*.png "$D/web/" 2>/dev/null
bash games/typershark/tools/phase2/runport.sh "$N" "$SEED" -drawevery | tail -1
# both sides are the port's own 640x480 pictures: no crop, and no tolerance (the same code must draw the same bytes)
echo '{"client": [640, 480]}' > build/ts-p2/webshots.json
python tools/shotdiff/shotdiff.py --game typershark --orig "$D/port" --port "$D/web" --run "web-$N" --config build/ts-p2/webshots.json --tolerance 0 2>&1 | tail -2
python games/typershark/tools/phase2/sumshot.py "web-$N" | head -20
if [ -z "${KEEP:-}" ]; then rm -rf "$D/web" "$D/port" "$D/ud" "$W"; fi
