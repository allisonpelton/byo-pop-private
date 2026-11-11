#!/usr/bin/env bash
# One coverage run of the original Big Money Deluxe (docs/COVERAGE.md) on the host: origrun's --coverage on the list that
# `python -I tools/coverage.py list bigmoney` writes (build/coverage/bigmoney/functions.txt), through orig.sh (the virtual clock,
# no sound device, unless ORIG_CLOCK=real: the real clock with the silent stand-in, so the sound code runs). The hits go to
# build/coverage/bigmoney/hits/<name>.txt, the run's output beside them.
#   games/bigmoney/tools/coverage/run.sh game   <name> <mode 0..2> <skill 1..3> <seed> <cap updates> [bot args...]
#       the port's bot plays a game (as origcmp/game.sh does), and the original gets the bot's input (fromport.py)
#   games/bigmoney/tools/coverage/run.sh script <name> <origrun script> [seed]
#       the original plays an origrun script (origcmp/tours/*.txt, or a short one of this folder)
# ORIG_TIMEOUT (default 400 s) is origrun's timeout. Waits for room first: at most 3 game processes on the machine.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HERE="$ROOT/games/bigmoney/tools/origcmp"
H="$ROOT/build/coverage/bigmoney/hits"; mkdir -p "$H"
KIND="$1"; NAME="$2"; shift 2
cd "$ROOT" || exit 1
wait_room() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done; }
case "$KIND" in
game)
	MODE="$1"; SKILL="$2"; SEED="$3"; CAP="$4"; shift 4
	B="$ROOT/build/bmcmp/$NAME-bot"
	wait_room
	bash "$HERE/port.sh" "$NAME-bot" - "$SEED" -autoplay="$SEED" -autoplaymode="$MODE" -autoplayskill="$SKILL" \
		-autoplaycap="$CAP" -autoplaylog="$(cygpath -w "$B")\\bot.txt" "$@" || { echo "$NAME: the bot's run failed"; exit 1; }
	[ -s "$B/bot.txt" ] || { echo "$NAME: the bot wrote no input"; exit 1; }
	python "$HERE/fromport.py" "$B/bot.txt" > "$B/orig.txt" || exit 1
	SCRIPT="$B/orig.txt" ;;
script)
	SCRIPT="$1"; SEED="${2:-1}" ;;
*) echo "usage: run.sh game|script <name> ..." >&2; exit 2 ;;
esac
SEED="${SEED:-1}"
rm -f "$ROOT/build/origrun/$NAME/coverage.txt"
wait_room
ORIG_TIMEOUT="${ORIG_TIMEOUT:-400}" bash "$HERE/orig.sh" "$NAME" "$SCRIPT" "$SEED" --coverage "$(cygpath -w "$ROOT/build/coverage/bigmoney/functions.txt")" \
	> "$H/$NAME.log" 2>&1
cp "$ROOT/build/origrun/$NAME/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file (see $H/$NAME.log)"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1); $(grep RESULT "$H/$NAME.log" | tail -n 1)"
