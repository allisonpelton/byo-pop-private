#!/bin/bash
# equiv.sh <old exe> <new exe> <out dir> [mods dir]: the loading change leaves the play the same. The check's three mouse-played
# cases (tools/check.sh, tiptop_state_game) are played by both exes with the fixes mod on (the blocks follow the seed alone), and
# cmp_offset.py compares each pair of state logs apart from the shift the shorter loading puts on the update numbers.
# Run it under rundesk (build/native/rundesk.exe bash equiv.sh ...); one game process at a time.
OLD=$1; NEW=$2; OUT=$3; MODS=${4:-build/tt/mods-on}
cd "$(git rev-parse --show-toplevel)" || exit 1
MODS="$(cd "$MODS" && pwd -W)"
rm -rf "$OUT"; mkdir -p "$OUT"
run() {  # <name> <exe> <mode> <bot seed> <game seed> <games> [arguments]
	local d="$OUT/$1" exe="$2" m="$3" bs="$4" gs="$5" n="$6"; shift 6
	mkdir -p "$d"
	timeout 600 "$exe" -gamedir=assets/tiptop -userdir="$d/user" -modsdir="$MODS" -fast -seed="$gs" -autoplay="$bs" -autoplayui \
		-autoplaymode="$m" -autoplaygames="$n" -autoplaycap=80000 -statelog="$d/state.txt" "$@" > "$d/stdout.txt" 2> "$d/stderr.txt"
}
for a in "reg regular 1 1 2" "str strategy 2 3 1" "save regular 2 2 1 -autoplaysave=30"; do
	read -r k m bs gs n x <<< "$a"
	run "old-$k" "$OLD" "$m" "$bs" "$gs" "$n" $x
	run "new-$k" "$NEW" "$m" "$bs" "$gs" "$n" $x
	echo "$k: $(python games/tiptop/tools/perf/cmp_offset.py "$OUT/old-$k/state.txt" "$OUT/new-$k/state.txt")"
done
