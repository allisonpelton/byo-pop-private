#!/bin/bash
# runall.sh <list> [first-name...]: the coverage runs of a list, one after the other (run.sh), skipping those whose hits file is there
# already. A list line: <name> <script> [seed] [timeout] [VAR=value ...]  (blank lines and # lines are skipped; the VAR=value words
# are exported for that run: PROFILE=..., HIRES=1, CLOCK=real, EXTRA="...", KEEP_STATE=1).
cd "$(dirname "$0")/../../../.." || exit 2
LIST=$1; shift
H=build/coverage/bejeweled2/hits
while read -r name script seed timeout rest; do
	[ -z "$name" ] && continue
	case "$name" in \#*) continue;; esac
	if [ $# -gt 0 ]; then
		case " $* " in *" $name "*) ;; *) continue;; esac
	fi
	[ -f "$H/$name.txt" ] && { echo "$name: done already"; continue; }
	(
		for kv in $rest; do export "$kv"; done
		bash games/bejeweled2/tools/coverage/run.sh "$name" "$script" "${seed:-1}" "${timeout:-300}"
	)
done < "$LIST"
echo "runall: finished"
