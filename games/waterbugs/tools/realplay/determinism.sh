#!/bin/bash
# Does the same input, at the same updates, play the same game on every run? Plays a short fixed game (a board, three shots
# at fixed updates) in real time N times with the fixes mod off and N times with it on (steadyClock), and compares the
# game's state at several updates (docs/GAME_NOTES.md, "The fixes mod").
#
#   build/native/rundesk.exe bash games/waterbugs/tools/realplay/determinism.sh [runs]
#
# Prints one line per run (a hash of its states) and "same" or "differ" for each mod setting. Run under rundesk.
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd -W)"
EXE="$ROOT/build/native/waterbugs.exe"
RUNS="${1:-3}"
OUT="$ROOT/build/play/determinism"
rm -rf "$OUT"
mkdir -p "$OUT/mods-on" "$OUT/mods-off"
cp -r "$ROOT/games/waterbugs/mods/fixes" "$OUT/mods-on/fixes"
cat > "$OUT/input.txt" <<'EOF'
# update command: the main menu is up at about 270
300 level 1
900 down game 400 450 left
903 up game 400 450 left
1200 move game 216 120
1260 down game 216 120 left
1263 up game 216 120 left
1700 state
2200 move game 408 248
2260 down game 408 248 left
2263 up game 408 248 left
2700 state
3200 move game 120 280
3260 down game 120 280 left
3263 up game 120 280 left
3700 state
4200 state
4201 quit
EOF
for mode in off on; do
  for i in $(seq 1 "$RUNS"); do
    d="$OUT/$mode-$i"
    mkdir -p "$d/user"
    "$EXE" -gamedir="$ROOT/assets/waterbugs" -userdir="$d/user" -modsdir="$OUT/mods-$mode" -realinput="$OUT/input.txt" > "$d/game.log" 2>&1
    python - "$d/game.log" "$mode" "$i" <<'PYEOF'
import hashlib, json, sys
states = []
for line in open(sys.argv[1], encoding='utf-8', errors='replace'):
    if line.startswith('STATE '):
        s = json.loads(line[6:])
        s.pop('frame', None)
        states.append(json.dumps(s, sort_keys=True))
h = hashlib.sha1('\n'.join(states).encode()).hexdigest()[:12]
last = json.loads(states[-1]) if states else {}
print('mods %-3s run %s: %d states, hash %s, score %s, lives %s, percent %s, rand %s' % (sys.argv[2], sys.argv[3], len(states), h, last.get('score'), last.get('lives'), last.get('percent'), last.get('rand')))
open(sys.argv[1] + '.hash', 'w').write(h + chr(10))
PYEOF
  done
  n=$(cat "$OUT/$mode"-*/game.log.hash | sort -u | wc -l)
  if [ "$n" = "1" ]; then echo "mods $mode: same on every run"; else echo "mods $mode: $n different games in $RUNS runs"; fi
done
