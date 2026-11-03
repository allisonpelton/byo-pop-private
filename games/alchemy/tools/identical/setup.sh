#!/bin/bash
# Stages the original for the identical-play runs: a copy of the installed game under a short path, with the game
# carved out of Alchemy.exe (re/alchemy/bin/AlchemyGame.exe) next to it, and the one-line patch list that gives the run's
# seed to srand(GetTickCount()). Nothing here runs the game.
source "$(dirname "$0")/common.sh"
[ -f re/alchemy/bin/AlchemyGame.exe ] || { echo "re/alchemy/bin/AlchemyGame.exe is missing (tools/extract_game_exe.py)"; exit 1; }
"$ORIGRUN" --game alchemy --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" --copy-only || exit 1
cp re/alchemy/bin/AlchemyGame.exe "$OG_GAME/AlchemyGame.exe" || exit 1
mkdir -p tools/origrun/patches
echo 'seed AlchemyGame.exe va:0x47198c ff1564e04d00 b8{seed}90 srand(GetTickCount()) in SexyApp::Init takes the run'"'"'s seed' > tools/origrun/patches/alchemy.txt
echo "staged in $OG_GAME"
