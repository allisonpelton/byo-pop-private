#!/bin/bash
# Stages the original for the identical-play runs: a copy of the Steam install under a short path (the launcher exits
# when its folder path is too deep) and the one-line patch list that gives the run's seed to SexyAppBase::Init's
# SRand(GetTickCount()). The game is not wrapped, so nothing is carved out. Nothing here runs the game.
source "$(dirname "$0")/common.sh"
"$ORIGRUN" --game bookwormadv2 --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" --copy-only || exit 1
mkdir -p tools/origrun/patches
echo 'seed BookwormAdventuresVol2.exe va:0x5593af ffd7894308 b8{seed} SexyAppBase::Init: SRand(GetTickCount()) takes the run'"'"'s seed (the store into mRandSeed at +8 is dropped; nothing reads it)' > tools/origrun/patches/bookwormadv2.txt
echo "staged in $OG_GAME"
