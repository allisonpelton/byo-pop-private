#!/usr/bin/env bash
# The port on the PopCap Game Collection's files, against the same port on the retail copy's (GAME_NOTES.md, "The PopCap Game
# Collection version"). Needs build/native/bookworm.exe and rundesk, assets/bookworm staged from the retail copy, and the web build
# (tools/check.sh build-web:bookworm) for the last step.
#   games/bookworm/tools/collection/run.sh <the Collection's BookwormDeluxeComponent folder>
# Stages the Collection's files into build/bw-coll/assets/bookworm, then: the texts (the log line that says how many were found
# by their content), the four bot games of tools/check.sh's bookworm suite once on each set of files (the state logs must be the
# same, update for update), the tours of games/bookworm/tools/shots once on each set of files (the same inputs at the same updates,
# a fresh profile folder each) and compared by shotdiff (the pictures that differ are the Collection's own art), and the web build
# under Node on the Collection's files (the first tour against the native shots). One game process at a time, invisible and
# silent under rundesk, with -fast.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
SRC="$1"
S=games/bookworm/tools/shots
TOURS="${TOURS:-menus classic action}"
OUTD=build/bw-coll
RUNDESK="$R/build/native/rundesk.exe"
EXE="$R/build/native/bookworm.exe"
RETAIL="$R/assets/bookworm"
COLL="$R/$OUTD/assets/bookworm"
python -I games/bookworm/tools/stage_bookworm_assets.py "$SRC" --out "$OUTD/assets/bookworm" | tail -1 || exit 1

# a bot game with no mods, its state log in <name>/state.txt (tools/check.sh, bookworm_state_game)
state_game() {  # <name> <game dir> <user dir> <port arguments...>
	local d="$OUTD/state/$1" g="$2" u="$3"; shift 3
	rm -rf "$d"; mkdir -p "$d/mods"
	[ -d "$u" ] || mkdir -p "$u"
	timeout 900 "$RUNDESK" "$EXE" -gamedir="$g" -userdir="$u" -modsdir="$d/mods" -fast -statelog="$d/state.txt" "$@" \
		> "$d/stdout.txt" 2> "$d/stderr.txt"
	echo "exit $? $(wc -l < "$d/state.txt") state lines"
}

echo "--- the texts and the bot games on the Collection's files"
for k in retail coll; do
	g="$RETAIL"; [ $k = coll ] && g="$COLL"
	rm -rf "$OUTD/state/$k-user"
	state_game "$k-c2" "$g" "$R/$OUTD/state/$k-user/c2" -seed=7 -autoplay=2 -autoplaymode=both -autoplaycap=60000 | sed "s/^/$k c2: /"
	state_game "$k-m1" "$g" "$R/$OUTD/state/$k-user/m1" -seed=9 -autoplay=7 -autoplaymode=classic,action,classic | sed "s/^/$k m1: /"
	state_game "$k-s1" "$g" "$R/$OUTD/state/$k-user/s1" -seed=11 -autoplay=3 -autoplaymode=classic -autoplaycap=3000 | sed "s/^/$k s1: /"
	state_game "$k-s1b" "$g" "$R/$OUTD/state/$k-user/s1" -seed=11 -autoplay=4 -autoplaymode=classic -autoplaycontinue=1 | sed "s/^/$k s1b: /"
done
for k in c2 m1 s1 s1b; do
	if cmp -s "$OUTD/state/retail-$k/state.txt" "$OUTD/state/coll-$k/state.txt"; then echo "state $k: same"; else echo "state $k: DIFFERS"; fi
done
echo "--- GameText lines (retail, then Collection)"
grep -h "GameText" "$OUTD/state/retail-c2/stderr.txt" "$OUTD/state/retail-c2/stdout.txt" | head -3
grep -h "GameText" "$OUTD/state/coll-c2/stderr.txt" "$OUTD/state/coll-c2/stdout.txt" | head -3

echo "--- the tours on both sets of files"
for t in $TOURS; do
	for k in retail coll; do
		g="$RETAIL"; [ $k = coll ] && g="$COLL"
		D="$OUTD/tour/$k-$t"
		rm -rf "$D"; mkdir -p "$D/port"
		python games/bookworm/tools/shots/make.py "$S/$t.txt" "$D" port > "$D/port.txt"
		BW_EXE=build/native/bookworm.exe PORT_TIMEOUT=600 bash games/bookworm/tools/identical/port.sh "$D" -seed=1 -gamedir="$g" \
			-script="$(cygpath -w "$D/port.txt")" | tail -1 | sed "s/^/$k $t: /"
	done
	# both are the port's own pictures: a game name with no shotdiff configuration, so no crop
	python tools/shotdiff/shotdiff.py --game bookworm-coll --orig "$OUTD/tour/retail-$t/port" --port "$OUTD/tour/coll-$t/port" \
		--run "coll-$t" | tail -1
done
echo "--- the web build under Node on the Collection's files"
NODE="$(ls "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/bin/node.exe "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe 2>/dev/null | head -n 1)"
for t in ${WEBTOURS:-menus}; do
	D="$OUTD/tour/coll-$t"; W="$D/webrun"
	rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$W/user" "$W/mods"
	python games/bookworm/tools/shots/make.py "$S/$t.txt" "$D" web > "$W/script.txt"
	( cd "$W" && "$RUNDESK" "$NODE" "$R/games/bejeweled/tools/verify/wasmrun.js" game "$R/build/web/bookworm.js" \
		-gamedir="$COLL" -userdir=user -modsdir=mods -fast -seed=1 -script=script.txt --timeout 600 > out.txt 2>&1
	  echo "web coll-$t exit=$? shots=$(ls shots | wc -l)" )
	mv "$W/shots" "$D/web"
	echo '{"client": [640, 480]}' > "$D/webshots.json"
	python tools/shotdiff/shotdiff.py --game bookworm-coll --orig "$D/port" --port "$D/web" --run "web-coll-$t" \
		--config "$D/webshots.json" --allow 0 | tail -1
done
echo done
