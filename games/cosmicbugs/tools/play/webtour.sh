#!/usr/bin/env bash
# webtour.sh <name> <runner script> [save]: one script played by the headless runner, the exe (-fast, under rundesk)
# and the web build under Node (wasmrun.js: no window, no sound, no URL), each from the same save (cb.dat; else a new
# player), with their state lines and shots in build/cbw/<name>-run, -exe and -web. The exe's shots are compared with
# the runner's and the web build's with the exe's by tools/shotdiff (runs <name>-exe and <name>-web).
set -uo pipefail
cd "$(dirname "$0")/../../../.."
N=$1; S=$2; SAVE=${3:-}
NODE="$(ls /e/retools/emsdk/node/*/node.exe 2>/dev/null | head -n 1)"
CFG='<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n'
for k in run exe web; do
	D=build/cbw/$N-$k; rm -rf $D; mkdir -p $D/user $D/shots
	printf "$CFG" > $D/user/cp.cfg
	[ -n "$SAVE" ] && cp "$SAVE" $D/user/cb.dat
done
cp build/cbw/$N-run/user/* build/cbw/$N-run/
build/native/cosmicbugs_run.exe assets/cosmicbugs build/cbw/$N-run "$S" > build/cbw/$N-run/out.txt 2>&1
mv build/cbw/$N-run/*.png build/cbw/$N-run/shots/ 2>/dev/null
D=$(cygpath -ma build/cbw/$N-exe)
python games/cosmicbugs/tools/play/fwscript.py "$S" "$D/shots" build/cbw/$N-exe/script.txt
timeout 900 build/native/rundesk.exe "$(cygpath -wa build/native/cosmicbugs.exe)" -gamedir="$(cygpath -wa assets/cosmicbugs)" -userdir="$D/user" -fast \
	-script="$D/script.txt" > build/cbw/$N-exe/out.txt 2>&1 || echo "webtour.sh: the exe exited with $?"
python games/cosmicbugs/tools/play/fwscript.py "$S" shots build/cbw/$N-web/script.txt
(cd build/cbw/$N-web && timeout 1800 "${NODE:-node}" ../../../games/bejeweled/tools/verify/wasmrun.js game ../../web/cosmicbugs.js \
	-gamedir="$(cygpath -w ../../../assets/cosmicbugs)" -userdir=user -fast -script=script.txt > out.txt 2>&1) ||
	echo "webtour.sh: the web build exited with $?"
for k in exe web; do
	echo "$k: $(ls build/cbw/$N-$k/shots | wc -l) shots; state: $(tr -d '\r' < build/cbw/$N-$k/out.txt | sed -n 's/.*INFO: state: //p' | tr '\n' ';')"
done
echo "run: $(ls build/cbw/$N-run/shots | wc -l) shots; state: $(grep ' ms: score' build/cbw/$N-run/out.txt | tr '\n' ';')"
python tools/shotdiff/shotdiff.py --game cosmicbugs --orig build/cbw/$N-run/shots --port build/cbw/$N-exe/shots --run $N-exe 2>&1 | tail -n 1
python tools/shotdiff/shotdiff.py --game cosmicbugs --orig build/cbw/$N-exe/shots --port build/cbw/$N-web/shots --run $N-web 2>&1 | tail -n 1
