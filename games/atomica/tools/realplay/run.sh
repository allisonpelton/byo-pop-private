#!/usr/bin/env bash
# games/atomica/tools/realplay/run.sh <name> [drive.py arguments]: one real-input run of the native port under rundesk, into
# build/rp/<name>/ (game.log, commands.txt, the run's JSON in result.json). It waits for room first (tools/gamecount.py: the
# machine's limit on game processes across every session), then runs drive.py with the port, the staged game files and a
# fresh user folder, so the tutorial boxes come up. Prints the result line.
set -u
cd "$(dirname "$0")/../../../.." || exit 2
NAME="$1"; shift
W="$(pwd -W)"
OUT="build/rp/$NAME"
rm -rf "$OUT"; mkdir -p "$OUT/user"
until python tools/gamecount.py --limit "${GAMELIMIT:-6}" > /dev/null 2>&1; do sleep 20; done
build/native/rundesk.exe python games/atomica/tools/realplay/drive.py --exe "$W/build/native/atomica.exe" \
	--gamedir "$W/assets/atomica" --userdir "$W/$OUT/user" --out "$W/$OUT" "$@" > "$OUT/result.json" 2> "$OUT/stderr.txt"
echo "exit $?"
tail -n 1 "$OUT/result.json" | cut -c1-2500
