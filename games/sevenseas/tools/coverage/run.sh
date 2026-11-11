#!/bin/bash
# run.sh <name> <origrun script> <seed> [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host: origrun's
# --coverage on the list that `python -I tools/coverage.py list sevenseas` writes, the virtual clock and the scripted pointer
# (CLOCK=real runs on the real clock, so the sound code the virtual clock skips runs). The hits go to
# build/coverage/sevenseas/hits/<name>.txt, the run's log beside them. Start it directly from the worktree, not under
# rundesk (origrun puts the game there itself). It waits for room first: at most 3 game processes on the machine.
NAME="$1"; S="$2"; SEED="$3"; T="${4:-240}"
R="$(pwd -W)"; H=build/coverage/sevenseas/hits; mkdir -p "$H"
RUN="cov$NAME"; OG="$R/build/origrun-bin/origrun.exe"
COV="$R/build/coverage/sevenseas/functions.txt"
grep -v -E '(^|[[:space:]])(shot|waitshot|burst|record|hwmove|hwclick|hwrclick|hwkey)[[:space:]]' "$S" > "$H/$NAME.script.txt"
S="$R/$H/$NAME.script.txt"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
rm -rf "build/origrun/$RUN"
# SAVE=<savegame.dat> puts a saved game into the copy first (the port writes the original's format: tools/coverage/README in
# GAME_NOTES "Coverage check"), and the run then uses that copy
if [ -n "$SAVE" ]; then
	"$OG" --game sevenseas --run "$RUN" --copy-only > "$H/$NAME.stage.log" 2>&1
	mkdir -p "build/origrun/$RUN/game/data"; cp "$SAVE" "build/origrun/$RUN/game/data/savegame.dat"
	EXE=(--exe "$R/build/origrun/$RUN/game/WinSS.exe")
fi
"$OG" --game sevenseas --run "$RUN" "${EXE[@]}" --script "$S" --clock "${CLOCK:-virtual}" --cursor scripted \
	--seed "$SEED" --timeout "$T" --coverage "$COV" > "$H/$NAME.log" 2>&1
cp "build/origrun/$RUN/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; tail -5 "$H/$NAME.log"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
