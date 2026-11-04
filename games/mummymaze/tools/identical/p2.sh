#!/bin/bash
# p2.sh <scenario file> <name> [seed]: phase two's screens. The scenario (see p2.py) is run by the port (-fast, in a fresh user
# folder) and by the original (origrun in Windows Sandbox, the one place it takes shots), the shots land in
# build/p2/<name>/port and build/p2/<name>/orig. The port runs under rundesk, the sandbox does not. PORT_ONLY=1 skips the original.
#   bash games/mummymaze/tools/identical/p2.sh <scenario> <name>
SCN="$1"; N="$2"; SEED="${3:-21}"
R="$(pwd -W)"; T="$R/games/mummymaze/tools/identical"; D="$R/build/p2"; mkdir -p "$D/$N/port" "$D/$N/orig"
python -I "$T/p2.py" make "$SCN" "$N" "$D"
U="$D/$N.user"; rm -rf "$U"; mkdir -p "$U"
grep -q " quit" "$D/$N.port.txt" || { echo "scenario has no quit" >&2; exit 2; }
"$R/build/native/rundesk.exe" "$R/build/native/mummymaze.exe" -userdir="$U" -fast -seed="$SEED" -script="$D/$N.port.txt" > "$D/$N.port.log" 2>&1
echo "port: $(ls "$D/$N/port" | wc -l) shots"
if [ -z "$PORT_ONLY" ]; then
	LAST=$(grep -o "^at [0-9]*" "$D/$N.orig.txt" | tail -1 | cut -d" " -f2)
	TO=${TIMEOUT:-$(( LAST / 400 + 120 ))}
	rm -rf "$R/build/origrun/p2$N"
	"$R/build/origrun-bin/origrun.exe" --game mummymaze --run "p2$N" --sandbox --script "$D/$N.orig.txt" --clock virtual \
		--cursor scripted --seed "$SEED" --timeout "$TO" > "$D/$N.orig.log" 2>&1
	tail -3 "$D/$N.orig.log"
	cp "$R/build/origrun/p2$N/shots/"*.png "$D/$N/orig/" 2>/dev/null
	echo "orig: $(ls "$D/$N/orig" | wc -l) shots"
fi
