#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch). The game's own score file and saved
# game are removed first (KEEP_SAVE=1 keeps the saved game a test put there), so every run starts the way a first run does. The log is on stdout.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
rm -f "$OG_GAME/hiscores.dat"
[ -n "$KEEP_SAVE" ] || rm -f "$OG_GAME/savegame.dat"
W=""; for e in "$@"; do W="$W --watch $e"; done
[ -z "$WATCHMAX" ] || W="$W --watch-max $WATCHMAX"
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/AlchemyGame.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter 0x516570 --update-ms 10 --seed "$SEED" \
	--patches "${PATCHES:-tools/origrun/patches/alchemy.txt}" --timeout "$T" $W
