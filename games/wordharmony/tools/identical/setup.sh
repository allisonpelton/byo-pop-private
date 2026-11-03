#!/bin/bash
# Stages the original for the identical-play runs: a copy of the installed game under a short path, with the game carved
# out of WordHarmony.exe (re/wordharmony/bin/WordHarmonyGame.exe) next to it, and the patch list: the run's seed for
# srand(timeGetTime()), a frame counter for origrun (the engine counts no updates), and frames of exactly the frame period. Nothing here runs the game.
source "$(dirname "$0")/common.sh"
[ -f re/wordharmony/bin/WordHarmonyGame.exe ] || { echo "re/wordharmony/bin/WordHarmonyGame.exe is missing (tools/extract_game_exe.py)"; exit 1; }
"$ORIGRUN" --game wordharmony --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" --copy-only || exit 1
cp re/wordharmony/bin/WordHarmonyGame.exe "$OG_GAME/WordHarmonyGame.exe" || exit 1
mkdir -p tools/origrun/patches
# The counter: the frame (0x4542e0) starts with a jump to the zero bytes past the end of .text (file 0x7f510, made part of
# the mapped section by the header's VirtualSize), which adds one to a dword in the zero tail of .data (0x4aa600), runs the
# two pushes the jump replaced and jumps back. The step: the frame's elapsed time is always the period, 33 ms (the virtual
# clock crawls while the game works between two waits, and the loop's millisecond carry then gives frames of 32 to 37 ms).
# The hints: the original searches them on a thread that reads the balls while the game moves them, with no lock, and kills
# it with TerminateThread when a new search starts; on the virtual clock that hangs the game within a minute of TSUNAMI.
# The copy searches at once on the main thread, as the port does.
cat > tools/origrun/patches/wordharmony.txt <<'P'
seed WordHarmonyGame.exe va:0x406c14 ff1534004800 b8{seed}90 srand(timeGetTime()) in the game's init takes the run's seed
test WordHarmonyGame.exe off:0x1f8 0ae50700 00f00700 .text's VirtualSize covers its file tail (the counter's code)
test WordHarmonyGame.exe off:0x7f510 000000000000000000000000000000000000 ff0500a64a006aff68d8d64700e9c54dfdff the counter's code: inc [0x4aa600], the two pushes, jmp 0x4542e7
test WordHarmonyGame.exe va:0x4542e0 6aff68d8d64700 e92bb202009090 the frame jumps to the counter's code
test WordHarmonyGame.exe off:0x7f522 00000000000000000000000000 8bf8b82100000050e9ec4dfdff the step's code: the time kept, the elapsed ms 33, pushed, jmp 0x45431b
test WordHarmonyGame.exe va:0x454316 8bf82bc250 e907b20200 the frame's elapsed time is the frame period (the step's code)
test WordHarmonyGame.exe va:0x419560 7518 eb18 startHints never finds a hint thread running
test WordHarmonyGame.exe va:0x4195ac 8d442408506a006a0068e09141006a006a00ff1528004800 6a00e82dfcffff31c0909090909090909090909090909090 startHints runs the hint search (0x4191e0) at once instead of on a thread; the handle is 0
P
echo "staged in $OG_GAME"
