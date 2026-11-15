#!/usr/bin/env bash
# The port on the PopCap Game Collection's files, against the same port on the Steam copy's (GAME_NOTES.md, "The PopCap Game
# Collection version"). Needs build/native/bejeweled2.exe and rundesk, assets/bejeweled2 staged from the Steam copy, and the web
# build (tools/check.sh build-web:bejeweled2) for the last step.
#   games/bejeweled2/tools/collection/run.sh <the Collection's Bejeweled2DeluxeComponent folder> [plays|shots|web]...
# Stages the Collection's files into build/b2-coll/assets/bejeweled2 (and a second set, build/b2-coll/cfg/assets/bejeweled2, with
# the Steam copy's properties/config.xml put in: the one file of the Collection's that changes how the game plays), with a copy of
# the exe in a bin folder beside each (the exe finds assets/bejeweled2 one level up), then
#   plays: the nine modes' bot games of tools/check.sh (tools/playbymove, seed 1) on both sets, each against the transcript and
#          end-screen hashes that check.sh holds for the Steam files (BJ2_PLAYS): the set with the Steam config must match them
#          all, and the Collection's own differs where its constants do;
#   shots: the coverage scripts' screens (menu_a, opts_a, ... with their `shot` lines) played once on each set and compared by
#          tools/shotdiff;
#   web:   the first of them under Node on the web build.
# Each step is named on the command line (all three by default). One game process of mine at a time apart from the plays, which
# take two; invisible and silent under rundesk, with -fast.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
W="$(pwd -W)"
SRC="$1"; shift
STEPS="${*:-plays shots web}"
B=build/b2-coll
OUT=build/logs/bej2/coll
EXE="$W/build/native/bejeweled2.exe"
mkdir -p "$B/bin" "$B/cfg/bin" "$OUT"
python -I games/bejeweled2/tools/stage_assets.py "$SRC" --out "$B/assets/bejeweled2" > "$OUT/stage.txt" || exit 1
rm -rf "$B/cfg/assets"; mkdir -p "$B/cfg/assets"; cp -r "$B/assets/bejeweled2" "$B/cfg/assets/bejeweled2"
cp assets/bejeweled2/properties/config.xml "$B/cfg/assets/bejeweled2/properties/config.xml"
cp build/native/bejeweled2.exe build/native/*.dll "$B/bin/"; cp build/native/bejeweled2.exe build/native/*.dll "$B/cfg/bin/"
BJ2_MODES="classic action puzzle endless twilight hyper cognito finity hidden"
steps_of() { case "$1" in classic) echo 1200;; puzzle) echo 500;; cognito|finity|hidden) echo 300;; *) echo 400;; esac; }  # check.sh's BJ2_STEPS

play() {  # <set name> <assets folder> <mode>: prints "<mode> <set> <transcript hash>:<end screen hash>"
	local d="$OUT/$1/$3" t s
	rm -rf "$d"; mkdir -p "$d/user"
	until python -I tools/gamecount.py --limit "${GAMELIMIT:-6}" > /dev/null; do sleep 20; done
	python tools/playbymove/playbymove.py --dir "$W/$d" --steps "$(steps_of "$3")" --first "unlock all" --first "goto $3" --timeout 300 \
		--shot "$W/$d/end.png" "$EXE" "-gamedir=$W/$2" "-userdir=$W/$d/user" -seed=1 > "$d/summary.txt" 2>&1
	t="$(grep -v screenshot "$d/driver.txt" 2>/dev/null | sha256sum | cut -c1-12)"
	s="$(sha256sum "$d/end.png" 2>/dev/null | cut -c1-12)"
	echo "$3 $1 $t:$s"
}

if [[ " $STEPS " == *" plays "* ]]; then
	echo "--- the bot games on the Collection's files, with the Steam config.xml and with its own"
	: > "$OUT/plays.txt"
	for m in $BJ2_MODES; do
		play cfg "$B/cfg/assets/bejeweled2" "$m" >> "$OUT/plays.txt" &
		play own "$B/assets/bejeweled2" "$m" >> "$OUT/plays.txt" &
		wait
	done
	for m in $BJ2_MODES; do
		want="$(grep -o "\[$m\]=\"[0-9a-f:]*\"" tools/check.sh | head -1 | sed 's/.*="\(.*\)"/\1/')"
		cfg="$(grep "^$m cfg " "$OUT/plays.txt" | cut -d' ' -f3)"; own="$(grep "^$m own " "$OUT/plays.txt" | cut -d' ' -f3)"
		echo "$m: steam $want | collection+steam config ${cfg:-none} $([ "$cfg" = "$want" ] && echo SAME || echo DIFFERS) | collection ${own:-none} $([ "$own" = "$want" ] && echo same || echo differs)"
	done
fi
if [[ " $STEPS " == *" shots "* ]]; then
	echo "--- the coverage scripts' screens on the Steam files, the Collection's with the Steam config.xml, and the Collection's own"
	PS=games/bejeweled2/tools/coverage/portshots.sh
	for s in ${SHOTSCRIPTS:-menu_a opts_a users_a title_q game_a}; do
		f=games/bejeweled2/tools/coverage/scripts/$s.txt
		bash $PS "$f" "$OUT/shots/steam-$s" > /dev/null 2>&1
		PORT_EXE="$W/$B/cfg/bin/bejeweled2.exe" REGKEY='SOFTWARE\PopCap\Bejeweled2' bash $PS "$f" "$OUT/shots/cfg-$s" > /dev/null 2>&1
		PORT_EXE="$W/$B/bin/bejeweled2.exe" REGKEY='SOFTWARE\PopCap\Bejeweled2' bash $PS "$f" "$OUT/shots/own-$s" > /dev/null 2>&1
		for v in cfg own; do
			# both are the port's own 640x480 pictures: a game name with no shotdiff configuration, so no crop
			echo "$s $v: $(python tools/shotdiff/shotdiff.py --game bejeweled2-coll --orig "$OUT/shots/steam-$s/cwd/shots" --port "$OUT/shots/$v-$s/cwd/shots" --run "$v-$s" --tolerance "${TOL:-0}" | tail -1)"
		done
	done
fi
if [[ " $STEPS " == *" web "* ]]; then
	echo "--- the web build under Node on the Steam files and on the Collection's own (the shots of the step above are the references)"
	NODE="$(ls "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe 2>/dev/null | tail -n 1)"
	webrun() {  # <folder> <script> <assets folder> <settings section>
		local D="$1"
		rm -rf "$D"; mkdir -p "$D/cwd/shots" "$D/user"
		cp -r build/logs/bej2/profile/users "$D/user/users"
		: > "$D/empty.ri"
		printf '[%s]\nScreenMode=dword:0\nHighResolution=dword:0\n' "$4" > "$D/user/registry.ini"
		until python -I tools/gamecount.py --limit "${GAMELIMIT:-6}" > /dev/null; do sleep 20; done
		( cd "$D/cwd" && "$NODE" "$W/games/bejeweled/tools/verify/wasmrun.js" game --timeout 900 "$W/build/web/bejeweled2.js" \
			-gamedir="$W/$3" -userdir="$W/$D/user" -commands="$W/$OUT/shots/steam-$2/cmd.txt" -realinput="$W/$D/empty.ri" \
			-hold -fast -drawevery -seed=1 > ../run.out 2>&1 )
	}
	for s in ${WEBSCRIPTS:-menu_a opts_a}; do
		webrun "$OUT/webs-$s" "$s" assets/bejeweled2 'SOFTWARE\Steam\Bejeweled2'
		webrun "$OUT/web-$s" "$s" "$B/assets/bejeweled2" 'SOFTWARE\PopCap\Bejeweled2'
		# the web build differs from the native one by a few levels of colour on any files (here the Steam ones), and not at all from itself
		echo "$s web, Steam files, against native: $(python tools/shotdiff/shotdiff.py --game bejeweled2-coll --orig "$OUT/shots/steam-$s/cwd/shots" --port "$OUT/webs-$s/cwd/shots" --run "webs-$s" --tolerance 0 | tail -1)"
		echo "$s web, Collection's files, against native: $(python tools/shotdiff/shotdiff.py --game bejeweled2-coll --orig "$OUT/shots/own-$s/cwd/shots" --port "$OUT/web-$s/cwd/shots" --run "web-$s" --tolerance 0 | tail -1)"
		echo "$s web, Collection's files, against web on the Steam files: $(python tools/shotdiff/shotdiff.py --game bejeweled2-coll --orig "$OUT/webs-$s/cwd/shots" --port "$OUT/web-$s/cwd/shots" --run "webcmp-$s" --tolerance 0 | tail -1)"
	done
fi
echo done
