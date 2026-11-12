#!/bin/bash
# run3d.sh <tour name> [--3d on|off]: the 3D comparison of one tour (docs/ORIGRUN.md, "Hardware acceleration"). Only between 22:00 and 08:00
# (CLAUDE.md, "Overnight, desktop and sound allowed"): the original runs on the user's visible desktop (visrun3d.py, real clock, posted
# input, PrintWindow shots), the port then plays the steps it landed at (mkport3d.py) under rundesk with Is3D=1 (or 0 with --3d off), and
# shotdiff compares the two (run id <tour>3d, build/shotreview/rocketmania/<tour>3d). GS=<game seed>; RM_EXTRA="Name=value,..." are more registry
# values for both (hf1: HintsDisabledFlag0=1 to HintsDisabledFlag7=1). visrun3d.py waits while another game's window is on the desktop.
source "$(dirname "$0")/../identical/common.sh"
N=$1; MODE=${2:---3d}; ON=${3:-on}
H=$(date +%H)
if [ "$H" -ge 8 ] && [ "$H" -lt 22 ]; then echo "run3d: not between 22:00 and 08:00"; exit 2; fi
SUF=3d; REG=1; [ "$ON" = off ] && { SUF=2d; REG=0; }
D=build/ph2/$N
# a copy of the build (build/native3d: the exe, the DLLs and mods-rocketmania, made by hand before the night) is the port of the run when it
# exists, so that a rebuild during the night leaves the evidence alone
[ -z "$PORT_EXE" ] && [ -f build/native3d/rocketmania.exe ] && export PORT_EXE=build/native3d/rocketmania.exe
mkdir -p "$D"
until python tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done
python games/rocketmania/tools/shots/visrun3d.py "$N" --3d "$ON" --seed ${GS:-1} || exit 1
grep -E "3D flag|refused|not reached|end:|killed" "$D/vis$SUF.log" | head -8
python games/rocketmania/tools/shots/mkport3d.py "$N" --suf "$SUF" --out "$D/$SUF" | tail -1
rm -rf "${D:?}/${SUF:?}/port"; mkdir -p "$D/$SUF/port"
RM_REG="Is3D=$REG,ScreenMode=0,HintsDisabled=0${RM_EXTRA:+,$RM_EXTRA}" timeout 900 "$TOOLS/port.sh" "$D/$SUF" -seed=${GS:-1} -script="$(win "$D/$SUF/port3d.txt")" | tail -1
python tools/shotdiff/shotdiff.py --game rocketmania --orig "$D/orig$SUF" --port "$D/$SUF/port" --run ${N}$SUF | tail -1
python - "$N" "$SUF" <<'PY'
import json, sys
s = json.load(open('build/shotreview/rocketmania/%s%s/summary.json' % (sys.argv[1], sys.argv[2])))
for p in s['pairs']:
    if p['status'] != 'match':
        print(p['name'], p['status'], p.get('shift'), [(b['x'], b['y'], b['w'], b['h']) for b in p.get('boxes', [])][:6])
PY
