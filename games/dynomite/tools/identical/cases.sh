#!/bin/bash
# cases.sh [name...]: the identical-play cases (docs/IDENTICAL_PLAY.md), all of them or the ones named. Each case's
# port script is made by mkscript.py from the parts below and played by case.sh on the port and the original; a case
# passes when every watched value changes at the same updates to the same values in both. Run setup.sh once first.
# sv1 also compares the saved games the two programs write (savecmp.py); sv2 and sv3 continue the port's and the
# original's saved game of sv1 in both programs. Prints one line per case (the details are in build/identical/<case>/)
# and exits 1 if any differs.
source "$(dirname "$0")/common.sh"
WANT=("$@")

# name, seed, the mode's values (case.sh), mkscript's rng seed and its parts
CASES=(
	"en1 2 panic 3 title menu:endless shots:80:40 wait:300 text:Zed click:352:369 wait:200 shot:table1 click:400:400 wait:300 shots:80:40 wait:300 text:Amy click:352:369 wait:200 shot:table2 quit"
	"st2 4 stomped 5 title menu:stomped:pack shots:45:40 wait:300 text:Sam click:352:369 wait:200 shot:table1 click:400:400 wait:300 shots:60:40 wait:300 shot:over2 quit"
	"fs1 6 fossil 7 title menu:fossil:strategic shots:55:40 wait:300 text:Fay click:352:369 wait:200 shot:table quit"
	"fp1 8 fossil 9 title menu:fossil:panic shots:60:40 wait:300 text:Pat click:352:369 wait:200 shot:table quit"
	"tt1 10 timetrial 11 title menu:timetrial:normal shots:150:40 wait:300 shot:end quit"
	"sv1 12 panic 13 title menu:endless shots:40:40 wait:200 save:Sv wait:100 shot:menu quit"
	"sv2 12 panic 14 title cont shots:60:40 wait:200 shot:end quit"
	"sv3 12 panic 14 title cont shots:60:40 wait:200 shot:end quit"
)

st=0
for c in "${CASES[@]}"; do
	set -- $c
	N=$1; SEED=$2; MODE=$3; R=$4; shift 4
	if [ ${#WANT[@]} -gt 0 ] && [[ " ${WANT[*]} " != *" $N "* ]]; then continue; fi
	D="$OUT/$N"; mkdir -p "$D"
	python "$TOOLS/mkscript.py" "$D/script.txt" 25000 "$R" "$@"
	SAVE=""
	case $N in
		sv2) SAVE="$OUT/port.SAV" ;;
		sv3) SAVE="$OUT/orig.SAV" ;;
	esac
	if [ -n "$SAVE" ] && [ ! -f "$SAVE" ]; then echo "$N: no $SAVE (run sv1 first)"; st=1; continue; fi
	if SAVE="$SAVE" HS=1 "$TOOLS/case.sh" "$N" "$SEED" "$D/script.txt" "$MODE" > "$D/case.log" 2>&1; then r=same
	else r=DIFFERS; st=1; fi
	echo "$N: $r ($(grep -c ' same$' "$D/case.log") of $(grep -c ' port ' "$D/case.log") values the same)"
	if [ $N = sv1 ]; then
		# the two saved games, byte by byte apart from the picture and the start time
		cp "$D/user/SLOT0.SAV" "$OUT/port.SAV"; cp "$OG_GAME/SLOT0.SAV" "$OUT/orig.SAV"
		if python "$TOOLS/savecmp.py" "$OUT/port.SAV" "$OUT/orig.SAV" > "$D/save.log" 2>&1; then r=same
		else r=DIFFERS; st=1; fi
		echo "sv1 saved games: $r ($(tail -1 "$D/save.log"))"
	fi
done
exit $st
