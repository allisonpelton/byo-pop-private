#!/usr/bin/env bash
# Screenshots of the first walking pair of the action bot (seed 1): the bot's log says "pair joined at update N"; this
# takes shots at N+3, N+6 ... N+18. Run it with two builds (before and after a change to the walk or its drawing) and
# compare the pictures. Invisible and silent (CLAUDE.md): run under rundesk.
#   build/native/rundesk.exe bash games/noahsark/tools/run_walk_shots.sh <out dir> [first update, default 387]
# The animated rain changes every frame, so whole-picture diffs always differ: compare the board region only,
# x 178..628, y 16..466 (the nine by nine field), where nothing but the animals and the flood moves.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
OUT="$1"; N="${2:-387}"
mkdir -p "$OUT"
for i in 1 2 3 4 5 6; do echo "$((N + 3 * i)) screenshot $OUT/walk$i.png"; done > "$OUT/script.txt"
echo "$((N + 25)) quit" >> "$OUT/script.txt"
"$(cygpath -w "$ROOT/build/native/noahsark.exe")" -userdir="$OUT/user" -fast -seed=1 -autoplay=1 -autoplaymode=action \
	-autoplaycap=600 -script="$OUT/script.txt" > "$OUT/run.log" 2>&1
