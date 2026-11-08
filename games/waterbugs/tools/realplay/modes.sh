#!/bin/bash
# A whole game of every mode by real input, natively: from the launch (the logos, the main menu), the Options screen's
# SHOOTER MODE and DIFFICULTY set by real clicks, then the bot behind `move` (tools/playbymove/playbymove.py) plays
# from the main menu's Start through the story screens and boards until the game is over (docs/GAME_NOTES.md, "Drivers").
#
#   build/native/rundesk.exe bash games/waterbugs/tools/realplay/modes.sh [extra playbymove options]
#
# Extra options (for instance --realtime, to play without -fast) go to playbymove as given. One line per mode:
# shooter mode (1 auto, 0 manual), difficulty (-1, 0, 1), the moves played, the boards reached, the score; the run
# folders are build/play/modes/<shooter>-<difficulty>/ (driver.txt holds every line sent and its answer).
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd -W)"
EXE="$ROOT/build/native/waterbugs.exe"
OUT="${MODES_OUT:-$ROOT/build/play/modes}"
rm -rf "$OUT"
for shooter in 1 0; do
  for diff in 0 1 -1; do
    d="$OUT/$shooter-$diff"
    mkdir -p "$d/user"
    first=(--first "wait 300")
    # the main menu's Options button, then the two buttons that cycle their value (difficulty goes 0, 1, -1; the
    # shooter mode toggles from its default, auto), then back to the main menu
    first+=(--first "down game 400 397 left" --first "wait 2" --first "up game 400 397 left" --first "wait 10")
    if [ "$shooter" = "0" ]; then
      first+=(--first "down game 400 317 left" --first "wait 2" --first "up game 400 317 left" --first "wait 10")
    fi
    clicks=0
    [ "$diff" = "1" ] && clicks=1
    [ "$diff" = "-1" ] && clicks=2
    for i in $(seq 1 $clicks); do
      first+=(--first "down game 400 377 left" --first "wait 2" --first "up game 400 377 left" --first "wait 10")
    done
    first+=(--first "down game 400 537 left" --first "wait 2" --first "up game 400 537 left" --first "wait 10")
    python "$ROOT/tools/playbymove/playbymove.py" --dir "$d" --steps 30000 --nulls 40 --stuck 200 --timeout 120 "${first[@]}" "$@" \
      "$EXE" -gamedir="$ROOT/assets/waterbugs" -userdir="$d/user" > "$d/out.txt" 2>&1
    python - "$d/out.txt" "$shooter" "$diff" <<'PYEOF'
import json, re, sys
text = open(sys.argv[1], encoding='utf-8', errors='replace').read().strip().split('\n')[-1]
m = re.match(r'playbymove waterbugs: (\d+) moves, stop ([^;]*); STATE (.*)', text)
if not m:
    print('shooter %s difficulty %s: %s' % (sys.argv[2], sys.argv[3], text[:200]))
else:
    s = json.loads(m.group(3))
    print('shooter %s difficulty %2s: %s moves, stop %s, played to board %d, score %d, lives %d, settings read back shooter %d difficulty %d' % (
        sys.argv[2], sys.argv[3], m.group(1), m.group(2), s['reached'] + 1, s['score'], s['lives'], s['shootermode'], s['difficulty']))
PYEOF
  done
done
