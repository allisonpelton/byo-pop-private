#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <name> [<name> ...]: the original on a script, under rundesk (origrun starts the game there itself), on origrun's
# virtual clock and scripted cursor, watching up to four of the values in values.txt (origrun --watch). Every run starts
# as a first run does: a fresh copy of setup.sh's master and no HKCU\Software\PopCap\BookWorm (the user allows changing
# PopCap games' registry values, CLAUDE.md). Before the key is removed it is exported to build/identical/regbackup/, so
# nothing the original wrote there is lost for good. KEEP_SAVE=1 keeps the copy and the key a run before left (a saved
# game, the high scores); ORIG_SAVE=<dir> copies a folder's files into the fresh copy. The log is on stdout, and after
# it the high-score tables the run left, as lines "scores: <reg query's line>". CLOCK=real runs on the wall clock instead
# (the coverage run that makes the sound code run, tools/coverage/run.sh).
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
if [ -z "$KEEP_SAVE" ]; then
	rm -rf "$OG_GAME" && cp -r "$OG_MASTER" "$OG_GAME" || exit 1
	if reg query "$REGKEY" > /dev/null 2>&1; then
		mkdir -p "$OUT/regbackup"
		B="$OUT/regbackup/BookWorm-$(date +%Y%m%d-%H%M%S)-$$.reg"
		reg export "$REGKEY" "$(win "$B")" /y > /dev/null || { echo "orig.sh: could not back up $REGKEY"; exit 1; }
		reg delete "$REGKEY" /f > /dev/null
	fi
	[ -n "$ORIG_SAVE" ] && cp -r "$ORIG_SAVE"/. "$OG_GAME"/
fi
W=()
for n in "$@"; do
	e=$(awk -v n="$n" '$1 == n { print $2 }' "$TOOLS/values.txt")
	[ -n "$e" ] || { echo "orig.sh: no value $n in values.txt"; exit 2; }
	e="${e//B/[$APP+0x1160]}"
	W+=(--watch "${e//APP/$APP}")
done
"$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/BookwormGame.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock "${CLOCK:-virtual}" --cursor scripted --counter "$COUNTER" --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/bookworm.txt --timeout "$T" --watch-max 20000 ${COVERAGE:+--coverage "$COVERAGE"} "${W[@]}"
st=$?
# the high-score tables the run left (setup.sh's patches keep them in HKCU), for hiscore.py
reg query "$REGKEY\Scores" 2> /dev/null | sed 's/^/scores: /'
exit $st
