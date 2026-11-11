#!/bin/bash
# pairs.sh <exe A> <exe B> <script> <pairs> [out file]: the processor time of two builds of the port over the same real-time
# scripted run, alternated (A, B, A, B, ...) so that the machine's own changes hit both (cputime.py has the details). From the
# worktree root; waits for room among the 3 game processes before each run; one JSON line per run.
cd "$(dirname "$0")/../../../.."
A="$1"; B="$2"; SCRIPT="$3"; N="$4"; OUT="${5:-/dev/stdout}"
for i in $(seq 1 "$N"); do
  for E in "$A" "$B"; do
    until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 15; done
    build/native/rundesk.exe python -I games/waterbugs/tools/profile/cputime.py "$E" "$SCRIPT" >> "$OUT"
  done
done
