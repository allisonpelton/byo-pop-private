#!/bin/bash
# Feeding Frenzy's check suite (tools/check.sh's suite_feedingfrenzy calls it): <out dir> [exe]
#
# The bot plays by real input through tools/playbymove (-fast, -seed=1): the normal game from launch for NORMAL_STEPS
# moves (the first stage, its end, the map, the second stage), then the timed game from the menu for TIMED_STEPS moves
# (a whole game: the stage's time runs out, the game is over and the name is entered). Each run is made twice, and the
# two transcripts (driver.txt, every line sent and its answer) must be the same. Then every one of the 40 stages is
# loaded and played for a few seconds from a script (a stage of each kind, its start screen taken), the output the same
# in both runs and every stage in play. When the frozen baseline build/baseline-feedingfrenzy/feedingfrenzy.exe is
# there, its runs must give the same transcripts as the port's. Prints one summary line; exits 0 when every check passed.
set -u
OUT="$1"
EXE="${2:-build/native/feedingfrenzy.exe}"
HERE="$(cd "$(dirname "$0")" && pwd)"
NORMAL_STEPS="${FF_NORMAL_STEPS:-1500}"
TIMED_STEPS="${FF_TIMED_STEPS:-1500}"
BASE=build/baseline-feedingfrenzy/feedingfrenzy.exe
STAGES=40
mkdir -p "$OUT"
fail=""

# <run name> <exe> <steps> [first commands...]: one run; prints its summary line's head
play() {
	local name="$1" exe="$2" steps="$3"; shift 3
	local dir="$OUT/$name" firsts=()
	rm -rf "$dir"; mkdir -p "$dir/user"
	for f in "$@"; do firsts+=(--first "$f"); done
	timeout 280 python -I tools/playbymove/playbymove.py --dir "$dir/run" --steps "$steps" "${firsts[@]}" \
		"$exe" -gamedir=assets/feedingfrenzy -userdir="$dir/user" -seed=1 > "$dir/out.txt" 2>&1
	tail -n 1 "$dir/out.txt" | cut -c1-60
}

# <name> <steps> [first commands...]: the run twice on the port, and once on the baseline when there is one
check() {
	local name="$1" steps="$2"; shift 2
	local line
	line="$(play "$name" "$EXE" "$steps" "$@")"
	play "$name-again" "$EXE" "$steps" "$@" > /dev/null
	case "$line" in *"stop limit"*|*"stop over"*) ;; *) fail="$fail $name($line)";; esac
	cmp -s "$OUT/$name/run/driver.txt" "$OUT/$name-again/run/driver.txt" || fail="$fail $name-rerun"
	if [ -x "$BASE" ]; then
		play "$name-base" "$BASE" "$steps" "$@" > /dev/null
		cmp -s "$OUT/$name/run/driver.txt" "$OUT/$name-base/run/driver.txt" || fail="$fail $name-baseline"
	fi
	echo "$name: $line"
}

# <run name> <exe>: the script that loads each stage (level <n>), takes its start screen (a click on Continue) and lets
# the stage play; prints the number of stages that were in play at the end of their time
sweep() {
	local name="$1" exe="$2" dir="$OUT/$1"
	rm -rf "$dir"; mkdir -p "$dir/user"
	python -I - "$dir/script.txt" "$STAGES" <<'EOF'
import sys
lines = []
t = 200
for k in range(1, int(sys.argv[2]) + 1):
    lines.append("%d level %d" % (t, k)); t += 260
    lines.append("%d click 313 425" % t); t += 200
    lines.append("%d state" % t); t += 1
lines.append("%d quit" % t)
open(sys.argv[1], "w").write("\n".join(lines) + "\n")
EOF
	timeout 280 "$exe" -gamedir=assets/feedingfrenzy -userdir="$dir/user" -fast -seed=1 -script="$dir/script.txt" \
		> "$dir/out.txt" 2>&1
	echo "exit $?" >> "$dir/out.txt"
	grep '^STATE' "$dir/out.txt" | grep -c '"phase":"playing"\|"phase":"ending"'
}

check normal "$NORMAL_STEPS" "wait 300"
check timed "$TIMED_STEPS" "wait 300" "goto timed"
played="$(sweep stages "$EXE")"
sweep stages-again "$EXE" > /dev/null
[ "$played" = "$STAGES" ] || fail="$fail stages($played of $STAGES in play)"
grep -q '^exit 0' "$OUT/stages/out.txt" || fail="$fail stages-exit"
cmp -s <(grep -v '^INFO' "$OUT/stages/out.txt") <(grep -v '^INFO' "$OUT/stages-again/out.txt") || fail="$fail stages-rerun"
if [ -x "$BASE" ]; then
	sweep stages-base "$BASE" > /dev/null
	cmp -s <(grep -v '^INFO' "$OUT/stages/out.txt") <(grep -v '^INFO' "$OUT/stages-base/out.txt") || fail="$fail stages-baseline"
fi
echo "stages: $played of $STAGES in play"
[ -x "$BASE" ] && base="baseline identical" || base="no baseline"
if [ -z "$fail" ]; then
	echo "feedingfrenzy: the normal and the timed game played by the bot, all $STAGES stages loaded and played; reruns identical; $base"
	exit 0
fi
echo "feedingfrenzy: FAILED:$fail"
exit 1
