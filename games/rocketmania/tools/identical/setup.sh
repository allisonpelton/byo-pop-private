#!/bin/bash
# Stages the original for the identical-play runs: a copy of the installed game under a short path, with the game carved
# out of RocketMania.exe (re/rocketmania/bin/RocketManiaGame.exe, made with the private tools/extract_game_exe.py) next to
# it, and the one-line patch list that gives the run's seed to srand(GetTickCount()). Nothing here runs the game.
source "$(dirname "$0")/common.sh"
[ -f re/rocketmania/bin/RocketManiaGame.exe ] || { echo "re/rocketmania/bin/RocketManiaGame.exe is missing"; exit 1; }
"$ORIGRUN" --game rocketmania --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" --copy-only || exit 1
cp re/rocketmania/bin/RocketManiaGame.exe "$OG_GAME/RocketManiaGame.exe" || exit 1
mkdir -p tools/origrun/patches
echo 'seed RocketManiaGame.exe va:0x46d331 ff15f8414c00 b8{seed}90 SRand(GetTickCount()) in SexyAppBase::Init takes the run'"'"'s seed' > tools/origrun/patches/rocketmania.txt
# the PNG loader (0x473100) sizes its pixel buffer from png_get_rowbytes before png_read_update_info (66 bytes for the
# 1-bit _Arial8.png, 528 pixels wide) times height times 4, so a 1-bit image overflows the buffer by about 26 KB: on a fresh
# heap that runs off the end of the committed pages and the game dies in the loading screen. The patch sizes it as
# width * height * 4, which is what the loop writes.
echo 'png RocketManiaGame.exe va:0x473216 e88ba100000faf45f4c1e002 8b45ec0faf45f4c1e0026690 the PNG loader sizes its buffer as width * height * 4 (it used the untransformed row bytes)' >> tools/origrun/patches/rocketmania.txt
echo "staged in $OG_GAME"
