#!/bin/bash
# shots.sh <case dir> <run id>: screenshots of the original (inside Windows Sandbox) and of the port at the same updates,
# for tools/shotdiff. The case (case.sh) gives the script the port's bot played and its offset (DELTA, 2500). The
# original takes its shots at the updates listed below plus DELTA and says in shots/landed.tsv at which update each one
# landed ("or-earlier": at it or a few updates before); the port is then run on the same script and takes each shot at
# the update the original's landed at. Compare with:
#   python tools/shotdiff/shotdiff.py --game pixelus --orig build/origrun/<run id>/shots --port <case dir>/shots-port --run <run id>
source "$(dirname "$0")/common.sh"
D="$1"; RUN="${2:-pixsb}"; DELTA="${DELTA:-2500}"
POINTS="${POINTS:-1200 1300 2600 2700 2850 2900 2980 3050 3100 3150 3350 3600 4000}"
python "$TOOLS/toorig.py" "$D/script.txt" "$DELTA" > "$D/orig-shots.txt"
for k in $POINTS; do echo "at $((k + DELTA)) shot u$k" >> "$D/orig-shots.txt"; done
# origrun runs the lines in order, so they must be in the order of their updates
sort -n -s -k2,2 "$D/orig-shots.txt" -o "$D/orig-shots.txt"
# a sandbox run wants the game inside its run folder
R="$ROOT/build/origrun/$RUN"
rm -rf "$R"; mkdir -p "$R"; cp -r "$OG_GAME" "$R/game"
rm -rf "$R/game/users"; cp -r "$OG_STAGE/levels.orig/." "$R/game/levels/"
"$ORIGRUN" --exe "$(win "$R/game/PixelusGame.exe")" --run "$RUN" --sandbox --sandbox-wait "${SBWAIT:-7200}" --script "$(win "$D/orig-shots.txt")" \
	--clock virtual --cursor scripted --counter 0x543270 --update-ms 10 --seed "${SEED:-1}" \
	--patches tools/origrun/patches/pixelus.txt --timeout "${TIMEOUT:-200}" > "$D/orig-shots.log" 2>&1
S="$R/shots"
[ -s "$S/landed.tsv" ] || { echo "no shots landed (see $D/orig-shots.log)"; exit 1; }
cat "$S/landed.tsv"
# the port: the same input, a shot at each update the original's landed at, then quit (a run is always bounded)
cp "$D/script.txt" "$D/script-shots.txt"
rm -rf "$D/shots-port"; mkdir -p "$D/shots-port"
LASTU=0
while IFS=$'\t' read -r name upd how; do
	echo "$((upd - DELTA)) screenshot $(cygpath -m "$D/shots-port/$name.png")" >> "$D/script-shots.txt"
	LASTU=$((upd - DELTA))
done < "$S/landed.tsv"
echo "$((LASTU + 5)) quit" >> "$D/script-shots.txt"
sort -n -s -k1,1 "$D/script-shots.txt" -o "$D/script-shots.txt"
rm -rf "$D/portshots/user"; mkdir -p "$D/portshots"
timeout 300 "$RUNDESK" "$(win build/native/pixelus.exe)" -userdir="$(win "$D/portshots/user")" -fast -seed="${SEED:-1}" -script="$(win "$D/script-shots.txt")" > "$D/portshots/out.txt" 2>&1
echo "port exit $?"
ls "$D/shots-port"
