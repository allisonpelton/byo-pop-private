#!/bin/bash
# run.sh <name> <origrun script> [timeout s]: one coverage run of the original Bonnie's Bookstore (docs/COVERAGE.md) on
# the host, from the repository root. origrun runs the copy in build/origrun/bonnies-cov on its real clock with the
# silent stand-in (so FMOD's sound code runs), its seed pinned as the port's -seed=1, and --coverage on the list that
# `python -I tools/coverage.py list bonnies` writes. The copy keeps its saves from run to run, so a script that needs the
# tour's saves follows the tour; FRESH=1 starts from a clean copy of the installed game (build/perf/template, see
# tools/perf.py). The script's shots are left out (origrun takes none on the host), and its first waitshot becomes a
# wait. SAVE=<file> puts that file in the copy as SavedGames/te.GAM first; SAVEDIR=<folder> replaces its SavedGames. The hits go to
# build/coverage/bonnies/hits/<name>.txt, origrun's log beside them.
NAME="$1"; S="$2"; T="${3:-300}"
H=build/coverage/bonnies/hits; mkdir -p "$H"
win() { cygpath -w "$1"; }
R=build/origrun/bonnies-cov
if [ -n "$FRESH" ] || [ ! -d "$R/game" ]; then rm -rf "${R:?}"; mkdir -p "$R"; cp -r build/perf/template "$R/game"; fi
[ -n "$SAVE" ] && cp "$SAVE" "$R/game/SavedGames/te.GAM"
[ -n "$SAVEDIR" ] && rm -rf "${R:?}/game/SavedGames" && cp -r "$SAVEDIR" "$R/game/SavedGames"
grep -v "^shot " "$S" | sed 's/^waitshot logo 20000$/wait 9000/; s/^waitshot \S* /wait /' > "$H/$NAME.script.txt"
rm -f "$R/coverage.txt"
until python -I tools/gamecount.py --limit 3 > /dev/null; do python -c "import time; time.sleep(20)"; done
build/origrun-bin/origrun.exe --exe "$(win "$PWD/$R/game/BonniesGame.exe")" --run bonnies-cov \
	--script "$(win "$PWD/$H/$NAME.script.txt")" --clock real --seed 1 --timeout "$T" \
	--patches "$(win "$PWD/games/bonnies/tools/origrun/patches.txt")" \
	--coverage "$(win "$PWD/build/coverage/bonnies/functions.txt")" > "$H/$NAME.log" 2>&1
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1); $(grep -a RESULT "$H/$NAME.log" | tail -1)"
