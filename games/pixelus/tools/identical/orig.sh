#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch). The game's saves (users/) and its
# levels folder are put back first (KEEP_SAVE=1 keeps the users folder a test put there), so every run starts the way a
# first run does. The log is on stdout.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
[ -n "$KEEP_SAVE" ] || rm -rf "$OG_GAME/users"
cp -r "$OG_STAGE/levels.orig/." "$OG_GAME/levels/"
W=(); for e in "$@"; do W+=(--watch "$e"); done
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/PixelusGame.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter 0x543270 --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/pixelus.txt --timeout "$T" $EXTRA "${W[@]}"
