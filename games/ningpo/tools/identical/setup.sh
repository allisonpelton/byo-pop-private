#!/bin/bash
# Stages the original for the identical-play runs: a copy of the installed game under a short path, and the patch list
# (tools/origrun/patches/ningpo.txt, git-ignored) with the run's seed and the registered game. Nothing here runs the game.
source "$(dirname "$0")/common.sh"
"$ORIGRUN" --game ningpo --run "$(basename "$OG_STAGE")" --stage-root "$(dirname "$OG_STAGE")" --copy-only || exit 1
mkdir -p tools/origrun/patches
{
	# PGApp::mainLoop seeds the generator with the first event's time (push eax at 0x40d8c6); the port's -seed=N
	# seeds it with N, so the copy pushes the run's seed instead (lea ecx; mov edx,[ecx]; push {seed})
	echo 'seed Ningpo.exe va:0x40d8ba 8b96345f00008d8e345f000050 8d8e345f00008b1168{seed} mainLoop seeds the generator with the run'"'"'s seed'
	# the port plays as the registered game: NingPo::checkRegistration (0x405cd0) answers true
	echo 'test Ningpo.exe va:0x405cd0 568bf1 b001c3 checkRegistration answers registered, as the port plays'
} > tools/origrun/patches/ningpo.txt
echo "staged in $OG_GAME"
