#!/bin/bash
# final.sh: reruns the port half of every screens scenario with the current build and pools the shots of both sides under
# unique names (<scenario>-u<update>.png) for one shotdiff run, `screens`, which tools/check.sh screens reads. The originals'
# shots are the ones each scenario took earlier (build/origrun/<run>/shots, see SCREENS.md for the scenarios).
#   games/pixelus/tools/screens/final.sh
cd "$(dirname "$0")/../../../.."
ROOT=$PWD
F=build/screens-final
rm -rf $F; mkdir -p $F/orig $F/port
# tag case-dir original-run preset
for c in "A build/sc/A scA -" "B build/sc/B scB P0" "CD build/sc/CD scCD P0" "C3 build/sc/C3 scC3 P0" "E build/sc/Eh scEh2 P2" \
         "H build/sc/H scH -" "I0 build/sc/I0 scI0 P0" "I1 build/sc/I1 scI1 P1"; do
	set -- $c
	until python tools/gamecount.py --limit 6 >/dev/null; do sleep 5; done
	if [ "$4" = "-" ]; then unset PRESET; else export PRESET=$ROOT/build/sc/$4/user; fi
	bash games/pixelus/tools/identical/portshots.sh $2 $3 2500 | tail -1
	for f in build/origrun/$3/shots/u*.png; do n=$(basename $f); cp $f $F/orig/$1-$n; cp $2/shots-port/$n $F/port/$1-$n; done
done
python tools/shotdiff/shotdiff.py --game pixelus --orig $F/orig --port $F/port --run screens | tail -1
