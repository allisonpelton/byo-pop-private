#!/usr/bin/env bash
# orig.sh <script> <run> [extra origrun args]: the original in a sandbox from a fresh copy of build/orig/tmpl (plus $SAVES)
cd "$(dirname "$0")/../.." || exit 1
R="$(pwd -W)"; S="$1"; RUN="$2"; shift 2
rm -rf "build/origrun/$RUN"
mkdir -p "build/origrun/$RUN"; cp -r build/orig/tmpl "build/origrun/$RUN/game"
[ -n "$SAVES" ] && cp -r "$SAVES"/. "build/origrun/$RUN/game/SavedGames/"
until python -I tools/gamecount.py --limit 6 >/dev/null; do sleep 10; done
build/origrun-bin/origrun.exe --exe "$R/build/origrun/$RUN/game/BonniesGame.exe" --run "$RUN" --script "$S" --sandbox --patches games/bonnies/tools/origrun/patches.txt --seed 1 --sandbox-wait 7200 --timeout "${TMO:-400}" "$@" > "build/scr/$RUN.log" 2>&1
echo "exit $?"; tail -5 "build/scr/$RUN.log"
