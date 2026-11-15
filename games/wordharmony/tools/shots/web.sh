#!/bin/bash
# web.sh <tour>: the web build (build/web/wordharmony.js) under Node (games/bejeweled/tools/verify/wasmrun.js) on a tour, as
# port.sh runs the native port (-fast, seed 1, the original's splash hold, no mods); its shots go to build/ph2/<tour>/web/,
# for shotdiff against the original's (cmp.sh <tour> --port build/ph2/<tour>/web). PORTREG=<registry.ini> as port.sh, and WH_GAMEDIR=<folder>.
source "$(dirname "$0")/../identical/common.sh"
ROOT="$PWD"
N=$1; D=build/ph2/$N; W=$D/webrun
NODE=$(ls /e/retools/emsdk/node/*/bin/node.exe 2>/dev/null | head -1); [ -n "$NODE" ] || NODE=$(ls /e/retools/emsdk/node/*/node.exe | head -1)
rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$W/user"
[ -z "$PORTREG" ] || cp "$PORTREG" "$W/user/registry.ini"
python games/wordharmony/tools/shots/make.py games/wordharmony/tools/shots/$N.txt "$D" web > "$W/script.txt"
( cd "$W" && "$ROOT/$RUNDESK" "$NODE" "$(win "$ROOT/games/bejeweled/tools/verify/wasmrun.js")" game "$(win "$ROOT/build/web/wordharmony.js")" \
	-gamedir="$(win "${WH_GAMEDIR:-$ROOT/assets/wordharmony}")" -userdir=user -nomods -fast -splashhold -seed=${GS:-1} -script=script.txt \
	-statelog=state.txt --timeout 280 > out.txt 2>&1 )
mv "$W/shots" "$D/web"; echo "$(ls "$D/web" | wc -l) shots"; tail -2 "$W/out.txt" | cut -c1-160
