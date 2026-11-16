#!/bin/bash
# chapters.sh <profiles dir> <name>...: the comparison case (case.sh) for each saved chapter profile in turn (the profiles
# tools/chapterprofiles.py saved): Adventure, the map's Enter, then the soak plays the chapter. One summary line each goes to
# build/chapter-results.txt (the verdicts of rngcmp.py and statecmp.py), and the case's outputs are deleted once it is judged.
cd "$(dirname "$0")/../../../.." || exit 1
P="$(cd "$1" && pwd)"; shift
# Adventure at 5000 (the original's startup load ran past 3000 once, 2026-11-15, and its click was lost), the map at 6500.
# the profiles as the game leaves them once its unlock panel has been shown (clearneeds.py), in a copy of their own
# one chapter's case (case.sh), in the origrun run folder bwa2s-<worker>
chapter() {
	n="$1"; export ORIG_RUN="bwa2s-$2"
	cf="build/chapter-$n.txt"
	printf '# profile %s: Adventure, the map'"'"'s Enter, then the soak plays the chapter to kill fast (fastkill)\n# until the map shows again (chapterend), at most 15000 updates\n5000 click 245 325\n6500 click 400 575\n8500 fastkill 1\n8500 chapterend\n8500 soak 23500\n23500 quit\n' "$n" > "$cf"
	rm -rf "build/chapter-profile-$n"; cp -r "$P/$n" "build/chapter-profile-$n"; python games/bookwormadv2/tools/identical/clearneeds.py "build/chapter-profile-$n"
	# origrun gives up on the sandbox lock after 600 s, which two other cases' originals can outlast: then the case runs again
	for try in 1 2 3; do
		PROFILE="$PWD/build/chapter-profile-$n" CALLOAD=1 bash games/bookwormadv2/tools/identical/case.sh "chapter-$n" "$PWD/$cf" > "build/chapter-$n.log" 2>&1
		grep -q "lock was not free" "build/identical/chapter-$n/sbox.log" 2>/dev/null || break
	done
	# the verdict lines (rngcmp's header and its last difference, the engine states) and how many differences were listed
	{ echo "== $n"; grep -E "^original:|^draws:|IDENTICAL|engine states|DIFFERENT|first difference|^time |chapter ended|ORIGINAL RUN FAILED|BAD INPUT" "build/chapter-$n.log" | cut -c1-200
	  echo "draw differences listed: $(grep -c '^update' "build/chapter-$n.log"); last: $(grep '^update' "build/chapter-$n.log" | tail -1 | cut -c1-120)"; } > "build/chapter-$n.verdict"
	cat "build/chapter-$n.verdict" >> build/chapter-results.txt; rm -f "build/chapter-$n.verdict"  # in one write: cases run at once
	# the case's own log (the verdicts in full, a few KB) stays in build/chapter-logs; its runs are deleted
	mkdir -p build/chapter-logs; mv "build/chapter-$n.log" build/chapter-logs/
	rm -rf "build/chapter-profile-$n" "$cf"
	[ -n "$KEEP" ] || rm -rf "build/identical/chapter-$n"  # KEEP=1 keeps the case's runs, to look into a difference
}
# JOBS cases at once (3 at most, the rule for one script): only one Windows Sandbox runs at a time (origrun waits for its
# lock), so the others' port runs go on meanwhile; each case runs one game process at a time
JOBS="${JOBS:-3}"; [ "$JOBS" -gt 3 ] && JOBS=3
declare -A busy
for n in "$@"; do
	while :; do
		for ((w = 1; w <= JOBS; w++)); do
			if [ -z "${busy[$w]}" ] || ! kill -0 "${busy[$w]}" 2>/dev/null; then break 2; fi
		done
		sleep 5
	done
	until python tools/gamecount.py --limit > /dev/null; do sleep 30; done
	chapter "$n" "$w" & busy[$w]=$!
	sleep 2
done
wait
echo "chapters.sh finished" >> build/chapter-results.txt
