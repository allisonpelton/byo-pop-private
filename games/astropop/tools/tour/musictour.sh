#!/bin/bash
# A tour with its music state: the port's run with BYOPOP_SOUNDLOG on (its song starts) and the original's run with watches on the
# application's music fields, then musiccmp.py.
#   games/astropop/tools/tour/musictour.sh <name> <tour file> [musiccmp options]    (env as tour.sh's)
# The watches (the original's AstroPopApp, global at 0x5dc220): +0x6dc the song position the game chose, +0x6d8 the slot that plays,
# +0x634 and +0x638 the two ends of the list of slots that the focus loss paused. Files: build/logs/ap/tour/<name>/port_sound.log,
# orig.log, music.txt.
set -uo pipefail
cd "$(dirname "$0")/../../../.."
N="$1"; TOUR="$2"; shift 2
D="build/logs/ap/tour/$N"
mkdir -p "$D"
export BYOPOP_SOUNDLOG="$(cygpath -w "$PWD/$D/port_sound.log")"
rm -f "$D/port_sound.log"
SKIP_ORIG=1 bash games/astropop/tools/tour/tour.sh "$N" "$TOUR" | tail -1
ORIG_WATCHES='[0x5dc220]+0x6dc [0x5dc220]+0x6d8 [0x5dc220]+0x634 [0x5dc220]+0x638' SKIP_PORT=1 bash games/astropop/tools/tour/tour.sh "$N" "$TOUR" | tail -1
python -I games/astropop/tools/tour/musiccmp.py "$D/orig.log" "$D/port_sound.log" "$@" | tee "$D/music.txt"
