#!/bin/bash
# origmem.sh <game: mahjongjapan|mahjongchina> <out dir> [times]: the original's memory at moments of the scripted play (the check's
# play through perfscript.py, as perfrun.sh orig), read by origmem.py from its memory map: private bytes by kind and the largest
# private regions at each of the seconds after its start (default 5,12,25,40,55). The original's saves are put away first and back at
# the end, as perfrun.sh does. Needs the staged original (perfrun.sh stages it); run from the repository's root:
#   bash games/mahjongjapan/tools/profile/origmem.sh mahjongjapan build/mem/orig-jp
cd "$(git rev-parse --show-toplevel)" || exit 1
export MSYS_NO_PATHCONV=1
GAME="$1"; OUT="$2"; TIMES="${3:-5,12,25,40,55}"
case "$GAME" in
	mahjongjapan) EXE=MahjongEscapeAJ.exe; SAVES='Mahjong Escape Ancient Japan'; CHECK=build/logs/check/mahjongjapan/game/script.txt ;;
	mahjongchina) EXE=MahjongEscapeAC.exe; SAVES='Mahjong Escape'; CHECK=build/logs/check/mahjongchina/game/script.txt ;;
esac
HERE=games/mahjongjapan/tools/profile
ST="${GAME}cov"; SD="E:/byo-og/$ST"; L="$LOCALAPPDATA/JollyBear/$SAVES"
mkdir -p "$OUT/saves"
python -I "$HERE/perfscript.py" "$CHECK" orig "$OUT/orig-script.txt" 6 > /dev/null
cp "$L"/* "$OUT/saves/" 2> /dev/null
if [ ! -f "$SD/game/$EXE" ]; then
	build/origrun-bin/origrun.exe --game "$GAME" --run "$ST" --stage-root 'E:\byo-og' --copy-only > "$OUT/stage.log" 2>&1
fi
PART="$(echo "$SD/game/$EXE" | tr 'A-Z' 'a-z')"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
rm -f "$L"/*.usr "$L"/*.sav
timeout 200 python -I "$HERE/origmem.py" "$OUT/mem.json" "$TIMES" --attach "$PART" > "$OUT/origmem.out" 2>&1 &
PM=$!
build/origrun-bin/origrun.exe --exe "$(cygpath -w "$SD/game/$EXE")" --run "$ST" --script "$(cygpath -w "$PWD/$OUT/orig-script.txt")" \
	--timeout 150 > "$OUT/origrun.log" 2>&1
wait $PM
rm -f "$L"/*.usr "$L"/*.sav
cp "$OUT/saves"/* "$L/" 2> /dev/null
cat "$OUT/origmem.out" | cut -c1-600
grep -E "RESULT|SOUND LEAK" "$OUT/origrun.log" | cut -c1-200
