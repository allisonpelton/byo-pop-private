#!/bin/bash
# cmp.sh <tour> [shotdiff arguments]: shotdiff of build/ph2/<tour>/orig against .../port, then one line per pair that is
# not a match (boxes: x y w h).
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift
python tools/shotdiff/shotdiff.py --game wordharmony --orig build/ph2/$N/orig --port build/ph2/$N/port --run "$N" "$@" | tail -1
python games/wordharmony/tools/shots/pairs.py "build/shotreview/wordharmony/$N/summary.json"
