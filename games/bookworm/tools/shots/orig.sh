#!/bin/bash
# orig.sh <tour> [timeout s] [origrun args]: the original on a tour inside Windows Sandbox (origrun --sandbox, the only
# place origrun takes shots), on the virtual clock with the scripted cursor; shots go to build/ph2/<tour>/orig/. The
# run's folder is build/origrun/bwsb-<tour>, its copy made from setup.sh's master; each run starts with no players
# (the sandbox's registry is fresh every time). GS=<seed> (default 1).
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-150}; shift 2
D=build/ph2/$N; R=build/origrun/bwsb-$N
mkdir -p "$D"
if [ ! -f "$R/game/BookwormGame.exe" ]; then
	mkdir -p "$R" && cp -r "$OG_MASTER" "$R/game" || exit 1
fi
rm -rf "$R/shots" "$R/game/players"
python games/bookworm/tools/shots/make.py games/bookworm/tools/shots/$N.txt "$D" orig > "$D/orig.txt"
"$ORIGRUN" --exe "$(win "$PWD/$R/game/BookwormGame.exe")" --run "bwsb-$N" --script "$(win "$PWD/$D/orig.txt")" --sandbox \
	--clock virtual --cursor scripted --counter "$COUNTER" --update-ms 10 --seed ${GS:-1} \
	--patches tools/origrun/patches/bookworm.txt --timeout "$T" "$@" > "$D/orig.log" 2>&1
rm -rf "$D/orig"; mkdir -p "$D/orig"; cp "$R"/shots/*.png "$D/orig/" 2>/dev/null
grep -E "RESULT|SOUND LEAK|refus|error" "$D/orig.log" | tail -5; ls "$D/orig" | wc -l
