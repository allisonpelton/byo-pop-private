#!/bin/bash
# A tour with its sound: tour.sh's run with the port's BYOPOP_SOUNDLOG on and the original's logs/sound.log kept, then soundcmp.py.
#   games/astropop/tools/tour/soundtour.sh <name> <tour file> [soundcmp --from N]    (env as tour.sh's)
# Files: build/logs/ap/tour/<name>/port_sound.log, orig_sound.log (the original's log is cleared before its run, so it holds that run alone).
set -uo pipefail
cd "$(dirname "$0")/../../../.."
N="$1"; TOUR="$2"; shift 2
D="build/logs/ap/tour/$N"
mkdir -p "$D"
export BYOPOP_SOUNDLOG="$(cygpath -w "$PWD/$D/port_sound.log")"
rm -f build/origrun/apc/logs/sound.log "$D/port_sound.log"
bash games/astropop/tools/tour/tour.sh "$N" "$TOUR" || exit $?
[ -z "${SKIP_ORIG:-}" ] && cp build/origrun/apc/logs/sound.log "$D/orig_sound.log"
python -I games/astropop/tools/tour/soundcmp.py "$D/port_sound.log" "$D/orig_sound.log" "$@" | tee "$D/sound.txt"
