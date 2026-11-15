#!/bin/bash
# run.sh <name> <port -script> <seed> [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host: the
# port script turned into origrun's (identical/toorig.py, no shots), played by identical/orig.sh (the virtual clock, the
# scripted cursor, the seed patches, settings.reg and mkhs.py's low tables) with origrun's --coverage on the list that
# `python -I tools/coverage.py list dynomite` writes. SAVE=<file> starts it with that saved game in the first slot;
# REAL=1 runs it on the real clock with the silent stand-in, so the sound code runs too. It waits for a free game slot
# first (tools/gamecount.py). The hits go to build/coverage/dynomite/hits/<name>.txt, the run's log beside them.
source "$(dirname "$0")/../identical/common.sh"
NAME="$1"; S="$2"; SEED="$3"; T="${4:-900}"
H=build/coverage/dynomite/hits; mkdir -p "$H"
R="build/origrun/$(basename "$OG_STAGE")"
python $TOOLS/toorig.py "$S" -1 > "$H/$NAME.orig.txt"
rm -f "$R/coverage.txt"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
ARGS="--coverage $(win "$PWD/build/coverage/dynomite/functions.txt")"
if [ -n "$REAL" ]; then
	# orig.sh's settings with the real clock: origrun's last --clock wins
	ARGS="$ARGS --clock real"
fi
ORIG_ARGS="$ARGS" HS=1 SAVE="$SAVE" bash $TOOLS/orig.sh "$(win "$PWD/$H/$NAME.orig.txt")" "$SEED" "$T" > "$H/$NAME.log" 2>&1
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
