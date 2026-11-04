#!/bin/bash
# sbox.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: like orig.sh, but in Windows Sandbox (the host run of this
# original does not get past its title; see IDENTICAL_PLAY.md). Only one sandbox may run on the machine at a time; origrun holds a lock for it and waits its turn. The
# registry preset is build/identical/orig16.reg (orig.reg in UTF-16). Extra origrun options go in ORIG_ARGS.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
# REGFILE: another preset (UTF-16) than the default; PROFILE: a folder with users/<name>.bwa that goes onto the sandbox's C: where the
# original keeps its players (C:\ProgramData\WildTangent\BWAVol2), for a run that continues a saved profile
REG="${REGFILE:-$OUT/orig16.reg}"
rm -rf build/origrun/bwa2s/sandbox-files
if [ -n "$PROFILE" ]; then mkdir -p build/origrun/bwa2s/sandbox-files/C/ProgramData/WildTangent/BWAVol2; cp -r "$PROFILE/users" build/origrun/bwa2s/sandbox-files/C/ProgramData/WildTangent/BWAVol2/; fi
[ -f "$OUT/orig16.reg" ] || python "$TOOLS/utf16.py" "$TOOLS/orig.reg" > "$OUT/orig16.reg"
W=""; for e in "$@"; do W="$W --watch $e"; done
"$RUNDESK" "$(win "$ORIGRUN")" --game bookwormadv2 --run bwa2s --sandbox --reg "$(win "$REG")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter "$COUNTER" --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/bookwormadv2.txt --timeout "$T" $ORIG_ARGS $W
