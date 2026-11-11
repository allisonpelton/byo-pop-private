#!/bin/bash
# Phase two's tour of one whole game: the port's bot plays it (record.py, real input), then tour.sh puts a shot every few updates
# into both sides' scripts and compares them (the original in Windows Sandbox).
#   games/astropop/tools/tour/gamerun.sh <name> <record.py arguments...>
# Env: EVERY (updates between shots, 150), FIRST (first shot, 3200), SKIP_REC=1 (reuse the recording), SKIP_ORIG=1, CHARGE=n and
# PROFILE=file as run_mode.sh has them (CHARGE adds --charge and the original's charge patch list; PROFILE=file.usr gives both
# sides that player, its registry preset going to the original as --reg), AT (more shots: u,u,...).
# Files: build/logs/ap/rec/<name>/ (the recording), build/logs/ap/tour/<name>/ (the tour: tour.sh's).
set -uo pipefail
cd "$(dirname "$0")/../../../.."
N="$1"; shift
R=build/logs/ap/rec
T=games/astropop/tools
mkdir -p "$R"
if [ -z "${SKIP_REC:-}" ]; then
  ARGS=(--play-at 3100 "$@")   # the original has loaded by update ~2900 (a click before that waits for it, and the whole game is late)
  [ -n "${CHARGE:-}" ] && ARGS+=(--charge "$CHARGE")
  [ -n "${PROFILE:-}" ] && ARGS+=(--profile "$PROFILE")
  python -I $T/origcmp/record.py --out "$R/$N" ${PORT_EXE:+--exe "$PORT_EXE"} "${ARGS[@]}" > "$R/$N.log" 2>&1 || { cat "$R/$N.log"; exit 2; }
  tail -1 "$R/$N.log"
fi
LAST=$(python -I -c "import json,sys; print(json.load(open(sys.argv[1]))['last'])" "$R/$N/events.json")
python -I $T/tour/gametour.py "$R/$N/events.json" "$R/$N.tour.txt" --every "${EVERY:-150}" --first "${FIRST:-3200}" --last "$LAST" --at "${AT:-}" || exit 2
if [ -n "${PROFILE:-}" ]; then
  python -I $T/origcmp/ini_to_reg.py "$R/$N/registry-start.ini" "$R/$N/profile.reg" || exit 2
  export ORIGRUN_ARGS="${ORIGRUN_ARGS:-} --reg $PWD/$R/$N/profile.reg"
fi
[ -n "${CHARGE:-}" ] && export ORIGRUN_ARGS="${ORIGRUN_ARGS:-} --patches tools/origrun/patches/astropop-charge.txt"
[ -n "${PROFILE:-}" ] && export PORT_USER="$PWD/$R/$N.user"
SEED=$(python -I - "$@" <<'EOF'
import sys
a = sys.argv[1:]
print(a[a.index('--seed') + 1] if '--seed' in a else '1')
EOF
)
export SEED
bash $T/tour/tour.sh "$N" "$R/$N.tour.txt"
