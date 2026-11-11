#!/bin/bash
# One comparison: the port plays a mode by its bot with real input (record.py), the same inputs go to the original in Windows
# Sandbox at the same updates (to_origrun.py, run_original.sh), and the four watches are compared (compare.py).
#   run_mode.sh <mode: classic|survival> <moves> [seed] [outdir]
# Writes <outdir>/ (default build/logs/ap/cmp/<mode>-<moves>-s<seed>): events.json, script.txt, game.log (the port's), orig.log
# (the original's) and result.txt (compare.py's output). Exits 0 when the two sides are identical.
mode=$1; moves=$2; seed=${3:-1}
cd "$(dirname "$0")/../../../.." || exit 2
# PROFILE=file.usr starts both games with that player (file.usr and file.ini, made by record.py --make-profile).
# BEST=n (with PROFILE) makes level n the best of every pilot in that player's registry keys: a classic game then starts at the
# checkpoint after it (4: level 5, 8: level 9, ...), through the box of the two gates.
# PILOT=n picks the pilot (0 Vector, 1 Sprocket); CHARGE=n starts the weapon's charge at n of 18000 in both games (the port's
# `set charge n`, the original's patch list astropop-charge.txt) so that a short game reaches the weapon
sfx=""; [ -n "$PILOT" ] && sfx="$sfx-p$PILOT"; [ -n "$CHARGE" ] && sfx="$sfx-c$CHARGE"; [ -n "$PROFILE" ] && sfx="$sfx-u"; [ -n "$BEST" ] && sfx="$sfx-b$BEST"
d=${4:-build/logs/ap/cmp/$mode-$moves-s$seed$sfx}
RECARGS=""; [ -n "$PILOT" ] && RECARGS="$RECARGS --pilot $PILOT"
[ -n "$BEST" ] && RECARGS="$RECARGS --best $BEST"
[ -n "$PROFILE" ] && RECARGS="$RECARGS --profile $PROFILE"      # PROFILE=file: both start with that profile (all pilots open)
if [ -n "$CHARGE" ]; then RECARGS="$RECARGS --charge $CHARGE"; export ORIGRUN_ARGS="$ORIGRUN_ARGS --patches tools/origrun/patches/astropop-charge.txt"; fi
T=games/astropop/tools/origcmp
python $T/record.py --out "$d" --mode "$mode" --moves "$moves" --seed "$seed" --max-updates "${MAX_UPDATES:-60000}" $RECARGS > "$d.record.txt" 2>&1 || { cat "$d.record.txt"; exit 2; }
python $T/to_origrun.py "$d/events.json" "$d/script.txt" > /dev/null || exit 2
# PROFILE=file.usr: the player is also in the registry (file.ini, kept by record.py --make-profile), which the original gets as a preset
if [ -n "$PROFILE" ]; then
  python $T/ini_to_reg.py "$d/registry-start.ini" "$d/profile.reg" || exit 2
  export ORIGRUN_ARGS="$ORIGRUN_ARGS --reg $d/profile.reg"
fi
$T/run_original.sh "$d/script.txt" "$d/orig.log" "$seed" 900 || exit 2
last=$(python -c "import json,sys; print(json.load(open(sys.argv[1]))['last'])" "$d/events.json")
python $T/compare.py "$d/orig.log" "$d/game.log" --last "$last" > "$d/result.txt" 2>&1
st=$?
echo "$mode $moves moves seed $seed: $(tail -1 $d.record.txt)"; cat "$d/result.txt"
exit $st
