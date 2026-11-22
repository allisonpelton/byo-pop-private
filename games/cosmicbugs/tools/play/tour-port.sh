#!/usr/bin/env bash
# tour-port.sh <tour name> [save]: the runner's side of a tour (CB_555), shots in build/cbw/<name>/
cd "$(dirname "$0")/../../../.."
N=$1; SAVE=${2:-}
python games/cosmicbugs/tools/play/tour.py games/cosmicbugs/test/tours/$N.txt $N || exit 1
rm -rf build/cbw/$N; mkdir -p build/cbw/$N
printf '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n' > build/cbw/$N/cp.cfg
[ -n "$SAVE" ] && cp "$SAVE" build/cbw/$N/cb.dat
CB_555=1 CB_NOWARP=1 build/native/rundesk.exe bash -c "build/native/cosmicbugs_run.exe assets/cosmicbugs build/cbw/$N build/cbw/$N.txt" > build/cbw/$N.out 2>&1
tail -1 build/cbw/$N.out; ls build/cbw/$N/*.png | wc -l
