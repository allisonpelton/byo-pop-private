#!/bin/bash
# case.sh <name> <mode> <bot seed> <game seed> [extra bot arguments]: one comparison. The port's bot plays (-autoplay;
# mode classic, action or both) and writes its input as a -script file, the port replays that script with -statelog,
# the original plays the same input on the same game seed, and its values (values.txt) are compared with the port's
# state log by cmp.py. origrun watches four values at a time, so the original runs once per four values. Files in
# build/identical/<name>. Prints the bot's last lines and one line per value; exits 1 if any differs.
# VALUES="a b c" compares only those values. The bot's input is shifted SHIFT updates later (default 600) before both
# replay it: the title's loading runs on a thread of its own at the machine's speed (the original unpacks its sounds
# until about update 130 under origrun, the port is done at 23), so the play link is clicked once both are surely done.
source "$(dirname "$0")/common.sh"
N="$1"; MODE="$2"; BS="$3"; GS="$4"; shift 4
D="$OUT/$N"
mkdir -p "$D"
"$TOOLS/port.sh" "$D/bot" -seed=$GS -autoplay=$BS -autoplaymode=$MODE -autoplaylog="$(win "$D/bot.txt")" "$@" > /dev/null
grep "^# update" "$D/bot.txt" | grep -i "game over\|hall of fame" | tail -3
awk -v d="${SHIFT:-600}" '/^[0-9]/ { $1 += d } { print }' "$D/bot.txt" > "$D/input.txt"
"$TOOLS/port.sh" "$D/port" -seed=$GS -script="$(win "$D/input.txt")" -statelog="$(win "$D/state.txt")" > /dev/null
python "$TOOLS/toorig.py" "$D/input.txt" > "$D/orig.txt"
LAST=$(grep -v "^#" "$D/input.txt" | tail -1 | cut -d' ' -f1)
TO=$((LAST / 150 + 90))
NAMES=(${VALUES:-$(grep -v '^#' "$TOOLS/values.txt" | awk 'NF { print $1 }')})
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	G=("${NAMES[@]:i:4}")
	"$TOOLS/orig.sh" "$D/orig.txt" $GS $TO "${G[@]}" > "$D/orig$((i / 4)).log" 2>&1
	python "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "${G[@]}" || st=1
done
python "$TOOLS/hiscore.py" "$D/port/user/registry.ini" "$D/orig0.log" || st=1
exit $st
