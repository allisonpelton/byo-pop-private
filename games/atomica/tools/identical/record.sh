#!/usr/bin/env bash
# Record a scenario for compare.py from the port's bot: the bot plays a game, every click it makes goes to the file
# (window coordinates), shifted so that the main menu's button is clicked at update 2310, after the title screen's click.
# GAMES=<n> plays a series of n games (the high score entry is typed and its button clicked).
#   games/atomica/tools/identical/record.sh <strategy|action> <bot seed> <cap in updates> <scenario file> [game seed]
# The bot runs as compare.py runs the port (-seed, -drawevery: the original draws every update), invisibly under rundesk.
set -euo pipefail
MODE="$1"; BOT="$2"; CAP="$3"; OUT="$4"; SEED="${5:-1}"
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
TMP="$(mktemp -d)"
EXE="$(cygpath -w "$ROOT/build/native/atomica.exe")"
D="$(cygpath -m "$TMP")"
timeout 600 "$ROOT/build/native/rundesk.exe" "$EXE" -userdir="$D/user" -fast -drawevery -seed="$SEED" -autoplay="$BOT" -autoplaymode="$MODE" \
	-autoplaycap="$CAP" -autoplaygames="${GAMES:-1}" -autoplaylog="$D/log.txt" -autoplayscript="$D/script.txt" > "$TMP/out.txt" 2>&1 || true
grep -E "game 1 (starts|over)|level .* done" "$TMP/log.txt" | head -12
SP="$D/script.txt" OUT="$OUT" MODE="$MODE" python -I - <<'PY'
import os
lines = [l.split() for l in open(os.environ['SP'])]
shift = 2310 - int(lines[0][0])
button = '316 104' if os.environ['MODE'] == 'strategy' else '316 206'
out = ['1500 move 320 445', '1510 click 320 445', '2300 move ' + button]
for l in lines:
    out.append('%d %s' % (int(l[0]) + shift, ' '.join(l[1:])))
last = int(lines[-1][0]) + shift
out.append('end %d' % (last + 100))
open(os.environ['OUT'], 'w', newline='\n').write(
    '# recorded by record.sh from the port\'s bot (' + os.environ['MODE'] + '); the menu button is clicked at update 2310\n' + '\n'.join(out) + '\n')
print('%d clicks, ends at %d' % (len(lines), last + 100))
PY
rm -rf "$TMP"
