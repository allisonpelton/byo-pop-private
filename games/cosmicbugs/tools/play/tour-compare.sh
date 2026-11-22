#!/usr/bin/env bash
# combine.sh <run id>: every tour's shots under <tour>_<shot>.png in build/shots-all/{orig,port}, then one shotdiff run
cd "$(dirname "$0")/../../../.."
rm -rf build/shots-all; mkdir -p build/shots-all/orig build/shots-all/port
for t in menus end players board mission; do
	for f in build/origrun/$t/shots/*.png; do [ -f "$f" ] && cp "$f" build/shots-all/orig/${t}_$(basename "$f"); done
	for f in build/cbw/$t/*.png; do [ -f "$f" ] && cp "$f" build/shots-all/port/${t}_$(basename "$f"); done
done
python tools/shotdiff/shotdiff.py --game cosmicbugs --orig build/shots-all/orig --port build/shots-all/port --run $1 2>&1 | tail -3
