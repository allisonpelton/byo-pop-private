#!/usr/bin/env bash
# The port on the PopCap Game Collection's files, against the same port on the installer copy's (GAME_NOTES.md, "The PopCap
# Game Collection version"). Needs build/native/bonnies.exe and rundesk, and assets/bonnies staged from the installer copy:
#   build/native/rundesk.exe bash games/bonnies/tools/collection_run.sh <the Collection's BonniesBookstoreDeluxeComponent folder>
# Stages the Collection's files into build/bonnies-coll/assets/bonnies, then runs tools/check.sh's tour
# (games/bonnies/tools/tour_test.txt, -fast -seed=1) once on each set of files, with the same exe, and compares every screen
# and every saved file; the three frozen screens (BB_SHOTS in tools/check.sh) must be the frozen ones on both. One game
# process at a time, invisible and silent under rundesk.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"
SRC="$1"
OUT=build/bonnies-coll/run
EXE=build/native/bonnies.exe
python -I games/bonnies/tools/stage_bonnies_assets.py "$SRC" --out build/bonnies-coll/assets/bonnies > build/bonnies-coll/stage.log 2>&1 \
	|| { echo "staging failed (build/bonnies-coll/stage.log)"; exit 1; }
SHOTS="logo menu name difficulty story board tutorial word played cake tale gameoptions highscores credits help1 help2 help3 mainmenu options action actionword paused resumed timeup endmenu"
FROZEN="$(grep '^BB_SHOTS=' tools/check.sh | sed -E 's/^BB_SHOTS="([^"]*)".*/\1/')"
run() {  # <name> <gamedir>
	local d="$OUT/$1"
	rm -rf "$d"; mkdir -p "$d/user"
	sed "s|@DIR@|$d|" games/bonnies/tools/tour_test.txt > "$d/script.txt"
	timeout 300 "$EXE" -gamedir="$2" -userdir="$d/user" -fast -seed=1 -script="$d/script.txt" > "$d/stdout.txt" 2>&1
	echo "$1: exit $? shots $(ls "$d"/*.png 2>/dev/null | wc -l) loaded $(grep -c 'Resource Loading Time' "$d/stdout.txt") notunderstood $(grep -c 'not understood' "$d/stdout.txt") missing $(grep -ci 'missing\|not found' "$d/stdout.txt")"
	echo "$1: frozen $(for f in played highscores timeup; do sha256sum "$d/$f.png" | cut -c1-12; done | paste -sd' ' -) (expected $FROZEN)"
}
run installer assets/bonnies
run collection build/bonnies-coll/assets/bonnies
n=0
for f in $SHOTS; do cmp -s "$OUT/installer/$f.png" "$OUT/collection/$f.png" || { echo "DIFFERENT SCREEN $f"; n=$((n + 1)); }; done
diff -rq "$OUT/installer/user" "$OUT/collection/user" > /dev/null && echo "saved files: identical" || { echo "SAVED FILES DIFFER"; n=$((n + 1)); }
echo "screens compared: $(echo $SHOTS | wc -w), differing runs: $n"
exit $n
