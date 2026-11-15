#!/bin/bash
# run4.sh <out> <runs>: presenttest's renderer rows (1 = the framework's chain at 800x600, 10 = its chain at 1600x1200, 11 = the
# 15% rect uploaded at 1600x1200) with each render driver forced, interleaved runs, 15 s windows (threadprof)
cd "$(git rev-parse --show-toplevel)"
ROOT="$(cygpath -m "$PWD")"
OUT="$1"; RUNS="$2"
mkdir -p "$OUT"
for i in $(seq 1 "$RUNS"); do
  for d in direct3d11 direct3d; do
    for m in 1 10 11; do
      SDL_RENDER_DRIVER=$d python -I games/mahjongjapan/tools/profile/threadprof.py "$OUT/$d.m$m.r$i.json" 4,19 -- "$ROOT/build/presenttest/presenttest.exe" $m 21 > "$OUT/$d.m$m.r$i.out" 2>&1
    done
  done
done
echo done > "$OUT.done"
