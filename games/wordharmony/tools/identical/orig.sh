#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch); the update counter is the patched
# frame count. The game's registry key (the settings, the saved games and the high scores) is cleared first, so every run
# starts the way a first run does; KEEP_REG=1 keeps it, REG=<file.reg> imports values after the clearing. The log is on stdout.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
if [ -z "$KEEP_REG" ]; then
	reg delete "$REGKEY" /f > /dev/null 2>&1
	reg add "$REGKEY" /v InstallCheck /t REG_SZ /d true /f > /dev/null
fi
[ -n "$REG" ] && { reg import "$(win "$REG")" > /dev/null 2>&1 || echo "reg import $REG failed"; }
W=""; for e in "$@"; do W="$W --watch $e"; done
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/WordHarmonyGame.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter 0x4aa600 --update-ms 33 --seed "$SEED" --watch-max 1000000 \
	--patches tools/origrun/patches/wordharmony.txt --timeout "$T" $W
