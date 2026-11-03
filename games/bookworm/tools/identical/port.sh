#!/bin/bash
# port.sh <out dir> <port arguments...>: the port under rundesk (invisible, silent, low priority) with -fast, on a fresh
# user dir inside the out dir (KEEP_USER=1 keeps the one a run before left; PORT_SAVE=<dir> copies a folder's files into
# the fresh one) and no mods. Its output goes to <out dir>/out.txt. BW_EXE names another exe (a frozen baseline).
source "$(dirname "$0")/common.sh"
D="$1"; shift
EXE="${BW_EXE:-build/native/bookworm.exe}"
mkdir -p "$D"
if [ -z "$KEEP_USER" ]; then
	rm -rf "$D/user"; mkdir -p "$D/user"
	[ -n "$PORT_SAVE" ] && cp -r "$PORT_SAVE"/. "$D/user"/
fi
mkdir -p "$D/mods"
timeout "${PORT_TIMEOUT:-600}" "$RUNDESK" "$(win "$EXE")" -userdir="$(win "$D/user")" -modsdir="$(win "$D/mods")" -fast "$@" \
	> "$D/out.txt" 2>&1
echo "exit $?"
