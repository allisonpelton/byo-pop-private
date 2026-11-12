#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch). The game's own score file and saved
# games are removed first (KEEP_SAVE=1 keeps the saved games a test put there), so every run starts the way a first run does. The log is
# on stdout.
# COVERAGE=<function list, a Windows path> adds origrun's --coverage (docs/COVERAGE.md); the hits are written to
# build/origrun/<stage name>/coverage.txt.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
if [ -z "$KEEP_SAVE" ]; then
	rm -f "$OG_GAME"/Players/hiscores.dat "$OG_GAME"/Players/save?.dat
fi
W=""; for e in "$@"; do W="$W --watch $e"; done
# RM_REG="Name=value,...": the settings the original starts with, in HKCU\Software\PopCap\RocketMania (--reg-map popcap sends its
# HKLM calls there), deleted and written again before every run
REGMAP=""
if [ -n "$RM_REG" ]; then
	REGMAP="--reg-map popcap"
	MSYS_NO_PATHCONV=1 reg.exe delete 'HKCU\Software\PopCap\RocketMania' /f > /dev/null 2>&1
	for kv in ${RM_REG//,/ }; do MSYS_NO_PATHCONV=1 reg.exe add 'HKCU\Software\PopCap\RocketMania' /v "${kv%%=*}" /t REG_DWORD /d "${kv#*=}" /f > /dev/null; done
fi
"$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/RocketManiaGame.exe")" --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter $COUNTER --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/rocketmania.txt --timeout "$T" $REGMAP $W ${COVERAGE:+--coverage "$COVERAGE"}
