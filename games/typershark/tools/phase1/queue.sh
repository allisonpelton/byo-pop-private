#!/usr/bin/env bash
# Run comparisons one after another (two original runs at once would share the copy's users folder and registry key).
#   games/typershark/tools/phase1/queue.sh <jobs file>
# A job line: <tag> <scenario file name in this folder> <seed> <step> [VAR=value ...]   (extra words are environment variables,
# e.g. HI=34000 TYPIST="every=4;typo=7"; a '#' line is skipped). The verdict line of each job goes to build/ts-p1/queue.log.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HERE="$ROOT/games/typershark/tools/phase1"
cd "$ROOT"
while read -r tag scen seed step rest; do
	[ -z "${tag:-}" ] && continue
	case "$tag" in \#*) continue ;; esac
	( for kv in $rest; do export "$kv"; done
	  python "$HERE/cmp.py" "$HERE/$scen" --seed "$seed" --step "$step" --max 6 --tag "$tag" > "$ROOT/build/ts-p1/$tag.out" 2>&1 )
	echo "$(date +%H:%M:%S) $(tail -1 "$ROOT/build/ts-p1/$tag.out" | cut -c1-300)" >> "$ROOT/build/ts-p1/queue.log"
done < "$1"
echo "queue done" >> "$ROOT/build/ts-p1/queue.log"
