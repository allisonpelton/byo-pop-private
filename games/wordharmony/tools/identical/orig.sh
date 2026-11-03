#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch); the update counter is the patched
# frame count. The game's registry key (the settings, the saved games and the high scores) is cleared first, so every run
# starts the way a first run does; KEEP_REG=1 keeps it, REG=<file.reg> imports values after the clearing, EXPORT=<file.reg>
# exports the key after the run (savedgame.py reads it). The log is on stdout. Git Bash would turn reg's switches (/f) into
# paths, so the reg commands run with MSYS2_ARG_CONV_EXCL.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
if [ -z "$KEEP_REG" ]; then
	MSYS2_ARG_CONV_EXCL='*' reg delete "$REGKEY" /f > /dev/null 2>&1
	MSYS2_ARG_CONV_EXCL='*' reg query "$REGKEY" > /dev/null 2>&1 && echo "the registry key could not be cleared"
fi
[ -n "$REG" ] && { reg import "$(win "$REG")" > /dev/null 2>&1 || echo "reg import $REG failed"; }
W=""; for e in "$@"; do W="$W --watch $e"; done
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/WordHarmonyGame.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter 0x4aa600 --update-ms 33 --seed "$SEED" --watch-max 1000000 \
	--patches tools/origrun/patches/wordharmony.txt --timeout "$T" $W
[ -n "$EXPORT" ] && { MSYS2_ARG_CONV_EXCL='*' reg export "$REGKEY" "$(win "$EXPORT")" /y > /dev/null 2>&1 || echo "reg export failed"; }
