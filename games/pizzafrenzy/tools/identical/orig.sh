#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on
# origrun's virtual clock and scripted cursor, with up to four watched values (origrun --watch); the update counter is
# the patched frame count (setup.sh). The log is on stdout.
#
# The registry: the game keeps its players and settings under HKCU\Software\GameHouse\Pizza Frenzy. A run starts the
# way a first run does only when the key is not there, so orig.sh refuses to start when it is (it never removes a key it
# did not make). REG=<file.reg> imports values first (a player, say). After the run, EXPORT=<file.reg> exports the key,
# and the key the run made is removed, which puts the registry back as it was. Git Bash would turn reg's switches (/f)
# into paths, so the reg commands run with MSYS2_ARG_CONV_EXCL.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
reg_cmd() { MSYS2_ARG_CONV_EXCL='*' reg "$@"; }
if reg_cmd query "$REGKEY" > /dev/null 2>&1; then
	echo "orig.sh: $REGKEY is there already; it is left alone, and the run does not start"
	exit 1
fi
[ -n "$REG" ] && { reg_cmd import "$(win "$REG")" > /dev/null 2>&1 || echo "reg import $REG failed"; }
W=""; for e in "$@"; do W="$W --watch $e"; done
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/PizzaFrenzyGame.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter 0x541a00 --update-ms 33 --seed "$SEED" \
	--watch-max 1000000 --patches "$PATCHES" --timeout "$T" $W
[ -n "$EXPORT" ] && { reg_cmd export "$REGKEY" "$(win "$EXPORT")" /y > /dev/null 2>&1 || echo "reg export failed"; }
# the key this run made (there was none before it)
reg_cmd delete "$REGKEY" /f > /dev/null 2>&1
