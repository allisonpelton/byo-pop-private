#!/bin/bash
# port.sh <tour name> [port args]: the port on a coverage tour (tools/coverage/tours/<name>.txt, the format of tools/shots/*.txt),
# -fast under rundesk, its shots in build/coverage/bookworm/port/<name>/: where the click positions of a coverage tour of the
# original come from (the original's dialogs lie where the port's do; a host run takes no shots of its own).
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift; D=build/coverage/bookworm/port/$N
rm -rf "$D"; mkdir -p "$D/port" "$D/user"
python games/bookworm/tools/shots/make.py games/bookworm/tools/coverage/tours/$N.txt "$D" port > "$D/port.txt"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
KEEP_USER=1 PORT_TIMEOUT=300 "$TOOLS/port.sh" "$D" -seed=${GS:-1} -script="$(win "$D/port.txt")" "$@" | tail -1
ls "$D/port" | wc -l
