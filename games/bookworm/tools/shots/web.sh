#!/bin/bash
# web.sh <tour>: the web build (build/web/bookworm.js) under Node (games/bejeweled/tools/verify/wasmrun.js) on a tour,
# -fast, under rundesk; its shots go to build/ph2/<tour>/web/. GS=<seed> as port.sh. Needs tools/build.sh web.
source "$(dirname "$0")/../identical/common.sh"
N=$1; D=build/ph2/$N; W=$D/webrun
NODE=$(ls /e/retools/emsdk/node/*/bin/node.exe 2>/dev/null | head -1); [ -n "$NODE" ] || NODE=$(ls /e/retools/emsdk/node/*/node.exe | head -1)
rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$W/user" "$W/mods"
python games/bookworm/tools/shots/make.py games/bookworm/tools/shots/$N.txt "$D" web > "$W/script.txt"
( cd "$W" && "$ROOT/$RUNDESK" "$(win "$NODE")" "$(win "$ROOT/games/bejeweled/tools/verify/wasmrun.js")" game "$(win "$ROOT/build/web/bookworm.js")" \
	-gamedir="$(win "$ROOT/assets/bookworm")" -userdir=user -modsdir=mods -fast -seed=${GS:-1} -script=script.txt --timeout 600 > out.txt 2>&1 )
mv "$W/shots" "$D/web"; ls "$D/web" | wc -l; tail -2 "$W/out.txt" | cut -c1-160
