#!/bin/bash
# chapters.sh <profiles dir> <name>...: the comparison case (case.sh) for each saved chapter profile in turn (the profiles
# tools/chapterprofiles.py saved): Adventure, the map's Enter, then the soak plays the chapter. One summary line each goes to
# build/chapter-results.txt (the verdicts of rngcmp.py and statecmp.py), and the case's outputs are deleted once it is judged.
cd "$(dirname "$0")/../../../.." || exit 1
P="$(cd "$1" && pwd)"; shift
for n in "$@"; do
	until python tools/gamecount.py --limit 6 > /dev/null; do sleep 30; done
	cf="build/chapter-$n.txt"
	printf '# profile %s: Adventure, the map'"'"'s Enter, then the soak plays the chapter\n3000 click 245 325\n4500 click 400 575\n6500 soak 50000\n56600 quit\n' "$n" > "$cf"
	PROFILE="$P/$n" CALLOAD=9000 bash games/bookwormadv2/tools/identical/case.sh "chapter-$n" "$PWD/$cf" > "build/chapter-$n.log" 2>&1
	# the verdict lines (rngcmp's header and its last difference, the engine states) and how many differences were listed
	{ echo "== $n"; grep -E "^original:|IDENTICAL|engine states|DIFFERENT|first difference" "build/chapter-$n.log" | cut -c1-200
	  echo "draw differences listed: $(grep -c '^update' "build/chapter-$n.log"); last: $(grep '^update' "build/chapter-$n.log" | tail -1 | cut -c1-120)"; } >> build/chapter-results.txt
	rm -rf "build/identical/chapter-$n" "build/chapter-$n.log" "$cf"
done
echo "chapters.sh finished" >> build/chapter-results.txt
