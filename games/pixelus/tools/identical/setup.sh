#!/bin/bash
# Stages the original for the identical-play runs: a copy of the installed game under a short path, with the game
# (re/pixelus/bin/PixelusGame.exe, made by tools/extract_game_exe.py from the wrapped Pixelus.exe) next to it, a pristine copy of
# its levels folder (the original rewrites levels/ when a puzzle beats the best known solution), and the patch list that
# gives the run's seed to both calls of srand(GetTickCount()) in SexyApp::Init. Nothing here runs the game.
source "$(dirname "$0")/common.sh"
[ -f re/pixelus/bin/PixelusGame.exe ] || { echo "re/pixelus/bin/PixelusGame.exe is missing (tools/extract_game_exe.py)"; exit 1; }
"$ORIGRUN" --game pixelus --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" --copy-only || exit 1
cp re/pixelus/bin/PixelusGame.exe "$OG_GAME/PixelusGame.exe" || exit 1
rm -rf "$OG_STAGE/levels.orig"; cp -r "$OG_GAME/levels" "$OG_STAGE/levels.orig"
mkdir -p tools/origrun/patches
{
	echo 'seed PixelusGame.exe va:0x4a0ee3 ff15dcb05000 b8{seed}90 SexyApp::Init: SRand(GetTickCount()) takes the run'"'"'s seed'
	echo 'seed PixelusGame.exe va:0x4a1132 ff15dcb05000 b8{seed}90 SexyApp::Init: srand(GetTickCount()) takes the run'"'"'s seed'
} > tools/origrun/patches/pixelus.txt
echo "staged in $OG_GAME"
