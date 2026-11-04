#!/bin/bash
# web.sh <name> [seed]: the web build (build/web/mummymaze.js) under Node (games/bejeweled/tools/verify/wasmrun.js) on the scenario p2.sh made for
# <name> (build/p2/<name>.port.txt); its shots go to build/p2/<name>/web and are compared with the native port's by shotdiff. SAVE=<file> as p2.sh.
# Needs build/web (TARGET=mummymaze tools/build.sh web).
N="$1"; SEED="${2:-21}"; R="$(pwd -W)"; D="$R/build/p2"; W="$D/$N.webrun"
NODE=$(ls /e/retools/emsdk/node/*/node.exe | head -1)
rm -rf "$W" "$D/$N/web"; mkdir -p "$W/shots" "$W/user"
[ -z "$SAVE" ] || cp "$SAVE" "$W/user/classic.sav"
sed -E "s#screenshot .*/([^/]+\.png)#screenshot shots/\1#" "$D/$N.port.txt" > "$W/script.txt"
( cd "$W" && "$R/build/native/rundesk.exe" "$NODE" "$R/games/bejeweled/tools/verify/wasmrun.js" game "$R/build/web/mummymaze.js" \
	-gamedir="$R/assets/mummymaze" -userdir=user -fast -seed=$SEED -script=script.txt --timeout 900 > out.txt 2>&1 )
mv "$W/shots" "$D/$N/web"; ls "$D/$N/web" | wc -l
python tools/shotdiff/shotdiff.py --game mummymaze --orig "build/p2/$N/port" --port "build/p2/$N/web" --run "web-$N" --allow 0 --config build/p2/empty.json | tail -1
