#!/bin/bash
# orig.sh <tour dir> [timeout s] [extra origrun args]: the original on <tour dir>/orig.txt (tour.py) inside Windows
# Sandbox, the only place origrun takes shots (docs/ORIGRUN.md), on the virtual clock with the scripted cursor and seed 7,
# with no sound device (--sound nodriver: with the stand-in the exe's music streamer stalls the virtual clock once a game
# is dealt, docs/ORIGRUN.md's row for Ning Po). The copy is staged in build/origrun/npsb with the exe of the identical-play
# copy (setup.sh's patches); its shots go to <tour dir>/orig/, the log to <tour dir>/orig.log.
source "$(dirname "$0")/../identical/common.sh"
D="$1"; T="${2:-300}"; shift; shift
R=build/origrun/npsb
if [ ! -f "$R/game/ningpo.pyz" ]; then
	"$ORIGRUN" --game ningpo --run npsb --copy-only > /dev/null || exit 1
fi
# origrun patches the copy from its saved unpatched exe (setup.sh made both)
cp "$OG_GAME/Ningpo.exe" "$OG_GAME/Ningpo.exe.origrun-unpatched" "$R/game/"
rm -rf "$R/shots"
ORIGRUN_CLOCK_GUARD=0 "$ORIGRUN" --exe "$(win "$PWD/$R/game/Ningpo.exe")" --run npsb --script "$(win "$PWD/$D/orig.txt")" \
	--sandbox --clock virtual --cursor scripted --counter 0x47be70 --update-ms 1 --sound nodriver --seed 7 \
	--patches tools/origrun/patches/ningpo.txt --timeout "$T" "$@" > "$D/orig.log" 2>&1
rm -rf "$D/orig"; mkdir -p "$D/orig"; cp "$R"/shots/*.png "$D/orig/" 2>/dev/null
grep -E "RESULT|SOUND LEAK" "$D/orig.log" | tail -3; ls "$D/orig" | wc -l
