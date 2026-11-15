#!/usr/bin/env bash
# games/heavyweapon/tools/fixes_check.sh: the fixes mod's savedBonuses and survivalBest (games/heavyweapon/mods/fixes/
# mod.json; GAME_NOTES.md, "Fixes (the fixes mod)"), under rundesk with -fast -seed=1. The script
# (tools/fixes_test.txt: level 1 won to its results, whose save is savedBonuses', then two survival games, the second's
# clock beating the first's time and its time after the penalty not: survivalBest's) runs four times: as the original
# (-original -nomods), with the mod at its defaults (a copy of mods/fixes in the run's folder as -modsdir), with
# -fix=savedBonuses and with -fix=survivalBest. No state line may differ. The first player's saved score
# (users.dat) must be more than the results' score in the original (the bonuses twice) and that score with
# savedBonuses; the best survival time must be the second game's in the original and the first's with survivalBest; and
# only the second game's game-over box, which shows the best, may look different, with survivalBest. The mod's run must
# give what both of those give. Then the bot (tools/playbymove) fights level 8's worm from its coming (`win`, nine tanks)
# as the original and with -fix=wormHead: the two games must part, as the head destroys a tank the original's lets by.
#
# Prints one line a run and "fixes: ok" or "fixes: FAILED". Logs: build/fixes/heavyweapon/<run>/.
set -u
cd "$(dirname "$0")/../../.." || exit 2
W="$(pwd -W)"
EXE="${HW_EXE:-$W/build/native/heavyweapon.exe}"
D=build/fixes/heavyweapon
fail=0
run() {  # <name> <extra arguments...>
	local n="$1"; shift
	local d="$D/$n"
	rm -rf "$d"; mkdir -p "$d/user" "$d/mods"
	cp -r games/heavyweapon/mods/fixes "$d/mods/fixes"
	sed "s|@DIR@|$W/$d|" games/heavyweapon/tools/fixes_test.txt > "$d/script.txt"
	timeout 300 "$W/build/native/rundesk.exe" "$EXE" -gamedir=assets/heavyweapon -userdir="$W/$d/user" -fast -seed=1 "$@" \
		-script="$W/$d/script.txt" > "$d/stdout.txt" 2>&1
	grep -E '^(OK|ERROR|STATE) ' "$d/stdout.txt" > "$d/states.txt"
	echo "$n: $(grep -c '^STATE' "$d/states.txt") states, $(ls "$d"/*.png 2>/dev/null | wc -l) screens, $(grep -c '^ERROR' "$d/states.txt") errors, player $(player "$d")"
}
player() {  # <run folder>: the first player's saved score and best survival time (users.dat: a byte, then 0x60-byte records)
	python -c "
import struct, sys
b = open(sys.argv[1] + '/user/users.dat', 'rb').read()
best, score = struct.unpack_from('<ii', b, 1 + 0x20)
print('score %d best %d' % (score, best))" "$1" 2>/dev/null || echo "none"
}
run off -original -nomods
run mod "-modsdir=$W/$D/mod/mods"
run saved -nomods -fix=savedBonuses
run best -nomods -fix=survivalBest
for r in mod saved best; do
	cmp -s $D/off/states.txt $D/$r/states.txt || { echo "the state lines of $r differ from the original's"; fail=1; }
done
grep -q '^ERROR' $D/*/states.txt && { echo "a test command failed"; fail=1; }
for f in $D/off/*.png; do
	n="$(basename "$f")"
	if [ "$n" = survival2.png ]; then
		cmp -s "$f" "$D/best/$n" && { echo "$n: the same with survivalBest on"; fail=1; }
		cmp -s "$f" "$D/saved/$n" || { echo "$n: differs with savedBonuses only"; fail=1; }
	else
		for r in saved best; do cmp -s "$f" "$D/$r/$n" || { echo "$n: differs with $r on"; fail=1; }; done
	fi
	cmp -s "$D/mod/$n" "$D/best/$n" || { echo "$n: the mod's run and -fix=survivalBest differ"; fail=1; }
done
# the score at the results (the state line after level 1's win), and the two survival games' times
results="$(grep -m1 '"screen":"results"' $D/off/states.txt | sed 's/.*"score":\([0-9]*\).*/\1/')"
read -r _ s_off _ b_off <<< "$(player $D/off)"
read -r _ s_saved _ b_saved <<< "$(player $D/saved)"
read -r _ s_best _ b_best <<< "$(player $D/best)"
read -r _ s_mod _ b_mod <<< "$(player $D/mod)"
echo "results score $results; saved score: original $s_off, savedBonuses $s_saved; best time: original $b_off, survivalBest $b_best"
[ -n "$results" ] && [ "$s_saved" = "$results" ] && [ "$s_off" -gt "$results" ] || { echo "savedBonuses: the saved scores are not as they should be"; fail=1; }
[ "$b_best" -gt "$b_off" ] && [ "$b_saved" = "$b_off" ] || { echo "survivalBest: the best times are not as they should be"; fail=1; }
[ "$s_mod" = "$s_saved" ] && [ "$b_mod" = "$b_best" ] || { echo "the mod's run is not both fixes'"; fail=1; }
worm() {  # <name> <extra arguments...>: the bot against level 8's worm
	local n="$1"; shift
	local d="$D/$n"
	rm -rf "$d"; mkdir -p "$d"
	timeout 900 "$W/build/native/rundesk.exe" python tools/playbymove/playbymove.py --dir "$d" --steps 3000 --nulls 60 \
		--timeout 120 --first "click game 320 240" --first "wait 2" --first "click game 320 240" --first "wait 10" \
		--first "unlock all" --first "level 8" --first "set lives 9" --first "win" \
		"$EXE" -seed=1 -userdir="$W/$d/user" "$@" > "$d/out.txt" 2>&1
	echo "$n: $(tail -n 1 "$d/out.txt" | tr -d '\r' | cut -c1-200)"
}
worm worm_off -original
worm worm_fix -fix=wormHead
[ -s $D/worm_off/driver.txt ] && ! cmp -s $D/worm_off/driver.txt $D/worm_fix/driver.txt ||
	{ echo "wormHead: the worm fights did not part"; fail=1; }
[ "$fail" = 0 ] && echo "fixes: ok" || echo "fixes: FAILED"
exit $fail
