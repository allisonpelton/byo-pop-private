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
# (origrun's virtual clock would move on at each read past 200 in a pass, and TipTop reads it up to about 500 times).
# rand also copies the generator's state to 0x48cff4 and counts its draws at 0x48cff8, through 22 bytes in the zero tail
# of the code section.
#
# For phase two: typed characters reach the game, though DirectInput has no keyboard in the sandbox and reports the
# keyboard lost at every poll of a hidden window (which made the poll throw the characters away); PlaySound plays
# though the window is not in front (the port asks only whether it is active); and Blit (0x430190), which locks the
# screen for its software blits, also keeps its pixel pointer at 0x48cfe0 and its pitch at 0x48cfe4 (the data
# section's zero tail), through a second cave after the first: inside Windows Sandbox PrintWindow sees
# none of DirectDraw's picture, so origrun reads the frame from the game's memory (shots/orig.sh, --surface-screen).
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
	echo 'test TipTop.exe va:0x431c66 c60540b0480000 90909090909090 the keyboard stays on without a DirectInput keyboard (the sandbox): typed characters reach the game'
	echo 'test TipTop.exe va:0x4317b2 e8e9050000 9090909090 the keyboard poll keeps typed characters when DirectInput says the keyboard was lost (a hidden window never has it)'
	echo 'test TipTop.exe va:0x43300c 0f85d3000000 909090909090 PlaySound plays without the window in front (a hidden window never is; the port asks whether it is active)'
	echo 'test TipTop.exe va:0x430330 8b9424b8000000 e99bbc03009090 Blit keeps the locked screen (phase two)'
	echo 'test TipTop.exe va:0x46bfd0 000000000000000000000000000000000000000000000000000000000000 8b442438a3e0cf48008b442424a3e4cf48008b9424b8000000e94943fcff the cave: the locked pixels at 0x48cfe0, the pitch at 0x48cfe4'
} > "$F"
# the sound runs (shots/sound.sh): the music starts, and its player runs, but every call it makes to the
# MIDI device is skipped (the open answers success with no device), so its state can be watched without a sound
M=tools/origrun/patches/tiptop-music.txt
{
	grep -v 'no MIDI music' "$F"
	echo 'test TipTop.exe va:0x427f45 ff155cc24600 83c41431c090 midiOutOpen skipped (its five arguments popped): success, no device'
	for A in 427c90 427e89 427ebe; do echo "test TipTop.exe va:0x$A ff1564c24600 83c408909090 midiOutShortMsg skipped"; done
	for A in 427fd0 428022; do echo "test TipTop.exe va:0x$A ff1560c24600 83c404909090 midiOutClose skipped"; done
} > "$M"
echo "staged in $OG_GAME; patches in $F and $M"
