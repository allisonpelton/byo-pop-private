#!/usr/bin/env bash
# Runs the web build (build/web/bigmoney.js) under Node on a port script, as port.sh runs the native exe, and compares
# it with that native run: its -statelog and its shots (games/bigmoney/docs/IDENTICAL_PLAY.md, phase two).
#   games/bigmoney/tools/origcmp/webport.sh <run id> <script.txt> [seed]
# The native run must be build/bmcmp/<run> (port.sh or tour.sh); this one goes to build/bmcmp/<run>-web, with the
# shots compared by shotdiff as build/shotreview/bigmoney/<run>-web. Settings: PORT_SETTINGS as in port.sh (else the
# original's HKLM key). Headless: no window, no sound, no URL (wasmrun.js), so no rundesk.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HERE="$ROOT/games/bigmoney/tools/origcmp"
RUN="$1"; SCRIPT="$2"; SEED="${3:-1}"
NATIVE="$ROOT/build/bmcmp/$RUN"; DIR="$ROOT/build/bmcmp/$RUN-web"
NODE="$(ls /e/retools/emsdk/node/*/node.exe 2>/dev/null | head -n 1)"
rm -rf "$DIR"; mkdir -p "$DIR/user" "$DIR/mods"
cp "$SCRIPT" "$DIR/script.txt"
if [ -n "${PORT_SETTINGS:-}" ]; then python "$HERE/regini.py" --settings "$PORT_SETTINGS" > "$DIR/user/registry.ini"
else python "$HERE/regini.py" --hklm > "$DIR/user/registry.ini"; fi
cd "$DIR"
"${NODE:-node}" "$ROOT/games/bejeweled/tools/verify/wasmrun.js" game "$ROOT/build/web/bigmoney.js" \
	-gamedir="$(cygpath -w "$ROOT/assets/bigmoney")" -userdir=user -modsdir=mods -seed="$SEED" -fast -script=script.txt \
	-statelog=state.txt > out.txt 2>&1 || { echo "webport.sh: the web build's run failed (see $DIR/out.txt)"; exit 1; }
mkdir -p shots; mv ./*.png shots/ 2>/dev/null
if diff -q --strip-trailing-cr "$NATIVE/state.txt" state.txt > /dev/null; then echo "web state: same as native"; else echo "web state: DIFFERENT from native"; fi
python "$ROOT/tools/shotdiff/shotdiff.py" --game bigmoney --config "$HERE/web-shotdiff.json" --orig "$NATIVE/shots" \
	--port shots --run "$RUN-web" 2>&1 | tail -n 1
