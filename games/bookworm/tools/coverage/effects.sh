#!/bin/bash
# effects.sh <mode> <bot seed> <game seed> <cap>...: the first session of `run.sh save` for each cap in turn (a game quit at
# that update, saved as the window closes), to catch the effect lists with effects in them: after each run, which of the
# Save functions of the effects (Sparkle 0x41eb00, Ember 0x41f0a0, FlyingPieceList 0x41fee0) the run reached.
cd "$(dirname "$0")/../../../.." || exit 1
export OG_STAGE="${OG_STAGE:-C:/byo-og/bwp}"
MODE=$1; BS=$2; GS=$3; shift 3
for CAP in "$@"; do
	ONLY1=1 bash games/bookworm/tools/coverage/run.sh save "fx-$MODE$GS-$CAP" "$MODE" "$BS" "$GS" "$CAP" > /dev/null 2>&1
	H=build/coverage/bookworm/hits/fx-$MODE$GS-$CAP-1.txt
	echo "cap $CAP: $(grep -c . "$H" 2>/dev/null) hits; effect saves: $(grep -E '^(0041eb00|0041f0a0|0041fee0|0041e8d0) ' "$H" 2>/dev/null | cut -d' ' -f1 | tr '\n' ' ')"
done
echo "effects done"
