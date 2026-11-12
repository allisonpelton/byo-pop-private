#!/usr/bin/env bash
# The port on the PopCap Game Collection's files, against the same port on the retail copy's (docs/README.md, "The PopCap Game
# Collection version"). Needs build/native/rocketmania.exe, build/baseline-rocketmania/rocketmania.exe, rundesk, assets/rocketmania
# staged from the Steam copy, and for the last step the web build (tools/check.sh build-web:rocketmania).
#   games/rocketmania/tools/collection/run.sh <the Collection's RocketManiaDeluxeComponent folder>
# Stages the Collection's files into build/rm-coll/assets/rocketmania, then: the three mouse-played games of tools/check.sh
# rocketmania on the retail files (the port, and the frozen baseline) and on the Collection's (their state logs must be the same),
# the screen tours of games/rocketmania/tools/shots once on each set of files (-fast, seed 1: the state logs must be the same, the
# pictures are compared by shotdiff) and the web build under Node on the Collection's files (the first tours, against the same run
# on the retail files; the first picture of a tour is the loading screen, which shows the Collection's own loading picture). One game process at a time, invisible and silent under rundesk. TOURS="u2 t1" and WEBTOURS="u2" run only some.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
SRC="$1"
T=games/rocketmania/tools/shots
TOURS="${TOURS:-u2 t1 t3 t4 t5 t6 t7}"
WEBTOURS="${WEBTOURS:-u2 t1}"
OUT=build/rm-coll
COLL="$ROOT/$OUT/assets/rocketmania"
EXE="$ROOT/build/native/rocketmania.exe"
BASE="$ROOT/build/baseline-rocketmania/rocketmania.exe"
waitslot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done; }
mkdir -p "$OUT"
python -I games/rocketmania/tools/stage_rocketmania_assets.py "$SRC" --out "$OUT/assets/rocketmania" || exit 1

state_game() { # <name> <exe> <game files or ""> <mode> <difficulty> <bot seed> <game seed>: a game played by mouse, its state log
	local d="$OUT/games/$1" gd=()
	rm -rf "$d"; mkdir -p "$d/user"
	[ -z "$3" ] || gd=(-gamedir="$(cygpath -w "$3")")
	waitslot
	build/native/rundesk.exe "$(cygpath -w "$2")" -userdir="$(cygpath -w "$d/user")" "${gd[@]}" -fast -drawevery -seed="$7" -uibot="$6" \
		-uistart=300000 -uimodes="$4" -uidiffs="$5" -uigames=1 -uicap=3000 -statelog="$(cygpath -w "$d/state.txt")" \
		-uiscript="$(cygpath -w "$d/script.txt")" > "$d/stdout.txt" 2> "$d/stderr.txt"
}

echo "--- the three mouse-played games: the port on the retail files, on the Collection's, and the frozen baseline on the retail files"
for g in "0 2 2 777" "1 2 1 12345" "2 3 3 4242"; do
	read -r m df bs gs <<< "$g"
	state_game "retail-$m" "$EXE" "" "$m" "$df" "$bs" "$gs"
	state_game "coll-$m" "$EXE" "$COLL" "$m" "$df" "$bs" "$gs"
	state_game "base-$m" "$BASE" "" "$m" "$df" "$bs" "$gs"
	if [ -s "$OUT/games/coll-$m/state.txt" ] && cmp -s "$OUT/games/retail-$m/state.txt" "$OUT/games/coll-$m/state.txt" &&
		cmp -s "$OUT/games/base-$m/state.txt" "$OUT/games/coll-$m/state.txt"; then
		echo "mode $m: state logs the same ($(wc -l < "$OUT/games/coll-$m/state.txt") lines)"
	else
		echo "mode $m: STATE LOGS DIFFER"
	fi
done

echo "--- the tours on each set of files"
for t in $TOURS; do
	rm -rf "$OUT/$t"; mkdir -p "$OUT/$t"
	waitslot
	bash $T/port.sh $t -statelog="$(cygpath -w "$ROOT/$OUT/$t/retail-state.txt")" | tail -n 1
	mv build/ph2/$t/port "$OUT/$t/retail"; cp build/ph2/$t/out.txt "$OUT/$t/retail-out.txt"
	waitslot
	RM_GAMEDIR="$COLL" bash $T/port.sh $t -statelog="$(cygpath -w "$ROOT/$OUT/$t/coll-state.txt")" | tail -n 1
	mv build/ph2/$t/port "$OUT/$t/coll"; cp build/ph2/$t/out.txt "$OUT/$t/coll-out.txt"
	cmp -s "$OUT/$t/retail-state.txt" "$OUT/$t/coll-state.txt" && echo "$t: state logs the same ($(wc -l < "$OUT/$t/coll-state.txt") lines)" || echo "$t: STATE LOGS DIFFER"
	# both sides are the port's own pictures: a game name with no shotdiff configuration, so no crop
	python tools/shotdiff/shotdiff.py --game rocketmania-coll --orig "$OUT/$t/retail" --port "$OUT/$t/coll" --run "coll-$t" | tail -n 1
done

echo "--- the web build under Node on each set of files"
for t in $WEBTOURS; do
	[ -d "$OUT/$t/coll" ] || continue
	waitslot
	bash $T/web.sh $t | tail -n 1
	rm -rf "$OUT/$t/web-retail"; mv build/ph2/$t/web "$OUT/$t/web-retail"
	waitslot
	RM_GAMEDIR="$COLL" bash $T/web.sh $t | tail -n 1
	rm -rf "$OUT/$t/web-coll"; mv build/ph2/$t/web "$OUT/$t/web-coll"
	python tools/shotdiff/shotdiff.py --game rocketmania-coll --orig "$OUT/$t/web-retail" --port "$OUT/$t/web-coll" --run "web-coll-$t" | tail -n 1
done
echo done
