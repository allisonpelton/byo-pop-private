#!/bin/bash
# webcmp.sh <tour stem>: the web build's shots against the native port's (shotdiff, the port's folder as "orig"); one line per pair that is not identical.
source "$(dirname "$0")/../identical/common.sh"
N=$1
python tools/shotdiff/shotdiff.py --game tiptop --orig build/ph2/$N/port --port build/ph2/$N/web --run web-$N --allow 0 | tail -1
