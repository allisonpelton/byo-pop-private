#!/usr/bin/env bash
# The port on the PopCap Game Collection's files, against the same port on the Steam copy's (GAME_NOTES.md, "The PopCap Game
# Collection version"). Needs build/native/astropop.exe and rundesk, assets/astropop staged from the Steam copy, and the web build
# (tools/check.sh build-web:astropop) for the last step.
#   games/astropop/tools/collection/run.sh <the Collection's AstroPopDeluxeComponent folder>
# Stages the Collection's files into build/ap-coll/assets/astropop with a copy of the exe in build/ap-coll/bin (the exe finds
# assets/astropop one level up), then: tools/check.sh astropop on that exe (the three frozen games' states, each game move for move
# as the frozen baseline plays it on the Steam files), the tours of games/astropop/tools/tour once on each set of files
# (tour.sh, port only: the same inputs at the same updates, a fresh profile folder each) and compared by shotdiff (every pair must
# match), the title screen's loading picture (the one picture that differs) and the web build under Node on the Collection's files
# (the first tour against the native shots). One game process at a time, invisible and silent under rundesk, with -fast.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
SRC="$1"
T=games/astropop/tools/tour
TOURS="${TOURS:-menus options halls users classic1 ingame1 survend quit1 ships1 emerg1}"
mkdir -p build/ap-coll/bin
python -I games/astropop/tools/stage_astropop_assets.py "$SRC" --out build/ap-coll/assets/astropop || exit 1
cp build/native/astropop.exe build/native/*.dll build/ap-coll/bin/
echo "--- the frozen games on the Collection's files"
AP_EXE="$R/build/ap-coll/bin/astropop.exe" tools/check.sh astropop | tail -1
for t in $TOURS; do
	SKIP_ORIG=1 bash $T/tour.sh "steam-$t" "$T/$t.txt" | tail -1
	SKIP_ORIG=1 PORT_EXE=build/ap-coll/bin/astropop.exe bash $T/tour.sh "coll-$t" "$T/$t.txt" | tail -1
	# both are the port's own 640x480 pictures: a game name with no shotdiff configuration, so no crop
	python tools/shotdiff/shotdiff.py --game astropop-coll --orig "build/logs/ap/tour/steam-$t/port" --port "build/logs/ap/tour/coll-$t/port" --run "coll-$t" | tail -1
done
echo "--- the web build under Node on the Collection's files"
NODE="$(ls "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe 2>/dev/null | head -n 1)"
for t in ${WEBTOURS:-menus options}; do
	D="build/logs/ap/tour/coll-$t"; W="$D/webrun"
	rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$D/web" "$W/ud"
	printf '[SOFTWARE\\PopCap\\AstroPop]\nIs3D=dword:0\nScreenMode=dword:0\n' > "$W/ud/registry.ini"
	sed -E "s#^([0-9]+) screenshot .*/([^/]+\.png)\$#\1 screenshot shots/\2#" "$D/port.txt" > "$W/script.txt"
	( cd "$W" && "$NODE" "$R/games/bejeweled/tools/verify/wasmrun.js" game --timeout 1500 "$R/build/web/astropop.js" -fast -drawevery -seed=1 \
		-gamedir="$R/build/ap-coll/assets/astropop" -userdir=ud -realinput=script.txt > run.out 2>&1; echo "web coll-$t exit=$? shots=$(ls shots | wc -l)" )
	mv "$W/shots/"*.png "$D/web/" 2>/dev/null
	echo '{"client": [640, 480]}' > "$D/webshots.json"
	python tools/shotdiff/shotdiff.py --game astropop-coll --orig "$D/port" --port "$D/web" --run "web-coll-$t" --config "$D/webshots.json" --tolerance "${TOL:-0}" | tail -1
done
echo done
