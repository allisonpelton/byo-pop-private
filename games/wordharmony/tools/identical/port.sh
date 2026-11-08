#!/bin/bash
# port.sh <out dir> <port arguments...>: the port under rundesk (invisible, silent, low priority) with -fast, on a fresh
# user dir inside the out dir (KEEP_USER=1 keeps the one a run before left), with the original's splash hold (-splashhold) so
# that its frames line up with the original's. Its output goes to <out dir>/out.txt.
source "$(dirname "$0")/common.sh"
D="$1"; shift
mkdir -p "$D"; [ -n "$KEEP_USER" ] || rm -rf "$D/user"
"$RUNDESK" "$(win build/native/wordharmony.exe)" -gamedir="$(win assets/wordharmony)" -userdir="$(win "$D/user")" -fast -splashhold "$@" > "$D/out.txt" 2>&1
echo "exit $?"
