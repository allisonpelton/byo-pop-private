#!/usr/bin/env bash
# games/venice/tools/fixes_check.sh: the fixes mod's two toggles, "typos" and "trickMessages" (games/venice/mods/fixes/mod.json;
# GAME_NOTES.md, "Fixes"). The port runs the same script (tools/fixes_test.txt: a new player's name, the name box's request on an
# invalid name, the title of a trick shot level, the summaries after six trick shot levels' wins and losses, then a level and
# the main menu that show nothing of either) four times under rundesk: as the original (-original: no option on), with the mod
# (-modsdir=games/venice/mods: each option at the mod's own default, typos on and trickMessages off), with typos on by the
# command line (-fix=typos), and with both on (-fix=typos -fix=trickMessages). The typo_*.png screens must differ between the
# original's run and the typos' ones, the msg_*.png screens between the original's and the run with both, every other screen
# (and the msg_ ones with typos only) must be equal, no state line may differ, and the mod's run must give what -fix=typos does.
# Prints one line a run and "fixes: ok" or "fixes: FAILED". Logs: build/fixes/venice/<run>/.
set -u
cd "$(dirname "$0")/../../.." || exit 2
W="$(pwd -W)"
EXE="${VENICE_EXE:-$W/build/native/venice.exe}"
fail=0
run() {  # <name> <extra arguments...>
	local n="$1"; shift
	local d="build/fixes/venice/$n"
	rm -rf "$d"; mkdir -p "$d/user"
	sed "s|@DIR@|$W/$d|" games/venice/tools/fixes_test.txt > "$d/script.txt"
	timeout 300 "$W/build/native/rundesk.exe" "$EXE" -gamedir=assets/venice -userdir="$W/$d/user" -fast -seed=1 "$@" \
		-script="$W/$d/script.txt" > "$d/stdout.txt" 2>&1
	grep -E '^(OK|ERROR|STATE) ' "$d/stdout.txt" > "$d/states.txt"
	echo "$n: $(grep -c '^STATE' "$d/states.txt") states, $(ls "$d"/*.png 2>/dev/null | wc -l) screens, $(grep -c '^ERROR' "$d/states.txt") errors"
}
run off -original -nomods
run mod "-modsdir=$W/games/venice/mods"
run typos -nomods -fix=typos
run both -nomods -fix=typos -fix=trickMessages
D=build/fixes/venice
for f in $D/off/*.png; do
	n="$(basename "$f")"
	case "$n" in
	typo_*) cmp -s "$f" "$D/mod/$n" && { echo "$n: the same with typos on"; fail=1; } ;;
	msg_*) cmp -s "$f" "$D/both/$n" && { echo "$n: the same with trickMessages on"; fail=1; }
		cmp -s "$f" "$D/mod/$n" || { echo "$n: differs with typos only"; fail=1; } ;;
	*) cmp -s "$f" "$D/both/$n" || { echo "$n: differs with both on"; fail=1; } ;;
	esac
done
for f in $D/mod/*.png; do
	cmp -s "$f" "$D/typos/$(basename "$f")" || { echo "$(basename "$f"): the mod's run and -fix=typos differ"; fail=1; }
done
for r in mod typos both; do
	cmp -s $D/off/states.txt $D/$r/states.txt || { echo "the state lines of $r differ from the original's"; fail=1; }
done
grep -q '^ERROR' $D/*/states.txt && { echo "a test command failed"; fail=1; }
[ "$fail" = 0 ] && echo "fixes: ok" || echo "fixes: FAILED"
exit $fail
