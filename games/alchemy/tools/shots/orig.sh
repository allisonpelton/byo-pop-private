#!/bin/bash
# orig.sh <tour name> (SAVE=<file> puts that saved game in the copy) [timeout s] [extra origrun args]: the original on a tour, inside Windows Sandbox, on the virtual clock with the
# scripted cursor; shots go to build/ph2/<tour>/orig/. The run's folder is build/origrun/alsb-<tour> (staged from the install).
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-60}; shift 2
D=build/ph2/$N; R=build/origrun/alsb-$N
mkdir -p "$D/orig"
if [ ! -f "$R/game/AlchemyGame.exe" ]; then
	"$ORIGRUN" --game alchemy --run "alsb-$N" --copy-only > /dev/null || exit 1
	cp re/alchemy/bin/AlchemyGame.exe "$R/game/AlchemyGame.exe"
fi
python games/alchemy/tools/shots/make.py games/alchemy/tools/shots/$N.txt "$D" orig > "$D/orig.txt"
rm -rf "$R/shots"; rm -f "$R/game/savegame.dat" "$R/game/hiscores.dat"
[ -z "$SAVE" ] || cp "$SAVE" "$R/game/savegame.dat"
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$PWD/$R/game/AlchemyGame.exe")" --run "alsb-$N" --script "$(win "$PWD/$D/orig.txt")" --sandbox \
	--clock virtual --cursor scripted --counter 0x516570 --update-ms 10 --seed ${GS:-1} --patches tools/origrun/patches/alchemy.txt --timeout "$T" "$@" > "$D/orig.log" 2>&1
rm -rf "$D/orig"; mkdir -p "$D/orig"; cp "$R"/shots/*.png "$D/orig/" 2>/dev/null
grep -E "RESULT|SOUND LEAK" "$D/orig.log" | tail -5; ls "$D/orig" | wc -l
