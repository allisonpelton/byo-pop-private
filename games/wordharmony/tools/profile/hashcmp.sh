#!/bin/bash
# hashcmp.sh <old exe> <new exe> <out dir>: whether two builds of the port draw every frame alike, not only the frames a tour
# photographs (pixcmp.sh): each build plays the phase-two tours (TOURS="tour1 tour2 tour3 tour5" by default) at -fast with seed 1,
# under rundesk, the two side by side, and writes a hash of its screen picture after every frame (-framehash=<file>, a test option:
# the frame's number and a 64-bit hash of the 800x600 pixels). The last line says "frames identical: N" or names the tours whose hash
# files, state logs or sound logs differ. The files stay in <out dir>/{old,new}/<tour>: delete the folder when the result is judged.
source "$(dirname "$0")/../identical/common.sh"
OLD=$1; NEW=$2; B=$3; TOURS="${TOURS:-tour1 tour2 tour3 tour5}"
run_side() {  # <side> <exe>
	local s=$1 exe=$2 n d ev="SIDE_ENV_$1"
	local envs=${!ev}  # SIDE_ENV_old / SIDE_ENV_new: environment assignments for that side's runs (SIDE_ENV_old=SURFACE_DISPLAY=off)
	local av="SIDE_ARGS_$1"; local args=${!av}  # SIDE_ARGS_old / SIDE_ARGS_new: more arguments for that side (SIDE_ARGS_old=-windowscale=1)
	for n in $TOURS; do
		d="$B/$s/$n"; rm -rf "$d"; mkdir -p "$d/port" "$d/user"
		[ -z "$PORTREG" ] || cp "$PORTREG" "$d/user/registry.ini"
		python games/wordharmony/tools/shots/make.py games/wordharmony/tools/shots/$n.txt "$(cygpath -m "$PWD/$d")" port > "$d/port.txt"
		until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
		timeout 900 env $envs "$RUNDESK" "$(win "$exe")" -gamedir="$(win assets/wordharmony)" -userdir="$(win "$d/user")" -fast -splashhold \
			-seed=1 -script="$(win "$d/port.txt")" -statelog="$(win "$d/state.txt")" -framehash="$(win "$d/hash.txt")" $args > "$d/out.txt" 2>&1
	done
}
run_side old "$OLD" &
run_side new "$NEW" &
wait
frames=0; bad=""
for n in $TOURS; do
	cmp -s "$B/old/$n/hash.txt" "$B/new/$n/hash.txt" || bad="$bad $n/hash.txt"
	cmp -s "$B/old/$n/state.txt" "$B/new/$n/state.txt" || bad="$bad $n/state.txt"
	frames=$((frames + $(wc -l < "$B/new/$n/hash.txt")))
done
if [ -z "$bad" ]; then echo "frames identical: $frames"; else echo "DIFFERENT:$bad ($frames frames)"; fi
