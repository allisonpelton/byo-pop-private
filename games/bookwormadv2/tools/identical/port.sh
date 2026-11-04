#!/bin/bash
# port.sh <out dir> <port arguments...>: the port under rundesk (invisible, silent, low priority) with -fast, on a fresh
# user dir inside the out dir (KEEP_USER=1 keeps the one a run before left). Its output goes to <out dir>/out.txt.
source "$(dirname "$0")/common.sh"
D="$1"; shift
case "$D" in /*|?:*) ;; *) D="$ROOT/$D" ;; esac  # the port wants an absolute -userdir (a relative one nests)
mkdir -p "$D"; [ -n "$KEEP_USER" ] || rm -rf "$D/user"
"$RUNDESK" "$(win "$PORT_EXE")" -gamedir="$(win "$GAMEDIR")" -userdir="$(win "$D/user")" -fast "$@" > "$D/out.txt" 2>&1
echo "exit $?"
