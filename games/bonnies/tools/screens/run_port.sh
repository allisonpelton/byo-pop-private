#!/usr/bin/env bash
# run_port.sh <script> <outdir> [extra args]: the port on a script, fast, seed 1, scale 1, fresh user folder unless $USERDIR is set
cd "$(dirname "$0")/../.." || exit 1
R="$(pwd -W)"; S="$1"; D="$2"; shift 2
rm -rf "$D"; mkdir -p "$D/user"
[ -n "$SAVES" ] && cp -r "$SAVES"/. "$D/user/"
Dw="$R/$D"
sed "s#@DIR@#$Dw#g" "$S" > "$D/script.txt"
until python -I tools/gamecount.py --limit 6 >/dev/null; do sleep 10; done
build/native/rundesk.exe "$R/build/native/bonnies.exe" -gamedir=assets/bonnies -userdir="$Dw/user" -fast -drawevery -seed=1 -renderscale=1 -script="$Dw/script.txt" "$@" > "$D/out.log" 2>&1
echo "exit $?"
