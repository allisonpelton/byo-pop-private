#!/usr/bin/env bash
# One coverage run of the original on the host (docs/COVERAGE.md): a phase 2 tour's origrun script without its shots (a host run takes
# none), on the virtual clock with the scripted cursor, with origrun's --coverage on the list that
# `python -I tools/coverage.py list typershark` writes and phase 2's patch list (its loading patch makes the copy load in one go,
# so the first typed keys meet a loaded game).
#   games/typershark/tools/coverage/run.sh <name> <script> [timeout s] [seed] [extra origrun options...]
# <script> is an origrun script: build/ts-p2/<tour>/orig.txt, or one of this folder's own (extra*.txt). Each run starts from a fresh
# users folder and registry key (as phase 1's runorig.sh does) unless KEEP=1 is set, for a run that goes on from what the last one
# left (it needs --reg-map popcap on both, so that the registry values are kept; docs/ORIGRUN.md, "The registry map"). The hits go to
# build/coverage/typershark/hits/<name>.txt, the run's log beside them; "--clock real --sound wav" as extra options gives a run on the
# real clock with the silent recording stand-in, so that the sound code runs too.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
NAME="$1"; SRC="$2"; T="${3:-900}"; SEED="${4:-1}"; shift 4 2>/dev/null || shift $#
H=build/coverage/typershark/hits; mkdir -p "$H"
S="$H/$NAME.script.txt"
# the last input is read an update after its line says, and `quit` ends the run at once: 300 updates of tail let it be handled
LAST="$(sed -nE 's/^at ([0-9]+) .*/\1/p' "$SRC" | sort -n | tail -1)"
{ grep -qE '^at 0 focus regain' "$SRC" || echo "at 0 focus regain"
  grep -vE '^(at [0-9]+ )?(shot|waitshot|record|burst)( |$)' "$SRC" | grep -v '^quit$'
  echo "at $((LAST + 300)) mark"; echo quit; } > "$S"
GAME=build/origrun/typershark/game
rm -f build/origrun/typershark/coverage.txt
if [ -z "${KEEP:-}" ]; then
	rm -rf "$GAME/users"; [ -n "${NOUSERS:-}" ] || cp -r build/ts-p1/fixtures/users-clean "$GAME/users"   # NOUSERS=1: no users folder at all
	reg delete 'HKCU\Software\PopCap\TyperShark' //f > /dev/null 2>&1
fi
PATCHES=(--patches "$R/build/ts-p2/patches.txt"); [ -n "${NOPATCH:-}" ] && PATCHES=()   # NOPATCH=1: the copy loads on its own thread, the title screen draws
bash games/typershark/tools/run_original.sh "$R/$S" "$H/$NAME.log" "$SEED" "$T" \
	--coverage "$R/build/coverage/typershark/functions.txt" --cursor scripted --reg-map popcap "${PATCHES[@]}" "$@"
cp build/origrun/typershark/coverage.txt "$H/$NAME.txt" || { echo "$NAME: no hits file"; tail -3 "$H/$NAME.log"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
