#!/bin/bash
# build.sh: scaletest.exe and presenttest.exe into build/presenttest/ (git-ignored), with the retools' llvm-mingw and SDL2
# (docs/BUILDS.md: E:/retools). run3.sh runs scaletest's modes and run4.sh presenttest's renderer rows with each render driver
# forced, both under threadprof.py (15 s of cycles of every thread); sumscale.py and sumrend.py print the medians.
cd "$(git rev-parse --show-toplevel)"
CC=/e/retools/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang
SDL=/e/retools/SDL2-2.32.10/x86_64-w64-mingw32
mkdir -p build/presenttest
for n in scaletest presenttest modeprobe stretchprobe; do
  "$CC" -O2 -o build/presenttest/$n.exe games/mahjongjapan/tools/display/present/$n.c -I$SDL/include/SDL2 -L$SDL/lib -lSDL2 -lgdi32 -luser32 -Wl,--subsystem,console || exit 1
done
cp "$SDL/bin/SDL2.dll" build/presenttest/
