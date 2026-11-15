#!/bin/bash
# run3.sh <out> <runs> <mode>...: scaletest modes, interleaved runs, 15 s windows (threadprof), a summary line each
cd "$(git rev-parse --show-toplevel)"
ROOT="$(cygpath -m "$PWD")"
OUT="$1"; RUNS="$2"; shift 2
mkdir -p "$OUT"
for i in $(seq 1 "$RUNS"); do
  for m in "$@"; do
    python -I games/mahjongjapan/tools/profile/threadprof.py "$OUT/m$m.r$i.json" 4,19 -- "$ROOT/build/presenttest/scaletest.exe" $m 21 > "$OUT/m$m.r$i.out" 2>&1
  done
done
echo done > "$OUT.done"
