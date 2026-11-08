#!/bin/bash
# orig.sh <tour name> [timeout s] [extra origrun args]: the original on a tour, inside Windows Sandbox, on the virtual clock with the
# scripted cursor; shots go to build/ph2/<tour>/orig/. GS=<game seed>. The run's folder is build/origrun/rmsb-<tour> (staged from the
# install, the carved exe put in it).
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-300}; shift 2
D=build/ph2/$N; R=build/origrun/rmsb-$N
mkdir -p "$D"
if [ ! -f "$R/game/RocketManiaGame.exe" ]; then
	"$ORIGRUN" --game rocketmania --run "rmsb-$N" --copy-only > /dev/null || exit 1
	cp re/rocketmania/bin/RocketManiaGame.exe "$R/game/RocketManiaGame.exe"
fi
python games/rocketmania/tools/shots/make.py games/rocketmania/tools/shots/$N.txt "$D" orig > "$D/orig.txt"
rm -rf "$R/shots"; rm -f "$R"/game/Players/hiscores.dat "$R"/game/Players/save?.dat
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$PWD/$R/game/RocketManiaGame.exe")" --run "rmsb-$N" --script "$(win "$PWD/$D/orig.txt")" --sandbox \
	--clock virtual --cursor scripted --counter $COUNTER --update-ms 10 --seed ${GS:-1} --patches tools/origrun/patches/rocketmania.txt \
	--timeout "$T" "$@" > "$D/orig.log" 2>&1
rm -rf "$D/orig"; mkdir -p "$D/orig"; cp "$R"/shots/*.png "$D/orig/" 2>/dev/null
grep -E "RESULT|SOUND LEAK" "$D/orig.log" | tail -5; ls "$D/orig" | wc -l
