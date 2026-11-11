#!/bin/bash
# gap.sh <scenario> <name> [seed] [timeout s]: a run for what the identical-play runs and the bot's runs miss (docs/COVERAGE.md).
# The scenario is in p2.py's syntax (the port's updates, `shot` lines for the port's pictures). SAVES=<folder> gives both the
# port's user folder and the original's copy the .sav files in it. PORT=1 runs only the port (under rundesk, with its shots in
# build/coverage/mummymaze/gap/<name>/port) to see that the clicks reach what they should; without it the original runs under
# run.sh, with --coverage, and its hits go to build/coverage/mummymaze/hits/<name>.txt. Start it from the worktree.
SCN="$1"; N="$2"; SEED="${3:-21}"; TO="${4:-120}"
R="$(pwd -W)"; T="$R/games/mummymaze/tools"; D="$R/build/coverage/mummymaze/gap"; mkdir -p "$D/$N/port"
python -I "$T/identical/p2.py" make "$SCN" "$N" "$D"
if [ -n "$PORT" ]; then
	until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
	U="$D/$N.user"; rm -rf "$U"; mkdir -p "$U"
	[ -z "$SAVES" ] || cp "$SAVES"/*.sav "$U/"
	"$R/build/native/rundesk.exe" "$R/build/native/mummymaze.exe" -userdir="$U" -fast -seed="$SEED" -script="$D/$N.port.txt" > "$D/$N.port.log" 2>&1
	echo "port: $(ls "$D/$N/port" | wc -l) shots"
else
	bash "$T/coverage/run.sh" "$N" "$D/$N.orig.txt" "$SEED" "$TO"
fi
