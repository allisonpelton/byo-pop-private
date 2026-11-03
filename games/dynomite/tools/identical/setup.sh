#!/bin/bash
# Stages the original for the identical-play runs: a copy of the installed game under a short path, with the game
# carved out of Dynomite.exe (re/dynomite/bin/DynomiteGame.exe) next to it, and the patch list that gives the run's seed
# to every generator the game seeds from GetTickCount. Nothing here runs the game.
source "$(dirname "$0")/common.sh"
[ -f re/dynomite/bin/DynomiteGame.exe ] || { echo "re/dynomite/bin/DynomiteGame.exe is missing"; exit 1; }
"$ORIGRUN" --game dynomite --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" --copy-only || exit 1
cp re/dynomite/bin/DynomiteGame.exe "$OG_GAME/DynomiteGame.exe" || exit 1
mkdir -p tools/origrun/patches
G=DynomiteGame.exe
{
	echo "seed $G va:0x4010c5 ff156c704f00 b8{seed}90 the launcher's own MTRand (never drawn from) takes the run's seed"
	echo "seed $G va:0x443054 909090909090 b8{seed}c3 a stub in the padding after SRand (0x443040) that returns the seed"
	echo "seed $G va:0x419a3f 8b3d6c704f00 bf5430440090 the app's constructor calls the stub for its MTRand and its FibRand (seed >> 1)"
	echo "seed $G va:0x48c5d2 ff156c704f00 b8{seed}90 SexyApp::Init's global MTRand (0x532060) takes the run's seed"
	echo "seed $G va:0x48c83c ff156c704f00 b8{seed}90 SexyApp::Init's srand takes the run's seed"
} > tools/origrun/patches/dynomite.txt
echo "staged in $OG_GAME"
