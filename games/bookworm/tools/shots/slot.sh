#!/bin/bash
# slot.sh <command...>: waits (up to an hour) until fewer than 3 game processes run on the machine
# (tools/gamecount.py, CLAUDE.md), then runs the command. Several of this worktree's jobs may wait at once: a lock
# folder (build/.slotlock) lets one at a time check the count and start, and is let go once its game has had 20
# seconds to appear (or the command has ended), so two never take the same slot.
cd "$(git rev-parse --show-toplevel)" || exit 1
LOCK=build/.slotlock
n=0
while true; do
	if mkdir "$LOCK" 2> /dev/null; then
		if python tools/gamecount.py --limit 6 > /dev/null 2>&1; then
			break
		fi
		rmdir "$LOCK"
	fi
	sleep 15; n=$((n+1)); [ $n -gt 240 ] && { echo "slot.sh: no free slot in an hour"; exit 2; }
done
"$@" &
PID=$!
for i in $(seq 20); do
	kill -0 $PID 2> /dev/null || break
	sleep 1
done
rmdir "$LOCK"
wait $PID
