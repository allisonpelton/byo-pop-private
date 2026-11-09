#!/bin/bash
# run.sh <name> [save.dat|-] [timeout s]: one coverage run of the original Bejeweled (docs/COVERAGE.md) on the host, with
# origrun's --coverage on the list that `python -I tools/coverage.py list bejeweled` writes, the script
# build/coverage/bejeweled/scripts/<name>.txt (scripts.py writes them), the virtual clock and the silent stand-in;
# REAL=1 runs on the real clock instead, so the sound code runs.
# The copy is the prepared one (the exe carved out of WinBej.exe, as origcmp/orig.sh --prepare makes it) in
# C:\byo-og\cov-bej-rd\game. On the host the original keeps its saved game in C:\ProgramData\Steam\Bejeweled\userdata:
# the run starts with none, or with the given save there; the caller puts the user's save back afterwards.
# The counter is the int at 0x53ba60, whose top byte is the app's loaded flag (scripts.py waits for it).
# The hits go to build/coverage/bejeweled/hits/<name>.txt, the run's log beside them. It waits for a game slot first.
set -u
cd "$(dirname "$0")/../../../.."
NAME="$1"; SAVE="${2:--}"; T="${3:-300}"
OR="$PWD/build/origrun-bin/origrun.exe"
EXE='C:\byo-og\cov-bej-rd\game\BejeweledGame.exe'
H=build/coverage/bejeweled/hits; mkdir -p "$H"
USERDATA=/c/ProgramData/Steam/Bejeweled/userdata
win() { cygpath -w "$1"; }
while ! python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 20; done
rm -f "$USERDATA/savegame.dat"
[ "$SAVE" != - ] && cp "$SAVE" "$USERDATA/savegame.dat"
CLOCK=virtual; [ -n "${REAL:-}" ] && CLOCK=real
R="build/origrun/cbj-$NAME"
rm -f "$R/coverage.txt"
"$OR" --exe "$EXE" --run "cbj-$NAME" --clock $CLOCK --sound stub --counter 0x53ba60 \
	--coverage "$(win "$PWD/build/coverage/bejeweled/functions.txt")" \
	--script "$(win "$PWD/build/coverage/bejeweled/scripts/$NAME.txt")" --timeout "$T" > "$H/$NAME.log" 2>&1
echo "origrun exit $?" >> "$H/$NAME.log"
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
