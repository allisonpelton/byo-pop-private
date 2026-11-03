#!/bin/bash
# Shared settings of the identical-play scripts (games/rocketmania/docs/IDENTICAL_PLAY.md). Source it, do not run it.
#   OG_STAGE   where the original is staged (default C:/byo-og/rm; a short path, outside the repository)
#   OUT        where a case's files go (default build/identical)
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 1
OG_STAGE="${OG_STAGE:-C:/byo-og/rm}"
OG_GAME="$OG_STAGE/game"
OUT="${OUT:-build/identical}"
TOOLS=games/rocketmania/tools/identical
RUNDESK="build/native/rundesk.exe"
ORIGRUN="build/origrun-bin/origrun.exe"
win() { cygpath -w "$1"; }
# the app is a static object at 0x4f6460; its update counter is +0x3f0 and its board pointer +0x598
APP=0x4f6460
COUNTER=0x4f6850
BOARD=0x4f69f8
