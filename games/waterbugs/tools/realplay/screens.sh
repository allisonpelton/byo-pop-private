#!/bin/bash
# The same whole game, by real input, with the window changed on the way: the port starts in full screen (the original's
# default); Alt+Enter at the main menu (windowed), Alt+Enter twice on a board (windowed, then full screen again), the
# focus lost and regained on a board (the game stops while it is lost), and a minimize and restore. Each plays from the
# fourth board (the `level` test command) with the bot behind `move`, and must end the game exactly as the plain run does:
# the same boards reached, score, game frame and random state (the window and the focus do not change the game, only how
# many updates pass without it moving).
#
#   build/native/rundesk.exe bash games/waterbugs/tools/realplay/screens.sh
#
# Prints one line per run and "same" or "differ" against the plain run; the folders are build/play/screens/<name>/.
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd -W)"
EXE="$ROOT/build/native/waterbugs.exe"
OUT="$ROOT/build/play/screens"
rm -rf "$OUT"
ALT_ENTER=(--first "keydown Left Alt" --first "key Return" --first "keyup Left Alt" --first "wait 30")
run() {
  name="$1"; shift
  d="$OUT/$name"
  mkdir -p "$d/user"
  python "$ROOT/tools/playbymove/playbymove.py" --dir "$d" --steps 30000 --nulls 40 --stuck 200 --timeout 120 "$@" \
    "$EXE" -gamedir="$ROOT/assets/waterbugs" -userdir="$d/user" > "$d/out.txt" 2>&1
  python - "$d" "$name" <<'PYEOF'
import json, re, sys
d, name = sys.argv[1], sys.argv[2]
text = open(d + '/out.txt', encoding='utf-8', errors='replace').read().strip().split('\n')[-1]
m = re.match(r'playbymove waterbugs: (\d+) moves, stop ([^;]*); STATE (.*)', text)
if not m:
    print('%-12s %s' % (name, text[:200]))
    sys.exit()
s = json.loads(m.group(3))
modes = re.findall(r'fullscreen="(\d)"', open(d + '/user/cp.cfg', encoding='utf-8', errors='replace').read()) if __import__('os').path.exists(d + '/user/cp.cfg') else []
key = '%d moves, stop %s, board %d, score %d, frame %d, rand %d' % (int(m.group(1)), m.group(2), s['reached'] + 1, s['score'], s['frame'], s['rand'])
open(d + '/result.txt', 'w').write(key + '\n')
print('%-12s %s (fullscreen setting at the end: %s)' % (name, key, modes[-1] if modes else '?'))
PYEOF
}
# the random numbers the menu's and the board's animations draw while the game waits depend on how long it waited, so each
# run seeds the generator just before the board is made (docs/TEST_COMMANDS.md, `seed`), and the runs that do not change the
# window wait as long as the ones that do
run plain --first "wait 330" --first "seed 7" --first "level 4" --first "wait 200" --first "wait 30" --first "wait 100" --first "wait 30" --first "wait 100"
run windowed --first "wait 300" "${ALT_ENTER[@]}" --first "seed 7" --first "level 4" --first "wait 200" --first "wait 30" --first "wait 100" --first "wait 30" --first "wait 100"
run toggle --first "wait 330" --first "seed 7" --first "level 4" --first "wait 200" "${ALT_ENTER[@]}" --first "wait 100" "${ALT_ENTER[@]}" --first "wait 100"
run focus --first "wait 330" --first "seed 7" --first "level 4" --first "wait 200" --first "focus lost" --first "wait 300" --first "focus gained" --first "wait 100"
run minimize --first "wait 330" --first "seed 7" --first "level 4" --first "wait 200" --first "focus lost" --first "minimize" --first "wait 200" --first "restore" --first "focus gained" --first "wait 100"
# A run that changes the window or the focus on the board is another game, though each ends in a game over: the frames the
# game steps in the update that sees the change, and the one that follows it, are not the 10 a plain update steps (a lost
# focus stops the steps and the next frame goes on with the last frame's time, GameFrame in WBStart.cpp), so the board is
# a few steps off the driver's updates and the bot meets other enemy positions. The windowed run, which changes the display
# at the menu, plays the same game as the plain one.
same() { sed -E 's/^[0-9]+ moves, //; s/, frame [0-9]+//' "$OUT/$1/result.txt" 2>/dev/null; }
ref="$(same plain)"
for n in windowed toggle focus minimize; do
  if [ "$(same $n)" = "$ref" ]; then echo "$n: the same game as plain"; else echo "$n: another game than plain, as expected after a change on the board (it also ended in a game over)"; fi
done
