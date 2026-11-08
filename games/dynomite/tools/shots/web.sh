#!/bin/bash
# web.sh <tour> [cmp]: a tour (tour.sh, which writes build/ph2/<tour>/script.txt) in the web build
# (build/web/dynomite.js, tools/build.sh web; WEBJS names another, such as one linked with --profiling-funcs) under Node
# (games/bejeweled/tools/verify/wasmrun.js), -fast -drawevery like the native port, on a fresh user folder with
# port.sh's settings and mkhs.py's tables; its shots go to build/ph2/<tour>/web/. "cmp" then compares them with the
# native port's (shotdiff, the native shots as "orig", no difference allowed) and prints one line per pair that is not
# identical.
source "$(dirname "$0")/../identical/common.sh"
N=$1; D=build/ph2/$N; W=$D/webrun
NODE=$(ls /e/retools/emsdk/node/*/bin/node.exe /e/retools/emsdk/node/*/node.exe 2>/dev/null | head -1)
[ -f "$D/script.txt" ] || { echo "run tour.sh $N port first"; exit 1; }
rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$W/user"
printf '%s\n' '[SOFTWARE\PopCap\Dynomite]' CustomCursors=dword:0 InProgress=dword:0 MusicVolume=dword:50 Muted=dword:0 \
	PreferredX=dword:-1 PreferredY=dword:-1 ScreenMode=dword:1 SfxVolume=dword:85 TimesExecuted=dword:5 \
	TimesPlayed=dword:0 WaitForVSync=dword:0 > "$W/user/registry.ini"
python "$TOOLS/mkhs.py" "$W/user"
[ -z "$SAVE" ] || cp "$SAVE" "$W/user/SLOT0.SAV"
SEED=$(grep "^$N " games/dynomite/tools/shots/tours.txt | cut -d' ' -f2)
sed -E 's#screenshot [^ ]*[/\]([^/\ ]+\.png)#screenshot shots/\1#' "$D/script.txt" > "$W/script.txt"
( cd "$W" && "$ROOT/$RUNDESK" "$NODE" "$(win "$ROOT/games/bejeweled/tools/verify/wasmrun.js")" game --timeout 280 "$(win "$ROOT/${WEBJS:-build/web/dynomite.js}")" \
	-gamedir="$(win "$ROOT/assets/dynomite")" -userdir=user -fast -drawevery -seed=$SEED -script=script.txt $WEBARGS > out.txt 2>&1 )
mv "$W/shots" "$D/web"; echo "web shots: $(ls "$D/web" | wc -l)"; tail -1 "$W/out.txt" | cut -c1-160
if [ "$2" = cmp ]; then
	python tools/shotdiff/shotdiff.py --game dynomite --orig "$D/port" --port "$D/web" --run "web-$N" --allow 0 | tail -1
fi
