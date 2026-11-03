#!/usr/bin/env bash
# One compared game: the port's bot plays it, the original gets the bot's input, the port replays the original's input,
# and statecmp.py compares the two runs' watched values (games/bigmoney/docs/IDENTICAL_PLAY.md).
#   games/bigmoney/tools/origcmp/game.sh <name> <mode 0..2> <skill 1..3> <seed> <cap updates> [bot args...]
# Runs one game process at a time. Output in build/bmcmp/<name>-bot, build/origrun/<name>, build/bmcmp/<name>;
# the comparison is printed and kept in build/bmcmp/<name>/statecmp.txt. WATCH3=mt0 as in orig.sh (then the port's
# watch 3 is compared with --names score,state,mt0,level, from its own log's fifth value).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HERE="$ROOT/games/bigmoney/tools/origcmp"
NAME="$1"; MODE="$2"; SKILL="$3"; SEED="$4"; CAP="$5"; shift 5
B="$ROOT/build/bmcmp/$NAME-bot"
bash "$HERE/port.sh" "$NAME-bot" - "$SEED" -autoplay="$SEED" -autoplaymode="$MODE" -autoplayskill="$SKILL" \
	-autoplaycap="$CAP" -autoplaylog="$(cygpath -w "$B")\bot.txt" "$@" || { echo "game.sh: the bot's run failed"; exit 1; }
[ -s "$B/bot.txt" ] || { echo "game.sh: the bot wrote no input"; exit 1; }
python "$HERE/fromport.py" "$B/bot.txt" > "$B/orig.txt" || exit 1
bash "$HERE/orig.sh" "$NAME" "$B/orig.txt" "$SEED" > "$B/orig-out.txt" 2>&1 || { echo "game.sh: the original's run failed (see $B/orig-out.txt)"; exit 1; }
OL="$ROOT/build/origrun/$NAME/logs/origrun.log"
python "$HERE/toport.py" "$OL" > "$B/port.txt" || exit 1
bash "$HERE/port.sh" "$NAME" "$B/port.txt" "$SEED" || { echo "game.sh: the port's replay failed"; exit 1; }
NAMES=score,state,mti,level
ST="$ROOT/build/bmcmp/$NAME/state.txt"
if [ "${WATCH3:-}" = mt0 ]; then
	NAMES=score,state,mt0,level
	sed -e '/^watch 3:/d' -e 's/^watch 5:/watch 3:/' "$ST" > "$ST.mt0"; ST="$ST.mt0"
fi
grep -E "^# (level|game) " "$B/bot.txt" | tail -n 3
python "$HERE/statecmp.py" "$OL" "$ST" --names "$NAMES" | tee "$ROOT/build/bmcmp/$NAME/statecmp.txt"
