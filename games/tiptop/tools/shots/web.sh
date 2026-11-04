#!/bin/bash
# web.sh <tour stem>: the web build (build/web/tiptop.js) under Node (games/bejeweled/tools/verify/wasmrun.js) on a tour; its shots go to
# build/ph2/<tour>/web/. SAVE=<file>, GS=<game seed> as port.sh. Needs build/web (tools/build.sh web).
source "$(dirname "$0")/../identical/common.sh"
N=$1; D=build/ph2/$N; W=$D/webrun
NODE=$(ls /e/retools/emsdk/node/*/bin/node.exe 2>/dev/null | head -1); [ -n "$NODE" ] || NODE=$(ls /e/retools/emsdk/node/*/node.exe | head -1)
rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$W/user"
[ -z "$SAVE" ] || cp "$SAVE" "$W/user/savegame.dat"
python games/tiptop/tools/shots/make.py games/tiptop/tools/shots/$N.txt "$D" web > "$W/script.txt"
( cd "$W" && "$ROOT/$RUNDESK" "$NODE" "$(win "$ROOT/games/bejeweled/tools/verify/wasmrun.js")" game "$(win "$ROOT/build/web/tiptop.js")" \
	-gamedir="$(win "$ROOT/assets/tiptop")" -userdir=user -fast -seed=${GS:-1} -script=script.txt --timeout 280 > out.txt 2>&1 )
mv "$W/shots" "$D/web"; ls "$D/web" | wc -l; tail -2 "$W/out.txt" | cut -c1-160
