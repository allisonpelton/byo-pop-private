#!/bin/bash
# case.sh <name> <port script> [seed]: one comparison of the original and the port on the same input.
#  1. the port plays the script (-fast, its default load lengths) and logs every click the test hooks make (soak, word);
#  2. that input, the script's own clicks and keys go to the
#     original in Windows Sandbox (sbox.sh) with watches on the RNG's index and the battle engine's state;
#  3. calib.py reads the original's load lengths from its log, and the port plays the same input with them;
#  4. rngcmp.py and statecmp.py compare the two timelines: the draws and the engine's states, by update.
# The result is in build/identical/<name>/ (orig.log, state.txt, out.txt) and a last line IDENTICAL or the first difference.
source "$(dirname "$0")/common.sh"
# the seconds each stage took, for the batch's timing (chapters.sh collects the "time" lines)
T0=$SECONDS; TL=$SECONDS
stage() { echo "time $1: $((SECONDS - TL)) s (total $((SECONDS - T0)) s)"; TL=$SECONDS; }
N="$1"; SCRIPT="$2"; SEED="${3:-1}"
D="$OUT/$N"; mkdir -p "$D"
PORTSCRIPT="$D/script.txt"; cp "$SCRIPT" "$PORTSCRIPT"
# PROFILE=<a port user folder> (users/<name>.bwa and the registry.ini that names the player): both programs start from that
# profile, the original in its sandbox, where it keeps its players (C:\ProgramData\WildTangent\BWAVol2\users)
if [ -n "$PROFILE" ]; then
	rm -rf "$D/profile"; mkdir -p "$D/profile"; cp -r "$PROFILE/." "$D/profile/"
	# the port windowed: a profile saved full screen draws at the private desktop's size (4K: five times slower; the logic is the same)
	sed -i 's/^ScreenMode=dword:1/ScreenMode=dword:0/' "$D/profile/registry.ini"
	NAME="$(basename "$(ls "$D"/profile/users/*.bwa | head -1)" .bwa)"
	{ cat "$TOOLS/orig.reg"; echo; echo "\"LastUser\"=\"$NAME\""; } > "$D/orig.reg"
	python "$TOOLS/utf16.py" "$D/orig.reg" > "$D/orig16.reg"
	export REGFILE="$D/orig16.reg" PROFILE="$D/profile"
fi
portrun() { if [ -n "$PROFILE" ]; then rm -rf "$1/user"; mkdir -p "$1"; cp -r "$PROFILE" "$1/user"; KEEP_USER=1 bash "$TOOLS/port.sh" "$@"; else bash "$TOOLS/port.sh" "$@"; fi; }
portrun "$D/cal" -seed=$SEED -reseedengine=$SEED -startloadupdates=820 -chapterloadupdates=${CALLOAD:-4107} -script="$PORTSCRIPT" -statelog="$D/cal/state.txt" > /dev/null
stage "port, first run"
# the input of the run: its script's clicks and the ones the hooks made (soak, word), in the port's script form
( grep -E '^[0-9]+ (click|move|rclick|key|text|char|focus)( |$)' "$PORTSCRIPT"
  grep 'script: input' "$D/cal/out.txt" | awk '{print $4, $5, $6, $7}' ) | sort -n -s -k1,1 > "$D/inputs.txt"
# a malformed input line (another output written into the middle of a log line) makes the replay refuse its whole script
if BAD=$(grep -E '^[0-9]+ ' "$D/inputs.txt" | grep -vE '^[0-9]+ (click|move|rclick) -?[0-9]+ -?[0-9]+$|^[0-9]+ (key|text|char|focus)( |$)' | head -3) && [ -n "$BAD" ]; then
	echo "BAD INPUT LINES in the first run's log: $BAD"; exit 3
fi
# the port replays exactly that input: the soak would choose again, and a timing that differs a little would make it choose
# another word
# the end: the script's quit, or, when the script asked for chapterend, the update the first run quit at (300 after the map
# showed again), so that neither program plays on past the chapter
END=$(awk '$2=="quit"{print $1}' "$PORTSCRIPT" | tail -1)
CEND=$(grep -o "script: chapterend [0-9]*" "$D/cal/out.txt" | head -1 | awk '{print $3}')
[ -n "$CEND" ] && END=$((CEND + 300)) && echo "chapter ended at update $CEND"
# the chapter's anchor: where the first run made its battle engine (the chapter load's end; its first "estate" after the
# first "chload 1"). The original's inputs from there on wait for its own engine (toorig.py) and count from its mark, and
# the original quits once the last of them is past (the peek at the end)
ANCHOR=$(awk '$2=="chload" && $3=="1" && !l {l=1} l && $2=="estate" && $3!="-" {print $1; exit}' "$D/cal/state.txt")
if [ -n "$ANCHOR" ]; then
	echo "the first run made its engine at update $ANCHOR"
	{ python "$TOOLS/toorig.py" "$D/inputs.txt" -1 "$ANCHOR" '[0x7870a4]'; echo "at +$((END - ANCHOR)) peek [0x7870a4]"; echo "quit"; } > "$D/orig.txt"
	TIMEOUT=$(( END / 100 + 600 ))
else
	python "$TOOLS/toorig.py" "$D/inputs.txt" -1 > "$D/orig.txt"
	TIMEOUT=$(( ${END:-30000} / 100 + 60 ))
fi
# the original's log is made fresh by this run only: a log left by an earlier case (a sandbox run that never started, because another
# session's sandbox held the lock or the sandbox failed) would be compared as if it were this case's (2026-11-11: the chapter batch
# compared one stale log against every profile)
rm -f build/origrun/bwa2s/logs/origrun-sandbox.log "$D/orig.log"
bash "$TOOLS/sbox.sh" "$D/orig.txt" $SEED $TIMEOUT 0x7c0000 '[0x7870a4]+0x30' > "$D/sbox.log" 2>&1
stage "original in the sandbox"
if ! cp build/origrun/bwa2s/logs/origrun-sandbox.log "$D/orig.log" 2>/dev/null || ! grep -q 'watch 1:' "$D/orig.log"; then
	echo "ORIGINAL RUN FAILED: no log with the watched values (see $D/sbox.log)"; exit 2
fi
# the port replays exactly that input (the soak would choose again, and a timing that differs a little would make it choose
# another word), the lines from the anchor on moved to the original's mark
MARK=$(grep -o "mark: the counter reads [0-9]*" "$D/orig.log" | head -1 | awk '{print $5}')
SHIFT=0
if [ -n "$ANCHOR" ]; then
	grep -q "waitfor.*: met" "$D/orig.log" || { echo "ORIGINAL RUN FAILED: its engine was never made (waitfor not met)"; exit 2; }
	SHIFT=$((MARK - ANCHOR)); echo "the original marked update $MARK: the replay's lines from $ANCHOR on move by $SHIFT"
fi
( awk -v a="${ANCHOR:-999999999}" -v s=$SHIFT '{ if ($1 >= a) $1 += s; print }' "$D/inputs.txt"; echo "$((END + SHIFT)) quit" ) | sort -n -s -k1,1 > "$D/replay.txt"
OPTS=$(python "$TOOLS/calib.py" "$D/orig.log" "$D/cal/state.txt")
echo "load lengths of the original: $OPTS"
portrun "$D/rep" -seed=$SEED -reseedengine=$SEED $OPTS -script="$D/replay.txt" -statelog="$D/state.txt" > /dev/null
stage "port, replay"
cp "$D/rep/out.txt" "$D/out.txt"
python "$TOOLS/rngcmp.py" "$D/orig.log" "$D/state.txt"
python "$TOOLS/statecmp.py" "$D/orig.log" "$D/state.txt"
stage "comparison"
