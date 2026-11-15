#!/bin/bash
# port.sh <name> <steps> [profile.bwa]: the same steps (run.sh's) played by the port under rundesk with -fast, to check what
# each click does before the original runs them: the board log's screen and dialog lines and the script's own lines
# go to build/coverage/bookwormadv/port/<name>/, and the screens and dialogs it went through are printed. Windowed, as
# the original's runs are (run.sh's backup has ScreenMode 0): in full screen both games update the screens otherwise.
set -u
ROOT="$(git rev-parse --show-toplevel)"; cd "$ROOT" || exit 1
NAME=$1; STEPS=$2; PROFILE=${3:-build/coverage/bookwormadv/backup/users/Tester.bwa}; EXTRA=${4:-}
D=build/coverage/bookwormadv/port/$NAME; rm -rf "$D"; mkdir -p "$D/user/users"
printf '[SOFTWARE\\PopCap\\WinBAD]\nLastUser=sz:Tester\nTotalBooks=dword:3\nScreenMode=dword:0\n' > "$D/user/registry.ini"
cp "$PROFILE" "$D/user/users/Tester.bwa"; [ -n "$EXTRA" ] && cp $EXTRA "$D/user/users/"
python -I games/bookwormadv/tools/pilot/gen.py "$STEPS" "$D/orig.txt" "$D/port.txt" "$(cygpath -m "$PWD/$D")" > /dev/null || exit 1
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 5; done
w() { cygpath -w "$PWD/$1"; }
timeout 300 build/native/rundesk.exe "$(w build/native/bookwormadv.exe)" -userdir="$(w "$D/user")" -fast -seed=12345 \
	-script="$(w "$D/port.txt")" -boardlog="$(w "$D/board.log")" > "$D/log.txt" 2>&1
echo "$NAME: exit $?"
grep -E '^(screen|dialog|talk|treasure|lostturn):' "$D/board.log" | cut -c1-150
grep -iE 'error|assert|fatal' "$D/log.txt" | head -5
