#!/bin/bash
# softcmp.sh <tour> [variant...]: one of the phase-two tours (tools/shots/<tour>.txt) played by the port in each variant, with
# every shot taken at the game's own size (-renderscale=1) and a draw on every update (-fast -drawevery), shots to
# build/ph2s/<tour>/<variant>/port/. The variants (default: all four):
#   base   build/baseline-alchemy/alchemy.exe at 1x: the GPU path as it was before the software screen (3D flags)
#   soft   the build at 1x: the software screen (GAME_NOTES.md, "The software screen at 1x")
#   gpu1   the build at 1x with SURFACE_DISPLAY=off: its GPU path (2D flags, as mDefault3D false gives)
#   gpu2   the build in its default window (2x): the GPU path there (2D flags)
# SAVE=<file> puts that saved game in the user folder (tour 6). Compare two with
#   python -I tools/shotdiff/shotdiff.py --game alchemy --orig build/ph2s/<tour>/base/port --port build/ph2s/<tour>/soft/port --run <id>
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift
VARIANTS="${*:-base soft gpu1 gpu2}"
for v in $VARIANTS; do
	D=build/ph2s/$N/$v
	rm -rf "$D"; mkdir -p "$D/port" "$D/user"
	[ -z "$SAVE" ] || cp "$SAVE" "$D/user/savegame.dat"
	python -I games/alchemy/tools/shots/make.py games/alchemy/tools/shots/$N.txt "$D" port > "$D/port.txt"
	exe=build/native/alchemy.exe; args="-windowscale=1"; env=""
	case $v in
	base) exe=build/baseline-alchemy/alchemy.exe ;;
	soft) ;;
	gpu1) env="SURFACE_DISPLAY=off" ;;
	gpu2) args="" ;;
	esac
	env $env "$RUNDESK" "$(win "$exe")" -userdir="$(win "$D/user")" -fast -drawevery -seed=${GS:-1} -renderscale=1 $args \
		-script="$(win "$D/port.txt")" > "$D/out.txt" 2>&1
	echo "$N $v: exit $? shots $(ls "$D/port" | wc -l)"
done
