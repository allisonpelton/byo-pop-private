#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch). The game's high-score files,
# saved games and pack copy are removed first (KEEP_SAVE=1 keeps the saved games a test put there), and its registry
# key is set to the values port.sh gives the port (settings.reg), so every run starts from the same state. The log is on
# stdout.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
rm -f "$OG_GAME"/*.HS "$OG_GAME"/*.hs "$OG_GAME/STOMPED.TMP"
[ -n "$KEEP_SAVE" ] || rm -f "$OG_GAME"/SLOT*.SAV
reg delete 'HKCU\Software\PopCap\Dynomite' /f > /dev/null 2>&1
reg import "$(win "$TOOLS/settings.reg")" > /dev/null 2>&1
W=""; for e in "$@"; do W="$W --watch $e"; done
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/DynomiteGame.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter 0x530960 --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/dynomite.txt --timeout "$T" --watch-max 20000 $W
