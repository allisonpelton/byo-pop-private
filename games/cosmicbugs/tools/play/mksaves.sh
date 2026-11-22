#!/usr/bin/env bash
# mksaves.sh: the two saves the screen tours start from, in build/saves/ (the port's own player files, never committed):
#   unlocked.dat  every board unlocked (the runner's `unlock all`), for mission.txt
#   onelife.dat   that file with board 4 in progress, one life and 15000 points (a profile's fields edited by hand: the
#                 original reads the same format), for end.txt
cd "$(dirname "$0")/../../../.."
mkdir -p build/saves build/cbw/unl
printf '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n' > build/cbw/unl/cp.cfg
rm -f build/cbw/unl/cb.dat
printf '2000 cmd unlock all\n3000 end\n' > build/cbw/unl.txt
CB_555=1 CB_NOWARP=1 build/native/rundesk.exe bash -c "build/native/cosmicbugs_run.exe assets/cosmicbugs build/cbw/unl build/cbw/unl.txt" > /dev/null 2>&1
cp build/cbw/unl/cb.dat build/saves/unlocked.dat
python -I - <<'PY'
import struct
d = bytearray(open('build/saves/unlocked.dat', 'rb').read())
base = 0x24 * 8 + 16                       # the first profile: eight 0x24-byte scores, four ints
struct.pack_into('<i', d, base + 0x20, 4)  # board reached
struct.pack_into('<4i', d, base + 0x28, 4, 1, 15000, 0)   # saved level, lives, score, beams fired
open('build/saves/onelife.dat', 'wb').write(d)
PY
echo "saves in build/saves"
