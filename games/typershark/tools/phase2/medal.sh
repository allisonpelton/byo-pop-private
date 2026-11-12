#!/usr/bin/env bash
# The end-of-adventure dialog on both programs (GAME_NOTES section 18): a saved adventure at the last site, the original's copy with the one
# patched byte (the space bar's step back to the map in the diver's first state), shotdiff and a summary.
#   games/typershark/tools/phase2/medal.sh <ngc: 0, 2 or 4 (bronze, silver, gold at the dialog)> [dialog|dialogkeep]
# Needs build/ts-p2/mk1 (mksaved.sh), build/ts-p2/patches.txt (the patch list of phase 2) and the fixture medal-ngc1 (mkprofile.py 1).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
N="$1"; MODE="${2:-dialog}"; TOUR="md_${MODE}_$N"
[ -s build/ts-p2/mk1/ud/users/user0.dat ] || bash games/typershark/tools/phase2/mksaved.sh || exit 1
python games/typershark/tools/phase2/mkprofile.py "$N" saved > /dev/null
python games/typershark/tools/phase2/tour_medal.py "$MODE" "$TOUR"
{ cat build/ts-p2/patches.txt; echo "test TyperSharkGame.exe va:0x454e9b 83f807 83f801 Board::KeyChar: the space bar's step back to the map is taken in the diver's first state (1), not in state 7 (the dialog is reached from a saved game without winning the level)"; } > build/ts-p2/patches-win.txt
USERS="medal-sv$N" REG="build/ts-p2/profiles/sv$N/orig.reg" STAGE="build/ts-p2/profiles/sv$N/orig.users" PATCHES=patches-win.txt \
	bash games/typershark/tools/phase2/chain.sh "$TOUR:1:420"
