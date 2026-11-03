#!/bin/bash
# run_port.sh <worktree> <port log> <bot arguments...>: the port's bot with the state trace, fast, in a fresh user folder
R="$1"; L="$2"; shift 2
U="$R/build/phase1/u"; rm -rf "$U"; mkdir -p "$U"
"$R/build/native/mummymaze.exe" -userdir="$U" -fast -autoplaytrace -autoplaylog="$L" "$@" > "$L.stdout" 2> "$L.stderr"
