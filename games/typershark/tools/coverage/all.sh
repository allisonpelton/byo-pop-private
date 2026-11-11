#!/usr/bin/env bash
# The coverage check's runs, one after another (two runs of the original would share the copy's folder): phase 2's tours as host runs
# (run.sh), then this folder's own scripts for what they miss. The hits are build/coverage/typershark/hits/<name>.txt.
#   games/typershark/tools/coverage/all.sh [name...]     (no names: all of them)
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
HERE=games/typershark/tools/coverage
T=build/ts-p2
run() { # name script timeout [extra options...]
	local n="$1" s="$2" t="$3"; shift 3
	if [ $# -gt 0 ] || [ -z "${ONLY:-}" ] || [[ " $ONLY " == *" $n "* ]]; then
		bash "$HERE/run.sh" "$n" "$s" "$t" 1 "$@" 2>&1 | tail -1
	fi
}
ONLY="$*"
run m1 $T/m1/orig.txt 300
run s1 $T/s1/orig.txt 600
run u1 $T/u1/orig.txt 300
run h1 $T/h1/orig.txt 600
run st1 $T/st1/orig.txt 600
run idle $T/g_idle/orig.txt 600
run tut18 $T/g_tut18/orig.txt 600
run tut17 $T/g_tutor17/orig.txt 600
run abyss $T/g_abyss/orig.txt 900
run exp $T/g_exp/orig.txt 1500
run bon1 $T/g_bon1/orig.txt 1500
run rt2 $T/g_rt2/orig.txt 2400
run rsa $T/rs_a/orig.txt 600
KEEP=1 run rsb $T/rs_b/orig.txt 600
echo "all done"
