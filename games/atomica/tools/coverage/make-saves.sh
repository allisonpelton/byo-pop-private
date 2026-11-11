#!/usr/bin/env bash
# make-saves.sh: the saved games that the sv-*.txt scenarios continue (their "# save: <kind>" line), into
# build/coverage/atomica/save/<kind>.dat: the port plays save-base.txt (-script, under rundesk, silent, fast: one game process for a
# few seconds, so wait for room first: python -I tools/gamecount.py --limit 3) and mksave.py crafts each kind from the SAVEGAME.DAT it
# leaves in build/coverage/atomica/save/ud. Run from anywhere; the port must be built (build/native/atomica.exe).
cd "$(dirname "$0")/../../../.." || exit 2
S=build/coverage/atomica/save
mkdir -p "$S"
rm -rf "$S/ud"
python -I - games/atomica/tools/coverage/save-base.txt "$S/base.port.txt" <<'PY' || exit 1
import sys
out, end = [], 0
for l in open(sys.argv[1]):
    w = l.split('#')[0].split()
    if not w:
        continue
    if w[0] == 'end':
        end = int(w[1])
    else:
        out.append('%d %s\n' % (int(w[0]) + 1, ' '.join(w[1:])))     # the port's script runs at the start of an update, the original's between two
open(sys.argv[2], 'w', newline='\n').write(''.join(out) + '%d quit\n' % (end + 1))
PY
bash games/atomica/tools/run_script.sh "$S/base.port.txt" "$S/ud" 300 -seed=1 -drawevery > /dev/null 2>&1
[ -f "$S/ud/SAVEGAME.DAT" ] || { echo "make-saves: the base game left no SAVEGAME.DAT" >&2; exit 1; }
for kind in elements elements-action molecule bonus over magnet; do
	python -I games/atomica/tools/coverage/mksave.py make "$S/ud/SAVEGAME.DAT" "$kind" "$S/$kind.dat" || exit 1
done
echo "make-saves: done"
