#!/usr/bin/env bash
# The sandbox queue's self-test, for check.sh's origrun suite: fake waiters (origrun --queue-waiter: no sandbox, no
# game, no window) take tickets in a queue folder of this test's own (--queue-dir), never the machine's, and must get the
# lock in the order they arrived. One holds the lock first; five arrive 300 ms apart while it does; a ticket left by a
# process that no longer runs (written by hand, older than all of them) must be skipped and removed; and one waiter is
# ended with TerminateProcess while it waits, so the runs behind it move up. origrun --queue must list the holder and the
# waiters in order meanwhile. About 8 s.
#   tools/origrun/queuetest.sh <folder with origrun.exe> <work folder>
set -uo pipefail
built="$1"; work="$2"
rm -rf "$work"; mkdir -p "$work/q/sandbox-queue"
[ -x "$built/origrun.exe" ] || { echo "FAIL: no origrun.exe in $built"; exit 1; }
q="$(cygpath -aw "$work/q")"
st=0

# A ticket of a process that no longer runs, from before everyone (a FILETIME of 1; pid 0xFFFFFFF0 is no process)
printf 'pid=4294967280\r\nrun=leftover\r\nsince=2026-01-01 00:00:00\r\ndir=\r\ncreated=0000000000000001\r\nwait=600\r\n' \
	> "$work/q/sandbox-queue/0000000000000001-FFFFFFF0.ticket"

# name,arrive ms,hold ms[,die ms]
specs=("holder,0,4000" "w1,300,300" "w2,600,300" "w3,900,300,1800" "w4,1200,300" "w5,1500,300")
pids=()
for s in "${specs[@]}"; do
	n="${s%%,*}"
	"$built/origrun.exe" --queue-waiter "$s" --queue-dir "$q" > "$work/$n.log" 2>&1 &
	pids+=($!)
done
sleep 2.6   # w3 has ended (1.8 s) and the holder still holds the lock (4 s)
"$built/origrun.exe" --queue --queue-dir "$q" > "$work/queue.txt" 2>&1
for p in "${pids[@]}"; do wait "$p" 2>/dev/null; done

got="$(tr -d '\r' < "$work/q/order.txt" 2>/dev/null | tr '\n' ' ')"
want="holder w1 w2 w4 w5 "
[ "$got" = "$want" ] || { echo "FAIL: the lock went to '$got', expected '$want'"; st=1; }
grep -q "^sandbox lock: held by run holder" "$work/queue.txt" || { echo "FAIL: --queue does not name the holder"; cat "$work/queue.txt"; st=1; }
listed="$(sed -n 's/^  [0-9]*\. run \([^ ]*\) .*/\1/p' "$work/queue.txt" | tr '\n' ' ')"
[ "$listed" = "w1 w2 w4 w5 " ] || { echo "FAIL: --queue lists the waiters as '$listed', expected 'w1 w2 w4 w5 '"; cat "$work/queue.txt"; st=1; }
[ ! -e "$work/q/sandbox-queue/0000000000000001-FFFFFFF0.ticket" ] || { echo "FAIL: the leftover ticket was not removed"; st=1; }
grep -q "removed the ticket of run leftover" "$work"/holder.log "$work"/w*.log || { echo "FAIL: no log says the leftover ticket was removed"; st=1; }
grep -q "position 5 of 5; ahead: holder" "$work/w4.log" || { echo "FAIL: w4's log does not give its first position (5 of 5)"; grep 'position' "$work/w4.log"; st=1; }
{ grep -q "position 6 of 6" "$work/w5.log" && grep -q "position 5 of 5" "$work/w5.log"; } || { echo "FAIL: w5's log does not show it moving up from 6 of 6 to 5 of 5 when w3 ended"; grep 'position' "$work/w5.log"; st=1; }
left="$(ls "$work/q/sandbox-queue" | wc -l)"
[ "$left" -eq 0 ] || { echo "FAIL: $left tickets left in the queue"; ls "$work/q/sandbox-queue"; st=1; }
if [ $st -eq 0 ]; then
	echo "queue: 5 waiters got the lock in arrival order, a dead waiter's and a leftover ticket skipped, --queue in order"
	rm -rf "$work"
fi
exit $st
