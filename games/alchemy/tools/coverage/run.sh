#!/bin/bash
# run.sh <name> <origrun script> <seed> [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host:
# tools/identical/orig.sh (the virtual clock, no sound device) with origrun's --coverage on the list that
# `python -I tools/coverage.py list alchemy` writes. REAL=1 runs on the real clock with the silent stand-in instead, so that the
# music and mixing code runs (the script's updates then come at 100 a second). KEEP_SAVE=1 keeps the saved game a test put in
# the copy's folder. The hits go to build/coverage/alchemy/hits/<name>.txt, the run's log beside them; a run waits for room first
# (python -I tools/gamecount.py --limit 3).
source "$(dirname "$0")/../identical/common.sh"
NAME="$1"; S="$2"; SEED="$3"; T="${4:-240}"
H=build/coverage/alchemy/hits; mkdir -p "$H"
export COVERAGE="$(win "$PWD/build/coverage/alchemy/functions.txt")"
R="build/origrun/$(basename "$OG_STAGE")"
rm -f "$R/coverage.txt"
until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 20; done
if [ -n "$REAL" ]; then
	rm -f "$OG_GAME/hiscores.dat"; [ -n "$KEEP_SAVE" ] || rm -f "$OG_GAME/savegame.dat"
	"$ORIGRUN" --exe "$(win "$OG_GAME/AlchemyGame.exe")" --run "$(basename "$OG_STAGE")" --script "$(win "$S")" --clock real \
		--counter 0x516570 --update-ms 10 --seed "$SEED" --patches "${PATCHES:-tools/origrun/patches/alchemy.txt}" \
		--timeout "$T" --coverage "$COVERAGE" > "$H/$NAME.log" 2>&1
else
	bash "$TOOLS/orig.sh" "$S" "$SEED" "$T" > "$H/$NAME.log" 2>&1
fi
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
