#!/bin/bash
# run.sh <name> <origrun script> <seed> [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host:
# tools/identical/orig.sh (the virtual clock, no sound device) with origrun's --coverage on the list that
# `python -I tools/coverage.py list ningpo` writes. SOUND=1 runs tools/sound/orig.sh's way instead (the real clock, the
# silent stand-in, so the mixer's code runs; <origrun script> is then a tour folder). The hits go to
# build/coverage/ningpo/hits/<name>.txt, the run's log beside them.
source "$(dirname "$0")/../identical/common.sh"
NAME="$1"; S="$2"; SEED="$3"; T="${4:-240}"
H=build/coverage/ningpo/hits; mkdir -p "$H"
export COVERAGE="$(win "$PWD/build/coverage/ningpo/functions.txt")"
R="build/origrun/$(basename "$OG_STAGE")"
rm -f "$R/coverage.txt"
until python tools/gamecount.py --limit 3 > /dev/null; do sleep 20; done
if [ -n "$SOUND" ]; then
	bash "$(dirname "$0")/../sound/orig.sh" "$S" "$T" > /dev/null 2>&1
	cp "$S/sound-orig.log" "$H/$NAME.log"
else
	bash "$(dirname "$0")/../identical/orig.sh" "$S" "$SEED" "$T" > "$H/$NAME.log" 2>&1
fi
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
