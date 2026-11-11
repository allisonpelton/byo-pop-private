#!/usr/bin/env bash
# The port on the PopCap Game Collection's files, against the same port on the Steam copy's (GAME_NOTES.md, "The PopCap Game
# Collection version"). Needs build/native/typershark.exe and rundesk, assets/typershark staged from the Steam copy, and the web
# build (tools/check.sh build-web:typershark) for the last step.
#   games/typershark/tools/collection/run.sh <the Collection's TyperSharkDeluxeComponent folder>
# Stages the Collection's files into build/ts-coll/assets/typershark with a copy of the exe in build/ts-coll/bin (the exe finds
# assets/typershark one level up), then: tools/regress.py on it (the state-log hashes must equal regress_expected.txt), the tours
# M, H and I and the title screen (tour_title.py) once on each set of files and compared by shotdiff (every pair must match), and the web
# build under Node on the Collection's files (the title and menu tours against the native shots, within the web build's own few levels).
# One game process at a time, invisible and silent under rundesk, with -fast.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
SRC="$1"
P=games/typershark/tools/phase2
mkdir -p build/ts-coll/bin
python -I games/typershark/tools/stage_typershark_assets.py "$SRC" --out build/ts-coll/assets/typershark || exit 1
cp build/native/typershark.exe build/native/*.dll build/ts-coll/bin/
echo "--- regress on the Collection's files"
python games/typershark/tools/regress.py --exe build/ts-coll/bin/typershark.exe --no-baseline | tail -1
for t in menu hall idle2; do python -I $P/tour_$t.py "steam-$t" > /dev/null; python -I $P/tour_$t.py "coll-$t" > /dev/null; done
for k in steam coll; do python -I games/typershark/tools/collection/tour_title.py "$k-title"; done
for t in menu hall idle2 title; do
	TSEXE="$R/build/native/typershark.exe" bash $P/runport.sh "steam-$t" 1 -drawevery | tail -1
	TSEXE="$R/build/ts-coll/bin/typershark.exe" bash $P/runport.sh "coll-$t" 1 -drawevery | tail -1
	# both are the port's own 640x480 pictures: a game name with no shotdiff configuration, so no crop
	python tools/shotdiff/shotdiff.py --game typershark-coll --orig "build/ts-p2/steam-$t/port" --port "build/ts-p2/coll-$t/port" --run "coll-$t" | tail -1
done
echo "--- the web build under Node on the Collection's files"
NODE="$(ls "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe 2>/dev/null | head -n 1)"
for N in coll-title coll-menu; do
	D="build/ts-p2/$N"; W="$D/webrun"
	bash $P/waitroom.sh || exit 3
	rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$D/web"
	cp -r build/ts-p1/fixtures/port-win "$W/ud"
	sed -E "s#^([0-9]+) shot .*/([^/]+\.png)\$#\1 shot shots/\2#" "$D/port.txt" > "$W/script.txt"
	( cd "$W" && "$NODE" "$R/games/bejeweled/tools/verify/wasmrun.js" game "$R/build/web/typershark.js" -fast -drawevery -seed=1 \
		-gamedir="$R/build/ts-coll/assets/typershark" -userdir=ud -script=script.txt --timeout 1500 > run.out 2>&1; echo "$N web exit=$? shots=$(ls shots | wc -l)" )
	mv "$W/shots/"*.png "$D/web/" 2>/dev/null
	python tools/shotdiff/shotdiff.py --game typershark-coll --orig "$D/port" --port "$D/web" --run "web-$N" --tolerance 0 | tail -1
done
echo done
