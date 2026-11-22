#!/usr/bin/env bash
# tour-orig.sh <tour> [save]: the original's side of a tour in the sandbox (tour-port.sh writes its script, build/origrun/<tour>.txt, first); shots in build/origrun/<tour>/shots
cd "$(dirname "$0")/../../../.."
N=$1
[ -f games/cosmicbugs/test/tours/$N.orig.txt ] && cp games/cosmicbugs/test/tours/$N.orig.txt build/origrun/$N.txt
[ -n "$PATCHES" ] || PATCHES="$(cygpath -wa tools/origrun/patches/cosmicbugs.txt)"
SAVE=${2:-} bash games/cosmicbugs/tools/play/or.sh $N build/origrun/$N.txt --patches "$PATCHES" --seed 0 --clock virtual --sandbox --sandbox-wait 7200 --surface-screen 708ee0,708edc,800,600,15 --surface-gate 707880 --watch-max 100000
tail -3 build/origrun/$N.out; ls build/origrun/$N/shots | wc -l
