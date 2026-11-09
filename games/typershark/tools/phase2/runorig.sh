#!/usr/bin/env bash
# Run the original on a phase-2 script inside Windows Sandbox (the only place origrun takes shots; docs/ORIGRUN.md, "Windows Sandbox").
#   games/typershark/tools/phase2/runorig.sh <name> [seed] [timeout s] [extra origrun options...]
# Reads build/ts-p2/<name>/orig.txt (made by p2lib.py), runs it on the copy build/origrun/typershark-p2/game (its own run, `typershark-p2`,
# so the host runs of phase 1 are left alone), from the clean users folder (or $USERS, a folder of build/ts-p1/fixtures; STAGE=<dir>
# copies <dir>/* over the copy's users folder first), the virtual clock, the scripted cursor, the update-timed shots of a SexyApp game of
# this era (its main loop's count of updates since the last draw, app+0x4a0, is both the pending-draw flag and the update step), a
# registry preset file if $REG names one. Copies the shots to build/ts-p2/<name>/orig/ and prints origrun's RESULT lines.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
N="$1"; SEED="${2:-1}"; LIMIT="${3:-240}"; shift 3 2>/dev/null || shift $#
D="$ROOT/build/ts-p2/$N"
RUN="$ROOT/build/origrun/typershark-p2"
bash "$ROOT/games/typershark/tools/phase2/waitroom.sh" || exit 3
rm -rf "$RUN/game/users" "$RUN/shots" "$RUN/logs" "$RUN/sandbox-files"
cp -r "$ROOT/build/ts-p1/fixtures/${USERS:-users-clean}" "$RUN/game/users"
[ -n "${STAGE:-}" ] && cp -r "$STAGE"/. "$RUN/game/users/"
mkdir -p "$D"; rm -rf "$D/orig"
"$ROOT/build/origrun-bin/origrun.exe" --exe "$R/build/origrun/typershark-p2/game/TyperSharkGame.exe" --run typershark-p2 \
	--script "$R/build/ts-p2/$N/orig.txt" --sandbox --sandbox-wait 3000 --clock virtual --cursor scripted --seed "$SEED" \
	--counter "[0x565380]+0x4b8" --pending-draw "[0x565380]+0x4a0" --update-state "[0x565380]+0x4a0" \
	--patches "$R/build/ts-p2/patches.txt" --timeout "$LIMIT" ${REG:+--reg "$REG"} "$@" > "$D/orig.out" 2>&1
echo "origrun exit=$?" >> "$D/orig.out"
mkdir -p "$D/orig"; cp "$RUN/shots/"*.png "$D/orig/" 2>/dev/null; cp "$RUN/shots/landed.tsv" "$D/orig.landed.tsv" 2>/dev/null
cp "$RUN/logs/origrun.log" "$D/orig.log" 2>/dev/null
cp -r "$RUN/game/users" "$D/orig.users" 2>/dev/null
grep -E "RESULT|exit=|WARNING|STOP|refus|cannot" "$D/orig.out" | head -20
echo "shots: $(ls "$D/orig" 2>/dev/null | wc -l)"
