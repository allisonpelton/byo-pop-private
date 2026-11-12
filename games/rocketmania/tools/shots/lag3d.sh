#!/bin/bash
# lag3d.sh <tour> [lag3d.py options]: the lag scan of a 3D run (run3d.sh made build/ph2/<tour>/orig3d and vis3d.steps.tsv): the port plays the run's
# steps with a shot at every update of a window after each of the original's shots (lag3d.py), the best frame of each shot is picked, and
# shotdiff compares the picked frames with the original's (run id <tour>3dbest, build/shotreview/rocketmania/<tour>3dbest). The port needs no
# desktop, so this may run at any hour (rundesk, Is3D=1, a copy of the build in build/native3d when there is one).
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift
[ -z "$PORT_EXE" ] && [ -f build/native3d/rocketmania.exe ] && export PORT_EXE=build/native3d/rocketmania.exe
D=build/ph2/$N/lag3d
python games/rocketmania/tools/shots/lag3d.py "$N" prepare "$@" | tail -1
until python tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done
RM_REG="Is3D=1,ScreenMode=0,HintsDisabled=0${RM_EXTRA:+,$RM_EXTRA}" timeout 1800 "$TOOLS/port.sh" "$D" -seed=${GS:-1} -script="$(win "$D/port3d.txt")" | tail -1
python games/rocketmania/tools/shots/lag3d.py "$N" compare "$@" > "$D/best.txt"
python tools/shotdiff/shotdiff.py --game rocketmania --orig "build/ph2/$N/orig3d" --port "$D/best" --run ${N}3dbest | tail -1
rm -rf "${D:?}/port"   # the window's frames (about fourteen to a shot) are not kept; best/ is
