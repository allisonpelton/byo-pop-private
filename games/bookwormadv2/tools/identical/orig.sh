#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch). The log is on stdout. Extra
# origrun options go in ORIG_ARGS (e.g. "--sound nodriver"). KEEP_SAVE=1 keeps the profile a test put there.
source "$(dirname "$0")/common.sh"
export MSYS_NO_PATHCONV=1  # reg.exe takes /f and /y as switches
S="$1"; SEED="$2"; T="$3"; shift 3
# the original keeps its settings in HKCU\Software\WildTangent\BWAVol2 (the user's own, for the Steam copy): the run
# starts from orig.reg (windowed, software renderer, no volume) and the user's key is put back when it ends
KEY='HKCU\Software\WildTangent\BWAVol2'
BACKUP="$OUT/user-bwa2-registry-backup.reg"
mkdir -p "$OUT"
[ -f "$BACKUP" ] || reg export "$KEY" "$(win "$BACKUP")" /y > /dev/null 2>&1
restore() { reg delete "$KEY" /f > /dev/null 2>&1; [ -f "$BACKUP" ] && reg import "$(win "$BACKUP")" > /dev/null 2>&1; }
trap restore EXIT
reg delete "$KEY" /f > /dev/null 2>&1
reg import "$(win "$TOOLS/orig.reg")" > /dev/null 2>&1
W=""; for e in "$@"; do W="$W --watch $e"; done
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/BookwormAdventuresVol2.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter "$COUNTER" --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/bookwormadv2.txt --timeout "$T" $ORIG_ARGS $W
