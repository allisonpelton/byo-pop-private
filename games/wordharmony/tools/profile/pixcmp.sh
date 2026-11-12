#!/bin/bash
# pixcmp.sh <old exe> <new exe> <out dir>: whether two builds of the port draw every pixel alike. Both play the phase-two tours
# (games/wordharmony/tools/shots: TOURS="tour1 tour2 tour3 tour5" by default, any others named in TOURS; a tour that needs a saved
# game, tour4, needs PORTREG=<registry.ini> as shots/port.sh) at -fast with seed 1, each under rundesk, the two builds side by side,
# and the PNGs they write are compared file by file with cmp. The last line says "pixels identical: N shots" or lists each
# shot that differs. The shots stay in <out dir>/{old,new}/<tour>/port: delete the folder when the result is judged.
source "$(dirname "$0")/../identical/common.sh"
OLD=$1; NEW=$2; B=$3; TOURS="${TOURS:-tour1 tour2 tour3 tour5}"
run_side() {  # <side> <exe>
	local s=$1 exe=$2 n d
	for n in $TOURS; do
		d="$B/$s/$n"; rm -rf "$d"; mkdir -p "$d/port" "$d/user"
		[ -z "$PORTREG" ] || cp "$PORTREG" "$d/user/registry.ini"
		python games/wordharmony/tools/shots/make.py games/wordharmony/tools/shots/$n.txt "$(cygpath -m "$PWD/$d")" port > "$d/port.txt"
		until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
		timeout 600 "$RUNDESK" "$(win "$exe")" -gamedir="$(win assets/wordharmony)" -userdir="$(win "$d/user")" -fast -splashhold \
			-seed=1 -script="$(win "$d/port.txt")" -statelog="$(win "$d/state.txt")" > "$d/out.txt" 2>&1
	done
}
run_side old "$OLD" &
run_side new "$NEW" &
wait
same=0; bad=""
for n in $TOURS; do
	for f in "$B/old/$n/port/"*.png; do
		g="$B/new/$n/port/$(basename "$f")"
		if [ -f "$g" ] && cmp -s "$f" "$g"; then same=$((same + 1)); else bad="$bad $n/$(basename "$f")"; fi
	done
	cmp -s "$B/old/$n/state.txt" "$B/new/$n/state.txt" || bad="$bad $n/state.txt"
done
if [ -z "$bad" ]; then echo "pixels identical: $same shots"; else echo "DIFFERENT:$bad ($same identical)"; fi
