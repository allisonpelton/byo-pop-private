#!/bin/bash
# randcase.sh <name> <mode> <bot seed> <game seed> [extra port arguments]: one game as case.sh plays it, but the one value compared is
# the C library's rand state after every update. The copy gets a patch that also stores _rand's state at 0x54f800
# (patches/alchemy-rand.txt), which origrun watches (--watch-max for a whole game's changes).
source "$(dirname "$0")/common.sh"
N="$1"; MODE="$2"; BS="$3"; GS="$4"; shift 4
D="$OUT/$N"
"$TOOLS/port.sh" "$D" -seed=$GS -autoplay=$BS -autoplayui -autoplaystart=450 -autoplaymode=$MODE -autoplaycap=30000 \
	-autoplaylog="$D/log.txt" -autoplayscript="$D/script.txt" -statelog="$D/state.txt" "$@" > /dev/null
grep "over at" "$D/log.txt" | tail -1
python "$TOOLS/toorig.py" "$D/script.txt" -1 > "$D/orig.txt"
LAST=$(tail -1 "$D/script.txt" | cut -d' ' -f1)
TO=$((LAST / 700 + 10))
WATCHMAX=200000 PATCHES=tools/origrun/patches/alchemy-rand.txt "$TOOLS/orig.sh" "$D/orig.txt" $GS $TO 0x54f800 > "$D/origrand.log" 2>&1
grep -c "watch 1" "$D/origrand.log"
python "$TOOLS/cmp.py" "$D/state.txt" "$D/origrand.log" rand
