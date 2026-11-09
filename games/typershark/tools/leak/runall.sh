#!/usr/bin/env bash
# runall.sh <exe dir name under build/ts-leak> <out dir name> [scenarios]: the leak scenarios one after another (each waits for room); docs/LEAKS.md
WW=$(cd "$(dirname "$0")/../../../.." && pwd -W 2>/dev/null || pwd)
HERE=$(cd "$(dirname "$0")" && pwd)
cd "$WW/build/ts-leak"
EXE="$WW/build/ts-leak/$1/typershark.exe"; OUT="$WW/build/ts-leak/$2"
shift 2
SC="${@:-menus route abyss1 tutor15d tutor17}"
for s in $SC; do
  python "$HERE/run.py" "$EXE" "$OUT" $s 1 "$WW/build/ts-leak/scripts/$s.txt"
done
echo runall finished
