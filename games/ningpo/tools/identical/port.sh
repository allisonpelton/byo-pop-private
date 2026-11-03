#!/bin/bash
# port.sh <port script> <seed> <state log> [exe]: the port (-fast, 10 ms of game time per update) on a script, under
# rundesk, from fresh settings, writing the values that decide play (-statelog) to <state log>
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; LOG="$3"; EXE="${4:-build/native/ningpo.exe}"
U="$(dirname "$LOG")/user"
[ -n "$PORT_KEEP_USER" ] || rm -rf "$U"  # PORT_KEEP_USER=1: start from the settings already in <log folder>/user
mkdir -p "$U"; rm -f "$LOG"
timeout 300 "$RUNDESK" "$(win "$ROOT/$EXE")" -gamedir=assets/ningpo -userdir="$U" -fast -seed="$SEED" -script="$S" -statelog="$LOG" \
	> "$(dirname "$LOG")/port-stdout.txt" 2>&1
