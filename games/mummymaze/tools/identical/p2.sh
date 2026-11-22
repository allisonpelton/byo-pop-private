#!/bin/bash
# p2.sh <scenario file> <name> [seed]: phase two's screens. The scenario (see p2.py) is run by the port (-fast, in a fresh user
# folder) and by the original (origrun in Windows Sandbox, the one place it takes shots), the shots land in
# build/p2/<name>/port and build/p2/<name>/orig. The port runs under rundesk, the sandbox does not. PORT_ONLY=1 skips the original.
# SAVES=<folder>: every .sav in it is put into both games' user folders first (the adventures' adv0.sav ...).
#   bash games/mummymaze/tools/identical/p2.sh <scenario> <name>
SCN="$1"; N="$2"; SEED="${3:-21}"
R="$(pwd -W)"; T="$R/games/mummymaze/tools/identical"; D="$R/build/p2"; mkdir -p "$D/$N/port" "$D/$N/orig"
python -I "$T/p2.py" make "$SCN" "$N" "$D"
U="$D/$N.user"; rm -rf "$U"; mkdir -p "$U"
[ -z "$SAVE" ] || cp "$SAVE" "$U/classic.sav"	# SAVE=<file>: a saved game both start from
[ -z "$SAVES" ] || cp "$SAVES"/*.sav "$U/"
grep -q " quit" "$D/$N.port.txt" || { echo "scenario has no quit" >&2; exit 2; }
if [ -z "$ORIG_ONLY" ]; then
	"$R/build/native/rundesk.exe" "$R/build/native/mummymaze.exe" -userdir="$U" -fast -seed="$SEED" -script="$D/$N.port.txt" > "$D/$N.port.log" 2>&1
	echo "port: $(ls "$D/$N/port" | wc -l) shots"
fi
if [ -z "$PORT_ONLY" ]; then
	LAST=$(grep -o "^at [0-9]*" "$D/$N.orig.txt" | tail -1 | cut -d" " -f2)
	TO=${TIMEOUT:-$(( LAST / 30 + 120 ))}	# the sandbox gets about 40 updates a second (0.4x); the host's reader runs at 19x
	rm -rf "$R/build/origrun/p2$N"
	if [ -n "$SAVE" ] || [ -n "$SAVES" ]; then	# the original's copy is made first and gets the file
		"$R/build/origrun-bin/origrun.exe" --game mummymaze --run "p2$N" --copy-only > "$D/$N.stage.log" 2>&1
		mkdir -p "$R/build/origrun/p2$N/game/data/saves"
		[ -z "$SAVE" ] || cp "$SAVE" "$R/build/origrun/p2$N/game/data/saves/classic.sav"
		[ -z "$SAVES" ] || cp "$SAVES"/*.sav "$R/build/origrun/p2$N/game/data/saves/"
	fi
	SBX=--sandbox; [ -z "$HOST" ] || SBX=""	# HOST=1: on the host, shots read by the in-game frame reader (no sandbox, no queue)
# the sandbox is one for the whole machine and its queue is long: a run that waited out its limit starts again
	for TRY in 1 2 3 4 5 6 7 8 9 10 11 12; do
		"$R/build/origrun-bin/origrun.exe" --game mummymaze --run "p2$N" $SBX --script "$D/$N.orig.txt" --clock virtual \
			--cursor scripted --seed "$SEED" --timeout "$TO" > "$D/$N.orig.log" 2>&1
		grep -q "the lock was not free" "$D/$N.orig.log" || break
		echo "try $TRY: the sandbox lock was not free; again"
	done
	tail -3 "$D/$N.orig.log"
	cp "$R/build/origrun/p2$N/shots/"*.png "$D/$N/orig/" 2>/dev/null
	echo "orig: $(ls "$D/$N/orig" | wc -l) shots"
fi
