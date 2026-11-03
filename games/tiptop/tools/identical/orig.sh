#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch). The game's preferences (which
# hold the high scores) and its saved game are removed first (KEEP_SAVE=1 keeps both, as a test put them there), so
# every run starts the way a first run does. The log is on stdout.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
[ -n "$KEEP_SAVE" ] || rm -f "$OG_GAME/Preferences.dat" "$OG_GAME/savegame.dat"
W=""; for e in "$@"; do W="$W --watch $e"; done
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/TipTop.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter 0x48cff0 --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/tiptop.txt --timeout "$T" $W
