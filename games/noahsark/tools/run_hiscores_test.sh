#!/usr/bin/env bash
# Test of the high-score loader (HighScoreWidget::Load, 0x411ea0): a record whose check does not match keeps its name
# and gets the score's remainder mod 256 and the next records are read; a name ends at its first NUL; a file that is cut
# short keeps the records read. Each case is a hiscores.dat made with make_hiscores.py, shown on the High Scores
# dialog (Options, High Scores) and compared by screenshot with the file that holds the values the original ends up with.
# Run on an invisible desktop (CLAUDE.md), which tools/check.sh noahsark does:
#   build/native/rundesk.exe bash games/noahsark/tools/run_hiscores_test.sh <out dir>
# Needs build/native/noahsark.exe and assets/noahsark. About 10 seconds. Prints "ok ..." or "FAIL ..." per check.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT" || exit 2
OUT="${1:-build/logs/noahsark-hiscores}"
EXE="$(cygpath -w "$ROOT/build/native/noahsark.exe")"
MK="python games/noahsark/tools/make_hiscores.py"
rm -rf "$OUT"; mkdir -p "$OUT"
FAILED=0
check() { if [ "$2" -eq 0 ]; then echo "ok   $1"; else echo "FAIL $1"; FAILED=1; fi; }

cat > "$OUT/shot.txt" <<'EOS'
301 click 320 456
360 click 198 188
381 click 320 195
420 click 400 405
700 click 400 240
800 click 70 410
900 click 325 365
1000 screenshot SHOT
1001 quit
EOS
# shot <name> <make_hiscores.py arguments...>: the dialog's screenshot with that file in the user folder
shot() {
	local n="$1"; shift
	mkdir -p "$OUT/user_$n"
	$MK "$OUT/user_$n/hiscores.dat" "$@"
	sed "s|SHOT|$OUT/$n.png|" "$OUT/shot.txt" > "$OUT/$n.txt"
	timeout 120 "$EXE" -userdir="$OUT/user_$n" -fast -seed=5 -script="$OUT/$n.txt" > "$OUT/$n.log" 2>&1
}
same() { cmp -s "$OUT/$1.png" "$OUT/$2.png" && [ -s "$OUT/$1.png" ]; }

# a bad check on record 3 (3333 is 5 mod 256), a NUL in record 4's name
shot tampered Aaa:5000 Bbbb:4000 Cc:3333:bad "Zed\0xx:100" Eee:50
shot expected Aaa:5000 Bbbb:4000 Cc:5 Zed:100 Eee:50
shot control  Aaa:5000 Bbbb:4000 Cc:3333 Zed:100 Eee:50
same tampered expected; check "a bad check: every name kept, that score cut to score % 256, later records read, a name ends at its NUL" $?
! same tampered control; check "the control (a good check on 3333) shows another score" $?

# a file cut inside the fourth record keeps the three before it
shot cut Aaa:5000 Bbbb:4000 Cc:3333 Zed:100 --cut 44
shot cutref Aaa:5000 Bbbb:4000 Cc:3333
same cut cutref; check "a cut file keeps the records read" $?
exit $FAILED
