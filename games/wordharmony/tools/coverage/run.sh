#!/bin/bash
# run.sh <name> <tour or script> <seed> [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host, as
# identical/orig.sh runs it (under rundesk's private desktop via origrun, phase one's patches, the registry key cleared) with
# origrun's --coverage on the list that `python -I tools/coverage.py list wordharmony` writes. The argument is a phase two
# tour (tools/shots/<name>.txt) or a file of the port's -script lines (the shots of either are dropped: the host takes none).
# The virtual clock and the scripted cursor, unless CLOCK=real (the real clock, the silent stand-in; the frames then take
# their real 33 ms, so the timeout is in real seconds). REG=<file.reg> presets the registry (a saved game, identical/savedgame.py).
# ARG=<text> is added to the game's command line (origrun's --arg: -s starts the screen saver).
# The hits go to build/coverage/wordharmony/hits/<name>.txt, the run's log beside them.
source "$(dirname "$0")/../identical/common.sh"
NAME="$1"; S="$2"; SEED="$3"; T="${4:-240}"
H=build/coverage/wordharmony/hits; mkdir -p "$H" "$OUT"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
if [ -f "games/wordharmony/tools/shots/$S.txt" ]; then SRC="games/wordharmony/tools/shots/$S.txt"; else SRC="$S"; fi
python games/wordharmony/tools/shots/make.py "$SRC" "$OUT/cov-$NAME" orig | grep -v ' shot ' > "$OUT/cov-$NAME.txt"
R="build/origrun/$(basename "$OG_STAGE")"; rm -f "$R/coverage.txt"
if [ -z "$KEEP_REG" ]; then
	MSYS2_ARG_CONV_EXCL='*' reg delete "$REGKEY" /f > /dev/null 2>&1
fi
[ -n "$REG" ] && { reg import "$(win "$REG")" > /dev/null 2>&1 || echo "reg import $REG failed"; }
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/WordHarmonyGame.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$OUT/cov-$NAME.txt")" --clock "${CLOCK:-virtual}" --cursor scripted --counter 0x4aa600 --update-ms 33 --seed "$SEED" \
	--patches tools/origrun/patches/wordharmony.txt --coverage "$(win "$PWD/build/coverage/wordharmony/functions.txt")" \
	${ARG:+--arg "$ARG"} --timeout "$T" > "$H/$NAME.log" 2>&1
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1); $(grep RESULT "$H/$NAME.log" | tail -1)"
