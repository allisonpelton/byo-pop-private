#!/bin/bash
# Stages the original for the identical-play runs: a copy of the installed game under a short path, and the patch list
# origrun applies to the copy (tools/origrun/patches/tiptop.txt, git-ignored). Nothing here runs the game.
#
# The patches: the start's srand(GetTickCount()) takes the run's seed, as the port's -seed does; the configuration's
# DisableUnregistered reads as set wherever the game reads it, its own path for a registered copy, as the port plays (it
# also keeps the game from writing its run count into a file in the Windows folder), and the run count's registry write
# is skipped; the music is not started (the original sends it to Windows' MIDI mapper, which origrun's silent sound does
# not cover). The main loop waits 10 ms a pass, the port's update under -fast, in place of the vertical blank (on
# origrun's virtual clock a wait for the vertical blank moves no time), and counts its passes at 0x48cff0 (the
# update counter of origrun's --counter, in the zero tail of the data section's last page). LOWOS's clock reads that
# count as 10 ms a pass, as the port's clock under -fast moves 10 ms an update: every read in a pass sees the same time
# (origrun's virtual clock would move on at each read past 200 in a pass, and TipTop reads it up to about 500 times). rand also copies the
# generator's state to 0x48cff4 and counts its draws at 0x48cff8, through 22 bytes in the zero tail of the code
# section.
source "$(dirname "$0")/common.sh"
"$ORIGRUN" --game tiptop --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" --copy-only || exit 1
mkdir -p tools/origrun/patches
F=tools/origrun/patches/tiptop.txt
FLAG='0fb605489b4800 b80100000090_90'
{
	echo 'seed TipTop.exe va:0x412021 ff1584c14600 b8{seed}90 srand(GetTickCount()) at the start takes the run'"'"'s seed'
	echo "test TipTop.exe va:0x411d13 $FLAG DisableUnregistered read as set (the run count file)"
	echo "test TipTop.exe va:0x412085 $FLAG DisableUnregistered read as set (registered)"
	echo "test TipTop.exe va:0x412535 $FLAG DisableUnregistered read as set (the run count file)"
	echo 'test TipTop.exe va:0x411f7a 55 c3 no RunCount registry write'
	echo 'test TipTop.exe va:0x41192f 55 c3 no MIDI music'
	echo 'test TipTop.exe va:0x42fc3e 743c eb3c no wait for the vertical blank'
	echo 'test TipTop.exe va:0x4128dc 0fb605f086480085c074086a32ff1520c14600 ff05f0cf48006a0aff1520c146009090909090 the loop counts its passes and waits 10 ms'
	echo 'test TipTop.exe va:0x42d0f0 a070a2480083ec08 b8e803000031d2c3 PerfFrequency: 1000'
	echo 'test TipTop.exe va:0x42d150 a070a2480083ec0884c056 a1f0cf48006bc00a31d2c3 PerfCounter: 10 ms a pass'
	echo 'test TipTop.exe va:0x44ca89 8948148bc1 e922f50100 rand goes through the cave'
	echo 'test TipTop.exe va:0x46bfb0 00000000000000000000000000000000000000000000 894814890df4cf4800ff05f8cf48008bc1e9c80afeff the cave: the state at 0x48cff4, the draws at 0x48cff8'
} > "$F"
echo "staged in $OG_GAME; patches in $F"
