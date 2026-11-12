#!/bin/bash
# run.sh <name> <tour> [seed] [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host: a tour of
# games/rocketmania/tools/shots/, or a script file given as the tour (a bot's -uiscript); make.py's host script has its clicks and
# keys, no shots) through tools/identical/orig.sh (the
# virtual clock, no sound device) with origrun's --coverage on the list that `python -I tools/coverage.py list rocketmania` writes.
# RM_REG="Name=value,..." sets the registry the original starts with. The hits go to build/coverage/rocketmania/hits/<name>.txt,
# the run's log beside them.
source "$(dirname "$0")/../identical/common.sh"
NAME="$1"; TOUR="$2"; SEED="${3:-1}"; T="${4:-300}"
H=build/coverage/rocketmania/hits; mkdir -p "$H"
S="$H/$NAME.script.txt"
TOURFILE="games/rocketmania/tools/shots/$TOUR.txt"; [ -f "$TOUR" ] && TOURFILE="$TOUR"
python games/rocketmania/tools/shots/make.py "$TOURFILE" "$H" host > "$S"
export COVERAGE="$(win "$PWD/build/coverage/rocketmania/functions.txt")"
R="build/origrun/$(basename "$OG_STAGE")"
rm -f "$R/coverage.txt"
until python tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
bash "$(dirname "$0")/../identical/orig.sh" "$S" "$SEED" "$T" > "$H/$NAME.log" 2>&1
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
