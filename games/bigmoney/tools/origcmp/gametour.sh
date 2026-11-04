#!/usr/bin/env bash
# One compared game with shots and sound (phase two): the port's bot plays it from the sandbox runs' settings, its input
# becomes an origrun script with shots (fromport.py --shots), and tour.sh runs the original in the sandbox and the port
# on it and compares state, shots and sound.
#   games/bigmoney/tools/origcmp/gametour.sh <name> <mode 0..2> <skill 1..3> <seed> <cap updates> <shot every N> [bot args...]
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HERE="$ROOT/games/bigmoney/tools/origcmp"
NAME="$1"; MODE="$2"; SKILL="$3"; SEED="$4"; CAP="$5"; EVERY="$6"; shift 6
B="$ROOT/build/bmcmp/$NAME-bot"
PORT_SETTINGS="$HERE/settings.txt" bash "$HERE/port.sh" "$NAME-bot" - "$SEED" -autoplay="$SEED" -autoplaymode="$MODE" \
	-autoplayskill="$SKILL" -autoplaycap="$CAP" -autoplaylog="$(cygpath -w "$B")\bot.txt" "$@" ||
	{ echo "gametour.sh: the bot's run failed"; exit 1; }
[ -s "$B/bot.txt" ] || { echo "gametour.sh: the bot wrote no input"; exit 1; }
python "$HERE/fromport.py" "$B/bot.txt" --shots "$EVERY" > "$B/orig.txt" || exit 1
grep -E "^# (level|game) " "$B/bot.txt" | tail -n 3
bash "$HERE/tour.sh" "$NAME" "$B/orig.txt" "$SEED"
