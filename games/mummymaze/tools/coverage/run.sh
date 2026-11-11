#!/bin/bash
# run.sh <name> <origrun script> <seed> [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host: origrun's
# --coverage on the list that `python -I tools/coverage.py list mummymaze` writes, the virtual clock and the scripted pointer
# the identical-play runs use. CLOCK=real runs on the real clock instead (the silent sound stand-in is origrun's default), so
# the sound code that the virtual clock skips runs. SAVE=<classic.sav> puts a saved game into the copy first (resume.sh's way,
# for the late game); SAVES=<folder> puts every .sav in it there (the adventures' adv0.sav ...). The hits go to build/coverage/mummymaze/hits/<name>.txt, the run's log beside them.
# Start it directly, not under rundesk (origrun puts the game there itself), from the worktree. It waits for room first: at
# most 3 game processes on the machine.
NAME="$1"; S="$2"; SEED="$3"; T="${4:-240}"
R="$(pwd -W)"; H=build/coverage/mummymaze/hits; mkdir -p "$H"
RUN="cov$NAME"; OG="$R/build/origrun-bin/origrun.exe"
COV="$R/build/coverage/mummymaze/functions.txt"
# the host takes no shots: the scenarios' shot lines go (the updates the other lines are timed to stay the same)
grep -v -E '(^|[[:space:]])(shot|waitshot|burst|record|hwmove|hwclick|hwrclick|hwkey)[[:space:]]' "$S" > "$H/$NAME.script.txt"
S="$R/$H/$NAME.script.txt"
until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 20; done
# the play count (the original shows its register nag from ten plays on; the copy is patched to be the registered game, but a
# run starts from zero all the same)
reg add "HKLM\Software\WOW6432Node\PopCap\MummyMaze" //v TimesPlayed //t REG_DWORD //d 0 //f > /dev/null
rm -rf "C:/byo-og/$RUN" "build/origrun/$RUN"
if [ -n "$SAVE" ] || [ -n "$SAVES" ]; then
	printf 'quit\n' > "$H/$NAME.stage.txt"
	"$OG" --game mummymaze --run "$RUN" --stage-root C:/byo-og --script "$R/$H/$NAME.stage.txt" --clock virtual --seed "$SEED" --timeout 60 > "$H/$NAME.stage.log" 2>&1
	mkdir -p "C:/byo-og/$RUN/game/data/saves"
	[ -z "$SAVE" ] || cp "$SAVE" "C:/byo-og/$RUN/game/data/saves/classic.sav"
	[ -z "$SAVES" ] || cp "$SAVES"/*.sav "C:/byo-og/$RUN/game/data/saves/"
	EXE=(--exe "C:/byo-og/$RUN/game/WinMM.exe")
fi
"$OG" --game mummymaze --run "$RUN" --stage-root C:/byo-og "${EXE[@]}" --script "$S" --clock "${CLOCK:-virtual}" --cursor scripted \
	--seed "$SEED" --timeout "$T" --coverage "$COV" > "$H/$NAME.log" 2>&1
cp "build/origrun/$RUN/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
