#!/bin/bash
# run.sh <name> <case dir> <delta> [seed] [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host: the
# case's script (tools/identical/case.sh wrote it for the port's bot) through tools/identical/orig.sh, with origrun's --coverage
# on the list `python -I tools/coverage.py list pixelus` writes. The hits go to build/coverage/pixelus/hits/<name>.txt.
source "$(dirname "$0")/../identical/common.sh"
NAME="$1"; D="$2"; DELTA="$3"; SEED="${4:-1}"; T="${5:-120}"
H=build/coverage/pixelus/hits; mkdir -p "$H"
python "$TOOLS/toorig.py" "$D/script.txt" "$DELTA" | grep -v ' shot ' > "$H/$NAME.script"
export EXTRA="--coverage $(win "$PWD/build/coverage/pixelus/functions.txt")"
R="build/origrun/$(basename "$OG_STAGE")"
rm -f "$R/coverage.txt"
until python tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
bash "$TOOLS/orig.sh" "$H/$NAME.script" "$SEED" "$T" > "$H/$NAME.log" 2>&1
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; tail -3 "$H/$NAME.log"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
