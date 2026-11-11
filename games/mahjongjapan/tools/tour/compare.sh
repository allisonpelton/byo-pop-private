#!/usr/bin/env bash
# The screen tour (tour.sh) of both Mahjong ports with two builds each, and their screenshots compared byte for byte:
# the exes of a "before" folder (mahjongjapan.exe, mahjongchina.exe; copy them there before a change) against
# build/native's, and the before exe against a second run of itself (a tour that does not repeat proves nothing).
#   build/native/rundesk.exe bash games/mahjongjapan/tools/tour/compare.sh <before folder> <out folder>
# The before folder also needs the DLLs of build/native beside its exes (SDL2.dll and the runtime's).
cd "$(dirname "$0")/../../../.." || exit 1
here=games/mahjongjapan/tools/tour
before="$1"; R="$2"
for g in japan china; do
	n=mahjong$g
	bash $here/tour.sh $g $before/$n.exe $R/$g-before
	bash $here/tour.sh $g build/native/$n.exe $R/$g-after
	bash $here/tour.sh $g $before/$n.exe $R/$g-before2
	same=0; diff=0; missing=0; rr=0
	for f in $R/$g-before/shots/*.png; do
		b=$(basename $f)
		if [ ! -f $R/$g-after/shots/$b ]; then missing=$((missing + 1)); continue; fi
		if cmp -s $f $R/$g-after/shots/$b; then same=$((same + 1)); else diff=$((diff + 1)); echo "DIFF $g $b"; fi
		cmp -s $f $R/$g-before2/shots/$b || rr=$((rr + 1))
	done
	echo "$g: $same shots identical, $diff differ, $missing missing; the before exe against its rerun: $rr differ;" \
		"$(sha256sum $R/$g-after/shots/*.png | cut -c1-12 | sort -u | wc -l) distinct images;" \
		"$(grep -c 'ERROR' $R/$g-after/stdout.txt) ERROR lines, $(grep -c 'script:' $R/$g-after/stderr.txt) script messages"
done
