#!/usr/bin/env bash
# portdbg.sh <scenario> <save kind> <shot updates, space separated>: a scenario of tools/coverage run in the port from a crafted save, with
# screenshots at the updates given (build/coverage/atomica/save/<kind>NNNN.png), to design a scenario before the original runs it.
# The scenario goes in through -realinput (SDL's own queue, as a hand's events go: -script has no double click), under rundesk.
set -u
cd "$(dirname "$0")/../../../.."
SC="$1"; KIND="$2"; shift 2
P="$(pwd -W)"; D=build/coverage/atomica/save/ud-$KIND
rm -rf "$D"; mkdir -p "$D"; cp build/coverage/atomica/save/$KIND.dat "$D/SAVEGAME.DAT"
SHOTS="$*" python -I - "$SC" "$P" "$KIND" <<'PY'
import os, sys
sc, P, kind = sys.argv[1:4]
out, end = [], 0
for l in open(sc):
    w = l.split('#')[0].split()
    if not w:
        continue
    if w[0] == 'end':
        end = int(w[1])
    elif w[1] in ('move', 'click', 'dclick', 'down', 'up'):
        cmd = 'dblclick' if w[1] == 'dclick' else w[1]
        out.append((int(w[0]) + 1, ' '.join([cmd, 'game'] + w[2:])))
    elif w[1] == 'key':      # origrun's key takes a virtual-key code, -realinput's an SDL key name
        out.append((int(w[0]) + 1, 'key ' + {'32': 'Space', '13': 'Return', '27': 'Escape'}.get(w[2], w[2])))
    else:
        out.append((int(w[0]) + 1, ' '.join(w[1:])))
for s in os.environ['SHOTS'].split():
    out.append((int(s), 'screenshot %s/build/coverage/atomica/save/%s%s.png' % (P, kind, s)))
out.sort(key=lambda o: o[0])
open('build/coverage/atomica/save/%s.real' % kind, 'w', newline='\n').write(''.join('%d %s\n' % o for o in out) + '%d quit\n' % (end + 1))
PY
timeout 300 build/native/rundesk.exe "$P/build/native/atomica.exe" -userdir="$P/$D" -fast -seed=1 -drawevery \
	-realinput="$P/build/coverage/atomica/save/$KIND.real" -statetrace="$P/$D/trace.txt" 2>&1 | grep -E 'ERROR|Update Count|realinput:'
