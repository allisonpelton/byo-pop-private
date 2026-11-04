#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch). The game's own score file and saved
# games are removed first (KEEP_SAVE=1 keeps the saved games a test put there), so every run starts the way a first run does. The log is
# on stdout.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
if [ -z "$KEEP_SAVE" ]; then
	rm -f "$OG_GAME"/Players/hiscores.dat "$OG_GAME"/Players/save?.dat
fi
W=""; for e in "$@"; do W="$W --watch $e"; done
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/RocketManiaGame.exe")" --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter $COUNTER --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/rocketmania.txt --timeout "$T" $W
