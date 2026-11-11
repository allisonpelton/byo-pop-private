#!/bin/bash
# The web build's pictures against the native port's: the tour's -realinput script (tour.sh's port.txt) run by build/web/astropop.js
# under Node (games/bejeweled/tools/verify/wasmrun.js game: no window, no sound, no URL) and the native port's shots of the same tour
# (tour.sh's), compared by shotdiff.
#   games/astropop/tools/tour/webshots.sh <tour name> [seed]
# Needs build/logs/ap/tour/<name>/ with port.txt and port/ (a tour.sh run, SKIP_ORIG=1 will do) and a web build (tools/build.sh web).
# Writes build/logs/ap/tour/<name>/web/*.png and the shotdiff run "web-<name>" (build/shotreview/astropop/web-<name>).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
N="$1"; SEED="${2:-1}"
D="build/logs/ap/tour/$N"; W="$D/webrun"
[ -s "$D/port.txt" ] || { echo "no port script for $N"; exit 1; }
rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$D/web" "$W/ud"
printf '[SOFTWARE\PopCap\AstroPop]\nIs3D=dword:0\nScreenMode=dword:0\n' > "$W/ud/registry.ini"
[ -n "${PORT_USER:-}" ] && cp -r "$PORT_USER"/. "$W/ud"/
# the script with the shots inside the run's own folder (Node writes back only what lies in the working directory)
sed -E "s#^([0-9]+) screenshot .*/([^/]+\.png)\$#\1 screenshot shots/\2#" "$D/port.txt" > "$W/script.txt"
NODE="$(ls "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe 2>/dev/null | head -n 1)"
[ -n "$NODE" ] || { echo "no node.exe under EMSDK_ROOT"; exit 1; }
( cd "$W" && "$NODE" "$R/games/bejeweled/tools/verify/wasmrun.js" game --timeout 1500 "$R/build/web/astropop.js" -fast -drawevery -seed="$SEED" \
	-gamedir="$R/assets/astropop" -userdir=ud -realinput=script.txt > run.out 2>&1; echo "web exit=$? shots=$(ls shots | wc -l)" )
mv "$W/shots/"*.png "$D/web/" 2>/dev/null
echo '{"client": [640, 480]}' > "$D/webshots.json"
python tools/shotdiff/shotdiff.py --game astropop --orig "$D/port" --port "$D/web" --run "web-$N" --config "$D/webshots.json" --tolerance "${TOL:-0}" 2>&1 | tail -2
