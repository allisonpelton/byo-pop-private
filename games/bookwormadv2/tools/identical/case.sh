#!/bin/bash
# case.sh <name> <port script> [seed]: one comparison of the original and the port on the same input.
#  1. the port plays the script (-fast, its default load lengths) and logs every click the test hooks make (soak, word);
#  2. that input, the script's own clicks and keys go to the
#     original in Windows Sandbox (sbox.sh) with watches on the RNG's index and the battle engine's state;
#  3. calib.py reads the original's load lengths from its log, and the port plays the same input with them;
#  4. rngcmp.py and statecmp.py compare the two timelines: the draws and the engine's states, by update.
# The result is in build/identical/<name>/ (orig.log, state.txt, out.txt) and a last line IDENTICAL or the first difference.
source "$(dirname "$0")/common.sh"
N="$1"; SCRIPT="$2"; SEED="${3:-1}"
D="$OUT/$N"; mkdir -p "$D"
PORTSCRIPT="$D/script.txt"; cp "$SCRIPT" "$PORTSCRIPT"
# PROFILE=<a port user folder> (users/<name>.bwa and the registry.ini that names the player): both programs start from that
# profile, the original in its sandbox, where it keeps its players (C:\ProgramData\WildTangent\BWAVol2\users)
if [ -n "$PROFILE" ]; then
	rm -rf "$D/profile"; mkdir -p "$D/profile"; cp -r "$PROFILE/." "$D/profile/"
	NAME="$(basename "$(ls "$D"/profile/users/*.bwa | head -1)" .bwa)"
	{ cat "$TOOLS/orig.reg"; echo; echo "\"LastUser\"=\"$NAME\""; } > "$D/orig.reg"
	python "$TOOLS/utf16.py" "$D/orig.reg" > "$D/orig16.reg"
	export REGFILE="$D/orig16.reg" PROFILE="$D/profile"
fi
portrun() { if [ -n "$PROFILE" ]; then rm -rf "$1/user"; mkdir -p "$1"; cp -r "$PROFILE" "$1/user"; KEEP_USER=1 bash "$TOOLS/port.sh" "$@"; else bash "$TOOLS/port.sh" "$@"; fi; }
portrun "$D/cal" -seed=$SEED -reseedengine=$SEED -startloadupdates=820 -chapterloadupdates=4107 -script="$PORTSCRIPT" -statelog="$D/cal/state.txt" > /dev/null
# the input of the run: its script's clicks and the ones the hooks made (soak, word), in the port's script form
( grep -E '^[0-9]+ (click|move|rclick|key|text|char|focus)( |$)' "$PORTSCRIPT"
  grep 'script: input' "$D/cal/out.txt" | awk '{print $4, $5, $6, $7}' ) | sort -n -s -k1,1 > "$D/inputs.txt"
# the port replays exactly that input: the soak would choose again, and a timing that differs a little would make it choose
# another word
( cat "$D/inputs.txt"; awk '$2=="quit"' "$PORTSCRIPT" ) | sort -n -s -k1,1 > "$D/replay.txt"
python "$TOOLS/toorig.py" "$D/inputs.txt" -1 > "$D/orig.txt"
END=$(awk '$2=="quit"{print $1}' "$PORTSCRIPT" | tail -1)
bash "$TOOLS/sbox.sh" "$D/orig.txt" $SEED $(( ${END:-30000} / 300 + 60 )) 0x7c0000 '[0x7870a4]+0x30' > "$D/sbox.log" 2>&1
cp build/origrun/bwa2s/logs/origrun-sandbox.log "$D/orig.log"
OPTS=$(python "$TOOLS/calib.py" "$D/orig.log" "$D/cal/state.txt")
echo "load lengths of the original: $OPTS"
portrun "$D/rep" -seed=$SEED -reseedengine=$SEED $OPTS -script="$D/replay.txt" -statelog="$D/state.txt" > /dev/null
cp "$D/rep/out.txt" "$D/out.txt"
python "$TOOLS/rngcmp.py" "$D/orig.log" "$D/state.txt"
python "$TOOLS/statecmp.py" "$D/orig.log" "$D/state.txt"
