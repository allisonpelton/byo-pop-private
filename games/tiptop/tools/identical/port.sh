#!/bin/bash
# port.sh <out dir> <port arguments...>: the port (PORT_EXE, default build/native/tiptop.exe) under rundesk (invisible,
# silent, low priority) with -fast, on a fresh user dir inside the out dir (KEEP_USER=1 keeps the one a run before
# left). Its output goes to <out dir>/out.txt.
source "$(dirname "$0")/common.sh"
D="$1"; shift
mkdir -p "$D"; [ -n "$KEEP_USER" ] || rm -rf "$D/user"
"$RUNDESK" "$(win "$PORT_EXE")" -gamedir="$(win assets/tiptop)" -userdir="$(win "$D/user")" -fast "$@" > "$D/out.txt" 2>&1
echo "exit $?"
