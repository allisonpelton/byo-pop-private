#!/bin/bash
# save.sh <name> <mode> <bot seed> <game seed> <cap>: a saved game written by one program and continued by the other.
# Session 1: the port's bot plays <mode> (classic or action) and quits at update <cap>, mid-game, so each program saves
# the game as it shuts down (players/<name>/<mode>.dat: the original's beside its exe, the port's in its user folder);
# the port (drawing every update: the last word's colour in the file is set as the board draws, and -fast alone
# skips draws) and the
# original replay that input and the two files must be the same, byte for byte, but for the original's uninitialised
# fields (savecmp.py). Session 2: the bot
# continues a saved game (-autoplaycontinue=1) and plays it out; the port replays that input from the original's file
# and the original from the port's, and their values are compared as case.sh compares them. Neither program keeps its
# players from one session to the next (the original cannot write its HKLM key, the port starts a fresh user folder),
# so both meet the new-player dialog again. Files in build/identical/<name>.
source "$(dirname "$0")/common.sh"
N="$1"; MODE="$2"; BS="$3"; GS="$4"; CAP="$5"
D="$OUT/$N"
mkdir -p "$D"
SH="${SHIFT:-600}"
st=0

# session 1
"$TOOLS/port.sh" "$D/bot1" -seed=$GS -autoplay=$BS -autoplaymode=$MODE -autoplaycap=$CAP -autoplaylog="$(win "$D/bot1.txt")" > /dev/null
awk -v d="$SH" '/^[0-9]/ { $1 += d } { print }' "$D/bot1.txt" > "$D/input1.txt"
"$TOOLS/port.sh" "$D/port1" -seed=$GS -drawevery -script="$(win "$D/input1.txt")" -statelog="$(win "$D/state1.txt")" > /dev/null
python "$TOOLS/toorig.py" "$D/input1.txt" > "$D/orig1.txt"
LAST=$(grep -v "^#" "$D/input1.txt" | tail -1 | cut -d' ' -f1)
"$TOOLS/orig.sh" "$D/orig1.txt" $GS $((LAST / 150 + 90)) score > "$D/orig1.log" 2>&1
rm -rf "$D/save-port" "$D/save-orig"; mkdir -p "$D/save-port" "$D/save-orig"
cp -r "$D/port1/user/players" "$D/save-port/" 2>/dev/null
cp -r "$OG_GAME/players" "$D/save-orig/" 2>/dev/null
P="$D/save-port/players/Autoplay Bot/$MODE.dat"; O="$D/save-orig/players/Autoplay Bot/$MODE.dat"
[ -f "$P" ] || P=; [ -f "$O" ] || O=
if [ -z "$P" ] || [ -z "$O" ]; then
	echo "saved game: port ${P:-none}, original ${O:-none}"; exit 1
fi
python "$TOOLS/savecmp.py" "$P" "$O" "$D/state1.txt" || st=1

# session 2: the bot continues the port's save, then each program continues the other's
PORT_SAVE="$D/save-port" "$TOOLS/port.sh" "$D/bot2" -seed=$GS -autoplay=$((BS + 1)) -autoplaymode=$MODE -autoplaycontinue=1 \
	-autoplaylog="$(win "$D/bot2.txt")" > /dev/null
grep "^# update" "$D/bot2.txt" | grep -i "continue\|game over" | head -3
awk -v d="$SH" '/^[0-9]/ { $1 += d } { print }' "$D/bot2.txt" > "$D/input2.txt"
PORT_SAVE="$D/save-orig" "$TOOLS/port.sh" "$D/port2" -seed=$GS -script="$(win "$D/input2.txt")" -statelog="$(win "$D/state.txt")" > /dev/null
python "$TOOLS/toorig.py" "$D/input2.txt" > "$D/orig2.txt"
LAST=$(grep -v "^#" "$D/input2.txt" | tail -1 | cut -d' ' -f1)
NAMES=(${VALUES:-$(grep -v '^#' "$TOOLS/values.txt" | awk 'NF { print $1 }')})
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	G=("${NAMES[@]:i:4}")
	ORIG_SAVE="$D/save-port" "$TOOLS/orig.sh" "$D/orig2.txt" $GS $((LAST / 150 + 90)) "${G[@]}" > "$D/orig2-$((i / 4)).log" 2>&1
	python "$TOOLS/cmp.py" "$D/state.txt" "$D/orig2-$((i / 4)).log" "${G[@]}" || st=1
done
python "$TOOLS/hiscore.py" "$D/port2/user/registry.ini" "$D/orig2-0.log" || st=1
exit $st
