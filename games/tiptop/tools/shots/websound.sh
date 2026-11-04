#!/bin/bash
# websound.sh <tour>: the web build under Node on a tour with -soundlog (build/ph2/<tour>/web_sound.log), compared with
# the native port's log from sound.sh (soundcmp.py <tour> web). GS=<seed> as port.sh. Needs build/web.
source "$(dirname "$0")/../identical/common.sh"
N=$1; D=build/ph2/$N; W=$D/websound
NODE=$(ls /e/retools/emsdk/node/*/bin/node.exe 2>/dev/null | head -1); [ -n "$NODE" ] || NODE=$(ls /e/retools/emsdk/node/*/node.exe | head -1)
rm -rf "$W"; mkdir -p "$W/user"
python games/tiptop/tools/shots/make.py games/tiptop/tools/shots/$N.txt "$D" port | grep -v " screenshot " > "$W/script.txt"
( cd "$W" && "$ROOT/$RUNDESK" "$NODE" "$(win "$ROOT/games/bejeweled/tools/verify/wasmrun.js")" game \
	"$(win "$ROOT/build/web/tiptop.js")" -gamedir="$(win "$ROOT/assets/tiptop")" -userdir=user -fast -seed=${GS:-1} \
	-script=script.txt -soundlog=sound.log --timeout 280 > out.txt 2>&1 )
cp "$W/sound.log" "$D/web_sound.log"
python games/tiptop/tools/shots/soundcmp.py $N web
