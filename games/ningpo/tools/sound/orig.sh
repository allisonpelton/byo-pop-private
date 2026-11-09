#!/bin/bash
# orig.sh <tour dir> [timeout s]: the original's sounds on the screen tour (tools/shots/tour.py's orig.txt, its shots
# left out, since it runs on the host), on the real clock with origrun's silent sound stand-in (with the stand-in the
# virtual clock stalls once a game is dealt: IDENTICAL_PLAY.md, "Oddities"). The original mixes its sounds itself
# into one DirectSound buffer (HMixer, embedded in the static app at 0x475550 + 0x6930), so the stand-in's log shows
# one stream; instead origrun watches the first word of voices 0 to 3 (0x47be90 + 0x28 each), the sample's pointer,
# which the mixer sets as a sound starts and clears when it has played. The log goes to <tour dir>/sound-orig.log.
source "$(dirname "$0")/../identical/common.sh"
D="$1"; T="${2:-200}"
# on the real clock the event time is the machine's (timeGetTime), and it reads 0 until the game has read an event:
# the mark comes 100 ms after the first click, and every "at +N" is 100 ms earlier to match
grep -v " shot " "$D/orig.txt" | awk '/^mark$/ { print "wait 100"; print; next } /^at \+/ { sub(/^at \+[0-9]+/, "at +" (substr($2, 2) - 100)) } 1' > "$D/sound-orig.txt"
for v in Layout Background Tileset Autohint "Sound Volume" "Music Volume" Fullscreen; do
	reg delete "$REGKEY" /v "$v" /f > /dev/null 2>&1
done
for ((l = 0; l < 18; l++)); do for ((r = 0; r < 8; r++)); do
	reg delete "$REGKEY" /v "HS:${l}x${r}" /f > /dev/null 2>&1
done; done
"$ORIGRUN" --exe "$(win "$OG_GAME/Ningpo.exe")" --run "$(basename "$OG_STAGE")" --script "$(win "$PWD/$D/sound-orig.txt")" \
	--clock real --cursor scripted --counter 0x47be70 --update-ms 1 --sound "${ORIG_SOUND:-stub}" --seed 7 \
	--patches tools/origrun/patches/ningpo.txt --timeout "$T" ${COVERAGE:+--coverage "$COVERAGE"} \
	--watch 0x47be90 --watch 0x47beb8 --watch 0x47bee0 --watch 0x47bf08 --watch-max 4000 > "$D/sound-orig.log" 2>&1
grep -E "RESULT|SOUND LEAK" "$D/sound-orig.log" | tail -3; grep -c "watch " "$D/sound-orig.log"
